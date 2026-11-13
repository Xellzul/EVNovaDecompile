/* clock.c -- RT3, the network clock: a time server queried by name over UDP (NTP packets) from the registration toolkit's idle proc; the mac 1.1.1 librt3_nonag.a(clock.o) (mac only). Notes: Research/notes/rt3mac.md */
#include <netinet/in.h>
#include "rt3_lib.h"

#define kMicrosPerSecond 1000000
#define kOutOfMemory (-7004)
#define kBadParameters (-7001)
#define kNoAddress (-7108)
#define kTimedOut (-7008)

static unsigned char gClockInitialized;
static struct ClockQuery *gClockQueryList;
static NetSocket *gClockDatagram;
static void *gClockResolver;
static ULongLong gMicrosecondsAdjust;
static MutexList gClockResolverFreeQ;
static MutexList gClockResolverRecvQ;
static MutexList gClockDatagramFreeQ;
static MutexList gClockDatagramRecvQ;

// MAC: __ClockInitialize i386 0x000d61c9 ppc 0x000e1284
static void _ClockInitialize(void)
{
    ULong second;

    if (!gClockInitialized) {
        MemoryClear(&gClockResolverFreeQ, sizeof(MutexList));
        MemoryClear(&gClockResolverRecvQ, sizeof(MutexList));
        MemoryClear(&gClockDatagramFreeQ, sizeof(MutexList));
        MemoryClear(&gClockDatagramRecvQ, sizeof(MutexList));
        TimerGetMicroseconds();
        TimerGetMicroseconds();
        second = TimerGetUTCSeconds();
        while (second == TimerGetUTCSeconds())
            ;
        gMicrosecondsAdjust = TimerGetMicroseconds() % kMicrosPerSecond;
        gClockInitialized = 1;
    }
}

// MAC: _ClockMakeWideSeconds i386 0x000d6291 ppc 0x000e1348
ULongLong ClockMakeWideSeconds(ULong seconds, ULongLong micros)
{
    ULong fraction;

    if (!gClockInitialized)
        _ClockInitialize();
    micros -= gMicrosecondsAdjust;
    micros %= kMicrosPerSecond;
    fraction = ((ULong)micros * 4294967296.0 / 1000000.0);
    return ((ULongLong)seconds << 32) | fraction;
}

/* a time server query: the list of them is walked by the network callback */
typedef struct ClockQuery {
    struct ClockQuery *next;
    NetAddress address;
    char server[0x100];
    int32_t *errorOut;
    int32_t error;
    ULongLong *response;
    ULongLong *outA;
    ULongLong *outB;
    ULongLong sentWide;
    ULongLong sentMillis;
    unsigned char blocking;
    ULong attempts;
    ULong state;
} ClockQuery;

#define kQueryDone 5
#define kErrorDisposed (-7007)

// MAC: _ClockGetWideSeconds i386 0x000d6374 ppc 0x000e1448
ULongLong ClockGetWideSeconds(void)
{
    ULong seconds;
    ULongLong micros;

    if (!gClockInitialized)
        _ClockInitialize();
    seconds = TimerGetUTCSeconds();
    micros = TimerGetMicroseconds();
    return ClockMakeWideSeconds(seconds, micros);
}

/* the SNTP request: version 4, client mode, the same timestamp in the three time fields */
typedef struct SNTPPacket {
    unsigned char flags;
    unsigned char stratum;
    unsigned char poll;
    unsigned char precision;
    ULong rootDelay;
    ULong rootDispersion;
    ULong referenceID;
    ULongLong reference;
    ULongLong originate;
    ULongLong receive;
    ULongLong transmit;
} SNTPPacket;

// MAC: __ClockSendPacket i386 0x000d63b8 ppc 0x000e14a0
static void _ClockSendPacket(ClockQuery *query)
{
    int32_t err;
    int32_t e;
    NetPacket *packet;
    SNTPPacket *sntp;
    ULongLong now;

    if (query->state == 2 || query->state == 3) {
        query->state = 3;
        if (query->attempts > 2) {
            err = kTimedOut;
fail:
            if (query->errorOut != 0)
                goto store;
            goto state;
        }
        packet = (NetPacket *)QueueRemove(&gClockDatagramFreeQ);
        if (packet == 0) {
            err = NetworkQueueFill(&gClockDatagramFreeQ, 2, 4, sizeof(SNTPPacket));
            e = err;
            if (e == 0) {
                packet = (NetPacket *)QueueRemove(&gClockDatagramFreeQ);
                if (packet == 0)
                    e = kOutOfMemory;
            }
        }
        if (packet != 0) {
            AddressDuplicate(&packet->address, &query->address);
            sntp = (SNTPPacket *)packet->data;
            packet->length = sizeof(SNTPPacket);
            MemoryClear(sntp, sizeof(SNTPPacket));
            query->sentMillis = TimerGetMilliseconds();
            query->attempts++;
            sntp->flags = 0x23;
            sntp->stratum = 4;
            sntp->poll = 0xa;
            sntp->precision = 0xfa;
            now = ClockGetWideSeconds();
            query->sentWide = now;
            sntp->receive = sntp->transmit = sntp->originate = SWAP64(now);
            e = DatagramSend(gClockDatagram, packet);
            if (e != 0)
                goto sendfail;
        }
        if (e != 0) {
            err = e;
            goto fail;
        }
    }
    return;
store:
    *query->errorOut = err;
state:
    query->error = err;
    query->state = kQueryDone;
    return;
sendfail:
    QueueInsert(&gClockDatagramFreeQ, packet);
    err = e;
    goto fail;
}

/* the resolver's and the datagram socket's event proc, and the timer tick: events 2 (tick), 9 (name resolved), 12 and 13 (packet received) */
// MAC: __ClockNetworkCallback i386 0x000d66e4 ppc 0x000e1618
static void _ClockNetworkCallback(NetSocket *sock, void *ref, unsigned short event)
{
    ClockQuery **link;
    ClockQuery *query;
    NetPacket *packet;
    NetAddress *address;
    unsigned short port;
    ULong ip;
    int32_t err;
    ULongLong now;
    SNTPPacket *sntp;
    ClockQuery *match;

    switch (event) {
    case 2:
        link = &gClockQueryList;
        while ((query = *link) != 0) {
            switch (query->state) {
            case 3:
                if (TimerGetMilliseconds() - query->sentMillis > 3999)
                    _ClockSendPacket(query);
                break;
            case kQueryDone:
                if (!query->blocking) {
                    *link = query->next;
                    MemoryDeallocate(query);
                    link = &gClockQueryList;
                    continue;
                }
                break;
            }
            link = &(*link)->next;
        }
        break;
    case 9:
        while ((packet = (NetPacket *)QueueRemove(&gClockResolverRecvQ)) != 0) {
            query = *(ClockQuery **)(packet->link + 0x10);
            err = 0;
            if (query->state == 1) {
                query->state = 2;
                err = kNoAddress;
                address = &packet->address;
                if (!AddressIsEmpty(address)) {
                    AddressDecomposeTCPIP(address, &port, &ip);
                    AddressComposeTCPIP(&query->address, port != 0 ? port : htons(123), ip);
                    err = 0;
                    _ClockSendPacket(query);
                }
            }
            QueueInsert(&gClockResolverFreeQ, packet);
            if (err != 0) {
                if (query->errorOut != 0)
                    *query->errorOut = err;
                query->error = err;
                query->state = kQueryDone;
            }
        }
        break;
    case 12:
    case 13:
        while ((packet = (NetPacket *)QueueRemove(&gClockDatagramRecvQ)) != 0) {
            now = ClockGetWideSeconds();
            sntp = (SNTPPacket *)packet->data;
            match = 0;
            if (packet->length < sizeof(SNTPPacket) || (sntp->flags & 0x3f) != 0x24 || sntp->originate == 0)
                err = kBadParameters;
            else {
                sntp->rootDelay = SWAP32(sntp->rootDelay);
                sntp->rootDispersion = SWAP32(sntp->rootDispersion);
                sntp->referenceID = SWAP32(sntp->referenceID);
                sntp->reference = SWAP64(sntp->reference);
                sntp->originate = SWAP64(sntp->originate);
                sntp->receive = SWAP64(sntp->receive);
                sntp->transmit = SWAP64(sntp->transmit);
                err = 0;
                for (match = gClockQueryList; match != 0; match = match->next) {
                    if (match->sentWide == sntp->originate) {
                        if (match->state == 3) {
                            match->state = 4;
                            if (match->outB != 0)
                                *match->outB = now;
                            if (match->outA != 0)
                                *match->outA = match->sentWide;
                            *match->response = (sntp->receive >> 1) + (sntp->transmit >> 1);
                            match->state = kQueryDone;
                        }
                        break;
                    }
                }
            }
            QueueInsert(&gClockDatagramFreeQ, packet);
            if (err != 0) {
                /* BUG: match is null when the packet was rejected */
                if (match->errorOut != 0)
                    *match->errorOut = err;
                match->error = err;
                match->state = kQueryDone;
            }
        }
        break;
    }
}

extern void ClockQueryDispose(void);

// MAC: _ClockQueryServer i386 0x000d6e95 ppc 0x000e1a50
int32_t ClockQueryServer(char *server, ULong timeout, ULongLong *outA, ULongLong *outB, ULongLong *response, int32_t *errorOut)
{
    char *name;
    int32_t err = 0;
    ClockQuery *query = 0;
    NetPacket *packet;
    int32_t e;
    ULong deadline;

    SystemCheckCallback();
    if (server == 0 || *server == 0)
        server = "time.apple.com:123";
    if (outA != 0)
        *outA = 0;
    if (outB != 0)
        *outB = 0;
    if (errorOut != 0)
        *errorOut = 0;
    *response = 0;
    if (!gClockInitialized)
        _ClockInitialize();
    if (!NetworkStackGetLoaded(0)) {
        e = NetworkStackLoad(1);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    SystemSetQuitProc(ClockQueryDispose);
    if (gClockResolver == 0) {
        e = QueueCreate(&gClockResolverFreeQ);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&gClockResolverRecvQ);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = NetworkQueueFill(&gClockResolverFreeQ, 1, 4, 0x218);
        if (e != 0) {
            err = e;
            goto done;
        }
        gClockResolver = ResolverCreate(0, 0, _ClockNetworkCallback, &gClockResolverRecvQ, &gClockResolverFreeQ);
        if (gClockResolver == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = ResolverOpen(gClockResolver);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (gClockDatagram == 0) {
        e = QueueCreate(&gClockDatagramFreeQ);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = QueueCreate(&gClockDatagramRecvQ);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = NetworkQueueFill(&gClockDatagramFreeQ, 2, 4, sizeof(SNTPPacket));
        if (e != 0) {
            err = e;
            goto done;
        }
        gClockDatagram = DatagramCreate(0, 0, _ClockNetworkCallback, &gClockDatagramFreeQ, &gClockDatagramFreeQ, &gClockDatagramRecvQ, &gClockDatagramRecvQ);
        if (gClockDatagram == 0) {
            err = kOutOfMemory;
            goto done;
        }
        DatagramControl(gClockDatagram, 0, 0x6000, 1);
        e = DatagramOpen(gClockDatagram, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    err = kOutOfMemory;
    query = MemoryAllocate(sizeof(ClockQuery));
    if (query == 0)
        goto done;
    MemoryClear(query, sizeof(ClockQuery));
    name = query->server;
    StringCopySafe(name, server, sizeof(query->server));
    query->errorOut = errorOut;
    query->response = response;
    query->outA = outA;
    query->outB = outB;
    query->blocking = timeout != 0;
    query->next = gClockQueryList;
    gClockQueryList = query;
    if (query->state == 0) {
        query->state = 1;
        packet = (NetPacket *)QueueRemove(&gClockResolverFreeQ);
        if (packet == 0) {
            e = NetworkQueueFill(&gClockResolverFreeQ, 2, 4, sizeof(SNTPPacket));
            if (e == 0) {
                packet = (NetPacket *)QueueRemove(&gClockResolverFreeQ);
                if (packet == 0)
                    e = kOutOfMemory;
            }
        }
        if (packet != 0) {
            AddressComposeEmpty(&packet->address);
            StringCopySafe(packet->data - 8, name, 0x100);
            *(ClockQuery **)(packet->link + 0x10) = query;
            e = ResolveName(gClockResolver, packet);
            if (e != 0)
                goto insertFail;
        }
        if (e != 0) {
report:
            if (query->errorOut != 0)
                *query->errorOut = e;
            query->error = e;
            query->state = kQueryDone;
        }
    }
    err = 0;
    if (timeout != 0) {
        deadline = TimerGetUTCSeconds() + timeout;
        while (deadline > TimerGetUTCSeconds() && query->state != kQueryDone)
            SystemIdle();
        if (*query->response == 0) {
            err = kTimedOut;
            if (query->error == 0)
                goto done;
        }
        err = query->error;
    }
done:
    if ((err != 0 || timeout != 0) && query != 0) {
        if (query == gClockQueryList)
            gClockQueryList = query->next;
        MemoryDeallocate(query);
    }
    if ((timeout != 0) && gClockQueryList == 0)
        ClockQueryDispose();
    if (errorOut != 0)
        *errorOut = err;
    return err;
insertFail:
    QueueInsert(&gClockResolverFreeQ, packet);
    goto report;
}

// MAC: _ClockQueryDispose i386 0x000d6d88 ppc 0x000e1920
void ClockQueryDispose(void)
{
    ClockQuery *query;

    SystemCheckCallback();
    if (gClockResolver != 0) {
        ResolverClose(gClockResolver);
        ResolverDispose(gClockResolver);
        gClockResolver = 0;
        NetworkQueueEmpty(&gClockResolverFreeQ);
        NetworkQueueEmpty(&gClockResolverRecvQ);
        QueueDispose(&gClockResolverFreeQ);
        QueueDispose(&gClockResolverRecvQ);
    }
    if (gClockDatagram != 0) {
        DatagramClose(gClockDatagram, 0);
        DatagramDispose(gClockDatagram);
        gClockDatagram = 0;
        NetworkQueueEmpty(&gClockDatagramFreeQ);
        NetworkQueueEmpty(&gClockDatagramRecvQ);
        QueueDispose(&gClockDatagramFreeQ);
        QueueDispose(&gClockDatagramRecvQ);
    }
    while ((query = gClockQueryList) != 0) {
        gClockQueryList = query->next;
        if (query->state != kQueryDone && query->errorOut != 0)
            *query->errorOut = kErrorDisposed;
        MemoryDeallocate(query);
    }
    gClockInitialized = 0;
done:
    ;
}
