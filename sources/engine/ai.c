// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* ai.c -- the AI object (tu-map O1, win 0x00401000..0x004160f0). Notes: Research/notes/ai.md */
#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "macport.h"
#include "gametypes.h"
#include "stellar.h"
#include "gameutil.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "burger.h"
#include "crashlog.h"
#include "sound.h"
#include "space.h"
#include "prototypes.h"
#include "burgermath.h"

#define kRespawnDelayTicks  45

#ifndef MACBUILD
unsigned char DAT_00591000[256];
#endif
Boolean scanActive;
#ifndef MACBUILD
char DAT_00591101[3];
#endif
short prevParent[64];
short aiComplexity;
#ifndef MACBUILD
short DAT_00591186;
unsigned char gRT3Table2[256];
#endif

#ifdef MACBUILD
#ifndef MAC1010 /* 1.1.1 only */
static Boolean AIShouldLeaveSystem(ShipObj *ship, short sys);
#endif
#endif

EXTERN_C_BEGIN

/* AIDispatch (mac _AIDispatch). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00401000
// MAC: _AIDispatch i386 0x0008fb52 ppc 0x0009b654
void AIDispatch(ShipObj *ship, Boolean noBrains)
{
    Boolean skip;
    Boolean attackStellar;

    if (ship->type == kShipTypeEscapePod) {
        ship->thrust = ShipAccelRate(ship);
        ship->speedLimit = ShipMaxSpeed(ship);
        ship->desiredHeading = ship->heading;
        ship->aiTimer -= gSpeedMult;
        if (ship->aiTimer < 0.0F)
            ship->active = 0;
        return;
    }
    if (gameFrozen != 0)
        return;

    attackStellar = 0;
    skip = 0;
    if (ship->specialMission != -1) {
        if (currentMissionObjectives[ship->specialMission].active != 0) {
            if (currentMissions[ship->specialMission].shipBehav == 1) {
                if (IsDisabled(ship) == 0) {
                    ship->aiType = kEscortAI;
                    ship->parentShip = 0;
                }
            } else if (currentMissions[ship->specialMission].shipBehav == 2)
                attackStellar = 1;
        } else {
            ship->specialMission = -1;
            if (ship->parentShip != -1) {
                ship->aiType = shipClass[ship->type].inherentAI;
                ship->parentShip = -1;
                AIMakeShipLeave(ship);
            }
        }
    }

    if (ship->aiTimer <= 0.0F)
        AIHandleCloaking(ship);
    if (ship->hasFormation != 0)
        AICalcFormationPositions(ship, 0);
    if (IsDisabled(ship) != 0 && ship->jumpTimer > 0.0F)
        ship->jumpTimer = -1;

    if (ship->jumpTimer < -900.0F) {
        ship->aiState = kAIStateJumpIn;
        ship->aiSubstate = kSubZoom;
        if (ship->foldSet < shipClass[ship->type].baseSets - 1 &&
            (shipClass[ship->type].shanFlags & kShanFoldingParts) != 0 &&
            ship->foldDir == 0) {
            if ((shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) != 0) {
                if (ReadTick() >=
                     ship->fireTick + kRespawnDelayTicks)
                    ship->foldDir = 1;
            } else
                ship->foldDir = 1;
        }
    } else if ((long)ship->aiSubstate != kSubHypJmp && (long)ship->aiSubstate != kSubJmpPar) {
        if (ship->aiState == kAIStateJumpIn)
            ship->aiState = kAIStateHiJack;
        if (ship->hasEscorts != 0 &&
            frameCounter % 8 == ship->index >> 3)
            AIIssueEscortOrders(ship);
        if (noBrains == 0 &&
            (long)ship->aiSubstate != kSubBoard && (long)ship->aiSubstate != kSubAfterBurn &&
            (long)ship->aiState != kAIStateJumpIn &&
            (long)ship->aiState != kAIStateRefuel &&
            (long)ship->aiState != kAIStateRepair &&
            (long)ship->aiState != kAIStateHiJack &&
            (long)ship->aiSubstate != kSubLoJack &&
            (long)ship->aiSubstate != kSubKillAst &&
            (long)ship->aiState != kAIStateDestStel &&
            aiComplexity != 0 &&
            frameCounter % aiComplexity != ship->index % aiComplexity)
            skip = 1;
        if (ship->aiState == kAIStateConfused) {
            if (gSpeedMult > 0.0) {
                if (Rand(100.0 / gSpeedMult) == 0)
                    ship->aiState = kAIStateHiJack;
                else
                    skip = 1;
            } else {
                if (Rand(100) == 0)
                    ship->aiState = kAIStateHiJack;
                else
                    skip = 1;
            }
        }
        if (skip == 0) {
            if (IsDisabled(ship) != 0) {
                ship->parentShip = -1;
                ship->targetShip = -1;
                ship->targetStellar = -1;
                ship->grudge = 0;
                ship->defenseStellar = -1;
                ship->aiState = kAIStateHiJack;
                ship->aiSubstate = kSubLoJack;
            } else if (AIShipIsEmergingFromHypergate(ship) == 0) {
                if (AIVerifySwarmLeader(ship) == 0)
                    AIFindSwarmLeader(ship);
                if (attackStellar != 0)
                    DeathStarAI(ship);
                else if (ship->defenseStellar != -1)
                    DefenseShipAI(ship);
                else {
                    if ((shipClass[ship->type].flags3 & kShipMiner) != 0 &&
                        ship->parentShip == -1)
                        MinerAI(ship);
                    else {
                        if (ship->aiType == 1)
                            WimpyTraderAI(ship);
                        else if (ship->aiType == 2)
                            BraveTraderAI(ship);
                        else if (ship->aiType == 3) {
                            if (ship->govt != -1) {
                                if ((govtData[ship->govt].flags & kGovtPlunders) != 0)
                                    PirateWarshipAI(ship);
                                else
                                    WarshipAI(ship);
                            } else
                                WarshipAI(ship);
                        } else if (ship->aiType == 4)
                            InterceptorAI(ship);
                        else if (ship->aiType >= 5)
                            EscortAI(ship);
                    }
                }
            }
        }
        if ((shipClass[ship->type].flags3 & kShipMiner) != 0 &&
            ship->parentShip == -1)
            miningShipsActive = 1;
        if (ship->aiTimer <= 0.0F && ship->aiState != kAIStateGoStel &&
            ship->foldDir == 0 &&
            ship->foldSet < shipClass[ship->type].baseSets - 1 &&
            (shipClass[ship->type].shanFlags & kShanFoldingParts) != 0) {
            if ((shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) != 0) {
                if (ReadTick() >= ship->fireTick + kRespawnDelayTicks)
                    ship->foldDir = 1;
            } else
                ship->foldDir = 1;
        }
    } else {
        if (ship->aiState == kAIStateHypPr && ship->parentShip == 0 &&
            IsDisabled(&shipData[ship->parentShip]) != 0) {
            ship->aiState = kAIStateHiJack;
            ship->aiSubstate = kSubLoJack;
            ship->targetShip = -1;
            ship->targetStellar = -1;
            ship->jumpTimer = -1;
        }
        if (ship->aiState == kAIStateHypPr && ship->parentShip > 0 &&
            ship->jumpTimer <= 1.0F &&
            IsDisabled(&shipData[ship->parentShip]) == 0 &&
            shipData[ship->parentShip].aiState == kAIStateFightSh) {
            ship->aiState = kAIStateHiJack;
            ship->aiSubstate = kSubLoJack;
            ship->targetShip = -1;
            ship->targetStellar = -1;
            ship->jumpTimer = -1;
        }
        if (ship->foldSet > 0 && ship->foldDir == 0 &&
            (shipClass[ship->type].shanFlags & kShanFoldingParts) != 0 &&
            (shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) == 0)
            ship->foldDir = -1;
    }

    if (skip == 0)
        HighLevelAIHandler(ship);
    LowLevelAIHandler(ship);
}

#define kScanRange          100.0F
#define kScanChance         75

/* ScanPlayer (mac _ScanPlayer). STATE: MATCH 100.0%, exe 1095 / obj 1095, byte-identical yes
   (4103 bytes, relocations masked).
   OPEN: nothing in the body; the .data literal ORDER is the shared O1 object's. see notes */
// FUNCTION: EVNOVA 0x00401800
// MAC: _ScanPlayer i386 0x0007e46e ppc 0x000880c4
void ScanPlayer(ShipObj *ship)
{
    short i;
    short fine;
    long amt;
    float speed;
    short j;
    char c;
    unsigned char msg[256];
    unsigned char buf2[256];
    char *cmsg;

    if (ship->aiType != 3 && ship->aiType != 4)
        return;
    if (ship->govt == -1)
        return;
    if (govtData[ship->govt].penalties[0] == 0)
        return;
    if (FloatAbs(ship->pos.x - shipData->pos.x) > kScanRange ||
        FloatAbs(ship->pos.y - shipData->pos.y) > kScanRange)
        return;
    if (ShipVisibleToShip(shipData, ship) == 0)
        return;
    if (Rand(100) > kScanChance)
        return;

    if (shipData->jumpTimer > 0.0F && IsDisabled(shipData) == 0) {
        speed = (ReadTick() - shipData->jumpTick) * shipClass[shipData->type].mobilityFactor
                / (WarpSoundDuration() * 0.01);
        if (shipClass[shipData->type].mobilityFactor != 0.0f)
            speed = speed - 35.0 / shipClass[shipData->type].mobilityFactor;
        if (speed > 0.0)
            return;
    }

    if (ship->govt != -1) {
        for (j = 0; j < kMaxActiveMissions; j++) {
            if (currentMissionObjectives[j].active != 0 &&
                (govtData[ship->govt].scanMask &
                 currentMissions[j].scanMask) != 0 &&
                currentMissions[j].hasCargo != 0 &&
                currentMissions[j].cargoType != -1) {
                if ((currentMissions[j].flags & kMissionFailIfScanned) != 0 &&
                    currentMissionObjectives[j].failed == 0) {
                    currentMissionObjectives[j].failed = 1;
                    if ((currentMissionObjectives[j].flags & kMissionHidden) == 0) {
                        ST_PlaySound(beepSnd[1], 1, gSoundVolume);
                        GetIndString(msg, kGameStrList, 0x11e);
                        DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
                    }
                    QuickMissionFailure(j);
                    ship->targetShip = 0;
                    ship->targetStellar = -1;
                    ship->aiState = kAIStateFightSh;
                } else {
                    if (currentMissions[j].shipName[0] > 0) {
                        pstrcpy(msg,
                                (unsigned char *)currentMissions[j].shipName);
                    } else {
                        pstrcpy(msg,
                                shipClass[ship->type].name);
                    }
                    PtoCstr(msg);
                    strcat((char *)msg, ":  ");
                    strncpy((char *)buf2, (char *)shipName, 0x40);
                    PtoCstr(buf2);
                    strcat((char *)msg, (char *)buf2);
                    strcat((char *)msg, ", ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x178);
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x179);
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x17a);
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x17b);
                    strcat((char *)msg, ".");
                    ship->targetShip = 0;
                    ship->targetStellar = -1;
                    ship->aiState = kAIStateFightSh;
                    if (govtData[ship->govt].scanFine != 0) {
                        strcat((char *)msg, " ");
                        StrcatIndCString((char *)msg, kGameStrList, 0x17c);
                        strcat((char *)msg, " ");
#ifdef MAC1010 /* 1.0.10 spelling */
                        if (govtData[ship->govt].scanFine > 0)
                            amt = govtData[ship->govt].scanFine;
                        else {
                            amt = shipData->credits * (-govtData[ship->govt].scanFine * 0.0001);
                            amt = amt * 100;
                            if (amt < 1)
                                amt = 1;
                        }
#else
                        fine = govtData[ship->govt].scanFine;
                        if (fine <= 0) {
                            double scale = -fine * 0.0001;

                            amt = shipData->credits * scale;
                            amt = amt * 100;
                            if (amt < 1)
                                amt = 1;
                        } else
                            amt = fine;
#endif
                        StrcatCommaNumber(amt, (char *)msg);
                        strcat((char *)msg, " ");
                        if (amt > 1)
                            strcat((char *)msg, creditsCString);
                        else
                            strcat((char *)msg, creditCString);
                        strcat((char *)msg, ".");
                        shipData->credits -= amt;
                        if (shipData->credits < 0)
                            shipData->credits = 0;
                        statusRedraw[2] = 1;
                    }
                    CtoPstr((char *)msg);
                    ST_PlaySound(beepSnd[4], 1, gSoundVolume);
                    DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
                }
                break;
            }
        }
        if (playerHasIllegalJunk != 0) {
            if (FloatAbs(ship->pos.x - shipData->pos.x) <= kScanRange &&
                FloatAbs(ship->pos.y - shipData->pos.y) <= kScanRange) {
                for (i = 0; i < kMaxJunks; i++) {
                    if (junkData[i].held > 0 &&
                        (govtData[ship->govt].scanMask & junkData[i].scanMask) != 0) {
                        SlapWithPenalty(shipData->curSystem, ship->govt, 0, -1);
                        if (ship->persType != -1) {
                            pstrcpy(msg,
                                    (unsigned char *)personData[ship->persType].name);
                        } else {
                            pstrcpy(msg,
                                    shipClass[ship->type].name);
                        }
                        PtoCstr(msg);
                        strcat((char *)msg, ":  ");
                        strncpy((char *)buf2, (char *)shipName, 0x40);
                        PtoCstr(buf2);
                        strcat((char *)msg, (char *)buf2);
                        strcat((char *)msg, ", ");
                        StrcatIndCString((char *)msg, kGameStrList, 0x178);
                        strcat((char *)msg, " ");
                        strcat((char *)msg, junkData[i].lcName);
                        strcat((char *)msg, " ");
                        StrcatIndCString((char *)msg, kGameStrList, 0x17a);
                        strcat((char *)msg, " ");
                        StrcatIndCString((char *)msg, kGameStrList, 0x17b);
                        strcat((char *)msg, ".");
                        ship->targetShip = 0;
                        ship->targetStellar = -1;
                        ship->aiState = kAIStateFightSh;
                        if (govtData[ship->govt].scanFine != 0) {
                            strcat((char *)msg, " ");
                            StrcatIndCString((char *)msg, kGameStrList, 0x17c);
                            strcat((char *)msg, " ");
#ifdef MAC1010 /* 1.0.10 spelling */
                            if (govtData[ship->govt].scanFine > 0)
                                amt = govtData[ship->govt].scanFine;
                            else {
                                amt = shipData->credits * (-govtData[ship->govt].scanFine * 0.0001);
                                amt = amt * 100;
                                if (amt < 1)
                                    amt = 1;
                            }
#else
                            fine = govtData[ship->govt].scanFine;
                            if (fine <= 0) {
                                double scale = -fine * 0.0001;

                                amt = shipData->credits * scale;
                                amt = amt * 100;
                                if (amt < 1)
                                    amt = 1;
                            } else
                                amt = fine;
#endif
                            StrcatCommaNumber(amt, (char *)msg);
                            strcat((char *)msg, " ");
                            if (amt > 1)
                                strcat((char *)msg, creditsCString);
                            else
                                strcat((char *)msg, creditCString);
                            strcat((char *)msg, ".");
                            shipData->credits -= amt;
                            if (shipData->credits < 0)
                                shipData->credits = 0;
                            statusRedraw[2] = 1;
                        }
                        CtoPstr((char *)msg);
                        ST_PlaySound(beepSnd[4], 1, gSoundVolume);
                        DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
                        playerHasIllegalJunk = 0;
                        break;
                    }
                }
            }
        } else if (playerHasIllegalOutfits != 0 &&
                   FloatAbs(ship->pos.x - shipData->pos.x) <= kScanRange &&
                   FloatAbs(ship->pos.y - shipData->pos.y) <= kScanRange) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0 &&
                    (govtData[ship->govt].scanMask & outfitSystem[i].scanMask) != 0) {
                    SlapWithPenalty(shipData->curSystem, ship->govt, 0, -1);
                    if (ship->persType != -1) {
                        pstrcpy(msg,
                                (unsigned char *)personData[ship->persType].name);
                    } else {
                        pstrcpy(msg,
                                shipClass[ship->type].name);
                    }
                    PtoCstr(msg);
                    strcat((char *)msg, ":  ");
                    strncpy((char *)buf2, (char *)shipName, 0x40);
                    PtoCstr(buf2);
                    strcat((char *)msg, (char *)buf2);
                    strcat((char *)msg, ", ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x177);
                    strcat((char *)msg, " ");
                    if (shipSystems[i] == 1) {
                        c = tolower(lcOutfitName[i][1]);
                        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' ||
                            c == 'u') {
                            StrcatIndCString((char *)msg, kGameStrList, 0x18a);
                        } else {
                            StrcatIndCString((char *)msg, kGameStrList, 0x189);
                        }
                        strcat((char *)msg, " ");
                        pstrcpy(buf2, lcOutfitName[i]);
                        PtoCstr(buf2);
                        strcat((char *)msg, (char *)buf2);
                    } else {
                        pstrcpy(buf2, lcPluralOutfitName[i]);
                        PtoCstr(buf2);
                        strcat((char *)msg, (char *)buf2);
                    }
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x17b);
                    strcat((char *)msg, ".");
                    ship->targetShip = 0;
                    ship->targetStellar = -1;
                    ship->aiState = kAIStateFightSh;
                    if (govtData[ship->govt].scanFine > 0) {
                        strcat((char *)msg, " ");
                        StrcatIndCString((char *)msg, kGameStrList, 0x17c);
                        strcat((char *)msg, " ");
                        StrcatCommaNumber(govtData[ship->govt].scanFine, (char *)msg);
                        strcat((char *)msg, " ");
                        if (govtData[ship->govt].scanFine > 1)
                            strcat((char *)msg, creditsCString);
                        else
                            strcat((char *)msg, creditCString);
                        strcat((char *)msg, ".");
                        shipData->credits -= govtData[ship->govt].scanFine;
                        if (shipData->credits < 0)
                            shipData->credits = 0;
                        statusRedraw[2] = 1;
                    }
                    CtoPstr((char *)msg);
                    ST_PlaySound(beepSnd[4], 1, gSoundVolume);
                    DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
                    playerHasIllegalOutfits = 0;
                    break;
                }
            }
        }
    }
}

/* AISetup. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00402810
// MAC: _AISetup i386 0x0007f3e2 ppc 0x00088eb8
void AISetup(ShipObj *ship)
{
    ship->aiState = kAIStateHiJack;
    ship->aiSubstate = kSubLoJack;
    ship->lastStellar = -2;
    ship->lastInspected = -1;
    ship->escortOrder = -1;
    ship->swarmLeader = -1;
    ship->formLeader = -1;
}

#ifndef MAC1010 /* not in 1.0.10 */
/* RecentlyVisitedStellarInSystem (mac _RecentlyVisitedStellarInSystem, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _RecentlyVisitedStellarInSystem i386 0x0007f429 ppc 0x00088ee8
Boolean RecentlyVisitedStellarInSystem(ShipObj *ship, short sys)
{
    short i;
    short count;

    count = 0;
    for (i = 0; i < kNavSlots; i++) {
        if (systemData[sys].navDefault[i] != -1)
            count++;
    }
    if (count == 0)
        return 1;
    for (i = 0; i < kNavSlots; i++) {
        if (ship->lastStellar == systemData[sys].navDefault[i] && ship->lastStellar != -1)
            return 1;
    }
    return 0;
}
#endif

/* WimpyTraderAI. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00402860
// MAC: _WimpyTraderAI i386 0x0008b029 ppc 0x00096590
void WimpyTraderAI(ShipObj *ship)
{
    if (IsDisabled(ship) != 0)
        return;
    if ((int)ship->aiState == kAIStateRefuel || (int)ship->aiState == kAIStateRepair)
        return;
    if ((int)ship->aiState == kAIStateCountWait)
        return;
    if ((int)ship->aiState == kAIStateHiJack) {
        if (AIShouldLeaveSystem(ship, ship->curSystem) != 0) {
            if (AIShipHasFuelForJump(ship))
                AIMakeShipLeave(ship);
            else
                ship->aiState = kAIStateWait;
        } else {
            ship->targetStellar = -1;
            ship->targetStellar = SelectRandomStellarDest(ship, 0, 0);
            if (ship->targetStellar != -1) {
                ship->navTargetKind = 2;
                ship->aiState = kAIStateGoStel;
            } else if (AIShipHasFuelForJump(ship)) {
                AIMakeShipLeave(ship);
            } else {
                ship->aiState = kAIStateWait;
            }
        }
    }
    if (ship->grudge > 0 && ship->targetShip != -1) {
        if (ship->parentShip == 0) {
            ship->aiState = kAIStateFlyPr;
            ship->targetStellar = 0;
        } else {
            ship->aiState = kAIStateDefRet;
        }
    }
    if (ship->aiState == kAIStateDefRet)
        AICallForHelp(ship);
}

/* MinerAI. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00402980
// MAC: _MinerAI i386 0x0008b202 ppc 0x000967d8
void MinerAI(ShipObj *ship)
{
    Boolean flagA;
    Boolean flagB;
    Boolean flagC;
    short flags3;
    short result;

    if (IsDisabled(ship)) {
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
        ship->targetStellar = -1;
        ship->targetShip = -1;
        return;
    }
    if ((int)ship->aiState == kAIStateCountWait)
        return;

    if (ship->grudge > 0 && ship->targetShip != -1) {
        ship->aiState = kAIStateDefRet;
    } else {
        flagB = 0;
        flagA = 0;
        flagC = 0;
        flags3 = shipClass[ship->type].flags3;
        if (flags3 & kShipDestroysAsteroids)
            flagA = 1;
        if (flags3 & kShipScoopsDebris)
            flagB = 1;
        if (!flagA && !flagB)
            flagC = 1;

        if (flagA) {
            if ((int)ship->aiState != kAIStateHypOut) {
                if (asteroidData[0].active != 0) {
                    ship->aiState = kAIStateAtkAster;
                } else {
                    ship->targetShip = -1;
                    ship->targetStellar = -1;
                    ship->aiState = kAIStateWait;
                }
            }
        }
        if (flagB) {
            if (ship->canScoop != 0) {
                result = TotalCargo(ship);
                if (result >= shipClass[ship->type].cargo) {
                    AIMakeShipLeave(ship);
                } else if (miningBoxesActive) {
                    ship->aiState = kAIStateScoop;
                } else if (flagA) {
                    ship->aiState = kAIStateAtkAster;
                } else {
                    flagC = 1;
                }
            } else {
                flagC = 1;
            }
        }
        if (flagC) {
            if (ship->aiTimer <= 0.0f) {
                if (ship->targetStellar == -1)
                    ship->targetStellar = SelectNearestAnyStellar(ship, ship->lastStellar);
                if (ship->targetStellar != -1 && (int)ship->aiState != kAIStateHypOut
                        && (int)ship->aiState != kAIStateDefRet) {
                    ship->aiState = kAIStateGoStel;
                    ship->lastStellar = ship->targetStellar;
                } else if (AIShipHasFuelForJump(ship)) {
                    AIMakeShipLeave(ship);
                } else {
                    ship->aiState = kAIStateWait;
                }
            } else {
                ship->targetStellar = -1;
                ship->aiState = kAIStateHiJack;
            }
        }
    }

    if (ship->aiState == kAIStateDefRet)
        AICallForHelp(ship);
}

#define kEscortRange     0x4e2
#define kHelpThreshold       3

/* BraveTraderAI. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00402bd0
// MAC: _BraveTraderAI i386 0x0008b493 ppc 0x00096ab4
void BraveTraderAI(ShipObj *ship)
{
    short dx;
    short dy;

    if (IsDisabled(ship))
        return;
    if ((int)ship->aiState == kAIStateRefuel || (int)ship->aiState == kAIStateRepair)
        return;
    if ((int)ship->aiState == kAIStateCountWait)
        return;
    if ((int)ship->aiState == kAIStateHiJack) {
        if (AIShouldLeaveSystem(ship, ship->curSystem)) {
            if (AIShipHasFuelForJump(ship))
                AIMakeShipLeave(ship);
            else
                ship->aiState = kAIStateWait;
        } else {
            ship->targetStellar = -1;
            ship->targetStellar = SelectRandomStellarDest(ship, 0, 0);
            if (ship->targetStellar != -1) {
                ship->navTargetKind = 2;
                ship->aiState = kAIStateGoStel;
            } else if (AIShipHasFuelForJump(ship)) {
                AIMakeShipLeave(ship);
            } else {
                ship->aiState = kAIStateWait;
            }
        }
    }

    if (ship->grudge > 0 && ship->targetShip != -1) {
        dx = abs((short)(ship->pos.x - shipData[ship->targetShip].pos.x));
        dy = abs((short)(ship->pos.y - shipData[ship->targetShip].pos.y));
        if (dx <= kEscortRange && dy <= kEscortRange) {
            if (ship->jumpTimer <= 0.0f)
                ship->aiState = kAIStateFightSh;
        } else if (ship->parentShip == 0) {
            ship->aiState = kAIStateFlyPr;
            ship->targetStellar = 0;
        } else {
            ship->aiState = kAIStateDefRet;
        }
    }

    if ((int)ship->aiState == kAIStateDefRet) {
        AICallForHelp(ship);
        AICallForReinforcements(ship, 0);
    } else if ((int)ship->aiState == kAIStateFightSh && ship->targetShip != -1 && ship->targetShip != 0
            && shipData[ship->targetShip].aiType >= kHelpThreshold) {
        AICallForHelp(ship);
    }
}

#define kNeverRetreat       (-32767)

/* WarshipAI. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00402e50
// MAC: _WarshipAI i386 0x0008b729 ppc 0x00096e14
void WarshipAI(ShipObj *ship)
{
    short n;
    short i;
    short f;
    long thresh;

    if (IsDisabled(ship) != 0)
        return;
    if ((long)ship->aiState == kAIStateRefuel ||
        (long)ship->aiState == kAIStateRepair)
        return;
    if ((long)ship->aiState == kAIStateCountWait)
        return;

    if (ship->govt != -1 && ship->jumpTimer > -900.0F && ship->jumpTimer <= 0.0F) {
        if ((govtData[ship->govt].flags & kGovtAlwaysAttacksPlayer) != 0) {
            if (IsThreatToPlayer(ship) == 0 &&
                ShipVisibleToShip(shipData, ship) != 0) {
                AIMakeShipAttackPlayer(ship);
                ship->targetShip = 0;
                ship->grudge = 1;
            }
        } else {
            if ((Boolean)(govtData[ship->govt].flags & kGovtNeverAttacksPlayer) != 0 &&
                IsThreatToPlayer(ship) != 0) {
                ship->targetShip = -1;
                ship->aiState = kAIStateHiJack;
            }
        }
    }

    if (ship->aiSubstate == kSubHypJmp &&
        (long)ship->aiState != kAIStateHypOut &&
        (long)ship->aiState != kAIStateHypPr)
        ship->aiState = kAIStateHypOut;

    if ((long)ship->aiState == kAIStateHiJack) {
        if (ship->targetShip != -1) {
            if (ship->jumpTimer <= 0.0F &&
                (long)ship->aiState != kAIStateInspect)
                ship->aiState = kAIStateFightSh;
        } else if (ship->jumpTimer <= 0.0F &&
                   (long)ship->aiState != kAIStateDefRet) {
            if (ship->targetShip == -1) {
                SelectWarshipTarget(ship);
                if (ship->targetShip != -1)
                    ship->aiState = kAIStateFightSh;
                else if (AIShouldLeaveSystem(ship, ship->curSystem) != 0) {
                    if (AIShipHasFuelForJump(ship))
                        AIMakeShipLeave(ship);
                    else
                        ship->aiState = kAIStateWait;
                } else {
                    ship->targetStellar = -1;
                    ship->targetStellar = SelectRandomStellarDest(ship, 0, 0);
                    if (ship->targetStellar != -1) {
                        ship->navTargetKind = 2;
                        ship->aiState = kAIStateGoStel;
                    } else if (AIShipHasFuelForJump(ship))
                        AIMakeShipLeave(ship);
                    else
                        ship->aiState = kAIStateWait;
                }
            }
            if (ship->targetShip == -1) {
                if (AIShouldLeaveSystem(ship, ship->curSystem) != 0) {
                    if (AIShipHasFuelForJump(ship))
                        AIMakeShipLeave(ship);
                    else
                        ship->aiState = kAIStateWait;
                } else {
                    ship->targetStellar = -1;
                    ship->targetStellar = SelectRandomStellarDest(ship, 0, 0);
                    if (ship->targetStellar != -1) {
                        ship->navTargetKind = 2;
                        ship->aiState = kAIStateGoStel;
                    } else if (AIShipHasFuelForJump(ship))
                        AIMakeShipLeave(ship);
                    else
                        ship->aiState = kAIStateWait;
                }
            }
        }
    }

    if (ship->grudge > 0 && ship->targetShip != -1 && ship->jumpTimer <= 0.0F) {
        if (ship->aiState != kAIStateInspect && ship->aiState != kAIStateDefRet)
            ship->aiState = kAIStateFightSh;
    }

    if (ship->aiState == kAIStateGoStel ||
        ship->aiState == kAIStateGoHGate ||
        ship->aiState == kAIStateHypOut) {
        if (ship->targetShip != -1) {
            if (ship->jumpTimer <= 0.0F &&
                ship->aiState != kAIStateInspect)
                ship->aiState = kAIStateFightSh;
        } else if (ship->aiState != kAIStateDefRet &&
                   ship->jumpTimer <= 0.0F) {
            SelectWarshipTarget(ship);
            if (ship->targetShip != -1)
                ship->aiState = kAIStateFightSh;
        }
    }

    if (ship->aiState == kAIStateWait) {
        ship->targetShip = -1;
        SelectWarshipTarget(ship);
        if (ship->targetShip == -1) {
            n = 0;
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].parentShip == ship->index &&
                    shipData[i].active != 0 &&
                    shipData[i].aiType == 5 &&
                    shipData[i].defenseStellar == -1 &&
                    IsDisabled(&shipData[i]) == 0)
                    n++;
            }
            if (n == 0)
                ship->aiState = kAIStateHiJack;
        }
    }

    if (ship->targetShip != -1 && ship->aiState == kAIStateFightSh) {
        if (shipData[ship->targetShip].active == 0 ||
            IsDying(&shipData[ship->targetShip]) != 0 ||
            (IsDisabled(&shipData[ship->targetShip]) != 0 &&
             AIHasDestroyingWeapons(ship) == 0)) {
            ship->aiState = kAIStateHiJack;
            ship->targetShip = -1;
        }
    }

    if (ship->targetShip != -1 && ship->govt != -1 &&
        ship->aiState == kAIStateFightSh &&
        (govtData[ship->govt].flags & kGovtRetreatsWhenHurt) != 0 &&
        ship->shield < ShipShieldCapacity(ship) * 0.5 &&
        ship->odds >= 0.0F) {
        f = systemData[ship->curSystem].reinforceFleet;
        if (f != -1) {
            if (GovtAllies(ship->govt, fleetData[f].govt) != 0) {
                if (systemData[ship->curSystem].reinforceTimer > 0.0F) {
                    if (ship->odds > govtData[ship->govt].maxOdds * 2.0F)
                        ship->aiState = kAIStateDefRet;
                } else {
                    if (ship->odds > govtData[ship->govt].maxOdds)
                        ship->aiState = kAIStateDefRet;
                }
            } else {
                if (ship->odds > govtData[ship->govt].maxOdds)
                    ship->aiState = kAIStateDefRet;
            }
        } else {
            if (ship->odds > govtData[ship->govt].maxOdds)
                ship->aiState = kAIStateDefRet;
        }
    }

    if (ship->targetShip != -1 &&
        (long)ship->aiState != kAIStateInspect &&
        (long)ship->aiState != kAIStateDefRet) {
        if (ship->jumpTimer <= 0.0F)
            ship->aiState = kAIStateFightSh;

        if (ship->persType == -1) {
            thresh = kNeverRetreat;
            if (ship->aggression == 1)
                thresh = ShipShieldCapacity(ship) * 0.3;
            if (ship->aggression == 2)
                thresh = ShipShieldCapacity(ship) * 0.15;
            if (ship->aggression == 4)
                thresh = kNeverRetreat;
        } else
            thresh = ShipShieldCapacity(ship) *
                     (personData[ship->persType].coward * 0.01);

        if (ship->shield < thresh && ship->parentShip == -1 &&
            ship->govt != -1 &&
            (govtData[ship->govt].flags & kGovtRetreatsWhenHurt) != 0) {
            if ((long)ship->aiState != kAIStateHypOut && (long)ship->aiState != kAIStateDefRet
                    && (long)ship->aiState != kAIStateHypPr)
                ship->aiState = kAIStateDefRet;
        }

        if ((shipClass[ship->type].flags2 & kShipAmmoAware) != 0 &&
            AIShipIsOutOfAmmo(ship) != 0) {
            if ((long)ship->aiState != kAIStateHypOut && (long)ship->aiState != kAIStateDefRet
                    && (long)ship->aiState != kAIStateHypPr)
                ship->aiState = kAIStateDefRet;
        }
    }

    if (ship->aiState == kAIStateHypOut && AIShipHasFuelForJump(ship) == 0) {
        ship->targetStellar = -1;
        ship->targetStellar = SelectRandomStellarDest(ship, 0, 0);
        if (ship->targetStellar != -1) {
            ship->navTargetKind = 2;
            ship->aiState = kAIStateGoStel;
        } else if (AIShipHasFuelForJump(ship))
            AIMakeShipLeave(ship);
        else
            ship->aiState = kAIStateWait;
    }

    if (ship->aiState == kAIStateFightSh && AIShipIsOutOfAmmo(ship) == 2) {
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
        ship->targetShip = -1;
        ship->targetStellar = -1;
    }
}

#define kInherentAITrader   2     /* inherentAI <= this is a trader */
#define kInherentAIWarship  3

#define kBoardCashInDelay   100.0f
#define kBackOffDelay       120.0f

/* PirateWarshipAI (mac _PirateWarshipAI). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004038b0
// MAC: _PirateWarshipAI i386 0x0008c2d2 ppc 0x00097d20
void PirateWarshipAI(ShipObj *ship)
{
    short i;

    if (IsDisabled(ship) != 0)
        return;
    if (ship->aiState == kAIStateCountWait)
        return;

    if (ship->targetShip != -1) {
        if (shipData[ship->targetShip].active != 0) {
            if (IsDying(&shipData[ship->targetShip]) != 0) {
                ship->targetShip = -1;
                ship->aiState = kAIStateHiJack;
            }
        } else {
            ship->targetShip = -1;
            ship->aiState = kAIStateHiJack;
        }
    }

    if ((long)ship->aiState == kAIStateHiJack ||
        (ship->targetShip == -1 && (long)ship->aiState != kAIStateUnPlunder)) {
        ship->targetShip = -1;
        AIFindShipToPlunder(ship);
        if (ship->targetShip == -1)
            SelectWarshipTarget(ship);
        if (ship->targetShip != -1) {
            if ((ship->targetShip == 0 ||
                 shipData[ship->targetShip].formerEscort >= 0 ||
                 shipClass[shipData[ship->targetShip].type].inherentAI <=
                     kInherentAITrader) &&
                shipClass[shipData[ship->targetShip].type].crew != 0)
                ship->aiState = kAIStatePlunder;
            else
                ship->aiState = kAIStateFightSh;
        } else {
            if (AIShouldLeaveSystem(ship, ship->curSystem) != 0) {
                if (AIShipHasFuelForJump(ship))
                    AIMakeShipLeave(ship);
                else
                    ship->aiState = kAIStateWait;
            } else if (ship->targetStellar == -1) {
                ship->targetStellar = SelectRandomStellarDest(ship, 0, 0);
                if (ship->targetStellar != -1) {
                    ship->navTargetKind = 2;
                    ship->aiState = kAIStateGoStel;
                } else if (AIShipHasFuelForJump(ship)) {
                    AIMakeShipLeave(ship);
                } else {
                    ship->aiState = kAIStateWait;
                }
            } else if (AIShipHasFuelForJump(ship)) {
                AIMakeShipLeave(ship);
            } else {
                ship->aiState = kAIStateWait;
            }
        }
    }

    if (ship->targetShip != -1 &&
        ((long)ship->aiState == kAIStatePlunder ||
         (long)ship->aiState == kAIStateFightSh)) {
        if ((ship->targetShip == 0 ||
             shipData[ship->targetShip].formerEscort >= 0 ||
             shipClass[shipData[ship->targetShip].type].inherentAI <=
                 kInherentAITrader) &&
            shipClass[shipData[ship->targetShip].type].crew > 0) {
            if (shipData[ship->targetShip].boarded != 0) {
                ship->aiState = kAIStateFightSh;
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active != 0 &&
                        i != ship->index &&
                        i != ship->targetShip &&
                        shipData[i].parentShip != 0 &&
                        IsDisabled(&shipData[i]) == 0 &&
                        shipData[i].targetShip == ship->targetShip &&
                        (shipData[i].aiState == kAIStatePlunder ||
                         shipData[i].aiSubstate == kAIStateUnPlunder)) {
                        ship->aiState = kAIStateCountWait;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        ship->targetStellar = -1;
                        ship->grudge = 0;
                        ship->aiTimer = kBackOffDelay;
                        break;
                    }
                }
            } else {
                ship->aiState = kAIStatePlunder;
            }
        } else {
            ship->aiState = kAIStateFightSh;
        }
    }

    if (ship->aiState == kAIStateFightSh &&
        ship->aiSubstate == kSubBoard &&
        ship->targetStellar != -1 &&
        ship->aiTimer <= 0.0f) {
        ship->aiTimer = kBoardCashInDelay;
        ship->aiState = kAIStateUnPlunder;
        ship->aiSubstate = kSubLoJack;
        ship->defenseStellar = -1;
        AIPlunderShipContents(ship, &shipData[ship->targetStellar]);
        ship->targetShip = -1;
        ship->targetStellar = -1;
    }

    if (ship->aiState == kAIStateFightSh && ship->targetShip != -1 &&
        IsDisabled(&shipData[ship->targetShip]) != 0) {
        if (shipClass[shipData[ship->targetShip].type].inherentAI >=
                kInherentAIWarship &&
            ship->targetShip != 0 &&
            shipData[ship->targetShip].formerEscort < 0 &&
            (AIHasDestroyingWeapons(ship) == 0 ||
             AIShipIsOutOfAmmo(ship) == 2)) {
            ship->aiState = kAIStateHiJack;
            ship->targetShip = -1;
        }
    }

    if (((long)ship->aiState == kAIStateFightSh ||
         (long)ship->aiState == kAIStatePlunder) &&
        AIShipIsOutOfAmmo(ship) == 2) {
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
        ship->targetShip = -1;
        ship->targetStellar = -1;
    }
}

#define kSharewareGraceDays 30
#define kHailFirstIndex     381

/* InterceptorAI. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00403de0
// MAC: _InterceptorAI i386 0x0008c895 ppc 0x00098428
void InterceptorAI(ShipObj *ship)
{
    short i;
    short n;
    short j;
    short t;
    short s;
    short f;
    unsigned char msg[256];
    short m;
    unsigned char buf2[256];

    if (IsDisabled(ship) != 0)
        return;
    if ((long)ship->aiState == kAIStateCountWait)
        return;

    if (ship->persType == kPersCaptainHector) {
        if (ship->jumpTimer <= 0.0F) {
            if (ship->aiTimer <= 0.0F) {
                if (ship->lastInspected != 0) {
                    if (IsDying(shipData) != 0) {
                        if (AIShipHasFuelForJump(ship))
                            AIMakeShipLeave(ship);
                        else
                            ship->aiState = kAIStateWait;
                    } else if (totalDaysPlayed * (1 - (shipClass[gameIsRegisteredIndex].registered != 0
                                                      ? 1 : 0)) > kSharewareGraceDays) {
                        ship->aiState = kAIStateFightSh;
                        ship->targetShip = 0;
                        ship->targetStellar = -1;
                        ship->grudge = 1;
                    } else {
                        ship->aiState = kAIStateInspect;
                        ship->targetShip = 0;
                    }
                } else if ((long)ship->aiState != kAIStateFightSh) {
                    if (AIShipHasFuelForJump(ship))
                        AIMakeShipLeave(ship);
                    else
                        ship->aiState = kAIStateWait;
                }
            }
        }
        return;
    }

    if ((long)ship->aiState == kAIStateRefuel ||
        (long)ship->aiState == kAIStateRepair)
        return;

    if (ship->govt != -1 && ship->jumpTimer > -900.0F && ship->jumpTimer <= 0.0F) {
        if ((govtData[ship->govt].flags & kGovtAlwaysAttacksPlayer) != 0) {
            if (IsThreatToPlayer(ship) == 0 &&
                ShipVisibleToShip(shipData, ship) != 0) {
                AIMakeShipAttackPlayer(ship);
                ship->targetShip = 0;
                ship->grudge = 1;
            }
        } else {
            if ((Boolean)(govtData[ship->govt].flags & kGovtNeverAttacksPlayer) != 0 &&
                IsThreatToPlayer(ship) != 0) {
                ship->targetShip = -1;
                ship->aiState = kAIStateHiJack;
            }
        }
    }

    if (ship->grudge > 0 && ship->targetShip != -1 &&
        (long)ship->aiState != kAIStateDefRet &&
        (long)ship->aiState != kAIStateInspect &&
        (long)ship->aiState != kAIStateRefuel &&
        (long)ship->aiState != kAIStateRepair &&
        ship->jumpTimer <= 0.0F)
        ship->aiState = kAIStateFightSh;

    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].active == 0 && i == ship->lastInspected)
            ship->lastInspected = -1;
    }

    if (((long)ship->aiState == kAIStateHiJack ||
         (long)ship->aiState == kAIStateGoStel ||
         (long)ship->aiState == kAIStateGoHGate) &&
        ship->aiTimer <= 0.0F) {
        if (ship->targetShip != -1) {
            if (ship->jumpTimer <= 0.0F &&
                shipData[ship->targetShip].active != 0 && ship->grudge > 0)
                ship->aiState = kAIStateFightSh;
        } else {
            ship->targetShip = -1;
            SelectWarshipTarget(ship);
            if (ship->targetShip != -1 && ship->jumpTimer <= 0.0F)
                ship->aiState = kAIStateFightSh;
        }
        if (ship->targetShip == -1) {
            n = 0;
            for (i = 0; i < kMaxShips; i++) {
                if (i != ship->index && shipData[i].active != 0 &&
                    i != ship->lastInspected && shipData[i].aiType != 4 &&
                    ShipVisibleToShip(&shipData[i], ship) != 0 &&
                    shipData[i].curSystem == ship->curSystem)
                    n++;
            }
            if (n > 0) {
                while (ship->targetShip == -1) {
                    t = Rand(kMaxShips);
                    if (t != ship->index && shipData[t].active != 0 &&
                        t != ship->lastInspected && shipData[t].aiType != 4 &&
                        ShipVisibleToShip(&shipData[t], ship) != 0 &&
                        shipData[t].curSystem == ship->curSystem) {
                        ship->targetShip = t;
                        ship->lastInspected = t;
                        ship->aiState = kAIStateInspect;
                        if (t == 0) {
                            m = -1;
                            if (ship->govt != -1) {
                                for (j = 0; j < kMaxActiveMissions; j++) {
                                    if (currentMissionObjectives[j].active != 0) {
                                        if (currentMissions[j].hasCargo != 0) {
                                            if ((govtData[ship->govt].scanMask &
                                                 currentMissions[j].scanMask) != 0) {
                                                m = j;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if (m != -1 && commCount < 0) {
                                strncpy((char *)msg,
                                        (char *)shipClass[ship->type].name,
                                        0x3f);
                                PtoCstr(msg);
                                strcat((char *)msg, ":  ");
                                strncpy((char *)buf2, (char *)shipName,
                                        0x3f);
                                PtoCstr(buf2);
                                strcat((char *)msg, (char *)buf2);
                                strcat((char *)msg, ", ");
                                StrcatIndCString((char *)msg, kGameStrList,
                                                Rand(3) + kHailFirstIndex);
                                strcat((char *)msg, ".");
                                CtoPstr((char *)msg);
                                ST_PlaySound(beepSnd[4], 1, gSoundVolume);
                                DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
                            }
                        }
                    }
                }
            }
        }
        if (ship->targetShip == -1 && ship->aiState == kAIStateHiJack) {
            ship->lastStellar = -2;
            s = SelectRandomStellarDest(ship, 0, 1);
            if (s == -1) {
                if (AIShipHasFuelForJump(ship))
                    AIMakeShipLeave(ship);
                else
                    ship->aiState = kAIStateWait;
            } else {
                if (StellarIsDestroyed(&stellarData[s]) != 0)
                    ship->aiState = kAIStateWait;
                else {
                    ship->targetStellar = s;
                    ship->aiState = kAIStateGoStel;
                }
            }
        }
    }

    if (ship->targetShip != -1 && ship->govt != -1 &&
        ship->aiState == kAIStateFightSh &&
        (govtData[ship->govt].flags & kGovtPersNoEscapePod) != 0 &&
        ship->shield < ShipShieldCapacity(ship) * 0.5 &&
        ship->odds >= 0.0F) {
        f = systemData[ship->curSystem].reinforceFleet;
        if (f != -1) {
            if (GovtAllies(ship->govt, fleetData[f].govt) != 0) {
                if (systemData[ship->curSystem].reinforceTimer > 0.0F) {
                    if (ship->odds > govtData[ship->govt].maxOdds * 2.0F)
                        ship->aiState = kAIStateDefRet;
                } else {
                    if (ship->odds > govtData[ship->govt].maxOdds)
                        ship->aiState = kAIStateDefRet;
                }
            } else {
                if (ship->odds > govtData[ship->govt].maxOdds)
                    ship->aiState = kAIStateDefRet;
            }
        } else {
            if (ship->odds > govtData[ship->govt].maxOdds)
                ship->aiState = kAIStateDefRet;
        }
    }

    if (ship->targetShip != -1 && ship->aiState == kAIStateFightSh &&
        IsDisabled(&shipData[ship->targetShip]) != 0 &&
        AIHasDestroyingWeapons(ship) == 0) {
        ship->targetShip = -1;
        ship->aiState = kAIStateHiJack;
    }

    if ((shipClass[ship->type].flags2 & kShipAmmoAware) != 0 &&
        AIShipIsOutOfAmmo(ship) != 0) {
        if ((long)ship->aiState != kAIStateHypOut && (long)ship->aiState != kAIStateDefRet
                && (long)ship->aiState != kAIStateHypPr)
            ship->aiState = kAIStateDefRet;
    }

    if (ship->targetShip != -1 && ship->aiState == kAIStateInspect &&
        ShipVisibleToShip(&shipData[ship->targetShip], ship) == 0) {
        ship->targetShip = -1;
        ship->targetStellar = -1;
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
    }

    if (ship->targetShip != -1 && ship->govt != -1 && ship->aiState == kAIStateInspect) {
        t = ship->targetShip;
        SelectWarshipTarget(ship);
        if (ship->targetShip == -1) {
            ship->targetShip = t;
            ship->aiState = kAIStateInspect;
        }
    }
}

#define kEscortRangeSq      408375.0  /* squared; past it the target is dropped */

#define kVoiceMsgAttack     1
#define kNoTarget           0x226     /* FindNearestThreatToParent: any threat */

/* EscortAI. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004048a0
// MAC: _EscortAI i386 0x000838d2 ppc 0x0008ddf4
void EscortAI(ShipObj *ship)
{
    short parent;
    short orders;
    short ammo;

    if ((long)ship->aiState == kAIStateCountWait)
        return;
    parent = ship->parentShip;
    if (parent != 0 && ((long)ship->aiState == kAIStateRefuel ||
                        (long)ship->aiState == kAIStateRepair))
        return;
    if (parent == -1) {
        ship->aiType = shipClass[ship->type].inherentAI;
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
        ship->jumpTimer = -1.0f;
        return;
    }
    if (shipData[parent].active == 0 || IsDying(&shipData[parent]) != 0) {
        ship->parentShip = -1;
        ship->aiType = shipClass[ship->type].inherentAI;
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
        ship->jumpTimer = -1.0f;
        return;
    }

    if (ship->parentShip == 0) {
        if (ship->specialMission == -1)
            ship->govt = -1;
        if (ship->voiceClass != 0 && ship->voiceClass != 1) {
            ship->voiceClass = Rand(2);
            if (shipClass[ship->type].inherentGovtB != -1 &&
                    govtData[shipClass[ship->type].inherentGovtB].voiceClass != -1)
                ship->voiceClass = govtData[shipClass[ship->type].inherentGovtB].voiceClass;
        }
    }

    if ((ship->parentShip > 0 && shipData[ship->parentShip].aiSubstate == kSubHypJmp) ||
        (ship->parentShip > 0 && shipData[ship->parentShip].aiSubstate == kSubJmpPar) ||
        (ship->parentShip > 0 && shipData[ship->parentShip].aiState == kAIStateHypOut &&
             shipData[ship->parentShip].aiSubstate == kSubKillSpd) ||
        shipData[ship->parentShip].jumpTimer > 0.0f) {
        ship->targetStellar = shipData[ship->parentShip].targetStellar;
        ship->targetShip = -1;
        if (ship->parentShip != 0 && ShipIsInertialess(ship) != 0) {
            ship->parentShip = -1;
            ship->aiType = shipClass[ship->type].inherentAI;
            ship->aiState = kAIStateHypOut;
            ship->aiSubstate = kSubHypJmp;
            ship->jumpTimer = 0.0f;
            return;
        }
        ship->aiState = kAIStateHypPr;
    }

    if (ship->targetShip != -1 &&
            (shipData[ship->targetShip].active == 0 ||
             (IsDisabled(&shipData[ship->targetShip]) != 0 && ship->escortOrder != kOrderAttack)))
        ship->targetShip = -1;

    if (ship->parentShip == 0) {
        ship->escortOrder = playerEscortOrders[shipClass[ship->type].escortType];
        if (ship->escortOrder == -1)
            ship->escortOrder = kOrderFormation;
        if (ship->escortOrder == kOrderRegroup) {
            if (ship->aiType == 5) {
                if (ship->escortMayAttack == 0)
                    ship->escortOrder = kOrderFormation;
            } else {
                ship->escortOrder = kOrderFormation;
            }
        }
    } else if (ship->parentShip != -1 && shipData[ship->parentShip].parentShip == 0) {
        orders = shipData[ship->parentShip].escortOrder;
        if ((long)orders == kOrderAttack || (long)orders == kOrderDefend) {
            if (shipData[ship->parentShip].aiState == kAIStateFlyPr)
                ship->escortOrder = kOrderRegroup;
            else
                ship->escortOrder = orders;
        } else {
            ship->escortOrder = kOrderRegroup;
        }
    }

    if (((long)ship->escortOrder == kOrderAttack ||
         (long)ship->escortOrder == kOrderDefend) &&
            (shipClass[ship->type].flags2 & kShipAmmoAware) != 0) {
        ammo = AIShipIsOutOfAmmo(ship);
        if (ammo != 0) {
            if (ship->aiType == 5)
                ship->escortOrder = kOrderRegroup;
            else if (ammo == 2)
                ship->escortOrder = kOrderFormation;
        }
    }

    if (ship->aiState == kAIStateHypPr) {
        ship->targetShip = -1;
        if (ship->parentShip != 0 && ShipIsInertialess(ship) != 0) {
            ship->parentShip = -1;
            ship->aiType = shipClass[ship->type].inherentAI;
            ship->aiState = kAIStateHypOut;
            ship->aiSubstate = kSubHypJmp;
            ship->jumpTimer = 0.0f;
            return;
        }
    } else if ((long)ship->escortOrder == kOrderDefend) {
        ship->jumpTimer = -1.0f;
        ship->aiTimer = -1.0f;
        if (ship->targetShip != -1) {
            if (XYFloatSquaredDist(&shipData[ship->targetShip].pos,
                                   &shipData[ship->parentShip].pos)
                    <= kEscortRangeSq)
                ship->aiState = kAIStateFightSh;
            else
                ship->targetShip = -1;
        }
        if (ship->targetShip == -1) {
            ship->targetShip = FindNearestThreatToParent(ship, kNoTarget);
            if (ship->parentShip == 0 && ship->targetShip != -1 && pendingVoiceMsg == -1 &&
                    voiceSnd == 0 &&
                    shipClass[ship->type].escortType <= 1 &&
                    (shipClass[ship->type].flags2 & kShipNoFighterVoice) == 0 &&
                    Rand(3) == 0)
                SpeakVoiceMessage(kVoiceMsgAttack,
                                  shipClass[ship->type].inherentGovtB, ship->voiceClass);
        }
        if (ship->targetShip == -1) {
            ship->aiState = kAIStateFlyPr;
            ship->targetStellar = ship->parentShip;
        } else {
            ship->aiState = kAIStateFightSh;
        }
    } else if ((long)ship->escortOrder == kOrderAttack) {
        ship->jumpTimer = -1.0f;
        ship->aiTimer = -1.0f;
        if (ship->targetShip == -1) {
            ship->targetShip = FindNearestThreatToParent(ship, -1);
            ship->targetStellar = -1;
            if (ship->targetShip != -1) {
                ship->aiState = kAIStateFightSh;
            } else {
                ship->aiState = kAIStateFlyPr;
                ship->targetStellar = ship->parentShip;
            }
            if (ship->parentShip == 0 && ship->targetShip != -1 && pendingVoiceMsg == -1 &&
                    voiceSnd == 0 &&
                    shipClass[ship->type].escortType <= 1 &&
                    (shipClass[ship->type].flags2 & kShipNoFighterVoice) == 0 &&
                    Rand(3) == 0)
                SpeakVoiceMessage(kVoiceMsgAttack,
                                  shipClass[ship->type].inherentGovtB, ship->voiceClass);
        }
        if (ship->targetShip == -1) {
            ship->aiState = kAIStateFlyPr;
            ship->targetStellar = ship->parentShip;
        } else {
            ship->aiState = kAIStateFightSh;
        }
    } else if ((long)ship->escortOrder == kOrderHold) {
        ship->targetShip = -1;
        ship->targetStellar = -1;
        ship->aiState = kAIStateWait;
        AIFireTurret(ship);
    } else if (ship->aiType == 5 && (long)ship->escortOrder == kOrderRegroup) {
        ship->aiState = kAIStateGoHome;
        ship->targetShip = -1;
        ship->targetStellar = ship->parentShip;
    } else {
        ship->jumpTimer = -1.0f;
        ship->aiTimer = -1.0f;
        if (ship->targetShip != -1 &&
                AIInGunRange(ship, &shipData[ship->targetShip], -1) == 0)
            ship->targetShip = -1;
        if (ship->targetShip == -1)
            AIFightThreatToParent(ship);
        ship->aiState = kAIStateFlyPr;
        ship->targetStellar = ship->parentShip;
        if (ship->targetShip == ship->parentShip)
            ship->targetShip = -1;
        if (ship->targetShip != -1 && ship->parentShip != 0) {
            AIFireGun(ship, 0);
            AIFireTurret(ship);
        }
    }
}

#define kMaxTargetRangeSq  36000000.0f  /* 6000^2, hand-verified at 0x00575060 */

/* DefenseShipAI (mac _DefenseShipAI). STATE: MATCH 100.0% (163 exact, 10 reloc-equiv, 0 mismatched; exe 173 /
   obj 173), frame 0x18 EXACT, 6 of 6 slots, relocregcheck 0 of 10, call/imm/fcmp censuses
   exact, poolcheck 1 of 1; byte-identical pending the next regate --update (bytediff reads
   the baseline). OPEN: nothing. */
// FUNCTION: EVNOVA 0x00405120
// MAC: _DefenseShipAI i386 0x0007f7ed ppc 0x00089384
void DefenseShipAI(ShipObj *ship)
{
    float distAbs;
    short i;
    short pass;
    short bestIndex;
    long bestDist;
    short target;

    if (IsDying(ship))
        return;
    if (ship->aiState == kAIStateCountWait)
        return;

    bestIndex = -1;
    bestDist = -1;
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < 64; i++) {
            if (!shipData[i].active)
                continue;
            if (i != 0 && shipData[i].parentShip != 0)
                continue;
            if (!ShipVisibleToShip(&shipData[i], ship))
                continue;
            distAbs = FloatAbs(XYFloatSquaredDist(&ship->pos, &shipData[i].pos));
            if (distAbs <= kMaxTargetRangeSq || pass == 1) {
                if (i != 0)
                    target = i;
                if (distAbs < bestDist || bestDist == -1) {
                    bestDist = distAbs;
                    bestIndex = i;
                }
            }
        }
        if (pass == 0 && bestIndex != -1)
            break;
    }

    ship->aiTimer = 0.0f;
    ship->targetStellar = -1;
    if (ship->targetShip != -1) {
        /* BUG: i is the finished scan's counter, not targetShip */
        if (!ShipVisibleToShip(&shipData[i], ship))
            ship->targetShip = -1;
    }
    if (ship->targetShip == -1) {
        if (bestIndex != -1) {
            ship->targetShip = bestIndex;
            ship->aiState = kAIStateFightSh;
        } else {
            ship->targetStellar = ship->defenseStellar;
            ship->aiState = kAIStateGoStel;
        }
    }

    target = ship->targetShip;
    if (target != -1 && target != 0 && shipData[target].parentShip != 0) {
        if (bestIndex != -1) {
            ship->targetShip = bestIndex;
        } else {
            ship->targetShip = -1;
            ship->aiState = kAIStateHiJack;
            ship->aiSubstate = kSubLoJack;
        }
    }
}

/* DeathStarAI. STATE: MATCH 100.0%, exe 130 / obj 130, frame 0x4; byte identity unchecked
   until the next regate (bytediff covers baseline MATCHes only).
   OPEN: none. One counter `i' serves both scans; a second `j' was the extra entity. */
// FUNCTION: EVNOVA 0x004053c0
// MAC: _DeathStarAI i386 0x0008c125 ppc 0x00097b30
void DeathStarAI(ShipObj *ship)
{
    short i;
    short bestStellar;
    short foundWeapon;
    short guidance;
    short stellar;

    if (IsDisabled(ship) || IsDying(ship))
        return;
    if (ship->aiState == kAIStateCountWait)
        return;

    bestStellar = -1;
    for (i = 0; i < kNavSlots; i++) {
        stellar = systemData[ship->curSystem].navDefault[i];
        if (stellar == -1)
            continue;
        if (StellarIsDestroyed(&stellarData[stellar]))
            continue;
        if (stellarData[stellar].strength <= 0)
            continue;
        if (!GovtEnemies(ship->govt, stellarData[stellar].govt))
            continue;
        bestStellar = ship->targetStellar = stellar;
        break;
    }

    foundWeapon = -1;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].flags2 & kWeapPlanetType) {
            if (shipClass[ship->type].weapCount[i] <= 0)
                continue;
            if (!WeaponHasAmmo(ship, i))
                continue;
            guidance = weaponStats[i].guidance;
            if (guidance == kGuidanceBeam)
                continue;
            if (guidance == kGuidanceTurretBeam)
                continue;
            if (guidance > kGuidanceFrontQuad)
                continue;
            foundWeapon = i;
            break;
        }
    }

    if (bestStellar != -1 && foundWeapon != -1) {
        ship->targetShip = ship->targetStellar = bestStellar;
        ship->aiState = kAIStateDestStel;
    } else {
        if (ship->aiState == kAIStateDestStel) {
            ship->targetShip = -1;
            ship->targetStellar = -1;
            ship->aiState = kAIStateHiJack;
            ship->aiSubstate = kSubLoJack;
        }

        WarshipAI(ship);
    }
}

/* HighLevelAIHandler (mac _HighLevelAIHandler). STATE: MATCH 100%, exe 2492 / obj
   2492, frame 0x224 exact. OPEN: none in the body; see notes. */
// FUNCTION: EVNOVA 0x00405590
// MAC: _HighLevelAIHandler i386 0x0008d453 ppc 0x00098fdc
void HighLevelAIHandler(ShipObj *ship)
{
    short best;
    unsigned char msg[256];
    FloatPoint pt;
    char tmp[256];
    short reach;
    short range;
    short i;
    Boolean farOut;
    short dy;
    float dist;
    float rangeSq;

    if (ship->aiState == kAIStateCountWait) {
        if (ship->aiTimer > 0.0f) {
            ship->fireQueued = 0;
            ship->targetShip = -1;
            ship->targetStellar = -1;
            ship->aiSubstate = kSubKillSpd;
        } else {
            ship->aiState = kAIStateHiJack;
            ship->aiSubstate = kSubLoJack;
        }
    }

    if (ship->aiTimer > 0.0f && (int)ship->aiState != kAIStateFlyPr
            && (int)ship->aiState != kAIStateUnPlunder)
        return;

    if (ship->targetShip != -1 && shipData[ship->targetShip].active == 0)
        ship->targetShip = -1;

    if ((int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateHypOut
            && (int)ship->aiState != kAIStateDefRet)
        ship->jumpTimer = 0.0f;

    if (ship->jumpTimer > 0.0f) {
        if (ship->parentShip == 0)
            ship->aiState = kAIStateHypPr;
        else if (ship->targetShip != -1)
            ship->aiState = kAIStateDefRet;
        else
            ship->aiState = kAIStateHypOut;
    }

    if (ship->aiType == 5 && ship->aiState == kAIStateHypPr && !AIShipHasFuelForJump(ship)) {
        ship->aiState = kAIStateGoHome;
        ship->targetShip = -1;
        ship->targetStellar = ship->parentShip;
    }

    if (ship->specialMission != -1 && (int)ship->aiState != kAIStateHypPr
            && !((int)ship->aiState == kAIStateJumpIn || (int)ship->aiState == kAIStateEmerge)
            && currentMissions[ship->specialMission].shipBehav == 0) {
        if (!IsDisabled(shipData) || AIHasDestroyingWeapons(ship)) {
            if (ShipVisibleToShip(shipData, ship))
                AIMakeShipAttackPlayer(ship);
        } else
            AIMakeShipLeave(ship);
    }

    if (ship->targetShip != -1
            && ((int)ship->aiState == kAIStateFightSh || (int)ship->aiState == kAIStatePlunder)) {
        if (!ShipVisibleToShip(&shipData[ship->targetShip], ship)) {
            if (ship->aiType <= 2) {
                if (AIShipHasFuelForJump(ship))
                    AIMakeShipLeave(ship);
                else
                    ship->aiState = kAIStateWait;
            } else {
                ship->aiSubstate = kSubKillSpd;
                ship->jumpTimer = 0.0f;
                ship->targetStellar = -1;
                if (ship->searchTimer < 0.0f) {
                    ship->searchTimer = Rand(100) + 100;
                } else {
                    ship->searchTimer -= gSpeedMult;
                    if (ship->searchTimer <= 0.0f) {
                        ship->targetShip = -1;
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->searchTimer = -1.0f;
                    }
                }
                return;
            }
        } else {
            ship->searchTimer = -1.0f;
        }
    }

    if (ship->aiState == kAIStateGoStel && ship->targetStellar != -1) {
        if ((stellarData[ship->targetStellar].flags2 & kSpobGateOrWormhole) != 0) {
            ship->aiState = kAIStateGoHGate;
        } else {
            ship->jumpTimer = 0.0f;
            pt.x = (short)(stellarData[ship->targetStellar].pos.h - ship->pos.x);
            pt.y = (short)(stellarData[ship->targetStellar].pos.v - ship->pos.y);
            range = ((9.0 - (ShipTurnRate(ship) < 8.0 ? ShipTurnRate(ship) : 8.0)) * 8.0 + 32.0);
            if (FloatAbs(pt.x) > range || FloatAbs(pt.y) > range) {
                ship->aiSubstate = kSubFlyStel;
            } else {
                if ((shipClass[ship->type].shanFlags & kShanFoldingParts) != 0 && ship->foldSet > 0)
                    ship->foldDir = -1;
                if (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35) {
                    ship->aiSubstate = kSubKillSpd;
                    ship->vel.x *= 0.98;
                    ship->vel.y *= 0.98;
                    ship->accelMag *= 0.98;
                } else {
                    ship->vel.x = ship->vel.y = 0.0f;
                    ship->accelMag = 0.0f;
                    ship->lastStellar = ship->targetStellar;
                    ship->aiSubstate = kSubLoJack;
                    ship->aiState = kAIStateHiJack;
                    if ((shipClass[ship->type].flags3 & kShipScoopsDebris) != 0)
                        ship->aiTimer = (Rand(75) + 100);
                    else
                        ship->aiTimer = (Rand(200) + 300);
                }
            }
        }
    }

    if (ship->aiState == kAIStateGoHGate && ship->targetStellar >= 0
            && ship->targetStellar < 2048) {
        ship->jumpTimer = 0.0f;
        pt.x = (short)(stellarData[ship->targetStellar].pos.h - ship->pos.x);
        pt.y = (short)(stellarData[ship->targetStellar].pos.v - ship->pos.y);
        i = GetStellarSpriteXSize(ship->targetStellar) / 4;
        if (FloatAbs(pt.x) > i || FloatAbs(pt.y) > i) {
            ship->aiSubstate = kSubFlyStel;
            ship->aiTimer = -1.0f;
        } else {
            ship->vel.x = 0.0f;
            ship->vel.y = 0.0f;
            ship->aiSubstate = kSubEnterHG;
            if (ship->aiTimer < 0.0f || ship->aiTimer > 16.0f)
                ship->aiTimer = 16.0f;
#ifdef MAC1010 /* 1.0.10 spelling */
#define escortShip i
#else
#define escortShip dy
#endif
            for (escortShip = 1; escortShip < kMaxShips; escortShip++) {
                if (escortShip != ship->index && shipData[escortShip].active != 0
                        && ship->index == shipData[escortShip].parentShip) {
                    shipData[escortShip].parentShip = -1;
                    shipData[escortShip].aiType = ship->aiType;
                    if (shipData[escortShip].aiType > 3)
                        shipData[escortShip].aiType = 3;
                    shipData[escortShip].defenseStellar = -1;
                    shipData[escortShip].grudge = -1;
                    shipData[escortShip].targetShip = -1;
                    shipData[escortShip].targetStellar = ship->targetStellar;
                    shipData[escortShip].aiState = kAIStateGoHGate;
                    shipData[escortShip].aiSubstate = kSubLoJack;
                }
            }
#undef escortShip
        }
    }

    if (ship->aiState == kAIStateEmerge) {
        ship->aiSubstate = kSubLoJack;
        ship->targetShip = -1;
        if (ship->aiTimer <= 0.0f) {
            ship->aiTimer = -1.0f;
            ship->aiState = kAIStateJumpIn;
            ship->targetStellar = -1;
        }
    }

    if (ship->aiState == kAIStateHypOut) {
        farOut = 1;
        pt.x = pt.y = 0.0f;
        if (FloatAbs(XYFloatSquaredDist(&pt, &ship->pos))
                <= 1000000.0)
            farOut = 0;
        if (farOut) {
            if (!ShipCanExpiditeJumps(ship)
                    && (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35))
                ship->aiSubstate = kSubKillSpd;
            else
                ship->aiSubstate = kSubHypJmp;
        } else {
            ship->aiSubstate = kSubFlyHypD;
        }
    }

    if (ship->aiState == kAIStateHypPr) {
        if (ship->parentShip != -1) {
            if (ship->parentShip == 0) {
                if (shipData->jumpTimer > 0.0f) {
                    ship->targetStellar = shipData[ship->parentShip].targetStellar;
                    ship->targetShip = -1;
                    ship->aiSubstate = kSubJmpPar;
                } else {
                    ship->targetShip = -1;
                    ship->targetStellar = -1;
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                }
            } else {
                if ((shipData[ship->parentShip].aiState = kAIStateHypOut) != 0
                        && ((int)shipData[ship->parentShip].aiSubstate == kSubKillSpd
                            || (int)shipData[ship->parentShip].aiSubstate == kSubHypJmp)) {
                    ship->targetStellar = shipData[ship->parentShip].targetStellar;
                    ship->targetShip = -1;
                    ship->aiSubstate = kSubJmpPar;
                } else {
                    ship->targetShip = -1;
                    ship->targetStellar = -1;
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                }
            }
        } else {
            ship->targetShip = -1;
            ship->targetStellar = -1;
            ship->aiState = kAIStateHypOut;
            ship->aiSubstate = kSubHypJmp;
        }
    }

    if (ship->aiState == kAIStateDefRet) {
        if (ship->jumpTimer > 0.0f) {
            farOut = 1;
            pt.x = pt.y = 0.0f;
            if (FloatAbs(XYFloatSquaredDist(&pt, &ship->pos))
                    <= 1000000.0)
                farOut = 0;
            if (farOut) {
                ship->aiSubstate = kSubHypJmp;
            } else {
                ship->jumpTimer = 0.0f;
                ship->aiSubstate = kSubFlyHypD;
            }
        } else if (ship->targetShip != -1) {
            i = abs((short)(ship->pos.x - shipData[ship->targetShip].pos.x));
            dy = abs((short)(ship->pos.y - shipData[ship->targetShip].pos.y));
            if (i <= 250 && dy <= 250 && ship->aiSubstate != kSubHypJmp) {
                if (ship->jumpTimer <= 0.0f) {
                    ship->aiSubstate = kSubRunAwy;
                    if (ship->govt != -1 && ship->parentShip != 0
                            && GovtAllies(ship->govt, shipData[ship->targetShip].govt)
                            && shipData[ship->targetShip].parentShip != 0) {
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        return;
                    }
                }
                AICallForReinforcements(ship, 0);
            } else {
                farOut = 1;
                pt.x = pt.y = 0.0f;
                if (FloatAbs(XYFloatSquaredDist(&pt, &ship->pos))
                        <= 1000000.0)
                    farOut = 0;
                if (farOut && AIShipHasFuelForJump(ship)) {
                    if (!ShipCanExpiditeJumps(ship)
                            && (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35))
                        ship->aiSubstate = kSubKillSpd;
                    else
                        ship->aiSubstate = kSubHypJmp;
                } else {
                    ship->aiSubstate = kSubFlyHypD;
                }
            }
        } else {
            ship->aiState = kAIStateHiJack;
            ship->aiSubstate = kSubLoJack;
            ship->grudge = 0;
        }
    }

    if (ship->aiState == kAIStateFightSh) {
        ship->jumpTimer = 0.0f;
        if (ship->targetShip != -1 && ship->aiTimer <= 0.0f) {
            if (ship->govt != -1 && ship->parentShip != 0
                    && GovtAllies(ship->govt, shipData[ship->targetShip].govt)
                    && shipData[ship->targetShip].parentShip != 0) {
                ship->aiState = kAIStateHiJack;
                ship->aiSubstate = kSubLoJack;
                return;
            }
            if (ship->parentShip != -1) {
                if (ship->targetShip == ship->parentShip) {
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                    ship->targetShip = -1;
                    return;
                }
                if (ship->parentShip == shipData[ship->targetShip].parentShip) {
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                    ship->targetShip = -1;
                    return;
                }
                if (shipData[ship->targetShip].parentShip != -1
                        && shipData[shipData[ship->targetShip].parentShip].parentShip != -1) {
                    if (shipData[ship->targetShip].parentShip
                            == shipData[shipData[ship->targetShip].parentShip].parentShip) {
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        return;
                    }
                    if (shipData[ship->parentShip].parentShip != -1
                            && shipData[shipData[ship->parentShip].parentShip].parentShip
                               == shipData[shipData[ship->targetShip].parentShip].parentShip) {
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        return;
                    }
                }
            }
            if (ship->parentShip == ship->targetShip && ship->defenseStellar == -1) {
                ship->grudge = 0;
                ship->targetShip = -1;
                ship->aiState = kAIStateHiJack;
            }
            if (shipData[ship->targetShip].active != 0
                    && !IsDying(&shipData[ship->targetShip])) {
                if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > 165.0f
                        || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > 165.0f) {
                    if ((shipClass[ship->type].flags2 & kShipStandoff) != 0) {
                        i = (ShipMaxWeaponRange(ship) * 0.85);
                        if (IsDisabled(&shipData[ship->targetShip]))
                            i *= 0.5;
#ifdef MACBUILD
                        if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > i
                                || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > i)
                            ship->aiSubstate = kSubMissile;
                        else if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > 165.0f
                                || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > 165.0f)
                            ship->aiSubstate = kSubWaitTarg;
                        else
                            ship->aiSubstate = kSubRunAwy;
#else
                        {
                            float fx;
                            float fy;

                            fx = fabs((double)(ship->pos.x - shipData[ship->targetShip].pos.x));
                            if (i < fx
                                    || (fy = fabs((double)(ship->pos.y - shipData[ship->targetShip].pos.y)),
                                        i < fy))
                                ship->aiSubstate = kSubMissile;
                            else if (fx > 165.0f || fy > 165.0f)
                                ship->aiSubstate = kSubWaitTarg;
                            else
                                ship->aiSubstate = kSubRunAwy;
                        }
#endif
                    } else if (AIEvalHopelessChase(ship)) {
                        if (ship->aiType >= 3)
                            ship->aiSubstate = kSubWaitTarg;
                        else {
                            ship->aiState = kAIStateDefRet;
                            ship->aiSubstate = kSubRunAwy;
                        }
                    } else if (!AIDoSwarming(ship) && ship->aiSubstate != kSubAfterBurn) {
                        ship->aiSubstate = kSubMissile;
                    }
                } else if ((shipClass[ship->type].flags2 & kShipStandoff) != 0) {
                    ship->aiSubstate = kSubRunAwy;
                } else if (ship->aiSubstate != kSubVeerOff) {
                    ship->aiSubstate = kSubTF;
                }
                if (!IsDisabled(ship)) {
                    AILaunchFighter(ship);
                    AICallForReinforcements(ship, 0);
                }
            } else {
                ship->targetShip = -1;
                ship->grudge = 0;
                ship->aiState = kAIStateHiJack;
            }
        } else {
            ship->aiState = kAIStateHiJack;
        }
    }

    if (ship->aiState == kAIStatePlunder) {
        ship->jumpTimer = 0.0f;
        if (ship->targetShip != -1 && ship->aiTimer <= 0.0f) {
            if (ship->parentShip != -1) {
                if (ship->targetShip == ship->parentShip) {
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                    ship->targetShip = -1;
                    return;
                }
                if (ship->parentShip == shipData[ship->targetShip].parentShip) {
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                    ship->targetShip = -1;
                    return;
                }
                if (shipData[ship->targetShip].parentShip != -1
                        && shipData[shipData[ship->targetShip].parentShip].parentShip != -1) {
                    if (shipData[ship->targetShip].parentShip
                            == shipData[shipData[ship->targetShip].parentShip].parentShip) {
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        return;
                    }
                    if (shipData[ship->parentShip].parentShip != -1
                            && shipData[shipData[ship->parentShip].parentShip].parentShip
                               == shipData[shipData[ship->targetShip].parentShip].parentShip) {
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        return;
                    }
                }
            }
            if (ship->parentShip == ship->targetShip && ship->defenseStellar == -1) {
                ship->grudge = 0;
                ship->targetShip = -1;
                ship->aiState = kAIStateHiJack;
            }
            if (shipData[ship->targetShip].active != 0) {
                if (IsDisabled(&shipData[ship->targetShip])) {
                    if (shipData[ship->targetShip].boarded != 0 && ship->aiTimer <= 0.0f) {
                        ship->aiState = kAIStateHiJack;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        ship->targetStellar = -1;
                    } else {
                        ship->targetStellar = ship->targetShip;
                        reach = ((10.0f - shipClass[ship->type].turnRate) * 30.0f);
                        if (ShipIsInertialess(ship))
                            reach *= 4;
#ifdef MACBUILD
                        if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) <= reach
#else
                        if (FloatAbs((double)(ship->pos.x - shipData[ship->targetShip].pos.x)) <= reach
#endif
                                && FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) <= reach) {
                            ship->aiSubstate = kSubBoard;
                        } else {
#ifdef MACBUILD
                            if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) <= reach * 2
#else
                            if (FloatAbs((double)(ship->pos.x - shipData[ship->targetShip].pos.x)) <= reach * 2
#endif
                                    && FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) <= reach * 2)
                                ship->aiSubstate = kSubChasSlw;
                            else
                                ship->aiSubstate = kSubChase;
                        }
                    }
                } else {
                    ship->targetStellar = -1;
                    if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > 165.0f
                            || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > 165.0f) {
                        if (AIEvalHopelessChase(ship)) {
                            if (ship->aiType >= 3)
                                ship->aiSubstate = kSubWaitTarg;
                            else {
                                ship->aiState = kAIStateDefRet;
                                ship->aiSubstate = kSubRunAwy;
                            }
                        } else if (ship->aiSubstate != kSubAfterBurn) {
                            ship->aiSubstate = kSubMissile;
                        }
                    } else if ((int)ship->aiSubstate != kSubVeerOff
                            && (int)ship->aiSubstate != kSubAfterBurn) {
                        ship->aiSubstate = kSubTF;
                    }
                }
            } else {
                ship->targetShip = -1;
                ship->targetStellar = -1;
                ship->grudge = 0;
                ship->aiState = kAIStateHiJack;
            }
        } else {
            ship->aiState = kAIStateHiJack;
        }
    }

    if (ship->aiState == kAIStateUnPlunder) {
        ship->targetShip = -1;
        ship->targetStellar = -1;
        ship->aiSubstate = kSubLoJack;
        Accel(ship->heading, gSpeedMult * 0.7, &ship->pos);
        if (ship->aiTimer <= 0.0f)
            ship->aiState = kAIStateHiJack;
    }

    if (ship->aiState == kAIStateGoHome && ship->parentShip != -1 && ship->defenseStellar == -1) {
        ship->targetStellar = ship->parentShip;
        reach = ((10.0f - shipClass[ship->type].turnRate) * 50.0f);
        if (reach < 100)
            reach = 100;
        if (FloatAbs(ship->pos.x - shipData[ship->parentShip].pos.x) <= reach
                && FloatAbs(ship->pos.y - shipData[ship->parentShip].pos.y) <= reach)
            ship->aiSubstate = kSubDock;
        else
            ship->aiSubstate = kSubChasSlw;
        if (!ShipVisibleToShip(&shipData[ship->parentShip], ship))
            ship->aiSubstate = kSubKillSpd;
    }

    if (ship->aiState == kAIStateFlyPr && ship->parentShip != -1 && ship->defenseStellar == -1) {
        ship->targetStellar = ship->parentShip;
        if (!ShipVisibleToShip(&shipData[ship->parentShip], ship)) {
            if (FloatAbs(ship->pos.x - shipData[ship->parentShip].pos.x) <= 300.0f
                    && FloatAbs(ship->pos.y - shipData[ship->parentShip].pos.y) <= 300.0f)
                ship->aiSubstate = kSubKillSpd;
            else
                ship->aiSubstate = kSubChase;
        } else {
#ifdef MACBUILD
            if (FloatAbs(ship->pos.x - shipData[ship->parentShip].pos.x) <= 300.0f
                    && FloatAbs(ship->pos.y - shipData[ship->parentShip].pos.y) <= 300.0f)
                ship->aiSubstate = kSubFormFly;
            else if (FloatAbs(ship->pos.x - shipData[ship->parentShip].pos.x) <= 600.0f
                    && FloatAbs(ship->pos.y - shipData[ship->parentShip].pos.y) <= 600.0f)
                ship->aiSubstate = kSubChasSlw;
            else
                ship->aiSubstate = kSubChase;
#else
            float fy;
            float fx;

            fx = fabs((double)(ship->pos.x - shipData[ship->parentShip].pos.x));
            if (fx <= 300.0f
                    && (fy = fabs((double)(ship->pos.y - shipData[ship->parentShip].pos.y)),
                        fy <= 300.0f))
                ship->aiSubstate = kSubFormFly;
            else if (fx <= 600.0f
                    && (fy = fabs((double)(ship->pos.y - shipData[ship->parentShip].pos.y)),
                        fy <= 600.0f))
                ship->aiSubstate = kSubChasSlw;
            else
                ship->aiSubstate = kSubChase;
#endif
        }
    }

    if (ship->aiState == kAIStateProtect) {
        ship->jumpTimer = 0.0f;
        ship->targetStellar = 0;
        ship->targetShip = -1;
        reach = ((10.0f - shipClass[ship->type].turnRate) * 30.0f);
        if (shipData->jumpTimer > 0.0f) {
            ship->aiState = kAIStateHypPr;
        } else if (!ShipVisibleToShip(shipData, ship)) {
            ship->aiSubstate = kSubKillSpd;
        } else {
#ifdef MACBUILD
            if (FloatAbs(ship->pos.x - shipData->pos.x) <= reach
#else
            if (FloatAbs((double)(ship->pos.x - shipData->pos.x)) <= reach
#endif
                    && FloatAbs(ship->pos.y - shipData->pos.y) <= reach) {
                ship->aiSubstate = kSubFormFly;
            } else {
                reach = ((10.0f - shipClass[ship->type].turnRate) * 60.0f);
#ifdef MACBUILD
                if (FloatAbs(ship->pos.x - shipData->pos.x) <= reach
#else
                if (FloatAbs((double)(ship->pos.x - shipData->pos.x)) <= reach
#endif
                        && FloatAbs(ship->pos.y - shipData->pos.y) <= reach)
                    ship->aiSubstate = kSubChasSlw;
                else
                    ship->aiSubstate = kSubChase;
            }
        }
    }

    if (ship->aiState == kAIStateWait) {
        ship->jumpTimer = 0.0f;
        ship->aiSubstate = kSubKillSpd;
    }

    if (ship->aiState == kAIStateInspect) {
        if (ship->targetShip != -1 && ship->aiTimer <= 0.0f) {
            if (shipData[ship->targetShip].active != 0) {
                if (ship->targetShip == 0)
                    scanActive = 1;
                if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > 100.0f
                        || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > 100.0f) {
                    ship->aiSubstate = kSubChase;
                } else {
                    if (ship->targetShip == 0) {
                        pt.x = ship->vel.x - shipData->vel.x;
                        pt.y = ship->vel.y - shipData->vel.y;
                        if (ship->persType == kPersCaptainHector)
                            pt.x = pt.y = 0.0f;
                        if (ship->persType == kPersCaptainHector) {
                            ship->lastInspected = 0;
                            if (totalDaysPlayed * (1 - ((shipClass[gameIsRegisteredIndex].registered != 0)
                                        ? 1 : 0)) > 30) {
                                ship->aiState = kAIStateFightSh;
                                ship->targetShip = 0;
                                ship->targetStellar = -1;
                                ship->grudge = 1;
                            } else {
                                ST_PlaySound(beepSnd[4], 1, gSoundVolume);
                                GetIndString(msg, kHackedStrList, Rand(3) + 2);
                                DisplayComm((PStrChar *)msg, 0x168, 0, 12,
                                            &rgbWhite, 0, 0);
                                AIMakeShipLeave(ship);
                            }
                        } else {
                            ship->targetStellar = -1;
                            ship->targetShip = -1;
                            ship->aiState = kAIStateHiJack;
                            ship->aiSubstate = kSubLoJack;
                            ScanPlayer(ship);
                        }
                    } else {
                        ship->aiState = kAIStateHiJack;
                        ship->targetShip = -1;
                    }
                }
            } else {
                ship->targetShip = -1;
                ship->aiState = kAIStateHiJack;
            }
        } else {
            ship->aiState = kAIStateHiJack;
        }
    }

    if (ship->aiState == kAIStateJumpIn) {
        ship->jumpTimer = -999.0f;
        ship->aiSubstate = kSubZoom;
    }

    if (ship->aiState == kAIStateRefuel && ship->targetShip != -1) {
        ship->aiSubstate = kSubChasSlw;
        reach = ((10.0f - shipClass[ship->type].turnRate) * 15.0f);
        if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > reach
                || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > reach) {
            ship->aiSubstate = kSubChasSlw;
        } else {
            if (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35) {
                ship->aiSubstate = kSubKillSpd;
            } else {
                ship->vel.x = ship->vel.y = 0.0f;
                if (shipData[ship->targetShip].fuel <= 100.0) {
                    shipData[ship->targetShip].fuel += 1.0;
                    if (ship->targetShip == 0)
                        statusRedraw[4] = 1;
                } else {
                    if (ship->targetShip == 0) {
                        ST_PlaySound(beepSnd[1], 1, gSoundVolume);
                        strncpy((char *)msg, (char *)shipClass[ship->type].name, 62);
                        PtoCstr(msg);
                        strcat((char *)msg, ":  ");
                        pstrcpy((unsigned char *)tmp, fuelString[0]);
                        PtoCstr((unsigned char *)tmp);
                        strcat((char *)msg, tmp);
                        strcat((char *)msg, ", ");
                        strncpy(tmp, (char *)shipName, 62);
                        PtoCstr((unsigned char *)tmp);
                        strcat((char *)msg, tmp);
                        strcat((char *)msg, ".");
                        CtoPstr((char *)msg);
                        DisplayComm((PStrChar *)msg, 0xf0, 0, 12, &rgbWhite, 0, 0);
                        statusRedraw[1] = 1;
                        statusRedraw[4] = 1;
                    }
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                    ship->targetShip = -1;
                    ship->targetStellar = -1;
                }
            }
        }
    }

    if (ship->aiState == kAIStateRepair) {
        if (ship->targetShip != -1) {
            if (shipData[ship->targetShip].active == 0
                    || !IsDisabled(&shipData[ship->targetShip])) {
                ship->aiState = kAIStateHiJack;
                ship->aiSubstate = kSubLoJack;
                ship->targetShip = -1;
                ship->targetStellar = -1;
                return;
            }
            ship->targetStellar = ship->targetShip;
            reach = ((10.0f - shipClass[ship->type].turnRate) * 30.0f + 50.0f);
            if (ShipIsInertialess(ship))
                reach *= 4;
            if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) <= reach
                    && FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) <= reach)
                ship->aiSubstate = kSubBoard;
            else
                ship->aiSubstate = kSubChasSlw;
        }
    }

    if (ship->aiState == kAIStateAtkAster) {
        reach = ((10.0f - shipClass[ship->type].turnRate) * 15.0f);
        if (asteroidData->active == 0) {
            ship->aiSubstate = kSubKillSpd;
        } else {
            if (FloatAbs(ship->pos.x - asteroidData->pos.x) <= reach
                    && FloatAbs(ship->pos.y - asteroidData->pos.y) <= reach)
                ship->aiSubstate = kSubKillAst;
            else
                ship->aiSubstate = kSubChaseAst;
        }
    }

    if (ship->aiState == kAIStateDestStel) {
        pt.x = stellarData[ship->targetStellar].pos.h;
        pt.y = stellarData[ship->targetStellar].pos.v;
        best = -1;
        for (reach = 0; reach < kMaxWeapTypes; reach++) {
            if ((weaponStats[reach].flags2 & kWeapPlanetType) != 0
                    && shipClass[ship->type].weapCount[reach] > 0
                    && WeaponHasAmmo(ship, reach)
                    && weaponStats[reach].guidance != kGuidanceBeam
                    && weaponStats[reach].guidance != kGuidanceTurretBeam
                    && weaponStats[reach].guidance <= kGuidanceFrontQuad) {
                best = reach;
                break;
            }
        }
        dist = XYFloatSquaredDist(&ship->pos, &pt);
        if (best != -1) {
            rangeSq = weaponStats[best].totalRange;
            rangeSq = rangeSq * rangeSq;
        } else {
            rangeSq = 1.0f;
        }
        if (dist > rangeSq * 0.5625) {
            ship->aiSubstate = kSubFlyStel;
        } else if (dist < rangeSq * 0.0625) {
            ship->aiSubstate = kSubFlyHypD;
        } else {
            if (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35)
                ship->aiSubstate = kSubKillSpd;
            else
                ship->aiSubstate = kSubAttkStel;
            ship->vel.x *= 0.98;
            ship->vel.y *= 0.98;
            ship->accelMag *= 0.98;
        }
    }

    if (ship->aiState == kAIStateScoop)
        ship->aiSubstate = kSubScoop;

    if (ship->aiState == kAIStateHiJack)
        ship->aiSubstate = kSubLoJack;
}

/* LowLevelAIHandler (mac _LowLevelAIHandler). STATE: MATCH 100% (3963 exact, 477
   reloc-equivalent), exe 4440 / obj 4440, byte-identical yes (17982 bytes), relocregcheck 0 of
   477. OPEN: none. Notes: Research/notes/lowlevelai.md */
// FUNCTION: EVNOVA 0x00408150
// MAC: _LowLevelAIHandler i386 0x000851da ppc 0x000901c4
void LowLevelAIHandler(ShipObj *ship)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    float outer;
    float inner;
    float ax;
    float px;
    unsigned char msg[256];
    char tmp[256];
    FloatPoint pt;
    FloatPoint scaled;
    FloatPoint aim;
    FloatPoint delta;
    FloatPoint anchor;
    short best;
    short i;
    short box;
    short bestBox;
#else
    char tmp[256];
    short bestBox;
    unsigned char msg[256];
    FloatPoint pt;
    FloatPoint delta;
    FloatPoint anchor;
    FloatPoint scaled;
    FloatPoint aim;
    short i;
    short box;
    short best;
#endif
    short angle;
    short course;
    short limit;
    long bestDist;
    long dist;
    float step;
    short leader;
    Boolean disabled;

    ship->thrust = 0.0F;
    ship->fireQueued = 0;
    if (ship->speedLimit >= 0.0)
        ship->speedLimit = ShipMaxSpeed(ship);
    disabled = IsDisabled(ship);
    ship->desiredHeading = ship->heading;

    if (ship->aiSubstate == kSubLoJack)
        AIEscortFireUnprovoked(ship);

    if (ship->aiSubstate == kSubChaseSwarm && disabled == 0) {
        leader = ship->swarmLeader;
        if (leader > 0) {
            pt.x = shipData[leader].pos.x;
            pt.y = shipData[leader].pos.y;
            Accel(shipData[leader].heading,
                  ShipMaxSpeed(ship) * 15.0F, &pt);
            ship->desiredHeading =
                CalcInterceptAngle(&ship->pos, &pt);
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) + 20.0F)
                ship->thrust = ShipAccelRate(ship);
            if (ship->aiState == kAIStateFightSh && ship->targetShip != -1)
                AIFireMissile(ship);
        } else
            ship->aiSubstate = kSubLoJack;
    }

    if (ship->aiSubstate == kSubKillSpd && disabled == 0) {
        if (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35) {
            if (ShipIsInertialess(ship) != 0)
                ship->thrust = (-ShipAccelRate(ship) * 0.5);
            else {
                pt.x = pt.y = 0.0F;
                scaled.x = (ship->vel.x * 100.0);
                scaled.y = (ship->vel.y * 100.0);
                ship->desiredHeading =
                    (CalcInterceptAngle(&pt, &scaled) + 180) % 360;
                if (abs(AngularDifference(ship->desiredHeading,
                                          ship->heading)) <
                        ShipTurnRate(ship) + 1.0F) {
                    if (FloatAbs(ship->vel.x) >= 1.75 || FloatAbs(ship->vel.y) >= 1.75)
                        ship->thrust = ShipAccelRate(ship);
                    else {
                        ship->thrust = (ShipAccelRate(ship) * 0.5);
                        ship->vel.x *= 0.94;
                        ship->vel.y *= 0.94;
                        ship->accelMag *= 0.94;
                    }
                } else if (ship->aiState == kAIStateRefuel) {
                    ship->vel.x *= 0.94;
                    ship->vel.y *= 0.94;
                    ship->accelMag *= 0.94;
                }
            }
        } else {
            ship->vel.x *= 0.95;
            ship->vel.y *= 0.95;
            ship->accelMag *= 0.95;
            ship->aiSubstate = kSubLoJack;
        }
        AIEscortFireUnprovoked(ship);
    }

    if (ship->aiSubstate == kSubFlyStel && ship->targetStellar != -1 &&
            disabled == 0) {
        pt = XYShortToFloat(stellarData[ship->targetStellar].pos);
        ship->desiredHeading =
            CalcInterceptAngle(&ship->pos, &pt);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 5.0F) {
            pt.x = ship->pos.x - stellarData[ship->targetStellar].pos.h;
            pt.y = ship->pos.y - stellarData[ship->targetStellar].pos.v;
            if (FloatAbs(pt.x) < 500.0F && FloatAbs(pt.y) < 500.0F) {
                ship->thrust = ShipAccelRate(ship);
                ship->speedLimit = (ShipMaxSpeed(ship) * 0.25);
            } else {
                ship->thrust = ShipAccelRate(ship);
                ship->speedLimit = 0.0F;
            }
        }
    }

    if (ship->aiSubstate == kSubFlyHypD && disabled == 0) {
        pt.x = pt.y = 0.0F;
        ship->desiredHeading =
            CalcInterceptAngle(&pt, &ship->pos);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 3.0F) {
            ship->thrust = ShipAccelRate(ship);
            ship->speedLimit = 0.0F;
        }
    }

    if (ship->aiSubstate == kSubHypJmp && disabled == 0) {
        pt.x = pt.y = 0.0F;
        ship->desiredHeading =
            CalcInterceptAngle(&pt, &ship->pos);
        if (ship->jumpTimer <= 0.0F) {
            ship->jumpTimer = 1.0F;
            ship->jumpTick = ReadTick();
        }
        if (ship->jumpTimer > 1.0F && ship->jumpTick > ReadTick())
            ship->jumpTick = ReadTick();
        ship->jumpTimer += gSpeedMult;
        if (ReadTick() - ship->jumpTick >=
                WarpSoundDuration() / shipClass[ship->type].mobilityFactor) {
            if (ship->specialMission != -1 &&
                    currentMissions[ship->specialMission].shipSyst != -6 &&
                    currentMissions[ship->specialMission].shipGoal == 6) {
                currentMissions[ship->specialMission].specialShipsJumpedOut++;
                if (currentMissions[ship->specialMission].shipCount > 0)
                    currentMissions[ship->specialMission].shipCount--;
            }
            ship->jumpTimer = 0.0F;
            ship->active = 0;
            ship->curSystem = -1;
        }
    }

    if (ship->aiSubstate == kSubJmpPar && disabled == 0) {
        ship->desiredHeading = shipData[ship->parentShip].heading;
        if (shipData[ship->parentShip].jumpTimer <= 1.0F) {
            ship->vel = shipData[ship->parentShip].vel;
            AIMaintainFormation(ship, 0);
            ship->engineGlow = shipData[ship->parentShip].engineGlow;
        } else {
            if (ship->jumpTimer > 1.0F && ship->jumpTick > ReadTick())
                ship->jumpTick = ReadTick();
            if (ship->jumpTimer > 30.0F && ship->parentShip != 0) {
                ship->desiredHeading =
                    shipData[ship->parentShip].desiredHeading;
                ship->parentShip = -1;
                ship->aiType = shipClass[ship->type].inherentAI;
                ship->aiState = kAIStateHypOut;
                ship->aiSubstate = kSubHypJmp;
            }
            if (abs(AngularDifference(
                    shipData[ship->parentShip].heading,
                    shipData[ship->parentShip].desiredHeading)) > 10) {
                if (abs(AngularDifference(
                        shipData[ship->parentShip].heading,
                        shipData[ship->parentShip].desiredHeading)) >
                        ShipTurnRate(ship)) {
                    ship->desiredHeading =
                        shipData[ship->parentShip].desiredHeading;
                    ship->jumpTimer = -1.0F;
                } else
                    ship->aiTimer = 180.0F;
            } else {
                ship->targetStellar = shipData[ship->parentShip].targetStellar;
                if (ship->jumpTimer == 0.0F) {
                    ship->jumpTimer = 1.0F;
                    ship->jumpTick = ReadTick();
                }
                ship->vel.x *= 0.95;
                ship->vel.y *= 0.95;
                ship->thrust = 0.0F;
                ship->speedLimit = -1.0F;
                ship->jumpTimer += 1.0F;
            }
        }
    }

    if (ship->aiSubstate == kSubRunAwy && ship->targetShip != -1 &&
            disabled == 0) {
        leader = ship->swarmLeader;
        if (leader > 0) {
            AIMaintainFormation(ship, 0);
            ship->engineGlow = shipData[leader].engineGlow;
        }
        ship->desiredHeading =
            CalcInterceptAngle(&shipData[ship->targetShip].pos,
                               &ship->pos);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 20.0F) {
            ship->thrust = ShipAccelRate(ship);
            ship->speedLimit = 0.0F;
        }
        if ((int)ship->aiState == kAIStateDefRet || (int)ship->aiState == kAIStateFightSh)
            AIFireTurret(ship);
        if (ship->hasAfterburner != 0 &&
                (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) < 165.0F ||
                 FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) < 165.0F))
            ship->aiSubstate = kSubAfterBurn;
    }

    if (ship->aiSubstate == kSubTF && ship->targetShip != -1 &&
            disabled == 0) {
        if (ship->selectedWeapon != -1) {
            if (weaponStats[ship->selectedWeapon].guidance == kGuidanceUnguided ||
                    weaponStats[ship->selectedWeapon].guidance == kGuidanceRocket)
                ship->desiredHeading = CalcLeadAngle(
                    ship, &shipData[ship->targetShip], ship->selectedWeapon,
                    &ship->pos);
            else
                ship->desiredHeading = CalcInterceptAngle(
                    &ship->pos,
                    &shipData[ship->targetShip].pos);
        } else {
            if (ship->aimWeapon != -1)
                ship->desiredHeading = CalcLeadAngle(
                    ship, &shipData[ship->targetShip], ship->aimWeapon,
                    &ship->pos);
            else
                ship->desiredHeading = CalcInterceptAngle(
                    &ship->pos,
                    &shipData[ship->targetShip].pos);
        }
        AIFireTurret(ship);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 15.0F) {
            ship->thrust = ShipAccelRate(ship);
            ship->speedLimit = 0.0F;
        }
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) * 3.0F) {
            AIFireGun(ship, 0);
            if (ShipIsInertialess(ship) != 0 &&
                    FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) < 100.0F &&
                    FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) < 100.0F &&
                    ship->accelMag > shipData[ship->targetShip].accelMag)
                ship->speedLimit = shipData[ship->targetShip].accelMag;
        }
        if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) < 165.0F ||
                FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) < 165.0F ||
                ShipIsInertialess(ship) == 0) {
            if ((ship->targetShip != 0 && Rand(2) == 0) || AIPerformsComplexManeuvers(ship) != 0) {
                i = 123;
                if (shipClass[ship->type].inherentAI >= 3 &&
                        shipClass[ship->type].mass < 200 &&
                        abs(ship->pos.x - shipData[ship->targetShip].pos.x) < i &&
                        abs(ship->pos.y - shipData[ship->targetShip].pos.y) < i &&
                        (ship->index > ship->targetShip ||
                         (ship->index < ship->targetShip &&
                          shipClass[ship->type].mass <
                            shipClass[shipData[ship->targetShip].type].mass))) {
                    angle = CalcInterceptAngle(
                        &ship->pos,
                        &shipData[ship->targetShip].pos);
                    if (AngularDifference(ship->heading, angle) <= 30 &&
                            AngularDifference(
                                shipData[ship->targetShip].heading,
                                ((angle + 180) % 360)) <= 30) {
                        ship->aiSubstate = kSubVeerOff;
                        if (ship->index % 2 == 0)
                            ship->veerHeading = ship->heading + 135.0F;
                        else
                            ship->veerHeading = ship->heading - 135.0F;
                        if (ship->veerHeading < 0)
                            ship->veerHeading += 360;
                        if (ship->veerHeading >= 360)
                            ship->veerHeading -= 360;
                        if (ship->desiredHeading < 0)
                            ship->desiredHeading += 360;
                        if (ship->desiredHeading >= 360)
                            ship->desiredHeading -= 360;
                    }
                }
            }
        }
        if (ship->hasAfterburner != 0) {
            i = 82;
            if (abs(ship->pos.x - shipData[ship->targetShip].pos.x) > i &&
                    abs(ship->pos.y - shipData[ship->targetShip].pos.y) > i) {
                angle = CalcInterceptAngle(
                    &ship->pos,
                    &shipData[ship->targetShip].pos);
                if (AngularDifference(ship->heading, angle) <= 30)
                    ship->aiSubstate = kSubAfterBurn;
            }
        }
    }

    if (ship->aiSubstate == kSubVeerOff && ship->targetShip != -1 &&
            disabled == 0) {
        ship->desiredHeading = ship->veerHeading;
        ship->thrust = (ShipAccelRate(ship) * 1.5);
        ship->speedLimit = 0.0F;
        if (abs(ship->pos.x - shipData[ship->targetShip].pos.x) < 165 &&
                abs(ship->pos.y - shipData[ship->targetShip].pos.y) < 165)
            AIFireTurret(ship);
        if (ShipIsInertialess(ship) != 0) {
            if (abs(ship->pos.x - shipData[ship->targetShip].pos.x) > 165 ||
                    abs(ship->pos.y - shipData[ship->targetShip].pos.y) > 165)
                ship->aiSubstate = kSubTF;
        } else {
            if (AngularDifference(ship->heading, ship->veerHeading) <
                    ShipTurnRate(ship) * 3.0F)
                ship->aiSubstate = kSubTF;
        }
    }

    if (ship->aiSubstate == kSubAfterBurn && ship->targetShip != -1 &&
            disabled == 0) {
        ship->thrust = (ShipAccelRate(ship) * 2.75);
        ship->speedLimit = (ShipMaxSpeed(ship) * 1.8);
        ship->desiredHeading = CalcInterceptAngle(
            &ship->pos,
            &shipData[ship->targetShip].pos);
        leader = ship->swarmLeader;
        if (leader > 0) {
            AIMaintainFormation(ship, 0);
            ship->engineGlow = shipData[leader].engineGlow;
        }
        if (abs(ship->pos.x - shipData[ship->targetShip].pos.x) < 165 &&
                abs(ship->pos.y - shipData[ship->targetShip].pos.y) < 165) {
            AIFireTurret(ship);
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) * 3.0F)
                AIFireGun(ship, 0);
        } else {
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) * 3.0F)
                AIFireMissile(ship);
        }
        if ((abs(ship->pos.x - shipData[ship->targetShip].pos.x) <= 165 &&
                abs(ship->pos.y - shipData[ship->targetShip].pos.y) <= 165) ||
                Rand(100) == 0)
            ship->aiSubstate = kSubTF;
    }

    if (ship->aiSubstate == kSubMissile && ship->targetShip != -1 &&
            disabled == 0) {
        leader = ship->swarmLeader;
        if (leader > 0) {
            AIMaintainFormation(ship, 0);
            ship->engineGlow = shipData[leader].engineGlow;
        }
        if (ship->aimWeapon != -1)
            ship->desiredHeading = CalcLeadAngle(
                ship, &shipData[ship->targetShip], ship->selectedWeapon,
                &ship->pos);
        else
            ship->desiredHeading = CalcInterceptAngle(
                &ship->pos,
                &shipData[ship->targetShip].pos);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) * 3.0F) {
            AIFireGun(ship, 0);
            AIFireTurret(ship);
        }
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) * 4.0F) {
            ship->thrust = ShipAccelRate(ship);
            ship->speedLimit = 0.0F;
            AIFireMissile(ship);
        }
        if (ship->hasAfterburner != 0) {
            i = 82;
            if (abs(ship->pos.x - shipData[ship->targetShip].pos.x) > i &&
                    abs(ship->pos.y - shipData[ship->targetShip].pos.y) > i) {
                angle = CalcInterceptAngle(
                    &ship->pos,
                    &shipData[ship->targetShip].pos);
                if (AngularDifference(ship->heading, angle) <= 30)
                    ship->aiSubstate = kSubAfterBurn;
            }
        }
    }

    if (ship->aiSubstate == kSubBoard && ship->targetStellar != -1 &&
            disabled == 0) {
        delta.x = ship->vel.x - shipData[ship->targetStellar].vel.x;
        delta.y = ship->vel.y - shipData[ship->targetStellar].vel.y;
        if (FloatAbs(delta.x) >= 0.35 * 1.5 || FloatAbs(delta.y) >= 0.35 * 1.5) {
            if (ShipIsInertialess(ship)) {
                ship->thrust = 0.0F;
                ship->speedLimit = 0.0F;
                if (ship->accelMag > 0.0) {
                    ship->accelMag = (ship->accelMag -
                        ShipAccelRate(ship) * gSpeedMult);
                    if (ship->accelMag < 0.0)
                        ship->accelMag = 0.0F;
                }
            } else {
                pt.x = pt.y = 0.0F;
                scaled.x = (delta.x * 100.0);
                scaled.y = (delta.y * 100.0);
                ship->desiredHeading =
                    (CalcInterceptAngle(&pt, &scaled) + 180) % 360;
                if (abs(AngularDifference(ship->desiredHeading,
                                          ship->heading)) <
                        ShipTurnRate(ship) + 1.0F) {
                    ship->thrust = ShipAccelRate(ship);
                    ship->speedLimit = 0.0F;
                }
                if (FloatAbs(delta.x) <= 1.75 || FloatAbs(delta.y) <= 1.75) {
                    delta.x *= 0.95;
                    delta.y *= 0.95;
                    ship->vel.x = shipData[ship->targetStellar].vel.x + delta.x;
                    ship->vel.y = shipData[ship->targetStellar].vel.y + delta.y;
                }
            }
        } else {
            ship->desiredHeading =
                shipData[ship->targetStellar].heading;
            ship->vel.x = shipData[ship->targetStellar].vel.x;
            ship->vel.y = shipData[ship->targetStellar].vel.y;
            pt.x = FloatAbs(ship->pos.x -
                shipData[ship->targetStellar].pos.x);
            pt.y = FloatAbs(ship->pos.y -
                shipData[ship->targetStellar].pos.y);
            if (pt.x <= 3.0F && pt.y <= 3.0F) {
                if (ship->aiTimer <= 100.0F && ship->aiTimer > 0.0F) {
                    if ((int)ship->aiState == kAIStateRepair) {
                        shipData[ship->targetStellar].boarded = 0;
                        while (IsDisabled(&shipData[ship->targetStellar]) != 0)
                            shipData[ship->targetStellar].armor += 1.0F;
                        if (ship->targetStellar == 0) {
                            ST_PlaySound(beepSnd[1], 1, gSoundVolume);
                            strncpy((char *)msg, (char *)shipClass[ship->type].name,
                                    0x3e);
                            PtoCstr(msg);
                            strcat((char *)msg, ":  ");
                            StrcatIndCString((char *)msg, kGameStrList, 384);
                            strcat((char *)msg, ", ");
                            strncpy(tmp, (char *)shipName, 0x3e);
                            PtoCstr((unsigned char *)tmp);
                            strcat((char *)msg, tmp);
                            strcat((char *)msg, ".");
                            CtoPstr((char *)msg);
                            DisplayComm((PStrChar *)msg, 240, 0, 12, &rgbWhite, 0, 0);
                            statusRedraw[1] = 1;
                            statusRedraw[4] = 1;
                        }
                        ship->aiTimer = 100.0F;
                        ship->aiState = kAIStateUnPlunder;
                        ship->defenseStellar = -1;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        ship->targetStellar = -1;
                    } else if ((int)ship->aiState == kAIStatePlunder) {
                        ship->aiTimer = 100.0F;
                        ship->aiState = kAIStateUnPlunder;
                        ship->aiSubstate = kSubLoJack;
                        ship->defenseStellar = -1;
                        AIPlunderShipContents(ship,
                                              &shipData[ship->targetStellar]);
                        ship->targetShip = -1;
                        ship->targetStellar = -1;
                    }
                } else if (ship->aiTimer <= 0.0F && ship->aiState != kAIStateUnPlunder) {
                    shipData[ship->targetStellar].boarded = 1;
                    ship->aiTimer = (Rand(80) + 100);
                    ship->vel.x = shipData[ship->targetStellar].vel.x;
                    ship->vel.y = shipData[ship->targetStellar].vel.y;
                }
            } else {
                ship->vel.x = shipData[ship->targetStellar].vel.x;
                ship->vel.y = shipData[ship->targetStellar].vel.y;
                ship->accelMag = 0.0F;
                pt.x = ship->pos.x;
                pt.y = ship->pos.y;
                scaled.x = shipData[ship->targetStellar].pos.x;
                scaled.y = shipData[ship->targetStellar].pos.y;
                Accel(CalcInterceptAngle(&pt, &scaled), gSpeedMult,
                      &ship->pos);
            }
        }
    }

    if (ship->aiSubstate == kSubWaitTarg && ship->targetShip != -1 &&
            disabled == 0) {
        if (FloatAbs(ship->vel.x) >= 0.35 || FloatAbs(ship->vel.y) >= 0.35) {
            if (ShipIsInertialess(ship) != 0)
                ship->thrust = -ShipAccelRate(ship);
            else {
                pt.x = pt.y = 0.0F;
                scaled.x = (ship->vel.x * 100.0);
                scaled.y = (ship->vel.y * 100.0);
                if (ship->aimWeapon != -1)
                    ship->desiredHeading = CalcLeadAngle(
                        ship, &shipData[ship->targetShip],
                        ship->aimWeapon, &ship->pos);
                else
                    ship->desiredHeading =
                        (CalcInterceptAngle(&pt, &scaled) + 180) % 360;
                if (abs(AngularDifference(ship->desiredHeading,
                                          ship->heading)) <
                        ShipTurnRate(ship) + 1.0F)
                    ship->thrust = ShipAccelRate(ship);
            }
        } else {
            ship->vel.x *= 0.95;
            ship->vel.y *= 0.95;
            ship->desiredHeading = CalcInterceptAngle(
                &ship->pos,
                &shipData[ship->targetShip].pos);
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) * 3.0F) {
                AIFireGun(ship, 0);
                AIFireMissile(ship);
            }
            AIFireTurret(ship);
        }
        AILaunchFighter(ship);
    }

    if (ship->aiSubstate == kSubChasSlw &&
            (ship->targetShip != -1 || ship->targetStellar != -1) &&
            disabled == 0) {
        leader = ship->swarmLeader;
        if (leader > 0) {
            AIMaintainFormation(ship, 0);
            ship->engineGlow = shipData[leader].engineGlow;
        }
        if (ship->targetShip >= 0)
            i = ship->targetShip;
        else if (ship->targetStellar >= 0)
            i = ship->targetStellar;
        angle = CalcInterceptAngle(&ship->pos,
                                   &shipData[i].pos);
        if (FloatAbs(ship->pos.x - shipData[i].pos.x) <= 200.0F &&
                FloatAbs(ship->pos.y - shipData[i].pos.y) <= 200.0F) {
            pt.x = pt.y = 0.0F;
            scaled.x = (ship->vel.x * 100.0);
            scaled.y = (ship->vel.y * 100.0);
            course = CalcInterceptAngle(&pt, &scaled);
            if (abs(course - angle) > 15) {
                aim.x = aim.y = 0.0F;
                Accel(angle, ShipMaxSpeed(ship), &aim);
                delta.x = aim.x - ship->vel.x;
                delta.y = aim.y - ship->vel.y;
                if (FloatAbs(delta.x) > 0.35 || FloatAbs(delta.y) > 0.35)
                    ship->desiredHeading = CalcInterceptAngle(&pt, &delta);
            }
        } else
            ship->desiredHeading = angle;
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 1.0F) {
            ship->thrust = ShipAccelRate(ship);
            if (FloatAbs(ship->pos.x - shipData[i].pos.x) <= 100.0F &&
                    FloatAbs(ship->pos.y - shipData[i].pos.y) <= 100.0F)
                ship->speedLimit = (ShipMaxSpeed(ship) * 0.5);
            else
                ship->speedLimit = 0.0F;
        }
        AIEscortFireUnprovoked(ship);
    }

    if (ship->aiSubstate == kSubChase) {
        if ((ship->targetShip != -1 || ship->targetStellar != -1) &&
                disabled == 0) {
            if (ship->targetShip >= 0)
                i = ship->targetShip;
            else if (ship->targetStellar >= 0)
                i = ship->targetStellar;
            angle = CalcInterceptAngle(&ship->pos,
                                       &shipData[i].pos);
            if (FloatAbs(ship->pos.x - shipData[i].pos.x) <= 200.0F &&
                    FloatAbs(ship->pos.y - shipData[i].pos.y) <= 200.0F) {
                pt.x = pt.y = 0.0F;
                scaled.x = (ship->vel.x * 100.0);
                scaled.y = (ship->vel.y * 100.0);
                course = CalcInterceptAngle(&pt, &scaled);
                if (abs(course - angle) > 15) {
                    aim.x = aim.y = 0.0F;
                    Accel(angle, ShipMaxSpeed(ship), &aim);
                    delta.x = aim.x - ship->vel.x;
                    delta.y = aim.y - ship->vel.y;
                    if (FloatAbs(delta.x) > 0.35 || FloatAbs(delta.y) > 0.35)
                        ship->desiredHeading =
                            CalcInterceptAngle(&pt, &delta);
                }
            } else
                ship->desiredHeading = angle;
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) + 1.0F) {
                ship->thrust = ShipAccelRate(ship);
                ship->speedLimit = 0.0F;
            }
            AIEscortFireUnprovoked(ship);
        }
    }

    if (ship->aiSubstate == kSubFormFly && ship->targetStellar != -1 &&
            disabled == 0) {
        ship->aiSubstate = kSubLoJack;
        delta.x = ship->vel.x - shipData[ship->targetStellar].vel.x;
        delta.y = ship->vel.y - shipData[ship->targetStellar].vel.y;
        if (ShipIsInertialess(ship) == 0 &&
                (FloatAbs(delta.x) >= 0.35 * 1.5 || FloatAbs(delta.y) >= 0.35 * 1.5)) {
            pt.x = pt.y = 0.0F;
            scaled.x = (delta.x * 100.0);
            scaled.y = (delta.y * 100.0);
            ship->desiredHeading =
                (CalcInterceptAngle(&pt, &scaled) + 180) % 360;
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) + 1.0F) {
                ship->thrust = (ShipAccelRate(ship) * 0.66);
                ship->speedLimit = 0.0F;
            }
            if (FloatAbs(delta.x) <= 1.75 || FloatAbs(delta.y) <= 1.75) {
                delta.x *= 0.95;
                delta.y *= 0.95;
                ship->vel.x = shipData[ship->targetStellar].vel.x + delta.x;
                ship->vel.y = shipData[ship->targetStellar].vel.y + delta.y;
            }
        } else {
            ship->vel.x = shipData[ship->targetStellar].vel.x;
            ship->vel.y = shipData[ship->targetStellar].vel.y;
            i = abs(AngularDifference(
                ship->heading,
                shipData[ship->targetStellar].heading));
            limit = 25;
            if (ship->skill > 0.0F)
                limit /= ship->skill;
            if (i > 0 && i < limit) {
                ship->desiredHeading = ship->heading;
                best = (ship->heading -
                    shipData[ship->targetStellar].heading);
                if (best >= 360)
                    best -= 360;
                if (best < 0)
                    best += 360;
                if (best > 180)
                    ship->desiredHeading += 1;
                else
                    ship->desiredHeading -= 1;
                if (ship->desiredHeading >= 360)
                    ship->desiredHeading -= 360;
                if (ship->desiredHeading < 0)
                    ship->desiredHeading += 360;
            } else
                ship->desiredHeading =
                    shipData[ship->targetStellar].heading;
            if (ship->aiState == kAIStateInspect && ship->targetStellar != -1) {
                step = (ShipAccelRate(ship) * 10.0 * gSpeedMult);
                anchor = shipData[ship->targetStellar].pos;
                Accel((short)(shipData[ship->targetStellar].heading +
                              135.0F) % 360,
                      48.0F, &anchor);
                if (ship->pos.x <= anchor.x - step)
                    ship->pos.x += step;
                else if (ship->pos.x >= anchor.x + step)
                    ship->pos.x = ship->pos.x - step;
                if (ship->pos.y <= anchor.y - step)
                    ship->pos.y += step;
                else if (ship->pos.y >= anchor.y + step)
                    ship->pos.y = ship->pos.y - step;
            } else
                AIMaintainFormation(ship, 0);
            ship->engineGlow = shipData[ship->targetStellar].engineGlow;
        }
        ship->aiSubstate = kSubFormFly;
        AIEscortFireUnprovoked(ship);
    }

    if (ship->aiSubstate == kSubDock) {
        if (ship->targetStellar != -1 && IsDying(ship) == 0 &&
                IsDisabled(ship) == 0) {
            if (shipData[ship->targetStellar].active != 0 &&
                    shipData[ship->targetStellar].curSystem == ship->curSystem) {
                if (disabled == 0) {
                    ship->desiredHeading = CalcInterceptAngle(
                        &ship->pos,
                        &shipData[ship->targetStellar].pos);
                    if (abs(AngularDifference(ship->desiredHeading,
                                              ship->heading)) <
                            ShipTurnRate(ship) + 1.0F) {
                        ship->thrust = ShipAccelRate(ship);
                        ship->speedLimit = 0.0F;
                    }
                }
                best = GetMasterShipSpriteXSize(
                    shipData[ship->targetStellar].type);
                if (FloatAbs(shipData[ship->targetStellar].pos.x - ship->pos.x) <= best
                        && FloatAbs(shipData[ship->targetStellar].pos.y - ship->pos.y) <= best) {
                    if (ship->index != 0)
                        bestBox = 1;
                    else if (CanBuyFighter(ship->type, -1, -1) != 0)
                        bestBox = 1;
                    else
                        bestBox = 0;
                    if (bestBox != 1) {
                        ship->aiState = kAIStateProtect;
                        ship->aiSubstate = kSubLoJack;
                        ship->targetShip = -1;
                        ship->targetStellar = -1;
                    } else
                        AIAddFighterToParent(ship);
                } else {
                    step = (ShipAccelRate(ship) * 10.0 * gSpeedMult);
                    anchor = shipData[ship->parentShip].pos;
                    if (ship->pos.x <= anchor.x - step)
                        ship->pos.x += step;
                    else if (ship->pos.x >= anchor.x + step)
                        ship->pos.x = ship->pos.x - step;
                    if (ship->pos.y <= anchor.y - step)
                        ship->pos.y += step;
                    else if (ship->pos.y >= anchor.y + step)
                        ship->pos.y = ship->pos.y - step;
                    ship->speedLimit = ship->speedLimit - step;
                }
            } else {
                ship->aiType = shipClass[ship->type].inherentAI;
                ship->aiState = kAIStateHiJack;
            }
            goto doneFollow;
        }
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
    }
doneFollow:

    if (ship->aiSubstate == kSubZoom) {
        if (ship->speedLimit >= 0.0)
            ship->speedLimit = -50.0F;
        ship->thrust = -1.165F;
    }

    if (ship->aiSubstate == kSubChaseAst) {
        ship->desiredHeading = CalcInterceptAngle(
            &ship->pos, &asteroidData->pos);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 15.0F)
            ship->thrust = ShipAccelRate(ship);
    }

    if (ship->aiSubstate == kSubKillAst) {
        step = (ShipAccelRate(ship) * 1.5 * gSpeedMult);
        if (ship->vel.x >= asteroidData->vel.x + step)
            ship->vel.x -= step;
        else if (ship->vel.x <= asteroidData->vel.x - step)
            ship->vel.x = ship->vel.x + step;
        else
            ship->vel.x = asteroidData->vel.x;
        if (ship->vel.y >= asteroidData->vel.y + step)
            ship->vel.y -= step;
        else if (ship->vel.y <= asteroidData->vel.y - step)
            ship->vel.y = ship->vel.y + step;
        else
            ship->vel.y = asteroidData->vel.y;
#ifdef MAC1010 /* 1.0.10 spelling */
        px = ship->pos.x;
        ax = asteroidData->pos.x;
        outer = 150.0F;
        if (px > ax + outer)
            ship->pos.x = px - step;
        else if (px < ax - outer)
            ship->pos.x = px + step;
        px = ship->pos.y;
        ax = asteroidData->pos.y;
        outer = 150.0F;
        if (px > ax + outer)
            ship->pos.y = px - step;
        else if (px < ax - outer)
            ship->pos.y = px + step;
        if (ship->pos.x > asteroidData->pos.x &&
                ship->pos.x < asteroidData->pos.x + (inner = 80.0F))
            ship->pos.x = ship->pos.x + step;
        else if (ship->pos.x < asteroidData->pos.x &&
                ship->pos.x > asteroidData->pos.x - 80.0F)
            ship->pos.x -= step;
        if (ship->pos.y > asteroidData->pos.y &&
                ship->pos.y < asteroidData->pos.y + (inner = 80.0F))
            ship->pos.y = ship->pos.y + step;
        else if (ship->pos.y < asteroidData->pos.y &&
                ship->pos.y > asteroidData->pos.y - 80.0F)
            ship->pos.y -= step;
#else
        if (ship->pos.x > 150.0F + asteroidData->pos.x)
            ship->pos.x = ship->pos.x - step;
        else if (ship->pos.x < asteroidData->pos.x - 150.0F)
            ship->pos.x = ship->pos.x + step;
        if (ship->pos.y > 150.0F + asteroidData->pos.y)
            ship->pos.y = ship->pos.y - step;
        else if (ship->pos.y < asteroidData->pos.y - 150.0F)
            ship->pos.y = ship->pos.y + step;
        if (ship->pos.x > asteroidData->pos.x &&
                ship->pos.x < 80.0F + asteroidData->pos.x)
            ship->pos.x = ship->pos.x + step;
        else if (ship->pos.x < asteroidData->pos.x &&
                ship->pos.x > asteroidData->pos.x - 80.0F)
            ship->pos.x = ship->pos.x - step;
        if (ship->pos.y > asteroidData->pos.y &&
                ship->pos.y < 80.0F + asteroidData->pos.y)
            ship->pos.y = ship->pos.y + step;
        else if (ship->pos.y < asteroidData->pos.y &&
                ship->pos.y > asteroidData->pos.y - 80.0F)
            ship->pos.y = ship->pos.y - step;
#endif
        ship->desiredHeading = CalcObjectLeadAngle(
            ship, &asteroidData->pos,
            &asteroidData->vel, ship->selectedWeapon,
            &ship->pos);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 10.0F) {
            ship->targetShip = -1;
            AIFireGunUntargeted(ship);
        }
    }

    if (ship->aiSubstate == kSubScoop) {
        bestBox = -1;
        bestDist = 0;
        for (box = 0; box < kMaxBoxes; box++) {
            if (boxData[box].timer > 0.0F && boxData[box].mining != 0) {
                dist = XYFloatSquaredDist(&ship->pos,
                                                &boxData[box].pos);
                if (dist < bestDist || bestBox == -1) {
                    bestBox = box;
                    bestDist = dist;
                }
            }
        }
        if (bestBox != -1) {
            ship->desiredHeading = CalcInterceptAngle(
                &ship->pos,
                &boxData[bestBox].pos);
            if (abs(AngularDifference(ship->desiredHeading,
                                      ship->heading)) <
                    ShipTurnRate(ship) + 15.0F)
                ship->thrust = ShipAccelRate(ship);
            else {
                step = (ShipAccelRate(ship) * 1.75 * gSpeedMult);
                if (ship->vel.x >= step)
                    ship->vel.x -= step;
                else if (ship->vel.x <= -step)
                    ship->vel.x = ship->vel.x + step;
                else
                    ship->vel.x = 0.0F;
                if (ship->vel.y >= step)
                    ship->vel.y -= step;
                else if (ship->vel.y <= -step)
                    ship->vel.y = ship->vel.y + step;
                else
                    ship->vel.y = 0.0F;
            }
        } else
            ship->aiSubstate = kSubLoJack;
    }

    if (ship->aiSubstate == kSubAttkStel) {
        pt.x = stellarData[ship->targetStellar].pos.h;
        pt.y = stellarData[ship->targetStellar].pos.v;
        ship->desiredHeading =
            CalcInterceptAngle(&ship->pos, &pt);
        if (abs(AngularDifference(ship->desiredHeading,
                                  ship->heading)) <
                ShipTurnRate(ship) + 15.0F)
            AIFirePlanetGun(ship);
    }
    return;

noRoom:
    ship->aiState = kAIStateProtect;
    ship->aiSubstate = kSubLoJack;
    ship->targetShip = -1;
    ship->targetStellar = -1;
    goto doneFollow;
}

#define kNavTargetLink      3    /* ShipObj.navTargetKind: targetStellar holds a hyperlink index */

/* SelectRandomJumpDest (mac _SelectRandomJumpDest, uncalled there too): not linked, dead-stripped
   in the exe; it stays because SelectRandomStellarDest's frame is laid out after it (notes) */
// MAC: _SelectRandomJumpDest i386 0x000805ca ppc 0x0008a394
void SelectRandomJumpDest(ShipObj *ship)
{
    ship->navTargetKind = kNavTargetLink;
    ship->targetStellar = 1;
}

#define kPlayFieldLimit     0x3e8

/* SelectRandomStellarDest. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040c790
// MAC: _SelectRandomStellarDest i386 0x000805de ppc 0x0008a3a8
short SelectRandomStellarDest(ShipObj *ship, unsigned char allowSpecial, Boolean plainOnly)
{
    short special[kNavSlots];
    short landable[kNavSlots];
    Boolean enemy[kNavSlots];
    Boolean isKindA[kNavSlots];
    Boolean isKindB[kNavSlots];
    short i;
    short pick;
    short countAny;
    short countKindB;
    short countKindA;
    short countPlain;
    Boolean preferAvoid;
    Boolean preferKindA;
    Boolean preferKindB;
    short spob;
    short g;
    unsigned short f;

    countAny = 0;
    pick = -1;
    countKindA = 0;
    countKindB = 0;
    countPlain = 0;
    preferAvoid = 0;
    preferKindA = 0;
    preferKindB = 0;
    if (ship->govt != -1) {
        if ((govtData[ship->govt].flags2 & kGovtNoHypergates) != 0)
            preferAvoid = 1;
        if ((govtData[ship->govt].flags2 & kGovtPrefersHypergates) != 0)
            preferKindA = 1;
        if ((govtData[ship->govt].flags2 & kGovtPrefersWormholes) != 0)
            preferKindB = 1;
    }

    for (i = 0; i < kNavSlots; i++) {
        enemy[i] = 0;
        landable[i] = 0;
        special[i] = 0;
        isKindA[i] = 0;
        isKindB[i] = 0;
        spob = systemData[ship->curSystem].navDefault[i];
        if (spob == -1)
            continue;
        if (StellarIsLandable(&stellarData[spob])) {
            if (stellarData[systemData[ship->curSystem].navDefault[i]].pos.h < kPlayFieldLimit
                && stellarData[systemData[ship->curSystem].navDefault[i]].pos.v < kPlayFieldLimit)
                landable[i] = 1;
        }
        if ((stellarData[systemData[ship->curSystem].navDefault[i]].flags & kSpobUninhabited) != 0
            && (stellarData[systemData[ship->curSystem].navDefault[i]].flags2
                & kSpobGateOrWormhole) == 0)
            special[i] = 1;
        if (ship->govt != -1) {
            g = stellarData[systemData[ship->curSystem].navDefault[i]].govt;
            if (g != -1 && GovtEnemies(ship->govt, g))
                enemy[i] = 1;
        }
        if ((stellarData[systemData[ship->curSystem].navDefault[i]].flags2 & kSpobHypergate) != 0) {
            isKindA[i] = 1;
            if (landable[i] != 0 && enemy[i] == 0)
                countKindA++;
        }
        if ((stellarData[systemData[ship->curSystem].navDefault[i]].flags2 & kSpobWormhole) != 0) {
            isKindB[i] = 1;
            if (landable[i] != 0 && enemy[i] == 0)
                countKindB++;
        }
        if (landable[i] != 0 && special[i] == 0 && enemy[i] == 0) {
            countAny++;
            if (isKindA[i] == 0 && isKindB[i] == 0)
                countPlain++;
        }
    }

    if (plainOnly != 0) {
        if (countPlain > 0) {
            do {
                pick = Rand(kNavSlots);
            } while (landable[pick] == 0 || isKindB[pick] != 0
                     || isKindA[pick] != 0 || enemy[pick] != 0);
        }
    } else if (preferKindB != 0 && countKindB > 0) {
        do {
            pick = Rand(kNavSlots);
        } while (landable[pick] == 0 || isKindB[pick] == 0 || enemy[pick] != 0);
    } else if (preferKindA != 0 && countKindA > 0) {
        do {
            pick = Rand(kNavSlots);
        } while (landable[pick] == 0 || isKindA[pick] == 0 || enemy[pick] != 0);
    } else if (countAny > 0 && allowSpecial != 0 && preferAvoid == 0
               && (countPlain > 0 || countKindA > 0)) {
        do {
            pick = Rand(kNavSlots);
        } while (special[pick] != 0 || landable[pick] == 0
                 || enemy[pick] != 0 || isKindB[pick] != 0);
    } else if (countAny > 0
               && (countPlain > 0 || (countKindA > 0 && preferAvoid == 0))) {
        do {
            pick = Rand(kNavSlots);
        } while (special[pick] != 0 || landable[pick] == 0 || enemy[pick] != 0
                 || (isKindB[pick] != 0 && preferKindB == 0)
                 || (isKindA[pick] != 0 && preferAvoid != 0));
    }

    if (pick != -1)
        return systemData[ship->curSystem].navDefault[pick];
    return -1;
}

/* SelectNearestAnyStellar (mac _SelectNearestAnyStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040cc10
// MAC: _SelectNearestAnyStellar i386 0x0007fb00 ppc 0x00089750
short SelectNearestAnyStellar(ShipObj *ship, short exclude)
{
    FloatPoint pos;
    short i;
    short chosen;
    float bestDist;
    float dist;
    Boolean ok;
    short stellarGovt;
    short shipGovt;

    chosen = -1;
    for (i = 0; i < kNavSlots; i++) {
        if (systemData[ship->curSystem].navDefault[i] == -1)
            continue;
        ok = 1;
        shipGovt = ship->govt;
        if (shipGovt != -1) {
            stellarGovt = stellarData[systemData[ship->curSystem].navDefault[i]].govt;
            if (stellarGovt != -1 && GovtEnemies(shipGovt, stellarGovt))
                ok = 0;
        }
        if (stellarData[systemData[ship->curSystem].navDefault[i]].flags2 & kSpobGateOrWormhole)
            ok = 0;
        if (ok && exclude != systemData[ship->curSystem].navDefault[i]) {
            pos.x = stellarData[systemData[ship->curSystem].navDefault[i]].pos.h;
            pos.y = stellarData[systemData[ship->curSystem].navDefault[i]].pos.v;
            dist = XYFloatSquaredDist(&ship->pos, &pos);
            if (dist < bestDist || chosen == -1) {
                chosen = systemData[ship->curSystem].navDefault[i];
                bestDist = dist;
            }
        }
    }
    return chosen;
}

/* AIShouldLeaveSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040cd80
Boolean AIShouldLeaveSystem(ShipObj *ship, short sys)
{
    short i;
    short count;

    count = 0;
    for (i = 0; i < kNavSlots; i++) {
        if (systemData[sys].navDefault[i] != -1)
            count++;
    }
    if (count == 0)
        return 1;
    for (i = 0; i < kNavSlots; i++) {
        if (ship->lastStellar == systemData[sys].navDefault[i] &&
            systemData[sys].navDefault[i] != -1)
            return 1;
    }
    return 0;
}

/* AIFireTurret. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040ce00
// MAC: _AIFireTurret i386 0x00080ad3 ppc 0x0008a8f8
void AIFireTurret(ShipObj *ship)
{
    short k;
    short bearing;
    short angle;
    Boolean ok;
    short cand[2];
    short best[2];
    short weap;
    short j;
    short strength;
    float d;

    cand[0] = cand[1] = -1;
    best[0] = best[1] = 0;
    if (IsDisabled(ship))
        return;
    HandleShipPointDefense(ship);
    if (ship->targetShip == -1)
        return;
    if (!shipData[ship->targetShip].active)
        return;
    if (shipClass[shipData[ship->targetShip].type].flags2 & kShipUntargetable)
        return;
    if (!ShipVisibleToShip(&shipData[ship->targetShip], ship))
        return;
    for (k = 0; k < 256; k++) {
        if (ship->weap[k].count > 0 && ship->weap[k].reloadTimer <= 0.0f) {
            if (weaponStats[k].guidance == 3 || weaponStats[k].guidance == 4
                || weaponStats[k].guidance == 7 || weaponStats[k].guidance == 8) {
                if ((shipClass[shipData[ship->targetShip].type].flags
                     & kShipPlanetType)
                    == (weaponStats[k].flags2 & kWeapPlanetType)) {
                    if (weaponStats[k].guidance == 7 || weaponStats[k].guidance == 8) {
                        bearing = CalcInterceptAngle(&ship->pos,
                                               &shipData[ship->targetShip].pos);
                        ok = 0;
#ifdef MACBUILD
                        if (weaponStats[k].guidance == kGuidanceFrontQuad)
                            angle = abs((bearing - ship->heading)) % 360;
                        if (weaponStats[k].guidance == kGuidanceRearQuad)
                            angle = abs(bearing - ((short)ship->heading + 180) % 360) % 360;
#else
                        if (weaponStats[k].guidance == kGuidanceFrontQuad) {
                            d = fabs(bearing - ship->heading);
                            angle = (short)d % 360;
                        }
                        if (weaponStats[k].guidance == kGuidanceRearQuad) {
                            angle = (short)abs(bearing
                                                  - ((short)ship->heading + 180) % 360) % 360;
                        }
#endif
                        if (angle <= 0x2d)
                            ok = 1;
                    } else
                        ok = 1;
                    if (TurretBlindSpot(ship, bearing, k))
                        ok = 0;
                    if (ok)
                        ok = AIInGunRange(ship, &shipData[ship->targetShip], k);
                    if (ok && WeaponHasAmmo(ship, k)) {
                        for (j = 0; j <= 1; j++) {
                            strength = weaponStats[k].damage[j];
                            if (strength <= 0)
                                strength = 1;
                            if (strength > best[j]) {
                                best[j] = strength;
                                cand[j] = k;
                            }
                        }
                    }
                }
            }
        }
    }
    if (shipData[ship->targetShip].shield >= 0.0f)
        weap = cand[1];
    else
        weap = cand[0];
    if (weap != -1) {
        ship->selectedWeapon = weap;
        ship->fireQueued = 1;
    }
}

#define kRangeMargin     0.95

/* AIFireMissile (mac _AIFireMissile). STATE: MATCH 100.0%, exe 179 / obj 179, byte-identical
   yes. OPEN: none. */
// FUNCTION: EVNOVA 0x0040d220
// MAC: _AIFireMissile i386 0x0008115d ppc 0x0008afe4
void AIFireMissile(ShipObj *me)
{
    short i;
    short best;
    float distSq;
    float absDistSq;

    if (me->targetShip == -1)
        return;
    if (me->curSystem != shipData[me->targetShip].curSystem
        || shipData[me->targetShip].active == 0)
        return;
    if ((shipClass[shipData[me->targetShip].type].flags2 & kShipUntargetable) != 0
        && !ShipCanTargetUntargetableShips(me))
        return;
    if (ShipIsCloaked(&shipData[me->targetShip]) && !ShipCanTargetCloakedShips(me))
        return;
    if (SufficentTargetedDamage(&shipData[me->targetShip]))
        return;
    best = -1;
    distSq = XYFloatSquaredDist(&me->pos, &shipData[me->targetShip].pos);
    absDistSq = FloatAbs(distSq);
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (me->weap[i].count > 0
            && weaponStats[i].guidance == kGuidanceHoming
            && SuitableMissileType(i, &shipData[me->targetShip])
            && (shipClass[shipData[me->targetShip].type].flags & kShipPlanetType)
                   == (weaponStats[i].flags2 & kWeapPlanetType)
            && WeaponHasAmmo(me, i)
            && weaponStats[i].totalRange * weaponStats[i].totalRange
                   >= absDistSq * kRangeMargin) {
            best = i;
            break;
        }
    }
    if (best != -1 && me->weap[best].reloadTimer <= 0.0f) {
        me->selectedWeapon = best;
        me->fireQueued = 1;
    }
}

#define kDamageTypes            2
#define kRangeScale             2.5

/* AIFireGun. STATE: MATCH 100.0%, exe 229 / obj 229, byte-identical yes.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0040d470
// MAC: _AIFireGun i386 0x0007feb6 ppc 0x00089b98
void AIFireGun(ShipObj *ship, Boolean allowUnguided)
{
    short bestWeap[kDamageTypes];
    short best[kDamageTypes];
    short i;
    short j;
    short score;
    Boolean sawGuided;
    short pick;
    long dist;

    bestWeap[0] = bestWeap[1] = -1;
    best[0] = best[1] = 0;
    if (ship->targetShip == -1)
        return;
    if (ship->curSystem != shipData[ship->targetShip].curSystem
            || shipData[ship->targetShip].active == 0)
        return;
    if (ShipVisibleToShip(&shipData[ship->targetShip], ship) == 0)
        return;

    sawGuided = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count > 0
                && (weaponStats[i].guidance == kGuidanceUnguided
                    || weaponStats[i].guidance == kGuidanceBeam
                    || weaponStats[i].guidance == kGuidanceRocket
                    || (weaponStats[i].guidance == kGuidanceHoming
                        && allowUnguided != 0))
                && (shipClass[shipData[ship->targetShip].type].flags & kShipPlanetType)
                   == (weaponStats[i].flags2 & kWeapPlanetType)
                && WeaponHasAmmo(ship, i) != 0) {
            if (weaponStats[i].guidance != kGuidanceHoming)
                sawGuided = 1;
            if (ship->weap[i].reloadTimer <= 0.0f
                    && AIInGunRange(ship, &shipData[ship->targetShip], i) != 0) {
                for (j = 0; j <= kDamageTypes - 1; j++) {
                    score = weaponStats[i].damage[j];
                    if (score <= 0)
                        score = 1;
                    if (weaponStats[i].guidance == kGuidanceRocket
                            && weaponStats[i].proxRadius > 0) {
                        dist = FloatAbs(XYFloatSquaredDist(&ship->pos,
                                                           &shipData[ship->targetShip].pos));
                        if (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x)
                                    >= weaponStats[i].proxRadius * kRangeScale
                                && FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y)
                                    >= weaponStats[i].proxRadius * kRangeScale
                                && score > best[j]) {
                            best[j] = score;
                            bestWeap[j] = i;
                        }
                    }
                    else if (score > best[j]) {
                        best[j] = score;
                        bestWeap[j] = i;
                    }
                }
            }
        }
    }

    if (shipData[ship->targetShip].shield >= 0.0f)
        pick = bestWeap[1];
    else
        pick = bestWeap[0];
    if (pick >= 0)
        ship->selectedWeapon = pick;
    if (ship->selectedWeapon != -1) {
        ship->fireQueued = 1;
    }
    else if (sawGuided == 0 && allowUnguided == 0) {
        AIFireGun(ship, 1);
    }
}

/*
 * The scan keeps the LAST qualifying type rather than the first, so the highest
 * weapon index the ship can actually fire wins.
 */

/* AIFireGunUntargeted (mac _AIFireGunUntargeted). STATE: MATCH 100.0%, exe 83 / obj 83, relocregcheck 0 of 7. `guidance' is read
   from weaponStats[i] at each test; `i' is declared first. OPEN: none */
// FUNCTION: EVNOVA 0x0040d7e0
// MAC: _AIFireGunUntargeted i386 0x00080425 ppc 0x0008a164
void AIFireGunUntargeted(ShipObj *ship)
{
    short i;
    short massDmg;
    short chosen;
    short best;

    chosen = -1;
    best = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count <= 0)
            continue;
        if (weaponStats[i].guidance == kGuidanceUnguided)
            goto qualifies;
        if (weaponStats[i].guidance == kGuidanceBeam) {
            if ((weaponStats[i].seeker & 1) == 0)
                goto qualifies;
            if ((shipClass[ship->type].flags3 & kShipDestroysAsteroids) == 0)
                goto qualifies;
            goto reeval;
        }
      reeval:
        if (weaponStats[i].guidance == kGuidanceRocket)
            goto qualifies;
        if (weaponStats[i].guidance != kGuidanceFrontQuad)
            continue;
        if (ship->targetShip != -1)
            continue;
      qualifies:
        if ((weaponStats[i].flags2 & kWeapPlanetType) == 0
                && WeaponHasAmmo(ship, i)
                && ship->weap[i].reloadTimer <= 0.0f) {
            massDmg = weaponStats[i].damage[0];
            if (massDmg <= 0)
                massDmg = 1;
            if (massDmg > best) {
                best = massDmg;
                chosen = i;
            }
        }
    }
    if (chosen >= 0)
        ship->selectedWeapon = chosen;
    if (ship->selectedWeapon != -1)
        ship->fireQueued = 1;
}

/* AIFirePlanetGun (mac _AIFirePlanetGun). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040d910
// MAC: _AIFirePlanetGun i386 0x00080536 ppc 0x0008a2c0
void AIFirePlanetGun(ShipObj *ship)
{
    short i;
    short chosen;

    chosen = -1;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count > 0
                && weaponStats[i].guidance != kGuidanceBeam
                && weaponStats[i].guidance != kGuidanceTurretBeam
                && weaponStats[i].guidance < kGuidanceRearQuad && WeaponHasAmmo(ship, i) != 0
                && ship->weap[i].reloadTimer <= 0.0f)
            chosen = i;
    }
    if (chosen >= 0) {
        ship->selectedWeapon = chosen;
        ship->fireQueued = 1;
    }
}

/* AILaunchFighter (mac _AILaunchFighter). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040d9a0
// MAC: _AILaunchFighter i386 0x00081372 ppc 0x0008b224
void AILaunchFighter(ShipObj *ship)
{
    short i;
    short j;

    if (ship->targetShip == -1)
        return;
    if (ship->curSystem != shipData[ship->targetShip].curSystem || !shipData[ship->targetShip].active)
        return;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count <= 0)
            continue;
        if (ship->weap[i].ammo <= 0)
            continue;
        if (weaponStats[i].guidance != kGuidanceBay)
            continue;
        if (weaponStats[i].flags2 & kWeapFlagAIRestricted)
            continue;

        if (ship->weap[i].reloadTimer <= 0.0f) {
            ship->selectedWeapon = i;
            if (LaunchFighter(ship, i)) {
                PlaySoundDistance(-1, shotSnd[weaponStats[i].sound], 5, &ship->pos, &shipData[0].pos);
                ship->weap[i].reloadTimer = weaponStats[i].reload / ship->weap[i].count;
                ship->weap[i].ammo -= 1;
                if (weaponStats[i].flags3 & kWeapExclusive) {
                    for (j = 0; j < kMaxWeapTypes; j++) {
                        if (j == i)
                            continue;
                        if (ship->weap[j].reloadTimer < ship->weap[i].reloadTimer + 2.0f)
                            ship->weap[j].reloadTimer = ship->weap[i].reloadTimer + 2.0f;
                    }
                }
            }
        }
        break;
    }
}

/* DrawHighGoalAbbreviation (mac _DrawHighGoalAbbreviation). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040db70
// MAC: _DrawHighGoalAbbreviation i386 0x00081512 ppc 0x0008b420
void DrawHighGoalAbbreviation(short goal)
{
    int state;

    state = goal;
    if (state == kAIStateHiJack)
        DrawString("\pHiJack");
    if (state == kAIStateGoStel)
        DrawString("\pGoStel");
    if (state == kAIStateHypOut)
        DrawString("\pHypOut");
    if (state == kAIStateDefRet)
        DrawString("\pDefRet");
    if (state == kAIStateFightSh)
        DrawString("\pFightSh");
    if (state == kAIStateGoHome)
        DrawString("\pGoHome");
    if (state == kAIStateWait)
        DrawString("\pWait");
    if (state == kAIStateInspect)
        DrawString("\pInspect");
    if (state == kAIStateJumpIn)
        DrawString("\pJumpIn");
    if (state == kAIStateRefuel)
        DrawString("\pRefuel");
    if (state == kAIStateFlyPr)
        DrawString("\pFlyPr");
    if (state == kAIStateHypPr)
        DrawString("\pHypPr");
    if (state == kAIStateProtect)
        DrawString("\pProtect");
    if (state == kAIStatePlunder)
        DrawString("\pPlunder");
    if (state == kAIStateRepair)
        DrawString("\pRepair");
    if (state == kAIStateUnPlunder)
        DrawString("\pUnPlunder");
    if (state == kAIStateAtkAster)
        DrawString("\pAtkAster");
    if (state == kAIStateScoop)
        DrawString("\pScoop");
    if (state == kAIStateDestStel)
        DrawString("\pDestStel");
    if (state == kAIStateConfused)
        DrawString("\pConfused");
    if (state == kAIStateGoHGate)
        DrawString("\pGoHGate");
    if (state == kAIStateEmerge)
        DrawString("\pEmerge");
    if (state == kAIStateCountWait)
        DrawString("\pCountWait");
}

/*
 * TWENTY-FOUR INDEPENDENT ifs, not an else-if chain: every arm is parked past
 * the ret and jumps BACK to the next compare. The int intermediate is
 * load-bearing - it is what buys the exe's single `movsx ebx` and its 32-bit
 * compares in place of a `short`'s 16-bit ones.
 */

/* DrawLowGoalAbbreviation (mac _DrawLowGoalAbbreviation). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040ddc0
// MAC: _DrawLowGoalAbbreviation i386 0x0008169e ppc 0x0008b5ec
void DrawLowGoalAbbreviation(short goal)
{
    int state;

    state = goal;
    if (state == kSubLoJack)
        DrawString("\pLoJack");
    if (state == kSubKillSpd)
        DrawString("\pKillSpd");
    if (state == kSubFlyStel)
        DrawString("\pFlyStel");
    if (state == kSubFlyHypD)
        DrawString("\pFlyHypD");
    if (state == kSubHypJmp)
        DrawString("\pHypJmp");
    if (state == kSubRunAwy)
        DrawString("\pRunAwy");
    if (state == kSubTF)
        DrawString("\pT+F");
    if (state == kSubMissile)
        DrawString("\pMissle");
    if (state == kSubDock)
        DrawString("\pDock");
    if (state == kSubChase)
        DrawString("\pChase");
    if (state == kSubZoom)
        DrawString("\pZoom");
    if (state == kSubChasSlw)
        DrawString("\pChasSlw");
    if (state == kSubFormFly)
        DrawString("\pFormFly");
    if (state == kSubJmpPar)
        DrawString("\pJmpPar");
    if (state == kSubWaitTarg)
        DrawString("\pWaitTarg");
    if (state == kSubBoard)
        DrawString("\pBoard");
    if (state == kSubVeerOff)
        DrawString("\pVeerOff");
    if (state == kSubAfterBurn)
        DrawString("\pAfterBurn");
    if (state == kSubChaseSwarm)
        DrawString("\pChaseSwarm");
    if (state == kSubChaseAst)
        DrawString("\pChaseAst");
    if (state == kSubKillAst)
        DrawString("\pKillAst");
    if (state == kSubScoop)
        DrawString("\pScoop");
    if (state == kSubAttkStel)
        DrawString("\pAttkStel");
    if (state == kSubEnterHG)
        DrawString("\pEnterHG");
}

/* SelectWarshipTarget. STATE: MATCH 100.0%, exe 1352 / obj 1352 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0040e020
// MAC: _SelectWarshipTarget i386 0x00089d5e ppc 0x000951f0
void SelectWarshipTarget(ShipObj *ship)
{
    unsigned long long key;
    short i;
    short t;
    short s;
    short threats;
    short count;
    short myStrength;
    long best;
    long dx;
    long dy;
    short range;
    float fdx;
    float fdy;
    Boolean inRange;
    Boolean hostile;
    Boolean hasDestroying;
    Boolean hunts;
    long bestMass;
    KeyCheckFlag bad[5];
    long dist[kMaxShips];
    short strength[kMaxShips];
    Boolean cand[kMaxShips];

#ifndef MAC1010
    key = shipData[5].licenseCode;
#endif
    KeyMismatch(bad[0], key, shipData[5].licenseCode, 0xf6170bf185359c3dLL, 0x068887b12ff1e429LL);
    KeyMismatch(bad[1], key, shipData[5].licenseCode, 0xf620d1527b25efd6LL, 0x070575f85a9420d5LL);
    KeyMismatch(bad[2], key, shipData[5].licenseCode, 0x6e1bbb2b46dfcc17LL, 0x0c9f3b380ff9e811LL);
    KeyMismatch(bad[3], key, shipData[5].licenseCode, 0x8c0db41cb7473d23LL, 0x0faeaa47b4c5a01eLL);
    KeyMismatch(bad[4], key, shipData[5].licenseCode, 0x1f1e62f77679a8c9LL, 0x0e1f82a0c71946cbLL);
    for (i = 0; i < 5; i++) {
        if (bad[i] == 0) {
            AIMakeShipAttackPlayer(ship);
            return;
        }
    }

    if (ship->targetShip != -1 &&
        ((long)ship->aiState == kAIStateFightSh ||
         (long)ship->aiState == kAIStateDefRet) &&
        shipData[ship->targetShip].active != 0)
        return;

    if (ship->persType != -1) {
        if (ship->persType == kPersBountyHunter) {
            AIMakeShipAttackPlayer(ship);
            return;
        }
        if ((personData[ship->persType].flags & kPersHoldsGrudge) != 0 &&
            personData[ship->persType].grudge != 0 &&
            ShipVisibleToShip(shipData, ship) != 0) {
            AIMakeShipAttackPlayer(ship);
            return;
        }
    }

    if (ship->specialMission != -1 && currentMissionObjectives[ship->specialMission].active != 0) {
        if (currentMissions[ship->specialMission].shipBehav == 0 &&
            ShipVisibleToShip(shipData, ship) != 0) {
            AIMakeShipAttackPlayer(ship);
            return;
        }
        if (currentMissions[ship->specialMission].shipBehav == 1) {
            if (ship->targetShip == 0)
                ship->targetShip = -1;
            if (ship->targetShip == -1) {
                AIFightThreatToPlayer(ship);
                if (ship->targetShip == -1) {
                    ship->aiState = kAIStateProtect;
                    ship->targetStellar = 0;
                }
            }
            return;
        }
    }

    if (AIShipIsOutOfAmmo(ship) == 2)
        return;

    hasDestroying = AIHasDestroyingWeapons(ship);
    for (i = 0; i < kMaxShips; i++) {
        cand[i] = 0;
        strength[i] = 0;
    }

    if (ship->govt != -1) {
        if ((govtData[ship->govt].flags & kGovtXenophobic) != 0) {
            count = 0;
            for (i = 0; i < kMaxShips; i++) {
                if (IsDying(&shipData[i]) != 0)
                    continue;
                if (IsDisabled(&shipData[i]) != 0)
                    continue;
                if (ShipVisibleToShip(&shipData[i], ship) == 0)
                    continue;
                if (shipData[i].type == kShipTypeEscapePod)
                    continue;
                if (i == 0) {
                    hunts = (govtData[ship->govt].flags &
                                   kGovtNeverAttacksPlayer);
                    if (ShipVisibleToShip(shipData, ship) == 0)
                        continue;
                    if (hunts != 0)
                        continue;
                    if (systemData[shipData->curSystem].govt == ship->govt) {
                        if (playerRecord[shipData->curSystem] > 0)
                            continue;
                        if (PlayerBlessedByGovt(ship->govt, 0) != 0)
                            continue;
                        cand[i] = 1;
                        count++;
                    } else {
                        if (PlayerBlessedByGovt(ship->govt, 0) != 0)
                            continue;
                        cand[i] = 1;
                        count++;
                    }
                } else {
                    if (i == ship->index)
                        continue;
                    if (IsDisabled(&shipData[i]) != 0)
                        continue;
                    if (shipData[i].active == 0)
                        continue;
                    if (shipData[i].parentShip == ship->index)
                        continue;
                    if (shipData[i].curSystem != ship->curSystem)
                        continue;
                    if (shipData[i].defenseStellar != -1)
                        continue;
                    if (shipData[i].persType == kPersCaptainHector)
                        continue;
                    if (shipData[i].type == kShipTypeEscapePod)
                        continue;
                    if (GovtAllies(ship->govt, shipData[i].govt) != 0)
                        continue;
                    if (IsShipMyEnemy(ship, &shipData[i]) == 0)
                        continue;
                    cand[i] = 1;
                    count++;
                }
            }
            if (count > 0) {
                myStrength = AIShipFriendStrength(ship);
                for (i = 0; i < kMaxShips; i++) {
                    if (cand[i] != 0) {
                        strength[i] = AIShipFriendStrength(&shipData[i]);
                        dx = abs(shipData[i].pos.x - ship->pos.x);
                        dy = abs(shipData[i].pos.y - ship->pos.y);
                        dist[i] = dx * dx + dy * dy;
                    }
                }
                for (i = 0; i < kMaxShips; i++) {
                    if (cand[i] != 0 &&
                        strength[i] > myStrength * govtData[ship->govt].maxOdds) {
                        cand[i] = 0;
                        count--;
                    }
                }
            }
            if (count > 0) {
                best = -1;
                for (i = 0; i < kMaxShips; i++) {
                    if (cand[i] != 0) {
                        if (dist[i] < best || best < 0) {
                            best = dist[i];
                            ship->targetShip = i;
                        }
                    }
                }
                if (ship->targetShip == 0)
                    AIMakeShipAttackPlayer(ship);
            }
        } else {
            if (ship->aiType <= 4) {
                myStrength = -1;
                for (i = 1; i < kMaxShips; i++) {
                    if (i == ship->index)
                        continue;
                    if (shipData[i].active == 0)
                        continue;
                    if (shipData[i].govt == -1)
                        continue;
                    if (shipData[i].targetShip == -1)
                        continue;
                    if (!((long)shipData[i].aiState == kAIStateFightSh ||
                          (long)shipData[i].aiState == kAIStateDefRet))
                        continue;
                    if (GovtAllies(ship->govt, shipData[i].govt) == 0)
                        continue;
                    t = shipData[i].targetShip;
                    if (t == 0 || shipData[t].parentShip == 0) {
                        if (PlayerBlessedByGovt(ship->govt, 0) != 0)
                            continue;
                    }
                    if (ShipVisibleToShip(&shipData[t], ship) == 0)
                        continue;
                    s = AIShipFriendStrength(&shipData[t]);
                    if (myStrength < 0)
                        myStrength = AIShipFriendStrength(ship);
                    if (s <= myStrength * govtData[ship->govt].maxOdds) {
                        ship->targetShip = t;
                        ship->aiState = kAIStateFightSh;
                        return;
                    }
                }
            }

            inRange = 0;
            hostile = (govtData[ship->govt].flags & kGovtEnforcesAbroad);
            fdx = FloatAbs(ship->pos.x - shipData->pos.x);
            fdy = FloatAbs(ship->pos.y - shipData->pos.y);
            range = ship->aggression * kScanRangeStep;
            if (PlayerBlessedByGovt(ship->govt, 0) == 0) {
                if (fdx <= range && fdy <= range &&
                    ShipVisibleToShip(shipData, ship) != 0 &&
                    IsDying(shipData) == 0)
                    inRange = 1;
            }
            hunts = (govtData[ship->govt].flags & kGovtNeverAttacksPlayer);
            if (inRange != 0 && hunts == 0 &&
                ShipVisibleToShip(shipData, ship) != 0) {
                if (systemData[shipData->curSystem].govt == ship->govt) {
                    if (playerRecord[shipData->curSystem] < -govtData[ship->govt].crimeTol)
                        cand[0] = 1;
                } else if (systemData[shipData->curSystem].govt >= 0) {
                    if (GovtEnemies(ship->govt, systemData[shipData->curSystem].govt) != 0) {
                        if (playerRecord[shipData->curSystem]
                            > govtData[ship->govt].crimeTol)
                            cand[0] = 1;
                    } else if (GovtAllies(ship->govt,
                                          systemData[shipData->curSystem].govt) != 0) {
                        if (playerRecord[shipData->curSystem] <
                            -govtData[ship->govt].crimeTol * 1.5)
                            cand[0] = 1;
                    } else if (hostile != 0) {
                        if (playerRecord[shipData->curSystem] <
                            -govtData[ship->govt].crimeTol * 2)
                            cand[0] = 1;
                    }
                } else {
                    if (hostile != 0 &&
                        playerRecord[shipData->curSystem] <
                            -govtData[ship->govt].crimeTol * 2)
                        cand[0] = 1;
                }
            }

            if (ship->parentShip != 0 && ship->govt != -1 &&
                ship->specialMission == -1 && ship->auxMission == -1 &&
                shipClass[shipData->type].inherentGovt != -1 &&
                GovtEnemies(ship->govt,
                            shipClass[shipData->type].inherentGovt) != 0 &&
                Rand(0x32) == 0 && IsDying(shipData) == 0 &&
                ShipVisibleToShip(shipData, ship) != 0)
                cand[0] = 1;

        }
            if (ship->govt != -1 && govtData[ship->govt].iffScrambled != 0)
                cand[0] = 0;

        if (PlayerBlessedByGovt(ship->govt, 0) != 0)
            cand[0] = 0;

        bestMass = -1;
        count = 0;
        if (cand[0] != 0)
            count++;
        for (i = 1; i < kMaxShips; i++) {
            if (i == ship->index)
                continue;
            if (ShipVisibleToShip(&shipData[i], ship) == 0)
                continue;
            if (shipData[i].active == 0)
                continue;
            if (hasDestroying == 0 && IsDisabled(&shipData[i]) != 0)
                continue;
            if (IsShipMyEnemy(ship, &shipData[i]) == 0)
                continue;
            cand[i] = 1;
            count++;
        }

        if (count > 0) {
            myStrength = AIShipFriendStrength(ship);
            for (i = 0; i < kMaxShips; i++) {
                if (cand[i] != 0) {
                    strength[i] = AIShipFriendStrength(&shipData[i]);
                    dx = abs(shipData[i].pos.x - ship->pos.x);
                    dy = abs(shipData[i].pos.y - ship->pos.y);
                    dist[i] = dx * dx + dy * dy;
                }
            }
            for (i = 0; i < kMaxShips; i++) {
                if (cand[i] != 0) {
                    if (strength[i] > myStrength * govtData[ship->govt].maxOdds) {
                        cand[i] = 0;
                        count--;
                    } else if (ship->hasEscorts != 0) {
                        if (bestMass == -1 ||
                            shipClass[shipData[i].type].mass > bestMass)
                            bestMass = shipClass[shipData[i].type].mass;
                    }
                }
            }
        }

        if (count > 0) {
            best = -1;
            for (i = 0; i < kMaxShips; i++) {
                if (cand[i] != 0) {
                    if (dist[i] < best || best < 0) {
                        if (bestMass == -1 ||
                            bestMass == shipClass[shipData[i].type].mass) {
                            best = dist[i];
                            ship->targetShip = i;
                        }
                    }
                }
            }
            if (ship->targetShip == 0)
                AIMakeShipAttackPlayer(ship);
        }

        if (ship->targetShip == -1) {
            best = -1;
            for (i = 0; i < kMaxShips; i++) {
                if (i == ship->index)
                    continue;
                if (shipData[i].active == 0)
                    continue;
                if (ShipVisibleToShip(&shipData[i], ship) == 0)
                    continue;
                if (ExtendedIsThreatToShip(ship, &shipData[i]) == 0)
                    continue;
                if (cand[i] == 0) {
                    dx = abs(shipData[i].pos.x - ship->pos.x);
                    dy = abs(shipData[i].pos.y - ship->pos.y);
                    dist[i] = dx * dx + dy * dy;
                }
                if (dist[i] < best || best < 0) {
                    best = dist[i];
                    ship->targetShip = i;
                }
            }
        }

    }

    if (ship->aiType == 6 && ship->targetShip == -1) {
        threats = 0;
        for (i = 1; i < kMaxShips; i++)
            cand[i] = 0;
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && i != ship->index
                && i != shipData[ship->parentShip].index
                && ExtendedIsThreatToShip(ship, &shipData[i]) != 0
                && shipData[i].curSystem == ship->curSystem) {
                if (cand[i] == 0) {
                    cand[i] = 1;
                    dx = abs(shipData[i].pos.x - ship->pos.x);
                    dy = abs(shipData[i].pos.y - ship->pos.y);
                    dist[i] = dx * dx + dy * dy;
                }
                threats++;
            } else
                cand[i] = 0;
        }
        if (threats > 0) {
            best = -1;
            for (i = 1; i < kMaxShips; i++) {
                if (dist[i] < best || best < 0) {
                    best = dist[i];
                    ship->targetShip = i;
                    ship->aiState = kAIStateFightSh;
                }
            }
        }
    }
}

/* IsThreatToPlayer (mac _IsThreatToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040f6d0
// MAC: _IsThreatToPlayer i386 0x0007f501 ppc 0x00088ff8
Boolean IsThreatToPlayer(ShipObj *ship)
{
    if (ship->active != 0 && ship->aiTimer <= 0.0f && !IsDisabled(ship) && ship->targetShip >= 0
            && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel
            && (int)ship->aiState != kAIStateRepair && (int)ship->aiState != kAIStateFlyPr
            && (int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateGoHome
            && (int)ship->aiState != kAIStateProtect && (int)ship->aiState != kAIStateDestStel) {
        if (ship->targetShip == 0)
            return 1;
        if (ship->targetShip < kMaxShips && shipData[ship->targetShip].parentShip == 0)
            return 1;
    }
    return 0;
}

/* ExtendedIsThreatToPlayer (mac _ExtendedIsThreatToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040f780
// MAC: _ExtendedIsThreatToPlayer i386 0x00081b70 ppc 0x0008bba0
Boolean ExtendedIsThreatToPlayer(ShipObj *ship)
{
    short i;
    short w;

    if (ship->active != 0 && !IsDisabled(ship) && ship->parentShip != 0
            && ship->targetShip != -1 && ShipVisibleToShip(ship, shipData) != 0) {
        if (ship->defenseStellar != -1)
            return 1;
        if (ship->targetShip == 0 && ship->aiTimer <= 0.0f
                && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel
                && (int)ship->aiState != kAIStateRepair && (int)ship->aiState != kAIStateFlyPr
                && (int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateGoHome
                && (int)ship->aiState != kAIStateProtect && (int)ship->aiState != kAIStateDestStel)
            return 1;
        if (shipData[ship->targetShip].parentShip == 0 && ship->aiTimer <= 0.0f
                && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel
                && (int)ship->aiState != kAIStateRepair && (int)ship->aiState != kAIStateFlyPr
                && (int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateGoHome
                && (int)ship->aiState != kAIStateProtect && (int)ship->aiState != kAIStateDestStel)
            return 1;
        if (ship->targetShip != -1 && ship->aiTimer <= 0.0f
                && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel
                && (int)ship->aiState != kAIStateRepair && (int)ship->aiState != kAIStateFlyPr
                && (int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateGoHome
                && (int)ship->aiState != kAIStateProtect && (int)ship->aiState != kAIStateDestStel
                && shipData[ship->targetShip].parentShip == 0)
            return 1;
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].parentShip == ship->index && shipData[i].aiTimer <= 0.0f
                    && shipData[i].active != 0 && !IsDisabled(&shipData[i])) {
                w = shipData[i].targetShip;
                if (w == 0
                        && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel
                        && (int)ship->aiState != kAIStateRepair && (int)ship->aiState != kAIStateFlyPr
                        && (int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateGoHome
                        && (int)ship->aiState != kAIStateProtect && (int)ship->aiState != kAIStateDestStel)
                    return 1;
                if (w != -1
                        && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel
                        && (int)ship->aiState != kAIStateRepair && (int)ship->aiState != kAIStateFlyPr
                        && (int)ship->aiState != kAIStateHypPr && (int)ship->aiState != kAIStateGoHome
                        && (int)ship->aiState != kAIStateProtect && (int)ship->aiState != kAIStateDestStel
                        && shipData[w].parentShip == 0)
                    return 1;
            }
        }
    }
    return 0;
}

/* the AI states that mean the ship is leaving rather than fighting */
#define AIStateIsLeaving(s) \
    ((s) != 7 && (s) != 9 && (s) != 0xf && (s) != 10 \
     && (s) != 0xb && (s) != 5 && (s) != 0xc && (s) != 0x12)

/* ExtendedIsThreatToShip (mac _ExtendedIsThreatToShip). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040faa0
// MAC: _ExtendedIsThreatToShip i386 0x00081a27 ppc 0x0008ba0c
Boolean ExtendedIsThreatToShip(ShipObj *self, ShipObj *other)
{
    short n;

    if (other->active != 0 && IsDisabled(other) == 0 && other->parentShip != self->index) {
        if (other->index == 0) {
            if (PlayerBlessedByGovt(self->govt, 0) != 0)
                return 0;
            return IsThreatToPlayer(self);
        }
        if (other->targetShip == self->index
                && (int)other->aiState != kAIStateInspect && (int)other->aiState != kAIStateRefuel
                && (int)other->aiState != kAIStateRepair && (int)other->aiState != kAIStateFlyPr
                && (int)other->aiState != kAIStateHypPr && (int)other->aiState != kAIStateGoHome
                && (int)other->aiState != kAIStateProtect && (int)other->aiState != kAIStateDestStel)
            return 1;
        for (n = 1; n < kMaxShips; n++) {
            if (shipData[n].active != 0 && self->index == shipData[n].parentShip
                    && other->targetShip == n && n != other->index
                    && (int)other->aiState != kAIStateInspect && (int)other->aiState != kAIStateRefuel
                    && (int)other->aiState != kAIStateRepair && (int)other->aiState != kAIStateFlyPr
                    && (int)other->aiState != kAIStateHypPr && (int)other->aiState != kAIStateGoHome
                    && (int)other->aiState != kAIStateProtect && (int)other->aiState != kAIStateDestStel)
                return 1;
        }
    }
    return 0;
}

/* IsThreatToShip (mac _IsThreatToShip, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _IsThreatToShip i386 0x00081faf ppc 0x0008c0c8
Boolean IsThreatToShip(ShipObj *self, ShipObj *other)
{
    if (other->active != 0 && !IsDisabled(other) && other->parentShip != self->index) {
        if (other->index == 0) {
            if (PlayerBlessedByGovt(self->govt, 0))
                return 0;
            return IsThreatToPlayer(self);
        }
        if (other->targetShip == self->index
                && other->aiState != kAIStateInspect && other->aiState != kAIStateRefuel
                && other->aiState != kAIStateRepair && other->aiState != kAIStateFlyPr
                && other->aiState != kAIStateHypPr && other->aiState != kAIStateGoHome
                && other->aiState != kAIStateProtect && other->aiState != kAIStateDestStel)
            return 1;
    }
    return 0;
}

/* IsShipThreatened (mac _IsShipThreatened). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040fc00
// MAC: _IsShipThreatened i386 0x00082068 ppc 0x0008c1ac
Boolean IsShipThreatened(ShipObj *ship)
{
    short i;

    if (ship->active != 0 && !IsDisabled(ship)) {
        for (i = 1; i < kMaxShips; i++) {
            /* BUG: tests whether the ship targets itself, not whether shipData[i] targets it */
            if (shipData[i].active != 0 && ship->targetShip == ship->index
                    && i != ship->index
                    && (int)shipData[i].aiState != kAIStateInspect
                    && (int)shipData[i].aiState != kAIStateRefuel
                    && (int)shipData[i].aiState != kAIStateRepair
                    && (int)shipData[i].aiState != kAIStateFlyPr
                    && (int)shipData[i].aiState != kAIStateHypPr
                    && (int)shipData[i].aiState != kAIStateGoHome
                    && (int)shipData[i].aiState != kAIStateProtect)
                return 1;
        }
    }
    return 0;
}

/* IsRefuellingPlayer (mac _IsRefuellingPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040fca0
// MAC: _IsRefuellingPlayer i386 0x000820fd ppc 0x0008c298
Boolean IsRefuellingPlayer(ShipObj *ship)
{
    if (ship->active != 0) {
        if (!IsDisabled(ship) && ship->targetShip == 0 && ship->aiState == kAIStateRefuel)
            return 1;
    }
    return 0;
}

/* IsRepairingPlayer (mac _IsRepairingPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0040fce0
// MAC: _IsRepairingPlayer i386 0x0008213a ppc 0x0008c304
Boolean IsRepairingPlayer(ShipObj *ship)
{
    if (ship->active != 0) {
        if (!IsDisabled(ship) && ship->targetShip == 0 && ship->aiState == kAIStateRepair)
            return 1;
    }
    return 0;
}

/* AIDoesShipLikePlayer. STATE: MATCH 100.0%, exe 289 / obj 289, relocregcheck 0 of 35.
   The system-government test is written `govt >= 0' first, the unowned-system arm second.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0040fd20
// MAC: _AIDoesShipLikePlayer i386 0x00082177 ppc 0x0008c370
Boolean AIDoesShipLikePlayer(ShipObj *ship)
{
    Boolean xenophobe;
    Boolean flag2;
    short behav;

    if (ExtendedIsThreatToPlayer(ship) != 0)
        return 0;
    if (ship->parentShip == 0)
        return 1;
    if (PlayerBlessedByGovt(ship->govt, 0) != 0)
        return 1;

    if (ship->specialMission != -1) {
        if (currentMissionObjectives[ship->specialMission].active == 0)
            goto dislike;
        if (currentMissions[ship->specialMission].shipGoal != 3
                && currentMissions[ship->specialMission].shipGoal != 4)
            return 0;
        behav = currentMissions[ship->specialMission].shipBehav;
        if (behav == -1)
            goto dislike;
        if (behav == 1)
            return 1;
        if (behav != 0)
            goto dislike;
        return 0;
    }

    if (ship->govt == -1)
        goto like_govt;

    xenophobe = govtData[ship->govt].flags & kGovtXenophobic;
    flag2 = govtData[ship->govt].flags & kGovtEnforcesAbroad;
    if (xenophobe != 0) {
        if (ship->govt == systemData[shipData[0].curSystem].govt) {
            if (playerRecord[shipData[0].curSystem]
                    > govtData[systemData[shipData[0].curSystem].govt].crimeTol)
                return 1;
        } else if (systemData[shipData[0].curSystem].govt >= 0) {
            if (playerRecord[shipData[0].curSystem]
                    > govtData[systemData[shipData[0].curSystem].govt].crimeTol)
                return 0;
        } else {
            if (playerRecord[shipData[0].curSystem] > govtData[0].crimeTol)
                return 0;
        }
    }

    if (systemData[shipData[0].curSystem].govt >= 0) {
        if (GovtAllies(systemData[shipData[0].curSystem].govt, ship->govt) != 0) {
            if (playerRecord[shipData[0].curSystem]
                    >= -govtData[systemData[shipData[0].curSystem].govt].crimeTol)
                return 1;
        } else if (GovtEnemies(systemData[shipData[0].curSystem].govt, ship->govt) != 0) {
            if (playerRecord[shipData[0].curSystem]
                    < -govtData[systemData[shipData[0].curSystem].govt].crimeTol)
                return 1;
        } else if (flag2 != 0) {
            if (playerRecord[shipData[0].curSystem]
                    >= -govtData[systemData[shipData[0].curSystem].govt].crimeTol)
                return 1;
        } else {
            return 1;
        }
    } else {
        if (flag2 != 0) {
            if (playerRecord[shipData[0].curSystem]
                    >= -govtData[ship->govt].crimeTol)
                return 1;
        } else {
            return 1;
        }
    }

    if (govtData[ship->govt].iffScrambled == 0)
        goto dislike;
    return 1;

like_govt:
    return 1;
dislike:
    return 0;
}

/* IsPlayerThreatened (mac _IsPlayerThreatened). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410060
// MAC: _IsPlayerThreatened i386 0x00082442 ppc 0x0008c6b0
Boolean IsPlayerThreatened(void)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (IsThreatToPlayer(&shipData[i]))
            return 1;
    }
    return 0;
}

/* IsPlayerThreatenedByMyEnemy (mac _IsPlayerThreatenedByMyEnemy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004100a0
// MAC: _IsPlayerThreatenedByMyEnemy i386 0x00082483 ppc 0x0008c71c
Boolean IsPlayerThreatenedByMyEnemy(ShipObj *ship)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && i != ship->index
            && ExtendedIsThreatToPlayer(&shipData[i]) != 0
            && IsShipMyEnemy(ship, &shipData[i]) != 0)
            return 1;
    }
    return 0;
}

/* IsShipThreatenedByMyEnemy (mac _IsShipThreatenedByMyEnemy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410110
// MAC: _IsShipThreatenedByMyEnemy i386 0x000824f6 ppc 0x0008c7c0
Boolean IsShipThreatenedByMyEnemy(ShipObj *ship, ShipObj *me)
{
    short i;

#ifdef MAC1010 /* 1.0.10 spelling */
    assert(ship != 0);
#endif
    if (ship->index == 0)
        return IsPlayerThreatenedByMyEnemy(me);
    if (ExtendedIsThreatToPlayer(ship) != 0)
        return 1;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && i != me->index && i != ship->index
            && ExtendedIsThreatToShip(&shipData[i], ship) != 0
            && IsShipMyEnemy(me, &shipData[i]) != 0)
            return 1;
    }
    return 0;
}

/* IsShipMyEnemy (mac _IsShipMyEnemy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004101d0
// MAC: _IsShipMyEnemy i386 0x0008196c ppc 0x0008b93c
Boolean IsShipMyEnemy(ShipObj *ship, ShipObj *other)
{
    short theirs;
    short mine;

    if (ship->index == other->index)
        return 0;
    if (ship->active == 0 || other->active == 0)
        return 0;
    if (other->persType == kPersCaptainHector)
        return 0;
    mine = ship->govt;
    if (mine != -1 && (theirs = other->govt) != -1) {
        if (mine == theirs)
            return 0;
        if (GovtEnemies(mine, theirs) != 0
                || ((govtData[other->govt].flags & kGovtXenophobic) != 0
                    && GovtAllies(ship->govt, other->govt) == 0))
            return 1;
    }
    return 0;
}

/* IsShipFightingMe. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004102b0
// MAC: _IsShipFightingMe i386 0x000827fa ppc 0x0008cb68
Boolean IsShipFightingMe(ShipObj *ship, ShipObj *other)
{
    if (ship->targetShip == other->index && ship->aiState == kAIStateFightSh)
        return 1;
    return 0;
}

/* DoGoodSamaritan. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004102e0
// MAC: _DoGoodSamaritan i386 0x00082823 ppc 0x0008cb94
void DoGoodSamaritan(ShipObj *victim, ShipObj *attacker)
{
    short i;
    Boolean allied;
    Boolean blessed;
    short sgovt;
    Boolean help;

    if (victim == NULL || attacker == NULL)
        return;
    if (victim->persType == kPersCaptainHector ||
        attacker->persType == kPersBountyHunter)
        return;
    if (victim->govt >= 0 && victim->govt < kMaxGovts &&
        (govtData[victim->govt].flags & kGovtAlwaysDisabled))
        return;

    for (i = 1; i < kMaxShips; i++) {
        if (!(shipData[i].aiType == 3 || shipData[i].aiType == 4))
            continue;
        if (shipData[i].aiState == kAIStateFightSh)
            continue;
        help = (shipData[i].persType != kPersCaptainHector) ? (Boolean)1 : (Boolean)0;
        if (IsShipMyEnemy(&shipData[i], victim))
            continue;
        sgovt = shipData[i].govt;
        if (sgovt != -1 && victim->govt != -1) {
            if (GovtEnemies(shipData[i].govt, victim->govt))
                help = 0;
            if ((Boolean)(govtData[victim->govt].flags & kGovtXenophobic)) {
                if (victim->govt != shipData[i].govt)
                    help = 0;
            }
            blessed = govtData[shipData[i].govt].flags & kGovtEnforcesAbroad;
            allied = 0;
            if (attacker->index == 0 || attacker->parentShip == 0) {
                if (PlayerBlessedByGovt(shipData[i].govt, 0))
                    help = 0;
            }
            if (GovtAllies(shipData[i].govt, victim->govt))
                allied = 1;
            if (GovtAllies(victim->govt, shipData[i].govt))
                allied = 1;
            if (blessed == 0 && allied == 0)
                help = 0;
            if (GovtAllies(shipData[i].govt, attacker->govt))
                help = 0;
        }
        sgovt = shipData[i].govt;
        if (sgovt != -1 && victim->govt == -1) {
            blessed = govtData[sgovt].flags & kGovtEnforcesAbroad;
            if (!blessed)
                help = 0;
        }
        if (victim->govt != -1) {
            if (govtData[victim->govt].flags & kGovtAlwaysDisabled)
                help = 0;
            if (govtData[victim->govt].flags & kGovtIgnoredByNosy)
                help = 0;
        }
        if (sgovt == -1) {
            if (victim->govt != -1) {
                if ((Boolean)(govtData[victim->govt].flags & kGovtXenophobic))
                    help = 0;
            }
        }
        if (shipData[i].parentShip != -1) {
            if (attacker->index == shipData[i].parentShip)
                help = 0;
            else if (attacker->index == shipData[shipData[i].parentShip].parentShip)
                help = 0;
        }
        if (help) {
            shipData[i].aiState = kAIStateFightSh;
            shipData[i].targetShip = attacker->index;
        }
    }
}

/* AIMakeShipLeave (mac _AIMakeShipLeave). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410670
// MAC: _AIMakeShipLeave i386 0x0007e2b0 ppc 0x00087eb8
void AIMakeShipLeave(ShipObj *ship)
{
    ship->aiState = kAIStateHypOut;
    if (ship->jumpTimer < 0.0f)
        ship->jumpTimer = 0.0f;
    ship->targetShip = -1;
    ship->jumpTick = ReadTick();
}

/* AIMakeShipJumpWithParent (mac _AIMakeShipJumpWithParent). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004106b0
// MAC: _AIMakeShipJumpWithParent i386 0x00082bb6 ppc 0x0008ced0
void AIMakeShipJumpWithParent(ShipObj *ship)
{
    ship->aiState = kAIStateHypPr;
    ship->grudge = 0;
    ship->targetShip = -1;
    if (ship->jumpTimer <= 0.0f) {
        ship->jumpTimer = 1.0f;
        ship->jumpTick = ReadTick();
    }
}

/* AIMakeShipAttackPlayer (mac _AIMakeShipAttackPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410700
// MAC: _AIMakeShipAttackPlayer i386 0x00089c3e ppc 0x000950b0
void AIMakeShipAttackPlayer(ShipObj *ship)
{
    if (ExtendedIsThreatToPlayer(ship) == 0 && ship->persType != -1
        && ship->specialMission == -1
        && (personData[ship->persType].flags & kPersQuoteOnAttack) != 0
        && IsDisabled(ship) == 0 && IsDying(shipData) == 0) {
        offeringShipIndex = ship->index;
        commCount = -1;
        ShowPersonHailQuote(personData[ship->persType].hailQuote);
        ship->hailed = 1;
        offeringShipIndex = -1;
    }
    if (AIIsShipJumping(ship) != 0)
        ship->aiSubstate = kSubLoJack;
    ship->aiState = kAIStateFightSh;
    ship->targetStellar = -1;
    ship->targetShip = 0;
}

/* AIFightThreatToPlayer (mac _AIFightThreatToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004107e0
// MAC: _AIFightThreatToPlayer i386 0x0008183c ppc 0x0008b7cc
void AIFightThreatToPlayer(ShipObj *ship)
{
    short i;
    short count;

    count = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (i != ship->index && i != 0
                && !IsDisabled(&shipData[i]) && !IsDying(&shipData[i])
                && shipData[i].curSystem == ship->curSystem
                && IsThreatToPlayer(&shipData[i]))
            count++;
    }
    if (count > 0) {
        for (;;) {
            i = Rand(kMaxShips - 1) + 1;
            if (i != ship->index && i != 0
                    && !IsDisabled(&shipData[i]) && !IsDying(&shipData[i])
                    && shipData[i].curSystem == ship->curSystem
                    && IsThreatToPlayer(&shipData[i]))
                break;
        }
        ship->targetStellar = -1;
        ship->targetShip = i;
        ship->aiState = kAIStateFightSh;
    } else {
        ship->targetShip = -1;
    }
}

/* AIFightThreatToParent (mac _AIFightThreatToParent). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410900
// MAC: _AIFightThreatToParent i386 0x00081de6 ppc 0x0008becc
void AIFightThreatToParent(ShipObj *ship)
{
    short i;
    short count;

    count = 0;
    for (i = 0; i < kMaxShips; i++) {
        if (i != ship->index && i != ship->parentShip
                && !IsDisabled(&shipData[i])
                && shipData[i].curSystem == ship->curSystem) {
            if (i != 0) {
                if (ship->parentShip == 0) {
                    if (ExtendedIsThreatToPlayer(&shipData[i]))
                        count++;
                } else if (ExtendedIsThreatToShip(&shipData[ship->parentShip],
                                                  &shipData[i])) {
                    count++;
                }
            } else if (ExtendedIsThreatToPlayer(&shipData[ship->parentShip])) {
                count++;
            }
        }
    }
    if (count > 0) {
        for (;;) {
            i = Rand(kMaxShips);
            if (i == ship->index || i == ship->parentShip
                    || !shipData[i].active
                    || shipData[i].curSystem != ship->curSystem
                    || IsDisabled(&shipData[i]))
                continue;
            if (i != 0) {
                if (ship->parentShip == 0) {
                    if (ExtendedIsThreatToPlayer(&shipData[i])) {
                        ship->targetStellar = -1;
                        ship->targetShip = i;
                        ship->aiState = kAIStateFightSh;
                        break;
                    }
                } else if (ExtendedIsThreatToShip(&shipData[ship->parentShip],
                                                  &shipData[i])) {
                    ship->targetStellar = -1;
                    ship->targetShip = i;
                    ship->aiState = kAIStateFightSh;
                    break;
                }
            } else if (ExtendedIsThreatToPlayer(&shipData[ship->parentShip])) {
                ship->targetStellar = -1;
                ship->targetShip = 0;
                ship->aiState = kAIStateFightSh;
                break;
            }
        }
    } else {
        ship->targetShip = -1;
    }
}

/* AIMakeShipAttackShip (mac _AIMakeShipAttackShip, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _AIMakeShipAttackShip i386 0x00082c44 ppc 0x0008cf84
void AIMakeShipAttackShip(ShipObj *ship, ShipObj *target)
{
    ship->aiState = kAIStateFightSh;
    ship->targetShip = target->index;
}

/* AIMakeShipWait (mac _AIMakeShipWait, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _AIMakeShipWait i386 0x00082c63 ppc 0x0008cf9c
void AIMakeShipWait(ShipObj *ship)
{
    ship->aiState = kAIStateWait;
    ship->targetShip = -1;
    ship->targetStellar = -1;
}

/* AIFightEnemyThreatToPlayer (mac _AIFightEnemyThreatToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410b00
// MAC: _AIFightEnemyThreatToPlayer i386 0x00082c80 ppc 0x0008cfb8
void AIFightEnemyThreatToPlayer(ShipObj *ship)
{
    short count;
    short i;

    count = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (i != ship->index && i != 0
                && !IsDisabled(&shipData[i]) && !IsDying(&shipData[i])
                && shipData[i].targetShip == 0
                && ((int)shipData[i].aiState == kAIStateFightSh
                    || (int)shipData[i].aiState == kAIStateDefRet)
                && shipData[i].curSystem == ship->curSystem)
            count++;
    }
    if (count > 0) {
        for (;;) {
            i = Rand(kMaxShips - 1) + 1;
            if (i != ship->index && i != 0
                    && !IsDisabled(&shipData[i]) && !IsDying(&shipData[i])
                    && shipData[i].targetShip == 0
                    && ((int)shipData[i].aiState == kAIStateFightSh
                        || (int)shipData[i].aiState == kAIStateDefRet)
                    && shipData[i].curSystem == ship->curSystem)
                break;
        }
        ship->targetShip = i;
        ship->aiState = kAIStateFightSh;
    } else {
        ship->targetShip = -1;
    }
}

/* AIMakeShipRefuelPlayer (mac _AIMakeShipRefuelPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410c30
// MAC: _AIMakeShipRefuelPlayer i386 0x00082dbb ppc 0x0008d150
void AIMakeShipRefuelPlayer(ShipObj *ship)
{
    ship->grudge = 0;
    ship->jumpTimer = 0;
    ship->targetShip = 0;
    ship->aiState = kAIStateRefuel;
    ship->aiSubstate = kSubLoJack;
    ship->aiTimer = -1.0f;
}

/* AIMakeShipRepairPlayer (mac _AIMakeShipRepairPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410c70
// MAC: _AIMakeShipRepairPlayer i386 0x00082df2 ppc 0x0008d184
void AIMakeShipRepairPlayer(ShipObj *ship)
{
    ship->grudge = 0;
    ship->jumpTimer = 0;
    ship->targetShip = 0;
    ship->aiState = kAIStateRepair;
    ship->aiSubstate = kSubLoJack;
    ship->aiTimer = -1.0f;
}

/* AIMakeFighterReturn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410cb0
// MAC: _AIMakeFighterReturn i386 0x00082e29 ppc 0x0008d1b8
void AIMakeFighterReturn(ShipObj *ship)
{
    ship->targetShip = -1;
    ship->targetStellar = ship->parentShip;
    ship->aiState = kAIStateGoHome;
    ship->aiSubstate = kSubLoJack;
}

/* AIIsFighterReturning. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410ce0
// MAC: _AIIsFighterReturning i386 0x00082e54 ppc 0x0008d1e0
Boolean AIIsFighterReturning(ShipObj *ship)
{
    if (ship->aiType == 5 && ship->aiState == kAIStateGoHome)
        return 1;
    return 0;
}

/* AIMakeEscortFlyInForm. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410d10
// MAC: _AIMakeEscortFlyInForm i386 0x00089b8e ppc 0x00094fe8
void AIMakeEscortFlyInForm(ShipObj *ship)
{
    short i;

    ship->targetShip = -1;
    ship->targetStellar = ship->parentShip;
    if (ship->aiType == 5)
        ship->aiState = kAIStateFlyPr;
    else if (ship->parentShip == 0)
        ship->aiState = kAIStateProtect;
    else
        ship->aiState = kAIStateFlyPr;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && i != ship->index
            && ship->index == shipData[i].parentShip
            && shipData[i].aiType == 5)
            AIMakeFighterReturn(&shipData[i]);
    }
}

/* AIMakeShipStopJumping (mac _AIMakeShipStopJumping). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410dd0
// MAC: _AIMakeShipStopJumping i386 0x00082e77 ppc 0x0008d208
void AIMakeShipStopJumping(ShipObj *ship)
{
    if ((int)ship->aiState != kAIStateRefuel && (int)ship->aiState != kAIStateRepair) {
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
    }
    ship->targetStellar = -1;
    ship->jumpTimer = 0;
    ship->thrust = 0;
    ship->speedLimit = 0;
}

/* AIMakeShipJumpIn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410e20
// MAC: _AIMakeShipJumpIn i386 0x00082ebb ppc 0x0008d248
void AIMakeShipJumpIn(ShipObj *ship)
{
    ship->aiState = kAIStateJumpIn;
    ship->jumpTimer = -999.0f;
    ship->foldDir = 1;
    ship->foldSet = 0;
    ship->frameTimer = -Rand(10);
    ship->weapGlow = 0;
}

/* AIIsShipJumping (mac _AIIsShipJumping). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410e80
// MAC: _AIIsShipJumping i386 0x00082bf9 ppc 0x0008cf40
Boolean AIIsShipJumping(ShipObj *ship)
{
    if ((int)ship->aiState == kAIStateHypOut || (int)ship->aiState == kAIStateHypPr || (int)ship->aiState == kAIStateDefRet) {
        if ((int)ship->aiSubstate == kSubHypJmp || (int)ship->aiSubstate == kSubJmpPar)
            return 1;
    }
    return 0;
}

/* AIIsShipJumpingIn (mac _AIIsShipJumpingIn). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410ec0
// MAC: _AIIsShipJumpingIn i386 0x00082f12 ppc 0x0008d2d4
Boolean AIIsShipJumpingIn(ShipObj *ship)
{
    if (ship->aiState == kAIStateJumpIn)
        return 1;
    return 0;
}

/* IsShipDocking (mac _IsShipDocking, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _IsShipDocking i386 0x00082f28 ppc 0x0008d2ec
Boolean IsShipDocking(ShipObj *ship)
{
    return ship->aiState == kAIStateGoHome;
}

/* IsShipInForm (mac _IsShipInForm, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _IsShipInForm i386 0x00082f3e ppc 0x0008d304
Boolean IsShipInForm(ShipObj *ship)
{
    return ship->aiState == kAIStateFlyPr;
}

/* IsShipInCloseForm (mac _IsShipInCloseForm). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410ee0
// MAC: _IsShipInCloseForm i386 0x00082f54 ppc 0x0008d31c
Boolean IsShipInCloseForm(ShipObj *ship)
{
    if (ship->aiSubstate == kSubFormFly)
        return 1;
    return 0;
}

/* IsShipAttacking. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00410f00
// MAC: _IsShipAttacking i386 0x00082f6a ppc 0x0008d334
Boolean IsShipAttacking(ShipObj *ship)
{
    if (ship->aiState == kAIStateFightSh)
        return 1;
    return 0;
}

/* IsShipRetreating (mac _IsShipRetreating, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _IsShipRetreating i386 0x00082f80 ppc 0x0008d34c
Boolean IsShipRetreating(ShipObj *ship)
{
    return ship->aiState == kAIStateDefRet;
}

/* IsShipWaiting (mac _IsShipWaiting, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _IsShipWaiting i386 0x00082f96 ppc 0x0008d364
Boolean IsShipWaiting(ShipObj *ship)
{
    return ship->aiState == kAIStateWait;
}

/* AIEvalHopelessChase (mac _AIEvalHopelessChase). STATE: MATCH 100.0%, exe 265 / obj 265,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x00410f20
// MAC: _AIEvalHopelessChase i386 0x00080ea9 ppc 0x0008ad04
Boolean AIEvalHopelessChase(ShipObj *ship)
{
    FloatPoint zero;
    FloatPoint delta;
    short a1;
    short a2;
    short i;
    short found;
    float distSq;
    float absDistSq;

    if (ship->targetShip == -1)
        return 0;
    if (ship->curSystem != shipData[ship->targetShip].curSystem
            || !shipData[ship->targetShip].active)
        return 0;
    if (shipData[ship->targetShip].index != 0
            && shipData[ship->targetShip].aiState != kAIStateDefRet)
        return 0;
    if (shipClass[ship->type].mass < 100)
        return 0;

    zero.x = zero.y = 0;
    delta.x = shipData[ship->targetShip].vel.x - ship->vel.x;
    delta.y = shipData[ship->targetShip].vel.y - ship->vel.y;
    if (FloatAbs(delta.x) <= 0.35 && FloatAbs(delta.y) <= 0.35)
        return 0;

    a1 = CalcInterceptAngle(&zero, &delta);
    a2 = CalcInterceptAngle(&shipData[ship->targetShip].pos,
                      &ship->pos);
    if (abs(a1 - a2) < 90)
        return 0;

    found = -1;

    absDistSq = FloatAbs(XYFloatSquaredDist(&ship->pos, &shipData[ship->targetShip].pos));
    for (i = 0; i < 256; i++) {
        if (ship->weap[i].count > 0
                && (ship->weap[i].ammo > 0 || ship->weap[i].ammo == -1)
                && weaponStats[i].guidance == kGuidanceHoming
                && SuitableMissileType(i, &shipData[ship->targetShip])
                && WeaponHasAmmo(ship, i)) {
            if (weaponStats[i].totalRange * weaponStats[i].totalRange >= absDistSq * 0.8) {
                found = i;
                break;
            }
        }
    }

    if (found == -1) {
        if (shipClass[ship->type].maxSpeed
                > shipClass[shipData[ship->targetShip].type].maxSpeed)
            return 0;
        return 1;
    } else {
        if (shipClass[ship->type].maxSpeed
                >= shipClass[shipData[ship->targetShip].type].maxSpeed)
            return 0;
        return 1;
    }
}

/* AIIsShipBusy (mac _AIIsShipBusy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411270
// MAC: _AIIsShipBusy i386 0x00082fac ppc 0x0008d37c
Boolean AIIsShipBusy(ShipObj *ship)
{
    if ((int)ship->aiState != kAIStateHiJack && (int)ship->aiState != kAIStateHypOut && (int)ship->aiState != kAIStateGoStel
            && (int)ship->aiState != kAIStateGoHGate && (int)ship->aiState != kAIStateInspect)
        return 1;
    return 0;
}

/* AIIsShipLeaving. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004112a0
// MAC: _AIIsShipLeaving i386 0x00082fe5 ppc 0x0008d3b8
Boolean AIIsShipLeaving(ShipObj *ship)
{
    if (ship->aiState == kAIStateHypOut)
        return 1;
    return 0;
}

/*
 * shipHelpRequests (0x005d9bcc, mac name) is a 20 x 256 table of Pascal
 * phrases; entry Rand(20) is appended after ":  ". The class name is also
 * copied unconditionally first - the exe really does copy it twice.
 */

/* AICallForHelp. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004112c0
// MAC: _AICallForHelp i386 0x00082ffb ppc 0x0008d3d0
void AICallForHelp(ShipObj *ship)
{
    char msg[255];
    char name[256];

    if (ship->persType != -1)
        return;
    if (ship->govt != -1 && (govtData[ship->govt].flags2 & kGovtSilent))
        return;
    if (shipClass[ship->type].inherentGovtB != -1
            && (govtData[shipClass[ship->type].inherentGovtB].flags2 & kGovtSilent))
        return;
    if (commCount <= 0 && AIDoesShipLikePlayer(ship)
            && !ExtendedIsThreatToShip(ship, shipData)
            && !IsDying(shipData) && !ShipIsCloaked(ship)) {
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    strncpy(name, (char *)shipClass[ship->type].name, 0x3f);
    if (ship->specialMission != -1) {
        if (currentMissions[ship->specialMission].shipName[0] > 0)
            strncpy(name, currentMissions[ship->specialMission].shipName, 0x3f);
    } else if (ship->persType != -1) {
        strncpy(name, personData[ship->persType].name, 0x3f);
    } else {
        strncpy(name, (char *)shipClass[ship->type].name, 0x3f);
    }
    PtoCstr((unsigned char *)name);
    strcpy(msg, name);
    strcat(msg, ":  ");
    pstrcpy((unsigned char *)name, shipHelpRequests[Rand(0x14)]);
    PtoCstr((unsigned char *)name);
    strcat(msg, name);
    CtoPstr(msg);
    DisplayComm((PStrChar *)msg, 0xf0, 0, 12, &rgbWhite, 0, 0);
    }
}

/* IsShipScanningPlayer (mac _IsShipScanningPlayer, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _IsShipScanningPlayer i386 0x00083258 ppc 0x0008d638
Boolean IsShipScanningPlayer(ShipObj *ship)
{
    if (ship->active != 0 && ship->aiState == kAIStateInspect && ship->targetShip == 0
            && FloatAbs(ship->pos.x - shipData->pos.x) <= kScanRange
            && FloatAbs(ship->pos.y - shipData->pos.y) <= kScanRange)
        return 1;
    return 0;
}

/* AIEscortFireUnprovoked (mac _AIEscortFireUnprovoked). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411540
// MAC: _AIEscortFireUnprovoked i386 0x00080e4f ppc 0x0008ac68
void AIEscortFireUnprovoked(ShipObj *ship)
{
    short target;

    if (ship->aiType < 5)
        return;
    target = ship->targetShip;
    if (target != -1) {
        if (shipData[target].active != 0 && !IsDisabled(&shipData[target]))
            AIFireTurret(ship);
        else
            ship->targetShip = -1;
    }
}

/* AIIsShipAttemptingDisable. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004115a0
// MAC: _AIIsShipAttemptingDisable i386 0x000832e6 ppc 0x0008d6ec
Boolean AIIsShipAttemptingDisable(ShipObj *ship)
{
    if (ship->aiState == kAIStatePlunder)
        return 1;
    return 0;
}

/* AIPlunderIfPossible (mac _AIPlunderIfPossible, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _AIPlunderIfPossible i386 0x000832fc ppc 0x0008d704
void AIPlunderIfPossible(ShipObj *ship)
{
    short target;

    target = ship->targetShip;
    if (target >= 0 && ship->govt != -1
            && (govtData[ship->govt].flags & kGovtPlunders) != 0
            && shipClass[ship->type].crew > 0
            && (target == 0 || (shipData[target].aiType <= 2
                                && shipData[target].specialMission == -1
                                && shipData[target].boarded == 0)))
        ship->aiState = kAIStatePlunder;
}

/* AIMakeShipStopRefuelling. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004115c0
// MAC: _AIMakeShipStopRefuelling i386 0x0008339b ppc 0x0008d7b8
void AIMakeShipStopRefuelling(ShipObj *ship)
{
    if ((int)ship->aiState == kAIStateRefuel || (int)ship->aiState == kAIStateRepair) {
        ship->targetStellar = -1;
        ship->targetShip = -1;
        ship->aiTimer = 0;
        ship->aiState = kAIStateHiJack;
        ship->aiSubstate = kSubLoJack;
    }
}

#define kRangeSlack          32

/* AIInGunRange. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411600
// MAC: _AIInGunRange i386 0x0007f9d5 ppc 0x000895dc
Boolean AIInGunRange(ShipObj *me, ShipObj *target, short weapType)
{
    long distSq;
    long dx;
    long dy;
    long range;
    long i;

    dx = abs(me->pos.x - target->pos.x);
    dy = abs(me->pos.y - target->pos.y);
    distSq = dx * dx + dy * dy;
    if (weapType == -1) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (weaponStats[i].guidance <= kGuidanceRearQuad
                && shipClass[me->type].weapCount[i] > 0) {
                if (weaponStats[i].guidance == kGuidanceBeam
                    || weaponStats[i].guidance == kGuidanceTurretBeam)
                    range = weaponStats[i].beamLength + kRangeSlack;
                else
                    range = (weaponStats[i].totalRange + kRangeSlack);
                range *= range;
                if (distSq <= range)
                    return 1;
            }
        }
    } else {
        if (weaponStats[weapType].guidance == kGuidanceBeam
            || weaponStats[weapType].guidance == kGuidanceTurretBeam)
            range = weaponStats[weapType].beamLength + kRangeSlack;
        else
            range = (weaponStats[weapType].totalRange + kRangeSlack);
        range *= range;
        if (distSq <= range)
            return 1;
    }
    return 0;
}

#define kMinShieldWeight        0.25
#define kMaxShieldWeight        1.0

/* AIShipFriendStrength. STATE: MATCH 100.0%, exe 192 / obj 192; byte identity unchecked
   until the next regate (bytediff covers baseline MATCHes only).
   OPEN: none. The outer strength is read through `strength', declared first. */
// FUNCTION: EVNOVA 0x00411800
// MAC: _AIShipFriendStrength i386 0x000825a8 ppc 0x0008c89c
short AIShipFriendStrength(ShipObj *ship)
{
    short i;
    short strength;
    short total;
    float ratio;
    float cap;

    cap = ShipShieldCapacity(ship);
    if (cap > 0.0f)
        ratio = ship->shield / cap;
    else
        ratio = ship->shield;
    if (ratio > kMaxShieldWeight)
        ratio = kMaxShieldWeight;
    if (ratio < kMinShieldWeight)
        ratio = kMinShieldWeight;
    strength = shipClass[ship->type].strength;
    total = (ratio * strength);

    for (i = 0; i < kMaxShips; i++) {
        if (i != ship->index && shipData[i].active != 0
                && IsDying(&shipData[i]) == 0) {
            strength = 0;
            if (shipData[i].parentShip == ship->index) {
                strength = shipClass[shipData[i].type].strength;
            }
            else if (ship->index == 0) {
                if (GovtAllies(shipClass[ship->type].inherentGovt,
                               shipData[i].govt) != 0)
                    strength = shipClass[shipData[i].type].strength;
            }
            else if (GovtAllies(ship->govt, shipData[i].govt) != 0) {
                strength = shipClass[shipData[i].type].strength;
            }

            cap = ShipShieldCapacity(&shipData[i]);
            if (cap > 0.0f)
                ratio = shipData[i].shield / cap;
            if (ratio > kMaxShieldWeight)
                ratio = kMaxShieldWeight;
            if (ratio < kMinShieldWeight)
                ratio = kMinShieldWeight;
            if (strength > 0
                    && IsShipThreatenedByMyEnemy(&shipData[i], ship) != 0)
                strength *= 2;
            total += strength * ratio;
        }
    }
    return total;
}

/* AIDoSwarming (mac _AIDoSwarming). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411ae0
// MAC: _AIDoSwarming i386 0x00080250 ppc 0x00089f54
Boolean AIDoSwarming(ShipObj *ship)
{
    if ((shipClass[ship->type].flags2 & kShipSwarms) == 0 || ship->defenseStellar != -1)
        return 0;
    if (ship->swarmLeader > 0 && ship->swarmLeader != ship->parentShip) {
        ship->aiSubstate = kSubChaseSwarm;
        return 1;
    }
    return 0;
}


/* AIVerifySwarmLeader (mac _AIVerifySwarmLeader). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411b40
// MAC: _AIVerifySwarmLeader i386 0x0007e2fe ppc 0x00087f2c
Boolean AIVerifySwarmLeader(ShipObj *ship)
{
    short leader;

    if ((shipClass[ship->type].flags2 & kShipSwarms) == 0)
        return 1;
    leader = ship->swarmLeader;
    if (leader >= 1 && leader < ship->index && shipData[leader].active != 0
        && (shipClass[shipData[leader].type].flags2 & kShipSwarms) != 0
        && ship->targetShip == shipData[leader].targetShip
        && ((shipData[leader].govt == ship->govt && ship->govt != -1)
            || (shipData[leader].parentShip == ship->parentShip
                && ship->parentShip != -1)))
        return 1;
    return 0;
}

/* AIFindSwarmLeader (mac _AIFindSwarmLeader). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411c20
// MAC: _AIFindSwarmLeader i386 0x0007e39e ppc 0x00087ffc
short AIFindSwarmLeader(ShipObj *ship)
{
    short i;

    ship->swarmLeader = -1;
    for (i = 1; i < ship->index; i++) {
        if (shipData[i].active != 0
            && (shipClass[shipData[i].type].flags2 & kShipSwarms) != 0
            && ship->targetShip == shipData[i].targetShip
            && ((shipData[i].govt == ship->govt && ship->govt != -1)
                || (shipData[i].parentShip == ship->parentShip
                    && ship->parentShip != -1))) {
            ship->swarmLeader = i;
            return i;
        }
    }
    return -1;
}

/*
 * flags2 cloak-behaviour bits: 0x100 while any mounted weapon's reload
 * countdown exceeds its base reload, 0x200/0x400/0x800/0x2000 on aiState/
 * aiSubstate sets, 0x1000 keeps a cloaked ship cloaked while beyond 165px of
 * its target on either axis. 0x00575000 is 0.0f, 0x005750b0 is 165.0f.
 */

#define kCloakDistance          165.0f

/* ShipRec flags2 cloak-behaviour bits (meanings behavioural, names TBC) */
#define kCloakWhenReloading     0x100
#define kCloakStateSet1         0x200
#define kCloakStateSet2         0x400
#define kCloakStateSet3         0x800
#define kCloakUntilClose        0x1000
#define kCloakWhenIdle          0x2000

/* AIHandleCloaking. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00411d00
// MAC: _AIHandleCloaking i386 0x000833e1 ppc 0x0008d7f4
void AIHandleCloaking(ShipObj *ship)
{
    Boolean wantCloak;
    Boolean found;
    short i;

    if (!HasCloak(ship) || IsDisabled(ship)) {
        if (ship->cloak > 0.0f)
            DoShipUncloak(ship);
        return;
    }
    if (CloakFuelUsage(ship) > 0 && ship->fuel <= 0.0f) {
        DoShipUncloak(ship);
        return;
    }
    if (CloakShieldUsage(ship) > 0 && ship->shield <= 0.0f) {
        DoShipUncloak(ship);
        return;
    }

    wantCloak = 0;
    if (shipClass[ship->type].flags2 & kCloakWhenReloading) {
        found = 0;
        for (i = 0; i < 256; i++) {
            if (ship->weap[i].count > 0
                    && ship->weap[i].reloadTimer > weaponStats[i].reload) {
                found = 1;
                break;
            }
        }
        if (found)
            wantCloak = 1;
    }
    if (shipClass[ship->type].flags2 & kCloakStateSet1) {
        if ((int)ship->aiState == kAIStateDefRet)
            wantCloak = 1;
        if ((int)ship->aiState == kAIStateAtkAster && ship->aiSubstate == kSubChaseAst)
            wantCloak = 1;
    }
    if (shipClass[ship->type].flags2 & kCloakStateSet2) {
        if (ship->aiSubstate == kSubHypJmp || (int)ship->aiState == kAIStateJumpIn
                || (int)ship->aiState == kAIStateHypPr)
            wantCloak = 1;
    }
    if (shipClass[ship->type].flags2 & kCloakStateSet3) {
        if ((int)ship->aiState == kAIStateGoStel || (int)ship->aiState == kAIStateWait
                || (int)ship->aiState == kAIStateGoHGate || (int)ship->aiState == kAIStateInspect
                || (int)ship->aiState == kAIStateFlyPr)
            wantCloak = 1;
    }
    if (shipClass[ship->type].flags2 & kCloakUntilClose) {
        if (ShipIsCloaked(ship) && ship->targetShip != -1 && ship->aiState == kAIStateFightSh
                && (FloatAbs(ship->pos.x - shipData[ship->targetShip].pos.x) > kCloakDistance
                    || FloatAbs(ship->pos.y - shipData[ship->targetShip].pos.y) > kCloakDistance))
            wantCloak = 1;
        if (ShipIsCloaked(ship) && ship->aiState == kAIStateAtkAster
                && ship->aiSubstate != kSubKillAst)
            wantCloak = 1;
    }
    if (shipClass[ship->type].flags2 & kCloakWhenIdle) {
        if (ship->aiState == kAIStateHiJack)
            wantCloak = 1;
    }
    if (ship->aiType >= 5 && ship->parentShip != -1 && ship->aiSubstate == kSubFormFly
            && ShipIsCloaked(&shipData[ship->parentShip]))
        wantCloak = 1;

    if (wantCloak)
        DoShipCloak(ship);
    else
        DoShipUncloak(ship);
}

/* FindNearestThreatToParent. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00412030
// MAC: _FindNearestThreatToParent i386 0x00083861 ppc 0x0008dd60
short FindNearestThreatToParent(ShipObj *me, short spob)
{
    short i;
    short best;
    int bestScore;
    int score;

    best = -1;
    bestScore = -1;
    for (i = 0; i < kMaxShips; i++) {
        score = IsShipANearbyThreatToParent(&shipData[i], me, spob);
        if (score > 0 && (score < bestScore || bestScore < 0)) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

/* IsShipANearbyThreatToParent. STATE: MATCH 100.0%, exe 199 / obj 199, byte-identical yes,
   relocregcheck 0 of 11. The range test is written `range <= 0' arm first.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00412090
// MAC: _IsShipANearbyThreatToParent i386 0x000836a4 ppc 0x0008db5c
long IsShipANearbyThreatToParent(ShipObj *ship, ShipObj *me, short range)
{
    long dist;
    long score;
    short aiType1;
    short aiType2;
    Boolean threat;

    if (ship->index == me->index)
        goto reject;
    if (ship->index == ship->parentShip)
        goto reject;
    if (ship->index == me->parentShip)
        goto reject;
    if (ship->parentShip == me->parentShip)
        goto reject;
    if (!ship->active)
        goto reject;
    if (!ShipVisibleToShip(ship, me))
        goto reject;
    if (IsDisabled(ship)) {
        if (ship->escortOrder != 2)
            goto reject;
    }

    if (me->parentShip == 0)
        threat = ExtendedIsThreatToPlayer(ship);
    else
        threat = ExtendedIsThreatToShip(&shipData[me->parentShip], ship);
    if (!threat)
        goto reject;

    dist = XYFloatSquaredDist(&shipData[me->parentShip].pos,
                                    &ship->pos);
    aiType1 = shipClass[ship->type].escortType;
    aiType2 = shipClass[me->type].escortType;
    if (range <= 0) {
        score = XYFloatSquaredDist(&me->pos,
                                         &ship->pos);
        if (aiType1 != aiType2) {
            score /= 2;
            if (aiType1 == 0 && aiType2 == 2)
                score /= 2;
        }
        if (score < 1)
            score = 1;
        return score;
    } else {
        if (dist > range * range)
            goto reject;
        score = dist + XYFloatSquaredDist(&me->pos, &ship->pos);
        if (aiType1 != aiType2) {
            score /= 2;
            if (aiType1 == 0 && aiType2 == 2)
                score /= 2;
        }
    }
    if (score < 1)
        score = 1;
    return score;

reject:
    return 0;
}

#define kModPlunderOdds     0x19    /* outfit mod: shifts the capture odds by modVal */

#define kStrCargoWord       0x175   /* follows "tons of" (text TBC) */
#define kStrPlunderTail     0x176   /* ends both messages (text TBC) */
#define kStrShipSeized      0xa9    /* formerEscort == 0 (text TBC) */
#define kStrShipSeizedAlt   0xa8

/* AIFindShipToPlunder. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00412330
// MAC: _AIFindShipToPlunder i386 0x0007fd10 ppc 0x000899e0
void AIFindShipToPlunder(ShipObj *ship)
{
    short i;
    short best;
    long bestDist;
    long dist;
    Boolean armed;

    best = -1;
    armed = AIHasDestroyingWeapons(ship);
    for (i = 0; i < kMaxShips; i++) {
        if (i != ship->index && shipData[i].parentShip != ship->index
                && shipData[i].active != 0 && shipData[i].boarded == 0
                && IsDisabled(&shipData[i]) != 0 && shipData[i].specialMission == -1
                && (i == 0 || shipData[i].formerEscort >= 0
                    || shipData[i].aiType <= 2
                    || shipClass[shipData[i].type].inherentAI <= 2 || armed != 0)
                && shipClass[shipData[i].type].crew > 0
                && (shipData[i].parentShip == 0
                    || GovtAllies(ship->govt, shipData[i].govt) == 0)) {
            dist = XYFloatSquaredDist(&ship->pos, &shipData[i].pos);
            if (dist < bestDist || best == -1) {
                bestDist = dist;
                best = i;
            }
        }
    }
    if (best != -1) {
        ship->targetShip = best;
        ship->aiState = kAIStatePlunder;
        ship->aiSubstate = kSubLoJack;
    }
}

/* AIShipIsPlunderingShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004124f0
// MAC: _AIShipIsPlunderingShip i386 0x00083f5e ppc 0x0008e5cc
Boolean AIShipIsPlunderingShip(ShipObj *ship, ShipObj *other)
{
    if (((int)ship->aiState == kAIStatePlunder
                || ((int)ship->aiState == kAIStateFightSh && ship->aiSubstate == kSubBoard))
            && ship->targetShip == other->index)
        return 1;
    return 0;
}

/* AIShipIsDocking. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00412530
// MAC: _AIShipIsDocking i386 0x00083f95 ppc 0x0008e60c
Boolean AIShipIsDocking(ShipObj *ship)
{
    if (ship->aiSubstate == kSubBoard)
        return 1;
    return 0;
}

/* AIPlunderShipContents (mac _AIPlunderShipContents). STATE: MATCH 100.0%, 667 exact /
   102 reloc-equivalent / 0 mismatched, exe 769 / obj 769, relocregcheck 0 of 102 differ,
   byte-identical yes (bytediff, baseline c8b0e3e6). OPEN: none */
// FUNCTION: EVNOVA 0x00412550
// MAC: _AIPlunderShipContents i386 0x00083fab ppc 0x0008e624
void AIPlunderShipContents(ShipObj *me, ShipObj *target)
{
    unsigned char msg[256];
    unsigned char buf2[256];
    short j;
    short i;
    short avail;
    short room;
    short maxTake;
    short taken;
    short myStr;
    short targStr;
    long credits;
    short oddsL;

    if (IsDying(target) != 0 || target->active == 0)
        return;
    avail = 0;
    taken = 0;
    room = shipClass[me->type].cargo;
    if (target->index == 0)
        maxTake = ShipTotalHolds();
    else
        maxTake = shipClass[target->type].cargo;
    for (i = 0; i < 6; i++) {
        if (me->cargo[i] > 0)
            room -= me->cargo[i];
        if (target->cargo[i] > 0)
            avail += target->cargo[i];
    }
    if (avail > maxTake)
        avail = maxTake;
    myStr = shipClass[me->type].strength;
    targStr = shipClass[target->type].strength;
    /* BUG: all four defOutfID loops weigh outfitSystem[i].modVal[j], not the tested outfit's */
    for (i = 0; i < 8; i++) {
        if (shipClass[me->type].defOutfCount[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[shipClass[me->type].defOutfID[i]].modType[j] == kModPlunderOdds
                        && outfitSystem[shipClass[me->type].defOutfID[i]].modVal[j] > 0)
                    myStr += outfitSystem[i].modVal[j] * shipClass[me->type].defOutfCount[i];
            }
        }
    }
    if (target->index == 0) {
        for (i = 0; i < 512; i++) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kModPlunderOdds && outfitSystem[i].modVal[j] > 0)
                    targStr += outfitSystem[i].modVal[j] * shipSystems[i];
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[target->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[target->type].defOutfID[i]].modType[j] == kModPlunderOdds
                            && outfitSystem[shipClass[target->type].defOutfID[i]].modVal[j] > 0)
                        targStr += outfitSystem[i].modVal[j] * shipClass[target->type].defOutfCount[i];
                }
            }
        }
    }
    if (targStr <= 0)
        targStr = 1;
    oddsL = (myStr * 100.0) / (float)(targStr * 2.0);
    for (i = 0; i < 8; i++) {
        if (shipClass[me->type].defOutfCount[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[shipClass[me->type].defOutfID[i]].modType[j] == kModPlunderOdds
                        && outfitSystem[shipClass[me->type].defOutfID[i]].modVal[j] < 0)
                    oddsL -= outfitSystem[i].modVal[j] * shipClass[me->type].defOutfCount[i];
            }
        }
    }
    if (target->index == 0) {
        for (i = 0; i < 512; i++) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kModPlunderOdds && outfitSystem[i].modVal[j] < 0)
                    oddsL += outfitSystem[i].modVal[j] * shipSystems[i];
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[target->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[target->type].defOutfID[i]].modType[j] == kModPlunderOdds
                            && outfitSystem[shipClass[target->type].defOutfID[i]].modVal[j] < 0)
                        oddsL += outfitSystem[i].modVal[j] * shipClass[target->type].defOutfCount[i];
                }
            }
        }
    }
    oddsL += 10 - Rand(21);
    if (oddsL < 10)
        oddsL = 10;
    if (oddsL > 100)
        oddsL = 100;
    while (room > 0 && avail > 0) {
        i = Rand(6);
        if (target->cargo[i] > 0) {
            short amount;

            amount = target->cargo[i];
            if (amount > room)
                amount = room;
            if (amount + taken > maxTake)
                amount = maxTake - taken;
            if (amount < 0)
                amount = 0;
            target->cargo[i] -= amount;
            me->cargo[i] += amount;
            avail -= amount;
            room -= amount;
            taken += amount;
        }
    }
    if (target->index == 0) {
        credits = (target->credits * (oddsL * 30 * 0.01) * 0.01);
        me->credits += credits;
        target->credits -= credits;
    } else {
        target->credits = 0;
        credits = 0;
    }
    if (target->index == 0
            || (target->parentShip == -1 && target->specialMission == -1
                && target->formerEscort >= 1)) {
        statusRedraw[2] = 1;
        strcpy((char *)msg, "");
        if (taken > 0 || credits > 0) {
            if (taken > 0) {
                StrcatCommaNumber(taken, (char *)msg);
                strcat((char *)msg, " ");
                if (taken == 1)
                    pstrcpy(buf2, tonString);
                else
                    pstrcpy(buf2, tonsString);
                PtoCstr(buf2);
                strcat((char *)msg, (char *)buf2);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, kStrOf);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, kStrCargoWord);
                strcat((char *)msg, " ");
                if (credits > 0) {
                    StrcatIndCString((char *)msg, kGameStrList, kStrAnd);
                    strcat((char *)msg, " ");
                }
            }
            if (credits > 0) {
                if (taken <= 0)
                    strcpy((char *)msg, "");
                StrcatCommaNumber(credits, (char *)msg);
                strcat((char *)msg, " ");
                if (credits > 1)
                    strcat((char *)msg, creditsCString);
                else
                    strcat((char *)msg, creditCString);
                strcat((char *)msg, " ");
            }
            StrcatIndCString((char *)msg, kGameStrList, kStrPlunderTail);
            CtoPstr((char *)msg);
            ST_PlaySound(beepSnd[1], 1, gSoundVolume);
            DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
        }
    }
    if (target->index != 0 && target->specialMission == -1
            && ((oddsL > 40 && Rand(101) <= oddsL * 0.5) || cheatsActive != 0)) {
        if (target->parentShip == 0 || target->formerEscort >= 0) {
            if (target->formerEscort == 0)
                GetIndString(msg, kGameStrList, kStrShipSeized);
            else
                GetIndString(msg, kGameStrList, kStrShipSeizedAlt);
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active != 0
                        && shipData[i].targetShip == target->index) {
                    shipData[i].targetShip = -1;
                    shipData[i].targetStellar = -1;
                    shipData[i].aiState = kAIStateHiJack;
                    shipData[i].aiSubstate = kSubLoJack;
                    shipData[i].grudge = 0;
                }
            }
            PtoCstr(msg);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, kStrPlunderTail);
            CtoPstr((char *)msg);
            ST_PlaySound(beepSnd[1], 1, gSoundVolume);
            DisplayComm((PStrChar *)msg, 400, 0, 0xc, &rgbWhite, 0, 0);
        }
        target->parentShip = me->index;
        target->govt = me->govt;
        target->persType = -1;
        target->aiType = 6;
        target->formerEscort = -1;
        target->aiState = kAIStateHiJack;
        target->aiSubstate = kSubLoJack;
        target->boarded = 0;
        target->aiTimer = 150.0f;
        target->armor = ShipArmorCapacity(target) * 0.66;
        target->shield = 0;
    }
    if (target->index == 0) {
        for (i = 0; i < kMaxActiveMissions; i++) {
            if (currentMissionObjectives[i].active != 0
                    && currentMissionObjectives[i].failed == 0
                    && (currentMissions[i].flags & kMissionFailIfBoarded) != 0)
                DoMissionFailure(i);
        }
    }
}

/*
 * A per-govt byte (govtData[].reinfInhibited, distinct from iffScrambled)
 * blocks arming and marks the system pending (unkC4=1). systemData.unkC8 is a
 * float: fld/fstp'd here, only ever zeroed elsewhere.
 */

#define kKillsPerStrength 0x1900

/* AICalculateOddsAgainst. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004133f0
// MAC: _AICalculateOddsAgainst i386 0x00084a2e ppc 0x0008f038
// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
void AICalculateOddsAgainst(ShipObj *ship)
{
    float playerWeight;
    short i;
    short friendly;
    short enemy;

    friendly = shipClass[ship->type].strength;
    enemy = 0;
    if (ExtendedIsThreatToShip(ship, shipData)) {
        playerWeight = (playerKills
                               / (shipClass[0].strength * kKillsPerStrength));
        if (playerWeight < 1.0)
            playerWeight = 1.0;
        if (playerWeight > 2.0)
            playerWeight = 2.0;
        enemy += shipClass[shipData->type].strength * playerWeight;
    }

    for (i = 1; i < kMaxShips; i++) {
        if (i != ship->index && shipData[i].active
            && !IsDisabled(&shipData[i])) {
            if (GovtAllies(shipData[i].govt, ship->govt)) {
                friendly += shipClass[shipData[i].type].strength;
            } else if (GovtEnemies(shipData[i].govt, ship->govt)
                       || (shipData[i].targetShip == ship->index
                           && shipData[i].aiState == kAIStateFightSh)) {
                enemy += shipClass[shipData[i].type].strength;
            }
        }
    }

    if (friendly == 0)
        friendly = 1;
    ship->odds = enemy / (float)friendly;
}

/* AICallForReinforcements. STATE: MATCH 100.0%, exe 161 / obj 161, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00413610
// MAC: _AICallForReinforcements i386 0x0007f5a1 ppc 0x000890ec
// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
Boolean AICallForReinforcements(ShipObj *ship, Boolean force)
{
    float threshold;
    Boolean found;

    if (ship->govt == -1)
        return 0;

    if (systemData[ship->curSystem].reinforceFleet == -1)
        return 0;

    if (systemData[ship->curSystem].reinforceTimer > 0.0f)
        return 0;

    if (systemData[ship->curSystem].reinforceDaysLeft > 0)
        return 0;

    if (ship->odds <= 0.0f)
        return 0;

    if (!GovtAllies(ship->govt, fleetData[systemData[ship->curSystem].reinforceFleet].govt))
        return 0;

    threshold = govtData[fleetData[systemData[ship->curSystem].reinforceFleet].govt].maxOdds * 0.5;
    if (ship->odds > threshold || force) {
        found = allReinforcementsInhibited;
        if (!found) {
            if (ship->govt != -1 && govtData[ship->govt].reinfInhibited != 0)
                found = 1;
        }
        if (!found) {
            if (fleetData[systemData[ship->curSystem].reinforceFleet].govt != -1
                    && govtData[fleetData[systemData[ship->curSystem].reinforceFleet].govt].reinfInhibited != 0)
                found = 1;
        }
        if (found) {
            systemData[ship->curSystem].reinforceDaysLeft = 1;
            return 0;
        }
        systemData[ship->curSystem].reinforceTimer = systemData[ship->curSystem].reinforceTime;
        return 1;
    }
    return 0;
}

/* AIForceBurstReload (mac _AIForceBurstReload). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00413810
// MAC: _AIForceBurstReload i386 0x00084c37 ppc 0x0008f2d0
void AIForceBurstReload(ShipObj *ship)
{
    short i;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (shipClass[ship->type].weapCount[i] > 0
            && weaponStats[i].burstCount > 0 && weaponStats[i].burstReload > 0) {
            ship->weap[i].burstCount = 0;
            ship->weap[i].reloadTimer = weaponStats[i].burstReload;
        }
    }
}

/* AIShipIsOutOfAmmo (mac _AIShipIsOutOfAmmo). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004138a0
// MAC: _AIShipIsOutOfAmmo i386 0x0007f71f ppc 0x0008928c
short AIShipIsOutOfAmmo(ShipObj *ship)
{
    short i;
    short out = 0;
    short ammoUsing = 0;
    short present = 0;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count > 0) {
            present++;
            if (weaponStats[i].ammoType >= 0
                    || weaponStats[i].ammoType < -kWeapAmmoFuelBase) {
                ammoUsing++;
                if (weaponStats[i].ammoType >= 0) {
                    if (ship->weap[i].ammo <= 0)
                        out++;
                } else if (ship->fuel < abs(weaponStats[i].ammoType) - kWeapAmmoFuelBase) {
                    out++;
                }
            }
        }
    }
    if (present == 0)
        return 2;
    if (out > 0 && out == present)
        return 2;
    if (out > 0 && out == ammoUsing)
        return 1;
    return 0;
}

/* AICalcFormationPositions (mac _AICalcFormationPositions). STATE: MATCH 100.0%, exe 131 / obj 131,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00413990
// MAC: _AICalcFormationPositions i386 0x00085040 ppc 0x0008ffb8
// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
void AICalcFormationPositions(ShipObj *ship, Boolean recalc)
{
    short escortCount;
    short validCount;
    short i;
    short radius;
    short type;
    short dist;

    validCount = 1;
    radius = kMaxShips;
    type = ship->type;
    if (type >= 0 && type < kMaxShipTypes) {
        if (shipClass[type].spriteIndex >= 0 && shipClass[type].spriteIndex < kMaxShipTypes)
            radius = GetMasterShipSpriteXSize(type);
    }

    for (i = 1; i < kMaxShips; i++) {
        if (i != ship->index && shipData[i].active && shipData[i].formLeader == ship->index) {
            dist = GetMasterShipSpriteXSize(shipData[i].type);
            if (radius < dist)
                radius = dist;
            validCount++;
        }
    }

    if (validCount == 1)
        return;

    escortCount = 1;
    radius *= 0.6;
    if (radius > 60)
        radius = 60;
    if (radius < 24)
        radius = 24;

    for (i = 1; i < kMaxShips; i++) {
        if (i != ship->index && shipData[i].active && shipData[i].formLeader == ship->index) {
            escortCount++;
            AICalcShipFormPos(&shipData[i], ship, escortCount, validCount, radius);
            if (recalc && shipData[i].aiState != kAIStateEmerge)
                AIMaintainFormation(&shipData[i], 1);
        }
    }
}

/*
 * The two float-to-int conversions are mwcc's inlined rounding idiom
 * (fist/fild/fsubp/test/sbb/setg), not source; only the second one carries a
 * real `% 360`. Slots 1-3 and 9-21 are identical in both tables; only 4..8
 * permute.
 */

#define kQuarterTurn    90.0f

/* AICalcShipFormPos. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00413b60
// MAC: _AICalcShipFormPos i386 0x00084cb8 ppc 0x0008f394
void AICalcShipFormPos(ShipObj *escort, ShipObj *leader, short escortIndex,
                       short escortCount, short spacing)
{
    float lateral;
    float behind;

    lateral = behind = 0.0f;
    if (escortCount % 2 == 0) {
        if (escortIndex == 1) {
            lateral = 0.0f;
            behind = 0.0f;
        } else if (escortIndex == 2) {
            lateral = -spacing;
            behind = -spacing;
        } else if (escortIndex == 3) {
            lateral = spacing;
            behind = -spacing;
        } else if (escortIndex == 4) {
            lateral = 0.0f;
            behind = spacing * -2;
        } else if (escortIndex == 5) {
            lateral = spacing * -2;
            behind = spacing * -2;
        } else if (escortIndex == 6) {
            lateral = spacing * 2;
            behind = spacing * -2;
        } else if (escortIndex == 7) {
            lateral = -spacing;
            behind = spacing * -3;
        } else if (escortIndex == 8) {
            lateral = spacing;
            behind = spacing * -3;
        } else if (escortIndex == 9) {
            lateral = spacing * -3;
            behind = spacing * -3;
        } else if (escortIndex == 10) {
            lateral = spacing * 3;
            behind = spacing * -3;
        } else if (escortIndex == 11) {
            lateral = 0.0f;
            behind = spacing * -4;
        } else if (escortIndex == 12) {
            lateral = spacing * -2;
            behind = spacing * -4;
        } else if (escortIndex == 13) {
            lateral = spacing * 2;
            behind = spacing * -4;
        } else if (escortIndex == 14) {
            lateral = spacing * -4;
            behind = spacing * -4;
        } else if (escortIndex == 15) {
            lateral = spacing * 4;
            behind = spacing * -4;
        } else if (escortIndex == 16) {
            lateral = -spacing;
            behind = spacing * -5;
        } else if (escortIndex == 17) {
            lateral = spacing;
            behind = spacing * -5;
        } else if (escortIndex == 18) {
            lateral = spacing * -3;
            behind = spacing * -5;
        } else if (escortIndex == 19) {
            lateral = spacing * 3;
            behind = spacing * -5;
        } else if (escortIndex == 20) {
            lateral = spacing * -5;
            behind = spacing * -5;
        } else if (escortIndex == 21) {
            lateral = spacing * 5;
            behind = spacing * -5;
        }
    } else {
        if (escortIndex == 1) {
            lateral = 0.0f;
            behind = 0.0f;
        } else if (escortIndex == 2) {
            lateral = -spacing;
            behind = -spacing;
        } else if (escortIndex == 3) {
            lateral = spacing;
            behind = -spacing;
        } else if (escortIndex == 4) {
            lateral = spacing * -2;
            behind = spacing * -2;
        } else if (escortIndex == 5) {
            lateral = spacing * 2;
            behind = spacing * -2;
        } else if (escortIndex == 6) {
            lateral = -spacing;
            behind = spacing * -3;
        } else if (escortIndex == 7) {
            lateral = spacing;
            behind = spacing * -3;
        } else if (escortIndex == 8) {
            lateral = 0.0f;
            behind = spacing * -2;
        } else if (escortIndex == 9) {
            lateral = spacing * -3;
            behind = spacing * -3;
        } else if (escortIndex == 10) {
            lateral = spacing * 3;
            behind = spacing * -3;
        } else if (escortIndex == 11) {
            lateral = 0.0f;
            behind = spacing * -4;
        } else if (escortIndex == 12) {
            lateral = spacing * -2;
            behind = spacing * -4;
        } else if (escortIndex == 13) {
            lateral = spacing * 2;
            behind = spacing * -4;
        } else if (escortIndex == 14) {
            lateral = spacing * -4;
            behind = spacing * -4;
        } else if (escortIndex == 15) {
            lateral = spacing * 4;
            behind = spacing * -4;
        } else if (escortIndex == 16) {
            lateral = -spacing;
            behind = spacing * -5;
        } else if (escortIndex == 17) {
            lateral = spacing;
            behind = spacing * -5;
        } else if (escortIndex == 18) {
            lateral = spacing * -3;
            behind = spacing * -5;
        } else if (escortIndex == 19) {
            lateral = spacing * 3;
            behind = spacing * -5;
        } else if (escortIndex == 20) {
            lateral = spacing * -5;
            behind = spacing * -5;
        } else if (escortIndex == 21) {
            lateral = spacing * 5;
            behind = spacing * -5;
        }
    }

    escort->formPos.x = 0.0f;
    escort->formPos.y = 0.0f;
    Accel(leader->heading, behind, &escort->formPos);
    Accel((short)(leader->heading + kQuarterTurn) % kFullCircle, lateral,
          &escort->formPos);
    escort->formPos.x = escort->formPos.x + leader->pos.x;
    escort->formPos.y = escort->formPos.y + leader->pos.y;
}

#define kDeadband               8.0f
#define kAccelScale             10.0
#define kDampCap                0.8

/* AIMaintainFormation (mac _AIMaintainFormation). STATE: MATCH 100.0%, exe 129 / obj 129
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00414390
// MAC: _AIMaintainFormation i386 0x00084f04 ppc 0x0008fe50
void AIMaintainFormation(ShipObj *ship, Boolean snap)
{
    float damp;
    float step;
    float dy;
#ifdef MACBUILD
    float x;
    float fy;
    float y;
    float band;
#endif

    if (ship->jumpTimer > 0.0f && snap == 0)
        return;
    if (ship->formLeader < 0 || ship->formLeader >= kMaxShips)
        return;
    step = ShipAccelRate(ship) * kAccelScale;
    if (ship->ionization > 0.0f) {
        damp = ShipIonizationFactor(ship);
        if (damp > kDampCap)
            damp = kDampCap;
        step *= 1.0 - damp;
    }
    dy = step;
    step *= gSpeedMult;
    dy *= gSpeedMult;
#ifdef MACBUILD
    band = kDeadband;
    if (snap != 0) {
        ship->pos = ship->formPos;
    }
    else {
        x = ship->pos.x;
        if (x <= ship->formPos.x - band)
            ship->pos.x = x + step;
        else if (x >= ship->formPos.x + band)
            ship->pos.x = x - step;
        y = ship->pos.y;
        fy = ship->formPos.y;
        if (y <= fy - band)
            ship->pos.y = y + dy;
        else if (y >= fy + band)
            ship->pos.y = y - dy;
    }
#else
    if (snap != 0) {
        ship->pos = ship->formPos;
    }
    else {
        if (ship->pos.x <= ship->formPos.x - kDeadband)
            ship->pos.x = ship->pos.x + step;
        else if (ship->formPos.x + kDeadband <= ship->pos.x)
            ship->pos.x = ship->pos.x - step;
        if (ship->pos.y <= ship->formPos.y - kDeadband)
            ship->pos.y = ship->pos.y + dy;
        else if (ship->formPos.y + kDeadband <= ship->pos.y)
            ship->pos.y = ship->pos.y - dy;
    }
#endif
}

/* AIShipIsScooping (mac _AIShipIsScooping). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00414530
// MAC: _AIShipIsScooping i386 0x00088727 ppc 0x00093824
Boolean AIShipIsScooping(ShipObj *ship)
{
    if (ship->aiState == kAIStateScoop)
        return 1;
    return 0;
}

#define kRangePad          32.0f
#define kHalfTurnDeg       180
#define kGunSoundFlags     4

/* FireAIShipWeapon. STATE: MATCH 100%, 910/910, relocregcheck 0 of 88.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00414550
// MAC: _FireAIShipWeapon i386 0x0008873d ppc 0x0009383c
void FireAIShipWeapon(ShipObj *ship)
{
    short ammoUsed;
    short slot;
    short shot;
    short i;
    short maxShots;
    short range;
    short shotsFired;
    short cone;
    short angle;
    float reload;

    ship->fireTick = ReadTick();
    if ((shipClass[ship->type].shanFlags & kShanFoldingParts) != 0
            && (shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) != 0
            && ship->foldSet > 0
            && shipClass[ship->type].baseSets > 1) {
        ship->foldDir = -1;
        return;
    }
    if (gameFrozen != 0)
        return;

    shotsFired = 0;
    if (ship->weap[ship->selectedWeapon].reloadTimer <= 0.0f && ship->weap[ship->selectedWeapon].count > 0) {
        maxShots = WeaponMaxSimultShots(ship, ship->selectedWeapon);
        for (shot = 0; shot < maxShots; shot++) {
            if (WeaponHasAmmo(ship, ship->selectedWeapon)) {
                if (ship->targetShip != -1) {
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceTurretBeam
                            || weaponStats[ship->selectedWeapon].guidance == kGuidanceTurret) {
                        if (!TurretBlindSpot(ship,
                                CalcInterceptAngle(&ship->pos,
                                    &shipData[ship->targetShip].pos),
                                ship->selectedWeapon)) {
                            if (weaponStats[ship->selectedWeapon].guidance == kGuidanceTurretBeam)
                                range = weaponStats[ship->selectedWeapon].beamLength + 32;
                            else if (weaponStats[ship->selectedWeapon].guidance == kGuidanceTurret)
                                range = (weaponStats[ship->selectedWeapon].totalRange
                                                + kRangePad);
                            if (FloatAbs(ship->pos.x
                                            - shipData[ship->targetShip].pos.x) < range
                                    && FloatAbs(ship->pos.y
                                            - shipData[ship->targetShip].pos.y) < range) {
                                if (weaponStats[ship->selectedWeapon].guidance == kGuidanceTurretBeam) {
                                    SpawnBeam(ship->index, ship->targetShip,
                                        ship->selectedWeapon,
                                        CalcInterceptAngle(
                                            &ship->pos,
                                            &shipData[ship->targetShip].pos),
                                        -1, -1);
                                } else {
                                    SpawnShot(ship->index, ship->targetShip, ship->selectedWeapon, 0);
                                }
                                shotsFired++;
                            }
                        }
                    }
                }

                if (ship->targetShip != -1) {
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceFrontQuad
                            || weaponStats[ship->selectedWeapon].guidance == kGuidanceRearQuad) {
                        range = (weaponStats[ship->selectedWeapon].totalRange + kRangePad);
                        if (FloatAbs(ship->pos.x
                                        - shipData[ship->targetShip].pos.x) < range
                                && FloatAbs(ship->pos.y
                                        - shipData[ship->targetShip].pos.y) < range) {
                            angle = CalcInterceptAngle(
                                        &ship->pos,
                                        &shipData[ship->targetShip].pos);
#ifdef MACBUILD
                            if (weaponStats[ship->selectedWeapon].guidance == kGuidanceFrontQuad)
                                cone = abs((long)(angle - ship->heading)) % kFullCircle;
                            if (weaponStats[ship->selectedWeapon].guidance == kGuidanceRearQuad)
                                cone = abs(angle
                                           - ((short)ship->heading + kHalfTurnDeg)
                                             % kFullCircle) % kFullCircle;
#else
                            if (weaponStats[ship->selectedWeapon].guidance == kGuidanceFrontQuad)
                                cone = (short)FloatAbs(angle
                                           - ship->heading) % kFullCircle;
                            if (weaponStats[ship->selectedWeapon].guidance == kGuidanceRearQuad)
                                cone = (short)abs(angle
                                           - ((short)ship->heading + kHalfTurnDeg)
                                             % kFullCircle) % kFullCircle;
#endif
                            if (cone <= 45) {
                                SpawnShot(ship->index, ship->targetShip, ship->selectedWeapon, 0);
                                shotsFired++;
                            }
                        }
                    }
                }

                if (ship->targetShip == -1) {
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceFrontQuad) {
                        SpawnShot(ship->index, -1, ship->selectedWeapon, 0);
                        shotsFired++;
                    }
                }
                if (ship->targetShip != -1) {
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceHoming) {
                        SpawnShot(ship->index, ship->targetShip, ship->selectedWeapon, 0);
                        shotsFired++;
                    }
                }

                if (weaponStats[ship->selectedWeapon].guidance == kGuidanceUnguided
                        || weaponStats[ship->selectedWeapon].guidance == kGuidanceBeam
                        || weaponStats[ship->selectedWeapon].guidance == kGuidanceRocket) {
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceBeam)
                        range = weaponStats[ship->selectedWeapon].beamLength + 32;
                    else if (weaponStats[ship->selectedWeapon].guidance == kGuidanceUnguided
                            || weaponStats[ship->selectedWeapon].guidance == kGuidanceRocket)
                        range = (weaponStats[ship->selectedWeapon].totalRange + kRangePad);
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceBeam)
                        SpawnBeam(ship->index, ship->targetShip, ship->selectedWeapon,
                                  ship->heading, -1, -1);
                    else
                        SpawnShot(ship->index, ship->targetShip, ship->selectedWeapon, 0);
                    shotsFired++;
                }

                if (weaponStats[ship->selectedWeapon].ammoType != -1
                        && ship->persType != kPersCaptainHector
                        && (weaponStats[ship->selectedWeapon].flags3 & kWeapAmmoAtBurstEnd) == 0) {
                    if (weaponStats[ship->selectedWeapon].guidance == kGuidanceBay
                            || (weaponStats[ship->selectedWeapon].ammoType >= 0
                                && weaponStats[ship->selectedWeapon].ammoType < kMaxWeapTypes)) {
                        ship->weap[ship->selectedWeapon].ammo -= 1;
                    } else if (weaponStats[ship->selectedWeapon].ammoType <= -kWeapAmmoFuelBase) {
                        ship->fuel -= (abs(weaponStats[ship->selectedWeapon].ammoType)
                                          - kWeapAmmoFuelBase) * 0.1;
                        if (ship->fuel < 0.0f)
                            ship->fuel = 0.0f;
                    }
                }
            }
        }
    }

    if (shotsFired > 0) {
        if ((weaponStats[ship->selectedWeapon].flags2 & kWeapUsesShipSprite) != 0)
            ship->weapGlow = 32.0f;
        if (weaponStats[ship->selectedWeapon].sound >= 0) {
            if ((weaponStats[ship->selectedWeapon].flags & kWeapLoopSound) != 0) {
                if (!ST_IsSoundPlaying(shotSnd[weaponStats[ship->selectedWeapon].sound]))
                    PlaySoundDistance(-1, shotSnd[weaponStats[ship->selectedWeapon].sound],
                                      kGunSoundFlags, &ship->pos,
                                      &shipData[0].pos);
            } else if (weaponStats[ship->selectedWeapon].guidance == kGuidanceBeam
                       || weaponStats[ship->selectedWeapon].guidance == kGuidanceTurretBeam) {
                PlaySoundDistance(3, shotSnd[weaponStats[ship->selectedWeapon].sound],
                                  kGunSoundFlags, &ship->pos,
                                  &shipData[0].pos);
            } else {
                PlaySoundDistance(-1, shotSnd[weaponStats[ship->selectedWeapon].sound],
                                  kGunSoundFlags, &ship->pos,
                                  &shipData[0].pos);
            }
        }

        if (weaponStats[ship->selectedWeapon].recoil > 0 || weaponStats[ship->selectedWeapon].recoil < -1) {
            AdjustedAccel((short)(ship->heading + kHalfTurn) % kFullCircle,
                          (float)weaponStats[ship->selectedWeapon].recoil
                              / shipClass[ship->type].mass,
                          &ship->vel,
                          shipClass[ship->type].maxSpeed);
        }

#ifdef MACBUILD
        if ((weaponStats[ship->selectedWeapon].flags & kWeapFireSimultaneously) != 0)
            reload = weaponStats[ship->selectedWeapon].reload;
        else
            reload = shotsFired * (weaponStats[ship->selectedWeapon].reload / ship->weap[ship->selectedWeapon].count);
#else
        slot = ship->selectedWeapon;
        if ((weaponStats[slot].flags & kWeapFireSimultaneously) != 0)
            reload = weaponStats[slot].reload;
        else
            reload = shotsFired * (weaponStats[slot].reload / ship->weap[slot].count);
#endif

        if (ship->targetShip == 0) {
            if (playerKills < shipClass->strength * 100)
                reload *= 1.75;
            else if (playerKills < shipClass->strength * 400)
                reload *= 1.5;
            else if (playerKills < shipClass->strength * 800)
                reload *= 1.25;
            else if (playerKills < shipClass->strength * 1600)
                reload *= 1.1;
        }

        if (weaponStats[ship->selectedWeapon].burstCount > 0) {
            ship->weap[ship->selectedWeapon].burstCount++;
            if ((weaponStats[ship->selectedWeapon].flags3 & kWeapAmmoAtBurstEnd) != 0
                    && ship->weap[ship->selectedWeapon].burstCount
                        % weaponStats[ship->selectedWeapon].burstCount == 0) {
                if ((weaponStats[ship->selectedWeapon].flags & kWeapFireSimultaneously) != 0)
                    ammoUsed = shotsFired;
                else
                    ammoUsed = 1;
                if (weaponStats[ship->selectedWeapon].guidance == kGuidanceBay
                        || (ship->weap[ship->selectedWeapon].ammo > 0
                            && (weaponStats[ship->selectedWeapon].flags3 & kWeapAmmoAtBurstEnd) != 0)) {
                    ship->weap[ship->selectedWeapon].ammo -= ammoUsed;
                    if (ship->weap[ship->selectedWeapon].ammo < 0)
                        ship->weap[ship->selectedWeapon].ammo = 0;
                } else if (weaponStats[ship->selectedWeapon].ammoType <= -kWeapAmmoFuelBase) {
                    ship->fuel -= (abs(weaponStats[ship->selectedWeapon].ammoType) - kWeapAmmoFuelBase)
                                  * 0.1 * ammoUsed;
                    if (ship->fuel < 0.0f)
                        ship->fuel = 0.0f;
                }
            }
#ifdef MACBUILD
            if (ship->weap[ship->selectedWeapon].burstCount
                    >= WeaponBurstInterval(ship->selectedWeapon,
                           ship->weap[ship->selectedWeapon].count)) {
                ship->weap[ship->selectedWeapon].burstCount = 0;
                reload = weaponStats[ship->selectedWeapon].burstReload;
            }
        }

        ship->weap[ship->selectedWeapon].reloadTimer = reload;
#else
            slot = ship->selectedWeapon;
            if (ship->weap[slot].burstCount
                    >= WeaponBurstInterval(ship->selectedWeapon,
                           ship->weap[(int)ship->selectedWeapon].count)) {
                ship->weap[slot].burstCount = 0;
                slot = ship->selectedWeapon;
                reload = weaponStats[slot].burstReload;
            }
        }

        ship->weap[slot].reloadTimer = reload;
#endif
        if ((weaponStats[ship->selectedWeapon].flags3 & kWeapExclusive) != 0) {
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (i != ship->selectedWeapon
                        && ship->weap[i].reloadTimer
                            < ship->weap[ship->selectedWeapon].reloadTimer + 2.0f)
                    ship->weap[i].reloadTimer =
                        ship->weap[ship->selectedWeapon].reloadTimer + 2.0f;
            }
        }
        if (weaponStats[ship->selectedWeapon].guidance == kGuidanceUnguided
                || weaponStats[ship->selectedWeapon].guidance == kGuidanceRocket)
            ship->aimWeapon = ship->selectedWeapon;
    }
}

#define kAITypes                4
#define kEscortMinStrength      5

/* AIIssueEscortOrders. STATE: MATCH 100.0%, exe 217 / obj 217 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004152e0
// MAC: _AIIssueEscortOrders i386 0x0008986d ppc 0x00094c04
void AIIssueEscortOrders(ShipObj *ship)
{
    short advice[kAITypes];
    Boolean theyCanHurtMe;
    Boolean iCanHurtThem;
    Boolean preyDisabled;
    short state;
    short substate;
    short i;

    for (i = 0; i < kAITypes; i++)
        advice[i] = -1;
    if (ship->aiType <= 2) {
        if (ship->shield < ShipShieldCapacity(ship) * 0.66) {
            advice[0] = kOrderDefend;
        }
        else {
            advice[0] = kOrderAttack;
        }
        advice[1] = kOrderDefend;
        advice[2] = kOrderDefend;
        advice[3] = kOrderFormation;
    }
    else {
        theyCanHurtMe = 0;
        iCanHurtThem = 0;
        preyDisabled = 0;
        if (ship->targetShip != -1) {
            if (IsShipFightingMe(&shipData[ship->targetShip], ship) != 0) {
                theyCanHurtMe = AIInGunRange(&shipData[ship->targetShip], ship, 1);
                iCanHurtThem = AIInGunRange(ship, &shipData[ship->targetShip], 1);
            }
            preyDisabled = IsDisabled(&shipData[ship->targetShip]);
        }

        if (preyDisabled != 0 && ship->aiState == kAIStatePlunder) {
            advice[0] = kOrderRegroup;
            advice[1] = kOrderRegroup;
            advice[2] = kOrderRegroup;
        }
        else if (ship->shield < ShipShieldCapacity(ship) * 0.33) {
            if (preyDisabled != 0) {
                advice[0] = kOrderAttack;
                advice[1] = kOrderAttack;
            }
            else if (theyCanHurtMe != 0 && iCanHurtThem == 0) {
                advice[0] = kOrderAttack;
                advice[1] = kOrderAttack;
            }
            else {
                advice[0] = kOrderDefend;
                advice[1] = kOrderDefend;
            }
            advice[2] = kOrderDefend;
        }
        else if (ship->shield < ShipShieldCapacity(ship) * 0.66) {
            if (preyDisabled != 0) {
                advice[0] = kOrderAttack;
            }
            else if (theyCanHurtMe != 0 && iCanHurtThem == 0) {
                advice[0] = kOrderAttack;
            }
            else {
                advice[0] = kOrderDefend;
            }
            advice[1] = kOrderAttack;
            advice[2] = kOrderDefend;
        }
        else {
            if (preyDisabled != 0)
                advice[0] = kOrderAttack;
            else if (iCanHurtThem != 0)
                advice[0] = kOrderDefend;
            else
                advice[0] = kOrderAttack;
            advice[1] = kOrderAttack;
            if (ship->odds >= 0.0 && ship->odds < 0.5) {
                advice[2] = kOrderAttack;
            }
            else {
                advice[2] = kOrderFormation;
            }
        }
        advice[3] = kOrderFormation;
    }

    state = ship->aiState;
    if ((long)state != kAIStateDefRet && (long)state != kAIStateFightSh
            && (long)state != kAIStatePlunder && (long)state != kAIStateConfused) {
        advice[0] = kOrderRegroup;
        advice[1] = kOrderRegroup;
        advice[2] = kOrderRegroup;
        advice[3] = kOrderRegroup;
    }
    if ((long)state == kAIStateDefRet) {
        substate = ship->aiSubstate;
        if ((long)substate == kSubHypJmp || (long)substate == kSubKillSpd) {
            advice[0] = kOrderRegroup;
            advice[1] = kOrderRegroup;
            advice[2] = kOrderRegroup;
            advice[3] = kOrderRegroup;
        }
        else {
            advice[0] = kOrderDefend;
            advice[1] = kOrderDefend;
        }
    }
    if ((long)state == kAIStateHypOut) {
        advice[0] = kOrderRegroup;
        advice[1] = kOrderRegroup;
        advice[2] = kOrderRegroup;
        advice[3] = kOrderRegroup;
    }

    for (i = 1; i < kMaxShips; i++) {
        if (i != ship->index && ship->index == shipData[i].parentShip
                && shipData[i].aiType >= kEscortMinStrength) {
            shipData[i].escortOrder =
                advice[shipClass[shipData[i].type].escortType];
            shipData[i].escortMayAttack = 1;
        }
    }
}

/* AICheckParentExists (mac _AICheckParentExists). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004156a0
// MAC: _AICheckParentExists i386 0x0008926a ppc 0x000944cc
void AICheckParentExists(ShipObj *ship)
{
    short i;
    short chosen;
    long best;
    long mass;

    if (ship->parentShip < 0 || ship->parentShip >= kMaxShips)
        return;
    if (shipData[ship->parentShip].active == 0
            || IsDisabled(&shipData[ship->parentShip])) {
    chosen = -1;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active == 0)
            continue;
        if (prevParent[i] != ship->parentShip)
            continue;
        if (IsDisabled(&shipData[i]))
            continue;
        mass = shipClass[shipData[i].type].mass;
        if (mass > best || chosen == -1) {
            best = mass;
            chosen = i;
        }
    }

    if (chosen != -1) {
        if (chosen == ship->index) {
            ship->aiType = shipClass[ship->type].inherentAI;
            ship->jumpTimer = -1.0f;
            if (ship->govt != -1
                    && ship->govt == shipData[ship->parentShip].govt) {
                ship->targetShip = shipData[ship->parentShip].targetShip;
                ship->targetStellar = shipData[ship->parentShip].targetStellar;
                ship->aiState = shipData[ship->parentShip].aiState;
                ship->aiSubstate = shipData[ship->parentShip].aiSubstate;
            } else {
                if ((int)ship->aiState == kAIStateGoHome || (int)ship->aiState == kAIStateFlyPr
                        || (int)ship->aiState == kAIStateProtect) {
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                } else if ((int)ship->aiState == kAIStateHypPr) {
                    ship->aiState = kAIStateHypOut;
                    ship->aiSubstate = kSubHypJmp;
                } else if (ship->targetShip != -1 && (int)ship->aiState != kAIStateInspect
                        && (int)ship->aiState != kAIStateInspect && (int)ship->aiState != kAIStateRefuel) {
                    ship->aiState = kAIStateFightSh;
                    ship->aiSubstate = kSubLoJack;
                } else {
                    ship->aiState = kAIStateHiJack;
                    ship->aiSubstate = kSubLoJack;
                }
            }
            ship->parentShip = -1;
        } else {
            ship->parentShip = chosen;
            if (ship->aiType == 5)
                ship->aiType = 6;
            if (ship->jumpTimer < 0.0f)
                ship->aiState = kAIStateConfused;
        }
    } else {
        ship->parentShip = -1;
        ship->aiType = shipClass[ship->type].inherentAI;
        ship->aiState = kAIStateConfused;
        ship->aiSubstate = kSubLoJack;
        ship->jumpTimer = -1.0f;
    }
    }
}

#define kDockRange               16.0f
#define kJumpFuel                100

/* AIShipIsEnteringHypergate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004159a0
// MAC: _AIShipIsEnteringHypergate i386 0x00089502 ppc 0x000947a0
Boolean AIShipIsEnteringHypergate(ShipObj *ship)
{
    if (ship->aiSubstate == kSubEnterHG)
        return 1;
    return 0;
}

/* AIShipIsEmergingFromHypergate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004159c0
// MAC: _AIShipIsEmergingFromHypergate i386 0x0007e2e8 ppc 0x00087f14
Boolean AIShipIsEmergingFromHypergate(ShipObj *ship)
{
    if (ship->aiState == kAIStateEmerge)
        return 1;
    return 0;
}

/*
 * SpobRecord +0x28 is spelled customSound in resources.h, but here it is
 * range-checked against 0..359 and converted to a float heading, so that name
 * is wrong for this field.
 */
/* AIMakeShipEmergeFromHyperGate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004159e0
// MAC: _AIMakeShipEmergeFromHyperGate i386 0x00089518 ppc 0x000947b8
void AIMakeShipEmergeFromHyperGate(ShipObj *ship, short stellar)
{
    ship->aiState = kAIStateEmerge;
    ship->aiSubstate = kSubLoJack;
    ship->targetShip = -1;
    ship->targetStellar = stellar;
    ship->aiTimer = 60.0f;
    ship->jumpTimer = -1.0f;
    if (stellar >= 0 && stellar < 0x800) {
        if (ShipFuelCapacity(ship) > 0)
            ship->lastStellar = stellar;
        else
            ship->lastStellar = -2;
        if (stellarData[stellar].customSound >= 0 &&
            stellarData[stellar].customSound <= 0x167)
            ship->heading = stellarData[stellar].customSound;
        else
            ship->heading = Rand(0x168);
    }
    if (ship->parentShip == 0)
        ship->speedLimit = -15.0f;
    else
        ship->speedLimit = -30.0f;
    ship->thrust = -3.0f;
}

/* AIShipIsShipGoingToStellar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415ad0
// MAC: _AIShipIsShipGoingToStellar i386 0x000895d6 ppc 0x000948e8
Boolean AIShipIsShipGoingToStellar(ShipObj *ship, short stellar)
{
    if (ship->aiState == kAIStateGoStel ||
        ship->aiState == kAIStateGoHGate) {
        if (ship->targetStellar == stellar)
            return 1;
    }
    return 0;
}

/* AIShipIsDockedWithShip (mac _AIShipIsDockedWithShip). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415b00
// MAC: _AIShipIsDockedWithShip i386 0x0008960a ppc 0x00094918
Boolean AIShipIsDockedWithShip(ShipObj *ship, ShipObj *other)
{
    float dx, dy;

    if (ship->aiSubstate == kSubBoard) {
        if (ship->targetShip == other->index || ship->targetStellar == other->index) {
            dx = FloatAbs(ship->pos.x - other->pos.x);
            if (dx <= kDockRange) {
                dy = FloatAbs(ship->pos.y - other->pos.y);
                if (dy <= kDockRange)
                    return 1;
            }
        }
    }
    return 0;
}

/* AIShipHasFuelForJump. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415b80
// MAC: _AIShipHasFuelForJump i386 0x0007f48f ppc 0x00088f68
Boolean AIShipHasFuelForJump(ShipObj *ship)
{
    if (shipClass[ship->type].fuel < kJumpFuel)
        return 0;
    if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
        return 0;
    if (ship->persType != -1) {
        if ((personData[ship->persType].flags2 & kPersStartsNoFuel) != 0) {
            if (ship->fuel < 100.0f)
                return 0;
        }
    }
    return 1;
}

/* AIHasDestroyingWeapons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415c10
// MAC: _AIHasDestroyingWeapons i386 0x0007fc7c ppc 0x00089908
Boolean AIHasDestroyingWeapons(ShipObj *ship)
{
    short i;

    for (i = 0; i < 256; i++) {
        if (ship->weap[i].count > 0 && weaponStats[i].damage[0] > 0
                && (weaponStats[i].flags2 & kWeapNotDestroying) == 0
                && (weaponStats[i].guidance == kGuidanceUnguided
                    || weaponStats[i].guidance == kGuidanceBeam
                    || weaponStats[i].guidance == kGuidanceTurretBeam
                    || weaponStats[i].guidance == kGuidanceTurret
                    || weaponStats[i].guidance == kGuidanceRocket
                    || weaponStats[i].guidance == kGuidanceFrontQuad
                    || weaponStats[i].guidance == kGuidanceRearQuad)
                && WeaponHasAmmo(ship, i))
            return 1;
    }
    return 0;
}

/* AIShipCapturedByPlayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415cb0
// MAC: _AIShipCapturedByPlayer i386 0x00089693 ppc 0x000949c8
void AIShipCapturedByPlayer(ShipObj *ship)
{
    short i;

    for (i = 1; i < 64; i++) {
        if (i != ship->index) {
            if (shipData[i].targetShip == ship->index) {
                shipData[i].aiState = kAIStateHiJack;
                shipData[i].aiSubstate = kSubLoJack;
                shipData[i].targetShip = -1;
                shipData[i].targetStellar = -1;
                shipData[i].grudge = 0;
                shipData[i].defenseStellar = -1;
            }
        }
    }
    ship->aiState = kAIStateHiJack;
    ship->aiSubstate = kSubLoJack;
    ship->targetShip = -1;
    ship->targetStellar = -1;
    ship->grudge = 0;
    ship->defenseStellar = -1;
    ship->persType = -1;
    ship->voiceClass = Rand(2);
    if (shipClass[ship->type].inherentGovtB != -1) {
        if (govtData[shipClass[ship->type].inherentGovtB].voiceClass != -1)
            ship->voiceClass = govtData[shipClass[ship->type].inherentGovtB].voiceClass;
    }
}

/* AIShipBoardedByPlayer (mac _AIShipBoardedByPlayer, dead on win: uncalled, the linker strips it). STATE: see the mac baseline. OPEN: none */
// MAC: _AIShipBoardedByPlayer i386 0x000897a8 ppc 0x00094b0c
void AIShipBoardedByPlayer(ShipObj *ship)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && i != ship->index
                && (ship->index == shipData[i].targetShip
                    || ship->index == shipData[i].targetStellar)) {
            shipData[i].aiState = kAIStateHiJack;
            shipData[i].aiSubstate = kSubLoJack;
            shipData[i].targetShip = -1;
            shipData[i].targetStellar = -1;
            shipData[i].grudge = 0;
        }
    }
}

/*
 * Name invented - no mac twin has been identified for it.
 */
#ifndef MACBUILD
/* ClearTargetsOnShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415dc0
void ClearTargetsOnShip(ShipObj *ship)
{
    short i;

    for (i = 1; i < 64; i++) {
        if (shipData[i].active) {
            if (i != ship->index) {
                if (shipData[i].targetShip == ship->index ||
                    shipData[i].targetStellar == ship->index) {
                    shipData[i].aiState = kAIStateHiJack;
                    shipData[i].aiSubstate = kSubLoJack;
                    shipData[i].targetShip = -1;
                    shipData[i].targetStellar = -1;
                    shipData[i].grudge = 0;
                }
            }
        }
    }
}
#endif

/* AIShipIsAttackingAsteroid. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415e60
// MAC: _AIShipIsAttackingAsteroid i386 0x0008982b ppc 0x00094bbc
Boolean AIShipIsAttackingAsteroid(ShipObj *ship)
{
    if (ship->aiState == kAIStateAtkAster)
        return 1;
    return 0;
}

/* AIShipIsPlundering. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00415e80
// MAC: _AIShipIsPlundering i386 0x00089841 ppc 0x00094bd4
Boolean AIShipIsPlundering(ShipObj *ship)
{
    if (ship->aiState == kAIStatePlunder)
        return 1;
    return 0;
}

/* AIAddFighterToParent. STATE: MATCH, byte-identical (reloc rows 0 of 11 differ). OPEN: none */
// FUNCTION: EVNOVA 0x00415ea0
// MAC: _AIAddFighterToParent i386 0x000802a5 ppc 0x00089fb8
void AIAddFighterToParent(ShipObj *fighter)
{
    short parent;
    short i;
    short slot;

    parent = fighter->parentShip;
    if (parent < 0 || parent >= 64)
        return;
    slot = -1;
    for (i = 0; i < 256; i++) {
        if (weaponStats[i].ammoType == fighter->type + 128 &&
            weaponStats[i].guidance == kGuidanceBay &&
            shipData[fighter->parentShip].weap[i].count > 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        for (i = 0; i < 256; i++) {
            if (weaponStats[i].ammoType == shipClass[fighter->type].spriteIndex + 128 &&
                weaponStats[i].guidance == kGuidanceBay &&
                shipData[fighter->parentShip].weap[i].count > 0) {
                slot = i;
                break;
            }
        }
    }
    if (slot != -1) {
        if (shipData[parent].weap[slot].ammo == 0) {
            if (shipData[parent].weap[slot].reloadTimer < weaponStats[slot].reload)
                shipData[parent].weap[slot].reloadTimer = weaponStats[slot].reload;
        }
        shipData[fighter->parentShip].weap[slot].ammo += 1;
        if (fighter->parentShip == 0)
            statusRedraw[3] = 1;
        fighter->active = 0;
        fighter->parentShip = -1;
        fighter->aiType = -1;
    }
}

/* AIIsShipDoingCountWait. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00416070
// MAC: _AIIsShipDoingCountWait i386 0x00089857 ppc 0x00094bec
Boolean AIIsShipDoingCountWait(ShipObj *ship)
{
    if (ship->aiState == kAIStateCountWait)
        return 1;
    return 0;
}

/*
 * The four functions below are not AI state predicates: their only callers
 * are InitEVNova and PlayGame (RT3 registration-library scrambling, same as
 * sources/engine/parser.c 0x0044aa10-0x0044aa50). No mac symbol exists for any
 * of the four.
 */

#ifndef MACBUILD
/* RegScramA2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00416090
Boolean RegScramA2(unsigned int x)
{
    return (IsPlatformOpen() * x);
}

/* RegScramB2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004160a0
void RegScramB2(unsigned char x)
{
    gRT3Table2[x]++;
}

/* RegScramC2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004160b0
int RegScramC2(unsigned char x)
{
    int n = x;
    return ((n >> (8 - n)) | (n << n)) & 0xff;
}

/* RegScramF2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004160d0
void *RegScramF2(void)
{
    char buf[4];
    return buf;
}

/* RegScramD2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004160e0
Boolean RegScramD2(char *p)
{
    TimerGetSeconds();
    return *p;
}

/* RegScramE2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004160f0
Boolean RegScramE2(void)
{
    return 0;
}
#endif

EXTERN_C_END
