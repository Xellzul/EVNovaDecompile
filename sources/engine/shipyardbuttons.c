// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
#ifdef MAC1010 /* 1.0.10 spelling: plain numbers at the sites */
#define kMapDoneButton 0
#define kMapZoomOutButton 1
#define kMapZoomInButton 2
#define kMapClearRouteButton 3
#define kMapInfluenceButton 4
#define kMapSearchButton 5
#define kShipyardDoneButton 0
#define kShipyardBuyButton 1
#define kShipyardInfoButton 2
#define kShipyardUpButton 3
#define kShipyardDownButton 4
#define kOutfitDoneButton 0
#define kOutfitBuyButton 1
#define kOutfitSellButton 2
#define kOutfitUpButton 3
#define kOutfitDownButton 4
#endif

/* TrackMapButtons. STATE: MATCH 100%, exe 247 / obj 247, byte-identical.
   The GetDialogItem sites index `box' with the slot enum (-enum min), which keeps their
   addresses out of the CSE class the PtInRect sites share. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049eef0
// MAC: _TrackMapButtons i386 0x0002aafa ppc 0x0002f048
short TrackMapButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    Rect box[6];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(mapDialog, 1, &itemType, &item, &box[kMapDoneButton]);
    GetDialogItem(mapDialog, 4, &itemType, &item, &box[kMapZoomOutButton]);
    GetDialogItem(mapDialog, 5, &itemType, &item, &box[kMapZoomInButton]);
    GetDialogItem(mapDialog, 8, &itemType, &item, &box[kMapClearRouteButton]);
    GetDialogItem(mapDialog, 9, &itemType, &item, &box[kMapInfluenceButton]);
    GetDialogItem(mapDialog, 0xa, &itemType, &item, &box[kMapSearchButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRectPoint(where, &box[1]) != 0 && canZoomOut != 0)
        drawnButton = 1;
    if (PtInRectPoint(where, &box[2]) != 0 && canZoomIn != 0)
        drawnButton = 2;
    if (PtInRectPoint(where, &box[3]) != 0 && showClearRouteButton != 0)
        drawnButton = 3;
    if (PtInRectPoint(where, &box[4]) != 0)
        drawnButton = 4;
    if (PtInRectPoint(where, &box[5]) != 0)
        drawnButton = 5;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawMapButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRectPoint(mouse, &box[0]) != 0)
                overButton = 0;
            if (PtInRectPoint(mouse, &box[1]) != 0 && canZoomOut != 0)
                overButton = 1;
            if (PtInRectPoint(mouse, &box[2]) != 0 && canZoomIn != 0)
                overButton = 2;
            if (PtInRectPoint(mouse, &box[3]) != 0 && showClearRouteButton != 0)
                overButton = 3;
            if (PtInRectPoint(mouse, &box[4]) != 0)
                overButton = 4;
            if (PtInRectPoint(mouse, &box[5]) != 0)
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
// MAC: _DrawMapButtons i386 0x0002a86e ppc 0x0002ee28
void DrawMapButtons(short hilited)
{
    short i;
    Rect box[6];
    Boolean enabled[6];
    Handle item;
    short itemType;

    ForeColor(blackColor);
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
    mapButtonTable[0] = kDoneButtonLabel;
    mapButtonTable[1] = kMinusButtonLabel;
    mapButtonTable[2] = kPlusButtonLabel;
    mapButtonTable[3] = kClearRouteButtonLabel;
    if (settings.showMapBorders != 0)
        mapButtonTable[4] = kHideBordersButtonLabel;
    else
        mapButtonTable[4] = kShowBordersButtonLabel;
    mapButtonTable[5] = kFindButtonLabel;
    for (i = 0; i < 6; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(mapDialog), &box[i],
                           buttonLabel[mapButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(mapDialog), &box[i],
                           buttonLabel[mapButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(mapDialog), &box[i],
                       buttonLabel[mapButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackShipyardButtons. STATE: MATCH 100%, exe 245 / obj 245, byte-identical.
   `Rect box[5]' with the slot enum at the GetDialogItem sites (-enum min); unlike its
   siblings the buy button's Rect is taken once into `buyBox' ahead of the first pass.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049f3f0
// MAC: _TrackShipyardButtons i386 0x0002a54e ppc 0x0002eb08
short TrackShipyardButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    short i;
    short count;
    Rect box[5];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(shipyardDialog, 7, &itemType, &item, &box[kShipyardDoneButton]);
    GetDialogItem(shipyardDialog, 1, &itemType, &item, &box[kShipyardBuyButton]);
    GetDialogItem(shipyardDialog, 0xa, &itemType, &item, &box[kShipyardInfoButton]);
    GetDialogItem(shipyardDialog, 0xc, &itemType, &item, &box[kShipyardUpButton]);
    GetDialogItem(shipyardDialog, 0xd, &itemType, &item, &box[kShipyardDownButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[1]) != 0 && canBuy != 0)
        drawnButton = 1;
    if (PtInRectPoint(where, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRectPoint(where, &box[2]) != 0 && selShipyardItem != -1)
        drawnButton = 2;
    if (PtInRectPoint(where, &box[3]) != 0
            && shipyardScroll >= kGridScrollStep)
        drawnButton = 3;
    if (PtInRectPoint(where, &box[4]) != 0) {
        count = 0;
        for (i = 0; i < kMaxShipTypes; i++)
            if (dockedPortShipTypes[i] != -1)
                count++;
        if (shipyardScroll < count - kGridPageCells)
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
            if (PtInRectPoint(mouse, &box[1]) != 0 && canBuy != 0)
                overButton = 1;
            if (PtInRectPoint(mouse, &box[0]) != 0)
                overButton = 0;
            if (PtInRectPoint(mouse, &box[2]) != 0 && selShipyardItem != -1)
                overButton = 2;
            if (PtInRectPoint(mouse, &box[3]) != 0
                    && shipyardScroll >= kGridScrollStep)
                overButton = 3;
            if (PtInRectPoint(mouse, &box[4]) != 0) {
                count = 0;
                for (i = 0; i < kMaxShipTypes; i++)
                    if (dockedPortShipTypes[i] != -1)
                        count++;
                if (shipyardScroll < count - kGridPageCells)
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
// MAC: _DrawShipyardButtons i386 0x0002a2e2 ppc 0x0002e8b8
void DrawShipyardButtons(short hilited)
{
    short i;
    Rect box[5];
    Boolean enabled[5];
    Handle item;
    short itemType;
    short count;

    GetDialogItem(shipyardDialog, 7, &itemType, &item, &box[0]);
    GetDialogItem(shipyardDialog, 1, &itemType, &item, &box[1]);
    GetDialogItem(shipyardDialog, 0xa, &itemType, &item, &box[2]);
    GetDialogItem(shipyardDialog, 0xc, &itemType, &item, &box[3]);
    GetDialogItem(shipyardDialog, 0xd, &itemType, &item, &box[4]);
    shipyardButtonTable[0] = kDoneButtonLabel;
    if (shipBuyMode == 0)
        shipyardButtonTable[1] = kBuyShipButtonLabel;
    else
        shipyardButtonTable[1] = kHireEscortButtonLabel;
    shipyardButtonTable[2] = kInfoButtonLabel;
    shipyardButtonTable[3] = kScrollUpButtonLabel;
    shipyardButtonTable[4] = kScrollDownButtonLabel;
    enabled[0] = 1;
    enabled[1] = canBuy;
    enabled[2] = selShipyardItem != -1;
    enabled[3] = shipyardScroll >= kGridScrollStep;
    count = 0;
    for (i = 0; i < kMaxShipTypes; i++)
        if (dockedPortShipTypes[i] != -1)
            count++;
    enabled[4] = shipyardScroll < count - kGridPageCells;
    for (i = 0; i < 5; i++) {
        if (enabled[i] != 0) {
            if (i == hilited)
                DrawButton(GetDialogPort(shipyardDialog), &box[i],
                           buttonLabel[shipyardButtonTable[i]], 0, 1, 0);
            else
                DrawButton(GetDialogPort(shipyardDialog), &box[i],
                           buttonLabel[shipyardButtonTable[i]], 0, 0, 0);
        } else
            DrawButton(GetDialogPort(shipyardDialog), &box[i],
                       buttonLabel[shipyardButtonTable[i]], kButtonDim, 0, 0);
    }
}

/* TrackOutfitButtons. STATE: MATCH 100%, exe 225 / obj 225, byte-identical.
   Same shape as TrackMapButtons: one `Rect box[5]', the slot enum at the GetDialogItem
   sites (-enum min). OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049f8f0
// MAC: _TrackOutfitButtons i386 0x0002a029 ppc 0x0002e628
short TrackOutfitButtons(Point where)
{
    short drawnButton;
    short downButton;
    short overButton;
    short count;
    short i;
    Rect box[5];
    short itemType;
    Handle item;
    Point mouse;

    ForeColor(blackColor);
    GetDialogItem(outfitDialog, 1, &itemType, &item, &box[kOutfitDoneButton]);
    GetDialogItem(outfitDialog, 7, &itemType, &item, &box[kOutfitBuyButton]);
    GetDialogItem(outfitDialog, 4, &itemType, &item, &box[kOutfitSellButton]);
    GetDialogItem(outfitDialog, 0xa, &itemType, &item, &box[kOutfitUpButton]);
    GetDialogItem(outfitDialog, 0xb, &itemType, &item, &box[kOutfitDownButton]);
    drawnButton = -1;
    if (PtInRectPoint(where, &box[0]) != 0)
        drawnButton = 0;
    if (PtInRectPoint(where, &box[1]) != 0)
        drawnButton = 1;
    if (PtInRectPoint(where, &box[2]) != 0)
        drawnButton = 2;
    if (PtInRectPoint(where, &box[3]) != 0)
        drawnButton = 3;
    if (PtInRectPoint(where, &box[4]) != 0)
        drawnButton = 4;
    downButton = drawnButton;
    if (drawnButton != -1) {
        DrawOutfitButtons(drawnButton);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            overButton = -1;
            if (PtInRectPoint(mouse, &box[0]) != 0)
                overButton = 0;
            if (PtInRectPoint(mouse, &box[1]) != 0 && canBuy != 0)
                overButton = 1;
            if (PtInRectPoint(mouse, &box[2]) != 0 && canSell != 0)
                overButton = 2;
            if (PtInRectPoint(mouse, &box[3]) != 0
                    && outfitScroll >= kGridScrollStep)
                overButton = 3;
            if (PtInRectPoint(mouse, &box[4]) != 0) {
                count = 0;
                for (i = 0; i < kMaxOutfits; i++)
                    if (dockedPortItems[i] != -1)
                        count++;
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
// MAC: _DrawOutfitButtons i386 0x00029d6a ppc 0x0002e398
void DrawOutfitButtons(short hilited)
{
    short i;
    Rect box[5];
    Boolean enabled[5];
    Boolean shown[5];
    short itemType;
    Handle item;
    short count;

    ForeColor(blackColor);
    GetDialogItem(outfitDialog, 1, &itemType, &item, &box[0]);
    GetDialogItem(outfitDialog, 7, &itemType, &item, &box[1]);
    GetDialogItem(outfitDialog, 4, &itemType, &item, &box[2]);
    GetDialogItem(outfitDialog, 0xa, &itemType, &item, &box[3]);
    GetDialogItem(outfitDialog, 0xb, &itemType, &item, &box[4]);
    for (i = 0; i <= 4; i++)
        enabled[i] = 1;
    if (canBuy == 0)
        enabled[1] = 0;
    if (canSell == 0)
        enabled[2] = 0;
    if (outfitScroll < kGridScrollStep)
        enabled[3] = 0;
    count = 0;
    for (i = 0; i < kMaxOutfits; i++)
        if (dockedPortItems[i] != -1)
            count++;
    if (outfitScroll >= count - kGridPageCells)
        enabled[4] = 0;
    outfitButtonTable[0] = kDoneButtonLabel;
    outfitButtonTable[1] = kBuyButtonLabel;
    outfitButtonTable[2] = kSellButtonLabel;
    outfitButtonTable[3] = kScrollUpButtonLabel;
    outfitButtonTable[4] = kScrollDownButtonLabel;
    shown[0] = 1;
    shown[2] = 1;
    shown[3] = 1;
    shown[4] = 1;
    if (portBuysNoOutfits != 0)
        shown[1] = 0;
    else
        shown[1] = 1;
    for (i = 0; i <= 4; i++) {
        if (shown[i] != 0) {
            if (enabled[i] != 0) {
                if (i == hilited)
                    DrawButton(GetDialogPort(outfitDialog), &box[i],
                               buttonLabel[outfitButtonTable[i]], 0, 1, 0);
                else
                    DrawButton(GetDialogPort(outfitDialog), &box[i],
                               buttonLabel[outfitButtonTable[i]], 0, 0, 0);
            } else
                DrawButton(GetDialogPort(outfitDialog), &box[i],
                           buttonLabel[outfitButtonTable[i]], kButtonDim, 0, 0);
        }
    }
}
