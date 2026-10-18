// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixietranslucentclearcopy8.c -- BlitPixieTranslucentClearCopy8 (win 0x00473dc0). Notes: Research/notes/blitpixietranslucentclearcopy8.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieTranslucentClearCopy8 (mac _BlitPixieTranslucentClearCopy8). STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473dc0
void BlitPixieTranslucentClearCopy8(unsigned char *dstPixels,
        unsigned char *srcPixels, long numPixels, unsigned char clearValue)
{
    long i;
    unsigned char *s;
    unsigned char *d;

    /* s/d copies in exe load order (src first); increments stay dst-first
       to match the exe's add order -- same family lever as the translucent
       copy siblings. */
    s = srcPixels;
    d = dstPixels;
    for (i = 0; i < numPixels; i++) {
        if (*s != clearValue)
            *d = *s;

        d++;
        s++;
    }
}
