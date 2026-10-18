// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixietranslucentcopy8.c -- BlitPixieTranslucentCopy8 (win 0x00473c20). Notes: Research/notes/blitpixietranslucentcopy8.md */
/* The eight-long parameter block at 0x007ceac8: [0] translucency, [1][2][3] per-channel
   scales clamped to 32, [4][5][6] overflow addends pre-shifted into 1555 field position,
   [7] 32 - tintMode. This body uses [0] as the table row selector. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* BlitPixieTranslucentCopy8 (mac _BlitPixieTranslucentCopy8). STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473c20
void BlitPixieTranslucentCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                               long count)
{
    long i;
    unsigned char *s;
    unsigned char *d;

    /* s before d, src-first increments -- same lever as the bias sibling
       (blitpixietranslucentbiascopy8.c): copy locals pick the exe's load
       order where the ABI-ordered parameters load dst first. */
    s = srcPixels;
    d = dstPixels;
    for (i = 0; i < count; i++) {
        *d = alphaTable[alpha][*s][*d];
        s++;
        d++;
    }
}
