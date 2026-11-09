/* blitpixiedoublerects.c -- BlitPixieDoubleRects, the mac 1.1.1 BlitPixieDoubleRects.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieDoubleRects (mac _BlitPixieDoubleRects, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDoubleRects i386 0x000bf1e9 ppc 0x000cad9c
void BlitPixieDoubleRects(
	unsigned char *src,
	unsigned char *dst,
	unsigned short bytesA,
	unsigned short bytesB,
	unsigned short rows,
	BlitPixieOffsetInfoPtr info)
{
	BLITPIXIE_ASSERT( rows > 0 );
	BLITPIXIE_ASSERT( bytesA > 0 );
	BLITPIXIE_ASSERT( bytesB > 0 );

	while ( rows-- )
	{
		BlitPixieMemCopy( dst, src, bytesA );
		src += bytesA;
		dst += bytesA;

		src += info->srcOffsetAtoB;
		dst += info->dstOffsetAtoB;

		BlitPixieMemCopy( dst, src, bytesB );
		src += bytesB;
		dst += bytesB;

		src += info->srcOffsetBtoA;
		dst += info->dstOffsetBtoA;
	}
}
