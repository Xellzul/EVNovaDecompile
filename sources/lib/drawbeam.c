// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* drawbeam.c -- the SWBeams.c run from DrawTranslucentLine to EraseLightningBeam (win 0x00479590-0x0047b399). Notes: Research/notes/drawbeam.md */
#include "macport.h"
#include "spriteworld.h"

/* DrawTranslucentLine (mac _DrawTranslucentLine). STATE: MATCH, byte-identical.
   OPEN: none; notes in Research/notes/drawtranslucentline.md */
// FUNCTION: EVNOVA 0x00479590
void DrawTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                unsigned long color, short srcShade, short dstShade,
                short srcRamp, short dstRamp)
{
    char thereIsALine;
    char longerHoriz;
    short origDstV, origSrcV, origSrcH, origDstH;
    int dh, dv;

    if (dstFrameP == 0)
        return;

    if (dstFrameP == 0 || !dstFrameP->isFrameLocked)
        SWAssertFail("SWBeams.c", 23);

    origSrcV = srcPoint.v;
    origSrcH = srcPoint.h;
    origDstV = dstPoint.v;
    origDstH = dstPoint.h;

    thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &dstFrameP->frameRect);

    if (thereIsALine)
    {
        dv = ABS(srcPoint.v - dstPoint.v);
        dh = ABS(srcPoint.h - dstPoint.h);

        if (dh > dv)
            longerHoriz = 1;
        else
            longerHoriz = 0;

        if (longerHoriz)
        {
            srcRamp -= ABS(srcPoint.h - origSrcH);
            dstRamp -= ABS(dstPoint.h - origDstH);
        }
        else
        {
            srcRamp -= ABS(srcPoint.v - origSrcV);
            dstRamp -= ABS(dstPoint.v - origDstV);
        }

        if (srcShade < 0)
            srcShade = 0;
        if (srcShade > 32)
            srcShade = 32;

        if (dstShade < 0)
            dstShade = 0;
        if (dstShade > 32)
            dstShade = 32;

        if (srcRamp < 0)
            srcRamp = 0;
        if (srcRamp > 32)
            srcRamp = 32;

        if (dstRamp < 0)
            dstRamp = 0;
        if (dstRamp > 32)
            dstRamp = 32;

        BlitTranslucentLine(dstFrameP, srcPoint, dstPoint, color,
                 srcShade, dstShade, srcRamp, dstRamp);
    }
}

/* BlitTranslucentLine (mac _BlitTranslucentLine). STATE: MATCH, byte-identical.
   OPEN: none; notes in Research/notes/blittranslucentline.md */
// FUNCTION: EVNOVA 0x004797b0
void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                         unsigned long color, long srcShade, long dstShade,
                         unsigned long srcRamp, unsigned long dstRamp)
{
    unsigned long   alpha;
    short           x1, y1, x2, y2;
    short           dx, dy, sx, sy;
    short           accum;
    short           curX, curY;
    short           shade;

    x1 = srcPoint.h;
    y1 = srcPoint.v;
    x2 = dstPoint.h;
    y2 = dstPoint.v;

    dx = x2 - x1;
    dy = y2 - y1;

    sx = SIGN(dx);
    sy = SIGN(dy);

    dx = ABS(dx);
    dy = ABS(dy);

    curX = x1;
    curY = y1;

    shade = srcShade;
    alpha = srcShade;
    if (srcRamp != 0)
        alpha = dstShade;

    if (alpha != 0)
        *SW_PIXELPTR16(dstFrameP, curX, curY) =
            Trans16Bit(color, *SW_PIXELPTR16(dstFrameP, curX, curY), alpha);

    if (dx > dy)
    {
        accum = dx >> 1;
        while (x1 != x2)
        {
            accum -= dy;
            if (accum < 0)
            {
                accum += dx;
                curY += sy;
            }
            x1 += sx;
            curX += sx;

            if (ABS(x1 - srcPoint.h) < srcRamp)
            {
                if (alpha < shade)
                    alpha++;
            }
            else if (ABS(x1 - dstPoint.h) < dstRamp)
            {
                if (alpha == 0)
                    return;
                alpha--;
            }
            else if (alpha != shade)
                alpha = shade;

            if (alpha != 0)
                *SW_PIXELPTR16(dstFrameP, curX, curY) =
                    Trans16Bit(color, *SW_PIXELPTR16(dstFrameP, curX, curY), alpha);
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
                curX += sx;
            }
            y1 += sy;
            curY += sy;

            if (ABS(y1 - srcPoint.v) < srcRamp)
            {
                if (alpha < shade)
                    alpha++;
            }
            else if (ABS(y1 - dstPoint.v) < dstRamp)
            {
                if (alpha == 0)
                    return;
                alpha--;
            }
            else if (alpha != shade)
                alpha = shade;

            if (alpha != 0)
                *SW_PIXELPTR16(dstFrameP, curX, curY) =
                    Trans16Bit(color, *SW_PIXELPTR16(dstFrameP, curX, curY), alpha);
        }
    }
}

/* CopyLine (mac _CopyLine). STATE: MATCH, byte-identical.
   OPEN: none; notes in Research/notes/copyline.md */
// FUNCTION: EVNOVA 0x00479c50
void CopyLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint)
{
    char thereIsALine;

    if (srcFrameP == 0 || !srcFrameP->isFrameLocked)
        SWAssertFail("SWBeams.c", 214);
    if (dstFrameP == 0 || !dstFrameP->isFrameLocked)
        SWAssertFail("SWBeams.c", 215);

    if (srcPoint.h == dstPoint.h && srcPoint.v == dstPoint.v)
        return;

    thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &dstFrameP->frameRect);

    if (thereIsALine)
        BlitCopyLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
}

/* BlitCopyLine (mac _BlitCopyLine). STATE: MATCH, byte-identical.
   OPEN: none; notes in Research/notes/blitcopyline.md */
// FUNCTION: EVNOVA 0x00479cf0
void BlitCopyLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint)
{
    short   x1, y1, x2, y2;
    short   dx, dy, sx, sy;
    short   accum;
    short   curX, curY;
    short   DEPTH;

    x1 = srcPoint.h;
    y1 = srcPoint.v;
    x2 = dstPoint.h;
    y2 = dstPoint.v;

    if (x1 == x2 && y1 == y2)
        return;

    DEPTH = 16;

    dx = x2 - x1;
    dy = y2 - y1;

    sx = SIGN(dx);
    sy = SIGN(dy);

    dx = ABS(dx);
    dy = ABS(dy);

    curX = x1;
    curY = y1;

    SET_DEPTH_PIXEL(dstFrameP,x1, y1, GET_DEPTH_PIXEL(srcFrameP,x1, y1))

    if (dx > dy)
    {
        accum = dx >> 1;
        while (x1 != x2)
        {
            accum -= dy;
            if (accum < 0)
            {
                accum += dx;
                curY += sy;
            }
            x1 += sx;
            curX += sx;
            SET_DEPTH_PIXEL(dstFrameP,curX, curY, GET_DEPTH_PIXEL(srcFrameP,curX, curY))
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
                curX += sx;
            }
            y1 += sy;
            curY += sy;
            SET_DEPTH_PIXEL(dstFrameP,curX, curY, GET_DEPTH_PIXEL(srcFrameP,curX, curY))
        }
    }
}

/* DrawBeam (mac _DrawBeam). STATE: MATCH, byte-identical (exe 304 / obj 304); it matches
   only compiled after BlitCopyLine, its exe predecessor (88.5 alone). OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00479fe0
void DrawBeam(FramePtr dstFrameP, Point srcPoint, Point dstPoint, unsigned long color,
              short halfWidth, unsigned long coronaColor, short fadeStep)
{
    Point   ptB;
    Point   ptA;
    int     dh;
    int     dv;
    char    longerHoriz;
    short   i;
    short   shade;
    Rect    clipRect;

    if (srcPoint.h == dstPoint.h && srcPoint.v == dstPoint.v)
        return;

    clipRect.top = dstFrameP->frameRect.top;
    clipRect.left = dstFrameP->frameRect.left;
    clipRect.bottom = dstFrameP->frameRect.bottom;
    clipRect.right = dstFrameP->frameRect.right;

    InsetRect(&clipRect, -32, -32);

    if (srcPoint.h < clipRect.left && dstPoint.h < clipRect.left)
        return;
    if (srcPoint.h > clipRect.right && dstPoint.h > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    dv = srcPoint.v - dstPoint.v;
    if (dv < 0) dv = -dv;
    dh = srcPoint.h - dstPoint.h;
    if (dh < 0) dh = -dh;

    if (dh > dv)
        longerHoriz = 1;
    else
        longerHoriz = 0;

    for (i = 0; i < halfWidth; i++)
    {
        shade = 32 - i * 16;

        if (longerHoriz)
        {
            ptA.h = srcPoint.h;
            ptA.v = srcPoint.v - i;
        }
        else
        {
            ptA.h = srcPoint.h - i;
            ptA.v = srcPoint.v;
        }
        ptB.h = dstPoint.h + ptA.h - srcPoint.h;
        ptB.v = dstPoint.v + ptA.v - srcPoint.v;
        DrawTranslucentLine(dstFrameP, ptA, ptB, color, shade, shade, 0, 20 - i * 10);

        if (i > 0)
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + i;
            }
            else
            {
                ptA.h = srcPoint.h + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            DrawTranslucentLine(dstFrameP, ptA, ptB, color, shade, shade, 0, 20 - i * 10);
        }
    }

    i = 0;
    shade = 16;
    if (fadeStep > 0)
    {
        do
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v - halfWidth - i;
            }
            else
            {
                ptA.h = srcPoint.h - halfWidth - i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            DrawTranslucentLine(dstFrameP, ptA, ptB, coronaColor, shade, 8 - i * 2, i * 2, 8 + i * 6);

            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + halfWidth + i;
            }
            else
            {
                ptA.h = srcPoint.h + halfWidth + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            DrawTranslucentLine(dstFrameP, ptA, ptB, coronaColor, shade, 8 - i * 2, i * 2, 8 + i * 6);

            shade -= fadeStep;
            i++;
        }
        while (shade > 2);
    }
}

/* DrawLightningBeam (mac _DrawLightningBeam). STATE: MATCH 100.0%, exe 577 / obj 577 instrs,
   frame 0x80 exact, byte-identical yes; compiled after DrawBeam, its exe predecessor.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0047a410
void DrawLightningBeam(FramePtr frameA, FramePtr frameB, Point srcPoint, Point dstPoint,
                       unsigned long color, short width, short segPercent, short jitter,
                       short shade)
{
    int     srcV;
    int     dstH;
    int     dstV;
    int     srcH;
    Rect    bounds;
    Rect    clipRect;
    Point   ptB;
    float   stepV;
    float   curV;
    short   srcPtH;
    short   dstPtH;
    int     dH;
    int     dV;
    char    longerHoriz;
    short   segs;
    short   seg;
    short   i;
    short   lastV;
    short   newH;
    short   newV;
    short   h;
    short   shadeStep;
    short   shade2;
    short   lastH;
    short   absV;
    float   curH;
    int     rampEnd;
    Point   ptA;
    float   stepH;
    short   v;

    srcPtH = srcPoint.h;
    dstPtH = dstPoint.h;

    clipRect.top = frameA->frameRect.top;
    clipRect.left = frameA->frameRect.left;
    clipRect.bottom = frameA->frameRect.bottom;
    clipRect.right = frameA->frameRect.right;

    InsetRect(&clipRect, -32, -32);

    if (srcPtH < clipRect.left && dstPtH < clipRect.left)
        return;
    if (srcPtH > clipRect.right && dstPtH > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    dstH = dstPtH;
    srcH = srcPtH;
    dH = srcH - dstH;
    segs = __builtin_abs(dH);

    dstV = dstPoint.v;
    srcV = srcPoint.v;
    dV = srcV - dstV;
    absV = __builtin_abs(dV);

    if (segs > absV)
        segs = (segPercent * segs * 0.01);
    else
        segs = (segPercent * absV * 0.01);

    if (segs < 1)
        segs = 1;

    stepH = (dstH - srcH) / (float)segs;
    stepV = (dstV - srcV) / (float)segs;
    curH = srcPtH;
    curV = srcPoint.v;

    lastH = curH;
    lastV = curV;

    dV = ABS(dV);
    dH = ABS(dH);

    if (dH > dV)
        longerHoriz = 1;
    else
        longerHoriz = 0;

    if (dstPtH > srcPtH)
    {
        bounds.left = srcPtH;
        bounds.right = dstPtH;
    }
    else
    {
        bounds.left = dstPtH;
        bounds.right = srcPtH;
    }

    if (dstPoint.v > srcPoint.v)
    {
        bounds.top = srcPoint.v;
        bounds.bottom = dstPoint.v;
    }
    else
    {
        bounds.top = dstPoint.v;
        bounds.bottom = srcPoint.v;
    }

    if (longerHoriz)
        InsetRect(&bounds, 0, -(jitter + width));
    else
        InsetRect(&bounds, -(jitter + width), 0);

    shadeStep = 32 / (width + 1);
    srcPtH = jitter * 2 + 1;
    for (seg = 0; seg < segs - 1; seg++)
    {
        curH += stepH;
        curV += stepV;

        h = curH;
        v = curV;

        if (seg < segs - 2)
        {
            newH = h + Random() % srcPtH - jitter;
            newV = v + Random() % srcPtH - jitter;
        }
        else
        {
            newV = dstPoint.v;
            newH = dstPtH;
        }

        if (newH < bounds.left)
            newH = bounds.left;
        if (newH > bounds.right)
            newH = bounds.right;
        if (newV < bounds.top)
            newV = bounds.top;
        if (newV > bounds.bottom)
            newV = bounds.bottom;

        i = 0;

        if (width > 0)
        {
            do
            {
                ptA.v = lastV;
                ptA.h = lastH;
                ptB.v = newV;
                ptB.h = newH;

                if (seg == segs - 2)
                    rampEnd = 32;
                else
                    rampEnd = 0;

                shade2 = shade - i * shadeStep;
                if (shade2 < 1)
                    shade2 = 1;

                if (longerHoriz)
                {
                    ptA.v += i;
                    ptB.v += i;
                    DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);

                    if (i > 0)
                    {
                        ptA.v = lastV - i;
                        ptB.v = newV - i;
                        DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                        DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    }
                }
                else
                {
                    ptA.h += i;
                    ptB.h += i;
                    DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);

                    if (i > 0)
                    {
                        ptA.h = lastH - i;
                        ptB.h = newH - i;
                        DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                        DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    }
                }

                i++;
            }
            while (i < width);
        }

        lastV = newV;
        lastH = newH;
    }
}

/* EraseBeam (mac _EraseBeam). STATE: MATCH, byte-identical (exe 243 / obj 243); it matches
   only compiled after DrawLightningBeam, its exe predecessor (1.6 alone). OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0047ac50
void EraseBeam(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint,
               short halfWidth, short fadeStep)
{
    Point   ptB;
    Point   ptA;
    int     dh;
    int     dv;
    char    longerHoriz;
    short   i;
    short   level;
    Rect    clipRect;

    clipRect.top = dstFrameP->frameRect.top;
    clipRect.left = dstFrameP->frameRect.left;
    clipRect.bottom = dstFrameP->frameRect.bottom;
    clipRect.right = dstFrameP->frameRect.right;

    InsetRect(&clipRect, -32, -32);

    if (srcPoint.h < clipRect.left && dstPoint.h < clipRect.left)
        return;
    if (srcPoint.h > clipRect.right && dstPoint.h > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    dv = srcPoint.v - dstPoint.v;
    if (dv < 0) dv = -dv;
    dh = srcPoint.h - dstPoint.h;
    if (dh < 0) dh = -dh;

    if (dh > dv)
        longerHoriz = 1;
    else
        longerHoriz = 0;

    for (i = 0; i <= halfWidth; i++)
    {
        if (longerHoriz)
        {
            ptA.h = srcPoint.h;
            ptA.v = srcPoint.v - i;
        }
        else
        {
            ptA.h = srcPoint.h - i;
            ptA.v = srcPoint.v;
        }
        ptB.h = dstPoint.h + ptA.h - srcPoint.h;
        ptB.v = dstPoint.v + ptA.v - srcPoint.v;
        CopyLine(srcFrameP, dstFrameP, ptA, ptB);

        if (i > 0)
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + i;
            }
            else
            {
                ptA.h = srcPoint.h + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            CopyLine(srcFrameP, dstFrameP, ptA, ptB);
        }
    }

    i = 0;
    level = 16;
    if (fadeStep > 0)
    {
        do
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v - halfWidth - i;
            }
            else
            {
                ptA.h = srcPoint.h - halfWidth - i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            CopyLine(srcFrameP, dstFrameP, ptA, ptB);

            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + halfWidth + i;
            }
            else
            {
                ptA.h = srcPoint.h + halfWidth + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            CopyLine(srcFrameP, dstFrameP, ptA, ptB);

            level -= fadeStep;
            i++;
        }
        while (level > 2);
    }
}

/* EraseLightningBeam (mac _EraseLightningBeam). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047afd0
void EraseLightningBeam(FramePtr srcFrameP, FramePtr dstFrameP,
                        Point srcPoint, Point dstPoint,
                        short halfWidth, short segPercent, short jitter)
{
    short   i;
    Rect    r;
    Rect    clipRect;
    float   stepH, stepV, curH, curV;
    short   dh, dv;
    short   segs;
    short   margin, t;

    clipRect.top = dstFrameP->frameRect.top;
    clipRect.left = dstFrameP->frameRect.left;
    clipRect.bottom = dstFrameP->frameRect.bottom;
    clipRect.right = dstFrameP->frameRect.right;


    InsetRect(&clipRect, -32, -32);

    if (srcPoint.h < clipRect.left && dstPoint.h < clipRect.left)
        return;
    if (srcPoint.h > clipRect.right && dstPoint.h > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    dh = __builtin_abs(srcPoint.h - dstPoint.h);

    dv = __builtin_abs(srcPoint.v - dstPoint.v);

    if (dh > dv)
        segs = (segPercent * dh * 0.01);
    else
        segs = (segPercent * dv * 0.01);

    if (segs < 1)
        segs = 1;

    stepH = (dstPoint.h - srcPoint.h) / (float)segs;
    stepV = (dstPoint.v - srcPoint.v) / (float)segs;
    curH = srcPoint.h;
    curV = srcPoint.v;

    margin = ((halfWidth + jitter) * 2.5);

    t = stepH;
    t = __builtin_abs(t) * 2;
    if (margin < t)
        margin = t;

    t = stepV;
    t = __builtin_abs(t) * 2;
    if (margin < t)
        margin = t;

    for (i = 0; i < segs - 1; i++)
    {
        curH += stepH;
        curV += stepV;

        r.right = curH;
        r.left = r.right;
        r.top = curV;
        r.bottom = r.top;

        InsetRect(&r, -margin, -margin);
        BlitPixieRectDrawProc(srcFrameP, dstFrameP, &r, &r);
    }
}
