// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixieaddovercopy.c -- BlitPixieAddOverCopy (win 0x00473b60), inside the un-anchored BlitPixie block. Notes: Research/notes/blitpixieaddovercopy.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieAddOverCopy (mac _BlitPixieAddOverCopy). STATE: MATCH 100.0%, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473b60
// MAC: _BlitPixieAddOverCopy i386 0x000c177d ppc 0x000cd7d4
#ifdef MACBUILD
#ifdef MACBUILD
void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                  int32_t byteWidth)
{
    int32_t i;
    uint32_t *sp;
    uint32_t *dp;

    sp = (uint32_t *)srcPixels;
    dp = (uint32_t *)dstPixels;
    for (i = 0; i < (byteWidth >> 2); i++) {
        *dp |= *sp++;
        dp++;
    }

    srcPixels = (unsigned char *)sp;
    dstPixels = (unsigned char *)dp;

    if (byteWidth & 2)
        *(unsigned short *)dstPixels |= *(unsigned short *)srcPixels;

    if (byteWidth & 1)
        *dstPixels |= *srcPixels;
}
#else
void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                  int32_t byteWidth)
{
    int32_t i;

    for (i = 0; i < (byteWidth >> 2); i++) {
        *dstPixels |= *srcPixels;
        srcPixels += 4;
        dstPixels += 4;
    }

    if (byteWidth & 2)
        *dstPixels |= *srcPixels;

    if (byteWidth & 1)
        *dstPixels |= *srcPixels;
}
#endif
#else
void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                  int32_t byteWidth)
{
    int32_t i;
    unsigned char *s;
    unsigned char *d;

    /* s/d copies: decl (s,d) picks the load regs (src=esi dst=ebx), d-first
       statements pick dst-first load order. Increments stay src-first to
       match the exe's add order. */
    d = dstPixels;
    s = srcPixels;
    for (i = 0; i < (byteWidth >> 2); i++) {
        *(uint32_t *)d |= *(uint32_t *)s;
        s += 4;
        d += 4;
    }

    if (byteWidth & 2) {
        unsigned short *wordDst = (unsigned short *)d;
        *wordDst = *wordDst | *(unsigned short *)s;
    }

    /* BUG: the tails never advance s and d, so a width of 4n+3 re-ORs the short's low byte */
    if (byteWidth & 1) {
        *d = *d | *s;
    }
}
#endif
