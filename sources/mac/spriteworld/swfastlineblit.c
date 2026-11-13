/* swfastlineblit.c -- SWFastLine.o, the diagonal and straight line blitters: BlitLine to BPCopyStraightWrappingLine (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/swutils.h"
#include "../include/swfastline.h"

#define SIGN(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))
#define ABS(x) ((x) < 0 ? -(x) : (x))

/* BlitLine (mac _BlitLine, mac-only). STATE: see the mac gate. */
// MAC: _BlitLine i386 0x000c6a60 ppc 0x000d2d88
void BlitLine(
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Byte color)
{
	register Ptr	baseAddr = dstFrameP->frameBaseAddr;
	uint32_t	rowBytes = dstFrameP->frameRowBytes;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy, sx, sy, iy;
	short			accum;

	dx = x2 - x1;
	dy = y2 - y1;

	sx = SIGN(dx);
	sy = SIGN(dy);

	dx = ABS(dx);
	dy = ABS(dy);

	iy = (sy < 0) ? -rowBytes : rowBytes;

	baseAddr += dstFrameP->scanLinePtrArray[y1] + x1;
	*baseAddr = color;	// Draw the first pixel

	if(dx > dy)		// Longer horizontally than vertically
	{
		accum = dx >> 1;
		while(x1 != x2)
		{
			accum -= dy;
			if(accum < 0)
			{
				accum += dx;
				baseAddr += iy;
			}
			x1 += sx;
			baseAddr += sx;
			*baseAddr = color;
		}
	}
	else 	// Longer vertically than horizontally, or both equal in length
	{
		accum = dy >> 1;
		while(y1 != y2)
		{
			accum -= dx;
			if(accum < 0)
			{
				accum += dy;
				baseAddr += sx;
			}
			y1 += sy;
			baseAddr += iy;
			*baseAddr = color;
		}
	}
}

/* BPCopyLine (mac _BPCopyLine, mac-only). STATE: see the mac gate. */
// MAC: _BPCopyLine i386 0x000c6ef6 ppc 0x000d31d8
void BPCopyLine(
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint)
{
	Ptr				srcBaseAddr = srcFrameP->frameBaseAddr;
	Ptr				dstBaseAddr = dstFrameP->frameBaseAddr;
	uint32_t	srcRowBytes = srcFrameP->frameRowBytes;
	uint32_t	dstRowBytes = dstFrameP->frameRowBytes;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy, sx, sy, srciy, dstiy;
	short			accum;

	dx = x2 - x1;
	dy = y2 - y1;

	sx = SIGN(dx);
	sy = SIGN(dy);

	dx = ABS(dx);
	dy = ABS(dy);

	srcBaseAddr += srcFrameP->scanLinePtrArray[y1] + x1;
	dstBaseAddr += dstFrameP->scanLinePtrArray[y1] + x1;

		// Copy the first pixel in the line
	*dstBaseAddr = *srcBaseAddr;

	srciy = (sy < 0) ? -srcRowBytes : srcRowBytes;
	dstiy = (sy < 0) ? -dstRowBytes : dstRowBytes;


	if (dx > dy)		// Longer horizontally than vertically
	{
		accum = dx >> 1;
		while(x1 != x2)
		{
			accum -= dy;
			if(accum < 0)
			{
				accum += dx;
				srcBaseAddr += srciy;
				dstBaseAddr += dstiy;
			}
			x1 += sx;
			srcBaseAddr += sx;
			dstBaseAddr += sx;
			*dstBaseAddr = *srcBaseAddr;
		}
	}
	else 	// Longer vertically than horizontally, or both equal in length
	{
		accum = dy >> 1;
		while(y1 != y2)
		{
			accum -= dx;
			if(accum < 0)
			{
				accum += dy;
				srcBaseAddr += sx;
				dstBaseAddr += sx;
			}
			y1 += sy;
			srcBaseAddr += srciy;
			dstBaseAddr += dstiy;
			*dstBaseAddr = *srcBaseAddr;
		}
	}
}

/* BlitWrappingLine (mac _BlitWrappingLine, mac-only). STATE: see the mac gate. */
// MAC: _BlitWrappingLine i386 0x000c79be ppc 0x000d3bd8
void BlitWrappingLine(
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Byte color)
{
	Ptr				baseAddr;
	uint32_t	rowBytes = dstFrameP->frameRowBytes;
	uint32_t	vertOffset;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy, sx, sy, iy;
	short			accum;
	Rect			frameRect = dstFrameP->frameRect;
	short			frameWidth, frameHeight;

	frameWidth = SW_RECT_WIDTH(frameRect);
	frameHeight = SW_RECT_HEIGHT(frameRect);

		// vertOffset = rowBytes * frameHeight;
	vertOffset = dstFrameP->scanLinePtrArray[dstFrameP->numScanLines-1] + rowBytes -
				 dstFrameP->scanLinePtrArray[0];

	dx = x2 - x1;
	dy = y2 - y1;

	sx = SIGN(dx);
	sy = SIGN(dy);

	dx = ABS(dx);
	dy = ABS(dy);

		// Even the very first pixel we draw might need to be wrapped before we draw it.
	if (y1 >= frameRect.bottom)
	{
		y1 -= frameHeight;
		y2 -= frameHeight;
	}
	else if (y1 < frameRect.top)
	{
		y1 += frameHeight;
		y2 += frameHeight;
	}

	if (x1 >= frameRect.right)
	{
		x1 -= frameWidth;
		x2 -= frameWidth;
	}
	else if (x1 < frameRect.left)
	{
		x1 += frameWidth;
		x2 += frameWidth;
	}

	baseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y1] + x1;
	*baseAddr = color;

	iy = (sy < 0) ? -rowBytes : rowBytes;

	if (dx > dy)		// Longer horizontally than vertically
	{
		accum = dx >> 1;	// accum = dx/2;
		while (x1 != x2)
		{
			accum -= dy;
			if (accum < 0)
			{
				accum += dx;
				y1 += sy;
				baseAddr += iy;		// Move pixel up/down one row

				if (y1 >= frameRect.bottom)
				{
					y1 -= frameHeight;
					y2 -= frameHeight;
					baseAddr -= vertOffset;		// Jump to top side
				}
				else if (y1 < frameRect.top)
				{
					y1 += frameHeight;
					y2 += frameHeight;
					baseAddr += vertOffset;		// Jump to bottom side
				}
			}
			x1 += sx;
			baseAddr += sx;				// Move left/right one pixel

			if (x1 >= frameRect.right)
			{
				x1 -= frameWidth;
				x2 -= frameWidth;
				baseAddr -= frameWidth;	// Jump to left side
			}
			else if (x1 < frameRect.left)
			{
				x1 += frameWidth;
				x2 += frameWidth;
				baseAddr += frameWidth;	// Jump to right side
			}

			*baseAddr = color;
		}
	}
	else 	// Longer vertically than horizontally, or both equal in length
	{
		accum = dy >> 1;	// accum = dy/2;
		while (y1 != y2)
		{
			accum -= dx;
			if (accum < 0)
			{
				accum += dy;
				x1 += sx;
				baseAddr += sx;

				if (x1 >= frameRect.right)
				{
					x1 -= frameWidth;
					x2 -= frameWidth;
					baseAddr -= frameWidth;	// Jump to left side
				}
				else if (x1 < frameRect.left)
				{
					x1 += frameWidth;
					x2 += frameWidth;
					baseAddr += frameWidth;	// Jump to right side
				}
			}
			y1 += sy;
			baseAddr += iy;

			if (y1 >= frameRect.bottom)
			{
				y1 -= frameHeight;
				y2 -= frameHeight;
				baseAddr -= vertOffset;		// Jump to top side
			}
			else if (y1 < frameRect.top)
			{
				y1 += frameHeight;
				y2 += frameHeight;
				baseAddr += vertOffset;		// Jump to bottom side
			}

			*baseAddr = color;
		}
	}
}

/* BPCopyWrappingLine (mac _BPCopyWrappingLine, mac-only). STATE: see the mac gate. */
// MAC: _BPCopyWrappingLine i386 0x000c84fd ppc 0x000d4618
void BPCopyWrappingLine(
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint)
{
	Ptr				srcBaseAddr = srcFrameP->frameBaseAddr;
	Ptr				dstBaseAddr = dstFrameP->frameBaseAddr;
	uint32_t	srcRowBytes = srcFrameP->frameRowBytes;
	uint32_t	dstRowBytes = dstFrameP->frameRowBytes;
	uint32_t	srcVertOffset, dstVertOffset;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy, sx, sy, srciy, dstiy;
	short			accum;
	Rect			frameRect = dstFrameP->frameRect;
	short			frameWidth, frameHeight;

	frameWidth = SW_RECT_WIDTH(frameRect);
	frameHeight = SW_RECT_HEIGHT(frameRect);

		// srcVertOffset = srcRowBytes * frameHeight;
	srcVertOffset = srcFrameP->scanLinePtrArray[srcFrameP->numScanLines-1] + srcRowBytes -
					srcFrameP->scanLinePtrArray[0];
	dstVertOffset = dstFrameP->scanLinePtrArray[dstFrameP->numScanLines-1] + dstRowBytes -
					dstFrameP->scanLinePtrArray[0];

	dx = x2 - x1;
	dy = y2 - y1;

	sx = SIGN(dx);
	sy = SIGN(dy);

	dx = ABS(dx);
	dy = ABS(dy);

		// Even the very first pixel we draw might need to be wrapped before we draw it.
	if (y1 >= frameRect.bottom)
	{
		y1 -= frameHeight;
		y2 -= frameHeight;
	}
	else if (y1 < frameRect.top)
	{
		y1 += frameHeight;
		y2 += frameHeight;
	}

	if (x1 >= frameRect.right)
	{
		x1 -= frameWidth;
		x2 -= frameWidth;
	}
	else if (x1 < frameRect.left)
	{
		x1 += frameWidth;
		x2 += frameWidth;
	}

	srcBaseAddr += srcFrameP->scanLinePtrArray[y1] + x1;
	dstBaseAddr += dstFrameP->scanLinePtrArray[y1] + x1;

		// Copy the first pixel in the line
	*dstBaseAddr = *srcBaseAddr;

	srciy = (sy < 0) ? -srcRowBytes : srcRowBytes;
	dstiy = (sy < 0) ? -dstRowBytes : dstRowBytes;

	if (dx > dy)		// Longer horizontally than vertically
	{
		accum = dx >> 1;	// accum = dx/2;
		while (x1 != x2)
		{
			accum -= dy;
			if (accum < 0)
			{
				accum += dx;
				y1 += sy;
				srcBaseAddr += srciy;		// Move pixel up/down one row
				dstBaseAddr += dstiy;

				if (y1 >= frameRect.bottom)
				{
					y1 -= frameHeight;
					y2 -= frameHeight;
					srcBaseAddr -= srcVertOffset;		// Jump to top side
					dstBaseAddr -= dstVertOffset;
				}
				else if (y1 < frameRect.top)
				{
					y1 += frameHeight;
					y2 += frameHeight;
					srcBaseAddr += srcVertOffset;		// Jump to bottom side
					dstBaseAddr += dstVertOffset;
				}
			}
			x1 += sx;
			srcBaseAddr += sx;				// Move left/right one pixel
			dstBaseAddr += sx;

			if (x1 >= frameRect.right)
			{
				x1 -= frameWidth;
				x2 -= frameWidth;
				srcBaseAddr -= frameWidth;	// Jump to left side
				dstBaseAddr -= frameWidth;
			}
			else if (x1 < frameRect.left)
			{
				x1 += frameWidth;
				x2 += frameWidth;
				srcBaseAddr += frameWidth;	// Jump to right side
				dstBaseAddr += frameWidth;
			}

			*dstBaseAddr = *srcBaseAddr;	// Copy the pixel
		}
	}
	else 	// Longer vertically than horizontally, or both equal in length
	{
		accum = dy >> 1;	// accum = dy/2;
		while (y1 != y2)
		{
			accum -= dx;
			if (accum < 0)
			{
				accum += dy;
				x1 += sx;
				srcBaseAddr += sx;
				dstBaseAddr += sx;

				if (x1 >= frameRect.right)
				{
					x1 -= frameWidth;
					x2 -= frameWidth;
					srcBaseAddr -= frameWidth;	// Jump to left side
					dstBaseAddr -= frameWidth;
				}
				else if (x1 < frameRect.left)
				{
					x1 += frameWidth;
					x2 += frameWidth;
					srcBaseAddr += frameWidth;	// Jump to right side
					dstBaseAddr += frameWidth;
				}
			}
			y1 += sy;
			srcBaseAddr += srciy;
			dstBaseAddr += dstiy;

			if (y1 >= frameRect.bottom)
			{
				y1 -= frameHeight;
				y2 -= frameHeight;
				srcBaseAddr -= srcVertOffset;		// Jump to top side
				dstBaseAddr -= dstVertOffset;
			}
			else if (y1 < frameRect.top)
			{
				y1 += frameHeight;
				y2 += frameHeight;
				srcBaseAddr += srcVertOffset;		// Jump to bottom side
				dstBaseAddr += dstVertOffset;
			}

			*dstBaseAddr = *srcBaseAddr;	// Copy the pixel
		}
	}
}

/* BlitStraightLine (mac _BlitStraightLine, mac-only). STATE: see the mac gate. */
// MAC: _BlitStraightLine i386 0x000c63f0 ppc 0x000d2700
void BlitStraightLine(
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Byte color)
{
	uint32_t	rowBytes = dstFrameP->frameRowBytes;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy;


	if (x1 == x2)		// Straight vertical line
	{
		register Ptr	baseAddr;

		if (y2 < y1)
		{
			y1 = dstPoint.v;	// Swap y1 and y2 if they're
			y2 = srcPoint.v;	// in the wrong order.
		}

			// Make sure some part of the line is inside the frameRect
		if (x1 < dstFrameP->frameRect.left || x1 >= dstFrameP->frameRect.right)
			return;

			// Clip the ends of the line
		if (y1 < dstFrameP->frameRect.top)
			y1 = dstFrameP->frameRect.top;
		if (y2 >= dstFrameP->frameRect.bottom)
			y2 = dstFrameP->frameRect.bottom-1;

		dy = y2 - y1 + 1;	// dy = number of pixels in line
		if (dy > 0)	// Make sure there is a line first!
		{
			baseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y1] + x1;
			*baseAddr = color;

			while (--dy)
			{
				baseAddr += rowBytes;
				*baseAddr = color;
			}
		}
	}
	else if (y1 == y2)	// Straight horizontal line
	{
		register uint32_t 	*longBaseAddr;
		register 				numLongBlits;
		uint32_t			longColor;

			// Fill longColor with color
		longColor = (color<<24) | (color<<16) | (color<<8) | (color);

		if (x2 < x1)
		{
			x1 = dstPoint.h;	// Swap x1 and x2 if they're
			x2 = srcPoint.h;	// in the wrong order.
		}

			// Make sure some part of the line is inside the frameRect
		if (y1 < dstFrameP->frameRect.top || y1 >= dstFrameP->frameRect.bottom)
			return;

		if (x1 < dstFrameP->frameRect.left)
			x1 = dstFrameP->frameRect.left;
		if (x2 >= dstFrameP->frameRect.right)
			x2 = dstFrameP->frameRect.right-1;

		dx = x2 - x1 + 1;	// dx = number of pixels in line
		if (dx > 0)	// Make sure there is a line first!
		{
			numLongBlits = (dx >> 2);
			longBaseAddr = (uint32_t *)(dstFrameP->frameBaseAddr +
					dstFrameP->scanLinePtrArray[y1] + x1);

			while (numLongBlits--)
			{
				*(longBaseAddr)++ = longColor;
			}

#ifdef __MWERKS__  // Do it nicely
				// Handle extra pixels at end of line
			if (dx & 0x2)
				*((unsigned short *) longBaseAddr)++ = longColor;
			if (dx & 0x1)
				*((unsigned char *) longBaseAddr)++ = color;
#else 	// Make Think C happy
				// Handle extra pixels at end of line
			if (dx & 0x2)
			{
				*((unsigned short *) longBaseAddr) = longColor;
				longBaseAddr = (uint32_t *)(((unsigned short *)longBaseAddr) + 1);
			}
			if (dx & 0x1)
			{
				*((unsigned char *) longBaseAddr) = color;
				longBaseAddr = ((longBaseAddr) + 1);
			}
#endif
		}
	}
}

/* BPCopyStraightLine (mac _BPCopyStraightLine, mac-only). STATE: see the mac gate. */
// MAC: _BPCopyStraightLine i386 0x000c6cf6 ppc 0x000d2fe4
void BPCopyStraightLine(
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint)
{
	uint32_t	srcRowBytes = srcFrameP->frameRowBytes;
	uint32_t	dstRowBytes = dstFrameP->frameRowBytes;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy;


	if (x1 == x2)		// Straight vertical line
	{
		register Ptr	srcBaseAddr;
		register Ptr	dstBaseAddr;

		if (y2 < y1)
		{
			y1 = dstPoint.v;	// Swap y1 and y2 if they're
			y2 = srcPoint.v;	// in the wrong order.
		}

			// Make sure some part of the line is inside the frameRect
		if (x1 < dstFrameP->frameRect.left || x1 >= dstFrameP->frameRect.right)
			return;

			// Clip the ends of the line
		if (y1 < dstFrameP->frameRect.top)
			y1 = dstFrameP->frameRect.top;
		if (y2 >= dstFrameP->frameRect.bottom)
			y2 = dstFrameP->frameRect.bottom-1;

		dy = y2 - y1 + 1;	// dy = number of pixels in line
		if (dy > 0)	// Make sure there is a line first!
		{
			srcBaseAddr = srcFrameP->frameBaseAddr + srcFrameP->scanLinePtrArray[y1] + x1;
			dstBaseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y1] + x1;

			*dstBaseAddr = *srcBaseAddr;	// Copy the first pixel

			while (--dy)
			{
				srcBaseAddr += srcRowBytes;
				dstBaseAddr += dstRowBytes;
				*dstBaseAddr = *srcBaseAddr;
			}
		}
	}
	else if (y1 == y2)	// Straight horizontal line
	{
		register uint32_t	*srcBaseAddr;
		register uint32_t	*dstBaseAddr;
		register 				numLongBlits;

		if (x2 < x1)
		{
			x1 = dstPoint.h;	// Swap x1 and x2 if they're
			x2 = srcPoint.h;	// in the wrong order.
		}

			// Make sure some part of the line is inside the frameRect
		if (y1 < dstFrameP->frameRect.top || y1 >= dstFrameP->frameRect.bottom)
			return;

			// Clip the ends of the line
		if (x1 < dstFrameP->frameRect.left)
			x1 = dstFrameP->frameRect.left;
		if (x2 >= dstFrameP->frameRect.right)
			x2 = dstFrameP->frameRect.right-1;

		dx = x2 - x1 + 1;	// dx = number of pixels in line
		if (dx > 0)		// Make sure there is a line first!
		{
			numLongBlits = (dx >> 2);
			srcBaseAddr = (uint32_t *)(srcFrameP->frameBaseAddr +
					srcFrameP->scanLinePtrArray[y1] + x1);
			dstBaseAddr = (uint32_t *)(dstFrameP->frameBaseAddr +
					dstFrameP->scanLinePtrArray[y1] + x1);

			while (numLongBlits--)
			{
				*(dstBaseAddr)++ = *(srcBaseAddr)++;
			}


#ifdef __MWERKS__  // Do it nicely
				// Handle extra pixels at end of line
			if (dx & 0x2)
				*((unsigned short *) dstBaseAddr)++ = *((unsigned short *) srcBaseAddr)++;
			if (dx & 0x1)
				*((unsigned char *) dstBaseAddr)++ = *((unsigned char *) srcBaseAddr)++;
#else 	// Make Think C happy
				// Handle extra pixels at end of line
			if (dx & 0x2)
			{
				*((unsigned short *) dstBaseAddr) = *((unsigned short *) srcBaseAddr);
				dstBaseAddr = (uint32_t *)(((unsigned short *)dstBaseAddr) + 1);
				srcBaseAddr = (uint32_t *)(((unsigned short *)srcBaseAddr) + 1);
			}
			if (dx & 0x1)
			{
				*((unsigned char *) dstBaseAddr) = *((unsigned char *) srcBaseAddr);
				dstBaseAddr = (uint32_t *)(((unsigned char *)dstBaseAddr) + 1);
				srcBaseAddr = (uint32_t *)(((unsigned char *)srcBaseAddr) + 1);
			}
#endif
		}
	}
}

/* BlitStraightWrappingLine (mac _BlitStraightWrappingLine, mac-only). STATE: see the mac gate. */
// MAC: _BlitStraightWrappingLine i386 0x000c761b ppc 0x000d3854
void BlitStraightWrappingLine(
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Byte color)
{
	uint32_t	rowBytes = dstFrameP->frameRowBytes;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy;

	if (x1 == x2)		// Straight vertical line
	{
		register Ptr	baseAddr;
		short			y3, y4;		// Second half of wrapped line

		if (y2 < y1)
		{
			y1 = dstPoint.v;	// Swap y1 and y2 if they're
			y2 = srcPoint.v;	// in the wrong order.
		}

			// Wrap line horizontally if necessary before we start (only done once)
			// Remember x1 and x2 are the same, and x2 isn't used.
		if (x1 < dstFrameP->frameRect.left)
			x1 += dstFrameP->frameRect.right;
		else if (x1 >= dstFrameP->frameRect.right)
			x1 -= dstFrameP->frameRect.right;

			// Now we check to see if the line wraps vertically, in which case we
			// split it into two lines before drawing it.
		if (y1 < dstFrameP->frameRect.top)
		{
			if (y2 < dstFrameP->frameRect.top)
				y4 = y2 + dstFrameP->frameRect.bottom;
			else
				y4 = dstFrameP->frameRect.bottom-1;

			y3 = y1 + dstFrameP->frameRect.bottom;
			y1 = dstFrameP->frameRect.top;
		}
		else if (y2 >= dstFrameP->frameRect.bottom)
		{
			if (y1 >= dstFrameP->frameRect.bottom)
				y3 = y1 - dstFrameP->frameRect.bottom;
			else
				y3 = dstFrameP->frameRect.top;

			y4 = y2 - dstFrameP->frameRect.bottom;
			y2 = dstFrameP->frameRect.bottom-1;
		}
		else
		{
			y3 = 0;		// The formula for no second line
			y4 = -1;
		}

			// -----Draw the first line-----
		dy = y2 - y1 + 1;	// dy = number of pixels in line
		if (dy > 0)		// Make sure there is a line first!
		{
			baseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y1] + x1;
			*baseAddr = color;

			while(--dy)
			{
				baseAddr += rowBytes;
				*baseAddr = color;
			}
		}


			// -----Draw the second line-----
		dy = y4 - y3 + 1;	// dy = number of pixels in line
		if (dy > 0)			// Make sure there is a line first!
		{
			baseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y3] + x1;
			*baseAddr = color;

			while(--dy)
			{
				baseAddr += rowBytes;
				*baseAddr = color;
			}
		}
	}
	else if (y1 == y2)	// Straight horizontal line
	{
		register uint32_t 	*longBaseAddr;
		register 				numLongBlits;
		uint32_t			longColor;
		short					x3, x4;		// Second half of wrapped line

			// Fill longColor with color
		longColor = (color<<24) | (color<<16) | (color<<8) | (color);

		if (x2 < x1)
		{
			x1 = dstPoint.h;	// Swap x1 and x2 if they're
			x2 = srcPoint.h;	// in the wrong order.
		}

			// Wrap line vertically if necessary before we start (only done once)
			// Remember y1 and y2 are the same, and y2 isn't used.
		if (y1 < dstFrameP->frameRect.top)
			y1 += dstFrameP->frameRect.bottom;
		else if (y1 >= dstFrameP->frameRect.bottom)
			y1 -= dstFrameP->frameRect.bottom;

			// Now we check to see if the line wraps horizontally, in which case we
			// split it into two lines before drawing it.
		if (x1 < dstFrameP->frameRect.left)
		{
			if (x2 < dstFrameP->frameRect.left)
				x4 = x2 + dstFrameP->frameRect.right;
			else
				x4 = dstFrameP->frameRect.right-1;

			x3 = x1 + dstFrameP->frameRect.right;
			x1 = dstFrameP->frameRect.left;
		}
		else if (x2 >= dstFrameP->frameRect.right)
		{
			if (x1 >= dstFrameP->frameRect.right)
				x3 = x1 - dstFrameP->frameRect.right;
			else
				x3 = dstFrameP->frameRect.left;

			x4 = x2 - dstFrameP->frameRect.right;
			x2 = dstFrameP->frameRect.right-1;
		}
		else
		{
			x3 = 0;		// The formula for no second line
			x4 = -1;
		}

			// ----Draw the first line----
		dx = x2 - x1 + 1;	// dx = number of pixels in line
		if (dx > 0)			// Make sure there is a line first!
		{
			numLongBlits = (dx >> 2);
			longBaseAddr = (uint32_t *)(dstFrameP->frameBaseAddr +
					dstFrameP->scanLinePtrArray[y1] + x1);

			while (numLongBlits--)
			{
				*(longBaseAddr)++ = longColor;
			}

#ifdef __MWERKS__  // Do it nicely
				// Handle extra pixels at end of line
			if (dx & 0x2)
				*((unsigned short *) longBaseAddr)++ = longColor;
			if (dx & 0x1)
				*((unsigned char *) longBaseAddr)++ = color;
#else 	// Make Think C happy
				// Handle extra pixels at end of line
			if (dx & 0x2)
			{
				*((unsigned short *) longBaseAddr) = longColor;
				longBaseAddr = (uint32_t *)(((unsigned short *)longBaseAddr) + 1);
			}
			if (dx & 0x1)
			{
				*((unsigned char *) longBaseAddr) = color;
				longBaseAddr = (uint32_t *)(((unsigned char *)longBaseAddr) + 1);
			}
#endif
		}


			// ----Draw the second line----
		dx = x4 - x3 + 1;	// dx = number of pixels in line
		if (dx > 0)			// Make sure there is a line first!
		{
			numLongBlits = (dx >> 2);
			longBaseAddr = (uint32_t *)(dstFrameP->frameBaseAddr +
					dstFrameP->scanLinePtrArray[y1] + x3);

			while (numLongBlits--)
			{
				*(longBaseAddr)++ = longColor;
			}

#ifdef __MWERKS__  // Do it nicely
				// Handle extra pixels at end of line
			if (dx & 0x2)
				*((unsigned short *) longBaseAddr)++ = longColor;
			if (dx & 0x1)
				*((unsigned char *) longBaseAddr)++ = color;
#else 	// Make Think C happy
				// Handle extra pixels at end of line
			if (dx & 0x2)
			{
				*((unsigned short *) longBaseAddr) = longColor;
				longBaseAddr = (uint32_t *)(((unsigned short *)longBaseAddr) + 1);
			}
			if (dx & 0x1)
			{
				*((unsigned char *) longBaseAddr) = color;
				longBaseAddr = (uint32_t *)(((unsigned char *)longBaseAddr) + 1);
			}
#endif
		}
	}
}

/* BPCopyStraightWrappingLine (mac _BPCopyStraightWrappingLine, mac-only). STATE: see the mac gate. */
// MAC: _BPCopyStraightWrappingLine i386 0x000c80ec ppc 0x000d4224
void BPCopyStraightWrappingLine(
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint)
{
	uint32_t	srcRowBytes = srcFrameP->frameRowBytes;
	uint32_t	dstRowBytes = dstFrameP->frameRowBytes;
	short			x1 = srcPoint.h, y1 = srcPoint.v,
					x2 = dstPoint.h, y2 = dstPoint.v;
	short			dx, dy;

	if (x1 == x2)		// Straight vertical line
	{
		register Ptr	srcBaseAddr, dstBaseAddr;
		short			y3, y4;		// Second half of wrapped line

		if (y2 < y1)
		{
			y1 = dstPoint.v;	// Swap y1 and y2 if they're
			y2 = srcPoint.v;	// in the wrong order.
		}

			// Wrap line horizontally if necessary before we start (only done once)
			// Remember x1 and x2 are the same, and x2 isn't used.
		if (x1 < dstFrameP->frameRect.left)
			x1 += dstFrameP->frameRect.right;
		else if (x1 >= dstFrameP->frameRect.right)
			x1 -= dstFrameP->frameRect.right;

			// Now we check to see if the line wraps vertically, in which case we
			// split it into two lines before drawing it.
		if (y1 < dstFrameP->frameRect.top)
		{
			if (y2 < dstFrameP->frameRect.top)
				y4 = y2 + dstFrameP->frameRect.bottom;
			else
				y4 = dstFrameP->frameRect.bottom-1;

			y3 = y1 + dstFrameP->frameRect.bottom;
			y1 = dstFrameP->frameRect.top;
		}
		else if (y2 >= dstFrameP->frameRect.bottom)
		{
			if (y1 >= dstFrameP->frameRect.bottom)
				y3 = y1 - dstFrameP->frameRect.bottom;
			else
				y3 = dstFrameP->frameRect.top;

			y4 = y2 - dstFrameP->frameRect.bottom;
			y2 = dstFrameP->frameRect.bottom-1;
		}
		else
		{
			y3 = 0;		// The formula for no second line
			y4 = -1;
		}

			// -----Draw the first line-----
		dy = y2 - y1 + 1;	// dy = number of pixels in line
		if (dy > 0)			// Make sure there is a line first!
		{
			srcBaseAddr = srcFrameP->frameBaseAddr + srcFrameP->scanLinePtrArray[y1] + x1;
			dstBaseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y1] + x1;

			*dstBaseAddr = *srcBaseAddr;	// Copy the first pixel

			while (--dy)
			{
				srcBaseAddr += srcRowBytes;
				dstBaseAddr += dstRowBytes;
				*dstBaseAddr = *srcBaseAddr;
			}
		}


			// -----Draw the second line-----
		dy = y4 - y3 + 1;	// dy = number of pixels in line
		if (dy > 0)			// Make sure there is a line first!
		{
			srcBaseAddr = srcFrameP->frameBaseAddr + srcFrameP->scanLinePtrArray[y3] + x1;
			dstBaseAddr = dstFrameP->frameBaseAddr + dstFrameP->scanLinePtrArray[y3] + x1;

			*dstBaseAddr = *srcBaseAddr;	// Copy the first pixel

			while (--dy)
			{
				srcBaseAddr += srcRowBytes;
				dstBaseAddr += dstRowBytes;
				*dstBaseAddr = *srcBaseAddr;
			}
		}
	}
	else if (y1 == y2)	// Straight horizontal line
	{
		register uint32_t 	*srcBaseAddr, *dstBaseAddr;
		register 				numLongBlits;
		short					x3, x4;		// Second half of wrapped line

		if (x2 < x1)
		{
			x1 = dstPoint.h;	// Swap x1 and x2 if they're
			x2 = srcPoint.h;	// in the wrong order.
		}

			// Wrap line vertically if necessary before we start (only done once)
			// Remember y1 and y2 are the same, and y2 isn't used.
		if (y1 < dstFrameP->frameRect.top)
			y1 += dstFrameP->frameRect.bottom;
		else if (y1 >= dstFrameP->frameRect.bottom)
			y1 -= dstFrameP->frameRect.bottom;

			// Now we check to see if the line wraps horizontally, in which case we
			// split it into two lines before drawing it.
		if (x1 < dstFrameP->frameRect.left)
		{
			if (x2 < dstFrameP->frameRect.left)
				x4 = x2 + dstFrameP->frameRect.right;
			else
				x4 = dstFrameP->frameRect.right-1;

			x3 = x1 + dstFrameP->frameRect.right;
			x1 = dstFrameP->frameRect.left;
		}
		else if (x2 >= dstFrameP->frameRect.right)
		{
			if (x1 >= dstFrameP->frameRect.right)
				x3 = x1 - dstFrameP->frameRect.right;
			else
				x3 = dstFrameP->frameRect.left;

			x4 = x2 - dstFrameP->frameRect.right;
			x2 = dstFrameP->frameRect.right-1;
		}
		else
		{
			x3 = 0;		// The formula for no second line
			x4 = -1;
		}

			// ----Draw the first line----
		dx = x2 - x1 + 1;	// dx = number of pixels in line
		if (dx > 0)			// Make sure there is a line first!
		{
			numLongBlits = (dx >> 2);
			srcBaseAddr = (uint32_t *)(srcFrameP->frameBaseAddr +
					srcFrameP->scanLinePtrArray[y1] + x1);
			dstBaseAddr = (uint32_t *)(dstFrameP->frameBaseAddr +
					dstFrameP->scanLinePtrArray[y1] + x1);

			while (numLongBlits--)
			{
				*(dstBaseAddr)++ = *(srcBaseAddr)++;
			}


#ifdef __MWERKS__  // Do it nicely
				// Handle extra pixels at end of line
			if (dx & 0x2)
				*((unsigned short *) dstBaseAddr)++ = *((unsigned short *) srcBaseAddr)++;
			if (dx & 0x1)
				*((unsigned char *) dstBaseAddr)++ = *((unsigned char *) srcBaseAddr)++;
#else 	// Make Think C happy
				// Handle extra pixels at end of line
			if (dx & 0x2)
			{
				*((unsigned short *) dstBaseAddr) = *((unsigned short *) srcBaseAddr);
				dstBaseAddr = (uint32_t *)(((unsigned short *)dstBaseAddr) + 1);
				srcBaseAddr = (uint32_t *)(((unsigned short *)srcBaseAddr) + 1);
			}
			if (dx & 0x1)
			{
				*((unsigned char *) dstBaseAddr) = *((unsigned char *) srcBaseAddr);
				dstBaseAddr = (uint32_t *)((dstBaseAddr) + 1);
				srcBaseAddr = (uint32_t *)(((unsigned char *)srcBaseAddr) + 1);
			}
#endif
		}


			// ----Draw the second line----
		dx = x4 - x3 + 1;	// dx = number of pixels in line
		if (dx > 0)			// Make sure there is a line first!
		{
			numLongBlits = (dx >> 2);
			srcBaseAddr = (uint32_t *)(srcFrameP->frameBaseAddr +
					srcFrameP->scanLinePtrArray[y1] + x3);
			dstBaseAddr = (uint32_t *)(dstFrameP->frameBaseAddr +
					dstFrameP->scanLinePtrArray[y1] + x3);

			while (numLongBlits--)
			{
				*(dstBaseAddr)++ = *(srcBaseAddr)++;
			}


#ifdef __MWERKS__  // Do it nicely
				// Handle extra pixels at end of line
			if (dx & 0x2)
				*((unsigned short *) dstBaseAddr)++ = *((unsigned short *) srcBaseAddr)++;
			if (dx & 0x1)
				*((unsigned char *) dstBaseAddr)++ = *((unsigned char *) srcBaseAddr)++;
#else 	// Make Think C happy
				// Handle extra pixels at end of line
			if (dx & 0x2)
			{
				*((unsigned short *) dstBaseAddr) = *((unsigned short *) srcBaseAddr);
				dstBaseAddr = (uint32_t *)(((unsigned short *)dstBaseAddr) + 1);
				srcBaseAddr = (uint32_t *)(((unsigned short *)srcBaseAddr) + 1);
			}
			if (dx & 0x1)
			{
				*((unsigned char *) dstBaseAddr) = *((unsigned char *) srcBaseAddr);
				dstBaseAddr = (uint32_t *)(((unsigned char *)dstBaseAddr) + 1);
				srcBaseAddr = (uint32_t *)(((unsigned char *)srcBaseAddr) + 1);
			}
#endif
		}
	}
}
