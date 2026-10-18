// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* spriteworld.c -- SpriteWorld's SpriteWorld.c, tu-map object O17: the sprite world, its update and animation. Notes: Research/notes/spriteworld.md */
#include <stddef.h>

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "spriteworld.h"
#include "burger.h"
#include "render.h"
#include "game.h"

/* SWCreateSpriteWorldFromWindow (mac _SWCreateSpriteWorldFromWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477910
short SWCreateSpriteWorldFromWindow(SpriteWorldPtr *spriteWorldP, CWindowPtr srcWindowP,
                                    Rect *worldRectP, Rect *offscreenRectP, short maxDepth)
{
    short err = noErr;
    SWFrameRec *windowFrameP, *backFrameP, *workFrameP;
    Rect tempRect, windRect;
    void *mainGDH;
    Rect *srcRectP;

    *spriteWorldP = 0;
    windowFrameP = backFrameP = workFrameP = 0;

    SetPortWindowPort(srcWindowP);

    if (worldRectP == 0)
        srcRectP = GetCWindowPortRect(srcWindowP);
    else
        srcRectP = worldRectP;

    windRect.top = srcRectP->top;
    windRect.left = srcRectP->left;
    windRect.bottom = srcRectP->bottom;
    windRect.right = srcRectP->right;

    mainGDH = (void *)GetMainDevice();

    if (offscreenRectP == 0) {
        tempRect.top = windRect.top;
        tempRect.left = windRect.left;
        tempRect.bottom = windRect.bottom;
        tempRect.right = windRect.right;
    } else {
        tempRect.top = offscreenRectP->top;
        tempRect.left = offscreenRectP->left;
        tempRect.bottom = offscreenRectP->bottom;
        tempRect.right = offscreenRectP->right;
    }

    OffsetRect(&tempRect, -tempRect.left, -tempRect.top);

    err = SWCreateWindowFrame(srcWindowP, &windowFrameP, &windRect, tempRect.bottom);

    if ((int)err == noErr)
        err = SWCreateFrame(mainGDH, &backFrameP, &tempRect, 16, kCreateGWorld);

    if ((int)err == noErr)
        err = SWCreateFrame(mainGDH, &workFrameP, &tempRect, 16, kCreateGWorld);

    if ((int)err == noErr)
        err = SWCreateSpriteWorld(mainGDH, spriteWorldP, windowFrameP,
                                  backFrameP, workFrameP, maxDepth);

    if ((int)err != noErr) {
        if (windowFrameP != 0)
            SWDisposeWindowFrame(&windowFrameP);

        if (backFrameP != 0)
            SWDisposeFrame(&backFrameP);

        if (workFrameP != 0)
            SWDisposeFrame(&workFrameP);
    }

    SWSetStickyIfError(err);

    return err;
}

/* SWExitSpriteWorld (mac _SWExitSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477af0
void SWExitSpriteWorld(void)
{
}

/* SWAddSpriteLayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477b00
void SWAddSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *newSpriteLayerP)
{
    SpriteLayerRec *tailSpriteLayerP = spriteWorldP->tailSpriteLayerP;

    if (spriteWorldP == NULL)
        SWAssertFail("SpriteWorld.c", 0x1cb);
    if (tailSpriteLayerP != NULL)
        tailSpriteLayerP->nextSpriteLayerP = newSpriteLayerP;
    else
        spriteWorldP->headSpriteLayerP = newSpriteLayerP;
    newSpriteLayerP->prevSpriteLayerP = tailSpriteLayerP;
    newSpriteLayerP->nextSpriteLayerP = NULL;
    spriteWorldP->tailSpriteLayerP = newSpriteLayerP;
}

/* SWRemoveSpriteLayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477b50
void SWRemoveSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *oldSpriteLayerP)
{
    if (spriteWorldP == NULL)
        SWAssertFail("SpriteWorld.c", 0x1e2);
    if (oldSpriteLayerP->nextSpriteLayerP != NULL)
        oldSpriteLayerP->nextSpriteLayerP->prevSpriteLayerP = oldSpriteLayerP->prevSpriteLayerP;
    else
        spriteWorldP->tailSpriteLayerP = oldSpriteLayerP->prevSpriteLayerP;
    if (oldSpriteLayerP->prevSpriteLayerP != NULL)
        oldSpriteLayerP->prevSpriteLayerP->nextSpriteLayerP = oldSpriteLayerP->nextSpriteLayerP;
    else
        spriteWorldP->headSpriteLayerP = oldSpriteLayerP->nextSpriteLayerP;
}

/* SWLockSpriteWorld. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477ba0
void SWLockSpriteWorld(SpriteWorldPtr spriteWorld)
{
    SpriteLayerPtr curSpriteLayerP;
    SWFrameRec *extraBackFrameP;

    if (spriteWorld == NULL)
        SWAssertFail("SpriteWorld.c", 0x270);
    SWLockWindowFrame(spriteWorld->windowFrameP);
    SWLockFrame(spriteWorld->workFrameP);
    SWLockFrame(spriteWorld->backFrameP);
    extraBackFrameP = spriteWorld->extraBackFrameP;
    if (extraBackFrameP != NULL)
        SWLockFrame(extraBackFrameP);
    curSpriteLayerP = spriteWorld->headSpriteLayerP;
    while (curSpriteLayerP != NULL) {
        SWLockSpriteLayer(curSpriteLayerP);
        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}

/* SWSetPortToBackground. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c10
void SWSetPortToBackground(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld == NULL)
        SWAssertFail("SpriteWorld.c", 0x2af);
    SWSetPortToFrame(spriteWorld->backFrameP);
}

/* SWSetPortToWorkArea. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c40
void SWSetPortToWorkArea(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld == NULL) {
        SWAssertFail("SpriteWorld.c", 0x2bc);
    }
    SWSetPortToFrame(spriteWorld->workFrameP);
}

/* SWSetPortToWindow (mac _SWSetPortToWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c70
void SWSetPortToWindow(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld != NULL) {
        SWSetPortToFrame(spriteWorld->windowFrameP);
        return;
    }
}

/* SWSetSpriteWorldScreenDrawProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c90
short SWSetSpriteWorldScreenDrawProc(SpriteWorldPtr spriteWorld, DrawProcPtr proc)
{
    if (spriteWorld == NULL) {
        SWAssertFail("SpriteWorld.c", 0x2fc);
    }
    if (proc == NULL) {
        SWAssertFail("SpriteWorld.c", 0x2fd);
    }
    spriteWorld->screenDrawProc = proc;
    SWSetStickyIfError(0);
    return 0;
}

/* SWSetPostEraseCallBack. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477ce0
void SWSetPostEraseCallBack(SpriteWorldPtr spriteWorld, WorldProcPtr proc)
{
    spriteWorld->postEraseCallBack = proc;
}

/* SWSetPostDrawCallBack. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477cf0
void SWSetPostDrawCallBack(SpriteWorldPtr spriteWorld, WorldProcPtr proc)
{
    spriteWorld->postDrawCallBack = proc;
}

/* SWSetSpriteWorldMaxFPS. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477d00
void SWSetSpriteWorldMaxFPS(SpriteWorldPtr spriteWorld, short maxFPS)
{
    if (spriteWorld == NULL) {
        SWAssertFail("SpriteWorld.c", 0x336);
    }
    if (maxFPS <= 0 || maxFPS > 1000)
        spriteWorld->frameInterval = 0;
    else
        spriteWorld->frameInterval = 1000 / maxFPS;
}

#define bgRed(sw)   (*(unsigned short *)((char *)(sw) + 0xdf))
#define bgGreen(sw) (*(unsigned short *)((char *)(sw) + 0xe1))
#define bgBlue(sw)  (*(unsigned short *)((char *)(sw) + 0xe3))

/* SWSetBackgroundColor (mac _SWSetBackgroundColor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477d60
void SWSetBackgroundColor(SpriteWorldPtr spriteWorldP, RGBColor *color)
{
    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 845);
    if (spriteWorldP->workFrameP == 0)
        SWAssertFail("SpriteWorld.c", 846);

    bgRed(spriteWorldP) = color->red;
    bgGreen(spriteWorldP) = color->green;
    bgBlue(spriteWorldP) = color->blue;
    spriteWorldP->backgroundValue = SWGetValueFromColor(spriteWorldP->workFrameP, color);
}

/* SWStdWorldDrawProc (mac _SWStdWorldDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477de0
void SWStdWorldDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                        Rect *srcRect, Rect *dstRect)
{
    Rect srcCopyBitsRect = *srcRect;
    Rect dstCopyBitsRect = *dstRect;

    if (dstCopyBitsRect.top < dstFrameP->frameRect.top) {
        srcCopyBitsRect.top += dstFrameP->frameRect.top - dstCopyBitsRect.top;
        dstCopyBitsRect.top = dstFrameP->frameRect.top;
        if (dstCopyBitsRect.top >= dstCopyBitsRect.bottom)
            return;
    }

    if (dstCopyBitsRect.bottom > dstFrameP->frameRect.bottom) {
        srcCopyBitsRect.bottom -= dstCopyBitsRect.bottom - dstFrameP->frameRect.bottom;
        dstCopyBitsRect.bottom = dstFrameP->frameRect.bottom;
        if (dstCopyBitsRect.bottom <= dstCopyBitsRect.top)
            return;
    }

    if (dstCopyBitsRect.left < dstFrameP->frameRect.left) {
        srcCopyBitsRect.left += dstFrameP->frameRect.left - dstCopyBitsRect.left;
        dstCopyBitsRect.left = dstFrameP->frameRect.left;
        if (dstCopyBitsRect.left >= dstCopyBitsRect.right)
            return;
    }

    if (dstCopyBitsRect.right > dstFrameP->frameRect.right) {
        srcCopyBitsRect.right -= dstCopyBitsRect.right - dstFrameP->frameRect.right;
        dstCopyBitsRect.right = dstFrameP->frameRect.right;
        if (dstCopyBitsRect.right <= dstCopyBitsRect.left)
            return;
    }

    CopyBits(srcFrameP->framePix,(BitMapPtr)dstFrameP->framePix,
             &srcCopyBitsRect, &dstCopyBitsRect, srcCopy, 0);
}

/* SWWindowMoved (mac _SWWindowMoved). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477f10
void SWWindowMoved(SpriteWorldPtr spriteWorldP)
{
    SWWindowFrameMoved(spriteWorldP->windowFrameP, &spriteWorldP->windRect);
}

/* SWWindowFrameMoved (mac _SWWindowFrameMoved). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477f30
void SWWindowFrameMoved(SWFrameRec *windowFrameP, Rect *frameRect)
{
    CGrafPtr savePort;

    if (windowFrameP == 0)
        SWAssertFail("SpriteWorld.c", 1346);
    if (!windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1347);

    GetPort(&savePort);
    SetPort(windowFrameP->framePort);

    if (windowFrameP->isFrameLocked) {
        SWUnlockWindowFrame(windowFrameP);
        SWLockWindowFrame(windowFrameP);
    }

    SetPort(savePort);
}

/* SWUpdateSpriteWorld (mac _SWUpdateSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477fa0
void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP, char updateWindow)
{
    UpdateRectStructPtr curRectStructP, nextRectStructP;
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;
    CGrafPtr holdGWorld;
    void *holdGDH;
    short curTileLayer;
    DrawProcPtr saveScreenDrawProc;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1440);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1441);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1442);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1443);

    if (!sw_depth_ok(spriteWorldP))
        return;

    saveScreenDrawProc = spriteWorldP->screenDrawProc;
    SWSetSpriteWorldScreenDrawProc(spriteWorldP, BlitPixieRectDrawProc);

    GetPort(&holdGWorld);
    holdGDH = GetGWorldDevice(holdGWorld);

    gSWCurrentSpriteWorld = spriteWorldP;
    SWIsWindowInForeground(spriteWorldP);

    SWSetPortToFrame(spriteWorldP->workFrameP);
    (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                                       spriteWorldP->workFrameP,
                                       &spriteWorldP->backFrameP->frameRect,
                                       &spriteWorldP->workFrameP->frameRect);

    if (spriteWorldP->postEraseCallBack != 0)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP != 0) {
                curSpriteP->tileDepth = curTileLayer;

                if (curSpriteP->isVisible && curSpriteP->curFrameP->isFrameLocked) {
                    gSWCurrentSpriteBeingDrawn = curSpriteP;

                    (*curSpriteP->frameDrawProc)(curSpriteP->curFrameP,
                                                 spriteWorldP->workFrameP,
                                                 &curSpriteP->curFrameP->frameRect,
                                                 &curSpriteP->destFrameRect);

                    gSWCurrentSpriteBeingDrawn = 0;
                }

                curSpriteP->deltaFrameRect.top = curSpriteP->destFrameRect.top;
                curSpriteP->deltaFrameRect.left = curSpriteP->destFrameRect.left;
                curSpriteP->deltaFrameRect.bottom = curSpriteP->destFrameRect.bottom;
                curSpriteP->deltaFrameRect.right = curSpriteP->destFrameRect.right;

                curSpriteP->oldFrameRect.top = curSpriteP->destFrameRect.top;
                curSpriteP->oldFrameRect.left = curSpriteP->destFrameRect.left;
                curSpriteP->oldFrameRect.bottom = curSpriteP->destFrameRect.bottom;
                curSpriteP->oldFrameRect.right = curSpriteP->destFrameRect.right;

                curSpriteP->needsToBeDrawn = 0;
                curSpriteP->needsToBeErased = 0;
                curSpriteP->onTop = 0;
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        if (curSpriteLayerP->layerCallBack != 0)
            (*curSpriteLayerP->layerCallBack)(spriteWorldP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (spriteWorldP->postDrawCallBack != 0)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    if (updateWindow) {
        SWSetPortToFrame(spriteWorldP->windowFrameP);

        (*spriteWorldP->screenDrawProc)(spriteWorldP->workFrameP,
                                        spriteWorldP->windowFrameP,
                                        &spriteWorldP->visScrollRect,
                                        &spriteWorldP->windowFrameP->frameRect);
    }

    nextRectStructP = spriteWorldP->headUpdateRectP;
    while (nextRectStructP != 0) {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        DeallocAPointer(curRectStructP);
    }
    spriteWorldP->headUpdateRectP = 0;

    spriteWorldP->numTilesChanged = 0;

    SWSetSpriteWorldScreenDrawProc(spriteWorldP, saveScreenDrawProc);

    gSWCurrentSpriteWorld = 0;

    SetPort(holdGWorld);
}


/* SWProcessSpriteWorld (mac _SWProcessSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004781f0
void SWProcessSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    unsigned long long curMicroseconds;
    unsigned long millisecDifference;
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP, nextSpriteP;
    SWFrameRec *newFrameP;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1618);

    gSWCurrentSpriteWorld = spriteWorldP;

    curMicroseconds = TimerGetMicroseconds();
    millisecDifference = (curMicroseconds - spriteWorldP->lastMicroseconds) / 1000;

    if (millisecDifference > 0) {
        spriteWorldP->runningTimeCount += millisecDifference;
        spriteWorldP->lastMicroseconds = curMicroseconds;
    }

    if (spriteWorldP->runningTimeCount - spriteWorldP->timeOfLastFrame >=
            spriteWorldP->frameInterval) {
        spriteWorldP->frameHasOccurred = 1;
        spriteWorldP->timeOfLastFrame = spriteWorldP->runningTimeCount;
    } else {
        spriteWorldP->frameHasOccurred = 0;
        return;
    }

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != 0)
        SWFindSpritesToBeRemoved(spriteWorldP);

    spriteWorldP->thereAreNonScrollingLayers = 0;

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        if (curSpriteLayerP->layerIsNonScrolling) {
            spriteWorldP->thereAreNonScrollingLayers = 1;
        } else if (!curSpriteLayerP->isPaused) {
            curSpriteP = curSpriteLayerP->headSpriteP;

            while (curSpriteP != 0) {
                nextSpriteP = curSpriteP->nextSpriteP;

                if (curSpriteP->curFrameP != 0) {
                    if (!curSpriteP->curFrameP->isFrameLocked)
                        SWLockFrame(curSpriteP->curFrameP);

                    if (curSpriteP->frameTimeInterval >= 0 &&
                        (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastFrameChange) >=
                            curSpriteP->frameTimeInterval &&
                        curSpriteP->curFrameP->isFrameLocked)
                    {
                        long currentFrameIndex;

                        curSpriteP->timeOfLastFrameChange = spriteWorldP->runningTimeCount;

                        currentFrameIndex  = curSpriteP->curFrameIndex;
                        currentFrameIndex += curSpriteP->frameAdvance;

                        if (currentFrameIndex < curSpriteP->firstFrameIndex) {
                            if (curSpriteP->frameAdvanceMode == kSWWrapAroundMode) {
                                currentFrameIndex = curSpriteP->lastFrameIndex;
                            } else {
                                curSpriteP->frameAdvance = -curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                            }
                        } else if (currentFrameIndex > curSpriteP->lastFrameIndex) {
                            if (curSpriteP->frameAdvanceMode == kSWWrapAroundMode) {
                                currentFrameIndex = curSpriteP->firstFrameIndex;
                            } else {
                                curSpriteP->frameAdvance = -curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                                currentFrameIndex += curSpriteP->frameAdvance;
                            }
                        }

                        curSpriteP->curFrameIndex = currentFrameIndex;

                        if (curSpriteP->frameChangeProc != 0) {
                            newFrameP = curSpriteP->frameArray[currentFrameIndex];

                            (*curSpriteP->frameChangeProc)(curSpriteP, newFrameP,
                                    &curSpriteP->curFrameIndex);

                            if (curSpriteP->curFrameIndex < 0) {
                                curSpriteP->curFrameIndex = 0;
                            } else if (curSpriteP->curFrameIndex >= curSpriteP->maxFrames) {
                                curSpriteP->curFrameIndex = curSpriteP->maxFrames - 1;
                            }
                        }

                        newFrameP = curSpriteP->frameArray[curSpriteP->curFrameIndex];

                        if (curSpriteP->curFrameP != newFrameP)
                            SWSetCurrentFrameIndex(curSpriteP, curSpriteP->curFrameIndex);
                    }

                    if (curSpriteP->moveTimeInterval >= 0 &&
                        (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastMove) >=
                            curSpriteP->moveTimeInterval)
                    {
                        curSpriteP->timeOfLastMove = spriteWorldP->runningTimeCount;

                        if (curSpriteP->spriteMoveProc != 0)
                            (*curSpriteP->spriteMoveProc)(curSpriteP);
                    }
                }

                curSpriteP = nextSpriteP;
            }
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    SWIsWindowInForeground(spriteWorldP);

    gSWCurrentSpriteWorld = 0;
}



SpriteWorldRec *gSWCurrentSpriteWorld;                   /* 0x008706f0 */

/* SWAnimateSpriteWorld (mac _SWAnimateSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478480
void SWAnimateSpriteWorld(SpriteWorldRec *spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;
    SpritePtr headActiveSpriteP = 0;
    SpritePtr headIdleSpriteP = 0;
    SpritePtr curActiveSpriteP = 0;
    SpritePtr curIdleSpriteP = 0;
    Rect screenDestRect;
    Rect oldScreenDestRect;
    short curTileLayer;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 1984);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1985);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1986);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 1987);

    if (!sw_depth_ok(spriteWorldP))
        return;

    gSWCurrentSpriteWorld = spriteWorldP;

    if (!spriteWorldP->frameHasOccurred)
        return;

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != 0)
        SWAddSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);

    SWSetPortToFrame(spriteWorldP->workFrameP);

    MyProfileEnter(17);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP) {
                curSpriteP->tileDepth = curTileLayer;

                if (curSpriteP->curFrameP->isFrameLocked) {
                    if ((curSpriteP->needsToBeDrawn && curSpriteP->isVisible) ||
                        curSpriteP->needsToBeErased)
                    {
                        if (headActiveSpriteP == 0)
                            headActiveSpriteP = curSpriteP;

                        if (curActiveSpriteP != 0)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                        curActiveSpriteP = curSpriteP;

                        if (curSpriteP->justBecameVisible) {
                            if (!curSpriteP->needsToBeErased) {
                                curSpriteP->oldFrameRect.top = curSpriteP->destFrameRect.top;
                                curSpriteP->oldFrameRect.left = curSpriteP->destFrameRect.left;
                                curSpriteP->oldFrameRect.bottom = curSpriteP->destFrameRect.bottom;
                                curSpriteP->oldFrameRect.right = curSpriteP->destFrameRect.right;
                            }

                            curSpriteP->justBecameVisible = 0;
                        }

                        curSpriteP->srD3 = 0;

                        curSpriteP->deltaFrameRect.top =
                            SW_MIN(curSpriteP->oldFrameRect.top, curSpriteP->destFrameRect.top);
                        curSpriteP->deltaFrameRect.left =
                            SW_MIN(curSpriteP->oldFrameRect.left, curSpriteP->destFrameRect.left);
                        curSpriteP->deltaFrameRect.bottom =
                            SW_MAX(curSpriteP->oldFrameRect.bottom, curSpriteP->destFrameRect.bottom);
                        curSpriteP->deltaFrameRect.right =
                            SW_MAX(curSpriteP->oldFrameRect.right, curSpriteP->destFrameRect.right);
                        {
                            short temp;

                            curSpriteP->deltaFrameRect.left &=
                                    (spriteWorldP->workFrameP->leftAlignFactor);

                            temp = curSpriteP->deltaFrameRect.right &
                                spriteWorldP->workFrameP->rightAlignFactor;
                            if (temp != 0)
                            {
                                curSpriteP->deltaFrameRect.right +=
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

                        if ((curSpriteP->deltaFrameRect.right - curSpriteP->deltaFrameRect.left) *
                            (curSpriteP->deltaFrameRect.bottom - curSpriteP->deltaFrameRect.top) >
                            (curSpriteP->oldFrameRect.right - curSpriteP->oldFrameRect.left) *
                            (curSpriteP->oldFrameRect.bottom - curSpriteP->oldFrameRect.top) * 2)
                        {
                            curSpriteP->srD3 = 1;
                        }

                        if (!curSpriteP->onTop || curSpriteP->needsToBeErased) {
                            if (curSpriteP->oldFrameRect.left <=
                                    spriteWorldP->workFrameP->frameRect.right &&
                                curSpriteP->oldFrameRect.right >=
                                    spriteWorldP->workFrameP->frameRect.left &&
                                curSpriteP->oldFrameRect.bottom >=
                                    spriteWorldP->workFrameP->frameRect.top &&
                                curSpriteP->oldFrameRect.top <=
                                    spriteWorldP->workFrameP->frameRect.bottom)
                            {
                                (*spriteWorldP->offscreenDrawProc)(
                                        spriteWorldP->backFrameP,
                                        spriteWorldP->workFrameP,
                                        &curSpriteP->oldFrameRect,
                                        &curSpriteP->oldFrameRect);
                            }
                        }
                    }
                    else if (curSpriteP->isVisible && !curSpriteP->onTop)
                    {
                        if (headIdleSpriteP == 0)
                            headIdleSpriteP = curSpriteP;

                        if (curIdleSpriteP != 0)
                            curIdleSpriteP->nextIdleSpriteP = curSpriteP;

                        curIdleSpriteP = curSpriteP;
                    }
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    MyProfileExit(17);

    if (curActiveSpriteP != 0)
        curActiveSpriteP->nextActiveSpriteP = 0;

    if (curIdleSpriteP != 0)
        curIdleSpriteP->nextIdleSpriteP = 0;

    if (spriteWorldP->postEraseCallBack != 0)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    MyProfileEnter(18);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != 0) {
            if (curSpriteP->isVisible) {
                if (curSpriteP->needsToBeDrawn) {
                    if (curSpriteP->destFrameRect.left <=
                            spriteWorldP->workFrameP->frameRect.right &&
                        curSpriteP->destFrameRect.right >=
                            spriteWorldP->workFrameP->frameRect.left &&
                        curSpriteP->destFrameRect.bottom >=
                            spriteWorldP->workFrameP->frameRect.top &&
                        curSpriteP->destFrameRect.top <=
                            spriteWorldP->workFrameP->frameRect.bottom)
                    {
                        gSWCurrentSpriteBeingDrawn = curSpriteP;

                        (*curSpriteP->frameDrawProc)(
                                curSpriteP->curFrameP,
                                spriteWorldP->workFrameP,
                                &curSpriteP->curFrameP->frameRect,
                                &curSpriteP->destFrameRect);

                        gSWCurrentSpriteBeingDrawn = 0;
                    }
                }
                else
                {
                    SWCheckIdleSpriteOverlap(spriteWorldP, curSpriteP, headActiveSpriteP);
                }
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        if (curSpriteLayerP->layerCallBack != 0)
            (*curSpriteLayerP->layerCallBack)(spriteWorldP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    MyProfileExit(18);

    if (spriteWorldP->postDrawCallBack != 0)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    MyProfileEnter(19);

    SWSetPortToFrame(spriteWorldP->windowFrameP);

    curSpriteP = headActiveSpriteP;

    if (spriteWorldP->fastBlit) {
        while (curSpriteP != 0 && AreWeFrontProcess()) {
            if (curSpriteP->srD3) {
                screenDestRect.top = curSpriteP->destFrameRect.top;
                screenDestRect.left = curSpriteP->destFrameRect.left;
                screenDestRect.bottom = curSpriteP->destFrameRect.bottom;
                screenDestRect.right = curSpriteP->destFrameRect.right;

                oldScreenDestRect.top = curSpriteP->oldFrameRect.top;
                oldScreenDestRect.left = curSpriteP->oldFrameRect.left;
                oldScreenDestRect.bottom = curSpriteP->oldFrameRect.bottom;
                oldScreenDestRect.right = curSpriteP->oldFrameRect.right;

                oldScreenDestRect.left += spriteWorldP->windRect.left;
                oldScreenDestRect.right += spriteWorldP->windRect.left;
                oldScreenDestRect.top += spriteWorldP->windRect.top;
                oldScreenDestRect.bottom += spriteWorldP->windRect.top;
            }
            else
            {
                screenDestRect.top = curSpriteP->deltaFrameRect.top;
                screenDestRect.left = curSpriteP->deltaFrameRect.left;
                screenDestRect.bottom = curSpriteP->deltaFrameRect.bottom;
                screenDestRect.right = curSpriteP->deltaFrameRect.right;
            }

            screenDestRect.left += spriteWorldP->windRect.left;
            screenDestRect.right += spriteWorldP->windRect.left;
            screenDestRect.top += spriteWorldP->windRect.top;
            screenDestRect.bottom += spriteWorldP->windRect.top;

            if (screenDestRect.left <= spriteWorldP->windowFrameP->frameRect.right &&
                screenDestRect.right >= spriteWorldP->windowFrameP->frameRect.left &&
                screenDestRect.bottom >= spriteWorldP->windowFrameP->frameRect.top &&
                screenDestRect.top <= spriteWorldP->windowFrameP->frameRect.bottom)
            {
                if (curSpriteP->srD3) {
                    BlitPixieRectDrawProc(
                            spriteWorldP->workFrameP,
                            spriteWorldP->windowFrameP,
                            &curSpriteP->destFrameRect,
                            &screenDestRect);
                }
                else
                {
                    BlitPixieRectDrawProc(
                            spriteWorldP->workFrameP,
                            spriteWorldP->windowFrameP,
                            &curSpriteP->deltaFrameRect,
                            &screenDestRect);
                }
            }

            if (curSpriteP->srD3) {
                if (oldScreenDestRect.left <= spriteWorldP->windowFrameP->frameRect.right &&
                    oldScreenDestRect.right >= spriteWorldP->windowFrameP->frameRect.left &&
                    oldScreenDestRect.bottom >= spriteWorldP->windowFrameP->frameRect.top &&
                    oldScreenDestRect.top <= spriteWorldP->windowFrameP->frameRect.bottom)
                {
                    BlitPixieRectDrawProc(
                            spriteWorldP->workFrameP,
                            spriteWorldP->windowFrameP,
                            &curSpriteP->oldFrameRect,
                            &oldScreenDestRect);
                }
            }

            curSpriteP->deltaFrameRect.top = curSpriteP->destFrameRect.top;
            curSpriteP->deltaFrameRect.left = curSpriteP->destFrameRect.left;
            curSpriteP->deltaFrameRect.bottom = curSpriteP->destFrameRect.bottom;
            curSpriteP->deltaFrameRect.right = curSpriteP->destFrameRect.right;

            curSpriteP->oldFrameRect.top = curSpriteP->destFrameRect.top;
            curSpriteP->oldFrameRect.left = curSpriteP->destFrameRect.left;
            curSpriteP->oldFrameRect.bottom = curSpriteP->destFrameRect.bottom;
            curSpriteP->oldFrameRect.right = curSpriteP->destFrameRect.right;

            curSpriteP->needsToBeDrawn = 0;
            curSpriteP->needsToBeErased = 0;
            curSpriteP->onTop = 0;

            curSpriteP = curSpriteP->nextActiveSpriteP;
        }
    }

    spriteWorldP->numTilesChanged = 0;

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != 0)
        SWRemoveSpriteLayer(spriteWorldP, spriteWorldP->deadSpriteLayerP);

    MyProfileExit(19);

    gSWCurrentSpriteWorld = 0;
}

/* The srcSectRect copy goes through a POINTER local -- `mov edi,[ebx+0x50] / add
   edi,0x22' then four movzx/mov word pairs. SW_MIN/SW_MAX stay macros: they
   double-evaluate, and the exe re-reads deltaFrameRect.left and .bottom for the pair. */

/* SWCheckIdleSpriteOverlap (mac _SWCheckIdleSpriteOverlap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478bb0
void SWCheckIdleSpriteOverlap(SpriteWorldRec *spriteWorldP, SpritePtr idleSpriteP,
                              SpritePtr headActiveSpriteP)
{
    SpritePtr activeSpriteP = headActiveSpriteP;
    Rect srcSectRect, dstSectRect;
    Rect *frameRectP;

    while (activeSpriteP != 0) {
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

            frameRectP = &idleSpriteP->curFrameP->frameRect;
            srcSectRect.top = frameRectP->top;
            srcSectRect.left = frameRectP->left;
            srcSectRect.bottom = frameRectP->bottom;
            srcSectRect.right = frameRectP->right;

            srcSectRect.left += (dstSectRect.left - idleSpriteP->destFrameRect.left);
            srcSectRect.top += (dstSectRect.top - idleSpriteP->destFrameRect.top);
            srcSectRect.right -= (idleSpriteP->destFrameRect.right - dstSectRect.right);
            srcSectRect.bottom -= (idleSpriteP->destFrameRect.bottom - dstSectRect.bottom);

            gSWCurrentSpriteBeingDrawn = idleSpriteP;

            (*idleSpriteP->frameDrawProc)(idleSpriteP->curFrameP,
                                          spriteWorldP->workFrameP,
                                          &srcSectRect,
                                          &dstSectRect);

            gSWCurrentSpriteBeingDrawn = 0;
        }

        activeSpriteP = activeSpriteP->nextActiveSpriteP;
    }
}

#define kSWRemoveSprite             1
#define kSWRemoveAndDisposeSprite   2

/* SWFindSpritesToBeRemoved (mac _SWFindSpritesToBeRemoved). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478cf0
void SWFindSpritesToBeRemoved(SpriteWorldPtr spriteWorldP)
{
    SpritePtr curSpriteP, nextSpriteP;

    curSpriteP = spriteWorldP->deadSpriteLayerP->headSpriteP;

    while (curSpriteP != 0) {
        nextSpriteP = curSpriteP->nextSpriteP;

        if (curSpriteP->spriteRemoval == kSWDontRemoveSprite)
            SWAssertFail("SpriteWorld.c", 2613);

        if (curSpriteP->needsToBeErased == 0) {
            SWRemoveSprite(curSpriteP);

            if (curSpriteP->spriteRemoval == kSWRemoveAndDisposeSprite)
                SWDisposeSprite(&curSpriteP);
            else
                curSpriteP->spriteRemoval = kSWDontRemoveSprite;
        }

        curSpriteP = nextSpriteP;
    }
}

/* SWFlagRectAsChanged (mac _SWFlagRectAsChanged). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478d70
short SWFlagRectAsChanged(SpriteWorldPtr spriteWorldP, Rect *theChangedRect)
{
    UpdateRectStructPtr curRectStructP, newUpdateRectP;
    short temp;
    short err = 0;

    theChangedRect->left &= spriteWorldP->workFrameP->leftAlignFactor;

    temp = theChangedRect->right & spriteWorldP->workFrameP->rightAlignFactor;
    if (temp != 0)
        theChangedRect->right += (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;

    newUpdateRectP = (UpdateRectStructPtr)AllocAPointer(sizeof(UpdateRectStruct));

    if (newUpdateRectP != 0) {
        newUpdateRectP->updateRect.top = theChangedRect->top;
        newUpdateRectP->updateRect.left = theChangedRect->left;
        newUpdateRectP->updateRect.bottom = theChangedRect->bottom;
        newUpdateRectP->updateRect.right = theChangedRect->right;
        newUpdateRectP->nextRectStructP = 0;

        if (spriteWorldP->headUpdateRectP == 0) {
            spriteWorldP->headUpdateRectP = newUpdateRectP;
        } else {
            curRectStructP = spriteWorldP->headUpdateRectP;

            while (curRectStructP->nextRectStructP != 0)
                curRectStructP = curRectStructP->nextRectStructP;

            curRectStructP->nextRectStructP = newUpdateRectP;
        }
    } else {
        err = MemError();
    }

    return err;
}

/* SWIsWindowInForeground (mac _SWIsWindowInForeground). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478e20
char SWIsWindowInForeground(SpriteWorldPtr spriteWorldP)
{
    spriteWorldP->fastBlit = 1;

    return spriteWorldP->fastBlit;
}

/* AreWeFrontProcess (mac _AreWeFrontProcess). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478e40
char AreWeFrontProcess(void)
{
    return 1;
}

/* SWHideAllSprites (mac _SWHideAllSprites). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478e50
void SWHideAllSprites(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteWorld.c", 2847);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 2848);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 2849);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SWAssertFail("SpriteWorld.c", 2850);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != 0) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != 0) {
            if (curSpriteP->curFrameP != 0 && curSpriteP->isVisible)
                SWSetSpriteVisible(curSpriteP, 0);

            curSpriteP = curSpriteP->nextSpriteP;
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}
