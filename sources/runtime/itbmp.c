// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itbmp.c -- the interface tool's Windows / OS/2 bitmap reader: the 4-bit row unpacker, the RLE row reader, the header test and the loader. Notes: Research/notes/itbmp.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

#define kRleEndOfLine       0
#define kRleEndOfBitmap     1
#define kRleDelta           2
#define kBmpMinSize         0x0e
#define kBmpSizeOffset      2
#define kBmpBitsOffset      10
#define kBmpWinMinFile      0x36
#define kBmpOs2MinFile      0x28

/* DT_BmpUnpack4 (4-bit pixels to one byte each; `odd' starts on a low nibble; answers whether
   it stopped halfway through a byte). STATE: MATCH 100%, exe 37 / obj 37, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051247c
unsigned char DT_BmpUnpack4(unsigned char *dst, const unsigned char *src, ULong count,
                           unsigned char odd)
{
    if (odd != 0 && count != 0) {
        *dst = *src & 0xf;
        src++;
        dst++;
        count--;
    }
    while (count > 1) {
        dst[0] = (*src >> 4) & 0xf;
        dst[1] = *src & 0xf;
        src++;
        dst += 2;
        count -= 2;
    }
    if (count != 0)
        *dst = (*src >> 4) & 0xf;
    return count;
}

/* the row a run lands on: bottom-up unless the height is negative */
#define RowAt(y)    (height < 0 ? (y) : rows - (y) - 1)

/* _BMP_DecodePixels (decode BI_RLE8 / BI_RLE4 rows off the stream, storing each finished row;
   escapes: end of line, end of bitmap, delta, absolute run). STATE: MATCH 100%, exe 350 /
   obj 350, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005124d2
long _BMP_DecodePixels(ITStream *stream, ITImage *image, unsigned char *buf, unsigned char *row,
                  long width, long height, unsigned char rle4, ULong rows)
{
    long err;
    long e;
    ULong x;
    ULong y;
    ULong count;
    unsigned char odd;

    err = 0;
    x = 0;
    y = 0;
    do {
        e = DT_BufferRead(stream, buf, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        count = buf[0];
        if (count != 0) {
            if (rle4) {
                buf[0] = (buf[1] >> 4) & 0xf;
                buf[1] = buf[1] & 0xf;
            }
            do {
                if (rle4) {
                    /* BUG: count runs down, so an odd run starts on the low nibble */
                    do
                        row[x++] = buf[count & 1];
                    while (--count != 0 && x < width);
                } else {
                    do
                        row[x++] = buf[1];
                    while (--count != 0 && x < width);
                }
                if (x == width) {
                    DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
                    x = 0;
                    y++;
                }
            } while (count != 0 && y < rows);
            continue;
        }
        count = buf[1];
        switch (count) {
        case kRleEndOfLine:
            do
                row[x++] = 0;
            while (x < width);
            DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
            x = 0;
            y++;
            break;
        case kRleEndOfBitmap:
            do
                row[x++] = 0;
            while (x < width);
            DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
            x = 0;
            y++;
            MemoryClear(row, width);
            while (y < rows) {
                DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
                x = 0;
                y++;
            }
            break;
        case kRleDelta:
            e = DT_BufferRead(stream, buf, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            count = buf[0];
            count += x;
            if (buf[1] != 0) {
                do
                    row[x++] = 0;
                while (x < width);
                DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
                x = 0;
                y++;
                buf[1]--;
            }
            MemoryClear(row, width);
            while (buf[1] != 0 && y < rows) {
                DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
                x = 0;
                y++;
                buf[1]--;
            }
            x = count;
            break;
        default:
            if (rle4) {
                odd = 0;
                e = DT_BufferRead(stream, buf, (count + 3) >> 2 << 1);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                while (x + count >= width && y < rows) {
                    odd = DT_BmpUnpack4(row + x, buf, width - x, odd);
                    /* BUG: += where -= was meant, and each row unpacks from the start of buf again */
                    count += width - x;
                    DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
                    x = 0;
                    y++;
                }
                odd = DT_BmpUnpack4(row + x, buf, count, odd);
                x += count * 2 + (odd ? 1 : 0);
            } else {
                e = DT_BufferRead(stream, buf, count + (count & 1));
                if (e != 0) {
                    err = e;
                    goto done;
                }
                while (x + count >= width && y < rows) {
                    MemoryCopy(row + x, buf, width - x);
                    /* BUG: += where -= was meant, and each row copies from the start of buf again */
                    count += width - x;
                    DT_ImageSetRawPixels(image, NULL, RowAt(y), row);
                    x = 0;
                    y++;
                }
                MemoryCopy(row + x, buf, count);
                x += count;
            }
            break;
        }
    } while (y < rows);
done:
    return err;
}

/* BMP_CheckFileType (a "BM" file over 0x36 bytes or a "BA" array of at least 0x28, with a bits
   offset). STATE: MATCH 100%, exe 56 / obj 56, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00512903
unsigned char BMP_CheckFileType(const void *buffer, ULong size)
{
    const unsigned char *data = (const unsigned char *)buffer;
    ULong fileSize;

    fileSize = 0;
    if (size > 6)
        MemoryCopy(&fileSize, data + kBmpSizeOffset, sizeof(fileSize));
    if (size >= kBmpMinSize && data[0] == 'B' && data[1] == 'M' && fileSize > kBmpWinMinFile &&
        (data[kBmpBitsOffset] != 0 || data[kBmpBitsOffset + 1] != 0 ||
         data[kBmpBitsOffset + 2] != 0 || data[kBmpBitsOffset + 3] != 0))
        return 1;
    if (size >= kBmpMinSize && data[0] == 'B' && data[1] == 'A' && fileSize >= kBmpOs2MinFile &&
        (data[kBmpBitsOffset] != 0 || data[kBmpBitsOffset + 1] != 0 ||
         data[kBmpBitsOffset + 2] != 0 || data[kBmpBitsOffset + 3] != 0))
        return 1;
    return 0;
}

/* BITMAPFILEHEADER (and the OS/2 array header, same fields) and BITMAPINFOHEADER with the
   BI_BITFIELDS masks, as the loader unpacks them */
typedef struct BmpFileHeader {
    short type;
    ULong size;
    unsigned short reserved1;
    unsigned short reserved2;
    ULong offBits;
} BmpFileHeader;

typedef struct BmpInfoHeader {
    ULong size;
    long width;
    long height;
    short planes;
    short bitCount;
    ULong compression;
    ULong sizeImage;
    long xPelsPerMeter;
    long yPelsPerMeter;
    long clrUsed;
    ULong clrImportant;
    ULong redMask;
    ULong greenMask;
    ULong blueMask;
    ULong alphaMask;
} BmpInfoHeader;

#define kBmpTypeBM          0x424d        /* "BM" */
#define kBmpTypeBA          0x4241        /* "BA", an OS/2 bitmap array */
#define kBmpArrayEntry      0x28
#define kBmpRGB             0
#define kBmpRLE8            1
#define kBmpRLE4            2
#define kBmpBitFields       3
#define kBmpSkipChunk       0x100
#define kFormatBmp          0x424d5066    /* 'BMPf' */
#define kBmpGamma           2.2
#define kBmpBadHeader       -7301

/* a mask's shift, and its width turned into the factor that widens it to 16 bits */
#define MaskShape(m, mask, shift, scale)                    \
    m = (mask);                                             \
    if (m != 0) {                                           \
        for (shift = 0; (m & 1) == 0; shift++)              \
            m >>= 1;                                        \
        for (scale = 0; m != 0; scale++)                    \
            m >>= 1;                                        \
        scale = gDT_Mults[scale];                        \
    }

#undef RowAt
#define RowAt(y)    (info.height < 0 ? (y) : rows - (y) - 1)

#define ReadOrFail(ptr, len)                                \
    do {                                                    \
        long r = DT_BufferRead(stream, ptr, len);            \
        if (r != 0) {                                       \
            err = r;                                        \
            goto done;                                      \
        }                                                   \
    } while (0)

/* BMP_Decode (load a Windows bitmap, or the first of an OS/2 array, off a stream: 1/4/8-bit
   through its palette, RLE4/RLE8 through 0x005124d2, 16/32-bit bit fields, 24-bit BGR). STATE:
   MATCH 100%, exe 899 / obj 899, byte-identical yes. OPEN: none; quadSize is the exe's two
   dead `mov eax,4'; ReadOrFail's block-local `r' keeps `e' off the reload reuse; notes/itbmp.md */
// FUNCTION: EVNOVA 0x0051299a
long BMP_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut,
                  ITTransparent *trans, ITBackground *bg)
{
    long err;
    long e;
    ULong n;
    ULong y;
    ULong rShift;
    ULong gShift;
    ULong bShift;
    ULong aShift;
    unsigned short rScale;
    unsigned short gScale;
    unsigned short bScale;
    unsigned short aScale;
    unsigned char locked;
    unsigned char hasAlpha;
    unsigned char *row;
    unsigned char *indices;
    ITColor *pixels;
    ULong consumed;
    ULong rows;
    ULong width4;
    ULong x;
    ULong k;
    ULong v;
    ULong w;
    ULong size;
    ULong quadSize;
    ULong depth;
    ULong c;
    ULong bit;
    unsigned char quads[256 * 4];
    unsigned char scratch[kBmpSkipChunk];
    BmpFileHeader array;
    BmpFileHeader file;
    BmpInfoHeader info;
    ITPalette palette;
    ITRect bounds;

    err = 0;
    rShift = 0;
    gShift = 0;
    bShift = 0;
    aShift = 0;
    rScale = 0;
    gScale = 0;
    bScale = 0;
    aScale = 0;
    locked = 0;
    hasAlpha = 0;
    row = NULL;
    indices = NULL;
    pixels = NULL;
    consumed = 0;
    width4 = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);

    MemoryClear(&array, sizeof(array));
    do {
        MemoryClear(&file, sizeof(file));
        ReadOrFail(&file.type, 2);
        ReadOrFail(&file.size, 4);
        ReadOrFail(&file.reserved1, 2);
        ReadOrFail(&file.reserved2, 2);
        ReadOrFail(&file.offBits, 4);
        file.type = Swap16(file.type);
        if (file.type == kBmpTypeBA) {
            if (array.type != 0) {
                err = kBmpBadHeader;
                goto done;
            }
            MemoryCopy(&array, &file, sizeof(array));
            if (array.size != kBmpArrayEntry) {
                err = kBmpBadHeader;
                goto done;
            }
        } else {
            if (file.type != kBmpTypeBM) {
                err = kBmpBadHeader;
                goto done;
            }
            if (file.size <= kBmpWinMinFile) {
                err = kBmpBadHeader;
                goto done;
            }
            if (file.offBits < kBmpWinMinFile) {
                err = kBmpBadHeader;
                goto done;
            }
        }
    } while (file.type == kBmpTypeBA);

    MemoryClear(&info, sizeof(info));
    ReadOrFail(&info.size, 4);
    size = info.size;
    if (size <= sizeof(info)) {
        ReadOrFail(&info.width, size - 4);
        consumed = info.size;
    } else {
        ReadOrFail(&info.width, sizeof(info) - 4);
        consumed = sizeof(info);
    }
    if (info.width == 0) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.height == 0) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.bitCount != 1 && info.bitCount != 4 && info.bitCount != 8 &&
        info.bitCount != 16 && info.bitCount != 24 && info.bitCount != 32) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.compression != kBmpRGB && info.compression != kBmpRLE8 &&
        info.compression != kBmpRLE4 && info.compression != kBmpBitFields) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.compression == kBmpRLE8 && info.bitCount != 8) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.compression == kBmpRLE4 && info.bitCount != 4) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.compression == kBmpBitFields && info.bitCount != 16 && info.bitCount != 32) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.compression == kBmpBitFields &&
        (info.redMask == 0 || info.greenMask == 0 || info.blueMask == 0)) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.clrUsed > 1 << info.bitCount) {
        err = kStreamOverrun;
        goto done;
    }
    if (info.bitCount == 16 && info.compression == kBmpRGB) {
        info.compression = kBmpBitFields;
        info.redMask = 0x7c00;
        info.greenMask = 0x3e0;
        info.blueMask = 0x1f;
    } else if (info.bitCount == 32 && info.compression == kBmpRGB) {
        info.compression = kBmpBitFields;
        info.redMask = 0xff0000;
        info.greenMask = 0xff00;
        info.blueMask = 0xff;
    }
    MaskShape(n, info.redMask, rShift, rScale)
    MaskShape(n, info.greenMask, gShift, gScale)
    MaskShape(n, info.blueMask, bShift, bScale)
    MaskShape(n, info.alphaMask, aShift, aScale)

    while ((n = info.size - consumed) != 0) {
        if (n > kBmpSkipChunk)
            n = kBmpSkipChunk;
        ReadOrFail(scratch, n);
        consumed = consumed + n;
    }
    if (info.bitCount <= 8) {
        if (info.clrUsed == 0)
            info.clrUsed = 1 << info.bitCount;
        quadSize = 4;
        ReadOrFail(quads, info.clrUsed * quadSize);
    } else {
        info.clrUsed = 0;
    }
    if (file.offBits < stream->pos) {
        err = kStreamOverrun;
        goto done;
    }
    while ((n = file.offBits - stream->pos) != 0) {
        if (n > kBmpSkipChunk)
            n = kBmpSkipChunk;
        ReadOrFail(scratch, n);
    }

    palette.count = info.clrUsed;
    for (n = k = 0; n < palette.count; n++, quadSize = 4, k += quadSize) {
        palette.entry[n].alpha = 0xff;
        palette.entry[n].red = quads[k + 2];
        palette.entry[n].green = quads[k + 1];
        palette.entry[n].blue = quads[k];
    }
    DT_SetRect(&bounds, 0, 0, info.width, info.height < 0 ? -info.height : info.height);
    depth = info.bitCount < 8 ? kIndexedDepth : info.bitCount;
    *imageOut = DT_ImageCreate(&bounds, depth, palette.count != 0 ? &palette : NULL);
    if (*imageOut == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    n = (info.width * info.bitCount + 31) / 32 * 32 / 8;
    width4 = (info.width + 3) / 4 * 4;
    if (info.height < 0)
        rows = -info.height;
    else
        rows = info.height;
    if (bg != NULL) {
        bg->format = kFormatBmp;
        bg->gamma = kBmpGamma;
    }
    row = MemoryAllocate(n);
    if (row == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    if (info.bitCount > 8) {
        pixels = MemoryAllocate(width4 * sizeof(ITColor));
        if (pixels == NULL) {
            err = kOutOfMemory;
            goto done;
        }
    } else {
        indices = MemoryAllocate(width4);
        if (indices == NULL) {
            err = kOutOfMemory;
            goto done;
        }
    }
    e = DT_ImageLock(*imageOut, NULL, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    locked = 1;

    switch (info.bitCount) {
    case 1:
        for (y = 0; y < rows; y++) {
            ReadOrFail(row, n);
            for (x = 0; x < info.width; x += 8) {
                c = row[x >> 3];
                for (bit = 0; bit < 8; bit++, c >>= 1)
                    indices[x + 7 - bit] = c & 1;
            }
            DT_ImageSetRawPixels(*imageOut, NULL, RowAt(y), indices);
        }
        break;
    case 4:
        if (info.compression == kBmpRLE4) {
            e = _BMP_DecodePixels(stream, *imageOut, row, indices, width4, info.height, 1, rows);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        }
        for (y = 0; y < rows; y++) {
            ReadOrFail(row, n);
            DT_BmpUnpack4(indices, row, info.width, 0);
            DT_ImageSetRawPixels(*imageOut, NULL, RowAt(y), indices);
        }
        break;
    case 8:
        if (info.compression == kBmpRLE8) {
            e = _BMP_DecodePixels(stream, *imageOut, row, indices, width4, info.height, 0, rows);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        }
        for (y = 0; y < rows; y++) {
            ReadOrFail(row, n);
            DT_ImageSetRawPixels(*imageOut, NULL, RowAt(y), row);
        }
        break;
    case 16:
        for (y = 0; y < rows; y++) {
            ReadOrFail(row, n);
            for (x = 0; x < info.width; x++) {
                unsigned short t;

                v = ((unsigned short *)row)[x];
                t = (v & info.redMask) >> rShift;
                pixels[x].red = t * rScale >> 8;
                t = (v & info.greenMask) >> gShift;
                pixels[x].green = t * gScale >> 8;
                t = (v & info.blueMask) >> bShift;
                pixels[x].blue = t * bScale >> 8;
                /* BUG: alpha is read with the blue mask and shift (here and in the 32-bit rows) */
                t = (v & info.blueMask) >> bShift;
                pixels[x].alpha = t * aScale >> 8;
            }
            DT_ImageSetRawColors(*imageOut, NULL, RowAt(y), pixels);
        }
        break;
    case 24:
        for (y = 0; y < rows; y++) {
            ReadOrFail(row, n);
            for (x = 0; x < info.width; x++) {
                pixels[x].blue = row[x * 3];
                pixels[x].green = row[x * 3 + 1];
                pixels[x].red = row[x * 3 + 2];
                pixels[x].alpha = 0xff;
            }
            DT_ImageSetRawColors(*imageOut, NULL, RowAt(y), pixels);
        }
        break;
    case 32:
        for (y = 0; y < rows; y++) {
            ReadOrFail(row, n);
            for (x = 0; x < info.width; x++) {
                unsigned short t;

                w = ((ULong *)row)[x];
                t = (w & info.redMask) >> rShift;
                pixels[x].red = t * rScale >> 8;
                t = (w & info.greenMask) >> gShift;
                pixels[x].green = t * gScale >> 8;
                t = (w & info.blueMask) >> bShift;
                pixels[x].blue = t * bScale >> 8;
                t = (w & info.blueMask) >> bShift;
                pixels[x].alpha = t * aScale >> 8;
            }
            DT_ImageSetRawColors(*imageOut, NULL, RowAt(y), pixels);
        }
        break;
    }

done:
    if (indices != NULL)
        MemoryDeallocate(indices);
    if (pixels != NULL)
        MemoryDeallocate(pixels);
    if (row != NULL)
        MemoryDeallocate(row);
    if (locked)
        DT_ImageUnlock(*imageOut, NULL);
    if (err != 0 && *imageOut != NULL) {
        DT_ImageDispose(*imageOut);
        *imageOut = NULL;
    }
    return err;
}
