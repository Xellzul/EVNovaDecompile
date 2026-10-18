// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atnetup.c -- is there a usable network?. Notes: Research/notes/atnetup.md */

#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"


extern long _NetworkStackQueryInterfaces(void *buf, unsigned long *size);  /* 0x004f2b4e list */

typedef ULong (__stdcall *WSASocketProc)(int af, int type, int protocol,
                                         void *info, ULong group, ULong flags);
typedef int (__stdcall *WSAIoctlProc)(ULong s, ULong code, void *in, ULong inSize,
                                      void *out, ULong outSize, ULong *returned,
                                      void *overlapped, void *completion);

char gWinsockLibInfo[400];                          /* 0x0086d4f0 WSADATA */

#define kWinsock2Version        0x202
#define kProtoUdp               0x11
#define SIO_GET_INTERFACE_LIST  0x4004747f
#define kNetErrNoWinsock2       -7100
#define kNetErrStartup          -7101

#define kWinsockVersion 0x101
#define kListBuffer     0x390
#define kInterfaceBigA  0x4c
#define kInterfaceBigB  0x34
#define kFamilyInet     2
#define kFlagUp         1
#define kFlagLoopback   4

#define kModeEnumerate  0
#define kModeSocket     1
#define kProbeAf        0x10
#define kProbeType      2
#define kProbeProtocol  0x3e82

typedef struct IpAddrRow {
    ULong addr;
    ULong index;
    ULong mask;
    ULong broadcast;
    ULong reasmSize;
    unsigned short unused;
    unsigned short type;
} IpAddrRow;

typedef struct IpAddrTable {
    ULong count;
    IpAddrRow rows[1];
} IpAddrTable;

typedef ULong (__stdcall *GetIpAddrTableProc)(IpAddrTable *table, ULong *size, int order);

typedef struct SockAddrIn {
    unsigned short family;
    unsigned short port;
    ULong addr;
    char zero[8];
} SockAddrIn;

static GetIpAddrTableProc gGetIpAddrTableProc;          /* 0x00867498 */
static WSAIoctlProc gWSAIoctlProc;                /* 0x00867494 */
static WSASocketProc gWSASocketProc;               /* 0x00867490 */
ULong gStacks;                                   /* 0x0086ff98 the socket kinds that work */

#define kSocketKinds    32
#define kAnyKinds       0xffffffff
#define kLoopback       0x0100007f
#define kRouteProbe     0x01000080
#define kEchoPort       0x0700
#define kInetAddrBytes  0x10
#define kHostNameBytes  0x100
#define kSocketError    -1
#define kPasses         2

void NetworkStackUnload(void);
void NetworkStackTickle(void);

/* NetworkStackLoad (start the network: WSAStartup, the exit and idle procs, then every wanted
   socket kind that works; -7101 unless all wanted work, or any does when kAnyKinds).
   STATE: MATCH 100%, exe 70 / obj 70, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2a6a
long NetworkStackLoad(ULong wanted)
{
    ULong kind;
    long err, e;
    ULong version;

    err = 0;
    if (IsPlatformOpen() == 0) {
        e = PlatformOpen();
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    version = kWinsockVersion;
    e = WSAStartup(version, gWinsockLibInfo);
    if (e != 0) {
        err = e;
        err = kNetErrStartup;
    } else {
        SystemSetQuitProc(NetworkStackUnload);
        SystemSetIdleProc(NetworkStackTickle);
        for (kind = 0; kind < kSocketKinds; kind++) {
            if (AtomicMaskTest(&gStacks, kind) == 0 && AtomicMaskTest(&wanted, kind) != 0 &&
                NetworkStackGetAvail(kind) != 0)
                AtomicMaskApply(&gStacks, kind);
        }
        if (gStacks == 0)
            err = kNetErrStartup;
        else if (wanted != gStacks && wanted != kAnyKinds)
            err = kNetErrStartup;
    }
done:
    return err;
}

/* NetworkStackUnload (at exit: close every socket, forget the kinds, WSACleanup; no CSV row).
   STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2b2a
void NetworkStackUnload(void)
{
    long err, e;

    err = 0;
    _EndpointDisposeAll();
    gStacks = 0;
    e = WSACleanup();
    if (e != 0)
        err = e;
}

/* NetworkStackTickle (the idle proc: wake every socket; no CSV row). STATE: MATCH 100%, exe 4 / obj 4, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f2b44
void NetworkStackTickle(void)
{
    long err;

    err = 0;
    _EndpointTickleAll();
}

/* _NetworkStackQueryInterfaces. STATE: MATCH 100%, exe 95 / obj 95, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2b4e
long _NetworkStackQueryInterfaces(void *buf, unsigned long *size)
{
    char data[400];
    long err;
    long e;
    unsigned char started;
    ULong s;
    long result;

    err = 0;
    started = 0;
    result = 0;
    s = 0;
    if (gWSASocketProc == 0)
        gWSASocketProc = (WSASocketProc)GetProcAddress(GetModuleHandleA("WS2_32.dll"),
                                                     "WSASocketA");
    if (gWSAIoctlProc == 0)
        gWSAIoctlProc = (WSAIoctlProc)GetProcAddress(GetModuleHandleA("WS2_32.dll"),
                                                    "WSAIoctl");
    if (gWSASocketProc == 0 || gWSAIoctlProc == 0)
        return kNetErrNoWinsock2;
    if (buf == 0 || size == 0)
        return 0;

    e = WSAStartup(kWinsock2Version, data);
    if (e != 0) {
        err = e;
        err = kNetErrStartup;
    } else {
        started = 1;
        s = gWSASocketProc(kFamilyInet, SOCK_DGRAM, kProtoUdp, 0, 0, 0);
        if (s == 0) {
            err = WSAGetLastError();
            err = kNetErrNoWinsock2;
        } else {
            e = gWSAIoctlProc(s, SIO_GET_INTERFACE_LIST, 0, 0, buf, *size, size, 0, 0);
            if (e != 0) {
                err = e;
                err = kNetErrNoWinsock2;
            }
        }
    }

    if (s != 0)
        closesocket(s);
    if (started)
        WSACleanup();
    if (err != 0)
        *size = 0;
    return err;
}

/* NetworkStackGetAvail (can a socket of this kind be made: 0 a UDP socket, 1 the 0x10/0x3e82
   probe). STATE: MATCH 100%, exe 59 / obj 59, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2c78
unsigned char NetworkStackGetAvail(unsigned short mode)
{
    long err;
    unsigned char available;
    ULong s;
    unsigned short version;
    long e;

    err = 0;
    available = 0;
    version = kWinsockVersion;
    e = WSAStartup(version, gWinsockLibInfo);
    if (e != 0)
        err = e;
    if (err == 0) {
        switch (mode) {
        case kModeEnumerate:
            s = socket(AF_INET, SOCK_DGRAM, kProtoUdp);
            if (s == 0 || (long)s <= 0)
                goto cleanup;
            if (closesocket(s) < 0)
                err = WSAGetLastError();
            available = 1;
            break;
        case kModeSocket:
            s = socket(kProbeAf, kProbeType, kProbeProtocol);
            if (s == 0 || (long)s <= 0)
                goto cleanup;
            if (closesocket(s) < 0)
                err = WSAGetLastError();
            available = 1;
            break;
        default:
            available = 0;
            break;
        }
    cleanup:
        e = WSACleanup();
        if (e != 0)
            err = e;
    }
    return available;
}

typedef struct AddressGen {           /* sockaddr_gen */
    short family;
    unsigned short port;
    ULong address;
    char zero[16];
} AddressGen;

typedef struct InterfaceInfo {        /* INTERFACE_INFO */
    ULong flags;
    AddressGen address;
    AddressGen broadcast;
    AddressGen netmask;
} InterfaceInfo;

typedef struct AddressIn {            /* sockaddr_in */
    short family;
    unsigned short port;
    ULong address;
    char zero[8];
} AddressIn;

typedef struct InterfaceInfoOld {     /* the WinSock 1 layout */
    ULong flags;
    AddressIn address;
    AddressIn broadcast;
    AddressIn netmask;
} InterfaceInfoOld;

/* NetworkStackGetActive. STATE: MATCH 100%, exe 126 / obj 126, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2d0f
char NetworkStackGetActive(unsigned short mode)
{
    char list[kListBuffer];
    unsigned long size;
    long err;
    char available;
    unsigned short version;
    long e;
    ULong s;

    err = 0;
    available = 0;
    version = kWinsockVersion;
    e = WSAStartup(version, gWinsockLibInfo);
    if (e != 0)
        err = e;
    if (err == 0) {
        switch (mode) {
        case kModeEnumerate:
            size = kListBuffer;
            e = _NetworkStackQueryInterfaces(list, &size);
            if (e != 0)
                err = e;
            if (size % sizeof(InterfaceInfo) == 0) {
                ULong i;
                ULong count;
                InterfaceInfo *info;
                ULong flags;

                count = size / sizeof(*info);
                info = (InterfaceInfo *)list;
                for (i = 0; i < count && available == 0; i++) {
                    if (info[i].address.family == kFamilyInet) {
                        flags = info[i].flags;
                        if ((flags & kFlagLoopback) == 0 && (flags & kFlagUp) != 0)
                            available = 1;
                    }
                }
            } else if (size % sizeof(InterfaceInfoOld) == 0) {
                ULong i;
                ULong count;
                InterfaceInfoOld *info;
                ULong flags;

                count = size / sizeof(*info);
                info = (InterfaceInfoOld *)list;
                for (i = 0; i < count && available == 0; i++) {
                    if (info[i].address.family == kFamilyInet) {
                        flags = info[i].flags;
                        if ((flags & kFlagLoopback) == 0 && (flags & kFlagUp) != 0)
                            available = 1;
                    }
                }
            }
            break;
        case kModeSocket:
            s = 0;
            s = socket(kProbeAf, kProbeType, kProbeProtocol);
            if (s != 0) {
                if (closesocket(s) < 0)
                    err = WSAGetLastError();
                available = 1;
            }
            break;
        default:
            available = 0;
            break;
        }
        e = WSACleanup();
        if (e != 0)
            err = e;
    }
    return available;
}

/* NetworkStackGetAddressList (this machine's IPv4 addresses, loopback last: GetIpAddrTable, else the
   Winsock 2 interface list, else the host name, else the route to 128.0.0.1). STATE:
   MATCH 100%, exe 380 / obj 380, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2e98
ULong NetworkStackGetAddressList(unsigned short kind, NetAddress *out, ULong max)
{
    char name[kHostNameBytes];
    SockAddrIn local;
    SockAddrIn probe;
    IpAddrTable *table;
    ULong count;
    AddressGen *addrGen;
    AddressIn *addrIn;
    ULong addr;
    ULong j;
    ULong pass;
    ULong i;
    ULong s;
    long err;
    long e;
    HostEnt *host;

    err = 0;
    count = 0;
    table = 0;
    host = 0;
    s = 0;
    AtomicMaskTest(&gStacks, kind);
    for (i = 0; i < max; i++)
        AddressComposeEmpty(&out[i]);
    if (kind != kModeEnumerate) {
        err = kNetErrNoWinsock2;
        goto done;
    }
    if (gGetIpAddrTableProc == 0)
        gGetIpAddrTableProc = (GetIpAddrTableProc)GetProcAddress(GetModuleHandleA("Iphlpapi.dll"),
                                                          "GetIpAddrTable");
    if (gGetIpAddrTableProc != 0) {
        unsigned long size;

        size = 0;
        gGetIpAddrTableProc(0, &size, 0);
        table = MemoryAllocate(size);
        if (table == 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = gGetIpAddrTableProc(table, &size, 0);
        if (e != 0) {
            err = e;
            err = kNetErrNoWinsock2;
            goto done;
        }
        for (pass = 0; pass < kPasses && count < max; pass++) {
            for (j = 0; j < table->count && count < max; j++) {
                addr = table->rows[j].addr;
                if (addr != 0 && (pass != 0 || addr != kLoopback) &&
                    (pass == 0 || addr == kLoopback)) {
                    AddressComposeTCPIP(out++, 0, addr);
                    count++;
                }
            }
        }
        goto done;
    }

    err = _NetworkStackQueryInterfaces(0, 0);
    if (err == 0) {
        char list[kListBuffer];
        unsigned long size;

        size = kListBuffer;
        e = _NetworkStackQueryInterfaces(list, &size);
        if (e != 0)
            err = e;
        if (size % kInterfaceBigA == 0) {
            InterfaceInfo *info;
            ULong entries;
            ULong flags;

            entries = size / kInterfaceBigA;
            info = (InterfaceInfo *)list;
            for (pass = 0; pass < kPasses && count < max; pass++) {
                for (j = 0; j < entries && count < max; j++) {
                    flags = info[j].flags;
                    if ((flags & kFlagUp) != 0 && (pass != 0 || (flags & kFlagLoopback) == 0) &&
                        (pass == 0 || (flags & kFlagLoopback) != 0) &&
                        info[j].address.family == kFamilyInet &&
                        (addrGen = &info[j].address)->address != 0) {
                        out->kind = kInetAddrBytes;
                        BlockMoveData(addrGen, &out->family, out->kind);
                        out++;
                        count++;
                    }
                }
            }
        } else if (size % kInterfaceBigB == 0) {
            InterfaceInfoOld *info;
            ULong entries;
            ULong flags;

            entries = size / kInterfaceBigB;
            info = (InterfaceInfoOld *)list;
            for (pass = 0; pass < kPasses && count < max; pass++) {
                for (j = 0; j < entries && count < max; j++) {
                    flags = info[j].flags;
                    if ((flags & kFlagUp) != 0 && (pass != 0 || (flags & kFlagLoopback) == 0) &&
                        (pass == 0 || (flags & kFlagLoopback) != 0) &&
                        info[j].address.family == kFamilyInet &&
                        (addrIn = &info[j].address)->address != 0) {
                        out->kind = kInetAddrBytes;
                        BlockMoveData(addrIn, &out->family, out->kind);
                        out++;
                        count++;
                    }
                }
            }
        }
        goto done;
    }

    if (gethostname(name, kHostNameBytes) != kSocketError &&
        (host = gethostbyname(name)) != 0) {
        for (i = 0; max > count && host->addrList[i] != 0; i++) {
            AddressComposeTCPIP(out++, 0, ((ULong *)host->addrList[0])[i]);
            count++;
        }
        if (max > count)
            AddressComposeTCPIP(out++, 0, kLoopback);
        count++;
    } else {
        int length;

        memset(&local, 0, sizeof(local));
        memset(&probe, 0, sizeof(probe));
        probe.family = kFamilyInet;
        probe.port = kEchoPort;
        probe.addr = kRouteProbe;
        s = socket(kFamilyInet, SOCK_DGRAM, kProtoUdp);
        if (s != kNoSocket) {
            length = sizeof(local);
            if (connect(s, &probe, sizeof(probe)) != kSocketError)
                getsockname(s, &local, &length);
            if (length == 0)
                memset(&local, 0, sizeof(local));
            closesocket(s);
            s = 0;
        }
        if (local.addr != 0) {
            if (count < max)
                AddressComposeTCPIP(out++, 0, local.addr);
            if (count + 1 < max)
                AddressComposeTCPIP(out++, 0, kLoopback);
            count += 2;
        }
    }
done:
    if (table != 0)
        MemoryDeallocate(table);
    /* BUG: the error codes gathered on the way are never returned */
    return count;
}
