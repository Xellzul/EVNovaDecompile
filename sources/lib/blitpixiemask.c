// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixiemask.c -- BlitPixieMask (win 0x00471a70), the per-row masked blit BlitPixieMaskDrawProc calls. Notes: Research/notes/blitpixiemask.md */
/* The row strides are pre-adjusted once at the top, on the PARAMETER slots, so the
   per-row advance adds only the leftover stride. The source pixels are pre-masked by
   the caller, which is why an OR suffices; both counters are unsigned. */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieMask (mac _BlitPixieMask). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471a70
// MAC: _BlitPixieMask i386 0x000c0f75 ppc 0x000cce04
void BlitPixieMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, unsigned long srcRowBytes,
        unsigned long dstRowBytes, unsigned short byteWidth,
        unsigned short numRows)
{
    unsigned long i;

    srcRowBytes -= byteWidth;
    dstRowBytes -= byteWidth;

    while (numRows--) {
#ifdef MACBUILD /* mac spelling */
        for (i = 0; i < byteWidth; dstPixels++, i++, srcPixels++, maskPixels++) {
            *dstPixels = (*dstPixels & *maskPixels) | *srcPixels;
        }
#else
        for (i = 0; i < byteWidth; i++) {
            *dstPixels = (*dstPixels & *maskPixels) | *srcPixels;

            maskPixels++;
            srcPixels++;
            dstPixels++;
        }
#endif

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
        maskPixels += srcRowBytes;
    }
}
