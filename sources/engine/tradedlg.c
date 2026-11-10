// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
#include "cefix.h"
#include "ai.h"
#include "space.h"

#define kFirstRowItem  4
#define kLastRowItem   11
#define kLastRow       7
#define kBuyItem       13
#define kSellItem      14

PictCacheSlot iconCache[128];
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
char dialogDescTextPtr[8192];
short planetPicId;
short selOutfitItem;
short selShipyardItem;
short selOutfitListItem;
short selPlayerInfoMode;
short selShipListItem;
short lastClickItem;
short outfitScroll;
short shipyardScroll;
short shipBuyMode;
short loopingMovieResRefNum;
short junkType[2];
unsigned long lastClickTime;
float priceFluxMult[2];
PicHandle portBackgroundPic;
PicHandle bigShipPic;
PicHandle shipyardBackgroundPic;
PicHandle outfitBackgroundPic;
PicHandle shipDescBackgroundPic;
PicHandle captureBackgroundPic;
PicHandle tradeBackgroundPic;
PicHandle playerInfoBackgroundPic[3];
PicHandle textDialogBackgroundPic[3];
PicHandle previewPic;
void *planetSnd;
Boolean canBuy;
Boolean canSell;
Boolean canDoMissions;
Boolean shipyardEgg;
Boolean didOutfit;
Boolean didBuyShip;
#ifndef MACBUILD
unsigned char DAT_007d4c06[2];
#endif
Boolean boughtMap;
Boolean boughtID;
Boolean onPlanet = 0;
Boolean canScrollTextUp;
Boolean canScrollTextDown;
Boolean portBuysNoOutfits;
unsigned char textInputString[256];
Rect listPicOffRect[128];
Rect listScreenRect[20];
Rect listPicScreenRect[20];
void *loopingMovie = NULL;
TEHandle textDialogTextHandle;
unsigned long lastTextScrollTime;
unsigned long ambientNoiseDelay;
GWorldContainer iconsPort;
GWorldContainer planetPicContainer;
char playerInfoText[3][4096];
#ifdef MACBUILD
AEEventHandlerUPP OAPPHandlerUPP;
AEEventHandlerUPP ODOCHandlerUPP;
AEEventHandlerUPP PDOCHandlerUPP;
AEEventHandlerUPP QUITHandlerUPP;
AEEventHandlerUPP PREFHandlerUPP;
#endif
short nagCount;
short previewPicID;
RGBColor shopGridColor[2];
#ifndef MACBUILD
void *gIntroMovieData;
Rect gMovieBox;
#endif
#ifdef MACBUILD
extern long gSystemVersion;
extern Boolean introMusicPaused;
extern void SetRetainedWindow(WindowRef window, Boolean retained);
extern void KeepMenuBarHidden(GrafPtr port);
extern void AdjustMenus(void);
extern void DoMenuClick(long menuResult);
extern void KeyDialogUpdate(void);
void GetWindowDragBounds(Rect *bounds, Point where);
void ForceWindowOnscreen(void);
#endif
/* the menu icon cache draws into a port on mac, into the port's container on win */
#ifdef MACBUILD
#define MenuIconPort(gwc) ((gwc)->world)
#else
#define MenuIconPort(gwc) (gwc)
#endif

EXTERN_C_BEGIN

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


/* the mac engine invalidates a dialog item through the caller's own GetDialogItem locals */
#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &box), InvalWindowRect(GetDialogWindow(dlg), &box))
#endif
/* DoTradeDialog (mac _DoTradeDialog). STATE: MATCH, 628/628, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0048c730
// MAC: _DoTradeDialog i386 0x0005dbe1 ppc 0x000657f4
void DoTradeDialog(void)
{
#ifdef MACBUILD
    short i;
    short savedStellar;
    short savedRoute;
    short itemHit;
    short itemType;
    Handle item;
    Rect box;
    short mods;
    double mult;
    short j;
    short space;
    short maxSell;
    long afford;
    long qty;
    Boolean done;
#else
    Boolean done;
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
#endif

    done = 0;
    savedStellar = shipData->targetStellar;
    savedRoute = shipData->navTargetKind;
    mult = 1.25;
    if (dockedPort->govt != -1
        && playerRecord[dockedPort->system] < 0) {
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
            && disasterData[i].stellarID == shipData->targetStellar) {
            tradePrice[disasterData[i].commodity] =
                basePrice[disasterData[i].commodity];
            tradePrice[disasterData[i].commodity] += disasterData[i].amount;
            if (tradePrice[disasterData[i].commodity] < 5)
                tradePrice[disasterData[i].commodity] = 5;
        }
    }
    junkType[0] = -1;
    junkType[1] = -1;
    tradePrice[6] = tradePrice[7] = 0;
    for (i = 0; i < 0x80; i++) {
        for (j = 0; j < 8; j++) {
            if (junkData[i].boughtAt[j] == shipData->targetStellar) {
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
            if (junkData[i].soldAt[j] == shipData->targetStellar) {
                if (EvalMissionBitTestString(junkData[i].sellOn) != 0) {
                    junkType[1] = i;
                    tradePrice[7] = junkData[i].basePrice / mult;
                    break;
                }
            }
        }
    }
    missionsOutside = kTradeVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(0x1e) + 0x1e;
    tradeDialog = NULL;
    tradeDialog = GetNewDialog(kTradeDlogID, TradeDialogUpdate,
                               TradeFilter);
    if (tradeDialog == NULL)
        return;
    tradeBackgroundPic = GetPicture(kTradePictID);
    CenterWindow(FrontWindow(tradeDialog), FrontWindow(portDialog));
    selTradeItem = 7;
    TradeDialogTabKey(0);
    ShowWindowNoFlash(FrontWindow(tradeDialog));
    SetDialogGWorld(tradeDialog);
    TradeDialogUpdate();
    SelectWindow(FrontWindow(tradeDialog));
    BringToFront(FrontWindow(tradeDialog));
    OfferMissionFromPort(kTradeVenue);
    RedrawDialog(tradeDialog);
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(tradeDialog, TradeFilter, &itemHit, &mods);
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
#ifdef MACBUILD
                /* BUG: invalidates the trade window by the port dialog's bounds */
                InvalWindowRect(GetDialogWindow(tradeDialog), GetDialogPortRect(portDialog));
#else
                RedrawDialog(tradeDialog);
#endif
                UpdateAllPortDialogs();
                SetDialogGWorld(tradeDialog);
                RedrawDialog(tradeDialog);
            }
        }
        if (itemHit == kBuyItem) {
#ifdef MACBUILD
            InvalDialogItem(tradeDialog, kBuyItem);
            InvalDialogItem(tradeDialog, kSellItem);
#else
            RedrawDialog(tradeDialog);
#endif
            space = TotalFleetHolds() - TotalCargo(shipData);
            if (space > 0) {
                afford = shipData->credits / (float)tradePrice[selTradeItem];
                if ((mods & optionKey) != 0) {
                    if (afford > space)
                        qty = space;
                    else
                        qty = afford;
                    if (qty > kMaxTradeQty)
                        qty = kMaxTradeQty;
                    qty = DoQuantityDialog(qty, 0);
                    TradeDialogUpdate();
                } else {
                    qty = 10;
                    if (afford < 10)
                        qty = afford;
                    if (qty > space)
                        qty = space;
                }
                if (qty > 0) {
                    if (selTradeItem <= kLastCommodity)
                        shipData->cargo[selTradeItem] += qty;
                    else if (junkType[selTradeItem - kFirstJunkItem] != -1)
                        junkData[junkType[selTradeItem - kFirstJunkItem]]
                            .held += qty;
                    shipData->credits -= tradePrice[selTradeItem] * qty;
                }
#ifdef MACBUILD
                InvalDialogItem(tradeDialog, selTradeItem + kFirstRowItem);
#else
                RedrawDialog(tradeDialog);
#endif
                statusRedraw[2] = 1;
                DrawStatus();
                SetDialogGWorld(tradeDialog);
            }
            RedrawDialog(tradeDialog);
        }
        if (itemHit == kSellItem) {
#ifdef MACBUILD
            InvalDialogItem(tradeDialog, kBuyItem);
            InvalDialogItem(tradeDialog, kSellItem);
#else
            RedrawDialog(tradeDialog);
#endif
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
                    TradeDialogUpdate();
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
                    if (selTradeItem <= kLastCommodity)
                        shipData->cargo[selTradeItem] -= qty;
                    else if (junkType[selTradeItem - kFirstJunkItem] != -1)
                        junkData[junkType[selTradeItem - kFirstJunkItem]]
                            .held -= qty;
                    shipData->credits += tradePrice[selTradeItem] * qty;
                }
#ifdef MACBUILD
                InvalDialogItem(tradeDialog, selTradeItem + kFirstRowItem);
#else
                RedrawDialog(tradeDialog);
#endif
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
#ifdef MACBUILD
    RefreshSAT();
#endif
    shipData->targetStellar = savedStellar;
    shipData->navTargetKind = savedRoute;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}

/* TradeFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048d190
// MAC: _TradeFilter i386 0x0004e092 ppc 0x00056d08
Boolean TradeFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    Point mouse;
    Handle item;
    short j;
    short last;
    short i;
#if V111
    short key;
#else
    EventChar c;
#endif
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
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if ((int)ev->what == kKeyDown) {
            if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
                ev->what = kMouseDown;
                *itemHit = 1;
                return 1;
            }
        }
        if (key == kTabKeyCode) {
            if ((ev->modifiers & shiftKey) != 0)
                TradeDialogTabKey(1);
            else
                TradeDialogTabKey(0);
        }
        if (key == kDownArrowKeyCode)
            TradeDialogTabKey(0);
        if (key == kUpArrowKeyCode)
            TradeDialogTabKey(1);
        if (key == kBKeyCode) {
            *itemHit = kBuyItem;
            return 1;
        }
        if (key == kSKeyCode) {
            *itemHit = kSellItem;
            return 1;
        }
#else
        c = EventCharOf(ev);
        if ((int)ev->what == kKeyDown) {
            if (IsReturnChar(c) || c == kEscapeChar) {
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
        c = tolower(c);
        if (c == 'b') {
            *itemHit = kBuyItem;
            return 1;
        }
        if (c == 's') {
            *itemHit = kSellItem;
            return 1;
        }
#endif
    }
    if ((int)ev->what == kMouseDown) {
        SetDialogGWorld(tradeDialog);
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        for (i = kFirstRowItem; i <= kLastRowItem; i++) {
            GetDialogItem(tradeDialog, i, &itemType, &item, &box);
            if (PtInRectPoint(where, &box) &&
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
                        TradeDialogUpdate();
                    }
                    GetMouse(&mouse);
                    for (j = 0; j <= kLastRow; j++) {
                        GetDialogItem(tradeDialog, j + kFirstRowItem, &itemType,
                                      &item, &box);
                        if (PtInRectPoint(mouse, &box) &&
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
        TradeDialogUpdate();
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(tradeDialog));
#endif
        TradeDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(tradeDialog));
#endif
        return 0;
    }
    return 0;
}
#ifdef MACBUILD
#undef InvalDialogItem
#endif

#define kMapFilterCount 8


/* TradeDialogTabKey. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048d5a0
// MAC: _TradeDialogTabKey i386 0x0004cd81 ppc 0x00055cc0
void TradeDialogTabKey(Boolean back)
{
    Rect box;
    Rect bounds;
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
#ifdef MACBUILD
    GetDialogItem(tradeDialog, 13, &itemType, (Handle *)&item, &box);
    InvalWindowRect(GetDialogWindow(tradeDialog), &box);
    GetDialogItem(tradeDialog, 14, &itemType, (Handle *)&item, &box);
    InvalWindowRect(GetDialogWindow(tradeDialog), &box);
#else
    InvalDialogItem(tradeDialog, 13);
    InvalDialogItem(tradeDialog, 14);
#endif
    GetDialogItem(tradeDialog, 3, &itemType, (Handle *)&item, &box);
    bounds = box;
    GetDialogItem(tradeDialog, 12, &itemType, (Handle *)&item, &box);
    bounds.bottom = box.bottom;
    InvalDialogRect(tradeDialog, &bounds);
}

/* TradeDialogUpdate (mac _TradeDialogUpdate). STATE: MATCH, 1241/1241, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0048d6f0
// MAC: _TradeDialogUpdate i386 0x0004ceea ppc 0x00055e5c
void TradeDialogUpdate(void)
{
    extern char *strcpy(char *, const char *);   /* MSL, CALLED not inlined */

#ifdef MAC1010
    Rect box;
    Boolean seen[256];
    Handle item;
    short itemType;
    short available;
    short missionTons;
    short junkTons;
    short missionSlots;
    Boolean junkSeen[0x80];
    short junkSlots;
    short k;
    long row;
    Rect insetBox;
    unsigned char num[256];
    char message[255];
    char text[2048];
    short i;
    Boolean found;
    short j;
    short qty;
    short p;
    short fleetHolds;
    short holds;
    short totalCargo;
    short over;
#elif defined(MACBUILD)
    Rect box;
    char message[255];
    Boolean seen[256];
    Handle item;
    short itemType;
    short missionTons;
    short junkTons;
    short missionSlots;
    Boolean junkSeen[0x80];
    short junkSlots;
    short k;
    long row;
    Rect insetBox;
    unsigned char num[256];
    char text[2048];
    Boolean found;
    short i;
    short j;
    short qty;
    short p;
    short fleetHolds;
    short holds;
    short totalCargo;
    short available;
    short over;
#else
    unsigned char message[256];
    void *item;
    short itemType;
    short missionTons;
    short junkTons;
    short missionSlots;
    Boolean seen[256];
    short junkSlots;
    short k;
    long row;
    Rect insetBox;
    unsigned char num[256];
    Rect box;
    unsigned char text[2048];
    Boolean found;
    short i;
    short j;
    short qty;
    short p;
    short fleetHolds;
    short holds;
    short totalCargo;
    short available;
    short missionCargo;
#endif

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(tradeDialog));
    if (tradeBackgroundPic != NULL)
        DrawPicHandle(tradeBackgroundPic, GetDialogPortRect(tradeDialog));

    ForeColor(blackColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    GetDialogItem(tradeDialog, kHoldsItem, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
        RGBForeColor(&rgbDarkGrey);
        FrameRect(&box);
        RGBForeColor(&rgbLightGrey);
#ifdef CEFIX /* F12.42 */
        CE_SetPen(box.left + CE_Scale(7), box.top + CE_Scale(12));
#else
        MoveTo((box.left + 7), (box.top + 12));
#endif
        DrawIndString(kGameStrList, 0xc5);
#ifdef CEFIX /* F12.43 */
        CE_SetPen(box.right - CE_Scale(0x7b), box.top + CE_Scale(12));
#else
        MoveTo((box.right - 0x7b), (box.top + 12));
#endif
        if (TotalFleetHolds() > ShipTotalHolds())
            DrawIndString(kGameStrList, 0xc6);
        else
            DrawIndString(kGameStrList, 0xc7);
#ifdef CEFIX /* F12.44 */
        CE_SetPen(box.right - CE_Scale(0x46), box.top + CE_Scale(12));
#else
        MoveTo((box.right - 0x46), (box.top + 12));
#endif
        DrawIndString(kGameStrList, 0xc8);
        ForeColor(blackColor);
    }

    for (i = 0; i <= kLastRow; i++) {
#ifdef MAC1010
        GetDialogItem(tradeDialog, i + kFirstRowItem, &itemType, &item, &box);
#else
        row = i;
        GetDialogItem(tradeDialog, row + kFirstRowItem, &itemType, (Handle *)&item, &box);
#endif
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
            insetBox = box;
            InsetRect(&insetBox, 1, 1);
            if (i == selTradeItem)
                RGBForeColor((RGBColor *)&ldefColors[2]);
            else
                RGBForeColor((RGBColor *)&ldefColors[1]);
            PaintRect(&insetBox);
            ForeColor(blackColor);

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

#ifdef MAC1010
            row = i;
            if (tradePrice[row] != 0) {
#else
            if (tradePrice[i] != 0) {
#endif
                RGBForeColor((RGBColor *)&ldefColors[0]);
                if (i <= kLastCommodity) {
#ifdef CEFIX /* F12.49 */
                    CE_SetPen(box.left + CE_Scale(7), box.top + CE_Scale(10));
#else
                    MoveTo((box.left + 7), (box.top + 10));
#endif
#ifdef MAC1010
                    DrawString(cargoName[row]);
                    qty = shipData->cargo[row];
#else
                    DrawString(cargoName[i]);
                    qty = shipData->cargo[i];
#endif
                    if (qty > 0) {
                        NumToString(qty, (StringPtr)text);
#ifdef CEFIX /* F12.50 */
                        CE_SetPen(box.right - (StringWidth((StringPtr)text) + CE_Scale(0x6e)),
                                  box.top + CE_Scale(10));
#else
                        MoveTo((box.right -
                                       (StringWidth((StringPtr)text) + 0x6e)),
                               (box.top + 10));
#endif
                        DrawString((StringPtr)text);
                    }
#ifdef MAC1010
                } else if (junkType[i - kFirstJunkItem] != -1) {
#else
                } else if (junkType[(int)row - kFirstJunkItem] != -1) {
                    int junkIdx = row - kFirstJunkItem;
#endif
#ifdef CEFIX /* F12.45 */
                    CE_SetPen(box.left + CE_Scale(6), box.top + CE_Scale(10));
#else
                    MoveTo((box.left + 6), (box.top + 10));
#endif
#ifdef MAC1010
                    DrawString(junkData[junkType[i - kFirstJunkItem]].name);
                    qty = junkData[junkType[i - kFirstJunkItem]].held;
#else
                    DrawString(junkData[junkType[junkIdx]].name);
                    qty = junkData[junkType[junkIdx]].held;
#endif
                    if (qty > 0) {
                        NumToString(qty, (StringPtr)text);
#ifdef CEFIX /* F12.46 */
                        CE_SetPen(box.right - (StringWidth((StringPtr)text) + CE_Scale(0x6e)),
                                  box.top + CE_Scale(10));
#else
                        MoveTo((box.right -
                                       (StringWidth((StringPtr)text) + 0x6e)),
                               (box.top + 10));
#endif
                        DrawString((StringPtr)text);
                    }
                }

#ifdef CEFIX /* F12.47 */
                CE_SetPen(box.right - CE_Scale(0x46), box.top + CE_Scale(10));
#else
                MoveTo((box.right - 0x46), (box.top + 10));
#endif
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
#ifdef MAC1010
                        if (portDemand[row] == kPortShortage)
                            DrawIndString(kGameStrList, 0xc9);
                        if (portDemand[row] == kPortSurplus)
                            DrawIndString(kGameStrList, 0xca);
                        if (portDemand[row] == kPortEmbargo)
                            DrawIndString(kGameStrList, 0xcb);
                    } else if (junkType[i - kFirstJunkItem] != -1) {
#else
                        if (portDemand[i] == kPortShortage)
                            DrawIndString(kGameStrList, 0xc9);
                        if (portDemand[i] == kPortSurplus)
                            DrawIndString(kGameStrList, 0xca);
                        if (portDemand[i] == kPortEmbargo)
                            DrawIndString(kGameStrList, 0xcb);
                    } else if (junkType[(int)row - kFirstJunkItem] != -1) {
#endif
                        if (i == kFirstJunkItem)
                            DrawIndString(kGameStrList, 0xcb);
                        if (i == kLastRow)
                            DrawIndString(kGameStrList, 0xc9);
                    }
                }

#ifdef MAC1010
                NumToString(tradePrice[row], (StringPtr)text);
#else
                NumToString(tradePrice[i], text);
#endif
#ifdef CEFIX /* F12.48 */
                CE_SetPen(box.right - (StringWidth((StringPtr)text) + CE_Scale(10)),
                          box.top + CE_Scale(10));
#else
                MoveTo((box.right - (StringWidth((StringPtr)text) + 10)),
                       (box.top + 10));
#endif
                DrawString((StringPtr)text);
            }
            ForeColor(blackColor);
        }
    }

    GetDialogItem(tradeDialog, kCargoItem, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
        ForeColor(blackColor);
        insetBox = box;
        InsetRect(&insetBox, 5, 3);
        strcpy((char *)text, "");

#ifdef MACBUILD
        available = 0;
        k = 0;
        missionTons = 0;
        junkTons = 0;
        missionSlots = 0;
        junkSlots = 0;

        for (i = 0; i < 0x100; i++)
            seen[i] = 0;
#ifdef MAC1010 /* 1.0.10 spelling */
        for (i = 0; i < 0x80; i++)
#else
        for (i = 0; i < sizeof(junkSeen); i++)
#endif
            junkSeen[i] = 0;
        for (i = 0; i <= 1; i++)
            if (junkType[i] != -1)
                junkSeen[junkType[i]] = 1;
        for (j = 0; j < 16; j++) {
            if (currentMissionObjectives[j].active && currentMissions[j].hasCargo &&
                currentMissions[j].cargoType != -1 && currentMissions[j].cargoQty >= 0) {
                available++;
                missionSlots++;
                missionTons += currentMissions[j].cargoQty;
                if (seen[currentMissions[j].cargoType] == 0) {
                    seen[currentMissions[j].cargoType] = 1;
                    k++;
                }
            }
        }
        for (p = 0; p < 0x80; p++) {
            if (junkData[p].held > 0 && p != junkType[0] && p != junkType[1]) {
                available++;
                junkTons += junkData[p].held;
                junkSlots++;
                k++;
            }
        }
        if (available > 0) {
#else
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
#endif
            if (missionSlots + junkSlots > 0) {
                StrcatIndCString((char *)text, kGameStrList, 0x16b);
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
                    StrcatIndCString((char *)text, kGameStrList, 0x187);
                    strcat((char *)text, " ");
                }
                if (missionSlots > 0) {
                    StrcatIndCString((char *)text, kGameStrList, 0x16f);
                    if (junkSlots > 0) {
                        strcat((char *)text, " ");
                        StrcatIndCString((char *)text, kGameStrList, 0x188);
                        strcat((char *)text, " ");
                    }
                }
                if (junkSlots > 0)
                    StrcatIndCString((char *)text, kGameStrList, 0x170);
                strcat((char *)text, "\r\r");
            }
        }

        holds = ShipTotalHolds();
        fleetHolds = TotalFleetHolds();
        totalCargo = TotalCargo(shipData);
        StrcatIndCString((char *)text, kGameStrList, 0x16c);
        if (fleetHolds > holds) {
            strcat((char *)text, " ");
            StrcatIndCString((char *)text, kGameStrList, 0x16d);
        }
        strcat((char *)text, ": ");
        if (fleetHolds > holds) {
#ifdef MACBUILD
            over = totalCargo - missionTons;
            over -= fleetHolds - holds;
            if (over < 0)
                over = 0;
            available = holds - (over + missionTons);
#else
            available = totalCargo - missionTons;
            available -= fleetHolds - holds;
            if (available < 0)
                available = 0;
            available = holds - (available + missionTons);
#endif
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
            available = fleetHolds - totalCargo;
            if (available < 0)
                available = 0;
            strcat((char *)text, "\r");
            StrcatIndCString((char *)text, kGameStrList, 0x16c);
            strcat((char *)text, " ");
            StrcatIndCString((char *)text, kGameStrList, 0x16e);
            strcat((char *)text, ": ");
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
        }

        TETextBox(text, strlen((char *)text), &insetBox, 0);
        InvertRect(&insetBox);
        RGBForeColor(&rgbDarkGrey);
        FrameRect(&box);
        ForeColor(blackColor);
    }
    DrawTradeButtons(-1);

    GetDialogItem(tradeDialog, kNewsItem, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
        for (p = 0; p < 0x100; p++) {
            if (disasterData[p].loaded != 0 && disasterData[p].countdown > 0 &&
                disasterData[p].stellarID == shipData->targetStellar) {
                if (cheatsActive) {
                    strcpy((char *)text, "Disaster ID ");
                    NumToString(p + 0x80, num);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                } else {
                    strncpy((char *)text, (char *)disasterData[p].name, 0x3f);
                    PtoCstr((StringPtr)text);
                }
                strcat((char *)text, " ");
                StrcatIndCString((char *)text, kGameStrList, 0xc0);
                strcat((char *)text, " ");
                if (disasterData[p].amount > 0)
                    StrcatIndCString((char *)text, kGameStrList, 0xc1);
                else
                    StrcatIndCString((char *)text, kGameStrList, 0xc2);
                strcat((char *)text, " ");
                StrcatIndCString((char *)text, kGameStrList, 0xb5);
                strcat((char *)text, " ");
                strncpy((char *)num, (char *)lcCargoName[disasterData[p].commodity], 0x40);
                PtoCstr(num);
                strcat((char *)text, (char *)num);
                strcat((char *)text, ".");

                strcpy((char *)message, (char *)text);
                ForeColor(blackColor);
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
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(tradeDialog)),
             GetDialogPortRect(tradeDialog), GetDialogPortRect(tradeDialog), 0,
             GetWindowVisRgn(FrontWindow(tradeDialog)));
}

#define kStellarBackdropPictID 0x2134

/* PreloadPortDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048e970
// MAC: _PreloadPortDialog i386 0x0004e5c7 ppc 0x000571c0
void PreloadPortDialog(SpobRecord *spob, short stellar)
{
    Rect rect;
    PicHandle pic;

    portBackgroundPic = GetPicture(kStellarBackdropPictID);
    planetPicContainer.world = NULL;

    if (spob->customPict >= 128)
        planetPicId = spob->customPict;
    else
        planetPicId = spob->type + 10000;

    pic = GetPicture(planetPicId);
    if (pic != NULL) {
        QDGetPictureBounds(pic, &rect);
        OffsetRect(&rect, -rect.left, -rect.top);
#ifdef MACBUILD
        CreateGWorldContainer(&planetPicContainer, GetMainDevice(), gSpriteWorld->pixelDepth, &rect);
#else
        CreateGWorldContainer(&planetPicContainer, 0, gSpriteWorld->pixelDepth, &rect);
#endif
        if (planetPicContainer.world != NULL) {
            SetGWorldContainer(&planetPicContainer);
            ForeColor(blackColor);
            PaintRect(&planetPicContainer.bounds);
            DrawPicHandle(pic, &planetPicContainer.bounds);
            QDReleaseResource(pic);
            UnsetGWorldContainer(&planetPicContainer);
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
// MAC: _DoOutfitDialog i386 0x0005bacf ppc 0x00063b20
#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &box), InvalWindowRect(GetDialogWindow(dlg), &box))
#endif
void DoOutfitDialog(void)
#ifdef MAC1010
{
    short i;
    short j;
    short count;
    short shortfall;
    short ammo;
    short weapID;
    Boolean shown;
    short itemHit;
    long cost;
    short itemType;
    short savedUnk6C;
    short savedUnk6A;
    long qtyLimit;
    int have;
    Boolean done;
    short filterKey;
    Handle item;
    short maxAfford;
    short savedOutfits[kMaxOutfits];
    Rect box;
    unsigned char msg[256];
    unsigned char nameBuf[256];
    short m;
    short k;

    done = 0;
    for (i = 0; i < kMaxOutfits; i++)
        savedOutfits[i] = shipSystems[i];
    savedUnk6C = shipData->targetStellar;
    savedUnk6A = shipData->navTargetKind;
    selOutfitItem = -1;
    selOutfitListItem = -1;
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
    m = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (dockedPortItems[i] != -1) {
            m++;
            break;
        }
    }
    if (m == 0) {
        extern char *strcpy(char *, const char *);

        strcpy(textShowString, "");
        StrcatIndCString(textShowString, kGameStrList, 0xce);
        ShowTextDialog(0, 0, 0);
        return;
    }
    previewPic = NULL;
    previewPicID = -1;
    missionsOutside = kOutfitVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    outfitDialog = NULL;
    outfitDialog = GetNewDialog(kOutfitDlogID, OutfitDialogUpdate, OutfitFilter);
    if (outfitDialog == NULL)
        return;
    outfitBackgroundPic = GetPicture(kOutfitPictID);
    CenterWindow(FrontWindow(outfitDialog), FrontWindow(portDialog));
    GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
    SetupListRects((Point *)&box);
    strncpy(dialogDescTextPtr, "", 3);
    ShowWindowNoFlash(FrontWindow(outfitDialog));
    SetDialogGWorld(outfitDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    OutfitDialogUpdate();
    SelectWindow(FrontWindow(outfitDialog));
    BringToFront(FrontWindow(outfitDialog));
    OfferMissionFromPort(kOutfitVenue);
    RedrawDialog(outfitDialog);

    do {
        if (gQuit)
            done = 1;
        MyModalDialog(outfitDialog, OutfitFilter, &itemHit, &filterKey);
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
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0
                    && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
                    count++;
            }
            if (count > 0) {
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
            if (selOutfitItem != -1)
                canBuy = CanBuyOutfitItem(selOutfitItem);
            else
                canBuy = 0;
            if (canBuy && selOutfitItem != -1) {
                count = 1;
                if ((filterKey & optionKey) != 0) {
                    cost = ApplyPriceAndTechnologyFlux(
                        outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                        outfitSystem[selOutfitItem].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
                    cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    if (cost > 0) {
                        qtyLimit = shipData->credits / cost;
                        if (qtyLimit > 32000)
                            qtyLimit = 32000;
                    } else
                        qtyLimit = 32000;
                    if (outfitSystem[selOutfitItem].maxCount
                        > shipSystems[selOutfitItem]) {
                        if (qtyLimit > outfitSystem[selOutfitItem].maxCount
                                       - shipSystems[selOutfitItem])
                            qtyLimit = outfitSystem[selOutfitItem].maxCount
                                       - shipSystems[selOutfitItem];
                    }
                    if (qtyLimit > 1) {
                        HasMaxOfItem(selOutfitItem, 0, &maxAfford);
                        if (maxAfford < qtyLimit)
                            qtyLimit = maxAfford;
                        if (maxAfford - shipSystems[selOutfitItem] < qtyLimit)
                            qtyLimit = maxAfford - shipSystems[selOutfitItem];
                    }
                    if (qtyLimit > 1) {
                        count = DoQuantityDialog(qtyLimit, 0);
                        OutfitDialogUpdate();
                    }
                }
                for (i = 0; i < count; i++) {
                    if (canBuy) {
                        cost = ApplyPriceAndTechnologyFlux(
                            outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                            outfitSystem[selOutfitItem].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
                        cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                                &shipClass[shipData->type]);
                        GrantOutfitItem(selOutfitItem);
                        shipData->credits -= cost;
                        crashlog_mission_bit_set(
                            "purchasing outfit",
                            (char *)outfitSystem[selOutfitItem].name, 1,
                            selOutfitItem + kFirstResourceID,
                            outfitSystem[selOutfitItem].onPurchase);
                        if (shipData->credits < 0) {
                            crashlog("warning: player's cash went below zero");
                            shipData->credits = 0;
                            i = count;
                        }
                        EvalMissionBitSetString(outfitSystem[selOutfitItem].onPurchase);
                    }
                    if (selOutfitItem != -1)
                        canBuy = CanBuyOutfitItem(selOutfitItem);
                    else
                        canBuy = 0;
                }
                SystemInfoToShipStats();
                if (SetupPortAvailableItems(dockedPort)) {
                    selOutfitItem = 0;
                    selOutfitListItem = -1;
                    outfitScroll = 0;
                    canBuy = canSell = 0;
                    strncpy(dialogDescTextPtr, "", 3);
                    InvalDialogItem(outfitDialog, 6);
                    InvalDialogItem(outfitDialog, 8);
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
                InvalDialogItem(outfitDialog, 4);
                InvalDialogItem(outfitDialog, 5);
                InvalDialogItem(outfitDialog, 7);
                InvalDialogItem(outfitDialog, 9);
                didOutfit = 1;
                if (shipSystems[selOutfitItem] > 0) {
                    if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
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
            if ((filterKey & optionKey) != 0) {
                qtyLimit = shipSystems[selOutfitItem];
                if (qtyLimit > 32000)
                    qtyLimit = 32000;
                if (qtyLimit > 1) {
                    count = DoQuantityDialog(qtyLimit, 1);
                    OutfitDialogUpdate();
                }
            }
            shown = 0;
            for (i = 0; i < count; i++) {
                if (canSell) {
                    have = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    if (have < 0 && ShipFreeMass() + have < 0) {
                        if (!shown) {
                            GetIndString(msg, kGameStrList, 0xcf);
                            PtoCstr(msg);
                            strcpy(textShowString, (char *)msg);
                            ShowTextDialog(0, 0, 0);
                            shown = 1;
                        }
                        canSell = 0;
                    }
                }
                if (canSell) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[selOutfitItem].modType[j] == 0x1b
                            && outfitSystem[selOutfitItem].modVal[j] >= kFirstResourceID
                            && outfitSystem[selOutfitItem].modVal[j] < kFirstResourceID + kMaxOutfits) {
                            k = (shipSystems[selOutfitItem] - count)
                                * outfitSystem[outfitSystem[selOutfitItem].modVal[j] - kFirstResourceID].maxCount;
                            shortfall = shipSystems[outfitSystem[selOutfitItem].modVal[j] - kFirstResourceID] - k;
                            if (shortfall > 0) {
                                if (!shown) {
                                    strcpy((char *)msg,"");

                                    StrcatIndCString((char *)msg,kGameStrList, 0xd0);
                                    strcat((char *)msg," ");
                                    if (shortfall > 3)
                                        StrcatCommaNumber(shortfall, (char *)msg);
                                    else
                                        StrcatOrdinalNumber(shortfall, (char *)msg,0);
                                    strcat((char *)msg," ");
                                    if (shortfall > 1)
                                        pstrcpy(nameBuf,
                                            lcPluralOutfitName[outfitSystem[selOutfitItem].modVal[j] - kFirstResourceID]);
                                    else
                                        pstrcpy(nameBuf,
                                            lcOutfitName[outfitSystem[selOutfitItem].modVal[j] - kFirstResourceID]);
                                    PtoCstr(nameBuf);
                                    strcat((char *)msg,(char *)nameBuf);
                                    strcat((char *)msg," ");
                                    StrcatIndCString((char *)msg,kGameStrList, 0xd4);
                                    strcat((char *)msg," ");
                                    if (i < count - 1)
                                        pstrcpy(nameBuf, lcPluralOutfitName[selOutfitItem]);
                                    else
                                        pstrcpy(nameBuf, lcOutfitName[selOutfitItem]);
                                    PtoCstr(nameBuf);
                                    strcat((char *)msg,(char *)nameBuf);
                                    strcat((char *)msg,".");
                                    strcpy(textShowString, (char *)msg);
                                    ShowTextDialog(0, 0, 0);
                                    shown = 1;
                                }
                                canSell = 0;
                            }
                        }
                    }
                    weapID = -1;
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[selOutfitItem].modType[j] == 1) {
                            weapID = outfitSystem[selOutfitItem].modVal[j];
                            break;
                        }
                    }
                    if (weapID != -1 && weaponStats[weapID].maxAmmo > 0) {
                        if (weaponStats[weapID].ammoType >= 0
                            && weaponStats[weapID].ammoType < 256
                            && weaponStats[weapID].guidance != kGuidanceBay)
                            ammo = shipData->weap[weaponStats[weapID].ammoType].ammo;
                        else
                            ammo = shipData->weap[weapID].ammo;
                        if (weaponStats[weapID].guidance == kGuidanceBay) {
                            for (j = 1; j < kMaxShips; j++) {
                                if (shipData[j].active != 0
                                    && shipData[j].parentShip == 0
                                    && shipData[j].aiType == 5
                                    && shipData[j].type
                                       == weaponStats[weapID].ammoType - kFirstResourceID
                                    && !IsDisabled(&shipData[j]))
                                    ammo++;
                            }
                        }
                        shortfall = ammo
                            - ((shipSystems[selOutfitItem] - (i + 1))
                                      * weaponStats[weapID].maxAmmo);
                        if (shortfall > 0 && ammo > 0) {
                            if (!shown) {
                                strcpy((char *)msg,"");

                                StrcatIndCString((char *)msg,kGameStrList, 0xd0);
                                strcat((char *)msg," ");
                                if (shortfall > 3)
                                    StrcatCommaNumber(shortfall, (char *)msg);
                                else
                                    StrcatOrdinalNumber(shortfall, (char *)msg,0);
                                strcat((char *)msg," ");
                                k = -1;
                                for (j = 0; j < kMaxOutfits; j++) {
                                    for (m = 0; m < kOutfMods; m++) {
                                        if (outfitSystem[j].modType[m] == 3
                                            && outfitSystem[j].modVal[m] == weapID) {
                                            k = j;
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
                                    strcat((char *)msg,(char *)nameBuf);
                                } else {
                                    if (shortfall > 1)
                                        StrcatIndCString((char *)msg,kGameStrList, 0xd2);
                                    else
                                        StrcatIndCString((char *)msg,kGameStrList, 0xd1);
                                    strcat((char *)msg," ");
                                    StrcatIndCString((char *)msg,kGameStrList, 0xd3);
                                }
                                strcat((char *)msg," ");
                                StrcatIndCString((char *)msg,kGameStrList, 0xd4);
                                strcat((char *)msg," ");
                                if (i < count - 1)
                                    pstrcpy(nameBuf, lcPluralOutfitName[selOutfitItem]);
                                else
                                    pstrcpy(nameBuf, lcOutfitName[selOutfitItem]);
                                PtoCstr(nameBuf);
                                strcat((char *)msg,(char *)nameBuf);
                                strcat((char *)msg,".");
                                strcpy(textShowString, (char *)msg);
                                ShowTextDialog(0, 0, 0);
                                shown = 1;
                            }
                            canSell = 0;
                        }
                    }
                }
                if (canSell) {
                    cost = ApplyPriceAndTechnologyFlux(
                        outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                        outfitSystem[selOutfitItem].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
                    cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    if (shipSystems[selOutfitItem]
                        <= savedOutfits[selOutfitItem])
                        cost *= 0.5;
                    have = ShipFreeMass();
                    shipSystems[selOutfitItem] -= 1;
                    if (ShipFreeMass() < 0 && have >= 0) {
                        shipSystems[selOutfitItem] += 1;
                        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                        break;
                    } else {
                        shipData->credits += cost;
                        for (j = 0; j < kOutfMods; j++) {
                            if (shipSystems[selOutfitItem] <= 0) {
                                if (outfitSystem[selOutfitItem].modType[j] == 1
                                    && shipData->selectedWeapon
                                       == outfitSystem[selOutfitItem].modVal[j]) {
                                    shipData->selectedWeapon = -1;
                                    statusRedraw[3] = 1;
                                    break;
                                }
                            }
                        }
                        crashlog_mission_bit_set(
                            "selling outfit",
                            (char *)outfitSystem[selOutfitItem].name, 1,
                            selOutfitItem + kFirstResourceID,
                            outfitSystem[selOutfitItem].onSell);
                        EvalMissionBitSetString(outfitSystem[selOutfitItem].onSell);
                        SystemInfoToShipStats();
                        if (SetupPortAvailableItems(dockedPort)) {
                            selOutfitItem = 0;
                            selOutfitListItem = -1;
                            outfitScroll = 0;
                            canBuy = canSell = 0;
                            strncpy(dialogDescTextPtr, "", 3);
                            InvalDialogItem(outfitDialog, 6);
                            InvalDialogItem(outfitDialog, 8);
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
                    }
                }
                if (selOutfitItem != -1 && shipSystems[selOutfitItem] > 0) {
                    if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
                        canSell = 0;
                    else
                        canSell = 1;
                } else
                    canSell = 0;
            }
            if (selOutfitItem != -1)
                canBuy = CanBuyOutfitItem(selOutfitItem);
            else
                canBuy = 0;
            InvalDialogItem(outfitDialog, 4);
            InvalDialogItem(outfitDialog, 5);
            InvalDialogItem(outfitDialog, 7);
            InvalDialogItem(outfitDialog, 9);
            didOutfit = 1;
            statusRedraw[2] = 1;
            DrawStatus();
            SetDialogGWorld(outfitDialog);
            DrawOutfitButtons(-1);
        }
        if (itemHit == 10 && outfitScroll >= kGridCols) {
            outfitScroll -= kGridCols;
            if (selOutfitListItem >= 0 && selOutfitListItem < kGridCells - kGridCols) {
                selOutfitListItem += kGridCols;
                selOutfitItem =
                    dockedPortItems[outfitScroll + selOutfitListItem];
                LoadDescription(dialogDescTextPtr, selOutfitItem + kOutfitDescBase);
            } else {
                selOutfitItem = selOutfitListItem = -1;
                strncpy(dialogDescTextPtr, "", 3);
            }
            SetDialogGWorld(outfitDialog);
            InvalDialogItem(outfitDialog, 4);
            InvalDialogItem(outfitDialog, 7);
            InvalDialogItem(outfitDialog, 5);
            InvalDialogItem(outfitDialog, 6);
            InvalDialogItem(outfitDialog, 8);
            InvalDialogItem(outfitDialog, 9);
            InvalDialogItem(outfitDialog, 10);
            InvalDialogItem(outfitDialog, 11);
            InvalDialogItem(outfitDialog, 13);
        }
        if (itemHit == 11) {
            count = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (dockedPortItems[i] != -1)
                    count++;
            }
            if (outfitScroll < count - kGridCells) {
                outfitScroll += kGridCols;
                if (selOutfitListItem >= kGridCols) {
                    selOutfitListItem -= kGridCols;
                    selOutfitItem =
                        dockedPortItems[outfitScroll + selOutfitListItem];
                    LoadDescription(dialogDescTextPtr, selOutfitItem + kOutfitDescBase);
                } else {
                    selOutfitItem = selOutfitListItem = -1;
                    strncpy(dialogDescTextPtr, "", 3);
                }
                SetDialogGWorld(outfitDialog);
                InvalDialogItem(outfitDialog, 4);
                InvalDialogItem(outfitDialog, 7);
                InvalDialogItem(outfitDialog, 5);
                InvalDialogItem(outfitDialog, 6);
                InvalDialogItem(outfitDialog, 8);
                InvalDialogItem(outfitDialog, 9);
                InvalDialogItem(outfitDialog, 10);
                InvalDialogItem(outfitDialog, 11);
                InvalDialogItem(outfitDialog, 13);
            }
        }
    } while (!done);

    k = ShipFuelCapacity(shipData);
    if (shipData->fuel > k) {
        shipData->fuel = k;
        statusRedraw[4] = 1;
    }
    count = ShipShieldCapacity(shipData);
    if (shipData->shield > count) {
        shipData->shield = count;
        statusRedraw[1] = 1;
    }
    count = ShipArmorCapacity(shipData);
    if (shipData->armor > count) {
        shipData->armor = count;
        statusRedraw[1] = 1;
    }
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0 && (outfitSystem[i].flags & kOutfRemoveAfterBuy) != 0)
            shipSystems[i] = 0;
    }
    SystemInfoToShipStats();
    DisposeDialogNoFlash(outfitDialog);
    outfitDialog = NULL;
    if (outfitBackgroundPic != NULL) {
        QDReleaseResource(outfitBackgroundPic);
        outfitBackgroundPic = NULL;
    }
    if (previewPic != NULL) {
        HUnlock((Handle)previewPic);
        QDReleaseResource(previewPic);
        previewPic = NULL;
    }
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
#if !TARGET_API_MAC_CARBON
    RedrawStatus(0);
#endif
    SWSetPortToWindow(gSpriteWorld);
    RefreshSAT();
    shipData->targetStellar = savedUnk6C;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}
#else
{
#ifdef MACBUILD
    short shortfall;
    short itemHit;
    long cost;
    short itemType;
    short savedUnk6C;
    short savedUnk6A;
    long qtyLimit;
    int have;
    short count;
    short weapID;
    Boolean shown;
    Boolean done;
    short filterKey;
    Handle item;
    short maxAfford;
    short savedOutfits[kMaxOutfits];
    Rect box;
    unsigned char nameBuf[256];
    unsigned char msg[256];
    short i;
    short j;
    short k;
    short m;
    short ammo;
    short n;
#else
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
    Boolean done;
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
    Boolean shown;
    long cost;
#endif

    done = 0;
    for (i = 0; i < kMaxOutfits; i++)
        savedOutfits[i] = shipSystems[i];
    savedUnk6C = shipData->targetStellar;
    savedUnk6A = shipData->navTargetKind;
    selOutfitItem = -1;
    selOutfitListItem = -1;
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
#ifdef MACBUILD
    m = 0;
    for (count = 0; count < kMaxOutfits; count++) {
        if (dockedPortItems[count] != -1) {
            m++;
            break;
        }
    }
    if (m == 0) {
#else
    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (dockedPortItems[i] != -1) {
            count++;
            break;
        }
    }
    if (count == 0) {
#endif
        extern char *strcpy(char *, const char *);

        strcpy(textShowString, "");
        StrcatIndCString(textShowString, kGameStrList, 0xce);
        ShowTextDialog(0, 0, 0);
        return;
    }
    previewPic = NULL;
    previewPicID = -1;
    missionsOutside = kOutfitVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    outfitDialog = NULL;
    outfitDialog = GetNewDialog(kOutfitDlogID, OutfitDialogUpdate, OutfitFilter);
    if (outfitDialog == NULL)
        return;
    outfitBackgroundPic = GetPicture(kOutfitPictID);
    CenterWindow(FrontWindow(outfitDialog), FrontWindow(portDialog));
    GetDialogItem(outfitDialog, kGridItem, &itemType, (Handle *)&item, &box);
    SetupListRects((Point *)&box);
    strncpy(dialogDescTextPtr, "", 3);
    ShowWindowNoFlash(FrontWindow(outfitDialog));
    SetDialogGWorld(outfitDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    OutfitDialogUpdate();
    SelectWindow(FrontWindow(outfitDialog));
    BringToFront(FrontWindow(outfitDialog));
    OfferMissionFromPort(kOutfitVenue);
    RedrawDialog(outfitDialog);

    do {
        if (gQuit)
            done = 1;
        MyModalDialog(outfitDialog, OutfitFilter, &itemHit, &filterKey);
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
            if (selOutfitItem != -1)
                canBuy = CanBuyOutfitItem(selOutfitItem);
            else
                canBuy = 0;
            if (canBuy && selOutfitItem != -1) {
                count = 1;
                if ((filterKey & optionKey) != 0) {
#ifndef V111
                    cost = ApplyPriceAndTechnologyFlux(
                        outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                        outfitSystem[selOutfitItem].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
#endif
                    cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    if (cost <= 0)
                        qtyLimit = 32000;
                    else {
                        qtyLimit = shipData->credits / cost;
                        if (qtyLimit > 32000)
                            qtyLimit = 32000;
                    }
                    if (outfitSystem[selOutfitItem].maxCount
                        > shipSystems[selOutfitItem]) {
                        if (qtyLimit > outfitSystem[selOutfitItem].maxCount
                                       - shipSystems[selOutfitItem])
                            qtyLimit = outfitSystem[selOutfitItem].maxCount
                                       - shipSystems[selOutfitItem];
                    }
                    if (qtyLimit > 1) {
                        HasMaxOfItem(selOutfitItem, 0, &maxAfford);
                        if (maxAfford < qtyLimit)
                            qtyLimit = maxAfford;
                        if (maxAfford - shipSystems[selOutfitItem] < qtyLimit)
                            qtyLimit = maxAfford - shipSystems[selOutfitItem];
                    }
                    if (qtyLimit > 1) {
                        count = DoQuantityDialog(qtyLimit, 0);
                        OutfitDialogUpdate();
                    }
                }
                for (m = 0; m < count; m++) {
                    if (canBuy) {
#ifndef V111
                        cost = ApplyPriceAndTechnologyFlux(
                            outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                            outfitSystem[selOutfitItem].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
#endif
                        cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                                &shipClass[shipData->type]);
                        GrantOutfitItem(selOutfitItem);
                        shipData->credits -= cost;
                        crashlog_mission_bit_set(
                            "purchasing outfit",
                            (char *)outfitSystem[selOutfitItem].name, 1,
                            selOutfitItem + kFirstResourceID,
                            outfitSystem[selOutfitItem].onPurchase);
                        if (shipData->credits < 0) {
                            crashlog("warning: player's cash went below zero");
                            shipData->credits = 0;
                            m = count;
                        }
                        EvalMissionBitSetString(outfitSystem[selOutfitItem].onPurchase);
                    }
                    if (selOutfitItem != -1)
                        canBuy = CanBuyOutfitItem(selOutfitItem);
                    else
                        canBuy = 0;
                }
                SystemInfoToShipStats();
                if (SetupPortAvailableItems(dockedPort)) {
                    selOutfitItem = 0;
                    selOutfitListItem = -1;
                    outfitScroll = 0;
                    canBuy = canSell = 0;
                    strncpy(dialogDescTextPtr, "", 3);
#ifdef MACBUILD
                    InvalDialogItem(outfitDialog, 6);
                    InvalDialogItem(outfitDialog, 8);
#else
                    RedrawDialog(outfitDialog);
#endif
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
#ifdef MACBUILD
                InvalDialogItem(outfitDialog, 4);
                InvalDialogItem(outfitDialog, 5);
                InvalDialogItem(outfitDialog, 7);
                InvalDialogItem(outfitDialog, 9);
#else
                RedrawDialog(outfitDialog);
#endif
                didOutfit = 1;
                if (shipSystems[selOutfitItem] > 0) {
                    if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
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
            n = 1;
            if ((filterKey & optionKey) != 0) {
                qtyLimit = shipSystems[selOutfitItem];
                if (qtyLimit > 32000)
                    qtyLimit = 32000;
                if (qtyLimit > 1) {
                    n = DoQuantityDialog(qtyLimit, 1);
                    OutfitDialogUpdate();
                }
            }
            shown = 0;
            for (count = 0; count < n; count++) {
                if (canSell) {
                    have = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    if (have < 0 && ShipFreeMass() + have < 0) {
                        if (!shown) {
                            GetIndString((unsigned char *)msg, kGameStrList, 0xcf);
                            PtoCstr((unsigned char *)msg);
                            strcpy(textShowString, (char *)msg);
                            ShowTextDialog(0, 0, 0);
                            shown = 1;
                        }
                        canSell = 0;
                    }
                }
                if (canSell) {
                    for (i = 0; i < kOutfMods; i++) {
                        if (outfitSystem[selOutfitItem].modType[i] == 0x1b
                            && outfitSystem[selOutfitItem].modVal[i] >= kFirstResourceID
                            && outfitSystem[selOutfitItem].modVal[i] < kFirstResourceID + kMaxOutfits) {
                            shortfall = shipSystems[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID]
                                - ((shipSystems[selOutfitItem] - n)
                                          * outfitSystem[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID].maxCount);
                            if (shortfall > 0) {
                                if (!shown) {
                                    strcpy((char *)msg,"");

                                    StrcatIndCString((char *)msg,kGameStrList, 0xd0);
                                    strcat((char *)msg," ");
                                    if (shortfall > 3)
                                        StrcatCommaNumber(shortfall, (char *)msg);
                                    else
                                        StrcatOrdinalNumber(shortfall, (char *)msg,0);
                                    strcat((char *)msg," ");
                                    if (shortfall > 1)
                                        pstrcpy(nameBuf,
                                            lcPluralOutfitName[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID]);
                                    else
                                        pstrcpy(nameBuf,
                                            lcOutfitName[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID]);
                                    PtoCstr(nameBuf);
                                    strcat((char *)msg,(char *)nameBuf);
                                    strcat((char *)msg," ");
                                    StrcatIndCString((char *)msg,kGameStrList, 0xd4);
                                    strcat((char *)msg," ");
                                    if (count < n - 1)
                                        pstrcpy(nameBuf, lcPluralOutfitName[selOutfitItem]);
                                    else
                                        pstrcpy(nameBuf, lcOutfitName[selOutfitItem]);
                                    PtoCstr(nameBuf);
                                    strcat((char *)msg,(char *)nameBuf);
                                    strcat((char *)msg,".");
                                    strcpy(textShowString, (char *)msg);
                                    ShowTextDialog(0, 0, 0);
                                    shown = 1;
                                }
                                canSell = 0;
                            }
                        }
                    }
                    weapID = -1;
                    for (i = 0; i < kOutfMods; i++) {
                        if (outfitSystem[selOutfitItem].modType[i] == 1) {
                            weapID = outfitSystem[selOutfitItem].modVal[i];
                            break;
                        }
                    }
                    if (weapID != -1 && weaponStats[weapID].maxAmmo > 0) {
                        if (weaponStats[weapID].ammoType >= 0
                            && weaponStats[weapID].ammoType < 256
                            && weaponStats[weapID].guidance != kGuidanceBay)
                            ammo = shipData->weap[weaponStats[weapID].ammoType].ammo;
                        else
                            ammo = shipData->weap[weapID].ammo;
                        if (weaponStats[weapID].guidance == kGuidanceBay) {
                            for (m = 1; m < kMaxShips; m++) {
                                if (shipData[m].active != 0
                                    && shipData[m].parentShip == 0
                                    && shipData[m].aiType == 5
                                    && shipData[m].type
                                       == weaponStats[weapID].ammoType - kFirstResourceID
                                    && !IsDisabled(&shipData[m]))
                                    ammo++;
                            }
                        }
                        shortfall = ammo
                            - ((shipSystems[selOutfitItem] - (count + 1))
                                      * weaponStats[weapID].maxAmmo);
                        if (shortfall > 0 && ammo > 0) {
                            if (!shown) {
                                strcpy((char *)msg,"");

                                StrcatIndCString((char *)msg,kGameStrList, 0xd0);
                                strcat((char *)msg," ");
                                if (shortfall > 3)
                                    StrcatCommaNumber(shortfall, (char *)msg);
                                else
                                    StrcatOrdinalNumber(shortfall, (char *)msg,0);
                                strcat((char *)msg," ");
                                k = -1;
                                for (m = 0; m < kMaxOutfits; m++) {
                                    for (j = 0; j < kOutfMods; j++) {
                                        if (outfitSystem[m].modType[j] == 3
                                            && outfitSystem[m].modVal[j] == weapID) {
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
                                    strcat((char *)msg,(char *)nameBuf);
                                } else {
                                    if (shortfall > 1)
                                        StrcatIndCString((char *)msg,kGameStrList, 0xd2);
                                    else
                                        StrcatIndCString((char *)msg,kGameStrList, 0xd1);
                                    strcat((char *)msg," ");
                                    StrcatIndCString((char *)msg,kGameStrList, 0xd3);
                                }
                                strcat((char *)msg," ");
                                StrcatIndCString((char *)msg,kGameStrList, 0xd4);
                                strcat((char *)msg," ");
                                if (count < n - 1)
                                    pstrcpy(nameBuf, lcPluralOutfitName[selOutfitItem]);
                                else
                                    pstrcpy(nameBuf, lcOutfitName[selOutfitItem]);
                                PtoCstr(nameBuf);
                                strcat((char *)msg,(char *)nameBuf);
                                strcat((char *)msg,".");
                                strcpy(textShowString, (char *)msg);
                                ShowTextDialog(0, 0, 0);
                                shown = 1;
                            }
                            canSell = 0;
                        }
                    }
                }
                if (canSell) {
#ifndef V111
                    cost = ApplyPriceAndTechnologyFlux(
                        outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                        outfitSystem[selOutfitItem].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
#endif
                    cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    if (shipSystems[selOutfitItem]
                        <= savedOutfits[selOutfitItem])
                        cost *= 0.5;
                    have = ShipFreeMass();
                    shipSystems[selOutfitItem] -= 1;
                    if (ShipFreeMass() < 0 && have >= 0) {
                        shipSystems[selOutfitItem] += 1;
                        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                        break;
                    } else {
                        shipData->credits += cost;
                        for (j = 0; j < kOutfMods; j++) {
                            if (shipSystems[selOutfitItem] <= 0) {
                                if (outfitSystem[selOutfitItem].modType[j] == 1
                                    && shipData->selectedWeapon
                                       == outfitSystem[selOutfitItem].modVal[j]) {
                                    shipData->selectedWeapon = -1;
                                    statusRedraw[3] = 1;
                                    break;
                                }
                            }
                        }
                        crashlog_mission_bit_set(
                            "selling outfit",
                            (char *)outfitSystem[selOutfitItem].name, 1,
                            selOutfitItem + kFirstResourceID,
                            outfitSystem[selOutfitItem].onSell);
                        EvalMissionBitSetString(outfitSystem[selOutfitItem].onSell);
                        SystemInfoToShipStats();
                        if (SetupPortAvailableItems(dockedPort)) {
                            selOutfitItem = 0;
                            selOutfitListItem = -1;
                            outfitScroll = 0;
                            canBuy = canSell = 0;
                            strncpy(dialogDescTextPtr, "", 3);
#ifdef MACBUILD
                            InvalDialogItem(outfitDialog, 6);
                            InvalDialogItem(outfitDialog, 8);
#else
                            RedrawDialog(outfitDialog);
#endif
                            k = 0;
                            for (count = 0; count < kMaxOutfits; count++) {
                                if (dockedPortItems[count] != -1) {
                                    k++;
                                    break;
                                }
                            }
                            if (k == 0)
                                done = 1;
                        }
                    }
                }
                if (selOutfitItem != -1 && shipSystems[selOutfitItem] > 0) {
                    if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
                        canSell = 0;
                    else
                        canSell = 1;
                } else
                    canSell = 0;
            }
            if (selOutfitItem != -1)
                canBuy = CanBuyOutfitItem(selOutfitItem);
            else
                canBuy = 0;
#ifdef MACBUILD
            InvalDialogItem(outfitDialog, 4);
            InvalDialogItem(outfitDialog, 5);
            InvalDialogItem(outfitDialog, 7);
            InvalDialogItem(outfitDialog, 9);
#else
            RedrawDialog(outfitDialog);
#endif
            didOutfit = 1;
            statusRedraw[2] = 1;
            DrawStatus();
            SetDialogGWorld(outfitDialog);
            DrawOutfitButtons(-1);
        }
        if (itemHit == 10 && outfitScroll >= kGridCols) {
            outfitScroll -= kGridCols;
            if (selOutfitListItem >= 0 && selOutfitListItem < kGridCells - kGridCols) {
                selOutfitListItem += kGridCols;
                selOutfitItem =
                    dockedPortItems[outfitScroll + selOutfitListItem];
                LoadDescription(dialogDescTextPtr, selOutfitItem + kOutfitDescBase);
            } else {
                selOutfitItem = selOutfitListItem = -1;
                strncpy(dialogDescTextPtr, "", 3);
            }
            SetDialogGWorld(outfitDialog);
#ifdef MACBUILD
            InvalDialogItem(outfitDialog, 4);
            InvalDialogItem(outfitDialog, 7);
            InvalDialogItem(outfitDialog, 5);
            InvalDialogItem(outfitDialog, 6);
            InvalDialogItem(outfitDialog, 8);
            InvalDialogItem(outfitDialog, 9);
            InvalDialogItem(outfitDialog, 10);
            InvalDialogItem(outfitDialog, 11);
            InvalDialogItem(outfitDialog, 13);
#else
            RedrawDialog(outfitDialog);
#endif
        }
        if (itemHit == 11) {
            m = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (dockedPortItems[i] != -1)
                    m++;
            }
            if (outfitScroll < m - kGridCells) {
                outfitScroll += kGridCols;
                if (selOutfitListItem >= kGridCols) {
                    selOutfitListItem -= kGridCols;
                    selOutfitItem =
                        dockedPortItems[outfitScroll + selOutfitListItem];
                    LoadDescription(dialogDescTextPtr, selOutfitItem + kOutfitDescBase);
                } else {
                    selOutfitItem = selOutfitListItem = -1;
                    strncpy(dialogDescTextPtr, "", 3);
                }
                SetDialogGWorld(outfitDialog);
#ifdef MACBUILD
                InvalDialogItem(outfitDialog, 4);
                InvalDialogItem(outfitDialog, 7);
                InvalDialogItem(outfitDialog, 5);
                InvalDialogItem(outfitDialog, 6);
                InvalDialogItem(outfitDialog, 8);
                InvalDialogItem(outfitDialog, 9);
                InvalDialogItem(outfitDialog, 10);
                InvalDialogItem(outfitDialog, 11);
                InvalDialogItem(outfitDialog, 13);
#else
                RedrawDialog(outfitDialog);
#endif
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
        if (shipSystems[i] > 0 && (outfitSystem[i].flags & kOutfRemoveAfterBuy) != 0)
            shipSystems[i] = 0;
    }
    SystemInfoToShipStats();
    DisposeDialogNoFlash(outfitDialog);
    outfitDialog = NULL;
    if (outfitBackgroundPic != NULL) {
        QDReleaseResource(outfitBackgroundPic);
        outfitBackgroundPic = NULL;
    }
    if (previewPic != NULL) {
#ifdef MACBUILD
        HUnlock((Handle)previewPic);
#endif
        QDReleaseResource(previewPic);
        previewPic = NULL;
    }
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
#ifndef V111
    RedrawStatus(0);
#endif
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    RefreshSAT();
#endif
    shipData->targetStellar = savedUnk6C;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}
#endif
#ifdef MACBUILD
#undef InvalDialogItem
#endif

/* the grid filters' arrow keys: virtual key codes in 1.1.1, characters on win */
#if V111
#define kGridLeftKey        0x7b
#define kGridRightKey       0x7c
#define kGridUpKey          kUpArrowKeyCode
#define kGridDownKey        kDownArrowKeyCode
#define kGridPageUpKey      kPageUpKeyCode
#define kGridPageDownKey    kPageDownKeyCode
#else
#define kGridLeftKey        kLeftArrowChar
#define kGridRightKey       kRightArrowChar
#define kGridUpKey          kUpArrowChar
#define kGridDownKey        kDownArrowChar
#define kGridPageUpKey      kPageUpChar
#define kGridPageDownKey    kPageDownChar
#endif
#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &r), InvalWindowRect(GetDialogWindow(dlg), &r))
#endif
/* OutfitFilter. STATE: MATCH 100%, 562/562, relocregcheck 0 of 154.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004903c0
// MAC: _OutfitFilter i386 0x00058039 ppc 0x00060298
Boolean OutfitFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
#ifdef MACBUILD
    Rect r;
#endif
    short itemType;
    Handle item;
    Point where;
    EventKey c;
    short i;
    short count;
    long slot;
    short old;
    short oldPos;
    short row;
    short button;
    int what;

    if ((absquatulateStr)[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 1;
        return 1;
    }
    *itemHit = -1;
    if (selOutfitItem != -1) {
        canBuy = CanBuyOutfitItem(selOutfitItem);
        if (shipSystems[selOutfitItem] > 0) {
            if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
                canSell = 0;
            else
                canSell = 1;
        } else {
            canSell = 0;
        }
    } else {
        canSell = 0;
        canBuy = 0;
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
#if V111
        c = (ev->message & kKeyCodeMask) >> 8;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnKeyCode || c == kEnterKeyCode || c == kEscapeKeyCode) {
                ev->what = kMouseDown;
                *itemHit = 1;
                return 1;
            }
        }
        if (c == kBKeyCode) {
            ev->what = kMouseDown;
            *itemHit = 7;
            return 1;
        }
        if (c == kSKeyCode) {
            ev->what = kMouseDown;
            *itemHit = 4;
            return 1;
        }
#else
        c = EventCharOf(ev);
#ifdef MAC1010 /* 1.0.10 real: 0x10037490 the what test jumps over all three chars, escape too */
        if ((int)ev->what == kKeyDown && (IsReturnChar(c) || c == kEscapeChar)) {
#else
        if (((int)ev->what == kKeyDown && IsReturnChar(c)) || c == kEscapeChar) {
#endif
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
#endif
#ifdef MAC1010 /* 1.0.10 spelling */
        if ((c >= kGridLeftKey && c <= kGridDownKey) || c == kGridPageUpKey || c == kGridPageDownKey) {
#else
        if (c == kGridLeftKey || c == kGridRightKey || c == kGridUpKey ||
            c == kGridDownKey || c == kGridPageUpKey || c == kGridPageDownKey) {
#endif
            old = selOutfitItem;
            if (selOutfitListItem != -1) {
                count = 0;
                for (i = 0; i < 512; i++) {
                    if (dockedPortItems[i] != -1)
                        count++;
                }
                if (c == kGridRightKey) {
                    if (selOutfitListItem < kGridCells - 1) {
                        if (dockedPortItems[outfitScroll + selOutfitListItem
                                            + 1] != -1)
                            selOutfitListItem++;
                    } else if (outfitScroll < count - kGridCells) {
                        outfitScroll += kGridCols;
                        selOutfitListItem -= kGridCols - 1;
                    }
                } else if (c == kGridLeftKey) {
                    if (selOutfitListItem > 0) {
                        selOutfitListItem--;
                    } else if (outfitScroll > 0) {
                        outfitScroll -= kGridCols;
                        selOutfitListItem += kGridCols - 1;
                    }
                } else if (c == kGridUpKey || c == kGridPageUpKey) {
                    if (selOutfitListItem >= kGridCols)
                        selOutfitListItem -= kGridCols;
                    else if (outfitScroll > 0)
                        outfitScroll -= kGridCols;
                } else if (c == kGridDownKey || c == kGridPageDownKey) {
                    if (selOutfitListItem < kGridCells - kGridCols) {
                        if (dockedPortItems[outfitScroll + selOutfitListItem
                                            + kGridCols] != -1)
                            selOutfitListItem += kGridCols;
                    } else if (outfitScroll < count - kGridCells) {
                        outfitScroll += kGridCols;
                    }
                }
                selOutfitItem = dockedPortItems[outfitScroll + selOutfitListItem];
                while (selOutfitItem == -1 && selOutfitListItem > 0) {
                    selOutfitListItem--;
                    selOutfitItem = dockedPortItems[outfitScroll + selOutfitListItem];
                }
            } else {
                if (c == kGridRightKey || c == kGridDownKey ||
                    c == kGridPageDownKey)
                    selOutfitListItem = 0;
                else
                    selOutfitListItem = kGridCells - 1;
#ifdef MACBUILD
                selOutfitItem = dockedPortItems[outfitScroll + selOutfitListItem];
                while (selOutfitItem == -1 && selOutfitListItem > 0) {
                    selOutfitListItem--;
                    selOutfitItem = dockedPortItems[outfitScroll + selOutfitListItem];
                }
#else
                what = selOutfitListItem;
                what += outfitScroll;
                selOutfitItem = dockedPortItems[what];
                while (selOutfitItem == -1 && selOutfitListItem > 0) {
                    what--;
                    selOutfitListItem--;
                    selOutfitItem = dockedPortItems[what];
                }
#endif
            }
            if (selOutfitItem != old) {
                if (selOutfitListItem != -1) {
                    LoadDescription(dialogDescTextPtr,
                                    selOutfitItem + kOutfitDescBase);
                    RedrawDialog(outfitDialog);
                    canBuy = CanBuyOutfitItem(selOutfitItem);
                    if (shipSystems[selOutfitItem] > 0) {
                        if ((outfitSystem[selOutfitItem].flags
                             & kOutfCantSell) != 0)
                            canSell = 0;
                        else
                            canSell = 1;
                    } else {
                        canSell = 0;
                    }
                } else {
                    canBuy = canSell = 0;
                }
            }
        }
    }
    what = ev->what;
    if (what == kMouseDown) {
        SetDialogGWorld(outfitDialog);
        RedrawDialog(outfitDialog);
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
        if (PtInRectPoint(where, &box)) {
#ifdef CEFIX /* F13.01 */
            count = CE_GridDivide(where.h - box.left, ceGridCellWidth);
            row = CE_GridDivide(where.v - box.top, ceGridCellHeight);
#else
            count = (where.h - box.left) / kGridCellWide;
            row = (where.v - box.top) / kGridCellHigh;
#endif
            if (count >= 0 && count <= kGridCols - 1 && row >= 0 &&
                row <= kGridCells / kGridCols - 1) {
                oldPos = selOutfitListItem;
                selOutfitListItem = row * kGridCols + count;
                if (dockedPortItems[selOutfitListItem + outfitScroll] == -1) {
                    selOutfitListItem = selOutfitItem = -1;
                } else {
                    selOutfitItem =
                        dockedPortItems[selOutfitListItem + outfitScroll];
                }
                if (selOutfitListItem != oldPos) {
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
                    if (selOutfitItem != -1) {
                        canBuy = CanBuyOutfitItem(selOutfitItem);
                        if (shipSystems[selOutfitItem] > 0) {
                            if ((outfitSystem[selOutfitItem].flags
                                 & kOutfCantSell) != 0)
                                canSell = 0;
                            else
                                canSell = 1;
                        } else {
                            canSell = 0;
                        }
                        LoadDescription(dialogDescTextPtr,
                                        selOutfitItem + kOutfitDescBase);
                    } else {
                        canBuy = canSell = 0;
                        strncpy(dialogDescTextPtr, "", 3);
                    }
                }
                lastClickItem = selOutfitListItem;
                lastClickTime = ev->when;
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(outfitDialog));
#endif
        OutfitDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(outfitDialog));
#endif
        return 1;
    }
    if (ReadTick() >= missionOfferTime) {
        *itemHit = 8;
        missionOfferTime = ReadTick() + Rand(30) + 30;
        return 1;
    }
    return 0;
}
#ifdef MACBUILD
#undef InvalDialogItem
#endif

/* win reads the character once; on the mac the two ctype calls read it each */
#if V111 /* V111 spelling */
#define LineLeadChar(line)
#define LineLeadsAlnum(line) (isalpha((line)[1]) || isdigit((line)[1]))
#else
#define LineLeadChar(line) c = (line)[1]
#define LineLeadsAlnum(line) (isalpha(c) || isdigit(c))
#endif

/* OutfitDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00490c70
// MAC: _OutfitDialogUpdate i386 0x000572cb ppc 0x0005f680
void OutfitDialogUpdate(void)
{
#ifdef MACBUILD
    int c;
    short skipped;
    Handle item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short i;
    short drawn;
    long mass;
    long freeMass;
#else
    int c;
    short drawn;
    short skipped;
    Handle item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short i;
    long mass;
    long freeMass;
#endif

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(outfitDialog));
    if (outfitBackgroundPic != NULL)
        DrawPicHandle(outfitBackgroundPic, GetDialogPortRect(outfitDialog));
    ForeColor(blackColor);
    skipped = 0;
    GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(blackColor);
        drawn = 0;
        for (i = outfitScroll; i < 512; i++) {
            if (drawn < kGridCells && (i >= 0 && dockedPortItems[i] != -1)) {
                if (skipped < 3) {
                    if (i - outfitScroll >= 0 &&
                        i - outfitScroll < kGridCells) {
                        if (!RetrieveMenuIcon(dockedPortItems[i] + kOutfitPictBase,
                                            MenuIconPort(&loadPort),
                                            &listPicScreenRect[i - outfitScroll]))
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
                    ForeColor(whiteColor);
#ifdef CEFIX /* F12.54 */
                    CE_SetPen(listScreenRect[i - outfitScroll].right
                                  - (StringWidth(line1) + CE_Scale(3)),
                              listScreenRect[i - outfitScroll].top + CE_Scale(12));
#else
                    MoveTo((listScreenRect[i - outfitScroll].right
                                   - (StringWidth(line1) + 3)),
                           (listScreenRect[i - outfitScroll].top + 12));
#endif
                    DrawString(line1);
                    ForeColor(blackColor);
                }
                if (cheatsActive) {
                    NumToString(dockedPortItems[i] + kFirstResourceID, line1);
                    ForeColor(whiteColor);
                    MoveTo(((listScreenRect[i - outfitScroll].left
                                    + listScreenRect[i - outfitScroll].right
                                    - StringWidth(line1)) / 2),
                           (listScreenRect[i - outfitScroll].top + 24));
                    DrawString(line1);
                    ForeColor(blackColor);
                }
                if (SplitTwoLineString(outfitShortNames[dockedPortItems[i]],
                                       line1, line2)) {
                    LineLeadChar(line1);
                    if (LineLeadsAlnum(line1))
                        ForeColor(whiteColor);
                    else
                        RGBForeColor(&rgbLessDarkGrey);
#ifdef CEFIX /* F12.51 */
                    CE_SetPen((listScreenRect[i - outfitScroll].left
                                   + listScreenRect[i - outfitScroll].right) / 2
                                  - StringWidth(line1) / 2,
                              listScreenRect[i - outfitScroll].bottom - CE_Scale(14));
#else
                    MoveTo(((listScreenRect[i - outfitScroll].left
                                    + listScreenRect[i - outfitScroll].right) / 2
                                   - StringWidth(line1) / 2),
                           (listScreenRect[i - outfitScroll].bottom - 14));
#endif
                    DrawString(line1);
                    LineLeadChar(line2);
                    if (LineLeadsAlnum(line2))
                        ForeColor(whiteColor);
                    else
                        RGBForeColor(&rgbLessDarkGrey);
#ifdef CEFIX /* F12.52 */
                    CE_SetPen((listScreenRect[i - outfitScroll].left
                                   + listScreenRect[i - outfitScroll].right) / 2
                                  - StringWidth(line2) / 2,
                              listScreenRect[i - outfitScroll].bottom - CE_Scale(3));
#else
                    MoveTo(((listScreenRect[i - outfitScroll].left
                                    + listScreenRect[i - outfitScroll].right) / 2
                                   - StringWidth(line2) / 2),
                           (listScreenRect[i - outfitScroll].bottom - 3));
#endif
                    DrawString(line2);
                    ForeColor(blackColor);
                } else {
#ifdef CEFIX /* F12.53 */
                    CE_SetPen((listScreenRect[i - outfitScroll].left
                                   + listScreenRect[i - outfitScroll].right) / 2
                                  - StringWidth(line1) / 2,
                              listScreenRect[i - outfitScroll].bottom - CE_Scale(6));
#else
                    MoveTo(((listScreenRect[i - outfitScroll].left
                                    + listScreenRect[i - outfitScroll].right) / 2
                                   - StringWidth(line1) / 2),
                           (listScreenRect[i - outfitScroll].bottom - 6));
#endif
                    ForeColor(whiteColor);
                    DrawString(line1);
                    ForeColor(blackColor);
                }
                RGBForeColor(&shopGridColor[1]);
                FrameRect(&listScreenRect[i - outfitScroll]);
                ForeColor(blackColor);
                drawn++;
            }
        }
        if (selOutfitListItem != -1) {
            RGBForeColor(&shopGridColor[0]);
            FrameRect(&listScreenRect[selOutfitListItem]);
            ForeColor(blackColor);
        }
    }
    if (selOutfitListItem != -1) {
        GetDialogItem(outfitDialog, 8, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
            if (previewPicID != selOutfitItem + kOutfitPictBase) {
                if (previewPic != NULL) {
#ifdef MACBUILD
                    HUnlock((Handle)previewPic);
#endif
                    QDReleaseResource(previewPic);
                    previewPic = NULL;
                }
                previewPicID = selOutfitItem + kOutfitPictBase;
#ifdef MACBUILD
                if (previewPicID >= kFirstResourceID) {
                    previewPic = GetPicture(previewPicID);
                    if (previewPic != NULL)
                        HLock((Handle)previewPic);
                }
#else
                if (previewPicID >= kFirstResourceID)
                    previewPic = GetPicture(previewPicID);
#endif
            }
            if (previewPic != NULL) {
                ForeColor(blackColor);
                DrawPicHandle(previewPic, &box);
            } else {
                ForeColor(whiteColor);
                TextFont(smallTextFont);
                TextSize(smallTextSize);
                GetIndString(line1, kGameStrList, 0xd5);
                DrawStringCentered(line1, box.left, box.right,
                                   ((box.bottom + box.top) / 2 - 6));
                GetIndString(line1, kGameStrList, 0xd6);
                DrawStringCentered(line1, box.left, box.right,
                                   ((box.bottom + box.top) / 2 + 6));
                ForeColor(blackColor);
            }
        }
        GetDialogItem(outfitDialog, 9, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            ForeColor(whiteColor);
            MoveTo(box.left, (box.top + 12));
            DrawIndString(kGameStrList, 0xd7);
            MoveTo((box.left + 0x46), (box.top + 12));
            ApplyPriceAndTechnologyFlux(outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                            outfitSystem[selOutfitItem].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
            DrawCommaNumber(AdjustedItemCost(&outfitSystem[selOutfitItem],
                                             &shipClass[shipData->type]));
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x22);
            MoveTo(box.left, (box.top + 0x18));
            DrawIndString(kGameStrList, 0xd9);
            MoveTo((box.left + 0x46), (box.top + 0x18));
            DrawCommaNumber(shipData->credits);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x22);
            if (outfitSystem[selOutfitItem].mass > 0) {
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xd8);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                mass = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                DrawCommaNumber(mass);
                DrawString("\p ");
                if (mass == 1)
                    DrawString(tonString);
                else
                    DrawString(tonsString);
                MoveTo(box.left, (box.top + 0x3c));
                DrawIndString(kGameStrList, 0xda);
                MoveTo((box.left + 0x46), (box.top + 0x3c));
                freeMass = ShipFreeMass();
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
            if (HasMaxOfItem(selOutfitItem, NULL, NULL)) {
                if (shipSystems[selOutfitItem] > 0)
                    DrawIndString(kGameStrList, 0xdb);
                else
                    DrawIndString(kGameStrList, 0xdc);
            } else {
                mass = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                if (mass > 0 && mass > ShipFreeMass()) {
                    if (shipSystems[selOutfitItem] > 0)
                        DrawIndString(kGameStrList, 0xdd);
                    else
                        DrawIndString(kGameStrList, 0xde);
                }
            }
            ForeColor(blackColor);
        }
    }
    GetDialogItem(outfitDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(dialogDescTextPtr, strlen(dialogDescTextPtr), &box, 0);
        InvertRect(&box);
    }
    DrawOutfitButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(outfitDialog);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(outfitDialog)),
             GetDialogPortRect(outfitDialog), GetDialogPortRect(outfitDialog), 0,
             NULL);
#ifdef MAC1010
    QDFlushPortBuffer(GetWindowGrafPort(GetDialogWindow(outfitDialog)), NULL);
#else
#if V111
    QDFlushPortBuffer(GetWindowGrafPort(FrontWindow(outfitDialog)), NULL);
#endif
#endif
}

#define kSpecialTechCount  8

#define kPortDlogID         0x3e8
#define kLastPriceMult      1
#define kPortVenue          3
#define kMissionOfferSpread 30
#define kLandTimeSteps      3
#define kAllButtonsOff      -8

#define SpobHasTrade(s)     ((s)->flags & kSpobTradeCenter)
#define SpobHasOutfitter(s) ((s)->flags & kSpobHasOutfitter)
#define SpobAmbientLoop(s)  ((s)->flags2 & kSpobLoopSound)



/* CanBuyOutfitItem (mac _CanBuyOutfitItem). STATE: MATCH, 391/391, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00491950
// MAC: _CanBuyOutfitItem i386 0x0004e7c4 ppc 0x00057448
Boolean CanBuyOutfitItem(short outfit)
{
    Boolean isCargo;
    Boolean isMap;
#ifndef MAC1010
    short cargoDelta;
#endif
    long freeMass;
    long mass;
    Boolean can;
    Boolean isFakeID;
    Boolean avail;
    short i;
    short stellar;
#ifdef MAC1010
    short cargoDelta;
#endif

    if (outfit != -1) {
        freeMass = ShipFreeMass();
        if (outfitSystem[outfit].techLevel != -1) {
            can = 0;
            if (outfitSystem[outfit].techLevel <=
                    stellarData[shipData[0].targetStellar].techLevel) {
                can = 1;
            } else {
                for (i = 0; i < kSpecialTechCount; i++) {
                    if (stellarData[shipData[0].targetStellar]
                                .specialTech[i] == outfitSystem[outfit].techLevel) {
                        can = 1;
                        break;
                    }
                }
            }
            if (!can)
                return 0;
        }
        mass = AdjustedItemMass(&outfitSystem[outfit],
                                &shipClass[shipData[0].type]);
        if (!HasMaxOfItem(outfit, 0, 0)
                && (freeMass >= mass || mass <= 0)) {
            isCargo = 0;
            cargoDelta = 0;
            isMap = 0;
            isFakeID = 0;
            for (i = 0; i < kOutfMods; i++) {
                if (outfitSystem[outfit].modType[i] == kOutfModCargo) {
                    isCargo = 1;
                    cargoDelta = outfitSystem[outfit].modVal[i];
                }
                if (outfitSystem[outfit].modType[i] == kModTypeMap)
                    isMap = 1;
                if (outfitSystem[outfit].modType[i] == kModTypeFakeID)
                    isFakeID = 1;
            }
            if (outfitSystem[outfit].modType[0] == kOutfModAmmo) {
                if (outfitSystem[outfit].modVal[0] >= 0) {
                    if (weaponStats[outfitSystem[outfit].modVal[0]].guidance ==
                            kGuidanceBay) {
                        if (CanBuyFighter(
                                weaponStats[outfitSystem[outfit].modVal[0]].ammoType
                                    - kFirstResourceID,
                                outfit, outfitSystem[outfit].modVal[0]))
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
                if (cargoDelta < 0) {
                    if (shipClass[shipData[0].type].cargoNotFixed == 0) {
                        can = 0;
                    } else if (ShipTotalHolds() - TotalCargo(shipData) >=
                            abs(cargoDelta)) {
                        can = 1;
                    } else {
                        can = 0;
                    }
                } else {
                    can = 1;
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
                    if (InGovtClass(outfitSystem[outfit].govtRestrict, 0)) {
                        avail = 0;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(0))
                            avail = 1;
                        else if (stellarData[stellar].govt != -1
                                 && GovtAllies(stellarData[stellar].govt,
                                               outfitSystem[outfit].govtRestrict
                                                   - ResClassBase(0)))
                            avail = 1;
                    } else if (InGovtClass(outfitSystem[outfit].govtRestrict, 1)) {
                        avail = 0;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(1)) {
                            avail = 1;
                        } else if (stellarData[stellar].govt != -1) {
                            if (GovtAllies(stellarData[stellar].govt,
                                           outfitSystem[outfit].govtRestrict
                                               - ResClassBase(1)))
                                avail = 1;
                        } else {
                            avail = 1;
                        }
                    } else if (InGovtClass(outfitSystem[outfit].govtRestrict, 2)) {
                        avail = 1;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(2))
                            avail = 0;
                        else if (stellarData[stellar].govt != -1
                                 && GovtAllies(stellarData[stellar].govt,
                                               outfitSystem[outfit].govtRestrict
                                                   - ResClassBase(2)))
                            avail = 0;
                    } else if (InGovtClass(outfitSystem[outfit].govtRestrict, 3)) {
                        avail = 1;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(3)) {
                            avail = 0;
                        } else if (stellarData[stellar].govt != -1) {
                            if (GovtAllies(stellarData[stellar].govt,
                                           outfitSystem[outfit].govtRestrict
                                               - ResClassBase(3)))
                                avail = 0;
                        } else {
                            avail = 0;
                        }
                    }
                }
            }
            if (avail) {
                if (!PlayerMeetsRequirements(outfitSystem[outfit].requires[0],
                                             outfitSystem[outfit].requires[1]))
                    can = 0;
            }
        }
        if (can) {
            if (!EvalMissionBitTestString(outfitSystem[outfit].availability))
                can = 0;
        }
        if (can) {
            ApplyPriceAndTechnologyFlux(outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                            outfitSystem[selOutfitItem].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
            {
                long cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                             &shipClass[shipData[0].type]);

                if (shipData[0].credits < cost)
                    can = 0;
            }
        }
    } else {
        can = 0;
    }
    return can;
}

/* the mac engine invalidates a dialog item through the caller's own GetDialogItem locals */
#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &box), InvalWindowRect(GetDialogWindow(dlg), &box))
#endif
/* DoPortDialog (mac _DoPortDialog). STATE: MATCH 561/561, byte-identical yes (2413 bytes),
   relocregcheck 0 of 237. `n' holds the fuel call's result and counts the shown missions;
   the sound handle is stored straight into planetSnd. OPEN: none. */
// FUNCTION: EVNOVA 0x00491f30
// MAC: _DoPortDialog i386 0x0005f911 ppc 0x000675a4
void DoPortDialog(SpobRecord *spob, short stellar)
{
    Boolean uninhabited;
    short i;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short n;
    short j;
    unsigned short vol;
    Boolean quit;
#ifdef MACBUILD
    short itemType;
    Handle item;
    Rect box;
#endif

    quit = 0;
    if (screenFaded != 0)
        return;
    onPlanet = 1;
    dockedPort = spob;
    didOutfit = didBuyShip = 0;
    strncpy(descTextPtr, "", 5);
    LoadDescription(descTextPtr, stellar + kFirstResourceID);
    for (i = 0; i <= kLastPriceMult; i++)
        priceFluxMult[i] = 1.0f;
    if (spob->govt != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0
                    && GovtAllies(spob->govt, rankData[i].govt)) {
                for (j = 0; j <= kLastPriceMult; j++)
                    priceFluxMult[j] *= (float)rankData[i].priceMod
                                        * kPortPriceScale;
            }
        }
    }
    portDialog = NULL;
    portDialog = GetNewDialog(kPortDlogID, PortDialogUpdate,
                                  PortFilter);
    if (portDialog == NULL)
        return;
    if (spob->customSound >= kFirstResourceID && settings.ambientSounds != 0) {
        planetSnd = ST_LoadSndResource(spob->customSound);
        planetSoundSpec.handle = (void **)planetSnd;
        vol = gSoundVolume;
        planetSoundSpec.leftLevel = vol;
        planetSoundSpec.rightLevel = vol;
        ambientNoiseDelay = 0;
    } else {
        planetSnd = NULL;
    }
    CenterWindow(FrontWindow(portDialog), gWind);
    if (SpobUninhabited(dockedPort) != 0)
        canDoMissions = 0;
    else
        canDoMissions = 1;
    SetDialogGWorld(portDialog);
    BackColor(blackColor);
    ShowWindowNoFlash(FrontWindow(portDialog));
    BackColor(whiteColor);
    SetDialogGWorld(portDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    RedrawDialog(portDialog);
    PortDialogUpdate();
    SelectWindow(FrontWindow(portDialog));
    BringToFront(FrontWindow(portDialog));
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
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
    missionsOutside = kPortVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(kMissionOfferSpread)
                       + kMissionOfferSpread;
    OfferMissionFromPort(kPortVenue);
#if V111
    if (!useLegacyRenderingPipeline) {
        statusRedraw[4] = 1;
        statusRedraw[2] = 1;
        gSpriteWorld->fastBlit = 1;
        DrawStatus();
        gSpriteWorld->fastBlit = 0;
    }
#endif
    do {
        if (gQuit)
            quit = 1;
        MyModalDialog(portDialog, PortFilter, &itemHit, 0);
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
            uninhabited = SpobUninhabited(dockedPort);
            if (uninhabited == 0) {
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
            if (planetSnd != NULL
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
#ifndef MACBUILD
                RedrawDialog(portDialog);
#endif
            }
        } else if (itemHit == 10) {
            if (canDoMissions != 0) {
                savedStellar = shipData->targetStellar;
                savedRoute = shipData->navTargetKind;
                SetDialogGWorld(portDialog);
#ifdef MACBUILD
                InvalDialogItem(portDialog, 10);
#else
                RedrawDialog(portDialog);
#endif
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
#ifdef MACBUILD
            InvalDialogItem(portDialog, 11);
#else
            RedrawDialog(portDialog);
#endif
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
    if (planetPicContainer.world != NULL)
        DisposeGWorldContainer(&planetPicContainer);
    if (portBackgroundPic != NULL) {
        QDReleaseResource(portBackgroundPic);
        portBackgroundPic = NULL;
    }
    if (planetSnd != NULL) {
        ST_HaltSound(planetSnd);
        ST_DisposeSound(planetSnd);
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
#ifdef MACBUILD
#undef InvalDialogItem
#endif

#define kActivateEvt  8

#define kAmbientPeriod   480


#define kNameBaseline 0x12
#define kNameSize     0x12

/*
 * A click inside item 5 - the planet image - reports the image rather than
 * whatever button the tracker thought it was over.
 */

/* PortFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004928a0
// MAC: _PortFilter i386 0x0004f041 ppc 0x00057c18
Boolean PortFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    Point where;
    short button;
#if V111
    short key;
#else
    EventChar c;
#endif

    *itemHit = -1;
    if ((absquatulateStr)[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 12;
        return 1;
    }
    if (SpobUninhabited(dockedPort))
        canDoMissions = 0;
    else
        canDoMissions = 1;
    if ((int)ev->what == kActivateEvt && ambientNoiseDelay != 0)
        ambientNoiseDelay = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
    if (planetSnd != NULL) {
        if ((dockedPort->flags2 & kSpobLoopSound) != 0) {
            if (ST_IsSoundPlaying(planetSnd) == 0)
                ST_PlaySoundParam(&planetSoundSpec);
        } else if (ReadTick() >= ambientNoiseDelay) {
            ST_PlaySoundParam(&planetSoundSpec);
            ambientNoiseDelay = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
        }
    }
    *itemHit = -1;
    if ((int)ev->what == kActivateEvt)
        ambientNoiseDelay = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
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
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
            ev->what = kMouseDown;
            *itemHit = 12;
            return 1;
        }
        if (key == kRKeyCode || key == kFKeyCode) {
            *itemHit = 4;
            return 1;
        }
        if (key == kCKeyCode || key == kTKeyCode) {
            *itemHit = 7;
            return 1;
        }
        if (key == kOKeyCode) {
            *itemHit = 8;
            return 1;
        }
        if (key == kSKeyCode) {
            *itemHit = 9;
            return 1;
        }
        if (key == kNKeyCode) {
            *itemHit = 10;
            return 1;
        }
        if (key == kBKeyCode) {
            *itemHit = 11;
            return 1;
        }
#else
        c = EventCharOf(ev);
        c = tolower(c);
        if (IsReturnChar(c) || c == kEscapeChar) {
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
#endif
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        PortDialogUpdate();
        button = TrackPlanetButtons(where);
        GetDialogItem(portDialog, 5, &itemType, (Handle *)&item, &box);
        if (PtInRectPoint(where, &box)) {
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(portDialog));
#endif
        PortDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(portDialog));
#endif
        return 1;
    }
    return 0;
}

/* PortDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00492c40
// MAC: _PortDialogUpdate i386 0x0004ebbd ppc 0x00057884
void PortDialogUpdate(void)
{
#ifdef MACBUILD
    Rect memBox;
    Str255 memText;
#endif
    Rect box;
    short itemType;
    Handle item;
    Boolean inside;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(portDialog));
    if (portBackgroundPic != NULL)
        DrawPicHandle(portBackgroundPic, GetDialogPortRect(portDialog));
    GetDialogItem(portDialog, 3, &itemType, &item, &box);
    inside = RectInRgn(&box, GetWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        ForeColor(blackColor);
        PaintRect(&box);
        ForeColor(whiteColor);
        MoveTo(box.left, (box.top + kNameBaseline));
        TextFont(gTimesFont);
        TextSize(kNameSize);
#ifdef CEFIX /* F12.55 */
        DrawStringCentered(dockedPort->name, box.left, box.right,
                           box.top + CE_Scale(kNameBaseline));
#else
        DrawStringCentered(dockedPort->name, box.left, box.right,
                           (box.top + kNameBaseline));
#endif
        ForeColor(blackColor);
    }
    GetDialogItem(portDialog, 5, &itemType, &item, &box);
    inside = RectInRgn(&box, GetWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        if (planetPicContainer.world != NULL)
            CopyBits(GWorldContainerBits(&planetPicContainer),
                     (BitMapPtr)GWorldContainerBits(&loadPort),
                     &planetPicContainer.bounds, &box, 0,
                     GetWindowVisRgn(FrontWindow(portDialog)));
#ifdef MACBUILD
        if (cheatsActive) {
            memBox.top = box.top + 3;
            memBox.left = box.left + 3;
            memBox.right = memBox.left + 64;
            memBox.bottom = memBox.top + 11;
            ForeColor(blackColor);
            PaintRect(&memBox);
            MoveTo(memBox.left + 3, memBox.top + 9);
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            ForeColor(whiteColor);
            NumToString(FreeMem() / 1000, memText);
            DrawString(memText);
            DrawString("\pK free");
            ForeColor(blackColor);
        }
#endif
    }
    GetDialogItem(portDialog, 6, &itemType, &item, &box);
    inside = RectInRgn(&box, GetWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        ForeColor(blackColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(descTextPtr, strlen(descTextPtr), &box, 0);
        InvertRect(&box);
    }
    SetGWorldContainer(&loadPort);
    DrawPlanetButtons(-8);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(portDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
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
/* MyModalDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00492f10
// MAC: _MyModalDialog i386 0x00059954 ppc 0x00061ac4
void MyModalDialog(DialogPtr dlg, void *eventProc, short *itemHit,
                    short *modifiers)
#ifdef MACBUILD
{
    EventRecord ev;
    Rect r;
    GrafPtr savedPort;
    WindowPtr window;
    long sleep;

    *itemHit = -1;
#ifdef MAC1010
    HandleIntroMusic(0);
    sleep = inBackground ? ((gameLoaded && inBackground) ? 30 : 0) : 0;
    WaitNextEvent(everyEvent, &ev, sleep, NULL);
#else
    WaitNextEvent(everyEvent, &ev, (inBackground && gameLoaded) ? 30 : 0, NULL);
#endif
    if (ev.what == kHighLevelEvent)
        AEProcessAppleEvent(&ev);
    if (ev.what == activateEvt && (ev.modifiers & activeFlag))
        SwitchedIn(&ev);
    if (ev.what == osEvt && (ev.message & (suspendResumeMessage << 24))) {
        if (ev.message & resumeFlag) {
            inBackground = 0;
            InitCursor();
            cursorHidden = 0;
            ShowTheCursor();
            if (!settings.windowed) {
#if V111
                SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
#else
                HideTheMenuBar();
#endif
                mBarHidden = 1;
                r = *GetCWindowPortRect((WindowRef)gWind);
                r.top -= 30;
                InvalWindowRect((WindowRef)gWind, &r);
                if (gSystemVersion < 0x1000 && mBarHidden && SWIsWindowInForeground(gSpriteWorld))
                    KeepMenuBarHidden((GrafPtr)gSpriteWorld->windowFrameP->framePort);
            }
            SwitchedIn(&ev);
        } else {
            if (!settings.windowed) {
#if V111
                SetSystemUIMode(kUIModeNormal, 0);
#else
                ShowTheMenuBar();
#endif
                mBarHidden = 0;
            }
            inBackground = 1;
            InitCursor();
            cursorHidden = 0;
            ShowTheCursor();
            SwitchedOut(&ev);
        }
    }
    if (!inBackground) {
        SelectWindow(GetDialogWindow(dlg));
        SetPort((GrafPtr)GetDialogPort(dlg));
    }
    if (ev.what == updateEvt) {
        GetPort(&savedPort);
        window = (WindowPtr)ev.message;
        BeginUpdate(window);
        if (window == (WindowPtr)gWind) {
            SetPort((GrafPtr)GetCWindowGrafPort((WindowRef)gWind));
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    ForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    ForeColor(blackColor);
                    PaintRect(GetWindowPortRect((WindowRef)gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                RedrawStatus(0);
                RefreshSAT();
            }
        }
        if (portDialog != NULL && window == GetDialogWindow(portDialog)) {
            SetPort((GrafPtr)GetDialogPort(portDialog));
            PortDialogUpdate();
        }
        if (shipyardDialog != NULL && window == GetDialogWindow(shipyardDialog)) {
            SetPort((GrafPtr)GetDialogPort(shipyardDialog));
            ShipyardDialogUpdate();
        }
        if (shipDescDialog != NULL && window == GetDialogWindow(shipDescDialog)) {
            SetPort((GrafPtr)GetDialogPort(shipDescDialog));
            ShipDescDialogUpdate();
        }
        if (outfitDialog != NULL && window == GetDialogWindow(outfitDialog)) {
            SetPort((GrafPtr)GetDialogPort(outfitDialog));
            OutfitDialogUpdate();
        }
        if (tradeDialog != NULL && window == GetDialogWindow(tradeDialog)) {
            SetPort((GrafPtr)GetDialogPort(tradeDialog));
            TradeDialogUpdate();
        }
        if (textDialog != NULL && window == GetDialogWindow(textDialog)) {
            SetPort((GrafPtr)GetDialogPort(textDialog));
            TextDialogUpdate();
        }
        if (infoDialog != NULL && window == GetDialogWindow(infoDialog)) {
            SetPort((GrafPtr)GetDialogPort(infoDialog));
            PlayerInfoDialogUpdate();
        }
        if (missionDialog != NULL && window == GetDialogWindow(missionDialog)) {
            SetPort((GrafPtr)GetDialogPort(missionDialog));
            if (missionsOutside == 0 && portDialog != NULL)
                MissionDialogUpdate();
            else
                SingleMissionDialogUpdate();
        }
        if (missionInfoDialog != NULL && window == GetDialogWindow(missionInfoDialog)) {
            SetPort((GrafPtr)GetDialogPort(missionInfoDialog));
            MissionInfoDialogUpdate();
        }
        if (barDialog != NULL && window == GetDialogWindow(barDialog)) {
            SetPort((GrafPtr)GetDialogPort(barDialog));
            BarDialogUpdate();
        }
        if (raceDialog != NULL && window == GetDialogWindow(raceDialog)) {
            SetPort((GrafPtr)GetDialogPort(raceDialog));
            RaceDialogUpdate();
        }
        if (newsDialog != NULL && window == GetDialogWindow(newsDialog)) {
            SetPort((GrafPtr)GetDialogPort(newsDialog));
            NewsDialogUpdate();
        }
        if (commDialog != NULL && window == GetDialogWindow(commDialog)) {
            SetPort((GrafPtr)GetDialogPort(commDialog));
            if (commDialogMode == 1) {
                PlanetCommDialogUpdate();
            } else if (commShip != NULL) {
                if (commDialogMode == 2)
                    EscortDialogUpdate();
                else
                    CommDialogUpdate();
            }
        }
        if (haggleDialog != NULL && window == GetDialogWindow(haggleDialog)) {
            SetPort((GrafPtr)GetDialogPort(haggleDialog));
            HaggleDialogUpdate();
        }
        if (plunderDialog != NULL && window == GetDialogWindow(plunderDialog)) {
            SetPort((GrafPtr)GetDialogPort(plunderDialog));
            PlunderDialogUpdate();
        }
        if (mapDialog != NULL && window == GetDialogWindow(mapDialog)) {
            SetPort((GrafPtr)GetDialogPort(mapDialog));
            MapDialogUpdate();
        }
        if (captureDialog != NULL && window == GetDialogWindow(captureDialog)) {
            SetPort((GrafPtr)GetDialogPort(captureDialog));
            CapturedAssignmentDialogUpdate();
        }
        if (newsDialog != NULL && window == GetDialogWindow(newsDialog)) {
            SetPort((GrafPtr)GetDialogPort(newsDialog));
            NewsDialogUpdate();
        }
        if (keyDialog != NULL && window == GetDialogWindow(keyDialog)) {
            SetPort((GrafPtr)GetDialogPort(keyDialog));
            KeyDialogUpdate();
        }
        EndUpdate(window);
        SetPort(savedPort);
    }
    if (ev.what != osEvt && !inBackground) {
        if (eventProc != NULL) {
            ((ModalFilterProcPtr)eventProc)(dlg, &ev, itemHit);
            if (modifiers != NULL)
                *modifiers = ev.modifiers;
        }
    }
}
#else
{
    ModalDialog(NULL, itemHit);
    if (modifiers != NULL)
        *modifiers = GetKeyModifiers();
}
#endif

#define kShipDescBase 13000    /* desc ID of ship 0's shipyard blurb */
#define kHireDescBase 14000     /* ...and of its hire-an-escort blurb */

/* the item grid: four across, five down, out of item 5's box */
#define kMovieItem    8


#define kShipyardPictID 0x2135

/* per-mission state, stride 0x14; buttons.c's MissionState only names +0x00 */

/* DoShipyardDialog (mac _DoShipyardDialog). STATE: MATCH 100%, exe 981 / obj 981, frame 0x32c,
   byte-identical yes (2026-09-25). `weap' also counts the first scan, the name digits and the
   default outfits; `itemType' and `digitBuf' are declared last (a compound: neither alone). */
// FUNCTION: EVNOVA 0x00492f30
// MAC: _DoShipyardDialog i386 0x0005e679 ppc 0x00066438
void DoShipyardDialog(void)
{
    void *item;
    short halfHeight;
#ifdef MAC1010 /* 1.0.10 spelling */
    unsigned char msgText[256];
    int hireCost;
    unsigned char defaultName[256];
#else
    unsigned char defaultName[256];
    int hireCost;
    unsigned char msgText[256];
#endif
    Rect box;
    short itemHit;
    short i;
    short itemType;
    short outf;
    long tradeIn;
    long cost;
    short weap;
    Boolean weapHasSlot;
    short savedTargetStellar;
    short n;
    short savedUnk6A;
    Boolean confirmed;
    Boolean done;
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
        strcpy(textShowString, "");
        if (shipBuyMode == 0)
            StrcatIndCString(textShowString, kGameStrList, 0xdf);
        else
            StrcatIndCString(textShowString, kGameStrList, 0xe0);
        ShowTextDialog(0, 0, 0);
        return;
    }
    SWSetPortToWindow(gSpriteWorld);
    selShipyardItem = -1;
    selShipListItem = -1;
    lastClickItem = -1;
    shipyardScroll = 0;
    canBuy = 0;
    savedTargetStellar = shipData->targetStellar;
    savedUnk6A = shipData->navTargetKind;
    previewPic = NULL;
    previewPicID = -1;
    missionsOutside = 5;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    shipyardDialog = NULL;
    shipyardDialog = GetNewDialog(0x3ec, ShipyardDialogUpdate, ShipyardFilter);
    if (shipyardDialog == NULL)
        return;
    shipyardBackgroundPic = GetPicture(kShipyardPictID);
    CenterWindow(FrontWindow(shipyardDialog), FrontWindow(portDialog));
    GetDialogItem(shipyardDialog, kGridItem, &itemType, (Handle *)&item, &box);
    SetupListRects((Point *)&box);
    strncpy(dialogDescTextPtr, "", 3);
    ShowWindowNoFlash(FrontWindow(shipyardDialog));
    SetDialogGWorld(shipyardDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    ShipyardDialogUpdate();
    SelectWindow(FrontWindow(shipyardDialog));
    BringToFront(FrontWindow(shipyardDialog));
    if (shipBuyMode == 0)
        OfferMissionFromPort(5);
    RedrawDialog(shipyardDialog);
    GetDialogItem(shipyardDialog, kMovieItem, &itemType, (Handle *)&item, &box);

    do {
        if (gQuit)
            done = 1;
        MyModalDialog(shipyardDialog, ShipyardFilter, &itemHit, 0);
        if (itemHit == 1) {
            if (selShipyardItem != -1) {
                n = selShipyardItem;
                if (canBuy != 0) {
                    if (shipBuyMode == 0) {
                        extern char *strcpy(char *, const char *);

                        RedrawDialog(shipyardDialog);
                        strcpy((char *)msgText, "");
                        StrcatIndCString((char *)msgText, kGameStrList, 0x78);
                        strcat((char *)msgText, " ");
                        strncpy((char *)defaultName, (char *)shipyardLongNames[n], 200);
                        PtoCstr(defaultName);
                        strcat((char *)msgText, (char *)defaultName);
                        strcat((char *)msgText, ": ");
                        CtoPstr((char *)msgText);
                        pstrcpy(defaultName,
                               shipClass[n].name);
                        PtoCstr(defaultName);
                        strcat((char *)defaultName, " ");
                        for (weap = 0; weap <= 2; weap++) {
                            NumToString(Rand(9) + 1, (unsigned char *)digitBuf);
                            PtoCstr((unsigned char *)digitBuf);
                            strcat((char *)defaultName, digitBuf);
                        }
                        CtoPstr((char *)defaultName);
                        confirmed = EVTextInputDialog(msgText,
                                                      defaultName, 0x40);
#ifdef MACBUILD
                        RefreshSAT();
#endif
                        SetDialogGWorld(shipyardDialog);
                        RedrawDialog(shipyardDialog);
                        if (confirmed != 0) {
                            didBuyShip = 1;
                            tradeIn = ApplyPriceAndTechnologyFlux(
                                PlayerShipTradeInPrice(), priceFluxMult[0],
                                shipClass[shipData->type].techLevel,
                                stellarData[shipData->targetStellar].techLevel);
                            tradeIn = ApplyPriceAndTechnologyFlux(
                                tradeIn, priceFluxMult[1],
                                shipClass[shipData->type].techLevel,
                                stellarData[shipData->targetStellar].techLevel);
                            shipData->credits += tradeIn;
                            crashlog_mission_bit_set(
                                "retiring ship", (char *)shipClass[shipData->type].name,
                                1, shipData->type + 128,
                                shipClass[shipData->type].onRetire);
                            EvalMissionBitSetString(shipClass[shipData->type].onRetire);
                            shipData->type = n;
                            cost = ApplyPriceAndTechnologyFlux(
                                shipClass[n].cost, priceFluxMult[1],
                                shipClass[n].techLevel,
                                stellarData[shipData->targetStellar].techLevel);
                            shipData->credits -= cost;
                            pstrcpy(shipName, textInputString);
                            CullNameString(shipName);
                            for (weap = 1; weap < 0x40; weap++) {
                                if (shipData[weap].active != 0 &&
                                    shipData[weap].parentShip == 0 &&
                                    shipData[weap].aiType == 5)
                                    shipData[weap].active = 0;
                            }
                            for (weap = 0; weap < 256; weap++) {
                                weapHasSlot = 0;
                                for (outf = 0; outf < 512; outf++) {
                                    if (outfitSystem[outf].modType[0] == 1 &&
                                        outfitSystem[outf].modVal[0] == weap &&
                                        outfitSystem[outf].persistent != 0) {
                                        weapHasSlot = 1;
                                        break;
                                    }
                                }
                                if (!weapHasSlot) {
                                    shipData->weap[weap].count = 0;
                                    shipData->weap[weap].ammo = 0;
                                }
                            }
                            for (weap = 0; weap < 512; weap++) {
                                if (outfitSystem[weap].persistent == 0)
                                    shipSystems[weap] = 0;
                            }
                            for (outf = 0; outf < 8; outf++) {
                                if (shipClass[shipData->type].defOutfCount[outf] > 0)
                                    shipSystems[shipClass[shipData->type].defOutfID[outf]] +=
                                        shipClass[shipData->type].defOutfCount[outf];
                            }
                            SystemInfoToShipStats();
                            for (weap = 0; weap < 256; weap++) {
                                if (shipClass[shipData->type].weapCount[weap] >
                                    shipData->weap[weap].count)
                                    shipData->weap[weap].count =
                                        shipClass[shipData->type].weapCount[weap];
                                if (shipClass[shipData->type].weapAmmo[weap] > 0) {
                                    if (weaponStats[weap].ammoType >= 0 && weaponStats[weap].ammoType < 0x100
                                        && weaponStats[weap].guidance != kGuidanceBay)
                                        i = weaponStats[weap].ammoType;
                                    else
                                        i = weap;
                                    if (shipClass[shipData->type].weapAmmo[weap] >
                                        shipData->weap[i].ammo)
                                        shipData->weap[i].ammo =
                                            shipClass[shipData->type].weapAmmo[weap];
                                }
                            }
                            DestroyPartialFleetCargo(0);
                            crashlog_mission_bit_set(
                                "purchasing ship", (char *)shipClass[shipData->type].name,
                                1, shipData->type + 128,
                                shipClass[shipData->type].onPurchase);
                            EvalMissionBitSetString(shipClass[shipData->type].onPurchase);
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
                        shipClass[selShipyardItem].dailySaleRoll = Rand(100) + 1;
                        SetDialogGWorld(shipyardDialog);
                        RedrawDialog(shipyardDialog);
                    } else {
                        hireCost = ApplyPriceAndTechnologyFlux(shipClass[selShipyardItem].cost,
                                                   priceFluxMult[1],
                                                   shipClass[selShipyardItem].techLevel,
                                                   stellarData[shipData->targetStellar].techLevel);
                        shipData->credits -= hireCost * 0.1;
                        statusRedraw[2] = 1;
                        SpawnEscort(selShipyardItem, shipData->targetStellar);
                        shipClass[selShipyardItem].dailyHireRoll = Rand(100) + 1;
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
            savedTargetStellar = shipData->targetStellar;
            savedUnk6A = shipData->navTargetKind;
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
#ifdef MACBUILD
                UpdateAllPortDialogs(portDialog);
                SetDialogGWorld(shipyardDialog);
                ShipyardDialogUpdate(shipyardDialog);
#else
                UpdateAllPortDialogs();
                SetDialogGWorld(shipyardDialog);
                ShipyardDialogUpdate();
#endif
            }
        }
        if (itemHit == 0xb) {
            n = 0;
            for (weap = 0; weap < 16; weap++) {
                if (currentMissionObjectives[weap].active != 0 &&
                    (currentMissionObjectives[weap].flags & kMissionHidden) == 0)
                    n++;
            }
            if (n > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                PortDialogUpdate();
                SetDialogGWorld(shipyardDialog);
                RedrawDialog(shipyardDialog);
            }
        }
        if (itemHit == 10 && selShipyardItem != -1) {
            ShipyardDialogUpdate();
            DoShipyardDescription(selShipyardItem);
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 0xc && shipyardScroll >= 4) {
            shipyardScroll -= 4;
            selShipyardItem = selShipListItem = -1;
            strncpy(dialogDescTextPtr, "", 3);
            EndLoopingMovie();
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 0xd) {
            n = 0;
            for (weap = 0; weap < 768; weap++) {
                if (dockedPortShipTypes[weap] != -1)
                    n++;
            }
            if (shipyardScroll < n - 20) {
                shipyardScroll += 4;
                selShipyardItem = selShipListItem = -1;
                strncpy(dialogDescTextPtr, "", 3);
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
    if (shipyardBackgroundPic != NULL) {
        QDReleaseResource(shipyardBackgroundPic);
        shipyardBackgroundPic = NULL;
    }
    if (previewPic != NULL) {
#ifdef MACBUILD
        HUnlock((Handle)previewPic);
#endif
        QDReleaseResource(previewPic);
        previewPic = NULL;
    }
    if (shipBuyMode != 0) {
        box.left = box.right = ((GetCWindowPortRect((WindowRef)gWind)->left +
                             GetCWindowPortRect((WindowRef)gWind)->right) / 2);
        box.top = box.bottom = ((GetCWindowPortRect((WindowRef)gWind)->top +
                              GetCWindowPortRect((WindowRef)gWind)->bottom) / 2);
        InsetRect(&box, n - 1, halfHeight - 1);
        SWSetPortToWindow(gSpriteWorld);
        ForeColor(blackColor);
        PaintRect(&box);
        RedrawStatus(1);
        RedrawComm(0);
    }
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    RedrawStatus(0);
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    RefreshSAT();
#endif
    shipData->targetStellar = savedTargetStellar;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[0] = 1;
    ResetMissionOfferVenue();
}

#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &movieBox), InvalWindowRect(GetDialogWindow(dlg), &movieBox))
#endif
/* ShipyardFilter (mac _ShipyardFilter). STATE: MATCH 587/587 (2026-09-24).
   OPEN: none */
// FUNCTION: EVNOVA 0x00493fc0
// MAC: _ShipyardFilter i386 0x00060449 ppc 0x00068154
Boolean ShipyardFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    Rect movieBox;
    short itemType;
    Handle item;
    Point where;
    EventKey c;
    short i;
    short count;
    long slot;
    short old;
    short oldPos;
    short row;
    short button;
    long cell;

    if ((absquatulateStr)[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 7;
        return 1;
    }
    LoopingMovieIdle();
    *itemHit = -1;
    CalcShipCanBuy();
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
#if V111
        c = (ev->message & kKeyCodeMask) >> 8;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnKeyCode || c == kEnterKeyCode || c == kEscapeKeyCode) {
                ev->what = kMouseDown;
                *itemHit = 7;
                return 1;
            }
            if ((c == kBKeyCode && shipBuyMode == 0) || (c == kHKeyCode && shipBuyMode == 1)) {
                ev->what = kMouseDown;
                *itemHit = 1;
                return 1;
            }
            if (c == kSpaceKeyCode) {
                ev->what = kMouseDown;
                *itemHit = 10;
                return 1;
            }
        }
#else
        c = EventCharOf(ev);
        if ((int)ev->what == kKeyDown) {
            if (IsReturnChar(c) || c == kEscapeChar) {
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
#ifdef CEFIX /* F19.01 */
            if (c == ' ') {
                ev->what = kMouseDown;
                *itemHit = 10;
                return 1;
            }
#endif
        }
#endif
#ifdef MAC1010 /* 1.0.10 spelling */
        if ((c >= kGridLeftKey && c <= kGridDownKey) || c == kGridPageUpKey || c == kGridPageDownKey) {
#else
        if (c == kGridLeftKey || c == kGridRightKey || c == kGridUpKey ||
            c == kGridDownKey || c == kGridPageUpKey || c == kGridPageDownKey) {
#endif
            old = selShipyardItem;
            if (selShipListItem != -1) {
                count = 0;
                for (i = 0; i < 512; i++) {
                    if (dockedPortShipTypes[i] != -1)
                        count++;
                }
                if (c == kGridRightKey) {
                    if (selShipListItem < kGridCells - 1) {
                        if (dockedPortShipTypes[shipyardScroll + selShipListItem
                                                + 1] != -1)
                            selShipListItem++;
                    } else if (shipyardScroll < count - kGridCells) {
                        shipyardScroll += kGridCols;
                        selShipListItem -= kGridCols - 1;
                    }
                } else if (c == kGridLeftKey) {
                    if (selShipListItem > 0) {
                        selShipListItem--;
                    } else if (shipyardScroll > 0) {
                        shipyardScroll -= kGridCols;
                        selShipListItem += kGridCols - 1;
                    }
                } else if (c == kGridUpKey || c == kGridPageUpKey) {
                    if (selShipListItem >= kGridCols)
                        selShipListItem -= kGridCols;
                    else if (shipyardScroll > 0)
                        shipyardScroll -= kGridCols;
                } else if (c == kGridDownKey || c == kGridPageDownKey) {
                    if (selShipListItem < kGridCells - kGridCols) {
                        if (dockedPortShipTypes[shipyardScroll + selShipListItem
                                                + kGridCols] != -1)
                            selShipListItem += kGridCols;
                    } else if (shipyardScroll < count - kGridCells) {
                        shipyardScroll += kGridCols;
                    }
                }
                selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                while (selShipyardItem == -1 && selShipListItem > 0) {
                    selShipListItem--;
                    selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                }
            } else {
                if (c == kGridRightKey || c == kGridDownKey ||
                    c == kGridPageDownKey)
                    selShipListItem = 0;
                else
                    selShipListItem = kGridCells - 1;
                selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                while (selShipyardItem == -1 && selShipListItem > 0) {
                    selShipListItem--;
                    selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                }
            }
            if (selShipyardItem != old) {
                if (selShipyardItem != -1) {
                    if (shipBuyMode != 0)
                        LoadDescription(dialogDescTextPtr,
                                        (selShipyardItem + kHireDescBase));
                    else
                        LoadDescription(dialogDescTextPtr,
                                        (selShipyardItem + kShipDescBase));
                    RedrawDialog(shipyardDialog);
                    CalcShipCanBuy();
                    EndLoopingMovie();
                    if (selShipyardItem != -1 &&
                        shipMovieFilenames[selShipyardItem][0] != 0) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(shipMovieFilenames[selShipyardItem],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                } else {
                    canBuy = 0;
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
#if V111
        return 0;
#else
        c = tolower(EventCharOf(ev));
        return 0;
#endif
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
        if (PtInRectPoint(where, &box)) {
#ifdef CEFIX /* F13.02 */
            count = CE_GridDivide(where.h - box.left, ceGridCellWidth);
            row = CE_GridDivide(where.v - box.top, ceGridCellHeight);
#else
            count = (where.h - box.left) / kGridCellWide;
            row = (where.v - box.top) / kGridCellHigh;
#endif
            if (count >= 0 && count <= kGridCols - 1 && row >= 0 &&
                row <= kGridCells / kGridCols - 1) {
                oldPos = selShipListItem;
                selShipListItem = row * kGridCols + count;
                if (dockedPortShipTypes[selShipListItem + shipyardScroll] == -1) {
                    selShipyardItem = selShipListItem = -1;
                } else {
                    selShipyardItem =
                        dockedPortShipTypes[selShipListItem + shipyardScroll];
                }
                if (selShipListItem != oldPos) {
                    InvalDialogItem(shipyardDialog, 1);
                    InvalDialogItem(shipyardDialog, 5);
                    InvalDialogItem(shipyardDialog, 6);
                    InvalDialogItem(shipyardDialog, 8);
                    InvalDialogItem(shipyardDialog, 9);
                    InvalDialogItem(shipyardDialog, 10);
                    if (selShipListItem != -1) {
                        if (shipBuyMode != 0)
                            LoadDescription(dialogDescTextPtr,
                                            (selShipyardItem + kHireDescBase));
                        else
                            LoadDescription(dialogDescTextPtr,
                                            (selShipyardItem + kShipDescBase));
                    } else {
                        strncpy(dialogDescTextPtr, "", 3);
                    }
                    CalcShipCanBuy();
                    EndLoopingMovie();
                    if (selShipyardItem != -1 &&
                        shipMovieFilenames[selShipyardItem][0] != 0) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(shipMovieFilenames[selShipyardItem],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                }
                if (selShipListItem == lastClickItem && selShipListItem != -1 &&
                    ev->when - lastClickTime <= 15) {
                    ShipyardDialogUpdate();
                    DoShipyardDescription(selShipyardItem);
                    SetDialogGWorld(shipyardDialog);
                    RedrawDialog(shipyardDialog);
                }
                lastClickItem = selShipListItem;
                lastClickTime = ev->when;
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(shipyardDialog));
#endif
        ShipyardDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(shipyardDialog));
#endif
        return 0;
    }
    return 0;
}
#ifdef MACBUILD
#undef InvalDialogItem
#endif

/* ShipyardDialogUpdate (mac _ShipyardDialogUpdate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004948b0
// MAC: _ShipyardDialogUpdate i386 0x00058a10 ppc 0x00060d04
void ShipyardDialogUpdate(void)
{
#ifdef MACBUILD
    int c;
    short i;
    Handle item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short drawn;
    short skipped;
    long price;
    long tradeIn;
    long net;
#else
    int c;
    short drawn;
    short skipped;
    Handle item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short i;
    long price;
    long tradeIn;
    long net;
#endif

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(shipyardDialog));
    if (shipyardBackgroundPic != NULL)
        DrawPicHandle(shipyardBackgroundPic, GetDialogPortRect(shipyardDialog));
    skipped = 0;
    GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
    box.right += 1;
    box.bottom += 1;
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(blackColor);
        drawn = 0;
        for (i = shipyardScroll; i < 768; i++) {
                if (RectInRgn(&listScreenRect[i - shipyardScroll],
                              GetWindowVisRgn(FrontWindow(shipyardDialog))) &&
                    drawn < kGridCells && (i >= 0 &&
                    dockedPortShipTypes[i] != -1)) {
                    if (skipped < 3) {
                        if (i - shipyardScroll >= 0 &&
                            i - shipyardScroll < kGridCells) {
                            if (!RetrieveMenuIcon(
                                    shipClass[dockedPortShipTypes[i]].shipyardPictID,
                                    MenuIconPort(&loadPort), &listPicScreenRect[i - shipyardScroll]))
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
                        ForeColor(whiteColor);
                        MoveTo(((listScreenRect[i - shipyardScroll].left
                                        + listScreenRect[i - shipyardScroll].right
                                        - StringWidth(line1)) / 2),
                               (listScreenRect[i - shipyardScroll].top + 24));
                        DrawString(line1);
                        ForeColor(blackColor);
                    }
                    if (SplitTwoLineString(shipyardShortNames[dockedPortShipTypes[i]],
                                           line1, line2)) {
                        LineLeadChar(line1);
                        if (LineLeadsAlnum(line1))
                            ForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
#ifdef CEFIX /* F12.56 */
                        CE_SetPen((listScreenRect[i - shipyardScroll].left
                                       + listScreenRect[i - shipyardScroll].right) / 2
                                      - StringWidth(line1) / 2,
                                  listScreenRect[i - shipyardScroll].bottom - CE_Scale(14));
#else
                        MoveTo(((listScreenRect[i - shipyardScroll].left
                                        + listScreenRect[i - shipyardScroll].right) / 2
                                       - StringWidth(line1) / 2),
                               (listScreenRect[i - shipyardScroll].bottom - 14));
#endif
                        DrawString(line1);
                        LineLeadChar(line2);
                        if (LineLeadsAlnum(line2))
                            ForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
#ifdef CEFIX /* F12.57 */
                        CE_SetPen((listScreenRect[i - shipyardScroll].left
                                       + listScreenRect[i - shipyardScroll].right) / 2
                                      - StringWidth(line2) / 2,
                                  listScreenRect[i - shipyardScroll].bottom - CE_Scale(3));
#else
                        MoveTo(((listScreenRect[i - shipyardScroll].left
                                        + listScreenRect[i - shipyardScroll].right) / 2
                                       - StringWidth(line2) / 2),
                               (listScreenRect[i - shipyardScroll].bottom - 3));
#endif
                        DrawString(line2);
                        ForeColor(blackColor);
                    } else {
#ifdef CEFIX /* F12.58 */
                        CE_SetPen((listScreenRect[i - (long)shipyardScroll].left
                                       + listScreenRect[i - (long)shipyardScroll].right) / 2
                                      - StringWidth(line1) / 2,
                                  listScreenRect[i - (long)shipyardScroll].bottom - CE_Scale(6));
#else
                        MoveTo(((listScreenRect[i - (long)shipyardScroll].left
                                        + listScreenRect[i - (long)shipyardScroll].right) / 2
                                       - StringWidth(line1) / 2),
                               (listScreenRect[i - (long)shipyardScroll].bottom - 6));
#endif
                        ForeColor(whiteColor);
                        DrawString(line1);
                        ForeColor(blackColor);
                    }
                    RGBForeColor(&shopGridColor[1]);
                    FrameRect(&listScreenRect[i - shipyardScroll]);
                    ForeColor(blackColor);
                    drawn++;
                }
        }
        if (selShipListItem != -1) {
            RGBForeColor(&shopGridColor[0]);
            FrameRect(&listScreenRect[selShipListItem]);
            ForeColor(blackColor);
        }
    }
    if (selShipyardItem != -1) {
        GetDialogItem(shipyardDialog, kMovieItem, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
            if (loopingMovie != NULL) {
                CopyBits(GetPortBitMapForCopyBits(
                             GetDialogPort(shipyardDialog)),
                         (BitMapPtr)GWorldContainerBits(&loadPort), &box, &box, 0,
                         GetWindowVisRgn(FrontWindow(shipyardDialog)));
            } else {
                if (previewPicID != shipClass[selShipyardItem].shipyardPictID) {
                    if (previewPic != NULL) {
#ifdef MACBUILD
                        HUnlock((Handle)previewPic);
#endif
                        QDReleaseResource(previewPic);
                        previewPic = NULL;
                    }
                    previewPicID = shipClass[selShipyardItem].shipyardPictID;
#ifdef MACBUILD
                    if (previewPicID >= kFirstResourceID) {
                        previewPic = GetPicture(previewPicID);
                        if (previewPic != NULL)
                            HLock((Handle)previewPic);
                    }
#else
                    if (previewPicID >= kFirstResourceID)
                        previewPic = GetPicture(previewPicID);
#endif
                }
                if (previewPic != NULL) {
                    ForeColor(blackColor);
                    DrawPicHandle(previewPic, &box);
                } else {
                    ForeColor(whiteColor);
                    TextFont(smallTextFont);
                    TextSize(smallTextSize);
                    GetIndString(line1, kGameStrList, 0xd5);
                    DrawStringCentered(line1, box.left, box.right,
                                       ((box.bottom + box.top) / 2 - 6));
                    GetIndString(line1, kGameStrList, 0xd6);
                    DrawStringCentered(line1, box.left, box.right,
                                       ((box.bottom + box.top) / 2 + 6));
                    ForeColor(blackColor);
                }
            }
        }
        GetDialogItem(shipyardDialog, 9, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            if (shipBuyMode == 0 && selShipyardItem != shipData->type) {
                ForeColor(whiteColor);
                MoveTo(box.left, (box.top + 12));
                DrawIndString(kGameStrList, 0xe1);
                MoveTo((box.left + 0x46), (box.top + 12));
                price = ApplyPriceAndTechnologyFlux(
                    shipClass[selShipyardItem].cost, priceFluxMult[1],
                    shipClass[selShipyardItem].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(price);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x18));
                DrawIndString(kGameStrList, 0xe2);
                MoveTo((box.left + 0x46), (box.top + 0x18));
                net = tradeIn = ApplyPriceAndTechnologyFlux(
                    PlayerShipTradeInPrice(), priceFluxMult[0],
                    shipClass[shipData->type].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                    net, priceFluxMult[1],
                    shipClass[shipData->type].techLevel,
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
                ForeColor(blackColor);
            } else if (shipBuyMode != 0) {
                ForeColor(whiteColor);
                MoveTo(box.left, (box.top + 12));
                DrawIndString(kGameStrList, 0xe4);
                MoveTo((box.left + 0x46), (box.top + 12));
                tradeIn = ApplyPriceAndTechnologyFlux(
                    shipClass[selShipyardItem].cost, priceFluxMult[1],
                    shipClass[selShipyardItem].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(0.1 * tradeIn);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x24));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x24));
                DrawCommaNumber(shipData->credits);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                ForeColor(blackColor);
            }
        }
    }
    GetDialogItem(shipyardDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(dialogDescTextPtr, strlen(dialogDescTextPtr), &box, 0);
        InvertRect(&box);
    }
    DrawShipyardButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(shipyardDialog);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(shipyardDialog)),
             GetDialogPortRect(shipyardDialog),
             GetDialogPortRect(shipyardDialog), 0,
             GetWindowVisRgn(FrontWindow(shipyardDialog)));
}


#define kLabelStrList 150
#define kLabelStrIdx  5



#define kDescDlogID      0x3fb   /* 1019: the layout with a picture well */
#define kPlainDescDlogID 0x3ed   /* 1005: the layout without one */
#define kDescPictID      8507    /* backdrop to match each layout */
#define kPlainDescPictID 8506

/* DoShipyardDescription (mac _DoShipyardDescription). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004956a0
// MAC: _DoShipyardDescription i386 0x0005b6d2 ppc 0x00063764
void DoShipyardDescription(short ship)
{
    Boolean done;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short i;
    short shown;

    done = 0;
    shipDescDialog = NULL;
    bigShipPic = NULL;
    if (descResourcePictId >= kFirstResourceID)
        bigShipPic = GetPicture(descResourcePictId);
    if (bigShipPic != NULL)
        shipDescDialog = GetNewDialog(kDescDlogID, ShipDescDialogUpdate, ShipDescFilter);
    else
        shipDescDialog = GetNewDialog(kPlainDescDlogID, ShipDescDialogUpdate,
                                ShipDescFilter);
    if (shipDescDialog == NULL)
        return;
    if (descResourcePictId >= kFirstResourceID)
        shipDescBackgroundPic = GetPicture(kDescPictID);
    else
        shipDescBackgroundPic = GetPicture(kPlainDescPictID);
    CenterWindow(FrontWindow(shipDescDialog), FrontWindow(portDialog));
    ShowWindowNoFlash(FrontWindow(shipDescDialog));
    SetDialogGWorld(shipDescDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    RedrawDialog(shipDescDialog);
    ShipDescDialogUpdate();
    SelectWindow(FrontWindow(shipDescDialog));
    BringToFront(FrontWindow(shipDescDialog));
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(shipDescDialog, ShipDescFilter, &itemHit, 0);
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
            PortDialogUpdate();
            SetDialogGWorld(shipDescDialog);
            RedrawDialog(shipDescDialog);
        }
        if (itemHit == 2) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            PortDialogUpdate();
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
                PortDialogUpdate();
                SetDialogGWorld(shipDescDialog);
                RedrawDialog(shipDescDialog);
            }
        }
    } while (done == 0);
    DisposeDialogNoFlash(shipDescDialog);
    shipDescDialog = NULL;
    if (shipDescBackgroundPic != NULL) {
        QDReleaseResource(shipDescBackgroundPic);
        shipDescBackgroundPic = NULL;
    }
    if (bigShipPic != NULL)
        QDReleaseResource(bigShipPic);
    bigShipPic = NULL;
#ifdef MACBUILD
    RefreshSAT();
#endif
}

/* ShipDescFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004959d0
// MAC: _ShipDescFilter i386 0x000510e5 ppc 0x00059a0c
Boolean ShipDescFilter(long unused, EventRecord *ev, short *itemHit)
{
    short itemType;
    void *item;
    unsigned char label[256];
    Rect box;
    Point where;
    short lastHit;
    short hit;
#if V111
    short key;
#else
    EventChar c;
#endif

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
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode
                || key == kSpaceKeyCode) {
#else
        c = EventCharOf(ev);
        c = tolower(c);
#ifdef CEFIX /* F19.02 */
        if (IsReturnChar(c) || c == kEscapeChar || c == ' ') {
#else
        if (IsReturnChar(c) || c == kEscapeChar) {
#endif
#endif
            ev->what = kMouseDown;
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(shipDescDialog));
#endif
        DrawDialog(shipDescDialog);
        ShipDescDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(shipDescDialog));
#endif
        return 0;
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        GetDialogItem(shipDescDialog, 1, &itemType, (Handle *)&item, &box);
        if (PtInRectPoint(where, &box)) {
            GetIndString(label, kLabelStrList, kLabelStrIdx);
            DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 1, 0);
#ifndef MACBUILD
            nv_UpdateDialogWindow(0);
#endif
            lastHit = 1;
            while (StillDown()) {
                BusyIdle();
                GetMouse(&where);
                if (PtInRectPoint(where, &box))
                    hit = 1;
                else
                    hit = -1;
                if (hit != lastHit) {
                    if (hit == 1)
                        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 1, 0);
                    else
                        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 0, 0);
#ifndef MACBUILD
                    nv_UpdateDialogWindow(0);
#endif
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
// MAC: _ShipDescDialogUpdate i386 0x0004f915 ppc 0x00058710
void ShipDescDialogUpdate(void)
{
#ifdef MACBUILD
    short numWeapTypes;
    short drawn;
    short itemType;
    short sel;
    Rect box;
    Boolean hasPict;
    char line[1024];
    unsigned char buf[256];
    Rect box2;
    Handle item;
    short i;
    short j;
    long freeMass;
    long mass;
#else
    long drawn;
    long numWeapTypes;
    short itemType;
    short sel;
    long lineV;
    Rect box2;
    Boolean hasPict;
    char line[1024];
    unsigned char buf[256];
    Rect box;
    Handle item;
    short i;
    short j;
    short jumps;
    long freeMass;
    long mass;
#endif

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(shipDescDialog));
    if (shipDescBackgroundPic != NULL)
        DrawPicHandle(shipDescBackgroundPic, GetDialogPortRect(shipDescDialog));
    if (descResourcePictId >= 128)
        hasPict = 1;
    else
        hasPict = 0;
    ForeColor(blackColor);
    GetDialogItem(shipDescDialog, 3, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        TextFont(gTimesFont);
        TextSize(18);
        MoveTo(box.left, (box.top + 18));
        ForeColor(whiteColor);
#ifdef CEFIX /* F12.59 */
        if (hasPict)
            DrawStringCentered(shipyardLongNames[selShipyardItem], box.left,
                               box.right, box.top + CE_Scale(18));
        else
            DrawStringCentered(shipClass[selShipyardItem].name, box.left,
                               box.right, box.top + CE_Scale(18));
#else
        if (hasPict)
            DrawStringCentered(shipyardLongNames[selShipyardItem], box.left,
                               box.right, (box.top + 18));
        else
            DrawStringCentered(shipClass[selShipyardItem].name, box.left,
                               box.right, (box.top + 18));
#endif
        ForeColor(blackColor);
    }
    GetDialogItem(shipDescDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        sel = selShipyardItem;
        ForeColor(blackColor);
        PaintRect(&box);
        ForeColor(whiteColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 12));
        DrawIndString(kGameStrList, 229);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 12));
#ifdef CEFIX /* F29.01 */
        NumToString(CE_RoundD(shipClass[sel].maxSpeed * 100.0), buf);
#else
        NumToString((shipClass[sel].maxSpeed * 100.0), buf);
#endif
        DrawString(buf);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 24));
        DrawIndString(kGameStrList, 230);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 24));
        if (shipClass[sel].accel > 0.073)
            DrawIndString(kGameStrList, 245);
        else if (shipClass[sel].accel > 0.055)
            DrawIndString(kGameStrList, 246);
        else if (shipClass[sel].accel > 0.038)
            DrawIndString(kGameStrList, 247);
        else if (shipClass[sel].accel > 0.025)
            DrawIndString(kGameStrList, 248);
        else if (shipClass[sel].accel > 0.013)
            DrawIndString(kGameStrList, 249);
        else if (shipClass[sel].accel > 0.0)
            DrawIndString(kGameStrList, 250);
        else
            DrawIndString(kGameStrList, 396);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 36));
        DrawIndString(kGameStrList, 231);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 36));
        if (shipClass[sel].turnRate <= 0.0f)
            DrawIndString(kGameStrList, 396);
        else if (shipClass[sel].turnRate <= 1.0f)
            DrawIndString(kGameStrList, 250);
        else if (shipClass[sel].turnRate <= 2.0f)
            DrawIndString(kGameStrList, 249);
        else if (shipClass[sel].turnRate <= 3.0f)
            DrawIndString(kGameStrList, 248);
        else if (shipClass[sel].turnRate <= 4.0f)
            DrawIndString(kGameStrList, 247);
        else if (shipClass[sel].turnRate <= 5.0f)
            DrawIndString(kGameStrList, 246);
        else
            DrawIndString(kGameStrList, 245);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 48));
        DrawString(shieldString[0]);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 48));
        if (shipClass[sel].shield > 0)
            DrawCommaNumber(shipClass[sel].shield);
        else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 60));
        DrawString(armorString[0]);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 60));
        if (shipClass[sel].armor > 0)
            DrawCommaNumber(shipClass[sel].armor);
        else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 72));
        DrawIndString(kGameStrList, 232);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 72));
        if (shipClass[sel].maxGuns > 0) {
            NumToString(shipClass[sel].maxGuns, buf);
            DrawIndString(kGameStrList, 238);
            DrawString("\p ");
            DrawString(buf);
        } else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 84));
        DrawIndString(kGameStrList, 233);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 84));
        if (shipClass[sel].maxTurrets > 0) {
            NumToString(shipClass[sel].maxTurrets, buf);
            DrawIndString(kGameStrList, 238);
            DrawString("\p ");
            DrawString(buf);
        } else
            DrawIndString(kGameStrList, 335);

        freeMass = shipClass[sel].freeMass;
        for (i = 0; i < 256; i++) {
            if (shipClass[sel].weapCount[i] > 0) {
                for (j = 0; j < 512; j++) {
                    if (outfitSystem[j].modType[0] == 1
                        && i == outfitSystem[j].modVal[0]
                        && outfitSystem[j].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&outfitSystem[j], &shipClass[sel]);
                        mass = shipClass[sel].weapCount[i] * mass;
                        if (mass > freeMass)
                            freeMass = 0;
                        else
                            freeMass -= mass;
                        break;
                    }
                }
            }
            if (shipClass[sel].weapAmmo[i] > 0) {
                for (j = 0; j < 512; j++) {
                    if (outfitSystem[j].modType[0] == 3
                        && i == outfitSystem[j].modVal[0]
                        && outfitSystem[j].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&outfitSystem[j], &shipClass[sel]);
                        mass = shipClass[sel].weapAmmo[i] * mass;
                        if (mass > freeMass)
                            freeMass = 0;
                        else
                            freeMass -= mass;
                        break;
                    }
                }
            }
        }
        for (i = 0; i < 8; i++) {
            if (shipClass[sel].defOutfID[i] != -1
                && shipClass[sel].defOutfCount[i] > 0
                && outfitSystem[shipClass[sel].defOutfID[i]].techLevel < kOutfTechAbsent) {
                mass = AdjustedItemMass(&outfitSystem[shipClass[sel].defOutfID[i]],
                                        &shipClass[sel]);
                mass = shipClass[sel].defOutfCount[i] * mass;
                if (mass > freeMass)
                    freeMass = 0;
                else
                    freeMass -= mass;
            }
        }

        MoveTo((box.left + 130), (box.top + 12));
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 234);
        ForeColor(whiteColor);
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
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 24));
        NumToString(shipClass[sel].cargo, buf);
        DrawString(buf);
        DrawString("\p ");
        if (shipClass[sel].cargo == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 36));
        DrawString(fuelString[4]);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 36));
        if (shipClass[sel].fuel == 0)
            DrawIndString(kGameStrList, 335);
        else {
#ifdef MACBUILD
            numWeapTypes = shipClass[sel].fuel / 100;
            NumToString(numWeapTypes, buf);
            DrawString(buf);
            DrawString("\p ");
            if (numWeapTypes == 1)
#else
            jumps = shipClass[sel].fuel / 100;
            NumToString(jumps, buf);
            DrawString(buf);
            DrawString("\p ");
            if (jumps == 1)
#endif
                DrawIndString(kGameStrList, 239);
            else
                DrawIndString(kGameStrList, 240);
        }

        numWeapTypes = 0;
        for (i = 0; i < 256; i++) {
            if (shipClass[sel].weapCount[i] > 0)
                numWeapTypes++;
        }
        if ((short)numWeapTypes == 0) {
            ForeColor(whiteColor);
            if (hasPict) {
                GetDialogItem(shipDescDialog, 8, &itemType, &item, &box2);
                MoveTo(box2.left, (box2.top + 12));
            } else {
                MoveTo(box.left, (box.top + 108));
            }
            DrawIndString(kGameStrList, 242);
        } else {
            ForeColor(whiteColor);
            drawn = 0;
            if (hasPict) {
                strcpy(line, "");
                GetDialogItem(shipDescDialog, 8, &itemType, &item, &box2);
                MoveTo(box2.left, (box2.top + 12));
                box2.top += 16;
            } else {
                MoveTo(box.left, (box.top + 108));
            }
            DrawIndString(kGameStrList, 241);
            for (i = 0; i < 256; i++) {
                if (shipClass[sel].weapCount[i] > 0) {
                    if (!hasPict)
                        MoveTo(box.left + 8, box.top + 120 + drawn * 12);
                    if (weaponStats[i].guidance != kGuidanceBay) {
                        NumToString(shipClass[sel].weapCount[i], buf);
                        if (hasPict) {
                            PtoCstr(buf);
                            strcat(line, (char *)buf);
                            strcat(line, " ");
                        } else {
                            DrawString(buf);
                            DrawString("\p ");
                        }
                        for (j = 0; j < 512; j++) {
                            if (outfitSystem[j].modType[0] == 1
                                && i == outfitSystem[j].modVal[0]) {
                                if (hasPict) {
                                    pstrcpy(buf, outfitSystem[j].name);
                                    PtoCstr(buf);
                                    strcat(line, (char *)buf);
                                } else {
                                    DrawString(outfitSystem[j].name);
                                }
                                break;
                            }
                        }
                        if (shipClass[sel].weapCount[i] > 1) {
                            if (hasPict)
                                strcat(line, "s");
                            else
                                DrawString("\ps");
                        }
                        if (shipClass[sel].weapAmmo[i] > 0) {
                            NumToString(shipClass[sel].weapAmmo[i], buf);
                            if (hasPict) {
                                strcat(line, " + ");
                                PtoCstr(buf);
                                strcat(line, (char *)buf);
                                strcat(line, " ");
                                StrcatIndCString(line, kGameStrList, 243);
                            } else {
                                DrawString("\p + ");
                                DrawString(buf);
                                DrawString("\p ");
                                DrawIndString(kGameStrList, 243);
                            }
                        }
                    } else {
                        NumToString(shipClass[sel].weapAmmo[i], buf);
                        if (hasPict) {
                            PtoCstr(buf);
                            strcat(line, (char *)buf);
                            strcat(line, " ");
                        } else {
                            DrawString(buf);
                            DrawString("\p ");
                        }
                        for (j = 0; j < 512; j++) {
                            if (outfitSystem[j].modType[0] == 3
                                && i == outfitSystem[j].modVal[0]) {
                                if (hasPict) {
                                    pstrcpy(buf, outfitSystem[j].name);
                                    PtoCstr(buf);
                                    strcat(line, (char *)buf);
                                } else {
                                    DrawString(outfitSystem[j].name);
                                }
                                break;
                            }
                        }
                        if (shipClass[sel].weapAmmo[i] > 1) {
                            if (hasPict)
                                strcat(line, "s");
                            else
                                DrawString("\ps");
                        }
                    }
                    if (hasPict && (short)numWeapTypes > 1
                        && (short)drawn < (short)numWeapTypes - 1)
                        strcat(line, ", ");
                    drawn++;
                }
            }
            if (hasPict) {
                ForeColor(blackColor);
                TETextBox(line, strlen(line), &box2, 0);
                InvertRect(&box2);
            }
        }

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 48));
        DrawIndString(kGameStrList, 235);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 48));
        NumToString(shipClass[sel].shipLength, buf);
        DrawString(buf);
        DrawString("\p ");
        DrawIndString(kGameStrList, 244);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 60));
        DrawIndString(kGameStrList, 236);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 60));
        NumToString(shipClass[sel].mass, buf);
        DrawString(buf);
        DrawString("\p ");
        if (shipClass[sel].mass == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        if (shipClass[sel].crew > 0) {
            RGBForeColor(&rgbMedGrey);
            MoveTo((box.left + 130), (box.top + 72));
            DrawIndString(kGameStrList, 237);
            ForeColor(whiteColor);
            MoveTo((box.left + 175), (box.top + 72));
            NumToString(shipClass[sel].crew, buf);
            DrawString(buf);
        }
    }
    GetDialogItem(shipDescDialog, 1, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        GetIndString(buf, kButtonStrList, 5);
        DrawButton(GetDialogPort(shipDescDialog), &box, buf, 0, 0, 0);
    }
    if (bigShipPic != NULL) {
        GetDialogItem(shipDescDialog, 7, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog))))
            DrawPicHandle(bigShipPic, &box);
    }
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(shipDescDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(shipDescDialog)),
             GetDialogPortRect(shipDescDialog), GetDialogPortRect(shipDescDialog), 0, 0);
}

/* UpdateAllPortDialogs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497430
// MAC: _UpdateAllPortDialogs i386 0x000598a6 ppc 0x000619d8
void UpdateAllPortDialogs(void)
{
    if (portDialog != NULL) {
        SetDialogGWorld(portDialog);
        PortDialogUpdate();
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
        if (missionsOutside != 0)
            SingleMissionDialogUpdate();
        else if (portDialog != NULL)
            MissionDialogUpdate();
    }
#ifndef MACBUILD
    if (portDialog != NULL || shipyardDialog != NULL || outfitDialog != NULL
            || missionDialog != NULL)
        nv_UpdateDialogWindow(1);
#endif
}

#define kFileMenu 128

/* IterateInGameEventLoop (mac _IterateInGameEventLoop). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497500
// MAC: _IterateInGameEventLoop i386 0x00056859 ppc 0x0005ebe0
void IterateInGameEventLoop(short tickCount)
#ifdef MACBUILD
{
    EventRecord ev;
    WindowPtr window;
    Rect dragBounds;
    Str255 str;
    short i;
    Boolean exitLoop;
    Boolean gotEvent;
    long menuChoice;
    Boolean depthMismatch;
#if !TARGET_API_MAC_CARBON
    Point where;
#endif

    exitLoop = false;
    i = 0;
    while (!exitLoop) {
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        HandleIntroMusic(0);
#endif
        gotEvent = WaitNextEvent(everyEvent, &ev, (gameLoaded && inBackground) ? 30 : 0, NULL);
        i++;
        if (!gotEvent && ev.what != nullEvent)
            continue;
        switch (ev.what) {
        case kHighLevelEvent:
            AEProcessAppleEvent(&ev);
            break;
        case mouseDown:
            switch (FindWindow(ev.where, &window)) {
            case inDrag:
                if (gameInProgress && gameLoaded)
                    RefreshSAT();
                if (window == (WindowPtr)gWind) {
                    if (useLegacyRenderingPipeline) {
                        GetWindowDragBounds(&dragBounds, ev.where);
                        if (dragBounds.right > dragBounds.left && dragBounds.bottom > dragBounds.top) {
                            DragWindow(window, ev.where, &dragBounds);
                            ForceWindowOnscreen();
                            SWWindowMoved(gSpriteWorld);
                            if (gameInProgress && gameLoaded)
                                RefreshSAT();
                        }
                    } else {
                        DragWindow(window, ev.where, NULL);
                        ForceWindowOnscreen();
                        SWWindowMoved(gSpriteWorld);
                        if (gameInProgress && gameLoaded)
                            RefreshSAT();
                    }
                }
                break;
            case inMenuBar:
                InitCursor();
                cursorHidden = 0;
                ShowTheCursor();
                AdjustMenus();
                DoMenuClick(MenuSelect(ev.where));
                break;
            case inGoAway:
                if (gameLoaded) {
                    if ((FrontWindow)() == (WindowRef)gWind && !screenFaded) {
                        ShowTheCursor();
                        GetIndString(str, kAlertStrList, 10);
                        if (EVYesNoDialog(str))
                            gQuit = 1;
                        else
                            RedrawStatus(1);
                    }
                } else {
                    SysBeep(0);
                }
                break;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
            case inSysWindow:
                SystemClick(&ev, window);
                break;
#endif
            }
            break;
        case keyDown:
        case autoKey:
            if (ev.modifiers & cmdKey) {
                AdjustMenus();
#ifdef MAC1010
                menuChoice = MenuKey(ev.message);
                if ((short)(menuChoice >> 16) == kFileMenu && (short)menuChoice == 1 && gameInProgress && gameLoaded) {
                    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                    ShowTheCursor();
                    HideCursorSprite();
                    RefreshSAT();
                    if (!screenFaded) {
                        GetIndString(str, kAlertStrList, 10);
                        if (EVYesNoDialog(str))
                            gQuit = 1;
                        else
                            RedrawStatus(1);
                    }
                    RefreshSAT();
                }
#else
                MenuKey(ev.message);
#endif
            }
            break;
        case updateEvt:
            depthMismatch = 0;
            if (useLegacyRenderingPipeline && gSpriteWorld->mainSWGDH != NULL
                    && (*(*(GDHandle)gSpriteWorld->mainSWGDH)->gdPMap)->pixelSize
                        != gSpriteWorld->pixelDepth)
                depthMismatch = 1;
            if (depthMismatch)
                break;
            SWSetPortToWindow(gSpriteWorld);
            BeginUpdate((WindowRef)gWind);
            if (!creatingPilotFile) {
                if (gameInProgress && gameLoaded) {
                    if (onPlanet || inBackground) {
                        if (useLegacyRenderingPipeline) {
                            RedrawStatus(1);
                            RefreshSAT();
                            RedrawComm(1);
                        } else {
                            RedrawStatus(0);
                            RefreshSAT();
                            RedrawComm(0);
                        }
                    }
                } else {
                    CopyBits(GWorldContainerBits(&loadPort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                             GetCWindowPortRect((WindowRef)gWind), GetCWindowPortRect((WindowRef)gWind), 0, NULL);
                }
            }
            SWSetPortToWindow(gSpriteWorld);
            EndUpdate((WindowRef)gWind);
            SWSetPortToWindow(gSpriteWorld);
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
            ValidRect(GetCWindowPortRect((WindowRef)gWind));
#else
            ValidWindowRect(gWind, GetWindowPortRect(gWind));
#endif
            break;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        case diskEvt:
            if ((long)ev.message >> 16) {
                where.v = where.h = 100;
                DILoad();
                DIBadMount(where, ev.message);
                DIUnload();
            }
            break;
#endif
        case osEvt:
            if (ev.message & (suspendResumeMessage << 24)) {
                if (ev.message & resumeFlag) {
#ifdef MAC1010
                    HideTheMenuBar();
#endif
                    inBackground = 0;
                    InitCursor();
                    cursorHidden = 0;
                    ShowTheCursor();
                    if (gSystemVersion < 0x1000 && mBarHidden && SWIsWindowInForeground(gSpriteWorld))
                        KeepMenuBarHidden((GrafPtr)gSpriteWorld->windowFrameP->framePort);
                    SwitchedIn(&ev);
                    if (gameLoaded) {
                        RefreshSAT();
                        RedrawStatus(1);
                    }
                } else {
                    inBackground = 1;
                    InitCursor();
                    cursorHidden = 0;
                    ShowTheCursor();
                    SwitchedOut(&ev);
                    if (gameInProgress && gameLoaded)
                        RefreshSAT();
                }
            }
            break;
        case nullEvent:
            if (gSystemVersion < 0x1000 && mBarHidden && SWIsWindowInForeground(gSpriteWorld))
                KeepMenuBarHidden((GrafPtr)gSpriteWorld->windowFrameP->framePort);
            exitLoop = true;
            break;
        }
        if (tickCount > 0 && i >= tickCount)
            exitLoop = true;
    }
}
#else
{
    Boolean exitLoop;
    Boolean depthMismatch;
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
                        CopyBits(GWorldContainerBits(&loadPort),
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
                GetIndString(str, kAlertStrList, 10);
                if (EVYesNoDialog(str))
                    gQuit = 1;
            }
        }
        if (tickCount > 0 && i >= tickCount)
            exitLoop = 1;
    } while (!exitLoop);
}
#endif

/* the port's body takes no arguments and answers the single window; every
   caller still passes the dialog, so it is declared that way here */

/* EVNoteAlert (mac _EVNoteAlert). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004976b0
// MAC: _EVNoteAlert i386 0x00055710 ppc 0x0005d994
void EVNoteAlert(const unsigned char *text)
{
    Boolean done;
    Rect box;
    short itemHit;
    short itemType;
    Handle item;
#ifdef MACBUILD
    Str255 str;
#endif

    done = 0;
    alertDialog = NULL;
    alertDialog = GetNewDialog(3000, NULL, NULL);
    if (alertDialog == NULL)
        return;
    SetDialogGWorld(alertDialog);
    SetDialogDefaultItem(alertDialog, 1);
    ShowMacWindow(FrontWindow(alertDialog));
    SetDialogGWorld(alertDialog);
    SelectWindow(FrontWindow(alertDialog));
    SetDialogGWorld(alertDialog);
    DrawDialog(alertDialog);
    FrameDefaultButton(alertDialog, 1);
#ifdef MACBUILD
    strncpy((char *)str, (const char *)text, 254);
#endif
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
#ifdef MACBUILD
    SetDialogItemText(item, str);
#else
    SetDialogItemText((Handle)item,text);
#endif
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == 1)
            done = 1;
    } while (done == 0);
#ifdef MACBUILD
    HideWindow(GetDialogWindow(alertDialog));
#endif
    DisposeDialog(alertDialog);
    alertDialog = NULL;
}

/* EVYesNoDialog (mac _EVYesNoDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004977d0
// MAC: _EVYesNoDialog i386 0x0005558c ppc 0x0005d84c
Boolean EVYesNoDialog(unsigned char *text)
{
    Boolean done;
    Boolean result;
    Rect box;
    short itemHit;
    short itemType;
    Handle item;
#ifdef MACBUILD
    Str255 str;
#endif

    done = 0;
    alertDialog = NULL;
    alertDialog = GetNewDialog(3002, NULL, NULL);
    if (alertDialog == NULL)
        return 0;
    SetDialogDefaultItem(alertDialog, 1);
    ShowMacWindow(FrontWindow(alertDialog));
    SelectWindow(FrontWindow(alertDialog));
    SetDialogGWorld(alertDialog);
    FrameDefaultButton(alertDialog, 1);
#ifdef MACBUILD
    strncpy((char *)str, (const char *)text, 254);
#endif
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
#ifdef MACBUILD
    SetDialogItemText(item, str);
#else
    SetDialogItemText((Handle)item,text);
#endif
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
#ifdef MACBUILD
    HideWindow(GetDialogWindow(alertDialog));
#endif
    DisposeDialog(alertDialog);
    alertDialog = NULL;
    SWSetPortToWindow(gSpriteWorld);
    return result;
}

/* EVTextInputDialog (mac _EVTextInputDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497900
// MAC: _EVTextInputDialog i386 0x000552a3 ppc 0x0005d610
Boolean EVTextInputDialog(unsigned char *prompt, unsigned char *initial, short maxLen)
{
    Boolean result;
    Boolean done;
    Str255 text;
    short itemHit;
    Rect box;
    Handle item;
    short itemType;
#ifdef MACBUILD
    short len;
#endif

    done = 0;
    alertDialog = NULL;
    alertDialog = GetNewDialog(3001, NULL, NULL);
    if (alertDialog == NULL)
        return 0;
    SetDialogDefaultItem(alertDialog, 1);
    ShowMacWindow(FrontWindow(alertDialog));
    SelectWindow(FrontWindow(alertDialog));
    SetDialogGWorld(alertDialog);
    FrameDefaultButton(alertDialog, 1);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((Handle)item,prompt);
#ifdef MACBUILD
    strncpy((char *)text, (const char *)initial, 254);
    GetDialogItem(alertDialog, 5, &itemType, &item, &box);
    SetDialogItemText(item, text);
#else
    GetDialogItem(alertDialog, 5, &itemType, &item, &box);
    SetDialogItemText((Handle)item,initial);
#endif
    SelectDialogItemText(alertDialog, 5, 0, 254);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        ModalDialog(NULL, &itemHit);
        if (itemHit == 1) {
            GetDialogItem(alertDialog, 5, &itemType, &item, &box);
            GetDialogItemText((Handle)item,text);
#ifdef MACBUILD
            PtoCstr(text);
            len = strlen((char *)text);
            if (len <= maxLen) {
#else
            if (text[0] <= maxLen) {
#endif
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
    GetDialogItemText((Handle)item,textInputString);
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(alertDialog);
    alertDialog = NULL;
    SWSetPortToWindow(gSpriteWorld);
    return result;
}

#define kPictCacheSlots 128


/* RetrieveMenuIcon. STATE: MATCH 100.0%, exe 122 / obj 122, byte-identical yes (376 bytes),
   relocregcheck 0 of 29. OPEN: none. The miss-path rect address is held in `j', a later web
   of the scan counter (a split web is numbered after the CSEs; a Rect * local is band 1
   and takes ESI before GetPicture's argument), with `oldest' declared before `j'. `j' is a
   long: a 16-bit-int Mac compiler carried an address in a long, never an int. */
// FUNCTION: EVNOVA 0x00497b70
// MAC: _RetrieveMenuIcon i386 0x0004f425 ppc 0x000580cc
#ifdef MACBUILD
Boolean RetrieveMenuIcon(short picID, CGrafPtr dest, Rect *destRect)
{
    short i;
    short slot;
    unsigned long oldest;
    PicHandle pic;
    CGrafPtr savedPort;
    Boolean missed;

    slot = -1;
    for (i = 0; i < kPictCacheSlots; i++) {
        if (iconCache[i].picID == -1)
            slot = i;
        if (iconCache[i].picID == picID) {
            CopyBits(GetPortBitMapForCopyBits(iconsPort.world), GetPortBitMapForCopyBits(dest),
                     &listPicOffRect[i], destRect, 0, NULL);
            iconCache[i].stamp = TickCount();
            return true;
        }
    }
    if (slot == -1) {
        oldest = iconCache[0].stamp;
        for (i = 0; i < kPictCacheSlots; i++) {
            if (iconCache[i].stamp < oldest) {
                oldest = iconCache[i].stamp;
                slot = i;
            }
        }
    }
    if (slot < 0 || slot >= kPictCacheSlots)
        slot = 0;
    GetPort((GrafPtr *)&savedPort);
    SetPort((GrafPtr)iconsPort.world);
    ForeColor(blackColor);
    pic = GetPicture(picID);
    if (pic != NULL) {
        DrawPicHandle(pic, &listPicOffRect[slot]);
        QDReleaseResource(pic);
        iconCache[slot].picID = picID;
        iconCache[slot].stamp = TickCount();
        missed = false;
    } else {
        PaintRect(&listPicOffRect[slot]);
        missed = true;
    }
    SetPort((GrafPtr)savedPort);
    CopyBits(GetPortBitMapForCopyBits(iconsPort.world), GetPortBitMapForCopyBits(dest),
             &listPicOffRect[slot], destRect, 0, NULL);
    return missed;
}
#else
Boolean RetrieveMenuIcon(short picID, GWorldContainer *dest, Rect *destRect)
{
    int slot;
    int i;
    PicHandle pic;
    CGrafPtr savedPort;
    Boolean missed;
    unsigned long oldest;
    long j;
    short id;

    slot = -1;
    i = 0;
    do {
        id = iconCache[(short)i].picID;
        if (id == -1)
            slot = i;
        if (picID == id) {
            CopyBits((BitMapPtr)GetGWCPixMap(&iconsPort), (BitMapPtr)GetGWCPixMap(dest),
                     &listPicOffRect[(short)i], destRect, 0, 0);
            iconCache[(short)i].stamp = ReadTick();
            return 1;
        }
        i = i + 1;
    } while ((short)i < kPictCacheSlots);
    if ((short)slot == -1) {
        oldest = iconCache[0].stamp;
        j = 0;
        do {
            if (iconCache[(short)j].stamp < oldest) {
                oldest = iconCache[(short)j].stamp;
                slot = j;
            }
            j = j + 1;
        } while ((short)j < kPictCacheSlots);
    }
    if ((short)slot < 0 || kPictCacheSlots <= (short)slot)
        slot = 0;
    GetPort(&savedPort);
    SetPort(iconsPort.world);
    ForeColor(blackColor);
    pic = GetPicture(picID);
    if (pic != 0) {
        j = (long)&listPicOffRect[(short)slot];
        DrawPicHandle(pic, (Rect *)j);
        QDReleaseResource(pic);
        iconCache[(short)slot].picID = picID;
        iconCache[(short)slot].stamp = ReadTick();
        missed = 0;
    } else {
        j = (long)&listPicOffRect[(short)slot];
        PaintRect((Rect *)j);
        missed = 1;
    }
    SetPort(savedPort);
    CopyBits((BitMapPtr)GetGWCPixMap(&iconsPort), (BitMapPtr)GetGWCPixMap(dest),
             (Rect *)j, destRect, 0, 0);
    return missed;
}
#endif

/* SwitchedIn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497cf0
// MAC: _SwitchedIn i386 0x000513a4 ppc 0x00059ca0
#ifdef MACBUILD
void SwitchedIn(EventRecord *ev)
#else
void SwitchedIn(short unused)
#endif
{
#if TARGET_API_MAC_CARBON
    ProcessSerialNumber psn;

    if (GetCurrentProcess(&psn) != procNotFound)
        SetFrontProcess(&psn);
#endif
    inBackground = 0;
    lastFPSUpdateTime = 0;
    fpsCounter = 0;
    gSpeedMult = 1.0;
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
#if defined(MACBUILD) && V111
    if (introMusicMovie != NULL)
        introMusicPaused = 0;
#else
    if (introMusicMovie != NULL)
        StartMovie(introMusicMovie);
#endif
}

/* SwitchedOut. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497d50
// MAC: _SwitchedOut i386 0x0004f381 ppc 0x00057ff8
#ifdef MACBUILD
void SwitchedOut(EventRecord *ev)
#else
void SwitchedOut(short unused)
#endif
{
    void *snd;

    inBackground = 1;
#if defined(MACBUILD) && V111
    if (introMusicMovie != NULL)
        introMusicPaused = 1;
#else
    if (introMusicMovie != NULL)
        StopMovie(introMusicMovie);
#endif
#ifdef MAC1010
#if TARGET_API_MAC_CARBON
    if (warpUpSnd[0] != NULL && ST_IsSoundPlaying(warpUpSnd[0]) != 0)
        ST_HaltSound(warpUpSnd[0]);
    if (warpUpSnd[1] != NULL && ST_IsSoundPlaying(warpUpSnd[1]) != 0)
        ST_HaltSound(warpUpSnd[1]);
#endif
#else
    if (warpUpSnd[0] != NULL && ST_IsSoundPlaying(warpUpSnd[0]) != 0)
        ST_HaltSound(warpUpSnd[0]);
    snd = warpUpSnd[1];
    if (snd != NULL && ST_IsSoundPlaying(snd) != 0)
        ST_HaltSound(warpUpSnd[1]);
#endif
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    BackColor(whiteColor);
}

/*
 * SpriteWorld's +0x1c is windowFrameP, so the destination rect is the same
 * frameRect member reached through the other frame.
 */

/* RefreshWindowBuffer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497df0
// MAC: _RefreshWindowBuffer i386 0x00051420 ppc 0x00059d54
void RefreshWindowBuffer(void)
{
#if !defined(MAC1010) || TARGET_API_MAC_CARBON
    SWSetPortToWorkArea(gSpriteWorld);
    if (gameLoaded && gameInProgress)
        SetSystemBackgroundColor();
    else
        ForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
#if V111
    if (useLegacyRenderingPipeline) {
#ifdef MACBUILD
        SetRetainedWindow(gWind, false);
#endif
        CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                 GetPortPixMapBits(GetWindowPort(gWind)),
                 &gSpriteWorld->workFrameP->frameRect,
                 &gSpriteWorld->windowFrameP->frameRect,
                 0, NULL);
        QDFlushPortBuffer(GetWindowPort(gWind), NULL);
#ifdef MACBUILD
        SetRetainedWindow(gWind, true);
#endif
        QDFlushPortBuffer(GetWindowPort(gWind), NULL);
    } else {
        QDFlushPortBuffer(GetWindowPort(gWind), NULL);
    }
#elif defined(MAC1010)
    SetRetainedWindow(gWind, false);
    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
             GetPortPixMapBits(GetWindowPort(gWind)),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, NULL);
    QDFlushPortBuffer(GetWindowPort(gWind), NULL);
    SetRetainedWindow(gWind, true);
    QDFlushPortBuffer(GetWindowPort(gWind), NULL);
#else
    CopyBits(GetPortBitMapForCopyBits(
                 gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);
    ScreenEndScene();
#endif
#endif
}

/*
 * mwcc 418 marks a name intrinsic-eligible on the declaration it gets from the
 * SYSTEM header, so any redeclaration that does not merge with it clears the
 * marking. Block scope is the only spelling that works.
 */



#define kCaptureStrIdx  0x76

Boolean CapturedAssignmentFilter(long unused, EventRecord *ev, short *itemHit);

#define kCaptureDlogID   0x3fa
#define kCapturePictID   0x2144

/* DoCapturedShipAssignmentDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00497eb0
// MAC: _DoCapturedShipAssignmentDialog i386 0x0005b508 ppc 0x000635a8
Boolean DoCapturedShipAssignmentDialog(void)
{
    Boolean done;
    Boolean result;
    short itemHit;

    done = 0;
    captureDialog = NULL;
    captureDialog = GetNewDialog(kCaptureDlogID, CapturedAssignmentDialogUpdate,
                                 CapturedAssignmentFilter);
    if (captureDialog == NULL)
        return 0;
    captureBackgroundPic = GetPicture(kCapturePictID);
    CenterWindow(FrontWindow(captureDialog), gWind);
    ShowWindowNoFlash(FrontWindow(captureDialog));
    SetDialogGWorld(captureDialog);
    CapturedAssignmentDialogUpdate();
    SelectWindow(FrontWindow(captureDialog));
    BringToFront(FrontWindow(captureDialog));
    FlushEvents(everyEvent - diskMask, 0);
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(captureDialog, CapturedAssignmentFilter, &itemHit, 0);
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
    if (captureBackgroundPic != NULL) {
        QDReleaseResource(captureBackgroundPic);
        captureBackgroundPic = NULL;
    }
    return result;
}

/* CapturedAssignmentFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00498040
// MAC: _CapturedAssignmentFilter i386 0x00051782 ppc 0x0005a024
Boolean CapturedAssignmentFilter(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    EventKey key;
    EventWhat what;

    what = ev->what;
    if (what == kKeyDown) {
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kMKeyCode) {
            *itemHit = 1;
            return 1;
        }
        if (key == kEKeyCode) {
            *itemHit = 2;
            return 1;
        }
#else
        key = EventCharOf(ev);
        key = tolower(key);
        if (key == 'm') {
            *itemHit = 1;
            return 1;
        }
        if (key == 'e') {
            *itemHit = 2;
            return 1;
        }
#endif
    }
    if (what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        CapturedAssignmentDialogUpdate();
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
    if (what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(captureDialog));
#endif
        CapturedAssignmentDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(captureDialog));
#endif
    }
    return 0;
}

/* CapturedAssignmentDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00498110
// MAC: _CapturedAssignmentDialogUpdate i386 0x0005157d ppc 0x00059e90
void CapturedAssignmentDialogUpdate(void)
{
    short itemType;
    void *item;
    Rect box;
    char text[255];

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(captureDialog));
    if (captureBackgroundPic != NULL)
        DrawPicHandle(captureBackgroundPic, GetDialogPortRect(captureDialog));
    ForeColor(whiteColor);
    BackColor(blackColor);
    GetDialogItem(captureDialog, 3, &itemType, (Handle *)&item, &box);
    strcpy(text, "");
    StrcatIndCString(text, kGameStrList, kCaptureStrIdx);
    TETextBox(text, strlen(text), &box, 0);
    ForeColor(blackColor);
    BackColor(whiteColor);
    DrawCapAssignButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(captureDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(captureDialog)),
             GetDialogPortRect(captureDialog), GetDialogPortRect(captureDialog), 0,
             GetWindowVisRgn(FrontWindow(captureDialog)));
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

/* on mac ShowTextDialog paints gWind through GetWindowPortRect and invalidates items itself */
#ifdef MACBUILD
#define GetCWindowPortRect(w)       GetWindowPortRect(w)
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &box), InvalWindowRect(GetDialogWindow(dlg), &box))
#endif
/* ShowTextDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004982a0
// MAC: _ShowTextDialog i386 0x0005a752 ppc 0x000628d4
void ShowTextDialog(Boolean allowMap, Boolean redrawMission, Boolean showMovie)
{
    Rect box;
    short savedStellar;
    short itemHit;
    short savedRoute;
    short itemType;
    Handle item;
    Boolean savedUp;
    Rect *win;
    Boolean savedDown;
    Point pt;
    Boolean done;
    short savedBottom;
    short slack;
    short viewHeight;
    short textHeight;
    short minHeight;
    short i;

    done = 0;
    if (showMovie && settings.noMovies == 0 && descMovieName[0] != 0
        && (descFlags & kDescMovieAfter) == 0)
        PlayQTMovie(descMovieName, (descFlags & kDescMovieDoubled),
                    (descFlags & kDescMovieFade));

    if (strlen(textShowString) == 0) {
        if (screenFaded) {
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    ForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    ForeColor(blackColor);
                    PaintRect(GetCWindowPortRect((WindowRef)gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                UpdateAllPortDialogs();
#ifdef MACBUILD
                RefreshSAT();
#endif
                RedrawStatus(1);
            }
            if (showingIntroText == 0)
                FadeScreenIn();
        }
        return;
    }

    strcat(textShowString, "\r");
    textDialog = NULL;
    if (descResourcePictId >= kFirstResourceID && showMovie) {
        textDialog = GetNewDialog(kTextDialogPictID, TextDialogUpdate,
                                  TextDialogFilter);
    } else {
        textDialog = GetNewDialog(kTextDialogPlainID, TextDialogUpdate,
                                  TextDialogFilter);
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
    TESetLineMetrics(textDialogTextHandle);
    TESetText(textShowString, strlen(textShowString), textDialogTextHandle);
    textHeight = TEMeasureText(textDialogTextHandle, box.right - box.left);
    GetDialogItem(textDialog, 3, &itemType, &item, &box);
    viewHeight = box.bottom - box.top;
    if (textHeight < viewHeight && descResourcePictId < kFirstResourceID) {
        if (textHeight >= kTextDlgMinHeight)
            minHeight = textHeight;
        else
            minHeight = kTextDlgMinHeight;
        win = GetDialogPortRect(textDialog);
        box = *win;
        slack = viewHeight - minHeight - kTextDlgSlackMargin;
        box.bottom -= slack;
#ifdef MACBUILD
        SizeWindow(GetDialogWindow(textDialog), (box.right - box.left),
                   (box.bottom - box.top), true);
        pt.v = 0;
        pt.h = 0;
        LocalToGlobal(&pt);
        pt.v += slack * kTextDlgRecentre;
        MoveWindow(GetDialogWindow(textDialog), pt.h, pt.v, true);
#else
        SizeDialogWindow(textDialog, (box.right - box.left),
                     (box.bottom - box.top), 1);
        pt.v = 0;
        pt.h = 0;
        pt.v += slack * kTextDlgRecentre;
        OffsetDialogWindow(textDialog, pt.h, pt.v, 1);
#endif
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
    TextDialogUpdate();
    SelectWindow(FrontWindow(textDialog));
    BringToFront(FrontWindow(textDialog));
    ShowTheCursor();

    if (gPause) {
        if (showingIntroText) {
            SetGWorldContainer(&loadPort);
            ForeColor(blackColor);
            PaintRect(&loadPort.bounds);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
        } else {
            mainScreenBackgroundDirty = 1;
            MainScreenUpdate(1);
        }
    }
    if (screenFaded) {
        if (gPause) {
            if (showingIntroText) {
                SetGWorldContainer(&loadPort);
                ForeColor(blackColor);
                PaintRect(&loadPort.bounds);
                SWSetPortToWindow(gSpriteWorld);
                ForeColor(blackColor);
                PaintRect(GetCWindowPortRect((WindowRef)gWind));
            } else {
                mainScreenBackgroundDirty = 1;
                MainScreenUpdate(1);
            }
        } else {
#ifdef MACBUILD
            RefreshSAT();
#endif
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
        MyModalDialog(textDialog, TextDialogFilter, &itemHit, 0);
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
                PortDialogUpdate();
            }
            if (redrawMission) {
                SetDialogGWorld(missionDialog);
                RedrawDialog(missionDialog);
            }
            SetDialogGWorld(textDialog);
            RedrawDialog(textDialog);
            TextDialogUpdate();
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
            ForeColor(blackColor);
            PaintRect(&loadPort.bounds);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
        } else {
            mainScreenBackgroundDirty = 1;
            MainScreenUpdate(1);
        }
    } else {
        UpdateAllPortDialogs();
    }
    if (descMovieName[0] != 0 && settings.noMovies == 0
        && (descFlags & kDescMovieAfter) != 0) {
        PlayQTMovie(descMovieName, (descFlags & kDescMovieDoubled),
                    (descFlags & kDescMovieFade));
        if (screenFaded) {
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    ForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    ForeColor(blackColor);
                    PaintRect(GetCWindowPortRect((WindowRef)gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                UpdateAllPortDialogs();
#ifdef MACBUILD
                RefreshSAT();
#endif
                RedrawStatus(1);
            }
            FadeScreenIn();
        }
    }
}
#ifdef MACBUILD
#undef GetCWindowPortRect
#undef InvalDialogItem
#endif

/* CalcShipCanBuy (mac _CalcShipCanBuy). STATE: MATCH 100.0%, exe 212 / obj 212 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00498dc0
// MAC: _CalcShipCanBuy i386 0x0004f5eb ppc 0x000582b4
void CalcShipCanBuy(void)
{
    long price;
    long need;
    long tradeIn;
    short i;
    unsigned long long key;
    Boolean sig[4];

#ifndef MAC1010
    key = shipData[5].licenseCode;
#endif
    KeyMismatch(sig[0], key, shipData[5].licenseCode, 0xef0971bf3d10d5ddLL, 0x0c302b2a15333394LL);
    KeyMismatch(sig[1], key, shipData[5].licenseCode, 0x921ec281c26516f7LL, 0x0b5e33bfc5433b83LL);
    KeyMismatch(sig[2], key, shipData[5].licenseCode, 0xab89232d6fa52638LL, 0x0359021eca0c887fLL);
    KeyMismatch(sig[3], key, shipData[5].licenseCode, 0x69d6a8458c7c1bd1LL, 0x083ec9282b467090LL);
    for (i = 0; i < 4; i++) {
        if (sig[i] == 0) {
            canBuy = 0;
            return;
        }
    }

    if (shipBuyMode == 0) {
        if (selShipyardItem != -1) {
            if (shipData[0].type == selShipyardItem) {
                canBuy = 0;
            } else {
                price = ApplyPriceAndTechnologyFlux(
                            shipClass[selShipyardItem].cost, priceFluxMult[1],
                            shipClass[selShipyardItem].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                            PlayerShipTradeInPrice(), priceFluxMult[0],
                            shipClass[shipData[0].type].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                            tradeIn, priceFluxMult[1],
                            shipClass[shipData[0].type].techLevel,
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
            if (PlayerMeetsRequirements(shipClass[selShipyardItem].requires[0],
                                        shipClass[selShipyardItem].requires[1]) == 0)
                canBuy = 0;
        }
    } else {
        if (selShipyardItem != -1) {
            need = ApplyPriceAndTechnologyFlux(
                        shipClass[selShipyardItem].cost, priceFluxMult[1],
                        shipClass[selShipyardItem].techLevel,
                        stellarData[shipData[0].targetStellar].techLevel);
            need = need * 0.1;
            if (shipData[0].credits >= need)
                canBuy = 1;
            else
                canBuy = 0;
        } else {
            canBuy = 0;
        }
    }
    if (canBuy != 0) {
        if (EvalMissionBitTestString(shipClass[selShipyardItem].availability) == 0)
            canBuy = 0;
    }
}

#define kStripCells 128
#define kCellWide 0x53
#define kCellHigh 0x36

/* SetupListRects (mac _SetupListRects). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00499150
// MAC: _SetupListRects i386 0x00051a2c ppc 0x0005a340
void SetupListRects(Point *where)
{
#ifdef CEFIX /* F12.60 */
    int i;
    Rect *cell;
    Rect *icon;

    for (i = 0; i < kGridCells; i++) {
        cell = &listScreenRect[i];
        cell->left = i % kGridCols * ceGridCellWidth + where->h;
        cell->top = i / kGridCols * ceGridCellHeight + where->v;
        cell->right = cell->left + ceGridCellWidth + 1;
        cell->bottom = cell->top + ceGridCellHeight + 1;
        icon = &listPicScreenRect[i];
        icon->left = (cell->left + cell->right - ceGridThumbSize) / 2;
        icon->bottom = (cell->top + cell->bottom + ceGridThumbSize / 2) / 2;
        icon->right = icon->left + ceGridThumbSize;
        icon->top = icon->bottom - ceGridThumbSize;
    }
    for (i = 0; i < kStripCells; i++) {
        listPicOffRect[i].left = ceGridThumbSize * i;
        listPicOffRect[i].top = 0;
        listPicOffRect[i].right = listPicOffRect[i].left + ceGridThumbSize;
        listPicOffRect[i].bottom = ceGridThumbSize;
    }
#else
    short i;
    short col;
    short row;

    for (i = 0; i < kGridCells; i++) {
        row = i / 4;
        col = i % 4;
        SetRect(&listScreenRect[i], col * kCellWide + where->h, row * kCellHigh + where->v,
                (col + 1) * kCellWide + where->h + 1,
                (row + 1) * kCellHigh + where->v + 1);
        col = (listScreenRect[i].right + listScreenRect[i].left) / 2;
        row = (listScreenRect[i].top + listScreenRect[i].bottom) / 2;
        SetRect(&listPicScreenRect[i], col - 0x10, row - 0x18, col + 0x10, row + 8);
    }
    for (i = 0; i < kStripCells; i++)
        SetRect(&listPicOffRect[i], i * 0x20, 0, (i + 1) * 0x20, 0x20);
#endif
}

#define kScrollTicksToLines 0.01667     /* ~1/60: ticks to scroll units */


/* ScrollDialogText. STATE: MATCH 100.0%, exe 144 / obj 144 instrs. Notes: Research/notes/scrolldlgtext.md */
// FUNCTION: EVNOVA 0x00499270
// MAC: _ScrollDialogText i386 0x00051839 ppc 0x0005a10c
void ScrollDialogText(DialogPtr dlg, short item, TEHandle hTE, short dv, Boolean force)
{
    Rect box;
    short type;
    void *handle;
    short savedTop;
    short delta;

    if (!force) {
        delta = dv * (float)(ReadTick() - lastTextScrollTime) * kScrollTicksToLines;
        dv = delta;
        if (dv == 0)
            return;
    }

    SetGWorldContainer(&loadPort);
    savedTop = (*hTE)->destRect.top;
    (*hTE)->inPort = (GrafPtr)loadPort.world;
    ForeColor(whiteColor);
    BackColor(blackColor);
    TEPinScroll(0, dv, hTE);
    ForeColor(blackColor);
    BackColor(whiteColor);
    (*hTE)->inPort = (GrafPtr)GetDialogPort(textDialog);
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

    GetDialogItem(dlg, item, &type, (Handle *)&handle, &box);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(dlg);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort), GetPortBitMapForCopyBits(GetDialogPort(dlg)),
             &box, &box, 0, 0);
}


#define kScrollPageStep   250


/* TextDialogFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00499440
// MAC: _TextDialogFilter i386 0x00051f77 ppc 0x0005a820
Boolean TextDialogFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    Point where;
    short itemType;
    void *item;
    short button;
#if V111
    short key;
#else
    EventChar c;
#endif

    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map)) {
            *itemHit = 4;
            return 1;
        }
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if ((int)ev->what == kKeyDown) {
            if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
                *itemHit = 1;
                return 1;
            }
        }
        if (canScrollTextUp != 0) {
            if (key == kUpArrowKeyCode) {
                *itemHit = 5;
                return 1;
            }
            if (key == kHomeKeyCode) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextUp = 0;
                return 1;
            }
            if (key == kPageUpKeyCode) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, kScrollPageStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
        if (canScrollTextDown != 0) {
            if (key == kDownArrowKeyCode) {
                *itemHit = 6;
                return 1;
            }
            if (key == kEndKeyCode) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, -kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextDown = 0;
                return 1;
            }
            if (key == kPageDownKeyCode) {
                ScrollDialogText(textDialog, 3, textDialogTextHandle, -kScrollPageStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
#else
        c = EventCharOf(ev);
        if ((int)ev->what == kKeyDown) {
            if (IsReturnChar(c) || c == kEscapeChar) {
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
#endif
    }
    if ((int)ev->what == kMouseDown) {
        SetDialogGWorld(textDialog);
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        if (canScrollTextUp != 0) {
            GetDialogItem(textDialog, 5, &itemType, (Handle *)&item, &box);
            if (PtInRectPoint(where, &box) && canScrollTextUp != 0) {
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
            GetDialogItem(textDialog, 6, &itemType, (Handle *)&item, &box);
            if (PtInRectPoint(where, &box) && canScrollTextDown != 0) {
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
        TextDialogUpdate();
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(textDialog));
#endif
        TextDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(textDialog));
#endif
    }
    return 0;
}

/* TextDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00499870
// MAC: _TextDialogUpdate i386 0x00051b40 ppc 0x0005a474
void TextDialogUpdate(void)
{
    PicHandle loaded;
    PicHandle pic;
    Rect r;
    Rect r2;
    short itemType;
    Handle item;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
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
#ifdef CEFIX /* F12.61 */
            CE_ScaleOffsetRect(&r, GetDialogPortRect(textDialog)->left,
                               GetDialogPortRect(textDialog)->top);
#else
            OffsetRect(&r, GetDialogPortRect(textDialog)->left,
                       GetDialogPortRect(textDialog)->top);
#endif
            DrawPicHandle(textDialogBackgroundPic[1], &r);
        }
        if (textDialogBackgroundPic[0] != NULL) {
            QDGetPictureBounds(textDialogBackgroundPic[0], &r);
            OffsetRect(&r, -r.left, -r.top);
#ifdef CEFIX /* F12.62 */
            CE_ScaleOffsetRect(&r, GetDialogPortRect(textDialog)->left,
                               GetDialogPortRect(textDialog)->top);
#else
            OffsetRect(&r, GetDialogPortRect(textDialog)->left,
                       GetDialogPortRect(textDialog)->top);
#endif
            DrawPicHandle(textDialogBackgroundPic[0], &r);
        }
        pic = textDialogBackgroundPic[2];
        if (pic != NULL) {
            QDGetPictureBounds(pic, &r);
            OffsetRect(&r, -r.left, -r.top);
#ifdef CEFIX /* F12.63 */
            CE_ScaleOffsetRectBottom(&r, GetDialogPortRect(textDialog)->left,
                                     GetDialogPortRect(textDialog)->bottom - (r.bottom - r.top));
#else
            OffsetRect(&r, GetDialogPortRect(textDialog)->left,
                       GetDialogPortRect(textDialog)->bottom - (r.bottom - r.top));
#endif
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
    (*textDialogTextHandle)->inPort = (GrafPtr)loadPort.world;
    GetDialogItem(textDialog, 3, &itemType, &item, &r);
    ForeColor(whiteColor);
    BackColor(blackColor);
    TEUpdate(&r, textDialogTextHandle);
    ForeColor(blackColor);
    BackColor(whiteColor);
    (*textDialogTextHandle)->inPort = (GrafPtr)GetDialogPort(textDialog);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(textDialog);
    GetDialogItem(textDialog, 1, &itemType, &item, &r);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
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

#ifdef MACBUILD
/* MyModalDialogIsland (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _MyModalDialogIsland i386 0x0005a749 ppc 0x000628d0
void MyModalDialogIsland(DialogPtr dlg, void *eventProc, short *itemHit, short *modifiers)
{
    MyModalDialog(dlg, eventProc, itemHit, modifiers);
}
#endif

/* DoPlayerInfoDialog (mac _DoPlayerInfoDialog). STATE: MATCH 100.0%, exe 470 / obj 470,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00499c10
// MAC: _DoPlayerInfoDialog i386 0x0005a00a ppc 0x00062244
void DoPlayerInfoDialog(void)
{
    Handle item;
    short itemHit;
    Rect r;
    Rect buttonBox[2];
    short itemType;
    Rect saved;
    Rect box6;
    unsigned char buf[256];
    Rect *win;
    short savedTop;
    short savedLeft;
    short savedBottom;
    short savedRight;
    short delta;
    short i;
    short tallest;
    Boolean done;
    short height;
    short boxHeight;
    short dv;
    char *p;
#ifdef MACBUILD
    Point pt;
#endif

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
    selPlayerInfoMode = 1;
    infoDialog = NULL;
    infoDialog = GetNewDialog(kPlayerInfoDlogID, PlayerInfoDialogUpdate,
                                    PlayerInfoFilter);
    if (infoDialog == NULL)
        return;
    for (i = 0; i < 3; i++)
        playerInfoBackgroundPic[i] = GetPicture(kPlayerInfoFirstPictID + i);
    SetDialogGWorld(infoDialog);
    SetupPlayerInfoText();
#ifdef MACBUILD
    saved = *GetDialogPortRect(infoDialog);
#else
    win = GetDialogPortRect(infoDialog);
    savedTop = win->top;
    savedLeft = win->left;
    savedBottom = win->bottom;
    savedRight = win->right;
#endif
    GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
    GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
    GetDialogItem(infoDialog, 7, &itemType, &item, &buttonBox[1]);
    textDialogTextHandle = TENew(&box6, &box6);
    (*textDialogTextHandle)->txFont = smallTextFont;
    (*textDialogTextHandle)->txSize = smallTextSize;
    TextFont((*textDialogTextHandle)->txFont);
    TextSize((*textDialogTextHandle)->txSize);
    TESetLineMetrics(textDialogTextHandle);
    tallest = -1;
    for (i = 0; i < 3; i++) {
        p = playerInfoText[i];
        TESetText(p, strlen(p), textDialogTextHandle);
        TECalText(textDialogTextHandle);
        height = TEWrappedHeight(textDialogTextHandle);
        if (height > tallest || tallest <= 0)
            tallest = height;
    }
    TEDispose(textDialogTextHandle);
    boxHeight = box6.bottom - box6.top;
    if (tallest > boxHeight) {
        delta = tallest - boxHeight + 4;
#ifdef MACBUILD
        r = saved;
        r.bottom += delta;
        SizeWindow(GetDialogWindow(infoDialog), (r.right - r.left),
                   (r.bottom - r.top), true);
        pt.v = 0;
        pt.h = 0;
        LocalToGlobal(&pt);
        pt.v -= delta * 0.5;
        MoveWindow(GetDialogWindow(infoDialog), pt.h, pt.v, true);
        r = buttonBox[0];
        OffsetRect(&r, 0, delta);
        GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
        SetDialogItem(infoDialog, 1, itemType, item, &r);
        r = buttonBox[1];
        GetDialogItem(infoDialog, 7, &itemType, &item, &buttonBox[1]);
        OffsetRect(&r, 0, delta);
        SetDialogItem(infoDialog, 7, itemType, item, &r);
        r = box6;
        r.bottom += delta;
        GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
        SetDialogItem(infoDialog, 6, itemType, item, &r);
#else
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
        GetDialogItem(infoDialog, 7, &itemType, &item, &buttonBox[1]);
        OffsetRect(&r, 0, delta);
        SetDialogItem(infoDialog, 7, itemType, item, &r);
        r.top = box6.top;
        r.left = box6.left;
        r.bottom = box6.bottom;
        r.right = box6.right;
        r.bottom += delta;
        GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
        SetDialogItem(infoDialog, 6, itemType, item, &r);
#endif
    }
    CenterWindow(FrontWindow(infoDialog), gWind);
    ShowWindowNoFlash(FrontWindow(infoDialog));
    SetDialogGWorld(infoDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(infoDialog);
    PlayerInfoDialogUpdate();
    SelectWindow(FrontWindow(infoDialog));
    BringToFront(FrontWindow(infoDialog));
    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(infoDialog, PlayerInfoFilter, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit >= 2 && itemHit <= 5) {
            selPlayerInfoMode = itemHit - 1;
            RedrawDialog(infoDialog);
        }
        if (itemHit == 7 && selPlayerInfoMode == 2) {
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
#ifdef MACBUILD
    RefreshSAT();
#endif
    FlushEvents(7, 0);
}

/* PlayerInfoFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049a3a0
// MAC: _PlayerInfoFilter i386 0x00053ae9 ppc 0x0005c1b8
Boolean PlayerInfoFilter(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
#if V111
    short key;
#else
    int c;
#endif

#if V111
    if (ev->what == kKeyDown || ev->what == kAutoKey) {
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kTabKeyCode) {
            if ((ev->modifiers & shiftKey) != 0)
                selPlayerInfoMode -= 1;
            else
                selPlayerInfoMode += 1;
            if (selPlayerInfoMode > kPlayerInfoPages)
                selPlayerInfoMode = 1;
            if (selPlayerInfoMode < 1)
                selPlayerInfoMode = kPlayerInfoPages;
            RedrawDialog(infoDialog);
            *itemHit = selPlayerInfoMode + 1;
            return 1;
        }
        if (ev->what == kKeyDown) {
            if (key == kEnterKeyCode || key == kReturnKeyCode || key == kEscapeKeyCode
                    || KeyCheck(keyPrefs.playerInfo)) {
                *itemHit = 1;
                return 1;
            }
        }
    }
#else
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        c = EventCharOf(ev);
        if (c == kTabChar) {
            if ((*(unsigned short *)&ev->modifiers & shiftKey) != 0)
                selPlayerInfoMode -= 1;
            else
                selPlayerInfoMode += 1;
            if (selPlayerInfoMode > kPlayerInfoPages)
                selPlayerInfoMode = 1;
            if (selPlayerInfoMode < 1)
                selPlayerInfoMode = kPlayerInfoPages;
            RedrawDialog(infoDialog);
            *itemHit = selPlayerInfoMode + 1;
            return 1;
        }
        if ((int)ev->what == kKeyDown) {
#ifdef MAC1010 /* 1.0.10 spelling */
            if (c == kEnterChar || c == kReturnChar || c == kEscapeChar || KeyCheck(keyPrefs.playerInfo)) {
#else
            if (IsReturnChar(c) || c == kEscapeChar || KeyCheck(keyPrefs.playerInfo)) {
#endif
                *itemHit = 1;
                return 1;
            }
        }
    }
#endif
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        PlayerInfoDialogUpdate();
        button = TrackPlayerInfoButtons(selPlayerInfoMode, where);
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(infoDialog));
#endif
        PlayerInfoDialogUpdate();
#ifdef MACBUILD
        EndUpdate(GetDialogWindow(infoDialog));
#endif
        return 0;
    }
    return 0;
}

/* PlayerInfoDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049a540
// MAC: _PlayerInfoDialogUpdate i386 0x0005244f ppc 0x0005ac78
void PlayerInfoDialogUpdate(void)
{
    Rect box;
    Rect box2;
    short itemType;
    void *itemHandle;
    unsigned char cheatBuf[256];
    PStrChar buf[2048];
    short i;
    long stellarWorth;
    long escortWorth;
    short sys;
    short jumps;
    short count;
    Boolean onMission;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
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
#ifdef CEFIX /* F12.64 */
        CE_ScaleOffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                           GetDialogPortRect(infoDialog)->top);
#else
        OffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                   GetDialogPortRect(infoDialog)->top);
#endif
        DrawPicHandle(playerInfoBackgroundPic[1], &box);
    }

    if (playerInfoBackgroundPic[0] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[0], &box);
        OffsetRect(&box, -box.left, -box.top);
#ifdef CEFIX /* F12.65 */
        CE_ScaleOffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                           GetDialogPortRect(infoDialog)->top);
#else
        OffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                   GetDialogPortRect(infoDialog)->top);
#endif
        DrawPicHandle(playerInfoBackgroundPic[0], &box);
    }

    if (playerInfoBackgroundPic[2] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[2], &box);
        OffsetRect(&box, -box.left, -box.top);
#ifdef CEFIX /* F12.66 */
        CE_ScaleOffsetRectBottom(&box, GetDialogPortRect(infoDialog)->left,
                                 GetDialogPortRect(infoDialog)->bottom -
                                 (box.bottom - box.top));
#else
        OffsetRect(&box, GetDialogPortRect(infoDialog)->left,
                   GetDialogPortRect(infoDialog)->bottom -
                   (box.bottom - box.top));
#endif
        DrawPicHandle(playerInfoBackgroundPic[2], &box);
    }

    TextFont(smallTextFont);
    TextSize(smallTextSize);
    GetDialogItem(infoDialog, 6, &itemType, (Handle *)&itemHandle, &box);
    if (!RectInRgn(&box, GetWindowVisRgn(FrontWindow(infoDialog))))
        goto blit_back;

    if (selPlayerInfoMode == 1) {
        /* --- left column --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0xc));
        DrawIndString(kGameStrList, 0xfb);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0xc));
        DrawString(playerName);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x1c));
        DrawIndString(kGameStrList, 0xfc);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x1c));
        DateToMedString(gameTime, (char *)buf);
        CtoPstr((char *)buf);
        DrawString((const unsigned char *)buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x2c));
        DrawIndString(kGameStrList, 0xfd);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x2c));
        DrawString(systemData[shipData->curSystem].name);
        if (cheatsActive) {
            DrawString("\p (");
            NumToString(shipData->curSystem + 128, cheatBuf);
            DrawString(cheatBuf);
            DrawString("\p)");
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x3c));
        DrawIndString(kGameStrList, 0x146);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x3c));
        if (SystemIsInhabited(shipData->curSystem)) {
            DrawLegalStatusString(shipData->curSystem);
            if (cheatsActive) {
                DrawString("\p (");
                NumToString(playerRecord[shipData->curSystem], cheatBuf);
                DrawString(cheatBuf);
                DrawString("\p)");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x4c));
        DrawIndString(kGameStrList, 0xfe);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x4c));
        DrawCombatRatingString();
        if (cheatsActive) {
            DrawString("\p (");
            NumToString(playerKills, cheatBuf);
            DrawString(cheatBuf);
            DrawString("\p)");
        }

        /* --- right column --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0xc));
        DrawIndString(kGameStrList, 0xff);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0xc));
        DrawString(shipName);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x1c));
        DrawIndString(kGameStrList, 0x100);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x1c));
        DrawString(shipClass[shipData->type].name);
        if (cheatsActive) {
            DrawString("\p (");
            NumToString(shipData->type + 128, cheatBuf);
            DrawString(cheatBuf);
            DrawString("\p)");
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x2c));
        DrawIndString(kGameStrList, 0x101);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x2c));
#ifdef CEFIX /* F29.02 */
        NumToString(CE_TruncF32(ShipTurnRate(shipData) * 30.0f), (StringPtr)buf);
#else
        NumToString((ShipTurnRate(shipData) * 30.0f), (StringPtr)buf);
#endif
        DrawString((const unsigned char *)buf);
        DrawIndString(kGameStrList, 0x102);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x3c));
        DrawIndString(kGameStrList, 0x103);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x3c));
#ifdef CEFIX /* F29.03 */
        NumToString(CE_TruncF32(ShipAccelRate(shipData) * 2500.0), (StringPtr)buf);
#else
        NumToString((ShipAccelRate(shipData) * 2500.0), (StringPtr)buf);
#endif
        DrawString((const unsigned char *)buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x4c));
        DrawIndString(kGameStrList, 0x104);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x4c));
#ifdef CEFIX /* F29.04-05 */
        if (strictPlay)
            NumToString(CE_TruncF32(ShipMaxSpeed(shipData) * 100.0f), (StringPtr)buf);
        else
            NumToString(CE_TruncF32(ShipMaxSpeed(shipData) * 100.0f *
                                    0.6666666666666666), (StringPtr)buf);
#else
        if (strictPlay)
            NumToString((ShipMaxSpeed(shipData) * 100.0f), (StringPtr)buf);
        else
            NumToString((ShipMaxSpeed(shipData) * 100.0f *
                               0.6666666666666666), (StringPtr)buf);
#endif
        DrawString((const unsigned char *)buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x5c));
        pstrcpy((StringPtr)buf, creditsString);
        if ((char)buf[0] > 0)
            buf[1] = toupper((char)buf[1]);
        DrawString((const unsigned char *)buf);
        DrawString("\p:");
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x5c));
        DrawCommaNumber(shipData->credits);

        /* --- shields / armor / fuel, left column continued --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x5c));
        DrawString(shieldString[1]);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x5c));
        if (ShipShieldCapacity(shipData) > 0.0) {
            if (shipData->shield >= 0.0f) {
                if (shipData->shield <= ShipShieldCapacity(shipData)) {
                    NumToString((shipData->shield /
                                       ShipShieldCapacity(shipData) * 100.0), (StringPtr)buf);
                    DrawString((const unsigned char *)buf);
                    DrawString("\p%");
                } else {
                    DrawString("\p100%");
                }
            } else {
                DrawString("\p0%");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x6c));
        DrawString(armorString[1]);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x6c));
        if (IsDying(shipData)) {
            DrawIndString(kGameStrList, 0x105);
        } else if (ShipArmorCapacity(shipData) > 0.0) {
            if (shipData->armor <= ShipArmorCapacity(shipData)) {
                NumToString((shipData->armor /
                                   ShipArmorCapacity(shipData) * 100.0), (StringPtr)buf);
                DrawString((const unsigned char *)buf);
                DrawString("\p%");
            } else {
                DrawString("\p100%");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x7c));
        DrawString(fuelString[5]);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x7c));
        if (ShipFuelCapacity(shipData) <= 0.0) {
            DrawIndString(kGameStrList, 0x14f);
        } else {
            NumToString((shipData->fuel /
                               ShipFuelCapacity(shipData) * 100.0), (StringPtr)buf);
            DrawString((const unsigned char *)buf);
            DrawString("\p%");
            jumps = (shipData->fuel / 100.0);
            if (jumps > 0) {
                DrawString("\p  (");
                NumToString(jumps, (StringPtr)buf);
                DrawString((const unsigned char *)buf);
                DrawString("\p ");
                if (jumps > 1)
                    DrawIndString(kGameStrList, 0xf0);
                else
                    DrawIndString(kGameStrList, 0xef);
                if ((short)shipData->fuel % 100 > 0) {
                    DrawString("\p ");
                    DrawIndString(kGameStrList, 0x106);
                    DrawString("\p ");
                    DrawString(fuelString[6]);
                }
                DrawString("\p)");
            } else if (shipData->fuel > 0.0f) {
                DrawString("\p  (");
                DrawString(fuelString[6]);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x107);
                DrawString("\p)");
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
            escortWorth += shipClass[shipData[i].type].cost * 0.01;
        }

        for (i = 0; i < 0x800; i++) {
            if (stellarData[i].loaded == 0 || stellarData[i].dominated == 0)
                continue;
            sys = stellarData[i].system;
            if (sys < 0 || sys >= 0x800)
                continue;
            if (systemData[sys].loaded == 0 || systemData[sys].prepared == 0)
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
                DrawString("\p  ");
                ForeColor(whiteColor);
                DrawCommaNumber(escortWorth - stellarWorth);
            } else if (escortWorth != 0) {
                DrawIndString(kGameStrList, 0x10a);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x109);
                DrawString("\p  ");
                ForeColor(whiteColor);
                DrawCommaNumber(stellarWorth - escortWorth);
            } else {
                DrawIndString(kGameStrList, 0x109);
                DrawString("\p  ");
                ForeColor(whiteColor);
                DrawCommaNumber(stellarWorth);
            }
            DrawString("\p ");
            DrawString(creditsString);
            RGBForeColor(&rgbLightGrey);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x10b);
        }
    } else if (selPlayerInfoMode == 2) {
        if (PlayerHasAnyCargo()) {
            ForeColor(blackColor);
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[0], strlen(playerInfoText[0]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            if (TotalFleetHolds() > ShipTotalHolds())
                GetIndString((StringPtr)buf, kGameStrList, 0x10c);
            else
                GetIndString((StringPtr)buf, kGameStrList, 0x10d);
#ifdef CEFIX /* F12.67 */
            DrawStringCentered((StringPtr)buf, box.left, box.right, box.top + CE_Scale(0x37));
#else
            DrawStringCentered((StringPtr)buf, box.left, box.right, (box.top + 0x37));
#endif
        }
    } else if (selPlayerInfoMode == 3) {
        count = 0;
        for (i = 0; i < 0x200; i++) {
            if (shipSystems[i] > 0 && lcOutfitName[i][0] != 0 &&
                    (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                count++;
                break;
            }
        }
        if (count > 0) {
            ForeColor(blackColor);
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[1], strlen(playerInfoText[1]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            GetIndString((StringPtr)buf, kGameStrList, 0x10e);
#ifdef CEFIX /* F12.67 */
            DrawStringCentered((StringPtr)buf, box.left, box.right, box.top + CE_Scale(0x37));
#else
            DrawStringCentered((StringPtr)buf, box.left, box.right, (box.top + 0x37));
#endif
        }
    } else if (selPlayerInfoMode == 4) {
        count = 0;
        for (i = 0; i < 0x80; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0) {
                count++;
                break;
            }
        }
        for (i = 0; i < 0x200; i++) {
            if (shipSystems[i] > 0 && lcOutfitName[i][0] != 0 &&
                    (outfitSystem[i].flags & kOutfShowInRanks) != 0)
                count++;
        }
        if (count > 0) {
            ForeColor(blackColor);
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[2], strlen(playerInfoText[2]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            GetIndString((StringPtr)buf, kGameStrList, 0x10f);
#ifdef CEFIX /* F12.67 */
            DrawStringCentered((StringPtr)buf, box.left, box.right, box.top + CE_Scale(0x37));
#else
            DrawStringCentered((StringPtr)buf, box.left, box.right, (box.top + 0x37));
#endif
        }
    }

blit_back:
    DrawPlayerInfoButtons(selPlayerInfoMode, -1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(infoDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(infoDialog)),
             GetDialogPortRect(infoDialog),
             GetDialogPortRect(infoDialog),
             0,
             GetWindowVisRgn(FrontWindow(infoDialog)));
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
// MAC: _SetupPlayerInfoText i386 0x00053cb8 ppc 0x0005c3e4
void SetupPlayerInfoText(void)
#ifdef MAC1010
{
    Boolean rankShown[128];
    short i;
    short shown;
    short count;
    short freeSpace;
    short n;
    short len;
    short maxWeight;
    short found;
    short type;
    long credits;
    short j;
    short weight;
    char c;
    short cargoQty[256];
    short outfitQty[512];
    unsigned char str[256];
    Boolean seen[256];
    short missionTons;
    short missionCount;
    short junkTons;
    short numTypes;
    short numJunk;
    int starred;

    for (i = 0; i < 3; i++)
        playerInfoText[i][0] = 0;
    missionTons = 0;
    junkTons = 0;
    missionCount = 0;
    numJunk = 0;
    numTypes = 0;
    for (i = 0; i < kMaxCargo; i++) {
        seen[i] = 0;
        cargoQty[i] = -1;
    }

    for (i = 0; i <= 5; i++) {
        if (shipData->cargo[i] > 0) {
            if (cargoQty[i] < 0)
                cargoQty[i] = 0;
            cargoQty[i] += shipData->cargo[i];
        }
    }

    for (j = 0; j < kMaxActiveMissions; j++) {
        if (currentMissionObjectives[j].active != 0 && currentMissions[j].hasCargo != 0 &&
            currentMissions[j].cargoType != -1 && currentMissions[j].cargoQty >= 0) {
            if (cargoQty[currentMissions[j].cargoType] < 0)
                cargoQty[currentMissions[j].cargoType] = 0;
            cargoQty[currentMissions[j].cargoType] += currentMissions[j].cargoQty;
            missionTons += currentMissions[j].cargoQty;
            missionCount++;
        }
    }

    for (i = 0; i < kMaxCargo; i++) {
        if (cargoQty[i] >= 0)
            numTypes++;
    }

    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            junkTons += junkData[i].held;
            numJunk++;
            numTypes++;
        }
    }

    StrcatIndCString(playerInfoText[0], kGameStrList, kStrCargoHeader);
    strcat(playerInfoText[0], " ");
    if (TotalFleetHolds() > ShipTotalHolds())
        StrcatIndCString(playerInfoText[0], kGameStrList, kStrHolds);
    else
        StrcatIndCString(playerInfoText[0], kGameStrList, kStrHold);
    strcat(playerInfoText[0], ":\r\r");
    len = strlen(playerInfoText[0]);

    if (numTypes > 0) {
        shown = 0;
        for (i = 0; i < kMaxCargo; i++) {
            n = cargoQty[i];
            if (n >= 0) {
                starred = lcCargoName[i][1] == '*';
                if (n > 0 && (Boolean)starred == 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(n, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(n, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (n == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrOf);
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
                    if ((Boolean)starred != 0) {
                        strcat(playerInfoText[0], ", ");
                    } else {
                        if (numTypes >= 3)
                            strcat(playerInfoText[0], ",");
                        strcat(playerInfoText[0], " ");
                        StrcatIndCString(playerInfoText[0], kGameStrList, kStrAnd);
                        strcat(playerInfoText[0], " ");
                    }
                }
                shown++;
            }
        }

        if (numJunk > 0) {
            for (i = 0; i < kMaxJunks; i++) {
                if (junkData[i].held > 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(junkData[i].held, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(junkData[i].held, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (junkData[i].held == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrOf);
                    strcat(playerInfoText[0], " ");
                    /* BUG: a junk name of 64+ characters is left unterminated for the strcat below */
                    strncpy((char *)str, junkData[i].lcName, 64);
                    strcat(playerInfoText[0], (char *)str);
                    if (shown < numTypes - 2)
                        strcat(playerInfoText[0], ", ");
                    if (shown == numTypes - 2) {
                        if (numTypes >= 3)
                            strcat(playerInfoText[0], ",");
                        strcat(playerInfoText[0], " ");
                        StrcatIndCString(playerInfoText[0], kGameStrList, kStrAnd);
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
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrFreeSpace);
                strcat(playerInfoText[0], " ");
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrHolds);
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
                n = TotalMissionCargoSpace();
                if (n < 0)
                    n = 0;
                if (n != freeSpace && n > 0) {
                    strcat(playerInfoText[0], "  (");
                    NumToString(n, str);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    if (n == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                } else if (n == 0 && freeSpace > 0) {
                    strcat(playerInfoText[0], "(");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrNo);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                }
            } else {
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrFreeSpace);
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

    StrcatIndCString(playerInfoText[1], kGameStrList, kStrOutfitHeader);
    strcat(playerInfoText[1], "\r\r");

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        missionTons = shipSystems[i];
        if (missionTons > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
            if (outfitSystem[i].sameNameIndex >= 0 && outfitSystem[i].sameNameIndex < i)
                type = outfitSystem[i].sameNameIndex;
            else
                type = i;
            if (outfitQty[type] == 0) {
                if (outfitSystem[type].dispWeight > maxWeight || count == 0)
                    maxWeight = outfitSystem[type].dispWeight;
                count++;
            }
            outfitQty[type] += missionTons;
        }
    }

    shown = 0;
    len = strlen(playerInfoText[1]);
    do {
        freeSpace = 0;
        n = 0;
        type = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (outfitQty[i] > 0 && lcOutfitName[i][0] != 0 &&
                (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                if (maxWeight == (weight = outfitSystem[i].dispWeight)) {
                    freeSpace++;
                    if (outfitQty[i] == 1) {
                        c = tolower(lcOutfitName[i][1]);
                        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrAn);
                        else
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrA);
                    } else if (outfitQty[i] == 2) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 2);
                    } else if (outfitQty[i] == 3) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 3);
                    } else if (outfitQty[i] > 3) {
                        NumToString(outfitQty[i], str);
                        PtoCstr(str);
                        strcat(playerInfoText[1], (char *)str);
                    }
                    strcat(playerInfoText[1], " ");
                    if (outfitQty[i] <= 3 && len > 0)
                        playerInfoText[1][len] = toupper(playerInfoText[1][len]);
                    len = -1;
                    if (outfitQty[i] > 1)
                        pstrcpy(str, lcPluralOutfitName[i]);
                    else
                        pstrcpy(str, lcOutfitName[i]);
                    PtoCstr(str);
                    strcat(playerInfoText[1], (char *)str);
                    shown++;
                    if (shown <= count - 1 && count > 2)
                        strcat(playerInfoText[1], ",");
                    if (shown < count - 1) {
                        strcat(playerInfoText[1], " ");
                    } else if (shown == count - 1) {
                        strcat(playerInfoText[1], " ");
                        StrcatIndCString(playerInfoText[1], kGameStrList, kStrAnd);
                        strcat(playerInfoText[1], " ");
                    }
                    outfitQty[i] = 0;
                } else if (weight < maxWeight) {
                    if (weight > type || n == 0)
                        type = weight;
                }
                n++;
            }
        }
        maxWeight = type;
    } while (freeSpace > 0);

    strcat(playerInfoText[1], ".\r\r");

    credits = PlayerShipTradeInPrice();
    if (credits > 0) {
        StrcatIndCString(playerInfoText[1], kGameStrList, kStrTradeInValue);
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
            if (rankData[i].priority > maxWeight || count == 0)
                maxWeight = rankData[i].priority;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    for (i = 0; i < kMaxOutfits; i++) {
        j = shipSystems[i];
        if (j > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
            if (outfitSystem[i].sameNameIndex >= 0 && outfitSystem[i].sameNameIndex < i)
                type = outfitSystem[i].sameNameIndex;
            else
                type = i;
            outfitQty[type] += j;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (outfitQty[i] > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0)
            count++;
    }

    if (count > 0) {
        strcpy(playerInfoText[2], "");
        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAwardsHeader);
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
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAnd);
                        strcat(playerInfoText[2], " ");
                    }
                }
            }
            found = 0;
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 && rankShown[i] == 0) {
                    if (rankData[i].priority > maxWeight || found == 0)
                        maxWeight = rankData[i].priority;
                    found++;
                }
            }
        } while (found > 0);

        len = (shown == 0) ? (short)strlen(playerInfoText[2]) : (short)-1;

        for (i = 0; i < kMaxOutfits; i++) {
            type = outfitQty[i];
            if (type > 0 && lcOutfitName[i][0] != 0 &&
                (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
                if (type == 1) {
                    c = tolower(lcOutfitName[i][1]);
                    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAn);
                    else
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrA);
                } else if (type == 2) {
                    StrcatIndCString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 2);
                } else if (type == 3) {
                    StrcatIndCString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 3);
                } else if (type > 3) {
                    NumToString(type, str);
                    PtoCstr(str);
                    strcat(playerInfoText[2], (char *)str);
                }
                strcat(playerInfoText[2], " ");
                if (type <= 3 && len > 0)
                    playerInfoText[2][len] = toupper(playerInfoText[2][len]);
                len = -1;
                if (type > 1)
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
#else
{
#ifdef MACBUILD
    Boolean rankShown[128];
    long credits;
    char c;
    short shown;
    short count;
    short numTypes;
    short numJunk;
    short j;
    short n;
    short maxWeight;
    short weight;
    short found;
    short freeSpace;
    short space;
    short len;
    short type;
    short qty;
    short i;
    Boolean starred;
    short cargoQty[256];
    short outfitQty[512];
    unsigned char str[256];
#else
    Boolean rankShown[128];
    long credits;
    char c;
    Boolean starred;
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
#endif

    for (qty = 0; qty < 3; qty++)
        playerInfoText[qty][0] = 0;
    numJunk = 0;
    numTypes = 0;
#ifdef MACBUILD
    for (j = 0; j < kMaxCargo; j++)
        cargoQty[j] = -1;

    for (j = 0; j <= 5; j++) {
        if (shipData->cargo[j] > 0) {
            if (cargoQty[j] < 0)
                cargoQty[j] = 0;
            cargoQty[j] += shipData->cargo[j];
        }
    }
#else
    for (qty = 0; qty < kMaxCargo; qty++)
        cargoQty[qty] = -1;

    for (qty = 0; qty <= 5; qty++) {
        if (shipData->cargo[qty] > 0) {
            if (cargoQty[qty] < 0)
                cargoQty[qty] = 0;
            cargoQty[qty] += shipData->cargo[qty];
        }
    }
#endif

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

    StrcatIndCString(playerInfoText[0], kGameStrList, kStrCargoHeader);
    strcat(playerInfoText[0], " ");
    if (TotalFleetHolds() > ShipTotalHolds())
        StrcatIndCString(playerInfoText[0], kGameStrList, kStrHolds);
    else
        StrcatIndCString(playerInfoText[0], kGameStrList, kStrHold);
    strcat(playerInfoText[0], ":\r\r");
    len = strlen(playerInfoText[0]);

    if (numTypes > 0) {
        shown = 0;
        for (i = 0; i < kMaxCargo; i++) {
#ifdef MACBUILD
            n = cargoQty[i];
            if (n >= 0) {
                starred = (lcCargoName[i][1] == '*') ? 1 : 0;
                if (n > 0 && starred == 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(n, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(n, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (n == 1)
#else
            qty = cargoQty[i];
            if (qty >= 0) {
                starred = (lcCargoName[i][1] == '*') ? (Boolean)1 : (Boolean)0;
                if (qty > 0 && starred == 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(qty, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(qty, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (qty == 1)
#endif
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrOf);
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
                        StrcatIndCString(playerInfoText[0], kGameStrList, kStrAnd);
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
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrOf);
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
                        StrcatIndCString(playerInfoText[0], kGameStrList, kStrAnd);
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
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrFreeSpace);
                strcat(playerInfoText[0], " ");
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrHolds);
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
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                } else if (space == 0 && freeSpace > 0) {
                    strcat(playerInfoText[0], "(");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrNo);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                }
            } else {
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrFreeSpace);
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

    StrcatIndCString(playerInfoText[1], kGameStrList, kStrOutfitHeader);
    strcat(playerInfoText[1], "\r\r");

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        qty = shipSystems[i];
        if (qty > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
#ifdef MACBUILD
            if (outfitSystem[i].sameNameIndex < 0 || outfitSystem[i].sameNameIndex >= i)
                type = i;
            else
                type = outfitSystem[i].sameNameIndex;
            if (outfitQty[type] == 0) {
                if (outfitSystem[type].dispWeight > maxWeight || count == 0)
                    maxWeight = outfitSystem[type].dispWeight;
                count++;
            }
            outfitQty[type] += qty;
#else
            slot = outfitSystem[i].sameNameIndex;
            if (slot < 0 || slot >= i)
                slot = i;
            if (outfitQty[slot] == 0) {
                weight = outfitSystem[slot].dispWeight;
                if (weight > maxWeight || count == 0)
                    maxWeight = weight;
                count++;
            }
            outfitQty[slot] += qty;
#endif
        }
    }

#ifdef MACBUILD
    shown = 0;
    len = strlen(playerInfoText[1]);
    do {
        j = 0;
        type = 0;
        n = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && lcOutfitName[i][0] != 0 &&
                (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                weight = outfitSystem[i].dispWeight;
                if (maxWeight == weight) {
                    j++;
                    if (qty == 1) {
                        c = tolower(lcOutfitName[i][1]);
                        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrAn);
                        else
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrA);
                    } else if (qty == 2) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 2);
                    } else if (qty == 3) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 3);
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
                    shown++;
                    if (shown <= count - 1 && count > 2)
                        strcat(playerInfoText[1], ",");
                    if (shown < count - 1) {
                        strcat(playerInfoText[1], " ");
                    } else if (shown == count - 1) {
                        strcat(playerInfoText[1], " ");
                        StrcatIndCString(playerInfoText[1], kGameStrList, kStrAnd);
                        strcat(playerInfoText[1], " ");
                    }
                    outfitQty[i] = 0;
                } else if (weight < maxWeight) {
                    if (weight > n || type == 0)
                        n = weight;
                }
                type++;
            }
        }
        maxWeight = n;
    } while (j > 0);
#else
    numTypes = 0;
    len = strlen(playerInfoText[1]);
    do {
        found = 0;
        type = 0;
        nextWeight = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && lcOutfitName[i][0] != 0 &&
                (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                weight = outfitSystem[i].dispWeight;
                if (maxWeight == weight) {
                    found++;
                    if (qty == 1) {
                        c = tolower(lcOutfitName[i][1]);
                        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrAn);
                        else
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrA);
                    } else if (qty == 2) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 2);
                    } else if (qty == 3) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 3);
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
                        StrcatIndCString(playerInfoText[1], kGameStrList, kStrAnd);
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
#endif

    strcat(playerInfoText[1], ".\r\r");

    credits = PlayerShipTradeInPrice();
    if (credits > 0) {
        StrcatIndCString(playerInfoText[1], kGameStrList, kStrTradeInValue);
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
            if (rankData[i].priority > maxWeight || count == 0)
                maxWeight = rankData[i].priority;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    for (i = 0; i < kMaxOutfits; i++) {
#ifdef MACBUILD
        j = shipSystems[i];
        if (j > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
            if (outfitSystem[i].sameNameIndex < 0 || outfitSystem[i].sameNameIndex >= i)
                type = i;
            else
                type = outfitSystem[i].sameNameIndex;
            outfitQty[type] += j;
        }
#else
        qty = shipSystems[i];
        if (qty > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
            slot = outfitSystem[i].sameNameIndex;
            if (slot < 0 || slot >= i)
                slot = i;
            outfitQty[slot] += qty;
        }
#endif
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (outfitQty[i] > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0)
            count++;
    }

    if (count > 0) {
        strcpy(playerInfoText[2], "");
        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAwardsHeader);
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
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAnd);
                        strcat(playerInfoText[2], " ");
                    }
                }
            }
            found = 0;
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 && rankShown[i] == 0) {
                    if (rankData[i].priority > maxWeight || found == 0)
                        maxWeight = rankData[i].priority;
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
                (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
                if (qty == 1) {
                    c = tolower(lcOutfitName[i][1]);
                    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAn);
                    else
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrA);
                } else if (qty == 2) {
                    StrcatIndCString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 2);
                } else if (qty == 3) {
                    StrcatIndCString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 3);
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
#endif

/* FrameDefaultButton (mac _FrameDefaultButton). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049d630
// MAC: _FrameDefaultButton i386 0x00055200 ppc 0x0005d578
void FrameDefaultButton(void *dlg, short flag)
{
#ifdef MACBUILD
    short itemType;
    Handle item;
    Rect box;

    if (gSystemVersion < 0x1000) {
        SetPort((GrafPtr)GetDialogPort(dlg));
        GetDialogItem(dlg, flag, &itemType, &item, &box);
        PenSize(3, 3);
        InsetRect(&box, -4, -4);
        FrameRoundRect(&box, 16, 16);
    }
#endif
}

/* ApplyPriceAndTechnologyFlux. STATE: MATCH 100.0%, exe 114 / obj 114, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0049d640
// MAC: _ApplyPriceAndTechnologyFlux i386 0x0004e6cd ppc 0x000572e4
long ApplyPriceAndTechnologyFlux(long cost, float multiplier, short itemTech, short portTech)
{
    long price;

    if (cost <= 0)
        return 0;

    price = cost;
    if (itemTech <= 5 && portTech <= 5 && itemTech < portTech && price >= 100)
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

#ifdef MAC1010
void HideTheMenuBar(void)
{
    if (!settings.windowed)
        HideMenuBar();
}

void ShowTheMenuBar(void)
{
    if (!settings.windowed)
        ShowMenuBar();
}
#elif defined(MACBUILD)
/* ShowTheMenuBar (mac _ShowTheMenuBar). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _ShowTheMenuBar i386 0x0005588b ppc 0x0005dac4
void ShowTheMenuBar(void)
{
#if V111
    if (!settings.windowed)
        SetSystemUIMode(kUIModeNormal, 0);
#endif
}

/* HideTheMenuBar. STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _HideTheMenuBar i386 0x000558b2 ppc 0x0005dae4
void HideTheMenuBar(void)
{
#if V111
    if (!settings.windowed)
        SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
#endif
}
#else
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
#endif


/* StartLoopingMovie. STATE: MATCH. OPEN: none */
// FUNCTION: EVNOVA 0x0049d7c0
// MAC: _StartLoopingMovie i386 0x00060266 ppc 0x00067fb8
void StartLoopingMovie(const unsigned char *name, void *window, Rect *box)
#ifdef MAC1010
{
    short resID;
    short vRefNum;
    long dirID;
    FSSpec spec;
    TimeBase timeBase;
    OSErr err;
    long flags;

    if (settings.noMovies != 0)
        return;
    if (loopingMovie != NULL)
        EndLoopingMovie();
    err = GetPluginSubfolder(gARef, "\pNova Plug-Ins", &dirID, &vRefNum);
    if (err != noErr) {
        err = GetPluginSubfolder(gARef, "\pNova Files", &dirID, &vRefNum);
        if (err != noErr)
            return;
    }
    FSMakeFSSpec(vRefNum, dirID, name, &spec);
    err = OpenMovieFile(&spec, &loopingMovieResRefNum, fsRdPerm);
    if (err != noErr) {
        err = GetPluginSubfolder(gARef, "\pNova Files", &dirID, &vRefNum);
        if (err != noErr)
            return;
        FSMakeFSSpec(vRefNum, dirID, name, &spec);
        err = OpenMovieFile(&spec, &loopingMovieResRefNum, fsRdPerm);
        if (err != noErr)
            return;
    }
    resID = 0;
    NewMovieFromFile((Movie *)&loopingMovie, loopingMovieResRefNum, &resID, NULL, newMovieActive, NULL);
    CloseMovieFile(loopingMovieResRefNum);
    if (err != noErr) {
        loopingMovie = NULL;
        return;
    }
#if TARGET_API_MAC_CARBON
    SetMovieGWorld(loopingMovie, GetCWindowGrafPort(window), 0);
#else
    SetMovieGWorld(loopingMovie, window, 0);
#endif
    SetMovieBox(loopingMovie, box);
    SetMoviePlayHints(loopingMovie, hintsLoop, hintsLoop);
    timeBase = GetMovieTimeBase(loopingMovie);
    flags = GetTimeBaseFlags(timeBase);
    SetTimeBaseFlags(timeBase, flags | loopTimeBase);
    GoToBeginningOfMovie(loopingMovie);
    SetMovieRate(loopingMovie, 0x10000);
    StartMovie(loopingMovie);
}
#elif defined(MACBUILD)
{
    HFSUniStr255 uniName;
    FSRef ref;
    Handle dataRef;
    OSType dataRefType;
    void *timeBase;
    short movieResId = -1;

    introMusicMovie = NULL;
    if (settings.noMovies != 0)
        return;
    EndLoopingMovie();
    HFSNameGetUnicodeName(name, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&pluginsFolder, uniName.length, uniName.unicode,
                           kTextEncodingUnknown, &ref) != noErr
        && FSMakeFSRefUnicode(&novaFilesFolder, uniName.length, uniName.unicode,
                              kTextEncodingUnknown, &ref) != noErr)
        goto fail;
    if (QTNewDataReferenceFromFSRef(&ref, 0, &dataRef, &dataRefType) != noErr)
        goto fail;
    if (NewMovieFromDataRef(&loopingMovie, newMovieActive, &movieResId, dataRef,
                            dataRefType) != noErr)
        goto fail;
    SetMovieGWorld(loopingMovie, GetCWindowGrafPort(window), 0);
    SetMovieBox(loopingMovie, box);
    SetMoviePlayHints(loopingMovie, hintsLoop, hintsLoop);
    timeBase = GetMovieTimeBase(loopingMovie);
    SetTimeBaseFlags(timeBase, GetTimeBaseFlags(timeBase) | loopTimeBase);
    GoToBeginningOfMovie(loopingMovie);
    SetMovieRate(loopingMovie, 0x10000);
    StartMovie(loopingMovie);
    return;
fail:
    loopingMovie = NULL;
}
#else
{
    short found;
    CGrafPtr port;
    void *timeBase;
    char cname[0x100];
    char path[0x100];
    char scratch[0x108];
    short movieResId = 0;
    CGrafPtr *windowPort = (CGrafPtr *)window;

    if (settings.noMovies != 0)
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
#ifdef CEFIX /* F07.01 */
    CE_ExpandPlugInsPath(path, path);
#else
    ExpandAPathToBufferNative(path, path);
#endif
    CStr2PStr(path, path);
    found = FSMakeFSSpec(0, 0, path, scratch);
    if ((int)found == 0)
        found = OpenMovieFile(scratch, &loopingMovieResRefNum, 1);
    if ((int)found != 0) {
        PStr2CStr(cname, name);
        sprintf(path, "9:Nova Files:%s", cname);
#ifdef CEFIX /* F07.02 */
        CE_ExpandFilesPath(path, path);
#else
        ExpandAPathToBufferNative(path, path);
#endif
        CStr2PStr(path, path);
        found = FSMakeFSSpec(0, 0, path, scratch);
        if ((int)found == 0)
            found = OpenMovieFile(scratch, &loopingMovieResRefNum, 1);
        if ((int)found != 0)
            return;
    }

    found = NewMovieFromFile(&loopingMovie, loopingMovieResRefNum, &movieResId, 0, 1, 0);
    CloseMovieFile(loopingMovieResRefNum);
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
#endif

/* LoopingMovieIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049da80
// MAC: _LoopingMovieIdle i386 0x000558d9 ppc 0x0005db04
void LoopingMovieIdle(void)
{
#if V111
    EventRecord ev;

#endif
    if (loopingMovie != NULL) {
        MoviesTask(loopingMovie, 0);
#ifdef MACBUILD
#if V111
        WaitNextEvent(0, &ev, gameLoaded && inBackground ? 30 : 0, NULL);
#endif
#else
        nv_UpdateDialogWindow(0);
        ScreenEndScene();
#endif
    }
}

/* EndLoopingMovie. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049dab0
// MAC: _EndLoopingMovie i386 0x00055934 ppc 0x0005db80
void EndLoopingMovie(void)
{
    if (loopingMovie != NULL) {
        DisposeMovie(loopingMovie);
        loopingMovie = NULL;
#ifndef MACBUILD
        FUN_004fc310(gIntroMovieData);
        gIntroMovieData = NULL;
#endif
    }
}

/* the movie's idle callback; taken by address at 0x0049deb1 */
#ifndef MACBUILD
/* IntroMovieIdleProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049daf0
short IntroMovieIdleProc(void)
{
    ScreenEndScene();
    return 0;
}
#endif

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
// MAC: _PlayQTMovie i386 0x00055957 ppc 0x0005dbc4
void PlayQTMovie(unsigned char *name, unsigned char doubled, unsigned char fade)
#ifdef MACBUILD
{
#ifdef MAC1010
    FSSpec spec;
    Rect box;
    Rect r;
    Rect scaled;
    Movie movie;
    long dirID;
    short fileRef;
    short movieResId;
    short vRefNum;
    long savedPaintWhite;
#else
    HFSUniStr255 uniName;
    FSRef ref;
    Handle dataRef;
    OSType dataRefType;
    Movie movie;
    short movieResId = -1;
    Rect scaled;
    Rect r;
    Rect box;
#endif
    WindowRef win;
    short dw, dh, w, h;
    float ratio;
    unsigned long last;
    EventRecord ev;
    OSErr err;

    if (settings.noMovies != 0)
        return;
#ifdef MAC1010
    if (GetPluginSubfolder(gARef, "\pNova Plug-Ins", &dirID, &vRefNum) != noErr)
        return;
    FSMakeFSSpec(vRefNum, dirID, name, &spec);
    err = OpenMovieFile(&spec, &fileRef, fsRdPerm);
    if (err != noErr) {
        if (GetPluginSubfolder(gARef, "\pNova Files", &dirID, &vRefNum) != noErr)
            return;
        FSMakeFSSpec(vRefNum, dirID, name, &spec);
        err = OpenMovieFile(&spec, &fileRef, fsRdPerm);
        if (err != noErr)
            return;
    }
    movieResId = 0;
    NewMovieFromFile(&movie, fileRef, &movieResId, NULL, newMovieActive, NULL);
    CloseMovieFile(fileRef);
    if (err != noErr)
        return;
#else
    HFSNameGetUnicodeName(name, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&pluginsFolder, uniName.length, uniName.unicode,
                           kTextEncodingUnknown, &ref) != noErr
        && FSMakeFSRefUnicode(&novaFilesFolder, uniName.length, uniName.unicode,
                              kTextEncodingUnknown, &ref) != noErr)
        return;
    if (QTNewDataReferenceFromFSRef(&ref, 0, &dataRef, &dataRefType) != noErr)
        return;
    if (NewMovieFromDataRef(&movie, newMovieActive, &movieResId, dataRef,
                            dataRefType) != noErr)
        return;
#endif
    if (fade != 0) {
        FadeScreenOut();
        HideTheCursor();
    }
    GetMovieBox(movie, &box);
    OffsetRect(&box, -box.left, -box.top);
    if (doubled != 0) {
        scaled.left = 0;
        scaled.top = 0;
        scaled.right = (box.right - box.left) * 2;
        scaled.bottom = (box.bottom - box.top) * 2;
        scaled.right++;
        scaled.bottom++;
        OffsetRect(&scaled, -scaled.left, -scaled.top);
        box = scaled;
    }
    SetMovieBox(movie, &box);
    if (fade != 0)
        r = *GetCWindowPortRect((WindowRef)gWind);
    else
        r = box;
    win = NewCWindow(NULL, &r, "\p", false, plainDBox, NULL, true, -1);
    CenterWindow(win, gWind);
    ShowWindowNoFlash(win);
    SetPort((GrafPtr)GetWindowPort(win));
    ForeColor(blackColor);
    PaintRect(GetWindowPortRect(win));
#ifndef MAC1010
    QDFlushPortBuffer(GetWindowPort(win), NULL);
#endif
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    savedPaintWhite = LMGetPaintWhite();
    LMSetPaintWhite(0);
#endif
    SelectWindow(win);
    BringToFront(win);
    ForeColor(blackColor);
    PaintRect(GetWindowPortRect(win));
#ifndef MAC1010
    QDFlushPortBuffer(GetWindowPort(win), NULL);
#endif
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    SetMovieGWorld(movie, GetWindowPort(win), 0);
#else
    SetMovieGWorld(movie, GetCWindowGrafPort(win), 0);
#endif
    w = GetWindowPortRect(win)->right - GetWindowPortRect(win)->left;
    h = GetWindowPortRect(win)->bottom - GetWindowPortRect(win)->top;
    dw = box.right - box.left;
    dh = box.bottom - box.top;
    /* BUG: both letterbox fits compute the new bottom from box.left, not box.top */
    if (dw > w) {
        ratio = (float)w / dw;
        box.right = box.left + dw * ratio;
        box.bottom = box.left + dh * ratio;
    }
    dw = box.right - box.left;
    dh = box.bottom - box.top;
    if (dh > h) {
        ratio = (float)h / dh;
        box.right = box.left + dw * ratio;
        box.bottom = box.left + dh * ratio;
    }
    CenterRect(&box, GetWindowPortRect(win));
    SetMovieBox(movie, &box);
    GoToBeginningOfMovie(movie);
    LoadMovieIntoRam(movie, 0, GetMovieDuration(movie), 0);
    if (fade != 0)
        FadeScreenIn();
    last = 0;
    StartMovie(movie);
    while (!IsMovieDone(movie)) {
        if (TickCount() > last + 6) {
            last = TickCount();
            MoviesTask(movie, 0);
#ifdef MAC1010
            BusyIdle();
#else
            WaitNextEvent(0, &ev, (gameLoaded != 0 && inBackground != 0) ? 30 : 0, NULL);
#endif
        }
        if (Button() || KeyCheck(MacKey(0x31)) || KeyCheck(MacKey(0x24)) || KeyCheck(MacKey(0x4c))) {
            FlushEvents(0xf, 0);
            break;
        }
    }
    StopMovie(movie);
    if (fade != 0) {
        FadeScreenOut();
        ShowTheCursor();
    }
    HideWindow(win);
    DisposeWindow(win);
#ifdef MAC1010
#if !TARGET_API_MAC_CARBON
    LMSetPaintWhite(savedPaintWhite);
#endif
#else
    if (fade != 0)
        FadeScreenIn();
#endif
    DisposeMovie(movie);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
}
#else
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

    if (settings.noMovies == 0) {
        PStr2CStr(nameBuf, name);
        sprintf(pathBuf, "9:Nova Plug-ins:%s", nameBuf);
#ifdef CEFIX /* F07.03 */
        CE_ExpandPlugInsPath(pathBuf, pathBuf);
#else
        ExpandAPathToBufferNative(pathBuf, pathBuf);
#endif
        CStr2PStr(pathBuf, pathBuf);
        r = FSMakeFSSpec(0, 0, pathBuf, &spec);
        if ((long)r == 0)
            r = OpenMovieFile(&spec, &refNum, 1);
        err = r;
        if (err == 0)
            goto found;
        PStr2CStr(nameBuf, name);
        sprintf(pathBuf, "9:Nova Files:%s", nameBuf);
#ifdef CEFIX /* F07.04 */
        CE_ExpandFilesPath(pathBuf, pathBuf);
#else
        ExpandAPathToBufferNative(pathBuf, pathBuf);
#endif
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
        ForeColor(blackColor);
        RedrawDialogs(1);
        return;
    }
}
#endif

/* PlayQTMovieFromDesc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049e3c0
// MAC: _PlayQTMovieFromDesc i386 0x00055f11 ppc 0x0005e194
void PlayQTMovieFromDesc(short id)
{
    extern char *strcpy(char *, const char *);   /* MSL, CALLED not inlined */

    strcpy(descTextPtr, "");
    LoadDescription(descTextPtr, id);
    if (descMovieName[0] != 0)
        PlayQTMovie(descMovieName, (descFlags & kDescMovieDoubled),
                    (descFlags & kDescMovieFade));
}

#ifdef MACBUILD
#define kSoundDoneMsg  1

OSErr DummyAEVTHandler(const AppleEvent *ev, AppleEvent *reply, long refCon);
OSErr PrefAEVTHandler(const AppleEvent *ev, AppleEvent *reply, long refCon);
OSErr QuitAEVTHandler(const AppleEvent *ev, AppleEvent *reply, long refCon);

/* PlanetSoundCallback (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _PlanetSoundCallback i386 0x00055f5d ppc 0x0005e210
void PlanetSoundCallback(long message)
{
    if (message == kSoundDoneMsg && dockedPort != NULL && (dockedPort->flags2 & kSpobLoopSound))
        PlaySound(planetSnd, 10, gSoundVolume, gSoundVolume);
}

/* InitAEVTHandlers (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _InitAEVTHandlers i386 0x00055fa3 ppc 0x0005e25c
void InitAEVTHandlers(void)
{
    OSErr err;
    long response;

    OAPPHandlerUPP = NewAEEventHandlerUPP((AEEventHandlerProcPtr)DummyAEVTHandler);
    err = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, OAPPHandlerUPP, 0, false);
    if (err == noErr) {
        ODOCHandlerUPP = NewAEEventHandlerUPP((AEEventHandlerProcPtr)DummyAEVTHandler);
        err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, ODOCHandlerUPP, 0, false);
    }
    if (err == noErr) {
        PDOCHandlerUPP = NewAEEventHandlerUPP((AEEventHandlerProcPtr)DummyAEVTHandler);
        err = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, PDOCHandlerUPP, 0, false);
    }
    if (err == noErr) {
        QUITHandlerUPP = NewAEEventHandlerUPP((AEEventHandlerProcPtr)QuitAEVTHandler);
        err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, QUITHandlerUPP, 0, false);
    }
    if (err == noErr) {
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        PREFHandlerUPP = NULL;
#else
        Gestalt(gestaltMenuMgrAttr, &response);
        if (response & gestaltMenuMgrAquaLayoutMask) {
            PREFHandlerUPP = NewAEEventHandlerUPP((AEEventHandlerProcPtr)PrefAEVTHandler);
            AEInstallEventHandler(kCoreEventClass, kAEShowPreferences, PREFHandlerUPP, 0, false);
        } else {
            PREFHandlerUPP = NULL;
        }
#endif
    }
}

/* GotRequiredAEVTParams (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _GotRequiredAEVTParams i386 0x0005611d ppc 0x0005e3f4
OSErr GotRequiredAEVTParams(const AppleEvent *ev)
{
    DescType returnedType;
    Size actualSize;
    OSErr err;
    OSErr result;

    err = AEGetAttributePtr(ev, keyMissedKeywordAttr, typeWildCard, &returnedType, NULL, 0,
                            &actualSize);
    if (err == errAEDescNotFound)
        result = noErr;
    else if (err == noErr)
        result = errAEEventNotHandled;
    else
        result = err;
    return result;
}

/* DummyAEVTHandler (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _DummyAEVTHandler i386 0x00056174 ppc 0x0005e454
OSErr DummyAEVTHandler(const AppleEvent *ev, AppleEvent *reply, long refCon)
{
    return noErr;
}

/* PrefAEVTHandler (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _PrefAEVTHandler i386 0x0005617b ppc 0x0005e45c
OSErr PrefAEVTHandler(const AppleEvent *ev, AppleEvent *reply, long refCon)
{
    DoPrefsDialog();
    AdjustSoundVolume();
    if (!gPause)
        RefreshSAT();
    return noErr;
}

/* QuitAEVTHandler (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _QuitAEVTHandler i386 0x0005619e ppc 0x0005e49c
OSErr QuitAEVTHandler(const AppleEvent *ev, AppleEvent *reply, long refCon)
{
#if TARGET_API_MAC_CARBON
    Str255 text;

    crashlog("got quit Apple event");
    if (gPause) {
        gQuit = 1;
    } else {
        SetRetainedWindow(gWind, false);
        if (gameLoaded && gameInProgress && !gPause)
            RefreshSAT();
        GetIndString(text, kAlertStrList, 10);
        if (EVYesNoDialog(text)) {
            gQuit = 1;
        } else {
            InvalWindowRect(gWind, GetWindowPortRect(gWind));
            SetRetainedWindow(gWind, true);
            RefreshSAT();
        }
    }
#else
    crashlog("got quit Apple event");
    gQuit = 1;
#endif
    return noErr;
}
#endif


#ifndef MACBUILD
/* FUN_0049e420. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049e420
void FUN_0049e420(void)
{
}
#endif

/* DrawEscortMenuInFrame (mac _DrawEscortMenuInFrame). STATE: MATCH 100.0%, exe
   305 / obj 305, reloc 57 with 0 hidden, byte-identical. The `(long)status'
   casts hold the exe's 32-bit sign-extended compares; a plain `long status'
   scores 98.0% because a named 32-bit local is coloured out of the temp ring.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x0049e430
// MAC: _DrawEscortMenuInFrame i386 0x00056268 ppc 0x0005e590
void DrawEscortMenuInFrame()
{
    Rect bar;
    Rect box;
    Rect *frame;
    Boolean flags[4];
    Str255 str;
    short i;
    short category;

    SWSetPortToFrame(floatingEscortMenuFrame);
    frame = &floatingEscortMenuFrame->frameRect;
    box = *frame;

    ForeColor(blackColor);
    PaintRect(&box);

    for (i = 0; i < 4; i++)
        flags[i] = 0;

    RGBForeColor(&floatingBorderColor);
    FrameRect(&box);

    ForeColor(whiteColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);

    GetIndString(str, kGameStrList, 0x85);
#ifdef CEFIX /* F12.68 */
    CE_SetPen((box.left + box.right) / 2 - StringWidth(str) / 2, CE_Scale(box.top + 0xf));
#else
    MoveTo((box.left + box.right) / 2 - StringWidth(str) / 2, box.top + 0xf);
#endif
    DrawString(str);

    for (i = 1; i < 64; i++) {
        if (!shipData[i].active)
            continue;
        if (shipData[i].parentShip != 0)
            continue;
        category = shipClass[shipData[i].type].escortType;
        if (category < 0 || category > 3)
            continue;
        flags[category] = 1;
    }

    for (i = -1; i < 4; i++) {
        if (selEscortGroup == i) {
            bar.left = box.left + 4;
            bar.right = box.right - 4;
            bar.top = box.top + 0x1e + (i + 1) * 18;
            bar.bottom = box.top + 0x2b + (i + 1) * 18;
            RGBForeColor(&escortMenuColor);
#ifdef CEFIX /* F12.71 */
            bar.top = CE_Scale(bar.top);
            bar.bottom = CE_Scale(bar.bottom);
            PaintRect(&bar);
#else
            PaintRect(&bar);
#endif
        }

        if (i == -1)
            ForeColor(whiteColor);
        else if (flags[i] != 0)
            ForeColor(whiteColor);
        else
            RGBForeColor(&rgbDarkGrey);

#ifdef CEFIX /* F12.69 */
        CE_SetPen(box.left + 7, CE_Scale(box.top + 0x28 + (i + 1) * 18));
#else
        MoveTo(box.left + 7, box.top + 0x28 + (i + 1) * 18);
#endif
        NumToString(i + 2, str);
        DrawString(str);
        DrawString("\p) ");

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
            if ((long)playerEscortOrders[i] == 1)
                GetIndString(str, kGameStrList, 0x91);
            else if ((long)playerEscortOrders[i] == 2)
                GetIndString(str, kGameStrList, 0x92);
            else if ((long)playerEscortOrders[i] == 4)
                GetIndString(str, kGameStrList, 0x93);
            else if ((long)playerEscortOrders[i] == 3)
                GetIndString(str, kGameStrList, 0x94);
            else
                GetIndString(str, kGameStrList, 0x95);
            ForeColor(whiteColor);
#ifdef CEFIX /* F12.70 */
            CE_SetPen(box.right - StringWidth(str) - 7, CE_Scale(box.top + 0x28 + (i + 1) * 18));
#else
            MoveTo(box.right - StringWidth(str) - 7, box.top + 0x28 + (i + 1) * 18);
#endif
            DrawString(str);
        }
    }

    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
}

#define kDefaultMapZoom 0x20

/* ShowEscortMenu. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049e8d0
// MAC: _ShowEscortMenu i386 0x0005664a ppc 0x0005e980
void ShowEscortMenu(void)
{
    escortMenuOnscreen = kDefaultMapZoom;
    DrawEscortMenuInFrame();
}

#ifdef MACBUILD
/* GetWindowDragBounds (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _GetWindowDragBounds i386 0x0005665d ppc 0x0005e994
void GetWindowDragBounds(Rect *bounds, Point where)
{
    Point origin;
    Rect portRect;
    GDHandle device;

    bounds->left = bounds->right = bounds->top = bounds->bottom = 0;
    if (useLegacyRenderingPipeline && gSpriteWorld != NULL && gSpriteWorld->mainSWGDH != NULL) {
        device = gSpriteWorld->mainSWGDH;
        *bounds = (*device)->gdRect;
        SWSetPortToWindow(gSpriteWorld);
        origin.v = 0;
        origin.h = 0;
        LocalToGlobal(&origin);
        portRect = *GetCWindowPortRect((WindowRef)gWind);
        OffsetRect(&portRect, -portRect.left, -portRect.top);
        OffsetRect(&portRect, origin.h, origin.v);
        bounds->left = where.h - portRect.left;
        bounds->right -= portRect.right - where.h;
        bounds->top = GetMBarHeight() + where.v - portRect.top;
        bounds->bottom -= portRect.bottom - where.v;
    }
}

/* ForceWindowOnscreen (mac only). STATE: mac MATCH i386 and ppc. OPEN: none */
// MAC: _ForceWindowOnscreen i386 0x0005677b ppc 0x0005eae4
void ForceWindowOnscreen(void)
{
    Rect bounds;
    Rect screen;
    short h;
    short v;
    short left;
    short top;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    Point pt;
#endif

    if (useLegacyRenderingPipeline) {
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        bounds = *GetCWindowPortRect((WindowRef)gWind);
        OffsetRect(&bounds, -bounds.left, -bounds.top);
        pt.v = 0;
        pt.h = 0;
        SetPort((GrafPtr)gWind);
        LocalToGlobal(&pt);
        OffsetRect(&bounds, pt.h, pt.v);
        screen = (*GetMainDevice())->gdRect;
        h = left = pt.h;
        v = top = pt.v;
#else
        GetWindowBounds(gWind, kWindowContentRgn, &bounds);
        left = bounds.left;
        top = bounds.top;
        screen = (*GetMainDevice())->gdRect;
        h = left;
        v = top;
#endif
        if (bounds.left < screen.left)
            h = left + (screen.left - bounds.left);
        if (bounds.top < screen.top)
            v += screen.top - bounds.top;
        if (bounds.right > screen.right)
            h -= bounds.right - screen.right;
        if (bounds.bottom > screen.bottom)
            v -= bounds.bottom - screen.bottom;
#ifdef MAC1010 /* 1.0.10 spelling */
        if (h != left || v != top)
            MoveWindow((WindowRef)gWind, h, v, false);
#else
        if (h != left)
            MoveWindow(gWind, h, v, false);
        else if (v != top)
            MoveWindow(gWind, h, v, false);
#endif
    }
}
#endif


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
// MAC: _DoQuantityDialog i386 0x00056d5b ppc 0x0005f1e4
long DoQuantityDialog(long maxCount, short mode)
{
#ifdef MAC1010
    long value;
    Handle item;
    short itemHit;
    Boolean done;
    Rect box;
    short itemType;
    Str255 text;
#elif V111 /* V111 spelling */
    long value;
    Str255 text;
    short itemType;
    Handle item;
    short itemHit;
    Boolean done;
    short i;
    Rect box;
#else
    Handle item;
    short itemHit;
    long value;
    Boolean done;
    short i;
    Rect box;
    short itemType;
    Str255 text;
#endif
    Str255 digits;
    Boolean allDigits;
#ifdef MAC1010
    short i;
#endif
    short destIdx;
    long result;

    qtyDialog = NULL;
    qtyDialog = GetNewDialog(kQtyDlogID, NULL, NULL);
    if (qtyDialog == NULL)
        return 0;

    CenterWindow(FrontWindow(qtyDialog), gWind);
    ShowMacWindow(FrontWindow(qtyDialog));
    SelectWindow(FrontWindow(qtyDialog));
    SetDialogGWorld(qtyDialog);
    FrameDefaultButton(qtyDialog, 1);
    SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);

    GetDialogItem(qtyDialog, kQtyPromptItem, &itemType, &item, &box);
    if (mode == 1)
        SetControlTitle((ControlRef)item,buttonLabel[2]);
    else if (mode == 2)
        SetControlTitle((ControlRef)item,buttonLabel[0x3a]);
    else
        SetControlTitle((ControlRef)item,buttonLabel[1]);
    if (mode == 2)
        GetIndString(text, kGameStrList, 0x174);
    else
        GetIndString(text, kGameStrList, 0x173);

    GetDialogItem(qtyDialog, kQtyMaxItem, &itemType, &item, &box);
    SetDialogItemText((Handle)item,text);

    GetDialogItem(qtyDialog, kQtyTextItem, &itemType, &item, &box);
    NumToString(maxCount, text);
    SetDialogItemText((Handle)item,text);

    done = 0;
    result = 0;
    do {
        ModalDialog(0, &itemHit);
        if (itemHit == 4) {
            done = 1;
        } else if (itemHit == 1) {
            GetDialogItem(qtyDialog, kQtyTextItem, &itemType, &item, &box);
            GetDialogItemText((Handle)item,text);
            if (text[0] != 0) {
                allDigits = 1;
                digits[0] = 0;
                destIdx = 1;
                for (i = 0; i < text[0]; i++) {
                    if (text[i + 1] >= '0' && text[i + 1] <= '9') {
                        digits[0]++;
                        digits[destIdx] = text[i + 1];
                        destIdx++;
                    } else {
                        allDigits = 0;
                    }
                }
                if (allDigits) {
                    StringToNum(text, &value);
                    if (value >= 0 && value <= maxCount) {
                        result = value;
                        done = 1;
                    } else {
                        SysBeep(0);
                        if (value < 0)
                            NumToString(0, text);
                        else
                            NumToString(maxCount, text);
                        SetDialogItemText((Handle)item,text);
                        SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
                    }
                } else {
                    SysBeep(0);
                    SetDialogItemText((Handle)item,digits);
                    SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
                }
            } else {
                SysBeep(0);
                SetDialogItemText((Handle)item,(const unsigned char *)"\p");
#if !defined(MACBUILD) && !V111
                SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
#endif
            }
        }
    } while (!done);

    DisposeDialogNoFlash(qtyDialog);
    qtyDialog = NULL;
    return result;
}

#define kMapWorldWide 4096
#define kMapWorldHigh 32

/* SetupDialogs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049ed00
// MAC: _SetupDialogs i386 0x0005715d ppc 0x0005f500
void SetupDialogs(void)
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
        iconCache[i].picID = -1;
        iconCache[i].stamp = 0;
    }
    nagCount = 0;
#ifdef CEFIX /* F12.72 */
    SetRect(&r, 0, 0, ceGridThumbSize * 128, ceGridThumbSize);
#else
    SetRect(&r, 0, 0, kMapWorldWide, kMapWorldHigh);
#endif
#ifdef MACBUILD
    CreateGWorldContainer(&iconsPort, GetMainDevice(), 0, &r);
#else
    CreateGWorldContainer(&iconsPort, 0, 0, &r);
#endif
}

/* DisposeDialogs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049eec0
// MAC: _DisposeDialogs i386 0x000572b0 ppc 0x0005f668
void DisposeDialogs(void)
{
    if (iconsPort.world != NULL)
        DisposeGWorldContainer(&iconsPort);
}

#ifndef MACBUILD
/* CrtInitHook5. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0049eee0
void CrtInitHook5(void)
{
}
#endif

EXTERN_C_END

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
#endif
