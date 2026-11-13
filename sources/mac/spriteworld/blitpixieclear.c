/* blitpixieclear.c -- BlitPixieClear, the mac 1.1.1 BlitPixieClear.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieClear (mac _BlitPixieClear, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieClear i386 0x000bfc85 ppc 0x000cb8e4
void BlitPixieClear(
	unsigned char *dst,
	uint32_t dstRowBytes,
	unsigned short bytes,
	unsigned short rows,
	uint32_t clear )
{
	BLITPIXIE_ASSERT(rows > 0 );
	BLITPIXIE_ASSERT(bytes > 0 );

	while (rows--)
	{
		BlitPixieMemSet( dst, clear, bytes );
		dst += dstRowBytes;
	}
}
