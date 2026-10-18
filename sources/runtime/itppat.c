// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itppat.c -- the interface tool's 'ppat' (QuickDraw pixel pattern) reader: the header test and the loader. Notes: Research/notes/itppat.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

/* the PixPat, its PixMap and its ColorTable, unpacked from the big-endian resource */
typedef struct PpatPixMap {
    long baseAddr;
    unsigned short rowBytes;
    short top;
    short left;
    short bottom;
    short right;
    short pmVersion;
    short packType;
    long packSize;
    ULong hRes;
    ULong vRes;
    short pixelType;
    short pixelSize;
    short cmpCount;
    short cmpSize;
    ULong planeBytes;
    ULong pmTable;
    long pmReserved;
} PpatPixMap;

typedef struct PpatRecord {
    short patType;
    ULong patMap;
    ULong patData;
    long patXData;
    short patXValid;
    long patXMap;
    unsigned char pat1Data[8];
    PpatPixMap map;
    ULong unk54;
    PictColorTable table;
} PpatRecord;

#define kFormatPpat         0x70706174    /* 'ppat' */
#define kPpatGamma          1.7
#define kRowBytesInvalid    0x4003        /* flag bits and an odd or unaligned row */
#define kCtabIndexed        0x8000        /* ctFlags: entries in order, values ignored */
#define kMinDepth           8

#define kPpatMinSize        0x4e
#define kPpatColorType      1
#define kPpatMapOffset      0x1c
#define kPpatMapAt          2
#define kPpatDataAt         6
#define kPpatHResAt         0x32
#define kPpatVResAt         0x36
#define kPpatPixelSizeAt    0x3c
#define kFixed72            0x00480000

#define SwapField(v)    ((((v) >> 8) & 0xff) | (((v) & 0xff) << 8))

/* PPAT_CheckFileType (a colour 'ppat': type 1, its PixMap at 0x1c, data after it, 72 dpi, a pixel
   size of 1, 2, 4, 8, 16 or 32; all fields big-endian). STATE: MATCH 100%, exe 187 / obj 187,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051770e
unsigned char PPAT_CheckFileType(const unsigned char *data, ULong size)
{
    unsigned short type;
    ULong value;
    long long depth;

    if (size >= kPpatMinSize) {
        MemoryCopy(&type, data, sizeof(type));
        if ((long long)SwapField(type) != kPpatColorType)
            return 0;
        MemoryCopy(&value, data + kPpatMapAt, sizeof(value));
        if ((long long)Swap32(value) != kPpatMapOffset)
            return 0;
        MemoryCopy(&value, data + kPpatDataAt, sizeof(value));
        if ((long long)Swap32(value) < kPpatMinSize)
            return 0;
        MemoryCopy(&value, data + kPpatHResAt, sizeof(value));
        if ((long long)Swap32(value) != kFixed72)
            return 0;
        MemoryCopy(&value, data + kPpatVResAt, sizeof(value));
        if ((long long)Swap32(value) != kFixed72)
            return 0;
        MemoryCopy(&type, data + kPpatPixelSizeAt, sizeof(type));
        if ((depth = SwapField(type)) == 1 || depth == 2 || depth == 4 || depth == 8 ||
            depth == 16 || depth == 32) {
        } else
            return 0;
        return 1;
    }
    return 0;
}

#define ReadShort(f)                                        \
    e = DT_BufferRead(stream, &(f), 2);                      \
    if (e != 0) {                                           \
        err = e;                                            \
        goto done;                                          \
    }                                                       \
    (f) = SwapField(f)

#define ReadLong(f)                                         \
    e = DT_BufferRead(stream, &(f), 4);                      \
    if (e != 0) {                                           \
        err = e;                                            \
        goto done;                                          \
    }                                                       \
    (f) = Swap32(f)

/* PPAT_Decode (load a 'ppat' off a stream: header, PixMap and optional ColorTable, then the
   unpacked rows at 1, 2, 4 or 8 bits through the palette, 16 or 32 bits direct). STATE: MATCH
   100%, exe 919 / obj 919, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00517979
long PPAT_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut,
                  ITTransparent *trans, ITBackground *bg)
{
    long err;
    long e;
    ULong y;
    unsigned char locked;
    unsigned char *row;
    unsigned char *indices;
    ITColor *pixels;
    ULong width;
    ULong height;
    ULong rowBytes;
    ITPalette palette;
    ITRect bounds;
    PpatRecord rec;
    ULong depth;
    ULong x;
    ULong i;
    ULong bit;
    ULong c;
    ULong v;
    ULong w;
    ULong index;
    ULong unused;

    err = 0;
    locked = 0;
    row = NULL;
    indices = NULL;
    pixels = NULL;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);
    MemoryClear(&rec, sizeof(rec));

    ReadShort(rec.patType);
    ReadLong(rec.patMap);
    ReadLong(rec.patData);
    ReadLong(rec.patXData);
    ReadShort(rec.patXValid);
    ReadLong(rec.patXMap);
    e = DT_BufferRead(stream, rec.pat1Data, sizeof(rec.pat1Data));
    if (e != 0) {
        err = e;
        goto done;
    }
    ReadLong(rec.map.baseAddr);
    ReadShort(rec.map.rowBytes);
    e = _PICT_ReadRect(stream, &rec.map.top);
    if (e != 0) {
        err = e;
        goto done;
    }
    width = rec.map.right - rec.map.left;
    height = rec.map.bottom - rec.map.top;
    rowBytes = rec.map.rowBytes & kRowBytesMask;
    ReadShort(rec.map.pmVersion);
    ReadShort(rec.map.packType);
    ReadLong(rec.map.packSize);
    ReadLong(rec.map.hRes);
    ReadLong(rec.map.vRes);
    ReadShort(rec.map.pixelType);
    ReadShort(rec.map.pixelSize);
    ReadShort(rec.map.cmpCount);
    ReadShort(rec.map.cmpSize);
    ReadLong(rec.map.planeBytes);
    ReadLong(rec.map.pmTable);
    ReadLong(rec.map.pmReserved);

    if (rec.map.left >= rec.map.right || rec.map.top >= rec.map.bottom ||
        (rec.map.rowBytes & kRowBytesInvalid) != 0 || rec.map.pmTable > stream->size)
        goto bad;
    if (rec.map.pmTable != 0 && rec.map.pmTable < stream->size) {
        stream->pos = rec.map.pmTable;
        e = _PICT_ReadColorTable(stream, &rec.table);
        if (e != 0) {
            err = e;
            goto done;
        }
        MemoryClear(&palette, sizeof(palette));
        palette.count = 1 << rec.map.pixelSize;
        for (i = 0; i <= rec.table.size; i++) {
            if ((rec.table.flags & kCtabIndexed) != 0)
                index = i;
            else
                index = rec.table.entry[i].value;
            unused = index;
            if (index < palette.count) {
                palette.entry[index].alpha = 0xff;
                palette.entry[index].red = rec.table.entry[i].red >> 8;
                palette.entry[index].green = rec.table.entry[i].green >> 8;
                palette.entry[index].blue = rec.table.entry[i].blue >> 8;
            }
        }
    }
    if (rec.patData > stream->size || rowBytes * height + rec.patData > stream->size ||
        rec.map.packType != 0) {
bad:
        err = kStreamOverrun;
        goto done;
    }

    DT_SetRect(&bounds, 0, 0, width, height);
    depth = rec.map.pixelSize < kMinDepth ? kMinDepth : rec.map.pixelSize;
    /* BUG: with no colour table, palette is passed uninitialised */
    *imageOut = DT_ImageCreate(&bounds, depth, depth == kMinDepth ? &palette : NULL);
    if (*imageOut == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    if (bg != NULL) {
        bg->format = kFormatPpat;
        bg->gamma = kPpatGamma;
    }
    row = (unsigned char *)MemoryAllocate(rowBytes);
    if (row == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    if (rec.map.pixelSize > kMinDepth) {
        pixels = (ITColor *)MemoryAllocate(width * sizeof(ITColor));
        if (pixels == NULL) {
            err = kOutOfMemory;
            goto done;
        }
    } else {
        indices = (unsigned char *)MemoryAllocate(width);
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
    stream->pos = rec.patData;

    switch (rec.map.pixelSize) {
    case 1:
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, row, rowBytes);
            if (e != 0) {
                err = e;
                goto done;
            }
            for (x = 0; x < width; x += 8) {
                c = row[x >> 3];
                for (bit = 0; bit < 8; bit++, c >>= 1)
                    indices[x + 7 - bit] = c & 1;
            }
            DT_ImageSetRawPixels(*imageOut, NULL, y, indices);
        }
        break;
    case 2:
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, row, rowBytes);
            if (e != 0) {
                err = e;
                goto done;
            }
            for (x = 0; x < width; x += 4) {
                c = row[x >> 2];
                for (bit = 0; bit < 4; bit++, c >>= 2)
                    indices[x + 3 - bit] = c & 3;
            }
            DT_ImageSetRawPixels(*imageOut, NULL, y, indices);
        }
        break;
    case 4:
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, row, rowBytes);
            if (e != 0) {
                err = e;
                goto done;
            }
            for (x = 0; x < width; x += 2) {
                indices[x] = (row[x >> 1] >> 4) & 0xf;
                indices[x + 1] = row[x >> 1] & 0xf;
            }
            DT_ImageSetRawPixels(*imageOut, NULL, y, indices);
        }
        break;
    case 8:
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, row, rowBytes);
            if (e != 0) {
                err = e;
                goto done;
            }
            DT_ImageSetRawPixels(*imageOut, NULL, y, row);
        }
        break;
    case 16:
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, row, rowBytes);
            if (e != 0) {
                err = e;
                goto done;
            }
            for (x = 0; x < width; x++) {
                ULong chan;

                /* BUG: read little-endian, where the pattern's words are big-endian */
                v = ((unsigned short *)row)[x];
                pixels[x].alpha = 0xff;
                chan = (v >> 10) & 0x1f;
                pixels[x].red = (unsigned char)(chan << 3) | (unsigned char)(chan >> 2);
                chan = (v >> 5) & 0x1f;
                pixels[x].green = (unsigned char)(chan << 3) | (unsigned char)(chan >> 2);
                chan = v & 0x1f;
                pixels[x].blue = (unsigned char)(chan << 3) | (unsigned char)(chan >> 2);
            }
            DT_ImageSetRawColors(*imageOut, NULL, y, pixels);
        }
        break;
    case 32:
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, row, rowBytes);
            if (e != 0) {
                err = e;
                goto done;
            }
            for (x = 0; x < width; x++) {
                /* BUG: read little-endian, where the pattern's longs are big-endian */
                w = ((ULong *)row)[x];
                pixels[x].alpha = 0xff;
                pixels[x].red = (w >> 16) & 0xff;
                pixels[x].green = (w >> 8) & 0xff;
                pixels[x].blue = w & 0xff;
            }
            DT_ImageSetRawColors(*imageOut, NULL, y, pixels);
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
