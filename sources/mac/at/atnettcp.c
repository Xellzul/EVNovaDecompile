/* atnettcp.c -- 1.0.10: TCP/IP without Open Transport: TCP streams on the MacTCP driver. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <MacTCP.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON

/* a socket on MacTCP's TCP. The driver's user data is a cookie that keeps pointing at the socket when an accepted connection swaps streams; it is overwritten on dispose so a late completion is seen */
#define kEventNeedBuffers   14
#define kFlagSocket         0
#define kFlagPosted         2
#define kFlagUdpBusy        6
#define kFlagHasBuffer      4
#define kStateClosing       7
#define kEventSent          10
#define kEventReceived      12
#define kOpenFailed2        -7102
#define kNotOpenStream       -7109
#define kTcpBytes           0x34c
#define kTcpBuffer          0x4000
#define kCookieDead         0xdeadbeef
#define kStateListening     1
#define kStateAccepting     2
#define kStateConnecting    3
#define kStateConnected     4
#define kStatePeerClosed    6
#define kStateDead          8
#define kEventListening     15
#define kEventAccepted      16
#define kEventPeerClosed2   17
#define kEventRemoteClosed  18
#define kEventFailed2       19
#define kSendBusy           -7105
#define kOpenRefused        -7110
#define kTimedOut           -7008
#define kConnectionExists   -23017
#define kConnectionClosing  -23012
#define kInProgress         1

typedef struct TCPMacTCP {
    struct TCPMacTCP *next;
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
    int32_t unk240;
    StreamPtr stream;
    Ptr buffer;
    TCPiopb sendPb;
    TCPiopb ctlPb;
    wdsEntry wds[2];
    MutexList sendQueue;
    NetChunk *sending;
    NetChunk *receiving;
    int32_t sendBytes;
    short pad340;
    unsigned short listenPort;
    ULong listenHost;
    struct TCPMacTCP **cookie;
} TCPMacTCP;

static void StreamListenMacTCP(TCPMacTCP *s);
void StreamPostMacTCP(TCPMacTCP *s);
void StreamReceiveMacTCP(TCPMacTCP *s);
void StreamCreateStreamMacTCP(TCPMacTCP *s);
void StreamCloseAsyncMacTCP(TCPMacTCP *s);
void StreamAbortAsyncMacTCP(TCPMacTCP *s);
static void StreamDoneMacTCP(TCPiopb *pb);
static void StreamSyncDoneMacTCP(TCPiopb *pb);
static pascal void StreamNotifyMacTCP(StreamPtr stream, unsigned short event, Ptr userData, unsigned short terminReason, struct ICMPReport *report);
static RoutineDescriptor sStreamDoneDesc = BUILD_ROUTINE_DESCRIPTOR(kCStackBased | STACK_ROUTINE_PARAMETER(1, kFourByteCode), StreamDoneMacTCP);
static UniversalProcPtr sStreamDone = &sStreamDoneDesc;
static RoutineDescriptor sStreamSyncDesc = BUILD_ROUTINE_DESCRIPTOR(kCStackBased | STACK_ROUTINE_PARAMETER(1, kFourByteCode), StreamSyncDoneMacTCP);
static UniversalProcPtr sStreamSync = &sStreamSyncDesc;
static RoutineDescriptor sStreamNotifyDesc = BUILD_ROUTINE_DESCRIPTOR(kPascalStackBased | STACK_ROUTINE_PARAMETER(1, kFourByteCode) | STACK_ROUTINE_PARAMETER(2, kTwoByteCode) | STACK_ROUTINE_PARAMETER(3, kFourByteCode) | STACK_ROUTINE_PARAMETER(4, kTwoByteCode) | STACK_ROUTINE_PARAMETER(5, kFourByteCode), StreamNotifyMacTCP);
static UniversalProcPtr sStreamNotify = &sStreamNotifyDesc;

NetStream *StreamCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    int32_t err, e;
    TCPMacTCP *s;

    err = 0;
    s = MemoryAllocate(kTcpBytes);
    if (s == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(s, kTcpBytes);
    s->kind = 3;
    s->stack = 0;
    s->state = 0;
    s->proc = proc;
    s->ref = ref;
    s->flags = 0;
    s->freeList = freeList;
    s->sentList = sentList;
    s->recvList = recvList;
    s->recvList2 = recvList2;
    s->unk240 = 0;
    if ((s->cookie = MemoryAllocate(sizeof(*s->cookie))) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    *s->cookie = s;
    e = CriticalCreate(s->lock);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((s->buffer = MemoryAllocate(kTcpBuffer)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    _EndpointRegister((NetSocket *)s, 3);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventCreated);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
done:
    if (err != 0 && s != 0) {
        s->proc = 0;
        s->ref = 0;
        StreamDisposeMacTCP((NetSocket *)s);
        s = 0;
    }
    return (NetStream *)s;
}

void StreamTickleMacTCP(TCPMacTCP *s)
{
    int32_t state;

    state = s->state;
    if (state != 0)
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnected);
    state = s->state;
    if (state == kStateConnected && !QueueIsEmpty(s->freeList) && AtomicMaskClear(&s->flags, kFlagHasBuffer))
        StreamReceiveMacTCP(s);
}

void StreamDisposeMacTCP(NetSocket *sock)
{
    TCPMacTCP *s;

    s = (TCPMacTCP *)sock;
    StreamClose(sock, 0);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventDisposed);
    if (s->cookie != 0) {
        *(ULong *)s->cookie = kCookieDead;
        MemoryDeallocate(s->cookie);
        s->cookie = 0;
    }
    if (s->buffer != 0) {
        MemoryDeallocate(s->buffer);
        s->buffer = 0;
    }
    CriticalDispose(s->lock);
    _EndpointUnregister((NetSocket *)s, 3);
    MemoryDeallocate(s);
}

int32_t StreamAcceptServerMacTCP(void *ref, void *other)
{
    TCPMacTCP *s;
    TCPMacTCP *o;
    Ptr buffer;
    TCPMacTCP **cookie;

    s = ref;
    o = other;
    if (s == o) {
        o->state = kStateConnected;
        NetworkMakeCallback(o->proc, o->ref, (NetSocket *)o, kEventAccepted);
        goto done;
    }
    if (o != 0) {
        o->stream = s->stream;
        s->stream = 0;
        buffer = o->buffer;
        o->buffer = s->buffer;
        s->buffer = buffer;
        cookie = o->cookie;
        o->cookie = s->cookie;
        s->cookie = cookie;
        *o->cookie = o;
        *s->cookie = s;
        AddressDuplicate(&o->local, &s->local);
        AddressDuplicate(&o->remote, &s->remote);
        AddressComposeEmpty(&s->remote);
        s->state = 0;
        o->state = kStateConnected;
        AtomicMaskApply(&o->flags, 0);
        AtomicMaskClear(&o->flags, kFlagHasBuffer);
        NetworkMakeCallback(o->proc, o->ref, (NetSocket *)o, kEventAccepted);
        StreamCreateStreamMacTCP(s);
    } else {
        s->state = kStateClosing;
        AddressComposeEmpty(&s->remote);
        StreamCloseAsyncMacTCP(s);
    }
done:
    return 0;
}

int32_t StreamOpenClientMacTCP(TCPMacTCP *s, NetAddress *local, const NetAddress *remote, ULong timeout)
{
    unsigned short remotePort;
    ULong remoteHost;
    TCPiopb pb;
    unsigned short localPort;
    ULong localHost;
    int32_t err;
    int32_t e;
    ULongLong deadline;

    remoteHost = 0;
    remotePort = 0;
    localHost = 0;
    localPort = 0;
    AddressToNetworkStack(remote);
    s->sendBytes = 0;
    e = QueueCreate(&s->sendQueue);
    if (e != 0) {
        err = e;
        goto done;
    }
    AtomicMaskClear(&s->flags, kFlagHasBuffer);
    AtomicMaskClear(&s->flags, kFlagPosted);
    if (local != 0)
        AddressDecomposeTCPIP(local, &localPort, &localHost);
    AddressDecomposeTCPIP(remote, &remotePort, &remoteHost);
    pb.ioCompletion = 0;
    pb.ioResult = 0;
    pb.ioCRefNum = gMacTCPRefNum;
    pb.csCode = TCPCreate;
    pb.tcpStream = 0;
    pb.csParam.create.rcvBuff = s->buffer;
    pb.csParam.create.rcvBuffLen = kTcpBuffer;
    pb.csParam.create.notifyProc = sStreamNotify;
    pb.csParam.create.userDataPtr = (Ptr)s->cookie;
    if (PBControlSync((ParmBlkPtr)&pb) != 0) {
        err = kOpenFailed2;
        goto done;
    }
    s->stream = pb.tcpStream;
    s->state = kStateConnecting;
    s->ctlPb.ioCompletion = sStreamDone;
    s->ctlPb.ioResult = 0;
    s->ctlPb.ioCRefNum = gMacTCPRefNum;
    s->ctlPb.csCode = TCPActiveOpen;
    s->ctlPb.tcpStream = s->stream;
    s->ctlPb.csParam.open.ulpTimeoutValue = 0;
    s->ctlPb.csParam.open.ulpTimeoutAction = 0;
    s->ctlPb.csParam.open.validityFlags = 0;
    s->ctlPb.csParam.open.commandTimeoutValue = 0xf;
    s->ctlPb.csParam.open.remoteHost = remoteHost;
    s->ctlPb.csParam.open.remotePort = remotePort;
    s->ctlPb.csParam.open.localHost = localHost;
    s->ctlPb.csParam.open.localPort = localPort;
    s->ctlPb.csParam.open.tosFlags = 0;
    s->ctlPb.csParam.open.precedence = 0;
    s->ctlPb.csParam.open.dontFrag = 0;
    s->ctlPb.csParam.open.timeToLive = 0;
    s->ctlPb.csParam.open.security = 0;
    s->ctlPb.csParam.open.optionCnt = 0;
    s->ctlPb.csParam.open.userDataPtr = (Ptr)s->cookie;
    err = PBControlAsync((ParmBlkPtr)&s->ctlPb);
    {
        short result;

        result = pb.ioResult;
        if (result == kConnectionExists && result != 0) {
            err = kSendBusy;
            goto done;
        }
        if (result == kConnectionClosing && result != 0) {
            err = kOpenRefused;
            goto done;
        }
        if (result != 0 && result != kInProgress) {
            err = kSendBusy;
            goto done;
        }
    }
    if (err != 0) {
        err = kSendBusy;
        goto done;
    }
    if (timeout != 0) {
        deadline = TimerGetMilliseconds() + timeout;
        while ((int32_t)s->state == kStateConnecting && (long long)(deadline - TimerGetMilliseconds()) > 0)
            SystemIdle();
        if ((int32_t)s->state == kStateConnecting) {
            StreamCloseMacTCP((NetSocket *)s, 0);
            err = kTimedOut;
        }
    }
done:
    if (err != 0)
        s->state = 0;
    return err;
}

int32_t StreamSendMacTCP(void *ref, NetChunk *chunk)
{
    TCPMacTCP *s;
    int32_t err;
    int32_t state;

    s = ref;
    err = 0;
    state = s->state;
    if (state == 0 || state == kStateDead) {
        err = kNotOpenStream;
    } else {
        AtomicIncrement(&s->sendBytes, chunk->length);
        QueueInsert(&s->sendQueue, &chunk->link);
        StreamPostMacTCP(s);
    }
    return err;
}

void StreamCloseMacTCP(NetSocket *sock, unsigned char graceful)
{
    unsigned char wait;
    unsigned char close;
    TCPMacTCP *s;
    unsigned char abort;
    int32_t err;
    unsigned char waitEnd;
    TCPiopb pb;
    volatile unsigned char finished;
    int32_t e;
    int32_t state;

    s = (TCPMacTCP *)sock;
    err = 0;
    waitEnd = 0;
    wait = 0;
    close = 0;
    abort = 0;
    finished = 0;
    SystemCheckCallback();
    state = s->state;
    switch (state) {
    case 0:
    case 1:
    case 2:
        wait = 0;
        abort = 0;
        close = 0;
        waitEnd = 0;
        break;
    case 3:
        wait = 0;
        abort = 1;
        close = 0;
        waitEnd = 0;
        break;
    case 4:
        abort = graceful == 0;
        wait = 1;
        close = 1;
        waitEnd = 1;
        break;
    case 5:
        abort = graceful == 0;
        wait = 1;
        close = 1;
        waitEnd = 1;
        break;
    case 6:
        abort = graceful == 0;
        wait = 1;
        close = 1;
        waitEnd = 0;
        break;
    case 7:
        abort = graceful == 0;
        wait = 1;
        close = 1;
        waitEnd = 0;
        break;
    case 8:
        wait = 1;
        abort = 0;
        close = 0;
        waitEnd = 0;
        break;
    }
    if (state != 0)
        s->state = kStateClosing;
    if (s->stream != 0 && wait) {
        s->state = kStateClosing;
        while (s->sending != 0 || !QueueIsEmpty(&s->sendQueue))
            SystemIdle();
    }
    if (s->stream != 0 && abort) {
        finished = 0;
        pb.ioCompletion = sStreamSync;
        pb.ioResult = 0;
        pb.ioCRefNum = gMacTCPRefNum;
        pb.csCode = TCPAbort;
        pb.tcpStream = s->stream;
        pb.csParam.abort.userDataPtr = (Ptr)&finished;
        e = PBControlAsync((ParmBlkPtr)&pb);
        if (e != 0)
            err = e;
        while (err == 0 && !finished)
            SystemIdle();
    } else if (s->stream != 0 && close) {
        finished = 0;
        pb.ioCompletion = sStreamSync;
        pb.ioResult = 0;
        pb.ioCRefNum = gMacTCPRefNum;
        pb.csCode = TCPClose;
        pb.tcpStream = s->stream;
        pb.csParam.close.ulpTimeoutValue = 0xf;
        pb.csParam.close.ulpTimeoutAction = 1;
        pb.csParam.close.validityFlags = 0xc0;
        pb.csParam.close.filler = 0;
        pb.csParam.close.userDataPtr = (Ptr)&finished;
        e = PBControlAsync((ParmBlkPtr)&pb);
        if (e != 0)
            err = e;
        while (err == 0 && !finished)
            SystemIdle();
    }
    while (s->stream != 0 && waitEnd && (int32_t)s->state != 0 && (int32_t)s->state != kStatePeerClosed && (int32_t)s->state != kStateDead)
        SystemIdle();
    if (s->stream != 0) {
        pb.ioCompletion = 0;
        pb.ioResult = 0;
        pb.ioCRefNum = gMacTCPRefNum;
        pb.csCode = TCPRelease;
        pb.tcpStream = s->stream;
        pb.csParam.create.rcvBuff = 0;
        pb.csParam.create.rcvBuffLen = 0;
        pb.csParam.create.notifyProc = 0;
        pb.csParam.create.userDataPtr = (Ptr)&finished;
        PBControlSync((ParmBlkPtr)&pb);
        s->stream = 0;
    }
    s->state = 0;
    AtomicMaskClear(&s->flags, kFlagSocket);
    AtomicMaskClear(&s->flags, kFlagHasBuffer);
    AtomicMaskClear(&s->flags, kFlagPosted);
    QueueDispose(&s->sendQueue);
    s->sendBytes = 0;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
}

int32_t StreamGetAddressMacTCP(TCPMacTCP *s, NetAddress *local, NetAddress *remote)
{
    int32_t err;
    int32_t state;

    err = 0;
    state = s->state;
    if (state == 0 || state == kStateDead)
        err = kNotOpenStream;
    if (err != 0) {
        AddressComposeEmpty(local);
        AddressComposeEmpty(remote);
    } else {
        AddressDuplicate(local, &s->local);
        AddressDuplicate(remote, &s->remote);
    }
    return err;
}

void StreamCreateStreamMacTCP(TCPMacTCP *s)
{
    s->ctlPb.ioCompletion = sStreamDone;
    s->ctlPb.ioResult = 0;
    s->ctlPb.ioCRefNum = gMacTCPRefNum;
    s->ctlPb.csCode = TCPCreate;
    s->ctlPb.tcpStream = 0;
    s->ctlPb.csParam.create.rcvBuff = s->buffer;
    s->ctlPb.csParam.create.rcvBuffLen = kTcpBuffer;
    s->ctlPb.csParam.create.notifyProc = sStreamNotify;
    s->ctlPb.csParam.create.userDataPtr = (Ptr)s->cookie;
    PBControlAsync((ParmBlkPtr)&s->ctlPb);
}

static void StreamListenMacTCP(TCPMacTCP *s)
{
    int32_t err;
    int32_t e;

    err = 0;
    s->state = kStateListening;
    AtomicMaskApply(&s->flags, 0);
    s->ctlPb.ioCompletion = sStreamDone;
    s->ctlPb.ioResult = 0;
    s->ctlPb.ioCRefNum = gMacTCPRefNum;
    s->ctlPb.csCode = TCPPassiveOpen;
    s->ctlPb.tcpStream = s->stream;
    s->ctlPb.csParam.open.ulpTimeoutValue = 0;
    s->ctlPb.csParam.open.ulpTimeoutAction = 0;
    s->ctlPb.csParam.open.validityFlags = 0;
    s->ctlPb.csParam.open.commandTimeoutValue = 0;
    s->ctlPb.csParam.open.remoteHost = 0;
    s->ctlPb.csParam.open.remotePort = 0;
    s->ctlPb.csParam.open.localHost = s->listenHost;
    s->ctlPb.csParam.open.localPort = s->listenPort;
    s->ctlPb.csParam.open.tosFlags = 0;
    s->ctlPb.csParam.open.precedence = 0;
    s->ctlPb.csParam.open.dontFrag = 0;
    s->ctlPb.csParam.open.timeToLive = 0;
    s->ctlPb.csParam.open.security = 0;
    s->ctlPb.csParam.open.optionCnt = 0;
    s->ctlPb.csParam.open.userDataPtr = (Ptr)s->cookie;
    e = PBControlAsync((ParmBlkPtr)&s->ctlPb);
    if (e != 0)
        err = e;
    if (err != 0)
        AtomicMaskClear(&s->flags, 0);
    if (err != 0)
        s->state = 0;
}

void StreamPostMacTCP(TCPMacTCP *s)
{
    char token[16];
    unsigned char post;
    int32_t e;

    post = 0;
    AtomicEnterCritical(token);
    if (s->sending == 0 && (s->sending = (NetChunk *)NetworkQueuePoll(&s->sendQueue)) != 0)
        post = 1;
    AtomicLeaveCritical(token);
    if (post) {
        s->wds[0].length = s->sending->length;
        s->wds[0].ptr = s->sending->data;
        s->wds[1].length = 0;
        s->sendPb.ioCompletion = sStreamDone;
        s->sendPb.ioResult = 0;
        s->sendPb.ioCRefNum = gMacTCPRefNum;
        s->sendPb.csCode = TCPSend;
        s->sendPb.tcpStream = s->stream;
        s->sendPb.csParam.send.ulpTimeoutValue = 0;
        s->sendPb.csParam.send.ulpTimeoutAction = 0;
        s->sendPb.csParam.send.validityFlags = 0;
        s->sendPb.csParam.send.pushFlag = 0;
        s->sendPb.csParam.send.urgentFlag = 0;
        s->sendPb.csParam.send.filler = 0;
        s->sendPb.csParam.send.wdsPtr = (Ptr)s->wds;
        s->sendPb.csParam.send.sendFree = 0;
        s->sendPb.csParam.send.sendLength = 0;
        s->sendPb.csParam.send.userDataPtr = (Ptr)s->cookie;
        e = PBControlAsync((ParmBlkPtr)&s->sendPb);
        if (e != 0 && e == kConnectionClosing) {
            s->state = 0;
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventFailed2);
        }
    }
}

void StreamReceiveMacTCP(TCPMacTCP *s)
{
    int32_t err;
    NetChunk *chunk;

    err = 0;
    if (AtomicMaskTest(&s->flags, kFlagPosted) && !AtomicMaskApply(&s->flags, kFlagUdpBusy)) {
        chunk = (NetChunk *)NetworkQueuePoll(s->freeList);
        if (chunk == 0 && !AtomicMaskApply(&s->flags, kFlagHasBuffer)) {
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventNeedBuffers);
            chunk = (NetChunk *)NetworkQueuePoll(s->freeList);
        }
        s->receiving = chunk;
        if (chunk != 0) {
            AtomicMaskClear(&s->flags, kFlagPosted);
            s->receiving->length = 0;
            s->ctlPb.ioCompletion = sStreamDone;
            s->ctlPb.ioResult = 0;
            s->ctlPb.ioCRefNum = gMacTCPRefNum;
            s->ctlPb.csCode = TCPRcv;
            s->ctlPb.tcpStream = s->stream;
            s->ctlPb.csParam.receive.commandTimeoutValue = 0;
            s->ctlPb.csParam.receive.markFlag = 0;
            s->ctlPb.csParam.receive.urgentFlag = 0;
            s->ctlPb.csParam.receive.filler = 0;
            s->ctlPb.csParam.receive.rcvBuff = s->receiving->data;
            s->ctlPb.csParam.receive.rcvBuffLen = s->receiving->size;
            s->ctlPb.csParam.receive.rdsPtr = 0;
            s->ctlPb.csParam.receive.secondTimeStamp = 0;
            s->ctlPb.csParam.receive.userDataPtr = (Ptr)s->cookie;
            err = PBControlAsync((ParmBlkPtr)&s->ctlPb);
            if (err != 0 && err == kConnectionClosing) {
                s->state = 0;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventFailed2);
            }
        }
        if (chunk == 0 || err != 0) {
            AtomicMaskClear(&s->flags, kFlagUdpBusy);
            StreamReceiveMacTCP(s);
        }
    }
}

void StreamCloseAsyncMacTCP(TCPMacTCP *s)
{
    s->ctlPb.ioCompletion = sStreamDone;
    s->ctlPb.ioResult = 0;
    s->ctlPb.ioCRefNum = gMacTCPRefNum;
    s->ctlPb.csCode = TCPAbort;
    s->ctlPb.tcpStream = s->stream;
    s->ctlPb.csParam.abort.userDataPtr = (Ptr)s->cookie;
    PBControlAsync((ParmBlkPtr)&s->ctlPb);
}

void StreamAbortAsyncMacTCP(TCPMacTCP *s)
{
    s->ctlPb.ioCompletion = sStreamDone;
    s->ctlPb.ioResult = 0;
    s->ctlPb.ioCRefNum = gMacTCPRefNum;
    s->ctlPb.csCode = TCPClose;
    s->ctlPb.tcpStream = 0;
    s->ctlPb.csParam.close.ulpTimeoutValue = 0;
    s->ctlPb.csParam.close.ulpTimeoutAction = 0;
    s->ctlPb.csParam.close.validityFlags = 0;
    s->ctlPb.csParam.close.filler = 0;
    s->ctlPb.csParam.close.userDataPtr = (Ptr)s->cookie;
    PBControlAsync((ParmBlkPtr)&s->ctlPb);
    s->state = 0;
}

static void StreamDoneMacTCP(TCPiopb *pb)
{
    TCPMacTCP *s;

    switch (pb->csCode) {
    case TCPCreate:
        s = *(TCPMacTCP **)pb->csParam.create.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            s->stream = pb->tcpStream;
            StreamListenMacTCP(s);
            SystemLeaveCallback(1);
        }
        break;
    case TCPPassiveOpen:
        s = *(TCPMacTCP **)pb->csParam.open.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            AddressComposeTCPIP(&s->local, s->ctlPb.csParam.open.localPort, s->ctlPb.csParam.open.localHost);
            AddressComposeTCPIP(&s->remote, s->ctlPb.csParam.open.remotePort, s->ctlPb.csParam.open.remoteHost);
            s->state = kStateAccepting;
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventListening);
            SystemLeaveCallback(1);
        }
        break;
    case TCPActiveOpen:
        s = *(TCPMacTCP **)pb->csParam.open.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            AtomicMaskApply(&s->flags, 0);
            AddressComposeTCPIP(&s->local, s->ctlPb.csParam.open.localPort, s->ctlPb.csParam.open.localHost);
            AddressComposeTCPIP(&s->remote, s->ctlPb.csParam.open.remotePort, s->ctlPb.csParam.open.remoteHost);
            s->state = kStateConnected;
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAccepted);
            SystemLeaveCallback(1);
        }
        break;
    case TCPSend:
        s = *(TCPMacTCP **)pb->csParam.send.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            AtomicIncrement(&s->sendBytes, -s->sending->length);
            QueueInsert(s->sentList, &s->sending->link);
            s->sending = 0;
            StreamPostMacTCP(s);
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
            SystemLeaveCallback(1);
        }
        break;
    case TCPRcv:
        s = *(TCPMacTCP **)pb->csParam.receive.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            s->receiving->length = s->ctlPb.csParam.receive.rcvBuffLen;
            QueueInsert(s->recvList, &s->receiving->link);
            s->receiving = 0;
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventReceived);
            AtomicMaskClear(&s->flags, kFlagUdpBusy);
            StreamReceiveMacTCP(s);
            SystemLeaveCallback(1);
        }
        break;
    case TCPAbort:
        s = *(TCPMacTCP **)pb->csParam.abort.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            StreamAbortAsyncMacTCP(s);
            SystemLeaveCallback(1);
        }
        break;
    case TCPClose:
        s = *(TCPMacTCP **)pb->csParam.close.userDataPtr;
        if (pb->ioResult == 0) {
            SystemEnterCallback(1);
            StreamListenMacTCP(s);
            SystemLeaveCallback(1);
        }
        break;
    }
}

static void StreamSyncDoneMacTCP(TCPiopb *pb)
{
    switch (pb->csCode) {
    case TCPAbort:
        *pb->csParam.abort.userDataPtr = 1;
        break;
    case TCPClose:
        *pb->csParam.close.userDataPtr = 1;
        break;
    }
}

static pascal void StreamNotifyMacTCP(StreamPtr stream, unsigned short event, Ptr userData, unsigned short terminReason, struct ICMPReport *report)
{
    TCPMacTCP *s;
    unsigned char quiet;
    int32_t state;

    quiet = 0;
    s = *(TCPMacTCP **)userData;
    SystemEnterCallback(1);
    switch (event) {
    case TCPDataArrival:
        AtomicMaskApply(&s->flags, kFlagPosted);
        StreamReceiveMacTCP(s);
        break;
    case TCPClosing:
        state = s->state;
        if (state == kStateConnected) {
            s->state = kStatePeerClosed;
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventPeerClosed2);
        } else if (state == kStateClosing) {
            s->state = 0;
        }
        break;
    case TCPTerminate:
        state = s->state;
        if (state == kStateConnecting || state == 0)
            quiet = 1;
        switch (terminReason) {
        case 2:
            if (!quiet) {
                s->state = kStateDead;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventRemoteClosed);
            }
            break;
        case 3:
            if (!quiet) {
                s->state = kStateDead;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventRemoteClosed);
            }
            break;
        case 5:
            if (!quiet)
                s->state = 0;
            break;
        case 6:
            if (!quiet)
                s->state = 0;
            break;
        case 7:
            if (!quiet)
                s->state = 0;
            break;
        default:
            if (!quiet) {
                s->state = 0;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventFailed2);
            }
            break;
        }
        break;
    }
    SystemLeaveCallback(1);
}

#endif
