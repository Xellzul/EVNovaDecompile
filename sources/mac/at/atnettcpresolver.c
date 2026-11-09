/* atnettcpresolver.c -- 1.0.10: TCP/IP without Open Transport: MacTCP's name resolver. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <MacTCP.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON
#include "AddressXlation.h"

#define kResolverBytes      0x268
#define kFlagDeferred       2
#define kFlagNoBuffer       4
#define kFlagBusy           6
#define kFlagReady          16
#define kFlagForward        17
#define kOpenFailed         -7102
#define kCacheFault         -23042
#define kEventResolved      8
#define kEventLookupDone    9
#define kEventNeedBuffers   14

/* the resolver on MacTCP's name resolver */
typedef struct DNRResolver {
    struct DNRResolver *next;
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
    hostInfo info;
    MutexList requests;
    ULong wanted;
    NetLookup *current;
    ULong ip;
    unsigned short port;
    char name[0x102];
} DNRResolver;

static void ResolverNextMacTCP(DNRResolver *r, unsigned char start);

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
    long n;
    const unsigned char *p;

    p = (const unsigned char *)s - 1;
    n = -1;
    do {
        n++;
    } while (*++p != 0);
    return n;
}
static pascal void ResolverDoneMacTCP(hostInfo *info, char *userData);
static RoutineDescriptor sResolverDoneDesc = BUILD_ROUTINE_DESCRIPTOR(kPascalStackBased | STACK_ROUTINE_PARAMETER(1, kFourByteCode) | STACK_ROUTINE_PARAMETER(2, kFourByteCode), ResolverDoneMacTCP);
static UniversalProcPtr sResolverDone = &sResolverDoneDesc;
#pragma explicit_zero_data on
static short sResolverOpens = 0;
#pragma explicit_zero_data reset

NetResolver *ResolverCreateMacTCP(NetStackID stack, void *ref, NetProc proc, MutexList *doneList, MutexList *freeList)
{
    DNRResolver *r;

    r = MemoryAllocate(kResolverBytes);
    if (r == 0)
        goto done;
    MemoryClear(r, kResolverBytes);
    r->kind = 1;
    r->stack = 0;
    r->state = 0;
    r->proc = proc;
    r->ref = ref;
    r->flags = 0;
    r->freeList = freeList;
    r->doneList = doneList;
    r->unk30 = 0;
    r->current = 0;
    r->wanted = 1;
    if (CriticalCreate(r->lock) != 0)
        goto done;
    _EndpointRegister((NetSocket *)r, 1);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventCreated);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventClosed);
done:
    return (NetResolver *)r;
}

void ResolverTickleMacTCP(DNRResolver *r)
{
    long state;

    state = r->state;
    if (state != 0)
        NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventConnected);
    if (AtomicMaskTest(&r->flags, kFlagDeferred) && AtomicMaskClear(&r->flags, kFlagReady))
        ResolverNextMacTCP(r, 0);
}

void ResolverDisposeMacTCP(DNRResolver *r)
{
    ResolverCloseMacTCP(r);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventDisposed);
    CriticalDispose(r->lock);
    _EndpointUnregister((NetSocket *)r, 1);
    MemoryDeallocate(r);
}

long ResolverOpenMacTCP(DNRResolver *r)
{
    long err, e;

    err = 0;
    e = QueueCreate(&r->requests);
    if (e != 0) {
        err = e;
        goto done;
    }
    AtomicMaskApply(&r->flags, kFlagReady);
    AtomicMaskClear(&r->flags, kFlagDeferred);
    AtomicMaskClear(&r->flags, kFlagNoBuffer);
    AtomicMaskClear(&r->flags, kFlagBusy);
    r->current = 0;
    if (sResolverOpens++ == 0) {
        if (OpenResolver(0) != 0)
            err = kOpenFailed;
        else
            r->state = kStateOpen;
    }
done:
    if (err != 0)
        ResolverCloseMacTCP(r);
    return err;
}

long ResolverControlMacTCP(void *ref, NetLookup *lookup)
{
    DNRResolver *r;

    r = ref;
    AddressComposeEmpty(&lookup->address);
    QueueInsert(&r->requests, &lookup->link);
    if (AtomicMaskTest(&r->flags, kFlagReady))
        AtomicMaskApply(&r->flags, kFlagDeferred);
    else
        ResolverNextMacTCP(r, 0);
    return 0;
}

void ResolverCloseMacTCP(DNRResolver *r)
{
    long state;

    SystemCheckCallback();
    state = r->state;
    if (state != 0) {
        if (--sResolverOpens == 0)
            CloseResolver();
    }
    r->state = 0;
    AtomicMaskClear(&r->flags, kFlagDeferred);
    AtomicMaskClear(&r->flags, kFlagNoBuffer);
    AtomicMaskClear(&r->flags, kFlagBusy);
    QueueDispose(&r->requests);
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventClosed);
}

static void ResolverNextMacTCP(DNRResolver *r, unsigned char start)
{
    if (!start && !QueueIsEmpty(&r->requests) && !AtomicMaskApply(&r->flags, kFlagBusy))
        start = 1;
    if (start && (r->current = (NetLookup *)NetworkQueuePoll(&r->requests)) != 0) {
        long e;

        if (r->current->name[0] != 0) {
            char c;
            ULong i;

            AtomicMaskApply(&r->flags, kFlagForward);
            r->ip = 0;
            r->port = 0;
            MslCopy(r->name, r->current->name);
            i = 0;
            while ((c = r->name[i]) != 0 && c != ':' && c != ',' && c != ' ')
                i++;
            if (r->name[i] != 0) {
                r->name[i++] = 0;
                r->port = StringToNumber(&r->name[i]);
            }
            e = StrToAddr(r->name, &r->info, sResolverDone, (char *)r);
            if (e != 0 && e == kCacheFault)
                goto done;
            r->info.rtnCode = e;
            ResolverDoneMacTCP(&r->info, (char *)r);
        } else {
            AtomicMaskClear(&r->flags, kFlagForward);
            AddressDecomposeTCPIP(&r->current->address, &r->port, &r->ip);
            e = AddrToName(r->ip, &r->info, sResolverDone, (char *)r);
            if (e != 0 && e == kCacheFault)
                goto done;
            r->info.rtnCode = e;
            ResolverDoneMacTCP(&r->info, (char *)r);
        }
    } else if (start) {
        AtomicMaskClear(&r->flags, kFlagDeferred);
        AtomicMaskClear(&r->flags, kFlagBusy);
        ResolverNextMacTCP(r, 0);
    }
done:
    return;
}

static pascal void ResolverDoneMacTCP(hostInfo *info, char *userData)
{
    DNRResolver *r;
    NetLookup *lookup;
    ULong i;

    r = (DNRResolver *)userData;
    SystemEnterCallback(1);
    if (info->rtnCode == 0 && AtomicMaskTest(&r->flags, kFlagForward)) {
        AddressComposeTCPIP(&r->current->address, r->port, info->addr[0]);
        AtomicMaskClear(&r->flags, kFlagReady);
        QueueInsert(r->doneList, &r->current->link);
        r->current = 0;
        NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
        for (i = 1; i < r->wanted && i < NUM_ALT_ADDRS && info->addr[i] != 0; i++) {
            lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            if (lookup == 0 && !AtomicMaskApply(&r->flags, kFlagNoBuffer)) {
                NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventNeedBuffers);
                lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            }
            AtomicMaskClear(&r->flags, kFlagNoBuffer);
            if (lookup == 0)
                break;
            MslCopy(lookup->name, r->name);
            AddressComposeTCPIP(&lookup->address, r->port, info->addr[i]);
            QueueInsert(r->doneList, &lookup->link);
            NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
        }
    } else {
        if (info->rtnCode == 0) {
            ULong n;

            MslCopy(r->current->name, info->cname);
            n = MslLength(r->current->name);
            if (n != 0 && r->current->name[n - 1] == '.')
                r->current->name[n - 1] = 0;
        }
        AtomicMaskClear(&r->flags, kFlagReady);
        QueueInsert(r->doneList, &r->current->link);
        r->current = 0;
        NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventResolved);
    }
    NetworkMakeCallback(r->proc, r->ref, (NetSocket *)r, kEventLookupDone);
    ResolverNextMacTCP(r, 1);
    SystemLeaveCallback(1);
}


#endif
