// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
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

#define kGovtNoRefuel 0x0001
#define kCommButtons  3

DialogPtr commDialog;
DialogPtr haggleDialog;
DialogPtr plunderDialog;
unsigned char DAT_007d17c4[2];
short haggleMode;
short responseRandom;
short adviceRandom;
short adviceDisasterRandom;
unsigned char DAT_007d17ce[2];
short salvageType;
short salvageQty;
unsigned short salvageAmmoType;
short salvageAmmoQty;
short salvageFuelQty;
short captureOdds;
short bribeRandom;
long salvageCredits;
long gBribeAsked;
long haggleVal;
float gCommMood;
char hatesPlayer;
char DAT_007d17f1;
char willTakeBribe;
char hasDemandedTribute;
char commShipXeno;
char releaseCommEscort;
char canUpgradeEscort;
char canSellEscort;
PicHandle commShipPic;
PicHandle commBackgroundPic;
PicHandle haggleDialogPict;
PicHandle plunderDialogPict;
ShipObj *commShip;
unsigned char commResponseString[256];
unsigned char adviceString[256];
unsigned char gCommGovtName[256];
unsigned char shipClassStr[256];
unsigned char DAT_007d1c0c[256];
unsigned char shipGovtStr[256];
short commDialogMode;
unsigned char DAT_007d1e0e[2];
unsigned char gRT3Table3[256];

#define kCommDlgID2      0x3ef
#define kCommPictID2     0x213f

/* the fields this dialog reads of the 0x8e6-byte mission record */

/* the fields this dialog reads of the 0x120-byte rank record */

/* DoCommDialog (mac _DoCommDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e470
void DoCommDialog(short shipIndex)
{
    short i;
    short itemHit;
    char hasMission;
    char noBribe;
    char cantTalk;
    char canAskEscort;
    char isMissionShip;
    char canAskHelp;
    char done;
    char missionPending;
    unsigned char buf[256];
    short n;
    short r;
    long amount;
    done = 0;
    if (screenFaded)
        return;
    if (shipIndex < 0 || shipIndex >= 64)
        return;
    if (!shipData[shipIndex].active)
        return;
    commShip = &shipData[shipIndex];
    if (commShip->aiType == 6 && commShip->parentShip == 0
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
        if (commShip->aiType <= 2
            && (govtData[commShip->govt].flags & 0x2000) != 0)
            willTakeBribe = 1;
        if (commShip->aiType >= 3
            && (govtData[commShip->govt].flags & 0x200) != 0)
            willTakeBribe = 1;
        if ((govtData[commShip->govt].flags2 & 1) != 0)
            isMissionShip = 1;
        if ((govtData[commShip->govt].flags2 & 8) != 0)
            cantTalk = 1;
        if (gShipTypes[commShip->type].inherentGovtB != -1
            && (govtData[gShipTypes[commShip->type].inherentGovtB].flags2 & 8) != 0)
            cantTalk = 1;
        if (commShip->aiType >= 3
            && (govtData[commShip->govt].flags & 0x1000) != 0)
            noBribe = 1;
    }
    responseRandom = Rand(5);
    gCommMood = (Rand(41) + 80) * 0.01;
    if (Rand(5) == 0)
        gCommMood -= 0.5;
    else if (Rand(5) == 0)
        gCommMood += 0.5;
    n = (shipData->credits * 0.0000005);
    if (n <= 0)
        n = 1;
    gBribeAsked = ((Rand(n) * 1000 + 3000) * gCommMood);
    if (commShip->govt != -1
        && (govtData[commShip->govt].flags & 0x8000) != 0) {
        n = (shipData->credits * 0.0001);
        if (n <= 0)
            n = 1;
        gBribeAsked = ((Rand(n) * 1000 + 10000) * gCommMood);
    }
    if (gBribeAsked > shipData->credits * 0.333)
        gBribeAsked = (shipData->credits * 0.333);
    gBribeAsked /= 1000;
    gBribeAsked *= 1000;
    if (gBribeAsked > 20000)
        gBribeAsked = 20000;
    if (gBribeAsked < 1000)
        gBribeAsked = 1000;
    commShipXeno = 0;
    if (commShip->govt != -1
        && (govtData[commShip->govt].flags & 1) != 0)
        commShipXeno = 1;
    n = gShipTypes[commShip->type].shipyardPictID;
    if (commShip->persType != -1
        && *(short *)((char *)&personData[commShip->persType] + 0x12) >= 128)
        n = *(short *)((char *)&personData[commShip->persType] + 0x12);
    commShipPic = GetPicture(n);
    if (commShip->govt != -1)
        pstrcpy(shipGovtStr, govtShortNames[commShip->govt]);
    else
        pstrcpy(shipGovtStr, "\p");
    pstrcpy(shipClassStr, gShipCommNames[commShip->type]);
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    ShowTheCursor();
    hasMission = 0;
    if (commShip->specialMission != -1
        && currentMissionObjectives[commShip->specialMission].active != 0
        && currentMissions[commShip->specialMission].shipBehav == 1)
        hasMission = 1;
    LoadAdvice();
    LoadResponse(0);
    if (commShip->specialMission != -1 && !IsDisabled(commShip) && !isMissionShip) {
        if (currentMissionObjectives[commShip->specialMission].active != 0) {
            n = currentMissions[commShip->specialMission].shipGoal;
            if (n == 0)
                LoadResponse(2);
            else if (n == 3) {
                LoadResponse(8);
                PtoCstr(commResponseString);
                pstrcpy(buf, shipName);
                PtoCstr(buf);
                strcat((char *)commResponseString, (char *)buf);
                strcat((char *)commResponseString, ".");
                CtoPstr((char *)commResponseString);
            }
            if (currentMissions[commShip->specialMission].shipBehav == 0)
                LoadResponse(2);
            if (currentMissions[commShip->specialMission].shipBehav == 1)
                LoadResponse(4);
        }
    } else if (!IsDisabled(commShip) && !isMissionShip) {
        if (ExtendedIsThreatToPlayer(commShip))
            LoadResponse(2);
        else if (commShip->parentShip == 0) {
            if (commShip->aiType == 5)
                LoadResponse(5);
            if (commShip->aiType == 6)
                LoadResponse(4);
        } else if (AIDoesShipLikePlayer(commShip))
            LoadResponse(0);
        else
            LoadResponse(2);
    }
    commDialog = NULL;
    commDialog = GetNewDialog(kCommDlgID2, CommDialogUpdate, CommDialogEvent);
    if (commDialog == NULL)
        return;
    canAskEscort = 0;
    canAskHelp = 0;
    for (i = 0; i < 128; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0
            && GovtAllies(commShip->govt, rankData[i].govt)) {
            if ((rankData[i].flags & 0x400) != 0)
                canAskEscort = 1;
            if ((rankData[i].flags & 0x800) != 0)
                canAskHelp = 1;
        }
    }
    if (commShip->govt != -1
        && (govtData[commShip->govt].flags2 & 0x10) != 0)
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
    FrontWindow(commDialog);
    FrontWindow(commDialog);

    do {
        if (gQuit)
            done = 1;
        RunModalDialog(commDialog, CommDialogEvent, &itemHit, 0);
        if (itemHit == 1) {
            DrawCommDialogButtons(-1);
            done = 1;
        }
        if (itemHit == 2) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            if (!IsDisabled(commShip)) {
                if (commShip->parentShip == 0 && commShip->aiType == 6
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
                            && currentMissions[commShip->specialMission].shipBehav == 0)
                            missionPending = 1;
                        if (!missionPending) {
                            if (commShip->defenseStellar != -1)
                                LoadResponse(0x13);
                            else if (commShip->persType == kPersBountyHunter)
                                LoadResponse(0x13);
                            else if (willTakeBribe) {
                                if (gCommMood < 0.8)
                                    LoadResponse(0x17);
                                else if (gCommMood > 1.2)
                                    LoadResponse(0x18);
                                else
                                    LoadResponse(0x12);
                                CommDialogUpdate();
                                r = OfferBribe();
                                SetDialogGWorld(commDialog);
                                if (r == 1) {
                                    LoadResponse(0x14);
                                    AIMakeShipLeave(commShip);
                                    commShip->aiType = 1;
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
                            else if (commShip->aiType <= 4)
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
                                else if (commShip->aiType == 1)
                                    LoadResponse(0x11);
                                else if (commShip->aiType == 2) {
                                    if (commShip->index % 3 == 0
                                        && commShip->govt == -1) {
                                        if (gCommMood < 0.8)
                                            LoadResponse(0x17);
                                        else if (gCommMood > 1.2)
                                            LoadResponse(0x18);
                                        else
                                            LoadResponse(0x12);
                                        CommDialogUpdate();
                                        r = OfferBribe();
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
                                } else if (commShip->aiType == 3
                                           || commShip->aiType == 4) {
                                    if (gCommMood < 0.8)
                                        LoadResponse(0x17);
                                    else if (gCommMood > 1.2)
                                        LoadResponse(0x18);
                                    else
                                        LoadResponse(0x12);
                                    CommDialogUpdate();
                                    r = OfferBribe();
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
                            if (!(shipData->fuel < 100 && ShipFuelCapacity(shipData) > 0)
                                && !IsDisabled(shipData)) {
                                LoadResponse(0xe);
                            } else if (commShipXeno)
                                LoadResponse(0x13);
                            else if (commShip->aiType <= 4) {
                                if (gCommMood < 0.8)
                                    LoadResponse(0x17);
                                else if (gCommMood > 1.2)
                                    LoadResponse(0x18);
                                else
                                    LoadResponse(0x1c);
                                CommDialogUpdate();
                                r = 0;
                                if (canAskHelp)
                                    r++;
                                if (r != 1) {
                                    r = OfferBribe();
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
        QDReleaseResource(commShipPic);
        commShipPic = NULL;
    }
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
    if (releaseCommEscort) {
        DestroyPartialFleetCargo(commShip->index);
        commShip->parentShip = -1;
        commShip->aiType = gShipTypes[commShip->type].inherentAI;
        commShip->boarded = 1;
        commShip->formerEscort = -1;
        AISetup(commShip);
        AIMakeShipLeave(commShip);
        statusRedraw[2] = 1;
    }
    GetDialogPortRect(commDialog);
    GetDialogPortRect(commDialog);
    GetDialogPortRect(commDialog);
    GetDialogPortRect(commDialog);
    DisposeDialogNoFlash(commDialog);
    commDialog = NULL;
    gTimeScale = 0;
    commShip->hailed = 1;
    commShip = NULL;
    SWSetPortToWindow(gSpriteWorld);
}

/*
 * A no raises the asking price by a thousand for next time, and -1 means the
 * player could not have paid it anyway.
 */
/* OfferBribe. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047f9c0
int OfferBribe(void)
{
    short answer;

    CommDialogUpdate();
    haggleVal = gBribeAsked;
    answer = DoHaggleDialog(0, 0x23);
    SetDialogGWorld(commDialog);
    if (shipData->credits >= haggleVal) {
        if (answer == 1) {
            shipData->credits -= haggleVal;
            statusRedraw[2] = 1;
            return 1;
        }
        gBribeAsked += 1000;
        return 0;
    }
    return -1;
}

/* CommDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047fa40
char CommDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int key;
    short govt;
    char noRefuel;
    int what;

    what = ev->what;
    if (what == kKeyDown) {
        key = ev->message & kCharCodeMask;
        key = tolower(key);
        if (key == kReturnChar || key == 'e' || key == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        noRefuel = 0;
        if (commShip != NULL) {
            govt = commShip->govt;
            if (govt != -1) {
                if ((govtData[govt].flags2 & kGovtNoRefuel) != 0)
                    noRefuel = 1;
            }
        }
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
    }
    if (what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        CommDialogUpdate();
        button = TrackCommDialogButtons(where);
        if (button >= 0 && button <= kCommButtons) {
            *itemHit = button + 1;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt)
        CommDialogUpdate();
    return 0;
}

/* CommDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047fb70
void CommDialogUpdate(void)
{
    void *item;
    unsigned char text[256];
    short itemType;
    Rect textBox;
    Rect box;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(commDialog));
    if (commBackgroundPic != NULL)
        DrawPicHandle(commBackgroundPic, GetDialogPortRect(commDialog));
    DrawCommDialogButtons(-1);
    GetDialogItem(commDialog, 11, &itemType, &item, &box);
    if (commShipPic != NULL) {
        RGBForeColor(blackColor);
        DrawPicHandle(commShipPic, &box);
    }
    GetDialogItem(commDialog, 12, &itemType, &item, &box);
    RGBForeColor(blackColor);
    PaintRect(&box);
    RGBForeColor(&rgbMedGrey);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    MoveTo(box.left, (box.top + 12));
    DrawIndString(kGameStrList, 0xc3);
    MoveTo((box.left + 0x23), (box.top + 12));
    RGBForeColor(whiteColor);
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
        RGBForeColor(redColor);
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
    RGBForeColor(blackColor);
    GetDialogItem(commDialog, 10, &itemType, &item, &box);
    pstrcpy(text, commResponseString);
    PtoCstr(text);
    InsetRect(&box, 1, 1);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(whiteColor);
    PaintRect(&box);
    textBox.top = box.top;
    textBox.left = box.left;
    textBox.bottom = box.bottom;
    textBox.right = box.right;
    InsetRect(&textBox, 4, 2);
    RGBForeColor(blackColor);
    strcpy(briefingTextPtr, (char *)text);
    MungeBriefing(0, -1);
    TETextBox(briefingTextPtr, strlen(briefingTextPtr), &textBox, 0);
    InvertRect(&box);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(commDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(commDialog)),
             GetDialogPortRect(commDialog), GetDialogPortRect(commDialog), 0, NULL);
}

/*
 * Which key reaches button 2 depends on whether the target hates the player;
 * button 3 is demand-tribute while the stellar is undominated and release once
 * it is, the release being refused outright for stellars carrying the second
 * flags2 bit.
 */

#define kSpobNoRelease 0x0020          /* flags2; name TBC */
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
void DoPlanetCommDialog(short stellar)
{
    short itemHit;
    char done;
    unsigned char pname[256];
    char text[254];
    short i;
    short n;

    done = 0;
    if (screenFaded)
        return;
    if (shipData->targetStellar == -1)
        return;
    if ((stellarData[shipData->targetStellar].flags & 0x20) != 0) {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        return;
    }
    responseRandom = Rand(5);
    adviceRandom = adviceDisasterRandom = -1;
    commShip = NULL;
    n = (shipData->credits * 0.000001);
    if (n <= 0)
        n = 1;
    gBribeAsked = Rand(n) * 1000 + 3000;
    if (stellarData[shipData->targetStellar].govt != -1
        && (govtData[stellarData[shipData->targetStellar].govt].flags & 0x8000) != 0)
        gBribeAsked *= 1.5;
    if (gBribeAsked > shipData->credits * 0.333)
        gBribeAsked = (shipData->credits * 0.333);
    gBribeAsked /= 1000;
    gBribeAsked *= 1000;
    if (gBribeAsked > 900000)
        gBribeAsked = 900000;
    if (gBribeAsked < 1000)
        gBribeAsked = 1000;
    hasDemandedTribute = 0;
    if (!LoadPluginString(stellarData[shipData->targetStellar].type + 7000, gCommGovtName))
        GetIndString(gCommGovtName, kGovtNameList, stellarData[shipData->targetStellar].type + 1);
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    ShowTheCursor();
    commDialog = NULL;
    commDialog = GetNewDialog(kCommDlogID, UpdateStellarCommDialog,
                              StellarCommDialogEvent);
    if (commDialog == NULL)
        return;
    if (stellarData[shipData->targetStellar].minStatus == 0x7fff)
        hatesPlayer = 1;
    else if (playerLegalStatus[shipData->curSystem] >= stellarData[shipData->targetStellar].minStatus
             || stellarData[shipData->targetStellar].minStatus == (short)0x8001)
        hatesPlayer = 0;
    else
        hatesPlayer = 1;
    if (hatesPlayer && stellarData[shipData->targetStellar].govt != -1) {
        if (PlayerBlessedByGovt(stellarData[shipData->targetStellar].govt, 1))
            hatesPlayer = 0;
        else if (*((char *)&govtData[stellarData[shipData->targetStellar].govt] + 0x83) != 0)
            hatesPlayer = 0;
    }
    willTakeBribe = 0;
    if (bribeRandom < 0)
        bribeRandom = Rand(100);
    if (bribeRandom > 30) {
        if (stellarData[shipData->targetStellar].govt == -1)
            willTakeBribe = 1;
        else if ((govtData[stellarData[shipData->targetStellar].govt].flags & 0x4000) != 0)
            willTakeBribe = 1;
    }
    if (stellarData[shipData->targetStellar].govt != -1
        && (govtData[stellarData[shipData->targetStellar].govt].flags & 0x8000) != 0)
        willTakeBribe = 1;
    if ((stellarData[shipData->targetStellar].flags & 0x20) != 0)
        LoadResponse(1);
    else if (hatesPlayer && stellarData[shipData->targetStellar].dominated == 0)
        LoadResponse(2);
    else {
        GetHaggleReply(0);
        strncpy(text, (char *)commResponseString, 254);
        PtoCstr((unsigned char *)text);
        pstrcpy(pname, stellarData[shipData->targetStellar].name);
        PtoCstr(pname);
        strcat(text, (char *)pname);
        strcat(text, ".");
        CtoPstr(text);
        strncpy((char *)commResponseString, text, 254);
    }
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
    UpdateStellarCommDialog();
    FrontWindow(commDialog);
    FrontWindow(commDialog);

    do {
        if (gQuit)
            done = 1;
        RunModalDialog(commDialog, StellarCommDialogEvent, &itemHit, 0);
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
                    if (willTakeBribe == 0) {
                        GetHaggleReply(9);
                        UpdateStellarCommDialog();
                    } else {
                        GetHaggleReply(8);
                        UpdateStellarCommDialog();
                        haggleVal = gBribeAsked;
                        i = DoHaggleDialog(0, 0x23);
                        PlaySound(beepSnd[1], 1, gSoundVolume,
                                  gSoundVolume);
                        if (shipData->credits >= haggleVal) {
                        if (i == 1) {
                            dockPermission = 0x2ef;
                            dockTarget = shipData->targetStellar;
                            shipData->aiTimer = 0;
                            pstrcpy((unsigned char *)text, shipName);
                            PtoCstr((unsigned char *)text);
                            strcat(text, ", ");
                            if ((stellarData[shipData->targetStellar].flags
                                 & 0x10) != 0)
                                AppendIndString(text, kGameStrList, 0x5f);
                            else
                                AppendIndString(text, kGameStrList, 0x62);
                            strcat(text, " ");
                            AppendIndString(text, kGameStrList, 0x64);
                            CtoPstr(text);
                            DisplayComm((unsigned char *)text, 0xfa, 0, 12,
                                        &rgbWhite, 0, 0);
                            shipData->credits -= haggleVal;
                            statusRedraw[2] = 1;
                            willTakeBribe = 0;
                            done = 1;
                        } else {
                            willTakeBribe = 0;
                            bribeRandom = 0;
                            gBribeAsked += 1000;
                            GetHaggleReply(6);
                            UpdateStellarCommDialog();
                        }
                        } else {
                            GetHaggleReply(4);
                            UpdateStellarCommDialog();
                        }
                        RedrawDialog(commDialog);
                    }
                } else if (!hatesPlayer) {
                    LoadResponse(9);
                    RedrawDialog(commDialog);
                } else {
                    GetHaggleReply(4);
                    RedrawDialog(commDialog);
                }
            }
            if (itemHit == 3) {
                            if ((stellarData[shipData->targetStellar].flags2 & 0x20) == 0) {
                    if (stellarData[shipData->targetStellar].dominated != 0) {
                        if ((stellarData[shipData->targetStellar].flags & 0x10) != 0)
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
                        if (playerLegalStatus[shipData->curSystem]
                                >= stellarData[shipData->targetStellar].minStatus
                            && stellarData[shipData->targetStellar].minStatus > (short)0x8001
                            && stellarData[shipData->targetStellar].minStatus != 0x7fff)
                            playerLegalStatus[shipData->curSystem] =
                                stellarData[shipData->targetStellar].minStatus - 1;
                        crashlog_mission_bit_set(
                            "releasing stellar",
                            (char *)stellarData[shipData->targetStellar].name, 1, shipData->targetStellar + 128,
                            stellarData[shipData->targetStellar].onRelease);
                        EvalMissionBitSetString(stellarData[shipData->targetStellar].onRelease);
                        RedrawDialog(commDialog);
                    } else {
                        hatesPlayer = 1;
                        if (playerLegalStatus[shipData->curSystem]
                                >= stellarData[shipData->targetStellar].minStatus
                            && stellarData[shipData->targetStellar].minStatus > (short)0x8001
                            && stellarData[shipData->targetStellar].minStatus != 0x7fff)
                            playerLegalStatus[shipData->curSystem] =
                                stellarData[shipData->targetStellar].minStatus - 1;
                        SlapWithPenalty(shipData->curSystem,
                                     stellarData[shipData->targetStellar].govt, 3, -1);
                        if (playerKills < 0x3200 && !cheatsActive) {
                            SetDialogGWorld(commDialog);
                            GetHaggleReply(1);
                            RedrawDialog(commDialog);
                        } else if (hasDemandedTribute
                                   || stellarData[shipData->targetStellar].defWaveSize > 0
                                   || stellarData[shipData->targetStellar].defenseActive != 0) {
                            SetDialogGWorld(commDialog);
                            GetHaggleReply(2);
                            RedrawDialog(commDialog);
                            if (stellarData[shipData->targetStellar].defenseActive == 0) {
                                for (i = 0; i <= 4; i++)
                                    SlapWithPenalty(shipData->curSystem,
                                                 stellarData[shipData->targetStellar].govt, 3, -1);
                                if (stellarData[shipData->targetStellar].defCount > 1000) {
                                    stellarData[shipData->targetStellar].defWaveSize -=
                                        stellarData[shipData->targetStellar].defCount % 10;
                                    n = stellarData[shipData->targetStellar].defCount % 10;
                                    if (stellarData[shipData->targetStellar].defWaveSize < 0) {
                                        n += stellarData[shipData->targetStellar].defWaveSize;
                                        stellarData[shipData->targetStellar].defWaveSize = 0;
                                    }
                                } else {
                                    n = stellarData[shipData->targetStellar].defWaveSize;
                                    stellarData[shipData->targetStellar].defWaveSize = 0;
                                }
                                for (i = 0; i < n; i++)
                                    SpawnStellarDefenseShip(shipData->targetStellar);
                            }
                        } else {
                            for (i = 0; i <= 4; i++)
                                SlapWithPenalty(shipData->curSystem,
                                             stellarData[shipData->targetStellar].govt, 3, -1);
                            if ((stellarData[shipData->targetStellar].flags & 0x10) != 0)
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
                                (char *)stellarData[shipData->targetStellar].name, 1, shipData->targetStellar + 128,
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
    GetDialogPortRect(commDialog);
    GetDialogPortRect(commDialog);
    GetDialogPortRect(commDialog);
    GetDialogPortRect(commDialog);
    DisposeDialogNoFlash(commDialog);
    commDialog = NULL;
    gTimeScale = 0;
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    RedrawStatus(1);
    RedrawComm(0);
}

/* StellarCommDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00481140
char StellarCommDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int key;
    int what;

    what = ev->what;
    if (what == kKeyDown) {
        key = tolower(ev->message & kCharCodeMask);
        if (key == kReturnChar || key == 'e' || key == kEscapeChar) {
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
            if (stellarData[shipData->targetStellar].dominated == 0) {
                *itemHit = 3;
                return 1;
            }
        }
        if (key == 'r') {
            if (stellarData[shipData->targetStellar].dominated != 0 &&
                (stellarData[shipData->targetStellar].flags2 & kSpobNoRelease) == 0) {
                *itemHit = 3;
                return 1;
            }
        }
    }
    if (what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdateStellarCommDialog();
        button = TrackPlanetCommDialogButtons(where);
        if (button >= 0 && button <= kStellarCommButtons) {
            *itemHit = button + 1;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt)
        UpdateStellarCommDialog();
    return 0;
}

/* UpdateStellarCommDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004812c0
void UpdateStellarCommDialog(void)
{
    void *item;
    short itemType;
    RGBColor rgbGold;
    unsigned char text[256];
    Rect textBox;
    Rect box;
    short i;
    short spriteWidth;
    short spriteHeight;

    rgbGold.red = 0xffff;
    rgbGold.green = 0x6666;
    rgbGold.blue = 0;
    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(commDialog));
    if (commBackgroundPic != NULL)
        DrawPicHandle(commBackgroundPic, GetDialogPortRect(commDialog));
    DrawPlanetCommDialogButtons(-1);
    GetDialogItem(commDialog, 4, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(commDialog)))) {
        strncpy((char *)text, (char *)commResponseString, 0xfe);
        PtoCstr(text);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(whiteColor);
        InsetRect(&box, 1, 1);
        PaintRect(&box);
        textBox.top = box.top;
        textBox.left = box.left;
        textBox.bottom = box.bottom;
        textBox.right = box.right;
        InsetRect(&textBox, 4, 2);
        RGBForeColor(blackColor);
        TETextBox(text, strlen((char *)text), &textBox, 0);
        InvertRect(&box);
    }
    GetDialogItem(commDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(commDialog)))) {
        RGBForeColor(blackColor);
        for (i = 0; i < kStellarSprites; i++) {
            if (stellarSprite[i]->isVisible != 0 &&
                shipData->targetStellar == stellarSprite[i]->userData) {
                spriteWidth = GetSpriteXSize(stellarSprite[i]);
                spriteHeight = GetSpriteYSize(stellarSprite[i]);
                break;
            }
        }
        textBox.right = (box.left + box.right) / 2;
        textBox.left = textBox.right;
        textBox.bottom = (box.top + box.bottom) / 2;
        textBox.top = textBox.bottom;
        InsetRect(&textBox, -spriteWidth / 2, -spriteHeight / 2);
        CopyBits(GetGWCPixMap(&loadPort),
                 GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                 &textBox, &textBox, 0, NULL);
        BlitPixieRLEDrawProc(stellarSprite[i]->curFrameP, gSpriteWorld->workFrameP,
                             &stellarSprite[i]->curFrameP->frameRect, &textBox);
        CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                     gSpriteWorld->workFrameP->framePort),
                 (BitMapPtr)GetGWCPixMap(&loadPort),
                 &textBox, &textBox, 0, NULL);
    }
    GetDialogItem(commDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(commDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(box.left, (box.top + 12));
        RGBForeColor(whiteColor);
        DrawString(stellarData[shipData->targetStellar].name);
        MoveTo(box.left, (box.top + 0x1a));
        RGBForeColor(&rgbMedGrey);
        DrawString(gCommGovtName);
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
            if (stellarData[shipData->targetStellar].flags2 & kSpobNoRelease)
                DrawIndString(kGameStrList, 0xab);
            else
                DrawIndString(kGameStrList, 0xac);
        } else if (hatesPlayer != 0) {
            MoveTo(box.left, (box.top + 0x2a));
            RGBForeColor(&rgbMedGrey);
            DrawIndString(kGameStrList, 0xc4);
            DrawString("\p  ");
            TextFace(1);
            if (playerLegalStatus[shipData->curSystem] >= 0) {
                RGBForeColor(&rgbGold);
                DrawIndString(kGameStrList, 0xad);
            } else {
                RGBForeColor(redColor);
                DrawIndString(kGameStrList, 0xae);
            }
            TextFace(0);
        }
    }
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(commDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(commDialog)),
             GetDialogPortRect(commDialog), GetDialogPortRect(commDialog), 0,
             GetCWindowVisRgn(FrontWindow(commDialog)));
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

#define kDudeHasAdvice0     0x1000
#define kDudeHasAdvice1     0x2000
#define kDudeHasAdvice2     0x4000
#define kDudeHasAdvice3     0x8000
#define kDudeHasAdviceAny   (kDudeHasAdvice0 | kDudeHasAdvice1 | \
                             kDudeHasAdvice2 | kDudeHasAdvice3)

#define kPersHasAdvice      0x8000

#define kSpobLandOrTrade    0x00000003UL  /* kSpobLandOrTrade | kSpobTradeCenter */

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


/* BurgerLib's SwapEndian written out; the STR# entry count is big-endian because the
   resource is Mac-authored */
static __inline short SwapEndianShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

/* LoadAdvice (mac _LoadAdvice). STATE: MATCH 100.0%, exe 598 / obj 598, relocregcheck 0 of
   162; the stellar pick reuses `count' once the count is spent. OPEN: none */
// FUNCTION: EVNOVA 0x004819d0
void LoadAdvice(void)
{
    unsigned char text[256];
    short cargo;
    short which;
    short found;
    short i;
    short count;
    short newsCount;
    short strCount;
    short demand;
    short d;
    Handle h;

    GetIndString(adviceString, kGameStrList, kAdviceNone);
    which = -1;
    if (commShip->dude != -1
        && (dudeData[commShip->dude].adviceFlags & kDudeHasAdviceAny) != 0) {
        for (;;) {
            which = Rand(4);
            if (which == 0
                && (dudeData[commShip->dude].adviceFlags & kDudeHasAdvice0) != 0)
                break;
            else if (which == 1
                     && (dudeData[commShip->dude].adviceFlags & kDudeHasAdvice1) != 0)
                break;
            else if (which == 2
                     && (dudeData[commShip->dude].adviceFlags & kDudeHasAdvice2) != 0)
                break;
            else if (which == 3
                     && (dudeData[commShip->dude].adviceFlags & kDudeHasAdvice3) != 0)
                break;
        }
    }
    if (commShip->persType != -1
        && (personData[commShip->persType].flags & kPersHasAdvice) != 0
        && personData[commShip->persType].commQuote == -1)
        which = 1;

    if (which == 0) {
        count = 0;
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].loaded != 0
                && (stellarData[i].flags & kSpobLandOrTrade) != 0
                && (stellarData[i].flags & kSpobHidden) == 0
                && (stellarData[i].flags & kSpobDemandExtremes) != 0)
                count++;
        }
        if (count > 0) {
            do {
                cargo = Rand(kCargoTypes);
                count = Rand(kMaxStellars);
                if (stellarData[count].loaded == 0
                    || (stellarData[count].flags & kSpobLandOrTrade) == 0
                    || (stellarData[count].flags & kSpobHidden) != 0
                    || ((d = CalcPortDemand(cargo,
                                                 stellarData[count].flags))
                            != kDemandLow
                        && d != kDemandHigh))
                    count = -1;
            } while (count == -1);
            if (stellarData[count].loaded != 0
                && (stellarData[count].flags & kSpobLandOrTrade) != 0
                && (stellarData[count].flags & kSpobHidden) == 0) {
                demand = CalcPortDemand(cargo, stellarData[count].flags);
                pstrcpy(text, stellarData[count].name);
                PtoCstr(text);
                strcpy((char *)adviceString, (char *)text);
                strcat((char *)adviceString, " ");
                AppendIndString((char *)adviceString, kGameStrList, kAdviceWants);
                strcat((char *)adviceString, " ");
                if (demand == kDemandLow)
                    AppendIndString((char *)adviceString, kGameStrList,
                                    kAdviceCheap);
                else
                    AppendIndString((char *)adviceString, kGameStrList, kAdviceDear);
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
        newsCount = 0;
        for (i = 0; i < kNewsCount; i++) {
            if (disasterData[i].countdown > 1)
                newsCount++;
        }
        found = -1;
        if (newsCount == 1) {
            for (found = 0; found < kNewsCount; found++) {
                if (disasterData[found].countdown > 1)
                    break;
            }
        } else if (newsCount > 1) {
            do {
                found = Rand(kNewsCount);
            } while (disasterData[found].countdown <= 1);
        }
        if (found != -1) {
            if (disasterData[found].stellarID >= 0
                && disasterData[found].stellarID < kMaxStellars) {
                strcpy((char *)adviceString, "");
                AppendIndString((char *)adviceString, kGameStrList, kAdviceHeard);
                strcat((char *)adviceString, " ");
                if ((stellarData[disasterData[found].stellarID].flags & kSpobDestroyed)
                        != 0)
                    AppendIndString((char *)adviceString, kGameStrList,
                                    kAdviceDestroyed);
                else
                    AppendIndString((char *)adviceString, kGameStrList, kAdviceThe);
                strcat((char *)adviceString, " ");
                pstrcpy(text, stellarData[disasterData[found].stellarID].name);
                PtoCstr(text);
                strcat((char *)adviceString, (char *)text);
                strcat((char *)adviceString, ", ");
                AppendIndString((char *)adviceString, kGameStrList,
                                kAdviceRunningOut);
                strcat((char *)adviceString, " ");
                pstrcpy(text, lcCargoName[disasterData[found].commodity]);
                PtoCstr(text);
                strcat((char *)adviceString, (char *)text);
                strcat((char *)adviceString, " ");
                AppendIndString((char *)adviceString, kGameStrList,
                                (Rand(kAdviceVariants) + kAdviceSoon));
                strcat((char *)adviceString, " ");
                if (disasterData[found].amount < 0)
                    AppendIndString((char *)adviceString, kGameStrList,
                                    kAdviceRising);
                else
                    AppendIndString((char *)adviceString, kGameStrList,
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
            strCount = SwapEndianShort((const unsigned short *)*h);
            UnlockAHandle(h);
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
        if (commShip->aiType <= 2) {
            if (!LoadPluginString((commShip->govt * kGovtStride
                                          + kGovtPluginLow + responseRandom),
                                  adviceString))
                GetIndString(adviceString, kGovtStrings + commShip->govt,
                             responseRandom + kGovtAdviceLow);
        } else {
            if (!LoadPluginString((commShip->govt * kGovtStride
                                          + kGovtPluginHigh + responseRandom),
                                  adviceString))
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
                             adviceString))
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
short DoHaggleDialog(char raise, short chance)
{
    float scratch;
    long fscratch;
    short itemHit;
    char canHaggle;
    long conv;
    short result;
    char done;

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
    haggleDialog = GetNewDialog(kHaggleDlogID, HaggleDialogUpdate, HaggleDialogEvent);
    if (haggleDialog == NULL)
        return 0;

    haggleDialogPict = GetPicture(kHagglePictID);
    CenterWindow(FrontWindow(haggleDialog), gWind);
    ShowWindowNoFlash(FrontWindow(haggleDialog));
    SetDialogGWorld(haggleDialog);
    HaggleDialogUpdate();
    FrontWindow(haggleDialog);
    FrontWindow(haggleDialog);
    FlushEvents(everyEvent - diskMask, 0);

    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(haggleDialog, HaggleDialogEvent, &itemHit, 0);
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

    if (haggleDialogPict != NULL) {
        QDReleaseResource(haggleDialogPict);
        haggleDialogPict = NULL;
    }
    GetDialogPortRect(haggleDialog);
    GetDialogPortRect(haggleDialog);
    GetDialogPortRect(haggleDialog);
    GetDialogPortRect(haggleDialog);
    GetCWindowPortRect(gWind);
    GetCWindowPortRect(gWind);
    GetCWindowPortRect(gWind);
    GetCWindowPortRect(gWind);
    DisposeDialogNoFlash(haggleDialog);
    haggleDialog = NULL;
    return result;
}

/* HaggleDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004825d0
char HaggleDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;

    if ((int)ev->what == kKeyDown) {
        if ((ev->message & kCharCodeMask) == kReturnChar) {
            *itemHit = 1;
            return 1;
        }
        if ((ev->message & kCharCodeMask) == 'a') {
            *itemHit = 1;
            return 1;
        }
        if ((ev->message & kCharCodeMask) == 'l') {
            *itemHit = 2;
            return 1;
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
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
    if ((int)ev->what == kUpdateEvt)
        HaggleDialogUpdate();
    return 0;
}

/* HaggleDialogUpdate (mac _HaggleDialogUpdate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004826a0
void HaggleDialogUpdate(void)
{
    Rect box;
    short itemType;
    void *item;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(haggleDialog));
    if (haggleDialogPict != NULL)
        DrawPicHandle(haggleDialogPict, GetDialogPortRect(haggleDialog));
    GetDialogItem(haggleDialog, 3, &itemType, &item, &box);
    RGBForeColor(whiteColor);
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
    RGBForeColor(blackColor);
    DrawHaggleButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(haggleDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(haggleDialog)),
             GetDialogPortRect(haggleDialog), GetDialogPortRect(haggleDialog), 0,
             GetCWindowVisRgn(FrontWindow(haggleDialog)));
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
#define kHaggleGreetLow     0xbb8
#define kHaggleGreetHigh    0xbb9
#define kHaggleReplies      0xbba
#define kHaggleHighBias     -0xbd

/* LoadResponse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004828c0
void LoadResponse(short which)
{
    if (which <= kHaggleSplit)
        GetIndString(commResponseString, kHaggleGreetLow,
                     which * kHaggleStride + responseRandom + 1);
    else
        GetIndString(commResponseString, kHaggleGreetHigh,
                     which * kHaggleStride + responseRandom + kHaggleHighBias);
}

/* GetHaggleReply. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00482910
void GetHaggleReply(short which)
{
    GetIndString(commResponseString, kHaggleReplies,
                 which * kHaggleStride + responseRandom + 1);
}

/*
 * Note the fifth button reports 6, not 5, so the mapping is a chain of
 * explicit tests rather than an offset.
 */

#define kRegisterStrList 30000


#define kDudeCargoMask     0x003f    /* the six cargo-class bits */
#define kDudeUsesShipCost  0x0040    /* credits come from the hull, not the pers */
#define kOutfitModPlunder    0x19    /* modType that shifts the capture odds */

#define kMinShipCredits      1000
#define kMaxCaptureOdds        75
#define kOddsJitter            11
#define kOddsJitterMid          5
#define kEscortSlots         0x40
#define kWeaponTypes        0x100
#define kWarshipAI              3   /* inherentAI at or above this is a warship */

/* only the plunder-class word is read here; stride from the mac's dudeData */

#define kPlunderDlogID 0x3f3
#define kPlunderPictID 0x2143

/* DoPlunderDialog (mac _DoPlunderDialog). STATE: MATCH 100.0%, 1552/1552, byte-identical yes.
   OPEN: nothing. */
// FUNCTION: EVNOVA 0x00482940
void DoPlunderDialog(void)
{
    short pers;
    char c;
    short risk;
    long avail;
    short count;
    short itemHit;
    short i;
    long idx;
    short qty;
    short n;
    short j;
    short found;
    long idx2;
    short space;
    short govt;
    short type;
    char ok;
    char ok2;
    unsigned char *fullMsg;
    unsigned char *partialMsg;
    unsigned char *noneMsg;
    char risky;
    char done;
    unsigned char tmp[256];
    unsigned char text[256];

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
            if (gOutfits[i].itemClass == personData[pers].grantClass
                && HasMaxOfItem(i, 0, 0) == 0)
                count++;
        }
        if (count > 0) {
            do {
                n = Rand(kMaxOutfits);
                idx = n;
            } while (gOutfits[idx].itemClass != personData[pers].grantClass
                     || HasMaxOfItem(n, 0, 0) != 0);
            qty = ((Rand(51) + 50) * personData[pers].grantCount / 100.0);
            if (qty <= 0)
                qty = 1;
            ShipStatsToSystemInfo();
            avail = (short)FreeMass();
            while (qty > 0 && gOutfits[idx].mass * qty > avail)
                qty--;
            if (qty > 0) {
                for (j = 0; j < qty; j++)
                    GrantOutfitItem(n);
                SystemInfoToShipStats();
                strcpy((char *)text, "");
                AppendIndString((char *)text, kGameStrList, 0x6a);
                if (qty == 1) {
                    c = tolower(lcOutfitName[idx][1]);
                    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                        AppendIndString((char *)text, kGameStrList, 0x18a);
                    else
                        AppendIndString((char *)text, kGameStrList, 0x189);
                } else if (qty < 2 || qty > 10) {
                    NumToString(qty, tmp);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                } else {
                    AppendIndString((char *)text, 0x89, qty + 0x1c);
                }
                strcat((char *)text, " ");
                if (qty > 1)
                    pstrcpy(tmp, lcPluralOutfitName[idx]);
                else
                    pstrcpy(tmp, lcOutfitName[idx]);
                PtoCstr(tmp);
                strcat((char *)text, (char *)tmp);
                strcat((char *)text, " ");
                AppendIndString((char *)text, kGameStrList, 0x6c);
                CtoPstr((char *)text);
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
            }
        }
    }

    ShowTheCursor();
    plunderDialog = NULL;
    plunderDialog = GetNewDialog(kPlunderDlogID, PlunderDialogUpdate,
                                 PlunderDialogEvent);
    if (plunderDialog == NULL)
        return;
    plunderDialogPict = GetPicture(kPlunderPictID);
    CenterWindow(FrontWindow(plunderDialog), gWind);
    ShowWindowNoFlash(FrontWindow(plunderDialog));
    SetDialogGWorld(plunderDialog);
    PlunderDialogUpdate();
    FrontWindow(plunderDialog);
    FrontWindow(plunderDialog);
    FlushEvents(everyEvent - diskMask, 0);
    fullMsg = fuelString[1];
    partialMsg = fuelString[2];
    noneMsg = fuelString[3];
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(plunderDialog, PlunderDialogEvent, &itemHit, 0);
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
            DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
            SetDialogGWorld(plunderDialog);
            break;
        }
        risky = 0;
        if (itemHit == 2) {
            PlunderDialogUpdate();
            if (salvageType != -1) {
                if (salvageQty > TotalFleetHolds() - TotalCargo(shipData))
                    salvageQty = TotalFleetHolds() - TotalCargo(shipData);
                if (salvageQty > 0) {
                    strcpy((char *)text, "");
                    AppendIndString((char *)text, kGameStrList, 0x73);
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
                    AppendIndString((char *)text, kGameStrList, 0x187);
                    strcat((char *)text, " ");
                    pstrcpy(tmp, lcCargoName[salvageType]);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                    strcat((char *)text, " ");
                    AppendIndString((char *)text, kGameStrList, 0x6c);
                    CtoPstr((char *)text);
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                    shipData->cargo[salvageType] += salvageQty;
                    statusRedraw[2] = 1;
                    salvageQty = 0;
                    salvageType = -1;
                    SetDialogGWorld(plunderDialog);
                    RedrawDialog(plunderDialog);
                } else {
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    GetIndString(text, kGameStrList, 0x72);
                    DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
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
                AppendIndString((char *)text, kGameStrList, 0x74);
                strcat((char *)text, " ");
                strcat((char *)text, creditsCString);
                strcat((char *)text, " ");
                AppendIndString((char *)text, kGameStrList, 0x6c);
                CtoPstr((char *)text);
                DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                shipData->credits += salvageCredits;
                risk *= 1.25;
                risky = 1;
                statusRedraw[2] = 1;
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
            if (salvageAmmoType != (unsigned short)-1) {
                for (i = 0; i < kMaxOutfits; i++) {
                    if (gOutfits[i].modType[0] == kOutfModAmmo
                        && salvageAmmoType == (unsigned short)gOutfits[i].modVal[0]) {
                        found = i;
                        break;
                    }
                }
            }
            if (found != -1) {
                idx2 = found;
                qty = 0;
                while (FreeMass() >= gOutfits[idx2].mass
                       && qty < salvageAmmoQty
                       && HasMaxOfItem(found, 0, 0) == 0) {
                    shipData->weap[(short)salvageAmmoType].ammo++;
                    qty++;
                    ShipStatsToSystemInfo();
                }
                if (qty > 0) {
                    strcpy((char *)text, "");
                    AppendIndString((char *)text, kGameStrList, 0x73);
                    strcat((char *)text, " ");
                    StrcatOrdinalNumber(qty, (char *)text, 0);
                    strcat((char *)text, " ");
                    if (qty > 1)
                        pstrcpy(tmp, lcPluralOutfitName[idx2]);
                    else
                        pstrcpy(tmp, lcOutfitName[idx2]);
                    PtoCstr(tmp);
                    strcat((char *)text, (char *)tmp);
                    strcat((char *)text, " ");
                    AppendIndString((char *)text, kGameStrList, 0x6c);
                    CtoPstr((char *)text);
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                } else {
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    GetIndString(text, kGameStrList, 0x75);
                    DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                }
                statusRedraw[3] = 1;
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
                    shipData->fuel = space + shipData->fuel;
                    if (ShipFuelCapacity(shipData) <= shipData->fuel)
                        DisplayComm(fullMsg, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                    else
                        DisplayComm(partialMsg, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                } else {
                    DisplayComm(noneMsg, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                }
                statusRedraw[4] = 1;
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
                    DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                    SetDialogGWorld(plunderDialog);
                } else {
                    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                    if (CanHireEscorts() != 0) {
                        if (gShipTypes[shipData->type].crew <= 0)
                            ok = 0;
                        else
                            ok = DoCapturedShipAssignmentDialog();
                        if (ok != 0) {
                            done = 1;
                            GetIndString(text, kGameStrList, 0x77);
                            pstrcpy(tmp, gShipTypes[shipData[shipData->targetShip].type].name);
                            tmp[0]++;
                            tmp[tmp[0]] = ' ';
                            tmp[0]++;
                            tmp[tmp[0]] = Rand(9) + '1';
                            tmp[0]++;
                            tmp[tmp[0]] = Rand(9) + '1';
                            tmp[0]++;
                            tmp[tmp[0]] = Rand(9) + '1';
                            tmp[0]++;
                            tmp[tmp[0]] = 0;
                            ok2 = EVTextInputDialog(text, tmp, 0x40);
                            SetDialogGWorld(plunderDialog);
                            if (ok2 != 0) {
                                pstrcpy(shipName, gTextInputResult);
                                SetDialogGWorld(plunderDialog);
                                DoShipCapture(&shipData[shipData->targetShip], 0);
                                SetDialogGWorld(plunderDialog);
                                LoadIntfResource();
                            } else {
                                GetIndString(text, kGameStrList, 0x7a);
                                DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                                SetDialogGWorld(plunderDialog);
                            }
                        } else {
                            done = 1;
                            type = shipData[shipData->targetShip].type;
                            crashlog_mission_bit_set("capturing",
                                                     (char *)gShipTypes[type].name, 1,
                                                     type + 128, gShipTypes[type].onCapture);
                            EvalMissionBitSetString(gShipTypes[shipData[shipData->targetShip].type].onCapture);
                            shipData[shipData->targetShip].aiType = 6;
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
                            DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                            SetDialogGWorld(plunderDialog);
                            statusRedraw[2] = 1;
                        }
                    } else {
                        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                        GetIndString(text, kGameStrList, 0x7c);
                        DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                        SetDialogGWorld(plunderDialog);
                    }
                }
            } else {
                done = 1;
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                GetIndString(text, kGameStrList, 0x7d);
                DisplayComm(text, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
                SetDialogGWorld(plunderDialog);
            }
        }
        if (done == 0) {
            PlunderDialogUpdate();
            HandleComm();
            SetDialogGWorld(plunderDialog);
        }
    } while (done == 0);
    if (plunderDialogPict != NULL) {
        QDReleaseResource(plunderDialogPict);
        plunderDialogPict = NULL;
    }
    SWSetPortToWindow(gSpriteWorld);
    DisposeDialogNoFlash(plunderDialog);
    plunderDialog = NULL;
    SWSetPortToWindow(gSpriteWorld);
    ResetPlayerPrecalcedValues();
}

/* SetPlunderValues (mac _SetPlunderValues). STATE: MATCH 100.0%, exe 611 / obj 611,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x00484230
void SetPlunderValues(void)
{
    short count;
    short flags;
    short dude;
    short pers;
    short value;
    short n;
    short i;
    short j;
    short fuel;
    long crew;
    long mass;
    char ok;

    dude = shipData[shipData->targetShip].dude;
    if (dude == -1)
        flags = 0;
    else
        flags = dudeData[dude].flags;

    if ((flags & kDudeUsesShipCost) != 0) {
        value = gShipTypes[shipData[shipData->targetShip].type].cost / 1000;
        if (value * 0.025 > 2.0)
            salvageCredits = ((Rand(value * 0.025) + value * 0.025) * 1000.0);
        else
            salvageCredits = (value * 0.025 * 1000.0);
        if (salvageCredits < kMinShipCredits)
            salvageCredits = kMinShipCredits;
    } else {
        pers = shipData[shipData->targetShip].persType;
        if (pers != -1) {
            value = personData[pers].credits / 1000;
            if (value * 0.5 > 2.0)
                salvageCredits = ((Rand(value * 0.5) + value * 0.5) * 1000.0);
            else
                salvageCredits = (value * 0.5 * 1000.0);
        } else {
            salvageCredits = -1;
        }
    }
    if (salvageCredits <= 0)
        salvageCredits = -1;

    if ((flags & (kDudeCargoMask | 0x80 | 0xff00)) == 0) {
        salvageType = -1;
    } else {
        do {
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
        } while (ok == 0);
        if (gShipTypes[shipData[shipData->targetShip].type].cargo > 0) {
            salvageQty = Rand(gShipTypes[shipData[shipData->targetShip].type].cargo / 2)
                         + gShipTypes[shipData[shipData->targetShip].type].cargo / 2;
        } else {
            salvageQty = 0;
            salvageType = -1;
        }
    }
    if (salvageQty <= 0)
        salvageType = -1;
    if (salvageType == -1)
        salvageQty = 0;

    count = 0;
    for (i = 0; i < kWeaponTypes; i++) {
        if (shipData[shipData->targetShip].weap[i].ammo > 0
            && gWeapons[i].guidance != kGuidanceBay
            && PlayerCanUseAmmoType(i) != 0)
            count++;
    }
    if (count > 0) {
        do {
            n = Rand(kWeaponTypes);
        } while (shipData[shipData->targetShip].weap[n].ammo <= 0
                 || gWeapons[n].guidance == kGuidanceBay
                 || PlayerCanUseAmmoType(n) == 0);
        salvageAmmoType = n;
        salvageAmmoQty = shipData[shipData->targetShip].weap[n].ammo;
    } else {
        salvageAmmoType = -1;
    }

    fuel = gShipTypes[shipData[shipData->targetShip].type].fuel;
    if (fuel > 0)
        salvageFuelQty = Rand(fuel / 10) * 10;
    else
        salvageFuelQty = 0;

    crew = gShipTypes[shipData->type].strength;
    mass = gShipTypes[shipData->type].crew;
    for (i = 1; i < kEscortSlots; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0
            && shipData[i].aiType == 6 && shipData[i].specialMission == -1
            && gShipTypes[shipData[i].type].inherentAI >= kWarshipAI) {
            crew = (gShipTypes[shipData[i].type].strength * 0.1 + crew);
            mass = (gShipTypes[shipData[i].type].crew * 0.1 + mass);
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        for (j = 0; j < kOutfMods; j++) {
            if (gOutfits[i].modType[j] == kOutfitModPlunder
                && gOutfits[i].modVal[j] > 0)
                mass += gOutfits[i].modVal[j] * shipSystems[i];
        }
    }

    captureOdds = (mass
                          / (gShipTypes[shipData[shipData->targetShip].type].crew * 10.0)
                          * 100.0);
    for (i = 0; i < kMaxOutfits; i++) {
        for (j = 0; j < kOutfMods; j++) {
            if (gOutfits[i].modType[j] == kOutfitModPlunder
                && gOutfits[i].modVal[j] < 0)
                captureOdds += -gOutfits[i].modVal[j] * shipSystems[i];
        }
    }
    if (crew > gShipTypes[shipData[shipData->targetShip].type].strength * 5)
        captureOdds += 10;
    captureOdds += kOddsJitterMid - Rand(kOddsJitter);
    if (captureOdds < 1)
        captureOdds = 1;
    if (captureOdds > kMaxCaptureOdds)
        captureOdds = kMaxCaptureOdds;

    if (shipData[shipData->targetShip].govt != -1
        && (govtData[shipData[shipData->targetShip].govt].flags & kGovtAlwaysDisabled) != 0)
        captureOdds = 0;
    if (((gShipTypes[gameIsRegisteredIndex].registered != 0) ? 1 : 0) == 0)
        captureOdds = 0;
    if (CanHireEscorts() == 0)
        captureOdds = 0;
}

/* PlunderDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00484c20
char PlunderDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;

    if ((int)ev->what == kKeyDown) {
        if ((ev->message & kCharCodeMask) == kReturnChar ||
            (ev->message & kCharCodeMask) == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
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
    if ((int)ev->what == kUpdateEvt)
        PlunderDialogUpdate();
    return 0;
}

/* PlunderDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00484d30
void PlunderDialogUpdate(void)
{
    short itemType;
    void *item;
    Rect box;
    unsigned char text[256];
    short i;
    short j;
    char found;
    long registered;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(plunderDialog));
    if (plunderDialogPict != NULL)
        DrawPicHandle(plunderDialogPict, GetDialogPortRect(plunderDialog));
    GetDialogItem(plunderDialog, 5, &itemType, &item, &box);
    RGBForeColor(blackColor);
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
        RGBForeColor(whiteColor);
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
        RGBForeColor(whiteColor);
        DrawCommaNumber(salvageCredits);
    } else {
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 0x14f);
    }
    MoveTo((box.left + 0x32), (box.top + 0x38));
    if (salvageAmmoType != (unsigned short)-1) {
        RGBForeColor(whiteColor);
        NumToString(salvageAmmoQty, text);
        DrawString(text);
        DrawString("\p ");
        found = 0;
        for (i = 0; i < 512; i++) {
            for (j = 0; j < kOutfMods; j++) {
                if (gOutfits[i].modType[j] == kOutfModAmmo &&
                    salvageAmmoType == (unsigned short)gOutfits[i].modVal[j]) {
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
        RGBForeColor(whiteColor);
        DrawCommaNumber(salvageFuelQty);
    } else {
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 0x14f);
    }
    MoveTo((box.left + 0x78), (box.top + 0x46));
    RGBForeColor(&rgbLightGrey);
    DrawIndString(kGameStrList, 0x70);
    MoveTo((box.left + 0xc3), (box.top + 0x46));
    RGBForeColor(whiteColor);
    NumToString(captureOdds, text);
    DrawString(text);
    DrawString("\p%");
    registered = (gShipTypes[gameIsRegisteredIndex].registered != 0) ? 1 : 0;
    if (!registered) {
        DrawString("\p  (");
        DrawIndString(kRegisterStrList, 1);
        DrawString("\p)");
    }
    RGBForeColor(blackColor);
    DrawPlunderButtons(-1);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(plunderDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(plunderDialog)),
             GetDialogPortRect(plunderDialog), GetDialogPortRect(plunderDialog), 0,
             GetCWindowVisRgn(FrontWindow(plunderDialog)));
}

/*
 * Shares commDialog/commShip/commBackgroundPic/commShipPic/canUpgradeEscort/
 * canSellEscort with the comm dialog (same global slots, one dialog open at a
 * time) and reuses beepSnd[2] for its own close sound.
 */

#define kEscortDialogID 0x3fe

/* DoEscortDialog (mac _DoEscortDialog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004853a0
void DoEscortDialog(ShipObj *ship)
{
    short itemHit;
    char done;

    if (ship == NULL)
        return;

    commShip = ship;
    pstrcpy(shipClassStr, gShipCommNames[commShip->type]);
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    commShipPic = GetPicture(gShipTypes[commShip->type].shipyardPictID);
    ShowTheCursor();

    commDialog = NULL;
    commDialog = GetNewDialog(kEscortDialogID, UpdateEscortDialog, EscortDialogEvent);
    if (commDialog == NULL)
        return;

    commBackgroundPic = GetPicture(0x2141);
    CenterWindow(FrontWindow(commDialog), gWind);

    canUpgradeEscort = 0;
    canSellEscort = 0;
    if (commShip->hired == 0)
        canSellEscort = 1;

    if (gShipTypes[commShip->type].upgradeTo >= 0 && gShipTypes[commShip->type].upgradeTo < 0x300) {
        if (EvalMissionBitTestString(gShipTypes[gShipTypes[commShip->type].upgradeTo].availability))
            canUpgradeEscort = 1;
    }

    done = 0;
    commDialogMode = 2;

    ShowWindowNoFlash(FrontWindow(commDialog));
    SetDialogGWorld(commDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(commDialog);
    UpdateEscortDialog();

    FrontWindow(commDialog);
    FrontWindow(commDialog);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);

    do {
        if (gQuit)
            done = 1;

        RunModalDialog(commDialog, EscortDialogEvent, &itemHit, 0);

        if (itemHit == 1) {
            done = 1;
        } else if (itemHit == 2) {
            DestroyPartialFleetCargo(commShip->index);
            commShip->parentShip = -1;
            commShip->formerEscort = -1;
            commShip->boarded = 1;
            commShip->aiType = gShipTypes[commShip->type].inherentAI;
            AISetup(commShip);
            AIMakeShipLeave(commShip);
            statusRedraw[2] = 1;
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
        QDReleaseResource(commShipPic);
        commShipPic = NULL;
    }

    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);

    if (releaseCommEscort) {
        DestroyPartialFleetCargo(commShip->index);
        commShip->parentShip = -1;
        commShip->aiType = gShipTypes[commShip->type].inherentAI;
        AIMakeShipLeave(commShip);
        AIDispatch(commShip, 1);
        statusRedraw[2] = 1;
    }

    DisposeDialogNoFlash(commDialog);
    commShip = NULL;
    commDialog = NULL;
    gTimeScale = 0;
    if (commBackgroundPic != NULL) {
        QDReleaseResource(commBackgroundPic);
        commBackgroundPic = NULL;
    }
    SWSetPortToWindow(gSpriteWorld);
}

/* EscortDialogEvent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00485840
char EscortDialogEvent(long unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    int c;
    int key;
    int what;

    what = ev->what;
    if (what == kKeyDown) {
        c = ev->message & kCharCodeMask;
        if (c == kReturnChar || c == kEscapeChar) {
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
    }
    if (what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        UpdateEscortDialog();
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
    if (what == kUpdateEvt)
        UpdateEscortDialog();
    return 0;
}

/* UpdateEscortDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00485970
void UpdateEscortDialog(void)
{
    unsigned char text[256];
    short itemType;
    void *item;
    Rect box;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(commDialog));
    if (commBackgroundPic != NULL)
        DrawPicHandle(commBackgroundPic, GetDialogPortRect(commDialog));
    DrawEscortButtons(-1);
    GetDialogItem(commDialog, 11, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(commDialog)))) {
        RGBForeColor(blackColor);
        DrawPicHandle(commShipPic, &box);
    }
    GetDialogItem(commDialog, 12, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(commDialog)))) {
        RGBForeColor(blackColor);
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
        RGBForeColor(whiteColor);
        MoveTo((box.left + 3), (box.top + 0x1b));
        DrawString(shipClassStr);
        pstrcpy(text, gShipTypes[commShip->type].subtitle);
        if (text[0] != 0) {
            MoveTo((box.left + 3), (box.top + 0x29));
            DrawString(text);
        }
        RGBForeColor(blackColor);
    }
    GetDialogItem(commDialog, 10, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(commDialog)))) {
        RGBForeColor(blackColor);
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
                RGBForeColor(whiteColor);
                DrawCommaNumber(gShipTypes[commShip->type].escUpgradeCost);
                DrawString("\p ");
                if (gShipTypes[commShip->type].escUpgradeCost == 1)
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
                RGBForeColor(whiteColor);
                DrawCommaNumber(gShipTypes[commShip->type].tradeIn);
                DrawString("\p ");
                if (gShipTypes[commShip->type].tradeIn == 1)
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
            RGBForeColor(whiteColor);
            DrawCommaNumber((gShipTypes[commShip->type].cost * 0.01));
            DrawString("\p ");
            if (gShipTypes[commShip->type].cost * 0.01 != 1.0)
                DrawString(creditsString);
            else
                DrawString(creditString);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x10b);
        }
    }
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(commDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(commDialog)),
             GetDialogPortRect(commDialog), GetDialogPortRect(commDialog), 0,
             GetCWindowVisRgn(FrontWindow(commDialog)));
}

/*
 * The four functions below are not escort-dialog code: their only caller is
 * EnterGameFromMainScreen (RT3 registration-library scrambling, same family
 * as sources/engine/parser.c 0x0044aa10-0x0044aa50 and
 * sources/engine/ai.c 0x00416090-0x004160d0). No mac symbol exists for
 * any of the four.
 */

/* RegScramA3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004860b0
char RegScramA3(unsigned int x)
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
