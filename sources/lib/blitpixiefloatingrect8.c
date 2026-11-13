// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixiefloatingrect8.c -- BlitPixieFloatingRect8 (win 0x00471dc0). Notes: Research/notes/blitpixiefloatingrect8.md */
/* The four bytes of frame are real and unused: the exe opens `sub esp, 4' and nothing
   references [esp+0..3]. A local that is declared and never read is the only thing that
   produces it. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieFloatingRect8 (mac _BlitPixieFloatingRect8). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471dc0
// MAC: _BlitPixieFloatingRect8 i386 0x000be5a8 ppc 0x000c9f6c
void BlitPixieFloatingRect8(unsigned char *srcPixels, unsigned char *dstPixels,
#ifdef MACBUILD
        uint32_t srcRowBytes, uint32_t dstRowBytes, unsigned short byteWidth,
#else
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
#endif
        unsigned short numRows, unsigned short backgroundValue)
{
    int32_t unused;

    while (numRows--) {
        BlitPixieTranslucentClearCopy8(dstPixels, srcPixels, byteWidth,
                backgroundValue);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
