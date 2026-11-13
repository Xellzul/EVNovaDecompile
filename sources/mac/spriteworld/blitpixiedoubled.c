/* blitpixiedoubled.c -- BlitPixieDoubled8/16/32Bit, the mac 1.1.1 BlitPixieDoubled.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieDoubled8Bit (mac _BlitPixieDoubled8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDoubled8Bit i386 0x000bfcc9 ppc 0x000cb94c
void BlitPixieDoubled8Bit(
	unsigned char *source,			unsigned char *destination,
	uint32_t srcRowBytes,		uint32_t dstRowBytes,
	unsigned short width,			unsigned short height)
{
	int			x,y;
	int			x2,y2;
	unsigned char		c;

	for ( y = 0, y2 = 0; y < height; y++, y2 += 2 )
	{
		for ( x = 0, x2 = 0; x < width; x++, x2 += 2 )
		{
			c = source[ y * srcRowBytes + x ];

			destination[ y2 * dstRowBytes + x2 ] = c;
			destination[ y2 * dstRowBytes + (x2 + 1) ] = c;
			destination[ (y2 + 1) * dstRowBytes + x2 ] = c;
			destination[ (y2 + 1) * dstRowBytes + (x2 + 1) ] = c;
		}
	}
}

/* BlitPixieDoubled16Bit (mac _BlitPixieDoubled16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDoubled16Bit i386 0x000bfd52 ppc 0x000cb9b4
void BlitPixieDoubled16Bit(
	unsigned short *source,			unsigned short *destination,
	uint32_t srcRowBytes,		uint32_t dstRowBytes,
	unsigned short width,			unsigned short height)
{
	int			x,y;
	int			x2,y2;
	unsigned short		c;

	for ( y = 0, y2 = 0; y < height; y++, y2 += 2 )
	{
		for ( x = 0, x2 = 0; x < width; x++, x2 += 2 )
		{
			c = source[ y * srcRowBytes + x ];

			destination[ y2 * dstRowBytes + x2 ] = c;
			destination[ y2 * dstRowBytes + (x2 + 1) ] = c;
			destination[ (y2 + 1) * dstRowBytes + x2 ] = c;
			destination[ (y2 + 1) * dstRowBytes + (x2 + 1) ] = c;
		}
	}
}

/* BlitPixieDoubled32Bit (mac _BlitPixieDoubled32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDoubled32Bit i386 0x000bfde0 ppc 0x000cba28
void BlitPixieDoubled32Bit(
	uint32_t *source,			uint32_t *destination,
	uint32_t srcRowBytes,		uint32_t dstRowBytes,
	unsigned short width,			unsigned short height)
{
	int			x,y;
	int			x2,y2;
	uint32_t		c;

	for ( y = 0, y2 = 0; y < height; y++, y2 += 2 )
	{
		for ( x = 0, x2 = 0; x < width; x++, x2 += 2 )
		{
			c = source[ y * srcRowBytes + x ];

			destination[ y2 * dstRowBytes + x2 ] = c;
			destination[ y2 * dstRowBytes + (x2 + 1) ] = c;
			destination[ (y2 + 1) * dstRowBytes + x2 ] = c;
			destination[ (y2 + 1) * dstRowBytes + (x2 + 1) ] = c;
		}
	}
}
