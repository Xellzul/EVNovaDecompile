// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* ceppat.c -- EV Nova CE: the 'ppat' decoder behind the radar interference, 1, 2, 4 and
   8 bits per pixel, in place of BurgerLib's 8-bit-only ImageParsePPat (CEFIX only).
   Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <string.h>
#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "cefix.h"

#define kPixPatFullColor    1                   /* patType */

/* CE bug: the swaps shift the signed value right, so a field whose last byte is >= 0x80
   reads with 0xFF above that byte; such a plug-in ppat fails validation, gets a black
   palette (ctSize >= 128) or writes before the palette (a colour index >= 128). */
#define SWAP16(val) (val >> 8 | val << 8)
#define SWAP32(val) (val >> 24 | (val & 0xff0000) >> 8 | (val & 0xff00) << 8 | val << 24)

static short ReadShort(const void *data)
{
    return SWAP16(*(const short *)data);
}

static long ReadLong(const void *data)
{
    return SWAP32(*(const long *)data);
}

/* The PixPat header, its PixMap, the pixels expanded to one byte each and the colour
   table as RGB triplets: an IMAGE8_PAL image. Returns 0, or 1 with the image released.
   As CE's decoder has it: the offsets are taken as shorts, ctSize as the entry count, and
   each colour component's low byte. */
int CE_ImageParsePPat(Image_t *image, const void *ppat)
{
    const unsigned char *src;
    const unsigned char *map;
    const unsigned char *data;
    const unsigned char *ctab;
    unsigned char *dst;
    Word8 *palette;
    unsigned short rowBytes;
    short mapOffset;
    short dataOffset;
    short depth;
    short count;
    short index;
    long width;
    long height;
    long ctabOffset;
    int perByte;
    int mask;
    int x;
    int y;
    int i;

    memset(image, 0, sizeof(Image_t));
    src = ppat;
    mapOffset = ReadLong(src + 2);
    dataOffset = ReadLong(src + 6);
    if (ReadShort(src) == kPixPatFullColor && mapOffset > 0 && dataOffset > 0) {
        map = src + mapOffset;
        rowBytes = ReadShort(map + 4);
        height = ReadShort(map + 10) - ReadShort(map + 6);
        width = ReadShort(map + 12) - ReadShort(map + 8);
        depth = ReadShort(map + 32);
        ctabOffset = ReadLong(map + 42);
        if ((rowBytes & kPixMapFlagMask) == kPixMapFlag && width > 0 && height > 0
                && ReadShort(map + 14) == 0 && ReadShort(map + 16) == 0
                && (depth == 1 || depth == 2 || depth == 4 || depth == 8)
                && ctabOffset > 0) {
            data = src + dataOffset;
            dst = AllocAPointer(width * height);
            image->ImagePtr = dst;
            image->Width = width;
            image->Height = height;
            image->RowBytes = width;
            image->DataType = IMAGE8_PAL;

            rowBytes &= kRowBytesMask;
            perByte = 8 / depth;
            mask = (1 << depth) - 1;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++)
                    *dst++ = data[y * rowBytes + x / perByte]
                             >> (8 - depth - x % perByte * depth) & mask;
            }

            ctab = src + ctabOffset;
            palette = AllocAPointer(kPaletteBytes);
            memset(palette, 0, kPaletteBytes);
            image->PalettePtr = palette;
            count = ReadShort(ctab + 6);
            ctab += 8;
            for (i = 0; i < count; i++) {
                index = ReadShort(ctab);
                if (index < 256) {
                    palette[index * 3] = ctab[3];
                    palette[index * 3 + 1] = ctab[5];
                    palette[index * 3 + 2] = ctab[7];
                }
                ctab += 8;
            }
            return 0;
        }
    }
    ImageDestroy(image);
    return 1;
}

#endif /* CEFIX */
