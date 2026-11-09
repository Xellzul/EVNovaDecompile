// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixiememcopy.c -- BlitPixieMemCopy (win 0x00474350). Notes: Research/notes/blitpixiememcopy.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieMemCopy (mac _BlitPixieMemCopy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474350
// MAC: _BlitPixieMemCopy i386 0x000c03d0 ppc 0x000cc0cc
#ifdef MACBUILD
void BlitPixieMemCopy( unsigned char *dst, unsigned char *src, long bytes)
{
	long	i;

	for ( i = 0; i < (bytes >> 2); i++ )
		*((unsigned long *) dst)++ = *((unsigned long *) src)++;
	if ( bytes & 2 )
		*((unsigned short *) dst)++ = *((unsigned short *) src)++;
	if ( bytes & 1 )
		*( dst)++ = *((unsigned char *) src)++;
}
#else
void BlitPixieMemCopy(unsigned char *dstPixels, unsigned char *srcPixels,
        long byteWidth)
{
    long i;

    for (i = 0; i < (byteWidth >> 2); i++) {
        *(unsigned long *)dstPixels = *(unsigned long *)srcPixels;
        srcPixels += 4;
        dstPixels += 4;
    }

    if (byteWidth & 2) {
        *(unsigned short *)dstPixels = *(unsigned short *)srcPixels;
        srcPixels += 2;
        dstPixels += 2;
    }

    if (byteWidth & 1) {
        *dstPixels = *srcPixels;
    }
}
#endif
