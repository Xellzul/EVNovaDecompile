/* atnetotstack.c -- 1.0.10: the network layer's stack on Open Transport, what is available and its addresses, and the names a socket registers. Notes: Research/notes/maclibs.md */
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

extern int32_t CriticalCreateFromProvider(void *lock, ProviderRef provider);

#if TARGET_API_MAC_CARBON
#define InitOpenTransport() InitOpenTransportInContext(kInitOTForApplicationMask, 0)
#define CloseOpenTransport() CloseOpenTransportInContext(0)
#define OTOpenEndpoint(c, o, i, e) OTOpenEndpointInContext(c, o, i, e, 0)
#define OTOpenMapper(c, o, e) OTOpenMapperInContext(c, o, e, 0)
#define MemoryClear(p, n) BlockZero(p, n)
#define kOTEndpointProc OTOpenEndpointInContext
#define kOTInetProc OTOpenInternetServicesInContext
#define kOTATalkProc OTOpenAppleTalkServicesInContext
#define kResolverNotifier gResolverNotifierUPP
#define kDatagramNotifier gDatagramNotifierUPP
#define kStreamNotifier gStreamNotifierUPP
#else
#define kResolverNotifier ResolverNotifierOT
#define kDatagramNotifier DatagramNotifierOT
#define kStreamNotifier StreamNotifierOT
#define kOTEndpointProc OTOpenEndpoint
#define kOTInetProc OTOpenInternetServices
#define kOTATalkProc OTOpenAppleTalkServices
#endif

#define kNetErrStartup      -7101
#define kNameResource       -16413
#define kNetErrConfig       -7100
#define kBadSyncState       -3155
#define kLoopback           0x7f000001

#define kOTVersion111 0x01110000
#define kOTVersion130 0x01300000

extern unsigned char gOTOpen;
unsigned char gOTInetOpen = 0;
unsigned char gOTAppleTalkOpen = 0;
EndpointRef gOTInetEndpoint = 0;
EndpointRef gOTAppleTalkEndpoint = 0;
#if TARGET_API_MAC_CARBON
extern unsigned char gOTContextOpen;
OTNotifyUPP gResolverNotifierUPP = 0;
OTNotifyUPP gDatagramNotifierUPP = 0;
OTNotifyUPP gStreamNotifierUPP = 0;
extern pascal void ResolverNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie);
extern pascal void DatagramNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie);
extern pascal void StreamNotifierOT(void *ref, OTEventCode code, OTResult result, void *cookie);
#endif

int32_t StackOpenOT(ULong wanted)
{
    OSStatus status;
    int32_t response;
    ULong wantInet;
    ULong wantAppleTalk;
    int32_t err;
    OTConfigurationRef config;

    err = 0;
    status = 0;
    response = 0;
    if (IsPlatformOpen() == 0) {
        int32_t e = PlatformOpen();
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (Gestalt(gestaltOpenTpt, &response) != 0) {
        err = kNetErrStartup;
        goto done;
    }
    if ((response & gestaltOpenTptPresentMask) == 0) {
        err = kNetErrStartup;
        goto done;
    }
    if (Gestalt(gestaltOpenTptVersions, &response) != 0) {
        err = kNetErrStartup;
        goto done;
    }
    if (response < kOTVersion111) {
        err = kNetErrStartup;
        goto done;
    }
    wantInet = wanted & 1;
    if (wantInet != 0 && InetAvailableOT() == 0) {
        err = kNetErrStartup;
        goto done;
    }
    wantAppleTalk = wanted & 2;
    if (wantAppleTalk != 0 && AppleTalkAvailableOT() == 0) {
        err = kNetErrStartup;
        goto done;
    }
#if TARGET_API_MAC_CARBON
    if (gOTOpen == 0 && gOTContextOpen == 0 && InitOpenTransport() != 0) {
        err = kNetErrStartup;
        goto done;
    }
    gOTContextOpen = 1;
    gOTOpen = 1;
    if (gResolverNotifierUPP == 0 && (gResolverNotifierUPP = NewOTNotifyUPP(ResolverNotifierOT)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (gDatagramNotifierUPP == 0 && (gDatagramNotifierUPP = NewOTNotifyUPP(DatagramNotifierOT)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (gStreamNotifierUPP == 0 && (gStreamNotifierUPP = NewOTNotifyUPP(StreamNotifierOT)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
#else
    if (kOTEndpointProc == 0) {
        err = kNetErrStartup;
        goto done;
    }
    if (gOTOpen == 0 && InitOpenTransport() != 0) {
        err = kNetErrStartup;
        goto done;
    }
    gOTOpen = 1;
#endif
    if (wantInet != 0) {
        if (InetInitOT() == 0) {
            err = kNetErrStartup;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        if (!_PlatformIsThreaded()) {
#endif
        config = OTCreateConfiguration("udp");
        if (config == 0) {
            err = kNetErrConfig;
            goto done;
        }
        gOTInetEndpoint = OTOpenEndpoint(config, 0, 0, &status);
        if (gOTInetEndpoint == 0) {
            err = kNetErrStartup;
            goto done;
        }
        if (OTBind(gOTInetEndpoint, 0, 0) != 0) {
            err = kNetErrStartup;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        }
#endif
        gOTInetOpen = 1;
    }
    if (wantAppleTalk != 0) {
        if (AppleTalkInitOT() == 0) {
            err = kNetErrStartup;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        if (!_PlatformIsThreaded()) {
#endif
        config = OTCreateConfiguration("ddp");
        if (config == 0) {
            err = kNetErrConfig;
            goto done;
        }
        gOTAppleTalkEndpoint = OTOpenEndpoint(config, 0, 0, &status);
        if (gOTAppleTalkEndpoint == 0) {
            err = kNetErrStartup;
            goto done;
        }
        if (OTBind(gOTAppleTalkEndpoint, 0, 0) != 0) {
            err = kNetErrStartup;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        }
#endif
        gOTAppleTalkOpen = 1;
    }
done:
    return err;
}

void StackCloseOT(void)
{
    OSStatus status, e;

    status = 0;
    if (gOTAppleTalkEndpoint != 0) {
        e = OTSetSynchronous(gOTAppleTalkEndpoint);
        if (e != 0)
            status = e;
        if (status != kBadSyncState) {
            e = OTCloseProvider(gOTAppleTalkEndpoint);
            if (e != 0)
                status = e;
        }
        gOTAppleTalkEndpoint = 0;
    }
    gOTAppleTalkOpen = 0;
    if (gOTInetEndpoint != 0) {
        e = OTSetSynchronous(gOTInetEndpoint);
        if (e != 0)
            status = e;
        if (status != kBadSyncState)
            OTCloseProvider(gOTInetEndpoint);
        gOTInetEndpoint = 0;
    }
    gOTInetOpen = 0;
#if TARGET_API_MAC_CARBON
    if (!_PlatformIsThreaded()) {
        if (gOTOpen != 0)
            CloseOpenTransport();
        gOTContextOpen = 0;
    }
#else
    if (gOTOpen != 0)
        CloseOpenTransport();
#endif
    gOTOpen = 0;
}

void StackTickleOT(void)
{
    _EndpointTickleAll();
}

unsigned char StackGetAvailOT(unsigned short mode)
{
    unsigned char available;

    available = 0;
    switch (mode) {
    case kStackInet: {
        int32_t ok = 0;
        if (InetAvailableOT() && InetInitOT() && InetActiveOT())
            ok = 1;
        available = ok;
        break;
    }
    case kStackAppleTalk: {
        int32_t ok = 0;
        if (AppleTalkAvailableOT() && AppleTalkInitOT() && AppleTalkActiveOT())
            ok = 1;
        available = ok;
        break;
    }
    }
    return available;
}

ULong StackGetAddressListOT(unsigned short kind, NetAddress *out, ULong max)
{
    ULong k;
    ULong count;
    void **list;
    ULong *secondary;
    void *services;
    ULong i;

    count = 0;
    secondary = 0;
    services = 0;
    for (i = 0; i < max; i++)
        AddressComposeEmpty(&out[i]);
    switch (kind) {
    case kStackInet: {
        ULong loopback;
        int32_t response;
        int32_t interface;
        unsigned char hasSecondary;
        InetInterfaceInfo info;

        loopback = kLoopback;
        hasSecondary = 0;
        if (Gestalt(gestaltOpenTptVersions, &response) == 0 && response >= kOTVersion130 && OTInetGetSecondaryAddresses != 0)
            hasSecondary = 1;
        list = IndirectAllocate(0);
        if (list == 0)
            break;
        interface = -1;
        while (OTInetGetInterfaceInfo(&info, interface) == 0) {
            if (info.fAddress != 0) {
                ULong n;
                ULong j;

                j = 0;
                n = IndirectGetSize(list) >> 2;
                while (j < n && info.fAddress != ((ULong *)*list)[j])
                    j++;
                if (j == n && IndirectAppendData(list, &info.fAddress, 4) != 0)
                    goto done;
                if (hasSecondary && info.fIPSecondaryCount != 0) {
                    if ((secondary = MemoryAllocate(info.fIPSecondaryCount << 2)) == 0)
                        goto done;
                    if (OTInetGetSecondaryAddresses(secondary, &info.fIPSecondaryCount, interface) != 0)
                        goto done;
                    for (k = 0; k < info.fIPSecondaryCount; k++) {
                        if (secondary[k] != kLoopback) {
                            j = 0;
                            n = IndirectGetSize(list) >> 2;
                            while (j < n && ((ULong *)*list)[j] != secondary[k])
                                j++;
                            if (j == n && IndirectAppendData(list, &secondary[k], 4) != 0)
                                goto done;
                        }
                    }
                    MemoryDeallocate(secondary);
                    secondary = 0;
                }
            }
            interface++;
        }
        if (IndirectAppendData(list, &loopback, 4) != 0)
            goto done;
        count = IndirectGetSize(list) >> 2;
        if (count < max)
            max = count;
        for (i = 0; i < max; i++)
            AddressComposeTCPIP(out++, 0, ((ULong *)*list)[i]);
        break;
    }
    case kStackAppleTalk: {
        OSStatus status;
        TNetbuf buffer;
        AppleTalkInfo info;

        status = 0;
        if ((services = OTOpenAppleTalkServices(kDefaultAppleTalkServicesPath, 0, &status)) == 0)
            break;
        buffer.maxlen = sizeof(info);
        buffer.len = sizeof(info);
        buffer.buf = (UInt8 *)&info;
        if (OTATalkGetInfo(services, &buffer) != 0)
            break;
        if (count < max)
            AddressComposeAppleTalk(out, info.fOurAddress.fNetwork, info.fOurAddress.fNodeID, 0, 0);
        count = 1;
        break;
    }
    }
done:
    if (secondary != 0)
        MemoryDeallocate(secondary);
    if (services != 0)
        OTCloseProvider(services);
    return count;
}

unsigned char InetAvailableOT(void)
{
    int32_t response;
    unsigned char ok;

    ok = 1;
    if (Gestalt(gestaltOpenTpt, &response) != 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptPresentMask) == 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptTCPPresentMask) == 0)
        ok = 0;
    if (ok && kOTEndpointProc == 0)
        ok = 0;
    if (ok && kOTInetProc == 0)
        ok = 0;
    return ok;
}

unsigned char InetInitOT(void)
{
    unsigned char ok;

    ok = 1;
    if (InetAvailableOT() == 0)
        ok = 0;
    if (ok && gOTOpen == 0 && InitOpenTransport() != 0)
        ok = 0;
    else
        gOTOpen = 1;
    return ok;
}

unsigned char InetActiveOT(void)
{
    InetInterfaceInfo info;
    int32_t response;
    unsigned char ok;

    ok = 1;
    if (Gestalt(gestaltOpenTpt, &response) != 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptPresentMask) == 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptTCPPresentMask) == 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptTCPLoadedMask) == 0)
        ok = 0;
    if (ok && OTInetGetInterfaceInfo(&info, 0) != 0)
        ok = 0;
    return ok;
}

unsigned char AppleTalkAvailableOT(void)
{
    int32_t response;
    unsigned char ok;

    ok = 1;
    if (Gestalt(gestaltOpenTpt, &response) != 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptPresentMask) == 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptAppleTalkPresentMask) == 0)
        ok = 0;
    if (ok && kOTEndpointProc == 0)
        ok = 0;
    if (ok && kOTATalkProc == 0)
        ok = 0;
#if TARGET_API_MAC_CARBON
    if (ok && _PlatformIsThreaded() && (response & gestaltOpenTptAppleTalkLoadedMask) == 0)
        ok = 0;
#endif
    return ok;
}

unsigned char AppleTalkInitOT(void)
{
    unsigned char ok;

    ok = 1;
    if (AppleTalkAvailableOT() == 0)
        ok = 0;
    return ok;
}

unsigned char AppleTalkActiveOT(void)
{
    int32_t response;
    unsigned char ok;

    ok = 1;
    if (Gestalt(gestaltOpenTpt, &response) != 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptPresentMask) == 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptAppleTalkPresentMask) == 0)
        ok = 0;
    if (ok && (response & gestaltOpenTptAppleTalkLoadedMask) == 0)
        ok = 0;
    return ok;
}


static const char *sHexDigitsOT = "0123456789ABCDEF";

/* MSL's strlen, which the exe has inline */
static inline int32_t MslLength(const char *s)
{
    const unsigned char *p;
    int32_t n;

    p = (const unsigned char *)s - 1;
    n = -1;
    do {
        n++;
    } while (*++p != 0);
    return n;
}

void MakeUniqueNBPNameOT(const char *name, char *out, ULong serial)
{
    char *mark;
    ULong len;
    int32_t n;

#if !TARGET_API_MAC_CARBON
    IsMPPOpen();
#endif
    mark = StringFindDelimiter(name, ":");
    len = mark != 0 ? mark - name : MslLength(name);
    n = len;
    if (len != 0)
        BlockMoveData(name, out, len);
    if (serial != 0) {
        ULong v;

        v = (serial * 2 + 1) * TickCount();
        out[n++] = '-';
        out[n++] = sHexDigitsOT[(v >> 12) & 0xf];
        out[n++] = sHexDigitsOT[(v >> 8) & 0xf];
        out[n++] = sHexDigitsOT[(v >> 4) & 0xf];
        out[n++] = sHexDigitsOT[v & 0xf];
    }
    if (mark != 0) {
        const unsigned char *p;
        unsigned char *q;

        q = (unsigned char *)(out + n - 1);
        p = (const unsigned char *)mark - 1;
        while ((*++q = *++p) != 0)
            ;
    } else {
        out[n] = 0;
    }
}

#define kNoHostName -7108
#define kHostNameBytes 0x100

extern void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName);

static inline void PascalToC(char *dst, const unsigned char *src)
{
    ULong n;

    n = *src + 1;
    dst--;
    while (--n != 0)
        *++dst = *++src;
    dst[1] = 0;
}

int32_t GetNameResource(char *out)
{
    int32_t err;
    StringHandle h;
#if TARGET_API_MAC_CARBON
    static int32_t (*sGetHostName)(char *name, int32_t size);
    char name[kHostNameBytes];
#endif

    err = 0;
#if TARGET_API_MAC_CARBON
    h = 0;
    out[0] = 0;
    if (_PlatformIsThreaded()) {
        if (sGetHostName == 0 && (sGetHostName = _PlatformFindBundleSymbol(CFSTR("System.framework"), CFSTR("gethostname"))) == 0) {
            err = kNoHostName;
            goto done;
        }
        if (sGetHostName(name, kHostNameBytes) == -1) {
            err = kNoHostName;
            goto done;
        }
        StringCopySafe(out, name, kHostNameBytes);
        goto done;
    }
#else
    out[0] = 0;
#endif
    h = GetString(kNameResource);
    if (h == 0) {
        ResError();
        err = kOutOfMemory;
    } else {
        PascalToC(out, *h);
    }
done:
    if (h != 0)
        ReleaseResource((Handle)h);
    return err;
}
