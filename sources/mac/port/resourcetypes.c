/* resourcetypes.c -- opening a resource file by FSRef and the Core Endian flippers for Nova's resources, the mac 1.1.1 ResourceTypes.o (mac only; win: engine/rezopen.c and engine/rezfixup.c). Notes: Research/notes/resourcetypes.md */
#include "macport.h"
#include "gametypes.h"
#include "pilotfile.h"
#include "resources.h"

#define kCsumType 0x63739f6d     /* 'csüm' */
#define kYearType 0x79918aa8     /* 'yëä®' */
#define kTtabType 0x54748a62     /* 'Ttäb' */
#define kMprfType 0x4d70a8c4     /* 'Mp®ƒ' */
#define kRle8Type 0x726c9138     /* 'rlë8' */
#define kRleDType 0x726c9144     /* 'rlëD' */
#define kPilotResID 128          /* the pilot file's first record; 129 is the galaxy record */

#define FLIP16(field) ((field) = Endian16_Swap(field))
#define FLIP32(field) ((field) = Endian32_Swap(field))

#pragma options align=mac68k

typedef struct EbugRes {
    short words[32];
} EbugRes;

typedef struct Rle8Res {
    short width;
    short height;
    short depth;
    short palette;
    short frames;
    short reserved[3];
} Rle8Res;

typedef struct MprfRes {
    short version;
    short musicEnabled;
    short unk04;
    short volume;
    short keys[36];
    short unk50;
    DateTimeRec date;
    short unk60;
    short earlyOut;
    short unk64;
    short noDebris;
    short windowed;
    short loadShipSets;
    short loadShipGlow;
    short loadShipWeapons;
    short loadShipLights;
    short parallaxStars;
    short planetSounds;
    short showInfluence;
    short noHyperGamma;
    short noVersionCheck;
    short reserved[8];
} MprfRes;

#pragma options align=reset

/* EVOpenResourceFile (mac _EVOpenResourceFile, mac-only). STATE: see the mac gate. */
// MAC: _EVOpenResourceFile i386 0x000a5287 ppc 0x000b0fcc
short EVOpenResourceFile(const FSRef *ref, SInt8 permission)
{
    HFSUniStr255 forkName;
    short refNum;

    refNum = -1;
#if V111
    CFURLCreateFromFSRef(kCFAllocatorDefault, ref);
#endif
    FSGetDataForkName(&forkName);
    if (FSOpenResourceFile(ref, forkName.length, forkName.unicode, permission, &refNum) != noErr) {
        FSGetResourceForkName(&forkName);
        FSOpenResourceFile(ref, forkName.length, forkName.unicode, permission, &refNum);
    }
    return refNum;
}

/* EVEnumerateResources (mac _EVEnumerateResources, mac-only). STATE: see the mac gate. */
// MAC: _EVEnumerateResources i386 0x000a5342 ppc 0x000b1068
void EVEnumerateResources(short refNum)
{
    short savedResFile;
    short typeCount;
    int i;
    ResType type;

    savedResFile = CurResFile();
    UseResFile(refNum);
    typeCount = CountTypes();
    for (i = 1; i <= typeCount; i++) {
        GetIndType(&type, i);
        CountResources(type);
    }
    UseResFile(savedResFile);
}

#if TARGET_RT_LITTLE_ENDIAN
extern void BlitPixieRLESwap(unsigned char *tokens, RLEPixelData *header);
static OSStatus FlipYear(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipTtab(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipDesc(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipLeet(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipOops(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipBoom(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipDude(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipJunk(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipSpin(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipFlet(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipNebu(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipShan(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipEbug(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipRle8(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipMprf(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipCsum(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipGovt(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipMisn(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipSpob(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipCron(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipRoid(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipColr(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipShip(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipChar(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipIntf(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipOutf(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipPers(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipRank(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipSyst(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
static OSStatus FlipWeap(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon);
#endif

/* EVInstallCsumFlipper (mac _EVInstallCsumFlipper, mac-only). STATE: see the mac gate. */
// MAC: _EVInstallCsumFlipper i386 0x000a53a5 ppc 0x000b10d8
OSStatus EVInstallCsumFlipper(void)
{
#if TARGET_RT_LITTLE_ENDIAN
    return CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kCsumType, FlipCsum, NULL);
#else
    return noErr;
#endif
}

/* EVInstallEndianFlippers (mac _EVInstallEndianFlippers, mac-only). STATE: see the mac gate. */
// MAC: _EVInstallEndianFlippers i386 0x000a53d1 ppc 0x000b10e0
OSStatus EVInstallEndianFlippers(void)
{
#if TARGET_RT_LITTLE_ENDIAN
    OSStatus err;

    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kGovtType, FlipGovt, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kMisnType, FlipMisn, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kSpobType, FlipSpob, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kOopsType, FlipOops, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kBoomType, FlipBoom, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kCronType, FlipCron, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kRoidType, FlipRoid, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kColrType, FlipColr, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kDudeType, FlipDude, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kJunkType, FlipJunk, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kShipType, FlipShip, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kSpinType, FlipSpin, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kCharType, FlipChar, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kFletType, FlipFlet, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kIntfType, FlipIntf, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kNebuType, FlipNebu, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kOutfType, FlipOutf, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kPersType, FlipPers, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kRankType, FlipRank, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kSystType, FlipSyst, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kWeapType, FlipWeap, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kYearType, FlipYear, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kTtabType, FlipTtab, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kShanType, FlipShan, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kDescType, FlipDesc, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kLeetType, FlipLeet, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kMprfType, FlipMprf, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kEBugType, FlipEbug, NULL);
    if (err != noErr)
        return err;
    err = CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kRle8Type, FlipRle8, NULL);
    if (err != noErr)
        return err;
    return CoreEndianInstallFlipper(kCoreEndianResourceManagerDomain, kRleDType, FlipRle8, NULL);
#else
    return noErr;
#endif
}

/* OSType2String (mac _OSType2String, mac-only). STATE: see the mac gate. */
// MAC: _OSType2String i386 0x000a58ed ppc 0x000b10e8
void OSType2String(OSType type, char *string)
{
    string[0] = type >> 24;
    string[1] = type >> 16;
    string[2] = type >> 8;
    string[3] = type;
    string[4] = 0;
}

#if TARGET_RT_LITTLE_ENDIAN
/* FlipYear (mac _FlipYear, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipYear i386 0x000a5916 ppc -
static OSStatus FlipYear(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    return noErr;
}

/* FlipTtab (mac _FlipTtab, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipTtab i386 0x000a591d ppc -
static OSStatus FlipTtab(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    if (dataSize < 0x50000)
        return errCoreEndianDataTooShortForFormat;
    return noErr;
}

/* FlipDesc (mac _FlipDesc, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipDesc i386 0x000a5930 ppc -
static OSStatus FlipDesc(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    return noErr;
}

/* FlipLeet (mac _FlipLeet, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipLeet i386 0x000a5937 ppc -
static OSStatus FlipLeet(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    return noErr;
}

/* FlipOops (mac _FlipOops, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipOops i386 0x000a593e ppc -
static OSStatus FlipOops(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    OopsRes *oops = dataPtr;
    int i;

    if (dataSize < sizeof(OopsRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(oops->stellar);
    FLIP16(oops->commodity);
    FLIP16(oops->priceDelta);
    FLIP16(oops->duration);
    FLIP16(oops->freq);
    for (i = 0; i < 8; i++)
        FLIP16(oops->reserved[i]);
    return noErr;
}

/* FlipBoom (mac _FlipBoom, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipBoom i386 0x000a5983 ppc -
static OSStatus FlipBoom(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    BoomRes *boom = dataPtr;

    if (dataSize < sizeof(BoomRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(boom->frameAdvance);
    FLIP16(boom->soundIndex);
    FLIP16(boom->graphicIndex);
    return noErr;
}

/* FlipDude (mac _FlipDude, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipDude i386 0x000a59a6 ppc -
static OSStatus FlipDude(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    DudeRes *dude = dataPtr;
    int i;

    if (dataSize < sizeof(DudeRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(dude->aiType);
    FLIP16(dude->govt);
    FLIP16(dude->booty);
    FLIP16(dude->infoTypes);
    for (i = 0; i < 16; i++)
        FLIP16(dude->shipTypes[i]);
    for (i = 0; i < 16; i++)
        FLIP16(dude->probs[i]);
    for (i = 0; i < 8; i++)
        FLIP16(dude->reserved[i]);
    return noErr;
}

/* FlipJunk (mac _FlipJunk, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipJunk i386 0x000a59ff ppc -
static OSStatus FlipJunk(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    JunkRes *junk = dataPtr;
    int i;

    if (dataSize < sizeof(JunkRes))
        return errCoreEndianDataTooShortForFormat;
    for (i = 0; i < 8; i++)
        FLIP16(junk->soldAt[i]);
    for (i = 0; i < 8; i++)
        FLIP16(junk->boughtAt[i]);
    FLIP16(junk->basePrice);
    FLIP16(junk->flags);
    FLIP16(junk->scanMask);
    return noErr;
}

/* FlipSpin (mac _FlipSpin, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipSpin i386 0x000a5a44 ppc -
static OSStatus FlipSpin(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    SpinRec *spin = dataPtr;

    if (dataSize < sizeof(SpinRec))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(spin->spriteID);
    FLIP16(spin->maskID);
    FLIP16(spin->xSize);
    FLIP16(spin->ySize);
    FLIP16(spin->xTiles);
    FLIP16(spin->yTiles);
    return noErr;
}

/* FlipFlet (mac _FlipFlet, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipFlet i386 0x000a5a76 ppc -
static OSStatus FlipFlet(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    FletRes *flet = dataPtr;
    int i;

    if (dataSize < sizeof(FletRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(flet->leadShip);
    for (i = 0; i < 4; i++)
        FLIP16(flet->escortShip[i]);
    for (i = 0; i < 4; i++)
        FLIP16(flet->minCount[i]);
    for (i = 0; i < 4; i++)
        FLIP16(flet->maxCount[i]);
    FLIP16(flet->govt);
    FLIP16(flet->linkSyst);
    FLIP16(flet->quote);
    FLIP16(flet->flags);
    for (i = 0; i < 8; i++)
        FLIP16(flet->reserved[i]);
    return noErr;
}

/* FlipNebu (mac _FlipNebu, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipNebu i386 0x000a5af6 ppc -
static OSStatus FlipNebu(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    NebuRes *nebu = dataPtr;

    if (dataSize < sizeof(NebuRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(nebu->xPos);
    FLIP16(nebu->yPos);
    FLIP16(nebu->xSize);
    FLIP16(nebu->ySize);
    return noErr;
}

/* RectSwap (mac _RectSwap, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _RectSwap i386 0x000a5b21 ppc -
static void RectSwap(Rect *r)
{
    FLIP16(r->top);
    FLIP16(r->left);
    FLIP16(r->bottom);
    FLIP16(r->right);
}

/* FlipShan (mac _FlipShan, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipShan i386 0x000a5b39 ppc -
static OSStatus FlipShan(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    ShanRec *shan = dataPtr;
    int i;

    if (dataSize < sizeof(ShanRec))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(shan->baseImageID);
    FLIP16(shan->baseMaskID);
    FLIP16(shan->baseSetCount);
    FLIP16(shan->baseXSize);
    FLIP16(shan->baseYSize);
    FLIP16(shan->baseTransparency);
    FLIP16(shan->altImageID);
    FLIP16(shan->altMaskID);
    FLIP16(shan->altSetCount);
    FLIP16(shan->altXSize);
    FLIP16(shan->altYSize);
    FLIP16(shan->glowImageID);
    FLIP16(shan->glowMaskID);
    FLIP16(shan->glowXSize);
    FLIP16(shan->glowYSize);
    FLIP16(shan->lightImageID);
    FLIP16(shan->lightMaskID);
    FLIP16(shan->lightXSize);
    FLIP16(shan->lightYSize);
    FLIP16(shan->weapImageID);
    FLIP16(shan->weapMaskID);
    FLIP16(shan->weapXSize);
    FLIP16(shan->weapYSize);
    FLIP16(shan->flags);
    FLIP16(shan->animDelay);
    FLIP16(shan->weapDecay);
    FLIP16(shan->framesPer);
    FLIP16(shan->blinkMode);
    for (i = 0; i < 4; i++)
        FLIP16(shan->blinkValue[i]);
    FLIP16(shan->shieldImageID);
    FLIP16(shan->shieldMaskID);
    FLIP16(shan->shieldXSize);
    FLIP16(shan->shieldYSize);
    RectSwap((Rect *)shan->mountPos[0].x);
    RectSwap((Rect *)shan->mountPos[0].y);
    RectSwap((Rect *)shan->mountPos[1].x);
    RectSwap((Rect *)shan->mountPos[1].y);
    RectSwap((Rect *)shan->mountPos[2].x);
    RectSwap((Rect *)shan->mountPos[2].y);
    RectSwap((Rect *)shan->mountPos[3].x);
    RectSwap((Rect *)shan->mountPos[3].y);
    FLIP16(shan->upCompressX);
    FLIP16(shan->upCompressY);
    FLIP16(shan->downCompressX);
    FLIP16(shan->downCompressY);
    RectSwap((Rect *)shan->mountPosZ[0]);
    RectSwap((Rect *)shan->mountPosZ[1]);
    RectSwap((Rect *)shan->mountPosZ[2]);
    RectSwap((Rect *)shan->mountPosZ[3]);
    for (i = 0; i < 8; i++)
        FLIP16(shan->unkB0[i]);
    return noErr;
}

/* FlipEbug (mac _FlipEbug, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipEbug i386 0x000a5cb2 ppc -
static OSStatus FlipEbug(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    EbugRes *ebug = dataPtr;
    int i;

    if (dataSize < sizeof(EbugRes))
        return errCoreEndianDataTooShortForFormat;
    for (i = 0; i < 32; i++)
        FLIP16(ebug->words[i]);
    return noErr;
}

/* FlipRle8 (mac _FlipRle8, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipRle8 i386 0x000a5cd6 ppc -
static OSStatus FlipRle8(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    Rle8Res *rle = dataPtr;

    if (dataSize < sizeof(Rle8Res))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(rle->width);
    FLIP16(rle->height);
    FLIP16(rle->depth);
    FLIP16(rle->palette);
    FLIP16(rle->frames);
    FLIP16(rle->reserved[0]);
    FLIP16(rle->reserved[1]);
    FLIP16(rle->reserved[2]);
    BlitPixieRLESwap(dataPtr + sizeof(Rle8Res), dataPtr);
    return noErr;
}

#endif

/* DateTimeRecSwap (mac _DateTimeRecSwap, mac-only; inlined on ppc). STATE: see the mac gate. */
// MAC: _DateTimeRecSwap i386 0x000a5d26 ppc -
static void DateTimeRecSwap(DateTimeRec *date)
{
    FLIP16(date->year);
    FLIP16(date->month);
    FLIP16(date->day);
    FLIP16(date->hour);
    FLIP16(date->minute);
    FLIP16(date->second);
    FLIP16(date->dayOfWeek);
}

#if TARGET_RT_LITTLE_ENDIAN
/* FlipMprf (mac _FlipMprf, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipMprf i386 0x000a5d4d ppc -
static OSStatus FlipMprf(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    MprfRes *mprf = dataPtr;
    int i;

    if (dataSize < sizeof(MprfRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(mprf->version);
    FLIP16(mprf->musicEnabled);
    FLIP16(mprf->unk04);
    FLIP16(mprf->volume);
    for (i = 0; i < 36; i++)
        FLIP16(mprf->keys[i]);
    FLIP16(mprf->unk50);
    DateTimeRecSwap(&mprf->date);
    FLIP16(mprf->unk60);
    FLIP16(mprf->earlyOut);
    FLIP16(mprf->unk64);
    FLIP16(mprf->noDebris);
    FLIP16(mprf->windowed);
    FLIP16(mprf->loadShipSets);
    FLIP16(mprf->loadShipGlow);
    FLIP16(mprf->loadShipWeapons);
    FLIP16(mprf->loadShipLights);
    FLIP16(mprf->parallaxStars);
    FLIP16(mprf->planetSounds);
    FLIP16(mprf->showInfluence);
    FLIP16(mprf->noHyperGamma);
    FLIP16(mprf->noVersionCheck);
    for (i = 0; i < 8; i++)
        FLIP16(mprf->reserved[i]);
    return noErr;
}

/* FlipCsum (mac _FlipCsum, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipCsum i386 0x000a5dfb ppc -
static OSStatus FlipCsum(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    long csum = Endian32_Swap(*(long *)dataPtr);

    return noErr;
}

/* FlipGovt (mac _FlipGovt, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipGovt i386 0x000a5e02 ppc -
static OSStatus FlipGovt(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    GovtRes *govt = dataPtr;
    int i;

    if (dataSize < sizeof(GovtRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(govt->voiceType);
    FLIP16(govt->flags);
    FLIP16(govt->flags2);
    FLIP16(govt->scanFine);
    FLIP16(govt->crimeTol);
    for (i = 0; i < 5; i++)
        FLIP16(govt->penalty[i]);
    FLIP16(govt->initialRec);
    FLIP16(govt->maxOdds);
    for (i = 0; i < 4; i++)
        FLIP16(govt->classes[i]);
    for (i = 0; i < 4; i++)
        FLIP16(govt->allies[i]);
    for (i = 0; i < 4; i++)
        FLIP16(govt->enemies[i]);
    FLIP16(govt->skillMult);
    FLIP16(govt->scanMask);
    for (i = 0; i < 2; i++)
        FLIP32(govt->require[i]);
    for (i = 0; i < 4; i++)
        FLIP16(govt->inhJam[i]);
    FLIP32(govt->color);
    FLIP32(govt->shipColor);
    FLIP16(govt->interfaceID);
    FLIP16(govt->newsPic);
    for (i = 0; i < 8; i++)
        FLIP16(govt->reserved[i]);
    return noErr;
}

/* FlipMisn (mac _FlipMisn, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipMisn i386 0x000a5eff ppc -
static OSStatus FlipMisn(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    MisnRes *misn = dataPtr;
    int i;

    if (dataSize < sizeof(MisnRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(misn->availStel);
    FLIP16(misn->unused02);
    FLIP16(misn->availLoc);
    FLIP16(misn->availRecord);
    FLIP16(misn->availRating);
    FLIP16(misn->availRandom);
    FLIP16(misn->travelStel);
    FLIP16(misn->returnStel);
    FLIP16(misn->cargoType);
    FLIP16(misn->cargoQty);
    FLIP16(misn->pickupMode);
    FLIP16(misn->dropOffMode);
    FLIP16(misn->scanMask);
    FLIP16(misn->unused1A);
    FLIP32(misn->payVal);
    FLIP16(misn->shipCount);
    FLIP16(misn->shipSyst);
    FLIP16(misn->shipDude);
    FLIP16(misn->shipGoal);
    FLIP16(misn->shipBehav);
    FLIP16(misn->shipNameID);
    FLIP16(misn->shipStart);
    FLIP16(misn->compGovt);
    FLIP16(misn->compReward);
    FLIP16(misn->shipSubtitle);
    FLIP16(misn->briefText);
    FLIP16(misn->quickBrief);
    FLIP16(misn->loadCargText);
    FLIP16(misn->dumpCargoText);
    FLIP16(misn->compText);
    FLIP16(misn->failText);
    FLIP16(misn->timeLimit);
    FLIP16(misn->canAbort);
    FLIP16(misn->shipDoneText);
    FLIP16(misn->unused46);
    FLIP16(misn->auxShipCount);
    FLIP16(misn->auxShipDude);
    FLIP16(misn->auxShipSyst);
    FLIP16(misn->unused4E);
    FLIP16(misn->flags);
    FLIP16(misn->flags2);
    FLIP16(misn->unused54);
    FLIP16(misn->unused56);
    FLIP16(misn->refuseText);
    FLIP16(misn->availShipType);
    FLIP32(misn->require1);
    FLIP32(misn->require2);
    FLIP16(misn->datePostInc);
    FLIP16(misn->dispWeight);
    for (i = 0; i < 8; i++)
        FLIP16(misn->reserved[i]);
    return noErr;
}

/* FlipSpob (mac _FlipSpob, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipSpob i386 0x000a603f ppc -
static OSStatus FlipSpob(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    SpobRes *spob = dataPtr;
    int i;

    if (dataSize < sizeof(SpobRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(spob->xPos);
    FLIP16(spob->yPos);
    FLIP16(spob->type);
    FLIP32(spob->flags);
    FLIP16(spob->tribute);
    FLIP16(spob->techLevel);
    for (i = 0; i < 3; i++)
        FLIP16(spob->specialTech[i]);
    FLIP16(spob->govt);
    FLIP16(spob->minStatus);
    FLIP16(spob->custPicID);
    FLIP16(spob->custSndID);
    FLIP16(spob->defenseDude);
    FLIP16(spob->defCount);
    FLIP16(spob->flags2);
    FLIP16(spob->animDelay);
    FLIP16(spob->frame0Bias);
    for (i = 0; i < 8; i++)
        FLIP16(spob->hyperLink[i]);
    FLIP32(spob->fee);
    FLIP16(spob->gravity);
    FLIP16(spob->weapon);
    FLIP32(spob->strength);
    FLIP16(spob->deadType);
    FLIP16(spob->deadTime);
    FLIP16(spob->explodType);
    for (i = 0; i < 5; i++)
        FLIP16(spob->specialTech2[i]);
    for (i = 0; i < 8; i++)
        FLIP16(spob->reserved[i]);
    return noErr;
}

/* FlipCron (mac _FlipCron, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipCron i386 0x000a613b ppc -
static OSStatus FlipCron(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    CronRes *cron = dataPtr;
    int i;

    if (dataSize < sizeof(CronRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(cron->firstDay);
    FLIP16(cron->firstMonth);
    FLIP16(cron->firstYear);
    FLIP16(cron->lastDay);
    FLIP16(cron->lastMonth);
    FLIP16(cron->lastYear);
    FLIP16(cron->random);
    FLIP16(cron->duration);
    FLIP16(cron->preHoldoff);
    FLIP16(cron->postHoldoff);
    FLIP16(cron->flags);
    FLIP16(cron->indNewsStr);
    for (i = 0; i < 2; i++)
        FLIP32(cron->contribute[i]);
    for (i = 0; i < 2; i++)
        FLIP32(cron->require[i]);
    for (i = 0; i < 4; i++)
        FLIP16(cron->newsGovt[i]);
    for (i = 0; i < 4; i++)
        FLIP16(cron->govtNewsStr[i]);
    return noErr;
}

/* FlipRoid (mac _FlipRoid, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipRoid i386 0x000a61f8 ppc -
static OSStatus FlipRoid(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    RoidRes *roid = dataPtr;
    int i;

    if (dataSize < sizeof(RoidRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(roid->strength);
    FLIP16(roid->spinRate);
    FLIP16(roid->yieldType);
    FLIP16(roid->yieldQty);
    FLIP16(roid->partCount);
    FLIP32(roid->partColor);
    for (i = 0; i < 2; i++)
        FLIP16(roid->fragType[i]);
    FLIP16(roid->fragCount);
    FLIP16(roid->explodType);
    FLIP16(roid->mass);
    for (i = 0; i < 8; i++)
        FLIP16(roid->reserved[i]);
    return noErr;
}

/* FlipColr (mac _FlipColr, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipColr i386 0x000a6260 ppc -
static OSStatus FlipColr(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    ColrRes *colr = dataPtr;
    int i;

    if (dataSize < sizeof(ColrRes))
        return errCoreEndianDataTooShortForFormat;
    for (i = 0; i < 3; i++)
        FLIP32(colr->buttonColor[i]);
    FLIP16(colr->menuFontSize);
    for (i = 0; i < 2; i++)
        FLIP32(colr->menuColor[i]);
    for (i = 0; i < 2; i++)
        FLIP32(colr->gridColor[i]);
    RectSwap(&colr->progressBar);
    for (i = 0; i < 3; i++)
        FLIP32(colr->progressColor[i]);
    for (i = 0; i < 12; i++)
        FLIP16(colr->buttonPos[i]);
    FLIP32(colr->floatingMapColor);
    for (i = 0; i < 3; i++)
        FLIP32(colr->listColor[i]);
    FLIP32(colr->escortHiliteColor);
    FLIP16(colr->buttonFontSize);
    for (i = 0; i < 2; i++)
        FLIP16(colr->logoPos[i]);
    for (i = 0; i < 2; i++)
        FLIP16(colr->rolloverPos[i]);
    for (i = 0; i < 6; i++)
        FLIP16(colr->slidePos[i]);
    return noErr;
}

/* FlipShip (mac _FlipShip, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipShip i386 0x000a637b ppc -
static OSStatus FlipShip(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    ShipRes *ship = dataPtr;
    int i;

    if (dataSize < sizeof(ShipRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(ship->cargo);
    FLIP16(ship->shield);
    FLIP16(ship->accel);
    FLIP16(ship->speed);
    FLIP16(ship->turn);
    FLIP16(ship->energy);
    FLIP16(ship->freeSpace);
    FLIP16(ship->armor);
    FLIP16(ship->shieldRe);
    for (i = 0; i < 4; i++)
        FLIP16(ship->wType[i]);
    for (i = 0; i < 4; i++)
        FLIP16(ship->wCount[i]);
    for (i = 0; i < 4; i++)
        FLIP16(ship->ammo[i]);
    FLIP16(ship->maxGun);
    FLIP16(ship->maxTur);
    FLIP16(ship->techLevel);
    FLIP32(ship->cost);
    FLIP16(ship->deathDelay);
    FLIP16(ship->armorRe);
    for (i = 0; i < 2; i++)
        FLIP16(ship->explode[i]);
    FLIP16(ship->dispWeight);
    FLIP16(ship->mass);
    FLIP16(ship->length);
    FLIP16(ship->inherentAI);
    FLIP16(ship->crew);
    FLIP16(ship->strength);
    FLIP16(ship->inherentGovt);
    FLIP16(ship->flags);
    FLIP16(ship->podCount);
    RectSwap((Rect *)ship->defaultItems);
    RectSwap((Rect *)ship->itemCount);
    FLIP16(ship->fuelRegen);
    FLIP16(ship->skillVar);
    FLIP16(ship->flags2);
    FLIP32(ship->contribute1);
    FLIP32(ship->contribute2);
    FLIP16(ship->deionize);
    FLIP16(ship->ionizeMax);
    FLIP16(ship->keyCarried);
    RectSwap((Rect *)ship->defaultItems2);
    RectSwap((Rect *)ship->itemCount2);
    FLIP32(ship->require1);
    FLIP32(ship->require2);
    FLIP16(ship->buyRandom);
    FLIP16(ship->hireRandom);
    for (i = 0; i < 34; i++)
        FLIP16(ship->unused38C[i]);
    for (i = 0; i < 4; i++)
        FLIP16(ship->wType2[i]);
    for (i = 0; i < 4; i++)
        FLIP16(ship->wCount2[i]);
    for (i = 0; i < 4; i++)
        FLIP16(ship->ammo2[i]);
    FLIP16(ship->flags3);
    FLIP16(ship->upgradeTo);
    FLIP32(ship->escUpgrdCost);
    FLIP32(ship->escSellValue);
    FLIP16(ship->escortType);
    for (i = 0; i < 8; i++)
        FLIP16(ship->reserved[i]);
    return noErr;
}

/* FlipChar (mac _FlipChar, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipChar i386 0x000a6593 ppc -
static OSStatus FlipChar(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    CharRecord *chr = dataPtr;
    int i;

    if (dataSize < sizeof(CharRecord))
        return errCoreEndianDataTooShortForFormat;
    FLIP32(chr->cash);
    FLIP16(chr->shipType);
    for (i = 0; i < 4; i++)
        FLIP16(chr->startSyst[i]);
    for (i = 0; i < 4; i++)
        FLIP16(chr->govt[i]);
    for (i = 0; i < 4; i++)
        FLIP16(chr->standing[i]);
    FLIP16(chr->kills);
    for (i = 0; i < 4; i++)
        FLIP16(chr->introPICT[i]);
    for (i = 0; i < 4; i++)
        FLIP16(chr->introDelay[i]);
    FLIP16(chr->introTextID);
    FLIP16(chr->flags);
    FLIP16(chr->startDay);
    FLIP16(chr->startMonth);
    FLIP16(chr->startYear);
    for (i = 0; i < 8; i++)
        FLIP16(chr->unk15a[i]);
    return noErr;
}

/* FlipIntf (mac _FlipIntf, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipIntf i386 0x000a6656 ppc -
static OSStatus FlipIntf(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    IntfRec *intf = dataPtr;
    int i;

    if (dataSize < sizeof(IntfRec))
        return errCoreEndianDataTooShortForFormat;
    FLIP32(intf->brightText.rgb);
    FLIP32(intf->dimText.rgb);
    RectSwap(&intf->radarArea);
    FLIP32(intf->brightRadar.rgb);
    FLIP32(intf->dimRadar.rgb);
    RectSwap(&intf->shieldArea);
    FLIP32(intf->shieldColor.rgb);
    RectSwap(&intf->armorArea);
    FLIP32(intf->armorColor.rgb);
    RectSwap(&intf->fuelArea);
    FLIP32(intf->fuelFull.rgb);
    FLIP32(intf->fuelPartial.rgb);
    RectSwap(&intf->navArea);
    RectSwap(&intf->weapArea);
    RectSwap(&intf->targArea);
    RectSwap(&intf->cargoArea);
    for (i = 0; i < 2; i++)
        FLIP16(intf->statFontSize[i]);
    FLIP16(intf->statusBkgnd);
    return noErr;
}

/* FlipOutf (mac _FlipOutf, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipOutf i386 0x000a6715 ppc -
static OSStatus FlipOutf(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    OutfRes *outf = dataPtr;
    int i;

    if (dataSize < sizeof(OutfRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(outf->dispWeight);
    FLIP16(outf->mass);
    FLIP16(outf->techLevel);
    FLIP16(outf->modType);
    FLIP16(outf->modVal);
    FLIP16(outf->max);
    FLIP16(outf->flags);
    FLIP32(outf->cost);
    FLIP16(outf->modType2);
    FLIP16(outf->modVal2);
    FLIP16(outf->modType3);
    FLIP16(outf->modVal3);
    FLIP16(outf->modType4);
    FLIP16(outf->modVal4);
    FLIP32(outf->contribute1);
    FLIP32(outf->contribute2);
    FLIP32(outf->require1);
    FLIP32(outf->require2);
    FLIP16(outf->itemClass);
    FLIP16(outf->scanMask);
    FLIP16(outf->buyRandom);
    FLIP16(outf->require3);
    for (i = 0; i < 8; i++)
        FLIP16(outf->reserved[i]);
    return noErr;
}

/* FlipPers (mac _FlipPers, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipPers i386 0x000a67ce ppc -
static OSStatus FlipPers(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    PersRes *pers = dataPtr;
    int i;

    if (dataSize < sizeof(PersRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(pers->linkSyst);
    FLIP16(pers->govt);
    FLIP16(pers->aiType);
    FLIP16(pers->aggress);
    FLIP16(pers->coward);
    FLIP16(pers->shipType);
    RectSwap(&pers->weapType);
    RectSwap(&pers->weapCount);
    RectSwap(&pers->ammoLoad);
    FLIP32(pers->credits);
    FLIP16(pers->shieldMod);
    FLIP16(pers->hailPict);
    FLIP16(pers->commQuote);
    FLIP16(pers->hailQuote);
    FLIP16(pers->linkMission);
    FLIP16(pers->flags);
    FLIP16(pers->grantClass);
    FLIP16(pers->grantCount);
    FLIP16(pers->grantProb);
    FLIP32(pers->shipColor);
    FLIP16(pers->flags2);
    for (i = 0; i < 8; i++)
        FLIP16(pers->reserved[i]);
    return noErr;
}

/* FlipRank (mac _FlipRank, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipRank i386 0x000a6890 ppc -
static OSStatus FlipRank(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    RankRes *rank = dataPtr;
    int i;

    if (dataSize < sizeof(RankRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(rank->weight);
    FLIP16(rank->attachTo);
    FLIP16(rank->flags);
    FLIP32(rank->salary);
    FLIP32(rank->salaryCap);
    for (i = 0; i < 2; i++)
        FLIP32(rank->contribute[i]);
    FLIP16(rank->priceMod);
    return noErr;
}

/* FlipSyst (mac _FlipSyst, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipSyst i386 0x000a68e2 ppc -
static OSStatus FlipSyst(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    SystRes *syst = dataPtr;
    int i;

    if (dataSize < sizeof(SystRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(syst->xPos);
    FLIP16(syst->yPos);
    for (i = 0; i < 16; i++)
        FLIP16(syst->con[i]);
    for (i = 0; i < 16; i++)
        FLIP16(syst->nav[i]);
    for (i = 0; i < 8; i++)
        FLIP16(syst->dudeTypes[i]);
    for (i = 0; i < 8; i++)
        FLIP16(syst->probs[i]);
    FLIP16(syst->avgShips);
    FLIP16(syst->govt);
    FLIP16(syst->message);
    FLIP16(syst->asteroids);
    FLIP16(syst->interference);
    for (i = 0; i < 8; i++)
        FLIP16(syst->person[i]);
    for (i = 0; i < 8; i++)
        FLIP16(syst->personProb[i]);
    FLIP32(syst->bkgndColor);
    FLIP16(syst->murk);
    FLIP16(syst->astTypes);
    FLIP16(syst->reinfFleet);
    FLIP16(syst->reinfTime);
    FLIP16(syst->reinfInterval);
    for (i = 0; i < 8; i++)
        FLIP16(syst->reserved[i]);
    return noErr;
}

/* FlipWeap (mac _FlipWeap, mac-only, i386 only). STATE: see the mac gate. */
// MAC: _FlipWeap i386 0x000a69da ppc -
static OSStatus FlipWeap(OSType dataDomain, OSType dataType, SInt16 id, void *dataPtr, ByteCount dataSize, Boolean currentlyNative, void *refcon)
{
    WeapRes *weap = dataPtr;
    int i;

    if (dataSize < sizeof(WeapRes))
        return errCoreEndianDataTooShortForFormat;
    FLIP16(weap->reload);
    FLIP16(weap->count);
    for (i = 0; i < 2; i++)
        FLIP16(weap->massDmg[i]);
    FLIP16(weap->guidance);
    FLIP16(weap->speed);
    FLIP16(weap->ammoType);
    FLIP16(weap->graphic);
    FLIP16(weap->inaccuracy);
    FLIP16(weap->sound);
    FLIP16(weap->impact);
    FLIP16(weap->explodType);
    FLIP16(weap->proxRadius);
    FLIP16(weap->blastRadius);
    FLIP16(weap->flags);
    FLIP16(weap->seeker);
    FLIP16(weap->smokeSet);
    FLIP16(weap->decay);
    FLIP16(weap->particles);
    FLIP16(weap->partVel);
    FLIP16(weap->partLifeMin);
    FLIP16(weap->partLifeMax);
    FLIP32(weap->partColor);
    FLIP16(weap->beamLength);
    FLIP16(weap->beamWidth);
    FLIP16(weap->falloff);
    FLIP32(weap->beamColor);
    FLIP32(weap->coronaColor);
    FLIP16(weap->subCount);
    FLIP16(weap->subType);
    FLIP16(weap->subTheta);
    FLIP16(weap->subLimit);
    FLIP16(weap->proxSafety);
    FLIP16(weap->flags2);
    FLIP16(weap->ionization);
    FLIP16(weap->hitParticles);
    FLIP16(weap->hitPartLife);
    FLIP16(weap->hitPartVel);
    FLIP32(weap->hitPartColor);
    FLIP16(weap->recoil);
    FLIP16(weap->exitType);
    FLIP16(weap->burstCount);
    FLIP16(weap->burstReload);
    for (i = 0; i < 4; i++)
        FLIP16(weap->jamVuln[i]);
    FLIP16(weap->flags3);
    FLIP16(weap->durability);
    FLIP16(weap->guidedTurn);
    FLIP16(weap->maxAmmo);
    FLIP16(weap->liCohesive);
    FLIP16(weap->liAmplitude);
    FLIP32(weap->liColor);
    for (i = 0; i < 8; i++)
        FLIP16(weap->reserved[i]);
    return noErr;
}
#endif

/* RGBColorSwap: inlined into FlipNpil on both slices (no N_FUN). */
static void RGBColorSwap(RGBColor *color)
{
    FLIP16(color->red);
    FLIP16(color->blue);
    FLIP16(color->green);
}

/* FlipNpil (mac _FlipNpil, mac-only). STATE: MATCH on both slices (pilotfile.h carries unk59 as
   long[2] and flags2 unsigned on MACBUILD, priceFlux[2][2] everywhere). OPEN: none */
// MAC: _FlipNpil i386 0x000a6b27 ppc 0x000b1110
OSStatus FlipNpil(short resID, void *data, long size, Boolean toDisk)
{
    PilotRec *player = data;
    PilotRec2 *galaxy = data;
    int i;
    int j;

    if (resID == kPilotResID) {
        FLIP16(player->stellar);
        FLIP16(player->shipType);
        for (i = 0; i < 6; i++)
            FLIP16(player->cargo[i]);
        FLIP16(player->shield);
        FLIP16(player->fuel);
        FLIP16(player->month);
        FLIP16(player->day);
        FLIP16(player->year);
        for (i = 0; i < 0x800; i++)
            FLIP16(player->explore[i]);
        for (i = 0; i < 0x200; i++)
            FLIP16(player->outfits[i]);
        for (i = 0; i < 0x800; i++)
            FLIP16(player->legalStatus[i]);
        for (i = 0; i < 0x100; i++)
            FLIP16(player->weapCount[i]);
        for (i = 0; i < 0x100; i++)
            FLIP16(player->weapAmmo[i]);
        FLIP32(player->credits);
        for (i = 0; i < 16; i++) {
            MissionState *obj = &player->objectives[i];

            FLIP16(obj->flags);
            DateTimeRecSwap(&obj->dueDate);
        }
        for (i = 0; i < 16; i++) {
            MissionRecord *m = &player->missions[i];

            FLIP16(m->travelStellar);
            FLIP16(m->unk2);
            FLIP16(m->returnStellar);
            FLIP16(m->shipCount);
            FLIP16(m->shipDude);
            FLIP16(m->shipGoal);
            FLIP16(m->shipBehav);
            FLIP16(m->shipStart);
            FLIP16(m->shipSyst);
            FLIP16(m->cargoType);
            FLIP16(m->cargoQty);
            FLIP16(m->pickupMode);
            FLIP16(m->dropoffMode);
            FLIP16(m->scanMask);
            FLIP16(m->compGovt);
            FLIP16(m->compReward);
            FLIP16(m->datePostIncrement);
            FLIP32(m->payment);
            FLIP16(m->specialShipsKilled);
            FLIP16(m->specialShipsBoarded);
            FLIP16(m->specialShipsDisabled);
            FLIP16(m->specialShipsJumpedIn);
            FLIP16(m->specialShipsJumpedOut);
            FLIP16(m->initialShipCount);
            FLIP16(m->briefDesc);
            FLIP16(m->quickBriefDesc);
            FLIP16(m->loadCargoDesc);
            FLIP16(m->dropCargoDesc);
            FLIP16(m->compDesc);
            FLIP16(m->failDesc);
            FLIP16(m->refuseDesc);
            FLIP16(m->shipDoneDesc);
            FLIP16(m->timeLimit);
            FLIP16(m->destStrList);
            FLIP16(m->destStrIndex);
            FLIP16(m->shipDelay);
            FLIP16(m->misnIndex);
            FLIP16(m->shipStrList);
            FLIP16(m->shipStrIndex);
            FLIP16(m->shipTypeIndex);
            FLIP16(m->flags);
            FLIP16(m->flags2);
            for (j = 0; j < 2; j++)
                FLIP32(m->unk59[j]);
            FLIP16(m->auxShipCount);
            FLIP16(m->auxShipDude);
            FLIP16(m->auxShipSyst);
            FLIP16(m->auxShipsSpawned);
            FLIP16(m->auxShipDelay);
            FLIP16(m->auxShipsLeft);
        }
        for (i = 0; i < 0x40; i++)
            FLIP16(player->escortType[i]);
        for (i = 0; i < 0x40; i++)
            FLIP16(player->fighterType[i]);
        for (i = 0; i < 0x40; i++)
            FLIP16(player->escortA[i]);
        for (i = 0; i < 0x40; i++)
            FLIP16(player->escortB[i]);
        for (i = 0; i < 0x40; i++)
            FLIP16(player->fighterVoice[i]);
        FLIP32(player->kills);
    } else if (resID == kPilotResID + 1) {
        FLIP16(galaxy->version);
        FLIP16(galaxy->strict);
        FLIP16(galaxy->gender);
        for (i = 0; i < 0x800; i++)
            FLIP16(galaxy->defWave[i]);
        for (i = 0; i < 0x400; i++)
            FLIP16(galaxy->persActive[i]);
        for (i = 0; i < 0x400; i++)
            FLIP16(galaxy->persB[i]);
        for (i = 0; i < 0x40; i++)
            FLIP16(galaxy->unk2006[i]);
        for (i = 0; i < 0x800; i++)
            FLIP16(galaxy->tribute[i]);
        for (i = 0; i < 0x100; i++)
            FLIP16(galaxy->newsC[i]);
        for (i = 0; i < 0x100; i++)
            FLIP16(galaxy->newsB[i]);
        for (i = 0; i < 0x80; i++)
            FLIP16(galaxy->junkHeld[i]);
        for (i = 0; i < 2; i++)
            for (j = 0; j < 2; j++)
                FLIP16(galaxy->priceFlux[i][j]);
        for (i = 0; i < 0x200; i++)
            FLIP16(galaxy->cronA[i]);
        for (i = 0; i < 0x200; i++)
            FLIP16(galaxy->cronB[i]);
        for (i = 0; i < 0x800; i++)
            FLIP16(galaxy->systState[i]);
        for (i = 0; i < 0x800; i++)
            FLIP16(galaxy->spobDead[i]);
        for (i = 0; i < 4; i++)
            FLIP16(galaxy->escortOrders[i]);
        RGBColorSwap(&galaxy->paint);
        for (i = 0; i < 0x80; i++)
            FLIP16(galaxy->rankOwned[i]);
        for (i = 0; i < 0x400; i++)
            FLIP16(galaxy->unk5efe[i]);
    }
    return noErr;
}
