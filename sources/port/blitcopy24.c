// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
// COMPILE-WHY: `#pragma opt_repositioncode off' (pragma-only) around CopyBitsScale16Average alone, on
// after CopyBitsScale8 and reset before CopyBitsScale24. Without it the block mover parks the
// nearest-neighbour arm; left on past CopyBitsScale16Average it moves LoadPictResource's dispose arm.
/* blitcopy24.c --tu-map object O27: the 24-bit and scaled blitters, the blit dispatch, and the QuickDraw port, rect, pen, oval, picture and cicn emulation. Notes: Research/notes/blitcopy24.md */
#include <time.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "macport.h"
#include "game.h"
#include "stellar.h"
#include "shipobj.h"
#include "gametypes.h"
#include "gameconst.h"
#include "burger.h"
#include "spriteworld.h"
#include "burgerplat.h"
#include "player.h"

/* CopyBitsCopy24. STATE: MATCH, exe 376 / obj 376 instrs, byte-identical yes (1216 bytes).
   OPEN: none */
// FUNCTION: EVNOVA 0x004b76d0
void CopyBitsCopy24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    long width;
    long height;
    long srcRow;
    long dstRow;
    unsigned char *srcP;
    unsigned short *dstP;
    unsigned long rgb;
    unsigned long rgb2;
    long n;
    unsigned long raw;
    unsigned long raw2;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 3;
    srcRow = src->rowBytes - width * 3;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes)
        + (dstRect->left - dst->bounds.left);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            if (n > 1) {
                do {
                    raw = *(unsigned long *)srcP;
                    raw2 = *(unsigned long *)(srcP + 3);
                    rgb = (raw << 7 & 0x7c00) + (raw >> 6 & 0x3e0) +
                        (raw >> 0x13 & 0x1f);
                    rgb2 = (raw2 << 7 & 0x7c00) + (raw2 >> 6 & 0x3e0) +
                        (raw2 >> 0x13 & 0x1f);
                    *(unsigned long *)dstP = rgb + (rgb2 << 16);
                    srcP += 6;
                    dstP += 2;
                    n -= 2;
                } while (n > 1);
            }
            if (n != 0) {
                rgb = *(unsigned long *)srcP;
                *dstP = ((rgb << 7 & 0x7c00) +
                    (rgb >> 6 & 0x3e0) + (rgb >> 0x13 & 0x1f));
                srcP += 3;
                dstP++;
            }
            srcP += srcRow;
            dstP = (unsigned short *)((unsigned char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

/* CopyBitsScale8. STATE: MATCH, exe 88 / obj 88 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004b7b90
void CopyBitsScale8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                    long xStep, long yStep, long xStart, long yStart)
{
    long width;
    long height;
    long srcRowBytes;
    long dstRow;
    unsigned short *dstP;
    unsigned long rgb;
    unsigned char *table;
    unsigned char *srcBase;
    unsigned char *srcP;
    long x;
    unsigned long n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcRowBytes = src->rowBytes;
    srcBase = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * srcRowBytes
        + (srcRect->left - src->bounds.left);
    table = src->ctab;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            srcP = srcBase + (yStart >> 16) * srcRowBytes;
            n = width;
            x = xStart;
            do {
                rgb = *(unsigned long *)(table + srcP[x >> 16] * 3);
                *dstP = ((rgb << 7 & 0x7c00) + (rgb >> 6 & 0x3e0) +
                                          (rgb >> 0x13 & 0x1f));
                dstP++;
                x += xStep;
                n--;
            } while (n != 0);
            yStart += yStep;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

/* CopyBitsScale16: not linked, dead-stripped in the exe. The 16-bit cell of the scale table
   before CopyBitsScale16Average took it over (1.0.6 still points it at a plain 16-bit scaler); it stays
   because CopyBitsScale16Average is compiled after it (Research/notes/blitscalearea.md) */
void CopyBitsScale16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                     long xStep, long yStep, long xStart, long yStart)
{
    long width;
    long height;
    long srcRowBytes;
    long dstRow;
    unsigned short *dstP;
    unsigned char *srcBase;
    unsigned short *srcP;
    unsigned long a;
    unsigned long b;
    long x;
    long n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcRowBytes = src->rowBytes;
    srcBase = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * srcRowBytes
        + (srcRect->left - src->bounds.left) * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            srcP = (unsigned short *)(srcBase + (yStart >> 16) * srcRowBytes);
            n = width;
            x = xStart;
            if (n > 1) {
                do {
                    a = srcP[x >> 16];
                    x += xStep;
                    b = srcP[x >> 16];
                    x += xStep;
                    *(unsigned long *)dstP = a + (b << 16);
                    dstP += 2;
                    n -= 2;
                } while (n > 1);
            }
            if (n != 0) {
                *dstP = srcP[x >> 16];
                dstP++;
            }
            yStart += yStep;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

#pragma opt_repositioncode off

/* CopyBitsScale16Average. STATE: MATCH, exe 350 / obj 350, byte-identical yes. The 16bpp area-averaging
   scaler, cell [1][1] of the mode-1 table. OPEN: none */
// FUNCTION: EVNOVA 0x004b7cb0
void CopyBitsScale16Average(PixMapPtr srcBits, PixMapPtr dstBits, Rect *srcRect,
                  Rect *dstRect, long xStep, long yStep, long xFrac,
                  long yFrac)
{
    long rowBytes;
    long dstP;
    long width;
    long height;
    long srcRow;
    int dstRow;
    long srcBase;
    long n;
    long x;
    long y;
    long xEnd;
    long yEnd;
    long xStart;
    long cnt;
    long yStart;
    long xStartAligned;
    long yStartAligned;
    long xpos;
    long yEndAligned;
    long pixel;
    long srcRow2;
    long w;
    long ys;
    int xs;
    float red;
    float green;
    float blue;
    float total;
    float rowRed;
    float rowGreen;
    float rowBlue;
    float colWeight;
    float rowWeight;
    float rowTotal;
    unsigned short *rowP;
    unsigned short *rowP2;
    long xEndAligned;
    long dst;
    long y2;

    n = dstRect->left;
    width = dstRect->right - n;
    y = dstRect->top;
    height = dstRect->bottom - y;

    srcRow = srcBits->rowBytes;
    srcBase = srcRect->left - srcBits->bounds.left;
    srcBase *= 2;
    srcBase += (srcRect->top - srcBits->bounds.top) * srcRow
        + (long)srcBits->baseAddr;

    srcRow2 = srcRow;
    rowBytes = dstBits->rowBytes;
    dstP = n - dstBits->bounds.left;
    dstP *= 2;
    y -= dstBits->bounds.top;
    y *= rowBytes;
    y += (long)dstBits->baseAddr;
    dstP += y;
    dstRow = rowBytes - width * 2;

    if (width == 0)
        goto done;
    if (height == 0)
        goto done;

    if (xStep > 0x10000)
        goto scaled;

nearest:
        {
            do {
                rowP = (unsigned short *)((yFrac >> 16) * srcRow + srcBase);
                cnt = width;
                xpos = xFrac;
                dst = dstP;
                do {
                    *(unsigned short *)dst = rowP[xpos >> 16];
                    dst += 2;
                    xpos += xStep;
                    cnt--;
                } while (cnt != 0);
                dst += dstRow;
                dstP = dst;
                yFrac += yStep;
                height--;
            } while (height != 0);
            goto done;
        }

scaled:
    if (xStep <= 0x10000)
        goto nearest;

        {
            w = width;
            ys = yStep;
            xs = xStep;
            do {
                dst = dstP;
                n = w;
                yEnd = dstRect->bottom - dstRect->top;
                yEnd -= height;
                yEnd += 1;
                yEnd *= ys;
                yStart = yEnd - yStep;
                yEndAligned = yEnd & 0xffff0000;
                yStartAligned = yStart & 0xffff0000;
                do {
                    xEnd = width;
                    xEnd -= n;
                    xEnd += 1;
                    xEnd *= xs;
                    xStart = xEnd - xStep;
                    xEndAligned = xEnd & 0xffff0000;
                    xStartAligned = xStart & 0xffff0000;
                    red = green = blue = total = 0.0f;
                    rowWeight = 1.0
                        - (yStart - yStartAligned) / 65536.0f;
                    for (y2 = yStartAligned; y2 <= yEndAligned; y2 += 0x10000) {
                        if (y2 == yEndAligned) {
                            if (yEnd == yEndAligned)
                                break;
                            rowWeight = (yEnd - yEndAligned) / 65536.0f;
                        }
                        rowP2 = (unsigned short *)((y2 >> 16) * srcRow2
                            + srcBase);
                        rowRed = rowGreen = rowBlue = rowTotal = 0.0f;
                        colWeight = 1.0 - (xStart - xStartAligned) / 65536.0f;
                        for (x = xStartAligned; x <= xEndAligned;
                                x += 0x10000) {
                            if (x == xEndAligned) {
                                if (xEnd == xEndAligned)
                                    break;
                                colWeight = (xEnd - xEndAligned) / 65536.0f;
                            }
                            pixel = rowP2[x >> 16];
                            rowRed += ((pixel >> 10) & 0x1f)
                                * colWeight;
                            rowGreen += ((pixel >> 5) & 0x1f)
                                * colWeight;
                            rowBlue += (pixel & 0x1f) * colWeight;
                            rowTotal += colWeight;
                            colWeight = 1.0f;
                        }
                        red += rowWeight * rowRed;
                        green += rowWeight * rowGreen;
                        blue += rowWeight * rowBlue;
                        total += rowWeight * rowTotal;
                        rowWeight = 1.0f;
                    }
                    red /= total;
                    green /= total;
                    blue /= total;
                    *(unsigned short *)dst =
                        (((long)red << 10) | ((long)green << 5)
                        | (long)blue);
                    dst += 2;
                    n--;
                } while (n != 0);
                dstP = dst + dstRow;
                height--;
            } while (height != 0);
        }

done:
    ;
}

#pragma opt_repositioncode reset

/* CopyBitsScale24. STATE: MATCH 88/88 (2026-09-25). OPEN: none */
// FUNCTION: EVNOVA 0x004b81e0
void CopyBitsScale24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                   long xStep, long yStep, long xFrac, long yFrac)
{
    long width;
    long height;
    long srcRow;
    long dstRow;
    unsigned char *srcBase;
    unsigned short *dstP;
    unsigned long pixel;
    long n;
    unsigned char *rowP;
    long x;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;

    srcRow = src->rowBytes;
    srcBase = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * srcRow
        + (srcRect->left - src->bounds.left) * 3;

    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;

    if (width != 0 && height != 0) {
        do {
            n = width;
            rowP = srcBase + (yFrac >> 16) * srcRow;
            x = xFrac;
            do {
                pixel = *(unsigned long *)(rowP + (x >> 16) * 3);
                *dstP = ((pixel << 7) & kRed555) + ((pixel >> 6) & kGreen555)
                      + ((pixel >> 0x13) & kBlue555);
                dstP++;
                x += xStep;
                n--;
            } while (n != 0);
            yFrac += yStep;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

#define kRedBlue555 0x7c1f

/* CopyBitsAddOver16. STATE: MATCH, exe 78 / obj 78, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8300
void CopyBitsAddOver16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    long width;
    long height;
    long srcRow;
    long dstRow;
    unsigned short *srcP;
    unsigned short *dstP;
    long n;
    long srcPixel;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned short *)((unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 2);
    srcRow = src->rowBytes;
    srcRow -= width * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                srcPixel = *srcP;
                *dstP = (((srcPixel & kRedBlue555) + (*dstP & kRedBlue555)) & kRedBlue555)
                      + (((srcPixel & kGreen555) + (*dstP & kGreen555)) & kGreen555);
                srcP++;
                dstP++;
                n--;
            } while (n != 0);
            srcP = (unsigned short *)((char *)srcP + srcRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

/* CopyBitsAdMax16. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8400
void CopyBitsAdMax16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    long width;
    long height;
    long srcRow;
    long dstRow;
    unsigned short *srcP;
    unsigned short *dstP;
    long n;
    unsigned long d;
    unsigned long dr;
    unsigned long dg;
    unsigned long db;
    unsigned long g;
    unsigned long r;
    unsigned long b;
    unsigned long s;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned short *)((unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 2);
    srcRow = src->rowBytes - width * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                d = *dstP;
                s = *srcP;
                g = s & kGreen555;
                dg = d & kGreen555;
                r = s & kRed555;
                dr = d & kRed555;
                b = s & kBlue555;
                db = d & kBlue555;
                if (g <= dg)
                    g = dg;
                if (r <= dr)
                    r = dr;
                if (b <= db)
                    b = db;
                *dstP = r + g + b;
                srcP++;
                dstP++;
                n--;
            } while (n != 0);
            srcP = (unsigned short *)((char *)srcP + srcRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

/* CopyBitsNone. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8520
void CopyBitsNone(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
}

typedef void (*BlitProc)(PixMapPtr srcBits, PixMapPtr dstBits,
                          Rect *srcRect, Rect *dstRect,
                          long xStep, long yStep, long xFrac, long yFrac);

#define BP (BlitProc)

BlitProc copyBitsProcs[5][5] = {             /* 0x00575b28 same-size srcCopy */
    { NULL, NULL,             NULL,                NULL,             NULL             },
    { NULL, BP CopyBitsNone, BP CopyBitsCopy8,    BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsCopy16,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsCopy24,   BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone }
};

BlitProc scaleBitsCopyProcs[5][5] = {        /* 0x00575b8c scaled srcCopy */
    { NULL, NULL,             NULL,                NULL,             NULL             },
    { NULL, BP CopyBitsNone, BP CopyBitsScale8,   BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsScale16Average,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsScale24,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone }
};

BlitProc copyBitsAddOverProcs[5][5] = {      /* 0x00575bf0 same-size addOver */
    { NULL, NULL,             NULL,                NULL,             NULL             },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsAddOver16,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone }
};

BlitProc copyBitsAdMaxProcs[5][5] = {        /* 0x00575c54 same-size adMax */
    { NULL, NULL,             NULL,                NULL,             NULL             },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsAdMax16,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone },
    { NULL, BP CopyBitsNone, BP CopyBitsNone,     BP CopyBitsNone, BP CopyBitsNone }
};
#define transfer_srcCopy 0
#define transfer_srcOr   1
#define transfer_addOver 34
#define transfer_adMax   37

/* CopyBits (mac _CopyBits). STATE: MATCH 100.0%, exe 400 / obj 400 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004b8530
void CopyBits(PixMapPtr srcBits, BitMapPtr dstBits, const Rect *srcRect,
              const Rect *dstRect, long mode, void *maskRgn)
{
    Rect dst;
    Rect src;
    long srcWidth;
    long xStep;
    long yStep;
    long xFrac;
    long yFrac;
    PixMapPtr dstPix;
    BlitProc proc;
    long scaled;

    xStep = 0;
    yStep = 0;
    if (srcBits == NULL || dstBits == NULL || srcRect == NULL || dstRect == NULL)
        return;

    src.top = srcRect->top;
    src.left = srcRect->left;
    src.bottom = srcRect->bottom;
    src.right = srcRect->right;
    dst.top = dstRect->top;
    dst.left = dstRect->left;
    dst.bottom = dstRect->bottom;
    dst.right = dstRect->right;

    dstPix = (PixMapPtr)dstBits;

    scaled = 1;
    srcWidth = srcRect->right - srcRect->left;
    if (dstRect->right - dstRect->left - srcWidth == 0
            && dstRect->bottom - dstRect->top == srcRect->bottom - srcRect->top)
        scaled = 0;
    if ((char)scaled) {
        switch (mode) {
        case transfer_srcCopy:
            proc = scaleBitsCopyProcs[srcBits->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        }
        xStep = srcWidth;
        xStep <<= 16;
        xStep /= dstRect->right - dstRect->left;
        yStep = srcRect->bottom - srcRect->top;
        yStep <<= 16;
        yStep /= dstRect->bottom - dstRect->top;
    } else {
        switch (mode) {
        case transfer_srcCopy:
            proc = copyBitsProcs[srcBits->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        case transfer_srcOr:
            proc = BP CopyBitsOr8;
            break;
        case transfer_addOver:
            proc = copyBitsAddOverProcs[srcBits->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        case transfer_adMax:
            proc = copyBitsAdMaxProcs[srcBits->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        }
    }

    if (src.top < srcBits->bounds.top) {
        dst.top += (((dstRect->bottom - dstRect->top) * (srcBits->bounds.top - src.top)) / (srcRect->bottom - srcRect->top));
        src.top = srcBits->bounds.top;
    }
    if (src.left < srcBits->bounds.left) {
        dst.left += (((dstRect->right - dstRect->left) * (srcBits->bounds.left - src.left)) / (srcRect->right - srcRect->left));
        src.left = srcBits->bounds.left;
    }
    if (src.bottom > srcBits->bounds.bottom) {
        dst.bottom -= (((dstRect->bottom - dstRect->top) * (src.bottom - srcBits->bounds.bottom)) / (srcRect->bottom - srcRect->top));
        src.bottom = srcBits->bounds.bottom;
    }
    if (src.right > srcBits->bounds.right) {
        dst.right -= (((dstRect->right - dstRect->left) * (src.right - srcBits->bounds.right)) / (srcRect->right - srcRect->left));
        src.right = srcBits->bounds.right;
    }

    if (dst.top < dstPix->bounds.top) {
        src.top += (((srcRect->bottom - srcRect->top) * (dstPix->bounds.top - dst.top)) / (dstRect->bottom - dstRect->top));
        dst.top = dstPix->bounds.top;
    }
    if (dst.left < dstPix->bounds.left) {
        src.left += (((srcRect->right - srcRect->left) * (dstPix->bounds.left - dst.left)) / (dstRect->right - dstRect->left));
        dst.left = dstPix->bounds.left;
    }
    if (dst.bottom > dstPix->bounds.bottom) {
        src.bottom -= (((srcRect->bottom - srcRect->top) * (dst.bottom - dstPix->bounds.bottom)) / (dstRect->bottom - dstRect->top));
        dst.bottom = dstPix->bounds.bottom;
    }
    if (dst.right > dstPix->bounds.right) {
        src.right -= (((srcRect->right - srcRect->left) * (dst.right - dstPix->bounds.right)) / (dstRect->right - dstRect->left));
        dst.right = dstPix->bounds.right;
    }

    xFrac = xStep >> 1;
    yFrac = yStep >> 1;

    if (dst.left >= dst.right || dst.top >= dst.bottom)
        return;
    if (src.left < srcBits->bounds.left || src.top < srcBits->bounds.top
            || src.right > srcBits->bounds.right
            || src.bottom > srcBits->bounds.bottom)
        return;

    proc(srcBits, dstPix, &src, &dst, xStep, yStep, xFrac, yFrac);
}

/* GetPortBitMapForCopyBits (mac _GetPortBitMapForCopyBits). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8a70
BitMapPtr GetPortBitMapForCopyBits(CGrafPtr port)
{
    BitMapPtr bits;

    if (port == NULL)
        bits = NULL;
    else
        bits = (BitMapPtr)&port->pix;
    return bits;
}

/* SetPort (mac _SetPort). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8a90
void SetPort(CGrafPtr port)
{
    CurrentGrafPort = port;
}

/* GetPort (mac _GetPort). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8aa0
void GetPort(CGrafPtr *port)
{
    *port = CurrentGrafPort;
}

/* ShowMacWindow (mac _ShowWindow; Win32 owns that name). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8ab0
void ShowMacWindow(void *arg)
{
}

/* HideWindow (mac _HideWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8ac0
void HideWindow(CWindowPtr window)
{
}

/* DisposeWindow (mac _DisposeWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8ad0
void DisposeWindow(CWindowPtr window)
{
}

/* the blitter object's globals */
CGrafPort gGamePort;
RGBColor blackRGB;
char gParamText0[256];
char gParamText1[256];
char gParamText2[256];
char gParamText3[256];
unsigned char gGammaRamp[32];

/* OpenGamePort. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8ae0
CGrafPtr OpenGamePort(Rect *bounds, const unsigned char *title)
{
    memset(&gGamePort, 0, sizeof(gGamePort));
    if (Win95MainWindow != NULL) {
        MoveWindow(Win95MainWindow, bounds->left, bounds->top,
                   bounds->right - bounds->left, bounds->bottom - bounds->top, 1);
        UpdateWindow(Win95MainWindow);
        SetFocus(Win95MainWindow);
    }
    LockVideoPage();
    gGamePort.pnSize.v = 1;
    gGamePort.pnSize.h = gGamePort.pnSize.v;
    gGamePort.pix.pixelSize = (VideoColorDepth + 7) & ~7;
    gGamePort.pix.bounds.top = bounds->top;
    gGamePort.pix.bounds.left = bounds->left;
    gGamePort.pix.bounds.bottom = bounds->bottom;
    gGamePort.pix.bounds.right = bounds->right;
    gGamePort.pix.baseAddr = videoBase;
    gGamePort.pix.width = gVideoWidth;
    gGamePort.pix.height = gVideoHeight;
    gGamePort.pix.rowBytes = VideoWidth;
    gGamePort.depth = VideoColorDepth;
    gGamePort.hdc = (Win95MainWindow != NULL) ? GetDC(Win95MainWindow) : 0;
    CurrentGrafPort = &gGamePort;
    return &gGamePort;
}

/*
 * Taken when the video mode is (re)established and given back when it is torn
 * down. The mode change also re-caches the video layer's page pointers, and
 * tells SpriteWorld where the back buffer went if a world already exists.
 */

/* ReleaseScreenDC. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8c20
void ReleaseScreenDC(void)
{
    void *dc;

    dc = gGamePort.hdc;
    if (dc != NULL)
        ReleaseDC(Win95MainWindow, dc);
}

/* AcquireScreenDC. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8c40
void AcquireScreenDC(void)
{
    void *dc;

    LockVideoPage();
    gGamePort.pix.baseAddr = videoBase;
    gGamePort.pix.rowBytes = VideoWidth;
    if (Win95MainWindow != NULL)
        dc = GetDC(Win95MainWindow);
    else
        dc = NULL;
    gGamePort.hdc = dc;
    if (gSpriteWorld != NULL)
        SWWindowMoved(gSpriteWorld);
}

/* Two sources in order: an EVNova.ini override keyed by list and index, then the STR#
   resource. `entry' is seeded from `base' before the count is read (`entry = base + 2'
   coalesces the pair). */


/* same shape as BurgerLib's SwapEndian, which is a C++ inline a C translation
   unit cannot reach */
static __inline short SwapEndianShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

/* GetIndString (mac _GetIndString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8ca0
void GetIndString(unsigned char *str, int strListID, int index)
{
    Handle h;
    unsigned char *entry;
    unsigned short count;
    unsigned char *base;

    str[0] = 0;
    GetIniIndString((char *)str, strListID, index);
    if (str[0] != 0) {
        CtoPstr((char *)str);
        return;
    }
    h = GetResource(kStringListType, strListID);
    if (h == NULL)
        return;
    LockAHandle(h);
    base = (unsigned char *)*h;
    entry = base;
    count = SwapEndianShort(&((StringListRec *)base)->count);
    entry += 2;
    if (index != 0 && index <= count) {
        while (--index != 0)
            entry += *entry + 1;
        memcpy(str, entry, *entry + 1);
    }
    UnlockAHandle(h);
    ReleaseResource(h);
}

/* OffsetRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8d50
void OffsetRect(Rect *r, int dh, int dv)
{
    if (r == NULL)
        return;
    r->left += dh;
    r->right += dh;
    r->top += dv;
    r->bottom += dv;
}

/* SetRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8d80
void SetRect(Rect *r, int left, int top, int right, int bottom)
{
    if (r == NULL)
        return;
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
}

/* InsetRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8dc0
void InsetRect(Rect *r, int dh, int dv)
{
    if (r == NULL)
        return;
    r->left += dh;
    r->right -= dh;
    r->top += dv;
    r->bottom -= dv;
}

/* SectRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8df0
int SectRect(const Rect *a, const Rect *b, Rect *dst)
{
    int meet;
    int edge;
    int other;
    Rect scratch;

    if (dst == NULL)
        dst = &scratch;

    edge = a->left;
    other = b->left;
    if (edge < other)
        edge = other;
    dst->left = edge;

    edge = a->right;
    other = b->right;
    if (edge >= other)
        edge = other;
    dst->right = edge;

    edge = a->top;
    other = b->top;
    if (edge < other)
        edge = other;
    dst->top = edge;

    edge = a->bottom;
    other = b->bottom;
    if (edge >= other)
        edge = other;
    dst->bottom = edge;

    meet = 1;
    if (dst->left >= dst->right || dst->top >= dst->bottom) {
        meet = 0;
        dst->left = 0;
        dst->right = 0;
        dst->top = 0;
        dst->bottom = 0;
    }
    return meet;
}

/* PtInRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b8e90
int PtInRect(int h, int v, Rect *r)
{
    int inside;

    inside = 0;
    if (h < r->right && h >= r->left && v < r->bottom && v >= r->top)
        inside = 1;
    return inside;
}

/* LoadPictResource. STATE: MATCH 100.0%, exe 137 / obj 137, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x004b8ed0
PicHandle LoadPictResource(int picID, char gamma)
{
    char **h;
    PicHandle img;
    long err;
    long advance;
    unsigned char *p;
    unsigned short pix;
    long bytes;
    long pixels;
    long rows;

    h = GetResource(kPictType, picID);
    if (h != NULL) {
        img = (Image_t *)AllocAPointer(sizeof(Image_t));
        LockAHandle(h);
        err = ImageParsePict(img, (unsigned char *)*h);
        UnlockAHandle(h);
        ReleaseResource(h);
        if (err != 0) {
            DeallocAPointer(img);
            return NULL;
        }
        if (gamma != 0 && img->Width != 0 && img->Height != 0) {
            switch ((img->DataType + 7) & 0xfffffff8) {
            case 8:
                p = img->PalettePtr;
                if (p != NULL) {
                    bytes = 256 * 3;
                    do {
                        *p = (gGammaRamp[(*p >> 3) & 0x1f] << 3);
                        p++;
                    } while (--bytes != 0);
                }
                break;
            case 16:
                p = img->ImagePtr;
                advance = img->RowBytes - img->Width * 2;
                rows = img->Height;
                do {
                    pixels = img->Width;
                    do {
                        pix = *(unsigned short *)p;
                        pix = ((gGammaRamp[(pix >> 10) & 0x1f] << 10) +
                                               (gGammaRamp[(pix >> 5) & 0x1f] << 5) +
                                               gGammaRamp[pix & 0x1f]);
                        *(unsigned short *)p = pix;
                        p += 2;
                    } while (--pixels != 0);
                    p += advance;
                } while (--rows != 0);
                break;
            case 24:
                p = img->ImagePtr;
                advance = img->RowBytes - img->Width * 3;
                rows = img->Height;
                do {
                    bytes = img->Width * 3;
                    do {
                        *p = (gGammaRamp[(*p >> 3) & 0x1f] << 3);
                        p++;
                    } while (--bytes != 0);
                    p += advance;
                } while (--rows != 0);
                break;
            }
        }
        return img;
    }
    return NULL;
}


/* GetPicture. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9050
PicHandle GetPicture(int picID)
{
    return LoadPictResource(picID, 1);
}

/*
 * The same one-line wrapper at the next address, passing 0 where GetPicture
 * passes 1. Ghidra's decompilation prints both immediates as 0; the exe bytes
 * say otherwise, which is why this pair is worth the note.
 */
/* GetPictureNoGamma. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9060
PicHandle GetPictureNoGamma(int picID)
{
    return LoadPictResource(picID, 0);
}

/*
 * Blits a decoded picture straight into the current port. Name invented - the
 * mac has no twin, since on that side the caller would have had a real PixMap
 * already.
 */
/* DrawPicHandle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9070
void DrawPicHandle(PicHandle pic, Rect *dstRect)
{
    SrcBitMap src;

    if (pic == NULL || dstRect == NULL)
        return;
    if (CurrentGrafPort == NULL)
        return;

    SetRect(&src.bounds, 0, 0, pic->Width, pic->Height);
    src.rowBytes = (pic->DataType + 7) & ~7;
    src.image.ImagePtr = pic->ImagePtr;
    src.image.PalettePtr = pic->PalettePtr;
    src.image.AlphaPtr = pic->AlphaPtr;
    src.image.Width = pic->Width;
    src.image.Height = pic->Height;
    src.image.RowBytes = pic->RowBytes;
    src.image.DataType = pic->DataType;
    CopyBits((PixMapPtr)&src, GetPortBitMapForCopyBits(CurrentGrafPort),
             &src.bounds, dstRect, 0, NULL);
}

/*
 * Named apart from the resource manager's ReleaseResource at 0x004ce370,
 * which is a different function; the other TUs still declare this one under
 * the shared name and want reconciling.
 */
/* QDReleaseResource. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9130
void QDReleaseResource(PicHandle pic)
{
    ImageDelete(pic);
}

/* QDGetPictureBounds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9140
void QDGetPictureBounds(PicHandle pic, Rect *r)
{
    SetRect(r, 0, 0, pic->Width, pic->Height);
}

/* two more of the port's compiled-away QuickDraw calls */
/* GetPortVisRgn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9160
void *GetPortVisRgn(void *port)
{
    return NULL;
}

/* GetCWindowVisRgn (mac _GetCWindowVisRgn). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9170
void *GetCWindowVisRgn(void *window)
{
    return 0;
}

/* Classic Toolbox parameter order (srcBits, maskBits, dstBits, srcRect, maskRect,
   dstRect) with a 16.16 scale accumulator appended; reached only from the
   mode-indexed table at 0x00575d20, so there is no mac twin. */

/* CopyMaskScale16. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9180
void CopyMaskScale16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
                   Rect *srcRect, Rect *maskRect, Rect *dstRect,
                   long xStep, long yStep, long xFrac, long yFrac)
{
    long width;
    long height;
    long srcRow;
    long maskRow;
    long dstRow;
    unsigned char *srcBase;
    unsigned char *maskBase;
    long n;
    unsigned short *srcRowP;
    unsigned short *maskRowP;
    unsigned short *dstP;
    long x;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;

    srcRow = srcBits->rowBytes;
    srcBase = (unsigned char *)srcBits->baseAddr
        + (srcRect->top - srcBits->bounds.top) * srcRow
        + (srcRect->left - srcBits->bounds.left) * 2;

    maskRow = maskBits->rowBytes;
    maskBase = (unsigned char *)maskBits->baseAddr
        + (maskRect->top - maskBits->bounds.top) * maskRow
        + (maskRect->left - maskBits->bounds.left) * 2;

    dstP = (unsigned short *)((unsigned char *)dstBits->baseAddr
        + (dstRect->top - dstBits->bounds.top) * dstBits->rowBytes
        + (dstRect->left - dstBits->bounds.left) * 2);
    dstRow = dstBits->rowBytes - width * 2;

    if (width != 0 && height != 0) {
        do {
            srcRowP = (unsigned short *)(srcBase + (yFrac >> 16) * srcRow);
            maskRowP = (unsigned short *)(maskBase + (yFrac >> 16) * maskRow);
            n = width;
            x = xFrac;
            do {
                if (maskRowP[x >> 16] == 0)
                    *dstP = srcRowP[x >> 16];
                dstP++;
                x += xStep;
                n--;
            } while (n != 0);
            yFrac += yStep;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}


/* CopyMask16. STATE: MATCH, exe 95 / obj 95 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004b92d0
void CopyMask16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
                Rect *srcRect, Rect *maskRect, Rect *dstRect)
{
    long srcRow;
    long dstLeft;
    long dstTop;
    long width;
    long height;
    long maskRow;
    long dstRow;
    unsigned short *srcP;
    unsigned short *maskP;
    unsigned short *dstP;
    long n;

    dstLeft = dstRect->left;
    width = dstRect->right - dstLeft;
    dstTop = dstRect->top;
    height = dstRect->bottom - dstTop;

    srcP = (unsigned short *)((unsigned char *)srcBits->baseAddr
        + (srcRect->top - srcBits->bounds.top) * srcBits->rowBytes
        + (srcRect->left - srcBits->bounds.left) * 2);
    srcRow = srcBits->rowBytes - width * 2;

    maskP = (unsigned short *)((unsigned char *)maskBits->baseAddr
        + (maskRect->top - maskBits->bounds.top) * maskBits->rowBytes
        + (maskRect->left - maskBits->bounds.left) * 2);
    maskRow = maskBits->rowBytes - width * 2;

    dstP = (unsigned short *)((unsigned char *)dstBits->baseAddr
        + (dstTop - dstBits->bounds.top) * dstBits->rowBytes
        + (dstLeft - dstBits->bounds.left) * 2);
    dstRow = dstBits->rowBytes - width * 2;

    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                if (*maskP == 0)
                    *dstP = *srcP;
                srcP++;
                maskP++;
                dstP++;
            } while (--n != 0);
            srcP = (unsigned short *)((char *)srcP + srcRow);
            maskP = (unsigned short *)((char *)maskP + maskRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

/* CopyMaskNone. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9400
void CopyMaskNone(PixMapPtr src, PixMapPtr mask, PixMapPtr dst,
                  Rect *srcRect, Rect *maskRect, Rect *dstRect)
{
}

typedef void (*BlitMaskProc)(PixMapPtr srcBits, PixMapPtr maskBits,
                             PixMapPtr dstBits, Rect *srcRect, Rect *maskRect,
                             Rect *dstRect, long xStep, long yStep,
                             long xFrac, long yFrac);

BlitMaskProc copyMaskProcs[5][5] = {    /* 0x00575cb8 */
    { NULL, NULL,                      NULL,                      NULL,                      NULL                      },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMask16,   (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone }
};

BlitMaskProc scaleMaskProcs[5][5] = {   /* 0x00575d1c */
    { NULL, NULL,                      NULL,                      NULL,                      NULL                      },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone },
    { NULL, (BlitMaskProc)CopyMaskNone, CopyMaskScale16, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone },
    { NULL, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone, (BlitMaskProc)CopyMaskNone }
};

/* CopyMask. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9410
void CopyMask(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
              Rect *srcRect, Rect *maskRect, Rect *dstRect)
{
    Rect mask;
    Rect src;
    Rect dst;
    long srcWidth;
    long dstWidth;
    long xStep;
    long yStep;
    long yFrac;
    long xFrac;
    long delta;
    BlitMaskProc proc;

    xStep = 0;
    yStep = 0;
    xFrac = 0;
    yFrac = 0;
    if (srcBits == NULL || maskBits == NULL || dstBits == NULL
            || srcRect == NULL || maskRect == NULL || dstRect == NULL)
        return;

    src.top = srcRect->top;
    src.left = srcRect->left;
    src.bottom = srcRect->bottom;
    src.right = srcRect->right;
    mask.top = maskRect->top;
    mask.left = maskRect->left;
    mask.bottom = maskRect->bottom;
    mask.right = maskRect->right;
    dst.top = dstRect->top;
    dst.left = dstRect->left;
    dst.bottom = dstRect->bottom;
    dst.right = dstRect->right;

    srcWidth = srcRect->right - srcRect->left;
    dstWidth = dstRect->right - dstRect->left;
    if (dstWidth == srcWidth
            && dstRect->bottom - dstRect->top
                   == srcRect->bottom - srcRect->top) {
        proc = copyMaskProcs[srcBits->pixelSize / 8][dstBits->pixelSize / 8];
    } else {
        proc = scaleMaskProcs[srcBits->pixelSize / 8][dstBits->pixelSize / 8];
        xStep = srcWidth;
        xStep <<= 16;
        xStep /= dstWidth;
        yStep = srcRect->bottom - srcRect->top;
        yStep <<= 16;
        yStep /= dstRect->bottom - dstRect->top;
    }

    if (dst.left < dstBits->bounds.left) {
        delta = dstBits->bounds.left - dst.left;
        src.left += delta;
        mask.left += delta;
        dst.left += delta;
        xFrac = xStep * delta;
    }
    if (dst.top < dstBits->bounds.top) {
        delta = dstBits->bounds.top - dst.top;
        src.top += delta;
        mask.top += delta;
        dst.top += delta;
        /* BUG: the top clip scales by xStep, not yStep */
        yFrac = xStep * delta;
    }
    if (dst.right > dstBits->bounds.right)
        dst.right -= dst.right - dstBits->bounds.right;
    if (dst.bottom > dstBits->bounds.bottom)
        dst.bottom -= dst.bottom - dstBits->bounds.bottom;

    if (dst.left >= dst.right || dst.top >= dst.bottom)
        return;
    if (src.left < srcBits->bounds.left || src.top < srcBits->bounds.top
            || src.right > srcBits->bounds.right
            || src.bottom > srcBits->bounds.bottom)
        return;
    if (mask.left < maskBits->bounds.left || mask.top < maskBits->bounds.top
            || mask.right > maskBits->bounds.right
            || mask.bottom > maskBits->bounds.bottom)
        return;

    proc(srcBits, maskBits, dstBits, &src, &mask, &dst,
         xStep, yStep, xFrac, yFrac);
}

/* The pen position and size live in the current port, so these are field stores with
   the Toolbox's null guard; the relative Line is spelled in terms of the absolute
   LineTo exactly as the Toolbox defines it. */

/* Line. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b9790
void Line(short dh, short dv)
{
    if (CurrentGrafPort == NULL)
        return;
    LineTo((CurrentGrafPort->pnLoc.h + dh), (CurrentGrafPort->pnLoc.v + dv));
}


/* LineTo. STATE: MATCH, exe 811 / obj 811, byte-identical yes (2822 bytes); `long clipTop'
   and the one walk counter `count' are the frame's two facts, notes/qdpen.md. The swaps save
   x0/y0 in `i'.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004b97e0
void LineTo(short h, short v)
{
    char *q;
    int x0;
    char wide;
    int x1;
    long clipTop;
    int clipBottom;
    int y1;
    unsigned int n;
    short boundsTop;
    long color32;
    int yStep;
    int rowAdvance;
    int penH;
    unsigned int penW;
    int y0;
    int xStep;
    unsigned int dx;
    int rows;
    unsigned int err;
    unsigned int count;
    int skipped;
    int clipRight;
    int clipLeft;
    unsigned int nRows;
    char *p;
    unsigned int dy;
    int i;

    x1 = h;
    y1 = v;
    if (CurrentGrafPort == NULL)
        return;
    x0 = CurrentGrafPort->pnLoc.h;
    y0 = CurrentGrafPort->pnLoc.v;
    if (x0 > x1)
        x0--;
    else if (x0 != x1)
        x1--;
    if (y0 > y1)
        y0--;
    else if (y0 != y1)
        y1--;

    color32 = gGammaRamp[CurrentGrafPort->foreColor.blue >> 11] |
              (gGammaRamp[CurrentGrafPort->foreColor.red >> 11] << 10 |
               gGammaRamp[CurrentGrafPort->foreColor.green >> 11] << 5);
    color32 |= color32 << 16;

    penW = CurrentGrafPort->pnSize.h;
    penH = CurrentGrafPort->pnSize.v;
    if (y0 > y1) {
        i = x0;
        x0 = x1;
        x1 = i;
        i = y0;
        y0 = y1;
        y1 = i;
    }
    boundsTop = CurrentGrafPort->pix.bounds.top;
    clipTop = boundsTop;
    clipLeft = CurrentGrafPort->pix.bounds.left;
    clipBottom = CurrentGrafPort->pix.bounds.bottom;
    clipRight = CurrentGrafPort->pix.bounds.right;
    wide = penW > 1;
    if (wide) {
        clipTop += penH - 1;
        clipBottom -= penH - 1;
        clipLeft += penW - 1;
        clipRight -= penW - 1;
    }
    /* BUG: the clip rejections return without moving the pen to (h, v) (exe 0x004b9e56,
       0x004b9e88, 0x004b9e90) */
    if (y1 < clipTop || y0 >= clipBottom)
        return;

    dx = x1 - x0;
    if ((int)dx < 0) {
        dx = -dx;
        xStep = -2;
        if (x0 < clipLeft || x1 >= clipRight)
            return;
    } else {
        if (x1 < clipLeft || x0 >= clipRight)
            return;
        xStep = 2;
    }
    yStep = CurrentGrafPort->pix.rowBytes;
    dy = y1 - y0;
    p = (char *)CurrentGrafPort->pix.baseAddr +
        (y0 - boundsTop) * CurrentGrafPort->pix.rowBytes +
        (x0 - CurrentGrafPort->pix.bounds.left) * 2;
    if (wide)
        rowAdvance = CurrentGrafPort->pix.rowBytes - penW * 2;

    if (dx >= dy) {
        if (dx == 0) {
            if (wide) {
                rows = penH;
                do {
                    n = penW;
                    if (n > 1) {
                        do {
                            *(unsigned long *)p = color32;
                            p += 4;
                            n -= 2;
                        } while (n > 1);
                    }
                    if (n != 0) {
                        *(unsigned short *)p = color32;
                        p += 2;
                    }
                    p += rowAdvance;
                    rows--;
                } while (rows != 0);
            } else {
                *(unsigned short *)p = color32;
            }
            return;
        }
        err = dx >> 1;
        count = dx;
        skipped = 0;
        if (xStep <= 0) {
            if (x0 >= clipRight) {
                i = (x0 - clipRight) + 1;
                err += i * dy;
                p -= i * 2;
                count -= i;
                nRows = err / dx;
                err = err % dx;
                p = (char *)((long)p + yStep * nRows);
                y0 += nRows;
            }
            if (x1 < clipLeft) {
                skipped = clipLeft - x1;
                count -= skipped;
            }
        } else {
            if (x0 < clipLeft) {
                i = clipLeft - x0;
                err += i * dy;
                p += i * 2;
                count -= i;
                nRows = err / dx;
                err = err % dx;
                p = (char *)((long)p + yStep * nRows);
                y0 += nRows;
            }
            if (x1 >= clipRight) {
                skipped = (x1 - clipRight) + 1;
                count -= skipped;
            }
        }
        if (y0 < clipTop) {
            i = clipTop - y0;
            i = (i * dx - err) / dy + 1;
            err += i * dy;
            p = (char *)((long)p + i * xStep);
            count -= i;
            nRows = err / dx;
            err = err % dx;
            p = (char *)((long)p + yStep * nRows);
        }
        if (y1 >= clipBottom) {
            i = (y1 - clipBottom) + 1;
            i = (i * dx - err) / dy + 1;
            if (i > skipped)
                count -= i - skipped;
        }
        if ((int)count >= 0) {
            if (wide) {
                q = p;
                rows = penH;
                do {
                    n = penW;
                    if (n > 1) {
                        do {
                            *(unsigned long *)q = color32;
                            q += 4;
                            n -= 2;
                        } while (n > 1);
                    }
                    if (n != 0) {
                        *(unsigned short *)q = color32;
                        q += 2;
                    }
                    q = (char *)((long)q + rowAdvance);
                    rows--;
                } while (rows != 0);
                if (count != 0) {
                    do {
                        err += dy;
                        if (err >= dx) {
                            err -= dx;
                            p = (char *)((long)p + yStep);
                        }
                        p = (char *)((long)p + xStep);
                        q = p;
                        rows = penH;
                        do {
                            n = penW;
                            if (n > 1) {
                                do {
                                    *(unsigned long *)q = color32;
                                    q += 4;
                                    n -= 2;
                                } while (n > 1);
                            }
                            if (n != 0) {
                                *(unsigned short *)q = color32;
                                q += 2;
                            }
                            q += rowAdvance;
                            rows--;
                        } while (rows != 0);
                    } while (--count != 0);
                }
            } else {
                *(unsigned short *)p = color32;
                if (count != 0) {
                    do {
                        err += dy;
                        if (err >= dx) {
                            err -= dx;
                            p = (char *)((long)p + yStep);
                        }
                        p = (char *)((long)p + xStep);
                        *(unsigned short *)p = color32;
                    } while (--count != 0);
                }
            }
        }
        /* BUG: the x-major walk returns without moving the pen to (h, v) (exe 0x004ba0f1 and
           0x004b9ec6 for dx == 0; the one pnLoc store is 0x004ba2c0) */
        return;
    }

    err = dy >> 1;
    count = dy;
    if (xStep <= 0) {
        if (x0 >= clipRight) {
            rows = (x0 - clipRight) + 1;
            rows = (rows * dy - err) / dx + 1;
            y0 += rows;
            err += rows * dx;
            p = (char *)((long)p + rows * yStep);
            count -= rows;
            i = err / dy;
            err = err % dy;
            p = (char *)((long)p + xStep * i);
        }
        if (x1 < clipLeft) {
            i = clipLeft - x1;
            i = (i * dy - err) / dx + 1;
            count -= i;
            y1 -= i;
        }
    } else {
        if (x0 < clipLeft) {
            i = clipLeft - x0;
            i = (i * dy - err) / dx + 1;
            y0 += i;
            err += i * dx;
            p = (char *)((long)p + i * yStep);
            count -= i;
            nRows = err / dy;
            err = err % dy;
            p = (char *)((long)p + xStep * nRows);
        }
        if (x1 >= clipRight) {
            i = (x1 - clipRight) + 1;
            i = (i * dy - err) / dx + 1;
            count -= i;
            y1 -= i;
        }
    }
    if (y0 < clipTop) {
        skipped = clipTop - y0;
        err += skipped * dx;
        p = (char *)((long)p + skipped * yStep);
        count -= skipped;
        i = err / dy;
        err = err % dy;
        p = (char *)((long)p + xStep * i);
    }
    if (y1 >= clipBottom)
        count -= (y1 - clipBottom) + 1;
    if ((int)count >= 0) {
        if (wide) {
            q = p;
            rows = penH;
            do {
                n = penW;
                if (n > 1) {
                    do {
                        *(unsigned long *)q = color32;
                        q += 4;
                        n -= 2;
                    } while (n > 1);
                }
                if (n != 0) {
                    *(unsigned short *)q = color32;
                    q += 2;
                }
                q = (char *)((long)q + rowAdvance);
                rows--;
            } while (rows != 0);
            if (count != 0) {
                do {
                    err += dx;
                    if (err >= dy) {
                        err -= dy;
                        p = (char *)((long)p + xStep);
                    }
                    p = (char *)((long)p + yStep);
                    q = p;
                    rows = penH;
                    do {
                        n = penW;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = color32;
                                q += 4;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0) {
                            *(unsigned short *)q = color32;
                            q += 2;
                        }
                        q = (char *)((long)q + rowAdvance);
                        rows--;
                    } while (rows != 0);
                } while (--count != 0);
            }
        } else {
            *(unsigned short *)p = color32;
            if (count != 0) {
                do {
                    err += dx;
                    if (err >= dy) {
                        err -= dy;
                        p = (char *)((long)p + xStep);
                    }
                    p = (char *)((long)p + yStep);
                    *(unsigned short *)p = color32;
                } while (--count != 0);
            }
        }
    }
    CurrentGrafPort->pnLoc.h = h;
    CurrentGrafPort->pnLoc.v = v;
}

/* MoveTo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ba2f0
void MoveTo(short h, short v)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->pnLoc.h = h;
    CurrentGrafPort->pnLoc.v = v;
}

/* PenSize. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ba320
void PenSize(short width, short height)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->pnSize.h = width;
    CurrentGrafPort->pnSize.v = height;
}

/* FrameOval (mac _FrameOval). STATE: MATCH, exe 1117 / obj 1117, byte-identical yes.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004ba350
void FrameOval(Rect *r)
{
    int left;
    int top;
    int bottom;
    int right;
    int penW;
    int penH;
    char thick;
    int y;
    int d;
    int k;
    int x;
    int cx;
    int rowBytes;
    unsigned long colour;
    int radius;
    int cy;
    unsigned char *base;
    int px;
    int py;
    int bx;
    int by;
    int fw;
    int fh;
    /* the thick pen's eight blocks: the later ones keep their own row pointer and height */
    int fh8;
    int fh6;
    int fh7;
    int n;
    unsigned short *p7;
    unsigned short *p8;
    unsigned short *p3;
    unsigned short *p4;
    unsigned short *p5;
    unsigned short *p6;
    unsigned short *p;
    unsigned short *q;

    if (CurrentGrafPort == NULL)
        return;
    left = CurrentGrafPort->pix.bounds.left;
    top = CurrentGrafPort->pix.bounds.top;
    bottom = CurrentGrafPort->pix.bounds.bottom;
    right = CurrentGrafPort->pix.bounds.right;
    if (r->right < left || r->left > right ||
        r->bottom < top || r->top > bottom)
        return;

    penW = CurrentGrafPort->pnSize.h;
    penH = CurrentGrafPort->pnSize.v;
    thick = (penW != 1 || penH != 1);

    x = 0;
    radius = (r->bottom - r->top) / 2;
    y = radius;
    d = 1 - radius;
    cx = (r->right + r->left) / 2;
    cy = (r->top + r->bottom) / 2;
    rowBytes = CurrentGrafPort->pix.rowBytes;
    base = (unsigned char *)CurrentGrafPort->pix.baseAddr;
    colour = (gGammaRamp[CurrentGrafPort->foreColor.red >> 11] << 10) |
             (gGammaRamp[CurrentGrafPort->foreColor.green >> 11] << 5) |
             gGammaRamp[CurrentGrafPort->foreColor.blue >> 11];
    colour |= colour << 16;

    if (y >= x) {
        k = 3;
        do {
            if (!thick) {
                px = cx + x;
                py = cy + y;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx + x;
                py = cy - y;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx - x;
                py = cy + y;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx - x;
                py = cy - y;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx + y;
                py = cy + x;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx + y;
                py = cy - x;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx - y;
                py = cy + x;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
                px = cx - y;
                py = cy - x;
                if (px >= left && px < right && py >= top && py < bottom)
                    *(unsigned short *)(base + rowBytes * py + px * 2) =
                        colour;
            } else {
                fh = penH;
                fw = penW;
                bx = cx + x;
                by = cy + y;
                if (by < top) {
                    fh -= top - by;
                    by = top;
                }
                if (by + fh >= bottom)
                    fh = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh > 0 && fw > 0) {
                    p = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;    /* BUG: one pixel per two-pixel store, so about half of fw is painted (all eight blocks) */
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;    /* BUG: a byte store, half of the last pixel (all eight blocks) */
                        fh--;
                    } while (fh != 0);    /* BUG: p never advances by rowBytes, so every pass repaints one row (all eight blocks) */
                }

                fh = penH;
                fw = penW;
                bx = cx + x;
                by = cy - y;
                if (by < top) {
                    fh -= top - by;
                    by = top;
                }
                if (by + fh >= bottom)
                    fh = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh > 0 && fw > 0) {
                    p = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh--;
                    } while (fh != 0);
                }

                fh = penH;
                fw = penW;
                bx = cx - x;
                by = cy + y;
                if (by < top) {
                    fh -= top - by;
                    by = top;
                }
                if (by + fh >= bottom)
                    fh = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh > 0 && fw > 0) {
                    p3 = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p3;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh--;
                    } while (fh != 0);
                }

                fh = penH;
                fw = penW;
                bx = cx - x;
                by = cy - y;
                if (by < top) {
                    fh -= top - by;
                    by = top;
                }
                if (by + fh >= bottom)
                    fh = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh > 0 && fw > 0) {
                    p4 = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p4;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh--;
                    } while (fh != 0);
                }

                fh = penH;
                fw = penW;
                bx = cx + y;
                by = cy + x;
                if (by < top) {
                    fh -= top - by;
                    by = top;
                }
                if (by + fh >= bottom)
                    fh = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh > 0 && fw > 0) {
                    p5 = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p5;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh--;
                    } while (fh != 0);
                }

                fh6 = penH;
                fw = penW;
                bx = cx + y;
                by = cy - x;
                if (by < top) {
                    fh6 -= top - by;
                    by = top;
                }
                if (by + fh6 >= bottom)
                    fh6 = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh6 > 0 && fw > 0) {
                    p6 = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p6;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh6--;
                    } while (fh6 != 0);
                }

                fh7 = penH;
                fw = penW;
                bx = cx - y;
                by = cy + x;
                if (by < top) {
                    fh7 -= top - by;
                    by = top;
                }
                if (by + fh7 >= bottom)
                    fh7 = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh7 > 0 && fw > 0) {
                    p7 = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p7;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh7--;
                    } while (fh7 != 0);
                }

                fh8 = penH;
                fw = penW;
                bx = cx - y;
                by = cy - x;
                if (by < top) {
                    fh8 -= top - by;
                    by = top;
                }
                if (by + fh8 >= bottom)
                    fh8 = bottom - by;
                if (bx < left) {
                    fw -= left - bx;
                    bx = left;
                }
                if (bx + fw >= right)
                    fw = right - bx;
                if (fh8 > 0 && fw > 0) {
                    p8 = (unsigned short *)(base + rowBytes * by + bx * 2);
                    do {
                        q = p8;
                        n = fw;
                        if (n > 1) {
                            do {
                                *(unsigned long *)q = colour;
                                q++;
                                n -= 2;
                            } while (n > 1);
                        }
                        if (n != 0)
                            *(unsigned char *)q = colour;
                        fh8--;
                    } while (fh8 != 0);
                }
            }

            if (d < 0)
                d += k;
            else {
                d += 2 * (x - y) + 5;
                y--;
            }
            k += 2;
            x++;
        } while (y >= x);
    }
}

/* PaintOval (mac _PaintOval). STATE: MATCH, exe 447 / obj 447, byte-identical yes.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004bb260
void PaintOval(Rect *r)
{
    long left;
    long top;
    long k;
    long d;
    long right;
    unsigned long colour;
    long x;
    long y;
    long bottom;
    unsigned char *base;
    long cy;
    long bot;
    long rowBytes;
    long cx;
    long radius;
    long px0;
    long px1;
    long qx2;
    long ybot;
    long qx3;
    long w2;
    long qx0;
    long qx1;
    long x1;
    long w4;
    long x0;
    long ytop;
    long w1;
    long w3;
    unsigned short *q;
    unsigned short *q2;

    if (CurrentGrafPort == NULL)
        return;
    cy = r->top;
    bot = r->bottom;
    radius = (bot - cy) / 2;
    x = 0;
    y = radius;
    d = 1 - radius;
    cx = (r->right + r->left) / 2;
    cy = (cy + bot) / 2;
    left = CurrentGrafPort->pix.bounds.left;
    top = CurrentGrafPort->pix.bounds.top;
    bottom = CurrentGrafPort->pix.bounds.bottom;
    right = CurrentGrafPort->pix.bounds.right;
    rowBytes = CurrentGrafPort->pix.rowBytes;
    base = (unsigned char *)CurrentGrafPort->pix.baseAddr;
    colour = (gGammaRamp[CurrentGrafPort->foreColor.red >> 11] << 10) |
             (gGammaRamp[CurrentGrafPort->foreColor.green >> 11] << 5) |
             gGammaRamp[CurrentGrafPort->foreColor.blue >> 11];
    colour |= colour << 16;

    if (y >= x) {
        k = 3;
        do {
            ybot = cy + y;
            x0 = cx - x;
            x1 = cx + x;
            px0 = x0;
            px1 = x1;
            if (ybot >= top && ybot < bottom && x0 < right && x1 >= left) {
                if (x0 < left)
                    px0 = 0;    /* BUG: clips to column 0, not to left (all four spans) */
                if (x1 >= right)
                    px1 = right - 1;    /* BUG: the span is [px0, px1), so this drops column right - 1 (all four spans) */
                if (px0 != px1) {
                    w1 = px1 - px0;
                    q = (unsigned short *)(base + rowBytes * ybot + px0 * 2);
                    if (w1 > 1) {
                        do {
                            *(unsigned long *)q = colour;
                            w1 -= 2;
                            q += 2;
                        } while (w1 > 1);
                    }
                    if (w1 != 0)
                        *q = colour;
                }
            }

            ytop = cy - y;
            qx0 = x0;
            qx1 = x1;
            if (ytop >= top && ytop < bottom && x0 < right && x1 >= left) {
                if (x0 < left)
                    qx0 = 0;
                if (x1 >= right)
                    qx1 = right - 1;
                if (qx0 != qx1) {
                    w2 = qx1 - qx0;
                    q = (unsigned short *)(base + rowBytes * ytop + qx0 * 2);
                    if (w2 > 1) {
                        do {
                            *(unsigned long *)q = colour;
                            w2 -= 2;
                            q += 2;
                        } while (w2 > 1);
                    }
                    if (w2 != 0)
                        *q = colour;
                }
            }

            ybot = cy + x;
            x0 = cx - y;
            x1 = cx + y;
            px0 = x0;
            px1 = x1;
            if (ybot >= top && ybot < bottom && x0 < right && x1 >= left) {
                if (x0 < left)
                    px0 = 0;
                if (x1 >= right)
                    px1 = right - 1;
                if (px0 != px1) {
                    w3 = px1 - px0;
                    q2 = (unsigned short *)(base + rowBytes * ybot + px0 * 2);
                    if (w3 > 1) {
                        do {
                            *(unsigned long *)q2 = colour;
                            w3 -= 2;
                            q2 += 2;
                        } while (w3 > 1);
                    }
                    if (w3 != 0)
                        *q2 = colour;
                }
            }

            ytop = cy - x;
            qx2 = x0;
            qx3 = x1;
            if (ytop >= top && ytop < bottom && x0 < right && x1 >= left) {
                if (x0 < left)
                    qx2 = 0;
                if (x1 >= right)
                    qx3 = right - 1;
                if (qx2 != qx3) {
                    w4 = qx3 - qx2;
                    q2 = (unsigned short *)(base + rowBytes * ytop + qx2 * 2);
                    if (w4 > 1) {
                        do {
                            *(unsigned long *)q2 = colour;
                            w4 -= 2;
                            q2 += 2;
                        } while (w4 > 1);
                    }
                    if (w4 != 0)
                        *q2 = colour;
                }
            }

            if (d < 0)
                d += k;
            else {
                d += 2 * (x - y) + 5;
                y--;
            }
            k += 2;
            x++;
        } while (y >= x);
    }
}

/* RectInRgn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bb860
char RectInRgn(Rect *r, void *rgn)
{
    return 1;
}

/* The DIB is created top-down (negative biHeight) so row 0 is the top, and its rowBytes
   is the width rounded up to a multiple of eight. 16 maps to 15 because the port's DIBs
   are 555. */

/* NewGWorld (mac _NewGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bb870
short NewGWorld(CGrafPtr *out, int depth, const Rect *bounds, void *theGDH)
{
    int bitCount;
    CGrafPtr gw;
    void *screenDC;

    /* BUG: no default, so any other depth stores an uninitialised bitCount */
    switch (depth) {
    case 8:
        bitCount = 8;
        break;
    case 16:
        bitCount = 15;
        break;
    case 32:
        bitCount = 32;
        break;
    }
    gw = (CGrafPtr)AllocAPointer(0x50);
    gw->pix.bounds.top = bounds->top;
    gw->pix.bounds.left = bounds->left;
    gw->pix.bounds.bottom = bounds->bottom;
    gw->pix.bounds.right = bounds->right;
    gw->pix.pixelSize = depth;
    gw->backColor.green = -1;
    gw->backColor.blue = gw->backColor.green;
    gw->backColor.red = gw->backColor.blue;
    gw->foreColor.green = 0;
    gw->foreColor.blue = gw->foreColor.green;
    gw->foreColor.red = gw->foreColor.blue;
    gw->pnSize.v = 1;
    gw->pnSize.h = gw->pnSize.v;
    gw->txFont = 0;
    gw->txFace = 0;
    gw->txSize = 9;
    gw->offscreen = 1;
    screenDC = GetDC(Win95MainWindow);
    gw->hdc = CreateCompatibleDC(screenDC);
    ReleaseDC(Win95MainWindow, screenDC);
    gw->pix.ctab = NULL;
    gw->pix.originV = 0;
    gw->pix.width = bounds->right - bounds->left;
    gw->pix.height = bounds->bottom - bounds->top;
    gw->depth = bitCount;
    {
        BitmapInfo bmi = {0};

        bmi.bmiHeader.biSize = kDIBHeaderSize;
        bmi.bmiHeader.biWidth = gw->pix.width;
        bmi.bmiHeader.biHeight = -(gw->pix.height + 1);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = depth;
        bmi.bmiHeader.biCompression = 0;
        bmi.bmiHeader.biSizeImage = 0;
        gw->bitmap = CreateDIBSection(gw->hdc, &bmi, 0, &gw->pix.baseAddr, NULL, 0);
    }
    gw->pix.rowBytes = depth / 8 * gw->pix.width;
    *out = gw;
    return 0;
}

/* DisposeGWorld (mac _DisposeGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba00
void DisposeGWorld(CGrafPtr port)
{
    if (port == NULL)
        return;
    if (port->offscreen != 0) {
        DeleteObject(port->bitmap);
        DeleteDC(port->hdc);
    }
    DeallocAPointer(port);
}

/* GetPixBaseAddr. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba40
void *GetPixBaseAddr(PixMapPtr pm)
{
    return pm->baseAddr;
}

/* GetPixRowBytes. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba50
short GetPixRowBytes(PixMapPtr pm)
{
    return pm->rowBytes;
}

/* GetPixDepth. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba60
short GetPixDepth(PixMapPtr pm)
{
    return pm->pixelSize;
}

/* GetGWorldPixMap. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba70
PixMapPtr GetGWorldPixMap(CGrafPtr world)
{
    return (PixMapPtr)world;
}

/* GetWindowPortRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba80
Rect *GetWindowPortRect(CWindowPtr window)
{
    return &window->pix.bounds;
}

/* GetCWindowPortRect (mac _GetCWindowPortRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bba90
Rect *GetCWindowPortRect(CWindowPtr window)
{
    return GetWindowPortRect(window);
}

/* GetCWindowGrafPort. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbaa0
CGrafPtr GetCWindowGrafPort(CWindowPtr window)
{
    return window;
}

/* SetPortWindowPort. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbab0
void SetPortWindowPort(CWindowPtr window)
{
    SetPort(GetCWindowGrafPort(window));
}

/* GetGWorldDevice (mac _GetGWorldDevice). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbad0
void *GetGWorldDevice(CGrafPtr port)
{
    return NULL;
}

/* GetCWindowPixMap (mac _GetCWindowPixMap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbae0
PixMapPtr GetCWindowPixMap(CWindowPtr window)
{
    return &window->pix;
}

/* GetWindowPixMap (mac _GetWindowPixMap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbaf0
PixMapPtr GetWindowPixMap(CWindowPtr window)
{
    return GetCWindowPixMap(GetCWindowGrafPort(window));
}

/* GetTime (mac (Toolbox GetTime shim)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbb10
void GetTime(DateTimeRec *dtr)
{
    time_t now;
    struct tm *t;

    now = time(NULL);
    t = localtime(&now);
    dtr->year = t->tm_year + 1900;
    dtr->month = t->tm_mon + 1;
    dtr->day = t->tm_mday;
    dtr->hour = t->tm_hour;
    dtr->minute = t->tm_min;
    dtr->second = t->tm_sec;
    dtr->dayOfWeek = t->tm_wday + 1;
}

/* GetCIcon. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbb80
Image_t *GetCIcon(short id)
{
    char **h;
    Image_t *icon;
    int err;

    h = GetResource('cicn', id);
    if (h != NULL) {
        icon = (Image_t *)AllocAPointer(sizeof(Image_t));
        LockAHandle(h);
        err = ImageParseCicn(icon, (Word8 *)*h);
        UnlockAHandle(h);
        ReleaseResource(h);
        if (err != 0) {
            DeallocAPointer(icon);
            return NULL;
        }
        return icon;
    }
    return NULL;
}

/* nv_BlitImageOr. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbbf0
void nv_BlitImageOr(Rect *dstRect, Image_t *icon)
{
    SrcBitMap src;

    if (icon == NULL || dstRect == NULL)
        return;
    if (CurrentGrafPort == NULL)
        return;

    SetRect(&src.bounds, 0, 0, icon->Width, icon->Height);
    src.rowBytes = (icon->DataType + 7) & ~7;
    src.image.ImagePtr = icon->ImagePtr;
    src.image.PalettePtr = icon->PalettePtr;
    src.image.AlphaPtr = icon->AlphaPtr;
    src.image.Width = icon->Width;
    src.image.Height = icon->Height;
    src.image.RowBytes = icon->RowBytes;
    src.image.DataType = icon->DataType;
    CopyBits((PixMapPtr)&src, GetPortBitMapForCopyBits(CurrentGrafPort),
             &src.bounds, dstRect, 1, NULL);
}

/* DisposeCIcon. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbcb0
void DisposeCIcon(Image_t *icon)
{
    ImageDelete(icon);
}

/* QDGetCIconBounds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbcc0
void QDGetCIconBounds(Image_t *icon, Rect *r)
{
    SetRect(r, 0, 0, icon->Width, icon->Height);
}

/* NumToString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbce0
void NumToString(long value, unsigned char *str)
{
    char text[256];

    NumberToAscii(text, value, 0);
    CStr2PStr((char *)str, text);
}

/* GetGDevice (mac _GetGDevice). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbd20
void *GetGDevice(void)
{
    return NULL;
}

/* ExitToShell. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbd30
void ExitToShell(void)
{
    exit(0);
}

/* DisposePixPat (mac _DisposePixPat). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbd40
void DisposePixPat(Image_t *pat)
{
    ImageDelete(pat);
}

/* GetPixPat. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbd50
Image_t *GetPixPat(short id)
{
    char **h;
    Image_t *pat;
    int err;

    h = GetResource('ppat', id);
    if (h != NULL) {
        pat = (Image_t *)AllocAPointer(sizeof(Image_t));
        LockAHandle(h);
        err = ImageParsePPat(pat, (Word8 *)*h);
        UnlockAHandle(h);
        ReleaseResource(h);
        if (err != 0) {
            DeallocAPointer(pat);
            return NULL;
        }
        return pat;
    }
    return NULL;
}

/* FillCRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbdc0
void FillCRect(Rect *dstRect, Image_t *pat)
{
    Rect srcR;
    Rect dstR;
    int x;
    SrcBitMap src;
    unsigned int cols;
    unsigned int rows;
    unsigned int tileW;
    unsigned int tileH;
    int y;
    unsigned int col;

    if (CurrentGrafPort == NULL)
        return;
    tileW = pat->Width;
    cols = dstRect->right - dstRect->left;
    cols = cols + tileW;
    cols = cols - 1;
    cols = cols / tileW;
    tileH = pat->Height;
    rows = dstRect->bottom - dstRect->top;
    rows = rows + tileH;
    rows = rows - 1;
    rows = rows / tileH;
    if (cols == 0 || rows == 0)
        return;

    SetRect(&src.bounds, 0, 0, tileW, tileH);
    src.rowBytes = (pat->DataType + 7) & ~7;
    src.image.ImagePtr = pat->ImagePtr;
    src.image.PalettePtr = pat->PalettePtr;
    src.image.AlphaPtr = pat->AlphaPtr;
    src.image.Width = pat->Width;
    src.image.Height = pat->Height;
    src.image.RowBytes = pat->RowBytes;
    src.image.DataType = pat->DataType;
    y = 0;
    do {
        col = cols;
        x = 0;
        do {
            dstR.left = dstRect->left + x;
            dstR.right = dstR.left + pat->Width;
            dstR.top = dstRect->top + y;
            dstR.bottom = dstR.top + pat->Height;
            if (dstR.right > dstRect->right)
                dstR.right = dstRect->right;
            if (dstR.bottom > dstRect->bottom)
                dstR.bottom = dstRect->bottom;
            srcR.left = srcR.top = 0;
            srcR.right = dstR.right - dstR.left;
            srcR.bottom = dstR.bottom - dstR.top;
            CopyBits((PixMapPtr)&src, GetPortBitMapForCopyBits(CurrentGrafPort),
                     &srcR, &dstR, 0, NULL);
            x = x + pat->Width;
            col -= 1;
        } while (col != 0);
        y += pat->Height;
        rows -= 1;
    } while (rows != 0);
}

#define kResourceErrorCode 0x83

/* ParamText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bbf90
void ParamText(const unsigned char *p0, const unsigned char *p1,
               const unsigned char *p2, const unsigned char *p3)
{
    gParamText0[0] = 0;
    gParamText1[0] = 0;
    gParamText2[0] = 0;
    gParamText3[0] = 0;
    if (p0 != NULL && p0[0] != 0) {
        memcpy(gParamText0, p0 + 1, p0[0]);
        gParamText0[p0[0]] = 0;
    }
    if (p1 != NULL && p1[0] != 0) {
        memcpy(gParamText1, p1 + 1, p1[0]);
        gParamText1[p1[0]] = 0;
    }
    if (p2 != NULL && p2[0] != 0) {
        memcpy(gParamText2, p2 + 1, p2[0]);
        gParamText2[p2[0]] = 0;
    }
    if (p3 != NULL && p3[0] != 0) {
        memcpy(gParamText3, p3 + 1, p3[0]);
        gParamText3[p3[0]] = 0;
    }
}

/* Alert. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc070
short Alert(short code, void *filterProc)
{
    char message[1024];

    if (code == kResourceErrorCode)
        sprintf(message, "Resource ID:  %s\nCode: %s\n%s",
                gParamText0, gParamText1, gParamText2);
    else
        sprintf(message, "%s %s %s", gParamText0, gParamText1, gParamText2);
    ShowAlert("", message);
    return 1;
}

/* Delay. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc0e0
void Delay(unsigned long ticks, unsigned long *finalTicks)
{
    LastTick = ReadTick();
    WaitTicks(ticks);
}

/* ReplaceInHandle. STATE: MATCH. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004bc100
long ReplaceInHandle(char **h, long offset, const char *find, long findLen,
                      const char *repl, long replLen)
{
    char *start;
    long count;
    char c;
    char *p;
    long oldSize;
    long newSize;
    long n;
    int handleSize;

    LockAHandle(h);
    start = *h + offset;
    count = GetAHandleSize(h) - offset - findLen + 1;
    if (count <= 0) {
        UnlockAHandle(h);
        return -1;
    }
    c = find[0];
    p = (char *)memchr(start, c, count);
    while (p != NULL) {
        if (memcmp(p, find, findLen) == 0) {
            oldSize = GetAHandleSize(h);
            handleSize = oldSize;
            newSize = oldSize + replLen - findLen;
            offset += p - start;
            if (newSize > oldSize) {
                ResizeAHandle(h, newSize);
                LockAHandle(h);
                p = *h + offset;
            }
            if (replLen != findLen) {
                n = handleSize - offset - findLen;
                if (n != 0)
                    memmove(p + replLen, p + findLen, n);
            }
            memcpy(p, repl, replLen);
            if (newSize < oldSize)
                ResizeAHandle(h, newSize);
            UnlockAHandle(h);
            return offset + replLen;
        }
        p++;
        count -= p - start;
        offset += p - start;
        start = p;
        p = (char *)memchr(start, c, count);
    }
    UnlockAHandle(h);
    return -1;
}

/* LoadSoundResource. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc2a0
void *LoadSoundResource(short id)
{
    char **h;
    void **copy;
    void *sound;

    sound = NULL;
    h = GetResource(kSoundType, id);
    if (h != NULL) {
        LockAHandle(h);
        copy = (void **)IndirectInitialize(*h, GetAHandleSize(h));
        ReleaseResource(h);
        if (copy != NULL) {
            sound = ST_LoadSndResourceIndirect(copy, 0);
            IndirectDeallocate(copy);
        }
    }
    return sound;
}

/* SysBeep. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc300
void SysBeep(short duration)
{
}

#define kGammaSteps  32
#define kGammaRampMax 31

/* BuildGammaRamp. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bc310
void BuildGammaRamp(register float gamma)
{
    int i;
    double e;

    e = 1.0 / (gamma / 100.0f);
    for (i = 0; i < kGammaSteps; i++) {
        gGammaRamp[i] = (pow(i / 31.0, e) * 31.0 + 0.5);
        if (gGammaRamp[i] > kGammaRampMax)
            gGammaRamp[i] = kGammaRampMax;
    }
}
