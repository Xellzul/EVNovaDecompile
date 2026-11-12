// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* portbuttons.c -- the spaceport's button pair, TrackPlanetButtons and DrawPlanetButtons. Notes: Research/notes/portbuttons.md, Research/notes/trackplanetbuttons.md */
/*
 * A disabled button draws kButtonDisabled, the selected one with the hilite
 * flag, and everything else plain.
 */
#include <stddef.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "ships.h"
#include "gametypes.h"
#include "stellar.h"

#define kLabelSlot      0x100


/* button slots: the index into the spaceport's Rect array */
enum { kPortLeaveButton, kPortRefuelButton, kPortTradeButton, kPortOutfitButton,
       kPortShipyardButton, kPortMissionsButton, kPortBarButton };
#ifdef MAC1010 /* 1.0.10 spelling: plain numbers at the sites */
#define kPortLeaveButton 0
#define kPortRefuelButton 1
#define kPortTradeButton 2
#define kPortOutfitButton 3
#define kPortShipyardButton 4
#define kPortMissionsButton 5
#define kPortBarButton 6
#endif

/* TrackPlanetButtons (mac _TrackPlanetButtons). STATE: MATCH 100.0%, exe 309 / obj 309
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0049fe10
// MAC: _TrackPlanetButtons i386 0x0002996f ppc 0x0002df5c
short TrackPlanetButtons(Point where)
{
    Rect box[kPortButtons];
    long conv;
    short itemType;
    Handle item;
    Point pt;
    short i;
    short hit;
    short orig;
    short over;
    Boolean noShips;

    ForeColor(blackColor);
    GetDialogItem(portDialog, 0xc, &itemType, &item, &box[kPortLeaveButton]);
    GetDialogItem(portDialog, 4, &itemType, &item, &box[kPortRefuelButton]);
    GetDialogItem(portDialog, 7, &itemType, &item, &box[kPortTradeButton]);
    GetDialogItem(portDialog, 8, &itemType, &item, &box[kPortOutfitButton]);
    GetDialogItem(portDialog, 9, &itemType, &item, &box[kPortShipyardButton]);
    GetDialogItem(portDialog, 0xa, &itemType, &item, &box[kPortMissionsButton]);
    GetDialogItem(portDialog, 0xb, &itemType, &item, &box[kPortBarButton]);

    noShips = (dockedPort->flags & kSpobUninhabited);
    hit = -1;
    for (i = 0; i <= kPortButtons - 1; i++) {
        if (PtInRectPoint(where, &box[i])) {
            if (i == 0)
                hit = i;
            if (i == 1) {
                if (noShips == 0 && shipData->credits > 0
                && shipData->fuel < ShipFuelCapacity(shipData))
                    hit = i;
            }
            if (i == 2) {
                if ((dockedPort->flags & kSpobTradeCenter) != 0)
                    hit = i;
            }
            if (i == 3) {
                if ((dockedPort->flags & kSpobHasOutfitter) != 0)
                    hit = i;
            }
            if (i == 4) {
                if ((dockedPort->flags & kSpobHasShipyard) != 0)
                    hit = i;
            }
            if (i == 5) {
                if (canDoMissions != 0)
                    hit = i;
            }
            if (i == 6) {
                if ((dockedPort->flags & kSpobHasBar) != 0)
                    hit = i;
            }
        }
    }

    orig = hit;
    if (hit != -1) {
        DrawPlanetButtons(hit);
        nv_UpdateDialogWindow(0);
        while (StillDown()) {
            BusyIdle();
            GetMouse(&pt);
            over = -1;
            if (PtInRectPoint(pt, &box[0]))
                over = 0;
            if (PtInRectPoint(pt, &box[1]) && noShips == 0
                && shipData->credits > 0
                && shipData->fuel < ShipFuelCapacity(shipData))
                over = 1;
            if (PtInRectPoint(pt, &box[2])
                && (dockedPort->flags & kSpobTradeCenter) != 0)
                over = 2;
            if (PtInRectPoint(pt, &box[3])
                && (dockedPort->flags & kSpobHasOutfitter) != 0)
                over = 3;
            if (PtInRectPoint(pt, &box[4])
                && (dockedPort->flags & kSpobHasShipyard) != 0)
                over = 4;
            if (PtInRectPoint(pt, &box[5]) && canDoMissions != 0)
                over = 5;
            if (PtInRectPoint(pt, &box[6])
                && (dockedPort->flags & kSpobHasBar) != 0)
                over = 6;
            if (over != hit && over != orig)
                over = -1;
            if (over != hit) {
                DrawPlanetButtons(over);
                nv_UpdateDialogWindow(0);
            }
            hit = over;
        }
    }
    return hit;
}

/* DrawPlanetButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0220
// MAC: _DrawPlanetButtons i386 0x0002968d ppc 0x0002dc84
void DrawPlanetButtons(short selected)
{
    Rect box[kPortButtons];
    Boolean live[kPortButtons];
    short itemType;
    Handle item;
    short i;
    Boolean noShips;

    ForeColor(blackColor);
    GetDialogItem(portDialog, 0xc, &itemType, &item, &box[0]);
    GetDialogItem(portDialog, 4, &itemType, &item, &box[1]);
    GetDialogItem(portDialog, 7, &itemType, &item, &box[2]);
    GetDialogItem(portDialog, 8, &itemType, &item, &box[3]);
    GetDialogItem(portDialog, 9, &itemType, &item, &box[4]);
    GetDialogItem(portDialog, 0xa, &itemType, &item, &box[5]);
    GetDialogItem(portDialog, 0xb, &itemType, &item, &box[6]);

    noShips = (dockedPort->flags & kSpobUninhabited);
    live[0] = 1;
    if (shipData->credits > 0 && noShips == 0
        && shipData->fuel < ShipFuelCapacity(shipData))
        live[1] = 1;
    else
        live[1] = 0;
    if ((dockedPort->flags & kSpobTradeCenter) != 0)
        live[2] = 1;
    else
        live[2] = 0;
    if ((dockedPort->flags & kSpobHasOutfitter) != 0)
        live[3] = 1;
    else
        live[3] = 0;
    if ((dockedPort->flags & kSpobHasShipyard) != 0)
        live[4] = 1;
    else
        live[4] = 0;
    live[5] = canDoMissions;
    if ((dockedPort->flags & kSpobHasBar) != 0)
        live[6] = 1;
    else
        live[6] = 0;

    planetButtonTable[0] = kLeaveButtonLabel;
    planetButtonTable[1] = kRechargeButtonLabel;
    planetButtonTable[2] = kTradeCenterButtonLabel;
    planetButtonTable[3] = kOutfitterButtonLabel;
    planetButtonTable[4] = kShipyardButtonLabel;
    planetButtonTable[5] = kMissionBBSButtonLabel;
    planetButtonTable[6] = kBarButtonLabel;

    for (i = 0; i <= kPortButtons - 1; i++) {
        if (live[i] != 0) {
            if (i == selected)
                DrawButton(GetDialogPort(portDialog), &box[i],
                           buttonLabel[planetButtonTable[i]], 0, 1, NULL);
            else
                DrawButton(GetDialogPort(portDialog), &box[i],
                           buttonLabel[planetButtonTable[i]], 0, 0, NULL);
        } else
            DrawButton(GetDialogPort(portDialog), &box[i],
                       buttonLabel[planetButtonTable[i]], kButtonDisabled, 0, NULL);
    }
}
