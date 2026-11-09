/* atnetotresolver.c -- 1.0.10: the network layer on Open Transport: the resolver, an internet services provider or a mapper for NBP. Notes: Research/notes/maclibs.md */
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
#define OTOpenMapper(c, o, e) OTOpenMapperInContext(c, o, e, 0)
#define MemoryClear(p, n) BlockZero(p, n)
#define kResolverNotifier gResolverNotifierUPP
#else
#define kResolverNotifier ResolverNotifierOT
#endif

#define kNetErrConfig       -7100
#define kErrGone            -3153
#define kErrNoData          -3162
#define kBadSyncState       -3155

extern OTNotifyUPP gResolverNotifierUPP;

#define kResolverBytes      0x284
#define kResolverBuffer     0x1214
#define kOTOpenFailed       -7102
#define kFlagNoBuffer       4
#define kFlagBusy           6
#define kFlagMore           16
#define kFlagInNotifier     17
#define kNotOpen            -7109
#define kEventResolved      8
#define kEventLookupDone    9
#define kEventNeedBuffers   14
#define kOTLookupNameBytes  0x100

#define kEventAborted       19
#define kPrivateLookupDone  0x30000000

/* the resolver on Open Transport: an internet services provider, or a mapper for NBP */
typedef struct OTResolver {
    struct OTResolver *next;
    unsigned short kind;
    unsigned short stack;
    unsigned short state;
    unsigned short padA;
    ULong lock[4];
    NetProc proc;
    void *ref;
    ULong flags;
    MutexList *freeList;
    MutexList *doneList;
    long unk30;
    MutexList requests;
    ProviderRef provider;
    NetLookup *current;
    ULong left;
    ULong wanted;
    union {
        struct {
            TLookupReply reply;
            TLookupRequest request;
            char *buffer;
            ULong bufferSize;
        } nbp;
        InetHostInfo host;
    } u;
    ULong ip;
    unsigned short port;
    char pad182[2];
    char name[0x100];
} OTResolver;

extern pascal void ResolverNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie);
static void ResolverNextOT(OTResolver *r, unsigned char start);

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
    long n;
    const unsigned char *p;

    p = (const unsigned char *)s - 1;
    n = -1;
    do {
        n++;
    } while (*++p != 0);
    return n;
}

NetResolver *ResolverCreateOT(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList)
{
    long err, e;
    OTResolver *r;

    err = 0;
    r = MemoryAllocate(kResolverBytes);
    if (r == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(r, kResolverBytes);
    r->kind = 1;
    r->stack = stack;
    r->state = 0;
    r->proc = proc;
    r->ref = ref;
    r->flags = 0;
    r->freeList = freeList;
    r->doneList = doneList;
    r->wanted = 1;
    if (r->stack == kStackAppleTalk) {
        r->u.nbp.bufferSize = kResolverBuffer;
        if ((r->u.nbp.buffer = MemoryAllocate(r->u.nbp.bufferSize)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    r->unk30 = 0;
    e = QueueCreate(&r->requests);
    if (e != 0) {
        err = e;
        goto done;
    }
    _EndpointRegister((NetSocket *)r, 1);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventCreated);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventClosed);
done:
    if (err != 0 && r != 0) {
        if (r->stack == kStackAppleTalk && r->u.nbp.buffer != 0) {
            MemoryDeallocate(r->u.nbp.buffer);
            r->u.nbp.buffer = 0;
        }
        ResolverDispose(r);
        r = 0;
    }
    return (NetResolver *)r;
}

void ResolverTickleOT(NetSocket *s)
{
    long state;

    state = s->state;
    if (state != 0)
        NetworkMakeCallback(s->proc, s->ref, s, kEventConnected);
}

void ResolverDisposeOT(OTResolver *r)
{
    ResolverCloseOT(r);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventDisposed);
    QueueDispose(&r->requests);
    if (r->stack == kStackAppleTalk && r->u.nbp.buffer != 0) {
        MemoryDeallocate(r->u.nbp.buffer);
        r->u.nbp.buffer = 0;
    }
    _EndpointUnregister((NetSocket *)r, 1);
    MemoryDeallocate(r);
}

long ResolverOpenOT(void *ref)
{
    OTResolver *r;
    long err;
    OSStatus status;
    long e;

    r = ref;
    err = 0;
    status = 0;
    switch (r->stack) {
    case kStackInet:
        if ((r->provider = OTOpenInternetServices(kDefaultInternetServicesPath, 0, &status)) == 0) {
            err = kOTOpenFailed;
            goto done;
        }
        break;
    case kStackAppleTalk: {
        OTConfigurationRef config;

        config = OTCreateConfiguration("nbp");
        if (config == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((r->provider = OTOpenMapper(config, 0, &status)) == 0) {
            err = kOTOpenFailed;
            goto done;
        }
        break;
    }
    }
    if (status != 0) {
        err = kOTOpenFailed;
        goto done;
    }
    e = CriticalCreateFromProvider(r->lock, r->provider);
    if (e != 0) {
        err = e;
        goto done;
    }
    r->state = 4;
    AtomicMaskClear(&r->flags, kFlagNoBuffer);
    AtomicMaskClear(&r->flags, kFlagBusy);
    AtomicMaskClear(&r->flags, kFlagMore);
    if (OTInstallNotifier(r->provider, kResolverNotifier, r) != 0) {
        err = kNetErrConfig;
        goto done;
    }
    if (OTSetAsynchronous(r->provider) != 0)
        err = kNetErrConfig;
done:
    if (err != 0)
        ResolverCloseOT(r);
    return err;
}

long ResolverControlOT(void *ref, NetLookup *lookup)
{
    OTResolver *r;
    long err;
    Boolean entered;
    long state;

    r = ref;
    err = 0;
    state = r->state;
    if (state == 0) {
        err = kNotOpen;
        goto done;
    }
    AddressComposeEmpty(&lookup->address);
    QueueInsert(&r->requests, &lookup->link);
    entered = OTEnterNotifier(r->provider);
    ResolverNextOT(r, 0);
    if (entered)
        OTLeaveNotifier(r->provider);
done:
    return err;
}

void ResolverCloseOT(void *ref)
{
    OTResolver *r;
    OSStatus status, e;

    r = ref;
    status = 0;
    SystemCheckCallback();
    if (r->provider != 0) {
        OTRemoveNotifier(r->provider);
        e = OTSetSynchronous(r->provider);
        if (e != 0)
            status = e;
        if (status != kBadSyncState)
            OTCloseProvider(r->provider);
#if TARGET_API_MAC_CARBON
        if (_PlatformIsThreaded() && AtomicMaskTest(&r->flags, kFlagInNotifier)) {
            do {
                SystemIdle();
                MPYield();
            } while (AtomicMaskTest(&r->flags, kFlagInNotifier));
        }
#endif
    }
    r->provider = 0;
    CriticalDispose(r->lock);
    r->state = 0;
    AtomicMaskClear(&r->flags, kFlagNoBuffer);
    AtomicMaskClear(&r->flags, kFlagBusy);
    AtomicMaskClear(&r->flags, kFlagMore);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventClosed);
}

static void ResolverNextOT(OTResolver *r, unsigned char start)
{
    if (!start && !QueueIsEmpty(&r->requests) && !AtomicMaskApply(&r->flags, kFlagBusy))
        start = 1;
    if (start && (r->current = (NetLookup *)NetworkQueuePoll(&r->requests)) != 0) {
        OSStatus e;

        AtomicMaskApply(&r->flags, kFlagMore);
        if (r->current->name[0] != 0) {
            switch (r->stack) {
            case kStackInet: {
                char c;
                ULong i;

                r->ip = 0;
                r->port = 0;
                MslCopy(r->name, r->current->name);
                r->left = r->wanted;
                i = 0;
                while ((c = r->name[i]) != 0 && c != ':' && c != ',' && c != ' ')
                    i++;
                if (r->name[i] != 0) {
                    r->name[i++] = 0;
                    r->port = StringToNumber(&r->name[i]);
                }
                e = OTInetStringToAddress(r->provider, r->name, &r->u.host);
                if (e != 0)
                    ResolverNotifierOT(r, T_DNRSTRINGTOADDRCOMPLETE, e, r);
                break;
            }
            case kStackAppleTalk: {
                ULong n;

                MslCopy(r->name, r->current->name);
                r->u.nbp.request.name.maxlen = kOTLookupNameBytes;
                r->u.nbp.request.name.len = MslLength(r->name);
                r->u.nbp.request.name.buf = (UInt8 *)r->name;
                r->u.nbp.request.addr.maxlen = 0;
                r->u.nbp.request.addr.len = 0;
                r->u.nbp.request.addr.buf = 0;
                n = r->wanted != 0 ? r->wanted : 0xffffffff;
                r->left = n;
                r->u.nbp.request.maxcnt = n;
                r->u.nbp.request.timeout = 0;
                r->u.nbp.request.flags = 0;
                r->u.nbp.reply.names.maxlen = r->u.nbp.bufferSize;
                r->u.nbp.reply.names.len = 0;
                r->u.nbp.reply.names.buf = (UInt8 *)r->u.nbp.buffer;
                r->u.nbp.reply.rspcount = 0;
                e = OTLookupName(r->provider, &r->u.nbp.request,&r->u.nbp.reply);
                if (e != 0)
                    ResolverNotifierOT(r, T_LKUPNAMECOMPLETE, e, r);
                break;
            }
            }
        } else {
            switch (r->stack) {
            case kStackInet:
                AddressDecomposeTCPIP(&r->current->address, &r->port, &r->ip);
                r->left = r->wanted;
                e = OTInetAddressToName(r->provider, r->ip, r->name);
                if (e != 0)
                    ResolverNotifierOT(r, T_DNRADDRTONAMECOMPLETE, e, r);
                break;
            case kStackAppleTalk:
                AddressTranscribe(&r->current->address, r->current->name);
                r->left = r->wanted;
                ResolverNotifierOT(r, kPrivateLookupDone, 0, r);
                break;
            }
        }
    } else if (start) {
        AtomicMaskClear(&r->flags, kFlagBusy);
        ResolverNextOT(r, 0);
    }
}

static unsigned char LookupReplyToName(const unsigned short *entry, NetLookup *lookup)
{
    const unsigned short *addr;
    const unsigned char *name;
    unsigned char ok;
    long n;

    addr = entry + 2;
    n = entry[0];
    name = (const unsigned char *)addr + n;
    ok = 0;
    if (n == 8 && addr[0] == kNetFamilyIPX) {
        AddressComposeAppleTalk(&lookup->address, addr[1], ((const unsigned char *)addr)[4], ((const unsigned char *)addr)[5], ((const unsigned char *)addr)[6]);
        if (entry[1] != 0)
            BlockMoveData(name, lookup->name, entry[1]);
        lookup->name[entry[1]] = 0;
        ok = 1;
    }
    return ok;
}

pascal void ResolverNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie)
{
    OSStatus status;
    OTResolver *r;
    unsigned char posted;
    unsigned char ended;
    unsigned char next;
    Boolean entered;
    NetLookup *lookup;
    unsigned char held;

    r = ref;
    status = 0;
    posted = 0;
    ended = 0;
    next = 0;
    held = 0;
    if (_PlatformIsThreaded())
        held = AtomicMaskApply(&r->flags, kFlagInNotifier);
    switch (result) {
    case kErrGone:
        code = kOTProviderIsClosed;
        break;
    case kErrNoData:
        code = kPrivateLookupDone;
        break;
    case 0:
        break;
    default:
        code = kPrivateLookupDone;
        status = result;
        break;
    }
    switch (code) {
    case T_DNRSTRINGTOADDRCOMPLETE: {
        ULong i;

        entered = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        if (r->u.host.addrs[0] != 0)
            AddressComposeTCPIP(&r->current->address, r->port, r->u.host.addrs[0]);
        AtomicMaskClear(&r->flags, kFlagMore);
        QueueInsert(r->doneList, &r->current->link);
        r->current = 0;
        NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
        for (i = 1; i < r->left && i < kMaxHostAddrs && r->u.host.addrs[i] != 0; i++) {
            lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            if (lookup == 0 && !AtomicMaskApply(&r->flags, kFlagNoBuffer)) {
                NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventNeedBuffers);
                lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            }
            AtomicMaskClear(&r->flags, kFlagNoBuffer);
            if (lookup == 0)
                break;
            MslCopy(lookup->name, r->name);
            AddressComposeTCPIP(&lookup->address, r->port, r->u.host.addrs[i]);
            QueueInsert(r->doneList, &lookup->link);
            NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
        }
        next = 1;
        ended = 1;
        posted = 0;
        SystemLeaveCallback(0);
        if (entered)
            OTLeaveNotifier(r->provider);
        break;
    }
    case T_DNRADDRTONAMECOMPLETE: {
        Boolean e2;
        ULong n;
        char number[0x100];

        e2 = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        MslCopy(r->current->name, r->name);
        n = MslLength(r->current->name);
        if (n != 0 && r->current->name[n - 1] == '.')
            r->current->name[n - 1] = 0;
        if (r->port != 0) {
            StringFromNumber(number, r->port);
            MslAppend(r->current->name, ":");
            MslAppend(r->current->name, number);
        }
        AtomicMaskClear(&r->flags, kFlagMore);
        QueueInsert(r->doneList, &r->current->link);
        r->current = 0;
        next = 1;
        ended = 1;
        posted = 1;
        SystemLeaveCallback(0);
        if (e2)
            OTLeaveNotifier(r->provider);
        break;
    }
    case T_RESOLVEADDRCOMPLETE: {
        Boolean e6;

        e6 = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        SystemLeaveCallback(0);
        if (e6)
            OTLeaveNotifier(r->provider);
        break;
    }
    case T_LKUPNAMECOMPLETE:
    case T_LKUPNAMERESULT: {
        const unsigned short *entry;
        Boolean e3;

        entry = (const unsigned short *)r->u.nbp.reply.names.buf;
        e3 = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        while (r->u.nbp.reply.rspcount != 0) {
            lookup = AtomicMaskTest(&r->flags, kFlagMore) ? r->current : (NetLookup *)NetworkQueuePoll(r->freeList);
            if (lookup == 0 && !AtomicMaskApply(&r->flags, kFlagNoBuffer)) {
                NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventNeedBuffers);
                lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            }
            AtomicMaskClear(&r->flags, kFlagNoBuffer);
            if (lookup == 0)
                break;
            if (LookupReplyToName(entry, lookup)) {
                AtomicMaskClear(&r->flags, kFlagMore);
                QueueInsert(r->doneList, &lookup->link);
                NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
            } else if (!AtomicMaskTest(&r->flags, kFlagMore)) {
                QueueInsert(r->freeList, &lookup->link);
            }
            r->u.nbp.reply.rspcount--;
            {
                ULong bytes;

                bytes = entry[0] + entry[1];
                bytes = (bytes + 7) & ~3;
                entry = (const unsigned short *)((const char *)entry + bytes);
            }
        }
        if (code == T_LKUPNAMECOMPLETE) {
            if (AtomicMaskClear(&r->flags, kFlagMore))
                QueueInsert(r->doneList, &r->current->link);
            r->current = 0;
            next = 1;
            ended = 1;
            posted = 0;
        }
        SystemLeaveCallback(0);
        if (e3)
            OTLeaveNotifier(r->provider);
        break;
    }
    case kOTProviderWillClose:
    case kOTProviderIsClosed: {
        long state;
        OSStatus e;

        state = r->state;
        if (state != 0) {
            r->state = 0;
            NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventAborted);
            status = kNotOpen;
        }
        if (r->provider != 0) {
            OTRemoveNotifier(r->provider);
            e = OTSetSynchronous(r->provider);
            if (e != 0)
                status = e;
            if (status != kBadSyncState)
                OTCloseProvider(r->provider);
            r->provider = 0;
        }
        break;
    }
    case kPrivateLookupDone: {
        Boolean e4;

        e4 = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        AtomicMaskClear(&r->flags, kFlagMore);
        QueueInsert(r->doneList, &r->current->link);
        r->current = 0;
        next = 1;
        ended = 1;
        posted = 1;
        SystemLeaveCallback(0);
        if (e4)
            OTLeaveNotifier(r->provider);
        break;
    }
    default: {
        Boolean e5;

        e5 = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        SystemLeaveCallback(0);
        if (e5)
            OTLeaveNotifier(r->provider);
        break;
    }
    }
    if (posted || ended || next) {
        entered = OTEnterNotifier(r->provider);
        SystemEnterCallback(0);
        if (posted)
            NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
        if (ended)
            NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventLookupDone);
        if (next)
            ResolverNextOT(r, 1);
        SystemLeaveCallback(0);
        if (entered)
            OTLeaveNotifier(r->provider);
    }
    if (_PlatformIsThreaded() && !held)
        AtomicMaskClear(&r->flags, kFlagInNotifier);
}
