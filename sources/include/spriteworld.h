/* spriteworld.h -- the SpriteWorld fork, defined in sources/lib/. Notes:
   Research/declaration-mismatches.md */
#ifndef SPRITEWORLD_H
#define SPRITEWORLD_H
#include "gametypes.h"
#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* a frame's mask kinds: an enum (an int) on mac, a byte on the win port */
#ifdef MACBUILD
typedef enum MaskType {
    kNoMask = 0,
    kPixelMask = 1,
    kRegionMask = 2,
    kFatMask = (kPixelMask + kRegionMask),
    kSolidMask = 4
} MaskType;
#else
typedef unsigned char MaskType;
#define kPixelMask              1
#endif
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
                                Boolean updateWindow);      /* 0x00477fa0 */
extern void SWLockSpriteWorld(SpriteWorldPtr spriteWorld);   /* 0x00477ba0 */
extern void SWSetPortToBackground(SpriteWorldPtr spriteWorld); /* 0x00477c10 */
extern Boolean SWIsWindowInForeground(SpriteWorldPtr spriteWorldP); /* 0x00478e20 */

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
extern Boolean SWDisposeFrame(SWFrameRec **oldFramePP);     /* 0x00476bd0 */
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
                                 MaskType maskType);              /* 0x00474970 */
extern short SWRemoveSprite(SpriteRec *oldSpriteP);      /* 0x004771c0 */
extern void SWUnlockSprite(SpritePtr srcSpriteP);        /* 0x004756c0 */
extern void SWDisposeSprite(SpritePtr *deadSpritePP);    /* 0x00475470 */
extern short SWCreateSprite(SpritePtr *newSpriteP, void *spriteStorageP,
                            short maxFrames);            /* 0x004744a0 */
extern short SWAddFrame(SpritePtr srcSpriteP, FramePtr newFrameP); /* 0x00475740 */
extern short SWAddSprite(SpriteLayerRec *spriteLayerP, SpriteRec *newSpriteP); /* 0x00477140 */
extern short SWCreateFrameFromCicnResource(SpriteWorldRec *destSpriteWorld,
        SWFrameRec **newFrameP, short iconResID,
        MaskType maskType);                         /* 0x00476800 */
#if !defined(MACBUILD) || !V111 /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short SWCreateRLEData(SpritePtr srcSpriteP, short depth);   /* 0x004760c0 */
#endif
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
                          MaskType maskType);
extern void SWWindowMoved(SpriteWorldPtr spriteWorldP);            /* 0x00477f10 */
extern Boolean sw_depth_ok(SpriteWorldPtr world);                    /* 0x00478f10 */
extern short SWCloneFramesFromMaster(SpritePtr dstSpriteP, SpritePtr masterSpriteP); /* 0x00475f70 */
extern void SWSetSpriteFrameRange(SpritePtr srcSpriteP, short firstFrameIndex,
                                  short lastFrameIndex); /* 0x00475a60 */
extern void SWTempSpriteMoveProc(SpritePtr srcSpriteP);  /* 0x00475800 */
extern short SWSetSpriteDrawProc(SpritePtr srcSpriteP,
                                 DrawProcPtr drawProc);  /* 0x00475970 */
extern void SWLockSprite(SpritePtr srcSpriteP);          /* 0x00475550 */
extern Boolean SWRLECollision(SpritePtr srcSpriteP,
                           SpritePtr dstSpriteP);        /* 0x00475c80 */
extern Boolean SWRLECollisionWithPoint(SpritePtr srcSpriteP,
                                    Point *testPoint);   /* 0x00475e20 */
extern Boolean SWRadiusCollision(SpritePtr srcSpriteP,
                              SpritePtr dstSpriteP);     /* 0x00475be0 */
extern Boolean SWIsSpriteInRect(SpritePtr srcSpriteP,
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
extern Boolean BlitPixieRLECollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                                 Rect *srcRect, Rect *dstRect);        /* 0x00471800 */
extern Boolean BlitPixieRLEPointCollisionProc(SpritePtr srcSpriteP, Point *testPoint,
                                 Rect *destRect);                      /* 0x00471920 */
extern void BlitCopyLine(FramePtr srcFrameP, FramePtr dstFrameP,
                                 Point srcPoint, Point dstPoint);      /* 0x00479cf0 */
extern void CopyLine(FramePtr srcFrameP, FramePtr dstFrameP,
                                 Point srcPoint, Point dstPoint);      /* 0x00479c50 */
#ifdef MACBUILD
extern void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                 unsigned long color, unsigned long alpha, unsigned long dstShade,
                                 unsigned long srcRamp, unsigned long dstRamp);
#else
extern void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                 unsigned long color, long srcShade, long dstShade,
                                 unsigned long srcRamp, unsigned long dstRamp);
                                                                       /* 0x004797b0 */
#endif
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
#ifndef MACBUILD
extern void ComputePixelValues(short pixelSize);                       /* 0x00479210 */
#endif
#ifdef MACBUILD
extern short GetGDeviceDepth(GDHandle theGDH);
#else
extern short GetGDeviceDepth(void *theGDH);                            /* 0x0047b7c0 */
#endif
extern void MoveParticles(void);                                       /* 0x0047c800 */
extern SpriteRec *gSWCurrentSpriteBeingDrawn;            /* 0x00870da8 */
extern SpriteWorldRec *gSWCurrentSpriteWorld;            /* 0x008706f0 */
#pragma cplusplus on
EXTERN_CPP_BEGIN
#ifndef MACBUILD
extern unsigned short SWMaskedRGBValue;                  /* 0x007d0500 */
extern short gSWStickyError;                             /* 0x007d0504 */
#endif
EXTERN_CPP_END
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
extern Boolean BlitPixieRLECollision(unsigned char *srcTokens, unsigned char *dstTokens,
        Rect *srcRect, Rect *dstRect);                   /* 0x00472190 */
extern Boolean BlitPixieRLECollisionWithPoint(unsigned char *rleData, Point *pt,
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
#if defined(MAC1010) || !(defined(MACBUILD) && defined(SW_DEFINES_FLOATINGRECT8))   /* BlitPixieRect.o calls it undeclared */
extern void BlitPixieTranslucentClearCopy8(unsigned char *dstPixels,
        unsigned char *srcPixels, long numPixels,
        unsigned char clearValue);                       /* 0x00473dc0 */
#endif
#if defined(MACBUILD) && !defined(MAC1010) && !defined(SW_DEFINES_FLOATINGRECT8)
extern void BlitPixieFloatingRect8();   /* the fork calls it with no prototype in scope */
#elif defined(MACBUILD)
extern void BlitPixieFloatingRect8(unsigned char *srcPixels,
        unsigned char *dstPixels, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short byteWidth, unsigned short numRows,
        unsigned short backgroundValue);
#else
extern void BlitPixieFloatingRect8(unsigned char *srcPixels,
        unsigned char *dstPixels, long srcRowBytes, long dstRowBytes,
        unsigned short byteWidth, unsigned short numRows,
        unsigned short backgroundValue);                 /* 0x00471dc0 */
#endif
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
extern unsigned long fix_power_of_two(unsigned long level);  /* 0x004736a0 */
#ifndef MACBUILD
extern long GetMainDevice(void);                         /* 0x004f2a50 */
#endif

/* --- the blitters' shared state --- */
#pragma cplusplus on
EXTERN_CPP_BEGIN
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
EXTERN_CPP_END
#pragma cplusplus reset

/* --- translucent blends (mac _Trans8Bit/_Trans16Bit, inlined in the win build) --- */
#if defined(MACBUILD) && (!defined(MAC1010) || defined(SW_DEFINES_TRANS))
#ifndef SW_DEFINES_TRANS
extern char Trans8Bit(unsigned long src, unsigned long dst, long alpha);
extern unsigned short Trans16Bit(unsigned long src, unsigned long dst, long alpha);
#endif
#else
static inline char Trans8Bit(char src, char dst, unsigned long alpha)
{
    /* BUG: any alpha above 3 skips the blend, so (31 - alpha) >> 3 only ever selects row 3 */
    if (alpha > 3)
        return src;
    return alphaTable[(31 - alpha) >> 3][src][dst];
}

static inline unsigned short Trans16Bit(unsigned long src, unsigned long dst, unsigned long alpha)
{
    unsigned long srcSplit, dstSplit, blended;

    srcSplit = SW_SPLIT15(src);
    dstSplit = SW_SPLIT15(dst);
    blended = srcSplit * alpha + dstSplit * (32 - alpha);
    return ((blended >> 5) & 0x7c1f) | ((blended >> 20) & 0x3e0);
}
#endif

#pragma cplusplus on
EXTERN_CPP_BEGIN
#ifndef MACBUILD
extern unsigned short SWTransparentRGBValue;             /* 0x007d0502 */
#endif
EXTERN_CPP_END
#pragma cplusplus reset

/* --- lines --- */
extern void DrawTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                                unsigned long color, short srcShade, short dstShade,
                                short srcRamp, short dstRamp);        /* 0x00479590 */
#ifdef MACBUILD
extern Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint, Rect *clipRect);
#else
extern Boolean SWClipLineWithRect(Point *srcPoint, Point *dstPoint,
                               Rect *clipRectP);         /* 0x0047b3c0 */
#endif

/* --- errors --- */
/* an SW_ASSERT site: the fork file's name and line, which differ per build (the win port's
   lines, the mac 1.1.1 build machine's path and lines). SW_LINE1010, set per umbrella, takes a
   1.1.1 line to 1.0.10's: Research/notes/spriteworld-mac.md */
#ifdef MAC1010
#ifndef SW_LINE1010
#define SW_LINE1010(n) (n)
#endif
#define SW_LINE(n) SW_LINE1010(n)
#else
#define SW_LINE(n) (n)
#endif
#ifdef MACBUILD
#ifdef MAC1010
#define kSWSourceDir ""
#else
#define kSWSourceDir "/Users/jerome/Ambrosia/ev_nova1.1.0/sources/3rdparty/ev_nova/__obsolete__/Libraries/SpriteWorld 2.3.1/SpriteWorld Files/Sources/"
#endif
#define SW_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(kSWSourceDir file, SW_LINE(macLine))
/* the fork's Carbon pixmap accessors */
#define SWGetPixBaseAddr(pm) GetPixBaseAddr(pm)
#ifdef MAC1010
#define SWGetPixRowBytes(pm) ((**(pm)).rowBytes & 0x3FFF)
#define SWGetPixBitDepth(pm) ((**(pm)).pixelSize)
#else
#define SWGetPixRowBytes(pm) GetPixRowBytes(pm)
#define SWGetPixBitDepth(pm) GetPixDepth(pm)
#endif
extern Rect *GetRgnBBox(RgnHandle rgn);
/* the mac bodies' SW_ASSERT, with the line of the mac file named by the umbrella's SW_FILE */
#define SW_ASSERT(condition, macLine) if (!(condition)) SWAssertFail(kSWSourceDir SW_FILE, SW_LINE(macLine))
/* an engine assert site: the 1.1.1 build machine's engine path and line */
#define EV_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(kEngineSourceDir file, SW_LINE(macLine))
#else
#define SW_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(file, winLine)
#define EV_ASSERT_FAIL(file, winLine, macLine) SWAssertFail(file, winLine)
#endif
extern void SWAssertFail(char *filename, int lineNum);   /* 0x004794d0 */
extern void SWSetStickyIfError(short errNum);            /* 0x004794b0 */

/* --- particles --- */
extern void EraseParticlesOffscreen(SpriteWorldPtr spriteWorldP); /* 0x0047bb30 */
extern void UpdateParticlesInWindow(SpriteWorldRec *spriteWorldP); /* 0x0047c3a0 */
#ifdef MACBUILD
extern OSErr SWEnterSpriteWorld(void);
#else
extern int SWEnterSpriteWorld(void);                           /* 0x00477630 */
#endif
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
#ifdef MACBUILD
#ifndef IMPLICIT_SWGWORLDS /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP, short pictResID);
#endif
#else
extern short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP, short pictResID,
                                    Boolean useGetPicture); /* 0x00478f30 */
#endif
extern short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *pictGWorldP,
                                    void *pictH);        /* 0x00478fa0 */
struct Image_t;
extern short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                    CGrafPtr *maskGWorldP,
                                    struct Image_t *cIconH); /* 0x00479070 */
#ifndef IMPLICIT_SWGWORLDS /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld,
                                    CGrafPtr maskGWorld, Rect *rectP); /* 0x00479250 */
#endif
extern short SWCreateFrameFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld,
                                    short maxWidth, short frameHeight,
                                    MaskType maskType);   /* 0x004769d0 */
extern void SWCreateFrameFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld);
                                                         /* 0x004769e0 */
extern short SWCreateFrameFromGWorldAndRectPartial(SWFrameRec **newFrameP,
                                    CGrafPtr pictGWorldP, CGrafPtr maskGWorldP,
                                    CGrafPtr tempMaskGWorldP, Rect *frameRect,
                                    MaskType maskType);   /* 0x004769f0 */
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

#ifdef MACBUILD
/* the mac 1.1.1 SpriteWorld's functions the win port never had */
extern void SWSetFrameMaskRgn(SWFrameRec *srcFrameP, RgnHandle maskRgn);
extern short SWCreateRegionFromCIconMask(RgnHandle *maskRgn, CIconHandle cIconH);
extern short SWCreateRegionFromGWorldAndRect(RgnHandle *maskRgn, CGrafPtr maskGWorld, Rect *frameRect);
extern short SWCreateRegionFromGWorldAndRectStart(CGrafPtr *tempMaskGWorld, short maxWidth, short maxHeight);
extern short SWCreateRegionFromGWorldAndRectPartial(RgnHandle *maskRgn, CGrafPtr maskGWorld,
        CGrafPtr tempMaskGWorld, Rect *frameRect);
extern short SWCreateRegionMaskFromGWorld(CGrafPtr gWorld, Rect *rectP, RgnHandle maskRgn);
extern void SWSetTransparentColor(RGBColor *color);
extern RGBColor gSWTransparentColor;
extern Boolean SWAlignFrameToWindow(SWFrameRec *dstFrameP, SWFrameRec *windowFrameP);
extern Rect *GetCPortPortRect(CGrafPtr port);
extern short SWCreateMaskAndFixImage(CGrafPtr pictureGWorld, CGrafPtr maskGWorld, Rect *rectP);
extern void SWCopyGWorlds(CGrafPtr srcGWorldP, CGrafPtr dstGWorldP);
extern void SWFlipPixelsHorizontally(CGrafPtr dstGWorldP, Rect *rectP);
extern void SWFlipPixelsVertically(CGrafPtr dstGWorldP, Rect *rectP);
extern short SWBlackenGWorld(CGrafPtr oldGWorld);
extern short SWWhitenGWorld(CGrafPtr oldGWorld);
extern short SWFastWhitenGWorld(CGrafPtr pictureGWorld);
extern short SWStickyError(void);
extern Boolean SWHasSystem7(void);

/* the masking blitters' alpha-free colour masks */
#define kAlphaMask16    0x7FFF
#define kAlphaMask32    0x00FFFFFFL

/* SpriteWorld's error codes the win port never names */
#define kSystemTooOldErr            100
#define kMaxFramesErr               101
#define kInvalidFramesIndexErr      102
#define kNotCWindowErr              103
#define kNilParameterErr            104
#define kOutOfRangeErr              107
#define kTilingNotInitialized       108
#define kTilingAlreadyInitialized   109
#define kNullTileMapErr             110
#define kTileMapNotLockedErr        111
#define kAlreadyCalledErr           112
#define kSpriteNotCompiledErr       113
#define kBadParameterErr            114
#define kSpriteAlreadyInLayer       115
#define kNilFrameErr                116
#define kNotLockedErr               117
#define k68kOnlyErr                 118

/* BlitPixieInterface.c's draw procs the win port dropped */
#define SW_68K 0
typedef struct BlitPixieOffsetInfo {
    long srcOffsetAtoB;
    long srcOffsetBtoA;
    long dstOffsetAtoB;
    long dstOffsetBtoA;
} BlitPixieOffsetInfo, *BlitPixieOffsetInfoPtr;
typedef void (*BlitFuncPtr)(long srcRowBytes, long dstRowBytes, char *srcBaseAddr,
        char *dstBaseAddr);
#define BP_EQUAL_RECT(srcRect, dstRect) \
    (srcRect.top == dstRect.top && \
     srcRect.bottom == dstRect.bottom && \
     srcRect.left == dstRect.left && \
     srcRect.right == dstRect.right)
extern void BlitPixieClear(unsigned char *dstPixel, unsigned long dstRowBytes,
        unsigned short bytes, unsigned short rows, unsigned long color);
extern void BlitPixieDoubleRects(unsigned char *src, unsigned char *dst, unsigned short bytesA,
        unsigned short bytesB, unsigned short rows, BlitPixieOffsetInfo *info);
extern void BlitPixieMaskColor(unsigned long srcColor, unsigned char *dstPixel,
        unsigned char *maskPixel, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short bytes, unsigned short rows);
extern Boolean BlitPixieMaskCollision(unsigned char *maskPtr1, unsigned char *maskPtr2,
        unsigned long rowBytes1, unsigned long rowBytes2, unsigned short bytes,
        unsigned short rows);
extern void BlitPixieFlip8Bit(unsigned char *srcPixel, unsigned char *dstPixel,
        unsigned long srcRowBytes, unsigned long dstRowBytes, unsigned short width,
        unsigned short height);
extern void BlitPixieFlip16Bit(unsigned short *srcPixel, unsigned short *dstPixel,
        unsigned long srcRowBytes, unsigned long dstRowBytes, unsigned short width,
        unsigned short height);
extern void BlitPixieFlip32Bit(unsigned long *srcPixel, unsigned long *dstPixel,
        unsigned long srcRowBytes, unsigned long dstRowBytes, unsigned short width,
        unsigned short height);
extern void BlitPixieFlipMask8Bit(unsigned char *srcPixel, unsigned char *dstPixel,
        unsigned char *maskPixel, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short width, unsigned short height);
extern void BlitPixieFlipMask16Bit(unsigned short *srcPixel, unsigned short *dstPixel,
        unsigned short *maskPixel, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short width, unsigned short height);
extern void BlitPixieFlipMask32Bit(unsigned long *srcPixel, unsigned long *dstPixel,
        unsigned long *maskPixel, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short width, unsigned short height);
extern Boolean BlitPixieFlipMaskCollision(unsigned char *maskPtr1, unsigned char *maskPtr2,
        unsigned long rowBytes1, unsigned long rowBytes2, unsigned short bytes,
        unsigned short rows);
extern void BlitPixieFlipMaskColor(unsigned long srcColor, unsigned char *dstPixel,
        unsigned char *maskPixel, unsigned long srcRowBytes, unsigned long dstRowBytes,
        unsigned short bytes, unsigned short rows);
extern void BlitPixieRLEColor(unsigned char *srcTokens, unsigned long srcColor,
        unsigned char *dstPixel, unsigned long dstRowBytes);
extern void BlitPixieRLEColorClipped(unsigned char *srcTokens, unsigned long srcColor,
        unsigned char *dstPixel, unsigned long dstRowBytes, Rect *dstClipRect);
typedef struct StarArray {
    short horizLoc;
    short vertLoc;
    short oldHorizLoc;
    short oldVertLoc;
    short horizSpeed;
    short vertSpeed;
    unsigned long color;
    Boolean isOn;
    Boolean needsToBeErased;
    short userData;
} StarArray, *StarArrayPtr;
#define SW_BITSET(p,n)  (p)[n >> 3] |= 0x80U >> (n & 7)
#define SW_BITCLR(p,n)  (p)[n >> 3] &= ~(0x80U >> (n & 7))
#define SW_BITTST(p,n)  ((p)[n >> 3] & (0x80U >> (n & 7))) != 0
#define SW_NIBBLEGET(p,n) (((p)[n >> 1] & ~(0x0F << ((n & 1) << 2))) >> (((~n) & 1) << 2))
#define SW_NIBBLESET(p,n,c) (p)[n >> 1] = (((c) & 0x0F) << (((~n) & 1) << 2)) | \
        ((p)[n >> 1] & (0x0F << ((n & 1) << 2)))
extern void SWGetColorFromValue(FramePtr dstFrameP, RGBColor *color, unsigned long pixelValue);
extern void SWSetPixel(FramePtr dstFrameP, short x, short y, unsigned long color);
extern unsigned long SWGetPixel(FramePtr srcFrameP, short x, short y);
extern void SWAnimateStarField(SpriteWorldPtr spriteWorldP, StarArray *starArray, short numStars,
        unsigned long backColor);
extern void SWCopyBitsDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieClearDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieMaskColorDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieRLEColorDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieFlipRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieFlipMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieFlipMaskColorDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieCompiledSpriteDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieAllBitRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieAllBitMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern void BlitPixieAllBitPartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, Rect *srcRect, Rect *dstRect);
extern Boolean BlitPixieMaskCollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP, Rect *srcRect, Rect *dstRect);
extern Boolean BlitPixieFlipMaskCollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP, Rect *srcRect, Rect *dstRect);
extern short SWFlagRectAsChanged(SpriteWorldPtr spriteWorldP, Rect *theChangedRect);

/* SpriteLayer.c's functions the win port never had */
extern void SWRemoveAllSpritesFromLayer(SpriteLayerPtr srcSpriteLayerP);
extern void SWDisposeAllSpritesInLayer(SpriteLayerPtr spriteLayerP);
extern short SWCountNumSpritesInLayer(SpriteLayerPtr spriteLayerP);
extern OSErr SWSwapSprite(SpritePtr srcSpriteP, SpritePtr dstSpriteP);
extern OSErr SWInsertSpriteAfterSprite(SpritePtr newSpriteP, SpritePtr dstSpriteP);
extern OSErr SWInsertSpriteBeforeSprite(SpritePtr newSpriteP, SpritePtr dstSpriteP);
extern SpritePtr SWGetNextSprite(SpriteLayerPtr spriteLayerP, SpritePtr curSpriteP);
extern SpritePtr SWGetPreviousSprite(SpriteLayerPtr spriteLayerP, SpritePtr curSpriteP);
extern void SWSortSpriteLayer(SpriteLayerPtr theLayer);
extern void SWPauseSpriteLayer(SpriteLayerPtr spriteLayerP);
extern void SWUnpauseSpriteLayer(SpriteLayerPtr spriteLayerP);
extern void SWSetLayerAsNonScrolling(SpriteLayerPtr spriteLayerP, Boolean layerIsNonScrolling);
extern SpritePtr SWFindSpriteByPoint(SpriteLayerPtr spriteLayerP, SpritePtr startSpriteP,
                                     Point testPoint);
extern Boolean SWIsPointInSprite(SpritePtr srcSpriteP, Point testPoint);

/* Sprite.c's types and functions the win port never had */
typedef struct RectArray {
    short numFrames;
    Rect frameRects[];
} RectArray, *RectArrayPtr, **RectArrayHdl;

typedef enum {
    kSWFlipNone = 0,
    kSWFlipHorizontal = 1,
    kSWFlipVertical = 2,
    kSWFlipBoth = 3
} FlipType;

#define SW_COLLISION_RECT_TOP(spriteP) \
    (spriteP->destFrameRect.top + spriteP->curFrameP->collisionInset.top)
#define SW_COLLISION_RECT_LEFT(spriteP) \
    (spriteP->destFrameRect.left + spriteP->curFrameP->collisionInset.left)
#define SW_COLLISION_RECT_BOTTOM(spriteP) \
    (spriteP->destFrameRect.bottom - spriteP->curFrameP->collisionInset.bottom)
#define SW_COLLISION_RECT_RIGHT(spriteP) \
    (spriteP->destFrameRect.right - spriteP->curFrameP->collisionInset.right)

extern RgnHandle gSWCollisionSpareRgn;
extern RgnHandle gSWCollisionSectRgn;

extern void DebugInteger(long n);
extern OSErr SWCreateSpriteFromPictResource(SpriteWorldPtr destSpriteWorldP, SpritePtr *newSpriteP,
        void *spriteStorageP, short pictResID, short maskResID, short maxFrames,
        MaskType maskType);
extern OSErr SWCreateSpriteFromSinglePict(SpriteWorldPtr destSpriteWorldP, SpritePtr *newSpriteP,
        void *spriteStorageP, short pictResID, short maskResID, short frameDimension,
        short borderWidth, MaskType maskType);
extern OSErr SWUpdateSpriteFromPictResource(SpritePtr theSpriteP, short pictResID);
extern OSErr SWSaveRLESprite(SpritePtr srcSpriteP, short rleResID);
extern OSErr SWCloneSpriteFromTile(SpriteWorldPtr spriteWorldP, SpritePtr *newSpriteP,
        void *spriteStorageP, short firstTileID, short lastTileID);
extern void SWRemoveSpriteFromAnimation(SpriteWorldPtr spriteWorldP, SpritePtr spriteP,
        Boolean disposeOfSprite);
extern void SWFlipSprite(SpritePtr srcSpriteP, FlipType direction);
extern OSErr SWInsertFrame(SpritePtr srcSpriteP, FramePtr newFrameP, long frameIndex);
extern void SWRemoveFrame(SpritePtr srcSpriteP, FramePtr oldFrameP);
extern void SWStdSpriteColorDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect,
        Rect *dstRect);
extern OSErr SWSetCurrentFrame(SpritePtr srcSpriteP, FramePtr newFrameP);
extern void SWSetSpriteColor(SpritePtr srcSpriteP, RGBColor *color);
extern void SWSetSpriteFlippedMode(SpritePtr srcSpriteP, Boolean flippedHorizontal,
        Boolean flippedVertical);
extern void SWSetSpriteFrameAdvance(SpritePtr srcSpriteP, short frameAdvance);
extern void SWSetSpriteFrameAdvanceMode(SpritePtr srcSpriteP, AdvanceType advanceMode);
extern void SWSetSpriteFrameTime(SpritePtr srcSpriteP, long timeInterval);
extern void SWSetSpriteFrameProc(SpritePtr srcSpriteP, FrameProcPtr frameProc);
extern void SWSetSpriteMoveBounds(SpritePtr srcSpriteP, Rect *moveBoundsRect);
extern void SWSetSpriteMoveDelta(SpritePtr srcSpriteP, short horizDelta, short vertDelta);
extern void SWSetSpriteMoveTime(SpritePtr srcSpriteP, long timeInterval);
extern void SWSetSpriteMoveProc(SpritePtr srcSpriteP, MoveProcPtr moveProc);
extern short SWGetSpriteVertLoc(SpritePtr srcSpriteP);
extern short SWGetSpriteHorizLoc(SpritePtr srcSpriteP);
extern void SWMoveSprite(SpritePtr srcSpriteP, short horizLoc, short vertLoc);
extern void SWOffsetSprite(SpritePtr srcSpriteP, short horizOffset, short vertOffset);
extern Boolean SWBounceSprite(SpritePtr srcSpriteP);
extern Boolean SWWrapSprite(SpritePtr srcSpriteP);
extern Boolean SWRegionCollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP);
extern Boolean SWPixelCollision(SpritePtr srcSpriteP, SpritePtr dstSpriteP);
extern Boolean SWIsSpriteFullyInRect(SpritePtr srcSpriteP, Rect *testRect);
extern short SWCreateFrameFromPictResource(SpriteWorldPtr destSpriteWorldP, FramePtr *newFrameP,
        short pictResID, short maskResID, MaskType maskType);
extern OSErr SWUpdateFrameMasks(FramePtr srcFrameP);

/* Scrolling.c (mac only) */
typedef void (*WorldMoveProcPtr)(SpriteWorldPtr spriteWorldP, SpritePtr followSpriteP);
typedef void (*DoubleDrawProcPtr)(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRectA,
                                  Rect *dstRectA, Rect *srcRectB, Rect *dstRectB);
extern void BlitPixieDoubleRectDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRectA,
        Rect *dstRectA, Rect *srcRectB, Rect *dstRectB);
extern void SWUpdateScrollingWindow(SpriteWorldPtr spriteWorldP);
extern void SWUpdateScrollingSpriteWorld(SpriteWorldPtr spriteWorldP, Boolean updateWindow);
extern void SWProcessScrollingSpriteWorld(SpriteWorldPtr spriteWorldP);
extern void SWAnimateScrollingSpriteWorld(SpriteWorldPtr spriteWorldP);
extern void SWFastAnimateScrollingSpriteWorld(SpriteWorldPtr spriteWorldP);
extern void SWUnwrapWorkAreaToBackground(SpriteWorldPtr spriteWorldP);
extern void SWDrawWrappedSprite(SpritePtr srcSpriteP, FramePtr dstFrameP, Rect *srcRect,
                                Rect *dstRect);
extern void SWEraseWrappedSprite(SpriteWorldPtr spriteWorldP, Rect *dstRect);
extern void SWWrapWorkAreaToFrame(SpriteWorldPtr spriteWorldP, FramePtr dstFrameP, Rect *dstRectP);
extern void SWWrapRectToScreen(SpriteWorldPtr spriteWorldP, Rect *srcRectP);
extern void SWCheckWrappedIdleSpriteOverlap(SpriteWorldPtr spriteWorldP, SpritePtr idleSpriteP,
                                            SpritePtr headActiveSpriteP);
extern void SWCheckWrappedIdleSpritesWithTiles(SpriteWorldPtr spriteWorldP,
                                               SpritePtr headIdleSpriteP);
extern void SWCheckWrappedIdleSpritesWithRects(SpriteWorldPtr spriteWorldP,
                                               SpritePtr headIdleSpriteP);
extern void SWSetScrollingWorldMoveBounds(SpriteWorldPtr spriteWorldP, Rect *scrollRectMoveBounds);
extern void SWSetScrollingWorldMoveProc(SpriteWorldPtr spriteWorldP,
                                        WorldMoveProcPtr worldMoveProcP, SpritePtr followSpriteP);
extern void SWSetDoubleRectDrawProc(SpriteWorldPtr spriteWorldP,
                                    DoubleDrawProcPtr doubleRectDrawProc);
extern void SWSetSpriteWorldScrollDelta(SpriteWorldPtr spriteWorldP, short horizDelta,
                                        short vertDelta);
extern void SWMoveVisScrollRect(SpriteWorldPtr spriteWorldP, short horizPos, short vertPos);
extern void SWOffsetVisScrollRect(SpriteWorldPtr spriteWorldP, short horizOffset,
                                  short vertOffset);
extern void SWResizeVisScrollRect(SpriteWorldPtr spriteWorldP, short horizSize, short vertSize);
extern void SWCalculateOffscreenScrollRect(SpriteWorldPtr spriteWorldP);

/* SpriteWorld.c's functions the win port never had */
typedef void (*CleanUpCallBackPtr)(void);
#define SW_RECT_WIDTH(theRect)  (theRect.right - theRect.left)
#define SW_RECT_HEIGHT(theRect) (theRect.bottom - theRect.top)
extern SpriteWorldPtr gSWCleanUpSpriteWorldP;
extern CleanUpCallBackPtr gSWCleanUpCallBackP;
extern Boolean gSWmmuMode;
extern Rect mainDeviceInitBounds;
#ifdef MACBUILD
extern void BlitPixieGetProcessorType(void);
#else
extern short BlitPixieGetProcessorType(void);
#endif
extern void SWDisposeSpriteWorld(SpriteWorldPtr *spriteWorldPP);
extern void SWSwapSpriteLayer(SpriteWorldPtr spriteWorldP, SpriteLayerPtr srcSpriteLayerP,
        SpriteLayerPtr dstSpriteLayerP);
extern SpriteLayerPtr SWGetNextSpriteLayer(SpriteWorldPtr spriteWorldP,
        SpriteLayerPtr curSpriteLayerP);
extern void SWUnlockSpriteWorld(SpriteWorldPtr spriteWorldP);
extern OSErr SWSetSpriteWorldOffscreenDrawProc(SpriteWorldPtr spriteWorldP, DrawProcPtr drawProc);
extern void SWStdWorldClearDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect,
        Rect *dstRect);
extern void SWInterlacedCopyBitsDrawProc(FramePtr srcFrameP, FramePtr dstFrameP, Rect *srcRect,
        Rect *dstRect);
extern void SWCopyBackgroundToWorkArea(SpriteWorldPtr spriteWorldP);
extern OSErr SWChangeWorldRect(SpriteWorldPtr spriteWorldP, Rect *worldRect,
        Boolean changeOffscreenAreas);
extern void SWRestoreWorldRect(SpriteWorldPtr spriteWorldP);
extern void SWUpdateWindow(SpriteWorldPtr spriteWorldP);
extern void SWSetCleanUpSpriteWorld(SpriteWorldPtr spriteWorldP);
extern void SWSetCleanUpFunction(CleanUpCallBackPtr callBackP);
extern unsigned long SWGetSpriteWorldVersion(void);
extern void SWProcessNonScrollingLayers(SpriteWorldPtr spriteWorldP);
extern void SWProcessSpriteLayer(SpriteWorldPtr spriteWorldP, SpriteLayerPtr curSpriteLayerP);
extern void SWCheckIdleSpritesWithTiles(SpriteWorldPtr spriteWorldP, SpritePtr headIdleSpriteP);
extern void SWCheckIdleSpritesWithRects(SpriteWorldPtr spriteWorldP, SpritePtr headIdleSpriteP);
extern short SWFlagScrollingRectAsChanged(SpriteWorldPtr spriteWorldP, Rect *theChangedRect);

/* Tiling.c (mac only) */
#define kNumTileLayers      10
#define kMaskIsPartialMask      true
#define kMaskIsNotPartialMask   false

typedef enum {
    kSWEntireSprite = 0,
    kSWTopSide,
    kSWRightSide,
    kSWBottomSide,
    kSWLeftSide
} SWTileSearchType;

typedef short **TileMapPtr;

typedef struct TileMapStruct {
    short numRows;
    short numCols;
    TileMapPtr tileMap;
    Handle tileMapDataH;
    Handle arrayOfPointersH;
    Boolean isLocked;
} TileMapStruct, *TileMapStructPtr;

typedef void (*TileChangeProcPtr)(SpriteWorldPtr spriteWorldP);
typedef void (*TileRectDrawProcPtr)(SpriteWorldPtr spriteWorldP, Rect *updateRectP,
                                    Boolean optimizingOn);

extern OSErr SWInitTiling(SpriteWorldPtr spriteWorldP, short tileHeight, short tileWidth,
                          short maxNumTiles);
extern void SWExitTiling(SpriteWorldPtr spriteWorldP);
extern OSErr SWInitTilingCache(SpriteWorldPtr spriteWorldP);
extern OSErr SWCreateTileMap(TileMapStructPtr *tileMapStructPP, short numTileMapRows,
                             short numTileMapCols);
extern void SWDisposeTileMap(TileMapStructPtr *tileMapStructPP);
extern void SWLockTileMap(TileMapStructPtr tileMapStructP);
extern void SWUnlockTileMap(TileMapStructPtr tileMapStructP);
extern void SWInstallTileMap(SpriteWorldPtr spriteWorldP, TileMapStructPtr tileMapStructP,
                             short tileLayer);
extern OSErr SWLoadTileMap(ResType resType, TileMapStructPtr *tileMapStructPP, short resourceID);
extern OSErr SWSaveTileMap(ResType resType, TileMapStructPtr tileMapStructP, short destResID);
extern OSErr SWResizeTileMap(TileMapStructPtr tileMapStructP, short numNewTileMapRows,
                             short numNewTileMapCols);
extern OSErr SWLoadTileFromCicnResource(SpriteWorldPtr spriteWorldP, short tileID, short cicnID,
                                        MaskType maskType, Boolean maskIsPartialMask);
extern OSErr SWLoadTilesFromPictResource(SpriteWorldPtr spriteWorldP, short startTileID,
        short endTileID, short pictResID, short maskResID, MaskType maskType,
        Boolean maskIsPartialMask, short horizBorderWidth, short vertBorderHeight);
extern void SWDisposeTile(SpriteWorldPtr spriteWorldP, short tileID);
extern void SWLockTiles(SpriteWorldPtr spriteWorldP);
extern void SWUnlockTiles(SpriteWorldPtr spriteWorldP);
extern OSErr SWCreateExtraBackFrame(SpriteWorldPtr spriteWorldP, Rect *frameRect);
extern void SWDisposeExtraBackFrame(SpriteWorldPtr spriteWorldP);
extern OSErr SWSetPortToExtraBackFrame(SpriteWorldPtr spriteWorldP);
extern void SWSetTilingOn(SpriteWorldPtr spriteWorldP, Boolean tilingIsOn);
extern OSErr SWChangeTileSize(SpriteWorldPtr spriteWorldP, short tileHeight, short tileWidth);
extern void SWSetSpriteLayerUnderTileLayer(SpriteLayerPtr spriteLayerP, short tileLayer);
extern OSErr SWSetTileMaskDrawProc(SpriteWorldPtr spriteWorldP, DrawProcPtr drawProc);
extern OSErr SWSetPartialMaskDrawProc(SpriteWorldPtr spriteWorldP, DrawProcPtr drawProc);
extern void SWSetTileChangeProc(SpriteWorldPtr spriteWorldP, TileChangeProcPtr tileChangeProc);
extern void SWSetTileMaskSolidMode(SpriteWorldPtr spriteWorldP, short tileID, Boolean isSolid);
extern OSErr SWDrawTilesInBackground(SpriteWorldPtr spriteWorldP);
extern void SWDrawTile(SpriteWorldPtr spriteWorldP, short dstTileLayer, short tileRow,
                       short tileCol, short tileID);
extern void SWDrawTilesInRect(SpriteWorldPtr spriteWorldP, Rect *updateRectP, Boolean faster);
extern void SWDrawTileLayersInRect(SpriteWorldPtr spriteWorldP, Rect *updateRectP,
                                   Boolean optimizingOn);
extern void SWDrawTilesAboveSprite(SpriteWorldPtr spriteWorldP, Rect *updateRectP,
                                   short startLayer);
extern void SWResetTilingCache(SpriteWorldPtr spriteWorldP);
extern void SWAddChangedRect(SpriteWorldPtr spriteWorldP, Rect *changedRectP);
extern void SWChangeTileImage(SpriteWorldPtr spriteWorldP, short tileID, short currentFrame);
extern void SWUpdateTileOnScreen(SpriteWorldPtr spriteWorldP, short tileID);
extern void SWResetCurrentTileImages(SpriteWorldPtr spriteWorldP);
extern short SWReturnTileUnderPixel(SpriteWorldPtr spriteWorldP, short tileLayer,
                                    short pixelCol, short pixelRow);
extern Boolean SWCheckSpriteWithTiles(SpriteWorldPtr spriteWorldP, SpritePtr srcSpriteP,
        SWTileSearchType searchType, Rect *insetRectP, short startTileLayer, short endTileLayer,
        short firstTileID, short lastTileID, Boolean fixPosition);
extern void SWWrapRectToWorkArea(SpriteWorldPtr spriteWorldP, Rect *dstRectP);
extern void SWWrapRectFromExtraBackFrame(SpriteWorldPtr spriteWorldP, Rect *dstRectP);
#define topLeft(r)  (((Point *)&(r))[0])
#define botRight(r) (((Point *)&(r))[1])
#endif

#ifdef __cplusplus
}
#endif

#endif /* SPRITEWORLD_H */
