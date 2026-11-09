/* atnetotstream.c -- 1.0.10: the network layer on Open Transport: TCP and ADSP sockets. Notes: Research/notes/maclibs.md */
#include <OpenTransport.h>
#include <OpenTptClient.h>
#include <OpenTptInternet.h>
#include <OpenTptAppleTalk.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"
#if TARGET_API_MAC_CARBON
#include <Multiprocessing.h>
#endif
extern unsigned char _PlatformIsThreaded(void);

extern long CriticalCreateFromProvider(void *lock, ProviderRef provider);

#if TARGET_API_MAC_CARBON
#define OTOpenEndpoint(c, o, i, e) OTOpenEndpointInContext(c, o, i, e, 0)
#define MemoryClear(p, n) BlockZero(p, n)
#define kStreamNotifier gStreamNotifierUPP
#else
#define kStreamNotifier StreamNotifierOT
#endif

#define kNetErrConfig       -7100
#define kErrFlow            -3161
#define kErrBusy            -3211
#define kErrLook            -3158
#define kErrGone            -3153
#define kErrNoData          -3162
#define kAcceptFailed       -7103

extern OTNotifyUPP gStreamNotifierUPP;

#define kOTOpenFailed       -7102
#define kFlagNoBuffer       4
#define kNotOpen            -7109
#define kEventNeedBuffers   14
#define kFlagInNotifierTCP  19
#define kEventIncoming      15
#define kEventConnectedStream 16
#define kOTCanceledResult   -3180
#define kOTSyncIdleEvent    0x20000011
#define kOTUnbindCompleteEvent 0x20000005
#define kOTGetProtAddressEvent 0x20000008

extern unsigned char gIdleHookEnabled;
#define kEventAborted       19

#define kFlagRecvWait       8
#define kFlagReset          16
#define kNameBytes          0x100
#define kFlagSendBusy       7
#define kFlagFlow           9
#define kFlagData           2
#define kEventSent          10
#define kEventReceived      12
#define kEventReceived2     13
#define kStreamBytes        0x480
#define kChunkStride        0x11c
#define kFlagDispose        17
#define kStateIdle          1
#define kStateConnecting    3
#define kStateDead          8
#define kOpenFailedStream   -7105
#define kOpenRefusedStream  -7110
#define kTimedOutStream     -7008
#define kErrNoStructure     -3208
#define kErrNotSupported    -3168

unsigned char gOTOpen = 0;
extern long gOTSocketCount;

/* a socket on Open Transport's TCP or ADSP */

typedef struct OTStream {
    struct OTStream *next;
    unsigned short kind;
    unsigned short stack;
    unsigned short state;
    unsigned short padA;
    ULong lock[4];
    NetProc proc;
    void *ref;
    ULong flags;
    MutexList *sentList;
    MutexList *freeList;
    MutexList *recvList;
    MutexList *recvList2;
    NetAddress local;
    NetAddress remote;
    long unk240;
    EndpointRef endpoint;
    OTSequence sequence;
    MapperRef mapper;
    OTNameID nameID;
    TBind bind;
    NetAddress peer;
    MutexList sendQueue;
    NetChunk *sending;
    long sendBytes;
} OTStream;

extern long StreamOpenEndpointOT(void *ref);
extern void StreamPostOT(OTStream *s);
extern void StreamReceiveOT(void *ref);
extern void StreamLookOT(OTStream *s);
extern void StreamEndpointCloseOT(OTStream *s);
extern pascal void StreamNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie);

/* MSL's strcpy and strlen, which the exe has inline */
static inline char *MslCopy(char *dst, const char *src)
{
    unsigned char *q = (unsigned char *)dst - 1;
    const unsigned char *p = (const unsigned char *)src - 1;

    while ((*++q = *++p) != 0)
        ;
    return dst;
}

static inline char *MslAppend(char *dst, const char *src)
{
    const unsigned char *p = (const unsigned char *)src - 1;
    unsigned char *q = (unsigned char *)dst - 1;

    while (*++q != 0)
        ;
    q--;
    while ((*++q = *++p) != 0)
        ;
    return dst;
}

static inline long MslLength(const char *s)
{
    const unsigned char *p;
    long n;

    p = (const unsigned char *)s - 1;
    n = -1;
    do {
        n++;
    } while (*++p != 0);
    return n;
}

NetStream *StreamCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    long err, e;
    OTStream *s;

    err = 0;
    s = MemoryAllocate(kStreamBytes);
    if (s == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(s, kStreamBytes);
    s->kind = 3;
    s->stack = stack;
    s->state = 0;
    s->proc = proc;
    s->ref = ref;
    s->flags = 0;
    s->freeList = freeList;
    s->sentList = sentList;
    s->recvList = recvList;
    s->recvList2 = recvList2;
    s->sending = 0;
    e = QueueCreate(&s->sendQueue);
    if (e != 0) {
        err = e;
        goto done;
    }
    s->unk240 = 0;
    e = StreamOpenEndpointOT(s);
    if (e != 0) {
        err = e;
        goto done;
    }
    _EndpointRegister((NetSocket *)s, 3);
    gOTSocketCount++;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventCreated);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
done:
    if (err != 0 && s != 0) {
        StreamDisposeOT((NetSocket *)s);
        s = 0;
    }
    return (NetStream *)s;
}

void StreamTickleOT(OTStream *s)
{
    long state;
    Boolean entered;

    state = s->state;
    if (state != 0)
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnected);
    if (s->endpoint != 0) {
        if (!QueueIsEmpty(s->freeList) && AtomicMaskClear(&s->flags, kFlagNoBuffer)) {
            entered = OTEnterNotifier(s->endpoint);
            StreamReceiveOT(s);
            if (entered)
                OTLeaveNotifier(s->endpoint);
        }
        if (AtomicMaskClear(&s->flags, kFlagRecvWait)) {
            entered = OTEnterNotifier(s->endpoint);
            StreamPostOT(s);
            if (entered)
                OTLeaveNotifier(s->endpoint);
        }
    }
}

void StreamDisposeOT(NetSocket *sock)
{
    OTStream *s;

    s = (OTStream *)sock;
    AtomicMaskApply(&s->flags, kFlagDispose);
    StreamCloseOT(sock, 0);
    StreamEndpointCloseOT(s);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventDisposed);
    QueueDispose(&s->sendQueue);
    _EndpointUnregister((NetSocket *)s, 3);
    MemoryDeallocate(s);
    gOTSocketCount--;
}

long StreamAcceptServerOT(OTStream *s, void *other)
{
    long err;
    OTStream *o;
    unsigned short state;
    OTResult result;
    TCall call;

    o = other;
    err = 0;
    if ((long)s->state == 0) {
        err = kNotOpen;
        goto done;
    }
    if (o != 0) {
        do {
            AtomicMaskClear(&o->flags, kFlagNoBuffer);
            if (s != o) {
                AddressDuplicate(&o->local, &s->local);
                AddressDuplicate(&o->remote, &s->remote);
                AddressComposeEmpty(&s->remote);
            }
            state = o->state;
            o->state = kStateConnecting;
            call.addr.maxlen = 0;
            call.addr.len = 0;
            call.addr.buf = 0;
            call.opt.maxlen = 0;
            call.opt.len = 0;
            call.opt.buf = 0;
            call.udata.maxlen = 0;
            call.udata.len = 0;
            call.udata.buf = 0;
            call.sequence = s->sequence;
            result = OTAccept(s->endpoint, o->endpoint, &call);
            if (result < 0) {
                if (result == kErrLook) {
                    StreamLookOT(s);
                } else {
                    o->state = state;
                    err = kOpenFailedStream;
                    goto done;
                }
            } else if (o != s) {
                s->state = kStateIdle;
            }
        } while (result < 0);
    } else {
        OSStatus e;

        call.addr.maxlen = 0;
        call.addr.len = 0;
        call.addr.buf = 0;
        call.opt.maxlen = 0;
        call.opt.len = 0;
        call.opt.buf = 0;
        call.udata.maxlen = 0;
        call.udata.len = 0;
        call.udata.buf = 0;
        call.sequence = s->sequence;
        s->state = kStateIdle;
        e = OTSndDisconnect(s->endpoint, &call);
        if (e != 0)
            err = e;
        AddressComposeEmpty(&s->remote);
    }
done:
    return err;
}

long StreamOpenClientOT(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout)
{
    OTStream *s;
    long err;
    ULong ip;
    unsigned short port;
    unsigned short net;
    unsigned char node;
    unsigned char socket;
    unsigned char type;
    NetAddress any;
    TBind request;
    TBind reply;
    TCall call;
    TDiscon discon;
    OTResult look;
    ULongLong deadline;
    Boolean entered;
    unsigned char bind;

    s = ref;
    look = 0;
    ip = 0;
    port = 0;
    net = 0;
    node = 0;
    socket = 0;
    type = 0;
    OTRemoveNotifier(s->endpoint);
    if (OTSetSynchronous(s->endpoint) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    AtomicMaskClear(&s->flags, kFlagNoBuffer);
    AtomicMaskClear(&s->flags, kFlagData);
    s->sendBytes = 0;
    if (local != 0) {
        switch (s->stack) {
        case kStackInet:
            AddressDecomposeTCPIP(local, &port, &ip);
            bind = (port != 0 || ip != 0) != 0;
            break;
        case kStackAppleTalk:
            AddressDecomposeAppleTalk(local, &net, &node, &socket, &type);
            bind = (socket != 0 || type != 0) != 0;
            break;
        default:
            bind = 1;
            break;
        }
    } else if (_PlatformIsThreaded()) {
        switch (s->stack) {
        case kStackInet:
            local = &any;
            AddressComposeTCPIP(local, 0, 0);
            bind = 1;
            break;
        case kStackAppleTalk:
            local = &any;
            AddressComposeAppleTalk(local, 0, 0, 0, 0);
            bind = 1;
            break;
        default:
            bind = 0;
            break;
        }
    } else {
        bind = 0;
    }
    if (bind) {
        request.addr.maxlen = kNameBytes;
        request.addr.len = local->kind;
        request.addr.buf = (UInt8 *)&local->family;
    } else {
        request.addr.maxlen = 0;
        request.addr.len = 0;
        request.addr.buf = 0;
    }
    request.qlen = 1;
    reply.addr.maxlen = kNameBytes;
    reply.addr.len = s->local.kind;
    reply.addr.buf = (UInt8 *)&s->local.family;
    reply.qlen = 0;
    if (OTBind(s->endpoint, &request, &reply) != 0) {
        err = kAcceptFailed;
        goto done;
    }
    s->local.kind = reply.addr.len;
    AtomicMaskClear(&s->flags, kFlagReset);
    AtomicMaskApply(&s->flags, 0);
    if (OTInstallNotifier(s->endpoint, kStreamNotifier, s) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    if (OTSetAsynchronous(s->endpoint) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    if ((void *)OTUseSyncIdleEvents != 0 && OTUseSyncIdleEvents(s->endpoint, true) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    s->state = kStateConnecting;
    call.addr.maxlen = kNameBytes;
    call.addr.len = remote->kind;
    call.addr.buf = (UInt8 *)&remote->family;
    call.opt.maxlen = 0;
    call.opt.len = 0;
    call.opt.buf = 0;
    call.udata.maxlen = 0;
    call.udata.len = 0;
    call.udata.buf = 0;
    call.sequence = 0;
    err = OTConnect(s->endpoint, &call, 0);
    if (err == kErrNoData)
        err = 0;
    if (err == kErrLook) {
        look = OTLook(s->endpoint);
        err = 0;
    }
    if (err != 0) {
        err = kOpenFailedStream;
        goto done;
    }
    switch (look) {
    case 0:
        break;
    case T_LISTEN:
        err = OTRcvConnect(s->endpoint, &call);
        if (err == kErrLook) {
            if (OTLook(s->endpoint) == T_DISCONNECT)
                goto disconnected;
            err = kOpenFailedStream;
            goto done;
        }
        if (err != 0) {
            err = kOpenFailedStream;
            goto done;
        }
        break;
    case T_DISCONNECT:
    disconnected:
        OTRcvDisconnect(s->endpoint, &discon);
        err = kOpenRefusedStream;
        goto done;
    default:
        if (err != 0) {
            err = kOpenFailedStream;
            goto done;
        }
        break;
    }
    AddressDuplicate(&s->remote, remote);
    if (local != 0)
        AddressDuplicate(local, &s->local);
    if (timeout != 0) {
        deadline = TimerGetMilliseconds() + timeout;
        while ((long)s->state == kStateConnecting && (long long)(deadline - TimerGetMilliseconds()) > 0)
            SystemWait(10);
        if ((long)s->state == kStateConnecting) {
            entered = OTEnterNotifier(s->endpoint);
            if ((long)s->state == kStateConnecting) {
                OTSndDisconnect(s->endpoint, 0);
                if (entered)
                    OTLeaveNotifier(s->endpoint);
                err = kTimedOutStream;
                goto done;
            }
            if (entered)
                OTLeaveNotifier(s->endpoint);
        }
    }
done:
    if (err != 0)
        StreamCloseOT((NetSocket *)s, 0);
    return err;
}

long StreamSendOT(OTStream *s, NetChunk *chunk)
{
    long err;
    NetChunk *p;
    Boolean entered;
    long state;

    err = 0;
    state = s->state;
    if (state == 0 || state == kStateDead) {
        err = kNotOpen;
    } else {
        entered = OTEnterNotifier(s->endpoint);
        AtomicIncrement(&s->sendBytes, chunk->length);
        QueueInsert(&s->sendQueue, &chunk->link);
        StreamPostOT(s);
        if (entered)
            OTLeaveNotifier(s->endpoint);
    }
    return err;
}

void StreamCloseOT(NetSocket *sock, unsigned char inGraceful)
{
    unsigned char linger;
    unsigned char graceful;
    OTStream *s;
    unsigned char reopen;
    unsigned char abortive;
    ULong tries;

    s = (OTStream *)sock;
    graceful = inGraceful;
    linger = 1;
    reopen = 0;
    abortive = 0;
    SystemCheckCallback();
    if (s->endpoint != 0 && AtomicMaskTest(&s->flags, 0)) {
        OTRemoveNotifier(s->endpoint);
        OTSetSynchronous(s->endpoint);
    }
    if (s->endpoint != 0 && AtomicMaskTest(&s->flags, 0)) {
        StreamLookOT(s);
        if (AtomicMaskTest(&s->flags, kFlagReset))
            linger = 0;
        if ((long)s->state != 4 && (long)s->state != 6)
            graceful = 0;
        if ((long)s->state != kStateDead)
            abortive = 1;
        s->state = 7;
        if (linger) {
            while (graceful && s->sending != 0) {
                SystemIdle();
                StreamLookOT(s);
                StreamPostOT(s);
                if ((long)s->state != 7)
                    graceful = 0;
            }
            if (graceful) {
                OTSndOrderlyDisconnect(s->endpoint);
                for (tries = 0; tries < 3 && (long)s->state != 0; tries++) {
                    SystemIdle();
                    StreamLookOT(s);
                    if (OTGetEndpointState(s->endpoint) == 2)
                        s->state = 0;
                }
            } else if (OTGetEndpointState(s->endpoint) == 2) {
                s->state = 0;
            }
            if ((long)s->state != 0) {
                OTIoctl(s->endpoint, I_FLUSH, (void *)FLUSHRW);
                OTSndDisconnect(s->endpoint, 0);
                if (abortive) {
                    s->state = 0;
                } else {
                    for (tries = 0; tries < 3 && (long)s->state != 0; tries++) {
                        SystemIdle();
                        StreamLookOT(s);
                    }
                }
            }
        }
        if (AtomicMaskClear(&s->flags, 1) && s->mapper != 0 && s->nameID != 0) {
            OTDeleteNameByID(s->mapper, s->nameID);
            s->nameID = 0;
        }
        if (s->mapper != 0) {
            OTSetSynchronous(s->mapper);
            OTCloseProvider(s->mapper);
            s->mapper = 0;
        }
        if (OTUnbind(s->endpoint) != 0) {
            SystemIdle();
            if (OTUnbind(s->endpoint) != 0)
                reopen = 1;
        }
    }
    if (s->endpoint != 0 && !AtomicMaskTest(&s->flags, kFlagDispose)) {
        if (reopen) {
            StreamEndpointCloseOT(s);
            if (StreamOpenEndpointOT(s) != 0)
                goto done;
        } else {
            OTRemoveNotifier(s->endpoint);
            if (OTInstallNotifier(s->endpoint, kStreamNotifier, s) != 0)
                goto done;
            if (OTSetAsynchronous(s->endpoint) != 0)
                goto done;
            if ((void *)OTUseSyncIdleEvents != 0 && OTUseSyncIdleEvents(s->endpoint, true) != 0)
                goto done;
        }
    }
    if (gOTOpen == 0)
        StreamEndpointCloseOT(s);
    s->state = 0;
    AtomicMaskClear(&s->flags, kFlagNoBuffer);
    AtomicMaskClear(&s->flags, kFlagData);
    AtomicMaskClear(&s->flags, 8);
    AtomicMaskClear(&s->flags, kFlagFlow);
    AtomicMaskClear(&s->flags, 0);
    AtomicMaskClear(&s->flags, kFlagReset);
    s->sequence = 0;
    s->sending = 0;
    s->sendBytes = 0;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
done:
    return;
}

long StreamGetAddressOT(OTStream *s, NetAddress *local, NetAddress *remote)
{
    long err;
    long state;
    Boolean entered;

    err = 0;
    state = s->state;
    if (state == 0 || state == kStateDead)
        err = kNotOpen;
    if (err != 0) {
        AddressComposeEmpty(local);
        AddressComposeEmpty(remote);
    } else {
        entered = OTEnterNotifier(s->endpoint);
        AddressDuplicate(local, &s->local);
        AddressDuplicate(remote, &s->remote);
        if (entered)
            OTLeaveNotifier(s->endpoint);
    }
    return err;
}

long StreamOpenEndpointOT(void *ref)
{
    OTStream *s;
    long err;
    OTConfigurationRef config;
    OSStatus status;
    long e;

    s = ref;
    err = 0;
    config = 0;
    status = 0;
    AtomicMaskClear(&s->flags, kFlagReset);
    AtomicMaskClear(&s->flags, 0);
    switch (s->stack) {
    case kStackInet:
        config = OTCreateConfiguration("tcp");
        if (config == 0) {
            err = kOutOfMemory;
            goto done;
        }
        break;
    case kStackAppleTalk:
        config = OTCreateConfiguration("adsp");
        if (config == 0) {
            err = kOutOfMemory;
            goto done;
        }
        break;
    }
    if ((s->endpoint = OTOpenEndpoint(config, 0, 0, &status)) == 0) {
        err = kOTOpenFailed;
        goto done;
    }
    e = CriticalCreateFromProvider(s->lock, s->endpoint);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (OTInstallNotifier(s->endpoint, kStreamNotifier, s) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    if (OTSetAsynchronous(s->endpoint) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    if ((void *)OTUseSyncIdleEvents != 0 && OTUseSyncIdleEvents(s->endpoint, true) != 0)
        err = kNetErrConfig;
done:
    return err;
}

static inline NetChunk *_StreamNextChunk(OTStream *s)
{
    if (s->sending != 0)
        return s->sending;
    return (NetChunk *)NetworkQueuePoll(&s->sendQueue);
}

void StreamPostOT(OTStream *s)
{
    NetChunk *chunk;
    unsigned char held;
    unsigned char done;
    OTResult err;
    OTResult sent;
    long state;

    err = 0;
    state = s->state;
    if (state != 0 && state != kStateDead) {
        held = AtomicMaskApply(&s->flags, kFlagSendBusy);
        done = held;
        while (!done && !AtomicMaskTest(&s->flags, kFlagFlow)) {
            chunk = _StreamNextChunk(s);
            s->sending = 0;
            if (chunk == 0)
                break;
            sent = OTSnd(s->endpoint, chunk->data, chunk->length, 0);
            if (sent >= 0 && sent < chunk->length || sent == kErrFlow) {
                AtomicMaskApply(&s->flags, kFlagFlow);
                if (sent >= 0) {
                    ULong left;

                    left = chunk->length - sent;
                    chunk->length = left;
                    if (left != 0)
                        /* BUG: the stride of a whole chunk, not of a byte */
                        BlockMoveData((char *)chunk + sent * kChunkStride, chunk, left);
                    AtomicIncrement(&s->sendBytes, -sent);
                }
                s->sending = chunk;
                break;
            }
            if (sent == kErrBusy) {
                AtomicMaskApply(&s->flags, 8);
                s->sending = chunk;
                break;
            }
            if (err == kErrLook) {
                StreamLookOT(s);
            } else if (err == kErrGone) {
                state = s->state;
                if (state != 0) {
                    s->state = 0;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAborted);
                    err = kNotOpen;
                }
                StreamEndpointCloseOT(s);
                AtomicIncrement(&s->sendBytes, -chunk->length);
                QueueInsert(s->sentList, &chunk->link);
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
            } else {
                if (sent < 0)
                    err = sent;
                AtomicIncrement(&s->sendBytes, -chunk->length);
                QueueInsert(s->sentList, &chunk->link);
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
            }
        }
        if (!held) {
            AtomicMaskClear(&s->flags, kFlagSendBusy);
            if (!QueueIsEmpty(&s->sendQueue) && !AtomicMaskTest(&s->flags, kFlagFlow) && !AtomicMaskTest(&s->flags, 8))
                StreamPostOT(s);
        }
    }
}

void StreamReceiveOT(void *ref)
{
    OTStream *s;
    OTResult err;
    NetChunk *chunk;
    OTResult got;
    OTFlags flags;
    size_t count;
    long state;
    Boolean entered;
    long event;

    s = ref;
    err = 0;
    flags = 0;
    state = s->state;
    if (state != 0 && state != kStateDead)
    while (err == 0 && AtomicMaskTest(&s->flags, kFlagData)) {
        chunk = (NetChunk *)NetworkQueuePoll(s->freeList);
        if (chunk == 0 && !AtomicMaskApply(&s->flags, kFlagNoBuffer)) {
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventNeedBuffers);
            chunk = (NetChunk *)NetworkQueuePoll(s->freeList);
        }
        if (chunk == 0)
            break;
        AtomicMaskClear(&s->flags, kFlagNoBuffer);
        got = OTRcv(s->endpoint, chunk->data, chunk->size, &flags);
        if (got < 0) {
            err = got;
            got = 0;
        }
        if (err == kErrNoData || OTCountDataBytes(s->endpoint, &count) == kErrNoData || count == 0)
            AtomicMaskClear(&s->flags, kFlagData);
        if (err == 0 && (chunk->length = got) != 0) {
            QueueInsert((flags & T_EXPEDITED) != 0 ? s->recvList2 : s->recvList, &chunk->link);
            {
                NetProc proc;
                void *cbref;
                NetSocket *sock;

                proc = s->proc;
                cbref = s->ref;
                sock = (NetSocket *)s;
                event = kEventReceived;
                if ((flags & T_EXPEDITED) != 0)
                    event = kEventReceived2;
                NetworkMakeCallback(proc, cbref, sock, event);
            }
        } else {
            QueueInsert(s->freeList, &chunk->link);
            if (err == kErrLook) {
                StreamLookOT(s);
            } else if (err == -3155 && OTGetEndpointState(s->endpoint) == 7) {
                entered = OTEnterNotifier(s->endpoint);
                AtomicMaskClear(&s->flags, kFlagData);
                switch (s->state) {
                case 4:
                    s->state = 6;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 17);
                    break;
                case 5:
                    s->state = kStateDead;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 18);
                    break;
                case 7:
                    s->state = 0;
                    break;
                }
                if (entered)
                    OTLeaveNotifier(s->endpoint);
            } else if (err == kErrGone) {
                state = s->state;
                if (state != 0) {
                    s->state = 0;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAborted);
                    err = kNotOpen;
                }
                StreamEndpointCloseOT(s);
            }
            if (err != 0)
                return;
        }
    }
}

void StreamLookOT(OTStream *s)
{
    OTResult code;

    code = OTLook(s->endpoint);
    if (code != 0 && code != kErrNoStructure && code != kErrNotSupported && code >= 0)
        StreamNotifierOT(s, code, 0, s);
}

void StreamEndpointCloseOT(OTStream *s)
{
    if (s->endpoint != 0 && gOTOpen != 0) {
        OTRemoveNotifier(s->endpoint);
        OTSetSynchronous(s->endpoint);
        OTCloseProvider(s->endpoint);
#if TARGET_API_MAC_CARBON
        if (_PlatformIsThreaded() && AtomicMaskTest(&s->flags, kFlagInNotifierTCP)) {
            do {
                SystemIdle();
            } while (AtomicMaskTest(&s->flags, kFlagInNotifierTCP));
        }
#endif
        CriticalDispose(s->lock);
        s->state = 0;
    }
    s->endpoint = kOTInvalidEndpointRef;
}

pascal void StreamNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie)
{
    OSStatus status;
    OTStream *s;
    unsigned char held;

    status = 0;
    held = 0;
    if (code != T_CONNECT || result != kOTCanceledResult) {
        if (result != 0)
            status = result;
    }
    s = ref;
    if (_PlatformIsThreaded())
        held = AtomicMaskApply(&s->flags, kFlagInNotifierTCP);
    if (OTIsSynchronous(s->endpoint) == 0 || code == T_ORDREL || code == kOTProviderWillClose || code == kOTProviderIsClosed) {
        switch (code) {
        case T_LISTEN: {
            Boolean entered;
            TCall call;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            call.addr.maxlen = kNameBytes;
            call.addr.len = s->remote.kind;
            call.addr.buf = (UInt8 *)&s->remote.family;
            call.opt.maxlen = 0;
            call.opt.len = 0;
            call.opt.buf = 0;
            call.udata.maxlen = 0;
            call.udata.len = 0;
            call.udata.buf = 0;
            call.sequence = 0;
            s->sequence = 0;
            if (OTListen(s->endpoint, &call) == 0) {
                s->remote.kind = call.addr.len;
                s->sequence = call.sequence;
                s->state = 2;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventIncoming);
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_CONNECT: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            if (result != kOTCanceledResult) {
                if (OTRcvConnect(s->endpoint, 0) == kErrLook)
                    code = OTLook(s->endpoint);
                if (code == T_DISCONNECT) {
                    s->state = kStateDead;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 18);
                } else {
                    s->state = 4;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnectedStream);
                }
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_DATA:
        case T_EXDATA: {
            Boolean entered;
            long state;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            AtomicMaskApply(&s->flags, kFlagData);
            state = s->state;
            if (state == 4 || state == 5)
                StreamReceiveOT(s);
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_DISCONNECT: {
            Boolean entered;
            NetChunk *chunk;
            TDiscon discon;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            chunk = (NetChunk *)QueueRemove(s->freeList);
            if (chunk != 0) {
                discon.udata.maxlen = chunk->size;
                discon.udata.len = 0;
                discon.udata.buf = (UInt8 *)chunk->data;
                discon.reason = 0;
                discon.sequence = 0;
            }
            if (OTRcvDisconnect(s->endpoint, chunk != 0 ? &discon : 0) != 0) {
                if (chunk != 0)
                    QueueInsert(s->freeList, &chunk->link);
            } else {
                if (chunk != 0) {
                    if ((chunk->length = discon.udata.len) != 0) {
                        QueueInsert(s->recvList, &chunk->link);
                        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventReceived);
                    } else {
                        QueueInsert(s->freeList, &chunk->link);
                    }
                }
                AtomicMaskClear(&s->flags, kFlagData);
                switch (s->state) {
                case 3:
                case 4:
                case 5:
                case 6:
                    s->state = kStateDead;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 18);
                    break;
                case 7:
                    s->state = 0;
                    break;
                }
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case kOTUnbindCompleteEvent: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            if ((long)s->state == 3) {
                s->state = kStateDead;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 18);
            } else {
                s->state = 0;
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_ORDREL: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            if (OTRcvOrderlyDisconnect(s->endpoint) == 0) {
                AtomicMaskClear(&s->flags, kFlagData);
                switch (s->state) {
                case 4:
                    s->state = 6;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 17);
                    break;
                case 5:
                    s->state = kStateDead;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 18);
                    break;
                case 7:
                    s->state = 0;
                    break;
                }
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_GODATA:
        case T_GOEXDATA: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            AtomicMaskClear(&s->flags, kFlagFlow);
            StreamPostOT(s);
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_ACCEPTCOMPLETE: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_PASSCON: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            if (result != 0) {
                s->state = kStateDead;
                AtomicMaskClear(&s->flags, kFlagReset);
                AtomicMaskClear(&s->flags, 0);
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, 18);
            } else {
                s->bind.addr.maxlen = kNameBytes;
                s->bind.addr.len = s->peer.kind;
                s->bind.addr.buf = (UInt8 *)&s->peer.family;
                s->bind.qlen = 0;
                {
                    OSStatus e;

                    e = OTGetProtAddress(s->endpoint, &s->bind, 0);
                    if (e != 0)
                        status = e;
                }
                if (status != 0)
                    /* BUG: the endpoint where the notifier wants the socket */
                    StreamNotifierOT(s->endpoint, kOTGetProtAddressEvent, status, &s->bind);
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case kOTGetProtAddressEvent: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            if (result == 0 && s->bind.addr.len != 0) {
                s->peer.kind = s->bind.addr.len;
                AddressDuplicate(&s->local, &s->peer);
            }
            s->state = 4;
            AtomicMaskClear(&s->flags, kFlagReset);
            AtomicMaskApply(&s->flags, 0);
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnectedStream);
            if (AtomicMaskTest(&s->flags, kFlagData))
                StreamReceiveOT(s);
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case kOTSyncIdleEvent:
            if (gIdleHookEnabled)
                SystemIdle();
            StreamTickle((NetStream *)s);
            break;
        case kOTProviderWillClose:
        case kOTProviderIsClosed:
            if ((long)s->state != 0) {
                s->state = 0;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAborted);
            }
            break;
        default: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        }
    }
    if (_PlatformIsThreaded() && !held)
        AtomicMaskClear(&s->flags, kFlagInNotifierTCP);
}
