// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreatesprite.c -- SWCreateSprite (win 0x004744a0, donor line 56). Notes: Research/notes/swcreatesprite.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"



/* SWCreateSprite (mac _SWCreateSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004744a0
short SWCreateSprite(SpritePtr *newSpriteP, void *spriteStorageP, short maxFrames)
{
    short err = 0;
    SpritePtr tempSpriteP;
    long frameArraySize;

    *newSpriteP = 0;

    tempSpriteP = (spriteStorageP == 0) ? (SpritePtr)AllocAPointer(sizeof(SpriteRec))
                                        : (SpritePtr)spriteStorageP;

    if (tempSpriteP != 0) {
        frameArraySize = (long)maxFrames * sizeof(FramePtr);

        tempSpriteP->frameArray = (FramePtr *)AllocAPointerClear(frameArraySize);

        if (tempSpriteP->frameArray != 0) {
            tempSpriteP->parentSpriteLayerP = 0;
            tempSpriteP->nextSpriteP = 0;
            tempSpriteP->prevSpriteP = 0;

            tempSpriteP->isVisible = 1;
            tempSpriteP->needsToBeDrawn = 1;
            tempSpriteP->needsToBeErased = 0;
            tempSpriteP->justBecameVisible = 0;
            tempSpriteP->onTop = 0;
            tempSpriteP->destFrameRect.left = 0;
            tempSpriteP->destFrameRect.top = 0;
            tempSpriteP->destFrameRect.right = 0;
            tempSpriteP->destFrameRect.bottom = 0;
            tempSpriteP->oldFrameRect.left = 0;
            tempSpriteP->oldFrameRect.top = 0;
            tempSpriteP->oldFrameRect.right = 0;
            tempSpriteP->oldFrameRect.bottom = 0;
            tempSpriteP->deltaFrameRect.left = 0;
            tempSpriteP->deltaFrameRect.top = 0;
            tempSpriteP->deltaFrameRect.right = 0;
            tempSpriteP->deltaFrameRect.bottom = 0;
            tempSpriteP->frameDrawProc = SWStdSpriteDrawProc;
            tempSpriteP->tileDepth = 1;

            tempSpriteP->destOffscreenRect.left = 0;
            tempSpriteP->destOffscreenRect.top = 0;
            tempSpriteP->destOffscreenRect.right = 0;
            tempSpriteP->destOffscreenRect.bottom = 0;
            tempSpriteP->oldRectIsVisible = 0;

            tempSpriteP->curFrameP = 0;
            tempSpriteP->numFrames = 0;
            tempSpriteP->maxFrames = maxFrames;
            tempSpriteP->frameTimeInterval = -1;
            tempSpriteP->timeOfLastFrameChange = 0;
            tempSpriteP->frameAdvance = 1;
            tempSpriteP->curFrameIndex = 0;
            tempSpriteP->firstFrameIndex = 0;
            tempSpriteP->lastFrameIndex = 0;
            tempSpriteP->frameChangeProc = 0;

            tempSpriteP->moveTimeInterval = 0;
            tempSpriteP->timeOfLastMove = 0;
            tempSpriteP->horizMoveDelta = 0;
            tempSpriteP->vertMoveDelta = 0;
            tempSpriteP->moveBoundsRect.left = 0;
            tempSpriteP->moveBoundsRect.top = 0;
            tempSpriteP->moveBoundsRect.right = 0;
            tempSpriteP->moveBoundsRect.bottom = 0;
            tempSpriteP->spriteMoveProc = SWTempSpriteMoveProc;
            tempSpriteP->realSpriteMoveProc = 0;

            tempSpriteP->spriteCollideProc = 0;

            tempSpriteP->sharedPictGWorld = 0;
            tempSpriteP->sharedMaskGWorld = 0;
            tempSpriteP->spriteRemoval = kSWDontRemoveSprite;
            tempSpriteP->frameAdvanceMode = kSWWrapAroundMode;

            tempSpriteP->scaledWidth = -1;
            tempSpriteP->scaledHeight = -1;

            tempSpriteP->colorRGB.red = 0;
            tempSpriteP->colorRGB.green = 0;
            tempSpriteP->colorRGB.blue = 0;
            tempSpriteP->colorValue = 0;

            tempSpriteP->flippedHorizontal = 0;
            tempSpriteP->flippedVertical = 0;

            tempSpriteP->userData = 0;

            tempSpriteP->translucency = 0;
            tempSpriteP->tintRed = 0;
            tempSpriteP->tintGreen = 0;
            tempSpriteP->tintBlue = 0;

            tempSpriteP->rleDataH = 0;
            tempSpriteP->collisionsDisabled = 0;
            tempSpriteP->ownsRLEData = 0;
            tempSpriteP->isEmptySprite = 0;
            tempSpriteP->srD3 = 0;

            *newSpriteP = tempSpriteP;
        } else {
            err = memFullErr;

            DeallocAPointer(tempSpriteP);
        }
    } else {
        err = memFullErr;
    }

    SWSetStickyIfError(err);
    return err;
}
