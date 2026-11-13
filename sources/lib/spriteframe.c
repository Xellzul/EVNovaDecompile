// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* spriteframe.c -- SpriteWorld's SpriteFrame.c, tu-map object O15: frame creation, disposal and locking. Notes: Research/notes/spriteframe.md */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"
#include "cefix.h"
#ifdef MACBUILD
#include "render.h"
#endif

/* SWCreateFrame (mac _SWCreateFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476400
// MAC: _SWCreateFrame i386 0x000ad556 ppc 0x000b8a00
#ifdef MACBUILD
short SWCreateFrame(void *theGDH, SWFrameRec **newFrameP, Rect *frameRect,
                    short depth, unsigned char createGWorld)
{
    OSErr err = noErr;
    GWorldPtr frameGWorld;
    FramePtr tempFrameP;
    int32_t numScanLines;

    SW_ASSERT(theGDH != NULL, 64);
    SW_ASSERT(frameRect->top >= 0 && frameRect->left >= 0 &&
              frameRect->right > frameRect->left && frameRect->bottom > frameRect->top, 66);

    *newFrameP = NULL;

    numScanLines = frameRect->bottom - frameRect->top;

#ifdef MAC1010
    tempFrameP = (FramePtr)NewPtrClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * (numScanLines + 1)));
#else
    tempFrameP = (FramePtr)NewPtrClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * numScanLines));
#endif

    if (tempFrameP != NULL) {
        tempFrameP->frameRect = *frameRect;
        tempFrameP->frameDepth = depth;
        tempFrameP->pixelShift = depth >> 4;
        tempFrameP->numScanLines = numScanLines;
        tempFrameP->collisionInset.left = 0;
        tempFrameP->collisionInset.top = 0;
        tempFrameP->collisionInset.right = 0;
        tempFrameP->collisionInset.bottom = 0;
        tempFrameP->usesCollisionInset = false;
        tempFrameP->rleDataH = NULL;

        if (createGWorld) {
            if (useLegacyRenderingPipeline) {
                if (SWGetPixBitDepth((**(GDHandle)theGDH).gdPMap) == depth)
                    err = (NewGWorld)(&frameGWorld, depth, frameRect, NULL, theGDH, noNewDevice);
                else
                    err = (NewGWorld)(&frameGWorld, depth, frameRect, NULL, NULL, 0);
            } else {
                err = (NewGWorld)(&frameGWorld, 16, frameRect, NULL, NULL, kNativeEndianPixMap);
            }

            if (err == noErr) {
                tempFrameP->framePort = frameGWorld;
                tempFrameP->frameDevice = GetGWorldDevice(frameGWorld);
                tempFrameP->framePixHndl = GetGWorldPixMap(frameGWorld);

                SWInitializeFrame(tempFrameP);
            }
        } else {
            tempFrameP->framePort = NULL;
            tempFrameP->frameDevice = NULL;
            tempFrameP->framePixHndl = NULL;
            err = noErr;
        }

        if (err == noErr) {
            tempFrameP->hotSpotH = 0;
            tempFrameP->hotSpotV = 0;
            tempFrameP->tileMaskIsSolid = false;
            tempFrameP->sharesGWorld = false;
            tempFrameP->isFrameLocked = false;
            tempFrameP->isWindowFrame = false;
            tempFrameP->interlacingIsOn = false;
            tempFrameP->skipOddLines = true;

            tempFrameP->rleDataH = NULL;
            tempFrameP->rleTokenStart = NULL;

            tempFrameP->useCount = 0;

            *newFrameP = tempFrameP;
        }
    } else {
        err = MemError();
    }

    if (err != noErr) {
        if (tempFrameP != NULL) {
            if (tempFrameP->framePort != NULL)
                DisposeGWorld(tempFrameP->framePort);
            DisposePtr((Ptr)tempFrameP);
        }
    }

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateFrame(void *theGDH, SWFrameRec **newFrameP, Rect *frameRect,
                    short depth, unsigned char createGWorld)
{
    short err = 0;
    CGrafPtr frameGWorld;
    SWFrameRec *tempFrameP;
    int32_t numScanLines;

    if (!(frameRect->top >= 0 && frameRect->left >= 0 &&
            frameRect->right > frameRect->left && frameRect->bottom > frameRect->top))
        SWAssertFail("SpriteFrame.c", 72);

    *newFrameP = 0;

    numScanLines = frameRect->bottom - frameRect->top;

    tempFrameP = (SWFrameRec *)AllocAPointerClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * (numScanLines + 1)));

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
#ifdef CEFIX /* F11.02 */
            err = CE_NewGWorld(&frameGWorld, depth, frameRect, theGDH);
#else
            err = NewGWorld(&frameGWorld, depth, frameRect, theGDH);
#endif

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
#endif

/* SW_MIN/SW_MAX are the donor's double-evaluating macros over a non-inlinable accessor,
   so each clip mentions its window bound twice and the exe parks the second call out of
   line. Write the macro; do not hoist the call into a local. */


/* SWCreateWindowFrame (mac _SWCreateWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004765d0
// MAC: _SWCreateWindowFrame i386 0x000ad32f ppc 0x000b877c
#ifdef MACBUILD
short SWCreateWindowFrame(CWindowPtr srcWindowP, SWFrameRec **newFrameP,
                          Rect *frameRect, short maxHeight)
{
    OSErr err = noErr;
    FramePtr tempFrameP = NULL;
    int32_t numScanLines;

    SW_ASSERT(frameRect->top >= 0 && frameRect->left >= 0, 183);
    SW_ASSERT(frameRect->right > frameRect->left && frameRect->bottom > frameRect->top, 184);

    *newFrameP = NULL;

    numScanLines = (GetCWindowPortRect((WindowRef)srcWindowP)->bottom -
                    GetCWindowPortRect((WindowRef)srcWindowP)->top);
    numScanLines = SW_MAX(numScanLines, maxHeight);

    frameRect->top = SW_MAX(frameRect->top, GetCWindowPortRect((WindowRef)srcWindowP)->top);
    frameRect->left = SW_MAX(frameRect->left, GetCWindowPortRect((WindowRef)srcWindowP)->left);
    frameRect->bottom = SW_MIN(frameRect->bottom, GetCWindowPortRect((WindowRef)srcWindowP)->bottom);
    frameRect->right = SW_MIN(frameRect->right, GetCWindowPortRect((WindowRef)srcWindowP)->right);

    tempFrameP = (FramePtr)NewPtrClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * numScanLines));

    if (tempFrameP != NULL) {
        tempFrameP->framePort = GetWindowPort(srcWindowP);
        tempFrameP->frameDevice = GetMainDevice();
        tempFrameP->framePixHndl = (PixMapHandle)GetCWindowPixMap((WindowRef)srcWindowP);
        if (tempFrameP->framePixHndl == NULL)
            tempFrameP->framePixHndl = GetGWorldPixMap(GetWindowPort(srcWindowP));
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        tempFrameP->frameDepth = (**srcWindowP->portPixMap).pixelSize;
        tempFrameP->frameRect = *frameRect;
#else
        if (useLegacyRenderingPipeline)
            tempFrameP->frameDepth = (**GetCWindowPixMap(srcWindowP)).pixelSize;
        else
            tempFrameP->frameDepth = 16;
        tempFrameP->frameRect = *frameRect;
        OffsetRect(&tempFrameP->frameRect, -tempFrameP->frameRect.left, -tempFrameP->frameRect.top);
#endif
        tempFrameP->numScanLines = numScanLines;
        tempFrameP->sharesGWorld = false;
        tempFrameP->isWindowFrame = true;
        tempFrameP->isFrameLocked = false;
        tempFrameP->interlacingIsOn = false;
        tempFrameP->skipOddLines = true;

        SWInitializeFrame(tempFrameP);

        *newFrameP = tempFrameP;
    } else {
        err = MemError();
    }

    if (err != noErr) {
        if (tempFrameP != NULL) {
            DisposePtr((Ptr)tempFrameP);
        }
    }

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateWindowFrame(CWindowPtr srcWindowP, SWFrameRec **newFrameP,
                          Rect *frameRect, short maxHeight)
{
    short err = 0;
    SWFrameRec *tempFrameP = 0;
    int32_t numScanLines;

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
            (sizeof(uint32_t) * numScanLines));

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
#endif

/* `saveGDH = GetGWorldDevice(saveGWorld)' is never read, but the statement has to be
   written: mwcc keeps the call and drops the store. SpriteWorldRec's mainSWGDH is at
   +0xc0, immediately in front of the pixelDepth macport.h pins at +0xc4. */

/* SWCreateFrameFromCicnResource (mac _SWCreateFrameFromCicnResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476800
// MAC: _SWCreateFrameFromCicnResource i386 0x000add00 ppc 0x000b91f4
#ifdef MACBUILD
short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
                                    SWFrameRec **newFrameP, short iconResID,
                                    MaskType maskType)
{
    OSErr err;
    GWorldPtr saveGWorld;
    GDHandle saveGDH;
    GDHandle theGDH;
    FramePtr tempFrameP;
    CIconHandle cIconH;
    RgnHandle maskRgn;
    Rect frameRect;

    *newFrameP = NULL;
    tempFrameP = NULL;

    SW_ASSERT(destSpriteWorld != NULL, 278);

    GetGWorld(&saveGWorld, &saveGDH);

    cIconH = GetCIcon(iconResID);

    if (cIconH != NULL) {
        HNoPurge((Handle)cIconH);
        frameRect = (**cIconH).iconPMap.bounds;

        theGDH = destSpriteWorld->mainSWGDH;

        err = SWCreateFrame(theGDH, &tempFrameP, &frameRect, destSpriteWorld->pixelDepth, kCreateGWorld);
    } else {
        err = MemError();
        if (err == noErr)
            err = resNotFound;
    }

    if (err == noErr) {
        LockPixels(tempFrameP->framePixHndl);
        SetGWorld(tempFrameP->framePort, NULL);
        EraseRect(&frameRect);
        PlotCIcon(&frameRect, cIconH);
        UnlockPixels(tempFrameP->framePixHndl);

        if ((maskType & kRegionMask) != 0) {
            err = SWCreateRegionFromCIconMask(&maskRgn, cIconH);

            if (err == noErr) {
                SWSetFrameMaskRgn(tempFrameP, maskRgn);
            }
        }
    }

    if (err == noErr) {
        if ((maskType & kPixelMask) != 0) {
            err = SWCreateGWorldFromCIconMask(destSpriteWorld, &tempFrameP->maskPort, (struct Image_t *)cIconH);

            if (err == noErr) {
                tempFrameP->maskDevice = GetGWorldDevice(tempFrameP->maskPort);
                tempFrameP->maskPixHndl = GetGWorldPixMap(tempFrameP->maskPort);

                SWFixImageGWorld(tempFrameP->framePort, tempFrameP->maskPort, NULL);
            }

            if (err == noErr && destSpriteWorld->pixelDepth <= 8) {
                SWInvertGWorld(tempFrameP->maskPort);
            }
        }
    }

    if (cIconH != NULL) {
        DisposeCIcon(cIconH);
    }

    if (err == noErr) {
        *newFrameP = tempFrameP;
    }

    if (err != noErr) {
        if (tempFrameP != NULL) {
            SWDisposeFrame(&tempFrameP);
        }
    }

    SetGWorld(saveGWorld, NULL);

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
                                    SWFrameRec **newFrameP, short iconResID,
                                    unsigned char maskType)
{
    short err;
    CGrafPtr saveGWorld;
    void *saveGDH;
    SWFrameRec *tempFrameP;
    CIconHandle cIconH;
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
#endif

#ifdef MACBUILD
/* SWCreateFrameFromPictResource (mac _SWCreateFrameFromPictResource, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateFrameFromPictResource i386 0x000ad9dc ppc 0x000b8edc
short SWCreateFrameFromPictResource(SpriteWorldPtr destSpriteWorldP, FramePtr *newFrameP,
                                    short pictResID, short maskResID, MaskType maskType)
{
    OSErr err;
    GWorldPtr saveGWorld, tempMaskGWorldP;
    GDHandle saveGDH;
    GDHandle theGDH;
    PicHandle spritePictH;
    FramePtr tempFrameP;
    RgnHandle maskRgn;
    Rect frameRect;
    Boolean needToInvertMask;

    SW_ASSERT(destSpriteWorldP != NULL, 395);

    tempFrameP = NULL;
    *newFrameP = NULL;
    tempMaskGWorldP = NULL;
    needToInvertMask = true;

    GetGWorld(&saveGWorld, &saveGDH);

    spritePictH = GetPicture(pictResID);

    if (spritePictH != NULL) {
        frameRect = (**spritePictH).picFrame;
        OffsetRect(&frameRect, -frameRect.left, -frameRect.top);

        if (useLegacyRenderingPipeline)
            theGDH = destSpriteWorldP->mainSWGDH;

        err = SWCreateFrame(theGDH, &tempFrameP, &frameRect, destSpriteWorldP->pixelDepth, kCreateGWorld);

        if (err == noErr) {
            LockPixels(tempFrameP->framePixHndl);
            SetGWorld(tempFrameP->framePort, NULL);
            EraseRect(&frameRect);
            DrawPicture(spritePictH, &frameRect);
            UnlockPixels(tempFrameP->framePixHndl);
            ReleaseResource((Handle)spritePictH);
        }

        if (err == noErr && maskType != kNoMask) {
            err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempMaskGWorldP, maskResID);

            if (err == noErr) {
                if (pictResID == maskResID && tempMaskGWorldP != NULL) {
                    err = SWCreatePixelMaskFromGWorld(tempFrameP->framePort, tempMaskGWorldP, NULL);
                    needToInvertMask = false;

                    if (destSpriteWorldP->pixelDepth <= 8 && (maskType & kRegionMask) != 0) {
                        SWInvertGWorld(tempMaskGWorldP);
                        needToInvertMask = true;
                    }
                } else if ((maskType & kPixelMask) != 0) {
                    SWFixImageGWorld(tempFrameP->framePort, tempMaskGWorldP, NULL);
                }
            }
        }

        if (((maskType & kRegionMask) != 0) && (err == noErr)) {
            err = SWCreateRegionFromGWorldAndRect(&maskRgn,
                    tempMaskGWorldP, &tempFrameP->frameRect);

            if (err == noErr) {
                SWSetFrameMaskRgn(tempFrameP, maskRgn);
            }
        }

        if (err == noErr) {
            if ((maskType & kPixelMask) != 0) {
                if (destSpriteWorldP->pixelDepth <= 8 && needToInvertMask) {
                    SWInvertGWorld(tempMaskGWorldP);
                }
            } else {
                if (tempMaskGWorldP != NULL) {
                    DisposeGWorld(tempMaskGWorldP);
                    tempMaskGWorldP = NULL;
                }
            }

            if (tempMaskGWorldP != NULL) {
                tempFrameP->maskPort = tempMaskGWorldP;
                tempFrameP->maskDevice = GetGWorldDevice(tempFrameP->maskPort);
                tempFrameP->maskPixHndl = GetGWorldPixMap(tempFrameP->maskPort);
            } else {
                tempFrameP->maskPort = NULL;
                tempFrameP->maskDevice = NULL;
                tempFrameP->maskPixHndl = NULL;
            }
        }

        if (err == noErr) {
            *newFrameP = tempFrameP;
        } else {
            if (tempFrameP != NULL) {
                SWDisposeFrame(&tempFrameP);
            }
        }
    } else {
        err = MemError();
        if (err == noErr)
            err = resNotFound;
    }

    SetGWorld(saveGWorld, NULL);

    SWSetStickyIfError(err);
    return err;
}

/* SWCreateFrameFromGWorldAndRect (mac _SWCreateFrameFromGWorldAndRect, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateFrameFromGWorldAndRect i386 0x000aebdd ppc 0x000ba140
short SWCreateFrameFromGWorldAndRect(FramePtr *newFrameP, GWorldPtr pictGWorld,
                                     GWorldPtr maskGWorld, Rect *frameRect, MaskType maskType)
{
    OSErr err;
    GWorldPtr tempMaskGWorld;

    err = SWCreateFrameFromGWorldAndRectStart(&tempMaskGWorld,
            frameRect->right - frameRect->left, frameRect->bottom - frameRect->top, maskType);

    if (err == noErr) {
        err = SWCreateFrameFromGWorldAndRectPartial(newFrameP, pictGWorld, maskGWorld,
                tempMaskGWorld, frameRect, maskType);

        SWCreateFrameFromGWorldAndRectFinish(tempMaskGWorld);
    }

    SWSetStickyIfError(err);
    return err;
}
#endif

/* Start's `if ((maskType & kRegionMask) != 0)' arm is GONE, not merely never taken, so
   its three trailing parameters are dead; they survive because 0x00474d07 still pushes
   four arguments, which is the only evidence for the signature. */

/* SWCreateFrameFromGWorldAndRectStart (mac _SWCreateFrameFromGWorldAndRectStart). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004769d0
// MAC: _SWCreateFrameFromGWorldAndRectStart i386 0x000ad091 ppc 0x000b8454
#ifdef MACBUILD
short SWCreateFrameFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld, short maxWidth,
                                          short maxHeight, MaskType maskType)
{
    OSErr err = noErr;

    *tempMaskGWorld = NULL;
    if ((maskType & kRegionMask) != 0) {
        err = SWCreateRegionFromGWorldAndRectStart(tempMaskGWorld, maxWidth, maxHeight);
    }
    return err;
}
#else
short SWCreateFrameFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld, short maxWidth,
                                          short maxHeight, unsigned char maskType)
{
    short err = 0;

    *tempMaskGWorld = 0;
    return err;
}
#endif

/* SWCreateFrameFromGWorldAndRectFinish (mac _SWCreateFrameFromGWorldAndRectFinish). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004769e0
// MAC: _SWCreateFrameFromGWorldAndRectFinish i386 0x000ad0ce ppc 0x000b8490
void SWCreateFrameFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld)
{
    SWCreateRegionFromGWorldAndRectFinish(tempMaskGWorld);
}

/* SWCreateFrameFromGWorldAndRectPartial (mac _SWCreateFrameFromGWorldAndRectPartial). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004769f0
// MAC: _SWCreateFrameFromGWorldAndRectPartial i386 0x000ad7f1 ppc 0x000b8cc4
#ifdef MACBUILD
short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
        CGrafPtr pictGWorld, CGrafPtr maskGWorld, CGrafPtr tempMaskGWorld,
        Rect *frameRect, MaskType maskType)
{
    OSErr err = noErr;
    short depth;
    int32_t numScanLines;
    FramePtr tempFrameP;
    RgnHandle maskRgn;

    SW_ASSERT(pictGWorld != NULL, 607);
    SW_ASSERT(frameRect->top >= 0 && frameRect->left >= 0 &&
              frameRect->right > frameRect->left && frameRect->bottom > frameRect->top, 609);

    tempFrameP = NULL;
    *newFrameP = NULL;

    numScanLines = frameRect->bottom - frameRect->top;
    depth = SWGetPixBitDepth(GetGWorldPixMap(pictGWorld));

    tempFrameP = (FramePtr)NewPtrClear(sizeof(SWFrameRec) +
            (sizeof(uint32_t) * (numScanLines + 1)));

    if (tempFrameP != NULL) {
        tempFrameP->framePort = pictGWorld;
        tempFrameP->frameDevice = GetGWorldDevice(pictGWorld);
        tempFrameP->framePixHndl = GetGWorldPixMap(pictGWorld);
        tempFrameP->frameDepth = depth;

        if ((maskType & kPixelMask) != 0) {
            SW_ASSERT(maskGWorld != NULL, 630);
            tempFrameP->maskPort = maskGWorld;
            tempFrameP->maskDevice = GetGWorldDevice(maskGWorld);
            tempFrameP->maskPixHndl = GetGWorldPixMap(maskGWorld);
        }

        tempFrameP->frameRect = *frameRect;
        tempFrameP->sharesGWorld = true;
        tempFrameP->isWindowFrame = false;
        tempFrameP->numScanLines = numScanLines;

        tempFrameP->useCount = 0;

        SWInitializeFrame(tempFrameP);
    } else {
        err = MemError();
    }

    if (err == noErr) {
        if (((maskType & kRegionMask) != 0) && (err == noErr)) {
            SW_ASSERT(maskGWorld != NULL, 659);

            err = SWCreateRegionFromGWorldAndRectPartial(&maskRgn, maskGWorld,
                    tempMaskGWorld, frameRect);

            if (err == noErr) {
                SWSetFrameMaskRgn(tempFrameP, maskRgn);
            }
        }

        if (err == noErr) {
            *newFrameP = tempFrameP;
        } else {
            if (tempFrameP != NULL) {
                SWDisposeFrame(&tempFrameP);
            }
        }
    }

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
        CGrafPtr pictGWorld, CGrafPtr maskGWorld, CGrafPtr tempMaskGWorld,
        Rect *frameRect, unsigned char maskType)
{
    short err = 0;
    short depth;
    int32_t numScanLines;
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
            (sizeof(uint32_t) * (numScanLines + 1)));

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
#endif

/* The donor's `SW_ASSERT(!oldFrameP->isWindowFrame)' became a guard that RETURNS FALSE
   through its own exit. useCount is unsigned -- `cmp word ptr [esi+0x58], 1' followed by
   `ja', not `jg'. */

/* SWDisposeFrame (mac _SWDisposeFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476bd0
// MAC: _SWDisposeFrame i386 0x000ad0d7 ppc 0x000b8494
#ifdef MACBUILD
Boolean SWDisposeFrame(SWFrameRec **oldFramePP)
{
    Boolean frameDisposed;
    FramePtr oldFrameP = *oldFramePP;

    frameDisposed = false;
    if (oldFrameP != NULL) {
        if (oldFrameP->isWindowFrame)
            return false;

        if (oldFrameP->useCount > 1) {
            oldFrameP->useCount--;
        } else {
            frameDisposed = true;
            if (oldFrameP->framePort != NULL) {
                if (!oldFrameP->sharesGWorld)
                    DisposeGWorld(oldFrameP->framePort);
                oldFrameP->framePort = NULL;
                oldFrameP->frameDevice = NULL;
                oldFrameP->framePixHndl = NULL;
            }

            if (oldFrameP->maskRgn != NULL) {
                DisposeRgn(oldFrameP->maskRgn);
                oldFrameP->maskRgn = NULL;
            }

            if (oldFrameP->maskPort != NULL) {
                if (!oldFrameP->sharesGWorld)
                    DisposeGWorld(oldFrameP->maskPort);
                oldFrameP->maskPort = NULL;
                oldFrameP->maskDevice = NULL;
                oldFrameP->maskPixHndl = NULL;
            }

            DisposePtr((Ptr)oldFrameP);
            *oldFramePP = NULL;
        }
    }

    return frameDisposed;
}
#else
Boolean SWDisposeFrame(SWFrameRec **oldFramePP)
{
    Boolean frameDisposed;
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
#endif

/* SWDisposeWindowFrame (mac _SWDisposeWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476c70
// MAC: _SWDisposeWindowFrame i386 0x000ad197 ppc 0x000b8574
#ifdef MACBUILD
void SWDisposeWindowFrame(SWFrameRec **oldFramePP)
{
    FramePtr oldFrameP = *oldFramePP;

    if (oldFrameP != NULL) {
        SW_ASSERT(oldFrameP->isWindowFrame, 770);

        DisposePtr((Ptr)oldFrameP);
        *oldFramePP = NULL;
    }
}
#else
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
#endif

/* SWSetPortToFrame (mac _SWSetPortToFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476cb0
// MAC: _SWSetPortToFrame i386 0x000ad1dc ppc 0x000b85dc
#ifdef MACBUILD
void SWSetPortToFrame(SWFrameRec *dstFrameP)
{
    SW_ASSERT(dstFrameP != NULL, 786);
    SW_ASSERT(dstFrameP->framePort != NULL, 787);
    SW_ASSERT(dstFrameP->isFrameLocked, 788);

    if (useLegacyRenderingPipeline)
        SetGWorld(dstFrameP->framePort, dstFrameP->frameDevice);
    else
        SetGWorld(dstFrameP->framePort, NULL);
}
#else
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
#endif

#ifdef MACBUILD
/* SWAlignFrameToWindow (mac _SWAlignFrameToWindow, mac-only). STATE: see the mac gate. */
// MAC: _SWAlignFrameToWindow i386 0x000ae7ec ppc 0x000b9ccc
Boolean SWAlignFrameToWindow(FramePtr dstFrameP, FramePtr windowFrameP)
{
    Rect globalRect;
    GWorldPtr newGWorld;
    GWorldFlags flags;
    Boolean wasLocked;
    GWorldPtr oldGWorld;
    GDHandle oldDevice;
    Boolean wasCurrent;
    OSErr err;
    Boolean redraw = false;

    SW_ASSERT(windowFrameP != NULL, 815);
    SW_ASSERT(windowFrameP->isWindowFrame, 816);
    SW_ASSERT(dstFrameP != NULL, 817);
    SW_ASSERT(!dstFrameP->isWindowFrame, 818);

    if (dstFrameP->framePort != NULL &&
            windowFrameP->framePort != NULL &&
            dstFrameP->frameDepth == windowFrameP->frameDepth) {
        GetGWorld(&oldGWorld, &oldDevice);
        newGWorld = dstFrameP->framePort;

        wasCurrent = (newGWorld == oldGWorld);
        SetPort((GrafPtr)windowFrameP->framePort);

        globalRect = *GetCPortPortRect(dstFrameP->framePort);
        LocalToGlobal(&topLeft(globalRect));
        LocalToGlobal(&botRight(globalRect));

        wasLocked = dstFrameP->isFrameLocked;
        if (wasLocked)
            SWUnlockFrame(dstFrameP);

        flags = UpdateGWorld(&newGWorld, 0, &globalRect,
                             NULL, NULL, 0);

        if (!(flags & gwFlagErr)) {
            dstFrameP->framePort = newGWorld;
            dstFrameP->framePixHndl = GetGWorldPixMap(newGWorld);
            dstFrameP->frameDevice = GetGWorldDevice(newGWorld);

            SetPort((GrafPtr)newGWorld);
            SetOrigin(0, 0);

            if (flags & (reallocPix | newDepth | newRowBytes)) {
                SWInitializeFrame(dstFrameP);
            }
            if (flags & (reallocPix | newDepth | mapPix)) {
                redraw = true;
            }
        } else
            err = flags;

        if (wasLocked)
            SWLockFrame(dstFrameP);

        if (wasCurrent)
            SWSetPortToFrame(dstFrameP);
        else if (useLegacyRenderingPipeline)
            SetGWorld(oldGWorld, oldDevice);
        else
            SetGWorld(oldGWorld, NULL);
    }

    return redraw;
}
#endif

/* SWInitializeFrame (mac _SWInitializeFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476d10
// MAC: _SWInitializeFrame i386 0x000ad25d ppc 0x000b8678
#ifdef MACBUILD
void SWInitializeFrame(SWFrameRec *tempFrameP)
{
    int32_t curScanLine;
    int32_t rowOffset;
    short depth;
    PixMapHandle pixmap;

    SW_ASSERT(tempFrameP != NULL, 891);
    if (tempFrameP->framePixHndl == NULL) {
        if (tempFrameP->framePort != NULL)
            tempFrameP->framePixHndl = GetPortPixMap(tempFrameP->framePort);
    }
    SW_ASSERT(tempFrameP->framePixHndl != NULL, 899);
    SW_ASSERT(tempFrameP->frameDepth != 0, 900);

    pixmap = tempFrameP->framePixHndl;
    depth = tempFrameP->frameDepth;

    tempFrameP->frameRowBytes = SWGetPixRowBytes(pixmap);

    tempFrameP->rightAlignFactor = (32 / depth) - 1;
    tempFrameP->leftAlignFactor = ~(tempFrameP->rightAlignFactor);

    tempFrameP->pixelShift = depth >> 4;

    tempFrameP->scanLinePtrArray = (uint32_t *)(tempFrameP + 1);

    rowOffset = tempFrameP->frameRect.top * tempFrameP->frameRowBytes;
    for (curScanLine = 0; curScanLine < tempFrameP->numScanLines; curScanLine++) {
        tempFrameP->scanLinePtrArray[curScanLine] = rowOffset;
        rowOffset += tempFrameP->frameRowBytes;
    }
}
#else
void SWInitializeFrame(SWFrameRec *tempFrameP)
{
    int32_t curScanLine;
    int32_t rowOffset;
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

    tempFrameP->scanLinePtrArray = (uint32_t *)(tempFrameP + 1);

    rowOffset = tempFrameP->frameRect.top * tempFrameP->frameRowBytes;
    for (curScanLine = 0; curScanLine < tempFrameP->numScanLines; curScanLine++) {
        tempFrameP->scanLinePtrArray[curScanLine] = rowOffset;
        rowOffset += tempFrameP->frameRowBytes;
    }
}
#endif

#ifdef MACBUILD
/* SWSetFrameMaskRgn (mac _SWSetFrameMaskRgn, mac-only). STATE: see the mac gate. */
// MAC: _SWSetFrameMaskRgn i386 0x000ad77f ppc 0x000b8c34
void SWSetFrameMaskRgn(FramePtr srcFrameP, RgnHandle maskRgn)
{
    SW_ASSERT(srcFrameP != NULL, 947);
    SW_ASSERT(maskRgn != NULL, 948);

    if (maskRgn != NULL) {
        srcFrameP->maskRgn = maskRgn;

        srcFrameP->offsetPoint.h = GetRgnBBox(maskRgn)->left;
        srcFrameP->offsetPoint.v = GetRgnBBox(maskRgn)->top;
    }
}

/* SWSetFrameHotSpot (mac _SWSetFrameHotSpot, mac-only). STATE: see the mac gate. */
// MAC: _SWSetFrameHotSpot i386 0x000adf1a ppc 0x000b93f8
void SWSetFrameHotSpot(FramePtr srcFrameP, short hotSpotH, short hotSpotV)
{
    SW_ASSERT(srcFrameP != NULL, 969);

    srcFrameP->hotSpotH = hotSpotH;
    srcFrameP->hotSpotV = hotSpotV;
}

/* SWSetFrameCollisionInset (mac _SWSetFrameCollisionInset, mac-only). STATE: see the mac gate. */
// MAC: _SWSetFrameCollisionInset i386 0x000adf5d ppc 0x000b9448
void SWSetFrameCollisionInset(FramePtr srcFrameP, short leftInset, short topInset,
                              short rightInset, short bottomInset)
{
    SW_ASSERT(srcFrameP != NULL, 987);

    if (leftInset == 0 && topInset == 0 && rightInset == 0 && bottomInset == 0)
        srcFrameP->usesCollisionInset = false;
    else
        srcFrameP->usesCollisionInset = true;

    srcFrameP->collisionInset.left = leftInset;
    srcFrameP->collisionInset.top = topInset;
    srcFrameP->collisionInset.right = rightInset;
    srcFrameP->collisionInset.bottom = bottomInset;
}

/* SWSetFrameInterlacingMode (mac _SWSetFrameInterlacingMode, mac-only). STATE: see the mac gate. */
// MAC: _SWSetFrameInterlacingMode i386 0x000adff0 ppc 0x000b94e0
void SWSetFrameInterlacingMode(FramePtr dstFrameP, Boolean skipEveryOtherLine,
                               Boolean skipOddLines)
{
    SW_ASSERT(dstFrameP != NULL, 1007);

    dstFrameP->interlacingIsOn = skipEveryOtherLine;
    dstFrameP->skipOddLines = skipOddLines;
}

/* SWCopyFrame (mac _SWCopyFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWCopyFrame i386 0x000ae035 ppc 0x000b9530
OSErr SWCopyFrame(SpriteWorldPtr destSpriteWorldP, FramePtr oldFrameP, FramePtr *newFrameP,
                  Boolean copyMasks)
{
    OSErr err;
    GWorldPtr saveGWorld;
    GDHandle saveGDH;
    GDHandle theGDH;
    GWorldFlags pixelState;
    short depth;
    FramePtr tempFrameP;
    Rect frameRect;

    SW_ASSERT(newFrameP != NULL, 1033);
    SW_ASSERT(oldFrameP != NULL, 1034);
    SW_ASSERT(oldFrameP->framePort != NULL, 1035);
    SW_ASSERT(destSpriteWorldP != NULL, 1036);

    *newFrameP = NULL;
    tempFrameP = NULL;
    depth = destSpriteWorldP->pixelDepth;
    if (useLegacyRenderingPipeline)
        theGDH = destSpriteWorldP->mainSWGDH;

    GetGWorld(&saveGWorld, &saveGDH);

    frameRect = oldFrameP->frameRect;
    OffsetRect(&frameRect, -frameRect.left, -frameRect.top);

    err = SWCreateFrame(theGDH, &tempFrameP, &frameRect, depth, kCreateGWorld);

    if (err == noErr) {
        pixelState = GetPixelsState(oldFrameP->framePixHndl);
        LockPixels(GetGWorldPixMap(oldFrameP->framePort));
        LockPixels(GetGWorldPixMap(tempFrameP->framePort));
        SetGWorld(oldFrameP->framePort, NULL);
        ForeColor(blackColor);
        BackColor(whiteColor);
        SetGWorld(tempFrameP->framePort, NULL);

        CopyBits((BitMap *)*GetGWorldPixMap(oldFrameP->framePort),
                 (BitMap *)*GetGWorldPixMap(tempFrameP->framePort),
                 &oldFrameP->frameRect,
                 &frameRect,
                 srcCopy,
                 NULL);

        SetPixelsState(GetPortPixMap(oldFrameP->framePort), pixelState);

        UnlockPixels(GetGWorldPixMap(tempFrameP->framePort));
    }

    if ((oldFrameP->maskRgn != NULL) && (err == noErr)) {
        tempFrameP->maskRgn = NewRgn();

        if (tempFrameP->maskRgn == NULL)
            err = MemError();
        else if (copyMasks)
            CopyRgn(oldFrameP->maskRgn, tempFrameP->maskRgn);
    }

    if ((oldFrameP->maskPort != NULL) && (err == noErr)) {
        if (useLegacyRenderingPipeline) {
            if (SWGetPixBitDepth((**theGDH).gdPMap) == depth)
                err = (NewGWorld)(&tempFrameP->maskPort, depth, &frameRect, NULL, theGDH, noNewDevice);
            else
                err = (NewGWorld)(&tempFrameP->maskPort, depth, &frameRect, NULL, NULL, 0);
        } else {
            err = (NewGWorld)(&tempFrameP->maskPort, 16, &frameRect, NULL, NULL, kNativeEndianPixMap);
        }

        if ((err == noErr) && (copyMasks)) {
            tempFrameP->maskDevice = GetGWorldDevice(tempFrameP->maskPort);
            tempFrameP->maskPixHndl = GetGWorldPixMap(tempFrameP->maskPort);

            pixelState = GetPixelsState(oldFrameP->maskPixHndl);
            LockPixels(oldFrameP->maskPixHndl);
            LockPixels(tempFrameP->maskPixHndl);
            SetGWorld(oldFrameP->maskPort, NULL);
            ForeColor(blackColor);
            BackColor(whiteColor);
            SetGWorld(tempFrameP->maskPort, NULL);

            CopyBits((BitMap *)*oldFrameP->maskPixHndl,
                     (BitMap *)*tempFrameP->maskPixHndl,
                     &oldFrameP->frameRect,
                     &frameRect,
                     srcCopy,
                     NULL);

            SetPixelsState(oldFrameP->maskPixHndl, pixelState);

            UnlockPixels(tempFrameP->maskPixHndl);
        }
    }

    if (err == noErr) {
        *newFrameP = tempFrameP;
    } else {
        if (tempFrameP != NULL) {
            SWDisposeFrame(&tempFrameP);
        }
    }

    SetGWorld(saveGWorld, NULL);

    SWSetStickyIfError(err);
    return err;
}

/* SWUpdateFrameMasks (mac _SWUpdateFrameMasks, mac-only). STATE: see the mac gate. */
// MAC: _SWUpdateFrameMasks i386 0x000ae65a ppc 0x000b9b38
OSErr SWUpdateFrameMasks(FramePtr srcFrameP)
{
    RGBColor oldTransparentColor, myBlackColor;
    Boolean frameWasUnlocked = false;
    short depth;
    OSErr err = noErr;

    SW_ASSERT(srcFrameP != NULL, 1163);

    if (!srcFrameP->isFrameLocked) {
        SWLockFrame(srcFrameP);
        frameWasUnlocked = true;
    }

    depth = srcFrameP->frameDepth;

    if (srcFrameP->maskPort != NULL) {
        SW_ASSERT(srcFrameP->framePort != NULL, 1176);

        if (depth > 8) {
            oldTransparentColor = gSWTransparentColor;
            myBlackColor.red = myBlackColor.green = myBlackColor.blue = 0;
            SWSetTransparentColor(&myBlackColor);
        }

        SWCreatePixelMaskFromGWorld(srcFrameP->framePort, srcFrameP->maskPort, &srcFrameP->frameRect);

        if (depth > 8)
            SWSetTransparentColor(&oldTransparentColor);

        if (srcFrameP->maskRgn != NULL) {
            DisposeRgn(srcFrameP->maskRgn);

            if (depth <= 8)
                SWInvertRect(srcFrameP->maskPort, &srcFrameP->frameRect);

            err = SWCreateRegionFromGWorldAndRect((RgnHandle *)&srcFrameP->maskRgn,
                    srcFrameP->maskPort, &srcFrameP->frameRect);

            if (err == noErr)
                SWSetFrameMaskRgn(srcFrameP, srcFrameP->maskRgn);

            if (depth <= 8)
                SWInvertRect(srcFrameP->maskPort, &srcFrameP->frameRect);
        }
    } else if (srcFrameP->maskRgn != NULL) {
        err = SWCreateRegionMaskFromGWorld(srcFrameP->framePort,
                &srcFrameP->frameRect, srcFrameP->maskRgn);

        if (err == noErr)
            SWSetFrameMaskRgn(srcFrameP, srcFrameP->maskRgn);
    }

    if (frameWasUnlocked)
        SWUnlockFrame(srcFrameP);

    SWSetStickyIfError(err);
    return err;
}
#endif

/* SWLockFrame (mac _SWLockFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476e10
// MAC: _SWLockFrame i386 0x000ae467 ppc 0x000b98ec
#ifdef MACBUILD
void SWLockFrame(SWFrameRec *srcFrameP)
{
    PixMapHandle pixMapH;
    int32_t offsetX, offsetY;

    if (srcFrameP == NULL)
        return;
    SW_ASSERT(srcFrameP != NULL, 1251);

    if (srcFrameP->isWindowFrame)
        return;
    SW_ASSERT(!srcFrameP->isWindowFrame, 1256);

    if (!srcFrameP->isFrameLocked) {
        srcFrameP->isFrameLocked = true;

        if (srcFrameP->framePort != NULL) {
            SW_ASSERT(srcFrameP->framePixHndl != NULL, 1264);

            pixMapH = srcFrameP->framePixHndl;
            LockPixels(pixMapH);
            HLockHi((Handle)pixMapH);
            srcFrameP->framePix = *pixMapH;

            srcFrameP->frameBaseAddr = SWGetPixBaseAddr(pixMapH);
            srcFrameP->frameRowBytes = SWGetPixRowBytes(pixMapH);
            srcFrameP->frameDepth = SWGetPixBitDepth(pixMapH);

            offsetX = -(**pixMapH).bounds.left;
            offsetY = -(**pixMapH).bounds.top;

            if (offsetX != 0 || offsetY != 0) {
                srcFrameP->frameBaseAddr += offsetY * srcFrameP->frameRowBytes;
                srcFrameP->frameBaseAddr += (offsetX * srcFrameP->frameDepth) >> 3;
            }

            if (srcFrameP->frameDepth < 8) {
                short pixelStart;
                short roundOff;

                pixelStart = -(**pixMapH).bounds.left;
                roundOff = 8 / srcFrameP->frameDepth;

                srcFrameP->worldRectOffset = pixelStart % roundOff;
            } else
                srcFrameP->worldRectOffset = 0;
        }

        if (srcFrameP->maskPort != NULL) {
            SW_ASSERT(srcFrameP->maskPixHndl != NULL, 1309);

            pixMapH = srcFrameP->maskPixHndl;
            LockPixels(pixMapH);
            HLockHi((Handle)pixMapH);
            srcFrameP->maskPix = *pixMapH;

            srcFrameP->maskBaseAddr = SWGetPixBaseAddr(pixMapH);
        }
    }
}
#else
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

            srcFrameP->frameBaseAddr = (char *)GetPixBaseAddr(pixMapH);
            srcFrameP->frameRowBytes = GetPixRowBytes(pixMapH);
            srcFrameP->frameDepth = GetPixDepth(pixMapH);

            srcFrameP->worldRectOffset = 0;
        }

        if (srcFrameP->maskPort != 0) {
            if (srcFrameP->maskPixHndl == 0)
                SWAssertFail("SpriteFrame.c", 1348);

            pixMapH = srcFrameP->maskPixHndl;
            srcFrameP->maskPix = pixMapH;

            srcFrameP->maskBaseAddr = (char *)GetPixBaseAddr(pixMapH);
        }
    }
}
#endif

/* SWUnlockFrame (mac _SWUnlockFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476f10
// MAC: _SWUnlockFrame i386 0x000ae5b1 ppc 0x000b9a6c
#ifdef MACBUILD
void SWUnlockFrame(SWFrameRec *srcFrameP)
{
    SW_ASSERT(srcFrameP != NULL, 1345);
    SW_ASSERT(!srcFrameP->isWindowFrame, 1346);

    if (srcFrameP->isFrameLocked) {
        srcFrameP->isFrameLocked = false;

        if (srcFrameP->framePort != NULL && srcFrameP->framePixHndl != NULL) {
            HUnlock((Handle)srcFrameP->framePixHndl);
            UnlockPixels(srcFrameP->framePixHndl);
        }

        srcFrameP->framePix = NULL;
        srcFrameP->frameBaseAddr = NULL;

        if (srcFrameP->maskPort != NULL && srcFrameP->maskPixHndl != NULL) {
            HUnlock((Handle)srcFrameP->maskPixHndl);
            UnlockPixels(srcFrameP->maskPixHndl);
        }

        srcFrameP->maskPix = NULL;
        srcFrameP->maskBaseAddr = NULL;
    }
}
#else
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
#endif

/* The dead Rect is real and must be written: the exe still brackets a
   GetWindowPortRect(framePort) whose result goes nowhere, because mwcc keeps the call and
   drops the copy. worldRectOffset collapses to the constant 0 the exe stores at +0x5a. */

/* SWLockWindowFrame (mac _SWLockWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00476f80
// MAC: _SWLockWindowFrame i386 0x000ae9d8 ppc 0x000b9ec4
#ifdef MACBUILD
void SWLockWindowFrame(SWFrameRec *windowFrameP)
{
    GrafPtr savePort;
    Rect globalRect;
    PixMapHandle pixMapH;
    GDHandle maxDevice;
    int32_t offsetX, offsetY;

    SW_ASSERT(windowFrameP != NULL, 1394);
    SW_ASSERT(windowFrameP->framePort != NULL, 1395);
    SW_ASSERT(windowFrameP->framePixHndl != NULL, 1396);
    SW_ASSERT(windowFrameP->isWindowFrame, 1397);

    if (!windowFrameP->isFrameLocked) {
        windowFrameP->isFrameLocked = true;
        pixMapH = windowFrameP->framePixHndl;
        HLockHi((Handle)pixMapH);
        windowFrameP->framePix = *pixMapH;

        GetPort(&savePort);
        SetPort((GrafPtr)windowFrameP->framePort);
        globalRect = *GetCPortPortRect(windowFrameP->framePort);
        LocalToGlobal(&topLeft(globalRect));
        LocalToGlobal(&botRight(globalRect));
        SetPort(savePort);

        maxDevice = GetMainDevice();
        pixMapH = (**maxDevice).gdPMap;

        windowFrameP->frameBaseAddr = SWGetPixBaseAddr(pixMapH);
        windowFrameP->frameRowBytes = SWGetPixRowBytes(pixMapH);
        if (useLegacyRenderingPipeline)
            windowFrameP->frameDepth = SWGetPixBitDepth(pixMapH);
        else
            windowFrameP->frameDepth = 16;

        offsetX = globalRect.left - (**maxDevice).gdRect.left;
        offsetY = globalRect.top - (**maxDevice).gdRect.top;

        offsetX -= (**pixMapH).bounds.left;
        offsetY -= (**pixMapH).bounds.top;

        if (offsetX != 0 || offsetY != 0) {
            windowFrameP->frameBaseAddr += offsetY * windowFrameP->frameRowBytes;
            windowFrameP->frameBaseAddr += (offsetX * windowFrameP->frameDepth) >> 3;
        }

        if (windowFrameP->frameDepth < 8) {
            short pixelStart;
            short roundOff;

            pixelStart = offsetX;
            roundOff = 8 / windowFrameP->frameDepth;

            windowFrameP->worldRectOffset = pixelStart % roundOff;
        } else
            windowFrameP->worldRectOffset = 0;

        SWInitializeFrame(windowFrameP);
    }
}
#else
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

        windowFrameP->frameBaseAddr = (char *)GetPixBaseAddr(pixMapH);
        windowFrameP->frameRowBytes = GetPixRowBytes(pixMapH);
        windowFrameP->frameDepth = GetPixDepth(pixMapH);
        windowFrameP->worldRectOffset = 0;

        SWInitializeFrame(windowFrameP);
    }
}
#endif

/* SWUnlockWindowFrame (mac _SWUnlockWindowFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477070
// MAC: _SWUnlockWindowFrame i386 0x000aeb78 ppc 0x000ba0c0
#ifdef MACBUILD
void SWUnlockWindowFrame(SWFrameRec *windowFrameP)
{
    SW_ASSERT(windowFrameP != NULL, 1475);
    SW_ASSERT(windowFrameP->isWindowFrame, 1476);

    if (windowFrameP->isFrameLocked) {
        windowFrameP->isFrameLocked = false;
        HUnlock((Handle)windowFrameP->framePixHndl);
        windowFrameP->framePix = NULL;
        windowFrameP->frameBaseAddr = NULL;
    }
}
#else
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
#endif
