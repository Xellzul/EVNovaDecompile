// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixierle.c -- BlitPixieRLE (win 0x00471e10). Notes: Research/notes/blitpixierle.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRLE (mac _BlitPixieRLE). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471e10
// MAC: _BlitPixieRLE i386 0x000bddd0 ppc 0x000c9820
void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel,
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
            BlitPixieMemCopy(dstPixel, rleTokens, count);
            rleTokens += count;
#if defined(MACBUILD) && V111
            (int32_t)rleTokens += (-(int32_t)rleTokens) & 3;
#else
            rleTokens += (-(int32_t)rleTokens) & 3;
#endif
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

#ifdef MACBUILD
/* BlitPixieRLESwap (mac _BlitPixieRLESwap, mac-only; win has FixupRLEPixelData). STATE: see the mac gate. */
// MAC: _BlitPixieRLESwap i386 0x000be3a8 ppc 0x000c9934
void BlitPixieRLESwap(unsigned char *tokens, RLEPixelData *header)
{
#if TARGET_RT_LITTLE_ENDIAN
    uint32_t token, opcode, count;
    unsigned short depth;
    uint32_t frames;

    depth = header->depth;
    frames = header->frames;

    if (frames == 0)
        return;

    do {
        token = Endian32_Swap(*(uint32_t *)tokens);
        *(uint32_t *)tokens = token;
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
                    *(uint32_t *)tokens = Endian32_Swap(*(uint32_t *)tokens);
                    tokens += 4;
                } while (--count);
                break;
            }
            tokens += (-(int32_t)tokens) & 3;
            break;
        case 4:
            *(uint32_t *)tokens = Endian32_Swap(*(uint32_t *)tokens);
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
