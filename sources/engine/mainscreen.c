// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* mainscreen.c -- the main screen: DoMainScreen, its choice and keyboard handlers (win 0x00486880..0x004872a0). Notes: Research/notes/mainscreen.md */
/*
 * The checks are a straight run of independent ifs rather than a chain, so each
 * handled key falls through to the remaining comparisons.
 */
#include <stddef.h>

#include "prototypes.h"

#define kMainScreenNew      0
#define kMainScreenOpen     1
#define kMainScreenQuit     2
#define kMainScreenEnter    3
#define kMainScreenPrefs    4
#define kMainScreenAbout    5

#define kKeyS               0x1f
#define kKeyL               0x26
#define kCreditsTextID      0x7ffe

#include "resources.h"
#include "shipobj.h"
#include "gametypes.h"
#include "gameconst.h"
#include "macport.h"
#include "dialogs.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "burger.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "crashlog.h"
#include "game.h"

#define kStrListMainScreen   0x8c
#define kStrNewPilotConfirm     8
#define kStrMissingResource  0x34

/* ---- DoMainScreen (0x00486880) ---- */

unsigned long LastTick;                   /* 0x008700f0 */

/* 0x0058f8ac, 0x0058f8b0 and 0x0058f710 are IAT slots, not variables:
   dllimport is what makes the call go through them. */

#define kIdleWaitTicks       30
#define kStrQuitConfirm     0xa
#define kStrListVersion    0x82
#define kStrVersionURL        5
#define kStartupIterations 0x32
#define kBackRectOffset    0x34

/* DoMainScreen (mac _DoMainScreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486880
void DoMainScreen(void)
{
    long mouseV;
    Str255 confirm;
    long mouseH;
    Str255 versionURL;
    long mode[4];
    Point where;
    long err;
    short delay;
    int key;
    int upper;
    int i;
    int buttons;
    char animCut;

    crashlog("preparing main screen");
    mainScreenBackgroundDirty = 0;
    showingIntroText = 0;
    gDlgWorld.world = NULL;
    gMainPicFrame = 0;
    if (menuRolloverSprite != NULL)
        gHilitedButton = menuRolloverSprite->numFrames - 1;
    else
        gHilitedButton = 0;
    gHiliteLevel = 0;
    gDlgWorlds[0].world = NULL;
    gAnimStep[0] = 0;
    gDlgWorlds[1].world = NULL;
    gAnimStep[1] = 0;
    gDlgWorlds[2].world = NULL;
    gAnimStep[2] = 0;
    mainScreenSnd[0] = NULL;
    mainScreenSnd[1] = NULL;
    mainScreenSnd[2] = NULL;
    mainScreenSnd[3] = NULL;
    crashlog("setting up world");

    if (gameInProgress) {
        DeleteAllComputerShips(0, 1);
        PropagateMissionBitEffects();
        SetupShipsInSystem(shipData->curSystem, 1);
        CreateAsteroids();
        ExploreNebulaeFromSystem(shipData->curSystem);
        HandleStellarSystemVisibility();
        MissionPostLoadValidCheck();
        onPlanet = 0;
        killShots = killBoxes = killExplods = killSmoke = killAsteroids = 1;
        missionSoundID = -1;
        SWSetPortToBackground(gSpriteWorld);
        SetSystemBackgroundColor();
        PaintRect((Rect *)((char *)gSpriteWorld + kBackRectOffset));
        RGBForeColor(blackColor);
        SWSetPortToWorkArea(gSpriteWorld);
        SetSystemBackgroundColor();
        PaintRect((Rect *)((char *)gSpriteWorld + kBackRectOffset));
        RGBForeColor(blackColor);
        SWSetPortToWindow(gSpriteWorld);
    }

    if (noVersionCheckPref == 0) {
        crashlog("checking network stack");
        if (NetworkStackGetActive(0)) {
            crashlog("beginning version check");
            GetIndString(versionURL, kStrListVersion, kStrVersionURL);
            PtoCstr(versionURL);
            err = ReggieVersionBegin(0, (char *)versionURL);
            if (err != 0) {
                crashlog_print("couldn't begin version check (error code ");
                crashlog_print_num(err);
                crashlog_print(")\n");
            } else {
                gVersionCheckStart = TimerGetSeconds();
            }
        } else {
            crashlog("network stack not active");
        }
    }

    crashlog("setting up main screen graphics");
    SetupInterfaceBackground();
    ShowTheCursor();
    crashlog("doing main screen");
    LoadMainScreenAnim();
    MainScreenAnimDraw();
    MainScreenIdle();
    LoadMainScreenSounds();
    SWSetPortToWindow(gSpriteWorld);
    GetCWindowPortRect(gWind);
    gNeedUpdateFlag = 1;
    regNoticeShown = 1;

    if (RT3_IsRegistered() == 0) {
        VideoGetCurrentMode(mode);
        if ((mode[3] & kModeFlagOwned) != 0) {
            ReleaseScreenDC();
            SetDisplayToSize(mode[0], mode[1], mode[2], mode[3] & ~kModeFlagOwned);
            AcquireScreenDC();
            MainScreenUpdate(1);
            ScreenEndScene();
        }
        _PlatformSetWindow(Win95MainWindow);
        RT3_RunReminder(0, 0);
        SetForegroundWindow(Win95MainWindow);
        SetFocus(Win95MainWindow);
        while (Win95ApplsActive == 0) {
            Sleep(0);
            KeyboardKbhit();
        }
        if ((mode[3] & kModeFlagOwned) != 0) {
            ReleaseScreenDC();
            VideoSetMode(mode);
            AcquireScreenDC();
        }
        MainScreenUpdate(1);
    }

    GetCWindowPortRect(gWind);
    gNeedUpdateFlag = 1;
    AdjustSoundVolume();
    ShowTheCursor();
    RT3_Idle();
    IterateInGameEventLoop(kStartupIterations);
    gUpdateFlag = 0;

    while (gQuit == 0) {
        if (gUpdateFlag != 0)
            MainScreenUpdate(1);

        delay = (gAnimStep[0] < kAnimDone || gAnimStep[1] < kAnimDone
                 || gAnimStep[2] < kAnimDone)
                ? 0
                : ((gameLoaded != 0 && inBackground != 0)
                   ? kIdleWaitTicks : 0);
        if (delay != 0) {
            LastTick = ReadTick();
            WaitTicks(delay);
        }

        key = KeyboardGet();
        upper = toupper(key);
        buttons = MouseReadButtons();
        MainScreenIdlePoll();
        if (buttons != 0) {
            animCut = 0;
            MouseClicked = 0;
            i = 0;
            do {
                if (gAnimStep[(short)i] < kAnimDone) {
                    gAnimStep[(short)i] = kAnimDone;
                    animCut = 1;
                }
                i = i + 1;
            } while ((short)i < 3);
            SWSetPortToWindow(gSpriteWorld);
            MouseReadPosition(&mouseH, &mouseV);
            if (animCut) {
                SetupInterfaceBackground();
                MainScreenAnimDraw();
                MainScreenUpdate(1);
            } else {
                MainScreenAnimDraw();
                MainScreenUpdate(0);
                ScreenEndScene();
            }
            where.h = mouseH;
            where.v = mouseV;
            MainScreenChoice(TrackMainScreenButtons(where));
        } else if (key != 0) {
            MainScreenKey(upper, 0, 0);
        }

        if (gNeedUpdateFlag != 0) {
            MainScreenAnimDraw();
            MainScreenUpdate(1);
            gNeedUpdateFlag = 0;
        }
        if (closeRequested != 0) {
            closeRequested = 0;
            if (screenFaded == 0) {
                ShowTheCursor();
                GetIndString(confirm, kStrListMainScreen, kStrQuitConfirm);
                if (EVYesNoDialog(confirm))
                    gQuit = 1;
            }
        }
    }

    DisposeDialogGWorlds();
    StopDialogSounds();
}

/* FUN_00486ec0. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486ec0
void FUN_00486ec0(void)
{
}

/* MainScreenChoice (mac _MainScreenChoice). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486ed0
void MainScreenChoice(short which)
{
    char confirmed;
    char missingResource;
    unsigned char msg[256];
    short i;
    short j;

    if (which == kMainScreenNew) {
        if (gameInProgress) {
            GetIndString(msg, kStrListMainScreen, kStrNewPilotConfirm);
            confirmed = EVYesNoDialog(msg);
            MainScreenAnimDraw();
            MainScreenUpdate(1);
        } else {
            confirmed = 1;
        }
        if (confirmed) {
            DoNewPilot();
            SWSetPortToWindow(gSpriteWorld);
            GetCWindowPortRect(gWind);
            gNeedUpdateFlag = 1;
            SWHideAllSprites(gSpriteWorld);
        }
    }

    if (which == kMainScreenOpen) {
        crashlog("opening pilot file");
        if (OpenPilotFile(&missingResource)) {
            if (missingResource) {
                crashlog("showing missing resource warning");
                MainScreenAnimDraw();
                MainScreenUpdate(1);
                GetIndString(msg, kStrListMainScreen, kStrMissingResource);
                EVNoteAlert(msg);
            }
            crashlog("preparing universe");
            gameInProgress = 1;
            GetCWindowPortRect(gWind);
            gNeedUpdateFlag = 1;
            DeleteAllComputerShips(0, 1);
            PropagateMissionBitEffects();
            SetupShipsInSystem(shipData->curSystem, 1);
            HandleStellarSystemVisibility();
            CreateAsteroids();

            for (i = 0; i < 0x800; i++) {
                if (gSystems[i].loaded != 0 && gSystems[i].exploreLevel > 0)
                    ExploreNebulaeFromSystem(i);
            }

            crashlog("preparing missions");
            for (j = 0; j < 0x10; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissions[j].flags & kMissionFlagRearmShips) != 0)
                        currentMissions[j].auxShipsLeft = currentMissions[j].auxShipCount;
                    if (currentMissions[j].auxShipsLeft > 0) {
                        currentMissions[j].auxShipDelay = (Rand(0x46) + 0x46);
                        currentMissions[j].auxShipsSpawned = 0;
                    }
                }
            }

            onPlanet = 0;
            killShots = killBoxes = killExplods = killSmoke = killAsteroids = 1;
            missionSoundID = -1;
            SWHideAllSprites(gSpriteWorld);
            crashlog("finished opening pilot file");
        }
    }

    if (which == kMainScreenQuit) {
        Delay(0xf, &tix);
        gQuit = 1;
    }

    if (which == kMainScreenEnter) {
        if (gameInProgress) {
            if (!IsDying(shipData)) {
                EnterGameFromMainScreen();
            } else {
                PlaySound(beepSnd[3], 0xa, gSoundVolume, gSoundVolume);
                MainScreenAnimDraw();
                MainScreenUpdate(1);
            }
        } else {
            PlaySound(beepSnd[3], 0xa, gSoundVolume, gSoundVolume);
            MainScreenAnimDraw();
            MainScreenUpdate(1);
        }
    }

    if (which == kMainScreenPrefs) {
        DoPrefsDialog();
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        SWSetPortToWindow(gSpriteWorld);
        GetCWindowPortRect(gWind);
        gNeedUpdateFlag = 1;
        AdjustSoundVolume();
    }

    if (which == kMainScreenAbout || which == 6) {
        DoAboutBox();
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        SWSetPortToWindow(gSpriteWorld);
        GetCWindowPortRect(gWind);
        gNeedUpdateFlag = 1;
    }

    if (!gQuit) {
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    RT3_Idle();
}

/* MainScreenKey (mac _MainScreenKey). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004872a0
void MainScreenKey(char key, long a, long b)
{
    key = tolower(key);
    if (key == 'n') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenNew);
    }
    if (key == 'o') {
        if (KeyCheck(kKeyS) == 0 && KeyCheck(kKeyL) == 0) {
            PlayMainScreenClickSound();
            MainScreenChoice(kMainScreenOpen);
        }
    }
    if (key == 'q') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenQuit);
    }
    if (key == 'e') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenEnter);
    }
    if (key == 'p') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenPrefs);
    }
    if (key == 'a') {
        PlayMainScreenClickSound();
        MainScreenChoice(kMainScreenAbout);
    }
    if (key == 'x') {
        PlayMainScreenClickSound();
        mainScreenBackgroundDirty = 1;
        LoadDescription(briefingTextPtr, kCreditsTextID);
        MungeBriefing(0, -1);
        strcpy(gTextShowString, briefingTextPtr);
        ShowTextDialog(0, 0, 1);
        mainScreenBackgroundDirty = 1;
    }
}
