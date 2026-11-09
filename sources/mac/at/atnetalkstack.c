/* atnetalkstack.c -- 1.0.10: AppleTalk's stack without Open Transport and the unique name: an object of their own by DatagramOpenDDP's backward `bl; nop`. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <AppleTalk.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON

#define kNetErrStartup      -7101
#define kDriverNameOffset   0x12

static short UnitCount(void)
{
    return *(short *)0x1d2;
}

long StackOpenAppleTalk(void)
{
    long err;

    err = 0;
    if (MPPOpen() != 0)
        err = kNetErrStartup;
    return err;
}

void StackCloseAppleTalk(void)
{
    _EndpointDisposeAll();
}

void StackTickleAppleTalk(void)
{
    _EndpointTickleAll();
}

unsigned char StackGetAvailAppleTalk(unsigned short mode)
{
    unsigned char mpp;
    unsigned char dsp;
    DCtlHandle h;
    Str255 name;
    short refNum;
    StringPtr driver;

    mpp = 0;
    dsp = 0;
    for (refNum = -UnitCount(); refNum < 0; refNum++) {
        h = GetDCtlEntry(refNum);
        if (h == 0)
            continue;
        if (((*h)->dCtlFlags & dRAMBasedMask) != 0)
            driver = *(StringHandle)(*h)->dCtlDriver;
        else
            driver = (StringPtr)(*h)->dCtlDriver;
        driver += kDriverNameOffset;
        BlockMoveData(driver, name, *driver + 1);
        if (EqualString(name, "\p.MPP", false, true))
            mpp = ((*h)->dCtlFlags & dOpenedMask) != 0;
        if (EqualString(name, "\p.DSP", false, true))
            dsp = ((*h)->dCtlFlags & dOpenedMask) != 0;
        if (mpp && dsp)
            break;
    }
    return mpp && dsp;
}

ULong StackGetAddressListAppleTalk(unsigned short kind, NetAddress *out, ULong max)
{
    unsigned short net;
    unsigned short node;
    NetAddress *p;
    ULong i;
    ULong count;

    count = 0;
    IsMPPOpen();
    p = out;
    for (i = 0; i < max; i++) {
        AddressComposeEmpty(p);
        p++;
    }
    if (GetNodeAddress((short *)&node, (short *)&net) == 0) {
        if (count < max)
            AddressComposeAppleTalk(out, net, node, 0, 0);
        count = 1;
    }
    return count;
}

/* MSL's strcpy and strcat, which the exe has inline */
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

static const char *sHexDigits = "0123456789ABCDEF";

void MakeUniqueNBPName(const unsigned char *name, unsigned char *out, ULong serial)
{
    long n;

    IsMPPOpen();
    MslCopy((char *)out, (const char *)name);
    n = (char)name[0] + 1;
    if (n != 0)
        BlockMoveData(name, out, n);
    if (serial != 0) {
        ULong v;
        long m;

        v = (serial * 2 + 1) * TickCount();
        out[n] = '-';
        m = n + 1;
        out[m++] = sHexDigits[(v >> 12) & 0xf];
        out[m++] = sHexDigits[(v >> 8) & 0xf];
        out[m++] = sHexDigits[(v >> 4) & 0xf];
        out[m++] = sHexDigits[v & 0xf];
        out[0] = m - 1;
        out[m] = 0;
    }
    MslAppend((char *)out, (const char *)name + n);
}
#endif
