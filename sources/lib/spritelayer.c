// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* spritelayer.c -- SpriteWorld's SpriteLayer.c, tu-map object O16: sprite layers. Notes: Research/notes/spritelayer.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#define kSpriteAlreadyInLayer   115

/* SWAddSprite (mac _SWAddSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477140
// MAC: _SWAddSprite i386 0x000ac5de ppc 0x000b7708
short SWAddSprite(SpriteLayerRec *spriteLayerP, SpriteRec *newSpriteP)
{
    SpriteRec *tailSpriteP = spriteLayerP->tailSpriteP;
    short err = 0;

    if (spriteLayerP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 109, 103);
    if (newSpriteP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 110, 104);

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
// MAC: _SWRemoveSprite i386 0x000ac672 ppc 0x000b77cc
short SWRemoveSprite(SpriteRec *oldSpriteP)
{
    SpriteLayerRec *spriteLayerP;
    short err;

    err = 0;
    if (oldSpriteP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 153, 147);

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

#ifdef MACBUILD
/* SWRemoveAllSpritesFromLayer (mac _SWRemoveAllSpritesFromLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWRemoveAllSpritesFromLayer i386 0x000acf78 ppc 0x000b8308
void SWRemoveAllSpritesFromLayer(SpriteLayerPtr srcSpriteLayerP)
{
    SpritePtr curSpriteP;

    SW_ASSERT(srcSpriteLayerP != NULL, 196);

    while ((curSpriteP = SWGetNextSprite(srcSpriteLayerP, NULL)) != NULL) {
        SWRemoveSprite(curSpriteP);
    }
}

/* SWDisposeAllSpritesInLayer (mac _SWDisposeAllSpritesInLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWDisposeAllSpritesInLayer i386 0x000ac731 ppc 0x000b7908
void SWDisposeAllSpritesInLayer(SpriteLayerPtr spriteLayerP)
{
    SpritePtr curSpriteP, nextSpriteP;

    SW_ASSERT(spriteLayerP != NULL, 214);

    curSpriteP = spriteLayerP->headSpriteP;

    while (curSpriteP != NULL) {
        nextSpriteP = curSpriteP->nextSpriteP;

        SWRemoveSprite(curSpriteP);
        SWDisposeSprite(&curSpriteP);

        curSpriteP = nextSpriteP;
    }
}

/* SWCountNumSpritesInLayer (mac _SWCountNumSpritesInLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWCountNumSpritesInLayer i386 0x000ac789 ppc 0x000b7988
short SWCountNumSpritesInLayer(SpriteLayerPtr spriteLayerP)
{
    SpritePtr curSpriteP;
    short numSprites = 0;

    SW_ASSERT(spriteLayerP != NULL, 241);

    curSpriteP = spriteLayerP->headSpriteP;

    while (curSpriteP != NULL) {
        numSprites++;
        curSpriteP = curSpriteP->nextSpriteP;
    }

    return numSprites;
}

/* SWSwapSprite (mac _SWSwapSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWSwapSprite i386 0x000ac7c5 ppc 0x000b7a00
OSErr SWSwapSprite(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    SpriteLayerPtr spriteLayerP;
    register SpritePtr swapSpriteP;
    OSErr err = noErr;

    SW_ASSERT(srcSpriteP != NULL && dstSpriteP != NULL, 268);

    if (srcSpriteP == dstSpriteP)
        return noErr;

    spriteLayerP = srcSpriteP->parentSpriteLayerP;
    if (spriteLayerP != dstSpriteP->parentSpriteLayerP || spriteLayerP == NULL) {
        err = kBadParameterErr;
        SWSetStickyIfError(err);
        return err;
    }

    if (srcSpriteP->nextSpriteP == dstSpriteP ||
        dstSpriteP->nextSpriteP == srcSpriteP) {
        if (srcSpriteP->nextSpriteP == dstSpriteP) {
            if (srcSpriteP->prevSpriteP != NULL)
                (srcSpriteP->prevSpriteP)->nextSpriteP = dstSpriteP;
            if (dstSpriteP->nextSpriteP != NULL)
                (dstSpriteP->nextSpriteP)->prevSpriteP = srcSpriteP;

            dstSpriteP->prevSpriteP = srcSpriteP->prevSpriteP;
            srcSpriteP->nextSpriteP = dstSpriteP->nextSpriteP;

            dstSpriteP->nextSpriteP = srcSpriteP;
            srcSpriteP->prevSpriteP = dstSpriteP;
        } else {
            if (dstSpriteP->prevSpriteP != NULL)
                (dstSpriteP->prevSpriteP)->nextSpriteP = srcSpriteP;
            if (srcSpriteP->nextSpriteP != NULL)
                (srcSpriteP->nextSpriteP)->prevSpriteP = dstSpriteP;

            srcSpriteP->prevSpriteP = dstSpriteP->prevSpriteP;
            dstSpriteP->nextSpriteP = srcSpriteP->nextSpriteP;

            srcSpriteP->nextSpriteP = dstSpriteP;
            dstSpriteP->prevSpriteP = srcSpriteP;
        }
    } else {
        if (srcSpriteP->prevSpriteP != NULL)
            srcSpriteP->prevSpriteP->nextSpriteP = dstSpriteP;
        if (dstSpriteP->prevSpriteP != NULL)
            dstSpriteP->prevSpriteP->nextSpriteP = srcSpriteP;
        if (srcSpriteP->nextSpriteP != NULL)
            srcSpriteP->nextSpriteP->prevSpriteP = dstSpriteP;
        if (dstSpriteP->nextSpriteP != NULL)
            dstSpriteP->nextSpriteP->prevSpriteP = srcSpriteP;

        swapSpriteP = srcSpriteP->nextSpriteP;
        srcSpriteP->nextSpriteP = dstSpriteP->nextSpriteP;
        dstSpriteP->nextSpriteP = swapSpriteP;

        swapSpriteP = srcSpriteP->prevSpriteP;
        srcSpriteP->prevSpriteP = dstSpriteP->prevSpriteP;
        dstSpriteP->prevSpriteP = swapSpriteP;
    }

    if (srcSpriteP->nextSpriteP == NULL) {
        spriteLayerP->tailSpriteP = srcSpriteP;
    } else if (srcSpriteP->prevSpriteP == NULL) {
        spriteLayerP->headSpriteP = srcSpriteP;
    }

    if (dstSpriteP->nextSpriteP == NULL) {
        spriteLayerP->tailSpriteP = dstSpriteP;
    } else if (dstSpriteP->prevSpriteP == NULL) {
        spriteLayerP->headSpriteP = dstSpriteP;
    }

    return noErr;
}

/* SWInsertSpriteAfterSprite (mac _SWInsertSpriteAfterSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWInsertSpriteAfterSprite i386 0x000ac8ff ppc 0x000b7bd4
OSErr SWInsertSpriteAfterSprite(SpritePtr newSpriteP, SpritePtr dstSpriteP)
{
    SpriteLayerPtr spriteLayerP;
    OSErr err = noErr;

    SW_ASSERT(newSpriteP != NULL && dstSpriteP != NULL, 367);

    spriteLayerP = dstSpriteP->parentSpriteLayerP;
    if (newSpriteP->parentSpriteLayerP != NULL)
        err = kSpriteAlreadyInLayer;

    if (err == noErr) {
        if (dstSpriteP->nextSpriteP != NULL)
            (dstSpriteP->nextSpriteP)->prevSpriteP = newSpriteP;

        newSpriteP->nextSpriteP = dstSpriteP->nextSpriteP;
        dstSpriteP->nextSpriteP = newSpriteP;
        newSpriteP->prevSpriteP = dstSpriteP;

        if (newSpriteP->nextSpriteP == NULL) {
            spriteLayerP->tailSpriteP = newSpriteP;
        }

        newSpriteP->needsToBeDrawn = true;

        newSpriteP->parentSpriteLayerP = spriteLayerP;
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWInsertSpriteBeforeSprite (mac _SWInsertSpriteBeforeSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWInsertSpriteBeforeSprite i386 0x000ac9ae ppc 0x000b7c9c
OSErr SWInsertSpriteBeforeSprite(SpritePtr newSpriteP, SpritePtr dstSpriteP)
{
    SpriteLayerPtr spriteLayerP;
    OSErr err = noErr;

    SW_ASSERT(newSpriteP != NULL && dstSpriteP != NULL, 409);

    spriteLayerP = dstSpriteP->parentSpriteLayerP;
    if (newSpriteP->parentSpriteLayerP != NULL)
        err = kSpriteAlreadyInLayer;

    if (err == noErr) {
        if (dstSpriteP->prevSpriteP != NULL)
            (dstSpriteP->prevSpriteP)->nextSpriteP = newSpriteP;

        newSpriteP->prevSpriteP = dstSpriteP->prevSpriteP;
        dstSpriteP->prevSpriteP = newSpriteP;
        newSpriteP->nextSpriteP = dstSpriteP;

        if (newSpriteP->prevSpriteP == NULL) {
            spriteLayerP->headSpriteP = newSpriteP;
        }

        newSpriteP->needsToBeDrawn = true;

        newSpriteP->parentSpriteLayerP = spriteLayerP;
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWGetNextSprite (mac _SWGetNextSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWGetNextSprite i386 0x000ac6eb ppc 0x000b78ac
SpritePtr SWGetNextSprite(SpriteLayerPtr spriteLayerP, SpritePtr curSpriteP)
{
    SW_ASSERT(spriteLayerP != NULL, 448);

    return (curSpriteP == NULL) ? spriteLayerP->headSpriteP : curSpriteP->nextSpriteP;
}

/* SWGetPreviousSprite (mac _SWGetPreviousSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWGetPreviousSprite i386 0x000aca5e ppc 0x000b7d5c
SpritePtr SWGetPreviousSprite(SpriteLayerPtr spriteLayerP, SpritePtr curSpriteP)
{
    SW_ASSERT(spriteLayerP != NULL, 462);

    return (curSpriteP == NULL) ? spriteLayerP->tailSpriteP : curSpriteP->prevSpriteP;
}
#endif

/* The fork's extra SpriteLayerRec member: this function stores its proc argument to
   +0x10 and does nothing else, and SWCreateSpriteLayer is what clears it. */

/* SWSetLayerCallBack (mac _SWSetLayerCallBack). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477240
// MAC: _SWSetLayerCallBack i386 0x000acab1 ppc 0x000b7db8
void SWSetLayerCallBack(SpriteLayerRec *spriteLayerP, WorldProcPtr callBack)
{
    spriteLayerP->layerCallBack = callBack;
}

/* SWLockSpriteLayer (mac _SWLockSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477250
// MAC: _SWLockSpriteLayer i386 0x000acabf ppc 0x000b7dc0
void SWLockSpriteLayer(SpriteLayerRec *spriteLayerP)
{
    SpriteRec *curSpriteP;

    if (spriteLayerP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 498, 490);

    curSpriteP = spriteLayerP->headSpriteP;

    while (curSpriteP != 0) {
        SWLockSprite(curSpriteP);
        curSpriteP = curSpriteP->nextSpriteP;
    }
}

/* SWUnlockSpriteLayer (mac _SWUnlockSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477290
// MAC: _SWUnlockSpriteLayer i386 0x000acb0b ppc 0x000b7e34
void SWUnlockSpriteLayer(SpriteLayerRec *spriteLayerP)
{
    SpriteRec *curSpriteP;

    if (spriteLayerP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 520, 512);

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
// MAC: _SWCollideSpriteLayer i386 0x000acb4b ppc 0x000b7ea4
void SWCollideSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *srcSpriteLayerP,
                          SpriteLayerRec *dstSpriteLayerP)
{
    SpritePtr srcSpriteP, nextSrcSpriteP;
    SpritePtr dstSpriteP, nextDstSpriteP;
    Rect sectRect;
    unsigned char collisionOccurred;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 547, 539);
    if (srcSpriteLayerP == 0 || dstSpriteLayerP == 0)
        SW_ASSERT_FAIL("SpriteLayer.c", 548, 540);

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

#ifdef MACBUILD
#define SortCriteria(bottomSpriteP, topSpriteP) \
    if (bottomSpriteP->destFrameRect.bottom > topSpriteP->destFrameRect.bottom)

/* SWSortSpriteLayer (mac _SWSortSpriteLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWSortSpriteLayer i386 0x000acfcf ppc 0x000b8374
void SWSortSpriteLayer(SpriteLayerPtr theLayer)
{
    SpritePtr curSpriteP, prevSprite, insertSprite;
    Boolean done;

    prevSprite = theLayer->headSpriteP;
    curSpriteP = SWGetNextSprite(theLayer, prevSprite);

    while (curSpriteP != NULL) {
        SortCriteria(prevSprite, curSpriteP) {
            insertSprite = curSpriteP->prevSpriteP;
            done = false;

            while (insertSprite != NULL && !done) {
                SortCriteria(insertSprite, curSpriteP)
                    insertSprite = insertSprite->prevSpriteP;
                else
                    done = true;
            }

            if (insertSprite != NULL) {
                SWRemoveSprite(curSpriteP);

                SWInsertSpriteAfterSprite(curSpriteP, insertSprite);
                curSpriteP = SWGetNextSprite(theLayer, prevSprite);
            } else {
                SWRemoveSprite(curSpriteP);

                SWInsertSpriteBeforeSprite(curSpriteP, theLayer->headSpriteP);
                curSpriteP = SWGetNextSprite(theLayer, prevSprite);
            }
        } else {
            prevSprite = curSpriteP;
            curSpriteP = curSpriteP->nextSpriteP;
        }
    }
}

/* SWPauseSpriteLayer (mac _SWPauseSpriteLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWPauseSpriteLayer i386 0x000ace4f ppc 0x000b8194
void SWPauseSpriteLayer(SpriteLayerPtr spriteLayerP)
{
    SW_ASSERT(spriteLayerP != NULL, 708);

    spriteLayerP->isPaused = true;
}

/* SWUnpauseSpriteLayer (mac _SWUnpauseSpriteLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWUnpauseSpriteLayer i386 0x000ace8e ppc 0x000b81dc
void SWUnpauseSpriteLayer(SpriteLayerPtr spriteLayerP)
{
    SW_ASSERT(spriteLayerP != NULL, 721);

    spriteLayerP->isPaused = false;
}

/* SWSetLayerAsNonScrolling (mac _SWSetLayerAsNonScrolling, mac-only). STATE: see the mac gate. */
// MAC: _SWSetLayerAsNonScrolling i386 0x000acece ppc 0x000b822c
void SWSetLayerAsNonScrolling(SpriteLayerPtr spriteLayerP, Boolean layerIsNonScrolling)
{
    SW_ASSERT(spriteLayerP != NULL, 735);

    spriteLayerP->layerIsNonScrolling = layerIsNonScrolling;
}

/* SWFindSpriteByPoint (mac _SWFindSpriteByPoint, mac-only). STATE: see the mac gate. */
// MAC: _SWFindSpriteByPoint i386 0x000acf0f ppc 0x000b8280
SpritePtr SWFindSpriteByPoint(SpriteLayerPtr spriteLayerP, SpritePtr startSpriteP, Point testPoint)
{
    SpritePtr curSpriteP;

    SW_ASSERT(spriteLayerP != NULL, 752);

    curSpriteP = (startSpriteP == NULL) ? spriteLayerP->tailSpriteP : startSpriteP;

    while ((curSpriteP != NULL) && !SWIsPointInSprite(curSpriteP, testPoint)) {
        curSpriteP = curSpriteP->prevSpriteP;
    }

    return curSpriteP;
}
#endif
