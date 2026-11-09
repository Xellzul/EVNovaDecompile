/* blitpixieflip.c -- BlitPixieFlip*, the mac 1.1.1 BlitPixieFlip.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

/* BlitPixieFlip8Bit (mac _BlitPixieFlip8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlip8Bit i386 0x000bfe69 ppc 0x000cba9c
void BlitPixieFlip8Bit(
	unsigned char *src,				unsigned char *dst,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short width,			unsigned short height)
{
	unsigned long		x,y;

	srcRowBytes += width;
	dstRowBytes -= width;

	src += width;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
			*dst++ = *--src;

		src += srcRowBytes;
		dst += dstRowBytes;
	}
}

/* BlitPixieFlip16Bit (mac _BlitPixieFlip16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlip16Bit i386 0x000bfebd ppc 0x000cbafc
void BlitPixieFlip16Bit(
	unsigned short *src,			unsigned short *dst,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short width,			unsigned short height)
{
	unsigned long			x,y;

	srcRowBytes += width << 1;
	dstRowBytes -= width << 1;

	src += width;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
			*dst++ = *--src;

		src = (unsigned short *) ( (char *) src + srcRowBytes);
		dst = (unsigned short *) ( (char *) dst + dstRowBytes);
	}
}

/* BlitPixieFlip32Bit (mac _BlitPixieFlip32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlip32Bit i386 0x000bff18 ppc 0x000cbb54
void BlitPixieFlip32Bit(
	unsigned long *src,				unsigned long *dst,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short width,			unsigned short height)
{
	unsigned long			x,y;

	srcRowBytes += width << 2;
	dstRowBytes -= width << 2;

	src += width;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
			*dst++ = *--src;

		src = (unsigned long *) ( (char *) src + srcRowBytes);
		dst = (unsigned long *) ( (char *) dst + dstRowBytes);
	}
}

/* BlitPixieFlipMask8Bit (mac _BlitPixieFlipMask8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMask8Bit i386 0x000bff75 ppc 0x000cbbac
void BlitPixieFlipMask8Bit(
	unsigned char *src,				unsigned char *dst,			unsigned char *mask,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short width,			unsigned short height)
{
	unsigned long		x,y;
	unsigned char		m;

	srcRowBytes += width;
	dstRowBytes -= width;

	src += width;
	mask += width;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			m = *--mask;
			*dst++ = (*dst & m) | (*--src &~ m);
		}

		src += srcRowBytes;
		mask += srcRowBytes;
		dst += dstRowBytes;
	}
}

/* BlitPixieFlipMask16Bit (mac _BlitPixieFlipMask16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMask16Bit i386 0x000bfff0 ppc 0x000cbc38
void BlitPixieFlipMask16Bit(
	unsigned short *src,			unsigned short *dst,			unsigned short *mask,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short width,			unsigned short height)
{
	unsigned long		x,y;
	unsigned short		m;

	srcRowBytes += width << 1;
	dstRowBytes -= width << 1;

	src += width;
	mask += width;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			m = *--mask;
			*dst++ = (*dst & m) | (*--src &~ m);
		}

		src = (unsigned short *) ( (char *) src + srcRowBytes);
		mask = (unsigned short *) ( (char *) mask + srcRowBytes);
		dst = (unsigned short *) ( (char *) dst + dstRowBytes);
	}
}

/* BlitPixieFlipMask32Bit (mac _BlitPixieFlipMask32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMask32Bit i386 0x000c0088 ppc 0x000cbcb0
void BlitPixieFlipMask32Bit(
	unsigned long *src,				unsigned long *dst,				unsigned long *mask,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short width,			unsigned short height)
{
	unsigned long		x,y;
	unsigned long		m;

	srcRowBytes += width << 2;
	dstRowBytes -= width << 2;

	src += width;
	mask += width;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			m = *--mask;
			*dst++ = (*dst & m) | (*--src &~ m);
		}

		src = (unsigned long *) ( (char *) src + srcRowBytes);
		mask = (unsigned long *) ( (char *) mask + srcRowBytes);
		dst = (unsigned long *) ( (char *) dst + dstRowBytes);
	}
}

/* BlitPixieFlipMaskCollision (mac _BlitPixieFlipMaskCollision, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMaskCollision i386 0x000c011d ppc 0x000cbd28
Boolean BlitPixieFlipMaskCollision(
	unsigned char *maskPtr1,	unsigned char *maskPtr2,
	unsigned long rowBytes1,	unsigned long rowBytes2,
	unsigned short bytes, 		unsigned short rows)
{
	unsigned long	x;

	rowBytes1 -= bytes;
	maskPtr2 += bytes;

	while ( rows-- )
	{
		for ( x = 0; x < bytes; x++)
		{
			if ( (*maskPtr1++ | *--maskPtr2) == 0 )
				return true;
		}

		maskPtr1 += rowBytes1;
		maskPtr2 += rowBytes2;
	}

	return false;
}

/* BlitPixieFlipMaskColor (mac _BlitPixieFlipMaskColor, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMaskColor i386 0x000c017d ppc 0x000cbdac
void BlitPixieFlipMaskColor(
	unsigned long srcColor,			unsigned char *dstPixelP,		unsigned char *maskPixelP,
	unsigned long srcRowBytes,		unsigned long dstRowBytes,
	unsigned short bytes,			unsigned short rows )
{
	unsigned long index;
	unsigned long mask;

	srcRowBytes += bytes;
	dstRowBytes -= bytes;

	maskPixelP += bytes;

	while (rows--)
	{
		// note: must do this with longs and shorts first, since depth can be both 32, 16 or 8 bits

		for (index=0; index < (bytes>>2); index++)
		{
			maskPixelP -= sizeof(unsigned long);
		#if USE_PPC_ASSEMBLY
			mask = __lwbrx( maskPixelP, 0 );
		#else
			mask = *((unsigned long *) maskPixelP);
			mask = ((mask >> 24) & 0xFF) | ((mask >> 8) & 0xFF00) | ((mask & 0xFF00) << 8) | ((mask & 0xFF) << 24);
		#endif
			*((unsigned long *) dstPixelP) = (*((unsigned long *) dstPixelP) & mask) | (srcColor &~ mask);
			dstPixelP += sizeof(unsigned long);
		}

		if ( bytes & 2 )
		{
			maskPixelP -= sizeof(unsigned short);
		#if USE_PPC_ASSEMBLY
			mask = __lhbrx( maskPixelP, 0 );
		#else
			mask = *((unsigned short *) maskPixelP);
			mask = (mask >> 8) & 0x00FF | ( (mask << 8) & 0xFF00 );
		#endif
			*((unsigned short *) dstPixelP) = (*((unsigned short *) dstPixelP) & mask) | (srcColor &~ mask);
			dstPixelP += sizeof(unsigned short);
		}

		if ( bytes & 1 )
		{
			mask = *--maskPixelP;
			*dstPixelP++ = (*dstPixelP & mask) | (srcColor &~ mask);
		}

			// bump to next row
		dstPixelP += dstRowBytes;
		maskPixelP += srcRowBytes;
	}
}
