/* blitpixieblitclear.c -- BlitPixieBlitClear, the mac 1.1.1 BlitPixieBlitClear.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieBlitClear (mac _BlitPixieBlitClear, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieBlitClear i386 0x000c026c ppc 0x000cbe9c
void BlitPixieBlitClear(
	unsigned char *src,
	unsigned char *dst,
	unsigned long color,
	unsigned long srcRowBytes,
	unsigned long dstRowBytes,
	unsigned short bytes,
	unsigned short rows)
{
	BLITPIXIE_ASSERT(rows > 0 );
	BLITPIXIE_ASSERT(bytes > 0 );

	while (rows--)
	{
		BlitPixieMemCopy( dst, src, bytes );
		BlitPixieMemSet( src, color, bytes );
		src += srcRowBytes;
		dst += dstRowBytes;
	}
}
