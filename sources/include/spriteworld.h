/* spriteworld.h -- the SpriteWorld fork, defined in sources/lib/. Notes:
   Research/declaration-mismatches.md */
#ifndef SPRITEWORLD_H
#define SPRITEWORLD_H
#include "gametypes.h"
#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kSWDontRemoveSprite     0
#define kSWWrapAroundMode       0
#define kPixelMask              1
#define kCreateGWorld           1
#define kWrongDepthErr          105
#define kWrongMaskErr           106


/* --- sprite world --- */
extern short SWCreateSpriteWorldFromWindow(SpriteWorldPtr *spriteWorldP,
        CWindowPtr srcWindowP, Rect *worldRectP, Rect *offscreenRectP,
        short maxDepth);                                 /* 0x00477910 */
extern void SWAnimateSpriteWorld(SpriteWorldRec *spriteWorldP);  /* 0x00478480 */
extern void SWProcessSpriteWorld(SpriteWorldRec *spriteWorldP);  /* 0x004781f0 */
extern void SWUpdateSpriteWorld(SpriteWorldRec *spriteWorldP,
                                char updateWindow);      /* 0x00477fa0 */
extern void SWLockSpriteWorld(SpriteWorldPtr spriteWorld);   /* 0x00477ba0 */
extern void SWSetPortToBackground(SpriteWorldPtr spriteWorld); /* 0x00477c10 */
extern char SWIsWindowInForeground(SpriteWorldPtr spriteWorldP); /* 0x00478e20 */

/* --- layers --- */
extern short SWCreateSpriteLayer(SpriteLayerRec **spriteLayerP);  /* 0x004770d0 */
extern void SWDisposeSpriteLayer(SpriteLayerRec **spriteLayerPP); /* 0x00477120 */
extern void SWLockSpriteLayer(SpriteLayerRec *spriteLayerP);   /* 0x00477250 */
extern void SWUnlockSpriteLayer(SpriteLayerRec *spriteLayerP); /* 0x00477290 */
extern void SWAddSpriteLayer(SpriteWorldRec *spriteWorldP,
                          SpriteLayerRec *newSpriteLayerP);              /* 0x00477b00 */
extern void SWRemoveSpriteLayer(SpriteWorldRec *spriteWorldP,
                          SpriteLayerRec *oldSpriteLayerP);              /* 0x00477b50 */
extern void SWCollideSpriteLayer(SpriteWorldRec *spriteWorldP, SpriteLayerRec *srcSpriteLayerP,
                          SpriteLayerRec *dstSpriteLayerP);              /* 0x004772d0 */
extern void SWSetLayerCallBack(SpriteLayerRec *spriteLayerP,
        WorldProcPtr callBack);                          /* 0x00477240 */

/* --- frames --- */
extern short SWCreateFrame(void *theGDH, SWFrameRec **newFrameP, Rect *frameRect,
        short depth, unsigned char createGWorld);        /* 0x00476400 */
extern char SWDisposeFrame(SWFrameRec **oldFramePP);     /* 0x00476bd0 */
extern void SWDisposeWindowFrame(SWFrameRec **oldFramePP); /* 0x00476c70 */
extern void SWLockWindowFrame(SWFrameRec *windowFrameP); /* 0x00476f80 */
extern void SWSetPortToFrame(SWFrameRec *dstFrameP);     /* 0x00476cb0 */
extern void SWInitializeFrame(SWFrameRec *tempFrameP);   /* 0x00476d10 */
extern void SWLockFrame(SWFrameRec *srcFrameP);          /* 0x00476e10 */
extern void SWUnlockFrame(SWFrameRec *srcFrameP);        /* 0x00476f10 */
extern void SWInvertGWorld(CGrafPtr gWorldP);            /* 0x004791b0 */
extern void SWInvertRect(CGrafPtr gWorldP, Rect *rectP); /* 0x004791d0 */

/* --- sprites --- */
extern short SWCloneSprite(SpritePtr cloneSpriteP, SpritePtr *newSpriteP,
                           void *spriteStorageP);        /* 0x00475050 */
extern short SWCreateEmptySprite(SpritePtr *newSpriteP,
                                 void *spriteStorageP);  /* 0x00474720 */
extern short SWCreateSpriteFromCicnResource(SpriteWorldPtr destSpriteWorldP, SpritePtr *slot,
                                 void *spriteStorageP, short cIconID, short maxFrames,
                                 unsigned char maskType);              /* 0x00474970 */
extern short SWRemoveSprite(SpriteRec *oldSpriteP);      /* 0x004771c0 */
extern void SWUnlockSprite(SpritePtr srcSpriteP);        /* 0x004756c0 */
extern void SWDisposeSprite(SpritePtr *deadSpritePP);    /* 0x00475470 */
extern short SWCreateSprite(SpritePtr *newSpriteP, void *spriteStorageP,
                            short maxFrames);            /* 0x004744a0 */
extern short SWAddFrame(SpritePtr srcSpriteP, FramePtr newFrameP); /* 0x00475740 */
extern short SWAddSprite(SpriteLayerRec *spriteLayerP, SpriteRec *newSpriteP); /* 0x00477140 */
extern short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
        SWFrameRec **newFrameP, short iconResID,
        unsigned char maskType);                         /* 0x00476800 */
extern short SWCreateRLEData(SpritePtr srcSpriteP, short depth);   /* 0x004760c0 */
extern void SWSetBackgroundColor(SpriteWorldPtr spriteWorldP,
                          RGBColor *color);                      /* 0x00477d60 */
extern void SWSetSpriteCollideProc(SpritePtr srcSpriteP,
                          CollideProcPtr collideProc);             /* 0x00475940 */
extern short SWCreateSpriteFromRLEResource(SpritePtr *srcSpriteP, void *spriteStorageP,
                          short rleResID, short maxFrames, short mask);  /* 0x00474e90 */
extern short SWCreateSpriteFromSinglePictXY(SpriteWorldRec *destSpriteWorldP,
                          SpritePtr *newSpriteP, void *spriteStorageP, short pictResID,
                          short maskResID, short frameWidth, short frameHeight,
                          short borderWidth, short borderHeight, short maxFrames,
                          unsigned char maskType);
extern void SWWindowMoved(SpriteWorldPtr spriteWorldP);            /* 0x00477f10 */
extern char sw_depth_ok(SpriteWorldPtr world);                    /* 0x00478f10 */
extern short SWCloneFramesFromMaster(SpritePtr dstSpriteP, SpritePtr masterSpriteP); /* 0x00475f70 */
extern void SWSetSpriteFrameRange(SpritePtr srcSpriteP, short firstFrameIndex,
                                  short lastFrameIndex); /* 0x00475a60 */
extern void SWTempSpriteMoveProc(SpritePtr srcSpriteP);  /* 0x00475800 */
extern short SWSetSpriteDrawProc(SpritePtr srcSpriteP,
                                 DrawProcPtr drawProc);  /* 0x00475970 */
extern void SWLockSprite(SpritePtr srcSpriteP);          /* 0x00475550 */
extern char SWRLECollision(SpritePtr srcSpriteP,
                           SpritePtr dstSpriteP);        /* 0x00475c80 */
extern char SWRLECollisionWithPoint(SpritePtr srcSpriteP,
                                    Point *testPoint);   /* 0x00475e20 */
extern char SWRadiusCollision(SpritePtr srcSpriteP,
                              SpritePtr dstSpriteP);     /* 0x00475be0 */
extern char SWIsSpriteInRect(SpritePtr srcSpriteP,
                             Rect *testRect);            /* 0x00475ec0 */
extern void SWStdSpriteDrawProc(FramePtr srcFrameP, FramePtr dstFrameP,
                                Rect *srcRect, Rect *dstRect);  /* 0x004757a0 */

/* --- draw procs --- */
extern void BlitPixieRLEDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00470ee0 */
extern void BlitPixieRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x004703e0 */
extern void BlitPixieRLETranslucentDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x004711e0 */
extern void BlitPixieFloatingRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect, short level,
                                 long backgroundValue);                /* 0x00470680 */
extern void BlitPixieMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00470960 */
extern void BlitPixiePartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00470c20 */
extern char BlitPixieRLECollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00471800 */
extern char BlitPixieRLEPointCollisionProc(SpritePtr srcSpriteP, Point *testPoint,
                                 Rect *destRect);                      /* 0x00471920 */
extern void BlitCopyLine(FramePtr srcFrameP, FramePtr dstFrameP,
                                 Point srcPoint, Point dstPoint);      /* 0x00479cf0 */
extern void CopyLine(FramePtr srcFrameP, FramePtr dstFrameP,
                                 Point srcPoint, Point dstPoint);      /* 0x00479c50 */
extern void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                 unsigned long color, long srcShade, long dstShade,
                                 unsigned long srcRamp, unsigned long dstRamp);
                                                                       /* 0x004797b0 */
extern void DrawBeam(FramePtr dstFrameP, Point srcPoint, Point dstPoint, unsigned long color,
                     short halfWidth, unsigned long coronaColor, short fadeStep); /* 0x00479fe0 */
extern void DrawLightningBeam(FramePtr frameA, FramePtr frameB, Point srcPoint, Point dstPoint,
                              unsigned long color, short width, short segPercent, short jitter,
                              short shade);                    /* 0x0047a410 */
extern void EraseBeam(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                      short halfWidth, short fadeStep);        /* 0x0047ac50 */
extern void EraseLightningBeam(FramePtr srcFrameP, FramePtr dstFrameP,
                               Point srcPoint, Point dstPoint,
                               short halfWidth, short segPercent, short jitter); /* 0x0047afd0 */
extern void ComputePixelValues(short pixelSize);                       /* 0x00479210 */
extern short GetGDeviceDepth(void *theGDH);                            /* 0x0047b7c0 */
extern void MoveParticles(void);                                       /* 0x0047c800 */
extern SpriteRec *gSWCurrentSpriteBeingDrawn;            /* 0x00870da8 */
extern SpriteWorldRec *gSWCurrentSpriteWorld;            /* 0x008706f0 */
#pragma cplusplus on
extern "C++" {
extern unsigned short SWMaskedRGBValue;                  /* 0x007d0500 */
extern short gSWStickyError;                             /* 0x007d0504 */
}
#pragma cplusplus reset

/* --- the rect blitters: whole-rect copies, src first --- */
extern void BlitPixieRect(unsigned char *srcPixels, unsigned char *dstPixels,
        long srcRowBytes, long dstRowBytes, unsigned short byteWidth,
        unsigned short numRows);                         /* 0x00471cf0 */
extern void BlitPixieFloatingRect(unsigned char *srcPixels, unsigned char *dstPixels,
        long srcRowBytes, long dstRowBytes, unsigned short byteWidth,
        unsigned short numRows, unsigned short level);   /* 0x00471d30 */
extern void BlitPixieMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short byteWidth, unsigned short numRows);      /* 0x00471a70 */
extern void BlitPixiePartialMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short byteWidth, unsigned short numRows);      /* 0x00471b70 */
extern char BlitPixieRLECollision(unsigned char *srcTokens, unsigned char *dstTokens,
        Rect *srcRect, Rect *dstRect);                   /* 0x00472190 */
extern char BlitPixieRLECollisionWithPoint(unsigned char *rleData, Point *pt,
        Rect *r);                                        /* 0x00472420 */
extern short BlitPixieRLEParsePixels(unsigned char *srcPixels, unsigned char *maskPixels,
        unsigned long rowBytes, unsigned short byteWidth, unsigned short numRows,
        unsigned short depth, void ***frameData);        /* 0x00472510 */

/* --- the RLE blitters: token stream in, one sprite each --- */
extern void BlitPixieRLETranslucent(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes);           /* 0x00472900 */
extern void BlitPixieRLEAddOver(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes);           /* 0x00472b30 */
extern void BlitPixieRLETranslucent8(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes);           /* 0x00472bb0 */
extern void BlitPixieRLEAddOver8(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes);           /* 0x00472cf0 */
extern void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes, Rect *srcBlitRect);    /* 0x00472d70 */
extern void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes, Rect *srcBlitRect);    /* 0x004730d0 */
extern void BlitPixieRLETranslucentClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes, Rect *srcBlitRect);    /* 0x00473270 */
extern void BlitPixieRLEAddOverClipped8(unsigned char *rleTokens, unsigned char *dstPixel,
                            long dstRowBytes, Rect *srcBlitRect);    /* 0x00473500 */

/* --- the pixie blitters: one scanline each, dst first --- */
extern void BlitPixieTranslucentClearCopy8(unsigned char *dstPixels,
        unsigned char *srcPixels, long numPixels,
        unsigned char clearValue);                       /* 0x00473dc0 */
extern void BlitPixieFloatingRect8(unsigned char *srcPixels,
        unsigned char *dstPixels, long srcRowBytes, long dstRowBytes,
        unsigned short byteWidth, unsigned short numRows,
        unsigned short backgroundValue);                 /* 0x00471dc0 */
extern void BlitPixieMemCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                             long byteWidth);            /* 0x00474350 */
extern void BlitPixieMemSet(unsigned char *dstPixels, unsigned long value,
                            long byteWidth);             /* 0x00474410 */
extern void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                                 long byteWidth);        /* 0x00473b60 */
extern void BlitPixieAddOverCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                                  long count);           /* 0x00473fa0 */
extern void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                                     long byteWidth);    /* 0x004736c0 */
extern void BlitPixieTranslucentCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                                      long count);       /* 0x00473c20 */
extern void BlitPixieTranslucentBiasCopy8(unsigned char *dstPixels, unsigned char *srcPixels,
                                          long count);   /* 0x00473e00 */
extern void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel,
                         long dstRowBytes);              /* 0x00471e10 */
extern void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel,
                                long dstRowBytes, Rect *srcBlitRect); /* 0x00471e90 */
extern unsigned long ClampAlphaLevel(unsigned long level);  /* 0x004736a0 */
extern long GetMainDevice(void);                         /* 0x004f2a50 */

/* --- the blitters' shared state --- */
#pragma cplusplus on
extern "C++" {
extern unsigned long alpha;                              /* 0x007ceac8 */
extern unsigned long alphaR;                             /* 0x007ceacc */
extern unsigned long alphaG;                             /* 0x007cead0 */
extern unsigned long alphaB;                             /* 0x007cead4 */
extern unsigned long overR;                              /* 0x007cead8 */
extern unsigned long overG;                              /* 0x007ceadc */
extern unsigned long overB;                              /* 0x007ceae0 */
extern unsigned long overBias;                           /* 0x007ceae4 */
extern short scaleR;                                     /* 0x007ceae8 */
extern short scaleG;                                     /* 0x007ceaea */
extern short scaleB;                                     /* 0x007ceaec */
extern unsigned long biasColor;                          /* 0x007ceaf0 */
extern unsigned char *alphaTable[5][256];                /* 0x007ceaf4 */
}
#pragma cplusplus reset

/* --- translucent blends (mac _Trans8Bit/_Trans16Bit, inlined in the win build) --- */
static inline unsigned char Trans8Bit(unsigned char src, unsigned char dst, unsigned long alpha)
{
    /* BUG: any alpha above 3 skips the blend, so (31 - alpha) >> 3 only ever selects row 3 */
    if (alpha > 3)
        return src;
    return alphaTable[(31 - alpha) >> 3][(char)src][(char)dst];
}

static inline unsigned short Trans16Bit(unsigned long src, unsigned long dst, unsigned long alpha)
{
    unsigned long srcSplit, dstSplit, blended;

    srcSplit = SW_SPLIT15(src);
    dstSplit = SW_SPLIT15(dst);
    blended = srcSplit * alpha + dstSplit * (32 - alpha);
    return ((blended >> 5) & 0x7c1f) | ((blended >> 20) & 0x3e0);
}

#pragma cplusplus on
extern "C++" {
extern unsigned short SWTransparentRGBValue;             /* 0x007d0502 */
}
#pragma cplusplus reset

/* --- lines --- */
extern void DrawTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                unsigned long color, short srcShade, short dstShade,
                                short srcRamp, short dstRamp);        /* 0x00479590 */
extern char SWClipLineWithRect(Point *srcPoint, Point *dstPoint,
                               Rect *clipRectP);         /* 0x0047b3c0 */

/* --- errors --- */
extern void SWAssertFail(char *filename, int lineNum);   /* 0x004794d0 */
extern void SWSetStickyIfError(short errNum);            /* 0x004794b0 */

/* --- particles --- */
extern void EraseParticlesOffscreen(SpriteWorldPtr spriteWorldP); /* 0x0047bb30 */
extern void UpdateParticlesInWindow(SpriteWorldRec *spriteWorldP); /* 0x0047c3a0 */
extern int SWEnterSpriteWorld(void);                           /* 0x00477630 */
extern void SetParticleScrollOffset(int h, int v);                  /* 0x0047b940 */

extern void ShipCollisionHandler(SpriteRec *shipSpr, SpriteRec *otherSpr,
                                 Rect *sectRect);        /* 0x004374f0 */
extern void AsteroidCollisionHandler(SpriteRec *roidSpr, SpriteRec *otherSpr,
                                     Rect *sectRect);    /* 0x00436f70 */

extern unsigned long SWGetValueFromColor(SWFrameRec *dstFrameP, RGBColor *color);
                                                         /* 0x004719d0 */
extern short SWCreateWindowFrame(CWindowPtr srcWindowP, SWFrameRec **newFrameP,
                                 Rect *frameRect, short maxHeight); /* 0x004765d0 */
extern short SWCreateSpriteWorld(void *mainGDH, SpriteWorldRec **spriteWorldP,
                                 SWFrameRec *windowFrameP, SWFrameRec *backFrameP,
                                 SWFrameRec *workFrameP, short maxDepth);
                                                         /* 0x00477640 */
extern short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP, short pictResID,
                                    char useGetPicture); /* 0x00478f30 */
extern short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP,
                                    void *pictH);        /* 0x00478fa0 */
struct Image_t;
extern short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *maskGWorldP,
                                    struct Image_t *cIconH); /* 0x00479070 */
extern short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld,
                                    CGrafPtr maskGWorld, Rect *rectP); /* 0x00479250 */
extern short SWCreateFrameFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld,
                                    short maxWidth, short frameHeight,
                                    unsigned char maskType);   /* 0x004769d0 */
extern void SWCreateFrameFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld);
                                                         /* 0x004769e0 */
extern short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
                                    CGrafPtr pictGWorldP, CGrafPtr maskGWorldP,
                                    CGrafPtr tempMaskGWorldP, Rect *frameRect,
                                    unsigned char maskType);   /* 0x004769f0 */
extern void SWCreateRegionFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld);
                                                         /* 0x00478f20 */

extern void SWFindSpritesToBeRemoved(SpriteWorldPtr spriteWorldP); /* 0x00478cf0 */
extern void SWCloseSprite(SpritePtr deadSpriteP);        /* 0x004754a0 */
extern void SWClearStickyError(void);                    /* 0x004794a0 */
extern void SWStdWorldDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                               Rect *srcRect, Rect *dstRect); /* 0x00477de0 */
extern short SWFixImageGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld,
                              Rect *rectP);              /* 0x00479260 */
extern void SWUnlockWindowFrame(SWFrameRec *windowFrameP); /* 0x00477070 */
extern void SWWindowFrameMoved(SWFrameRec *windowFrameP, Rect *frameRect);
                                                         /* 0x00477f30 */

extern void SWCheckIdleSpriteOverlap(SpriteWorldRec *spriteWorldP,
                                     SpritePtr idleSpriteP,
                                     SpritePtr headActiveSpriteP); /* 0x00478bb0 */

#ifdef __cplusplus
}
#endif

#endif /* SPRITEWORLD_H */
