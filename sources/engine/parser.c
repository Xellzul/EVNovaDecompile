// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* parser.c -- the Parser.c object: the mission-bit expression machine, ParseTextDescription and the parse-buffer save/restore (win 0x00448ba0-0x0044aa60, from its head SkipSpaces, the space skipper). Notes: Research/notes/parser.md */
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

/* SkipSpaces. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448ba0
void SkipSpaces(short *pos)
{
    while (parseBuffer[*pos] == ' ') {
        *pos += 1;
        if (*pos >= strlen(parseBuffer))
            break;
    }
}

/* GetToken. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448be0
char GetToken(short *pos)
{
    short i;
    short num;
    char c;
    char r;

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
        *pos += 1;
        num = 0;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            *pos += 1;
            num = num * 10 + parseBuffer[i] - '0';
            i++;
        }
        if ((gShipTypes[gameIsRegisteredIndex].registered != 0 ? 1 : 0) != 0
                || gSharewareDays < num)
            return '1';
        return '0';
    } else if (c == 'g' || c == 'G') {
        *pos += 1;
        if (playerGender == 1)
            return '1';
        return '0';
    } else if (c == '<' || c == '=' || c == '>') {
        *pos += 1;
        return parseBuffer[*pos - 1];
    } else if (c == 'b' || c == 'B') {
        *pos += 1;
        num = 0;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            *pos += 1;
            num = num * 10 + parseBuffer[i] - '0';
            i++;
        }
        r = '0';
        if (num < 10000 && missionBit[num] != 0)
            r = '1';
        return r;
    } else if (c == 'o' || c == 'O') {
        *pos += 1;
        num = 0;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            *pos += 1;
            num = num * 10 + parseBuffer[i] - '0';
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
        *pos += 1;
        num = 0;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            *pos += 1;
            num = num * 10 + parseBuffer[i] - '0';
            i++;
        }
        r = '0';
        if (num >= 128 && num < 2176 && gSystems[num - 128].exploreLevel > 0)
            r = '1';
        return r;
    } else if (c >= '0' && c <= '9') {
        num = 0;
        i = *pos;
        while (i < strlen(parseBuffer)) {
            if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                break;
            *pos += 1;
            num = num * 10 + parseBuffer[i] - '0';
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
short EvalTestExp(short *pos)
{
    char op;
    char c;
    short end;
    short i;
    short j;
    short opens;
    short closes;
    short lastVal;
    short count;
    short result;
    short sub;
    char negate;
    char tok;

    i = *pos;
    c = parseBuffer[*pos];
    if (c == '(' || c == '[')
        opens = 0;
    else
        opens = 1;
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

    op = '?';
    i = *pos;
    result = 1;
    lastVal = 0;
    negate = 0;
    count = 0;
    while (i < strlen(parseBuffer)) {
        if (parseBuffer[i] == ' ') {
            SkipSpaces(&i);
        } else {
            tok = GetToken(&i);
            if (tok == '(' || tok == '[') {
                sub = EvalTestExp(&i);
                if (negate != 0) {
                    negate = 0;
                    sub = sub == 0 ? 1 : 0;
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
            } else if (tok == '<' || tok == '=' || tok == '>') {
                result = 0;
                op = tok;
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
void EvalSetExp(void)
{
    /* short 4th argument here; game.h declares char sound */

    short max;
    unsigned char str[256];
    short i;
    short pick;
    short seen;
    char c;
    char op;
    char armed;
    unsigned char out[256];
    short num;
    short n;
    int j;
    void *saved;
    short k;
    char wasSet;
    short m;

    op = '?';
    pick = -1;
    seen = 0;
    num = 0;
    i = 0;
    while (i <= strlen(parseBuffer)) {
        c = parseBuffer[i];
        armed = 0;
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
            op = toupper(c);
            break;
        case 'R':
            seen = 0;
            op = '?';
            pick = Rand(2);
            break;
        default:
            if (c >= '0' && c <= '9') {
                num = num * 10 + c - '0';
            } else if (pick != -1 && seen == pick) {
                if (seen != 0)
                    seen++;
                else
                    i++;
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
                                && num - kFirstResourceID == currentMissions[k].misnIndex) {
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
                                && num - kFirstResourceID == currentMissions[k].misnIndex)
                            currentMissionObjectives[k].failed = 1;
                    }
                }
                break;
            case 'S':
                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxMissions) {
                    wasSet = offeringFromShip;
                    if (playerIsLanded != 0)
                        offeringFromShip = 0;
                    else
                        offeringFromShip = 1;
                    saved = SaveBuffer();
                    j = num - kFirstResourceID;
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
                        for (j = 0; (short)j < kMaxOutfits; j++) {
                            if ((gOutfits[j].flags & (kOutfPersistent | kOutfMissionPersistent)) == 0)
                                shipSystems[(short)j] = 0;
                        }
                    }
                    SystemInfoToShipStats();
                    shipData->type = num - kFirstResourceID;
                    if (op == 'E' || op == 'H') {
                        for (k = 0; k < kMaxWeapTypes; k++) {
                            shipData->weap[k].count +=
                                gShipTypes[shipData->type].weapCount[k];
                            if (gWeapons[k].ammoType >= 0 && gWeapons[k].ammoType < kMaxWeapTypes)
                                shipData->weap[gWeapons[k].ammoType].ammo +=
                                    gShipTypes[shipData->type].weapAmmo[k];
                            else
                                shipData->weap[k].ammo +=
                                    gShipTypes[shipData->type].weapAmmo[k];
                        }
                        for (k = 0; k < 8; k++) {
                            if (gShipTypes[shipData->type].defOutfCount[k] > 0)
                                shipSystems[gShipTypes[shipData->type].defOutfID[k]] +=
                                    gShipTypes[shipData->type].defOutfCount[k];
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
                            && ShipArmorCapacity(shipData) > shipData->armor)
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
                        if (gSystems[n].navDefault[k] != -1) {
                            if (playerIsLanded != 0) {
                                shipData->targetStellar = gSystems[n].navDefault[k];
                            } else {
                                shipData->pos.x =
                                    stellarData[gSystems[n].navDefault[k]].pos.h;
                                shipData->pos.y =
                                    stellarData[gSystems[n].navDefault[k]].pos.v;
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
                k = num - kFirstResourceID;
                if (k >= 0 && k < kMaxRanks) {
                    saved = SaveBuffer();
                    ActivateRank((char *)rankData + k * kRankStride);
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
                k = num - kFirstResourceID;
                if (k >= 0 && k < kMaxStellars) {
                    if (shipData->curSystem == stellarData[k].system) {
                        if (StellarIsDestroyed(&stellarData[k]) == 0
                                && stellarData[k].explodType != -1)
                            CreateExplosion(
                                XYShortToFloat(stellarData[k].pos),
                                stellarData[k].explodType, 0, 1);
                    }
                    if (stellarData[k].deadDays >= 0)
                        stellarData[k].deadDaysLeft = stellarData[k].deadDays;
                    else
                        stellarData[k].deadDaysLeft = 1;
                    stellarData[k].deadCounter = -1;
                }
                break;
            case 'U':
                k = num - kFirstResourceID;
                if (k >= 0 && k < kMaxStellars) {
                    stellarData[k].deadDaysLeft = -1;
                    stellarData[k].deadCounter = stellarData[k].strength;
                }
                break;
            case 'T':
                GetRandomIndString(str, num);
                if (str[0] != 0) {
                    out[0] = 0;
                    for (n = 1; n <= str[0]; n++) {
                        if (str[n] == '*') {
                            for (m = 1; m <= shipName[0]; m++) {
                                out[0]++;
                                out[out[0]] = shipName[m];
                            }
                        } else {
                            out[0]++;
                            out[out[0]] = str[n];
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
                    strcpy(briefingTextPtr, absquatulateStr);
                    MungeBriefing(0, currentMissionIndexForSetParser);
                    strcpy(absquatulateStr, briefingTextPtr);
                    CtoPstr(absquatulateStr);
                    RestoreBuffer(saved);
                }
                break;
            case 'X':
                k = num - kFirstResourceID;
                if (k >= 0 && k < kMaxSystems) {
                    if (gSystems[k].exploreLevel < 1)
                        gSystems[k].exploreLevel = 1;
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
void ParseTextDescription(void)
{
    char out[kParseBufSize];
    char c;
    char negate;
    char kind;
    char escape;
    long state;
    long i;
    long outLen;
    long num;
    long j;
    long k;
    char keep;

    state = 0;
    outLen = 0;
    num = 0;
    escape = 0;
    negate = 0;
    kind = '?';
    for (i = 0; i < strlen(parseBuffer); i++) {
        if (state == 1) {
            if (parseBuffer[i] == 'g' || parseBuffer[i] == 'G') {
                if (negate != 0)
                    state = playerGender == 1 ? 5 : 3;
                else
                    state = playerGender == 1 ? 3 : 5;
            } else if (parseBuffer[i] == 'p' || parseBuffer[i] == 'P') {
                num = 0;
                kind = 'P';
                state = 2;
            } else if (parseBuffer[i] == 'b' || parseBuffer[i] == 'B') {
                num = 0;
                kind = 'B';
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
                    if ((gShipTypes[gameIsRegisteredIndex].registered != 0
                             ? 1 : 0) != 0)
                        keep = 1;
                    else if (gSharewareDays < num)
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
                if (keep != 0)
                    state = c == '"' ? 4 : 3;
                else
                    state = c == '"' ? 6 : 5;
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
                                num = 0;
                                for (i = outLen + 4; i < strlen(parseBuffer); i++) {
                                    if (parseBuffer[i] < '0' || parseBuffer[i] > '9')
                                        break;
                                    num = num * 10 + parseBuffer[i] - '0';
                                }
                                if (num >= kFirstResourceID && num < kFirstResourceID + kMaxGovts) {
                                    if (parseBuffer[j] == 'P')
                                        gDescRankGovt[0] = num - kFirstResourceID;
                                    else
                                        gDescRankGovt[1] = num - kFirstResourceID;
                                } else {
                                    if (parseBuffer[j] == 'P')
                                        gDescRankGovt[0] = -1;
                                    else
                                        gDescRankGovt[1] = -1;
                                }
                                outLen += 4;
                            }
                        }
                    }
                }
            }
        }
    }
}

/* SaveBuffer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044a980
void *SaveBuffer(void)
{
    void *saved;

    saved = AllocAPointer(kParseBufSize);
    if (saved == NULL)
        SWAssertFail("Parser.c", 0x45c);
    BlockMove(parseBuffer, saved, kParseBufSize);
    return saved;
}

/* RestoreBuffer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044a9d0
void RestoreBuffer(void *saved)
{
    if (saved == NULL)
        SWAssertFail("Parser.c", 0x466);
    BlockMove(saved, parseBuffer, kParseBufSize);
    DeallocAPointer(saved);
}

/*
 * There is no mac symbol for any of the four below, so they keep FUN_ names;
 * 0x004d3356/0x004d3961 are unnamed helpers in the runtime block above
 * 0x004d3000 and are declared extern rather than written.
 */

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
char RegScramD(char *p)
{
    TimerGetSeconds();
    return *p;
}

/* RegScramE. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0044aa60
char RegScramE(void)
{
    return 0;
}

char parseBuffer[8192];
short lastInteger;
unsigned char DAT_007caa12[6];
unsigned char gRT3Table[256];
