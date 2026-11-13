// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* swcreatepixelmaskfromgworld.c -- SWCreatePixelMaskFromGWorld (win 0x00479250, donor SpriteWorldUtils.c line 930). Notes: Research/notes/swcreatepixelmaskfromgworld.md */
/* A four-byte stub, `or eax,-1 / ret': EV Nova asks for a pixel mask nowhere, so the
   porter replaced the donor's 780-byte body with a failure return rather than deleting
   the entry point its callers still reference. -1 is not an OSErr, it is "did not happen". */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#ifdef MACBUILD
#include "render.h"
#define kSolid  0x00000000UL
#define kClear  0xFFFFFFFFUL
#endif

/* SWCreatePixelMaskFromGWorld (mac _SWCreatePixelMaskFromGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00479250
// MAC: _SWCreatePixelMaskFromGWorld i386 0x000a875d ppc 0x000b36b8
#ifdef MACBUILD
short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld, Rect *rectP)
{
    UInt32 transparent;
    GWorldPtr saveGWorld;
    GDHandle saveDevice;
    Ptr baseAddr;
    Ptr maskAddr;
    int32_t rowBytes;
    int32_t width, height;
    int32_t depth;
    int32_t x, y;
    unsigned short transparentValue;
    PixMapHandle pictPix;
    PixMapHandle maskPix;
    GWorldFlags pictPixelsState;
    GWorldFlags maskPixelsState;
    OSErr err = noErr;

    pictPix = GetGWorldPixMap(pictureGWorld);
    maskPix = GetGWorldPixMap(maskGWorld);

    SW_ASSERT((**pictPix).pixelSize == (**maskPix).pixelSize, 1015);

    if (rectP == NULL)
        rectP = GetCPortPortRect(pictureGWorld);

    width = rectP->right - rectP->left;
    height = rectP->bottom - rectP->top;

    depth = (**pictPix).pixelSize;
    ComputePixelValues(depth);
    transparentValue = SWTransparentRGBValue;

    if (gSWTransparentColor.red != transparentValue ||
            gSWTransparentColor.green != transparentValue ||
            gSWTransparentColor.blue != transparentValue) {
        return SWCreateMaskAndFixImage(pictureGWorld, maskGWorld, rectP);
    }

    if (depth < 8) {
        SWCopyGWorlds(pictureGWorld, maskGWorld);
        SWBlackenGWorld(maskGWorld);
        if (depth <= 8)
            SWInvertGWorld(maskGWorld);

        return noErr;
    }

    GetGWorld(&saveGWorld, &saveDevice);

    pictPixelsState = GetPixelsState(pictPix);
    LockPixels(pictPix);
    maskPixelsState = GetPixelsState(maskPix);
    LockPixels(maskPix);

    SetGWorld(pictureGWorld, NULL);
    transparent = Color2Index(&gSWTransparentColor);

    baseAddr = SWGetPixBaseAddr(pictPix);
    maskAddr = SWGetPixBaseAddr(maskPix);
    rowBytes = SWGetPixRowBytes(pictPix);

    baseAddr += rectP->top * rowBytes;
    baseAddr += (rectP->left * depth) >> 3;
    maskAddr += rectP->top * rowBytes;
    maskAddr += (rectP->left * depth) >> 3;
    rowBytes -= (width * depth) >> 3;

    switch (depth) {
    case 8:
        {
            UInt8 *dst = (UInt8 *)baseAddr;
            UInt8 *mask = (UInt8 *)maskAddr;

            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if (*dst++ == transparent)
                        *mask++ = kClear;
                    else
                        *mask++ = kSolid;
                }
                dst = (UInt8 *)(dst + rowBytes);
                mask = (UInt8 *)(mask + rowBytes);
            }
        }
        break;
    case 16:
        {
            UInt16 *dst = (UInt16 *)baseAddr;
            UInt16 *mask = (UInt16 *)maskAddr;

            transparent &= kAlphaMask16;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if ((*dst++ & kAlphaMask16) == transparent)
                        *mask++ = kClear;
                    else
                        *mask++ = kSolid;
                }
                dst = (UInt16 *)((char *)dst + rowBytes);
                mask = (UInt16 *)((char *)mask + rowBytes);
            }
        }
        break;
    case 32:
        {
            UInt32 *dst = (UInt32 *)baseAddr;
            UInt32 *mask = (UInt32 *)maskAddr;

            transparent &= kAlphaMask32;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if ((*dst++ & kAlphaMask32) == transparent)
                        *mask++ = kClear;
                    else
                        *mask++ = kSolid;
                }
                dst = (UInt32 *)((char *)dst + rowBytes);
                mask = (UInt32 *)((char *)mask + rowBytes);
            }
        }
        break;
    default:
        err = paramErr;
    }

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveDevice);
    else
        SetGWorld(saveGWorld, NULL);

    SetPixelsState(pictPix, pictPixelsState);
    SetPixelsState(maskPix, maskPixelsState);

    return err;
}
#else
short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld,
                                  Rect *rectP)
{
    return -1;
}
#endif
