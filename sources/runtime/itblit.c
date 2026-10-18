// COMPILE: -opt level=2,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: the draw and sound tools call memset/memcpy at constant sizes, so no ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itblit.c -- the draw tool: the two standard palettes and their built guard, the image records (a header owning a port, a device and a pixel map, allocated in one block) and the file-format sniff, and the pixel blitter: the span routines (frame x operation x destination alpha, plus same-format fast paths), their dispatch tables, the scaling blitter and the tiling blitter. Notes: Research/notes/itblit.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "win32.h"
#include "ambrosia.h"

extern void *memset(void *dst, int c, unsigned int n);

long gGDIDIBSectionFunc;                               /* 0x00870dd0, 8 B bound */
unsigned char gDT_Open;                      /* 0x00870e0a, 1 B, the palettes-built guard */

#define kCubeLevels     6
#define kRampSteps      10
#define kRampCount      4
#define kOpaque         0xff

ITPalette gDT_DefaultPalette;                          /* 0x0087097c system: count, then the entries */
ITPalette gDT_AlphaPalette;                          /* 0x008701c4 greyscale: count, then the entries */
void *gMsimgTransparentBlit;                              /* 0x008706fc TransparentBlt */

/* 0x00576d00: per channel width, the factor that widens it to 16 bits (low word). */
unsigned long gDT_Mults[9] = {
    0x00000000, 0x0000ffff, 0x00005555, 0x00002492, 0x00001111,
    0x00000842, 0x00000410, 0x00000204, 0x00000101
};
/* QuickDraw's standard colours and patterns: white, light grey, grey, dark grey, black. */
ITColor gDT_ColorWhite = { 0xff, 0xff, 0xff, 0xff }; /* 0x00576d24 default background */
ITColor gDT_ColorLtGray = { 0xff, 0xc0, 0xc0, 0xc0 }; /* 0x00576d28 */
ITColor gDT_ColorMdGray = { 0xff, 0x80, 0x80, 0x80 }; /* 0x00576d2c */
ITColor gDT_ColorDkGray = { 0xff, 0x40, 0x40, 0x40 }; /* 0x00576d30 */
ITColor gDT_ColorBlack = { 0xff, 0x00, 0x00, 0x00 }; /* 0x00576d34 default text colour, black */
unsigned char gDT_PatternWhite[16] = {              /* 0x00576d38 the white pattern */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
unsigned char gDT_PatternLtGray[16] = {              /* 0x00576d48 the light grey pattern */
    0x77, 0x77, 0xdd, 0xdd, 0x77, 0x77, 0xdd, 0xdd,
    0x77, 0x77, 0xdd, 0xdd, 0x77, 0x77, 0xdd, 0xdd
};
unsigned char gDT_PatternMdGray[16] = {              /* 0x00576d58 the 50% grey pattern */
    0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
    0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55
};
unsigned char gDT_PatternDkGray[16] = {              /* 0x00576d68 the dark grey pattern */
    0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22,
    0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22
};
unsigned char gDT_PatternBlack[16] = { 0 };          /* 0x00576d78 the solid pattern */

/* DT_Open. STATE: MATCH 100%, exe 137 / obj 137, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db31b
long DT_Open(void)
{
    long err = 0;
    unsigned char cube[kCubeLevels] = { 0xff, 0xcc, 0x99, 0x66, 0x33, 0x00 };
    unsigned char levels[kRampSteps] = {
        0xee, 0xee, 0xbb, 0xaa, 0x88, 0x77, 0x55, 0x44, 0x22, 0x11
    };
    long e;
    long i;
    ULong r;
    ULong g;
    ULong b;
    long ramp;
    ULong step;

    if (gDT_Open != 0)
        goto done;
    e = PlatformOpen();
    if (e != 0) {
        err = e;
        goto done;
    }

    gDT_DefaultPalette.count = kPaletteEntries;

    i = 0;
    for (r = 0; r < kCubeLevels; r = r + 1) {
        for (g = 0; g < kCubeLevels; g = g + 1) {
            for (b = 0; b < kCubeLevels; b = b + 1) {
                gDT_DefaultPalette.entry[i].alpha = kOpaque;
                gDT_DefaultPalette.entry[i].red = cube[r];
                gDT_DefaultPalette.entry[i].green = cube[g];
                gDT_DefaultPalette.entry[i].blue = cube[b];
                i = i + 1;
            }
        }
    }
    i = i - 1;

    for (ramp = 0; ramp < kRampCount; ramp = ramp + 1) {
        for (step = 0; step < kRampSteps; step = step + 1) {
            gDT_DefaultPalette.entry[i].alpha = kOpaque;
            gDT_DefaultPalette.entry[i].red =
                (ramp == 0 || ramp == 3) ? levels[step] : 0;
            gDT_DefaultPalette.entry[i].green =
                (ramp == 1 || ramp == 3) ? levels[step] : 0;
            gDT_DefaultPalette.entry[i].blue =
                (ramp == 2 || ramp == 3) ? levels[step] : 0;
            i = i + 1;
        }
    }

    gDT_DefaultPalette.entry[i].alpha = kOpaque;
    gDT_DefaultPalette.entry[i].red = 0;
    gDT_DefaultPalette.entry[i].green = 0;
    gDT_DefaultPalette.entry[i].blue = 0;

    gDT_AlphaPalette.count = kPaletteEntries;
    for (i = 0; i < kPaletteEntries; i = i + 1) {
        gDT_AlphaPalette.entry[i].alpha = kOpaque;
        gDT_AlphaPalette.entry[i].blue = i;
        gDT_AlphaPalette.entry[i].green = gDT_AlphaPalette.entry[i].blue;
        gDT_AlphaPalette.entry[i].red = gDT_AlphaPalette.entry[i].green;
    }

    gMsimgTransparentBlit = GetProcAddress(GetModuleHandleA("MSIMG32.DLL"),
                                  "TransparentBlt");
    gGDIDIBSectionFunc = (long)GetProcAddress(GetModuleHandleA("GDI32.DLL"),
                                        "CreateDIBSection");
    gDT_Open = 1;
done:
    return err;
}

/* DT_Close. STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db4df
void DT_Close(void)
{
    long err;

    err = 0;
    gGDIDIBSectionFunc = 0;
    gDT_Open = 0;
}

/* DT_IsOpen (have the standard palettes been built: DT_Open's guard). STATE:
   MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004db4f3
unsigned char DT_IsOpen(void)
{
    return gDT_Open;
}

static const double DAT_00576dd8[2] = { 1.0 / 255.0, 255.0 };    /* 0x00576dd8, unreferenced */

#define kPortSize           sizeof(ITPort)
#define kDeviceSize         0x7c
#define kPixelsSize         sizeof(ITPixels)
#define kSniffHead          0x10
#define kSniffPict          0x264
#define kSniffPpat          0x4e
#define kSniffTga           0x12
#define kUnknownFormat      -7301

extern unsigned char BMP_CheckFileType(const void *data, ULong size);            /* itbmp.c */
extern unsigned char GIF_CheckFileType(const unsigned char *data, ULong size);   /* itgif.c */
extern unsigned char JPEG_CheckFileType(const void *data, ULong size);            /* itjpeg.c */
extern unsigned char PNG_CheckFileType(unsigned char *data, ULong size);         /* itpng.c */
extern unsigned char PICT_CheckFileType(const unsigned char *data, ULong size);   /* itpict.c */
extern unsigned char PPAT_CheckFileType(const unsigned char *data, ULong size);   /* itppat.c */
extern unsigned char TGA_CheckFileType(const void *data, ULong size);            /* ittga.c */

/* _DT_SurfaceCreate (a new image: port and device, port and pixels, pixels alone or port alone, by
   which out-pointers are given; NULL when the block cannot be had). STATE: MATCH 100%, exe 200 /
   obj 200, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db4f9
ITImage *_DT_SurfaceCreate(ITPort **portOut, ITDevice **deviceOut, ITPixels **pixelsOut)
{
    long err;
    ITImage *image;
    char *mem;
    ULong size;

    err = 0;
    image = 0;
    mem = 0;
    size = 0;
    if (portOut != 0 && deviceOut != 0) {
        *portOut = 0;
        *deviceOut = 0;
        size = sizeof(ITImage) + kPortSize + kDeviceSize;
        mem = MemoryAllocate(size);
        if (mem == 0)
            goto nomem;
        memset(mem, 0, size);
        image = (ITImage *)mem;
        *portOut = (ITPort *)(mem + sizeof(ITImage));
        *deviceOut = (ITDevice *)(mem + (sizeof(ITImage) + kPortSize));
        image->portSelf = image;
        image->port = *portOut;
        image->deviceSelf = image;
        image->device = *deviceOut;
        image->pixelsSelf = 0;
        image->pixels = 0;
        (*portOut)->link.owner = image;
        (*portOut)->link.owner2 = image;
        (*portOut)->link.self = image->portSelf;
        (*deviceOut)->link.owner = image;
        (*deviceOut)->link.owner2 = image;
        (*deviceOut)->link.self = image->deviceSelf;
    } else if (portOut != 0 && pixelsOut != 0) {
        *portOut = 0;
        *pixelsOut = 0;
        size = sizeof(ITImage) + kPortSize + kPixelsSize;
        mem = MemoryAllocate(size);
        if (mem == 0)
            goto nomem;
        memset(mem, 0, size);
        image = (ITImage *)mem;
        *portOut = (ITPort *)(mem + sizeof(ITImage));
        *pixelsOut = (ITPixels *)(mem + (sizeof(ITImage) + kPortSize));
        image->portSelf = image;
        image->port = *portOut;
        image->deviceSelf = 0;
        image->device = 0;
        image->pixelsSelf = image;
        image->pixels = *pixelsOut;
        (*portOut)->link.owner = image;
        (*portOut)->link.owner2 = image;
        (*portOut)->link.self = image->portSelf;
        (*pixelsOut)->link.owner = image;
        (*pixelsOut)->link.owner2 = image;
        (*pixelsOut)->link.self = image->pixelsSelf;
    } else if (portOut == 0) {
        *pixelsOut = 0;
        size = sizeof(ITImage) + kPixelsSize;
        mem = MemoryAllocate(size);
        if (mem == 0)
            goto nomem;
        memset(mem, 0, size);
        image = (ITImage *)mem;
        *pixelsOut = (ITPixels *)(mem + sizeof(ITImage));
        image->portSelf = 0;
        image->port = 0;
        image->deviceSelf = 0;
        image->device = 0;
        image->pixelsSelf = image;
        image->pixels = *pixelsOut;
        (*pixelsOut)->link.owner = image;
        (*pixelsOut)->link.owner2 = image;
        (*pixelsOut)->link.self = image->pixelsSelf;
    } else if (pixelsOut == 0) {
        *portOut = 0;
        size = sizeof(ITImage) + kPortSize;
        mem = MemoryAllocate(size);
        if (mem == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
        memset(mem, 0, size);
        image = (ITImage *)mem;
        *portOut = (ITPort *)(mem + sizeof(ITImage));
        image->portSelf = image;
        image->port = *portOut;
        image->deviceSelf = 0;
        image->device = 0;
        image->pixelsSelf = 0;
        image->pixels = 0;
        (*portOut)->link.owner = image;
        (*portOut)->link.owner2 = image;
        (*portOut)->link.self = image->portSelf;
    }
done:
    return image;
}

/* _DT_SurfaceDispose (dispose an image and its parts; answers the device's status; the three flags,
   which part's disposer is calling, are never read). STATE: MATCH 100%, exe 70 / obj 70, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db721
long _DT_SurfaceDispose(ITImage *image, unsigned char fromPort, unsigned char fromDevice,
                  unsigned char fromPixels)
{
    long status;

    status = 0;
    if (image->port != 0) {
        image->port->link.owner = 0;
        image->port->link.owner2 = 0;
        image->port->link.self = 0;
        if (image->port->dc != 0)
            _DT_ContextDispose(image->port);
        image->port = 0;
        image->portSelf = 0;
    }
    if (image->device != 0) {
        image->device->link.owner = 0;
        image->device->link.owner2 = 0;
        image->device->link.self = 0;
        _DT_PrinterDispose(image->device);
        status = image->device->status;
        image->device = 0;
        image->deviceSelf = 0;
    }
    if (image->pixels != 0) {
        image->pixels->link.owner = 0;
        image->pixels->link.owner2 = 0;
        image->pixels->link.self = 0;
        _DT_ImageDispose(image->pixels);
        image->pixels = 0;
        image->pixelsSelf = 0;
    }
    MemoryDeallocate(image);
    return status;
}

/* _DT_SurfaceGetContext (an image's port). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db7cb
ITPort *_DT_SurfaceGetContext(void *imageRef)
{
    ITImage *image = (ITImage *)imageRef;
    long err;

    err = 0;
    return image->port;
}

/* _DT_SurfaceGetImage (an image's pixel map). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db7d9
ITPixels *_DT_SurfaceGetImage(void *imageRef)
{
    ITImage *image = (ITImage *)imageRef;
    long err;

    err = 0;
    return image->pixels;
}

/* _DT_SurfaceGetAlpha (an image's pixel map, NULL for no image; the key test's error is dead in
   the exe too). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db7e7
ITPixels *_DT_SurfaceGetAlpha(void *imageRef)
{
    ITImage *image = (ITImage *)imageRef;
    long err;

    err = 0;
    if (image != 0 && image->pixels->key != kPixelsKey)
        err = kBadArgument;
    return image != 0 ? image->pixels : 0;
}

/* _DT_ContextGetContext. STATE: MATCH 100%, exe 4 / obj 4, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db810
ITPort *_DT_ContextGetContext(ITImage *image)
{
    return _DT_SurfaceGetContext(image);
}

/* _DT_ImageGetImage. STATE: MATCH 100%, exe 4 / obj 4, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db81b
ITPixels *_DT_ImageGetImage(ITImage *image)
{
    return _DT_SurfaceGetImage(image);
}

/* _DT_ImageGetAlpha. STATE: MATCH 100%, exe 4 / obj 4, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db826
ITPixels *_DT_ImageGetAlpha(ITImage *image)
{
    return _DT_SurfaceGetAlpha(image);
}

/* DT_ImageFileFormat (the file format of a handle's data by its header, as a type code; 0 when none).
   STATE: MATCH 100%, exe 133 / obj 133, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db831
ULong DT_ImageFileFormat(void **handle)
{
    unsigned char pict[kSniffPict];
    unsigned char ppat[kSniffPpat];
    unsigned char tga[kSniffTga];
    unsigned char head[kSniffHead];
    long err;
    ULong type;

    err = 0;
    type = 0;
    if (IndirectGetSize(handle) >= kSniffHead) {
        void *data = *handle;
        void *copy = data;
        memmove(head, data, kSniffHead);
        if (BMP_CheckFileType(head, kSniffHead) != 0)
            return 'BMPf';
        if (GIF_CheckFileType(head, kSniffHead) != 0)
            return 'GIFf';
        if (JPEG_CheckFileType(head, kSniffHead) != 0)
            return 'JPEG';
        if (PNG_CheckFileType(head, kSniffHead) != 0)
            return 'PNGf';
    }
    if (IndirectGetSize(handle) >= kSniffPict) {
        void *data = *handle;
        void *copy = data;
        memmove(pict, data, kSniffPict);
        if (PICT_CheckFileType(pict, kSniffPict) != 0)
            return 'PICT';
    }
    if (IndirectGetSize(handle) >= kSniffPpat) {
        void *data = *handle;
        void *copy = data;
        memmove(ppat, data, kSniffPpat);
        if (PPAT_CheckFileType(ppat, kSniffPpat) != 0)
            return 'ppat';
    }
    if (IndirectGetSize(handle) >= kSniffTga) {
        void *data = *handle;
        void *copy = data;
        memmove(tga, data, kSniffTga);
        /* BUG: sniffs the PICT buffer, not the one just filled */
        if (TGA_CheckFileType(pict, kSniffTga) != 0)
            return 'TPIC';
    }
    return 0;
}

/* DT_ImageFileLoad (load an image out of a handle: sniff its format and run that loader over a memory
   stream on it). STATE: MATCH 100%, exe 133 / obj 133, byte-identical (see the build). OPEN:
   none */
// FUNCTION: EVNOVA 0x004db9b0
long DT_ImageFileLoad(void **handle, ITImage **imageOut, ITImage **maskOut, ITTransparent *trans,
                  ITBackground *bg)
{
    long err;
    ITStream stream;

    err = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);
    stream.data = handle;
    stream.size = IndirectGetSize(handle);
    stream.pos = 0;
    switch (DT_ImageFileFormat(handle)) {
    case 'BMP ':
    case 'BMPf':
    {
        long e = BMP_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 'GIF ':
    case 'GIFf':
    {
        long e = GIF_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 'JFIF':
    case 'JPEG':
    {
        long e = JPEG_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 'PICT':
    {
        long e = PICT_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 'PNG ':
    case 'PNGf':
    {
        long e = PNG_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 'ppat':
    {
        long e = PPAT_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 'TGA ':
    case 'TPIC':
    {
        long e = TGA_Decode(&stream, imageOut, maskOut, trans, bg);

        if (e != 0)
            err = e;
        break;
    }
    case 0:
        err = kUnknownFormat;
        goto done;
    default:
        err = kUnknownFormat;
    }
done:
    return err;
}

/* DT_ImageGetRect (an image's bounds). STATE: MATCH 100%, exe 20 / obj 20, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004dbb13
ITRect *DT_ImageGetRect(ITImage *image, ITRect *bounds)
{
    ITPixels *pixels;
    long err;

    pixels = _DT_ImageGetImage(image);
    err = 0;
    *bounds = pixels->bounds;
    return bounds;
}

/* DT_ImageGetInfo (describe a locked image's pixels for a drawing routine: sizes, bounds, the bits
   of it and of `other', its DIB, a port DC ready for GDI, palette and inverse table for one-byte
   pixels, the format; answers whether it was locked). STATE: MATCH 100%, exe 103 / obj 103, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dbb37
unsigned char DT_ImageGetInfo(void *imageRef, ITImage *other, ITPixelInfo *info)
{
    ITImage *image;
    ITPixels *pixels;
    ITPixels *otherPixels;
    long err;
    unsigned char locked;
    unsigned char isLocked;
    ULong i;

    image = (ITImage *)imageRef;
    pixels = _DT_ImageGetImage(image);
    otherPixels = _DT_ImageGetAlpha(other);
    err = 0;
    locked = 0;
    memset(info, 0, sizeof(*info));
    if (pixels->locks != 0)
        isLocked = 1;
    else
        isLocked = 0;
    locked = isLocked;
    info->bytesPerPixel = pixels->bytesPerPixel;
    info->depth = pixels->depth;
    info->rowBytes = pixels->rowBytes;
    info->otherRowBytes = otherPixels != 0 ? otherPixels->rowBytes : 0;
    info->bounds = pixels->bounds;
    info->bits = (void *)pixels->baseAddr;
    info->otherBits = otherPixels != 0 ? (void *)otherPixels->baseAddr : 0;
    info->dib = pixels->dib;
    info->image = imageRef;
    if (image->port != 0) {
        info->dc = DT_PeekAtSysContext(image->portSelf);
        info->portImage = image->portSelf;
    }
    info->formatFlag = pixels->formatFlag;
    for (i = 0; i < 4; i++) {
        info->channelBits[i] = pixels->bits[i];
        info->shift[i] = pixels->shift[i];
        info->mask[i] = pixels->mask[i];
        info->scale[i] = pixels->scale[i];
    }
    if (pixels->bytesPerPixel == 1) {
        info->palette = &pixels->palette;
        info->inverse = pixels->inverse;
    }
    info->key = pixels->key;
    return isLocked;
}

/* DT_ImageLock (lock an image, and `other', for drawing into its bits: pixels GDI last wrote
   are read back from the bitmap first, the pixels marked changed, `info' filled when given).
   STATE: MATCH 100%, exe 54 / obj 54, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dbc5e
long DT_ImageLock(ITImage *image, ITImage *other, ITPixelInfo *info, unsigned char *rebuilt)
{
    ITPixels *pixels;
    long err;
    long e;
    long result;

    pixels = _DT_ImageGetImage(image);
    _DT_ImageGetAlpha(other);
    err = 0;
    result = 0;
    if (rebuilt != 0)
        *rebuilt = 0;
    e = _DT_ImageLock(image, other, rebuilt);
    if (e != 0)
        goto fail;
    if ((pixels->flags & kPixelsInBitmap) != 0) {
        e = DT_SyncDib(pixels->dib, 0, 1);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        pixels->flags &= ~kPixelsInBitmap;
    }
    pixels->flags |= kPixelsBitsChanged;
    if (info != 0)
        DT_ImageGetInfo(image, other, info);
done:
    return err;
}

/* DT_ImageUnlock (mark an image's pixels changed and unlock it and `other'). STATE: MATCH 100%,
   exe 24 / obj 24, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dbce3
void DT_ImageUnlock(ITImage *image, ITImage *other)
{
    ITPixels *pixels;
    long err;

    pixels = _DT_ImageGetImage(image);
    _DT_ImageGetAlpha(other);
    err = 0;
    pixels->flags |= kPixelsBitsChanged;
    _DT_ImageUnlock(image, other);
}

/* DT_PeekAtSysContext (a port's DC for GDI drawing: changed or locked pixels are stored into the
   bitmap first). STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dbd13
void *DT_PeekAtSysContext(ITImage *image)
{
    ITPort *port;
    ITPixels *pixels;
    long err;
    long e;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (port->link.owner2->pixelsSelf != 0) {
        pixels = _DT_SurfaceGetImage(port->link.owner2);
        if (pixels->locks != 0 || (pixels->flags & kPixelsBitsChanged) != 0) {
            e = DT_SyncDib(pixels->dib, 1, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            pixels->flags &= ~kPixelsBitsChanged;
        }
        pixels->flags |= kPixelsInBitmap;
    }
done:
    return port->dc;
}

/* how the source's pixels meet the destination's (ITTransparent.mode, or implied) */
#define kBlitCopy           0
#define kBlitMask           1
#define kBlitAlpha          2
#define kBlitBlend          3
#define kBlitMaskBlend      4
#define kBlitKey            5

/* what becomes of the destination's alpha channel */
#define kAlphaOpaque        0
#define kAlphaKeep          1
#define kAlphaSet           2

#define kIndexedBytes       1
#define kKeyAlphaNone       0xff

typedef void (*ITSpanRows)(ITPixels *dst, ITPixels *src, ITPixels *mask, unsigned char *dstRow,
                           unsigned char *srcRow, unsigned char *maskRow, ULong height,
                           ULong width, ULong *dstClut, unsigned char *inverse, ULong *srcClut,
                           ULong extra);
typedef void (*ITSpanScaled)(ITPixels *dst, ITPixels *src, ITPixels *mask, unsigned char *dstRow,
                             unsigned char *srcRow, unsigned char *maskRow, ULong height,
                             ULong width, ULong yErr, ULong xErr, ULong dstHeight,
                             ULong dstWidth, ULong srcHeight, ULong srcWidth, ULong *dstClut,
                             unsigned char *inverse, ULong *srcClut, ULong extra);
typedef void (*ITSpanTiled)(ITPixels *dst, ITPixels *src, ITPixels *mask, unsigned char *dstRow,
                            unsigned char *srcRow, unsigned char *maskRow, ULong height,
                            ULong width, ULong yPhase, ULong xPhase, ULong srcHeight,
                            ULong srcWidth, ULong *dstClut, unsigned char *inverse,
                            ULong *srcClut, ULong extra);

static ITPoint gDT_TileOrigin;                        /* 0x00865b08 the default tiling origin */

/* ---- one pixel, any format ---- */

#define READ_PIXEL(pix, p, clut, out)                                           \
    {                                                                           \
        ITPixels *rp = (pix);                                                   \
        ULong *rc = (clut);                                                     \
        unsigned char *rq = (p);                                                \
        ULong rv;                                                               \
        switch (rp->bytesPerPixel) {                                            \
        case 1: rv = rc[*rq]; break;                                            \
        case 2: rv = *(unsigned short *)rq; break;                              \
        case 3: rv = rq[0]; rv |= rq[1] << 8; rv |= rq[2] << 16; break;         \
        case 4: rv = *(ULong *)rq; break;                                       \
        }                                                                       \
        out = rv;                                                               \
    }

/* channel n (0 alpha, 1 red, 2 green, 3 blue) of a pixel, widened to 8 bits, into x */
#define CHANNEL_INTO(x, pix, v, n)                                              \
    x = (v) >> (pix)->shift[n];                                                 \
    x &= (pix)->mask[n];                                                        \
    x = (unsigned short)x * (pix)->scale[n] >> 8;

/* an 8-bit channel narrowed into its place in the pixel */
#define FIELD(pix, c, n)    (((c) >> (8 - (pix)->bits[n])) << (pix)->shift[n])

#define SOURCE_RGB()                                                            \
    CHANNEL_INTO(r, src, v, 1)                                                  \
    CHANNEL_INTO(g, src, v, 2)                                                  \
    CHANNEL_INTO(b, src, v, 3)

#define DEST_RGB()                                                              \
    CHANNEL_INTO(dr, dst, dv, 1)                                                \
    CHANNEL_INTO(dg, dst, dv, 2)                                                \
    CHANNEL_INTO(db, dst, dv, 3)

/* the new pixel goes through dv, the destination's value: the inverse table's index or the
   packed pixel (the alpha field, then red, green and blue) */
#define WRITE_PIXEL(p, r, g, b, alpha)                                          \
    if (dstClut != NULL) {                                                      \
        dv = ((r) & 0xf0) << 4 | ((g) & 0xf0) | ((b) & 0xf0) >> 4;              \
        *(p) = inverse[dv];                                                     \
    } else {                                                                    \
        dv = (alpha) | FIELD(dst, r, 1) | FIELD(dst, g, 2) | FIELD(dst, b, 3);  \
        for (i = 0; i < dst->bytesPerPixel; ) {                                 \
            (p)[i++] = (unsigned char)dv;                                       \
            dv >>= 8;                                                           \
        }                                                                       \
    }

/* the destination channel moved toward the source by a / 256 */
#define BLEND_CHANNEL(dc, sc)                                                   \
    sc -= dc;                                                                   \
    dc += (short)a * (short)sc / 256;

/* the same, the source channel kept (the 555 / 565 fast paths) */
#define BLEND_CHANNEL_KEEP(dc, sc)                                              \
    dc += (short)a * (short)(sc - dc) / 256;

#define BLEND_RGB_KEEP()                                                        \
    BLEND_CHANNEL_KEEP(dr, r)                                                   \
    BLEND_CHANNEL_KEEP(dg, g)                                                   \
    BLEND_CHANNEL_KEEP(db, b)

/* the blue (and, in the Tiled Opaque and the MaskBlend Keep spans, the red) source channel reaches
   its blend through a copy that coalesces away: the eliminated mov's weight is added to the
   channel's spill cost without an instruction, which is what puts the exe's spills in its order */
#define BLEND_RGB()                                                             \
    BLEND_CHANNEL(dr, r)                                                        \
    BLEND_CHANNEL(dg, g)                                                        \
    sb = b;                                                                     \
    BLEND_CHANNEL(db, sb)
#define BLEND_RGB_SR()                                                          \
    sr = r;                                                                     \
    BLEND_CHANNEL(dr, sr)                                                       \
    BLEND_CHANNEL(dg, g)                                                        \
    sb = b;                                                                     \
    BLEND_CHANNEL(db, sb)

/* a coverage of 0..255 taken to 0..256 */
#define FULL_SCALE(a)       ((a) + ((a) >> 7 & 1))

/* ---- the per-pixel operations; Opaque writes the destination's alpha bits full, Keep
   keeps the destination's own alpha, Set writes the coverage as the alpha ---- */

#define PIXEL_COPY_OPAQUE                                                       \
    READ_PIXEL(src, s, srcClut, v);                                             \
    a = 0xff;                                                                   \
    SOURCE_RGB();                                                               \
    da = a; dr = r; dg = g; db = b;                                             \
    WRITE_PIXEL(d, dr, dg, db, alphaBits);

#define PIXEL_SET(coverage)                                                     \
    READ_PIXEL(src, s, srcClut, v);                                             \
    coverage                                                                    \
    SOURCE_RGB();                                                               \
    da = a; dr = r; dg = g; db = b;                                             \
    WRITE_PIXEL(d, dr, dg, db, FIELD(dst, da, 0));

#define PIXEL_COVER(coverage, dalpha, alpha, blend)                             \
    READ_PIXEL(src, s, srcClut, v);                                             \
    coverage                                                                    \
    if (a != 0) {                                                               \
        SOURCE_RGB();                                                           \
        READ_PIXEL(dst, d, dstClut, dv);                                        \
        dalpha                                                                  \
        a = FULL_SCALE(a);                                                      \
        DEST_RGB();                                                             \
        blend();                                                                \
        WRITE_PIXEL(d, dr, dg, db, alpha);                                      \
    }

#define PIXEL_BLEND(dalpha, alpha, blend)                                       \
    READ_PIXEL(src, s, srcClut, v);                                             \
    a = extra;                                                                  \
    SOURCE_RGB();                                                               \
    READ_PIXEL(dst, d, dstClut, dv);                                            \
    dalpha                                                                      \
    DEST_RGB();                                                                 \
    blend();                                                                    \
    WRITE_PIXEL(d, dr, dg, db, alpha);

#define PIXEL_KEY(alpha)                                                        \
    READ_PIXEL(src, s, srcClut, v);                                             \
    a = 0xff;                                                                   \
    if (srcClut != NULL) {                                                      \
        if (*s == extra)                                                        \
            goto skip;                                                          \
    } else if ((v & keyMask) == extra)                                          \
        goto skip;                                                              \
    SOURCE_RGB();                                                               \
    da = a; dr = r; dg = g; db = b;                                             \
    WRITE_PIXEL(d, dr, dg, db, alpha);                                          \
skip:

#define DEST_ALPHA          FIELD(dst, da, 0)
#define DA_OPAQUE           da = 0xff;
#define DA_KEEP             CHANNEL_INTO(da, dst, dv, 0)
#define COV_OPAQUE          a = 0xff;
#define COV_MASK            a = *m;
#define COV_ALPHA           CHANNEL_INTO(a, src, v, 0)
#define COV_EXTRA           a = extra;
#define COV_MASKBLEND       a = FULL_SCALE((unsigned short)*m) * FULL_SCALE((unsigned short)extra) / 256;

#define PIXEL_COPY_SET          PIXEL_SET(COV_OPAQUE)
#define PIXEL_MASK_OPAQUE       PIXEL_COVER(COV_MASK, DA_OPAQUE, alphaBits, BLEND_RGB)
#define PIXEL_MASK_OPAQUE_T     PIXEL_COVER(COV_MASK, DA_OPAQUE, alphaBits, BLEND_RGB_SR)
#define PIXEL_MASK_KEEP         PIXEL_COVER(COV_MASK, DA_KEEP, DEST_ALPHA, BLEND_RGB)
#define PIXEL_MASK_SET          PIXEL_SET(COV_MASK)
#define PIXEL_ALPHA_OPAQUE      PIXEL_COVER(COV_ALPHA, DA_OPAQUE, alphaBits, BLEND_RGB)
#define PIXEL_ALPHA_OPAQUE_T    PIXEL_COVER(COV_ALPHA, DA_OPAQUE, alphaBits, BLEND_RGB_SR)
#define PIXEL_ALPHA_KEEP        PIXEL_COVER(COV_ALPHA, DA_KEEP, DEST_ALPHA, BLEND_RGB)
#define PIXEL_ALPHA_SET         PIXEL_SET(COV_ALPHA)
#define PIXEL_BLEND_OPAQUE      PIXEL_BLEND(DA_OPAQUE, alphaBits, BLEND_RGB)
#define PIXEL_BLEND_OPAQUE_T    PIXEL_BLEND(DA_OPAQUE, alphaBits, BLEND_RGB_SR)
#define PIXEL_BLEND_KEEP        PIXEL_BLEND(DA_KEEP, DEST_ALPHA, BLEND_RGB)
#define PIXEL_BLEND_SET         PIXEL_SET(COV_EXTRA)
#define PIXEL_MASKBLEND_OPAQUE  PIXEL_COVER(COV_MASKBLEND, DA_OPAQUE, alphaBits, BLEND_RGB)
#define PIXEL_MASKBLEND_KEEP    PIXEL_COVER(COV_MASKBLEND, DA_KEEP, DEST_ALPHA, BLEND_RGB_SR)
#define PIXEL_MASKBLEND_SET     PIXEL_SET(COV_MASKBLEND)
#define PIXEL_KEY_OPAQUE        PIXEL_KEY(alphaBits)
#define PIXEL_KEY_KEEP          PIXEL_KEY(DEST_ALPHA)
#define PIXEL_KEY_SET           PIXEL_SET(COV_OPAQUE)

/* ---- same-format fast paths (source and destination share key and depth) ---- */

#define PIXEL_COPY_8        v = *s; a = 0xff; dv = v; *d = (unsigned char)v;
#define PIXEL_COPY_16       v = *(unsigned short *)s; dv = v; a = 0xff; v |= alphaBits; *(unsigned short *)d = v;
#define PIXEL_COPY_24                                                           \
    v = *(ULong *)s; dv = v; a = 0xff; v = v | alphaBits;                       \
    for (i = 0; i < dst->bytesPerPixel; ) {                                     \
        d[i++] = (unsigned char)v;                                              \
        v >>= 8;                                                                \
    }
#define PIXEL_COPY_32       v = *(ULong *)s; dv = v; a = 0xff; v |= alphaBits; *(ULong *)d = v;

#define PIXEL_KEY_8         v = *s; a = 0xff; if (*s != extra) { dv = v; *d = (unsigned char)v; }
#define KEY_SETUP_15        keyMask = 0x7fff;
#define KEY_SETUP_16        keyMask = 0xffff;
#define KEY_SETUP_24        keyMask = 0xffffff00;
#define KEY_SETUP_32        keyMask = 0xffffff00;

#define PIXEL_KEY_16                                                            \
    v = *(unsigned short *)s; a = 0xff;                                         \
    if ((v & keyMask) != extra) {                                               \
        v |= alphaBits;                                                         \
        *(unsigned short *)d = v;                                               \
    }
#define PIXEL_KEY_24                                                            \
    v = *(ULong *)s; a = 0xff;                                                  \
    if ((v & keyMask) != extra) {                                               \
        v |= alphaBits;                                                         \
        for (i = 0; i < dst->bytesPerPixel; ) {                                 \
            d[i++] = v;                                                         \
            v >>= 8;                                                            \
        }                                                                       \
    }
#define PIXEL_KEY_32                                                            \
    v = *(ULong *)s; a = 0xff;                                                  \
    if ((v & keyMask) != extra) {                                               \
        v |= alphaBits;                                                         \
        *(ULong *)d = v;                                                        \
    }

/* the masked blend over one fixed layout: UNPACK splits a pixel into three channels in its
   own order, PACK stores them back */
#define PIXEL_MASK_FIXED(READ_SRC, READ_DST, UNPACK, BLEND, STORE)              \
    v = READ_SRC;                                                               \
    a = *m;                                                                     \
    if (a != 0) {                                                               \
        UNPACK(v, r, g, b, COPY_TO_WV);                                         \
        dv = READ_DST;                                                          \
        da = 0xff;                                                              \
        a = FULL_SCALE(a);                                                      \
        UNPACK(dv, dr, dg, db, NO_COPY);                                        \
        BLEND();                                                                \
        STORE;                                                                  \
    }
#define COPY_TO_WV(x)       wv = (x);
#define NO_COPY(x)

#define WIDEN5(x)           ((x) << 3 | (x) >> 2)
#define WIDEN6(x)           ((x) << 2 | (x) >> 4)

#define UNPACK_ARGB(p, c1, c2, c3, K)                                           \
    K(p) c1 = (p) >> 8 & 0xff; K(c1) c2 = (p) >> 16 & 0xff; K(c2) c3 = (p) >> 24 & 0xff; K(c3)
#define UNPACK_555(p, c1, c2, c3, K)                                            \
    K(p) c1 = (p) & 0x1f; c2 = (p) >> 5 & 0x1f; c3 = (p) >> 10 & 0x1f;                     \
    c1 = WIDEN5(c1); c2 = WIDEN5(c2); c3 = WIDEN5(c3);
#define UNPACK_565(p, c1, c2, c3, K)                                            \
    K(p) c1 = (p) & 0x1f; c2 = (p) >> 5 & 0x3f; c3 = (p) >> 11 & 0x1f;                     \
    c1 = WIDEN5(c1); c2 = WIDEN6(c2); c3 = WIDEN5(c3);
#define UNPACK_8888(p, c1, c2, c3, K)                                           \
    K(p) c1 = (p) & 0xff; K(c1) c2 = (p) >> 8 & 0xff; K(c2) c3 = (p) >> 16 & 0xff; K(c3)

#define STORE_INDEX                                                             \
    dv = (dr & 0xf0) << 4 | (dg & 0xf0) | (db & 0xf0) >> 4;                     \
    *d = inverse[dv]
#define STORE_555                                                               \
    dv = 0x8000 | (dr & 0x1f) | (dg & 0x1f) << 5 | (db & 0x1f) << 10;           \
    *(unsigned short *)d = dv
#define STORE_565                                                               \
    dv = (dr & 0x1f) | (dg & 0x3f) << 5 | (db & 0x1f) << 11;                    \
    *(unsigned short *)d = dv
#define STORE_24                                                                \
    dv = 0xff000000 | (dr & 0xff) | (dg & 0xff) << 8 | (db & 0xff) << 16;      \
    for (i = 0; i < dst->bytesPerPixel; ) {                                     \
        d[i++] = (unsigned char)dv;                                             \
        dv >>= 8;                                                               \
    }
#define STORE_32                                                                \
    dv = 0xff000000 | (dr & 0xff) | (dg & 0xff) << 8 | (db & 0xff) << 16;      \
    *(ULong *)d = dv

#define PIXEL_MASK_8        PIXEL_MASK_FIXED(srcClut[*s], dstClut[*d], UNPACK_ARGB, BLEND_RGB_KEEP, STORE_INDEX)
#define PIXEL_MASK_15       PIXEL_MASK_FIXED(*(unsigned short *)s, *(unsigned short *)d, UNPACK_555, BLEND_RGB_KEEP, STORE_555)
#define PIXEL_MASK_16       PIXEL_MASK_FIXED(*(unsigned short *)s, *(unsigned short *)d, UNPACK_565, BLEND_RGB_KEEP, STORE_565)
#define PIXEL_MASK_24       PIXEL_MASK_FIXED(*(ULong *)s, *(ULong *)d, UNPACK_ARGB, BLEND_RGB, STORE_24)
#define PIXEL_MASK_32       PIXEL_MASK_FIXED(*(ULong *)s, *(ULong *)d, UNPACK_8888, BLEND_RGB, STORE_32)

/* ---- the three frames ---- */

#define SPAN_LOCALS_R                                                           \
    ULong sb, sr, alpha, b, g, da, r;                                           \
    unsigned char *s, *m, *d;                                                   \
    long err;                                                                   \
    ULong x, y, keyMask, alphaBits, v, dv, i;                                   \
    ULong a, dr, dg, db, wv;
#define SPAN_LOCALS_R15                                                         \
    ULong alpha, b, g, da, r;                                                   \
    unsigned char *d, *s, *m;                                                   \
    long err;                                                                   \
    ULong x, y, keyMask, alphaBits, v, dv, i;                                   \
    ULong a, dr, dg, db, wv;
#define SPAN_LOCALS_S                                                           \
    ULong sb, sr, b, g, r;                                                      \
    long err;                                                                   \
    unsigned char *s, *m, *d;                                                   \
    ULong x, y, keyMask, alphaBits, ye, xe, v, dv, i;                           \
    ULong a, da, dr, dg, db, wv;
#define SPAN_LOCALS_T                                                           \
    ULong sb, sr, b, g, r;                                                      \
    unsigned char *s, *m, *d;                                                   \
    long err;                                                                   \
    ULong x, y, keyMask, alphaBits, v, dv, i;                                   \
    ULong da, dr, dg, db, wv, ty, tx, a;

#define NO_SETUP
#define NO_ALPHA
#define ALPHA_BITS          alphaBits = dst->mask[0] << dst->shift[0];
#define ALPHA_MASK          alpha = dst->mask[0] << dst->shift[0];
#define KEY_SETUP                                                               \
    keyMask = src->mask[1] << src->shift[1] | src->mask[2] << src->shift[2] |   \
              src->mask[3] << src->shift[3];

#define ANY_DST             dst->bytesPerPixel
#define ANY_SRC             src->bytesPerPixel

/* rows copied one for one */
#define ROWS_PARAMS                                                             \
    ITPixels *dst, ITPixels *src, ITPixels *mask, unsigned char *dstRow,        \
    unsigned char *srcRow, unsigned char *maskRow, ULong height, ULong width,   \
    ULong *dstClut, unsigned char *inverse, ULong *srcClut, ULong extra
#define ROWS_BODY(PIXEL, DSTEP, SSTEP, ALPHA, SETUP)                            \
    ROWS_BODY_L(SPAN_LOCALS_R, PIXEL, DSTEP, SSTEP, ALPHA, SETUP)
#define ROWS_BODY_L(LOCALS, PIXEL, DSTEP, SSTEP, ALPHA, SETUP)                  \
{                                                                               \
    LOCALS                                                                      \
    err = 0;                                                                    \
    s = srcRow;                                                                 \
    m = maskRow;                                                                \
    d = dstRow;                                                                 \
    ALPHA                                                                      \
    if (mask == NULL)                                                           \
        mask = src;                                                             \
    SETUP                                                                       \
    for (y = 0; y < height; y++) {                                              \
        for (x = 0; x < width; x++) {                                           \
            PIXEL                                                               \
            d += DSTEP;                                                         \
            s += SSTEP;                                                         \
            m++;                                                                \
        }                                                                       \
        dstRow += dst->rowBytes;                                                \
        d = dstRow;                                                             \
        srcRow += src->rowBytes;                                                \
        maskRow += mask->rowBytes;                                              \
        s = srcRow;                                                             \
        m = maskRow;                                                            \
    }                                                                           \
}

/* rows stretched: the error terms step the source by srcWidth / dstWidth */
#define SCALED_PARAMS                                                           \
    ITPixels *dst, ITPixels *src, ITPixels *mask, unsigned char *dstRow,        \
    unsigned char *srcRow, unsigned char *maskRow, ULong height, ULong width,   \
    ULong yErr, ULong xErr, ULong dstHeight, ULong dstWidth, ULong srcHeight,   \
    ULong srcWidth, ULong *dstClut, unsigned char *inverse, ULong *srcClut,     \
    ULong extra
#define SCALED_BODY(PIXEL, DSTEP, SSTEP, ALPHA, SETUP)                          \
    SCALED_BODY_L(SPAN_LOCALS_S, PIXEL, DSTEP, SSTEP, ALPHA, SETUP)
#define SCALED_BODY_L(LOCALS, PIXEL, DSTEP, SSTEP, ALPHA, SETUP)                \
{                                                                               \
    LOCALS                                                                      \
    err = 0;                                                                    \
    s = srcRow;                                                                 \
    m = maskRow;                                                                \
    d = dstRow;                                                                 \
    ALPHA                                                                      \
    if (mask == NULL)                                                           \
        mask = src;                                                             \
    SETUP                                                                       \
    ye = yErr;                                                                  \
    for (y = 0; y < height; y++) {                                              \
        xe = xErr;                                                              \
        for (x = 0; x < width; x++) {                                           \
            PIXEL                                                               \
            d += DSTEP;                                                         \
            for (xe += srcWidth; xe >= dstWidth; xe -= width) {                 \
                s += SSTEP;                                                     \
                m++;                                                            \
            }                                                                   \
        }                                                                       \
        dstRow += dst->rowBytes;                                                \
        d = dstRow;                                                             \
        for (ye += srcHeight; ye >= dstHeight; ye -= height) {                  \
            srcRow += src->rowBytes;                                            \
            maskRow += mask->rowBytes;                                          \
        }                                                                       \
        s = srcRow;                                                             \
        m = maskRow;                                                            \
    }                                                                           \
}

/* the source repeated: its rows and columns wrap at srcHeight / srcWidth */
#define TILED_PARAMS                                                            \
    ITPixels *dst, ITPixels *src, ITPixels *mask, unsigned char *dstRow,        \
    unsigned char *srcRow, unsigned char *maskRow, ULong height, ULong width,   \
    ULong yPhase, ULong xPhase, ULong srcHeight, ULong srcWidth,                \
    ULong *dstClut, unsigned char *inverse, ULong *srcClut, ULong extra
#define TILED_BODY(PIXEL, DSTEP, SSTEP, ALPHA, SETUP)                           \
    TILED_BODY_L(SPAN_LOCALS_T, PIXEL, DSTEP, SSTEP, ALPHA, SETUP)
#define TILED_BODY_L(LOCALS, PIXEL, DSTEP, SSTEP, ALPHA, SETUP)                 \
{                                                                               \
    LOCALS                                                                      \
    err = 0;                                                                    \
    s = srcRow;                                                                 \
    m = maskRow;                                                                \
    d = dstRow;                                                                 \
    ALPHA                                                                      \
    if (mask == NULL)                                                           \
        mask = src;                                                             \
    SETUP                                                                       \
    ty = yPhase;                                                                \
    for (y = 0; y < height; y++) {                                              \
        tx = xPhase;                                                            \
        for (x = 0; x < width; x++) {                                           \
            PIXEL                                                               \
            d += DSTEP;                                                         \
            s += SSTEP;                                                         \
            m++;                                                                \
            if (++tx == srcWidth) {                                             \
                s -= srcWidth * src->bytesPerPixel;                             \
                m -= srcWidth;                                                  \
                tx = 0;                                                         \
            }                                                                   \
        }                                                                       \
        dstRow += dst->rowBytes;                                                \
        d = dstRow;                                                             \
        srcRow += src->rowBytes;                                                \
        maskRow += mask->rowBytes;                                              \
        s = srcRow;                                                             \
        m = maskRow;                                                            \
        if (++ty == srcHeight) {                                                \
            srcRow -= srcHeight * src->rowBytes;                                \
            s = srcRow;                                                         \
            maskRow -= srcHeight * mask->rowBytes;                              \
            m = maskRow;                                                        \
            ty = 0;                                                             \
        }                                                                       \
    }                                                                           \
}

/* ---- the span routines, in link order ---- */

/* The 76 spans (Span<frame><operation><alpha or same-format depth>). STATE: MATCH byte-identical,
   75 of 78: every span but Rows Mask8 (77, colour: the exe pushes r and the blue group in the first
   simplify scan, and every dest group last). OPEN: that one; the sr / sb blend copies and the
   per-span list SPAN_LOCALS_R15 are the levers; one block for the family (notes/itblit.md) */
// FUNCTION: EVNOVA 0x004dbd6a
void _DT_ImageBlitNoneIgnore(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dbfcc
void _DT_ImageBlitNoneIgnore_8(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_8, 1, 1, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc069
void _DT_ImageBlitNoneIgnore_15(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_16, 2, 2, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc110
void _DT_ImageBlitNoneIgnore_16(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_16, 2, 2, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc1b7
void _DT_ImageBlitNoneIgnore_24(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_24, 3, 3, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc26f
void _DT_ImageBlitNoneIgnore_32(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_32, 4, 4, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc314
void _DT_ImageBlitCopyIgnore(ROWS_PARAMS)
ROWS_BODY(PIXEL_COPY_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc55c
void _DT_ImageScaleNoneIgnore(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc80e
void _DT_ImageScaleNoneIgnore_8(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_8, 1, 1, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc8f6
void _DT_ImageScaleNoneIgnore_15(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_16, 2, 2, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dc9f8
void _DT_ImageScaleNoneIgnore_16(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_16, 2, 2, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dcafa
void _DT_ImageScaleNoneIgnore_24(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_24, 3, 3, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dcc0d
void _DT_ImageScaleNoneIgnore_32(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_32, 4, 4, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dcd0d
void _DT_ImageScaleCopyIgnore(SCALED_PARAMS)
SCALED_BODY(PIXEL_COPY_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004dcfa9
void _DT_ImageTileNoneIgnore(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dd278
void _DT_ImageTileNoneIgnore_8(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_8, 1, 1, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dd37e
void _DT_ImageTileNoneIgnore_15(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_16, 2, 2, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dd489
void _DT_ImageTileNoneIgnore_16(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_16, 2, 2, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dd594
void _DT_ImageTileNoneIgnore_24(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_24, 3, 3, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dd6b4
void _DT_ImageTileNoneIgnore_32(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_32, 4, 4, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dd7bd
void _DT_ImageTileCopyIgnore(TILED_PARAMS)
TILED_BODY(PIXEL_COPY_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004dda76
void _DT_ImageBlitNoneBitmap(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASK_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dde59
void _DT_ImageBlitNoneBitmap_8(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASK_8, 1, 1, ALPHA_MASK, NO_SETUP)
// FUNCTION: EVNOVA 0x004de038
void _DT_ImageBlitNoneBitmap_15(ROWS_PARAMS)
ROWS_BODY_L(SPAN_LOCALS_R15, PIXEL_MASK_15, 2, 2, ALPHA_MASK, NO_SETUP)
// FUNCTION: EVNOVA 0x004de233
void _DT_ImageBlitNoneBitmap_16(ROWS_PARAMS)
ROWS_BODY_L(SPAN_LOCALS_R15, PIXEL_MASK_16, 2, 2, ALPHA_MASK, NO_SETUP)
// FUNCTION: EVNOVA 0x004de429
void _DT_ImageBlitNoneBitmap_24(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASK_24, 3, 3, ALPHA_MASK, NO_SETUP)
// FUNCTION: EVNOVA 0x004de603
void _DT_ImageBlitNoneBitmap_32(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASK_32, 4, 4, ALPHA_MASK, NO_SETUP)
// FUNCTION: EVNOVA 0x004de7c7
void _DT_ImageScaleNoneBitmap(SCALED_PARAMS)
SCALED_BODY(PIXEL_MASK_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004dec1b
void _DT_ImageTileNoneBitmap(TILED_PARAMS)
TILED_BODY(PIXEL_MASK_OPAQUE_T, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004df096
void _DT_ImageBlitSaveBitmap(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASK_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004df4bd
void _DT_ImageScaleSaveBitmap(SCALED_PARAMS)
SCALED_BODY(PIXEL_MASK_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004df955
void _DT_ImageTileSaveBitmap(TILED_PARAMS)
TILED_BODY(PIXEL_MASK_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004dfe0d
void _DT_ImageBlitCopyBitmap(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASK_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e006b
void _DT_ImageScaleCopyBitmap(SCALED_PARAMS)
SCALED_BODY(PIXEL_MASK_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e0326
void _DT_ImageTileCopyBitmap(TILED_PARAMS)
TILED_BODY(PIXEL_MASK_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e05f5
void _DT_ImageBlitNoneInline(ROWS_PARAMS)
ROWS_BODY(PIXEL_ALPHA_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e09f8
void _DT_ImageScaleNoneInline(SCALED_PARAMS)
SCALED_BODY(PIXEL_ALPHA_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e0e6c
void _DT_ImageTileNoneInline(TILED_PARAMS)
TILED_BODY(PIXEL_ALPHA_OPAQUE_T, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e1307
void _DT_ImageBlitSaveInline(ROWS_PARAMS)
ROWS_BODY(PIXEL_ALPHA_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e174c
void _DT_ImageScaleSaveInline(SCALED_PARAMS)
SCALED_BODY(PIXEL_ALPHA_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e1c02
void _DT_ImageTileSaveInline(TILED_PARAMS)
TILED_BODY(PIXEL_ALPHA_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e20d8
void _DT_ImageBlitCopyInline(ROWS_PARAMS)
ROWS_BODY(PIXEL_ALPHA_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e2356
void _DT_ImageScaleCopyInline(SCALED_PARAMS)
SCALED_BODY(PIXEL_ALPHA_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e2631
void _DT_ImageTileCopyInline(TILED_PARAMS)
TILED_BODY(PIXEL_ALPHA_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e2920
void _DT_ImageBlitNoneUniform(ROWS_PARAMS)
ROWS_BODY(PIXEL_BLEND_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e2cf8
void _DT_ImageScaleNoneUniform(SCALED_PARAMS)
SCALED_BODY(PIXEL_BLEND_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e3141
void _DT_ImageTileNoneUniform(TILED_PARAMS)
TILED_BODY(PIXEL_BLEND_OPAQUE_T, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e35ad
void _DT_ImageBlitSaveUniform(ROWS_PARAMS)
ROWS_BODY(PIXEL_BLEND_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e39aa
void _DT_ImageScaleSaveUniform(SCALED_PARAMS)
SCALED_BODY(PIXEL_BLEND_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e3e1b
void _DT_ImageTileSaveUniform(TILED_PARAMS)
TILED_BODY(PIXEL_BLEND_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e42af
void _DT_ImageBlitCopyUniform(ROWS_PARAMS)
ROWS_BODY(PIXEL_BLEND_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e44f4
void _DT_ImageScaleCopyUniform(SCALED_PARAMS)
SCALED_BODY(PIXEL_BLEND_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e4796
void _DT_ImageTileCopyUniform(TILED_PARAMS)
TILED_BODY(PIXEL_BLEND_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e4a4c
void _DT_ImageBlitNoneCombined(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASKBLEND_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e4e5b
void _DT_ImageScaleNoneCombined(SCALED_PARAMS)
SCALED_BODY(PIXEL_MASKBLEND_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e52db
void _DT_ImageTileNoneCombined(TILED_PARAMS)
TILED_BODY(PIXEL_MASKBLEND_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e577e
void _DT_ImageBlitSaveCombined(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASKBLEND_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e5bc3
void _DT_ImageScaleSaveCombined(SCALED_PARAMS)
SCALED_BODY(PIXEL_MASKBLEND_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e6079
void _DT_ImageTileSaveCombined(TILED_PARAMS)
TILED_BODY(PIXEL_MASKBLEND_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e654f
void _DT_ImageBlitCopyCombined(ROWS_PARAMS)
ROWS_BODY(PIXEL_MASKBLEND_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e67df
void _DT_ImageScaleCopyCombined(SCALED_PARAMS)
SCALED_BODY(PIXEL_MASKBLEND_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e6acb
void _DT_ImageTileCopyCombined(TILED_PARAMS)
TILED_BODY(PIXEL_MASKBLEND_SET, ANY_DST, ANY_SRC, NO_ALPHA, NO_SETUP)
// FUNCTION: EVNOVA 0x004e6dcb
void _DT_ImageBlitNoneColorKey(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e708a
void _DT_ImageBlitNoneColorKey_8(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_8, 1, 1, ALPHA_BITS, NO_SETUP)
// FUNCTION: EVNOVA 0x004e7135
void _DT_ImageBlitNoneColorKey_15(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_16, 2, 2, ALPHA_BITS, KEY_SETUP_15)
// FUNCTION: EVNOVA 0x004e71f4
void _DT_ImageBlitNoneColorKey_16(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_16, 2, 2, ALPHA_BITS, KEY_SETUP_16)
// FUNCTION: EVNOVA 0x004e72b3
void _DT_ImageBlitNoneColorKey_24(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_24, 3, 3, ALPHA_BITS, KEY_SETUP_24)
// FUNCTION: EVNOVA 0x004e7389
void _DT_ImageBlitNoneColorKey_32(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_32, 4, 4, ALPHA_BITS, KEY_SETUP_32)
// FUNCTION: EVNOVA 0x004e7446
void _DT_ImageScaleNoneColorKey(SCALED_PARAMS)
SCALED_BODY(PIXEL_KEY_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e7767
void _DT_ImageTileNoneColorKey(TILED_PARAMS)
TILED_BODY(PIXEL_KEY_OPAQUE, ANY_DST, ANY_SRC, ALPHA_BITS, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e7a99
void _DT_ImageBlitSaveColorKey(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e7d39
void _DT_ImageScaleSaveColorKey(SCALED_PARAMS)
SCALED_BODY(PIXEL_KEY_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e803f
void _DT_ImageTileSaveColorKey(TILED_PARAMS)
TILED_BODY(PIXEL_KEY_KEEP, ANY_DST, ANY_SRC, NO_ALPHA, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e8356
void _DT_ImageBlitCopyColorKey(ROWS_PARAMS)
ROWS_BODY(PIXEL_KEY_SET, ANY_DST, ANY_SRC, NO_ALPHA, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e85c2
void _DT_ImageScaleCopyColorKey(SCALED_PARAMS)
SCALED_BODY(PIXEL_KEY_SET, ANY_DST, ANY_SRC, NO_ALPHA, KEY_SETUP)
// FUNCTION: EVNOVA 0x004e8882
void _DT_ImageTileCopyColorKey(TILED_PARAMS)
TILED_BODY(PIXEL_KEY_SET, ANY_DST, ANY_SRC, NO_ALPHA, KEY_SETUP)

/* ---- the dispatch tables ---- */

/* 0x00576de8: a depth's same-format column (0 none; 1 is the Set column) */
static const ULong gSpanColumn[33] = {
    0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 3,
    4, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
    6
};

/* 0x00576e6c: seven zero words the exe keeps between the column table and the tables below */
static const ULong gSpanSpare[7] = { 0 };

#define SAME_FORMAT_COLUMNS(none, copy)                                       \
    none, copy, none##_8, none##_15, none##_16, none##_24, none##_32

/* 0x00576e88 .. 0x00576ef8 */
static const ITSpanRows gRowsCopy[7] = { SAME_FORMAT_COLUMNS(_DT_ImageBlitNoneIgnore, _DT_ImageBlitCopyIgnore) };
static const ITSpanTiled gTiledCopy[7] = { SAME_FORMAT_COLUMNS(_DT_ImageTileNoneIgnore, _DT_ImageTileCopyIgnore) };
static const ITSpanScaled gScaledCopy[7] = { SAME_FORMAT_COLUMNS(_DT_ImageScaleNoneIgnore, _DT_ImageScaleCopyIgnore) };
static const ITSpanRows gRowsMask[7] = { SAME_FORMAT_COLUMNS(_DT_ImageBlitNoneBitmap, _DT_ImageBlitCopyBitmap) };
static const ITSpanRows gRowsKey[7] = { SAME_FORMAT_COLUMNS(_DT_ImageBlitNoneColorKey, _DT_ImageBlitCopyColorKey) };

/* the column for a source over a destination: its same-format fast path, else 0 */
#define SPAN_COLUMN(src, dst)                                                   \
    gSpanColumn[(src)->key == (dst)->key && (src)->depth == (dst)->depth ? (src)->depth : 0]

#define ROWS_ARGS(extra)                                                        \
    (dstPixels, srcPixels, maskPixels, dstRow, srcRow, maskRow, height, width,  \
     dstClut, inverse, srcClut, extra)
#define SCALED_ARGS(extra)                                                      \
    (dstPixels, srcPixels, maskPixels, dstRow, srcRow, maskRow, height, width,  \
     yErr, xErr, dstHeight, dstWidth, yDen, xDen, dstClut, inverse, srcClut, extra)
#define TILED_ARGS(extra)                                                       \
    (dstPixels, srcPixels, maskPixels, dstRow, srcRow, maskRow, height, width,  \
     yPhase, xPhase, srcHeight, srcWidth, dstClut, inverse, srcClut, extra)

/* a key colour in the source's own format */
#define KEY_INTO(v, pix, t)                                                     \
    v = ((t)->key.green >> (8 - (pix)->bits[2])) << (pix)->shift[2];            \
    v |= ((t)->key.red >> (8 - (pix)->bits[1])) << (pix)->shift[1];             \
    v |= ((t)->key.blue >> (8 - (pix)->bits[3])) << (pix)->shift[3];

/* DT_ImageCopy (draw src, through its mask or a transparency, from srcRect into dstRect of
   dst; stretched when the two sizes differ; `copy' carries the source's coverage into the
   destination's alpha). STATE: MATCH 100%, exe 854 / obj 854, byte-identical yes. OPEN: none;
   srcColumn is a write-only holder whose declaration slot numbers the group of srcPixels->
   bytesPerPixel's CSE below dstHeight, and dstWidth is declared first for the same tie (notes/itblit.md) */
// FUNCTION: EVNOVA 0x004e8b5f
long DT_ImageCopy(ITImage *dst, const ITRect *dstRect, unsigned char copy, ITImage *src,
                  ITImage *srcMask, const ITTransparent *trans, const ITRect *srcRect)
{
    ITPixels *dstPixels;
    ITPixels *srcPixels;
    ITRect dr;
    ITRect sr;
    long e;
    ULong kind;
    unsigned int dstWidth, dstHeight, srcWidth, srcHeight;
    ULong height, width;
    unsigned char *srcColumn;
    ITPixels *maskPixels;
    long err;
    ULong yErr, yDen, xErr, xDen;
    ULong column;
    ULong key;
    unsigned char alpha;
    unsigned char *dstRow;
    unsigned char *srcRow;
    ULong *dstClut;
    ULong *srcClut;
    unsigned char *inverse;
    unsigned char scaled;
    unsigned char dstLocked;
    unsigned char srcLocked;
    unsigned char *maskRow;

    dstPixels = _DT_ImageGetImage(dst);
    srcPixels = _DT_ImageGetImage(src);
    maskPixels = _DT_ImageGetAlpha(srcMask);
    err = 0;
    yErr = 0;
    yDen = 0;
    xErr = 0;
    xDen = 0;
    alpha = kAlphaOpaque;
    dstClut = NULL;
    srcClut = NULL;
    inverse = NULL;
    scaled = 0;
    dstLocked = 0;
    srcLocked = 0;
    if (maskPixels != NULL)
        DT_IsRectEqual(&srcPixels->bounds, &maskPixels->bounds);

    if (trans != NULL) {
        kind = trans->mode;
        switch (kind) {
        case kBlitBlend:
            if (trans->key.alpha == 0)
                goto done;
            if (trans->key.alpha == kKeyAlphaNone)
                kind = kBlitCopy;
            break;
        case kBlitMaskBlend:
            if (trans->key.alpha == 0)
                goto done;
            if (trans->key.alpha == kKeyAlphaNone)
                kind = kBlitMask;
            break;
        case kBlitCopy:
        case kBlitMask:
        case kBlitAlpha:
        case kBlitKey:
            break;
        }
        goto chosen;
    }
    if (maskPixels != NULL) {
        kind = kBlitMask;
        goto chosen;
    }
    if (srcPixels->bits[0] != 0) {
        kind = kBlitAlpha;
        goto chosen;
    }
    kind = kBlitCopy;
chosen:

    if ((copy && maskPixels != NULL) ||
        (copy && trans != NULL && trans->mode == kBlitCopy) ||
        (copy && trans != NULL && trans->mode == kBlitAlpha) ||
        (copy && trans == NULL && srcPixels->bits[0] != 0))
        alpha = kAlphaSet;
    else if (!copy && dstPixels->bits[0] != 0)
        alpha = kAlphaKeep;

    if (dstRect == NULL)
        dstRect = &dstPixels->bounds;
    if (srcRect == NULL)
        srcRect = &srcPixels->bounds;
    dr = *dstRect;
    sr = *srcRect;
    if (sr.bottom < srcPixels->bounds.top || sr.top > srcPixels->bounds.bottom ||
        sr.right < srcPixels->bounds.left || sr.left > srcPixels->bounds.right)
        goto done;
    if (dr.bottom < dstPixels->bounds.top || dr.top > dstPixels->bounds.bottom ||
        dr.right < dstPixels->bounds.left || dr.left > dstPixels->bounds.right)
        goto done;

    e = DT_ImageLock(dst, NULL, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    dstLocked = 1;
    e = DT_ImageLock(src, srcMask, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    srcLocked = 1;

    if (dstPixels->bytesPerPixel == kIndexedBytes) {
        dstClut = (ULong *)dstPixels->palette.entry;
        inverse = dstPixels->inverse;
    }
    if (srcPixels->bytesPerPixel == kIndexedBytes)
        srcClut = (ULong *)srcPixels->palette.entry;

    /* clip the source to its map, moving the destination in proportion, then the reverse */
    if (sr.top < srcPixels->bounds.top) {
        dr.top += (dstRect->bottom - dstRect->top) * (srcPixels->bounds.top - sr.top) /
                  (srcRect->bottom - srcRect->top);
        sr.top = srcPixels->bounds.top;
    }
    if (sr.left < srcPixels->bounds.left) {
        dr.left += (dstRect->right - dstRect->left) * (srcPixels->bounds.left - sr.left) /
                   (srcRect->right - srcRect->left);
        sr.left = srcPixels->bounds.left;
    }
    if (sr.bottom > srcPixels->bounds.bottom) {
        dr.bottom -= (dstRect->bottom - dstRect->top) * (sr.bottom - srcPixels->bounds.bottom) /
                     (srcRect->bottom - srcRect->top);
        sr.bottom = srcPixels->bounds.bottom;
    }
    if (sr.right > srcPixels->bounds.right) {
        dr.right -= (dstRect->right - dstRect->left) * (sr.right - srcPixels->bounds.right) /
                    (srcRect->right - srcRect->left);
        sr.right = srcPixels->bounds.right;
    }
    if (dr.top < dstPixels->bounds.top) {
        sr.top += (srcRect->bottom - srcRect->top) * (dstPixels->bounds.top - dr.top) /
                  (dstRect->bottom - dstRect->top);
        dr.top = dstPixels->bounds.top;
    }
    if (dr.left < dstPixels->bounds.left) {
        sr.left += (srcRect->right - srcRect->left) * (dstPixels->bounds.left - dr.left) /
                   (dstRect->right - dstRect->left);
        dr.left = dstPixels->bounds.left;
    }
    if (dr.bottom > dstPixels->bounds.bottom) {
        sr.bottom -= (srcRect->bottom - srcRect->top) * (dr.bottom - dstPixels->bounds.bottom) /
                     (dstRect->bottom - dstRect->top);
        dr.bottom = dstPixels->bounds.bottom;
    }
    if (dr.right > dstPixels->bounds.right) {
        sr.right -= (srcRect->right - srcRect->left) * (dr.right - dstPixels->bounds.right) /
                    (dstRect->right - dstRect->left);
        dr.right = dstPixels->bounds.right;
    }

    /* the error terms start mid-pixel, advanced to the clipped first row and column */
    srcHeight = srcRect->bottom - srcRect->top;
    dstHeight = dstRect->bottom - dstRect->top;
    if (dstHeight != srcHeight) {
        yErr = yDen = srcHeight;
        yErr >>= 1;
        yErr += (dr.top - dstRect->top) * srcHeight;
        yErr %= dstHeight;
        scaled = 1;
    }
    srcWidth = srcRect->right - srcRect->left;
    dstWidth = dstRect->right - dstRect->left;
    if (dstWidth != srcWidth) {
        xErr = xDen = srcWidth;
        xErr >>= 1;
        xErr += (dr.left - dstRect->left) * srcWidth;
        xErr %= dstWidth;
        scaled = 1;
    }

    dstRow = (unsigned char *)dstPixels->baseAddr +
             (dr.left - dstPixels->bounds.left) * dstPixels->bytesPerPixel +
             (dr.top - dstPixels->bounds.top) * dstPixels->rowBytes;
    srcRow = (srcColumn = (unsigned char *)srcPixels->baseAddr +
              (sr.left - srcPixels->bounds.left) * srcPixels->bytesPerPixel) +
             (sr.top - srcPixels->bounds.top) * srcPixels->rowBytes;
    maskRow = !maskPixels ? NULL :
              (unsigned char *)maskPixels->baseAddr +
              (sr.left - srcPixels->bounds.left) +
              (sr.top - srcPixels->bounds.top) * maskPixels->rowBytes;
    height = dr.bottom - dr.top;
    width = dr.right - dr.left;

    switch (kind) {
    case kBlitCopy:
        column = alpha == kAlphaSet ? 1 : SPAN_COLUMN(srcPixels, dstPixels);
        if (!scaled)
            gRowsCopy[column] ROWS_ARGS(0);
        else
            gScaledCopy[column] SCALED_ARGS(0);
        goto done;
    case kBlitMask:
        if (!scaled && alpha == kAlphaOpaque) {
            column = SPAN_COLUMN(srcPixels, dstPixels);
            gRowsMask[column] ROWS_ARGS(0);
        } else if (!scaled) {
            ITSpanRows spans[3] = { _DT_ImageBlitNoneBitmap, _DT_ImageBlitSaveBitmap, _DT_ImageBlitCopyBitmap };
            spans[alpha] ROWS_ARGS(0);
        } else {
            ITSpanScaled spans[3] = { _DT_ImageScaleNoneBitmap, _DT_ImageScaleSaveBitmap,
                                      _DT_ImageScaleCopyBitmap };
            spans[alpha] SCALED_ARGS(0);
        }
        goto done;
    case kBlitAlpha:
        if (!scaled) {
            ITSpanRows spans[3] = { _DT_ImageBlitNoneInline, _DT_ImageBlitSaveInline, _DT_ImageBlitCopyInline };
            spans[alpha] ROWS_ARGS(0);
        } else {
            ITSpanScaled spans[3] = { _DT_ImageScaleNoneInline, _DT_ImageScaleSaveInline,
                                      _DT_ImageScaleCopyInline };
            spans[alpha] SCALED_ARGS(0);
        }
        goto done;
    case kBlitBlend:
        if (!scaled) {
            ITSpanRows spans[3] = { _DT_ImageBlitNoneUniform, _DT_ImageBlitSaveUniform, _DT_ImageBlitCopyUniform };
            spans[alpha] ROWS_ARGS(trans->key.alpha);
        } else {
            ITSpanScaled spans[3] = { _DT_ImageScaleNoneUniform, _DT_ImageScaleSaveUniform,
                                      _DT_ImageScaleCopyUniform };
            spans[alpha] SCALED_ARGS(trans->key.alpha);
        }
        goto done;
    case kBlitMaskBlend:
        if (!scaled) {
            ITSpanRows spans[3] = { _DT_ImageBlitNoneCombined, _DT_ImageBlitSaveCombined,
                                    _DT_ImageBlitCopyCombined };
            spans[alpha] ROWS_ARGS(trans->key.alpha);
        } else {
            ITSpanScaled spans[3] = { _DT_ImageScaleNoneCombined, _DT_ImageScaleSaveCombined,
                                      _DT_ImageScaleCopyCombined };
            spans[alpha] SCALED_ARGS(trans->key.alpha);
        }
        goto done;
    case kBlitKey:
        if (srcClut != NULL)
            key = trans->key.alpha;
        else {
            KEY_INTO(key, srcPixels, trans)
        }
        if (!scaled && alpha == kAlphaOpaque) {
            column = SPAN_COLUMN(srcPixels, dstPixels);
            gRowsKey[column] ROWS_ARGS(key);
        } else if (!scaled) {
            ITSpanRows spans[3] = { _DT_ImageBlitNoneColorKey, _DT_ImageBlitSaveColorKey, _DT_ImageBlitCopyColorKey };
            spans[alpha] ROWS_ARGS(key);
        } else {
            ITSpanScaled spans[3] = { _DT_ImageScaleNoneColorKey, _DT_ImageScaleSaveColorKey,
                                      _DT_ImageScaleCopyColorKey };
            spans[alpha] SCALED_ARGS(key);
        }
        goto done;
    default:
        goto done;
    }

done:
    if (dstLocked)
        DT_ImageUnlock(dst, NULL);
    if (srcLocked)
        DT_ImageUnlock(src, srcMask);
    return err;
}

/* DT_ImageTile (fill dstRect of dst with src repeated from `origin', through its mask or a
   transparency; `copy' as for the blitter above). STATE: MATCH 100%, exe 504 / obj 504,
   frame exact, byte-identical yes. OPEN: none; the dead `e = key;` in the Key case is what makes
   the exe's push go through eax (a copy of key that is not pushed; notes/itblit.md) */
// FUNCTION: EVNOVA 0x004e97fa
long DT_ImageTile(ITImage *dst, const ITRect *dstRect, const ITPoint *origin, unsigned char copy,
                  ITImage *src, ITImage *srcMask, const ITTransparent *trans)
{
    ULong srcHeight, srcWidth;
    ITPixels *dstPixels;
    ITPixels *srcPixels;
    const ITRect *srcBounds;
    ITRect dr;
    ITRect sb;
    ITPoint home;
    long e;
    long phase;
    long column;
    ULong kind;
    unsigned char *dstRow;
    ITPixels *maskPixels;
    ULong height, width;
    long err;
    ULong yPhase, xPhase;
    unsigned char alpha;
    unsigned char *srcRow;
    ULong key;
    ULong *dstClut;
    ULong *srcClut;
    unsigned char *inverse;
    unsigned char scaled;
    unsigned char dstLocked;
    unsigned char srcLocked;
    unsigned char *maskRow;

    dstPixels = _DT_ImageGetImage(dst);
    srcPixels = _DT_ImageGetImage(src);
    maskPixels = _DT_ImageGetAlpha(srcMask);
    err = 0;
    srcBounds = &srcPixels->bounds;
    alpha = kAlphaOpaque;
    home = gDT_TileOrigin;
    dstClut = NULL;
    srcClut = NULL;
    inverse = NULL;
    scaled = 0;
    dstLocked = 0;
    srcLocked = 0;
    if (maskPixels != NULL)
        DT_IsRectEqual(&srcPixels->bounds, &maskPixels->bounds);

    if (trans != NULL) {
        kind = trans->mode;
        switch (kind) {
        case kBlitBlend:
            if (trans->key.alpha == 0)
                goto done;
            if (trans->key.alpha == kKeyAlphaNone)
                kind = kBlitCopy;
            break;
        case kBlitMaskBlend:
            if (trans->key.alpha == 0)
                goto done;
            if (trans->key.alpha == kKeyAlphaNone)
                kind = kBlitMask;
            break;
        case kBlitCopy:
        case kBlitMask:
        case kBlitAlpha:
        case kBlitKey:
            break;
        }
    } else {
        kind = maskPixels != NULL ? kBlitMask : kBlitCopy;
    }

    /* BUG: the second arm tests trans != NULL and trans == NULL, so `copy' with a transparency never sets alpha */
    if ((copy && maskPixels != NULL) || (copy && trans != NULL && trans == NULL))
        alpha = kAlphaSet;
    else if (!copy && dstPixels->bits[0] != 0)
        alpha = kAlphaKeep;

    if (dstRect == NULL)
        dstRect = &dstPixels->bounds;
    if (origin == NULL)
        origin = &home;
    dr = *dstRect;
    sb = *srcBounds;
    if (dr.bottom < dstPixels->bounds.top || dr.top > dstPixels->bounds.bottom ||
        dr.right < dstPixels->bounds.left || dr.left > dstPixels->bounds.right)
        goto done;

    e = DT_ImageLock(dst, NULL, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    dstLocked = 1;
    e = DT_ImageLock(src, srcMask, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    srcLocked = 1;

    if (dstPixels->bytesPerPixel == kIndexedBytes) {
        dstClut = (ULong *)dstPixels->palette.entry;
        inverse = dstPixels->inverse;
    }
    if (srcPixels->bytesPerPixel == kIndexedBytes)
        srcClut = (ULong *)srcPixels->palette.entry;

    if (dr.top < dstPixels->bounds.top)
        dr.top = dstPixels->bounds.top;
    if (dr.left < dstPixels->bounds.left)
        dr.left = dstPixels->bounds.left;
    if (dr.bottom > dstPixels->bounds.bottom)
        dr.bottom = dstPixels->bounds.bottom;
    if (dr.right > dstPixels->bounds.right)
        dr.right = dstPixels->bounds.right;

    /* where in the source the first row and column fall */
    for (phase = dr.top - origin->v; phase < 0; phase += sb.bottom - sb.top)
        ;
    srcHeight = sb.bottom - sb.top;
    yPhase = phase % (long)srcHeight;
    for (column = dr.left - origin->h; column < 0; column += sb.right - sb.left)
        ;
    srcWidth = sb.right - sb.left;
    xPhase = column % (long)srcWidth;

    dstRow = (unsigned char *)dstPixels->baseAddr +
             (dr.left - dstPixels->bounds.left) * dstPixels->bytesPerPixel +
             (dr.top - dstPixels->bounds.top) * dstPixels->rowBytes;
    srcRow = (unsigned char *)srcPixels->baseAddr + (sb.left + xPhase) * srcPixels->bytesPerPixel +
             (sb.top + yPhase) * srcPixels->rowBytes;
    maskRow = !maskPixels ? NULL :
              (unsigned char *)maskPixels->baseAddr + (sb.left + xPhase) +
              (sb.top + yPhase) * maskPixels->rowBytes;
    height = dr.bottom - dr.top;
    width = dr.right - dr.left;

    switch (kind) {
    case kBlitCopy:
        column = alpha == kAlphaSet ? 1 : SPAN_COLUMN(srcPixels, dstPixels);
        gTiledCopy[column] TILED_ARGS(0);
        goto done;
    case kBlitMask: {
        ITSpanTiled spans[3] = { _DT_ImageTileNoneBitmap, _DT_ImageTileSaveBitmap, _DT_ImageTileCopyBitmap };
        spans[alpha] TILED_ARGS(0);
        goto done;
    }
    case kBlitAlpha: {
        ITSpanTiled spans[3] = { _DT_ImageTileNoneInline, _DT_ImageTileSaveInline, _DT_ImageTileCopyInline };
        spans[alpha] TILED_ARGS(0);
        goto done;
    }
    case kBlitBlend: {
        ITSpanTiled spans[3] = { _DT_ImageTileNoneUniform, _DT_ImageTileSaveUniform, _DT_ImageTileCopyUniform };
        spans[alpha] TILED_ARGS(trans->key.alpha);
        goto done;
    }
    case kBlitMaskBlend: {
        ITSpanTiled spans[3] = { _DT_ImageTileNoneCombined, _DT_ImageTileSaveCombined,
                                 _DT_ImageTileCopyCombined };
        spans[alpha] TILED_ARGS(trans->key.alpha);
        goto done;
    }
    case kBlitKey: {
        ITSpanTiled spans[3] = { _DT_ImageTileNoneColorKey, _DT_ImageTileSaveColorKey, _DT_ImageTileCopyColorKey };
        if (srcClut != NULL)
            key = trans->key.alpha;
        else {
            KEY_INTO(key, srcPixels, trans)
        }
        e = key;
        spans[alpha] TILED_ARGS(key);
        goto done;
    }
    default:
        goto done;
    }

done:
    if (dstLocked)
        DT_ImageUnlock(dst, NULL);
    if (srcLocked)
        DT_ImageUnlock(src, srcMask);
    return err;
}

#define kPixelsState        (offsetof(ITPixels, palette) - offsetof(ITPixels, bytesPerPixel))
#define kPortState          (sizeof(ITPort) - offsetof(ITPort, textAlign))

#define kFormatRgb555       0x555
#define kFormatRgb565       0x565
#define kFormatRgb888       0x888
#define kFormatArgb1555     0x1555
#define kFormatArgb4444     0x4444
#define kFormatArgb8888     0x8888
#define kFormatRgba4444     0x80004444
#define kFormatRgba8888     0x80008888
#define kFormatReversed     0x80000000
#define kImageLocked        -7305
#define kMinTileSide        128

ULong gDT_NextIndexedKey = 0x888a;                     /* 0x00576fc8 next indexed-format key */

/* _DT_PaletteToInverse (the inverse table: every 4-4-4 colour to its nearest palette index). STATE: MATCH
   100%, exe 79 / obj 79, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004e9f32
void _DT_PaletteToInverse(unsigned char *table, const ITPalette *palette)
{
    ULong i;
    ULong j;
    ULong best;
    long err;
    ULong bestDist;
    ULong dist;
    unsigned short qb;
    unsigned short qg;
    unsigned short qr;
    unsigned short r;
    unsigned short g;
    unsigned short b;

    err = 0;
    for (i = 0; i < kInverseTableSize; i++) {
        r = (i >> 8) & 0xf;
        r = r << 4 | r;
        g = (i >> 4) & 0xf;
        g = g << 4 | g;
        b = i & 0xf;
        b = b << 4 | b;
        j = best = 0;
        bestDist = 0xffffffff;
        for (; j < palette->count; j++) {
            qr = palette->entry[j].red;
            qg = palette->entry[j].green;
            qb = palette->entry[j].blue;
            dist = (r - qr) * (r - qr) + (g - qg) * (g - qg) + (b - qb) * (b - qb);
            if (dist < bestDist) {
                best = j;
                bestDist = dist;
            }
        }
        table[i] = best;
    }
}

/* _DT_ImageCalcFormat (a format key's bytes per pixel, channel widths a r g b, shifts, masks and
   16-bit widening factors; -1 for an unknown key). STATE: MATCH 100%, exe 231 / obj 231,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea028
long _DT_ImageCalcFormat(ULong format, unsigned char *flag, unsigned char *bits,
                  unsigned char *shift, unsigned char *mask, unsigned short *scale)
{
    long err;
    long bytes;
    ULong a;
    ULong r;
    ULong g;
    ULong b;
    ULong aShift;
    ULong rShift;
    ULong gShift;
    ULong bShift;
    ULong i;

    err = 0;
    bytes = -1;
    if (format > 0x20) {
        a = (format >> 12) & 0xf;
        r = (format >> 8) & 0xf;
        g = (format >> 4) & 0xf;
        b = format & 0xf;
    }
    switch (format) {
    case 8:
        bytes = 1;
        bits[0] = 8;
        bits[1] = 8;
        bits[2] = 8;
        bits[3] = 8;
        shift[0] = 0;
        shift[1] = 8;
        shift[2] = 16;
        shift[3] = 24;
        break;
    case 15:
    case 16:
        a = 0;
        r = g = b = 5;
        if (format == 16)
            g++;
    case kFormatRgb555:
    case kFormatRgb565:
    case kFormatArgb1555:
    case kFormatArgb4444:
        bytes = 2;
        aShift = r + (rShift = g + (gShift = b + (bShift = 0)));
        bits[0] = a;
        bits[1] = r;
        bits[2] = g;
        bits[3] = b;
        shift[0] = aShift;
        shift[1] = rShift;
        shift[2] = gShift;
        shift[3] = bShift;
        break;
    case kFormatRgba4444:
        bytes = 2;
        rShift = g + (gShift = b + (bShift = a + (aShift = 0)));
        bits[0] = a;
        bits[1] = r;
        bits[2] = g;
        bits[3] = b;
        shift[0] = aShift;
        shift[1] = rShift;
        shift[2] = gShift;
        shift[3] = bShift;
        break;
    case 24:
        a = 0;
        r = g = b = 8;
    case kFormatRgb888:
        bytes = 3;
        aShift = r + (rShift = g + (gShift = b + (bShift = 0)));
        bits[0] = a;
        bits[1] = r;
        bits[2] = g;
        bits[3] = b;
        shift[0] = aShift;
        shift[1] = rShift;
        shift[2] = gShift;
        shift[3] = bShift;
        break;
    case 32:
        a = 0;
        r = g = b = 8;
    case kFormatArgb8888:
        bytes = 4;
        aShift = r + (rShift = g + (gShift = b + (bShift = 0)));
        bits[0] = a;
        bits[1] = r;
        bits[2] = g;
        bits[3] = b;
        shift[0] = aShift;
        shift[1] = rShift;
        shift[2] = gShift;
        shift[3] = bShift;
        break;
    case kFormatRgba8888:
        bytes = 4;
        aShift = b + (bShift = g + (gShift = r + (rShift = 0)));
        bits[0] = a;
        bits[1] = r;
        bits[2] = g;
        bits[3] = b;
        shift[0] = aShift;
        shift[1] = rShift;
        shift[2] = gShift;
        shift[3] = bShift;
        break;
    }
    /* BUG: an unknown key leaves bits[] unwritten, so its masks and scales come from
       whatever the caller's buffer held */
    for (i = 0; i < 4; i++) {
        mask[i] = (1 << bits[i]) - 1;
        scale[i] = gDT_Mults[bits[i]];
    }
    *flag = 0;
    return bytes;
}

/* _DT_ImageCalcStamp (the format key of a pixel description: a fresh one per indexed image, else the
   channel widths as nibbles, bit 31 when the shifts run low to high). STATE: MATCH 100%, exe
   28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea377
ULong _DT_ImageCalcStamp(const ITPixels *info)
{
    ULong key;

    if (info->depth == kIndexedDepth) {
        key = gDT_NextIndexedKey;
        gDT_NextIndexedKey++;
    } else {
        key = (ULong)info->bits[0] << 12 | (ULong)info->bits[1] << 8 |
              (ULong)info->bits[2] << 4 | info->bits[3];
        if (info->shift[0] > info->shift[3] && info->shift[3] > info->shift[2] &&
            info->shift[2] > info->shift[1])
            key |= kFormatReversed;
    }
    return key;
}

/* _DT_ImageReplace (swap two images' pixel-map state, everything between the link and the tables).
   STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea3c9
void _DT_ImageReplace(ITImage *a, ITImage *b)
{
    ITPixels *pa;
    ITPixels *pb;
    ITPixels saved;
    long err;

    pa = _DT_ImageGetImage(a);
    pb = _DT_ImageGetImage(b);
    err = 0;
    memmove(&saved, pa, sizeof(saved));
    memmove(&pa->bytesPerPixel, &pb->bytesPerPixel, kPixelsState);
    memmove(&pb->bytesPerPixel, &saved.bytesPerPixel, kPixelsState);
}

/* _DT_SurfaceReplace (swap two images' port state and pixel-map state, then trade the ports' +0xc
   word, mirrored into each pixel map's +0x5c). STATE: MATCH 100%, exe 80 / obj 80,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea43e
void _DT_SurfaceReplace(ITImage *a, ITImage *b)
{
    ITPort *portA;
    ITPort *portB;
    ITPixels *pixA;
    ITPixels *pixB;
    ITPort savedPort;
    ITPixels savedPixels;
    long err;

    portA = _DT_SurfaceGetContext(a);
    portB = _DT_SurfaceGetContext(b);
    pixA = _DT_SurfaceGetImage(a);
    pixB = _DT_SurfaceGetImage(b);
    err = 0;
    memmove(&savedPort, portA, sizeof(savedPort));
    memmove(&portA->textAlign, &portB->textAlign, kPortState);
    memmove(&portB->textAlign, &savedPort.textAlign, kPortState);
    memmove(&savedPixels, pixA, sizeof(savedPixels));
    memmove(&pixA->bytesPerPixel, &pixB->bytesPerPixel, kPixelsState);
    memmove(&pixB->bytesPerPixel, &savedPixels.bytesPerPixel, kPixelsState);
    portA->dc = portB->dc;
    pixA->drawDC = (long)portA->dc;
    portB->dc = savedPort.dc;
    pixB->drawDC = (long)portB->dc;
}

/* DT_ImageResize (re-frame an image and its mask to new bounds: a moved frame of the same size
   is taken as it is, else each is redrawn into a fresh image of its format and the pixel maps
   swapped). STATE: MATCH 100%, exe 133 / obj 133, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea528
long DT_ImageResize(ITImage *image, ITImage *mask, const ITRect *bounds)
{
    ITPixels *pixels;
    ITPixels *maskPixels;
    ITImage *fresh;
    ULong format;
    long err;
    long e;

    pixels = _DT_ImageGetImage(image);
    maskPixels = _DT_ImageGetAlpha(mask);
    err = 0;
    fresh = NULL;
    if (maskPixels != NULL)
        DT_IsRectEqual(&pixels->bounds, &maskPixels->bounds);
    if (pixels->locks != 0) {
        err = kImageLocked;
    } else if (bounds->right - bounds->left == pixels->bounds.right - pixels->bounds.left &&
               bounds->bottom - bounds->top == pixels->bounds.bottom - pixels->bounds.top) {
        pixels->bounds = *bounds;
    } else {
        format = pixels->bytesPerPixel == 1 ? kIndexedDepth :
                 pixels->bits[0] << 12 | pixels->bits[1] << 8 | pixels->bits[2] << 4 |
                 pixels->bits[3];
        fresh = DT_ImageCreate(bounds, format, &pixels->palette);
        if (fresh == NULL)
            goto nomem;
        DT_ImageCopy(fresh, NULL, 1, image, NULL, NULL, NULL);
        _DT_ImageReplace(image, fresh);
        if (maskPixels != NULL) {
            DT_ImageDispose(fresh);
            fresh = NULL;
            fresh = DT_ImageCreate(bounds, kIndexedDepth, &gDT_AlphaPalette);
            if (fresh == NULL) {
nomem:
                err = kOutOfMemory;
                goto done;
            }
            e = DT_ImageCopy(fresh, NULL, 1, mask, NULL, NULL, NULL);
            if (e != 0) {
                err = e;
                goto done;
            }
            _DT_ImageReplace(mask, fresh);
        }
    }
done:
    if (fresh != NULL)
        DT_ImageDispose(fresh);
    return err;
}

/* DT_ImagePreflightTile (widen an image and its mask to at least kMinTileSide on each side by tiling each
   into a fresh image of its format, the pixel maps swapped). STATE: MATCH 100%, exe 144 / obj
   144, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea683
long DT_ImagePreflightTile(ITImage *image, ITImage *mask)
{
    ITPixels *pixels;
    ITPixels *maskPixels;
    long err;
    ITImage *fresh;
    ITRect bounds;
    ULong format;
    long e;
    long nx;
    long ny;

    pixels = _DT_ImageGetImage(image);
    maskPixels = _DT_ImageGetAlpha(mask);
    err = 0;
    fresh = NULL;
    if (pixels->locks != 0) {
        err = kImageLocked;
    } else if (pixels->bounds.right - pixels->bounds.left < kMinTileSide ||
               pixels->bounds.bottom - pixels->bounds.top < kMinTileSide) {
        bounds.top = pixels->bounds.top;
        bounds.left = pixels->bounds.left;
        bounds.right = pixels->bounds.right;
        for (nx = 0; nx++, bounds.right - bounds.left < kMinTileSide; )
            bounds.right += pixels->bounds.right - pixels->bounds.left;
        bounds.bottom = pixels->bounds.bottom;
        for (ny = 0; ny++, bounds.bottom - bounds.top < kMinTileSide; )
            bounds.bottom += pixels->bounds.bottom - pixels->bounds.top;
        format = pixels->bytesPerPixel == 1 ? kIndexedDepth :
                 pixels->bits[0] << 12 | pixels->bits[1] << 8 | pixels->bits[2] << 4 |
                 pixels->bits[3];
        fresh = DT_ImageCreate(&bounds, format, &pixels->palette);
        if (fresh == NULL)
            goto nomem;
        e = DT_ImageTile(fresh, NULL, NULL, 0, image, NULL, NULL);
        if (e != 0) {
            err = e;
            goto done;
        }
        _DT_ImageReplace(image, fresh);
        if (maskPixels != NULL) {
            DT_ImageDispose(fresh);
            fresh = NULL;
            fresh = DT_ImageCreate(&bounds, kIndexedDepth, &gDT_AlphaPalette);
            if (fresh == NULL) {
nomem:
                err = kOutOfMemory;
                goto done;
            }
            e = DT_ImageTile(fresh, NULL, NULL, 0, mask, NULL, NULL);
            if (e != 0) {
                err = e;
                goto done;
            }
            _DT_ImageReplace(mask, fresh);
        }
    }
done:
    if (fresh != NULL)
        DT_ImageDispose(fresh);
    return err;
}

/* DT_BufferRead (read count bytes off a memory stream; kStreamOverrun past its end). STATE:
   MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ea80c
long DT_BufferRead(ITStream *stream, void *dst, ULong count)
{
    long err;

    err = 0;
    if (stream->pos + count > stream->size) {
        err = kStreamOverrun;
    } else if (count != 0) {
        char *src = (char *)*stream->data + stream->pos;

        if (count != 0)
            memmove(dst, src, count);
        stream->pos += count;
    }
    return err;
}

/* DT_ImageSetRawPixels (store one row of 8-bit pixels into an image, and into `mask' the palette alpha of
   each; the image is locked around it when it was not). STATE: MATCH 100%, exe 70 / obj 70,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea850
void DT_ImageSetRawPixels(ITImage *image, ITImage *mask, ULong row, const unsigned char *src)
{
    long e;
    ULong i;
    unsigned char locked;
    ULong width;
    ULong count;
    unsigned char *dst;
    ITPixels *pixels;
    long err;
    ITPixels *maskPixels;

    pixels = _DT_ImageGetImage(image);
    maskPixels = _DT_ImageGetAlpha(mask);
    err = 0;
    locked = 0;
    if (pixels->baseAddr == 0) {
        e = DT_ImageLock(image, mask, 0, 0);
        if (e != 0)
            err = e;
        locked = 1;
    }
    dst = (unsigned char *)(row * pixels->rowBytes + pixels->baseAddr);
    width = pixels->bounds.right - pixels->bounds.left;
    if (width != 0)
        memmove(dst, src, width);
    if (maskPixels != 0) {
        count = pixels->bounds.right - pixels->bounds.left;
        dst = (unsigned char *)(row * maskPixels->rowBytes + maskPixels->baseAddr);
        for (i = 0; i < count; i++)
            dst[i] = pixels->palette.entry[src[i]].alpha;
    }
    if (locked != 0)
        DT_ImageUnlock(image, mask);
}

/* DT_ImageSetRawColors (store one row of a r g b pixels into an image in its own format, each channel
   narrowed to its width and shifted into place, low byte first; alpha into `mask'). STATE: MATCH
   100%, exe 111 / obj 111, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ea90b
void DT_ImageSetRawColors(ITImage *image, ITImage *mask, ULong row, const ITColor *src)
{
    ITPixels *pixels;
    ITPixels *maskPixels;
    long err;
    long e;
    unsigned char *maskRow;
    unsigned char locked;
    ULong width;
    unsigned char *dst;
    ULong value;
    ULong i;
    ULong b;

    pixels = _DT_ImageGetImage(image);
    maskPixels = _DT_ImageGetAlpha(mask);
    err = 0;
    maskRow = 0;
    locked = 0;
    if (pixels->baseAddr == 0) {
        e = DT_ImageLock(image, mask, 0, 0);
        if (e != 0)
            err = e;
        locked = 1;
    }
    width = pixels->bounds.right - pixels->bounds.left;
    dst = (unsigned char *)(row * pixels->rowBytes + pixels->baseAddr);
    if (maskPixels != 0)
        maskRow = (unsigned char *)(row * maskPixels->rowBytes + maskPixels->baseAddr);
    for (i = 0; i < width; i++, dst += pixels->bytesPerPixel) {
        value = (src[i].green >> (8 - pixels->bits[2])) << pixels->shift[2];
        value |= (src[i].red >> (8 - pixels->bits[1])) << pixels->shift[1] |
                 pixels->mask[0] << pixels->shift[0];
        value |= (src[i].blue >> (8 - pixels->bits[3])) << pixels->shift[3];
        for (b = 0; b < pixels->bytesPerPixel; b++, value >>= 8)
            dst[b] = value;
        if (maskRow != 0)
            maskRow[i] = src[i].alpha;
    }
    if (locked != 0)
        DT_ImageUnlock(image, mask);
}

/* DT_AlphaInfoDefaults (a loader's transparency answer at "none"). STATE: MATCH 100%, exe 14 / obj 14, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004eaa61
void DT_AlphaInfoDefaults(ITTransparent *t)
{
    long err;

    err = 0;
    memset(t, 0, sizeof(*t));
    t->mode = 0;
    t->key.alpha = 0xff;
}

/* DT_FileInfoDefaults (a loader's background answer at its defaults). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004eaa81
void DT_FileInfoDefaults(ITBackground *bg)
{
    long err;

    err = 0;
    memset(bg, 0, sizeof(*bg));
    bg->format = 0;
    bg->gamma = 0.0;
    bg->color = gDT_ColorWhite;
    bg->hasColor = 0;
    bg->interlaced = 0;
    bg->pad16 = 0;
}
