/* atnettcpstack.c -- 1.0.10: the MacTCP stack: the driver, its availability and addresses; an object of its own by its TC run. Notes: Research/notes/maclibs.md */
#include <Devices.h>
#include <MacTCP.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atnetmac.h"

#if !TARGET_API_MAC_CARBON

#define kNetErrStartup      -7101
#define kDriverNameOffset   0x12
#define kLoopback           0x7f000001
#define kMacTCPVersion      'mtcp'

short gMacTCPRefNum;

static short UnitCount(void)
{
    return *(short *)0x1d2;
}

long StackOpenMacTCP(void)
{
    long response;
    long err;

    err = 0;
    response = 0;
    if (Gestalt(kMacTCPVersion, &response) != 0)
        err = kNetErrStartup;
    else if (OpenDriver("\p.ipp", &gMacTCPRefNum) != 0)
        err = kNetErrStartup;
    return err;
}

void StackCloseMacTCP(void)
{
    _EndpointDisposeAll();
}

void StackTickleMacTCP(void)
{
    _EndpointTickleAll();
}

unsigned char StackGetAvailMacTCP(unsigned short mode)
{
    unsigned char available;
    long response;
    short refNum;
    DCtlHandle h;
    Str255 name;
    StringPtr driver;

    available = 0;
    if (Gestalt(kMacTCPVersion, &response) == 0) {
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
            if (EqualString(name, "\p.ipp", false, true)) {
                available = ((*h)->dCtlFlags & dOpenedMask) != 0;
                break;
            }
        }
    }
    return available;
}

ULong StackGetAddressListMacTCP(unsigned short kind, NetAddress *out, ULong max)
{
    GetAddrParamBlock pb;
    NetAddress *p;
    ULong i;
    ULong count;

    p = out;
    count = 0;
    for (i = 0; i < max; i++) {
        AddressComposeEmpty(p);
        p++;
    }
    pb.ioCompletion = 0;
    pb.ioResult = 0;
    pb.ioCRefNum = gMacTCPRefNum;
    pb.csCode = ipctlGetAddr;
    pb.ourAddress = 0;
    pb.ourNetMask = 0;
    if (PBControlSync((ParmBlkPtr)&pb) == 0) {
        if (count < max)
            AddressComposeTCPIP(out++, 0, pb.ourAddress);
        if (count + 1 < max)
            AddressComposeTCPIP(out++, 0, kLoopback);
        count = 2;
    }
    return count;
}

#endif
