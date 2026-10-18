// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixieinterface.c -- SpriteWorld's BlitPixieInterface.c, tu-map object O12: the sprite draw and collision procs. Notes: Research/notes/blitpixieinterface.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* BlitPixieRectDrawProc (mac _BlitPixieRectDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004703e0
void BlitPixieRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    long srcRowBytes = srcFrameP->frameRowBytes;
    long dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 155);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 156);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 157);
    if (!(dstFrameP->frameDepth >= 8))
        SWAssertFail("BlitPixieInterface.c", 158);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SWAssertFail("BlitPixieInterface.c", 159);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn)
    {
        if (((dstBlitRect.top - frameRectP->top) & 1) ==
                (unsigned char)dstFrameP->skipOddLines)
        {
            srcBlitRect.top++;
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    BlitPixieRect(
        (unsigned char *)SW_PIXELPTR(srcFrameP, srcBlitRect.left, srcBlitRect.top),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}

/* The seventh argument differs between the two arms: the 8-bit blitter is handed the
   sprite world's backgroundValue, a palette INDEX to blend toward, and the direct-colour
   blitter `32 - level', an alpha weight out of 32. Each arm ignores the other's. */

/* BlitPixieFloatingRectDrawProc (mac _BlitPixieFloatingRectDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470680
void BlitPixieFloatingRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                   Rect *srcRect, Rect *dstRect, short level,
                                   long backgroundValue)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    long srcRowBytes = srcFrameP->frameRowBytes;
    long dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 222);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 223);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 224);
    if (!(dstFrameP->frameDepth >= 8))
        SWAssertFail("BlitPixieInterface.c", 225);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SWAssertFail("BlitPixieInterface.c", 226);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    if (dstFrameP->frameDepth == 8)
    {
        BlitPixieFloatingRect8(
            (unsigned char *)SW_PIXELPTR(srcFrameP, srcBlitRect.left, srcBlitRect.top),
            (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
            srcRowBytes,
            dstRowBytes,
            SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
            numRowsToCopy,
            backgroundValue);
    }
    else
    {
        BlitPixieFloatingRect(
            (unsigned char *)SW_PIXELPTR(srcFrameP, srcBlitRect.left, srcBlitRect.top),
            (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
            srcRowBytes,
            dstRowBytes,
            SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
            numRowsToCopy,
            32 - level);
    }
}

/* BlitPixieMaskDrawProc (mac _BlitPixieMaskDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470960
void BlitPixieMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned long srcRowBytes = srcFrameP->frameRowBytes;
    unsigned long dstRowBytes = dstFrameP->frameRowBytes;
    long srcBaseOffset;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 283);
    if (srcFrameP->maskPort == 0)
        SWAssertFail("BlitPixieInterface.c", 284);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 285);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 286);
    if (!(dstFrameP->frameDepth >= 8))
        SWAssertFail("BlitPixieInterface.c", 287);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SWAssertFail("BlitPixieInterface.c", 288);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn)
    {
        if (((dstBlitRect.top - frameRectP->top) & 1) ==
                (unsigned char)dstFrameP->skipOddLines)
        {
            srcBlitRect.top++;
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    srcBaseOffset = SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
                    SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

    BlitPixieMask(
        (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}

/* BlitPixiePartialMaskDrawProc (mac _BlitPixiePartialMaskDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470c20
void BlitPixiePartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                  Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    long srcRowBytes = srcFrameP->frameRowBytes;
    long dstRowBytes = dstFrameP->frameRowBytes;
    long srcBaseOffset;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 355);
    if (srcFrameP->maskPort == 0)
        SWAssertFail("BlitPixieInterface.c", 356);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 357);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 358);
    if (!(dstFrameP->frameDepth >= 8))
        SWAssertFail("BlitPixieInterface.c", 359);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SWAssertFail("BlitPixieInterface.c", 360);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn)
    {
        if (((dstBlitRect.top - frameRectP->top) & 1) ==
                (unsigned char)dstFrameP->skipOddLines)
        {
            srcBlitRect.top++;
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    srcBaseOffset = SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
                    SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

    BlitPixiePartialMask(
        (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}

/* The clipped variant gets srcBlitRect converted from pixels to BYTES first, and the
   conversion uses the DESTINATION frame's pixelShift on the SOURCE rect -- that is the
   donor's own text, not a transcription slip. */

/* BlitPixieRLEDrawProc (mac _BlitPixieRLEDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470ee0
void BlitPixieRLEDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                          Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned char *dstPixel;
    unsigned long dstRowBytes;

    if (srcFrameP->rleTokenStart == 0)
        SWAssertFail("BlitPixieInterface.c", 565);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 566);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 567);
    if (!(srcFrameP->frameDepth >= 8))
        SWAssertFail("BlitPixieInterface.c", 568);

    if (dstBlitRect.left > frameRectP->right ||
        dstBlitRect.right < frameRectP->left ||
        dstBlitRect.bottom < frameRectP->top ||
        dstBlitRect.top > frameRectP->bottom)
        return;

    srcBlitRect.right -= srcBlitRect.left;
    srcBlitRect.bottom -= srcBlitRect.top;
    srcBlitRect.left = 0;
    srcBlitRect.top = 0;
    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    if (dstBlitRect.left >= dstBlitRect.right)
        return;
    if (dstBlitRect.top >= dstBlitRect.bottom)
        return;

    dstPixel = (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top);
    dstRowBytes = dstFrameP->frameRowBytes;

    if (srcBlitRect.right - srcBlitRect.left !=
                srcFrameP->frameRect.right - srcFrameP->frameRect.left ||
        srcBlitRect.bottom - srcBlitRect.top !=
                srcFrameP->frameRect.bottom - srcFrameP->frameRect.top)
    {
        dstPixel -= (srcBlitRect.top * dstRowBytes);
        dstPixel -= SW_PIXELBYTES(dstFrameP, srcBlitRect.left);

        srcBlitRect.left = SW_PIXELBYTES(dstFrameP, srcBlitRect.left);
        srcBlitRect.right = SW_PIXELBYTES(dstFrameP, srcBlitRect.right);

        BlitPixieRLEClipped(
            srcFrameP->rleTokenStart,
            dstPixel,
            dstRowBytes,
            &srcBlitRect);
    }
    else
    {
        BlitPixieRLE(
            srcFrameP->rleTokenStart,
            dstPixel,
            dstRowBytes);
    }
}

/* Sprite fields, described not recovered: +0xa2 is a translucency out of 64 (compared
   against 0, 32 and 64), +0xa4/+0xa6/+0xa8 three 5-bit tint scales compared against 32,
   +0xaa only tested against 0. An 8-bit sprite with translucency >= 32 and a
   non-neutral tint draws NOTHING; that is what the exe does. */

/* BlitPixieRLETranslucentDrawProc (mac _BlitPixieRLETranslucentDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004711e0
void BlitPixieRLETranslucentDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                     Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned char *dstPixel;
    unsigned long dstRowBytes;

    if (srcFrameP->rleTokenStart == 0)
        SWAssertFail("BlitPixieInterface.c", 640);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 641);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 642);
    if (!(srcFrameP->frameDepth >= 8))
        SWAssertFail("BlitPixieInterface.c", 643);

    if (dstBlitRect.left > frameRectP->right ||
        dstBlitRect.right < frameRectP->left ||
        dstBlitRect.bottom < frameRectP->top ||
        dstBlitRect.top > frameRectP->bottom)
        return;

    srcBlitRect.right -= srcBlitRect.left;
    srcBlitRect.bottom -= srcBlitRect.top;
    srcBlitRect.left = 0;
    srcBlitRect.top = 0;
    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    if (dstBlitRect.left >= dstBlitRect.right)
        return;
    if (dstBlitRect.top >= dstBlitRect.bottom)
        return;

    dstPixel = (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top);
    dstRowBytes = dstFrameP->frameRowBytes;

    if (srcBlitRect.right - srcBlitRect.left !=
                srcFrameP->frameRect.right - srcFrameP->frameRect.left ||
        srcBlitRect.bottom - srcBlitRect.top !=
                srcFrameP->frameRect.bottom - srcFrameP->frameRect.top)
    {
        dstPixel -= (srcBlitRect.top * dstRowBytes);
        dstPixel -= SW_PIXELBYTES(dstFrameP, srcBlitRect.left);

        srcBlitRect.left = SW_PIXELBYTES(dstFrameP, srcBlitRect.left);
        srcBlitRect.right = SW_PIXELBYTES(dstFrameP, srcBlitRect.right);

        if (srcFrameP->frameDepth == 8)
        {
            if (gSWCurrentSpriteBeingDrawn->translucency > 0 ||
                gSWCurrentSpriteBeingDrawn->tintMode > 0)
            {
                if (gSWCurrentSpriteBeingDrawn->translucency < 32)
                    BlitPixieRLETranslucentClipped8(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes, &srcBlitRect);
                else if (gSWCurrentSpriteBeingDrawn->tintRed == 32 &&
                         gSWCurrentSpriteBeingDrawn->tintGreen == 32 &&
                         gSWCurrentSpriteBeingDrawn->tintBlue == 32 &&
                         gSWCurrentSpriteBeingDrawn->translucency == 32 &&
                         gSWCurrentSpriteBeingDrawn->tintMode == 0)
                    BlitPixieRLEAddOverClipped8(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes, &srcBlitRect);
            }
            else
            {
                BlitPixieRLEClipped(srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes, &srcBlitRect);
            }
        }
        else
        {
            if (gSWCurrentSpriteBeingDrawn->translucency > 0 ||
                gSWCurrentSpriteBeingDrawn->tintMode > 0 ||
                gSWCurrentSpriteBeingDrawn->tintRed != 32 ||
                gSWCurrentSpriteBeingDrawn->tintGreen != 32 ||
                gSWCurrentSpriteBeingDrawn->tintBlue != 32)
            {
                if (gSWCurrentSpriteBeingDrawn->tintRed == 32 &&
                    gSWCurrentSpriteBeingDrawn->tintGreen == 32 &&
                    gSWCurrentSpriteBeingDrawn->tintBlue == 32 &&
                    gSWCurrentSpriteBeingDrawn->translucency == 32 &&
                    gSWCurrentSpriteBeingDrawn->tintMode == 0)
                    BlitPixieRLEAddOverClipped(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes, &srcBlitRect);
                else if (gSWCurrentSpriteBeingDrawn->translucency < 64)
                    BlitPixieRLETranslucentClipped(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes, &srcBlitRect);
            }
            else
            {
                BlitPixieRLEClipped(srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes, &srcBlitRect);
            }
        }
    }
    else
    {
        if (srcFrameP->frameDepth == 8)
        {
            if (gSWCurrentSpriteBeingDrawn->translucency > 0 ||
                gSWCurrentSpriteBeingDrawn->tintMode > 0)
            {
                if (gSWCurrentSpriteBeingDrawn->translucency < 32)
                    BlitPixieRLETranslucent8(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes);
                else if (gSWCurrentSpriteBeingDrawn->tintRed == 32 &&
                         gSWCurrentSpriteBeingDrawn->tintGreen == 32 &&
                         gSWCurrentSpriteBeingDrawn->tintBlue == 32 &&
                         gSWCurrentSpriteBeingDrawn->translucency == 32 &&
                         gSWCurrentSpriteBeingDrawn->tintMode == 0)
                    BlitPixieRLEAddOver8(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes);
            }
            else
            {
                BlitPixieRLE(srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes);
            }
        }
        else
        {
            if (gSWCurrentSpriteBeingDrawn->translucency > 0 ||
                gSWCurrentSpriteBeingDrawn->tintMode > 0 ||
                gSWCurrentSpriteBeingDrawn->tintRed != 32 ||
                gSWCurrentSpriteBeingDrawn->tintGreen != 32 ||
                gSWCurrentSpriteBeingDrawn->tintBlue != 32)
            {
                if (gSWCurrentSpriteBeingDrawn->tintRed == 32 &&
                    gSWCurrentSpriteBeingDrawn->tintGreen == 32 &&
                    gSWCurrentSpriteBeingDrawn->tintBlue == 32 &&
                    gSWCurrentSpriteBeingDrawn->translucency == 32 &&
                    gSWCurrentSpriteBeingDrawn->tintMode == 0)
                    BlitPixieRLEAddOver(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes);
                else if (gSWCurrentSpriteBeingDrawn->translucency < 64)
                    BlitPixieRLETranslucent(srcFrameP->rleTokenStart,
                            dstPixel, dstRowBytes);
            }
            else
            {
                BlitPixieRLE(srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes);
            }
        }
    }
}

/* BlitPixieRLECollisionProc (mac _BlitPixieRLECollisionProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471800
char BlitPixieRLECollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *srcRect, Rect *dstRect)
{
    char collision;
    SWFrameRec *srcFrameP = srcSpriteP->curFrameP;
    SWFrameRec *dstFrameP = dstSpriteP->curFrameP;

    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SWAssertFail("BlitPixieInterface.c", 0x333);
    if (!(srcFrameP->rleTokenStart != 0 && dstFrameP->rleTokenStart != 0))
        SWAssertFail("BlitPixieInterface.c", 0x334);

    if (!(srcRect->right - srcRect->left == dstRect->right - dstRect->left))
        SWAssertFail("BlitPixieInterface.c", 0x336);
    if (!(srcRect->bottom - srcRect->top == dstRect->bottom - dstRect->top))
        SWAssertFail("BlitPixieInterface.c", 0x337);

    srcRect->left = SW_PIXELBYTES(srcFrameP, srcRect->left);
    srcRect->right = SW_PIXELBYTES(srcFrameP, srcRect->right);
    dstRect->left = SW_PIXELBYTES(dstFrameP, dstRect->left);
    dstRect->right = SW_PIXELBYTES(dstFrameP, dstRect->right);

    collision = BlitPixieRLECollision(
                    srcFrameP->rleTokenStart,
                    dstFrameP->rleTokenStart,
                    srcRect,
                    dstRect);

    return collision;
}

/* BlitPixieRLEPointCollisionProc (mac _BlitPixieRLEPointCollisionProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00471920
char BlitPixieRLEPointCollisionProc(SpritePtr sprite, Point *pt, Rect *rect)
{
    Rect r;
    Point p;
    SWFrameRec *fr;

    fr = sprite->curFrameP;
    if (!fr->isFrameLocked)
        SWAssertFail("BlitPixieInterface.c", 0x356);
    if (fr->rleTokenStart == 0)
        SWAssertFail("BlitPixieInterface.c", 0x357);
    r.left = rect->left << fr->pixelShift;
    r.right = rect->right << fr->pixelShift;
    r.top = rect->top;
    r.bottom = rect->bottom;
    p.h = pt->h << fr->pixelShift;
    p.v = pt->v;
    return BlitPixieRLECollisionWithPoint(fr->rleTokenStart, &p, &r);
}

/* SWGetValueFromColor (mac _SWGetValueFromColor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004719d0
unsigned long SWGetValueFromColor(SWFrameRec *dstFrameP, RGBColor *color)
{
    CGrafPtr saveGWorld;
    void *saveGDevice;
    unsigned long pixelValue;

    if (dstFrameP == 0)
        SWAssertFail("BlitPixieInterface.c", 1929);
    if (dstFrameP->framePort == 0)
        SWAssertFail("BlitPixieInterface.c", 1930);

    GetPort(&saveGWorld);
    saveGDevice = GetGWorldDevice(saveGWorld);
    SetPort(dstFrameP->framePort);

    pixelValue = Color2Index(color);

    if (dstFrameP->frameDepth < 16)
        pixelValue |= pixelValue << 8;
    if (dstFrameP->frameDepth < 32)
        pixelValue |= pixelValue << 16;

    SetPort(saveGWorld);

    return pixelValue;
}
