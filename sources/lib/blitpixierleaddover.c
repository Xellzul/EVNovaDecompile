// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* blitpixierleaddover.c -- BlitPixieRLEAddOver (win 0x00472b30), the direct-colour additive RLE walker. Notes: Research/notes/blitpixierleaddover.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRLEAddOver (mac _BlitPixieRLEAddOver). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00472b30
// MAC: _BlitPixieRLEAddOver i386 0x000c24bf ppc 0x000ce7f0
void BlitPixieRLEAddOver(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes)
{
    uint32_t token, opcode, count;
    unsigned char *rowStart = dstPixel;

    do {
        token = *(uint32_t *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieAddOverCopy(dstPixel, rleTokens, count);
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
