// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierle.c -- BlitPixieRLE (win 0x00471e10). Notes: Research/notes/blitpixierle.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRLE (mac _BlitPixieRLE). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471e10
// MAC: _BlitPixieRLE i386 0x000bddd0 ppc 0x000c9820
void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel,
        long dstRowBytes)
{
    unsigned long token, opcode, count;
    unsigned char *rowStart = dstPixel;

    do {
        token = *(unsigned long *)rleTokens;
        opcode = token >> 24;
        rleTokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            BlitPixieMemCopy(dstPixel, rleTokens, count);
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

#ifdef MACBUILD
/* BlitPixieRLESwap (mac _BlitPixieRLESwap, mac-only; win has FixupRLEPixelData). STATE: see the mac gate. */
// MAC: _BlitPixieRLESwap i386 0x000be3a8 ppc 0x000c9934
void BlitPixieRLESwap(unsigned char *tokens, RLEPixelData *header)
{
#if TARGET_RT_LITTLE_ENDIAN
    unsigned long token, opcode, count;
    unsigned short depth;
    unsigned long frames;

    depth = header->depth;
    frames = header->frames;

    if (frames == 0)
        return;

    do {
        token = Endian32_Swap(*(unsigned long *)tokens);
        *(unsigned long *)tokens = token;
        opcode = token >> 24;
        tokens += 4;
        count = token & 0xffffff;

        switch (opcode) {
        case 2:
            switch (depth) {
            case 8:
                tokens += count;
                break;
            case 16:
                count >>= 1;
                do {
                    *(unsigned short *)tokens = Endian16_Swap(*(unsigned short *)tokens);
                    tokens += 2;
                } while (--count);
                break;
            case 32:
                count >>= 2;
                do {
                    *(unsigned long *)tokens = Endian32_Swap(*(unsigned long *)tokens);
                    tokens += 4;
                } while (--count);
                break;
            }
            tokens += (-(long)tokens) & 3;
            break;
        case 4:
            *(unsigned long *)tokens = Endian32_Swap(*(unsigned long *)tokens);
            tokens += 4;
            break;
        case 0:
        case 1:
        case 3:
            break;
        default:
            return;
        }
    } while (opcode != 0 || --frames != 0);
#endif
}
#endif
