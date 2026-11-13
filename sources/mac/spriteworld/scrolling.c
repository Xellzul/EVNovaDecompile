/* scrolling.c -- SpriteWorld 2.3.1's Scrolling.c as the mac 1.1.1 fork built it (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"
#include "render.h"

/* SWUpdateScrollingWindow (mac _SWUpdateScrollingWindow, mac-only). STATE: see the mac gate. */
// MAC: _SWUpdateScrollingWindow i386 0x000b3db7 ppc 0x000bf1b8
void SWUpdateScrollingWindow(
    SpriteWorldPtr spriteWorldP)
{
    GWorldPtr       holdGWorld;
    GDHandle        holdGDH;

    SW_ASSERT(spriteWorldP != NULL, 56);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 57);
    SW_ASSERT(spriteWorldP->windowFrameP->isFrameLocked, 58);

    GetGWorld( &holdGWorld, &holdGDH );

    SetGWorld(spriteWorldP->windowFrameP->framePort, nil);

    gSWCurrentSpriteWorld = spriteWorldP;

    SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->windowFrameP, &spriteWorldP->windRect);

    gSWCurrentSpriteWorld = NULL;

    if (useLegacyRenderingPipeline)
        SetGWorld( holdGWorld, holdGDH );
    else
        SetGWorld( holdGWorld, NULL );
}

/* SWUpdateScrollingSpriteWorld (mac _SWUpdateScrollingSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWUpdateScrollingSpriteWorld i386 0x000b4092 ppc 0x000bf560
void SWUpdateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP,
    Boolean updateWindow)
{
    UpdateRectStructPtr         curRectStructP,
                                nextRectStructP;
    register SpriteLayerPtr     curSpriteLayerP;
    register SpritePtr          curSpriteP;
    short                       curTileLayer;
    Rect                        *visScrollRectP = &spriteWorldP->visScrollRect;
    Rect                        srcFrameRect, tempDstRect;
    GWorldPtr                   saveGWorld;
    GDHandle                    saveGDH;

    SW_ASSERT(spriteWorldP != NULL, 98);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 99);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 100);
    SW_ASSERT(spriteWorldP->windowFrameP->isFrameLocked, 101);

    GetGWorld( &saveGWorld, &saveGDH );

    gSWCurrentSpriteWorld = spriteWorldP;

    SWSetPortToFrame(spriteWorldP->workFrameP);
    (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                        spriteWorldP->workFrameP,
                        &spriteWorldP->backFrameP->frameRect,
                        &spriteWorldP->workFrameP->frameRect);

    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != NULL)
        {
            SW_ASSERT(curSpriteP->curFrameP->isFrameLocked, 135);
            curSpriteP->tileDepth = curTileLayer;

            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect = curSpriteP->destFrameRect;
                srcFrameRect = curSpriteP->curFrameP->frameRect;

                if (curSpriteP->destOffscreenRect.top < visScrollRectP->top)
                {
                    srcFrameRect.top += visScrollRectP->top -
                            curSpriteP->destOffscreenRect.top;
                    curSpriteP->destOffscreenRect.top =  visScrollRectP->top;
                }

                if (curSpriteP->destOffscreenRect.bottom > visScrollRectP->bottom)
                {
                    srcFrameRect.bottom += visScrollRectP->bottom -
                            curSpriteP->destOffscreenRect.bottom;
                    curSpriteP->destOffscreenRect.bottom = visScrollRectP->bottom;
                }

                if (curSpriteP->destOffscreenRect.left < visScrollRectP->left)
                {
                    srcFrameRect.left += visScrollRectP->left -
                            curSpriteP->destOffscreenRect.left;
                    curSpriteP->destOffscreenRect.left = visScrollRectP->left;
                }

                if (curSpriteP->destOffscreenRect.right > visScrollRectP->right)
                {
                    srcFrameRect.right += visScrollRectP->right -
                            curSpriteP->destOffscreenRect.right;
                    curSpriteP->destOffscreenRect.right = visScrollRectP->right;
                }

                curSpriteP->destRectIsVisible =
                        ( (curSpriteP->destOffscreenRect.right >
                        curSpriteP->destOffscreenRect.left) &&
                        (curSpriteP->destOffscreenRect.bottom >
                        curSpriteP->destOffscreenRect.top) );

                if (curSpriteP->destRectIsVisible)
                {
                    curSpriteP->destOffscreenRect.top -= spriteWorldP->vertScrollRectOffset;
                    curSpriteP->destOffscreenRect.bottom -= spriteWorldP->vertScrollRectOffset;
                    curSpriteP->destOffscreenRect.left -= spriteWorldP->horizScrollRectOffset;
                    curSpriteP->destOffscreenRect.right -= spriteWorldP->horizScrollRectOffset;

                    gSWCurrentSpriteBeingDrawn = curSpriteP;

                    SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                        &srcFrameRect, &curSpriteP->destOffscreenRect);

                    gSWCurrentSpriteBeingDrawn = NULL;

                    if (spriteWorldP->tilingIsOn &&
                        curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                    {
                        tempDstRect = curSpriteP->destOffscreenRect;
                        tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                        tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                        tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                        tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                        SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                    }
                }
            }

            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
            curSpriteP->oldRectIsVisible = curSpriteP->destRectIsVisible;

            curSpriteP->needsToBeDrawn = false;
            curSpriteP->needsToBeErased = false;

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;

    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    if (updateWindow)
    {
        SWSetPortToFrame(spriteWorldP->windowFrameP);

        SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->windowFrameP, &spriteWorldP->windRect);
    }

    nextRectStructP = spriteWorldP->headUpdateRectP;
    while ( nextRectStructP != NULL )
    {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        DisposePtr( (Ptr)curRectStructP );
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

    gSWCurrentSpriteWorld = NULL;

    if (useLegacyRenderingPipeline)
        SetGWorld( saveGWorld, saveGDH );
    else
        SetGWorld( saveGWorld, NULL );
}

/* SWProcessScrollingSpriteWorld (mac _SWProcessScrollingSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWProcessScrollingSpriteWorld i386 0x000b6915 ppc 0x000c2474
void SWProcessScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP)
{
    SWProcessSpriteWorld(spriteWorldP);
    if ( !spriteWorldP->frameHasOccurred )
    {
        return;
    }

    gSWCurrentSpriteWorld = spriteWorldP;

    if (spriteWorldP->worldMoveProc != NULL)
    {
        (*spriteWorldP->worldMoveProc)(spriteWorldP, spriteWorldP->followSpriteP);
    }

    if (spriteWorldP->horizScrollDelta || spriteWorldP->vertScrollDelta)
    {
        SWOffsetVisScrollRect(spriteWorldP,
            spriteWorldP->horizScrollDelta,
            spriteWorldP->vertScrollDelta);
    }

    SWProcessNonScrollingLayers(spriteWorldP);

    gSWCurrentSpriteWorld = NULL;
}

/* SWAnimateScrollingSpriteWorld (mac _SWAnimateScrollingSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWAnimateScrollingSpriteWorld i386 0x000b4bad ppc 0x000c03b4
void SWAnimateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP)
{
    UpdateRectStructPtr         curRectStructP,
                                nextRectStructP;
    register SpriteLayerPtr     curSpriteLayerP;
    register SpritePtr          curSpriteP;
    SpritePtr                   headActiveSpriteP = NULL;
    SpritePtr                   headIdleSpriteP = NULL;
    SpritePtr                   curActiveSpriteP = NULL;
    SpritePtr                   curIdleSpriteP = NULL;
    Rect                        *visScrollRectP = &spriteWorldP->visScrollRect;
    Rect                        tempDstRect, tempSrcRect;
    short                       hScrollDelta, vScrollDelta, curTileLayer;
    short                       oldVertScrollRectOffset, oldHorizScrollRectOffset;

    SW_ASSERT(spriteWorldP != NULL, 322);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 323);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 324);
    SW_ASSERT(spriteWorldP->windowFrameP->isFrameLocked, 325);

    if (!spriteWorldP->frameHasOccurred)
        return;

    gSWCurrentSpriteWorld = spriteWorldP;

    oldVertScrollRectOffset = spriteWorldP->backRect.bottom *
        (spriteWorldP->oldVisScrollRect.top / spriteWorldP->backRect.bottom);

    oldHorizScrollRectOffset = spriteWorldP->backRect.right *
        (spriteWorldP->oldVisScrollRect.left / spriteWorldP->backRect.right);

    hScrollDelta = visScrollRectP->left - spriteWorldP->oldVisScrollRect.left;
    vScrollDelta = visScrollRectP->top - spriteWorldP->oldVisScrollRect.top;

    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWAddSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }

    if (spriteWorldP->tilingIsOn)
    {
        if (hScrollDelta)
        {
            tempDstRect = *visScrollRectP;

            if (hScrollDelta < 0)
            {
                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.left)
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.left;
            }
            else
            {
                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.right)
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.right;
            }

            (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &tempDstRect, true);
            SWWrapRectToWorkArea(spriteWorldP, &tempDstRect);

            if (vScrollDelta)
            {
                tempDstRect = spriteWorldP->visScrollRect;

                if (vScrollDelta < 0)
                {
                    if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.top)
                        tempDstRect.bottom = spriteWorldP->oldVisScrollRect.top;
                }
                else
                {
                    if (tempDstRect.top < spriteWorldP->oldVisScrollRect.bottom)
                        tempDstRect.top = spriteWorldP->oldVisScrollRect.bottom;
                }

                if (hScrollDelta < 0)
                {
                    if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                        tempDstRect.left = spriteWorldP->oldVisScrollRect.left;
                }
                else
                {
                    if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                        tempDstRect.right = spriteWorldP->oldVisScrollRect.right;
                }

                if (tempDstRect.right > tempDstRect.left)
                {
                    (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &tempDstRect, false);
                    SWWrapRectToWorkArea(spriteWorldP, &tempDstRect);
                }
            }
        }
        else if (vScrollDelta)
        {
            tempDstRect = *visScrollRectP;

            if (vScrollDelta < 0)
            {
                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.top)
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.top;
            }
            else
            {
                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.bottom)
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.bottom;
            }

            (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &tempDstRect, true);
            SWWrapRectToWorkArea(spriteWorldP, &tempDstRect);
        }
    }

    SetGWorld(spriteWorldP->workFrameP->framePort, nil);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != NULL)
        {
            SW_ASSERT(curSpriteP->curFrameP->isFrameLocked, 455);
            curSpriteP->tileDepth = curTileLayer;

            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect = curSpriteP->destFrameRect;
                curSpriteP->clippedSourceRect = curSpriteP->curFrameP->frameRect;

                if (curSpriteP->destOffscreenRect.top < visScrollRectP->top)
                {
                    curSpriteP->clippedSourceRect.top += visScrollRectP->top -
                            curSpriteP->destOffscreenRect.top;
                    curSpriteP->destOffscreenRect.top = visScrollRectP->top;
                }

                if (curSpriteP->destOffscreenRect.bottom > visScrollRectP->bottom)
                {
                    curSpriteP->clippedSourceRect.bottom += visScrollRectP->bottom -
                            curSpriteP->destOffscreenRect.bottom;
                    curSpriteP->destOffscreenRect.bottom = visScrollRectP->bottom;
                }

                if (curSpriteP->destOffscreenRect.left < visScrollRectP->left)
                {
                    curSpriteP->clippedSourceRect.left += visScrollRectP->left -
                            curSpriteP->destOffscreenRect.left;
                    curSpriteP->destOffscreenRect.left = visScrollRectP->left;
                }

                if (curSpriteP->destOffscreenRect.right > visScrollRectP->right)
                {
                    curSpriteP->clippedSourceRect.right += visScrollRectP->right -
                            curSpriteP->destOffscreenRect.right;
                    curSpriteP->destOffscreenRect.right = visScrollRectP->right;
                }

                curSpriteP->destRectIsVisible =
                        (curSpriteP->destOffscreenRect.right >
                        curSpriteP->destOffscreenRect.left &&
                        curSpriteP->destOffscreenRect.bottom >
                        curSpriteP->destOffscreenRect.top);
            }

            if (curSpriteP->needsToBeDrawn && curSpriteP->isVisible ||
                curSpriteP->needsToBeErased && !curSpriteP->isVisible)
            {
                tempDstRect = curSpriteP->oldFrameRect;

                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.top;

                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;

                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.left;

                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                if (tempDstRect.right > tempDstRect.left &&
                    tempDstRect.bottom > tempDstRect.top)
                {
                    tempDstRect.top -= oldVertScrollRectOffset;
                    tempDstRect.bottom -= oldVertScrollRectOffset;
                    tempDstRect.left -= oldHorizScrollRectOffset;
                    tempDstRect.right -= oldHorizScrollRectOffset;

                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                        curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;

                    {
                        short temp;

                        tempDstRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                        temp = tempDstRect.right &
                                spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            tempDstRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }

                        curSpriteP->oldFrameRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                        temp = curSpriteP->oldFrameRect.right &
                            spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            curSpriteP->oldFrameRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }
                    }

                    curSpriteP->deltaFrameRect.top =
                        SW_MIN(curSpriteP->oldFrameRect.top, curSpriteP->destFrameRect.top);
                    curSpriteP->deltaFrameRect.left =
                        SW_MIN(curSpriteP->oldFrameRect.left, curSpriteP->destFrameRect.left);
                    curSpriteP->deltaFrameRect.bottom =
                        SW_MAX(curSpriteP->oldFrameRect.bottom, curSpriteP->destFrameRect.bottom);
                    curSpriteP->deltaFrameRect.right =
                        SW_MAX(curSpriteP->oldFrameRect.right, curSpriteP->destFrameRect.right);

                    SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                }
                else if (curSpriteP->destRectIsVisible)
                {
                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;
                }
            }
            else if (curSpriteP->isVisible)
            {
                if (curSpriteP->oldRectIsVisible)
                {
                    if ((hScrollDelta > 0 &&
                        (curSpriteP->destFrameRect.left < visScrollRectP->left) &&
                        (curSpriteP->destFrameRect.right > spriteWorldP->oldVisScrollRect.left)) ||
                        (hScrollDelta < 0 &&
                        (curSpriteP->destFrameRect.left < spriteWorldP->oldVisScrollRect.right) &&
                        (curSpriteP->destFrameRect.right > visScrollRectP->right)) )
                    {
                        tempDstRect = curSpriteP->oldFrameRect;

                        if (hScrollDelta > 0)
                        {
                            if (tempDstRect.right > visScrollRectP->left)
                                tempDstRect.right = visScrollRectP->left;
                        }
                        else
                        {
                            if (tempDstRect.left < visScrollRectP->right)
                                tempDstRect.left = visScrollRectP->right;
                        }

                        if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                            tempDstRect.top = spriteWorldP->oldVisScrollRect.top;
                        if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                            tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;
                        if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                            tempDstRect.left = spriteWorldP->oldVisScrollRect.left;
                        if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                            tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
                        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

                        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                    }

                    if ((vScrollDelta > 0 &&
                        (curSpriteP->destFrameRect.top < visScrollRectP->top) &&
                        (curSpriteP->destFrameRect.bottom > spriteWorldP->oldVisScrollRect.top)) ||
                        (vScrollDelta < 0 &&
                        (curSpriteP->destFrameRect.top < spriteWorldP->oldVisScrollRect.bottom) &&
                        (curSpriteP->destFrameRect.bottom > visScrollRectP->bottom)) )
                    {
                        tempDstRect = curSpriteP->oldFrameRect;

                        if (vScrollDelta > 0)
                        {
                            if (tempDstRect.bottom > visScrollRectP->top)
                                tempDstRect.bottom = visScrollRectP->top;
                        }
                        else
                        {
                            if (tempDstRect.top < visScrollRectP->bottom)
                                tempDstRect.top = visScrollRectP->bottom;
                        }

                        if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                            tempDstRect.top = spriteWorldP->oldVisScrollRect.top;
                        if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                            tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;
                        if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                            tempDstRect.left = spriteWorldP->oldVisScrollRect.left;
                        if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                            tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
                        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

                        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                    }
                }

                if (curSpriteP->destRectIsVisible)
                {
                    if (headIdleSpriteP == NULL)
                        headIdleSpriteP = curSpriteP;

                    if (curIdleSpriteP != NULL)
                        curIdleSpriteP->nextIdleSpriteP = curSpriteP;

                    curIdleSpriteP = curSpriteP;
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (curActiveSpriteP != NULL)
        curActiveSpriteP->nextActiveSpriteP = NULL;

    if (curIdleSpriteP != NULL)
        curIdleSpriteP->nextIdleSpriteP = NULL;

    curIdleSpriteP = headIdleSpriteP;
    while (curIdleSpriteP != NULL)
    {
        if ((vScrollDelta > 0 &&
            (curIdleSpriteP->destFrameRect.top < visScrollRectP->bottom) &&
            (curIdleSpriteP->destFrameRect.bottom > spriteWorldP->oldVisScrollRect.bottom) ) ||
            (vScrollDelta < 0 &&
            (curIdleSpriteP->destFrameRect.top < spriteWorldP->oldVisScrollRect.top) &&
            (curIdleSpriteP->destFrameRect.bottom > visScrollRectP->top)) )
        {
            tempDstRect = curIdleSpriteP->destOffscreenRect;
            tempSrcRect = curIdleSpriteP->clippedSourceRect;

            if (vScrollDelta < 0)
            {
                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.top)
                {
                    tempSrcRect.bottom += spriteWorldP->oldVisScrollRect.top -
                        tempDstRect.bottom;
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.top;
                }
            }
            else
            {
                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.bottom)
                {
                    tempSrcRect.top += spriteWorldP->oldVisScrollRect.bottom -
                        tempDstRect.top;
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.bottom;
                }
            }

            tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
            tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

            SWDrawWrappedSprite(curIdleSpriteP, spriteWorldP->workFrameP,
                    &tempSrcRect, &tempDstRect);

            if (spriteWorldP->tilingIsOn &&
                curIdleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
            {
                tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curIdleSpriteP->tileDepth);
            }
        }

        if ((hScrollDelta > 0 &&
            (curIdleSpriteP->destFrameRect.left < visScrollRectP->right) &&
            (curIdleSpriteP->destFrameRect.right > spriteWorldP->oldVisScrollRect.right) ) ||
            (hScrollDelta < 0 &&
            (curIdleSpriteP->destFrameRect.left < spriteWorldP->oldVisScrollRect.left) &&
            (curIdleSpriteP->destFrameRect.right > visScrollRectP->left)) )
        {
            tempDstRect = curIdleSpriteP->destOffscreenRect;
            tempSrcRect = curIdleSpriteP->clippedSourceRect;

            if (hScrollDelta < 0)
            {
                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.left)
                {
                    tempSrcRect.right += spriteWorldP->oldVisScrollRect.left -
                        tempDstRect.right;
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.left;
                }
            }
            else
            {
                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.right)
                {
                    tempSrcRect.left += spriteWorldP->oldVisScrollRect.right -
                        tempDstRect.left;
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.right;
                }
            }

            tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
            tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
            tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

            SWDrawWrappedSprite(curIdleSpriteP, spriteWorldP->workFrameP,
                    &tempSrcRect, &tempDstRect);

            if (spriteWorldP->tilingIsOn &&
                curIdleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
            {
                tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curIdleSpriteP->tileDepth);
            }
        }

        curIdleSpriteP = curIdleSpriteP->nextIdleSpriteP;
    }

    curRectStructP = spriteWorldP->headUpdateRectP;
    while ( curRectStructP != NULL )
    {
        tempDstRect = curRectStructP->updateRect;

        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
        curRectStructP = curRectStructP->nextRectStructP;
    }

    if (spriteWorldP->numTilesChanged > 0)
        SWCheckWrappedIdleSpritesWithTiles(spriteWorldP, headIdleSpriteP);

    if (spriteWorldP->headUpdateRectP != NULL)
        SWCheckWrappedIdleSpritesWithRects(spriteWorldP, headIdleSpriteP);

    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != NULL)
        {
            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect.top -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.bottom -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.left -= spriteWorldP->horizScrollRectOffset;
                curSpriteP->destOffscreenRect.right -= spriteWorldP->horizScrollRectOffset;

                if (curSpriteP->needsToBeDrawn)
                {
                    if (curSpriteP->destRectIsVisible)
                    {
                        gSWCurrentSpriteBeingDrawn = curSpriteP;

                        SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                            &curSpriteP->clippedSourceRect,
                            &curSpriteP->destOffscreenRect);

                        gSWCurrentSpriteBeingDrawn = NULL;

                        if (spriteWorldP->tilingIsOn &&
                            curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                        {
                            tempDstRect = curSpriteP->destOffscreenRect;
                            tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                            tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                            SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                        }
                    }
                }
                else
                {
                    if (curSpriteP->destRectIsVisible)
                    {
                        SWCheckWrappedIdleSpriteOverlap(spriteWorldP,
                            curSpriteP, headActiveSpriteP);
                    }
                }
            }

            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
            curSpriteP->oldRectIsVisible = curSpriteP->destRectIsVisible;

            curSpriteP->needsToBeDrawn = false;
            curSpriteP->needsToBeErased = false;

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;

    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    SetGWorld(spriteWorldP->windowFrameP->framePort, nil);

    SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->windowFrameP, &spriteWorldP->windRect);

    nextRectStructP = spriteWorldP->headUpdateRectP;
    while ( nextRectStructP != NULL )
    {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        DisposePtr( (Ptr)curRectStructP );
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

    gSWCurrentSpriteWorld = NULL;

    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWRemoveSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }
}

/* SWFastAnimateScrollingSpriteWorld (mac _SWFastAnimateScrollingSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWFastAnimateScrollingSpriteWorld i386 0x000b5fe7 ppc 0x000c1a54
void SWFastAnimateScrollingSpriteWorld(
    SpriteWorldPtr spriteWorldP)
{
    UpdateRectStructPtr         curRectStructP,
                                nextRectStructP;
    register SpriteLayerPtr     curSpriteLayerP;
    register SpritePtr          curSpriteP;
    SpritePtr                   headActiveSpriteP = NULL;
    SpritePtr                   headIdleSpriteP = NULL;
    SpritePtr                   curActiveSpriteP = NULL;
    SpritePtr                   curIdleSpriteP = NULL;
    Rect                        *visScrollRectP = &spriteWorldP->visScrollRect;
    Rect                        tempDstRect;
    Rect                        *changedRectP;
    short                       hScrollDelta, vScrollDelta, curTileLayer, index;
    short                       oldVertScrollRectOffset, oldHorizScrollRectOffset;

    SW_ASSERT(spriteWorldP != NULL, 1007);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 1008);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 1009);
    SW_ASSERT(spriteWorldP->windowFrameP->isFrameLocked, 1010);

    if (!spriteWorldP->frameHasOccurred)
        return;

    gSWCurrentSpriteWorld = spriteWorldP;

    oldVertScrollRectOffset = spriteWorldP->backRect.bottom *
        (spriteWorldP->oldVisScrollRect.top / spriteWorldP->backRect.bottom);

    oldHorizScrollRectOffset = spriteWorldP->backRect.right *
        (spriteWorldP->oldVisScrollRect.left / spriteWorldP->backRect.right);

    hScrollDelta = visScrollRectP->left - spriteWorldP->oldVisScrollRect.left;
    vScrollDelta = visScrollRectP->top - spriteWorldP->oldVisScrollRect.top;
    if (hScrollDelta || vScrollDelta)
    {
        SWAnimateScrollingSpriteWorld(spriteWorldP);
        return;
    }

    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWAddSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }

    SetGWorld(spriteWorldP->workFrameP->framePort, nil);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != NULL)
        {
            SW_ASSERT(curSpriteP->curFrameP->isFrameLocked, 1059);
            curSpriteP->tileDepth = curTileLayer;

            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect = curSpriteP->destFrameRect;
                curSpriteP->clippedSourceRect = curSpriteP->curFrameP->frameRect;

                if (curSpriteP->destOffscreenRect.top < visScrollRectP->top)
                {
                    curSpriteP->clippedSourceRect.top += visScrollRectP->top -
                            curSpriteP->destOffscreenRect.top;
                    curSpriteP->destOffscreenRect.top = visScrollRectP->top;
                }

                if (curSpriteP->destOffscreenRect.bottom > visScrollRectP->bottom)
                {
                    curSpriteP->clippedSourceRect.bottom += visScrollRectP->bottom -
                            curSpriteP->destOffscreenRect.bottom;
                    curSpriteP->destOffscreenRect.bottom = visScrollRectP->bottom;
                }

                if (curSpriteP->destOffscreenRect.left < visScrollRectP->left)
                {
                    curSpriteP->clippedSourceRect.left += visScrollRectP->left -
                            curSpriteP->destOffscreenRect.left;
                    curSpriteP->destOffscreenRect.left = visScrollRectP->left;
                }

                if (curSpriteP->destOffscreenRect.right > visScrollRectP->right)
                {
                    curSpriteP->clippedSourceRect.right += visScrollRectP->right -
                            curSpriteP->destOffscreenRect.right;
                    curSpriteP->destOffscreenRect.right = visScrollRectP->right;
                }

                curSpriteP->destRectIsVisible =
                        (curSpriteP->destOffscreenRect.right >
                        curSpriteP->destOffscreenRect.left &&
                        curSpriteP->destOffscreenRect.bottom >
                        curSpriteP->destOffscreenRect.top);
            }

            if (curSpriteP->needsToBeDrawn && curSpriteP->isVisible ||
                curSpriteP->needsToBeErased && !curSpriteP->isVisible)
            {
                tempDstRect = curSpriteP->oldFrameRect;

                if (tempDstRect.top < spriteWorldP->oldVisScrollRect.top)
                    tempDstRect.top = spriteWorldP->oldVisScrollRect.top;

                if (tempDstRect.bottom > spriteWorldP->oldVisScrollRect.bottom)
                    tempDstRect.bottom = spriteWorldP->oldVisScrollRect.bottom;

                if (tempDstRect.left < spriteWorldP->oldVisScrollRect.left)
                    tempDstRect.left = spriteWorldP->oldVisScrollRect.left;

                if (tempDstRect.right > spriteWorldP->oldVisScrollRect.right)
                    tempDstRect.right = spriteWorldP->oldVisScrollRect.right;

                if (tempDstRect.right > tempDstRect.left &&
                    tempDstRect.bottom > tempDstRect.top)
                {
                    tempDstRect.top -= oldVertScrollRectOffset;
                    tempDstRect.bottom -= oldVertScrollRectOffset;
                    tempDstRect.left -= oldHorizScrollRectOffset;
                    tempDstRect.right -= oldHorizScrollRectOffset;

                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;

                    {
                        short temp;

                        tempDstRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                        temp = tempDstRect.right &
                            spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            tempDstRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }

                        curSpriteP->oldFrameRect.left &=
                            (spriteWorldP->workFrameP->leftAlignFactor);

                        temp = curSpriteP->oldFrameRect.right &
                            spriteWorldP->workFrameP->rightAlignFactor;
                        if (temp != 0)
                        {
                            curSpriteP->oldFrameRect.right +=
                                (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                        }
                    }

                    curSpriteP->deltaFrameRect.top =
                        SW_MIN(curSpriteP->oldFrameRect.top, curSpriteP->destFrameRect.top);
                    curSpriteP->deltaFrameRect.left =
                        SW_MIN(curSpriteP->oldFrameRect.left, curSpriteP->destFrameRect.left);
                    curSpriteP->deltaFrameRect.bottom =
                        SW_MAX(curSpriteP->oldFrameRect.bottom, curSpriteP->destFrameRect.bottom);
                    curSpriteP->deltaFrameRect.right =
                        SW_MAX(curSpriteP->oldFrameRect.right, curSpriteP->destFrameRect.right);

                    SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
                }
                else if (curSpriteP->destRectIsVisible)
                {
                    if (headActiveSpriteP == NULL)
                        headActiveSpriteP = curSpriteP;

                    if (curActiveSpriteP != NULL)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                    curActiveSpriteP = curSpriteP;
                }
            }
            else if (curSpriteP->isVisible)
            {
                if (curSpriteP->destRectIsVisible)
                {
                    if (headIdleSpriteP == NULL)
                        headIdleSpriteP = curSpriteP;

                    if (curIdleSpriteP != NULL)
                        curIdleSpriteP->nextIdleSpriteP = curSpriteP;

                    curIdleSpriteP = curSpriteP;
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (curActiveSpriteP != NULL)
        curActiveSpriteP->nextActiveSpriteP = NULL;

    if (curIdleSpriteP != NULL)
        curIdleSpriteP->nextIdleSpriteP = NULL;

    curRectStructP = spriteWorldP->headUpdateRectP;
    while ( curRectStructP != NULL )
    {
        tempDstRect = curRectStructP->updateRect;

        tempDstRect.top -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.bottom -= spriteWorldP->vertScrollRectOffset;
        tempDstRect.left -= spriteWorldP->horizScrollRectOffset;
        tempDstRect.right -= spriteWorldP->horizScrollRectOffset;

        SWEraseWrappedSprite(spriteWorldP, &tempDstRect);
        curRectStructP = curRectStructP->nextRectStructP;
    }

    if (spriteWorldP->numTilesChanged > 0)
        SWCheckWrappedIdleSpritesWithTiles(spriteWorldP, headIdleSpriteP);

    if (spriteWorldP->headUpdateRectP != NULL)
        SWCheckWrappedIdleSpritesWithRects(spriteWorldP, headIdleSpriteP);

    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL)
    {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != NULL)
        {
            if (curSpriteP->isVisible)
            {
                curSpriteP->destOffscreenRect.top -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.bottom -= spriteWorldP->vertScrollRectOffset;
                curSpriteP->destOffscreenRect.left -= spriteWorldP->horizScrollRectOffset;
                curSpriteP->destOffscreenRect.right -= spriteWorldP->horizScrollRectOffset;

                if (curSpriteP->needsToBeDrawn)
                {
                    if (curSpriteP->destRectIsVisible)
                    {
                        gSWCurrentSpriteBeingDrawn = curSpriteP;

                        SWDrawWrappedSprite(curSpriteP, spriteWorldP->workFrameP,
                            &curSpriteP->clippedSourceRect,
                            &curSpriteP->destOffscreenRect);

                        gSWCurrentSpriteBeingDrawn = NULL;

                        if (spriteWorldP->tilingIsOn &&
                            curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                        {
                            tempDstRect = curSpriteP->destOffscreenRect;
                            tempDstRect.top += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.bottom += spriteWorldP->vertScrollRectOffset;
                            tempDstRect.left += spriteWorldP->horizScrollRectOffset;
                            tempDstRect.right += spriteWorldP->horizScrollRectOffset;
                            SWDrawTilesAboveSprite(spriteWorldP, &tempDstRect, curSpriteP->tileDepth);
                        }
                    }
                }
                else
                {
                    if (curSpriteP->destRectIsVisible)
                    {
                        SWCheckWrappedIdleSpriteOverlap(spriteWorldP,
                            curSpriteP, headActiveSpriteP);
                    }
                }
            }

            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
            curSpriteP->oldRectIsVisible = curSpriteP->destRectIsVisible;

            curSpriteP->needsToBeDrawn = false;
            curSpriteP->needsToBeErased = false;

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;

    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    SetGWorld(spriteWorldP->windowFrameP->framePort, nil);

    curRectStructP = spriteWorldP->headUpdateRectP;
    while ( curRectStructP != NULL )
    {
        SWWrapRectToScreen(spriteWorldP, &curRectStructP->updateRect);
        curRectStructP = curRectStructP->nextRectStructP;
    }

    changedRectP = spriteWorldP->changedTiles;
    for (index = 0; index < spriteWorldP->numTilesChanged; index++, changedRectP++)
    {
        SWWrapRectToScreen(spriteWorldP, changedRectP);
    }

    curSpriteP = headActiveSpriteP;
    while (curSpriteP != NULL)
    {
        SWWrapRectToScreen(spriteWorldP, &curSpriteP->deltaFrameRect);
        curSpriteP->deltaFrameRect = curSpriteP->destFrameRect;
        curSpriteP = curSpriteP->nextActiveSpriteP;
    }

    nextRectStructP = spriteWorldP->headUpdateRectP;
    while ( nextRectStructP != NULL )
    {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        DisposePtr( (Ptr)curRectStructP );
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

    gSWCurrentSpriteWorld = NULL;

    if ( spriteWorldP->deadSpriteLayerP->headSpriteP != NULL )
    {
        SWRemoveSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);
    }
}

/* SWUnwrapWorkAreaToBackground (mac _SWUnwrapWorkAreaToBackground, mac-only). STATE: see the mac gate. */
// MAC: _SWUnwrapWorkAreaToBackground i386 0x000b6701 ppc 0x000c22a0
void SWUnwrapWorkAreaToBackground(
    SpriteWorldPtr spriteWorldP)
{
    Rect    destRect;

    destRect = spriteWorldP->offscreenScrollRect;
    OffsetRect(&destRect, -destRect.left, -destRect.top);

    SWWrapWorkAreaToFrame(spriteWorldP, spriteWorldP->backFrameP, &destRect);
}

/* SWDrawWrappedSprite (mac _SWDrawWrappedSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawWrappedSprite i386 0x000b3e8c ppc 0x000bf2a0
void SWDrawWrappedSprite(
    SpritePtr srcSpriteP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    Rect        tempDstRect;

    SW_ASSERT(srcSpriteP->frameDrawProc != NULL , 1437);
    SW_ASSERT(srcSpriteP->curFrameP != NULL , 1438);

    (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP, srcRect, dstRect);

    if (dstRect->bottom > dstFrameP->frameRect.bottom)
    {
        tempDstRect.top = dstRect->top - dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom - dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);

        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
    }

    if (dstRect->right > dstFrameP->frameRect.right)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left - dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right - dstFrameP->frameRect.right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);
    }
    else if (dstRect->left < dstFrameP->frameRect.left)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left + dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right + dstFrameP->frameRect.right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);
    }

    if (dstRect->top < dstFrameP->frameRect.top)
    {
        tempDstRect.top = dstRect->top + dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom + dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
            srcRect, &tempDstRect);

        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*srcSpriteP->frameDrawProc)(srcSpriteP->curFrameP, dstFrameP,
                srcRect, &tempDstRect);
        }
    }
}

/* SWEraseWrappedSprite (mac _SWEraseWrappedSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWEraseWrappedSprite i386 0x000b43ec ppc 0x000bf984
void SWEraseWrappedSprite(
    SpriteWorldPtr spriteWorldP,
    Rect* dstRect)
{
    Rect        tempDstRect;
    FramePtr    srcFrameP = spriteWorldP->backFrameP;
    FramePtr    dstFrameP = spriteWorldP->workFrameP;

    (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP, dstRect, dstRect);

    if (dstRect->bottom > dstFrameP->frameRect.bottom)
    {
        tempDstRect.top = dstRect->top - dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom - dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);

        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
    }

    if (dstRect->right > dstFrameP->frameRect.right)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left - dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right - dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);
    }
    else if (dstRect->left < dstFrameP->frameRect.left)
    {
        tempDstRect.top = dstRect->top;
        tempDstRect.bottom = dstRect->bottom;
        tempDstRect.left = dstRect->left + dstFrameP->frameRect.right;
        tempDstRect.right = dstRect->right + dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);
    }

    if (dstRect->top < dstFrameP->frameRect.top)
    {
        tempDstRect.top = dstRect->top + dstFrameP->frameRect.bottom;
        tempDstRect.bottom = dstRect->bottom + dstFrameP->frameRect.bottom;
        tempDstRect.left = dstRect->left;
        tempDstRect.right = dstRect->right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDstRect, &tempDstRect);

        if (dstRect->right > dstFrameP->frameRect.right)
        {
            tempDstRect.left -= dstFrameP->frameRect.right;
            tempDstRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
        else if (dstRect->left < dstFrameP->frameRect.left)
        {
            tempDstRect.left += dstFrameP->frameRect.right;
            tempDstRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDstRect, &tempDstRect);
        }
    }
}

/* SWWrapWorkAreaToFrame (mac _SWWrapWorkAreaToFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWWrapWorkAreaToFrame i386 0x000b3747 ppc 0x000bebac
void SWWrapWorkAreaToFrame(
    SpriteWorldPtr  spriteWorldP,
    FramePtr dstFrameP,
    Rect *dstRectP)
{
    FramePtr    srcFrameP = spriteWorldP->workFrameP;

    Rect    srcRect = spriteWorldP->offscreenScrollRect;
    Rect    dstRect = *dstRectP;
    Rect    srcRectA, srcRectB, dstRectA, dstRectB;

    short   topClip=0, rightClip=0, bottomClip=0, leftClip=0;

    if (srcRect.top < srcFrameP->frameRect.top)
    {
        topClip = srcFrameP->frameRect.top - srcRect.top;
        srcRect.top += topClip;
    }

    if (srcRect.bottom > srcFrameP->frameRect.bottom)
    {
        bottomClip = srcRect.bottom - srcFrameP->frameRect.bottom;
        srcRect.bottom -= bottomClip;
    }

    if (srcRect.left < srcFrameP->frameRect.left)
    {
        leftClip = srcFrameP->frameRect.left - srcRect.left;
        srcRect.left += leftClip;
    }

    if (srcRect.right > srcFrameP->frameRect.right)
    {
        rightClip = srcRect.right - srcFrameP->frameRect.right;
        srcRect.right -= rightClip;
    }

    if (topClip)
    {
        srcRectA.right = srcRect.right;
        srcRectA.left = srcRect.left;
        srcRectA.bottom = srcFrameP->frameRect.bottom;
        srcRectA.top = srcFrameP->frameRect.bottom - topClip;

        dstRectA.top = dstRect.top;
        dstRectA.bottom = dstRect.top + topClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;

        if (leftClip)
        {
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.left + leftClip;
            dstRectB.bottom = dstRect.top + topClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            }
        }
        else if (rightClip)
        {
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;

            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.right;
            dstRectB.bottom = dstRect.top + topClip;
            dstRectB.left = dstRect.right - rightClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                     &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            }
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }

    dstRectA.left = dstRect.left + leftClip;
    dstRectA.top = dstRect.top + topClip;
    dstRectA.right = dstRect.right - rightClip;
    dstRectA.bottom = dstRect.bottom - bottomClip;

    if (leftClip)
    {
        srcRectB.top = srcRect.top;
        srcRectB.bottom = srcRect.bottom;
        srcRectB.right = srcFrameP->frameRect.right;
        srcRectB.left = srcFrameP->frameRect.right - leftClip;

        dstRectB.left = dstRect.left;
        dstRectB.right = dstRect.left + leftClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        if (spriteWorldP->doubleRectDrawProc != NULL)
        {
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRectB, &dstRectB, &srcRect, &dstRectA);
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
        }
    }
    else if (rightClip)
    {
        srcRectB.top = srcRect.top;
        srcRectB.bottom = srcRect.bottom;
        srcRectB.left = srcFrameP->frameRect.left;
        srcRectB.right = srcFrameP->frameRect.left + rightClip;

        dstRectB.right = dstRect.right;
        dstRectB.left = dstRect.right - rightClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        if (spriteWorldP->doubleRectDrawProc != NULL)
        {
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRect, &dstRectA, &srcRectB, &dstRectB);
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
        }
    }
    else
    {
        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
    }

    if (bottomClip)
    {
        srcRectA.right = srcRect.right;
        srcRectA.left = srcRect.left;
        srcRectA.top = srcFrameP->frameRect.top;
        srcRectA.bottom = srcFrameP->frameRect.top + bottomClip;

        dstRectA.bottom = dstRect.bottom;
        dstRectA.top = dstRect.bottom - bottomClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;

        if (leftClip)
        {
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

            dstRectB.bottom = dstRect.bottom;
            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.right = dstRect.left + leftClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            }
        }
        else if (rightClip)
        {
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;

            dstRectB.bottom = dstRect.bottom;
            dstRectB.right = dstRect.right;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.left = dstRect.right - rightClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            }
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }
}

/* SWWrapRectToScreen (mac _SWWrapRectToScreen, mac-only). STATE: see the mac gate. */
// MAC: _SWWrapRectToScreen i386 0x000b584d ppc 0x000c1350
void SWWrapRectToScreen(
    SpriteWorldPtr  spriteWorldP,
    Rect            *srcRectP)
{
    FramePtr    srcFrameP = spriteWorldP->workFrameP;
    FramePtr    dstFrameP = spriteWorldP->windowFrameP;
    Rect        srcRect, dstRect, srcRectA, srcRectB, dstRectA, dstRectB;
    short       topClip=0, rightClip=0, bottomClip=0, leftClip=0;

    srcRect = *srcRectP;

    dstRect = srcRect;
    dstRect.top -= spriteWorldP->visScrollRect.top;
    dstRect.bottom -= spriteWorldP->visScrollRect.top;
    dstRect.left -= spriteWorldP->visScrollRect.left;
    dstRect.right -= spriteWorldP->visScrollRect.left;
    dstRect.top += spriteWorldP->windRect.top;
    dstRect.bottom += spriteWorldP->windRect.top;
    dstRect.left += spriteWorldP->windRect.left;
    dstRect.right += spriteWorldP->windRect.left;

    if (dstRect.top < spriteWorldP->windRect.top)
    {
        srcRect.top += spriteWorldP->windRect.top - dstRect.top;
        dstRect.top = spriteWorldP->windRect.top;
    }

    if (dstRect.bottom > spriteWorldP->windRect.bottom)
    {
        srcRect.bottom += spriteWorldP->windRect.bottom - dstRect.bottom;
        dstRect.bottom = spriteWorldP->windRect.bottom;
    }

    if (dstRect.left < spriteWorldP->windRect.left)
    {
        srcRect.left += spriteWorldP->windRect.left - dstRect.left;
        dstRect.left = spriteWorldP->windRect.left;
    }

    if (dstRect.right > spriteWorldP->windRect.right)
    {
        srcRect.right += spriteWorldP->windRect.right - dstRect.right;
        dstRect.right = spriteWorldP->windRect.right;
    }

    if ( (dstRect.right <= dstRect.left) || (dstRect.bottom <= dstRect.top) )
        return;

    srcRect.top -= spriteWorldP->vertScrollRectOffset;
    srcRect.bottom -= spriteWorldP->vertScrollRectOffset;
    srcRect.left -= spriteWorldP->horizScrollRectOffset;
    srcRect.right -= spriteWorldP->horizScrollRectOffset;

    if (srcRect.top < srcFrameP->frameRect.top)
    {
        topClip = srcFrameP->frameRect.top - srcRect.top;
        srcRect.top += topClip;
    }

    if (srcRect.bottom > srcFrameP->frameRect.bottom)
    {
        bottomClip = srcRect.bottom - srcFrameP->frameRect.bottom;
        srcRect.bottom -= bottomClip;
    }

    if (srcRect.left < srcFrameP->frameRect.left)
    {
        leftClip = srcFrameP->frameRect.left - srcRect.left;
        srcRect.left += leftClip;
    }

    if (srcRect.right > srcFrameP->frameRect.right)
    {
        rightClip = srcRect.right - srcFrameP->frameRect.right;
        srcRect.right -= rightClip;
    }

    if (topClip)
    {
        srcRectA.right = srcRect.right;
        srcRectA.left = srcRect.left;
        srcRectA.bottom = srcFrameP->frameRect.bottom;
        srcRectA.top = srcFrameP->frameRect.bottom - topClip;

        dstRectA.top = dstRect.top;
        dstRectA.bottom = dstRect.top + topClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;

        if (leftClip)
        {
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.left + leftClip;
            dstRectB.bottom = dstRect.top + topClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            }
        }
        else if (rightClip)
        {
            srcRectB.bottom = srcFrameP->frameRect.bottom;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;
            srcRectB.top = srcFrameP->frameRect.bottom - topClip;

            dstRectB.top = dstRect.top;
            dstRectB.right = dstRect.right;
            dstRectB.bottom = dstRect.top + topClip;
            dstRectB.left = dstRect.right - rightClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                     &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            }
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }

    dstRectA.left = dstRect.left + leftClip;
    dstRectA.top = dstRect.top + topClip;
    dstRectA.right = dstRect.right - rightClip;
    dstRectA.bottom = dstRect.bottom - bottomClip;

    if (leftClip)
    {
        srcRectB.top = srcRect.top;
        srcRectB.bottom = srcRect.bottom;
        srcRectB.right = srcFrameP->frameRect.right;
        srcRectB.left = srcFrameP->frameRect.right - leftClip;

        dstRectB.left = dstRect.left;
        dstRectB.right = dstRect.left + leftClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        if (spriteWorldP->doubleRectDrawProc != NULL)
        {
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRectB, &dstRectB, &srcRect, &dstRectA);
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
        }
    }
    else if (rightClip)
    {
        srcRectB.top = srcRect.top;
        srcRectB.bottom = srcRect.bottom;
        srcRectB.left = srcFrameP->frameRect.left;
        srcRectB.right = srcFrameP->frameRect.left + rightClip;

        dstRectB.right = dstRect.right;
        dstRectB.left = dstRect.right - rightClip;
        dstRectB.top = dstRect.top + topClip;
        dstRectB.bottom = dstRect.bottom - bottomClip;

        if (spriteWorldP->doubleRectDrawProc != NULL)
        {
            (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                &srcRect, &dstRectA, &srcRectB, &dstRectB);
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
        }
    }
    else
    {
        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &dstRectA);
    }

    if (bottomClip)
    {
        srcRectA.right = srcRect.right;
        srcRectA.left = srcRect.left;
        srcRectA.top = srcFrameP->frameRect.top;
        srcRectA.bottom = srcFrameP->frameRect.top + bottomClip;

        dstRectA.bottom = dstRect.bottom;
        dstRectA.top = dstRect.bottom - bottomClip;
        dstRectA.left = dstRect.left + leftClip;
        dstRectA.right = dstRect.right - rightClip;

        if (leftClip)
        {
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.right = srcFrameP->frameRect.right;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.left = srcFrameP->frameRect.right - leftClip;

            dstRectB.bottom = dstRect.bottom;
            dstRectB.left = dstRect.left;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.right = dstRect.left + leftClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectB, &dstRectB, &srcRectA, &dstRectA);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
            }
        }
        else if (rightClip)
        {
            srcRectB.top = srcFrameP->frameRect.top;
            srcRectB.left = srcFrameP->frameRect.left;
            srcRectB.bottom = srcFrameP->frameRect.top + bottomClip;
            srcRectB.right = srcFrameP->frameRect.left + rightClip;

            dstRectB.bottom = dstRect.bottom;
            dstRectB.right = dstRect.right;
            dstRectB.top = dstRect.bottom - bottomClip;
            dstRectB.left = dstRect.right - rightClip;

            if (spriteWorldP->doubleRectDrawProc != NULL)
            {
                (*spriteWorldP->doubleRectDrawProc)(srcFrameP, dstFrameP,
                    &srcRectA, &dstRectA, &srcRectB, &dstRectB);
            }
            else
            {
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
                (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectB, &dstRectB);
            }
        }
        else
        {
            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRectA, &dstRectA);
        }
    }
}

/* SWCheckWrappedIdleSpriteOverlap (mac _SWCheckWrappedIdleSpriteOverlap, mac-only). STATE: see the mac gate. */
// MAC: _SWCheckWrappedIdleSpriteOverlap i386 0x000b4997 ppc 0x000c0140
void SWCheckWrappedIdleSpriteOverlap(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       idleSpriteP,
    SpritePtr       headActiveSpriteP)
{
    Rect                *visScrollRectP = &spriteWorldP->visScrollRect;
    register SpritePtr  activeSpriteP = headActiveSpriteP;
    Rect                srcSectRect,
                        dstSectRect;

    while (activeSpriteP != NULL)
    {
        if ((idleSpriteP->oldFrameRect.top < activeSpriteP->deltaFrameRect.bottom) &&
             (idleSpriteP->oldFrameRect.bottom > activeSpriteP->deltaFrameRect.top) &&
             (idleSpriteP->oldFrameRect.left < activeSpriteP->deltaFrameRect.right) &&
             (idleSpriteP->oldFrameRect.right > activeSpriteP->deltaFrameRect.left))
        {
            dstSectRect.left =
                SW_MAX(idleSpriteP->destFrameRect.left, activeSpriteP->deltaFrameRect.left);
            dstSectRect.top =
                SW_MAX(idleSpriteP->destFrameRect.top, activeSpriteP->deltaFrameRect.top);
            dstSectRect.right =
                SW_MIN(idleSpriteP->destFrameRect.right, activeSpriteP->deltaFrameRect.right);
            dstSectRect.bottom =
                SW_MIN(idleSpriteP->destFrameRect.bottom, activeSpriteP->deltaFrameRect.bottom);

            if (dstSectRect.top < visScrollRectP->top)
                dstSectRect.top = visScrollRectP->top;
            if (dstSectRect.bottom > visScrollRectP->bottom)
                dstSectRect.bottom = visScrollRectP->bottom;
            if (dstSectRect.left < visScrollRectP->left)
                dstSectRect.left = visScrollRectP->left;
            if (dstSectRect.right > visScrollRectP->right)
                dstSectRect.right = visScrollRectP->right;

            srcSectRect = idleSpriteP->curFrameP->frameRect;

            srcSectRect.left += (dstSectRect.left - idleSpriteP->destFrameRect.left);
            srcSectRect.top += (dstSectRect.top - idleSpriteP->destFrameRect.top);
            srcSectRect.right -= (idleSpriteP->destFrameRect.right - dstSectRect.right);
            srcSectRect.bottom -= (idleSpriteP->destFrameRect.bottom - dstSectRect.bottom);

            dstSectRect.top -= spriteWorldP->vertScrollRectOffset;
            dstSectRect.bottom -= spriteWorldP->vertScrollRectOffset;
            dstSectRect.left -= spriteWorldP->horizScrollRectOffset;
            dstSectRect.right -= spriteWorldP->horizScrollRectOffset;

            gSWCurrentSpriteBeingDrawn = idleSpriteP;

            SWDrawWrappedSprite(idleSpriteP, spriteWorldP->workFrameP,
                &srcSectRect, &dstSectRect);

            gSWCurrentSpriteBeingDrawn = NULL;

            if (spriteWorldP->tilingIsOn &&
                idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
            {
                dstSectRect.top += spriteWorldP->vertScrollRectOffset;
                dstSectRect.bottom += spriteWorldP->vertScrollRectOffset;
                dstSectRect.left += spriteWorldP->horizScrollRectOffset;
                dstSectRect.right += spriteWorldP->horizScrollRectOffset;
                SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
            }
        }

        activeSpriteP = activeSpriteP->nextActiveSpriteP;
    }
}

/* SWCheckWrappedIdleSpritesWithTiles (mac _SWCheckWrappedIdleSpritesWithTiles, mac-only). STATE: see the mac gate. */
// MAC: _SWCheckWrappedIdleSpritesWithTiles i386 0x000b45b5 ppc 0x000bfc0c
void SWCheckWrappedIdleSpritesWithTiles(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       headIdleSpriteP)
{
    Rect        *visScrollRectP = &spriteWorldP->visScrollRect;
    Rect        srcSectRect, dstSectRect;
    register    SpritePtr idleSpriteP;
    Rect        *changedRectP;
    short       index;

    changedRectP = spriteWorldP->changedTiles;
    for (index = 0; index < spriteWorldP->numTilesChanged; index++, changedRectP++)
    {
        idleSpriteP = headIdleSpriteP;

        while (idleSpriteP != NULL)
        {
            if ((idleSpriteP->oldFrameRect.top < changedRectP->bottom) &&
                 (idleSpriteP->oldFrameRect.bottom > changedRectP->top) &&
                 (idleSpriteP->oldFrameRect.left < changedRectP->right) &&
                 (idleSpriteP->oldFrameRect.right > changedRectP->left))
            {
                dstSectRect.left = SW_MAX(idleSpriteP->oldFrameRect.left,
                                        changedRectP->left);
                dstSectRect.top = SW_MAX(idleSpriteP->oldFrameRect.top,
                                        changedRectP->top);
                dstSectRect.right = SW_MIN(idleSpriteP->oldFrameRect.right,
                                        changedRectP->right);
                dstSectRect.bottom = SW_MIN(idleSpriteP->oldFrameRect.bottom,
                                        changedRectP->bottom);

                if (dstSectRect.top < visScrollRectP->top)
                    dstSectRect.top = visScrollRectP->top;
                if (dstSectRect.bottom > visScrollRectP->bottom)
                    dstSectRect.bottom = visScrollRectP->bottom;
                if (dstSectRect.left < visScrollRectP->left)
                    dstSectRect.left = visScrollRectP->left;
                if (dstSectRect.right > visScrollRectP->right)
                    dstSectRect.right = visScrollRectP->right;

                srcSectRect = idleSpriteP->curFrameP->frameRect;

                srcSectRect.left += (dstSectRect.left - idleSpriteP->oldFrameRect.left);
                srcSectRect.top += (dstSectRect.top - idleSpriteP->oldFrameRect.top);
                srcSectRect.right -= (idleSpriteP->oldFrameRect.right - dstSectRect.right);
                srcSectRect.bottom -= (idleSpriteP->oldFrameRect.bottom - dstSectRect.bottom);

                dstSectRect.top -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.left -= spriteWorldP->horizScrollRectOffset;
                dstSectRect.right -= spriteWorldP->horizScrollRectOffset;

                gSWCurrentSpriteBeingDrawn = idleSpriteP;

                SWDrawWrappedSprite(idleSpriteP, spriteWorldP->workFrameP,
                    &srcSectRect, &dstSectRect);

                gSWCurrentSpriteBeingDrawn = NULL;

                if (spriteWorldP->tilingIsOn &&
                    idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                {
                    dstSectRect.top += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.bottom += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.left += spriteWorldP->horizScrollRectOffset;
                    dstSectRect.right += spriteWorldP->horizScrollRectOffset;
                    SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
                }
            }

            idleSpriteP = idleSpriteP->nextIdleSpriteP;
        }
    }
}

/* SWCheckWrappedIdleSpritesWithRects (mac _SWCheckWrappedIdleSpritesWithRects, mac-only). STATE: see the mac gate. */
// MAC: _SWCheckWrappedIdleSpritesWithRects i386 0x000b47a6 ppc 0x000bfeac
void SWCheckWrappedIdleSpritesWithRects(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       headIdleSpriteP)
{
    UpdateRectStructPtr curRectStructP;
    register SpritePtr  idleSpriteP;
    Rect                *visScrollRectP = &spriteWorldP->visScrollRect;
    Rect                srcSectRect, dstSectRect;
    Rect                *changedRectP;

    curRectStructP = spriteWorldP->headUpdateRectP;

    while (curRectStructP != NULL)
    {
        changedRectP = &curRectStructP->updateRect;
        idleSpriteP = headIdleSpriteP;

        while (idleSpriteP != NULL)
        {
            if ((idleSpriteP->oldFrameRect.top < changedRectP->bottom) &&
                 (idleSpriteP->oldFrameRect.bottom > changedRectP->top) &&
                 (idleSpriteP->oldFrameRect.left < changedRectP->right) &&
                 (idleSpriteP->oldFrameRect.right > changedRectP->left))
            {
                dstSectRect.left = SW_MAX(idleSpriteP->oldFrameRect.left,
                                        changedRectP->left);
                dstSectRect.top = SW_MAX(idleSpriteP->oldFrameRect.top,
                                        changedRectP->top);
                dstSectRect.right = SW_MIN(idleSpriteP->oldFrameRect.right,
                                        changedRectP->right);
                dstSectRect.bottom = SW_MIN(idleSpriteP->oldFrameRect.bottom,
                                        changedRectP->bottom);

                if (dstSectRect.top < visScrollRectP->top)
                    dstSectRect.top = visScrollRectP->top;
                if (dstSectRect.bottom > visScrollRectP->bottom)
                    dstSectRect.bottom = visScrollRectP->bottom;
                if (dstSectRect.left < visScrollRectP->left)
                    dstSectRect.left = visScrollRectP->left;
                if (dstSectRect.right > visScrollRectP->right)
                    dstSectRect.right = visScrollRectP->right;

                srcSectRect = idleSpriteP->curFrameP->frameRect;

                srcSectRect.left += (dstSectRect.left - idleSpriteP->oldFrameRect.left);
                srcSectRect.top += (dstSectRect.top - idleSpriteP->oldFrameRect.top);
                srcSectRect.right -= (idleSpriteP->oldFrameRect.right - dstSectRect.right);
                srcSectRect.bottom -= (idleSpriteP->oldFrameRect.bottom - dstSectRect.bottom);

                dstSectRect.top -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstSectRect.left -= spriteWorldP->horizScrollRectOffset;
                dstSectRect.right -= spriteWorldP->horizScrollRectOffset;

                gSWCurrentSpriteBeingDrawn = idleSpriteP;

                SWDrawWrappedSprite(idleSpriteP, spriteWorldP->workFrameP,
                    &srcSectRect, &dstSectRect);

                gSWCurrentSpriteBeingDrawn = NULL;

                if (spriteWorldP->tilingIsOn &&
                    idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer)
                {
                    dstSectRect.top += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.bottom += spriteWorldP->vertScrollRectOffset;
                    dstSectRect.left += spriteWorldP->horizScrollRectOffset;
                    dstSectRect.right += spriteWorldP->horizScrollRectOffset;
                    SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
                }
            }

            idleSpriteP = idleSpriteP->nextIdleSpriteP;
        }

        curRectStructP = curRectStructP->nextRectStructP;
    }
}

/* SWSetScrollingWorldMoveBounds (mac _SWSetScrollingWorldMoveBounds, mac-only). STATE: see the mac gate. */
// MAC: _SWSetScrollingWorldMoveBounds i386 0x000b6a62 ppc 0x000c25a8
void SWSetScrollingWorldMoveBounds(
    SpriteWorldPtr  spriteWorldP,
    Rect* scrollRectMoveBounds)
{
    spriteWorldP->scrollRectMoveBounds = *scrollRectMoveBounds;

    SWMoveVisScrollRect(spriteWorldP,
            spriteWorldP->visScrollRect.left,
            spriteWorldP->visScrollRect.top);
}

/* SWSetScrollingWorldMoveProc (mac _SWSetScrollingWorldMoveProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetScrollingWorldMoveProc i386 0x000b6758 ppc 0x000c2310
void SWSetScrollingWorldMoveProc(
    SpriteWorldPtr spriteWorldP,
    WorldMoveProcPtr worldMoveProcP,
    SpritePtr followSpriteP)
{
    spriteWorldP->worldMoveProc = worldMoveProcP;
    spriteWorldP->followSpriteP = followSpriteP;
}

/* SWSetDoubleRectDrawProc (mac _SWSetDoubleRectDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetDoubleRectDrawProc i386 0x000b676f ppc 0x000c231c
void SWSetDoubleRectDrawProc(
    SpriteWorldPtr      spriteWorldP,
    DoubleDrawProcPtr   drawProc)
{
    spriteWorldP->doubleRectDrawProc = drawProc;
}

/* SWSetSpriteWorldScrollDelta (mac _SWSetSpriteWorldScrollDelta, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteWorldScrollDelta i386 0x000b677d ppc 0x000c2324
void SWSetSpriteWorldScrollDelta(
    SpriteWorldPtr  spriteWorldP,
    short           horizDelta,
    short           vertDelta)
{
    spriteWorldP->horizScrollDelta = horizDelta;
    spriteWorldP->vertScrollDelta = vertDelta;
}

/* SWMoveVisScrollRect (mac _SWMoveVisScrollRect, mac-only). STATE: see the mac gate. */
// MAC: _SWMoveVisScrollRect i386 0x000b6988 ppc 0x000c2504
void SWMoveVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizPos,
    short           vertPos)
{
    short   width, height;

    height = spriteWorldP->visScrollRect.bottom - spriteWorldP->visScrollRect.top;
    width = spriteWorldP->visScrollRect.right - spriteWorldP->visScrollRect.left;

    if ((int32_t)vertPos < spriteWorldP->scrollRectMoveBounds.top)
    {
        spriteWorldP->visScrollRect.top = spriteWorldP->scrollRectMoveBounds.top;
        spriteWorldP->visScrollRect.bottom = spriteWorldP->visScrollRect.top + height;
    }
    else if ((int32_t)vertPos + height > spriteWorldP->scrollRectMoveBounds.bottom)
    {
        spriteWorldP->visScrollRect.bottom = spriteWorldP->scrollRectMoveBounds.bottom;
        spriteWorldP->visScrollRect.top = spriteWorldP->visScrollRect.bottom - height;
    }
    else
    {
        spriteWorldP->visScrollRect.top = vertPos;
        spriteWorldP->visScrollRect.bottom = vertPos + height;
    }

    if ((int32_t)horizPos < spriteWorldP->scrollRectMoveBounds.left)
    {
        spriteWorldP->visScrollRect.left = spriteWorldP->scrollRectMoveBounds.left;
        spriteWorldP->visScrollRect.right = spriteWorldP->visScrollRect.left + width;
    }
    else if ((int32_t)horizPos + width > spriteWorldP->scrollRectMoveBounds.right)
    {
        spriteWorldP->visScrollRect.right = spriteWorldP->scrollRectMoveBounds.right;
        spriteWorldP->visScrollRect.left = spriteWorldP->visScrollRect.right - width;
    }
    else
    {
        spriteWorldP->visScrollRect.left = horizPos;
        spriteWorldP->visScrollRect.right = horizPos + width;
    }

    SWCalculateOffscreenScrollRect(spriteWorldP);
}

/* SWOffsetVisScrollRect (mac _SWOffsetVisScrollRect, mac-only). STATE: see the mac gate. */
// MAC: _SWOffsetVisScrollRect i386 0x000b6820 ppc 0x000c239c
void SWOffsetVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           horizOffset,
    short           vertOffset)
{
    if ((int32_t)spriteWorldP->visScrollRect.top + vertOffset <
            spriteWorldP->scrollRectMoveBounds.top)
    {
        spriteWorldP->visScrollRect.bottom -= spriteWorldP->visScrollRect.top -
            spriteWorldP->scrollRectMoveBounds.top;
        spriteWorldP->visScrollRect.top = spriteWorldP->scrollRectMoveBounds.top;
    }
    else if ((int32_t)spriteWorldP->visScrollRect.bottom + vertOffset >
            spriteWorldP->scrollRectMoveBounds.bottom)
    {
        spriteWorldP->visScrollRect.top -= spriteWorldP->visScrollRect.bottom -
            spriteWorldP->scrollRectMoveBounds.bottom;
        spriteWorldP->visScrollRect.bottom = spriteWorldP->scrollRectMoveBounds.bottom;
    }
    else
    {
        spriteWorldP->visScrollRect.top += vertOffset;
        spriteWorldP->visScrollRect.bottom += vertOffset;
    }

    if ((int32_t)spriteWorldP->visScrollRect.left + horizOffset <
            spriteWorldP->scrollRectMoveBounds.left)
    {
        spriteWorldP->visScrollRect.right -= spriteWorldP->visScrollRect.left -
            spriteWorldP->scrollRectMoveBounds.left;
        spriteWorldP->visScrollRect.left = spriteWorldP->scrollRectMoveBounds.left;
    }
    else if ((int32_t)spriteWorldP->visScrollRect.right + horizOffset >
            spriteWorldP->scrollRectMoveBounds.right)
    {
        spriteWorldP->visScrollRect.left -= spriteWorldP->visScrollRect.right -
            spriteWorldP->scrollRectMoveBounds.right;
        spriteWorldP->visScrollRect.right = spriteWorldP->scrollRectMoveBounds.right;
    }
    else
    {
        spriteWorldP->visScrollRect.left += horizOffset;
        spriteWorldP->visScrollRect.right += horizOffset;
    }

    SWCalculateOffscreenScrollRect(spriteWorldP);
}

/* SWResizeVisScrollRect (mac _SWResizeVisScrollRect, mac-only). STATE: see the mac gate. */
// MAC: _SWResizeVisScrollRect i386 0x000b6a93 ppc 0x000c25c4
void SWResizeVisScrollRect(
    SpriteWorldPtr  spriteWorldP,
    short           width,
    short           height)
{
    if ((int32_t)spriteWorldP->visScrollRect.top + height >
        spriteWorldP->scrollRectMoveBounds.bottom)
    {
        spriteWorldP->visScrollRect.bottom = spriteWorldP->scrollRectMoveBounds.bottom;
        spriteWorldP->visScrollRect.top = spriteWorldP->visScrollRect.bottom - height;
    }
    else
    {
        spriteWorldP->visScrollRect.bottom = spriteWorldP->visScrollRect.top + height;
    }

    if ((int32_t)spriteWorldP->visScrollRect.left + width >
        spriteWorldP->scrollRectMoveBounds.right)
    {
        spriteWorldP->visScrollRect.right = spriteWorldP->scrollRectMoveBounds.right;
        spriteWorldP->visScrollRect.left = spriteWorldP->visScrollRect.right - width;
    }
    else
    {
        spriteWorldP->visScrollRect.right = spriteWorldP->visScrollRect.left + width;
    }

    spriteWorldP->oldVisScrollRect = spriteWorldP->visScrollRect;
    SWCalculateOffscreenScrollRect(spriteWorldP);
}

/* SWCalculateOffscreenScrollRect (mac _SWCalculateOffscreenScrollRect, mac-only). STATE: see the mac gate. */
// MAC: _SWCalculateOffscreenScrollRect i386 0x000b6793 ppc 0x000c2330
void SWCalculateOffscreenScrollRect(
    SpriteWorldPtr spriteWorldP)
{
    Rect *offscreenScrollRectP = &spriteWorldP->offscreenScrollRect;
    Rect *visScrollRectP = &spriteWorldP->visScrollRect;

    spriteWorldP->vertScrollRectOffset = spriteWorldP->backRect.bottom *
        (visScrollRectP->top / spriteWorldP->backRect.bottom);

    spriteWorldP->horizScrollRectOffset = spriteWorldP->backRect.right *
        (visScrollRectP->left / spriteWorldP->backRect.right);

    offscreenScrollRectP->top = visScrollRectP->top -
            spriteWorldP->vertScrollRectOffset;
    offscreenScrollRectP->bottom = visScrollRectP->bottom -
            spriteWorldP->vertScrollRectOffset;
    offscreenScrollRectP->left = visScrollRectP->left -
            spriteWorldP->horizScrollRectOffset;
    offscreenScrollRectP->right = visScrollRectP->right -
            spriteWorldP->horizScrollRectOffset;
}
