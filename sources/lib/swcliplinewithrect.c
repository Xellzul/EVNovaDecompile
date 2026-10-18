// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcliplinewithrect.c -- SWClipLineWithRect (win 0x0047b3c0, donor SWFastLine.c line 659). Notes: Research/notes/swcliplinewithrect.md */
#include "macport.h"


/* SWClipLineWithRect (mac _SWClipLineWithRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b3c0
char SWClipLineWithRect(Point *srcPoint, Point *dstPoint, Rect *clipRectP)
{
    short       x1 = srcPoint->h, y1 = srcPoint->v,
                x2 = dstPoint->h, y2 = dstPoint->v;
    short       dx, dy, sx, sy, accum;
    short       lastX, lastY;
    char        x1IsInRect, y1IsInRect, x2IsInRect, y2IsInRect;
    char        foundStartPt, endPointIsInRect;
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
