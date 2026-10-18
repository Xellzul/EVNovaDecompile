// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* spriteframe.c -- SpriteWorld's SpriteFrame.c, tu-map object O15: frame creation, disposal and locking. Notes: Research/notes/spriteframe.md */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"

/* SWCreateFrame (mac _SWCreateFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476400
short SWCreateFrame(void *theGDH, SWFrameRec **newFrameP, Rect *frameRect,
                    short depth, unsigned char createGWorld)
{
    short err = 0;
    CGrafPtr frameGWorld;
    SWFrameRec *tempFrameP;
    long numScanLines;

    if (!(frameRect->top >= 0 && frameRect->left >= 0 &&
            frameRect->right > frameRect->left && frameRect->bottom > frameRect->top))
        SWAssertFail("SpriteFrame.c", 72);

    *newFrameP = 0;

    numScanLines = frameRect->bottom - frameRect->top;

    tempFrameP = (SWFrameRec *)AllocAPointerClear(sizeof(SWFrameRec) +
            (sizeof(unsigned long) * (numScanLines + 1)));

    if (tempFrameP != 0) {
        tempFrameP->frameRect.top = frameRect->top;
        tempFrameP->frameRect.left = frameRect->left;
        tempFrameP->frameRect.bottom = frameRect->bottom;
        tempFrameP->frameRect.right = frameRect->right;
        tempFrameP->frameDepth = depth;
        tempFrameP->pixelShift = depth >> 4;
        tempFrameP->numScanLines = numScanLines;
        tempFrameP->collisionInset.left = 0;
        tempFrameP->collisionInset.top = 0;
        tempFrameP->collisionInset.right = 0;
        tempFrameP->collisionInset.bottom = 0;
        tempFrameP->usesCollisionInset = 0;
        tempFrameP->rleDataH = 0;

        if (createGWorld) {
            err = NewGWorld(&frameGWorld, depth, frameRect, theGDH);

            if ((int)err == noErr) {
                tempFrameP->framePort = frameGWorld;
                tempFrameP->frameDevice = GetGWorldDevice(frameGWorld);
                tempFrameP->framePixHndl = GetCWindowPixMap(frameGWorld);

                SWInitializeFrame(tempFrameP);
            }
        } else {
            tempFrameP->framePort = 0;
            tempFrameP->frameDevice = 0;
            tempFrameP->framePixHndl = 0;
            err = 0;
        }

        if ((int)err == noErr) {
            tempFrameP->hotSpotH = 0;
            tempFrameP->hotSpotV = 0;
            tempFrameP->tileMaskIsSolid = 0;
            tempFrameP->sharesGWorld = 0;
            tempFrameP->isFrameLocked = 0;
            tempFrameP->isWindowFrame = 0;
            tempFrameP->interlacingIsOn = 0;
            tempFrameP->skipOddLines = 1;

            tempFrameP->rleDataH = 0;
            tempFrameP->rleTokenStart = 0;

            tempFrameP->useCount = 0;

            *newFrameP = tempFrameP;
        }
    } else {
        err = memFullErr;
    }

    if ((int)err != noErr) {
        if (tempFrameP != 0) {
            if (tempFrameP->framePort != 0)
                DisposeGWorld(tempFrameP->framePort);
            DeallocAPointer(tempFrameP);
        }
    }

    SWSetStickyIfError(err);
    return err;
}

/* SW_MIN/SW_MAX are the donor's double-evaluating macros over a non-inlinable accessor,
   so each clip mentions its window bound twice and the exe parks the second call out of
   line. Write the macro; do not hoist the call into a local. */


/* SWCreateWindowFrame (mac _SWCreateWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004765d0
short SWCreateWindowFrame(CWindowPtr srcWindowP, SWFrameRec **newFrameP,
                          Rect *frameRect, short maxHeight)
{
    short err = 0;
    SWFrameRec *tempFrameP = 0;
    long numScanLines;

    if (!(frameRect->top >= 0 && frameRect->left >= 0))
        SWAssertFail("SpriteFrame.c", 190);
    if (!(frameRect->right > frameRect->left && frameRect->bottom > frameRect->top))
        SWAssertFail("SpriteFrame.c", 191);

    *newFrameP = 0;

    numScanLines = (GetCWindowPortRect(srcWindowP)->bottom -
                    GetCWindowPortRect(srcWindowP)->top);
    numScanLines = SW_MAX(numScanLines, maxHeight);

    frameRect->top = SW_MAX(frameRect->top, GetCWindowPortRect(srcWindowP)->top);
    frameRect->left = SW_MAX(frameRect->left, GetCWindowPortRect(srcWindowP)->left);
    frameRect->bottom = SW_MIN(frameRect->bottom, GetCWindowPortRect(srcWindowP)->bottom);
    frameRect->right = SW_MIN(frameRect->right, GetCWindowPortRect(srcWindowP)->right);

    tempFrameP = (SWFrameRec *)AllocAPointerClear(sizeof(SWFrameRec) +
            (sizeof(unsigned long) * numScanLines));

    if (tempFrameP != 0) {
        tempFrameP->framePort = GetCWindowGrafPort(srcWindowP);
        tempFrameP->frameDevice = (void *)GetMainDevice();
        tempFrameP->framePixHndl = GetWindowPixMap(srcWindowP);
        if (tempFrameP->framePixHndl == 0)
            tempFrameP->framePixHndl =
                    GetCWindowPixMap(GetCWindowGrafPort(srcWindowP));
        tempFrameP->frameDepth = GetPixDepth(tempFrameP->framePixHndl);
        tempFrameP->frameRect.top = frameRect->top;
        tempFrameP->frameRect.left = frameRect->left;
        tempFrameP->frameRect.bottom = frameRect->bottom;
        tempFrameP->frameRect.right = frameRect->right;
        tempFrameP->numScanLines = numScanLines;
        tempFrameP->sharesGWorld = 0;
        tempFrameP->isWindowFrame = 1;
        tempFrameP->isFrameLocked = 0;
        tempFrameP->interlacingIsOn = 0;
        tempFrameP->skipOddLines = 1;

        SWInitializeFrame(tempFrameP);

        *newFrameP = tempFrameP;
    } else {
        err = memFullErr;
    }

    if ((int)err != noErr) {
        if (tempFrameP != 0) {
            DeallocAPointer(tempFrameP);
        }
    }

    SWSetStickyIfError(err);
    return err;
}

/* `saveGDH = GetGWorldDevice(saveGWorld)' is never read, but the statement has to be
   written: mwcc keeps the call and drops the store. SpriteWorldRec's mainSWGDH is at
   +0xc0, immediately in front of the pixelDepth macport.h pins at +0xc4. */

/* SWCreateFrameFromCicnResource (mac _SWCreateFrameFromCicnResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476800
short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
                                    SWFrameRec **newFrameP, short iconResID,
                                    unsigned char maskType)
{
    short err;
    CGrafPtr saveGWorld;
    void *saveGDH;
    SWFrameRec *tempFrameP;
    void *cIconH;
    Rect frameRect;

    *newFrameP = 0;
    tempFrameP = 0;

    if (destSpriteWorld == 0)
        SWAssertFail("SpriteFrame.c", 288);

    GetPort(&saveGWorld);
    saveGDH = GetGWorldDevice(saveGWorld);

    cIconH = GetCIcon(iconResID);

    if (cIconH != 0) {
        QDGetCIconBounds(cIconH, &frameRect);

        err = SWCreateFrame(destSpriteWorld->mainSWGDH, &tempFrameP, &frameRect,
                destSpriteWorld->pixelDepth, kCreateGWorld);
    } else {
        err = resNotFound;
    }

    if ((int)err == noErr) {
        SetPort(tempFrameP->framePort);
        EraseRect(&frameRect);
        nv_BlitImageOr(&frameRect, cIconH);
    }

    if ((int)err == noErr) {
        if ((maskType & kPixelMask) != 0) {
            err = SWCreateGWorldFromCIconMask(destSpriteWorld,
                    &tempFrameP->maskPort, cIconH);

            if ((int)err == noErr) {
                tempFrameP->maskDevice = GetGWorldDevice(tempFrameP->maskPort);
                tempFrameP->maskPixHndl =
                        GetCWindowPixMap(tempFrameP->maskPort);

                SWFixImageGWorld(tempFrameP->framePort, tempFrameP->maskPort,
                        0);
            }

            if ((int)err == noErr && destSpriteWorld->pixelDepth <= 8) {
                SWInvertGWorld(tempFrameP->maskPort);
            }
        }
    }

    if (cIconH != 0) {
        DisposeCIcon(cIconH);
    }

    if ((int)err == noErr) {
        *newFrameP = tempFrameP;
    }

    if ((int)err != noErr) {
        if (tempFrameP != 0) {
            SWDisposeFrame(&tempFrameP);
        }
    }

    SetPort(saveGWorld);

    SWSetStickyIfError(err);
    return err;
}

/* Start's `if ((maskType & kRegionMask) != 0)' arm is GONE, not merely never taken, so
   its three trailing parameters are dead; they survive because 0x00474d07 still pushes
   four arguments, which is the only evidence for the signature. */

/* SWCreateFrameFromGWorldAndRectStart (mac _SWCreateFrameFromGWorldAndRectStart). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004769d0
short SWCreateFrameFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld, short maxWidth,
                                          short maxHeight, unsigned char maskType)
{
    short err = 0;

    *tempMaskGWorld = 0;
    return err;
}

/* SWCreateFrameFromGWorldAndRectFinish (mac _SWCreateFrameFromGWorldAndRectFinish). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004769e0
void SWCreateFrameFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld)
{
    SWCreateRegionFromGWorldAndRectFinish(tempMaskGWorld);
}

/* SWCreateFrameFromGWorldAndRectPartial (mac _SWCreateFrameFromGWorldAndRectPartial). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004769f0
short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
        CGrafPtr pictGWorld, CGrafPtr maskGWorld, CGrafPtr tempMaskGWorld,
        Rect *frameRect, unsigned char maskType)
{
    short err = 0;
    short depth;
    long numScanLines;
    SWFrameRec *tempFrameP;
    Rect *destRectP;

    if (pictGWorld == 0)
        SWAssertFail("SpriteFrame.c", 631);
    if (!(frameRect->top >= 0 && frameRect->left >= 0 &&
            frameRect->right > frameRect->left && frameRect->bottom > frameRect->top))
        SWAssertFail("SpriteFrame.c", 633);

    tempFrameP = 0;
    *newFrameP = 0;

    numScanLines = frameRect->bottom - frameRect->top;
    depth = GetPixDepth(GetCWindowPixMap(pictGWorld));

    tempFrameP = (SWFrameRec *)AllocAPointerClear(sizeof(SWFrameRec) +
            (sizeof(unsigned long) * (numScanLines + 1)));

    if (tempFrameP != 0) {
        tempFrameP->framePort = pictGWorld;
        tempFrameP->frameDevice = GetGWorldDevice(pictGWorld);
        tempFrameP->framePixHndl = GetCWindowPixMap(pictGWorld);
        tempFrameP->frameDepth = depth;

        if ((maskType & kPixelMask) != 0) {
            if (maskGWorld == 0)
                SWAssertFail("SpriteFrame.c", 654);
            tempFrameP->maskPort = maskGWorld;
            tempFrameP->maskDevice = GetGWorldDevice(maskGWorld);
            tempFrameP->maskPixHndl = GetCWindowPixMap(maskGWorld);
        }

        destRectP = &tempFrameP->frameRect;
        destRectP->top = frameRect->top;
        destRectP->left = frameRect->left;
        destRectP->bottom = frameRect->bottom;
        destRectP->right = frameRect->right;
        tempFrameP->sharesGWorld = 1;
        tempFrameP->isWindowFrame = 0;
        tempFrameP->numScanLines = numScanLines;

        tempFrameP->useCount = 0;

        SWInitializeFrame(tempFrameP);
    } else {
        err = memFullErr;
    }

    if ((int)err == noErr) {
        if ((int)err == noErr) {
            *newFrameP = tempFrameP;
        } else {
            if (tempFrameP != 0) {
                SWDisposeFrame(&tempFrameP);
            }
        }
    }

    SWSetStickyIfError(err);
    return err;
}

/* The donor's `SW_ASSERT(!oldFrameP->isWindowFrame)' became a guard that RETURNS FALSE
   through its own exit. useCount is unsigned -- `cmp word ptr [esi+0x58], 1' followed by
   `ja', not `jg'. */

/* SWDisposeFrame (mac _SWDisposeFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476bd0
char SWDisposeFrame(SWFrameRec **oldFramePP)
{
    char frameDisposed;
    SWFrameRec *oldFrameP = *oldFramePP;

    frameDisposed = 0;
    if (oldFrameP != 0) {
        if (oldFrameP->isWindowFrame)
            return 0;

        if (oldFrameP->useCount > 1) {
            oldFrameP->useCount--;
        } else {
            frameDisposed = 1;

            if (oldFrameP->framePort != 0) {
                if (!oldFrameP->sharesGWorld)
                    DisposeGWorld(oldFrameP->framePort);
                oldFrameP->framePort = 0;
                oldFrameP->frameDevice = 0;
                oldFrameP->framePixHndl = 0;
            }

            if (oldFrameP->maskPort != 0) {
                if (!oldFrameP->sharesGWorld)
                    DisposeGWorld(oldFrameP->maskPort);
                oldFrameP->maskPort = 0;
                oldFrameP->maskDevice = 0;
                oldFrameP->maskPixHndl = 0;
            }

            DeallocAPointer(oldFrameP);
            *oldFramePP = 0;
        }
    }

    return frameDisposed;
}

/* SWDisposeWindowFrame (mac _SWDisposeWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476c70
void SWDisposeWindowFrame(SWFrameRec **oldFramePP)
{
    SWFrameRec *oldFrameP = *oldFramePP;

    if (oldFrameP != 0) {
        if (!oldFrameP->isWindowFrame)
            SWAssertFail("SpriteFrame.c", 802);

        DeallocAPointer(oldFrameP);
        *oldFramePP = 0;
    }
}

/* SWSetPortToFrame (mac _SWSetPortToFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476cb0
void SWSetPortToFrame(SWFrameRec *dstFrameP)
{
    if (dstFrameP == 0)
        SWAssertFail("SpriteFrame.c", 818);
    if (dstFrameP->framePort == 0)
        SWAssertFail("SpriteFrame.c", 819);
    if (!dstFrameP->isFrameLocked)
        SWAssertFail("SpriteFrame.c", 820);

    SetPort(dstFrameP->framePort);
}

/* SWInitializeFrame (mac _SWInitializeFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476d10
void SWInitializeFrame(SWFrameRec *tempFrameP)
{
    long curScanLine;
    long rowOffset;
    short depth;
    PixMapPtr pixmap;

    if (tempFrameP == 0)
        SWAssertFail("SpriteFrame.c", 920);

    if (tempFrameP->framePixHndl == 0) {
        if (tempFrameP->framePort != 0)
            tempFrameP->framePixHndl =
                    GetCWindowPixMap(tempFrameP->framePort);
    }

    if (tempFrameP->framePixHndl == 0)
        SWAssertFail("SpriteFrame.c", 928);
    if (tempFrameP->frameDepth == 0)
        SWAssertFail("SpriteFrame.c", 929);

    pixmap = tempFrameP->framePixHndl;
    depth = tempFrameP->frameDepth;

    tempFrameP->frameRowBytes = GetPixRowBytes(pixmap);

    tempFrameP->rightAlignFactor = (32 / depth) - 1;
    tempFrameP->leftAlignFactor = ~(tempFrameP->rightAlignFactor);

    tempFrameP->pixelShift = depth >> 4;

    tempFrameP->scanLinePtrArray = (unsigned long *)(tempFrameP + 1);

    rowOffset = tempFrameP->frameRect.top * tempFrameP->frameRowBytes;
    for (curScanLine = 0; curScanLine < tempFrameP->numScanLines; curScanLine++) {
        tempFrameP->scanLinePtrArray[curScanLine] = rowOffset;
        rowOffset += tempFrameP->frameRowBytes;
    }
}

/* SWLockFrame (mac _SWLockFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476e10
void SWLockFrame(SWFrameRec *srcFrameP)
{
    PixMapPtr pixMapH;

    if (srcFrameP == 0)
        return;
    if (srcFrameP == 0)
        SWAssertFail("SpriteFrame.c", 1278);

    if (srcFrameP->isWindowFrame)
        return;
    if (srcFrameP->isWindowFrame)
        SWAssertFail("SpriteFrame.c", 1283);

    if (!srcFrameP->isFrameLocked) {
        srcFrameP->isFrameLocked = 1;

        if (srcFrameP->framePort != 0) {
            if (srcFrameP->framePixHndl == 0)
                SWAssertFail("SpriteFrame.c", 1291);

            pixMapH = srcFrameP->framePixHndl;
            srcFrameP->framePix = pixMapH;

            srcFrameP->frameBaseAddr = GetPixBaseAddr(pixMapH);
            srcFrameP->frameRowBytes = GetPixRowBytes(pixMapH);
            srcFrameP->frameDepth = GetPixDepth(pixMapH);

            srcFrameP->worldRectOffset = 0;
        }

        if (srcFrameP->maskPort != 0) {
            if (srcFrameP->maskPixHndl == 0)
                SWAssertFail("SpriteFrame.c", 1348);

            pixMapH = srcFrameP->maskPixHndl;
            srcFrameP->maskPix = pixMapH;

            srcFrameP->maskBaseAddr = GetPixBaseAddr(pixMapH);
        }
    }
}

/* SWUnlockFrame (mac _SWUnlockFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476f10
void SWUnlockFrame(SWFrameRec *srcFrameP)
{
    if (srcFrameP == 0)
        SWAssertFail("SpriteFrame.c", 1389);
    if (srcFrameP->isWindowFrame)
        SWAssertFail("SpriteFrame.c", 1390);

    if (srcFrameP->isFrameLocked) {
        srcFrameP->isFrameLocked = 0;

        srcFrameP->framePix = 0;
        srcFrameP->frameBaseAddr = 0;

        srcFrameP->maskPix = 0;
        srcFrameP->maskBaseAddr = 0;
    }
}

/* The dead Rect is real and must be written: the exe still brackets a
   GetWindowPortRect(framePort) whose result goes nowhere, because mwcc keeps the call and
   drops the copy. worldRectOffset collapses to the constant 0 the exe stores at +0x5a. */

/* SWLockWindowFrame (mac _SWLockWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476f80
void SWLockWindowFrame(SWFrameRec *windowFrameP)
{
    CGrafPtr savePort;
    Rect globalRect;
    PixMapPtr pixMapH;

    if (windowFrameP == 0)
        SWAssertFail("SpriteFrame.c", 1442);
    if (windowFrameP->framePort == 0)
        SWAssertFail("SpriteFrame.c", 1443);
    if (windowFrameP->framePixHndl == 0)
        SWAssertFail("SpriteFrame.c", 1444);
    if (!windowFrameP->isWindowFrame)
        SWAssertFail("SpriteFrame.c", 1445);

    if (!windowFrameP->isFrameLocked) {
        windowFrameP->isFrameLocked = 1;
        pixMapH = windowFrameP->framePixHndl;
        windowFrameP->framePix = pixMapH;

        GetPort(&savePort);
        SetPort(windowFrameP->framePort);
        globalRect = *GetWindowPortRect(windowFrameP->framePort);
        SetPort(savePort);

        windowFrameP->frameBaseAddr = GetPixBaseAddr(pixMapH);
        windowFrameP->frameRowBytes = GetPixRowBytes(pixMapH);
        windowFrameP->frameDepth = GetPixDepth(pixMapH);
        windowFrameP->worldRectOffset = 0;

        SWInitializeFrame(windowFrameP);
    }
}

/* SWUnlockWindowFrame (mac _SWUnlockWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477070
void SWUnlockWindowFrame(SWFrameRec *windowFrameP)
{
    if (windowFrameP == 0)
        SWAssertFail("SpriteFrame.c", 1528);
    if (!windowFrameP->isWindowFrame)
        SWAssertFail("SpriteFrame.c", 1529);

    if (windowFrameP->isFrameLocked) {
        windowFrameP->isFrameLocked = 0;
        windowFrameP->framePix = 0;
        windowFrameP->frameBaseAddr = 0;
    }
}
