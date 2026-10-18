// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixiefloatingrect8.c -- BlitPixieFloatingRect8 (win 0x00471dc0). Notes: Research/notes/blitpixiefloatingrect8.md */
/* The four bytes of frame are real and unused: the exe opens `sub esp, 4' and nothing
   references [esp+0..3]. A local that is declared and never read is the only thing that
   produces it. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieFloatingRect8 (mac _BlitPixieFloatingRect8). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471dc0
void BlitPixieFloatingRect8(unsigned char *srcPixels, unsigned char *dstPixels,
        long srcRowBytes, long dstRowBytes, unsigned short byteWidth,
        unsigned short numRows, unsigned short backgroundValue)
{
    long unused;

    while (numRows--) {
        BlitPixieTranslucentClearCopy8(dstPixels, srcPixels, byteWidth,
                backgroundValue);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
