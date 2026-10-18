// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* spritelayer.c -- SpriteWorld's SpriteLayer.c, tu-map object O16: sprite layers. Notes: Research/notes/spritelayer.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#define kSpriteAlreadyInLayer   115

/* SWAddSprite (mac _SWAddSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477140
short SWAddSprite(SpriteLayerRec *spriteLayerP, SpriteRec *newSpriteP)
{
    SpriteRec *tailSpriteP = spriteLayerP->tailSpriteP;
    short err = 0;

    if (spriteLayerP == 0)
        SWAssertFail("SpriteLayer.c", 109);
    if (newSpriteP == 0)
        SWAssertFail("SpriteLayer.c", 110);

    if (newSpriteP->parentSpriteLayerP != 0)
        err = kSpriteAlreadyInLayer;

    if ((int)err == noErr) {
        if (tailSpriteP != 0)
            tailSpriteP->nextSpriteP = newSpriteP;
        else
            spriteLayerP->headSpriteP = newSpriteP;

        newSpriteP->prevSpriteP = tailSpriteP;
        newSpriteP->nextSpriteP = 0;

        spriteLayerP->tailSpriteP = newSpriteP;

        newSpriteP->needsToBeDrawn = 1;

        newSpriteP->parentSpriteLayerP = spriteLayerP;
    }

    SWSetStickyIfError(err);
    return err;
}

#define kNilSpriteLayerErr  114

/* SWRemoveSprite (mac _SWRemoveSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004771c0
short SWRemoveSprite(SpriteRec *oldSpriteP)
{
    SpriteLayerRec *spriteLayerP;
    short err;

    err = 0;
    if (oldSpriteP == 0)
        SWAssertFail("SpriteLayer.c", 153);

    spriteLayerP = oldSpriteP->parentSpriteLayerP;
    if (spriteLayerP == 0)
        err = kNilSpriteLayerErr;

    if ((int)err == noErr) {
        if (oldSpriteP->nextSpriteP != 0)
            oldSpriteP->nextSpriteP->prevSpriteP = oldSpriteP->prevSpriteP;
        else
            spriteLayerP->tailSpriteP = oldSpriteP->prevSpriteP;

        if (oldSpriteP->prevSpriteP != 0)
            oldSpriteP->prevSpriteP->nextSpriteP = oldSpriteP->nextSpriteP;
        else
            spriteLayerP->headSpriteP = oldSpriteP->nextSpriteP;

        oldSpriteP->parentSpriteLayerP = 0;
    }

    SWSetStickyIfError(err);
    return err;
}

/* The fork's extra SpriteLayerRec member: this function stores its proc argument to
   +0x10 and does nothing else, and SWCreateSpriteLayer is what clears it. */

/* SWSetLayerCallBack (mac _SWSetLayerCallBack). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477240
void SWSetLayerCallBack(SpriteLayerRec *spriteLayerP, WorldProcPtr callBack)
{
    spriteLayerP->layerCallBack = callBack;
}

/* SWLockSpriteLayer (mac _SWLockSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477250
void SWLockSpriteLayer(SpriteLayerRec *spriteLayerP)
{
    SpriteRec *curSpriteP;

    if (spriteLayerP == 0)
        SWAssertFail("SpriteLayer.c", 498);

    curSpriteP = spriteLayerP->headSpriteP;

    while (curSpriteP != 0) {
        SWLockSprite(curSpriteP);
        curSpriteP = curSpriteP->nextSpriteP;
    }
}

/* SWUnlockSpriteLayer (mac _SWUnlockSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477290
void SWUnlockSpriteLayer(SpriteLayerRec *spriteLayerP)
{
    SpriteRec *curSpriteP;

    if (spriteLayerP == 0)
        SWAssertFail("SpriteLayer.c", 520);

    curSpriteP = spriteLayerP->headSpriteP;

    while (curSpriteP != 0) {
        SWUnlockSprite(curSpriteP);
        curSpriteP = curSpriteP->nextSpriteP;
    }
}

/* Four fork guards, two per sprite: isVisible (+0x14) and collisionsDisabled (+0xd0),
   neither in the donor. `collisionOccurred' is a Boolean -- zeroed with a DWORD store,
   tested with a BYTE compare. The SW_MIN/SW_MAX/SW_COLLISION_RECT macros double-evaluate. */



#define SW_COLLISION_RECT_TOP(spriteP) \
    (spriteP->destFrameRect.top + spriteP->curFrameP->collisionInset.top)

#define SW_COLLISION_RECT_LEFT(spriteP) \
    (spriteP->destFrameRect.left + spriteP->curFrameP->collisionInset.left)

#define SW_COLLISION_RECT_BOTTOM(spriteP) \
    (spriteP->destFrameRect.bottom - spriteP->curFrameP->collisionInset.bottom)

#define SW_COLLISION_RECT_RIGHT(spriteP) \
    (spriteP->destFrameRect.right - spriteP->curFrameP->collisionInset.right)

/* SWCollideSpriteLayer (mac _SWCollideSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004772d0
void SWCollideSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *srcSpriteLayerP,
                          SpriteLayerRec *dstSpriteLayerP)
{
    SpritePtr srcSpriteP, nextSrcSpriteP;
    SpritePtr dstSpriteP, nextDstSpriteP;
    Rect sectRect;
    unsigned char collisionOccurred;

    if (spriteWorldP == 0)
        SWAssertFail("SpriteLayer.c", 547);
    if (srcSpriteLayerP == 0 || dstSpriteLayerP == 0)
        SWAssertFail("SpriteLayer.c", 548);

    if (!spriteWorldP->frameHasOccurred)
        return;

    srcSpriteP = srcSpriteLayerP->headSpriteP;

    while (srcSpriteP != 0) {
        dstSpriteP = dstSpriteLayerP->headSpriteP;
        nextSrcSpriteP = srcSpriteP->nextSpriteP;

        if (srcSpriteP->isVisible && !srcSpriteP->collisionsDisabled) {
            while (dstSpriteP != 0) {
                nextDstSpriteP = dstSpriteP->nextSpriteP;

                if (srcSpriteP != dstSpriteP && dstSpriteP->isVisible &&
                        !dstSpriteP->collisionsDisabled) {
                    if ((srcSpriteP->destFrameRect.top < dstSpriteP->destFrameRect.bottom) &&
                        (srcSpriteP->destFrameRect.bottom > dstSpriteP->destFrameRect.top) &&
                        (srcSpriteP->destFrameRect.left < dstSpriteP->destFrameRect.right) &&
                        (srcSpriteP->destFrameRect.right > dstSpriteP->destFrameRect.left))
                    {
                        if (srcSpriteP->curFrameP->usesCollisionInset ||
                            dstSpriteP->curFrameP->usesCollisionInset)
                        {
                            collisionOccurred =
                                ((SW_COLLISION_RECT_TOP(srcSpriteP) < SW_COLLISION_RECT_BOTTOM(dstSpriteP)) &&
                                (SW_COLLISION_RECT_BOTTOM(srcSpriteP) > SW_COLLISION_RECT_TOP(dstSpriteP)) &&
                                (SW_COLLISION_RECT_LEFT(srcSpriteP) < SW_COLLISION_RECT_RIGHT(dstSpriteP)) &&
                                (SW_COLLISION_RECT_RIGHT(srcSpriteP) > SW_COLLISION_RECT_LEFT(dstSpriteP)) );
                        }
                        else
                        {
                            collisionOccurred = 1;
                        }

                        if (collisionOccurred && srcSpriteP->spriteCollideProc != 0) {
                            sectRect.left =
                                SW_MAX(SW_COLLISION_RECT_LEFT(srcSpriteP), SW_COLLISION_RECT_LEFT(dstSpriteP));
                            sectRect.top =
                                SW_MAX(SW_COLLISION_RECT_TOP(srcSpriteP), SW_COLLISION_RECT_TOP(dstSpriteP));
                            sectRect.right =
                                SW_MIN(SW_COLLISION_RECT_RIGHT(srcSpriteP), SW_COLLISION_RECT_RIGHT(dstSpriteP));
                            sectRect.bottom =
                                SW_MIN(SW_COLLISION_RECT_BOTTOM(srcSpriteP), SW_COLLISION_RECT_BOTTOM(dstSpriteP));

                            (*srcSpriteP->spriteCollideProc)(srcSpriteP, dstSpriteP, &sectRect);

                            if (srcSpriteP->spriteRemoval != kSWDontRemoveSprite ||
                                srcSpriteP->parentSpriteLayerP == 0)
                            {
                                break;
                            }
                        }
                    }
                }

                if (nextDstSpriteP != 0 && nextDstSpriteP->spriteRemoval != kSWDontRemoveSprite)
                    dstSpriteP = dstSpriteLayerP->headSpriteP;
                else
                    dstSpriteP = nextDstSpriteP;
            }
        }

        if (nextSrcSpriteP != 0 && nextSrcSpriteP->spriteRemoval != kSWDontRemoveSprite)
            srcSpriteP = srcSpriteLayerP->headSpriteP;
        else
            srcSpriteP = nextSrcSpriteP;
    }
}
