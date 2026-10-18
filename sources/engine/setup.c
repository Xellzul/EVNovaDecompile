// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
// COMPILE-WHY: two scoped `#pragma opt_repositioncode off' regions (pragma-only, no -opt
// spelling), over SetupMonitor and over ConvertSpriteToRLE; each matches only with it.
/* setup.c --tu-map object O26, the original Setup.c: SpriteWorld and sprite setup, loading, cleanup and game-state reset, then the QuickDraw state shims. Notes: Research/notes/setup.md */
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


/* SetupSpriteWorld (mac _SetupSpriteWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab980
void SetupSpriteWorld(void)
{
    short i;
    short err;
    Rect *frameRect;
    Str255 str;
    Rect windRect;
    RGBColor color;

    mBarHidden = 1;
    windRect.top = mainWindRect.top;
    windRect.left = mainWindRect.left;
    windRect.bottom = mainWindRect.bottom;
    windRect.right = mainWindRect.right;
    windRect.right -= 194;
    SetPortWindowPort(gWind);
    err = SWCreateSpriteWorldFromWindow(&gSpriteWorld, gWind, &windRect, &windRect, 0);
    if ((long)err != 0) {
        FadeScreenIn();
        GetIndString(str, 140, 36);
        EVNoteAlert(str);
        ST_Close();
        ExitToShell();
    }
    if (gSpriteWorld->pixelDepth != 8 && gSpriteWorld->pixelDepth != 16) {
        FadeScreenIn();
        GetIndString(str, 140, 37);
        EVNoteAlert(str);
        ST_Close();
        ExitToShell();
    }

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

    crashlog("creating scratch frames");
    windRect.top = 0;
    windRect.left = 0;
    windRect.right = ((mainWindRect.right - mainWindRect.left) * 0.25);
    if (windRect.right < 200)
        windRect.right = 200;
    windRect.bottom = windRect.right;
    SWCreateFrame(GetGDevice(), &floatingMapFrame, &windRect, gSpriteWorld->pixelDepth, 1);
    SWLockFrame(floatingMapFrame);
    floatingMapRect.top = windRect.top;
    floatingMapRect.left = windRect.left;
    floatingMapRect.bottom = windRect.bottom;
    floatingMapRect.right = windRect.right;
    OffsetRect(&floatingMapRect,
            GetCWindowPortRect(gWind)->right - GetCWindowPortRect(gWind)->left
            - (windRect.right - windRect.left + 210), 150);
    SetRect(&windRect, 0, 0, 160, 120);
    SWCreateFrame(GetGDevice(), &floatingEscortMenuFrame, &windRect, gSpriteWorld->pixelDepth, 1);
    SWLockFrame(floatingEscortMenuFrame);
    frameRect = &floatingEscortMenuFrame->frameRect;
    escortMenuRect.top = frameRect->top;
    escortMenuRect.left = frameRect->left;
    escortMenuRect.bottom = frameRect->bottom;
    escortMenuRect.right = frameRect->right;
    OffsetRect(&escortMenuRect, -escortMenuRect.left, -escortMenuRect.top);
    OffsetRect(&escortMenuRect, 15, 150);

    color.red = color.green = color.blue = 0;
    SWSetBackgroundColor(gSpriteWorld, &color);
    SWSetPostEraseCallBack(gSpriteWorld, PostEraseCallback);
    SWSetPostDrawCallBack(gSpriteWorld, PostDrawCallback);
    SWSetSpriteWorldMaxFPS(gSpriteWorld, 0);
    SWLockSpriteWorld(gSpriteWorld);
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));

    spriteClipRect.top = windRect.top;
    spriteClipRect.left = windRect.left;
    spriteClipRect.bottom = windRect.bottom;
    spriteClipRect.right = windRect.right;
    spriteClipRect.top -= 75;
    spriteClipRect.left -= 75;

    for (i = 0; i < 768; i++) {
        masterShipSprite[i] = 0;
        masterShipGlowSprite[i] = 0;
        masterShipLightSprite[i] = 0;
        masterShipWeaponSprite[i] = 0;
        masterShipAltSprite[i] = 0;
        masterShipShieldSprite[i] = 0;
    }
    for (i = 0; i < 256; i++)
        masterStellarSprite[i] = 0;
    for (i = 0; i < 64; i++)
        masterExplodSprite[i] = 0;
    for (i = 0; i < 256; i++)
        masterShotSprite[i] = 0;

    if ((long)InitParticles(1024, 0) != 0)
        SWAssertFail("Setup.c", 0x133);
    SWSetPortToBackground(gSpriteWorld);
    if (gSpriteWorld->pixelDepth == 8) {
        err = Construct8BitAlphaTable();
        if ((long)err != 0) {
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
void SetupWind(void)
{
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
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    if (windowedPref == 0) {
        HideTheMenuBar();
        mBarHidden = 1;
    }
    screenCenter.h = ((GetCWindowPortRect(gWind)->right
                             - GetCWindowPortRect(gWind)->left - 194) * 0.5);
    screenCenter.v = ((GetCWindowPortRect(gWind)->bottom
                             - GetCWindowPortRect(gWind)->top) * 0.5);
}

/* InitArrays (mac _InitArrays). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ac560
void InitArrays(void)
{
    if (!(shipData = (ShipObj *)AllocAPointer(0x325200)))
        SWAssertFail("Setup.c", 0x1b6);
    if (!(stellarData = (SpobRecord *)AllocAPointer(0x24c000)))
        SWAssertFail("Setup.c", 0x1b7);
    if (!(gSystems = (SystRecord *)AllocAPointer(0xfe000)))
        SWAssertFail("Setup.c", 0x1b8);
    if (!(gShipTypes = (ShipRec *)AllocAPointer(0x203400)))
        SWAssertFail("Setup.c", 0x1b9);
    if (!(gWeapons = (WeapRecord *)AllocAPointer(0xc800)))
        SWAssertFail("Setup.c", 0x1ba);
    if (!(explodData = (BoomSlot *)AllocAPointer(0x300)))
        SWAssertFail("Setup.c", 0x1bb);
    if (!(explodType = (BoomRecord *)AllocAPointer(0x200)))
        SWAssertFail("Setup.c", 0x1bc);
    if (!(gDebrisSlots = (DebrisSlot *)AllocAPointer(0x600)))
        SWAssertFail("Setup.c", 0x1bd);
    if (!(beamData = (BeamRecord *)AllocAPointer(0x880)))
        SWAssertFail("Setup.c", 0x1be);
    if (!(gOutfits = (OutfRecord *)AllocAPointer(0x6f800)))
        SWAssertFail("Setup.c", 0x1bf);
    if (!(dudeData = (DudeRecord *)AllocAPointer(0x9400)))
        SWAssertFail("Setup.c", 0x1c0);
    if (!(govtData = (GovtRecord *)AllocAPointer(0x8800)))
        SWAssertFail("Setup.c", 0x1c1);
    if (!(personData = (PersRecord *)AllocAPointer(0x1e5000)))
        SWAssertFail("Setup.c", 0x1c2);
    if (!(asteroidData = (AsteroidObj *)AllocAPointer(0x240)))
        SWAssertFail("Setup.c", 0x1c3);
    if (!(disasterData = (OopsRecord *)AllocAPointer(0x21000)))
        SWAssertFail("Setup.c", 0x1c4);
    if (!(nebulaData = (NebuRecord *)AllocAPointer(0x4100)))
        SWAssertFail("Setup.c", 0x1c5);
    if (!(fleetData = (FletRecord *)AllocAPointer(0x12400)))
        SWAssertFail("Setup.c", 0x1c6);
    if (!(boxData = (BoxObj *)AllocAPointer(0xa00)))
        SWAssertFail("Setup.c", 0x1c7);
    if (!(podData = (PodObj *)AllocAPointer(0x300)))
        SWAssertFail("Setup.c", 0x1c8);
    if (!(shotData = (ShotObj *)AllocAPointer(0x2400)))
        SWAssertFail("Setup.c", 0x1c9);
    if (!(junkData = (JunkRecord *)AllocAPointer(0x29300)))
        SWAssertFail("Setup.c", 0x1ca);
    if (!(gCrons = (CronRecord *)AllocAPointer(0x6a000)))
        SWAssertFail("Setup.c", 0x1cb);
    if (!(rankData = (RankRecord *)AllocAPointer(0x9000)))
        SWAssertFail("Setup.c", 0x1cc);
    if (!(starData = (StarSlot *)AllocAPointer(0x190)))
        SWAssertFail("Setup.c", 0x1cd);
    if (!(missionBit = (char *)AllocAPointer(0x2710)))
        SWAssertFail("Setup.c", 0x1ce);
    if (!(currentMissions = (MissionRecord *)AllocAPointer(0x8e60)))
        SWAssertFail("Setup.c", 0x1cf);
    if (!(currentMissionObjectives = (MissionState *)AllocAPointer(0x140)))
        SWAssertFail("Setup.c", 0x1d0);
}

/* InitOffscreen (mac _InitOffscreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ac950
void InitOffscreen(void)
{
    Rect *bounds;
    unsigned char msg[256];
    Rect r;

    bounds = GetCWindowPortRect(gWind);
    r.top = bounds->top;
    r.left = bounds->left;
    r.bottom = bounds->bottom;
    r.right = bounds->right;
    OffsetRect(&r, -r.left, -r.top);
    CreateGWorldContainer(&loadPort, GetMainDevice(), 0, &r);
    if (loadPort.world == NULL) {
        HideWindow(gWind);
        ShowTheMenuBar();
        ShowTheCursor();
        FadeScreenIn();
        GetIndString(msg, 0x8c, 0x27);
        EVNoteAlert(msg);
        ST_Close();
        ExitToShell();
    }
    r.right = r.left + 0xc2;
    CreateGWorldContainer(&statusOffPort, GetMainDevice(), 0, &r);
    if (statusOffPort.world == NULL) {
        HideWindow(gWind);
        ShowTheMenuBar();
        ShowTheCursor();
        FadeScreenIn();
        GetIndString(msg, 0x8c, 0x27);
        EVNoteAlert(msg);
        ST_Close();
        ExitToShell();
    }
    SWSetPortToWindow(gSpriteWorld);
}


/* CleanUp. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004acac0
void CleanUp(void)
{
    short i;
    short j;
    void *snd;

    crashlog("doing cleanup");
    DisposeIntroProgressBar();
    FlushEvents(everyEvent - diskMask, 0);

    crashlog("disposing sounds");
    ST_HaltSound(0);
    if (warpUpSnd[0] != NULL)
        ST_DisposeSound(warpUpSnd[0]);
    snd = warpUpSnd[1];
    if (snd != NULL)
        ST_DisposeSound(snd);
    if (warpOutSnd != NULL)
        ST_DisposeSound(warpOutSnd);
    for (i = 0; i <= 4; i++) {
        if (beepSnd[i] != NULL)
            ST_DisposeSound(beepSnd[i]);
    }
    for (i = 0; i < 0x100; i++) {
        if (shotSnd[i] != NULL)
            ST_DisposeSound(shotSnd[i]);
    }
    for (i = 0; i < 0x40; i++) {
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

    crashlog("disposing buttons");
    SetPortWindowPort(gWind);
    DisposeButtons();
    ShowTheMenuBar();

    crashlog("disposing sprites");
    for (i = 0; i < 0x40; i++) {
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
    for (i = 0; i < 0x20; i++) {
        if (podData[i].sprite)
            DisposeClonedSprite(&podData[i].sprite);
    }
    for (i = 0; i < 0x300; i++) {
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
        if (gShipTypes[i].frameAngles != NULL)
            DeallocAPointer(gShipTypes[i].frameAngles);
    }
    for (i = 0; i < 0x10; i++) {
        if (stellarSprite[i] != NULL)
            DeallocAPointer(stellarSprite[i]);
    }
    for (i = 0; i < 0x100; i++) {
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
    for (i = 0; i < 0x20; i++) {
        if (explodData[i].sprite)
            DisposeClonedSprite(&explodData[i].sprite);
    }
    for (i = 0; i < 0x40; i++) {
        if (masterExplodSprite[i])
            SWDisposeSprite(&masterExplodSprite[i]);
    }
    for (i = 0; i < 0x80; i++) {
        if (shotData[i].sprite)
            DisposeClonedSprite(&shotData[i].sprite);
    }
    for (i = 0; i < 0x100; i++) {
        if (masterShotSprite[i])
            SWDisposeSprite(&masterShotSprite[i]);
    }
    for (i = 0; i < 0x40; i++) {
        if (gDebrisSlots[i].sprite)
            DisposeClonedSprite(&gDebrisSlots[i].sprite);
    }
    for (i = 0; i < 8; i++) {
        if (masterSmokeSprite[i])
            SWDisposeSprite(&masterSmokeSprite[i]);
    }
    for (i = 0; i < 0x10; i++) {
        if (asteroidData[i].sprite)
            DisposeClonedSprite(&asteroidData[i].sprite);
    }
    for (i = 0; i < 0x10; i++) {
        if (masterAsteroidSprite[i])
            SWDisposeSprite(&masterAsteroidSprite[i]);
    }
    for (i = 0; i < 6; i++) {
        if (gMainButtonSprites[i])
            SWDisposeSprite(&gMainButtonSprites[i]);
    }
    if (menuRolloverSprite != NULL)
        SWDisposeSprite(&menuRolloverSprite);
    for (i = 0; i < 0x40; i++) {
        if (boxData[i].sprite)
            DisposeClonedSprite(&boxData[i].sprite);
    }
    for (i = 0; i < 5; i++) {
        if (masterBoxSprite[i])
            SWDisposeSprite(&masterBoxSprite[i]);
    }
    for (i = 0; i < 0x14; i++) {
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
    for (i = 0; i < 4; i++) {
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

    crashlog("disposing particles");
    DisposeParticles();
    SWDisposeFrame(&floatingMapFrame);
    SWDisposeFrame(&floatingEscortMenuFrame);

    crashlog("disposing patterns & picts");
    if (gSpriteWorld->pixelDepth == 8)
        Dispose8BitAlphaTable();
    for (i = 0; i < 0xa; i++) {
        if (staticPat[i] != NULL)
            DisposePixPat((Image_t *)staticPat[i]);
    }
    for (i = 0; i < 0x300; i++) {
        if (targetInfoPic[i] != NULL) {
            QDReleaseResource((PicHandle)targetInfoPic[i]);
            targetInfoPic[i] = NULL;
        }
    }

    crashlog("disposing sprite layers");
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
        SWDisposeWindowFrame(&gSpriteWorld->windowFrameP);
    }

    crashlog("disposing sprite world");
    if (gSpriteWorld != NULL)
        DeallocAPointer(gSpriteWorld);
    SWExitSpriteWorld();

    crashlog("disposing offscreen buffers");
    DisposeGWorldContainer(&loadPort);
    DisposeGWorldContainer(&statusOffPort);

    crashlog("disposing array pointers");
    DeallocAPointer(shipData);
    DeallocAPointer(shotData);
    DeallocAPointer(govtData);
    DeallocAPointer(dudeData);
    DeallocAPointer(beamData);
    DeallocAPointer(stellarData);
    DeallocAPointer(gSystems);
    DeallocAPointer(gShipTypes);
    DeallocAPointer(gWeapons);
    DeallocAPointer(explodData);
    DeallocAPointer(explodType);
    DeallocAPointer(gDebrisSlots);
    DeallocAPointer(gOutfits);
    DeallocAPointer(personData);
    DeallocAPointer(asteroidData);
    DeallocAPointer(disasterData);
    DeallocAPointer(nebulaData);
    DeallocAPointer(fleetData);
    DeallocAPointer(boxData);
    DeallocAPointer(podData);
    DeallocAPointer(junkData);
    DeallocAPointer(gCrons);
    DeallocAPointer(rankData);
    DeallocAPointer(starData);
    DeallocAPointer(missionBit);
    DeallocAPointer(currentMissions);
    DeallocAPointer(currentMissionObjectives);

    crashlog("disposing dialog buffer");
    DisposeMapWorld();

    crashlog("disposing window");
    HideWindow(gWind);
    DisposeWindow(gWind);
    gWind = NULL;

    crashlog("closing QuickTime");
    ExitMovies();

    crashlog("carbon cleanup");
    ShowTheCursor();

    crashlog("closing MT");

    crashlog("closing other tools");
    ST_Close();
    RT3_Close();
    IT_Close();
    DT_Close();
    FT_Close();
    PlatformClose();

    crashlog("showing control strip");
    RestoreControlStrip();

    crashlog("clean exit");

    crashlog("\n\n- end of log -");
    crashlog_close();
    ExitToShell();
}

/* InitColors. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ad7c0
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
void LoadSprites(void)
{
    short spriteID, maskID;
    short xSize, ySize;
    SpritePtr *p;
    short xTiles, yTiles;
    short i;
    long resID;
    SpritePtr *q;
    short err;
    unsigned long rleType;

    for (i = 0, p = masterShotSprite; i < 256; i++, p++) {
        if (GetSpriteInfo(i + 3000, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
            if (ResourceExists(rleType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(p,NULL, spriteID, xTiles * yTiles, 0);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from RLE resource, sp\x95n ID ");
                    crashlog_print_num(i + 3000);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 3000, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, p,NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from PICT resource, sp\x95n ID ");
                    crashlog_print_num(i + 3000);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 3000, err);
                }
                SWLockSprite(masterShotSprite[i]);
                err = ConvertSpriteToRLE(masterShotSprite[i]);
                if ((long)err != 0) {
                    crashlog_print("error converting sprite to RLE, sp\x95n ID ");
                    crashlog_print_num(i + 3000);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 3000, err);
                }
                masterShotSprite[i]->numFrames = xTiles * yTiles;
            }
            IterateInGameEventLoop(1);
        }
    }

    for (i = 0; i < 64; i++) {
        if (GetSpriteInfo(i + 400, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
            if (ResourceExists(rleType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterExplodSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                    crashlog_print_num(i + 400);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 400, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterExplodSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                    crashlog_print_num(i + 400);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 400, err);
                }
                SWLockSprite(masterExplodSprite[i]);
                err = ConvertSpriteToRLE(masterExplodSprite[i]);
                if ((long)err != 0) {
                    crashlog_print("error converting sprite to RLE, sp\x95n ID ");
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
            rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
            if (ResourceExists(rleType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterBoxSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                    crashlog_print_num(i + 500);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 500, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterBoxSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                    crashlog_print_num(i + 500);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 500, err);
                }
                SWLockSprite(masterBoxSprite[i]);
                err = ConvertSpriteToRLE(masterBoxSprite[i]);
                if ((long)err != 0) {
                    crashlog_print("error converting sprite to RLE, sp\x95n ID ");
                    crashlog_print_num(i + 500);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 500, err);
                }
                masterBoxSprite[i]->numFrames = xTiles * yTiles;
            }
            IterateInGameEventLoop(1);
        } else {
            crashlog_print("resource not found, sp\x95n ID ");
            crashlog_print_num(i + 500);
            crashlog_print("\n");
            ReportSpriteLoadError(1, i + 500, resNotFound);
        }
    }

    for (i = 0; i < 16; i++) {
        masterAsteroidSprite[i] = 0;
        if (GetSpriteInfo(i + 800, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
            if (ResourceExists(rleType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&masterAsteroidSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                    crashlog_print_num(i + 800);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 800, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterAsteroidSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                    crashlog_print_num(i + 800);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 800, err);
                }
                SWLockSprite(masterAsteroidSprite[i]);
                err = ConvertSpriteToRLE(masterAsteroidSprite[i]);
                if ((long)err != 0) {
                    crashlog_print("error converting sprite to RLE, sp\x95n ID ");
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
                crashlog_print("resource not found, sp\x95n ID ");
                crashlog_print_num(i + 800);
                crashlog_print("\n");
                ReportSpriteLoadError(1, i + 800, resNotFound);
            }
        }
    }

    for (i = 0; i < 6; i++) {
        gMainButtonSprites[i] = 0;
        if (GetSpriteInfo(i + 600, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
            if (ResourceExists(rleType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&gMainButtonSprites[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                    crashlog_print_num(i + 600);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, i + 600, err);
                }
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &gMainButtonSprites[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((long)err != 0) {
                    crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                    crashlog_print_num(i + 600);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, i + 600, err);
                }
                err = ConvertSpriteToRLE(gMainButtonSprites[i]);
                if ((long)err != 0) {
                    crashlog_print("error converting sprite to RLE, sp\x95n ID ");
                    crashlog_print_num(i + 600);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, i + 600, err);
                }
                gMainButtonSprites[i]->numFrames = xTiles * yTiles;
            }
            SWLockSprite(gMainButtonSprites[i]);
            SWSetCurrentFrameIndex(gMainButtonSprites[i], 0);
            IterateInGameEventLoop(1);
        } else
            gMainButtonSprites[i] = 0;
    }

    if (GetSpriteInfo(700, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
        if (ResourceExists(rleType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&masterStarSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((long)err != 0) {
                crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                crashlog_print_num(700);
                crashlog_print("\n");
                ReportSpriteLoadError(2, 700, err);
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterStarSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((long)err != 0) {
                crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                crashlog_print_num(700);
                crashlog_print("\n");
                ReportSpriteLoadError(3, 700, err);
            }
            err = ConvertSpriteToRLE(masterStarSprite);
            if ((long)err != 0) {
                crashlog_print("error converting sprite to RLE, sp\x95n ID ");
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
            crashlog_print("resource not found, sp\x95n ID ");
            crashlog_print_num(700);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 700, resNotFound);
        }
    }

    if (GetSpriteInfo(607, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
        if (ResourceExists(rleType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&menuRolloverSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((long)err != 0) {
                menuRolloverSprite = NULL;
                crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &menuRolloverSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((long)err != 0) {
                menuRolloverSprite = NULL;
                crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
            err = ConvertSpriteToRLE(menuRolloverSprite);
            if ((long)err != 0) {
                crashlog_print("error converting sprite to RLE, sp\x95n ID ");
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
            crashlog_print("resource not found, sp\x95n ID ");
            crashlog_print_num(607);
            crashlog_print("\n");
        }
    }

    if (GetSpriteInfo(650, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
        if (ResourceExists(rleType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&cursorSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((long)err != 0) {
                cursorSprite = NULL;
                crashlog_print("error loading sprite from RLE, sp\x95n ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &cursorSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((long)err != 0) {
                crashlog_print("error loading sprite from PICT, sp\x95n ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
            err = ConvertSpriteToRLE(cursorSprite);
            if ((long)err != 0) {
                crashlog_print("error converting sprite to RLE, sp\x95n ID ");
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
            crashlog_print("resource not found, sp\x95n ID ");
            crashlog_print_num(650);
            crashlog_print("\n");
        }
    }

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterNavTargetSprite, NULL, 10000, 8, 1);
    if ((long)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(10000);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 10000, err);
    }
    SWLockSprite(masterNavTargetSprite);
    err = SWCreateRLEData(masterNavTargetSprite, 0);
    if ((long)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(10000);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 10000, err);
    }

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterWeapTargetSprite, NULL, 10008, 16, 1);
    if ((long)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(10008);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 10008, err);
    }
    SWLockSprite(masterWeapTargetSprite);
    err = SWCreateRLEData(masterWeapTargetSprite, 0);
    if ((long)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(10008);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 10008, err);
    }
    IterateInGameEventLoop(1);

    for (i = 0; i < 768; i++)
        targetInfoPic[i] = GetPicture(i + 3000);

    for (i = 0; i < 10; i++)
        staticPat[i] = GetPixPat(i + 128);
    IterateInGameEventLoop(1);

    for (i = 0, resID = 1000, q = masterSmokeSprite; i < 2; i++, resID += 8, q++) {
        masterSmokeSprite[i] = 0;
        err = SWCreateSpriteFromCicnResource(gSpriteWorld, q, NULL, resID, 8, 1);
        if ((long)err == 0 && masterSmokeSprite[i] != NULL) {
            SWLockSprite(masterSmokeSprite[i]);
            err = SWCreateRLEData(masterSmokeSprite[i], 0);
            if ((long)err != 0) {
                crashlog_print("error converting sprite to RLE, sp\x95n ID ");
                crashlog_print_num(resID);
                crashlog_print("\n");
                ReportSpriteLoadError(4, resID, err);
            }
        } else
            masterSmokeSprite[i] = 0;
    }
    IterateInGameEventLoop(1);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &alertSprite, NULL, 18000, 2, 1);
    if ((long)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(18000);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 18000, err);
    }
    SWLockSprite(alertSprite);
    err = SWCreateRLEData(alertSprite, 0);
    if ((long)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(18000);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 18000, err);
    }
    SWSetSpriteDrawProc(alertSprite, BlitPixieRLETranslucentDrawProc);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &doubleTimeSprite, NULL, 20000, 1, 1);
    if ((long)err != 0) {
        crashlog_print("error loading sprite from cicn, ID ");
        crashlog_print_num(20000);
        crashlog_print("\n");
        ReportSpriteLoadError(5, 20000, err);
    }
    SWLockSprite(doubleTimeSprite);
    err = SWCreateRLEData(doubleTimeSprite, 0);
    if ((long)err != 0) {
        crashlog_print("error converting cicn sprite to RLE, ID ");
        crashlog_print_num(20000);
        crashlog_print("\n");
        ReportSpriteLoadError(4, 20000, err);
    }
    SWSetSpriteDrawProc(doubleTimeSprite, BlitPixieRLETranslucentDrawProc);
    IterateInGameEventLoop(1);

    redMapArrow = GetCIcon(15000);
    greenMapArrow = GetCIcon(15001);
}

/* LoadShipSprites (mac _LoadShipSprites). STATE: MATCH 100.0%, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aeda0
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
        HandleIntroMusic(0);
        count = LoadExtendedShipSprites(shipType);
        if (count > 0) {
            AdvanceLoadBar(1.0);

            if (targetInfoPic[shipType]) {
                gShipTypes[shipType].targetPictID = shipType;
            } else {
                gShipTypes[shipType].targetPictID = spriteIndex = gShipTypes[shipType].spriteIndex;
                if (shipType != kMaxShipTypes - 1) {
                    if (crashlog_isactive()) {
                        if (targetInfoPic[gShipTypes[shipType].targetPictID] == 0) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(gShipTypes[shipType].name);
                            crashlog_print(" (");
                            crashlog_print_num(shipType + 128);
                            crashlog_print(") lacks target picture\n");
                        }
                    }
                }
            }

            if (ResourceExists(kPictType, shipType + 5000)) {
                gShipTypes[shipType].shipyardPictID = shipType + 5000;
            } else {
                gShipTypes[shipType].shipyardPictID = gShipTypes[shipType].spriteIndex + 5000;
                if (crashlog_isactive()) {
                    if (gShipTypes[shipType].buyRandom != 0 || gShipTypes[shipType].hireRandom != 0) {
                        if (!ResourceExists(kPictType, gShipTypes[shipType].spriteIndex + 5000)) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(gShipTypes[shipType].name);
                            crashlog_print(" (");
                            crashlog_print_num(shipType + 128);
                            crashlog_print(") lacks shipyard picture\n");
                        }
                    }
                }
            }
        }

        IterateInGameEventLoop(1);
    }
}

/* ShipObj's first six words are its six sprite instances; shipobj.h still
   spells that span `char *unk0; char unk4[0x14];`, and this is the only
   function in the tree that touches all six, so the names live here until
   the header can be widened on a quiet tree. */

#define shipSprites(i) (*(ShipSprites *)&shipData[i])


/* InitSprites (mac _InitSprites). STATE: MATCH 100.0%, exe 1542 / obj 1542, byte-identical
   yes (5907 bytes), relocregcheck 0 of 566. OPEN: none. */
// FUNCTION: EVNOVA 0x004af020
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

        if ((gShipTypes[i].shanFlags & kShipAngleTable)
                && gShipTypes[i].framesPer > 0) {
            gShipTypes[i].frameAngles =
                (short *)AllocAPointer(gShipTypes[i].framesPer * 2);
            if (gShipTypes[i].frameAngles == NULL)
                SWAssertFail("Setup.c", 1740);
            step = 360.0 / gShipTypes[i].framesPer;
            for (j = 0; j < gShipTypes[i].framesPer; j++) {
                angle = j * step;
                vel.x = vel.y = 0.0f;
                Accel(angle, 100.0, &vel);
                if (angle <= 90.0 || angle > 270.0) {
                    vel.x *= gShipTypes[i].compress[0][0];
                    vel.y *= gShipTypes[i].compress[0][1];
                } else {
                    vel.x *= gShipTypes[i].compress[1][0];
                    vel.y *= gShipTypes[i].compress[1][1];
                }
                gShipTypes[i].frameAngles[j] = CalcInterceptAngle(&origin, &vel);
            }
        }
    }

    IterateInGameEventLoop(1);

    for (i = kMaxShips - 1; i >= 0; i--) {
        HandleIntroMusic(0);
        err = SWCloneSprite(masterShipSprite[0], &shipSprites(i).sprite, NULL);
        if ((long)err != 0) {
            crashlog_print("error cloning ship base sprite, #");
            crashlog_print_num(i);
            crashlog_print("\n");
            ReportSpriteLoadError(1, 0, err);
        }
        SWCloneFramesFromMaster(shipSprites(i).sprite, masterShipSprite[0]);
        SWAddSprite(shipLayer, shipSprites(i).sprite);
        SWSetSpriteVisible(shipSprites(i).sprite, 0);
        SWSetCurrentFrameIndex(shipSprites(i).sprite, 0);
        SWSetSpriteDrawProc(shipSprites(i).sprite, BlitPixieRLETranslucentDrawProc);
        SWSetSpriteCollideProc(shipSprites(i).sprite, ShipCollisionHandler);
        shipSprites(i).sprite->userData = i;
        SWLockSprite(shipSprites(i).sprite);

        if (firstShipAltIndex != -1) {
            err = SWCloneSprite(masterShipAltSprite[firstShipAltIndex],
                                &shipSprites(i).altSprite, NULL);
            if ((long)err != 0) {
                crashlog_print("error cloning ship alt sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipSprites(i).altSprite);
            SWSetSpriteVisible(shipSprites(i).altSprite, 0);
            SWSetCurrentFrameIndex(shipSprites(i).altSprite, 0);
            SWSetSpriteDrawProc(shipSprites(i).altSprite,
                                BlitPixieRLETranslucentDrawProc);
            shipSprites(i).altSprite->userData = i;
            SWSetSpriteCollideProc(shipSprites(i).altSprite, ShipCollisionHandler);
            SWLockSprite(shipSprites(i).altSprite);
            shipSprites(i).altSprite->translucency = 0;
            shipSprites(i).altSprite->tintMode = 0;
        }
        if (firstShipGlowIndex != -1) {
            err = SWCloneSprite(masterShipGlowSprite[firstShipGlowIndex],
                                &shipSprites(i).glowSprite, NULL);
            if ((long)err != 0) {
                crashlog_print("error cloning ship glow sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipSprites(i).glowSprite);
            SWSetSpriteVisible(shipSprites(i).glowSprite, 0);
            SWSetCurrentFrameIndex(shipSprites(i).glowSprite, 0);
            SWSetSpriteDrawProc(shipSprites(i).glowSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipSprites(i).glowSprite);
            shipSprites(i).glowSprite->translucency = 0x20;
            shipSprites(i).glowSprite->tintRed = 0x20;
            shipSprites(i).glowSprite->tintGreen = 0x20;
            shipSprites(i).glowSprite->tintBlue = 0x20;
            shipSprites(i).glowSprite->tintMode = 0;
            shipSprites(i).glowSprite->collisionsDisabled = 1;
        }
        if (firstShipLightIndex != -1) {
            err = SWCloneSprite(masterShipLightSprite[firstShipLightIndex],
                                &shipSprites(i).lightSprite, NULL);
            if ((long)err != 0) {
                crashlog_print("error cloning ship light sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipSprites(i).lightSprite);
            SWSetSpriteVisible(shipSprites(i).lightSprite, 0);
            SWSetCurrentFrameIndex(shipSprites(i).lightSprite, 0);
            SWSetSpriteDrawProc(shipSprites(i).lightSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipSprites(i).lightSprite);
            shipSprites(i).lightSprite->translucency = 0x20;
            shipSprites(i).lightSprite->tintRed = 0x20;
            shipSprites(i).lightSprite->tintGreen = 0x20;
            shipSprites(i).lightSprite->tintBlue = 0x20;
            shipSprites(i).lightSprite->tintMode = 0;
            shipSprites(i).lightSprite->collisionsDisabled = 1;
        }
        if (firstShipWeapIndex != -1) {
            err = SWCloneSprite(masterShipWeaponSprite[firstShipWeapIndex],
                                &shipSprites(i).weapSprite, NULL);
            if ((long)err != 0) {
                crashlog_print("error cloning ship weapon sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipSprites(i).weapSprite);
            SWSetSpriteVisible(shipSprites(i).weapSprite, 0);
            SWSetCurrentFrameIndex(shipSprites(i).weapSprite, 0);
            SWSetSpriteDrawProc(shipSprites(i).weapSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipSprites(i).weapSprite);
            shipSprites(i).weapSprite->translucency = 0x20;
            shipSprites(i).weapSprite->tintRed = 0x20;
            shipSprites(i).weapSprite->tintGreen = 0x20;
            shipSprites(i).weapSprite->tintBlue = 0x20;
            shipSprites(i).weapSprite->tintMode = 0;
            shipSprites(i).weapSprite->collisionsDisabled = 1;
        }
        if (firstShipShieldIndex != -1) {
            err = SWCloneSprite(masterShipShieldSprite[firstShipShieldIndex],
                                &shipSprites(i).shieldSprite, NULL);
            if ((long)err != 0) {
                crashlog_print("error cloning ship shield sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(shipLayer, shipSprites(i).shieldSprite);
            SWSetSpriteVisible(shipSprites(i).shieldSprite, 0);
            SWSetCurrentFrameIndex(shipSprites(i).shieldSprite, 0);
            SWSetSpriteDrawProc(shipSprites(i).shieldSprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(shipSprites(i).shieldSprite);
            shipSprites(i).shieldSprite->translucency = 0x20;
            shipSprites(i).shieldSprite->tintRed = 0x20;
            shipSprites(i).shieldSprite->tintGreen = 0x20;
            shipSprites(i).shieldSprite->tintBlue = 0x20;
            shipSprites(i).shieldSprite->tintMode = 0;
        }
    }

    for (i = 0; i < kMasterStellarSprites; i++)
        masterStellarSprite[i] = NULL;

    HandleIntroMusic(0);
    LoadMasterStellarSprite(0);

    for (i = 0; i < kNavSlots; i++) {
        err = SWCreateEmptySprite(&stellarSprite[i], NULL);
        if ((long)err != 0)
            ReportSpriteLoadError(1, 0, err);
        SWCloneFramesFromMaster(stellarSprite[i], masterStellarSprite[0]);
        SWAddSprite(stellarLayer, stellarSprite[i]);
        SWSetCurrentFrameIndex(stellarSprite[i], 0);
        SWSetSpriteVisible(stellarSprite[i], 0);
        SWSetSpriteDrawProc(stellarSprite[i], BlitPixieRLETranslucentDrawProc);
        SWLockSprite(stellarSprite[i]);
    }

    HandleIntroMusic(0);

    for (i = 0; i < kMaxExplods; i++) {
        err = SWCloneSprite(masterExplodSprite[0], &explodData[i].sprite, NULL);
        if ((long)err != 0) {
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

    HandleIntroMusic(0);

    for (i = 0; i < kMaxShots; i++) {
        err = SWCloneSprite(masterShotSprite[0], &shotData[i].sprite, NULL);
        if ((long)err != 0) {
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

    HandleIntroMusic(0);

    for (i = 0; i < kMaxBoxes; i++) {
        err = SWCloneSprite(masterBoxSprite[0], &boxData[i].sprite, NULL);
        if ((long)err != 0) {
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
        if ((long)err != 0) {
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
        if ((long)err != 0) {
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

    if (masterSmokeSprite[0] == NULL) {
        for (i = 0; i < kMaxDebris; i++)
            gDebrisSlots[i].sprite = NULL;
    } else {
        for (i = 0; i < kMaxDebris; i++) {
            err = SWCloneSprite(masterSmokeSprite[0], &gDebrisSlots[i].sprite, NULL);
            if ((long)err != 0) {
                crashlog_print("error cloning smoke sprite, #");
                crashlog_print_num(i);
                crashlog_print("\n");
                ReportSpriteLoadError(1, 0, err);
            }
            SWAddSprite(smokeLayer, gDebrisSlots[i].sprite);
            SWSetCurrentFrameIndex(gDebrisSlots[i].sprite, 0);
            SWSetSpriteVisible(gDebrisSlots[i].sprite, 0);
            SWSetSpriteDrawProc(gDebrisSlots[i].sprite,
                                BlitPixieRLETranslucentDrawProc);
            SWLockSprite(gDebrisSlots[i].sprite);
        }
    }

    HandleIntroMusic(0);

    for (i = 0; i < kMaxAsteroids; i++) {
        err = SWCloneSprite(masterAsteroidSprite[0], &asteroidData[i].sprite, NULL);
        if ((long)err != 0) {
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
        if ((long)err != 0) {
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
        if ((long)err != 0) {
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

    HandleIntroMusic(0);

    SWAddSprite(targetLayer, alertSprite);
    SWSetCurrentFrameIndex(alertSprite, 0);
    SWSetSpriteVisible(alertSprite, 0);
    SWSetSpriteDrawProc(alertSprite, BlitPixieRLETranslucentDrawProc);
    SWLockSprite(alertSprite);
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

    HandleIntroMusic(0);

    SetRect(&commRect, GetCWindowPortRect(gWind)->left + 25,
            GetCWindowPortRect(gWind)->bottom - 5,
            GetCWindowPortRect(gWind)->right - 244,
            GetCWindowPortRect(gWind)->bottom - 5);
    commRect.top -= 29470 / (commRect.right - commRect.left);
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



/* LoadSounds. STATE: MATCH 100%, exe 182 / obj 182 instrs. The census computes each
   resource ID in the call; the baseID/id induction variables were invented (mwcc's own
   strength reduction makes them). OPEN: none. Notes: Research/notes/loadsounds.md */
// FUNCTION: EVNOVA 0x004b0740
void LoadSounds(void)
{
    short i;
    short set;
    short msg;
    AudioDesc info;
    Handle h;
    unsigned short vol;

    for (set = 0; set < kVoiceSets; set++) {
        for (msg = 0; msg < kVoiceMessages; msg++) {
            voiceResourceCount[set][msg] = 0;
            for (i = 0; i < kVoiceClipsMax; i++) {
                if (ResourceExists(kSoundType,
                        kFirstVoiceID + set * kVoiceSetStride + msg * kVoiceMsgStride + i) == 0)
                    break;
                voiceResourceCount[set][msg] += 1;
            }
        }
    }

    for (i = 0; i <= 1; i++) {
        jumpSoundLength[i] = kDefaultLoopTicks;
        h = GetResource(kSoundType, i + kFirstResourceID);
        if (h != NULL) {
            LockAHandle(h);
            if (ParseSoundFileImage(&info, (Word8 *)*h, GetAHandleSize(h)) == 0)
                jumpSoundLength[i] = (info.SampleCount * kTicksPerSecond
                                            / info.SampleRate);
            ReleaseResource(h);
        }
    }

    warpUpSnd[0] = LoadSoundResource(0x80);
    warpUpSnd[1] = LoadSoundResource(0x81);
    warpOutSnd = LoadSoundResource(0x82);

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

    for (i = 0; i <= kEngineSounds - 1; i++)
        beepSnd[i] = LoadSoundResource(i + kFirstEngineID);

    for (i = 0; i < kShotSounds; i++)
        shotSnd[i] = LoadSoundResource(i + kFirstShotID);

    for (i = 0; i < kExplodSounds; i++)
        explodSnd[i] = LoadSoundResource(i + kFirstExplodID);

    redAlertSnd = LoadSoundResource(0x172);
    klaxxonSnd = LoadSoundResource(0x173);
    escapePodSnd = LoadSoundResource(0x174);
    cloakSnd[0] = LoadSoundResource(0x17c);
    cloakSnd[1] = LoadSoundResource(0x17d);
    hardDockSnd = LoadSoundResource(0x186);
    clickSnd = LoadSoundResource(0x258);
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
void LoadMasterStellarSprite(short type)
{
    short spriteID, maskID, xSize, ySize, xTiles, yTiles;
    long resID;
    unsigned long rleType;
    short err;
    SpriteRec *entry;

    resID = type + kStellarSpinResBase;
    if (GetSpriteInfo(resID, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
        if (ResourceExists(rleType, spriteID)) {
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


#define kMaxRoidTypes    16
#define kEscortCount     4

#define kBasePriceStringID 9300     /* plugin string, else STR# 4004 item i+1 */
#define kBasePriceListID   4004
#define kSystemGovtUnset   (-32767)
#define kRoidGrey          0x8012   /* 8-bit asteroid colour, run through Color2Index */
#define kRoidPixel         0x1ce7   /* the direct 15-bit pixel otherwise */



/* InitObjects. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b0c20
void InitObjects(void)
{
    short i, j, k;
    RGBColor c;
    Str255 buf;
    long value;

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
        *(short *)shipData[i].unk6e = 1;
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
        *(short *)shipData[i].unk9C = 0;
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
        for (j = 0; j < kCargoTypes; j++)
            shipData[i].cargo[j] = 0;
        shipData[i].sprite = 0;
        shipData[i].altSprite = 0;
        shipData[i].glowSprite = 0;
        shipData[i].lightSprite = 0;
        shipData[i].weapSprite = 0;
        shipData[i].shieldSprite = 0;
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        gShipTypes[i].techLevel = kResUnloaded;
        gShipTypes[i].shield = 1;
        gShipTypes[i].armor = 1;
        gShipTypes[i].strength = 1;
        gShipTypes[i].dispWeight = 1;
        gShipTypes[i].upgradeTo = -1;
        gShipTypes[i].altSets = 0;
        gShipTypes[i].baseSets = 0;
        gShipTypes[i].transparency = 0;
        gShipTypes[i].keyCarried = -1;
        gShipTypes[i].shanFlags = 0;
        gShipTypes[i].contributes[0] = 0;
        gShipTypes[i].contributes[1] = 0;
        gShipTypes[i].spriteIndex = i;
        gShipTypes[i].mass = -1;
        gShipTypes[i].hireRandom = 100;
        gShipTypes[i].buyRandom = 100;
        gShipTypes[i].dailyHireRoll = 50;
        gShipTypes[i].dailySaleRoll = 50;
        for (j = 0; j < kMaxWeapTypes; j++) {
            gShipTypes[i].weapCount[j] = 0;
            gShipTypes[i].weapAmmo[j] = 0;
        }
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                gShipTypes[i].weapPos[j][k][0] = 0;
                gShipTypes[i].weapPos[j][k][1] = 0;
                gShipTypes[i].weapPosZ[j][k] = 0;
            }
        }
        gShipTypes[i].compress[0][0] = 1.0f;
        gShipTypes[i].compress[0][1] = 0.71f;
        gShipTypes[i].compress[1][0] = 1.0f;
        gShipTypes[i].compress[1][1] = 0.71f;
        gShipTypes[i].frameAngles = 0;
        masterShipSprite[i] = 0;
        masterShipGlowSprite[i] = 0;
        masterShipLightSprite[i] = 0;
        masterShipWeaponSprite[i] = 0;
        masterShipAltSprite[i] = 0;
        masterShipShieldSprite[i] = 0;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        gOutfits[i].techLevel = kOutfTechAbsent;
        gOutfits[i].dispWeight = 0;
        gOutfits[i].contributes[0] = 0;
        gOutfits[i].contributes[1] = 0;
        gOutfits[i].requires[0] = 0;
        gOutfits[i].requires[1] = 0;
        gOutfits[i].itemClass = -1;
        gOutfits[i].scanMask = 0;
        gOutfits[i].buyRandom = 0;
        gOutfits[i].dailyRoll = 50;
        gOutfits[i].sameNameIndex = i;
        gOutfits[i].govtRestrict = -1;
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
        gSystems[i].loaded = 0;
        gSystems[i].prepared = 0;
        gSystems[i].govt = kSystemGovtUnset;
        gSystems[i].asteroidCount = 0;
        gSystems[i].astTypes = 0;
        gSystems[i].interference = 0;
        gSystems[i].exploreLevel = 0;
        gSystems[i].govt = -1;
        gSystems[i].xPos = gSystems[i].yPos = 0;
        for (j = 0; j < kMaxSystemLinks; j++)
            gSystems[i].links[j] = -1;
        for (j = 0; j < kNavSlots; j++)
            gSystems[i].navDefault[j] = -1;
        for (j = 0; j < 8; j++) {
            gSystems[i].fleets[j] = -1;
            gSystems[i].dudeTypes[j] = -1;
            gSystems[i].dudeProbs[j] = 0;
        }
        gSystems[i].fleetCount = 0;
        gSystems[i].fleetProbTotal = 0;
        gSystems[i].known = 0;
        gSystems[i].colocNext = -1;
        gSystems[i].colocGroup = -1;
        gSystems[i].rgb.red = 0;
        gSystems[i].rgb.green = 0;
        gSystems[i].rgb.blue = 0;
        gSystems[i].murk = 0;
        gSystems[i].reinforceFleet = -1;
        gSystems[i].reinforceTime = 0;
        gSystems[i].reinforceDaysLeft = 0;
        gSystems[i].reinforceInterval = 0;
        gSystems[i].reinforceTimer = 0.0f;
        strncpy((char *)gSystems[i].name, (char *)"\p", 31);
    }

    for (i = 0; i < kMaxDudes; i++) {
        dudeData[i].loaded = 0;
        for (j = 0; j < 16; j++)
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

    for (i = 0; i < kMaxRoidTypes; i++) {
        asteroidType[i].strength = 100;
        asteroidType[i].yieldQty = 0;
        asteroidType[i].yieldType = -1;
        asteroidType[i].fragType[0] = -1;
        asteroidType[i].fragType[1] = -1;
        asteroidType[i].fragCount = 0;
        asteroidType[i].partCount = 50;
        asteroidType[i].frameRate = 1.0f;
        if (gSpriteWorld->pixelDepth == 8) {
            c.red = c.green = c.blue = kRoidGrey;
            asteroidType[i].color = Color2Index(&c);
        } else {
            asteroidType[i].color = kRoidPixel;
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
        gWeapons[i].guidance = kResAbsent;
        for (j = 0; j < 4; j++)
            gWeapons[i].jamVuln[j] = 0;
    }

    playerEscortOrders[0] = -1;
    playerEscortOrders[1] = -1;
    playerEscortOrders[2] = -1;
    playerEscortOrders[3] = -1;

    for (i = 0; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;

    for (i = 0; i <= 5; i++) {
        if (!LoadPluginString(i + kBasePriceStringID, buf))
            GetIndString(buf, kBasePriceListID, i + 1);
        StringToNum(buf, &value);
        basePrice[i] = value;
    }

    for (i = 0; i < kMaxMissions; i++)
        missionRandom[i] = Rand(100) + 1;

    for (i = 0; i < kMaxCrons; i++)
        gCrons[i].loaded = 0;

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
    Microseconds(&lastFrameMicroseconds);
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

#define kSpobFlags2_20 0x20

/* ResetPlayer (mac _ResetPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b3350
void ResetPlayer(char newPilot)
{
    short i;
    short j;
    short g;
    short link;
    short coloc;

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
        shipData->weap[i].count = gShipTypes[shipData->type].weapCount[i];
        if (gShipTypes[shipData->type].weapAmmo[i] > 0) {
            j = gWeapons[i].ammoType;
            if (j >= 0 && j < kMaxWeapTypes)
                shipData->weap[j].ammo = gShipTypes[shipData->type].weapAmmo[i];
            else
                shipData->weap[i].ammo = gShipTypes[shipData->type].weapAmmo[i];
        }
    }
    for (i = 0; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;
    for (i = 0; i < kMaxOutfits; i++) {
        if (newPilot || gOutfits[i].persistent == 0)
            shipSystems[i] = 0;
    }
    shipData->cargo[0] = 0;
    shipData->cargo[1] = 0;
    shipData->cargo[2] = 0;
    shipData->cargo[3] = 0;
    shipData->cargo[4] = 0;
    shipData->cargo[5] = 0;
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
        gDebrisSlots[i].timer = kFreeSlot;
        gDebrisSlots[i].angle = 0.0f;
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        gShipTypes[i].dailyHireRoll = Rand(kDailyRollRange) + 1;
        gShipTypes[i].dailySaleRoll = Rand(kDailyRollRange) + 1;
    }
    for (i = 0; i < kMaxOutfits; i++)
        gOutfits[i].dailyRoll = Rand(kDailyRollRange) + 1;

    killShots = 1;
    killBoxes = 1;
    killExplods = 1;
    killSmoke = 1;
    weapLawFlag = 0;
    smuggleFlag = 0;
    greenMissionSyst = -1;
    commCount = 0;
    missionSoundID = -1;

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0)
            AbortMission(i, 0);
        currentMissionObjectives[i].active = 0;
    }
    for (i = 0; i < kMaxMissions; i++)
        missionRandom[i] = Rand(kDailyRollRange) + 1;

    globalPriceFlux[0] = kPriceFluxBase;
    localPriceFlux[0] = kPriceFluxBase;
    globalPriceFlux[1] = kPriceFluxBase;
    localPriceFlux[1] = kPriceFluxBase;

    if (newPilot) {
        shipData->credits = kStartingCredits;
        for (i = 0; i < kMaxSystems; i++) {
            gSystems[i].exploreLevel = 0;
            g = gSystems[i].govt;
            if (g >= 0 && g < 256)
                playerLegalStatus[i] = govtData[g].initialRec;
            else
                playerLegalStatus[i] = 0;
            gSystems[i].reinforceDaysLeft = 0;
            gSystems[i].reinforceTimer = 0.0f;
        }
        for (i = 0; i < kNavSlots; i++) {
            link = gSystems[shipData->curSystem].links[i];
            if (link != -1 && gSystems[link].loaded != 0) {
                gSystems[link].exploreLevel = 1;
                coloc = FindFirstCoLocatedSystem(link);
                if (coloc >= 0 && coloc < kMaxSystems)
                    gSystems[coloc].exploreLevel = 1;
            }
        }
        for (i = 0; i < kMaxNebulae; i++)
            nebulaData[i].explored = 0;
        for (i = 0; i < kMaxPersons; i++)
            personData[i].grudge = 0;
        playerEscortOrders[0] = -1;
        playerEscortOrders[1] = -1;
        playerEscortOrders[2] = -1;
        playerEscortOrders[3] = -1;
    }
    mapScroll.x = gSystems[shipData->curSystem].xPos;
    mapScroll.y = gSystems[shipData->curSystem].yPos;
    gSystems[shipData->curSystem].reinforceTimer = kFreeSlot;
    gSystems[shipData->curSystem].reinforceDaysLeft = 0;
    lastMissionStellar = -1;
    recentActivatedRank = -1;
    playerPaint.red = playerPaint.green = playerPaint.blue =
        kPlayerPaintLevel;
    ResetPlayerPrecalcedValues();
}

/* ResetPlayerRecord. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4220
void ResetPlayerRecord(char newPilot)
{
    short i;

    if (newPilot != 0)
        playerKills = 0;

    for (i = 0; i < 2048; i++) {
        short minStatus;

        if (gSystems[i].govt < 0)
            minStatus = 0;
        else
            minStatus = govtData[gSystems[i].govt].initialRec;
        if (playerLegalStatus[i] < minStatus)
            playerLegalStatus[i] = minStatus;
    }
    for (i = 0; i < 2048; i++) {
        if ((stellarData[i].flags2 & kSpobFlags2_20) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
    }
    recentActivatedRank = 0xffff;
}

/*
 * The bounds come from the video layer's current mode rather than from a
 * window, which is why the two sources are BurgerLib globals.
 */

/* ResetScreenRect. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b42f0
void ResetScreenRect(void)
{
    mainWindRect.top = 0;
    mainWindRect.left = 0;
    mainWindRect.bottom = gVideoHeight;
    mainWindRect.right = gVideoWidth;
}

/*
 * No mac twin: 1.1.1 rewrote its preferences layer for OS X (_LoadPrefs is 2243
 * bytes and EVPrefsDialogController is Objective-C), so these two names
 * describe the body.
 */

/* SetDefaultPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4320
void SetDefaultPrefs(void)
{
    short depth;

    if (gSpriteWorld != NULL)
        depth = gSpriteWorld->pixelDepth;
    else
        depth = 16;

    musicEnabledPref = 1;
    DAT_005914d1 = 1;
    volumePref = 5;
    DAT_005914d2 = 0;
    gEarlyOutFlag = 0;
    gNoDebris = 0;
    windowedPref = 0;
    yieldEventsPref = 1;
    DAT_005914d4 = 0;
    loadShipSetsPref = 1;
    loadShipGlowPref = 1;
    parallaxStarsPref = 1;
    planetSoundsPref = 1;
    noHyperGammaPref = 0;
    noVersionCheckPref = 1;
    gShowInfluence = 0;
    if (depth >= 16) {
        loadShipWeaponsPref = 1;
        loadShipLightsPref = 1;
    } else {
        loadShipWeaponsPref = 0;
        loadShipLightsPref = 0;
    }
    gammaPref = 3;
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
    keyPrefs.cheatArmor = 0x62;
    keyPrefs.cheatDamage = 0x67;
    keyPrefs.cheatAmmo = 0x37;
    keyPrefs.cheatCredits = 5;
    keyPrefs.cheatSpare = 0x65;
    keyPrefs.cheatShipNext = 0x4e;
    keyPrefs.cheatShipPrev = 0x4a;
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

#pragma opt_repositioncode off


/* 0x0058f8b4 is the IAT slot, not a variable: the exe reaches
   GetSystemMetrics through it because the declaration is dllimport. */
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define kModeFlagBase   2
#define kModeDepth      15
#define kModeFailAlert  0x25

/* SetupMonitor. STATE: MATCH 100%, exe 46 / obj 46, byte-identical yes (under its
   scoped opt_repositioncode pragma).
   OPEN: none */
// FUNCTION: EVNOVA 0x004b45e0
void SetupMonitor(void)
{
    long flags;
    long width;
    long height;

    flags = (windowedPref != 0 ? 0 : 1) | kModeFlagBase;
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    if (((flags & 1) == 0 && width <= 1024)
        || SetDisplayToSize(1024, 768, kModeDepth, flags) != 0) {
        if (SetDisplayToSize(800, 600, kModeDepth, flags) != 0)
            ShowIndAlert(kModeFailAlert);
    }
    screenMinusDockRect.top = 0;
    screenMinusDockRect.left = 0;
    screenMinusDockRect.bottom = gVideoHeight;
    screenMinusDockRect.right = gVideoWidth;
}

#pragma opt_repositioncode reset

#define kSpobDominated  0x20    /* flags2: starts under someone's control */
#define kSpobStartsDestroyed  0x40    /* flags2: starts destroyed */

/* ResetGameData. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4690
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
        if ((stellarData[i].flags2 & kSpobDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
        if ((stellarData[i].flags2 & kSpobStartsDestroyed) != 0) {
            stellarData[i].deadCounter = -1;
            if (stellarData[i].deadDays < 0)
                stellarData[i].deadDaysLeft = 1;
            else
                stellarData[i].deadDaysLeft = stellarData[i].deadDays;
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

#pragma opt_repositioncode off

/* ConvertSpriteToRLE (mac _ConvertSpriteToRLE). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4cf0
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

#pragma opt_repositioncode reset

/*
 * Every out-parameter is optional, so a caller that wants only the tile counts
 * passes NULL for the rest. Field names are the Nova Bible's documented spin
 * layout (six shorts: spriteID, maskID, xSize, ySize, xTiles, yTiles), which
 * the six out-parameters pick up in order.
 */


typedef struct SpinRec {
    short spriteID;     /* +0x00 */
    short maskID;       /* +0x02 */
    short xSize;        /* +0x04 */
    short ySize;        /* +0x06 */
    short xTiles;       /* +0x08 */
    short yTiles;       /* +0x0a */
} SpinRec, **SpinHandle;

/* the setup object's globals, here and after ShanRec below */
SpinRec spriteInfo;
SpinHandle spriteInfoHand;

/* GetSpriteInfo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b4e10
char GetSpriteInfo(short id, short *spriteID, short *maskID, short *xSize,
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


/*
 * The `shan` resource, resized to 0xc0 on the way out.  Field names follow
 * the Nova Bible; every one of them is read below.
 */
typedef struct ShanRec {
    short baseImageID;          /* +0x00 */
    short baseMaskID;           /* +0x02 */
    short baseSetCount;         /* +0x04 */
    short baseXSize;            /* +0x06 */
    short baseYSize;            /* +0x08 */
    short baseTransparency;     /* +0x0a */
    short altImageID;           /* +0x0c */
    short altMaskID;            /* +0x0e */
    short altSetCount;          /* +0x10 */
    short altXSize;             /* +0x12 */
    short altYSize;             /* +0x14 */
    short glowImageID;          /* +0x16 */
    short glowMaskID;           /* +0x18 */
    short glowXSize;            /* +0x1a */
    short glowYSize;            /* +0x1c */
    short lightImageID;         /* +0x1e */
    short lightMaskID;          /* +0x20 */
    short lightXSize;           /* +0x22 */
    short lightYSize;           /* +0x24 */
    short weapImageID;          /* +0x26 */
    short weapMaskID;           /* +0x28 */
    short weapXSize;            /* +0x2a */
    short weapYSize;            /* +0x2c */
    short flags;                /* +0x2e */
    short animDelay;            /* +0x30 */
    short weapDecay;            /* +0x32 */
    short framesPer;            /* +0x34 */
    short blinkMode;            /* +0x36 */
    short blinkValue[4];        /* +0x38 */
    short shieldImageID;        /* +0x40 */
    short shieldMaskID;         /* +0x42 */
    short shieldXSize;          /* +0x44 */
    short shieldYSize;          /* +0x46 */
    struct {
        short x[4];             /* gun/turret/guided/beam X, four lanes */
        short y[4];             /* ... and Y */
    } mountPos[4];              /* +0x48 */
    short upCompressX;          /* +0x88 */
    short upCompressY;          /* +0x8a */
    short downCompressX;        /* +0x8c */
    short downCompressY;        /* +0x8e */
    short mountPosZ[4][4];      /* +0x90 */
    short unkB0[8];             /* +0xb0, never read: the handle is resized to 0xc0 */
} ShanRec;

ShanRec shipSpriteInfo;
ShanRec **shipSpriteInfoHand;
short firstShipGlowIndex;
short firstShipLightIndex;
short firstShipAltIndex;
short firstShipWeapIndex;
short firstShipShieldIndex;
short shipSpriteLoaded[kMaxShipTypes][6];
Rect mainWindRect;
Rect screenMinusDockRect;
long fullScreenRestore;


/* LoadExtendedShipSprites (mac _LoadExtendedShipSprites). STATE: MATCH 100.0% (1048 exact,
   421 reloc-equiv), exe 1469 / obj 1469, byte-identical yes (5945 bytes, relocs masked).
   OPEN: none. */
// FUNCTION: EVNOVA 0x004b4ee0
short LoadExtendedShipSprites(short shipType)
{
    short spinID;
    short lightID;
    short maskID;
    short count;
    short xSize;
    short setCount;
    short ySize;
    short i;
    short err;
    short total;
    long resID;
    unsigned long rleType;

    total = 0;
    resID = shipType + 128;
    shipSpriteInfoHand = (ShanRec **)GetResource(kShanType, resID);
    if (shipSpriteInfoHand != NULL) {
        LockAHandle((Handle)shipSpriteInfoHand);

        spinID = (*shipSpriteInfoHand)->baseImageID;
        maskID = (*shipSpriteInfoHand)->baseMaskID;
        xSize = (*shipSpriteInfoHand)->baseXSize;
        ySize = (*shipSpriteInfoHand)->baseYSize;
        gShipTypes[shipType].framesPer = (*shipSpriteInfoHand)->framesPer;
        if (gShipTypes[shipType].framesPer == 0)
            gShipTypes[shipType].framesPer = 36;
        setCount = (*shipSpriteInfoHand)->baseSetCount;
        if (setCount <= 0)
            setCount = 1;
        if (loadShipSetsPref != 0) {
            count = setCount;
            count *= gShipTypes[shipType].framesPer;
        } else {
            count = gShipTypes[shipType].framesPer;
        }
        gShipTypes[shipType].weapDecay = (*shipSpriteInfoHand)->weapDecay * 0.333;

        i = FindSlotEntry(spinID, shipType, 0);
        if (i != -1) {
            SWCloneSprite(masterShipSprite[i], &masterShipSprite[shipType], NULL);
            SWLockSprite(masterShipSprite[shipType]);
            shipSpriteLoaded[shipType][0] = spinID;
            gShipTypes[shipType].spriteIndex = i;
        } else {
            rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
            if (ResourceExists(rleType, spinID)) {
                err = SWCreateSpriteFromRLEResource(&masterShipSprite[shipType], NULL,
                                                    spinID, count, 0);
                if ((long)err != 0) {
                    crashlog_print("error loading ship base sprite from RLE, sh\x8an ID ");
                    crashlog_print_num(resID);
                    crashlog_print("\n");
                    ReportSpriteLoadError(2, resID, err);
                }
                SWLockSprite(masterShipSprite[shipType]);
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterShipSprite[shipType],
                                                     NULL, spinID, maskID, xSize, ySize, 0, 0,
                                                     gShipTypes[shipType].framesPer * setCount, 1);
                if ((long)err != 0) {
                    crashlog_print("error loading ship base sprite from PICT, sh\x8an ID ");
                    crashlog_print_num(resID);
                    crashlog_print("\n");
                    ReportSpriteLoadError(3, resID, err);
                }
                SWLockSprite(masterShipSprite[shipType]);
                err = ConvertSpriteToRLE(masterShipSprite[shipType]);
                if ((long)err != 0) {
                    crashlog_print("error converting ship base sprite to RLE, sh\x8an ID ");
                    crashlog_print_num(resID);
                    crashlog_print("\n");
                    ReportSpriteLoadError(4, resID, err);
                }
                masterShipSprite[shipType]->numFrames =
                    gShipTypes[shipType].framesPer * setCount;
            }
        }
        shipSpriteLoaded[shipType][0] = spinID;
        total += masterShipSprite[shipType]->numFrames;

        gShipTypes[shipType].shanFlags = (*shipSpriteInfoHand)->flags;
        gShipTypes[shipType].animDelay = (*shipSpriteInfoHand)->animDelay;
        gShipTypes[shipType].transparency = (*shipSpriteInfoHand)->baseTransparency;
        if (loadShipSetsPref != 0)
            gShipTypes[shipType].baseSets = setCount;
        else
            gShipTypes[shipType].baseSets = 1;

        if ((*shipSpriteInfoHand)->altImageID > 0 && (*shipSpriteInfoHand)->altSetCount > 0) {
            if (loadShipSetsPref != 0)
                gShipTypes[shipType].altSets = (*shipSpriteInfoHand)->altSetCount;
            else
                gShipTypes[shipType].altSets = 1;
            spinID = (*shipSpriteInfoHand)->altImageID;
            maskID = (*shipSpriteInfoHand)->altMaskID;
            xSize = (*shipSpriteInfoHand)->altXSize;
            ySize = (*shipSpriteInfoHand)->altYSize;

            i = FindSlotEntry(spinID, shipType, 1);
            if (i != -1) {
                SWCloneSprite(masterShipAltSprite[i], &masterShipAltSprite[shipType], NULL);
                SWLockSprite(masterShipAltSprite[shipType]);
            } else {
                rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
                if (ResourceExists(rleType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipAltSprite[shipType], NULL,
                                                        spinID,
                                                        gShipTypes[shipType].altSets *
                                                            gShipTypes[shipType].framesPer, 0);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship alt sprite from RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, resID, err);
                    }
                    SWLockSprite(masterShipAltSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipAltSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0,
                                                         gShipTypes[shipType].framesPer *
                                                             gShipTypes[shipType].altSets, 1);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship alt sprite from PICT, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, resID, err);
                    }
                    masterShipAltSprite[shipType]->numFrames =
                        gShipTypes[shipType].framesPer * gShipTypes[shipType].altSets;
                    SWLockSprite(masterShipAltSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipAltSprite[shipType]);
                    if ((long)err != 0) {
                        crashlog_print("error converting ship alt sprite to RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, resID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][1] = spinID;
            total += masterShipAltSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->glowImageID > 0 && loadShipGlowPref != 0) {
            spinID = (*shipSpriteInfoHand)->glowImageID;
            maskID = (*shipSpriteInfoHand)->glowMaskID;
            xSize = (*shipSpriteInfoHand)->glowXSize;
            ySize = (*shipSpriteInfoHand)->glowYSize;

            i = FindSlotEntry(spinID, shipType, 2);
            if (i != -1) {
                SWCloneSprite(masterShipGlowSprite[i], &masterShipGlowSprite[shipType], NULL);
                SWLockSprite(masterShipGlowSprite[shipType]);
            } else {
                rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
                if (ResourceExists(rleType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipGlowSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship glow sprite from RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, resID, err);
                    }
                    SWLockSprite(masterShipGlowSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipGlowSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0,
                                                         gShipTypes[shipType].framesPer * setCount, 1);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship glow sprite from PICT, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, resID, err);
                    }
                    masterShipGlowSprite[shipType]->numFrames =
                        gShipTypes[shipType].framesPer * setCount;
                    SWLockSprite(masterShipGlowSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipGlowSprite[shipType]);
                    if ((long)err != 0) {
                        crashlog_print("error converting ship glow sprite to RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, resID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][2] = spinID;
            total += masterShipGlowSprite[shipType]->numFrames;
        }

        lightID = (*shipSpriteInfoHand)->lightImageID;
        if (lightID > 0 && loadShipLightsPref != 0) {
            maskID = (*shipSpriteInfoHand)->lightMaskID;
            xSize = (*shipSpriteInfoHand)->lightXSize;
            ySize = (*shipSpriteInfoHand)->lightYSize;

            i = FindSlotEntry(lightID, shipType, 3);
            if (i != -1) {
                SWCloneSprite(masterShipLightSprite[i], &masterShipLightSprite[shipType], NULL);
                SWLockSprite(masterShipLightSprite[shipType]);
            } else {
                rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
                if (ResourceExists(rleType, lightID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipLightSprite[shipType], NULL,
                                                        lightID, count, 0);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship light sprite from RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, resID, err);
                    }
                    SWLockSprite(masterShipLightSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipLightSprite[shipType], NULL,
                                                         lightID, maskID, xSize, ySize, 0, 0,
                                                         gShipTypes[shipType].framesPer * setCount, 1);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship light sprite from PICT, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, resID, err);
                    }
                    masterShipLightSprite[shipType]->numFrames =
                        gShipTypes[shipType].framesPer * setCount;
                    SWLockSprite(masterShipLightSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipLightSprite[shipType]);
                    if ((long)err != 0) {
                        crashlog_print("error converting ship light sprite to RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, resID, err);
                    }
                }
            }
            gShipTypes[shipType].blinkMode = (*shipSpriteInfoHand)->blinkMode;
            for (i = 0; i < 4; i++)
                gShipTypes[shipType].blinkValue[i] = (*shipSpriteInfoHand)->blinkValue[i];
            if (gShipTypes[shipType].blinkMode == 2 &&
                gShipTypes[shipType].blinkValue[2] > 31)
                gShipTypes[shipType].blinkValue[2] = 31;
            if (gShipTypes[shipType].blinkMode == 3 &&
                gShipTypes[shipType].blinkValue[1] > 31)
                gShipTypes[shipType].blinkValue[1] = 31;
            shipSpriteLoaded[shipType][3] = lightID;
            total += masterShipLightSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->weapImageID > 0 && loadShipWeaponsPref != 0) {
            spinID = (*shipSpriteInfoHand)->weapImageID;
            maskID = (*shipSpriteInfoHand)->weapMaskID;
            xSize = (*shipSpriteInfoHand)->weapXSize;
            ySize = (*shipSpriteInfoHand)->weapYSize;

            i = FindSlotEntry(spinID, shipType, 4);
            if (i != -1) {
                SWCloneSprite(masterShipWeaponSprite[i],
                              &masterShipWeaponSprite[shipType], NULL);
                SWLockSprite(masterShipWeaponSprite[shipType]);
            } else {
                rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
                if (ResourceExists(rleType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipWeaponSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship weapon sprite from RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, resID, err);
                    }
                    SWLockSprite(masterShipWeaponSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipWeaponSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0,
                                                         gShipTypes[shipType].framesPer * setCount, 1);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship weapon sprite from PICT, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, resID, err);
                    }
                    masterShipWeaponSprite[shipType]->numFrames =
                        gShipTypes[shipType].framesPer * setCount;
                    SWLockSprite(masterShipWeaponSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipWeaponSprite[shipType]);
                    if ((long)err != 0) {
                        crashlog_print("error converting ship weapon sprite to RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, resID, err);
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

            i = FindSlotEntry(spinID, shipType, 5);
            if (i != -1) {
                SWCloneSprite(masterShipShieldSprite[i],
                              &masterShipShieldSprite[shipType], NULL);
                SWLockSprite(masterShipShieldSprite[shipType]);
            } else {
                rleType = (gSpriteWorld->pixelDepth == 16) ? kRLEDResType : kRLE8ResType;
                if (ResourceExists(rleType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipShieldSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship shield sprite from RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(2, resID, err);
                    }
                    SWLockSprite(masterShipShieldSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipShieldSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0, 0, 1);
                    if ((long)err != 0) {
                        crashlog_print("error loading ship shield sprite from PICT, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(3, resID, err);
                    }
                    SWLockSprite(masterShipShieldSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipShieldSprite[shipType]);
                    if ((long)err != 0) {
                        crashlog_print("error converting ship shield sprite to RLE, sh\x8an ID ");
                        crashlog_print_num(resID);
                        crashlog_print("\n");
                        ReportSpriteLoadError(4, resID, err);
                    }
                }
            }
            shipSpriteLoaded[shipType][5] = spinID;
            total += masterShipShieldSprite[shipType]->numFrames;
        }

        gShipTypes[shipType].weapPos[0][0][0] = (*shipSpriteInfoHand)->mountPos[0].x[0];
        gShipTypes[shipType].weapPos[0][0][1] = (*shipSpriteInfoHand)->mountPos[0].y[0];
        gShipTypes[shipType].weapPos[0][1][0] = (*shipSpriteInfoHand)->mountPos[0].x[1];
        gShipTypes[shipType].weapPos[0][1][1] = (*shipSpriteInfoHand)->mountPos[0].y[1];
        gShipTypes[shipType].weapPos[0][2][0] = (*shipSpriteInfoHand)->mountPos[0].x[2];
        gShipTypes[shipType].weapPos[0][2][1] = (*shipSpriteInfoHand)->mountPos[0].y[2];
        gShipTypes[shipType].weapPos[0][3][0] = (*shipSpriteInfoHand)->mountPos[0].x[3];
        gShipTypes[shipType].weapPos[0][3][1] = (*shipSpriteInfoHand)->mountPos[0].y[3];
        gShipTypes[shipType].weapPos[1][0][0] = (*shipSpriteInfoHand)->mountPos[1].x[0];
        gShipTypes[shipType].weapPos[1][0][1] = (*shipSpriteInfoHand)->mountPos[1].y[0];
        gShipTypes[shipType].weapPos[1][1][0] = (*shipSpriteInfoHand)->mountPos[1].x[1];
        gShipTypes[shipType].weapPos[1][1][1] = (*shipSpriteInfoHand)->mountPos[1].y[1];
        gShipTypes[shipType].weapPos[1][2][0] = (*shipSpriteInfoHand)->mountPos[1].x[2];
        gShipTypes[shipType].weapPos[1][2][1] = (*shipSpriteInfoHand)->mountPos[1].y[2];
        gShipTypes[shipType].weapPos[1][3][0] = (*shipSpriteInfoHand)->mountPos[1].x[3];
        gShipTypes[shipType].weapPos[1][3][1] = (*shipSpriteInfoHand)->mountPos[1].y[3];
        gShipTypes[shipType].weapPos[2][0][0] = (*shipSpriteInfoHand)->mountPos[2].x[0];
        gShipTypes[shipType].weapPos[2][0][1] = (*shipSpriteInfoHand)->mountPos[2].y[0];
        gShipTypes[shipType].weapPos[2][1][0] = (*shipSpriteInfoHand)->mountPos[2].x[1];
        gShipTypes[shipType].weapPos[2][1][1] = (*shipSpriteInfoHand)->mountPos[2].y[1];
        gShipTypes[shipType].weapPos[2][2][0] = (*shipSpriteInfoHand)->mountPos[2].x[2];
        gShipTypes[shipType].weapPos[2][2][1] = (*shipSpriteInfoHand)->mountPos[2].y[2];
        gShipTypes[shipType].weapPos[2][3][0] = (*shipSpriteInfoHand)->mountPos[2].x[3];
        gShipTypes[shipType].weapPos[2][3][1] = (*shipSpriteInfoHand)->mountPos[2].y[3];
        gShipTypes[shipType].weapPos[3][0][0] = (*shipSpriteInfoHand)->mountPos[3].x[0];
        gShipTypes[shipType].weapPos[3][0][1] = (*shipSpriteInfoHand)->mountPos[3].y[0];
        gShipTypes[shipType].weapPos[3][1][0] = (*shipSpriteInfoHand)->mountPos[3].x[1];
        gShipTypes[shipType].weapPos[3][1][1] = (*shipSpriteInfoHand)->mountPos[3].y[1];
        gShipTypes[shipType].weapPos[3][2][0] = (*shipSpriteInfoHand)->mountPos[3].x[2];
        gShipTypes[shipType].weapPos[3][2][1] = (*shipSpriteInfoHand)->mountPos[3].y[2];
        gShipTypes[shipType].weapPos[3][3][0] = (*shipSpriteInfoHand)->mountPos[3].x[3];
        gShipTypes[shipType].weapPos[3][3][1] = (*shipSpriteInfoHand)->mountPos[3].y[3];

        gShipTypes[shipType].weapPosZ[0][0] = (*shipSpriteInfoHand)->mountPosZ[0][0];
        gShipTypes[shipType].weapPosZ[0][1] = (*shipSpriteInfoHand)->mountPosZ[0][1];
        gShipTypes[shipType].weapPosZ[0][2] = (*shipSpriteInfoHand)->mountPosZ[0][2];
        gShipTypes[shipType].weapPosZ[0][3] = (*shipSpriteInfoHand)->mountPosZ[0][3];
        gShipTypes[shipType].weapPosZ[1][0] = (*shipSpriteInfoHand)->mountPosZ[1][0];
        gShipTypes[shipType].weapPosZ[1][1] = (*shipSpriteInfoHand)->mountPosZ[1][1];
        gShipTypes[shipType].weapPosZ[1][2] = (*shipSpriteInfoHand)->mountPosZ[1][2];
        gShipTypes[shipType].weapPosZ[1][3] = (*shipSpriteInfoHand)->mountPosZ[1][3];
        gShipTypes[shipType].weapPosZ[2][0] = (*shipSpriteInfoHand)->mountPosZ[2][0];
        gShipTypes[shipType].weapPosZ[2][1] = (*shipSpriteInfoHand)->mountPosZ[2][1];
        gShipTypes[shipType].weapPosZ[2][2] = (*shipSpriteInfoHand)->mountPosZ[2][2];
        gShipTypes[shipType].weapPosZ[2][3] = (*shipSpriteInfoHand)->mountPosZ[2][3];
        gShipTypes[shipType].weapPosZ[3][0] = (*shipSpriteInfoHand)->mountPosZ[3][0];
        gShipTypes[shipType].weapPosZ[3][1] = (*shipSpriteInfoHand)->mountPosZ[3][1];
        gShipTypes[shipType].weapPosZ[3][2] = (*shipSpriteInfoHand)->mountPosZ[3][2];
        gShipTypes[shipType].weapPosZ[3][3] = (*shipSpriteInfoHand)->mountPosZ[3][3];

        gShipTypes[shipType].compress[0][0] = (*shipSpriteInfoHand)->upCompressX * 0.01;
        gShipTypes[shipType].compress[0][1] = (*shipSpriteInfoHand)->upCompressY * 0.01;
        gShipTypes[shipType].compress[1][0] = (*shipSpriteInfoHand)->downCompressX * 0.01;
        gShipTypes[shipType].compress[1][1] = (*shipSpriteInfoHand)->downCompressY * 0.01;

        for (i = 0; i <= 1; i++) {
            if (gShipTypes[shipType].compress[i][0] <= 0.0f)
                gShipTypes[shipType].compress[i][0] = 1.0f;
            if (gShipTypes[shipType].compress[i][1] <= 0.0f)
                gShipTypes[shipType].compress[i][1] = 1.0f;
        }

        AdjustHandleSize((Handle)shipSpriteInfoHand, 0xc0);
        UnlockAHandle((Handle)shipSpriteInfoHand);
        ReleaseResource((Handle)shipSpriteInfoHand);
    }
    return total;
}

extern "C" {

/* ReportSpriteLoadError. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6620
void ReportSpriteLoadError(short label, short value, short special)
{
    unsigned char labelText[256];
    unsigned char valueText[256];
    unsigned char specialText[256];
    long kind;
    extern void NumToString(int value, unsigned char *str);  /* 0x004bbce0 */

    kind = special;
    if (kind == 0)
        return;
    GetIndString(labelText, 0x8c, label);
    NumToString(value, valueText);
    if (special == -0xc0)
        GetIndString(specialText, 0x8d, 1);
    else if (special == -0x27)
        GetIndString(specialText, 0x8d, 2);
    else if (kind == 0x6b)
        GetIndString(specialText, 0x8d, 3);
    else if (kind == -0x6c)
        GetIndString(specialText, 0x8d, 4);
    else
        NumToString(special, specialText);
    ParamText(labelText, valueText, specialText, "\p");
    Alert(0x83, 0);
}

}

/* ShowIndAlert. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b6760
void ShowIndAlert(short index)
{
    unsigned char text[256];

    GetIndString(text, 140, index);
    ParamText(text, "\p", "\p", "\p");
    Alert(128, NULL);
    ExitToShell();
}

/* FindSlotEntry. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b67c0
short FindSlotEntry(short value, short count, short field)
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
void DisposeClonedSprite(void *p)
{
    SpritePtr q;

    if (p == NULL)
        return;
    q = *(SpritePtr *)p;
    if (q == NULL)
        return;
    if (q->isEmptySprite != 0) {
        q->frameArray = NULL;
        q->numFrames = 0;
    }
    SWDisposeSprite((SpritePtr *)p);
}

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
    long h;
    long v;

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
char Button(void)
{
    KeyboardKbhit();
    return MouseReadButtons();
}

/* StillDown. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004b68f0
char StillDown(void)
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
    if (CurrentGrafPort == NULL)
        return;
    CurrentGrafPort->txSize = size;
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
unsigned long Color2Index(RGBColor *color)
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
