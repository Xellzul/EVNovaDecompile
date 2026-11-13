// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* blitpixiefloatingrect.c -- BlitPixieFloatingRect (win 0x00471d30), the direct-colour twin of BlitPixieFloatingRect8. Notes: Research/notes/blitpixiefloatingrect.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* BlitPixieFloatingRect (mac _BlitPixieFloatingRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471d30
// MAC: _BlitPixieFloatingRect i386 0x000be4f8 ppc 0x000c9e7c
void BlitPixieFloatingRect(unsigned char *srcPixels, unsigned char *dstPixels,
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows, unsigned short level)
{
    overBias = 32;
    alpha = 32;
    alphaR = alphaG = alphaB = level;
    overR = overG = overB = 0;

    while (numRows--) {
        BlitPixieTranslucentCopy(dstPixels, srcPixels, byteWidth);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
