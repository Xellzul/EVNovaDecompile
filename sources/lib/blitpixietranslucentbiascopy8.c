// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixietranslucentbiascopy8.c -- BlitPixieTranslucentBiasCopy8 (win 0x00473e00). Notes: Research/notes/blitpixietranslucentbiascopy8.md */
/* `*dst = alphaTable[level2][bias][*src]': the row is selected by a FIXED colour held
   in a global and the columns by the source, so the destination is never read. level2 is
   index 7 of the parameter block, where the plain copy reads index 0. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* BlitPixieTranslucentBiasCopy8 (mac _BlitPixieTranslucentBiasCopy8). STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473e00
void BlitPixieTranslucentBiasCopy8(unsigned char *dstPixels,
                                   unsigned char *srcPixels, long count)
{
    long i;
    unsigned char *s;
    unsigned char *d;

    /* s before d: the copy locals' declaration order picks the load order
       (src first into edx), where the parameters' ABI order loads dst first.
       Increments are likewise source-first. */
    s = srcPixels;
    d = dstPixels;
    for (i = 0; i < count; i++) {
        *d = alphaTable[overBias][biasColor][*s];
        s++;
        d++;
    }
}
