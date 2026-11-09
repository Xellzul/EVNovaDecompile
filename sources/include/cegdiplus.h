/* cegdiplus.h -- the part of the GDI+ flat API that the CE blitters call (sources/cefix/ceblit.c),
   declared by hand because the SDK's GdiPlus.h is C++ only. Values from the 418 SDK's
   GdiPlusFlat.h, GdiPlusInit.h, GdiPlusEnums.h and GdiPlusPixelFormats.h; imports from its
   GdiPlus.lib (tools/link.ps1 -Cefix). Declares nothing outside #ifdef CEFIX. */
#ifndef CEGDIPLUS_H
#define CEGDIPLUS_H

#ifdef CEFIX

typedef struct GpImage GpImage;
typedef GpImage GpBitmap;
typedef struct GpGraphics GpGraphics;
typedef struct GpImageAttributes GpImageAttributes;

typedef struct GdiplusStartupInput {
    unsigned int GdiplusVersion;
    void *DebugEventCallback;
    int SuppressBackgroundThread;
    int SuppressExternalCodecs;
} GdiplusStartupInput;

#define kGdiplusVersion                     1

#define PixelFormatGDI                      0x00020000
#define PixelFormatAlpha                    0x00040000
#define PixelFormat16bppRGB555              (5 | (16 << 8) | PixelFormatGDI)
#define PixelFormat16bppARGB1555            (7 | (16 << 8) | PixelFormatAlpha | PixelFormatGDI)
#define PixelFormat24bppRGB                 (8 | (24 << 8) | PixelFormatGDI)
#define PixelFormat32bppRGB                 (9 | (32 << 8) | PixelFormatGDI)

#define InterpolationModeHighQualityBicubic 7
#define PixelOffsetModeHighQuality          2
#define UnitPixel                           2

__declspec(dllimport) int __stdcall GdiplusStartup(unsigned long *token,
    const GdiplusStartupInput *input, void *output);
__declspec(dllimport) int __stdcall GdipCreateBitmapFromScan0(int width, int height, int stride,
    int format, unsigned char *scan0, GpBitmap **bitmap);
__declspec(dllimport) int __stdcall GdipGetImageGraphicsContext(GpImage *image,
    GpGraphics **graphics);
__declspec(dllimport) int __stdcall GdipSetInterpolationMode(GpGraphics *graphics, int mode);
__declspec(dllimport) int __stdcall GdipSetPixelOffsetMode(GpGraphics *graphics, int mode);
__declspec(dllimport) int __stdcall GdipDrawImageRectRectI(GpGraphics *graphics, GpImage *image,
    int dstx, int dsty, int dstwidth, int dstheight, int srcx, int srcy, int srcwidth,
    int srcheight, int srcUnit, const GpImageAttributes *imageAttributes, void *callback,
    void *callbackData);
__declspec(dllimport) int __stdcall GdipDeleteGraphics(GpGraphics *graphics);
__declspec(dllimport) int __stdcall GdipDisposeImage(GpImage *image);

#endif /* CEFIX */

#endif /* CEGDIPLUS_H */
