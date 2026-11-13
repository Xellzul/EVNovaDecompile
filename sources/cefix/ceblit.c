// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* ceblit.c -- EV Nova CE: GDI+ bicubic blitters in the CopyBits and CopyMask tables, and the
   true row stride of a new GWorld (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <stdlib.h>
#include <string.h>
#include "macport.h"
#include "cefix.h"
#include "cegdiplus.h"

#define kGdipRowAlign       4           /* GDI+ wants every scan line on a 4-byte boundary */
#define kAlpha1555High      0x80        /* bit 15 of a 1555 pixel, in its high byte */

typedef struct WinBitmap {              /* Win32 BITMAP */
    int32_t bmType;
    int32_t bmWidth;
    int32_t bmHeight;
    int32_t bmWidthBytes;
    unsigned short bmPlanes;
    unsigned short bmBitsPixel;
    void *bmBits;
} WinBitmap;

__declspec(dllimport) int __stdcall GetObjectA(void *object, int size, void *out);

static uint32_t ceGdiplusToken;

static void StartupGdiplus(void)
{
    GdiplusStartupInput input;

    if (ceGdiplusToken != 0)
        return;
    input.GdiplusVersion = kGdiplusVersion;
    input.DebugEventCallback = NULL;
    input.SuppressBackgroundThread = 0;
    input.SuppressExternalCodecs = 0;
    GdiplusStartup(&ceGdiplusToken, &input, NULL);
}

static int PixelFormatOf(int32_t pixelSize)
{
    switch (pixelSize) {
    case 16:
        return PixelFormat16bppRGB555;
    case 24:
        return PixelFormat24bppRGB;
    default:
        return PixelFormat32bppRGB;
    }
}

/* srcRect of SRC drawn bicubic into dstRect of DST's own pixels. */
static void DrawBitmap(GpBitmap *src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect)
{
    GpBitmap *target;
    GpGraphics *graphics;

    target = NULL;
    GdipCreateBitmapFromScan0(dst->width, dst->height, dst->rowBytes,
                              PixelFormatOf(dst->pixelSize), dst->baseAddr, &target);
    graphics = NULL;
    GdipGetImageGraphicsContext(target, &graphics);
    GdipSetInterpolationMode(graphics, InterpolationModeHighQualityBicubic);
    GdipSetPixelOffsetMode(graphics, PixelOffsetModeHighQuality);
    GdipDrawImageRectRectI(graphics, src,
                           dstRect->left, dstRect->top,
                           dstRect->right - dstRect->left, dstRect->bottom - dstRect->top,
                           srcRect->left, srcRect->top,
                           srcRect->right - srcRect->left, srcRect->bottom - srcRect->top,
                           UnitPixel, NULL, NULL, NULL);
    GdipDeleteGraphics(graphics);
    GdipDisposeImage(target);
}

/* A source whose rows are not 4-byte aligned is drawn from an aligned copy. */
static void Blit16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect, int alpha)
{
    GpBitmap *bitmap;
    unsigned char *buffer;
    unsigned char *out;
    unsigned char *in;
    int format;
    int32_t stride;
    int32_t y;

    StartupGdiplus();
    format = alpha ? PixelFormat16bppARGB1555 : PixelFormat16bppRGB555;
    stride = src->rowBytes;
    bitmap = NULL;
    if (stride % kGdipRowAlign != 0) {
        stride += kGdipRowAlign - stride % kGdipRowAlign;
        buffer = malloc(stride * src->height);
        out = buffer;
        in = src->baseAddr;
        for (y = 0; y < src->height; y++) {
            memcpy(out, in, src->rowBytes);
            out += stride;
            in += src->rowBytes;
        }
        GdipCreateBitmapFromScan0(src->width, src->height, stride, format, buffer, &bitmap);
        DrawBitmap(bitmap, dst, srcRect, dstRect);
        GdipDisposeImage(bitmap);
        free(buffer);
    } else {
        GdipCreateBitmapFromScan0(src->width, src->height, stride, format, src->baseAddr,
                                  &bitmap);
        DrawBitmap(bitmap, dst, srcRect, dstRect);
        GdipDisposeImage(bitmap);
    }
}

void CE_Blit16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit16(src, dst, srcRect, dstRect, 0);
}

/* GDI+ keeps a 24bppRGB pixel as B, G, R: the source's R, G, B go to an aligned copy swapped. */
void CE_Blit24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    GpBitmap *bitmap;
    unsigned char *buffer;
    unsigned char *out;
    unsigned char *in;
    int32_t stride;
    int32_t x;
    int32_t y;

    StartupGdiplus();
    stride = src->rowBytes;
    if (stride % kGdipRowAlign != 0)
        stride += kGdipRowAlign - stride % kGdipRowAlign;
    buffer = malloc(stride * src->height);
    out = buffer;
    in = src->baseAddr;
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            out[x * 3 + 0] = in[x * 3 + 2];
            out[x * 3 + 1] = in[x * 3 + 1];
            out[x * 3 + 2] = in[x * 3 + 0];
        }
        out += stride;
        in += src->rowBytes;
    }
    bitmap = NULL;
    GdipCreateBitmapFromScan0(src->width, src->height, stride, PixelFormat24bppRGB, buffer,
                              &bitmap);
    DrawBitmap(bitmap, dst, srcRect, dstRect);
    GdipDisposeImage(bitmap);
    free(buffer);
}

void CE_Blit32(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    GpBitmap *bitmap;

    StartupGdiplus();
    bitmap = NULL;
    GdipCreateBitmapFromScan0(src->width, src->height, src->rowBytes, PixelFormat32bppRGB,
                              src->baseAddr, &bitmap);
    DrawBitmap(bitmap, dst, srcRect, dstRect);
    GdipDisposeImage(bitmap);
}

/* A black mask pixel makes its source pixel opaque, written into the source itself as CE
   does; maskRect is not read. */
void CE_Blit16Mask(PixMapPtr src, PixMapPtr mask, PixMapPtr dst, Rect *srcRect,
                   Rect *maskRect, Rect *dstRect)
{
    unsigned char *pixels;
    unsigned char *maskPixels;
    int32_t x;
    int32_t y;

    pixels = src->baseAddr;
    maskPixels = mask->baseAddr;
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            if (maskPixels[y * mask->rowBytes + x * 2] == 0)
                pixels[y * src->rowBytes + x * 2 + 1] |= kAlpha1555High;
        }
    }
    Blit16(src, dst, srcRect, dstRect, 1);
}

/* NewGWorld records depth / 8 * width as the row stride, but the DIB section pads each row
   to 4 bytes: take the stride the bitmap really has, so odd widths draw straight. */
short CE_NewGWorld(CGrafPtr *out, int depth, const Rect *bounds, void *theGDH)
{
    WinBitmap bm;
    short err;

    err = NewGWorld(out, depth, bounds, theGDH);
    GetObjectA((*out)->bitmap, sizeof(bm), &bm);
    (*out)->pix.rowBytes = bm.bmWidthBytes;
    return err;
}

#endif /* CEFIX */
