// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
// COMPILE-WHY: two scoped `#pragma opt_repositioncode off' regions (pragma-only, no -opt
// spelling), over SetupMonitor and over ConvertSpriteToRLE; each matches only with it.
/* setup.c --tu-map object O26, the original Setup.c: SpriteWorld and sprite setup, loading, cleanup and game-state reset, then the QuickDraw state shims. Notes: Research/notes/setup.md */
#if defined(MACBUILD) && V111
#define IMPLICIT_RESOURCEEXISTS
#define IMPLICIT_SWGWORLDS
#endif
#include <math.h>
#include <stddef.h>

#include "burgerplat.h"
#include "macport.h"
#include "burger.h"
#include "game.h"
#include "render.h"
#include "stellar.h"
#include "crashlog.h"
#include "spriteworld.h"
#include "dialogs.h"
#include "cefix.h"
#include "shipobj.h"
#include "resources.h"
#include "space.h"
#include "gametypes.h"
#include "gameconst.h"
#include "ai.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "ships.h"
#include "sound.h"
#ifdef MACBUILD
extern void CancelMusicThread(void);
extern void CarbonHackCleanup(void);
extern FSRef *novaSupportFolder;
extern short startupDepth;
extern short oldDepth;
extern int32_t gSystemVersion;
extern double userSpeedSetting;
extern void InitCluts(void);
extern void BlitPixieDitherInit(void);
extern int32_t QDGetNativeWindowFromPort(CGrafPtr port);
extern int _CGSDefaultConnection(void);
extern int CGSSetWindowDepthLimit(int cid, int window, int depth);
extern int CGSSetWindowDepth(int cid, int window, int depth);
#endif

/* the resource type names in the log lines: MacRoman in the 1.0.10 text, UTF-8 in 1.1.1's */
#if V111 /* V111: the log text carries UTF-8, 1.0.10's is MacRoman */
#define kSpinTypeName "sp\xc3\xafn"
#define kShanTypeName "sh\xc3\xa4n"
#else
#define kSpinTypeName "sp\x95n"
#define kShanTypeName "sh\x8an"
#endif

#define kMaxRoidTypes    16

#ifdef MACBUILD
/* SetupToolbox (mac _SetupToolbox, mac-only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _SetupToolbox i386 0x0001b389 ppc 0x0001f3f8
void SetupToolbox(void)
{
    short i;

#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
#endif
    InitCursor();
    FlushEvents(everyEvent, 0);
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    MaxApplZone();
    for (i = 0; i < 6; i++)
        MoreMasters();
#elif defined(MAC1010)
    for (i = 0; i < 6; i++)
        MoreMasterPointers(64);
#else
    for (i = 0; i < 6; i++)
        MoreMasterPointers(128);
#endif
    Rand(0);
}

#endif
#if V111
/* DestroySpriteWorld (mac _DestroySpriteWorld, mac-only: the middle of the 1.1.1 Bye, which
   keeps its own copy). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _DestroySpriteWorld i386 0x000240a7 ppc 0x00028450
void DestroySpriteWorld(void)
{
    int32_t i;
    int32_t j;

    SetPort(GetCWindowGrafPort(gWind));
    DisposeButtons();
    ShowTheMenuBar();

    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].sprite)
            DisposeClonedSprite(&shipData[i].sprite);
        if (shipData[i].altSprite)
            DisposeClonedSprite(&shipData[i].altSprite);
        if (shipData[i].glowSprite)
            DisposeClonedSprite(&shipData[i].glowSprite);
        if (shipData[i].lightSprite)
            DisposeClonedSprite(&shipData[i].lightSprite);
        if (shipData[i].weapSprite)
            DisposeClonedSprite(&shipData[i].weapSprite);
        if (shipData[i].shieldSprite)
            DisposeClonedSprite(&shipData[i].shieldSprite);
    }
    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].sprite)
            DisposeClonedSprite(&podData[i].sprite);
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        if (masterShipSprite[i])
            SWDisposeSprite(&masterShipSprite[i]);
        if (masterShipGlowSprite[i])
            SWDisposeSprite(&masterShipGlowSprite[i]);
        if (masterShipLightSprite[i])
            SWDisposeSprite(&masterShipLightSprite[i]);
        if (masterShipWeaponSprite[i])
            SWDisposeSprite(&masterShipWeaponSprite[i]);
        if (masterShipAltSprite[i])
            SWDisposeSprite(&masterShipAltSprite[i]);
        if (masterShipShieldSprite[i])
            SWDisposeSprite(&masterShipShieldSprite[i]);
        if (shipClass[i].frameAngles != NULL)
            DeallocAPointer(shipClass[i].frameAngles);
    }
    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] != NULL)
            DeallocAPointer(stellarSprite[i]);
    }
    for (i = 0; i < kMasterStellarSprites; i++) {
        if (masterStellarSprite[i] != NULL) {
            if (masterStellarSprite[i]->frameArray != NULL) {
                for (j = 0; j < masterStellarSprite[i]->numFrames; j++) {
                    DeallocAPointer(masterStellarSprite[i]->frameArray[j]);
                    masterStellarSprite[i]->frameArray[j] = NULL;
                }
                masterStellarSprite[i]->numFrames = 0;
                DeallocAPointer(masterStellarSprite[i]->frameArray);
                masterStellarSprite[i]->frameArray = NULL;
            }
            SWDisposeSprite(&masterStellarSprite[i]);
        }
    }
    for (i = 0; i < kMaxExplods; i++) {
        if (explodData[i].sprite)
            DisposeClonedSprite(&explodData[i].sprite);
    }
    for (i = 0; i < kMaxExplodTypes; i++) {
        if (masterExplodSprite[i])
            SWDisposeSprite(&masterExplodSprite[i]);
    }
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].sprite)
            DisposeClonedSprite(&shotData[i].sprite);
    }
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (masterShotSprite[i])
            SWDisposeSprite(&masterShotSprite[i]);
    }
    for (i = 0; i < kMaxDebris; i++) {
        if (smokeData[i].sprite)
            DisposeClonedSprite(&smokeData[i].sprite);
    }
    for (i = 0; i < 8; i++) {
        if (masterSmokeSprite[i])
            SWDisposeSprite(&masterSmokeSprite[i]);
    }
    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].sprite)
            DisposeClonedSprite(&asteroidData[i].sprite);
    }
    for (i = 0; i < kMaxRoidTypes; i++) {
        if (masterAsteroidSprite[i])
            SWDisposeSprite(&masterAsteroidSprite[i]);
    }
    for (i = 0; i < kMainButtons; i++) {
        if (menuButtonSprite[i])
            SWDisposeSprite(&menuButtonSprite[i]);
    }
    if (menuRolloverSprite != NULL)
        SWDisposeSprite(&menuRolloverSprite);
    for (i = 0; i < kMaxBoxes; i++) {
        if (boxData[i].sprite)
            DisposeClonedSprite(&boxData[i].sprite);
    }
    for (i = 0; i < 5; i++) {
        if (masterBoxSprite[i])
            SWDisposeSprite(&masterBoxSprite[i]);
    }
    for (i = 0; i < kMaxStars; i++) {
        if (starData[i].sprite)
            DisposeClonedSprite(&starData[i].sprite);
    }
    if (masterStarSprite != NULL)
        SWDisposeSprite(&masterStarSprite);
    if (alertSprite != NULL)
        SWDisposeSprite(&alertSprite);
    if (doubleTimeSprite != NULL)
        SWDisposeSprite(&doubleTimeSprite);
    if (cursorSprite != NULL)
        SWDisposeSprite(&cursorSprite);
    for (i = 0; i < kMaxNavTargets; i++) {
        if (navTargetSprite[i])
            DisposeClonedSprite(&navTargetSprite[i]);
    }
    for (i = 0; i < kMaxNavTargets; i++) {
        if (weapTargetSprite[i])
            DisposeClonedSprite(&weapTargetSprite[i]);
    }
    if (masterNavTargetSprite != NULL)
        SWDisposeSprite(&masterNavTargetSprite);
    if (masterWeapTargetSprite != NULL)
        SWDisposeSprite(&masterWeapTargetSprite);
    if (redMapArrow != NULL)
        DisposeCIcon(redMapArrow);
    if (greenMapArrow != NULL)
        DisposeCIcon(greenMapArrow);

    DisposeParticles();
    SWDisposeFrame(&floatingMapFrame);
    SWDisposeFrame(&floatingEscortMenuFrame);

    if (gSpriteWorld->pixelDepth == 8)
        Dispose8BitAlphaTable();
    for (i = 0; i < 0xa; i++) {
        if (staticPat[i] != NULL)
            DisposePixPat((Image_t *)staticPat[i]);
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        if (targetInfoPic[i] != NULL) {
            QDReleaseResource((PicHandle)targetInfoPic[i]);
            targetInfoPic[i] = NULL;
        }
    }

    if (shipLayer != NULL)
        SWDisposeSpriteLayer(&shipLayer);
    if (stellarLayer != NULL)
        SWDisposeSpriteLayer(&stellarLayer);
    if (explodLayer != NULL)
        SWDisposeSpriteLayer(&explodLayer);
    if (shotLayer != NULL)
        SWDisposeSpriteLayer(&shotLayer);
    if (guidedShotLayer != NULL)
        SWDisposeSpriteLayer(&guidedShotLayer);
    if (pointDefenseLayer != NULL)
        SWDisposeSpriteLayer(&pointDefenseLayer);
    if (targetLayer != NULL)
        SWDisposeSpriteLayer(&targetLayer);
    if (asteroidLayer != NULL)
        SWDisposeSpriteLayer(&asteroidLayer);
    if (smokeLayer != NULL)
        SWDisposeSpriteLayer(&smokeLayer);
    if (disabledShipLayer != NULL)
        SWDisposeSpriteLayer(&disabledShipLayer);
    if (escortLayer != NULL)
        SWDisposeSpriteLayer(&escortLayer);
    if (boxLayer != NULL)
        SWDisposeSpriteLayer(&boxLayer);
    if (starLayer != NULL)
        SWDisposeSpriteLayer(&starLayer);
    if (turretedShotLayer != NULL)
        SWDisposeSpriteLayer(&turretedShotLayer);
    if (gSpriteWorld != NULL) {
        if (gSpriteWorld->deadSpriteLayerP != NULL)
            SWDisposeSpriteLayer(&gSpriteWorld->deadSpriteLayerP);
        SWDisposeFrame(&gSpriteWorld->backFrameP);
        SWDisposeFrame(&gSpriteWorld->workFrameP);
        SWDisposeFrame(&gSpriteWorld->extraBackFrameP);
    }

    if (gSpriteWorld != NULL)
        DeallocAPointer(gSpriteWorld);
    SWExitSpriteWorld();

    DisposeGWorldContainer(&loadPort);
    DisposeGWorldContainer(&statusOffPort);
#ifdef MACBUILD
    if (tempRgn != NULL)
        DisposeRgn(tempRgn);
#endif

    DeallocAPointer(shipData);
    DeallocAPointer(shotData);
    DeallocAPointer(govtData);
    DeallocAPointer(dudeData);
    DeallocAPointer(beamData);
    DeallocAPointer(stellarData);
    DeallocAPointer(systemData);
    DeallocAPointer(shipClass);
    DeallocAPointer(weaponStats);
    DeallocAPointer(explodData);
    DeallocAPointer(explodType);
    DeallocAPointer(smokeData);
    DeallocAPointer(outfitSystem);
    DeallocAPointer(personData);
    DeallocAPointer(asteroidData);
    DeallocAPointer(disasterData);
    DeallocAPointer(nebulaData);
    DeallocAPointer(fleetData);
    DeallocAPointer(boxData);
    DeallocAPointer(podData);
    DeallocAPointer(junkData);
    DeallocAPointer(cronEvent);
    DeallocAPointer(rankData);
    DeallocAPointer(starData);
    DeallocAPointer(missionBit);
    DeallocAPointer(currentMissions);
    DeallocAPointer(currentMissionObjectives);
}
#endif

#ifdef MACBUILD
#if V111
/* changeWindowModes (mac _changeWindowModes, mac-only; CheckSystem repeats its body). STATE: mac
   MATCH i386 and ppc. OPEN: none */
// MAC: _changeWindowModes i386 0x0001b402 ppc 0x0001f480
void changeWindowModes(void)
{
    CGRect bounds;

    bounds = CGDisplayBounds(CGMainDisplayID());
    mainWindRect.top = bounds.origin.x;
    mainWindRect.left = bounds.origin.y;
    mainWindRect.bottom = bounds.size.height;
    mainWindRect.right = bounds.size.width;
}
#endif

#endif
/* SetupSpriteWorld (mac _SetupSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab980
// MAC: _SetupSpriteWorld i386 0x00021b3a ppc 0x00025fac
void SetupSpriteWorld(void)
{
    short i;
    short err;
    Rect *frameRect;
    Str255 str;
    Rect windRect;
    RGBColor color;

    mBarHidden = 1;
    windRect = mainWindRect;
#ifdef CEFIX /* F14.18 */
    windRect.right -= ceStatusBarWidth;
#else
    windRect.right -= kTargPanelInset;
#endif
    SetPortWindowPort((WindowRef)gWind);
    err = SWCreateSpriteWorldFromWindow(&gSpriteWorld, gWind, &windRect, &windRect, 0);
    if ((int32_t)err != 0) {
        FadeScreenIn();
        GetIndString(str, kAlertStrList, 36);
        EVNoteAlert(str);
        ST_Close();
        ExitToShell();
    }
#if V111
    if (useLegacyRenderingPipeline) {
        if (gSpriteWorld->pixelDepth != 8 && gSpriteWorld->pixelDepth != 16) {
            FadeScreenIn();
            GetIndString(str, kAlertStrList, 37);
            EVNoteAlert(str);
            ST_Close();
            ExitToShell();
        }
    }
#else
    if (gSpriteWorld->pixelDepth != 8 && gSpriteWorld->pixelDepth != 16) {
        FadeScreenIn();
        GetIndString(str, kAlertStrList, 37);
        EVNoteAlert(str);
        ST_Close();
        ExitToShell();
    }
#endif

    SWCreateSpriteLayer(&starLayer);
    SWCreateSpriteLayer(&stellarLayer);
    SWCreateSpriteLayer(&asteroidLayer);
    SWCreateSpriteLayer(&disabledShipLayer);
    SWCreateSpriteLayer(&boxLayer);
    SWCreateSpriteLayer(&smokeLayer);
    SWCreateSpriteLayer(&shotLayer);
    SWCreateSpriteLayer(&guidedShotLayer);
    SWCreateSpriteLayer(&pointDefenseLayer);
    SWCreateSpriteLayer(&escortLayer);
    SWCreateSpriteLayer(&shipLayer);
    SWCreateSpriteLayer(&turretedShotLayer);
    SWCreateSpriteLayer(&explodLayer);
    SWCreateSpriteLayer(&targetLayer);

    SWAddSpriteLayer(gSpriteWorld, starLayer);
    SWAddSpriteLayer(gSpriteWorld, stellarLayer);
    SWAddSpriteLayer(gSpriteWorld, asteroidLayer);
    SWAddSpriteLayer(gSpriteWorld, disabledShipLayer);
    SWAddSpriteLayer(gSpriteWorld, boxLayer);
    SWAddSpriteLayer(gSpriteWorld, smokeLayer);
    SWAddSpriteLayer(gSpriteWorld, pointDefenseLayer);
    SWAddSpriteLayer(gSpriteWorld, guidedShotLayer);
    SWAddSpriteLayer(gSpriteWorld, shotLayer);
    SWAddSpriteLayer(gSpriteWorld, escortLayer);
    SWAddSpriteLayer(gSpriteWorld, shipLayer);
    SWAddSpriteLayer(gSpriteWorld, turretedShotLayer);
    SWAddSpriteLayer(gSpriteWorld, explodLayer);
    SWAddSpriteLayer(gSpriteWorld, targetLayer);

    SWSetLayerCallBack(stellarLayer, BeamDrawOffscreenCallback);
    SWSetLayerCallBack(targetLayer, BeamDrawCallback);
    SWSetSpriteWorldMaxFPS(gSpriteWorld, 0);
#if defined(MACBUILD) && (!defined(MAC1010) || TARGET_API_MAC_CARBON) /* the Carbon screen blitter */
    SWSetSpriteWorldScreenDrawProc(gSpriteWorld, SWCopyBitsDrawProc);
#endif

    crashlog("creating scratch frames");
    windRect.top = 0;
    windRect.left = 0;
    windRect.right = ((mainWindRect.right - mainWindRect.left) * 0.25);
    if (windRect.right < 200)
        windRect.right = 200;
    windRect.bottom = windRect.right;
    SWCreateFrame(GetGDevice(), &floatingMapFrame, &windRect, gSpriteWorld->pixelDepth, 1);
    SWLockFrame(floatingMapFrame);
    floatingMapRect = windRect;
#ifdef CEFIX /* F14.19 */
    OffsetRect(&floatingMapRect,
            GetCWindowPortRect(gWind)->right - GetCWindowPortRect(gWind)->left
            - (windRect.right - windRect.left + ceStatusBarWidth + 16), 150);
#else
    OffsetRect(&floatingMapRect,
            GetCWindowPortRect((WindowRef)gWind)->right - GetCWindowPortRect((WindowRef)gWind)->left
            - (windRect.right - windRect.left + 210), 150);
#endif
#ifdef CEFIX /* F12.79 */
    SetRect(&windRect, 0, 0, CE_Scale(160), CE_Scale(120));
#else
    SetRect(&windRect, 0, 0, 160, 120);
#endif
    SWCreateFrame(GetGDevice(), &floatingEscortMenuFrame, &windRect, gSpriteWorld->pixelDepth, 1);
    SWLockFrame(floatingEscortMenuFrame);
    escortMenuRect = floatingEscortMenuFrame->frameRect;
    OffsetRect(&escortMenuRect, -escortMenuRect.left, -escortMenuRect.top);
    OffsetRect(&escortMenuRect, 15, 150);

    color.red = color.green = color.blue = 0;
    SWSetBackgroundColor(gSpriteWorld, &color);
#ifdef MACBUILD
    BlitPixieDitherInit();
#endif
    SWSetPostEraseCallBack(gSpriteWorld, PostEraseCallback);
    SWSetPostDrawCallBack(gSpriteWorld, PostDrawCallback);
    SWSetSpriteWorldMaxFPS(gSpriteWorld, 0);
    SWLockSpriteWorld(gSpriteWorld);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));

    spriteClipRect = windRect;
    spriteClipRect.top -= 75;
    spriteClipRect.left -= 75;

    for (i = 0; i < kMaxShipTypes; i++) {
        masterShipSprite[i] = 0;
        masterShipGlowSprite[i] = 0;
        masterShipLightSprite[i] = 0;
        masterShipWeaponSprite[i] = 0;
        masterShipAltSprite[i] = 0;
        masterShipShieldSprite[i] = 0;
    }
    for (i = 0; i < kMasterStellarSprites; i++)
        masterStellarSprite[i] = 0;
    for (i = 0; i < kMaxExplodTypes; i++)
        masterExplodSprite[i] = 0;
    for (i = 0; i < kMaxWeapTypes; i++)
        masterShotSprite[i] = 0;

#if V111
    err = InitParticles(1024, 0);
    if (err != 0)
        EV_ASSERT_FAIL("Setup.c", 0x133, 583);
#else
#ifdef CEFIX /* F25.01 */
    if ((int32_t)InitParticles(100000, 0) != 0)
        SWAssertFail("Setup.c", SW_LINE(0x133));
#else
    if ((int32_t)InitParticles(1024, 0) != 0)
        SWAssertFail("Setup.c", SW_LINE(0x133));
#endif
#endif
    SWSetPortToBackground(gSpriteWorld);
    if (gSpriteWorld->pixelDepth == 8) {
        err = Construct8BitAlphaTable();
        if ((int32_t)err != 0) {
            crashlog_print("error loading color table, ID ");
            crashlog_print_num(128);
            crashlog_print("\n");
            ReportSpriteLoadError(6, 128, err);
            ExitToShell();
        }
    }
}

/* SetupWind (mac _SetupWind). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ac380
// MAC: _SetupWind i386 0x0001b462 ppc 0x0001f524
void SetupWind(void)
{
#ifdef MACBUILD
    Rect bounds;
#ifndef MAC1010
    int32_t window;
#elif TARGET_API_MAC_CARBON
    short width, height;
#endif

    fullScreenRestore = 0;
    if (settings.windowed) {
        bounds = mainWindRect;
        if (mainWindRect.right - mainWindRect.left > 832
                && mainWindRect.bottom - mainWindRect.top > 624)
            SetRect(&mainWindRect, 0, 0, 832, 604);
        else if (mainWindRect.right - mainWindRect.left > 800
                && mainWindRect.bottom - mainWindRect.top > 600)
            SetRect(&mainWindRect, 0, 0, 800, 580);
        else {
            mainWindRect.left += 5;
            mainWindRect.right -= 5;
            mainWindRect.top += 45;
            mainWindRect.bottom -= 5;
        }
        bounds.top += 20;
        CenterRect(&mainWindRect, &bounds);
        if (mainWindRect.bottom - mainWindRect.top < 600)
            OffsetRect(&mainWindRect, 0, 8);
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        gWind = (CWindowPtr)NewCWindow(NULL, &mainWindRect, "\pEV Nova", false, documentProc, NULL, true, -1);
#else
        CreateNewWindow(kDocumentWindowClass, kWindowCloseBoxAttribute, &mainWindRect, &gWind);
#ifndef MAC1010
        window = QDGetNativeWindowFromPort(GetWindowPort(gWind));
        CGSSetWindowDepthLimit(_CGSDefaultConnection(), window, 3);
        CGSSetWindowDepth(_CGSDefaultConnection(), window, 3);
#endif
        SetWTitle(gWind, "\pEV Nova");
#endif
        OffsetRect(&mainWindRect, -mainWindRect.left, -mainWindRect.top);
    } else {
#ifndef MAC1010
        bounds = mainWindRect;
        CreateNewWindow(kPlainWindowClass, kWindowNoAttributes, &mainWindRect, &gWind);
        window = QDGetNativeWindowFromPort(GetWindowPort(gWind));
        CGSSetWindowDepthLimit(_CGSDefaultConnection(), window, 3);
        CGSSetWindowDepth(_CGSDefaultConnection(), window, 3);
#elif TARGET_API_MAC_CARBON
        CreateNewWindow(kPlainWindowClass, kWindowNoAttributes, &mainWindRect, &gWind);
        height = width = 0;
        BeginFullScreen((Ptr *)&fullScreenRestore, NULL, &width, &height, NULL, &rgbBlack, 0);
#else
        gWind = (CWindowPtr)NewCWindow(NULL, &mainWindRect, "\pEV Nova", false, plainDBox, NULL, true, -1);
#endif
    }
#ifdef MAC1010
    crashlog_print("main window rect: ("); /* BUG: `bounds' is unset when full screen */
    crashlog_print_num(bounds.left);
    crashlog_print(", ");
    crashlog_print_num(bounds.top);
    crashlog_print(", ");
    crashlog_print_num(bounds.right);
    crashlog_print(", ");
    crashlog_print_num(bounds.bottom);
    crashlog_print(")\n");
#endif
    ShowWindowNoFlash(gWind);
    SetPort((GrafPtr)GetCWindowGrafPort((WindowRef)gWind));
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
    SelectWindow((WindowRef)gWind);
    BringToFront((WindowRef)gWind);
    if (!useLegacyRenderingPipeline)
        QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
    if (settings.windowed == 0) {
        HideTheMenuBar();
        mBarHidden = 1;
    }
    screenCenter.h = ((GetCWindowPortRect((WindowRef)gWind)->right
                             - GetCWindowPortRect((WindowRef)gWind)->left - kTargPanelInset) * 0.5);
    screenCenter.v = ((GetCWindowPortRect((WindowRef)gWind)->bottom
                             - GetCWindowPortRect((WindowRef)gWind)->top) * 0.5);
#else
    short top;
    short left;
    short bottom;
    short right;

    fullScreenRestore = 0;
    gWind = OpenGamePort(&mainWindRect, "\pEV Nova");
    top = mainWindRect.top;
    left = mainWindRect.left;
    bottom = mainWindRect.bottom;
    right = mainWindRect.right;
    crashlog_print("main window rect: (");
    crashlog_print_num(left);
    crashlog_print(", ");
    crashlog_print_num(top);
    crashlog_print(", ");
    crashlog_print_num(right);
    crashlog_print(", ");
    crashlog_print_num(bottom);
    crashlog_print(")\n");
    ShowWindowNoFlash(gWind);
    SetPortWindowPort(gWind);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    if (settings.windowed == 0) {
        HideTheMenuBar();
        mBarHidden = 1;
    }
#ifdef CEFIX /* F14.20 */
    screenCenter.h = ((GetCWindowPortRect(gWind)->right
                             - GetCWindowPortRect(gWind)->left - ceStatusBarWidth) * 0.5);
#else
    screenCenter.h = ((GetCWindowPortRect(gWind)->right
                             - GetCWindowPortRect(gWind)->left - kTargPanelInset) * 0.5);
#endif
    screenCenter.v = ((GetCWindowPortRect(gWind)->bottom
                             - GetCWindowPortRect(gWind)->top) * 0.5);
#endif
}

#ifdef MAC1010 /* 1.0.10 spelling */
#define AllocFailed(var, expr) (var = (expr), !var)
#else
#define AllocFailed(var, expr) (!(var = (expr)))
#endif

/* InitArrays (mac _InitArrays). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ac560
// MAC: _InitArrays i386 0x0001b796 ppc 0x0001f8b4
void InitArrays(void)
{
#ifdef MACBUILD
    Str255 msg;
    Str255 num;
#endif

    if (AllocFailed(shipData, (ShipObj *)AllocAPointer(sizeof(ShipObj) * kMaxShips)))
        EV_ASSERT_FAIL("Setup.c", 0x1b6, 747);
    if (AllocFailed(stellarData, (SpobRecord *)AllocAPointer(0x24c000)))
        EV_ASSERT_FAIL("Setup.c", 0x1b7, 748);
    if (AllocFailed(systemData, (SystRecord *)AllocAPointer(0xfe000)))
        EV_ASSERT_FAIL("Setup.c", 0x1b8, 749);
    if (AllocFailed(shipClass, (ShipRec *)AllocAPointer(0x203400)))
        EV_ASSERT_FAIL("Setup.c", 0x1b9, 750);
    if (AllocFailed(weaponStats, (WeapRecord *)AllocAPointer(0xc800)))
        EV_ASSERT_FAIL("Setup.c", 0x1ba, 751);
    if (AllocFailed(explodData, (BoomSlot *)AllocAPointer(0x300)))
        EV_ASSERT_FAIL("Setup.c", 0x1bb, 752);
    if (AllocFailed(explodType, (BoomRecord *)AllocAPointer(0x200)))
        EV_ASSERT_FAIL("Setup.c", 0x1bc, 753);
    if (AllocFailed(smokeData, (DebrisSlot *)AllocAPointer(0x600)))
        EV_ASSERT_FAIL("Setup.c", 0x1bd, 754);
    if (AllocFailed(beamData, (BeamRecord *)AllocAPointer(0x880)))
        EV_ASSERT_FAIL("Setup.c", 0x1be, 755);
    if (AllocFailed(outfitSystem, (OutfRecord *)AllocAPointer(0x6f800)))
        EV_ASSERT_FAIL("Setup.c", 0x1bf, 756);
    if (AllocFailed(dudeData, (DudeRecord *)AllocAPointer(0x9400)))
        EV_ASSERT_FAIL("Setup.c", 0x1c0, 757);
    if (AllocFailed(govtData, (GovtRecord *)AllocAPointer(0x8800)))
        EV_ASSERT_FAIL("Setup.c", 0x1c1, 758);
    if (AllocFailed(personData, (PersRecord *)AllocAPointer(0x1e5000)))
        EV_ASSERT_FAIL("Setup.c", 0x1c2, 759);
    if (AllocFailed(asteroidData, (AsteroidObj *)AllocAPointer(0x240)))
        EV_ASSERT_FAIL("Setup.c", 0x1c3, 760);
    if (AllocFailed(disasterData, (OopsRecord *)AllocAPointer(0x21000)))
        EV_ASSERT_FAIL("Setup.c", 0x1c4, 761);
    if (AllocFailed(nebulaData, (NebuRecord *)AllocAPointer(0x4100)))
        EV_ASSERT_FAIL("Setup.c", 0x1c5, 762);
    if (AllocFailed(fleetData, (FletRecord *)AllocAPointer(0x12400)))
        EV_ASSERT_FAIL("Setup.c", 0x1c6, 763);
    if (AllocFailed(boxData, (BoxObj *)AllocAPointer(0xa00)))
        EV_ASSERT_FAIL("Setup.c", 0x1c7, 764);
    if (AllocFailed(podData, (PodObj *)AllocAPointer(0x300)))
        EV_ASSERT_FAIL("Setup.c", 0x1c8, 765);
    if (AllocFailed(shotData, (ShotObj *)AllocAPointer(0x2400)))
        EV_ASSERT_FAIL("Setup.c", 0x1c9, 766);
    if (AllocFailed(junkData, (JunkRecord *)AllocAPointer(0x29300)))
        EV_ASSERT_FAIL("Setup.c", 0x1ca, 767);
    if (AllocFailed(cronEvent, (CronRecord *)AllocAPointer(0x6a000)))
        EV_ASSERT_FAIL("Setup.c", 0x1cb, 768);
    if (AllocFailed(rankData, (RankRecord *)AllocAPointer(0x9000)))
        EV_ASSERT_FAIL("Setup.c", 0x1cc, 769);
    if (AllocFailed(starData, (StarSlot *)AllocAPointer(sizeof(StarSlot) * kMaxStars)))
        EV_ASSERT_FAIL("Setup.c", 0x1cd, 770);
    if (AllocFailed(missionBit, (Boolean *)AllocAPointer(sizeof(Boolean) * kMaxMissionBits)))
        EV_ASSERT_FAIL("Setup.c", 0x1ce, 771);
    if (AllocFailed(currentMissions, (MissionRecord *)AllocAPointer(sizeof(MissionRecord) * kMaxActiveMissions)))
        EV_ASSERT_FAIL("Setup.c", 0x1cf, 772);
    if (AllocFailed(currentMissionObjectives, (MissionState *)AllocAPointer(0x140)))
        EV_ASSERT_FAIL("Setup.c", 0x1d0, 773);
#ifdef MACBUILD
    if (EVBugResourceFieldTest(4)) {
        strcpy((char *)msg, "After creating arrays, stackspace is ");
        NumToString(StackSpace(), num);
        PtoCstr(num);
        strcat((char *)msg, (char *)num);
        strcat((char *)msg, " bytes");
        CtoPstr((char *)msg);
        DebugStr(msg);
    }
#endif
}

/* InitOffscreen (mac _InitOffscreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ac950
// MAC: _InitOffscreen i386 0x0001bd21 ppc 0x0001fe24
void InitOffscreen(void)
{
    Rect *bounds;
    unsigned char msg[256];
    Rect r;

    bounds = GetCWindowPortRect((WindowRef)gWind);
    r = *bounds;
    OffsetRect(&r, -r.left, -r.top);
    CreateGWorldContainer(&loadPort, GetMainDevice(), 0, &r);
    if (loadPort.world == NULL) {
        HideWindow((WindowRef)gWind);
        ShowTheMenuBar();
        ShowTheCursor();
        FadeScreenIn();
        GetIndString(msg, kAlertStrList, 0x27);
        EVNoteAlert(msg);
        ST_Close();
        ExitToShell();
    }
#ifdef CEFIX /* F14.21 */
    r.right = r.left + ceStatusBarWidth;
#else
    r.right = r.left + kTargPanelInset;
#endif
    CreateGWorldContainer(&statusOffPort, GetMainDevice(), 0, &r);
    if (statusOffPort.world == NULL) {
        HideWindow((WindowRef)gWind);
        ShowTheMenuBar();
        ShowTheCursor();
        FadeScreenIn();
        GetIndString(msg, kAlertStrList, 0x27);
        EVNoteAlert(msg);
        ST_Close();
        ExitToShell();
    }
    SWSetPortToWindow(gSpriteWorld);
}

#define kVoiceClipsMax           9
#define kVoiceSetStride       0x64
#define kVoiceMsgStride        0xa
#define kFirstVoiceID        0x3e8
#define kLoopSounds              2
#define kDefaultLoopTicks    0x15e
#define kTicksPerSecond         60
#define kFirstEngineID        0x96
#define kEngineSounds            5
#define kFirstShotID          0xc8
#define kShotSounds          0x100
#define kFirstExplodID       0x12c
#define kExplodSounds         0x40
#define kHighPriority        32000

/* 1.1.1 logs no shutdown steps */
#if V111
#define ByeLog(s)
#else
#define ByeLog(s) crashlog(s)
#endif

/* Bye. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004acac0
// MAC: _Bye i386 0x00024b6b ppc 0x00029044
void Bye(void)
{
    short i;
    short j;
    void *snd;

    ByeLog("doing cleanup");
    DisposeIntroProgressBar();
    FlushEvents(everyEvent - diskMask, 0);
#if defined(MACBUILD) && V111
    CancelMusicThread();
#endif

    ByeLog("disposing sounds");
    ST_HaltSound(0);
    if (warpUpSnd[0] != NULL)
        ST_DisposeSound(warpUpSnd[0]);
    snd = warpUpSnd[1];
    if (snd != NULL)
        ST_DisposeSound(snd);
    if (warpOutSnd != NULL)
        ST_DisposeSound(warpOutSnd);
    for (i = 0; i <= kEngineSounds - 1; i++) {
        if (beepSnd[i] != NULL)
            ST_DisposeSound(beepSnd[i]);
    }
    for (i = 0; i < kShotSounds; i++) {
        if (shotSnd[i] != NULL)
            ST_DisposeSound(shotSnd[i]);
    }
    for (i = 0; i < kExplodSounds; i++) {
        if (explodSnd[i] != NULL)
            ST_DisposeSound(explodSnd[i]);
    }
    if (redAlertSnd != NULL)
        ST_DisposeSound(redAlertSnd);
    if (klaxxonSnd != NULL)
        ST_DisposeSound(klaxxonSnd);
    if (escapePodSnd != NULL)
        ST_DisposeSound(escapePodSnd);
    if (cloakSnd[0] != NULL)
        ST_DisposeSound(cloakSnd[0]);
    snd = cloakSnd[1];
    if (snd != NULL)
        ST_DisposeSound(snd);
    if (hardDockSnd != NULL)
        ST_DisposeSound(hardDockSnd);
    if (clickSnd != NULL)
        ST_DisposeSound(clickSnd);
    if (missionSnd != NULL)
        ST_DisposeSound(missionSnd);
    if (voiceSnd != NULL)
        ST_DisposeSound(voiceSnd);

    ByeLog("disposing buttons");
    SetPortWindowPort((WindowRef)gWind);
    DisposeButtons();
    ShowTheMenuBar();

    ByeLog("disposing sprites");
    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].sprite)
            DisposeClonedSprite(&shipData[i].sprite);
        if (shipData[i].altSprite)
            DisposeClonedSprite(&shipData[i].altSprite);
        if (shipData[i].glowSprite)
            DisposeClonedSprite(&shipData[i].glowSprite);
        if (shipData[i].lightSprite)
            DisposeClonedSprite(&shipData[i].lightSprite);
        if (shipData[i].weapSprite)
            DisposeClonedSprite(&shipData[i].weapSprite);
        if (shipData[i].shieldSprite)
            DisposeClonedSprite(&shipData[i].shieldSprite);
    }
    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].sprite)
            DisposeClonedSprite(&podData[i].sprite);
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        if (masterShipSprite[i])
            SWDisposeSprite(&masterShipSprite[i]);
        if (masterShipGlowSprite[i])
            SWDisposeSprite(&masterShipGlowSprite[i]);
        if (masterShipLightSprite[i])
            SWDisposeSprite(&masterShipLightSprite[i]);
        if (masterShipWeaponSprite[i])
            SWDisposeSprite(&masterShipWeaponSprite[i]);
        if (masterShipAltSprite[i])
            SWDisposeSprite(&masterShipAltSprite[i]);
        if (masterShipShieldSprite[i])
            SWDisposeSprite(&masterShipShieldSprite[i]);
        if (shipClass[i].frameAngles != NULL)
            DeallocAPointer(shipClass[i].frameAngles);
    }
    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] != NULL)
            DeallocAPointer(stellarSprite[i]);
    }
    for (i = 0; i < kMasterStellarSprites; i++) {
        if (masterStellarSprite[i] != NULL) {
            if (masterStellarSprite[i]->frameArray != NULL) {
                for (j = 0; j < masterStellarSprite[i]->numFrames; j++) {
                    DeallocAPointer(masterStellarSprite[i]->frameArray[j]);
                    masterStellarSprite[i]->frameArray[j] = NULL;
                }
                masterStellarSprite[i]->numFrames = 0;
                DeallocAPointer(masterStellarSprite[i]->frameArray);
                masterStellarSprite[i]->frameArray = NULL;
            }
            SWDisposeSprite(&masterStellarSprite[i]);
        }
    }
    for (i = 0; i < kMaxExplods; i++) {
        if (explodData[i].sprite)
            DisposeClonedSprite(&explodData[i].sprite);
    }
    for (i = 0; i < kMaxExplodTypes; i++) {
        if (masterExplodSprite[i])
            SWDisposeSprite(&masterExplodSprite[i]);
    }
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].sprite)
            DisposeClonedSprite(&shotData[i].sprite);
    }
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (masterShotSprite[i])
            SWDisposeSprite(&masterShotSprite[i]);
    }
    for (i = 0; i < kMaxDebris; i++) {
        if (smokeData[i].sprite)
            DisposeClonedSprite(&smokeData[i].sprite);
    }
    for (i = 0; i < 8; i++) {
        if (masterSmokeSprite[i])
            SWDisposeSprite(&masterSmokeSprite[i]);
    }
    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].sprite)
            DisposeClonedSprite(&asteroidData[i].sprite);
    }
    for (i = 0; i < kMaxRoidTypes; i++) {
        if (masterAsteroidSprite[i])
            SWDisposeSprite(&masterAsteroidSprite[i]);
    }
    for (i = 0; i < kMainButtons; i++) {
        if (menuButtonSprite[i])
            SWDisposeSprite(&menuButtonSprite[i]);
    }
    if (menuRolloverSprite != NULL)
        SWDisposeSprite(&menuRolloverSprite);
    for (i = 0; i < kMaxBoxes; i++) {
        if (boxData[i].sprite)
            DisposeClonedSprite(&boxData[i].sprite);
    }
    for (i = 0; i < 5; i++) {
        if (masterBoxSprite[i])
            SWDisposeSprite(&masterBoxSprite[i]);
    }
    for (i = 0; i < kMaxStars; i++) {
        if (starData[i].sprite)
            DisposeClonedSprite(&starData[i].sprite);
    }
    if (masterStarSprite != NULL)
        SWDisposeSprite(&masterStarSprite);
    if (alertSprite != NULL)
        SWDisposeSprite(&alertSprite);
    if (doubleTimeSprite != NULL)
        SWDisposeSprite(&doubleTimeSprite);
    if (cursorSprite != NULL)
        SWDisposeSprite(&cursorSprite);
    for (i = 0; i < kMaxNavTargets; i++) {
        if (navTargetSprite[i])
            DisposeClonedSprite(&navTargetSprite[i]);
    }
    for (i = 0; i < kMaxNavTargets; i++) {
        if (weapTargetSprite[i])
            DisposeClonedSprite(&weapTargetSprite[i]);
    }
    if (masterNavTargetSprite != NULL)
        SWDisposeSprite(&masterNavTargetSprite);
    if (masterWeapTargetSprite != NULL)
        SWDisposeSprite(&masterWeapTargetSprite);
    if (redMapArrow != NULL)
        DisposeCIcon((CIconHandle)redMapArrow);
    if (greenMapArrow != NULL)
        DisposeCIcon((CIconHandle)greenMapArrow);

    ByeLog("disposing particles");
    DisposeParticles();
    SWDisposeFrame(&floatingMapFrame);
    SWDisposeFrame(&floatingEscortMenuFrame);

    ByeLog("disposing patterns & picts");
    if (gSpriteWorld->pixelDepth == 8)
        Dispose8BitAlphaTable();
    for (i = 0; i < 0xa; i++) {
        if (staticPat[i] != NULL)
            DisposePixPat((PixPatHandle)staticPat[i]);
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        if (targetInfoPic[i] != NULL) {
            QDReleaseResource((PicHandle)targetInfoPic[i]);
            targetInfoPic[i] = NULL;
        }
    }

    ByeLog("disposing sprite layers");
    if (shipLayer != NULL)
        SWDisposeSpriteLayer(&shipLayer);
    if (stellarLayer != NULL)
        SWDisposeSpriteLayer(&stellarLayer);
    if (explodLayer != NULL)
        SWDisposeSpriteLayer(&explodLayer);
    if (shotLayer != NULL)
        SWDisposeSpriteLayer(&shotLayer);
    if (guidedShotLayer != NULL)
        SWDisposeSpriteLayer(&guidedShotLayer);
    if (pointDefenseLayer != NULL)
        SWDisposeSpriteLayer(&pointDefenseLayer);
    if (targetLayer != NULL)
        SWDisposeSpriteLayer(&targetLayer);
    if (asteroidLayer != NULL)
        SWDisposeSpriteLayer(&asteroidLayer);
    if (smokeLayer != NULL)
        SWDisposeSpriteLayer(&smokeLayer);
    if (disabledShipLayer != NULL)
        SWDisposeSpriteLayer(&disabledShipLayer);
    if (escortLayer != NULL)
        SWDisposeSpriteLayer(&escortLayer);
    if (boxLayer != NULL)
        SWDisposeSpriteLayer(&boxLayer);
    if (starLayer != NULL)
        SWDisposeSpriteLayer(&starLayer);
    if (turretedShotLayer != NULL)
        SWDisposeSpriteLayer(&turretedShotLayer);
    if (gSpriteWorld != NULL) {
        if (gSpriteWorld->deadSpriteLayerP != NULL)
            SWDisposeSpriteLayer(&gSpriteWorld->deadSpriteLayerP);
        SWDisposeFrame(&gSpriteWorld->backFrameP);
        SWDisposeFrame(&gSpriteWorld->workFrameP);
        SWDisposeFrame(&gSpriteWorld->extraBackFrameP);
#if !V111
        SWDisposeWindowFrame(&gSpriteWorld->windowFrameP);
#endif
    }

    ByeLog("disposing sprite world");
    if (gSpriteWorld != NULL)
        DeallocAPointer(gSpriteWorld);
    SWExitSpriteWorld();

    ByeLog("disposing offscreen buffers");
    DisposeGWorldContainer(&loadPort);
    DisposeGWorldContainer(&statusOffPort);
#ifdef MACBUILD
    if (tempRgn != NULL)
        DisposeRgn(tempRgn);
#endif

    ByeLog("disposing array pointers");
    DeallocAPointer(shipData);
    DeallocAPointer(shotData);
    DeallocAPointer(govtData);
    DeallocAPointer(dudeData);
    DeallocAPointer(beamData);
    DeallocAPointer(stellarData);
    DeallocAPointer(systemData);
    DeallocAPointer(shipClass);
    DeallocAPointer(weaponStats);
    DeallocAPointer(explodData);
    DeallocAPointer(explodType);
    DeallocAPointer(smokeData);
    DeallocAPointer(outfitSystem);
    DeallocAPointer(personData);
    DeallocAPointer(asteroidData);
    DeallocAPointer(disasterData);
    DeallocAPointer(nebulaData);
    DeallocAPointer(fleetData);
    DeallocAPointer(boxData);
    DeallocAPointer(podData);
    DeallocAPointer(junkData);
    DeallocAPointer(cronEvent);
    DeallocAPointer(rankData);
    DeallocAPointer(starData);
    DeallocAPointer(missionBit);
    DeallocAPointer(currentMissions);
    DeallocAPointer(currentMissionObjectives);

    ByeLog("disposing dialog buffer");
    DisposeDialogs();
#if TARGET_API_MAC_CARBON
    if ((Ptr)fullScreenRestore != NULL)
        EndFullScreen((Ptr)fullScreenRestore, 0);
#endif

#if !TARGET_API_MAC_CARBON
    ByeLog("disposing window");
#endif
    HideWindow((WindowRef)gWind);
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    CloseWindow((WindowRef)gWind);
#else
    DisposeWindow(gWind);
#endif
    gWind = NULL;

    ByeLog("closing QuickTime");
    ExitMovies();

    ByeLog("carbon cleanup");
#ifdef MACBUILD
    CarbonHackCleanup();
    ShowCursor();
#else
    ShowTheCursor();
#endif
#if V111
    FadeScreenIn();
#endif

#if defined(MAC1010) && TARGET_API_MAC_CARBON
    ByeLog("closing RezLib");
    if (gScreenRef != 0) {
        FadeToGamma(gScreenRef, &gOrigGamma, 0.1f);
        DisposeScreenRef(gScreenRef);
    }
#else
    ByeLog("closing MT");
#endif
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    FadeScreenIn();
    DisposeGammaTools();
#endif

    ByeLog("closing other tools");
    ST_Close();
    RT3_Close();
    IT_Close();
    DT_Close();
    FT_Close();
    PlatformClose();
#if defined(MACBUILD) && V111
    free(novaSupportFolder);
#endif
#if !V111

#if !TARGET_API_MAC_CARBON
    crashlog("showing control strip");
    RestoreControlStrip();

#endif
    crashlog("clean exit");

    crashlog("\n\n- end of log -");
    crashlog_close();
    ExitToShell();
#endif
}

/* InitColors. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ad7c0
// MAC: _InitColors i386 0x0001be73 ppc 0x0001ff78
void InitColors(void)
{
    bkgndGreen.red = bkgndGreen.blue = 0;
    bkgndGreen.green = 0x2200;
    dimGreen.red = dimGreen.blue = 0;
    dimGreen.green = 0x9900;
    dimmerGreen.red = dimmerGreen.blue = 0;
    dimmerGreen.green = 0x6666;
    brightGreen.red = brightGreen.blue = 0;
    brightGreen.green = 0xffff;
    rgbWhite.red = rgbWhite.green = rgbWhite.blue = 0xffff;
    rgbDarkGrey.red = rgbDarkGrey.green = rgbDarkGrey.blue = 0x4000;
    rgbRealDarkGrey.red = rgbRealDarkGrey.green = rgbRealDarkGrey.blue = 0xfa0;
    rgbMedGrey.red = rgbMedGrey.green = rgbMedGrey.blue = 0x8000;
    rgbLessDarkGrey.red = rgbLessDarkGrey.green = rgbLessDarkGrey.blue = 0x4e20;
    rgbLightGrey.red = rgbLightGrey.green = rgbLightGrey.blue = 0xc000;
    rgbRealDarkBlue.red = rgbRealDarkBlue.green = 0;
    rgbRealDarkBlue.blue = 0x428f;
    rgbBlack.red = rgbBlack.green = rgbBlack.blue = 0;
}

/* LoadSprites (mac _LoadSprites). STATE: MATCH 100.0%, exe 1534 / obj 1534, byte-identical yes
   (5184 bytes), relocregcheck 0 of 403. OPEN: none. The smoke walker `q' is its own pointer
   (declared after resID, numbered after it); blocks 1-5 all count on `i' and write `i + K'. Notes. */
// FUNCTION: EVNOVA 0x004ad960
// MAC: _LoadSprites i386 0x00020541 ppc 0x00024ca0
void LoadSprites(void)
{
    short spriteID, maskID;
    short xSize, ySize;
    short xTiles, yTiles;
    short err;
    short i;
    int32_t resID;
#ifndef MAC1010
    SpritePtr *q;
#endif

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (GetSpriteInfo(i + 3000, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterShotSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from RLE resource, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 3000);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 3000, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterShotSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from PICT resource, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 3000);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 3000, err);
                }
                SWLockSprite(masterShotSprite[i]);
                err = ConvertSpriteToRLE(masterShotSprite[i]);
                if ((int32_t)err != 0) {
                    crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 3000);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 3000, err);
                }
                masterShotSprite[i]->numFrames = xTiles * yTiles;
            }
            IterateInGameEventLoop(1);
        }
    }

    for (i = 0; i < kMaxExplodTypes; i++) {
        if (GetSpriteInfo(i + 400, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterExplodSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 400);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 400, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterExplodSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 400);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 400, err);
                }
                SWLockSprite(masterExplodSprite[i]);
                err = ConvertSpriteToRLE(masterExplodSprite[i]);
                if ((int32_t)err != 0) {
                    crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 400);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 400, err);
                }
                masterExplodSprite[i]->numFrames = xTiles * yTiles;
            }
            IterateInGameEventLoop(1);
        }
    }

    for (i = 0; i < 5; i++) {
        if (GetSpriteInfo(i + 500, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            masterBoxSprite[i] = 0;
            if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterBoxSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 500);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 500, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterBoxSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 500);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 500, err);
                }
                SWLockSprite(masterBoxSprite[i]);
                err = ConvertSpriteToRLE(masterBoxSprite[i]);
                if ((int32_t)err != 0) {
                    crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 500);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 500, err);
                }
                masterBoxSprite[i]->numFrames = xTiles * yTiles;
            }
            IterateInGameEventLoop(1);
        } else {
            crashlog_print("resource not found, " kSpinTypeName " ID ");
            crashlog_print_num(i + 500);
            crashlog_print("\n");
            ReportSpriteLoadError(1, i + 500, resNotFound);
        }
    }

    for (i = 0; i < kMaxRoidTypes; i++) {
        masterAsteroidSprite[i] = 0;
        if (GetSpriteInfo(i + 800, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterAsteroidSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 800);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 800, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterAsteroidSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 800);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 800, err);
                }
                SWLockSprite(masterAsteroidSprite[i]);
                err = ConvertSpriteToRLE(masterAsteroidSprite[i]);
                if ((int32_t)err != 0) {
                    crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 800);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 800, err);
                }
                masterAsteroidSprite[i]->numFrames = xTiles * yTiles;
            }
            IterateInGameEventLoop(1);
        } else {
            masterAsteroidSprite[i] = 0;
            if (i == 0) {
                crashlog_print("resource not found, " kSpinTypeName " ID ");
                crashlog_print_num(i + 800);
                crashlog_print("\n");
                ReportSpriteLoadError(1, i + 800, resNotFound);
            }
        }
    }

    for (i = 0; i < kMainButtons; i++) {
        menuButtonSprite[i] = 0;
        if (GetSpriteInfo(i + 600, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&menuButtonSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 600);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 600, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &menuButtonSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 600);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 600, err);
                }
                err = ConvertSpriteToRLE(menuButtonSprite[i]);
                if ((int32_t)err != 0) {
                    crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                    crashlog_print_num(i + 600);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 600, err);
                }
                menuButtonSprite[i]->numFrames = xTiles * yTiles;
            }
            SWLockSprite(menuButtonSprite[i]);
            SWSetCurrentFrameIndex(menuButtonSprite[i], 0);
            IterateInGameEventLoop(1);
        } else
            menuButtonSprite[i] = 0;
    }

    if (GetSpriteInfo(700, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&masterStarSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((int32_t)err != 0) {
                crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                crashlog_print_num(700);
                crashlog_print("\n");
                ReportSpriteLoadError(2, 700, err);
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterStarSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int32_t)err != 0) {
                crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                crashlog_print_num(700);
                crashlog_print("\n");
                ReportSpriteLoadError(3, 700, err);
            }
            err = ConvertSpriteToRLE(masterStarSprite);
            if ((int32_t)err != 0) {
                crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                crashlog_print_num(700);
                crashlog_print("\n");
                ReportSpriteLoadError(4, 700, err);
            }
            masterStarSprite->numFrames = xTiles * yTiles;
        }
        SWLockSprite(masterStarSprite);
        SWSetCurrentFrameIndex(masterStarSprite, 0);
        IterateInGameEventLoop(1);
    } else {
        masterStarSprite = NULL;
        if (i == 0) {
            crashlog_print("resource not found, " kSpinTypeName " ID ");
            crashlog_print_num(700);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 700, resNotFound);
        }
    }

    if (GetSpriteInfo(607, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&menuRolloverSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((int32_t)err != 0) {
                menuRolloverSprite = NULL;
                crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &menuRolloverSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int32_t)err != 0) {
                menuRolloverSprite = NULL;
                crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
            err = ConvertSpriteToRLE(menuRolloverSprite);
            if ((int32_t)err != 0) {
                crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
            menuRolloverSprite->numFrames = xTiles * yTiles;
        }
        if (menuRolloverSprite != NULL) {
            SWLockSprite(menuRolloverSprite);
            SWSetCurrentFrameIndex(menuRolloverSprite, 0);
        }
        IterateInGameEventLoop(1);
    } else {
        menuRolloverSprite = NULL;
        if (i == 0) {
            crashlog_print("resource not found, " kSpinTypeName " ID ");
            crashlog_print_num(607);
            crashlog_print("\n");
        }
    }

    if (GetSpriteInfo(650, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&cursorSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((int32_t)err != 0) {
                cursorSprite = NULL;
                crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &cursorSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int32_t)err != 0) {
                crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
            err = ConvertSpriteToRLE(cursorSprite);
            if ((int32_t)err != 0) {
                crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
            cursorSprite->numFrames = xTiles * yTiles;
        }
        if (cursorSprite != NULL) {
            SWLockSprite(cursorSprite);
            SWSetCurrentFrameIndex(cursorSprite, 0);
        }
        IterateInGameEventLoop(1);
    } else {
        cursorSprite = NULL;
        if (i == 0) {
            crashlog_print("resource not found, " kSpinTypeName " ID ");
            crashlog_print_num(650);
            crashlog_print("\n");
        }
    }

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterNavTargetSprite, NULL, 10000, 8, 1);
    if ((int32_t)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(10000);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 10000, err);
    }
    SWLockSprite(masterNavTargetSprite);
    err = SWCreateRLEData(masterNavTargetSprite, 0);
    if ((int32_t)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(10000);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 10000, err);
    }

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterWeapTargetSprite, NULL, 10008, 16, 1);
    if ((int32_t)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(10008);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 10008, err);
    }
    SWLockSprite(masterWeapTargetSprite);
    err = SWCreateRLEData(masterWeapTargetSprite, 0);
    if ((int32_t)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(10008);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 10008, err);
    }
    IterateInGameEventLoop(1);

    for (i = 0; i < kMaxShipTypes; i++) {
        targetInfoPic[i] = GetPicture(i + 3000);
#ifdef MACBUILD
        if (targetInfoPic[i] != NULL)
            HNoPurge(targetInfoPic[i]);
#endif
    }

    for (i = 0; i < 10; i++)
        staticPat[i] = GetPixPat(i + 128);
    IterateInGameEventLoop(1);

#ifdef MAC1010
    for (i = 0; i < 2; i++) {
        masterSmokeSprite[i] = 0;
        err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterSmokeSprite[i], NULL, resID = 1000 + i * 8, 8, 1);
#elif V111 /* V111 spelling */
    for (i = 0; i < 2; i++) {
        resID = 1000 + i * 8;
        masterSmokeSprite[i] = 0;
        err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterSmokeSprite[i], NULL, resID, 8, 1);
#else
    for (i = 0, resID = 1000, q = masterSmokeSprite; i < 2; i++, resID += 8, q++) {
        masterSmokeSprite[i] = 0;
        err = SWCreateSpriteFromCicnResource(gSpriteWorld, q, NULL, resID, 8, 1);
#endif
        if ((int32_t)err == 0 && masterSmokeSprite[i] != NULL) {
            SWLockSprite(masterSmokeSprite[i]);
            err = SWCreateRLEData(masterSmokeSprite[i], 0);
            if ((int32_t)err != 0) {
                crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                crashlog_print_num(resID);
                crashlog_print("\n");
                ReportSpriteLoadError(4, resID, err);
            }
        } else
            masterSmokeSprite[i] = 0;
    }
    IterateInGameEventLoop(1);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &alertSprite, NULL, 18000, 2, 1);
    if ((int32_t)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(18000);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 18000, err);
    }
    SWLockSprite(alertSprite);
    err = SWCreateRLEData(alertSprite, 0);
    if ((int32_t)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(18000);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 18000, err);
    }
    SWSetSpriteDrawProc(alertSprite, BlitPixieRLETranslucentDrawProc);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &doubleTimeSprite, NULL, 20000, 1, 1);
    if ((int32_t)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(20000);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 20000, err);
    }
    SWLockSprite(doubleTimeSprite);
    err = SWCreateRLEData(doubleTimeSprite, 0);
    if ((int32_t)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(20000);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 20000, err);
    }
    SWSetSpriteDrawProc(doubleTimeSprite, BlitPixieRLETranslucentDrawProc);
    IterateInGameEventLoop(1);

    redMapArrow = (Image_t *)GetCIcon(15000);
    greenMapArrow = (Image_t *)GetCIcon(15001);
}

/* LoadShipSprites (mac _LoadShipSprites). STATE: MATCH 100.0%, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aeda0
// MAC: _LoadShipSprites i386 0x00023e3e ppc 0x000281a4
void LoadShipSprites(void)
{
    short k;
    short shipType;
    short count;
    short spriteIndex;

    for (shipType = 0; shipType < kMaxShipTypes; shipType++) {
        for (k = 0; k < 6; k++)
            shipSpriteLoaded[shipType][k] = -1;
        masterShipSprite[shipType] = 0;
        masterShipGlowSprite[shipType] = 0;
        masterShipLightSprite[shipType] = 0;
        masterShipWeaponSprite[shipType] = 0;
        masterShipAltSprite[shipType] = 0;
        masterShipShieldSprite[shipType] = 0;
    }

    for (shipType = 0; shipType < kMaxShipTypes; shipType++) {
        ServiceIntroMusic(0);
        count = LoadExtendedShipSprites(shipType);
        if (count > 0) {
            DeltaIntroProgressBar(1.0);

            if (targetInfoPic[shipType]) {
                shipClass[shipType].targetPictID = shipType;
            } else {
                shipClass[shipType].targetPictID = spriteIndex = shipClass[shipType].spriteIndex;
                if (shipType != kMaxShipTypes - 1) {
                    if (crashlog_isactive()) {
                        if (targetInfoPic[shipClass[shipType].targetPictID] == 0) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(shipClass[shipType].name);
                            crashlog_print(" (");
                            crashlog_print_num(shipType + kFirstResourceID);
                            crashlog_print(") lacks target picture\n");
                        }
                    }
                }
            }

            if (ResourceExists(kPictType, shipType + 5000)) {
                shipClass[shipType].shipyardPictID = shipType + 5000;
            } else {
                shipClass[shipType].shipyardPictID = shipClass[shipType].spriteIndex + 5000;
                if (crashlog_isactive()) {
                    if (shipClass[shipType].buyRandom != 0 || shipClass[shipType].hireRandom != 0) {
                        if (!ResourceExists(kPictType, shipClass[shipType].spriteIndex + 5000)) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(shipClass[shipType].name);
                            crashlog_print(" (");
                            crashlog_print_num(shipType + kFirstResourceID);
                            crashlog_print(") lacks shipyard picture\n");
                        }
                    }
                }
            }
        }

        IterateInGameEventLoop(1);
    }
}

/* InitSprites (mac _InitSprites). STATE: MATCH 100.0%, exe 1542 / obj 1542, byte-identical
   yes (5907 bytes), relocregcheck 0 of 566. OPEN: none. */
// FUNCTION: EVNOVA 0x004af020
// MAC: _InitSprites i386 0x0001eda2 ppc 0x0002365c
void InitSprites(void)
{
    FloatPoint vel;
    FloatPoint origin;
    float step;
    float angle;
    short i;
    short j;
    short err;

    SWUnlockSpriteLayer(starLayer);
    SWUnlockSpriteLayer(shipLayer);
    SWUnlockSpriteLayer(escortLayer);
    SWUnlockSpriteLayer(stellarLayer);
    SWUnlockSpriteLayer(asteroidLayer);
    SWUnlockSpriteLayer(boxLayer);
    SWUnlockSpriteLayer(smokeLayer);
    SWUnlockSpriteLayer(shotLayer);
    SWUnlockSpriteLayer(explodLayer);
    SWUnlockSpriteLayer(targetLayer);

    firstShipGlowIndex = firstShipLightIndex = firstShipAltIndex =
        firstShipWeapIndex = firstShipShieldIndex = -1;

    origin.x = origin.y = 0.0f;

    for (i = 0; i < kMaxShipTypes; i++) {
        if (masterShipGlowSprite[i] != NULL && firstShipGlowIndex == -1)
            firstShipGlowIndex = i;
        if (masterShipLightSprite[i] != NULL && firstShipLightIndex == -1)
            firstShipLightIndex = i;
        if (masterShipWeaponSprite[i] != NULL && firstShipWeapIndex == -1)
            firstShipWeapIndex = i;
        if (masterShipAltSprite[i] != NULL && firstShipAltIndex == -1)
            firstShipAltIndex = i;
        if (masterShipShieldSprite[i] != NULL && firstShipShieldIndex == -1)
            firstShipShieldIndex = i;

        if ((shipClass[i].shanFlags & kShanSkewCorrect)
                && shipClass[i].framesPer > 0) {
            shipClass[i].frameAngles =
                (short *)AllocAPointer(shipClass[i].framesPer * 2);
            if (shipClass[i].frameAngles == NULL)
                EV_ASSERT_FAIL("Setup.c", 1740, 2035);
            step = 360.0 / (float)shipClass[i].framesPer;
            for (j = 0; j < shipClass[i].framesPer; j++) {
                angle = step * j;
                vel.x = vel.y = 0.0f;
                Accel(angle, 100.0, &vel);
                if (angle <= 90.0 || angle > 270.0) {
                    vel.x *= shipClass[i].compress[0].x;
                    vel.y *= shipClass[i].compress[0].y;
                } else {
                    vel.x *= shipClass[i].compress[1].x;
                    vel.y *= shipClass[i].compress[1].y;
                }
                shipClass[i].frameAngles[j] = CalcInterceptAngle(&origin, &vel);
            }
        }
    }

    IterateInGameEventLoop(1);

    for (i = kMaxShips - 1; i >= 0; i--) {
        ServiceIntroMusic(0);
        err = SWCloneSprite(masterShipSprite[0], &shipData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning ship base sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWCloneFramesFromMaster(shipData[i].sprite, masterShipSprite[0]);
        SWAddSprite(shipLayer, shipData[i].sprite);
        SWSetSpriteVisible(shipData[i].sprite, 0);
        SWSetCurrentFrameIndex(shipData[i].sprite, 0);
        SWSetSpriteDrawProc(shipData[i].sprite, BlitPixieRLETranslucentDrawProc);
        SWSetSpriteCollideProc(shipData[i].sprite, ShipCollisionHandler);
        shipData[i].sprite->userData = i;
        SWLockSprite(shipData[i].sprite);

        if (firstShipAltIndex != -1) {
            err = SWCloneSprite(masterShipAltSprite[firstShipAltIndex],
                                &shipData[i].altSprite, NULL);
            if ((int32_t)err != 0) {
                crashlog_print("error cloning ship alt sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipData[i].altSprite);
            SWSetSpriteVisible(shipData[i].altSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].altSprite, 0);
            SWSetSpriteDrawProc(shipData[i].altSprite,
                                BlitPixieRLETranslucentDrawProc);
            shipData[i].altSprite->userData = i;
            SWSetSpriteCollideProc(shipData[i].altSprite, ShipCollisionHandler);
            SWLockSprite(shipData[i].altSprite);
            shipData[i].altSprite->translucency = 0;
            shipData[i].altSprite->tintMode = 0;
        }
        if (firstShipGlowIndex != -1) {
            err = SWCloneSprite(masterShipGlowSprite[firstShipGlowIndex],
                                &shipData[i].glowSprite, NULL);
            if ((int32_t)err != 0) {
                crashlog_print("error cloning ship glow sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipData[i].glowSprite);
            SWSetSpriteVisible(shipData[i].glowSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].glowSprite, 0);
            SWSetSpriteDrawProc(shipData[i].glowSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipData[i].glowSprite);
            shipData[i].glowSprite->translucency = 0x20;
            shipData[i].glowSprite->tintRed = 0x20;
            shipData[i].glowSprite->tintGreen = 0x20;
            shipData[i].glowSprite->tintBlue = 0x20;
            shipData[i].glowSprite->tintMode = 0;
            shipData[i].glowSprite->collisionsDisabled = 1;
        }
        if (firstShipLightIndex != -1) {
            err = SWCloneSprite(masterShipLightSprite[firstShipLightIndex],
                                &shipData[i].lightSprite, NULL);
            if ((int32_t)err != 0) {
                crashlog_print("error cloning ship light sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipData[i].lightSprite);
            SWSetSpriteVisible(shipData[i].lightSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].lightSprite, 0);
            SWSetSpriteDrawProc(shipData[i].lightSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipData[i].lightSprite);
            shipData[i].lightSprite->translucency = 0x20;
            shipData[i].lightSprite->tintRed = 0x20;
            shipData[i].lightSprite->tintGreen = 0x20;
            shipData[i].lightSprite->tintBlue = 0x20;
            shipData[i].lightSprite->tintMode = 0;
            shipData[i].lightSprite->collisionsDisabled = 1;
        }
        if (firstShipWeapIndex != -1) {
            err = SWCloneSprite(masterShipWeaponSprite[firstShipWeapIndex],
                                &shipData[i].weapSprite, NULL);
            if ((int32_t)err != 0) {
                crashlog_print("error cloning ship weapon sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipData[i].weapSprite);
            SWSetSpriteVisible(shipData[i].weapSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].weapSprite, 0);
            SWSetSpriteDrawProc(shipData[i].weapSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipData[i].weapSprite);
            shipData[i].weapSprite->translucency = 0x20;
            shipData[i].weapSprite->tintRed = 0x20;
            shipData[i].weapSprite->tintGreen = 0x20;
            shipData[i].weapSprite->tintBlue = 0x20;
            shipData[i].weapSprite->tintMode = 0;
            shipData[i].weapSprite->collisionsDisabled = 1;
        }
        if (firstShipShieldIndex != -1) {
            err = SWCloneSprite(masterShipShieldSprite[firstShipShieldIndex],
                                &shipData[i].shieldSprite, NULL);
            if ((int32_t)err != 0) {
                crashlog_print("error cloning ship shield sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipData[i].shieldSprite);
            SWSetSpriteVisible(shipData[i].shieldSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].shieldSprite, 0);
            SWSetSpriteDrawProc(shipData[i].shieldSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipData[i].shieldSprite);
            shipData[i].shieldSprite->translucency = 0x20;
            shipData[i].shieldSprite->tintRed = 0x20;
            shipData[i].shieldSprite->tintGreen = 0x20;
            shipData[i].shieldSprite->tintBlue = 0x20;
            shipData[i].shieldSprite->tintMode = 0;
        }
    }

    for (i = 0; i < kMasterStellarSprites; i++)
        masterStellarSprite[i] = NULL;

    ServiceIntroMusic(0);
    LoadMasterStellarSprite(0);

    for (i = 0; i < kNavSlots; i++) {
        err = SWCreateEmptySprite(&stellarSprite[i], NULL);
        if ((int32_t)err != 0)
            ReportSpriteLoadError(1, 0, err);
        SWCloneFramesFromMaster(stellarSprite[i], masterStellarSprite[0]);
        SWAddSprite(stellarLayer, stellarSprite[i]);
        SWSetCurrentFrameIndex(stellarSprite[i], 0);
        SWSetSpriteVisible(stellarSprite[i], 0);
        SWSetSpriteDrawProc(stellarSprite[i], BlitPixieRLETranslucentDrawProc);
        SWLockSprite(stellarSprite[i]);
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxExplods; i++) {
        err = SWCloneSprite(masterExplodSprite[0], &explodData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning explosion sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(explodLayer, explodData[i].sprite);
        SWSetCurrentFrameIndex(explodData[i].sprite, 0);
        SWSetSpriteVisible(explodData[i].sprite, 0);
        SWSetSpriteDrawProc(explodData[i].sprite, BlitPixieRLETranslucentDrawProc);
        explodData[i].sprite->translucency = 0x20;
        explodData[i].sprite->tintRed = 0x20;
        explodData[i].sprite->tintGreen = 0x20;
        explodData[i].sprite->tintBlue = 0x20;
        explodData[i].sprite->tintMode = 0;
        SWLockSprite(explodData[i].sprite);
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxShots; i++) {
        err = SWCloneSprite(masterShotSprite[0], &shotData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning shot sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(shotLayer, shotData[i].sprite);
        SWSetCurrentFrameIndex(shotData[i].sprite, 0);
        SWSetSpriteVisible(shotData[i].sprite, 0);
        SWSetSpriteDrawProc(shotData[i].sprite, BlitPixieRLETranslucentDrawProc);
        shotData[i].sprite->userData = i;
        SWLockSprite(shotData[i].sprite);
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxBoxes; i++) {
        err = SWCloneSprite(masterBoxSprite[0], &boxData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning box sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(boxLayer, boxData[i].sprite);
        SWSetCurrentFrameIndex(boxData[i].sprite, 0);
        SWSetSpriteVisible(boxData[i].sprite, 0);
        SWSetSpriteDrawProc(boxData[i].sprite, BlitPixieRLETranslucentDrawProc);
        SWLockSprite(boxData[i].sprite);
    }

    for (i = 0; i < kMaxPods; i++) {
        err = SWCloneSprite(masterShipSprite[kMaxShipTypes - 1],
                            &podData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning pod sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(boxLayer, podData[i].sprite);
        SWSetCurrentFrameIndex(podData[i].sprite, 0);
        SWSetSpriteVisible(podData[i].sprite, 0);
        SWSetSpriteDrawProc(podData[i].sprite, BlitPixieRLETranslucentDrawProc);
        SWLockSprite(podData[i].sprite);
    }

    for (i = 0; i < kMaxStars; i++) {
        err = SWCloneSprite(masterStarSprite, &starData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning star sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        starData[i].sprite->tintRed = 0;
        starData[i].sprite->tintGreen = 0;
        starData[i].sprite->tintBlue = 0;
        SWAddSprite(starLayer, starData[i].sprite);
        SWSetCurrentFrameIndex(starData[i].sprite, 0);
        SWSetSpriteVisible(starData[i].sprite, 0);
        SWSetSpriteDrawProc(starData[i].sprite, BlitPixieRLEDrawProc);
        SWLockSprite(starData[i].sprite);
    }

    if (masterSmokeSprite[0] != NULL) {
        for (i = 0; i < kMaxDebris; i++) {
            err = SWCloneSprite(masterSmokeSprite[0], &smokeData[i].sprite, NULL);
            if ((int32_t)err != 0) {
                crashlog_print("error cloning smoke sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(smokeLayer, smokeData[i].sprite);
            SWSetCurrentFrameIndex(smokeData[i].sprite, 0);
            SWSetSpriteVisible(smokeData[i].sprite, 0);
            SWSetSpriteDrawProc(smokeData[i].sprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(smokeData[i].sprite);
        }
    } else {
        for (i = 0; i < kMaxDebris; i++)
            smokeData[i].sprite = NULL;
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxAsteroids; i++) {
        err = SWCloneSprite(masterAsteroidSprite[0], &asteroidData[i].sprite, NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning asteroid sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(asteroidLayer, asteroidData[i].sprite);
        SWSetCurrentFrameIndex(asteroidData[i].sprite, 0);
        SWSetSpriteVisible(asteroidData[i].sprite, 0);
        SWSetSpriteDrawProc(asteroidData[i].sprite, BlitPixieRLETranslucentDrawProc);
        SWSetSpriteCollideProc(asteroidData[i].sprite, AsteroidCollisionHandler);
        SWLockSprite(asteroidData[i].sprite);
        asteroidData[i].sprite->userData = i;
    }

    for (i = 0; i < kMaxNavTargets; i++) {
        err = SWCloneSprite(masterNavTargetSprite, &navTargetSprite[i], NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning nav target sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(targetLayer, navTargetSprite[i]);
        SWSetSpriteVisible(navTargetSprite[i], 0);
        SWSetSpriteDrawProc(navTargetSprite[i], BlitPixieRLETranslucentDrawProc);
        navTargetSprite[i]->tintMode = 0;
        navTargetSprite[i]->translucency = 0x20;
        navTargetSprite[i]->tintRed = 0x20;
        navTargetSprite[i]->tintGreen = 0x20;
        navTargetSprite[i]->tintBlue = 0x20;
        SWLockSprite(navTargetSprite[i]);
    }

    for (i = 0; i < kMaxNavTargets; i++) {
        err = SWCloneSprite(masterWeapTargetSprite, &weapTargetSprite[i], NULL);
        if ((int32_t)err != 0) {
            crashlog_print("error cloning weap target sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWAddSprite(targetLayer, weapTargetSprite[i]);
        SWSetSpriteVisible(weapTargetSprite[i], 0);
        SWSetSpriteDrawProc(weapTargetSprite[i], BlitPixieRLETranslucentDrawProc);
        weapTargetSprite[i]->tintMode = 0;
        weapTargetSprite[i]->translucency = 0x20;
        weapTargetSprite[i]->tintRed = 0x20;
        weapTargetSprite[i]->tintGreen = 0x20;
        weapTargetSprite[i]->tintBlue = 0x20;
        SWLockSprite(weapTargetSprite[i]);
    }

    if (cursorSprite != NULL) {
        SWAddSprite(targetLayer, cursorSprite);
        SWSetSpriteVisible(cursorSprite, 0);
        SWSetSpriteDrawProc(cursorSprite, BlitPixieRLETranslucentDrawProc);
        cursorSprite->tintMode = 0;
        cursorSprite->translucency = 0x20;
        cursorSprite->tintRed = 0x20;
        cursorSprite->tintGreen = 0x20;
        cursorSprite->tintBlue = 0x20;
        SWLockSprite(cursorSprite);
    }

    ServiceIntroMusic(0);

    SWAddSprite(targetLayer, alertSprite);
    SWSetCurrentFrameIndex(alertSprite, 0);
    SWSetSpriteVisible(alertSprite, 0);
    SWSetSpriteDrawProc(alertSprite, BlitPixieRLETranslucentDrawProc);
#if V111
    if (useLegacyRenderingPipeline)
        SWLockSprite(alertSprite);
#else
    SWLockSprite(alertSprite);
#endif
    SWSetSpriteLocation(alertSprite, screenCenter.h * 2 - GetSpriteXSize(alertSprite), 0);

    SWAddSprite(targetLayer, doubleTimeSprite);
    SWSetCurrentFrameIndex(doubleTimeSprite, 0);
    SWSetSpriteVisible(doubleTimeSprite, 0);
    SWSetSpriteDrawProc(doubleTimeSprite, BlitPixieRLETranslucentDrawProc);
    SWLockSprite(doubleTimeSprite);
    SWSetSpriteLocation(doubleTimeSprite, 0, 0);

    SWLockSpriteLayer(starLayer);
    SWLockSpriteLayer(shipLayer);
    SWLockSpriteLayer(escortLayer);
    SWLockSpriteLayer(shotLayer);
    SWLockSpriteLayer(stellarLayer);
    SWLockSpriteLayer(asteroidLayer);
    SWLockSpriteLayer(boxLayer);
    SWLockSpriteLayer(smokeLayer);
    SWLockSpriteLayer(explodLayer);
    SWLockSpriteLayer(targetLayer);

    ServiceIntroMusic(0);

#ifdef CEFIX /* F12.80-81 */
    commRect.left = GetCWindowPortRect(gWind)->left + 25;
    commRect.right = GetCWindowPortRect(gWind)->right - ceStatusBarWidth - 50;
    commRect.bottom = GetCWindowPortRect(gWind)->bottom - 5;
    commRect.top = commRect.bottom - CE_Scale(26) - 4;
    /* CE bug: CE drops only the store of 29470 / width; its idiv still runs and faults on a
       zero width. */
    if (commRect.right - commRect.left == 0)
        RaiseException(kCEIntDivideByZero, 0, 0, NULL);
#else
    SetRect(&commRect, GetCWindowPortRect((WindowRef)gWind)->left + 25,
            GetCWindowPortRect((WindowRef)gWind)->bottom - 5,
            GetCWindowPortRect((WindowRef)gWind)->right - 244,
            GetCWindowPortRect((WindowRef)gWind)->bottom - 5);
    commRect.top -= 29470 / (commRect.right - commRect.left);
#endif
#ifdef MACBUILD
    tempRgn = NewRgn();
#endif
}



/* LoadSounds. STATE: MATCH 100%, exe 182 / obj 182 instrs. The census computes each
   resource ID in the call; the baseID/id induction variables were invented (mwcc's own
   strength reduction makes them). OPEN: none. Notes: Research/notes/loadsounds.md */
// FUNCTION: EVNOVA 0x004b0740
// MAC: _LoadSounds i386 0x0001c036 ppc 0x000201a4
void LoadSounds(void)
{
    short msg;
    short i;
    short set;
#ifdef MACBUILD
    SoundComponentData info;
    uint32_t frames;
    uint32_t offset;
#else
    AudioDesc info;
#endif
    Handle h;
    unsigned short vol;

    for (set = 0; set < kVoiceSets; set++) {
        for (msg = 0; msg < kVoiceMessages; msg++) {
            voiceResourceCount[set][msg] = 0;
            i = 0;
            do {
                if (ResourceExists(kSoundType,
                        kFirstVoiceID + set * kVoiceSetStride + msg * kVoiceMsgStride + i) == 0)
                    break;
                voiceResourceCount[set][msg] += 1;
                i++;
            } while (i < kVoiceClipsMax);
        }
    }

    for (msg = 0; msg <= 1; msg++) {
        jumpSoundLength[msg] = kDefaultLoopTicks;
        h = GetResource(kSoundType, msg + kFirstResourceID);
        if (h != NULL) {
#ifdef MACBUILD
            ParseSndHeader((SndListHandle)h, &info, &frames, &offset);
            jumpSoundLength[msg] = info.sampleCount * kTicksPerSecond / (info.sampleRate >> 16);
#else
            LockAHandle(h);
            if (ParseSoundFileImage(&info, (Word8 *)*h, GetAHandleSize(h)) == 0)
                jumpSoundLength[msg] = (info.SampleCount * kTicksPerSecond
                                            / info.SampleRate);
#endif
            ReleaseResource(h);
        }
    }

    warpUpSnd[0] = ST_LoadSndResource(0x80);
    warpUpSnd[1] = ST_LoadSndResource(0x81);
    warpOutSnd = ST_LoadSndResource(0x82);

    warpUpSoundSpec.handle = (void **)warpUpSnd[0];
    warpUpSoundSpec.rate = kUnityRate;
    warpUpSoundSpec.done = NULL;
    warpUpSoundSpec.priority = kHighPriority;
    vol = gSoundVolume;
    warpUpSoundSpec.leftLevel = vol;
    warpUpSoundSpec.rightLevel = vol;
    planetSoundSpec.handle = NULL;
    planetSoundSpec.rate = kUnityRate;
    planetSoundSpec.done = NULL;
    planetSoundSpec.priority = kHighPriority;
    planetSoundSpec.leftLevel = vol;
    planetSoundSpec.rightLevel = vol;

    for (msg = 0; msg <= kEngineSounds - 1; msg++)
        beepSnd[msg] = ST_LoadSndResource(msg + kFirstEngineID);

    for (msg = 0; msg < kShotSounds; msg++)
        shotSnd[msg] = ST_LoadSndResource(msg + kFirstShotID);

    for (msg = 0; msg < kExplodSounds; msg++)
        explodSnd[msg] = ST_LoadSndResource(msg + kFirstExplodID);

    redAlertSnd = ST_LoadSndResource(0x172);
    klaxxonSnd = ST_LoadSndResource(0x173);
    escapePodSnd = ST_LoadSndResource(0x174);
    cloakSnd[0] = ST_LoadSndResource(0x17c);
    cloakSnd[1] = ST_LoadSndResource(0x17d);
    hardDockSnd = ST_LoadSndResource(0x186);
    clickSnd = ST_LoadSndResource(0x258);
    missionSoundID = -1;
    missionSnd = NULL;
    voiceSnd = NULL;
    pendingVoiceMsg = -1;
    pendingVoiceGovt = -1;
    pendingVoiceParity = -1;
}

/* 'r' 'l' <MacRoman e-diaeresis, 0x91> '8'/'D' as big-endian ResTypes */

#define kStellarSpinResBase 1000

/* LoadMasterStellarSprite (mac _LoadMasterStellarSprite). STATE: MATCH 100.0%, exe 160 /
   obj 160 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004b0a30
// MAC: _LoadMasterStellarSprite i386 0x0001eb72 ppc 0x00023474
void LoadMasterStellarSprite(short type)
{
    short spriteID, maskID, xSize, ySize, xTiles, yTiles;
#if V111 /* V111 spelling */
    short resID;
#else
    int32_t resID;
#endif
    short err;
    SpriteRec *entry;

    resID = type + kStellarSpinResBase;
    if (GetSpriteInfo(resID, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&masterStellarSprite[type], NULL, spriteID,
                                xTiles * yTiles, 0);
            if ((int)err != 0) {
                crashlog_print("error loading stellar sprite from RLE, type ");
                crashlog_print_num(type);
                crashlog_print("\n");
                ReportSpriteLoadError(2, resID, err);
            }
            SWLockSprite(masterStellarSprite[type]);
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterStellarSprite[type], NULL,
                                spriteID, maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int)err != 0) {
                crashlog_print("error loading stellar sprite from PICT, type ");
                crashlog_print_num(type);
                crashlog_print("\n");
                ReportSpriteLoadError(3, resID, err);
            }
            SWLockSprite(masterStellarSprite[type]);
            err = ConvertSpriteToRLE(masterStellarSprite[type]);
            if ((int)err != 0) {
                crashlog_print("error converting stellar sprite to RLE, type ");
                crashlog_print_num(type);
                crashlog_print("\n");
                ReportSpriteLoadError(4, resID, err);
            }
        }
        entry = masterStellarSprite[type];
        if (entry != NULL)
            entry->numFrames = xTiles * yTiles;
    }
}


#define kEscortCount     4

#define kBasePriceStringID 9300     /* plugin string, else STR# 4004 item i+1 */
#define kBasePriceListID   4004
#define kSystemGovtUnset   (-32767)
#define kRoidGrey          0x8012   /* 8-bit asteroid colour, run through Color2Index */
#define kRoidPixel         0x1ce7   /* the direct 15-bit pixel otherwise */



#if V111 /* V111 spelling */
#define roid n
#else
#define roid i
#endif

/* InitObjects (mac _InitObjects). STATE: win MATCH, byte-identical; mac MATCH both slices.
   OPEN: none; notes */
// FUNCTION: EVNOVA 0x004b0c20
// MAC: _InitObjects i386 0x0001c2e5 ppc 0x000204b8
void InitObjects(void)
{
    short i, j, k;
    RGBColor c;
    Str255 buf;
    int32_t value;
    int n;

    for (i = 0; i < kMaxShips; i++) {
        shipData[i].index = i;
        shipData[i].heading = 0.0f;
        shipData[i].desiredHeading = 0;
        shipData[i].curSystem = -1;
        shipData[i].type = 0;
        shipData[i].boarded = 0;
        shipData[i].active = 0;
        shipData[i].cloakDir = 0;
        shipData[i].cloak = 0.0f;
        shipData[i].credits = 0;
        shipData[i].defenseStellar = -1;
        shipData[i].unk6e = 1;
        shipData[i].targetShip = -1;
        shipData[i].selectedWeapon = -1;
        shipData[i].shield = 0.0f;
        shipData[i].armor = 0.0f;
        shipData[i].aiType = 0;
        shipData[i].aiTimer = 0.0f;
        shipData[i].navTargetKind = -1;
        shipData[i].targetStellar = -1;
        shipData[i].lastInspected = -1;
        shipData[i].lastStellar = -2;
        shipData[i].jumpTimer = 0.0f;
        shipData[i].fuel = 100.0f;
        shipData[i].parentShip = -1;
        shipData[i].deathTimer = 0.0f;
        shipData[i].ionization = 0.0f;
        shipData[i].ionColor = 0;
        shipData[i].lightLevel = 0;
        shipData[i].blinkCount = 0;
        shipData[i].skill = 1.0f;
        shipData[i].unk9C = 0;
        shipData[i].formerEscort = -1;
        shipData[i].sprite = 0;
        shipData[i].glowSprite = 0;
        shipData[i].weapSprite = 0;
        shipData[i].lightSprite = 0;
        shipData[i].altSprite = 0;
        shipData[i].shieldSprite = 0;
        shipData[i].bankDir = 0;
        shipData[i].foldDir = 0;
        shipData[i].foldSet = 0;
        shipData[i].frameTimer = 0.0f;
        shipData[i].shieldGlow = 0;
        shipData[i].weapGlow = 0;
        shipData[i].annoyance = 0;
        for (j = 0; j < kMaxWeapTypes; j++) {
            shipData[i].weap[j].reloadTimer = 0.0f;
            shipData[i].weap[j].burstCount = 0;
            shipData[i].weap[j].count = 0;
            shipData[i].weap[j].ammo = 0;
        }
        for (j = 0; j <= kCargoTypes - 1; j++)
            shipData[i].cargo[j] = 0;
        shipData[i].sprite = 0;
        shipData[i].altSprite = 0;
        shipData[i].glowSprite = 0;
        shipData[i].lightSprite = 0;
        shipData[i].weapSprite = 0;
        shipData[i].shieldSprite = 0;
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        shipClass[i].techLevel = kResUnloaded;
        shipClass[i].shield = 1;
        shipClass[i].armor = 1;
        shipClass[i].strength = 1;
        shipClass[i].dispWeight = 1;
        shipClass[i].upgradeTo = -1;
        shipClass[i].altSets = 0;
        shipClass[i].baseSets = 0;
        shipClass[i].transparency = 0;
        shipClass[i].keyCarried = -1;
        shipClass[i].shanFlags = 0;
        shipClass[i].contributes[0] = 0;
        shipClass[i].contributes[1] = 0;
        shipClass[i].spriteIndex = i;
        shipClass[i].mass = -1;
        shipClass[i].hireRandom = 100;
        shipClass[i].buyRandom = 100;
        shipClass[i].dailyHireRoll = 50;
        shipClass[i].dailySaleRoll = 50;
        for (j = 0; j < kMaxWeapTypes; j++) {
            shipClass[i].weapCount[j] = 0;
            shipClass[i].weapAmmo[j] = 0;
        }
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                shipClass[i].weapPos[j][k].h = 0;
                shipClass[i].weapPos[j][k].v = 0;
                shipClass[i].weapPosZ[j][k] = 0;
            }
        }
        for (j = 0; j < 2; j++) {
            shipClass[i].compress[j].x = 1.0f;
            shipClass[i].compress[j].y = 0.71f;
        }
        shipClass[i].frameAngles = 0;
        masterShipSprite[i] = 0;
        masterShipGlowSprite[i] = 0;
        masterShipLightSprite[i] = 0;
        masterShipWeaponSprite[i] = 0;
        masterShipAltSprite[i] = 0;
        masterShipShieldSprite[i] = 0;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        outfitSystem[i].techLevel = kOutfTechAbsent;
        outfitSystem[i].dispWeight = 0;
        outfitSystem[i].contributes[0] = 0;
        outfitSystem[i].contributes[1] = 0;
        outfitSystem[i].requires[0] = 0;
        outfitSystem[i].requires[1] = 0;
        outfitSystem[i].itemClass = -1;
        outfitSystem[i].scanMask = 0;
        outfitSystem[i].buyRandom = 0;
        outfitSystem[i].dailyRoll = 50;
        outfitSystem[i].sameNameIndex = i;
        outfitSystem[i].govtRestrict = -1;
    }

    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].index = i;
        stellarData[i].loaded = 0;
        stellarData[i].prepared = 0;
        stellarData[i].type = 0;
        stellarData[i].techLevel = 0;
        stellarData[i].dominated = 0;
        stellarData[i].daysDominated = 0;
        stellarData[i].system = -1;
        stellarData[i].numFrames = 1;
        stellarData[i].curFrame = 0;
        stellarData[i].frameTimer = 0;
        stellarData[i].gravity = 0;
        stellarData[i].deadDays = 0;
        stellarData[i].frame0Bias = 1;
        stellarData[i].reloadTimer = 0;
        stellarData[i].burstShots = 0;
        stellarData[i].deadCounter = 0;
        stellarData[i].strength = 0;
        stellarData[i].deadDays = 0;
        stellarData[i].deadDaysLeft = 0;
        stellarData[i].tribute = 1000;
        stellarData[i].landingFee = 0;
        for (j = 0; j < kSpobHyperLinks; j++)
            stellarData[i].hyperLinks[j] = -1;
        for (j = 0; j < 8; j++)
            stellarData[i].specialTech[j] = 0;
    }

    for (i = 0; i < kMaxSystems; i++) {
        systemData[i].loaded = 0;
        systemData[i].prepared = 0;
        systemData[i].govt = kSystemGovtUnset;
        systemData[i].asteroidCount = 0;
        systemData[i].astTypes = 0;
        systemData[i].interference = 0;
        systemData[i].exploreLevel = 0;
        systemData[i].govt = -1;
        systemData[i].pos.h = systemData[i].pos.v = 0;
        for (j = 0; j < kMaxSystemLinks; j++)
            systemData[i].links[j] = -1;
        for (j = 0; j < kNavSlots; j++)
            systemData[i].navDefault[j] = -1;
        for (j = 0; j < 8; j++) {
            systemData[i].fleets[j] = -1;
            systemData[i].dudeTypes[j] = -1;
            systemData[i].dudeProbs[j] = 0;
        }
        systemData[i].fleetCount = 0;
        systemData[i].fleetProbTotal = 0;
        systemData[i].known = 0;
        systemData[i].colocNext = -1;
        systemData[i].colocGroup = -1;
        systemData[i].rgb.red = 0;
        systemData[i].rgb.green = 0;
        systemData[i].rgb.blue = 0;
        systemData[i].murk = 0;
        systemData[i].reinforceFleet = -1;
        systemData[i].reinforceTime = 0;
        systemData[i].reinforceDaysLeft = 0;
        systemData[i].reinforceInterval = 0;
        systemData[i].reinforceTimer = 0.0f;
        strncpy((char *)systemData[i].name, (char *)"\p", 31);
    }

    for (i = 0; i < kMaxDudes; i++) {
        dudeData[i].loaded = 0;
        for (j = 0; j < kDudeShipSlots; j++)
            dudeData[i].shipTypes[j] = -1;
    }

    for (i = 0; i < kMaxPersons; i++) {
        personData[i].loaded = 0;
        personData[i].canAppear = 0;
        personData[i].prepared = 0;
        personData[i].linkSyst = 0;
        personData[i].grantClass = -1;
        personData[i].grantProb = 0;
        personData[i].grantCount = 0;
        for (j = 0; j < kMaxWeapTypes; j++) {
            personData[i].weapCount[j] = 0;
            personData[i].weapAmmo[j] = 0;
        }
        personData[i].subtitle[0] = 0;
    }

    for (i = 0; i < kMaxFleets; i++) {
        fleetData[i].leadShipType = -1;
        fleetData[i].govt = -1;
        fleetData[i].usable = 0;
    }

    for (i = 0; i < kMaxRanks; i++) {
        rankData[i].index = i;
        rankData[i].active = 0;
        rankData[i].govt = -1;
        rankData[i].flags = 0;
    }

    for (i = 0; i < kMaxAsteroids; i++)
        asteroidData[i].active = 0;

    for (roid = 0; roid < kMaxRoidTypes; roid++) {
        asteroidType[roid].strength = 100;
        asteroidType[roid].yieldQty = 0;
        asteroidType[roid].yieldType = -1;
        asteroidType[roid].fragType[0] = -1;
        asteroidType[roid].fragType[1] = -1;
        asteroidType[roid].fragCount = 0;
        asteroidType[roid].partCount = 50;
        asteroidType[roid].frameRate = 1.0f;
        if (gSpriteWorld->pixelDepth == 8) {
            c.red = c.green = c.blue = kRoidGrey;
            asteroidType[roid].color = Color2Index(&c);
        } else {
            asteroidType[roid].color = kRoidPixel;
        }
    }

    for (i = 0; i < kMaxNebulae; i++) {
        nebulaData[i].loaded = 0;
        nebulaData[i].explored = 0;
    }

    for (i = 0; i < kMaxJunks; i++) {
        junkData[i].scanMask = 0;
        for (j = 0; j < 8; j++) {
            junkData[i].boughtAt[j] = -1;
            junkData[i].soldAt[j] = -1;
            pstrcpy(junkData[i].name, "\p");
            pstrcpy((unsigned char *)junkData[i].abbrev, "\p");
            pstrcpy((unsigned char *)junkData[i].lcName, "\p");
        }
    }

    for (i = 0; i < kMaxShots; i++) {
        shotData[i].weapType = 0;
        shotData[i].life = -2.0f;
    }

    for (i = 0; i < kMaxBoxes; i++)
        boxData[i].timer = -2.0f;

    for (i = 0; i < kMaxStars; i++) {
        starData[i].live = 0;
        starData[i].frame = 0;
        starData[i].depth = 1.0f;
    }

    for (i = 0; i < kMaxPods; i++)
        podData[i].life = -1.0f;

    for (i = 0; i < kMaxBeams; i++) {
        beamData[i].state = -2;
        beamData[i].owner = -1;
    }

    for (i = 0; i < kMaxWeapTypes; i++) {
        weaponStats[i].guidance = kResAbsent;
        for (j = 0; j < 4; j++)
            weaponStats[i].jamVuln[j] = 0;
    }

    for (i = 0; i < kEscortCount; i++)
        playerEscortOrders[i] = -1;

    for (i = 0; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;

    for (i = 0; i <= kCargoTypes - 1; i++) {
        if (!LoadPluginString(i + kBasePriceStringID, buf))
            GetIndString(buf, kBasePriceListID, i + 1);
        StringToNum(buf, &value);
        basePrice[i] = value;
    }

    for (i = 0; i < kMaxMissions; i++)
        missionRandom[i] = Rand(kDailyRollRange) + 1;

    for (i = 0; i < kMaxCrons; i++)
        cronEvent[i].loaded = 0;

    for (i = 0; i < kMaxDisasters; i++)
        disasterData[i].loaded = 0;

    for (i = 0; i < kMaxGovts; i++) {
        for (j = 0; j < 4; j++) {
            govtData[i].allies[j] = -1;
            govtData[i].enemies[j] = -1;
            govtData[i].classes[j] = -1;
        }
        for (j = 0; j < 4; j++)
            govtData[i].inhJam[j] = 0;
        govtData[i].maxOdds = 2.0f;
        govtData[i].skillMult = 1.0f;
        govtData[i].scanFine = 0;
        govtData[i].iffScrambled = 0;
        govtData[i].reinfInhibited = 0;
        govtData[i].voiceType = -1;
        govtData[i].voiceClass = -1;
        govtData[i].interfaceID = -1;
        govtData[i].loaded = 0;
    }

    strictPlay = 1;
    allReinforcementsInhibited = 0;
    alertSpritePending = -1;
    dockPermission = dockTarget = -1;
    GetTime(&gameTime);
    gameTime.year += kGameYearOffset;
    pstrcpy(lastPlayerName, "\p");
    pstrcpy(playerName, "\pNo Name");
    pstrcpy(shipName, "\pNo Name");
    descResourcePictId = 0;
    pstrcpy(descMovieName, "\p");
    leetMode = 0;
    escortMenuOnscreen = 0;
    showFloatingMap = -1;
    showFPS = 0;
    playerSabotaged = 0;
    recentActivatedRank = -1;
    lastCursorAnimTime = 0;
    lastFrameTime = 0;
#ifdef MACBUILD
    lastFrameMicroseconds.hi = 0;
    lastFrameMicroseconds.lo = 0;
#else
    Microseconds(&lastFrameMicroseconds);
#endif
    SetRect(&statusOffRect[0], 30, 8, 163, 142);
    SetRect(&statusOffRect[1], 99, 155, 173, 161);
    SetRect(&statusOffRect[2], 99, 170, 173, 176);
    SetRect(&statusOffRect[3], 99, 185, 173, 191);
    SetRect(&statusOffRect[4], 9, 204, 184, 239);
    SetRect(&statusOffRect[5], 9, 251, 184, 268);
    SetRect(&statusOffRect[6], 9, 280, 184, 395);
    SetRect(&statusOffRect[7], 9, 407, 184, 495);
    SetRect(&progressDefRect, -95, -15, 95, 15);
    statusColor[0].red = 0xffff;
    statusColor[0].green = 0xffff;
    statusColor[0].blue = 0xffff;
    statusColor[1].red = 0x7fff;
    statusColor[1].green = 0x7fff;
    statusColor[1].blue = 0x7fff;
    statusColor[2].red = 0xffff;
    statusColor[2].green = 0xffff;
    statusColor[2].blue = 0xffff;
    statusColor[3].red = 0x7fff;
    statusColor[3].green = 0x7fff;
    statusColor[3].blue = 0x7fff;
    statusColor[4].red = 0xbd00;
    statusColor[4].green = 0;
    statusColor[4].blue = 0;
    statusColor[5].red = 0xa600;
    statusColor[5].green = 0xa600;
    statusColor[5].blue = 0xa600;
    statusColor[6].red = 0x4c00;
    statusColor[6].green = 0x5c00;
    statusColor[6].blue = 0x6f00;
    statusColor[7].red = 0x3c00;
    statusColor[7].green = 0x4900;
    statusColor[7].blue = 0x5800;
    menuTextColor[0].red = 0xffff;
    menuTextColor[0].green = 0;
    menuTextColor[0].blue = 0;
    menuTextColor[1].red = 0x84d0;
    menuTextColor[1].green = 0;
    menuTextColor[1].blue = 0;
    statusFontID = 0;
#ifdef MAC1010 /* BUG: overwritten by the next line */
    statusFontSize[0] = 12;
#endif
    statusFontSize[0] = 11;
    menuFontID = 3;
    menuTextSize = 9;
    playerIsLanded = 0;
    ignoreKeys = 0;
    GetFNum("\pGeneva", &smallTextFont);
    smallTextSize = 9;
    hypergateMapMode = 0;
    hypergateMapModeStellar = -1;
}


#define kStartingCredits  10000
#define kAdviceRange      0x800     /* Rand() range for the opening advice pick */
#define kPriceFluxBase    100       /* 100% == no price movement yet */
#define kNewbieUnset      0x7fff
#define kPlayerPaintLevel 0x20      /* the default ship tint, applied to all three guns */

#define kStartY           50.0f
#define kNoDeathTimer     (-999.0f)
#define kFreeSlot         (-1.0f)   /* boom/debris timer: < 0 means unused */

/* ResetPlayer (mac _ResetPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b3350
// MAC: _ResetPlayer i386 0x0001d40d ppc 0x000219c8
void ResetPlayer(Boolean newPilot)
{
    short i;
    short link;
    short coloc;
    short j;

    shipData->pos.x = shipData->pos.y = kStartY;
    shipData->vel.x = shipData->vel.y = 0.0f;
    shipData->accelMag = 0.0f;
    shipData->heading = 0.0f;
    shipData->type = 0;
    shipData->active = 1;
    shipData->cloakDir = 0;
    shipData->cloak = 0.0f;
    shipData->navTargetKind = -1;
    shipData->govt = -1;
    shipData->aiType = -1;
    shipData->curSystem = 0;
    shipData->shield = ShipShieldCapacity(shipData);
    shipData->armor = ShipArmorCapacity(shipData);
    shipData->fuel = ShipFuelCapacity(shipData);
    shipData->targetShip = -1;
    shipData->targetStellar = -1;
    shipData->navTargetKind = -1;
    shipData->selectedWeapon = -1;
    shipData->parentShip = -1;
    shipData->specialMission = -1;
    shipData->persType = -1;
    shipData->aiState = shipData->aiSubstate = kSubLoJack;
    shipData->deathTimer = kNoDeathTimer;
    shipData->ionization = 0.0f;
    shipData->ionColor = 0;
    shipData->lightLevel = 0.0f;
    shipData->blinkCount = 0;
    shipData->altSet = 0;
    shipData->animSet = 0;
    shipData->foldDir = 1;
    shipData->foldSet = 0;
    shipData->frameTimer = 0.0f;
    shipData->bankDir = 0;
    shipData->shieldGlow = 0;
    shipData->weapGlow = 0;
    shipData->annoyance = 0.0f;
    shipData->animTimer = 0.0f;
    shipData->skill = 1.0f;
    shipData->jumpTimer = 0.0f;
    shipData->prevSystem = -1;
    shipData->engineGlow = 0;
    shipData->fireTick = 0;
    shipData->tractorShip = -1;
    shipData->tractorTick = 0;

    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = shipClass[shipData->type].weapCount[i];
        if (shipClass[shipData->type].weapAmmo[i] > 0) {
            if (weaponStats[i].ammoType >= 0 && weaponStats[i].ammoType < kMaxWeapTypes)
                shipData->weap[weaponStats[i].ammoType].ammo = shipClass[shipData->type].weapAmmo[i];
            else
                shipData->weap[i].ammo = shipClass[shipData->type].weapAmmo[i];
        }
    }
    for (i = 0; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;
    for (i = 0; i < kMaxOutfits; i++) {
        if (newPilot || outfitSystem[i].persistent == 0)
            shipSystems[i] = 0;
    }
    for (i = 0; i <= kCargoTypes - 1; i++)
        shipData->cargo[i] = 0;
    for (i = 0; i < kMaxJunks; i++)
        junkData[i].held = 0;

    antiEscortDelay = -1;
    lastMissionStellar = -1;
    shipData->escapePods = -1;
    destructCount = kFreeSlot;
    dockTarget = -1;
    dockPermission = -1;
    adviceRandom = Rand(kAdviceRange);
    bribeRandom = -1;
    playerThreatened = oldPlayerThreatened = 0;
    fadePending[0] = fadePending[1] = 0;
    newbieFlag = kNewbieUnset;
    alertSpritePending = -1;

    for (i = 0; i < kMaxMissions; i++)
        missionRandom[i] = Rand(kDailyRollRange) + 1;
    for (i = 0; i < kMaxExplods; i++) {
        explodData[i].timer = kFreeSlot;
        explodData[i].delay = 0.0f;
    }
    for (i = 0; i < kMaxDebris; i++) {
        smokeData[i].timer = kFreeSlot;
        smokeData[i].angle = 0.0f;
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        shipClass[i].dailyHireRoll = Rand(kDailyRollRange) + 1;
        shipClass[i].dailySaleRoll = Rand(kDailyRollRange) + 1;
    }
    for (i = 0; i < kMaxOutfits; i++)
        outfitSystem[i].dailyRoll = Rand(kDailyRollRange) + 1;

    killShots = 1;
    killBoxes = 1;
    killExplods = 1;
    killSmoke = 1;
    weapLawFlag = 0;
    smuggleFlag = 0;
    greenMissionSyst = -1;
    commCount = 0;
    missionSoundID = -1;

    for (j = 0; j < kMaxActiveMissions; j++) {
        if (currentMissionObjectives[j].active != 0)
            AbortMission(j, 0);
        currentMissionObjectives[j].active = 0;
    }
    for (j = 0; j < kMaxMissions; j++)
        missionRandom[j] = Rand(kDailyRollRange) + 1;

    for (i = 0; i <= 1; i++) {
        globalPriceFlux[i] = kPriceFluxBase;
        localPriceFlux[i] = kPriceFluxBase;
    }

    if (newPilot) {
        shipData->credits = kStartingCredits;
        for (i = 0; i < kMaxSystems; i++) {
            systemData[i].exploreLevel = 0;
            if (systemData[i].govt >= 0 && systemData[i].govt < kMaxGovts)
                playerRecord[i] = govtData[systemData[i].govt].initialRec;
            else
                playerRecord[i] = 0;
            systemData[i].reinforceDaysLeft = 0;
            systemData[i].reinforceTimer = 0.0f;
        }
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[shipData->curSystem].links[i] != -1 && systemData[systemData[shipData->curSystem].links[i]].loaded != 0) {
                link = systemData[shipData->curSystem].links[i];
                systemData[link].exploreLevel = 1;
                coloc = FindFirstCoLocatedSystem(link);
                if (coloc >= 0 && coloc < kMaxSystems)
                    systemData[coloc].exploreLevel = 1;
            }
        }
        for (i = 0; i < kMaxNebulae; i++)
            nebulaData[i].explored = 0;
        for (i = 0; i < kMaxPersons; i++)
            personData[i].grudge = 0;
        for (i = 0; i < kEscortCount; i++)
            playerEscortOrders[i] = -1;
    }
    mapScroll.h = systemData[shipData->curSystem].pos.h;
    mapScroll.v = systemData[shipData->curSystem].pos.v;
    systemData[shipData->curSystem].reinforceTimer = kFreeSlot;
    systemData[shipData->curSystem].reinforceDaysLeft = 0;
    lastMissionStellar = -1;
    recentActivatedRank = -1;
    playerPaint.red = playerPaint.green = playerPaint.blue =
        kPlayerPaintLevel;
    ResetPlayerPrecalcedValues();
}

/* ResetPlayerRecord. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4220
// MAC: _ResetPlayerRecord i386 0x0001db70 ppc 0x0002231c
void ResetPlayerRecord(Boolean newPilot)
{
    short i;

    if (newPilot != 0)
        playerKills = 0;

    for (i = 0; i < kMaxSystems; i++) {
        short minStatus;

        if (systemData[i].govt >= 0)
            minStatus = govtData[systemData[i].govt].initialRec;
        else
            minStatus = 0;
        if (playerRecord[i] < minStatus)
            playerRecord[i] = minStatus;
    }
    for (i = 0; i < kMaxStellars; i++) {
        if ((stellarData[i].flags2 & kSpobAlwaysDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
    }
    recentActivatedRank = 0xffff;
}

#ifdef MACBUILD
#define kMinSystemVersion    0x810       /* Mac OS 8.1 */
#define kMinCarbonVersion    0x131       /* CarbonLib 1.3.1, before Mac OS X */
#define kMinQuickTimeVersion 0x3000000   /* QuickTime 3.0 */

/* CheckSystem (mac _CheckSystem, mac-only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _CheckSystem i386 0x0001dc00 ppc 0x000223e8
void CheckSystem(void)
{
    Str255 msg;
    int32_t response;
    OSErr err;

    err = Gestalt(gestaltSystemVersion, &gSystemVersion);
    crashlog_print("system software version is ");
    sprintf((char *)msg, "%04X", gSystemVersion);
    crashlog_print((char *)msg);
    crashlog_print("\n");
    if (gSystemVersion < kMinSystemVersion || err != noErr) {
        FadeScreenIn();
        GetIndString(msg, kAlertStrList, 40);
        EVNoteAlert(msg);
        ShowTheMenuBar();
        ST_Close();
        ExitToShell();
    }
#if TARGET_API_MAC_CARBON
    if (gSystemVersion < kMacOSXSystemVersion) {
        Gestalt(gestaltCarbonVersion, &response);
        if (response < kMinCarbonVersion) {
            FadeScreenIn();
            GetIndString(msg, kAlertStrList, 41);
            EVNoteAlert(msg);
            ShowTheMenuBar();
            ST_Close();
            ExitToShell();
        }
    }
#endif
    oldDepth = 8;
    err = Gestalt(gestaltQuickTimeVersion, &response);
    if (response < kMinQuickTimeVersion || err != noErr) {
        FadeScreenIn();
        GetIndString(msg, kAlertStrList, 42);
        EVNoteAlert(msg);
        ShowTheMenuBar();
        ST_Close();
        ExitToShell();
    }
#if V111
    {
        CGRect bounds;

        bounds = CGDisplayBounds(CGMainDisplayID());
        mainWindRect.top = bounds.origin.x;
        mainWindRect.left = bounds.origin.y;
        mainWindRect.bottom = bounds.size.height;
        mainWindRect.right = bounds.size.width;
    }
#elif defined(MAC1010)
    mainWindRect = (*mainScreenDevice)->gdRect;
    if (mainWindRect.right - mainWindRect.left < 800 || mainWindRect.bottom - mainWindRect.top < 600) {
        GetIndString(msg, kAlertStrList, 38);
        if (!EVYesNoDialog(msg))
            ExitToShell();
    }
#endif
}

/* BailCheck (mac _BailCheck, mac-only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _BailCheck i386 0x0001ddd0 ppc 0x000225e0
void BailCheck(void)
{
    if (KeyCheck(kEscapeKeyCode)) {
        FlushEvents(everyEvent - diskMask, 0);
        ShowTheMenuBar();
        ST_Close();
        ExitToShell();
    }
}

#endif
/*
 * The bounds come from the video layer's current mode rather than from a
 * window, which is why the two sources are BurgerLib globals.
 */

#ifndef MACBUILD
/* ResetScreenRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b42f0
void ResetScreenRect(void)
{
    mainWindRect.top = 0;
    mainWindRect.left = 0;
    mainWindRect.bottom = gVideoHeight;
    mainWindRect.right = gVideoWidth;
}
#endif

/*
 * No mac twin: 1.1.1 rewrote its preferences layer for OS X (_LoadPrefs is 2243
 * bytes and EVPrefsDialogController is Objective-C), so these two names
 * describe the body.
 */

#ifdef MAC1010
void SetDefaultPrefs(void)
{
    short depth;

    if (gSpriteWorld != NULL)
        depth = gSpriteWorld->pixelDepth;
    else
        depth = (*(*GetMainDevice())->gdPMap)->pixelSize;

    settings.introMusic = 1;
    settings.voicesOn = 1;
    settings.volume = 5;
    settings.clipping = 0;
    settings.noMovies = 0;
    settings.noDebris = 0;
    settings.windowed = 0;
    settings.yieldEvents = 1;
    settings.unused = 0;
    settings.shipAnimations = 1;
    settings.engineGlows = 1;
    settings.parallaxStars = 1;
    settings.ambientSounds = 1;
    settings.noHyperGamma = 0;
    settings.noVersionCheck = 0;
    settings.showMapBorders = 0;
    if (depth >= 16) {
        settings.weaponGlows = 1;
        settings.runningLights = 1;
    } else {
        settings.weaponGlows = 0;
        settings.runningLights = 0;
    }
    DAT_00735488 = 1;
    pstrcpy(lastPlayerName, "\p");
}

void SetDefaultKeys(void)
{
    keyPrefs.selectSecondary = 0x5;
    keyPrefs.deselectSecondary = 0x9;
    keyPrefs.firePrimary = 0x39;
    keyPrefs.fireSecondary = 0x33;
    keyPrefs.hail = 0x18;
    keyPrefs.land = 0x2d;
    keyPrefs.dismiss = 0x2c;
    keyPrefs.faceTarget = 0x8;
    keyPrefs.clearTarget = 0x3a;
    keyPrefs.map = 0x26;
    keyPrefs.targetNext = 0x38;
    keyPrefs.targetNearest = 0x7;
    keyPrefs.hyperspace = 0xc;
    keyPrefs.nextSystem = 0x22;
    keyPrefs.jump = 0x2e;
    keyPrefs.board = 0x3;
    keyPrefs.turnLeft = 0x73;
    keyPrefs.turnRight = 0x74;
    keyPrefs.accelerate = 0x76;
    keyPrefs.reverse = 0x75;
    keyPrefs.pause = 0x3d;
    keyPrefs.afterburner = 0xe;
    keyPrefs.playerInfo = 0x2b;
    keyPrefs.navPreset[0] = 0x72;
    keyPrefs.navPreset[1] = 0x70;
    keyPrefs.navPreset[2] = 0x6b;
    keyPrefs.navPreset[3] = 0x7e;
    keyPrefs.cheatSpawn = 0x7a;
    keyPrefs.cheatKill = 0x7d;
    keyPrefs.cheatRevive = 0x7b;
    keyPrefs.cheatArmor = 0x7c;
    keyPrefs.cheatDamage = 0x71;
    keyPrefs.cheatAmmo = 0x4b;
    keyPrefs.cheatCredits = 0x1d;
    keyPrefs.cheatSpare = 0x7f;
    keyPrefs.cheatShipNext = 0x4d;
    keyPrefs.cheatShipPrev = 0x46;
    keyPrefs.missions = 0x2a;
    keyPrefs.cloak = 0x28;
    keyPrefs.toggleFPS = 0x67;
    keyPrefs.escortMenu = 0x6;
    keyPrefs.jettison = 0x20;
    keyPrefs.eject = 0xf;
    keyPrefs.selfDestruct = 0x13;
    keyPrefs.quickKey[0] = 0x1a;
    keyPrefs.quickKey[1] = 0x1b;
    keyPrefs.quickKey[2] = 0x1c;
    keyPrefs.quickKey[3] = 0x1d;
    keyPrefs.quickKey[4] = 0x1f;
    keyPrefs.escortOrder[0] = 0xb;
    keyPrefs.escortOrder[1] = 0xa;
    keyPrefs.escortOrder[2] = 0x1;
    keyPrefs.escortOrder[3] = 0x0;
}
#endif

#ifndef MACBUILD
/* SetDefaultPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4320
void SetDefaultPrefs(void)
{
    short depth;

    if (gSpriteWorld != NULL)
        depth = gSpriteWorld->pixelDepth;
    else
        depth = 16;

    settings.introMusic = 1;
    settings.voicesOn = 1;
    settings.volume = 5;
    settings.clipping = 0;
    settings.noMovies = 0;
    settings.noDebris = 0;
    settings.windowed = 0;
    settings.yieldEvents = 1;
    settings.unused = 0;
    settings.shipAnimations = 1;
    settings.engineGlows = 1;
    settings.parallaxStars = 1;
    settings.ambientSounds = 1;
    settings.noHyperGamma = 0;
    settings.noVersionCheck = 1;
    settings.showMapBorders = 0;
    if (depth >= 16) {
        settings.weaponGlows = 1;
        settings.runningLights = 1;
    } else {
        settings.weaponGlows = 0;
        settings.runningLights = 0;
    }
    settings.gamma = 3;
    DAT_00735488 = 1;
    pstrcpy(lastPlayerName, "\p");
}

/* SetDefaultKeys. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4400
void SetDefaultKeys(void)
{
    keyPrefs.selectSecondary = 0x11;
    keyPrefs.deselectSecondary = 0x1f;
    keyPrefs.firePrimary = 0x39;
    keyPrefs.fireSecondary = 0x1d;
    keyPrefs.hail = 0x15;
    keyPrefs.land = 0x26;
    keyPrefs.dismiss = 0x1c;
    keyPrefs.faceTarget = 0x1e;
    keyPrefs.clearTarget = 0x31;
    keyPrefs.map = 0x32;
    keyPrefs.targetNext = 0x29;
    keyPrefs.targetNearest = 0x13;
    keyPrefs.hyperspace = 0x23;
    keyPrefs.nextSystem = 0x2b;
    keyPrefs.jump = 0x24;
    keyPrefs.board = 0x30;
    keyPrefs.turnLeft = 0x63;
    keyPrefs.turnRight = 0x64;
    keyPrefs.accelerate = 0x61;
    keyPrefs.reverse = 0x66;
    keyPrefs.pause = 1;
    keyPrefs.afterburner = 0x2c;
    keyPrefs.playerInfo = 0x19;
    keyPrefs.navPreset[0] = 0x3b;
    keyPrefs.navPreset[1] = 0x3c;
    keyPrefs.navPreset[2] = 0x3d;
    keyPrefs.navPreset[3] = 0x3e;
    keyPrefs.cheatSpawn = 0x68;
    keyPrefs.cheatKill = 0x69;
    keyPrefs.cheatRevive = 0x60;
#ifdef CEFIX /* F21.11-13 */
    keyPrefs.cheatArmor = kCEScanNone;
    keyPrefs.cheatDamage = kCEScanNone;
    keyPrefs.cheatAmmo = kCEScanF6;
#else
    keyPrefs.cheatArmor = 0x62;
    keyPrefs.cheatDamage = 0x67;
    keyPrefs.cheatAmmo = 0x37;
#endif
    keyPrefs.cheatCredits = 5;
    keyPrefs.cheatSpare = 0x65;
#ifdef CEFIX /* F21.14-15 */
    keyPrefs.cheatShipNext = kCEScanPageUp;
    keyPrefs.cheatShipPrev = kCEScanPageDown;
#else
    keyPrefs.cheatShipNext = 0x4e;
    keyPrefs.cheatShipPrev = 0x4a;
#endif
    keyPrefs.missions = 0x17;
    keyPrefs.cloak = 0x16;
    keyPrefs.toggleFPS = 0x58;
    keyPrefs.escortMenu = 0x12;
    keyPrefs.jettison = 0x25;
    keyPrefs.eject = 0x2d;
    keyPrefs.selfDestruct = 0xc;
    keyPrefs.quickKey[0] = 2;
    keyPrefs.quickKey[1] = 3;
    keyPrefs.quickKey[2] = 4;
    keyPrefs.quickKey[3] = 5;
    keyPrefs.quickKey[4] = 6;
    keyPrefs.escortOrder[0] = 0x21;
    keyPrefs.escortOrder[1] = 0x20;
    keyPrefs.escortOrder[2] = 0x2f;
    keyPrefs.escortOrder[3] = 0x2e;
}
#endif

#pragma opt_repositioncode off


/* 0x0058f8b4 is the IAT slot, not a variable: the exe reaches
   GetSystemMetrics through it because the declaration is dllimport. */
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define kModeFlagBase   2
#define kModeDepth      15
#define kModeFailAlert  0x25
#ifdef MAC1010
#define kMonitorFailAlert 0x2b
#if TARGET_API_MAC_CARBON
/* a display mode as the Carbon gamma library hands it back; the fields read are named */
typedef struct ScreenSetting {
    int32_t unk0;
    Fixed refreshRate;
    int32_t height;
    int32_t width;
    char unk10[7];
    Boolean unk17;
    char unk18[0x20];
} ScreenSetting;
extern ScreenRef NewScreenRef(GDHandle device, int32_t *err);
extern int32_t GetCurrentSetting(ScreenRef screen, ScreenSetting *setting);
extern int32_t GetColorDepthsAtThisResolution(ScreenRef screen);
extern int32_t GetGamma(ScreenRef screen, ScreenGamma *gamma);
extern int32_t SetColorDepth(ScreenRef screen, short depth);
extern pascal void GetAvailableWindowPositioningBounds(GDHandle device, Rect *bounds);
extern ScreenGamma gFadeGamma;
#endif
#endif

/* SetupMonitor. STATE: MATCH 100%, exe 46 / obj 46, byte-identical yes (under its
   scoped opt_repositioncode pragma).
   OPEN: none */
// FUNCTION: EVNOVA 0x004b45e0
// MAC: _SetupMonitor i386 0x0001e699 ppc 0x00022fcc
void SetupMonitor(void)
{
#ifdef MAC1010
#if TARGET_API_MAC_CARBON
    int32_t err;
    ScreenSetting setting;
    short depths;

    mainScreenDevice = GetMainDevice();
    crashlog("getting available window positioning bounds");
    GetAvailableWindowPositioningBounds(mainScreenDevice, &screenMinusDockRect);
    crashlog("opening RezLib");
    gScreenRef = NewScreenRef(0, &err);
    if (err != 0) {
        crashlog_print("NewScreenRef returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kMonitorFailAlert);
    }
    setting.unk17 = 0;
    err = GetCurrentSetting(gScreenRef, &setting);
    if (err != 0) {
        crashlog_print("GetCurrentSetting returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kMonitorFailAlert);
    }
    startupDepth = (*(*mainScreenDevice)->gdPMap)->pixelSize;
    crashlog_print("main display at startup was ");
    crashlog_print_num(setting.width);
    crashlog_print("x");
    crashlog_print_num(setting.height);
    crashlog_print(" / ");
    crashlog_print_num(startupDepth);
    crashlog_print(" bits / ");
    crashlog_print_num((uint32_t)setting.refreshRate >> 16);
    crashlog_print(" Hz\n");
    if (startupDepth != 8 && startupDepth != 16) {
        depths = GetColorDepthsAtThisResolution(gScreenRef);
        err = GetGamma(gScreenRef, &gFadeGamma);
        if (err != 0) {
            crashlog_print("GetGamma returned ");
            crashlog_print_num(err);
            crashlog_print("\n");
            FatalStartupError(kMonitorFailAlert);
        }
        err = 0;
        if (depths & 0x10) {
            crashlog("switching to 16 bits");
            err = SetColorDepth(gScreenRef, 16);
            Delay(30, &tix);
            if (err != 0) {
                crashlog_print("SetColorDepth returned ");
                crashlog_print_num(err);
                crashlog_print("\n");
            }
        } else if (depths & 8) {
            crashlog("switching to 8 bits");
            err = SetColorDepth(gScreenRef, 8);
            Delay(30, &tix);
            if (err != 0) {
                crashlog_print("SetColorDepth returned ");
                crashlog_print_num(err);
                crashlog_print("\n");
            }
        } else
            FatalStartupError(0x2e);
        if (err != 0) {
            crashlog_print("SetColorDepth returned ");
            crashlog_print_num(err);
            crashlog_print("\n");
            FatalStartupError(0x2f);
        }
    }
    GetGamma(gScreenRef, &gOrigGamma);
    mainScreenDevice = GetMainDevice();
    if ((*(*mainScreenDevice)->gdPMap)->pixelSize != 8
            && (*(*mainScreenDevice)->gdPMap)->pixelSize != 16)
        FatalStartupError(kModeFailAlert);
    mainDeviceInitBounds = (*(*mainScreenDevice)->gdPMap)->bounds;
    InitCluts();
#else
    VTRequest wanted;
    VTRequest current;
    int err;
    short e;

    crashlog("opening monitor tool");
    e = MT_Open();
    err = e;
    if (err != 0) {
        crashlog_print("MT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(0x2d);
    }
    mainScreenDevice = GetMainDevice();
    MemoryClear(&wanted, sizeof(VTRequest));
    e = MT_GetMode(1, &current);
    err = e;
    if (err == 0) {
        crashlog_print("main display at startup was ");
        crashlog_print_num(current.width);
        crashlog_print("x");
        crashlog_print_num(current.height);
        crashlog_print(" / ");
        crashlog_print_num(current.depth);
        crashlog_print(" bits / ");
        crashlog_print_num(current.rate);
        crashlog_print(" Hz\n");
        wanted.rate = current.rate;
        wanted.rateRule = 1;
        wanted.depth = current.depth;
        wanted.width = current.width;
        wanted.height = current.height;
    } else {
        crashlog_print("MT_GetMode returned ");
        crashlog_print_num(err);
        crashlog_print(", ignoring refresh rate\n");
        wanted.rateRule = 0;
        wanted.rate = 0;
        wanted.width = (*(*mainScreenDevice)->gdPMap)->bounds.right - (*(*mainScreenDevice)->gdPMap)->bounds.left;
        wanted.height = (*(*mainScreenDevice)->gdPMap)->bounds.bottom - (*(*mainScreenDevice)->gdPMap)->bounds.top;
        wanted.depth = (*(*mainScreenDevice)->gdPMap)->pixelSize;
    }
    if (wanted.width < 800 && wanted.height < 600) {
        wanted.width = 800;
        wanted.height = 600;
    }
    if (wanted.depth < 8)
        wanted.depth = 8;
    if (wanted.depth > 16)
        wanted.depth = 16;
    wanted.sizeRule = 1;
    wanted.depthRule = 1;
    wanted.colorRule = 0;
    wanted.color = 1;
    crashlog_print("attempting to set main display to ");
    crashlog_print_num(wanted.width);
    crashlog_print("x");
    crashlog_print_num(wanted.height);
    crashlog_print(" / ");
    crashlog_print_num(wanted.depth);
    crashlog_print(" bits / ");
    if (wanted.rateRule != 0) {
        crashlog_print_num(wanted.rate);
        crashlog_print(" Hz\n");
    } else
        crashlog_print(" any refresh rate\n");
    if ((*(*mainScreenDevice)->gdPMap)->pixelSize < 8 || (*(*mainScreenDevice)->gdPMap)->pixelSize > 16) {
        crashlog_print("setting main display to ");
        crashlog_print_num(wanted.width);
        crashlog_print("x");
        crashlog_print_num(wanted.height);
        crashlog_print(" / ");
        crashlog_print_num(wanted.depth);
        crashlog_print(" bits / ");
        if (wanted.rateRule != 0) {
            crashlog_print_num(wanted.rate);
            crashlog_print(" Hz\n");
        } else
            crashlog_print(" any refresh rate\n");
        e = MT_SetMode(1, &wanted);
        err = e;
        if (err != 0) {
            crashlog_print("MT_SetMode returned ");
            crashlog_print_num(err);
            crashlog_print(", trying again\n");
            wanted.rateRule = 0;
            crashlog_print("setting main display to ");
            crashlog_print_num(wanted.width);
            crashlog_print("x");
            crashlog_print_num(wanted.height);
            crashlog_print(" / ");
            crashlog_print_num(wanted.depth);
            crashlog_print(" bits / any refresh rate (using main screen device)\n");
            e = MT_SetMode(1, &wanted);
            err = e;
            if (err != 0) {
                crashlog_print("MT_SetMode returned ");
                crashlog_print_num(err);
                crashlog_print(", trying again\n");
                wanted.sizeRule = 0;
                crashlog_print("setting main display to ");
                crashlog_print_num(wanted.depth);
                crashlog_print(" bits / any resolution / any refresh rate (using main screen device)\n");
                e = MT_SetMode(1, &wanted);
                err = e;
                if (err != 0) {
                    crashlog_print("MT_SetMode returned ");
                    crashlog_print_num(err);
                    crashlog_print(", exiting\n");
                    FatalStartupError(0x2c);
                }
            }
        }
    }
    mainScreenDevice = GetMainDevice();
    if ((*(*mainScreenDevice)->gdPMap)->pixelSize != 8
            && (*(*mainScreenDevice)->gdPMap)->pixelSize != 16)
        FatalStartupError(kModeFailAlert);
    mainDeviceInitBounds = (*(*mainScreenDevice)->gdPMap)->bounds;
    InitCluts();
#endif
#elif defined(MACBUILD)
#if V111
    CGDisplayConfigRef config;
    CFNumberRef refreshNumber;
    int32_t refreshRate;
    boolean_t exactMatch;
    CFNumberRef depthNumber;
    int32_t depth;
    CGRect bounds;
    CFDictionaryRef mode;

    if (useLegacyRenderingPipeline) {
        mainScreenDevice = GetMainDevice();
        crashlog("getting available window positioning bounds");
        GetAvailableWindowPositioningBounds(mainScreenDevice, &screenMinusDockRect);
        startupDepth = CGDisplayBitsPerPixel(CGMainDisplayID());
        bounds = CGDisplayBounds(CGMainDisplayID());
        mode = CGDisplayCurrentMode(CGMainDisplayID());
        if (mode != NULL) {
            CFDictionaryGetValueIfPresent(mode, CFSTR("RefreshRate"), (const void **)&refreshNumber);
            if (refreshNumber != NULL)
                CFNumberGetValue(refreshNumber, kCFNumberIntType, &refreshRate);
        }
        crashlog_print("main display at startup was ");
        crashlog_print_num(bounds.size.width);
        crashlog_print("x");
        crashlog_print_num(bounds.size.height);
        crashlog_print(" / ");
        crashlog_print_num(startupDepth);
        crashlog_print(" bits / ");
        crashlog_print_num(refreshRate);
        crashlog_print(" Hz\n");
        if (startupDepth != 8 && startupDepth != 16) {
            exactMatch = 0;
            mode = CGDisplayBestModeForParameters(CGMainDisplayID(), 16, bounds.size.width,
                                                  bounds.size.height, &exactMatch);
            if (mode == NULL)
                goto done;
            CFDictionaryGetValueIfPresent(mode, CFSTR("BitsPerPixel"), (const void **)&depthNumber);
            if (depthNumber == NULL)
                goto done;
            CFNumberGetValue(depthNumber, kCFNumberIntType, &depth);
            if (depth != 16) {
                mode = CGDisplayBestModeForParameters(CGMainDisplayID(), 8, bounds.size.width,
                                                      bounds.size.height, &exactMatch);
                if (mode != NULL) {
                    /* BUG: passes the CFNumberRef, not its address */
                    CFDictionaryGetValueIfPresent(mode, CFSTR("BitsPerPixel"), (const void **)depthNumber);
                    if (depthNumber != NULL)
                        CFNumberGetValue(depthNumber, kCFNumberIntType, &depth);
                }
            }
            if ((exactMatch || depth == 16 || depth == 8)
                    && CGBeginDisplayConfiguration(&config) == kCGErrorSuccess) {
                FadeScreenOut();
                CGConfigureDisplayMode(config, CGMainDisplayID(), mode);
                CGCompleteDisplayConfiguration(config, kCGConfigureForAppOnly);
            } else {
                CGCancelDisplayConfiguration(config);
                FatalStartupError(46);
            }
        }
    } else {
        mainScreenDevice = GetMainDevice();
        GetAvailableWindowPositioningBounds(mainScreenDevice, &screenMinusDockRect);
        startupDepth = (*(*mainScreenDevice)->gdPMap)->pixelSize;
    }
done:
#endif
    mainScreenDevice = GetMainDevice();
    if (useLegacyRenderingPipeline) {
        if ((*(*mainScreenDevice)->gdPMap)->pixelSize != 8
                && (*(*mainScreenDevice)->gdPMap)->pixelSize != 16)
            FatalStartupError(kModeFailAlert);
    }
    GetPixBounds((*mainScreenDevice)->gdPMap, &mainDeviceInitBounds);
    InitCluts();
#else
    int32_t flags;
    int32_t width;
    int32_t height;

    flags = (settings.windowed != 0 ? 0 : 1) | kModeFlagBase;
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    if (((flags & 1) == 0 && width <= 1024)
        || SetDisplayToSize(1024, 768, kModeDepth, flags) != 0) {
        if (SetDisplayToSize(800, 600, kModeDepth, flags) != 0)
            FatalStartupError(kModeFailAlert);
    }
    screenMinusDockRect.top = 0;
    screenMinusDockRect.left = 0;
    screenMinusDockRect.bottom = gVideoHeight;
    screenMinusDockRect.right = gVideoWidth;
#endif
}

#pragma opt_repositioncode reset

/* ResetGameData. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4690
// MAC: _ResetGameData i386 0x0001de0c ppc 0x00022640
void ResetGameData(void)
{
    short i;

    for (i = 0; i < kMaxPersons; i++) {
        if (personData[i].aiType >= 1) {
            personData[i].loaded = 1;
            personData[i].grudge = 0;
        }
    }
    for (i = 0; i < kMaxStellars; i++) {
        if ((stellarData[i].flags2 & kSpobAlwaysDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
        if ((stellarData[i].flags2 & kSpobStartsDestroyed) != 0) {
            stellarData[i].deadCounter = -1;
            if (stellarData[i].deadDays >= 0)
                stellarData[i].deadDaysLeft = stellarData[i].deadDays;
            else
                stellarData[i].deadDaysLeft = 1;
        } else {
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadDaysLeft = 0;
        }
    }
    for (i = 0; i < kMaxMissionBits; i++)
        missionBit[i] = 0;
    for (i = 0; i < kMaxDisasters; i++)
        disasterData[i].countdown = 0;
    for (i = 0; i < kMaxBeams; i++)
        beamData[i].state = -2;
    for (i = 0; i < kMaxShots; i++)
        shotData[i].life = -2.0f;
    for (i = 0; i < kMaxBoxes; i++)
        boxData[i].timer = -2.0f;
    for (i = 0; i < kMaxNebulae; i++) {
        nebulaData[i].explored = 0;
        nebulaData[i].loaded = 0;
    }
    RandomizeStars();
    dockPermission = dockTarget = -1;
    GetTime(&gameTime);
    gameTime.year += kGameYearOffset;
    pstrcpy(lastPlayerName, "\p");
    pstrcpy(playerName, "\pNo Name");
    pstrcpy(shipName, "\pNo Name");
    lastShipAIDispatched = 1;
    hypergateMapMode = 0;
    seenIntroScreen = 0;
}

#ifdef MACBUILD
/* MeasureProcessorSpeed (mac _MeasureProcessorSpeed, mac-only). STATE: mac MATCH i386 and ppc.
   OPEN: none */
// MAC: _MeasureProcessorSpeed i386 0x0001dfea ppc 0x00022904
void MeasureProcessorSpeed(void)
{
    int32_t counts[10];
    int32_t total;
    uint32_t start;
    int32_t average;
    int32_t i;

    total = 0;
    for (i = 0; i < 10; i++) {
        counts[i] = 0;
        start = TickCount();
        while (TickCount() <= start + 2)
            counts[i]++;
        total += counts[i];
    }
    average = total / 10.0;
    userSpeedSetting = 7200.0f / average;
    if (userSpeedSetting > 3.0)
        userSpeedSetting = 3.0;
    else if (userSpeedSetting < 1.05)
        userSpeedSetting = 1.0;
}

/* EmergencyBail (mac _EmergencyBail, mac-only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _EmergencyBail i386 0x0001e091 ppc 0x00022a18
void EmergencyBail(void)
{
    ST_Close();
}

/* SWAddFramesFromSinglePictXY (mac _SWAddFramesFromSinglePictXY, mac-only). STATE: mac MATCH
   i386 and ppc. OPEN: none */
// MAC: _SWAddFramesFromSinglePictXY i386 0x0001e09a ppc 0x00022a1c
short SWAddFramesFromSinglePictXY(SpriteWorldPtr destSpriteWorldP, SpritePtr srcSpriteP,
        short pictResID, short maskResID, short frameWidth, short frameHeight,
        short borderWidth, short borderHeight, short maxFrames, MaskType maskType)
{
    short err;
    PicHandle tempPic;
    CGrafPtr tempPictGWorldP;
    CGrafPtr tempMaskGWorldP;
    CGrafPtr tempTempMaskGWorldP;
    FramePtr newFrameP;
    Rect frameRect;
    short frame;
    Boolean needToInvertMask;

    err = noErr;

    if (destSpriteWorldP == NULL)
        SWAssertFail(kEngineSourceDir "Setup.c", 4047);

    tempPictGWorldP = NULL;
    tempMaskGWorldP = NULL;
    needToInvertMask = true;

    tempPic = GetPicture(pictResID);
    if (tempPic == NULL) {
        err = MemError();
        if (err == noErr)
            err = resNotFound;
    } else {
        ReleaseResource((Handle)tempPic);
    }

    if (err == noErr) {
        err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempPictGWorldP, pictResID);
        if (err == noErr && maskType != kNoMask)
            err = SWCreateGWorldFromPictResource(destSpriteWorldP, &tempMaskGWorldP, maskResID);

        if (err == noErr) {
            if (pictResID == maskResID && tempMaskGWorldP != NULL) {
                err = SWCreatePixelMaskFromGWorld(tempPictGWorldP, tempMaskGWorldP, NULL);
                needToInvertMask = false;
                if (destSpriteWorldP->pixelDepth <= 8 && (maskType & kRegionMask) != 0) {
                    SWInvertGWorld(tempMaskGWorldP);
                    needToInvertMask = true;
                }
            } else if ((maskType & kPixelMask) != 0) {
                SWFixImageGWorld(tempPictGWorldP, tempMaskGWorldP, NULL);
                SetPort((GrafPtr)tempMaskGWorldP);
                ForeColor(blackColor);
                BackColor(whiteColor);
                InvertRect(GetWindowPortRect(GetWindowFromPort(tempMaskGWorldP)));
                needToInvertMask = true;
            }
        }

        if (maxFrames == 0) {
            maxFrames = (GetWindowPortRect(GetWindowFromPort(tempPictGWorldP))->right / frameWidth) *
                        (GetWindowPortRect(GetWindowFromPort(tempPictGWorldP))->bottom / frameHeight);
        }

        if (err == noErr)
            err = SWCreateFrameFromGWorldAndRectStart(&tempTempMaskGWorldP, frameWidth,
                    frameHeight, maskType);

        if (err == noErr) {
            frameRect.left = 0;
            frameRect.top = 0;
            frameRect.right = frameWidth;
            frameRect.bottom = frameHeight;

            for (frame = 0; frame < maxFrames; frame++) {
                err = SWCreateFrameFromGWorldAndRectPartial(&newFrameP, tempPictGWorldP,
                        tempMaskGWorldP, tempTempMaskGWorldP, &frameRect, maskType);

                if (err == noErr) {
                    err = SWAddFrame(srcSpriteP, newFrameP);
                    if (frame < maxFrames - 1) {
                        frameRect.left += frameWidth + borderWidth;
                        frameRect.right = frameRect.left + frameWidth;
                        if (frameRect.right >
                                GetWindowPortRect(GetWindowFromPort(tempPictGWorldP))->right) {
                            frameRect.left = 0;
                            frameRect.right = frameWidth;
                            frameRect.top += frameHeight + borderHeight;
                            frameRect.bottom = frameRect.top + frameHeight;

                            if (frameRect.bottom >
                                    GetWindowPortRect(GetWindowFromPort(tempPictGWorldP))->bottom)
                                err = kOutOfRangeErr;
                        }
                    }
                }

                if (err != noErr) {
                    SWDisposeFrame(&newFrameP);
                    break;
                }
            }

            SWCreateFrameFromGWorldAndRectFinish(tempTempMaskGWorldP);
        }

        if (err == noErr) {
            if ((maskType & kPixelMask) != 0) {
                if (destSpriteWorldP->pixelDepth <= 8 && needToInvertMask)
                    SWInvertGWorld(tempMaskGWorldP);
            } else {
                if (tempMaskGWorldP != NULL) {
                    DisposeGWorld(tempMaskGWorldP);
                    tempMaskGWorldP = NULL;
                }
            }

            SWSetSpriteFrameRange(srcSpriteP, 0, maxFrames - 1);
            SWSetCurrentFrameIndex(srcSpriteP, 0);
        }
    }

    SWSetStickyIfError(err);
    return err;
}

#endif
#pragma opt_repositioncode off

/* ConvertSpriteToRLE (mac _ConvertSpriteToRLE). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4cf0
// MAC: _ConvertSpriteToRLE i386 0x0001e489 ppc 0x00022d98
#ifdef MACBUILD
short ConvertSpriteToRLE(SpriteRec *set)
{
    Rect r;
    int32_t result;
    short err;
    short i;

    if (set->frameArray[0]->framePort != NULL) {
        r = *GetCPortPortRect(set->frameArray[0]->framePort);
        if ((r.right - r.left) * (r.bottom - r.top) !=
            (set->frameArray[0]->frameRect.right - set->frameArray[0]->frameRect.left) * set->numFrames *
            (set->frameArray[0]->frameRect.bottom - set->frameArray[0]->frameRect.top)) {
            crashlog("error - bad sprite image size");
            return -1;
        }
    }
    err = SWCreateRLEData(set, 0);
    result = err;
    if (result != 0)
        return err;
    if (set->frameArray[0]->framePort != NULL)
        DisposeGWorld(set->frameArray[0]->framePort);
    if (set->frameArray[0]->maskPort != NULL)
        DisposeGWorld(set->frameArray[0]->maskPort);
    if (set->frameArray[0]->maskRgn != NULL)
        DisposeRgn(set->frameArray[0]->maskRgn);
    for (i = 0; i < set->maxFrames; i++) {
        set->frameArray[i]->framePort = NULL;
        set->frameArray[i]->frameDevice = NULL;
        set->frameArray[i]->framePixHndl = NULL;
        set->frameArray[i]->maskPort = NULL;
        set->frameArray[i]->maskDevice = NULL;
        set->frameArray[i]->maskPixHndl = NULL;
        set->frameArray[i]->maskRgn = NULL;
    }
    if (result == 0) {
        set->sharedPictGWorld = NULL;
        set->sharedMaskGWorld = NULL;
    }
    SWLockSprite(set);
    return err;
}
#else
short ConvertSpriteToRLE(SpriteRec *set)
{
    int i;
    short result;
    Rect *r;
    SWFrameRec *first;
    int err;

    first = set->frameArray[0];
    if (first->framePort != NULL) {
        r = GetWindowPortRect(first->framePort);
        first = set->frameArray[0];
        if ((r->right - r->left) * (r->bottom - r->top) !=
            (first->frameRect.right - first->frameRect.left) * set->numFrames *
            (first->frameRect.bottom - first->frameRect.top)) {
            crashlog("error - bad sprite image size");
            return -1;
        }
    }
    result = SWCreateRLEData(set, 0);
    err = result;
    if (err == 0) {
        if (set->frameArray[0]->framePort != NULL)
            DisposeGWorld(set->frameArray[0]->framePort);
        if (set->frameArray[0]->maskPort != NULL)
            DisposeGWorld(set->frameArray[0]->maskPort);
        i = 0;
        if (set->maxFrames > 0) {
            do {
                set->frameArray[(short)i]->framePort = NULL;
                set->frameArray[(short)i]->framePix = NULL;
                set->frameArray[(short)i]->maskPort = NULL;
                set->frameArray[(short)i]->maskPix = NULL;
                i = i + 1;
            } while ((short)i < set->maxFrames);
        }
    } else {
        return result;
    }
    if (err == 0)
        goto clearCaches;
finish:
    SWLockSprite(set);
    return result;
clearCaches:
    set->sharedPictGWorld = NULL;
    set->sharedMaskGWorld = NULL;
    goto finish;
}
#endif

#pragma opt_repositioncode reset

/* Every out-parameter is optional, so a caller that wants only the tile counts
   passes NULL for the rest. */

/* the setup object's globals, here and after ShanRec below */
#ifdef MACBUILD
extern SpinRec spriteInfo;
SpinHandle spriteInfoHand;
#else
SpinRec spriteInfo;
SpinHandle spriteInfoHand;
#endif

/* GetSpriteInfo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4e10
// MAC: _GetSpriteInfo i386 0x0001bf80 ppc 0x000200a0
Boolean GetSpriteInfo(short id, short *spriteID, short *maskID, short *xSize,
                  short *ySize, short *xTiles, short *yTiles)
{
    spriteInfoHand = (SpinHandle)GetResource(kSpinType, id);
    if (spriteInfoHand != NULL) {
        if (spriteID != NULL)
            *spriteID = (*spriteInfoHand)->spriteID;
        if (maskID != NULL)
            *maskID = (*spriteInfoHand)->maskID;
        if (xSize != NULL)
            *xSize = (*spriteInfoHand)->xSize;
        if (ySize != NULL)
            *ySize = (*spriteInfoHand)->ySize;
        if (xTiles != NULL)
            *xTiles = (*spriteInfoHand)->xTiles;
        if (yTiles != NULL)
            *yTiles = (*spriteInfoHand)->yTiles;
        ReleaseResource((Handle)spriteInfoHand);
        return 1;
    }
    return 0;
}


#ifdef MACBUILD
extern ShanRec shipSpriteInfo;
ShanRec **shipSpriteInfoHand;
#else
ShanRec shipSpriteInfo;
ShanRec **shipSpriteInfoHand;
#endif
short firstShipGlowIndex;
short firstShipLightIndex;
short firstShipAltIndex;
short firstShipWeapIndex;
short firstShipShieldIndex;
short shipSpriteLoaded[kMaxShipTypes][6];
Rect mainWindRect;
#ifdef MACBUILD
Rect mainDeviceInitBounds;
#else
Rect screenMinusDockRect;
#endif
int32_t fullScreenRestore;


/* LoadExtendedShipSprites (mac _LoadExtendedShipSprites). STATE: MATCH 100.0% (1048 exact,
   421 reloc-equiv), exe 1469 / obj 1469, byte-identical yes (5945 bytes, relocs masked);
   mac MATCH both slices. OPEN: none. */
// FUNCTION: EVNOVA 0x004b4ee0
// MAC: _LoadExtendedShipSprites i386 0x000226af ppc 0x00026b28
short LoadExtendedShipSprites(short shipType)
{
    short total;
    short spinID;
    short lightID;
    short maskID;
    short xSize;
    short ySize;
    short setCount;
    short i;
    short err;
    short count;
    short j;

    total = 0;
    shipSpriteInfoHand = (ShanRec **)GetResource(kShanType, shipType + kFirstResourceID);
    if (shipSpriteInfoHand != NULL) {
        LockAHandle((Handle)shipSpriteInfoHand);

        spinID = (*shipSpriteInfoHand)->baseImageID;
        maskID = (*shipSpriteInfoHand)->baseMaskID;
        xSize = (*shipSpriteInfoHand)->baseXSize;
        ySize = (*shipSpriteInfoHand)->baseYSize;
        shipClass[shipType].framesPer = (*shipSpriteInfoHand)->framesPer;
        if (shipClass[shipType].framesPer == 0)
            shipClass[shipType].framesPer = 36;
        setCount = (*shipSpriteInfoHand)->baseSetCount;
        if (setCount <= 0)
            setCount = 1;
        if (settings.shipAnimations != 0) {
            count = setCount;
            count *= shipClass[shipType].framesPer;
        } else {
            count = shipClass[shipType].framesPer;
        }
        shipClass[shipType].weapDecay = (*shipSpriteInfoHand)->weapDecay * 0.333;

        i = FindPriorLoadedShipSprite(spinID, shipType, 0);
        if (i != -1) {
            SWCloneSprite(masterShipSprite[i], &masterShipSprite[shipType], NULL);
            SWLockSprite(masterShipSprite[shipType]);
            shipSpriteLoaded[shipType][0] = spinID;
            shipClass[shipType].spriteIndex = i;
        } else {
            if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spinID)) {
                err = SWCreateSpriteFromRLEResource(&masterShipSprite[shipType], NULL,
                                                    spinID, count, 0);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading ship base sprite from RLE, " kShanTypeName " ID ");
                    crashlog_print_num(shipType + kFirstResourceID);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, shipType + kFirstResourceID, err);
                }
                SWLockSprite(masterShipSprite[shipType]);
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterShipSprite[shipType],
                                                     NULL, spinID, maskID, xSize, ySize, 0, 0,
                                                     shipClass[shipType].framesPer * setCount, 1);
                if ((int32_t)err != 0) {
                    crashlog_print("error loading ship base sprite from PICT, " kShanTypeName " ID ");
                    crashlog_print_num(shipType + kFirstResourceID);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, shipType + kFirstResourceID, err);
                }
                SWLockSprite(masterShipSprite[shipType]);
                err = ConvertSpriteToRLE(masterShipSprite[shipType]);
                if ((int32_t)err != 0) {
                    crashlog_print("error converting ship base sprite to RLE, " kShanTypeName " ID ");
                    crashlog_print_num(shipType + kFirstResourceID);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, shipType + kFirstResourceID, err);
                }
                masterShipSprite[shipType]->numFrames =
                    shipClass[shipType].framesPer * setCount;
            }
        }
        shipSpriteLoaded[shipType][0] = spinID;
        total += masterShipSprite[shipType]->numFrames;

        shipClass[shipType].shanFlags = (*shipSpriteInfoHand)->flags;
        shipClass[shipType].animDelay = (*shipSpriteInfoHand)->animDelay;
        shipClass[shipType].transparency = (int)(*shipSpriteInfoHand)->baseTransparency;
        if (settings.shipAnimations != 0)
            shipClass[shipType].baseSets = setCount;
        else
            shipClass[shipType].baseSets = 1;

        if ((*shipSpriteInfoHand)->altImageID > 0 && (*shipSpriteInfoHand)->altSetCount > 0) {
            if (settings.shipAnimations != 0)
                shipClass[shipType].altSets = (*shipSpriteInfoHand)->altSetCount;
            else
                shipClass[shipType].altSets = 1;
            spinID = (*shipSpriteInfoHand)->altImageID;
            maskID = (*shipSpriteInfoHand)->altMaskID;
            xSize = (*shipSpriteInfoHand)->altXSize;
            ySize = (*shipSpriteInfoHand)->altYSize;

            i = FindPriorLoadedShipSprite(spinID, shipType, 1);
            if (i != -1) {
                SWCloneSprite(masterShipAltSprite[i], &masterShipAltSprite[shipType], NULL);
                SWLockSprite(masterShipAltSprite[shipType]);
            } else {
                if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipAltSprite[shipType], NULL,
                                                        spinID,
                                                        shipClass[shipType].altSets *
                                                            shipClass[shipType].framesPer, 0);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship alt sprite from RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, shipType + kFirstResourceID, err);
                    }
                    SWLockSprite(masterShipAltSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipAltSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0,
                                                         shipClass[shipType].framesPer *
                                                             shipClass[shipType].altSets, 1);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship alt sprite from PICT, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, shipType + kFirstResourceID, err);
                    }
                    masterShipAltSprite[shipType]->numFrames =
                        shipClass[shipType].framesPer * shipClass[shipType].altSets;
                    SWLockSprite(masterShipAltSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipAltSprite[shipType]);
                    if ((int32_t)err != 0) {
                        crashlog_print("error converting ship alt sprite to RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, shipType + kFirstResourceID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][1] = spinID;
            total += masterShipAltSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->glowImageID > 0 && settings.engineGlows != 0) {
            spinID = (*shipSpriteInfoHand)->glowImageID;
            maskID = (*shipSpriteInfoHand)->glowMaskID;
            xSize = (*shipSpriteInfoHand)->glowXSize;
            ySize = (*shipSpriteInfoHand)->glowYSize;

            i = FindPriorLoadedShipSprite(spinID, shipType, 2);
            if (i != -1) {
                SWCloneSprite(masterShipGlowSprite[i], &masterShipGlowSprite[shipType], NULL);
                SWLockSprite(masterShipGlowSprite[shipType]);
            } else {
                if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipGlowSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship glow sprite from RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, shipType + kFirstResourceID, err);
                    }
                    SWLockSprite(masterShipGlowSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipGlowSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0,
                                                         shipClass[shipType].framesPer * setCount, 1);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship glow sprite from PICT, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, shipType + kFirstResourceID, err);
                    }
                    masterShipGlowSprite[shipType]->numFrames =
                        shipClass[shipType].framesPer * setCount;
                    SWLockSprite(masterShipGlowSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipGlowSprite[shipType]);
                    if ((int32_t)err != 0) {
                        crashlog_print("error converting ship glow sprite to RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, shipType + kFirstResourceID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][2] = spinID;
            total += masterShipGlowSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->lightImageID > 0 && settings.runningLights != 0) {
            lightID = (*shipSpriteInfoHand)->lightImageID;
            maskID = (*shipSpriteInfoHand)->lightMaskID;
            xSize = (*shipSpriteInfoHand)->lightXSize;
            ySize = (*shipSpriteInfoHand)->lightYSize;

            i = FindPriorLoadedShipSprite(lightID, shipType, 3);
            if (i != -1) {
                SWCloneSprite(masterShipLightSprite[i], &masterShipLightSprite[shipType], NULL);
                SWLockSprite(masterShipLightSprite[shipType]);
            } else {
                if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, lightID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipLightSprite[shipType], NULL,
                                                        lightID, count, 0);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship light sprite from RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, shipType + kFirstResourceID, err);
                    }
                    SWLockSprite(masterShipLightSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipLightSprite[shipType], NULL,
                                                         lightID, maskID, xSize, ySize, 0, 0,
                                                         shipClass[shipType].framesPer * setCount, 1);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship light sprite from PICT, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, shipType + kFirstResourceID, err);
                    }
                    masterShipLightSprite[shipType]->numFrames =
                        shipClass[shipType].framesPer * setCount;
                    SWLockSprite(masterShipLightSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipLightSprite[shipType]);
                    if ((int32_t)err != 0) {
                        crashlog_print("error converting ship light sprite to RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, shipType + kFirstResourceID, err);
                    }
                }
            }
            shipClass[shipType].blinkMode = (*shipSpriteInfoHand)->blinkMode;
            for (j = 0; j < 4; j++)
                shipClass[shipType].blinkValue[j] = (*shipSpriteInfoHand)->blinkValue[j];
            if (shipClass[shipType].blinkMode == 2 &&
                shipClass[shipType].blinkValue[2] > 31)
                shipClass[shipType].blinkValue[2] = 31;
            if (shipClass[shipType].blinkMode == 3 &&
                shipClass[shipType].blinkValue[1] > 31)
                shipClass[shipType].blinkValue[1] = 31;
            shipSpriteLoaded[shipType][3] = lightID;
            total += masterShipLightSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->weapImageID > 0 && settings.weaponGlows != 0) {
            spinID = (*shipSpriteInfoHand)->weapImageID;
            maskID = (*shipSpriteInfoHand)->weapMaskID;
            xSize = (*shipSpriteInfoHand)->weapXSize;
            ySize = (*shipSpriteInfoHand)->weapYSize;

            i = FindPriorLoadedShipSprite(spinID, shipType, 4);
            if (i != -1) {
                SWCloneSprite(masterShipWeaponSprite[i],
                              &masterShipWeaponSprite[shipType], NULL);
                SWLockSprite(masterShipWeaponSprite[shipType]);
            } else {
                if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipWeaponSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship weapon sprite from RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, shipType + kFirstResourceID, err);
                    }
                    SWLockSprite(masterShipWeaponSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipWeaponSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0,
                                                         shipClass[shipType].framesPer * setCount, 1);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship weapon sprite from PICT, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, shipType + kFirstResourceID, err);
                    }
                    masterShipWeaponSprite[shipType]->numFrames =
                        shipClass[shipType].framesPer * setCount;
                    SWLockSprite(masterShipWeaponSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipWeaponSprite[shipType]);
                    if ((int32_t)err != 0) {
                        crashlog_print("error converting ship weapon sprite to RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, shipType + kFirstResourceID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][4] = spinID;
            total += masterShipWeaponSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->shieldImageID > 0) {
            spinID = (*shipSpriteInfoHand)->shieldImageID;
            maskID = (*shipSpriteInfoHand)->shieldMaskID;
            xSize = (*shipSpriteInfoHand)->shieldXSize;
            ySize = (*shipSpriteInfoHand)->shieldYSize;

            i = FindPriorLoadedShipSprite(spinID, shipType, 5);
            if (i != -1) {
                SWCloneSprite(masterShipShieldSprite[i],
                              &masterShipShieldSprite[shipType], NULL);
                SWLockSprite(masterShipShieldSprite[shipType]);
            } else {
                if (ResourceExists((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipShieldSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship shield sprite from RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, shipType + kFirstResourceID, err);
                    }
                    SWLockSprite(masterShipShieldSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipShieldSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0, 0, 1);
                    if ((int32_t)err != 0) {
                        crashlog_print("error loading ship shield sprite from PICT, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, shipType + kFirstResourceID, err);
                    }
                    SWLockSprite(masterShipShieldSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipShieldSprite[shipType]);
                    if ((int32_t)err != 0) {
                        crashlog_print("error converting ship shield sprite to RLE, " kShanTypeName " ID ");
                        crashlog_print_num(shipType + kFirstResourceID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, shipType + kFirstResourceID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][5] = spinID;
            total += masterShipShieldSprite[shipType]->numFrames;
        }

        shipClass[shipType].weapPos[0][0].h = (*shipSpriteInfoHand)->mountPos[0].x[0];
        shipClass[shipType].weapPos[0][0].v = (*shipSpriteInfoHand)->mountPos[0].y[0];
        shipClass[shipType].weapPos[0][1].h = (*shipSpriteInfoHand)->mountPos[0].x[1];
        shipClass[shipType].weapPos[0][1].v = (*shipSpriteInfoHand)->mountPos[0].y[1];
        shipClass[shipType].weapPos[0][2].h = (*shipSpriteInfoHand)->mountPos[0].x[2];
        shipClass[shipType].weapPos[0][2].v = (*shipSpriteInfoHand)->mountPos[0].y[2];
        shipClass[shipType].weapPos[0][3].h = (*shipSpriteInfoHand)->mountPos[0].x[3];
        shipClass[shipType].weapPos[0][3].v = (*shipSpriteInfoHand)->mountPos[0].y[3];
        shipClass[shipType].weapPos[1][0].h = (*shipSpriteInfoHand)->mountPos[1].x[0];
        shipClass[shipType].weapPos[1][0].v = (*shipSpriteInfoHand)->mountPos[1].y[0];
        shipClass[shipType].weapPos[1][1].h = (*shipSpriteInfoHand)->mountPos[1].x[1];
        shipClass[shipType].weapPos[1][1].v = (*shipSpriteInfoHand)->mountPos[1].y[1];
        shipClass[shipType].weapPos[1][2].h = (*shipSpriteInfoHand)->mountPos[1].x[2];
        shipClass[shipType].weapPos[1][2].v = (*shipSpriteInfoHand)->mountPos[1].y[2];
        shipClass[shipType].weapPos[1][3].h = (*shipSpriteInfoHand)->mountPos[1].x[3];
        shipClass[shipType].weapPos[1][3].v = (*shipSpriteInfoHand)->mountPos[1].y[3];
        shipClass[shipType].weapPos[2][0].h = (*shipSpriteInfoHand)->mountPos[2].x[0];
        shipClass[shipType].weapPos[2][0].v = (*shipSpriteInfoHand)->mountPos[2].y[0];
        shipClass[shipType].weapPos[2][1].h = (*shipSpriteInfoHand)->mountPos[2].x[1];
        shipClass[shipType].weapPos[2][1].v = (*shipSpriteInfoHand)->mountPos[2].y[1];
        shipClass[shipType].weapPos[2][2].h = (*shipSpriteInfoHand)->mountPos[2].x[2];
        shipClass[shipType].weapPos[2][2].v = (*shipSpriteInfoHand)->mountPos[2].y[2];
        shipClass[shipType].weapPos[2][3].h = (*shipSpriteInfoHand)->mountPos[2].x[3];
        shipClass[shipType].weapPos[2][3].v = (*shipSpriteInfoHand)->mountPos[2].y[3];
        shipClass[shipType].weapPos[3][0].h = (*shipSpriteInfoHand)->mountPos[3].x[0];
        shipClass[shipType].weapPos[3][0].v = (*shipSpriteInfoHand)->mountPos[3].y[0];
        shipClass[shipType].weapPos[3][1].h = (*shipSpriteInfoHand)->mountPos[3].x[1];
        shipClass[shipType].weapPos[3][1].v = (*shipSpriteInfoHand)->mountPos[3].y[1];
        shipClass[shipType].weapPos[3][2].h = (*shipSpriteInfoHand)->mountPos[3].x[2];
        shipClass[shipType].weapPos[3][2].v = (*shipSpriteInfoHand)->mountPos[3].y[2];
        shipClass[shipType].weapPos[3][3].h = (*shipSpriteInfoHand)->mountPos[3].x[3];
        shipClass[shipType].weapPos[3][3].v = (*shipSpriteInfoHand)->mountPos[3].y[3];

        shipClass[shipType].weapPosZ[0][0] = (*shipSpriteInfoHand)->mountPosZ[0][0];
        shipClass[shipType].weapPosZ[0][1] = (*shipSpriteInfoHand)->mountPosZ[0][1];
        shipClass[shipType].weapPosZ[0][2] = (*shipSpriteInfoHand)->mountPosZ[0][2];
        shipClass[shipType].weapPosZ[0][3] = (*shipSpriteInfoHand)->mountPosZ[0][3];
        shipClass[shipType].weapPosZ[1][0] = (*shipSpriteInfoHand)->mountPosZ[1][0];
        shipClass[shipType].weapPosZ[1][1] = (*shipSpriteInfoHand)->mountPosZ[1][1];
        shipClass[shipType].weapPosZ[1][2] = (*shipSpriteInfoHand)->mountPosZ[1][2];
        shipClass[shipType].weapPosZ[1][3] = (*shipSpriteInfoHand)->mountPosZ[1][3];
        shipClass[shipType].weapPosZ[2][0] = (*shipSpriteInfoHand)->mountPosZ[2][0];
        shipClass[shipType].weapPosZ[2][1] = (*shipSpriteInfoHand)->mountPosZ[2][1];
        shipClass[shipType].weapPosZ[2][2] = (*shipSpriteInfoHand)->mountPosZ[2][2];
        shipClass[shipType].weapPosZ[2][3] = (*shipSpriteInfoHand)->mountPosZ[2][3];
        shipClass[shipType].weapPosZ[3][0] = (*shipSpriteInfoHand)->mountPosZ[3][0];
        shipClass[shipType].weapPosZ[3][1] = (*shipSpriteInfoHand)->mountPosZ[3][1];
        shipClass[shipType].weapPosZ[3][2] = (*shipSpriteInfoHand)->mountPosZ[3][2];
        shipClass[shipType].weapPosZ[3][3] = (*shipSpriteInfoHand)->mountPosZ[3][3];

        shipClass[shipType].compress[0].x = (*shipSpriteInfoHand)->upCompressX * 0.01;
        shipClass[shipType].compress[0].y = (*shipSpriteInfoHand)->upCompressY * 0.01;
        shipClass[shipType].compress[1].x = (*shipSpriteInfoHand)->downCompressX * 0.01;
        shipClass[shipType].compress[1].y = (*shipSpriteInfoHand)->downCompressY * 0.01;

        for (i = 0; i <= 1; i++) {
            if (shipClass[shipType].compress[i].x <= 0.0f)
                shipClass[shipType].compress[i].x = 1.0f;
            if (shipClass[shipType].compress[i].y <= 0.0f)
                shipClass[shipType].compress[i].y = 1.0f;
        }

        AdjustHandleSize((Handle)shipSpriteInfoHand, 0xc0);
        UnlockAHandle((Handle)shipSpriteInfoHand);
        ReleaseResource((Handle)shipSpriteInfoHand);
    }
    return total;
}

EXTERN_C_BEGIN

/* ReportSpriteLoadError. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6620
// MAC: _ReportSpriteLoadError i386 0x0001ea6c ppc 0x0002337c
void ReportSpriteLoadError(short label, short value, short special)
{
    unsigned char labelText[256];
    unsigned char valueText[256];
    unsigned char specialText[256];
#ifndef MACBUILD
    extern void NumToString(int value, unsigned char *str);  /* 0x004bbce0 */
#endif

    if ((int32_t)special == 0)
        return;
    GetIndString(labelText, kAlertStrList, label);
    NumToString(value, valueText);
    if (special == -0xc0)
        GetIndString(specialText, kSpecialValueStrList, 1);
    else if (special == -0x27)
        GetIndString(specialText, kSpecialValueStrList, 2);
    else if ((int32_t)special == 0x6b)
        GetIndString(specialText, kSpecialValueStrList, 3);
    else if ((int32_t)special == -0x6c)
        GetIndString(specialText, kSpecialValueStrList, 4);
    else
        NumToString(special, specialText);
    ParamText(labelText, valueText, specialText, "\p");
    StopAlert(0x83, 0);
}

EXTERN_C_END

/* FatalStartupError. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6760
// MAC: _FatalStartupError i386 0x0001e62f ppc 0x00022f78
void FatalStartupError(short index)
{
    unsigned char text[256];

#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    DisposeGammaTools();
#endif
    GetIndString(text, kAlertStrList, index);
    ParamText(text, "\p", "\p", "\p");
    StopAlert(128, NULL);
    ExitToShell();
}

/* FindPriorLoadedShipSprite. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b67c0
// MAC: _FindPriorLoadedShipSprite i386 0x0001e5e4 ppc 0x00022f34
short FindPriorLoadedShipSprite(short value, short count, short field)
{
    short i;

    for (i = 0; i < count; i++) {
        if (value == shipSpriteLoaded[i][field])
            return i;
    }
    return -1;
}

/* DisposeClonedSprite (mac _DisposeClonedSprite). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6810
// MAC: _DisposeClonedSprite i386 0x0001b3d0 ppc 0x0001f450
void DisposeClonedSprite(SpritePtr *p)
#ifdef MAC1010
;
#else
#include "../engine/disposeclonedsprite.inc"
#endif

#ifndef MACBUILD
/* ScreenEndScene (mac @ScreenEndScene@0). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6850
void ScreenEndScene(void)
{
    if (gDisableUpdatesMode == 0) {
        UpdateAndPageFlip();
        LockVideoPage();
        if (gGamePort.pix.baseAddr != videoBase) {
            gGamePort.pix.baseAddr = videoBase;
            if (gSpriteWorld != NULL)
                SWWindowMoved(gSpriteWorld);
        }
    }
}

/* GetGlobalMouse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b68a0
void GetGlobalMouse(Point *pt)
{
    int32_t h;
    int32_t v;

    MouseReadPosition(&h, &v);
    pt->h = h;
    pt->v = v;
}

/* FlushEvents. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b68d0
void FlushEvents(unsigned short whichMask, unsigned short stopMask)
{
    KeyboardFlush();
}

/* Button. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b68e0
Boolean Button(void)
{
    KeyboardKbhit();
    return MouseReadButtons();
}

/* StillDown. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b68f0
Boolean StillDown(void)
{
    KeyboardKbhit();
    return MouseReadButtons();
}

/* TextFont. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6900
void TextFont(short font)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->txFont = font;
}

/* TextSize. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6920
void TextSize(short size)
{
#ifdef CEFIX /* F12.82 */
    /* CE bug: CE's replacement drops the NULL-port test (no caller in play has no port). */
    CurrentGrafPort->txSize = CE_Scale(size);
#else
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->txSize = size;
#endif
}

/* TextFace. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6940
void TextFace(short face)
{
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->txFace = face;
}

/* Color2Index. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6960
uint32_t Color2Index(RGBColor *color)
{
    return ((color->red & 0xf800) >> 1) | ((color->green & 0xf800) >> 6) |
           ((color->blue & 0xf800) >> 11);
}

/* RGBForeColor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6990
void RGBForeColor(const RGBColor *color)
{
    RGBColor *dst;

    if (CurrentGrafPort == NULL || color == NULL)
        return;
    dst = &CurrentGrafPort->foreColor;
    dst->red = color->red;
    dst->green = color->green;
    dst->blue = color->blue;
}

/* BackColor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b69d0
void BackColor(const RGBColor *color)
{
    RGBColor *dst;

    if (CurrentGrafPort == NULL || color == NULL)
        return;
    dst = &CurrentGrafPort->backColor;
    dst->red = color->red;
    dst->green = color->green;
    dst->blue = color->blue;
}
#endif

#ifdef MACBUILD
#ifdef MAC1010
void PreflightMemoryUsage(int32_t *base, int32_t *engineGlow, int32_t *runningLights, int32_t *weaponGlow);
int32_t PreflightMemoryUsageTotal(void)
#include "../engine/preflighttotal.inc"
#endif

/* PreflightMemoryUsage (mac _PreflightMemoryUsage, mac-only). STATE: mac MATCH i386 and ppc.
   OPEN: none */
// MAC: _PreflightMemoryUsage i386 0x00022214 ppc 0x00026624
void PreflightMemoryUsage(int32_t *base, int32_t *engineGlow, int32_t *runningLights, int32_t *weaponGlow)
{
    int32_t sizes[6];
    short i;
    short j;
    short k;
    short id;
    Handle h;
    short baseSets;
    short altSets;
    int32_t total;

    for (i = 0; i < kMaxShipTypes; i++)
        for (k = 0; k < 6; k++)
            shipSpriteLoaded[i][k] = -1;
    *base = *engineGlow = *runningLights = *weaponGlow = 0;
    for (i = 0; i < kMaxShipTypes; i++) {
        shipSpriteInfoHand = NULL;
        shipSpriteInfoHand = (ShanRec **)GetResource(kShanType, i + kFirstResourceID);
        if (shipSpriteInfoHand == NULL)
            continue;
        baseSets = (*shipSpriteInfoHand)->baseSetCount;
        altSets = (*shipSpriteInfoHand)->altSetCount;
        if (baseSets < 1)
            baseSets = 1;
        if (altSets < 1)
            altSets = 1;
        for (j = 0; j < 6; j++) {
            sizes[j] = 0;
            if (j == 0)
                id = (*shipSpriteInfoHand)->baseImageID;
            else if (j == 1)
                id = (*shipSpriteInfoHand)->altImageID;
            else if (j == 2)
                id = (*shipSpriteInfoHand)->glowImageID;
            else if (j == 3)
                id = (*shipSpriteInfoHand)->lightImageID;
            else if (j == 4)
                id = (*shipSpriteInfoHand)->weapImageID;
            else if (j == 5)
                id = (*shipSpriteInfoHand)->shieldImageID;
            if (id <= 0)
                continue;
            for (k = 0; k < i; k++) {
                if (shipSpriteLoaded[k][j] == id) {
                    id = -1;
                    break;
                }
            }
            if (id == -1)
                continue;
#ifdef MAC1010
            SetResLoad(false);
#endif
            h = GetResource((gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType, id);
            if (h != NULL) {
                sizes[j] = GetResourceSizeOnDisk(h);
                ReleaseResource(h);
            }
#ifdef MAC1010
            SetResLoad(true);
#endif
            shipSpriteLoaded[i][j] = id;
        }
        *base = *base + sizes[0];
        *base = *base + sizes[1];
        *base = *base + sizes[5];
        *engineGlow = *engineGlow + sizes[2];
        *runningLights = *runningLights + sizes[3];
        *weaponGlow = *weaponGlow + sizes[4];
        total = 0;
        for (j = 0; j < 6; j++)
            total += sizes[j];
        ReleaseResource((Handle)shipSpriteInfoHand);
    }
}

#ifdef MAC1010
void DisposeClonedSprite(SpritePtr *p)
#include "../engine/disposeclonedsprite.inc"
#endif

/* PreflightMemoryUsageTotal (mac _PreflightMemoryUsageTotal, mac-only). STATE: mac MATCH i386
   and ppc. OPEN: none */
// MAC: _PreflightMemoryUsageTotal i386 0x00022438 ppc 0x000268b0
int32_t PreflightMemoryUsageTotal(void)
#ifdef MAC1010
;
#else
#include "../engine/preflighttotal.inc"
#endif

/* DoMemoryUsageDialog (mac _DoMemoryUsageDialog, mac-only). STATE: mac MATCH i386 and ppc.
   OPEN: none */
// MAC: _DoMemoryUsageDialog i386 0x00022492 ppc 0x0002693c
void DoMemoryUsageDialog(void)
{
    DialogPtr dlg;
    short itemHit;
    short itemType;
    Handle itemHand;
    Rect itemRect;
    Boolean done;
    ControlHandle ctl;

    dlg = GetNewDialog(5000, NULL, -1);
    if (dlg == NULL)
        return;
    ShowWindow(GetDialogWindow(dlg));
    SetPort((GrafPtr)GetDialogPort(dlg));
    SelectWindow(GetDialogWindow(dlg));
    FrameDefaultButton(dlg, 1);
    done = false;
    do {
        GetDialogItem(dlg, 3, &itemType, &itemHand, &itemRect);
        ctl = (ControlHandle)itemHand;
        if (settings.shipAnimations)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(dlg, 4, &itemType, &itemHand, &itemRect);
        ctl = (ControlHandle)itemHand;
        if (settings.engineGlows)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(dlg, 5, &itemType, &itemHand, &itemRect);
        ctl = (ControlHandle)itemHand;
        if (settings.runningLights)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(dlg, 6, &itemType, &itemHand, &itemRect);
        ctl = (ControlHandle)itemHand;
        if (settings.weaponGlows)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        ModalDialog(NULL, &itemHit);
        if (itemHit == 1)
            done = true;
        if (itemHit == 3)
            settings.shipAnimations = !settings.shipAnimations;
        if (itemHit == 4)
            settings.engineGlows = !settings.engineGlows;
        if (itemHit == 5)
            settings.runningLights = !settings.runningLights;
        if (itemHit == 6)
            settings.weaponGlows = !settings.weaponGlows;
    } while (!done);
    DisposeDialog(dlg);
}
#endif
