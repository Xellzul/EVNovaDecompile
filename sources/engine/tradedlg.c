// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* tradedlg.c -- tu-map object O24: the trade, outfit, shipyard, port, player-info and description dialogs, then the escort menu, quantity dialog and map world, ending at the .CRT hook. Notes: Research/notes/tradedlg.md */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "burger.h"
#include "sound.h"
#include "ai.h"
#include "space.h"

#define kFirstRowItem  4
#define kLastRowItem   11
#define kLastRow       7
#define kBuyItem       13
#define kSellItem      14

PictCacheSlot pictCache[128];
DialogPtr alertDialog;
DialogPtr depthDialog;
DialogPtr loadingDialog;
DialogPtr tradeDialog;
DialogPtr portDialog;
DialogPtr qtyDialog;
DialogPtr textDialog;
DialogPtr outfitDialog;
DialogPtr outfitDescDialog;
DialogPtr shipyardDialog;
DialogPtr shipDescDialog;
DialogPtr infoDialog;
DialogPtr captureDialog;
char gItemDescText[8192];
short gStellarPictID;
short gSelectedOutfit;
short gSelectedShip;
short gOutfitGridPos;
short playerInfoPage;
short gShipGridPos;
short gLastClickPos;
short outfitScroll;
short gShipGridScroll;
short shipBuyMode;
short gMovieRefNum;
short junkType[2];
unsigned long gLastClickTime;
float priceFluxMult[2];
PicHandle gStellarDialogPict;
PicHandle gShipDescPict;
PicHandle gShipyardDialogPict;
PicHandle gOutfitDialogPict;
PicHandle gShipDescBackdrop;
PicHandle captureDialogPict;
PicHandle tradeBackgroundPic;
PicHandle playerInfoBackgroundPic[3];
PicHandle textDialogBackgroundPic[3];
PicHandle previewPic;
void *gAmbientSound;
char canBuy;
char canSell;
char gSpobInhabited;
char shipyardEgg;
char didOutfit;
char didBuyShip;
unsigned char DAT_007d4c06[2];
char boughtMap;
char boughtID;
char onPlanet;
char canScrollTextUp;
char canScrollTextDown;
char portBuysNoOutfits;
unsigned char gTextInputResult[256];
Rect pictCacheRect[128];
Rect gCellRects[20];
Rect gIconRects[20];
void *loopingMovie;
TEHandle textDialogTextHandle;
unsigned long lastTextScrollTime;
unsigned long gAmbientDeadline;
GWorldContainer pictCachePort;
GWorldContainer gStellarImage;
char playerInfoText[3][4096];
short nagCount;
short previewPicID;
RGBColor shopGridColor[2];
void *gIntroMovieData;
Rect gMovieBox;

extern "C" {

#define kTradeDlogID   0x3e9
#define kTradePictID   0x213e
#define kTradeVenue    4
#define kMaxTradeQty   0x7d00

#define kHoldsItem     3
#define kCargoItem     0xc
#define kNewsItem      0xf
#define kLastCommodity 5

/* port price-status codes read off portDemand[]; the STR# 2002 index each
   selects (0xc9/0xca/0xcb) is read straight from the exe, name TBC */
#define kPortShortage 1
#define kPortSurplus  2
#define kPortEmbargo  4


/* DoTradeDialog (mac _DoTradeDialog). STATE: MATCH, 628/628, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0048c730
void DoTradeDialog(void)
{
    char done;
    short savedStellar;
    short savedRoute;
    short mods;
    double mult;
    short itemHit;
    short i;
    short j;
    short space;
    short maxSell;
    long afford;
    long qty;

    done = 0;
    savedStellar = shipData->targetStellar;
    savedRoute = shipData->navTargetKind;
    mult = 1.25;
    if (dockedPort->govt != -1
        && playerLegalStatus[dockedPort->system] < 0) {
        mult = 1.1;
    }
    if (dockedPort->dominated != 0) {
        mult = 1.5;
    }
    for (i = 0; i <= kLastCommodity; i++) {
        portDemand[i] = CalcPortDemand(i, dockedPort->flags);
        tradePrice[i] = 0;
        if (portDemand[i] > 0) {
            if (portDemand[i] == kPortShortage)
                tradePrice[i] = basePrice[i] / mult;
            else if (portDemand[i] == kPortSurplus)
                tradePrice[i] = basePrice[i];
            else if (portDemand[i] == kPortEmbargo)
                tradePrice[i] = basePrice[i] * mult;
            if (tradePrice[i] < 5)
                tradePrice[i] = 5;
        }
    }
    for (i = 0; i < 0x100; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].countdown > 0
            && shipData->targetStellar == disasterData[i].stellarID) {
            tradePrice[disasterData[i].commodity] =
                basePrice[disasterData[i].commodity];
            tradePrice[disasterData[i].commodity] += disasterData[i].amount;
            if (tradePrice[disasterData[i].commodity] < 5)
                tradePrice[disasterData[i].commodity] = 5;
        }
    }
    junkType[0] = -1;
    junkType[1] = -1;
    tradePrice[7] = 0;
    tradePrice[6] = tradePrice[7];
    for (i = 0; i < 0x80; i++) {
        for (j = 0; j < 8; j++) {
            if (shipData->targetStellar == junkData[i].boughtAt[j]) {
                if (EvalMissionBitTestString(junkData[i].buyOn) != 0) {
                    junkType[0] = i;
                    tradePrice[6] = junkData[i].basePrice * mult;
                    break;
                }
            }
        }
    }
    for (i = 0; i < 0x80; i++) {
        for (j = 0; j < 8; j++) {
            if (shipData->targetStellar == junkData[i].soldAt[j]) {
                if (EvalMissionBitTestString(junkData[i].sellOn) != 0) {
                    junkType[1] = i;
                    tradePrice[7] = junkData[i].basePrice / mult;
                    break;
                }
            }
        }
    }
    missionListVenue = kTradeVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(0x1e) + 0x1e;
    tradeDialog = NULL;
    tradeDialog = GetNewDialog(kTradeDlogID, UpdateTradeDialog,
                               TradeDialogEvent);
    if (tradeDialog == NULL)
        return;
    tradeBackgroundPic = GetPicture(kTradePictID);
    CenterWindow(FrontWindow(tradeDialog), FrontWindow(portDialog));
    selTradeItem = 7;
    TradeDialogTabKey(0);
    ShowWindowNoFlash(FrontWindow(tradeDialog));
    SetDialogGWorld(tradeDialog);
    UpdateTradeDialog();
    FrontWindow(tradeDialog);
    FrontWindow(tradeDialog);
    OfferMissionFromPort(kTradeVenue);
    RedrawDialog(tradeDialog);
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(tradeDialog, TradeDialogEvent, &itemHit, &mods);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2) {
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            UpdateAllPortDialogs();
            SetDialogGWorld(tradeDialog);
            RedrawDialog(tradeDialog);
        }
        if (itemHit == 0x10) {
            DoPlayerInfoDialog();
            UpdateAllPortDialogs();
            SetDialogGWorld(tradeDialog);
            RedrawDialog(tradeDialog);
        }
        if (itemHit == 0x11) {
            qty = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0
                    && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
                    qty++;
            }
            if (qty > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(tradeDialog);
                UpdateAllPortDialogs();
                SetDialogGWorld(tradeDialog);
                RedrawDialog(tradeDialog);
            }
        }
        if (itemHit == kBuyItem) {
            RedrawDialog(tradeDialog);
            space = TotalFleetHolds() - TotalCargo(shipData);
            if (space > 0) {
                afford = shipData->credits / (double)tradePrice[selTradeItem];
                if ((mods & optionKey) != 0) {
                    if (afford > space)
                        qty = space;
                    else
                        qty = afford;
                    if (qty > kMaxTradeQty)
                        qty = kMaxTradeQty;
                    qty = DoQuantityDialog(qty, 0);
                    UpdateTradeDialog();
                } else {
                    qty = 10;
                    if (afford < 10)
                        qty = afford;
                    if (qty > space)
                        qty = space;
                }
                if (qty > 0) {
                    if (selTradeItem > kLastCommodity) {
                        if (junkType[selTradeItem - kFirstJunkItem] != -1)
                            junkData[junkType[selTradeItem - kFirstJunkItem]]
                                .held += qty;
                    } else
                        shipData->cargo[selTradeItem] += qty;
                    shipData->credits -= tradePrice[selTradeItem] * qty;
                }
                RedrawDialog(tradeDialog);
                statusRedraw[2] = 1;
                DrawStatus();
                SetDialogGWorld(tradeDialog);
            }
            RedrawDialog(tradeDialog);
        }
        if (itemHit == kSellItem) {
            RedrawDialog(tradeDialog);
            /* BUG: an empty junk row leaves qty unset, so maxSell comes from garbage */
            if (selTradeItem <= kLastCommodity)
                qty = shipData->cargo[selTradeItem];
            else if (junkType[selTradeItem - kFirstJunkItem] != -1)
                qty = junkData[junkType[selTradeItem - kFirstJunkItem]].held;
            if (qty > kMaxTradeQty)
                maxSell = kMaxTradeQty;
            else
                maxSell = qty;
            qty = 0;
            if (maxSell > 0) {
                if ((mods & optionKey) != 0) {
                    qty = DoQuantityDialog(maxSell, 1);
                    UpdateTradeDialog();
                } else {
                    qty = 10;
                    if (selTradeItem <= kLastCommodity) {
                        if (shipData->cargo[selTradeItem] < qty)
                            qty = shipData->cargo[selTradeItem];
                    } else if (junkType[selTradeItem - kFirstJunkItem] != -1) {
                        if (junkData[junkType[selTradeItem - kFirstJunkItem]]
                                .held < qty)
                            qty = junkData[junkType[selTradeItem
                                                    - kFirstJunkItem]].held;
                    }
                }
                if (qty > 0) {
                    if (selTradeItem > kLastCommodity) {
                        if (junkType[selTradeItem - kFirstJunkItem] != -1)
                            junkData[junkType[selTradeItem - kFirstJunkItem]]
                                .held -= qty;
                    } else
                        shipData->cargo[selTradeItem] -= qty;
                    shipData->credits += tradePrice[selTradeItem] * qty;
                }
                RedrawDialog(tradeDialog);
                statusRedraw[2] = 1;
                DrawStatus();
                SetDialogGWorld(tradeDialog);
            }
            RedrawDialog(tradeDialog);
        }
        if (itemHit == kNewsItem) {
            if (OfferMissionFromPort(kTradeVenue) != 0) {
                SetDialogGWorld(portDialog);
                UpdateAllPortDialogs();
            }
        }
    } while (done == 0);
    DisposeDialogNoFlash(tradeDialog);
    tradeDialog = NULL;
    if (tradeBackgroundPic != NULL) {
        QDReleaseResource(tradeBackgroundPic);
        tradeBackgroundPic = NULL;
    }
    shipData->targetStellar = savedStellar;
    shipData->navTargetKind = savedRoute;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}

/* TradeDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048d190
char TradeDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point mouse;
    Point where;
    void *item;
    short j;
    short last;
    short i;
    int c;
    int key;
    short button;

    SetDialogGWorld(tradeDialog);
    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 1;
        return 1;
    }
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map)) {
            *itemHit = 2;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 16;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 17;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnChar || c == kEscapeChar) {
                ev->what = kMouseDown;
                *itemHit = 1;
                return 1;
            }
        }
        if (c == kTabChar) {
            if ((*(unsigned short *)&ev->modifiers & shiftKey) != 0)
                TradeDialogTabKey(1);
            else
                TradeDialogTabKey(0);
        }
        if (c == kDownArrowChar)
            TradeDialogTabKey(0);
        if (c == kUpArrowChar)
            TradeDialogTabKey(1);
        key = tolower(c);
        if (key == 'b') {
            *itemHit = kBuyItem;
            return 1;
        }
        if (key == 's') {
            *itemHit = kSellItem;
            return 1;
        }
    }
    if ((int)ev->what == kMouseDown) {
        SetDialogGWorld(tradeDialog);
        where.v = ev->where.v;
        where.h = ev->where.h;
        for (i = kFirstRowItem; i <= kLastRowItem; i++) {
            GetDialogItem(tradeDialog, i, &itemType, &item, &box);
            if (PtInRect(where.h, where.v, &box) &&
                tradePrice[i - kFirstRowItem] != 0) {
                InvalDialogItem(tradeDialog, selTradeItem + kFirstRowItem);
                InvalDialogItem(tradeDialog, kBuyItem);
                InvalDialogItem(tradeDialog, kSellItem);
                last = selTradeItem;
                selTradeItem = i - kFirstRowItem;
                while (StillDown()) {
                    if (selTradeItem != last) {
                        InvalDialogItem(tradeDialog,
                                     selTradeItem + kFirstRowItem);
                        InvalDialogItem(tradeDialog, last + kFirstRowItem);
                        UpdateTradeDialog();
                    }
                    GetMouse(&mouse);
                    for (j = 0; j <= kLastRow; j++) {
                        GetDialogItem(tradeDialog, j + kFirstRowItem, &itemType,
                                      &item, &box);
                        if (PtInRect(mouse.h, mouse.v, &box) &&
                            tradePrice[j] != 0) {
                            last = selTradeItem;
                            selTradeItem = j;
                            break;
                        }
                    }
                }
                SetDialogGWorld(tradeDialog);
                InvalDialogItem(tradeDialog, selTradeItem + kFirstRowItem);
                return 0;
            }
        }
        UpdateTradeDialog();
        button = TrackTradeButtons(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = kBuyItem;
            return 1;
        }
        if (button == 2) {
            *itemHit = kSellItem;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (ReadTick() >= missionOfferTime) {
        *itemHit = 15;
        missionOfferTime = ReadTick() + Rand(30) + 30;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        UpdateTradeDialog();
        return 0;
    }
    return 0;
}

#define kMapFilterCount 8


/* TradeDialogTabKey. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048d5a0
void TradeDialogTabKey(char back)
{
    Rect box[2];
    short itemType;
    void *item;

    if (back)
        selTradeItem -= 1;
    else
        selTradeItem += 1;
    if (selTradeItem > kMapFilterCount - 1)
        selTradeItem = 0;
    if (selTradeItem < 0)
        selTradeItem = kMapFilterCount - 1;
    while (tradePrice[selTradeItem] == 0) {
        if (back)
            selTradeItem -= 1;
        else
            selTradeItem += 1;
        if (selTradeItem > kMapFilterCount - 1)
            selTradeItem = 0;
        if (selTradeItem < 0)
            selTradeItem = kMapFilterCount - 1;
    }
    InvalDialogItem(tradeDialog, 13);
    InvalDialogItem(tradeDialog, 14);
    GetDialogItem(tradeDialog, 3, &itemType, &item, &box[0]);
    box[1].top = box[0].top;
    box[1].left = box[0].left;
    box[1].bottom = box[0].bottom;
    box[1].right = box[0].right;
    GetDialogItem(tradeDialog, 12, &itemType, &item, &box[0]);
    box[1].bottom = box[0].bottom;
    InvalDialogRect(tradeDialog, &box[1]);
}

/* UpdateTradeDialog (mac _TradeDialogUpdate). STATE: MATCH, 1241/1241, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0048d6f0
void UpdateTradeDialog(void)
{
    extern char *strcpy(char *, const char *);   /* MSL, CALLED not inlined */

    unsigned char message[256];
    void *item;
    short itemType;
    short missionTons;
    short junkTons;
    short missionSlots;
    char seen[256];
    short junkSlots;
    short k;
    long row;
    Rect insetBox;
    unsigned char num[256];
    Rect box;
    unsigned char text[2048];
    unsigned char *rowName;
    char found;
    short i;
    short j;
    short qty;
    short p;
    short fleetHolds;
    short holds;
    short totalCargo;
    short available;
    short missionCargo;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(tradeDialog));
    if (tradeBackgroundPic != NULL)
        DrawPicHandle(tradeBackgroundPic, GetDialogPortRect(tradeDialog));

    RGBForeColor(blackColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    GetDialogItem(tradeDialog, kHoldsItem, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(tradeDialog)))) {
        RGBForeColor(&rgbDarkGrey);
        FrameRect(&box);
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 7), (box.top + 12));
        DrawIndString(kGameStrList, 0xc5);
        MoveTo((box.right - 0x7b), (box.top + 12));
        if (TotalFleetHolds() > ShipTotalHolds())
            DrawIndString(kGameStrList, 0xc6);
        else
            DrawIndString(kGameStrList, 0xc7);
        MoveTo((box.right - 0x46), (box.top + 12));
        DrawIndString(kGameStrList, 0xc8);
        RGBForeColor(blackColor);
    }

    i = 0;
    rowName = cargoName[0];
    for (; i <= kLastRow; i++, rowName += 0x100) {
        row = i;
        GetDialogItem(tradeDialog, row + kFirstRowItem, &itemType, &item, &box);
        if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(tradeDialog)))) {
            insetBox.top = box.top;
            insetBox.left = box.left;
            insetBox.bottom = box.bottom;
            insetBox.right = box.right;
            InsetRect(&insetBox, 1, 1);
            if (i == selTradeItem)
                RGBForeColor(&ldefColors[2]);
            else
                RGBForeColor(&ldefColors[1]);
            PaintRect(&insetBox);
            RGBForeColor(blackColor);

            RGBForeColor(&rgbDarkGrey);
            MoveTo(box.left, box.top);
            LineTo(box.left, (box.bottom - 1));
            MoveTo((box.right - 1), box.top);
            LineTo((box.right - 1), (box.bottom - 1));
            if (i == 0) {
                MoveTo(box.left, box.top);
                LineTo((box.right - 1), box.top);
            }
            if (i == kLastRow) {
                MoveTo(box.left, (box.bottom - 1));
                LineTo((box.right - 1), (box.bottom - 1));
            }

            if (tradePrice[i] != 0) {
                RGBForeColor(&ldefColors[0]);
                if (i <= kLastCommodity) {
                    MoveTo((box.left + 7), (box.top + 10));
                    DrawString(rowName);
                    qty = shipData->cargo[i];
                    if (qty > 0) {
                        NumToString(qty, text);
                        MoveTo((box.right -
                                       (StringWidth(text) + 0x6e)),
                               (box.top + 10));
                        DrawString(text);
                    }
                } else if (junkType[(int)row - kFirstJunkItem] != -1) {
                    int junkIdx = row - kFirstJunkItem;
                    MoveTo((box.left + 6), (box.top + 10));
                    DrawString(junkData[junkType[junkIdx]].name);
                    qty = junkData[junkType[junkIdx]].held;
                    if (qty > 0) {
                        NumToString(qty, text);
                        MoveTo((box.right -
                                       (StringWidth(text) + 0x6e)),
                               (box.top + 10));
                        DrawString(text);
                    }
                }

                MoveTo((box.right - 0x46), (box.top + 10));
                found = 0;
                for (k = 0; k < 0x100; k++) {
                    if (disasterData[k].loaded != 0 && disasterData[k].countdown > 0 &&
                        disasterData[k].stellarID == shipData->targetStellar &&
                        i == disasterData[k].commodity) {
                        found = 1;
                        if (disasterData[k].amount > 0)
                            DrawIndString(kGameStrList, 0xcc);
                        if (disasterData[k].amount < 0)
                            DrawIndString(kGameStrList, 0xcd);
                        break;
                    }
                }
                if (!found) {
                    if (i <= kLastCommodity) {
                        if (portDemand[i] == kPortShortage)
                            DrawIndString(kGameStrList, 0xc9);
                        if (portDemand[i] == kPortSurplus)
                            DrawIndString(kGameStrList, 0xca);
                        if (portDemand[i] == kPortEmbargo)
                            DrawIndString(kGameStrList, 0xcb);
                    } else if (junkType[(int)row - kFirstJunkItem] != -1) {
                        if (i == kFirstJunkItem)
                            DrawIndString(kGameStrList, 0xcb);
                        if (i == kLastRow)
                            DrawIndString(kGameStrList, 0xc9);
                    }
                }

                NumToString(tradePrice[i], text);
                MoveTo((box.right - (StringWidth(text) + 10)),
                       (box.top + 10));
                DrawString(text);
            }
            RGBForeColor(blackColor);
        }
    }

    GetDialogItem(tradeDialog, kCargoItem, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(tradeDialog)))) {
        RGBForeColor(blackColor);
        insetBox.top = box.top;
        insetBox.left = box.left;
        insetBox.bottom = box.bottom;
        insetBox.right = box.right;
        InsetRect(&insetBox, 5, 3);
        strcpy((char *)text, "");

        missionCargo = 0;
        k = 0;
        missionTons = 0;
        junkTons = 0;
        missionSlots = 0;
        junkSlots = 0;

        for (i = 0; i < 0x100; i++)
            seen[i] = 0;
        for (j = 0; j < 16; j++) {
            if (currentMissionObjectives[j].active && currentMissions[j].hasCargo &&
                currentMissions[j].cargoType != -1 && currentMissions[j].cargoQty >= 0) {
                missionCargo++;
                missionSlots++;
                missionTons += currentMissions[j].cargoQty;
                if (seen[currentMissions[j].cargoType] == 0) {
                    seen[currentMissions[j].cargoType] = 1;
                    k++;
                }
            }
        }
        for (j = 0; j < 0x80; j++) {
            if (junkData[j].held > 0 && j != junkType[0] && j != junkType[1]) {
                missionCargo++;
                junkTons += junkData[j].held;
                junkSlots++;
                k++;
            }
        }
        if (missionCargo > 0) {
            if (missionSlots + junkSlots > 0) {
                AppendIndString((char *)text, kGameStrList, 0x16b);
                strcat((char *)text, " ");
                if (missionTons + junkTons > 0) {
                    NumToString(missionTons + junkTons, num);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                    strcat((char *)text, " ");
                    if (missionTons + junkTons == 1)
                        pstrcpy(num, tonString);
                    else
                        pstrcpy(num, tonsString);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                    strcat((char *)text, " ");
                    AppendIndString((char *)text, kGameStrList, 0x187);
                    strcat((char *)text, " ");
                }
                if (missionSlots > 0) {
                    AppendIndString((char *)text, kGameStrList, 0x16f);
                    if (junkSlots > 0) {
                        strcat((char *)text, " ");
                        AppendIndString((char *)text, kGameStrList, 0x188);
                        strcat((char *)text, " ");
                    }
                }
                if (junkSlots > 0)
                    AppendIndString((char *)text, kGameStrList, 0x170);
                strcat((char *)text, "\r\r");
            }
        }

        holds = ShipTotalHolds();
        fleetHolds = TotalFleetHolds();
        totalCargo = TotalCargo(shipData);
        AppendIndString((char *)text, kGameStrList, 0x16c);
        if (fleetHolds > holds) {
            strcat((char *)text, " ");
            AppendIndString((char *)text, kGameStrList, 0x16d);
        }
        strcat((char *)text, ": ");
        if (fleetHolds > holds) {
            available = totalCargo - missionTons;
            available -= fleetHolds - holds;
            if (available < 0)
                available = 0;
            available = holds - (available + missionTons);
            if (available < 0)
                available = 0;
        } else {
            available = holds - totalCargo;
            if (available < 0)
                available = 0;
        }
        NumToString(available, num);
        PtoCstr(num);
        strcat((char *)text, (char *)num);
        strcat((char *)text, " ");
        if (available == 1)
            pstrcpy(num, tonString);
        else
            pstrcpy(num, tonsString);
        PtoCstr(num);
        strcat((char *)text, (char *)num);
        if (fleetHolds > holds) {
            fleetHolds = fleetHolds - totalCargo;
            if (fleetHolds < 0)
                fleetHolds = 0;
            strcat((char *)text, "\r");
            AppendIndString((char *)text, kGameStrList, 0x16c);
            strcat((char *)text, " ");
            AppendIndString((char *)text, kGameStrList, 0x16e);
            strcat((char *)text, ": ");
            NumToString(fleetHolds, num);
            PtoCstr(num);
            strcat((char *)text, (char *)num);
            strcat((char *)text, " ");
            if (fleetHolds == 1)
                pstrcpy(num, tonString);
            else
                pstrcpy(num, tonsString);
            PtoCstr(num);
            strcat((char *)text, (char *)num);
        }

        TETextBox(text, strlen((char *)text), &insetBox, 0);
        InvertRect(&insetBox);
        RGBForeColor(&rgbDarkGrey);
        FrameRect(&box);
        RGBForeColor(blackColor);
    }
    DrawTradeButtons(-1);

    GetDialogItem(tradeDialog, kNewsItem, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(tradeDialog)))) {
        for (p = 0; p < 0x100; p++) {
            if (disasterData[p].loaded != 0 && disasterData[p].countdown > 0 &&
                shipData->targetStellar == disasterData[p].stellarID) {
                if (cheatsActive) {
                    strcpy((char *)text, "Disaster ID ");
                    NumToString(p + 0x80, num);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                } else {
                    strncpy((char *)text, (char *)disasterData[p].name, 0x3f);
                    PtoCstr(text);
                }
                strcat((char *)text, " ");
                AppendIndString((char *)text, kGameStrList, 0xc0);
                strcat((char *)text, " ");
                if (disasterData[p].amount > 0)
                    AppendIndString((char *)text, kGameStrList, 0xc1);
                else
                    AppendIndString((char *)text, kGameStrList, 0xc2);
                strcat((char *)text, " ");
                AppendIndString((char *)text, kGameStrList, 0xb5);
                strcat((char *)text, " ");
                strncpy((char *)num, (char *)lcCargoName[disasterData[p].commodity], 0x40);
                PtoCstr(num);
                strcat((char *)text, (char *)num);
                strcat((char *)text, ".");

                strcpy((char *)message, (char *)text);
                RGBForeColor(blackColor);
                TextFont(smallTextFont);
                TextSize(smallTextSize);
                TETextBox(message, strlen((char *)message), &box, 0);
                InvertRect(&box);
                break;
            }
        }
    }

    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(tradeDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(tradeDialog)),
             GetDialogPortRect(tradeDialog), GetDialogPortRect(tradeDialog), 0,
             GetCWindowVisRgn(FrontWindow(tradeDialog)));
}

#define kStellarBackdropPictID 0x2134

/* PreloadPortDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048e970
void PreloadPortDialog(SpobRecord *spob, short stellar)
{
    Rect rect;
    PicHandle pic;

    gStellarDialogPict = GetPicture(kStellarBackdropPictID);
    gStellarImage.world = NULL;

    if (spob->customPict >= 128)
        gStellarPictID = spob->customPict;
    else
        gStellarPictID = spob->type + 10000;

    pic = GetPicture(gStellarPictID);
    if (pic != NULL) {
        QDGetPictureBounds(pic, &rect);
        OffsetRect(&rect, -rect.left, -rect.top);
        CreateGWorldContainer(&gStellarImage, 0, gSpriteWorld->pixelDepth, &rect);
        if (gStellarImage.world != NULL) {
            SetGWorldContainer(&gStellarImage);
            RGBForeColor(blackColor);
            PaintRect(&gStellarImage.bounds);
            DrawPicHandle(pic, &gStellarImage.bounds);
            QDReleaseResource(pic);
            UnsetGWorldContainer(&gStellarImage);
        }
    }
}

#define kOutfitDescBase 3000    /* desc ID of outfit 0's description */
#define kOutfitPictBase 6000    /* PICT ID of outfit 0's picture */

/* the item grid: four across, five down, out of item 5's box */


#define kOutfitDlogID    0x3ea
#define kOutfitPictID    0x2136
#define kOutfitVenue     6


/* DoOutfitDialog (mac _DoOutfitDialog). STATE: MATCH 100.0%, 1509/1509, byte-identical yes.
   OPEN: none. The order of the address-taken locals decides the frame's use-5 tie between
   weapID and the sell quantity (see notes). */
// FUNCTION: EVNOVA 0x0048ea70
void DoOutfitDialog(void)
{
    short itemHit;
    short itemType;
    void *item;
    short filterKey;
    short savedUnk6C;
    short savedUnk6A;
    long qtyLimit;
    int have;
    short count;
    short weapID;
    short ammo;
    char done;
    short shortfall;
    short n;
    short maxAfford;
    short savedOutfits[kMaxOutfits];
    Rect box;
    unsigned char nameBuf[256];
    char msg[256];
    short i;
    short j;
    short k;
    short m;
    char shown;
    long cost;

    done = 0;
    for (i = 0; i < kMaxOutfits; i++)
        savedOutfits[i] = shipSystems[i];
    savedUnk6C = shipData->targetStellar;
    savedUnk6A = shipData->navTargetKind;
    gSelectedOutfit = -1;
    gOutfitGridPos = -1;
    outfitScroll = 0;
    boughtMap = boughtID = 0;
    canBuy = canSell = 0;
    ShipStatsToSystemInfo();
    SetupPortAvailableItems(dockedPort);
    if (dockedPort->techLevel == 0) {
        portBuysNoOutfits = 1;
        for (i = 0; i < 8; i++) {
            if (dockedPort->specialTech[i] > 0) {
                portBuysNoOutfits = 0;
                break;
            }
        }
    } else
        portBuysNoOutfits = 0;
    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (dockedPortItems[i] != -1) {
            count++;
            break;
        }
    }
    if (count == 0) {
        extern char *strcpy(char *, const char *);

        strcpy(gTextShowString, "");
        AppendIndString(gTextShowString, kGameStrList, 0xce);
        ShowTextDialog(0, 0, 0);
        return;
    }
    previewPic = NULL;
    previewPicID = -1;
    missionListVenue = kOutfitVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    outfitDialog = NULL;
    outfitDialog = GetNewDialog(kOutfitDlogID, OutfitDialogUpdate, OutfitFilter);
    if (outfitDialog == NULL)
        return;
    gOutfitDialogPict = GetPicture(kOutfitPictID);
    CenterWindow(FrontWindow(outfitDialog), FrontWindow(portDialog));
    GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
    SetupListRects((Point *)&box);
    strncpy(gItemDescText, "", 3);
    ShowWindowNoFlash(FrontWindow(outfitDialog));
    SetDialogGWorld(outfitDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RGBForeColor(blackColor);
    OutfitDialogUpdate();
    FrontWindow(outfitDialog);
    FrontWindow(outfitDialog);
    OfferMissionFromPort(kOutfitVenue);
    RedrawDialog(outfitDialog);

    do {
        if (gQuit)
            done = 1;
        RunModalDialog(outfitDialog, OutfitFilter, &itemHit, &filterKey);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2) {
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedUnk6C;
            shipData->navTargetKind = savedUnk6A;
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            SetDialogGWorld(outfitDialog);
            RedrawDialog(outfitDialog);
        }
        if (itemHit == 13) {
            n = 0;
            for (count = 0; count < kMaxActiveMissions; count++) {
                if (currentMissionObjectives[count].active != 0
                    && (currentMissionObjectives[count].flags & kMissionHidden) == 0)
                    n++;
            }
            if (n > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateAllPortDialogs();
                SetDialogGWorld(outfitDialog);
                RedrawDialog(portDialog);
            }
        }
        if (itemHit == 8) {
            if (OfferMissionFromPort(kOutfitVenue)) {
                SetDialogGWorld(portDialog);
                UpdateAllPortDialogs();
                SetDialogGWorld(outfitDialog);
            }
        }
        if (itemHit == 7) {
            if (gSelectedOutfit == -1)
                canBuy = 0;
            else
                canBuy = CanBuyOutfitItem(gSelectedOutfit);
            if (canBuy && gSelectedOutfit != -1) {
                count = 1;
                if ((filterKey & 0x800) != 0) {
                    cost = ApplyPriceAndTechnologyFlux(
                        gOutfits[gSelectedOutfit].cost, priceFluxMult[0],
                        gOutfits[gSelectedOutfit].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
                    cost = AdjustedItemCost(&gOutfits[gSelectedOutfit],
                                            &gShipTypes[shipData->type]);
                    if (cost <= 0)
                        qtyLimit = 32000;
                    else {
                        qtyLimit = shipData->credits / cost;
                        if (qtyLimit > 32000)
                            qtyLimit = 32000;
                    }
                    if (gOutfits[gSelectedOutfit].maxCount
                        > shipSystems[gSelectedOutfit]) {
                        if (qtyLimit > gOutfits[gSelectedOutfit].maxCount
                                       - shipSystems[gSelectedOutfit])
                            qtyLimit = gOutfits[gSelectedOutfit].maxCount
                                       - shipSystems[gSelectedOutfit];
                    }
                    if (qtyLimit > 1) {
                        HasMaxOfItem(gSelectedOutfit, 0, &maxAfford);
                        if (maxAfford < qtyLimit)
                            qtyLimit = maxAfford;
                        if (maxAfford - shipSystems[gSelectedOutfit] < qtyLimit)
                            qtyLimit = maxAfford - shipSystems[gSelectedOutfit];
                    }
                    if (qtyLimit > 1) {
                        count = DoQuantityDialog(qtyLimit, 0);
                        OutfitDialogUpdate();
                    }
                }
                for (m = 0; m < count; m++) {
                    if (canBuy) {
                        cost = ApplyPriceAndTechnologyFlux(
                            gOutfits[gSelectedOutfit].cost, priceFluxMult[0],
                            gOutfits[gSelectedOutfit].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
                        cost = AdjustedItemCost(&gOutfits[gSelectedOutfit],
                                                &gShipTypes[shipData->type]);
                        GrantOutfitItem(gSelectedOutfit);
                        shipData->credits -= cost;
                        crashlog_mission_bit_set(
                            "purchasing outfit",
                            (char *)gOutfits[gSelectedOutfit].name, 1,
                            gSelectedOutfit + kFirstResourceID,
                            gOutfits[gSelectedOutfit].onPurchase);
                        if (shipData->credits < 0) {
                            crashlog("warning: player's cash went below zero");
                            shipData->credits = 0;
                            m = count;
                        }
                        EvalMissionBitSetString(gOutfits[gSelectedOutfit].onPurchase);
                    }
                    if (gSelectedOutfit == -1)
                        canBuy = 0;
                    else
                        canBuy = CanBuyOutfitItem(gSelectedOutfit);
                }
                SystemInfoToShipStats();
                if (SetupPortAvailableItems(dockedPort)) {
                    gSelectedOutfit = 0;
                    gOutfitGridPos = -1;
                    outfitScroll = 0;
                    canBuy = canSell = 0;
                    strncpy(gItemDescText, "", 3);
                    RedrawDialog(outfitDialog);
                    m = 0;
                    for (i = 0; i < kMaxOutfits; i++) {
                        if (dockedPortItems[i] != -1) {
                            m++;
                            break;
                        }
                    }
                    if (m == 0)
                        done = 1;
                }
                RedrawDialog(outfitDialog);
                didOutfit = 1;
                if (shipSystems[gSelectedOutfit] > 0) {
                    if ((gOutfits[gSelectedOutfit].flags & kOutfCantSell) != 0)
                        canSell = 0;
                    else
                        canSell = 1;
                } else
                    canSell = 0;
                statusRedraw[2] = 1;
                DrawStatus();
                SetDialogGWorld(outfitDialog);
                DrawOutfitButtons(-1);
            }
        }
        if (itemHit == 4 && canSell) {
            count = 1;
            if ((filterKey & 0x800) != 0) {
                qtyLimit = shipSystems[gSelectedOutfit];
                if (qtyLimit > 32000)
                    qtyLimit = 32000;
                if (qtyLimit > 1) {
                    count = DoQuantityDialog(qtyLimit, 1);
                    OutfitDialogUpdate();
                }
            }
            shown = 0;
            for (n = 0; n < count; n++) {
                if (canSell) {
                    have = AdjustedItemMass(&gOutfits[gSelectedOutfit],
                                            &gShipTypes[shipData->type]);
                    if (have < 0 && FreeMass() + have < 0) {
                        if (!shown) {
                            GetIndString((unsigned char *)msg, kGameStrList, 0xcf);
                            PtoCstr((unsigned char *)msg);
                            strcpy(gTextShowString, msg);
                            ShowTextDialog(0, 0, 0);
                            shown = 1;
                        }
                        canSell = 0;
                    }
                }
                if (canSell) {
                    for (i = 0; i < kOutfMods; i++) {
                        if (gOutfits[gSelectedOutfit].modType[i] == 0x1b
                            && gOutfits[gSelectedOutfit].modVal[i] >= kFirstResourceID
                            && gOutfits[gSelectedOutfit].modVal[i] < kFirstResourceID + kMaxOutfits) {
                            shortfall = shipSystems[gOutfits[gSelectedOutfit].modVal[i] - kFirstResourceID]
                                - ((shipSystems[gSelectedOutfit] - count)
                                          * gOutfits[gOutfits[gSelectedOutfit].modVal[i] - kFirstResourceID].maxCount);
                            if (shortfall > 0) {
                                if (!shown) {
                                    strcpy(msg, "");

                                    AppendIndString(msg, kGameStrList, 0xd0);
                                    strcat(msg, " ");
                                    if (shortfall > 3)
                                        StrcatCommaNumber(shortfall, msg);
                                    else
                                        StrcatOrdinalNumber(shortfall, msg, 0);
                                    strcat(msg, " ");
                                    if (shortfall > 1)
                                        pstrcpy(nameBuf,
                                            lcPluralOutfitName[gOutfits[gSelectedOutfit].modVal[i] - kFirstResourceID]);
                                    else
                                        pstrcpy(nameBuf,
                                            lcOutfitName[gOutfits[gSelectedOutfit].modVal[i] - kFirstResourceID]);
                                    PtoCstr(nameBuf);
                                    strcat(msg, (char *)nameBuf);
                                    strcat(msg, " ");
                                    AppendIndString(msg, kGameStrList, 0xd4);
                                    strcat(msg, " ");
                                    if (n < count - 1)
                                        pstrcpy(nameBuf, lcPluralOutfitName[gSelectedOutfit]);
                                    else
                                        pstrcpy(nameBuf, lcOutfitName[gSelectedOutfit]);
                                    PtoCstr(nameBuf);
                                    strcat(msg, (char *)nameBuf);
                                    strcat(msg, ".");
                                    strcpy(gTextShowString, msg);
                                    ShowTextDialog(0, 0, 0);
                                    shown = 1;
                                }
                                canSell = 0;
                            }
                        }
                    }
                    weapID = -1;
                    for (i = 0; i < kOutfMods; i++) {
                        if (gOutfits[gSelectedOutfit].modType[i] == 1) {
                            weapID = gOutfits[gSelectedOutfit].modVal[i];
                            break;
                        }
                    }
                    if (weapID != -1 && gWeapons[weapID].maxAmmo > 0) {
                        if (gWeapons[weapID].ammoType >= 0
                            && gWeapons[weapID].ammoType < 256
                            && gWeapons[weapID].guidance != kGuidanceBay)
                            ammo = shipData->weap[gWeapons[weapID].ammoType].ammo;
                        else
                            ammo = shipData->weap[weapID].ammo;
                        if (gWeapons[weapID].guidance == kGuidanceBay) {
                            for (m = 1; m < kMaxShips; m++) {
                                if (shipData[m].active != 0
                                    && shipData[m].parentShip == 0
                                    && shipData[m].aiType == 5
                                    && shipData[m].type
                                       == gWeapons[weapID].ammoType - kFirstResourceID
                                    && !IsDisabled(&shipData[m]))
                                    ammo++;
                            }
                        }
                        shortfall = ammo
                            - ((shipSystems[gSelectedOutfit] - (n + 1))
                                      * gWeapons[weapID].maxAmmo);
                        if (shortfall > 0 && ammo > 0) {
                            if (!shown) {
                                strcpy(msg, "");

                                AppendIndString(msg, kGameStrList, 0xd0);
                                strcat(msg, " ");
                                if (shortfall > 3)
                                    StrcatCommaNumber(shortfall, msg);
                                else
                                    StrcatOrdinalNumber(shortfall, msg, 0);
                                strcat(msg, " ");
                                k = -1;
                                for (m = 0; m < kMaxOutfits; m++) {
                                    for (j = 0; j < kOutfMods; j++) {
                                        if (gOutfits[m].modType[j] == 3
                                            && gOutfits[m].modVal[j] == weapID) {
                                            k = m;
                                            break;
                                        }
                                    }
                                    if (k != -1)
                                        break;
                                }
                                if (k != -1) {
                                    if (shortfall > 1)
                                        pstrcpy(nameBuf, lcPluralOutfitName[k]);
                                    else
                                        pstrcpy(nameBuf, lcOutfitName[k]);
                                    PtoCstr(nameBuf);
                                    strcat(msg, (char *)nameBuf);
                                } else {
                                    if (shortfall > 1)
                                        AppendIndString(msg, kGameStrList, 0xd2);
                                    else
                                        AppendIndString(msg, kGameStrList, 0xd1);
                                    strcat(msg, " ");
                                    AppendIndString(msg, kGameStrList, 0xd3);
                                }
                                strcat(msg, " ");
                                AppendIndString(msg, kGameStrList, 0xd4);
                                strcat(msg, " ");
                                if (n < count - 1)
                                    pstrcpy(nameBuf, lcPluralOutfitName[gSelectedOutfit]);
                                else
                                    pstrcpy(nameBuf, lcOutfitName[gSelectedOutfit]);
                                PtoCstr(nameBuf);
                                strcat(msg, (char *)nameBuf);
                                strcat(msg, ".");
                                strcpy(gTextShowString, msg);
                                ShowTextDialog(0, 0, 0);
                                shown = 1;
                            }
                            canSell = 0;
                        }
                    }
                }
                if (canSell) {
                    cost = ApplyPriceAndTechnologyFlux(
                        gOutfits[gSelectedOutfit].cost, priceFluxMult[0],
                        gOutfits[gSelectedOutfit].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
                    cost = AdjustedItemCost(&gOutfits[gSelectedOutfit],
                                            &gShipTypes[shipData->type]);
                    if (shipSystems[gSelectedOutfit]
                        <= savedOutfits[gSelectedOutfit])
                        cost *= 0.5;
                    have = FreeMass();
                    shipSystems[gSelectedOutfit] -= 1;
                    if (FreeMass() >= 0 || have < 0) {
                        shipData->credits += cost;
                        for (j = 0; j < kOutfMods; j++) {
                            if (shipSystems[gSelectedOutfit] <= 0) {
                                if (gOutfits[gSelectedOutfit].modType[j] == 1
                                    && shipData->selectedWeapon
                                       == gOutfits[gSelectedOutfit].modVal[j]) {
                                    shipData->selectedWeapon = -1;
                                    statusRedraw[3] = 1;
                                    break;
                                }
                            }
                        }
                        crashlog_mission_bit_set(
                            "selling outfit",
                            (char *)gOutfits[gSelectedOutfit].name, 1,
                            gSelectedOutfit + kFirstResourceID,
                            gOutfits[gSelectedOutfit].onSell);
                        EvalMissionBitSetString(gOutfits[gSelectedOutfit].onSell);
                        SystemInfoToShipStats();
                        if (SetupPortAvailableItems(dockedPort)) {
                            gSelectedOutfit = 0;
                            gOutfitGridPos = -1;
                            outfitScroll = 0;
                            canBuy = canSell = 0;
                            strncpy(gItemDescText, "", 3);
                            RedrawDialog(outfitDialog);
                            k = 0;
                            for (n = 0; n < kMaxOutfits; n++) {
                                if (dockedPortItems[n] != -1) {
                                    k++;
                                    break;
                                }
                            }
                            if (k == 0)
                                done = 1;
                        }
                    } else {
                        shipSystems[gSelectedOutfit] += 1;
                        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                        break;
                    }
                }
                if (gSelectedOutfit != -1 && shipSystems[gSelectedOutfit] > 0) {
                    if ((gOutfits[gSelectedOutfit].flags & kOutfCantSell) != 0)
                        canSell = 0;
                    else
                        canSell = 1;
                } else
                    canSell = 0;
            }
            if (gSelectedOutfit == -1)
                canBuy = 0;
            else
                canBuy = CanBuyOutfitItem(gSelectedOutfit);
            RedrawDialog(outfitDialog);
            didOutfit = 1;
            statusRedraw[2] = 1;
            DrawStatus();
            SetDialogGWorld(outfitDialog);
            DrawOutfitButtons(-1);
        }
        if (itemHit == 10 && outfitScroll >= kGridCols) {
            outfitScroll -= kGridCols;
            if (gOutfitGridPos >= 0 && gOutfitGridPos < kGridCells - kGridCols) {
                gOutfitGridPos += kGridCols;
                gSelectedOutfit =
                    dockedPortItems[outfitScroll + gOutfitGridPos];
                LoadDescription(gItemDescText, gSelectedOutfit + kOutfitDescBase);
            } else {
                gSelectedOutfit = gOutfitGridPos = -1;
                strncpy(gItemDescText, "", 3);
            }
            SetDialogGWorld(outfitDialog);
            RedrawDialog(outfitDialog);
        }
        if (itemHit == 11) {
            m = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (dockedPortItems[i] != -1)
                    m++;
            }
            if (outfitScroll < m - kGridCells) {
                outfitScroll += kGridCols;
                if (gOutfitGridPos < kGridCols) {
                    gSelectedOutfit = gOutfitGridPos = -1;
                    strncpy(gItemDescText, "", 3);
                } else {
                    gOutfitGridPos -= kGridCols;
                    gSelectedOutfit =
                        dockedPortItems[outfitScroll + gOutfitGridPos];
                    LoadDescription(gItemDescText, gSelectedOutfit + kOutfitDescBase);
                }
                SetDialogGWorld(outfitDialog);
                RedrawDialog(outfitDialog);
            }
        }
    } while (!done);

    k = ShipFuelCapacity(shipData);
    if (shipData->fuel > k) {
        shipData->fuel = k;
        statusRedraw[4] = 1;
    }
    i = ShipShieldCapacity(shipData);
    if (shipData->shield > i) {
        shipData->shield = i;
        statusRedraw[1] = 1;
    }
    j = ShipArmorCapacity(shipData);
    if (shipData->armor > j) {
        shipData->armor = j;
        statusRedraw[1] = 1;
    }
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0 && (gOutfits[i].flags & kOutfRemoveAfterBuy) != 0)
            shipSystems[i] = 0;
    }
    SystemInfoToShipStats();
    DisposeDialogNoFlash(outfitDialog);
    outfitDialog = NULL;
    if (gOutfitDialogPict != NULL) {
        QDReleaseResource(gOutfitDialogPict);
        gOutfitDialogPict = NULL;
    }
    if (previewPic != NULL) {
        QDReleaseResource(previewPic);
        previewPic = NULL;
    }
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    RGBForeColor(blackColor);
    RedrawStatus(0);
    SWSetPortToWindow(gSpriteWorld);
    shipData->targetStellar = savedUnk6C;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}

/* OutfitFilter. STATE: MATCH 100%, 562/562, relocregcheck 0 of 154.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004903c0
char OutfitFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    Point where;
    int c;
    short i;
    short count;
    long slot;
    short old;
    short oldPos;
    short col;
    short row;
    short button;
    int what;

    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 1;
        return 1;
    }
    *itemHit = -1;
    if (gSelectedOutfit == -1) {
        canSell = 0;
        canBuy = 0;
    } else {
        canBuy = CanBuyOutfitItem(gSelectedOutfit);
        if (shipSystems[gSelectedOutfit] > 0) {
            if ((gOutfits[gSelectedOutfit].flags & kOutfCantSell) != 0)
                canSell = 0;
            else
                canSell = 1;
        } else {
            canSell = 0;
        }
    }
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 2;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 13;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        if (((int)ev->what == kKeyDown && c == kReturnChar) || c == kEscapeChar) {
            ev->what = kMouseDown;
            *itemHit = 1;
            return 1;
        }
        c = tolower(c);
        if (c == 'b') {
            ev->what = kMouseDown;
            *itemHit = 7;
            return 1;
        }
        if (c == 's') {
            ev->what = kMouseDown;
            *itemHit = 4;
            return 1;
        }
        if (c == kLeftArrowChar || c == kRightArrowChar || c == kUpArrowChar ||
            c == kDownArrowChar || c == kPageUpChar || c == kPageDownChar) {
            old = gSelectedOutfit;
            if (gOutfitGridPos != -1) {
                count = 0;
                for (i = 0; i < 512; i++) {
                    if (dockedPortItems[i] != -1)
                        count++;
                }
                if (c == kRightArrowChar) {
                    if (gOutfitGridPos < kGridCells - 1) {
                        if (dockedPortItems[outfitScroll + gOutfitGridPos
                                            + 1] != -1)
                            gOutfitGridPos++;
                    } else if (outfitScroll < count - kGridCells) {
                        outfitScroll += kGridCols;
                        gOutfitGridPos -= kGridCols - 1;
                    }
                } else if (c == kLeftArrowChar) {
                    if (gOutfitGridPos > 0) {
                        gOutfitGridPos--;
                    } else if (outfitScroll > 0) {
                        outfitScroll -= kGridCols;
                        gOutfitGridPos += kGridCols - 1;
                    }
                } else if (c == kUpArrowChar || c == kPageUpChar) {
                    if (gOutfitGridPos < kGridCols) {
                        if (outfitScroll > 0)
                            outfitScroll -= kGridCols;
                    } else {
                        gOutfitGridPos -= kGridCols;
                    }
                } else if (c == kDownArrowChar || c == kPageDownChar) {
                    if (gOutfitGridPos < kGridCells - kGridCols) {
                        if (dockedPortItems[outfitScroll + gOutfitGridPos
                                            + kGridCols] != -1)
                            gOutfitGridPos += kGridCols;
                    } else if (outfitScroll < count - kGridCells) {
                        outfitScroll += kGridCols;
                    }
                }
                slot = gOutfitGridPos;
                slot += outfitScroll;
                gSelectedOutfit = dockedPortItems[slot];
                while (gSelectedOutfit == -1 && gOutfitGridPos > 0) {
                    slot--;
                    gOutfitGridPos--;
                    gSelectedOutfit = dockedPortItems[slot];
                }
            } else {
                if (c == kRightArrowChar || c == kDownArrowChar ||
                    c == kPageDownChar)
                    gOutfitGridPos = 0;
                else
                    gOutfitGridPos = kGridCells - 1;
                what = gOutfitGridPos;
                what += outfitScroll;
                gSelectedOutfit = dockedPortItems[what];
                while (gSelectedOutfit == -1 && gOutfitGridPos > 0) {
                    what--;
                    gOutfitGridPos--;
                    gSelectedOutfit = dockedPortItems[what];
                }
            }
            if (gSelectedOutfit != old) {
                if (gOutfitGridPos == -1) {
                    canBuy = canSell = 0;
                } else {
                    LoadDescription(gItemDescText,
                                    gSelectedOutfit + kOutfitDescBase);
                    RedrawDialog(outfitDialog);
                    canBuy = CanBuyOutfitItem(gSelectedOutfit);
                    if (shipSystems[gSelectedOutfit] > 0) {
                        if ((gOutfits[gSelectedOutfit].flags
                             & kOutfCantSell) != 0)
                            canSell = 0;
                        else
                            canSell = 1;
                    } else {
                        canSell = 0;
                    }
                }
            }
        }
    }
    what = ev->what;
    if (what == kMouseDown) {
        SetDialogGWorld(outfitDialog);
        RedrawDialog(outfitDialog);
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            col = (where.h - box.left) / kGridCellWide;
            row = (where.v - box.top) / kGridCellHigh;
            if (col >= 0 && col <= kGridCols - 1 && row >= 0 &&
                row <= kGridCells / kGridCols - 1) {
                oldPos = gOutfitGridPos;
                gOutfitGridPos = row * kGridCols + col;
                if (dockedPortItems[gOutfitGridPos + outfitScroll] == -1) {
                    gOutfitGridPos = gSelectedOutfit = -1;
                } else {
                    gSelectedOutfit =
                        dockedPortItems[gOutfitGridPos + outfitScroll];
                }
                if (gOutfitGridPos != oldPos) {
                    SetDialogGWorld(outfitDialog);
                    InvalDialogItem(outfitDialog, 1);
                    InvalDialogItem(outfitDialog, 4);
                    InvalDialogItem(outfitDialog, 7);
                    InvalDialogItem(outfitDialog, 5);
                    InvalDialogItem(outfitDialog, 6);
                    InvalDialogItem(outfitDialog, 8);
                    InvalDialogItem(outfitDialog, 9);
                    InvalDialogItem(outfitDialog, 10);
                    InvalDialogItem(outfitDialog, 11);
                    if (gSelectedOutfit != -1) {
                        canBuy = CanBuyOutfitItem(gSelectedOutfit);
                        if (shipSystems[gSelectedOutfit] > 0) {
                            if ((gOutfits[gSelectedOutfit].flags
                                 & kOutfCantSell) != 0)
                                canSell = 0;
                            else
                                canSell = 1;
                        } else {
                            canSell = 0;
                        }
                        LoadDescription(gItemDescText,
                                        gSelectedOutfit + kOutfitDescBase);
                    } else {
                        canBuy = canSell = 0;
                        strncpy(gItemDescText, "", 3);
                    }
                }
                gLastClickPos = gOutfitGridPos;
                gLastClickTime = ev->when;
                *itemHit = -1;
                return 1;
            }
        }
        OutfitDialogUpdate();
        button = TrackOutfitButtons(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 7;
            return 1;
        }
        if (button == 2) {
            *itemHit = 4;
            return 1;
        }
        if (button == 3) {
            *itemHit = 10;
            return 1;
        }
        if (button == 4) {
            *itemHit = 11;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt) {
        OutfitDialogUpdate();
        return 1;
    }
    if (ReadTick() >= missionOfferTime) {
        *itemHit = 8;
        missionOfferTime = ReadTick() + Rand(30) + 30;
        return 1;
    }
    return 0;
}

/* OutfitDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00490c70
void OutfitDialogUpdate(void)
{
    int c;
    RGBColor *cellColor;
    short drawn;
    short skipped;
    void *item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short i;
    long mass;
    long freeMass;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(outfitDialog));
    if (gOutfitDialogPict != NULL)
        DrawPicHandle(gOutfitDialogPict, GetDialogPortRect(outfitDialog));
    RGBForeColor(blackColor);
    skipped = 0;
    GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(outfitDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(blackColor);
        drawn = 0;
        i = outfitScroll;
        if (i < 512) {
            cellColor = &shopGridColor[1];
            do {
            if (drawn < kGridCells && i >= 0 && dockedPortItems[i] != -1) {
                if (skipped < 3) {
                    if (i - outfitScroll >= 0 &&
                        i - outfitScroll < kGridCells) {
                        if (!DrawCachedPict(dockedPortItems[i] + kOutfitPictBase,
                                            &loadPort,
                                            &gIconRects[i - outfitScroll]))
                            skipped++;
                    } else {
                        skipped++;
                    }
                } else {
                    InvalDialogRect(outfitDialog, &box);
                    break;
                }
                if (shipSystems[dockedPortItems[i]] > 0) {
                    NumToString(shipSystems[dockedPortItems[i]], line1);
                    RGBForeColor(whiteColor);
                    MoveTo((gCellRects[i - outfitScroll].right
                                   - (StringWidth(line1) + 3)),
                           (gCellRects[i - outfitScroll].top + 12));
                    DrawString(line1);
                    RGBForeColor(blackColor);
                }
                if (cheatsActive) {
                    NumToString(dockedPortItems[i] + kFirstResourceID, line1);
                    RGBForeColor(whiteColor);
                    MoveTo(((gCellRects[i - outfitScroll].left
                                    + gCellRects[i - outfitScroll].right
                                    - StringWidth(line1)) / 2),
                           (gCellRects[i - outfitScroll].top + 24));
                    DrawString(line1);
                    RGBForeColor(blackColor);
                }
                if (SplitTwoLineString(gOutfShortNames[dockedPortItems[i]],
                                       line1, line2)) {
                    c = line1[1];
                    if (isalpha(c) || isdigit(c))
                        RGBForeColor(whiteColor);
                    else
                        RGBForeColor(&rgbLessDarkGrey);
                    MoveTo(((gCellRects[i - outfitScroll].left
                                    + gCellRects[i - outfitScroll].right) / 2
                                   - StringWidth(line1) / 2),
                           (gCellRects[i - outfitScroll].bottom - 14));
                    DrawString(line1);
                    c = line2[1];
                    if (isalpha(c) || isdigit(c))
                        RGBForeColor(whiteColor);
                    else
                        RGBForeColor(&rgbLessDarkGrey);
                    MoveTo(((gCellRects[i - outfitScroll].left
                                    + gCellRects[i - outfitScroll].right) / 2
                                   - StringWidth(line2) / 2),
                           (gCellRects[i - outfitScroll].bottom - 3));
                    DrawString(line2);
                } else {
                    MoveTo(((gCellRects[i - (long)outfitScroll].left
                                    + gCellRects[i - (long)outfitScroll].right) / 2
                                   - StringWidth(line1) / 2),
                           (gCellRects[i - (long)outfitScroll].bottom - 6));
                    RGBForeColor(whiteColor);
                    DrawString(line1);
                }
                RGBForeColor(blackColor);
                RGBForeColor(cellColor);
                FrameRect(&gCellRects[i - outfitScroll]);
                RGBForeColor(blackColor);
                drawn++;
            }
            i++;
            } while (i < 512);
        }
        if (gOutfitGridPos != -1) {
            RGBForeColor(&shopGridColor[0]);
            FrameRect(&gCellRects[gOutfitGridPos]);
            RGBForeColor(blackColor);
        }
    }
    if (gOutfitGridPos != -1) {
        GetDialogItem(outfitDialog, 8, &itemType, &item, &box);
        if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(outfitDialog)))) {
            if (previewPicID != gSelectedOutfit + kOutfitPictBase) {
                if (previewPic != NULL) {
                    QDReleaseResource(previewPic);
                    previewPic = NULL;
                }
                previewPicID = gSelectedOutfit + kOutfitPictBase;
                if (previewPicID >= kFirstResourceID)
                    previewPic = GetPicture(previewPicID);
            }
            if (previewPic != NULL) {
                RGBForeColor(blackColor);
                DrawPicHandle(previewPic, &box);
            } else {
                RGBForeColor(whiteColor);
                TextFont(smallTextFont);
                TextSize(smallTextSize);
                GetIndString(line1, kGameStrList, 0xd5);
                DrawStringCentered(line1, box.left, box.right,
                                   ((box.bottom + box.top) / 2 - 6));
                GetIndString(line1, kGameStrList, 0xd6);
                DrawStringCentered(line1, box.left, box.right,
                                   ((box.bottom + box.top) / 2 + 6));
                RGBForeColor(blackColor);
            }
        }
        GetDialogItem(outfitDialog, 9, &itemType, &item, &box);
        if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(outfitDialog)))) {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            RGBForeColor(whiteColor);
            MoveTo(box.left, (box.top + 12));
            DrawIndString(kGameStrList, 0xd7);
            MoveTo((box.left + 0x46), (box.top + 12));
            ApplyPriceAndTechnologyFlux(gOutfits[gSelectedOutfit].cost, priceFluxMult[0],
                            gOutfits[gSelectedOutfit].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
            DrawCommaNumber(AdjustedItemCost(&gOutfits[gSelectedOutfit],
                                             &gShipTypes[shipData->type]));
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x22);
            MoveTo(box.left, (box.top + 0x18));
            DrawIndString(kGameStrList, 0xd9);
            MoveTo((box.left + 0x46), (box.top + 0x18));
            DrawCommaNumber(shipData->credits);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x22);
            if (gOutfits[gSelectedOutfit].mass > 0) {
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xd8);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                mass = AdjustedItemMass(&gOutfits[gSelectedOutfit],
                                        &gShipTypes[shipData->type]);
                DrawCommaNumber(mass);
                DrawString("\p ");
                if (mass == 1)
                    DrawString(tonString);
                else
                    DrawString(tonsString);
                MoveTo(box.left, (box.top + 0x3c));
                DrawIndString(kGameStrList, 0xda);
                MoveTo((box.left + 0x46), (box.top + 0x3c));
                freeMass = FreeMass();
                if (freeMass < 0)
                    freeMass = 0;
                DrawCommaNumber(freeMass);
                DrawString("\p ");
                if (freeMass == 1)
                    DrawString(tonString);
                else
                    DrawString(tonsString);
            }
            MoveTo(box.left, (box.top + 0x5d));
            if (HasMaxOfItem(gSelectedOutfit, NULL, NULL)) {
                if (shipSystems[gSelectedOutfit] > 0)
                    DrawIndString(kGameStrList, 0xdb);
                else
                    DrawIndString(kGameStrList, 0xdc);
            } else {
                mass = AdjustedItemMass(&gOutfits[gSelectedOutfit],
                                        &gShipTypes[shipData->type]);
                if (mass > 0 && mass > FreeMass()) {
                    if (shipSystems[gSelectedOutfit] > 0)
                        DrawIndString(kGameStrList, 0xdd);
                    else
                        DrawIndString(kGameStrList, 0xde);
                }
            }
            RGBForeColor(blackColor);
        }
    }
    GetDialogItem(outfitDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(outfitDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(gItemDescText, strlen(gItemDescText), &box, 0);
        InvertRect(&box);
    }
    DrawOutfitButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(outfitDialog);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(outfitDialog)),
             GetDialogPortRect(outfitDialog), GetDialogPortRect(outfitDialog), 0,
             NULL);
}

#define kSpecialTechCount  8

#define kPortDlogID         0x3e8
#define kPriceMultipliers   2
#define kPortVenue          3
#define kMissionOfferSpread 30
#define kLandTimeSteps      3
#define kAllButtonsOff      -8

/* spob Flags; resources.h already names kSpobUninhabited and kSpobHasBar */
#define kSpobHasTrade       0x00000002UL
#define kSpobHasOutfitter   0x00000004UL

#define SpobHasTrade(s)     ((s)->flags & kSpobHasTrade)
#define SpobHasOutfitter(s) ((s)->flags & kSpobHasOutfitter)
#define SpobAmbientLoop(s)  ((s)->flags2 & kSpobAmbientLoop)



/* CanBuyOutfitItem (mac _CanBuyOutfitItem). STATE: MATCH, 391/391, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00491950
char CanBuyOutfitItem(short outfit)
{
    long freeMass;
    long mass;
    char isMap;
    char can;
    char found;
    char isCargo;
    char isFakeID;
    char avail;
    short cargoDelta;
    short i;
    short stellar;

    if (outfit == -1) {
        can = 0;
    } else {
        freeMass = FreeMass();
        if (gOutfits[outfit].techLevel != -1) {
            found = 0;
            if (gOutfits[outfit].techLevel >
                    stellarData[shipData[0].targetStellar].techLevel) {
                for (i = 0; i < kSpecialTechCount; i++) {
                    if (gOutfits[outfit].techLevel ==
                            stellarData[shipData[0].targetStellar]
                                .specialTech[i]) {
                        found = 1;
                        break;
                    }
                }
            } else {
                found = 1;
            }
            if (!found)
                return 0;
        }
        mass = AdjustedItemMass(&gOutfits[outfit],
                                &gShipTypes[shipData[0].type]);
        if (!HasMaxOfItem(outfit, 0, 0)
                && (freeMass >= mass || mass <= 0)) {
            isCargo = 0;
            cargoDelta = 0;
            isFakeID = 0;
            isMap = 0;
            for (i = 0; i < kOutfMods; i++) {
                if (gOutfits[outfit].modType[i] == kOutfModCargo) {
                    isCargo = 1;
                    cargoDelta = gOutfits[outfit].modVal[i];
                }
                if (gOutfits[outfit].modType[i] == kModTypeMap)
                    isMap = 1;
                if (gOutfits[outfit].modType[i] == kModTypeFakeID)
                    isFakeID = 1;
            }
            if (gOutfits[outfit].modType[0] == kOutfModAmmo) {
                if (gOutfits[outfit].modVal[0] >= 0) {
                    if (gWeapons[gOutfits[outfit].modVal[0]].guidance ==
                            kGuidanceBay) {
                        if (CanBuyFighter(
                                gWeapons[gOutfits[outfit].modVal[0]].ammoType
                                    - kFirstResourceID,
                                outfit, gOutfits[outfit].modVal[0]))
                            can = 1;
                        else
                            can = 0;
                    } else {
                        can = 1;
                    }
                } else {
                    can = 1;
                }
            } else if (isCargo) {
                if (cargoDelta >= 0) {
                    can = 1;
                } else if (gShipTypes[shipData[0].type].cargoNotFixed == 0) {
                    can = 0;
                } else if (ShipTotalHolds() - TotalCargo(shipData) >=
                        abs(cargoDelta)) {
                    can = 1;
                } else {
                    can = 0;
                }
            } else if (isMap) {
                if (boughtMap == 0)
                    can = 1;
                else
                    can = 0;
            } else if (isFakeID) {
                if (boughtID == 0)
                    can = 1;
                else
                    can = 0;
            } else {
                can = 1;
            }
        } else {
            can = 0;
        }
        if (can) {
            avail = 1;
            if (playerIsLanded != 0) {
                stellar = shipData[0].targetStellar;
                if (stellar != -1) {
                    if (InGovtClass(gOutfits[outfit].govtRestrict, 0)) {
                        avail = 0;
                        if (stellarData[stellar].govt ==
                                gOutfits[outfit].govtRestrict - ResClassBase(0))
                            avail = 1;
                        else if (stellarData[stellar].govt != -1
                                 && GovtAllies(stellarData[stellar].govt,
                                               gOutfits[outfit].govtRestrict
                                                   - ResClassBase(0)))
                            avail = 1;
                    } else if (InGovtClass(gOutfits[outfit].govtRestrict, 1)) {
                        avail = 0;
                        if (stellarData[stellar].govt ==
                                gOutfits[outfit].govtRestrict - ResClassBase(1)) {
                            avail = 1;
                        } else if (stellarData[stellar].govt != -1) {
                            if (GovtAllies(stellarData[stellar].govt,
                                           gOutfits[outfit].govtRestrict
                                               - ResClassBase(1)))
                                avail = 1;
                        } else {
                            avail = 1;
                        }
                    } else if (InGovtClass(gOutfits[outfit].govtRestrict, 2)) {
                        avail = 1;
                        if (stellarData[stellar].govt ==
                                gOutfits[outfit].govtRestrict - ResClassBase(2))
                            avail = 0;
                        else if (stellarData[stellar].govt != -1
                                 && GovtAllies(stellarData[stellar].govt,
                                               gOutfits[outfit].govtRestrict
                                                   - ResClassBase(2)))
                            avail = 0;
                    } else if (InGovtClass(gOutfits[outfit].govtRestrict, 3)) {
                        avail = 1;
                        if (stellarData[stellar].govt ==
                                gOutfits[outfit].govtRestrict - ResClassBase(3)) {
                            avail = 0;
                        } else if (stellarData[stellar].govt != -1) {
                            if (GovtAllies(stellarData[stellar].govt,
                                           gOutfits[outfit].govtRestrict
                                               - ResClassBase(3)))
                                avail = 0;
                        } else {
                            avail = 0;
                        }
                    }
                }
            }
            if (avail) {
                if (!PlayerMeetsRequirements(gOutfits[outfit].requires[0],
                                             gOutfits[outfit].requires[1]))
                    can = 0;
            }
        }
        if (can) {
            if (!EvalMissionBitTestString(gOutfits[outfit].availability))
                can = 0;
        }
        if (can) {
            ApplyPriceAndTechnologyFlux(gOutfits[gSelectedOutfit].cost, priceFluxMult[0],
                            gOutfits[gSelectedOutfit].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
            if (shipData[0].credits <
                    AdjustedItemCost(&gOutfits[gSelectedOutfit],
                                     &gShipTypes[shipData[0].type]))
                can = 0;
        }
    }
    return can;
}

/* DoPortDialog (mac _DoPortDialog). STATE: MATCH 561/561, byte-identical yes (2413 bytes),
   relocregcheck 0 of 237. `n' holds the fuel call's result and counts the shown missions;
   the sound handle is stored straight into gAmbientSound. OPEN: none. */
// FUNCTION: EVNOVA 0x00491f30
void DoPortDialog(SpobRecord *spob, short stellar)
{
    short i;
    char quit;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short n;
    short j;
    unsigned short vol;

    quit = 0;
    if (screenFaded != 0)
        return;
    onPlanet = 1;
    dockedPort = spob;
    didOutfit = didBuyShip = 0;
    strncpy(gDescText, "", 5);
    LoadDescription(gDescText, stellar + kFirstResourceID);
    for (j = 0; j < kPriceMultipliers; j++)
        priceFluxMult[j] = 1.0f;
    if (spob->govt != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0
                    && GovtAllies(spob->govt, rankData[i].govt)) {
                for (j = 0; j < kPriceMultipliers; j++)
                    priceFluxMult[j] *= rankData[i].priceMod
                                        * kPortPriceScale;
            }
        }
    }
    portDialog = NULL;
    portDialog = GetNewDialog(kPortDlogID, UpdateStellarDialog,
                                  StellarDialogEvent);
    if (portDialog == NULL)
        return;
    if (spob->customSound >= kFirstResourceID && planetSoundsPref != 0) {
        gAmbientSound = LoadSoundResource(spob->customSound);
        planetSoundSpec.handle = (void **)gAmbientSound;
        vol = gSoundVolume;
        planetSoundSpec.leftLevel = vol;
        planetSoundSpec.rightLevel = vol;
        gAmbientDeadline = 0;
    } else {
        gAmbientSound = NULL;
    }
    CenterWindow(FrontWindow(portDialog), gWind);
    if (SpobUninhabited(dockedPort) != 0)
        gSpobInhabited = 0;
    else
        gSpobInhabited = 1;
    SetDialogGWorld(portDialog);
    BackColor(blackColor);
    ShowWindowNoFlash(FrontWindow(portDialog));
    BackColor(whiteColor);
    SetDialogGWorld(portDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RGBForeColor(blackColor);
    RedrawDialog(portDialog);
    UpdateStellarDialog();
    FrontWindow(portDialog);
    FrontWindow(portDialog);
    SWSetPortToWorkArea(gSpriteWorld);
    RGBForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    SWSetPortToWindow(gSpriteWorld);
    DrawStatusRadar(1);
    DrawStatusCargo();
    SetDialogGWorld(portDialog);
    UpdateAllPortDialogs();
    MissionLandCheck();
    HandleStellarSystemVisibility();
    DrawStatusCargo();
    UpdateAllPortDialogs();
    missionListVenue = kPortVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(kMissionOfferSpread)
                       + kMissionOfferSpread;
    OfferMissionFromPort(kPortVenue);
    do {
        if (gQuit)
            quit = 1;
        RunModalDialog(portDialog, StellarDialogEvent, &itemHit, 0);
        if (gQuit)
            quit = 1;
        if (itemHit == 12) {
            quit = 1;
        } else if (itemHit == 2) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        } else if (itemHit == 3) {
            savedStellar = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            UpdateAllPortDialogs();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        } else if (itemHit == 4) {
            if ((char)SpobUninhabited(dockedPort) == 0) {
                n = ShipFuelCapacity(shipData);
                shipData->fuel = (short)shipData->fuel;
                n -= shipData->fuel;
                if (dockedPort->dominated == 0) {
                    if (n > shipData->credits)
                        n = shipData->credits;
                    shipData->credits -= n;
                }
                shipData->fuel += n;
                statusRedraw[4] = 1;
                statusRedraw[2] = 1;
                gSpriteWorld->fastBlit = 1;
                DrawStatus();
                gSpriteWorld->fastBlit = 0;
                SetDialogGWorld(portDialog);
            }
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        } else if (itemHit == 5) {
            if (gAmbientSound != NULL
                    && SpobAmbientLoop(dockedPort) == 0)
                ST_PlaySoundParam(&planetSoundSpec);
        } else if (itemHit == 7) {
            if (SpobHasTrade(dockedPort) != 0) {
                SetDialogGWorld(portDialog);
                DrawPlanetButtons(kAllButtonsOff);
                DoTradeDialog();
                statusRedraw[2] = 1;
                DrawStatus();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 8) {
            if (SpobHasOutfitter(dockedPort) != 0) {
                InvalDialogItem(portDialog, 8);
                UpdateAllPortDialogs();
                DoOutfitDialog();
                statusRedraw[2] = 1;
                DrawStatus();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 9) {
            if (SpobHasShipyard(dockedPort) != 0) {
                InvalDialogItem(portDialog, 9);
                UpdateAllPortDialogs();
                shipBuyMode = 0;
                DoShipyardDialog();
                statusRedraw[2] = 1;
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 10) {
            if (gSpobInhabited != 0) {
                savedStellar = shipData->targetStellar;
                savedRoute = shipData->navTargetKind;
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateAllPortDialogs();
                reenterMissionDialog = 0;
                do {
                    DoMissionDialog();
                    statusRedraw[2] = 1;
                    statusRedraw[0] = 1;
                    DrawStatus();
                } while (reenterMissionDialog != 0);
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                shipData->targetStellar = savedStellar;
                shipData->navTargetKind = savedRoute;
                statusRedraw[2] = 1;
                statusRedraw[0] = 1;
                DrawStatus();
            }
        } else if (itemHit == 11) {
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            if (SpobHasBar(dockedPort) != 0) {
                DoBarDialog();
                DrawStatusCargo();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 14) {
            n = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0
                    && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
                    n++;
            }
            if (n > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 15) {
            if (OfferMissionFromPort(kPortVenue)) {
                SetDialogGWorld(portDialog);
                UpdateAllPortDialogs();
            }
        }
    } while (quit == 0);
    DoEscortLand();
    if (gStellarImage.world != NULL)
        DisposeGWorldContainer(&gStellarImage);
    if (gStellarDialogPict != NULL) {
        QDReleaseResource(gStellarDialogPict);
        gStellarDialogPict = NULL;
    }
    if (gAmbientSound != NULL) {
        ST_HaltSound(gAmbientSound);
        ST_DisposeSound(gAmbientSound);
    }
    if (didOutfit != 0)
        IncrementGameTime();
    if (didBuyShip != 0) {
        for (i = 0; i <= kLandTimeSteps; i++)
            IncrementGameTime();
    }
    if (didOutfit != 0 || didBuyShip != 0) {
        shipData->targetShip = -1;
        statusRedraw[3] = 1;
    }
    DisposeDialogNoFlash(portDialog);
    portDialog = NULL;
    onPlanet = 0;
}

#define kActivateEvt  8

#define kAmbientPeriod   480


#define kNameBaseline 0x12
#define kNameSize     0x12

/*
 * A click inside item 5 - the planet image - reports the image rather than
 * whatever button the tracker thought it was over.
 */

/* StellarDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004928a0
char StellarDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    Point where;
    short button;
    int c;

    *itemHit = -1;
    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 12;
        return 1;
    }
    if (SpobUninhabited(dockedPort))
        gSpobInhabited = 0;
    else
        gSpobInhabited = 1;
    if ((int)ev->what == kActivateEvt && gAmbientDeadline != 0)
        gAmbientDeadline = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
    if (gAmbientSound != NULL) {
        if ((dockedPort->flags2 & kSpobAmbientLoop) != 0) {
            if (ST_IsSoundPlaying(gAmbientSound) == 0)
                ST_PlaySoundParam(&planetSoundSpec);
        } else if (ReadTick() >= gAmbientDeadline) {
            ST_PlaySoundParam(&planetSoundSpec);
            gAmbientDeadline = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
        }
    }
    *itemHit = -1;
    if ((int)ev->what == kActivateEvt)
        gAmbientDeadline = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
    if ((int)ev->what == kKeyDown) {
        if (KeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 3;
            return 1;
        }
        if (KeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 2;
            return 1;
        }
        if (KeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 14;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        c = tolower(c);
        if (c == kReturnChar || c == kEscapeChar) {
            ev->what = kMouseDown;
            *itemHit = 12;
            return 1;
        }
        if (c == 'r' || c == 'f') {
            *itemHit = 4;
            return 1;
        }
        if (c == 'c' || c == 't') {
            *itemHit = 7;
            return 1;
        }
        if (c == 'o') {
            *itemHit = 8;
            return 1;
        }
        if (c == 's') {
            *itemHit = 9;
            return 1;
        }
        if (c == 'n') {
            *itemHit = 10;
            return 1;
        }
        if (c == 'b') {
            *itemHit = 11;
            return 1;
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdateStellarDialog();
        button = TrackPlanetButtons(where);
        GetDialogItem(portDialog, 5, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            *itemHit = 5;
            return 1;
        }
        if (button == 0) {
            *itemHit = 12;
            return 1;
        }
        if (button == 1) {
            *itemHit = 4;
            return 1;
        }
        if (button == 2) {
            *itemHit = 7;
            return 1;
        }
        if (button == 3) {
            *itemHit = 8;
            return 1;
        }
        if (button == 4) {
            *itemHit = 9;
            return 1;
        }
        if (button == 5) {
            *itemHit = 10;
            return 1;
        }
        if (button == 6) {
            *itemHit = 11;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        UpdateStellarDialog();
        return 1;
    }
    return 0;
}

/* UpdateStellarDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00492c40
void UpdateStellarDialog(void)
{
    Rect box;
    short itemType;
    void *item;
    char inside;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(portDialog));
    if (gStellarDialogPict != NULL)
        DrawPicHandle(gStellarDialogPict, GetDialogPortRect(portDialog));
    GetDialogItem(portDialog, 3, &itemType, &item, &box);
    inside = RectInRgn(&box, GetCWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        RGBForeColor(blackColor);
        PaintRect(&box);
        RGBForeColor(whiteColor);
        MoveTo(box.left, (box.top + kNameBaseline));
        TextFont(gTimesFont);
        TextSize(kNameSize);
        DrawStringCentered(dockedPort->name, box.left, box.right,
                           (box.top + kNameBaseline));
        RGBForeColor(blackColor);
    }
    GetDialogItem(portDialog, 5, &itemType, &item, &box);
    inside = RectInRgn(&box, GetCWindowVisRgn(FrontWindow(portDialog)));
    if (inside && gStellarImage.world != NULL) {
        CopyBits(GetGWCPixMap(&gStellarImage),
                 (BitMapPtr)GetGWCPixMap(&loadPort),
                 &gStellarImage.bounds, &box, 0,
                 GetCWindowVisRgn(FrontWindow(portDialog)));
    }
    GetDialogItem(portDialog, 6, &itemType, &item, &box);
    inside = RectInRgn(&box, GetCWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        RGBForeColor(blackColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(gDescText, strlen(gDescText), &box, 0);
        InvertRect(&box);
    }
    SetGWorldContainer(&loadPort);
    DrawPlanetButtons(-8);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(portDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(portDialog)),
             GetDialogPortRect(portDialog), GetDialogPortRect(portDialog),
             0, NULL);
}

/*
 * The modal driver every dialog in the binary runs through. Its first two
 * parameters are dead here — the callers all pass the dialog and its event
 * proc, which were installed on the dialog record by GetNewDialog and are
 * picked up from there — and the fourth is an optional out-slot for the
 * modifier state that GetKeyModifiers reports.
 */
/* RunModalDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00492f10
void RunModalDialog(DialogPtr dlg, void *eventProc, short *itemHit,
                    short *modifiers)
{
    ModalDialog(NULL, itemHit);
    if (modifiers != NULL)
        *modifiers = GetKeyModifiers();
}

#define kShipDescBase 13000     /* desc ID of ship 0's shipyard blurb */
#define kHireDescBase 14000     /* ...and of its hire-an-escort blurb */

/* the item grid: four across, five down, out of item 5's box */
#define kMovieItem    8


#define kShipyardPictID 0x2135

/* per-mission state, stride 0x14; buttons.c's MissionState only names +0x00 */
#define kMissionAwayFromHome 0x400               /* effect-named: excluded from the "outside" count (TBC) */


/* DoShipyardDialog (mac _DoShipyardDialog). STATE: MATCH 100%, exe 981 / obj 981, frame 0x32c,
   byte-identical yes (2026-09-25). `weap' also counts the first scan, the name digits and the
   default outfits; `itemType' and `digitBuf' are declared last (a compound: neither alone). */
// FUNCTION: EVNOVA 0x00492f30
void DoShipyardDialog(void)
{
    void *item;
    char confirmed;
    char msgText[256];
    int hireCost;
    char defaultName[256];
    Rect box;
    short itemHit;

    short i;
    char done;
    short outf;
    long tradeIn;
    long cost;
    short weap;
    char weapHasSlot;
    short halfHeight;
    short n;
    int savedTargetStellar;
    int savedUnk6A;
    short itemType;
    char digitBuf[256];

    SetupPortAvailableShipTypes(dockedPort);
    done = 1;
    for (weap = 0; weap < 768; weap++) {
        if (dockedPortShipTypes[weap] != -1) {
            done = 0;
            break;
        }
    }
    if (done) {
        strcpy(gTextShowString, "");
        if (shipBuyMode == 0)
            AppendIndString(gTextShowString, kGameStrList, 0xdf);
        else
            AppendIndString(gTextShowString, kGameStrList, 0xe0);
        ShowTextDialog(0, 0, 0);
        return;
    }
    SWSetPortToWindow(gSpriteWorld);
    gSelectedShip = -1;
    gShipGridPos = -1;
    gLastClickPos = -1;
    gShipGridScroll = 0;
    canBuy = 0;
    savedTargetStellar = (unsigned short)shipData->targetStellar;
    savedUnk6A = (unsigned short)shipData->navTargetKind;
    previewPic = NULL;
    previewPicID = -1;
    missionListVenue = 5;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    shipyardDialog = NULL;
    shipyardDialog = GetNewDialog(0x3ec, ShipyardDialogUpdate, ShipyardFilter);
    if (shipyardDialog == NULL)
        return;
    gShipyardDialogPict = GetPicture(kShipyardPictID);
    CenterWindow(FrontWindow(shipyardDialog), FrontWindow(portDialog));
    GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
    SetupListRects((Point *)&box);
    strncpy(gItemDescText, "", 3);
    ShowWindowNoFlash(FrontWindow(shipyardDialog));
    SetDialogGWorld(shipyardDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RGBForeColor(blackColor);
    ShipyardDialogUpdate();
    FrontWindow(shipyardDialog);
    FrontWindow(shipyardDialog);
    if (shipBuyMode == 0)
        OfferMissionFromPort(5);
    RedrawDialog(shipyardDialog);
    GetDialogItem(shipyardDialog, kMovieItem, &itemType, &item, &box);

    do {
        if (gQuit)
            done = 1;
        RunModalDialog(shipyardDialog, ShipyardFilter, &itemHit, 0);
        if (itemHit == 1) {
            if (gSelectedShip != -1) {
                n = gSelectedShip;
                if (canBuy != 0) {
                    if (shipBuyMode == 0) {
                        extern char *strcpy(char *, const char *);

                        RedrawDialog(shipyardDialog);
                        strcpy(msgText, "");
                        AppendIndString(msgText, kGameStrList, 0x78);
                        strcat(msgText, " ");
                        strncpy(defaultName, (char *)gShipLongNames[n], 200);
                        PtoCstr((unsigned char *)defaultName);
                        strcat(msgText, defaultName);
                        strcat(msgText, ": ");
                        CtoPstr(msgText);
                        pstrcpy((unsigned char *)defaultName,
                               gShipTypes[n].name);
                        PtoCstr((unsigned char *)defaultName);
                        strcat(defaultName, " ");
                        for (weap = 0; weap <= 2; weap++) {
                            NumToString(Rand(9) + 1, (unsigned char *)digitBuf);
                            PtoCstr((unsigned char *)digitBuf);
                            strcat(defaultName, digitBuf);
                        }
                        CtoPstr(defaultName);
                        confirmed = EVTextInputDialog((unsigned char *)msgText,
                                                      (unsigned char *)defaultName, 0x40);
                        SetDialogGWorld(shipyardDialog);
                        RedrawDialog(shipyardDialog);
                        if (confirmed != 0) {
                            didBuyShip = 1;
                            tradeIn = ApplyPriceAndTechnologyFlux(
                                PlayerShipTradeInPrice(), priceFluxMult[0],
                                gShipTypes[shipData->type].techLevel,
                                stellarData[shipData->targetStellar].techLevel);
                            tradeIn = ApplyPriceAndTechnologyFlux(
                                tradeIn, priceFluxMult[1],
                                gShipTypes[shipData->type].techLevel,
                                stellarData[shipData->targetStellar].techLevel);
                            shipData->credits += tradeIn;
                            crashlog_mission_bit_set(
                                "retiring ship", (char *)gShipTypes[shipData->type].name,
                                1, shipData->type + 128,
                                gShipTypes[shipData->type].onRetire);
                            EvalMissionBitSetString(gShipTypes[shipData->type].onRetire);
                            shipData->type = n;
                            cost = ApplyPriceAndTechnologyFlux(
                                gShipTypes[n].cost, priceFluxMult[1],
                                gShipTypes[n].techLevel,
                                stellarData[shipData->targetStellar].techLevel);
                            shipData->credits -= cost;
                            pstrcpy(shipName, gTextInputResult);
                            CullNameString(shipName);
                            for (weap = 1; weap < 0x40; weap++) {
                                if (shipData[weap].active != 0 &&
                                    shipData[weap].parentShip == 0 &&
                                    shipData[weap].aiType == 5)
                                    shipData[weap].active = 0;
                            }
                            weap = 0;
                            do {
                                weapHasSlot = 0;
                                for (outf = 0; outf < 512; outf++) {
                                    if (gOutfits[outf].modType[0] == 1 &&
                                        gOutfits[outf].modVal[0] == weap &&
                                        gOutfits[outf].persistent != 0) {
                                        weapHasSlot = 1;
                                        break;
                                    }
                                }
                                if (!weapHasSlot) {
                                    shipData->weap[weap].count = 0;
                                    shipData->weap[weap].ammo = 0;
                                }
                            } while (++weap < 256);
                            for (outf = 0; outf < 512; outf++) {
                                if (gOutfits[outf].persistent == 0)
                                    shipSystems[outf] = 0;
                            }
                            for (weap = 0; weap < 8; weap++) {
                                if (gShipTypes[shipData->type].defOutfCount[weap] > 0)
                                    shipSystems[gShipTypes[shipData->type].defOutfID[weap]] +=
                                        gShipTypes[shipData->type].defOutfCount[weap];
                            }
                            SystemInfoToShipStats();
                            for (weap = 0; weap < 256; weap++) {
                                if (gShipTypes[shipData->type].weapCount[weap] >
                                    shipData->weap[weap].count)
                                    shipData->weap[weap].count =
                                        gShipTypes[shipData->type].weapCount[weap];
                                if (gShipTypes[shipData->type].weapAmmo[weap] > 0) {
                                    i = gWeapons[weap].ammoType;
                                    if (i < 0 || i >= 0x100 || gWeapons[weap].guidance == kGuidanceBay)
                                        i = weap;
                                    if (gShipTypes[shipData->type].weapAmmo[weap] >
                                        shipData->weap[i].ammo)
                                        shipData->weap[i].ammo =
                                            gShipTypes[shipData->type].weapAmmo[weap];
                                }
                            }
                            DestroyPartialFleetCargo(0);
                            crashlog_mission_bit_set(
                                "purchasing ship", (char *)gShipTypes[shipData->type].name,
                                1, shipData->type + 128,
                                gShipTypes[shipData->type].onPurchase);
                            EvalMissionBitSetString(gShipTypes[shipData->type].onPurchase);
                            shipData->targetShip = -1;
                            shipData->selectedWeapon = -1;
                            playerPaint.red = playerPaint.green =
                                playerPaint.blue = 0;
                            statusRedraw[3] = 1;
                            statusRedraw[0] = 1;
                            statusRedraw[6] = 1;
                            statusRedraw[2] = 1;
                            shipData->shield = ShipShieldCapacity(shipData);
                            shipData->armor = ShipArmorCapacity(shipData);
                            shipData->fuel = ShipFuelCapacity(shipData);
                            shipData->pos.x = dockedPort->pos.h;
                            shipData->pos.y = dockedPort->pos.v;
                            statusRedraw[2] = 1;
                            LoadIntfResource();
                            DrawStatus();
                            SetDialogGWorld(shipyardDialog);
                            done = 1;
                        }
                        gShipTypes[gSelectedShip].dailySaleRoll = Rand(100) + 1;
                        SetDialogGWorld(shipyardDialog);
                        RedrawDialog(shipyardDialog);
                    } else {
                        hireCost = ApplyPriceAndTechnologyFlux(gShipTypes[gSelectedShip].cost,
                                                   priceFluxMult[1],
                                                   gShipTypes[gSelectedShip].techLevel,
                                                   stellarData[shipData->targetStellar].techLevel);
                        shipData->credits -= hireCost * 0.1;
                        statusRedraw[2] = 1;
                        SpawnEscortOfType(gSelectedShip, shipData->targetStellar);
                        gShipTypes[gSelectedShip].dailyHireRoll = Rand(100) + 1;
                        done = 1;
                    }
                } else {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                }
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
        if (itemHit == 3) {
            savedTargetStellar = (unsigned short)shipData->targetStellar;
            savedUnk6A = (unsigned short)shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedTargetStellar;
            shipData->navTargetKind = savedUnk6A;
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 4) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 8 && shipBuyMode == 0) {
            if (OfferMissionFromPort(5) != 0) {
                SetDialogGWorld(portDialog);
                UpdateAllPortDialogs();
                SetDialogGWorld(shipyardDialog);
                ShipyardDialogUpdate();
            }
        }
        if (itemHit == 0xb) {
            n = 0;
            for (i = 0; i < 16; i++) {
                if (currentMissionObjectives[i].active != 0 &&
                    (currentMissionObjectives[i].flags & kMissionAwayFromHome) == 0)
                    n++;
            }
            if (n > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateStellarDialog();
                SetDialogGWorld(shipyardDialog);
                RedrawDialog(shipyardDialog);
            }
        }
        if (itemHit == 10 && gSelectedShip != -1) {
            ShipyardDialogUpdate();
            DoShipyardDescription(gSelectedShip);
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 0xc && gShipGridScroll >= 4) {
            gShipGridScroll -= 4;
            gSelectedShip = gShipGridPos = -1;
            strncpy(gItemDescText, "", 3);
            EndLoopingMovie();
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 0xd) {
            n = 0;
            for (i = 0; i < 768; i++) {
                if (dockedPortShipTypes[i] != -1)
                    n++;
            }
            if (gShipGridScroll < n - 20) {
                gShipGridScroll += 4;
                gSelectedShip = gShipGridPos = -1;
                strncpy(gItemDescText, "", 3);
                EndLoopingMovie();
                RedrawDialog(shipyardDialog);
            }
        }
        if (itemHit == 7)
            done = 1;
    } while (done == 0);

    if (shipBuyMode != 0) {
        n = -((GetDialogPortRect(shipyardDialog)->right -
                       GetDialogPortRect(shipyardDialog)->left) / 2);
        halfHeight = -((GetDialogPortRect(shipyardDialog)->bottom -
                        GetDialogPortRect(shipyardDialog)->top) / 2);
    }
    EndLoopingMovie();
    DisposeDialogNoFlash(shipyardDialog);
    shipyardDialog = NULL;
    shipyardEgg = 0;
    if (gShipyardDialogPict != NULL) {
        QDReleaseResource(gShipyardDialogPict);
        gShipyardDialogPict = NULL;
    }
    if (previewPic != NULL) {
        QDReleaseResource(previewPic);
        previewPic = NULL;
    }
    if (shipBuyMode != 0) {
        box.right = ((GetCWindowPortRect(gWind)->left +
                             GetCWindowPortRect(gWind)->right) / 2);
        box.left = box.right;
        box.bottom = ((GetCWindowPortRect(gWind)->top +
                              GetCWindowPortRect(gWind)->bottom) / 2);
        box.top = box.bottom;
        InsetRect(&box, n - 1, halfHeight - 1);
        SWSetPortToWindow(gSpriteWorld);
        RGBForeColor(blackColor);
        PaintRect(&box);
        RedrawStatus(1);
        RedrawComm(0);
    }
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    RGBForeColor(blackColor);
    RedrawStatus(0);
    SWSetPortToWindow(gSpriteWorld);
    shipData->targetStellar = savedTargetStellar;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}

/* ShipyardFilter (mac _ShipyardFilter). STATE: MATCH 587/587 (2026-09-24).
   OPEN: none */
// FUNCTION: EVNOVA 0x00493fc0
char ShipyardFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    Rect movieBox;
    short itemType;
    void *item;
    Point where;
    int c;
    short i;
    short count;
    long slot;
    short old;
    short oldPos;
    short col;
    short row;
    short button;
    long cell;

    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 7;
        return 1;
    }
    LoopingMovieIdle();
    *itemHit = -1;
    CalcShipCanBuy();
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        c = ev->message & kCharCodeMask;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnChar || c == kEscapeChar) {
                ev->what = kMouseDown;
                *itemHit = 7;
                return 1;
            }
            c = tolower(c);
            if ((c == 'b' && shipBuyMode == 0) || (c == 'h' && shipBuyMode == 1)) {
                ev->what = kMouseDown;
                *itemHit = 1;
                return 1;
            }
        }
        if (c == kLeftArrowChar || c == kRightArrowChar || c == kUpArrowChar ||
            c == kDownArrowChar || c == kPageUpChar || c == kPageDownChar) {
            old = gSelectedShip;
            if (gShipGridPos != -1) {
                count = 0;
                for (i = 0; i < 512; i++) {
                    if (dockedPortShipTypes[i] != -1)
                        count++;
                }
                if (c == kRightArrowChar) {
                    if (gShipGridPos < kGridCells - 1) {
                        if (dockedPortShipTypes[gShipGridScroll + gShipGridPos
                                                + 1] != -1)
                            gShipGridPos++;
                    } else if (gShipGridScroll < count - kGridCells) {
                        gShipGridScroll += kGridCols;
                        gShipGridPos -= kGridCols - 1;
                    }
                } else if (c == kLeftArrowChar) {
                    if (gShipGridPos > 0) {
                        gShipGridPos--;
                    } else if (gShipGridScroll > 0) {
                        gShipGridScroll -= kGridCols;
                        gShipGridPos += kGridCols - 1;
                    }
                } else if (c == kUpArrowChar || c == kPageUpChar) {
                    if (gShipGridPos < kGridCols) {
                        if (gShipGridScroll > 0)
                            gShipGridScroll -= kGridCols;
                    } else {
                        gShipGridPos -= kGridCols;
                    }
                } else if (c == kDownArrowChar || c == kPageDownChar) {
                    if (gShipGridPos < kGridCells - kGridCols) {
                        if (dockedPortShipTypes[gShipGridScroll + gShipGridPos
                                                + kGridCols] != -1)
                            gShipGridPos += kGridCols;
                    } else if (gShipGridScroll < count - kGridCells) {
                        gShipGridScroll += kGridCols;
                    }
                }
                slot = gShipGridPos;
                slot += gShipGridScroll;
                gSelectedShip = dockedPortShipTypes[slot];
                while (gSelectedShip == -1 && gShipGridPos > 0) {
                    slot--;
                    gShipGridPos--;
                    gSelectedShip = dockedPortShipTypes[slot];
                }
            } else {
                if (c == kRightArrowChar || c == kDownArrowChar ||
                    c == kPageDownChar)
                    gShipGridPos = 0;
                else
                    gShipGridPos = kGridCells - 1;
                cell = gShipGridPos;
                cell += gShipGridScroll;
                gSelectedShip = dockedPortShipTypes[cell];
                while (gSelectedShip == -1 && gShipGridPos > 0) {
                    cell--;
                    gShipGridPos--;
                    gSelectedShip = dockedPortShipTypes[cell];
                }
            }
            if (gSelectedShip != old) {
                if (gSelectedShip == -1) {
                    canBuy = 0;
                } else {
                    if (shipBuyMode != 0)
                        LoadDescription(gItemDescText,
                                        (gSelectedShip + kHireDescBase));
                    else
                        LoadDescription(gItemDescText,
                                        (gSelectedShip + kShipDescBase));
                    RedrawDialog(shipyardDialog);
                    CalcShipCanBuy();
                    EndLoopingMovie();
                    if (gSelectedShip != -1 &&
                        gShipMovieFiles[gSelectedShip][0] != 0) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(gShipMovieFiles[gSelectedShip],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                }
            }
        }
    }
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 3;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 4;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 11;
            return 1;
        }
        c = tolower(ev->message & kCharCodeMask);
        return 0;
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            col = (where.h - box.left) / kGridCellWide;
            row = (where.v - box.top) / kGridCellHigh;
            if (col >= 0 && col <= kGridCols - 1 && row >= 0 &&
                row <= kGridCells / kGridCols - 1) {
                oldPos = gShipGridPos;
                gShipGridPos = row * kGridCols + col;
                if (dockedPortShipTypes[gShipGridPos + gShipGridScroll] != -1) {
                    gSelectedShip =
                        dockedPortShipTypes[gShipGridPos + gShipGridScroll];
                } else {
                    gSelectedShip = gShipGridPos = -1;
                }
                if (gShipGridPos != oldPos) {
                    InvalDialogItem(shipyardDialog, 1);
                    InvalDialogItem(shipyardDialog, 5);
                    InvalDialogItem(shipyardDialog, 6);
                    InvalDialogItem(shipyardDialog, 8);
                    InvalDialogItem(shipyardDialog, 9);
                    InvalDialogItem(shipyardDialog, 10);
                    if (gShipGridPos == -1) {
                        strncpy(gItemDescText, "", 3);
                    } else if (shipBuyMode != 0) {
                        LoadDescription(gItemDescText,
                                        (gSelectedShip + kHireDescBase));
                    } else {
                        LoadDescription(gItemDescText,
                                        (gSelectedShip + kShipDescBase));
                    }
                    CalcShipCanBuy();
                    EndLoopingMovie();
                    if (gSelectedShip != -1 &&
                        gShipMovieFiles[gSelectedShip][0] != 0) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(gShipMovieFiles[gSelectedShip],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                }
                if (gShipGridPos == gLastClickPos && gShipGridPos != -1 &&
                    ev->when - gLastClickTime <= 15) {
                    ShipyardDialogUpdate();
                    DoShipyardDescription(gSelectedShip);
                    SetDialogGWorld(shipyardDialog);
                    RedrawDialog(shipyardDialog);
                }
                gLastClickPos = gShipGridPos;
                gLastClickTime = ev->when;
            }
        }
        ShipyardDialogUpdate();
        button = TrackShipyardButtons(where);
        if (button == 0) {
            *itemHit = 7;
            return 1;
        }
        if (button == 1) {
            *itemHit = 1;
            return 1;
        }
        if (button == 2) {
            *itemHit = 10;
            return 1;
        }
        if (button == 3) {
            *itemHit = 12;
            return 1;
        }
        if (button == 4) {
            *itemHit = 13;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (shipBuyMode == 0) {
        if (ReadTick() >= missionOfferTime) {
            *itemHit = 8;
            missionOfferTime = ReadTick() + Rand(30) + 30;
            return 1;
        }
    }
    if ((int)ev->what == kUpdateEvt) {
        ShipyardDialogUpdate();
        return 0;
    }
    return 0;
}

/* ShipyardDialogUpdate (mac _ShipyardDialogUpdate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004948b0
void ShipyardDialogUpdate(void)
{
    int c;
    RGBColor *cellColor;
    short drawn;
    short skipped;
    void *item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short i;
    long price;
    long tradeIn;
    long net;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(shipyardDialog));
    if (gShipyardDialogPict != NULL)
        DrawPicHandle(gShipyardDialogPict, GetDialogPortRect(shipyardDialog));
    skipped = 0;
    GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
    box.right += 1;
    box.bottom += 1;
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipyardDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(blackColor);
        drawn = 0;
        i = gShipGridScroll;
        if (i < 768) {
            cellColor = &shopGridColor[1];
            do {
                if (RectInRgn(&gCellRects[i - gShipGridScroll],
                              GetCWindowVisRgn(FrontWindow(shipyardDialog))) &&
                    drawn < kGridCells && i >= 0 &&
                    dockedPortShipTypes[i] != -1) {
                    if (skipped < 3) {
                        if (i - gShipGridScroll >= 0 &&
                            i - gShipGridScroll < kGridCells) {
                            if (!DrawCachedPict(
                                    gShipTypes[dockedPortShipTypes[i]].shipyardPictID,
                                    &loadPort, &gIconRects[i - gShipGridScroll]))
                                skipped++;
                        } else {
                            skipped++;
                        }
                    } else {
                        InvalDialogRect(shipyardDialog, &box);
                        break;
                    }
                    if (cheatsActive) {
                        NumToString(dockedPortShipTypes[i] + kFirstResourceID,
                                    line1);
                        RGBForeColor(whiteColor);
                        MoveTo(((gCellRects[i - gShipGridScroll].left
                                        + gCellRects[i - gShipGridScroll].right
                                        - StringWidth(line1)) / 2),
                               (gCellRects[i - gShipGridScroll].top + 24));
                        DrawString(line1);
                        RGBForeColor(blackColor);
                    }
                    if (SplitTwoLineString(gShipShortNames[dockedPortShipTypes[i]],
                                           line1, line2)) {
                        c = line1[1];
                        if (isalpha(c) || isdigit(c))
                            RGBForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        MoveTo(((gCellRects[i - gShipGridScroll].left
                                        + gCellRects[i - gShipGridScroll].right) / 2
                                       - StringWidth(line1) / 2),
                               (gCellRects[i - gShipGridScroll].bottom - 14));
                        DrawString(line1);
                        c = line2[1];
                        if (isalpha(c) || isdigit(c))
                            RGBForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        MoveTo(((gCellRects[i - gShipGridScroll].left
                                        + gCellRects[i - gShipGridScroll].right) / 2
                                       - StringWidth(line2) / 2),
                               (gCellRects[i - gShipGridScroll].bottom - 3));
                        DrawString(line2);
                    } else {
                        MoveTo(((gCellRects[i - (long)gShipGridScroll].left
                                        + gCellRects[i - (long)gShipGridScroll].right) / 2
                                       - StringWidth(line1) / 2),
                               (gCellRects[i - (long)gShipGridScroll].bottom - 6));
                        RGBForeColor(whiteColor);
                        DrawString(line1);
                    }
                    RGBForeColor(blackColor);
                    RGBForeColor(cellColor);
                    FrameRect(&gCellRects[i - gShipGridScroll]);
                    RGBForeColor(blackColor);
                    drawn++;
                }
                i++;
            } while (i < 768);
        }
        if (gShipGridPos != -1) {
            RGBForeColor(&shopGridColor[0]);
            FrameRect(&gCellRects[gShipGridPos]);
            RGBForeColor(blackColor);
        }
    }
    if (gSelectedShip != -1) {
        GetDialogItem(shipyardDialog, kMovieItem, &itemType, &item, &box);
        if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipyardDialog)))) {
            if (loopingMovie != NULL) {
                CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                             GetDialogPort(shipyardDialog)),
                         (BitMapPtr)GetGWCPixMap(&loadPort), &box, &box, 0,
                         GetCWindowVisRgn(FrontWindow(shipyardDialog)));
            } else {
                if (previewPicID != gShipTypes[gSelectedShip].shipyardPictID) {
                    if (previewPic != NULL) {
                        QDReleaseResource(previewPic);
                        previewPic = NULL;
                    }
                    previewPicID = gShipTypes[gSelectedShip].shipyardPictID;
                    if (previewPicID >= kFirstResourceID)
                        previewPic = GetPicture(previewPicID);
                }
                if (previewPic != NULL) {
                    RGBForeColor(blackColor);
                    DrawPicHandle(previewPic, &box);
                } else {
                    RGBForeColor(whiteColor);
                    TextFont(smallTextFont);
                    TextSize(smallTextSize);
                    GetIndString(line1, kGameStrList, 0xd5);
                    DrawStringCentered(line1, box.left, box.right,
                                       ((box.bottom + box.top) / 2 - 6));
                    GetIndString(line1, kGameStrList, 0xd6);
                    DrawStringCentered(line1, box.left, box.right,
                                       ((box.bottom + box.top) / 2 + 6));
                    RGBForeColor(blackColor);
                }
            }
        }
        GetDialogItem(shipyardDialog, 9, &itemType, &item, &box);
        if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipyardDialog)))) {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            if (shipBuyMode == 0 && gSelectedShip != shipData->type) {
                RGBForeColor(whiteColor);
                MoveTo(box.left, (box.top + 12));
                DrawIndString(kGameStrList, 0xe1);
                MoveTo((box.left + 0x46), (box.top + 12));
                price = ApplyPriceAndTechnologyFlux(
                    gShipTypes[gSelectedShip].cost, priceFluxMult[1],
                    gShipTypes[gSelectedShip].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(price);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x18));
                DrawIndString(kGameStrList, 0xe2);
                MoveTo((box.left + 0x46), (box.top + 0x18));
                net = ApplyPriceAndTechnologyFlux(
                    PlayerShipTradeInPrice(), priceFluxMult[0],
                    gShipTypes[shipData->type].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                    net, priceFluxMult[1],
                    gShipTypes[shipData->type].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(tradeIn);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                net = price - tradeIn;
                if (net < 0)
                    net = 0;
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xe3);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                DrawCommaNumber(net);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x48));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x48));
                DrawCommaNumber(shipData->credits);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                RGBForeColor(blackColor);
            } else if (shipBuyMode != 0) {
                RGBForeColor(whiteColor);
                MoveTo(box.left, (box.top + 12));
                DrawIndString(kGameStrList, 0xe4);
                MoveTo((box.left + 0x46), (box.top + 12));
                DrawCommaNumber((ApplyPriceAndTechnologyFlux(
                    gShipTypes[gSelectedShip].cost, priceFluxMult[1],
                    gShipTypes[gSelectedShip].techLevel,
                    stellarData[shipData->targetStellar].techLevel) * 0.1));
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x24));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x24));
                DrawCommaNumber(shipData->credits);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                RGBForeColor(blackColor);
            }
        }
    }
    GetDialogItem(shipyardDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipyardDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(gItemDescText, strlen(gItemDescText), &box, 0);
        InvertRect(&box);
    }
    DrawShipyardButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(shipyardDialog);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(shipyardDialog)),
             GetDialogPortRect(shipyardDialog),
             GetDialogPortRect(shipyardDialog), 0,
             GetCWindowVisRgn(FrontWindow(shipyardDialog)));
}


#define kLabelStrList 150
#define kLabelStrIdx  5



#define kDescDlogID      0x3fb   /* 1019: the layout with a picture well */
#define kPlainDescDlogID 0x3ed   /* 1005: the layout without one */
#define kDescPictID      8507    /* backdrop to match each layout */
#define kPlainDescPictID 8506

/* DoShipyardDescription (mac _DoShipyardDescription). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004956a0
void DoShipyardDescription(short ship)
{
    char done;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short i;
    short shown;

    done = 0;
    shipDescDialog = NULL;
    gShipDescPict = NULL;
    if (descResourcePictId >= kFirstResourceID)
        gShipDescPict = GetPicture(descResourcePictId);
    if (gShipDescPict != NULL)
        shipDescDialog = GetNewDialog(kDescDlogID, ShipDescDialogUpdate, Dlg1005EventProc);
    else
        shipDescDialog = GetNewDialog(kPlainDescDlogID, ShipDescDialogUpdate,
                                Dlg1005EventProc);
    if (shipDescDialog == NULL)
        return;
    if (descResourcePictId >= kFirstResourceID)
        gShipDescBackdrop = GetPicture(kDescPictID);
    else
        gShipDescBackdrop = GetPicture(kPlainDescPictID);
    CenterWindow(FrontWindow(shipDescDialog), FrontWindow(portDialog));
    ShowWindowNoFlash(FrontWindow(shipDescDialog));
    SetDialogGWorld(shipDescDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RGBForeColor(blackColor);
    RedrawDialog(shipDescDialog);
    ShipDescDialogUpdate();
    FrontWindow(shipDescDialog);
    FrontWindow(shipDescDialog);
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(shipDescDialog, Dlg1005EventProc, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 4) {
            savedStellar = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            SetDialogGWorld(portDialog);
            RedrawDialog(shipDescDialog);
            UpdateStellarDialog();
            SetDialogGWorld(shipDescDialog);
            RedrawDialog(shipDescDialog);
        }
        if (itemHit == 2) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateStellarDialog();
            SetDialogGWorld(shipDescDialog);
            RedrawDialog(shipDescDialog);
        }
        if (itemHit == 6) {
            shown = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0
                    && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
                    shown++;
            }
            if (shown > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateStellarDialog();
                SetDialogGWorld(shipDescDialog);
                RedrawDialog(shipDescDialog);
            }
        }
    } while (done == 0);
    DisposeDialogNoFlash(shipDescDialog);
    shipDescDialog = NULL;
    if (gShipDescBackdrop != NULL) {
        QDReleaseResource(gShipDescBackdrop);
        gShipDescBackdrop = NULL;
    }
    if (gShipDescPict != NULL)
        QDReleaseResource(gShipDescPict);
    gShipDescPict = NULL;
}

/* Dlg1005EventProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004959d0
char Dlg1005EventProc(long unused, EventRecord *ev, short *itemHit)
{
    short itemType;
    void *item;
    unsigned char label[256];
    Rect box;
    Point where;
    short lastHit;
    short hit;
    int c;

    if ((int)ev->what == kKeyDown) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 4;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 2;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 6;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        c = tolower(c);
        if (c == kReturnChar || c == kEscapeChar) {
            ev->what = kMouseDown;
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what == kUpdateEvt) {
        DrawDialog(shipDescDialog);
        ShipDescDialogUpdate();
        return 0;
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(shipDescDialog, 1, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            GetIndString(label, kLabelStrList, kLabelStrIdx);
            DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 1, 0);
            nv_UpdateDialogWindow(0);
            lastHit = 1;
            while (StillDown()) {
                BusyIdle();
                GetMouse(&where);
                if (PtInRect(where.h, where.v, &box))
                    hit = 1;
                else
                    hit = -1;
                if (hit != lastHit) {
                    if (hit == 1)
                        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 1, 0);
                    else
                        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 0, 0);
                    nv_UpdateDialogWindow(0);
                }
                lastHit = hit;
            }
            if (lastHit == 1) {
                *itemHit = 1;
                return 1;
            }
            *itemHit = -1;
            return 1;
        }
    }
    return 0;
}


/* ShipDescDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00495c80
void ShipDescDialogUpdate(void)
{
    long drawn;
    long numWeapTypes;
    short itemType;
    short sel;
    long lineV;
    Rect box2;
    char hasPict;
    char line[1024];
    unsigned char buf[256];
    Rect box;
    void *item;
    short i;
    short j;
    short jumps;
    long freeMass;
    long mass;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(shipDescDialog));
    if (gShipDescBackdrop != NULL)
        DrawPicHandle(gShipDescBackdrop, GetDialogPortRect(shipDescDialog));
    if (descResourcePictId >= 128)
        hasPict = 1;
    else
        hasPict = 0;
    RGBForeColor(blackColor);
    GetDialogItem(shipDescDialog, 3, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipDescDialog)))) {
        TextFont(gTimesFont);
        TextSize(18);
        MoveTo(box.left, (box.top + 18));
        RGBForeColor(whiteColor);
        if (hasPict)
            DrawStringCentered(gShipLongNames[gSelectedShip], box.left,
                               box.right, (box.top + 18));
        else
            DrawStringCentered(gShipTypes[gSelectedShip].name, box.left,
                               box.right, (box.top + 18));
        RGBForeColor(blackColor);
    }
    GetDialogItem(shipDescDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipDescDialog)))) {
        sel = gSelectedShip;
        RGBForeColor(blackColor);
        PaintRect(&box);
        RGBForeColor(whiteColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 12));
        DrawIndString(kGameStrList, 229);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 12));
        NumToString((gShipTypes[sel].maxSpeed * 100.0), buf);
        DrawString(buf);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 24));
        DrawIndString(kGameStrList, 230);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 24));
        if (gShipTypes[sel].accel > 0.073)
            DrawIndString(kGameStrList, 245);
        else if (gShipTypes[sel].accel > 0.055)
            DrawIndString(kGameStrList, 246);
        else if (gShipTypes[sel].accel > 0.038)
            DrawIndString(kGameStrList, 247);
        else if (gShipTypes[sel].accel > 0.025)
            DrawIndString(kGameStrList, 248);
        else if (gShipTypes[sel].accel > 0.013)
            DrawIndString(kGameStrList, 249);
        else if (gShipTypes[sel].accel > 0.0)
            DrawIndString(kGameStrList, 250);
        else
            DrawIndString(kGameStrList, 396);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 36));
        DrawIndString(kGameStrList, 231);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 36));
        if (gShipTypes[sel].turnRate <= 0.0f)
            DrawIndString(kGameStrList, 396);
        else if (gShipTypes[sel].turnRate <= 1.0f)
            DrawIndString(kGameStrList, 250);
        else if (gShipTypes[sel].turnRate <= 2.0f)
            DrawIndString(kGameStrList, 249);
        else if (gShipTypes[sel].turnRate <= 3.0f)
            DrawIndString(kGameStrList, 248);
        else if (gShipTypes[sel].turnRate <= 4.0f)
            DrawIndString(kGameStrList, 247);
        else if (gShipTypes[sel].turnRate <= 5.0f)
            DrawIndString(kGameStrList, 246);
        else
            DrawIndString(kGameStrList, 245);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 48));
        DrawString(shieldString[0]);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 48));
        if (gShipTypes[sel].shield > 0)
            DrawCommaNumber(gShipTypes[sel].shield);
        else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 60));
        DrawString(armorString[0]);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 60));
        if (gShipTypes[sel].armor > 0)
            DrawCommaNumber(gShipTypes[sel].armor);
        else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 72));
        DrawIndString(kGameStrList, 232);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 72));
        if (gShipTypes[sel].maxGuns > 0) {
            NumToString(gShipTypes[sel].maxGuns, buf);
            DrawIndString(kGameStrList, 238);
            DrawString("\p ");
            DrawString(buf);
        } else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 84));
        DrawIndString(kGameStrList, 233);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 84));
        if (gShipTypes[sel].maxTurrets > 0) {
            NumToString(gShipTypes[sel].maxTurrets, buf);
            DrawIndString(kGameStrList, 238);
            DrawString("\p ");
            DrawString(buf);
        } else
            DrawIndString(kGameStrList, 335);

        freeMass = gShipTypes[sel].freeMass;
        for (i = 0; i < 256; i++) {
            if (gShipTypes[sel].weapCount[i] > 0) {
                for (j = 0; j < 512; j++) {
                    if (gOutfits[j].modType[0] == 1
                        && i == gOutfits[j].modVal[0]
                        && gOutfits[j].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&gOutfits[j], &gShipTypes[sel])
                               * gShipTypes[sel].weapCount[i];
                        if (mass <= freeMass)
                            freeMass -= mass;
                        else
                            freeMass = 0;
                        break;
                    }
                }
            }
            if (gShipTypes[sel].weapAmmo[i] > 0) {
                for (j = 0; j < 512; j++) {
                    if (gOutfits[j].modType[0] == 3
                        && i == gOutfits[j].modVal[0]
                        && gOutfits[j].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&gOutfits[j], &gShipTypes[sel])
                               * gShipTypes[sel].weapAmmo[i];
                        if (mass <= freeMass)
                            freeMass -= mass;
                        else
                            freeMass = 0;
                        break;
                    }
                }
            }
        }
        for (i = 0; i < 8; i++) {
            if (gShipTypes[sel].defOutfID[i] != -1
                && gShipTypes[sel].defOutfCount[i] > 0
                && gOutfits[gShipTypes[sel].defOutfID[i]].techLevel < kOutfTechAbsent) {
                mass = AdjustedItemMass(&gOutfits[gShipTypes[sel].defOutfID[i]],
                                        &gShipTypes[sel])
                       * gShipTypes[sel].defOutfCount[i];
                if (mass <= freeMass)
                    freeMass -= mass;
                else
                    freeMass = 0;
            }
        }

        MoveTo((box.left + 130), (box.top + 12));
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 234);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 12));
        NumToString(freeMass, buf);
        DrawString(buf);
        DrawString("\p ");
        if (freeMass == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 24));
        DrawIndString(kGameStrList, 110);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 24));
        NumToString(gShipTypes[sel].cargo, buf);
        DrawString(buf);
        DrawString("\p ");
        if (gShipTypes[sel].cargo == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 36));
        DrawString(fuelString[4]);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 36));
        if (gShipTypes[sel].fuel != 0) {
            jumps = gShipTypes[sel].fuel / 100;
            NumToString(jumps, buf);
            DrawString(buf);
            DrawString("\p ");
            if (jumps == 1)
                DrawIndString(kGameStrList, 239);
            else
                DrawIndString(kGameStrList, 240);
        } else
            DrawIndString(kGameStrList, 335);

        numWeapTypes = 0;
        for (i = 0; i < 256; i++) {
            if (gShipTypes[sel].weapCount[i] > 0)
                numWeapTypes++;
        }
        if ((short)numWeapTypes == 0) {
            RGBForeColor(whiteColor);
            if (hasPict) {
                GetDialogItem(shipDescDialog, 8, &itemType, &item, &box2);
                MoveTo(box2.left, (box2.top + 12));
            } else {
                MoveTo(box.left, (box.top + 108));
            }
            DrawIndString(kGameStrList, 242);
        } else {
            RGBForeColor(whiteColor);
            drawn = 0;
            if (!hasPict) {
                MoveTo(box.left, (box.top + 108));
            } else {
                strcpy(line, "");
                GetDialogItem(shipDescDialog, 8, &itemType, &item, &box2);
                MoveTo(box2.left, (box2.top + 12));
                box2.top += 16;
            }
            DrawIndString(kGameStrList, 241);
            lineV = 0;
            for (i = 0; i < 256; i++) {
                if (gShipTypes[sel].weapCount[i] > 0) {
                    if (!hasPict)
                        MoveTo((box.left + 8),
                               (box.top + 120 + lineV));
                    if (gWeapons[i].guidance != kGuidanceBay) {
                        NumToString(gShipTypes[sel].weapCount[i], buf);
                        if (hasPict) {
                            PtoCstr(buf);
                            strcat(line, (char *)buf);
                            strcat(line, " ");
                        } else {
                            DrawString(buf);
                            DrawString("\p ");
                        }
                        for (j = 0; j < 512; j++) {
                            if (gOutfits[j].modType[0] == 1
                                && i == gOutfits[j].modVal[0]) {
                                if (hasPict) {
                                    pstrcpy(buf, gOutfits[j].name);
                                    PtoCstr(buf);
                                    strcat(line, (char *)buf);
                                } else {
                                    DrawString(gOutfits[j].name);
                                }
                                break;
                            }
                        }
                        if (gShipTypes[sel].weapCount[i] > 1) {
                            if (hasPict)
                                strcat(line, "s");
                            else
                                DrawString("\ps");
                        }
                        if (gShipTypes[sel].weapAmmo[i] > 0) {
                            NumToString(gShipTypes[sel].weapAmmo[i], buf);
                            if (hasPict) {
                                strcat(line, " + ");
                                PtoCstr(buf);
                                strcat(line, (char *)buf);
                                strcat(line, " ");
                                AppendIndString(line, kGameStrList, 243);
                            } else {
                                DrawString("\p + ");
                                DrawString(buf);
                                DrawString("\p ");
                                DrawIndString(kGameStrList, 243);
                            }
                        }
                    } else {
                        NumToString(gShipTypes[sel].weapAmmo[i], buf);
                        if (hasPict) {
                            PtoCstr(buf);
                            strcat(line, (char *)buf);
                            strcat(line, " ");
                        } else {
                            DrawString(buf);
                            DrawString("\p ");
                        }
                        for (j = 0; j < 512; j++) {
                            if (gOutfits[j].modType[0] == 3
                                && i == gOutfits[j].modVal[0]) {
                                if (hasPict) {
                                    pstrcpy(buf, gOutfits[j].name);
                                    PtoCstr(buf);
                                    strcat(line, (char *)buf);
                                } else {
                                    DrawString(gOutfits[j].name);
                                }
                                break;
                            }
                        }
                        if (gShipTypes[sel].weapAmmo[i] > 1) {
                            if (hasPict)
                                strcat(line, "s");
                            else
                                DrawString("\ps");
                        }
                    }
                    if (hasPict && (short)numWeapTypes > 1
                        && (short)drawn < (short)numWeapTypes - 1)
                        strcat(line, ", ");
                    lineV += 12;
                    drawn++;
                }
            }
            if (hasPict) {
                RGBForeColor(blackColor);
                TETextBox(line, strlen(line), &box2, 0);
                InvertRect(&box2);
            }
        }

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 48));
        DrawIndString(kGameStrList, 235);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 48));
        NumToString(gShipTypes[sel].shipLength, buf);
        DrawString(buf);
        DrawString("\p ");
        DrawIndString(kGameStrList, 244);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 60));
        DrawIndString(kGameStrList, 236);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 60));
        NumToString(gShipTypes[sel].mass, buf);
        DrawString(buf);
        DrawString("\p ");
        if (gShipTypes[sel].mass == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        if (gShipTypes[sel].crew > 0) {
            RGBForeColor(&rgbMedGrey);
            MoveTo((box.left + 130), (box.top + 72));
            DrawIndString(kGameStrList, 237);
            RGBForeColor(whiteColor);
            MoveTo((box.left + 175), (box.top + 72));
            NumToString(gShipTypes[sel].crew, buf);
            DrawString(buf);
        }
    }
    GetDialogItem(shipDescDialog, 1, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipDescDialog)))) {
        GetIndString(buf, 150, 5);
        DrawButton(GetDialogPort(shipDescDialog), &box, buf, 0, 0, 0);
    }
    if (gShipDescPict != NULL) {
        GetDialogItem(shipDescDialog, 7, &itemType, &item, &box);
        if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(shipDescDialog))))
            DrawPicHandle(gShipDescPict, &box);
    }
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(shipDescDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(shipDescDialog)),
             GetDialogPortRect(shipDescDialog), GetDialogPortRect(shipDescDialog), 0, 0);
}

/* UpdateAllPortDialogs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497430
void UpdateAllPortDialogs(void)
{
    if (portDialog != NULL) {
        SetDialogGWorld(portDialog);
        UpdateStellarDialog();
    }
    if (shipyardDialog != NULL) {
        SetDialogGWorld(shipyardDialog);
        ShipyardDialogUpdate();
    }
    if (outfitDialog != NULL) {
        SetDialogGWorld(outfitDialog);
        OutfitDialogUpdate();
    }
    if (missionDialog != NULL) {
        SetDialogGWorld(missionDialog);
        if (missionListVenue != 0)
            UpdateMissionDialog();
        else if (portDialog != NULL)
            MissionDialogUpdate();
    }
    if (portDialog != NULL || shipyardDialog != NULL || outfitDialog != NULL
            || missionDialog != NULL)
        nv_UpdateDialogWindow(1);
}

/* IterateInGameEventLoop (mac _IterateInGameEventLoop). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497500
void IterateInGameEventLoop(short tickCount)
{
    char exitLoop;
    char depthMismatch;
    short i;
    Str255 str;

    exitLoop = 0;
    i = 0;
    do {
        HandleIntroMusic(0);
        ResetLastTick();
        WaitTicks((gameLoaded && inBackground) ? 30 : 0);
        i++;
        if (gNeedUpdateFlag) {
            depthMismatch = 0;
            if (((VideoColorDepth + 7) & ~7) != gSpriteWorld->pixelDepth)
                depthMismatch = 1;
            if (!depthMismatch) {
                SWSetPortToWindow(gSpriteWorld);
                if (!creatingPilotFile) {
                    if (gameInProgress && gameLoaded) {
                        if (onPlanet || inBackground) {
                            RedrawStatus(1);
                            RefreshSAT();
                            RedrawComm(1);
                        }
                    } else {
                        CopyBits(GetGWCPixMap(&loadPort),
                                 GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                                 GetCWindowPortRect(gWind), GetCWindowPortRect(gWind),
                                 0, 0);
                    }
                }
                SWSetPortToWindow(gSpriteWorld);
                gNeedUpdateFlag = 0;
            }
        } else if (!closeRequested) {
            exitLoop = 1;
        } else {
            closeRequested = 0;
            if (!screenFaded) {
                ShowTheCursor();
                GetIndString(str, 140, 10);
                if (EVYesNoDialog(str))
                    gQuit = 1;
            }
        }
        if (tickCount > 0 && i >= tickCount)
            exitLoop = 1;
    } while (!exitLoop);
}

/* the port's body takes no arguments and answers the single window; every
   caller still passes the dialog, so it is declared that way here */

/* EVNoteAlert (mac _EVNoteAlert). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004976b0
void EVNoteAlert(const unsigned char *text)
{
    char done;
    Rect box;
    short itemHit;
    short itemType;
    void *item;

    done = 0;
    alertDialog = NULL;
    alertDialog = GetNewDialog(3000, NULL, NULL);
    if (alertDialog == NULL)
        return;
    SetDialogGWorld(alertDialog);
    SetDialogDefaultItem(alertDialog, 1);
    ShowMacWindow(FrontWindow(alertDialog));
    SetDialogGWorld(alertDialog);
    FrontWindow(alertDialog);
    SetDialogGWorld(alertDialog);
    DrawDialog(alertDialog);
    FrameDefaultButton(alertDialog, 1);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((ControlPtr)item,text);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == 1)
            done = 1;
    } while (done == 0);
    DisposeDialog(alertDialog);
    alertDialog = NULL;
}

/* EVYesNoDialog (mac _EVYesNoDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004977d0
char EVYesNoDialog(unsigned char *text)
{
    char done;
    char result;
    Rect box;
    short itemHit;
    short itemType;
    void *item;

    done = 0;
    alertDialog = NULL;
    alertDialog = GetNewDialog(3002, NULL, NULL);
    if (alertDialog == NULL)
        return 0;
    SetDialogDefaultItem(alertDialog, 1);
    ShowMacWindow(FrontWindow(alertDialog));
    FrontWindow(alertDialog);
    SetDialogGWorld(alertDialog);
    FrameDefaultButton(alertDialog, 1);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((ControlPtr)item,text);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == 1) {
            result = 1;
            done = 1;
        }
        if (itemHit == 5) {
            result = 0;
            done = 1;
        }
    } while (done == 0);
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(alertDialog);
    alertDialog = NULL;
    SWSetPortToWindow(gSpriteWorld);
    return result;
}

/* EVTextInputDialog (mac _EVTextInputDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497900
char EVTextInputDialog(unsigned char *prompt, unsigned char *initial, short maxLen)
{
    char done;
    Str255 text;
    short itemHit;
    Rect box;
    char result;
    void *item;
    short itemType;

    done = 0;
    alertDialog = NULL;
    alertDialog = GetNewDialog(3001, NULL, NULL);
    if (alertDialog == NULL)
        return 0;
    SetDialogDefaultItem(alertDialog, 1);
    ShowMacWindow(FrontWindow(alertDialog));
    FrontWindow(alertDialog);
    SetDialogGWorld(alertDialog);
    FrameDefaultButton(alertDialog, 1);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((ControlPtr)item,prompt);
    GetDialogItem(alertDialog, 5, &itemType, &item, &box);
    SetDialogItemText((ControlPtr)item,initial);
    SelectDialogItemText(alertDialog, 5, 0, 254);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == 1) {
            GetDialogItem(alertDialog, 5, &itemType, &item, &box);
            GetDialogItemText((ControlPtr)item,text);
            if (text[0] <= maxLen) {
                result = 1;
                done = 1;
            } else {
                SysBeep(0);
                SelectDialogItemText(alertDialog, 5, 0, maxLen - 1);
            }
        }
        if (itemHit == 6) {
            result = 0;
            done = 1;
        }
    } while (done == 0);
    GetDialogItem(alertDialog, 5, &itemType, &item, &box);
    GetDialogItemText((ControlPtr)item,gTextInputResult);
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(alertDialog);
    alertDialog = NULL;
    SWSetPortToWindow(gSpriteWorld);
    return result;
}

#define kPictCacheSlots 128


/* DrawCachedPict. STATE: MATCH 100.0%, exe 122 / obj 122, byte-identical yes (376 bytes),
   relocregcheck 0 of 29. OPEN: none. The miss-path rect address is held in `j', a later web
   of the scan counter (a split web is numbered after the CSEs; a Rect * local is band 1
   and takes ESI before GetPicture's argument), with `oldest' declared before `j'. `j' is a
   long: a 16-bit-int Mac compiler carried an address in a long, never an int. */
// FUNCTION: EVNOVA 0x00497b70
char DrawCachedPict(short picID, GWorldContainer *dest, Rect *destRect)
{
    int slot;
    int i;
    PicHandle pic;
    CGrafPtr savedPort;
    char missed;
    unsigned long oldest;
    long j;
    short id;

    slot = -1;
    i = 0;
    do {
        id = pictCache[(short)i].picID;
        if (id == -1)
            slot = i;
        if (picID == id) {
            CopyBits(GetGWCPixMap(&pictCachePort), (BitMapPtr)GetGWCPixMap(dest),
                     &pictCacheRect[(short)i], destRect, 0, 0);
            pictCache[(short)i].stamp = ReadTick();
            return 1;
        }
        i = i + 1;
    } while ((short)i < kPictCacheSlots);
    if ((short)slot == -1) {
        oldest = pictCache[0].stamp;
        j = 0;
        do {
            if (pictCache[(short)j].stamp < oldest) {
                oldest = pictCache[(short)j].stamp;
                slot = j;
            }
            j = j + 1;
        } while ((short)j < kPictCacheSlots);
    }
    if ((short)slot < 0 || kPictCacheSlots <= (short)slot)
        slot = 0;
    GetPort(&savedPort);
    SetPort(pictCachePort.world);
    RGBForeColor(blackColor);
    pic = GetPicture(picID);
    if (pic != 0) {
        j = (long)&pictCacheRect[(short)slot];
        DrawPicHandle(pic, (Rect *)j);
        QDReleaseResource(pic);
        pictCache[(short)slot].picID = picID;
        pictCache[(short)slot].stamp = ReadTick();
        missed = 0;
    } else {
        j = (long)&pictCacheRect[(short)slot];
        PaintRect((Rect *)j);
        missed = 1;
    }
    SetPort(savedPort);
    CopyBits(GetGWCPixMap(&pictCachePort), (BitMapPtr)GetGWCPixMap(dest),
             (Rect *)j, destRect, 0, 0);
    return missed;
}

/* ResumePlay. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497cf0
void ResumePlay(short unused)
{
    inBackground = 0;
    lastFPSUpdateTime = 0;
    fpsCounter = 0;
    gTimeScale = 1.0;
    OSCursorReset();
    cursorVisible = 0;
    ShowTheCursor();
    if (introMusicMovie != NULL)
        StartMovie(introMusicMovie);
}

/* SwitchedOut. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497d50
void SwitchedOut(short unused)
{
    void *snd;

    inBackground = 1;
    if (introMusicMovie != NULL)
        StopMovie(introMusicMovie);
    if (warpUpSnd[0] != NULL && ST_IsSoundPlaying(warpUpSnd[0]) != 0)
        ST_HaltSound(warpUpSnd[0]);
    snd = warpUpSnd[1];
    if (snd != NULL && ST_IsSoundPlaying(snd) != 0)
        ST_HaltSound(warpUpSnd[1]);
    OSCursorReset();
    cursorVisible = 0;
    ShowTheCursor();
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
    BackColor(whiteColor);
}

/*
 * SpriteWorld's +0x1c is windowFrameP, so the destination rect is the same
 * frameRect member reached through the other frame.
 */

/* RefreshWindowBuffer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497df0
void RefreshWindowBuffer(void)
{
    SWSetPortToWorkArea(gSpriteWorld);
    if (gameLoaded && gameInProgress)
        SetSystemBackgroundColor();
    else
        RGBForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                 gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);
    ScreenEndScene();
}

/*
 * mwcc 418 marks a name intrinsic-eligible on the declaration it gets from the
 * SYSTEM header, so any redeclaration that does not merge with it clears the
 * marking. Block scope is the only spelling that works.
 */



#define kCaptureStrIdx  0x76

char CaptureDialogEventProc(long unused, EventRecord *ev, short *itemHit);

#define kCaptureDlogID   0x3fa
#define kCapturePictID   0x2144

/* DoCapturedShipAssignmentDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497eb0
char DoCapturedShipAssignmentDialog(void)
{
    char done;
    char result;
    short itemHit;

    done = 0;
    captureDialog = NULL;
    captureDialog = GetNewDialog(kCaptureDlogID, UpdateCaptureDialog,
                                 CaptureDialogEventProc);
    if (captureDialog == NULL)
        return 0;
    captureDialogPict = GetPicture(kCapturePictID);
    CenterWindow(FrontWindow(captureDialog), gWind);
    ShowWindowNoFlash(FrontWindow(captureDialog));
    SetDialogGWorld(captureDialog);
    UpdateCaptureDialog();
    FrontWindow(captureDialog);
    FrontWindow(captureDialog);
    FlushEvents(everyEvent - diskMask, 0);
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(captureDialog, CaptureDialogEventProc, &itemHit, 0);
        if (itemHit == 1) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            result = 1;
            done = 1;
        }
        if (itemHit == 2) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            result = 0;
            done = 1;
        }
    } while (done == 0);
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialogNoFlash(captureDialog);
    captureDialog = NULL;
    if (captureDialogPict != NULL) {
        QDReleaseResource(captureDialogPict);
        captureDialogPict = NULL;
    }
    return result;
}

/* CaptureDialogEventProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00498040
char CaptureDialogEventProc(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int key;
    int what;

    what = ev->what;
    if (what == kKeyDown) {
        key = ev->message & kCharCodeMask;
        key = tolower(key);
        if (key == 'm') {
            *itemHit = 1;
            return 1;
        }
        if (key == 'e') {
            *itemHit = 2;
            return 1;
        }
    }
    if (what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdateCaptureDialog();
        button = TrackCapAssignButtons(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 2;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt)
        UpdateCaptureDialog();
    return 0;
}

/* UpdateCaptureDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00498110
void UpdateCaptureDialog(void)
{
    short itemType;
    void *item;
    Rect box;
    char text[256];

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(captureDialog));
    if (captureDialogPict != NULL)
        DrawPicHandle(captureDialogPict, GetDialogPortRect(captureDialog));
    RGBForeColor(whiteColor);
    BackColor(blackColor);
    GetDialogItem(captureDialog, 3, &itemType, &item, &box);
    strcpy(text, "");
    AppendIndString(text, kGameStrList, kCaptureStrIdx);
    TETextBox(text, strlen(text), &box, 0);
    RGBForeColor(blackColor);
    BackColor(whiteColor);
    DrawCapAssignButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(captureDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(captureDialog)),
             GetDialogPortRect(captureDialog), GetDialogPortRect(captureDialog), 0,
             GetCWindowVisRgn(FrontWindow(captureDialog)));
}

/*
 * The port's TE record. textedit.c calls +0x08 one `long unk8`; this driver
 * writes the two halves separately from smallTextFont/smallTextSize, which names
 * them.
 */

#define kTextDialogPictID      0xbbc     /* dlog 3004, with a picture item */
#define kTextDialogPlainID     0xbbb     /* dlog 3003 */
#define kTextDlgBackdropPictID 0x214c    /* three stacked backdrop pieces */
#define kTextDlgSinglePictID   0x214f
#define kTextDlgMinHeight      0x30
#define kTextDlgSlackMargin    16
#define kTextDlgRecentre       0.3
#define kScrollLineStep        10



                                                         /* 0x004d18b0 (mac _SizeWindow) */
                                                         /* 0x004d1a30 (mac _MoveWindow) */

/* ShowTextDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004982a0
void ShowTextDialog(char allowMap, char redrawMission, char showMovie)
{
    Rect box;
    char done;
    short itemHit;
    char savedUp;
    short itemType;
    void *item;
    short savedStellar;
    Rect *win;
    short savedRoute;
    Point pt;
    char savedDown;
    short savedBottom;
    short slack;
    short viewHeight;
    short textHeight;
    short i;

    done = 0;
    if (showMovie && gEarlyOutFlag == 0 && descMovieName[0] != 0
        && (gDescFlags & kDescMovieAfter) == 0)
        PlayQTMovie(descMovieName, (gDescFlags & kDescMovieDoubled),
                    (gDescFlags & kDescMovieFade));

    if (strlen(gTextShowString) == 0) {
        if (screenFaded) {
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    RGBForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    RGBForeColor(blackColor);
                    PaintRect(GetCWindowPortRect(gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                UpdateAllPortDialogs();
                RedrawStatus(1);
            }
            if (showingIntroText == 0)
                FadeScreenIn();
        }
        return;
    }

    strcat(gTextShowString, "\r");
    textDialog = NULL;
    if (descResourcePictId >= kFirstResourceID && showMovie) {
        textDialog = GetNewDialog(kTextDialogPictID, UpdateTextDialog,
                                  TextDialogEvent);
    } else {
        textDialog = GetNewDialog(kTextDialogPlainID, UpdateTextDialog,
                                  TextDialogEvent);
        descResourcePictId = -1;
    }
    if (textDialog == NULL)
        return;

    if (descResourcePictId >= kFirstResourceID && showMovie) {
        textDialogBackgroundPic[0] = GetPicture(kTextDlgSinglePictID);
        textDialogBackgroundPic[1] = NULL;
        textDialogBackgroundPic[2] = NULL;
    } else {
        for (i = 0; i < 3; i++)
            textDialogBackgroundPic[i] = GetPicture(kTextDlgBackdropPictID + i);
    }
    SetDialogGWorld(textDialog);
    GetDialogItem(textDialog, 3, &itemType, &item, &box);
    textDialogTextHandle = TENew(&box, &box);
    (*textDialogTextHandle)->txFont = smallTextFont;
    (*textDialogTextHandle)->txSize = smallTextSize;
    TextFont((*textDialogTextHandle)->txFont);
    TextSize((*textDialogTextHandle)->txSize);
    TESetText(gTextShowString, strlen(gTextShowString), textDialogTextHandle);
    textHeight = MeasureWrappedText((*textDialogTextHandle)->text,
                                    (*textDialogTextHandle)->textLength,
                                    box.right - box.left);
    GetDialogItem(textDialog, 3, &itemType, &item, &box);
    viewHeight = box.bottom - box.top;
    if (textHeight < viewHeight && descResourcePictId < kFirstResourceID) {
        textHeight = textHeight < kTextDlgMinHeight ? (short)kTextDlgMinHeight
                                                    : textHeight;
        win = GetDialogPortRect(textDialog);
        box.top = win->top;
        box.left = win->left;
        box.bottom = win->bottom;
        box.right = win->right;
        slack = viewHeight - textHeight - kTextDlgSlackMargin;
        box.bottom -= slack;
        SizeDialogWindow(textDialog, (box.right - box.left),
                     (box.bottom - box.top), 1);
        pt.v = 0;
        pt.h = 0;
        pt.v += slack * kTextDlgRecentre;
        OffsetDialogWindow(textDialog, pt.h, pt.v, 1);
        GetDialogItem(textDialog, 1, &itemType, &item, &box);
        OffsetRect(&box, 0, -slack);
        SetDialogItem(textDialog, 1, itemType, item, &box);
        GetDialogItem(textDialog, 5, &itemType, &item, &box);
        OffsetRect(&box, 0, -slack);
        SetDialogItem(textDialog, 5, itemType, item, &box);
        GetDialogItem(textDialog, 6, &itemType, &item, &box);
        OffsetRect(&box, 0, -slack);
        SetDialogItem(textDialog, 6, itemType, item, &box);
        GetDialogItem(textDialog, 3, &itemType, &item, &box);
        box.bottom -= slack;
        SetDialogItem(textDialog, 3, itemType, item, &box);
        GetDialogItem(textDialog, 3, &itemType, &item,
                      &(*textDialogTextHandle)->destRect);
        GetDialogItem(textDialog, 3, &itemType, &item,
                      &(*textDialogTextHandle)->viewRect);
        TECalText(textDialogTextHandle);
    }
    UpdateAllPortDialogs();
    CenterWindow(FrontWindow(textDialog), gWind);
    ShowWindowNoFlash(FrontWindow(textDialog));
    SetDialogGWorld(textDialog);
    lastTextScrollTime = ReadTick();
    FlushEvents(everyEvent - diskMask, 0);
    canScrollTextUp = 0;
    canScrollTextDown = 1;
    savedBottom = (*textDialogTextHandle)->destRect.bottom;
    TEPinScroll(0, -1, textDialogTextHandle);
    if ((*textDialogTextHandle)->destRect.bottom == savedBottom)
        canScrollTextDown = 0;
    TEPinScroll(0, 1, textDialogTextHandle);
    savedUp = canScrollTextUp;
    savedDown = canScrollTextDown;
    UpdateTextDialog();
    FrontWindow(textDialog);
    FrontWindow(textDialog);
    ShowTheCursor();

    if (gPause) {
        if (showingIntroText) {
            SetGWorldContainer(&loadPort);
            RGBForeColor(blackColor);
            PaintRect(&loadPort.bounds);
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(blackColor);
            PaintRect(GetCWindowPortRect(gWind));
        } else {
            mainScreenBackgroundDirty = 1;
            MainScreenUpdate(1);
        }
    }
    if (screenFaded) {
        if (gPause) {
            if (showingIntroText) {
                SetGWorldContainer(&loadPort);
                RGBForeColor(blackColor);
                PaintRect(&loadPort.bounds);
                SWSetPortToWindow(gSpriteWorld);
                RGBForeColor(blackColor);
                PaintRect(GetCWindowPortRect(gWind));
            } else {
                mainScreenBackgroundDirty = 1;
                MainScreenUpdate(1);
            }
        } else {
            RedrawStatus(1);
        }
        FadeScreenIn();
    }

    do {
        if ((*textDialogTextHandle)->viewRect.top
            > (*textDialogTextHandle)->destRect.top)
            canScrollTextUp = 1;
        else
            canScrollTextUp = 0;
        if (gQuit != 0)
            done = 1;
        RunModalDialog(textDialog, TextDialogEvent, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 5 && canScrollTextUp != 0)
            ScrollDialogText(textDialog, 3, textDialogTextHandle, kScrollLineStep, 1);
        if (itemHit == 6 && canScrollTextDown != 0)
            ScrollDialogText(textDialog, 3, textDialogTextHandle, -kScrollLineStep, 1);
        if (itemHit == 4 && allowMap) {
            savedStellar = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            if (portDialog != NULL) {
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateStellarDialog();
            }
            if (redrawMission) {
                SetDialogGWorld(missionDialog);
                RedrawDialog(missionDialog);
            }
            SetDialogGWorld(textDialog);
            RedrawDialog(textDialog);
            UpdateTextDialog();
        }
        SetDialogGWorld(textDialog);
        if (canScrollTextUp != savedUp) {
            InvalDialogItem(textDialog, 5);
            savedUp = canScrollTextUp;
        }
        if (canScrollTextDown != savedDown) {
            InvalDialogItem(textDialog, 6);
            savedDown = canScrollTextDown;
        }
    } while (done == 0);

    for (i = 0; i < 3; i++) {
        if (textDialogBackgroundPic[i] != NULL) {
            QDReleaseResource(textDialogBackgroundPic[i]);
            textDialogBackgroundPic[i] = NULL;
        }
    }
    TEDispose(textDialogTextHandle);
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialogNoFlash(textDialog);
    textDialog = NULL;

    if (gPause) {
        if (showingIntroText) {
            SetGWorldContainer(&loadPort);
            RGBForeColor(blackColor);
            PaintRect(&loadPort.bounds);
            SWSetPortToWindow(gSpriteWorld);
            RGBForeColor(blackColor);
            PaintRect(GetCWindowPortRect(gWind));
        } else {
            mainScreenBackgroundDirty = 1;
            MainScreenUpdate(1);
        }
    } else {
        UpdateAllPortDialogs();
    }
    if (descMovieName[0] != 0 && gEarlyOutFlag == 0
        && (gDescFlags & kDescMovieAfter) != 0) {
        PlayQTMovie(descMovieName, (gDescFlags & kDescMovieDoubled),
                    (gDescFlags & kDescMovieFade));
        if (screenFaded) {
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    RGBForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    RGBForeColor(blackColor);
                    PaintRect(GetCWindowPortRect(gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                UpdateAllPortDialogs();
                RedrawStatus(1);
            }
            FadeScreenIn();
        }
    }
}

/* CalcShipCanBuy (mac _CalcShipCanBuy). STATE: MATCH 100.0%, exe 212 / obj 212 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00498dc0
void CalcShipCanBuy(void)
{
    long price;
    long need;
    long tradeIn;
    short i;
    unsigned long long key;
    char sig[4];

    key = *(unsigned long long *)((char *)shipData + kLicenseCodePair);
    sig[0] = ((key ^ 0xef0971bf3d10d5ddLL) & kKeyMask) != 0x0c302b2a15333394LL;
    sig[1] = ((key ^ 0x921ec281c26516f7LL) & kKeyMask) != 0x0b5e33bfc5433b83LL;
    sig[2] = ((key ^ 0xab89232d6fa52638LL) & kKeyMask) != 0x0359021eca0c887fLL;
    sig[3] = ((key ^ 0x69d6a8458c7c1bd1LL) & kKeyMask) != 0x083ec9282b467090LL;
    for (i = 0; i < 4; i++) {
        if (sig[i] == 0) {
            canBuy = 0;
            return;
        }
    }

    if (shipBuyMode == 0) {
        if (gSelectedShip != -1) {
            if (shipData[0].type == gSelectedShip) {
                canBuy = 0;
            } else {
                price = ApplyPriceAndTechnologyFlux(
                            gShipTypes[gSelectedShip].cost, priceFluxMult[1],
                            gShipTypes[gSelectedShip].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                            PlayerShipTradeInPrice(), priceFluxMult[0],
                            gShipTypes[shipData[0].type].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                            tradeIn, priceFluxMult[1],
                            gShipTypes[shipData[0].type].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                need = price - tradeIn;
                if (need < 0)
                    need = 0;
                if (shipData[0].credits >= need)
                    canBuy = 1;
                else
                    canBuy = 0;
            }
        } else {
            canBuy = 0;
        }
        if (canBuy != 0) {
            if (PlayerMeetsRequirements(gShipTypes[gSelectedShip].requires[0],
                                        gShipTypes[gSelectedShip].requires[1]) == 0)
                canBuy = 0;
        }
    } else {
        if (gSelectedShip == -1) {
            canBuy = 0;
        } else {
            need = (ApplyPriceAndTechnologyFlux(
                        gShipTypes[gSelectedShip].cost, priceFluxMult[1],
                        gShipTypes[gSelectedShip].techLevel,
                        stellarData[shipData[0].targetStellar].techLevel) * 0.1);
            if (shipData[0].credits >= need)
                canBuy = 1;
            else
                canBuy = 0;
        }
    }
    if (canBuy != 0) {
        if (EvalMissionBitTestString(gShipTypes[gSelectedShip].availability) == 0)
            canBuy = 0;
    }
}

#define kStripCells 128
#define kCellWide 0x53
#define kCellHigh 0x36

/* SetupListRects (mac _SetupListRects). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00499150
void SetupListRects(Point *where)
{
    int i;
    int x;
    short row;
    short col;
    short midV;
    short midH;
    Point *origin;
    Rect *cell;
    Rect *icon;
    Rect *strip;

    i = 0;
    origin = where;
    cell = gCellRects;
    icon = gIconRects;
    do {
        row = (short)i / 4;
        col = (short)i % 4;
        SetRect(cell, col * kCellWide + origin->h, row * kCellHigh + origin->v,
                (col + 1) * kCellWide + origin->h + 1,
                (row + 1) * kCellHigh + origin->v + 1);
        midV = ((gCellRects[(short)i].top + gCellRects[(short)i].bottom) / 2);
        midH = ((gCellRects[(short)i].right + gCellRects[(short)i].left) / 2);
        SetRect(icon, midH - 0x10, midV - 0x18, midH + 0x10, midV + 8);
        i = i + 1;
        cell = cell + 1;
        icon = icon + 1;
    } while ((short)i < kGridCells);
    i = 0;
    x = 0;
    strip = pictCacheRect;
    do {
        SetRect(strip, x, 0, ((short)i + 1) * 0x20, 0x20);
        i = i + 1;
        x = x + 0x20;
        strip = strip + 1;
    } while ((short)i < kStripCells);
}

#define kScrollTicksToLines 0.01667     /* ~1/60: ticks to scroll units */

static __inline short CalcScroll(short dv)
{
    return (double)(ReadTick() - lastTextScrollTime) * dv * kScrollTicksToLines;
}

/* ScrollDialogText. STATE: MATCH 100.0%, exe 144 / obj 144 instrs. Notes: Research/notes/scrolldlgtext.md */
// FUNCTION: EVNOVA 0x00499270
void ScrollDialogText(DialogPtr dlg, short item, TEHandle hTE, short dv, char force)
{
    Rect box;
    short type;
    void *handle;
    short savedTop;

    if (force == 0) {
        dv = CalcScroll(dv);
        if (dv == 0)
            return;
    }

    SetGWorldContainer(&loadPort);
    savedTop = (*hTE)->destRect.top;
    (*hTE)->port = loadPort.world;
    RGBForeColor(whiteColor);
    BackColor(blackColor);
    TEPinScroll(0, dv, hTE);
    RGBForeColor(blackColor);
    BackColor(whiteColor);
    (*hTE)->port = GetDialogPort(textDialog);
    lastTextScrollTime = ReadTick();
    SetDialogGWorld(dlg);

    if ((*hTE)->viewRect.top > (*hTE)->destRect.top)
        canScrollTextUp = 1;
    else
        canScrollTextUp = 0;

    if ((*hTE)->destRect.top == savedTop && dv < 0)
        canScrollTextDown = 0;
    else if ((*hTE)->viewRect.bottom > (*hTE)->destRect.top)
        canScrollTextDown = 1;

    GetDialogItem(dlg, item, &type, &handle, &box);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(dlg);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort), GetPortBitMapForCopyBits(GetDialogPort(dlg)),
             &box, &box, 0, 0);
}


#define kScrollPageStep   250


/* TextDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00499440
char TextDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    void *item;
    short itemType;
    Point where;
    short button;
    int c;

    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map)) {
            *itemHit = 4;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnChar || c == kEscapeChar) {
                *itemHit = 1;
                return 1;
            }
        }
        if (canScrollTextUp != 0) {
            if (c == kUpArrowChar) {
                *itemHit = 5;
                return 1;
            }
            if (c == kHomeChar) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextUp = 0;
                return 1;
            }
            if (c == kPageUpChar) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, kScrollPageStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
        if (canScrollTextDown != 0) {
            if (c == kDownArrowChar) {
                *itemHit = 6;
                return 1;
            }
            if (c == kEndChar) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, -kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextDown = 0;
                return 1;
            }
            if (c == kPageDownChar) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, -kScrollPageStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
        SetDialogGWorld(textDialog);
        where.v = ev->where.v;
        where.h = ev->where.h;
        if (canScrollTextUp != 0) {
            GetDialogItem(textDialog, 5, &itemType, &item, &box);
            if (PtInRect(where.h, where.v, &box) && canScrollTextUp != 0) {
                lastTextScrollTime = ReadTick();
                while (StillDown()) {
                    BusyIdle();
                    ScrollDialogText(textDialog, 3, textDialogTextHandle, 1, 1);
                    SetDialogGWorld(textDialog);
                    DrawTextButtons(1);
                    while (lastTextScrollTime == ReadTick() && StillDown())
                        BusyIdle();
                }
                SetDialogGWorld(textDialog);
                DrawTextButtons(-1);
                *itemHit = -1;
                return 1;
            }
        }
        if (canScrollTextDown != 0) {
            GetDialogItem(textDialog, 6, &itemType, &item, &box);
            if (PtInRect(where.h, where.v, &box) && canScrollTextDown != 0) {
                lastTextScrollTime = ReadTick();
                while (StillDown()) {
                    BusyIdle();
                    ScrollDialogText(textDialog, 3, textDialogTextHandle, -1, 1);
                    SetDialogGWorld(textDialog);
                    DrawTextButtons(2);
                    while (lastTextScrollTime == ReadTick() && StillDown())
                        BusyIdle();
                }
                SetDialogGWorld(textDialog);
                DrawTextButtons(-1);
                *itemHit = -1;
                return 1;
            }
        }
        UpdateTextDialog();
        button = TrackTextButtons(where);
        if (button == 0)
            *itemHit = 1;
        else if (button == 1)
            *itemHit = 5;
        else if (button == 2)
            *itemHit = 6;
        else
            *itemHit = -1;
        SetDialogGWorld(textDialog);
        DrawTextButtons(-1);
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        if (gPause != 0)
            mainScreenBackgroundDirty = 1;
        UpdateTextDialog();
    }
    return 0;
}

/* UpdateTextDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00499870
void UpdateTextDialog(void)
{
    PicHandle loaded;
    PicHandle pic;
    Rect r;
    Rect r2;
    short itemType;
    void *item;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(textDialog));
    if (descResourcePictId < kFirstResourceID) {
        pic = textDialogBackgroundPic[1];
        if (pic != NULL) {
            QDGetPictureBounds(pic, &r);
            OffsetRect(&r, -r.left, -r.top);
            if (textDialogBackgroundPic[0] != NULL) {
                QDGetPictureBounds(textDialogBackgroundPic[0], &r2);
                OffsetRect(&r2, -r2.left, -r2.top);
                OffsetRect(&r, 0, r2.bottom);
            }
            OffsetRect(&r, GetDialogPortRect(textDialog)->left,
                       GetDialogPortRect(textDialog)->top);
            DrawPicHandle(textDialogBackgroundPic[1], &r);
        }
        if (textDialogBackgroundPic[0] != NULL) {
            QDGetPictureBounds(textDialogBackgroundPic[0], &r);
            OffsetRect(&r, -r.left, -r.top);
            OffsetRect(&r, GetDialogPortRect(textDialog)->left,
                       GetDialogPortRect(textDialog)->top);
            DrawPicHandle(textDialogBackgroundPic[0], &r);
        }
        pic = textDialogBackgroundPic[2];
        if (pic != NULL) {
            QDGetPictureBounds(pic, &r);
            OffsetRect(&r, -r.left, -r.top);
            OffsetRect(&r, GetDialogPortRect(textDialog)->left,
                       GetDialogPortRect(textDialog)->bottom - (r.bottom - r.top));
            DrawPicHandle(textDialogBackgroundPic[2], &r);
        }
    } else if (textDialogBackgroundPic[0] != NULL) {
        DrawPicHandle(textDialogBackgroundPic[0], GetDialogPortRect(textDialog));
    }
    DrawTextButtons(-1);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (descResourcePictId >= kFirstResourceID) {
        loaded = GetPicture(descResourcePictId);
        if (loaded != NULL) {
            GetDialogItem(textDialog, 2, &itemType, &item, &r);
            DrawPicHandle(loaded, &r);
            QDReleaseResource(loaded);
        }
    }
    (*textDialogTextHandle)->port = loadPort.world;
    GetDialogItem(textDialog, 3, &itemType, &item, &r);
    RGBForeColor(whiteColor);
    BackColor(blackColor);
    TEUpdate(&r, textDialogTextHandle);
    RGBForeColor(blackColor);
    BackColor(whiteColor);
    (*textDialogTextHandle)->port = GetDialogPort(textDialog);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(textDialog);
    GetDialogItem(textDialog, 1, &itemType, &item, &r);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(textDialog)),
             GetDialogPortRect(textDialog), GetDialogPortRect(textDialog), 0, NULL);
}

/*
 * 0x00591518 is the player-info key itself: the main-screen event proc at
 * 0x004959d0 opens this dialog on it, guarded by infoDialog == NULL, so
 * pressing it again while the dialog is up dismisses it.
 */


#define kPlayerInfoPages 4


/* the 3-picture composite; win 0x7d4be0/e4/e8 are one mac symbol's 3 slots */


/* per-mission state / record, only the fields this scan needs */
/* rank record, only the fields the net-worth/achievement scans touch */


/* pre-filled elsewhere (whatever grants the special item / mission text);
   this function only reads and measures them */


/* --- DoPlayerInfoDialog (0x00499c10) --- */

/* the port's TE record; only the fields this driver measures */

/*
 * 0x007d5278 is one mac symbol, _playerInfoText: three 4096-byte pages,
 * declared once at the head of the file; this function walks it.
 */
#define kPlayerInfoDlogID      0x3f9
#define kPlayerInfoFirstPictID 0x2146
#define kRetireQuestionIdx     0x123

/* DoPlayerInfoDialog (mac _DoPlayerInfoDialog). STATE: MATCH 100.0%, exe 470 / obj 470,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00499c10
void DoPlayerInfoDialog(void)
{
    void *item;
    short itemHit;
    Rect buttonBox[2];
    Rect r;
    Rect box6;
    short itemType;
    unsigned char buf[256];
    Rect *win;
    Rect *p7;
    short savedTop;
    short savedLeft;
    short savedBottom;
    short savedRight;
    char done;
    short i;
    short tallest;
    short height;
    short boxHeight;
    short delta;
    short dv;
    char *p;

    done = 0;
    if (screenFaded != 0)
        return;
    if (shipData[0].jumpTimer != 0)
        return;
    if (shipData[0].deathTimer > 0.0f)
        return;
    if (fadePending[0] != 0 || fadePending[1] != 0)
        return;
    ShipStatsToSystemInfo();
    playerInfoPage = 1;
    infoDialog = NULL;
    infoDialog = GetNewDialog(kPlayerInfoDlogID, UpdatePlayerInfoDialog,
                                    PlayerInfoDialogEvent);
    if (infoDialog == NULL)
        return;
    for (i = 0; i < 3; i++)
        playerInfoBackgroundPic[i] = GetPicture(kPlayerInfoFirstPictID + i);
    SetDialogGWorld(infoDialog);
    SetupPlayerInfoText();
    win = GetDialogPortRect(infoDialog);
    savedTop = win->top;
    savedLeft = win->left;
    savedBottom = win->bottom;
    savedRight = win->right;
    GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
    GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
    GetDialogItem(infoDialog, 7, &itemType, &item, p7 = &buttonBox[1]);
    textDialogTextHandle = TENew(&box6, &box6);
    (*textDialogTextHandle)->txFont = smallTextFont;
    (*textDialogTextHandle)->txSize = smallTextSize;
    TextFont((*textDialogTextHandle)->txFont);
    TextSize((*textDialogTextHandle)->txSize);
    tallest = -1;
    for (i = 0, p = playerInfoText[0]; i < 3; i++, p += 4096) {
        TESetText(p, strlen(p), textDialogTextHandle);
        TECalText(textDialogTextHandle);
        height = (*textDialogTextHandle)->height;
        if (height > tallest || tallest <= 0)
            tallest = height;
    }
    TEDispose(textDialogTextHandle);
    boxHeight = box6.bottom - box6.top;
    if (tallest > boxHeight) {
        delta = tallest - boxHeight + 4;
        r.top = savedTop;
        r.left = savedLeft;
        r.bottom = savedBottom;
        r.right = savedRight;
        r.bottom += delta;
        SizeDialogWindow(infoDialog, (r.right - r.left),
                     (r.bottom - r.top), 1);
        dv = 0;
        dv -= delta * 0.5;
        OffsetDialogWindow(infoDialog, 0, dv, 1);
        r.top = buttonBox[0].top;
        r.left = buttonBox[0].left;
        r.bottom = buttonBox[0].bottom;
        r.right = buttonBox[0].right;
        OffsetRect(&r, 0, delta);
        GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
        SetDialogItem(infoDialog, 1, itemType, item, &r);
        r.top = buttonBox[1].top;
        r.left = buttonBox[1].left;
        r.bottom = buttonBox[1].bottom;
        r.right = buttonBox[1].right;
        GetDialogItem(infoDialog, 7, &itemType, &item, p7);
        OffsetRect(&r, 0, delta);
        SetDialogItem(infoDialog, 7, itemType, item, &r);
        r.top = box6.top;
        r.left = box6.left;
        r.bottom = box6.bottom;
        r.right = box6.right;
        r.bottom += delta;
        GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
        SetDialogItem(infoDialog, 6, itemType, item, &r);
    }
    CenterWindow(FrontWindow(infoDialog), gWind);
    ShowWindowNoFlash(FrontWindow(infoDialog));
    SetDialogGWorld(infoDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(infoDialog);
    UpdatePlayerInfoDialog();
    FrontWindow(infoDialog);
    FrontWindow(infoDialog);
    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(infoDialog, PlayerInfoDialogEvent, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit >= 2 && itemHit <= 5) {
            playerInfoPage = itemHit - 1;
            RedrawDialog(infoDialog);
        }
        if (itemHit == 7 && playerInfoPage == 2) {
            if (PlayerHasJettisonableCargo()) {
                GetIndString(buf, kGameStrList, kRetireQuestionIdx);
                if (EVYesNoDialog(buf)) {
                    JettisonCargo(1);
                    done = 1;
                }
            }
            RedrawDialog(infoDialog);
        }
    } while (done == 0);
    SWSetPortToWindow(gSpriteWorld);
    while (KeyCheck(keyPrefs.playerInfo))
        BusyIdle();
    DisposeDialogNoFlash(infoDialog);
    infoDialog = NULL;
    for (i = 0; i < 3; i++) {
        if (playerInfoBackgroundPic[i] != NULL) {
            QDReleaseResource(playerInfoBackgroundPic[i]);
            playerInfoBackgroundPic[i] = NULL;
        }
    }
    SWSetPortToWindow(gSpriteWorld);
    FlushEvents(7, 0);
}

/* PlayerInfoDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049a3a0
char PlayerInfoDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int c;

    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        c = ev->message & kCharCodeMask;
        if (c == kTabChar) {
            if ((*(unsigned short *)&ev->modifiers & shiftKey) != 0)
                playerInfoPage -= 1;
            else
                playerInfoPage += 1;
            if (playerInfoPage > kPlayerInfoPages)
                playerInfoPage = 1;
            if (playerInfoPage < 1)
                playerInfoPage = kPlayerInfoPages;
            RedrawDialog(infoDialog);
            *itemHit = playerInfoPage + 1;
            return 1;
        }
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnChar || c == kEscapeChar || KeyCheck(keyPrefs.playerInfo)) {
                *itemHit = 1;
                return 1;
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdatePlayerInfoDialog();
        button = TrackPlayerInfoButtons(playerInfoPage, where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 2;
            return 1;
        }
        if (button == 2) {
            *itemHit = 3;
            return 1;
        }
        if (button == 3) {
            *itemHit = 4;
            return 1;
        }
        if (button == 4) {
            *itemHit = 5;
            return 1;
        }
        if (button == 5) {
            *itemHit = 7;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        UpdatePlayerInfoDialog();
        return 0;
    }
    return 0;
}

/* UpdatePlayerInfoDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049a540
void UpdatePlayerInfoDialog(void)
{
    Rect box2;
    Rect box;
    short itemType;
    void *itemHandle;
    unsigned char cheatBuf[256];
    unsigned char buf[2048];
    short i;
    long stellarWorth;
    long escortWorth;
    short fuelWhole;
    short sys;
    short jumps;
    short count;
    char onMission;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(infoDialog));

    if (playerInfoBackgroundPic[1] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[1], &box);
        OffsetRect(&box, -box.left, -box.top);
        if (playerInfoBackgroundPic[0] != NULL) {
            QDGetPictureBounds(playerInfoBackgroundPic[0], &box2);
            OffsetRect(&box2, -box2.left, -box2.top);
            OffsetRect(&box, 0, box2.bottom);
        }
        if (playerInfoBackgroundPic[2] != NULL) {
            QDGetPictureBounds(playerInfoBackgroundPic[2], &box2);
            box.bottom = (GetDialogPortRect(infoDialog)->bottom -
                             GetDialogPortRect(infoDialog)->top) -
                            (box2.bottom - box2.top);
        }
        QDGetPictureBounds(playerInfoBackgroundPic[1], &box2);
        if (box.bottom - box.top < box2.bottom - box2.top)
            box.bottom = box.top + (box2.bottom - box2.top);
        OffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                   GetDialogPortRect(infoDialog)->top);
        DrawPicHandle(playerInfoBackgroundPic[1], &box);
    }

    if (playerInfoBackgroundPic[0] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[0], &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                   GetDialogPortRect(infoDialog)->top);
        DrawPicHandle(playerInfoBackgroundPic[0], &box);
    }

    if (playerInfoBackgroundPic[2] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[2], &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                   GetDialogPortRect(infoDialog)->bottom -
                   (box.bottom - box.top));
        DrawPicHandle(playerInfoBackgroundPic[2], &box);
    }

    TextFont(smallTextFont);
    TextSize(smallTextSize);
    GetDialogItem(infoDialog, 6, &itemType, &itemHandle, &box);
    if (!RectInRgn(&box, GetCWindowVisRgn(FrontWindow(infoDialog))))
        goto blit_back;

    if (playerInfoPage == 1) {
        /* --- left column --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0xc));
        DrawIndString(kGameStrList, 0xfb);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0xc));
        DrawString(playerName);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x1c));
        DrawIndString(kGameStrList, 0xfc);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x1c));
        DateToMedString(gameTime, (char *)buf);
        CtoPstr((char *)buf);
        DrawString(buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x2c));
        DrawIndString(kGameStrList, 0xfd);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x2c));
        DrawString(gSystems[shipData->curSystem].name);
        if (cheatsActive) {
            DrawString((unsigned char *)"\p (");
            NumToString(shipData->curSystem + 128, cheatBuf);
            DrawString(cheatBuf);
            DrawString((unsigned char *)"\p)");
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x3c));
        DrawIndString(kGameStrList, 0x146);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x3c));
        if (SystemIsInhabited(shipData->curSystem)) {
            DrawLegalStatusString(shipData->curSystem);
            if (cheatsActive) {
                DrawString((unsigned char *)"\p (");
                NumToString(playerLegalStatus[shipData->curSystem], cheatBuf);
                DrawString(cheatBuf);
                DrawString((unsigned char *)"\p)");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x4c));
        DrawIndString(kGameStrList, 0xfe);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x4c));
        DrawCombatRatingString();
        if (cheatsActive) {
            DrawString((unsigned char *)"\p (");
            NumToString(playerKills, cheatBuf);
            DrawString(cheatBuf);
            DrawString((unsigned char *)"\p)");
        }

        /* --- right column --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0xc));
        DrawIndString(kGameStrList, 0xff);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0xc));
        DrawString(shipName);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x1c));
        DrawIndString(kGameStrList, 0x100);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x1c));
        DrawString(gShipTypes[shipData->type].name);
        if (cheatsActive) {
            DrawString((unsigned char *)"\p (");
            NumToString(shipData->type + 128, cheatBuf);
            DrawString(cheatBuf);
            DrawString((unsigned char *)"\p)");
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x2c));
        DrawIndString(kGameStrList, 0x101);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x2c));
        NumToString((ShipTurnRate(shipData) * 30.0f), buf);
        DrawString(buf);
        DrawIndString(kGameStrList, 0x102);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x3c));
        DrawIndString(kGameStrList, 0x103);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x3c));
        NumToString((ShipAccelRate(shipData) * 2500.0), buf);
        DrawString(buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x4c));
        DrawIndString(kGameStrList, 0x104);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x4c));
        if (strictPlay)
            NumToString((ShipMaxSpeed(shipData) * 100.0f), buf);
        else
            NumToString((ShipMaxSpeed(shipData) * 100.0f *
                               0.6666666666666666), buf);
        DrawString(buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x5c));
        pstrcpy(buf, creditsString);
        if ((char)buf[0] > 0)
            buf[1] = toupper((char)buf[1]);
        DrawString(buf);
        DrawString((unsigned char *)"\p:");
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x5c));
        DrawCommaNumber(shipData->credits);

        /* --- shields / armor / fuel, left column continued --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x5c));
        DrawString(shieldString[1]);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x5c));
        if (ShipShieldCapacity(shipData) > 0.0) {
            if (shipData->shield >= 0.0f) {
                if (ShipShieldCapacity(shipData) >= shipData->shield) {
                    NumToString((shipData->shield /
                                       ShipShieldCapacity(shipData) * 100.0), buf);
                    DrawString(buf);
                    DrawString((unsigned char *)"\p%");
                } else {
                    DrawString((unsigned char *)"\p100%");
                }
            } else {
                DrawString((unsigned char *)"\p0%");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x6c));
        DrawString(armorString[1]);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x6c));
        if (IsDying(shipData)) {
            DrawIndString(kGameStrList, 0x105);
        } else if (ShipArmorCapacity(shipData) > 0.0) {
            if (ShipArmorCapacity(shipData) >= shipData->armor) {
                NumToString((shipData->armor /
                                   ShipArmorCapacity(shipData) * 100.0), buf);
                DrawString(buf);
                DrawString((unsigned char *)"\p%");
            } else {
                DrawString((unsigned char *)"\p100%");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x7c));
        DrawString(fuelString[5]);
        RGBForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x7c));
        if (ShipFuelCapacity(shipData) <= 0.0) {
            DrawIndString(kGameStrList, 0x14f);
        } else {
            NumToString((shipData->fuel /
                               ShipFuelCapacity(shipData) * 100.0), buf);
            DrawString(buf);
            DrawString((unsigned char *)"\p%");
            jumps = (shipData->fuel / 100.0);
            if (jumps > 0) {
                DrawString((unsigned char *)"\p  (");
                NumToString(jumps, buf);
                DrawString(buf);
                DrawString((unsigned char *)"\p ");
                if (jumps > 1)
                    DrawIndString(kGameStrList, 0xf0);
                else
                    DrawIndString(kGameStrList, 0xef);
                fuelWhole = shipData->fuel;
                if (fuelWhole % 100 > 0) {
                    DrawString((unsigned char *)"\p ");
                    DrawIndString(kGameStrList, 0x106);
                    DrawString((unsigned char *)"\p ");
                    DrawString(fuelString[6]);
                }
                DrawString((unsigned char *)"\p)");
            } else if (shipData->fuel > 0.0f) {
                DrawString((unsigned char *)"\p  (");
                DrawString(fuelString[6]);
                DrawString((unsigned char *)"\p ");
                DrawIndString(kGameStrList, 0x107);
                DrawString((unsigned char *)"\p)");
            }
        }

        /* --- net worth: escort resale value + known-system tribute + rank bonus --- */
        stellarWorth = 0;
        escortWorth = 0;
        for (i = 1; i < 0x40; i++) {
            if (shipData[i].active == 0)
                continue;
            if (shipData[i].parentShip != 0)
                continue;
            onMission = 0;
            if (shipData[i].specialMission != -1 &&
                currentMissionObjectives[shipData[i].specialMission].active != 0 &&
                currentMissions[shipData[i].specialMission].shipBehav == 1)
                onMission = 1;
            if (onMission)
                continue;
            if (shipData[i].hired == 0)
                continue;
            escortWorth = (gShipTypes[shipData[i].type].cost * 0.01 +
                                  escortWorth);
        }

        for (i = 0; i < 0x800; i++) {
            if (stellarData[i].loaded == 0 || stellarData[i].dominated == 0)
                continue;
            sys = stellarData[i].system;
            if (sys < 0 || sys >= 0x800)
                continue;
            if (gSystems[sys].loaded == 0 || gSystems[sys].prepared == 0)
                continue;
            stellarWorth += stellarData[i].tribute;
        }

        for (i = 0; i < 0x80; i++) {
            if (rankData[i].active == 0 || rankData[i].loaded == 0)
                continue;
            if (rankData[i].salary == 0)
                continue;
            if (shipData->credits < rankData[i].salaryCap || rankData[i].salaryCap <= 0)
                stellarWorth += rankData[i].salary;
        }

        if (stellarWorth < 0) {
            escortWorth -= stellarWorth;
            stellarWorth = 0;
        }
        if (escortWorth > 0 || stellarWorth > 0) {
            RGBForeColor(&rgbLightGrey);
            MoveTo((box.left + 5), (box.top + 0x8c));
            if (escortWorth > stellarWorth) {
                DrawIndString(kGameStrList, 0x108);
                DrawString((unsigned char *)"\p  ");
                RGBForeColor(whiteColor);
                DrawCommaNumber(escortWorth - stellarWorth);
            } else if (escortWorth == 0) {
                DrawIndString(kGameStrList, 0x109);
                DrawString((unsigned char *)"\p  ");
                RGBForeColor(whiteColor);
                DrawCommaNumber(stellarWorth);
            } else {
                DrawIndString(kGameStrList, 0x10a);
                DrawString((unsigned char *)"\p ");
                DrawIndString(kGameStrList, 0x109);
                DrawString((unsigned char *)"\p  ");
                RGBForeColor(whiteColor);
                DrawCommaNumber(stellarWorth - escortWorth);
            }
            DrawString((unsigned char *)"\p ");
            DrawString(creditsString);
            RGBForeColor(&rgbLightGrey);
            DrawString((unsigned char *)"\p ");
            DrawIndString(kGameStrList, 0x10b);
        }
    } else if (playerInfoPage == 2) {
        if (PlayerHasAnyCargo()) {
            RGBForeColor(blackColor);
            box2.top = box.top;
            box2.left = box.left;
            box2.bottom = box.bottom;
            box2.right = box.right;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[0], strlen(playerInfoText[0]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            if (TotalFleetHolds() > ShipTotalHolds())
                GetIndString(buf, kGameStrList, 0x10c);
            else
                GetIndString(buf, kGameStrList, 0x10d);
            DrawStringCentered(buf, box.left, box.right, (box.top + 0x37));
        }
    } else if (playerInfoPage == 3) {
        count = 0;
        for (i = 0; i < 0x200; i++) {
            if (shipSystems[i] > 0 && lcOutfitName[i][0] != 0 &&
                    (gOutfits[i].flags & kOutfShowInRanks) == 0) {
                count++;
                break;
            }
        }
        if (count > 0) {
            RGBForeColor(blackColor);
            box2.top = box.top;
            box2.left = box.left;
            box2.bottom = box.bottom;
            box2.right = box.right;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[1], strlen(playerInfoText[1]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            GetIndString(buf, kGameStrList, 0x10e);
            DrawStringCentered(buf, box.left, box.right, (box.top + 0x37));
        }
    } else if (playerInfoPage == 4) {
        count = 0;
        for (i = 0; i < 0x80; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0) {
                count++;
                break;
            }
        }
        for (i = 0; i < 0x200; i++) {
            if (shipSystems[i] > 0 && lcOutfitName[i][0] != 0 &&
                    (gOutfits[i].flags & kOutfShowInRanks) != 0)
                count++;
        }
        if (count > 0) {
            RGBForeColor(blackColor);
            box2.top = box.top;
            box2.left = box.left;
            box2.bottom = box.bottom;
            box2.right = box.right;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[2], strlen(playerInfoText[2]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            GetIndString(buf, kGameStrList, 0x10f);
            DrawStringCentered(buf, box.left, box.right, (box.top + 0x37));
        }
    }

blit_back:
    DrawPlayerInfoButtons(playerInfoPage, -1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(infoDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(infoDialog)),
             GetDialogPortRect(infoDialog),
             GetDialogPortRect(infoDialog),
             0,
             GetCWindowVisRgn(FrontWindow(infoDialog)));
}


#define kStrCargoHeader    0x16a
#define kStrFreeSpace      0x16c
#define kStrHold           0x16d
#define kStrHolds          0x16e
#define kStrNo             0x150
#define kStrFree           0x171
#define kStrA              0x189
#define kStrAn             0x18a
#define kStrOutfitHeader   0x110
#define kStrTradeInValue   0x111
#define kStrAwardsHeader   0x112

/* SetupPlayerInfoText. STATE: MATCH 100.0% 1304/1304, byte-identical yes. */
// FUNCTION: EVNOVA 0x0049c050
void SetupPlayerInfoText(void)
{
    char rankShown[128];
    long credits;
    char c;
    char starred;
    short found;
    short nextWeight;
    short maxWeight;
    short weight;
    short slot;
    short count;
    short freeSpace;
    short space;
    short shown;
    short len;
    short numJunk;
    short numTypes;
    short type;
    short qty;
    short i;
    short cargoQty[256];
    short outfitQty[512];
    unsigned char str[256];

    playerInfoText[0][0] = 0;
    playerInfoText[1][0] = 0;
    playerInfoText[2][0] = 0;
    numJunk = 0;
    numTypes = 0;
    for (qty = 0; qty < kMaxCargo; qty++)
        cargoQty[qty] = -1;

    for (qty = 0; qty <= 5; qty++) {
        if (shipData->cargo[qty] > 0) {
            if (cargoQty[qty] < 0)
                cargoQty[qty] = 0;
            cargoQty[qty] += shipData->cargo[qty];
        }
    }

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0 &&
            currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0) {
            type = currentMissions[i].cargoType;
            if (cargoQty[type] < 0)
                cargoQty[type] = 0;
            cargoQty[type] += currentMissions[i].cargoQty;
        }
    }

    for (i = 0; i < kMaxCargo; i++) {
        if (cargoQty[i] >= 0)
            numTypes++;
    }

    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            numJunk++;
            numTypes++;
        }
    }

    AppendIndString(playerInfoText[0], kGameStrList, kStrCargoHeader);
    strcat(playerInfoText[0], " ");
    if (TotalFleetHolds() > ShipTotalHolds())
        AppendIndString(playerInfoText[0], kGameStrList, kStrHolds);
    else
        AppendIndString(playerInfoText[0], kGameStrList, kStrHold);
    strcat(playerInfoText[0], ":\r\r");
    len = strlen(playerInfoText[0]);

    if (numTypes > 0) {
        shown = 0;
        for (i = 0; i < kMaxCargo; i++) {
            qty = cargoQty[i];
            if (qty >= 0) {
                starred = (lcCargoName[i][1] == '*') ? (char)1 : (char)0;
                if (qty > 0 && starred == 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(qty, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(qty, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (qty == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    AppendIndString(playerInfoText[0], kGameStrList, kStrOf);
                    strcat(playerInfoText[0], " ");
                }
                /* BUG: copies 64 bytes of a Pascal name, so a name over 63 characters keeps stale bytes */
                strncpy((char *)str, (char *)lcCargoName[i], 64);
                PtoCstr(str);
                if (lcCargoName[i][1] == '*')
                    strcat(playerInfoText[0], (char *)&str[1]);
                else
                    strcat(playerInfoText[0], (char *)str);
                if (shown < numTypes - 2) {
                    strcat(playerInfoText[0], ", ");
                } else if (shown == numTypes - 2) {
                    if (starred != 0) {
                        strcat(playerInfoText[0], ", ");
                    } else {
                        if (numTypes >= 3)
                            strcat(playerInfoText[0], ",");
                        strcat(playerInfoText[0], " ");
                        AppendIndString(playerInfoText[0], kGameStrList, kStrAnd);
                        strcat(playerInfoText[0], " ");
                    }
                }
                shown++;
            }
        }

        if (numJunk > 0) {
            for (type = 0; type < kMaxJunks; type++) {
                if (junkData[type].held > 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(junkData[type].held, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(junkData[type].held, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (junkData[type].held == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    AppendIndString(playerInfoText[0], kGameStrList, kStrOf);
                    strcat(playerInfoText[0], " ");
                    /* BUG: a junk name of 64+ characters is left unterminated for the strcat below */
                    strncpy((char *)str, junkData[type].lcName, 64);
                    strcat(playerInfoText[0], (char *)str);
                    if (shown < numTypes - 2)
                        strcat(playerInfoText[0], ", ");
                    if (shown == numTypes - 2) {
                        if (numTypes >= 3)
                            strcat(playerInfoText[0], ",");
                        strcat(playerInfoText[0], " ");
                        AppendIndString(playerInfoText[0], kGameStrList, kStrAnd);
                        strcat(playerInfoText[0], " ");
                    }
                    shown++;
                }
            }
        }

        if (len != -1)
            playerInfoText[0][len] = toupper(playerInfoText[0][len]);
        strcat(playerInfoText[0], ".");
        strcat(playerInfoText[0], "\r\r");

        freeSpace = TotalFleetHolds() - TotalCargo(shipData);
        if (freeSpace > 0) {
            if (freeSpace < 0)
                freeSpace = 0;
            NumToString(freeSpace, str);
            if (TotalFleetHolds() > ShipTotalHolds()) {
                AppendIndString(playerInfoText[0], kGameStrList, kStrFreeSpace);
                strcat(playerInfoText[0], " ");
                AppendIndString(playerInfoText[0], kGameStrList, kStrHolds);
                strcat(playerInfoText[0], ":  ");
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
                strcat(playerInfoText[0], " ");
                if (freeSpace == 1)
                    pstrcpy(str, tonString);
                else
                    pstrcpy(str, tonsString);
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
                space = TotalMissionCargoSpace();
                if (space < 0)
                    space = 0;
                if (space != freeSpace && space > 0) {
                    strcat(playerInfoText[0], "  (");
                    NumToString(space, str);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    if (space == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    AppendIndString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                } else if (space == 0 && freeSpace > 0) {
                    strcat(playerInfoText[0], "(");
                    AppendIndString(playerInfoText[0], kGameStrList, kStrNo);
                    strcat(playerInfoText[0], " ");
                    AppendIndString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                }
            } else {
                AppendIndString(playerInfoText[0], kGameStrList, kStrFreeSpace);
                strcat(playerInfoText[0], ": ");
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
                strcat(playerInfoText[0], " ");
                if (freeSpace == 1)
                    pstrcpy(str, tonString);
                else
                    pstrcpy(str, tonsString);
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
            }
        }
    }

    AppendIndString(playerInfoText[1], kGameStrList, kStrOutfitHeader);
    strcat(playerInfoText[1], "\r\r");

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        qty = shipSystems[i];
        if (qty > 0 && lcOutfitName[i][0] != 0 &&
            (gOutfits[i].flags & kOutfShowInRanks) == 0) {
            slot = gOutfits[i].sameNameIndex;
            if (slot < 0 || slot >= i)
                slot = i;
            if (outfitQty[slot] == 0) {
                weight = gOutfits[slot].dispWeight;
                if (weight > maxWeight || count == 0)
                    maxWeight = weight;
                count++;
            }
            outfitQty[slot] += qty;
        }
    }

    numTypes = 0;
    len = strlen(playerInfoText[1]);
    do {
        found = 0;
        type = 0;
        nextWeight = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && lcOutfitName[i][0] != 0 &&
                (gOutfits[i].flags & kOutfShowInRanks) == 0) {
                weight = gOutfits[i].dispWeight;
                if (maxWeight == weight) {
                    found++;
                    if (qty == 1) {
                        c = tolower(lcOutfitName[i][1]);
                        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                            AppendIndString(playerInfoText[1], kGameStrList, kStrAn);
                        else
                            AppendIndString(playerInfoText[1], kGameStrList, kStrA);
                    } else if (qty == 2) {
                        AppendIndString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 2);
                    } else if (qty == 3) {
                        AppendIndString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 3);
                    } else if (qty > 3) {
                        NumToString(qty, str);
                        PtoCstr(str);
                        strcat(playerInfoText[1], (char *)str);
                    }
                    strcat(playerInfoText[1], " ");
                    if (qty <= 3 && len > 0)
                        playerInfoText[1][len] = toupper(playerInfoText[1][len]);
                    len = -1;
                    if (qty > 1)
                        pstrcpy(str, lcPluralOutfitName[i]);
                    else
                        pstrcpy(str, lcOutfitName[i]);
                    PtoCstr(str);
                    strcat(playerInfoText[1], (char *)str);
                    numTypes++;
                    if (numTypes <= count - 1 && count > 2)
                        strcat(playerInfoText[1], ",");
                    if (numTypes < count - 1) {
                        strcat(playerInfoText[1], " ");
                    } else if (numTypes == count - 1) {
                        strcat(playerInfoText[1], " ");
                        AppendIndString(playerInfoText[1], kGameStrList, kStrAnd);
                        strcat(playerInfoText[1], " ");
                    }
                    outfitQty[i] = 0;
                } else if (weight < maxWeight) {
                    if (weight > nextWeight || type == 0)
                        nextWeight = weight;
                }
                type++;
            }
        }
        maxWeight = nextWeight;
    } while (found > 0);

    strcat(playerInfoText[1], ".\r\r");

    credits = PlayerShipTradeInPrice();
    if (credits > 0) {
        AppendIndString(playerInfoText[1], kGameStrList, kStrTradeInValue);
        strcat(playerInfoText[1], " ");
        StrcatCommaNumber(credits, playerInfoText[1]);
        strcat(playerInfoText[1], " ");
        if (credits == 1)
            strcat(playerInfoText[1], creditCString);
        else
            strcat(playerInfoText[1], creditsCString);
        strcat(playerInfoText[1], "\r");
    }

    count = 0;
    for (i = 0; i < kMaxRanks; i++) {
        rankShown[i] = 0;
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            strlen(rankData[i].name) != 0) {
            weight = rankData[i].priority;
            if (weight > maxWeight || count == 0)
                maxWeight = weight;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    for (i = 0; i < kMaxOutfits; i++) {
        qty = shipSystems[i];
        if (qty > 0 && lcOutfitName[i][0] != 0 &&
            (gOutfits[i].flags & kOutfShowInRanks) != 0) {
            slot = gOutfits[i].sameNameIndex;
            if (slot < 0 || slot >= i)
                slot = i;
            outfitQty[slot] += qty;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (outfitQty[i] > 0 && lcOutfitName[i][0] != 0 &&
            (gOutfits[i].flags & kOutfShowInRanks) != 0)
            count++;
    }

    if (count > 0) {
        strcpy(playerInfoText[2], "");
        AppendIndString(playerInfoText[2], kGameStrList, kStrAwardsHeader);
        strcat(playerInfoText[2], "\r\r");
        shown = 0;
        do {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 &&
                    rankData[i].priority >= maxWeight && rankShown[i] == 0) {
                    strcat(playerInfoText[2], rankData[i].name);
                    shown++;
                    rankShown[i] = 1;
                    if (shown <= count - 1)
                        strcat(playerInfoText[2], ",");
                    if (shown < count - 1) {
                        strcat(playerInfoText[2], " ");
                    } else if (shown == count - 1) {
                        strcat(playerInfoText[2], " ");
                        AppendIndString(playerInfoText[2], kGameStrList, kStrAnd);
                        strcat(playerInfoText[2], " ");
                    }
                }
            }
            found = 0;
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 && rankShown[i] == 0) {
                    weight = rankData[i].priority;
                    if (weight > maxWeight || found == 0)
                        maxWeight = weight;
                    found++;
                }
            }
        } while (found > 0);

        if (shown != 0)
            len = -1;
        else
            len = strlen(playerInfoText[2]);

        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && lcOutfitName[i][0] != 0 &&
                (gOutfits[i].flags & kOutfShowInRanks) != 0) {
                if (qty == 1) {
                    c = tolower(lcOutfitName[i][1]);
                    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                        AppendIndString(playerInfoText[2], kGameStrList, kStrAn);
                    else
                        AppendIndString(playerInfoText[2], kGameStrList, kStrA);
                } else if (qty == 2) {
                    AppendIndString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 2);
                } else if (qty == 3) {
                    AppendIndString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 3);
                } else if (qty > 3) {
                    NumToString(qty, str);
                    PtoCstr(str);
                    strcat(playerInfoText[2], (char *)str);
                }
                strcat(playerInfoText[2], " ");
                if (qty <= 3 && len > 0)
                    playerInfoText[2][len] = toupper(playerInfoText[2][len]);
                len = -1;
                if (qty > 1)
                    pstrcpy(str, lcPluralOutfitName[i]);
                else
                    pstrcpy(str, lcOutfitName[i]);
                PtoCstr(str);
                strcat(playerInfoText[2], (char *)str);
                shown++;
                if (shown <= count - 1)
                    strcat(playerInfoText[2], ", ");
            }
        }
        strcat(playerInfoText[2], "\r");
    }
}

/* FrameDefaultButton (mac _FrameDefaultButton). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049d630
void FrameDefaultButton(void *dlg, short flag)
{
}

/* ApplyPriceAndTechnologyFlux. STATE: MATCH 100.0%, exe 114 / obj 114, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0049d640
long ApplyPriceAndTechnologyFlux(long cost, float multiplier, short itemTech, short portTech)
{
    long price;

    if (cost <= 0)
        return 0;

    price = cost;
    if (itemTech <= 5 && portTech <= 5 && itemTech < portTech && cost >= 100)
        price *= (100 - (portTech - itemTech) * 3) * kPortPriceScale;

    price *= multiplier;

    if (price > 100000) {
        price = price / 1000;
        price = price * 1000;
    } else if (price > 10000) {
        price = price / 100;
        price = price * 100;
    } else if (price > 100) {
        price = price / 10;
        price = price * 10;
    }

    if (cost > 0 && price <= 0)
        price = 1;

    return price;
}

/* HideTheMenuBar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049d7a0
void HideTheMenuBar(void)
{
}

/* ShowTheMenuBar (mac _ShowTheMenuBar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049d7b0
void ShowTheMenuBar(void)
{
}


/* StartLoopingMovie. STATE: MATCH. OPEN: none */
// FUNCTION: EVNOVA 0x0049d7c0
void StartLoopingMovie(const unsigned char *name, void *window, Rect *box)
{
    short found;
    CGrafPtr port;
    void *timeBase;
    char cname[0x100];
    char path[0x100];
    char scratch[0x108];
    short movieResId = 0;
    CGrafPtr *windowPort = (CGrafPtr *)window;

    if (gEarlyOutFlag != 0)
        return;

    if (loopingMovie != NULL)
        EndLoopingMovie();

    gMovieBox.left = box->left;
    gMovieBox.right = box->right;
    gMovieBox.top = box->top;
    gMovieBox.bottom = box->bottom;

    port = *windowPort;
    QTNewGWorldFromPtr(&gIntroMovieData, k16BE555PixelFormat, &gMovieBox, 0, 0, 0,
                       (long)port->pix.baseAddr, port->pix.rowBytes);

    PStr2CStr(cname, name);

    sprintf(path, "9:Nova Plug-ins:%s", cname);
    ExpandAPathToBufferNative(path, path);
    CStr2PStr(path, path);
    found = FSMakeFSSpec(0, 0, path, scratch);
    if ((int)found == 0)
        found = OpenMovieFile(scratch, &gMovieRefNum, 1);
    if ((int)found != 0) {
        PStr2CStr(cname, name);
        sprintf(path, "9:Nova Files:%s", cname);
        ExpandAPathToBufferNative(path, path);
        CStr2PStr(path, path);
        found = FSMakeFSSpec(0, 0, path, scratch);
        if ((int)found == 0)
            found = OpenMovieFile(scratch, &gMovieRefNum, 1);
        if ((int)found != 0)
            return;
    }

    found = NewMovieFromFile(&loopingMovie, gMovieRefNum, &movieResId, 0, 1, 0);
    CloseMovieFile(gMovieRefNum);
    if ((int)found != 0) {
        loopingMovie = NULL;
        return;
    }

    SetMovieGWorld(loopingMovie, gIntroMovieData, 0);
    SetMovieBox(loopingMovie, &gMovieBox);
    SetMoviePlayHints(loopingMovie, 2, 2);
    timeBase = GetMovieTimeBase(loopingMovie);
    SetTimeBaseFlags(timeBase, GetTimeBaseFlags(timeBase) | 1);
    GoToBeginningOfMovie(loopingMovie);
    SetMovieRate(loopingMovie, 0x10000);
    StartMovie(loopingMovie);
}

/* LoopingMovieIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049da80
void LoopingMovieIdle(void)
{
    if (loopingMovie != NULL) {
        MoviesTask(loopingMovie, 0);
        nv_UpdateDialogWindow(0);
        ScreenEndScene();
    }
}

/* EndLoopingMovie. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049dab0
void EndLoopingMovie(void)
{
    if (loopingMovie != NULL) {
        DisposeMovie(loopingMovie);
        loopingMovie = NULL;
        FUN_004fc310(gIntroMovieData);
        gIntroMovieData = NULL;
    }
}

/* the movie's idle callback; taken by address at 0x0049deb1 */
/* IntroMovieIdleProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049daf0
short IntroMovieIdleProc(void)
{
    ScreenEndScene();
    return 0;
}

#pragma options align=packed
typedef struct FileSpec {
    short vRefNum;
    long parID;
    char name[256];
} FileSpec;
#pragma options align=reset


#define SC_SPACE        0x39
#define SC_RETURN       0x1c
#define SC_KEYPADENTER  0x6a

/* PlayQTMovie. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049db00
void PlayQTMovie(unsigned char *name, unsigned char doubled, unsigned char fade)
{
    char nameBuf[256];
    FileSpec spec;
    short dw, dh;
    short w, h;
    long err;
    short r;
    short refNum;
    short resId;
    Rect movieBox;
    char pathBuf[2048];
    Rect scaled;
    Rect box;
    Movie movie;
    void *track;
    CGrafPtr port;
    float ratio;

    if (gEarlyOutFlag == 0) {
        PStr2CStr(nameBuf, name);
        sprintf(pathBuf, "9:Nova Plug-ins:%s", nameBuf);
        ExpandAPathToBufferNative(pathBuf, pathBuf);
        CStr2PStr(pathBuf, pathBuf);
        r = FSMakeFSSpec(0, 0, pathBuf, &spec);
        if ((long)r == 0)
            r = OpenMovieFile(&spec, &refNum, 1);
        err = r;
        if (err == 0)
            goto found;
        PStr2CStr(nameBuf, name);
        sprintf(pathBuf, "9:Nova Files:%s", nameBuf);
        ExpandAPathToBufferNative(pathBuf, pathBuf);
        CStr2PStr(pathBuf, pathBuf);
        r = FSMakeFSSpec(0, 0, pathBuf, &spec);
        if ((long)r == 0)
            r = OpenMovieFile(&spec, &refNum, 1);
        err = r;
        if (err == 0)
            goto found;
        return;
found:
        resId = 0;
        NewMovieFromFile(&movie, refNum, &resId, 0, 1, 0);
        CloseMovieFile(refNum);
        if (err != 0)
            return;
        if (fade != 0) {
            FadeScreenOut();
            HideTheCursor();
        }
        GetMovieBox(movie, &gMovieBox);
        box.left = gMovieBox.left;
        box.top = gMovieBox.top;
        box.right = gMovieBox.right;
        box.bottom = gMovieBox.bottom;
        OffsetRect(&box, -box.left, -box.top);
        if (doubled != 0) {
            scaled.top = 0;
            scaled.right = ((box.right - box.left) * 2);
            scaled.bottom = ((box.bottom - box.top) * 2);
            scaled.right += 1;
            scaled.bottom += 1;
            scaled.left = 0;
            OffsetRect(&scaled, -scaled.left, -scaled.top);
            box.top = scaled.top;
            box.left = scaled.left;
            box.bottom = scaled.bottom;
            box.right = scaled.right;
        }
        gMovieBox.left = box.left;
        gMovieBox.top = box.top;
        gMovieBox.right = box.right;
        gMovieBox.bottom = box.bottom;
        SetMovieBox(movie, &gMovieBox);
        if (fade != 0)
            GetCWindowPortRect(gWind);
        movieBox.bottom = gVideoHeight;
        movieBox.right = gVideoWidth;
        movieBox.top = 0;
        movieBox.left = 0;
        CenterRect(&box, &movieBox);
        gMovieBox.left = box.left;
        gMovieBox.top = box.top;
        gMovieBox.right = box.right;
        gMovieBox.bottom = box.bottom;
        SetMovieBox(movie, &gMovieBox);
        port = GetCWindowGrafPort(gWind);
        QTNewGWorldFromPtr(&gIntroMovieData, k16LE555PixelFormat, &gMovieBox, 0, 0, 0,
                           (long)port->pix.baseAddr +
                           box.top * port->pix.rowBytes +
                           (port->pix.pixelSize >> 3) * box.left,
                           port->pix.rowBytes);
        SetMovieGWorld(movie, gIntroMovieData, 0);
        track = GetMovieIndTrackType(movie, 1, 0x76696465, 1);
        SetTrackGWorld(track, gIntroMovieData, 0, IntroMovieIdleProc, 0);
        w = gVideoWidth;
        h = gVideoHeight;
        dw = box.right - box.left;
        dh = box.bottom - box.top;
        /* BUG: both letterbox fits compute the new bottom from box.left, not box.top */
        if (dw > w) {
            ratio = w / (float)dw;
            box.right = (box.left + dw * ratio);
            box.bottom = (box.left + dh * ratio);
        }
        dw = box.right - box.left;
        dh = box.bottom - box.top;
        if (dh > h) {
            ratio = h / (float)dh;
            box.right = (box.left + dw * ratio);
            box.bottom = (box.left + dh * ratio);
        }
        InsetRect(&box, -1, -1);
        FrameRect(&box);
        GoToBeginningOfMovie(movie);
        LoadMovieIntoRam(movie, 0, GetMovieDuration(movie), 0);
        if (fade != 0)
            FadeScreenIn();
        StartMovie(movie);
        while (IsMovieDone(movie) == 0) {
            MoviesTask(movie, 0);
            KeyboardKbhit();
            if (MouseReadButtons() != 0 || KeyIsDown(SC_SPACE) != 0 ||
                KeyIsDown(SC_RETURN) != 0 || KeyIsDown(SC_KEYPADENTER) != 0) {
                FlushEvents(0xf, 0);
                break;
            }
            LastTick = ReadTick();
            WaitTicks(2);
        }
        StopMovie(movie);
        if (fade != 0) {
            FadeScreenOut();
            ShowTheCursor();
        }
        DisposeMovie(movie);
        FUN_004fc310(gIntroMovieData);
        gIntroMovieData = NULL;
        SWSetPortToWindow(gSpriteWorld);
        RGBForeColor(blackColor);
        RedrawDialogs(1);
        return;
    }
}

/* PlayQTMovieFromDesc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049e3c0
void PlayQTMovieFromDesc(short id)
{
    extern char *strcpy(char *, const char *);   /* MSL, CALLED not inlined */

    strcpy(gDescText, "");
    LoadDescription(gDescText, id);
    if (descMovieName[0] != 0)
        PlayQTMovie(descMovieName, (gDescFlags & kDescMovieDoubled),
                    (gDescFlags & kDescMovieFade));
}

/* FUN_0049e420. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049e420
void FUN_0049e420(void)
{
}

/* DrawEscortMenuInFrame (mac _DrawEscortMenuInFrame). STATE: MATCH 100.0%, exe
   305 / obj 305, reloc 57 with 0 hidden, byte-identical. The `(long)status'
   casts hold the exe's 32-bit sign-extended compares; a plain `long status'
   scores 98.0% because a named 32-bit local is coloured out of the temp ring.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049e430
void DrawEscortMenuInFrame(void)
{
    Rect box;
    Rect bar;
    Rect *frame;
    char flags[4];
    Str255 str;
    short i;
    short category;
    short status;

    SWSetPortToFrame(floatingEscortMenuFrame);
    frame = &floatingEscortMenuFrame->frameRect;
    box.top = frame->top;
    box.left = frame->left;
    box.bottom = frame->bottom;
    box.right = frame->right;

    RGBForeColor(blackColor);
    PaintRect(&box);

    flags[0] = 0;
    flags[1] = 0;
    flags[2] = 0;
    flags[3] = 0;

    RGBForeColor(&floatingBorderColor);
    FrameRect(&box);

    RGBForeColor(whiteColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);

    GetIndString(str, kGameStrList, 0x85);
    MoveTo((box.left + box.right) / 2 - StringWidth(str) / 2, box.top + 0xf);
    DrawString(str);

    for (i = 1; i < 64; i++) {
        if (!shipData[i].active)
            continue;
        if (shipData[i].parentShip != 0)
            continue;
        category = gShipTypes[shipData[i].type].escortType;
        if (category < 0 || category > 3)
            continue;
        flags[category] = 1;
    }

    for (i = -1; i < 4; i++) {
        if (gSelectedQuickKey == i) {
            bar.left = box.left + 4;
            bar.right = box.right - 4;
            bar.top = box.top + 0x1e + (i + 1) * 18;
            bar.bottom = box.top + 0x2b + (i + 1) * 18;
            RGBForeColor(&escortMenuColor);
            PaintRect(&bar);
        }

        if (i == -1)
            RGBForeColor(whiteColor);
        else if (flags[i] != 0)
            RGBForeColor(whiteColor);
        else
            RGBForeColor(&rgbDarkGrey);

        MoveTo(box.left + 7, box.top + 0x28 + (i + 1) * 18);
        NumToString(i + 2, str);
        DrawString(str);
        DrawString((const unsigned char *)"\p) ");

        if (i == -1)
            DrawIndString(kGameStrList, 0x90);
        else if (i == 0)
            DrawIndString(kGameStrList, 0x8c);
        else if (i == 1)
            DrawIndString(kGameStrList, 0x8d);
        else if (i == 2)
            DrawIndString(kGameStrList, 0x8e);
        else if (i == 3)
            DrawIndString(kGameStrList, 0x8f);

        if (i != -1 && flags[i] != 0) {
            strcpy((char *)str, "");
            status = playerEscortOrders[i];
            if ((long)status == 1)
                GetIndString(str, kGameStrList, 0x91);
            else if ((long)status == 2)
                GetIndString(str, kGameStrList, 0x92);
            else if ((long)status == 4)
                GetIndString(str, kGameStrList, 0x93);
            else if ((long)status == 3)
                GetIndString(str, kGameStrList, 0x94);
            else
                GetIndString(str, kGameStrList, 0x95);
            RGBForeColor(whiteColor);
            MoveTo(box.right - StringWidth(str) - 7, box.top + 0x28 + (i + 1) * 18);
            DrawString(str);
        }
    }

    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
}

#define kDefaultMapZoom 0x20

/* ResetMapZoom. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049e8d0
void ResetMapZoom(void)
{
    escortMenuOnscreen = kDefaultMapZoom;
    DrawEscortMenuInFrame();
}


#define kQtyDlogID 0x3eb
#define kQtyPromptItem 1
#define kQtyMaxItem 2
#define kQtyTextItem 3

/* DoQuantityDialog (mac _DoQuantityDialog). STATE: MATCH 100.0%, exe 286 / obj 286,
   reloc 57 with 0 hidden, byte-identical. The digit filter reads `text[i + 1]' at
   each of its three sites: a named `ch' is coloured out of the temp ring and costs
   the whole eax<->ebx exchange. Both `(long)' casts are load-bearing -- without them
   the compares narrow and flip to unsigned. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049e8e0
long DoQuantityDialog(long maxCount, short mode)
{
    short i;
    void *item;
    short itemHit;
    long value;
    char done;
    Rect box;
    short itemType;
    long result;
    Str255 text;
    Str255 digits;
    char allDigits;
    short destIdx;

    qtyDialog = NULL;
    qtyDialog = GetNewDialog(kQtyDlogID, NULL, NULL);
    if (qtyDialog == NULL)
        return 0;

    CenterWindow(FrontWindow(qtyDialog), gWind);
    ShowMacWindow(FrontWindow(qtyDialog));
    FrontWindow(qtyDialog);
    SetDialogGWorld(qtyDialog);
    FrameDefaultButton(qtyDialog, 1);
    SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);

    GetDialogItem(qtyDialog, kQtyPromptItem, &itemType, &item, &box);
    if (mode == 1)
        SetControlTitle((ControlPtr)item,buttonLabel[2]);
    else if (mode == 2)
        SetControlTitle((ControlPtr)item,buttonLabel[0x3a]);
    else
        SetControlTitle((ControlPtr)item,buttonLabel[1]);
    if (mode == 2)
        GetIndString(text, kGameStrList, 0x174);
    else
        GetIndString(text, kGameStrList, 0x173);

    GetDialogItem(qtyDialog, kQtyMaxItem, &itemType, &item, &box);
    SetDialogItemText((ControlPtr)item,text);

    GetDialogItem(qtyDialog, kQtyTextItem, &itemType, &item, &box);
    NumToString(maxCount, text);
    SetDialogItemText((ControlPtr)item,text);

    done = 0;
    result = 0;
    do {
        ModalDialog(0, &itemHit);
        if (itemHit == 4) {
            done = 1;
        } else if (itemHit == 1) {
            GetDialogItem(qtyDialog, kQtyTextItem, &itemType, &item, &box);
            GetDialogItemText((ControlPtr)item,text);
            if (text[0] != 0) {
                allDigits = 1;
                digits[0] = 0;
                destIdx = 1;
                for (i = 0; i < text[0]; i++) {
                    if ((long)text[i + 1] >= '0' && (long)text[i + 1] <= '9') {
                        digits[0]++;
                        digits[destIdx] = text[i + 1];
                        destIdx++;
                    } else {
                        allDigits = 0;
                    }
                }
                if (allDigits) {
                    StringToNum(text, &value);
                    if (value < 0 || value > maxCount) {
                        SysBeep(0);
                        if (value >= 0)
                            NumToString(maxCount, text);
                        else
                            NumToString(0, text);
                        SetDialogItemText((ControlPtr)item,text);
                        SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
                    } else {
                        result = value;
                        done = 1;
                    }
                } else {
                    SysBeep(0);
                    SetDialogItemText((ControlPtr)item,digits);
                    SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
                }
            } else {
                SysBeep(0);
                SetDialogItemText((ControlPtr)item,(const unsigned char *)"\p");
                SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
            }
        }
    } while (!done);

    DisposeDialogNoFlash(qtyDialog);
    qtyDialog = NULL;
    return result;
}

#define kMapWorldWide 4096
#define kMapWorldHigh 32

/* InitMapWorld. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049ed00
void InitMapWorld(void)
{
    Rect r;
    short i;

    alertDialog = NULL;
    depthDialog = NULL;
    loadingDialog = NULL;
    tradeDialog = NULL;
    portDialog = NULL;
    outfitDialog = NULL;
    outfitDescDialog = NULL;
    shipyardDialog = NULL;
    shipDescDialog = NULL;
    infoDialog = NULL;
    missionDialog = NULL;
    barDialog = NULL;
    raceDialog = NULL;
    newsDialog = NULL;
    commDialog = NULL;
    haggleDialog = NULL;
    plunderDialog = NULL;
    mapDialog = NULL;
    captureDialog = NULL;
    for (i = 0; i < 128; i++) {
        pictCache[i].picID = -1;
        pictCache[i].stamp = 0;
    }
    nagCount = 0;
    SetRect(&r, 0, 0, kMapWorldWide, kMapWorldHigh);
    CreateGWorldContainer(&pictCachePort, 0, 0, &r);
}

/* DisposeMapWorld. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049eec0
void DisposeMapWorld(void)
{
    if (pictCachePort.world != NULL)
        DisposeGWorldContainer(&pictCachePort);
}

/* CrtInitHook5. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049eee0
void CrtInitHook5(void)
{
}

}

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
