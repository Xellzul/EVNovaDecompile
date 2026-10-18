// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* imagecicn.c -- BurgerLib's ImageParseCicn (win 0x004d2bd0), the cicn decoder GetCIcon feeds a raw resource. Notes: Research/notes/imagecicn.md */
#include "macport.h"
/* Image_t is pinned by BurgerLib's period Burger.h (2004): ImagePtr +0, PalettePtr
   +4, AlphaPtr +8, Width +0xc, Height +0x10, RowBytes +0x14, DataType +0x18. Index 0's
   colour is relocated to a free slot so 0 can mean black/transparent. */

#include "burgertypes.h"

#include "burgerplat.h"
#include "burger.h"

/* same shape as endianswap.c's helper - `static __inline` is the only
   spelling that inlines an asm block in C */
static __inline unsigned short SwapUShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

/* ImageParseCicn. STATE: MATCH, exe 396 / obj 396, byte-identical yes (1427 bytes).
   OPEN: none */
// FUNCTION: EVNOVA 0x004d2bd0
int ImageParseCicn(Image_t *Output, const Word8 *InputPtr)
{
    Word8 *WorkPtr;       /* walks the resource: header, mask, bitmap, color table */
    Word8 *dst;
    Word16 *sp;
    Word8 *maskRBPtr;     /* &iconMask.rowBytes */
    Word8 *maskPtr;       /* the 1-bit mask data */
    Word8 used[256];
    long depth;
    Word8 *DataPtr;       /* walks the color table, then the pixels */
    long rowFlags;
    Word8 *bmapRBPtr;
    long y;
    Word count;

    WorkPtr = (Word8 *)InputPtr;
    FastMemSet(Output, 0, 0x1c);
    sp = (Word16 *)WorkPtr;
    Output->Width = (short)SwapUShort(sp + 6)
                  - (short)SwapUShort(sp + 4);
    Output->Height = (short)SwapUShort(sp + 5)
                   - (short)SwapUShort(sp + 3);
    rowFlags = SwapUShort(sp + 2);
    if (sp[7] != 0 || sp[8] != 0
        || (rowFlags & 0xe000) != 0x8000) {
        NonFatal("Invalid / unsupported cicn header header\n");
        goto Abort;
    }
    depth = (short)SwapUShort(sp + 0x10);
    if (!(depth == 1 || depth == 2 || depth == 4 || depth == 8)) {
        NonFatal("Cicn's must be 2, 4 or 8 bits per pixel!\n");
        goto Abort;
    }
    Output->DataType = IMAGE8_PAL;
    Output->RowBytes = Output->Width;
    if (Output->Width == 0) {
        NonFatal("Width can't be zero.");
        goto Abort;
    }
    if (Output->Height == 0) {
        NonFatal("Height can't be zero.");
        goto Abort;
    }
    dst = AllocAPointer(Output->Width * Output->Height);
    if (dst == 0)
        goto Abort;
    Output->ImagePtr = dst;
    maskRBPtr = WorkPtr + 0x36;
    bmapRBPtr = WorkPtr + 0x44;
    WorkPtr += 0x52;
    maskPtr = WorkPtr;
    WorkPtr += SwapUShort((Word16 *)maskRBPtr) * Output->Height;
    WorkPtr += SwapUShort((Word16 *)bmapRBPtr) * Output->Height;
    FastMemSet(used, 0, 0x100);
    used[0] = 0xff;
    count = SwapUShort((Word16 *)(WorkPtr + 6)) + 1;
    {
        Word8 *pal;

        pal = AllocAPointerClear(0x300);
        if (pal == 0)
            goto Abort;
        Output->PalettePtr = pal;
        DataPtr = WorkPtr + 8;
        do {
            Word index;
            Word8 *dp;

            index = SwapUShort((Word16 *)DataPtr);
            if (index < 0x100) {
                used[index] = index;
                dp = pal + index * 3;
                dp[0] = DataPtr[2];
                dp[1] = DataPtr[4];
                dp[2] = DataPtr[6];
            }
            DataPtr += 8;
        } while (--count);
    }
    y = Output->Height;
    dst = Output->ImagePtr;
    if (depth == 8) {
        long rb;

        rb = rowFlags & 0x1fff;
        do {
            memcpy(dst, DataPtr, Output->Width);
            dst += Output->Width;
            DataPtr += rb;
        } while (--y);
    } else if (depth == 4) {
        long rb;

        rb = rowFlags & 0x1fff;
        do {
            long x;
            Word8 *src;

            x = Output->Width;
            src = DataPtr;
            do {
                Word c;

                c = *src++;
                *dst++ = (c >> 4);
                if (--x == 0)
                    break;
                *dst++ = (c & 0xf);
            } while (--x);
            DataPtr += rb;
        } while (--y);
    } else if (depth == 2) {
        long rb;

        rb = rowFlags & 0x1fff;
        do {
            long x;
            Word8 *src;

            x = Output->Width;
            src = DataPtr;
            do {
                Word c;

                c = *src++;
                *dst++ = (c >> 6);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 4) & 3);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 2) & 3);
                if (--x == 0)
                    break;
                *dst++ = (c & 3);
            } while (--x);
            DataPtr += rb;
        } while (--y);
    } else if (depth == 1) {
        long rb;

        rb = rowFlags & 0x1fff;
        do {
            long x;
            Word8 *src;

            x = Output->Width;
            src = DataPtr;
            do {
                Word c;

                c = *src++;
                *dst++ = ((c >> 7) & 1);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 6) & 1);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 5) & 1);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 4) & 1);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 3) & 1);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 2) & 1);
                if (--x == 0)
                    break;
                *dst++ = ((c >> 1) & 1);
                if (--x == 0)
                    break;
                *dst++ = (c & 1);
            } while (--x);
            DataPtr += rb;
        } while (--y);
    }
    if (used[0] != 0xff) {
        Word8 *pal;
        Word i;

        pal = Output->PalettePtr;
        i = 1;
        do {
            if (i != used[i]) {
                Word8 *dp;

                used[0] = i;
                dp = pal + i * 3;
                dp[0] = pal[0];
                dp[1] = pal[1];
                dp[2] = pal[2];
                break;
            }
        } while (++i < 0xff);
        /* BUG: the scan stops at 0xff, so this never runs and a full palette loses index 0's colour */
        if (i >= 0x100)
            used[0] = PaletteFindColorIndex(pal + 3, pal[0], pal[1],
                                            pal[2], 0xfe) + 1;
        pal[0] = 0;
        pal[1] = 0;
        pal[2] = 0;
        ImageRemapIndexed(Output, used);
    }
    {
        long rb;
        Word c;
        long row;
        long x;
        Word bit;
        Word8 *src;

        rb = SwapUShort((Word16 *)maskRBPtr) & 0x1fff;
        row = Output->Height;
        dst = Output->ImagePtr;
        do {
            x = Output->Width;
            src = maskPtr;
            bit = 0x80;
            c = *src++;
            do {
                if (!(c & bit))
                    *dst = 0;
                bit >>= 1;
                if (bit == 0) {
                    bit = 0x80;
                    c = *src++;
                }
                dst++;
            } while (--x);
            maskPtr += rb;
        } while (--row);
    }
    return 0;
Abort:
    ImageDestroy(Output);
    return 1;
}
