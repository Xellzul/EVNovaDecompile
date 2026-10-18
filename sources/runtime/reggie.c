// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: REGGIE, Ambrosia's Windows-only client over AmbrosiaTools' network layer, a library of its own: AmbrosiaTools' compiler and intrinsics line (it shares that layer's structs) with debug information on (-g), as libpng's line:the exe opens 13 REGGIE bodies with one zero copied into two dead locals (`xor eax,eax / mov ecx,eax / mov edi,eax'), a prologue found nowhere else in AmbrosiaTools and emitted by -g and by no spelling on the plain line; -g loses MATCHes in every AmbrosiaTools file and none in REGGIE; build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* reggie.c -- Ambrosia's REGGIE client (the version check against tracker.ambrosia.net, sessions, lookups, NAT probing and peer matching). Notes: Research/notes/reggie.md */

#include <stdarg.h>
#include <string.h>

#include "burgertypes.h"
#include "macport.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "reggiesession.h"
#include "win32.h"

GlobalEntry *gVersionList;                     /* 0x008701ac list head */

void _ReggieServerOffline(SessionContext *ctx, const char *reason);
long ReggieLookupTimer(SessionContext *ctx, SessionEntry *entry);

#define kProductEntry   0x108
#define kReplyFlags     8                        /* any of ! : < = > ~ separates */
#define kIdleSeconds    10

NetSocket *gReggiePrivateRef;                       /* 0x00870944 the tracker socket */
MutexList gReggiePrivateRecvQ;                      /* 0x0086d680 packets received */
MutexList gReggiePrivateFreeQ;                          /* 0x0086d690 packets free */
NetAddress gReggiePrivateAddress;                      /* 0x008705e4 the tracker's address */
ULong gReggiePrivateStamp;                       /* 0x0087005c last tracker traffic */
SessionContext *gReggieList;                       /* 0x0086ffa0 the sessions */
unsigned char gReggieOpen;                       /* 0x00870e07 network hooks installed */
unsigned char gReggieBusy;                       /* 0x00870e03 reset counting off */
static unsigned char pollRunning = 0;            /* 0x008674a0 a poll is running */
unsigned char (*gReggieSharedProc)(NetSocket *s); /* 0x008707c4 */
unsigned char (*gReggiePacketProc)(NetSocket *s, const char *data, ULong length,
                               const NetAddress *from);      /* 0x008707fc */
unsigned char (*gReggiePrejoinProc)(NetSocket *s, const NetAddress *local,
                                  const NetAddress *from);   /* 0x00870dec */
unsigned char (*gReggieSendFailedProc)(NetSocket *s, const NetAddress *from); /* 0x00870928 */
long (*gNT_MatchedProc)(NetSocket *s, const NetAddress *peer);  /* 0x0086ffac MATCH hook */
long (*gNT_InvitedProc)(NetSocket *s, const NetAddress *peer); /* 0x0086fffc INVITE hook */

#define kNatNone        0
#define kNatUnknown     1
#define kNatRestrictive 2
#define kNatConsistent  3
#define kNatProtective  4
#define kNatPermissive  5
#define kNatDirect      6
#define kWordBytes      0x100
#define kTextBytes      0x100
#define kOwnAddresses   10
#define kLoopbackHost   0x7f000001
#define kLengthPrefix   2
#define kNoFrame        0xffffffff

#define ReggieAssert(cond)  ((void)0)

typedef struct NatRoute {
    struct NatRoute *next;
    unsigned short localPort;
    unsigned short port;
    ULong ip;
    ULong natType;
    ULong seenAt;
    unsigned char tcp;
} NatRoute;

NatRoute *gMappingList;                          /* 0x00870174 the NAT routes seen */
#define kNameBuffer kSessionNameLen
#define kZlibTag            0x7a6c6962
#define kPunchNone      0
#define kPunchDirect    1
#define kPunchGuess     2
#define kLineBytes          0x1000

NatRoute *_ReggieFindMapping(unsigned short port, ULong ip, unsigned char tcp);
long ReggieOpenTracker(void);
long _ReggieSendHash(SessionContext *ctx, struct HashTable *t);
long _ReggieSendString(SessionContext *ctx, const char *fmt, ...);
long _ReggieSendIndirect(SessionContext *ctx, void **h);
SessionEntry *_ReggieMatchChannel(SessionContext *ctx, long kind, unsigned char tcp, const NetAddress *local, const NetAddress *remote);
long _ReggieMatchBegin(void *ref, long kind, unsigned char tcp, const NetAddress *local, const NetAddress *remote);
long _ReggieProbeEnd(SessionContext *ctx, SessionEntry *entry);
void _ReggieProbeUpdate(SessionContext *ctx, SessionEntry *entry, const NetAddress *from, const char *seen);
ULong ReggieMeetMethod(ULong nat, ULong peerNat);
ULong ReggieProbeCheck(void *ref, unsigned char tcp, const NetAddress *local, NetAddress *out);
SessionEntry *_ReggieMessageChannel(SessionContext *ctx);
long ReggieSendPermissions(SessionContext *ctx, SessionEntry *entry);
long _ReggieSearchEnd(SessionContext *ctx, SessionEntry *entry, unsigned char cancel);
ULong ReggieProbePrejoin(SessionContext *ctx, const NetAddress *address);
long _ReggieInsertEnd(SessionContext *ctx, SessionEntry *entry, unsigned char cancel);
long ReggieInsertBegin(void *ref, ULong slot, long mode, struct HashTable *record);
long _ReggieChannelClose(SessionContext *ctx, SessionEntry *entry, unsigned char cancel);
long ReggieRawPacket(void *ref, const char *data, ULong length, const NetAddress *from);
long _ReggiePacketExtract(void **h, struct HashTable **out);
long _ReggiePacketDecode(char *data, ULong length, struct ZState *z, unsigned char binary, void ***out);
long ReggiePacket(SessionContext *ctx, MutexNode *node);
long _ReggieOpen(void);

#define kStateIdle          0
#define kStateClosed        (-1)
#define kStateConnecting    (-2)
#define kStateHello         (-3)
#define kStateHelloSent     (-4)
#define kStateOnline        (-5)
#define kClosedError        -7110
#define kWrongFamily        -7001
#define kWrongEntry         -7002
#define kEventState         0
#define kEventOnline        1
#define kEventInserted      2
#define kEventSearched      3
#define kEventNotified      6
#define kEventMessage       7
#define kEventNat           9
#define kEventMatch         10
#define kEventInvite        11
#define kInsertHost         0
#define kInsertScore        1
#define kInsertData         2
#define kListPackets        2
#define kListChunks         3
#define kPacketBytes        0x800
#define kChunkBytes         0x1000
#define kOptionIpTtl        0x16000
#define kPunchTtl           2
#define kProbeReply         "REGGIE RESULT PROBE"
#define kTrackerHost        "tracker.ambrosia.net:42024"
#define kLookupFormat       "REGGIE LOOKUP CXID=%s HOSTID=%lu RECORD=%s"
#define kLookupsFormat      "REGGIE LOOKUP CXID=%s HOSTID=%lu RECORDS='%s'"
#define kCancelFormat       "REGGIE CANCEL CXID=%s HOSTID=%lu"

long ReggieOpenTracker(void);
long _ReggieOpen(void);
long _ReggieSendHash(SessionContext *ctx, struct HashTable *t);
long _ReggieSendString(SessionContext *ctx, const char *fmt, ...);
long _ReggieSendIndirect(SessionContext *ctx, void **h);
void _ReggieProbeUpdate(SessionContext *ctx, SessionEntry *entry, const NetAddress *from,
                  const char *seen);
SessionEntry *_ReggieMatchChannel(SessionContext *ctx, long kind, unsigned char tcp,
                           const NetAddress *local, const NetAddress *remote);
long _ReggieMatchBegin(void *ref, long kind, unsigned char tcp, const NetAddress *local,
                  const NetAddress *remote);
ULong ReggieProbeCheck(void *ref, unsigned char tcp, const NetAddress *local,
                   NetAddress *out);
SessionEntry *_ReggieMessageChannel(SessionContext *ctx);
long ReggieSendPermissions(SessionContext *ctx, SessionEntry *entry);
long _ReggieSearchEnd(SessionContext *ctx, SessionEntry *entry, unsigned char cancel);
long _ReggieInsertEnd(SessionContext *ctx, SessionEntry *entry, unsigned char cancel);
long ReggieInsertBegin(void *ref, ULong slot, long mode, struct HashTable *record);
long _ReggieChannelClose(SessionContext *ctx, SessionEntry *entry, unsigned char cancel);
long ReggieRawPacket(void *ref, const char *data, ULong length,
                  const NetAddress *from);
long ReggiePacket(SessionContext *ctx, MutexNode *node);

/* The owner's callback, with the entry's slot (0 without one). */
static inline void Notify(SessionContext *ctx, SessionEntry *entry, long event)
{
    long err;

    err = 0;
    if (ctx->proc != 0) {
        ctx->inCallback = 1;
        ctx->proc(ctx, ctx->procRef, entry != 0 ? entry->slot : 0, event);
        ctx->inCallback = 0;
    }
}

/* A reply field read as a number: 0 when it is missing or empty. */
static inline long TableNumber(struct HashTable *t, const char *key)
{
    const char *s;
    ULong value;

    s = 0;
    value = 0;
    s = HashLookup(t, key, 0);
    if (s != 0 && *s != 0)
        value = StringToNumber(s);
    return value;
}

/* The tracker socket and its two packet lists, torn down. */
static inline void CloseTracker(void)
{
    long err;

    err = 0;
    if (gReggiePrivateRef != 0) {
        DatagramDispose(gReggiePrivateRef);
        gReggiePrivateRef = 0;
        NetworkQueueEmpty(&gReggiePrivateFreeQ);
        NetworkQueueEmpty(&gReggiePrivateRecvQ);
    }
    QueueDispose(&gReggiePrivateFreeQ);
    QueueDispose(&gReggiePrivateRecvQ);
}

#define kNeverAsked         1
#define kMinAsk             3
#define kMaxAsk             12
#define kAskSpan            6
#define kTableKind          8
#define kEventRecord        4
#define kEventRecordsDone   5

#define kEventVersion       8
#define kGreetingLen        6
#define kZlibHeaderLen      2

static inline void StripOtherKeys(struct HashTable *t)
{
    char *key;
    char *value;
    ULong i;
    long err;

    err = 0;
    for (i = 0; HashIndex(t, i, &key, &value) != 0; i++) {
        if (*key != '$') {
            HashRemove(t, key);
            i--;
        }
    }
}

static inline struct HashTable *PopRequest(void **h)
{
    struct HashTable **tables;
    struct HashTable *next;
    ULong n;
    long err;
    long e;

    err = 0;
    next = 0;
    n = IndirectGetSize(h) / sizeof(struct HashTable *);
    if (n != 0) {
        tables = *h;
        next = tables[0];
        MemoryMove(tables, tables + 1, (n - 1) * sizeof(struct HashTable *));
        ((struct HashTable **)*h)[n - 1] = 0;
        e = IndirectSetSize(h, (n - 1) * sizeof(struct HashTable *));
        if (e != 0)
            err = e;
    }
    return next;
}

static inline long AppendTable(void **list, struct HashTable *table)
{
    long err;
    long e;

    err = 0;
    e = IndirectAppendData(list, &table, sizeof(table));
    if (e != 0)
        err = e;
    return err;
}

static inline void StripDollarKeys(struct HashTable *t)
{
    char *key;
    char *value;
    ULong i;
    long err;

    err = 0;
    for (i = 0; HashIndex(t, i, &key, &value) != 0; i++) {
        if (*key == '$') {
            HashRemove(t, key);
            i--;
        }
    }
}

/* How to reach a peer: our NAT type and the peer's, judged together. */
static inline ULong PeerMethod(SessionContext *ctx, const NetAddress **local,
                               const NetAddress **from)
{
    SessionContext *found;
    SessionContext **c;
    const NetAddress **l;
    const NetAddress **f;
    ULong nat, peerNat;
    long err;
    ULong a, b;

    peerNat = kNatUnknown;
    nat = kNatUnknown;
    found = ctx;
    err = 0;
    a = 0;
    b = 0;
    c = &found;
    l = local;
    f = from;
    nat = ReggieProbeCheck(found, 0, *local, 0);
    peerNat = ReggieProbePrejoin(found, *from);
    return ReggieMeetMethod(nat, peerNat);
}

#define kPunchRelay     4

static inline SessionEntry *FindDirect(SessionContext *ctx, unsigned short port,
                                       unsigned char tcp)
{
    SessionEntry *entry;
    long err;

    err = 0;
    entry = 0;
    for (entry = ctx->entries; entry != 0; entry = entry->next)
        if (entry->kind == kEntryKindDirect && entry->u.direct.localPort == port &&
            entry->u.direct.tcp == tcp)
            return entry;
    return 0;
}

static inline long SendCancel(SessionContext *ctx, SessionEntry *entry)
{
    long err;
    long e;

    err = 0;
    entry->counter++;
    e = _ReggieSendString(ctx, kCancelFormat,
                     StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu", entry->slot,
                                  entry->counter),
                     ctx->hostSource);
    if (e != 0) {
        err = e;
    } else {
        entry->cancelledAt = TimerGetElapsedSeconds();
        ctx->sentAt = entry->cancelledAt;
        entry->requestedAt = 0;
        entry->retries = 0;
    }
    return err;
}

static inline long DropNamed(SessionEntry **entry)
{
    long err = 0;
    struct HashTable *product;

    if ((*entry)->request != 0) {
        HashDispose((*entry)->request);
        (*entry)->request = 0;
    }
    product = (*entry)->u.named.product;
    if (product != 0) {
        HashDispose(product);
        (*entry)->u.named.product = 0;
    }
    (*entry)->u.named.name[0] = 0;
    return err;
}

static inline long DropPeer(SessionContext **ctx, SessionEntry **entry)
{
    long err = 0;
    SessionContext **c = ctx;
    SessionEntry **en = entry;

    return err;
}

static inline long PollPeer(SessionContext **ctx, SessionEntry **entry)
{
    long err;
    SessionEntry **en;
    SessionContext **c;

    err = 0;
    c = ctx;
    en = entry;
    return err;
}

static inline long PollHandle(SessionContext *ctx, SessionEntry *entry)
{
    long err = 0;
    SessionContext **c = &ctx;
    SessionEntry **en = &entry;

    return err;
}

static inline long PollIdle(SessionContext **ctx, SessionEntry **entry)
{
    SessionEntry *en;
    long err;
    SessionContext *c;

    en = *entry;
    c = *ctx;
    err = 0;
    return err;
}

static inline long CheckInstance(SessionContext *ctx, struct HashTable *table)
{
    long err = 0;
    const char *s;

    s = HashLookup(table, "$INSTANCE", 0);
    if (s != 0 && *s != 0 && ctx->instance[0] != 0 && StringCompare(s, ctx->instance) != 0) {
        _ReggieServerOffline(ctx, "Server has been restarted");
        ctx->stateAt = 1;
        ctx->receivedAt = ctx->stateAt;
    }
    return err;
}
#define kRequestKind 8

/* _ReggieNetworkFromString (a NAT type from the server's word; 1 when unknown). STATE: MATCH 100%, exe 71
   / obj 71, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f374f
ULong _ReggieNetworkFromString(void *unused, const char *text)
{
    char word[kWordBytes];
    ULong type;
    long err;

    err = 0;
    type = kNatUnknown;
    if (text == 0) {
        type = kNatUnknown;
    } else {
        String2UpperCopySafe(word, text, sizeof(word));
        if (StringCompare(word, "RESTRICTIVE") == 0)
            type = kNatRestrictive;
        else if (StringCompare(word, "CONSISTENT") == 0)
            type = kNatConsistent;
        else if (StringCompare(word, "PROTECTIVE") == 0)
            type = kNatProtective;
        else if (StringCompare(word, "PERMISSIVE") == 0)
            type = kNatPermissive;
        else if (StringCompare(word, "DIRECT") == 0)
            type = kNatDirect;
    }
    return type;
}

/* Stand-in: the linker strips this; the exe keeps its literals (body not recoverable). */
void NatNameList(char *text)
{
    StringFindDelimiter(text, ":, ");
    StringCopy(text, "Restrictive");
    StringCopy(text, "Consistent");
    StringCopy(text, "Protective");
    StringCopy(text, "Permissive");
    StringCopy(text, "Direct");
}

/* ReggieDescribeConnection (describe this machine's connection: its public address, its local
   addresses and its NAT type, as text). STATE: MATCH 100%, exe 239 / obj 239, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3817
unsigned char ReggieDescribeConnection(SessionContext *ctx, const char *host, char *publicText,
                           char *localText, char *natText)
{
    NetAddress address;
    char text[kTextBytes];
    ULong ip;
    unsigned short port;
    NetAddress own[kOwnAddresses];
    NatRoute *route;
    ULong i;
    ULong count;
    long err, e;
    long ok;

    err = 0;
    ok = 1;
    route = 0;
    ip = 0;
    port = 0;
    if (publicText != 0)
        *publicText = 0;
    if (localText != 0)
        *localText = 0;
    if (natText != 0)
        *natText = 0;
    if (host != 0 && *host != 0) {
        e = SimpleResolve(0, host, &address);
        if (e != 0) {
            err = e;
            goto done;
        }
        AddressDecomposeTCPIP(&address, &port, &ip);
        route = _ReggieFindMapping(port, ip, 0);
        if (route == 0)
            route = _ReggieFindMapping(port, ip, 1);
    } else if (ctx->mode == kModeShared) {
        e = DatagramGetAddress(ctx->socket, &address);
        if (e != 0) {
            err = e;
            goto done;
        }
        AddressDecomposeTCPIP(&address, &port, 0);
        route = _ReggieFindMapping(port, ip, 0);
    }
    if (route == 0)
        goto done;
    if (publicText != 0 && ctx->mode != kModeShared && route->natType >= kNatConsistent) {
        AddressComposeTCPIP(&address, route->port, route->ip);
        AddressTranscribe(&address, publicText);
    }
    if (localText != 0) {
        e = DatagramGetAddress(ctx->socket, &address);
        if (e != 0) {
            err = e;
            goto done;
        }
        AddressDecomposeTCPIP(&address, &port, &ip);
        if (ip != 0) {
            AddressTranscribe(&address, localText);
        } else {
            count = NetworkStackGetAddressList(0, own, kOwnAddresses);
            for (i = 0; i < count; i++) {
                AddressDecomposeTCPIP(&own[i], 0, &ip);
                if ((long long)Swap32(ip) != kLoopbackHost) {
                    AddressComposeTCPIP(&address, port, ip);
                    AddressTranscribe(&address, text);
                    if (*localText != 0)
                        StringAppendSafe((unsigned char *)localText, (const unsigned char *)", ",
                                  kTextBytes);
                    StringAppendSafe((unsigned char *)localText, (const unsigned char *)text,
                              kTextBytes);
                }
            }
        }
    }
    if (natText != 0) {
        switch (route->natType) {
        case kNatNone:
        case kNatUnknown:
            break;
        case kNatRestrictive:
            StringCopy(natText, "Restrictive");
            break;
        case kNatConsistent:
            StringCopy(natText, "Consistent");
            break;
        case kNatProtective:
            StringCopy(natText, "Protective");
            break;
        case kNatPermissive:
            StringCopy(natText, "Permissive");
            break;
        case kNatDirect:
            StringCopy(natText, "Direct");
            break;
        }
    }
done:
    return route != 0 ? true : false;
}

/* _ReggieFindMapping (the route for a local port, or for a public address and port; an address of
   this machine counts as local). STATE: MATCH 100%, exe 72 / obj 72, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x004f3b4c
NatRoute *_ReggieFindMapping(unsigned short port, ULong ip, unsigned char tcp)
{
    NetAddress own[kOwnAddresses];
    NatRoute *route;
    ULong addr;
    ULong i;
    ULong count;
    long err;

    err = 0;
    route = 0;
    if (ip == 0) {
        for (route = gMappingList; route != 0; route = route->next)
            if (port == route->localPort)
                return route;
    }
    if (ip != 0) {
        count = NetworkStackGetAddressList(0, own, kOwnAddresses);
        for (i = 0; i < count; i++) {
            AddressDecomposeTCPIP(&own[i], 0, &addr);
            if (ip == addr) {
                for (route = gMappingList; route != 0; route = route->next)
                    if (port == route->localPort)
                        return route;
            }
        }
    }
    if (ip != 0) {
        for (route = gMappingList; route != 0; route = route->next)
            if (ip == route->ip && port == route->port)
                return route;
    }
    return route;
}

/* _ReggieAddMapping (record what the server saw of a local port: its public address when the NAT
   type is better than known, and the time). STATE: MATCH 100%, exe 67 / obj 67, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3c1f
NatRoute *_ReggieAddMapping(unsigned short localPort, unsigned short port, ULong ip,
                       ULong natType, unsigned char tcp)
{
    NatRoute *route;
    long err;

    err = 0;
    route = 0;
    route = _ReggieFindMapping(localPort, 0, tcp);
    if (route == 0) {
        route = _ReggieFindMapping(port, ip, tcp);
        if (route == 0) {
            route = MemoryAllocate(sizeof(NatRoute));
            if (route == 0) {
                err = kOutOfMemory;
                goto done;
            }
            MemoryClear(route, sizeof(NatRoute));
            route->next = gMappingList;
            gMappingList = route;
        }
    }
    route->localPort = localPort;
    if (route->natType < natType) {
        if (ip != 0) {
            route->port = port;
            route->ip = ip;
        } else {
            route->port = localPort;
        }
        route->natType = natType;
    }
    route->seenAt = TimerGetElapsedSeconds();
    route->tcp = tcp;
done:
    return route;
}

/* _ReggieHashTimestamp (shift every $TIME / $TIME$... value of a table by `seconds', forward or
   back, rounding a plain one to the minute; each entry is re-added at the end). STATE:
   MATCH 100%, exe 110 / obj 110, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3ccc
long _ReggieHashTimestamp(struct HashTable *t, unsigned char forward, long seconds)
{
    char *key;
    char *value;
    char number[kTextBytes];
    ULong v;
    long err, e;
    ULong count;
    char tag;

    err = 0;
    count = 0;
    while (HashIndex(t, count, 0, 0) != 0)
        count++;
    while (count--) {
        HashIndex(t, 0, &key, &value);
        tag = value[-1];
        if ((StringCompare(key, "$TIME") == 0 || StringCompareSafe(key, "$TIME$", 6) == 0) &&
            StringToNumber(value) != 0) {
            v = StringToNumber(value) + seconds * (forward != 0 ? 1 : -1);
            if (tag == 0 || tag == '=')
                v = (v + 30) / 60 * 60;
            value = StringFromNumber(number, v);
        }
        e = _HashInsertAppend(t, key, StringGetLength((unsigned char *)key), value,
                         StringGetLength((unsigned char *)value), tag, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        HashRemove(t, key);
    }
done:
    return err;
}

/* _ReggieFixProductName. STATE: MATCH 100%, exe 44 / obj 44, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3e0a
void _ReggieFixProductName(char *out, const char *src)
{
    char *start;
    ULong n;
    char c;

    start = out;
    while ((n = out - start) < kNameBuffer - 1 && (*out = *src++) != 0) {
        c = *out;
        if (c >= 'a' && c <= 'z') {
            *out += 'A' - 'a';
            out++;
        } else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            out++;
        } else {
            *out++ = '_';
        }
    }
    if (n == kNameBuffer - 1)
        *out = 0;
}

/* _ReggieBufferCallback (the tracker stream's frame proc: a 2-byte big-endian length prefix when the
   context says so, else a text line). STATE: MATCH 100%, exe 52 / obj 52, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f3e67
ULong _ReggieBufferCallback(long unk0, void *ref, NetChunk *chunk, unsigned char second)
{
    SessionContext *ctx;
    ULong length;
    unsigned char *data;
    ULong n;
    ULong i;

    ctx = ref;
    length = chunk->length;
    data = (unsigned char *)chunk->data;
    if (ctx->lengthFrames != 0) {
        if (length < kLengthPrefix)
            return kNoFrame;
        n = Swap16(*(unsigned short *)data) + kLengthPrefix;
        if (n > chunk->size)
            n = chunk->size;
        return n;
    }
    for (i = 0; i < length; i++) {
        if (data[i] == '\r') {
            if (i + 1 == length)
                return i + 1;
            if (data[i + 1] == '\n')
                return i += 2;
            return i + 1;
        }
        if (data[i] == '\n')
            return i + 1;
    }
    return kNoFrame;
}

/* _ReggieNetworkCallback (the tracker stream's event proc: peer closed, failed or aborted marks the
   stream lost; no CSV row). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f3ee8
void _ReggieNetworkCallback(NetSocket *s, void *ref, unsigned short event)
{
    SessionContext *ctx;

    ctx = (SessionContext *)ref;
    switch (event) {
    case kEventPeerClosed:
    case kEventFailed:
    case kEventAborted:
        ctx->streamLost = 1;
        break;
    }
}

/* ReggieRefused (the server refused us: close the transport, drop every entry, tell the owner).
   STATE: MATCH 100%, exe 62 / obj 62, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3f01
void ReggieRefused(SessionContext *ctx, const char *reason)
{
    SessionEntry *e;
    long err;

    err = 0;
    e = 0;
    if (reason == 0 || *reason == 0)
        reason = "Connection request refused";
    switch (ctx->mode) {
    case kModeShared:
        break;
    case kModeOwnUdp:
        DatagramClose(ctx->socket, 0);
        break;
    case kModeTcp:
        StreamClose(ctx->socket, 0);
        break;
    }
    ctx->state = kStateClosed;
    ctx->stateAt = 0;
    ctx->resets = 0;
    /* BUG: reads e->next after _ReggieChannelClose has freed e */
    for (e = ctx->entries; e != 0; e = e->next)
        _ReggieChannelClose(ctx, e, 0);
    Notify(ctx, 0, kEventState);
}

/* _ReggieServerOffline (drop the connection and start over: requests go back to waiting, pending
   cancels are dropped). STATE: MATCH 100%, exe 85 / obj 85, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3fb1
void _ReggieServerOffline(SessionContext *ctx, const char *reason)
{
    SessionEntry *e;
    long err;

    err = 0;
    e = 0;
    if (reason == 0 || *reason == 0)
        reason = "Lost connection to server";
    switch (ctx->mode) {
    case kModeShared:
        break;
    case kModeOwnUdp:
        DatagramClose(ctx->socket, 0);
        break;
    case kModeTcp:
        StreamClose(ctx->socket, 0);
        break;
    }
    ctx->state = kStateIdle;
    ctx->natType = 0;
    ctx->stateAt = TimerGetElapsedSeconds();
    AddressComposeEmpty(&ctx->probe);
    AddressComposeEmpty(&ctx->publicAddress);
    ctx->streamLost = 0;
    ctx->resets = 0;
    /* BUG: a dropped entry's next is read after _ReggieChannelClose has freed it */
    for (e = ctx->entries; e != 0; e = e->next) {
        if (e->cancelledAt != 0)
            _ReggieChannelClose(ctx, e, 0);
        else if (e->request != 0)
            _ReggieChannelPacket(ctx, e);
    }
    Notify(ctx, 0, kEventState);
}

/* ReggieOnHello (the server said HELLO: keep its instance, message of the day and
   addresses, our host id and the clock difference, then go online). STATE: MATCH 100%,
   exe 122 / obj 122, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f40ac
void ReggieOnHello(SessionContext *ctx, const char *instance, const char *motd,
                  const char *motdUrl, const char *probe, const char *address, ULong hostId,
                  ULong serverTime)
{
    SessionEntry *messages;
    long err;
    long e;
    const char *text;

    err = 0;
    e = 0;
    StringCopySafe(ctx->instance, instance != 0 ? instance : "", kSessionNameLen);
    StringCopySafe(ctx->motd, motd != 0 ? motd : "", kSessionNameLen);
    if (motdUrl != 0)
        text = motdUrl;
    else
        text = "";
    StringCopySafe(ctx->motdUrl, text, kSessionNameLen);
    if (probe != 0 && *probe != 0) {
        e = SimpleResolve(0, probe, &ctx->probe);
        if (e != 0)
            err = e;
    }
    if (address != 0 && *address != 0) {
        e = SimpleResolve(0, address, &ctx->publicAddress);
        if (e != 0)
            err = e;
    }
    if (serverTime != 0)
        ctx->clockOffset = TimerGetLocalSeconds() - serverTime;
    ctx->hostSource = hostId;
    _ReggieProbeUpdate(ctx, 0, &ctx->server, address);
    ctx->state = kStateOnline;
    ctx->resets = 0;
    messages = _ReggieMessageChannel(ctx);
    if (messages != 0) {
        e = ReggieSendPermissions(ctx, messages);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    Notify(ctx, 0, kEventOnline);
done:
    ;
}

/* _ReggieVersionInsert (add a product to the list, with its reply table or a new one holding
   $PRODUCT). STATE: MATCH 100%, exe 72 / obj 72, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f420b
GlobalEntry *_ReggieVersionInsert(const char *name, struct HashTable *reply)
{
    long err, e;
    GlobalEntry *entry;

    err = 0;
    entry = 0;
    entry = MemoryAllocate(kProductEntry);
    if (entry == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(entry, kProductEntry);
    if (reply != 0) {
        entry->payload = reply;
    } else {
        if ((entry->payload = HashCreate(kReplyFlags)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = HashInsert(entry->payload, "$PRODUCT", name);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    StringCopySafe(entry->name, name, kSessionNameLen);
    entry->next = gVersionList;
    gVersionList = entry;
done:
    if (err != 0 && entry != 0) {
        if (entry->payload != 0 && entry->payload != reply)
            HashDispose(entry->payload);
        MemoryDeallocate(entry);
        entry = 0;
    }
    return entry;
}

/* _ReggiePrivateTickle (take the tracker's VERSION results off the socket's queue into the product
   list; after kIdleSeconds without traffic, dispose of the socket). STATE: MATCH 100%, exe 206
   / obj 206, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f42cd
void _ReggiePrivateTickle(void)
{
    unsigned char removed;
    char *product;
    char *value;
    char *key;
    NetPacket *packet;
    struct HashTable *reply;
    const char *prefix;
    GlobalEntry **link;
    ULong i;
    GlobalEntry *entry;
    GlobalEntry *spare;
    long err;
    long e;
    GlobalEntry *found;

    err = 0;
    packet = 0;
    reply = 0;
    spare = found = 0;
    ReggieAssert(gVersionList == 0 || gVersionList->name[0] != 0);
    product = 0;
    e = 0;
    if (gReggiePrivateRef == 0)
        goto done;

    while ((packet = (NetPacket *)QueueRemove(&gReggiePrivateRecvQ)) != 0) {
        prefix = "REGGIE RESULT VERSION ";
        if (packet->length == packet->capacity)
            packet->length--;
        packet->data[packet->length] = 0;
        if (!AddressCompare(&packet->from, &gReggiePrivateAddress))
            goto next;
        if (StringCompareSafe(packet->data, prefix, StringGetLength((const unsigned char *)prefix)) != 0)
            goto next;
        if ((reply = HashInitialize(kReplyFlags, packet->data)) == 0) {
            err = kOutOfMemory;
            goto done;
        }

        entry = 0;
        for (i = 0; HashIndex(reply, i, &key, &value) != 0; i++) {
            if (key[0] != '$') {
                HashRemove(reply, key);
                i--;
            }
        }
        product = HashLookup(reply, "$PRODUCT", 0);
        if (product == 0)
            goto next;
        if (HashLookup(reply, "$VERSION", 0) == 0)
            goto next;

        e = 0;
        removed = 0;
        link = &gVersionList;
        while ((entry = *link) != 0) {
            if (StringCompare(product, (*link)->name) == 0) {
                if (entry->payload != 0)
                    HashDispose(entry->payload);
                *link = (*link)->next;
                MemoryDeallocate(entry);
                link = 0;
                removed = 1;
            }
            if (link != 0)
                link = &(*link)->next;
            else
                link = &gVersionList;
        }
        if (removed == 0)
            goto next;
        if (_ReggieVersionInsert(product, reply) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        reply = 0;
    next:
        if (packet != 0) {
            QueueInsert(&gReggiePrivateFreeQ, &packet->link);
            packet = 0;
        }
        if (reply != 0) {
            HashDispose(reply);
            reply = 0;
        }
    }

    if (TimerGetElapsedSeconds() - gReggiePrivateStamp >= kIdleSeconds) {
        CloseTracker();
        gReggiePrivateStamp = 0;
    }
done:
    if (packet != 0)
        QueueInsert(&gReggiePrivateFreeQ, &packet->link);
    if (reply != 0)
        HashDispose(reply);
}

/* ReggieBeginVersionDirect (ask the tracker for a product's version with no session: the product is
   listed afresh and one VERSION datagram goes out). STATE: MATCH 100%, exe 118 / obj 118,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4513
long ReggieBeginVersionDirect(const char *name)
{
    long err, e;
    NetPacket *packet;
    GlobalEntry **link;
    GlobalEntry *entry;
    unsigned char found;
    long result;

    err = 0;
    packet = 0;
    result = 0;
    found = false;
    link = &gVersionList;
    while ((entry = *link) != 0) {
        if (StringCompare(name, (*link)->name) == 0) {
            if (entry->payload != 0)
                HashDispose(entry->payload);
            *link = (*link)->next;
            MemoryDeallocate(entry);
            link = 0;
            found = true;
        }
        if (link != 0)
            link = &(*link)->next;
        else
            link = &gVersionList;
    }
    if (_ReggieVersionInsert(name, 0) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = ReggieOpenTracker();
    if (e != 0) {
        err = e;
        goto done;
    }
    packet = (NetPacket *)QueueRemove(&gReggiePrivateFreeQ);
    if (packet == 0) {
        e = NetworkQueueFill(&gReggiePrivateFreeQ, kListPackets, 4, kPacketBytes);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    /* BUG: takes a second packet even when the first succeeded, and the first is lost */
    packet = (NetPacket *)QueueRemove(&gReggiePrivateFreeQ);
    if (packet == 0) {
        err = kOutOfMemory;
        goto done;
    }
    AddressDuplicate(&packet->from, &gReggiePrivateAddress);
    StringFormatSafe(packet->data, packet->capacity, "REGGIE VERSION $PRODUCT='%s'", name);
    packet->length = StringGetLength((unsigned char *)packet->data);
    e = DatagramSend(gReggiePrivateRef, packet);
    if (e != 0) {
        err = e;
        goto done;
    }
    packet = 0;
    gReggiePrivateStamp = TimerGetElapsedSeconds();
done:
    if (packet != 0)
        QueueInsert(&gReggiePrivateFreeQ, &packet->link);
    return err;
}

/* ReggieOpenTracker (open the tracker socket once: network start-up, the tracker's address, the two
   packet lists and a UDP socket). STATE: MATCH 100%, exe 92 / obj 92, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f4650
long ReggieOpenTracker(void)
{
    long err, e;

    err = 0;
    if (gReggiePrivateRef != 0)
        goto done;
    e = _ReggieOpen();
    if (e != 0) {
        err = e;
        goto done;
    }
    MemoryClear(&gReggiePrivateFreeQ, sizeof(MutexList));
    MemoryClear(&gReggiePrivateRecvQ, sizeof(MutexList));
    e = SimpleResolve(0, kTrackerHost, &gReggiePrivateAddress);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = QueueCreate(&gReggiePrivateFreeQ);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = QueueCreate(&gReggiePrivateRecvQ);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = NetworkQueueFill(&gReggiePrivateFreeQ, kListPackets, 4, kPacketBytes);
    if (e != 0) {
        err = e;
        goto done;
    }
    gReggiePrivateRef = DatagramCreate(0, 0, 0, &gReggiePrivateFreeQ, &gReggiePrivateFreeQ, &gReggiePrivateRecvQ,
                                &gReggiePrivateRecvQ);
    if (gReggiePrivateRef == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DatagramOpen(gReggiePrivateRef, 0);
    if (e != 0)
        err = e;
done:
    if (err != 0) {
        CloseTracker();
    }
    return err;
}

/* _ReggieLookupDispose (drop a lookup entry's records). STATE: MATCH 100%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4774
void _ReggieLookupDispose(SessionContext *ctx, SessionEntry *entry)
{
    ULong i;
    long err;

    err = 0;
    if (entry->records != 0) {
        for (i = 0; i < entry->recordCount; i++) {
            if (entry->records[i].table != 0) {
                HashDispose(entry->records[i].table);
                entry->records[i].table = 0;
            }
        }
        MemoryDeallocate(entry->records);
        entry->records = 0;
    }
    entry->answered = 0xffffffff;
    entry->recordCount = 0;
    entry->wanted = 0;
    entry->lastAsked = 0;
    entry->received = 0;
}

/* ReggieStoreLookupRecord (a lookup record arrived: keep it, recount the settled ones -- no table, an
   empty one, or one with a second key -- and tell the owner, 4 per record, 5 when all are
   settled). STATE: MATCH 100%, exe 98 / obj 98, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f47f1
void ReggieStoreLookupRecord(SessionContext *ctx, SessionEntry *entry, struct HashTable *table,
                  ULong index)
{
    char *key;
    char *value;
    ULong i;
    long err;

    err = 0;
    if (entry->records[index].table != 0)
        HashDispose(entry->records[index].table);
    entry->records[index].table = table;
    entry->records[index].askedAt = 0;
    entry->received++;
    entry->updates++;
    entry->answered = 0;
    for (i = 0; i < entry->recordCount; i++) {
        if (entry->records[i].table == 0 || HashIsEmpty(entry->records[i].table) != 0)
            entry->answered++;
        else if (HashIndex(entry->records[i].table, 1, &key, &value) != 0)
            entry->answered++;
    }
    Notify(ctx, entry, kEventRecord);
    if (entry->answered == entry->recordCount)
        Notify(ctx, entry, kEventRecordsDone);
}

/* _ReggieLookupRequest (the RECORDS list of a lookup: the first never-asked run, at most six, then
   the longest-waiting runs asked more than three idle periods ago, until `max' (3..12)).
   STATE: MATCH 100%, exe 164 / obj 164, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4925
void _ReggieLookupRequest(SessionContext *ctx, SessionEntry *entry, ULong max, char *out)
{
    long err;
    ULong now;
    ULong timeout;
    ULong listed;
    ULong end;
    ULong best;
    ULong i;
    ULong n;
    ULong asked;

    err = 0;
    now = TimerGetTicks() | 1;
    timeout = TimerGetTickRate() * 3;
    listed = 0;
    *out = 0;
    if (entry->u.lookup.finished != 0)
        return;
    if (entry->answered == entry->wanted)
        return;
    if (ctx->mode == kModeNoExtras) {
        StringFormat(out, entry->recordCount == 1 ? "1" : "1-%lu", entry->recordCount);
        for (i = 0; i < entry->recordCount; i++)
            entry->records[i].askedAt = 0;
        return;
    }
    if (max < kMinAsk)
        max = kMinAsk;
    if (max > kMaxAsk)
        max = kMaxAsk;
    i = 0;
    end = entry->recordCount;
    for (; i < end; i++) {
        if (entry->records[i].askedAt == kNeverAsked) {
            if (end > i + kAskSpan)
                end = i + kAskSpan;
            best = end;
            StringFormat(out, i + 1 < end ? "%lu-%lu" : "%lu", i + 1, end);
            /* BUG: both runs count one record too many, and the second marks one past its end */
            listed += end - i + 1;
            end = i;
            break;
        }
    }
    do {
        i = best = 0;
        n = timeout;
        for (; i < end; i++) {
            asked = entry->records[i].askedAt;
            if (asked != 0 && now - asked >= n) {
                best = i;
                n = now - asked;
            }
        }
        if (n == timeout)
            return;
        for (n = best;
             n < end && entry->records[n].askedAt == entry->records[best].askedAt;
             n++)
            ;
        StringFormat(out + StringGetLength((unsigned char *)out),
                     best + 1 < n ? "%s%lu-%lu" : "%s%lu", *out != 0 ? "," : "",
                     best + 1, n);
        listed += n - best + 1;
        for (i = best; i < n + 1; i++)
            entry->records[i].askedAt = now;
    } while (listed < max);
}

/* ReggieLookupTimer (a lookup's resend timer: while online and records are unanswered, ask again
   for the ones waiting longest, after 3 idle periods -- a quarter once three have come in).
   STATE: MATCH 100%, exe 101 / obj 101, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4b1c
long ReggieLookupTimer(SessionContext *ctx, SessionEntry *entry)
{
    char ranges[kTextBytes];
    double factor;
    ULong timeout;
    ULong now;
    long err, e;

    err = 0;
    if (ctx->state != kStateOnline) {
        _ReggieLookupDispose(ctx, entry);
        goto done;
    }
    if (ctx->mode == kModeTcp || entry->records == 0 || entry->answered >= entry->recordCount)
        goto done;
    if (entry->received < 3)
        factor = 3.0;
    else
        factor = 0.25;
    timeout = (ULong)(TimerGetTickRate() * factor);
    now = TimerGetTicks();
    if (now - entry->lastAsked <= timeout)
        goto done;
    /* BUG: passes the received count as the RECORDS maximum */
    _ReggieLookupRequest(ctx, entry, entry->received, ranges);
    if (ranges[0] == 0)
        goto done;
    entry->received = 0;
    entry->lastAsked = now;
    e = _ReggieSendString(ctx, StringFindDelimiter(ranges, "-,") != 0 ? kLookupsFormat : kLookupFormat,
                     entry->idText, ctx->hostSource, ranges);
    if (e != 0) {
        err = e;
        goto done;
    }
    ctx->sentAt = TimerGetElapsedSeconds();
done:
    return err;
}

/* ReggieMakeLookupRecords (make a lookup entry's `count' records, each a table holding key=value). STATE:
   MATCH 100%, exe 79 / obj 79, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4c79
long ReggieMakeLookupRecords(SessionContext *ctx, SessionEntry *entry, const char *key,
                  const char *value, ULong count)
{
    long err, e;
    ULong i;

    err = 0;
    if (value == 0)
        value = "";
    _ReggieLookupDispose(ctx, entry);
    entry->wanted = count;
    entry->recordCount = count;
    entry->answered = 0;
    entry->lastAsked = 0;
    entry->received = 0;
    if ((entry->records = MemoryAllocate(count * sizeof(LookupRecord))) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(entry->records, count * sizeof(LookupRecord));
    for (i = 0; i < count; i++) {
        if ((entry->records[i].table = HashCreate(kTableKind)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (key != 0 && *key != 0) {
            e = HashInsert(entry->records[i].table, key, value);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        entry->records[i].askedAt = kNeverAsked;
    }
done:
    if (err != 0)
        _ReggieLookupDispose(ctx, entry);
    return err;
}

/* _ReggieSendHash (send a table: a copy with its $TIME keys moved to the server's clock,
   serialised). STATE: MATCH 100%, exe 56 / obj 56, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4d6b
long _ReggieSendHash(SessionContext *ctx, struct HashTable *t)
{
    long err, e;
    void **h;
    struct HashTable *copy;

    err = 0;
    h = 0;
    copy = 0;
    copy = HashClone(t);
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = _ReggieHashTimestamp(copy, 0, ctx->clockOffset);
    if (e != 0) {
        err = e;
        goto done;
    }
    h = HashSave(copy);
    if (h == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = _ReggieSendIndirect(ctx, h);
    if (e != 0)
        err = e;
done:
    if (h != 0)
        IndirectDeallocate(h);
    if (copy != 0)
        HashDispose(copy);
    return err;
}

/* _ReggieSendString (send a formatted request line). STATE: MATCH 100%, exe 56 / obj 56, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f4de4
long _ReggieSendString(SessionContext *ctx, const char *fmt, ...)
{
    char text[kLineBytes];
    va_list args;
    void **h;
    long err;
    long result;

    err = 0;
    h = 0;
    va_start(args, fmt);
    StringFormatVarg(text, sizeof(text), fmt, args);
    va_end(args);
    h = IndirectInitialize(text, StringGetLength((unsigned char *)text));
    if (h == 0) {
        err = kOutOfMemory;
    } else {
        result = _ReggieSendIndirect(ctx, h);
        if (result != 0)
            err = result;
    }
    if (h != 0)
        IndirectDeallocate(h);
    return err;
}

/* _ReggieSendIndirect (send a handle to the server: over TCP deflated behind a 2-byte big-endian
   length, or as a CR LF line; packed with the 'zlib' tag over UDP when the context frames by
   length; a failure drops the connection). STATE: MATCH 100%, exe 103 / obj 103,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4e7c
long _ReggieSendIndirect(SessionContext *ctx, void **h)
{
    unsigned short length;
    char text[kLineBytes];
    ULong size;
    long err, e;

    err = 0;
    size = IndirectGetSize(h);
    if (size < kLineBytes - 1) {
        /* BUG: copies 0xfff bytes from a handle shorter than that */
        MemoryCopy(text, *h, kLineBytes - 1);
        text[size] = 0;
    } else {
        StringCopySafe(text, *h, kLineBytes);
    }
    if (ctx->lengthFrames != 0) {
        if (ctx->mode == kModeTcp) {
            e = ProgressiveCompress(h, &ctx->deflater);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = (unsigned short)Swap16(IndirectGetSize(h));
            e = IndirectInsertData(h, &length, sizeof(length));
        } else {
            e = IndirectCompress(h, kZlibTag, 0);
        }
        if (e != 0) {
            err = e;
            goto done;
        }
    } else if (ctx->mode == kModeTcp) {
        e = IndirectAppendData(h, "\r\n", 2);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = SimpleSend(ctx->socket, &ctx->server, h);
    if (e != 0)
        err = e;
done:
    if (err != 0)
        _ReggieServerOffline(ctx, "Cannot send to server");
    return err;
}

/* _ReggieIdleCallback (the REGGIE idle proc: poll every session, then the tracker socket; no CSV
   row). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f4f9c
void _ReggieIdleCallback(void)
{
    SessionContext *ctx;
    SessionContext *next;
    long err;

    err = 0;
    for (ctx = gReggieList; ctx != 0; ctx = next) {
        next = ctx->next;
        ReggieTickle(ctx);
    }
    _ReggiePrivateTickle();
}

/* ReggieResultVersion (a VERSION reply for a product entry: only for the entry this product name
   finds first, only while its request is out and only for its CXID; the reply, stripped to
   its $ keys, becomes the product table -- an empty one when RESULT carries an error --
   and the owner hears event 8). STATE: MATCH 100%, exe 181 / obj 181, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f4fbe
long ReggieResultVersion(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    const char *cxid;
    const char *s;
    const char *version;
    long err;

    err = 0;
    s = 0;
    version = 0;
    if (FindNamed(ctx, entry->u.named.name) != entry || entry->requestedAt == 0)
        goto done;
    cxid = HashLookup(reply, "CXID", 0);
    if (cxid == 0 || *cxid == 0 || StringCompare(cxid, entry->idText) != 0)
        goto done;
    s = HashLookup(reply, "RESULT", 0);
    if (s != 0 && *s != 0) {
        s = version = "";
    } else {
        s = HashLookup(reply, "$PRODUCT", 0);
        if (s == 0 || *s == 0)
            goto done;
        version = HashLookup(reply, "$VERSION", 0);
        if (version == 0 || *version == 0)
            goto done;
    }
    if (entry->request != 0) {
        HashDispose(entry->request);
        entry->request = 0;
    }
    entry->requestedAt = 0;
    entry->cancelledAt = 0;
    StripOtherKeys(reply);
    /* BUG: overwrites the previous product table without freeing it */
    entry->u.named.product = (version != 0 && *version != 0) ? HashClone(reply)
                                                            : HashCreate(kTableKind);
    if (entry->u.named.product == 0) {
        err = kOutOfMemory;
        goto done;
    }
    Notify(ctx, 0, kEventVersion);
done:
    return err;
}

/* ReggieResultSearch (a SEARCH result: the lookup's record count; make the records and ask for the
   first ones). STATE: MATCH 100%, exe 222 / obj 222, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5219
long ReggieResultSearch(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    char kindName[kSessionNameLen];
    char ranges[kTextBytes];
    ULong count;
    long e;
    long err;
    const char *s;

    err = 0;
    s = 0;
    if (entry == 0 || entry->kind != kEntryKindLookup || entry->requestedAt == 0)
        goto done;
    s = HashLookup(reply, "CXID", 0);
    if (s == 0 || *s == 0 || StringCompare(s, entry->idText) != 0)
        goto done;
    s = HashLookup(reply, "RESULT", 0);
    if (s != 0 && *s != 0) {
        _ReggieSearchEnd(ctx, entry, 0);
        goto done;
    }
    if (HashLookup(reply, "$KIND", 0) == 0 || HashLookup(reply, "COUNT", 0) == 0)
        goto done;
    s = HashLookup(reply, "$KIND", 0);
    StringCopySafe(kindName, s, sizeof(kindName));
    if (StringCompare(s, entry->u.lookup.kindName) != 0)
        goto done;
    if (entry->request != 0) {
        HashDispose(entry->request);
        entry->request = 0;
    }
    entry->requestedAt = 0;
    entry->cancelledAt = 0;
    count = TableNumber(reply, "COUNT");
    if (entry->u.lookup.finished == 0) {
        e = ReggieMakeLookupRecords(ctx, entry, "$KIND", entry->u.lookup.kindName, count);
        if (e != 0) {
            err = e;
            goto done;
        }
        _ReggieLookupRequest(ctx, entry, kAskSpan, ranges);
    } else {
        entry->wanted = count;
    }
    Notify(ctx, entry, kEventSearched);
    if (entry->answered == entry->recordCount)
        Notify(ctx, entry, kEventRecordsDone);
    /* BUG: ranges is uninitialised when the lookup was already finished */
    if (ranges[0] != 0) {
        entry->lastAsked = TimerGetTicks();
        entry->received = 0;
        e = _ReggieSendString(ctx, StringFindDelimiter(ranges, "-,") != 0 ? kLookupsFormat : kLookupFormat,
                         entry->idText, ctx->hostSource, ranges);
        if (e != 0) {
            err = e;
            goto done;
        }
        ctx->sentAt = TimerGetElapsedSeconds();
    }
    entry->updates++;
done:
    if (err != 0 && entry != 0)
        _ReggieLookupDispose(ctx, entry);
    return err;
}

/* _ReggieRecv_RESULT_PROBE (a PROBE result: classify our NAT from what the server saw, or close the direct
   entry when it failed). STATE: MATCH 100%, exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f54f6
long _ReggieRecv_RESULT_PROBE(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply,
                  const NetAddress *from)
{
    const char *s;
    long err;

    err = 0;
    s = HashLookup(reply, "RESULT", 0);
    if (s != 0 && *s != 0) {
        if (entry != 0)
            _ReggieProbeEnd(ctx, entry);
    } else {
        _ReggieProbeUpdate(ctx, entry, from, HashLookup(reply, "$ADDRESS", 0));
        if (entry != 0) {
            if (entry->request != 0) {
                HashDispose(entry->request);
                entry->request = 0;
            }
            entry->requestedAt = 0;
            entry->cancelledAt = 0;
        }
    }
    return 0;
}

/* Stand-in: the linker strips this; the exe keeps its literals (body not recoverable). */
long InstanceResult(SessionContext *ctx, struct HashTable *reply)
{
    return CheckInstance(ctx, reply);
}

/* ReggieResultMessage (a MESSAGE result: send the next queued message). STATE: MATCH 100%, exe
   122 / obj 122, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5583
long ReggieResultMessage(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    const char *s;
    long result;
    long err;
    long e;

    err = 0;
    result = 0;
    if (entry == 0 || entry->kind != kEntryKindMessages || entry->requestedAt == 0)
        goto done;
    s = HashLookup(reply, "CXID", 0);
    if (s == 0 || *s == 0 || StringCompare(s, entry->idText) != 0)
        goto done;
    s = HashLookup(reply, "RESULT", 0);
    if (entry->request != 0) {
        HashDispose(entry->request);
        entry->request = 0;
    }
    entry->requestedAt = 0;
    entry->cancelledAt = 0;
    if ((entry->request = PopRequest(entry->u.messages.pending)) != 0) {
        entry->counter++;
        e = HashAppend(entry->request, "CXID",
                         StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu", entry->slot,
                                      entry->counter));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _ReggieChannelPacket(ctx, entry);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* ReggieResultMatch (a MATCH result: the request is done). STATE: MATCH 100%, exe 57 / obj 57,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f56ef
long ReggieResultMatch(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    const char *s;
    const char *result;
    long err;

    result = 0;
    err = 0;
    if (entry != 0 && entry->kind == kEntryKindMatch && entry->requestedAt != 0) {
        s = HashLookup(reply, "CXID", 0);
        if (s != 0 && *s != 0 && StringCompare(s, entry->idText) == 0) {
            s = HashLookup(reply, "RESULT", 0);
            if (entry->request != 0) {
                HashDispose(entry->request);
                entry->request = 0;
            }
            entry->requestedAt = 0;
            entry->cancelledAt = 0;
            _ReggieChannelClose(ctx, entry, 0);
        }
    }
    return 0;
}

/* _ReggieRecv_RESULT_LOOKUP (a LOOKUP result: one record, stripped to its $ keys and moved to our
   clock). STATE: MATCH 100%, exe 161 / obj 161, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f578e
long _ReggieRecv_RESULT_LOOKUP(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    char kindName[kSessionNameLen];
    struct HashTable *copy;
    const char *s;
    char *value;
    char *key;
    ULong index;
    ULong i;
    long err;
    long e;
    long result;

    err = 0;
    copy = 0;
    if (entry == 0 || entry->kind != kEntryKindLookup || entry->records == 0)
        goto done;
    s = HashLookup(reply, "CXID", 0);
    if (s == 0 || *s == 0 || StringCompare(s, entry->idText) != 0)
        goto done;
    s = HashLookup(reply, "RESULT", 0);
    if (s != 0 && *s != 0) {
        _ReggieSearchEnd(ctx, entry, 0);
        goto done;
    }
    if (HashLookup(reply, "$KIND", 0) == 0 || HashLookup(reply, "$UNIQUEID", 0) == 0)
        goto done;
    s = HashLookup(reply, "$KIND", 0);
    StringCopySafe(kindName, s, sizeof(kindName));
    if (StringCompare(kindName, entry->u.lookup.kindName) != 0)
        goto done;
    index = TableNumber(reply, "RECORD") - 1;
    if (index >= entry->recordCount)
        goto done;
    result = 0;
    for (i = 0; HashIndex(reply, i, &key, &value) != 0; i++) {
        if (*key != '$') {
            HashRemove(reply, key);
            i--;
        }
    }
    e = _ReggieHashTimestamp(reply, 1, ctx->clockOffset);
    if (e != 0) {
        err = e;
        goto done;
    }
    copy = HashClone(reply);
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }
    ReggieStoreLookupRecord(ctx, entry, copy, index);
    copy = 0;
done:
    if (copy != 0)
        HashDispose(copy);
    return err;
}

/* ReggieResultInvite (an INVITE result: the request is done). STATE: MATCH 100%, exe 57 / obj 57,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f59b2
long ReggieResultInvite(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    const char *s;
    const char *result;
    long err;

    result = 0;
    err = 0;
    if (entry != 0 && entry->kind == kEntryKindInvite && entry->requestedAt != 0) {
        s = HashLookup(reply, "CXID", 0);
        if (s != 0 && *s != 0 && StringCompare(s, entry->idText) == 0) {
            s = HashLookup(reply, "RESULT", 0);
            if (entry->request != 0) {
                HashDispose(entry->request);
                entry->request = 0;
            }
            entry->requestedAt = 0;
            entry->cancelledAt = 0;
            _ReggieChannelClose(ctx, entry, 0);
        }
    }
    return 0;
}

/* ReggieResultInsert (an INSERT result: tell the owner, or drop the insert on an error). STATE:
   MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5a51
long ReggieResultInsert(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    const char *s;
    long err;

    err = 0;
    if (entry != 0 && entry->kind == kEntryKindInsert && entry->requestedAt != 0) {
        s = HashLookup(reply, "CXID", 0);
        if (s != 0 && *s != 0 && StringCompare(s, entry->idText) == 0) {
            s = HashLookup(reply, "RESULT", 0);
            if (s != 0 && *s != 0) {
                _ReggieInsertEnd(ctx, entry, 0);
            } else {
                Notify(ctx, entry, kEventInserted);
                if (entry->request != 0) {
                    HashDispose(entry->request);
                    entry->request = 0;
                }
                entry->requestedAt = 0;
                entry->cancelledAt = 0;
            }
        }
    }
    return 0;
}

/* _ReggieRecv_RESULT_HELLO (a HELLO result: go online; on a shared UDP socket with no better NAT type
   known, send the probe server two probes). STATE: MATCH 100%, exe 149 / obj 149,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5b48
long _ReggieRecv_RESULT_HELLO(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    long err = 0;
    void *packet = 0;
    const char *s;
    long hostId;
    long e;
    long result;

    if (ctx->state != kStateHelloSent)
        goto done;
    s = HashLookup(reply, "RESULT", 0);
    if (s != 0 && *s != 0) {
        ReggieRefused(ctx, s);
        goto done;
    }
    hostId = TableNumber(reply, "$HOSTID");
    ReggieOnHello(ctx, HashLookup(reply, "$INSTANCE", 0), HashLookup(reply, "$MOTD", 0),
                 HashLookup(reply, "$MOTD$URL", 0), HashLookup(reply, "$PROBE", 0),
                 HashLookup(reply, "$ADDRESS", 0), hostId, TableNumber(reply, "$TIME"));
    if (ctx->natType <= kNatRestrictive && ctx->mode == kModeShared &&
        AddressIsEmpty(&ctx->probe) == 0) {
        packet = "REGGIE PROBE=UDP SIMPLE";
        packet = IndirectInitialize(packet, StringGetLength(packet));
        if (packet == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = SimpleSend(ctx->socket, &ctx->probe, packet);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = SimpleSend(ctx->socket, &ctx->probe, packet);
        if (e != 0)
            err = e;
    }
done:
    if (packet != 0)
        IndirectDeallocate(packet);
    result = err;
    return result;
}

/* ReggieResultCancel (a CANCEL result: the entry is dropped either way). STATE: MATCH 100%, exe 59 /
   obj 59, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5cec
long ReggieResultCancel(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    long err;
    const char *s;
    const char *result;

    err = 0;
    result = 0;
    if (entry != 0 && entry->cancelledAt != 0) {
        s = HashLookup(reply, "CXID", 0);
        if (s != 0 && *s != 0 && StringCompare(s, entry->idText) == 0) {
            s = HashLookup(reply, "RESULT", 0);
            if (s != 0 && *s != 0) {
                _ReggieChannelClose(ctx, entry, 0);
            } else {
                if (entry != 0 && entry->cancelledAt != 0)
                    _ReggieChannelClose(ctx, entry, 0);
            }
        }
    }
    return 0;
}

/* _ReggieRecv_RESULT_BLOCK (an ALLOW or BLOCK result: send the next queued permission). STATE: MATCH
   100%, exe 151 / obj 151, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5d7d
long _ReggieRecv_RESULT_BLOCK(SessionContext *ctx, SessionEntry *entry, struct HashTable *reply)
{
    char *key;
    char *s;
    unsigned char block;
    long result;
    long err;
    long e;

    err = 0;
    block = 0;
    result = 0;
    /* BUG: whether the answer was BLOCK is worked out and never used */
    block = HashIndex(reply, 0, &key, &s) != 0 && !StringCompare(key, "BLOCK");
    if (entry == 0 || entry->kind != kEntryKindMessages || entry->requestedAt == 0)
        goto done;
    s = HashLookup(reply, "CXID", 0);
    if (s == 0 || *s == 0 || StringCompare(s, entry->idText) != 0)
        goto done;
    s = HashLookup(reply, "RESULT", 0);
    if (entry->request != 0) {
        HashDispose(entry->request);
        entry->request = 0;
    }
    entry->requestedAt = 0;
    entry->cancelledAt = 0;
    if ((entry->request = PopRequest(entry->u.messages.pending)) != 0) {
        entry->counter++;
        e = HashInsert(entry->request, "CXID",
                         StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu", entry->slot,
                                      entry->counter));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _ReggieChannelPacket(ctx, entry);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* ReggieRequestPing (the server's PING: answer with an empty RESULT). STATE: MATCH 100%, exe 52 /
   obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5f33
long ReggieRequestPing(SessionContext *ctx, SessionEntry *entry, struct HashTable *request)
{
    long err, e;
    struct HashTable *reply;

    err = 0;
    reply = 0;
    reply = HashClone(request);
    if (reply == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = HashInsert(reply, "RESULT", "");
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(reply, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieSendHash(ctx, reply);
    if (e != 0) {
        err = e;
        goto done;
    }
    ctx->sentAt = TimerGetElapsedSeconds();
done:
    if (reply != 0)
        HashDispose(reply);
    return err;
}

/* _ReggieRecv_NOTIFY (the server's NOTIFY for a lookup: mark it, tell the owner, answer). STATE:
   MATCH 100%, exe 95 / obj 95, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f5fb8
long _ReggieRecv_NOTIFY(SessionContext *ctx, SessionEntry *entry, struct HashTable *request)
{
    struct HashTable *reply;
    const char *s;
    long err, e;

    err = 0;
    reply = 0;
    if (entry == 0 || entry->kind != kEntryKindLookup)
        goto done;
    s = HashLookup(request, "CXID", 0);
    if (s != 0 && *s != 0 && StringCompare(s, entry->idText) == 0) {
        entry->u.lookup.notified = 1;
        entry->updates++;
        Notify(ctx, entry, kEventNotified);
    }
    reply = HashClone(request);
    if (reply == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = HashInsert(reply, "RESULT", "");
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(reply, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieSendHash(ctx, reply);
    if (e != 0) {
        err = e;
        goto done;
    }
    ctx->sentAt = TimerGetElapsedSeconds();
done:
    if (reply != 0)
        HashDispose(reply);
    return err;
}

/* _ReggieRecv_MESSAGE (the server's MESSAGE: queue a copy -- once per CXID -- and tell the owner,
   then answer with the request stripped of its $ keys). STATE: MATCH 100%, exe 170 / obj
   170, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f60c6
long _ReggieRecv_MESSAGE(SessionContext *ctx, SessionEntry *entry, struct HashTable *request)
{
    long err;
    struct HashTable *copy;
    struct HashTable *reply;
    const char *cxid;
    long e;
    long result;

    err = 0;
    reply = 0;
    copy = 0;
    entry = _ReggieMessageChannel(ctx);
    if (entry == 0) {
        err = kOutOfMemory;
        goto done;
    }
    cxid = HashLookup(request, "CXID", 0);
    if (cxid == 0 || StringCompare(cxid, entry->u.messages.lastCxid) != 0) {
        copy = HashClone(request);
        if (copy == 0) {
            err = kOutOfMemory;
            goto done;
        }
        result = AppendTable(entry->u.messages.received, copy);
        if (result) {
            err = result;
            goto done;
        }
        /* BUG: a message with no CXID copies from a null pointer (exe 0x004f6185; the
           dereference at 0x004eb806) */
        StringCopySafe(entry->u.messages.lastCxid, cxid, kSessionNameLen);
        while (HashRemove(copy, "MESSAGE") != 0)
            ;
        while (HashRemove(copy, "CXID") != 0)
            ;
        copy = 0;
        Notify(ctx, 0, kEventMessage);
    }
    reply = HashClone(request);
    if (reply == 0) {
        err = kOutOfMemory;
        goto done;
    }
    StripDollarKeys(reply);
    e = HashInsert(reply, "RESULT", "");
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(reply, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieSendHash(ctx, reply);
    if (e != 0) {
        err = e;
        goto done;
    }
    ctx->sentAt = TimerGetElapsedSeconds();
done:
    if (reply != 0)
        HashDispose(reply);
    if (copy != 0)
        HashDispose(copy);
    return err;
}

/* _ReggieRecv_MATCH (the server's MATCH: a peer wants to meet us; record the incoming entry,
   answer, tell the owner and the MATCH hook). STATE: MATCH 100%, exe 129 / obj 129,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f62b4
long _ReggieRecv_MATCH(SessionContext *ctx, SessionEntry *entry, struct HashTable *request)
{
    long err;
    NetAddress target;
    NetAddress sender;
    struct HashTable *reply;
    const char *result;
    const char *s;
    long e;
    unsigned char ok;

    err = 0;
    reply = 0;
    result = "";
    ok = 0;
    s = HashLookup(request, "$SENDER", 0);
    if (s == 0 || SimpleResolve(0, s, &sender) != 0) {
        result = "Invalid $SENDER in MATCH packet";
        goto answer;
    }
    s = HashLookup(request, "$TARGET", 0);
    if (s == 0 || SimpleResolve(0, s, &target) != 0) {
        result = "Invalid $TARGET in MATCH packet";
        goto answer;
    }
    entry = _ReggieMatchChannel(ctx, kEntryKindMatch, 0, &target, &sender);
    if (entry == 0) {
        err = kOutOfMemory;
        goto done;
    }
answer:
    reply = HashClone(request);
    if (reply == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = HashInsert(reply, "RESULT", result);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(reply, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieSendHash(ctx, reply);
    if (e != 0) {
        err = e;
        goto done;
    }
    /* BUG: an invalid packet leaves entry the dispatcher's CXID entry, so this still fires */
    if (entry != 0)
        Notify(ctx, 0, kEventMatch);
    if (ctx->mode == kModeShared && gNT_MatchedProc != 0) {
        e = gNT_MatchedProc(ctx->socket, &sender);
        if (e != 0)
            err = e;
    }
done:
    if (reply != 0)
        HashDispose(reply);
    return err;
}

/* _ReggieRecv_INVITE (the server's INVITE: as MATCH, with $METHOD=TCP asking for a stream). STATE:
   MATCH 100%, exe 143 / obj 143, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f643e
long _ReggieRecv_INVITE(SessionContext *ctx, SessionEntry *entry, struct HashTable *request)
{
    const char *result;
    NetAddress target;
    NetAddress sender;
    long err;
    long e;
    struct HashTable *reply;
    const char *s;
    unsigned char tcp;

    err = 0;
    reply = 0;
    result = "";
    tcp = 0;
    s = HashLookup(request, "$SENDER", 0);
    if (s == 0 || SimpleResolve(0, s, &sender) != 0) {
        result = "Invalid $SENDER in INVITE packet";
        goto answer;
    }
    s = HashLookup(request, "$TARGET", 0);
    if (s == 0 || SimpleResolve(0, s, &target) != 0) {
        result = "Invalid $TARGET in INVITE packet";
        goto answer;
    }
    s = HashLookup(request, "$METHOD", 0);
    if (s != 0 && StringCompare(s, "TCP") == 0)
        tcp = 1;
    entry = _ReggieMatchChannel(ctx, kEntryKindInvite, tcp, &target, &sender);
    if (entry == 0) {
        err = kOutOfMemory;
        goto done;
    }
answer:
    reply = HashClone(request);
    if (reply == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = HashInsert(reply, "RESULT", result);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(reply, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieSendHash(ctx, reply);
    if (e != 0) {
        err = e;
        goto done;
    }
    /* BUG: an invalid packet leaves entry the dispatcher's CXID entry, so this still fires */
    if (entry != 0)
        Notify(ctx, 0, kEventInvite);
    if (ctx->mode == kModeShared && gNT_InvitedProc != 0) {
        e = gNT_InvitedProc(ctx->socket, &sender);
        if (e != 0)
            err = e;
    }
done:
    if (reply != 0)
        HashDispose(reply);
    return err;
}

/* _ReggieMatchChannel (record an incoming MATCH or INVITE: our address and the peer's). STATE: MATCH
   100%, exe 67 / obj 67, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f6601
SessionEntry *_ReggieMatchChannel(SessionContext *ctx, long kind, unsigned char tcp,
                           const NetAddress *local, const NetAddress *remote)
{
    SessionEntry *entry;
    ULong slot;
    long err, e;

    err = 0;
    entry = 0;
    e = ReggieChannelOpen(ctx, &slot);
    if (e != 0) {
        err = e;
        goto done;
    }
    entry = MemoryAllocate(kSessionEntry);
    if (entry == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(entry, kSessionEntry);
    entry->next = ctx->entries;
    ctx->entries = entry;
    entry->kind = kind;
    entry->slot = slot;
    entry->u.invite.incoming = 1;
    entry->u.invite.tcp = tcp;
    AddressDuplicate(&entry->u.invite.local, local);
    AddressDuplicate(&entry->u.invite.remote, remote);
done:
    if (err != 0 && entry != 0)
        _ReggieChannelClose(ctx, entry, 0);
    return entry;
}

/* _ReggieMatchBegin (ask the server to MATCH or INVITE a peer: an outgoing entry and its request;
   $METHOD only for MATCH). STATE: MATCH 100%, exe 197 / obj 197, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x004f66b7
long _ReggieMatchBegin(void *ref, long kind, unsigned char tcp, const NetAddress *local,
                  const NetAddress *remote)
{
    ULong slot;
    char text[kTextBytes];
    char hostText[kTextBytes];
    SessionContext *ctx;
    SessionEntry *entry;
    long err, e;
    long result;

    ctx = (SessionContext *)ref;
    err = 0;
    result = 0;
    entry = 0;
    slot = 0;
    if (ctx->mode == kModeShared && (local == 0 || AddressIsEmpty(local) != 0))
        local = &ctx->publicAddress;
    if (local != 0)
        AddressIsEmpty(local);
    if (remote != 0)
        AddressIsEmpty(remote);
    e = ReggieChannelOpen(ctx, &slot);
    if (e != 0) {
        err = e;
        goto done;
    }
    entry = MemoryAllocate(kSessionEntry);
    if (entry == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(entry, kSessionEntry);
    /* BUG: listed before the request is built, so a failure below leaves it on the list */
    entry->next = ctx->entries;
    ctx->entries = entry;
    entry->kind = kind;
    entry->slot = slot;
    entry->u.invite.incoming = 0;
    /* BUG: always tcp, whatever the caller asked for */
    entry->u.invite.tcp = 1;
    AddressDuplicate(&entry->u.invite.local, local);
    AddressDuplicate(&entry->u.invite.remote, remote);
    if ((entry->request = HashCreate(kTableKind)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (kind == kEntryKindMatch) {
        e = HashInsert(entry->request, "$METHOD", tcp != 0 ? "TCP" : "UDP");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    AddressTranscribe(remote, text);
    e = HashInsert(entry->request, "$TARGET", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    AddressTranscribe(local, text);
    e = HashInsert(entry->request, "$SENDER", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp && ctx->hostSource != 0) {
        ULong host = ctx->hostSource;
        struct HashTable *t = entry->request;

        e = HashInsert(t, "HOSTID", StringFromNumber(hostText, host));
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    entry->counter++;
    e = HashInsert(entry->request, "CXID",
                     StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu", entry->slot,
                                  entry->counter));
    if (e != 0) {
        err = e;
        goto done;
    }
    e = HashInsert(entry->request, kind == kEntryKindMatch ? "MATCH" : "INVITE", "REQUEST");
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(entry->request, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieChannelPacket(ctx, entry);
    if (e != 0)
        err = e;
done:
    return err;
}

/* _ReggieProbeEnd (close a direct entry's socket, UDP or TCP, and empty its two lists). STATE:
   MATCH 100%, exe 48 / obj 48, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f6940
long _ReggieProbeEnd(SessionContext *ctx, SessionEntry *entry)
{
    NetSocket *s;
    long err;

    err = 0;
    s = entry->u.direct.socket;
    if (s != 0) {
        if (entry->u.direct.tcp != 0) {
            StreamClose(s, 0);
            StreamDispose(entry->u.direct.socket);
        } else {
            DatagramClose(s, 0);
            DatagramDispose(entry->u.direct.socket);
        }
        entry->u.direct.socket = 0;
        QueueEmpty(&entry->u.direct.queue2);
        QueueDispose(&entry->u.direct.queue2);
        QueueEmpty(&entry->u.direct.queue);
        QueueDispose(&entry->u.direct.queue);
    }
    entry->u.direct.startedAt = 0;
    return 0;
}

/* _ReggieReinsert (our address changed: insert every HOST record again). STATE: MATCH 100%, exe
   51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f69d1
long _ReggieReinsert(SessionContext *ctx)
{
    long err, e;
    struct HashTable *copy;
    SessionEntry *entry;

    err = 0;
    copy = 0;
    entry = 0;
    for (entry = ctx->entries; entry != 0; entry = entry->next) {
        if (entry->kind == kEntryKindInsert && entry->insertMode == kInsertHost) {
            copy = HashClone(entry->u.lookup.query);
            if (copy == 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = ReggieInsertBegin(ctx, entry->slot, entry->insertMode, copy);
            if (e != 0) {
                err = e;
                goto done;
            }
            HashDispose(copy);
            copy = 0;
        }
    }
done:
    if (copy != 0)
        HashDispose(copy);
    return err;
}

/* _ReggieProbeUpdate (classify our NAT from where an answer came from and the address the server
   saw; a better type is recorded as a route and the owner told). STATE: MATCH 100%, exe 258 /
   obj 258, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f6a47
void _ReggieProbeUpdate(SessionContext *ctx, SessionEntry *entry, const NetAddress *from,
                  const char *seen)
{
    long err = 0;
    ULong count;
    unsigned short serverPort = 0, fromPort = 0;
    ULong serverIp = 0, fromIp = 0;
    NetAddress seenAddress;
    NetAddress seenHost;
    NetAddress own[kOwnAddresses];
    ULong ip;
    NetAddress remote;
    NetAddress local;
    ULong nat;
    ULong old;
    ULong i;
    long e;
    long ok;

    nat = kNatUnknown;
    ok = 1;
    ReggieAssert(from != 0);
    AddressDecomposeTCPIP(&ctx->server, &serverPort, &serverIp);
    AddressDecomposeTCPIP(from, &fromPort, &fromIp);
    if (seen != 0) {
        e = SimpleResolve(0, seen, &seenAddress);
        if (e != 0)
            err = e;
        if (err != 0)
            seen = 0;
        err = 0;
    }
    if (AddressCompare(&ctx->probe, from) != 0) {
        nat = kNatRestrictive;
        if (seen != 0 &&
            AddressCompare(entry != 0 ? &entry->u.direct.peer : &ctx->publicAddress,
                         &seenAddress) != 0) {
            if ((entry != 0 && entry->u.direct.portMapped != 0) ||
                (entry == 0 && ctx->portMapped != 0))
                nat = kNatProtective;
            else
                nat = kNatConsistent;
        }
    } else if (fromIp != serverIp) {
        nat = kNatPermissive;
        if (seen != 0) {
            AddressDecomposeTCPIP(&seenAddress, 0, &ip);
            AddressComposeTCPIP(&seenHost, 0, ip);
            count = NetworkStackGetAddressList(0, own, kOwnAddresses);
            for (i = 0; i < count; i++)
                if (AddressCompare(&seenHost, &own[i]) != 0)
                    break;
            if (i < count)
                nat = kNatDirect;
        }
    } else if (fromPort != serverPort) {
        nat = kNatRestrictive;
        if (entry != 0)
            entry->u.direct.portMapped = 1;
        else
            ctx->portMapped = 1;
    } else {
        nat = kNatRestrictive;
        if (seen != 0)
            AddressDuplicate(entry != 0 ? &entry->u.direct.peer : &ctx->publicAddress,
                         &seenAddress);
    }
    if (entry != 0) {
        old = entry->u.direct.natType;
        if (nat > old)
            entry->u.direct.natType = nat;
    } else {
        old = ctx->natType;
        if (nat > old)
            ctx->natType = nat;
    }
    if (nat > old) {
        unsigned short localPort;
        unsigned short seenPort;
        ULong seenIp;

        localPort = 0;
        seenPort = 0;
        seenIp = 0;
        if (entry != 0) {
            localPort = entry->u.direct.localPort;
        } else if (ctx->mode == kModeTcp) {
            StreamGetAddress((NetStream *)ctx->socket, &local, &remote);
            AddressDecomposeTCPIP(&local, &localPort, 0);
        } else {
            DatagramGetAddress(ctx->socket, &local);
            AddressDecomposeTCPIP(&local, &localPort, 0);
        }
        AddressDecomposeTCPIP(&seenAddress, &seenPort, &seenIp);
        if (_ReggieAddMapping(localPort, seenPort, seenIp, nat, 0) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = _ReggieReinsert(ctx);
        if (e != 0) {
            err = e;
            goto done;
        }
        Notify(ctx, entry, kEventNat);
    }
done:
    return;
}

/* ReggiePollDirect (a direct entry's poll: UDP answers are read as PROBE results; a TCP connection
   that arrived is taken as DIRECT; after five idle periods the attempt gives up; either way
   the route is recorded and the socket closed). STATE: MATCH 100%, exe 205 / obj 205,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f6dc1
long ReggiePollDirect(SessionContext *ctx, SessionEntry *entry)
{
    NetPacket *packet;
    void **message;
    struct HashTable *table;
    unsigned short localPort;
    unsigned short port;
    ULong ip;
    ULong period;
    unsigned char timedOut;
    long err;
    long result;

    err = 0;
    timedOut = 0;
    message = 0;
    table = 0;
    packet = 0;
    if (entry->u.direct.socket == 0)
        goto done;
    if (entry->u.direct.tcp == 0 && QueueIsEmpty(&entry->u.direct.queue2) == 0) {
        while ((packet = (NetPacket *)QueueRemove(&entry->u.direct.queue2)) != 0) {
            result = _ReggiePacketDecode(packet->data, packet->length, 0, 1, &message);
            if (result != 0) {
                err = result;
                goto done;
            }
            result = _ReggiePacketExtract(message, &table);
            if (result != 0) {
                err = result;
                goto done;
            }
            result = _ReggieRecv_RESULT_PROBE(ctx, entry, table, &packet->from);
            if (result != 0) {
                err = result;
                goto done;
            }
            QueueInsert(&entry->u.direct.queue, &packet->link);
            packet = 0;
            IndirectDeallocate(message);
            message = 0;
            HashDispose(table);
            table = 0;
        }
    } else if (entry->u.direct.tcp != 0 && (int)EndpointGetStatus(entry->u.direct.socket) == 2) {
        result = StreamAcceptServer((NetStream *)entry->u.direct.socket, 0);
        if (result != 0)
            err = result;
        entry->u.direct.natType = kNatDirect;
        Notify(ctx, entry, kEventNat);
    } else {
        period = TimerGetTickRate();
        period *= 5;
        if (TimerGetTicks() - entry->u.direct.startedAt >= period) {
            if (entry->u.direct.natType < kNatRestrictive)
                entry->u.direct.natType = kNatRestrictive;
            timedOut = 1;
            Notify(ctx, entry, kEventNat);
        }
    }
    if (timedOut != 0 || entry->u.direct.natType == kNatDirect) {
        if (entry->u.direct.tcp != 0) {
            localPort = 0;
            port = 0;
            ip = 0;
            localPort = entry->u.direct.localPort;
            AddressDecomposeTCPIP(&entry->u.direct.peer, &port, &ip);
            if (_ReggieAddMapping(localPort, port, ip, entry->u.direct.natType, 1) == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        _ReggieProbeEnd(ctx, entry);
    }
done:
    if (packet != 0)
        QueueInsert(&entry->u.direct.queue, &packet->link);
    if (message != 0)
        IndirectDeallocate(message);
    if (table != 0)
        HashDispose(table);
    return err;
}

/* _ReggieProbePrejoin (the shared socket's hook for a peer's first packet: by the two NAT types,
   answer with a low-TTL PING and INVITE it, or MATCH it; no CSV row for the hook table).
   STATE: MATCH 100%, exe 157 / obj 157, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f705c
unsigned char _ReggieProbePrejoin(NetSocket *s, const NetAddress *local, const NetAddress *from)
{
    NetAddress mine;
    SessionContext *ctx;
    ULong ttl;
    ULong method;
    void **h;
    unsigned char handled;
    long err;
    long e;
    const char *text;
    const NetAddress *peer;
    const NetAddress *peer2;

    method = kPunchNone;
    err = 0;
    ctx = 0;
    h = 0;
    handled = 0;
    for (ctx = gReggieList; ctx != 0; ctx = ctx->next) {
        if (ctx->socket == s) {
            method = kPunchNone;
            method = PeerMethod(ctx, &local, &from);
            break;
        }
    }
    if (method == kPunchGuess) {
        text = "REGGIE RESULT PING";
        ttl = 60;
        h = IndirectInitialize(text, StringGetLength((const unsigned char *)text));
        if (h != 0) {
            e = DatagramControl(ctx->socket, &ttl, kOptionIpTtl, kPunchTtl);
            if (e != 0)
                err = e;
            e = SimpleSend(s, from, h);
            if (e != 0)
                err = e;
            e = DatagramControl(ctx->socket, 0, kOptionIpTtl, ttl);
            if (e != 0)
                err = e;
        }
        ReggieProbeCheck(ctx, 0, local, &mine);
        e = _ReggieMatchBegin(ctx, kEntryKindInvite, 0, &mine, peer = from);
        if (e != 0)
            err = e;
        handled = 1;
    } else if (method == kPunchDirect) {
        ReggieProbeCheck(ctx, 0, local, &mine);
        e = _ReggieMatchBegin(ctx, kEntryKindMatch, 0, &mine, peer2 = from);
        if (e != 0)
            err = e;
        handled = 0;
        SystemWait(1000);
    }
    if (h != 0)
        IndirectDeallocate(h);
    return handled;
}

/* ReggieMeetMethod (how two NAT types can meet: 0 not at all, 1, 2 or 4; decided by `peerNat'
   first). STATE: MATCH 100%, exe 65 / obj 65, byte-identical yes. OPEN: the method codes'
   names are guesses; which side each argument is was not traced */
// FUNCTION: EVNOVA 0x004f7223
ULong ReggieMeetMethod(ULong nat, ULong peerNat)
{
    ULong method;
    long err;

    method = kPunchNone;
    err = 0;
    switch (peerNat) {
    case 0:
    case kNatUnknown:
        method = kPunchNone;
        break;
    case kNatRestrictive:
        switch (nat) {
        case 0:
        case kNatUnknown:
            method = kPunchNone;
            break;
        case kNatRestrictive:
        case kNatConsistent:
            method = kPunchRelay;
            break;
        case kNatProtective:
        case kNatPermissive:
        case kNatDirect:
            method = kPunchGuess;
            break;
        }
        break;
    case kNatConsistent:
        switch (nat) {
        case 0:
        case kNatUnknown:
            method = kPunchNone;
            break;
        case kNatRestrictive:
            method = kPunchRelay;
            break;
        case kNatConsistent:
            method = kPunchDirect;
            break;
        case kNatProtective:
        case kNatPermissive:
        case kNatDirect:
            method = kPunchGuess;
            break;
        }
        break;
    case kNatProtective:
        switch (nat) {
        case 0:
        case kNatUnknown:
            method = kPunchNone;
            break;
        case kNatRestrictive:
        case kNatConsistent:
        case kNatProtective:
        case kNatPermissive:
        case kNatDirect:
            method = kPunchDirect;
            break;
        }
        break;
    case kNatPermissive:
    case kNatDirect:
        switch (nat) {
        case 0:
        case kNatUnknown:
            method = kPunchNone;
            break;
        case kNatRestrictive:
        case kNatConsistent:
        case kNatProtective:
        case kNatPermissive:
        case kNatDirect:
            method = kPunchNone;
            break;
        }
        break;
    }
    return method;
}

/* ReggieProbeCheck (the NAT type of one of our local ports, and optionally the public address
   it maps to: the context's own, a direct entry's, or a recorded route). STATE: MATCH 100%,
   exe 135 / obj 135, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f72c5
ULong ReggieProbeCheck(void *ref, unsigned char tcp, const NetAddress *local,
                   NetAddress *out)
{
    SessionContext *ctx;
    NetAddress mine;
    SessionEntry *entry;
    NatRoute *route;
    unsigned short ownPort;
    unsigned short port;
    ULong nat;
    long err;
    long e;

    ctx = ref;
    err = 0;
    nat = kNatUnknown;
    entry = 0;
    route = 0;
    e = 0;
    ownPort = 0;
    port = 0;
    AddressToNetworkStack(local);
    if (AddressToNetworkStack(local) != 0) {
        err = kWrongFamily;
        goto done;
    }
    AddressDecomposeTCPIP(local, &port, 0);
    if (port == 0) {
        err = kWrongEntry;
        goto done;
    }
    if (out != 0)
        AddressComposeEmpty(out);
    ReggieTickle(ref);
    if (ctx->mode == kModeShared && tcp == 0 && DatagramGetAddress(ctx->socket, &mine) == 0)
        AddressDecomposeTCPIP(&mine, &ownPort, 0);
    if (ctx->state == kStateIdle) {
        nat = 0;
        goto done;
    }
    if (tcp == 0 && ownPort == port) {
        nat = ctx->natType;
        if (out != 0)
            AddressDuplicate(out, &ctx->publicAddress);
        goto done;
    }
    if ((entry = FindDirect(ctx, port, tcp)) != 0) {
        nat = entry->u.direct.natType;
        if (out != 0)
            AddressDuplicate(out, &entry->u.direct.peer);
        goto done;
    }
    route = _ReggieFindMapping(port, 0, tcp);
    if (route != 0) {
        nat = route->natType;
        if (out != 0)
            AddressComposeTCPIP(out, route->port, route->ip);
    }
done:
    return nat;
}

/* ReggieVersionCheck. STATE: MATCH 100%, exe 92 / obj 92 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f7474
void *ReggieVersionCheck(void *ref, const char *name)
{
    SessionContext *ctx;
    SessionEntry *entry;
    GlobalEntry *node;
    void *found;
    long err;

    ctx = ref;
    err = 0;
    entry = 0;
    node = 0;
    found = 0;
    if (ctx != 0) {
        ReggieTickle(ref);
        entry = FindNamed(ctx, name);
        if (entry != 0 && (found = entry->u.named.product) != 0)
            goto done;
        found = ctx->fallback;
        goto done;
    }
    _ReggiePrivateTickle();
    for (node = gVersionList; node != 0; node = node->next)
        if (StringCompare(node->name, name) == 0 && (found = node->payload) != 0)
            goto done;
done:
    return found;
}

/* Stand-in (mac _ReggieProbeBegin): the linker strips this; the exe keeps its literals. */
void ReggieProbeBegin(struct HashTable *t, const char *port, char *packet)
{
    HashInsert(t, "$PORT", port);
    HashInsert(t, "PROBE", "TCP");
    StringCopy(packet, " SIMPLE");
}

/* ReggieVersionBegin. STATE: MATCH 100%, exe 191 / obj 191 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f7571
long ReggieVersionBegin(SessionContext *ctx, const char *name)
{
    long err = 0;
    long result;
    long e;
    SessionEntry *entry;
    ULong slot;

    result = 0;
    entry = 0;
    slot = 0;
    if (ctx != 0) {
        entry = FindNamed(ctx, name);
        if (entry == 0) {
            e = ReggieChannelOpen(ctx, &slot);
            if (e != 0) {
                err = e;
                goto done;
            }
            entry = (SessionEntry *)MemoryAllocate(kSessionEntry);
            if (entry == 0) {
                err = kOutOfMemory;
                goto done;
            }
            MemoryClear(entry, kSessionEntry);
            entry->next = ctx->entries;
            ctx->entries = entry;
            entry->kind = kEntryKindNamed;
            entry->slot = slot;
            StringCopySafe(entry->u.named.name, name, kSessionNameLen);
            if ((entry->u.named.product = HashCreate(kRequestKind)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = HashInsert(entry->u.named.product, "$PRODUCT", entry->u.named.name);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if ((entry->request = HashCreate(kRequestKind)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = HashInsert(entry->request, "$PRODUCT", entry->u.named.name);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (ctx->mode != kModeNoExtras && ctx->hostSource != 0) {
            if ((e = AddHostID(entry->request, ctx->hostSource)) != 0) {
                err = e;
                goto done;
            }
        }
        entry->counter++;
        e = HashInsert(entry->request, "CXID",
                         StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu",
                                      entry->slot, entry->counter));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = HashInsert(entry->request, "VERSION", "");
        if (e != 0) {
            err = e;
            goto done;
        }
        if (ctx->mode != kModeNoExtras) {
            e = HashInsert(entry->request, "REGGIE", "");
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        e = _ReggieChannelPacket(ctx, entry);
        if (e != 0)
            err = e;
    } else {
        e = ReggieBeginVersionDirect(name);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* ReggieFreeMessages (free a kind-3 entry's request, its two tables and its two handles of tables).
   STATE: MATCH 100%, exe 87 / obj 87, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f77e9
long ReggieFreeMessages(SessionContext *ctx, SessionEntry *entry)
{
    long err;
    ULong i;
    ULong count;
    void **h;

    err = 0;
    if (entry->request != 0) {
        HashDispose(entry->request);
        entry->request = 0;
    }
    if (entry->u.messages.table != 0) {
        HashDispose(entry->u.messages.table);
        entry->u.messages.table = 0;
    }
    if (entry->u.messages.permissions != 0) {
        HashDispose(entry->u.messages.permissions);
        entry->u.messages.permissions = 0;
    }
    h = entry->u.messages.received;
    if (h != 0) {
        count = IndirectGetSize(h) / sizeof(struct HashTable *);
        for (i = 0; i < count; i++)
            if (((struct HashTable **)*h)[i] != 0)
                HashDispose(((struct HashTable **)*h)[i]);
        IndirectDeallocate(h);
        entry->u.messages.received = 0;
    }
    h = entry->u.messages.pending;
    if (h != 0) {
        count = IndirectGetSize(h) / sizeof(struct HashTable *);
        for (i = 0; i < count; i++)
            if (((struct HashTable **)*h)[i] != 0)
                HashDispose(((struct HashTable **)*h)[i]);
        IndirectDeallocate(h);
        entry->u.messages.pending = 0;
    }
    return 0;
}

/* _ReggieMessageChannel (the context's one messages entry, made on first use: two handles of tables
   and the permissions table). STATE: MATCH 100%, exe 80 / obj 80, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f78c3
SessionEntry *_ReggieMessageChannel(SessionContext *ctx)
{
    long err = 0;
    SessionEntry *entry = 0;
    ULong slot;
    long e;

    for (entry = ctx->entries; entry != 0; entry = entry->next)
        if (entry->kind == kEntryKindMessages)
            return entry;
    if (entry == 0) {
        e = ReggieChannelOpen(ctx, &slot);
        if (e != 0) {
            err = e;
            goto done;
        }
        entry = MemoryAllocate(kSessionEntry);
        if (entry == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryClear(entry, kSessionEntry);
        entry->next = ctx->entries;
        ctx->entries = entry;
        entry->kind = kEntryKindMessages;
        entry->slot = slot;
        if ((entry->u.messages.received = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((entry->u.messages.pending = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((entry->u.messages.permissions = HashCreate(kTableKind)) == 0)
            err = kOutOfMemory;
    }
done:
    if (err != 0 && entry != 0)
        _ReggieChannelClose(ctx, entry, 0);
    return entry;
}

/* ReggieSendPermissions (send the permissions table: one ALLOW or BLOCK request per target, "*"
   meaning everyone; the first goes out, the rest queue behind it). STATE: MATCH 100%, exe 148
   / obj 148, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f79b3
long ReggieSendPermissions(SessionContext *ctx, SessionEntry *entry)
{
    ULong i;
    char *target;
    char *value;
    char hostText[kTextBytes];
    long err, e, result;
    struct HashTable *t;

    err = 0;
    t = 0;
    for (i = 0; HashIndex(entry->u.messages.permissions, i, &target, &value) != 0; i++) {
        t = HashCreate(kTableKind);
        if (t == 0) {
            err = kOutOfMemory;
            break;
        }
        if (StringCompare(target, "*") != 0) {
            e = HashInsert(t, "$TARGET", target);
            if (e != 0) {
                err = e;
                break;
            }
        }
        if (ctx->mode != kModeTcp && ctx->hostSource != 0) {
            SessionContext *c = ctx;
            ULong host = c->hostSource;

            e = HashInsert(t, "HOSTID", StringFromNumber(hostText, host));
            if (e != 0) {
                err = e;
                break;
            }
        }
        e = HashInsert(t, *value != 0 ? "BLOCK" : "ALLOW", "");
        if (e != 0) {
            err = e;
            break;
        }
        if (ctx->mode != kModeTcp) {
            e = HashInsert(t, "REGGIE", "");
            if (e != 0) {
                err = e;
                break;
            }
        }
        if (entry->requestedAt != 0) {
            result = AppendTable(entry->u.messages.pending, t);
            if (result) {
                err = result;
                break;
            }
            t = 0;
        } else {
            entry->counter++;
            e = HashAppend(t, "CXID",
                             StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu",
                                          entry->slot, entry->counter));
            if (e != 0) {
                err = e;
                break;
            }
            entry->request = t;
            t = 0;
            e = _ReggieChannelPacket(ctx, entry);
            if (e != 0) {
                err = e;
                break;
            }
        }
    }
    if (t != 0)
        HashDispose(t);
    return err;
}

/* _ReggieSearchEnd (end a lookup: at once, or by sending CANCEL first). STATE: MATCH 100%, exe 97
   / obj 97, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f7b7e
long _ReggieSearchEnd(SessionContext *ctx, SessionEntry *entry, unsigned char cancel)
{
    long err;
    long e;

    err = 0;
    if (cancel != 0) {
        e = SendCancel(ctx, entry);
        if (e)
            err = e;
        else
            _ReggieLookupDispose(ctx, entry);
    } else {
        _ReggieLookupDispose(ctx, entry);
        if (entry->u.lookup.query != 0) {
            HashDispose(entry->u.lookup.query);
            entry->u.lookup.query = 0;
        }
        if (entry->request != 0) {
            HashDispose(entry->request);
            entry->request = 0;
        }
        entry->requestedAt = 0;
        entry->cancelledAt = 0;
        entry->retries = 0;
        entry->u.lookup.notified = 0;
        entry->u.lookup.finished = 0;
    }
    if (cancel != 0 && err != 0)
        _ReggieSearchEnd(ctx, entry, 0);
    return err;
}

/* ReggieProbePrejoin (a peer's NAT type from the lookup records whose $ADDRESS is its address, with
   the port, else without it; 1 when none says, 0 when the context has no +0x004). STATE: MATCH
   100%, exe 113 / obj 113, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f7c96
ULong ReggieProbePrejoin(SessionContext *ctx, const NetAddress *address)
{
    char text[kTextBytes];
    ULong result;
    long err;
    SessionEntry *e;
    struct HashTable *t;
    const char *seen;
    char *sep;
    ULong i;

    err = 0;
    e = 0;
    t = 0;
    result = 0;
    if (ctx->state == 0)
        goto done;
    AddressTranscribe(address, text);
    for (e = ctx->entries; e != 0; e = e->next) {
        if (e->kind == kEntryKindLookup && e->u.lookup.finished == 0) {
            for (i = 0; i < e->recordCount; i++) {
                t = e->records[i].table;
                if (t != 0 && (seen = HashLookup(t, "$ADDRESS", 0)) != 0 &&
                    StringCompare(seen, text) == 0)
                    goto found;
            }
        }
    }
    sep = StringFindDelimiter(text, ":, ");
    if (sep != 0) {
        *sep = 0;
        for (e = ctx->entries; e != 0; e = e->next) {
            if (e->kind == kEntryKindLookup && e->u.lookup.finished == 0) {
                for (i = 0; i < e->recordCount; i++) {
                    t = e->records[i].table;
                    if (t != 0 && (seen = HashLookup(t, "$ADDRESS", 0)) != 0 &&
                        StringCompare(seen, text) == 0) {
found:
                        result = _ReggieNetworkFromString(ctx, HashLookup(t, "$NETWORK", 0));
                        goto done;
                    }
                }
            }
        }
    }
    result = kNatUnknown;
done:
    return result;
}

/* _ReggieInsertEnd (end an insert: at once, or by sending CANCEL first). STATE: MATCH 100%, exe 82
   / obj 82, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f7dc3
long _ReggieInsertEnd(SessionContext *ctx, SessionEntry *entry, unsigned char cancel)
{
    long err;
    long e;

    err = 0;
    if (cancel != 0) {
        e = SendCancel(ctx, entry);
        if (e)
            err = e;
    } else {
        if (entry->u.lookup.query != 0) {
            HashDispose(entry->u.lookup.query);
            entry->u.lookup.query = 0;
        }
        if (entry->request != 0) {
            HashDispose(entry->request);
            entry->request = 0;
        }
        entry->requestedAt = 0;
        entry->cancelledAt = 0;
        entry->retries = 0;
    }
    if (cancel != 0 && err != 0)
        _ReggieInsertEnd(ctx, entry, 0);
    return err;
}

/* Stand-in (mac _ReggieSearchBegin): the linker strips this; the exe keeps its literals. */
void ReggieSearchBegin(struct HashTable *record, struct HashTable *request)
{
    HashLookup(record, "$PRIVATE", 0);
    HashLookup(record, "NOTIFY", 0);
    HashLookup(record, "LIMIT", 0);
    HashInsert(request, "LIMIT", "100");
    HashLookup(record, "HOST", 0);
    HashLookup(record, "SCORE", 0);
    HashLookup(record, "DATA", 0);
    HashInsert(request, "SEARCH", "");
    HashInsert(request, "INSERT", "");
}

/* ReggieInsertBegin (INSERT a record under a slot: a copy kept for re-inserting, $KIND, our
   public and private addresses and NAT type added when the record lacks them).
   STATE: MATCH 100%, exe 269 / obj 269, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f7eb7
long ReggieInsertBegin(void *ref, ULong slot, long mode, struct HashTable *record)
{
    SessionContext *ctx = ref;
    long err = 0;
    long e;
    char publicText[kTextBytes];
    char localText[kTextBytes];
    char natText[kTextBytes];
    SessionEntry *entry;
    const char *s;

    entry = 0;
    for (entry = ctx->entries; entry != 0 && entry->slot != slot; entry = entry->next)
        ;
    if (entry == 0) {
        entry = MemoryAllocate(kSessionEntry);
        if (entry == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryClear(entry, kSessionEntry);
        entry->next = ctx->entries;
        ctx->entries = entry;
        entry->kind = kEntryKindInsert;
        entry->slot = slot;
    }
    if (entry->kind != kEntryKindInsert) {
        err = kWrongEntry;
        goto done;
    }
    _ReggieInsertEnd(ctx, entry, 0);
    if ((entry->u.lookup.query = HashClone(record)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    entry->insertMode = mode;
    s = HashLookup(record, "$KIND", 0);
    if (s != 0 && *s != 0)
        String2UpperCopySafe(entry->u.lookup.kindName, s, kSessionNameLen);
    else
        String2UpperCopySafe(entry->u.lookup.kindName, ctx->kind, kSessionNameLen);
    if ((entry->request = HashClone(record)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    while (HashRemove(entry->request, "$KIND") != 0)
        ;
    e = HashInsert(entry->request, "$KIND", entry->u.lookup.kindName);
    if (e != 0) {
        err = e;
        goto done;
    }
    s = HashLookup(entry->request, "$ADDRESS", 0);
    if (ReggieDescribeConnection(ctx, s, publicText, localText, natText) != 0) {
        if (s != 0 && *s != 0 && StringCompare(s, publicText) != 0) {
            /* BUG: deletes $ADDRESS from the caller's record, not from the request */
            while (HashRemove(record, "$ADDRESS") != 0)
                ;
            if (publicText[0] != 0) {
                e = HashInsert(entry->request, "$ADDRESS", publicText);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
        }
        if (localText[0] != 0 && HashLookup(record, "$PRIVATE", 0) == 0) {
            e = HashInsert(entry->request, "$PRIVATE", localText);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (natText[0] != 0 && HashLookup(record, "$NETWORK", 0) == 0) {
            e = HashInsert(entry->request, "$NETWORK", natText);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    if (ctx->mode != kModeTcp && ctx->hostSource != 0) {
        if ((e = AddHostID(entry->request, ctx->hostSource)) != 0) {
            err = e;
            goto done;
        }
    }
    entry->counter++;
    e = HashInsert(entry->request, "CXID",
                     StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu", entry->slot,
                                  entry->counter));
    if (e != 0) {
        err = e;
        goto done;
    }
    /* BUG: no default, so any other mode sends the request's $ADDRESS as the INSERT value */
    switch (mode) {
    case kInsertHost:
        s = "HOST";
        break;
    case kInsertScore:
        s = "SCORE";
        break;
    case kInsertData:
        s = "DATA";
        break;
    }
    e = HashInsert(entry->request, "INSERT", s);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (ctx->mode != kModeTcp) {
        e = HashInsert(entry->request, "REGGIE", "");
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = _ReggieChannelPacket(ctx, entry);
    if (e != 0)
        err = e;
done:
    if (err != 0 && entry != 0)
        _ReggieChannelClose(ctx, entry, 0);
    return err;
}

/* _ReggieChannelClose (drop an entry: its kind's own clean-up -- a CANCEL first when asked --
   then its request, and unlink and free it). STATE: MATCH 100%, exe 119 / obj 119,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f8245
long _ReggieChannelClose(SessionContext *ctx, SessionEntry *entry, unsigned char cancel)
{
    long err = 0;
    SessionEntry **link = 0;
    long e;
    long result;
    long dropped;
    long released;

    switch (entry->kind) {
    case kEntryKindInsert:
        result = _ReggieInsertEnd(ctx, entry, cancel);
        if (result != 0) {
            err = result;
            goto done;
        }
        break;
    case kEntryKindLookup:
        e = _ReggieSearchEnd(ctx, entry, cancel);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kEntryKindMessages:
        e = ReggieFreeMessages(ctx, entry);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kEntryKindNamed:
        dropped = DropNamed(&entry);
        break;
    case kEntryKindDirect:
        e = _ReggieProbeEnd(ctx, entry);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kEntryKindMatch:
    case kEntryKindInvite:
        released = DropPeer(&ctx, &entry);
        break;
    }
    if (entry->request != 0) {
        HashDispose(entry->request);
        entry->request = 0;
    }
    entry->requestedAt = 0;
    entry->cancelledAt = 0;
    entry->retries = 0;
    for (link = &ctx->entries; *link != 0; link = &(*link)->next) {
        if (*link == entry) {
            *link = entry->next;
            MemoryDeallocate(entry);
            break;
        }
    }
done:
    return err;
}

/* _ReggieChannelTickle (an entry's timers: its kind's poll, then resend the request or the CANCEL
   after 3 s plus one per try; after five tries the connection is dropped). STATE: MATCH
   100%, exe 185 / obj 185, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f83a8
long _ReggieChannelTickle(SessionContext *ctx, SessionEntry *entry)
{
    long elapsed2;
    long err = 0;
    long limit2;
    unsigned char tries2;
    long e;
    long polled;
    long inserted;
    unsigned long at2;
    long elapsed;
    long limit;
    unsigned char tries;
    unsigned long at;

    switch (entry->kind) {
    case kEntryKindInsert:
        inserted = PollPeer(&ctx, &entry);
        break;
    case kEntryKindLookup: {
        long result;
        long lookupErr;
        long status;
        SessionContext *c;
        SessionEntry *en;

        en = entry;
        c = ctx;
        lookupErr = 0;
        e = ReggieLookupTimer(c, en);
        if (e != 0)
            lookupErr = e;
        status = lookupErr;
        result = status;
        if (result) {
            err = result;
            goto done;
        }
        break;
    }
    case kEntryKindMessages:
        polled = PollHandle(ctx, entry);
        break;
    case kEntryKindNamed:
        polled = PollHandle(ctx, entry);
        break;
    case kEntryKindDirect:
        e = ReggiePollDirect(ctx, entry);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kEntryKindMatch:
    case kEntryKindInvite:
        (void)PollIdle(&ctx, &entry);
        break;
    }
    if (entry->requestedAt != 0) {
        elapsed = TimerGetElapsedSeconds() - (at = entry->requestedAt);
        limit = (tries = entry->retries) + 3;
        if (tries > 4 && elapsed > limit) {
            _ReggieServerOffline(ctx, "Transaction timeout to server");
            goto done;
        }
        if (at != 0xffffffff) {
            if (elapsed <= limit)
                goto done;
            if (ctx->mode == kModeTcp)
                goto done;
        }
        e = _ReggieSendHash(ctx, entry->request);
        if (e != 0) {
            err = e;
            goto done;
        }
        ctx->sentAt = entry->requestedAt = TimerGetElapsedSeconds();
        entry->cancelledAt = 0;
    } else {
        if (entry->cancelledAt == 0)
            goto done;
        elapsed2 = TimerGetElapsedSeconds() - (at2 = entry->cancelledAt);
        limit2 = (tries2 = entry->retries) + 3;
        if (tries2 > 4 && elapsed2 > limit2) {
            _ReggieServerOffline(ctx, "Transaction timeout to server");
            goto done;
        }
        if (at2 != 0xffffffff) {
            if (elapsed2 <= limit2)
                goto done;
            if (ctx->mode == kModeTcp)
                goto done;
        }
        e = _ReggieSendString(ctx, kCancelFormat,
                         StringFormatSafe(entry->idText, kSessionNameLen, "%lu/%lu", entry->slot,
                                      ++entry->counter),
                         ctx->hostSource);
        if (e != 0) {
            err = e;
            goto done;
        }
        ctx->sentAt = entry->cancelledAt = TimerGetElapsedSeconds();
        entry->requestedAt = 0;
    }
    entry->retries++;
done:
    return err;
}

/* _ReggieChannelPacket (send an entry's request now when online, else mark it waiting). STATE: MATCH
   100%, exe 41 / obj 41, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f8610
long _ReggieChannelPacket(SessionContext *ctx, SessionEntry *entry)
{
    long err, e;

    err = 0;
    HashLookup(entry->request, "CXID", 0);
    if (ctx->state == kStateOnline) {
        e = _ReggieSendHash(ctx, entry->request);
        if (e != 0) {
            err = e;
            goto done;
        }
        entry->requestedAt = TimerGetElapsedSeconds();
        ctx->sentAt = entry->requestedAt;
        entry->cancelledAt = 0;
        entry->retries = 0;
    } else {
        entry->requestedAt = 0xffffffff;
        entry->cancelledAt = 0;
        entry->retries = 0;
    }
done:
    return err;
}

typedef struct SlotText {
    char chars[kTextBytes];
} SlotText;
static SlotText sBlankSlot;                      /* 0x008675a1 the empty text ReggieFindByCxid starts from */

/* ReggieFindByCxid (the entry a CXID "slot/counter" names, by its slot). STATE: MATCH 100%,
   exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f8694
SessionEntry *ReggieFindByCxid(SessionContext *ctx, const char *cxid)
{
    long err = 0;
    SessionEntry *entry = 0;
    ULong slot = 0;
    SlotText text = sBlankSlot;
    char *token;
    char *next;

    if (cxid != 0 && *cxid != 0) {
        token = 0;
        next = 0;
        StringCopySafe(text.chars, cxid, sizeof(text.chars));
        while ((token = StringTokenize(text.chars, '/', &next)) != 0)
            ;
    }
    slot = StringToNumber(text.chars);
    for (entry = ctx->entries; entry != 0 && entry->slot != slot; entry = entry->next)
        ;
    return entry;
}

/* ReggieChannelOpen. STATE: MATCH 100%, exe 21 / obj 21, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f8733
long ReggieChannelOpen(void *ref, ULong *slot)
{
    SessionContext *ctx;
    SessionEntry *entry;

    ctx = (SessionContext *)ref;
    entry = 0;
    *slot = 0;
    ctx->lastSlot++;
    entry = ctx->entries;
    while (entry != 0) {
        if (entry->slot == ctx->lastSlot) {
            ctx->lastSlot++;
            entry = ctx->entries;
        } else {
            entry = entry->next;
        }
    }
    *slot = ctx->lastSlot;
    return 0;
}

/* ReggieSendFailed (the shared socket's hook for a connection reset from an address: count it
   against every session whose server that is; no CSV row). STATE: MATCH 100%, exe 29 / obj 29,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f876f
unsigned char ReggieSendFailed(NetSocket *s, const NetAddress *from)
{
    SessionContext *ctx;
    unsigned char seen;
    long err;

    err = 0;
    ctx = 0;
    seen = false;
    if (gReggieOpen != 0 && gReggieBusy == 0) {
        for (ctx = gReggieList; ctx != 0; ctx = ctx->next) {
            if (AddressCompare(from, &ctx->server) != 0) {
                ctx->resets++;
                seen = true;
            }
        }
    }
    return seen;
}

/* _ReggieSharedEndpoint (does a session own this socket; no CSV row). STATE: MATCH 100%, exe 17 / obj
   17, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f87b9
unsigned char _ReggieSharedEndpoint(NetSocket *s)
{
    long err;
    SessionContext *ctx;

    err = 0;
    ctx = 0;
    for (ctx = gReggieList; ctx != 0; ctx = ctx->next) {
        if (ctx->socket == s)
            break;
    }
    return ctx != 0 ? true : false;
}

/* Stand-in (mac _ReggieDispose): the linker strips this; the exe keeps its literals. */
void ReggieDispose(SessionContext *ctx)
{
    _ReggieSendString(ctx, "REGGIE GOODBYE HOSTID=%lu", ctx->hostSource);
}

/* _ReggiePacketSniffer (the shared socket's hook for a datagram: a session's server, or anyone's PROBE
   result, is taken). STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f87e1
unsigned char _ReggiePacketSniffer(NetSocket *s, const char *data, ULong length,
                           const NetAddress *from)
{
    SessionContext *ctx;
    unsigned char taken;
    long err, e;

    err = 0;
    taken = false;
    ctx = 0;
    for (ctx = gReggieList; ctx != 0; ctx = ctx->next) {
        if (ctx->socket == s)
            break;
    }
    if (ctx != 0 &&
        (AddressCompare(from, &ctx->server) != 0 ||
         StringCompareSafe(data, kProbeReply, sizeof(kProbeReply) - 1) == 0)) {
        e = ReggieRawPacket(ctx, data, length, from);
        if (e != 0)
            err = e;
        taken = true;
    }
    return taken;
}

/* ReggieRawPacket (a hooked datagram into one of the session's packets, dispatched at once).
   STATE: MATCH 100%, exe 57 / obj 57, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f8852
long ReggieRawPacket(void *ref, const char *data, ULong length,
                  const NetAddress *from)
{
    SessionContext *ctx;
    long err;
    MutexNode *node;
    NetPacket *packet;

    ctx = ref;
    err = 0;
    node = 0;
    node = QueueRemove(ctx->freeList);
    if (node == 0) {
        err = kOutOfMemory;
        goto done;
    }
    packet = (NetPacket *)node;
    packet->length = length < packet->capacity ? length : packet->capacity;
    MemoryMove(packet->data, data, packet->length);
    AddressDuplicate(&packet->from, from);
    err = ReggiePacket(ref, node);
    node = 0;
    if (err != 0) {
        node = 0;
        goto done;
    }
done:
    if (node != 0)
        QueueInsert(ctx->freeList, node);
    return err;
}

/* _ReggiePacketExtract (take lines off the front of a handle until one parses to a non-empty
   KEY=VALUE table). STATE: MATCH 100%, exe 127 / obj 127, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f88e6
long _ReggiePacketExtract(void **h, struct HashTable **out)
{
    ULong size, rest;
    long err, e;
    char line[kLineBytes];
    void **first;
    ULong n;

    err = 0;
    first = 0;
    *out = 0;
    while (*out == 0 && (size = IndirectGetSize(h)) != 0) {
        for (n = 0; n < size; n++) {
            if (((char *)*h)[n] == '\r' || ((char *)*h)[n] == '\n')
                break;
        }
        for (; n < size; n++) {
            if (((char *)*h)[n] != '\r' && ((char *)*h)[n] != '\n')
                break;
        }
        /* BUG: a skipped empty line's handle is never freed before this replaces it */
        first = IndirectAllocate(n);
        if (first == 0)
            goto nomem;
        MemoryCopy(*first, *h, n);
        rest = size - n;
        MemoryMove(*h, (char *)*h + n, rest);
        e = IndirectSetSize(h, rest);
        if (e != 0) {
            err = e;
            break;
        }
        StringFormatSafe(line, sizeof(line), "%*.*s", IndirectGetSize(first), IndirectGetSize(first),
                     *first);
        if ((*out = HashLoad(kTableKind, first)) == 0) {
nomem:
            err = kOutOfMemory;
            break;
        }
        if (HashIsEmpty(*out) != 0) {
            HashDispose(*out);
            *out = 0;
        }
    }
    if (first != 0)
        IndirectDeallocate(first);
    if (err != 0 && *out != 0) {
        HashDispose(*out);
        *out = 0;
    }
    return err;
}

/* _ReggiePacketDecode (a received message as a new pointer: binary -- after a 2-byte header,
   inflated, when there is a zlib state, else unpacked unless it starts "REGGIE" -- or text
   with its NULs turned to spaces). STATE: MATCH 100%, exe 87 / obj 87, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f8a31
long _ReggiePacketDecode(char *data, ULong length, struct ZState *z, unsigned char binary,
                  void ***out)
{
    ULong i;
    long result;
    long err;

    err = 0;
    *out = 0;
    if (binary != 0 &&
        (z != 0 || length < kGreetingLen || StringCompareSafe(data, "REGGIE", kGreetingLen) != 0)) {
        if (z != 0) {
            data += kZlibHeaderLen;
            length -= kZlibHeaderLen;
        }
        if ((*out = IndirectInitialize(data, length)) == 0) {
            err = kOutOfMemory;
        } else {
            if (z != 0)
                result = ProgressiveDecompress(*out, z);
            else
                result = IndirectDecompress(*out, kZlibTag, 0);
            if (result != 0)
                err = result;
        }
    } else {
        for (i = 0; i < length; i++)
            if (data[i] == 0)
                data[i] = ' ';
        *out = IndirectInitialize(data, length);
        if (*out == 0)
            err = kOutOfMemory;
    }
    if (err != 0 && *out != 0) {
        IndirectDeallocate(*out);
        *out = 0;
    }
    return err;
}

/* ReggiePacket (the reply dispatcher: decode a packet or stream chunk into lines, each a
   table; requests (INVITE MATCH MESSAGE NOTIFY PING) and RESULTs go to their handlers,
   anything else is answered "Unexpected or invalid request"). STATE: MATCH 100%, exe 616 /
   obj 616, byte-identical (see the commit). OPEN: none */
// FUNCTION: EVNOVA 0x004f8b05
long ReggiePacket(SessionContext *ctx, MutexNode *node)
{
    long err = 0;
    unsigned char handled = 0;
    char *key;
    char *value;
    struct HashTable *table = 0;
    struct HashTable *reply = 0;
    void **message = 0;
    char version[kSessionNameLen] = { 0 };
    SessionEntry *entry = 0;
    NetAddress from;
    NetPacket *packet;
    NetChunk *chunk;
    long result;
    long e;

    if (ctx->state == kStateClosed) {
        err = kClosedError;
        goto done;
    }
    if (SystemCheckCallback() != 0) {
        QueueInsert(ctx->recvList, node);
        node = 0;
        goto done;
    }
    if (ctx->mode == kModeTcp) {
        chunk = (NetChunk *)node;
        AddressDuplicate(&from, &ctx->server);
        e = _ReggiePacketDecode(chunk->data, chunk->length,
                         ctx->lengthFrames != 0 ? &ctx->inflater : 0, ctx->lengthFrames,
                         &message);
        if (e != 0) {
            err = e;
            goto done;
        }
        QueueInsert(ctx->freeList, node);
        node = 0;
    } else {
        packet = (NetPacket *)node;
        if (AddressCompare(&packet->from, &ctx->server) == 0 &&
            StringCompareSafe(packet->data, kProbeReply, sizeof(kProbeReply) - 1) != 0)
            goto done;
        AddressDuplicate(&from, &packet->from);
        e = _ReggiePacketDecode(packet->data, packet->length, 0, 1, &message);
        if (e != 0) {
            err = e;
            goto done;
        }
        QueueInsert(ctx->freeList, node);
        node = 0;
    }
    ctx->receivedAt = TimerGetElapsedSeconds();
    ctx->resets = 0;
    e = _ReggiePacketExtract(message, &table);
    if (e != 0) {
        err = e;
        goto done;
    }
    while (table != 0) {
        if (HashIndex(table, 0, &key, &value) != 0 && StringCompare(key, "REGGIE") == 0) {
            StringCopySafe(version, value != 0 ? value : "", sizeof(version));
            HashRemove(table, "REGGIE");
        }
        entry = ReggieFindByCxid(ctx, HashLookup(table, "CXID", 0));
        if (HashIndex(table, 0, &key, &value) == 0)
            goto dispatched;
        switch (*key) {
        case 'I':
            if (StringCompare(key, "INVITE") != 0)
                break;
            e = _ReggieRecv_INVITE(ctx, entry, table);
            if (e != 0)
                err = e;
            err = 0;
            handled = 1;
            break;
        case 'M':
            if (StringCompare(key, "MATCH") == 0) {
                e = _ReggieRecv_MATCH(ctx, entry, table);
                if (e != 0)
                    err = e;
            } else if (StringCompare(key, "MESSAGE") == 0) {
                e = _ReggieRecv_MESSAGE(ctx, entry, table);
                if (e != 0)
                    err = e;
            } else {
                break;
            }
            err = 0;
            handled = 1;
            break;
        case 'N':
            if (StringCompare(key, "NOTIFY") != 0)
                break;
            e = _ReggieRecv_NOTIFY(ctx, entry, table);
            if (e != 0)
                err = e;
            err = 0;
            handled = 1;
            break;
        case 'P':
            if (StringCompare(key, "PING") != 0)
                break;
            e = ReggieRequestPing(ctx, entry, table);
            if (e != 0)
                err = e;
            err = 0;
            handled = 1;
            break;
        case 'R':
            if (StringCompare(key, "RESULT") != 0)
                break;
            if (HashIndex(table, 1, &key, &value) == 0)
                goto result;
            switch (*key) {
            case 'A':
                if (StringCompareSafe(key, "ALLOW", sizeof("ALLOW")) != 0)
                    goto result;
                e = _ReggieRecv_RESULT_BLOCK(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            case 'B':
                if (StringCompare(key, "BLOCK") != 0)
                    goto result;
                e = _ReggieRecv_RESULT_BLOCK(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            case 'C':
                if (StringCompare(key, "CANCEL") != 0)
                    goto result;
                e = ReggieResultCancel(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            case 'H':
                if (StringCompare(key, "HELLO") != 0)
                    goto result;
                e = _ReggieRecv_RESULT_HELLO(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            case 'I':
                if (StringCompare(key, "INSERT") == 0) {
                    e = ReggieResultInsert(ctx, entry, table);
                    if (e != 0)
                        err = e;
                    err = 0;
                    handled = 1;
                } else if (StringCompare(key, "INVITE") == 0) {
                    e = ReggieResultInvite(ctx, entry, table);
                    if (e != 0)
                        err = e;
                    err = 0;
                    handled = 1;
                } else {
                    goto result;
                }
                goto result;
            case 'L':
                if (StringCompare(key, "LOOKUP") != 0)
                    goto result;
                e = _ReggieRecv_RESULT_LOOKUP(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            case 'M':
                if (StringCompare(key, "MATCH") == 0) {
                    e = ReggieResultMatch(ctx, entry, table);
                    if (e != 0)
                        err = e;
                    err = 0;
                    handled = 1;
                } else if (StringCompare(key, "MESSAGE") == 0) {
                    e = ReggieResultMessage(ctx, entry, table);
                    if (e != 0)
                        err = e;
                    err = 0;
                    handled = 1;
                } else {
                    goto result;
                }
                goto result;
            case 'P':
                if (StringCompare(key, "PING") == 0) {
                    result = CheckInstance(ctx, table);
                    err = 0;
                    handled = 1;
                } else if (StringCompare(key, "PROBE") == 0) {
                    e = _ReggieRecv_RESULT_PROBE(ctx, entry, table, &from);
                    if (e != 0)
                        err = e;
                    err = 0;
                    handled = 1;
                } else {
                    goto result;
                }
                goto result;
            case 'S':
                if (StringCompare(key, "SEARCH") != 0)
                    goto result;
                e = ReggieResultSearch(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            case 'V':
                if (StringCompare(key, "VERSION") != 0)
                    goto result;
                e = ReggieResultVersion(ctx, entry, table);
                if (e != 0)
                    err = e;
                err = 0;
                handled = 1;
                goto result;
            default:
                goto result;
            }
        result:
            if (handled == 0)
                handled = 1;
        dispatched:
            break;
        }
        /* BUG: handled is never cleared between lines, so after one handled line none is answered */
        if (handled == 0) {
            reply = HashClone(table);
            if (reply == 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = HashInsert(reply, "RESULT", "Unexpected or invalid request");
            if (e != 0) {
                err = e;
                goto done;
            }
            e = _ReggieSendHash(ctx, reply);
            if (e != 0) {
                err = e;
                goto done;
            }
            ctx->sentAt = TimerGetElapsedSeconds();
        }
        HashDispose(table);
        table = 0;
        e = _ReggiePacketExtract(message, &table);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
done:
    if (node != 0)
        QueueInsert(ctx->freeList, node);
    if (message != 0)
        IndirectDeallocate(message);
    if (table != 0)
        HashDispose(table);
    if (reply != 0)
        HashDispose(reply);
    return err;
}

/* ReggieTickle (a session's poll: the dispatcher over every waiting packet, then the
   connection's state machine -- reconnect on a backing-off schedule, HELLO, keep-alive
   PING, timeouts -- then each entry's timers while online). STATE: MATCH 100%, exe 393 /
   obj 393, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9309
long ReggieTickle(void *ref)
{
    SessionContext *ctx;
    struct HashTable *hello;
    MutexNode *node;
    ULong now;
    long elapsed;
    long err;
    long result;
    SessionEntry *e;
    unsigned char retry;
    unsigned char polling;

    polling = 0;
    ctx = (SessionContext *)ref;
    err = 0;
    now = TimerGetElapsedSeconds();
    retry = 0;
    e = 0;
    hello = 0;
    if (ctx->state == kStateClosed) {
        /* BUG: this and the free-list exit reach done before `polling' is saved, clearing the re-entry flag */
        err = kClosedError;
        goto done;
    }
    if (ctx->mode != kModeShared && QueueGetCount(ctx->freeList) <= 2) {
        if (ctx->mode == kModeTcp)
            result = NetworkQueueFill(ctx->freeList, kListChunks, 3, kChunkBytes);
        else
            result = NetworkQueueFill(ctx->freeList, kListPackets, 5, kPacketBytes);
        if (result != 0) {
            err = result;
            goto done;
        }
    }
    polling = pollRunning;
    if (pollRunning != 0 || ctx->inCallback != 0)
        goto done;
    pollRunning = 1;
    while ((node = QueueRemove(ctx->recvList)) != 0)
        ReggiePacket(ctx, node);
    if (ctx->streamLost != 0) {
        ctx->streamLost = 0;
        if (ctx->state != kStateIdle)
            _ReggieServerOffline(ctx, "Connection unexpectedly closed");
    }
    switch ((ULong)ctx->state) {
    case kStateIdle:
        elapsed = now - ctx->receivedAt;
        if (elapsed < 15) {
            if ((long)(now - ctx->stateAt) > 10)
                retry = 1;
        } else if (elapsed < 60) {
            if ((long)(now - ctx->stateAt) > 30)
                retry = 1;
        } else if (elapsed < 600) {
            if ((long)(now - ctx->stateAt) > 60)
                retry = 1;
        } else if (elapsed < 1800) {
            if ((long)(now - ctx->stateAt) > 180)
                retry = 1;
        } else if (elapsed < 3600) {
            if ((long)(now - ctx->stateAt) > 360)
                retry = 1;
        } else if (elapsed < 86400) {
            if ((long)(now - ctx->stateAt) > 900)
                retry = 1;
        } else if ((long)(now - ctx->stateAt) > 3600) {
            retry = 1;
        }
        if (retry != 0) {
            if (ctx->mode == kModeTcp) {
                ProgressiveDispose(&ctx->deflater);
                ProgressiveDispose(&ctx->inflater);
                ctx->stateAt = TimerGetElapsedSeconds();
                StreamDispose(ctx->socket);
                if ((ctx->socket = (NetSocket *)StreamCreateBuffered(0, ctx, _ReggieNetworkCallback, (long)_ReggieBufferCallback,
                                                             ctx->freeList, ctx->freeList,
                                                             ctx->recvList, ctx->recvList)) == 0) {
                    err = kOutOfMemory;
                    goto done;
                }
                result = StreamOpenClient((NetStream *)ctx->socket, 0, &ctx->server, 0);
                if (result != 0) {
                    err = result;
                    goto done;
                }
                ctx->state = kStateConnecting;
                ctx->stateAt = TimerGetElapsedSeconds();
                ctx->stateTicks = TimerGetTicks();
            } else {
                if (ctx->mode != kModeOwnUdp)
                    break;
                result = DatagramOpen(ctx->socket, 0);
                if (result != 0) {
                    err = result;
                    goto done;
                }
                ctx->state = kStateHello;
                ctx->stateAt = TimerGetElapsedSeconds();
                ctx->stateTicks = TimerGetTicks();
            }
        }
        break;
    case kStateConnecting:
        if ((int)EndpointGetStatus(ctx->socket) == 4) {
            ctx->state = kStateHello;
            goto sendHello;
        }
        if (TimerGetTicks() - ctx->stateTicks > TimerGetTickRate() * 10) {
            _ReggieServerOffline(ctx, "Connection request timed out");
            break;
        }
        if ((int)EndpointGetStatus(ctx->socket) != 3)
            _ReggieServerOffline(ctx, "Connection request refused");
        break;
    case kStateHello:
    sendHello:
        hello = HashCreate(kTableKind);
        if (hello == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (ctx->password[0] != 0) {
            result = HashInsert(hello, "$PASS", ctx->password);
            if (result != 0) {
                err = result;
                goto done;
            }
        }
        if (ctx->user[0] != 0) {
            result = HashInsert(hello, "$USER", ctx->user);
            if (result != 0) {
                err = result;
                goto done;
            }
        }
        if (ctx->kind[0] != 0) {
            result = HashInsert(hello, "$KIND", ctx->kind);
            if (result != 0) {
                err = result;
                goto done;
            }
        }
        if (ctx->mode == kModeShared) {
            result = HashInsert(hello, "PROBE", "UDP");
            if (result != 0) {
                err = result;
                goto done;
            }
        }
        result = HashInsert(hello, "HELLO", "");
        if (result != 0) {
            err = result;
            goto done;
        }
        result = HashInsert(hello, "REGGIE", "");
        if (result != 0) {
            err = result;
            goto done;
        }
        result = _ReggieSendHash(ctx, hello);
        if (result != 0) {
            err = result;
            goto done;
        }
        ctx->state = kStateHelloSent;
        if (ctx->helloRetried == 0) {
            ctx->sentAt = TimerGetElapsedSeconds();
            ctx->stateTicks = TimerGetTicks();
        }
        HashDispose(hello);
        hello = 0;
        break;
    case kStateHelloSent:
        if (ctx->resets >= 1) {
            if (TimerGetTicks() - ctx->stateTicks > TimerGetTickRate() * 3) {
                _ReggieServerOffline(ctx, "No response to connection request");
                break;
            }
        }
        if (TimerGetTicks() - ctx->stateTicks > TimerGetTickRate() * 10) {
            _ReggieServerOffline(ctx, "No response to connection request");
            break;
        }
        if (TimerGetTicks() - ctx->stateTicks > TimerGetTickRate() * 10 / 2 && ctx->mode != kModeTcp &&
            ctx->helloRetried == 0) {
            ctx->helloRetried = 1;
            goto sendHello;
        }
        break;
    case kStateOnline:
        if (ctx->resets >= 2 && (long)(now - ctx->receivedAt) > 3) {
            _ReggieServerOffline(ctx, "Lost connection to server");
            break;
        }
        elapsed = now - ctx->receivedAt;
        if (elapsed > 130) {
            _ReggieServerOffline(ctx, "Lost connection to server");
            break;
        }
        if (elapsed > 30 && (long)(now - ctx->sentAt) > 30) {
            result = _ReggieSendString(ctx, "REGGIE PING HOSTID=%lu", ctx->hostSource);
            if (result != 0) {
                err = result;
                goto done;
            }
            ctx->sentAt = TimerGetElapsedSeconds();
        }
        break;
    case kStateClosed:
        break;
    default:
        break;
    }
    for (e = ctx->entries; e != 0 && ctx->state == kStateOnline; e = e->next) {
        result = _ReggieChannelTickle(ctx, e);
        if (result != 0)
            err = result;
    }
done:
    if (hello != 0)
        HashDispose(hello);
    pollRunning = polling;
    return err;
}

/* _ReggieOpen (network start-up for REGGIE, once: the idle proc and the shared socket's four
   hooks). STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9899
long _ReggieOpen(void)
{
    long err, e;

    err = 0;
    if (gReggieOpen != 0)
        goto done;
    e = PlatformOpen();
    if (e != 0) {
        err = e;
        goto done;
    }
    SystemSetIdleProc(_ReggieIdleCallback);
    e = NetworkStackLoad(1);
    if (e != 0) {
        err = e;
        goto done;
    }
    gReggieSharedProc = _ReggieSharedEndpoint;
    gReggiePacketProc = _ReggiePacketSniffer;
    gReggiePrejoinProc = _ReggieProbePrejoin;
    gReggieSendFailedProc = ReggieSendFailed;
    gReggieBusy = false;
    gReggieOpen = true;
done:
    return err;
}
