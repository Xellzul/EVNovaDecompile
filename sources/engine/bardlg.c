// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* bardlg.c -- the bar and dialog-update object (tu-map O21, win 0x0047c8e0..0x0047e1e0). Notes: Research/notes/bardlg.md */
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "dialogs.h"
#include "sound.h"
#include "game.h"
#include "stellar.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "burger.h"

#define kBarDlogID          0x3fd   /* DLOG 1021, with portrait */
#define kPlainBarDlogID     0x3f5   /* DLOG 1013 */
#define kBarPictID          0x2138
#define kPlainBarPictID     0x2137
#define kBarVenue           1
#define kBrokeStrIndex      0x169

PicHandle gDlg1014Pict;
PicHandle barPortraitPict;
PicHandle barDialogPict;
PicHandle raceDialogPict;
PicHandle raceButtonPic[4][4];
DialogPtr barDialog;
DialogPtr newsDialog;
DialogPtr raceDialog;
char barDialogText[2048];
char newsString[2048];
char sponsorString[256];
unsigned char DAT_007d179c[8];
short racePick;
short raceWinner;
short lastRaceWinner;
short raceCount;
unsigned char DAT_007d17ac[8];
long raceBetAmount;

extern "C" {

/* DoBarDialog (mac _DoBarDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047c8e0
void DoBarDialog(void)
{
    char done;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short i;
    short shown;

    done = 0;
    savedStellar = shipData->targetStellar;
    savedRoute = shipData->navTargetKind;
    RandomizeNewsDialog();
    missionListVenue = 1;
    offeringFromShip = 0;
    if (dockedPortMissions[1][0] == -1)
        nextPortMission = -1;
    else
        nextPortMission = 0;
    missionOfferTime = ReadTick() + 0xf;
    lastRaceWinner = -1;
    strncpy(barDialogText, "", 3);
    LoadDescription(barDialogText, shipData->targetStellar + 10000);
    barPortraitPict = NULL;
    if (descResourcePictId >= kFirstResourceID)
        barPortraitPict = GetPicture(descResourcePictId);
    barDialog = NULL;
    if (barPortraitPict != NULL)
        barDialog = GetNewDialog(kBarDlogID, UpdateBarDialog, BarDialogEvent);
    else
        barDialog = GetNewDialog(kPlainBarDlogID, UpdateBarDialog,
                                 BarDialogEvent);
    if (barDialog == NULL)
        return;
    if (barPortraitPict != NULL)
        barDialogPict = GetPicture(kBarPictID);
    else
        barDialogPict = GetPicture(kPlainBarPictID);
    CenterWindow(FrontWindow(barDialog), FrontWindow(portDialog));
    ShowWindowNoFlash(FrontWindow(barDialog));
    FlushEvents(everyEvent - diskMask, 0);
    SetDialogGWorld(barDialog);
    RedrawDialog(barDialog);
    UpdateBarDialog();
    FrontWindow(barDialog);
    FrontWindow(barDialog);
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(barDialog, BarDialogEvent, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2) {
            if (shipData->credits > 0) {
                DoRaceDialog();
            } else {
                strcpy(gTextShowString, "");
                AppendIndString(gTextShowString, kGameStrList,
                                kBrokeStrIndex);
                ShowTextDialog(0, 0, 0);
            }
            UpdateStellarDialog();
            SetDialogGWorld(barDialog);
            UpdateBarDialog();
        }
        if (itemHit == 3) {
            SetDialogGWorld(barDialog);
            DrawBarButtons(-1);
            ShowDlg1014();
            UpdateStellarDialog();
            SetDialogGWorld(barDialog);
            UpdateBarDialog();
        }
        if (itemHit == 5) {
            if (CanHireEscorts()) {
                SetDialogGWorld(barDialog);
                DrawBarButtons(-1);
                shipBuyMode = 1;
                DoShipyardDialog();
                ShipStatsToSystemInfo();
                DrawStatusCargo();
                RedrawDialogs(1);
            }
        }
        if (itemHit == 6) {
            if (OfferMissionFromPort(kBarVenue)) {
                SetDialogGWorld(portDialog);
                UpdateStellarDialog();
                SetDialogGWorld(barDialog);
                UpdateBarDialog();
            }
        }
        if (itemHit == 8) {
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            UpdateAllPortDialogs();
            SetDialogGWorld(barDialog);
            RedrawDialog(barDialog);
        }
        if (itemHit == 9) {
            DoPlayerInfoDialog();
            UpdateAllPortDialogs();
            SetDialogGWorld(barDialog);
            RedrawDialog(barDialog);
        }
        if (itemHit == 0xa) {
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
                UpdateAllPortDialogs();
                SetDialogGWorld(barDialog);
                RedrawDialog(barDialog);
            }
        }
    } while (done == 0);
    if (barDialogPict != NULL) {
        QDReleaseResource(barDialogPict);
        barDialogPict = NULL;
    }
    if (barPortraitPict != NULL) {
        QDReleaseResource(barPortraitPict);
        barPortraitPict = NULL;
    }
    DisposeDialogNoFlash(barDialog);
    barDialog = NULL;
    ResetMissionOfferVenue();
}

/* nothing here reaches into a dialog; the accessors do all of it */


/* the oops record, this file's own copy; the disaster handler models the same
   0x210-byte record for the fields it needs */

/* The porters' no-op window stubs. The mac twin calls SelectWindow and
   BringToFront here and the win exe emits neither call, but it DOES still
   evaluate each one's argument, so they cannot have been deleted from the
   source -- only expanded to nothing. */
#define SelectWindow(w)  ((void)(w))
#define BringToFront(w)  ((void)(w))

#define kSponsorStrList      0x1fa4  /* STR# 8100, the sponsor lines */
#define kFillerNewsStrList   0x1fa5  /* STR# 8101, used when nothing qualifies */
#define kStrSponsor          0xbe
#define kStrNoNews           0xbf
#define kStrDisasterVerb     0xc0
#define kStrDisasterOn       0xc1
#define kStrDisasterIn       0xc2
#define kStrDisasterAffects  0xb5
#define kStrDisasterAt       0x3c

#define kDlg1014ID           0x3f6
#define kRaceDialogID        0x3ff
#define kDefaultBackdropPict 9000
#define kRaceBackdropPict    0x2151
#define kRaceRunnerPict      0x2152  /* +i per racer, +10 per animation frame */
#define kRacerCount          4
#define kRaceFirstButton     3       /* itemHit 3..6 pick a racer */
#define kRaceHelpDesc        0x7ffc
#define kRaceResultDesc      0x7ff8  /* + the winning racer */
#define kRaceWinStrIndex     370
#define kRacePayoutOdds      4
#define kMaxRaceBet          10000
#define kQuickRaceBet        1000
#define kKeyLeftAlt          0x38
#define kKeyRightAlt         0x6f    /* by the 0x1d/0x6b left/right control
                                        precedent elsewhere in the tree */
#define kTitleTop     0x8c
#define kTitleBottom  0xb4
#define kBodyTop      0xaa
#define kTextInset    10
#define kBodyMargin   4

/*
 * Rewriting ev->what to mouseDown on the way out is how the caller is told
 * this was a key that acts like a click.
 */

/* BarDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047cdb0
char BarDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int c;
    int key;

    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 1;
        return 1;
    }
    if ((int)ev->what == kKeyDown) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map)) {
            ev->what = kMouseDown;
            *itemHit = 8;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            ev->what = kMouseDown;
            *itemHit = 9;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            ev->what = kMouseDown;
            *itemHit = 10;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        if (c == kReturnChar || c == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        key = tolower(c);
        if (key == 'g' || key == 'r') {
            ev->what = kMouseDown;
            *itemHit = 2;
            return 1;
        }
        if (key == 'w' || key == 'n') {
            ev->what = kMouseDown;
            *itemHit = 3;
            return 1;
        }
        if (key == 'h' || key == 'e') {
            ev->what = kMouseDown;
            *itemHit = 5;
            return 1;
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdateBarDialog();
        button = TrackBarButtons(where);
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
        if (button == 4) {
            *itemHit = 5;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (ReadTick() >= missionOfferTime) {
        *itemHit = 6;
        missionOfferTime = ReadTick() + Rand(30) + 30;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        UpdateBarDialog();
        return 0;
    }
    return 0;
}

/* UpdateBarDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047cfe0
void UpdateBarDialog(void)
{
    Rect box;
    short itemType;
    void *item;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(barDialog));
    if (barDialogPict != NULL)
        DrawPicHandle(barDialogPict, GetDialogPortRect(barDialog));
    if (barPortraitPict != NULL) {
        GetDialogItem(barDialog, 8, &itemType, &item, &box);
        RGBForeColor(blackColor);
        DrawPicHandle(barPortraitPict, &box);
    }
    GetDialogItem(barDialog, 7, &itemType, &item, &box);
    RGBForeColor(blackColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    TETextBox(barDialogText, strlen(barDialogText), &box, 0);
    InvertRect(&box);
    RGBForeColor(blackColor);
    DrawBarButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(barDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(barDialog)),
             GetDialogPortRect(barDialog), GetDialogPortRect(barDialog), 0, NULL);
}

/* ShowDlg1014. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047d180
void ShowDlg1014(void)
{
    short govtPict;
    char done;
    short pictID;
    short itemHit;

    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    pictID = kDefaultBackdropPict;
    if (dockedPort->govt != -1) {
        govtPict = govtData[dockedPort->govt].newsPict;
        if (govtPict != -1)
            pictID = govtPict;
    }
    gDlg1014Pict = GetPicture(pictID);
    if (gDlg1014Pict == NULL && pictID != kDefaultBackdropPict)
        gDlg1014Pict = GetPicture(kDefaultBackdropPict);
    newsDialog = NULL;
    newsDialog = GetNewDialog(kDlg1014ID, UpdateDlg1014, Dlg1014EventProc);
    if (newsDialog == NULL)
        return;
    CenterWindow(FrontWindow(newsDialog), FrontWindow(barDialog));
    ShowWindowNoFlash(FrontWindow(newsDialog));
    SetDialogGWorld(newsDialog);
    FlushEvents(everyEvent - diskMask, 0);
    UpdateDlg1014();
    FrontWindow(newsDialog);
    FrontWindow(newsDialog);
    done = 0;
    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(newsDialog, Dlg1014EventProc, &itemHit, NULL);
        if (itemHit == 1)
            done = 1;
    } while (done == 0);
    QDReleaseResource(gDlg1014Pict);
    gDlg1014Pict = NULL;
    DisposeDialogNoFlash(newsDialog);
    newsDialog = NULL;
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
}

/* UpdateDlg1014. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047d370
void UpdateDlg1014(void)
{
    Rect box;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(newsDialog));
    DrawPicHandle(gDlg1014Pict, GetDialogPortRect(newsDialog));
    SetRect(&box, GetDialogPortRect(newsDialog)->left + kTextInset,
            GetDialogPortRect(newsDialog)->top + kTitleTop,
            GetDialogPortRect(newsDialog)->right - kTextInset,
            GetDialogPortRect(newsDialog)->top + kTitleBottom);
    TETextBox(sponsorString, strlen(sponsorString), &box, 0);
    InvertRect(&box);
    SetRect(&box, GetDialogPortRect(newsDialog)->left + kTextInset,
            GetDialogPortRect(newsDialog)->top + kBodyTop,
            GetDialogPortRect(newsDialog)->right - kTextInset,
            GetDialogPortRect(newsDialog)->bottom - kBodyMargin);
    TETextBox(newsString, strlen(newsString), &box, 0);
    InvertRect(&box);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(newsDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(newsDialog)),
             GetDialogPortRect(newsDialog), GetDialogPortRect(newsDialog), 0, NULL);
}

/* Dlg1014EventProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047d560
char Dlg1014EventProc(long unused, EventRecord *ev, short *itemHit)
{
    short v;
    short h;

    *itemHit = -1;
    if ((int)ev->what == kKeyDown &&
        ((ev->message & kCharCodeMask) == kReturnChar ||
         (ev->message & kCharCodeMask) == kEscapeChar)) {
        *itemHit = 1;
        return 1;
    }
    if ((int)ev->what == kMouseDown) {
        v = ev->where.v;
        h = ev->where.h;
        SetDialogGWorld(newsDialog);
        if (PtInRect(h, v, GetDialogPortRect(newsDialog)) != 0) {
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what != kUpdateEvt)
        return 0;
    UpdateDlg1014();
    return 0;
}

#define kRaceButtonCount 6

/* RandomizeNewsDialog. STATE: MATCH 100.0%, exe 404 / obj 404, frame 0x80c 6/6 slots and refs,
   byte-identical yes, relocregcheck 0 of 91. OPEN: none. The dead licence parity counts in
   `pick'. see notes */
// FUNCTION: EVNOVA 0x0047d600
void RandomizeNewsDialog(void)
{
    char cronActive[kMaxCrons];
    char anyActiveCron;
    char anyGovtNews;
    short i;
    short cronNewsID[kMaxCrons];
    Str255 tmp;
    Str255 line;
    long code;
    short eligible;
    short matching;
    short j;
    short pick;
    unsigned long long mem;

    mem = fakeLicenseCode;
    pick = 0;
    code = mem;
    code ^= mem >> 32;
    while (code != 0) {
        code &= code - 1;
        pick = 1 - pick;
    }

    GetRandomIndString((unsigned char *)sponsorString, kSponsorStrList);
    if (sponsorString[0] != 0) {
        PtoCstr((unsigned char *)sponsorString);
    } else {
        strcpy(sponsorString, "");
        AppendIndString(sponsorString, kGameStrList, kStrSponsor);
    }

    strcpy(newsString, "");
    eligible = 0;
    matching = 0;
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].countdown > 1) {
            eligible++;
            if (shipData[0].targetStellar == disasterData[i].stellarID ||
                disasterData[i].stellar == -2)
                matching++;
        }
    }
    if (eligible > 0) {
        if (matching > 0) {
            do {
                pick = Rand(kMaxDisasters);
            } while (disasterData[pick].loaded == 0 || disasterData[pick].countdown <= 1 ||
                     (shipData[0].targetStellar != disasterData[pick].stellarID &&
                      disasterData[pick].stellar != -2));
        } else {
            do {
                pick = Rand(kMaxDisasters);
            } while (disasterData[pick].loaded == 0 || disasterData[pick].countdown <= 1);
        }
        pstrcpy(line, "\p");
        if (disasterData[pick].stellarID != -1 && disasterData[pick].name[0] != 0) {
            strcpy((char *)line, "");
            AppendIndString((char *)line, kGameStrList, kStrSponsor);
            strcat((char *)line, " ");
            pstrcpy(tmp, disasterData[pick].name);
            PtoCstr(tmp);
            strcat((char *)line, (char *)tmp);
            strcat((char *)line, " ");
            AppendIndString((char *)line, kGameStrList, kStrDisasterVerb);
            strcat((char *)line, " ");
            if (disasterData[pick].amount > 0)
                AppendIndString((char *)line, kGameStrList, kStrDisasterOn);
            else
                AppendIndString((char *)line, kGameStrList, kStrDisasterIn);
            strcat((char *)line, " ");
            AppendIndString((char *)line, kGameStrList, kStrDisasterAffects);
            strcat((char *)line, " ");
            pstrcpy(tmp, lcCargoName[disasterData[pick].commodity]);
            PtoCstr(tmp);
            strcat((char *)line, (char *)tmp);
            strcat((char *)line, " ");
            AppendIndString((char *)line, kGameStrList, kStrDisasterAt);
            strcat((char *)line, " ");
            pstrcpy(tmp, stellarData[disasterData[pick].stellarID].name);
            PtoCstr(tmp);
            strcat((char *)line, (char *)tmp);
            strcat((char *)line, ".");
            strcpy(newsString, (char *)line);
        }
    }

    anyGovtNews = 0;
    anyActiveCron = 0;
    i = 0;
    for (i = 0; i < kMaxCrons; i++) {
        cronNewsID[i] = -2;
        cronActive[i] = 0;
        if (gCrons[i].active != 0 && gCrons[i].loaded != 0 &&
            gCrons[i].holdoffLeft <= 0) {
            for (j = 0; j < 4; j++) {
                if (gCrons[i].govts[j] != -1 &&
                    GovtAllies(gCrons[i].govts[j],
                               stellarData[shipData[0].targetStellar].govt)
                        != 0) {
                    cronNewsID[i] = gCrons[i].govtValues[j];
                    if (cronNewsID[i] > 0)
                        anyGovtNews = 1;
                }
            }
            if (cronNewsID[i] < -1 && gCrons[i].indNewsStr > 0) {
                cronActive[i] = 1;
                anyActiveCron = 1;
            }
        }
    }
    if (anyGovtNews != 0 || anyActiveCron != 0) {
        if (anyGovtNews != 0) {
            do {
                pick = Rand(kMaxCrons);
            } while (cronNewsID[pick] <= 0);
            GetRandomIndString((unsigned char *)newsString, cronNewsID[pick]);
        } else {
            do {
                pick = Rand(kMaxCrons);
            } while (cronActive[pick] == 0);
            GetRandomIndString((unsigned char *)newsString, gCrons[pick].indNewsStr);
        }
        PtoCstr((unsigned char *)newsString);
    }
    if (strlen(newsString) == 0) {
        GetRandomIndString((unsigned char *)newsString, kFillerNewsStrList);
        if (newsString[0] != 0) {
            PtoCstr((unsigned char *)newsString);
        } else {
            extern char *strcpy(char *, const char *);

            strcpy(newsString, "");
            AppendIndString(newsString, kGameStrList, kStrNoNews);
        }
    }
}

/* DoRaceDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047dc50
void DoRaceDialog(void)
{
    short itemHit;
    char done;
    short i;
    short j;
    long payout;

    raceDialog = NULL;
    raceDialog = GetNewDialog(kRaceDialogID, UpdateRaceDialog,
                              RaceDialogEventProc);
    if (raceDialog == NULL)
        return;
    raceWinner = -1;
    racePick = -1;
    raceCount = 0;
    raceDialogPict = GetPicture(kRaceBackdropPict);
    for (i = 0; i < kRacerCount; i++) {
        raceButtonPic[i][0] = GetPicture(kRaceRunnerPict + i);
        raceButtonPic[i][1] = GetPicture(kRaceRunnerPict + 10 + i);
        raceButtonPic[i][2] = GetPicture(kRaceRunnerPict + 20 + i);
        raceButtonPic[i][3] = GetPicture(kRaceRunnerPict + 30 + i);
    }
    CenterWindow(FrontWindow(raceDialog), FrontWindow(portDialog));
    ShowWindowNoFlash(FrontWindow(raceDialog));
    FlushEvents(everyEvent - diskMask, 0);
    SetDialogGWorld(raceDialog);
    RedrawDialog(raceDialog);
    UpdateRaceDialog();
    SelectWindow(FrontWindow(raceDialog));
    BringToFront(FrontWindow(raceDialog));
    done = 0;
    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(raceDialog, RaceDialogEventProc, &itemHit, NULL);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2) {
            LoadDescription(gTextShowString, kRaceHelpDesc);
            ShowTextDialog(0, 0, 0);
            SetDialogGWorld(raceDialog);
            UpdateRaceDialog();
        }
        if (itemHit >= kRaceFirstButton &&
            itemHit < kRaceFirstButton + kRacerCount) {
            if (KeyCheck(kKeyLeftAlt) != 0 || KeyCheck(kKeyRightAlt) != 0)
                raceBetAmount = DoQuantityDialog(
                    shipData[0].credits > kMaxRaceBet ? kMaxRaceBet
                                                      : shipData[0].credits, 2);
            else if (shipData[0].credits < kQuickRaceBet)
                raceBetAmount = shipData[0].credits;
            else
                raceBetAmount = kQuickRaceBet;
            if (raceWinner == -1 && raceBetAmount > 0) {
                racePick = itemHit - kRaceFirstButton;
                shipData[0].credits -= raceBetAmount;
                statusRedraw[2] = 1;
                DrawStatus();
                raceWinner = Rand(kRacerCount);
                if (raceWinner == lastRaceWinner) {
                    do {
                        raceWinner = Rand(kRacerCount);
                    } while (raceWinner == lastRaceWinner ||
                             raceWinner == racePick);
                }
                lastRaceWinner = raceWinner;
                raceCount++;
                RedrawDialog(raceDialog);
                LoadDescription(gTextShowString,
                                (raceWinner + kRaceResultDesc));
                if (raceWinner == racePick) {
                    payout = raceBetAmount * kRacePayoutOdds;
                    strcat(gTextShowString, "\r\r");
                    AppendIndString(gTextShowString, kGameStrList,
                                    kRaceWinStrIndex);
                    strcat(gTextShowString, ":  ");
                    StrcatCommaNumber(payout, gTextShowString);
                    strcat(gTextShowString, " ");
                    if (payout == 1)
                        strcat(gTextShowString, creditCString);
                    else
                        strcat(gTextShowString, creditsCString);
                    strcat(gTextShowString, ".");
                }
                ShowTextDialog(0, 0, 1);
                if (raceWinner == racePick) {
                    shipData[0].credits += payout;
                    statusRedraw[2] = 1;
                    DrawStatus();
                }
            } else {
                raceWinner = -1;
                racePick = -1;
            }
            SetDialogGWorld(raceDialog);
            RedrawDialog(raceDialog);
            UpdateRaceDialog();
        }
    } while (done == 0);
    if (raceDialogPict != NULL) {
        QDReleaseResource(raceDialogPict);
        raceDialogPict = NULL;
    }
    for (i = 0; i < kRacerCount; i++) {
        for (j = 0; j <= 3; j++) {
            if (raceButtonPic[i][j] != NULL) {
                QDReleaseResource(raceButtonPic[i][j]);
                raceButtonPic[i][j] = NULL;
            }
        }
    }
    DisposeDialogNoFlash(raceDialog);
    raceDialog = NULL;
}

/* RaceDialogEventProc. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e150
char RaceDialogEventProc(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;

    if ((int)ev->what == kKeyDown &&
        ((ev->message & kCharCodeMask) == kReturnChar ||
         (ev->message & kCharCodeMask) == kEscapeChar)) {
        *itemHit = 1;
        return 1;
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdateRaceDialog();
        button = TrackRaceButtons(where);
        if (button >= 0 && button < kRaceButtonCount) {
            *itemHit = button + 1;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what != kUpdateEvt)
        return 0;
    UpdateRaceDialog();
    return 0;
}

/* UpdateRaceDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e1e0
void UpdateRaceDialog(void)
{
    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(raceDialog));
    if (raceDialogPict != NULL)
        DrawPicHandle(raceDialogPict, GetDialogPortRect(raceDialog));
    RGBForeColor(blackColor);
    DrawRaceButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(raceDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(raceDialog)),
             GetDialogPortRect(raceDialog), GetDialogPortRect(raceDialog), 0, NULL);
}

}
