// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* blitpixierletranslucent8.c -- BlitPixieRLETranslucent8 (win 0x00472bb0), the 8-bit partial-translucency RLE walker. Notes: Research/notes/blitpixierletranslucent8.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* +0xac is a long copied straight into the parameter block and is left with a
   placeholder name rather than a guessed one. */


/* BlitPixieRLETranslucent8 (mac _BlitPixieRLETranslucent8). STATE: MATCH 100.0%, exe 96 /
   obj 96 instrs, byte-identical yes (314 bytes). OPEN: nothing. see notes */
// FUNCTION: EVNOVA 0x00472bb0
// MAC: _BlitPixieRLETranslucent8 i386 0x000c1cff ppc 0x000cde9c
#ifdef MACBUILD
void BlitPixieRLETranslucent8(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes)
{
    uint32_t token, opcode, count;
    unsigned char *rowStart;
    int32_t level;
    Boolean useTint;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = fix_power_of_two(alpha);
    alpha >>= 3;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;

    if (gSWCurrentSpriteBeingDrawn->tintMode >= 3) {
        useTint = 1;

        level = 32 - gSWCurrentSpriteBeingDrawn->tintMode;
        level = (level < 31) ? level : 31;

        overBias = level >> 3;
        overBias = (overBias < 4) ? overBias : 4;
    } else {
        useTint = 0;
    }

    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            if (useTint)
                BlitPixieTranslucentBiasCopy8(dstPixel, rleTokens, count);
            else
                BlitPixieTranslucentCopy8(dstPixel, rleTokens, count);
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
#else
void BlitPixieRLETranslucent8(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes)
{
    uint32_t token, opcode, count;
    unsigned char *rowStart;
    Boolean useBias;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = fix_power_of_two(alpha);
    alpha >>= 3;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;

    if (gSWCurrentSpriteBeingDrawn->tintMode >= 3) {
        useBias = 1;
        overBias =
                SW_MIN(32 - gSWCurrentSpriteBeingDrawn->tintMode, 31) >> 3;
        overBias = SW_MIN(overBias, 4);
    } else {
        useBias = 0;
    }

    rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            if (useBias)
                BlitPixieTranslucentBiasCopy8(dstPixel, rleTokens, count);
            else
                BlitPixieTranslucentCopy8(dstPixel, rleTokens, count);

            rleTokens += count;
            rleTokens += (-(int32_t)rleTokens) & 3;
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
#endif
