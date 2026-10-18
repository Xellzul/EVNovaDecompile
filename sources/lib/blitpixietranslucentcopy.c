// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixietranslucentcopy.c -- BlitPixieTranslucentCopy (win 0x004736c0). Notes: Research/notes/blitpixietranslucentcopy.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#define SW_RED(c, k)    ((((c) & 0x7c007c00) >> 5) * (k) & 0x7c007c00)
#define SW_GREEN(c, k)  (((c) & 0x3e003e0) * (k) >> 5 & 0x3e003e0)
#define SW_BLUE(c, k)   (((c) & 0x1f001f) * (k) >> 5 & 0x1f001f)

/* BlitPixieTranslucentCopy (mac _BlitPixieTranslucentCopy). STATE: MATCH, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x004736c0
void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
        long byteWidth)
{
    long i;
    long numLongs;
    unsigned long pixel;
    unsigned long src;
    unsigned long dst;
    unsigned char *s;
    unsigned char *d;

    d = dstPixels;
    s = srcPixels;

    for (i = 0, numLongs = byteWidth >> 2; i < numLongs; i++) {
        if (overBias < 32) {
            src = SW_RED(*(unsigned long *)s, overBias);
            src |= SW_GREEN(*(unsigned long *)s, overBias);
            src |= SW_BLUE(*(unsigned long *)s, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *(unsigned long *)s;

        dst = *(unsigned long *)d;
        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(dst, alpha);
        pixel |= SW_GREEN(dst, alpha);
        pixel |= SW_BLUE(dst, alpha);
        *(unsigned long *)d = pixel;

        s += 4;
        d += 4;
    }

    if (byteWidth & 2) {
        if (overBias < 32) {
            src = SW_RED(*(unsigned short *)s, overBias);
            src |= SW_GREEN(*(unsigned short *)s, overBias);
            src |= SW_BLUE(*(unsigned short *)s, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *(unsigned short *)s;

        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(*(unsigned short *)d, alpha);
        pixel |= SW_GREEN(*(unsigned short *)d, alpha);
        pixel |= SW_BLUE(*(unsigned short *)d, alpha);
        *(unsigned short *)d = pixel;

        s += 2;
        d += 2;
    }

    /* BUG: blends a single byte with the 16-bit channel masks, so the result is meaningless */
    if (byteWidth & 1) {
        if (overBias < 32) {
            src = SW_RED(*s, overBias);
            src |= SW_GREEN(*s, overBias);
            src |= SW_BLUE(*s, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *s;

        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(*d, alpha);
        pixel |= SW_GREEN(*d, alpha);
        pixel |= SW_BLUE(*d, alpha);
        *d = pixel;
    }
}
