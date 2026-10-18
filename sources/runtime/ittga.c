// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* ittga.c -- the interface tool's Targa reader: the header test, its row reversers and the loader. Notes: Research/notes/ittga.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

typedef struct TGAHeader {
    unsigned char idLength;                      /* +0x00 */
    unsigned char colorMapType;                  /* +0x01 */
    unsigned char imageType;                     /* +0x02 */
    unsigned char pad3;                          /* +0x03 */
    unsigned short colorMapFirst;                /* +0x04 */
    unsigned short colorMapLength;               /* +0x06 */
    unsigned char colorMapDepth;                 /* +0x08 */
    unsigned char pad9;                          /* +0x09 */
    unsigned short xOrigin;                      /* +0x0a */
    unsigned short yOrigin;                      /* +0x0c */
    unsigned short width;                        /* +0x0e */
    unsigned short height;                       /* +0x10 */
    unsigned char pixelDepth;                    /* +0x12 */
    unsigned char descriptor;                    /* +0x13 */
} TGAHeader;

#define kTGAFileHeaderSize  0x12
#define kTGANoImage         0
#define kTGAMapped          1
#define kTGATrueColor       2
#define kTGAGray            3
#define kTGAMappedRLE       9
#define kTGATrueColorRLE    10
#define kTGAGrayRLE         11
#define kTGANegative        0x8000
#define kTGAInterleave      0xc0

/* the Targa 2.0 file footer and extension area */
typedef struct TGAFooter {
    ULong extensionOffset;
    ULong developerOffset;
    char signature[16];
    char dot;
    char terminator;
} TGAFooter;

typedef struct TGAExtension {
    unsigned short size;
    char authorName[41];
    char authorComment[324];
    unsigned short dateTime[6];
    char jobName[41];
    unsigned short jobTime[3];
    char softwareId[41];
    unsigned char softwareVersion[3];
    ULong keyColor;
    unsigned short pixelRatioNum;
    unsigned short pixelRatioDen;
    unsigned short gammaNum;
    unsigned short gammaDen;
    ULong colorCorrectionOffset;
    ULong postageStampOffset;
    ULong scanLineOffset;
    unsigned char attributesType;
} TGAExtension;

#define kTGAFooterSize      26
#define kTGAMinFooterFile   (kTGAFileHeaderSize + kTGAFooterSize)
#define kTGASignature       "TRUEVISION-XFILE."
#define kTGAExtensionSize   0x1ef
#define kTGAAttrMax         4
#define kTGAAttrAlpha       3             /* useful alpha data */
#define kTGARLE             8             /* image type bit */
#define kTGARunPacket       0x80
#define kTGARunCount        0x7f
#define kTGARightToLeft     0x10U
#define kTGATopToBottom     0x20U
#define kTGAAlphaBits       0x0f
#define kTGAAlpha16         0x8000
#define kFormatTarga        0x54504943    /* 'TPIC' */
#define kTGADefaultGamma    2.2
#define kKey5BitMask        0xf1
#define kKey8BitMask        0xff
#define kTransparentAlpha   2
#define kMaskDepth          8
#define kPaletteMax         256

/* DT_ReverseBytes (reverse n bytes in place). STATE: MATCH 100%, exe 24 / obj 24, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00518757
void DT_ReverseBytes(char *p, ULong n)
{
    ULong i;
    ULong j;
    char t;

    for (i = 0, j = n - 1; i < n >> 1; i++, j--) {
        t = p[i];
        p[i] = p[j];
        p[j] = p[i];
    }
}

/* _TGA_ReverseColors (reverse n dwords in place). STATE: MATCH 100%, exe 25 / obj 25, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00518786
void _TGA_ReverseColors(ULong *p, ULong n)
{
    ULong i;
    ULong j;
    ITColor t;

    for (i = 0, j = n - 1; i < n >> 1; i++, j--) {
        t = ((ITColor *)p)[i];
        p[i] = p[j];
        p[j] = p[i];
    }
}

/* TGA_CheckFileType (is this a Targa file this reader takes). STATE: MATCH 100%, exe 177 / obj 177,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005187bd
unsigned char TGA_CheckFileType(const void *buffer, ULong size)
{
    const unsigned char *data = (const unsigned char *)buffer;
    TGAHeader h;

    if (size >= kTGAFileHeaderSize) {
        MemoryClear(&h, sizeof(h));
        MemoryCopy(&h.idLength, data, 1);
        MemoryCopy(&h.colorMapType, data + 1, 1);
        MemoryCopy(&h.imageType, data + 2, 1);
        MemoryCopy(&h.colorMapFirst, data + 3, 2);
        MemoryCopy(&h.colorMapLength, data + 5, 2);
        MemoryCopy(&h.colorMapDepth, data + 7, 1);
        MemoryCopy(&h.xOrigin, data + 8, 2);
        MemoryCopy(&h.yOrigin, data + 10, 2);
        MemoryCopy(&h.width, data + 12, 2);
        MemoryCopy(&h.height, data + 14, 2);
        MemoryCopy(&h.pixelDepth, data + 16, 1);
        MemoryCopy(&h.descriptor, data + 17, 1);

        if (h.colorMapType != 0 && h.colorMapType != 1)
            return 0;
        switch (h.imageType) {
        case kTGAMapped:
        case kTGAMappedRLE:
            if (h.colorMapType == 0)
                return 0;
            break;
        case kTGATrueColor:
        case kTGATrueColorRLE:
            break;
        case kTGAGray:
        case kTGAGrayRLE:
            if (h.colorMapType == 1)
                return 0;
            break;
        case kTGANoImage:
        default:
            return 0;
        }
        if (h.colorMapType == 1 && h.colorMapFirst >= h.colorMapLength)
            return 0;
        switch (h.colorMapDepth) {
        case 0:
            if (h.colorMapType == 1)
                return 0;
            break;
        case 15:
        case 16:
        case 24:
        case 32:
            if (h.colorMapType == 0)
                return 0;
            break;
        default:
            return 0;
        }
        if ((h.width & kTGANegative) != 0)
            return 0;
        if ((h.height & kTGANegative) != 0)
            return 0;
        switch (h.pixelDepth) {
        case 8:
            break;
        case 16:
        case 24:
        case 32:
            if (h.imageType == kTGAMapped)
                return 0;
            if (h.imageType == kTGAMappedRLE)
                return 0;
            break;
        default:
            return 0;
        }
        if ((h.descriptor & kTGAInterleave) != 0)
            return 0;
        return 1;
    }
    return 0;
}

/* a 5-bit channel already shifted to the top of a byte, its top bits repeated below */
#define Widen5(c)   ((c) | (c) >> 5)

/* TGA_Decode (load a Targa off a stream: header, the 2.0 footer and extension area, the
   colour map, then rows raw or run-length coded at 8, 16, 24 or 32 bits, into a new image and,
   when the file has alpha, an 8-bit mask). STATE: MATCH 100%, exe 1231 / obj 1231,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005189a5
long TGA_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut,
                  ITTransparent *trans, ITBackground *bg)
{
    long err;
    long e;
    char *cmap;
    char *row;
    ITColor *pixels;
    unsigned char locked;
    unsigned char hasAlpha;
    unsigned char rightToLeft;
    unsigned char topToBottom;
    ULong y;
    ULong width;
    ULong height;
    ULong carry;
    ULong count;
    ULong x;
    ULong i;
    ULong size;
    ULong k;
    ULong r;
    ULong g;
    ULong b;
    long alphaBits;
    unsigned char keyMask;
    unsigned char *key;
    unsigned char id[255];
    unsigned char packet;
    TGAHeader h;
    TGAFooter footer;
    TGAExtension ext;
    ITPalette palette;
    ITRect bounds;
    ITColor color;
    unsigned char index;
    unsigned short word;

    err = 0;
    cmap = NULL;
    size = 0;
    row = NULL;
    pixels = NULL;
    locked = 0;
    hasAlpha = 0;
    rightToLeft = 0;
    topToBottom = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);

    MemoryClear(&h, sizeof(h));
    e = DT_BufferRead(stream, &h.idLength, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.colorMapType, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.imageType, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.colorMapFirst, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.colorMapLength, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.colorMapDepth, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.xOrigin, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.yOrigin, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.width, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.height, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.pixelDepth, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_BufferRead(stream, &h.descriptor, 1);
    if (e != 0) {
        err = e;
        goto done;
    }

    MemoryClear(&footer, sizeof(footer));
    if (stream->size > kTGAMinFooterFile) {
        i = stream->pos;
        stream->pos = stream->size - kTGAFooterSize;
        e = DT_BufferRead(stream, &footer.extensionOffset, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &footer.developerOffset, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, footer.signature, sizeof(footer.signature));
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &footer.dot, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &footer.terminator, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        stream->pos = i;
        if (footer.terminator != 0 || StringCompare(footer.signature, kTGASignature) != 0)
            MemoryClear(&footer, sizeof(footer));
        if (footer.extensionOffset >= stream->size || footer.developerOffset >= stream->size)
            goto bad;
    }

    MemoryClear(&ext, sizeof(ext));
    if (footer.extensionOffset != 0) {
        i = stream->pos;
        stream->pos = footer.extensionOffset;
        e = DT_BufferRead(stream, &ext.size, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (ext.size == kTGAExtensionSize) {
            if (ext.size + footer.extensionOffset >= stream->size) {
                err = kStreamOverrun;
                goto done;
            }
            e = DT_BufferRead(stream, ext.authorName, sizeof(ext.authorName));
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, ext.authorComment, sizeof(ext.authorComment));
            if (e != 0) {
                err = e;
                goto done;
            }
            for (i = 0; i < 6; i++) {
                e = DT_BufferRead(stream, &ext.dateTime[i], 2);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
            e = DT_BufferRead(stream, ext.jobName, sizeof(ext.jobName));
            if (e != 0) {
                err = e;
                goto done;
            }
            for (i = 0; i < 3; i++) {
                e = DT_BufferRead(stream, &ext.jobTime[i], 2);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
            e = DT_BufferRead(stream, ext.softwareId, sizeof(ext.softwareId));
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, ext.softwareVersion, sizeof(ext.softwareVersion));
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.keyColor, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.pixelRatioNum, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.pixelRatioDen, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.gammaNum, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.gammaDen, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.colorCorrectionOffset, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.postageStampOffset, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.scanLineOffset, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &ext.attributesType, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            MemoryClear(&ext, sizeof(ext));
        }
        stream->pos = i;
        if (ext.attributesType >= kTGAAttrMax) {
            err = kStreamOverrun;
            goto done;
        }
    }

    if (h.idLength != 0) {
        /* BUG: skips one byte of the image ID, not idLength */
        e = DT_BufferRead(stream, id, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (h.colorMapType == 1) {
        /* BUG: the depth is in bits, so 8x the map's bytes are allocated and read */
        size = h.colorMapLength * h.colorMapDepth;
        cmap = MemoryAllocate(size);
        if (cmap == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        e = DT_BufferRead(stream, cmap, size);
        if (e != 0) {
            err = e;
            goto done;
        }
    }

    palette.count = 0;
    if (cmap != NULL) {
        palette.count = h.colorMapLength - h.colorMapFirst;
        if (palette.count > kPaletteMax)
            palette.count = kPaletteMax;
        switch (h.colorMapDepth) {
        case 15:
        case 16:
            /* BUG: k is both the byte offset and the colour word it reads, and the bytes are
               read signed */
            for (i = k = 0; i < palette.count; i++, k += 2) {
                k = (cmap[k + 1] << 8) | cmap[k];
                palette.entry[i].alpha = 0xff;
                r = (k & 0x7c00) >> 7;
                palette.entry[i].red = Widen5(r);
                g = (k & 0x3e0) >> 2;
                palette.entry[i].green = Widen5(g);
                b = (k & 0x1f) << 3;
                palette.entry[i].blue = Widen5(b);
            }
            break;
        case 24:
        case 32:
            for (i = k = 0; i < palette.count; i++, k += h.colorMapDepth / 8) {
                palette.entry[i].alpha = 0xff;
                palette.entry[i].red = cmap[k + 2];
                palette.entry[i].green = cmap[k + 1];
                palette.entry[i].blue = cmap[k];
            }
            break;
        }
    } else if (h.imageType == kTGAGray || h.imageType == kTGAGrayRLE) {
        /* BUG: palette.count is 0 for a greyscale image, so the ramp is never built */
        for (i = 0; i < palette.count; i++) {
            palette.entry[i].alpha = 0xff;
            palette.entry[i].red = i;
            palette.entry[i].green = i;
            palette.entry[i].blue = i;
        }
    }

    DT_SetRect(&bounds, (short)h.xOrigin, -(short)h.yOrigin, h.width, h.height);
    *imageOut = DT_ImageCreate(&bounds, h.pixelDepth, palette.count != 0 ? &palette : NULL);
    if (*imageOut == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    if (bg != NULL) {
        bg->format = kFormatTarga;
        /* BUG: gammaDen / gammaDen, always 1.0; gammaNum is never read */
        if (ext.gammaDen != 0)
            bg->gamma = (double)ext.gammaDen / ext.gammaDen;
        else
            bg->gamma = kTGADefaultGamma;
    }
    if (trans != NULL) {
        key = (unsigned char *)&ext.keyColor;
        color.red = key[2];
        color.green = key[1];
        color.blue = key[0];
        if (h.pixelDepth == 8) {
            keyMask = h.colorMapDepth <= 16 ? kKey5BitMask : kKey8BitMask;
            if (ext.keyColor != 0) {
                color.red &= keyMask;
                color.green &= keyMask;
                color.blue &= keyMask;
                for (i = 0; i < palette.count; i++) {
                    /* BUG: `entry & keyMask == key' is entry & (keyMask == key) */
                    if ((palette.entry[i].red & keyMask == color.red) &&
                        (palette.entry[i].green & keyMask == color.green) &&
                        (palette.entry[i].blue & keyMask == color.blue)) {
                        trans->mode = kTransparentKey;
                        trans->key.alpha = i;
                        break;
                    }
                }
            }
        } else if (h.pixelDepth == 24) {
            if (ext.keyColor != 0) {
                trans->mode = kTransparentKey;
                trans->key.alpha = 0;
                trans->key.red = color.red;
                trans->key.green = color.green;
                trans->key.blue = color.blue;
            }
        } else if (ext.attributesType == kTGAAttrAlpha) {
            trans->mode = kTransparentAlpha;
        } else if (ext.keyColor != 0) {
            trans->mode = kTransparentKey;
            trans->key.alpha = 0;
            trans->key.red = color.red;
            trans->key.green = color.green;
            trans->key.blue = color.blue;
        }
    }

    /* BUG: bits, not bytes: the row buffer is 8x too large */
    i = h.width * h.pixelDepth;
    width = h.width;
    height = h.height;
    if ((h.descriptor & kTGARightToLeft) != 0)
        rightToLeft = 1;
    if ((h.descriptor & kTGATopToBottom) != 0)
        topToBottom = 1;
    alphaBits = h.descriptor & kTGAAlphaBits;
    if (alphaBits != 0) {
        if (h.pixelDepth == 8 || h.pixelDepth == 24) {
            err = kStreamOverrun;
            goto done;
        }
        if (h.pixelDepth == 16 && alphaBits > 1) {
            err = kStreamOverrun;
            goto done;
        }
        if (h.pixelDepth == 32 && alphaBits != 8) {
            err = kStreamOverrun;
            goto done;
        }
        if (ext.attributesType > 1)
            hasAlpha = 1;
        else if (ext.size == 0)
            hasAlpha = 1;
    } else {
        if (ext.attributesType > 1)
            hasAlpha = 1;
        else if (ext.size == 0)
            hasAlpha = 1;
    }
    if (hasAlpha && maskOut != NULL) {
        *maskOut = DT_ImageCreate(&bounds, kMaskDepth, &gDT_AlphaPalette);
        if (*maskOut == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        if (trans != NULL) {
            trans->mode = kTransparentMask;
            trans->key.alpha = 0xff;
        }
    }
    row = MemoryAllocate(i);
    if (row == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    if (h.pixelDepth > 8) {
        pixels = MemoryAllocate(width * sizeof(ITColor));
        if (pixels == NULL) {
            err = kOutOfMemory;
            goto done;
        }
    }
    /* BUG: *maskOut is read without the NULL test the entry makes on maskOut (here and below) */
    e = DT_ImageLock(*imageOut, *maskOut, NULL, NULL);
    if (e != 0) {
        err = e;
        goto done;
    }
    locked = 1;

    switch (h.pixelDepth) {
    case 8:
        /* BUG: carry is only ever 0 (all three depths): a packet crossing the row end is
           rejected as bad data */
        carry = 0;
        for (y = 0; y < height; y++) {
            if (carry != 0) {
                count = carry;
                carry = 0;
            } else if ((h.imageType & kTGARLE) != 0) {
                count = 0;
            } else {
                count = width;
            }
            packet = 0;
            for (x = 0; x < width; ) {
                if (count == 0) {
                    e = DT_BufferRead(stream, &packet, 1);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    count = (packet & kTGARunCount) + 1;
                }
                if (x + count > width) {
                    err = kStreamOverrun;
                    goto done;
                }
                if ((packet & kTGARunPacket) != 0) {
                    e = DT_BufferRead(stream, &index, 1);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    while (count--)
                        row[x++] = index;
                } else {
                    e = DT_BufferRead(stream, row + x, count);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    x += count;
                }
                count = 0;
            }
            if (rightToLeft)
                DT_ReverseBytes(row, width);
            DT_ImageSetRawPixels(*imageOut, *maskOut, topToBottom ? y : height - y - 1, (unsigned char *)row);
        }
        break;
    case 16:
        carry = 0;
        for (y = 0; y < height; y++) {
            if (carry != 0) {
                count = carry;
                carry = 0;
            } else if ((h.imageType & kTGARLE) != 0) {
                count = 0;
            } else {
                count = width;
            }
            packet = 0;
            for (x = 0; x < width; ) {
                if (count == 0) {
                    e = DT_BufferRead(stream, &packet, 1);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    count = (packet & kTGARunCount) + 1;
                }
                if (x + count > width) {
                    err = kStreamOverrun;
                    goto done;
                }
                if ((packet & kTGARunPacket) != 0) {
                    e = DT_BufferRead(stream, &word, 2);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    color.alpha = hasAlpha && (word & kTGAAlpha16) != 0 ? 0 : 0xff;
                    r = (word & 0x7c00) >> 7;
                    color.red = Widen5(r);
                    g = (word & 0x3e0) >> 2;
                    color.green = Widen5(g);
                    b = (word & 0x1f) << 3;
                    color.blue = Widen5(b);
                    while (count--)
                        pixels[x++] = color;
                } else {
                    e = DT_BufferRead(stream, row, count * 2);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    for (k = 0; count--; k++) {
                        word = ((unsigned short *)row)[k];
                        b = hasAlpha && (word & kTGAAlpha16) != 0;
                        pixels[x].alpha = b ? 0 : 0xff;
                        r = (word & 0x7c00) >> 7;
                        pixels[x].red = Widen5(r);
                        g = (word & 0x3e0) >> 2;
                        pixels[x].green = Widen5(g);
                        b = (word & 0x1f) << 3;
                        pixels[x].blue = Widen5(b);
                        x++;
                    }
                }
                count = 0;
            }
            if (rightToLeft)
                _TGA_ReverseColors((ULong *)pixels, width);
            DT_ImageSetRawColors(*imageOut, *maskOut, topToBottom ? y : height - y - 1, pixels);
        }
        break;
    case 24:
        hasAlpha = 0;
        /* fall through */
    case 32:
        carry = 0;
        for (y = 0; y < height; y++) {
            if (carry != 0) {
                count = carry;
                carry = 0;
            } else if ((h.imageType & kTGARLE) != 0) {
                count = 0;
            } else {
                count = width;
            }
            packet = 0;
            for (x = 0; x < width; ) {
                if (count == 0) {
                    e = DT_BufferRead(stream, &packet, 1);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    count = (packet & kTGARunCount) + 1;
                }
                if (x + count > width) {
bad:
                    err = kStreamOverrun;
                    goto done;
                }
                if ((packet & kTGARunPacket) != 0) {
                    e = DT_BufferRead(stream, row, h.pixelDepth / 8);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    color.alpha = hasAlpha ? ((unsigned char *)row)[3] : 0xff;
                    color.red = row[2];
                    color.green = row[1];
                    color.blue = row[0];
                    while (count--)
                        pixels[x++] = color;
                } else {
                    e = DT_BufferRead(stream, row, h.pixelDepth * count >> 3);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    for (k = 0; count--; k += h.pixelDepth / 8) {
                        pixels[x].alpha = hasAlpha ? ((unsigned char *)row)[k + 3] : 0xff;
                        pixels[x].red = ((unsigned char *)row)[k + 2];
                        pixels[x].green = ((unsigned char *)row)[k + 1];
                        pixels[x].blue = ((unsigned char *)row)[k];
                        x++;
                    }
                }
                count = 0;
            }
            if (rightToLeft)
                _TGA_ReverseColors((ULong *)pixels, width);
            DT_ImageSetRawColors(*imageOut, *maskOut, topToBottom ? y : height - y - 1, pixels);
        }
        break;
    }

done:
    if (cmap != NULL)
        MemoryDeallocate(cmap);
    if (pixels != NULL)
        MemoryDeallocate(pixels);
    if (row != NULL)
        MemoryDeallocate(row);
    if (locked)
        DT_ImageUnlock(*imageOut, *maskOut);
    if (err != 0 && *imageOut != NULL) {
        DT_ImageDispose(*imageOut);
        *imageOut = NULL;
    }
    if (err != 0 && *maskOut != NULL) {
        DT_ImageDispose(*maskOut);
        *maskOut = NULL;
    }
    return err;
}
