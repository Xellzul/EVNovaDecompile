// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swfiximagegworld.c -- SWFixImageGWorld (win 0x00479260, SpriteWorldUtils.c line 1212, donor SpriteWorldUtils.c line 1099). Notes: Research/notes/swfiximagegworld.md */
/* kClear is all-ones (-1) and kSolid zero, one constant each: the exe stores 0xff,
   0xffff and 0xffffffff and the truncation does the rest. `case 8:' is unreachable below
   the `depth <= 8' return and is still compiled -- dropping it loses a dispatch edge. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#define paramErr    (-50)
#define kClear      (-1)
#define kSolid      0

/* SWFixImageGWorld (mac _SWFixImageGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00479260
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
