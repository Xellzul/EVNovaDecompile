/* atnetalkresolver.c -- 1.0.10: AppleTalk without Open Transport, the Preferred Pascal calls: the NBP resolver. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <AppleTalk.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON

#define kResolverBytes      0x2ac
#define kResolverBuffer     0xa28
#define kEventResolved      8
#define kEventLookupDone    9
#define kEventNeedBuffers   14
#define kFlagOpened         4
#define kFlagBusy           6
#define kFlagForward        16
#define kNoReply            -1105
#define kNotResolved        -7108
#define kNameLong           300
#define kNameShort          120

/* the resolver on NBP: one lookup at a time, its parameter block preceded by the object */
typedef struct NBPResolver {
    struct NBPResolver *next;
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
    int32_t unk30;
    struct NBPResolver *self;
    MPPParamBlock pb;
    char *buffer;
    ULong bufferSize;
    MutexList requests;
    int32_t deadline;
    int32_t wanted;
    NetLookup *current;
    char entity[0x100];
    char name[0x100];
} NBPResolver;

static void ResolverNextNBP(NBPResolver *r, unsigned char start);
static void ResolverDoneNBP(MPPPBPtr pb);
static RoutineDescriptor sResolverDoneNBPDesc = BUILD_ROUTINE_DESCRIPTOR(uppMPPCompletionProcInfo, ResolverDoneNBP);
static MPPCompletionUPP sResolverDoneNBP = &sResolverDoneNBPDesc;


NetResolver *ResolverCreateNBP(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList)
{
    int32_t err;
    NBPResolver *r;
    int32_t e;

    err = 0;
    IsMPPOpen();
    r = MemoryAllocate(kResolverBytes);
    if (r == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(r, kResolverBytes);
    r->kind = 1;
    r->stack = 1;
    r->state = 0;
    r->proc = proc;
    r->ref = ref;
    r->flags = 0;
    r->freeList = freeList;
    r->doneList = doneList;
    r->unk30 = 0;
    r->self = r;
    r->wanted = 1;
    e = CriticalCreate(r->lock);
    if (e != 0) {
        err = e;
        goto done;
    }
    r->bufferSize = kResolverBuffer;
    if ((r->buffer = MemoryAllocate(kResolverBuffer)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    _EndpointRegister((NetSocket *)r, 1);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventCreated);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventClosed);
done:
    if (err != 0 && r != 0) {
        ResolverDisposeNBP(r);
        r = 0;
    }
    return (NetResolver *)r;
}

void ResolverTickleNBP(NBPResolver *r)
{
    MPPParamBlock pb;
    int32_t state;

    IsMPPOpen();
    if (r->deadline != 0 && r->deadline <= TickCount()) {
        r->deadline = 0;
        MemoryClear(&pb, sizeof(pb));
        pb.NBPKILL.nKillQEl = &r->pb;
        PKillNBP(&pb, false);
    }
    state = r->state;
    if (state != 0)
        NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventConnected);
}

void ResolverDisposeNBP(NBPResolver *r)
{
    ResolverCloseNBP(r);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventDisposed);
    if (r->buffer != 0) {
        MemoryDeallocate(r->buffer);
        r->buffer = 0;
        r->bufferSize = 0;
    }
    CriticalDispose(r->lock);
    _EndpointUnregister((NetSocket *)r, 1);
    MemoryDeallocate(r);
}

int32_t ResolverOpenNBP(NBPResolver *r)
{
    int32_t err, e;

    err = 0;
    IsMPPOpen();
    e = QueueCreate(&r->requests);
    if (e != 0) {
        err = e;
        goto done;
    }
    r->state = kStateOpen;
    AtomicMaskClear(&r->flags, kFlagOpened);
    AtomicMaskClear(&r->flags, kFlagBusy);
    r->deadline = 0;
done:
    if (err != 0)
        ResolverCloseNBP(r);
    return err;
}

int32_t ResolverControlNBP(void *ref, NetLookup *lookup)
{
    NBPResolver *r;

    r = ref;
    IsMPPOpen();
    AddressComposeEmpty(&lookup->address);
    QueueInsert(&r->requests, &lookup->link);
    ResolverNextNBP(r, 0);
    return 0;
}

void ResolverCloseNBP(NBPResolver *r)
{
    IsMPPOpen();
    SystemCheckCallback();
    while (r->pb.NBP.ioResult == 1)
        ResolverTickleNBP(r);
    r->state = 0;
    AtomicMaskClear(&r->flags, kFlagOpened);
    AtomicMaskClear(&r->flags, kFlagBusy);
    QueueDispose(&r->requests);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventClosed);
}

static void ResolverNextNBP(NBPResolver *r, unsigned char start)
{
    Str32 object;
    Str32 type;
    Str32 zone;

    IsMPPOpen();
    if (!start && !QueueIsEmpty(&r->requests) && !AtomicMaskApply(&r->flags, kFlagBusy))
        start = 1;
    if (start && (r->current = (NetLookup *)NetworkQueuePoll(&r->requests)) != 0) {
        if (r->current->name[0] != 0) {
            ULong i;
            unsigned char *p;
            char c;
            unsigned char n;

            AtomicMaskApply(&r->flags, kFlagForward);
            {
                const unsigned char *from;

                from = (const unsigned char *)r->current->name - 1;
                p = (unsigned char *)r->name - 1;
                while ((*++p = *++from) != 0)
                    ;
            }
            p = object;
            n = c = 0;
            i = 0;
            while ((c = r->name[i]) != 0 && c != ':') {
                c = r->name[i++];
                n++;
                *++p = c;
            }
            object[0] = n;
            if (r->name[i] == ':')
                i++;
            p = type;
            n = 0;
            while ((c = r->name[i]) != 0 && c != '@') {
                c = r->name[i++];
                n++;
                *++p = c;
            }
            type[0] = n;
            if (r->name[i] == '@')
                i++;
            p = zone;
            n = 0;
            while (r->name[i] != 0) {
                c = r->name[i++];
                n++;
                *++p = c;
            }
            zone[0] = n;
            NBPSetEntity((Ptr)&r->entity, object, type, zone);
            MemoryClear(&r->pb, sizeof(r->pb));
            r->pb.NBP.ioCompletion = sResolverDoneNBP;
            r->pb.NBP.interval = 8;
            r->pb.NBP.count = 4;
            r->pb.NBP.nbpPtrs.entityPtr = (Ptr)&r->entity;
            r->pb.NBP.parm.Lookup.retBuffPtr = r->buffer;
            r->pb.NBP.parm.Lookup.retBuffSize = r->bufferSize - 2;
            r->pb.NBP.parm.Lookup.maxToGet = r->wanted;
            r->pb.NBP.parm.Lookup.numGotten = 0;
            MemoryClear(r->buffer, 2);
            {
                ULong now;
                ULong wait;

                now = TickCount();
                wait = kNameLong;
                if (r->pb.NBP.parm.Lookup.maxToGet == 1)
                    wait = kNameShort;
                r->deadline = wait + now;
            }
            {
                int32_t e;

                e = PLookupName(&r->pb, true);
                if (e != 0) {
                    r->pb.NBP.ioResult = e;
                    ResolverDoneNBP(&r->pb);
                }
            }
        } else {
            AtomicMaskClear(&r->flags, kFlagForward);
            AddressTranscribe(&r->current->address, r->current->name);
            r->pb.NBP.ioResult = 0;
            ResolverDoneNBP(&r->pb);
        }
    } else if (start) {
        AtomicMaskClear(&r->flags, kFlagBusy);
        ResolverNextNBP(r, 0);
    }
}

static void ResolverDoneNBP(MPPPBPtr pb)
{
    NBPResolver *r;
    unsigned char posted;
    NetLookup *lookup;
    AddrBlock addr;
    char *name;
    int32_t n;
    char *to;
    ULong i;

    posted = 0;
    lookup = 0;
    r = ((NBPResolver **)pb)[-1];
    IsMPPOpen();
    SystemEnterCallback(1);
    if (r->pb.NBP.ioResult == kNoReply)
        r->pb.NBP.ioResult = 0;
    if (r->pb.NBP.ioResult == 0 && AtomicMaskTest(&r->flags, kFlagForward)) {
        for (i = 0; i < r->pb.NBP.parm.Lookup.numGotten; i++) {
            lookup = !posted ? r->current : (NetLookup *)NetworkQueuePoll(r->freeList);
            if (lookup == 0 && !AtomicMaskApply(&r->flags, kFlagOpened)) {
                NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventNeedBuffers);
                lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            }
            AtomicMaskClear(&r->flags, kFlagOpened);
            if (lookup == 0)
                break;
            if ((r->pb.NBP.ioResult = NBPExtract(r->buffer, r->pb.NBP.parm.Lookup.numGotten, i + 1, (EntityName *)r->entity, &addr)) != 0)
                break;
            AddressComposeAppleTalk(&lookup->address, addr.aNet, addr.aNode, addr.aSocket, 0);
            to = lookup->name;
            if ((n = r->entity[0]) != 0)
                BlockMoveData(&r->entity[1], to, n);
            to[r->entity[0]] = ':';
            name = &r->entity[0x22];
            to += r->entity[0] + 1;
            if ((n = name[0]) != 0)
                BlockMoveData(name + 1, to, n);
            if (name[name[0] + 1] != 0 && (name[0x22] != 1 || name[0x23] != '*')) {
                to[name[0]] = '@';
                to += name[0] + 1;
                name += 0x22;
                if ((n = name[0]) != 0)
                    BlockMoveData(name + 1, to, n);
            }
            to[name[0]] = 0;
            if (!posted) {
                r->current = 0;
                posted = 1;
            }
            QueueInsert(r->doneList, &lookup->link);
            lookup = 0;
            NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
        }
    }
    if (!posted && (!AtomicMaskTest(&r->flags, kFlagForward) || r->pb.NBP.ioResult != 0 || r->pb.NBP.parm.Lookup.numGotten == 0)) {
        QueueInsert(r->doneList, &r->current->link);
        r->current = 0;
        NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
    } else if (lookup != 0) {
        QueueInsert(r->freeList, &lookup->link);
    }
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventLookupDone);
    r->deadline = 0;
    ResolverNextNBP(r, 1);
    SystemLeaveCallback(1);
}

#endif
