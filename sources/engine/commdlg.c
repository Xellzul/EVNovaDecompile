// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* commdlg.c -- the hail, haggle and plunder dialog object (tu-map O22, win 0x0047e470..0x004860f0). Notes: Research/notes/commdlg.md */
#include <stddef.h>
#include <string.h>

#include "burger.h"
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "dialogs.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "game.h"
#include "endianswap.h"

/* the mac engine invalidates a dialog item through the caller's own GetDialogItem locals */
#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &box), InvalWindowRect(GetDialogWindow(dlg), &box))
#endif
#define kCommButtons  3

DialogPtr commDialog;
DialogPtr haggleDialog;
DialogPtr plunderDialog;
#ifndef MACBUILD
unsigned char DAT_007d17c4[2];
#endif
short haggleMode;
short responseRandom;
short adviceRandom;
short adviceDisasterRandom;
#ifndef MACBUILD
unsigned char DAT_007d17ce[2];
#endif
short salvageType;
short salvageQty;
SalvageAmmo salvageAmmoType;
short salvageAmmoQty;
short salvageFuelQty;
short captureOdds;
short bribeRandom;
int32_t salvageCredits;
int32_t initialHaggleVal;
int32_t haggleVal;
float bribeFlux;
Boolean hatesPlayer;
#ifndef MACBUILD
Boolean DAT_007d17f1;
#endif
Boolean willTakeBribe;
Boolean hasDemandedTribute;
Boolean commShipXeno;
Boolean releaseCommEscort;
Boolean canUpgradeEscort;
Boolean canSellEscort;
PicHandle commShipPic;
PicHandle commBackgroundPic;
PicHandle haggleBackgroundPic;
PicHandle plunderBackgroundPic;
ShipObj *commShip;
unsigned char commResponseString[256];
unsigned char adviceString[256];
unsigned char stellarTypeStr[256];
unsigned char shipClassStr[256];
#ifndef MACBUILD
unsigned char DAT_007d1c0c[256];
#endif
unsigned char shipGovtStr[256];
short commDialogMode;
#ifndef MACBUILD
unsigned char DAT_007d1e0e[2];
unsigned char gRT3Table3[256];
#endif

#define kCommDlgID2      0x3ef
#define kCommPictID2     0x213f

/* the fields this dialog reads of the 0x8e6-byte mission record */

/* the fields this dialog reads of the 0x120-byte rank record */

/* DoCommDialog (mac _DoCommDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e470
// MAC: _DoCommDialog i386 0x000956d5 ppc 0x000a1380
void DoCommDialog(short shipIndex)
{
    short i;
    short itemHit;
    Boolean hasMission;
    Boolean isMissionShip;
    Boolean cantTalk;
    Boolean done;
    Boolean canAskEscort;
    Boolean canAskHelp;
    Boolean noBribe;
    Boolean missionPending;
    unsigned char buf[256];
    short n;
    short r;
    int32_t amount;
    short halfW;
    short halfH;
#ifdef MACBUILD
    short itemType;
    Handle item;
    Rect box;
#endif
    done = 0;
    if (screenFaded)
        return;
    if (shipIndex < 0 || shipIndex >= kMaxShips)
        return;
    if (!shipData[shipIndex].active)
        return;
    commShip = &shipData[shipIndex];
    if (commShip->aiType == kShipTypeEscort && commShip->parentShip == 0
        && commShip->specialMission == -1) {
        DoEscortDialog(commShip);
        return;
    }
    willTakeBribe = 0;
    hasDemandedTribute = 0;
    releaseCommEscort = 0;
    isMissionShip = 0;
    cantTalk = 0;
    noBribe = 0;
    if (commShip->govt == -1)
        willTakeBribe = 1;
    else {
        if (commShip->aiType <= kShipTypeBraveTrader
            && (govtData[commShip->govt].flags & kGovtFreightersBribable) != 0)
            willTakeBribe = 1;
        if (commShip->aiType >= kShipTypeWarship
            && (govtData[commShip->govt].flags & kGovtWarshipsBribable) != 0)
            willTakeBribe = 1;
        if ((govtData[commShip->govt].flags2 & kGovtNotTalkative) != 0)
            isMissionShip = 1;
        if ((govtData[commShip->govt].flags2 & kGovtSilent) != 0)
            cantTalk = 1;
        if (shipClass[commShip->type].inherentGovtB != -1
            && (govtData[shipClass[commShip->type].inherentGovtB].flags2 & kGovtSilent) != 0)
            cantTalk = 1;
        if (commShip->aiType >= kShipTypeWarship
            && (govtData[commShip->govt].flags & kGovtPlunders) != 0)
            noBribe = 1;
    }
    responseRandom = Rand(5);
    bribeFlux = (Rand(41) + 80) * 0.01;
    if (Rand(5) == 0)
        bribeFlux -= 0.5;
    else if (Rand(5) == 0)
        bribeFlux += 0.5;
    r = (shipData->credits * 0.0000005);
    if (r <= 0)
        r = 1;
    initialHaggleVal = ((Rand(r) * 1000 + 3000) * bribeFlux);
    if (commShip->govt != -1
        && (govtData[commShip->govt].flags & kGovtGreedy) != 0) {
        r = (shipData->credits * 0.0001);
        if (r <= 0)
            r = 1;
        initialHaggleVal = ((Rand(r) * 1000 + 10000) * bribeFlux);
    }
    if (initialHaggleVal > shipData->credits * 0.333)
        initialHaggleVal = (shipData->credits * 0.333);
    initialHaggleVal /= 1000;
    initialHaggleVal *= 1000;
    if (initialHaggleVal > 20000)
        initialHaggleVal = 20000;
    if (initialHaggleVal < 1000)
        initialHaggleVal = 1000;
    commShipXeno = 0;
    if (commShip->govt != -1
        && (govtData[commShip->govt].flags & kGovtXenophobic) != 0)
        commShipXeno = 1;
    n = shipClass[commShip->type].shipyardPictID;
    if (commShip->persType != -1
        && personData[commShip->persType].hailPict >= kFirstResourceID)
        n = personData[commShip->persType].hailPict;
    commShipPic = GetPicture(n);
    if (commShip->govt != -1)
        pstrcpy(shipGovtStr, govtShortNames[commShip->govt]);
    else
        pstrcpy(shipGovtStr, "\p");
    pstrcpy(shipClassStr, shipCommNames[commShip->type]);
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    ShowTheCursor();
    hasMission = 0;
    if (commShip->specialMission != -1
        && currentMissionObjectives[commShip->specialMission].active != 0
        && currentMissions[commShip->specialMission].shipBehav == kMissionModeEscort)
        hasMission = 1;
    LoadAdvice();
    LoadResponse(0);
    if (commShip->specialMission != -1 && !IsDisabled(commShip) && !isMissionShip) {
        if (currentMissionObjectives[commShip->specialMission].active != 0) {
            n = currentMissions[commShip->specialMission].shipGoal;
            if (n == kMissionGoalDestroy)
                LoadResponse(2);
            else if (n == kMissionGoalEscort) {
                LoadResponse(8);
                PtoCstr(commResponseString);
                pstrcpy(buf, shipName);
                PtoCstr(buf);
                strcat((char *)commResponseString, (char *)buf);
                strcat((char *)commResponseString, ".");
                CtoPstr((char *)commResponseString);
            }
            if (currentMissions[commShip->specialMission].shipBehav == kMissionModeAttackPlayer)
                LoadResponse(2);
            if (currentMissions[commShip->specialMission].shipBehav == kMissionModeEscort)
                LoadResponse(4);
        }
    } else if (!IsDisabled(commShip) && !isMissionShip) {
        if (ExtendedIsThreatToPlayer(commShip))
            LoadResponse(2);
        else if (commShip->parentShip == 0) {
            if (commShip->aiType == kShipTypeFighter)
                LoadResponse(5);
            if (commShip->aiType == kShipTypeEscort)
                LoadResponse(4);
        } else if (AIDoesShipLikePlayer(commShip))
            LoadResponse(0);
        else
            LoadResponse(2);
    }
    commDialog = NULL;
    commDialog = GetNewDialog(kCommDlgID2, CommDialogUpdate, CommFilter);
    if (commDialog == NULL)
        return;
    canAskEscort = 0;
    canAskHelp = 0;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0
            && GovtAllies(commShip->govt, rankData[i].govt)) {
            if ((rankData[i].flags & kRankCallsAssistance) != 0)
                canAskEscort = 1;
            if ((rankData[i].flags & kRankFreeRepair) != 0)
                canAskHelp = 1;
        }
    }
    if (commShip->govt != -1
        && (govtData[commShip->govt].flags2 & kGovtRoadsideAssistance) != 0)
        canAskHelp = 1;
    commDialogMode = 0;
    commBackgroundPic = GetPicture(kCommPictID2);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);
    CenterWindow(FrontWindow(commDialog), gWind);
    ShowWindowNoFlash(FrontWindow(commDialog));
    SetDialogGWorld(commDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(commDialog);
    CommDialogUpdate();
    SelectWindow(FrontWindow(commDialog));
    BringToFront(FrontWindow(commDialog));

    do {
        if (gQuit)
            done = 1;
        MyModalDialog(commDialog, CommFilter, &itemHit, 0);
        if (itemHit == 1) {
            DrawCommDialogButtons(-1);
            done = 1;
        }
        if (itemHit == 2) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            if (!IsDisabled(commShip)) {
                if (commShip->parentShip == 0 && commShip->aiType == kShipTypeEscort
                           && !hasMission) {
                    releaseCommEscort = 1;
                    LoadResponse(0x26);
                    InvalDialogItem(commDialog, 12);
                    DrawCommDialogButtons(-1);
                } else if (!cantTalk) {
                    DrawCommDialogButtons(-1);
                    if (ExtendedIsThreatToPlayer(commShip)) {
                        missionPending = 0;
                        if (commShip->specialMission != -1
                            && currentMissionObjectives[commShip->specialMission].active != 0
                            && currentMissions[commShip->specialMission].shipBehav == kMissionModeAttackPlayer)
                            missionPending = 1;
                        if (!missionPending) {
                            if (commShip->defenseStellar != -1)
                                LoadResponse(0x13);
                            else if (commShip->persType == kPersBountyHunter)
                                LoadResponse(0x13);
                            else if (willTakeBribe) {
                                if (bribeFlux < 0.8)
                                    LoadResponse(0x17);
                                else if (bribeFlux > 1.2)
                                    LoadResponse(0x18);
                                else
                                    LoadResponse(0x12);
                                CommDialogUpdate();
                                r = DoShipCommPayment();
                                SetDialogGWorld(commDialog);
                                if (r == 1) {
                                    LoadResponse(0x14);
                                    AIMakeShipLeave(commShip);
                                    commShip->aiType = kShipTypeWimpyTrader;
                                } else if (r == 0) {
                                    LoadResponse(0x1e);
                                    AIMakeShipAttackPlayer(commShip);
                                } else if (r == -1)
                                    LoadResponse(0xc);
                                RedrawDialog(commDialog);
                            } else
                                LoadResponse(0x13);
                        } else
                            LoadResponse(0x13);
                    } else if (AIDoesShipLikePlayer(commShip) && !commShipXeno
                               && !noBribe) {
                        if (IsShipThreatened(commShip) || AIIsShipBusy(commShip)) {
                            if (IsRefuellingPlayer(commShip) || IsRepairingPlayer(commShip))
                                LoadResponse(0x1d);
                            else if (commShip->aiType <= kShipTypeInterceptor)
                                LoadResponse(0x10);
                            else
                                LoadResponse(0x16);
                        } else if (IsPlayerThreatened()) {
                            if (IsPlayerThreatenedByMyEnemy(commShip)) {
                                if (canAskEscort) {
                                    LoadResponse(0x1d);
                                    AIFightEnemyThreatToPlayer(commShip);
                                    AICallForReinforcements(commShip, 1);
                                } else if (commShipXeno)
                                    LoadResponse(0x13);
                                else if (commShip->aiType == kShipTypeWimpyTrader)
                                    LoadResponse(0x11);
                                else if (commShip->aiType == kShipTypeBraveTrader) {
                                    if (commShip->index % 3 == 0
                                        && commShip->govt == -1) {
                                        if (bribeFlux < 0.8)
                                            LoadResponse(0x17);
                                        else if (bribeFlux > 1.2)
                                            LoadResponse(0x18);
                                        else
                                            LoadResponse(0x12);
                                        CommDialogUpdate();
                                        r = DoShipCommPayment();
                                        SetDialogGWorld(commDialog);
                                        if (r == 1) {
                                            LoadResponse(0x1d);
                                            AIFightEnemyThreatToPlayer(commShip);
                                        } else if (r == 0)
                                            LoadResponse(0x1f);
                                        else if (r == -1)
                                            LoadResponse(0xc);
                                    } else
                                        LoadResponse(0x11);
                                } else if (commShip->aiType == kShipTypeWarship
                                           || commShip->aiType == kShipTypeInterceptor) {
                                    if (bribeFlux < 0.8)
                                        LoadResponse(0x17);
                                    else if (bribeFlux > 1.2)
                                        LoadResponse(0x18);
                                    else
                                        LoadResponse(0x12);
                                    CommDialogUpdate();
                                    r = DoShipCommPayment();
                                    SetDialogGWorld(commDialog);
                                    if (r == 1) {
                                        LoadResponse(0x1d);
                                        AIFightEnemyThreatToPlayer(commShip);
                                    } else if (r == 0)
                                        LoadResponse(0x1f);
                                    else if (r == -1)
                                        LoadResponse(0xc);
                                } else {
                                    LoadResponse(0x1d);
                                    AIFightThreatToPlayer(commShip);
                                }
                            } else
                                LoadResponse(0x13);
                        } else {
                            if ((shipData->fuel < 100 && ShipFuelCapacity(shipData) > 0)
                                || IsDisabled(shipData)) {
                                if (commShipXeno)
                                    LoadResponse(0x13);
                                else if (commShip->aiType <= kShipTypeInterceptor) {
                                    if (bribeFlux < 0.8)
                                        LoadResponse(0x17);
                                    else if (bribeFlux > 1.2)
                                        LoadResponse(0x18);
                                    else
                                        LoadResponse(0x1c);
                                    CommDialogUpdate();
                                    r = 0;
                                    if (canAskHelp)
                                        r = 1;
                                    if (r != 1) {
                                        r = DoShipCommPayment();
                                        SetDialogGWorld(commDialog);
                                    }
                                    if (r == 1) {
                                        LoadResponse(0x1d);
                                        if (IsDisabled(shipData))
                                            AIMakeShipRepairPlayer(commShip);
                                        else
                                            AIMakeShipRefuelPlayer(commShip);
                                    } else if (r == 0)
                                        LoadResponse(0x1f);
                                    else if (r == -1)
                                        LoadResponse(0xc);
                                } else
                                    LoadResponse(0x19);
                            } else
                                LoadResponse(0xe);
                        }
                    } else
                        LoadResponse(0x13);
                }
            } else {
                if (!isMissionShip)
                    LoadResponse(1);
            }
        }
        if (itemHit == 3) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            DrawCommDialogButtons(-1);
            if (IsDisabled(commShip) || isMissionShip)
                LoadResponse(1);
            else if (ExtendedIsThreatToPlayer(commShip))
                LoadResponse(0xd);
            else if (AIDoesShipLikePlayer(commShip))
                strncpy((char *)commResponseString, (char *)adviceString, 254);
            else
                LoadResponse(0xd);
        }
        if (itemHit != -1) {
            SetDialogGWorld(commDialog);
            InvalDialogItem(commDialog, 10);
            InvalDialogItem(commDialog, 12);
        }
    } while (!done);

    if (commBackgroundPic != NULL) {
        QDReleaseResource(commBackgroundPic);
        commBackgroundPic = NULL;
    }
    if (commShipPic != NULL) {
#ifdef MACBUILD
        HPurge((Handle)commShipPic);
#endif
        QDReleaseResource(commShipPic);
        commShipPic = NULL;
    }
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
    if (releaseCommEscort) {
        DestroyPartialFleetCargo(commShip->index);
        commShip->parentShip = -1;
        commShip->aiType = shipClass[commShip->type].inherentAI;
        commShip->boarded = 1;
        commShip->formerEscort = -1;
        AISetup(commShip);
        AIMakeShipLeave(commShip);
        statusRedraw[kRedrawCargo] = 1;
    }
    halfW = (GetDialogPortRect(commDialog)->right - GetDialogPortRect(commDialog)->left) / 2;
    halfH = (GetDialogPortRect(commDialog)->bottom - GetDialogPortRect(commDialog)->top) / 2;
    DisposeDialogNoFlash(commDialog);
    commDialog = NULL;
    gSpeedMult = 0;
    commShip->hailed = 1;
    commShip = NULL;
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    RefreshSAT();
#endif
}

/*
 * A no raises the asking price by a thousand for next time, and -1 means the
 * player could not have paid it anyway.
 */
/* DoShipCommPayment. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047f9c0
// MAC: _DoShipCommPayment i386 0x000920f3 ppc 0x0009dd38
short DoShipCommPayment(void)
{
    short answer;

    CommDialogUpdate();
    haggleVal = initialHaggleVal;
    answer = DoHaggleDialog(0, 0x23);
    SetDialogGWorld(commDialog);
    if (shipData->credits >= haggleVal) {
        if (answer == 1) {
            shipData->credits -= haggleVal;
            statusRedraw[kRedrawCargo] = 1;
            return 1;
        }
        initialHaggleVal += 1000;
        return 0;
    }
    return -1;
}

/* CommFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047fa40
// MAC: _CommFilter i386 0x000908ed ppc 0x0009c440
Boolean CommFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int what;
    EventKey key;
    short govt;
    Boolean noRefuel;

    what = ev->what;
    if (what == kKeyDown) {
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEKeyCode
            || key == kEscapeKeyCode) {
#else
        key = EventCharOf(ev);
        key = tolower(key);
        if (IsReturnChar(key) || key == 'e' || key == kEscapeChar) {
#endif
            *itemHit = 1;
            return 1;
        }
        noRefuel = 0;
        if (commShip != NULL) {
            govt = commShip->govt;
            if (govt != -1) {
                if ((govtData[govt].flags2 & kGovtNotTalkative) != 0)
                    noRefuel = 1;
            }
        }
#if V111
        if (!noRefuel) {
            if (key == kRKeyCode) {
                *itemHit = 2;
                return 1;
            }
        }
        if (key == kGKeyCode) {
            *itemHit = 3;
            return 1;
        }
#else
        if (!noRefuel) {
            if (key == 'r') {
                *itemHit = 2;
                return 1;
            }
        }
        if (key == 'g') {
            *itemHit = 3;
            return 1;
        }
#endif
    }
    if (what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        CommDialogUpdate();
        button = TrackCommDialogButtons(where);
        if (button >= 0 && button <= kCommButtons) {
            *itemHit = button + 1;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(commDialog));
        CommDialogUpdate();
        EndUpdate(GetDialogWindow(commDialog));
#else
        CommDialogUpdate();
#endif
    }
    return 0;
}

/* CommDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047fb70
// MAC: _CommDialogUpdate i386 0x000903f5 ppc 0x0009c014
void CommDialogUpdate(void)
{
    Handle item;
    unsigned char text[255]; /* BUG: a 255-character commResponseString overruns text by one */
    short itemType;
    Rect box;
    Rect textBox;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(commDialog));
    if (commBackgroundPic != NULL)
        DrawPicHandle(commBackgroundPic, GetDialogPortRect(commDialog));
    DrawCommDialogButtons(-1);
    GetDialogItem(commDialog, 11, &itemType, &item, &box);
    if (commShipPic != NULL) {
        ForeColor(blackColor);
        DrawPicHandle(commShipPic, &box);
    }
    GetDialogItem(commDialog, 12, &itemType, &item, &box);
    ForeColor(blackColor);
    PaintRect(&box);
    RGBForeColor(&rgbMedGrey);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    MoveTo(box.left, (box.top + 12));
    DrawIndString(kGameStrList, 0xc3);
    MoveTo((box.left + 0x23), (box.top + 12));
    ForeColor(whiteColor);
    if (commShip->persType == kPersCaptainHector) {
        DrawString("\pAmbrosia Mascot");
    } else {
        DrawString(shipClassStr);
        if (shipGovtStr[0] != 0) {
            MoveTo((box.left + 0x23), (box.top + 0x19));
            DrawString("\p(");
            DrawString(shipGovtStr);
            DrawString("\p)");
        }
    }
    MoveTo(box.left, (box.top + 0x28));
    RGBForeColor(&rgbMedGrey);
    if (ExtendedIsThreatToPlayer(commShip)) {
        DrawIndString(kGameStrList, 0xc4);
        DrawString("\p  ");
        ForeColor(redColor);
        TextFace(1);
        DrawIndString(kGameStrList, 0xae);
        TextFace(0);
    } else if (commShip->parentShip == 0 && releaseCommEscort == 0) {
        DrawIndString(kGameStrList, 0xc4);
        MoveTo((box.left + 0x23), (box.top + 0x28));
        RGBForeColor(&brightGreen);
        if (commShip->hired != 0)
            DrawIndString(kGameStrList, 0xa6);
        else
            DrawIndString(kGameStrList, 0xa8);
    }
    ForeColor(blackColor);
    GetDialogItem(commDialog, 10, &itemType, &item, &box);
    pstrcpy(text, commResponseString);
    PtoCstr(text);
    InsetRect(&box, 1, 1);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(whiteColor);
    PaintRect(&box);
    textBox = box;
    InsetRect(&textBox, 4, 2);
    ForeColor(blackColor);
    strcpy(briefingTextPtr, (char *)text);
    MungeBriefing(0, -1);
    TETextBox(briefingTextPtr, strlen(briefingTextPtr), &textBox, 0);
    InvertRect(&box);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(commDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(commDialog)),
             GetDialogPortRect(commDialog), GetDialogPortRect(commDialog), 0, NULL);
}

/*
 * Which key reaches button 2 depends on whether the target hates the player;
 * button 3 is demand-tribute while the stellar is undominated and release once
 * it is, the release being refused outright for stellars carrying the second
 * flags2 bit.
 */

#define kStellarCommButtons 2

/* SpriteWorld's FrameRec, head only: the cel's rect sits at +0x22 */

/* SpriteWorld's SpriteRec, only the three fields the stellar scan reads */


#define kCommDlogID      0x3f1
#define kCommPictID      0x2140
#define kGovtNameList    0x44c
#define kTributeList     0xbba

/* DoPlanetCommDialog (mac _DoPlanetCommDialog). STATE: MATCH, exe 1020 / obj 1020,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00480030
// MAC: _DoPlanetCommDialog i386 0x00096949 ppc 0x000a25ac
void DoPlanetCommDialog(short stellar)
{
    short itemHit;
    unsigned char text[256];
    unsigned char pname[256];
    short i;
    short n;
    Boolean done;
    short halfW;
    short halfH;
#ifdef MACBUILD
    short itemType;
    Handle item;
    Rect box;
#endif

    done = 0;
    if (screenFaded)
        return;
    if (shipData->targetStellar == -1)
        return;
    if ((stellarData[shipData->targetStellar].flags & kSpobUninhabited) != 0) {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        return;
    }
    responseRandom = Rand(5);
    adviceRandom = adviceDisasterRandom = -1;
    commShip = NULL;
    n = (shipData->credits * 0.000001);
    if (n <= 0)
        n = 1;
    initialHaggleVal = Rand(n) * 1000 + 3000;
    if (stellarData[shipData->targetStellar].govt != -1
        && (govtData[stellarData[shipData->targetStellar].govt].flags & kGovtGreedy) != 0)
        initialHaggleVal *= 1.5;
    if (initialHaggleVal > shipData->credits * 0.333)
        initialHaggleVal = (shipData->credits * 0.333);
    initialHaggleVal /= 1000;
    initialHaggleVal *= 1000;
    if (initialHaggleVal > 900000)
        initialHaggleVal = 900000;
    if (initialHaggleVal < 1000)
        initialHaggleVal = 1000;
    hasDemandedTribute = 0;
    if (!LoadPluginString(stellarData[shipData->targetStellar].type + 7000, stellarTypeStr))
        GetIndString(stellarTypeStr, kGovtNameList, stellarData[shipData->targetStellar].type + 1);
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    ShowTheCursor();
    commDialog = NULL;
    commDialog = GetNewDialog(kCommDlogID, PlanetCommDialogUpdate,
                              PlanetCommFilter);
    if (commDialog == NULL)
        return;
    if (stellarData[shipData->targetStellar].minStatus == 0x7fff)
        hatesPlayer = 1;
    else if (playerRecord[shipData->curSystem] >= stellarData[shipData->targetStellar].minStatus
             || stellarData[shipData->targetStellar].minStatus == (short)0x8001)
        hatesPlayer = 0;
    else
        hatesPlayer = 1;
    if (hatesPlayer && stellarData[shipData->targetStellar].govt != -1) {
        if (PlayerBlessedByGovt(stellarData[shipData->targetStellar].govt, 1))
            hatesPlayer = 0;
        else if (govtData[stellarData[shipData->targetStellar].govt].iffScrambled != 0)
            hatesPlayer = 0;
    }
    willTakeBribe = 0;
    if (bribeRandom < 0)
        bribeRandom = Rand(100);
    if (bribeRandom > 30) {
        if (stellarData[shipData->targetStellar].govt == -1)
            willTakeBribe = 1;
        else if ((govtData[stellarData[shipData->targetStellar].govt].flags & kGovtPlanetsBribable) != 0)
            willTakeBribe = 1;
    }
    if (stellarData[shipData->targetStellar].govt != -1
        && (govtData[stellarData[shipData->targetStellar].govt].flags & kGovtGreedy) != 0)
        willTakeBribe = 1;
    if ((stellarData[shipData->targetStellar].flags & kSpobUninhabited) != 0)
        LoadResponse(1);
    else if (!hatesPlayer || stellarData[shipData->targetStellar].dominated != 0) {
        LoadStellarResponse(0);
        strncpy((char *)text, (char *)commResponseString, 254);
        PtoCstr(text);
        pstrcpy(pname, stellarData[shipData->targetStellar].name);
        PtoCstr(pname);
        strcat((char *)text, (char *)pname);
        strcat((char *)text, ".");
        CtoPstr((char *)text);
        strncpy((char *)commResponseString, (char *)text, 254);
    } else
        LoadResponse(2);
    commDialogMode = 1;
    commBackgroundPic = GetPicture(kCommPictID);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);
    CenterWindow(FrontWindow(commDialog), gWind);
    ShowWindowNoFlash(FrontWindow(commDialog));
    SetDialogGWorld(commDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(commDialog);
    PlanetCommDialogUpdate();
    SelectWindow(FrontWindow(commDialog));
    BringToFront(FrontWindow(commDialog));

    do {
        if (gQuit)
            done = 1;
        MyModalDialog(commDialog, PlanetCommFilter, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 2 || itemHit == 3) {
            SetDialogGWorld(commDialog);
            DrawPlanetCommDialogButtons(-1);
            LoadResponse(1);
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            if (itemHit == 2) {
                if (hatesPlayer
                    && stellarData[shipData->targetStellar].dominated == 0) {
                    if (willTakeBribe != 0) {
                        LoadStellarResponse(8);
                        PlanetCommDialogUpdate();
                        haggleVal = initialHaggleVal;
                        i = DoHaggleDialog(0, 0x23);
                        PlaySound(beepSnd[1], 1, gSoundVolume,
                                  gSoundVolume);
                        if (shipData->credits >= haggleVal) {
                        if (i == 1) {
                            dockPermission = 0x2ef;
                            dockTarget = shipData->targetStellar;
                            shipData->aiTimer = 0;
                            pstrcpy(text, shipName);
                            PtoCstr(text);
                            strcat((char *)text, ", ");
                            if ((stellarData[shipData->targetStellar].flags
                                 & kSpobIsStation) != 0)
                                StrcatIndCString((char *)text, kGameStrList, 0x5f);
                            else
                                StrcatIndCString((char *)text, kGameStrList, 0x62);
                            strcat((char *)text, " ");
                            StrcatIndCString((char *)text, kGameStrList, 0x64);
                            CtoPstr((char *)text);
                            DisplayComm((PStrChar *)text, 0xfa, 0, kCommTextSize,
                                        &rgbWhite, 0, 0);
                            shipData->credits -= haggleVal;
                            statusRedraw[kRedrawCargo] = 1;
                            willTakeBribe = 0;
                            done = 1;
                        } else {
                            willTakeBribe = 0;
                            bribeRandom = 0;
                            initialHaggleVal += 1000;
                            LoadStellarResponse(6);
                            PlanetCommDialogUpdate();
                        }
                        } else {
                            LoadStellarResponse(4);
                            PlanetCommDialogUpdate();
                        }
                        RedrawDialog(commDialog);
                    } else {
                        LoadStellarResponse(9);
                        PlanetCommDialogUpdate();
                    }
                } else {
                    if (!hatesPlayer)
                        LoadResponse(9);
                    else
                        LoadStellarResponse(4);
                    RedrawDialog(commDialog);
                }
            }
            if (itemHit == 3) {
                            if ((stellarData[shipData->targetStellar].flags2 & kSpobAlwaysDominated) == 0) {
                    if (stellarData[shipData->targetStellar].dominated != 0) {
                        if ((stellarData[shipData->targetStellar].flags & kSpobIsStation) != 0)
                            GetIndString(commResponseString, kTributeList, 0x25);
                        else
                            GetIndString(commResponseString, kTributeList, 0x24);
                        dockPermission = 0;
                        dockTarget = -1;
                        stellarData[shipData->targetStellar].dominated = 0;
                        if (stellarData[shipData->targetStellar].defCount > 1000) {
                            if (stellarData[shipData->targetStellar].defCount > 10000)
                                stellarData[shipData->targetStellar].defWaveSize =
                                    stellarData[shipData->targetStellar].defCount / 10 - 1000;
                            else
                                stellarData[shipData->targetStellar].defWaveSize =
                                    stellarData[shipData->targetStellar].defCount / 10 - 100;
                        } else
                            stellarData[shipData->targetStellar].defWaveSize = stellarData[shipData->targetStellar].defCount;
                        if (playerRecord[shipData->curSystem]
                                >= stellarData[shipData->targetStellar].minStatus
                            && stellarData[shipData->targetStellar].minStatus > (short)0x8001
                            && stellarData[shipData->targetStellar].minStatus != 0x7fff)
                            playerRecord[shipData->curSystem] =
                                stellarData[shipData->targetStellar].minStatus - 1;
                        crashlog_mission_bit_set(
                            "releasing stellar",
                            (char *)stellarData[shipData->targetStellar].name, 1, shipData->targetStellar + kFirstResourceID,
                            stellarData[shipData->targetStellar].onRelease);
                        EvalMissionBitSetString(stellarData[shipData->targetStellar].onRelease);
                        RedrawDialog(commDialog);
                    } else {
                        hatesPlayer = 1;
                        if (playerRecord[shipData->curSystem]
                                >= stellarData[shipData->targetStellar].minStatus
                            && stellarData[shipData->targetStellar].minStatus > (short)0x8001
                            && stellarData[shipData->targetStellar].minStatus != 0x7fff)
                            playerRecord[shipData->curSystem] =
                                stellarData[shipData->targetStellar].minStatus - 1;
                        SlapWithPenalty(shipData->curSystem,
                                     stellarData[shipData->targetStellar].govt, 3, -1);
                        if (playerKills < 0x3200 && !cheatsActive) {
                            SetDialogGWorld(commDialog);
                            LoadStellarResponse(1);
                            RedrawDialog(commDialog);
                        } else if (hasDemandedTribute
                                   || stellarData[shipData->targetStellar].defWaveSize > 0
                                   || stellarData[shipData->targetStellar].defenseActive != 0) {
                            SetDialogGWorld(commDialog);
                            LoadStellarResponse(2);
                            RedrawDialog(commDialog);
                            if (stellarData[shipData->targetStellar].defenseActive == 0) {
                                for (i = 0; i <= 4; i++)
                                    SlapWithPenalty(shipData->curSystem,
                                                 stellarData[shipData->targetStellar].govt, 3, -1);
                                if (stellarData[shipData->targetStellar].defCount <= 1000) {
                                    n = stellarData[shipData->targetStellar].defWaveSize;
                                    stellarData[shipData->targetStellar].defWaveSize = 0;
                                } else {
                                    stellarData[shipData->targetStellar].defWaveSize -=
                                        stellarData[shipData->targetStellar].defCount % 10;
                                    n = stellarData[shipData->targetStellar].defCount % 10;
                                    if (stellarData[shipData->targetStellar].defWaveSize < 0) {
                                        n += stellarData[shipData->targetStellar].defWaveSize;
                                        stellarData[shipData->targetStellar].defWaveSize = 0;
                                    }
                                }
                                for (i = 0; i < n; i++)
                                    SpawnStellarDefenseShip(shipData->targetStellar);
                            }
                        } else {
                            for (i = 0; i <= 4; i++)
                                SlapWithPenalty(shipData->curSystem,
                                             stellarData[shipData->targetStellar].govt, 3, -1);
                            if ((stellarData[shipData->targetStellar].flags & kSpobIsStation) != 0)
                                GetIndString(commResponseString, kTributeList, 0x1b);
                            else
                                GetIndString(commResponseString, kTributeList, 0x1a);
                            dockPermission = 0;
                            dockTarget = -1;
                            ((char *)personData)[0x1e46f8] = 1;
                            stellarData[shipData->targetStellar].dominated = 1;
                            stellarData[shipData->targetStellar].daysDominated = 0;
                            crashlog_mission_bit_set(
                                "dominating stellar",
                                (char *)stellarData[shipData->targetStellar].name, 1, shipData->targetStellar + kFirstResourceID,
                                stellarData[shipData->targetStellar].onDominate);
                            EvalMissionBitSetString(stellarData[shipData->targetStellar].onDominate);
                            SetDialogGWorld(commDialog);
                            RedrawDialog(commDialog);
                        }
                        hasDemandedTribute = 1;
                    }
                }
            }
            SetDialogGWorld(commDialog);
            InvalDialogItem(commDialog, 4);
            InvalDialogItem(commDialog, 6);
        }
    } while (!done);

    if (commBackgroundPic != NULL) {
        QDReleaseResource(commBackgroundPic);
        commBackgroundPic = NULL;
    }
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
    halfW = (GetDialogPortRect(commDialog)->right - GetDialogPortRect(commDialog)->left) / 2;
    halfH = (GetDialogPortRect(commDialog)->bottom - GetDialogPortRect(commDialog)->top) / 2;
    DisposeDialogNoFlash(commDialog);
    commDialog = NULL;
    gSpeedMult = 0;
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    RefreshSAT();
#endif
    RedrawStatus(1);
    RedrawComm(0);
}

/* PlanetCommFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00481140
// MAC: _PlanetCommFilter i386 0x000911a9 ppc 0x0009cc58
Boolean PlanetCommFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
#if V111
    short key;
#else
    EventChar key;
    int what;
#endif

#if V111 /* V111 spelling */
    if (ev->what == kKeyDown) {
#else
    what = ev->what;
    if (what == kKeyDown) {
#endif
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEKeyCode
            || key == kEscapeKeyCode) {
            *itemHit = 1;
            return 1;
        }
        if (key == kOKeyCode || key == kBKeyCode) {
            if (hatesPlayer != 0) {
                *itemHit = 2;
                return 1;
            }
        }
        if (key == kGKeyCode) {
            if (hatesPlayer == 0) {
                *itemHit = 2;
                return 1;
            }
        }
        if (key == kDKeyCode || key == kTKeyCode) {
#else
        key = EventCharOf(ev);
        key = tolower(key);
        if (IsReturnChar(key) || key == 'e' || key == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        if (key == 'o' || key == 'b') {
            if (hatesPlayer != 0) {
                *itemHit = 2;
                return 1;
            }
        }
        if (key == 'g') {
            if (hatesPlayer == 0) {
                *itemHit = 2;
                return 1;
            }
        }
        if (key == 'd' || key == 't') {
#endif
            if (stellarData[shipData->targetStellar].dominated == 0) {
                *itemHit = 3;
                return 1;
            }
        }
#if V111
        if (key == kRKeyCode) {
#else
        if (key == 'r') {
#endif
            if (stellarData[shipData->targetStellar].dominated != 0 &&
                (stellarData[shipData->targetStellar].flags2 & kSpobAlwaysDominated) == 0) {
                *itemHit = 3;
                return 1;
            }
        }
    }
#if V111 /* V111 spelling */
    if (ev->what == kMouseDown) {
#else
    if (what == kMouseDown) {
#endif
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        PlanetCommDialogUpdate();
        button = TrackPlanetCommDialogButtons(where);
        if (button >= 0 && button <= kStellarCommButtons) {
            *itemHit = button + 1;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
#if V111 /* V111 spelling */
    if (ev->what == kUpdateEvt) {
#else
    if (what == kUpdateEvt) {
#endif
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(commDialog));
        PlanetCommDialogUpdate();
        EndUpdate(GetDialogWindow(commDialog));
#else
        PlanetCommDialogUpdate();
#endif
    }
    return 0;
}

/* PlanetCommDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004812c0
// MAC: _PlanetCommDialogUpdate i386 0x00090a98 ppc 0x0009c618
void PlanetCommDialogUpdate(void)
{
    Handle item;
    short itemType;
#ifndef MAC1010 /* 1.0.10 spelling */
    RGBColor rgbGold;
#endif
    unsigned char text[255]; /* BUG: a 255-character commResponseString overruns text by one */
    Rect box;
    Rect textBox;
#ifdef MAC1010
    RGBColor rgbGold;
#endif
    short i;
    short spriteWidth;
    short spriteHeight;

    rgbGold.red = 0xffff;
    rgbGold.green = 0x6666;
    rgbGold.blue = 0;
    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(commDialog));
    if (commBackgroundPic != NULL)
        DrawPicHandle(commBackgroundPic, GetDialogPortRect(commDialog));
    DrawPlanetCommDialogButtons(-1);
    GetDialogItem(commDialog, 4, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(commDialog)))) {
        strncpy((char *)text, (char *)commResponseString, 0xfe);
        PtoCstr(text);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(whiteColor);
        InsetRect(&box, 1, 1);
        PaintRect(&box);
        textBox = box;
        InsetRect(&textBox, 4, 2);
        ForeColor(blackColor);
        TETextBox(text, strlen((char *)text), &textBox, 0);
        InvertRect(&box);
    }
    GetDialogItem(commDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(commDialog)))) {
        ForeColor(blackColor);
        for (i = 0; i < kStellarSprites; i++) {
            if (stellarSprite[i]->isVisible != 0 &&
                stellarSprite[i]->userData == shipData->targetStellar) {
                spriteWidth = GetSpriteXSize(stellarSprite[i]);
                spriteHeight = GetSpriteYSize(stellarSprite[i]);
                break;
            }
        }
        textBox.left = textBox.right = (box.left + box.right) / 2;
        textBox.top = textBox.bottom = (box.top + box.bottom) / 2;
        InsetRect(&textBox, -spriteWidth / 2, -spriteHeight / 2);
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                 &textBox, &textBox, 0, NULL);
        BlitPixieRLEDrawProc(stellarSprite[i]->curFrameP, gSpriteWorld->workFrameP,
                             &stellarSprite[i]->curFrameP->frameRect, &textBox);
        CopyBits(GetPortPixMapBits(
                     gSpriteWorld->workFrameP->framePort),
                 (BitMapPtr)GWorldContainerBits(&loadPort),
                 &textBox, &textBox, 0, NULL);
    }
    GetDialogItem(commDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(commDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(box.left, (box.top + 12));
        ForeColor(whiteColor);
        DrawString(stellarData[shipData->targetStellar].name);
        MoveTo(box.left, (box.top + 0x1a));
        RGBForeColor(&rgbMedGrey);
        DrawString(stellarTypeStr);
        if (stellarData[shipData->targetStellar].flags & kSpobUninhabited) {
            MoveTo(box.left, (box.top + 0x2a));
            RGBForeColor(&rgbMedGrey);
            DrawIndString(kGameStrList, 0xc4);
            DrawString("\p  ");
            RGBForeColor(&rgbLightGrey);
            DrawIndString(kGameStrList, 0xaa);
        } else if (stellarData[shipData->targetStellar].dominated != 0) {
            MoveTo(box.left, (box.top + 0x2a));
            RGBForeColor(&rgbMedGrey);
            DrawIndString(kGameStrList, 0xc4);
            DrawString("\p  ");
            RGBForeColor(&brightGreen);
            if (stellarData[shipData->targetStellar].flags2 & kSpobAlwaysDominated)
                DrawIndString(kGameStrList, 0xab);
            else
                DrawIndString(kGameStrList, 0xac);
        } else if (hatesPlayer != 0) {
            MoveTo(box.left, (box.top + 0x2a));
            RGBForeColor(&rgbMedGrey);
            DrawIndString(kGameStrList, 0xc4);
            DrawString("\p  ");
            TextFace(1);
            if (playerRecord[shipData->curSystem] >= 0) {
                RGBForeColor(&rgbGold);
                DrawIndString(kGameStrList, 0xad);
            } else {
                ForeColor(redColor);
                DrawIndString(kGameStrList, 0xae);
            }
            TextFace(0);
        }
    }
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(commDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(commDialog)),
             GetDialogPortRect(commDialog), GetDialogPortRect(commDialog), 0,
             GetWindowVisRgn(FrontWindow(commDialog)));
}

#define kAdviceNone         175
#define kAdviceWants        176
#define kAdviceCheap        177
#define kAdviceDear         178
#define kAdviceHeard        179
#define kAdviceDestroyed    180
#define kAdviceThe          60
#define kAdviceRunningOut   181
#define kAdviceSoon         182
#define kAdviceRising       185
#define kAdviceFalling      186

#define kResponseStrings    3000
#define kResponseAdvice     46
#define kGovtStrings        7000
#define kGovtStride         10
#define kGovtPluginLow      10010
#define kGovtPluginHigh     10015
#define kGovtAdviceLow      1
#define kGovtAdviceHigh     6

#define kDudeStrings        7500
#define kDudeStringMask     0x0fff
#define kPersPlugin         15000
#define kPersStrings        7100

/* the price levels CalcPortDemand reports as kDemandLow or kDemandHigh, so a
   stellar with none of them set can never produce a price tip */
#define kSpobDemandExtremes (kSpobDemandLow0 | kSpobDemandHigh0 | \
                             kSpobDemandLow1 | kSpobDemandHigh1 | \
                             kSpobDemandLow2 | kSpobDemandHigh2 | \
                             kSpobDemandLow3 | kSpobDemandHigh3 | \
                             kSpobDemandLow4 | kSpobDemandHigh4 | \
                             kSpobDemandLow5 | kSpobDemandHigh5)

#define kNewsCount          0x100
#define kAdviceVariants     3


/* LoadAdvice (mac _LoadAdvice). STATE: MATCH 100.0%, exe 598 / obj 598, relocregcheck 0 of
   162; the stellar pick reuses `count' once the count is spent. OPEN: none */
// FUNCTION: EVNOVA 0x004819d0
// MAC: _LoadAdvice i386 0x0009133c ppc 0x0009ce20
void LoadAdvice(void)
{
    unsigned char text[256];
    short cargo;
    short which;
    short i;
    short count;
    short strCount;
    short demand;
    short d;
    Handle h;

    GetIndString(adviceString, kGameStrList, kAdviceNone);
    which = -1;
    if (commShip->dude != -1
        && (dudeData[commShip->dude].adviceFlags & kDudeInfoAny) != 0) {
        for (;;) {
            which = Rand(4);
            if (which == 0
                && (dudeData[commShip->dude].adviceFlags & kDudeInfoPrices) != 0)
                break;
            else if (which == 1
                     && (dudeData[commShip->dude].adviceFlags & kDudeInfoDisasters) != 0)
                break;
            else if (which == 2
                     && (dudeData[commShip->dude].adviceFlags & kDudeInfoAdvice) != 0)
                break;
            else if (which == 3
                     && (dudeData[commShip->dude].adviceFlags & kDudeInfoGovtHail) != 0)
                break;
        }
    }
    if (commShip->persType != -1
        && (personData[commShip->persType].flags & kPersShowsDisasters) != 0
        && personData[commShip->persType].commQuote == -1)
        which = 1;

    if (which == 0) {
        count = 0;
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].loaded != 0
                && (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) != 0
                && (stellarData[i].flags & kSpobUninhabited) == 0
                && (stellarData[i].flags & kSpobDemandExtremes) != 0)
                count++;
        }
        if (count > 0) {
            do {
                cargo = Rand(kCargoTypes);
                i = Rand(kMaxStellars);
                if (stellarData[i].loaded != 0
                    && (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) != 0
                    && (stellarData[i].flags & kSpobUninhabited) == 0) {
                    d = CalcPortDemand(cargo, stellarData[i].flags);
                    if (d != kDemandLow && d != kDemandHigh)
                        i = -1;
                } else
                    i = -1;
            } while (i == -1);
            if (stellarData[i].loaded != 0
                && (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) != 0
                && (stellarData[i].flags & kSpobUninhabited) == 0) {
                demand = CalcPortDemand(cargo, stellarData[i].flags);
                pstrcpy(text, stellarData[i].name);
                PtoCstr(text);
                strcpy((char *)adviceString, (char *)text);
                strcat((char *)adviceString, " ");
                StrcatIndCString((char *)adviceString, kGameStrList, kAdviceWants);
                strcat((char *)adviceString, " ");
                if (demand == kDemandLow)
                    StrcatIndCString((char *)adviceString, kGameStrList,
                                    kAdviceCheap);
                else
                    StrcatIndCString((char *)adviceString, kGameStrList, kAdviceDear);
                strcat((char *)adviceString, " ");
                pstrcpy(text, lcCargoName[cargo]);
                PtoCstr(text);
                strcat((char *)adviceString, (char *)text);
                strcat((char *)adviceString, ".");
                CtoPstr((char *)adviceString);
            }
        } else {
            GetIndString(adviceString, kGameStrList, kAdviceNone);
        }
    }

    if (which == 1) {
        count = 0;
        for (i = 0; i < kNewsCount; i++) {
            if (disasterData[i].countdown > 1)
                count++;
        }
        i = -1;
        if (count == 1) {
            for (i = 0; i < kNewsCount; i++) {
                if (disasterData[i].countdown > 1)
                    break;
            }
        } else if (count > 1) {
            do {
                i = Rand(kNewsCount);
            } while (disasterData[i].countdown <= 1);
        }
        if (i != -1) {
            if (disasterData[i].stellarID >= 0
                && disasterData[i].stellarID < kMaxStellars) {
                strcpy((char *)adviceString, "");
                StrcatIndCString((char *)adviceString, kGameStrList, kAdviceHeard);
                strcat((char *)adviceString, " ");
                if ((stellarData[disasterData[i].stellarID].flags & kSpobIsStation)
                        != 0)
                    StrcatIndCString((char *)adviceString, kGameStrList,
                                    kAdviceDestroyed);
                else
                    StrcatIndCString((char *)adviceString, kGameStrList, kAdviceThe);
                strcat((char *)adviceString, " ");
                pstrcpy(text, stellarData[disasterData[i].stellarID].name);
                PtoCstr(text);
                strcat((char *)adviceString, (char *)text);
                strcat((char *)adviceString, ", ");
                StrcatIndCString((char *)adviceString, kGameStrList,
                                kAdviceRunningOut);
                strcat((char *)adviceString, " ");
                pstrcpy(text, lcCargoName[disasterData[i].commodity]);
                PtoCstr(text);
                strcat((char *)adviceString, (char *)text);
                strcat((char *)adviceString, " ");
                StrcatIndCString((char *)adviceString, kGameStrList,
                                (Rand(kAdviceVariants) + kAdviceSoon));
                strcat((char *)adviceString, " ");
                if (disasterData[i].amount < 0)
                    StrcatIndCString((char *)adviceString, kGameStrList,
                                    kAdviceRising);
                else
                    StrcatIndCString((char *)adviceString, kGameStrList,
                                    kAdviceFalling);
                strcat((char *)adviceString, ".");
                CtoPstr((char *)adviceString);
            } else {
                GetIndString(adviceString, kGameStrList, kAdviceNone);
            }
        }
    }

    if (which == 2) {
        h = GetResource(kStringListType,
                        ((dudeData[commShip->dude].adviceFlags
                                 & kDudeStringMask) + kDudeStrings));
        if (h != NULL) {
            LockAHandle(h);
#ifdef MACBUILD
            BlockMove(*h, &strCount, sizeof(strCount));
            UnlockAHandle(h);
            HPurge(h);
#else
            strCount = SwapEndianShort((const unsigned short *)*h);
            UnlockAHandle(h);
#endif
            ReleaseResource(h);
        }
        if (strCount > 0) {
            GetIndString(text,
                         (dudeData[commShip->dude].adviceFlags & kDudeStringMask)
                             + kDudeStrings,
                         Rand((adviceRandom % strCount + 1)) + 1);
            if (text[0] != 0) {
                pstrcpy((unsigned char *)parseBuffer, text);
                PtoCstr((unsigned char *)parseBuffer);
                ParseTextDescription();
                strcpy((char *)adviceString, parseBuffer);
                CtoPstr((char *)adviceString);
            } else {
                GetIndString(adviceString, kGameStrList, kAdviceNone);
            }
        }
    }

    if (which == 3) {
        if (commShip->aiType <= kShipTypeBraveTrader) {
            if (!LoadPluginString((commShip->govt * kGovtStride
                                          + kGovtPluginLow + responseRandom),
                                  (char *)adviceString))
                GetIndString(adviceString, kGovtStrings + commShip->govt,
                             responseRandom + kGovtAdviceLow);
        } else {
            if (!LoadPluginString((commShip->govt * kGovtStride
                                          + kGovtPluginHigh + responseRandom),
                                  (char *)adviceString))
                GetIndString(adviceString, kGovtStrings + commShip->govt,
                             responseRandom + kGovtAdviceHigh);
        }
    }

    if (adviceString[0] == '*' || adviceString[1] == '*'
        || strlen((char *)adviceString) <= 2)
        GetIndString(adviceString, kResponseStrings,
                     responseRandom + kResponseAdvice);

    if (commShip->persType != -1
        && personData[commShip->persType].commQuote >= 1
        && !LoadPluginString((personData[commShip->persType].commQuote
                                     + kPersPlugin),
                             (char *)adviceString))
        GetIndString(adviceString, kPersStrings,
                     personData[commShip->persType].commQuote);
}


#define kHaggleMission   0
#define kHaggleTrade     1
#define kHaggleCapture   2
#define kHaggleTradeStr  0xbd
#define kHaggleCaptureA  0xbb
#define kHaggleCaptureB  0xbc
#define kTextInsetH      4
#define kTextBaseline    0xc

#define kHaggleDlogID     0x3f0
#define kHagglePictID    0x2142
#define kHaggleRaise          1
#define kHaggleLower          2
#define kHaggleChanceMax    100
#define kHaggleRound        100
#define kItemDone             1
#define kItemHaggle           2

/* DoHaggleDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00482280
// MAC: _DoHaggleDialog i386 0x00091e67 ppc 0x0009d9e4
short DoHaggleDialog(Boolean raise, short chance)
{
    float scratch;
    int32_t fscratch;
    short itemHit;
    int32_t conv;
    short result;
    Boolean canHaggle;
    Boolean done;
    short halfW;
    short halfH;

    done = 0;
    if (raise != 0)
        haggleMode = kHaggleRaise;
    else
        haggleMode = kHaggleLower;
    result = 0;
    if (Rand(kHaggleChanceMax) <= chance)
        canHaggle = 1;
    else
        canHaggle = 0;

    haggleDialog = NULL;
    haggleDialog = GetNewDialog(kHaggleDlogID, HaggleDialogUpdate, HaggleFilter);
    if (haggleDialog == NULL)
        return 0;

    haggleBackgroundPic = GetPicture(kHagglePictID);
    CenterWindow(FrontWindow(haggleDialog), gWind);
    ShowWindowNoFlash(FrontWindow(haggleDialog));
    SetDialogGWorld(haggleDialog);
    HaggleDialogUpdate();
    SelectWindow(FrontWindow(haggleDialog));
    BringToFront(FrontWindow(haggleDialog));
    FlushEvents(everyEvent - diskMask, 0);

    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(haggleDialog, HaggleFilter, &itemHit, 0);
        if (itemHit == kItemDone) {
            DrawHaggleButtons(-1);
            if (result != 2)
                result = 1;
            done = 1;
        }
        if (itemHit == kItemHaggle) {
            if (canHaggle != 0) {
                if (haggleMode == kHaggleRaise)
                    haggleVal *= 1.33;
                if (haggleMode == kHaggleLower)
                    haggleVal *= 0.75;
                conv = haggleVal;
                conv *= 0.01;
                haggleVal = conv * kHaggleRound;
                HaggleDialogUpdate();
                canHaggle = 0;
                result = 1;
            } else {
                result = 2;
                done = 1;
            }
            DrawHaggleButtons(-1);
        }
    } while (done == 0);

    if (haggleBackgroundPic != NULL) {
        QDReleaseResource(haggleBackgroundPic);
        haggleBackgroundPic = NULL;
    }
    halfW = (GetDialogPortRect(haggleDialog)->right - GetDialogPortRect(haggleDialog)->left) / 2;
    halfH = (GetDialogPortRect(haggleDialog)->bottom - GetDialogPortRect(haggleDialog)->top) / 2;
    halfW = (GetCWindowPortRect((WindowRef)gWind)->right - GetCWindowPortRect((WindowRef)gWind)->left) / 2;
    halfH = (GetCWindowPortRect((WindowRef)gWind)->bottom - GetCWindowPortRect((WindowRef)gWind)->top) / 2;
    DisposeDialogNoFlash(haggleDialog);
    haggleDialog = NULL;
#ifdef MACBUILD
    RefreshSAT();
#endif
    return result;
}

/* HaggleFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004825d0
// MAC: _HaggleFilter i386 0x00091d9d ppc 0x0009d8e8
Boolean HaggleFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
#if V111
    short key;
#endif
    Point where;
    short button;

    if ((int)ev->what == kKeyDown) {
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode) {
            *itemHit = 1;
            return 1;
        }
        if (key == kAKeyCode) {
            *itemHit = 1;
            return 1;
        }
        if (key == kLKeyCode) {
            *itemHit = 2;
            return 1;
        }
#else
        if (IsReturnChar(EventCharOf(ev))) {
            *itemHit = 1;
            return 1;
        }
        if (EventCharOf(ev) == 'a') {
            *itemHit = 1;
            return 1;
        }
        if (EventCharOf(ev) == 'l') {
            *itemHit = 2;
            return 1;
        }
#endif
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        HaggleDialogUpdate();
        button = TrackHaggleButtons(where);
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
    if ((int)ev->what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(haggleDialog));
        HaggleDialogUpdate();
        EndUpdate(GetDialogWindow(haggleDialog));
#else
        HaggleDialogUpdate();
#endif
    }
    return 0;
}

/* HaggleDialogUpdate (mac _HaggleDialogUpdate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004826a0
// MAC: _HaggleDialogUpdate i386 0x00091b65 ppc 0x0009d6ac
void HaggleDialogUpdate(void)
{
    Rect box;
    short itemType;
    Handle item;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(haggleDialog));
    if (haggleBackgroundPic != NULL)
        DrawPicHandle(haggleBackgroundPic, GetDialogPortRect(haggleDialog));
    GetDialogItem(haggleDialog, 3, &itemType, &item, &box);
    ForeColor(whiteColor);
    MoveTo(box.left + kTextInsetH, box.top + kTextBaseline);
    if (haggleMode == kHaggleMission) {
        DrawString("\pThe pay for having completed this mission is ");
    } else if (haggleMode == kHaggleTrade) {
        DrawIndString(kGameStrList, kHaggleTradeStr);
        DrawString("\p ");
    } else if (haggleMode == kHaggleCapture) {
        if (shipData->targetShip != -1)
            DrawIndString(kGameStrList, kHaggleCaptureA);
        else
            DrawIndString(kGameStrList, kHaggleCaptureB);
        DrawString("\p ");
    }
    DrawCommaNumber(haggleVal);
    DrawString("\p ");
    if (haggleVal > 1)
        DrawString(creditsString);
    else
        DrawString(creditString);
    DrawString("\p.");
    ForeColor(blackColor);
    DrawHaggleButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(haggleDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(haggleDialog)),
             GetDialogPortRect(haggleDialog), GetDialogPortRect(haggleDialog), 0,
             GetWindowVisRgn(FrontWindow(haggleDialog)));
}

/*
 * The two STR# fetches behind the haggle dialog (win 0x004828c0
 * and 0x00482910).
 *
 * Each haggler owns five consecutive strings in its list, so an index scales
 * by five against a per-game base. The first fetch splits at 0x25 because the
 * greetings ran past one STR#'s 255 entries and continue in the next.
 */

#define kHaggleStride       5
#define kHaggleSplit        0x25
#define kHaggleGreetHigh    (kResponseStrings + 1)
#define kHaggleHighBias     -0xbd

/* LoadResponse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004828c0
// MAC: _LoadResponse i386 0x00090a0b ppc 0x0009c590
void LoadResponse(short which)
{
    if (which <= kHaggleSplit)
        GetIndString(commResponseString, kResponseStrings,
                     which * kHaggleStride + responseRandom + 1);
    else
        GetIndString(commResponseString, kHaggleGreetHigh,
                     which * kHaggleStride + responseRandom + kHaggleHighBias);
}

/* LoadStellarResponse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00482910
// MAC: _LoadStellarResponse i386 0x00090a65 ppc 0x0009c5ec
void LoadStellarResponse(short which)
{
    GetIndString(commResponseString, kTributeList,
                 which * kHaggleStride + responseRandom + 1);
}

/*
 * Note the fifth button reports 6, not 5, so the mapping is a chain of
 * explicit tests rather than an offset.
 */

#define kRegisterStrList 30000


#define kOutfitModPlunder    0x19    /* modType that shifts the capture odds */

#define kMinShipCredits      1000
#define kMaxCaptureOdds        75
#define kOddsJitter            11
#define kOddsJitterMid          5

/* only the plunder-class word is read here; stride from the mac's dudeData */

#define kPlunderDlogID 0x3f3
#define kPlunderPictID 0x2143

/* DoPlunderDialog (mac _DoPlunderDialog). STATE: MATCH 100.0%, 1552/1552, byte-identical yes;
   mac MATCH both slices. OPEN: nothing. */
// FUNCTION: EVNOVA 0x00482940
// MAC: _DoPlunderDialog i386 0x0009302b ppc 0x0009ef7c
void DoPlunderDialog(void)
{
#define n i
#define idx i
#define space qty
    short pers;
    char c;
    short avail;
    short count;
    short itemHit;
    short j;
    short i;
    short qty;
    short found;
    short govt;
    short type;
    Boolean ok;
    Boolean ok2;
    short risk;
    Boolean risky;
    Boolean done;
    unsigned char text[256];
    unsigned char tmp[256];

    risky = 0;
    done = 0;
    risk = Rand(26) + 15;
    SetPlunderValues();

    pers = shipData[shipData->targetShip].persType;
    if (pers != -1 && personData[pers].grantClass > 0 && personData[pers].grantProb > 0
        && personData[pers].grantCount > 0
        && Rand(100) + 1 <= personData[pers].grantProb) {
        count = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (outfitSystem[i].itemClass == personData[pers].grantClass
                && HasMaxOfItem(i, 0, 0) == 0)
                count++;
        }
        if (count > 0) {
            do {
                n = Rand(kMaxOutfits);
                idx = n;
            } while (outfitSystem[idx].itemClass != personData[pers].grantClass
                     || HasMaxOfItem(n, 0, 0) != 0);
            qty = ((Rand(51) + 50) * personData[pers].grantCount / 100.0);
            if (qty <= 0)
                qty = 1;
            ShipStatsToSystemInfo();
            avail = ShipFreeMass();
            while (qty > 0 && outfitSystem[idx].mass * qty > avail)
                qty--;
            if (qty > 0) {
                for (j = 0; j < qty; j++)
                    GrantOutfitItem(n);
                SystemInfoToShipStats();
                strcpy((char *)text, "");
                StrcatIndCString((char *)text, kGameStrList, 0x6a);
                if (qty == 1) {
                    c = tolower(lcOutfitName[idx][1]);
                    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                        StrcatIndCString((char *)text, kGameStrList, 0x18a);
                    else
                        StrcatIndCString((char *)text, kGameStrList, 0x189);
                } else if (qty >= 2 && qty <= 10) {
                    StrcatIndCString((char *)text, kOrdinalsSTR, qty + kOrdinalsBase);
                } else {
                    NumToString(qty, tmp);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                }
                strcat((char *)text, " ");
                if (qty > 1)
                    pstrcpy(tmp, lcPluralOutfitName[idx]);
                else
                    pstrcpy(tmp, lcOutfitName[idx]);
                PtoCstr(tmp);
                strcat((char *)text, (char *)tmp);
                strcat((char *)text, " ");
                StrcatIndCString((char *)text, kGameStrList, 0x6c);
                CtoPstr((char *)text);
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                RefreshSAT();
#endif
            }
        }
    }

    ShowTheCursor();
    plunderDialog = NULL;
    plunderDialog = GetNewDialog(kPlunderDlogID, PlunderDialogUpdate,
                                 PlunderFilter);
    if (plunderDialog == NULL)
        return;
    plunderBackgroundPic = GetPicture(kPlunderPictID);
    CenterWindow(FrontWindow(plunderDialog), gWind);
    ShowWindowNoFlash(FrontWindow(plunderDialog));
    SetDialogGWorld(plunderDialog);
    PlunderDialogUpdate();
    SelectWindow(FrontWindow(plunderDialog));
    BringToFront(FrontWindow(plunderDialog));
    FlushEvents(everyEvent - diskMask, 0);
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(plunderDialog, PlunderFilter, &itemHit, 0);
        if (itemHit == 1) {
            risk = -1;
            risky = 0;
            PlunderDialogUpdate();
            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
            done = 1;
        }
        if (risky != 0 && Rand(100) <= risk) {
            shipData[shipData->targetShip].shield = 0;
            shipData[shipData->targetShip].armor = 0;
            shipData[shipData->targetShip].deathTimer = 0;
            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
            GetIndString(text, kGameStrList, 0x71);
            DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
            SetDialogGWorld(plunderDialog);
            break;
        }
        risky = 0;
        if (itemHit == 2) {
            PlunderDialogUpdate();
            if (salvageType != -1) {
                if (salvageQty > TotalFleetHolds() - TotalCargo(shipData))
                    salvageQty = TotalFleetHolds() - TotalCargo(shipData);
                if (salvageQty <= 0) {
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    GetIndString(text, kGameStrList, 0x72);
                    DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                } else {
                    strcpy((char *)text, "");
                    StrcatIndCString((char *)text, kGameStrList, 0x73);
                    strcat((char *)text, " ");
                    NumToString(salvageQty, tmp);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                    if (salvageQty == 1)
                        pstrcpy(tmp, tonString);
                    else
                        pstrcpy(tmp, tonsString);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                    StrcatIndCString((char *)text, kGameStrList, 0x187);
                    strcat((char *)text, " ");
                    pstrcpy(tmp, lcCargoName[salvageType]);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                    strcat((char *)text, " ");
                    StrcatIndCString((char *)text, kGameStrList, 0x6c);
                    CtoPstr((char *)text);
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                    shipData->cargo[salvageType] += salvageQty;
                    statusRedraw[kRedrawCargo] = 1;
                    salvageQty = 0;
                    salvageType = -1;
                    SetDialogGWorld(plunderDialog);
                    RedrawDialog(plunderDialog);
                }
                salvageType = -1;
                risk *= 2.0;
                risky = 1;
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
        if (itemHit == 3) {
            SetDialogGWorld(plunderDialog);
            PlunderDialogUpdate();
            if (salvageCredits > 0) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                strcpy((char *)text, "");
                StrcatIndCString((char *)text, kGameStrList, 0x74);
                strcat((char *)text, " ");
                strcat((char *)text, creditsCString);
                strcat((char *)text, " ");
                StrcatIndCString((char *)text, kGameStrList, 0x6c);
                CtoPstr((char *)text);
                DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                RefreshSAT();
#endif
                shipData->credits += salvageCredits;
                risk *= 1.25;
                risky = 1;
                statusRedraw[kRedrawCargo] = 1;
                DrawStatus();
                salvageCredits = 0;
                SetDialogGWorld(plunderDialog);
                RedrawDialog(plunderDialog);
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
        if (itemHit == 4) {
            PlunderDialogUpdate();
            ShipStatsToSystemInfo();
            found = -1;
            if (salvageAmmoType != (SalvageAmmo)-1) {
                for (i = 0; i < kMaxOutfits; i++) {
                    if (outfitSystem[i].modType[0] == kOutfModAmmo
                        && salvageAmmoType == (SalvageAmmo)outfitSystem[i].modVal[0]) {
                        found = i;
                        break;
                    }
                }
            }
            if (found != -1) {
                qty = 0;
                for (;;) {
                    if (!(ShipFreeMass() >= outfitSystem[found].mass
                          && qty < salvageAmmoQty
                          && HasMaxOfItem(found, 0, 0) == 0))
                        break;
                    shipData->weap[(short)salvageAmmoType].ammo++;
                    qty++;
                    ShipStatsToSystemInfo();
                }
                if (qty > 0) {
                    strcpy((char *)text, "");
                    StrcatIndCString((char *)text, kGameStrList, 0x73);
                    strcat((char *)text, " ");
                    StrcatOrdinalNumber(qty, (char *)text, 0);
                    strcat((char *)text, " ");
                    if (qty > 1)
                        pstrcpy(tmp, lcPluralOutfitName[found]);
                    else
                        pstrcpy(tmp, lcOutfitName[found]);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                    strcat((char *)text, " ");
                    StrcatIndCString((char *)text, kGameStrList, 0x6c);
                    CtoPstr((char *)text);
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                } else {
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    GetIndString(text, kGameStrList, 0x75);
                    DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                }
                statusRedraw[kRedrawWeap] = 1;
                risk *= 2.0;
                risky = 1;
                salvageAmmoQty = 0;
                salvageAmmoType = -1;
                SetDialogGWorld(plunderDialog);
                RedrawDialog(plunderDialog);
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
        if (itemHit == 6) {
            PlunderDialogUpdate();
            if (salvageFuelQty > 0) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                space = (ShipFuelCapacity(shipData) - shipData->fuel);
                if (space > salvageFuelQty)
                    space = salvageFuelQty;
                SWSetPortToWindow(gSpriteWorld);
                if (space > 0) {
                    shipData->fuel = shipData->fuel + space;
                    if (shipData->fuel >= ShipFuelCapacity(shipData))
                        DisplayComm((PStrChar *)fuelString[1], kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
                    else
                        DisplayComm((PStrChar *)fuelString[2], kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                } else {
                    DisplayComm((PStrChar *)fuelString[3], kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                }
                statusRedraw[kRedrawFuel] = 1;
                DrawStatus();
                risk *= 1.5;
                risky = 1;
                salvageFuelQty = 0;
                SetDialogGWorld(plunderDialog);
                RedrawDialog(plunderDialog);
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
        if (itemHit == 7) {
            PlunderDialogUpdate();
            govt = shipData[shipData->targetShip].govt;
            if (govt != -1 && (govtData[govt].flags & kGovtAlwaysDisabled) != 0)
                captureOdds = -1;
            if ((Rand(100) <= captureOdds || cheatsActive != 0)
                && captureOdds > 0) {
                if (Rand(10) == 0) {
                    done = 1;
                    shipData[shipData->targetShip].shield = 0;
                    shipData[shipData->targetShip].armor = 0;
                    shipData[shipData->targetShip].deathTimer = 0;
                    GetIndString(text, kGameStrList, 0x71);
                    DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                    SetDialogGWorld(plunderDialog);
                } else {
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    if (CanHireEscorts() != 0) {
                        if (shipClass[shipData->type].crew > 0)
                            ok = DoCapturedShipAssignmentDialog();
                        else
                            ok = 0;
                        if (ok != 0) {
                            done = 1;
                            GetIndString(text, kGameStrList, 0x77);
                            pstrcpy(tmp, shipClass[shipData[shipData->targetShip].type].name);
                            tmp[0]++;
                            tmp[tmp[0]++] = ' ';
                            tmp[tmp[0]++] = Rand(9) + '1';
                            tmp[tmp[0]++] = Rand(9) + '1';
                            tmp[tmp[0]++] = Rand(9) + '1';
                            tmp[tmp[0]] = 0;
                            ok2 = EVTextInputDialog(text, tmp, 0x40);
                            SetDialogGWorld(plunderDialog);
                            if (ok2 != 0) {
                                pstrcpy(shipName, textInputString);
                                SetDialogGWorld(plunderDialog);
                                DoShipCapture(&shipData[shipData->targetShip], 0);
                                SetDialogGWorld(plunderDialog);
                                LoadIntfResource();
                            } else {
                                GetIndString(text, kGameStrList, 0x7a);
                                DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
                                SetDialogGWorld(plunderDialog);
                            }
                        } else {
                            done = 1;
                            type = shipData[shipData->targetShip].type;
                            crashlog_mission_bit_set("capturing",
                                                     (char *)shipClass[type].name, 1,
                                                     type + kFirstResourceID, shipClass[type].onCapture);
                            EvalMissionBitSetString(shipClass[shipData[shipData->targetShip].type].onCapture);
                            shipData[shipData->targetShip].aiType = kShipTypeEscort;
                            shipData[shipData->targetShip].parentShip = 0;
                            shipData[shipData->targetShip].hired = 0;
                            shipData[shipData->targetShip].armor =
                                ShipArmorCapacity(&shipData[shipData->targetShip]) * 0.5;
                            shipData[shipData->targetShip].govt = -1;
                            shipData[shipData->targetShip].persType = -1;
                            shipData[shipData->targetShip].targetShip = -1;
                            shipData[shipData->targetShip].boarded = 1;
                            shipData[shipData->targetShip].cloakDir = 0;
                            shipData[shipData->targetShip].cloak = 0;
                            shipData[shipData->targetShip].defenseStellar = -1;
                            shipData[shipData->targetShip].grudge = 0;
                            shipData[shipData->targetShip].targetShip = -1;
                            shipData[shipData->targetShip].sellOnLanding = 0;
                            shipData[shipData->targetShip].upgradeOnLanding = 0;
                            shipData[shipData->targetShip].fuelRegenState = -1;
                            AISetup(&shipData[shipData->targetShip]);
                            AIShipCapturedByPlayer(&shipData[shipData->targetShip]);
                            for (i = 0; i < 4; i++)
                                shipData[shipData->targetShip].jamming[i] = -1;
                            GetIndString(text, kGameStrList, 0x7b);
                            DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                            RefreshSAT();
#endif
                            SetDialogGWorld(plunderDialog);
                            statusRedraw[kRedrawCargo] = 1;
                        }
                    } else {
                        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                        GetIndString(text, kGameStrList, 0x7c);
                        DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                        RefreshSAT();
#endif
                        SetDialogGWorld(plunderDialog);
                    }
                }
            } else {
                done = 1;
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                GetIndString(text, kGameStrList, 0x7d);
                DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
#ifdef MACBUILD
                RefreshSAT();
#endif
                SetDialogGWorld(plunderDialog);
            }
        }
        if (done == 0) {
            PlunderDialogUpdate();
            HandleComm();
            SetDialogGWorld(plunderDialog);
        }
    } while (done == 0);
    if (plunderBackgroundPic != NULL) {
        QDReleaseResource(plunderBackgroundPic);
        plunderBackgroundPic = NULL;
    }
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialogNoFlash(plunderDialog);
    plunderDialog = NULL;
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    RefreshSAT();
#endif
    ResetPlayerPrecalcedValues();
#undef n
#undef idx
#undef space
}

#ifdef MACBUILD
short playerStudliness;

/* EvalPlayerStudliness (mac-only, no caller in 1.1.1) */
// MAC: _EvalPlayerStudliness i386 0x00092190 ppc 0x0009ddf0
void EvalPlayerStudliness(void)
{
    ShipObj *ship;

    ship = commShip;
    playerStudliness = 0;
    if (ship->govt == systemData[ship->curSystem].govt) {
        if (playerRecord[ship->curSystem] > 0)
            playerStudliness = 1;
        if (playerRecord[ship->curSystem] >= 120)
            playerStudliness += 2;
    }
    if (playerKills > 0)
        playerStudliness++;
    if (playerKills >= 500)
        playerStudliness += 2;
}
#endif

/* SetPlunderValues (mac _SetPlunderValues). STATE: MATCH 100.0%, exe 611 / obj 611,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x00484230
// MAC: _SetPlunderValues i386 0x00092219 ppc 0x0009debc
void SetPlunderValues(void)
{
    short count;
    short flags;
    short dude;
    short pers;
    short n;
    short i;
    short j;
    short fuel;
    int32_t crew;
    int32_t mass;
    Boolean ok;
#ifdef MAC1010
    extern Boolean PlayerCanUseAmmoType(short weapon);
#endif

    dude = shipData[shipData->targetShip].dude;
    if (dude != -1)
        flags = dudeData[dude].flags;
    else
        flags = 0;

    if ((flags & kDudeCarriesMoney) != 0) {
        count = shipClass[shipData[shipData->targetShip].type].cost / 1000;
        if (count * 0.025 > 2.0)
            salvageCredits = ((Rand(count * 0.025) + count * 0.025) * 1000.0);
        else
            salvageCredits = (count * 0.025 * 1000.0);
        if (salvageCredits < kMinShipCredits)
            salvageCredits = kMinShipCredits;
    } else {
        pers = shipData[shipData->targetShip].persType;
        if (pers != -1) {
            count = personData[pers].credits / 1000;
            if (count * 0.5 > 2.0)
                salvageCredits = ((Rand(count * 0.5) + count * 0.5) * 1000.0);
            else
                salvageCredits = (count * 0.5 * 1000.0);
        } else {
            salvageCredits = -1;
        }
    }
    if (salvageCredits <= 0)
        salvageCredits = -1;

    if ((flags & (kDudeCargoMask | 0x80 | 0xff00)) != 0) {
        ok = 0;
        while (ok == 0) {
            salvageType = Rand(7);
            ok = 0;
            if (salvageType == 0 && (flags & 0x01) != 0)
                ok = 1;
            if (salvageType == 1 && (flags & 0x02) != 0)
                ok = 1;
            if (salvageType == 2 && (flags & 0x04) != 0)
                ok = 1;
            if (salvageType == 3 && (flags & 0x08) != 0)
                ok = 1;
            if (salvageType == 4 && (flags & 0x10) != 0)
                ok = 1;
            if (salvageType == 5 && (flags & 0x20) != 0)
                ok = 1;
        }
        if (shipClass[shipData[shipData->targetShip].type].cargo > 0) {
            salvageQty = Rand(shipClass[shipData[shipData->targetShip].type].cargo / 2)
                         + shipClass[shipData[shipData->targetShip].type].cargo / 2;
        } else {
            salvageQty = 0;
            salvageType = -1;
        }
    } else {
        salvageType = -1;
    }
    if (salvageQty <= 0)
        salvageType = -1;
    if (salvageType == -1)
        salvageQty = 0;

    count = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (shipData[shipData->targetShip].weap[i].ammo > 0
            && weaponStats[i].guidance != kGuidanceBay
            && PlayerCanUseAmmoType(i) != 0)
            count++;
    }
    if (count > 0) {
        do {
            n = Rand(kMaxWeapTypes);
        } while (shipData[shipData->targetShip].weap[n].ammo <= 0
                 || weaponStats[n].guidance == kGuidanceBay
                 || PlayerCanUseAmmoType(n) == 0);
        salvageAmmoType = n;
        salvageAmmoQty = shipData[shipData->targetShip].weap[n].ammo;
        count = salvageAmmoQty;
    } else {
        salvageAmmoType = -1;
    }

    fuel = shipClass[shipData[shipData->targetShip].type].fuel;
    if (fuel > 0)
        salvageFuelQty = Rand(fuel / 10) * 10;
    else
        salvageFuelQty = 0;

    crew = shipClass[shipData->type].strength;
    mass = shipClass[shipData->type].crew;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0
            && shipData[i].aiType == kShipTypeEscort && shipData[i].specialMission == -1
            && shipClass[shipData[i].type].inherentAI >= kShipTypeWarship) {
            crew += shipClass[shipData[i].type].strength * 0.1;
            mass += shipClass[shipData[i].type].crew * 0.1;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        for (j = 0; j < kOutfMods; j++) {
            if (outfitSystem[i].modType[j] == kOutfitModPlunder
                && outfitSystem[i].modVal[j] > 0)
                mass += outfitSystem[i].modVal[j] * shipSystems[i];
        }
    }

    captureOdds = (mass
                          / (shipClass[shipData[shipData->targetShip].type].crew * 10.0)
                          * 100.0);
    for (i = 0; i < kMaxOutfits; i++) {
        for (j = 0; j < kOutfMods; j++) {
            if (outfitSystem[i].modType[j] == kOutfitModPlunder
                && outfitSystem[i].modVal[j] < 0)
                captureOdds += -1 * outfitSystem[i].modVal[j] * shipSystems[i];
        }
    }
    if (crew > shipClass[shipData[shipData->targetShip].type].strength * 5)
        captureOdds += 10;
    captureOdds += kOddsJitterMid - Rand(kOddsJitter);
    if (captureOdds < 1)
        captureOdds = 1;
    if (captureOdds > kMaxCaptureOdds)
        captureOdds = kMaxCaptureOdds;

    if (shipData[shipData->targetShip].govt != -1
        && (govtData[shipData[shipData->targetShip].govt].flags & kGovtAlwaysDisabled) != 0)
        captureOdds = 0;
    if (((shipClass[gameIsRegisteredIndex].registered != 0) ? 1 : 0) == 0)
        captureOdds = 0;
    if (CanHireEscorts() == 0)
        captureOdds = 0;
}

/* PlunderFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00484c20
// MAC: _PlunderFilter i386 0x00092f20 ppc 0x0009ee40
Boolean PlunderFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
#if V111
    short key;
#endif
    Point where;
    short button;

    if ((int)ev->what == kKeyDown) {
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
#else
        if (IsReturnChar(EventCharOf(ev)) ||
            EventCharOf(ev) == kEscapeChar) {
#endif
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        PlunderDialogUpdate();
        button = TrackPlunderButtons(where);
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
            *itemHit = 6;
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
        BeginUpdate(GetDialogWindow(plunderDialog));
        PlunderDialogUpdate();
        EndUpdate(GetDialogWindow(plunderDialog));
#else
        PlunderDialogUpdate();
#endif
    }
    return 0;
}

/* PlunderDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00484d30
// MAC: _PlunderDialogUpdate i386 0x000928b4 ppc 0x0009e884
void PlunderDialogUpdate(void)
{
    short itemType;
    Handle item;
    Rect box;
    unsigned char text[256];
    short i;
    short j;
    Boolean found;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(plunderDialog));
    if (plunderBackgroundPic != NULL)
        DrawPicHandle(plunderBackgroundPic, GetDialogPortRect(plunderDialog));
    GetDialogItem(plunderDialog, 5, &itemType, &item, &box);
    ForeColor(blackColor);
    PaintRect(&box);
    RGBForeColor(&rgbLightGrey);
    MoveTo(box.left, (box.top + 12));
    DrawIndString(kGameStrList, 0x6d);
    RGBForeColor(&rgbLightGrey);
    MoveTo(box.left, (box.top + 0x1c));
    DrawIndString(kGameStrList, 0x6e);
    MoveTo(box.left, (box.top + 0x2a));
    pstrcpy(text, creditsString);
    if (text[0] != 0) {
        text[1] = toupper(text[1]);
    }
    DrawString(text);
    DrawString("\p:");
    MoveTo(box.left, (box.top + 0x38));
    DrawIndString(kGameStrList, 0x6f);
    MoveTo((box.left + 0x32), (box.top + 0x1c));
    if (salvageType != -1) {
        ForeColor(whiteColor);
        NumToString(salvageQty, text);
        DrawString(text);
        DrawString("\p ");
        if (salvageQty > 1)
            DrawString(tonsString);
        else
            DrawString(tonString);
        DrawString("\p ");
        DrawIndString(kGameStrList, 0x187);
        DrawString("\p ");
        DrawString(lcCargoName[salvageType]);
    } else {
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 0x14f);
    }
    MoveTo((box.left + 0x32), (box.top + 0x2a));
    if (salvageCredits > 0) {
        ForeColor(whiteColor);
        DrawCommaNumber(salvageCredits);
    } else {
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 0x14f);
    }
    MoveTo((box.left + 0x32), (box.top + 0x38));
    if (salvageAmmoType != (SalvageAmmo)-1) {
        ForeColor(whiteColor);
        NumToString(salvageAmmoQty, text);
        DrawString(text);
        DrawString("\p ");
        found = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kOutfModAmmo &&
                    salvageAmmoType == (SalvageAmmo)outfitSystem[i].modVal[j]) {
                    if (salvageAmmoQty == 1)
                        DrawString(lcOutfitName[i]);
                    else
                        DrawString(lcPluralOutfitName[i]);
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
        }
    } else {
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 0x14f);
    }
    MoveTo((box.left + 1), (box.top + 0x46));
    RGBForeColor(&rgbLightGrey);
    DrawString(fuelString[4]);
    MoveTo((box.left + 0x32), (box.top + 0x46));
    if (salvageFuelQty > 0) {
        ForeColor(whiteColor);
        DrawCommaNumber(salvageFuelQty);
    } else {
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 0x14f);
    }
    MoveTo((box.left + 0x78), (box.top + 0x46));
    RGBForeColor(&rgbLightGrey);
    DrawIndString(kGameStrList, 0x70);
    MoveTo((box.left + 0xc3), (box.top + 0x46));
    ForeColor(whiteColor);
    NumToString(captureOdds, text);
    DrawString(text);
    DrawString("\p%");
    if (((shipClass[gameIsRegisteredIndex].registered != 0) ? 1 : 0) == 0) {
        DrawString("\p  (");
        DrawIndString(kRegisterStrList, 1);
        DrawString("\p)");
    }
    ForeColor(blackColor);
    DrawPlunderButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(plunderDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(plunderDialog)),
             GetDialogPortRect(plunderDialog), GetDialogPortRect(plunderDialog), 0,
             GetWindowVisRgn(FrontWindow(plunderDialog)));
}

/*
 * Shares commDialog/commShip/commBackgroundPic/commShipPic/canUpgradeEscort/
 * canSellEscort with the comm dialog (same global slots, one dialog open at a
 * time) and reuses beepSnd[2] for its own close sound.
 */

#define kEscortDialogID 0x3fe
#define kEscortPictID   0x2141

/* DoEscortDialog (mac _DoEscortDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004853a0
// MAC: _DoEscortDialog i386 0x000951ee ppc 0x000a0eb4
void DoEscortDialog(ShipObj *ship)
{
    short itemHit;
    Boolean done;

    if (ship == NULL)
        return;

    commShip = ship;
    pstrcpy(shipClassStr, shipCommNames[commShip->type]);
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    commShipPic = GetPicture(shipClass[commShip->type].shipyardPictID);
    ShowTheCursor();

    commDialog = NULL;
    commDialog = GetNewDialog(kEscortDialogID, EscortDialogUpdate, EscortFilter);
    if (commDialog == NULL)
        return;

    commBackgroundPic = GetPicture(kEscortPictID);
    CenterWindow(FrontWindow(commDialog), gWind);

    canUpgradeEscort = 0;
    canSellEscort = 0;
    if (commShip->hired == 0)
        canSellEscort = 1;

    if (shipClass[commShip->type].upgradeTo >= 0 && shipClass[commShip->type].upgradeTo < kMaxShipTypes) {
        if (EvalMissionBitTestString(shipClass[shipClass[commShip->type].upgradeTo].availability))
            canUpgradeEscort = 1;
    }

    done = 0;
    commDialogMode = 2;

    ShowWindowNoFlash(FrontWindow(commDialog));
    SetDialogGWorld(commDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(commDialog);
    EscortDialogUpdate();

    SelectWindow(FrontWindow(commDialog));
    BringToFront(FrontWindow(commDialog));
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);

    do {
        if (gQuit)
            done = 1;

        MyModalDialog(commDialog, EscortFilter, &itemHit, 0);

        if (itemHit == 1) {
            done = 1;
        } else if (itemHit == 2) {
            DestroyPartialFleetCargo(commShip->index);
            commShip->parentShip = -1;
            commShip->formerEscort = -1;
            commShip->boarded = 1;
            commShip->aiType = shipClass[commShip->type].inherentAI;
            AISetup(commShip);
            AIMakeShipLeave(commShip);
            statusRedraw[kRedrawCargo] = 1;
            done = 1;
        } else if (itemHit == 3) {
            if (canUpgradeEscort) {
                commShip->upgradeOnLanding = (commShip->upgradeOnLanding == 0);
                if (commShip->upgradeOnLanding)
                    commShip->sellOnLanding = 0;
            }
        } else if (itemHit == 4) {
            if (canSellEscort) {
                commShip->sellOnLanding = (commShip->sellOnLanding == 0);
                if (commShip->sellOnLanding)
                    commShip->upgradeOnLanding = 0;
            }
        }

        RedrawDialog(commDialog);
    } while (!done);

    if (commShipPic != NULL) {
#ifdef MACBUILD
        HPurge((Handle)commShipPic);
#endif
        QDReleaseResource(commShipPic);
        commShipPic = NULL;
    }

    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);

    if (releaseCommEscort) {
        DestroyPartialFleetCargo(commShip->index);
        commShip->parentShip = -1;
        commShip->aiType = shipClass[commShip->type].inherentAI;
        AIMakeShipLeave(commShip);
        AIDispatch(commShip, 1);
        statusRedraw[kRedrawCargo] = 1;
    }

    DisposeDialogNoFlash(commDialog);
    commShip = NULL;
    commDialog = NULL;
    gSpeedMult = 0;
    if (commBackgroundPic != NULL) {
        QDReleaseResource(commBackgroundPic);
        commBackgroundPic = NULL;
    }
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    RefreshSAT();
#endif
}

/* EscortFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00485840
// MAC: _EscortFilter i386 0x000950e9 ppc 0x000a0d7c
Boolean EscortFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
#if V111
    short key;
#else
    EventChar c;
    EventChar key;
#endif
    int what;

    what = ev->what;
    if (what == kKeyDown) {
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
            *itemHit = 1;
            return 1;
        }
        if (key == kUKeyCode) {
            *itemHit = 3;
            return 1;
        }
        if (key == kSKeyCode) {
            *itemHit = 4;
            return 1;
        }
        if (key == kRKeyCode) {
            *itemHit = 2;
            return 1;
        }
        if (key == kCKeyCode) {
            *itemHit = 1;
            return 1;
        }
#else
        c = EventCharOf(ev);
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        key = tolower(c);
        if (key == 'u') {
            *itemHit = 3;
            return 1;
        }
        if (key == 's') {
            *itemHit = 4;
            return 1;
        }
        if (key == 'r') {
            *itemHit = 2;
            return 1;
        }
        if (key == 'c') {
            *itemHit = 1;
            return 1;
        }
#endif
    }
    if (what == kMouseDown) {
        where = ev->where;
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        EscortDialogUpdate();
        button = TrackEscortButtons(where);
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
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(commDialog));
        EscortDialogUpdate();
        EndUpdate(GetDialogWindow(commDialog));
#else
        EscortDialogUpdate();
#endif
    }
    return 0;
}

/* EscortDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00485970
// MAC: _EscortDialogUpdate i386 0x00094a1b ppc 0x000a06c0
void EscortDialogUpdate(void)
{
    unsigned char text[256];
    short itemType;
    Handle item;
    Rect box;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(commDialog));
    if (commBackgroundPic != NULL)
        DrawPicHandle(commBackgroundPic, GetDialogPortRect(commDialog));
    DrawEscortButtons(-1);
    GetDialogItem(commDialog, 11, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(commDialog)))) {
        ForeColor(blackColor);
        DrawPicHandle(commShipPic, &box);
    }
    GetDialogItem(commDialog, 12, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(commDialog)))) {
        ForeColor(blackColor);
        PaintRect(&box);
        RGBForeColor(&rgbMedGrey);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(box.left, (box.top + 12));
        if (commShip->hired != 0)
            DrawIndString(kGameStrList, 0xa6);
        else
            DrawIndString(kGameStrList, 0xa7);
        DrawString("\p:");
        ForeColor(whiteColor);
        MoveTo((box.left + 3), (box.top + 0x1b));
        DrawString(shipClassStr);
        pstrcpy(text, shipClass[commShip->type].subtitle);
        if (text[0] != 0) {
            MoveTo((box.left + 3), (box.top + 0x29));
            DrawString(text);
        }
        ForeColor(blackColor);
    }
    GetDialogItem(commDialog, 10, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(commDialog)))) {
        ForeColor(blackColor);
        PaintRect(&box);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(box.left, (box.top + 12));
        if (canUpgradeEscort) {
            if (commShip->upgradeOnLanding != 0) {
                RGBForeColor(&rgbLightGrey);
                DrawIndString(kGameStrList, 0x124);
            } else {
                RGBForeColor(&rgbMedGrey);
                DrawIndString(kGameStrList, 0x125);
                DrawString("\p ");
                MoveTo((box.left + 0x46), (box.top + 12));
                ForeColor(whiteColor);
                DrawCommaNumber(shipClass[commShip->type].escUpgradeCost);
                DrawString("\p ");
                if (shipClass[commShip->type].escUpgradeCost == 1)
                    DrawString(creditString);
                else
                    DrawString(creditsString);
            }
        } else {
            RGBForeColor(&rgbLightGrey);
            DrawIndString(kGameStrList, 0x126);
        }
        if (canSellEscort) {
            MoveTo(box.left, (box.top + 0x1c));
            if (commShip->sellOnLanding != 0) {
                RGBForeColor(&rgbLightGrey);
                DrawIndString(kGameStrList, 0x127);
            } else {
                RGBForeColor(&rgbMedGrey);
                DrawIndString(kGameStrList, 0x128);
                DrawString("\p ");
                MoveTo((box.left + 0x46), (box.top + 0x1c));
                ForeColor(whiteColor);
                DrawCommaNumber(shipClass[commShip->type].tradeIn);
                DrawString("\p ");
                if (shipClass[commShip->type].tradeIn == 1)
                    DrawString(creditString);
                else
                    DrawString(creditsString);
            }
        }
        if (commShip->hired != 0) {
            MoveTo(box.left, (box.top + 0x2a));
            RGBForeColor(&rgbMedGrey);
            DrawIndString(kGameStrList, 0x129);
            DrawString("\p ");
            MoveTo((box.left + 0x1e), (box.top + 0x2a));
            ForeColor(whiteColor);
            DrawCommaNumber((shipClass[commShip->type].cost * 0.01));
            DrawString("\p ");
            if (shipClass[commShip->type].cost * 0.01 == 1.0)
                DrawString(creditString);
            else
                DrawString(creditsString);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x10b);
        }
    }
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(commDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(commDialog)),
             GetDialogPortRect(commDialog), GetDialogPortRect(commDialog), 0,
             GetWindowVisRgn(FrontWindow(commDialog)));
}

/*
 * The four functions below are not escort-dialog code: their only caller is
 * EnterGameFromMainScreen (RT3 registration-library scrambling, same family
 * as sources/engine/parser.c 0x0044aa10-0x0044aa50 and
 * sources/engine/ai.c 0x00416090-0x004160d0). No mac symbol exists for
 * any of the four.
 */

#ifndef MACBUILD
/* RegScramA3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004860b0
Boolean RegScramA3(unsigned int x)
{
    return (IsPlatformOpen() * x);
}

/* RegScramB3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004860c0
void RegScramB3(unsigned char x)
{
    gRT3Table3[x]++;
}

/* RegScramC3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004860d0
short RegScramC3(unsigned char x)
{
    int n = x;
    return ((n >> (8 - n)) | (n << n)) & 0xff;
}

/* RegScramF3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004860f0
void *RegScramF3(void)
{
    char buf[4];
    return buf;
}
#endif
