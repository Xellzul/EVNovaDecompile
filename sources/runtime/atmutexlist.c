// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atmutexlist.c -- the runtime block's mutex-guarded singly linked list, the one the thread table keeps its records on. Notes: Research/notes/atmutexlist.md */

#include "burgertypes.h"
#include "burger.h"
#ifndef MAC1010
#include "win32.h"
#endif

#include "burgerplat.h"

#ifdef MAC1010
/* the Mac list has no lock: the changes are lwarx/stwcx. loops, the count a shadow of how often the head moved */
extern void _AtomicEnter(void);
extern void _AtomicLeave(void);

static MutexNode *_QueueDequeue(MutexList *list);
static MutexNode *_QueueDequeueItem(MutexList *list, MutexNode *node);
static void _QueueEnqueue(MutexList *list, MutexNode *node);
static void _QueueSteal(MutexList *dst, MutexList *src);
static ULong _QueueCount(MutexList *list);

long QueueCreate(void *ref)
{
    SystemCheckCallback();
    MemoryClear(ref, sizeof(MutexList));
    return 0;
}

void QueueDispose(MutexList *list)
{
    SystemCheckCallback();
    MemoryClear(list, sizeof(MutexList));
}

void QueueInsert(void *ref, void *element)
{
    MutexNode *node;

    node = element;
    node->next = 0;
    node->owner = ref;
    _AtomicEnter();
    _QueueEnqueue(ref, node);
    _AtomicLeave();
}

MutexNode *QueueRemove(MutexList *list)
{
    MutexNode *node;

    _AtomicEnter();
    node = _QueueDequeue(list);
    _AtomicLeave();
    if (node != 0)
        node->owner = 0;
    return node;
}

MutexNode *QueueExtract(void *ref, void *element)
{
    MutexNode *found;

    found = element;
    _AtomicEnter();
    found = _QueueDequeueItem(ref, found);
    _AtomicLeave();
    if (found != 0)
        found->owner = 0;
    if (found != 0)
        return element;
    return 0;
}

void QueueTransfer(void *dstRef, void *srcRef)
{
    _AtomicEnter();
    _QueueSteal(dstRef, srcRef);
    _AtomicLeave();
}

ULong QueueGetCount(void *ref)
{
    ULong result;

    _AtomicEnter();
    result = _QueueCount(ref);
    _AtomicLeave();
    return result;
}

unsigned char QueueIsEmpty(void *ref)
{
    MutexList *list;
    unsigned char result;

    list = ref;
    result = true;
    if (list->head != 0)
        result = false;
    return result;
}

void QueueEmpty(MutexList *list)
{
    MutexNode *node;

    SystemCheckCallback();
    while ((node = QueueRemove(list)) != 0)
        MemoryDeallocate(node);
}

static asm MutexNode *_QueueDequeue(MutexList *list)
{
    nofralloc
    li      r0, 0
    lwz     r4, 0(r3)
    cmpwi   r4, 0
    beq     @done
    li      r9, 4
@bump:
    lwarx   r7, r9, r3
    addi    r7, r7, 1
    stwcx.  r7, r9, r3
    bne     @bump
    sync
@retry:
    lwarx   r4, 0, r3
    cmpwi   r4, 0
    beq     @done
    lwz     r9, 0(r4)
    stwcx.  r9, 0, r3
    bne     @retry
    sync
    stw     r0, 0(r4)
@done:
    mr      r3, r4
    blr
}

static asm MutexNode *_QueueDequeueItem(MutexList *list, MutexNode *node)
{
    nofralloc
    li      r0, 0
    lwz     r6, 0(r3)
    cmpwi   r6, 0
    beq     @done
@restart:
    lwz     r7, 4(r3)
    mr      r6, r3
@next:
    mr      r5, r6
@retry:
    lwarx   r6, 0, r5
    lwz     r8, 4(r3)
    cmpw    r7, r8
    bne     @restart
    cmpwi   r6, 0
    beq     @done
    cmpw    r6, r4
    bne     @next
    lwz     r9, 0(r6)
    stwcx.  r9, 0, r5
    bne     @retry
    sync
@done:
    mr      r3, r6
    blr
}

static asm void _QueueEnqueue(MutexList *list, MutexNode *node)
{
    nofralloc
    li      r0, 0
    stw     r0, 0(r4)
@restart:
    lwz     r7, 4(r3)
    mr      r6, r3
@next:
    mr      r5, r6
@retry:
    lwarx   r6, 0, r5
    lwz     r8, 4(r3)
    cmpw    r7, r8
    bne     @restart
    cmpwi   r6, 0
    bne     @next
    stwcx.  r4, 0, r5
    bne     @retry
    sync
    blr
}

static asm void _QueueSteal(MutexList *dst, MutexList *src)
{
    nofralloc
    li      r0, 0
    lwz     r5, 0(r4)
    cmpwi   r5, 0
    beqlr
    li      r9, 4
@bump:
    lwarx   r7, r9, r4
    addi    r7, r7, 1
    stwcx.  r7, r9, r4
    bne     @bump
    sync
@take:
    lwarx   r5, 0, r4
    cmpwi   r5, 0
    beqlr
    stwcx.  r0, 0, r4
    bne     @take
    sync
    mr      r4, r5
@restart:
    lwz     r7, 4(r3)
    mr      r6, r3
@next:
    mr      r5, r6
@retry:
    lwarx   r6, 0, r5
    lwz     r8, 4(r3)
    cmpw    r7, r8
    bne     @restart
    cmpwi   r6, 0
    bne     @next
    stwcx.  r4, 0, r5
    bne     @retry
    sync
    blr
}

static asm ULong _QueueCount(MutexList *list)
{
    nofralloc
    li      r0, 0
@restart:
    lwz     r7, 4(r3)
    mr      r6, r3
    li      r4, -1
@next:
    mr      r5, r6
    addi    r4, r4, 1
    lwz     r6, 0(r5)
    lwz     r8, 4(r3)
    cmpw    r7, r8
    bne     @restart
    cmpwi   r6, 0
    bne     @next
    mr      r3, r4
    blr
}
#else
/* QueueCreate. STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b99b
long QueueCreate(void *ref)
{
    MutexList *list;
    long err;
    long e;

    list = ref;
    err = 0;
    e = 0;
    SystemCheckCallback();
    MemoryClear(list, sizeof(MutexList));
    list->mutex = CreateMutexA(0, 0, 0);
    if (list->mutex == 0)
        err = kOutOfMemory;
    if (err != 0)
        QueueDispose(ref);
    return err;
}

/* QueueDispose. STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b9e4
void QueueDispose(MutexList *list)
{
    long err;
    long e;
    void *mutex;
    void *h;

    err = 0;
    e = 0;
    SystemCheckCallback();
    if (list->mutex != 0)
        WaitForSingleObject(list->mutex, INFINITE);
    h = list->mutex;
    if (h != 0) {
        mutex = h;
        list->mutex = 0;
        ReleaseMutex(h);
        CloseHandle(h);
    }
    MemoryClear(list, sizeof(MutexList));
}

/* QueueInsert. STATE: MATCH 100%, exe 26 / obj 26, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ba30
void QueueInsert(void *ref, void *element)
{
    MutexList *list;
    MutexNode *node;
    long err;

    list = ref;
    node = element;
    err = 0;
    node->next = 0;
    node->owner = ref;
    WaitForSingleObject(list->mutex, INFINITE);
    if (list->tail != 0)
        list->tail->next = node;
    else
        list->head = node;
    list->tail = node;
    list->count++;
    ReleaseMutex(list->mutex);
}

/* QueueRemove. STATE: MATCH 100%, exe 33 / obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ba71
MutexNode *QueueRemove(MutexList *list)
{
    MutexNode *node;
    long err;

    node = 0;
    err = 0;
    if (list->head != 0) {
        WaitForSingleObject(list->mutex, INFINITE);
        node = list->head;
        if (node != 0) {
            list->head = node->next;
            if (list->head == 0)
                list->tail = 0;
            node->next = 0;
            list->count--;
        }
        ReleaseMutex(list->mutex);
    }
    if (node != 0)
        node->owner = 0;
    return node;
}

/* QueueExtract. STATE: MATCH 100%, exe 64 / obj 64, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052babf
MutexNode *QueueExtract(void *ref, void *element)
{
    MutexList *list;
    MutexNode **link;
    MutexNode *p;
    MutexNode *found;
    long err;

    list = ref;
    found = element;
    err = 0;
    link = 0;
    if (list->head != 0) {
        WaitForSingleObject(list->mutex, INFINITE);
        link = &list->head;
        while ((p = *link) != 0) {
            if (p == found)
                break;
            link = &p->next;
        }
        if (p != 0) {
            *link = found->next;
            if (list->head == 0) {
                list->tail = 0;
            } else if (*link == 0) {
                for (list->tail = list->head; list->tail->next != 0; list->tail = list->tail->next)
                    ;
            }
            found->next = 0;
            list->count--;
        } else {
            found = 0;
        }
        ReleaseMutex(list->mutex);
    } else {
        found = 0;
    }
    if (found != 0)
        found->owner = 0;
    if (found != 0)
        return element;
    return 0;
}

/* QueueTransfer (move every node of one list onto another). STATE: MATCH 100%, exe 33 /
   obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052bb4e
void QueueTransfer(void *dstRef, void *srcRef)
{
    char token[16];
    MutexList *dst;
    MutexList *src;
    MutexNode *node;
    long err;

    dst = dstRef;
    src = srcRef;
    err = 0;
    if (src->head != 0) {
        AtomicEnterCritical(token);
        while ((node = QueueRemove(srcRef)) != 0)
            QueueInsert(dstRef, node);
        AtomicLeaveCritical(token);
    }
}

/* QueueGetCount (a list's node count). STATE: MATCH 100%, exe 9 / obj 9, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052bb97
ULong QueueGetCount(void *ref)
{
    MutexList *list;
    long err;
    ULong result;

    list = ref;
    err = 0;
    result = 0;
    result = list->count;
    return result;
}

/* QueueIsEmpty (a list is empty). STATE: MATCH 100%, exe 9 / obj 9, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052bbab
unsigned char QueueIsEmpty(void *ref)
{
    MutexList *list;
    long err;
    unsigned char result;

    list = ref;
    err = 0;
    result = true;
    if (list->head != 0)
        result = false;
    return result;
}

/* QueueEmpty (free every node of a list). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052bbbf
void QueueEmpty(MutexList *list)
{
    MutexNode *node;
    long err;

    err = 0;
    node = 0;
    SystemCheckCallback();
    while ((node = QueueRemove(list)) != 0)
        MemoryDeallocate(node);
}
#endif
