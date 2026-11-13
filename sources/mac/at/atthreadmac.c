/* atthreadmac.c -- 1.0.10: threads and critical sections on Mac, one object: threads are cooperative, records on four queues, a Time Manager task and a deferred task pre-empt them when the system has no threads of its own; a critical section is Open Transport's notifier lock when it has a provider, the atomic section when it has none. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#if TARGET_API_MAC_CARBON
#include <Multiprocessing.h>
#include <OpenTransport.h>
#endif

#if TARGET_API_MAC_CARBON
#define kThreadRecBytes     0x144
#else
#define kThreadRecBytes     0x13c
#endif
#define kMinStack           0x4000
#define kThreadsOpen        0
#define kThreadsReady       1
#define kThreadsInit        2
#define kThreadsBusy        28
#define kThreadsHeld        29
#define kThreadsLocked      30
#define kThreadsVM          31
#define kNoThread           -7005
#define kMutexOwnedElse     -7002
#define kPeriodNormal       10
#define kPeriodBusy         3
#define kTableMax           20
#define kYieldsPerCheck     3
#define kSliceMicros        10000
#define kHeadroomMicros     10000
#define kScanMax            10
#define dtQType             7
#define kThreadSliceGlobal  0x110
#define kMutexMagic         0xdead1ce5
#define kNoIndex            0xffffffff

typedef unsigned char (*EnterNotifierProc)(void *provider);
typedef void (*LeaveNotifierProc)(void *provider);

typedef struct WaitRec {
    ULong (*proc)(ULong id, void *arg, unsigned short code, void *ref);
    void *ref;
    unsigned short code;
    unsigned short pad;
    ULong result;
    ULongLong deadline;
} WaitRec;

typedef struct ThreadRec {
    MutexNode link;
    int32_t unk8[3];
    void (*proc)(struct ThreadRec *t, void *arg);
    void *arg;
    WaitRec *wait;
    ULong id;
    unsigned short index;
    unsigned char running;
    unsigned char dead;
    int32_t unk28;
    struct ThreadMutex *owned;
    ULong stackBytes;
    Ptr stack;
    char context[kThreadRecBytes - 0x38];
} ThreadRec;

typedef struct ThreadMutex {
    ULong magic;
    struct ThreadMutex *next;
    ULong depth;
    ULong owner;
} ThreadMutex;

typedef struct ThreadSlot {
    ThreadRec *thread;
    unsigned short serial;
    unsigned short pad;
} ThreadSlot;

typedef struct ThreadTimer {
    TMTask task;
    short pad;
    int32_t ref;
} ThreadTimer;

extern LeaveNotifierProc gLeaveNotifier;
extern EnterNotifierProc gEnterNotifier;
static ThreadTimer gThreadTimer;
static DeferredTask gThreadDeferred;
static MutexList gThreadsFree;
static MutexList gThreadsReady;
static MutexList gThreadsWaiting;
static MutexList gThreadsPreempted;
ThreadRec gMainThread;
static int32_t gCriticalToken[4];
static ThreadRec gIdleThread;
ULong gThreadFlags;
static ULongLong gThreadSliceStart;
static ULongLong gThreadSliceNow;
ThreadRec *gThreadCurrent;
extern unsigned char gPlatformThreaded;

static ThreadMutex *sAllMutexes;
static ULong sThreadCount;
static ThreadSlot *sThreadSlots;
static ULong sThreadsPreempted;
static ThreadRec *sThreadIdleNow;

void ThreadDeferredInstall(void);
void _ThreadDeferredProc(void);
static void _ThreadSwitch(ThreadRec *from, ThreadRec *to, MutexList *list);
void ThreadsPostflight(void);
extern int32_t AtomicCompareSwap(void *word, int32_t oldValue, int32_t newValue);
void ThreadYield(void);
int32_t _ThreadRecordClose(void *ref);
unsigned char _ThreadMutexEnter(ThreadMutex *m);
int32_t _ThreadMutexLeave(ThreadMutex *m);
extern int32_t ThreadDispose(ULong id);
extern void _AtomicEnter2(void);
extern void _AtomicLeave2(void);

static int32_t _ThreadRecordCreate(ULong stackSize);
static void _ThreadRecordFree(ThreadRec *rec);
static void _ThreadListMove(ThreadMutex *m, ThreadMutex **from, ThreadMutex **to);
static ThreadRec *_ThreadChoose(void);
static unsigned char _ThreadWake(ThreadRec *t, ULongLong now);
static ThreadRec *_ThreadRecordCurrent(void);
static ThreadRec *_ThreadRecordLookup(ULong id);
static ULong _ThreadRecordGetId(ThreadRec *rec);
static void _ThreadTableLock(void);
static void _ThreadTableUnlock(void);
static ThreadRec *_ThreadSetCurrent(ThreadRec *t);

#if TARGET_API_MAC_CARBON

#define kNoMemory           -7004
#define kNoDispose          -7002
#define kBadOwner           -29298
#define kForever            0x7fffffff

typedef struct TaskMutex {
    MPTaskID owner;
    ULong depth;
    MPSemaphoreID semaphore;
} TaskMutex;

typedef struct MacMutex {
    ULong magic;
    TaskMutex *mutex;
    void *provider;
    short depth;
    unsigned char entered;
    unsigned char pad;
} MacMutex;

extern unsigned char _PlatformIsThreaded(void);

static TaskStorageIndex gCriticalIndex = kNoIndex;

int32_t _MutexCreate(TaskMutex **out)
{
    TaskMutex *m;
    MPSemaphoreID semaphore;
    OSStatus e;

    if (out == 0)
        return paramErr;
    *out = 0;
    m = MPAllocateAligned(sizeof(TaskMutex), kMPAllocateDefaultAligned, kMPAllocateClearMask);
    if (m == 0)
        return memFullErr;
    e = MPCreateSemaphore(1, 1, &semaphore);
    if (e == 0) {
        *out = m;
        (*out)->owner = 0;
        (*out)->depth = 0;
        (*out)->semaphore = semaphore;
    } else {
        MPFree(m);
    }
    return e;
}

int32_t _MutexDispose(TaskMutex *m)
{
    OSStatus e;

    e = 0;
    if (m == 0)
        return paramErr;
    if (m->owner != 0 && m->depth != 0)
        return kBadOwner;
    if (m->semaphore != 0)
        e = MPDeleteSemaphore(m->semaphore);
    if (e != 0)
        return e;
    m->semaphore = 0;
    MPFree(m);
    return e;
}

int32_t _MutexEnter(TaskMutex *m, Duration timeout)
{
    MPTaskID self;
    OSStatus e;

    self = MPCurrentTaskID();
    if (m == 0)
        return paramErr;
    if (self == m->owner) {
        m->depth++;
        return 0;
    }
    e = MPWaitOnSemaphore(m->semaphore, timeout);
    if (e != 0)
        return e;
    m->owner = self;
    m->depth = 1;
    return e;
}

int32_t _MutexLeave(TaskMutex *m)
{
    MPTaskID self;
    OSStatus e;

    self = MPCurrentTaskID();
    e = 0;
    if (self != m->owner)
        return kBadOwner;
    if (m->depth == 0)
        return kBadOwner;
    m->depth--;
    if (m->depth == 0) {
        m->owner = 0;
        e = MPSignalSemaphore(m->semaphore);
    }
    return e;
}

int32_t _MutexDepth(TaskMutex *m, ULong *depth)
{
    MPTaskID self;

    self = MPCurrentTaskID();
    if (m == 0)
        return paramErr;
    if (depth == 0)
        return paramErr;
    if (self == m->owner)
        *depth = m->depth;
    else
        *depth = 0;
    return 0;
}

void ThreadYield(void)
{
}

ULong ThreadsCount(void)
{
    ULong n;

    AtomicMaskTest(&gThreadFlags, kThreadsReady) || AtomicMaskTest(&gThreadFlags, kThreadsInit);
    n = QueueGetCount(&gThreadsReady) + QueueGetCount(&gThreadsWaiting) + QueueGetCount(&gThreadsPreempted) + 1;
    return n;
}

int32_t CriticalCreateFromProvider(void *lock, void *provider)
{
    MacMutex *m;
    int32_t err;

    m = lock;
    err = 0;
    MemoryClear(m, sizeof(MacMutex));
    if (gCriticalIndex == kNoIndex) {
        if (_PlatformIsThreaded()) {
            if (MPAllocateTaskStorageIndex(&gCriticalIndex) != 0) {
                err = kNoMemory;
                goto done;
            }
        } else {
            gCriticalIndex = 0;
        }
    }
    m->magic = kMutexMagic;
    m->provider = provider;
done:
    if (err != 0)
        CriticalDispose(lock);
    return err;
}

int32_t CriticalCreate(void *lock)
{
    MacMutex *m;
    int32_t err;

    m = lock;
    err = 0;
    MemoryClear(m, sizeof(MacMutex));
    m->magic = kMutexMagic;
    if (gCriticalIndex == kNoIndex) {
        if (_PlatformIsThreaded()) {
            if (MPAllocateTaskStorageIndex(&gCriticalIndex) != 0) {
                err = kNoMemory;
                goto done;
            }
        } else {
            gCriticalIndex = 0;
        }
    }
    if (_PlatformIsThreaded() && _MutexCreate(&m->mutex) != 0)
        err = kNoMemory;
done:
    if (err != 0)
        CriticalDispose(lock);
    return err;
}

int32_t CriticalDispose(void *lock)
{
    MacMutex *m;
    int32_t err;

    m = lock;
    err = 0;
    if (m->mutex != 0) {
        if (_MutexDispose(m->mutex) != 0)
            err = kNoDispose;
        else
            m->mutex = 0;
    }
    if (err == 0)
        MemoryClear(m, sizeof(MacMutex));
    return err;
}

void CriticalEnter(void *lock)
{
    MacMutex *m;

    m = lock;
    _PlatformIsThreaded();
    if (m->provider != 0) {
        if (OTEnterNotifier(m->provider))
            m->entered = 1;
        m->depth++;
    } else if (_PlatformIsThreaded()) {
        _MutexEnter(m->mutex, kForever);
    } else {
        AtomicEnterCritical(gCriticalToken);
    }
    if (_PlatformIsThreaded())
        MPSetTaskStorageValue(gCriticalIndex, MPGetTaskStorageValue(gCriticalIndex) + 1);
}

void CriticalLeave(void *lock)
{
    MacMutex *m;
    TaskStorageValue depth;

    m = lock;
    _PlatformIsThreaded();
    if (_PlatformIsThreaded())
        MPGetTaskStorageValue(gCriticalIndex);
    if (_PlatformIsThreaded() && (depth = MPGetTaskStorageValue(gCriticalIndex)) != 0)
        MPSetTaskStorageValue(gCriticalIndex, depth - 1);
    if (m->provider != 0) {
        m->depth--;
        if (m->depth == 0 && m->entered) {
            m->entered = 0;
            OTLeaveNotifier(m->provider);
        }
    } else if (_PlatformIsThreaded()) {
        _MutexLeave(m->mutex);
    } else {
        AtomicLeaveCritical(gCriticalToken);
    }
}

unsigned char _ThreadCheckCritical(void)
{
    unsigned char held;

    held = 0;
    if (gCriticalIndex != kNoIndex) {
        if (_PlatformIsThreaded())
            held = MPGetTaskStorageValue(gCriticalIndex) != 0;
        else
            held = _AtomicCheckCritical();
    }
    return held;
}

#else

static RoutineDescriptor sThreadTimerDesc = BUILD_ROUTINE_DESCRIPTOR(uppTimerProcInfo, ThreadDeferredInstall);
static TimerUPP sThreadTimer = &sThreadTimerDesc;
static RoutineDescriptor sThreadDeferredDesc = BUILD_ROUTINE_DESCRIPTOR(uppDeferredTaskProcInfo, _ThreadDeferredProc);
static DeferredTaskUPP sThreadDeferred = &sThreadDeferredDesc;

int32_t ThreadsPreflight(int32_t count, ULong stackSize)
{
    int32_t err, e;

    err = 0;
    AtomicMaskTest(&gThreadFlags, kThreadsInit);
    if (stackSize < kMinStack)
        stackSize = kMinStack;
    if (!AtomicMaskApply(&gThreadFlags, kThreadsOpen)) {
        int32_t response;

        MemoryClear(&gMainThread, kThreadRecBytes);
        gMainThread.id = 1;
        MemoryClear(&gIdleThread, kThreadRecBytes);
        gIdleThread.id = 1;
        gThreadCurrent = &gMainThread;
        sThreadIdleNow = 0;
        if (Gestalt(gestaltVMAttr, &response) == 0 && (response & 1) != 0)
            AtomicMaskApply(&gThreadFlags, kThreadsVM);
    }
    if (!AtomicMaskTest(&gThreadFlags, kThreadsReady)) {
        e = QueueCreate(&gThreadsFree);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&gThreadsReady);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&gThreadsWaiting);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&gThreadsPreempted);
        if (e != 0) {
            err = e;
            goto done;
        }
        if ((sThreadSlots = MemoryAllocateClear(sizeof(ThreadSlot))) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        sThreadSlots[0].thread = &gMainThread;
        sThreadSlots[0].serial = 1;
        sThreadCount = 1;
        if (!gPlatformThreaded) {
            MemoryClear(&gThreadTimer, sizeof(gThreadTimer));
            gThreadTimer.task.tmAddr = sThreadTimer;
            gThreadTimer.ref = 0;
            MemoryClear(&gThreadDeferred, sizeof(gThreadDeferred));
            gThreadDeferred.qType = dtQType;
            gThreadDeferred.dtAddr = sThreadDeferred;
            gThreadDeferred.dtParam = 0;
            InsTime((QElemPtr)&gThreadTimer);
        }
        SystemSetQuitProc(ThreadsPostflight);
        AtomicMaskApply(&gThreadFlags, kThreadsReady);
    }
    while ((ULong)count-- != 0) {
        e = _ThreadRecordCreate(stackSize);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
done:
    return err;
}

void ThreadsPostflight(void)
{
    ThreadRec *t;

    if (AtomicMaskClear(&gThreadFlags, kThreadsReady) && !AtomicMaskApply(&gThreadFlags, kThreadsInit)) {
        _ThreadTableLock();
        if (!gPlatformThreaded) {
            RmvTime((QElemPtr)&gThreadTimer);
            AtomicMaskClear(&gThreadFlags, kThreadsBusy);
            MemoryClear(&gThreadTimer, sizeof(gThreadTimer));
            MemoryClear(&gThreadDeferred, sizeof(gThreadDeferred));
        }
        while ((t = (ThreadRec *)QueueRemove(&gThreadsReady)) != 0)
            ThreadDispose(t->id);
        while ((t = (ThreadRec *)QueueRemove(&gThreadsWaiting)) != 0)
            ThreadDispose(t->id);
        while ((t = (ThreadRec *)QueueRemove(&gThreadsFree)) != 0)
            _ThreadRecordFree(t);
        while (sAllMutexes != 0)
            _ThreadRecordClose(sAllMutexes);
        QueueDispose(&gThreadsFree);
        QueueDispose(&gThreadsReady);
        QueueDispose(&gThreadsWaiting);
        QueueDispose(&gThreadsPreempted);
        sThreadCount = 0;
        if (sThreadSlots != 0) {
            MemoryDeallocate(sThreadSlots);
            sThreadSlots = 0;
        }
        _ThreadTableUnlock();
        AtomicMaskClear(&gThreadFlags, kThreadsInit);
    }
}

int32_t ThreadDispose(ULong id)
{
    ThreadRec *t;
    int32_t err;
    unsigned char held1;
    unsigned char held2;
    ThreadRec *old;
    MutexNode *node;

    t = _ThreadRecordLookup(id);
    err = 0;
    if (t == 0) {
        err = kNoThread;
        goto done;
    }
    AtomicMaskTest(&gThreadFlags, kThreadsReady) || AtomicMaskTest(&gThreadFlags, kThreadsInit);
    if (!t->dead) {
        held1 = AtomicMaskApply(&gThreadFlags, kThreadsLocked);
        held2 = AtomicMaskApply(&gThreadFlags, kThreadsHeld);
        t->running = 1;
        old = _ThreadSetCurrent(t);
        if (t->proc != 0)
            t->proc(t, t->arg);
        if (t->owned != 0)
            err = 0;
        while (t->owned != 0)
            _ThreadMutexLeave(t->owned);
        _ThreadSetCurrent(old);
        t->running = 0;
        t->dead = 1;
        if (!held2)
            AtomicMaskClear(&gThreadFlags, kThreadsHeld);
        if (!held1)
            AtomicMaskClear(&gThreadFlags, kThreadsLocked);
    }
    sThreadSlots[t->index].serial++;
    t->id = 0;
    if (t == gThreadCurrent || t == sThreadIdleNow) {
        ThreadYield();
    } else {
        node = QueueExtract(&gThreadsReady, &t->link);
        if (node == 0) {
            _ThreadTableLock();
            if (node == 0)
                node = QueueExtract(&gThreadsWaiting, &t->link);
            if (node == 0)
                node = QueueExtract(&gThreadsPreempted, &t->link);
            _ThreadTableUnlock();
        }
        if (node != 0)
            QueueInsert(&gThreadsFree, node);
    }
done:
    return err;
}

void ThreadYield(void)
{
    ThreadRec *self;
    ThreadRec *next;
    MutexList *list;

    AtomicMaskTest(&gThreadFlags, kThreadsReady) || AtomicMaskTest(&gThreadFlags, kThreadsInit);
    self = _ThreadRecordCurrent();
    next = _ThreadChoose();
    list = 0;
    if (next != self) {
        if (self->dead) {
            list = &gThreadsFree;
        } else if (sThreadIdleNow == 0 && self != &gMainThread) {
            list = &gThreadsReady;
        } else if (sThreadIdleNow != 0 && self != &gIdleThread) {
            list = &gThreadsWaiting;
        }
        {
            ThreadRec *idle = sThreadIdleNow;

            if (idle != 0)
                sThreadIdleNow = next;
            if (idle == 0)
                gThreadCurrent = next;
        }
        _ThreadSwitch(self, next, list);
    }
}

ULong ThreadsCount(void)
{
    ULong n;

    AtomicMaskTest(&gThreadFlags, kThreadsReady) || AtomicMaskTest(&gThreadFlags, kThreadsInit);
    n = QueueGetCount(&gThreadsReady) + QueueGetCount(&gThreadsWaiting) + QueueGetCount(&gThreadsPreempted) + 1;
    return n;
}

int32_t _ThreadRecordClose(void *ref)
{
    ThreadRec *t;
    ThreadMutex *m;
    int32_t err;

    m = ref;
    t = _ThreadRecordCurrent();
    err = 0;
    AtomicMaskTest(&gThreadFlags, kThreadsReady) || AtomicMaskTest(&gThreadFlags, kThreadsInit);
    if (!_ThreadMutexEnter(m)) {
        err = kMutexOwnedElse;
    } else {
        _ThreadTableLock();
        _ThreadListMove(m, &t->owned, 0);
    }
    _ThreadTableUnlock();
    if (err == 0)
        MemoryClear(m, sizeof(*m));
    return err;
}

unsigned char _ThreadMutexEnter(ThreadMutex *m)
{
    ThreadRec *t;
    unsigned char got;

    t = _ThreadRecordCurrent();
    got = 0;
    if (m->depth == 0 && AtomicCompareSwap(&m->depth, 0, 1) == 0) {
        _ThreadTableLock();
        _ThreadListMove(m, &sAllMutexes, &t->owned);
        m->owner = t->id;
        _ThreadTableUnlock();
        got = 1;
    } else if (m->owner == t->id) {
        AtomicIncrement((int32_t *)&m->depth, 1);
        got = 1;
    }
    return got;
}

int32_t _ThreadMutexLeave(ThreadMutex *m)
{
    ThreadRec *t;
    int32_t err;

    t = _ThreadRecordCurrent();
    err = 0;
    if (m->depth == 0) {
        err = kMutexOwnedElse;
    } else if (m->owner != t->id) {
        err = kMutexOwnedElse;
    } else if (m->depth != 0 && AtomicIncrement((int32_t *)&m->depth, -1) == 1) {
        _ThreadTableLock();
        m->owner = 0;
        _ThreadListMove(m, &t->owned, &sAllMutexes);
        _ThreadTableUnlock();
    }
    return err;
}

static void _ThreadListMove(ThreadMutex *m, ThreadMutex **from, ThreadMutex **to)
{
    ThreadMutex *p;

    if (from != 0) {
        while ((p = *from) != 0 && p != m)
            from = &p->next;
        if (p != 0) {
            *from = m->next;
            m->next = 0;
        }
    }
    if (to != 0) {
        m->next = *to;
        *to = m;
    }
}

typedef struct MacMutex {
    ULong magic;
    void *provider;
    short depth;
    unsigned char entered;
    unsigned char pad;
    ULong unkC;
} MacMutex;

extern void *_PlatformFindSymbol(const char *library, const char *name);

static ULong gCriticalIndex = kNoIndex;

int32_t CriticalCreateFromProvider(void *lock, void *provider)
{
    MacMutex *m;
    int32_t err;

    m = lock;
    err = 0;
    MemoryClear(m, sizeof(MacMutex));
    if (gEnterNotifier == 0 && (gEnterNotifier = _PlatformFindSymbol("OTClientLib", "OTEnterNotifier")) == 0) {
        err = -7003;
        goto done;
    }
    if (gLeaveNotifier == 0 && (gLeaveNotifier = _PlatformFindSymbol("OTClientLib", "OTLeaveNotifier")) == 0) {
        err = -7003;
        goto done;
    }
    if (gCriticalIndex == kNoIndex)
        gCriticalIndex = 0;
    m->magic = kMutexMagic;
    m->provider = provider;
done:
    if (err != 0)
        CriticalDispose(lock);
    return err;
}

int32_t CriticalCreate(void *lock)
{
    MacMutex *m;

    m = lock;
    MemoryClear(m, sizeof(MacMutex));
    m->magic = kMutexMagic;
    if (gCriticalIndex == kNoIndex)
        gCriticalIndex = 0;
    return 0;
}

int32_t CriticalDispose(void *lock)
{
    MemoryClear(lock, sizeof(MacMutex));
    return 0;
}

void CriticalEnter(void *lock)
{
    MacMutex *m;

    m = lock;
    if (m->provider != 0) {
        if (gEnterNotifier(m->provider))
            m->entered = 1;
        m->depth++;
    } else {
        AtomicEnterCritical(gCriticalToken);
    }
}

void CriticalLeave(void *lock)
{
    MacMutex *m;

    m = lock;
    if (m->provider != 0) {
        m->depth--;
        if (m->depth == 0 && m->entered) {
            m->entered = 0;
            gLeaveNotifier(m->provider);
        }
    } else {
        AtomicLeaveCritical(gCriticalToken);
    }
}

unsigned char _ThreadCheckCritical(void)
{
    unsigned char held;

    held = 0;
    if (gCriticalIndex != kNoIndex)
        held = _AtomicCheckCritical();
    return held;
}

static int32_t _ThreadRecordCreate(ULong stackSize)
{
    int32_t err;
    ThreadRec *t;
    ThreadSlot *slots;

    err = 0;
    slots = 0;
    t = MemoryAllocateClear(kThreadRecBytes);
    if (t == 0) {
        err = kOutOfMemory;
        goto done;
    }
    t->index = sThreadCount;
    if (!gPlatformThreaded) {
        if ((t->stack = MemoryAllocate(stackSize)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        t->stackBytes = stackSize;
        if (AtomicMaskTest(&gThreadFlags, kThreadsVM))
            HoldMemory(t->stack, stackSize);
    } else {
        t->stackBytes = stackSize;
    }
    slots = MemoryAllocate((sThreadCount + 1) * sizeof(ThreadSlot));
    if (slots == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryCopy(slots, sThreadSlots, sThreadCount * sizeof(ThreadSlot));
    MemoryClear(&slots[sThreadCount], sizeof(ThreadSlot));
    slots[sThreadCount].thread = t;
    {
        ThreadSlot *old;

        old = sThreadSlots;
        sThreadSlots = slots;
        sThreadCount++;
        if (old != 0)
            MemoryDeallocate(old);
    }
    t->id = _ThreadRecordGetId(t);
    QueueInsert(&gThreadsFree, &t->link);
done:
    if (err != 0) {
        if (slots != 0)
            MemoryDeallocate(slots);
        if (t != 0)
            _ThreadRecordFree(t);
    }
    return err;
}

static void _ThreadRecordFree(ThreadRec *rec)
{
    if (rec->stack != 0) {
        if (AtomicMaskTest(&gThreadFlags, kThreadsVM))
            UnholdMemory(rec->stack, rec->stackBytes);
        MemoryDeallocate(rec->stack);
    }
    MemoryClear(&sThreadSlots[rec->index], sizeof(ThreadSlot));
    MemoryDeallocate(rec);
}

static ThreadRec *_ThreadRecordLookup(ULong id)
{
    ThreadRec *t;
    ULong index;

    index = id >> 16;
    t = 0;
    if ((id & 0xffff) == sThreadSlots[index].serial)
        t = sThreadSlots[index].thread;
    return t;
}

static ULong _ThreadRecordGetId(ThreadRec *rec)
{
    ULong index;

    index = rec->index;
    return (index << 16) | sThreadSlots[index].serial;
}

static ThreadRec *_ThreadRecordCurrent(void)
{
    if (sThreadIdleNow != 0)
        return sThreadIdleNow;
    return gThreadCurrent;
}

static void _ThreadTableLock(void)
{
    AtomicMaskApply(&gThreadFlags, kThreadsBusy + 2);
}

static void _ThreadTableUnlock(void)
{
    AtomicMaskClear(&gThreadFlags, kThreadsBusy + 2);
}

static ThreadRec *_ThreadChoose(void)
{
    ThreadRec *next;
    ULongLong now;

    next = 0;
    now = TimerGetMicroseconds();
    if (sThreadIdleNow != 0) {
        ULong used;

        used = now - gThreadSliceStart;
        if ((int32_t)(now - gThreadSliceNow) >= kHeadroomMicros)
            next = &gIdleThread;
        if (next == 0 && sThreadIdleNow != &gIdleThread && !sThreadIdleNow->dead && used * sThreadsPreempted < kSliceMicros)
            next = _ThreadWake(sThreadIdleNow, now) ? sThreadIdleNow : 0;
        if (next == 0) {
            while ((next = (ThreadRec *)QueueRemove(&gThreadsPreempted)) != 0) {
                if (_ThreadWake(next, now))
                    break;
                QueueInsert(&gThreadsWaiting, &next->link);
            }
        }
        if (next == 0)
            next = &gIdleThread;
        if (next != sThreadIdleNow)
            gThreadSliceStart = now;
    } else if (gThreadCurrent == &gMainThread) {
        ULong n;
        ULong i;
        ULong count;

        count = QueueGetCount(&gThreadsReady);
        i = 0;
        if (count < kScanMax)
            n = count;
        else
            n = kScanMax;
        for (; i < n && (next = (ThreadRec *)QueueRemove(&gThreadsReady)) != 0; i++) {
            if (_ThreadWake(next, now))
                break;
            QueueInsert(&gThreadsReady, &next->link);
            next = 0;
        }
        if (next == 0) {
            next = &gMainThread;
            _ThreadWake(&gMainThread, now);
        }
    } else {
        next = &gMainThread;
        _ThreadWake(&gMainThread, now);
    }
    return next;
}

static unsigned char _ThreadWake(ThreadRec *t, ULongLong now)
{
    WaitRec *w;
    unsigned char run;
    ThreadRec *old;

    w = t->wait;
    run = 0;
    if (t->unk28 == 0 && w == 0)
        run = 1;
    if (w != 0 && w->proc != 0) {
        old = _ThreadSetCurrent(t);
        if ((w->result = w->proc(t->id, t->arg, w->code, w->ref)) != 0)
            run = 1;
        _ThreadSetCurrent(old);
    }
    if (w != 0 && now >= w->deadline) {
        w->deadline = 0;
        run = 1;
    }
    return run;
}

void ThreadDeferredInstall(void)
{
    DTInstall(&gThreadDeferred);
}

void _ThreadDeferredProc(void)
{
    int32_t saved;
    ULongLong now;

    QueueTransfer(&gThreadsPreempted, &gThreadsWaiting);
    if (!AtomicMaskTest(&gThreadFlags, kThreadsLocked) && !QueueIsEmpty(&gThreadsPreempted)) {
        _AtomicEnter2();
        saved = *(int32_t *)kThreadSliceGlobal;
        *(int32_t *)kThreadSliceGlobal = 0;
        sThreadsPreempted = QueueGetCount(&gThreadsPreempted);
        if (sThreadsPreempted > kTableMax)
            sThreadsPreempted = kTableMax;
        now = TimerGetMicroseconds();
        gThreadSliceStart = now;
        gThreadSliceNow = now;
        sThreadIdleNow = &gIdleThread;
        ThreadYield();
        sThreadIdleNow = 0;
        sThreadsPreempted = 0;
        gThreadSliceNow = 0;
        gThreadSliceStart = 0;
        *(int32_t *)kThreadSliceGlobal = saved;
        _AtomicLeave2();
    }
    PrimeTime((QElemPtr)&gThreadTimer, AtomicMaskTest(&gThreadFlags, kThreadsLocked) ? kPeriodBusy : kPeriodNormal);
}

static ThreadRec *_ThreadSetCurrent(ThreadRec *t)
{
    ThreadRec *old;

    if (sThreadIdleNow != 0) {
        old = sThreadIdleNow;
        sThreadIdleNow = t;
    } else {
        old = gThreadCurrent;
        gThreadCurrent = t;
    }
    return old;
}

/* Hand assembly, Classic: _ThreadSwitch is an entry inside this frame; it runs on its caller's r30 and r31, keeps r11 up with stmw and returns through the frame's epilogue. None of that is C. */
extern void _savef14(void);
extern void _restf14(void);

asm void _ThreadSwitchFrame(ThreadRec *from, ThreadRec *to, MutexList *list)
{
    nofralloc
    mflr    r0
    mr      r11, r1
    bl      _savef14
    stw     r31, -0x94(r1)
    stw     r30, -0x98(r1)
    stw     r29, -0x9c(r1)
    stw     r0, 8(r1)
    stwu    r1, -0xe0(r1)
    mr      r31, r1
    mr      r31, r3
    mr      r30, r4
    mr      r29, r5
    entry static _ThreadSwitch
    mflr    r0
    stw     r0, 8(r1)
    mfcr    r0
    stw     r0, 4(r1)
    lwz     r0, 0x110(r0)
    stw     r0, 0x40(r31)
    li      r0, 0
    stw     r0, 0x110(r0)
    lwz     r0, 0x908(r0)
    stw     r0, 0x44(r31)
    lwz     r0, 0x114(r0)
    stw     r0, 0x48(r31)
    lwz     r0, 0x130(r0)
    stw     r0, 0x4c(r31)
    lwz     r0, 0xbae(r0)
    stw     r0, 0x50(r31)
    stw     r1, 0x54(r31)
    stw     r2, 0x58(r31)
    stmw    r11, 0x5c(r31)
    stfd    f14, 0xac(r31)
    stfd    f15, 0xb4(r31)
    stfd    f16, 0xbc(r31)
    stfd    f17, 0xc4(r31)
    stfd    f18, 0xcc(r31)
    stfd    f19, 0xd4(r31)
    stfd    f20, 0xdc(r31)
    stfd    f21, 0xe4(r31)
    stfd    f22, 0xec(r31)
    stfd    f23, 0xf4(r31)
    stfd    f24, 0xfc(r31)
    stfd    f25, 0x104(r31)
    stfd    f26, 0x10c(r31)
    stfd    f27, 0x114(r31)
    stfd    f28, 0x11c(r31)
    stfd    f29, 0x124(r31)
    stfd    f30, 0x12c(r31)
    stfd    f31, 0x134(r31)
    lfd     f14, 0xac(r30)
    lfd     f15, 0xb4(r30)
    lfd     f16, 0xbc(r30)
    lfd     f17, 0xc4(r30)
    lfd     f18, 0xcc(r30)
    lfd     f19, 0xd4(r30)
    lfd     f20, 0xdc(r30)
    lfd     f21, 0xe4(r30)
    lfd     f22, 0xec(r30)
    lfd     f23, 0xf4(r30)
    lfd     f24, 0xfc(r30)
    lfd     f25, 0x104(r30)
    lfd     f26, 0x10c(r30)
    lfd     f27, 0x114(r30)
    lfd     f28, 0x11c(r30)
    lfd     f29, 0x124(r30)
    lfd     f30, 0x12c(r30)
    lfd     f31, 0x134(r30)
    lmw     r11, 0x5c(r30)
    lwz     r2, 0x58(r30)
    lwz     r1, 0x54(r30)
    lwz     r0, 0x50(r30)
    stw     r0, 0xbae(r0)
    lwz     r0, 0x4c(r30)
    stw     r0, 0x130(r0)
    lwz     r0, 0x48(r30)
    stw     r0, 0x114(r0)
    lwz     r0, 0x44(r30)
    stw     r0, 0x908(r0)
    lwz     r0, 0x40(r30)
    stw     r0, 0x110(r0)
    cmpwi   r5, 0
    beq     L3b0
    stwu    r1, -0x60(r1)
    mr      r4, r3
    mr      r3, r5
    bl      QueueInsert
    nop     
    addi    r1, r1, 0x60
L3b0:
    mr      r3, r11
    mr      r4, r12
    lwz     r0, 4(r1)
    mtcrf   0xff, r0
    lwz     r0, 8(r1)
    mtlr    r0
    lwz     r0, 0xe8(r1)
    lwz     r1, 0(r1)
    mr      r11, r1
    bl      _restf14
    mtlr    r0
    lwz     r31, -0x94(r1)
    lwz     r30, -0x98(r1)
    lwz     r29, -0x9c(r1)
    blr
}

#endif
