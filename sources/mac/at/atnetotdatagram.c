/* atnetotdatagram.c -- 1.0.10: the network layer on Open Transport: UDP and DDP sockets. Notes: Research/notes/maclibs.md */
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
#define OTOpenMapper(c, o, e) OTOpenMapperInContext(c, o, e, 0)
#define MemoryClear(p, n) BlockZero(p, n)
#define kDatagramNotifier gDatagramNotifierUPP
#else
#define kDatagramNotifier DatagramNotifierOT
#endif

#define kNetErrConfig       -7100
#define kErrFlow            -3161
#define kErrBusy            -3211
#define kErrLook            -3158
#define kErrGone            -3153
#define kErrNoData          -3162
#define kAcceptFailed       -7103
#define kBadSyncState       -3155

extern OTNotifyUPP gDatagramNotifierUPP;

#define kOTOpenFailed       -7102
#define kFlagNoBuffer       4
#define kNotOpen            -7109
#define kEventNeedBuffers   14
#define kFlagInNotifierUDP  20
#define kEventReset         11
#define kOTSyncIdleEvent    0x20000011
#define kOTBindCompleteEvent 0x20000006

extern unsigned char gIdleHookEnabled;
#define kEventAborted       19

#define kUdpBytes           0x590
#define kFlagRecvWait       8
#define kFlagUnique         19
#define kFlagReset          16
#define kOptionReportReset  0x6000
#define kOptionNoAddress    0x6001
#define kOptionTtl          0x16000
#define kOptionName         0x22000
#define kUnsupported        -7003
#define kOptionFailed       -7100
#define kMaxTtl             0xff
#define kIP_TTL             3
#define kNameBytes          0x100
#define kOTNameTaken -3172
#define kNameFailedOT -7104
#define localIP(s) (*(ULong *)&(s)->reply[4])
#define kFlagSendBusy       7
#define kFlagFlow           9
#define kFlagData           2
#define kMaxUnbinds         3
#define kEventSent          10
#define kEventReceived      12
#define kEventReceived2     13

/* a socket on Open Transport's UDP or DDP */

typedef struct OTSocket {
    struct OTSocket *next;
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
    EndpointRef endpoint;
    MapperRef mapper;
    long nameID;
    MutexList sendQueue;
    NetPacket *sending;
    long sendBytes;
    NetAddress reset;
    char reply[0x228];
    char name[0x100];
} OTSocket;

long gOTSocketCount = 0;

extern long ConfigureDatagramOT(OTSocket *s, unsigned char reportReset);
extern long GetNameResource(char *out);
extern void DatagramPostOT(OTSocket *s);
extern void DatagramReceiveOT(void *ref);
extern void DatagramLookOT(OTSocket *s);
extern pascal void DatagramNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie);
void MakeUniqueNBPNameOT(const char *name, char *out, ULong serial);

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

NetSocket *DatagramCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2)
{
    long err, e;
    OTSocket *s;

    err = 0;
    s = MemoryAllocate(kUdpBytes);
    if (s == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(s, kUdpBytes);
    s->kind = 2;
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
    s->sendBytes = 0;
    e = QueueCreate(&s->sendQueue);
    if (e != 0) {
        err = e;
        goto done;
    }
    s->unk13c = 0;
    _EndpointRegister((NetSocket *)s, 2);
    gOTSocketCount++;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventCreated);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
done:
    if (err != 0 && s != 0) {
        DatagramDisposeOT(s);
        s = 0;
    }
    return (NetSocket *)s;
}

void DatagramTickleOT(NetSocket *ref)
{
    OTSocket *s;
    long state;
    Boolean entered;

    s = (OTSocket *)ref;
    state = s->state;
    if (state != 0)
        NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventConnected);
    if (s->endpoint != 0) {
        if (!QueueIsEmpty(s->freeList) && AtomicMaskClear(&s->flags, kFlagNoBuffer)) {
            entered = OTEnterNotifier(s->endpoint);
            DatagramReceiveOT(s);
            if (entered)
                OTLeaveNotifier(s->endpoint);
        }
        if (AtomicMaskClear(&s->flags, kFlagRecvWait)) {
            entered = OTEnterNotifier(s->endpoint);
            DatagramPostOT(s);
            if (entered)
                OTLeaveNotifier(s->endpoint);
        }
    }
}

void DatagramDisposeOT(OTSocket *s)
{
    DatagramCloseOT(s, 0);
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventDisposed);
    QueueDispose(&s->sendQueue);
    _EndpointUnregister((NetSocket *)s, 2);
    MemoryDeallocate(s);
    gOTSocketCount--;
}

long DatagramControlOT(void *ref, void *out, ULong option, ULong value)
{
    OTSocket *s;
    long err;
    long e;
    const char *name;
    char text[0x100];
    TOption ttl;
    TOptMgmt request;
    TOptMgmt reply;
    UInt8 *buf;

    s = ref;
    err = 0;
    switch (option) {
    case kOptionName:
        name = out;
        if (s->stack != kStackAppleTalk) {
            err = kUnsupported;
            break;
        }
        s->name[0] = 0;
        if (value != 0) {
            if (name[0] == '=' && name[1] == ':') {
                e = GetNameResource(text);
                if (e != 0) {
                    err = e;
                    break;
                }
                AtomicMaskApply(&s->flags, kFlagUnique);
                StringAppendSafe(text, name + 1, kNameBytes);
                StringCopySafe(s->name, text, kNameBytes);
            } else {
                AtomicMaskClear(&s->flags, kFlagUnique);
                StringCopySafe(s->name, name, kNameBytes);
            }
        }
        break;
    case kOptionReportReset:
        if ((value & 1) != 0) {
            ConfigureDatagramOT(s, 1);
            AtomicMaskApply(&s->flags, kFlagReset);
        } else {
            ConfigureDatagramOT(s, 0);
            AtomicMaskClear(&s->flags, kFlagReset);
            AtomicMaskClear(&s->flags, 17);
            AtomicMaskClear(&s->flags, 18);
        }
        break;
    case kOptionNoAddress: {
        NetAddress *address;

        address = out;
        switch (s->stack) {
        case kStackInet:
            if (localIP(s) != 0)
                AddressComposeTCPIP(address, 0, localIP(s));
            else
                AddressComposeEmpty(address);
            break;
        case kStackAppleTalk:
            AddressComposeAppleTalk(address, 0, 0xff, 0, 0);
            break;
        }
        break;
    }
    case kOptionTtl:
        if (s->stack != kStackInet) {
            err = kUnsupported;
            break;
        }
        if (out != 0) {
            buf = (UInt8 *)&ttl;
            ttl.level = INET_IP;
            ttl.name = kIP_TTL;
            ttl.len = kOTOneByteOptionSize;
            ttl.status = 0;
            *(unsigned char *)ttl.value = 0;
            request.opt.buf = buf;
            request.opt.len = kOTOneByteOptionSize;
            request.flags = T_CURRENT;
            reply.opt.buf = buf;
            reply.opt.maxlen = kOTOneByteOptionSize;
            if (OTOptionManagement(s->endpoint, &request, &reply) != 0) {
                err = kOptionFailed;
                break;
            }
            *(ULong *)out = *(unsigned char *)ttl.value;
        }
        if (value != 0) {
            ttl.level = INET_IP;
            ttl.name = kIP_TTL;
            ttl.len = kOTOneByteOptionSize;
            ttl.status = 0;
            *(unsigned char *)ttl.value = value < kMaxTtl ? value : kMaxTtl;
            request.opt.buf = (UInt8 *)&ttl;
            request.opt.len = kOTOneByteOptionSize;
            request.flags = T_NEGOTIATE;
            reply.opt.buf = (UInt8 *)&ttl;
            reply.opt.maxlen = kOTFourByteOptionSize;
            if (OTOptionManagement(s->endpoint, &request, &reply) != 0)
                err = kOptionFailed;
        }
        break;
    default:
        err = kUnsupported;
        break;
    }
    return err;
}

long DatagramOpenOT(void *ref, NetAddress *address)
{
    OTSocket *s;
    OSStatus status;
    ULong ip;
    unsigned short port;
    unsigned short net;
    unsigned char node;
    unsigned char socket;
    unsigned char type;
    TRegisterRequest registerRequest;
    TOption value;
    TBind request;
    TBind reply;
    TRegisterReply registerReply;
    TOptMgmt option;
    unsigned char nbp[0x6c];
    Str255 unique;
    NetAddress any;
    OTConfigurationRef config;
    long err;
    ULong tries;
    unsigned char bind;

    s = ref;
    err = 0;
    config = 0;
    status = 0;
    ip = 0;
    port = 0;
    net = 0;
    node = 0;
    socket = 0;
    type = 0;
    s->state = kStateOpen;
    AtomicMaskClear(&s->flags, kFlagNoBuffer);
    AtomicMaskClear(&s->flags, kFlagData);
    AtomicMaskClear(&s->flags, 8);
    AtomicMaskClear(&s->flags, kFlagFlow);
    AtomicMaskClear(&s->flags, 17);
    AtomicMaskClear(&s->flags, 18);
    switch (s->stack) {
    case kStackInet:
        config = OTCreateConfiguration("udp");
        if (config == 0) {
            err = kOutOfMemory;
            goto done;
        }
        break;
    case kStackAppleTalk:
        if (OTCreateConfiguration("ddp") == 0) {
            err = kOutOfMemory;
            goto done;
        }
        config = OTCreateConfiguration("ddp");
        break;
    }
    if ((s->endpoint = OTOpenEndpoint(config, 0, 0, &status)) == 0) {
        err = kOTOpenFailed;
        goto done;
    }
    {
        long e;

        e = CriticalCreateFromProvider(s->lock, s->endpoint);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (address != 0) {
        switch (s->stack) {
        case kStackInet:
            AddressDecomposeTCPIP(address, &port, &ip);
            bind = (port != 0 || ip != 0) != 0;
            break;
        case kStackAppleTalk:
            AddressDecomposeAppleTalk(address, &net, &node, &socket, &type);
            bind = (socket != 0 || type != 0) != 0;
            break;
        default:
            bind = 1;
            break;
        }
    } else if (_PlatformIsThreaded()) {
        switch (s->stack) {
        case kStackInet:
            address = &any;
            AddressComposeTCPIP(address, 0, 0);
            bind = 1;
            break;
        case kStackAppleTalk:
            address = &any;
            AddressComposeAppleTalk(address, 0, 0, 0, 0);
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
        request.addr.len = address->kind;
        request.addr.buf = (UInt8 *)&address->family;
    } else {
        request.addr.maxlen = 0;
        request.addr.len = 0;
        request.addr.buf = 0;
    }
    request.qlen = 0;
    reply.addr.maxlen = kNameBytes;
    reply.addr.len = s->local.kind;
    reply.addr.buf = (UInt8 *)&s->local.family;
    reply.qlen = 0;
    if (OTBind(s->endpoint, &request, &reply) != 0) {
        err = kAcceptFailed;
        goto done;
    }
    AtomicMaskApply(&s->flags, 0);
    s->local.kind = reply.addr.len;
    if ((char)s->name[0] != 0 && s->stack == kStackAppleTalk) {
        if ((s->mapper = OTOpenMapper(OTCreateConfiguration("nbp"), 0, &status)) == 0) {
            err = kOTOpenFailed;
            goto done;
        }
        registerRequest.addr.maxlen = reply.addr.maxlen;
        registerRequest.addr.len = reply.addr.len;
        registerRequest.addr.buf = reply.addr.buf;
        registerRequest.flags = 0;
        registerReply.addr.maxlen = 0;
        registerReply.addr.len = 0;
        registerReply.addr.buf = 0;
        registerReply.nameid = 0;
        for (tries = 0; tries < 4; tries++) {
            MakeUniqueNBPNameOT(s->name, (char *)unique, tries);
            registerRequest.name.maxlen = sizeof(nbp);
            registerRequest.name.len = OTInitNBPAddress(nbp, (char *)unique);
            registerRequest.name.buf = nbp;
            err = OTRegisterName(s->mapper, &registerRequest, &registerReply);
            if (err != kOTNameTaken)
                break;
        }
        if (err != 0) {
            err = kNameFailedOT;
            goto done;
        }
        AtomicMaskApply(&s->flags, 1);
        s->nameID = registerReply.nameid;
    }
    if (address != 0 && s->stack == kStackInet) {
        option.opt.maxlen = kOTFourByteOptionSize;
        option.opt.len = kOTFourByteOptionSize;
        option.opt.buf = (UInt8 *)&value;
        option.flags = 4;
        value.len = kOTFourByteOptionSize;
        value.level = 0;
        value.name = 4;
        value.status = 0;
        value.value[0] = 1;
        if (OTOptionManagement(s->endpoint, &option, 0) != 0) {
            err = kNetErrConfig;
            goto done;
        }
    }
    s->state = kStateOpen;
    if (address != 0) {
        if (s->stack == kStackAppleTalk && *(unsigned short *)&s->local == 0x101) {
            *(unsigned short *)&s->local = kNetFamilyIPX;
            ((unsigned char *)&s->local)[8] = 0;
            ((unsigned char *)&s->local)[7] = 0;
        }
        AddressDuplicate(address, &s->local);
    }
    {
        long e;

        e = ConfigureDatagramOT(s, AtomicMaskTest(&s->flags, kFlagReset));
        if (e != 0)
            err = e;
    }
    if (OTInstallNotifier(s->endpoint, kDatagramNotifier, s) != 0) {
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
    if (err != 0)
        DatagramClose(s, 0);
    return err;
}

long DatagramSendOT(OTSocket *s, NetPacket *packet)
{
    long err;
    NetPacket *p;
    Boolean entered;
    long state;

    err = 0;
    state = s->state;
    if (state == 0) {
        err = kNotOpen;
    } else {
        entered = OTEnterNotifier(s->endpoint);
        QueueInsert(&s->sendQueue, &packet->link);
        AtomicIncrement(&s->sendBytes, packet->length);
        DatagramPostOT(s);
        if (entered)
            OTLeaveNotifier(s->endpoint);
    }
    return err;
}

void DatagramCloseOT(void *ref, long graceful)
{
    OTSocket *s;
    OSStatus status, e;
    char wait;
    ULong tries;

    s = ref;
    status = 0;
    SystemCheckCallback();
    if (s->endpoint != 0) {
        OTRemoveNotifier(s->endpoint);
        e = OTSetSynchronous(s->endpoint);
        if (e != 0)
            status = e;
        if (status == kBadSyncState)
            s->endpoint = 0;
    }
    if (s->endpoint != 0) {
        DatagramLookOT(s);
        wait = graceful;
        while ((unsigned char)wait != 0 && s->sending != 0) {
            SystemIdle();
            DatagramLookOT(s);
            DatagramPostOT(s);
        }
        if (AtomicMaskClear(&s->flags, 1) && s->mapper != 0 && s->nameID != 0) {
            e = OTDeleteNameByID(s->mapper, s->nameID);
            if (e != 0)
                status = e;
            s->nameID = 0;
        }
        if (s->mapper != 0) {
            e = OTSetSynchronous(s->mapper);
            if (e != 0)
                status = e;
            if (status != kBadSyncState)
                OTCloseProvider(s->mapper);
            s->mapper = 0;
        }
        if (AtomicMaskClear(&s->flags, 0)) {
            tries = 0;
            while (OTUnbind(s->endpoint) == kErrLook && (unsigned char)graceful != 0) {
                SystemIdle();
                DatagramLookOT(s);
                if (tries >= kMaxUnbinds)
                    graceful = 0;
                tries++;
            }
        }
        OTCloseProvider(s->endpoint);
#if TARGET_API_MAC_CARBON
        if (_PlatformIsThreaded() && AtomicMaskTest(&s->flags, kFlagInNotifierUDP)) {
            do {
                SystemIdle();
                MPYield();
            } while (AtomicMaskTest(&s->flags, kFlagInNotifierUDP));
        }
#endif
    }
    s->endpoint = 0;
    CriticalDispose(s->lock);
    s->state = 0;
    AtomicMaskClear(&s->flags, kFlagNoBuffer);
    AtomicMaskClear(&s->flags, kFlagData);
    AtomicMaskClear(&s->flags, 8);
    AtomicMaskClear(&s->flags, kFlagFlow);
    AtomicMaskClear(&s->flags, 17);
    AtomicMaskClear(&s->flags, 18);
    QueueTransfer(s->freeList, &s->sendQueue);
    MemoryClear(s->reply, sizeof(s->reply));
    s->sendBytes = 0;
    s->sending = 0;
    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventClosed);
}

long DatagramGetAddressOT(OTSocket *s, NetAddress *out)
{
    long err;
    long state;

    err = 0;
    state = s->state;
    if (state == 0)
        err = kNotOpen;
    if (err != 0)
        AddressComposeEmpty(out);
    else
        AddressDuplicate(out, &s->local);
    return err;
}

static inline NetPacket *_DatagramNextPacket(OTSocket *s)
{
    if (s->sending != 0)
        return s->sending;
    return (NetPacket *)NetworkQueuePoll(&s->sendQueue);
}

void DatagramPostOT(OTSocket *s)
{
    unsigned char held;
    long state;
    NetPacket *packet;
    TUnitData data;
    OTResult result;
    unsigned char done;

    state = s->state;
    if (state != 0) {
        held = AtomicMaskApply(&s->flags, kFlagSendBusy);
        done = held;
        while (done == 0 && !AtomicMaskTest(&s->flags, kFlagFlow)) {
            packet = _DatagramNextPacket(s);
            s->sending = 0;
            if (packet == 0)
                break;
            data.addr.maxlen = kNameBytes;
            data.addr.len = packet->from.kind;
            data.addr.buf = (UInt8 *)&packet->from.family;
            data.opt.maxlen = 0;
            data.opt.len = 0;
            data.opt.buf = 0;
            data.udata.maxlen = packet->capacity;
            data.udata.len = packet->length;
            data.udata.buf = (UInt8 *)packet->data;
            result = OTSndUData(s->endpoint, &data);
            if (result == kErrFlow) {
                AtomicMaskApply(&s->flags, kFlagFlow);
                s->sending = packet;
                break;
            }
            if (result == kErrBusy) {
                AtomicMaskApply(&s->flags, 8);
                s->sending = packet;
                break;
            }
            if (result == kErrLook) {
                DatagramLookOT(s);
            } else if (result == kErrGone) {
                state = s->state;
                if (state != 0) {
                    s->state = 0;
                    NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAborted);
                    result = kNotOpen;
                }
                if (s->endpoint != 0) {
                    OSStatus e;

                    OTRemoveNotifier(s->endpoint);
                    e = OTSetSynchronous(s->endpoint);
                    if (e != 0)
                        result = e;
                    if (result != kBadSyncState)
                        OTCloseProvider(s->endpoint);
                    s->endpoint = 0;
                }
                AtomicIncrement(&s->sendBytes, -packet->length);
                QueueInsert(s->sentList, &packet->link);
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
            } else {
                AtomicIncrement(&s->sendBytes, -packet->length);
                QueueInsert(s->sentList, &packet->link);
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventSent);
            }
        }
        if (!held) {
            AtomicMaskClear(&s->flags, kFlagSendBusy);
            if (!QueueIsEmpty(&s->sendQueue) && !AtomicMaskTest(&s->flags, kFlagFlow) && !AtomicMaskTest(&s->flags, 8))
                DatagramPostOT(s);
        }
    }
}

void DatagramReceiveOT(void *ref)
{
    OTSocket *s;
    OTResult err;
    NetPacket *packet;
    size_t count;
    OTFlags flags;
    TUnitData data;
    long state;
    long event;

    s = ref;
    err = 0;
    flags = 0;
    state = s->state;
    if (state != 0)
    while (err == 0 && AtomicMaskTest(&s->flags, kFlagData) && !AtomicMaskTest(&s->flags, kFlagNoBuffer)) {
        packet = (NetPacket *)NetworkQueuePoll(s->freeList);
        if (packet == 0 && !AtomicMaskApply(&s->flags, kFlagNoBuffer)) {
            NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventNeedBuffers);
            packet = (NetPacket *)NetworkQueuePoll(s->freeList);
        }
        if (packet == 0)
            break;
        AtomicMaskClear(&s->flags, kFlagNoBuffer);
        data.addr.maxlen = kNameBytes;
        data.addr.len = packet->from.kind;
        data.addr.buf = (UInt8 *)&packet->from.family;
        data.opt.maxlen = 0;
        data.opt.len = 0;
        data.opt.buf = 0;
        data.udata.maxlen = packet->capacity;
        data.udata.len = packet->length;
        data.udata.buf = (UInt8 *)packet->data;
        {
            OTResult got;

            got = OTRcvUData(s->endpoint, &data, &flags);
            if (got < 0)
                err = got;
        }
        if (err == kErrNoData || OTCountDataBytes(s->endpoint, &count) == kErrNoData || count == 0)
            AtomicMaskClear(&s->flags, kFlagData);
        if (err == 0) {
            packet->from.kind = data.addr.len;
            packet->length = data.udata.len;
            QueueInsert((flags & T_EXPEDITED) != 0 ? s->recvList2 : s->recvList, &packet->link);
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
        } else if (err == kErrGone) {
            state = s->state;
            if (state != 0) {
                s->state = 0;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAborted);
                err = kNotOpen;
            }
            if (s->endpoint != 0) {
                OSStatus e;

                OTRemoveNotifier(s->endpoint);
                e = OTSetSynchronous(s->endpoint);
                if (e != 0)
                    err = e;
                if (err != kBadSyncState) {
                    e = OTCloseProvider(s->endpoint);
                    if (e != 0)
                        err = e;
                }
                s->endpoint = 0;
            }
        } else {
            QueueInsert(s->freeList, &packet->link);
            if (err == kErrLook) {
                DatagramLookOT(s);
                return;
            }
            return;
        }
    }
}

void DatagramLookOT(OTSocket *s)
{
    OTResult code;

    code = OTLook(s->endpoint);
    if (code != 0)
        DatagramNotifierOT(s, code, 0, s);
}

long ConfigureDatagramOT(OTSocket *s, unsigned char reportReset)
{
    long err;
    TOption option;
    TOptMgmt request;

    err = 0;
    if (s != 0 && (long)s->state == kStateOpen) {
        option.len = kOTOneByteOptionSize;
        option.level = INET_UDP;
        option.name = 2;
        option.status = 0;
        *(unsigned char *)option.value = reportReset != 0;
        request.opt.buf = (UInt8 *)&option;
        request.opt.len = kOTFourByteOptionSize;
        request.flags = T_NEGOTIATE;
        if (OTOptionManagement(s->endpoint, &request, 0) != 0)
            err = kOptionFailed;
    }
    return err;
}

pascal void DatagramNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie)
{
    OTSocket *s;
    OSStatus status;
    unsigned char held;

    s = ref;
    status = 0;
    held = 0;
    if (result != 0)
        status = result;
    if (_PlatformIsThreaded())
        held = AtomicMaskApply(&s->flags, kFlagInNotifierUDP);
    if (OTIsSynchronous(s->endpoint) == 0) {
        switch (code) {
        case T_DATA:
        case T_EXDATA: {
            Boolean entered;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            AtomicMaskApply(&s->flags, kFlagData);
            DatagramReceiveOT(s);
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_UDERR: {
            Boolean entered;
            TUDErr error;
            char option[kNameBytes];
            NetAddress from;

            entered = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            error.addr.maxlen = kNameBytes;
            error.addr.len = 0;
            error.addr.buf = (UInt8 *)&from.family;
            error.opt.maxlen = kNameBytes;
            error.opt.len = 0;
            error.opt.buf = (UInt8 *)option;
            error.error = 0;
            {
                OSStatus e;

                e = OTRcvUDErr(s->endpoint, &error);
                if (e != 0)
                    status = e;
            }
            if (status == 0 && (ULong)(from.kind = error.addr.len) != 0 && AtomicMaskTest(&s->flags, kFlagReset) && !AtomicMaskTest(&s->flags, 17) && !AtomicMaskApply(&s->flags, 18)) {
                AddressDuplicate(&s->reset, &from);
                AtomicMaskApply(&s->flags, 17);
                AtomicMaskClear(&s->flags, 18);
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventReset);
            }
            SystemLeaveCallback(0);
            if (entered)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case T_GODATA:
        case T_GOEXDATA: {
            Boolean e2;

            e2 = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            AtomicMaskClear(&s->flags, kFlagFlow);
            DatagramPostOT(s);
            SystemLeaveCallback(0);
            if (e2)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        case kOTSyncIdleEvent:
            if (gIdleHookEnabled)
                SystemIdle();
            DatagramTickle((NetSocket *)s);
            break;
        case kOTBindCompleteEvent:
            break;
        case kOTProviderWillClose:
        case kOTProviderIsClosed: {
            long state;
            OSStatus e;

            state = s->state;
            if (state != 0) {
                s->state = 0;
                NetworkMakeCallback(s->proc, s->ref, (NetSocket *)s, kEventAborted);
                status = kNotOpen;
            }
            if (s->endpoint != 0) {
                OTRemoveNotifier(s->endpoint);
                e = OTSetSynchronous(s->endpoint);
                if (e != 0)
                    status = e;
                if (status != kBadSyncState)
                    OTCloseProvider(s->endpoint);
                s->endpoint = 0;
            }
            break;
        }
        default: {
            Boolean e3;

            e3 = OTEnterNotifier(s->endpoint);
            SystemEnterCallback(0);
            SystemLeaveCallback(0);
            if (e3)
                OTLeaveNotifier(s->endpoint);
            break;
        }
        }
    }
    if (_PlatformIsThreaded() && !held)
        AtomicMaskClear(&s->flags, kFlagInNotifierUDP);
}
