// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* shipyardbuttons.c -- the head of the buttons TU (win 0x0049eef0..0x0049fbb0), the three Draw/Track button pairs buttons.c does not yet carry: the map, the shipyard and the outfitter. Notes: Research/notes/shipyardbuttons.md */
#include "macport.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "player.h"
#include "stellar.h"
#include "burger.h"
#include "gametypes.h"

/* Toolbox shims (win port implementations) */

/* win-only: pushes the dialog's offscreen to the window between repaints */

/* rest of this TU */

#define kGridPageCells 20    /* the scroll-down guard's slack */
#define kGridScrollStep 4    /* ...and the scroll-up guard's */

/* button slots: the index into each Track/Draw pair's Rect array */
enum { kMapDoneButton, kMapZoomOutButton, kMapZoomInButton, kMapClearRouteButton,
       kMapInfluenceButton, kMapSearchButton };
enum { kShipyardDoneButton, kShipyardBuyButton, kShipyardInfoButton, kShipyardUpButton,
       kShipyardDownButton };
enum { kOutfitDoneButton, kOutfitBuyButton, kOutfitSellButton, kOutfitUpButton,
       kOutfitDownButton };

/* TrackMapButtons. STATE: MATCH 100%, exe 247 / obj 247, byte-identical.
   The GetDialogItem sites index `box' with the slot enum (-enum min), which keeps their
   addresses out of the CSE class the PtInRect sites share. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049eef0
short TrackMapButtons(Point where)
{
    Rect box[6];
    short drawnButton;
    short overButton;
    Point mouse;
    void *item;
    short itemType;
    short downButton;

    RGBForeColor(blackColor);
    GetDialogItem(mapDialog, 1, &itemType, &item, &box[kMapDoneButton]);
    GetDialogItem(mapDialog, 4, &itemType, &item, &box[kMapZoomOutButton]);
    GetDialogItem(mapDialog, 5, &itemType, &item, &box[kMapZoomInButton]);
    GetDialogItem(mapDialog, 8, &itemType, &item, &box[kMapClearRouteButton]);
    GetDialogItem(mapDialog, 9, &itemType, &item, &box[kMapInfluenceButton]);
    GetDialogItem(mapDialog, 0xa, &itemType, &item, &box[kMapSearchButton]);
    drawnButton = -1;
    if (PtInRect(where.h, where.v, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRect(where.h, where.v, &box[1]) != 0 && canZoomOut != 0)
        drawnButton = 1;
    if (PtInRect(where.h, where.v, &box[2]) != 0 && canZoomIn != 0)
        drawnButton = 2;
    if (PtInRect(where.h, where.v, &box[3]) != 0 && showClearRouteButton != 0)
        drawnButton = 3;
    if (PtInRect(where.h, where.v, &box[4]) != 0)
        drawnButton = 4;
    if (PtInRect(where.h, where.v, &box[5]) != 0)
        drawnButton = 5;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawMapButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRect(mouse.h, mouse.v, &box[0]) != 0)
                overButton = 0;
            if (PtInRect(mouse.h, mouse.v, &box[1]) != 0 && canZoomOut != 0)
                overButton = 1;
            if (PtInRect(mouse.h, mouse.v, &box[2]) != 0 && canZoomIn != 0)
                overButton = 2;
            if (PtInRect(mouse.h, mouse.v, &box[3]) != 0 && showClearRouteButton != 0)
                overButton = 3;
            if (PtInRect(mouse.h, mouse.v, &box[4]) != 0)
                overButton = 4;
            if (PtInRect(mouse.h, mouse.v, &box[5]) != 0)
                overButton = 5;
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawMapButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawMapButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049f1f0
void DrawMapButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect box[6];
    char enabled[6];
    void *item;
    short itemType;

    RGBForeColor(blackColor);
    GetDialogItem(mapDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(mapDialog, 4, &itemType, &item, &box[1]);
    GetDialogItem(mapDialog, 5, &itemType, &item, &box[2]);
    GetDialogItem(mapDialog, 8, &itemType, &item, &box[3]);
    GetDialogItem(mapDialog, 9, &itemType, &item, &box[4]);
    GetDialogItem(mapDialog, 0xa, &itemType, &item, &box[5]);
    enabled[0] = 1;
    enabled[1] = canZoomOut;
    enabled[2] = canZoomIn;
    enabled[3] = showClearRouteButton;
    enabled[4] = 1;
    enabled[5] = 1;
    mapButtonTable[0] = 4;
    mapButtonTable[1] = 0x10;
    mapButtonTable[2] = 0x11;
    mapButtonTable[3] = 0x30;
    if (gShowInfluence != 0)
        mapButtonTable[4] = 0x38;
    else
        mapButtonTable[4] = 0x37;
    mapButtonTable[5] = 0x3b;
    i = 0;
    walk = box;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(mapDialog), walk,
                       buttonLabel[mapButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(mapDialog), walk,
                       buttonLabel[mapButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(mapDialog), walk,
                       buttonLabel[mapButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 6);
}

/* TrackShipyardButtons. STATE: MATCH 100%, exe 245 / obj 245, byte-identical.
   `Rect box[5]' with the slot enum at the GetDialogItem sites (-enum min); unlike its
   siblings the buy button's Rect is taken once into `buyBox' ahead of the first pass.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049f3f0
short TrackShipyardButtons(Point where)
{
    Rect *buyBox;
    short drawnButton;
    short overButton;
    int downV;
    int downH;
    int i;
    short count;
    Rect box[5];
    Point mouse;
    void *item;
    short itemType;
    short downButton;

    RGBForeColor(blackColor);
    GetDialogItem(shipyardDialog, 7, &itemType, &item, &box[kShipyardDoneButton]);
    GetDialogItem(shipyardDialog, 1, &itemType, &item, &box[kShipyardBuyButton]);
    GetDialogItem(shipyardDialog, 0xa, &itemType, &item, &box[kShipyardInfoButton]);
    GetDialogItem(shipyardDialog, 0xc, &itemType, &item, &box[kShipyardUpButton]);
    GetDialogItem(shipyardDialog, 0xd, &itemType, &item, &box[kShipyardDownButton]);
    buyBox = &box[1];
    drawnButton = -1;
    downV = where.v;
    downH = where.h;
    if (PtInRect(downH, downV, buyBox) != 0 && canBuy != 0)
        drawnButton = 1;
    if (PtInRect(downH, downV, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRect(downH, downV, &box[2]) != 0 && gSelectedShip != -1)
        drawnButton = 2;
    if (PtInRect(downH, downV, &box[3]) != 0
            && gShipGridScroll >= kGridScrollStep)
        drawnButton = 3;
    if (PtInRect(downH, downV, &box[4]) != 0) {
        count = 0;
        i = 0;
        do {
            if (dockedPortShipTypes[(short)i] != -1)
                count++;
            i = i + 1;
        } while ((short)i < kMaxShipTypes);
        if (gShipGridScroll < count - kGridPageCells)
            drawnButton = 4;
    }
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawShipyardButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRect(mouse.h, mouse.v, buyBox) != 0 && canBuy != 0)
                overButton = 1;
            if (PtInRect(mouse.h, mouse.v, &box[0]) != 0)
                overButton = 0;
            if (PtInRect(mouse.h, mouse.v, &box[2]) != 0 && gSelectedShip != -1)
                overButton = 2;
            if (PtInRect(mouse.h, mouse.v, &box[3]) != 0
                    && gShipGridScroll >= kGridScrollStep)
                overButton = 3;
            if (PtInRect(mouse.h, mouse.v, &box[4]) != 0) {
                count = 0;
                i = 0;
                do {
                    if (dockedPortShipTypes[(short)i] != -1)
                        count++;
                    i = i + 1;
                } while ((short)i < kMaxShipTypes);
                if (gShipGridScroll < count - kGridPageCells)
                    overButton = 4;
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawShipyardButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawShipyardButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049f6f0
void DrawShipyardButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect box[5];
    char enabled[5];
    void *item;
    short itemType;
    short count;

    GetDialogItem(shipyardDialog, 7, &itemType, &item, &box[0]);
    GetDialogItem(shipyardDialog, 1, &itemType, &item, &box[1]);
    GetDialogItem(shipyardDialog, 0xa, &itemType, &item, &box[2]);
    GetDialogItem(shipyardDialog, 0xc, &itemType, &item, &box[3]);
    GetDialogItem(shipyardDialog, 0xd, &itemType, &item, &box[4]);
    shipyardButtonTable[0] = 4;
    if (shipBuyMode == 0)
        shipyardButtonTable[1] = 3;
    else
        shipyardButtonTable[1] = 0xc;
    shipyardButtonTable[2] = 0x2f;
    shipyardButtonTable[3] = 0x12;
    shipyardButtonTable[4] = 0x13;
    enabled[0] = 1;
    enabled[1] = canBuy;
    enabled[2] = gSelectedShip != -1;
    enabled[3] = gShipGridScroll >= kGridScrollStep;
    count = 0;
    i = 0;
    do {
        if (dockedPortShipTypes[(short)i] != -1)
            count++;
        i = i + 1;
    } while ((short)i < kMaxShipTypes);
    enabled[4] = gShipGridScroll < count - kGridPageCells;
    i = 0;
    walk = box;
    do {
        if (enabled[(short)i] == 0)
            DrawButton(GetDialogPort(shipyardDialog), walk,
                       buttonLabel[shipyardButtonTable[(short)i]], -1, 0, 0);
        else if ((short)i == hilited)
            DrawButton(GetDialogPort(shipyardDialog), walk,
                       buttonLabel[shipyardButtonTable[(short)i]], 0, 1, 0);
        else
            DrawButton(GetDialogPort(shipyardDialog), walk,
                       buttonLabel[shipyardButtonTable[(short)i]], 0, 0, 0);
        i = i + 1;
        walk = walk + 1;
    } while ((short)i < 5);
}

/* TrackOutfitButtons. STATE: MATCH 100%, exe 225 / obj 225, byte-identical.
   Same shape as TrackMapButtons: one `Rect box[5]', the slot enum at the GetDialogItem
   sites (-enum min). OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049f8f0
short TrackOutfitButtons(Point where)
{
    short downButton;
    short drawnButton;
    short overButton;
    int downV;
    int downH;
    short count;
    int i;
    Rect box[5];
    Point mouse;
    void *item;
    short itemType;

    RGBForeColor(blackColor);
    GetDialogItem(outfitDialog, 1, &itemType, &item, &box[kOutfitDoneButton]);
    GetDialogItem(outfitDialog, 7, &itemType, &item, &box[kOutfitBuyButton]);
    GetDialogItem(outfitDialog, 4, &itemType, &item, &box[kOutfitSellButton]);
    GetDialogItem(outfitDialog, 0xa, &itemType, &item, &box[kOutfitUpButton]);
    GetDialogItem(outfitDialog, 0xb, &itemType, &item, &box[kOutfitDownButton]);
    drawnButton = -1;
    downV = where.v;
    downH = where.h;
    if (PtInRect(downH, downV, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRect(downH, downV, &box[1]) != 0)
        drawnButton = 1;
    if (PtInRect(downH, downV, &box[2]) != 0)
        drawnButton = 2;
    if (PtInRect(downH, downV, &box[3]) != 0)
        drawnButton = 3;
    if (PtInRect(downH, downV, &box[4]) != 0)
        drawnButton = 4;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawOutfitButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRect(mouse.h, mouse.v, &box[0]) != 0)
                overButton = 0;
            if (PtInRect(mouse.h, mouse.v, &box[1]) != 0 && canBuy != 0)
                overButton = 1;
            if (PtInRect(mouse.h, mouse.v, &box[2]) != 0 && canSell != 0)
                overButton = 2;
            if (PtInRect(mouse.h, mouse.v, &box[3]) != 0
                    && outfitScroll >= kGridScrollStep)
                overButton = 3;
            if (PtInRect(mouse.h, mouse.v, &box[4]) != 0) {
                count = 0;
                i = 0;
                do {
                    if (dockedPortItems[(short)i] != -1)
                        count++;
                    i = i + 1;
                } while ((short)i < kMaxOutfits);
                if (outfitScroll < count - kGridPageCells)
                    overButton = 4;
            }
            if (overButton != drawnButton && overButton != downButton)
                overButton = -1;
            if (overButton != drawnButton) {
                DrawOutfitButtons(overButton);
                nv_UpdateDialogWindow(0);
            }
            drawnButton = overButton;
        }
    }
    return drawnButton;
}

/* DrawOutfitButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049fbb0
void DrawOutfitButtons(short hilited)
{
    int i;
    Rect *walk;
    Rect box[5];
    char shown[5];
    char enabled[5];
    short itemType;
    void *item;
    short count;

    RGBForeColor(blackColor);
    GetDialogItem(outfitDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(outfitDialog, 7, &itemType, &item, &box[1]);
    GetDialogItem(outfitDialog, 4, &itemType, &item, &box[2]);
    GetDialogItem(outfitDialog, 0xa, &itemType, &item, &box[3]);
    GetDialogItem(outfitDialog, 0xb, &itemType, &item, &box[4]);
    enabled[0] = 1;
    enabled[1] = 1;
    enabled[2] = 1;
    enabled[3] = 1;
    enabled[4] = 1;
    if (canBuy == 0)
        enabled[1] = 0;
    if (canSell == 0)
        enabled[2] = 0;
    if (outfitScroll < kGridScrollStep)
        enabled[3] = 0;
    count = 0;
    i = 0;
    do {
        if (dockedPortItems[(short)i] != -1)
            count++;
        i = i + 1;
    } while ((short)i < kMaxOutfits);
    if (outfitScroll >= count - kGridPageCells)
        enabled[4] = 0;
    outfitButtonTable[0] = 4;
    outfitButtonTable[1] = 1;
    outfitButtonTable[2] = 2;
    outfitButtonTable[3] = 0x12;
    outfitButtonTable[4] = 0x13;
    shown[0] = 1;
    shown[2] = 1;
    shown[3] = 1;
    shown[4] = 1;
    if (portBuysNoOutfits != 0)
        shown[1] = 0;
    else
        shown[1] = 1;
    i = 0;
    walk = box;
    do {
        if (shown[(short)i] != 0) {
            if (enabled[(short)i] == 0)
                DrawButton(GetDialogPort(outfitDialog), walk,
                           buttonLabel[outfitButtonTable[(short)i]], -1, 0, 0);
            else if ((short)i == hilited)
                DrawButton(GetDialogPort(outfitDialog), walk,
                           buttonLabel[outfitButtonTable[(short)i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(outfitDialog), walk,
                           buttonLabel[outfitButtonTable[(short)i]], 0, 0, 0);
        }
        i = i + 1;
        walk = walk + 1;
    } while ((short)i <= 4);
}
