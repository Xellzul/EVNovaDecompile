/* blitpixierledither.c -- BlitPixieRLEDither.o: the dithered RLE blitter and its random table (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

#define kTokenShift 24
#define kCountMask 0x00ffffff
#define kEndShapeToken 0
#define kLineStartToken 1
#define kDrawPixelsToken 2
#define kSkipPixelsToken 3
#define kSingleColorToken 4
typedef uint32_t TokenDataType;
#define ALIGN_PTR(p) (-((int32_t)p) & 3)

#define kRandomTableSize 1024

uint32_t randomTable[kRandomTableSize];
short randomIndex;

extern short GetRandom(short min, short max);
extern void Randomize(void);

/* BlitPixieDitherCopy (mac _BlitPixieDitherCopy, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDitherCopy i386 0x000c06e0 ppc 0x000cc430
void BlitPixieDitherCopy(unsigned char *dst, unsigned char *src, int32_t bytes, uint32_t level)
{
    int32_t i;

    {
        uint32_t *srcTemp = (uint32_t *)src,
                      *dstTemp = (uint32_t *)dst;

        for (i = 0; i < (bytes >> 2); i++) {
            if (level <= randomTable[randomIndex])
                *dstTemp = *srcTemp;
            dstTemp++;
            srcTemp++;
            if (++randomIndex >= kRandomTableSize)
                randomIndex = 0;
        }

        src = (unsigned char *)srcTemp;
        dst = (unsigned char *)dstTemp;
    }
    if (bytes & 2) {
        unsigned short *srcTemp = (unsigned short *)src,
                       *dstTemp = (unsigned short *)dst;

        if (level <= randomTable[randomIndex])
            *dstTemp = *srcTemp;
        dstTemp++;
        srcTemp++;
        if (++randomIndex >= kRandomTableSize)
            randomIndex = 0;

        src = (unsigned char *)srcTemp;
        dst = (unsigned char *)dstTemp;
    }
    if (bytes & 1) {
        unsigned char *srcTemp = src,
                      *dstTemp = dst;

        if (level <= randomTable[randomIndex])
            *dstTemp = *srcTemp;
        dstTemp++;
        srcTemp++;
        if (++randomIndex >= kRandomTableSize)
            randomIndex = 0;

        src = srcTemp;
        dst = dstTemp;
    }
}

/* BlitPixieRLEDither (mac _BlitPixieRLEDither, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRLEDither i386 0x000c080d ppc 0x000cc55c
void BlitPixieRLEDither(unsigned char *rleTokens, unsigned char *dstPixel, int32_t dstRowBytes)
{
    unsigned char *rowStart = dstPixel;
    uint32_t token, opcode, count;
    int32_t level;

    level = gSWCurrentSpriteBeingDrawn->translucency;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieDitherCopy(dstPixel, rleTokens, count, level);
            rleTokens += count;
            PtrAddBytes(rleTokens, (-(int32_t)rleTokens) & 3);
            dstPixel += count;
            break;

        case 3:
            dstPixel += count;
            break;

        case 4:
            BlitPixieMemSet(dstPixel, *(uint32_t *)rleTokens, count);
            rleTokens += 4;
            dstPixel += count;
            break;

        case 1:
            dstPixel = rowStart;
            rowStart += dstRowBytes;
            break;

        case 0:
            break;

        default:
            return;
        }
    } while (opcode != 0);
}

/* BlitPixieRLEDitherClipped (mac _BlitPixieRLEDitherClipped, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRLEDitherClipped i386 0x000c08ec ppc 0x000cc684
void BlitPixieRLEDitherClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    uint32_t token,op;
    uint32_t count;
    unsigned char *rowStart;
    int32_t extra,spanend;
    int32_t level;
    short x,y;

    level = gSWCurrentSpriteBeingDrawn->translucency;

    y = 0;
    rowStart = dstPixel;

    do {
        token = *((TokenDataType *) rleTokens);
        rleTokens += sizeof(TokenDataType);

        op = token >> kTokenShift;
        count = token & kCountMask;

        switch (op) {
            case kDrawPixelsToken:

                spanend = x + count - 1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                        rleTokens += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieDitherCopy(dstPixel, rleTokens, count, level);

                    x += extra;
                    dstPixel += extra;
                    rleTokens += extra;
                }

                x += count;
                dstPixel += count;
                rleTokens += count;
                PtrAddBytes(rleTokens, ALIGN_PTR(rleTokens));

                break;

            case kSkipPixelsToken:
                x += count;
                dstPixel += count;
                break;

            case kSingleColorToken:

                spanend = x + count -1;

                if (x < srcBlitRect->right && spanend > srcBlitRect->left) {
                    if (x < srcBlitRect->left) {
                        extra = srcBlitRect->left - x;

                        count -= extra;

                        x += extra;
                        dstPixel += extra;
                    }

                    extra = 0;
                    if (spanend > srcBlitRect->right) {
                        extra = spanend - srcBlitRect->right;

                        count -= extra;
                    }

                    BlitPixieMemSet(dstPixel, *((uint32_t *) rleTokens), count);

                    x += extra;
                    dstPixel += extra;
                }

                x += count;
                dstPixel += count;

                rleTokens += sizeof(int32_t);
                break;
            case kLineStartToken:
                if (y < srcBlitRect->top) {
                    rleTokens += count;
                } else if (y >= srcBlitRect->bottom) {
                    op = kEndShapeToken;
                    break;
                }

                x = 0;
                y++;
                dstPixel = rowStart;
                rowStart += dstRowBytes;
                break;
            case kEndShapeToken:
                break;
            default:
                DebugStr("\punknown token");
                return;
        }
    } while (op != kEndShapeToken);
}

/* BlitPixieDitherInit (mac _BlitPixieDitherInit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDitherInit i386 0x000c0ad1 ppc 0x000cc8c0
void BlitPixieDitherInit(void)
{
    uint32_t *p;
#ifdef MAC1010 /* 1.0.10 spelling */
    short i;

    Randomize();
    i = 0;
    for (p = randomTable; i < kRandomTableSize; i++) {
        *p = GetRandom(0, 31);
        p++;
    }
#else

    Randomize();
    p = randomTable;
    do {
        *p = GetRandom(0, 31);
        p++;
    } while (p != randomTable + kRandomTableSize);
#endif
    randomIndex = GetRandom(0, kRandomTableSize - 1);
}
