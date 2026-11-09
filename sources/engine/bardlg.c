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
#include "cefix.h"

#define kBarDlogID          0x3fd   /* DLOG 1021, with portrait */
#define kPlainBarDlogID     0x3f5   /* DLOG 1013 */
#define kBarPictID          0x2138
#define kPlainBarPictID     0x2137
#define kBarVenue           1
#define kBrokeStrIndex      0x169

PicHandle newsPic;
PicHandle barPic;
PicHandle barBackgroundPic;
PicHandle raceBackgroundPic;
PicHandle raceButtonPic[4][4];
DialogPtr barDialog;
DialogPtr newsDialog;
DialogPtr raceDialog;
char barDescText[2048];
char newsString[2048];
char sponsorString[256];
#ifndef MACBUILD
unsigned char DAT_007d179c[8];
#endif
short racePick;
short raceWinner;
short lastRaceWinner;
short raceCount;
#ifndef MACBUILD
unsigned char DAT_007d17ac[8];
#endif
long raceBetAmount;

EXTERN_C_BEGIN

#ifdef MACBUILD
static inline void LicenseParity(void)
{
    short parity;
    unsigned long code;

    code = fakeLicenseCode;
    parity = 0;
    code ^= (fakeLicenseCode >> 32);
    while (code != 0) {
        code &= code - 1;
        parity = 1 - parity;
    }
}
#endif

/* DoBarDialog (mac _DoBarDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047c8e0
// MAC: _DoBarDialog i386 0x00048ef5 ppc 0x00051898
void DoBarDialog(void)
{
    short itemHit;
    short savedStellar;
    short savedRoute;
    Boolean done;
    short i;
    short shown;

    done = 0;
    savedStellar = shipData->targetStellar;
    savedRoute = shipData->navTargetKind;
    RandomizeNewsDialog();
    missionsOutside = 1;
    offeringFromShip = 0;
    if (dockedPortMissions[1][0] == -1)
        nextPortMission = -1;
    else
        nextPortMission = 0;
    missionOfferTime = ReadTick() + 0xf;
    lastRaceWinner = -1;
    strncpy(barDescText, "", 3);
    LoadDescription(barDescText, shipData->targetStellar + 10000);
    barPic = NULL;
    if (descResourcePictId >= kFirstResourceID)
        barPic = GetPicture(descResourcePictId);
    barDialog = NULL;
    if (barPic != NULL)
        barDialog = GetNewDialog(kBarDlogID, BarDialogUpdate, BarFilter);
    else
        barDialog = GetNewDialog(kPlainBarDlogID, BarDialogUpdate,
                                 BarFilter);
    if (barDialog == NULL)
        return;
    if (barPic != NULL)
        barBackgroundPic = GetPicture(kBarPictID);
    else
        barBackgroundPic = GetPicture(kPlainBarPictID);
    CenterWindow(FrontWindow(barDialog), FrontWindow(portDialog));
    ShowWindowNoFlash(FrontWindow(barDialog));
    FlushEvents(everyEvent - diskMask, 0);
    SetDialogGWorld(barDialog);
    RedrawDialog(barDialog);
    BarDialogUpdate();
    SelectWindow(FrontWindow(barDialog));
    BringToFront(FrontWindow(barDialog));
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(barDialog, BarFilter, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2) {
            if (shipData->credits > 0) {
                DoRaceDialog();
            } else {
                strcpy(textShowString, "");
                StrcatIndCString(textShowString, kGameStrList,
                                kBrokeStrIndex);
                ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
                RefreshSAT();
#endif
            }
            PortDialogUpdate();
            SetDialogGWorld(barDialog);
            BarDialogUpdate();
        }
        if (itemHit == 3) {
            SetDialogGWorld(barDialog);
            DrawBarButtons(-1);
            DoNewsDialog();
            PortDialogUpdate();
            SetDialogGWorld(barDialog);
            BarDialogUpdate();
        }
        if (itemHit == 5) {
            if (CanHireEscorts()) {
                SetDialogGWorld(barDialog);
                DrawBarButtons(-1);
                shipBuyMode = 1;
                DoShipyardDialog();
                ShipStatsToSystemInfo();
                DrawStatusCargo();
#ifdef MACBUILD
                PortDialogUpdate();
                SetDialogGWorld(barDialog);
                BarDialogUpdate();
#else
                RedrawDialogs(1);
#endif
            }
        }
        if (itemHit == 6) {
            if (OfferMissionFromPort(kBarVenue)) {
                SetDialogGWorld(portDialog);
                PortDialogUpdate();
                SetDialogGWorld(barDialog);
                BarDialogUpdate();
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
    if (barBackgroundPic != NULL) {
        QDReleaseResource(barBackgroundPic);
        barBackgroundPic = NULL;
    }
    if (barPic != NULL) {
        QDReleaseResource(barPic);
        barPic = NULL;
    }
    DisposeDialogNoFlash(barDialog);
    barDialog = NULL;
#ifdef MACBUILD
    RefreshSAT();
#endif
    ResetMissionOfferVenue();
}

/* nothing here reaches into a dialog; the accessors do all of it */


/* the oops record, this file's own copy; the disaster handler models the same
   0x210-byte record for the fields it needs */

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

/* BarFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047cdb0
// MAC: _BarFilter i386 0x00047a4b ppc 0x000503e8
Boolean BarFilter(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
#if V111
    char key;
#else
    EventChar c;
    EventChar key;
#endif

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
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
            *itemHit = 1;
            return 1;
        }
        if (key == kGKeyCode || key == kRKeyCode) {
            ev->what = kMouseDown;
            *itemHit = 2;
            return 1;
        }
        if (key == kWKeyCode || key == kNKeyCode) {
            ev->what = kMouseDown;
            *itemHit = 3;
            return 1;
        }
        if (key == kHKeyCode || key == kEKeyCode) {
            ev->what = kMouseDown;
            *itemHit = 5;
            return 1;
        }
#else
        c = EventCharOf(ev);
        if (IsReturnChar(c) || c == kEscapeChar) {
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
#endif
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        BarDialogUpdate();
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
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(barDialog));
        BarDialogUpdate();
        EndUpdate(GetDialogWindow(barDialog));
#else
        BarDialogUpdate();
#endif
        return 0;
    }
    return 0;
}

/* BarDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047cfe0
// MAC: _BarDialogUpdate i386 0x000477e8 ppc 0x0005020c
void BarDialogUpdate(void)
{
    Rect box;
    short itemType;
    Handle item;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(barDialog));
    if (barBackgroundPic != NULL)
        DrawPicHandle(barBackgroundPic, GetDialogPortRect(barDialog));
    if (barPic != NULL) {
        GetDialogItem(barDialog, 8, &itemType, (void **)&item, &box);
        ForeColor(blackColor);
        DrawPicHandle(barPic, &box);
    }
    GetDialogItem(barDialog, 7, &itemType, (void **)&item, &box);
    ForeColor(blackColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    TETextBox(barDescText, strlen(barDescText), &box, 0);
    InvertRect(&box);
    ForeColor(blackColor);
    DrawBarButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(barDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(barDialog)),
             GetDialogPortRect(barDialog), GetDialogPortRect(barDialog), 0, NULL);
#ifdef MAC1010
    QDFlushPortBuffer(GetWindowGrafPort(GetDialogWindow(barDialog)), NULL);
#else
#if V111
    QDFlushPortBuffer(GetWindowGrafPort(FrontWindow(barDialog)), NULL);
#endif
#endif
}

/* DoNewsDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047d180
// MAC: _DoNewsDialog i386 0x00047f0a ppc 0x000508b8
void DoNewsDialog(void)
{
    Boolean done;
    short pictID;
    short itemHit;

    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    pictID = kDefaultBackdropPict;
    if (dockedPort->govt != -1) {
        if (govtData[dockedPort->govt].newsPict != -1)
            pictID = govtData[dockedPort->govt].newsPict;
    }
    newsPic = GetPicture(pictID);
    if (newsPic == NULL && pictID != kDefaultBackdropPict)
        newsPic = GetPicture(kDefaultBackdropPict);
    newsDialog = NULL;
    newsDialog = GetNewDialog(kDlg1014ID, NewsDialogUpdate, NewsDialogFilter);
    if (newsDialog == NULL)
        return;
    CenterWindow(FrontWindow(newsDialog), FrontWindow(barDialog));
    ShowWindowNoFlash(FrontWindow(newsDialog));
    SetDialogGWorld(newsDialog);
    FlushEvents(everyEvent - diskMask, 0);
    NewsDialogUpdate();
    SelectWindow(FrontWindow(newsDialog));
    BringToFront(FrontWindow(newsDialog));
    done = 0;
    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(newsDialog, NewsDialogFilter, &itemHit, NULL);
        if (itemHit == 1)
            done = 1;
    } while (done == 0);
#ifdef MACBUILD
    HPurge((Handle)newsPic);
#endif
    QDReleaseResource(newsPic);
    newsPic = NULL;
    DisposeDialogNoFlash(newsDialog);
    newsDialog = NULL;
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
#ifdef MACBUILD
    RefreshSAT();
#endif
}

/* NewsDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047d370
// MAC: _NewsDialogUpdate i386 0x00047c57 ppc 0x00050688
void NewsDialogUpdate(void)
{
    Rect box;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(newsDialog));
    DrawPicHandle(newsPic, GetDialogPortRect(newsDialog));
#ifdef CEFIX /* F12.40 */
    SetRect(&box, GetDialogPortRect(newsDialog)->left + CE_Scale(kTextInset),
            GetDialogPortRect(newsDialog)->top + CE_Scale(kTitleTop),
            GetDialogPortRect(newsDialog)->right - CE_Scale(kTextInset),
            GetDialogPortRect(newsDialog)->top + CE_Scale(kTitleBottom));
#else
    SetRect(&box, GetDialogPortRect(newsDialog)->left + kTextInset,
            GetDialogPortRect(newsDialog)->top + kTitleTop,
            GetDialogPortRect(newsDialog)->right - kTextInset,
            GetDialogPortRect(newsDialog)->top + kTitleBottom);
#endif
    TETextBox(sponsorString, strlen(sponsorString), &box, 0);
    InvertRect(&box);
#ifdef CEFIX /* F12.41 */
    SetRect(&box, GetDialogPortRect(newsDialog)->left + CE_Scale(kTextInset),
            GetDialogPortRect(newsDialog)->top + CE_Scale(kBodyTop),
            GetDialogPortRect(newsDialog)->right - CE_Scale(kTextInset),
            GetDialogPortRect(newsDialog)->bottom - CE_Scale(kBodyMargin));
#else
    SetRect(&box, GetDialogPortRect(newsDialog)->left + kTextInset,
            GetDialogPortRect(newsDialog)->top + kBodyTop,
            GetDialogPortRect(newsDialog)->right - kTextInset,
            GetDialogPortRect(newsDialog)->bottom - kBodyMargin);
#endif
    TETextBox(newsString, strlen(newsString), &box, 0);
    InvertRect(&box);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(newsDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(newsDialog)),
             GetDialogPortRect(newsDialog), GetDialogPortRect(newsDialog), 0, NULL);
#if defined(MACBUILD) || V111
    QDFlushPortBuffer(GetDialogPort(newsDialog), NULL);
#endif
}

/* NewsDialogFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047d560
// MAC: _NewsDialogFilter i386 0x00048160 ppc 0x00050acc
Boolean NewsDialogFilter(long unused, EventRecord *ev, short *itemHit)
{
#if V111
    char key;
#endif
    Point where;

    *itemHit = -1;
#if V111
    if ((int)ev->what == kKeyDown) {
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
            *itemHit = 1;
            return 1;
        }
    }
#else
    if ((int)ev->what == kKeyDown &&
        (IsReturnChar(EventCharOf(ev)) ||
         EventCharOf(ev) == kEscapeChar)) {
        *itemHit = 1;
        return 1;
    }
#endif
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
        SetDialogGWorld(newsDialog);
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        if (PtInRectPoint(where, GetDialogPortRect(newsDialog)) != 0) {
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(newsDialog));
        NewsDialogUpdate();
        EndUpdate(GetDialogWindow(newsDialog));
#else
        NewsDialogUpdate();
#endif
        return 0;
    }
    return 0;
}

#define kRaceButtonCount 6

/* RandomizeNewsDialog. STATE: MATCH 100.0%, exe 404 / obj 404, frame 0x80c 6/6 slots and refs,
   byte-identical yes, relocregcheck 0 of 91. OPEN: none. The dead licence parity counts in
   `pick'. see notes */
// FUNCTION: EVNOVA 0x0047d600
// MAC: _RandomizeNewsDialog i386 0x00048238 ppc 0x00050bd0
void RandomizeNewsDialog(void)
{
#ifdef MACBUILD
    Boolean cronActive[kMaxCrons];
    short i;
    short pick;
    short j;
    short cronNewsID[kMaxCrons];
    Str255 line;
    Str255 tmp;
    Boolean anyGovtNews;
    Boolean anyActiveCron;
    short eligible;
    short matching;

    LicenseParity();
#else
    Boolean cronActive[kMaxCrons];
    Boolean anyGovtNews;
    Boolean anyActiveCron;
    short i;
    short cronNewsID[kMaxCrons];
    Str255 line;
    Str255 tmp;
    unsigned long code;
    short eligible;
    short matching;
    short j;
    short pick;
    unsigned long long mem;

    mem = fakeLicenseCode;
    pick = 0;
    code = mem;
    code ^= (mem >> 32);
    while (code != 0) {
        code &= code - 1;
        pick = 1 - pick;
    }
#endif

    GetRandomIndString((unsigned char *)sponsorString, kSponsorStrList);
    if ((unsigned char)sponsorString[0] != 0) {
        PtoCstr((unsigned char *)sponsorString);
    } else {
        strcpy(sponsorString, "");
        StrcatIndCString(sponsorString, kGameStrList, kStrSponsor);
    }

    strcpy(newsString, "");
    i = 0;
    eligible = 0;
    matching = 0;
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].countdown > 1) {
            eligible++;
            if (disasterData[i].stellarID == shipData[0].targetStellar ||
                disasterData[i].stellar == -2)
                matching++;
        }
    }
    if (eligible > 0) {
        if (matching > 0) {
            do {
                pick = Rand(kMaxDisasters);
            } while (disasterData[pick].loaded == 0 || disasterData[pick].countdown <= 1 ||
                     (disasterData[pick].stellarID != shipData[0].targetStellar &&
                      disasterData[pick].stellar != -2));
        } else {
            do {
                pick = Rand(kMaxDisasters);
            } while (disasterData[pick].loaded == 0 || disasterData[pick].countdown <= 1);
        }
        pstrcpy(line, "\p");
        if (disasterData[pick].stellarID != -1 && disasterData[pick].name[0] != 0) {
            strcpy((char *)line, "");
            StrcatIndCString((char *)line, kGameStrList, kStrSponsor);
            strcat((char *)line, " ");
            pstrcpy(tmp, disasterData[pick].name);
            PtoCstr(tmp);
            strcat((char *)line, (char *)tmp);
            strcat((char *)line, " ");
            StrcatIndCString((char *)line, kGameStrList, kStrDisasterVerb);
            strcat((char *)line, " ");
            if (disasterData[pick].amount > 0)
                StrcatIndCString((char *)line, kGameStrList, kStrDisasterOn);
            else
                StrcatIndCString((char *)line, kGameStrList, kStrDisasterIn);
            strcat((char *)line, " ");
            StrcatIndCString((char *)line, kGameStrList, kStrDisasterAffects);
            strcat((char *)line, " ");
            pstrcpy(tmp, lcCargoName[disasterData[pick].commodity]);
            PtoCstr(tmp);
            strcat((char *)line, (char *)tmp);
            strcat((char *)line, " ");
            StrcatIndCString((char *)line, kGameStrList, kStrDisasterAt);
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
        if (cronEvent[i].active != 0 && cronEvent[i].loaded != 0 &&
            cronEvent[i].holdoffLeft <= 0) {
            for (j = 0; j < 4; j++) {
                if (cronEvent[i].govts[j] != -1 &&
                    GovtAllies(cronEvent[i].govts[j],
                               stellarData[shipData[0].targetStellar].govt)
                        != 0) {
                    cronNewsID[i] = cronEvent[i].govtValues[j];
                    if (cronNewsID[i] > 0)
                        anyGovtNews = 1;
                }
            }
            if (cronNewsID[i] < -1 && cronEvent[i].indNewsStr > 0) {
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
            GetRandomIndString((unsigned char *)newsString, cronEvent[pick].indNewsStr);
        }
        PtoCstr((unsigned char *)newsString);
    }
    if (strlen(newsString) == 0) {
        GetRandomIndString((unsigned char *)newsString, kFillerNewsStrList);
        if ((unsigned char)newsString[0] != 0) {
            PtoCstr((unsigned char *)newsString);
        } else {
            extern char *strcpy(char *, const char *);

            strcpy(newsString, "");
            StrcatIndCString(newsString, kGameStrList, kStrNoNews);
        }
    }
}

/* DoRaceDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047dc50
// MAC: _DoRaceDialog i386 0x000489ca ppc 0x00051360
void DoRaceDialog(void)
{
    short itemHit;
    Boolean done;
    short i;
    short j;
    long payout;
    long credits;

    raceDialog = NULL;
    raceDialog = GetNewDialog(kRaceDialogID, RaceDialogUpdate,
                              RaceFilter);
    if (raceDialog == NULL)
        return;
    raceWinner = -1;
    racePick = -1;
    raceCount = 0;
    raceBackgroundPic = GetPicture(kRaceBackdropPict);
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
    RaceDialogUpdate();
    SelectWindow(FrontWindow(raceDialog));
    BringToFront(FrontWindow(raceDialog));
    done = 0;
    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(raceDialog, RaceFilter, &itemHit, NULL);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2) {
            LoadDescription(textShowString, kRaceHelpDesc);
            ShowTextDialog(0, 0, 0);
            SetDialogGWorld(raceDialog);
            RaceDialogUpdate();
        }
        if (itemHit >= kRaceFirstButton &&
            itemHit < kRaceFirstButton + kRacerCount) {
#ifdef MACBUILD
            if (KeyCheck(kScanOption) != 0)
#else
            if (KeyCheck(kKeyLeftAlt) != 0 || KeyCheck(kKeyRightAlt) != 0)
#endif
            {
                if (shipData[0].credits > kMaxRaceBet)
                    credits = kMaxRaceBet;
                else
                    credits = shipData[0].credits;
                raceBetAmount = DoQuantityDialog(credits, 2);
            }
            else if (shipData[0].credits >= kQuickRaceBet)
                raceBetAmount = kQuickRaceBet;
            else
                raceBetAmount = shipData[0].credits;
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
                LoadDescription(textShowString,
                                (raceWinner + kRaceResultDesc));
                if (raceWinner == racePick) {
                    payout = raceBetAmount * kRacePayoutOdds;
                    strcat(textShowString, "\r\r");
                    StrcatIndCString(textShowString, kGameStrList,
                                    kRaceWinStrIndex);
                    strcat(textShowString, ":  ");
                    StrcatCommaNumber(payout, textShowString);
                    strcat(textShowString, " ");
                    if (payout == 1)
                        strcat(textShowString, creditCString);
                    else
                        strcat(textShowString, creditsCString);
                    strcat(textShowString, ".");
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
            RaceDialogUpdate();
        }
    } while (done == 0);
    if (raceBackgroundPic != NULL) {
        QDReleaseResource(raceBackgroundPic);
        raceBackgroundPic = NULL;
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

/* RaceFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e150
// MAC: _RaceFilter i386 0x0004890e ppc 0x00051284
Boolean RaceFilter(long unused, EventRecord *ev, short *itemHit)
{
#if V111
    char key;
#endif
    Point where;
    short button;

#if V111
    if ((int)ev->what == kKeyDown) {
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
            *itemHit = 1;
            return 1;
        }
        return 0;
    }
#else
    if ((int)ev->what == kKeyDown &&
        (IsReturnChar(EventCharOf(ev)) ||
         EventCharOf(ev) == kEscapeChar)) {
        *itemHit = 1;
        return 1;
    }
#endif
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        RaceDialogUpdate();
        button = TrackRaceButtons(where);
        if (button >= 0 && button < kRaceButtonCount) {
            *itemHit = button + 1;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(raceDialog));
        RaceDialogUpdate();
        EndUpdate(GetDialogWindow(raceDialog));
#else
        RaceDialogUpdate();
#endif
        return 0;
    }
    return 0;
}

/* RaceDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e1e0
// MAC: _RaceDialogUpdate i386 0x0004879d ppc 0x0005115c
void RaceDialogUpdate(void)
{
    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(raceDialog));
    if (raceBackgroundPic != NULL)
        DrawPicHandle(raceBackgroundPic, GetDialogPortRect(raceDialog));
    ForeColor(blackColor);
    DrawRaceButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(raceDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(raceDialog)),
             GetDialogPortRect(raceDialog), GetDialogPortRect(raceDialog), 0, NULL);
#ifdef MAC1010
    QDFlushPortBuffer(GetWindowGrafPort(GetDialogWindow(raceDialog)), NULL);
#else
#if V111
    QDFlushPortBuffer(GetWindowGrafPort(FrontWindow(raceDialog)), NULL);
#endif
#endif
}

EXTERN_C_END
