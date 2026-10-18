// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atthreadtbl.c -- the platform layer's thread table, the TLS-counted lock, and the record primitives. Notes: Research/notes/atthreadtbl.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"
#define kMainBit        0
#define kOpenBit        1
#define kClosingBit     2
#define kMinStack       0x4000
#define kWaitObject0    0
#define kWaitAbandoned  0x80
#define kWaitTimeout    0x102
#define kWaitFailed     0xffffffff
#define kMainThreadId   1

typedef struct ThreadRec ThreadRec;

typedef void (*ThreadCleanup)(ThreadRec *rec, void *arg);

struct ThreadRec {
    ThreadRec *next;                             /* +0x00 list link */
    void *link;                                  /* +0x04 owner, or next */
    void *waitHandle;                            /* +0x08 */
    long reserved0c;                             /* +0x0c */
    long reserved10;                             /* +0x10 */
    ThreadCleanup cleanup;                       /* +0x14 */
    void *cleanupArg;                            /* +0x18 */
    long reserved1c;                             /* +0x1c */
    ULong id;                                    /* +0x20 */
    unsigned short index;                        /* +0x24 */
    unsigned short reserved26;                   /* +0x26 */
    void *thread;                                /* +0x28 */
    long reserved2c;                             /* +0x2c */
    long reserved30;                             /* +0x30 */
};

typedef struct ThreadSlot {
    ThreadRec *rec;                              /* +0x00 */
    unsigned short generation;                   /* +0x04 */
    unsigned short reserved;                     /* +0x06 */
} ThreadSlot;

ULong gThreadIndex = kNoTlsIndex;                /* 0x00578740 record TLS index */
ULong gThreadFlags;                              /* 0x00870178 state bits */
ThreadRec gMainThread;                          /* 0x0086ffb0 main thread */
static MutexList gThreadFreeQ;                   /* 0x00867d84 spare */
static MutexList DAT_00867d74;                   /* 0x00867d74 running */
static MutexList DAT_00867d64;                   /* 0x00867d64 running */
static MutexList DAT_00867d54;                   /* 0x00867d54 */
static char gThreadLock[16];                    /* 0x00867d44 lock token */
static ThreadRec *gThreadDying;                  /* 0x00867d40 dying list */
static ULong gThreadCount;                       /* 0x00867d3c slot count */
static ThreadSlot *gThreadList;                 /* 0x00867d38 slot table */

long _ThreadRecordCreate(ULong stackSize);
void _ThreadRecordFree(ThreadRec *rec);
ThreadRec *_ThreadRecordLookup(long id);
ULong _ThreadRecordGetId(ThreadRec *rec);
void *_ThreadRecordCurrent(void);
void _ThreadTableLock(void);
void _ThreadTableUnlock(void);

void ThreadsPostflight(void);
long ThreadDispose(ULong id);
long _ThreadRecordClose(void *thread);
unsigned char _ThreadIsAlive(void *thread);
void _ThreadListMove(void *thread, ThreadRec **head, ThreadRec **pushTo);

/* ThreadsPreflight. STATE: MATCH 100%, exe 105 / obj 105, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504026
long ThreadsPreflight(long count, ULong stackSize)
{
    long err, e;

    err = 0;
    AtomicMaskTest(&gThreadFlags, kClosingBit);
    if (stackSize < kMinStack)
        stackSize = kMinStack;
    if (AtomicMaskApply(&gThreadFlags, kMainBit) == 0) {
        MemoryClear(&gMainThread, sizeof(ThreadRec));
        gMainThread.id = kMainThreadId;
    }
    if (AtomicMaskTest(&gThreadFlags, kOpenBit) == 0) {
        e = QueueCreate(&gThreadFreeQ);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&DAT_00867d74);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&DAT_00867d64);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&DAT_00867d54);
        if (e != 0) {
            err = e;
            goto done;
        }
        gThreadList = MemoryAllocateClear(sizeof(ThreadSlot));
        if (gThreadList == 0) {
            err = kOutOfMemory;
            goto done;
        }
        gThreadList->rec = &gMainThread;
        gThreadList->generation = 1;
        gThreadCount = 1;
        if (gThreadIndex == kNoTlsIndex) {
            gThreadIndex = TlsAlloc();
            if (gThreadIndex == kNoTlsIndex) {
                err = GetLastError();
                err = kOSFailure;
                goto done;
            }
            if (TlsSetValue(gThreadIndex, &gMainThread) == 0) {
                err = GetLastError();
                err = kOSFailure;
                goto done;
            }
        }
        SystemSetQuitProc(ThreadsPostflight);
        AtomicMaskApply(&gThreadFlags, kOpenBit);
    }
    while (count--) {
        e = _ThreadRecordCreate(stackSize);
        if (e != 0) {
            err = e;
            break;
        }
    }
done:
    return err;
}

/* ThreadsPostflight. STATE: MATCH 100%, exe 83 / obj 83, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504188
void ThreadsPostflight(void)
{
    ThreadRec *rec;
    ThreadRec *thread;
    ThreadRec *dying;

    rec = 0;
    thread = 0;
    dying = 0;
    if (AtomicMaskClear(&gThreadFlags, kOpenBit) != 0 &&
        AtomicMaskApply(&gThreadFlags, kClosingBit) == 0) {
        _ThreadTableLock();
        while ((rec = (ThreadRec *)QueueRemove(&DAT_00867d74)) != 0)
            ThreadDispose(rec->id);
        while ((rec = (ThreadRec *)QueueRemove(&DAT_00867d64)) != 0)
            ThreadDispose(rec->id);
        while ((thread = (ThreadRec *)QueueRemove(&gThreadFreeQ)) != 0)
            _ThreadRecordFree(thread);
        while ((dying = gThreadDying) != 0)
            _ThreadRecordClose(dying);
        QueueDispose(&gThreadFreeQ);
        QueueDispose(&DAT_00867d74);
        QueueDispose(&DAT_00867d64);
        QueueDispose(&DAT_00867d54);
        gThreadCount = 0;
        if (gThreadList != 0) {
            MemoryDeallocate(gThreadList);
            gThreadList = 0;
        }
        _ThreadTableUnlock();
        AtomicMaskClear(&gThreadFlags, kClosingBit);
    }
}

/* ThreadDispose. STATE: MATCH 100%, exe 102 / obj 102, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504287
long ThreadDispose(ULong id)
{
    ThreadRec *rec;
    MutexNode *node;
    long err;

    rec = _ThreadRecordLookup(id);
    err = 0;
    if (rec == 0) {
        err = kNotFound;
        goto done;
    }
    if (AtomicMaskTest(&gThreadFlags, kOpenBit) == 0)
        AtomicMaskTest(&gThreadFlags, kClosingBit);
    _ThreadTableLock();
    if (rec->thread != 0) {
        if (_PlatformSuspendThread(rec->thread) == kWaitFailed) {
            err = GetLastError();
            _PlatformResumeThread(rec->thread);
            _ThreadTableUnlock();
            err = kOSFailure;
            goto done;
        }
        if (rec->cleanup != 0)
            rec->cleanup(rec, rec->cleanupArg);
        if (TerminateThread(rec->thread, 0) == 0)
            err = GetLastError();
        if (CloseHandle(rec->thread) == 0)
            err = GetLastError();
        err = 0;
        rec->thread = 0;
    }
    _ThreadTableUnlock();
    gThreadList[rec->index].generation++;
    rec->id = 0;
    node = 0;
    node = QueueExtract(&DAT_00867d74, (MutexNode *)rec);
    if (node == 0)
        node = QueueExtract(&DAT_00867d64, (MutexNode *)rec);
    if (node == 0)
        node = QueueExtract(&DAT_00867d54, (MutexNode *)rec);
    if (node != 0)
        QueueInsert(&gThreadFreeQ, node);
done:
    return err;
}

/* _ThreadRecordClose. STATE: MATCH 100%, exe 57 / obj 57, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005043a1
long _ThreadRecordClose(void *thread)
{
    ThreadRec *rec;
    long err;

    rec = (ThreadRec *)thread;
    _ThreadRecordCurrent();
    err = 0;
    if (AtomicMaskTest(&gThreadFlags, kOpenBit) == 0)
        AtomicMaskTest(&gThreadFlags, kClosingBit);
    if (_ThreadIsAlive(thread) == 0) {
        err = kBadArgument;
    } else {
        _ThreadTableLock();
        if (rec->waitHandle != 0) {
            if (CloseHandle(rec->waitHandle) == 0)
                err = GetLastError();
            err = 0;
            rec->waitHandle = 0;
            _ThreadListMove(thread, &gThreadDying, 0);
        }
    }
    _ThreadTableUnlock();
    if (err == 0)
        MemoryClear(rec, 16);
    return err;
}

/* _ThreadIsAlive. STATE: MATCH 100%, exe 29 / obj 29, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504432
unsigned char _ThreadIsAlive(void *thread)
{
    ThreadRec *rec;
    long err;
    unsigned char alive;

    rec = (ThreadRec *)thread;
    _ThreadRecordCurrent();
    err = 0;
    alive = 0;
    switch (WaitForSingleObject(rec->waitHandle, 0)) {
    case kWaitObject0:
    case kWaitAbandoned:
    case kWaitTimeout:
        alive = 1;
        break;
    case kWaitFailed:
        break;
    default:
        err = GetLastError();
        err = kOSFailure;
        break;
    }
    return alive;
}

/* _ThreadListMove. STATE: MATCH 100%, exe 37 / obj 37, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504481
void _ThreadListMove(void *thread, ThreadRec **head, ThreadRec **pushTo)
{
    long err;
    ThreadRec *rec;
    ThreadRec **link;
    ThreadRec *p;

    rec = (ThreadRec *)thread;
    err = 0;
    link = 0;
    if (head != 0) {
        link = head;
        while ((p = *link) != 0 && p != thread)
            link = (ThreadRec **)&p->link;
        if (p != 0) {
            *link = rec->link;
            rec->link = 0;
        }
    }
    if (pushTo != 0) {
        rec->link = *pushTo;
        *pushTo = thread;
    }
}

ULong gCriticalIndex = kNoTlsIndex;                /* 0x00578744 thread TLS index */

typedef struct Mutex {
    ULong magic;
    void *section;
    unsigned char live;
    unsigned char pad[7];
} Mutex;

#define kMutexMagic       0xdead1ce5UL
#define kCriticalSection  0x18                   /* sizeof(CRITICAL_SECTION) */

/* CriticalCreate. STATE: MATCH 100%, exe 45 / obj 45, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005044ce
long CriticalCreate(void *lock)
{
    Mutex *m;
    long err;

    m = lock;
    err = 0;
    MemoryClear(m, sizeof(Mutex));
    m->magic = kMutexMagic;
    if (gCriticalIndex == kNoTlsIndex) {
        gCriticalIndex = TlsAlloc();
        if (gCriticalIndex == kNoTlsIndex) {
            err = GetLastError();
            err = kOSFailure;
            goto failed;
        }
    }
    if ((m->section = MemoryAllocateClear(kCriticalSection)) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    } else {
        InitializeCriticalSection(m->section);
        m->live = 1;
    }
failed:
    if (err != 0)
        CriticalDispose(lock);
    return err;
}

/* CriticalDispose. STATE: MATCH 100%, exe 29 / obj 29, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050454f
long CriticalDispose(void *lock)
{
    Mutex *m;
    long err;

    m = lock;
    err = 0;
    if (m->live != 0 && m->section != 0) {
        DeleteCriticalSection(m->section);
        m->live = 0;
    }
    if (m->section != 0) {
        MemoryDeallocate(m->section);
        m->section = 0;
    }
    MemoryClear(m, sizeof(Mutex));
    return 0;
}

/* CriticalEnter. STATE: MATCH 100%, exe 17 / obj 17, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504592
void CriticalEnter(void *lock)
{
    Mutex *m;
    long err;

    m = lock;
    err = 0;
    EnterCriticalSection(m->section);
    if (TlsSetValue(gCriticalIndex,
                    (void *)((long)TlsGetValue(gCriticalIndex) + 1)) == 0)
        err = GetLastError();
}

/* CriticalLeave. STATE: MATCH 100%, exe 22 / obj 22, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005045cc
void CriticalLeave(void *lock)
{
    Mutex *m;
    long held;
    long err;

    m = lock;
    err = 0;
    TlsGetValue(gCriticalIndex);
    held = (long)TlsGetValue(gCriticalIndex);
    if (held != 0) {
        if (TlsSetValue(gCriticalIndex, (void *)(held - 1)) == 0)
            err = GetLastError();
    }
    LeaveCriticalSection(m->section);
}

/* _ThreadCheckCritical. STATE: MATCH 100%, exe 14 / obj 14, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504616
unsigned char _ThreadCheckCritical(void)
{
    long err;
    unsigned char result;

    err = 0;
    result = 0;
    if (gCriticalIndex != kNoTlsIndex)
        result = TlsGetValue(gCriticalIndex) ? 1 : 0;
    return result;
}

/* _ThreadRecordCreate. STATE: MATCH 100%, exe 89 / obj 89, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050463d
long _ThreadRecordCreate(ULong stackSize)
{
    long err;
    ThreadRec *rec;
    ThreadSlot *slots;
    ThreadSlot *old;
    char token[16];

    err = 0;
    rec = 0;
    slots = 0;
    old = 0;
    AtomicEnterCritical(token);
    rec = MemoryAllocateClear(sizeof(ThreadRec));
    if (rec == 0)
        goto fail;
    rec->index = gThreadCount;
    slots = MemoryAllocate((gThreadCount + 1) * sizeof(ThreadSlot));
    if (slots == 0) {
fail:
        err = kOutOfMemory;
    } else {
        MemoryCopy(slots, gThreadList, gThreadCount * sizeof(ThreadSlot));
        MemoryClear(&slots[gThreadCount], sizeof(ThreadSlot));
        slots[gThreadCount].rec = rec;
        old = gThreadList;
        gThreadList = slots;
        gThreadCount = gThreadCount + 1;
        if (old != 0)
            MemoryDeallocate(old);
        rec->id = _ThreadRecordGetId(rec);
        QueueInsert(&gThreadFreeQ, (MutexNode *)rec);
    }
    AtomicLeaveCritical(token);
    if (err != 0) {
        if (slots != 0)
            MemoryDeallocate(slots);
        if (rec != 0)
            _ThreadRecordFree(rec);
    }
    return err;
}

/* _ThreadRecordFree. STATE: MATCH 100%, exe 17 / obj 17, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504723
void _ThreadRecordFree(ThreadRec *rec)
{
    long err;

    err = 0;
    MemoryClear(&gThreadList[rec->index], sizeof(ThreadSlot));
    MemoryDeallocate(rec);
}

/* _ThreadRecordLookup. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050474c
ThreadRec *_ThreadRecordLookup(long id)
{
    long err;
    ULong key;
    ULong index;
    ULong generation;
    ThreadRec *rec;

    err = 0;
    key = id;
    index = (key & 0xffff0000) >> 16;
    generation = key & 0xffff;
    rec = 0;
    if ((unsigned short)generation == gThreadList[(unsigned short)index].generation)
        rec = gThreadList[(unsigned short)index].rec;
    return rec;
}

/* _ThreadRecordGetId. STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00504781
ULong _ThreadRecordGetId(ThreadRec *rec)
{
    long err;
    ULong index;
    ULong id;

    err = 0;
    index = 0;
    index = rec->index;
    id = (index << 16) | gThreadList[index].generation;
    index = id;
    return id;
}

/* _ThreadRecordCurrent. STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005047a6
void *_ThreadRecordCurrent(void)
{
    long err;
    void *result;

    err = 0;
    result = 0;
    result = TlsGetValue(gThreadIndex);
    return result;
}

/* _ThreadTableLock. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x005047bb
void _ThreadTableLock(void)
{
    AtomicEnterCritical(gThreadLock);
}

/* _ThreadTableUnlock. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x005047c7
void _ThreadTableUnlock(void)
{
    AtomicLeaveCritical(gThreadLock);
}
