// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* portbuttons.c -- the spaceport's button pair, TrackPlanetButtons and DrawPlanetButtons. Notes: Research/notes/portbuttons.md, Research/notes/trackplanetbuttons.md */
/*
 * A disabled button draws with state -2, the selected one with the hilite
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
#define kButtonDisabled    -2


/* button slots: the index into the spaceport's Rect array */
enum { kPortLeaveButton, kPortRefuelButton, kPortOutfitButton, kPortShipyardButton,
       kPortTradeButton, kPortBarButton, kPortMissionsButton };

/* TrackPlanetButtons (mac _TrackPlanetButtons). STATE: MATCH 100.0%, exe 309 / obj 309
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0049fe10
short TrackPlanetButtons(Point where)
{
    Rect box[kPortButtons];
    char noShips;
    long conv;
    Point pt;
    void *item;
    short itemType;
    short i;
    short hit;
    short orig;
    short over;

    RGBForeColor(blackColor);
    GetDialogItem(portDialog, 0xc, &itemType, &item, &box[kPortLeaveButton]);
    GetDialogItem(portDialog, 4, &itemType, &item, &box[kPortRefuelButton]);
    GetDialogItem(portDialog, 7, &itemType, &item, &box[kPortOutfitButton]);
    GetDialogItem(portDialog, 8, &itemType, &item, &box[kPortShipyardButton]);
    GetDialogItem(portDialog, 9, &itemType, &item, &box[kPortTradeButton]);
    GetDialogItem(portDialog, 0xa, &itemType, &item, &box[kPortBarButton]);
    GetDialogItem(portDialog, 0xb, &itemType, &item, &box[kPortMissionsButton]);

    noShips = (dockedPort->flags & kSpobFlagNoShips);
    hit = -1;
    for (i = 0; i <= kPortButtons - 1; i++) {
        if (PtInRect(where.h, where.v, &box[i])) {
            if (i == 0)
                hit = i;
            if (i == 1 && noShips == 0 && shipData->credits > 0
                && ShipFuelCapacity(shipData) > shipData->fuel)
                hit = i;
            if (i == 2 && (dockedPort->flags & kSpobFlagOutfit) != 0)
                hit = i;
            if (i == 3 && (dockedPort->flags & kSpobFlagShipyard) != 0)
                hit = i;
            if (i == 4 && (dockedPort->flags & kSpobFlagTrade) != 0)
                hit = i;
            if (i == 5 && gSpobInhabited != 0)
                hit = i;
            if (i == 6 && (dockedPort->flags & kSpobFlagMissions) != 0)
                hit = i;
        }
    }

    orig = hit;
    if (hit != -1) {
        DrawPlanetButtons(hit);
        nv_UpdateDialogWindow(0);
        if (StillDown()) {
            do {
                BusyIdle();
                GetMouse(&pt);
                over = -1;
                if (PtInRect(pt.h, pt.v, &box[0]))
                    over = 0;
                if (PtInRect(pt.h, pt.v, &box[1]) && noShips == 0
                    && shipData->credits > 0
                    && ShipFuelCapacity(shipData) > shipData->fuel)
                    over = 1;
                if (PtInRect(pt.h, pt.v, &box[2])
                    && (dockedPort->flags & kSpobFlagOutfit) != 0)
                    over = 2;
                if (PtInRect(pt.h, pt.v, &box[3])
                    && (dockedPort->flags & kSpobFlagShipyard) != 0)
                    over = 3;
                if (PtInRect(pt.h, pt.v, &box[4])
                    && (dockedPort->flags & kSpobFlagTrade) != 0)
                    over = 4;
                if (PtInRect(pt.h, pt.v, &box[5]) && gSpobInhabited != 0)
                    over = 5;
                if (PtInRect(pt.h, pt.v, &box[6])
                    && (dockedPort->flags & kSpobFlagMissions) != 0)
                    over = 6;
                if (over != hit && over != orig)
                    over = -1;
                if (over != hit) {
                    DrawPlanetButtons(over);
                    nv_UpdateDialogWindow(0);
                }
                hit = over;
            } while (StillDown());
        }
    }
    return hit;
}

/* DrawPlanetButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a0220
void DrawPlanetButtons(short selected)
{
    Rect box[kPortButtons];
    char live[kPortButtons];
    short itemType;
    void *item;
    short i;
    char noShips;

    RGBForeColor(blackColor);
    GetDialogItem(portDialog, 0xc, &itemType, &item, &box[0]);
    GetDialogItem(portDialog, 4, &itemType, &item, &box[1]);
    GetDialogItem(portDialog, 7, &itemType, &item, &box[2]);
    GetDialogItem(portDialog, 8, &itemType, &item, &box[3]);
    GetDialogItem(portDialog, 9, &itemType, &item, &box[4]);
    GetDialogItem(portDialog, 0xa, &itemType, &item, &box[5]);
    GetDialogItem(portDialog, 0xb, &itemType, &item, &box[6]);

    noShips = (dockedPort->flags & kSpobFlagNoShips);
    live[0] = 1;
    if (shipData->credits > 0 && noShips == 0
        && ShipFuelCapacity(shipData) > shipData->fuel)
        live[1] = 1;
    else
        live[1] = 0;
    if ((dockedPort->flags & kSpobFlagOutfit) != 0)
        live[2] = 1;
    else
        live[2] = 0;
    if ((dockedPort->flags & kSpobFlagShipyard) != 0)
        live[3] = 1;
    else
        live[3] = 0;
    if ((dockedPort->flags & kSpobFlagTrade) != 0)
        live[4] = 1;
    else
        live[4] = 0;
    live[5] = gSpobInhabited;
    if ((dockedPort->flags & kSpobFlagMissions) != 0)
        live[6] = 1;
    else
        live[6] = 0;

    portButtonID[0] = 0;
    portButtonID[1] = 5;
    portButtonID[2] = 6;
    portButtonID[3] = 7;
    portButtonID[4] = 8;
    portButtonID[5] = 0xf;
    portButtonID[6] = 9;

    i = 0;
    do {
        if (live[i] == 0)
            DrawButton(GetDialogPort(portDialog), &box[i],
                       buttonLabel[portButtonID[i]], kButtonDisabled, 0, NULL);
        else if (i == selected)
            DrawButton(GetDialogPort(portDialog), &box[i],
                       buttonLabel[portButtonID[i]], 0, 1, NULL);
        else
            DrawButton(GetDialogPort(portDialog), &box[i],
                       buttonLabel[portButtonID[i]], 0, 0, NULL);
        i++;
    } while (i <= kPortButtons - 1);
}
