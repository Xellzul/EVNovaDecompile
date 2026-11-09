// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcliplinewithrect.c -- SWClipLineWithRect (win 0x0047b3c0, donor SWFastLine.c line 659). Notes: Research/notes/swcliplinewithrect.md */
#include "macport.h"


/* SWClipLineWithRect (mac _SWClipLineWithRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b3c0
// MAC: _SWClipLineWithRect i386 0x000c65ea ppc 0x000d28d0
#ifdef MACBUILD
#define SIGN(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))
#undef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
Boolean SWClipLineWithRect(
	Point *srcPoint,
	Point *dstPoint,
	Rect *clipRectP)
{
	short		x1 = srcPoint->h, y1 = srcPoint->v,
				x2 = dstPoint->h, y2 = dstPoint->v;
	short		dx, dy, sx, sy, accum;
	short		lastX, lastY;
	Boolean		x1IsInRect, y1IsInRect, x2IsInRect, y2IsInRect;
	Boolean		foundStartPt, endPointIsInRect;
	Rect		clipRect = *clipRectP;

//	InsetRect(&clipRect, 50, 50);	// Useful for testing

	x1IsInRect = (x1 >= clipRect.left && x1 < clipRect.right);
	y1IsInRect = (y1 >= clipRect.top && y1 < clipRect.bottom);
	x2IsInRect = (x2 >= clipRect.left && x2 < clipRect.right);
	y2IsInRect = (y2 >= clipRect.top && y2 < clipRect.bottom);

		// If line needs no clipping, return
	if (x1IsInRect && y1IsInRect && x2IsInRect && y2IsInRect)
	{
		return true;
	}


	dx = x2 - x1;
	dy = y2 - y1;

	sx = SIGN(dx);
	sy = SIGN(dy);

	dx = ABS(dx);
	dy = ABS(dy);

	foundStartPt = false;
	endPointIsInRect = false;

		// The possibilities are: A) Both start and end points are outside the rect.
		// B) Only the start point is outside. C) Only the end point is outside.
		// We know that at least one point is outside, since otherwise this function
		// would have returned already.

		// Check first pixel in line
	if (x1IsInRect && y1IsInRect)
	{
		foundStartPt = true;
		lastX = x1;			// Store last point in line as well, in case only
		lastY = y1;			// one pixel is inside the clipRect.
	}

	if (x2IsInRect && y2IsInRect)
	{
		endPointIsInRect = true;
	}


	if (dx > dy)	// Longer horizontally than vertically
	{
		accum = dx >> 1;	// accum = dx/2;
		while (x1 != x2)
		{
			accum -= dy;
			if (accum < 0)
			{
				accum += dx;
				y1 += sy;
				y1IsInRect = (y1 >= clipRect.top && y1 < clipRect.bottom);
			}
			x1 += sx;
			x1IsInRect = (x1 >= clipRect.left && x1 < clipRect.right);

			if (x1IsInRect && y1IsInRect)	// Determine if current point is in rect
			{
				if (foundStartPt == false)
				{
					srcPoint->h = x1;	// Store first point in line
					srcPoint->v = y1;
					foundStartPt = true;

					if (endPointIsInRect)
						return true; // Leave early, since we only needed to clip the first half
				}

				lastX = x1;	// Store last point in line
				lastY = y1;	// (We keep doing this until we hit the end.)
			}
			else if (foundStartPt == true)
			{
				break;	// the line is leaving clipRect, so break early to save time
			}
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
				x1IsInRect = (x1 >= clipRect.left && x1 < clipRect.right);
			}
			y1 += sy;
			y1IsInRect = (y1 >= clipRect.top && y1 < clipRect.bottom);

			if (x1IsInRect && y1IsInRect)	// Determine if current point is in rect
			{
				if (foundStartPt == false)
				{
					srcPoint->h = x1;	// Store first point in line
					srcPoint->v = y1;
					foundStartPt = true;

					if (endPointIsInRect)
						return true; // Leave early, since we only needed to clip the first half
				}

				lastX = x1;	// Store last point in line
				lastY = y1;	// (We keep doing this until we hit the end.)
			}
			else if (foundStartPt == true)
			{
				break;	// the line is leaving clipRect, so break early to save time
			}
		}
	}

		// Store last point in line
	dstPoint->h = lastX;	// lastX is the pixel just before we ran into the pixel outside bounds
	dstPoint->v = lastY;

		// Tell caller if we ever found any point of the line inside clipRect
	return foundStartPt;
}
#else
Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint, Rect *clipRectP)
{
    short       x1 = srcPoint->h, y1 = srcPoint->v,
                x2 = dstPoint->h, y2 = dstPoint->v;
    short       dx, dy, sx, sy, accum;
    short       lastX, lastY;
    Boolean        x1IsInRect, y1IsInRect, x2IsInRect, y2IsInRect;
    Boolean        foundStartPt, endPointIsInRect;
    Rect        clipRect = *clipRectP;

    x1IsInRect = (x1 >= clipRect.left && x1 < clipRect.right);
    y1IsInRect = (y1 >= clipRect.top && y1 < clipRect.bottom);
    x2IsInRect = (x2 >= clipRect.left && x2 < clipRect.right);
    y2IsInRect = (y2 >= clipRect.top && y2 < clipRect.bottom);

    if (x1IsInRect && y1IsInRect && x2IsInRect && y2IsInRect)
    {
        return 1;
    }

    dx = x2 - x1;
    dy = y2 - y1;

    sx = SIGN(dx);
    sy = SIGN(dy);

    dx = ABS(dx);
    dy = ABS(dy);

    foundStartPt = 0;
    endPointIsInRect = 0;

    if (x1IsInRect && y1IsInRect)
    {
        foundStartPt = 1;
        lastX = x1;
        lastY = y1;
    }

    if (x2IsInRect && y2IsInRect)
    {
        endPointIsInRect = 1;
    }

    if (dx > dy)
    {
        accum = dx >> 1;
        while (x1 != x2)
        {
            accum -= dy;
            if (accum < 0)
            {
                accum += dx;
                y1 += sy;
                y1IsInRect = (y1 >= clipRect.top && y1 < clipRect.bottom);
            }
            x1 += sx;
            x1IsInRect = (x1 >= clipRect.left && x1 < clipRect.right);

            if (x1IsInRect && y1IsInRect)
            {
                if (foundStartPt == 0)
                {
                    srcPoint->h = x1;
                    srcPoint->v = y1;
                    foundStartPt = 1;

                    if (endPointIsInRect)
                        return 1;
                }

                lastX = x1;
                lastY = y1;
            }
            else if (foundStartPt == 1)
            {
                break;
            }
        }
    }
    else
    {
        accum = dy >> 1;
        while (y1 != y2)
        {
            accum -= dx;
            if (accum < 0)
            {
                accum += dy;
                x1 += sx;
                x1IsInRect = (x1 >= clipRect.left && x1 < clipRect.right);
            }
            y1 += sy;
            y1IsInRect = (y1 >= clipRect.top && y1 < clipRect.bottom);

            if (x1IsInRect && y1IsInRect)
            {
                if (foundStartPt == 0)
                {
                    srcPoint->h = x1;
                    srcPoint->v = y1;
                    foundStartPt = 1;

                    if (endPointIsInRect)
                        return 1;
                }

                lastX = x1;
                lastY = y1;
            }
            else if (foundStartPt == 1)
            {
                break;
            }
        }
    }

    dstPoint->h = lastX;
    dstPoint->v = lastY;

    return foundStartPt;
}
#endif
