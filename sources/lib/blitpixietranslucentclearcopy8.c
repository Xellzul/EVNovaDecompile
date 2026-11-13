// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* blitpixietranslucentclearcopy8.c -- BlitPixieTranslucentClearCopy8 (win 0x00473dc0). Notes: Research/notes/blitpixietranslucentclearcopy8.md */
#include "macport.h"
#include "shipobj.h"

EXTERN_C_BEGIN

/* BlitPixieTranslucentClearCopy8 (mac _BlitPixieTranslucentClearCopy8). STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473dc0
// MAC: _BlitPixieTranslucentClearCopy8 i386 0x000c1bca ppc 0x000cdd30
void BlitPixieTranslucentClearCopy8(unsigned char *dstPixels,
        unsigned char *srcPixels, int32_t numPixels, unsigned char clearValue)
{
    int32_t i;
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

EXTERN_C_END
