/* blitpixiemaskcolor.c -- BlitPixieMaskColor, the mac 1.1.1 BlitPixieMask.o's last function (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieMaskColor (mac _BlitPixieMaskColor, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskColor i386 0x000c105e ppc 0x000ccf1c
void BlitPixieMaskColor(
	uint32_t srcColor,
	unsigned char *dstPixelP,
	unsigned char *maskPixelP,
	uint32_t srcOffset,
	uint32_t dstOffset,
	unsigned short numBytesPerRow,
	unsigned short rowsToCopy)
{
	uint32_t index;
	uint32_t mask;

	srcOffset -= numBytesPerRow;
	dstOffset -= numBytesPerRow;

	while (rowsToCopy--)
	{
		// note: must do this with longs and shorts first, since depth can be both 32, 16 or 8 bits

		for ( index=0; index < (numBytesPerRow >> 2); index++)
		{
			mask = *((uint32_t *) maskPixelP);
			maskPixelP += sizeof(uint32_t);
			*((uint32_t *) dstPixelP) = (*((uint32_t *) dstPixelP) & mask) | (srcColor &~ mask);
			dstPixelP += sizeof(uint32_t);
		}

		if ( numBytesPerRow & 2 )
		{
			mask = *((unsigned short *) maskPixelP);
			maskPixelP += sizeof(unsigned short);
			*((unsigned short *) dstPixelP) = (*((unsigned short *) dstPixelP) & mask) | (srcColor &~ mask);
			dstPixelP += sizeof(unsigned short);
		}

		if ( numBytesPerRow & 1 )
		{
			mask = *maskPixelP++;
			*dstPixelP++ = (*dstPixelP & mask) | (srcColor &~ mask);
		}

			// bump to next row
		dstPixelP += dstOffset;
		maskPixelP += srcOffset;
	}
}
