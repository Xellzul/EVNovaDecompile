/* blitpixiescale.c -- BlitPixieScale, the mac 1.1.1 BlitPixieScale.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

#define FP_SHIFT			16
#define TO_SCREEN			true

/* BlitPixieScale8Bit (mac _BlitPixieScale8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieScale8Bit i386 0x000bf669 ppc 0x000cb2c0
void BlitPixieScale8Bit(
	unsigned char *src,	uint32_t srcBytes,	int32_t width1, int32_t height1, int32_t startu,
	unsigned char *dst,	uint32_t dstBytes,	int32_t width2, int32_t height2, int32_t startv )
{
	uint32_t		u,du;
	uint32_t		out;
	int32_t				x;
	int32_t				errorTerm,error1,error2;

 	du = (width1 << FP_SHIFT) / width2;

	errorTerm =	(startv >> FP_SHIFT) - height2;
	error1 = height1+height1;
	error2 = height2+height2;

	dstBytes -= width2;

	while( height2-- )
	{
		u = startu;

		if ( TO_SCREEN )
		{
				// do longs
			for (x = 0; x < (width2 >> 2) ; x++)
			{
				out =              src[(u >> FP_SHIFT)];
				u += du;
				out = (out << 8) | src[(u >> FP_SHIFT)];
				u += du;
				out = (out << 8) | src[(u >> FP_SHIFT)];
				u += du;
				out = (out << 8) | src[(u >> FP_SHIFT)];
				u += du;

				*((uint32_t *) dst) = out;
				dst += sizeof(uint32_t);
			}

				// do bytes
			if (width2 & 2)
			{
				*dst++ = src[(u >> FP_SHIFT)];
				u += du;
				*dst++ = src[(u >> FP_SHIFT)];
				u += du;
			}
			if (width2 & 1)
			{
				*dst++ = src[(u >> FP_SHIFT)];
				u += du;
			}
		}
		else
		{
				// do bytes
			for (x = 0; x < width2; x++)
			{
				*dst++ = src[(u >> FP_SHIFT)];
				u += du;
			}
		}

			// advance to next row(s)
		errorTerm += error1;
		dst =  ( dst + dstBytes);
	  	while ( errorTerm > 0 )
		{
	  		src =  ( src + srcBytes);
			errorTerm -= error2;
	  	}

	}

}

/* BlitPixieScaleMask8Bit (mac _BlitPixieScaleMask8Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieScaleMask8Bit i386 0x000bf7a8 ppc 0x000cb3ec
void BlitPixieScaleMask8Bit(
	unsigned char *src,	uint32_t srcBytes,	int32_t width1, int32_t height1, int32_t startu,
	unsigned char *dst,	uint32_t dstBytes,	int32_t width2, int32_t height2, int32_t startv,
	unsigned char *mask)
{
	uint32_t		u,du;
	uint32_t		outpixel,outmask;
	int32_t				x;
	int32_t				errorTerm,error1,error2;

 	du = (width1 << FP_SHIFT) / width2;

	errorTerm =	(startv >> FP_SHIFT) - height2;
	error1 = height1+height1;
	error2 = height2+height2;

	dstBytes -= width2;

	while( height2-- )
	{
		u = startu;

		if ( TO_SCREEN )
		{
				// do longs
			for (x = 0; x < (width2 >> 2) ; x++)
			{
				outmask =                   mask[u >> FP_SHIFT];
				outpixel =                   src[u >> FP_SHIFT];
				u += du;
				outmask =  (outmask << 8) | mask[u >> FP_SHIFT];
				outpixel = (outpixel << 8) | src[u >> FP_SHIFT];
				u += du;
				outmask =  (outmask << 8) | mask[u >> FP_SHIFT];
				outpixel = (outpixel << 8) | src[u >> FP_SHIFT];
				u += du;
				outmask =  (outmask << 8) | mask[u >> FP_SHIFT];
				outpixel = (outpixel << 8) | src[u >> FP_SHIFT];
				u += du;

				*((uint32_t *) dst) &= outmask;
				*((uint32_t *) dst) |= outpixel;
				dst += sizeof(uint32_t);
			}

				// do bytes
			if (width2 & 2)
			{
				*dst &= mask[u >> FP_SHIFT];
				*dst |=  src[u >> FP_SHIFT];
				u += du;
				dst++;
				*dst &= mask[u >> FP_SHIFT];
				*dst |=  src[u >> FP_SHIFT];
				u += du;
				dst++;
			}
			if (width2 & 1)
			{
				*dst &= mask[u >> FP_SHIFT];
				*dst |=  src[u >> FP_SHIFT];
				u += du;
				dst++;
			}
		}
		else
		{
				// do bytes
			for (x = 0; x < width2; x++)
			{
				*dst &= mask[u >> FP_SHIFT];
				*dst |=  src[u >> FP_SHIFT];
				u += du;
				dst++;
			}
		}
			// advance to next row(s)
		errorTerm += error1;
		dst =  ( dst + dstBytes);
	  	while ( errorTerm > 0 )
		{
	  		src =  ( src + srcBytes);
			mask =  ( mask + srcBytes);
			errorTerm -= error2;
	  	}

	}

}

/* BlitPixieScale16Bit (mac _BlitPixieScale16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieScale16Bit i386 0x000bf95b ppc 0x000cb5ac
void BlitPixieScale16Bit(
	unsigned short *src, uint32_t srcBytes,	int32_t width1, int32_t height1, int32_t startu,
	unsigned short *dst, uint32_t dstBytes,	int32_t width2, int32_t height2, int32_t startv )
{
	uint32_t		u,du;
	uint32_t		out;
	int32_t				x;
	int32_t				errorTerm,error1,error2;

 	du = (width1 << FP_SHIFT) / width2;

	errorTerm =	(startv >> FP_SHIFT) - height2;
	error1 = height1+height1;
	error2 = height2+height2;

	dstBytes -= width2 + width2;

	while( height2-- )
	{
		u = startu;

		if ( TO_SCREEN )
		{
				// do longs
			for (x = 0; x < (width2 >> 1) ; x++)
			{
				out =               src[(u >> FP_SHIFT)];
				u += du;
				out = (out << 16) | src[(u >> FP_SHIFT)];
				u += du;

				*((uint32_t *) dst) = out;
				dst += sizeof(uint32_t);
			}

				// do shorts
			if (width2 & 1)
			{
				*dst++ = src[(u >> FP_SHIFT)];
				u += du;
			}
		}
		else
		{
				// do shorts
			for (x = 0; x < width2; x++)
			{
				*dst++ = src[(u >> FP_SHIFT)];
				u += du;
			}
		}
			// advance to next row(s)
		errorTerm += error1;
		dst = (unsigned short *) ((char *) dst + dstBytes);
	  	while ( errorTerm > 0 )
		{
	  		src = (unsigned short *) ((char *) src + srcBytes);
			errorTerm -= error2;
	  	}

	}

}

/* BlitPixieScaleMask16Bit (mac _BlitPixieScaleMask16Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieScaleMask16Bit i386 0x000bfa2a ppc 0x000cb67c
void BlitPixieScaleMask16Bit(
	unsigned short *src, uint32_t srcBytes,	int32_t width1, int32_t height1, int32_t startu,
	unsigned short *dst, uint32_t dstBytes,	int32_t width2, int32_t height2, int32_t startv,
	unsigned short *mask)
{
	uint32_t		u,du;
	uint32_t		outpixel,outmask;
	int32_t				x;
	int32_t				errorTerm,error1,error2;

 	du = (width1 << FP_SHIFT) / width2;

	errorTerm =	(startv >> FP_SHIFT) - height2;
	error1 = height1+height1;
	error2 = height2+height2;

	dstBytes -= width2 + width2;

	while( height2-- )
	{
		u = startu;

		if ( TO_SCREEN )
		{
				// do longs
			for (x = 0; x < (width2 >> 1) ; x++)
			{
				outmask =                    mask[u >> FP_SHIFT];
				outpixel =                    src[u >> FP_SHIFT];
				u += du;
				outmask =  (outmask << 16) | mask[u >> FP_SHIFT];
				outpixel = (outpixel << 16) | src[u >> FP_SHIFT];
				u += du;

				*((uint32_t *) dst) &= outmask;
				*((uint32_t *) dst) |= outpixel;
				dst += sizeof(uint32_t);
			}

				// do shorts
			if (width2 & 1)
			{
				*dst &= mask[u >> FP_SHIFT];
				*dst |=  src[u >> FP_SHIFT];
				u += du;
				dst++;
			}
		}
		else
		{
				// do shorts
			for (x = 0; x < width2; x++)
			{
				*dst &= mask[u >> FP_SHIFT];
				*dst |=  src[u >> FP_SHIFT];
				u += du;
				dst++;
			}
		}
			// advance to next row(s)
		errorTerm += error1;
		dst = (unsigned short *) ((char *) dst + dstBytes);
	  	while ( errorTerm > 0 )
		{
	  		src = (unsigned short *) ((char *) src + srcBytes);
			mask = (unsigned short *) ((char *) mask + srcBytes);
			errorTerm -= error2;
	  	}

	}

}

/* BlitPixieScale32Bit (mac _BlitPixieScale32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieScale32Bit i386 0x000bfb4a ppc 0x000cb788
void BlitPixieScale32Bit(
	uint32_t *src,	uint32_t srcBytes,	int32_t width1, int32_t height1, int32_t startu,
	uint32_t *dst,	uint32_t dstBytes,	int32_t width2, int32_t height2, int32_t startv )
{
	uint32_t		u,du;
	int32_t				x;
	int32_t				errorTerm,error1,error2;

 	du = (width1 << FP_SHIFT) / width2;

	errorTerm =	(startv >> FP_SHIFT) - height2;
	error1 = height1+height1;
	error2 = height2+height2;

	dstBytes -= width2 << 2;

	while( height2-- )
	{
		u = startu;

			// do longs
		for (x = 0; x < width2; x++)
		{
			*dst++ = src[(u >> FP_SHIFT)];
			u += du;
		}

			// advance to next row(s)
		errorTerm += error1;
		dst = (uint32_t *) ((char *) dst + dstBytes);
	  	while ( errorTerm > 0 )
		{
	  		src = (uint32_t *) ((char *) src + srcBytes);
			errorTerm -= error2;
	  	}
	}

}

/* BlitPixieScaleMask32Bit (mac _BlitPixieScaleMask32Bit, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieScaleMask32Bit i386 0x000bfbd5 ppc 0x000cb824
void BlitPixieScaleMask32Bit(
	uint32_t *src,	uint32_t srcBytes,	int32_t width1, int32_t height1, int32_t startu,
	uint32_t *dst,	uint32_t dstBytes,	int32_t width2, int32_t height2, int32_t startv,
	uint32_t *mask)
{
	uint32_t		u,du;
	int32_t				x;
	int32_t				errorTerm,error1,error2;

 	du = (width1 << FP_SHIFT) / width2;

	errorTerm =	(startv >> FP_SHIFT) - height2;
	error1 = height1+height1;
	error2 = height2+height2;

	dstBytes -= width2 << 2;

	while( height2-- )
	{
		u = startu;

			// do longs
		for (x = 0; x < width2; x++)
		{
			*dst &= mask[u >> FP_SHIFT];
			*dst |=  src[u >> FP_SHIFT];
			u += du;
			dst++;
		}

			// advance to next row(s)
		errorTerm += error1;
		dst = (uint32_t *) ((char *) dst + dstBytes);
	  	while ( errorTerm > 0 )
		{
	  		src = (uint32_t *) ((char *) src + srcBytes);
			mask = (uint32_t *) ((char *) mask + srcBytes);
			errorTerm -= error2;
	  	}
	}

}
