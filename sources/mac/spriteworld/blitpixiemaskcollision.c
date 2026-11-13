/* blitpixiemaskcollision.c -- BlitPixieMaskCollision, the mac 1.1.1 BlitPixieMaskCollision.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieMaskCollision (mac _BlitPixieMaskCollision, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskCollision i386 0x000c066e ppc 0x000cc3b0
Boolean BlitPixieMaskCollision(
	unsigned char *maskPtr1,
	unsigned char *maskPtr2,
	uint32_t rowBytes1,
	uint32_t rowBytes2,
	unsigned short bytes,
	unsigned short rows)
{
	uint32_t	x;

	rowBytes1 -= bytes;
	rowBytes2 -= bytes;

	while ( rows-- )
	{
		for ( x = 0; x < bytes; x++)
		{
			if ( (*maskPtr1++ | *maskPtr2++) == 0 )
				return true;
		}

		maskPtr1 += rowBytes1;
		maskPtr2 += rowBytes2;
	}

	return false;
}
