// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itgif.c -- the interface tool's GIF reader: the signature test, the loader, its LZW decoder, the stream readers and the row writer. Notes: Research/notes/itgif.md */

#include <stddef.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

typedef struct GIFScreen {
    unsigned short width;                        /* +0x0 */
    unsigned short height;                       /* +0x2 */
    unsigned char bits;                          /* +0x4 */
    unsigned char resolution;                    /* +0x5 */
    unsigned char background;                    /* +0x6 */
    unsigned char hasMap;                        /* +0x7 */
    short mapSize;                               /* +0x8 */
    unsigned char map[256][3];                   /* +0xa */
} GIFScreen;

typedef struct GIFImage {
    unsigned short top;                          /* +0x0 */
    unsigned short left;                         /* +0x2 */
    unsigned short width;                        /* +0x4 */
    unsigned short height;                       /* +0x6 */
    unsigned char bits;                          /* +0x8 */
    unsigned char transparent;                   /* +0x9 */
    unsigned char hasTransparent;                /* +0xa */
    unsigned char interlaced;                    /* +0xb */
    unsigned char hasMap;                        /* +0xc */
    unsigned char padD;                          /* +0xd */
    short mapSize;                               /* +0xe */
    unsigned char map[256][3];                   /* +0x10 */
} GIFImage;

/* Placement only: no reader in the exe. */
static unsigned char sGifPad[8];            /* 0x0086ba20 */

#define kLzwCodes           0x1000
#define kLzwMaxBits         12

/* the LZW decoder's state, allocated with room for one row of pixels after it */
typedef struct GIFCodeReader {
    short codeSize;                              /* +0x000 */
    short maxCode;
    short clearCode;
    short endCode;
    short firstFree;
    short freeCode;
    short bitsLeft;                              /* +0x00c */
    unsigned char blockLeft;                     /* +0x00e */
    unsigned char current;                       /* +0x00f */
    unsigned char block[0x104];                  /* +0x010 */
    unsigned char *next;                         /* +0x114 */
    short prefix[kLzwCodes];                     /* +0x118 */
    unsigned char suffix[kLzwCodes];             /* +0x2118 */
    unsigned char stack[kLzwCodes];              /* +0x3118 */
    unsigned char row[4];                        /* +0x4118, the row buffer */
} GIFCodeReader;

/* where the decoded rows go: the interlace pass and the next row */
typedef struct GIFRows {
    unsigned char transparent;
    unsigned char hasTransparent;
    unsigned char interlaced;
    unsigned short pass;
    unsigned short width;
    unsigned short height;
    unsigned short column;
    unsigned short row;
    unsigned short rowsDone;
    ULong unk10;
    ULong unk14;
} GIFRows;

#define kGifSignatureSize   6
#define kGifImageSeparator  ','
#define kGifExtension       '!'
#define kGifGraphicControl  0xf9
#define kGifHasTransparent  0x01
#define kGifMapBits         0x07
#define kGifResolution      0x70
#define kGifInterlaced      0x40
#define kGifHasMap          0x80
#define kGifPasses          4
#define kFormatGif          0x47494666    /* 'GIFf' */
#define kGifBadData         -7301

long DT_GifReadByte(struct ITStream *stream, unsigned char *b);
long DT_GifReadSignature(struct ITStream *stream, char *signature);
long DT_GifReadScreen(struct ITStream *stream, GIFScreen *screen);
long DT_GifReadImageDesc(struct ITStream *stream, GIFImage *image);
long _ReadNextCode(struct ITStream *stream, GIFCodeReader *r, short *code);
long _WriteImageLine(GIFRows *rows, ITImage *image, const unsigned char *pixels, unsigned short count);
long DT_GifDecodeLZW(GIFRows *rows, ITStream *stream, ITImage *image);

/* GIF_CheckFileType (a "GIF87a" or "GIF89a" signature). STATE: MATCH 100%, exe 32 / obj 32,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00513794
unsigned char GIF_CheckFileType(const unsigned char *data, ULong size)
{
    char signature[kGifSignatureSize + 2];

    if (size >= kGifSignatureSize) {
        MemoryCopy(signature, data, kGifSignatureSize);
        signature[kGifSignatureSize] = 0;
        if (StringCompare(signature, "GIF87a") == 0 || StringCompare(signature, "GIF89a") == 0)
            return 1;
    }
    return 0;
}

/* 0x00579b18 / 0x00579b20: each interlace pass's row step and first row. */
unsigned short gDT_GifRowStep[kGifPasses] = { 8, 8, 4, 2 };
unsigned short gDT_GifRowStart[kGifPasses] = { 0, 4, 2, 1 };

/* 0x00579b28: the low n bits, n = 0..16. */
ULong gDT_GifCodeMask[17] = {
    0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

/* GIF_Decode (load the first image of a GIF off a stream into an 8-bit image over its local or
   global colour map). STATE: MATCH 100%, exe 263 / obj 263, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005137eb
long GIF_Decode(ITStream *stream, ITImage **imageOut, ITImage **maskOut,
                  ITTransparent *trans, ITBackground *bg)
{
    long err;
    long e;
    ULong useTransparent;
    unsigned short mapSize;
    unsigned char locked;
    char signature[kGifSignatureSize + 2];
    ITRect bounds;
    ITPalette palette;
    GIFScreen screen;
    GIFImage image;
    GIFRows rows;
    unsigned char (*map)[3];
    ULong depth;
    ULong is89;
    ULong i;

    err = 0;
    locked = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);
    e = DT_GifReadSignature(stream, signature);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (StringCompare(signature, "GIF87a") != 0 && StringCompare(signature, "GIF89a") != 0) {
        err = kGifBadData;
        goto done;
    }
    e = DT_GifReadScreen(stream, &screen);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadImageDesc(stream, &image);
    if (e != 0)
        goto fail;
    if (image.hasMap) {
        mapSize = image.mapSize;
        map = image.map;
        depth = (1 << image.bits) <= mapSize ? image.bits : kIndexedDepth;
    } else if (screen.hasMap) {
        mapSize = screen.mapSize;
        map = screen.map;
        depth = (1 << screen.bits) <= mapSize ? screen.bits : kIndexedDepth;
    } else {
        err = kGifBadData;
        goto done;
    }

    useTransparent = 0;
    is89 = 0;
    if (StringCompare(signature, "GIF89a") == 0)
        is89 = 1;
    if (is89 && ((screen.hasMap || image.hasTransparent) != 0))
        useTransparent = 1;
    rows.hasTransparent = useTransparent ? (unsigned char)1 : (unsigned char)0;
    rows.transparent = image.hasTransparent ? image.transparent : screen.background;
    rows.interlaced = image.interlaced;
    rows.pass = 0;
    rows.column = rows.row = 0;
    rows.rowsDone = 0;
    rows.height = image.height;
    rows.width = image.width;
    rows.unk14 = 0;
    DT_SetRect(&bounds, image.left, image.top, image.left + image.width,
                 image.top + image.height);
    palette.count = mapSize;
    for (i = 0; i < mapSize; i++) {
        palette.entry[i].alpha = 0xff;
        palette.entry[i].red = map[i][0];
        palette.entry[i].green = map[i][1];
        palette.entry[i].blue = map[i][2];
    }
    if (rows.hasTransparent)
        palette.entry[rows.transparent].alpha = 0;
    if ((*imageOut = DT_ImageCreate(&bounds, kIndexedDepth, &palette)) == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_ImageLock(*imageOut, NULL, NULL, NULL);
    if (e != 0) {
fail:
        err = e;
        goto done;
    }
    locked = 1;
    if (bg != NULL) {
        bg->format = kFormatGif;
        bg->interlaced = rows.interlaced ? (unsigned char)1 : (unsigned char)0;
        bg->gamma = 0.0;
    }
    if (trans != NULL && rows.hasTransparent) {
        trans->mode = kTransparentKey;
        trans->key.alpha = rows.transparent;
    }
    e = DT_GifDecodeLZW(&rows, stream, *imageOut);
    if (e != 0)
        err = e;
done:
    if (locked)
        DT_ImageUnlock(*imageOut, NULL);
    if (err != 0 && *imageOut != NULL) {
        DT_ImageDispose(*imageOut);
        *imageOut = NULL;
    }
    return err;
}

/* DT_GifDecodeLZW (the LZW decoder: codes off the image's data sub-blocks, pixels a row at a
   time to the row writer). STATE: MATCH 100%, exe 239 / obj 239, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00513bb1
long DT_GifDecodeLZW(GIFRows *rows, ITStream *stream, ITImage *image)
{
    long err;
    long e;
    GIFCodeReader *d;
    unsigned char *sp;
    unsigned char *out;
    short bad;
    short firstChar;
    unsigned char codeSize;
    short code;
    short cur;
    short oldCode;
    short left;

    err = 0;
    d = NULL;
    bad = 0;
    e = DT_GifReadByte(stream, &codeSize);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (codeSize < 2 || codeSize > 9) {
        err = kGifBadData;
        goto done;
    }
    d = (GIFCodeReader *)MemoryAllocate(sizeof(GIFCodeReader) + rows->width);
    if (d == NULL) {
        err = kOutOfMemory;
        goto done;
    }
    d->codeSize = codeSize + 1;
    d->maxCode = 1 << d->codeSize;
    d->clearCode = 1 << codeSize;
    d->endCode = d->clearCode + 1;
    d->firstFree = d->endCode + 1;
    d->freeCode = d->firstFree;
    d->blockLeft = d->bitsLeft = 0;
    oldCode = firstChar = 0;
    sp = d->stack;
    out = d->row;
    left = rows->width;

    while ((err = _ReadNextCode(stream, d, &code)) == 0 && code != d->endCode) {
        if (code == d->clearCode) {
            d->codeSize = codeSize + 1;
            d->freeCode = d->firstFree;
            d->maxCode = 1 << d->codeSize;
            do {
                err = _ReadNextCode(stream, d, &code);
            } while (err == 0 && code == d->clearCode);
            e = err;
            if (err != 0)
                goto done;
            if (code == d->endCode)
                break;
            if (code >= d->freeCode)
                code = 0;
            oldCode = firstChar = code;
            *out++ = code;
            if (--left == 0) {
                e = _WriteImageLine(rows, image, d->row, rows->width);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                out = d->row;
                left = rows->width;
            }
            continue;
        }
        cur = code;
        if (cur >= d->freeCode) {
            if (cur > d->freeCode)
                bad++;
            cur = oldCode;
            *sp++ = firstChar;
        }
        while (cur >= d->firstFree) {
            *sp++ = d->suffix[cur];
            cur = d->prefix[cur];
        }
        *sp++ = cur;
        if (d->freeCode < d->maxCode) {
            d->suffix[d->freeCode] = firstChar = cur;
            d->prefix[d->freeCode++] = oldCode;
            oldCode = code;
        }
        if (d->freeCode >= d->maxCode && d->codeSize < kLzwMaxBits) {
            d->maxCode <<= 1;
            d->codeSize++;
        }
        while (sp > d->stack) {
            *out++ = *--sp;
            if (--left == 0) {
                e = _WriteImageLine(rows, image, d->row, rows->width);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                out = d->row;
                left = rows->width;
            }
        }
    }
    e = err;
    if (err != 0)
        goto done;
    if (left != rows->width) {
        e = _WriteImageLine(rows, image, d->row, rows->width - left);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (bad > 0)
        err = kGifBadData;
done:
    if (d != NULL)
        MemoryDeallocate(d);
    return err;
}

/* DT_GifReadByte (one byte off the stream). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00513ebd
long DT_GifReadByte(struct ITStream *stream, unsigned char *b)
{
    return DT_BufferRead(stream, b, 1);
}

/* DT_GifReadWord (a little-endian word off the stream). STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00513ed0
long DT_GifReadWord(struct ITStream *stream, unsigned short *w)
{
    long err;
    long e;

    err = 0;
    e = DT_BufferRead(stream, w, 2);
    if (e != 0)
        err = e;
    return err;
}

/* DT_GifReadBytes (n bytes off the stream). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00513eef
long DT_GifReadBytes(struct ITStream *stream, void *dst, ULong count)
{
    return DT_BufferRead(stream, dst, count);
}

/* DT_GifReadSignature (the six signature bytes off the stream, terminated). STATE: MATCH 100%, exe 19 /
   obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00513f04
long DT_GifReadSignature(struct ITStream *stream, char *signature)
{
    long err;
    long e;

    err = 0;
    e = DT_BufferRead(stream, signature, kGifSignatureSize);
    if (e != 0)
        err = e;
    else
        signature[kGifSignatureSize] = 0;
    return err;
}

/* DT_GifReadScreen (the logical screen descriptor and its global colour map). STATE: MATCH 100%, exe
   88 / obj 88, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00513f2d
long DT_GifReadScreen(struct ITStream *stream, GIFScreen *screen)
{
    long err;
    long e;
    unsigned char flags;
    unsigned char hasMap;

    err = 0;
    e = DT_GifReadWord(stream, &screen->width);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadWord(stream, &screen->height);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadByte(stream, &flags);
    if (e != 0) {
        err = e;
        goto done;
    }
    screen->resolution = ((flags & kGifResolution) >> 4) + 1;
    screen->bits = (flags & kGifMapBits) + 1;
    screen->mapSize = 1 << screen->bits;
    if (flags & kGifHasMap)
        hasMap = 1;
    else
        hasMap = 0;
    screen->hasMap = hasMap;
    e = DT_GifReadByte(stream, &screen->background);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadByte(stream, &flags);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (screen->hasMap != 0) {
        e = DT_GifReadBytes(stream, screen->map, screen->mapSize * 3);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* DT_GifReadImageDesc (the next image descriptor and its local colour map, skipping extensions; a
   graphic control extension's transparent index is kept). STATE: MATCH 100%, exe 168 / obj 168,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00513ff7
long DT_GifReadImageDesc(struct ITStream *stream, GIFImage *image)
{
    long err;
    long e;
    unsigned char hasTransparent;
    unsigned char transparent;
    unsigned char b;
    unsigned char interlaced;
    unsigned char hasMap;
    unsigned char block[256];

    err = 0;
    hasTransparent = 0;
    transparent = 0;
    for (;;) {
        e = DT_GifReadByte(stream, &b);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (b == kGifImageSeparator)
            break;
        if (b != kGifExtension)
            continue;
        e = DT_GifReadByte(stream, &b);
        if (e != 0) {
            err = e;
            goto done;
        }
        switch (b) {
        case kGifGraphicControl:
            e = DT_GifReadByte(stream, &b);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_GifReadBytes(stream, block, b + 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (b >= 4 && (block[0] & kGifHasTransparent) != 0) {
                transparent = block[3];
                hasTransparent = 1;
            }
            break;
        default:
            e = DT_GifReadByte(stream, &b);
            if (e != 0) {
                err = e;
                goto done;
            }
            while (b != 0) {
                e = DT_GifReadBytes(stream, block, b);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_GifReadByte(stream, &b);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
            break;
        }
    }
    e = DT_GifReadWord(stream, &image->left);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadWord(stream, &image->top);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadWord(stream, &image->width);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadWord(stream, &image->height);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GifReadByte(stream, &b);
    if (e != 0) {
        err = e;
        goto done;
    }
    image->bits = (b & kGifMapBits) + 1;
    image->mapSize = 1 << image->bits;
    if (b & kGifInterlaced)
        interlaced = 1;
    else
        interlaced = 0;
    image->interlaced = interlaced;
    if (b & kGifHasMap)
        hasMap = 1;
    else
        hasMap = 0;
    image->hasMap = hasMap;
    image->transparent = transparent;
    image->hasTransparent = hasTransparent;
    if (image->hasMap != 0) {
        e = DT_GifReadBytes(stream, image->map, image->mapSize * 3);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _ReadNextCode (the next LZW code, codeSize bits taken low bit first across the image's data
   sub-blocks). STATE: MATCH 100%, exe 88 / obj 88, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005141b8
long _ReadNextCode(struct ITStream *stream, GIFCodeReader *r, short *code)
{
    long err;
    long e;
    ULong bits;

    err = 0;
    if (r->bitsLeft == 0) {
        if (r->blockLeft == 0) {
            r->next = r->block;
            e = DT_GifReadByte(stream, &r->blockLeft);
            if (e != 0)
                goto fail;
            e = DT_GifReadBytes(stream, r->block, r->blockLeft);
            if (e != 0)
                goto fail;
        }
        r->current = *r->next++;
        r->bitsLeft = 8;
        r->blockLeft--;
    }
    bits = r->current >> (8 - r->bitsLeft);
    while (r->codeSize > r->bitsLeft) {
        if (r->blockLeft == 0) {
            r->next = r->block;
            e = DT_GifReadByte(stream, &r->blockLeft);
            if (e != 0)
                goto fail;
            e = DT_GifReadBytes(stream, r->block, r->blockLeft);
            if (e != 0) {
fail:
                err = e;
                goto done;
            }
        }
        r->current = *r->next++;
        bits |= (ULong)r->current << r->bitsLeft;
        r->bitsLeft += 8;
        r->blockLeft--;
    }
    r->bitsLeft -= r->codeSize;
    bits &= gDT_GifCodeMask[r->codeSize];
    *code = bits;
done:
    return err;
}

/* _WriteImageLine (store one decoded row, stepping through the four interlace passes; count is
   passed and never read). STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005142be
long _WriteImageLine(GIFRows *rows, ITImage *image, const unsigned char *pixels, unsigned short count)
{
    long err;

    err = 0;
    if (rows->rowsDone >= rows->height) {
        err = kStreamOverrun;
    } else if (!rows->interlaced) {
        DT_ImageSetRawPixels(image, NULL, rows->row, pixels);
        rows->row++;
        rows->rowsDone++;
    } else {
        if (rows->rowsDone++ != 0)
            rows->row += gDT_GifRowStep[rows->pass];
        if (rows->row >= rows->height) {
            rows->pass++;
            if (rows->pass < kGifPasses)
                rows->row = gDT_GifRowStart[rows->pass];
        }
        DT_ImageSetRawPixels(image, NULL, rows->row, pixels);
    }
    return err;
}
