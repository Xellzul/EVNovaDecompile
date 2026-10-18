// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierletranslucent8.c -- BlitPixieRLETranslucent8 (win 0x00472bb0), the 8-bit partial-translucency RLE walker. Notes: Research/notes/blitpixierletranslucent8.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* +0xac is a long copied straight into the parameter block and is left with a
   placeholder name rather than a guessed one. */


/* BlitPixieRLETranslucent8 (mac _BlitPixieRLETranslucent8). STATE: MATCH 100.0%, exe 96 /
   obj 96 instrs, byte-identical yes (314 bytes). OPEN: nothing. see notes */
// FUNCTION: EVNOVA 0x00472bb0
void BlitPixieRLETranslucent8(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes)
{
    unsigned char *rowStart;
    unsigned long token, opcode, count;
    char useBias;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    alpha = ClampAlphaLevel(alpha);
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
        token = *(unsigned long *)rleTokens;
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
            rleTokens += (-(long)rleTokens) & 3;
            dstPixel += count;
            break;

        case 3:
            dstPixel += count;
            break;

        case 4:
            BlitPixieMemSet(dstPixel, *(unsigned long *)rleTokens, count);
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
