// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
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
// MAC: _SWCreateSpriteWorldFromWindow i386 0x000b6e56 ppc 0x000c2954
#ifdef MACBUILD
short SWCreateSpriteWorldFromWindow(SpriteWorldPtr *spriteWorldP, CWindowPtr srcWindowP,
                                    Rect *worldRectP, Rect *offscreenRectP, short maxDepth)
{
    OSErr err = noErr;
    FramePtr windowFrameP, backFrameP, workFrameP;
    Rect tempRect, globalRect, windRect;
    GDHandle mainGDH;
    PixMapHandle basePixMap;
    short depth;

    *spriteWorldP = NULL;
    windowFrameP = backFrameP = workFrameP = NULL;

    SetPort((GrafPtr)GetCWindowGrafPort((WindowRef)srcWindowP));

    windRect = (worldRectP == NULL) ? *GetCWindowPortRect((WindowRef)srcWindowP) : *worldRectP;

    globalRect = windRect;
    LocalToGlobal((Point *)&globalRect.top);
    LocalToGlobal((Point *)&globalRect.bottom);

    mainGDH = GetMainDevice();
    if (useLegacyRenderingPipeline) {
        basePixMap = (**mainGDH).gdPMap;
        if (maxDepth == 0 || (**basePixMap).pixelSize <= maxDepth) {
            depth = (**basePixMap).pixelSize;
        } else {
            depth = maxDepth;
        }
    } else {
        depth = 16;
        basePixMap = GetPortPixMap(GetCWindowGrafPort((WindowRef)srcWindowP));
    }

#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    if (srcWindowP->portVersion < (short)0xC000)
        err = kNotCWindowErr;
#endif

    if (offscreenRectP == NULL)
        tempRect = windRect;
    else
        tempRect = *offscreenRectP;

    OffsetRect(&tempRect, -tempRect.left, -tempRect.top);

    if (err == noErr) {
        err = SWCreateWindowFrame(srcWindowP, &windowFrameP, &windRect, tempRect.bottom);
    }

    if (err == noErr) {
        err = SWCreateFrame(mainGDH, &backFrameP, &tempRect, depth, kCreateGWorld);
    }

    if (err == noErr) {
        err = SWCreateFrame(mainGDH, &workFrameP, &tempRect, depth, kCreateGWorld);
    }

    if (err == noErr) {
        err = SWCreateSpriteWorld(mainGDH, spriteWorldP, windowFrameP,
                backFrameP, workFrameP, maxDepth);
    }

    if (err != noErr) {
        if (windowFrameP != NULL) {
            SWDisposeWindowFrame(&windowFrameP);
        }

        if (backFrameP != NULL) {
            SWDisposeFrame(&backFrameP);
        }

        if (workFrameP != NULL) {
            SWDisposeFrame(&workFrameP);
        }
    }

    SWSetStickyIfError(err);
    return err;
}
#else
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
#endif

#ifdef MACBUILD
/* SWDisposeSpriteWorld (mac _SWDisposeSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWDisposeSpriteWorld i386 0x000b94d1 ppc 0x000c541c
void SWDisposeSpriteWorld(SpriteWorldPtr *spriteWorldPP)
{
    SpriteWorldPtr spriteWorldP = *spriteWorldPP;
    SpriteLayerPtr curLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP != NULL) {
        SWUnlockSpriteWorld(spriteWorldP);

        while ((curLayerP = SWGetNextSpriteLayer(spriteWorldP, NULL)) != NULL) {
            while ((curSpriteP = SWGetNextSprite(curLayerP, NULL)) != NULL) {
                SWRemoveSprite(curSpriteP);
                SWDisposeSprite(&curSpriteP);
            }

            SWRemoveSpriteLayer(spriteWorldP, curLayerP);
            SWDisposeSpriteLayer(&curLayerP);
        }

        SWDisposeAllSpritesInLayer(spriteWorldP->deadSpriteLayerP);
        SWDisposeSpriteLayer(&spriteWorldP->deadSpriteLayerP);

        SWDisposeFrame(&spriteWorldP->backFrameP);
        SWDisposeFrame(&spriteWorldP->workFrameP);
        SWDisposeFrame(&spriteWorldP->extraBackFrameP);

        SWDisposeWindowFrame(&spriteWorldP->windowFrameP);

        SWExitTiling(spriteWorldP);
        DisposeRgn(spriteWorldP->windowRgn);

        DisposePtr((Ptr)spriteWorldP);
        *spriteWorldPP = NULL;
    }
}
#endif

/* SWExitSpriteWorld (mac _SWExitSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477af0
// MAC: _SWExitSpriteWorld i386 0x000b70b2 ppc 0x000c2b8c
#ifdef MACBUILD
void SWExitSpriteWorld(void)
{
    if (gSWCollisionSectRgn != NULL)
        DisposeRgn(gSWCollisionSectRgn);
    if (gSWCollisionSpareRgn != NULL)
        DisposeRgn(gSWCollisionSpareRgn);

    gSWCollisionSectRgn = NULL;
    gSWCollisionSpareRgn = NULL;
}
#else
void SWExitSpriteWorld(void)
{
}
#endif

/* SWAddSpriteLayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477b00
// MAC: _SWAddSpriteLayer i386 0x000b71b0 ppc 0x000c2cf4
void SWAddSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *newSpriteLayerP)
{
    SpriteLayerRec *tailSpriteLayerP = spriteWorldP->tailSpriteLayerP;

    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 459, 444);
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
// MAC: _SWRemoveSpriteLayer i386 0x000b7140 ppc 0x000c2c48
void SWRemoveSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *oldSpriteLayerP)
{
    if (spriteWorldP == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 482, 467);
    if (oldSpriteLayerP->nextSpriteLayerP != NULL)
        oldSpriteLayerP->nextSpriteLayerP->prevSpriteLayerP = oldSpriteLayerP->prevSpriteLayerP;
    else
        spriteWorldP->tailSpriteLayerP = oldSpriteLayerP->prevSpriteLayerP;
    if (oldSpriteLayerP->prevSpriteLayerP != NULL)
        oldSpriteLayerP->prevSpriteLayerP->nextSpriteLayerP = oldSpriteLayerP->nextSpriteLayerP;
    else
        spriteWorldP->headSpriteLayerP = oldSpriteLayerP->nextSpriteLayerP;
}

#ifdef MACBUILD
/* SWSwapSpriteLayer (mac _SWSwapSpriteLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWSwapSpriteLayer i386 0x000b71e0 ppc 0x000c2d34
void SWSwapSpriteLayer(
    SpriteWorldPtr spriteWorldP,
    SpriteLayerPtr srcSpriteLayerP,
    SpriteLayerPtr dstSpriteLayerP)
{
    register SpriteLayerPtr swapSpriteLayerP;

    SW_ASSERT(spriteWorldP != NULL, 506);
    SW_ASSERT(srcSpriteLayerP != NULL && dstSpriteLayerP != NULL, 507);

    if (srcSpriteLayerP == dstSpriteLayerP)
        return;

    if (srcSpriteLayerP->nextSpriteLayerP == dstSpriteLayerP ||
        dstSpriteLayerP->nextSpriteLayerP == srcSpriteLayerP) {
        if (srcSpriteLayerP->nextSpriteLayerP == dstSpriteLayerP) {
            if (srcSpriteLayerP->prevSpriteLayerP != NULL)
                (srcSpriteLayerP->prevSpriteLayerP)->nextSpriteLayerP = dstSpriteLayerP;
            if (dstSpriteLayerP->nextSpriteLayerP != NULL)
                (dstSpriteLayerP->nextSpriteLayerP)->prevSpriteLayerP = srcSpriteLayerP;

            dstSpriteLayerP->prevSpriteLayerP = srcSpriteLayerP->prevSpriteLayerP;
            srcSpriteLayerP->nextSpriteLayerP = dstSpriteLayerP->nextSpriteLayerP;

            dstSpriteLayerP->nextSpriteLayerP = srcSpriteLayerP;
            srcSpriteLayerP->prevSpriteLayerP = dstSpriteLayerP;
        } else {
            if (dstSpriteLayerP->prevSpriteLayerP != NULL)
                (dstSpriteLayerP->prevSpriteLayerP)->nextSpriteLayerP = srcSpriteLayerP;
            if (srcSpriteLayerP->nextSpriteLayerP != NULL)
                (srcSpriteLayerP->nextSpriteLayerP)->prevSpriteLayerP = dstSpriteLayerP;

            srcSpriteLayerP->prevSpriteLayerP = dstSpriteLayerP->prevSpriteLayerP;
            dstSpriteLayerP->nextSpriteLayerP = srcSpriteLayerP->nextSpriteLayerP;

            srcSpriteLayerP->nextSpriteLayerP = dstSpriteLayerP;
            dstSpriteLayerP->prevSpriteLayerP = srcSpriteLayerP;
        }
    } else {
        if (srcSpriteLayerP->prevSpriteLayerP != NULL)
            srcSpriteLayerP->prevSpriteLayerP->nextSpriteLayerP = dstSpriteLayerP;
        if (dstSpriteLayerP->prevSpriteLayerP != NULL)
            dstSpriteLayerP->prevSpriteLayerP->nextSpriteLayerP = srcSpriteLayerP;
        if (srcSpriteLayerP->nextSpriteLayerP != NULL)
            srcSpriteLayerP->nextSpriteLayerP->prevSpriteLayerP = dstSpriteLayerP;
        if (dstSpriteLayerP->nextSpriteLayerP != NULL)
            dstSpriteLayerP->nextSpriteLayerP->prevSpriteLayerP = srcSpriteLayerP;

        swapSpriteLayerP = srcSpriteLayerP->nextSpriteLayerP;
        srcSpriteLayerP->nextSpriteLayerP = dstSpriteLayerP->nextSpriteLayerP;
        dstSpriteLayerP->nextSpriteLayerP = swapSpriteLayerP;

        swapSpriteLayerP = srcSpriteLayerP->prevSpriteLayerP;
        srcSpriteLayerP->prevSpriteLayerP = dstSpriteLayerP->prevSpriteLayerP;
        dstSpriteLayerP->prevSpriteLayerP = swapSpriteLayerP;
    }

    if (srcSpriteLayerP->nextSpriteLayerP == NULL) {
        spriteWorldP->tailSpriteLayerP = srcSpriteLayerP;
    } else if (srcSpriteLayerP->prevSpriteLayerP == NULL) {
        spriteWorldP->headSpriteLayerP = srcSpriteLayerP;
    }

    if (dstSpriteLayerP->nextSpriteLayerP == NULL) {
        spriteWorldP->tailSpriteLayerP = dstSpriteLayerP;
    } else if (dstSpriteLayerP->prevSpriteLayerP == NULL) {
        spriteWorldP->headSpriteLayerP = dstSpriteLayerP;
    }
}

/* SWGetNextSpriteLayer (mac _SWGetNextSpriteLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWGetNextSpriteLayer i386 0x000b70f0 ppc 0x000c2bf0
SpriteLayerPtr SWGetNextSpriteLayer(SpriteWorldPtr spriteWorldP, SpriteLayerPtr curSpriteLayerP)
{
    SW_ASSERT(spriteWorldP != NULL, 592);

    return (curSpriteLayerP == NULL) ?
            spriteWorldP->headSpriteLayerP :
            curSpriteLayerP->nextSpriteLayerP;
}
#endif

/* SWLockSpriteWorld. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477ba0
// MAC: _SWLockSpriteWorld i386 0x000b734d ppc 0x000c2ef4
#ifdef MACBUILD
void SWLockSpriteWorld(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerPtr curSpriteLayerP;

    SW_ASSERT(spriteWorldP != NULL, 609);

    SWLockWindowFrame(spriteWorldP->windowFrameP);
    SWLockFrame(spriteWorldP->workFrameP);
    SWLockFrame(spriteWorldP->backFrameP);
    if (spriteWorldP->extraBackFrameP != NULL)
        SWLockFrame(spriteWorldP->extraBackFrameP);

    SWLockTiles(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL) {
        SWLockSpriteLayer(curSpriteLayerP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}
#else
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
#endif

#ifdef MACBUILD
/* SWUnlockSpriteWorld (mac _SWUnlockSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWUnlockSpriteWorld i386 0x000b73ca ppc 0x000c2f84
void SWUnlockSpriteWorld(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerPtr curSpriteLayerP;

    SW_ASSERT(spriteWorldP != NULL, 639);

    SWUnlockWindowFrame(spriteWorldP->windowFrameP);
    SWUnlockFrame(spriteWorldP->backFrameP);
    SWUnlockFrame(spriteWorldP->workFrameP);
    if (spriteWorldP->extraBackFrameP != NULL)
        SWUnlockFrame(spriteWorldP->extraBackFrameP);

    SWUnlockTiles(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL) {
        SWUnlockSpriteLayer(curSpriteLayerP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }
}
#endif

/* SWSetPortToBackground. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c10
// MAC: _SWSetPortToBackground i386 0x000b744a ppc 0x000c3014
void SWSetPortToBackground(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld == NULL)
        SW_ASSERT_FAIL("SpriteWorld.c", 687, 668);
    SWSetPortToFrame(spriteWorld->backFrameP);
}

/* SWSetPortToWorkArea. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c40
// MAC: _SWSetPortToWorkArea i386 0x000b7494 ppc 0x000c3058
void SWSetPortToWorkArea(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld == NULL) {
        SW_ASSERT_FAIL("SpriteWorld.c", 700, 681);
    }
    SWSetPortToFrame(spriteWorld->workFrameP);
}

/* SWSetPortToWindow (mac _SWSetPortToWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c70
// MAC: _SWSetPortToWindow i386 0x000b74e4 ppc 0x000c3098
void SWSetPortToWindow(SpriteWorldPtr spriteWorld)
{
    if (spriteWorld != NULL) {
        SWSetPortToFrame(spriteWorld->windowFrameP);
        return;
    }
}

#ifdef MACBUILD
/* SWSetSpriteWorldOffscreenDrawProc (mac _SWSetSpriteWorldOffscreenDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteWorldOffscreenDrawProc i386 0x000b7502 ppc 0x000c30a8
OSErr SWSetSpriteWorldOffscreenDrawProc(SpriteWorldPtr spriteWorldP, DrawProcPtr drawProc)
{
    OSErr err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 713);
    SW_ASSERT(drawProc != NULL, 714);

    if (drawProc == BlitPixieAllBitRectDrawProc)
        err = k68kOnlyErr;

    if (spriteWorldP->pixelDepth < 8) {
        if (drawProc == BlitPixieRectDrawProc)
            err = kWrongDepthErr;
    }

    if (err == noErr)
        spriteWorldP->offscreenDrawProc = drawProc;

    SWSetStickyIfError(err);
    return err;
}
#endif

/* SWSetSpriteWorldScreenDrawProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477c90
// MAC: _SWSetSpriteWorldScreenDrawProc i386 0x000b7599 ppc 0x000c3170
#ifdef MACBUILD
OSErr SWSetSpriteWorldScreenDrawProc(SpriteWorldPtr spriteWorldP, DrawProcPtr drawProc)
{
    OSErr err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 745);
    SW_ASSERT(drawProc != NULL, 746);

    if (drawProc == BlitPixieAllBitRectDrawProc)
        err = k68kOnlyErr;

#ifdef MAC1010
    if ((*GetPortPixMap(spriteWorldP->windowFrameP->framePort))->pixelSize < 8 ||
        (*GetPortPixMap(spriteWorldP->windowFrameP->framePort))->pixelSize !=
        spriteWorldP->pixelDepth) {
        if (drawProc == BlitPixieRectDrawProc)
            err = kWrongDepthErr;
    }
#else
    if (useLegacyRenderingPipeline) {
        if ((*GetPortPixMap(spriteWorldP->windowFrameP->framePort))->pixelSize < 8 ||
            (*GetPortPixMap(spriteWorldP->windowFrameP->framePort))->pixelSize !=
            spriteWorldP->pixelDepth) {
            if (drawProc == BlitPixieRectDrawProc)
                err = kWrongDepthErr;
        }
    } else {
        if ((*GetPortPixMap(spriteWorldP->windowFrameP->framePort))->pixelSize < 8) {
            if (drawProc == BlitPixieRectDrawProc)
                err = kWrongDepthErr;
        }
    }
#endif

    if (err == noErr)
        spriteWorldP->screenDrawProc = drawProc;

    SWSetStickyIfError(err);
    return err;
}
#else
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
#endif

/* SWSetPostEraseCallBack. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477ce0
// MAC: _SWSetPostEraseCallBack i386 0x000b76a8 ppc 0x000c32a0
void SWSetPostEraseCallBack(SpriteWorldPtr spriteWorld, WorldProcPtr proc)
{
    spriteWorld->postEraseCallBack = proc;
}

/* SWSetPostDrawCallBack. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477cf0
// MAC: _SWSetPostDrawCallBack i386 0x000b76b6 ppc 0x000c32a8
void SWSetPostDrawCallBack(SpriteWorldPtr spriteWorld, WorldProcPtr proc)
{
    spriteWorld->postDrawCallBack = proc;
}

/* SWSetSpriteWorldMaxFPS. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477d00
// MAC: _SWSetSpriteWorldMaxFPS i386 0x000b76c4 ppc 0x000c32b0
#ifdef MACBUILD
void SWSetSpriteWorldMaxFPS(SpriteWorldPtr spriteWorldP, short framesPerSec)
{
    SW_ASSERT(spriteWorldP != NULL, 811);

    if (framesPerSec > 0 && framesPerSec <= 1000) {
        spriteWorldP->frameInterval = 1000/framesPerSec;
    }

    else {
        spriteWorldP->frameInterval = 0;
    }
}
#else
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
#endif

#define bgRed(sw)   (*(unsigned short *)((char *)(sw) + 0xdf))
#define bgGreen(sw) (*(unsigned short *)((char *)(sw) + 0xe1))
#define bgBlue(sw)  (*(unsigned short *)((char *)(sw) + 0xe3))

/* SWSetBackgroundColor (mac _SWSetBackgroundColor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477d60
// MAC: _SWSetBackgroundColor i386 0x000b7737 ppc 0x000c3324
#ifdef MACBUILD
void SWSetBackgroundColor(SpriteWorldPtr spriteWorldP, RGBColor *color)
{
    SW_ASSERT(spriteWorldP != NULL, 834);
    SW_ASSERT(spriteWorldP->workFrameP != NULL, 835);

    spriteWorldP->backgroundColor = *color;
    spriteWorldP->backgroundValue = SWGetValueFromColor(spriteWorldP->workFrameP, color);
}
#else
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
#endif

#ifdef MACBUILD
/* SWStdWorldClearDrawProc (mac _SWStdWorldClearDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWStdWorldClearDrawProc i386 0x000b77bc ppc 0x000c33a4
void SWStdWorldClearDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect* srcRect, Rect* dstRect)
{
    Rect dstCopyBitsRect = *dstRect;

    if (dstCopyBitsRect.top < dstFrameP->frameRect.top) {
        dstCopyBitsRect.top = dstFrameP->frameRect.top;
        if (dstCopyBitsRect.top >= dstCopyBitsRect.bottom) return;
    }

    if (dstCopyBitsRect.bottom > dstFrameP->frameRect.bottom) {
        dstCopyBitsRect.bottom = dstFrameP->frameRect.bottom;
        if (dstCopyBitsRect.bottom <= dstCopyBitsRect.top) return;
    }

    if (dstCopyBitsRect.left < dstFrameP->frameRect.left) {
        dstCopyBitsRect.left = dstFrameP->frameRect.left;
        if (dstCopyBitsRect.left >= dstCopyBitsRect.right)
            return;
    }

    if (dstCopyBitsRect.right > dstFrameP->frameRect.right) {
        dstCopyBitsRect.right = dstFrameP->frameRect.right;
        if (dstCopyBitsRect.right <= dstCopyBitsRect.left) return;
    }

    SW_ASSERT(gSWCurrentSpriteWorld != NULL , 881);

    RGBBackColor(&gSWCurrentSpriteWorld->backgroundColor);
    EraseRect(&dstCopyBitsRect);
    BackColor(whiteColor);
}
#endif

/* SWStdWorldDrawProc (mac _SWStdWorldDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477de0
// MAC: _SWStdWorldDrawProc i386 0x000b788a ppc 0x000c34bc
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

    CopyBits((BitMapPtr)srcFrameP->framePix,(BitMapPtr)dstFrameP->framePix,
             &srcCopyBitsRect, &dstCopyBitsRect, srcCopy, 0);
}

#ifdef MACBUILD
/* SWInterlacedCopyBitsDrawProc (mac _SWInterlacedCopyBitsDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWInterlacedCopyBitsDrawProc i386 0x000b7997 ppc 0x000c35f0
void SWInterlacedCopyBitsDrawProc(
    FramePtr srcFrameP,
    FramePtr dstFrameP,
    Rect* srcRect,
    Rect* dstRect)
{
    PixMap srcMap = *srcFrameP->framePix;
    PixMap dstMap = *dstFrameP->framePix;
    Rect srcCopyBitsRect = *srcRect;
    Rect dstCopyBitsRect = *dstRect;
    UInt32 srcRowBytes, dstRowBytes;
    SInt32 sourceTopShift, destTopShift;

    if (dstCopyBitsRect.top < dstFrameP->frameRect.top) {
        srcCopyBitsRect.top += dstFrameP->frameRect.top - dstCopyBitsRect.top;
        dstCopyBitsRect.top = dstFrameP->frameRect.top;
        if (dstCopyBitsRect.top >= dstCopyBitsRect.bottom) return;
    }

    if (dstCopyBitsRect.bottom > dstFrameP->frameRect.bottom) {
        srcCopyBitsRect.bottom -= dstCopyBitsRect.bottom - dstFrameP->frameRect.bottom;
        dstCopyBitsRect.bottom = dstFrameP->frameRect.bottom;
        if (dstCopyBitsRect.bottom <= dstCopyBitsRect.top) return;
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
        if (dstCopyBitsRect.right <= dstCopyBitsRect.left) return;
    }

    if (dstFrameP->interlacingIsOn) {
        srcRowBytes = srcMap.rowBytes & 0x3FFF;
        dstRowBytes = dstMap.rowBytes & 0x3FFF;

        {
            srcMap.baseAddr -= (srcRowBytes + srcRowBytes);
            srcMap.bounds.top -= 2;
            srcMap.bounds.bottom -= 2;

            dstMap.baseAddr -= (dstRowBytes + dstRowBytes);
            dstMap.bounds.top -= 2;
            dstMap.bounds.bottom -= 2;
        }

        if (!dstFrameP->skipOddLines) {
            if (!((dstCopyBitsRect.top - dstFrameP->frameRect.top) & 1)) {
                srcCopyBitsRect.top++;
                dstCopyBitsRect.top++;
            }

            srcMap.baseAddr += srcRowBytes;
            dstMap.baseAddr += dstRowBytes;

            srcCopyBitsRect.top--;
            dstCopyBitsRect.top--;

            if ((srcCopyBitsRect.bottom - srcCopyBitsRect.top) & 1)
                srcCopyBitsRect.bottom--;

            if ((dstCopyBitsRect.bottom - dstCopyBitsRect.top) & 1)
                dstCopyBitsRect.bottom--;
        } else {
            if ((dstCopyBitsRect.top - dstFrameP->frameRect.top) & 1) {
                srcCopyBitsRect.top++;
                dstCopyBitsRect.top++;
            }

            if ((srcCopyBitsRect.bottom - srcCopyBitsRect.top) & 1)
                srcCopyBitsRect.bottom++;

            if ((dstCopyBitsRect.bottom - dstCopyBitsRect.top) & 1)
                dstCopyBitsRect.bottom++;
        }

        sourceTopShift = (srcCopyBitsRect.top - srcMap.bounds.top) & 1;
        destTopShift = (dstCopyBitsRect.top - dstMap.bounds.top) & 1;

        if (sourceTopShift)
            srcMap.baseAddr -= srcRowBytes;

        if (destTopShift)
            dstMap.baseAddr -= dstRowBytes;

        srcCopyBitsRect.bottom = (srcCopyBitsRect.bottom - srcMap.bounds.top) >> 1;
        srcCopyBitsRect.top = (srcCopyBitsRect.top - srcMap.bounds.top) >> 1;

        srcMap.bounds.top >>= 1;
        srcMap.bounds.bottom >>= 1;

        dstCopyBitsRect.bottom = (dstCopyBitsRect.bottom - dstMap.bounds.top) >> 1;
        dstCopyBitsRect.top = (dstCopyBitsRect.top - dstMap.bounds.top) >> 1;

        dstMap.bounds.top >>= 1;
        dstMap.bounds.bottom >>= 1;

        srcCopyBitsRect.bottom += (srcMap.bounds.top + sourceTopShift);
        srcCopyBitsRect.top += (srcMap.bounds.top + sourceTopShift);

        dstCopyBitsRect.bottom += (dstMap.bounds.top + destTopShift);
        dstCopyBitsRect.top += (dstMap.bounds.top + destTopShift);

        srcCopyBitsRect.bottom += sourceTopShift;
        srcCopyBitsRect.top += sourceTopShift;
        dstCopyBitsRect.bottom += sourceTopShift;
        dstCopyBitsRect.top += sourceTopShift;

        srcMap.bounds.bottom += 1;
        dstMap.bounds.bottom += 1;

        srcRowBytes += srcRowBytes;
        srcMap.rowBytes = (srcMap.rowBytes & 0xC000) | (srcRowBytes & 0x3FFF);

        dstRowBytes += dstRowBytes;
        dstMap.rowBytes = (dstMap.rowBytes & 0xC000) | (dstRowBytes & 0x3FFF);

        if (srcCopyBitsRect.top >= srcCopyBitsRect.bottom) return;
        if (dstCopyBitsRect.top >= dstCopyBitsRect.bottom) return;

        CopyBits(
            (BitMap*)&srcMap,
            (BitMap*)&dstMap,
            &srcCopyBitsRect,
            &dstCopyBitsRect,
            srcCopy, NULL);
    } else {
        CopyBits(
            (BitMap*)srcFrameP->framePix,
            (BitMap*)dstFrameP->framePix,
            &srcCopyBitsRect,
            &dstCopyBitsRect,
            srcCopy, NULL);
    }
}

/* SWCopyBackgroundToWorkArea (mac _SWCopyBackgroundToWorkArea, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyBackgroundToWorkArea i386 0x000b7e84 ppc 0x000c3a54
void SWCopyBackgroundToWorkArea(SpriteWorldPtr spriteWorldP)
{
    GWorldPtr holdGWorld;
    GDHandle holdGDH;

    SW_ASSERT(spriteWorldP != NULL, 1157);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 1158);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 1159);

    GetGWorld(&holdGWorld, &holdGDH);

    SWSetPortToFrame(spriteWorldP->workFrameP);

    (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                                       spriteWorldP->workFrameP,
                                       &spriteWorldP->backFrameP->frameRect,
                                       &spriteWorldP->workFrameP->frameRect);

    if (useLegacyRenderingPipeline)
        SetGWorld(holdGWorld, holdGDH);
    else
        SetGWorld(holdGWorld, NULL);
}

/* SWChangeWorldRect (mac _SWChangeWorldRect, mac-only). STATE: see the mac gate. */
// MAC: _SWChangeWorldRect i386 0x000b8d0b ppc 0x000c4ca0
OSErr SWChangeWorldRect(
    SpriteWorldPtr spriteWorldP,
    Rect* newWorldRectP,
    Boolean changeOffscreenAreas)
{
    Rect oldWindRect;
    short hChange, vChange;
    OSErr err = noErr;

    SW_ASSERT(spriteWorldP != NULL && newWorldRectP != NULL, 1191);

    if (SW_RECT_WIDTH((*newWorldRectP)) > SW_RECT_WIDTH(spriteWorldP->originalBackRect) ||
         SW_RECT_HEIGHT((*newWorldRectP)) > SW_RECT_HEIGHT(spriteWorldP->originalBackRect)) {
        err = kOutOfRangeErr;
    }

    if (err == noErr) {
        oldWindRect = spriteWorldP->windRect;
        hChange = (oldWindRect.right - oldWindRect.left) -
                    (newWorldRectP->right - newWorldRectP->left);
        vChange = (oldWindRect.bottom - oldWindRect.top) -
                    (newWorldRectP->bottom - newWorldRectP->top);

        spriteWorldP->windRect = *newWorldRectP;

        SWResizeVisScrollRect(spriteWorldP,
                newWorldRectP->right - newWorldRectP->left,
                newWorldRectP->bottom - newWorldRectP->top);

        if (changeOffscreenAreas) {
            spriteWorldP->workFrameP->frameRect.bottom -= vChange;
            spriteWorldP->workFrameP->frameRect.right -= hChange;
            spriteWorldP->backFrameP->frameRect.bottom -= vChange;
            spriteWorldP->backFrameP->frameRect.right -= hChange;

            spriteWorldP->backRect.bottom -= vChange;
            spriteWorldP->backRect.right -= hChange;
        }

        SWWindowMoved(spriteWorldP);
    }

    return err;
}

/* SWRestoreWorldRect (mac _SWRestoreWorldRect, mac-only). STATE: see the mac gate. */
// MAC: _SWRestoreWorldRect i386 0x000b8e59 ppc 0x000c4e08
void SWRestoreWorldRect(SpriteWorldPtr spriteWorldP)
{
    Rect originalWindRect = spriteWorldP->originalWindRect;

    SW_ASSERT(spriteWorldP != NULL, 1195);

    spriteWorldP->windRect = originalWindRect;
    spriteWorldP->workFrameP->frameRect = spriteWorldP->originalBackRect;
    spriteWorldP->backFrameP->frameRect = spriteWorldP->originalBackRect;
    spriteWorldP->backRect = spriteWorldP->originalBackRect;

    SWResizeVisScrollRect(spriteWorldP,
            originalWindRect.right - originalWindRect.left,
            originalWindRect.bottom - originalWindRect.top);

    SWWindowMoved(spriteWorldP);
}
#endif

/* SWWindowMoved (mac _SWWindowMoved). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477f10
// MAC: _SWWindowMoved i386 0x000b8d06 ppc 0x000c4c9c
void SWWindowMoved(SpriteWorldPtr spriteWorldP)
{
    SWWindowFrameMoved(spriteWorldP->windowFrameP, &spriteWorldP->windRect);
}

/* SWWindowFrameMoved (mac _SWWindowFrameMoved). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477f30
// MAC: _SWWindowFrameMoved i386 0x000b7f92 ppc 0x000c3b40
#ifdef MAC1010
void SWWindowFrameMoved(SWFrameRec *windowFrameP, Rect *frameRect)
{
    GrafPtr savedPort;
    Rect windowRect;
    Rect deviceRect;

    SW_ASSERT(windowFrameP != NULL, 1329);
    SW_ASSERT(windowFrameP->isFrameLocked, 1330);

    GetPort(&savedPort);
    windowRect = *frameRect;
    SetPort((GrafPtr)windowFrameP->framePort);
    LocalToGlobal((Point *)&windowRect.top);
    LocalToGlobal((Point *)&windowRect.bottom);
    deviceRect = (**GetMaxDevice(&windowRect)).gdRect;
    if (windowRect.top < deviceRect.top)
        windowRect.top = deviceRect.top;
    if (windowRect.bottom > deviceRect.bottom)
        windowRect.bottom = deviceRect.bottom;
    if (windowRect.left < deviceRect.left)
        windowRect.left = deviceRect.left;
    if (windowRect.right > deviceRect.right)
        windowRect.right = deviceRect.right;
    GlobalToLocal((Point *)&windowRect.top);
    GlobalToLocal((Point *)&windowRect.bottom);
    windowFrameP->frameRect = windowRect;
    if (windowFrameP->isFrameLocked) {
        SWUnlockWindowFrame(windowFrameP);
        SWLockWindowFrame(windowFrameP);
    }
    SetPort(savedPort);
}
#elif defined(MACBUILD)
void SWWindowFrameMoved(SWFrameRec *windowFrameP, Rect *frameRect)
{
}
#else
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
#endif

#ifdef MACBUILD
/* SWUpdateWindow (mac _SWUpdateWindow, mac-only). STATE: see the mac gate. */
// MAC: _SWUpdateWindow i386 0x000b8ee7 ppc 0x000c4e74
void SWUpdateWindow(SpriteWorldPtr spriteWorldP)
{
    GWorldPtr holdGWorld;
    GDHandle holdGDH;

    SW_ASSERT(spriteWorldP != NULL, 1377);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 1378);
    SW_ASSERT(spriteWorldP->windowFrameP->isFrameLocked, 1379);

    if (!sw_depth_ok(spriteWorldP))
        return;

    GetGWorld(&holdGWorld, &holdGDH);

    SWSetPortToFrame(spriteWorldP->windowFrameP);
    spriteWorldP->numTilesChanged = 0;

    (*spriteWorldP->screenDrawProc)(spriteWorldP->workFrameP,
                                    spriteWorldP->windowFrameP,
                                    &spriteWorldP->visScrollRect,
                                    &spriteWorldP->windowFrameP->frameRect);

    if (useLegacyRenderingPipeline)
        SetGWorld(holdGWorld, holdGDH);
    else
        SetGWorld(holdGWorld, NULL);
}
#endif

/* SWUpdateSpriteWorld (mac _SWUpdateSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477fa0
// MAC: _SWUpdateSpriteWorld i386 0x000b9062 ppc 0x000c4ff0
#ifdef MACBUILD
void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP, Boolean updateWindow)
{
    UpdateRectStructPtr curRectStructP, nextRectStructP;
    register SpriteLayerPtr curSpriteLayerP;
    register SpritePtr curSpriteP;
    Rect tempRect;
    GWorldPtr holdGWorld;
    GDHandle holdGDH;
    short curTileLayer;
    DrawProcPtr saveScreenDrawProc;

    SW_ASSERT(spriteWorldP != NULL, 1421);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 1422);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 1423);
    SW_ASSERT(spriteWorldP->windowFrameP->isFrameLocked, 1424);

    if (!sw_depth_ok(spriteWorldP))
        return;

    saveScreenDrawProc = spriteWorldP->screenDrawProc;
#ifdef MAC1010
    SWSetSpriteWorldScreenDrawProc(spriteWorldP, BlitPixieRectDrawProc);
#else
    if (useLegacyRenderingPipeline)
        SWSetSpriteWorldScreenDrawProc(spriteWorldP, BlitPixieRectDrawProc);
    else
        SWSetSpriteWorldScreenDrawProc(spriteWorldP, SWCopyBitsDrawProc);
#endif

    GetGWorld(&holdGWorld, &holdGDH);

    gSWCurrentSpriteWorld = spriteWorldP;
    SWIsWindowInForeground(spriteWorldP);

    SWSetPortToFrame(spriteWorldP->workFrameP);
    (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                                       spriteWorldP->workFrameP,
                                       &spriteWorldP->backFrameP->frameRect,
                                       &spriteWorldP->workFrameP->frameRect);

    if (spriteWorldP->postEraseCallBack != NULL)
        (*spriteWorldP->postEraseCallBack)(spriteWorldP);

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;
    curTileLayer = 0;

    while (curSpriteLayerP != NULL) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        if (curSpriteLayerP->tileLayer > curTileLayer)
            curTileLayer = curSpriteLayerP->tileLayer;

        while (curSpriteP != NULL) {
            if (curSpriteP->curFrameP != NULL) {
                curSpriteP->tileDepth = curTileLayer;

                if (curSpriteP->isVisible && curSpriteP->curFrameP->isFrameLocked) {
                    gSWCurrentSpriteBeingDrawn = curSpriteP;

                    (*curSpriteP->frameDrawProc)(curSpriteP->curFrameP,
                                                 spriteWorldP->workFrameP,
                                                 &curSpriteP->curFrameP->frameRect,
                                                 &curSpriteP->destFrameRect);

                    gSWCurrentSpriteBeingDrawn = NULL;

                    if (spriteWorldP->tilingIsOn &&
                        curSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer) {
                        tempRect = curSpriteP->destFrameRect;

                        if (tempRect.left < spriteWorldP->visScrollRect.left)
                            tempRect.left = spriteWorldP->visScrollRect.left;
                        if (tempRect.right > spriteWorldP->visScrollRect.right)
                            tempRect.right = spriteWorldP->visScrollRect.right;
                        if (tempRect.top < spriteWorldP->visScrollRect.top)
                            tempRect.top = spriteWorldP->visScrollRect.top;
                        if (tempRect.bottom > spriteWorldP->visScrollRect.bottom)
                            tempRect.bottom = spriteWorldP->visScrollRect.bottom;

                        SWDrawTilesAboveSprite(spriteWorldP, &tempRect, curSpriteP->tileDepth);
                    }
                }

                curSpriteP->deltaFrameRect = curSpriteP->destFrameRect;
                curSpriteP->oldFrameRect = curSpriteP->destFrameRect;

                curSpriteP->needsToBeDrawn = false;
                curSpriteP->needsToBeErased = false;
                curSpriteP->onTop = false;
            }

            curSpriteP = curSpriteP->nextSpriteP;
        }

        if (curSpriteLayerP->layerCallBack != NULL)
            (*curSpriteLayerP->layerCallBack)(spriteWorldP);

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    if (spriteWorldP->postDrawCallBack != NULL)
        (*spriteWorldP->postDrawCallBack)(spriteWorldP);

    if (updateWindow) {
        SWSetPortToFrame(spriteWorldP->windowFrameP);

#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        (*spriteWorldP->screenDrawProc)(spriteWorldP->workFrameP,
                                        spriteWorldP->windowFrameP,
                                        &spriteWorldP->visScrollRect,
                                        &spriteWorldP->windowFrameP->frameRect);
#else
        CopyBits(GetPortBitMapForCopyBits(spriteWorldP->workFrameP->framePort),
                 GetPortBitMapForCopyBits(spriteWorldP->windowFrameP->framePort),
                 &spriteWorldP->visScrollRect, &spriteWorldP->windowFrameP->frameRect,
                 srcCopy, NULL);

        if (!useLegacyRenderingPipeline)
            QDFlushPortBuffer(spriteWorldP->windowFrameP->framePort, NULL);
#endif
    }

    nextRectStructP = spriteWorldP->headUpdateRectP;
    while (nextRectStructP != NULL) {
        curRectStructP = nextRectStructP;
        nextRectStructP = curRectStructP->nextRectStructP;
        DisposePtr((Ptr)curRectStructP);
    }
    spriteWorldP->headUpdateRectP = NULL;

    spriteWorldP->numTilesChanged = 0;

    SWSetSpriteWorldScreenDrawProc(spriteWorldP, saveScreenDrawProc);

    gSWCurrentSpriteWorld = NULL;

#ifdef MAC1010
    SetGWorld(holdGWorld, holdGDH);
#else
    if (useLegacyRenderingPipeline)
        SetGWorld(holdGWorld, holdGDH);
    else
        SetGWorld(holdGWorld, NULL);
#endif
}
#else
void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP, Boolean updateWindow)
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
#endif


/* SWProcessSpriteWorld (mac _SWProcessSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004781f0
// MAC: _SWProcessSpriteWorld i386 0x000b8957 ppc 0x000c485c
#ifdef MACBUILD
void SWProcessSpriteWorld(SpriteWorldPtr spriteWorldP)
{
    UnsignedWide curMicroseconds;
    register unsigned long millisecDifference;
    register SpriteLayerPtr curSpriteLayerP;
    register SpritePtr curSpriteP, nextSpriteP;
    register FramePtr newFrameP;

    SW_ASSERT(spriteWorldP != NULL, 1602);

    gSWCurrentSpriteWorld = spriteWorldP;

    Microseconds(&curMicroseconds);
    if (curMicroseconds.hi > spriteWorldP->lastMicroseconds.hi) {
        millisecDifference =
            (0xffffffff - (spriteWorldP->lastMicroseconds.lo - curMicroseconds.lo)) / 1000;
    } else {
        millisecDifference = (curMicroseconds.lo - spriteWorldP->lastMicroseconds.lo) / 1000;
    }

    if (millisecDifference > 0) {
        spriteWorldP->runningTimeCount += millisecDifference;
        spriteWorldP->lastMicroseconds.lo = curMicroseconds.lo;
        spriteWorldP->lastMicroseconds.hi = curMicroseconds.hi;
    }

    if ((spriteWorldP->runningTimeCount - spriteWorldP->timeOfLastFrame >=
            spriteWorldP->frameInterval)) {
        spriteWorldP->frameHasOccurred = true;
        spriteWorldP->timeOfLastFrame = spriteWorldP->runningTimeCount;
    } else {
        spriteWorldP->frameHasOccurred = false;
        return;
    }

    if (spriteWorldP->deadSpriteLayerP->headSpriteP != NULL) {
        SWFindSpritesToBeRemoved(spriteWorldP);
    }

    if (spriteWorldP->tileChangeProc != NULL) {
        (*spriteWorldP->tileChangeProc)(spriteWorldP);
    }

    spriteWorldP->thereAreNonScrollingLayers = false;

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL) {
        if (curSpriteLayerP->layerIsNonScrolling) {
            spriteWorldP->thereAreNonScrollingLayers = true;
        } else if (!curSpriteLayerP->isPaused) {
            curSpriteP = curSpriteLayerP->headSpriteP;

            while (curSpriteP != NULL) {
                nextSpriteP = curSpriteP->nextSpriteP;

                if (curSpriteP->curFrameP != NULL) {
                    if (!curSpriteP->curFrameP->isFrameLocked)
                        SWLockFrame(curSpriteP->curFrameP);

                    if (curSpriteP->frameTimeInterval >= 0 &&
                        (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastFrameChange) >=
                        curSpriteP->frameTimeInterval && curSpriteP->curFrameP->isFrameLocked) {
                        register long currentFrameIndex;

                        curSpriteP->timeOfLastFrameChange = spriteWorldP->runningTimeCount;

                        currentFrameIndex = curSpriteP->curFrameIndex;
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

                        if (curSpriteP->frameChangeProc != NULL) {
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

                        if (curSpriteP->curFrameP != newFrameP) {
                            SWSetCurrentFrameIndex(curSpriteP, curSpriteP->curFrameIndex);
                        }
                    }

                    if (curSpriteP->moveTimeInterval >= 0 &&
                        (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastMove) >=
                        curSpriteP->moveTimeInterval) {
                        curSpriteP->timeOfLastMove = spriteWorldP->runningTimeCount;

                        if (curSpriteP->spriteMoveProc != NULL) {
                            (*curSpriteP->spriteMoveProc)(curSpriteP);
                        }
                    }
                }
                curSpriteP = nextSpriteP;
            }
        }
        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    SWIsWindowInForeground(spriteWorldP);
    gSWCurrentSpriteWorld = NULL;
}
#else
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
#endif

#ifdef MACBUILD
/* SWProcessNonScrollingLayers (mac _SWProcessNonScrollingLayers, mac-only). STATE: see the mac gate. */
// MAC: _SWProcessNonScrollingLayers i386 0x000b824e ppc 0x000c3e84
void SWProcessNonScrollingLayers(SpriteWorldPtr spriteWorldP)
{
    register SpriteLayerPtr curSpriteLayerP;

    if (spriteWorldP->thereAreNonScrollingLayers == false)
        return;

    curSpriteLayerP = spriteWorldP->headSpriteLayerP;

    while (curSpriteLayerP != NULL) {
        if (curSpriteLayerP->layerIsNonScrolling) {
            SWProcessSpriteLayer(spriteWorldP, curSpriteLayerP);
        }

        curSpriteLayerP = curSpriteLayerP->nextSpriteLayerP;
    }

    spriteWorldP->thereAreNonScrollingLayers = false;
}

/* SWProcessSpriteLayer (mac _SWProcessSpriteLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWProcessSpriteLayer i386 0x000b80e1 ppc 0x000c3cb4
void SWProcessSpriteLayer(SpriteWorldPtr spriteWorldP, SpriteLayerPtr curSpriteLayerP)
{
    register SpritePtr curSpriteP, nextSpriteP;
    register FramePtr newFrameP;

    if (!curSpriteLayerP->isPaused) {
        curSpriteP = curSpriteLayerP->headSpriteP;

        while (curSpriteP != NULL) {
            nextSpriteP = curSpriteP->nextSpriteP;

            if (curSpriteP->frameTimeInterval >= 0 &&
                (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastFrameChange) >=
                curSpriteP->frameTimeInterval && curSpriteP->curFrameP->isFrameLocked) {
                register long currentFrameIndex;

                curSpriteP->timeOfLastFrameChange = spriteWorldP->runningTimeCount;

                currentFrameIndex = curSpriteP->curFrameIndex;
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

                if (curSpriteP->frameChangeProc != NULL) {
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

                if (curSpriteP->curFrameP != newFrameP) {
                    SWSetCurrentFrameIndex(curSpriteP, curSpriteP->curFrameIndex);
                }
            }

            if (curSpriteP->moveTimeInterval >= 0 &&
                (spriteWorldP->runningTimeCount - curSpriteP->timeOfLastMove) >=
                curSpriteP->moveTimeInterval) {
                curSpriteP->timeOfLastMove = spriteWorldP->runningTimeCount;

                if (curSpriteP->spriteMoveProc != NULL) {
                    (*curSpriteP->spriteMoveProc)(curSpriteP);
                }
            }

            curSpriteP = nextSpriteP;
        }
    }
}
#endif



#ifndef MACBUILD
SpriteWorldRec *gSWCurrentSpriteWorld;                   /* 0x008706f0 */
#endif

/* SWAnimateSpriteWorld (mac _SWAnimateSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478480
// MAC: _SWAnimateSpriteWorld i386 0x000b95e6 ppc 0x000c5550
#ifdef MACBUILD
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
        SW_ASSERT_FAIL("SpriteWorld.c", 1984, 1960);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 1985, 1961);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 1986, 1962);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 1987, 1963);

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
                        curSpriteP->needsToBeErased) {
                        if (headActiveSpriteP == 0)
                            headActiveSpriteP = curSpriteP;

                        if (curActiveSpriteP != 0)
                            curActiveSpriteP->nextActiveSpriteP = curSpriteP;

                        curActiveSpriteP = curSpriteP;

                        if (curSpriteP->justBecameVisible) {
                            if (!curSpriteP->needsToBeErased) {
                                curSpriteP->oldFrameRect = curSpriteP->destFrameRect;
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
                            if (temp != 0) {
                                curSpriteP->deltaFrameRect.right +=
                                        (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                            }

                            curSpriteP->oldFrameRect.left &=
                                (spriteWorldP->workFrameP->leftAlignFactor);

                            temp = curSpriteP->oldFrameRect.right &
                                spriteWorldP->workFrameP->rightAlignFactor;
                            if (temp != 0) {
                                curSpriteP->oldFrameRect.right +=
                                    (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
                            }
                        }

                        if ((curSpriteP->deltaFrameRect.right - curSpriteP->deltaFrameRect.left) *
                            (curSpriteP->deltaFrameRect.bottom - curSpriteP->deltaFrameRect.top) >
                            (curSpriteP->oldFrameRect.right - curSpriteP->oldFrameRect.left) *
                            (curSpriteP->oldFrameRect.bottom - curSpriteP->oldFrameRect.top) * 2) {
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
                                    spriteWorldP->workFrameP->frameRect.bottom) {
                                (*spriteWorldP->offscreenDrawProc)(
                                        spriteWorldP->backFrameP,
                                        spriteWorldP->workFrameP,
                                        &curSpriteP->oldFrameRect,
                                        &curSpriteP->oldFrameRect);
                            }
                        }
                    } else if (curSpriteP->isVisible && !curSpriteP->onTop) {
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
                            spriteWorldP->workFrameP->frameRect.bottom) {
                        gSWCurrentSpriteBeingDrawn = curSpriteP;

                        (*curSpriteP->frameDrawProc)(
                                curSpriteP->curFrameP,
                                spriteWorldP->workFrameP,
                                &curSpriteP->curFrameP->frameRect,
                                &curSpriteP->destFrameRect);

                        gSWCurrentSpriteBeingDrawn = 0;
                    }
                } else {
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
                screenDestRect = curSpriteP->destFrameRect;

                oldScreenDestRect = curSpriteP->oldFrameRect;

                oldScreenDestRect.left += spriteWorldP->windRect.left;
                oldScreenDestRect.right += spriteWorldP->windRect.left;
                oldScreenDestRect.top += spriteWorldP->windRect.top;
                oldScreenDestRect.bottom += spriteWorldP->windRect.top;
            } else {
                screenDestRect = curSpriteP->deltaFrameRect;
            }

            screenDestRect.left += spriteWorldP->windRect.left;
            screenDestRect.right += spriteWorldP->windRect.left;
            screenDestRect.top += spriteWorldP->windRect.top;
            screenDestRect.bottom += spriteWorldP->windRect.top;

            if (screenDestRect.left <= spriteWorldP->windowFrameP->frameRect.right &&
                screenDestRect.right >= spriteWorldP->windowFrameP->frameRect.left &&
                screenDestRect.bottom >= spriteWorldP->windowFrameP->frameRect.top &&
                screenDestRect.top <= spriteWorldP->windowFrameP->frameRect.bottom) {
                if (curSpriteP->srD3) {
                    if (useLegacyRenderingPipeline)
                        BlitPixieRectDrawProc(spriteWorldP->workFrameP, spriteWorldP->windowFrameP,
                                              &curSpriteP->destFrameRect, &screenDestRect);
                    else
                        SWCopyBitsDrawProc(spriteWorldP->workFrameP, spriteWorldP->windowFrameP,
                                           &curSpriteP->destFrameRect, &screenDestRect);
                } else {
                    if (useLegacyRenderingPipeline)
                        BlitPixieRectDrawProc(spriteWorldP->workFrameP, spriteWorldP->windowFrameP,
                                              &curSpriteP->deltaFrameRect, &screenDestRect);
                    else
                        SWCopyBitsDrawProc(spriteWorldP->workFrameP, spriteWorldP->windowFrameP,
                                           &curSpriteP->deltaFrameRect, &screenDestRect);
                }
            }

            if (curSpriteP->srD3) {
                if (oldScreenDestRect.left <= spriteWorldP->windowFrameP->frameRect.right &&
                    oldScreenDestRect.right >= spriteWorldP->windowFrameP->frameRect.left &&
                    oldScreenDestRect.bottom >= spriteWorldP->windowFrameP->frameRect.top &&
                    oldScreenDestRect.top <= spriteWorldP->windowFrameP->frameRect.bottom) {
                    if (useLegacyRenderingPipeline)
                        BlitPixieRectDrawProc(spriteWorldP->workFrameP, spriteWorldP->windowFrameP,
                                              &curSpriteP->oldFrameRect, &oldScreenDestRect);
                    else {
                        SWSetPortToFrame(spriteWorldP->windowFrameP);
                        SWCopyBitsDrawProc(spriteWorldP->workFrameP, spriteWorldP->windowFrameP,
                                           &curSpriteP->oldFrameRect, &oldScreenDestRect);
                    }
                }
            }

            curSpriteP->deltaFrameRect = curSpriteP->destFrameRect;

            curSpriteP->oldFrameRect = curSpriteP->destFrameRect;

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
#else
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
#endif

/* The srcSectRect copy goes through a POINTER local -- `mov edi,[ebx+0x50] / add
   edi,0x22' then four movzx/mov word pairs. SW_MIN/SW_MAX stay macros: they
   double-evaluate, and the exe re-reads deltaFrameRect.left and .bottom for the pair. */

/* SWCheckIdleSpriteOverlap (mac _SWCheckIdleSpriteOverlap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478bb0
// MAC: _SWCheckIdleSpriteOverlap i386 0x000b82a0 ppc 0x000c3efc
#ifdef MACBUILD
void SWCheckIdleSpriteOverlap(
    SpriteWorldPtr spriteWorldP,
    SpritePtr idleSpriteP,
    SpritePtr headActiveSpriteP)
{
    register SpritePtr activeSpriteP = headActiveSpriteP;
    Rect srcSectRect, dstSectRect;

    while (activeSpriteP != NULL) {
        if ((idleSpriteP->oldFrameRect.top < activeSpriteP->deltaFrameRect.bottom) &&
             (idleSpriteP->oldFrameRect.bottom > activeSpriteP->deltaFrameRect.top) &&
             (idleSpriteP->oldFrameRect.left < activeSpriteP->deltaFrameRect.right) &&
             (idleSpriteP->oldFrameRect.right > activeSpriteP->deltaFrameRect.left)) {
            dstSectRect.left =
                    SW_MAX(idleSpriteP->destFrameRect.left, activeSpriteP->deltaFrameRect.left);
            dstSectRect.top =
                    SW_MAX(idleSpriteP->destFrameRect.top, activeSpriteP->deltaFrameRect.top);
            dstSectRect.right =
                    SW_MIN(idleSpriteP->destFrameRect.right, activeSpriteP->deltaFrameRect.right);
            dstSectRect.bottom =
                    SW_MIN(idleSpriteP->destFrameRect.bottom, activeSpriteP->deltaFrameRect.bottom);

            srcSectRect = idleSpriteP->curFrameP->frameRect;

            srcSectRect.left += (dstSectRect.left - idleSpriteP->destFrameRect.left);
            srcSectRect.top += (dstSectRect.top - idleSpriteP->destFrameRect.top);
            srcSectRect.right -= (idleSpriteP->destFrameRect.right - dstSectRect.right);
            srcSectRect.bottom -= (idleSpriteP->destFrameRect.bottom - dstSectRect.bottom);

            gSWCurrentSpriteBeingDrawn = idleSpriteP;

            (*idleSpriteP->frameDrawProc)(
                    idleSpriteP->curFrameP,
                    spriteWorldP->workFrameP,
                    &srcSectRect,
                    &dstSectRect);

            gSWCurrentSpriteBeingDrawn = NULL;

            if (spriteWorldP->tilingIsOn &&
                idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer) {
                if (dstSectRect.left < spriteWorldP->visScrollRect.left)
                    dstSectRect.left = spriteWorldP->visScrollRect.left;
                if (dstSectRect.right > spriteWorldP->visScrollRect.right)
                    dstSectRect.right = spriteWorldP->visScrollRect.right;
                if (dstSectRect.top < spriteWorldP->visScrollRect.top)
                    dstSectRect.top = spriteWorldP->visScrollRect.top;
                if (dstSectRect.bottom > spriteWorldP->visScrollRect.bottom)
                    dstSectRect.bottom = spriteWorldP->visScrollRect.bottom;

                SWDrawTilesAboveSprite(spriteWorldP, &dstSectRect, idleSpriteP->tileDepth);
            }
        }

        activeSpriteP = activeSpriteP->nextActiveSpriteP;
    }
}
#else
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
#endif

#ifdef MACBUILD
/* SWCheckIdleSpritesWithTiles (mac _SWCheckIdleSpritesWithTiles, mac-only). STATE: see the mac gate. */
// MAC: _SWCheckIdleSpritesWithTiles i386 0x000b844f ppc 0x000c4110
void SWCheckIdleSpritesWithTiles(SpriteWorldPtr spriteWorldP, SpritePtr headIdleSpriteP)
{
    Rect srcSectRect, dstSectRect;
    register SpritePtr idleSpriteP;
    short srcHorizOffset;
    short srcVertOffset;
    Rect *changedRectP;
    short index;

    changedRectP = spriteWorldP->changedTiles;
    for (index = 0; index < spriteWorldP->numTilesChanged; index++, changedRectP++) {
        idleSpriteP = headIdleSpriteP;

        while (idleSpriteP != NULL) {
            if ((idleSpriteP->oldFrameRect.top < changedRectP->bottom) &&
                 (idleSpriteP->oldFrameRect.bottom > changedRectP->top) &&
                 (idleSpriteP->oldFrameRect.left < changedRectP->right) &&
                 (idleSpriteP->oldFrameRect.right > changedRectP->left)) {
                dstSectRect.left =
                        SW_MAX(idleSpriteP->oldFrameRect.left, changedRectP->left);
                dstSectRect.top =
                        SW_MAX(idleSpriteP->oldFrameRect.top, changedRectP->top);
                dstSectRect.right =
                        SW_MIN(idleSpriteP->oldFrameRect.right, changedRectP->right);
                dstSectRect.bottom =
                        SW_MIN(idleSpriteP->oldFrameRect.bottom, changedRectP->bottom);

                srcSectRect = dstSectRect;

                srcHorizOffset = idleSpriteP->curFrameP->frameRect.left;
                srcVertOffset = idleSpriteP->curFrameP->frameRect.top;

                srcSectRect.left -= (idleSpriteP->oldFrameRect.left - srcHorizOffset);
                srcSectRect.right -= (idleSpriteP->oldFrameRect.left - srcHorizOffset);
                srcSectRect.top -= (idleSpriteP->oldFrameRect.top - srcVertOffset);
                srcSectRect.bottom -= (idleSpriteP->oldFrameRect.top - srcVertOffset);

                gSWCurrentSpriteBeingDrawn = idleSpriteP;

                (*idleSpriteP->frameDrawProc)(
                        idleSpriteP->curFrameP,
                        spriteWorldP->workFrameP,
                        &srcSectRect,
                        &dstSectRect);

                gSWCurrentSpriteBeingDrawn = NULL;

                if (spriteWorldP->tilingIsOn &&
                    idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer) {
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

/* SWCheckIdleSpritesWithRects (mac _SWCheckIdleSpritesWithRects, mac-only). STATE: see the mac gate. */
// MAC: _SWCheckIdleSpritesWithRects i386 0x000b85e7 ppc 0x000c435c
void SWCheckIdleSpritesWithRects(SpriteWorldPtr spriteWorldP, SpritePtr headIdleSpriteP)
{
    UpdateRectStructPtr curRectStructP;
    register SpritePtr idleSpriteP;
    Rect srcSectRect, dstSectRect;
    short srcHorizOffset;
    short srcVertOffset;
    Rect *changedRectP;

    curRectStructP = spriteWorldP->headUpdateRectP;

    while (curRectStructP != NULL) {
        changedRectP = &curRectStructP->updateRect;
        idleSpriteP = headIdleSpriteP;

        while (idleSpriteP != NULL) {
            if ((idleSpriteP->oldFrameRect.top < changedRectP->bottom) &&
                 (idleSpriteP->oldFrameRect.bottom > changedRectP->top) &&
                 (idleSpriteP->oldFrameRect.left < changedRectP->right) &&
                 (idleSpriteP->oldFrameRect.right > changedRectP->left)) {
                dstSectRect.left =
                    SW_MAX(idleSpriteP->oldFrameRect.left, changedRectP->left);
                dstSectRect.top =
                    SW_MAX(idleSpriteP->oldFrameRect.top, changedRectP->top);
                dstSectRect.right =
                    SW_MIN(idleSpriteP->oldFrameRect.right, changedRectP->right);
                dstSectRect.bottom =
                    SW_MIN(idleSpriteP->oldFrameRect.bottom, changedRectP->bottom);

                srcSectRect = dstSectRect;

                srcHorizOffset = idleSpriteP->curFrameP->frameRect.left;
                srcVertOffset = idleSpriteP->curFrameP->frameRect.top;

                srcSectRect.left -= (idleSpriteP->oldFrameRect.left - srcHorizOffset);
                srcSectRect.right -= (idleSpriteP->oldFrameRect.left - srcHorizOffset);
                srcSectRect.top -= (idleSpriteP->oldFrameRect.top - srcVertOffset);
                srcSectRect.bottom -= (idleSpriteP->oldFrameRect.top - srcVertOffset);

                gSWCurrentSpriteBeingDrawn = idleSpriteP;

                (*idleSpriteP->frameDrawProc)(
                        idleSpriteP->curFrameP,
                        spriteWorldP->workFrameP,
                        &srcSectRect,
                        &dstSectRect);

                gSWCurrentSpriteBeingDrawn = NULL;

                if (spriteWorldP->tilingIsOn &&
                    idleSpriteP->tileDepth <= spriteWorldP->lastActiveTileLayer) {
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
#endif

/* SWFindSpritesToBeRemoved (mac _SWFindSpritesToBeRemoved). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478cf0
// MAC: _SWFindSpritesToBeRemoved i386 0x000b8042 ppc 0x000c3be8
#ifdef MACBUILD
void SWFindSpritesToBeRemoved(SpriteWorldPtr spriteWorldP)
{
    SpritePtr curSpriteP, nextSpriteP;

    curSpriteP = spriteWorldP->deadSpriteLayerP->headSpriteP;

    while (curSpriteP != NULL) {
        nextSpriteP = curSpriteP->nextSpriteP;

        SW_ASSERT(curSpriteP->spriteRemoval != kSWDontRemoveSprite, 2600);

        if (curSpriteP->needsToBeErased == false) {
            SWRemoveSprite(curSpriteP);

            if (curSpriteP->spriteRemoval == kSWRemoveAndDisposeSprite)
                SWDisposeSprite(&curSpriteP);
            else
                curSpriteP->spriteRemoval = kSWDontRemoveSprite;
        }

        curSpriteP = nextSpriteP;
    }
}
#else
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
#endif

/* SWFlagRectAsChanged (mac _SWFlagRectAsChanged). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478d70
// MAC: _SWFlagRectAsChanged i386 0x000b8758 ppc 0x000c4594
#ifdef MACBUILD
OSErr SWFlagRectAsChanged(SpriteWorldPtr spriteWorldP, Rect* theChangedRect)
{
    UpdateRectStructPtr curRectStructP,
                            newUpdateRectP;
    short temp;
    OSErr err = noErr;

    theChangedRect->left &= (spriteWorldP->workFrameP->leftAlignFactor);

    temp = theChangedRect->right & spriteWorldP->workFrameP->rightAlignFactor;
    if (temp != 0) {
        theChangedRect->right += (spriteWorldP->workFrameP->rightAlignFactor + 1) - temp;
    }

    newUpdateRectP = (UpdateRectStructPtr)NewPtr(sizeof(UpdateRectStruct));
    if (newUpdateRectP != NULL) {
        newUpdateRectP->updateRect = *theChangedRect;
        newUpdateRectP->nextRectStructP = NULL;

        if (spriteWorldP->headUpdateRectP == NULL) {
            spriteWorldP->headUpdateRectP = newUpdateRectP;
        } else {
            curRectStructP = spriteWorldP->headUpdateRectP;
            while (curRectStructP->nextRectStructP != NULL) {
                curRectStructP = curRectStructP->nextRectStructP;
            }
            curRectStructP->nextRectStructP = newUpdateRectP;
        }
    } else {
        err = MemError();
    }

    return err;
}
#else
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
#endif

#ifdef MACBUILD
/* SWFlagScrollingRectAsChanged (mac _SWFlagScrollingRectAsChanged, mac-only). STATE: see the mac gate. */
// MAC: _SWFlagScrollingRectAsChanged i386 0x000b87e2 ppc 0x000c4688
OSErr SWFlagScrollingRectAsChanged(SpriteWorldPtr spriteWorldP, Rect* theChangedRect)
{
    UpdateRectStructPtr curRectStructP,
                            newUpdateRectP;
    OSErr err = noErr;

    newUpdateRectP = (UpdateRectStructPtr)NewPtr(sizeof(UpdateRectStruct));
    if (newUpdateRectP != NULL) {
        newUpdateRectP->updateRect = *theChangedRect;
        newUpdateRectP->nextRectStructP = NULL;

        if (spriteWorldP->headUpdateRectP == NULL) {
            spriteWorldP->headUpdateRectP = newUpdateRectP;
        } else {
            curRectStructP = spriteWorldP->headUpdateRectP;
            while (curRectStructP->nextRectStructP != NULL) {
                curRectStructP = curRectStructP->nextRectStructP;
            }
            curRectStructP->nextRectStructP = newUpdateRectP;
        }
    } else {
        err = MemError();
    }

    return err;
}

/* SWSetCleanUpSpriteWorld (mac _SWSetCleanUpSpriteWorld, mac-only). STATE: see the mac gate. */
// MAC: _SWSetCleanUpSpriteWorld i386 0x000b8842 ppc 0x000c4738
void SWSetCleanUpSpriteWorld(SpriteWorldPtr spriteWorldP)
{
    gSWCleanUpSpriteWorldP = spriteWorldP;
}

/* SWSetCleanUpFunction (mac _SWSetCleanUpFunction, mac-only). STATE: see the mac gate. */
// MAC: _SWSetCleanUpFunction i386 0x000b884f ppc 0x000c4744
void SWSetCleanUpFunction(CleanUpCallBackPtr callBackP)
{
    gSWCleanUpCallBackP = callBackP;
}

/* SWGetSpriteWorldVersion (mac _SWGetSpriteWorldVersion, mac-only). STATE: see the mac gate. */
// MAC: _SWGetSpriteWorldVersion i386 0x000b885c ppc 0x000c4750
unsigned long SWGetSpriteWorldVersion(void)
{
    return 0x02318000;
}
#endif

/* SWIsWindowInForeground (mac _SWIsWindowInForeground). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478e20
// MAC: _SWIsWindowInForeground i386 0x000b88cd ppc 0x000c47c8
#ifdef MACBUILD
Boolean SWIsWindowInForeground(SpriteWorldPtr spriteWorldP)
{
    if (!AreWeFrontProcess()) {
        spriteWorldP->fastBlit = false;
    } else if ((FrontWindow)() != NULL) {
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        if (spriteWorldP->windowFrameP->framePort == (CGrafPtr)(FrontWindow)()) {
            if (EmptyRgn(((WindowPeek)(FrontWindow)())->contRgn))
#else
        if (spriteWorldP->windowFrameP->framePort == GetWindowGrafPort((FrontWindow)())) {
            GetWindowRegion((FrontWindow)(), kWindowContentRgn, spriteWorldP->windowRgn);
            if (EmptyRgn(spriteWorldP->windowRgn))
#endif
                spriteWorldP->fastBlit = false;
            else
                spriteWorldP->fastBlit = true;
        } else {
            spriteWorldP->fastBlit = false;
        }
    } else {
        spriteWorldP->fastBlit = false;
    }

    return spriteWorldP->fastBlit;
}
#else
Boolean SWIsWindowInForeground(SpriteWorldPtr spriteWorldP)
{
    spriteWorldP->fastBlit = 1;

    return spriteWorldP->fastBlit;
}
#endif

/* AreWeFrontProcess (mac _AreWeFrontProcess). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478e40
// MAC: _AreWeFrontProcess i386 0x000b8866 ppc 0x000c475c
#ifdef MACBUILD
Boolean AreWeFrontProcess(void)
{
    ProcessSerialNumber currentPSN, frontPSN;
    Boolean sameProcess;

    if (GetCurrentProcess(&currentPSN) == procNotFound)
        return true;
    if (GetFrontProcess(&frontPSN) == procNotFound)
        return true;
    SameProcess(&currentPSN, &frontPSN, &sameProcess);
    return sameProcess;
}
#else
Boolean AreWeFrontProcess(void)
{
    return 1;
}
#endif

/* SWHideAllSprites (mac _SWHideAllSprites). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478e50
// MAC: _SWHideAllSprites i386 0x000b8c09 ppc 0x000c4ba4
void SWHideAllSprites(SpriteWorldPtr spriteWorldP)
{
    SpriteLayerRec *curSpriteLayerP;
    SpritePtr curSpriteP;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SpriteWorld.c", 2847, 2823);
    if (!spriteWorldP->backFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 2848, 2824);
    if (!spriteWorldP->workFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 2849, 2825);
    if (!spriteWorldP->windowFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SpriteWorld.c", 2850, 2826);

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
