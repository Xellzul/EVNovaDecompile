// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
// MAC: _BlitPixieTranslucentCopy i386 0x000c1110 ppc 0x000ccfe0
#ifdef MACBUILD
void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
        int32_t byteWidth)
{
    int32_t numLongs;
    int32_t i;
    uint32_t pixel;
    uint32_t src;
    uint32_t dst;
    unsigned char *s;
    uint32_t *sp;
    unsigned char *d;
    uint32_t *dp;

    d = dstPixels;
    s = srcPixels;

    sp = (uint32_t *)s;
    dp = (uint32_t *)d;
    for (i = 0, numLongs = byteWidth >> 2; i < numLongs; i++) {
        if (overBias < 32) {
            src = SW_RED(*sp, overBias);
            src |= SW_GREEN(*sp, overBias);
            src |= SW_BLUE(*sp, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *sp;

        dst = *dp;
        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(dst, alpha);
        pixel |= SW_GREEN(dst, alpha);
        pixel |= SW_BLUE(dst, alpha);
        *dp = pixel;

        dp++;
        sp++;
    }

    s = (unsigned char *)sp;
    d = (unsigned char *)dp;
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

#ifdef MAC1010 /* 1.0.10 spelling */
        d += 2;
        s += 2;
#else
        s += 2;
        d += 2;
#endif
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
#else
void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
        int32_t byteWidth)
{
    int32_t i;
    int32_t numLongs;
    uint32_t pixel;
    uint32_t src;
    uint32_t dst;
    unsigned char *s;
    unsigned char *d;

    d = dstPixels;
    s = srcPixels;

    for (i = 0, numLongs = byteWidth >> 2; i < numLongs; i++) {
        if (overBias < 32) {
            src = SW_RED(*(uint32_t *)s, overBias);
            src |= SW_GREEN(*(uint32_t *)s, overBias);
            src |= SW_BLUE(*(uint32_t *)s, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *(uint32_t *)s;

        dst = *(uint32_t *)d;
        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(dst, alpha);
        pixel |= SW_GREEN(dst, alpha);
        pixel |= SW_BLUE(dst, alpha);
        *(uint32_t *)d = pixel;

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
#endif
