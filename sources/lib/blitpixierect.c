// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierect.c -- BlitPixieRect (win 0x00471cf0). Notes: Research/notes/blitpixierect.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRect (mac _BlitPixieRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471cf0
void BlitPixieRect(unsigned char *srcPixels, unsigned char *dstPixels,
        long srcRowBytes, long dstRowBytes, unsigned short byteWidth,
        unsigned short numRows)
{
    while (numRows--) {
        BlitPixieMemCopy(dstPixels, srcPixels, byteWidth);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
