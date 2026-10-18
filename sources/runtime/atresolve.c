// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atresolve.c -- the runtime block's host-name resolver: a network object of kind 1 whose thread runs gethostbyname / gethostbyaddr on queued lookups. Notes: Research/notes/reggie.md */

#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "macport.h"
#include "win32.h"

#define kResolver           0x54
#define kKindResolver       1
#define kEventResolved      8
#define kEventLookupDone    9
#define kPortSeparators     " ,:"
#define kNameBytes          0x100

/* Placement only: no reader in the exe. */
static unsigned char sResolverPad[8];            /* 0x0086d4c0 */

#define NOTIFY_RESOLVER(r, event) \
    NetworkMakeCallback((r)->proc, (r)->ref, (struct NetSocket *)(r), (event))

ULong __stdcall _ResolverThread(void *param);
void _ResolverPostAnswer(NetResolver *r, NetLookup *lookup, HostEnt *host, const char *name,
                  unsigned short port);

/* ResolverCreate (a new resolver). STATE: MATCH 100%, exe 59 / obj 59, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540ba3
NetResolver *ResolverCreate(void *unused, void *ref, NetProc proc, MutexList *doneList,
                          MutexList *freeList)
{
    NetResolver *r;
    long err;

    err = 0;
    r = 0;
    r = MemoryAllocate(kResolver);
    if (r == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(r, 0, kResolver);
    r->kind = kKindResolver;
    r->unk6 = 0;
    r->state = 0;
    r->proc = proc;
    r->ref = ref;
    r->freeList = freeList;
    r->doneList = doneList;
    r->wanted = 1;
    if (CriticalCreate(r->lock) == 0) {
        _EndpointRegister((struct NetSocket *)r, kListResolver);
        NOTIFY_RESOLVER(r, kEventCreated);
        NOTIFY_RESOLVER(r, kEventClosed);
    }
done:
    return r;
}

/* ResolverTickle (report a live network object as connected). STATE: MATCH 100%, exe 13 /
   obj 13, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540c41
void ResolverTickle(NetSocket *s)
{
    long err;

    err = 0;
    switch (s->state) {
    case 0:
        break;
    default:
        NetworkMakeCallback(s->proc, s->ref, s, kEventConnected);
        break;
    }
}

/* ResolverDispose (dispose of a resolver). STATE: MATCH 100%, exe 28 / obj 28,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540c63
void ResolverDispose(void *ref)
{
    NetResolver *r;
    long err;

    r = (NetResolver *)ref;
    err = 0;
    ResolverClose(ref);
    NOTIFY_RESOLVER(r, kEventDisposed);
    CriticalDispose(r->lock);
    _EndpointUnregister((struct NetSocket *)r, kListResolver);
    MemoryDeallocate(r);
}

/* ResolverOpen (start a resolver's thread). STATE: MATCH 100%, exe 47 / obj 47,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540ca3
long ResolverOpen(void *ref)
{
    NetResolver *r;
    ULong threadId;
    long err, e;

    r = (NetResolver *)ref;
    err = 0;
    threadId = 0;
    e = QueueCreate(&r->requests);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((r->thread = CreateThread(0, kThreadStack, _ResolverThread, r, CREATE_SUSPENDED,
                                  &threadId)) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (SetThreadPriority(r->thread, THREAD_PRIORITY_ABOVE_NORMAL) == 0) {
        err = GetLastError();
        err = kThreadFailed;
        goto done;
    }
    r->state = kStateOpen;
    r->wanted = 1;
    r->waiting = 1;
done:
    return err;
}

/* ResolverControl (look a name up: a dotted address completes at once, anything else goes to
   the thread). STATE: MATCH 100%, exe 48 / obj 48, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540d2f
long ResolverControl(void *ref, NetLookup *lookup)
{
    NetResolver *r;
    long err;

    r = (NetResolver *)ref;
    err = 0;
    AddressComposeEmpty(&lookup->address);
    if (_AddressFromString(&lookup->address, lookup->name)) {
        _ResolverPostAnswer(r, lookup, 0, 0, 0);
    } else {
        AtomicIncrement(&r->pending, 1);
        QueueInsert(&r->requests, &lookup->link);
        lookup = 0;
        _PlatformResumeThread(r->thread);
    }
    return 0;
}

/* ResolverClose (close a resolver: stop its thread, hand back what is queued). STATE:
   MATCH 100%, exe 68 / obj 68, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540d9a
void ResolverClose(void *ref)
{
    NetResolver *r;
    long err;

    r = (NetResolver *)ref;
    err = 0;
    SystemCheckCallback();
    r->state = 0;
    r->wanted = 1;
    r->waiting = 0;
    if (r->thread != 0) {
        r->quit = 1;
        while (r->done == 0) {
            _PlatformResumeThread(r->thread);
            if (r->done == 0)
                SleepEx(kSleepMs, 1);
        }
        CriticalEnter(r->lock);
        if (CloseHandle(r->thread) == 0)
            err = GetLastError();
        if (r->current != 0)
            QueueInsert(r->doneList, &r->current->link);
        r->thread = 0;
        r->current = 0;
        CriticalLeave(r->lock);
    }
    QueueTransfer(r->doneList, &r->requests);
    QueueDispose(&r->requests);
    NOTIFY_RESOLVER(r, kEventClosed);
}

/* _ResolverPostAnswer (post a lookup's answer: none, every address of a forward lookup up to
   `wanted' (the extra ones in buffers from freeList), or the name of a reverse one with
   ":port"). STATE: MATCH 100%, exe 225 / obj 225, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00540e55
void _ResolverPostAnswer(NetResolver *r, NetLookup *lookup, HostEnt *host, const char *name,
                  unsigned short port)
{
    ULong i;
    ULong netPort;
    unsigned char asked;
    char portText[kNameBytes];
    ULong ip;
    long err;

    err = 0;
    asked = 0;
    CriticalEnter(r->lock);
    if (host == 0) {
        QueueInsert(r->doneList, &lookup->link);
        lookup = 0;
        NOTIFY_RESOLVER(r, kEventResolved);
        NOTIFY_RESOLVER(r, kEventLookupDone);
    } else if (lookup->name[0] != 0) {
        memmove(&ip, host->addrList[0], sizeof(ip));
        AddressComposeTCPIP(&lookup->address, netPort = Swap16(port), ip);
        QueueInsert(r->doneList, &lookup->link);
        lookup = 0;
        NOTIFY_RESOLVER(r, kEventResolved);
        for (i = 1; i < r->wanted && host != 0 && host->addrList[i] != 0; i++) {
            lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            if (lookup == 0 && asked == 0) {
                asked = 1;
                NOTIFY_RESOLVER(r, kEventNeedBuffers);
                lookup = (NetLookup *)NetworkQueuePoll(r->freeList);
            }
            if (lookup == 0)
                break;
            asked = 0;
            CopyString(lookup->name, name);
            memmove(&ip, host->addrList[i], sizeof(ip));
            AddressComposeTCPIP(&lookup->address, netPort, ip);
            QueueInsert(r->doneList, &lookup->link);
            lookup = 0;
            NOTIFY_RESOLVER(r, kEventResolved);
        }
        NOTIFY_RESOLVER(r, kEventLookupDone);
    } else {
        CopyString(lookup->name, host->name);
        if (port != 0) {
            StringFromNumber(portText, port);
            StringAppendSafe((unsigned char *)lookup->name, (const unsigned char *)":", kNameBytes);
            StringAppendSafe((unsigned char *)lookup->name, (const unsigned char *)portText,
                      kNameBytes);
        }
        QueueInsert(r->doneList, &lookup->link);
        NOTIFY_RESOLVER(r, kEventResolved);
        NOTIFY_RESOLVER(r, kEventLookupDone);
    }
    CriticalLeave(r->lock);
}

/* _ResolverThread (the resolver thread: "host[ ,:port]" forward, an empty name reverse).
   STATE: MATCH 100%, exe 146 / obj 146, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00541124
ULong __stdcall _ResolverThread(void *param)
{
    NetResolver *r;
    HostEnt *entry;
    char *separator;
    long err;

    r = param;
    err = 0;
    entry = 0;
    while (r->quit == 0) {
        while (r->quit == 0 &&
               (r->current = (NetLookup *)QueueRemove(&r->requests)) != 0) {
            char text[kNameBytes] = "";
            char host[kNameBytes] = "";
            ULong ip;
            unsigned short port;

            separator = 0;
            ip = 0;
            port = 0;
            if (r->current->name[0] != 0) {
                StringCopySafe(text, r->current->name, kNameBytes);
                StringCopySafe(host, r->current->name, kNameBytes);
                separator = StringFindDelimiter(host, kPortSeparators);
                if (separator != 0) {
                    *separator = 0;
                    port = StringToNumber(separator + 1);
                }
                entry = gethostbyname(host);
                if (entry == 0)
                    err = _EndpointGetError((NetSocket *)r);
            } else {
                AddressDecomposeTCPIP(&r->current->address, &port, &ip);
                entry = gethostbyaddr(&ip, sizeof(ip), AF_INET);
                if (entry == 0)
                    err = _EndpointGetError((NetSocket *)r);
                port = Swap16(port);
            }
            CriticalEnter(r->lock);
            if (r->pending != 0)
                AtomicIncrement(&r->pending, -1);
            _ResolverPostAnswer(r, r->current, entry, text, port);
            r->current = 0;
            CriticalLeave(r->lock);
        }
        if (r->quit == 0)
            _PlatformSuspendThread(GetCurrentThread());
    }
    r->done = 1;
    return 0;
}
