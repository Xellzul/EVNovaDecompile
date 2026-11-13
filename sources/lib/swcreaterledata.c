// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* swcreaterledata.c -- SWCreateRLEData (win 0x004760c0, SpriteCompiler.c lines 241-242, donor SpriteCompiler.c line 150). Notes: Research/notes/swcreaterledata.md */
#include "macport.h"
#include "spriteworld.h"
#include "burger.h"




/* 0x00472510 -- the donor's seven-argument BlitPixieRLEParsePixels. */

#ifdef MACBUILD
/* SWCompileSprite (mac _SWCompileSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWCompileSprite i386 0x000aec7b ppc 0x000ba1ec
short SWCompileSprite(SpritePtr srcSpriteP)
{
    return noErr;
}

/* SWCompileFrame (mac _SWCompileFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWCompileFrame i386 0x000aec82 ppc 0x000ba1f4
short SWCompileFrame(FramePtr srcFrameP)
{
    return noErr;
}
#endif

/* SWCreateRLEData (mac _SWCreateRLEData). STATE: MATCH 100.0%, exe 242 / obj 242 instrs.
   OPEN: none */
// FUNCTION: EVNOVA 0x004760c0
// MAC: _SWCreateRLEData i386 0x000aec89 ppc 0x000ba1fc
#ifdef MACBUILD
short SWCreateRLEData(SpritePtr srcSpriteP, short requestedDepth)
{
    short err = noErr;
    short frame;
    FramePtr srcFrameP;
    RLEPixelDataHdl spriteData, oldData;
    void **frameData;
    unsigned short depth;
    short maxWidth, maxHeight;
    Rect *rectP;
    Boolean lockedState;
    uint32_t srcBaseOffset;

    if (requestedDepth > 0) {
        depth = requestedDepth;
    } else {
        depth = srcSpriteP->frameArray[0]->frameDepth;
        if (depth != 8 && depth != 16 && depth != 32)
            err = kWrongDepthErr;
    }

    oldData = (RLEPixelDataHdl)srcSpriteP->frameArray[0]->rleDataH;

    maxWidth = 0;
    maxHeight = 0;

    for (frame = 0; (err == noErr) && (frame < srcSpriteP->numFrames); frame++) {
        srcFrameP = srcSpriteP->frameArray[frame];

        rectP = &srcFrameP->frameRect;

        if (rectP->right - rectP->left > maxWidth) {
            maxWidth = rectP->right - rectP->left;
        }
        if (rectP->bottom - rectP->top > maxHeight) {
            maxHeight = rectP->bottom - rectP->top;
        }
    }

    spriteData = (RLEPixelDataHdl)NewHandle(kRLEResourceHeaderSize);
    if (err == noErr && spriteData != NULL) {
        (**spriteData).width = maxWidth;
        (**spriteData).height = maxHeight;
        (**spriteData).depth = depth;
        (**spriteData).palette = 0;
        (**spriteData).frames = srcSpriteP->numFrames;
        (**spriteData).reserved1 = 0;
        (**spriteData).reserved2 = 0;
        (**spriteData).reserved3 = 0;

        for (frame = 0; (err == noErr) && (frame < srcSpriteP->numFrames); frame++) {
            srcFrameP = srcSpriteP->frameArray[frame];
            rectP = &srcFrameP->frameRect;

            if (srcFrameP->maskPort == NULL) {
                err = kWrongMaskErr;
                break;
            }

            lockedState = srcFrameP->isFrameLocked;
            if (lockedState == false)
                SWLockFrame(srcFrameP);

            srcBaseOffset =
                    SW_ROWBYTES(srcFrameP, rectP->top) +
                    SW_PIXELBYTES(srcFrameP, rectP->left);

            if (srcFrameP->frameBaseAddr == NULL)
                SW_ASSERT_FAIL("SpriteCompiler.c", 241, 239);
            if (srcFrameP->maskBaseAddr == NULL)
                SW_ASSERT_FAIL("SpriteCompiler.c", 242, 240);

            err = BlitPixieRLEParsePixels(
                    (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
                    (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
                    srcFrameP->frameRowBytes,
                    SW_PIXELBYTES(srcFrameP, rectP->right - rectP->left),
                    (rectP->bottom - rectP->top),
                    depth,
                    &frameData);

            if (err == noErr && frameData != NULL) {
                srcFrameP->rleDataH = spriteData;
                srcFrameP->rleTokenStart = *spriteData + GetHandleSize((Handle)spriteData);

                HLock((Handle)frameData);
                err = HandAndHand((Handle)frameData, (Handle)spriteData);

                DisposeHandle((Handle)frameData);
            }

            if (lockedState == false)
                SWUnlockFrame(srcFrameP);
        }

        if (err != noErr)
            DisposeHandle((Handle)spriteData);
        else {
            if (oldData != NULL)
                DisposeHandle((Handle)oldData);

            srcSpriteP->rleDataH = (char **)spriteData;
            srcSpriteP->ownsRLEData = true;
        }
    } else {
        if (err == noErr)
            err = MemError();
    }

    SWSetStickyIfError(err);
    return err;
}
#else
short SWCreateRLEData(SpritePtr srcSpriteP, short depth)
{
    short frame;
    FramePtr srcFrameP;
    RLEPixelDataHdl spriteData;
    RLEPixelDataHdl oldData;
    void **frameData;
    short maxWidth;
    short maxHeight;
    short err;
    Rect *rectP;
    unsigned char lockedState;
    uint32_t srcBaseOffset;
    uint32_t spriteDataSize;
    uint32_t frameDataSize;

    err = 0;

    if (depth <= 0) {
        depth = srcSpriteP->frameArray[0]->frameDepth;
        if (depth != 8 && depth != 16 && depth != 32)
            err = kWrongDepthErr;
    }

    oldData = (RLEPixelDataHdl)srcSpriteP->frameArray[0]->rleDataH;

    maxWidth = 0;
    maxHeight = 0;

    frame = 0;
    while (((int)err == noErr) && (frame < srcSpriteP->numFrames)) {
        srcFrameP = srcSpriteP->frameArray[frame];

        rectP = &srcFrameP->frameRect;

        if (rectP->right - rectP->left > maxWidth) {
            maxWidth = rectP->right - rectP->left;
        }
        if (rectP->bottom - rectP->top > maxHeight) {
            maxHeight = rectP->bottom - rectP->top;
        }
        frame++;
    }

    spriteData = (RLEPixelDataHdl)AllocAHandle2(kRLEResourceHeaderSize, 0);
    if ((int)err == noErr && spriteData != 0) {
        (*spriteData)->width = maxWidth;
        (*spriteData)->height = maxHeight;
        (*spriteData)->depth = depth;
        (*spriteData)->palette = 0;
        (*spriteData)->frames = srcSpriteP->numFrames;
        (*spriteData)->reserved1 = 0;
        (*spriteData)->reserved2 = 0;
        (*spriteData)->reserved3 = 0;

        for (frame = 0; ((int)err == noErr) && (frame < srcSpriteP->numFrames); frame++) {
            srcFrameP = srcSpriteP->frameArray[frame];
            rectP = &srcFrameP->frameRect;

            if (srcFrameP->maskPort == 0) {
                err = kWrongMaskErr;
                break;
            }

            lockedState = srcFrameP->isFrameLocked;
            if (lockedState == 0)
                SWLockFrame(srcFrameP);

            srcBaseOffset =
                    SW_ROWBYTES(srcFrameP, rectP->top) +
                    SW_PIXELBYTES(srcFrameP, rectP->left);

            if (srcFrameP->frameBaseAddr == 0)
                SW_ASSERT_FAIL("SpriteCompiler.c", 241, 239);
            if (srcFrameP->maskBaseAddr == 0)
                SW_ASSERT_FAIL("SpriteCompiler.c", 242, 240);

            err = BlitPixieRLEParsePixels(
                    (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
                    (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
                    srcFrameP->frameRowBytes,
                    SW_PIXELBYTES(srcFrameP, rectP->right - rectP->left),
                    (rectP->bottom - rectP->top),
                    depth,
                    &frameData);

            if ((int)err == noErr && frameData != 0) {
                srcFrameP->rleDataH = spriteData;
                srcFrameP->rleTokenStart =
                        *spriteData + GetAHandleSize((Handle)spriteData);

#ifdef MACBUILD
                HLock(frameData);
                err = HandAndHand(frameData, spriteData);
#else
                LockAHandle((Handle)frameData);
                frameDataSize = GetAHandleSize((Handle)frameData);
                spriteDataSize = GetAHandleSize((Handle)spriteData);
                ResizeAHandle((Handle)spriteData, frameDataSize + spriteDataSize);
                memmove((char *)*spriteData + spriteDataSize, *frameData,
                        frameDataSize);
#endif

                DeallocAHandle(frameData);
            }

            if (lockedState == 0)
                SWUnlockFrame(srcFrameP);
        }

        if ((int)err != noErr)
            DeallocAHandle(spriteData);
        else {
            if (oldData != 0)
                DeallocAHandle(oldData);

            srcSpriteP->rleDataH = (char **)spriteData;
            srcSpriteP->ownsRLEData = 1;
        }
    } else {
        if ((int)err == noErr)
            err = MemError();
    }

    SWSetStickyIfError(err);
    return err;
}
#endif
