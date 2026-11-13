// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* blitpixierect.c -- BlitPixieRect (win 0x00471cf0). Notes: Research/notes/blitpixierect.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRect (mac _BlitPixieRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471cf0
// MAC: _BlitPixieRect i386 0x000be4a0 ppc 0x000c9df0
void BlitPixieRect(unsigned char *srcPixels, unsigned char *dstPixels,
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows)
{
    while (numRows--) {
        BlitPixieMemCopy(dstPixels, srcPixels, byteWidth);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
