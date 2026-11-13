/* blitpixiebliterase.c -- BlitPixieBlitErase, the mac 1.1.1 BlitPixieBlitErase.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieBlitErase (mac _BlitPixieBlitErase, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieBlitErase i386 0x000c2805 ppc 0x000cebc0
void BlitPixieBlitErase(
	unsigned char *src,
	unsigned char *dst,
	unsigned char *bkg,
	uint32_t srcRowBytes,
	uint32_t dstRowBytes,
	unsigned short bytes,
	unsigned short rows)
{
	BLITPIXIE_ASSERT(rows > 0 );
	BLITPIXIE_ASSERT(bytes > 0 );

	while (rows--)
	{
		BlitPixieMemCopy( dst, src, bytes );
		BlitPixieMemCopy( src, bkg, bytes );
		src += srcRowBytes;
		dst += dstRowBytes;
		bkg += srcRowBytes;
	}
}
