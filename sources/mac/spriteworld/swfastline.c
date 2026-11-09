/* swfastline.c -- SWFastLine.o, the line API: SWDrawLine to SWCopyFrameRectInScrollingWorld (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/swutils.h"
#include "../include/swfastline.h"

/* SWDrawLine (mac _SWDrawLine, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawLine i386 0x000c6c10 ppc 0x000d2efc
void SWDrawLine(
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Byte color)
{
	Boolean		thereIsALine = true;

	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 61);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 63);

		// Check to see if the line is straight horizontally or vertically
	if (srcPoint.v == dstPoint.v || srcPoint.h == dstPoint.h)
	{
		BlitStraightLine(dstFrameP, srcPoint, dstPoint, color);
	}
	else
	{
			// Clip the line with the frameRect
		thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &dstFrameP->frameRect);

		if (thereIsALine)
			BlitLine(dstFrameP, srcPoint, dstPoint, color);
	}

}

/* SWCopyLine (mac _SWCopyLine, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyLine i386 0x000c70c7 ppc 0x000d3388
void SWCopyLine(
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint)
{
	Boolean		thereIsALine = true;

	SW_ASSERT(srcFrameP != NULL && srcFrameP->isFrameLocked, 96);
	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 97);
	SW_ASSERT((*GetPortPixMap(srcFrameP->framePort))->pixelSize == 8, 99);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 101);

		// Check to see if the line is straight horizontally or vertically
	if (srcPoint.v == dstPoint.v || srcPoint.h == dstPoint.h)
	{
		BPCopyStraightLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
	}
	else
	{
			// Clip the line with the dstFrameP->frameRect
		thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &dstFrameP->frameRect);

		if (thereIsALine)
			BPCopyLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
	}

}

/* SWDrawFrameRect (mac _SWDrawFrameRect, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawFrameRect i386 0x000c7211 ppc 0x000d34c4
void SWDrawFrameRect(
	FramePtr dstFrameP,
	Rect *dstRectP,
	Byte color)
{
	Point	srcPoint, dstPoint;
	Rect	destRect = *dstRectP;

	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 134);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 136);

		// Swap things around if they're in the wrong order
	if (destRect.right < destRect.left)
	{
		destRect.right = destRect.left;
		destRect.left = dstRectP->right;
	}

	if (destRect.bottom < destRect.top)
	{
		destRect.bottom = destRect.top;
		destRect.top = dstRectP->bottom;
	}

		// Clip destRect with dstFrameP->frameRect
	if (destRect.top < dstFrameP->frameRect.top)
		destRect.top = dstFrameP->frameRect.top;
	if (destRect.bottom > dstFrameP->frameRect.bottom)
		destRect.bottom = dstFrameP->frameRect.bottom;
	if (destRect.left < dstFrameP->frameRect.left)
		destRect.left = dstFrameP->frameRect.left;
	if (destRect.right > dstFrameP->frameRect.right)
		destRect.right = dstFrameP->frameRect.right;

	if (destRect.right <= destRect.left || destRect.bottom <= destRect.top)
		return;


		// Draw top side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.right-1;
	dstPoint.v = destRect.top;
	BlitStraightLine(dstFrameP, srcPoint, dstPoint, color);

		// Draw right side
	srcPoint.h = destRect.right-1;
	dstPoint.v = destRect.bottom-1;
	BlitStraightLine(dstFrameP, srcPoint, dstPoint, color);

		// Draw bottom side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.bottom-1;
	BlitStraightLine(dstFrameP, srcPoint, dstPoint, color);

		// Draw left side
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.left;
	BlitStraightLine(dstFrameP, srcPoint, dstPoint, color);

}

/* SWCopyFrameRect (mac _SWCopyFrameRect, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyFrameRect i386 0x000c73f3 ppc 0x000d3664
void SWCopyFrameRect(
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Rect *dstRectP)
{
	Point	srcPoint, dstPoint;
	Rect	destRect = *dstRectP;

	SW_ASSERT(srcFrameP != NULL && srcFrameP->isFrameLocked, 204);
	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 205);
	SW_ASSERT((*GetPortPixMap(srcFrameP->framePort))->pixelSize == 8, 207);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 209);

		// Swap things around if they're in the wrong order
	if (destRect.right < destRect.left)
	{
		destRect.right = destRect.left;
		destRect.left = dstRectP->right;
	}

	if (destRect.bottom < destRect.top)
	{
		destRect.bottom = destRect.top;
		destRect.top = dstRectP->bottom;
	}

		// Clip destRect with dstFrameP->frameRect
	if (destRect.top < dstFrameP->frameRect.top)
		destRect.top = dstFrameP->frameRect.top;
	if (destRect.bottom > dstFrameP->frameRect.bottom)
		destRect.bottom = dstFrameP->frameRect.bottom;
	if (destRect.left < dstFrameP->frameRect.left)
		destRect.left = dstFrameP->frameRect.left;
	if (destRect.right > dstFrameP->frameRect.right)
		destRect.right = dstFrameP->frameRect.right;

	if (destRect.right <= destRect.left || destRect.bottom <= destRect.top)
		return;


		// Draw top side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.right-1;
	dstPoint.v = destRect.top;
	BPCopyStraightLine(srcFrameP, dstFrameP, srcPoint, dstPoint);

		// Draw right side
	srcPoint.h = destRect.right-1;
	dstPoint.v = destRect.bottom-1;
	BPCopyStraightLine(srcFrameP, dstFrameP, srcPoint, dstPoint);

		// Draw bottom side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.bottom-1;
	BPCopyStraightLine(srcFrameP, dstFrameP, srcPoint, dstPoint);

		// Draw left side
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.left;
	BPCopyStraightLine(srcFrameP, dstFrameP, srcPoint, dstPoint);

}

/* SWDrawLineInScrollingWorld (mac _SWDrawLineInScrollingWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawLineInScrollingWorld i386 0x000c7d3f ppc 0x000d3ef8
void SWDrawLineInScrollingWorld(
	SpriteWorldPtr spriteWorldP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Byte color)
{
	Boolean		thereIsALine;
	Rect		frameRect = dstFrameP->frameRect;

	SW_ASSERT(spriteWorldP != NULL, 279);
	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 280);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 281);

		// Check to see if the line is straight horizontally or vertically
	if (srcPoint.v == dstPoint.v || srcPoint.h == dstPoint.h)
	{
		Rect	visScrollRect = spriteWorldP->visScrollRect;

			// Clip the line with the visScrollRect
		if (srcPoint.v < visScrollRect.top)		srcPoint.v = visScrollRect.top;
		if (dstPoint.v < visScrollRect.top)		dstPoint.v = visScrollRect.top;
		if (srcPoint.v >= visScrollRect.bottom)	srcPoint.v = visScrollRect.bottom-1;
		if (dstPoint.v >= visScrollRect.bottom)	dstPoint.v = visScrollRect.bottom-1;
		if (srcPoint.h < visScrollRect.left)	srcPoint.h = visScrollRect.left;
		if (dstPoint.h < visScrollRect.left)	dstPoint.h = visScrollRect.left;
		if (srcPoint.h >= visScrollRect.right)	srcPoint.h = visScrollRect.right-1;
		if (dstPoint.h >= visScrollRect.right)	dstPoint.h = visScrollRect.right-1;

			// Make the line's points local to the offscreen area
		srcPoint.h -= spriteWorldP->horizScrollRectOffset;
		srcPoint.v -= spriteWorldP->vertScrollRectOffset;
		dstPoint.h -= spriteWorldP->horizScrollRectOffset;
		dstPoint.v -= spriteWorldP->vertScrollRectOffset;

			// Determine whether to call the wrapping or non-wrapping version
		if (srcPoint.h >= frameRect.left && srcPoint.h < frameRect.right &&
			dstPoint.h >= frameRect.left && dstPoint.h < frameRect.right &&
			srcPoint.v >= frameRect.top && srcPoint.v < frameRect.bottom &&
			dstPoint.v >= frameRect.top && dstPoint.v < frameRect.bottom)
		{
				// The line is contained entirely inside the dstFrameP
			BlitStraightLine(dstFrameP, srcPoint, dstPoint, color);
		}
		else
		{
				// The line needs wrapping
			BlitStraightWrappingLine(dstFrameP, srcPoint, dstPoint, color);
		}
	}
	else
	{
			// Clip the line with the visScrollRect
		thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &spriteWorldP->visScrollRect);

			// Make the line's points local to the offscreen area
		srcPoint.h -= spriteWorldP->horizScrollRectOffset;
		srcPoint.v -= spriteWorldP->vertScrollRectOffset;
		dstPoint.h -= spriteWorldP->horizScrollRectOffset;
		dstPoint.v -= spriteWorldP->vertScrollRectOffset;

		if (thereIsALine)
		{
				// Determine whether to call the wrapping or non-wrapping version
			if (srcPoint.h >= frameRect.left && srcPoint.h < frameRect.right &&
				dstPoint.h >= frameRect.left && dstPoint.h < frameRect.right &&
				srcPoint.v >= frameRect.top && srcPoint.v < frameRect.bottom &&
				dstPoint.v >= frameRect.top && dstPoint.v < frameRect.bottom)
			{
					// The line is contained entirely inside the dstFrameP
				BlitLine(dstFrameP, srcPoint, dstPoint, color);
			}
			else
			{
					// The line needs wrapping
				BlitWrappingLine(dstFrameP, srcPoint, dstPoint, color);
			}
		}
	}

}

/* SWCopyLineInScrollingWorld (mac _SWCopyLineInScrollingWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyLineInScrollingWorld i386 0x000c88f6 ppc 0x000d49c8
void SWCopyLineInScrollingWorld(
	SpriteWorldPtr spriteWorldP,
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Point srcPoint,
	Point dstPoint,
	Rect *clipRectP)
{
	Boolean		thereIsALine;
	Rect		frameRect = dstFrameP->frameRect;
	short		vertScrollRectOffset, horizScrollRectOffset;

	SW_ASSERT(spriteWorldP != NULL, 370);
	SW_ASSERT(srcFrameP != NULL && srcFrameP->isFrameLocked, 371);
	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 372);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 373);

		// ClipRectP should be either the visScrollRect or oldVisScrollRect
	vertScrollRectOffset = spriteWorldP->backRect.bottom *
		(clipRectP->top / spriteWorldP->backRect.bottom);

	horizScrollRectOffset = spriteWorldP->backRect.right *
		(clipRectP->left / spriteWorldP->backRect.right);

		// Check to see if the line is straight horizontally or vertically
	if (srcPoint.v == dstPoint.v || srcPoint.h == dstPoint.h)
	{
			// Clip the line with the visScrollRect
		if (srcPoint.v < clipRectP->top)		srcPoint.v = clipRectP->top;
		if (dstPoint.v < clipRectP->top)		dstPoint.v = clipRectP->top;
		if (srcPoint.v >= clipRectP->bottom)	srcPoint.v = clipRectP->bottom-1;
		if (dstPoint.v >= clipRectP->bottom)	dstPoint.v = clipRectP->bottom-1;
		if (srcPoint.h < clipRectP->left)		srcPoint.h = clipRectP->left;
		if (dstPoint.h < clipRectP->left)		dstPoint.h = clipRectP->left;
		if (srcPoint.h >= clipRectP->right)		srcPoint.h = clipRectP->right-1;
		if (dstPoint.h >= clipRectP->right)		dstPoint.h = clipRectP->right-1;

			// Make the line's points local to the offscreen area
		srcPoint.h -= horizScrollRectOffset;
		srcPoint.v -= vertScrollRectOffset;
		dstPoint.h -= horizScrollRectOffset;
		dstPoint.v -= vertScrollRectOffset;

			// Determine whether to call the wrapping or non-wrapping version
		if (srcPoint.h >= frameRect.left && srcPoint.h < frameRect.right &&
			dstPoint.h >= frameRect.left && dstPoint.h < frameRect.right &&
			srcPoint.v >= frameRect.top && srcPoint.v < frameRect.bottom &&
			dstPoint.v >= frameRect.top && dstPoint.v < frameRect.bottom)
		{
				// The line is contained entirely inside the dstFrameP
			BPCopyStraightLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
		}
		else
		{
				// The line needs wrapping
			BPCopyStraightWrappingLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
		}
	}
	else
	{
			// Clip the line with clipRectP (visScrollRect or oldVisScrollRect)
		thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, clipRectP);

			// Make the line's points local to the offscreen area
		srcPoint.h -= horizScrollRectOffset;
		srcPoint.v -= vertScrollRectOffset;
		dstPoint.h -= horizScrollRectOffset;
		dstPoint.v -= vertScrollRectOffset;

		if (thereIsALine)
		{
				// Determine whether to call the wrapping or non-wrapping version
			if (srcPoint.h >= frameRect.left && srcPoint.h < frameRect.right &&
				dstPoint.h >= frameRect.left && dstPoint.h < frameRect.right &&
				srcPoint.v >= frameRect.top && srcPoint.v < frameRect.bottom &&
				dstPoint.v >= frameRect.top && dstPoint.v < frameRect.bottom)
			{
					// The line is contained entirely inside the frameRect
				BPCopyLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
			}
			else
			{
					// The line needs wrapping
				BPCopyWrappingLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
			}
		}
	}

}

/* SWDrawFrameRectInScrollingWorld (mac _SWDrawFrameRectInScrollingWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawFrameRectInScrollingWorld i386 0x000c8cb9 ppc 0x000d4d44
void SWDrawFrameRectInScrollingWorld(
	SpriteWorldPtr spriteWorldP,
	FramePtr dstFrameP,
	Rect *dstRectP,
	Byte color)
{
	Point			srcPoint, dstPoint;
	Rect			destRect = *dstRectP;
	Rect			frameRect = dstFrameP->frameRect;
	Boolean			leftClipped, topClipped, rightClipped, bottomClipped;
	BlitLineProcPtr	lineProcPtr;

	SW_ASSERT(spriteWorldP != NULL, 468);
	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 469);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 470);

	leftClipped = topClipped = rightClipped = bottomClipped = false;

		// Clip destRect with spriteWorldP->visScrollRect
	if (destRect.top < spriteWorldP->visScrollRect.top)
	{
		destRect.top = spriteWorldP->visScrollRect.top;
		topClipped = true;
	}
	if (destRect.bottom > spriteWorldP->visScrollRect.bottom)
	{
		destRect.bottom = spriteWorldP->visScrollRect.bottom;
		bottomClipped = true;
	}
	if (destRect.left < spriteWorldP->visScrollRect.left)
	{
		destRect.left = spriteWorldP->visScrollRect.left;
		leftClipped = true;
	}
	if (destRect.right > spriteWorldP->visScrollRect.right)
	{
		destRect.right = spriteWorldP->visScrollRect.right;
		rightClipped = true;
	}

	if (destRect.right <= destRect.left || destRect.bottom <= destRect.top)
		return;


		// Make the line's points local to the offscreen area
	destRect.left -= spriteWorldP->horizScrollRectOffset;
	destRect.right -= spriteWorldP->horizScrollRectOffset;
	destRect.top -= spriteWorldP->vertScrollRectOffset;
	destRect.bottom -= spriteWorldP->vertScrollRectOffset;

		// Determine whether to call the wrapping or non-wrapping version
	if (destRect.left >= frameRect.left && destRect.left < frameRect.right &&
		destRect.right >= frameRect.left && destRect.right < frameRect.right &&
		destRect.top >= frameRect.top && destRect.top < frameRect.bottom &&
		destRect.bottom >= frameRect.top && destRect.bottom < frameRect.bottom)
	{
		lineProcPtr = BlitLine;	 // The line is contained entirely inside the frameRect
	}
	else
	{
		lineProcPtr = BlitWrappingLine;	// The line needs wrapping
	}


		// Draw top side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.right-1;
	dstPoint.v = destRect.top;
	if (!topClipped)
		(*lineProcPtr)(dstFrameP, srcPoint, dstPoint, color);

		// Draw right side
	srcPoint.h = destRect.right-1;
	dstPoint.v = destRect.bottom-1;
	if (!rightClipped)
		(*lineProcPtr)(dstFrameP, srcPoint, dstPoint, color);

		// Draw bottom side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.bottom-1;
	if (!bottomClipped)
		(*lineProcPtr)(dstFrameP, srcPoint, dstPoint, color);

		// Draw left side
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.left;
	if (!leftClipped)
		(*lineProcPtr)(dstFrameP, srcPoint, dstPoint, color);

}

/* SWCopyFrameRectInScrollingWorld (mac _SWCopyFrameRectInScrollingWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyFrameRectInScrollingWorld i386 0x000c8f87 ppc 0x000d4fe4
void SWCopyFrameRectInScrollingWorld(
	SpriteWorldPtr spriteWorldP,
	FramePtr srcFrameP,
	FramePtr dstFrameP,
	Rect *dstRectP,
	Rect *clipRectP)
{
	Point			srcPoint, dstPoint;
	Rect			destRect = *dstRectP;
	Rect			frameRect = dstFrameP->frameRect;
	Boolean			leftClipped, topClipped, rightClipped, bottomClipped;
	short			horizScrollRectOffset, vertScrollRectOffset;
	CopyLineProcPtr	lineProcPtr;

	SW_ASSERT(spriteWorldP != NULL, 570);
	SW_ASSERT(srcFrameP != NULL && srcFrameP->isFrameLocked, 571);
	SW_ASSERT(dstFrameP != NULL && dstFrameP->isFrameLocked, 572);
	SW_ASSERT((*GetPortPixMap(srcFrameP->framePort))->pixelSize == 8, 573);
	SW_ASSERT((*GetPortPixMap(dstFrameP->framePort))->pixelSize == 8, 574);

		// ClipRectP should be either the visScrollRect or oldVisScrollRect
	vertScrollRectOffset = spriteWorldP->backRect.bottom *
		(clipRectP->top / spriteWorldP->backRect.bottom);

	horizScrollRectOffset = spriteWorldP->backRect.right *
		(clipRectP->left / spriteWorldP->backRect.right);

	leftClipped = topClipped = rightClipped = bottomClipped = false;

		// Clip destRect with spriteWorldP->visScrollRect
	if (destRect.top < clipRectP->top)
	{
		destRect.top = clipRectP->top;
		topClipped = true;
	}
	if (destRect.bottom > clipRectP->bottom)
	{
		destRect.bottom = clipRectP->bottom;
		bottomClipped = true;
	}
	if (destRect.left < clipRectP->left)
	{
		destRect.left = clipRectP->left;
		leftClipped = true;
	}
	if (destRect.right > clipRectP->right)
	{
		destRect.right = clipRectP->right;
		rightClipped = true;
	}

	if (destRect.right <= destRect.left || destRect.bottom <= destRect.top)
		return;

		// Make the line's points local to the offscreen area
	destRect.left -= horizScrollRectOffset;
	destRect.right -= horizScrollRectOffset;
	destRect.top -= vertScrollRectOffset;
	destRect.bottom -= vertScrollRectOffset;

		// Determine whether to call the wrapping or non-wrapping version
	if (destRect.left >= frameRect.left && destRect.left < frameRect.right &&
		destRect.right >= frameRect.left && destRect.right < frameRect.right &&
		destRect.top >= frameRect.top && destRect.top < frameRect.bottom &&
		destRect.bottom >= frameRect.top && destRect.bottom < frameRect.bottom)
	{
		lineProcPtr = BPCopyStraightLine; // The line is contained entirely inside the frameRect
	}
	else
	{
		lineProcPtr = BPCopyStraightWrappingLine;	// The line needs wrapping
	}


		// Draw top side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.right-1;
	dstPoint.v = destRect.top;
	if (!topClipped)		// Draw top if it wasn't clipped
		(*lineProcPtr)(srcFrameP, dstFrameP, srcPoint, dstPoint);

		// Draw right side
	srcPoint.h = destRect.right-1;
	dstPoint.v = destRect.bottom-1;
	if (!rightClipped)	// Draw right if it wasn't clipped
		(*lineProcPtr)(srcFrameP, dstFrameP, srcPoint, dstPoint);

		// Draw bottom side
	srcPoint.h = destRect.left;
	srcPoint.v = destRect.bottom-1;
	if (!bottomClipped)	// Draw bottom if it wasn't clipped
		(*lineProcPtr)(srcFrameP, dstFrameP, srcPoint, dstPoint);

		// Draw left side
	srcPoint.v = destRect.top;
	dstPoint.h = destRect.left;
	if (!leftClipped)		// Draw left if it wasn't clipped
		(*lineProcPtr)(srcFrameP, dstFrameP, srcPoint, dstPoint);

}
