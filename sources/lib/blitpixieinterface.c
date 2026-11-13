// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixieinterface.c -- SpriteWorld's BlitPixieInterface.c, tu-map object O12: the sprite draw and collision procs. Notes: Research/notes/blitpixieinterface.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#ifdef MACBUILD
#include "render.h"
#endif

#ifdef MACBUILD
/* SWCopyBitsDrawProc (mac _SWCopyBitsDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyBitsDrawProc i386 0x000b9e10 ppc 0x000c5de0
void SWCopyBitsDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect, Rect *dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = {0, 0, 0, 0};
    Rect dstBlitRect = *dstRect;

    SW_ASSERT(dstFrameP->framePort != NULL, 59);
    SW_ASSERT(dstFrameP->isFrameLocked, 60);
    SW_ASSERT(dstFrameP->frameDepth >= 8, 61);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    CopyBits(GetPortBitMapForCopyBits(srcFrameP->framePort),
             GetPortBitMapForCopyBits(dstFrameP->framePort),
             &srcBlitRect, &dstBlitRect, srcCopy, NULL);
}

/* BlitPixieClearDrawProc (mac _BlitPixieClearDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieClearDrawProc i386 0x000b9fb2 ppc 0x000c5fa4
void BlitPixieClearDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect* srcRect, Rect* dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = {0,0,0,0};
    Rect dstBlitRect = *dstRect;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow,numRowsToCopy;

    uint32_t clearColor;

    SW_ASSERT(dstFrameP->framePort != NULL , 93);
    SW_ASSERT(dstFrameP->isFrameLocked , 94);
    SW_ASSERT(dstFrameP->frameDepth >= 8 , 95);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn) {
        if (((dstBlitRect.top - frameRectP->top) & 1) == dstFrameP->skipOddLines) {
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    clearColor = gSWCurrentSpriteWorld->backgroundValue;

    BlitPixieClear(
        (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy,
        clearColor);
}
#endif

/* BlitPixieRectDrawProc (mac _BlitPixieRectDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004703e0
// MAC: _BlitPixieRectDrawProc i386 0x000ba16f ppc 0x000c6154
void BlitPixieRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 155, 157);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 156, 158);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 157, 159);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 158, 160);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 159, 161);

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
// MAC: _BlitPixieFloatingRectDrawProc i386 0x000ba405 ppc 0x000c63a4
void BlitPixieFloatingRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                   Rect *srcRect, Rect *dstRect, short level,
                                   int32_t backgroundValue)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 222, 224);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 223, 225);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 224, 226);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 225, 227);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 226, 228);

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
// MAC: _BlitPixieMaskDrawProc i386 0x000ba6c5 ppc 0x000c6644
void BlitPixieMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 283, 285);
    if (srcFrameP->maskPort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 284, 286);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 285, 287);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 286, 288);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 287, 289);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 288, 290);

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
// MAC: _BlitPixiePartialMaskDrawProc i386 0x000ba98b ppc 0x000c68c4
void BlitPixiePartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                  Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 355, 357);
    if (srcFrameP->maskPort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 356, 358);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 357, 359);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 358, 360);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 359, 361);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 360, 362);

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

#ifdef MACBUILD
/* BlitPixieMaskCollisionProc (mac _BlitPixieMaskCollisionProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskCollisionProc i386 0x000bac4b ppc 0x000c6b44
Boolean BlitPixieMaskCollisionProc(
    SpritePtr srcSpriteP,
    SpritePtr dstSpriteP,
    Rect* srcRect,
    Rect* dstRect)
{
    uint32_t srcBaseOffset,
                    dstBaseOffset;
    unsigned char* srcPixelPtr;
    unsigned char* dstPixelPtr;
    int numPixelsPerRow;
    int numRows;
    Boolean collision;
    FramePtr srcFrameP = srcSpriteP->curFrameP;
    FramePtr dstFrameP = dstSpriteP->curFrameP;

    SW_ASSERT(dstFrameP->frameDepth >= 8 , 430);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 431);

    SW_ASSERT(srcRect->right > srcRect->left && srcRect->bottom > srcRect->top , 433);
    SW_ASSERT(dstRect->right > dstRect->left && dstRect->bottom > dstRect->top , 434);

    SW_ASSERT(srcRect->right - srcRect->left == dstRect->right - dstRect->left , 436);
    SW_ASSERT(srcRect->bottom - srcRect->top == dstRect->bottom - dstRect->top , 437);

    srcBaseOffset = SW_ROWBYTES(srcFrameP, srcRect->top) + SW_PIXELBYTES(srcFrameP, srcRect->left);

    dstBaseOffset = SW_ROWBYTES(dstFrameP, dstRect->top) + SW_PIXELBYTES(dstFrameP, dstRect->left);

    srcPixelPtr = (unsigned char*) (srcFrameP->maskBaseAddr + srcBaseOffset);
    dstPixelPtr = (unsigned char*) (dstFrameP->maskBaseAddr + dstBaseOffset);

    numPixelsPerRow = (dstRect->right - dstRect->left);
    numRows = (dstRect->bottom - dstRect->top);

    collision = BlitPixieMaskCollision(
                    srcPixelPtr,
                    dstPixelPtr,
                    srcFrameP->frameRowBytes,
                    dstFrameP->frameRowBytes,
                    SW_PIXELBYTES(dstFrameP,numPixelsPerRow),
                    numRows);

    return collision;
}

/* BlitPixieMaskColorDrawProc (mac _BlitPixieMaskColorDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieMaskColorDrawProc i386 0x000bae39 ppc 0x000c6d30
void BlitPixieMaskColorDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow,numRowsToCopy;
    uint32_t color;

    SW_ASSERT(srcFrameP->framePort != NULL , 487);
    SW_ASSERT(srcFrameP->maskPort != NULL , 488);
    SW_ASSERT(dstFrameP->framePort != NULL , 489);
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked , 490);
    SW_ASSERT(dstFrameP->frameDepth >= 8 , 491);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 492);

    if (dstFrameP->frameDepth < 8)
        return;

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn) {
        if (((dstBlitRect.top - frameRectP->top) & 1) == dstFrameP->skipOddLines) {
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

    color = gSWCurrentSpriteBeingDrawn->colorValue;

    BlitPixieMaskColor(
        color,
        (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        (UInt8 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}
#endif

/* The clipped variant gets srcBlitRect converted from pixels to BYTES first, and the
   conversion uses the DESTINATION frame's pixelShift on the SOURCE rect -- that is the
   donor's own text, not a transcription slip. */

/* BlitPixieRLEDrawProc (mac _BlitPixieRLEDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470ee0
// MAC: _BlitPixieRLEDrawProc i386 0x000bb12d ppc 0x000c6fc4
void BlitPixieRLEDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                          Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned char *dstPixel;
    uint32_t dstRowBytes;

    if (srcFrameP->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 565, 567);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 566, 568);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 567, 569);
    if (!(srcFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 568, 570);

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
// MAC: _BlitPixieRLETranslucentDrawProc i386 0x000bb3fc ppc 0x000c7234
void BlitPixieRLETranslucentDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                     Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned char *dstPixel;
    uint32_t dstRowBytes;

    if (srcFrameP->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 640, 642);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 641, 643);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 642, 644);
    if (!(srcFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 643, 645);

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
            if (gSWCurrentSpriteBeingDrawn->translucency <= 0 &&
                gSWCurrentSpriteBeingDrawn->tintMode <= 0)
            {
                BlitPixieRLEClipped(srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes, &srcBlitRect);
            }
            else if (gSWCurrentSpriteBeingDrawn->translucency < 32)
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
            if (gSWCurrentSpriteBeingDrawn->translucency <= 0 &&
                gSWCurrentSpriteBeingDrawn->tintMode <= 0)
            {
                BlitPixieRLE(srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes);
            }
            else if (gSWCurrentSpriteBeingDrawn->translucency < 32)
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
// MAC: _BlitPixieRLECollisionProc i386 0x000bba11 ppc 0x000c7744
Boolean BlitPixieRLECollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *srcRect, Rect *dstRect)
{
    Boolean collision;
    SWFrameRec *srcFrameP = srcSpriteP->curFrameP;
    SWFrameRec *dstFrameP = dstSpriteP->curFrameP;

    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x333, 821);
    if (!(srcFrameP->rleTokenStart != 0 && dstFrameP->rleTokenStart != 0))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x334, 822);

    if (!(srcRect->right - srcRect->left == dstRect->right - dstRect->left))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x336, 824);
    if (!(srcRect->bottom - srcRect->top == dstRect->bottom - dstRect->top))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x337, 825);

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
// MAC: _BlitPixieRLEPointCollisionProc i386 0x000bbb59 ppc 0x000c7888
Boolean BlitPixieRLEPointCollisionProc(SpritePtr sprite, Point *pt, Rect *rect)
{
    Rect r;
    Point p;
    SWFrameRec *fr;

    fr = sprite->curFrameP;
    if (!fr->isFrameLocked)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x356, 856);
    if (fr->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x357, 857);
    r.left = rect->left << fr->pixelShift;
    r.right = rect->right << fr->pixelShift;
    r.top = rect->top;
    r.bottom = rect->bottom;
    p.h = pt->h << fr->pixelShift;
    p.v = pt->v;
    return BlitPixieRLECollisionWithPoint(fr->rleTokenStart, &p, &r);
}

#ifdef MACBUILD
/* BlitPixieRLEColorDrawProc (mac _BlitPixieRLEColorDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieRLEColorDrawProc i386 0x000bbc29 ppc 0x000c795c
void BlitPixieRLEColorDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect* srcRect, Rect* dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    UInt8 *dstPixel;
    uint32_t dstRowBytes;
    uint32_t color;

    SW_ASSERT(srcFrameP->rleTokenStart != NULL , 892);
    SW_ASSERT(dstFrameP->framePort != NULL , 893);
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked , 894);
    SW_ASSERT(srcFrameP->frameDepth >= 8 , 895);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 896);

    srcBlitRect.right -= srcBlitRect.left;
    srcBlitRect.bottom -= srcBlitRect.top;
    srcBlitRect.left = 0;
    srcBlitRect.top = 0;
    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    if (dstBlitRect.left >= dstBlitRect.right)
        return;
    if (dstBlitRect.top >= dstBlitRect.bottom)
        return;

    dstPixel = (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top);
    dstRowBytes = dstFrameP->frameRowBytes;

    color = gSWCurrentSpriteBeingDrawn->colorValue;

    if (srcBlitRect.right - srcBlitRect.left != srcFrameP->frameRect.right - srcFrameP->frameRect.left ||
            srcBlitRect.bottom - srcBlitRect.top != srcFrameP->frameRect.bottom - srcFrameP->frameRect.top) {
        dstPixel -= (srcBlitRect.top * dstRowBytes);
        dstPixel -= SW_PIXELBYTES(dstFrameP, srcBlitRect.left);

        srcBlitRect.left = SW_PIXELBYTES(dstFrameP, srcBlitRect.left);
        srcBlitRect.right = SW_PIXELBYTES(dstFrameP, srcBlitRect.right);

        BlitPixieRLEColorClipped(
             srcFrameP->rleTokenStart,
            color,
            dstPixel,
            dstRowBytes,
            &srcBlitRect);
    } else {
        BlitPixieRLEColor(
             srcFrameP->rleTokenStart,
            color,
            dstPixel,
            dstRowBytes);
    }
}

/* BlitPixieDoubleRectDrawProc (mac _BlitPixieDoubleRectDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieDoubleRectDrawProc i386 0x000bbf00 ppc 0x000c7be4
void BlitPixieDoubleRectDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRectA,
    Rect* dstRectA,
    Rect* srcRectB,
    Rect* dstRectB)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRectA = *srcRectA,
                        dstBlitRectA = *dstRectA,
                        srcBlitRectB = *srcRectB,
                        dstBlitRectB = *dstRectB;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    BlitPixieOffsetInfo info;
    int numPixelsPerRowA,numPixelsPerRowB,numRowsToCopy;

    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked , 969);
    SW_ASSERT(dstFrameP->frameDepth >= 8 , 970);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 971);
    SW_ASSERT(dstRectA->left <= dstRectB->left , 972);

    BP_CLIP_RECT(frameRectP, srcBlitRectA, dstBlitRectA);
    BP_CLIP_RECT(frameRectP, srcBlitRectB, dstBlitRectB);

    numRowsToCopy = dstBlitRectA.bottom - dstBlitRectA.top;

    if (dstFrameP->interlacingIsOn) {
        if (((dstBlitRectA.top - frameRectP->top) & 1) == dstFrameP->skipOddLines) {
            srcBlitRectA.top++;
            dstBlitRectA.top++;
            srcBlitRectB.top++;
            dstBlitRectB.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numRowsToCopy <= 0)
        return;

    numPixelsPerRowA = srcBlitRectA.right - srcBlitRectA.left;
    numPixelsPerRowB = srcBlitRectB.right - srcBlitRectB.left;

    if (numPixelsPerRowA <= 0) {
        srcBlitRectA.right = srcBlitRectA.left;
        dstBlitRectA.right = dstBlitRectA.left;
        numPixelsPerRowA = 0;
    }
    if (numPixelsPerRowB <= 0) {
        srcBlitRectB.right = srcBlitRectB.left;
        dstBlitRectB.right = dstBlitRectB.left;
        numPixelsPerRowB = 0;
    }

    if (numPixelsPerRowA <= 0 && numPixelsPerRowB <= 0)
        return;

    info.srcOffsetAtoB = SW_PIXELBYTES(srcFrameP, srcBlitRectB.left - srcBlitRectA.right);
    info.srcOffsetBtoA = SW_PIXELBYTES(srcFrameP, srcBlitRectA.left - srcBlitRectB.right);
    info.dstOffsetAtoB = SW_PIXELBYTES(dstFrameP, dstBlitRectB.left - dstBlitRectA.right);
    info.dstOffsetBtoA = SW_PIXELBYTES(dstFrameP, dstBlitRectA.left - dstBlitRectB.right);

    info.srcOffsetBtoA += srcRowBytes;
    info.dstOffsetBtoA += dstRowBytes;

    BlitPixieDoubleRects(
        (UInt8 *) SW_PIXELPTR(srcFrameP, srcBlitRectA.left, srcBlitRectA.top),
        (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRectA.left, dstBlitRectA.top),
        SW_PIXELBYTES(srcFrameP, numPixelsPerRowA),
        SW_PIXELBYTES(srcFrameP, numPixelsPerRowB),
        numRowsToCopy,
        &info);
}

/* BlitPixieFlipRectDrawProc (mac _BlitPixieFlipRectDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipRectDrawProc i386 0x000bc424 ppc 0x000c7f24
void BlitPixieFlipRectDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect* srcRect, Rect* dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    SW_ASSERT(srcFrameP->framePort != NULL , 1064);
    SW_ASSERT(srcFrameP->maskPort != NULL , 1065);
    SW_ASSERT(dstFrameP->framePort != NULL , 1066);
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked , 1067);
    SW_ASSERT(dstFrameP->frameDepth >= 8 , 1068);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 1069);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn) {
        if (((dstBlitRect.top - frameRectP->top) & 1) == dstFrameP->skipOddLines) {
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

    if (gSWCurrentSpriteBeingDrawn->flippedVertical) {
        srcBlitRect.top = (srcRect->bottom-1) + (srcRect->top - srcBlitRect.top);

        srcRowBytes = -srcRowBytes;
    }

    if (gSWCurrentSpriteBeingDrawn->flippedHorizontal) {
        srcBlitRect.left = srcRect->left + (srcRect->right - srcBlitRect.right);

        if (dstFrameP->frameDepth == 8) {
            BlitPixieFlip8Bit(
                SW_PIXELPTR8(srcFrameP, srcBlitRect.left, srcBlitRect.top),
                SW_PIXELPTR8(dstFrameP, dstBlitRect.left, dstBlitRect.top),
                srcRowBytes,
                dstRowBytes,
                numPixelsPerRow,
                numRowsToCopy);
        } else if (dstFrameP->frameDepth == 16) {
            BlitPixieFlip16Bit(
                SW_PIXELPTR16(srcFrameP, srcBlitRect.left, srcBlitRect.top),
                SW_PIXELPTR16(dstFrameP, dstBlitRect.left, dstBlitRect.top),
                srcRowBytes,
                dstRowBytes,
                numPixelsPerRow,
                numRowsToCopy);
        } else {
            BlitPixieFlip32Bit(
                SW_PIXELPTR32(srcFrameP, srcBlitRect.left, srcBlitRect.top),
                SW_PIXELPTR32(dstFrameP, dstBlitRect.left, dstBlitRect.top),
                srcRowBytes,
                dstRowBytes,
                numPixelsPerRow,
                numRowsToCopy);
        }
    } else {
        BlitPixieRect(
            (UInt8 *) SW_PIXELPTR(srcFrameP, srcBlitRect.left, srcBlitRect.top),
            (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
            srcRowBytes,
            dstRowBytes,
            SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
            numRowsToCopy);
    }
}

/* BlitPixieFlipMaskDrawProc (mac _BlitPixieFlipMaskDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMaskDrawProc i386 0x000bc8aa ppc 0x000c8374
void BlitPixieFlipMaskDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect* srcRect, Rect* dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow,numRowsToCopy;

    SW_ASSERT(srcFrameP->framePort != NULL , 1178);
    SW_ASSERT(srcFrameP->maskPort != NULL , 1179);
    SW_ASSERT(dstFrameP->framePort != NULL , 1180);
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked , 1181);
    SW_ASSERT(dstFrameP->frameDepth >= 8 , 1182);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 1183);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn) {
        if (((dstBlitRect.top - frameRectP->top) & 1) == dstFrameP->skipOddLines) {
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

    if (gSWCurrentSpriteBeingDrawn->flippedVertical) {
        srcBlitRect.top = (srcRect->bottom-1) + (srcRect->top - srcBlitRect.top);

        srcRowBytes = -srcRowBytes;
    }

    if (gSWCurrentSpriteBeingDrawn->flippedHorizontal) {
        srcBlitRect.left = srcRect->left + (srcRect->right - srcBlitRect.right);

        srcBaseOffset =
            SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
            SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

        if (dstFrameP->frameDepth == 8) {
            BlitPixieFlipMask8Bit(
                (UInt8 *) (srcFrameP->frameBaseAddr + srcBaseOffset),
                SW_PIXELPTR8(dstFrameP, dstBlitRect.left, dstBlitRect.top),
                (UInt8 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
                srcRowBytes,
                dstRowBytes,
                numPixelsPerRow,
                numRowsToCopy);
        } else if (dstFrameP->frameDepth == 16) {
            BlitPixieFlipMask16Bit(
                (UInt16 *) (srcFrameP->frameBaseAddr + srcBaseOffset),
                SW_PIXELPTR16(dstFrameP, dstBlitRect.left, dstBlitRect.top),
                (UInt16 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
                srcRowBytes,
                dstRowBytes,
                numPixelsPerRow,
                numRowsToCopy);
        } else {
            BlitPixieFlipMask32Bit(
                (UInt32 *) (srcFrameP->frameBaseAddr + srcBaseOffset),
                SW_PIXELPTR32(dstFrameP, dstBlitRect.left, dstBlitRect.top),
                (UInt32 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
                srcRowBytes,
                dstRowBytes,
                numPixelsPerRow,
                numRowsToCopy);
        }
    } else {
        srcBaseOffset =
            SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
            SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

        BlitPixieMask(
            (UInt8 *) (srcFrameP->frameBaseAddr + srcBaseOffset),
            (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
            (UInt8 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
            srcRowBytes,
            dstRowBytes,
            numPixelsPerRow,
            numRowsToCopy);
    }
}

/* BlitPixieFlipMaskCollisionProc (mac _BlitPixieFlipMaskCollisionProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMaskCollisionProc i386 0x000bcd33 ppc 0x000c87a4
Boolean BlitPixieFlipMaskCollisionProc(
    SpritePtr srcSpriteP,
    SpritePtr dstSpriteP,
    Rect* srcRect,
    Rect* dstRect)
{
    uint32_t srcBaseOffset,
                    dstBaseOffset;
    unsigned char* srcPixelPtr;
    unsigned char* dstPixelPtr;
    FramePtr srcFrameP = srcSpriteP->curFrameP;
    FramePtr dstFrameP = dstSpriteP->curFrameP;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    short numBytesPerRow;
    short numRows;
    Boolean collision;

    SW_ASSERT(dstFrameP->frameDepth >= 8 , 1309);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 1310);

    SW_ASSERT(srcRect->right > srcRect->left && srcRect->bottom > srcRect->top , 1312);
    SW_ASSERT(dstRect->right > dstRect->left && dstRect->bottom > dstRect->top , 1313);

    SW_ASSERT(srcRect->right - srcRect->left == dstRect->right - dstRect->left , 1315);
    SW_ASSERT(srcRect->bottom - srcRect->top == dstRect->bottom - dstRect->top , 1316);

    srcBaseOffset = SW_ROWBYTES(srcFrameP, srcRect->top) + SW_PIXELBYTES(srcFrameP, srcRect->left);

    dstBaseOffset = SW_ROWBYTES(dstFrameP, dstRect->top) + SW_PIXELBYTES(dstFrameP, dstRect->left);

    srcPixelPtr = (unsigned char*) (srcFrameP->maskBaseAddr + srcBaseOffset);
    dstPixelPtr = (unsigned char*) (dstFrameP->maskBaseAddr + dstBaseOffset);

    numBytesPerRow = SW_PIXELBYTES(dstFrameP, (dstRect->right - dstRect->left));

    numRows = (dstRect->bottom - dstRect->top);

    if (srcSpriteP->flippedVertical) {
        srcPixelPtr += (numRows-1) * srcRowBytes;
        srcRowBytes = -srcRowBytes;
    } else if (dstSpriteP->flippedVertical) {
        dstPixelPtr += (numRows-1) * dstRowBytes;
        dstRowBytes = -dstRowBytes;
    }

    if (srcSpriteP->flippedHorizontal && !dstSpriteP->flippedHorizontal) {
        collision = BlitPixieFlipMaskCollision(
                    dstPixelPtr,
                    srcPixelPtr,
                    dstRowBytes,
                    srcRowBytes,
                    numBytesPerRow,
                    numRows);
    } else if (dstSpriteP->flippedHorizontal && !srcSpriteP->flippedHorizontal) {
        collision = BlitPixieFlipMaskCollision(
                    srcPixelPtr,
                    dstPixelPtr,
                    srcRowBytes,
                    dstRowBytes,
                    numBytesPerRow,
                    numRows);
    } else {
        collision = BlitPixieMaskCollision(
                    srcPixelPtr,
                    dstPixelPtr,
                    srcRowBytes,
                    dstRowBytes,
                    numBytesPerRow,
                    numRows);
    }

    return collision;
}

/* BlitPixieFlipMaskColorDrawProc (mac _BlitPixieFlipMaskColorDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieFlipMaskColorDrawProc i386 0x000bd03b ppc 0x000c8a50
void BlitPixieFlipMaskColorDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    RectPtr frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow,numRowsToCopy;
    uint32_t color;

    SW_ASSERT(srcFrameP->framePort != NULL , 1401);
    SW_ASSERT(srcFrameP->maskPort != NULL , 1402);
    SW_ASSERT(dstFrameP->framePort != NULL , 1403);
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked , 1404);
    SW_ASSERT(dstFrameP->frameDepth >= 8 , 1405);
    SW_ASSERT(srcFrameP->frameDepth == dstFrameP->frameDepth , 1406);

    if (dstFrameP->frameDepth < 8)
        return;

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn) {
        if (((dstBlitRect.top - frameRectP->top) & 1) == dstFrameP->skipOddLines) {
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

    color = gSWCurrentSpriteBeingDrawn->colorValue;

    if (gSWCurrentSpriteBeingDrawn->flippedVertical) {
        srcBlitRect.top = (srcRect->bottom-1) + (srcRect->top - srcBlitRect.top);

        srcRowBytes = -srcRowBytes;
    }

    if (gSWCurrentSpriteBeingDrawn->flippedHorizontal) {
        srcBlitRect.left = srcRect->left + (srcRect->right - srcBlitRect.right);

        srcBaseOffset =
            SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
            SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

        BlitPixieFlipMaskColor(
            color,
            (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
            (UInt8 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
            srcRowBytes,
            dstRowBytes,
            SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
            numRowsToCopy);
    } else {
        srcBaseOffset =
            SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
            SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

        BlitPixieMaskColor(
            color,
            (UInt8 *) SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
            (UInt8 *) (srcFrameP->maskBaseAddr + srcBaseOffset),
            srcRowBytes,
            dstRowBytes,
            SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
            numRowsToCopy);
    }
}

/* BlitPixieCompiledSpriteDrawProc (mac _BlitPixieCompiledSpriteDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieCompiledSpriteDrawProc i386 0x000bd444 ppc 0x000c8dd4
void BlitPixieCompiledSpriteDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect *srcRect,
    Rect *dstRect)
{
    SW_ASSERT(SW_68K, 1505);
}

/* BlitPixieAllBitRectDrawProc (mac _BlitPixieAllBitRectDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieAllBitRectDrawProc i386 0x000bd45b ppc 0x000c8de4
void BlitPixieAllBitRectDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    SW_ASSERT(SW_68K, 1587);
}

/* BlitPixieAllBitMaskDrawProc (mac _BlitPixieAllBitMaskDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieAllBitMaskDrawProc i386 0x000bd472 ppc 0x000c8df4
void BlitPixieAllBitMaskDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    SW_ASSERT(SW_68K, 1698);
}

/* BlitPixieAllBitPartialMaskDrawProc (mac _BlitPixieAllBitPartialMaskDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieAllBitPartialMaskDrawProc i386 0x000bd489 ppc 0x000c8e04
void BlitPixieAllBitPartialMaskDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    SW_ASSERT(SW_68K, 1811);
}
#endif

/* SWGetValueFromColor (mac _SWGetValueFromColor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004719d0
// MAC: _SWGetValueFromColor i386 0x000bd4a0 ppc 0x000c8e14
#ifdef MACBUILD
uint32_t SWGetValueFromColor(FramePtr dstFrameP, RGBColor *color)
{
    GWorldPtr saveGWorld;
    GDHandle saveGDevice;
    UInt32 pixelValue;

    SW_ASSERT(dstFrameP != NULL, 1927);
    SW_ASSERT(dstFrameP->framePort != NULL, 1928);

    GetGWorld(&saveGWorld, &saveGDevice);
    if (useLegacyRenderingPipeline)
        SetGWorld(dstFrameP->framePort, dstFrameP->frameDevice);
    else
        SetGWorld(dstFrameP->framePort, NULL);

    pixelValue = Color2Index(color);

    if (dstFrameP->frameDepth < 16)
        pixelValue |= pixelValue << 8;
    if (dstFrameP->frameDepth < 32)
        pixelValue |= pixelValue << 16;

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveGDevice);
    else
        SetGWorld(saveGWorld, NULL);
    return pixelValue;
}
#else
uint32_t SWGetValueFromColor(SWFrameRec *dstFrameP, RGBColor *color)
{
    CGrafPtr saveGWorld;
    void *saveGDevice;
    uint32_t pixelValue;

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
#endif

#ifdef MACBUILD
/* SWGetColorFromValue (mac _SWGetColorFromValue, mac-only). STATE: see the mac gate. */
// MAC: _SWGetColorFromValue i386 0x000bd5ce ppc 0x000c8f14
void SWGetColorFromValue(FramePtr dstFrameP, RGBColor *color, uint32_t pixelValue)
{
    GWorldPtr saveGWorld;
    GDHandle saveGDevice;

    SW_ASSERT(dstFrameP != NULL, 1965);
    SW_ASSERT(dstFrameP->framePort != NULL, 1966);

    GetGWorld(&saveGWorld, &saveGDevice);
    if (useLegacyRenderingPipeline)
        SetGWorld(dstFrameP->framePort, dstFrameP->frameDevice);
    else
        SetGWorld(dstFrameP->framePort, NULL);

    if (dstFrameP->frameDepth < 32)
        pixelValue &= (1L << dstFrameP->frameDepth) - 1L;

    Index2Color(pixelValue, color);

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveGDevice);
    else
        SetGWorld(saveGWorld, NULL);
}

/* SWSetPixel (mac _SWSetPixel, mac-only). STATE: see the mac gate. */
// MAC: _SWSetPixel i386 0x000bd6cc ppc 0x000c9010
void SWSetPixel(FramePtr dstFrameP, short x, short y, uint32_t color)
{
    SW_ASSERT(dstFrameP->isFrameLocked , 1998);
    SW_ASSERT(x >= dstFrameP->frameRect.left && x < dstFrameP->frameRect.right , 1999);
    SW_ASSERT(y >= dstFrameP->frameRect.top && y < dstFrameP->frameRect.bottom , 2000);

    switch (dstFrameP->frameDepth) {
        case 1 :
            if (color)
                SW_BITSET(dstFrameP->frameBaseAddr + SW_ROWBYTES(dstFrameP, y), x);
            else
                SW_BITCLR(dstFrameP->frameBaseAddr + SW_ROWBYTES(dstFrameP, y), x);
            break;
        case 4 :
            SW_NIBBLESET(dstFrameP->frameBaseAddr + SW_ROWBYTES(dstFrameP, y), x, color);
            break;
        case 8 :
            *SW_PIXELPTR8(dstFrameP, x, y) = color;
            break;
        case 16 :
            *SW_PIXELPTR16(dstFrameP, x, y) = color;
            break;
        case 32 :
            *SW_PIXELPTR32(dstFrameP, x, y) = color;
            break;
        default: {
                RGBColor rgb;
                GWorldPtr saveGWorld;
                GDHandle saveGDevice;

                GetGWorld(&saveGWorld, &saveGDevice);
                if (useLegacyRenderingPipeline)
                    SetGWorld(dstFrameP->framePort, dstFrameP->frameDevice);
                else
                    SetGWorld(dstFrameP->framePort, NULL);
                Index2Color(color, &rgb);
                SetCPixel(x, y, &rgb);
                if (useLegacyRenderingPipeline)
                    SetGWorld(saveGWorld, saveGDevice);
                else
                    SetGWorld(saveGWorld, NULL);
            }
            break;
    }
}

/* SWGetPixel (mac _SWGetPixel, mac-only). STATE: see the mac gate. */
// MAC: _SWGetPixel i386 0x000bd94e ppc 0x000c9340
uint32_t SWGetPixel(FramePtr srcFrameP, short x, short y)
{
    uint32_t val;

    SW_ASSERT(srcFrameP->isFrameLocked , 2061);
    SW_ASSERT(x >= srcFrameP->frameRect.left && x < srcFrameP->frameRect.right , 2062);
    SW_ASSERT(y >= srcFrameP->frameRect.top && y < srcFrameP->frameRect.bottom , 2063);

    switch (srcFrameP->frameDepth) {
        case 1 :
            val = SW_BITTST(srcFrameP->frameBaseAddr + SW_ROWBYTES(srcFrameP, y), x);
        case 4 :
            val = SW_NIBBLEGET(srcFrameP->frameBaseAddr + SW_ROWBYTES(srcFrameP, y), x);
        case 8 :
            val = *SW_PIXELPTR8(srcFrameP, x, y);
        case 16 :
            val = *SW_PIXELPTR16(srcFrameP, x, y);
        case 32 :
            val = *SW_PIXELPTR32(srcFrameP, x, y);
        default: {
                RGBColor rgb;
                GWorldPtr saveGWorld;
                GDHandle saveGDevice;

                GetGWorld(&saveGWorld, &saveGDevice);
                if (useLegacyRenderingPipeline)
                    SetGWorld(srcFrameP->framePort, srcFrameP->frameDevice);
                else
                    SetGWorld(srcFrameP->framePort, NULL);
                GetCPixel(x, y, &rgb);
                val = Color2Index(&rgb);
                if (useLegacyRenderingPipeline)
                    SetGWorld(saveGWorld, saveGDevice);
                else
                    SetGWorld(saveGWorld, NULL);
            }
    }

    return val;
}

/* SWAnimateStarField (mac _SWAnimateStarField, mac-only). STATE: see the mac gate. */
// MAC: _SWAnimateStarField i386 0x000bdac9 ppc 0x000c9474
void SWAnimateStarField(
    SpriteWorldPtr spriteWorldP,
    StarArray *starArray,
    short numStars,
    uint32_t backColor)
{
    short curStar;
    short horizOffset, vertOffset;
    FramePtr windFrameP = spriteWorldP->windowFrameP;
    FramePtr workFrameP = spriteWorldP->workFrameP;
    StarArrayPtr curStarP;
    int x, y;

    SW_ASSERT(windFrameP->isFrameLocked && workFrameP->isFrameLocked, 2123);
    SW_ASSERT(spriteWorldP->pixelDepth >= 8, 2124);

    horizOffset = windFrameP->frameRect.left;
    vertOffset = windFrameP->frameRect.top;

    curStarP = starArray;

    switch (spriteWorldP->pixelDepth) {
        case 8 :
            for (curStar = 0; curStar < numStars; curStar++, curStarP++) {
                if (curStarP->needsToBeErased) {
                    x = curStarP->oldHorizLoc;
                    y = curStarP->oldVertLoc;
                    *SW_PIXELPTR8(windFrameP, x, y) = backColor;
                }

                if (curStarP->isOn) {
                    x = curStarP->horizLoc;
                    y = curStarP->vertLoc;

                    if (*SW_PIXELPTR8(workFrameP, x, y) == backColor) {
                        *SW_PIXELPTR8(windFrameP, x, y) = curStarP->color;

                        curStarP->needsToBeErased = true;
                    } else {
                        curStarP->needsToBeErased = false;
                    }
                } else {
                    curStarP->needsToBeErased = false;
                }
            }
            break;
        case 16:
            for (curStar = 0; curStar < numStars; curStar++, curStarP++) {
                if (curStarP->needsToBeErased) {
                    x = curStarP->oldHorizLoc;
                    y = curStarP->oldVertLoc;
                    *SW_PIXELPTR16(windFrameP, x, y) = backColor;
                }

                if (curStarP->isOn) {
                    x = curStarP->horizLoc;
                    y = curStarP->vertLoc;

                    if (*SW_PIXELPTR16(workFrameP, x, y) == backColor) {
                        *SW_PIXELPTR16(windFrameP, x, y) = curStarP->color;

                        curStarP->needsToBeErased = true;
                    } else {
                        curStarP->needsToBeErased = false;
                    }
                } else {
                    curStarP->needsToBeErased = false;
                }
            }
            break;
        case 32:
            for (curStar = 0; curStar < numStars; curStar++, curStarP++) {
                if (curStarP->needsToBeErased) {
                    x = curStarP->oldHorizLoc;
                    y = curStarP->oldVertLoc;
                    *SW_PIXELPTR32(windFrameP, x, y) = backColor;
                }

                if (curStarP->isOn) {
                    x = curStarP->horizLoc;
                    y = curStarP->vertLoc;

                    if (*SW_PIXELPTR32(workFrameP, x, y) == backColor) {
                        *SW_PIXELPTR32(windFrameP, x, y) = curStarP->color;

                        curStarP->needsToBeErased = true;
                    } else {
                        curStarP->needsToBeErased = false;
                    }
                } else {
                    curStarP->needsToBeErased = false;
                }
            }
            break;
    }
}
#endif
