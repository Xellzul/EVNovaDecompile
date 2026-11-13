// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itpict.c -- the interface tool's PICT reader: the opcode loader, its table and handlers, the stream readers. Notes: Research/notes/itpict.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "macport.h"

#ifdef MAC1010
#define FromBE16(x) (x)
#define WideBE16(x) (x)
#define IMAGE_RECT imageBounds
#define FromBE32(x) (x)
#define PictLocal static            /* local symbols in the 1.1.1 framework */
#else
#define FromBE16(x) Swap16(x)
#define WideBE16(x) ((long long)Swap16(x))
#define IMAGE_RECT bounds
#define FromBE32(x) Swap32(x)
#define PictLocal
#endif

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
    int32_t baseAddr;
    unsigned short rowBytes;
    PictFrame bounds;
    short pmVersion;
    short packType;
    int32_t packSize;
    int32_t hRes;
    int32_t vRes;
    short pixelType;
    short pixelSize;
    short cmpCount;
    short cmpSize;
    int32_t planeBytes;
    int32_t pmTable;
    int32_t pmReserved;
} PictPixMap;

/* the picture being drawn: its frame, version, target images and the loader's answers */
typedef struct PictState {
    ITRect bounds;
    short version;
#ifdef MAC1010
    ITRect imageBounds;
#endif
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
typedef int32_t (*PictHandler)(ITStream *stream, PictState *s, const struct PictOpcode *op,
                            ULong opcode);
typedef struct PictOpcode {
    ULong length;
    PictHandler handler;
#ifdef MAC1010
    unsigned char unsupported;  /* the ops that draw: the decoder refuses a picture holding one */
#endif
    const char *name;
} PictOpcode;

#ifdef MAC1010
#define PICT_OP(length, handler)    { length, handler, 0, "" }
#define PICT_DRAW_OP(length)        { length, NULL, 1, "" }
#else
#define PICT_OP(length, handler)    { length, handler, "" }
#define PICT_DRAW_OP(length)        { length, NULL, "" }
#endif

/* _DeferredImageCreate (create the picture's image in a format and palette, lock it, and when asked
   paint it white). STATE: MATCH 100%, exe 61 / obj 61, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515931
int32_t _DeferredImageCreate(PictState *s, ULong format, const ITPalette *palette, unsigned char white)
{
    int32_t err;
    int32_t e;
    ULong width;
    ULong x;
    ULong y;
    ULong height;
    unsigned char *p;

    err = 0;
    if ((s->image = DT_ImageCreate(&s->IMAGE_RECT, format, palette)) == NULL) {
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
                height = s->IMAGE_RECT.bottom - s->IMAGE_RECT.top;
                width = s->IMAGE_RECT.right - s->IMAGE_RECT.left;
                width *= s->info.bytesPerPixel;
                p = s->info.bits;
                for (y = 0; y < height; y++) {
                    x = 0;
                    while (x < width)
                        p[x++] = 0xff;
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
    int32_t err;

    err = 0;
#ifdef MAC1010
    s->imageBounds = s->bounds;
#endif
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
int32_t _PICT_ReadOpCode(ITStream *stream, unsigned short version, unsigned short *opcode)
{
    int32_t err;
    int32_t e;
    unsigned char b;
    unsigned short w;

    err = 0;
    if (version == kPictVersionV1) {
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        *opcode = b;
    } else {
        if ((stream->pos & 1) != 0) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        e = DT_BufferRead(stream, &w, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        *opcode = FromBE16(w);
    }
done:
    return err;
}

/* _PICT_ReadRect (a big-endian rectangle, four words). STATE: MATCH 100%, exe 49 / obj 49,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515ac9
int32_t _PICT_ReadRect(ITStream *stream, short *rect)
{
    int32_t err;
    int32_t e;

    err = 0;
    e = DT_BufferRead(stream, rect, 8);
    if (e != 0) {
        err = e;
    } else {
        rect[0] = FromBE16(rect[0]);
        rect[1] = FromBE16(rect[1]);
        rect[2] = FromBE16(rect[2]);
        rect[3] = FromBE16(rect[3]);
    }
    return err;
}

#define ReadWord(f)                                         \
    e = DT_BufferRead(stream, &(f), 2);                      \
    if (e != 0) {                                       \
        err = e;                                        \
        goto done;                                      \
    }                                                   \
    (f) = FromBE16(f)

#define ReadLong(f)                                         \
    e = DT_BufferRead(stream, &(f), 4);                      \
    if (e != 0) {                                       \
        err = e;                                        \
        goto done;                                      \
    }                                                   \
    (f) = FromBE32(f)

#ifdef MAC1010
#define kPictPixMapFlag     0x8000          /* rowBytes: a PixMap, not a BitMap */
#define kPict72Dpi          0x00480000      /* Fixed */

int32_t _PICT_ReadPixMapTable(ITStream *stream, PictPixMap *map, PictColorTable *table)
{
    int32_t err;
    int32_t e;
    ULong last;
    ULong i;

    err = 0;
    e = DT_BufferRead(stream, &map->rowBytes, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _PICT_ReadRect(stream, (short *)&map->bounds);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (map->rowBytes & kPictPixMapFlag) {
        e = DT_BufferRead(stream, &map->pmVersion, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->packType, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->packSize, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->hRes, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->vRes, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->pixelType, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->pixelSize, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->cmpCount, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->cmpSize, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->planeBytes, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->pmTable, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &map->pmReserved, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &table->seed, 4);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &table->flags, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_BufferRead(stream, &table->size, 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        if ((last = table->size) > kColorTableMax) {
            err = kStreamOverrun;
            goto done;
        }
        for (i = 0; i <= last; i++) {
            e = DT_BufferRead(stream, &table->entry[i], sizeof(PictColorSpec));
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    } else {
        map->pmVersion = 0;
        map->packType = 0;
        map->packSize = 0;
        map->hRes = kPict72Dpi;
        map->vRes = kPict72Dpi;
        map->pixelType = 0;
        map->pixelSize = 1;
        map->cmpCount = 1;
        map->cmpSize = 1;
        map->planeBytes = 0;
        map->pmTable = 0;
        map->pmReserved = 0;
        table->seed = 0;
        table->size = 1;
        table->flags = 0;
        table->entry[0].value = 0;
        table->entry[0].red = 0xffff;
        table->entry[0].green = 0xffff;
        table->entry[0].blue = 0xffff;
        table->entry[1].value = 1;
        table->entry[1].red = 0;
        table->entry[1].green = 0;
        table->entry[1].blue = 0;
    }
done:
    return err;
}
#endif

/* _PICT_ReadPixMap (a PixMap record after its baseAddr, big-endian). STATE: MATCH 100%, exe 279 / obj
   279, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515b5f
int32_t _PICT_ReadPixMap(ITStream *stream, PictPixMap *map)
{
    int32_t err;
    int32_t e;

    err = 0;
    ReadWord(map->rowBytes);
    e = _PICT_ReadRect(stream, (short *)&map->bounds);
    if (e != 0) {
        err = e;
        goto done;
    }
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
        err = e;
    } else {
        map->pmReserved = FromBE32(map->pmReserved);
    }
done:
    return err;
}

/* _PICT_ReadColorTable (a big-endian ColorTable; kStreamOverrun past 256 entries). STATE: MATCH 100%,
   exe 122 / obj 122, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00515eb8
int32_t _PICT_ReadColorTable(ITStream *stream, PictColorTable *table)
{
    int32_t err;
    int32_t e;
    ULong i;
    ULong last;

    err = 0;
    e = DT_BufferRead(stream, &table->seed, 4);
    if (e != 0) {
        err = e;
        goto done;
    }
    table->seed = FromBE32(table->seed);
    e = DT_BufferRead(stream, &table->flags, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    table->flags = FromBE16(table->flags);
    e = DT_BufferRead(stream, &table->size, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    table->size = FromBE16(table->size);
    if (table->size > kColorTableMax) {
        err = kStreamOverrun;
        goto done;
    }
    for (i = 0, last = table->size; i <= last; i++) {
        e = DT_BufferRead(stream, &table->entry[i], sizeof(PictColorSpec));
        if (e != 0) {
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
int32_t _PICT_ReadRegion(ITStream *stream, void ***regionOut)
{
    int32_t err;
    int32_t e;
    void **h;
    short header[6];

    err = 0;
    h = NULL;
    *regionOut = NULL;
    e = DT_BufferRead(stream, header, kRegionHeader);
    if (e != 0) {
        err = e;
        goto done;
    }
    header[0] = FromBE16(header[0]);
    header[1] = FromBE16(header[1]);
    header[2] = FromBE16(header[2]);
    header[3] = FromBE16(header[3]);
    header[4] = FromBE16(header[4]);
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
PictLocal int32_t _PICT_SkipPixPat(ITStream *stream, PictState *s, const PictOpcode *op, ULong opcode)
{
    int32_t err;
    int32_t e;
    unsigned short patType;
    unsigned char skip[kPatRGBSkip + 2];

    err = 0;
    e = DT_BufferRead(stream, &patType, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    patType = FromBE16(patType);
    if (patType == kPatTypeFull) {
        err = kPictUnsupported;
    } else if (patType == kPatTypeRGB) {
        e = DT_BufferRead(stream, skip, kPatRGBSkip);
        if (e != 0) {
            err = e;
        }
    } else {
        err = kPictBadData;
    }
done:
    return err;
}

#ifdef MAC1010
static int32_t _PICT_HandleBits(ITStream *stream, PictState *s, const PictOpcode *op, ULong opcode);
#include "../runtime/pictoptable.inc"          /* 1.0.10 spelling: the table is made ahead of the bits handler's jump table */
#endif

/* _PICT_HandleBits (the bits opcodes BitsRect / BitsRgn, PackBitsRect / PackBitsRgn, DirectBitsRect /
   DirectBitsRgn: rows into the picture's image when they cover its frame, else skipped). STATE:
   MATCH 100.0%, exe 754 / obj 754, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00516201
PictLocal int32_t _PICT_HandleBits(ITStream *stream, PictState *s, const PictOpcode *op, ULong opcode)
#ifdef MAC1010
#include "../runtime/picthandlebits1010.inc"
#else
{
    void **packed;
    int32_t err;
    int32_t e;
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
                    v = FromBE16(((unsigned short *)*packed)[x]);
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
                n = FromBE16(count16);
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
                    v = FromBE16(((unsigned short *)*packed)[x]);
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
#endif

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
    if (WideBE16(frame.top) < WideBE16(frame.bottom)) {
    } else
        return 0;
    if (WideBE16(frame.left) < WideBE16(frame.right)) {
    } else
        return 0;
    while (size != 0 && *data == 0) {
        data++;
        size--;
    }
    if (size != 0 && data[0] == kPictVersionOp) {
    } else
        return 0;
    if (data[1] == kPictVersion1)
        return 1;
    if (data[1] == kPictVersion2 && data[2] == kPictVersion2Tail)
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

#ifndef MAC1010
#include "../runtime/pictoptable.inc"
#endif

/* PICT_Decode (the PICT loader: header, version, then every opcode's data skipped or handed to
   its handler up to OpEndPic). STATE: MATCH 100.0%, exe 561 / obj 561, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00516dac
int32_t PICT_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut, ITTransparent *trans,
                  ITBackground *bg)
#ifdef MAC1010
#include "../runtime/pictdecode1010.inc"
#else
{
    int32_t err;
    int32_t e;
    unsigned short version;
    int32_t length;
    unsigned char unused;
    unsigned char skip[kPictSkipChunk];
    unsigned short picSize;
    PictFrame frame;
    unsigned short opcode;
    char b;
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
    picSize = FromBE16(picSize);
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
    if (b != kPictVersionOp) {
        err = kPictBadData;
        goto done;
    }
    e = DT_BufferRead(stream, &b, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    switch (b) {
    case kPictVersion1:
        version = 1;
        break;
    case kPictVersion2:
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (b != (char)kPictVersion2Tail) {
            err = kPictBadData;
            goto done;
        }
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
        PictOpcode wordData = PICT_OP(kOpWordLength, NULL);
        PictOpcode longData = PICT_OP(kOpLongLength, NULL);
        PictOpcode noData = PICT_OP(0, NULL);
        PictOpcode wordFixed = PICT_OP(2, NULL);
        PictOpcode longFixed = PICT_OP(4, NULL);
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

        if (entry.length == kOpHandler) {
            err = kPictBadData;
            goto done;
        }
        else if (entry.length == kOpWordLength) {
            e = DT_BufferRead(stream, &wordLength, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE16(wordLength) - 2;
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
            length = FromBE32(longLength) - 4;
            if (length < 0) {
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
            length = FromBE16(wordSize) - 2;
        } else if (entry.length == kOpLongSize) {
            e = DT_BufferRead(stream, &longSize, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE32(longSize) - 4;
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
            length = (unsigned char)b;
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
            length = (unsigned char)b;
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
            length = (unsigned char)b;
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
            length = FromBE16(commentSize);
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
#endif
