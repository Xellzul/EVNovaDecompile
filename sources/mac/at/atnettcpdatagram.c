/* atnettcpdatagram.c -- 1.0.10: TCP/IP without Open Transport: UDP on the MacTCP driver. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <MacTCP.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON

/* a socket on MacTCP's UDP: the stream's own buffer, one write and one read in flight */
#define kEventNeedBuffers   14
#define kUdpBytes           0x1e8
#define kUdpBuffer          0x4000
#define kFlagSocket         0
#define kFlagPosted         2
#define kFlagUdpBusy        6
#define kFlagHasBuffer      4
#define kFlagResetOption    16
#define kStateClosing       7
#define kNoMoreStreams      -23017
#define kPortInUse          -7103
#define kOptionReportReset  0x6000
#define kOptionNoAddress    0x6001
#define kOptionTtl          0x16000
#define kUnsupported        -7003
#define kEventSent          10
#define kEventReceived      12
#define kNoNode             -7100
#define kOpenFailed2        -7102

typedef struct UDPMacTCP {
    struct UDPMacTCP *next;
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
    long unk13c;
    StreamPtr stream;
    Ptr buffer;
    UDPiopb sendPb;
    UDPiopb recvPb;
    wdsEntry wds[2];
    ULong mask;
    MutexList sendQueue;
    NetPacket *sending;
    NetPacket *receiving;
    long sendBytes;
} UDPMacTCP;

void DatagramPostMacTCP(UDPMacTCP *s);
void DatagramReceiveMacTCP(UDPMacTCP *s);
static inline void MoveBytes(void *dst, const void *src, ULong n)
{
    if (n != 0)
        BlockMoveData(src, dst, n);
}

static void DatagramDoneMacTCP(UDPiopb *pb);
static pascal void DatagramNotifyMacTCP(StreamPtr stream, unsigned short event, Ptr userData, struct ICMPReport *report);
static RoutineDescriptor sDatagramDoneDesc = BUILD_ROUTINE_DESCRIPTOR(kCStackBased | STACK_ROUTINE_PARAMETER(1, kFourByteCode), DatagramDoneMacTCP);
static UniversalProcPtr sDatagramDone = &sDatagramDoneDesc;
static RoutineDescriptor sDatagramNotifyDesc = BUILD_ROUTINE_DESCRIPTOR(kPascalStackBased | STACK_ROUTINE_PARAMETER(1, kFourByteCode) | STACK_ROUTINE_PARAMETER(2, kTwoByteCode) | STACK_ROUTINE_PARAMETER(3, kFourByteCode) | STACK_ROUTINE_PARAMETER(4, kFourByteCode), DatagramNotifyMacTCP);
static UniversalProcPtr sDatagramNotify = &sDatagramNotifyDesc;

NetSocket *DatagramCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    UDPMacTCP *s;

    s = MemoryAllocate(kUdpBytes);
    if (s == 0)
        goto done;
    MemoryClear(s, kUdpBytes);
    s->kind = 2;
    s->stack = 0;
    s->state = 0;
    s->proc = proc;
    s->ref = ref;
    s->flags = 0;
    s->freeList = freeList;
    s->sentList = sentList;
    s->recvList = recvList;
    s->recvList2 = recvList2;
    s->unk13c = 0;
    if (CriticalCreate(s->lock) != 0)
        goto done;
    _EndpointRegister((NetSocket *)s, 2);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventCreated);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
done:
    return (NetSocket *)s;
}

void DatagramTickleMacTCP(NetSocket *ref)
{
    UDPMacTCP *s;
    long state;

    s = (UDPMacTCP *)ref;
    state = s->state;
    if (state != 0)
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnected);
    if (!QueueIsEmpty(s->freeList) && AtomicMaskClear(&s->flags, kFlagHasBuffer))
        DatagramReceiveMacTCP(s);
}

void DatagramDisposeMacTCP(UDPMacTCP *s)
{
    DatagramClose(s, 0);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventDisposed);
    CriticalDispose(s->lock);
    _EndpointUnregister((NetSocket *)s, 2);
    MemoryDeallocate(s);
}

long DatagramControlMacTCP(UDPMacTCP *s, void *out, ULong option, ULong value)
{
    long err;

    err = 0;
    switch (option) {
    case kOptionReportReset:
        if ((value & 1) != 0)
            AtomicMaskApply(&s->flags, kFlagResetOption);
        else
            AtomicMaskClear(&s->flags, kFlagResetOption);
        break;
    case kOptionNoAddress:
        AddressComposeTCPIP(out, 0, s->mask);
        break;
    case kOptionTtl:
        err = kUnsupported;
        break;
    default:
        err = kUnsupported;
        break;
    }
    return err;
}

long DatagramOpenMacTCP(void *ref, NetAddress *address)
{
    UDPMacTCP *s;
    long err, e;
    unsigned short port;
    ULong ip;
    GetAddrParamBlock addr;
    UDPiopb pb;

    s = ref;
    ip = 0;
    port = 0;
    s->sendBytes = 0;
    e = QueueCreate(&s->sendQueue);
    if (e != 0) {
        err = e;
        goto done;
    }
    AtomicMaskClear(&s->flags, kFlagHasBuffer);
    AtomicMaskClear(&s->flags, kFlagPosted);
    if (address != 0)
        AddressDecomposeTCPIP(address, &port, &ip);
    if ((s->buffer = MemoryAllocate(kUdpBuffer)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    pb.ioCompletion = 0;
    pb.ioResult = 0;
    pb.ioCRefNum = gMacTCPRefNum;
    pb.csCode = UDPCreate;
    pb.udpStream = 0;
    pb.csParam.create.rcvBuff = s->buffer;
    pb.csParam.create.rcvBuffLen = kUdpBuffer;
    pb.csParam.create.notifyProc = sDatagramNotify;
    pb.csParam.create.localPort = port;
    pb.csParam.create.userDataPtr = (Ptr)s;
    err = PBControlSync((ParmBlkPtr)&pb);
    if (err == kNoMoreStreams && err != 0) {
        err = kPortInUse;
        goto done;
    }
    if (err != 0) {
        err = kOpenFailed2;
        goto done;
    }
    AtomicMaskApply(&s->flags, kFlagSocket);
    s->stream = pb.udpStream;
    s->state = kStateOpen;
    port = pb.csParam.create.localPort;
    if (ip != 0) {
        addr.ioCompletion = 0;
        addr.ioResult = 0;
        addr.ioCRefNum = gMacTCPRefNum;
        addr.csCode = ipctlGetAddr;
        addr.ourAddress = 0;
        addr.ourNetMask = 0;
        if (PBControlSync((ParmBlkPtr)&addr) != 0) {
            err = kNoNode;
            goto done;
        }
    } else {
        addr.ourAddress = 0;
    }
    AddressComposeTCPIP(&s->local, pb.csParam.create.localPort, addr.ourAddress);
    s->mask = addr.ourNetMask;
    if (address != 0)
        AddressDuplicate(address, &s->local);
done:
    if (err != 0)
        DatagramClose(ref, 0);
    return err;
}

long DatagramSendMacTCP(void *ref, NetPacket *packet)
{
    UDPMacTCP *s;

    s = ref;
    AddressToNetworkStack(&packet->from);
    QueueInsert(&s->sendQueue, &packet->link);
    AtomicIncrement(&s->sendBytes, packet->length);
    DatagramPostMacTCP(s);
    return 0;
}

void DatagramCloseMacTCP(UDPMacTCP *s, long graceful)
{
    UDPiopb pb;

    SystemCheckCallback();
    if ((unsigned char)graceful != 0 && (long)s->state != 0 && s->stream != 0) {
        s->state = kStateClosing;
        while (s->sending != 0 || !QueueIsEmpty(&s->sendQueue))
            SystemIdle();
    }
    if (s->stream != 0) {
        pb.ioCompletion = 0;
        pb.ioResult = 0;
        pb.ioCRefNum = gMacTCPRefNum;
        pb.csCode = UDPRelease;
        pb.udpStream = s->stream;
        pb.csParam.create.rcvBuff = 0;
        pb.csParam.create.rcvBuffLen = 0;
        pb.csParam.create.notifyProc = 0;
        pb.csParam.create.userDataPtr = 0;
        PBControlSync((ParmBlkPtr)&pb);
        s->stream = 0;
    }
    if (s->buffer != 0) {
        DisposePtr(s->buffer);
        s->buffer = 0;
    }
    s->mask = 0;
    s->state = 0;
    AtomicMaskClear(&s->flags, kFlagHasBuffer);
    AtomicMaskClear(&s->flags, kFlagPosted);
    AtomicMaskClear(&s->flags, kFlagSocket);
    QueueTransfer(s->freeList, &s->sendQueue);
    QueueDispose(&s->sendQueue);
    s->sendBytes = 0;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
}

long DatagramGetAddressMacTCP(void *ref, NetAddress *out)
{
    AddressDuplicate(out, &((UDPMacTCP *)ref)->local);
    return 0;
}


void DatagramPostMacTCP(UDPMacTCP *s)
{
    char token[16];
    unsigned short port;
    ULong ip;
    unsigned char post;

    post = 0;
    AtomicEnterCritical(token);
    if (s->sending == 0 && (s->sending = (NetPacket *)NetworkQueuePoll(&s->sendQueue)) != 0)
        post = 1;
    AtomicLeaveCritical(token);
    if (post) {
        AddressDecomposeTCPIP(&s->sending->from, &port, &ip);
        s->wds[0].length = s->sending->length;
        s->wds[0].ptr = s->sending->data;
        s->wds[1].length = 0;
        s->sendPb.ioCompletion = sDatagramDone;
        s->sendPb.ioResult = 0;
        s->sendPb.ioCRefNum = gMacTCPRefNum;
        s->sendPb.csCode = UDPWrite;
        s->sendPb.udpStream = s->stream;
        s->sendPb.csParam.send.reserved = 0;
        s->sendPb.csParam.send.remoteHost = ip;
        s->sendPb.csParam.send.remotePort = port;
        s->sendPb.csParam.send.wdsPtr = (Ptr)s->wds;
        s->sendPb.csParam.send.checkSum = 1;
        s->sendPb.csParam.send.filler = 0;
        s->sendPb.csParam.send.sendLength = 0;
        s->sendPb.csParam.send.userDataPtr = (Ptr)s;
        s->sendPb.csParam.send.localPort = 0;
        PBControlAsync((ParmBlkPtr)&s->sendPb);
    }
}

void DatagramReceiveMacTCP(UDPMacTCP *s)
{
    long err;
    NetPacket *packet;
    long e;

    err = 0;
    if (AtomicMaskTest(&s->flags, kFlagPosted) && !AtomicMaskApply(&s->flags, kFlagUdpBusy)) {
        packet = (NetPacket *)NetworkQueuePoll(s->freeList);
        if (packet == 0 && !AtomicMaskApply(&s->flags, kFlagHasBuffer)) {
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventNeedBuffers);
            packet = (NetPacket *)NetworkQueuePoll(s->freeList);
        }
        s->receiving = packet;
        if (packet != 0) {
            AtomicMaskClear(&s->flags, kFlagPosted);
            s->recvPb.ioCompletion = sDatagramDone;
            s->recvPb.ioResult = 0;
            s->recvPb.ioCRefNum = gMacTCPRefNum;
            s->recvPb.csCode = UDPRead;
            s->recvPb.udpStream = s->stream;
            s->recvPb.csParam.receive.timeOut = 0;
            s->recvPb.csParam.receive.remoteHost = 0;
            s->recvPb.csParam.receive.remotePort = 0;
            s->recvPb.csParam.receive.rcvBuff = 0;
            s->recvPb.csParam.receive.rcvBuffLen = 0;
            s->recvPb.csParam.receive.secondTimeStamp = 0;
            s->recvPb.csParam.receive.userDataPtr = (Ptr)s;
            s->recvPb.csParam.receive.destHost = 0;
            s->recvPb.csParam.receive.destPort = 0;
            e = PBControlAsync((ParmBlkPtr)&s->recvPb);
            if (e != 0)
                err = e;
        }
        if (packet == 0 || err != 0) {
            AtomicMaskClear(&s->flags, kFlagUdpBusy);
            DatagramReceiveMacTCP(s);
        }
    }
}

static void DatagramDoneMacTCP(UDPiopb *pb)
{
    UDPMacTCP *s;

    switch (pb->csCode) {
    case UDPWrite:
        s = (UDPMacTCP *)pb->csParam.send.userDataPtr;
        SystemEnterCallback(1);
        AtomicIncrement(&s->sendBytes, -s->sending->length);
        QueueInsert(s->sentList, &s->sending->link);
        s->sending = 0;
        DatagramPostMacTCP(s);
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
        SystemLeaveCallback(1);
        break;
    case UDPRead:
        s = (UDPMacTCP *)pb->csParam.receive.userDataPtr;
        SystemEnterCallback(1);
        if ((s->receiving->length = s->recvPb.csParam.receive.rcvBuffLen) != 0) {
            if (s->receiving->length > s->receiving->capacity)
                s->receiving->length = s->receiving->capacity;
            MoveBytes(s->receiving->data, s->recvPb.csParam.receive.rcvBuff, s->receiving->length);
        }
        AddressComposeTCPIP(&s->receiving->from, s->recvPb.csParam.receive.remotePort, s->recvPb.csParam.receive.remoteHost);
        QueueInsert(s->recvList, &s->receiving->link);
        s->receiving = 0;
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventReceived);
        s->recvPb.csCode = UDPBfrReturn;
        PBControlAsync((ParmBlkPtr)&s->recvPb);
        SystemLeaveCallback(1);
        break;
    case UDPBfrReturn:
        s = (UDPMacTCP *)pb->csParam.receive.userDataPtr;
        SystemEnterCallback(1);
        AtomicMaskClear(&s->flags, kFlagUdpBusy);
        DatagramReceiveMacTCP(s);
        SystemLeaveCallback(1);
        break;
    }
}

static pascal void DatagramNotifyMacTCP(StreamPtr stream, unsigned short event, Ptr userData, struct ICMPReport *report)
{
    UDPMacTCP *s;

    s = (UDPMacTCP *)userData;
    SystemEnterCallback(1);
    switch (event) {
    case UDPDataArrival:
        AtomicMaskApply(&s->flags, kFlagPosted);
        DatagramReceiveMacTCP(s);
        break;
    }
    SystemLeaveCallback(1);
}


#endif
