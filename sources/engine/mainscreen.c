// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
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

#ifdef MACBUILD
#define kKeyS               MacKey(0x01)
#define kKeyL               MacKey(0x25)
#define kKeyE               MacKey(0x0e)
#else
#define kKeyS               0x1f
#define kKeyL               0x26
#endif
#define kCreditsTextID      0x7ffe

#define kAppleMenu         1000
#define kFileMenu           128
#define kEditMenu           129
#define kAboutItem            1
#define kQuitItem             1
#define kPrefsItem            7

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
#ifdef MACBUILD
#include "../mac/include/ASWRegistrationCarbonAPI.h"
#endif

#define kStrListMainScreen   0x8c
#define kStrNewPilotConfirm     8
#define kStrMissingResource  0x34

/* ---- DoMainScreen (0x00486880) ---- */

#ifndef MACBUILD
unsigned long LastTick;                   /* 0x008700f0 */
#endif

/* 0x0058f8ac, 0x0058f8b0 and 0x0058f710 are IAT slots, not variables:
   dllimport is what makes the call go through them. */

#define kIdleWaitTicks       30
#define kStrQuitConfirm     0xa
#define kStrListVersion    0x82
#define kStrVersionURL        5
#define kStartupIterations 0x32

/* DoMainScreen (mac _DoMainScreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486880
// MAC: _DoMainScreen i386 0x0001af02 ppc 0x0001ee70
#ifdef MACBUILD
void DoMainScreen(void)
{
    Str255 versionURL;
    long err;
#if TARGET_API_MAC_OS8
    Point diskPt;
    Boolean gotEvent;
#endif
    unsigned char ch;
    long menuChoice;
    short i;
    short sleep;

    crashlog("preparing main screen");
    mainScreenBackgroundDirty = 0;
    showingIntroText = 0;
    logoPort.world = NULL;
    logoIndex = 0;
    if (menuRolloverSprite != NULL)
        rolloverIndex = menuRolloverSprite->numFrames - 1;
    else
        rolloverIndex = 0;
    rolloverAlpha = 0;
    for (i = 0; i < 3; i++) {
        slidePort[i].world = NULL;
        slideIndex[i] = 0;
    }
    for (i = 0; i < 4; i++)
        mainScreenSnd[i] = NULL;
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
        PaintRect(&gSpriteWorld->windRect);
        ForeColor(blackColor);
        SWSetPortToWorkArea(gSpriteWorld);
        SetSystemBackgroundColor();
        PaintRect(&gSpriteWorld->windRect);
        ForeColor(blackColor);
        SWSetPortToWindow(gSpriteWorld);
    }

#ifdef MAC1010
    if (settings.noVersionCheck == 0) {
        crashlog("checking network stack");
        if (NetworkStackGetActive(0)) {
            crashlog("beginning version check");
            GetIndString(versionURL, kStrListVersion, kStrVersionURL);
            PtoCstr(versionURL);
            err = ReggieVersionBegin(0, (char *)versionURL);
            if (err == 0) {
                reggieStarted = TimerGetSeconds();
            } else {
                crashlog_print("couldn't begin version check (error code ");
                crashlog_print_num(err);
                crashlog_print(")\n");
            }
        } else {
            crashlog("network stack not active");
        }
    }

#endif
    crashlog("setting up main screen graphics");
    SetupInterfaceBackground();
    ShowTheCursor();
    crashlog("doing main screen");
    LoadMainScreenAnim();
    MainScreenAnimDraw();
    MainScreenAppearStart();
    MainScreenAppear();
    SWSetPortToWindow(gSpriteWorld);
    InvalWindowPortRect((WindowRef)gWind);
#ifndef MAC1010
    FadeScreenIn();
#endif
    if (!inBackground) {
        regNoticeShown = 1;
#ifdef MAC1010
        if (!RT3_IsRegistered()) {
            RT3_RunReminder(0, 0);
            MainScreenUpdate(1);
        }
#else
        if (!RT3_IsRegistered())
            MainScreenUpdate(1);
#endif
    } else {
        regNoticeShown = 0;
    }

    InvalWindowPortRect((WindowRef)gWind);
    AdjustSoundVolume();
    ShowTheCursor();
    RT3_Idle();
    IterateInGameEventLoop(kStartupIterations);
    gUpdateFlag = 0;

    while (!gQuit) {
        if (gUpdateFlag)
            MainScreenUpdate(1);

        if (slideIndex[0] < kAnimDone || slideIndex[1] < kAnimDone
            || slideIndex[2] < kAnimDone)
            sleep = 0;
        else
            sleep = (gameLoaded && inBackground) ? kIdleWaitTicks : 0;
#if TARGET_API_MAC_OS8
        if (slideIndex[2] < kAnimDone) {
            gotEvent = 0;
            gEvent.what = nullEvent;
        } else {
            gotEvent = WaitNextEvent(everyEvent, &gEvent, sleep, NULL);
        }
        if (!gotEvent && gEvent.what == nullEvent) {
#else
        if (!WaitNextEvent(everyEvent, &gEvent, sleep, NULL) && gEvent.what == nullEvent) {
#endif
            MainScreenIdle();
            continue;
        }
        switch (gEvent.what) {
        case kHighLevelEvent:
            AEProcessAppleEvent(&gEvent);
            break;
        case mouseDown:
            MainScreenMouse(&gEvent);
            break;
        case keyDown:
        case autoKey:
            if (gEvent.modifiers & cmdKey) {
                AdjustMenus();
                menuChoice = MenuKey(gEvent.message);
                if (HiWord(menuChoice) != 0) {
                    if (HiWord(menuChoice) == kFileMenu && LoWord(menuChoice) == kQuitItem)
                        gQuit = 1;
                } else {
                    ch = gEvent.message & charCodeMask;
                    MainScreenKey(ch, ch, gEvent.modifiers);
                }
            } else {
                ch = gEvent.message & charCodeMask;
                MainScreenKey(ch, ch, gEvent.modifiers);
            }
            break;
        case updateEvt:
            SWSetPortToWindow(gSpriteWorld);
            BeginUpdate((WindowRef)gWind);
            MainScreenAnimDraw();
            MainScreenUpdate(1);
            EndUpdate((WindowRef)gWind);
            break;
#if TARGET_API_MAC_OS8
        case diskEvt:
            if ((long)gEvent.message >> 16) {
                diskPt.v = diskPt.h = 100;
                DILoad();
                DIBadMount(diskPt, gEvent.message);
                DIUnload();
            }
            break;
#endif
        case osEvt:
            if (gEvent.message & (suspendResumeMessage << 24)) {
                if (gEvent.message & resumeFlag) {
                    MainScreenActivate(true);
                    InvalWindowPortRect((WindowRef)gWind);
                    SwitchedIn(&gEvent);
                } else {
                    MainScreenActivate(false);
                    InitCursor();
                    cursorHidden = 0;
                    ShowTheCursor();
                    SwitchedOut(&gEvent);
                }
            }
            break;
        }
    }

    DisposeMainScreenAnim();
    DisposeMainScreenSounds();
}
#else
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
    Boolean animCut;

    crashlog("preparing main screen");
    mainScreenBackgroundDirty = 0;
    showingIntroText = 0;
    logoPort.world = NULL;
    logoIndex = 0;
    if (menuRolloverSprite != NULL)
        rolloverIndex = menuRolloverSprite->numFrames - 1;
    else
        rolloverIndex = 0;
    rolloverAlpha = 0;
    slidePort[0].world = NULL;
    slideIndex[0] = 0;
    slidePort[1].world = NULL;
    slideIndex[1] = 0;
    slidePort[2].world = NULL;
    slideIndex[2] = 0;
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
        PaintRect(&gSpriteWorld->windRect);
        ForeColor(blackColor);
        SWSetPortToWorkArea(gSpriteWorld);
        SetSystemBackgroundColor();
        PaintRect(&gSpriteWorld->windRect);
        ForeColor(blackColor);
        SWSetPortToWindow(gSpriteWorld);
    }

    if (settings.noVersionCheck == 0) {
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
                reggieStarted = TimerGetSeconds();
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
    MainScreenAppearStart();
    MainScreenAppear();
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

        delay = (slideIndex[0] < kAnimDone || slideIndex[1] < kAnimDone
                 || slideIndex[2] < kAnimDone)
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
        MainScreenIdle();
        if (buttons != 0) {
            animCut = 0;
            MouseClicked = 0;
            i = 0;
            do {
                if (slideIndex[(short)i] < kAnimDone) {
                    slideIndex[(short)i] = kAnimDone;
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

    DisposeMainScreenAnim();
    DisposeMainScreenSounds();
}
#endif

#ifdef MAC1010
/* the mac body of win's empty FUN_00486ec0 */
#define kMainMenuBar 128
#define kAquaMenuBar 129
#define kAppleMenu 1000
void FUN_00486ec0(void)
{
    Handle bar;
    MenuHandle menu;
#if TARGET_API_MAC_CARBON
    long response;

    Gestalt(gestaltMenuMgrAttr, &response);
    if (response & gestaltMenuMgrAquaLayoutMask)
        bar = GetNewMBar(kAquaMenuBar);
    else
        bar = GetNewMBar(kMainMenuBar);
#else

    bar = GetNewMBar(kMainMenuBar);
#endif
    if (bar != NULL) {
        SetMenuBar(bar);
        menu = GetMenuHandle(kAppleMenu);
        if (menu != NULL)
            AppendResMenu(menu, 'DRVR');
    }
    DrawMenuBar();
}
#elif !defined(MACBUILD)
/* FUN_00486ec0. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486ec0
void FUN_00486ec0(void)
{
}
#endif

/* MainScreenChoice (mac _MainScreenChoice). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486ed0
// MAC: _MainScreenChoice i386 0x0001a72a ppc 0x0001e614
void MainScreenChoice(short which)
{
    Boolean confirmed;
    Boolean missingResource;
    unsigned char msg[256];
    short i;
    short j;
#if defined(MACBUILD) && V111
    AppleEvent quitEvent;
    OSErr err;
#endif

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
            InvalWindowPortRect((WindowRef)gWind);
            SWHideAllSprites(gSpriteWorld);
        }
    }

    if (which == kMainScreenOpen) {
        crashlog("opening pilot file");
        if (OpenPilotFile((char *)&missingResource)) {
            if (missingResource) {
                crashlog("showing missing resource warning");
                MainScreenAnimDraw();
                MainScreenUpdate(1);
                GetIndString(msg, kStrListMainScreen, kStrMissingResource);
                EVNoteAlert(msg);
            }
            crashlog("preparing universe");
            gameInProgress = 1;
            InvalWindowPortRect((WindowRef)gWind);
            DeleteAllComputerShips(0, 1);
            PropagateMissionBitEffects();
            SetupShipsInSystem(shipData->curSystem, 1);
            HandleStellarSystemVisibility();
            CreateAsteroids();

            for (i = 0; i < 0x800; i++) {
                if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0)
                    ExploreNebulaeFromSystem(i);
            }

            crashlog("preparing missions");
            for (j = 0; j < 0x10; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissions[j].flags & kMissionInfiniteAuxShips) != 0)
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
#if defined(MACBUILD) && V111
        err = MoreAECreateAppleEventSelfTarget(kCoreEventClass, kAEQuitApplication, &quitEvent);
        if (err != noErr)
            ExitToShell();
        AESendMessage(&quitEvent, NULL, kAENoReply, kNoTimeOut);
#endif
    }

    if (which == kMainScreenEnter) {
        if (gameInProgress) {
            if (!IsDying(shipData)) {
#if V111
#ifdef MACBUILD
                FlushKeys();
#endif
#endif
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
        InvalWindowPortRect((WindowRef)gWind);
        AdjustSoundVolume();
    }

    if (which == kMainScreenAbout || which == 6) {
        DoAboutBox();
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        SWSetPortToWindow(gSpriteWorld);
        InvalWindowPortRect((WindowRef)gWind);
    }

    if (!gQuit) {
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    RT3_Idle();
}

#ifdef MACBUILD
/* MainScreenMouse (mac _MainScreenMouse): mac only. */
// MAC: _MainScreenMouse i386 0x0001aced ppc 0x0001ec14
void MainScreenMouse(EventRecord *event)
{
    WindowPtr window;
    Point where;
    Rect dragBounds;
    BitMap screenBits;
    Str255 prompt;
    short i;
    Boolean animCut;

    where = event->where;
    switch (FindWindow(where, &window)) {
    case inDesk:
        SysBeep(1);
        break;
    case inDrag:
        if (window == (WindowPtr)gWind) {
            if (useLegacyRenderingPipeline) {
                GetWindowDragBounds(&dragBounds, event->where);
                if (dragBounds.right <= dragBounds.left || dragBounds.bottom <= dragBounds.top)
                    break;
                DragWindow(window, event->where, &dragBounds);
            } else {
                DragWindow(window, event->where, NULL);
            }
            ForceWindowOnscreen();
            SWWindowMoved(gSpriteWorld);
        } else {
            if (useLegacyRenderingPipeline) {
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
                DragWindow(window, event->where, &qd.screenBits.bounds);
#else
                GetQDGlobalsScreenBits(&screenBits);
                DragWindow(window, event->where, &screenBits.bounds);
#endif
            } else {
                DragWindow(window, event->where, NULL);
            }
            ForceWindowOnscreen();
        }
        break;
    case inMenuBar:
        InitCursor();
        cursorHidden = 0;
        ShowTheCursor();
        AdjustMenus();
        DoMenuClick(MenuSelect(event->where));
        break;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    case inSysWindow:
        SystemClick(event, window);
        break;
#endif
    case inContent:
        animCut = 0;
        for (i = 0; i < 3; i++) {
            if (slideIndex[i] < kAnimDone) {
                slideIndex[i] = kAnimDone;
                animCut = 1;
            }
        }
        SWSetPortToWindow(gSpriteWorld);
        GlobalToLocal(&where);
        if (animCut) {
            SetupInterfaceBackground();
            MainScreenAnimDraw();
            MainScreenUpdate(1);
        } else {
            MainScreenAnimDraw();
            MainScreenUpdate(0);
        }
        MainScreenChoice(TrackMainScreenButtons(where));
        break;
    case inGoAway:
        if ((FrontWindow)() == (WindowRef)gWind) {
            ShowTheCursor();
            GetIndString(prompt, kStrListMainScreen, kStrQuitConfirm);
            if (EVYesNoDialog(prompt))
                gQuit = 1;
        }
        break;
    }
}
#endif

/* MainScreenKey (mac _MainScreenKey). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004872a0
// MAC: _MainScreenKey i386 0x0001aba0 ppc 0x0001eabc
#ifdef MACBUILD
void MainScreenKey(char key, unsigned char code, short modifiers)
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
#ifndef MAC1010
        do
            QuickKeyCheckHandler();
        while (QuickKeyCheck(kKeyE));
#endif
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
        strcpy(textShowString, briefingTextPtr);
        ShowTextDialog(0, 0, 1);
        mainScreenBackgroundDirty = 1;
    }
}
#else
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
        strcpy(textShowString, briefingTextPtr);
        ShowTextDialog(0, 0, 1);
        mainScreenBackgroundDirty = 1;
    }
}
#endif

#ifdef MACBUILD
/* DoMenuClick (mac _DoMenuClick): mac only. */
// MAC: _DoMenuClick i386 0x00019c49 ppc 0x0001d8f8
void DoMenuClick(long menuChoice)
#ifdef MAC1010
;
#else
#include "../engine/domenuclick.inc"
#endif

/* MainScreenActivate (mac _MainScreenActivate): mac only. */
// MAC: _MainScreenActivate i386 0x000193ea ppc 0x0001d0cc
void MainScreenActivate(Boolean activate)
#ifdef MAC1010
;
#else
#include "../engine/mainscreenactivate.inc"
#endif
#endif
