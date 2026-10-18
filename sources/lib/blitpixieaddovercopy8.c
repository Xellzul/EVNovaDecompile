// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixieaddovercopy8.c -- BlitPixieAddOverCopy8 (win 0x00473fa0), inside the un-anchored BlitPixie block. Notes: Research/notes/blitpixieaddovercopy8.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieAddOverCopy8 (mac _BlitPixieAddOverCopy8). STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473fa0
void BlitPixieAddOverCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                  long count)
{
    long i;
    unsigned char *s;
    unsigned char *d;

    /* s/d copies in exe load order; src-first increments -- same family
       lever as the translucent-copy siblings. */
    s = srcPixels;
    d = dstPixels;
    for (i = 0; i < count; i++) {
        *d = alphaTable[4][*d][*s];
        s++;
        d++;
    }
}
