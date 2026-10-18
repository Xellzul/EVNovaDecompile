// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreategworldfromciconmask.c -- SWCreateGWorldFromCIconMask (win 0x00479070, donor SpriteWorldUtils.c line 444). Notes: Research/notes/swcreategworldfromciconmask.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#include "burgerplat.h"

#define kWhite555   0x7fff

/* SWCreateGWorldFromCIconMask (mac _SWCreateGWorldFromCIconMask). STATE: MATCH, relocregcheck
   12 rows 0 differ. OPEN: none */
// FUNCTION: EVNOVA 0x00479070
short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                  CGrafPtr *maskGWorldP, Image_t *cIconH)
{
    CGrafPtr saveGWorld;
    Rect bounds;
    CGrafPtr tempGWorldP;
    long srcSkip;
    short err;
    PixMapPtr maskPix;
    PixMapPtr pixBounds;
    void *saveGDevice;
    short depth;
    long y;
    long width;
    long x;
    unsigned char *src;
    unsigned short *dst;
    long dstSkip;

    GetPort(&saveGWorld);
    saveGDevice = GetGWorldDevice(saveGWorld);

    depth = destSpriteWorld->pixelDepth;

    SetRect(&bounds, 0, 0, cIconH->Width, cIconH->Height);

    err = NewGWorld(&tempGWorldP, depth, &bounds, 0);

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
