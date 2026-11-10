// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixierletranslucent.c -- BlitPixieRLETranslucent (win 0x00472900), the direct-colour partial-translucency RLE walker and the twin of BlitPixieRLETranslucent8. Notes: Research/notes/blitpixierletranslucent.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* This body is the only writer of the three tint shorts. */

/* BlitPixieRLETranslucent (mac _BlitPixieRLETranslucent). STATE: MATCH, exe 134 / obj 134.
   OPEN: none */
// FUNCTION: EVNOVA 0x00472900
// MAC: _BlitPixieRLETranslucent i386 0x000c1568 ppc 0x000cd4fc
void BlitPixieRLETranslucent(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes)
{
    unsigned long token, opcode, count;
    unsigned char *rowStart;
    unsigned long redScale, greenScale, blueScale;

    alpha = gSWCurrentSpriteBeingDrawn->translucency;
    scaleR = gSWCurrentSpriteBeingDrawn->tintRed;
    scaleG = gSWCurrentSpriteBeingDrawn->tintGreen;
    scaleB = gSWCurrentSpriteBeingDrawn->tintBlue;

    biasColor = gSWCurrentSpriteBeingDrawn->tintColor;
#ifdef MACBUILD
    biasColor = (biasColor & 0xffff) | (biasColor << 16);
#else
    biasColor = ((biasColor << 16) & 0xffff0000) |
            (biasColor & 0xffff);
#endif

    overBias = 32 - gSWCurrentSpriteBeingDrawn->tintMode;

    overR = overG = overB = 0;

    redScale = (scaleR + 32 - alpha)
            ? (scaleR + 32 - alpha) : 0;
    alphaR = redScale;
    greenScale = (scaleG + 32 - alpha)
            ? (scaleG + 32 - alpha) : 0;
    alphaG = greenScale;
    blueScale = (scaleB + 32 - alpha)
            ? (scaleB + 32 - alpha) : 0;
    alphaB = blueScale;

    if (redScale > 32) {
        overR = ((redScale - 32) * 0x1f001f << 5) & 0x7c007c00;
        alphaR = 32;
    }
    if (greenScale > 32) {
        overG = ((greenScale - 32) * 0x1f001f) & 0x3e003e0;
        alphaG = 32;
    }
    if (blueScale > 32) {
        overB = (((blueScale - 32) * 0x1f001f) >> 5) & 0x1f001f;
        alphaB = 32;
    }

    rowStart = dstPixel;

    do {
        token = *(unsigned long *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieTranslucentCopy(dstPixel, rleTokens, count);
            rleTokens += count;
#if defined(MACBUILD) && V111
            (long)rleTokens += (-(long)rleTokens) & 3;
#else
            rleTokens += (-(long)rleTokens) & 3;
#endif
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
