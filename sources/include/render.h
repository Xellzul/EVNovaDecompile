/* render.h -- sprites, GWorlds, the status bar and everything that draws. Notes: Research/notes/render.md */
#ifndef RENDER_H
#define RENDER_H
#include "burger.h"
#include "gametypes.h"
#include "macport.h"
#include "spriteworld.h"

#ifdef __cplusplus
extern "C" {
#endif

static __inline void CopyRect(Rect *d, const Rect *s)
{
    d->top = s->top;
    d->left = s->left;
    d->bottom = s->bottom;
    d->right = s->right;
}

extern void AdvanceLoadBar(double amount);
extern char AreWeFrontProcess(void);
extern void CenterRect(Rect *r, Rect *bounds);
extern void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, long flags,
                          short depth, short picID);
#pragma cplusplus on
extern "C++" {
extern SpriteRec *weapTargetSprite[4];
extern SpriteRec *navTargetSprite[4];
extern SpriteRec *masterExplodSprite[64];
extern SpriteRec *masterSmokeSprite[8];
extern SpriteRec *masterBoxSprite[5];
extern Rect statusOffRect[8];
extern unsigned char targKeyOnNow;
extern unsigned short lastTargetedShipClass;
extern unsigned char lastDebugTargInfoState;
extern char gUpdateFlag;
extern char regNoticeShown;
}
#pragma cplusplus reset
extern void DisposeDialogGWorlds(void);
extern void CreateGWorldContainer(GWorldContainer *gwc, long flags, short depth,
                                  Rect *bounds);
extern void DisposeGWorldContainer(GWorldContainer *gwc);
extern void DoMainScreen(void);
extern char DrawCachedPict(short picID, GWorldContainer *dest,
                           Rect *destRect);
extern void DrawIndString(short strListID, short index);
extern void DrawLoadBar(void);
extern void DrawStatus(void);
extern void DrawStatusCargo(void);
extern void DrawStatusFuel(void);
extern void DrawStatusNav(void);
extern void DrawStatusRadar(char highlight);
extern void DrawStatusShield(void);
extern void DrawStatusTarg(void);
extern void DrawStatusWeap(void);
extern void DrawStringCentered(const unsigned char *str, short left,
                               short right, short v);
extern void EndLoopingMovie(void);
extern void FloaterEraseCallback(void *a);
extern void SetupListRects(Point *where);
extern void InitOffscreen(void);
extern void FadeToBlack(void);
extern int FlushOverlayStrips(void);
extern char GetSpriteInfo(short id, short *spriteID, short *maskID, short *xSize,
                          short *ySize, short *xTiles, short *yTiles);
extern short GetStellarSpriteXSize(short id);
extern void HandleStellarSprites(void);
extern void HideTheCursor(void);
extern void InitColors(void);
extern void InitLoadBar(void);
extern short IntroMovieIdleProc(void);
extern int KeyboardGet(void);
extern short LoadExtendedShipSprites(short shipType);
extern void LoadIntfResource(void);
extern void LoadMainScreenAnim(void);
extern void LoadMainScreenSounds(void);
extern void LoadMasterStellarSprite(short type);
extern void LoadShipSprites(void);
extern void LoadSprites(void);
extern void LoopingMovieIdle(void);
extern void MainScreenAnimDraw(void);
extern void MainScreenChoice(short which);
extern void MainScreenIdle(void);
void MainScreenKey(char key, long a, long b);
extern void MainScreenRolloverDraw(void);
extern void MainScreenUpdate(short which);
extern int MouseReadButtons(void);
extern void MouseReadPosition(long *h, long *v);
extern void PlayMainScreenClickSound(void);
extern void PrepareAllSprites(void);
extern void SetGWorldContainer(GWorldContainer *gwc);
extern void SetupInterfaceBackground(void);
extern void ShowAmbrosiaLogo(void);
extern void ShowIntroScreen(void);
extern void ShowSplashScreen(void);
extern void StopDialogSounds(void);
extern void TrackMainScreenAnim(void);
extern void TrackMainScreenHilite(void);
extern void TradeDialogTabKey(char up);
extern void UnsetGWorldContainer(GWorldContainer *gwc);
#pragma cplusplus on
extern "C++" {
extern RGBColor *armorStatusColor;
extern Rect commRect;
extern Rect escortMenuRect;
extern Rect floatingMapRect;
extern RGBColor *fuelStatusColor[2];
extern AnimDelta gAnimDelta[3];
extern short gAnimEnd[];
extern short gAnimStep[];
extern GWorldContainer gDlgWorld;
extern GWorldContainer gDlgWorlds[3];
extern unsigned long gHiliteLastTick;
extern short gHiliteLevel;
extern short gHilitedButton;
extern Handle gIntfFileHand;
extern void *gIntroMovieData;
}
#pragma cplusplus reset
extern void QTNewGWorldFromPtr(void **gw, long pixelFormat, Rect *boundsRect,
                               long cTable, long aGDevice, long flags, long baseAddr,
                               long rowBytes);
#pragma cplusplus on
extern "C++" {
extern unsigned long gLastLogoAnimTick;
extern unsigned long gLastSlideAnimTick;
extern Rect gLoadBarBase;
extern short gLogoAnimEnd;
extern short gMainPicFrame;
extern short gMainPicHeightDelta;
extern short gMainPicWidthDelta;
extern short gMovieRefNum;
extern short gStellarPictID;
extern long gVersionCheckStart;
extern unsigned char gWeapNames[256][256];
extern Rect interfacePicRect;
extern short interferenceThisFrame;
extern short lastIntfResourceID;
extern long lastFPSUpdateTime;
extern unsigned long lastRadarUpdateTime;
extern short lastTargetedShipShields;
extern RGBColor ldefColors[3];
extern ShortPoint logoDestPoint;
extern Rect logoDestRect;
extern ShortPoint mainMenuCenter;
extern Rect pictCacheRect[];
extern char preventStatusCopy;
extern RGBColor progressColor[3];
extern double progressCount;
extern double progressMax;
extern Rect progressRect;
extern short radarCount;
extern RGBColor *radarStatusColor[2];
extern ShortPoint rolloverDestPoint;
extern Rect rolloverDestRect;
extern ShortPoint screenCenter;
extern RGBColor *shieldStatusColor;
extern short shipSpriteLoaded[][6];
extern short showFloatingMap;
extern char showingIntroText;
extern ShortPoint slideDestPoint[3];
extern short smallTextFont;
extern short smallTextSize;
extern RGBColor *statusBrightColor;
extern RGBColor statusColor[8];
extern RGBColor *statusDimColor;
extern short statusFontID;
extern GWorldContainer statusOffPort;
extern unsigned long tix;

extern void *loopingMovie;

extern void *mainScreenSnd[4];
extern Rect slideDestRect[3];
extern void *staticPat[10];
extern void *targetInfoPic[768];
}
#pragma cplusplus reset

extern void RefreshSAT(void);
extern void RedrawStatus(char legacyPipeline);
extern void ShowTheCursor(void);
extern short GetMasterShipSpriteXSize(short id);
#pragma cplusplus on
extern "C++" {
extern short gSoundVolume;

extern short statusFontSize[2];
}
#pragma cplusplus reset
extern void DoPlayGameWork(char advance);
extern void HandleSpriteMurkiness(short x, short y, SpriteRec *sprite);
extern void FloaterDrawCallback(SpriteWorldPtr world);
extern void BeamEraseCallback(SpriteWorldPtr world);
#pragma cplusplus on
extern "C++" {
extern Rect gMovieBox;
}
#pragma cplusplus reset
extern void StartLoopingMovie(const unsigned char *name, void *window,
                              Rect *box);
extern void PreloadPortDialog(SpobRecord *spob, short stellar);

#ifdef __cplusplus
}
#endif

#endif
