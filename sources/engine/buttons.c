// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
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

/* Toolbox shims (win port implementations) */

/* win-only: pushes the dialog's offscreen to the window between repaints */

/* rest of this TU */

/* set when the branch below hides the planet-comm dialog's third button; the
   name is read off that effect, not from the Nova Bible */
#define kSpobTributeDisabled 0x0020

/* set when the comm dialog swaps its third item for item 4; likewise named
   from that effect rather than from the Nova Bible */
#define kGovtAltCommItem 0x0001

#define kButtonPictID         7500
#define kButtonMaskPictID     7600

/* button slots: the index into a Track/Draw pair's Rect array */
enum { kTradeDoneButton, kTradeBuyButton, kTradeSellButton };
enum { kMissionAcceptButton, kMissionOkButton };
enum { kMissionInfoDoneButton, kMissionInfoAbortButton };
enum { kHaggleYesButton, kHaggleNoButton };

/* the button object's globals; this file ends that object (the .CRT hook).
   mapButtonTable's sixth slot is shipyardButtonTable[0], as in the exe */
GWorldContainer buttonPort;
GWorldContainer buttonImagePort;
GWorldContainer buttonMaskPort;
short mapButtonTable[5];
short shipyardButtonTable[5];
short outfitButtonTable[5];
short portButtonID[kPortButtons];
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
unsigned char DAT_007dc2aa[261];

/* TrackTradeButtons. STATE: MATCH 100%, exe 158 / obj 158, byte-identical.
   The slot enum (-enum min) at the GetDialogItem sites keeps their addresses out of the CSE
   class the PtInRect sites share. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004a04d0
short TrackTradeButtons(Point where)
{
    short drawnButton;
    short overButton;
    short downH;
    Rect box[3];
    short itemType;
    void *item;
    short downButton;
    Point mouse;

    RGBForeColor(blackColor);
    GetDialogItem(tradeDialog, 1, &itemType, &item, &box[kTradeDoneButton]);
    GetDialogItem(tradeDialog, 0xd, &itemType, &item, &box[kTradeBuyButton]);
    GetDialogItem(tradeDialog, 0xe, &itemType, &item, &box[kTradeSellButton]);
    drawnButton = -1;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    if (PtInRect(downH, downButton, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRect(downH, downButton, &box[1]) != 0)
        drawnButton = 1;
    if (PtInRect(downH, downButton, &box[2]) != 0)
        drawnButton = 2;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawTradeButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRect(mouse.h, mouse.v, &box[0]) != 0)
                overButton = 0;
            if (PtInRect(mouse.h, mouse.v, &box[1]) != 0
                    && CanBuyGoods(selTradeItem) != 0)
                overButton = 1;
            if (PtInRect(mouse.h, mouse.v, &box[2]) != 0
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
void DrawTradeButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect box[3];
    char enabled[3];
    void *item;
    short itemType;

    RGBForeColor(blackColor);
    GetDialogItem(tradeDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(tradeDialog, 0xd, &itemType, &item, &box[1]);
    GetDialogItem(tradeDialog, 0xe, &itemType, &item, &box[2]);
    enabled[0] = 1;
    enabled[1] = CanBuyGoods(selTradeItem);
    enabled[2] = CanSellGoods(selTradeItem);
    i = 0;
    tradeButtonTable[0] = 4;
    tradeButtonTable[1] = 1;
    tradeButtonTable[2] = 2;
    walk = box;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(tradeDialog), walk,
                       buttonLabel[tradeButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(tradeDialog), walk,
                       buttonLabel[tradeButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(tradeDialog), walk,
                       buttonLabel[tradeButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 3);
}

/* TrackCapAssignButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0810
short TrackCapAssignButtons(Point where)
{
    Rect *walk;
    Rect *r;
    int downH;
    int overButton;
    int i;
    char down;
    int drawnButton;
    int over;
    Rect box[2];
    short itemType;
    void *item;
    Rect *first;
    Point mouse;
    int downButton;

    i = 0;
    walk = box;
    do {
        GetDialogItem(captureDialog, (short)i + 1, &itemType, &item, walk);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 1);
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0) {
            drawnButton = overButton;
            break;
        }
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton <= 1);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawCapAssignButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        if (down != '\0') {
            do {
                BusyIdle();
                over = -1;
                GetMouse(&mouse);
                overButton = 0;
                walk = first;
                do {
                    if (PtInRect(mouse.h, mouse.v, walk) != 0) {
                        over = overButton;
                        break;
                    }
                    overButton = overButton + 1;
                    walk = walk + 1;
                } while ((short)overButton <= 1);
                if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                    over = -1;
                if ((short)over != (short)drawnButton) {
                    DrawCapAssignButtons(over);
                    nv_UpdateDialogWindow(0);
                }
                drawnButton = over;
                down = StillDown();
            } while (down != '\0');
        }
    }
    return drawnButton;
}

/* DrawCapAssignButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0950
void DrawCapAssignButtons(short hilited)
{
    int i;
    Rect box;
    short itemType;
    void *item;

    capAssignButtonTable[0] = 0x2e;
    capAssignButtonTable[1] = 0x2d;
    i = 0;
    do {
        GetDialogItem(captureDialog, (short)i + 1, &itemType, &item, &box);
        if ((short)i == hilited)
            DrawButton(GetDialogPort(captureDialog), &box,
                       buttonLabel[capAssignButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(captureDialog), &box,
                       buttonLabel[capAssignButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
    } while ((short)i <= 1);
}

/* TrackCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a09e0
short TrackCommDialogButtons(Point where)
{
    char altItem;
    Rect *walk;
    Rect *r;
    int downH;
    int overButton;
    int i;
    char down;
    int drawnButton;
    int over;
    Rect box[3];
    short itemType;
    void *item;
    Rect *first;
    Point mouse;
    int downButton;

    altItem = 0;
    if (commShip != 0 && commShip->govt != -1
            && (govtData[commShip->govt].flags2 & kGovtAltCommItem) != 0)
        altItem = 1;
    if (altItem != 0) {
        GetDialogItem(commDialog, 1, &itemType, &item, &box[0]);
        GetDialogItem(commDialog, 4, &itemType, &item, &box[1]);
        GetDialogItem(commDialog, 2, &itemType, &item, &box[2]);
    } else {
        i = 0;
        walk = box;
        do {
            GetDialogItem(commDialog, (short)i + 1, &itemType, &item, walk);
            i = i + 1;
            walk = walk + 1;
        } while ((short)i <= 2);
    }
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0) {
            drawnButton = overButton;
            break;
        }
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton <= 2);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawCommDialogButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        if (down != '\0') {
            do {
                BusyIdle();
                over = -1;
                GetMouse(&mouse);
                overButton = 0;
                walk = first;
                do {
                    if (PtInRect(mouse.h, mouse.v, walk) != 0) {
                        over = overButton;
                        break;
                    }
                    overButton = overButton + 1;
                    walk = walk + 1;
                } while ((short)overButton <= 2);
                if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                    over = -1;
                if ((short)over != (short)drawnButton) {
                    DrawCommDialogButtons((short)over);
                    nv_UpdateDialogWindow(0);
                }
                drawnButton = over;
                down = StillDown();
            } while (down != '\0');
        }
    }
    return drawnButton;
}

/* DrawCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0bc0
void DrawCommDialogButtons(short hilited)
{
    Rect box[3];
    char altItem;
    char hasMission;
    short itemType;
    void *item;
    int i;
    Rect *walk;
    Rect *fill;
    char enabled[4];

    RGBForeColor(blackColor);
    altItem = 0;
    if (commShip != 0 && commShip->govt != -1
            && (govtData[commShip->govt].flags2 & kGovtAltCommItem) != 0)
        altItem = 1;
    if (altItem == 0) {
        i = 0;
        walk = box;
        fill = walk;
        do {
            GetDialogItem(commDialog, (short)i + 1, &itemType, &item, fill);
            i = i + 1;
            fill = fill + 1;
        } while ((short)i <= 2);
    } else {
        GetDialogItem(commDialog, 1, &itemType, &item, &box[0]);
        GetDialogItem(commDialog, 4, &itemType, &item, &box[1]);
        GetDialogItem(commDialog, 2, &itemType, &item, &box[2]);
    }
    hasMission = 0;
    if (commShip != 0 && commShip->specialMission != -1
            && currentMissionObjectives[commShip->specialMission].active != 0
            && currentMissions[commShip->specialMission].shipBehav == 1)
        hasMission = 1;
    commButtonTable[0] = 0x14;
    enabled[0] = 1;
    enabled[2] = 1;
    commButtonTable[2] = 0x15;
    if (altItem != 0) {
        enabled[1] = 0;
    } else {
        enabled[1] = 1;
        if (commShip != 0) {
            if (ExtendedIsThreatToPlayer(commShip) != 0)
                commButtonTable[1] = 0x18;
            else if (commShip->parentShip == 0 && commShip->aiType == 6 &&
                     hasMission == 0)
                commButtonTable[1] = 0x1f;
            else
                commButtonTable[1] = 0x16;
        } else {
            commButtonTable[1] = 0x16;
        }
    }
    i = 0;
    walk = box;
    do {
        if (enabled[(short)i] != 0) {
            if ((short)i == hilited)
                DrawButton(GetDialogPort(commDialog), walk,
                           buttonLabel[commButtonTable[(short)i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(commDialog), walk,
                           buttonLabel[commButtonTable[(short)i]], 0, 0, 0);
        }
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 2);
}

/* TrackPlanetCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0e50
short TrackPlanetCommDialogButtons(Point where)
{
    Rect *walk;
    Rect *r;
    int downH;
    int overButton;
    int i;
    char down;
    int drawnButton;
    int over;
    Rect box[4];    /* four slots reserved; only the three items are tracked */
    short itemType;
    void *item;
    Rect *first;
    Point mouse;
    int downButton;

    i = 0;
    walk = box;
    do {
        GetDialogItem(commDialog, (short)i + 1, &itemType, &item, walk);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 2);
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0) {
            drawnButton = overButton;
            break;
        }
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton <= 2);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawPlanetCommDialogButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        if (down != '\0') {
            do {
                BusyIdle();
                over = -1;
                GetMouse(&mouse);
                overButton = 0;
                walk = first;
                do {
                    if (PtInRect(mouse.h, mouse.v, walk) != 0) {
                        over = overButton;
                        break;
                    }
                    overButton = overButton + 1;
                    walk = walk + 1;
                } while ((short)overButton <= 2);
                if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                    over = -1;
                if ((short)over != (short)drawnButton) {
                    DrawPlanetCommDialogButtons(over);
                    nv_UpdateDialogWindow(0);
                }
                drawnButton = over;
                down = StillDown();
            } while (down != '\0');
        }
    }
    return drawnButton;
}

/* DrawPlanetCommDialogButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0f90
void DrawPlanetCommDialogButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect *fill;
    short stellar;
    char enabled[3];
    short itemType;
    Rect box[3];
    void *item;

    RGBForeColor(blackColor);
    i = 0;
    walk = box;
    fill = walk;
    do {
        GetDialogItem(commDialog, (short)i + 1, &itemType, &item, fill);
        i = i + 1;
        fill = fill + 1;
    } while ((short)i < 3);
    planetCommButtonTable[0] = 0x14;
    enabled[0] = 1;
    enabled[1] = 1;
    if (hatesPlayer != 0)
        planetCommButtonTable[1] = 0x17;
    else
        planetCommButtonTable[1] = 0x15;
    stellar = shipData[0].targetStellar;
    if (stellarData[stellar].dominated != 0) {
        if ((stellarData[stellar].flags2 & kSpobTributeDisabled) != 0) {
            enabled[2] = 0;
        } else {
            enabled[2] = 1;
            planetCommButtonTable[2] = 0x1f;
        }
    } else {
        enabled[2] = 1;
        planetCommButtonTable[2] = 0x2c;
    }
    i = 0;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(commDialog), walk,
                       buttonLabel[planetCommButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(commDialog), walk,
                       buttonLabel[planetCommButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(commDialog), walk,
                       buttonLabel[planetCommButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 3);
    RGBForeColor(blackColor);
}

/* TrackMissionButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1130
short TrackMissionButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    short downV;
    short downH;
    Rect box[2];
    short itemType;
    void *item;
    Point mouse;

    RGBForeColor(blackColor);
    GetDialogItem(missionDialog, 1, &itemType, &item, &box[kMissionAcceptButton]);
    GetDialogItem(missionDialog, 7, &itemType, &item, &box[kMissionOkButton]);
    drawnButton = -1;
    downV = *(short *)&where;
    downH = ((short *)&where)[1];
    if (PtInRect(downH, downV, &box[0]) != 0 && selMissionListItem != -1)
        drawnButton = 0;
    else if (PtInRect(downH, downV, &box[1]) != 0)
        drawnButton = 1;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawMissionButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        if (StillDown() != 0) {
            do {
                BusyIdle();
                GetMouse(&mouse);
                overButton = -1;
                if (PtInRect(mouse.h, mouse.v, &box[0]) != 0
                        && selMissionListItem != -1)
                    overButton = 0;
                else if (PtInRect(mouse.h, mouse.v, &box[1]) != 0)
                    overButton = 1;
                if (overButton != drawnButton && overButton != downButton)
                    overButton = -1;
                if (overButton != drawnButton) {
                    DrawMissionButtons(overButton);
                    nv_UpdateDialogWindow(0);
                }
                drawnButton = overButton;
            } while (StillDown() != 0);
        }
    }
    return drawnButton;
}

/* DrawMissionButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1290
void DrawMissionButtons(short hilited)
{
    unsigned char *label;
    Rect *walk;
    short i;
    Rect box[2];
    char enabled[2];
    void *item;
    short itemType;

    RGBForeColor(blackColor);
    GetDialogItem(missionDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(missionDialog, 7, &itemType, &item, &box[1]);
    missionButtonTable[0] = 0x19;
    missionButtonTable[1] = 0;
    if (selMissionListItem == -1)
        enabled[0] = '\0';
    else
        enabled[0] = '\x01';
    i = 0;
    enabled[1] = 1;
    walk = box;
    do {
        if (enabled[i] == '\0') {
            label = buttonLabel[missionButtonTable[i]];
            DrawButton(GetDialogPort(missionDialog), walk, label, -1, 0, 0);
        } else if (i == hilited) {
            label = buttonLabel[missionButtonTable[i]];
            DrawButton(GetDialogPort(missionDialog), walk, label, 0, 1, 0);
        } else {
            label = buttonLabel[missionButtonTable[i]];
            DrawButton(GetDialogPort(missionDialog), walk, label, 0, 0, 0);
        }
        i = i + 1;
        walk = walk + 1;
    } while (i <= 1);
}

/* TrackMissionInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a13c0
short TrackMissionInfoButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    short downV;
    short downH;
    Rect box[2];
    short itemType;
    void *item;
    Point mouse;

    RGBForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 1, &itemType, &item, &box[kMissionInfoDoneButton]);
    GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box[kMissionInfoAbortButton]);
    drawnButton = -1;
    downV = *(short *)&where;
    downH = ((short *)&where)[1];
    if (PtInRect(downH, downV, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRect(downH, downV, &box[1]) != 0 && selMissionListItem != -1)
        drawnButton = 1;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawMissionInfoButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != '\0') {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRect(mouse.h, mouse.v, &box[0]) != 0)
                overButton = 0;
            if (PtInRect(mouse.h, mouse.v, &box[1]) != 0 && selMissionListItem != -1)
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
void DrawMissionInfoButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect box[2];
    char enabled[2];
    void *item;
    short itemType;

    RGBForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box[1]);
    enabled[0] = 1;
    if (selMissionListItem == -1 || showAbortMissionButton == 0)
        enabled[1] = 0;
    else
        enabled[1] = 1;
    i = 0;
    missionInfoButtonTable[0] = 4;
    missionInfoButtonTable[1] = 0x22;
    walk = box;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(missionInfoDialog), walk,
                       buttonLabel[missionInfoButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(missionInfoDialog), walk,
                       buttonLabel[missionInfoButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(missionInfoDialog), walk,
                       buttonLabel[missionInfoButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 2);
}

/* TrackSingleMissionButtons. STATE: MATCH 100%, 146/146, byte-identical. Every loop indexes
   box[], `where' is read in place and the tracking loop tests StillDown() directly.
   OPEN: none */
// FUNCTION: EVNOVA 0x004a1670
short TrackSingleMissionButtons(Point where)
{
    int drawnButton;
    int over;
    int overButton;
    int i;
    Rect box[4];
    short itemType;
    void *item;
    Point mouse;
    int downButton;

    RGBForeColor(blackColor);
    if (canRefuseMission != 0) {
        for (i = 0; (short)i <= 1; i++)
            GetDialogItem(missionDialog, (short)i + 1, &itemType, &item, &box[i]);
    } else {
        GetDialogItem(missionDialog, 6, &itemType, &item, &box[0]);
        SetRect(&box[1], -1, -1, 0, 0);
    }
    for (overButton = 0; (short)overButton <= 1; overButton++)
        GetDialogItem(missionDialog, (short)overButton + 9, &itemType, &item,
                      &box[(short)overButton + 2]);
    drawnButton = -1;
    for (overButton = 0; (short)overButton <= 3; overButton++) {
        if (PtInRect(where.h, where.v, &box[overButton]) != 0)
            drawnButton = overButton;
    }
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawSingleMissionButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown()) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            for (overButton = 0; (short)overButton <= 3; overButton++) {
                if (PtInRect(mouse.h, mouse.v, &box[overButton]) != 0)
                    over = overButton;
            }
            if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                over = -1;
            if ((short)over != (short)drawnButton) {
                DrawSingleMissionButtons(over);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = over;
        }
    }
    return drawnButton;
}

/* DrawSingleMissionButtons (mac _DrawSingleMissionButtons). STATE: MATCH 100.0%, exe 191 /
   obj 191, relocregcheck 0 of 41; `int j' is declared last. OPEN: none */
// FUNCTION: EVNOVA 0x004a1820
void DrawSingleMissionButtons(short hilited)
{
    unsigned char *label;
    short itemType;
    int i;
    void *item;
    Rect box[4];
    short dimmed[4];
    Rect *walk;
    Rect *fill;
    Rect *walk2;
    int j;

    RGBForeColor(blackColor);
    singleMissionButtonTable[2] = 0x12;
    singleMissionButtonTable[3] = 0x13;
    if (canScrollTextUp == 0 && canScrollTextDown == 0) {
        dimmed[2] = -2;
        dimmed[3] = -2;
    } else {
        if (canScrollTextUp != 0)
            dimmed[2] = 0;
        else
            dimmed[2] = -1;
        if (canScrollTextDown != 0)
            dimmed[3] = 0;
        else
            dimmed[3] = -1;
    }
    GetDialogItem(missionDialog, 9, &itemType, &item, &box[2]);
    GetDialogItem(missionDialog, 10, &itemType, &item, &box[3]);
    if (canRefuseMission) {
        i = 0;
        walk2 = box;
        fill = walk2;
        do {
            GetDialogItem(missionDialog, (short)i + 1, &itemType, &item, fill);
            i = i + 1;
            fill = fill + 1;
        } while ((short)i <= 1);
        j = 0;
        singleMissionButtonTable[0] = 0x31;
        singleMissionButtonTable[1] = 0x32;
        dimmed[0] = 0;
        dimmed[1] = 0;
        label = missionButtonLabel[0];
        do {
            if (hilited == (short)j)
                DrawButton(GetDialogPort(missionDialog), walk2, label, dimmed[(short)j], 1, 0);
            else
                DrawButton(GetDialogPort(missionDialog), walk2, label, dimmed[(short)j], 0, 0);
            j = j + 1;
            label = label + 256;
            walk2 = walk2 + 1;
        } while ((short)j <= 1);
    } else {
        GetDialogItem(missionDialog, 6, &itemType, &item, &box[0]);
        GetDialogItem(missionDialog, 6, &itemType, &item, &box[1]);
        singleMissionButtonTable[0] = 0x1a;
        dimmed[0] = 0;
        dimmed[1] = -2;
        if (hilited == 0) {
            label = missionButtonLabel[0];
            DrawButton(GetDialogPort(missionDialog), &box[0], label, dimmed[0], 1, 0);
        } else {
            label = missionButtonLabel[0];
            DrawButton(GetDialogPort(missionDialog), &box[0], label, dimmed[0], 0, 0);
        }
    }
    i = 2;
    walk = &box[2];
    do {
        if (hilited == (short)i) {
            label = buttonLabel[singleMissionButtonTable[(short)i]];
            DrawButton(GetDialogPort(missionDialog), walk, label, dimmed[(short)i], 1, 0);
        } else {
            label = buttonLabel[singleMissionButtonTable[(short)i]];
            DrawButton(GetDialogPort(missionDialog), walk, label, dimmed[(short)i], 0, 0);
        }
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 3);
}

/* TrackPlayerInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1ae0
short TrackPlayerInfoButtons(short page, Point where)
{
    Rect *walk;
    Rect *r;
    int downH;
    int overButton;
    int i;
    char down;
    int drawnButton;
    int over;
    Rect box[6];
    short itemType;
    void *item;
    Rect *first;
    Point mouse;
    int downButton;

    RGBForeColor(blackColor);
    i = 0;
    walk = box;
    do {
        GetDialogItem(infoDialog, (short)i + 1, &itemType, &item, walk);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 4);
    GetDialogItem(infoDialog, 7, &itemType, &item, &box[5]);
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0) {
            drawnButton = overButton;
            break;
        }
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton <= 5);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawPlayerInfoButtons(page, drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        while (down != 0) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            overButton = 0;
            walk = first;
            do {
                if (PtInRect(mouse.h, mouse.v, walk) != 0) {
                    over = overButton;
                    break;
                }
                overButton = overButton + 1;
                walk = walk + 1;
            } while ((short)overButton <= 5);
            if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                over = -1;
            if ((short)over != (short)drawnButton) {
                DrawPlayerInfoButtons(page, (short)over);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = over;
            down = StillDown();
        }
    }
    return drawnButton;
}

/* DrawPlayerInfoButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1c40
void DrawPlayerInfoButtons(short page, short hilited)
{
    unsigned char *label;
    short itemType;
    int i;
    void *item;
    Rect box[6];
    char enabled[6];
    int j;
    Rect *walk;
    Rect *fill;

    i = 0;
    walk = box;
    playerInfoButtonTable[0] = 4;
    playerInfoButtonTable[1] = 0x23;
    playerInfoButtonTable[2] = 0x24;
    playerInfoButtonTable[3] = 0x25;
    playerInfoButtonTable[4] = 0x26;
    playerInfoButtonTable[5] = 0x3c;
    fill = walk;
    do {
        enabled[(short)i] = '\x01';
        GetDialogItem(infoDialog, (short)i + 1, &itemType, &item, fill);
        i = i + 1;
        fill = fill + 1;
    } while ((short)i <= 4);
    GetDialogItem(infoDialog, 7, &itemType, &item, &box[5]);
    if (page == 2)
        enabled[5] = PlayerHasJettisonableCargo();
    else
        enabled[5] = 0;
    RGBForeColor(blackColor);
    j = 0;
    do {
        if (enabled[(short)j] != '\0') {
            if (hilited == (short)j || page == (short)j) {
                label = buttonLabel[playerInfoButtonTable[(short)j]];
                DrawButton(GetDialogPort(infoDialog), walk, label, 0, 1, 0);
            } else {
                label = buttonLabel[playerInfoButtonTable[(short)j]];
                DrawButton(GetDialogPort(infoDialog), walk, label, 0, 0, 0);
            }
        }
        j = j + 1;
        walk = walk + 1;
    } while ((short)j <= 5);
}

/* TrackEscortButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1d90
short TrackEscortButtons(Point where)
{
    Rect *walk;
    Rect *r;
    int downH;
    int overButton;
    int i;
    char down;
    int drawnButton;
    int over;
    Rect box[4];
    short itemType;
    void *item;
    Rect *first;
    Point mouse;
    int downButton;

    RGBForeColor(blackColor);
    i = 0;
    walk = box;
    do {
        GetDialogItem(commDialog, (short)i + 1, &itemType, &item, walk);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 3);
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0) {
            drawnButton = overButton;
            break;
        }
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton <= 3);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawEscortButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        while (down != 0) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            overButton = 0;
            walk = first;
            do {
                if (PtInRect(mouse.h, mouse.v, walk) != 0) {
                    over = overButton;
                    break;
                }
                overButton = overButton + 1;
                walk = walk + 1;
            } while ((short)overButton <= 3);
            if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                over = -1;
            if ((short)over != (short)drawnButton) {
                DrawEscortButtons((short)over);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = over;
            down = StillDown();
        }
    }
    return drawnButton;
}

/* DrawEscortButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a1ed0
void DrawEscortButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect *fill;
    Rect box[4];
    short itemType;
    void *item;
    char enabled[4];

    RGBForeColor(blackColor);
    i = 0;
    walk = box;
    fill = walk;
    do {
        GetDialogItem(commDialog, (short)i + 1, &itemType, &item, fill);
        i = i + 1;
        fill = fill + 1;
    } while ((short)i < 4);
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
    escortButtonTable[0] = 0x14;
    escortButtonTable[1] = 0x1f;
    if (commShip == 0) {
        escortButtonTable[2] = 0x33;
        escortButtonTable[3] = 0x35;
    } else {
        if (commShip->upgradeOnLanding != 0)
            escortButtonTable[2] = 0x34;
        else
            escortButtonTable[2] = 0x33;
        if (commShip->sellOnLanding != 0)
            escortButtonTable[3] = 0x36;
        else
            escortButtonTable[3] = 0x35;
    }
    i = 0;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(commDialog), walk,
                       buttonLabel[escortButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(commDialog), walk,
                       buttonLabel[escortButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(commDialog), walk,
                       buttonLabel[escortButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 4);
}

/* TrackHaggleButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2090
short TrackHaggleButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    short downV;
    short downH;
    Rect box[2];
    short itemType;
    void *item;
    Point mouse;

    RGBForeColor(blackColor);
    GetDialogItem(haggleDialog, 1, &itemType, &item, &box[kHaggleYesButton]);
    GetDialogItem(haggleDialog, 2, &itemType, &item, &box[kHaggleNoButton]);
    drawnButton = -1;
    downV = *(short *)&where;
    downH = ((short *)&where)[1];
    if (PtInRect(downH, downV, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRect(downH, downV, &box[1]) != 0)
        drawnButton = 1;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawHaggleButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRect(mouse.h, mouse.v, &box[0]) != 0)
                overButton = 0;
            if (PtInRect(mouse.h, mouse.v, &box[1]) != 0)
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
void DrawHaggleButtons(short hilited)
{
    Rect *walk;
    short i;
    Rect box[2];
    short itemType;
    void *item;

    RGBForeColor(blackColor);
    GetDialogItem(haggleDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(haggleDialog, 2, &itemType, &item, &box[1]);
    if (haggleMode == 1) {
        haggleButtonTable[0] = 0x1b;
        haggleButtonTable[1] = 0x1c;
    } else {
        haggleButtonTable[0] = 0x1d;
        haggleButtonTable[1] = 0x1e;
    }
    i = 0;
    walk = box;
    do {
        if (i == hilited)
            DrawButton(GetDialogPort(haggleDialog), walk,
                       buttonLabel[haggleButtonTable[i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(haggleDialog), walk,
                       buttonLabel[haggleButtonTable[i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while (i <= 1);
}

/* TrackPlunderButtons. STATE: MATCH 100%, exe 157 / obj 157, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004a22e0
short TrackPlunderButtons(Point where)
{
    char down;
    int hit;
    int downH;
    int downV;
    int overButton;
    Rect *walk;
    Rect *r;
    Rect *fill;
    int i;
    Rect box[6];
    char enabled[6];
    void *item;
    Rect *first;
    Point mouse;
    short itemType;
    int drawnButton;

    RGBForeColor(blackColor);
    i = 0;
    fill = box;
    do {
        GetDialogItem(plunderDialog, (short)i + 1, &itemType, &item, fill);
        i = i + 1;
        fill = fill + 1;
    } while ((short)i <= 3);
    i = 4;
    fill = &box[4];
    do {
        GetDialogItem(plunderDialog, (short)i + 2, &itemType, &item, fill);
        i = i + 1;
        fill = fill + 1;
    } while ((short)i <= 5);
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
    hit = -1;
    overButton = 0;
    first = box;
    walk = box;
    downV = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (enabled[(short)overButton] != 0
                && PtInRect(downH, downV, walk) != 0)
            hit = overButton;
        overButton = overButton + 1;
        walk = walk + 1;
    } while ((short)overButton <= 5);
    drawnButton = hit;
    if ((short)hit != -1) {
        DrawPlunderButtons(hit);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        while (down != 0) {
            BusyIdle();
            GetMouse(&mouse);
            i = -1;
            overButton = 0;
            r = first;
            do {
                if (enabled[(short)overButton] != 0
                        && PtInRect(mouse.h, mouse.v, r) != 0)
                    i = overButton;
                overButton = overButton + 1;
                r = r + 1;
            } while ((short)overButton <= 5);
            if ((short)i != (short)hit && (short)i != (short)drawnButton)
                i = -1;
            if ((short)i != (short)hit) {
                DrawPlunderButtons(i);
                nv_UpdateDialogWindow(0);
            }
            hit = i;
            down = StillDown();
        }
    }
    return hit;
}

/* DrawPlunderButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a24e0
void DrawPlunderButtons(short hilited)
{
    Rect *walk;
    int i;
    Rect box[6];
    char enabled[6];
    void *item;
    short itemType;

    i = 0;
    walk = box;
    do {
        GetDialogItem(plunderDialog, (short)i + 1, &itemType, &item, &box[i]);
        i = i + 1;
    } while ((short)i <= 3);
    i = 4;
    do {
        GetDialogItem(plunderDialog, (short)i + 2, &itemType, &item, &box[i]);
        i = i + 1;
    } while ((short)i <= 5);
    plunderButtonTable[0] = 0x22;
    plunderButtonTable[1] = 0x27;
    plunderButtonTable[2] = 0x28;
    plunderButtonTable[3] = 0x29;
    plunderButtonTable[4] = 0x2a;
    plunderButtonTable[5] = 0x2b;
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
    RGBForeColor(blackColor);
    i = 0;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(plunderDialog), walk,
                       buttonLabel[plunderButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(plunderDialog), walk,
                       buttonLabel[plunderButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(plunderDialog), walk,
                       buttonLabel[plunderButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 5);
}

/* TrackBarButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a26e0
short TrackBarButtons(Point where)
{
    Rect *walk;
    Rect *r;
    int downH;
    int overButton;
    int i;
    char down;
    int drawnButton;
    int over;
    Rect box[6];
    short itemType;
    void *item;
    Rect *first;
    Point mouse;
    int downButton;

    RGBForeColor(blackColor);
    i = 0;
    walk = box;
    do {
        GetDialogItem(barDialog, (short)i + 1, &itemType, &item, walk);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 5);
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0)
            drawnButton = overButton;
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton <= 5);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawBarButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        while (down != 0) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            overButton = 0;
            walk = first;
            do {
                if (PtInRect(mouse.h, mouse.v, walk) != 0)
                    over = overButton;
                overButton = overButton + 1;
                walk = walk + 1;
            } while ((short)overButton <= 5);
            if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                over = -1;
            if ((short)over != (short)drawnButton) {
                DrawBarButtons(over);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = over;
            down = StillDown();
        }
    }
    return drawnButton;
}

/* DrawBarButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2810
void DrawBarButtons(short hilited)
{
    int i;
    int j;
    Rect *walk;
    Rect *fill;
    unsigned char *label;
    short hi;
    short dim;
    PicHandle icon;
    Rect box[6];
    char enabled[6];
    void *item;
    short itemType;

    i = 0;
    walk = box;
    fill = walk;
    do {
        GetDialogItem(barDialog, (short)i + 1, &itemType, &item, fill);
        i = i + 1;
        fill = fill + 1;
    } while ((short)i <= 5);
    enabled[0] = '\x01';
    enabled[1] = '\x01';
    enabled[2] = '\x01';
    enabled[3] = '\0';
    enabled[4] = CanHireEscorts();
    enabled[5] = '\0';
    barButtonTable[0] = 0;
    barButtonTable[1] = 10;
    barButtonTable[2] = 0xb;
    barButtonTable[4] = 0xc;
    RGBForeColor(blackColor);
    j = 0;
    do {
        if (enabled[(short)j] == '\0') {
            label = buttonLabel[barButtonTable[(short)j]];
            icon = 0;
            hi = 0;
            dim = -1;
            DrawButton(GetDialogPort(barDialog), walk, label, dim, hi, icon);
        } else if ((short)j == hilited) {
            label = buttonLabel[barButtonTable[(short)j]];
            icon = 0;
            hi = 1;
            dim = 0;
            DrawButton(GetDialogPort(barDialog), walk, label, dim, hi, icon);
        } else {
            label = buttonLabel[barButtonTable[(short)j]];
            icon = 0;
            hi = 0;
            dim = 0;
            DrawButton(GetDialogPort(barDialog), walk, label, dim, hi, icon);
        }
        j = j + 1;
        walk = walk + 1;
    } while ((short)j < 6);
}

/* TrackTextButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2960
short TrackTextButtons(Point where)
{
    int overButton;
    char down;
    int hit;
    Rect *walk;
    int i;
    int drawnButton;
    Rect *first;
    Rect box[3];
    short itemType;
    void *item;
    Point mouse;
    int downButton;

    RGBForeColor(blackColor);
    GetDialogItem(textDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(textDialog, 5, &itemType, &item, &box[1]);
    GetDialogItem(textDialog, 6, &itemType, &item, &box[2]);
    drawnButton = -1;
    overButton = 0;
    first = box;
    walk = box;
    downButton = *(short *)&where;
    i = ((short *)&where)[1];
    do {
        hit = PtInRect(i, downButton, walk);
        if (hit != 0)
            drawnButton = overButton;
        overButton = overButton + 1;
        walk = walk + 1;
    } while ((short)overButton < 3);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawTextButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        while (down != '\0') {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            i = 0;
            walk = first;
            do {
                hit = PtInRect(mouse.h, mouse.v, walk);
                if (hit != 0)
                    overButton = i;
                i = i + 1;
                walk = walk + 1;
            } while ((short)i < 3);
            if ((short)overButton != (short)drawnButton && (short)overButton != (short)downButton)
                overButton = -1;
            if ((short)overButton != (short)drawnButton) {
                DrawTextButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
            down = StillDown();
        }
    }
    return drawnButton;
}

/* DrawTextButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2ac0
void DrawTextButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect box[3];
    short dimmed[4];
    void *item;
    short itemType;

    GetDialogItem(textDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(textDialog, 5, &itemType, &item, &box[1]);
    GetDialogItem(textDialog, 6, &itemType, &item, &box[2]);
    textButtonTable[0] = 0x1a;
    textButtonTable[1] = 0x12;
    textButtonTable[2] = 0x13;
    dimmed[0] = 0;
    if (canScrollTextUp == 0 && canScrollTextDown == 0) {
        dimmed[1] = -2;
        dimmed[2] = -2;
    } else {
        if (canScrollTextUp != 0)
            dimmed[1] = 0;
        else
            dimmed[1] = -1;
        if (canScrollTextDown != 0)
            dimmed[2] = 0;
        else
            dimmed[2] = -1;
    }
    RGBForeColor(blackColor);
    i = 0;
    walk = box;
    do {
        if (hilited == (short)i)
            DrawButton(GetDialogPort(textDialog), walk,
                       buttonLabel[textButtonTable[(short)i]], dimmed[(short)i], 1, 0);
        else
            DrawButton(GetDialogPort(textDialog), walk,
                       buttonLabel[textButtonTable[(short)i]], dimmed[(short)i], 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 3);
}

/* DrawRaceButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2c30
void DrawRaceButtons(short hilited)
{
    PicHandle pic;
    short dim;
    int i;
    Rect *walk;
    Rect *fill;
    int racer;
    Rect box[6];
    short dimmed[6];
    void *item;
    short itemType;

    i = 0;
    walk = box;
    fill = walk;
    do {
        GetDialogItem(raceDialog, (short)i + 1, &itemType, &item, fill);
        dimmed[(short)i] = 0;
        i = i + 1;
        fill = fill + 1;
    } while ((short)i <= 1);
    if (raceWinner == -1 && raceCount == 0)
        raceButtonTable[0] = 0x21;
    else
        raceButtonTable[0] = 4;
    raceButtonTable[1] = 0x39;
    i = 0;
    do {
        racer = (short)i + 2;
        GetDialogItem(raceDialog, (short)i + 3, &itemType, &item, &box[racer]);
        i = i + 1;
        dimmed[racer] = -3;
        raceButtonTable[racer] = 0;
    } while ((short)i < 4);
    i = 0;
    do {
        dim = dimmed[(short)i];
        if (dim == -3) {
            racer = (short)i - 2;
            if (raceWinner == racer) {
                pic = raceButtonPic[racer][2];
            } else if (raceWinner != -1) {
                pic = raceButtonPic[racer][3];
            } else if (hilited == (short)i) {
                pic = raceButtonPic[racer][1];
            } else {
                pic = raceButtonPic[racer][0];
            }
        } else {
            pic = 0;
        }
        if (hilited == (short)i)
            DrawButton(GetDialogPort(raceDialog), walk,
                       buttonLabel[raceButtonTable[(short)i]], dim, 1, pic);
        else
            DrawButton(GetDialogPort(raceDialog), walk,
                       buttonLabel[raceButtonTable[(short)i]], dim, 0, pic);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 6);
}

/* TrackRaceButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a2de0
short TrackRaceButtons(Point where)
{
    char down;
    int drawnButton;
    int over;
    Rect *r;
    int downH;
    int overButton;
    Rect *walk;
    int i;
    Rect box[6];
    short itemType;
    Rect *first;
    void *item;
    Point mouse;
    int downButton;

    i = 0;
    walk = box;
    do {
        GetDialogItem(raceDialog, (short)i + 1, &itemType, &item, walk);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 1);
    overButton = 0;
    do {
        GetDialogItem(raceDialog, (short)overButton + 3, &itemType, &item,
                      &box[(short)overButton + 2]);
        overButton = overButton + 1;
    } while ((short)overButton < 4);
    drawnButton = -1;
    overButton = 0;
    first = box;
    r = box;
    downButton = *(short *)&where;
    downH = ((short *)&where)[1];
    do {
        if (PtInRect(downH, downButton, r) != 0) {
            drawnButton = overButton;
            break;
        }
        overButton = overButton + 1;
        r = r + 1;
    } while ((short)overButton < 6);
    downButton = drawnButton;
    if ((short)drawnButton != -1) {
        DrawRaceButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        down = StillDown();
        while (down != 0) {
            BusyIdle();
            GetMouse(&mouse);
            over = -1;
            overButton = 0;
            walk = first;
            do {
                if (PtInRect(mouse.h, mouse.v, walk) != 0) {
                    over = overButton;
                    break;
                }
                overButton = overButton + 1;
                walk = walk + 1;
            } while ((short)overButton < 6);
            if ((short)over != (short)drawnButton && (short)over != (short)downButton)
                over = -1;
            if ((short)over != (short)drawnButton) {
                DrawRaceButtons(over);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = over;
            down = StillDown();
        }
    }
    return drawnButton;
}

/* InitButtons. STATE: MATCH 100%, exe 264 / obj 264, byte-identical. The mask walker is
   set before the paint loop (the exe loads it there). OPEN: none */
// FUNCTION: EVNOVA 0x004a2f50
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
        if (buttonPic[n] == 0) {
            SetRect(&buttonPieceRect[n], 0, 0, 0xc, 0x18);
        } else {
            QDGetPictureBounds(buttonPic[n], &buttonPieceRect[n]);
            OffsetRect(&buttonPieceRect[n], -buttonPieceRect[n].left, -buttonPieceRect[n].top);
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
    bounds.top = 0;
    bounds.left = bounds.top;
    bounds.right = (buttonPieceRect[2].right > buttonPieceRect[5].right) ?
                    buttonPieceRect[2].right : buttonPieceRect[5].right;
    bounds.right = (bounds.right > buttonPieceRect[8].right) ?
                    bounds.right : buttonPieceRect[8].right;
    bounds.bottom = (buttonPieceRect[6].bottom > buttonPieceRect[7].bottom) ?
                     buttonPieceRect[6].bottom : buttonPieceRect[7].bottom;
    bounds.bottom = (bounds.bottom > buttonPieceRect[8].bottom) ?
                     bounds.bottom : buttonPieceRect[8].bottom;
    CreateGWorldContainer(&buttonImagePort, 0, gSpriteWorld->pixelDepth, &bounds);
    CreateGWorldContainer(&buttonMaskPort, 0, 1, &bounds);
    SetGWorldContainer(&buttonImagePort);
    RGBForeColor(blackColor);
    mask = buttonPieceRect;
    for (n = 0, piece = buttonPieceRect; n < kButtonPieceCount; n++) {
        PaintRect(piece);
        if (buttonPic[n] != 0)
            DrawPicHandle(buttonPic[n], piece);
        piece++;
    }
    UnsetGWorldContainer(&buttonImagePort);
    SetGWorldContainer(&buttonMaskPort);
    RGBForeColor(blackColor);
    for (n = 0; n < kButtonPieceCount; n++) {
        PaintRect(mask);
        if (buttonMaskPic[n] != 0)
            DrawPicHandle(buttonMaskPic[n], mask);
        mask++;
    }
    UnsetGWorldContainer(&buttonMaskPort);
    for (n = 0; n < kButtonPieceCount; n++) {
        if (buttonPic[n] != 0)
            QDReleaseResource(buttonPic[n]);
        if (buttonMaskPic[n] != 0)
            QDReleaseResource(buttonMaskPic[n]);
    }
    for (n = 0; n < kButtonLabelCount; n++)
        GetIndString(buttonLabel[n], kButtonStrList, n + 1);
    buttonColor[0].red = buttonColor[0].green = buttonColor[0].blue = 0xffff;
    buttonColor[1].red = buttonColor[1].green = buttonColor[1].blue = 0x8000;
    buttonColor[2].red = buttonColor[2].green = buttonColor[2].blue = 0x8000;
    SetRect(&bounds, 0, 0, 200, 0x32);
    CreateGWorldContainer(&buttonPort, 0, gSpriteWorld->pixelDepth, &bounds);
}

/* DisposeButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a3300
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


#define kButtonPicture (-3)
#define kButtonDim     (-1)

#define kGlyphCaret 0x5e
#define kGlyphAmp   0x26
#define kGlyphPlus  0x2b
#define kGlyphMinus 0x2d

/* DrawButton. STATE: MATCH 100%, exe 597 / obj 597. Every Rect copy goes through the inline
   CopyRect(), whose pointer operands stop mwcc reusing box.left for OffsetRect's argument.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004a3340
void DrawButton(CGrafPtr port, Rect *bounds, unsigned char *label,
                short state, short selected, PicHandle pic)
{
    Rect box;
    Rect piece;
    Rect mid;
    CGrafPtr savedPort;
    short mode;
    short size;
    long glyph;

    if (state == 0) {
        if (selected == 0)
            mode = 0;
        else
            mode = 1;
    } else
        mode = 2;
    GetPort(&savedPort);
    if (state == kButtonPicture) {
        SetGWorldContainer(&loadPort);
        RGBForeColor(blackColor);
        DrawPicHandle(pic, bounds);
        SetPort(savedPort);
        RGBForeColor(blackColor);
        CopyBits(GetGWCPixMap(&loadPort), GetPortBitMapForCopyBits(savedPort),
                 bounds, bounds, 0, GetPortVisRgn(port));
    } else {
        SetGWorldContainer(&buttonPort);
        RGBForeColor(blackColor);
        CopyRect(&box, bounds);
        CopyRect(&mid, &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&mid, -mid.left, -mid.top);
        CopyBits(GetGWCPixMap(&loadPort), (BitMapPtr)GetGWCPixMap(&buttonPort),
                 bounds, &box, 0, 0);
        if (state == kButtonDim || state == 0) {
            CopyRect(&piece, &buttonPieceRect[mode * 3]);
            OffsetRect(&piece, -piece.left, -piece.top);
            OffsetRect(&piece, box.left, box.top);
            piece.bottom = box.bottom;
            CopyMask(GetGWCPixMap(&buttonImagePort), GetGWCPixMap(&buttonMaskPort),
                     GetGWCPixMap(&buttonPort), &buttonPieceRect[mode * 3],
                     &buttonPieceRect[mode * 3], &piece);
            mid.left = piece.right;
            CopyRect(&piece, &buttonPieceRect[mode * 3 + 2]);
            OffsetRect(&piece, -piece.left, -piece.top);
            OffsetRect(&piece, box.right - piece.right, box.top);
            piece.bottom = box.bottom;
            CopyMask(GetGWCPixMap(&buttonImagePort), GetGWCPixMap(&buttonMaskPort),
                     GetGWCPixMap(&buttonPort), &buttonPieceRect[mode * 3 + 2],
                     &buttonPieceRect[mode * 3 + 2], &piece);
            mid.right = piece.left;
            CopyBits(GetGWCPixMap(&buttonImagePort), (BitMapPtr)GetGWCPixMap(&buttonPort),
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
                MoveTo((box.left + box.right) / 2 - StringWidth(label) / 2,
                       (box.top + box.bottom) / 2 + 5);
                DrawString(label);
            }
        }
        SetPort(savedPort);
        RGBForeColor(blackColor);
        CopyBits(GetGWCPixMap(&buttonPort), GetPortBitMapForCopyBits(savedPort),
                 &box, bounds, 0, GetPortVisRgn(port));
    }
}

/* CrtInitHook6. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a3a90
void CrtInitHook6(void)
{
}

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
