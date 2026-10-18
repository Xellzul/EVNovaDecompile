// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atnet.c -- the runtime block's threaded Winsock layer under the REGGIE client: framed TCP streams, datagram and stream sockets, their threads, close and dispose. Notes: Research/notes/atnet.md */

#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "win32.h"

#define kStateClosing   7

#define kEventReceived      12
#define kStreamConnected    4

void BufferedStreamCallback(NetSocket *s, void *ref, unsigned short event);
void BufferedGatherData(NetSocket *s, NetFramed *w, NetChunk **cur, MutexList *queue, MutexList *out, unsigned char flush);
ULong _BufferedFrameLength(NetFramed *w, NetChunk *chunk, unsigned char second);
long _BufferedFindLine(const char *buf, ULong n);
long _BufferedFindCRLF(const char *buf, ULong n);
long _BufferedFindNUL(const char *buf, ULong n);
long _DatagramConnResetControl(void *ref, unsigned char enable);
ULong __stdcall _DatagramRecvThread(void *param);
ULong __stdcall _DatagramSendThread(void *param);
long _StreamConnected(NetSocket *sock);
long _StreamHasPending(NetStream *s);
void _StreamPeerClosed(NetStream *s, unsigned short event);
long _StreamReceive(void *ref);
ULong __stdcall _StreamRecvThread(void *param);
ULong __stdcall _StreamSendThread(void *param);

/* A TCP stream cut into frames (NetFramed, 0x48 bytes): the stream's event proc reassembles
   received chunks into whole frames on the caller's two output lists. */

#define kFramedSize         0x48
#define kFrameLine          -1
#define kFrameCrLf          -2
#define kFrameNul           -3
#define kFrameChunk         -4
#define kFrameLength        -5
#define kLengthBytes        4
#define kEventReceived2     13
#define kFlagNoBuffers      0
#define kFlagPeerClosed     1
#define kFlagFailed         2
#define kWrongKind          -7001
#define kNodeBytes          16

#define FNOTIFY(w, s, event) NetworkMakeCallback((w)->proc, (w)->ref, (s), (event))

/* StreamCreateBuffered (a framed TCP socket: the frame state, then the stream with the frame proc).
   STATE: MATCH 100%, exe 100 / obj 100, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052961c
NetStream *StreamCreateBuffered(void *unused, void *ref, NetProc proc, long frameMode,
                        MutexList *sentList, MutexList *freeList, MutexList *outList,
                        MutexList *outList2)
{
    long err, e;
    NetFramed *w;
    NetStream *s;

    err = 0;
    w = 0;
    s = 0;
    w = MemoryAllocate(kFramedSize);
    if (w == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(w, 0, kFramedSize);
    w->ref = ref;
    w->frameMode = frameMode;
    w->proc = proc;
    w->freeList = freeList;
    w->outList = outList;
    w->outList2 = outList2;
    memset(&w->queue, 0, sizeof(MutexList));
    memset(&w->queue2, 0, sizeof(MutexList));
    e = QueueCreate(&w->queue);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = QueueCreate(&w->queue2);
    if (e != 0) {
        err = e;
        goto done;
    }
    s = StreamCreate(unused, w, BufferedStreamCallback, sentList, freeList, &w->queue, &w->queue2);
    if (s == 0)
        err = kOutOfMemory;
done:
    if (err != 0) {
        if (w != 0) {
            QueueDispose(&w->queue);
            QueueDispose(&w->queue2);
            MemoryDeallocate(w);
        }
        if (s != 0)
            StreamDispose((NetSocket *)s);
        s = 0;
    }
    return s;
}

/* BufferedStreamStatus (a framed socket still waiting for receive buffers reports itself connected).
   STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052970b
unsigned short BufferedStreamStatus(NetSocket *s, unsigned short state)
{
    NetProc proc;
    void *ref;
    long err;
    NetFramed *w;

    err = 0;
    proc = 0;
    ref = 0;
    w = 0;
    EndpointGetCallback(s, &proc, &ref);
    if (proc == BufferedStreamCallback && (w = (NetFramed *)ref) != 0 &&
        AtomicMaskTest(&w->flags, kFlagNoBuffers))
        state = kStreamConnected;
    return state;
}

/* BufferedStreamCallback (the framed stream's event proc: frames what arrives, holds back peer-closed
   and failed until the receive buffers are back, frees itself on dispose). STATE: MATCH 100%,
   exe 323 / obj 323, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529768
void BufferedStreamCallback(NetSocket *s, void *ref, unsigned short event)
{
    NetFramed *w = ref;
    long err;
    char token[16];
    NetProc proc;
    void *owner;
    unsigned char flush;

    err = 0;
    switch (event) {
    case kEventClosed:
        if (w->partial != 0) {
            QueueInsert(w->freeList, &w->partial->link);
            w->partial = 0;
        }
        if (w->partial2 != 0) {
            QueueInsert(w->freeList, &w->partial2->link);
            w->partial2 = 0;
        }
        QueueTransfer(w->freeList, &w->queue);
        QueueTransfer(w->freeList, &w->queue2);
        w->flags = 0;
        FNOTIFY(w, s, event);
        goto end;
    case kEventConnected:
        FNOTIFY(w, s, event);
        if (QueueIsEmpty(w->freeList) == 0 && AtomicMaskClear(&w->flags, kFlagNoBuffers) != 0) {
            AtomicEnterCritical(token);
            flush = AtomicMaskTest(&w->flags, kFlagPeerClosed) != 0 ||
                    AtomicMaskTest(&w->flags, kFlagFailed) != 0;
            BufferedGatherData(s, w, &w->partial, &w->queue, w->outList, flush);
            BufferedGatherData(s, w, &w->partial2, &w->queue2, w->outList2, flush);
            AtomicLeaveCritical(token);
            if (AtomicMaskTest(&w->flags, kFlagNoBuffers) == 0) {
                if (AtomicMaskClear(&w->flags, kFlagPeerClosed) != 0)
                    FNOTIFY(w, s, kEventPeerClosed);
                if (AtomicMaskClear(&w->flags, kFlagFailed) != 0)
                    FNOTIFY(w, s, kEventFailed);
            }
        }
        goto end;
    case kEventDisposed:
        QueueDispose(&w->queue);
        QueueDispose(&w->queue2);
        proc = w->proc;
        owner = w->ref;
        MemoryDeallocate(w);
        NetworkMakeCallback(proc, owner, s, event);
        goto end;
    case kEventReceived:
        BufferedGatherData(s, w, &w->partial, &w->queue, w->outList, 0);
        goto end;
    case kEventReceived2:
        BufferedGatherData(s, w, &w->partial2, &w->queue2, w->outList2, 0);
        goto end;
    case kEventNeedBuffers:
        FNOTIFY(w, s, event);
        if (QueueIsEmpty(w->freeList) != 0)
            AtomicMaskApply(&w->flags, kFlagNoBuffers);
        goto end;
    case kEventPeerClosed:
    case kEventFailed:
        if (AtomicMaskTest(&w->flags, kFlagNoBuffers) == 0) {
            AtomicEnterCritical(token);
            BufferedGatherData(s, w, &w->partial, &w->queue, w->outList, 1);
            BufferedGatherData(s, w, &w->partial2, &w->queue2, w->outList2, 1);
            AtomicLeaveCritical(token);
        }
        if (AtomicMaskTest(&w->flags, kFlagNoBuffers) == 0) {
            FNOTIFY(w, s, event);
            goto end;
        }
        if (event == kEventPeerClosed)
            AtomicMaskApply(&w->flags, kFlagPeerClosed);
        else if (event == kEventFailed)
            AtomicMaskApply(&w->flags, kFlagFailed);
        goto end;
    case kEventAborted:
        AtomicEnterCritical(token);
        BufferedGatherData(s, w, &w->partial, &w->queue, w->outList, 1);
        BufferedGatherData(s, w, &w->partial2, &w->queue2, w->outList2, 1);
        AtomicLeaveCritical(token);
        FNOTIFY(w, s, event);
        goto end;
    }
    FNOTIFY(w, s, event);
end:
    return;
}

/* MemoryCopy's body (atplatheap.c), which the network layer inlines */
static inline void MoveBytes(void *dst, const void *src, ULong n)
{
    if (n != 0)
        memmove(dst, src, n);
}

/* BufferedGatherData (cut the received chunks into frames: a whole frame goes to `out', a short
   one borrows bytes from the next chunk, a long one is split into a free chunk; the flush
   ignores QueueIsEmpty's answer, as in the exe). STATE: MATCH 100%, exe 276 / obj 276,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529a96
void BufferedGatherData(NetSocket *s, NetFramed *w, NetChunk **cur, MutexList *queue,
                  MutexList *out, unsigned char flush)
{
    NetChunk *chunk;
    NetChunk *piece;
    ULong n;
    NetChunk *spare;
    ULong tail;
    ULong event;
    unsigned char second;
    ULong len;

    second = cur == &w->partial2 ? true : false;
    event = !second ? kEventReceived : kEventReceived2;
    len = 0;
    if (*cur == 0)
        *cur = (NetChunk *)NetworkQueuePoll(queue);
    spare = 0;
    while (*cur != 0) {
        n = _BufferedFrameLength(w, *cur, second);
        if (n > (*cur)->size)
            n = (*cur)->size;
        if (n == (*cur)->length) {
            QueueInsert(out, &(*cur)->link);
            FNOTIFY(w, s, event);
            if (spare != 0) {
                *cur = spare;
                spare = 0;
            } else {
                *cur = (NetChunk *)NetworkQueuePoll(queue);
            }
        } else if (n < (*cur)->length) {
            piece = (NetChunk *)NetworkQueuePoll(w->freeList);
            if (piece == 0 && AtomicMaskApply(&w->flags, kFlagNoBuffers) == 0) {
                FNOTIFY(w, s, kEventNeedBuffers);
                piece = (NetChunk *)NetworkQueuePoll(w->freeList);
            }
            if (piece != 0) {
                AtomicMaskClear(&w->flags, kFlagNoBuffers);
                piece->length = n;
                MoveBytes(piece->data, (*cur)->data, piece->length);
                QueueInsert(out, &piece->link);
                FNOTIFY(w, s, event);
                chunk = *cur;
                chunk->length -= n;
                MoveBytes((*cur)->data, (*cur)->data + n, chunk->length);
            } else if (spare != 0) {
                tail = (*cur)->length;
                tail -= n;
                MoveBytes(spare->data + tail, spare->data, spare->length);
                MoveBytes(spare->data, (*cur)->data + n, tail);
                (*cur)->length = n;
                spare->length += tail;
                QueueInsert(out, &(*cur)->link);
                FNOTIFY(w, s, event);
                *cur = spare;
                spare = 0;
            } else {
                break;
            }
        } else {
            if (spare == 0) {
                spare = (NetChunk *)NetworkQueuePoll(queue);
                if (spare == 0)
                    break;
            }
            n = (*cur)->size - (*cur)->length;
            if (n > spare->length)
                n = spare->length;
            MoveBytes((*cur)->data + (*cur)->length, spare->data, n);
            MoveBytes(spare->data, spare->data + n, spare->length - n);
            (*cur)->length += n;
            spare->length -= n;
            if (spare->length == 0) {
                QueueInsert(w->freeList, &spare->link);
                spare = 0;
            }
        }
    }
    if (flush != 0 && AtomicMaskTest(&w->flags, kFlagNoBuffers) == 0) {
        QueueIsEmpty(queue);
        if (*cur != 0) {
            QueueInsert(out, &(*cur)->link);
            *cur = 0;
            FNOTIFY(w, s, event);
        }
    }
}

/* _BufferedFrameLength (the length of the frame at the head of a chunk, at most the chunk's size;
   a length prefix is read in host order and counts itself). STATE: MATCH 100%, exe 78 /
   obj 78, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529d86
ULong _BufferedFrameLength(NetFramed *w, NetChunk *chunk, unsigned char second)
{
    long err;
    ULong n;
    long mode;

    err = 0;
    n = chunk->size;
    mode = w->frameMode;
    if (mode == kFrameLine) {
        n = _BufferedFindLine(chunk->data, chunk->length);
    } else if (mode == kFrameCrLf) {
        n = _BufferedFindCRLF(chunk->data, chunk->length);
    } else if (mode == kFrameNul) {
        n = _BufferedFindNUL(chunk->data, chunk->length);
    } else if (mode == kFrameChunk) {
        n = chunk->size;
    } else if (mode == kFrameLength) {
        if (chunk->length >= kLengthBytes) {
            memmove(&n, chunk->data, kLengthBytes);
            if (n < kLengthBytes)
                n = kLengthBytes;
        } else {
            n = kLengthBytes;
        }
    } else if (mode != 0) {
        n = ((FrameProc)mode)(w->unk0, w->ref, chunk, second);
    }
    if (n > chunk->size)
        n = chunk->size;
    return n;
}

/* _BufferedFindLine (the offset past the first line end: CR LF, CR or LF; -1 for none).
   STATE: MATCH 100%, exe 28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529e55
long _BufferedFindLine(const char *buf, ULong n)
{
    ULong i;

    for (i = 0; i < n; i++) {
        if (buf[i] == '\r') {
            if (i + 1 == n)
                return i + 1;
            if (buf[i + 1] == '\n')
                return i + 2;
            return i + 1;
        }
        if (buf[i] == '\n')
            return i + 1;
    }
    return -1;
}

/* _BufferedFindCRLF (the offset past the first CR LF; a CR ending the buffer counts; -1 for
   none). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529e92
long _BufferedFindCRLF(const char *buf, ULong n)
{
    ULong i;

    for (i = 0; i < n; i++) {
        if (buf[i] == '\r') {
            if (i + 1 == n)
                return i + 1;
            if (buf[i + 1] == '\n')
                return i + 2;
        }
    }
    return -1;
}

/* _BufferedFindNUL (a NUL-framed message: always -1, the scan's index is discarded as in the
   exe). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529ec5
long _BufferedFindNUL(const char *buf, ULong n)
{
    ULong i;

    for (i = 0; i < n && buf[i] != 0; i++)
        ;
    return -1;
}

#define kResolveSeconds     10
#define kNotResolved        -7108

/* SimpleResolve (resolve "host[:port]" to an address, waiting up to 10 s on a resolver of
   its own). STATE: MATCH 100%, exe 123 / obj 123, byte-identical yes. OPEN: the answer is
   read from the caller's own lookup record, not from the node that came back, as in the exe */
// FUNCTION: EVNOVA 0x00529ede
long SimpleResolve(void *unused, const char *host, NetAddress *out)
{
    NetLookup lookup;
    MutexList doneList;
    MutexList freeList;
    long err, e;
    NetResolver *r;

    err = 0;
    r = 0;
    AddressComposeEmpty(out);
    memset(&doneList, 0, sizeof(doneList));
    memset(&freeList, 0, sizeof(freeList));
    e = QueueCreate(&doneList);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = QueueCreate(&freeList);
    if (e != 0) {
        err = e;
        goto done;
    }
    r = ResolverCreate(unused, 0, 0, &doneList, &freeList);
    if (r == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = ResolverOpen(r);
    if (e != 0) {
        err = e;
        goto done;
    }
    memset(&lookup, 0, sizeof(lookup));
    CopyString(lookup.name, host);
    e = ResolverControl(r, &lookup);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (NetworkQueueWait(&doneList, kResolveSeconds) != 0)
        AddressDuplicate(out, &lookup.address);
    if (AddressIsEmpty(out))
        err = kNotResolved;
    ResolverClose(r);
done:
    if (r != 0)
        ResolverDispose(r);
    QueueDispose(&doneList);
    QueueDispose(&freeList);
    return err;
}

/* SimpleSend (send a handle's bytes: one datagram, cut to the packet's capacity, or as
   many stream chunks as it takes, all taken before any is sent). STATE: MATCH 100%, exe 234
   / obj 234, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a035
long SimpleSend(NetSocket *s, const NetAddress *to, void **h)
{
    MutexList *freeList;
    MutexNode *list;
    NetChunk *chunk;
    NetPacket *packet;
    NetChunk *node;
    NetProc proc;
    void *ref;
    ULong size;
    ULong total;
    ULong n;
    ULong len;
    long err;
    unsigned short kind;

    err = 0;
    list = 0;
    node = 0;
    packet = 0;
    chunk = 0;
    freeList = 0;
    kind = EndpointGetElem(s);
    size = IndirectGetSize(h);
    if ((long)kind != kListDatagram && (long)kind != kListStream) {
        err = kWrongKind;
        goto done;
    }
    if ((long)kind == kListDatagram) {
        long e;

        AddressToNetworkStack(to);
        EndpointGetStack(s);
        DatagramGetQueues(s, 0, &freeList, 0, 0);
        node = (NetChunk *)NetworkQueuePoll(freeList);
        if (node == 0) {
            EndpointGetCallback(s, &proc, &ref);
            NetworkMakeCallback(proc, ref, s, kEventNeedBuffers);
            node = (NetChunk *)NetworkQueuePoll(freeList);
            if (node == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        packet = (NetPacket *)node;
        if (size >= packet->capacity)
            size = packet->capacity;
        packet->length = size;
        MoveBytes(packet->data, *h, packet->length);
        AddressDuplicate(&packet->from, to);
        err = DatagramSend(s, packet);
        packet = 0;
        e = err;
        if (e != 0)
            err = e;
        goto done;
    }
    StreamGetQueues((NetStream *)s, 0, &freeList, 0, 0);
    for (total = 0; total < size; total += n) {
        node = (NetChunk *)NetworkQueuePoll(freeList);
        if (node == 0) {
            EndpointGetCallback(s, &proc, &ref);
            NetworkMakeCallback(proc, ref, s, kEventNeedBuffers);
            node = (NetChunk *)NetworkQueuePoll(freeList);
            if (node == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        n = node->size;
        node->link.next = list;
        list = &node->link;
    }
    for (total = 0; total < size; total += n) {
        char *from;
        ULong count;
        long e;

        chunk = node = (NetChunk *)list;
        list = list->next;
        memset(chunk, 0, kNodeBytes);
        len = size - total;
        if (len >= chunk->size)
            len = chunk->size;
        n = chunk->length = len;
        count = chunk->length;
        from = (char *)*h + total;
        if (count != 0)
            memmove(chunk->data, from, count);
        err = StreamSend(s, chunk);
        chunk = 0;
        e = err;
        if (e != 0)
            goto done;
    }
done:
    while ((chunk = node = (NetChunk *)list) != 0) {
        list = list->next;
        memset(chunk, 0, kNodeBytes);
        QueueInsert(freeList, &chunk->link);
    }
    /* the exe frees the list a second time; this loop always finds it empty */
    while ((chunk = node = (NetChunk *)list) != 0) {
        list = list->next;
        memset(chunk, 0, kNodeBytes);
        QueueInsert(freeList, &chunk->link);
    }
    return err;
}

#define kUdpSocket          0x268
#define kNotOpen            -7109
#define kReceiveFailed      -7107
#define kSocketFailed       -7102
#define kBindFailed         -7103
#define kOptionFailed       -7100
#define kEventSent          10
#define kEventReset         11

#define kIpprotoUdp         0x11
#define kSolSocket          0xffff
#define kSoBroadcast        0x20
#define kSoReuseAddr        4
#define kIpprotoIp          0
#define kIpTtl              7
#define kMaxTtl             0xff
#define kSioUdpConnReset    0x9800000c
#define kWin32sOr9x         0x80000000
#define kAddressBytes       0x100

#define kWsaEintr           10004
#define kWsaEinval          10022
#define kWsaEmsgsize        10040
#define kWsaEconnreset      10054
#define kWsaEnotsock        10038

#define kOptionReportReset  0x6000
#define kOptionNoAddress    0x6001
#define kOptionTtl          0x16000

typedef int (__stdcall *WSAIoctlProc)(ULong s, ULong code, void *in, ULong inSize,
                                      void *out, ULong outSize, ULong *returned,
                                      void *overlapped, void *completion);

static WSAIoctlProc gDatagramWSAIoctlProc;                /* 0x0086cb10 */

ULong __stdcall _DatagramRecvThread(void *param);
ULong __stdcall _DatagramSendThread(void *param);
long _DatagramConnResetControl(void *ref, unsigned char enable);

#define NOTIFY(s, event)    NetworkMakeCallback((s)->proc, (s)->ref, (s), (event))

/* DatagramCreate (a new UDP socket with its sender and receiver threads, both suspended; the
   socket is returned even when a step failed). STATE: MATCH 100%, exe 118 / obj 118,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a2ae
NetSocket *DatagramCreate(void *unused, void *ref, NetProc proc, MutexList *sentList,
                        MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    NetSocket *s;
    ULong threadId;
    long err, e;

    err = 0;
    s = 0;
    s = MemoryAllocate(kUdpSocket);
    if (s == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(s, 0, kUdpSocket);
    s->kind = kListDatagram;
    s->unk6 = 0;
    s->state = 0;
    s->proc = proc;
    s->ref = ref;
    s->sentList = sentList;
    s->freeList = freeList;
    s->recvList = recvList;
    s->recvList2 = recvList2;
    s->reportReset = 0;
    s->socket = kNoSocket;
    e = CriticalCreate(s->lock);
    if (e != 0) {
        err = e;
        goto done;
    }
    threadId = 0;
    e = QueueCreate(&s->queue);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((s->thread = CreateThread(0, kThreadStack, _DatagramSendThread, s, CREATE_SUSPENDED,
                                  &threadId)) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (SetThreadPriority(s->thread, THREAD_PRIORITY_HIGHEST) == 0)
        goto noPriority;
    if ((s->thread2 = CreateThread(0, kThreadStack, _DatagramRecvThread, s, CREATE_SUSPENDED,
                                   &threadId)) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (SetThreadPriority(s->thread2, THREAD_PRIORITY_HIGHEST) == 0) {
    noPriority:
        err = GetLastError();
        err = kThreadFailed;
        goto done;
    }
    _EndpointRegister(s, kListDatagram);
    NOTIFY(s, kEventCreated);
    NOTIFY(s, kEventClosed);
done:
    return s;
}

/* DatagramTickle (receive buffers were given back: wake the receiver if it waited). STATE:
   MATCH 100%, exe 31 / obj 31, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a421
void DatagramTickle(NetSocket *s)
{
    long state;
    long err;

    err = 0;
    state = s->state;
    if (state != 0)
        NOTIFY(s, kEventConnected);
    if ((s->waitRecv != 0 || s->waitSend != 0) && QueueIsEmpty(s->freeList) == 0) {
        s->waitRecv = 0;
        s->waitSend = 0;
        _PlatformResumeThread(s->thread2);
    }
}

/* DatagramDispose (dispose of a socket: close it, stop its threads, return its packets).
   STATE: MATCH 100%, exe 87 / obj 87, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a474
void DatagramDispose(void *ref)
{
    NetSocket *s;
    long err;

    s = (NetSocket *)ref;
    err = 0;
    DatagramClose(ref, 0);
    NetworkMakeCallback(s->proc, s->ref, s, kEventDisposed);
    s->quit = 1;
    while ((s->thread != 0 && s->done == 0) || (s->thread2 != 0 && s->done2 == 0)) {
        if (s->thread != 0 && s->done == 0)
            _PlatformResumeThread(s->thread);
        if (s->thread2 != 0 && s->done2 == 0)
            _PlatformResumeThread(s->thread2);
        SleepEx(kSleepMs, 1);
    }
    if (s->thread2 != 0 && CloseHandle(s->thread2) == 0)
        err = GetLastError();
    if (s->thread != 0 && CloseHandle(s->thread) == 0)
        err = GetLastError();
    QueueTransfer(s->freeList, &s->queue);
    QueueDispose(&s->queue);
    CriticalDispose(s->lock);
    _EndpointUnregister(s, kListDatagram);
    MemoryDeallocate(s);
}

/* DatagramControl (a UDP socket option: report resets, forget the last reset, the TTL).
   STATE: MATCH 100%, exe 86 / obj 86, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a570
long DatagramControl(void *ref, void *out, ULong option, ULong value)
{
    NetSocket *s;
    NetAddress *address;
    int old;
    int length;
    long err, e;

    s = (NetSocket *)ref;
    err = 0;
    switch (option) {
    case kOptionReportReset:
        s->reportReset = (value & 1) != 0 ? true : false;
        if (s->socket >= 0) {
            e = _DatagramConnResetControl(ref, s->reportReset);
            if (e != 0)
                err = e;
        }
        break;
    case kOptionNoAddress:
        address = (NetAddress *)out;
        AddressComposeEmpty(address);
        break;
    case kOptionTtl:
        length = sizeof(int);
        if (out != 0) {
            if (getsockopt(s->socket, kIpprotoIp, kIpTtl, &old, &length) < 0)
                goto failed;
            *(int *)out = old;
        }
        old = value < kMaxTtl ? value : kMaxTtl;
        if (setsockopt(s->socket, kIpprotoIp, kIpTtl, &old, sizeof(int)) >= 0)
            break;
    failed:
        err = _EndpointGetError(ref);
        err = kOptionFailed;
        break;
    default:
        err = kUnsupported;
        break;
    }
    return err;
}

/* DatagramOpen (open a UDP socket: create, SO_REUSEADDR, bind to an address or any, the
   reset option, the local address; then wake the receiver). STATE: MATCH 100%, exe 136 /
   obj 136, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a65d
long DatagramOpen(void *ref, NetAddress *address)
{
    NetSocket *s;
    NetAddress any;
    int length;
    int on;
    long err, e;

    s = (NetSocket *)ref;
    err = 0;
    if ((s->socket = socket(AF_INET, SOCK_DGRAM, kIpprotoUdp)) < 0) {
        err = _EndpointGetError(ref);
        err = kSocketFailed;
        goto done;
    }
    on = 1;
    if (setsockopt(s->socket, kSolSocket, kSoReuseAddr, &on, sizeof(on)) < 0)
        goto failed;
    if (address == 0) {
        AddressComposeTCPIP(address = &any, 0, 0);
    }
    if (bind(s->socket, &address->family, address->kind) < 0) {
        err = _EndpointGetError(ref);
        err = kBindFailed;
        goto done;
    }
    on = 1;
    if (setsockopt(s->socket, kSolSocket, kSoReuseAddr, &on, sizeof(on)) < 0)
        goto failed;
    e = _DatagramConnResetControl(ref, s->reportReset);
    if (e != 0)
        err = e;
    err = 0;
    AddressComposeEmpty(&s->local);
    length = kAddressBytes;
    if (getsockname(s->socket, &s->local.family, &length) != 0) {
        if (_EndpointGetError(ref) != kWsaEinval) {
        failed:
            err = _EndpointGetError(ref);
            err = kOptionFailed;
            goto done;
        }
        AddressComposeTCPIP(&s->local, 0, 0);
    }
    s->local.kind = length;
    s->state = kStateOpen;
    s->waitRecv = 0;
    s->waitSend = 0;
    _PlatformResumeThread(s->thread2);
    if (address != 0)
        AddressDuplicate(address, &s->local);
done:
    if (err != 0)
        DatagramClose(ref, 0);
    return err;
}

/* DatagramSend (queue a packet to send and wake the sender; a closed socket hands it
   straight back as sent). STATE: MATCH 100%, exe 52 / obj 52, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052a7f3
long DatagramSend(void *ref, NetPacket *packet)
{
    char token[16];
    NetSocket *s;
    long err;
    long state;

    s = (NetSocket *)ref;
    err = 0;
    state = s->state;
    if (state == 0) {
        err = kNotOpen;
    } else {
        AtomicEnterCritical(token);
        s->sendBytes += packet->length;
        s->sendCount++;
        AtomicLeaveCritical(token);
        QueueInsert(&s->queue, &packet->link);
        packet = 0;
        _PlatformResumeThread(s->thread);
    }
    if (packet != 0) {
        QueueInsert(s->sentList, &packet->link);
        NOTIFY(s, kEventSent);
    }
    return err;
}

/* DatagramClose (close a socket). STATE: MATCH 100%, exe 40 / obj 40, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052a87e
void DatagramClose(void *ref, long unused)
{
    NetSocket *s;
    long err;
    long state;

    s = (NetSocket *)ref;
    err = 0;
    SystemCheckCallback();
    if (s->socket >= 0) {
        state = s->state;
        if (state != 0)
            s->state = kStateClosing;
        _PlatformResumeThread(s->thread);
        if (closesocket(s->socket) < 0)
            err = _EndpointGetError(ref);
        s->socket = kNoSocket;
        s->sendBytes = 0;
        s->sendCount = 0;
    }
    s->state = 0;
    NetworkMakeCallback(s->proc, s->ref, s, kEventClosed);
}

/* DatagramGetAddress (a UDP socket's local address; cleared when it is not open). STATE: MATCH
   100%, exe 22 / obj 22, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a8f2
long DatagramGetAddress(void *ref, NetAddress *out)
{
    NetSocket *s;
    long err;

    s = (NetSocket *)ref;
    err = 0;
    switch (s->state) {
    case 0:
        err = kNotOpen;
        break;
    }
    if (err != 0)
        AddressComposeEmpty(out);
    else
        AddressDuplicate(out, &s->local);
    return err;
}

/* _DatagramConnResetControl (SIO_UDP_CONNRESET through WSAIoctl, looked up once; on Windows 9x only
   turning it off succeeds; the input buffer is the one-byte argument with a length of 1).
   STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a926
long _DatagramConnResetControl(void *ref, unsigned char enable)
{
    NetSocket *s;
    ULong returned;
    long err;

    s = (NetSocket *)ref;
    err = 0;
    returned = 0;
    if ((GetVersion() & kWin32sOr9x) != 0) {
        if (enable != 0)
            err = kUnsupported;
        goto done;
    }
    if (gDatagramWSAIoctlProc == 0)
        gDatagramWSAIoctlProc = (WSAIoctlProc)GetProcAddress(GetModuleHandleA("WS2_32.dll"),
                                                    "WSAIoctl");
    if (gDatagramWSAIoctlProc == 0) {
        err = kUnsupported;
        goto done;
    }
    if (gDatagramWSAIoctlProc(s->socket, kSioUdpConnReset, &enable, 1, 0, 0, &returned, 0, 0) < 0)
        err = _EndpointGetError(ref);
done:
    return err;
}

/* _DatagramReceive (receive one datagram into a free packet: to recvList when it has bytes,
   back to freeList when empty; a connection reset records the sender once). STATE: MATCH
   100%, exe 132 / obj 132, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052a9bd
long _DatagramReceive(void *ref)
{
    NetSocket *s;
    long err;
    NetPacket *packet;
    int fromLength;
    long got;

    s = (NetSocket *)ref;
    err = 0;
    packet = 0;
    packet = (NetPacket *)NetworkQueuePoll(s->freeList);
    if (packet == 0) {
        s->waitRecv = 1;
        NOTIFY(s, kEventNeedBuffers);
        packet = (NetPacket *)NetworkQueuePoll(s->freeList);
        if (packet != 0)
            s->waitRecv = 0;
    }
    if (packet == 0)
        goto done;

    fromLength = kAddressBytes;
    got = recvfrom(s->socket, packet->data, packet->capacity, 0, &packet->from.family,
                   &fromLength);
    packet->from.kind = fromLength;
    if (got >= 0) {
        packet->length = got;
        QueueInsert(packet->length != 0 ? s->recvList : s->freeList, &packet->link);
        packet = 0;
    } else {
        err = _EndpointGetError(ref);
    }

    if (packet != 0 && err == kWsaEconnreset) {
        if (s->reportReset != 0 && AddressIsEmpty(&s->reset)) {
            CriticalEnter(s->lock);
            AddressDuplicate(&s->reset, &packet->from);
            CriticalLeave(s->lock);
            NOTIFY(s, kEventReset);
        }
    } else if (packet != 0 && err != kWsaEintr && err != kWsaEinval && err != kWsaEnotsock) {
        err = kReceiveFailed;
    } else {
        err = 0;
        if (got > 0)
            NOTIFY(s, kEventReceived);
    }
done:
    if (packet != 0)
        QueueInsert(s->freeList, &packet->link);
    return err;
}

/* _DatagramRecvThread (the receiver thread: receive while open and buffered, else suspend itself).
   STATE: MATCH 100%, exe 31 / obj 31, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ab29
ULong __stdcall _DatagramRecvThread(void *param)
{
    NetSocket *s;
    long err, e;

    s = param;
    e = 0;
    err = 0;
    while (s->quit == 0) {
        if (s->waitRecv == 0 && (int)s->state == kStateOpen) {
            e = _DatagramReceive(s);
            if (e == 0)
                continue;
        } else if (s->quit == 0) {
            _PlatformSuspendThread(GetCurrentThread());
        }
    }
    s->done2 = 1;
    return 0;
}

/* _DatagramSendThread (the sender thread: send the queue while open, hand each packet back as
   sent; a closed socket hands back the rest unsent). STATE: MATCH 100%, exe 86 / obj 86,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ab7c
ULong __stdcall _DatagramSendThread(void *param)
{
    char token[16];
    NetSocket *s;
    NetPacket *packet;
    long e;

    s = param;
    e = 0;
    packet = 0;
    while (s->quit == 0) {
        while (s->quit == 0 && (int)s->state == kStateOpen &&
               (packet = (NetPacket *)QueueRemove(&s->queue)) != 0) {
            e = sendto(s->socket, packet->data, packet->length, 0, &packet->from.family,
                       packet->from.kind);
            if (e < 0)
                _EndpointGetError(s);
            AtomicEnterCritical(token);
            s->sendBytes -= packet->length;
            s->sendCount--;
            AtomicLeaveCritical(token);
            QueueInsert(s->sentList, &packet->link);
            NOTIFY(s, kEventSent);
        }
        if ((int)s->state == kStateClosing || (int)s->state == 0) {
            s->sendBytes = 0;
            s->sendCount = 0;
            QueueTransfer(s->sentList, &s->queue);
        }
        if (s->quit == 0)
            _PlatformSuspendThread(GetCurrentThread());
    }
    s->done = 1;
    return 0;
}

/* The TCP stream socket (NetStream, 0x378 bytes). States: 0 closed, 1 listening, 2 a
   connection waiting to be taken, 3 connecting, 4 connected, 5 sending shut down, 6 the
   peer closed, 7 closing, 8 failed. */

#define kTcpSocket          0x378
#define kSockStream         1
#define kIpprotoTcp         6
#define kSoLinger           0x80
#define kLingerSeconds      5
#define kShutdownSend       1
#define kShutdownBoth       2
#define kTimedOut           -7008
#define kRefused            -7110
#define kConnectFailed      -7105
#define kErrorRefused       1225
#define kWsaEconnaborted    10053
#define kWsaEconnrefused    10061

#define kStreamListening    1
#define kStreamWaiting      2
#define kStreamConnecting   3
#define kStreamSendDown     5
#define kStreamPeerClosed   6
#define kStreamFailed       8

#define kEventIncoming      15
#define kEventEstablished   16

#define SNOTIFY(s, event) \
    NetworkMakeCallback((s)->proc, (s)->ref, (NetSocket *)(s), (event))

typedef struct Linger {
    unsigned short onOff;
    unsigned short seconds;
} Linger;

ULong __stdcall _StreamRecvThread(void *param);
ULong __stdcall _StreamSendThread(void *param);
long _StreamConnected(NetSocket *sock);
long _StreamHasPending(NetStream *s);
void _StreamPeerClosed(NetStream *s, unsigned short event);
long _StreamReceive(void *ref);

/* StreamCreate (a new TCP socket: an auto-reset event and two suspended threads; the socket
   is returned even when a step failed). STATE: MATCH 100%, exe 129 / obj 129, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ac84
NetStream *StreamCreate(void *unused, void *ref, NetProc proc, MutexList *sentList,
                        MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    NetStream *s;
    ULong threadId;
    long err, e;

    err = 0;
    s = 0;
    s = MemoryAllocate(kTcpSocket);
    if (s == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(s, 0, kTcpSocket);
    s->kind = kListStream;
    s->unk6 = 0;
    s->state = 0;
    s->proc = proc;
    s->ref = ref;
    s->sentList = sentList;
    s->freeList = freeList;
    s->recvList = recvList;
    s->recvList2 = recvList2;
    s->socket = kNoSocket;
    e = CriticalCreate(s->lock);
    if (e != 0) {
        err = e;
        goto done;
    }
    threadId = 0;
    s->accepted = kNoSocket;
    AddressComposeEmpty(&s->acceptedPeer);
    e = QueueCreate(&s->queue);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((s->event = CreateEventA(0, 0, 0, 0)) == 0)
        goto noThread;
    if ((s->thread = CreateThread(0, kThreadStack, _StreamSendThread, s, CREATE_SUSPENDED,
                                  &threadId)) == 0) {
    noThread:
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (SetThreadPriority(s->thread, THREAD_PRIORITY_HIGHEST) == 0)
        goto noPriority;
    if ((s->thread2 = CreateThread(0, kThreadStack, _StreamRecvThread, s, CREATE_SUSPENDED,
                                   &threadId)) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (SetThreadPriority(s->thread2, THREAD_PRIORITY_HIGHEST) == 0) {
    noPriority:
        err = GetLastError();
        err = kThreadFailed;
        goto done;
    }
    _EndpointRegister((NetSocket *)s, kListStream);
    SNOTIFY(s, kEventCreated);
    SNOTIFY(s, kEventClosed);
done:
    return s;
}

/* StreamTickle (receive buffers were given back: wake the receiver if it waited). STATE:
   MATCH 100%, exe 31 / obj 31, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ae20
void StreamTickle(NetStream *s)
{
    long state;
    long err;

    err = 0;
    state = s->state;
    if (state != 0)
        SNOTIFY(s, kEventConnected);
    if ((s->waitRecv != 0 || s->waitSend != 0) && QueueIsEmpty(s->freeList) == 0) {
        s->waitRecv = 0;
        s->waitSend = 0;
        _PlatformResumeThread(s->thread2);
    }
}

/* StreamDispose (dispose of a TCP socket). STATE: MATCH 100%, exe 96 / obj 96,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052ae7f
void StreamDispose(NetSocket *sock)
{
    NetStream *s;
    long err;

    s = (NetStream *)sock;
    err = 0;
    StreamClose(sock, 0);
    s->quit = 1;
    while ((s->thread != 0 && s->done == 0) || (s->thread2 != 0 && s->done2 == 0)) {
        if (s->thread != 0 && s->done == 0)
            _PlatformResumeThread(s->thread);
        if (s->thread2 != 0 && s->done2 == 0)
            _PlatformResumeThread(s->thread2);
        SleepEx(kSleepMs, 1);
    }
    if (s->thread2 != 0 && CloseHandle(s->thread2) == 0)
        err = GetLastError();
    if (s->thread != 0 && CloseHandle(s->thread) == 0)
        err = GetLastError();
    if (s->event != 0 && CloseHandle(s->event) == 0)
        err = GetLastError();
    QueueTransfer(s->freeList, &s->queue);
    QueueDispose(&s->queue);
    SNOTIFY(s, kEventDisposed);
    CriticalDispose(s->lock);
    _EndpointUnregister((NetSocket *)s, kListStream);
    MemoryDeallocate(s);
}

/* StreamAcceptServer (take a listener's waiting connection: onto the listener itself, onto
   another socket, or, with none, refuse it; refusing leaves the listener's accepted socket 0,
   not -1). STATE: MATCH 100%, exe 115 / obj 115, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052af98
long StreamAcceptServer(void *ref, void *other)
{
    NetStream *listener;
    NetStream *s;
    long err;

    listener = (NetStream *)ref;
    s = (NetStream *)other;
    err = 0;
    if (listener == s) {
        if (closesocket(listener->socket) < 0)
            err = _EndpointGetError(ref);
        s->socket = listener->accepted;
        listener->accepted = kNoSocket;
        AddressDuplicate(&s->remote, &listener->acceptedPeer);
        AddressComposeEmpty(&listener->acceptedPeer);
        s->waitRecv = 0;
        s->waitSend = 0;
        s->state = kStreamConnected;
        SNOTIFY(s, kEventEstablished);
    } else if (s != 0) {
        AddressDuplicate(&s->local, &listener->local);
        AddressDuplicate(&s->remote, &listener->remote);
        s->socket = listener->accepted;
        listener->accepted = kNoSocket;
        AddressDuplicate(&s->remote, &listener->acceptedPeer);
        AddressComposeEmpty(&listener->acceptedPeer);
        listener->state = kStreamListening;
        s->waitRecv = 0;
        s->waitSend = 0;
        s->state = kStreamConnected;
        SNOTIFY(s, kEventEstablished);
        _PlatformResumeThread(listener->thread2);
    } else {
        goto refuse;
    }
    _PlatformResumeThread(s->thread2);
    goto done;

refuse:
    if (closesocket(listener->accepted) < 0)
        err = _EndpointGetError(ref);
    listener->accepted = 0;
    AddressComposeEmpty(&listener->acceptedPeer);
    listener->state = kStreamListening;
    _PlatformResumeThread(listener->thread2);
done:
    return err;
}

/* StreamOpenClient (connect: bind to local or any, then let the sender thread connect; with a
   timeout, wait for it). STATE: MATCH 100%, exe 108 / obj 108, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052b104
long StreamOpenClient(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout)
{
    NetStream *s;
    NetAddress any;
    long wait;
    long err, e;

    s = (NetStream *)ref;
    err = 0;
    if ((s->socket = socket(AF_INET, kSockStream, kIpprotoTcp)) < 0) {
        err = _EndpointGetError(ref);
        err = kSocketFailed;
        goto done;
    }
    if (local == 0)
        AddressComposeTCPIP(local = &any, 0, 0);
    if (bind(s->socket, &local->family, local->kind) < 0) {
        err = _EndpointGetError(ref);
        err = kBindFailed;
        goto done;
    }
    if (local != 0)
        AddressDuplicate(&s->local, local);
    AddressDuplicate(&s->remote, remote);
    s->state = kStreamConnecting;
    s->connectError = 0;
    ResetEvent(s->event);
    _PlatformResumeThread(s->thread);
    if (timeout != 0) {
        wait = WaitForSingleObject(s->event, timeout);
        switch (wait) {
        case WAIT_TIMEOUT:
            s->state = kStreamFailed;
            if (closesocket(s->socket) < 0)
                err = _EndpointGetError(ref);
            WaitForSingleObject(s->event, INFINITE);
            err = kTimedOut;
            break;
        case 0:
        case 0x80:
        default:
            e = s->connectError;
            if (e != 0)
                err = e;
            break;
        case WAIT_FAILED:
            err = GetLastError();
            err = kOptionFailed;
            break;
        }
    }
done:
    return err;
}

/* StreamSend (queue bytes to send and wake the sender; a closed or failed socket hands the
   chunk straight back). STATE: MATCH 100%, exe 54 / obj 54, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052b25b
long StreamSend(void *ref, NetChunk *chunk)
{
    char token[16];
    NetStream *s;
    long err;
    long state;

    s = (NetStream *)ref;
    err = 0;
    state = s->state;
    if (state == 0 || state == kStreamFailed) {
        err = kNotOpen;
    } else {
        AtomicEnterCritical(token);
        s->sendBytes += chunk->length;
        s->sendCount++;
        AtomicLeaveCritical(token);
        QueueInsert(&s->queue, &chunk->link);
        chunk = 0;
        _PlatformResumeThread(s->thread);
    }
    if (chunk != 0) {
        QueueInsert(s->sentList, &chunk->link);
        SNOTIFY(s, kEventSent);
    }
    return err;
}

/* StreamClose (close a TCP socket: SO_LINGER on, 5 s when graceful else 0; a waiting
   accepted socket is closed too). STATE: MATCH 100%, exe 78 / obj 78, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052b2e8
void StreamClose(NetSocket *sock, unsigned char graceful)
{
    Linger linger;
    long err;
    long state;
    long accepted;
    NetStream *s;

    s = (NetStream *)sock;
    err = 0;
    SystemCheckCallback();
    if (s->socket >= 0) {
        state = s->state;
        if (state != 0)
            s->state = kStateClosing;
        linger.onOff = 1;
        linger.seconds = graceful != 0 ? kLingerSeconds : 0;
        if (setsockopt(s->socket, kSolSocket, kSoLinger, &linger, sizeof(linger)) < 0)
            err = _EndpointGetError(sock);
        _PlatformResumeThread(s->thread);
        if (closesocket(s->socket) < 0)
            err = _EndpointGetError(sock);
        s->socket = kNoSocket;
        s->sendBytes = 0;
        s->sendCount = 0;
        accepted = s->accepted;
        if (accepted >= 0) {
            if (closesocket(accepted) < 0)
                err = _EndpointGetError(sock);
            s->accepted = kNoSocket;
        }
        AddressComposeEmpty(&s->acceptedPeer);
    }
    s->state = 0;
    SNOTIFY(s, kEventClosed);
}

/* StreamGetAddress (a TCP socket's local and remote addresses; cleared when closed). STATE:
   MATCH 100%, exe 40 / obj 40, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b3d9
long StreamGetAddress(void *ref, NetAddress *local, NetAddress *remote)
{
    NetStream *s;
    long err;
    long state;

    s = (NetStream *)ref;
    err = 0;
    state = s->state;
    if (state == 0)
        err = kNotOpen;
    else if (state == kStreamWaiting)
        err = 0;
    if (err != 0) {
        AddressComposeEmpty(local);
        AddressComposeEmpty(remote);
    } else {
        AddressDuplicate(local, &s->local);
        AddressDuplicate(remote, &s->remote);
    }
    return err;
}

/* _StreamConnected (the connection is up: read the local address, wake everyone). STATE: MATCH 100%, exe 65 / obj 65, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052b434
long _StreamConnected(NetSocket *sock)
{
    NetStream *s;
    long err;
    int length;
    long state;

    s = (NetStream *)sock;
    err = 0;
    state = s->state;
    if (state == 0) {
        err = kNotOpen;
    } else {
        AddressComposeEmpty(&s->local);
        length = kAddressBytes;
        if (getsockname(s->socket, &s->local.family, &length) < 0) {
            err = _EndpointGetError((NetSocket *)s);
            err = kOptionFailed;
        } else {
            s->local.kind = length;
            s->state = kStreamConnected;
            s->waitRecv = 0;
            s->waitSend = 0;
            SetEvent(s->event);
            SNOTIFY(s, kEventEstablished);
            _PlatformResumeThread(s->thread2);
        }
    }
    if (err != 0) {
        shutdown(s->socket, kShutdownBoth);
        _StreamPeerClosed((NetStream *)sock, kEventFailed);
    }
    return err;
}

/* _StreamHasPending (a listener has a connection waiting). STATE: MATCH 100%, exe 18 / obj 18,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b4ef
long _StreamHasPending(NetStream *s)
{
    long err;

    err = 0;
    switch (s->state) {
    case 0:
        err = kNotOpen;
        break;
    default:
        s->state = kStreamWaiting;
        SNOTIFY(s, kEventIncoming);
    }
    return err;
}

/* _StreamPeerClosed (the peer closed, the connection failed, or it was aborted: the state under
   the lock, then the event). STATE: MATCH 100%, exe 54 / obj 54, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b520
void _StreamPeerClosed(NetStream *s, unsigned short event)
{
    char token[16];
    long err;

    err = 0;
    switch (event) {
    case kEventPeerClosed:
        AtomicEnterCritical(token);
        s->state = kStreamPeerClosed;
        AtomicLeaveCritical(token);
        SNOTIFY(s, kEventPeerClosed);
        break;
    case kEventFailed:
        AtomicEnterCritical(token);
        s->state = kStreamFailed;
        AtomicLeaveCritical(token);
        SNOTIFY(s, kEventFailed);
        break;
    case kEventAborted:
        AtomicEnterCritical(token);
        s->state = 0;
        AtomicLeaveCritical(token);
        SNOTIFY(s, kEventAborted);
        break;
    }
}

/* _StreamReceive (receive into one free chunk). STATE: MATCH 100%, exe 112 / obj 112,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b5ae
long _StreamReceive(void *ref)
{
    NetStream *s;
    long err;
    NetChunk *chunk;
    long got;
    unsigned char failed;
    long code;

    s = (NetStream *)ref;
    err = 0;
    chunk = 0;
    chunk = (NetChunk *)NetworkQueuePoll(s->freeList);
    if (chunk == 0) {
        s->waitRecv = 1;
        SNOTIFY(s, kEventNeedBuffers);
        chunk = (NetChunk *)NetworkQueuePoll(s->freeList);
        if (chunk != 0)
            s->waitRecv = 0;
    }
    if (chunk == 0)
        goto done;
    got = recv(s->socket, chunk->data, chunk->size, 0);
    if (got > 0) {
        chunk->length = got;
        QueueInsert(s->recvList, &chunk->link);
        chunk = 0;
    }
    if (got > 0) {
        SNOTIFY(s, kEventReceived);
    } else if (got == 0) {
        _StreamPeerClosed(s, kEventPeerClosed);
    } else {
        failed = 0;
        err = _EndpointGetError(ref);
        switch (err) {
        case kWsaEintr:
        case kWsaEnotsock:
            err = 0;
            break;
        case kWsaEconnaborted:
            _StreamPeerClosed(ref, kEventAborted);
            break;
        case kWsaEconnreset:
            _StreamPeerClosed(ref, kEventFailed);
            break;
        default:
            failed = 1;
            break;
        }
        code = err;
        if (err != 0)
            err = (short)(failed != 0 ? kReceiveFailed : kNotOpen);
    }
done:
    if (chunk != 0)
        QueueInsert(s->freeList, &chunk->link);
    return err;
}

/* _StreamRecvThread (the receiver thread: accept while listening, receive while connected).
   STATE: MATCH 100%, exe 86 / obj 86, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0052b6c0
ULong __stdcall _StreamRecvThread(void *param)
{
    char token[16];
    int length;
    NetAddress peer;
    NetStream *s;
    long accepted;
    long result;
    long e;
    long err;
    int state;
    long code;

    s = param;
    err = 0;
    code = 0;
    accepted = 0;
    while (s->quit == 0) {
        state = s->state;
        if (state == kStreamListening) {
            AddressComposeEmpty(&peer);
            length = kAddressBytes;
            accepted = accept(s->socket, &peer.family, &length);
            peer.kind = length;
            if (accepted >= 0) {
                AtomicEnterCritical(token);
                s->accepted = accepted;
                AddressDuplicate(&s->acceptedPeer, &peer);
                accepted = kNoSocket;
                AtomicLeaveCritical(token);
                e = _StreamHasPending(s);
                if (e != 0)
                    err = e;
            } else {
                state = s->state;
                if (state != 0 && state != kStateClosing)
                    err = _EndpointGetError((NetSocket *)s);
            }
        } else if (s->waitRecv == 0 &&
                   (state == kStreamConnected || state == kStreamSendDown)) {
            result = _StreamReceive(s);
            if (result != 0)
                err = result;
        } else if (s->quit == 0) {
            _PlatformSuspendThread(GetCurrentThread());
        }
    }
    s->done2 = 1;
    return 0;
}

/* _StreamSendThread (the sender thread: connect, then send the queue while connected or while
   the peer has only closed its side; shut sending down on request; a refused connect drops
   the socket handle without closing it). STATE: MATCH 100%, exe 151 / obj 151,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052b7c1
ULong __stdcall _StreamSendThread(void *param)
{
    char token[16];
    NetStream *s;
    NetChunk *chunk;
    long err, e;

    s = param;
    err = 0;
    chunk = 0;
    while (s->quit == 0) {
        if ((int)s->state == kStreamConnecting) {
            if (connect(s->socket, &s->remote.family, s->remote.kind) < 0) {
                e = _EndpointGetError((NetSocket *)s);
                if (e == kWsaEconnrefused || e == kErrorRefused) {
                    err = e;
                    s->connectError = kRefused;
                } else if (e == kWsaEintr || e == kWsaEnotsock) {
                    err = e;
                    s->connectError = kTimedOut;
                } else {
                    err = e;
                    s->connectError = kConnectFailed;
                }
                _StreamPeerClosed(s, kEventFailed);
                s->socket = kNoSocket;
                s->sendBytes = 0;
                s->sendCount = 0;
                SetEvent(s->event);
            } else {
                e = _StreamConnected((NetSocket *)s);
                if (e != 0)
                    err = e;
            }
        }
        while (((int)s->state == kStreamConnected || (int)s->state == kStreamPeerClosed) &&
               (chunk = (NetChunk *)QueueRemove(&s->queue)) != 0) {
            if (send(s->socket, chunk->data, chunk->length, 0) < 0)
                err = _EndpointGetError((NetSocket *)s);
            AtomicEnterCritical(token);
            s->sendBytes -= chunk->length;
            s->sendCount--;
            AtomicLeaveCritical(token);
            QueueInsert(s->sentList, &chunk->link);
            SNOTIFY(s, kEventSent);
        }
        if (s->shutdownSend != 0) {
            s->state = kStreamSendDown;
            if (shutdown(s->socket, kShutdownSend) < 0)
                err = _EndpointGetError((NetSocket *)s);
            s->shutdownSend = 0;
        }
        if ((int)s->state == kStreamFailed || (int)s->state == kStateClosing ||
            (int)s->state == kStreamSendDown || (int)s->state == 0) {
            s->sendBytes = 0;
            s->sendCount = 0;
            QueueTransfer(s->sentList, &s->queue);
        }
        if (s->quit == 0)
            _PlatformSuspendThread(GetCurrentThread());
    }
    s->done = 1;
    return 0;
}

