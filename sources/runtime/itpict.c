// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itpict.c -- the interface tool's PICT reader: the opcode loader, its table and handlers, the stream readers. Notes: Research/notes/itpict.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "macport.h"

typedef struct PictFrame {
    short top;                                   /* +0 big-endian */
    short left;                                  /* +2 */
    short bottom;                                /* +4 */
    short right;                                 /* +6 */
} PictFrame;

#define kPictFrameOffset    2
#define kPictOpcodeOffset   10
#define kPictVersionOp      0x11
#define kPictVersion1       0x01
#define kPictVersion2       0x02
#define kPictVersion2Tail   0xff
#define kPictFileHeader     0x200

#define kPictVersionV1      1
#define kColorTableMax      0xff

#define kPictGamma          1.7
#define kFormatPict         0x50494354    /* 'PICT' */
#define kPatTypeRGB         2             /* a PixPat that is one RGB colour */
#define kPatTypeFull        1
#define kPatRGBSkip         14            /* pat1Data and the RGBColor */
#define kPictBadData        -7301
#define kPictUnsupported    -7003
#define kRegionHeader       11            /* read as 11 bytes, the size and bounding box */
#define kRegionData         10

#define kOpBitsRect         0x90
#define kOpBitsRgn          0x91
#define kOpPackBitsRect     0x98
#define kOpPackBitsRgn      0x99
#define kOpDirectBitsRect   0x9a
#define kOpDirectBitsRgn    0x9b
#define kPackMinRowBytes    8             /* narrower rows are never packed */
#define kPackCountByte      250           /* wider packed rows carry a word count */
#define kPackTypeDefault    0
#define kPackTypeNone       1
#define kPackTypeNoAlpha    2
#define kPackTypeWords      3
#define kPackTypeChannels   4
#define kPackTypeUnknown    0xffff
#define kTagPackBytes       0x7061636b    /* 'pack' */
#define kTagPackWords       0x5041434b    /* 'PACK' */

#define Expand5(c)      (((c) << 3) | ((c) >> 2))
#define HandleBytes(h)  (*(unsigned char **)(h))
#define CheckErr(e)     if ((e) != 0) { err = (e); goto done; }

/* an opcode entry's length: its data size, or how the size is found */
#define kOpHandler          -1            /* the handler reads it; none is bad data */
#define kOpWordLength       -2            /* a word length, then the data */
#define kOpLongLength       -3            /* a long length, then the data */
#define kOpWordSize         -4            /* a word size counting itself (regions, polygons) */
#define kOpLongSize         -5
#define kOpDHText           -6            /* a byte, then a counted string */
#define kOpDHDVText         -7            /* two bytes, then a counted string */
#define kOpLongText         -8            /* four bytes, then a counted string */
#define kOpLongComment      -9            /* a kind word, then a word length */

#define kPictLastTableOp        0xa1
#define kPictLastWordData       0xaf
#define kPictLastNoData         0xcf
#define kPictLastLongData       0xfe
#define kPictOpEndPic           0x00ff
#define kPictOpVersion          0x02ff
#define kPictOpHeader           0x0c00
#define kPictHeaderLength       24
#define kPictLastReserved       0x7fff
#define kPictLastReservedNoData 0x8000
#define kPictOpCompressedQT     0x8200
#define kPictOpUncompressedQT   0x8201
#define kPictSkipChunk          0x200
#define kPictBlankDepth         32        /* the image of a picture with no bits opcode */

/* QuickDraw's PixMap as the bits opcodes carry it (Fixed resolutions, signed) */
typedef struct PictPixMap {
    long baseAddr;
    unsigned short rowBytes;
    PictFrame bounds;
    short pmVersion;
    short packType;
    long packSize;
    long hRes;
    long vRes;
    short pixelType;
    short pixelSize;
    short cmpCount;
    short cmpSize;
    long planeBytes;
    long pmTable;
    long pmReserved;
} PictPixMap;

/* the picture being drawn: its frame, version, target images and the loader's answers */
typedef struct PictState {
    ITRect bounds;
    short version;
    ITImage *image;
    ITImage *mask;
    ITTransparent *trans;
    ITBackground *bg;
    ITPixelInfo info;
    unsigned char locked;
} PictState;

/* an opcode's entry in the table: its data length (negative: a special rule), its handler or
   none, and its name */
struct PictOpcode;
typedef long (*PictHandler)(ITStream *stream, PictState *s, const struct PictOpcode *op,
                            ULong opcode);
typedef struct PictOpcode {
    long length;
    PictHandler handler;
    const char *name;
} PictOpcode;

/* _DeferredImageCreate (create the picture's image in a format and palette, lock it, and when asked
   paint it white). STATE: MATCH 100%, exe 61 / obj 61, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515931
long _DeferredImageCreate(PictState *s, ULong format, const ITPalette *palette, unsigned char white)
{
    long err;
    long e;
    ULong width;
    ULong x;
    ULong y;
    ULong height;
    unsigned char *p;

    err = 0;
    s->image = DT_ImageCreate(&s->bounds, format, palette);
    if (s->image == NULL) {
        err = kOutOfMemory;
    } else {
        if (s->bg != NULL) {
            s->bg->format = kFormatPict;
            s->bg->gamma = kPictGamma;
        }
        e = DT_ImageLock(s->image, s->mask, &s->info, NULL);
        if (e != 0) {
            err = e;
        } else {
            s->locked = 1;
            if (white) {
                height = s->bounds.bottom - s->bounds.top;
                width = (s->bounds.right - s->bounds.left) * s->info.bytesPerPixel;
                p = (unsigned char *)s->info.bits;
                for (y = 0; y < height; y++) {
                    for (x = 0; x < width; x++)
                        p[x] = 0xff;
                    p += s->info.rowBytes;
                }
            }
        }
    }
    return err;
}

/* _DeferredImageDispose (drop the picture's images and clear the answers). STATE: MATCH 100%, exe 43 / obj 43, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005159d6
void _DeferredImageDispose(PictState *s)
{
    long err;

    err = 0;
    DT_ImageDispose(s->image);
    s->image = NULL;
    if (s->mask != NULL) {
        DT_ImageDispose(s->mask);
        s->mask = NULL;
    }
    if (s->trans != NULL)
        MemoryClear(s->trans, sizeof(*s->trans));
    if (s->bg != NULL)
        MemoryClear(s->bg, sizeof(*s->bg));
    MemoryClear(&s->info, sizeof(s->info));
    s->locked = 0;
}

/* _PICT_ReadOpCode (the next opcode: one byte in a version 1 picture, else a big-endian word at an
   even offset). STATE: MATCH 100%, exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515a39
long _PICT_ReadOpCode(ITStream *stream, short version, unsigned short *opcode)
{
    long err;
    long e;
    unsigned char b;
    unsigned short w;

    err = 0;
    if (version == kPictVersionV1) {
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0)
            goto fail;
        *opcode = b;
    } else {
        if ((stream->pos & 1) != 0) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0)
                goto fail;
        }
        e = DT_BufferRead(stream, &w, 2);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        *opcode = Swap16(w);
    }
done:
    return err;
}

/* _PICT_ReadRect (a big-endian rectangle, four words). STATE: MATCH 100%, exe 49 / obj 49,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515ac9
long _PICT_ReadRect(ITStream *stream, short *rect)
{
    long err;
    long e;

    err = 0;
    e = DT_BufferRead(stream, rect, 8);
    if (e != 0) {
        err = e;
    } else {
        rect[0] = Swap16(rect[0]);
        rect[1] = Swap16(rect[1]);
        rect[2] = Swap16(rect[2]);
        rect[3] = Swap16(rect[3]);
    }
    return err;
}

#define ReadWord(f)                                         \
    e = DT_BufferRead(stream, &(f), 2);                      \
    if (e != 0)                                             \
        goto fail;                                          \
    (f) = Swap16(f)

#define ReadLong(f)                                         \
    e = DT_BufferRead(stream, &(f), 4);                      \
    if (e != 0)                                             \
        goto fail;                                          \
    (f) = Swap32(f)

/* _PICT_ReadPixMap (a PixMap record after its baseAddr, big-endian). STATE: MATCH 100%, exe 279 / obj
   279, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515b5f
long _PICT_ReadPixMap(ITStream *stream, PictPixMap *map)
{
    long err;
    long e;

    err = 0;
    ReadWord(map->rowBytes);
    e = _PICT_ReadRect(stream, (short *)&map->bounds);
    if (e != 0)
        goto fail;
    ReadWord(map->pmVersion);
    ReadWord(map->packType);
    ReadLong(map->packSize);
    ReadLong(map->hRes);
    ReadLong(map->vRes);
    ReadWord(map->pixelType);
    ReadWord(map->pixelSize);
    ReadWord(map->cmpCount);
    ReadWord(map->cmpSize);
    ReadLong(map->planeBytes);
    ReadLong(map->pmTable);
    e = DT_BufferRead(stream, &(map->pmReserved), 4);
    if (e != 0) {
fail:
        err = e;
    } else {
        map->pmReserved = Swap32(map->pmReserved);
    }
    return err;
}

/* _PICT_ReadColorTable (a big-endian ColorTable; kStreamOverrun past 256 entries). STATE: MATCH 100%,
   exe 122 / obj 122, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515eb8
long _PICT_ReadColorTable(ITStream *stream, PictColorTable *table)
{
    long err;
    long e;
    ULong i;
    ULong last;

    err = 0;
    e = DT_BufferRead(stream, &table->seed, 4);
    if (e != 0)
        goto fail;
    table->seed = Swap32(table->seed);
    e = DT_BufferRead(stream, &table->flags, 2);
    if (e != 0)
        goto fail;
    table->flags = Swap16(table->flags);
    e = DT_BufferRead(stream, &table->size, 2);
    if (e != 0)
        goto fail;
    table->size = Swap16(table->size);
    if (table->size > kColorTableMax) {
        err = kStreamOverrun;
        goto done;
    }
    for (i = 0, last = table->size; i <= last; i++) {
        e = DT_BufferRead(stream, &table->entry[i], sizeof(PictColorSpec));
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        table->entry[i].value = Swap16(table->entry[i].value);
        table->entry[i].red = Swap16(table->entry[i].red);
        table->entry[i].green = Swap16(table->entry[i].green);
        table->entry[i].blue = Swap16(table->entry[i].blue);
    }
done:
    return err;
}

/* _PICT_ReadRegion (a QuickDraw region: its big-endian header, then the whole record into a new
   handle). STATE: MATCH 100%, exe 111 / obj 111, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00516046
long _PICT_ReadRegion(ITStream *stream, void ***regionOut)
{
    long err;
    long e;
    void **h;
    short header[6];

    err = 0;
    h = NULL;
    *regionOut = NULL;
    e = DT_BufferRead(stream, header, kRegionHeader);
    if (e != 0)
        goto fail;
    header[0] = Swap16(header[0]);
    header[1] = Swap16(header[1]);
    header[2] = Swap16(header[2]);
    header[3] = Swap16(header[3]);
    header[4] = Swap16(header[4]);
    h = IndirectAllocate(header[0]);
    if (h == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    IndirectSetLock(h, 1);
    /* BUG: 11 header bytes copied, the rest read from offset 10: the 11th byte is read twice
       and the handle's last byte is never written */
    MemoryCopy(*h, header, kRegionHeader);
    e = DT_BufferRead(stream, (char *)*h + kRegionData, header[0] - kRegionHeader);
    if (e != 0) {
fail:
        err = e;
        goto done;
    }
    IndirectSetLock(h, 0);
    *regionOut = h;
    h = NULL;
done:
    if (h != NULL)
        IndirectDeallocate(h);
    return err;
}

/* _PICT_SkipPixPat (the pixel-pattern opcodes: a one-colour PixPat is skipped, a full one is not
   supported). STATE: MATCH 100%, exe 43 / obj 43, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00516187
long _PICT_SkipPixPat(ITStream *stream, PictState *s, const PictOpcode *op, ULong opcode)
{
    long err;
    long e;
    unsigned short patType;
    unsigned char skip[kPatRGBSkip + 2];

    err = 0;
    e = DT_BufferRead(stream, &patType, 2);
    if (e != 0)
        goto fail;
    patType = Swap16(patType);
    if (patType == kPatTypeFull) {
        err = kPictUnsupported;
    } else if (patType == kPatTypeRGB) {
        e = DT_BufferRead(stream, skip, kPatRGBSkip);
        if (e != 0) {
fail:
            err = e;
        }
    } else {
        err = kPictBadData;
    }
    return err;
}

/* _PICT_HandleBits (the bits opcodes BitsRect / BitsRgn, PackBitsRect / PackBitsRgn, DirectBitsRect /
   DirectBitsRgn: rows into the picture's image when they cover its frame, else skipped). STATE:
   MATCH 100.0%, exe 754 / obj 754, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00516201
long _PICT_HandleBits(ITStream *stream, PictState *s, const PictOpcode *op, ULong opcode)
{
    void **packed;
    long err;
    long e;
    ULong y;
    ULong x;
    ULong rowBytes;
    ULong n;
    ULong v;
    ULong c;
    unsigned char fits;
    unsigned char *row;
    ITColor *rgb;
    ULong height;
    ULong width;
    void **region;
    PictPixMap map;
    PictColorTable table;
    PictFrame srcRect;
    PictFrame dstRect;
    ITRect src;
    ITRect dst;
    ITPalette palette;
    unsigned short packType;
    short mode;
    unsigned char count8;
    unsigned short count16;

    err = 0;
    fits = 0;
    packed = NULL;
    row = NULL;
    rgb = NULL;
    region = NULL;
    packType = kPackTypeUnknown;
    switch (opcode) {
    case kOpBitsRect:
    case kOpBitsRgn:
        map.baseAddr = 0;
        e = _PICT_ReadPixMap(stream, &map);
        CheckErr(e);
        e = _PICT_ReadColorTable(stream, &table);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&srcRect);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&dstRect);
        CheckErr(e);
        e = DT_BufferRead(stream, &mode, 2);
        CheckErr(e);
        if (opcode == kOpBitsRgn) {
            e = _PICT_ReadRegion(stream, &region);
            CheckErr(e);
        }
        packType = kPackTypeNone;
        break;
    case kOpPackBitsRect:
    case kOpPackBitsRgn:
        map.baseAddr = 0;
        e = _PICT_ReadPixMap(stream, &map);
        CheckErr(e);
        e = _PICT_ReadColorTable(stream, &table);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&srcRect);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&dstRect);
        CheckErr(e);
        e = DT_BufferRead(stream, &mode, 2);
        CheckErr(e);
        if (opcode == kOpPackBitsRgn) {
            e = _PICT_ReadRegion(stream, &region);
            CheckErr(e);
        }
        if ((map.rowBytes & kRowBytesMask) < kPackMinRowBytes)
            packType = kPackTypeNone;
        else
            packType = kPackTypeDefault;
        break;
    case kOpDirectBitsRect:
    case kOpDirectBitsRgn:
        table.size = 0xffff;
        e = DT_BufferRead(stream, &map.baseAddr, 4);
        CheckErr(e);
        e = _PICT_ReadPixMap(stream, &map);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&srcRect);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&dstRect);
        CheckErr(e);
        e = DT_BufferRead(stream, &mode, 2);
        CheckErr(e);
        if (opcode == kOpDirectBitsRgn) {
            e = _PICT_ReadRegion(stream, &region);
            CheckErr(e);
        }
        packType = map.packType;
        if (packType == kPackTypeNone || (map.rowBytes & kRowBytesMask) < kPackMinRowBytes)
            packType = kPackTypeNone;
        break;
    }

    height = map.bounds.bottom - map.bounds.top;
    width = map.bounds.right - map.bounds.left;
    rowBytes = map.rowBytes & kRowBytesMask;
    DT_SetRect(&src, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
    /* BUG: built from srcRect again (dstRect is never used), so "fits" is srcRect == frame
       (exe 0x0051657c) */
    DT_SetRect(&dst, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
    if (DT_IsRectEqual(&src, &s->bounds) && DT_IsRectEqual(&src, &dst))
        fits = 1;

    if (fits) {
        switch (map.pixelSize) {
        case 1:
        case 2:
        case 4:
        case 8:
            /* BUG: DirectBits with an 8-bit PixMap gives 65536 entries, overrunning the
               256-entry local palette */
            palette.count = table.size + 1;
            for (y = 0; y < palette.count; y++) {
                palette.entry[y].alpha = 0xff;
                palette.entry[y].red = table.entry[y].red >> 8;
                palette.entry[y].green = table.entry[y].green >> 8;
                palette.entry[y].blue = table.entry[y].blue >> 8;
            }
            if (s->image != NULL && s->info.depth != 8)
                _DeferredImageDispose(s);
            if (s->image == NULL) {
                e = _DeferredImageCreate(s, 8, palette.count != 0 ? &palette : NULL, 0);
                CheckErr(e);
            }
            row = MemoryAllocate(rowBytes);
            if (row == NULL) {
                err = kOutOfMemory;
                goto done;
            }
            break;
        case 16:
        case 32:
            if (s->image != NULL && s->info.depth != map.pixelSize)
                _DeferredImageDispose(s);
            if (s->image == NULL) {
                e = _DeferredImageCreate(s, map.pixelSize, NULL, 0);
                CheckErr(e);
            }
            rgb = MemoryAllocate((rowBytes / s->info.bytesPerPixel + 1) * sizeof(ITColor));
            if (rgb == NULL) {
                err = kOutOfMemory;
                goto done;
            }
            break;
        default:
            err = kPictBadData;
            goto done;
        }
    }

    /* BUG: when the bits do not fit, packed stays NULL: plain and packType 2 rows are read
       through it and PackBits rows resize it */
    switch (packType) {
    case kPackTypeNone:
        n = rowBytes;
        if (fits) {
            packed = IndirectAllocate(n);
            if (packed == NULL) {
                err = kOutOfMemory;
                goto done;
            }
            IndirectSetLock(packed, 1);
        }
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, *packed, n);
            CheckErr(e);
            if (fits && map.pixelSize == 32) {
                /* BUG: steps x by the byte stride, so only every 4th pixel is written and rgb overruns */
                for (x = 0; x < width; x += 4) {
                    rgb[x].alpha = 0xff;
                    rgb[x].red = HandleBytes(packed)[x + 1];
                    rgb[x].green = HandleBytes(packed)[x + 2];
                    rgb[x].blue = HandleBytes(packed)[x + 3];
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (fits && map.pixelSize == 16) {
                for (x = 0; x < width; x++) {
                    v = Swap16(((unsigned short *)*packed)[x]);
                    rgb[x].alpha = 0xff;
                    c = (v >> 10) & 0x1f;
                    rgb[x].red = Expand5(c);
                    c = (v >> 5) & 0x1f;
                    rgb[x].green = Expand5(c);
                    c = v & 0x1f;
                    rgb[x].blue = Expand5(c);
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (fits && map.pixelSize == 8) {
                MemoryCopy(row, *packed, width);
                DT_ImageSetRawPixels(s->image, s->mask, y, row);
            }
        }
        break;
    case kPackTypeNoAlpha:
        n = rowBytes * 3 / 4;
        if (fits) {
            packed = IndirectAllocate(n);
            if (packed == NULL) {
                err = kOutOfMemory;
                goto done;
            }
            IndirectSetLock(packed, 1);
        }
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, *packed, n);
            CheckErr(e);
            if (fits) {
                /* BUG: steps x by the byte stride, so only every 3rd pixel is written and rgb overruns */
                for (x = 0; x < width; x += 3) {
                    rgb[x].alpha = 0xff;
                    rgb[x].red = HandleBytes(packed)[x];
                    rgb[x].green = HandleBytes(packed)[x + 1];
                    rgb[x].blue = HandleBytes(packed)[x + 2];
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            }
        }
        break;
    case kPackTypeDefault:
    case kPackTypeWords:
    case kPackTypeChannels:
        if (fits) {
            packed = IndirectAllocate(0);
            if (packed == NULL) {
                err = kOutOfMemory;
                goto done;
            }
        }
        for (y = 0; y < height; y++) {
            if (rowBytes <= kPackCountByte) {
                e = DT_BufferRead(stream, &count8, 1);
                CheckErr(e);
                n = count8;
            } else {
                e = DT_BufferRead(stream, &count16, 2);
                CheckErr(e);
                n = Swap16(count16);
            }
            e = IndirectSetSize(packed, n);
            CheckErr(e);
            IndirectSetLock(packed, 1);
            e = DT_BufferRead(stream, *packed, n);
            CheckErr(e);
            IndirectSetLock(packed, 0);
            if (fits && map.pixelSize == 32) {
                e = IndirectDecompress(packed, kTagPackBytes, NULL);
                CheckErr(e);
                for (x = 0; x < width; x++) {
                    rgb[x].alpha = 0xff;
                    rgb[x].red = HandleBytes(packed)[x];
                    rgb[x].green = HandleBytes(packed)[(rowBytes >> 2) + x];
                    rgb[x].blue = HandleBytes(packed)[(rowBytes >> 1) + x];
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (fits && map.pixelSize == 16) {
                e = IndirectDecompress(packed, kTagPackWords, NULL);
                CheckErr(e);
                for (x = 0; x < width; x++) {
                    v = Swap16(((unsigned short *)*packed)[x]);
                    rgb[x].alpha = 0xff;
                    c = (v >> 10) & 0x1f;
                    rgb[x].red = Expand5(c);
                    c = (v >> 5) & 0x1f;
                    rgb[x].green = Expand5(c);
                    c = v & 0x1f;
                    rgb[x].blue = Expand5(c);
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (fits && map.pixelSize == 8) {
                e = IndirectDecompress(packed, kTagPackBytes, NULL);
                CheckErr(e);
                MemoryCopy(row, *packed, width);
                DT_ImageSetRawPixels(s->image, s->mask, y, row);
            }
        }
        break;
    default:
        err = kPictBadData;
        break;
    }
done:
    if (row != NULL)
        MemoryDeallocate(row);
    if (rgb != NULL)
        MemoryDeallocate(rgb);
    if (packed != NULL)
        IndirectDeallocate(packed);
    if (region != NULL)
        IndirectDeallocate(region);
    return err;
}

/* _PICT_CheckFileType (a PICT: a non-empty frame, then zero padding up to a version opcode 0x1101 or
   0x0011 0x02ff). STATE: MATCH 100%, exe 89 / obj 89, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00516c7d
unsigned char _PICT_CheckFileType(const void *dataRef, ULong size)
{
    const unsigned char *data;
    PictFrame frame;

    data = dataRef;
    MemoryCopy(&frame, data + kPictFrameOffset, sizeof(frame));
    data += kPictOpcodeOffset;
    size -= kPictOpcodeOffset;
    if ((long long)Swap16(frame.top) < (long long)Swap16(frame.bottom)) {
    } else
        return 0;
    if ((long long)Swap16(frame.left) < (long long)Swap16(frame.right)) {
    } else
        return 0;
    while (size != 0 && *data == 0) {
        data++;
        size--;
    }
    if (size != 0 && data[0] == kPictVersionOp) {
    } else
        return 0;
    if (data[1] == kPictVersion1 || (data[1] == kPictVersion2 && data[2] == kPictVersion2Tail))
        return 1;
    return 0;
}

/* PICT_CheckFileType (a PICT at the start or after a 512-byte file header). STATE: MATCH 100%, exe 28 / obj 28, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00516d6b
unsigned char PICT_CheckFileType(const unsigned char *data, ULong size)
{
    if (_PICT_CheckFileType(data, size) != 0)
        return 1;
    if (size > kPictFileHeader &&
        _PICT_CheckFileType(data + kPictFileHeader, size - kPictFileHeader) != 0)
        return 1;
    return 0;
}

/* the opcodes below 0xa2 (Inside Macintosh: Imaging With QuickDraw, A-22) */
PictOpcode gDT_PictOpTable[kPictLastTableOp + 1] = {
    { 0, NULL, "" },                            /* 0x00 NOP */
    { kOpWordSize, NULL, "" },                  /* 0x01 Clip */
    { 8, NULL, "" },                            /* 0x02 BkPat */
    { 2, NULL, "" },                            /* 0x03 TxFont */
    { 1, NULL, "" },                            /* 0x04 TxFace */
    { 2, NULL, "" },                            /* 0x05 TxMode */
    { 4, NULL, "" },                            /* 0x06 SpExtra */
    { 4, NULL, "" },                            /* 0x07 PnSize */
    { 2, NULL, "" },                            /* 0x08 PnMode */
    { 8, NULL, "" },                            /* 0x09 PnPat */
    { 8, NULL, "" },                            /* 0x0a FillPat */
    { 4, NULL, "" },                            /* 0x0b OvSize */
    { 4, NULL, "" },                            /* 0x0c Origin */
    { 2, NULL, "" },                            /* 0x0d TxSize */
    { 4, NULL, "" },                            /* 0x0e FgColor */
    { 4, NULL, "" },                            /* 0x0f BkColor */
    { 8, NULL, "" },                            /* 0x10 TxRatio */
    { 1, NULL, "" },                            /* 0x11 VersionOp */
    { kOpHandler, _PICT_SkipPixPat, "" },           /* 0x12 BkPixPat */
    { kOpHandler, _PICT_SkipPixPat, "" },           /* 0x13 PnPixPat */
    { kOpHandler, _PICT_SkipPixPat, "" },           /* 0x14 FillPixPat */
    { 2, NULL, "" },                            /* 0x15 PnLocHFrac */
    { 2, NULL, "" },                            /* 0x16 ChExtra */
    { 0, NULL, "" },                            /* 0x17 reserved */
    { 0, NULL, "" },
    { 0, NULL, "" },
    { 6, NULL, "" },                            /* 0x1a RGBFgCol */
    { 6, NULL, "" },                            /* 0x1b RGBBkCol */
    { 0, NULL, "" },                            /* 0x1c HiliteMode */
    { 6, NULL, "" },                            /* 0x1d HiliteColor */
    { 0, NULL, "" },                            /* 0x1e DefHilite */
    { 6, NULL, "" },                            /* 0x1f OpColor */
    { 8, NULL, "" },                            /* 0x20 Line */
    { 4, NULL, "" },                            /* 0x21 LineFrom */
    { 6, NULL, "" },                            /* 0x22 ShortLine */
    { 2, NULL, "" },                            /* 0x23 ShortLineFrom */
    { kOpWordLength, NULL, "" },                /* 0x24 reserved */
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpLongText, NULL, "" },                  /* 0x28 LongText */
    { kOpDHText, NULL, "" },                    /* 0x29 DHText */
    { kOpDHText, NULL, "" },                    /* 0x2a DVText */
    { kOpDHDVText, NULL, "" },                  /* 0x2b DHDVText */
    { kOpLongText, NULL, "" },                  /* 0x2c fontName */
    { 10, NULL, "" },                           /* 0x2d lineJustify */
    { 8, NULL, "" },                            /* 0x2e glyphState */
    { kOpWordLength, NULL, "" },                /* 0x2f reserved */
    { 8, NULL, "" },                            /* 0x30 frameRect */
    { 8, NULL, "" },                            /* 0x31 paintRect */
    { 8, NULL, "" },                            /* 0x32 eraseRect */
    { 8, NULL, "" },                            /* 0x33 invertRect */
    { 8, NULL, "" },                            /* 0x34 fillRect */
    { 8, NULL, "" },                            /* 0x35 reserved */
    { 8, NULL, "" },
    { 8, NULL, "" },
    { 0, NULL, "" },                            /* 0x38 frameSameRect */
    { 0, NULL, "" },                            /* 0x39 paintSameRect */
    { 0, NULL, "" },                            /* 0x3a eraseSameRect */
    { 0, NULL, "" },                            /* 0x3b invertSameRect */
    { 0, NULL, "" },                            /* 0x3c fillSameRect */
    { 0, NULL, "" },                            /* 0x3d reserved */
    { 0, NULL, "" },
    { 0, NULL, "" },
    { 8, NULL, "" },                            /* 0x40 frameRRect */
    { 8, NULL, "" },                            /* 0x41 paintRRect */
    { 8, NULL, "" },                            /* 0x42 eraseRRect */
    { 8, NULL, "" },                            /* 0x43 invertRRect */
    { 8, NULL, "" },                            /* 0x44 fillRRect */
    { 8, NULL, "" },                            /* 0x45 reserved */
    { 8, NULL, "" },
    { 8, NULL, "" },
    { 0, NULL, "" },                            /* 0x48 frameSameRRect */
    { 0, NULL, "" },                            /* 0x49 paintSameRRect */
    { 0, NULL, "" },                            /* 0x4a eraseSameRRect */
    { 0, NULL, "" },                            /* 0x4b invertSameRRect */
    { 0, NULL, "" },                            /* 0x4c fillSameRRect */
    { 0, NULL, "" },                            /* 0x4d reserved */
    { 0, NULL, "" },
    { 0, NULL, "" },
    { 8, NULL, "" },                            /* 0x50 frameOval */
    { 8, NULL, "" },                            /* 0x51 paintOval */
    { 8, NULL, "" },                            /* 0x52 eraseOval */
    { 8, NULL, "" },                            /* 0x53 invertOval */
    { 8, NULL, "" },                            /* 0x54 fillOval */
    { 8, NULL, "" },                            /* 0x55 reserved */
    { 8, NULL, "" },
    { 8, NULL, "" },
    { 0, NULL, "" },                            /* 0x58 frameSameOval */
    { 0, NULL, "" },                            /* 0x59 paintSameOval */
    { 0, NULL, "" },                            /* 0x5a eraseSameOval */
    { 0, NULL, "" },                            /* 0x5b invertSameOval */
    { 0, NULL, "" },                            /* 0x5c fillSameOval */
    { 0, NULL, "" },                            /* 0x5d reserved */
    { 0, NULL, "" },
    { 0, NULL, "" },
    { 12, NULL, "" },                           /* 0x60 frameArc */
    { 12, NULL, "" },                           /* 0x61 paintArc */
    { 12, NULL, "" },                           /* 0x62 eraseArc */
    { 12, NULL, "" },                           /* 0x63 invertArc */
    { 12, NULL, "" },                           /* 0x64 fillArc */
    { 12, NULL, "" },                           /* 0x65 reserved */
    { 12, NULL, "" },
    { 12, NULL, "" },
    { 4, NULL, "" },                            /* 0x68 frameSameArc */
    { 4, NULL, "" },                            /* 0x69 paintSameArc */
    { 4, NULL, "" },                            /* 0x6a eraseSameArc */
    { 4, NULL, "" },                            /* 0x6b invertSameArc */
    { 4, NULL, "" },                            /* 0x6c fillSameArc */
    { 4, NULL, "" },                            /* 0x6d reserved */
    { 4, NULL, "" },
    { 4, NULL, "" },
    { kOpWordSize, NULL, "" },                  /* 0x70 framePoly */
    { kOpWordSize, NULL, "" },                  /* 0x71 paintPoly */
    { kOpWordSize, NULL, "" },                  /* 0x72 erasePoly */
    { kOpWordSize, NULL, "" },                  /* 0x73 invertPoly */
    { kOpWordSize, NULL, "" },                  /* 0x74 fillPoly */
    { kOpWordLength, NULL, "" },                /* 0x75 reserved */
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { 0, NULL, "" },                            /* 0x78 frameSamePoly */
    { 0, NULL, "" },                            /* 0x79 paintSamePoly */
    { 0, NULL, "" },                            /* 0x7a eraseSamePoly */
    { 0, NULL, "" },                            /* 0x7b invertSamePoly */
    { 0, NULL, "" },                            /* 0x7c fillSamePoly */
    { 0, NULL, "" },                            /* 0x7d reserved */
    { 0, NULL, "" },
    { 0, NULL, "" },
    { kOpWordSize, NULL, "" },                  /* 0x80 frameRgn */
    { kOpWordSize, NULL, "" },                  /* 0x81 paintRgn */
    { kOpWordSize, NULL, "" },                  /* 0x82 eraseRgn */
    { kOpWordSize, NULL, "" },                  /* 0x83 invertRgn */
    { kOpWordSize, NULL, "" },                  /* 0x84 fillRgn */
    { kOpWordLength, NULL, "" },                /* 0x85 reserved */
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { 0, NULL, "" },                            /* 0x88 frameSameRgn */
    { 0, NULL, "" },                            /* 0x89 paintSameRgn */
    { 0, NULL, "" },                            /* 0x8a eraseSameRgn */
    { 0, NULL, "" },                            /* 0x8b invertSameRgn */
    { 0, NULL, "" },                            /* 0x8c fillSameRgn */
    { 0, NULL, "" },                            /* 0x8d reserved */
    { 0, NULL, "" },
    { 0, NULL, "" },
    { kOpHandler, _PICT_HandleBits, "" },           /* 0x90 BitsRect */
    { kOpHandler, _PICT_HandleBits, "" },           /* 0x91 BitsRgn */
    { kOpWordLength, NULL, "" },                /* 0x92 reserved */
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpHandler, _PICT_HandleBits, "" },           /* 0x98 PackBitsRect */
    { kOpHandler, _PICT_HandleBits, "" },           /* 0x99 PackBitsRgn */
    { kOpHandler, _PICT_HandleBits, "" },           /* 0x9a DirectBitsRect */
    { kOpHandler, _PICT_HandleBits, "" },           /* 0x9b DirectBitsRgn */
    { kOpWordLength, NULL, "" },                /* 0x9c reserved */
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { kOpWordLength, NULL, "" },
    { 2, NULL, "" },                            /* 0xa0 ShortComment */
    { kOpLongComment, NULL, "" }                /* 0xa1 LongComment */
};

/* PICT_Decode (the PICT loader: header, version, then every opcode's data skipped or handed to
   its handler up to OpEndPic). STATE: MATCH 100.0%, exe 561 / obj 561, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00516dac
long PICT_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut, ITTransparent *trans,
                  ITBackground *bg)
{
    long err;
    long e;
    long version;
    long length;
    unsigned char unused;
    unsigned char skip[kPictSkipChunk];
    unsigned short picSize;
    PictFrame frame;
    unsigned short opcode;
    unsigned char b;
    PictState s;
    PictOpcode entry;

    err = 0;
    unused = 0;
    picSize = 0;
    version = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);
    MemoryClear(&s, sizeof(s));
    if (!_PICT_CheckFileType(*stream->data, stream->size)) {
        e = DT_BufferRead(stream, skip, kPictFileHeader);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = DT_BufferRead(stream, &picSize, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    picSize = Swap16(picSize);
    e = _PICT_ReadRect(stream, (short *)&frame);
    if (e != 0) {
        err = e;
        goto done;
    }
    do {
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
    } while (b == 0);
    if (b != kPictVersionOp)
        goto bad;
    e = DT_BufferRead(stream, &b, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    switch ((char)b) {
    case kPictVersion1:
        version = 1;
        break;
    case kPictVersion2:
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (b != kPictVersion2Tail)
            goto bad;
        version = 2;
        break;
    default:
        err = kPictBadData;
        goto done;
    }
    s.version = version;
    DT_SetRect(&s.bounds, frame.left, frame.top, frame.right, frame.bottom);
    s.trans = trans;
    s.bg = bg;

    for (;;) {
        PictOpcode wordData = { kOpWordLength, NULL, "" };
        PictOpcode longData = { kOpLongLength, NULL, "" };
        PictOpcode noData = { 0, NULL, "" };
        PictOpcode wordFixed = { 2, NULL, "" };
        PictOpcode longFixed = { 4, NULL, "" };
        unsigned short wordLength;
        ULong longLength;
        unsigned short wordSize;
        ULong longSize;
        unsigned short commentSize;

        e = _PICT_ReadOpCode(stream, version, &opcode);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (opcode <= kPictLastTableOp)
            entry = gDT_PictOpTable[opcode];
        else if (opcode <= kPictLastWordData)
            entry = wordData;
        else if (opcode <= kPictLastNoData)
            entry = noData;
        else if (opcode <= kPictLastLongData)
            entry = longData;
        else if (opcode == kPictOpEndPic)
            break;
        else if (opcode == kPictOpVersion) {
            entry.length = 2;
            entry.handler = NULL;
            entry.name = "versionOp";
        } else if (opcode == kPictOpHeader) {
            entry.length = kPictHeaderLength;
            entry.handler = NULL;
            entry.name = "headerOp";
        } else if (opcode <= kPictLastReserved) {
            entry.length = (opcode & 0x7f00) >> 7;
            entry.handler = NULL;
            entry.name = "reserved";
        } else if (opcode <= kPictLastReservedNoData)
            entry = noData;
        else if (opcode == kPictOpCompressedQT) {
            entry.length = kOpLongLength;
            entry.handler = NULL;
            entry.name = "compressedQuicktime";
        } else if (opcode == kPictOpUncompressedQT) {
            entry.length = kOpLongLength;
            entry.handler = NULL;
            entry.name = "uncompressedQuicktime";
        } else
            /* BUG: 0x8001..0x80ff take a long length too, where Apple gives them no data */
            entry = longData;

        if (entry.handler != NULL) {
            e = entry.handler(stream, &s, &entry, opcode);
            if (e != 0) {
                err = e;
                goto done;
            }
            continue;
        }

        if (entry.length == kOpHandler)
            goto bad;
        else if (entry.length == kOpWordLength) {
            e = DT_BufferRead(stream, &wordLength, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = Swap16(wordLength) - 2;
            if (length < 0) {
                err = kPictBadData;
                goto done;
            }
        } else if (entry.length == kOpLongLength) {
            e = DT_BufferRead(stream, &longLength, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = Swap32(longLength) - 4;
            if (length < 0) {
bad:
                err = kPictBadData;
                goto done;
            }
        }
        if (entry.length == kOpWordSize) {
            e = DT_BufferRead(stream, &wordSize, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = Swap16(wordSize) - 2;
        } else if (entry.length == kOpLongSize) {
            e = DT_BufferRead(stream, &longSize, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = Swap32(longSize) - 4;
        } else if (entry.length == kOpDHText) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = b;
        } else if (entry.length == kOpDHDVText) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = b;
        } else if (entry.length == kOpLongText) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = b;
        } else if (entry.length == kOpLongComment) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &commentSize, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = Swap16(commentSize);
        } else
            /* BUG: also overwrites the -2 / -3 lengths read above, so their skip runs on
               0xfffffffe / 0xfffffffd until a stream error */
            length = entry.length;

        while (length > sizeof(skip)) {
            e = DT_BufferRead(stream, skip, sizeof(skip));
            if (e != 0) {
                err = e;
                goto done;
            }
            length -= sizeof(skip);
        }
        e = DT_BufferRead(stream, skip, length);
        if (e != 0) {
            err = e;
            goto done;
        }
    }

    if (s.image == NULL) {
        e = _DeferredImageCreate(&s, kPictBlankDepth, NULL, 1);
        if (e != 0)
            err = e;
    }
done:
    if (s.locked)
        DT_ImageUnlock(s.image, s.mask);
    if (err != 0 && s.image != NULL)
        DT_ImageDispose(s.image);
    if (err != 0 && s.mask != NULL)
        DT_ImageDispose(s.mask);
    if (err == 0) {
        *imageOut = s.image;
        if (maskOut != NULL)
            *maskOut = s.mask;
    }
    return err;
}
