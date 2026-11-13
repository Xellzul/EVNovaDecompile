// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* parser.c -- the Parser.c object: the mission-bit expression machine, ParseTextDescription and the parse-buffer save/restore (win 0x00448ba0-0x0044aa60, from its head Trim, the space skipper). Notes: Research/notes/parser.md */
/*
 * The filename is not a guess: both functions assert with the literal
 * "Parser.c" and a line number (1116 and 1126), so the original TU is named
 * by the binary itself.
 */
#include <stddef.h>
#include "burger.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameutil.h"
#include "ships.h"
#include "space.h"
#include "game.h"
#include "macport.h"
#include "missions.h"
#include "player.h"
#include "prototypes.h"
#include "render.h"
#include "stellar.h"

/* Trim. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448ba0
// MAC: _Trim i386 0x000147ac ppc 0x00018064
void Trim(short *pos)
{
    while (parseBuffer[*pos] == ' ') {
        *pos += 1;
        if (*pos >= strlen(parseBuffer))
            break;
    }
}

/* GetToken. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448be0
// MAC: _GetToken i386 0x000147e9 ppc 0x000180c4
char GetToken(short *pos)
{
    short i;
    char c;

    c = parseBuffer[*pos];
    if (c == '(') {
        *pos += 1;
        return '(';
    } else if (c == ')') {
        *pos += 1;
        return ')';
    } else if (c == '[') {
        *pos += 1;
        return '[';
    } else if (c == ']') {
        *pos += 1;
        return ']';
    } else if (c == '!') {
        *pos += 1;
        return '!';
    } else if (c == '&') {
        while (parseBuffer[*pos] == '&')
            *pos += 1;
        return '&';
    } else if (c == '|') {
        while (parseBuffer[*pos] == '|')
            *pos += 1;
        return '|';
    } else if (c == 'p' || c == 'P') {
        short num;

        num = 0;
        *pos += 1;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            num = num * 10 + parseBuffer[i] - '0';
            *pos += 1;
            i++;
        }
        if ((shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0) != 0
                || totalDaysPlayed < num)
            return '1';
        return '0';
    } else if (c == 'g' || c == 'G') {
        *pos += 1;
        if (playerGender == 1)
            return '1';
        return '0';
    } else if (c == '<' || c == '>' || c == '=') {
        *pos += 1;
        return parseBuffer[*pos - 1];
    } else if (c == 'b' || c == 'B') {
        short num;
        char r;

        num = 0;
        *pos += 1;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            num = num * 10 + parseBuffer[i] - '0';
            *pos += 1;
            i++;
        }
        r = '0';
        if (num < 10000 && missionBit[num] != 0)
            r = '1';
        return r;
    } else if (c == 'o' || c == 'O') {
        short num;
        char r;

        num = 0;
        *pos += 1;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            num = num * 10 + parseBuffer[i] - '0';
            *pos += 1;
            i++;
        }
        r = '0';
        if (num >= 128 && num < 640) {
            if (shipSystems[num - 128] > 0
                    || PlayerHasFightersOfOutfitType(num - 128) > 0)
                r = '1';
        }
        return r;
    } else if (c == 'e' || c == 'E') {
        short num;
        char r;

        num = 0;
        *pos += 1;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            num = num * 10 + parseBuffer[i] - '0';
            *pos += 1;
            i++;
        }
        r = '0';
        if (num >= 128 && num < 2176 && systemData[num - 128].exploreLevel > 0)
            r = '1';
        return r;
    } else if (c >= '0' && c <= '9') {
        short num;

        num = 0;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            num = num * 10 + parseBuffer[i] - '0';
            *pos += 1;
            i++;
        }
        lastInteger = num;
        return '#';
    }
    *pos += 1;
    return '?';
}

/* EvalTestExp. STATE: MATCH 100.0%, exe 227 / obj 227, byte-identical yes (838 bytes).
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00449020
// MAC: _EvalTestExp i386 0x00015cd4 ppc 0x00019968
short EvalTestExp(short *pos)
{
    short end;
    short j;
    short opens;
    short closes;
    short count;
    short lastVal;
    short result;
    Boolean negate;
    char op;
    char c;
    short sub;
    short i;
    char tok;

    i = *pos;
    c = parseBuffer[*pos];
    if (c != '(' && c != '[')
        opens = 1;
    else
        opens = 0;
    closes = 0;
    end = -1;
    j = *pos;
    while (j < strlen(parseBuffer)) {
        if (parseBuffer[j] == '(' || parseBuffer[j] == '[')
            opens++;
        else if (parseBuffer[j] == ')' || parseBuffer[j] == ']')
            closes++;
        if (closes == opens) {
            end = j - 1;
            break;
        }
        j++;
    }
    if (end == -1)
        end = j;

#ifdef MACBUILD
    result = 1;
    lastVal = 0;
    negate = 0;
    count = 0;
    op = '?';
    i = *pos;
#else
    op = '?';
    i = *pos;
    result = 1;
    lastVal = 0;
    negate = 0;
    count = 0;
#endif
    while (i < strlen(parseBuffer)) {
        if (parseBuffer[i] == ' ') {
            Trim(&i);
        } else {
            tok = GetToken(&i);
            if (tok == '(' || tok == '[') {
                sub = EvalTestExp(&i);
                if (negate != 0) {
                    negate = 0;
#ifdef MACBUILD
                    if (sub == 0)
                        sub = 1;
                    else
                        sub = 0;
#else
                    sub = sub == 0 ? 1 : 0;
#endif
                }
                if (op == '&') {
                    if (sub == 0)
                        lastVal = 0;
                    result = lastVal;
                } else if (op == '|') {
                    if (sub == 1)
                        lastVal = 1;
                    result = lastVal;
                } else {
                    lastVal = sub;
                }
            } else if (tok == ']') {
                op = '+';
                result = count;
                break;
            } else if (tok == ')') {
                break;
            } else if ((tok == '1' && negate == 0)
                    || (tok == '0' && negate != 0)) {
                count++;
                if (op == '|')
                    result = 1;
                if (negate != 0)
                    negate = 0;
                lastVal = 1;
            } else if ((tok == '0' && negate == 0)
                    || (tok == '1' && negate != 0)) {
                if (op == '&')
                    result = 0;
                if (negate != 0)
                    negate = 0;
                lastVal = 0;
            } else if (tok == '|') {
                op = '|';
                result = lastVal;
            } else if (tok == '&') {
                op = '&';
                result = lastVal;
            } else if (tok == '<' || tok == '>' || tok == '=') {
                op = tok;
                result = 0;
            } else if (tok == '#') {
                if (op == '<') {
                    if (lastVal < lastInteger)
                        result = 1;
                    else
                        result = 0;
                } else if (op == '>') {
                    if (lastVal > lastInteger)
                        result = 1;
                    else
                        result = 0;
                } else if (op == '=') {
                    if (lastVal == lastInteger)
                        result = 1;
                    else
                        result = 0;
                }
            } else if (tok == '!') {
                negate = 1;
            }
        }
    }
    if (op == '?')
        result = lastVal;
    *pos = end + 2;
    return result;
}



/* names not pinned by the twin -- addresses only, per the DAT_ rule */

/* the 16 live mission slots, stride 0x14 */
/* Their records: packed, stride 0x8e6 (`id` at the odd offset 0x4d), as LoadPilotData's. */
#define kRankStride 0x120

/* EvalSetExp (mac _EvalSetExp). STATE: MATCH 100.0%, 875/875, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00449370
// MAC: _EvalSetExp i386 0x000150fc ppc 0x00018ab0
void EvalSetExp(void)
{
    /* short 4th argument here; game.h declares char sound */

    short max;
    unsigned char str[256];
    char op;
    void *saved;
    short i;
    short k;
    short n;
    short seen;
    short pick;
    int j;
    Boolean armed;
    char c;
    unsigned char out[256];
    short num;
    Boolean wasSet;
    short m;

    op = '?';
    pick = -1;
    seen = 0;
    num = 0;
    i = 0;
    while (i <= strlen(parseBuffer)) {
        armed = 0;
        c = parseBuffer[i];
        switch (toupper(c)) {
        case 'B':
            num = 0;
            if (op == '?')
                op = ' ';
            break;
        case '!':
        case 'A':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'P':
        case 'Q':
        case 'S':
        case 'T':
        case 'U':
        case 'X':
        case 'Y':
        case '^':
            num = 0;
#if V111 /* V111 spelling */
            op = toupper(parseBuffer[i]);
#else
            op = toupper(c);
#endif
            break;
        case 'R':
            seen = 0;
            op = '?';
            pick = Rand(2);
            break;
        default:
#if V111 /* V111 spelling */
            if (parseBuffer[i] >= '0' && parseBuffer[i] <= '9') {
                num = num * 10 + parseBuffer[i] - '0';
#else
            if (c >= '0' && c <= '9') {
                num = num * 10 + c - '0';
#endif
            } else if (pick != -1 && seen == pick) {
                if (seen == 0)
                    i++;
                else
                    seen++;
                pick = -1;
                op = '?';
            } else {
                armed = 1;
            }
            break;
        }
        if (armed != 0) {
            switch (op) {
            case ' ':
                if (num < kMaxMissionBits)
                    missionBit[num] = 1;
                break;
            case '!':
                if (num < kMaxMissionBits)
                    missionBit[num] = 0;
                break;
            case '^':
                if (num < kMaxMissionBits) {
                    if (missionBit[num] != 0)
                        missionBit[num] = 0;
                    else
                        missionBit[num] = 1;
                }
                break;
            case 'A':
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxMissions) {
                    for (k = 0; k < kMaxActiveMissions; k++) {
                        if (currentMissionObjectives[k].active != 0
                                && currentMissions[k].misnIndex == num - kFirstResourceID) {
                            saved = SaveBuffer();
                            AbortMission(k, 1);
                            RestoreBuffer(saved);
                        }
                    }
                }
                break;
            case 'F':
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxMissions) {
                    for (k = 0; k < kMaxActiveMissions; k++) {
                        if (currentMissionObjectives[k].active != 0
                                && currentMissions[k].misnIndex == num - kFirstResourceID)
                            currentMissionObjectives[k].failed = 1;
                    }
                }
                break;
            case 'S':
#if V111 /* V111 spelling */
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxMissions) {
#else
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxMissions) {
#endif
                    wasSet = offeringFromShip;
                    if (playerIsLanded != 0)
                        offeringFromShip = 0;
                    else
                        offeringFromShip = 1;
                    saved = SaveBuffer();
#if V111 /* V111 spelling */
                    j = n;
#else
                    j = num - kFirstResourceID;
#endif
                    RandomizeOneMission(j);
                    DoMissionAccept(j);
                    offeringFromShip = wasSet;
                    RestoreBuffer(saved);
                }
                break;
            case 'G':
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxOutfits) {
                    saved = SaveBuffer();
                    ShipStatsToSystemInfo();
                    GrantOutfitItem(num - kFirstResourceID);
                    SystemInfoToShipStats();
                    RestoreBuffer(saved);
                }
                break;
            case 'D':
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxOutfits) {
                    saved = SaveBuffer();
                    ShipStatsToSystemInfo();
                    if (shipSystems[num - kFirstResourceID] > 0)
                        shipSystems[num - kFirstResourceID] -= 1;
                    SystemInfoToShipStats();
                    RestoreBuffer(saved);
                }
                break;
            case 'C':
            case 'E':
            case 'H':
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxShipTypes) {
                    saved = SaveBuffer();
                    ShipStatsToSystemInfo();
                    if (op == 'H') {
#if V111 /* V111 spelling */
                        for (j = 0; j < kMaxOutfits; j++) {
                            if ((outfitSystem[j].flags & (kOutfPersistent | kOutfMissionPersistent)) == 0)
                                shipSystems[j] = 0;
                        }
#else
                        for (j = 0; (short)j < kMaxOutfits; j++) {
#ifdef MAC1010 /* 1.0.10 spelling */
                            if ((outfitSystem[(short)j].flags & (kOutfPersistent | kOutfMissionPersistent)) == 0)
#else
                            if ((outfitSystem[j].flags & (kOutfPersistent | kOutfMissionPersistent)) == 0)
#endif
                                shipSystems[(short)j] = 0;
                        }
#endif
                    }
                    SystemInfoToShipStats();
                    shipData->type = num - kFirstResourceID;
                    if (op == 'E' || op == 'H') {
                        for (k = 0; k < kMaxWeapTypes; k++) {
                            shipData->weap[k].count +=
                                shipClass[shipData->type].weapCount[k];
                            if (weaponStats[k].ammoType >= 0 && weaponStats[k].ammoType < kMaxWeapTypes)
                                shipData->weap[weaponStats[k].ammoType].ammo +=
                                    shipClass[shipData->type].weapAmmo[k];
                            else
                                shipData->weap[k].ammo +=
                                    shipClass[shipData->type].weapAmmo[k];
                        }
                        for (k = 0; k < 8; k++) {
                            if (shipClass[shipData->type].defOutfCount[k] > 0)
                                shipSystems[shipClass[shipData->type].defOutfID[k]] +=
                                    shipClass[shipData->type].defOutfCount[k];
                        }
                        ShipStatsToSystemInfo();
                        for (k = 0; k < kMaxOutfits; k++) {
                            if (shipSystems[k] > 0) {
                                HasMaxOfItem(k, &max, 0);
                                if (max > 0 && shipSystems[k] > max)
                                    shipSystems[k] = max;
                            }
                        }
                    }
                    SystemInfoToShipStats();
                    while (IsDisabled(shipData) != 0
                            && shipData->armor < ShipArmorCapacity(shipData))
                        shipData->armor += 1.0f;
                    LoadIntfResource();
                    RestoreBuffer(saved);
                }
                break;
            case 'M':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxSystems) {
                    shipData->curSystem = n;
                    shipData->targetStellar = -1;
                    for (k = 0; k < kNavSlots; k++) {
                        if (systemData[n].navDefault[k] != -1) {
                            if (playerIsLanded != 0) {
                                shipData->targetStellar = systemData[n].navDefault[k];
                            } else {
                                shipData->pos.x =
                                    stellarData[systemData[n].navDefault[k]].pos.h;
                                shipData->pos.y =
                                    stellarData[systemData[n].navDefault[k]].pos.v;
                                shipData->vel.x = 0;
                                shipData->vel.y = 0;
                                shipData->targetStellar = -1;
                            }
                            break;
                        }
                    }
                    for (k = 1; k < kMaxShips; k++) {
                        if (shipData[k].active != 0 && shipData[k].parentShip == 0)
                            shipData[k].curSystem = shipData->curSystem;
                    }
                    if (playerIsLanded != 0) {
                        for (k = 0; k < kMaxMissions; k++) {
                            dockedPortMissions[0][k] = -1;
                            dockedPortMissions[1][k] = -1;
                        }
                    } else {
                        killShots = 1;
                        killBoxes = 1;
                        killExplods = 1;
                        killSmoke = 1;
                        killAsteroids = 1;
                        shipData->targetShip = -1;
                        HandleStellarSystemVisibility();
                        MissionHandlePlayerEnteredNewSystem();
                    }
                }
                break;
            case 'N':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxSystems) {
                    shipData->curSystem = n;
                    shipData->targetStellar = -1;
                    dontMovePlayerAfterLanding = 1;
                    for (k = 1; k < kMaxShips; k++) {
                        if (shipData[k].active != 0 && shipData[k].parentShip == 0)
                            shipData[k].curSystem = shipData->curSystem;
                    }
                }
                if (playerIsLanded != 0) {
                    for (k = 0; k < kMaxMissions; k++) {
                        dockedPortMissions[0][k] = -1;
                        dockedPortMissions[1][k] = -1;
                    }
                } else {
                    killShots = 1;
                    killBoxes = 1;
                    killExplods = 1;
                    killSmoke = 1;
                    killAsteroids = 1;
                    shipData->targetShip = -1;
                    HandleStellarSystemVisibility();
                    MissionHandlePlayerEnteredNewSystem();
                }
                break;
            case 'K':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxRanks) {
                    saved = SaveBuffer();
                    ActivateRank((char *)rankData + n * kRankStride);
                    RestoreBuffer(saved);
                }
                break;
            case 'L':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxRanks) {
                    saved = SaveBuffer();
                    DeactivateRank((char *)rankData + n * kRankStride);
                    RestoreBuffer(saved);
                }
                break;
            case 'P':
                missionSoundID = num;
                break;
            case 'Y':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxStellars) {
                    if (stellarData[n].system == shipData->curSystem) {
                        if (StellarIsDestroyed(&stellarData[n]) == 0
                                && stellarData[n].explodType != -1)
                            CreateExplosion(
                                XYShortToFloat(stellarData[n].pos),
                                stellarData[n].explodType, 0, 1);
                    }
                    if (stellarData[n].deadDays >= 0)
                        stellarData[n].deadDaysLeft = stellarData[n].deadDays;
                    else
                        stellarData[n].deadDaysLeft = 1;
                    stellarData[n].deadCounter = -1;
                }
                break;
            case 'U':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxStellars) {
                    stellarData[n].deadDaysLeft = -1;
                    stellarData[n].deadCounter = stellarData[n].strength;
                }
                break;
            case 'T':
                GetRandomIndString(str, num);
                if (str[0] != 0) {
                    out[0] = 0;
                    for (k = 1; k <= str[0]; k++) {
                        if (str[k] == '*') {
                            for (m = 1; m <= shipName[0]; m++) {
                                out[0]++;
                                out[out[0]] = shipName[m];
                            }
                        } else {
                            out[0]++;
                            out[out[0]] = str[k];
                        }
                    }
                    for (k = 0; k <= out[0]; k++)
                        shipName[k] = out[k];
                }
                break;
            case 'Q':
                GetRandomIndString((unsigned char *)absquatulateStr, num);
                if (currentMissionIndexForSetParser >= 0
                        && currentMissionIndexForSetParser < kMaxActiveMissions) {
                    saved = SaveBuffer();
                    PtoCstr((unsigned char *)absquatulateStr);
                    strcpy(briefingTextPtr, (char *)absquatulateStr);
                    MungeBriefing(0, currentMissionIndexForSetParser);
                    strcpy((char *)absquatulateStr, briefingTextPtr);
                    CtoPstr((char *)absquatulateStr);
                    RestoreBuffer(saved);
                }
                break;
            case 'X':
                n = num - kFirstResourceID;
                if (n >= 0 && n < kMaxSystems) {
                    if (systemData[n].exploreLevel < 1)
                        systemData[n].exploreLevel = 1;
                }
                break;
            default:
                break;
            }
            if (op != '?') {
                op = '?';
                seen++;
            }
        }
        i++;
    }
}

#define kParseBufSize 0x2000



/* only the registration byte at the tail of the shïp record is read here */


/* ParseTextDescription (mac _ParseTextDescription). STATE: MATCH 284/284, byte-identical,
   once compiled after EvalSetExp, its predecessor in the original object. OPEN: none */
// FUNCTION: EVNOVA 0x0044a4d0
// MAC: _ParseTextDescription i386 0x00014bdd ppc 0x00018598
void ParseTextDescription(void)
{
    char out[kParseBufSize];
    char c;
    char kind;
    int32_t i;
    int32_t outLen;
    int32_t num;
    int32_t state;
    Boolean negate;
    Boolean escape;
    int32_t j;
    int32_t k;
    int32_t rank;
    Boolean keep;

    state = 0;
    outLen = 0;
    num = 0;
    escape = 0;
    negate = 0;
    kind = '?';
    for (i = 0; i < strlen(parseBuffer); i++) {
        if (state == 1) {
            if (parseBuffer[i] == 'g' || parseBuffer[i] == 'G') {
                if (negate != 0) {
                    if (playerGender == 1)
                        state = 5;
                    else
                        state = 3;
                } else {
                    if (playerGender == 1)
                        state = 3;
                    else
                        state = 5;
                }
            } else if (parseBuffer[i] == 'p' || parseBuffer[i] == 'P') {
                kind = 'P';
                num = 0;
                state = 2;
            } else if (parseBuffer[i] == 'b' || parseBuffer[i] == 'B') {
                kind = 'B';
                num = 0;
                state = 2;
            } else if (parseBuffer[i] == '!') {
                negate = 1;
            }
        } else if (state == 2) {
            c = parseBuffer[i];
            if (c >= '0' && c <= '9') {
                num = num * 10 + c - '0';
            } else {
                if (kind == 'P') {
                    if ((shipClass[gameIsRegisteredIndex].registered != 0
                             ? 1 : 0) != 0)
                        keep = 1;
                    else if (totalDaysPlayed < num)
                        keep = 1;
                    else
                        keep = 0;
                } else if (num < 10000) {
                    keep = missionBit[num];
                } else {
                    keep = 0;
                }
                if (negate != 0)
                    keep = keep == 0;
                if (keep != 0) {
                    if (c == '"')
                        state = 4;
                    else
                        state = 3;
                } else {
                    if (c == '"')
                        state = 6;
                    else
                        state = 5;
                }
                kind = '?';
            }
        } else if (state == 3) {
            if (parseBuffer[i] == '"')
                state = 4;
        } else if (state == 4) {
            if (parseBuffer[i] == '\\') {
                escape = 1;
            } else if (parseBuffer[i] == '"' && escape == 0) {
                state = 8;
            } else {
                out[outLen] = parseBuffer[i];
                outLen++;
                escape = 0;
            }
        } else if (state == 5) {
            if (parseBuffer[i] == '"')
                state = 6;
        } else if (state == 6) {
            if (parseBuffer[i] == '\\') {
                escape = 1;
            } else if (parseBuffer[i] == '"' && escape == 0) {
                state = 7;
            } else {
                escape = 0;
            }
        } else if (state == 7) {
            if (parseBuffer[i] == '}')
                state = 0;
            else if (parseBuffer[i] == '"')
                state = 4;
        } else if (state == 8) {
            if (parseBuffer[i] == '}')
                state = 0;
        } else {
            if (parseBuffer[i] == '{') {
                state = 1;
            } else {
                out[outLen] = parseBuffer[i];
                outLen++;
            }
        }
    }
    out[outLen] = 0;
    strcpy(parseBuffer, out);
    if (strlen(parseBuffer) > 5) {
#ifdef MAC1010
        for (i = 0; i < strlen(parseBuffer) - 5; i++) {
            if (parseBuffer[i] == '<'
                && (parseBuffer[i + 1] == 'P' || parseBuffer[i + 1] == 'S')
                && parseBuffer[i + 2] == 'R' && parseBuffer[i + 3] == 'K'
                && parseBuffer[i + 4] != '>') {
                num = 0;
                for (outLen = i + 4; outLen < strlen(parseBuffer); outLen++) {
                    if (parseBuffer[outLen] < '0' || parseBuffer[outLen] > '9')
                        break;
                    num = num * 10 + parseBuffer[outLen] - '0';
                }
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxGovts) {
                    if (parseBuffer[i + 1] == 'P')
                        descRankGovt[0] = num - kFirstResourceID;
                    else
                        descRankGovt[1] = num - kFirstResourceID;
                } else {
                    if (parseBuffer[i + 1] == 'P')
                        descRankGovt[0] = -1;
                    else
                        descRankGovt[1] = -1;
                }
                i += 4;
            }
        }
#else
        for (outLen = 0; outLen < strlen(parseBuffer) - 5; outLen++) {
            if (parseBuffer[outLen] == '<') {
                j = outLen + 1;
                if (parseBuffer[j] == 'P' || parseBuffer[j] == 'S') {
                    k = outLen + 2;
                    if (parseBuffer[k] == 'R') {
                        k = outLen + 3;
                        if (parseBuffer[k] == 'K') {
                            k = outLen + 4;
                            if (parseBuffer[k] != '>') {
                                rank = 0;
                                for (i = outLen + 4; i < strlen(parseBuffer); i++) {
                                    if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                                        break;
                                    rank = rank * 10 + parseBuffer[i] - '0';
                                }
                                if (rank >= kFirstResourceID && rank < kFirstResourceID + kMaxGovts) {
                                    if (parseBuffer[j] == 'P')
                                        descRankGovt[0] = rank - kFirstResourceID;
                                    else
                                        descRankGovt[1] = rank - kFirstResourceID;
                                } else {
                                    if (parseBuffer[j] == 'P')
                                        descRankGovt[0] = -1;
                                    else
                                        descRankGovt[1] = -1;
                                }
                                outLen += 4;
                            }
                        }
                    }
                }
            }
        }
#endif
    }
}

/* SaveBuffer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044a980
// MAC: _SaveBuffer i386 0x000150ae ppc 0x00018a54
void *SaveBuffer(void)
{
    void *saved;

    saved = AllocAPointer(kParseBufSize);
    if (saved == NULL)
#ifdef MACBUILD
        SWAssertFail(kEngineSourceDir "Parser.c", 1117);
#else
        SWAssertFail("Parser.c", 0x45c);
#endif
    BlockMove(parseBuffer, saved, kParseBufSize);
    return saved;
}

/* RestoreBuffer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044a9d0
// MAC: _RestoreBuffer i386 0x00015066 ppc 0x00018a00
void RestoreBuffer(void *saved)
{
    if (saved == NULL)
#ifdef MACBUILD
        SWAssertFail(kEngineSourceDir "Parser.c", 1127);
#else
        SWAssertFail("Parser.c", 0x466);
#endif
    BlockMove(saved, parseBuffer, kParseBufSize);
    DeallocAPointer(saved);
}

/*
 * There is no mac symbol for any of the four below, so they keep FUN_ names;
 * 0x004d3356/0x004d3961 are unnamed helpers in the runtime block above
 * 0x004d3000 and are declared extern rather than written.
 */

#ifndef MACBUILD
/* RegScramA. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044aa10
unsigned char RegScramA(unsigned int x)
{
    return (IsPlatformOpen() * x);
}

/* RegScramB. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044aa20
void RegScramB(unsigned char x)
{
    gRT3Table[x]++;
}

/* RegScramC. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044aa30
int RegScramC(unsigned char x)
{
    int n = x;
    return ((n >> (8 - n)) | (n << n)) & 0xff;
}

/* RegScramD. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044aa50
Boolean RegScramD(char *p)
{
    TimerGetSeconds();
    return *p;
}

/* RegScramE. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044aa60
Boolean RegScramE(void)
{
    return 0;
}
#endif

char parseBuffer[8192];
short lastInteger;
#ifndef MACBUILD
unsigned char DAT_007caa12[6];
unsigned char gRT3Table[256];
#endif
