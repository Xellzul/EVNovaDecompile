// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixiepartialmask.c -- BlitPixiePartialMask (win 0x00471b70), the per-row blit BlitPixiePartialMaskDrawProc calls. Notes: Research/notes/blitpixiepartialmask.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixiePartialMask (mac _BlitPixiePartialMask). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471b70
void BlitPixiePartialMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short byteWidth, unsigned short numRows)
{
    unsigned long i;
    unsigned char maskByte;

    srcRowBytes -= byteWidth;
    dstRowBytes -= byteWidth;

    while (numRows--) {
        for (i = 0; i < byteWidth; i++) {
            maskByte = *maskPixels++;
            *dstPixels++ = (*dstPixels & maskByte) | (*srcPixels++ & ~maskByte);
        }

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
        maskPixels += srcRowBytes;
    }
}
