// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* blitpixiememset.c -- BlitPixieMemSet (win 0x00474410). Notes: Research/notes/blitpixiememset.md */
#include "macport.h"
#include "shipobj.h"

EXTERN_C_BEGIN

/* BlitPixieMemSet (mac _BlitPixieMemSet). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474410
// MAC: _BlitPixieMemSet i386 0x000c0452 ppc 0x000cc158
#ifdef MACBUILD
void BlitPixieMemSet( unsigned char *dst, uint32_t value, int32_t bytes )
{
	int32_t	i;

	for ( i = 0; i < (bytes >> 2); i++ )
		*((uint32_t *) dst)++ = value;
	if ( bytes & 2 )
		*((unsigned short *) dst)++ = value;
	if ( bytes & 1 )
		*((unsigned char *) dst)++ = value;
}
#else
void BlitPixieMemSet(unsigned char *dstPixels, uint32_t value,
        int32_t byteWidth)
{
    int32_t i;

    for (i = 0; i < (byteWidth >> 2); i++) {
        *(uint32_t *)dstPixels = value;
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
#endif

EXTERN_C_END
