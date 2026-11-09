// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swfiximagegworld.c -- SWFixImageGWorld (win 0x00479260, SpriteWorldUtils.c line 1212, donor SpriteWorldUtils.c line 1099). Notes: Research/notes/swfiximagegworld.md */
/* kClear is all-ones (-1) and kSolid zero, one constant each: the exe stores 0xff,
   0xffff and 0xffffffff and the truncation does the rest. `case 8:' is unreachable below
   the `depth <= 8' return and is still compiled -- dropping it loses a dispatch edge. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#ifdef MACBUILD
#include "render.h"
#endif

#define paramErr    (-50)
#undef kClear
#undef kSolid
#define kClear     (-1)
#define kSolid      0

/* SWFixImageGWorld (mac _SWFixImageGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00479260
// MAC: _SWFixImageGWorld i386 0x000a820c ppc 0x000b3028
#ifdef MACBUILD
short SWFixImageGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld, Rect *rectP)
{
    GWorldPtr saveGWorld;
    GDHandle saveDevice;
    Ptr baseAddr;
    Ptr maskAddr;
    long rowBytes;
    long width, height;
    long depth;
    long x, y;
    unsigned short transparentValue;
    PixMapHandle pictPix;
    PixMapHandle maskPix;
    GWorldFlags pictPixelsState;
    GWorldFlags maskPixelsState;
    OSErr err = noErr;

    pictPix = GetGWorldPixMap(pictureGWorld);
    maskPix = GetGWorldPixMap(maskGWorld);

    SW_ASSERT((**pictPix).pixelSize == (**maskPix).pixelSize, 1186);

    if (rectP == NULL)
        rectP = GetCPortPortRect(pictureGWorld);

    width = rectP->right - rectP->left;
    height = rectP->bottom - rectP->top;

    depth = (**pictPix).pixelSize;
    ComputePixelValues(depth);
    transparentValue = SWTransparentRGBValue;

    if (depth <= 8) {
        SWWhitenGWorld(pictureGWorld);
        return noErr;
    }

    GetGWorld(&saveGWorld, &saveDevice);

    pictPixelsState = GetPixelsState(pictPix);
    LockPixels(pictPix);
    maskPixelsState = GetPixelsState(maskPix);
    LockPixels(maskPix);

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
                    if (*mask) {
                        *dst++ = 0;
                        *mask++ = kClear;
                    } else {
                        dst++;
                        *mask++ = kSolid;
                    }
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

            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if (*mask) {
                        *dst++ = 0;
                        *mask++ = kClear;
                    } else {
                        dst++;
                        *mask++ = kSolid;
                    }
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

            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if (*mask) {
                        *dst++ = 0;
                        *mask++ = kClear;
                    } else {
                        dst++;
                        *mask++ = kSolid;
                    }
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
short SWFixImageGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld, Rect *rectP)
{
    CGrafPtr saveGWorld;
    void *saveDevice;
    char *baseAddr;
    char *maskAddr;
    long rowBytes;
    long width, height;
    long depth;
    long x, y;
    unsigned short transparentValue;
    PixMapPtr pictPix;
    PixMapPtr maskPix;
    short err = noErr;

    pictPix = GetCWindowPixMap(pictureGWorld);
    maskPix = GetCWindowPixMap(maskGWorld);

    if (GetPixDepth(pictPix) != GetPixDepth(maskPix))
        SWAssertFail("SpriteWorldUtils.c", 1212);

    if (rectP == 0)
        rectP = GetWindowPortRect(pictureGWorld);

    width = rectP->right - rectP->left;
    height = rectP->bottom - rectP->top;

    depth = GetPixDepth(pictPix);
    ComputePixelValues(depth);
    transparentValue = SWTransparentRGBValue;

    if (depth <= 8)
        return noErr;

    GetPort(&saveGWorld);
    saveDevice = GetGWorldDevice(saveGWorld);

    baseAddr = GetPixBaseAddr(pictPix);
    maskAddr = GetPixBaseAddr(maskPix);
    rowBytes = GetPixRowBytes(pictPix);

    baseAddr += rectP->top * rowBytes;
    baseAddr += (rectP->left * depth) >> 3;
    maskAddr += rectP->top * rowBytes;
    maskAddr += (rectP->left * depth) >> 3;
    rowBytes -= (width * depth) >> 3;

    switch (depth) {
        case 8:
            {
                unsigned char *dst = (unsigned char *)baseAddr;
                unsigned char *mask = (unsigned char *)maskAddr;

                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++) {
                        if (*mask) {
                            *dst++ = 0;
                            *mask++ = kClear;
                        } else {
                            dst++;
                            *mask++ = kSolid;
                        }
                    }
                    dst = dst + rowBytes;
                    mask = mask + rowBytes;
                }
            }
            break;

        case 16:
            {
                unsigned short *dst = (unsigned short *)baseAddr;
                unsigned short *mask = (unsigned short *)maskAddr;

                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++) {
                        if (*mask) {
                            *dst++ = 0;
                            *mask++ = kClear;
                        } else {
                            dst++;
                            *mask++ = kSolid;
                        }
                    }
                    dst = (unsigned short *)((char *)dst + rowBytes);
                    mask = (unsigned short *)((char *)mask + rowBytes);
                }
            }
            break;

        case 32:
            {
                unsigned long *dst = (unsigned long *)baseAddr;
                unsigned long *mask = (unsigned long *)maskAddr;

                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++) {
                        if (*mask) {
                            *dst++ = 0;
                            *mask++ = kClear;
                        } else {
                            dst++;
                            *mask++ = kSolid;
                        }
                    }
                    dst = (unsigned long *)((char *)dst + rowBytes);
                    mask = (unsigned long *)((char *)mask + rowBytes);
                }
            }
            break;

        default:
            err = paramErr;
    }

    SetPort(saveGWorld);

    return err;
}
#endif

#ifdef MACBUILD
/* SWCreateMaskAndFixImage (mac _SWCreateMaskAndFixImage, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateMaskAndFixImage i386 0x000a8480 ppc 0x000b3348
OSErr SWCreateMaskAndFixImage(GWorldPtr pictureGWorld, GWorldPtr maskGWorld, Rect *rectP)
{
    UInt32 transparent;
    GWorldPtr saveGWorld;
    GDHandle saveDevice;
    Ptr baseAddr;
    Ptr maskAddr;
    long rowBytes;
    long width, height;
    long depth;
    long x, y;
    unsigned short transparentValue;
    PixMapHandle pictPix;
    PixMapHandle maskPix;
    GWorldFlags pictPixelsState;
    GWorldFlags maskPixelsState;

    pictPix = GetGWorldPixMap(pictureGWorld);
    maskPix = GetGWorldPixMap(maskGWorld);

    SW_ASSERT((**pictPix).pixelSize == (**maskPix).pixelSize, 1359);

    if (rectP == NULL)
        rectP = GetCPortPortRect(pictureGWorld);

    width = rectP->right - rectP->left;
    height = rectP->bottom - rectP->top;

    depth = (**pictPix).pixelSize;
    ComputePixelValues(depth);
    transparentValue = SWTransparentRGBValue;

    if (depth < 8) {
        SWCopyGWorlds(pictureGWorld, maskGWorld);
        SWBlackenGWorld(maskGWorld);
        if (depth <= 8)
            SWInvertGWorld(maskGWorld);
        SWWhitenGWorld(pictureGWorld);

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
                    if (*dst == transparent) {
                        *dst++ = 0;
                        *mask++ = kClear;
                    } else {
                        dst++;
                        *mask++ = kSolid;
                    }
                }
                dst = (UInt8 *)(dst + rowBytes);
                mask = (UInt8 *)((char *)mask + rowBytes);
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
                    if ((*dst & kAlphaMask16) == transparent) {
                        *dst++ = 0;
                        *mask++ = kClear;
                    } else {
                        dst++;
                        *mask++ = kSolid;
                    }
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
                    if ((*dst & kAlphaMask32) == transparent) {
                        *dst++ = 0;
                        *mask++ = kClear;
                    } else {
                        dst++;
                        *mask++ = kSolid;
                    }
                }
                dst = (UInt32 *)((char *)dst + rowBytes);
                mask = (UInt32 *)((char *)mask + rowBytes);
            }
        }
        break;
    default:
        return paramErr;
    }

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveDevice);
    else
        SetGWorld(saveGWorld, NULL);

    SetPixelsState(pictPix, pictPixelsState);
    SetPixelsState(maskPix, maskPixelsState);

    return noErr;
}

/* SWBlackenGWorld (mac _SWBlackenGWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWBlackenGWorld i386 0x000a782a ppc 0x000b25ac
OSErr SWBlackenGWorld(GWorldPtr oldGWorld)
{
    return noErr;
}

/* SWCreateRegionMaskFromGWorld (mac _SWCreateRegionMaskFromGWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateRegionMaskFromGWorld i386 0x000a7838 ppc 0x000b25bc
OSErr SWCreateRegionMaskFromGWorld(CGrafPtr pictureGWorld, Rect *pictureRect, RgnHandle maskRegion)
{
    return noErr;
}

/* SWWhitenGWorld (mac _SWWhitenGWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWWhitenGWorld i386 0x000a7831 ppc 0x000b25b4
OSErr SWWhitenGWorld(GWorldPtr oldGWorld)
{
    return noErr;
}

/* SWFastWhitenGWorld (mac _SWFastWhitenGWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWFastWhitenGWorld i386 0x000a783f ppc 0x000b25c4
OSErr SWFastWhitenGWorld(GWorldPtr pictureGWorld)
{
    UInt32 transparent;
    GWorldPtr saveGWorld;
    GDHandle saveDevice;
    Ptr baseAddr;
    long rowBytes;
    long width, height;
    long depth, bytes;
    long x, y;
    unsigned short transparentValue;
    PixMapHandle pictPix;
    GWorldFlags pictPixelsState;

    pictPix = GetGWorldPixMap(pictureGWorld);

    width = GetCPortPortRect(pictureGWorld)->right - GetCPortPortRect(pictureGWorld)->left;
    height = GetCPortPortRect(pictureGWorld)->bottom - GetCPortPortRect(pictureGWorld)->top;

    depth = (**pictPix).pixelSize;
    ComputePixelValues(depth);
    transparentValue = SWTransparentRGBValue;

    if (depth < 8)
        return kWrongDepthErr;

    GetGWorld(&saveGWorld, &saveDevice);

    pictPixelsState = GetPixelsState(pictPix);
    LockPixels(pictPix);

    SetGWorld(pictureGWorld, (GDHandle)NULL);
    transparent = Color2Index(&gSWTransparentColor);

    baseAddr = SWGetPixBaseAddr(pictPix);
    rowBytes = SWGetPixRowBytes(pictPix);

    bytes = (width * depth) >> 3;
    rowBytes -= bytes;

    switch (depth) {
    case 8:
        {
            UInt8 *dst = (UInt8 *)baseAddr;

            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if (*dst == transparent)
                        *dst++ = 0;
                    else
                        dst++;
                }
                dst = (UInt8 *)((char *)dst + rowBytes);
            }
        }
        break;
    case 16:
        {
            UInt16 *dst = (UInt16 *)baseAddr;

            transparent &= kAlphaMask16;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if ((*dst & kAlphaMask16) == transparent)
                        *dst++ = 0;
                    else
                        dst++;
                }
                dst = (UInt16 *)((char *)dst + rowBytes);
            }
        }
        break;
    case 32:
        {
            UInt32 *dst = (UInt32 *)baseAddr;

            transparent &= kAlphaMask32;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    if ((*dst & kAlphaMask32) == transparent)
                        *dst++ = 0;
                    else
                        dst++;
                }
                dst = (UInt32 *)((char *)dst + rowBytes);
            }
        }
        break;
    default:
        return paramErr;
    }

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveDevice);
    else
        SetGWorld(saveGWorld, NULL);

    SetPixelsState(pictPix, pictPixelsState);

    return noErr;
}
#endif
