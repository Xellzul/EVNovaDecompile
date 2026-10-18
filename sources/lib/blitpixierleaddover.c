// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierleaddover.c -- BlitPixieRLEAddOver (win 0x00472b30), the direct-colour additive RLE walker. Notes: Research/notes/blitpixierleaddover.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRLEAddOver (mac _BlitPixieRLEAddOver). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472b30
void BlitPixieRLEAddOver(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes)
{
    unsigned char *rowStart = dstPixel;
    unsigned long token, opcode, count;

    do {
        token = *(unsigned long *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieAddOverCopy(dstPixel, rleTokens, count);
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
