// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* swcreategworldfromciconmask.c -- SWCreateGWorldFromCIconMask (win 0x00479070, donor SpriteWorldUtils.c line 444). Notes: Research/notes/swcreategworldfromciconmask.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#include "burgerplat.h"
#include "cefix.h"
#ifdef MACBUILD
#include "render.h"
#endif

#define kWhite555   0x7fff

/* SWCreateGWorldFromCIconMask (mac _SWCreateGWorldFromCIconMask). STATE: MATCH, relocregcheck
   12 rows 0 differ. OPEN: none */
// FUNCTION: EVNOVA 0x00479070
// MAC: _SWCreateGWorldFromCIconMask i386 0x000a7fbd ppc 0x000b2e0c
#ifdef MACBUILD
short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld, CGrafPtr *maskGWorldP,
                                  struct Image_t *cIcon)
{
    char saveState;
    CIconHandle cIconH = (CIconHandle)cIcon;
    OSErr err;
    GWorldPtr saveGWorld;
    GDHandle saveGDevice;
    BitMap maskBitMap;
    GWorldPtr tempGWorldP;
    short depth;
    GDHandle theGDH;
    Rect iconRect;
    PixMapHandle pixMapH;
    unsigned char *baseAddr;
    short rowBytes, height;
    int x, y;

    GetGWorld(&saveGWorld, &saveGDevice);

    depth = destSpriteWorld->pixelDepth;
    if (useLegacyRenderingPipeline)
        theGDH = destSpriteWorld->mainSWGDH;

    saveState = HGetState((Handle)cIconH);
    HLock((Handle)cIconH);

    maskBitMap.rowBytes = (**cIconH).iconMask.rowBytes;
    maskBitMap.bounds = (**cIconH).iconMask.bounds;
    maskBitMap.baseAddr = (Ptr)(**cIconH).iconMaskData;

    if (useLegacyRenderingPipeline) {
        if ((**((**theGDH).gdPMap)).pixelSize == depth)
            err = (NewGWorld)(&tempGWorldP, depth, &maskBitMap.bounds, NULL, theGDH, noNewDevice);
        else
            err = (NewGWorld)(&tempGWorldP, depth, &maskBitMap.bounds, NULL, NULL, 0);
    } else {
        err = (NewGWorld)(&tempGWorldP, 16, &maskBitMap.bounds, NULL, NULL, kNativeEndianPixMap);
    }

    if (err == noErr) {
        SetGWorld(tempGWorldP, NULL);

        LockPixels(GetGWorldPixMap(tempGWorldP));
#if TARGET_RT_LITTLE_ENDIAN
        iconRect = (**cIconH).iconBMap.bounds;
        EraseRect(&iconRect);
        PlotCIcon(&iconRect, cIconH);

        pixMapH = GetGWorldPixMap(tempGWorldP);
        baseAddr = (unsigned char *)GetPixBaseAddr(pixMapH);
        rowBytes = GetPixRowBytes(pixMapH);
        height = iconRect.bottom - iconRect.top;
        if (baseAddr != NULL) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < rowBytes; x++) {
                    if (baseAddr[y * rowBytes + x] != 0xFF)
                        baseAddr[y * rowBytes + x] = 0;
                }
            }
        }
#else
        CopyBits(&maskBitMap,
                 (BitMap *)*GetGWorldPixMap(tempGWorldP),
                 &maskBitMap.bounds,
                 &maskBitMap.bounds,
                 srcCopy,
                 NULL);
#endif
        UnlockPixels(GetGWorldPixMap(tempGWorldP));

        *maskGWorldP = tempGWorldP;
    }

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveGDevice);
    else
        SetGWorld(saveGWorld, NULL);

    HSetState((Handle)cIconH, saveState);

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                  CGrafPtr *maskGWorldP, Image_t *cIconH)
{
    CGrafPtr saveGWorld;
    Rect bounds;
    CGrafPtr tempGWorldP;
    int32_t srcSkip;
    short err;
    PixMapPtr maskPix;
    PixMapPtr pixBounds;
    void *saveGDevice;
    short depth;
    int32_t y;
    int32_t width;
    int32_t x;
    unsigned char *src;
    unsigned short *dst;
    int32_t dstSkip;

    GetPort(&saveGWorld);
    saveGDevice = GetGWorldDevice(saveGWorld);

    depth = destSpriteWorld->pixelDepth;

    SetRect(&bounds, 0, 0, cIconH->Width, cIconH->Height);

#ifdef CEFIX /* F11.04 */
    err = CE_NewGWorld(&tempGWorldP, depth, &bounds, 0);
#else
    err = NewGWorld(&tempGWorldP, depth, &bounds, 0);
#endif

    if ((int)err == noErr) {
        SetPort(tempGWorldP);

        src = cIconH->ImagePtr;
        srcSkip = cIconH->RowBytes - cIconH->Width;

        maskPix = GetCWindowPixMap(tempGWorldP);
        dst = GetPixBaseAddr(maskPix);

        pixBounds = GetGWorldPixMap((CGrafPtr)maskPix);
        dstSkip = GetPixRowBytes(maskPix)
                - (pixBounds->bounds.right - pixBounds->bounds.left) * 2;

        y = cIconH->Height;
        width = cIconH->Width;

        do {
            x = width;

            do {
                *dst = (*src == 0) ? 0 : kWhite555;
                dst++;
                src++;
            } while (--x != 0);

            src += srcSkip;
            dst = (unsigned short *)((char *)dst + dstSkip);
        } while (--y != 0);

        InvertRect(&bounds);

        *maskGWorldP = tempGWorldP;
    }

    SetPort(saveGWorld);

    SWSetStickyIfError(err);

    return err;
}
#endif
