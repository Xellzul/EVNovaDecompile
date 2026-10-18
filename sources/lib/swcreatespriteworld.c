// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreatespriteworld.c -- SWEnterSpriteWorld (SpriteWorld.c's order) and SWCreateSpriteWorld (win 0x00477640, donor SpriteWorld.c line 117). Notes: Research/notes/swcreatespriteworld.md */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"

/* backgroundColor is three shorts at +0xdf, +0xe1, +0xe3 -- ODD offsets, which x86
   natural alignment cannot produce, so it cannot be declared as a member here.
   Reached the way spriteworld.c's SWSetBackgroundColor reaches the same three. */
#define backgroundRed(sw)   (*(unsigned short *)((char *)(sw) + 0xdf))
#define backgroundGreen(sw) (*(unsigned short *)((char *)(sw) + 0xe1))
#define backgroundBlue(sw)  (*(unsigned short *)((char *)(sw) + 0xe3))

/* SWEnterSpriteWorld (mac _SWEnterSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477630
int SWEnterSpriteWorld(void)
{
    SWSetStickyIfError(0);
    return 0;
}

/* SWCreateSpriteWorld (mac _SWCreateSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477640
short SWCreateSpriteWorld(void *mainGDH, SpriteWorldRec **spriteWorldP,
                          SWFrameRec *windowFrameP, SWFrameRec *backFrameP,
                          SWFrameRec *workFrameP, short maxDepth)
{
    short err;
    SpriteWorldRec *tempSpriteWorldP;

    err = noErr;
    *spriteWorldP = 0;

    tempSpriteWorldP = (SpriteWorldRec *)AllocAPointerClear(sizeof(SpriteWorldRec));
    if (tempSpriteWorldP == 0)
        err = memFullErr;

    if ((int)err == noErr) {
        err = SWCreateSpriteLayer(&tempSpriteWorldP->deadSpriteLayerP);

        if ((int)err != noErr)
            DeallocAPointer(tempSpriteWorldP);
    }

    if ((int)err == noErr) {
        tempSpriteWorldP->pixelDepth = 16;

        tempSpriteWorldP->headSpriteLayerP = 0;
        tempSpriteWorldP->tailSpriteLayerP = 0;

        tempSpriteWorldP->mainSWGDH = mainGDH;
        tempSpriteWorldP->windowFrameP = windowFrameP;
        tempSpriteWorldP->extraBackFrameP = 0;
        tempSpriteWorldP->backFrameP = backFrameP;
        tempSpriteWorldP->workFrameP = workFrameP;
        tempSpriteWorldP->offscreenDrawProc = SWStdWorldDrawProc;
        tempSpriteWorldP->screenDrawProc = SWStdWorldDrawProc;
        tempSpriteWorldP->doubleRectDrawProc = 0;
        tempSpriteWorldP->postEraseCallBack = 0;
        tempSpriteWorldP->postDrawCallBack = 0;

        tempSpriteWorldP->windRect.top = windowFrameP->frameRect.top;
        tempSpriteWorldP->windRect.left = windowFrameP->frameRect.left;
        tempSpriteWorldP->windRect.bottom = windowFrameP->frameRect.bottom;
        tempSpriteWorldP->windRect.right = windowFrameP->frameRect.right;

        tempSpriteWorldP->backRect.top = backFrameP->frameRect.top;
        tempSpriteWorldP->backRect.left = backFrameP->frameRect.left;
        tempSpriteWorldP->backRect.bottom = backFrameP->frameRect.bottom;
        tempSpriteWorldP->backRect.right = backFrameP->frameRect.right;

        tempSpriteWorldP->originalWindRect.top = windowFrameP->frameRect.top;
        tempSpriteWorldP->originalWindRect.left = windowFrameP->frameRect.left;
        tempSpriteWorldP->originalWindRect.bottom = windowFrameP->frameRect.bottom;
        tempSpriteWorldP->originalWindRect.right = windowFrameP->frameRect.right;

        tempSpriteWorldP->originalBackRect.top = backFrameP->frameRect.top;
        tempSpriteWorldP->originalBackRect.left = backFrameP->frameRect.left;
        tempSpriteWorldP->originalBackRect.bottom = backFrameP->frameRect.bottom;
        tempSpriteWorldP->originalBackRect.right = backFrameP->frameRect.right;

        tempSpriteWorldP->headUpdateRectP = 0;

        tempSpriteWorldP->scrollRectMoveBounds.left = 0;
        tempSpriteWorldP->scrollRectMoveBounds.top = 0;
        tempSpriteWorldP->scrollRectMoveBounds.right = 32767;
        tempSpriteWorldP->scrollRectMoveBounds.bottom = 32767;
        tempSpriteWorldP->horizScrollRectOffset = 0;
        tempSpriteWorldP->vertScrollRectOffset = 0;
        tempSpriteWorldP->horizScrollDelta = 0;
        tempSpriteWorldP->vertScrollDelta = 0;
        tempSpriteWorldP->worldMoveProc = 0;
        tempSpriteWorldP->thereAreNonScrollingLayers = 0;
        tempSpriteWorldP->fastBlit = 1;

        tempSpriteWorldP->visScrollRect.top = windowFrameP->frameRect.top;
        tempSpriteWorldP->visScrollRect.left = windowFrameP->frameRect.left;
        tempSpriteWorldP->visScrollRect.bottom = windowFrameP->frameRect.bottom;
        tempSpriteWorldP->visScrollRect.right = windowFrameP->frameRect.right;

        OffsetRect(&tempSpriteWorldP->visScrollRect,
                   -tempSpriteWorldP->visScrollRect.left,
                   -tempSpriteWorldP->visScrollRect.top);

        tempSpriteWorldP->offscreenScrollRect.top = tempSpriteWorldP->visScrollRect.top;
        tempSpriteWorldP->offscreenScrollRect.left = tempSpriteWorldP->visScrollRect.left;
        tempSpriteWorldP->offscreenScrollRect.bottom = tempSpriteWorldP->visScrollRect.bottom;
        tempSpriteWorldP->offscreenScrollRect.right = tempSpriteWorldP->visScrollRect.right;

        tempSpriteWorldP->oldVisScrollRect.top = tempSpriteWorldP->visScrollRect.top;
        tempSpriteWorldP->oldVisScrollRect.left = tempSpriteWorldP->visScrollRect.left;
        tempSpriteWorldP->oldVisScrollRect.bottom = tempSpriteWorldP->visScrollRect.bottom;
        tempSpriteWorldP->oldVisScrollRect.right = tempSpriteWorldP->visScrollRect.right;

        tempSpriteWorldP->tileLayerArray = 0;
        tempSpriteWorldP->tilingCache = 0;
        tempSpriteWorldP->changedTiles = 0;
        tempSpriteWorldP->tilingIsInitialized = 0;
        tempSpriteWorldP->tilingIsOn = 0;
        tempSpriteWorldP->numTilesChanged = 0;
        tempSpriteWorldP->tileChangeProc = 0;
        tempSpriteWorldP->tileMaskDrawProc = SWStdSpriteDrawProc;
        tempSpriteWorldP->partialMaskDrawProc = SWStdSpriteDrawProc;

        tempSpriteWorldP->frameHasOccurred = 0;
        tempSpriteWorldP->frameInterval = 0;

        tempSpriteWorldP->lastMicroseconds = 0;
        tempSpriteWorldP->runningTimeCount = 0;

        backgroundRed(tempSpriteWorldP) = 0;
        backgroundGreen(tempSpriteWorldP) = 0;
        backgroundBlue(tempSpriteWorldP) = 0;
        tempSpriteWorldP->backgroundValue = 0;

        SWClearStickyError();

        *spriteWorldP = tempSpriteWorldP;
    }

    return err;
}
