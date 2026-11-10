/* atnetatalk.c -- 1.0.10: AppleTalk without Open Transport, the Preferred Pascal calls: DDP, the ADSP stubs, the 68K listener. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <AppleTalk.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON

/* a socket on DDP: the packet being written, a 68K listener's packet handed to a deferred task */
#define kEventNeedBuffers   14
#define kDatagramBytes      0x37c
#define kFlagHasName        1
#define kFlagHasSocket      0
#define kFlagPending        16
#define kFlagNoBuffer       4
#define kFlagUnique         18
#define kFlagHoldOff        17
#define kStateClosing       7
#define kEventSent          10
#define kEventReceived      12
#define kOptionReportReset  0x6000
#define kOptionNoAddress    0x6001
#define kOptionName         0x22000
#define kUnsupported        -7003
#define kBadName            -7001
#define kNoSocket2          -7103
#define kOpenFailed         -7102
#define kNoNode             -7100
#define kNameFailed         -7104
#define kNameTaken          -1027
#define kNameRetries        4
#define kMaxObject          0x21
#define kMenuDivider        0xc5
#define kListenerBytes      0x200
#define kDDPHeaderBytes     0x11
#define dtQType             7
#define kNetworkNameResource -16413
#define kFirstDynamicSocket 0x80
#define kBadSocket          -7103

typedef struct DDPSocket {
    struct DDPSocket *next;
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
    struct DDPSocket *self;
    MPPParamBlock pb;
    char pad192;
    unsigned char header[kDDPHeaderBytes];
    WDSElement wds[3];
    short pad1b6;
    Ptr listener;
    unsigned char socket;
    char pad1bd[3];
    MutexList sendQueue;
    MutexList recvQueue;
    NetPacket *sending;
    NetPacket *receiving;
    unsigned char rec[12];
    char *recvData;
    DeferredTask task;
    long sendBytes;
    unsigned char nte[0x6c];
    char name[0x100];
} DDPSocket;

void DatagramPostDDP(DDPSocket *s);
static void DatagramSentDDP(MPPPBPtr pb);
static void DatagramDeferredDDP(DDPSocket *s);
extern Ptr ListenerCreate(void *upp, void *socket);
extern void ListenerDispose(Ptr listener);
Ptr DatagramListenerDDP(Ptr rec, DDPSocket *s);
static RoutineDescriptor sDatagramSentDesc = BUILD_ROUTINE_DESCRIPTOR(uppMPPCompletionProcInfo, DatagramSentDDP);
static MPPCompletionUPP sDatagramSent = &sDatagramSentDesc;
static RoutineDescriptor sDatagramListenerDesc = BUILD_ROUTINE_DESCRIPTOR(kPascalStackBased | RESULT_SIZE(kFourByteCode) | STACK_ROUTINE_PARAMETER(1, kFourByteCode) | STACK_ROUTINE_PARAMETER(2, kFourByteCode), DatagramListenerDDP);
static UniversalProcPtr sDatagramListener = &sDatagramListenerDesc;
static RoutineDescriptor sDatagramDeferredDesc = BUILD_ROUTINE_DESCRIPTOR(uppDeferredTaskProcInfo, DatagramDeferredDDP);
static DeferredTaskUPP sDatagramDeferred = &sDatagramDeferredDesc;


/* the DDP socket listener, 68K: copied to the system heap, the UPP and the socket patched in at +2 and +6, the VM flag at +10 */
static unsigned short sListenerCode[] = {
    0x600c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x48e7, 0x40f8, 0x41fa, 0xfff6, 0x2010,
    0x4a80, 0x6708, 0x7005, 0xa08d, 0x4a00, 0x6620, 0x594f, 0x4878, 0x0000, 0x41fa, 0xffda, 0x2050,
    0x2f08, 0x41fa, 0xffce, 0x2050, 0x4e90, 0x201f, 0x4cdf, 0x1f02, 0x2400, 0x660a, 0x4283, 0x4eac,
    0x0002, 0x6000, 0x0094, 0x3601, 0x2642, 0x47eb, 0x000c, 0x2653, 0x4eac, 0x0002, 0x6600, 0x0082,
    0x2242, 0x4269, 0x0000, 0x4269, 0x0001, 0x4269, 0x0008, 0x7400, 0x302a, 0x0004, 0x0240, 0x3c00,
    0xe448, 0xe048, 0x3340, 0x0006, 0x322a, 0x0004, 0x0241, 0x03ff, 0x102a, 0x0003, 0x0c00, 0x0001,
    0x671c, 0x136a, 0x0010, 0x0001, 0x136a, 0x000c, 0x0000, 0x242a, 0x000a, 0xe14a, 0x142a, 0x000f,
    0x927c, 0x000d, 0x6014, 0x136a, 0x0008, 0x0001, 0x136a, 0x0006, 0x0000, 0x142a, 0x0007, 0xe14a,
    0x5b41, 0x2342, 0x0002, 0x3341, 0x000a, 0x594f, 0x2f09, 0x41fa, 0xff36, 0x2050, 0x2f08, 0x41fa,
    0xff2a, 0x2050, 0x4e90, 0x584f, 0x4e75, 0x0000,
};
static Ptr sListener = (Ptr)sListenerCode;


/* MSL's strcpy and strlen, which the exe has inline */
static inline char *MslCopy(char *dst, const char *src)
{
    unsigned char *q = (unsigned char *)dst - 1;
    const unsigned char *p = (const unsigned char *)src - 1;

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

NetSocket *DatagramCreateDDP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    DDPSocket *s;

    IsMPPOpen();
    s = MemoryAllocate(kDatagramBytes);
    if (s == 0)
        goto done;
    MemoryClear(s, kDatagramBytes);
    s->kind = 2;
    s->stack = 1;
    s->state = 0;
    s->proc = proc;
    s->ref = ref;
    s->flags = 0;
    s->freeList = freeList;
    s->sentList = sentList;
    s->recvList = recvList;
    s->recvList2 = recvList2;
    s->unk13c = 0;
    s->self = s;
    s->task.qType = dtQType;
    s->task.dtAddr = sDatagramDeferred;
    s->task.dtParam = (long)s;
    if (CriticalCreate(s->lock) != 0)
        goto done;
    _EndpointRegister((NetSocket *)s, 2);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventCreated);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
done:
    return (NetSocket *)s;
}

void DatagramTickleDDP(NetSocket *ref)
{
    DDPSocket *s;
    long state;

    s = (DDPSocket *)ref;
    IsMPPOpen();
    state = s->state;
    if (state != 0)
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnected);
}

void DatagramDisposeDDP(DDPSocket *s)
{
    DatagramClose(s, 0);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventDisposed);
    CriticalDispose(s->lock);
    _EndpointUnregister((NetSocket *)s, 2);
    MemoryDeallocate(s);
}

long DatagramControlDDP(DDPSocket *s, void *out, ULong option, ULong value)
{
    long err;
    Str255 entity;
    unsigned char *object;
    unsigned char *type;
    const char *text;
    Handle h;
    long n;
    unsigned char *base;

    err = 0;
    switch (option) {
    case kOptionName:
        s->name[0] = 0;
        text = out;
        if (value == 0)
            break;
        if (text[0] == '=' && text[1] == ':') {
            AtomicMaskApply(&s->flags, kFlagUnique);
            h = (Handle)GetString(kNetworkNameResource);
            if (h == 0) {
                ResError();
                err = kOutOfMemory;
                break;
            }
            if ((n = (unsigned char)**h + 1) != 0)
                BlockMoveData(*h, entity, n);
            text++;
            object = entity + (unsigned char)**h + 1;
            ReleaseResource(h);
        } else {
            AtomicMaskClear(&s->flags, kFlagUnique);
            base = entity;
            object = base;
            object++;
            /* BUG: kMenuDivider is over 127, so a signed char never equals it */
            while ((n = object - base) <= kMaxObject && *text != 0 && *text != ':' && *text != '=' && *text != kMenuDivider && *text != '*')
                *object++ = *text++;
            if (n <= 1 || n > kMaxObject || *text != ':') {
                err = kBadName;
                break;
            }
            *base = (char *)object - (char *)base - 1;
        }
        type = object + 1;
        text++;
        while ((n = type - object) <= kMaxObject && *text != 0 && *text != '@' && *text != '=' && *text != kMenuDivider && *text != '*')
            *type++ = *text++;
        if (n <= 1 || n > kMaxObject || (*text != 0 && *text != '@')) {
            err = kBadName;
            break;
        }
        object[0] = (char *)type - (char *)object - 1;
        type[0] = 1;
        type[1] = '*';
        type[2] = 0;
        MslCopy(s->name, (char *)entity);
        break;
    case kOptionReportReset:
        if ((value & 1) != 0)
            AtomicMaskApply(&s->flags, kFlagHoldOff);
        else
            AtomicMaskClear(&s->flags, kFlagHoldOff);
        break;
    case kOptionNoAddress:
        AddressComposeAppleTalk(out, 0, 0xff, 0, 0);
        break;
    default:
        err = kUnsupported;
        break;
    }
    return err;
}

long DatagramOpenDDP(void *ref, NetAddress *address)
{
    DDPSocket *s;
    short net;
    short node;
    long err, e;
    Str255 unique;
    ULong tries;
    ULong n;

    s = ref;
    err = 0;
    IsMPPOpen();
    s->sendBytes = 0;
    e = QueueCreate(&s->sendQueue);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = QueueCreate(&s->recvQueue);
    if (e != 0) {
        err = e;
        goto done;
    }
    AtomicMaskClear(&s->flags, kFlagNoBuffer);
    if (address != 0) {
        AddressDecomposeAppleTalk(address, 0, 0, &s->socket, 0);
        if (s->socket >= kFirstDynamicSocket) {
            err = kBadSocket;
            goto done;
        }
    } else {
        s->socket = 0;
    }
    if ((s->listener = ListenerCreate(sDatagramListener, s)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(&s->pb, sizeof(s->pb));
    s->pb.DDP.socket = s->socket;
    s->pb.DDP.u.listener = (DDPSocketListenerUPP)s->listener;
    s->socket = 0;
    if (POpenSkt(&s->pb, false) != 0) {
        err = kOpenFailed;
        goto done;
    }
    s->socket = s->pb.DDP.socket;
    s->state = 4;
    if (GetNodeAddress(&node, &net) != 0) {
        err = kNoNode;
        goto done;
    }
    AtomicMaskApply(&s->flags, kFlagHasSocket);
    AddressComposeAppleTalk(&s->local, net, node, s->socket, 0);
    if (s->name[0] != 0) {
        for (tries = 0; tries < kNameRetries; tries++) {
            MakeUniqueNBPName(s->name, unique, tries);
            *(short *)&s->nte[4] = net;
            s->nte[6] = node;
            s->nte[7] = s->socket;
            s->nte[8] = 0;
            n = MslLength((char *)unique);
            if (n != 0)
                BlockMoveData(unique, &s->nte[9], n);
            s->pb.NBP.interval = 5;
            s->pb.NBP.count = 5;
            s->pb.NBP.nbpPtrs.ntQElPtr = (Ptr)s->nte;
            s->pb.NBP.parm.verifyFlag = 1;
            err = PRegisterName(&s->pb, false);
            if (err != kNameTaken)
                break;
        }
    }
    if (err != 0) {
        err = kNameFailed;
        goto done;
    }
    AtomicMaskApply(&s->flags, kFlagHasName);
    if (address != 0)
        AddressDuplicate(address, &s->local);
done:
    if (err != 0)
        DatagramClose(ref, 0);
    return err;
}

long DatagramSendDDP(void *ref, NetPacket *packet)
{
    DDPSocket *s;

    s = ref;
    IsMPPOpen();
    QueueInsert(&s->sendQueue, &packet->link);
    AtomicIncrement(&s->sendBytes, packet->length);
    DatagramPostDDP(s);
    return 0;
}

void DatagramCloseDDP(DDPSocket *s, long graceful)
{
    MPPParamBlock pb;

    IsMPPOpen();
    SystemCheckCallback();
    if ((unsigned char)graceful != 0 && (long)s->state != 0 && s->socket != 0) {
        s->state = kStateClosing;
        while (s->sending != 0 || !QueueIsEmpty(&s->sendQueue))
            DatagramTickle((NetSocket *)s);
    }
    if ((char)s->nte[9] != 0 && AtomicMaskClear(&s->flags, kFlagHasName)) {
        pb.NBP.nbpPtrs.entityPtr = (Ptr)&s->nte[9];
        PRemoveName(&pb, false);
        s->nte[9] = 0;
    }
    if (s->socket != 0 && AtomicMaskClear(&s->flags, kFlagHasSocket)) {
        pb.DDP.socket = s->socket;
        PCloseSkt(&pb, false);
        s->socket = 0;
    }
    if (s->listener != 0) {
        ListenerDispose(s->listener);
        s->listener = 0;
    }
    s->state = 0;
    AtomicMaskClear(&s->flags, kFlagNoBuffer);
    QueueTransfer(s->freeList, &s->sendQueue);
    QueueDispose(&s->sendQueue);
    QueueDispose(&s->recvQueue);
    s->sendBytes = 0;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
}

long DatagramGetAddressDDP(DDPSocket *s, NetAddress *out)
{
    IsMPPOpen();
    AddressDuplicate(out, &s->local);
    return 0;
}

void DatagramPostDDP(DDPSocket *s)
{
    char token[16];
    unsigned char post;

    post = 0;
    IsMPPOpen();
    AtomicEnterCritical(token);
    if (s->sending == 0 && (s->sending = (NetPacket *)NetworkQueuePoll(&s->sendQueue)) != 0)
        post = 1;
    AtomicLeaveCritical(token);
    if (post) {
        AddressDecomposeAppleTalk(&s->sending->from, (unsigned short *)&s->header[7], &s->header[0xb], &s->header[0xd], &s->header[0xf]);
        s->wds[0].entryLength = kDDPHeaderBytes;
        s->wds[0].entryPtr = (Ptr)s->header;
        s->wds[1].entryLength = s->sending->length;
        s->wds[1].entryPtr = s->sending->data;
        s->wds[2].entryLength = 0;
        s->wds[2].entryPtr = 0;
        s->pb.DDP.ioCompletion = sDatagramSent;
        s->pb.DDP.socket = s->socket;
        s->pb.DDP.checksumFlag = 1;
        s->pb.DDP.u.wdsPointer = s->wds;
        PWriteDDP(&s->pb, true);
    }
}

Ptr DatagramListenerDDP(Ptr rec, DDPSocket *s)
{
    _AtomicEnter();
    SystemEnterCallback(1);
    if (rec == 0) {
        if ((s->receiving = (NetPacket *)NetworkQueuePoll(s->freeList)) != 0) {
            rec = (Ptr)s->rec;
            s->recvData = s->receiving->data;
        } else {
            AtomicMaskApply(&s->flags, kFlagNoBuffer);
        }
    } else {
        s->receiving->length = *(unsigned short *)&s->rec[10];
        AddressComposeAppleTalk(&s->receiving->from, *(unsigned short *)&s->rec[2], s->rec[4], s->rec[5], s->rec[1]);
        QueueInsert(&s->recvQueue, &s->receiving->link);
        if (!AtomicMaskApply(&s->flags, kFlagPending))
            DTInstall(&s->task);
    }
    SystemLeaveCallback(1);
    _AtomicLeave();
    return rec;
}

static void DatagramDeferredDDP(DDPSocket *s)
{
    NetPacket *p;

    SystemEnterCallback(1);
    AtomicMaskClear(&s->flags, kFlagPending);
    if (AtomicMaskClear(&s->flags, kFlagNoBuffer))
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventNeedBuffers);
    while ((p = (NetPacket *)NetworkQueuePoll(&s->recvQueue)) != 0) {
        QueueInsert(s->recvList, &p->link);
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventReceived);
    }
    SystemLeaveCallback(1);
}

static void DatagramSentDDP(MPPPBPtr pb)
{
    DDPSocket *s;

    s = ((DDPSocket **)pb)[-1];
    if (s != 0) {
        IsMPPOpen();
        SystemEnterCallback(1);
        AtomicIncrement(&s->sendBytes, -s->sending->length);
        QueueInsert(s->sentList, &s->sending->link);
        s->sending = 0;
        DatagramPostDDP(s);
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
    }
    SystemLeaveCallback(1);
}

NetStream *StreamCreateADSP(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    return 0;
}

void StreamTickleADSP(NetStream *s)
{
}

void StreamDisposeADSP(NetSocket *sock)
{
}

long StreamAcceptServerADSP(void *ref, void *other)
{
    return 0;
}

long StreamOpenClientADSP(void *ref, NetAddress *local, const NetAddress *remote, ULong timeout)
{
    return 0;
}

long StreamSendADSP(void *ref, NetChunk *chunk)
{
    return 0;
}

void StreamCloseADSP(NetSocket *sock, unsigned char graceful)
{
}

long StreamGetAddressADSP(void *ref, NetAddress *local, NetAddress *remote)
{
    return 0;
}

Ptr ListenerCreate(void *upp, void *socket)
{
    long response;
    Ptr p;

    response = 0;
    p = NewPtrSys(kListenerBytes);
    if (p != 0) {
        BlockMoveData(sListener, p, kListenerBytes);
        BlockMoveData(&upp, p + 2, sizeof(upp));
        BlockMoveData(&socket, p + 6, sizeof(socket));
        if (Gestalt(gestaltVMAttr, &response) == 0) {
            response &= 1;
            BlockMoveData(&response, p + 10, sizeof(response));
            if (response != 0)
                HoldMemory(p, kListenerBytes);
        }
    }
    return p;
}

void ListenerDispose(Ptr p)
{
    long held;

    held = 0;
    if (p != 0) {
        BlockMoveData(p + 10, &held, sizeof(held));
        if (held != 0)
            UnholdMemory(p, kListenerBytes);
        DisposePtr(p);
    }
}

#endif
