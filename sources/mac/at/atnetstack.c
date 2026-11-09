/* atnetstack.c -- 1.0.10: is there a usable network? Open Transport first, else MacTCP and AppleTalk, per stack. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "atnetmac.h"

#define kStackBitInet       1
#define kStackBitAppleTalk  2
#define kAnyKinds           0xffffffff
#define kNetErrStartup      -7101

#if !TARGET_API_MAC_CARBON

unsigned char gAppleTalkUsesOT;
unsigned char gInetUsesOT;
static ULong gStacks;

void NetworkStackUnload(void);
void NetworkStackTickle(void);

long NetworkStackLoad(ULong wanted)
{
    long err, e;

    err = 0;
    if (IsPlatformOpen() == 0) {
        e = PlatformOpen();
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if ((wanted & kStackBitInet) != 0) {
        gInetUsesOT = InetAvailableOT();
        if (gInetUsesOT == 0)
            e = StackOpenMacTCP();
        else
            e = StackOpenOT(kStackBitInet);
        if (e != 0)
            err = e;
        if (err == 0)
            gStacks |= kStackBitInet;
        else
            err = 0;
    }
    if ((wanted & kStackBitAppleTalk) != 0) {
        gAppleTalkUsesOT = AppleTalkAvailableOT();
        if (gAppleTalkUsesOT == 0)
            e = StackOpenAppleTalk();
        else
            e = StackOpenOT(kStackBitAppleTalk);
        if (e != 0)
            err = e;
        if (err == 0)
            gStacks |= kStackBitAppleTalk;
        else
            err = 0;
    }
    _SystemUnpatchTraps();
    SystemSetQuitProc(NetworkStackUnload);
    SystemSetIdleProc(NetworkStackTickle);
    if (gStacks == 0) {
        err = kNetErrStartup;
    } else {
        wanted &= ~gStacks;
        if (wanted != 0 && wanted != kAnyKinds)
            err = kNetErrStartup;
    }
done:
    return err;
}

void NetworkStackUnload(void)
{
    if ((gStacks & kStackBitAppleTalk) != 0) {
        if (gAppleTalkUsesOT == 0)
            StackCloseAppleTalk();
        else
            StackCloseOT();
        gStacks = gStacks & ~kStackBitAppleTalk;
    }
    if ((gStacks & kStackBitInet) != 0) {
        if (gInetUsesOT == 0)
            StackCloseMacTCP();
        else
            StackCloseOT();
        gStacks = gStacks & ~kStackBitInet;
    }
}

void NetworkStackTickle(void)
{
    unsigned char tickled;

    tickled = 0;
    /* BUG: tests the Inet bit, not AppleTalk's */
    if ((gStacks & kStackBitInet) != 0) {
        if (gAppleTalkUsesOT == 0) {
            StackTickleAppleTalk();
        } else {
            StackTickleOT();
            tickled = 1;
        }
    }
    if ((gStacks & kStackBitInet) != 0) {
        if (gInetUsesOT == 0)
            StackTickleMacTCP();
        else if (!tickled)
            StackTickleOT();
    }
}

unsigned char NetworkStackGetActive(unsigned short mode)
{
    unsigned char available;

    available = 0;
    if ((gStacks & (1 << mode)) != 0) {
        available = 1;
    } else {
        switch (mode) {
        case kStackInet:
            if (InetAvailableOT())
                available = StackGetAvailOT(mode);
            else
                available = StackGetAvailMacTCP(mode);
            /* BUG: no break, the answer is AppleTalk's */
        case kStackAppleTalk:
            if (AppleTalkAvailableOT())
                available = StackGetAvailOT(mode);
            else
                available = StackGetAvailAppleTalk(mode);
            break;
        }
    }
    return available;
}

ULong NetworkStackGetAddressList(unsigned short kind, NetAddress *out, ULong max)
{
    NetAddress *p;
    ULong i;
    ULong count;

    p = out;
    count = 0;
    for (i = 0; i < max; i++) {
        AddressComposeEmpty(p);
        p++;
    }
    if ((gStacks & (1 << kind)) != 0) {
        if (kind == kStackInet) {
            count = gInetUsesOT ? StackGetAddressListOT(kind, out, max) : StackGetAddressListMacTCP(kind, out, max);
        } else if (kind == kStackAppleTalk) {
            count = gAppleTalkUsesOT ? StackGetAddressListOT(kind, out, max) : StackGetAddressListAppleTalk(kind, out, max);
        }
    }
    return count;
}


#else /* Carbon */

unsigned char gAppleTalkUsesOT;
unsigned char gInetUsesOT;
static ULong gStacks;

void NetworkStackUnload(void);
void NetworkStackTickle(void);

long NetworkStackLoad(ULong wanted)
{
    long err, e;

    err = 0;
    if (IsPlatformOpen() == 0) {
        e = PlatformOpen();
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if ((wanted & kStackBitInet) != 0) {
        gInetUsesOT = InetAvailableOT();
        e = StackOpenOT(kStackBitInet);
        if (e != 0) {
            err = e;
            goto done;
        }
        gStacks |= kStackBitInet;
    }
    if ((wanted & kStackBitAppleTalk) != 0) {
        gAppleTalkUsesOT = AppleTalkAvailableOT();
        e = StackOpenOT(kStackBitAppleTalk);
        if (e != 0) {
            err = e;
            goto done;
        }
        gStacks |= kStackBitAppleTalk;
    }
    _SystemUnpatchTraps();
    SystemSetQuitProc(NetworkStackUnload);
    SystemSetIdleProc(NetworkStackTickle);
    if (gStacks == 0) {
        err = kNetErrStartup;
    } else {
        wanted &= ~gStacks;
        if (wanted != 0 && wanted != kAnyKinds)
            err = kNetErrStartup;
    }
done:
    return err;
}

void NetworkStackUnload(void)
{
    if ((gStacks & kStackBitAppleTalk) != 0) {
        StackCloseOT();
        gStacks = gStacks & ~kStackBitAppleTalk;
    }
    if ((gStacks & kStackBitInet) != 0) {
        StackCloseOT();
        gStacks = gStacks & ~kStackBitInet;
    }
}

void NetworkStackTickle(void)
{
    unsigned char tickled;

    tickled = 0;
    /* BUG: tests the Inet bit, not AppleTalk's */
    if ((gStacks & kStackBitInet) != 0) {
        StackTickleOT();
        tickled = 1;
    }
    if ((gStacks & kStackBitInet) != 0) {
        if (!tickled)
            StackTickleOT();
    }
}

unsigned char NetworkStackGetActive(unsigned short mode)
{
    unsigned char available;

    if ((gStacks & (1 << mode)) != 0)
        available = 1;
    else
        available = StackGetAvailOT(mode);
    return available;
}

ULong NetworkStackGetAddressList(unsigned short kind, NetAddress *out, ULong max)
{
    NetAddress *p;
    ULong i;
    ULong count;

    p = out;
    count = 0;
    for (i = 0; i < max; i++) {
        AddressComposeEmpty(p);
        p++;
    }
    if ((gStacks & (1 << kind)) != 0)
        count = StackGetAddressListOT(kind, out, max);
    return count;
}

#endif
