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

#ifdef MACBUILD
#define CopyRect(d, s) (*(d) = *(s))
#else
static __inline void CopyRect(Rect *d, const Rect *s)
{
    d->top = s->top;
    d->left = s->left;
    d->bottom = s->bottom;
    d->right = s->right;
}
#endif

extern void DeltaIntroProgressBar(double amount);
extern Boolean AreWeFrontProcess(void);
extern void CenterRect(Rect *r, Rect *bounds);
#ifdef MACBUILD
extern void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, GDHandle device,
                          short depth, short picID);
#else
extern void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, int32_t flags,
                          short depth, short picID);
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern SpriteRec *weapTargetSprite[4];
extern SpriteRec *navTargetSprite[4];
extern SpriteRec *masterExplodSprite[64];
extern SpriteRec *masterSmokeSprite[8];
extern SpriteRec *masterBoxSprite[5];
extern Rect statusOffRect[8];
extern unsigned char targKeyOnNow;
extern ShipClassID lastTargetedShipClass;
extern unsigned char lastDebugTargInfoState;
extern Boolean gUpdateFlag;
extern Boolean regNoticeShown;
EXTERN_CPP_END
#pragma cplusplus reset
extern void DisposeMainScreenAnim(void);
#ifdef MACBUILD
extern void CreateGWorldContainer(GWorldContainer *gwc, GDHandle device, short depth,
                                  Rect *bounds);
#else
extern void CreateGWorldContainer(GWorldContainer *gwc, int32_t flags, short depth,
                                  Rect *bounds);
#endif
extern void DisposeGWorldContainer(GWorldContainer *gwc);
extern void DoMainScreen(void);
#ifdef MACBUILD
extern Boolean RetrieveMenuIcon(short picID, CGrafPtr dest, Rect *destRect);
#else
extern Boolean RetrieveMenuIcon(short picID, GWorldContainer *dest,
                           Rect *destRect);
#endif
extern void DrawIndString(short strListID, short index);
extern void DrawIntroProgressBar(void);
extern void DrawStatus(void);
extern void DrawStatusCargo(void);
extern void DrawStatusFuel(void);
extern void DrawStatusNav(void);
extern void DrawStatusRadar(Boolean highlight);
extern void DrawStatusShield(void);
extern void DrawStatusTarg(void);
extern void DrawStatusWeap(void);
extern void DrawStringCentered(const unsigned char *str, short left,
                               short right, short v);
#ifdef MACBUILD
/* the mac engine draws centred text with its own DrawCenteredText (Util.o) */
extern void DrawCenteredText(StringPtr s, short left, short right, short y);
#define DrawStringCentered(str, left, right, v) DrawCenteredText(str, left, right, v)
#endif
extern void EndLoopingMovie(void);
extern void FloaterEraseCallback(void *a);
extern void SetupListRects(Point *where);
extern void InitOffscreen(void);
extern void HideAmbrosiaLogo(void);
#ifdef MAC1010
extern void PostAnimateHandler(void);
#else
extern int PostAnimateHandler(void);
#endif
#ifdef V111
extern unsigned char useLegacyRenderingPipeline;
#endif
#ifdef MACBUILD
extern RgnHandle tempRgn;
#endif
extern Boolean GetSpriteInfo(short id, short *spriteID, short *maskID, short *xSize,
                          short *ySize, short *xTiles, short *yTiles);
extern short GetStellarSpriteXSize(short id);
extern void HandleStellarSprites(void);
extern void HideTheCursor(void);
extern void InitColors(void);
extern void InitIntroProgressBar(void);
extern short IntroMovieIdleProc(void);
extern int KeyboardGet(void);
extern short LoadExtendedShipSprites(short shipType);
extern void LoadIntfResource(void);
extern void LoadMainScreenAnim(void);
extern void MainScreenAppear(void);
extern void LoadMasterStellarSprite(short type);
extern void LoadShipSprites(void);
extern void LoadSprites(void);
extern void LoopingMovieIdle(void);
extern void MainScreenAnimDraw(void);
extern void MainScreenChoice(short which);
extern void MainScreenAppearStart(void);
#ifdef MACBUILD
void MainScreenKey(char key, unsigned char code, short modifiers);
#else
void MainScreenKey(char key, int32_t a, int32_t b);
#endif
extern void MainScreenRolloverDraw(void);
extern void MainScreenUpdate(short which);
extern int MouseReadButtons(void);
extern void MouseReadPosition(int32_t *h, int32_t *v);
extern void PlayMainScreenClickSound(void);
extern void PrepareAllSprites(void);
extern void SetGWorldContainer(GWorldContainer *gwc);
extern void SetupInterfaceBackground(void);
extern void ShowAmbrosiaLogo(void);
extern void ShowIntroScreen(void);
extern void ShowSplashScreen(void);
extern void DisposeMainScreenSounds(void);
extern void MainScreenAnimIdle(void);
extern void MainScreenRolloverIdle(void);
extern void TradeDialogTabKey(Boolean up);
extern void UnsetGWorldContainer(GWorldContainer *gwc);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern RGBColor *armorStatusColor;
extern Rect commRect;
extern Rect escortMenuRect;
extern Rect floatingMapRect;
extern RGBColor *fuelStatusColor[2];
extern AnimDelta slideFrameSize[3];
extern short slideFrameCount[];
extern short slideIndex[];
extern GWorldContainer logoPort;
extern GWorldContainer slidePort[3];
extern uint32_t lastRolloverAnimTime;
extern short rolloverAlpha;
extern short rolloverIndex;
extern Handle intfFileHand;
extern void *gIntroMovieData;
#ifndef MACBUILD
EXTERN_CPP_END
#pragma cplusplus reset
extern void QTNewGWorldFromPtr(void **gw, int32_t pixelFormat, Rect *boundsRect,
                               int32_t cTable, int32_t aGDevice, int32_t flags, int32_t baseAddr,
                               int32_t rowBytes);
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern uint32_t lastLogoAnimTime;
extern uint32_t lastSlideAnimTime;
extern Rect gLoadBarBase;
extern short logoFrameCount;
extern short logoIndex;
#ifdef MACBUILD
extern AnimDelta logoFrameSize;
#else
extern short gMainPicHeightDelta;
extern short gMainPicWidthDelta;
#endif
extern short loopingMovieResRefNum;
extern short planetPicId;
#ifdef MAC1010
extern uint32_t reggieStarted;
#else
extern int32_t reggieStarted;
#endif
extern unsigned char weapName[256][256];
extern Rect interfacePicRect;
extern short interferenceThisFrame;
extern short lastIntfResourceID;
extern int32_t lastFPSUpdateTime;
extern uint32_t lastRadarUpdateTime;
extern short lastTargetedShipShields;
#ifdef MACBUILD /* rows of three shorts: Files.o's stores keep their decl, RGBColor's fields lose it */
extern unsigned short ldefColors[3][3];
#else
extern RGBColor ldefColors[3];
#endif
extern ShortPoint logoDestPoint;
extern Rect logoDestRect;
extern ShortPoint mainMenuCenter;
extern Rect listPicOffRect[];
extern Boolean preventStatusCopy;
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
extern Boolean showingIntroText;
extern ShortPoint slideDestPoint[3];
extern short smallTextFont;
extern short smallTextSize;
extern RGBColor *statusBrightColor;
extern RGBColor statusColor[8];
extern RGBColor *statusDimColor;
extern short statusFontID;
extern GWorldContainer statusOffPort;
extern uint32_t tix;

extern void *loopingMovie;

extern void *mainScreenSnd[4];
extern Rect slideDestRect[3];
extern void *staticPat[10];
extern void *targetInfoPic[768];
EXTERN_CPP_END
#pragma cplusplus reset

extern void RefreshSAT(void);
extern void RedrawStatus(Boolean legacyPipeline);
extern void ShowTheCursor(void);
extern short GetMasterShipSpriteXSize(short id);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short gSoundVolume;

extern short statusFontSize[2];
EXTERN_CPP_END
#pragma cplusplus reset
extern void DoPlayGameWork(unsigned char advance);
extern void HandleSpriteMurkiness(short x, short y, SpriteRec *sprite);
extern void FloaterDrawCallback(SpriteWorldPtr world);
extern void BeamEraseCallback(SpriteWorldPtr world);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Rect gMovieBox;
EXTERN_CPP_END
#pragma cplusplus reset
extern void StartLoopingMovie(const unsigned char *name, void *window,
                              Rect *box);
extern void PreloadPortDialog(SpobRecord *spob, short stellar);

#ifdef __cplusplus
}
#endif

#endif
