// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atnetsock.c -- the Winsock layer's socket events, packet pools and the three socket lists (wake all, close all, link, unlink). Notes: Research/notes/atnet.md */

#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"

#define kNetProcBits    ((NetProc)-1)

NetSocket *gResolverList;                        /* 0x008707c8 */
NetSocket *gDatagramList;                        /* 0x008705dc */
NetSocket *gStreamList;                          /* 0x008700c0 */

/* NetworkMakeCallback (tell a socket's owner about an event: a callback, or a bit set in the word
   `ref' when the callback is kNetProcBits). STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00528db4
void NetworkMakeCallback(NetProc proc, void *ref, NetSocket *s, unsigned short event)
{
    long err;

    err = 0;
    if (proc == kNetProcBits)
        AtomicMaskApply((ULong *)ref, event);
    else if (proc != 0)
        proc(s, ref, event);
}

#define kPacketStream   1
#define kPacketDatagram 2
#define kPacketBuffer   3
#define kStreamMinimum  0x218
#define kNodeHeader     0x14
#define kDatagramHeader 0x120
#define kBufferHeader   0x1c

typedef struct NetNode {
    MutexNode link;
    long unk8[3];
    ULong size;
} NetNode;

#define CLEAR_NODE(n) (memset((n), 0, 4 * sizeof(long)), (n)->unk8[2] = 0)

/* NetworkQueueFill (put count new nodes of one kind on a free list). STATE: MATCH 100%, exe
   100 / obj 100, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00528ded
long NetworkQueueFill(MutexList *list, unsigned short kind, ULong count, ULong size)
{
    ULong bytes;
    ULong type;
    long err;
    NetNode *node;

    err = 0;
    bytes = size;
    node = 0;
    switch (type = kind) {
    case kPacketStream:
        if (size < kStreamMinimum)
            size = kStreamMinimum;
        while (count--) {
            node = MemoryAllocate(size);
            if (node == 0) {
                err = kOutOfMemory;
                goto done;
            }
            CLEAR_NODE(node);
            QueueInsert(list, &node->link);
        }
        break;
    case kPacketDatagram:
        while (count--) {
            node = MemoryAllocate(size + kDatagramHeader);
            if (node == 0) {
                err = kOutOfMemory;
                goto done;
            }
            CLEAR_NODE(node);
            ((NetPacket *)node)->capacity = size;
            QueueInsert(list, &node->link);
        }
        break;
    case kPacketBuffer:
        while (count--) {
            node = MemoryAllocate(size + kBufferHeader);
            if (node == 0) {
                err = kOutOfMemory;
                goto done;
            }
            CLEAR_NODE(node);
            node->size = size;
            QueueInsert(list, &node->link);
        }
        break;
    }
done:
    return err;
}

/* NetworkQueueEmpty (free every packet on a list). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00528ed4
void NetworkQueueEmpty(MutexList *list)
{
    long err;

    err = 0;
    QueueEmpty(list);
}

/* NetworkQueuePoll (take the first packet off a list). STATE: MATCH 100%, exe 8 / obj 8, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00528ee3
MutexNode *NetworkQueuePoll(MutexList *list)
{
    long err;
    MutexNode *result;

    err = 0;
    result = 0;
    result = QueueRemove(list);
    return result;
}

#define kWaitSliceMs    20
#define kMsPerSecond    1000

/* NetworkQueueWait (take a node off a list, waiting up to `seconds' in 20 ms alertable sleeps
   that keep the platform idle running). STATE: MATCH 100%, exe 72 / obj 72, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00528ef6
MutexNode *NetworkQueueWait(MutexList *list, ULong seconds)
{
    long err;
    MutexNode *node;
    ULongLong now;
    ULongLong deadline;

    err = 0;
    node = 0;
    node = QueueRemove(list);
    if (node == 0) {
        now = TimerGetMilliseconds();
        deadline = now + seconds * kMsPerSecond;
        while (now <= deadline) {
            SleepEx(deadline - now < kWaitSliceMs ? deadline - now : kWaitSliceMs, 1);
            SystemIdle();
            node = QueueRemove(list);
            if (node != 0)
                break;
            now = TimerGetMilliseconds();
        }
    }
    return node;
}

/* _EndpointTickleAll (wake every socket of the three lists; one may leave its list meanwhile).
   STATE: MATCH 100%, exe 49 / obj 49, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00528fd5
void _EndpointTickleAll(void)
{
    NetSocket **link;
    NetSocket *s;
    long err;

    err = 0;
    link = 0;
    s = 0;
    link = &gResolverList;
    while ((s = *link) != 0) {
        ResolverTickle(*link);
        if (s == *link)
            link = &(*link)->next;
    }
    link = &gDatagramList;
    while ((s = *link) != 0) {
        DatagramTickle(*link);
        if (s == *link)
            link = &(*link)->next;
    }
    link = &gStreamList;
    while ((s = *link) != 0) {
        StreamTickle((NetStream *)*link);
        if (s == *link)
            link = &(*link)->next;
    }
}

/* _EndpointDisposeAll (close and dispose of every socket of the three lists). STATE: MATCH 100%, exe 41 / obj 41, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052903e
void _EndpointDisposeAll(void)
{
    NetSocket *s;
    long err;

    err = 0;
    s = 0;
    while ((s = gResolverList) != 0) {
        ResolverClose((struct NetResolver *)s);
        ResolverDispose((struct NetResolver *)s);
    }
    while ((s = gDatagramList) != 0) {
        DatagramClose(s, 0);
        DatagramDispose(s);
    }
    while ((s = gStreamList) != 0) {
        StreamClose(s, 0);
        StreamDispose(s);
    }
}

/* _EndpointRegister (link a socket at the head of its kind's list). STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052909d
void _EndpointRegister(NetSocket *s, unsigned short kind)
{
    char token[16];
    NetSocket **head;
    long err;

    err = 0;
    head = 0;
    switch (kind) {
    case kListResolver:
        head = &gResolverList;
        break;
    case kListDatagram:
        head = &gDatagramList;
        break;
    case kListStream:
        head = &gStreamList;
        break;
    }
    AtomicEnterCritical(token);
    s->next = *head;
    *head = s;
    AtomicLeaveCritical(token);
}

/* _EndpointUnregister (unlink a socket from its kind's list). STATE: MATCH 100%, exe 45 / obj 45, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005290f5
void _EndpointUnregister(NetSocket *s, unsigned short kind)
{
    char token[16];
    NetSocket **link;
    long err;

    err = 0;
    link = 0;
    switch (kind) {
    case kListResolver:
        link = &gResolverList;
        break;
    case kListDatagram:
        link = &gDatagramList;
        break;
    case kListStream:
        link = &gStreamList;
        break;
    }
    AtomicEnterCritical(token);
    while (*link != 0 && *link != s)
        link = &(*link)->next;
    if (*link != 0) {
        *link = s->next;
        s->next = 0;
    }
    AtomicLeaveCritical(token);
}
