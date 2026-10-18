// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcloneframesfrommaster.c -- SWCloneFramesFromMaster (win 0x00475f70). Notes: Research/notes/swcloneframesfrommaster.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"


/* Leading members only; the layout of record is macport.h's SpriteRec.
   +0xaa is a short this function zeroes and nothing else here reads, so it is
   left unnamed rather than guessed -- it falls in the +0xa4..+0xb3 run that
   SpriteRec also leaves unresolved. */

/* SWCloneFramesFromMaster (mac _SWCloneFramesFromMaster). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475f70
short SWCloneFramesFromMaster(SpritePtr dstSpriteP, SpritePtr masterSpriteP)
{
    short i;

    if (masterSpriteP == 0 || dstSpriteP == 0)
        return -1;

    if (masterSpriteP->frameArray == 0)
        return -1;

    if (dstSpriteP->ownsRLEData)
        return -1;

    if (!masterSpriteP->frameArray[0]->isFrameLocked)
        SWLockSprite(masterSpriteP);

    if (!dstSpriteP->isEmptySprite) {
        if (dstSpriteP->frameArray != 0) {
            for (i = 0; i < dstSpriteP->maxFrames; i++) {
                if (dstSpriteP->frameArray[i] != 0)
                    dstSpriteP->frameArray[i]->useCount--;
            }

            DeallocAPointer(dstSpriteP->frameArray);
            dstSpriteP->frameArray = 0;
        }

        dstSpriteP->isEmptySprite = 1;
    }

    dstSpriteP->frameArray = masterSpriteP->frameArray;
    dstSpriteP->sharedPictGWorld = masterSpriteP->sharedPictGWorld;
    dstSpriteP->sharedMaskGWorld = masterSpriteP->sharedMaskGWorld;
    dstSpriteP->numFrames = masterSpriteP->numFrames;
    dstSpriteP->maxFrames = masterSpriteP->maxFrames;
    dstSpriteP->rleDataH = masterSpriteP->rleDataH;

    if (dstSpriteP->curFrameIndex < dstSpriteP->numFrames)
        SWSetCurrentFrameIndex(dstSpriteP, dstSpriteP->curFrameIndex);
    else
        SWSetCurrentFrameIndex(dstSpriteP, 0);

    if (!dstSpriteP->curFrameP->isFrameLocked)
        SWLockSprite(dstSpriteP);

    dstSpriteP->tintMode = 0;

    return 0;
}
