/* blitpixiedirtyrects.c -- BlitPixieDirtyRects, the mac 1.1.1 BlitPixieDirtyRects.o (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

#define LOCAL static
#define MALLOC(s)		(void *) NewPtr( s )
#define FREE(p)			DisposePtr( (Ptr) p )

//	Local variables that store dirty rect variables

LOCAL		unsigned char	*matrixDirtyBits;	// bitmap dirty matrix ( 1 bit = 32x1 pixels)

LOCAL		unsigned short	matrixWidth;		// width (pixels)
LOCAL		unsigned short	matrixHeight;		// height (pixels)
LOCAL		unsigned short	matrixDepth;		// bits per pixel

LOCAL		unsigned short	matrixBytes;		// bytes (of screen pixels)
LOCAL		unsigned short	matrixRows;			// rows
LOCAL		unsigned short	matrixShift;		// bit positions (in depth)
LOCAL		unsigned short	matrixBits;			// pixel blocks of 32 bytes - matrix bits
LOCAL		unsigned long	matrixRowBytes;		// bytes per row (in matrix)

/* BlitPixieBlitClearDirtyRects (mac _BlitPixieBlitClearDirtyRects, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieBlitClearDirtyRects i386 0x000bf267 ppc 0x000cae44
void BlitPixieBlitClearDirtyRects(
	unsigned char	*src,
	unsigned char	*dst,
	unsigned long	color,
	unsigned long	srcBytes,
	unsigned long	dstBytes)
{
	int				x,y;
	unsigned char	*p,*row;
	unsigned int	c,m;

	srcBytes -= matrixBytes;
	dstBytes -= matrixBytes;

	row = matrixDirtyBits;
	for ( y = 0; y < matrixRows; y++ )
	{
		p = row;
		m = 0;

		for ( x = 0; x < matrixBits; x++ )
		{
			if ( m == 0 )
			{
				m = 0x80;
				c = *p;
				*p++ = 0;
			}

			if ( c & m )
			{
					// copy offscreen->screen
				BlitPixieMemCopy( dst, src, 32 );
					// clear offscreen
				BlitPixieMemSet( src, color, 32 );
			}

			src += 32;
			dst += 32;

			m >>= 1;
		}

		src += srcBytes;
		dst += dstBytes;

		row += matrixRowBytes;
	}
}

/* BlitPixieBlitEraseDirtyRects (mac _BlitPixieBlitEraseDirtyRects, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieBlitEraseDirtyRects i386 0x000bf323 ppc 0x000caf2c
void BlitPixieBlitEraseDirtyRects(
	unsigned char	*src,
	unsigned char	*dst,
	unsigned char	*bkg,
	unsigned long	srcBytes,
	unsigned long	dstBytes)
{
	int				x,y;
	unsigned char	*p,*row;
	unsigned int	c,m;

	srcBytes -= matrixBytes;
	dstBytes -= matrixBytes;

	row = matrixDirtyBits;
	for ( y = 0; y < matrixRows; y++ )
	{
		p = row;
		m = 0;

		for ( x = 0; x < matrixBits; x++ )
		{
			if ( m == 0 )
			{
				m = 0x80;
				c = *p;
				*p++ = 0;
			}

			if ( c & m )
			{
					// copy offscreen->screen
				BlitPixieMemCopy( dst, src, 32 );
					// erase offscreen
				BlitPixieMemCopy( src, bkg, 32 );
			}

			src += 32;
			dst += 32;
			bkg += 32;

			m >>= 1;
		}

		src += srcBytes;
		dst += dstBytes;
		bkg += srcBytes;

		row += matrixRowBytes;
	}
}

/* BlitPixieClearDirtyRects (mac _BlitPixieClearDirtyRects, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieClearDirtyRects i386 0x000bf3ec ppc 0x000cb01c
void BlitPixieClearDirtyRects( void )
{
		// clear all bits in dirty matrix
	BlitPixieMemSet(matrixDirtyBits,0L, matrixRowBytes * matrixRows);
}

/* BlitPixieAddDirtyRect (mac _BlitPixieAddDirtyRect, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieAddDirtyRect i386 0x000bf41b ppc 0x000cb040
void BlitPixieAddDirtyRect( Rect *dirtyRect )
{
	long				top,left,bottom,right;
	long				startBit,stopBit;
	long				y,bit,grid;
	unsigned char		*row,*p;
	unsigned char		m;

	if ( dirtyRect != NULL )
	{
		top = dirtyRect->top;
		left = dirtyRect->left;
		bottom = dirtyRect->bottom;
		right = dirtyRect->right;

		//	align to 32-byte grid
		grid = 32 >> matrixShift;
		left = (left) & ~(grid - 1);
		right = (right + grid - 1) & ~(grid - 1);

		//	clip
		if ( left < 0 )					left = 0;
		if ( top < 0 )					top = 0;
		if ( right >= matrixWidth )		right = matrixWidth - 1;
		if ( bottom >= matrixHeight )	bottom = matrixHeight - 1;

		//	check for empty/clipped rect
		if ( left >= right )	return;
		if ( top >= bottom )	return;
	}
	else
	{
			// add whole area
		left = 0;
		top = 0;
		right = matrixWidth - 1;
		bottom = matrixHeight - 1;
	}

	//	pixels to bytes
	left <<= matrixShift;
	right <<= matrixShift;

	//	calculate bit mask
	startBit = left >> 5;
	stopBit = right >> 5;

	//	add to dirty matrix (set bits)
	y = bottom - top;
	row = matrixDirtyBits + (top * matrixRowBytes);
	while (y--)
	{
		p = row + (startBit >> 3);
		m = 0x80U >> (startBit & 7);

		for ( bit = startBit; bit <= stopBit; bit++ )
		{
			*p |= m;

			m >>= 1;
			if ( m == 0)
			{
				p++;
				m = 0x80U;
			}
		}
		row += matrixRowBytes;
	}

}

/* BlitPixieExitDirtyRects (mac _BlitPixieExitDirtyRects, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieExitDirtyRects i386 0x000bf539 ppc 0x000cb184
void BlitPixieExitDirtyRects(void)
{
	if ( matrixDirtyBits )
	{
		FREE(matrixDirtyBits);
		matrixDirtyBits = NULL;
	}
}

/* BlitPixieInitDirtyRects (mac _BlitPixieInitDirtyRects, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieInitDirtyRects i386 0x000bf55c ppc 0x000cb1c8
Boolean BlitPixieInitDirtyRects( unsigned char *src, unsigned char *dst,
	unsigned short width, unsigned short height, unsigned short depth  )
{
	long	bytes;

	bytes = ( width * depth ) >> 3;

	if ( depth != 8 && depth != 16 && depth != 32 )
	{
		BLITPIXIE_DEBUGGER("DirtyRects: depth must be 8, 16 or 32");
		return false;
	}

	if ( (bytes & 31) != 0 )
	{
		BLITPIXIE_DEBUGGER("DirtyRects: bytes must be multiple of 32");
		return false;
	}

	if ( (((unsigned long) src) & 3) != 0 || (((unsigned long) dst) & 3) != 0)
	{
		BLITPIXIE_DEBUGGER("DirtyRects: src and dst must be aligned");
		return false;
	}

	matrixWidth = width;
	matrixHeight = height;
	matrixDepth = depth;

	matrixBytes = bytes;
	matrixRows = height;

	matrixShift = matrixDepth >> 4;
	matrixBits = matrixBytes >> 5;
	matrixRowBytes = (matrixBits + 7) >> 3;

	matrixDirtyBits =  MALLOC(matrixRowBytes * matrixRows);
	if ( matrixDirtyBits == NULL )
	{
		BLITPIXIE_DEBUGGER("DirtyRects: Couldn't get dirty matrix mem!");
		return false;
	}

	BlitPixieClearDirtyRects();
	BlitPixieAddDirtyRect(  NULL );

	return true;
}
