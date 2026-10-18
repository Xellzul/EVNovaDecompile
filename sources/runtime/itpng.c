// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itpng.c -- the interface tool's PNG reader over libpng 1.2.7: the stream read callback, the signature test and the loader. Notes: Research/notes/itpng.md */

#include <stddef.h>
#include "png.h"
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

#define kPngSignatureBytes  4
#define kFormatPng          0x504e4766    /* 'PNGf' */
#define kScreenGamma        2.2
#define kFormatDirect       0x8888
#define kDirectDepth        32
#define kMaxImageSide       0x1000
#define kMetresPerInch      0.0254
#define kPointsPerInch      72.0
#define kPpmAt72Dpi         2835          /* 72 dpi in pixels per metre */
#define kMicronsPerInch     25400.0
#define kTransparentNone    0

static const unsigned char DAT_0057aff8[8] = { 0 };   /* 0x0057aff8, unreferenced */
static char kNoMemoryText[] = "Out of memory.";

/* DT_png_read_data (libpng's read callback: the loaders' memory stream). STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051491d
void DT_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    long err;

    err = 0;
    if (DT_BufferRead((ITStream *)png_ptr->io_ptr, data, length) != 0)
        png_error(png_ptr, "Read Error");
}

static char kWriteError[] = "Write Error";

/* PNG_CheckFileType (the first four bytes of the PNG signature). STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051494c
unsigned char PNG_CheckFileType(png_bytep data, ULong size)
{
    if (size >= kPngSignatureBytes && png_sig_cmp(data, 0, kPngSignatureBytes) == 0)
        return 1;
    return 0;
}

/* PNG_Decode (load a PNG off a stream: libpng's example.c read with gamma, palettes and
   greys dithered to an 8-bit image, colour to 32 bits, alpha to a mask, then pHYs aspect and
   oFFs offsets applied by re-framing the image). STATE: MATCH 100%, exe 1105 / obj 1105,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051496d
long PNG_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut,
                  ITTransparent *trans, ITBackground *bg)
{
    long err;
    long e;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int interlace_type;
    int intent;
    int num_trans;
    png_bytep trans_alpha;
    png_color_16p trans_values;
    png_color_16 my_background;
    png_color_16p image_background;
    double screen_gamma;
    double image_gamma;
    ITPixelInfo info;
    ITPalette palette;
    ITColor transColor;
    ITRect bounds;
    ITImage *noMask;
    int num_palette;
    png_colorp palette_ptr;
    png_uint_16p histogram;
    png_bytepp rows;
    unsigned char locked;
    long transMode;
    ULong format;
    png_color grey_palette[256];
    long i;
    long j;
    long best;
    int count;
    long step;
    long levels;
    long odd;
    long bump;
    ULong x;
    long y;
    unsigned char *src;
    unsigned char *dst;
    ULong srcRowBytes;
    ULong dstRowBytes;
    long dh;
    long dv;
    float hScale;
    float vScale;
    float ratio;
    float scale;
    unsigned char first;

    err = 0;
    png_ptr = NULL;
    info_ptr = NULL;
    num_trans = 0;
    trans_alpha = NULL;
    image_background = &my_background;
    rows = NULL;
    locked = 0;
    transMode = kTransparentNone;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
        goto nomem;
    e = setjmp(png_ptr->jmpbuf);
    if (e != 0) {
        err = e;
        err = kStreamOverrun;
        goto done;
    }
    png_set_read_fn(png_ptr, stream, DT_png_read_data);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                 &interlace_type, NULL, NULL);

    screen_gamma = kScreenGamma;
    if (bg != NULL) {
        bg->format = kFormatPng;
        if (png_get_bKGD(png_ptr, info_ptr, &image_background)) {
            bg->hasColor = 1;
            if ((color_type & PNG_COLOR_MASK_COLOR) != 0) {
                bg->color.alpha = 0xff;
                bg->color.red = image_background->red;
                bg->color.green = image_background->green;
                bg->color.blue = image_background->blue;
            } else {
                bg->color.alpha = 0xff;
                bg->color.red = image_background->gray;
                bg->color.green = (unsigned char)image_background->gray;
                bg->color.blue = image_background->gray;
            }
        }
        if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
            png_set_sRGB(png_ptr, info_ptr, intent);
            bg->gamma = kScreenGamma;
        } else if (png_get_gAMA(png_ptr, info_ptr, &image_gamma)) {
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
            bg->gamma = 1.0 / image_gamma;
        } else {
            bg->gamma = kScreenGamma;
            image_gamma = 1.0 / bg->gamma;
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
        }
    } else {
        if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
            png_set_sRGB(png_ptr, info_ptr, intent);
        } else {
            image_gamma = 1.0 / kScreenGamma;
            if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
                png_set_gamma(png_ptr, screen_gamma, image_gamma);
            else
                png_set_gamma(png_ptr, screen_gamma, 1.0 / kScreenGamma);
        }
    }

    if ((color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_RGB) &&
        png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_expand(png_ptr);
        color_type |= PNG_COLOR_MASK_ALPHA;
    }
    switch (color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        palette_ptr = NULL;
        histogram = NULL;
        if (!png_get_PLTE(png_ptr, info_ptr, &palette_ptr, &num_palette)) {
            err = kImageFailed;
            goto done;
        }
        png_get_hIST(png_ptr, info_ptr, &histogram);
        png_set_dither(png_ptr, palette_ptr, num_palette, 1 << bit_depth, histogram, 0);
        for (i = 0; i < num_palette; i++) {
            palette.entry[i].alpha = 0xff;
            palette.entry[i].red = palette_ptr[i].red;
            palette.entry[i].green = palette_ptr[i].green;
            palette.entry[i].blue = palette_ptr[i].blue;
        }
        for (; i < 1 << bit_depth; i++) {
            palette.entry[i].alpha = 0xff;
            palette.entry[i].red = 0;
            palette.entry[i].green = 0;
            palette.entry[i].blue = 0;
        }
        if (png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, &trans_values) &&
            num_trans != 0) {
            for (i = best = 0; i < num_trans; i++) {
                palette.entry[i].alpha = trans_alpha[i];
                if (palette.entry[best].alpha < palette.entry[i].alpha)
                    best = i;
            }
            if (palette.entry[best].alpha != 0xff) {
                transMode = kTransparentKey;
                transColor = palette.entry[best];
            }
        }
        if (bit_depth < 8)
            png_set_packing(png_ptr);
        format = kIndexedDepth;
        break;
    case PNG_COLOR_TYPE_GRAY:
        if (bit_depth < 8)
            png_set_packing(png_ptr);
        if (bit_depth == 16) {
            png_set_strip_16(png_ptr);
            bit_depth = 8;
        }
        levels = 1 << bit_depth;
        count = levels;
        step = 0xff / (count - 1);
        if (step % 2 != 0)
            odd = 0;
        else
            odd = 1;
        bump = odd;
        for (j = 0; j < count / 2; j++) {
            palette.entry[j].alpha = 0xff;
            palette.entry[j].red = palette.entry[j].green = palette.entry[j].blue =
                grey_palette[j].red = grey_palette[j].green = grey_palette[j].blue =
                j * step;
        }
        for (; j < count; j++) {
            palette.entry[j].alpha = 0xff;
            palette.entry[j].red = palette.entry[j].green = palette.entry[j].blue =
                grey_palette[j].red = grey_palette[j].green = grey_palette[j].blue =
                j * step + bump;
        }
        for (; j < levels; j++) {
            palette.entry[j].alpha = 0xff;
            palette.entry[j].red = palette.entry[j].green = palette.entry[j].blue =
                grey_palette[j].red = grey_palette[j].green = grey_palette[j].blue = 0;
        }
        png_set_dither(png_ptr, grey_palette, count, count, NULL, 1);
        format = kIndexedDepth;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        if (bit_depth == 16)
            png_set_strip_16(png_ptr);
        if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
        png_set_gray_to_rgb(png_ptr);
        bit_depth = kDirectDepth;
        format = kFormatDirect;
        transMode = kTransparentMask;
        break;
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        if (bit_depth == 16)
            png_set_strip_16(png_ptr);
        png_set_bgr(png_ptr);
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
        bit_depth = kDirectDepth;
        format = kFormatDirect;
        if ((color_type & PNG_COLOR_MASK_ALPHA) != 0)
            transMode = kTransparentMask;
        break;
    }
    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    DT_SetRect(&bounds, 0, 0, width, height);
    if (bit_depth <= 8) {
        palette.count = 1 << bit_depth;
        bit_depth = 8;
    } else {
        palette.count = 0;
    }
    *imageOut = DT_ImageCreate(&bounds, format, &palette);
    if (*imageOut == NULL)
        goto nomem;
    switch (transMode) {
    case kTransparentNone:
        break;
    case kTransparentMask:
        if (maskOut != NULL) {
            *maskOut = DT_ImageCreate(&bounds, kIndexedDepth, &gDT_AlphaPalette);
            if (*maskOut == NULL)
                goto nomem;
            if (trans != NULL) {
                trans->mode = kTransparentMask;
                trans->key.alpha = 0xff;
            }
        }
        break;
    case kTransparentKey:
        if (trans != NULL) {
            trans->mode = kTransparentKey;
            trans->key = transColor;
        }
        break;
    }
    if (maskOut == NULL) {
        maskOut = &noMask;
        noMask = NULL;
    }
    e = DT_ImageLock(*imageOut, *maskOut, &info, NULL);
    if (e != 0)
        goto fail;
    locked = 1;
    rows = (png_bytepp)MemoryAllocate(height * sizeof(png_bytep));
    if (rows == NULL) {
nomem:
        err = kOutOfMemory;
        goto done;
    }
    for (y = 0; y < height; y++)
        rows[y] = (png_bytep)(y * info.rowBytes + (ULong)info.bits);
    png_read_image(png_ptr, rows);
    png_read_end(png_ptr, info_ptr);

    dst = (unsigned char *)info.otherBits;
    if (dst != NULL) {
        src = (unsigned char *)info.bits;
        srcRowBytes = info.rowBytes;
        dstRowBytes = info.otherRowBytes;
        if (bit_depth == 8) {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++)
                    dst[x] = palette.entry[src[x]].alpha;
                src += srcRowBytes;
                dst += dstRowBytes;
            }
        } else {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++)
                    dst[x] = src[x * 4 + 3];
                src += srcRowBytes;
                dst += dstRowBytes;
            }
        }
    }

    if ((info_ptr->valid & PNG_INFO_pHYs) != 0 && info_ptr->x_pixels_per_unit != 0 &&
        info_ptr->y_pixels_per_unit != 0) {
        hScale = 1.0f;
        vScale = 1.0f;
        ratio = 1.0f;
        scale = 1.0f;
        first = 1;
        ratio = (float)info_ptr->y_pixels_per_unit / (float)info_ptr->x_pixels_per_unit;
        scale = 1.0f;
        switch (info_ptr->phys_unit_type) {
        case PNG_RESOLUTION_UNKNOWN:
            hScale = ratio / scale;
            hScale = hScale > 1.0 ? hScale : 1.0;
            vScale = scale / ratio;
            vScale = vScale > 1.0 ? vScale : 1.0;
            break;
        case PNG_RESOLUTION_METER:
            if (info_ptr->x_pixels_per_unit != kPpmAt72Dpi)
                hScale = info_ptr->x_pixels_per_unit * kMetresPerInch * (1.0 / kPointsPerInch);
            if (info_ptr->y_pixels_per_unit != kPpmAt72Dpi)
                vScale = info_ptr->y_pixels_per_unit * kMetresPerInch * (1.0 / kPointsPerInch);
            break;
        default:
            hScale = vScale = 1.0f;
            break;
        }
        if (hScale != 1.0 || vScale != 1.0) {
            width = (png_uint_32)((bounds.right - bounds.left) * hScale + 0.5);
            height = (png_uint_32)((bounds.bottom - bounds.top) * vScale + 0.5);
            /* BUG: the scales never change between passes, so once entered this never ends */
            while (width >= kMaxImageSide || height >= kMaxImageSide) {
                hScale = ratio / scale;
                hScale = hScale > 1.0 ? hScale : 1.0;
                vScale = scale / ratio;
                vScale = vScale > 1.0 ? vScale : 1.0;
                width = (png_uint_32)((bounds.right - bounds.left) * hScale + 0.5);
                height = (png_uint_32)((bounds.bottom - bounds.top) * vScale + 0.5);
                first = 0;
                ratio /= 2.0f;
                scale /= 2.0f;
            }
        }
        if (hScale != 1.0 || vScale != 1.0) {
            bounds.right = bounds.left + width;
            bounds.bottom = bounds.top + height;
            DT_ImageUnlock(*imageOut, *maskOut);
            locked = 0;
            e = DT_ImageResize(*imageOut, *maskOut, &bounds);
            if (e != 0)
                goto fail;
            e = DT_ImageLock(*imageOut, *maskOut, NULL, NULL);
            if (e != 0)
                goto fail;
            locked = 1;
        }
    }

    dh = 0;
    dv = 0;
    if (info_ptr->offset_unit_type == PNG_OFFSET_MICROMETER) {
        dh = (long)(png_get_x_offset_microns(png_ptr, info_ptr) * kPointsPerInch *
                    (1.0 / kMicronsPerInch) + 0.5);
        dv = (long)(png_get_y_offset_microns(png_ptr, info_ptr) * kPointsPerInch *
                    (1.0 / kMicronsPerInch) + 0.5);
    } else if (info_ptr->offset_unit_type == PNG_OFFSET_PIXEL) {
        dh = png_get_x_offset_pixels(png_ptr, info_ptr);
        dv = png_get_y_offset_pixels(png_ptr, info_ptr);
    }
    if (dh != 0 || dv != 0) {
        DT_ImageUnlock(*imageOut, *maskOut);
        locked = 0;
        DT_RectOffset(DT_ImageGetRect(*imageOut, &bounds), dh, dv);
        e = DT_ImageResize(*imageOut, *maskOut, &bounds);
        if (e != 0)
            goto fail;
        e = DT_ImageLock(*imageOut, *maskOut, NULL, NULL);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        locked = 1;
    }

done:
    if (png_ptr != NULL)
        png_destroy_read_struct(&png_ptr, info_ptr != NULL ? &info_ptr : NULL, NULL);
    if (rows != NULL)
        MemoryDeallocate(rows);
    if (locked)
        DT_ImageUnlock(*imageOut, *maskOut);
    if (err != 0) {
        if (imageOut != NULL && *imageOut != NULL) {
            DT_ImageDispose(*imageOut);
            *imageOut = NULL;
        }
        if (maskOut != NULL && *maskOut != NULL) {
            DT_ImageDispose(*maskOut);
            *maskOut = NULL;
        }
        if (trans != NULL)
            DT_AlphaInfoDefaults(trans);
        if (bg != NULL)
            DT_FileInfoDefaults(bg);
    }
    return err;
}

static const unsigned char DAT_0057b080[8] = { 0 };   /* 0x0057b080, unreferenced */
