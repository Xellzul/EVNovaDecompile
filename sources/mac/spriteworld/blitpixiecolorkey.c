/* blitpixiecolorkey.c -- BlitPixieColorKey8/16/32Bit, the mac 1.1.1 BlitPixieColorkey.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

#define kAlphaMask16	0x7FFF
#define kAlphaMask32	0x00FFFFFFL

/* BlitPixieColorKey8Bit (mac _BlitPixieColorKey8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieColorKey8Bit i386 0x000c0b32 ppc 0x000cc928
void BlitPixieColorKey8Bit(
	unsigned char *src,
	unsigned char *dst,
	unsigned long srcOffset,
	unsigned long dstOffset,
	unsigned short width,
	unsigned short height,
	unsigned char colorKey )
{
	int		x;

	srcOffset -= width;
	dstOffset -= width;

	while ( height-- )
	{
		for (x = 0; x < width; x++)
		{
			if ( *src != colorKey)
				*dst = *src;

			src++;
			dst++;
		}

		src =  (  src + srcOffset );
		dst =  (  dst + dstOffset );
	}
}

/* BlitPixieColorKey16Bit (mac _BlitPixieColorKey16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieColorKey16Bit i386 0x000c0b9b ppc 0x000cc98c
void BlitPixieColorKey16Bit(
	unsigned short *src,
	unsigned short *dst,
	unsigned long srcOffset,
	unsigned long dstOffset,
	unsigned short width,
	unsigned short height,
	unsigned short colorKey )
{
	int		x;

	srcOffset -= (width + width);
	dstOffset -= (width + width);

	colorKey &= kAlphaMask16;
	while ( height-- )
	{
		for (x = 0; x < width; x++)
		{
			if ( (*src & kAlphaMask16) != colorKey)
				*dst = *src;

			src++;
			dst++;
		}

		src = (unsigned short *) ( (char *) src + srcOffset );
		dst = (unsigned short *) ( (char *) dst + dstOffset );
	}
}

/* BlitPixieColorKey32Bit (mac _BlitPixieColorKey32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieColorKey32Bit i386 0x000c0c17 ppc 0x000cc9f0
void BlitPixieColorKey32Bit(
	unsigned long *src,
	unsigned long *dst,
	unsigned long srcOffset,
	unsigned long dstOffset,
	unsigned short width,
	unsigned short height,
	unsigned long colorKey )
{
	int		x;

	srcOffset -= (width << 2);
	dstOffset -= (width << 2);

	colorKey &= kAlphaMask32;
	while ( height-- )
	{
		for (x = 0; x < width; x++)
		{
			if ( (*src & kAlphaMask32) != colorKey)
				*dst++ = *src;

			src++;
			dst++;
		}

		src = (unsigned long *) ( (char *) src + srcOffset );
		dst = (unsigned long *) ( (char *) dst + dstOffset );
	}
}
