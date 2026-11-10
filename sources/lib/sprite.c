// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* sprite.c -- SpriteWorld's Sprite.c, tu-map object O14: sprite creation, frames, movement and collision. Notes: Research/notes/sprite.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"
#ifdef MACBUILD
#include "render.h"
#endif

/* SWCreateSpriteFromCicnResource (mac _SWCreateSpriteFromCicnResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474970
// MAC: _SWCreateSpriteFromCicnResource i386 0x000aa808 ppc 0x000b55b0
short SWCreateSpriteFromCicnResource(SpriteWorldPtr destSpriteWorldP, SpritePtr *newSpriteP,
                                     void *spriteStorageP, short cIconID, short maxFrames,
                                     MaskType maskType)
{
    short err;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short frame;

    if (destSpriteWorldP == 0)
        SW_ASSERT_FAIL("Sprite.c", 366, 350);

    *newSpriteP = 0;

    err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);

    if ((int)err == noErr) {
        for (frame = 0; frame < maxFrames; frame++) {
            err = SWCreateFrameFromCicnResource(destSpriteWorldP, &newFrameP,
                    cIconID + frame, maskType);

            if ((int)err == noErr) {
                err = SWAddFrame(tempSpriteP, newFrameP);
            }

            if ((int)err != noErr) {
                SWDisposeFrame(&newFrameP);

                if (tempSpriteP->numFrames == 0) {
                    DeallocAPointer(tempSpriteP->frameArray);
                    tempSpriteP->frameArray = 0;
                }

                SWDisposeSprite(&tempSpriteP);
                break;
            }
        }

        if ((int)err == noErr) {
            SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(tempSpriteP, 0);
            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}

#ifdef MACBUILD
static void CalcMaxWidthHeight(RectArrayPtr newRectArrayP, short *maxWidthP, short *maxHeightP);

/* SWCreateSpriteFromPictResource (mac _SWCreateSpriteFromPictResource, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateSpriteFromPictResource i386 0x000aa6b9 ppc 0x000b5460
OSErr SWCreateSpriteFromPictResource(
    SpriteWorldPtr destSpriteWorldP,
    SpritePtr* newSpriteP,
    void* spriteStorageP,
    short pictResID,
    short maskResID,
    short maxFrames,
    MaskType maskType)
{
    OSErr err;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short frame;

    SW_ASSERT(destSpriteWorldP != NULL, 414);

    *newSpriteP = NULL;

    err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);

    if (err == noErr) {
        for (frame = 0; frame < maxFrames; frame++) {
            err = SWCreateFrameFromPictResource(destSpriteWorldP, &newFrameP,
                    pictResID + frame, maskResID + frame, maskType);

            if (err == noErr) {
                err = SWAddFrame(tempSpriteP, newFrameP);
            }

            if (err != noErr) {
                SWDisposeFrame(&newFrameP);
                SWDisposeSprite(&tempSpriteP);
                break;
            }
        }

        if (err == noErr) {
            SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(tempSpriteP, 0);
            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWCreateSpriteFromSinglePict (mac _SWCreateSpriteFromSinglePict, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateSpriteFromSinglePict i386 0x000aa07b ppc 0x000b4ec8
OSErr SWCreateSpriteFromSinglePict(
    SpriteWorldPtr destSpriteWorldP,
    SpritePtr* newSpriteP,
    void* spriteStorageP,
    short pictResID,
    short maskResID,
    short frameDimension,
    short borderWidth,
    MaskType maskType)
{
    OSErr err = noErr;
    Handle rectArrayHndl;
    RectArrayPtr newRectArrayP;
    PicHandle tempPic;
    GWorldPtr tempPictGWorldP;
    GWorldPtr tempMaskGWorldP;
    GWorldPtr tempTempMaskGWorldP;
    Rect frameRect;
    Boolean verticalStrip;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short maxFrames;
    short frame;
    short maxWidth, maxHeight;
    Boolean needToInvertMask;

    SW_ASSERT(destSpriteWorldP != NULL, 483);

    tempPictGWorldP = NULL;
    tempMaskGWorldP = NULL;
    *newSpriteP = NULL;
    rectArrayHndl = NULL;
    needToInvertMask = true;

    if (frameDimension == 0) {
        rectArrayHndl = Get1Resource('nrct', pictResID);

        if (rectArrayHndl == NULL) {
            err = MemError();
            if (err == noErr)
                err = resNotFound;
        } else {
            HLock(rectArrayHndl);
            newRectArrayP = (RectArrayPtr)*rectArrayHndl;
            maxFrames = newRectArrayP->numFrames;
            CalcMaxWidthHeight(newRectArrayP, &maxWidth, &maxHeight);
        }
    } else {
        tempPic = GetPicture(pictResID);
        if (tempPic == nil) {
            err = MemError();
            if (err == noErr)
                err = resNotFound;
        } else {
            frameRect = (**(tempPic)).picFrame;
            OffsetRect(&frameRect, -frameRect.left, -frameRect.top);
            ReleaseResource((Handle)tempPic);
            if (frameRect.right-frameRect.left < frameRect.bottom-frameRect.top) {
                verticalStrip = true;
                maxFrames = frameRect.bottom/frameDimension;
                frameRect.bottom = -borderWidth;
                maxWidth = frameRect.right-frameRect.left;
                maxHeight = frameDimension;
            } else {
                verticalStrip = false;
                maxFrames = frameRect.right/frameDimension;
                frameRect.right = -borderWidth;
                maxWidth = frameDimension;
                maxHeight = frameRect.bottom-frameRect.top;
            }
        }
    }

    if (err == noErr) {
        err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);

        if (err == noErr) {
            err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempPictGWorldP, pictResID);

            if (err == noErr && maskType != kNoMask)
                err = SWCreateGWorldFromPictResource(
                    destSpriteWorldP, &tempMaskGWorldP, maskResID);

            if (err == noErr) {
                if (pictResID == maskResID && tempMaskGWorldP != NULL) {
                    err = SWCreatePixelMaskFromGWorld(tempPictGWorldP, tempMaskGWorldP,  NULL);
                    needToInvertMask = false;

                    if (destSpriteWorldP->pixelDepth <= 8 && (maskType & kRegionMask) != 0) {
                        SWInvertGWorld(tempMaskGWorldP);
                        needToInvertMask = true;
                    }
                } else if ((maskType & kPixelMask) != 0) {
                    SWFixImageGWorld(tempPictGWorldP, tempMaskGWorldP,  NULL);
                }
            }

            if (err == noErr) {
                err = SWCreateFrameFromGWorldAndRectStart(&tempTempMaskGWorldP, maxWidth,
                    maxHeight, maskType);
            }

            if (err == noErr) {
                for (frame = 0; frame < maxFrames; frame++) {
                    if (frameDimension == 0) {
                        frameRect = newRectArrayP->frameRects[frame];
                    } else {
                        if (verticalStrip) {
                            frameRect.top = frameRect.bottom + borderWidth;
                            frameRect.bottom = frameRect.top + frameDimension;
                        } else {
                            frameRect.left = frameRect.right + borderWidth;
                            frameRect.right = frameRect.left + frameDimension;
                        }
                    }

                    err = SWCreateFrameFromGWorldAndRectPartial(&newFrameP, tempPictGWorldP,
                            tempMaskGWorldP, tempTempMaskGWorldP, &frameRect, maskType);

                    if (err == noErr) {
                        err = SWAddFrame(tempSpriteP, newFrameP);
                    }

                    if (err != noErr) {
                        SWDisposeFrame(&newFrameP);
                        SWDisposeSprite(&tempSpriteP);
                        break;
                    }
                }
                SWCreateFrameFromGWorldAndRectFinish(tempTempMaskGWorldP);
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

                SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
                SWSetCurrentFrameIndex(tempSpriteP, 0);
                tempSpriteP->sharedPictGWorld = tempPictGWorldP;
                tempSpriteP->sharedMaskGWorld = tempMaskGWorldP;

                *newSpriteP = tempSpriteP;
            }
        }
    }

    if (rectArrayHndl != NULL) {
        HUnlock(rectArrayHndl);
        ReleaseResource(rectArrayHndl);
    }

    SWSetStickyIfError(err);
    return err;
}
#endif

#define kNoMask             0
#define kOutOfRangeErr      107

/* SWCreateSpriteFromSinglePictXY (mac _SWCreateSpriteFromSinglePictXY). STATE: MATCH, exe 277 / obj 277 instrs, byte-identical yes.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00474ab0
// MAC: _SWCreateSpriteFromSinglePictXY i386 0x000a9bee ppc 0x000b4adc
#ifdef MACBUILD
short SWCreateSpriteFromSinglePictXY(
        SpriteWorldRec *destSpriteWorldP,
        SpritePtr *newSpriteP,
        void *spriteStorageP,
        short pictResID,
        short maskResID,
        short frameWidth,
        short frameHeight,
        short borderWidth,
        short borderHeight,
        short maxFrames,
        MaskType maskType)
{
    short err;
    void *tempPic;
    CGrafPtr tempPictGWorldP;
    CGrafPtr tempMaskGWorldP;
    CGrafPtr tempTempMaskGWorldP;
    Rect frameRect;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short frame;
    Boolean needToInvertMask;

    err = 0;

    if (destSpriteWorldP == 0)
        SW_ASSERT_FAIL("Sprite.c", 719, 699);

    tempPictGWorldP = 0;
    tempMaskGWorldP = 0;
    *newSpriteP = 0;
    needToInvertMask = 1;

    tempPic = GetPicture(pictResID);
    if (tempPic == 0) {
        err = MemError();
        if (err == noErr)
            err = resNotFound;
    } else {
        QDReleaseResource(tempPic);
    }

    if (err == noErr) {
        err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempPictGWorldP, pictResID);

        if (err == noErr && maskType != kNoMask)
            err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempMaskGWorldP, maskResID);

        SWInvertGWorld(tempMaskGWorldP);

        if (err == noErr) {
            if (pictResID == maskResID && tempMaskGWorldP != 0) {
                err = SWCreatePixelMaskFromGWorld(tempPictGWorldP, tempMaskGWorldP, 0);
                needToInvertMask = 0;

                if (destSpriteWorldP->pixelDepth <= 8 && (maskType & kRegionMask) != 0) {
                    SWInvertGWorld(tempMaskGWorldP);
                    needToInvertMask = 1;
                }
            }
        }

        if (maxFrames == 0) {
            maxFrames = (GetCPortPortRect(tempPictGWorldP)->right / frameWidth) *
                        (GetCPortPortRect(tempPictGWorldP)->bottom / frameHeight);
        }

        if (err == noErr) {
            err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);
        }

        if (err == noErr) {
            err = SWCreateFrameFromGWorldAndRectStart(&tempTempMaskGWorldP, frameWidth,
                    frameHeight, maskType);
        }

        if (err == noErr) {
            frameRect.left = 0;
            frameRect.top = 0;
            frameRect.right = frameWidth;
            frameRect.bottom = frameHeight;

            for (frame = 0; frame < maxFrames; frame++) {
                err = SWCreateFrameFromGWorldAndRectPartial(&newFrameP, tempPictGWorldP,
                        tempMaskGWorldP, tempTempMaskGWorldP, &frameRect, maskType);

                if (err == noErr) {
                    newFrameP->rleDataH = 0;

                    err = SWAddFrame(tempSpriteP, newFrameP);
                    if (frame < maxFrames - 1) {
                        frameRect.left += frameWidth + borderWidth;
                        frameRect.right = frameRect.left + frameWidth;
                        if (frameRect.right >
                                GetCPortPortRect(tempPictGWorldP)->right) {
                            frameRect.left = 0;
                            frameRect.right = frameWidth;
                            frameRect.top += frameHeight + borderHeight;
                            frameRect.bottom = frameRect.top + frameHeight;

                            if (frameRect.bottom >
                                    GetCPortPortRect(tempPictGWorldP)->bottom) {
                                err = kOutOfRangeErr;
                            }
                        }
                    }
                }

                if (err != noErr) {
                    SWDisposeFrame(&newFrameP);
                    SWDisposeSprite(&tempSpriteP);
                    break;
                }
            }

            SWCreateFrameFromGWorldAndRectFinish(tempTempMaskGWorldP);
        }

        if (err == noErr) {
            if ((maskType & kPixelMask) != 0) {
                if (destSpriteWorldP->pixelDepth <= 8 && needToInvertMask) {
                    SWInvertGWorld(tempMaskGWorldP);
                }
            } else {
                if (tempMaskGWorldP != 0) {
                    DisposeGWorld(tempMaskGWorldP);
                    tempMaskGWorldP = 0;
                }
            }

            SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(tempSpriteP, 0);
            tempSpriteP->sharedPictGWorld = tempPictGWorldP;
            tempSpriteP->sharedMaskGWorld = tempMaskGWorldP;

            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateSpriteFromSinglePictXY(
        SpriteWorldRec *destSpriteWorldP,
        SpritePtr *newSpriteP,
        void *spriteStorageP,
        short pictResID,
        short maskResID,
        short frameWidth,
        short frameHeight,
        short borderWidth,
        short borderHeight,
        short maxFrames,
        unsigned char maskType)
{
    short err;
    void *tempPic;
    CGrafPtr tempPictGWorldP;
    CGrafPtr tempMaskGWorldP;
    CGrafPtr tempTempMaskGWorldP;
    Rect frameRect;
    SpritePtr tempSpriteP;
    FramePtr newFrameP;
    short frame;
    Boolean needToInvertMask;

    err = 0;

    if (destSpriteWorldP == 0)
        SWAssertFail("Sprite.c", 719);

    tempPictGWorldP = 0;
    tempMaskGWorldP = 0;
    *newSpriteP = 0;
    needToInvertMask = 1;

    tempPic = GetPicture(pictResID);
    if (tempPic == 0) {
        err = resNotFound;
    } else {
        QDReleaseResource(tempPic);
    }

    if ((int)err == noErr) {
        err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempPictGWorldP, pictResID, 0);

        if ((int)err == noErr && maskType != kNoMask)
            err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempMaskGWorldP, maskResID, 0);

        SWInvertGWorld(tempMaskGWorldP);

        if ((int)err == noErr) {
            if (pictResID == maskResID && tempMaskGWorldP != 0) {
                err = SWCreatePixelMaskFromGWorld(tempPictGWorldP, tempMaskGWorldP, 0);
                needToInvertMask = 0;
            }
        }

        if (maxFrames == 0) {
            maxFrames = (GetWindowPortRect(tempPictGWorldP)->right / frameWidth) *
                        (GetWindowPortRect(tempPictGWorldP)->bottom / frameHeight);
        }

        if ((int)err == noErr) {
            err = SWCreateSprite(&tempSpriteP, spriteStorageP, maxFrames);
        }

        if ((int)err == noErr) {
            err = SWCreateFrameFromGWorldAndRectStart(&tempTempMaskGWorldP, frameWidth,
                    frameHeight, maskType);
        }

        if ((int)err == noErr) {
            frameRect.right = frameWidth;
            frameRect.left = 0;
            frameRect.top = 0;
            frameRect.bottom = frameHeight;

            for (frame = 0; frame < maxFrames; frame++) {
                err = SWCreateFrameFromGWorldAndRectPartial(&newFrameP, tempPictGWorldP,
                        tempMaskGWorldP, tempTempMaskGWorldP, &frameRect, maskType);

                if ((int)err == noErr) {
                    newFrameP->rleDataH = 0;

                    err = SWAddFrame(tempSpriteP, newFrameP);
                    if (frame < maxFrames - 1) {
                        frameRect.left += frameWidth + borderWidth;
                        frameRect.right = frameRect.left + frameWidth;
                        if (frameRect.right >
                                GetWindowPortRect(tempPictGWorldP)->right) {
                            frameRect.top += frameHeight + borderHeight;
                            frameRect.left = 0;
                            frameRect.right = frameWidth;
                            frameRect.bottom = frameRect.top + frameHeight;

                            if (frameRect.bottom >
                                    GetWindowPortRect(tempPictGWorldP)->bottom) {
                                err = kOutOfRangeErr;
                            }
                        }
                    }
                }

                if ((int)err != noErr) {
                    SWDisposeFrame(&newFrameP);
                    SWDisposeSprite(&tempSpriteP);
                    break;
                }
            }

            SWCreateFrameFromGWorldAndRectFinish(tempTempMaskGWorldP);
        }

        if ((int)err == noErr) {
            if ((maskType & kPixelMask) != 0) {
                if (destSpriteWorldP->pixelDepth <= 8 && needToInvertMask) {
                    SWInvertGWorld(tempMaskGWorldP);
                }
            } else {
                if (tempMaskGWorldP != 0) {
                    DisposeGWorld(tempMaskGWorldP);
                    tempMaskGWorldP = 0;
                }
            }

            SWSetSpriteFrameRange(tempSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(tempSpriteP, 0);
            tempSpriteP->sharedPictGWorld = tempPictGWorldP;
            tempSpriteP->sharedMaskGWorld = tempMaskGWorldP;

            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}
#endif

#define kSolidMask      4


/* SWCreateSpriteFromRLEResource (mac _SWCreateSpriteFromRLEResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474e90
// MAC: _SWCreateSpriteFromRLEResource i386 0x000a9990 ppc 0x000b48b8
#ifdef MACBUILD
short SWCreateSpriteFromRLEResource(SpritePtr *srcSpriteP, void *spriteStorageP,
                                    short rleResID, short maxFrames, short mask)
{
    SpritePtr tempSpriteP;
    FramePtr tempFrameP;
    RLEPixelDataHdl spriteData;
    short numFrames;
    short frame;
    OSErr err = noErr;
    GDHandle device;
    short depth;
    Rect frameRect;
    Boolean createGWorld;

    device = GetGDevice();

    if (GetGDeviceDepth(device) == 8)
        spriteData = (RLEPixelDataHdl)GetResource(kRLE8ResType, rleResID);
    else
        spriteData = (RLEPixelDataHdl)GetResource(kRLEDResType, rleResID);

    if (spriteData != 0) {
        *srcSpriteP = 0;

        if (err == noErr) {
            numFrames = (**spriteData).frames;
            depth = (**spriteData).depth;
            SetRect(&frameRect,
                0, 0,
                (**spriteData).width,
                (**spriteData).height);
        }

        if (maxFrames > 0 && numFrames > maxFrames)
            numFrames = maxFrames;

        err = SWCreateSprite(&tempSpriteP, spriteStorageP, numFrames);

        if (err == noErr) {
            createGWorld = (mask & (kSolidMask | kPixelMask | kRegionMask)) != 0;

            for (frame = 0; err == noErr && frame < tempSpriteP->maxFrames; frame++) {
                err = SWCreateFrame(device, &tempFrameP, &frameRect, depth, createGWorld);

                if (err == noErr) {
                    tempFrameP->rleDataH = spriteData;
                    tempFrameP->rleTokenStart = 0;

                    err = SWAddFrame(tempSpriteP, tempFrameP);
                }
            }

            if (err == noErr) {
                SWSetSpriteFrameRange(tempSpriteP, 0, numFrames - 1);
                SWSetCurrentFrameIndex(tempSpriteP, 0);

                *srcSpriteP = tempSpriteP;
            } else {
                SWDisposeSprite(&tempSpriteP);
            }
        }

        tempSpriteP->rleDataH = (Handle)spriteData;
        tempSpriteP->ownsRLEData = 1;

        if (err == noErr && (HGetState((Handle)spriteData) & 0x20)) {
            DetachResource((Handle)spriteData);
            err = ResError();
        }
    } else {
        err = ResError();
        if (err == noErr)
            err = resNotFound;
    }

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateSpriteFromRLEResource(SpritePtr *srcSpriteP, void *spriteStorageP,
                                    short rleResID, short maxFrames, short mask)
{
    SpritePtr tempSpriteP;
    FramePtr tempFrameP;
    RLEPixelDataHdl spriteData;
    void *device;
    short numFrames;
    unsigned char createGWorld;
    short frame;
    short err = 0;
    short depth;
    Rect frameRect;

    device = GetGDevice();

    if (GetGDeviceDepth(device) == 8)
        spriteData = (RLEPixelDataHdl)GetResource(kRLE8ResType, rleResID);
    else
        spriteData = (RLEPixelDataHdl)GetResource(kRLEDResType, rleResID);

    if (spriteData != 0) {
        *srcSpriteP = 0;

        numFrames = (**spriteData).frames;
        depth = (**spriteData).depth;
        SetRect(&frameRect,
            0, 0,
            (**spriteData).width,
            (**spriteData).height);

        if (maxFrames > 0 && numFrames > maxFrames)
            numFrames = maxFrames;

        err = SWCreateSprite(&tempSpriteP, spriteStorageP, numFrames);

        if ((int)err == noErr) {
            createGWorld = (mask & (kSolidMask | kPixelMask)) != 0;

            for (frame = 0; (int)err == noErr && frame < tempSpriteP->maxFrames; frame++) {
                err = SWCreateFrame(device, &tempFrameP, &frameRect, depth, createGWorld);

                if ((int)err == noErr) {
                    tempFrameP->rleDataH = spriteData;
                    tempFrameP->rleTokenStart = 0;

                    err = SWAddFrame(tempSpriteP, tempFrameP);
                }
            }

            if ((int)err == noErr) {
                SWSetSpriteFrameRange(tempSpriteP, 0, numFrames - 1);
                SWSetCurrentFrameIndex(tempSpriteP, 0);

                *srcSpriteP = tempSpriteP;
            } else {
                SWDisposeSprite(&tempSpriteP);
            }
        }

        tempSpriteP->rleDataH = (Handle)spriteData;
        tempSpriteP->ownsRLEData = 1;

        if ((int)err == noErr)
            DetachResource((Handle)spriteData);
    } else {
        err = resNotFound;
    }

    SWSetStickyIfError(err);
    return err;
}
#endif

#ifdef MACBUILD
/* SWUpdateSpriteFromPictResource (mac _SWUpdateSpriteFromPictResource, mac-only). STATE: see the mac gate. */
// MAC: _SWUpdateSpriteFromPictResource i386 0x000aaadf ppc 0x000b589c
OSErr SWUpdateSpriteFromPictResource(SpritePtr theSpriteP, short pictResID)
{
    OSErr err = noErr;
    GDHandle currentGDH;
    GWorldPtr currentGWorld;
    GWorldPtr frameGWorld;
    PicHandle pictH;
    Rect frameRect;
    short frame;

    SW_ASSERT(theSpriteP != NULL, 1103);

    GetGWorld(&currentGWorld, &currentGDH);

    SWLockSprite(theSpriteP);

    if (theSpriteP->sharedPictGWorld != NULL) {
        SetGWorld(theSpriteP->sharedPictGWorld, NULL);
        pictH = GetPicture(pictResID);
        if (pictH != NULL) {
            frameRect = *GetCPortPortRect(theSpriteP->sharedPictGWorld);
            EraseRect(&frameRect);
            DrawPicture(pictH, &frameRect);
            ReleaseResource((Handle)pictH);

            SWFastWhitenGWorld(theSpriteP->sharedPictGWorld);
        } else {
            err = MemError();
            if (err == noErr)
                err = resNotFound;
        }
    } else {
        for (frame = 0; frame < theSpriteP->maxFrames; frame++) {
            frameGWorld = theSpriteP->frameArray[frame]->framePort;

            if (frameGWorld != NULL) {
                SetGWorld(frameGWorld, nil);
                pictH = GetPicture(pictResID + frame);
                if (pictH != NULL) {
                    frameRect = *GetCPortPortRect(frameGWorld);
                    EraseRect(&frameRect);
                    DrawPicture(pictH, &frameRect);
                    ReleaseResource((Handle)pictH);

                    SWFastWhitenGWorld(frameGWorld);
                } else {
                    err = MemError();
                    if (err == noErr)
                        err = resNotFound;
                    break;
                }
            }
        }
    }
    if (useLegacyRenderingPipeline)
        SetGWorld(currentGWorld, currentGDH);
    else
        SetGWorld(currentGWorld, NULL);

    SWSetStickyIfError(err);
    return err;
}

/* CalcMaxWidthHeight (mac static, inlined). STATE: see the mac gate. */
static void CalcMaxWidthHeight(RectArrayPtr newRectArrayP, short *maxWidthP, short *maxHeightP)
{
    short frame;
    Rect *rectP;
    short maxWidth, maxHeight;

    maxWidth = 0;
    maxHeight = 0;

    rectP = &newRectArrayP->frameRects[0];
    for(frame = 0; frame < newRectArrayP->numFrames; frame++) {
        if (rectP->right - rectP->left > maxWidth) {
            maxWidth = rectP->right - rectP->left;
        }
        if (rectP->bottom - rectP->top > maxHeight) {
            maxHeight = rectP->bottom - rectP->top;
        }
        rectP++;
    }
    *maxWidthP = maxWidth;
    *maxHeightP =maxHeight;
}

/* SWSaveRLESprite (mac _SWSaveRLESprite, mac-only). STATE: see the mac gate. */
// MAC: _SWSaveRLESprite i386 0x000a92c6 ppc 0x000b41e4
OSErr SWSaveRLESprite(SpritePtr srcSpriteP, short rleResID)
{
    OSErr err = noErr;
    Handle spriteData;
    Handle oldResHandle;

    spriteData = srcSpriteP->rleDataH;
    if (spriteData == NULL)
        err = kSpriteNotCompiledErr;

    HLock(spriteData);
    HNoPurge(spriteData);

    if (err == noErr) {
        if (GetGDeviceDepth(GetGDevice()) == 8)
            oldResHandle = GetResource(kRLE8ResType, rleResID);
        else
            oldResHandle = GetResource(kRLEDResType, rleResID);
        if (oldResHandle != NULL)
            RemoveResource(oldResHandle);

        HLock(spriteData);
        if (GetGDeviceDepth(GetGDevice()) == 8)
            AddResource(spriteData, kRLE8ResType, rleResID, "\p");
        else
            AddResource(spriteData, kRLEDResType, rleResID, "\p");
        ChangedResource(spriteData);
        WriteResource(spriteData);
        UpdateResFile(CurResFile());
        DetachResource(spriteData);
    }

    SWSetStickyIfError(err);
    return err;
}
#endif

#define kNilParameterErr        104

/* SWCloneSprite (mac _SWCloneSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475050
// MAC: _SWCloneSprite i386 0x000a9419 ppc 0x000b430c
#ifdef MACBUILD
short SWCloneSprite(SpritePtr cloneSpriteP, SpritePtr *newSpriteP, void *spriteStorageP)
{
    SpritePtr tempSpriteP;
    short index;
    long frameArraySize;
    short err = 0;

    if (cloneSpriteP == 0)
        return kNilParameterErr;

    tempSpriteP = (spriteStorageP == 0) ? (SpritePtr)AllocAPointer(sizeof(SpriteRec))
                                        : (SpritePtr)spriteStorageP;
    if (tempSpriteP == 0)
        err = MemError();

    if (err == noErr) {
        if (GetPtrSize((Ptr)tempSpriteP) < sizeof(SpriteRec))
            SW_ASSERT_FAIL("Sprite.c", 1400, 1361);

        *tempSpriteP = *cloneSpriteP;

        tempSpriteP->parentSpriteLayerP = 0;
        tempSpriteP->nextSpriteP = 0;
        tempSpriteP->prevSpriteP = 0;

        tempSpriteP->spriteMoveProc = SWTempSpriteMoveProc;
        tempSpriteP->realSpriteMoveProc = cloneSpriteP->realSpriteMoveProc;
        tempSpriteP->ownsRLEData = 0;
    }

    if (err == noErr) {
        frameArraySize = cloneSpriteP->maxFrames * sizeof(FramePtr);
        tempSpriteP->frameArray = (FramePtr *)AllocAPointerClear(frameArraySize);

        if (tempSpriteP->frameArray != 0) {
            for (index = 0; index < cloneSpriteP->maxFrames; index++) {
                tempSpriteP->frameArray[index] = cloneSpriteP->frameArray[index];
                if (tempSpriteP->frameArray[index] == 0)
                    SW_ASSERT_FAIL("Sprite.c", 1430, 1391);
                tempSpriteP->frameArray[index]->useCount++;
            }
        } else {
            err = MemError();
            DeallocAPointer(tempSpriteP);
        }
    }

    if (err == noErr)
        *newSpriteP = tempSpriteP;

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCloneSprite(SpritePtr cloneSpriteP, SpritePtr *newSpriteP, void *spriteStorageP)
{
    SpritePtr tempSpriteP;
    short index;
    long frameArraySize;
    short err = 0;

    if (cloneSpriteP == 0)
        return kNilParameterErr;

    tempSpriteP = (spriteStorageP == 0) ? (SpritePtr)AllocAPointer(sizeof(SpriteRec))
                                        : (SpritePtr)spriteStorageP;
    if (tempSpriteP == 0)
        err = memFullErr;

    if ((int)err == noErr) {
        if (GetAPointerSize(tempSpriteP) < sizeof(SpriteRec))
            SWAssertFail("Sprite.c", 1400);

        tempSpriteP->parentSpriteLayerP = cloneSpriteP->parentSpriteLayerP;
        tempSpriteP->nextSpriteP = cloneSpriteP->nextSpriteP;
        tempSpriteP->prevSpriteP = cloneSpriteP->prevSpriteP;
        tempSpriteP->nextActiveSpriteP = cloneSpriteP->nextActiveSpriteP;
        tempSpriteP->nextIdleSpriteP = cloneSpriteP->nextIdleSpriteP;
        tempSpriteP->isVisible = cloneSpriteP->isVisible;
        tempSpriteP->needsToBeDrawn = cloneSpriteP->needsToBeDrawn;
        tempSpriteP->needsToBeErased = cloneSpriteP->needsToBeErased;
        tempSpriteP->onTop = cloneSpriteP->onTop;
        tempSpriteP->justBecameVisible = cloneSpriteP->justBecameVisible;
        tempSpriteP->pad1b = cloneSpriteP->pad1b;
        tempSpriteP->destFrameRect = cloneSpriteP->destFrameRect;
        tempSpriteP->oldFrameRect = cloneSpriteP->oldFrameRect;
        tempSpriteP->deltaFrameRect = cloneSpriteP->deltaFrameRect;
        tempSpriteP->frameDrawProc = cloneSpriteP->frameDrawProc;
        tempSpriteP->tileDepth = cloneSpriteP->tileDepth;
        tempSpriteP->destRectIsVisible = cloneSpriteP->destRectIsVisible;
        tempSpriteP->oldRectIsVisible = cloneSpriteP->oldRectIsVisible;
        tempSpriteP->clippedSourceRect = cloneSpriteP->clippedSourceRect;
        tempSpriteP->destOffscreenRect = cloneSpriteP->destOffscreenRect;
        tempSpriteP->frameArray = cloneSpriteP->frameArray;
        tempSpriteP->curFrameP = cloneSpriteP->curFrameP;
        tempSpriteP->numFrames = cloneSpriteP->numFrames;
        tempSpriteP->maxFrames = cloneSpriteP->maxFrames;
        tempSpriteP->frameTimeInterval = cloneSpriteP->frameTimeInterval;
        tempSpriteP->timeOfLastFrameChange = cloneSpriteP->timeOfLastFrameChange;
        tempSpriteP->frameAdvance = cloneSpriteP->frameAdvance;
        tempSpriteP->curFrameIndex = cloneSpriteP->curFrameIndex;
        tempSpriteP->firstFrameIndex = cloneSpriteP->firstFrameIndex;
        tempSpriteP->lastFrameIndex = cloneSpriteP->lastFrameIndex;
        tempSpriteP->frameChangeProc = cloneSpriteP->frameChangeProc;
        tempSpriteP->moveTimeInterval = cloneSpriteP->moveTimeInterval;
        tempSpriteP->timeOfLastMove = cloneSpriteP->timeOfLastMove;
        tempSpriteP->horizMoveDelta = cloneSpriteP->horizMoveDelta;
        tempSpriteP->vertMoveDelta = cloneSpriteP->vertMoveDelta;
        tempSpriteP->moveBoundsRect = cloneSpriteP->moveBoundsRect;
        tempSpriteP->spriteMoveProc = cloneSpriteP->spriteMoveProc;
        tempSpriteP->realSpriteMoveProc = cloneSpriteP->realSpriteMoveProc;
        tempSpriteP->spriteCollideProc = cloneSpriteP->spriteCollideProc;
        tempSpriteP->sharedPictGWorld = cloneSpriteP->sharedPictGWorld;
        tempSpriteP->sharedMaskGWorld = cloneSpriteP->sharedMaskGWorld;
        tempSpriteP->spriteRemoval = cloneSpriteP->spriteRemoval;
        tempSpriteP->frameAdvanceMode = cloneSpriteP->frameAdvanceMode;
        tempSpriteP->translucency = cloneSpriteP->translucency;
        tempSpriteP->tintRed = cloneSpriteP->tintRed;
        tempSpriteP->tintGreen = cloneSpriteP->tintGreen;
        tempSpriteP->tintBlue = cloneSpriteP->tintBlue;
        tempSpriteP->tintMode = cloneSpriteP->tintMode;
        tempSpriteP->tintColor = cloneSpriteP->tintColor;
        tempSpriteP->rotation = cloneSpriteP->rotation;
        tempSpriteP->scaledWidth = cloneSpriteP->scaledWidth;
        tempSpriteP->scaledHeight = cloneSpriteP->scaledHeight;
        tempSpriteP->colorValue = cloneSpriteP->colorValue;
        tempSpriteP->colorRGB = cloneSpriteP->colorRGB;
        tempSpriteP->flippedHorizontal = cloneSpriteP->flippedHorizontal;
        tempSpriteP->flippedVertical = cloneSpriteP->flippedVertical;
        tempSpriteP->pad2 = cloneSpriteP->pad2;
        tempSpriteP->userData = cloneSpriteP->userData;
        tempSpriteP->rleDataH = cloneSpriteP->rleDataH;
        tempSpriteP->collisionsDisabled = cloneSpriteP->collisionsDisabled;
        tempSpriteP->ownsRLEData = cloneSpriteP->ownsRLEData;
        tempSpriteP->isEmptySprite = cloneSpriteP->isEmptySprite;
        tempSpriteP->srD3 = cloneSpriteP->srD3;

        tempSpriteP->parentSpriteLayerP = 0;
        tempSpriteP->nextSpriteP = 0;
        tempSpriteP->prevSpriteP = 0;

        tempSpriteP->spriteMoveProc = SWTempSpriteMoveProc;
        tempSpriteP->realSpriteMoveProc = cloneSpriteP->realSpriteMoveProc;
        tempSpriteP->ownsRLEData = 0;
    }

    if ((int)err == noErr) {
        frameArraySize = cloneSpriteP->maxFrames * sizeof(FramePtr);
        tempSpriteP->frameArray = AllocAPointerClear(frameArraySize);

        if (tempSpriteP->frameArray != 0) {
            for (index = 0; index < cloneSpriteP->maxFrames; index++) {
                tempSpriteP->frameArray[index] = cloneSpriteP->frameArray[index];
                if (tempSpriteP->frameArray[index] == 0)
                    SWAssertFail("Sprite.c", 1430);
                tempSpriteP->frameArray[index]->useCount++;
            }
        } else {
            err = memFullErr;
            DeallocAPointer(tempSpriteP);
        }
    }

    if ((int)err == noErr)
        *newSpriteP = tempSpriteP;

    SWSetStickyIfError(err);
    return err;
}
#endif

#ifdef MACBUILD
/* SWCloneSpriteFromTile (mac _SWCloneSpriteFromTile, mac-only). STATE: see the mac gate. */
// MAC: _SWCloneSpriteFromTile i386 0x000a979a ppc 0x000b46f4
OSErr SWCloneSpriteFromTile(
    SpriteWorldPtr spriteWorldP,
    SpritePtr* newSpriteP,
    void* spriteStorageP,
    short firstTileID,
    short lastTileID)
{
    OSErr err;
    SpritePtr tempSpriteP;
    FramePtr tileFrameP;
    short curTileID, curIndex, numFrames;

    SW_ASSERT(firstTileID <= lastTileID, 1427);
    SW_ASSERT(spriteWorldP->tileFrameArray != NULL, 1428);
    SW_ASSERT(firstTileID >= 0 && lastTileID < spriteWorldP->maxNumTiles, 1429);

    *newSpriteP = NULL;
    numFrames = (lastTileID - firstTileID) + 1;
    err = SWCreateSprite(&tempSpriteP, spriteStorageP, numFrames);

    if (err == noErr) {
        for (curTileID = firstTileID; curTileID <= lastTileID; curTileID++) {
            tileFrameP = spriteWorldP->tileFrameArray[curTileID];
            SW_ASSERT(tileFrameP != NULL, 1440);
            err = SWAddFrame(tempSpriteP, tileFrameP);

            if (err != noErr) {
                SWDisposeSprite(&tempSpriteP);
                break;
            }
        }

        if (err == noErr) {
            SWSetSpriteFrameRange(tempSpriteP, 0, numFrames - 1);

            curIndex = spriteWorldP->curTileImage[firstTileID];
            if (curIndex >= firstTileID && curIndex <= lastTileID)
                curIndex = spriteWorldP->curTileImage[firstTileID] - firstTileID;
            else
                curIndex = 0;

            SWSetCurrentFrameIndex(tempSpriteP, curIndex);
            *newSpriteP = tempSpriteP;
        }
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWRemoveSpriteFromAnimation (mac _SWRemoveSpriteFromAnimation, mac-only). STATE: see the mac gate. */
// MAC: _SWRemoveSpriteFromAnimation i386 0x000a95dc ppc 0x000b450c
void SWRemoveSpriteFromAnimation(
    SpriteWorldPtr spriteWorldP,
    SpritePtr spriteP,
    Boolean disposeOfSprite)
{
    SW_ASSERT(spriteWorldP != NULL && spriteP != NULL, 1481);

    SWSetSpriteVisible(spriteP, false);
    SWRemoveSprite(spriteP);
    SWAddSprite(spriteWorldP->deadSpriteLayerP, spriteP);

    if (disposeOfSprite) {
        spriteP->spriteRemoval = kSWRemoveAndDisposeSprite;
    } else {
        spriteP->spriteRemoval = kSWRemoveSprite;
    }
}
#endif

/* +0xd2 marks a sprite that owns none of the things SWCloseSprite frees; +0xd1 is
   RLE-handle OWNERSHIP, since SWCloneSprite copies the handle and clears it. */

/* SWDisposeSprite (mac _SWDisposeSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475470
// MAC: _SWDisposeSprite i386 0x000a974e ppc 0x000b468c
void SWDisposeSprite(SpritePtr *deadSpritePP)
{
    SpritePtr deadSpriteP = *deadSpritePP;

    if (deadSpriteP != 0) {
        if (!deadSpriteP->isEmptySprite)
            SWCloseSprite(deadSpriteP);

        DeallocAPointer(deadSpriteP);

        *deadSpritePP = 0;
    }
}

/* SWCloseSprite (mac _SWCloseSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004754a0
// MAC: _SWCloseSprite i386 0x000a9666 ppc 0x000b459c
void SWCloseSprite(SpritePtr deadSpriteP)
{
    short frame;
    Boolean framesDisposed;

    if (deadSpriteP != 0) {
        if (deadSpriteP->ownsRLEData && deadSpriteP->rleDataH != 0) {
            DeallocAHandle(deadSpriteP->rleDataH);
            deadSpriteP->rleDataH = 0;
        }

        for (frame = 0; frame < deadSpriteP->numFrames; frame++) {
            framesDisposed = SWDisposeFrame(&deadSpriteP->frameArray[frame]);
        }

        /* BUG: read uninitialised when numFrames is 0 */
        if (framesDisposed) {
            if (deadSpriteP->sharedPictGWorld != 0) {
                DisposeGWorld(deadSpriteP->sharedPictGWorld);
                deadSpriteP->sharedPictGWorld = 0;
            }

            if (deadSpriteP->sharedMaskGWorld != 0) {
                DisposeGWorld(deadSpriteP->sharedMaskGWorld);
                deadSpriteP->sharedMaskGWorld = 0;
            }
        }

        if (deadSpriteP->frameArray != 0) {
            DeallocAPointer(deadSpriteP->frameArray);
            deadSpriteP->frameArray = 0;
        }
    }
}

/* Token constants read off the exe: `shr edx,0x18' gives kTokenShift 24 and makes
   TokenDataType unsigned, kEndShapeToken is 0, kLineStartToken 1, kCountMask 0xffffff and
   kRLEResourceHeaderSize 0x10. `dataLength' is SIGNED -- the difference is tested `jle'. */


typedef unsigned long TokenDataType;

#define kTokenShift             24
#define kCountMask              0x00ffffff

/* SWLockSprite (mac _SWLockSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475550
// MAC: _SWLockSprite i386 0x000aa965 ppc 0x000b5710
#ifdef MACBUILD
void SWLockSprite(SpritePtr srcSpriteP)
{
    register long curFrame;
    FramePtr srcFrameP;
    Handle spriteData;
    long dataLength;
    TokenDataType *tokens;
    long offset;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 1623, 1585);

    for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++)
        SWLockFrame(srcSpriteP->frameArray[curFrame]);

    spriteData = srcSpriteP->rleDataH;
    if (spriteData != 0) {
        LockAHandle(spriteData);

        dataLength = GetAHandleSize(spriteData) - kRLEResourceHeaderSize;
        if (dataLength <= 0)
            SW_ASSERT_FAIL("Sprite.c", 1642, 1604);

        offset = 0;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
        tokens = (TokenDataType *)(*spriteData + kRLEResourceHeaderSize
                                   - ((long)(*spriteData + kRLEResourceHeaderSize) & 3));
#else
        tokens = (TokenDataType *)(*spriteData + kRLEResourceHeaderSize);
#ifndef MAC1010
        (long)tokens &= ~3;
#endif
#endif

        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            srcFrameP = srcSpriteP->frameArray[curFrame];
            if (srcFrameP->rleDataH != spriteData)
                SW_ASSERT_FAIL("Sprite.c", 1656, 1618);

            srcFrameP->rleTokenStart = tokens;

            while (*tokens != (kEndShapeToken << kTokenShift)) {
                if ((*tokens >> kTokenShift) != kLineStartToken)
                    SW_ASSERT_FAIL("Sprite.c", 1665, 1627);

                offset = (*tokens & kCountMask) + offset;
                tokens = (TokenDataType *)((char *)tokens + (*tokens & kCountMask) + sizeof(TokenDataType));
                offset += sizeof(TokenDataType);

                if (offset > dataLength)
                    SW_ASSERT_FAIL("Sprite.c", 1672, 1634);
            }

            offset += sizeof(TokenDataType);
            tokens = (TokenDataType *)((char *)tokens + sizeof(TokenDataType));
        }
    }
}
#else
void SWLockSprite(SpritePtr srcSpriteP)
{
    register long curFrame;
    FramePtr srcFrameP;
    Handle spriteData;
    long dataLength;
    TokenDataType *tokens;
    long offset;

    if (srcSpriteP == 0)
        SWAssertFail("Sprite.c", 1623);

    for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
        SWLockFrame(srcSpriteP->frameArray[curFrame]);
    }

    spriteData = srcSpriteP->rleDataH;
    if (spriteData != 0) {
        LockAHandle(spriteData);

        dataLength = GetAHandleSize(spriteData) - kRLEResourceHeaderSize;
        if (dataLength <= 0)
            SWAssertFail("Sprite.c", 1642);

        offset = 0;
        tokens = (TokenDataType *)(*spriteData + kRLEResourceHeaderSize);

        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            srcFrameP = srcSpriteP->frameArray[curFrame];
            if (srcFrameP->rleDataH != spriteData)
                SWAssertFail("Sprite.c", 1656);

            srcFrameP->rleTokenStart = tokens;

            while (*tokens != (kEndShapeToken << kTokenShift)) {
                if ((*tokens >> kTokenShift) != kLineStartToken)
                    SWAssertFail("Sprite.c", 1665);

                offset += (*tokens & kCountMask) + sizeof(TokenDataType);
                tokens = (TokenDataType *)((char *)tokens + sizeof(TokenDataType) +
                                           (*tokens & kCountMask));

                if (offset > dataLength)
                    SWAssertFail("Sprite.c", 1672);
            }

            offset += sizeof(TokenDataType);
            tokens = (TokenDataType *)((char *)tokens + sizeof(TokenDataType));
        }
    }
}
#endif

/* SWUnlockSprite (mac _SWUnlockSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004756c0
// MAC: _SWUnlockSprite i386 0x000aad04 ppc 0x000b5aa8
#ifdef MACBUILD
void SWUnlockSprite(SpritePtr srcSpriteP)
{
    register long curFrame;
    Handle spriteData;
    FramePtr srcFrameP;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 1693, 1655);

    for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
        SWUnlockFrame(srcSpriteP->frameArray[curFrame]);
    }

    spriteData = srcSpriteP->frameArray[0]->rleDataH;
    if (spriteData != 0) {
        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            srcFrameP = srcSpriteP->frameArray[curFrame];

            srcFrameP->rleTokenStart = 0;
        }

        UnlockAHandle(spriteData);
    }
}
#else
void SWUnlockSprite(SpritePtr srcSpriteP)
{
    register long curFrame;
    Handle spriteData;
    FramePtr srcFrameP;

    if (srcSpriteP == 0)
        SWAssertFail("Sprite.c", 1693);

    if (srcSpriteP->frameArray != 0) {
        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            SWUnlockFrame(srcSpriteP->frameArray[curFrame]);
        }
    }

    spriteData = srcSpriteP->frameArray[0]->rleDataH;
    if (spriteData != 0) {
        for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
            srcFrameP = srcSpriteP->frameArray[curFrame];

            srcFrameP->rleTokenStart = 0;
        }

        UnlockAHandle(spriteData);
    }
}
#endif

#ifdef MACBUILD
/* SWFlipSprite (mac _SWFlipSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWFlipSprite i386 0x000aadc0 ppc 0x000b5b84
void SWFlipSprite(SpritePtr srcSpriteP, FlipType direction)
{
    long curFrame;
    FramePtr srcFrameP;

    for (curFrame = 0; curFrame < srcSpriteP->numFrames; curFrame++) {
        srcFrameP = srcSpriteP->frameArray[curFrame];

        if (srcFrameP->framePort != NULL) {
            if (direction & kSWFlipHorizontal)
                SWFlipPixelsHorizontally(srcFrameP->framePort, &srcFrameP->frameRect);

            if (direction & kSWFlipVertical)
                SWFlipPixelsVertically(srcFrameP->framePort, &srcFrameP->frameRect);

            if (srcFrameP->maskRgn == NULL

#if SW_68K
                && srcFrameP->pixCodeH == NULL
#endif
                ) {
                if (srcFrameP->maskPort != NULL) {
                    if (direction & kSWFlipHorizontal)
                        SWFlipPixelsHorizontally(srcFrameP->maskPort, &srcFrameP->frameRect);

                    if (direction & kSWFlipVertical)
                        SWFlipPixelsVertically(srcFrameP->maskPort, &srcFrameP->frameRect);
                }
            } else {
                SWUpdateFrameMasks(srcFrameP);
            }
        }
    }
}
#endif

/* kMaxFramesErr = 101, read straight off the exe's `mov ebx, 0x65'.
   SpriteRec layout: macport.h; its evidence: Research/notes/swsetcurrentframeindex.md. */

#define kMaxFramesErr   101

/* SWAddFrame (mac _SWAddFrame). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475740
// MAC: _SWAddFrame i386 0x000a9043 ppc 0x000b3eec
short SWAddFrame(SpritePtr srcSpriteP, FramePtr newFrameP)
{
    short err = 0;

    if (srcSpriteP == 0 || newFrameP == 0)
        SW_ASSERT_FAIL("Sprite.c", 1783, 1742);

    if (srcSpriteP->numFrames < srcSpriteP->maxFrames) {
        srcSpriteP->frameArray[srcSpriteP->numFrames] = newFrameP;

        srcSpriteP->numFrames++;
        newFrameP->useCount++;
    } else {
        err = kMaxFramesErr;
    }

    SWSetStickyIfError(err);
    return err;
}

#ifdef MACBUILD
/* SWInsertFrame (mac _SWInsertFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWInsertFrame i386 0x000aaea5 ppc 0x000b5c88
OSErr SWInsertFrame(SpritePtr srcSpriteP, FramePtr newFrameP, long frameIndex)
{
    register long index;
    register FramePtr *frameArray;
    OSErr err = noErr;

    SW_ASSERT(srcSpriteP != NULL && newFrameP != NULL, 1776);

    frameArray = srcSpriteP->frameArray;

    if (frameIndex < srcSpriteP->maxFrames && frameIndex >= 0 &&
        srcSpriteP->numFrames < srcSpriteP->maxFrames) {
        for (index = srcSpriteP->numFrames; index > frameIndex; index--)
            frameArray[index] = frameArray[index - 1];

        frameArray[frameIndex] = newFrameP;

        srcSpriteP->numFrames++;
        newFrameP->useCount++;
    } else {
        err = kMaxFramesErr;
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWRemoveFrame (mac _SWRemoveFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWRemoveFrame i386 0x000aaf72 ppc 0x000b5d6c
void SWRemoveFrame(SpritePtr srcSpriteP, FramePtr oldFrameP)
{
    register long curFrame;
    register FramePtr *frameArray;

    SW_ASSERT(srcSpriteP != NULL && oldFrameP != NULL, 1815);

    curFrame = srcSpriteP->numFrames;
    frameArray = srcSpriteP->frameArray;

    while (curFrame--) {
        if (frameArray[curFrame] == oldFrameP) {
            srcSpriteP->numFrames--;
            oldFrameP->useCount--;

            while (curFrame <= (srcSpriteP->numFrames - 1L)) {
                frameArray[curFrame] = frameArray[curFrame + 1L];
                curFrame++;
            }

            break;
        }
    }
}
#endif

/* maskRgn is never dereferenced -- this port has no Region Manager, so the donor's
   OffsetRgn block is gone -- but the handle is still PASSED to CopyBits as the mask,
   which is why the field is read at all. */

/* SWStdSpriteDrawProc (mac _SWStdSpriteDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004757a0
// MAC: _SWStdSpriteDrawProc i386 0x000ab015 ppc 0x000b5e34
#ifdef MACBUILD
void SWStdSpriteDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect, Rect *dstRect)
{
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked, 1851);

    if (srcFrameP->maskRgn != NULL) {
        Rect rgnRect;

#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        rgnRect = (**(RgnHandle)srcFrameP->maskRgn).rgnBBox;
#else
        GetRegionBounds(srcFrameP->maskRgn, &rgnRect);
#endif

        OffsetRgn(srcFrameP->maskRgn,
            (dstRect->left - (srcRect->left - srcFrameP->frameRect.left) - rgnRect.left)
            + srcFrameP->offsetPoint.h,
            (dstRect->top - (srcRect->top - srcFrameP->frameRect.top) - rgnRect.top)
            + srcFrameP->offsetPoint.v);
    }

    CopyBits((BitMapPtr)srcFrameP->framePix, (BitMapPtr)dstFrameP->framePix,
             srcRect, dstRect, srcCopy, srcFrameP->maskRgn);
}
#else
void SWStdSpriteDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect, Rect *dstRect)
{
    if (!srcFrameP->isFrameLocked || !dstFrameP->isFrameLocked)
        SWAssertFail("Sprite.c", 1892);

    CopyBits(GetPortBitMapForCopyBits(srcFrameP->framePort),
             GetPortBitMapForCopyBits(dstFrameP->framePort),
             srcRect, dstRect, srcCopy, srcFrameP->maskRgn);
}
#endif

#ifdef MACBUILD
/* SWStdSpriteColorDrawProc (mac _SWStdSpriteColorDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWStdSpriteColorDrawProc i386 0x000ab0e9 ppc 0x000b5f24
void SWStdSpriteColorDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect* srcRect, Rect* dstRect)
{
    SW_ASSERT(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked, 1887);

    if (srcFrameP->maskRgn != NULL) {
        Rect rgnRect;

        GetRegionBounds(srcFrameP->maskRgn, &rgnRect);

        OffsetRgn(srcFrameP->maskRgn,
            (dstRect->left - (srcRect->left - srcFrameP->frameRect.left) - rgnRect.left)
            + srcFrameP->offsetPoint.h,
            (dstRect->top - (srcRect->top - srcFrameP->frameRect.top) - rgnRect.top)
            + srcFrameP->offsetPoint.v);
    }

    RGBBackColor(&gSWCurrentSpriteBeingDrawn->colorRGB);
    EraseRgn(srcFrameP->maskRgn);
    BackColor(whiteColor);
}
#endif

/* `[s+0x8c] = [s+0x90]', then a call through +0x90 guarded on the byte +0x15: that is
   what pins spriteMoveProc/realSpriteMoveProc to +0x8c/+0x90 and needsToBeDrawn to +0x15
   for the rest of the file. */

/* SWTempSpriteMoveProc (mac _SWTempSpriteMoveProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475800
// MAC: _SWTempSpriteMoveProc i386 0x000ab1c9 ppc 0x000b600c
void SWTempSpriteMoveProc(SpritePtr srcSpriteP)
{
    srcSpriteP->spriteMoveProc = srcSpriteP->realSpriteMoveProc;

    if (srcSpriteP->needsToBeDrawn == 0) {
        if (srcSpriteP->realSpriteMoveProc != 0)
            (*srcSpriteP->realSpriteMoveProc)(srcSpriteP);
    }
}

#ifdef MACBUILD
/* SWSetCurrentFrame (mac _SWSetCurrentFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWSetCurrentFrame i386 0x000ab1f2 ppc 0x000b6034
OSErr SWSetCurrentFrame(SpritePtr srcSpriteP, FramePtr newFrameP)
{
    short frameIndex = 0;
    Boolean foundFrame = false;
    OSErr err = noErr;

    SW_ASSERT(srcSpriteP != NULL && newFrameP != NULL, 1949);

    for (frameIndex = 0; frameIndex < srcSpriteP->numFrames; frameIndex++) {
        if (srcSpriteP->frameArray[frameIndex] == newFrameP) {
            SWSetCurrentFrameIndex(srcSpriteP, frameIndex);
            foundFrame = true;
            break;
        }
    }

    if (foundFrame == false)
        err = kBadParameterErr;

    SWSetStickyIfError(err);
    return err;
}
#endif

/* SWSetCurrentFrameIndex (mac _SWSetCurrentFrameIndex). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475830
// MAC: _SWSetCurrentFrameIndex i386 0x000a90b6 ppc 0x000b3f88
void SWSetCurrentFrameIndex(SpritePtr srcSpriteP, short frameIndex)
{
    register FramePtr newFrameP;
    short horizOffset, vertOffset;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2025, 1980);

    if (frameIndex < 0)
        frameIndex = 0;

    frameIndex = frameIndex % srcSpriteP->numFrames;

    if (frameIndex < srcSpriteP->numFrames) {
        newFrameP = srcSpriteP->frameArray[frameIndex];

        if (srcSpriteP->curFrameP == 0)
            srcSpriteP->curFrameP = newFrameP;

        horizOffset = (srcSpriteP->destFrameRect.left - newFrameP->frameRect.left) +
                    (srcSpriteP->curFrameP->hotSpotH - newFrameP->hotSpotH);
        vertOffset = (srcSpriteP->destFrameRect.top - newFrameP->frameRect.top) +
                    (srcSpriteP->curFrameP->hotSpotV - newFrameP->hotSpotV);

        srcSpriteP->curFrameP = newFrameP;
#ifdef MACBUILD
        srcSpriteP->destFrameRect = newFrameP->frameRect;
#else
        srcSpriteP->destFrameRect.top = newFrameP->frameRect.top;
        srcSpriteP->destFrameRect.left = newFrameP->frameRect.left;
        srcSpriteP->destFrameRect.bottom = newFrameP->frameRect.bottom;
        srcSpriteP->destFrameRect.right = newFrameP->frameRect.right;
#endif

        if (srcSpriteP->scaledWidth > 0)
            srcSpriteP->destFrameRect.right = srcSpriteP->destFrameRect.left +
                                                srcSpriteP->scaledWidth;
        if (srcSpriteP->scaledHeight > 0)
            srcSpriteP->destFrameRect.bottom = srcSpriteP->destFrameRect.top +
                                                srcSpriteP->scaledHeight;

        srcSpriteP->destFrameRect.left += horizOffset;
        srcSpriteP->destFrameRect.right += horizOffset;
        srcSpriteP->destFrameRect.top += vertOffset;
        srcSpriteP->destFrameRect.bottom += vertOffset;

        srcSpriteP->curFrameIndex = frameIndex;

        srcSpriteP->needsToBeDrawn = 1;
    }
}

/* The only store is `[sprite+0x94] = arg2', and +0x94 is spriteCollideProc -- pinned by
   SWTempSpriteMoveProc, which moves +0x90 into +0x8c and so fixes
   realSpriteMoveProc/spriteMoveProc, leaving the donor's next member at +0x94. */

/* SWSetSpriteCollideProc (mac _SWSetSpriteCollideProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475940
// MAC: _SWSetSpriteCollideProc i386 0x000ab28d ppc 0x000b60fc
void SWSetSpriteCollideProc(SpritePtr srcSpriteP, CollideProcPtr collideProc)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2079, 2034);

    srcSpriteP->spriteCollideProc = collideProc;
}

#ifdef MACBUILD
/* SWSetSpriteColor (mac _SWSetSpriteColor, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteColor i386 0x000ab2d3 ppc 0x000b614c
void SWSetSpriteColor(SpritePtr srcSpriteP, RGBColor *color)
{
    FramePtr frameP;

    frameP = srcSpriteP->curFrameP != NULL ? srcSpriteP->curFrameP : srcSpriteP->frameArray[0];

    srcSpriteP->colorRGB = *color;
    srcSpriteP->colorValue = SWGetValueFromColor(frameP, color);
}
#endif

/* The framePort list repeats Mask and PartialMask, and that duplication is the donor's
   verbatim: where the exe writes a sequence out twice, write it out twice. */


#define kSpriteNotCompiledErr   113

/* SWSetSpriteDrawProc (mac _SWSetSpriteDrawProc). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475970
// MAC: _SWSetSpriteDrawProc i386 0x000ab327 ppc 0x000b61ac
#ifdef MACBUILD
OSErr SWSetSpriteDrawProc(SpritePtr srcSpriteP, DrawProcPtr drawProc)
{
    OSErr err = noErr;
    FramePtr headFrameP = NULL;

    SW_ASSERT(srcSpriteP != NULL && drawProc != NULL, 2067);

    if (drawProc == BlitPixieCompiledSpriteDrawProc)
        drawProc = BlitPixieMaskDrawProc;

    if (drawProc == BlitPixieAllBitRectDrawProc ||
        drawProc == BlitPixieAllBitMaskDrawProc ||
        drawProc == BlitPixieAllBitPartialMaskDrawProc) {
        err = k68kOnlyErr;
    }

    headFrameP = srcSpriteP->frameArray[0];
    if (headFrameP != NULL) {
        if (headFrameP->frameDepth < 8) {
            if ((drawProc == BlitPixieRectDrawProc) ||
                 (drawProc == BlitPixieMaskDrawProc) ||
                 (drawProc == BlitPixiePartialMaskDrawProc) ||
                 (drawProc == BlitPixieRLEDrawProc) ||
                 (drawProc == BlitPixieCompiledSpriteDrawProc)) {
                err = kWrongDepthErr;
            }
        }
    }

    if (headFrameP->framePort == NULL) {
        if ((drawProc == BlitPixieAllBitRectDrawProc) ||
                (drawProc == BlitPixieAllBitMaskDrawProc) ||
                (drawProc == BlitPixieAllBitPartialMaskDrawProc) ||
                (drawProc == BlitPixieRectDrawProc) ||
                (drawProc == BlitPixieMaskDrawProc) ||
                (drawProc == BlitPixiePartialMaskDrawProc) ||
                (drawProc == BlitPixieMaskDrawProc) ||
                (drawProc == BlitPixiePartialMaskDrawProc) ||
                (drawProc == BlitPixieCompiledSpriteDrawProc)
            ) {
            err = kWrongMaskErr;
        }
    }

    if (headFrameP->maskPort == NULL) {
        if ((drawProc == BlitPixieAllBitMaskDrawProc) ||
                (drawProc == BlitPixieAllBitPartialMaskDrawProc) ||
                (drawProc == BlitPixieMaskDrawProc) ||
                (drawProc == BlitPixiePartialMaskDrawProc) ||
                (drawProc == BlitPixieCompiledSpriteDrawProc)
            ) {
            err = kWrongMaskErr;
        }
    }

    if (drawProc == BlitPixieRLEDrawProc && headFrameP->rleDataH == NULL) {
        err = kSpriteNotCompiledErr;
    }

    if (err == noErr)
        srcSpriteP->frameDrawProc = drawProc;

    SWSetStickyIfError(err);
    return err;
}
#else
short SWSetSpriteDrawProc(SpritePtr srcSpriteP, DrawProcPtr drawProc)
{
    short err = 0;
    FramePtr headFrameP = 0;

    if (srcSpriteP == 0 || drawProc == 0)
        SWAssertFail("Sprite.c", 2112);

    headFrameP = srcSpriteP->frameArray[0];
    if (headFrameP != 0) {
        if (headFrameP->frameDepth < 8) {
            if ((drawProc == BlitPixieRectDrawProc) ||
                (drawProc == BlitPixieMaskDrawProc) ||
                (drawProc == BlitPixiePartialMaskDrawProc) ||
                (drawProc == BlitPixieRLEDrawProc)) {
                err = kWrongDepthErr;
            }
        }
    }

    if (headFrameP->framePort == 0) {
        if ((drawProc == BlitPixieRectDrawProc) ||
            (drawProc == BlitPixieMaskDrawProc) ||
            (drawProc == BlitPixiePartialMaskDrawProc) ||
            (drawProc == BlitPixieMaskDrawProc) ||
            (drawProc == BlitPixiePartialMaskDrawProc)) {
            err = kWrongMaskErr;
        }
    }

    if (headFrameP->maskPort == 0) {
        if ((drawProc == BlitPixieMaskDrawProc) ||
            (drawProc == BlitPixiePartialMaskDrawProc)) {
            err = kWrongMaskErr;
        }
    }

    if (drawProc == BlitPixieRLEDrawProc && headFrameP->rleDataH == 0) {
        err = kSpriteNotCompiledErr;
    }

    if ((int)err == noErr)
        srcSpriteP->frameDrawProc = drawProc;

    SWSetStickyIfError(err);
    return err;
}
#endif

#ifdef MACBUILD
/* SWSetSpriteFlippedMode (mac _SWSetSpriteFlippedMode, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteFlippedMode i386 0x000ab520 ppc 0x000b6420
void SWSetSpriteFlippedMode(
    SpritePtr srcSpriteP,
    Boolean flippedHorizontal,
    Boolean flippedVertical)
{
    srcSpriteP->flippedHorizontal = flippedHorizontal;
    srcSpriteP->flippedVertical = flippedVertical;
}

/* SWSetSpriteFrameAdvance (mac _SWSetSpriteFrameAdvance, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteFrameAdvance i386 0x000ab53a ppc 0x000b642c
void SWSetSpriteFrameAdvance(SpritePtr srcSpriteP, short frameAdvance)
{
    SW_ASSERT(srcSpriteP != NULL, 2168);
    srcSpriteP->frameAdvance = frameAdvance;
}

/* SWSetSpriteFrameAdvanceMode (mac _SWSetSpriteFrameAdvanceMode, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteFrameAdvanceMode i386 0x000ab582 ppc 0x000b647c
void SWSetSpriteFrameAdvanceMode(SpritePtr srcSpriteP, AdvanceType advanceMode)
{
    SW_ASSERT(srcSpriteP != NULL, 2181);
    srcSpriteP->frameAdvanceMode = advanceMode;
}
#endif

/* SWSetSpriteFrameRange (mac _SWSetSpriteFrameRange). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475a60
// MAC: _SWSetSpriteFrameRange i386 0x000a91f3 ppc 0x000b4108
void SWSetSpriteFrameRange(SpritePtr srcSpriteP, short firstFrameIndex, short lastFrameIndex)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2253, 2195);

    if (firstFrameIndex < 0)
        firstFrameIndex = 0;

    if (lastFrameIndex > srcSpriteP->maxFrames - 1)
        lastFrameIndex = srcSpriteP->maxFrames - 1;

    srcSpriteP->firstFrameIndex = firstFrameIndex;
    srcSpriteP->lastFrameIndex = lastFrameIndex;

    if (srcSpriteP->curFrameIndex < firstFrameIndex)
        SWSetCurrentFrameIndex(srcSpriteP, firstFrameIndex);
    else if (srcSpriteP->curFrameIndex > lastFrameIndex)
        SWSetCurrentFrameIndex(srcSpriteP, lastFrameIndex);
}

#ifdef MACBUILD
/* SWSetSpriteFrameTime (mac _SWSetSpriteFrameTime, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteFrameTime i386 0x000ab5c3 ppc 0x000b64cc
void SWSetSpriteFrameTime(SpritePtr srcSpriteP, long timeInterval)
{
    SW_ASSERT(srcSpriteP != NULL, 2223);
    srcSpriteP->frameTimeInterval = timeInterval;
}

/* SWSetSpriteFrameProc (mac _SWSetSpriteFrameProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteFrameProc i386 0x000ab600 ppc 0x000b651c
void SWSetSpriteFrameProc(SpritePtr srcSpriteP, FrameProcPtr frameProc)
{
    SW_ASSERT(srcSpriteP != NULL, 2236);
    srcSpriteP->frameChangeProc = frameProc;
}
#endif

/* SWSetSpriteLocation (mac _SWSetSpriteLocation). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475af0
// MAC: _SWSetSpriteLocation i386 0x000ab640 ppc 0x000b656c
void SWSetSpriteLocation(SpritePtr srcSpriteP, short horizLoc, short vertLoc)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2308, 2250);

    horizLoc -= srcSpriteP->curFrameP->hotSpotH;
    vertLoc -= srcSpriteP->curFrameP->hotSpotV;

    srcSpriteP->destFrameRect.bottom = vertLoc + (srcSpriteP->destFrameRect.bottom -
                                                srcSpriteP->destFrameRect.top);
    srcSpriteP->destFrameRect.right = horizLoc + (srcSpriteP->destFrameRect.right -
                                                srcSpriteP->destFrameRect.left);
    srcSpriteP->destFrameRect.top = vertLoc;
    srcSpriteP->destFrameRect.left = horizLoc;

#ifdef MACBUILD
    srcSpriteP->deltaFrameRect = srcSpriteP->destFrameRect;
#else
    srcSpriteP->deltaFrameRect.top = srcSpriteP->destFrameRect.top;
    srcSpriteP->deltaFrameRect.left = srcSpriteP->destFrameRect.left;
    srcSpriteP->deltaFrameRect.bottom = srcSpriteP->destFrameRect.bottom;
    srcSpriteP->deltaFrameRect.right = srcSpriteP->destFrameRect.right;
#endif

    srcSpriteP->needsToBeDrawn = 1;
}

#ifdef MACBUILD
/* SWSetSpriteMoveBounds (mac _SWSetSpriteMoveBounds, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteMoveBounds i386 0x000ab6c6 ppc 0x000b661c
void SWSetSpriteMoveBounds(SpritePtr srcSpriteP, Rect* moveBoundsRect)
{
    SW_ASSERT(srcSpriteP != NULL, 2276);
    srcSpriteP->moveBoundsRect = *moveBoundsRect;
}

/* SWSetSpriteMoveDelta (mac _SWSetSpriteMoveDelta, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteMoveDelta i386 0x000ab71e ppc 0x000b667c
void SWSetSpriteMoveDelta(SpritePtr srcSpriteP, short horizDelta, short vertDelta)
{
    SW_ASSERT(srcSpriteP != NULL, 2290);

    srcSpriteP->horizMoveDelta = horizDelta;
    srcSpriteP->vertMoveDelta = vertDelta;
}

/* SWSetSpriteMoveTime (mac _SWSetSpriteMoveTime, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteMoveTime i386 0x000ab78d ppc 0x000b66d0
void SWSetSpriteMoveTime(SpritePtr srcSpriteP, long timeInterval)
{
    SW_ASSERT(srcSpriteP != NULL, 2306);
    srcSpriteP->moveTimeInterval = timeInterval;
}

/* SWSetSpriteMoveProc (mac _SWSetSpriteMoveProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteMoveProc i386 0x000ab7d0 ppc 0x000b671c
void SWSetSpriteMoveProc(SpritePtr srcSpriteP, MoveProcPtr moveProc)
{
    SW_ASSERT(srcSpriteP != NULL, 2319);

    srcSpriteP->realSpriteMoveProc = moveProc;

    if (srcSpriteP->spriteMoveProc != SWTempSpriteMoveProc)
        srcSpriteP->spriteMoveProc = moveProc;
}
#endif

/* SWSetSpriteVisible (mac _SWSetSpriteVisible). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475b80
// MAC: _SWSetSpriteVisible i386 0x000a9590 ppc 0x000b44b8
void SWSetSpriteVisible(SpritePtr srcSpriteP, unsigned char isVisible)
{
    if (srcSpriteP == 0)
        return;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2398, 2339);

    if (srcSpriteP->isVisible != isVisible) {
        if (isVisible && !srcSpriteP->needsToBeDrawn && !srcSpriteP->needsToBeErased)
            srcSpriteP->justBecameVisible = 1;

        srcSpriteP->isVisible = isVisible;
        srcSpriteP->needsToBeDrawn = 1;
        srcSpriteP->needsToBeErased = !isVisible;
    }
}

#ifdef MACBUILD
/* SWGetSpriteVertLoc (mac _SWGetSpriteVertLoc, mac-only). STATE: see the mac gate. */
// MAC: _SWGetSpriteVertLoc i386 0x000ab81c ppc 0x000b6780
short SWGetSpriteVertLoc(SpritePtr srcSpriteP)
{
    return srcSpriteP->destFrameRect.top + srcSpriteP->curFrameP->hotSpotV;
}

/* SWGetSpriteHorizLoc (mac _SWGetSpriteHorizLoc, mac-only). STATE: see the mac gate. */
// MAC: _SWGetSpriteHorizLoc i386 0x000ab830 ppc 0x000b6798
short SWGetSpriteHorizLoc(SpritePtr srcSpriteP)
{
    return srcSpriteP->destFrameRect.left + srcSpriteP->curFrameP->hotSpotH;
}

/* SWMoveSprite (mac _SWMoveSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWMoveSprite i386 0x000ab844 ppc 0x000b67b0
void SWMoveSprite(SpritePtr srcSpriteP, short horizLoc, short vertLoc)
{
    SW_ASSERT(srcSpriteP != NULL, 2385);

    horizLoc -= srcSpriteP->curFrameP->hotSpotH;
    vertLoc -= srcSpriteP->curFrameP->hotSpotV;

    if ((horizLoc != srcSpriteP->destFrameRect.left) ||
        (vertLoc != srcSpriteP->destFrameRect.top)) {
        srcSpriteP->destFrameRect.bottom = vertLoc + (srcSpriteP->destFrameRect.bottom -
                                                    srcSpriteP->destFrameRect.top);
        srcSpriteP->destFrameRect.right = horizLoc + (srcSpriteP->destFrameRect.right -
                                                    srcSpriteP->destFrameRect.left);
        srcSpriteP->destFrameRect.top = vertLoc;
        srcSpriteP->destFrameRect.left = horizLoc;

        srcSpriteP->needsToBeDrawn = true;
    }
}

/* SWOffsetSprite (mac _SWOffsetSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWOffsetSprite i386 0x000ab8ce ppc 0x000b6868
void SWOffsetSprite(SpritePtr srcSpriteP, short horizOffset, short vertOffset)
{
    SW_ASSERT(srcSpriteP != NULL, 2414);

    if ((horizOffset != 0) || (vertOffset != 0)) {
        srcSpriteP->destFrameRect.right += horizOffset;
        srcSpriteP->destFrameRect.bottom += vertOffset;
        srcSpriteP->destFrameRect.left += horizOffset;
        srcSpriteP->destFrameRect.top += vertOffset;

        srcSpriteP->needsToBeDrawn = true;
    }
}

/* SWBounceSprite (mac _SWBounceSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWBounceSprite i386 0x000ab928 ppc 0x000b68fc
Boolean SWBounceSprite(SpritePtr srcSpriteP)
{
    short horizOffset, vertOffset;

    SW_ASSERT(srcSpriteP != NULL, 2437);

    if (srcSpriteP->destFrameRect.left < srcSpriteP->moveBoundsRect.left) {
        srcSpriteP->horizMoveDelta = -srcSpriteP->horizMoveDelta;
        horizOffset = (srcSpriteP->moveBoundsRect.left - srcSpriteP->destFrameRect.left) * 2;
    } else if (srcSpriteP->destFrameRect.right > srcSpriteP->moveBoundsRect.right) {
        srcSpriteP->horizMoveDelta = -srcSpriteP->horizMoveDelta;
        horizOffset = (srcSpriteP->moveBoundsRect.right - srcSpriteP->destFrameRect.right) * 2;
    } else
        horizOffset = 0;

    if (srcSpriteP->destFrameRect.top < srcSpriteP->moveBoundsRect.top) {
        srcSpriteP->vertMoveDelta = -srcSpriteP->vertMoveDelta;
        vertOffset = (srcSpriteP->moveBoundsRect.top - srcSpriteP->destFrameRect.top) * 2;
    } else if (srcSpriteP->destFrameRect.bottom > srcSpriteP->moveBoundsRect.bottom) {
        srcSpriteP->vertMoveDelta = -srcSpriteP->vertMoveDelta;
        vertOffset = (srcSpriteP->moveBoundsRect.bottom - srcSpriteP->destFrameRect.bottom) * 2;
    } else
        vertOffset = 0;

    if ((horizOffset != 0) || (vertOffset != 0)) {
        SWOffsetSprite(srcSpriteP, horizOffset, vertOffset);
        return true;
    } else {
        return false;
    }
}

/* SWWrapSprite (mac _SWWrapSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWWrapSprite i386 0x000aba0b ppc 0x000b6a20
Boolean SWWrapSprite(SpritePtr srcSpriteP)
{
    short horizOffset, vertOffset;

    SW_ASSERT(srcSpriteP != NULL, 2491);

    if (srcSpriteP->oldFrameRect.left > srcSpriteP->moveBoundsRect.right) {
        horizOffset = - ((srcSpriteP->moveBoundsRect.right -
                        srcSpriteP->moveBoundsRect.left) +
                        (srcSpriteP->destFrameRect.right -
                        srcSpriteP->destFrameRect.left) + srcSpriteP->horizMoveDelta);
    } else if (srcSpriteP->oldFrameRect.right < srcSpriteP->moveBoundsRect.left) {
        horizOffset = (srcSpriteP->moveBoundsRect.right -
                        srcSpriteP->moveBoundsRect.left) +
                        (srcSpriteP->destFrameRect.right -
                        srcSpriteP->destFrameRect.left) - srcSpriteP->horizMoveDelta;
    } else
        horizOffset = 0;

    if (srcSpriteP->oldFrameRect.top > srcSpriteP->moveBoundsRect.bottom) {
        vertOffset = -((srcSpriteP->moveBoundsRect.bottom -
                            srcSpriteP->moveBoundsRect.top) +
                            (srcSpriteP->destFrameRect.bottom -
                            srcSpriteP->destFrameRect.top) + srcSpriteP->vertMoveDelta);
    } else if (srcSpriteP->oldFrameRect.bottom < srcSpriteP->moveBoundsRect.top) {
        vertOffset = (srcSpriteP->moveBoundsRect.bottom -
                            srcSpriteP->moveBoundsRect.top) +
                            (srcSpriteP->destFrameRect.bottom -
                            srcSpriteP->destFrameRect.top) - srcSpriteP->vertMoveDelta;
    } else
        vertOffset = 0;

    if ((horizOffset != 0) || (vertOffset != 0)) {
        SWOffsetSprite(srcSpriteP, horizOffset, vertOffset);

        srcSpriteP->deltaFrameRect = srcSpriteP->destFrameRect;
        srcSpriteP->oldFrameRect = srcSpriteP->destFrameRect;

        return true;
    } else {
        return false;
    }
}

/* SWRegionCollision (mac _SWRegionCollision, mac-only). STATE: see the mac gate. */
// MAC: _SWRegionCollision i386 0x000abb0f ppc 0x000b6b70
Boolean SWRegionCollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    Rect rgnRect;

    SW_ASSERT(srcSpriteP != NULL && dstSpriteP != NULL, 2558);
    SW_ASSERT(srcSpriteP->curFrameP->maskRgn != NULL, 2559);
    SW_ASSERT(dstSpriteP->curFrameP->maskRgn != NULL, 2560);

    CopyRgn(dstSpriteP->curFrameP->maskRgn, gSWCollisionSpareRgn);

    GetRegionBounds(srcSpriteP->curFrameP->maskRgn, &rgnRect);

    OffsetRgn(srcSpriteP->curFrameP->maskRgn,
                (srcSpriteP->destFrameRect.left - rgnRect.left) +
                srcSpriteP->curFrameP->offsetPoint.h,
                (srcSpriteP->destFrameRect.top - rgnRect.top) +
                srcSpriteP->curFrameP->offsetPoint.v);

    GetRegionBounds(gSWCollisionSpareRgn, &rgnRect);

    OffsetRgn(gSWCollisionSpareRgn,
                (dstSpriteP->destFrameRect.left - rgnRect.left) +
                dstSpriteP->curFrameP->offsetPoint.h,
                (dstSpriteP->destFrameRect.top - rgnRect.top) +
                dstSpriteP->curFrameP->offsetPoint.v);

    SectRgn(srcSpriteP->curFrameP->maskRgn, gSWCollisionSpareRgn, gSWCollisionSectRgn);

    return (!EmptyRgn(gSWCollisionSectRgn));
}
#endif

/* `/2' on a signed int is the `cmp 0x80000000 / sbb -1 / sar 1' triple, which is what
   pins the radii SIGNED and not the donor's unsigned-looking widths. */

/* SWRadiusCollision (mac _SWRadiusCollision). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475be0
// MAC: _SWRadiusCollision i386 0x000abc88 ppc 0x000b6ce4
Boolean SWRadiusCollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    Point centerOne, centerTwo;
    register short radiusOne, radiusTwo;
    register short horizDistance, vertDistance;

    if (srcSpriteP == 0 || dstSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2670, 2610);

    radiusOne = (srcSpriteP->destFrameRect.right - srcSpriteP->destFrameRect.left) / 2;
    radiusTwo = (dstSpriteP->destFrameRect.right - dstSpriteP->destFrameRect.left) / 2;

    centerOne.h = srcSpriteP->destFrameRect.left + radiusOne;
    centerOne.v = srcSpriteP->destFrameRect.top + radiusOne;
    centerTwo.h = dstSpriteP->destFrameRect.left + radiusTwo;
    centerTwo.v = dstSpriteP->destFrameRect.top + radiusTwo;

    horizDistance = centerOne.h - centerTwo.h;
    vertDistance = centerOne.v - centerTwo.v;

    return ((horizDistance * horizDistance) + (vertDistance * vertDistance) <
        ((radiusOne + radiusTwo) * (radiusOne + radiusTwo)));
}

#ifdef MACBUILD
/* SWPixelCollision (mac _SWPixelCollision, mac-only). STATE: see the mac gate. */
// MAC: _SWPixelCollision i386 0x000abd27 ppc 0x000b6da4
Boolean SWPixelCollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    Boolean collision;
    Rect sectRect;
    Rect srcSectRect;
    Rect dstSectRect;
    FramePtr srcFrameP,
                    dstFrameP;

    SW_ASSERT(srcSpriteP != NULL, 2645);
    SW_ASSERT(dstSpriteP != NULL, 2646);
    SW_ASSERT(srcSpriteP->curFrameP->maskPort != NULL, 2647);
    SW_ASSERT(dstSpriteP->curFrameP->maskPort != NULL, 2648);
    SW_ASSERT(srcSpriteP->curFrameP->isFrameLocked, 2649);
    SW_ASSERT(dstSpriteP->curFrameP->isFrameLocked, 2650);
    SW_ASSERT(dstSpriteP->curFrameP->frameDepth >= 8, 2651);
    SW_ASSERT(srcSpriteP->curFrameP->frameDepth == dstSpriteP->curFrameP->frameDepth, 2652);

    sectRect.left = SW_MAX(srcSpriteP->destFrameRect.left, dstSpriteP->destFrameRect.left);
    sectRect.top = SW_MAX(srcSpriteP->destFrameRect.top, dstSpriteP->destFrameRect.top);
    sectRect.right = SW_MIN(srcSpriteP->destFrameRect.right, dstSpriteP->destFrameRect.right);
    sectRect.bottom = SW_MIN(srcSpriteP->destFrameRect.bottom, dstSpriteP->destFrameRect.bottom);

    if (sectRect.right > sectRect.left && sectRect.bottom > sectRect.top) {
        srcFrameP = srcSpriteP->curFrameP;
        dstFrameP = dstSpriteP->curFrameP;

        srcSectRect.left = srcFrameP->frameRect.left + (sectRect.left-srcSpriteP->destFrameRect.left);
        srcSectRect.top = srcFrameP->frameRect.top + (sectRect.top-srcSpriteP->destFrameRect.top);
        srcSectRect.right = srcFrameP->frameRect.left + (sectRect.right-srcSpriteP->destFrameRect.left);
        srcSectRect.bottom = srcFrameP->frameRect.top +(sectRect.bottom-srcSpriteP->destFrameRect.top);

        dstSectRect.left = dstFrameP->frameRect.left + (sectRect.left-dstSpriteP->destFrameRect.left);
        dstSectRect.top = dstFrameP->frameRect.top +(sectRect.top-dstSpriteP->destFrameRect.top);
        dstSectRect.right = dstFrameP->frameRect.left + (sectRect.right-dstSpriteP->destFrameRect.left);
        dstSectRect.bottom = dstFrameP->frameRect.top +(sectRect.bottom-dstSpriteP->destFrameRect.top);

        collision = BlitPixieMaskCollisionProc(srcSpriteP, dstSpriteP, &srcSectRect, &dstSectRect);
    } else
        collision = false;

    return collision;
}
#endif

/* sectRect never reaches the stack -- mwcc keeps all four members in registers because
   nothing takes its address; only srcSectRect and dstSectRect, which are passed by
   pointer, get frame slots. */



/* SWRLECollision (mac _SWRLECollision). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475c80
// MAC: _SWRLECollision i386 0x000abfbd ppc 0x000b6ff8
Boolean SWRLECollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP)
{
    Boolean collision;
    Rect sectRect;
    Rect srcSectRect;
    Rect dstSectRect;
    FramePtr srcFrameP, dstFrameP;

    if (srcSpriteP == 0 || dstSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2761, 2701);
    if (srcSpriteP->curFrameP->rleDataH == 0)
        SW_ASSERT_FAIL("Sprite.c", 2762, 2702);
    if (dstSpriteP->curFrameP->rleDataH == 0)
        SW_ASSERT_FAIL("Sprite.c", 2763, 2703);
    if (!srcSpriteP->curFrameP->isFrameLocked)
        SW_ASSERT_FAIL("Sprite.c", 2764, 2704);
    if (!dstSpriteP->curFrameP->isFrameLocked)
        SW_ASSERT_FAIL("Sprite.c", 2765, 2705);

    sectRect.left = SW_MAX(srcSpriteP->destFrameRect.left, dstSpriteP->destFrameRect.left);
    sectRect.top = SW_MAX(srcSpriteP->destFrameRect.top, dstSpriteP->destFrameRect.top);
    sectRect.right = SW_MIN(srcSpriteP->destFrameRect.right, dstSpriteP->destFrameRect.right);
    sectRect.bottom = SW_MIN(srcSpriteP->destFrameRect.bottom, dstSpriteP->destFrameRect.bottom);

    if (sectRect.right > sectRect.left && sectRect.bottom > sectRect.top) {
        srcFrameP = srcSpriteP->curFrameP;
        dstFrameP = dstSpriteP->curFrameP;

        srcSectRect.left = (sectRect.left - srcSpriteP->destFrameRect.left);
        srcSectRect.top = (sectRect.top - srcSpriteP->destFrameRect.top);
        srcSectRect.right = (sectRect.right - srcSpriteP->destFrameRect.left);
        srcSectRect.bottom = (sectRect.bottom - srcSpriteP->destFrameRect.top);

        dstSectRect.left = (sectRect.left - dstSpriteP->destFrameRect.left);
        dstSectRect.top = (sectRect.top - dstSpriteP->destFrameRect.top);
        dstSectRect.right = (sectRect.right - dstSpriteP->destFrameRect.left);
        dstSectRect.bottom = (sectRect.bottom - dstSpriteP->destFrameRect.top);

        collision = BlitPixieRLECollisionProc(srcSpriteP, dstSpriteP, &srcSectRect, &dstSectRect);
    } else
        collision = 0;

    return collision;
}

/* SWRLECollisionWithPoint (mac _SWRLECollisionWithPoint). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475e20
// MAC: _SWRLECollisionWithPoint i386 0x000ac1ab ppc 0x000b7198
Boolean SWRLECollisionWithPoint(SpritePtr srcSpriteP, Point *testPoint)
{
    Boolean collision;

    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2810, 2750);
    if (srcSpriteP->curFrameP->rleDataH == 0)
        SW_ASSERT_FAIL("Sprite.c", 2811, 2751);
    if (!srcSpriteP->curFrameP->isFrameLocked)
        SW_ASSERT_FAIL("Sprite.c", 2812, 2752);

    if (testPoint->h >= srcSpriteP->destFrameRect.left &&
        testPoint->h <= srcSpriteP->destFrameRect.right &&
        testPoint->v >= srcSpriteP->destFrameRect.top &&
        testPoint->v <= srcSpriteP->destFrameRect.bottom)
    {
        collision = BlitPixieRLEPointCollisionProc(srcSpriteP, testPoint,
                                                   &srcSpriteP->destFrameRect);
    } else
        collision = 0;

    return collision;
}

/* SW_COLLISION_RECT_* is destFrameRect adjusted by the current frame's collisionInset:
   sprite +0x1a/+0x1c/+0x1e/+0x20 against frame +0x2e/+0x30/+0x32/+0x34, which is how the
   inset's four members are pinned. */

/* SWIsSpriteInRect (mac _SWIsSpriteInRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00475ec0
// MAC: _SWIsSpriteInRect i386 0x000ac282 ppc 0x000b7288
Boolean SWIsSpriteInRect(SpritePtr srcSpriteP, Rect *testRect)
{
    if (srcSpriteP == 0)
        SW_ASSERT_FAIL("Sprite.c", 2836, 2776);

    return ((srcSpriteP->destFrameRect.top + srcSpriteP->curFrameP->collisionInset.top)
                < testRect->bottom) &&
           ((srcSpriteP->destFrameRect.bottom - srcSpriteP->curFrameP->collisionInset.bottom)
                > testRect->top) &&
           ((srcSpriteP->destFrameRect.left + srcSpriteP->curFrameP->collisionInset.left)
                < testRect->right) &&
           ((srcSpriteP->destFrameRect.right - srcSpriteP->curFrameP->collisionInset.right)
                > testRect->left);
}

#ifdef MACBUILD
/* SWIsSpriteFullyInRect (mac _SWIsSpriteFullyInRect, mac-only). STATE: see the mac gate. */
// MAC: _SWIsSpriteFullyInRect i386 0x000ac312 ppc 0x000b7348
Boolean SWIsSpriteFullyInRect(SpritePtr srcSpriteP, Rect* testRect)
{
    SW_ASSERT(srcSpriteP != NULL, 2793);

    return ((SW_COLLISION_RECT_TOP(srcSpriteP) >= testRect->top) &&
            (SW_COLLISION_RECT_BOTTOM(srcSpriteP) <= testRect->bottom) &&
            (SW_COLLISION_RECT_LEFT(srcSpriteP) >= testRect->left) &&
            (SW_COLLISION_RECT_RIGHT(srcSpriteP) <= testRect->right));
}

/* SWIsPointInSprite (mac _SWIsPointInSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWIsPointInSprite i386 0x000ac3a2 ppc 0x000b7408
Boolean SWIsPointInSprite(SpritePtr srcSpriteP, Point testPoint)
{
    SW_ASSERT(srcSpriteP != NULL, 2810);

    return (testPoint.h >= SW_COLLISION_RECT_LEFT(srcSpriteP)) &&
            (testPoint.h < SW_COLLISION_RECT_RIGHT(srcSpriteP)) &&
            (testPoint.v >= SW_COLLISION_RECT_TOP(srcSpriteP)) &&
            (testPoint.v < SW_COLLISION_RECT_BOTTOM(srcSpriteP));
}
#endif
