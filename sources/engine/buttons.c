// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* buttons.c -- the custom picture buttons every EV Nova dialog draws over its Dialog Manager items (win TU region 0x0049eef0..0x004a3aa0), ending in CrtInitHook6, the object's .CRT hook. Notes: Research/notes/buttons.md */
#include "macport.h"

#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "render.h"
#include "burger.h"
#include "prototypes.h"
#include "cefix.h"

/* Toolbox shims (win port implementations) */

/* win-only: pushes the dialog's offscreen to the window between repaints */

/* rest of this TU */

#define kButtonPictID         7500
#define kButtonMaskPictID     7600

/* the mac builds the offscreens on the main screen's device; the port's take none */
#ifdef MACBUILD
#define kButtonDevice GetMainDevice()
#else
#define kButtonDevice 0
#endif

/* button slots: the index into a Track/Draw pair's Rect array */
enum { kTradeDoneButton, kTradeBuyButton, kTradeSellButton };
enum { kMissionAcceptButton, kMissionOkButton };
enum { kMissionInfoDoneButton, kMissionInfoAbortButton };
enum { kHaggleYesButton, kHaggleNoButton };
#ifdef MAC1010 /* 1.0.10 spelling: plain numbers at the sites */
#define kTradeDoneButton 0
#define kTradeBuyButton 1
#define kTradeSellButton 2
#define kMissionAcceptButton 0
#define kMissionOkButton 1
#define kMissionInfoDoneButton 0
#define kMissionInfoAbortButton 1
#define kHaggleYesButton 0
#define kHaggleNoButton 1
#endif

/* the button object's globals; this file ends that object (the .CRT hook).
   mapButtonTable's sixth slot is shipyardButtonTable[0], as in the exe */
GWorldContainer buttonPort;
GWorldContainer buttonImagePort;
GWorldContainer buttonMaskPort;
short mapButtonTable[5];
short shipyardButtonTable[5];
short outfitButtonTable[5];
short planetButtonTable[kPortButtons];
short tradeButtonTable[3];
short commButtonTable[3];
short planetCommButtonTable[3];
short missionButtonTable[2];
short missionInfoButtonTable[2];
short escortButtonTable[4];
short haggleButtonTable[2];
short playerInfoButtonTable[6];
short barButtonTable[6];
short plunderButtonTable[6];
short capAssignButtonTable[2];
short textButtonTable[3];
short singleMissionButtonTable[4];
short raceButtonTable[6];
RGBColor buttonColor[3];
Rect buttonPieceRect[kButtonPieceCount];
unsigned char buttonLabel[kButtonLabelCount][256];
unsigned char missionButtonLabel[kMissionButtonLabelCount][256];
#ifndef MACBUILD
unsigned char DAT_007dc2aa[261];
#endif

/* TrackTradeButtons. STATE: MATCH 100%, exe 158 / obj 158, byte-identical.
   The slot enum (-enum min) at the GetDialogItem sites keeps their addresses out of the CSE
   class the PtInRect sites share. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004a04d0
// MAC: _TrackTradeButtons i386 0x000294cd ppc 0x0002dae4
short TrackTradeButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[3];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(tradeDialog, 1, &itemType, &item, &box[kTradeDoneButton]);
    GetDialogItem(tradeDialog, 0xd, &itemType, &item, &box[kTradeBuyButton]);
    GetDialogItem(tradeDialog, 0xe, &itemType, &item, &box[kTradeSellButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRectPoint(where, &box[1]) != 0)
        drawnButton = 1;
    if (PtInRectPoint(where, &box[2]) != 0)
        drawnButton = 2;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawTradeButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRectPoint(mouse, &box[0]) != 0)
                overButton = 0;
            if (PtInRectPoint(mouse, &box[1]) != 0
                    && CanBuyGoods(selTradeItem) != 0)
                overButton = 1;
            if (PtInRectPoint(mouse, &box[2]) != 0
                    && CanSellGoods(selTradeItem) != 0)
                overButton = 2;
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawTradeButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawTradeButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a06a0
// MAC: _DrawTradeButtons i386 0x000292e2 ppc 0x0002d960
void DrawTradeButtons(short hilited)
{
    Rect box[3];
    short itemType;
    Handle item;
    Boolean enabled[3];
    short i;

    ForeColor(blackColor);
    GetDialogItem(tradeDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(tradeDialog, 0xd, &itemType, &item, &box[1]);
    GetDialogItem(tradeDialog, 0xe, &itemType, &item, &box[2]);
    enabled[0] = 1;
    enabled[1] = CanBuyGoods(selTradeItem);
    enabled[2] = CanSellGoods(selTradeItem);
    tradeButtonTable[0] = kDoneButtonLabel;
    tradeButtonTable[1] = kBuyButtonLabel;
    tradeButtonTable[2] = kSellButtonLabel;
    for (i = 0; i < 3; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(tradeDialog), &box[i],
                           buttonLabel[tradeButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(tradeDialog), &box[i],
                           buttonLabel[tradeButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(tradeDialog), &box[i],
                       buttonLabel[tradeButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackCapAssignButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0810
// MAC: _TrackCapAssignButtons i386 0x000291c7 ppc 0x0002d824
short TrackCapAssignButtons(Point where)
{
    short drawnButton;
    short downButton;
    short i;
    short overButton;
    Rect box[2];
    short itemType;
    Handle item;
    Point mouse;

    for (i = 0; i <= 1; i++)
        GetDialogItem(captureDialog, i + 1, &itemType, &item, (Rect *)&box + i);
    drawnButton = -1;
    for (i = 0; i <= 1; i++) {
        if (PtInRectPoint(where, &box[i]) != 0) {
            drawnButton = i;
            break;
        }
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawCapAssignButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            overButton = -1;
            GetMouse(&mouse);
            for (i = 0; i <= 1; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0) {
                    overButton = i;
                    break;
                }
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawCapAssignButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawCapAssignButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0950
// MAC: _DrawCapAssignButtons i386 0x000290c7 ppc 0x0002d738
void DrawCapAssignButtons(short hilited)
{
    short i;
    Rect box;
    short itemType;
    Handle item;

    capAssignButtonTable[0] = kUseAsMyShipButtonLabel;
    capAssignButtonTable[1] = kUseAsEscortButtonLabel;
    for (i = 0; i <= 1; i++) {
        GetDialogItem(captureDialog, i + 1, &itemType, &item, &box);
        if (i == hilited)
            DrawButton(GetDialogPort(captureDialog), &box,
                       buttonLabel[capAssignButtonTable[i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(captureDialog), &box,
                       buttonLabel[capAssignButtonTable[i]], 0, 0, 0);
    }
}

/* TrackCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a09e0
// MAC: _TrackCommDialogButtons i386 0x00028eff ppc 0x0002d55c
short TrackCommDialogButtons(Point where)
{
    Boolean altItem;
    short drawnButton;
    short downButton;
    short i;
    short overButton;
    Rect box[3];
    short itemType;
    Handle item;
    Point mouse;

    altItem = 0;
    if (commShip != 0 && commShip->govt != -1
            && (govtData[commShip->govt].flags2 & kGovtNotTalkative) != 0)
        altItem = 1;
    if (altItem != 0) {
        GetDialogItem(commDialog, 1, &itemType, &item, &box[0]);
        GetDialogItem(commDialog, 4, &itemType, &item, &box[1]);
        GetDialogItem(commDialog, 2, &itemType, &item, &box[2]);
    } else {
        for (i = 0; i <= 2; i++)
            GetDialogItem(commDialog, i + 1, &itemType, &item, &box[i]);
    }
    drawnButton = -1;
    for (i = 0; i <= 2; i++) {
        if (PtInRectPoint(where, &box[i]) != 0) {
            drawnButton = i;
            break;
        }
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawCommDialogButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            overButton = -1;
            GetMouse(&mouse);
            for (i = 0; i <= 2; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0) {
                    overButton = i;
                    break;
                }
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawCommDialogButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0bc0
// MAC: _DrawCommDialogButtons i386 0x00028c43 ppc 0x0002d2a8
void DrawCommDialogButtons(short hilited)
{
    Rect box[3];
    Boolean hasMission;
    Boolean altItem;
    short itemType;
    Handle item;
    short i;
    Boolean enabled[4];

    ForeColor(blackColor);
    altItem = 0;
    if (commShip != 0 && commShip->govt != -1
            && (govtData[commShip->govt].flags2 & kGovtNotTalkative) != 0)
        altItem = 1;
    if (altItem != 0) {
        GetDialogItem(commDialog, 1, &itemType, &item, &box[0]);
        GetDialogItem(commDialog, 4, &itemType, &item, &box[1]);
        GetDialogItem(commDialog, 2, &itemType, &item, &box[2]);
    } else {
        for (i = 0; i <= 2; i++)
            GetDialogItem(commDialog, i + 1, &itemType, &item, &box[i]);
    }
    hasMission = 0;
    if (commShip != 0 && commShip->specialMission != -1
            && currentMissionObjectives[commShip->specialMission].active != 0
            && currentMissions[commShip->specialMission].shipBehav == kMissionModeEscort)
        hasMission = 1;
    enabled[0] = 1;
    enabled[2] = 1;
    commButtonTable[0] = kCloseChannelButtonLabel;
    commButtonTable[2] = kGreetingsButtonLabel;
    if (altItem != 0) {
        enabled[1] = 0;
    } else {
        enabled[1] = 1;
        if (commShip != 0) {
            if (ExtendedIsThreatToPlayer(commShip) != 0)
                commButtonTable[1] = kBegMercyButtonLabel;
            else if (commShip->parentShip == 0 && commShip->aiType == kShipTypeEscort &&
                     hasMission == 0)
                commButtonTable[1] = kReleaseButtonLabel;
            else
                commButtonTable[1] = kRequestAssistButtonLabel;
        } else {
            commButtonTable[1] = kRequestAssistButtonLabel;
        }
    }
    for (i = 0; i <= 2; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(commDialog), &box[i],
                           buttonLabel[commButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(commDialog), &box[i],
                           buttonLabel[commButtonTable[i]], 0, 0, 0);
        }
    }
}

/* TrackPlanetCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0e50
// MAC: _TrackPlanetCommDialogButtons i386 0x00028b28 ppc 0x0002d16c
short TrackPlanetCommDialogButtons(Point where)
{
    short drawnButton;
    short downButton;
    short i;
    short overButton;
    Rect box[4];    /* four slots reserved; only the three items are tracked */
    short itemType;
    Handle item;
    Point mouse;

    for (i = 0; i <= 2; i++)
        GetDialogItem(commDialog, i + 1, &itemType, &item, (Rect *)&box + i);
    drawnButton = -1;
    for (i = 0; i <= 2; i++) {
        if (PtInRectPoint(where, &box[i]) != 0) {
            drawnButton = i;
            break;
        }
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawPlanetCommDialogButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            overButton = -1;
            GetMouse(&mouse);
            for (i = 0; i <= 2; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0) {
                    overButton = i;
                    break;
                }
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawPlanetCommDialogButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawPlanetCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0f90
// MAC: _DrawPlanetCommDialogButtons i386 0x0002893c ppc 0x0002cf8c
void DrawPlanetCommDialogButtons(short hilited)
{
    short i;
    Rect box[3];
    short itemType;
    Handle item;
    Boolean enabled[3];

    ForeColor(blackColor);
    for (i = 0; i < 3; i++)
        GetDialogItem(commDialog, i + 1, &itemType, &item, &box[i]);
    enabled[0] = 1;
    enabled[1] = 1;
    planetCommButtonTable[0] = kCloseChannelButtonLabel;
    if (hatesPlayer != 0)
        planetCommButtonTable[1] = kOfferBribeButtonLabel;
    else
        planetCommButtonTable[1] = kGreetingsButtonLabel;
    if (stellarData[shipData[0].targetStellar].dominated != 0) {
        if ((stellarData[shipData[0].targetStellar].flags2 & kSpobAlwaysDominated) != 0) {
            enabled[2] = 0;
        } else {
            enabled[2] = 1;
            planetCommButtonTable[2] = kReleaseButtonLabel;
        }
    } else {
        enabled[2] = 1;
        planetCommButtonTable[2] = kDemandTributeButtonLabel;
    }
    for (i = 0; i < 3; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(commDialog), &box[i],
                           buttonLabel[planetCommButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(commDialog), &box[i],
                           buttonLabel[planetCommButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(commDialog), &box[i],
                       buttonLabel[planetCommButtonTable[i]], kButtonDim, 0, 0);
    }
    ForeColor(blackColor);
}

/* TrackMissionButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1130
// MAC: _TrackMissionButtons i386 0x000287f5 ppc 0x0002ce34
short TrackMissionButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[2];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(missionDialog, 1, &itemType, &item, &box[kMissionAcceptButton]);
    GetDialogItem(missionDialog, 7, &itemType, &item, &box[kMissionOkButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[0]) != 0 && selMissionListItem != -1)
        drawnButton = 0;
    else if (PtInRectPoint(where, &box[1]) != 0)
        drawnButton = 1;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawMissionButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRectPoint(mouse, &box[0]) != 0
                    && selMissionListItem != -1)
                overButton = 0;
            else if (PtInRectPoint(mouse, &box[1]) != 0)
                overButton = 1;
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawMissionButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawMissionButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1290
// MAC: _DrawMissionButtons i386 0x00028657 ppc 0x0002ccc8
void DrawMissionButtons(short hilited)
{
    short i;
    Rect box[2];
    short itemType;
    Handle item;
    Boolean enabled[2];

    ForeColor(blackColor);
    GetDialogItem(missionDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(missionDialog, 7, &itemType, &item, &box[1]);
    missionButtonTable[0] = kAcceptButtonLabel;
    missionButtonTable[1] = kLeaveButtonLabel;
    if (selMissionListItem == -1)
        enabled[0] = 0;
    else
        enabled[0] = 1;
    enabled[1] = 1;
    for (i = 0; i <= 1; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(missionDialog), &box[i],
                           buttonLabel[missionButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(missionDialog), &box[i],
                           buttonLabel[missionButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(missionDialog), &box[i],
                       buttonLabel[missionButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackMissionInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a13c0
// MAC: _TrackMissionInfoButtons i386 0x0002850e ppc 0x0002cb74
short TrackMissionInfoButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[2];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 1, &itemType, &item, &box[kMissionInfoDoneButton]);
    GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box[kMissionInfoAbortButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRectPoint(where, &box[1]) != 0 && selMissionListItem != -1)
        drawnButton = 1;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawMissionInfoButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != '\0') {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRectPoint(mouse, &box[0]) != 0)
                overButton = 0;
            if (PtInRectPoint(mouse, &box[1]) != 0 && selMissionListItem != -1)
                overButton = 1;
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawMissionInfoButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawMissionInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1520
// MAC: _DrawMissionInfoButtons i386 0x00028359 ppc 0x0002c9f4
void DrawMissionInfoButtons(short hilited)
{
    short i;
    Rect box[2];
    short itemType;
    Handle item;
    Boolean enabled[2];

    ForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box[1]);
    enabled[0] = 1;
    if (selMissionListItem == -1 || showAbortMissionButton == 0)
        enabled[1] = 0;
    else
        enabled[1] = 1;
    missionInfoButtonTable[0] = kDoneButtonLabel;
    missionInfoButtonTable[1] = kAbortButtonLabel;
    for (i = 0; i < 2; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(missionInfoDialog), &box[i],
                           buttonLabel[missionInfoButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(missionInfoDialog), &box[i],
                           buttonLabel[missionInfoButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(missionInfoDialog), &box[i],
                       buttonLabel[missionInfoButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackSingleMissionButtons. STATE: MATCH 100%, 146/146, byte-identical. Every loop indexes
   box[], `where' is read in place and the tracking loop tests StillDown() directly.
   OPEN: none */
// FUNCTION: EVNOVA 0x004a1670
// MAC: _TrackSingleMissionButtons i386 0x00028188 ppc 0x0002c820
short TrackSingleMissionButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    short i;
    Rect box[4];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    if (canRefuseMission != 0) {
        for (i = 0; i <= 1; i++)
            GetDialogItem(missionDialog, i + 1, &itemType, &item, &box[i]);
    } else {
        GetDialogItem(missionDialog, 6, &itemType, &item, &box[0]);
        SetRect(&box[1], -1, -1, 0, 0);
    }
    for (i = 0; i <= 1; i++)
        GetDialogItem(missionDialog, i + 9, &itemType, &item, &box[i + 2]);
    drawnButton = -1;
    for (i = 0; i <= 3; i++) {
        if (PtInRectPoint(where, &box[i]) != 0)
            drawnButton = i;
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawSingleMissionButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown()) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            for (i = 0; i <= 3; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0)
                    overButton = i;
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawSingleMissionButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawSingleMissionButtons (mac _DrawSingleMissionButtons). STATE: MATCH 100.0%, exe 191 /
   obj 191, relocregcheck 0 of 41; `int j' is declared last. OPEN: none */
// FUNCTION: EVNOVA 0x004a1820
// MAC: _DrawSingleMissionButtons i386 0x00027db8 ppc 0x0002c4d8
void DrawSingleMissionButtons(short hilited)
{
    short itemType;
    short i;
    Handle item;
    Rect box[4];
    short dimmed[4];

    ForeColor(blackColor);
    singleMissionButtonTable[2] = kScrollUpButtonLabel;
    singleMissionButtonTable[3] = kScrollDownButtonLabel;
    if (canScrollTextUp != 0 || canScrollTextDown != 0) {
        if (canScrollTextUp != 0)
            dimmed[2] = 0;
        else
            dimmed[2] = kButtonDim;
        if (canScrollTextDown != 0)
            dimmed[3] = 0;
        else
            dimmed[3] = kButtonDim;
    } else {
        dimmed[2] = kButtonDisabled;
        dimmed[3] = kButtonDisabled;
    }
    GetDialogItem(missionDialog, 9, &itemType, &item, &box[2]);
    GetDialogItem(missionDialog, 10, &itemType, &item, &box[3]);
    if (canRefuseMission) {
        for (i = 0; i <= 1; i++)
            GetDialogItem(missionDialog, i + 1, &itemType, &item, &box[i]);
        singleMissionButtonTable[0] = kYesButtonLabel;
        singleMissionButtonTable[1] = kNoButtonLabel;
        dimmed[0] = 0;
        dimmed[1] = 0;
        for (i = 0; i <= 1; i++) {
            if (hilited == i)
                DrawButton(GetDialogPort(missionDialog), &box[i], missionButtonLabel[i], dimmed[i], 1, 0);
            else
                DrawButton(GetDialogPort(missionDialog), &box[i], missionButtonLabel[i], dimmed[i], 0, 0);
        }
    } else {
        GetDialogItem(missionDialog, 6, &itemType, &item, &box[0]);
        GetDialogItem(missionDialog, 6, &itemType, &item, &box[1]);
        singleMissionButtonTable[0] = kOkButtonLabel;
        dimmed[0] = 0;
        dimmed[1] = kButtonDisabled;
        if (hilited == 0)
            DrawButton(GetDialogPort(missionDialog), &box[0], missionButtonLabel[0], dimmed[0], 1, 0);
        else
            DrawButton(GetDialogPort(missionDialog), &box[0], missionButtonLabel[0], dimmed[0], 0, 0);
    }
    for (i = 2; i <= 3; i++) {
        if (hilited == i)
            DrawButton(GetDialogPort(missionDialog), &box[i],
                       buttonLabel[singleMissionButtonTable[i]], dimmed[i], 1, 0);
        else
            DrawButton(GetDialogPort(missionDialog), &box[i],
                       buttonLabel[singleMissionButtonTable[i]], dimmed[i], 0, 0);
    }
}

/* TrackPlayerInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1ae0
// MAC: _TrackPlayerInfoButtons i386 0x00027c4d ppc 0x0002c370
short TrackPlayerInfoButtons(short page, Point where)
{
    short drawnButton;
    short downButton;
    short i;
    short overButton;
    Rect box[6];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    for (i = 0; i <= 4; i++)
        GetDialogItem(infoDialog, i + 1, &itemType, &item, (Rect *)&box + i);
    GetDialogItem(infoDialog, 7, &itemType, &item, &box[5]);
    drawnButton = -1;
    for (i = 0; i <= 5; i++) {
        if (PtInRectPoint(where, &box[i]) != 0) {
            drawnButton = i;
            break;
        }
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawPlayerInfoButtons(page, drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            for (i = 0; i <= 5; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0) {
                    overButton = i;
                    break;
                }
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawPlayerInfoButtons(page, overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawPlayerInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1c40
// MAC: _DrawPlayerInfoButtons i386 0x00027aae ppc 0x0002c1e4
void DrawPlayerInfoButtons(short page, short hilited)
{
    short itemType;
    short i;
    Handle item;
    Rect box[6];
    Boolean enabled[6];

    playerInfoButtonTable[0] = kDoneButtonLabel;
    playerInfoButtonTable[1] = kGeneralButtonLabel;
    playerInfoButtonTable[2] = kCargoButtonLabel;
    playerInfoButtonTable[3] = kExtrasButtonLabel;
    playerInfoButtonTable[4] = kHonorsButtonLabel;
    playerInfoButtonTable[5] = kJettisonButtonLabel;
    for (i = 0; i <= 4; i++) {
        enabled[i] = 1;
        GetDialogItem(infoDialog, i + 1, &itemType, &item, &box[i]);
    }
    GetDialogItem(infoDialog, 7, &itemType, &item, &box[5]);
    if (page == 2)
        enabled[5] = PlayerHasJettisonableCargo();
    else
        enabled[5] = 0;
    ForeColor(blackColor);
    for (i = 0; i <= 5; i++) {
        if (enabled[i] != 0) {
            if (hilited == i || page == i)
                DrawButton(GetDialogPort(infoDialog), &box[i],
                           buttonLabel[playerInfoButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(infoDialog), &box[i],
                           buttonLabel[playerInfoButtonTable[i]], 0, 0, 0);
        }
    }
}

/* TrackEscortButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1d90
// MAC: _TrackEscortButtons i386 0x00027987 ppc 0x0002c0a0
short TrackEscortButtons(Point where)
{
    short drawnButton;
    short downButton;
    short i;
    short overButton;
    Rect box[4];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    for (i = 0; i <= 3; i++)
        GetDialogItem(commDialog, i + 1, &itemType, &item, (Rect *)&box + i);
    drawnButton = -1;
    for (i = 0; i <= 3; i++) {
        if (PtInRectPoint(where, &box[i]) != 0) {
            drawnButton = i;
            break;
        }
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawEscortButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            for (i = 0; i <= 3; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0) {
                    overButton = i;
                    break;
                }
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawEscortButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawEscortButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1ed0
// MAC: _DrawEscortButtons i386 0x000277a4 ppc 0x0002bea0
void DrawEscortButtons(short hilited)
{
    short i;
    Rect box[4];
    short itemType;
    Handle item;
    Boolean enabled[4];

    ForeColor(blackColor);
    for (i = 0; i < 4; i++)
        GetDialogItem(commDialog, i + 1, &itemType, &item, &box[i]);
    enabled[0] = 1;
    enabled[1] = 1;
    if (canUpgradeEscort != 0)
        enabled[2] = 1;
    else
        enabled[2] = 0;
    if (canSellEscort != 0)
        enabled[3] = 1;
    else
        enabled[3] = 0;
    escortButtonTable[0] = kCloseChannelButtonLabel;
    escortButtonTable[1] = kReleaseButtonLabel;
    if (commShip != 0) {
        if (commShip->upgradeOnLanding != 0)
            escortButtonTable[2] = kCancelUpgradeButtonLabel;
        else
            escortButtonTable[2] = kUpgradeEscortButtonLabel;
        if (commShip->sellOnLanding != 0)
            escortButtonTable[3] = kCancelSaleButtonLabel;
        else
            escortButtonTable[3] = kSellEscortButtonLabel;
    } else {
        escortButtonTable[2] = kUpgradeEscortButtonLabel;
        escortButtonTable[3] = kSellEscortButtonLabel;
    }
    for (i = 0; i < 4; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(commDialog), &box[i],
                           buttonLabel[escortButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(commDialog), &box[i],
                           buttonLabel[escortButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(commDialog), &box[i],
                       buttonLabel[escortButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackHaggleButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2090
// MAC: _TrackHaggleButtons i386 0x00027670 ppc 0x0002bd78
short TrackHaggleButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[2];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(haggleDialog, 1, &itemType, &item, &box[kHaggleYesButton]);
    GetDialogItem(haggleDialog, 2, &itemType, &item, &box[kHaggleNoButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRectPoint(where, &box[1]) != 0)
        drawnButton = 1;
    downButton = drawnButton;
    if (downButton != -1) {
        DrawHaggleButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRectPoint(mouse, &box[0]) != 0)
                overButton = 0;
            if (PtInRectPoint(mouse, &box[1]) != 0)
                overButton = 1;
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawHaggleButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawHaggleButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a21e0
// MAC: _DrawHaggleButtons i386 0x0002750d ppc 0x0002bc34
void DrawHaggleButtons(short hilited)
{
    short i;
    Rect box[2];
    short itemType;
    Handle item;

    ForeColor(blackColor);
    GetDialogItem(haggleDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(haggleDialog, 2, &itemType, &item, &box[1]);
    if (haggleMode == 1) {
        haggleButtonTable[0] = kAcceptPaymentButtonLabel;
        haggleButtonTable[1] = kDemandMoreButtonLabel;
    } else {
        haggleButtonTable[0] = kAcceptPriceButtonLabel;
        haggleButtonTable[1] = kLowerPriceButtonLabel;
    }
    for (i = 0; i <= 1; i++) {
        if (i == hilited)
            DrawButton(GetDialogPort(haggleDialog), &box[i],
                       buttonLabel[haggleButtonTable[i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(haggleDialog), &box[i],
                       buttonLabel[haggleButtonTable[i]], 0, 0, 0);
    }
}

/* TrackPlunderButtons. STATE: MATCH 100%, exe 157 / obj 157, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004a22e0
// MAC: _TrackPlunderButtons i386 0x0002732a ppc 0x0002b9e0
short TrackPlunderButtons(Point where)
#ifdef MAC1010
{
    short i;
    short drawnButton;
    short downButton;
    short over;
    Rect box[6];
    Boolean enabled[6];
    Handle item;
    Point mouse;
    short itemType;

    ForeColor(blackColor);
    for (i = 0; i <= 3; i++)
        GetDialogItem(plunderDialog, i + 1, &itemType, &item, &box[i]);
    for (i = 4; i <= 5; i++)
        GetDialogItem(plunderDialog, i + 2, &itemType, &item, &box[i]);
    enabled[0] = 1;
    if (salvageType != -1)
        enabled[1] = 1;
    else
        enabled[1] = 0;
    if (0 < salvageCredits)
        enabled[2] = 1;
    else
        enabled[2] = 0;
    if (salvageAmmoType != -1)
        enabled[3] = 1;
    else
        enabled[3] = 0;
    if (0 < salvageFuelQty)
        enabled[4] = 1;
    else
        enabled[4] = 0;
    if (0 < captureOdds)
        enabled[5] = 1;
    else
        enabled[5] = 0;
    drawnButton = -1;
    for (i = 0; i <= 5; i++) {
        if (enabled[i] != 0 && (PtInRect)(where, &box[i]))
            drawnButton = i;
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawPlunderButtons(drawnButton);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            for (i = 0; i <= 5; i++) {
                if (enabled[i] != 0 && (PtInRect)(mouse, &box[i]))
                    over = i;
            }
            if (over != drawnButton && over != downButton)
                over = -1;
            if (over != drawnButton)
                DrawPlunderButtons(over);
            drawnButton = over;
        }
    }
    return drawnButton;
}
#else
{
    short drawnButton;
    int downH;
    int downV;
    short overButton;
    Rect *walk;
    Rect *r;
    Rect *fill;
    short over;
    short i;
    Rect box[6];
    Boolean enabled[6];
    Handle item;
    Rect *first;
    Point mouse;
    short itemType;
    short downButton;

    ForeColor(blackColor);
    for (over = 0, fill = box; over <= 3; over++, fill++)
        GetDialogItem(plunderDialog, over + 1, &itemType, &item, fill);
    for (over = 4, fill = &box[4]; over <= 5; over++, fill++)
        GetDialogItem(plunderDialog, over + 2, &itemType, &item, fill);
    enabled[0] = 1;
    if (salvageType != -1)
        enabled[1] = 1;
    else
        enabled[1] = 0;
    if (0 < salvageCredits)
        enabled[2] = 1;
    else
        enabled[2] = 0;
    if ((short)salvageAmmoType != -1)
        enabled[3] = 1;
    else
        enabled[3] = 0;
    if (0 < salvageFuelQty)
        enabled[4] = 1;
    else
        enabled[4] = 0;
    if (0 < captureOdds)
        enabled[5] = 1;
    else
        enabled[5] = 0;
    drawnButton = -1;
    overButton = 0;
    first = box;
    walk = box;
#if V111 /* V111 spelling */
#else
    downV = *(short *)&where;
    downH = ((short *)&where)[1];
#endif
    for (; overButton <= 5; overButton++, walk++) {
#if V111 /* V111 spelling */
        if (enabled[overButton] != 0 && PtInRectPoint(where, &box[overButton]) != 0)
#else
        if (enabled[overButton] != 0 && PtInRect(downH, downV, walk) != 0)
#endif
            drawnButton = overButton;
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawPlunderButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            i = -1;
            for (overButton = 0, r = first; overButton <= 5; overButton++, r++) {
#if V111 /* V111 spelling */
                if (enabled[overButton] != 0 && PtInRectPoint(mouse, &box[overButton]) != 0)
#else
                if (enabled[overButton] != 0 && PtInRectPoint(mouse, r) != 0)
#endif
                    i = overButton;
            }
            if (i != drawnButton && i != downButton)
                i = -1;
            if (i != drawnButton) {
                DrawPlunderButtons(i);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = i;
        }
    }
    return drawnButton;
}
#endif

/* DrawPlunderButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a24e0
// MAC: _DrawPlunderButtons i386 0x00027109 ppc 0x0002b784
void DrawPlunderButtons(short hilited)
{
    short i;
    Rect box[6];
    Boolean enabled[6];
    Handle item;
    short itemType;

    for (i = 0; i <= 3; i++)
        GetDialogItem(plunderDialog, i + 1, &itemType, &item, &box[i]);
    for (i = 4; i <= 5; i++)
        GetDialogItem(plunderDialog, i + 2, &itemType, &item, &box[i]);
    plunderButtonTable[0] = kAbortButtonLabel;
    plunderButtonTable[1] = kPlunderCargoButtonLabel;
    plunderButtonTable[2] = kCreditsButtonLabel;
    plunderButtonTable[3] = kAmmoButtonLabel;
    plunderButtonTable[4] = kEnergyButtonLabel;
    plunderButtonTable[5] = kCaptureShipButtonLabel;
    enabled[0] = 1;
    if (salvageType != -1)
        enabled[1] = 1;
    else
        enabled[1] = 0;
    if (0 < salvageCredits)
        enabled[2] = 1;
    else
        enabled[2] = 0;
    if ((short)salvageAmmoType != -1)
        enabled[3] = 1;
    else
        enabled[3] = 0;
    if (0 < salvageFuelQty)
        enabled[4] = 1;
    else
        enabled[4] = 0;
    if (0 < captureOdds)
        enabled[5] = 1;
    else
        enabled[5] = 0;
    ForeColor(blackColor);
    for (i = 0; i <= 5; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(plunderDialog), &box[i],
                           buttonLabel[plunderButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(plunderDialog), &box[i],
                           buttonLabel[plunderButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(plunderDialog), &box[i],
                       buttonLabel[plunderButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackBarButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a26e0
// MAC: _TrackBarButtons i386 0x00026fd5 ppc 0x0002b644
short TrackBarButtons(Point where)
{
    Rect *r;
    short i;
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[6];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    for (i = 0; i <= 5; i++)
        GetDialogItem(barDialog, i + 1, &itemType, &item, (Rect *)&box + i);
    drawnButton = -1;
    for (i = 0; i <= 5; i++) {
        if (PtInRectPoint(where, &box[i]) != 0)
            drawnButton = i;
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawBarButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            for (i = 0; i <= 5; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0)
                    overButton = i;
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawBarButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawBarButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2810
// MAC: _DrawBarButtons i386 0x00026e1d ppc 0x0002b4d0
void DrawBarButtons(short hilited)
{
    short i;
    Rect box[6];
    Boolean enabled[6];
    Handle item;
    short itemType;

    for (i = 0; i <= 5; i++)
        GetDialogItem(barDialog, i + 1, &itemType, &item, &box[i]);
    enabled[0] = 1;
    enabled[1] = 1;
    enabled[2] = 1;
    enabled[3] = 0;
    enabled[4] = CanHireEscorts();
    enabled[5] = 0;
    barButtonTable[0] = kLeaveButtonLabel;
    barButtonTable[1] = kGambleButtonLabel;
    barButtonTable[2] = kHolovidButtonLabel;
    barButtonTable[4] = kHireEscortButtonLabel;
    ForeColor(blackColor);
    for (i = 0; i < 6; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(barDialog), &box[i],
                           buttonLabel[barButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(barDialog), &box[i],
                           buttonLabel[barButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(barDialog), &box[i],
                       buttonLabel[barButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackOkButton (mac _TrackOkButton): dead on win (stripped). */
// MAC: _TrackOkButton i386 0x00026d7a ppc 0x0002b428
Boolean TrackOkButton(Rect box)
{
    Point mouse;
    Boolean inside;
    Boolean nowInside;

    GetMouse(&mouse);
    inside = PtInRectPoint(mouse, &box) != 0;
    DrawOkButton(box, inside);
    while (StillDown() != 0) {
        BusyIdle();
        GetMouse(&mouse);
        nowInside = PtInRectPoint(mouse, &box) != 0;
        if (nowInside != inside)
            DrawOkButton(box, nowInside);
        inside = nowInside;
    }
    return inside;
}

/* DrawOkButton (mac _DrawOkButton): dead on win (stripped). */
// MAC: _DrawOkButton i386 0x00026d06 ppc 0x0002b3b0
void DrawOkButton(Rect box, Boolean hilited)
{
    CGrafPtr port;

    GetPort((GrafPtr *)&port);
    if (hilited)
        DrawButton(port, &box, buttonLabel[kOkButtonLabel], 0, 1, 0);
    else
        DrawButton(port, &box, buttonLabel[kOkButtonLabel], 0, 0, 0);
}

/* TrackTextButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2960
// MAC: _TrackTextButtons i386 0x00026bb0 ppc 0x0002b25c
short TrackTextButtons(Point where)
{
    short i;
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[3];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(textDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(textDialog, 5, &itemType, &item, &box[1]);
    GetDialogItem(textDialog, 6, &itemType, &item, &box[2]);
    drawnButton = -1;
    for (i = 0; i < 3; i++) {
        if (PtInRectPoint(where, &box[i]) != 0)
            drawnButton = i;
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawTextButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            for (i = 0; i < 3; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0)
                    overButton = i;
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawTextButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawTextButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2ac0
// MAC: _DrawTextButtons i386 0x000269ea ppc 0x0002b090
void DrawTextButtons(short hilited)
{
    short i;
    Rect box[3];
    short dimmed[3];
    Handle item;
    short itemType;

    GetDialogItem(textDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(textDialog, 5, &itemType, &item, &box[1]);
    GetDialogItem(textDialog, 6, &itemType, &item, &box[2]);
    textButtonTable[0] = kOkButtonLabel;
    textButtonTable[1] = kScrollUpButtonLabel;
    textButtonTable[2] = kScrollDownButtonLabel;
    dimmed[0] = 0;
    if (canScrollTextUp != 0 || canScrollTextDown != 0) {
        if (canScrollTextUp != 0)
            dimmed[1] = 0;
        else
            dimmed[1] = kButtonDim;
        if (canScrollTextDown != 0)
            dimmed[2] = 0;
        else
            dimmed[2] = kButtonDim;
    } else {
        dimmed[1] = kButtonDisabled;
        dimmed[2] = kButtonDisabled;
    }
    ForeColor(blackColor);
    for (i = 0; i < 3; i++) {
        if (hilited == i)
            DrawButton(GetDialogPort(textDialog), &box[i],
                       buttonLabel[textButtonTable[i]], dimmed[i], 1, 0);
        else
            DrawButton(GetDialogPort(textDialog), &box[i],
                       buttonLabel[textButtonTable[i]], dimmed[i], 0, 0);
    }
}

/* DrawRaceButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2c30
// MAC: _DrawRaceButtons i386 0x00026617 ppc 0x0002acc4
void DrawRaceButtons(short hilited)
{
    short i;
    PicHandle pic;
    Rect box[6];
    short dimmed[6];
    Handle item;
    short itemType;

    for (i = 0; i <= 1; i++) {
        GetDialogItem(raceDialog, i + 1, &itemType, &item, &box[i]);
        dimmed[i] = 0;
    }
    if (raceWinner == -1 && raceCount == 0)
        raceButtonTable[0] = kCancelButtonLabel;
    else
        raceButtonTable[0] = kDoneButtonLabel;
    raceButtonTable[1] = kHelpButtonLabel;
    for (i = 0; i < 4; i++) {
        GetDialogItem(raceDialog, i + 3, &itemType, &item, &box[i + 2]);
        dimmed[i + 2] = kButtonPicture;
        raceButtonTable[i + 2] = 0;
    }
    for (i = 0; i < 6; i++) {
        if (dimmed[i] == kButtonPicture) {
            if (raceWinner == i - 2)
                pic = raceButtonPic[i - 2][2];
            else if (raceWinner != -1)
                pic = raceButtonPic[i - 2][3];
            else if (hilited == i)
                pic = raceButtonPic[i - 2][1];
            else
                pic = raceButtonPic[i - 2][0];
        } else {
            pic = 0;
        }
        if (hilited == i)
            DrawButton(GetDialogPort(raceDialog), &box[i],
                       buttonLabel[raceButtonTable[i]], dimmed[i], 1, pic);
        else
            DrawButton(GetDialogPort(raceDialog), &box[i],
                       buttonLabel[raceButtonTable[i]], dimmed[i], 0, pic);
    }
}

/* TrackRaceButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2de0
// MAC: _TrackRaceButtons i386 0x0002689d ppc 0x0002af18
short TrackRaceButtons(Point where)
{
    short drawnButton;
    short downButton;
    short i;
    short over;
    Rect box[6];
    short itemType;
    Handle item;
    Point mouse;

    for (i = 0; i <= 1; i++)
        GetDialogItem(raceDialog, i + 1, &itemType, &item, (Rect *)&box + i);
    for (i = 0; i < 4; i++)
        GetDialogItem(raceDialog, i + 3, &itemType, &item, &box[i + 2]);
    drawnButton = -1;
    for (i = 0; i < 6; i++) {
        if (PtInRectPoint(where, &box[i]) != 0) {
            drawnButton = i;
            break;
        }
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawRaceButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            for (i = 0; i < 6; i++) {
                if (PtInRectPoint(mouse, &box[i]) != 0) {
                    over = i;
                    break;
                }
            }
            if (over != drawnButton && over != downButton)
                over = -1;
            if (over != drawnButton) {
                DrawRaceButtons(over);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = over;
        }
    }
    return drawnButton;
}

/* InitButtons. STATE: MATCH 100%, exe 264 / obj 264, byte-identical. The mask walker is
   set before the paint loop (the exe loads it there). OPEN: none */
// FUNCTION: EVNOVA 0x004a2f50
// MAC: _InitButtons i386 0x00025aa1 ppc 0x0002a1e0
void InitButtons(void)
{
    PicHandle buttonPic[kButtonPieceCount];
    PicHandle buttonMaskPic[kButtonPieceCount];
    Rect bounds;
    Rect *mask;
    short n;
    Rect *piece;

    for (n = 0; n < kButtonPieceCount; n++) {
        buttonPic[n] = GetPicture(n + kButtonPictID);
        buttonMaskPic[n] = GetPicture(n + kButtonMaskPictID);
        if (buttonPic[n] != 0) {
            QDGetPictureBounds(buttonPic[n], &buttonPieceRect[n]);
            OffsetRect(&buttonPieceRect[n], -buttonPieceRect[n].left, -buttonPieceRect[n].top);
        } else {
            SetRect(&buttonPieceRect[n], 0, 0, 0xc, 0x18);
        }
    }
    OffsetRect(&buttonPieceRect[1], buttonPieceRect[0].right, 0);
    OffsetRect(&buttonPieceRect[2], buttonPieceRect[1].right, 0);
    OffsetRect(&buttonPieceRect[3], 0, buttonPieceRect[0].bottom);
    OffsetRect(&buttonPieceRect[4], buttonPieceRect[3].right, buttonPieceRect[0].bottom);
    OffsetRect(&buttonPieceRect[5], buttonPieceRect[4].right, buttonPieceRect[0].bottom);
    OffsetRect(&buttonPieceRect[6], 0, buttonPieceRect[3].bottom);
    OffsetRect(&buttonPieceRect[7], buttonPieceRect[6].right, buttonPieceRect[3].bottom);
    OffsetRect(&buttonPieceRect[8], buttonPieceRect[7].right, buttonPieceRect[3].bottom);
    bounds.left = bounds.top = 0;
    bounds.right = (buttonPieceRect[2].right > buttonPieceRect[5].right) ?
                    buttonPieceRect[2].right : buttonPieceRect[5].right;
    bounds.right = (bounds.right > buttonPieceRect[8].right) ?
                    bounds.right : buttonPieceRect[8].right;
    bounds.bottom = (buttonPieceRect[6].bottom > buttonPieceRect[7].bottom) ?
                     buttonPieceRect[6].bottom : buttonPieceRect[7].bottom;
    bounds.bottom = (bounds.bottom > buttonPieceRect[8].bottom) ?
                     bounds.bottom : buttonPieceRect[8].bottom;
    CreateGWorldContainer(&buttonImagePort, kButtonDevice, gSpriteWorld->pixelDepth, &bounds);
    CreateGWorldContainer(&buttonMaskPort, kButtonDevice, 1, &bounds);
    SetGWorldContainer(&buttonImagePort);
    ForeColor(blackColor);
#ifdef MAC1010
    for (n = 0; n < kButtonPieceCount; n++) {
        PaintRect(&buttonPieceRect[n]);
        if (buttonPic[n] != 0)
            DrawPicHandle(buttonPic[n], &buttonPieceRect[n]);
    }
    UnsetGWorldContainer(&buttonImagePort);
    SetGWorldContainer(&buttonMaskPort);
    ForeColor(blackColor);
    for (n = 0; n < kButtonPieceCount; n++) {
        PaintRect(&buttonPieceRect[n]);
        if (buttonMaskPic[n] != 0)
            DrawPicHandle(buttonMaskPic[n], &buttonPieceRect[n]);
    }
#else
    mask = buttonPieceRect;
    for (n = 0, piece = buttonPieceRect; n < kButtonPieceCount; n++) {
        PaintRect(piece);
        if (buttonPic[n] != 0)
            DrawPicHandle(buttonPic[n], piece);
        piece++;
    }
    UnsetGWorldContainer(&buttonImagePort);
    SetGWorldContainer(&buttonMaskPort);
    ForeColor(blackColor);
    for (n = 0; n < kButtonPieceCount; n++) {
        PaintRect(mask);
        if (buttonMaskPic[n] != 0)
            DrawPicHandle(buttonMaskPic[n], mask);
        mask++;
    }
#endif
    UnsetGWorldContainer(&buttonMaskPort);
    for (n = 0; n < kButtonPieceCount; n++) {
        if (buttonPic[n] != 0)
            QDReleaseResource(buttonPic[n]);
        if (buttonMaskPic[n] != 0)
            QDReleaseResource(buttonMaskPic[n]);
    }
    for (n = 0; (int32_t)n < kButtonLabelCount; n++)
        GetIndString(buttonLabel[n], kButtonStrList, n + 1);
    buttonColor[0].red = buttonColor[0].green = buttonColor[0].blue = 0xffff;
    buttonColor[1].red = buttonColor[1].green = buttonColor[1].blue = 0x8000;
    buttonColor[2].red = buttonColor[2].green = buttonColor[2].blue = 0x8000;
#ifdef CEFIX /* F12.73-74 */
    SetRect(&bounds, 0, 0, 500, 75);
#else
    SetRect(&bounds, 0, 0, 200, 0x32);
#endif
    CreateGWorldContainer(&buttonPort, kButtonDevice, gSpriteWorld->pixelDepth, &bounds);
}

/* DisposeButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a3300
// MAC: _DisposeButtons i386 0x00025e67 ppc 0x0002a55c
void DisposeButtons(void)
{
    if (buttonPort.world != 0)
        DisposeGWorldContainer(&buttonPort);
    if (buttonImagePort.world != 0)
        DisposeGWorldContainer(&buttonImagePort);
    if (buttonMaskPort.world != 0)
        DisposeGWorldContainer(&buttonMaskPort);
}

/* --- DrawButton (0x004a3340) --- */


#define kGlyphCaret 0x5e
#define kGlyphAmp   0x26
#define kGlyphPlus  0x2b
#define kGlyphMinus 0x2d

/* DrawButton. STATE: MATCH 100%, exe 597 / obj 597. Every Rect copy goes through the inline
   CopyRect(), whose pointer operands stop mwcc reusing box.left for OffsetRect's argument.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004a3340
// MAC: _DrawButton i386 0x00025ea8 ppc 0x0002a5dc
void DrawButton(CGrafPtr port, Rect *bounds, unsigned char *label,
                short state, short selected, PicHandle pic)
{
    Rect piece;
    Rect mid;
    Rect box;
    CGrafPtr savedPort;
    short mode;
    short size;
    uint32_t glyph;

    if (state == 0) {
        if (selected == 0)
            mode = 0;
        else
            mode = 1;
    } else
        mode = 2;
    GetPort((GrafPtr *)&savedPort);
    if (state == kButtonPicture) {
        GWorldContainerSetPort(&loadPort);
        ForeColor(blackColor);
        DrawPicHandle(pic, bounds);
        SetPort((GrafPtr)savedPort);
        ForeColor(blackColor);
        CopyBits(GWorldContainerBits(&loadPort), GetPortBitMapForCopyBits(savedPort),
                 bounds, bounds, 0, GetPortVisRgn(port));
    } else {
        GWorldContainerSetPort(&buttonPort);
        ForeColor(blackColor);
        box = *bounds;
        mid = box;
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&mid, -mid.left, -mid.top);
        CopyBits(GWorldContainerBits(&loadPort), (BitMapPtr)GWorldContainerBits(&buttonPort),
                 bounds, &box, 0, 0);
        if (state == 0 || state == kButtonDim) {
            piece = buttonPieceRect[mode * 3];
            OffsetRect(&piece, -piece.left, -piece.top);
            OffsetRect(&piece, box.left, box.top);
            piece.bottom = box.bottom;
#ifdef CEFIX /* F12.75 */
            piece.right = piece.left + CE_Scale(piece.right - piece.left);
#endif
            CopyMask(GWorldContainerBits(&buttonImagePort), GWorldContainerBits(&buttonMaskPort),
                     GWorldContainerBits(&buttonPort), &buttonPieceRect[mode * 3],
                     &buttonPieceRect[mode * 3], &piece);
            mid.left = piece.right;
            piece = buttonPieceRect[mode * 3 + 2];
            OffsetRect(&piece, -piece.left, -piece.top);
            OffsetRect(&piece, box.right - piece.right, box.top);
            piece.bottom = box.bottom;
#ifdef CEFIX /* F12.76 */
            piece.left = piece.right - CE_Scale(piece.right - piece.left);
#endif
            CopyMask(GWorldContainerBits(&buttonImagePort), GWorldContainerBits(&buttonMaskPort),
                     GWorldContainerBits(&buttonPort), &buttonPieceRect[mode * 3 + 2],
                     &buttonPieceRect[mode * 3 + 2], &piece);
            mid.right = piece.left;
            CopyBits(GWorldContainerBits(&buttonImagePort), (BitMapPtr)GWorldContainerBits(&buttonPort),
                     &buttonPieceRect[mode * 3 + 1], &mid, 0, 0);
            RGBForeColor(&buttonColor[mode]);
            glyph = label[1];
            if (glyph == kGlyphCaret) {
                size = (box.bottom - box.top) / 10;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 - size);
                Line(size * -2, size * 2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 - size);
                Line(size * 2, size * 2);
                PenSize(1, 1);
            } else if (glyph == kGlyphAmp) {
                size = (box.bottom - box.top) / 10;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 + size);
                Line(size * -2, size * -2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 + size);
                Line(size * 2, size * -2);
                PenSize(1, 1);
            } else if (glyph == kGlyphPlus) {
                size = (box.bottom - box.top) / 8;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2 - size, (box.bottom + box.top) / 2);
                Line(size * 2, 0);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 - size);
                Line(0, size * 2);
                PenSize(1, 1);
            } else if (glyph == kGlyphMinus) {
                size = (box.bottom - box.top) / 9;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2 - size, (box.bottom + box.top) / 2);
                Line(size * 2, 0);
                PenSize(1, 1);
            } else {
                TextFont(buttonFontID);
                TextSize(buttonFontSize);
#ifdef CEFIX /* F12.77 */
                CE_SetPen((box.left + box.right) / 2 - StringWidth(label) / 2,
                          (box.top + box.bottom) / 2 + CE_Scale(5));
#else
                MoveTo((box.left + box.right) / 2 - StringWidth(label) / 2,
                       (box.top + box.bottom) / 2 + 5);
#endif
                DrawString(label);
            }
        }
        SetPort((GrafPtr)savedPort);
        ForeColor(blackColor);
        CopyBits(GWorldContainerBits(&buttonPort), GetPortBitMapForCopyBits(savedPort),
                 &box, bounds, 0, GetPortVisRgn(port));
    }
}

#ifndef MACBUILD
/* CrtInitHook6. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a3a90
void CrtInitHook6(void)
{
}
#endif

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
#endif
