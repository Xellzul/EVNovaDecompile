// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixiememset.c -- BlitPixieMemSet (win 0x00474410). Notes: Research/notes/blitpixiememset.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieMemSet (mac _BlitPixieMemSet). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474410
void BlitPixieMemSet(unsigned char *dstPixels, unsigned long value,
        long byteWidth)
{
    long i;

    for (i = 0; i < (byteWidth >> 2); i++) {
        *(unsigned long *)dstPixels = value;
        dstPixels += 4;
    }

    if (byteWidth & 2) {
        *(unsigned short *)dstPixels = value;
        dstPixels += 2;
    }

    if (byteWidth & 1) {
        *dstPixels = value;
    }
}
