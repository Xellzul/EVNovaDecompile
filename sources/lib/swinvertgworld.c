// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swinvertgworld.c -- SWInvertGWorld (win 0x004791b0, donor SpriteWorldUtils.c line 506) and SWInvertRect (win 0x004791d0, donor line 518). Notes: Research/notes/swinvertgworld.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#ifdef MACBUILD
#include "render.h"
#endif

/* SWInvertGWorld (mac _SWInvertGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004791b0
// MAC: _SWInvertGWorld i386 0x000a703a ppc 0x000b1c60
#ifdef MACBUILD
void SWInvertGWorld(CGrafPtr gWorldP)
{
    SWInvertRect(gWorldP, GetCPortPortRect(gWorldP));
}
#else
void SWInvertGWorld(CGrafPtr gWorldP)
{
    SWInvertRect(gWorldP, GetWindowPortRect(gWorldP));
}
#endif

/* SWInvertRect (mac _SWInvertRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004791d0
// MAC: _SWInvertRect i386 0x000a6fa2 ppc 0x000b1bc4
#ifdef MACBUILD
void SWInvertRect(CGrafPtr gWorldP, Rect *rectP)
{
    PixMapHandle pixMapH;
    GWorldFlags pixelsState;
    GWorldPtr saveGWorld;
    GDHandle saveDevice;

    pixMapH = GetGWorldPixMap(gWorldP);
    pixelsState = GetPixelsState(pixMapH);
    LockPixels(pixMapH);

    GetGWorld(&saveGWorld, &saveDevice);
    SetGWorld(gWorldP, NULL);

    InvertRect(rectP);

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveDevice);
    else
        SetGWorld(saveGWorld, NULL);
    SetPixelsState(pixMapH, pixelsState);
}
#else
void SWInvertRect(CGrafPtr gWorldP, Rect *rectP)
{
    CGrafPtr saveGWorld;
    void *saveDevice;

    GetPort(&saveGWorld);
    saveDevice = GetGWorldDevice(saveGWorld);
    SetPort(gWorldP);

    InvertRect(rectP);

    SetPort(saveGWorld);
}
#endif

#ifdef MACBUILD
/* SWCopyGWorlds (mac _SWCopyGWorlds, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyGWorlds i386 0x000a705e ppc 0x000b1c94
void SWCopyGWorlds(GWorldPtr srcGWorldP, GWorldPtr dstGWorldP)
{
    PixMapHandle srcPixMapH;
    PixMapHandle dstPixMapH;
    GWorldFlags srcPixelsState;
    GWorldFlags dstPixelsState;
    GWorldPtr saveGWorld;
    GDHandle saveDevice;

    srcPixMapH = GetGWorldPixMap(srcGWorldP);
    dstPixMapH = GetGWorldPixMap(dstGWorldP);

    srcPixelsState = GetPixelsState(srcPixMapH);
    dstPixelsState = GetPixelsState(dstPixMapH);
    LockPixels(srcPixMapH);
    LockPixels(dstPixMapH);

    GetGWorld(&saveGWorld, &saveDevice);
    SetGWorld(dstGWorldP, NULL);

    CopyBits((BitMap *)*srcPixMapH, (BitMap *)*dstPixMapH,
             GetCPortPortRect(srcGWorldP), GetCPortPortRect(dstGWorldP), srcCopy, NULL);

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveDevice);
    else
        SetGWorld(saveGWorld, NULL);

    SetPixelsState(srcPixMapH, srcPixelsState);
    SetPixelsState(dstPixMapH, dstPixelsState);
}

/* SWFlipPixelsHorizontally (mac _SWFlipPixelsHorizontally, mac-only). STATE: see the mac gate. */
// MAC: _SWFlipPixelsHorizontally i386 0x000a7152 ppc 0x000b1d90
void SWFlipPixelsHorizontally(GWorldPtr dstGWorldP, Rect *rectP)
{
    Ptr baseAddr, row;
    UInt32 rowBytes;
    Rect bounds;
    int width, height;
    int x, y, depth;
    GWorldFlags state;
    PixMapHandle pixMapHdl;

    pixMapHdl = GetGWorldPixMap(dstGWorldP);

    state = GetPixelsState(pixMapHdl);
    LockPixels(pixMapHdl);

    depth = (**pixMapHdl).pixelSize;
    if (rectP == NULL)
        bounds = (**pixMapHdl).bounds;
    else
        bounds = *rectP;

    width = bounds.right - bounds.left;
    height = bounds.bottom - bounds.top;

    baseAddr = SWGetPixBaseAddr(pixMapHdl);
    rowBytes = SWGetPixRowBytes(pixMapHdl);

    baseAddr += bounds.top * rowBytes;
    baseAddr += (bounds.left * depth) >> 3;

    row = baseAddr;

    switch (depth) {
    case 1:
        {
            Boolean pixel1, pixel2;
            int x2;

            for (y = 0; y < height; y++) {
                for (x = 0, x2 = (width - 1); x < (width >> 1); x++, x2--) {
                    pixel1 = BitTst(row, x);
                    pixel2 = BitTst(row, x2);

                    pixel1 ? BitSet(row, x2) : BitClr(row, x2);
                    pixel2 ? BitSet(row, x) : BitClr(row, x);
                }
                row += rowBytes;
            }
            break;
        }
    case 2:
    case 4:
        {
            RGBColor color1, color2;
            int x2;
            GWorldPtr saveGWorld;
            GDHandle saveGDevice;

            GetGWorld(&saveGWorld, &saveGDevice);
            SetGWorld(dstGWorldP, NULL);

            for (y = 0; y < height; y++) {
                for (x = 0, x2 = (width - 1); x < (width >> 1); x++, x2--) {
                    GetCPixel(x, y, &color1);
                    GetCPixel(x2, y, &color2);

                    SetCPixel(x2, y, &color1);
                    SetCPixel(x, y, &color2);
                }
            }

            if (useLegacyRenderingPipeline)
                SetGWorld(saveGWorld, saveGDevice);
            else
                SetGWorld(saveGWorld, NULL);

            break;
        }
    case 8:
        {
            UInt8 *ptr1, *ptr2, temp;

            for (y = 0; y < height; y++) {
                ptr1 = (UInt8 *)row;
                ptr2 = ptr1 + width;
                for (x = 0; x < (width >> 1); x++) {
                    temp = *--ptr2;
                    *ptr2 = *ptr1;
                    *ptr1++ = temp;
                }
                row += rowBytes;
            }
            break;
        }
    case 16:
        {
            UInt16 *ptr1, *ptr2, temp;

            for (y = 0; y < height; y++) {
                ptr1 = (UInt16 *)row;
                ptr2 = ptr1 + width;
                for (x = 0; x < (width >> 1); x++) {
                    temp = *--ptr2;
                    *ptr2 = *ptr1;
                    *ptr1++ = temp;
                }
                row += rowBytes;
            }
            break;
        }
    case 32:
        {
            UInt32 *ptr1, *ptr2, temp;

            for (y = 0; y < height; y++) {
                ptr1 = (UInt32 *)row;
                ptr2 = ptr1 + width;
                for (x = 0; x < (width >> 1); x++) {
                    temp = *--ptr2;
                    *ptr2 = *ptr1;
                    *ptr1++ = temp;
                }
                row += rowBytes;
            }
            break;
        }
    default:
        break;
    }

    SetPixelsState(pixMapHdl, state);
}

/* SWFlipPixelsVertically (mac _SWFlipPixelsVertically, mac-only). STATE: see the mac gate. */
// MAC: _SWFlipPixelsVertically i386 0x000a74e7 ppc 0x000b2188
void SWFlipPixelsVertically(GWorldPtr dstGWorldP, Rect *rectP)
{
    Ptr baseAddr, row1, row2;
    UInt32 rowBytes;
    Rect bounds;
    int width, height;
    int x, y, depth;
    GWorldFlags state;
    PixMapHandle pixMapHdl;

    pixMapHdl = GetGWorldPixMap(dstGWorldP);

    state = GetPixelsState(pixMapHdl);
    LockPixels(pixMapHdl);

    depth = (**pixMapHdl).pixelSize;
    if (rectP == NULL)
        bounds = (**pixMapHdl).bounds;
    else
        bounds = *rectP;

    width = bounds.right - bounds.left;
    height = bounds.bottom - bounds.top;

    baseAddr = SWGetPixBaseAddr(pixMapHdl);
    rowBytes = SWGetPixRowBytes(pixMapHdl);

    baseAddr += bounds.top * rowBytes;
    baseAddr += (bounds.left * depth) >> 3;

    row1 = baseAddr;
    row2 = baseAddr + (height - 1) * rowBytes;

    switch (depth) {
    case 1:
        {
            Boolean pixel1, pixel2;

            for (y = 0; y < (height >> 1); y++) {
                for (x = 0; x < width; x++) {
                    pixel1 = BitTst(row1, x);
                    pixel2 = BitTst(row2, x);

                    pixel1 ? BitSet(row2, x) : BitClr(row2, x);
                    pixel2 ? BitSet(row1, x) : BitClr(row1, x);
                }
                row1 += rowBytes;
                row2 -= rowBytes;
            }
            break;
        }
    case 2:
    case 4:
        {
            RGBColor color1, color2;
            int y2;
            GWorldPtr saveGWorld;
            GDHandle saveGDevice;

            GetGWorld(&saveGWorld, &saveGDevice);
            SetGWorld(dstGWorldP, NULL);

            for (y = 0, y2 = (height - 1); y < (height >> 1); y++, y2--) {
                for (x = 0; x < width; x++) {
                    GetCPixel(x, y, &color1);
                    GetCPixel(x, y2, &color2);

                    SetCPixel(x, y2, &color1);
                    SetCPixel(x, y, &color2);
                }
            }

            if (useLegacyRenderingPipeline)
                SetGWorld(saveGWorld, saveGDevice);
            else
                SetGWorld(saveGWorld, NULL);

            break;
        }
    case 8:
        {
            UInt8 *ptr1, *ptr2, temp;

            for (y = 0; y < (height >> 1); y++) {
                ptr1 = (UInt8 *)row1;
                ptr2 = (UInt8 *)row2;
                for (x = 0; x < width; x++) {
                    temp = *ptr1;
                    *ptr1++ = *ptr2;
                    *ptr2++ = temp;
                }
                row1 += rowBytes;
                row2 -= rowBytes;
            }
            break;
        }
    case 16:
        {
            UInt16 *ptr1, *ptr2, temp;

            for (y = 0; y < (height >> 1); y++) {
                ptr1 = (UInt16 *)row1;
                ptr2 = (UInt16 *)row2;
                for (x = 0; x < width; x++) {
                    temp = *ptr1;
                    *ptr1++ = *ptr2;
                    *ptr2++ = temp;
                }
                row1 += rowBytes;
                row2 -= rowBytes;
            }
            break;
        }
    case 32:
        {
            UInt32 *ptr1, *ptr2, temp;

            for (y = 0; y < (height >> 1); y++) {
                ptr1 = (UInt32 *)row1;
                ptr2 = (UInt32 *)row2;
                for (x = 0; x < width; x++) {
                    temp = *ptr1;
                    *ptr1++ = *ptr2;
                    *ptr2++ = temp;
                }
                row1 += rowBytes;
                row2 -= rowBytes;
            }
            break;
        }
    default:
        break;
    }

    SetPixelsState(pixMapHdl, state);
}

/* SWSetTransparentColor (mac _SWSetTransparentColor, mac-only). STATE: see the mac gate. */
// MAC: _SWSetTransparentColor i386 0x000a7810 ppc 0x000b2590
void SWSetTransparentColor(RGBColor *RGB)
{
    gSWTransparentColor = *RGB;
}
#endif
