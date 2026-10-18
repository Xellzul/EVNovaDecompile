// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atmutexlist.c -- the runtime block's mutex-guarded singly linked list, the one the thread table keeps its records on. Notes: Research/notes/atmutexlist.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#include "burgerplat.h"

/* QueueCreate. STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b99b
long QueueCreate(void *ref)
{
    MutexList *list;
    long err;
    long e;

    list = (MutexList *)ref;
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

    list = (MutexList *)ref;
    node = (MutexNode *)element;
    err = 0;
    node->next = 0;
    node->owner = (MutexList *)ref;
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

    list = (MutexList *)ref;
    found = (MutexNode *)element;
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
        return (MutexNode *)element;
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

    dst = (MutexList *)dstRef;
    src = (MutexList *)srcRef;
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

    list = (MutexList *)ref;
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

    list = (MutexList *)ref;
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
