// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* engineutils.c -- the engine's utility object (tu-map O10, win 0x00462550..0x0046f3f0). Notes: Research/notes/engineutils.md */
#include <stdlib.h>
#include <math.h>
#include <stddef.h>

#include "burgermath.h"
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "game.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "gameutil.h"
#include "space.h"
#include "spriteworld.h"
#include "prototypes.h"
#include "dialogs.h"
#include "stellar.h"
#include "missions.h"
#include "burger.h"
#include "crashlog.h"

#define kRoidYieldBase   0x32
#define kRoidYieldSpread 0x65

extern "C" {

/* DestroyAsteroid (mac _DestroyAsteroid). STATE: MATCH 100.0%, exe 223 / obj 223 instrs, relocregcheck 23/23 clean.
   OPEN: byte-identity unchecked (bytediff only covers baseline-MATCH rows); the function
   still carries its address name -- no mac twin looked up. see notes */
// FUNCTION: EVNOVA 0x00462550
void DestroyAsteroid(AsteroidObj *roid)
{

    short expl[2];
    short i;
    short n;
    short spread;

    expl[0] = asteroidType[roid->type].fragType[0];
    spread = asteroidType[roid->type].fragCount;
    expl[1] = asteroidType[roid->type].fragType[1];

    if (asteroidType[roid->type].yieldQty > 0) {
        n = ((Rand(kRoidYieldSpread) + kRoidYieldBase)
                   * asteroidType[roid->type].yieldQty * 0.01);
        if (n > 0) {
            for (i = 0; i < n; i++)
                SpawnScoopableBox(&roid->pos,
                                  asteroidType[roid->type].yieldType,
                                  (roid->type >> 2) + 1);
        }
    }
    if (asteroidType[roid->type].partCount > 0) {
        SpawnParticles(roid->pos.x, roid->pos.y, 0.2f, 0x28, 0xf0,
                     0x1e0, asteroidType[roid->type].color, 0x20,
                     asteroidType[roid->type].partCount, GetSpriteXSize(roid->sprite) / 3);
    }
    if (asteroidType[roid->type].explodType != -1) {
        CreateExplosion(roid->pos,
                        asteroidType[roid->type].explodType, 0, 1);
    }
    if (spread > 0) {
        n = Rand(spread) + spread / 2;
        if (n > 0) {
            for (i = 0; i < n; i++) {
                if (expl[0] != -1 && expl[1] != -1)
                    SpawnSubAsteroid(&roid->pos, expl[Rand(2)]);
                else if (expl[0] != -1)
                    SpawnSubAsteroid(&roid->pos, expl[0]);
                else if (expl[1] != -1)
                    SpawnSubAsteroid(&roid->pos, expl[1]);
            }
        }
    }
    roid->active = 0;
    SWSetSpriteVisible(roid->sprite, 0);
}

/* FindNearestShipToPlayer (mac _FindNearestShipToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462850
short FindNearestShipToPlayer(void)
{
    short i;
    short best;
    char hasDetect2;
    char hasDetect1;
    float bestDist;
    float dx;
    float dy;
    float dist;

    hasDetect1 = ShipCanTargetUntargetableShips(shipData);
    hasDetect2 = ShipCanTargetCloakedShips(shipData);
    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kMaxShips; i++) {
        if (i == 0)
            continue;
        if (!shipData[i].active)
            continue;
        if (shipData[i].parentShip == 0)
            continue;
        if (IsDying(&shipData[i]))
            continue;
        if (ShipIsCloaked(&shipData[i]) && !hasDetect2)
            continue;
        if (shipData[0].curSystem != shipData[i].curSystem)
            continue;
        if (AIShipIsEmergingFromHypergate(&shipData[i]))
            continue;
        if (!hasDetect1 && (gShipTypes[shipData[i].type].flags2 & kShipFlag2Untargetable) != 0)
            continue;

        dx = FloatAbs(shipData[0].pos.x - shipData[i].pos.x);
        dy = FloatAbs(shipData[0].pos.y - shipData[i].pos.y);
        dist = dx * dx + dy * dy;
        if (dist < bestDist || bestDist < 0.0f) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

/* IsThreatToStellar (mac _IsThreatToStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004629e0
char IsThreatToStellar(ShipObj *ship, SpobRecord *spob)
{
    short target;
    char found;

    if (spob->dominated != 0)
        return 0;

    if (ship->index != 0) {
        if (AIIsShipJumpingIn(ship) != 0)
            return 0;
    }

    if (spob->flags2 & 0x200) {
        if (spob->defenseActive != 0) {
            if (ship->index == 0 || ship->parentShip == 0)
                return 1;
        }
        goto fail;
    }

    if (ship->index != 0 && ship->parentShip != 0) {
        if (GovtEnemies(spob->govt, ship->govt))
            return 1;
        goto fail;
    }

    target = -1;
    if (spob->govt == -1) {
        short sysGovt = gSystems[shipData->curSystem].govt;
        if (sysGovt != -1)
            target = sysGovt;
    } else {
        target = spob->govt;
    }

    found = 0;
    if (target != -1) {
        if (playerLegalStatus[shipData->curSystem] < -govtData[target].crimeTol)
            found = 1;
        if (!found) {
            if (govtData[target].flags & kGovtXenophobic) {
                if (playerLegalStatus[shipData->curSystem] < 0)
                    found = 1;
            } else {
                if (GovtEnemies(gShipTypes[shipData->type].inherentGovt, spob->govt))
                    found = 1;
            }
        }
        if (found) {
            if (PlayerBlessedByGovt(spob->govt, 0))
                found = 0;
            if (govtData[target].iffScrambled != 0)
                found = 0;
        }
    }

    if (dockTarget == spob->index)
        found = 0;

    if (!found)
        goto fail;
    return 1;

fail:
    return 0;
}

#define kGateRange2 250000.0

/* FindNearestThreatToPlayer (mac _FindNearestThreatToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462bd0
short FindNearestThreatToPlayer(void)
{
    short i;
    short best;
    short target;
    char hasDetect1;
    char hasDetect2;
    char valid;
    float bestDist;
    float dx;
    float dy;
    float dist;

    hasDetect1 = ShipCanTargetUntargetableShips(shipData);
    hasDetect2 = ShipCanTargetCloakedShips(shipData);
    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kMaxShips; i++) {
        if (i == 0)
            continue;
        if (!shipData[i].active)
            continue;
        if (IsDying(&shipData[i]))
            continue;
        if (IsDisabled(&shipData[i]))
            continue;
        if (ShipIsCloaked(&shipData[i]) && !hasDetect2)
            continue;
        if (shipData[i].parentShip == 0)
            continue;
        if (shipData[0].curSystem != shipData[i].curSystem)
            continue;
        if (!hasDetect1 && (gShipTypes[shipData[i].type].flags2 & kShipFlag2Untargetable) != 0)
            continue;

        valid = 0;
        if (IsThreatToPlayer(&shipData[i])) {
            valid = 1;
        } else {
            target = shipData[i].targetShip;
            if (target != -1 && shipData[target].parentShip == 0 && shipData[target].active
                    && IsShipFightingMe(&shipData[i], &shipData[target])) {
                valid = 1;
            }
        }
        if (valid) {
            dx = FloatAbs(shipData[0].pos.x - shipData[i].pos.x);
            dy = FloatAbs(shipData[0].pos.y - shipData[i].pos.y);
            dist = dx * dx + dy * dy;
            if (dist < bestDist || bestDist < 0.0f) {
                bestDist = dist;
                best = i;
            }
        }
    }
    return best;
}

/* FindNearestStellar (mac _FindNearestStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462db0
short FindNearestStellar(ShipObj *ship)
{
    short i;
    short stellar;
    short best;
    char inRange;
    float bestDist;
    float dx;
    float dy;
    float dist;

    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kNavSlots; i++) {
        stellar = gSystems[ship->curSystem].navDefault[i];
        if (stellar != -1 && stellarData[stellar].loaded != 0
            && SpobCanLand(&stellarData[stellar])) {
            dx = FloatAbs(ship->pos.x - stellarData[stellar].pos.h);
            dy = FloatAbs(ship->pos.y - stellarData[stellar].pos.v);
            dist = dx * dx + dy * dy;
            if (SpobIsHyperOrWorm(&stellarData[stellar])) {
                if (dist < kGateRange2)
                    inRange = 1;
                else
                    inRange = 0;
            } else
                inRange = 1;
            if (inRange && (dist < bestDist || bestDist < 0.0f)) {
                bestDist = dist;
                best = stellar;
            }
        }
    }
    return best;
}

/* ShipStatsToSystemInfo. STATE: MATCH 100.0%, 195/195, byte-identical yes (927 bytes).
   OPEN: none */
// FUNCTION: EVNOVA 0x00462ec0
void ShipStatsToSystemInfo(void)
{
    short countBuf[kMaxWeapTypes];
    short ammoBuf[kMaxWeapTypes];
    short k;
    short slot;
    short i;

    for (i = 0; i < kMaxWeapTypes; i++) {
        countBuf[i] = shipData->weap[i].count;
        ammoBuf[i] = shipData->weap[i].ammo;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] <= 0)
            continue;
        if (gOutfits[i].techLevel >= 0x8000)
            continue;
        if (gOutfits[i].modType[0] == kOutfModWeapon
                && gOutfits[i].modVal[0] >= 0 && gOutfits[i].modVal[0] < kMaxWeapTypes) {
            slot = gOutfits[i].modVal[0];
            if (shipSystems[i] >= countBuf[slot]) {
                shipSystems[i] = countBuf[slot];
                countBuf[slot] = 0;
            } else {
                countBuf[slot] -= shipSystems[i];
            }
        } else if (gOutfits[i].modType[0] == kOutfModAmmo
                && gOutfits[i].modVal[0] >= 0 && gOutfits[i].modVal[0] < kMaxWeapTypes) {
            slot = gOutfits[i].modVal[0];
            if (shipSystems[i] >= ammoBuf[slot]) {
                shipSystems[i] = ammoBuf[slot];
                ammoBuf[slot] = 0;
            } else {
                ammoBuf[slot] -= shipSystems[i];
            }
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] < 0)
            shipSystems[i] = 0;
    }

    for (k = 0; k < kMaxWeapTypes; k++) {
        if (countBuf[k] > 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (gOutfits[i].techLevel < 0x8000 && gOutfits[i].modType[0] == kOutfModWeapon
                        && gOutfits[i].modVal[0] == k) {
                    shipSystems[i] += countBuf[k];
                    countBuf[k] = 0;
                    break;
                }
            }
        }
        if (ammoBuf[k] > 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (gOutfits[i].techLevel < 0x8000 && gOutfits[i].modType[0] == kOutfModAmmo
                        && gOutfits[i].modVal[0] == k) {
                    shipSystems[i] += ammoBuf[k];
                    ammoBuf[k] = 0;
                    break;
                }
            }
        }
    }

    ResetPlayerPrecalcedValues();
}

/*
 * modType[0] == 1 IS a weapon (modVal[0] names the slot - add the owned count
 * to weap[].count), modType[0] == 3 IS ammo for a weapon slot (add to
 * weap[].ammo). Same modType convention loaddata.c's def-outfit grant scan and
 * the shipyard's persistent-outfit check already use.
 */

/* SystemInfoToShipStats. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463260
void SystemInfoToShipStats(void)
{
    short i;

    for (i = 0; i < 256; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }

    for (i = 0; i < 512; i++) {
        if (shipSystems[i] <= 0)
            continue;
        if (gOutfits[i].techLevel >= kOutfTechAbsent)
            continue;
        if (gOutfits[i].modType[0] == kOutfModWeapon)
            shipData->weap[gOutfits[i].modVal[0]].count += shipSystems[i];
        if (gOutfits[i].modType[0] == kOutfModAmmo)
            shipData->weap[gOutfits[i].modVal[0]].ammo += shipSystems[i];
    }

    ResetPlayerPrecalcedValues();
}

/* FreeMass. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463470
long FreeMass(void)
{
    int i;
    short type;
    long free;
    long mass;

    type = shipData[0].type;
    if (type < 0 || type >= 768)
        return 0;
    free = gShipTypes[type].freeMass;
    for (i = 0; (short)i < 512; i++) {
        if (shipSystems[(short)i] > 0) {
            mass = AdjustedItemMass(&gOutfits[i], &gShipTypes[shipData[0].type]);
            free -= mass * shipSystems[(short)i];
        }
    }
    if (free < 0)
        free = 0;
    return free;
}

/*
 * Both walk the 512 outfit slots and check each owned outfit's four mod slots
 * (modType 4 = shield boost, 15 = afterburner); both results are cached in
 * player-scoped globals that other code invalidates.
 */

#define kOutfModShieldBoost 4
#define kOutfModAfterburner 15

/* XYShortToFloat (mac _XYShortToFloat). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463520
FloatPoint XYShortToFloat(ShortPoint pt)
{
    FloatPoint f;

    f.x = pt.h;
    f.y = pt.v;
    return f;
}

/* not linked, not verified against the exe (mac _XYFloatToShort, _VectorToAngle, _CalcShipStopDist) */
ShortPoint XYFloatToShort(FloatPoint f)
{
    ShortPoint pt;

    pt.h = f.x;
    pt.v = f.y;
    return pt;
}

short VectorToAngle(FloatPoint *v)
{
    Rect unit;
    Point pt;
    short angle;

    pt.h = v->x * 1000.0f;
    pt.v = v->y * 1000.0f;
    SetRect(&unit, -1, -1, 1, 1);
    PtToAngle(&unit, pt, &angle);
    return angle;
}

float CalcShipStopDist(void)
{
    return 20000.0f;
}

/* ShipShieldCapacity (mac _ShipShieldCapacity). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463550
float ShipShieldCapacity(ShipObj *ship)
{
    long i;
    float cap;
    short j;
    short pers;
    float mult;

    cap = gShipTypes[ship->type].shield;
    if (ship->index == 0) {
        if (playerShieldCapacity >= 0.0f) {
            cap = playerShieldCapacity;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModShieldBoost)
                            cap += gOutfits[i].modVal[j] * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerShieldCapacity = cap;
        }
    } else {
        pers = ship->persType;
        if (pers != -1) {
            mult = personData[pers].shieldMod;
            if (mult > 0.0f)
                cap = cap * mult;
        }
    }
    if (ship->index != 0 && ship->aiType == 5)
        cap = cap * 1.333;
    return cap;
}

#define kOutfModShieldRech 5

/* ShipShieldRechargeRate (mac _ShipShieldRechargeRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463680
float ShipShieldRechargeRate(ShipObj *ship)
{
    short i;
    float cap;
    short j;
    int tmp;

    tmp = ship->type;
    cap = gShipTypes[tmp].shieldRech;
    if (ship->index == 0) {
        if (gCachedShieldRech >= 0.0f) {
            cap = gCachedShieldRech;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModShieldRech)
                            cap += gOutfits[i].modVal[j] * 0.001
                                   * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            gCachedShieldRech = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (ship->index != 0 && ship->aiType == 5)
        cap = cap * 1.333;
    return cap;
}

/*
 * The persType guard is a RANGE here (0 <= pers < 1024, the array's own size)
 * where the shield twin tests `pers != -1`, and the base is `fild`ed from a
 * long (`armor` at ShipRec+0x60) where shieldRech's was a float.
 */

#define kOutfModArmor 6

/* ShipArmorCapacity (mac _ShipArmorCapacity). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004637a0
float ShipArmorCapacity(ShipObj *ship)
{
    short i;
    float cap;
    short j;
    short pers;
    float mult;

    cap = gShipTypes[ship->type].armor;
    if (ship->index == 0) {
        if (gCachedArmorCap >= 0.0f) {
            cap = gCachedArmorCap;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModArmor)
                            cap += gOutfits[i].modVal[j] * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            gCachedArmorCap = cap;
        }
    } else {
        pers = ship->persType;
        if (pers >= 0 && pers < kMaxPersons) {
            mult = personData[pers].shieldMod;
            if (mult > 0.0f)
                cap = cap * mult;
        }
    }
    if (ship->index != 0 && ship->aiType == 5)
        cap = cap * 1.333;
    return cap;
}

#define kOutfModArmorRech 0x1d

/* ShipArmorRechargeRate (mac _ShipArmorRechargeRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004638e0
float ShipArmorRechargeRate(ShipObj *ship)
{
    short i;
    float cap;
    short j;

    if (IsDisabled(ship))
        return 0.0f;
    cap = gShipTypes[ship->type].armorRech;
    if (ship->index == 0) {
        if (gCachedArmorRech >= 0.0f) {
            cap = gCachedArmorRech;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModArmorRech)
                            cap += gOutfits[i].modVal[j] * 0.001
                                   * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            gCachedArmorRech = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (ship->index == 0 && cheatsActive != 0)
        cap = cap * 50.0f;
    return cap;
}

#define kOutfModType12  0xc
#define kMaxFuel        32000

/* ShipFuelCapacity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463a20
short ShipFuelCapacity(ShipObj *ship)
{
    short i;
    short j;
    long total;

    total = gShipTypes[ship->type].fuel;
    if (ship->index == 0) {
        if (gCachedFuelCap >= 0.0f) {
            total = gCachedFuelCap;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType12)
                            total += gOutfits[i].modVal[j]
                                     * shipSystems[i];
                    }
                }
            }
            if (total < 0)
                total = 0;
            if (total > kMaxFuel)
                total = kMaxFuel;
            gCachedFuelCap = total;
        }
    }
    return total;
}

#define kOutfModType18  0x12
#define kShipFlagRate   0x8    /* ShipRec flags bit 3; meaning TBC */

/* ShipFuelGenRate (mac _ShipFuelGenRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463b30
char ShipFuelGenRate(ShipObj *ship, float *rate)
{
    short i;
    short j;

    if (ship->fuelRegenState < 0) {
        ship->fuelRegenState = 0;
        ship->fuelRegenRate = 0;
        if (gShipTypes[ship->type].fuelRegen > 0 &&
            ((gShipTypes[ship->type].flags & kShipFlagRate) != 0 ||
             ship->index != 0)) {
            ship->fuelRegenState = 1;
            ship->fuelRegenRate = 1.0 / gShipTypes[ship->type].fuelRegen;
        }
        if (ship->index == 0) {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType18) {
                            ship->fuelRegenState = 1;
                            ship->fuelRegenRate += 1.0 / gOutfits[i].modVal[j] *
                                             shipSystems[i];
                        }
                    }
                }
            }
        } else {
            for (i = 0; i < 8; i++) {
                if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                    short id = gShipTypes[ship->type].defOutfID[i];

                    if (id >= 0 && id < 512) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (gOutfits[id].modType[j] == kOutfModType18) {
                                ship->fuelRegenState = 1;
                                ship->fuelRegenRate += 1.0 / gOutfits[id].modVal[j] *
                                    gShipTypes[ship->type].defOutfCount[i];
                            }
                        }
                    }
                }
            }
        }
    }
    if (ship->fuelRegenState == 0) {
        *rate = 0;
        return 0;
    }
    *rate = ship->fuelRegenRate;
    return 1;
}

#define kWeapNeedsQuickTurn 0x8    /* name TBC */

/* SuitableMissileType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463dc0
char SuitableMissileType(short weap, ShipObj *ship)
{
    short turns;

    turns = ShipTurnRate(ship);
    if ((gWeapons[weap].flags & kWeapNeedsQuickTurn) != 0 && turns > 3)
        return 0;
    if (gWeapons[weap].guidedTurn <= 2.0f && turns > 3)
        return 0;
    return 1;
}

#define kOutfModSpeed    8

#define kOutfModTurn    9

/* ShipTurnRate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463e70
float ShipTurnRate(ShipObj *ship)
{
    short i;
    float v;
    short j;
    float ratio;

    v = gShipTypes[ship->type].turnRate;
    if (ship->index == 0) {
        if (gCachedTurnRate >= 0.0f) {
            v = gCachedTurnRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModTurn)
                            v += gOutfits[i].modVal[j]
                                 * shipSystems[i] * 0.01;
                    }
                }
            }
            if (v < 1.0f && gShipTypes[ship->type].turnRate > 0.0f)
                v = 1.0f;
            gCachedTurnRate = v;
        }
    } else {
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
    }
    if (ship->persType == kPersCaptainHector) {
        if (v < 6.0f)
            v += 1.0f;
    }
    if (gShipTypes[ship->type].turnRate >= 1.0f && v < 1.0f)
        v = 1.0f;
    if (ship->ionization > 0.0f && ship->jumpTimer <= 0.0f) {
        ratio = ShipIonizationFactor(ship);
        if (ratio > 0.7)
            ratio = 0.7f;
        v *= (1.0 - ratio);
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

#define kOutfModAccel   7

/* ShipAccelRate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004640a0
float ShipAccelRate(ShipObj *ship)
{
    short i;
    float v;
    short j;
    float ratio;

    v = gShipTypes[ship->type].accel;
    if (ship->index == 0) {
        if (gCachedAccelRate >= 0.0f) {
            v = gCachedAccelRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModAccel)
                            v += (float)(gOutfits[i].modVal[j] / 10000.0)
                                 * shipSystems[i];
                    }
                }
            }
            if (v < 0.0f)
                v = 0.0f;
            gCachedAccelRate = v;
        }
        v *= 2.0;
    } else {
        if ((gShipTypes[ship->type].flags & kShipFlagNoMove) != 0)
            return 0.0f;
        v *= ship->skill;
        if (ship->govt != -1)
            v *= govtData[ship->govt].skillMult;
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
        else
            v *= 2.0;
    }
    if (ship->persType == kPersCaptainHector)
        v *= 2.0;
    if (ship->ionization > 0.0f) {
        ratio = ShipIonizationFactor(ship);
        if (ratio > 0.7)
            ratio = 0.7f;
        v *= (1.0 - ratio);
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

/* ShipMaxSpeed. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004642e0
float ShipMaxSpeed(ShipObj *ship)
{
    short i;
    float v;
    short j;

    v = gShipTypes[ship->type].maxSpeed;
    if (ship->index == 0) {
        if (gCachedMaxSpeed >= 0.0f) {
            v = gCachedMaxSpeed;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModSpeed)
                            v += (float)(gOutfits[i].modVal[j] / 100.0)
                                 * shipSystems[i];
                    }
                }
            }
            if (v < 0.0f)
                v = 0.0f;
            gCachedMaxSpeed = v;
        }
    } else {
        if ((gShipTypes[ship->type].flags & kShipFlagNoMove) != 0)
            return 0.0f;
        v = v * ship->skill;
        if (ship->govt != -1)
            v = v * govtData[ship->govt].skillMult;
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v = v * 0.333;
    }
    if (ship->persType == kPersCaptainHector)
        v = v * 2.0;
    if (ship->index == 0 || ship->parentShip == 0) {
        if (strictPlay == 0)
            v = v * 1.5;
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

/* Guidance 99 is the carried-ship type. */

#define kOutfModBay      11
#define kShipFlagCarried  0x8000  /* ShipRec flags bit 15; name TBC */

/* HasEscapePod (mac _HasEscapePod). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004644a0
char HasEscapePod(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModBay)
                        return 1;
                }
            }
        }
    }
    if (HasEscapeShip(ship))
        return 1;
    return 0;
}

/* HasEscapeShip (mac _HasEscapeShip). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464520
char HasEscapeShip(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < 256; i++) {
        if (gWeapons[i].guidance == kGuidanceBay &&
            ship->weap[i].ammo > 0) {
            type = gWeapons[i].ammoType - 128;
            if ((gShipTypes[type].flags & kShipFlagCarried) != 0)
                return 1;
        }
    }
    return 0;
}

/* EscapeShipType (mac _EscapeShipType). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464590
short EscapeShipType(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < 256; i++) {
        if (gWeapons[i].guidance == kGuidanceBay &&
            ship->weap[i].ammo > 0) {
            type = gWeapons[i].ammoType - 128;
            if ((gShipTypes[type].flags & kShipFlagCarried) != 0)
                return type;
        }
    }
    return -1;
}

#define kShipCanLaunch 0x8000       /* name TBC */

/* EscapeShipWeapType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464600
short EscapeShipWeapType(ShipObj *ship)
{
    int i;
    short type;

    for (i = 0; (short)i < kMaxWeapTypes; i++) {
        if (gWeapons[i].guidance == kGuidanceBay && ship->weap[i].ammo > 0) {
            type = gWeapons[i].ammoType - kFirstResourceID;
            if ((gShipTypes[type].flags & kShipCanLaunch) != 0)
                return i;
        }
    }
    return -1;
}

/* KeyCarriedShipTypeOnboard (mac _KeyCarriedShipTypeOnboard). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464670
char KeyCarriedShipTypeOnboard(ShipObj *ship)
{
    short i;
    short escort;
    short bay;

    escort = gShipTypes[ship->type].keyCarried;
    if (escort == -1)
        return 0;
    for (i = 0; i < 256; i++) {
        if (gWeapons[i].guidance == kGuidanceBay &&
            ship->weap[i].count > 0 && ship->weap[i].ammo > 0) {
            bay = (gWeapons[i].ammoType - 128);
            if (bay == escort)
                return 1;
        }
    }
    return 0;
}

#define kOutfModType20 0x14
#define kOutfModType15 0xf

/* does the player own any outfit carrying modifier type 0x14 */

/* HasAutoEject (mac _HasAutoEject). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464700
char HasAutoEject(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType20)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* HasAfterburner (mac _HasAfterburner). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464760
char HasAfterburner(ShipObj *ship)
{
    short i;
    short j;

    if (cheatsActive != 0)
        return 1;
    if (ship->index == 0) {
        if (playerHasAfterburner >= 0) {
            if (playerHasAfterburner != 0)
                return 1;
        } else {
            playerHasAfterburner = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType15) {
                            playerHasAfterburner = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

#define kOutfModType33  0x21
#define kGovtHalfDefense 0x80

/* ShipECM (mac _ShipECM). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464810
short ShipECM(ShipObj *ship, short kind)
{
    short i;
    short j;

    if (ship == NULL)
        return 0;
    if (kind < 0 || kind >= 4)
        return 0;
    if (IsDisabled(ship))
        return 0;
    if (ship->jamming[kind] >= 0)
        return ship->jamming[kind];

    if (gShipTypes[ship->type].inherentGovtB >= 0 &&
        gShipTypes[ship->type].inherentGovtB < 256)
        ship->jamming[kind] =
            govtData[gShipTypes[ship->type].inherentGovtB].inhJam[kind];
    else
        ship->jamming[kind] = 0;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType33 + kind)
                        ship->jamming[kind] += gOutfits[i].modVal[j];
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType33 + kind)
                        ship->jamming[kind] +=
                            gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j];
                }
            }
        }
        if (ship->govt >= 0 && ship->govt < 256 &&
            (govtData[ship->govt].flags & kGovtHalfDefense) != 0)
            ship->jamming[kind] = ship->jamming[kind] / 2;
    }

    if (ship->jamming[kind] < 0)
        ship->jamming[kind] = 0;
    if (ship->jamming[kind] > 100)
        ship->jamming[kind] = 100;
    return ship->jamming[kind];
}

/*
 * `-opt level=4,intrinsics` is forced: the exe emits `fabs` and THEN
 * `fst st(1)`, the compiler intrinsic operating in place, where MSL's
 * inline-asm fabs binds its parameter first and emits `fstp st(1)`.
 */


#define kDetectRange    200.0f

/* ShipVisibleToShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464a90
char ShipVisibleToShip(ShipObj *ship, ShipObj *viewer)
{
    if (AIShipIsEmergingFromHypergate(ship))
        return 0;
    if (viewer->persType == kPersCaptainHector)
        return 1;
    if (ShipIsCloaked(ship)) {
        float d;

        if (viewer->parentShip == ship->index) {
            if (HasCloak(viewer))
                return 1;
        }
        if (viewer->seesCloaked == 1) {
            d = fabs(viewer->pos.x - ship->pos.x);
            if (d <= kDetectRange) {
                d = fabs(viewer->pos.y - ship->pos.y);
                if (d <= kDetectRange)
                    return 1;
            }
        }
        return 0;
    }
    return 1;
}

#define kOutfModType13 0xd
#define kOutfModType14 0xe
#define kOutfModType30 0x1e
#define kOutfModVal30Bit2 4
#define kOutfModVal30Bit3 8
#define kOutfModNibblePair 0x11

/*
 * Every one of those four guards falls back to the default scan rather than to
 * a no, which is why they are written as a chain of `if (!x) goto the scan`
 * rather than a single `&&`.
 */

/* HasCloak (mac _HasCloak). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464b50
char HasCloak(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType17)
                        return 1;
                }
            }
        }
    } else {
        if (ship->parentShip != -1 && IsShipInCloseForm(ship) &&
            ShipIsCloaked(&shipData[ship->parentShip]) &&
            HasAreaCloak(&shipData[ship->parentShip]))
            return 1;
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* the same question as HasCloak but only for values carrying bit 12 */
#define kOutfModVal17Bit12 0x1000

/* HasAreaCloak (mac _HasAreaCloak). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464c80
char HasAreaCloak(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasAreaCloak == -1) {
            playerHasAreaCloak = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType17 &&
                            (gOutfits[i].modVal[j] & kOutfModVal17Bit12) != 0) {
                            playerHasAreaCloak = 1;
                            return 1;
                        }
                    }
                }
            }
        } else if (playerHasAreaCloak == 1) {
            return 1;
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17 &&
                        (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal17Bit12) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/*
 * The first owned mod-type-17 outfit's high nibble (win 0x00464db0). What
 * that mod type means is not established here — only that its modVal is read
 * as two nibbles and this returns the upper one, with 0 for anything but the
 * player's own ship.
 */
/* CloakFuelUsage (mac _CloakFuelUsage). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464db0
short CloakFuelUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModNibblePair)
                        return (gOutfits[i].modVal[j] >> 4) & 0xf;
                }
            }
        }
    }
    return 0;
}

/* CloakZeroesShields (mac _CloakZeroesShields). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464e30
char CloakZeroesShields(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType17) {
                        if ((gOutfits[i].modVal[j] & kOutfModVal30Bit2) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17) {
                        if ((gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                             kOutfModVal30Bit2) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

/* ShipDecloaksWhenHit (mac _ShipDecloaksWhenHit). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464f60
char ShipDecloaksWhenHit(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType17) {
                        if ((gOutfits[i].modVal[j] & kOutfModVal30Bit3) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17) {
                        if ((gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                             kOutfModVal30Bit3) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

/* CloakShieldUsage (mac _CloakShieldUsage). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465090
short CloakShieldUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType17)
                        return (gOutfits[i].modVal[j] >> 8) & 0xf;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17)
                        return (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] >>
                                8) & 0xf;
                }
            }
        }
    }
    return 0;
}

/*
 * A non-matching bit does NOT stop the scan - the walk keeps going, so the
 * test is one `&&` condition rather than a nested pair with their own
 * epilogues.
 */
#define kOutfModVal17Bit1 2

/* ShipCloakVisibleOnRadar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004651a0
char ShipCloakVisibleOnRadar(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType17 &&
                        (gOutfits[i].modVal[j] & kOutfModVal17Bit1) != 0)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17 &&
                        (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal17Bit1) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

#define kPersTypeAlways 0x3ff
#define kOutfModVal30Bit0 1
#define kOutfModVal30Bit1 2

/* ShipCanSeeCloakedShips (mac _ShipCanSeeCloakedShips). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004652a0
char ShipCanSeeCloakedShips(ShipObj *ship, short which)
{
    short i;
    short j;

    if (ship->persType == kPersTypeAlways)
        return 1;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType30) {
                        if ((gOutfits[i].modVal[j] & kOutfModVal30Bit1) != 0 &&
                            which == 0)
                            return 1;
                        if ((gOutfits[i].modVal[j] & kOutfModVal30Bit0) != 0 &&
                            which == 1)
                            return 1;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType30) {
                        if (which == 0 &&
                            (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                                kOutfModVal30Bit1) != 0)
                            return 1;
                        if (which == 1 &&
                            (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                                kOutfModVal30Bit0) != 0)
                            return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* HasDensityScanner. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465410
char HasDensityScanner(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (gHasModType13 >= 0) {
            if (gHasModType13 != 0)
                return 1;
        } else {
            gHasModType13 = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType13) {
                            gHasModType13 = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* the literal twin of the above, one modifier type along and its own cache */
/* HasIFF (mac _HasIFF). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004654b0
char HasIFF(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (gHasModType14 >= 0) {
            if (gHasModType14 != 0)
                return 1;
        } else {
            gHasModType14 = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType14) {
                            gHasModType14 = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

#define kOutfModType22 0x16

/* ShipHyperTransitTime (mac _ShipHyperTransitTime). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465550
short ShipHyperTransitTime(ShipObj *ship)
{
    int i;
    short j;
    short count;
    short mass;

    count = 1;
    mass = gShipTypes[ship->type].mass;
    if (mass >= 100)
        count++;
    if (mass >= 200)
        count++;
    if (ship->index == 0) {
        for (i = 0; (short)i < 512; i++) {
            if (shipSystems[(short)i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType22)
                        count += shipSystems[(short)i] * gOutfits[i].modVal[j];
                }
            }
        }
    }
    if (count < 1)
        count = 1;
    return count;
}

/*
 * A squared range: floored at zero and then squared, so the caller can compare
 * against a squared distance without a square root.
 */
#define kOutfModType23 0x17

/* ShipHyperSafeDist. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465610
long ShipHyperSafeDist(ShipObj *ship)
{
    int i;
    short j;
    long range;

    range = 1000;
    if (ship->index == 0) {
        for (i = 0; (short)i < 512; i++) {
            if (shipSystems[(short)i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType23)
                        range += shipSystems[(short)i] *
                                 gOutfits[i].modVal[j];
                }
            }
        }
    }
    if (range < 0)
        range = 0;
    return range * range;
}

#define kModTypeMaxOwned   0x1b
#define kModTypeGunLimit   0x2d
#define kModTypeTurretLimit 0x2e

/* HasMaxOfItem. STATE: MATCH, 299/299; byte check pending the next regate (bytediff reads
   the baseline). OPEN: none */
// FUNCTION: EVNOVA 0x004656a0
char HasMaxOfItem(short index, short *outBound, short *outCap)
{
    long idx;
    short limit;
    short maxCount;
    short maxAmmo;
    short j;
    short k;
    short sumOwned;
    short sumFlag1;
    short sumFlag2;

    if (index < 0 || index >= 0x200) {
        if (outBound)
            *outBound = 0;
        if (outCap)
            *outCap = 0;
        return 1;
    }

    idx = index;
    maxCount = gOutfits[idx].maxCount;
    if (outCap)
        *outCap = maxCount;

    if (gOutfits[idx].modType[0] == kOutfModAmmo) {
        maxAmmo = gWeapons[gOutfits[idx].modVal[0]].maxAmmo;
        if (maxAmmo > 0) {
            limit = maxAmmo * shipData->weap[gOutfits[(unsigned long)idx].modVal[0]].count;
            if (outCap && limit < *outCap)
                *outCap = limit;
            if (shipSystems[idx] >= limit) {
                if (outBound)
                    *outBound = limit;
                return 1;
            }
        }
    }

    sumOwned = 0;
    for (j = 0; j < 0x200; j++) {
        if (shipSystems[j] > 0) {
            for (k = 0; k < kOutfMods; k++) {
                if (gOutfits[j].modType[k] == kModTypeMaxOwned &&
                    gOutfits[j].modVal[k] == index + 128)
                    sumOwned += shipSystems[j];
            }
        }
    }
    if (sumOwned < 1)
        sumOwned = 1;
    limit = sumOwned * gOutfits[idx].maxCount;
    if (outCap && (limit < *outCap || sumOwned > 1))
        *outCap = limit;
    if (shipSystems[idx] >= limit) {
        if (outBound)
            *outBound = limit;
        return 1;
    }

    sumFlag1 = sumFlag2 = 0;
    for (j = 0; j < 0x200; j++) {
        if (shipSystems[j] > 0) {
            if (gOutfits[j].flags & kOutfFixedGun)
                sumFlag1 += shipSystems[j];
            if (gOutfits[j].flags & kOutfTurret)
                sumFlag2 += shipSystems[j];
        }
    }

    if (gOutfits[idx].flags & kOutfFixedGun) {
        limit = gShipTypes[shipData->type].maxGuns;
        for (j = 0; j < 0x200; j++) {
            if (shipSystems[j] > 0) {
                for (k = 0; k < kOutfMods; k++) {
                    if (gOutfits[j].modType[k] == kModTypeGunLimit) {
                        limit += gOutfits[j].modVal[k];
                        break;
                    }
                }
            }
        }
        if (outCap && limit < *outCap)
            *outCap = limit;
        if (limit > 0) {
            if (sumFlag1 >= limit) {
                if (outBound)
                    *outBound = limit;
                return 1;
            }
        } else {
            if (outBound)
                *outBound = 0;
            return 1;
        }
    }

    if (gOutfits[idx].flags & kOutfTurret) {
        limit = gShipTypes[shipData->type].maxTurrets;
        for (j = 0; j < 0x200; j++) {
            if (shipSystems[j] > 0) {
                for (k = 0; k < kOutfMods; k++) {
                    if (gOutfits[j].modType[k] == kModTypeTurretLimit) {
                        limit += gOutfits[j].modVal[k];
                        break;
                    }
                }
            }
        }
        if (outCap && limit < *outCap)
            *outCap = limit;
        if (limit > 0) {
            if (sumFlag2 >= limit) {
                if (outBound)
                    *outBound = limit;
                return 1;
            }
        } else {
            if (outBound)
                *outBound = 0;
            return 1;
        }
    }

    if (outBound)
        *outBound = shipSystems[idx];
    return 0;
}

/* DrawCommaNumber. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465af0
void DrawCommaNumber(long value)
{
    long group;
    unsigned char str[256];

    if (value < 1000) {
        NumToString(value, str);
        DrawString(str);
    } else if (value < 1000000) {
        group = value % 1000;
        NumToString(value / 1000, str);
        DrawString(str);
        DrawString((unsigned char *)"\1,");
        if (group < 100)
            DrawString((unsigned char *)"\0010");
        if (group < 10)
            DrawString((unsigned char *)"\0010");
        NumToString(group, str);
        DrawString(str);
    } else {
        NumToString(value / 1000000, str);
        DrawString(str);
        DrawString((unsigned char *)"\1.");
        group = (value % 1000000) / 10000;
        NumToString(group, str);
        if (group < 10)
            DrawString((unsigned char *)"\0010");
        DrawString(str);
        DrawString((unsigned char *)"\1M");
    }
}

/* StrcatCommaNumber. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465c10
void StrcatCommaNumber(long value, char *dst)
{
    long group;
    unsigned char str[256];

    if (value < 1000) {
        NumToString(value, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
    } else if (value < 1000000) {
        group = value % 1000;
        NumToString(value / 1000, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
        strcat(dst, ",");
        if (group < 100)
            strcat(dst, "0");
        if (group < 10)
            strcat(dst, "0");
        NumToString(group, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
    } else {
        NumToString(value / 1000000, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
        strcat(dst, ".");
        group = (value % 1000000) / 10000;
        NumToString(group, str);
        if (group < 10)
            strcat(dst, "0");
        PtoCstr(str);
        strcat(dst, (char *)str);
        strcat(dst, "M");
    }
}

/*
 * Append an ordinal to a C string: first through tenth come out of STR# 137
 * (its entries 29..38), anything else is spelled in digits. The word is built
 * in its own buffer so the leading letter can be capitalised before it is
 * appended.
 */
/* StrcatOrdinalNumber. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465d90
void StrcatOrdinalNumber(short n, char *dst, char capitalize)
{
    unsigned char numStr[256];
    unsigned char word[256];

    if (n >= 1 && n <= 10) {
        strcpy((char *)word, "");
        AppendIndString((char *)word, kOrdinalsSTR, n + kOrdinalsBase);
    } else {
        NumToString(n, numStr);
        PtoCstr(numStr);
        strcpy((char *)word, (char *)numStr);
    }
    if (capitalize != 0)
        word[0] = toupper(word[0]);
    strcat(dst, (char *)word);
}

/* CanBuyGoods (mac _CanBuyGoods). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465e50
char CanBuyGoods(short item)
{
    if (shipData->credits < tradePrice[item])
        return 0;
    if (TotalCargo(shipData) >= TotalFleetHolds())
        return 0;
    return 1;
}

/* CanSellGoods (mac _CanSellGoods). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465ea0
char CanSellGoods(short item)
{
    short junk;

    if (item <= 5) {
        if (shipData->cargo[item] > 0)
            return 1;
    } else {
        junk = junkType[item - kFirstJunkItem];
        if (junk != -1 && junkData[junk].held > 0)
            return 1;
    }
    return 0;
}

/* the player's standing in each system, inside the pilot record */

/* ColorCodeShip (mac _ColorCodeShip). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465f00
void ColorCodeShip(ShipObj *ship, RGBColor *color)
{
    short parent;

    if (ship->index == 0) {
        color->red = 0;
        color->green = 0xffff;
        color->blue = 0xffff;
        return;
    }
    if (IsDisabled(ship)) {
        color->red = rgbDarkGrey.red;
        color->green = rgbDarkGrey.green;
        color->blue = rgbDarkGrey.blue;
    } else if (IsThreatToPlayer(ship)) {
        color->red = 0xffff;
        color->green = 0;
        color->blue = 0;
    } else {
        parent = ship->parentShip;
        if (parent == 0 && ship->defenseStellar == -1) {
            color->red = brightGreen.red;
            color->green = brightGreen.green;
            color->blue = brightGreen.blue;
        } else if (parent == -1) {
            color->red = 0;
            color->green = 0;
            color->blue = 0xffff;
        } else if (shipData[parent].parentShip == 0) {
            color->red = brightGreen.red;
            color->green = brightGreen.green;
            color->blue = brightGreen.blue;
        } else {
            color->red = 0;
            color->green = 0;
            color->blue = 0xffff;
        }
    }
}

/* ColorCodeStellar (mac _ColorCodeStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466030
void ColorCodeStellar(SpobRecord *spob, RGBColor *color)
{
    char uninhabited;
    short minStatus;

    uninhabited = SpobUninhabited(spob);
    if (!StellarIsLandable(spob)) {
        color->red = rgbMedGrey.red;
        color->green = rgbMedGrey.green;
        color->blue = rgbMedGrey.blue;
    } else if (uninhabited) {
        color->red = rgbMedGrey.red;
        color->green = rgbMedGrey.green;
        color->blue = rgbMedGrey.blue;
    } else if (spob->flags2 & kSpobHypergate) {
        if (spob->govt == -1 || !PlayerBlessedByGovt(spob->govt, 1)) {
            minStatus = spob->minStatus;
            if (minStatus == 0x7fff ||
                (playerLegalStatus[spob->system] < minStatus && minStatus > -0x7fff)) {
                if (playerLegalStatus[spob->system] < 0) {
                    color->red = 0xffff;
                    color->green = 0;
                    color->blue = 0;
                } else {
                    color->red = 0xffff;
                    color->green = 0x6666;
                    color->blue = 0;
                }
            } else {
                color->red = 0xffff;
                color->green = 0xffff;
                color->blue = 0;
            }
        } else {
            color->red = 0xffff;
            color->green = 0xffff;
            color->blue = 0;
        }
    } else if (spob->flags2 & kSpobWormhole) {
        color->red = 0;
        color->green = 0x55f0;
        color->blue = 0x55f0;
    } else if (spob->dominated == 0) {
        if (spob->govt == -1 || !PlayerBlessedByGovt(spob->govt, 1)) {
            minStatus = spob->minStatus;
            if (minStatus == 0x7fff ||
                (playerLegalStatus[spob->system] < minStatus && minStatus > -0x7fff)) {
                if (playerLegalStatus[spob->system] < 0) {
                    color->red = 0xffff;
                    color->green = 0;
                    color->blue = 0;
                } else {
                    color->red = 0xffff;
                    color->green = 0x6666;
                    color->blue = 0;
                }
            } else {
                color->red = 0xffff;
                color->green = 0xffff;
                color->blue = 0;
            }
        } else {
            color->red = 0xffff;
            color->green = 0xffff;
            color->blue = 0;
        }
    } else {
        color->red = 0;
        color->green = 0xffff;
        color->blue = 0;
    }
}

/* ColorCodeSystem (mac _ColorCodeSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466260
void ColorCodeSystem(short sys, RGBColor *color)
{
    short first;
    short special;
    short warned;
    short hostile;
    short i;
    short friendly;
    short spob;
    short minStatus;

    first = FindFirstCoLocatedSystem(sys);
    sys = FindActiveCoLocatedSystem(sys);
    color->red = rgbDarkGrey.red;
    color->green = rgbDarkGrey.green;
    color->blue = rgbDarkGrey.blue;
    if (sys < 0 || sys >= 2048 || first < 0 || first >= 2048)
        return;
    if (gSystems[first].exploreLevel <= 0) {
        color->red = rgbDarkGrey.red;
        color->green = rgbDarkGrey.green;
        color->blue = rgbDarkGrey.blue;
    } else if (SystemIsInhabited(sys)) {
        friendly = 0;
        special = 0;
        warned = 0;
        hostile = 0;
        for (i = 0; i < kNavSlots; i++) {
            spob = gSystems[sys].navDefault[i];
            if (spob != -1 && stellarData[spob].loaded != 0 &&
                !SpobIsHyperOrWorm(&stellarData[spob]) &&
                !SpobUninhabited(&stellarData[spob]) &&
                StellarIsLandable(&stellarData[spob])) {
                if (stellarData[spob].dominated != 0) {
                    special++;
                } else if (stellarData[spob].govt != -1 &&
                           PlayerBlessedByGovt(stellarData[spob].govt, 1)) {
                    friendly++;
                } else {
                    minStatus = stellarData[spob].minStatus;
                    if (minStatus == 0x7fff ||
                        (playerLegalStatus[first] < minStatus && minStatus > -0x7fff)) {
                        if (playerLegalStatus[first] < 0)
                            hostile++;
                        else
                            warned++;
                    } else {
                        friendly++;
                    }
                }
            }
        }
        if (special > 0) {
            color->red = brightGreen.red;
            color->green = brightGreen.green;
            color->blue = brightGreen.blue;
        } else if (friendly > 0) {
            color->red = 0;
            color->green = 0;
            color->blue = 0xffff;
        } else if (warned > 0) {
            color->red = 0xffff;
            color->green = 0x6666;
            color->blue = 0;
        } else if (hostile > 0) {
            color->red = 0xffff;
            color->green = 0;
            color->blue = 0;
        } else {
            color->red = rgbLightGrey.red;
            color->green = rgbLightGrey.green;
            color->blue = rgbLightGrey.blue;
        }
    } else {
        color->red = rgbLightGrey.red;
        color->green = rgbLightGrey.green;
        color->blue = rgbLightGrey.blue;
    }
}

/* fills `frame` with the picture's bounds (name invented) */

/* CenterPict. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466540
void CenterPict(void *pict, Rect *bounds)
{
    short top;
    short left;
    short bottom;
    short right;
    Rect frame;

    if (pict != 0) {
        top = bounds->top;
        left = bounds->left;
        bottom = bounds->bottom;
        right = bounds->right;
        QDGetPictureBounds((PicHandle)pict, &frame);
        bounds->top = (((bottom - top) - (frame.bottom - frame.top)) / 2 + top);
        bounds->bottom = (frame.bottom - frame.top) + bounds->top;
        bounds->left = (((right - left) - (frame.right - frame.left)) / 2 + left);
        bounds->right = (frame.right - frame.left) + bounds->left;
    }
}

/* MSL <ctype.h> inlines tolower's table lookup; the exe calls the library
   function (0x004d6230), so declare it without the header. */

/* CullNameString (mac _CullNameString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004665f0
void CullNameString(unsigned char *name)
{
    int t;
    short skip;
    short len;
    short i;
    unsigned char culled[256];
    unsigned char work[256];

    pstrcpy(work, name);
    PtoCstr(work);
    skip = 0;
    len = strlen((char *)work);
    if (tolower(work[0]) == 't' && tolower(work[1]) == 'h'
            && tolower(work[2]) == 'e') {
        t = work[3];
        if (t == ' ') {
            skip += 4;
            len -= 4;
        }
    }
    for (i = 0; i < len; i++)
        culled[i] = work[i + skip];
    culled[i] = 0;
    CtoPstr((char *)culled);
    pstrcpy(name, culled);
}

/* FirstKeyDown. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466810
long FirstKeyDown(void)
{
    long code;

    KeyboardKbhit();
    code = 0;
    do {
        switch (code) {
        case 2: case 3: case 4: case 5:
        case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        case 0x70: case 0x71:
            break;
        default:
            if (KeyIsDown(code) != 0)
                return code;
            break;
        }
        code = code + 1;
    } while (code <= 0x7f);
    return -1;
}

/* KeyPrefsToSlotTable. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466850
void KeyPrefsToSlotTable(void)
{
    gSlotTable[0] = keyPrefs.accelerate;
    gSlotTable[1] = keyPrefs.reverse;
    gSlotTable[2] = keyPrefs.turnRight;
    gSlotTable[3] = keyPrefs.turnLeft;
    gSlotTable[4] = keyPrefs.afterburner;
    gSlotTable[5] = keyPrefs.faceTarget;
    gSlotTable[6] = keyPrefs.hyperspace;
    gSlotTable[7] = keyPrefs.nextSystem;
    gSlotTable[8] = keyPrefs.jump;
    gSlotTable[9] = keyPrefs.clearTarget;
    gSlotTable[10] = keyPrefs.hail;
    gSlotTable[11] = keyPrefs.land;
    gSlotTable[12] = keyPrefs.firePrimary;
    gSlotTable[13] = keyPrefs.fireSecondary;
    gSlotTable[14] = keyPrefs.selectSecondary;
    gSlotTable[15] = keyPrefs.deselectSecondary;
    gSlotTable[16] = keyPrefs.targetNext;
    gSlotTable[17] = keyPrefs.targetNearest;
    gSlotTable[18] = keyPrefs.escortMenu;
    gSlotTable[19] = keyPrefs.escortOrder[0];
    gSlotTable[20] = keyPrefs.escortOrder[1];
    gSlotTable[21] = keyPrefs.escortOrder[2];
    gSlotTable[22] = keyPrefs.escortOrder[3];
    gSlotTable[23] = keyPrefs.pause;
    gSlotTable[24] = keyPrefs.dismiss;
    gSlotTable[25] = keyPrefs.board;
    gSlotTable[26] = keyPrefs.jettison;
    gSlotTable[27] = keyPrefs.eject;
    gSlotTable[28] = keyPrefs.selfDestruct;
    gSlotTable[29] = keyPrefs.cloak;
    gSlotTable[30] = keyPrefs.map;
    gSlotTable[31] = keyPrefs.playerInfo;
    gSlotTable[32] = keyPrefs.missions;
    gSlotTable[33] = keyPrefs.toggleFPS;
}

/* SlotTableToKeyPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466a30
void SlotTableToKeyPrefs(void)
{
    keyPrefs.accelerate = gSlotTable[0];
    keyPrefs.reverse = gSlotTable[1];
    keyPrefs.turnRight = gSlotTable[2];
    keyPrefs.turnLeft = gSlotTable[3];
    keyPrefs.afterburner = gSlotTable[4];
    keyPrefs.faceTarget = gSlotTable[5];
    keyPrefs.hyperspace = gSlotTable[6];
    keyPrefs.nextSystem = gSlotTable[7];
    keyPrefs.jump = gSlotTable[8];
    keyPrefs.clearTarget = gSlotTable[9];
    keyPrefs.hail = gSlotTable[10];
    keyPrefs.land = gSlotTable[11];
    keyPrefs.firePrimary = gSlotTable[12];
    keyPrefs.fireSecondary = gSlotTable[13];
    keyPrefs.selectSecondary = gSlotTable[14];
    keyPrefs.deselectSecondary = gSlotTable[15];
    keyPrefs.targetNext = gSlotTable[16];
    keyPrefs.targetNearest = gSlotTable[17];
    keyPrefs.escortMenu = gSlotTable[18];
    keyPrefs.escortOrder[0] = gSlotTable[19];
    keyPrefs.escortOrder[1] = gSlotTable[20];
    keyPrefs.escortOrder[2] = gSlotTable[21];
    keyPrefs.escortOrder[3] = gSlotTable[22];
    keyPrefs.pause = gSlotTable[23];
    keyPrefs.dismiss = gSlotTable[24];
    keyPrefs.board = gSlotTable[25];
    keyPrefs.jettison = gSlotTable[26];
    keyPrefs.eject = gSlotTable[27];
    keyPrefs.selfDestruct = gSlotTable[28];
    keyPrefs.cloak = gSlotTable[29];
    keyPrefs.map = gSlotTable[30];
    keyPrefs.playerInfo = gSlotTable[31];
    keyPrefs.missions = gSlotTable[32];
    keyPrefs.toggleFPS = gSlotTable[33];
}

/* IsKeySlotInUse. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466c10
char IsKeySlotInUse(short value, short skip)
{
    short i;

    for (i = 0; i < 0x22; i++) {
        if (value == gSlotTable[i] && i != skip)
            return 1;
    }
    return 0;
}

/* AdvanceDate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466c40
void AdvanceDate(DateTimeRec *date, long daysToAdd)
{
    short days;

    switch (date->month) {
    case 4:
    case 6:
    case 9:
    case 11:
        days = 30;
        break;
    case 2:
        if (date->year % 4 == 0)
            days = 29;
        else
            days = 28;
        break;
    default:
        days = 31;
        break;
    }
    date->day++;
    if (date->day > days) {
        date->day = 1;
        date->month++;
        if (date->month > 12) {
            date->month = 1;
            date->year++;
        }
    }
}

#define kDefWaveEncoded   0x3e8
#define kDefGrowOdds      0x1c2

/* IncrementGameTime (mac _IncrementGameTime). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466cb0
void IncrementGameTime(void)
{
    short i;
    short j;
    short wave;
    AdvanceDate(&gameTime, 0);
    CronEventHandler();

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissions[i].timeLimit > 0)
            currentMissions[i].timeLimit--;
    }

    DoStellarTimePassage();
    DisasterHandler();

    for (j = 0; j < kMaxStellars; j++) {
        if (stellarData[j].loaded != 0) {
            stellarData[j].reloadTimer = 0;
            stellarData[j].burstShots = 0;
            if (stellarData[j].dominated != 0) {
                wave = stellarData[j].defCount;
                if (wave > kDefWaveEncoded)
                    wave = wave % kDefWaveEncoded;
                if (stellarData[j].defWaveSize < wave && Rand(kDefGrowOdds) == 0)
                    stellarData[j].defWaveSize++;
            }
            if (StellarIsDestroyed(&stellarData[j])) {
                if (stellarData[j].deadDays >= 0) {
                    stellarData[j].deadDaysLeft--;
                    if (stellarData[j].deadDaysLeft <= 0) {
                        stellarData[j].deadDaysLeft = -1;
                        stellarData[j].deadCounter = stellarData[j].strength;
                        crashlog_mission_bit_set("regenerating stellar",
                                                 (char *)stellarData[j].name, 1,
                                                 j + 128, stellarData[j].onRegen);
                        EvalMissionBitSetString(stellarData[j].onRegen);
                    }
                } else {
                    stellarData[j].deadDaysLeft = 1;
                }
            } else {
                stellarData[j].deadCounter = stellarData[j].strength;
                stellarData[j].deadDaysLeft = -1;
            }
        }
    }

    for (i = 0; i < kMaxStellars; i++) {
        if (gSystems[i].loaded != 0 && gSystems[i].reinforceDaysLeft > 0)
            gSystems[i].reinforceDaysLeft--;
    }

    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 && rankData[i].salary != 0) {
            if (shipData->credits < rankData[i].salaryCap || rankData[i].salaryCap <= 0)
                shipData->credits += rankData[i].salary;
            if (shipData->credits < 0)
                shipData->credits = 0;
        }
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        gShipTypes[i].dailyHireRoll = Rand(kDailyRollRange) + 1;
        gShipTypes[i].dailySaleRoll = Rand(kDailyRollRange) + 1;
    }

    for (i = 0; i < kMaxOutfits; i++)
        gOutfits[i].dailyRoll = Rand(kDailyRollRange) + 1;
}

#define kRankAlwaysLoses            0x40  /* name TBC */
#define kRankLosesOnPenaltyType     0x04  /* name TBC */

/* SlapWithPenalty (mac _SlapWithPenalty). STATE: MATCH 100.0%, exe 112 / obj 112
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00466fc0
void SlapWithPenalty(short sys, short govt, short type, short mode)
{
    short i;

    if (govt != -1) {
        if ((govtData[govt].flags & kGovtAlwaysDisabled) != 0)
            return;
    }
    for (i = 0; i < kMaxSystems; i++)
        systemVisited[i] = 0;

    if (mode == -1) {
        RecursivePenaltySlap(sys, govt, type, 1.0);

        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0
                    && (rankData[i].flags & kRankNeverDropped) == 0
                    && GovtAllies(govt, rankData[i].govt) != 0) {
                if ((rankData[i].flags & kRankAlwaysLoses) != 0)
                    DeactivateRank((char *)&rankData[i]);
                else if ((rankData[i].flags & kRankLosesOnPenaltyType) != 0
                        && (type == 1 || type == 3))
                    DeactivateRank((char *)&rankData[i]);
            }
        }
    }
}

#define kGovtFlag1  0x0001   /* govtData[].flags bit, name TBC */


/* RecursivePenaltySlap (mac _RecursivePenaltySlap). STATE: MATCH 100.0%, exe 387 / obj 387
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00467140
void RecursivePenaltySlap(short sys, short govt, short type, double amount)
{
    short startSys, curSys, i;
    short sysFlag1, sysFlag2;
    short govtFlag1;
    char dirty;
    float penalty;

    if (sys < 0 || sys >= kMaxSystems)
        return;
    if (systemVisited[sys])
        return;
    systemVisited[sys] = 1;

    if (govt != -1)
        govtFlag1 = govtData[govt].flags & kGovtFlag1;
    else
        govtFlag1 = 0;

    startSys = sys;
    dirty = 0;
    if (gSystems[sys].colocGroup != -1)
        sys = gSystems[sys].colocGroup;
    if (sys != -1) {
        curSys = sys;
        for (;;) {
            systemVisited[curSys] = 1;
            if (gSystems[curSys].loaded) {
                penalty = 0;
                sysFlag1 = 0;
                sysFlag2 = 0;
                if (gSystems[curSys].govt != -1) {
                    sysFlag1 = govtData[gSystems[curSys].govt].flags & kGovtFlag1;
                    sysFlag2 = govtData[gSystems[curSys].govt].flags & kGovtFlag2;
                }

                if (gSystems[curSys].govt == -1) {
                    if (govt == -1) {
                        penalty = govtData[0].penalties[type] * 0.5;
                    } else if (govtFlag1) {
                        penalty = -govtData[govt].penalties[type] * 0.5;
                    } else {
                        penalty = govtData[govt].penalties[type] * 0.25;
                    }
                } else if (govt == gSystems[curSys].govt) {
                    penalty = govtData[govt].penalties[type];
                } else if (govt == -1) {
                    if (sysFlag1)
                        penalty = -govtData[gSystems[curSys].govt].penalties[type] * 0.5;
                    else if (sysFlag2)
                        penalty = govtData[gSystems[curSys].govt].penalties[type] * 0.5;
                } else if (!GovtAllies(govt, gSystems[curSys].govt)) {
                    penalty = -govtData[gSystems[curSys].govt].penalties[type] * 0.5;
                } else if (!GovtEnemies(govt, gSystems[curSys].govt)) {
                    if (GovtAllies(govt, gSystems[curSys].govt))
                        penalty = govtData[govt].penalties[type];
                    else
                        penalty = govtData[govt].penalties[type] * 0.5;
                } else {
                    penalty = -govtData[gSystems[curSys].govt].penalties[type] * 0.5;
                }

                penalty *= amount;
                if (penalty >= 1.0 || penalty <= -1.0) {
                    dirty = 1;
                    if (penalty > 0.0) {
                        if (playerLegalStatus[curSys] - penalty < -32000.0f)
                            playerLegalStatus[curSys] = -32000;
                        else
                            playerLegalStatus[curSys] -= penalty;
                    } else {
                        if (playerLegalStatus[curSys] - penalty > 32000.0f)
                            playerLegalStatus[curSys] = 32000;
                        else
                            playerLegalStatus[curSys] -= penalty;
                    }
                    if (playerLegalStatus[curSys] > 32000)
                        playerLegalStatus[curSys] = 32000;
                    if (playerLegalStatus[curSys] < -32000)
                        playerLegalStatus[curSys] = -32000;
                }
            }
            curSys = gSystems[curSys].colocNext;
            if (curSys == -1)
                break;
        }
    }

    if (dirty) {
        amount *= 0.65;
        for (i = 0; i < kMaxSystemLinks; i++) {
            if (gSystems[startSys].links[i] != -1)
                RecursivePenaltySlap(FindFirstCoLocatedSystem(gSystems[startSys].links[i]),
                                     govt, type, amount);
        }
    }
}

#define kFadeMillis 500
#define kMessageStrings 1000    /* STR# holding the numbered player messages */


}

/* scratch recursion marks, one per system (name invented) */
char systemVisited[kMaxSystems];
unsigned char DAT_007ccd90[2];
short exploreDepthLimit;
unsigned char DAT_007ccd94[68];
unsigned long gRandSeed;

extern "C" {

/* screen fade state (names invented; the mac fades via CGDisplayFade instead) */
long screenGamma = 0x00010000;           /* 0x00575878 — 16.16, 1.0 = full brightness */

/*
 * The windows port drives the palette from the DirectDraw side, so the mac
 * gamma write is compiled out here. The ramp is still walked at the original
 * rate, which is what paces the fade.
 */
#define SetScreenGamma(level)  ((void)(level))

/* EscapeStellarSearch. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467710
short EscapeStellarSearch(short sys)
{
    short i;

    for (i = 0; i < kMaxSystems; i++)
        systemVisited[i] = 0;
    systemVisited[sys] = 1;
    return RecursiveEscapeStellarSearch(sys);
}

/* RecursiveEscapeStellarSearch. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004677a0
short RecursiveEscapeStellarSearch(short sys)
{
    short i;
    short j;
    short link;
    short link2;
    short stellar;
    short minStatus;

    systemVisited[sys] = 1;

    for (i = 0; i < kNavSlots; i++) {
        link = gSystems[sys].links[i];
        if (link == -1)
            continue;
        if (gSystems[link].loaded == 0)
            continue;
        if (gSystems[link].known == 0)
            continue;
        if (gSystems[link].exploreLevel <= 0)
            continue;
        if (systemVisited[link] != 0)
            continue;
        for (j = 0; j < kNavSlots; j++) {
            stellar = gSystems[link].navDefault[j];
            if (stellar == -1)
                continue;
            if (stellarData[stellar].loaded == 0)
                continue;
            if (!StellarIsNormalLandable(&stellarData[stellar]))
                continue;
            if ((stellarData[stellar].flags & kSpobHasShipyard) == 0)
                continue;
            minStatus = stellarData[stellar].minStatus;
            if (minStatus != 0x7fff &&
                    (playerLegalStatus[link] >= minStatus || minStatus == -0x7fff))
                return stellar;
        }
    }

    for (i = 0; i < kNavSlots; i++) {
        link2 = gSystems[sys].links[i];
        if (link2 == -1)
            continue;
        if (gSystems[link2].loaded == 0)
            continue;
        if (gSystems[link2].known == 0)
            continue;
        if (gSystems[link2].exploreLevel <= 0)
            continue;
        if (systemVisited[link2] != 0)
            continue;
        stellar = RecursiveEscapeStellarSearch(link2);
        if (stellar != -1)
            return stellar;
    }
    return -1;
}

/* AutoSetExploration (mac _AutoSetExploration). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467970
void AutoSetExploration(short sys, short maxDepth, short level)
{
    short i;
    short j;
    short link;

    for (i = 0; i < kMaxSystems; i++)
        systemVisited[i] = 0;

    exploreDepthLimit = maxDepth;
    RecursiveAutoExplore(sys, 0, level);

    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].loaded != 0 && gSystems[i].prepared != 0 &&
            gSystems[i].exploreLevel > 0) {
            gSystems[i].known = 1;
            for (j = 0; j < kNavSlots; j++) {
                link = gSystems[i].links[j];
                if (link != -1) {
                    link = FindActiveCoLocatedSystem(link);
                    if (link != -1)
                        gSystems[link].known = 1;
                }
            }
        }
    }
}

/* RecursiveAutoExplore (mac _RecursiveAutoExplore). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467ab0
void RecursiveAutoExplore(short sys, short depth, short level)
{
    short i;
    short coloc;
    short link;

    if (depth > exploreDepthLimit)
        return;
    if (systemVisited[sys] != 0)
        return;
    systemVisited[sys] = 1;

    ExploreNebulaeFromSystem(sys);

    if (gSystems[sys].exploreLevel < level)
        gSystems[sys].exploreLevel = level;

    coloc = FindFirstCoLocatedSystem(sys);
    if (coloc != sys && coloc >= 0 && coloc < kMaxSystems) {
        if (gSystems[coloc].exploreLevel < level)
            gSystems[coloc].exploreLevel = level;
    }

    for (i = 0; i < kNavSlots; i++) {
        link = gSystems[sys].links[i];
        if (link != -1) {
            link = FindActiveCoLocatedSystem(link);
            if (link != -1)
                RecursiveAutoExplore(link, depth + 1, level);
        }
    }
}

/* Any nebula whose (inset) map rectangle contains this system counts as visited. */
/* ExploreNebulaeFromSystem (mac _ExploreNebulaeFromSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467bd0
void ExploreNebulaeFromSystem(short sys)
{
    Rect box;
    short top;
    short i;

    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].loaded != 0 && nebulaData[i].explored == 0) {
            top = nebulaData[i].yPos;
            SetRect(&box, nebulaData[i].xPos, top,
                    nebulaData[i].xPos + nebulaData[i].width,
                    top + nebulaData[i].height);
            InsetRect(&box, 8, 8);
            if (PtInRect(gSystems[sys].xPos, gSystems[sys].yPos, &box) != 0) {
                nebulaData[i].explored = 1;
                crashlog_mission_bit_set("exploring nebula", 0, i + 128, -1,
                                         nebulaData[i].onExplore);
                EvalMissionBitSetString(nebulaData[i].onExplore);
            }
        }
    }
}

/* Show numbered message `index`, letting a plug-in override the built-in text. */
/* DisplayMessage (mac _DisplayMessage). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467cf0
void DisplayMessage(short index)
{
    Str255 text;

    if (LoadPluginString(index + kMessageStrings - 1, text) == 0)
        GetIndString(text, kMessageStrings, index);
    DisplayComm(text, 480, 0, 12, &rgbWhite, 0, 0);
}

/* Walk the gamma ramp to `percent` brightness over `duration` milliseconds. */
/* FadeGammaTo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467d60
void FadeGammaTo(unsigned long percent, unsigned long duration)
{
    unsigned long target;
    unsigned long start;
    unsigned long elapsed;
    long delta;

    target = (percent << 16) / 100;
    delta = target - screenGamma;
    if (duration != 0) {
        start = ReadTickMilliseconds();
        do {
            elapsed = ReadTickMilliseconds() - start;
            if (elapsed > duration)
                elapsed = duration;
            SetScreenGamma(FixedMultiply((elapsed << 16) / duration, delta));
        } while (elapsed < duration);
    }
    screenGamma = target;
}

/* FadeScreenOut (mac _FadeScreenOut). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467de0
void FadeScreenOut(void)
{
    if (screenFaded != 0 || inBackground != 0)
        return;
    ScreenEndScene();
    if (windowedPref == 0)
        FadeGammaTo(0, kFadeMillis);
    screenFaded = 1;
}

/* FadeScreenIn (mac _FadeScreenIn). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467e20
void FadeScreenIn(void)
{
    if (screenFaded == 0)
        return;
    ScreenEndScene();
    if (windowedPref == 0)
        FadeGammaTo(100, kFadeMillis);
    screenFaded = 0;
}

/* OnTheFlyWhiteFade (mac _OnTheFlyWhiteFade). STATE: MATCH, byte-identical.
   OPEN: none. `pct' is pushed by the caller and never read here. */
// FUNCTION: EVNOVA 0x00467e60
void OnTheFlyWhiteFade(short pct)
{
}

/* RestoreOrigClut (mac _RestoreOrigClut). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467e70
void RestoreOrigClut(void)
{
}

#define kShipFlags2Bit10 0x400

/* ShipCanCloak (no mac symbol). STATE: MATCH 100.0%, exe 174 / obj 174 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00467e80
char ShipCanCloak(ShipObj *ship)
{
    short idx;
    char found;
    short i;
    short j;
    char nibbleLo;
    char nibbleHi;

    if (IsDisabled(ship))
        return 0;

    found = 0;
    idx = ship->index;
    if (idx == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType17) {
                        found = 1;
                        /* BUG: both branches read modType[j] for modVal[j], so the nibbles are always 1 and 0 */
                        nibbleLo = (gOutfits[i].modType[j] >> 4) & 0xf;
                        nibbleHi = (gOutfits[i].modType[j] >> 8) & 0xf;
                        break;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17) {
                        found = 1;
                        nibbleLo = (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] >> 4) & 0xf;
                        nibbleHi = (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] >> 8) & 0xf;
                        break;
                    }
                }
            }
        }
    }

    if (found) {
        if (nibbleLo != 0) {
            if (ship->fuel <= 0.0f)
                return 0;
        }
        if (nibbleHi != 0) {
            if (ship->shield <= 0.0f)
                return 0;
        }

        if (idx != 0 || !(ship->jumpTimer > 0.0f)
            || (gShipTypes[ship->type].flags2 & kShipFlags2Bit10) != 0)
            return 1;
        return 0;
    }
    return 0;
}

/* DoShipCloak. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004680d0
void DoShipCloak(ShipObj *ship)
{
    if (!ShipIsCloaked(ship)) {
        if (ship->index == 0 && ship->cloakDir <= 0)
            PlaySound(cloakSnd[1], 8, gSoundVolume, gSoundVolume);
        if (ship->cloak <= 0.0f) {
            ship->cloakDir = 1;
        } else {
            if (ship->cloakDir < 0)
                ship->cloakDir = 0;
        }
        if (ship->shield > 0.0f && CloakZeroesShields(ship)) {
            ship->shield = 0;
            if (ship->index == 0)
                statusRedraw[1] = 1;
        }
    }
}

/* DoShipUncloak. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468190
void DoShipUncloak(ShipObj *ship)
{
    if (ShipIsCloaked(ship)) {
        if (ship->index == 0 && ship->cloakDir >= 0)
            PlaySound(cloakSnd[0], 8, gSoundVolume, gSoundVolume);
        if (!(ship->cloak > 0.0f)) {
            if (ship->cloakDir > 0)
                ship->cloakDir = 0;
        } else {
            ship->cloakDir = -1;
        }
    }
}

#define kSpobService4    0x00000004UL  /* name TBC */
#define kSpobService8    0x00000008UL  /* name TBC */

/* SystemTradesGoods. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468210
char SystemTradesGoods(short sysID, short commodity)
{
    short i;

    for (i = 0; i < kNavSlots; i++) {
        if (gSystems[sysID].navDefault[i] != -1
                && stellarData[gSystems[sysID].navDefault[i]].loaded != 0
                && (stellarData[gSystems[sysID].navDefault[i]].flags
                    & kSpobUninhabited) == 0
                && StellarIsNormalLandable(
                       &stellarData[gSystems[sysID].navDefault[i]])
                && (stellarData[gSystems[sysID].navDefault[i]].flags
                    & kSpobTradeCenter) != 0
                && CalcPortDemand(commodity,
                       stellarData[gSystems[sysID].navDefault[i]].flags) != 0)
            return 1;
    }
    return 0;
}

/* the same scan asking for one of three services by index; a `kind` outside
   0..2 leaves the mask unset, which the original does not guard */
/* SystemHasService. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004682d0
char SystemHasService(short sysID, short kind)
{
    short i;
    unsigned long mask;

    for (i = 0; i < kNavSlots; i++) {
        if (gSystems[sysID].navDefault[i] != -1
                && stellarData[gSystems[sysID].navDefault[i]].loaded != 0
                && (stellarData[gSystems[sysID].navDefault[i]].flags
                    & kSpobUninhabited) == 0
                && StellarIsNormalLandable(
                       &stellarData[gSystems[sysID].navDefault[i]])) {
            if (kind == 0)
                mask = kSpobTradeCenter;
            if (kind == 1)
                mask = kSpobService4;
            if (kind == 2)
                mask = kSpobService8;
            if ((stellarData[gSystems[sysID].navDefault[i]].flags & mask) != 0)
                return 1;
        }
    }
    return 0;
}

#define kRandMultiplier 16807

/* The 68k `swap` idiom: the >> 16 half is always zero for a value already
   masked to 15 bits and mwcc folds it away, but the PPC build emits it
   literally and mac 1.1.1 emits a `ror`, so it is in the source. */
#define SwapHalves(x) (((x) << 16) + ((x) >> 16))

/* Rand (mac _Rand). STATE: MATCH 100.0%, 45/45, byte-identical pending the next regate.
   OPEN: none. Park-Miller 16807 in 16-bit halves, transliterated register by register
   from the 68k Random(). */
// FUNCTION: EVNOVA 0x004683b0
short Rand(short range)
{
    unsigned long temp;
    unsigned long lo;
    unsigned long hi;
    unsigned long seed;
    unsigned long result;

    if (range == 0) {
        /* BUG: this path never sets result, so Rand(0) returns garbage */
        GetDateTime(&gRandSeed);
        gRandSeed += ReadTick();
    } else {
        seed = gRandSeed;
        lo = kRandMultiplier;
        hi = lo;
        lo *= seed & 0xffff;
        temp = lo;
        temp >>= 16;
        hi *= seed >> 16;
        hi += temp;
        temp = hi;
        temp += temp;
        temp >>= 16;
        lo &= 0xffff;
        lo -= 0x7fffffff;
        hi &= 0x7fff;
        hi = SwapHalves(hi);
        hi += temp;
        lo += hi;
        if ((lo & 0x80000000) != 0)
            lo += 0x7fffffff;
        gRandSeed = lo;
        if ((lo & 0xffff) == 0x8000)
            lo &= 0xffff0000;
        temp = range;
        temp *= lo & 0xffff;
        temp >>= 16;
        result = temp;
    }
    return result;
}

#define kDateStrings   137  /* STR# holding the month names and ordinals */
#define kFirstMonthStr  12  /* month 1 is index 13 */
#define kOrdinalSt      25
#define kOrdinalNd      26
#define kOrdinalRd      27
#define kOrdinalTh      28

/* DateToMedString (mac _DateToMedString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468450
void DateToMedString(DateTimeRec date, char *out)
{
    int day;
    int digit;
    Str255 str;

    strcpy(out, datePrefix);
    GetIndString(str, kDateStrings, date.month + kFirstMonthStr);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, " ");
    day = *(int *)&date.day;
    NumToString((short)day, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    GetIndString(str, kDateStrings, kOrdinalTh);
    digit = (short)day % 10;
    if (digit == 1)
        GetIndString(str, kDateStrings, kOrdinalSt);
    if (digit == 2)
        GetIndString(str, kDateStrings, kOrdinalNd);
    if (digit == 3)
        GetIndString(str, kDateStrings, kOrdinalRd);
    if ((short)day >= 11 && (short)day <= 13)
        GetIndString(str, kDateStrings, kOrdinalTh);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, ", ");
    NumToString(date.year, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, dateSuffix);
}

/* DateToLongString (mac _DateToLongString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468600
void DateToLongString(DateTimeRec date, char *out)
{
    int day;
    int digit;
    Str255 str;

    strcpy(out, datePrefix);
    GetIndString(str, kDateStrings, date.month);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, " ");
    day = *(int *)&date.day;
    NumToString((short)day, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    GetIndString(str, kDateStrings, kOrdinalTh);
    digit = (short)day % 10;
    if (digit == 1)
        GetIndString(str, kDateStrings, kOrdinalSt);
    if (digit == 2)
        GetIndString(str, kDateStrings, kOrdinalNd);
    if (digit == 3)
        GetIndString(str, kDateStrings, kOrdinalRd);
    if ((short)day >= 11 && (short)day <= 13)
        GetIndString(str, kDateStrings, kOrdinalTh);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, ", ");
    NumToString(date.year, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, dateSuffix);
}

/*
 * Disabled is three unrelated things at once: the government marks all its
 * ships that way, a mission of kind 5 holds this ship disabled, or the armour
 * has fallen below the threshold - a tenth for ship types carrying flag 0x10,
 * otherwise a third.
 */

/* IsDisabled (mac _IsDisabled). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004687b0
char IsDisabled(ShipObj *ship)
{
    short govt;
    short mission;

    govt = ship->govt;
    if (govt != -1 && (govtData[govt].flags & kGovtAlwaysDisabled) != 0)
        return 1;
    if (ship->index > 0) {
        mission = ship->specialMission;
        if (mission != -1 && currentMissionObjectives[mission].active != '\0'
                && currentMissions[mission].shipGoal == 5
                && currentMissionObjectives[mission].objectivesDone == '\0'
                && ship->boarded == '\0')
            return 1;
    }
    if (ship->index > 0 && ship->defenseStellar != -1)
        return 0;
    if ((gShipTypes[ship->type].flags & kShipArmorDisableTenth) != 0) {
        if (ship->armor * 100.0f < ShipArmorCapacity(ship) * 10.0)
            return 1;
    } else {
        if (ship->armor * 100.0f < ShipArmorCapacity(ship) * 33.333)
            return 1;
    }
    return 0;
}

/* IsDying (mac _IsDying). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004688e0
unsigned char IsDying(ShipObj *ship)
{
    if (ship->deathTimer > 0.0f)
        return 1;
    if (ship->armor <= 0.0f)
        return 1;
    return 0;
}

#define kMaxHiredEscorts 6

/* CanHireEscorts (mac _CanHireEscorts). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468920
char CanHireEscorts(void)
{
    short i;
    short escorts;

    escorts = 0;
    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0 &&
            shipData[i].aiType == 6 && shipData[i].specialMission == -1)
            escorts++;
    }
    if (escorts < kMaxHiredEscorts)
        return 1;
    return 0;
}

/* WeaponHasAmmo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468990
char WeaponHasAmmo(ShipObj *ship, short weaponIndex)
{
    short ammoType;

    if (ship->index != 0
            && (gWeapons[weaponIndex].flags2 & kWeapFlagAIRestricted) != 0)
        return 0;
    if (ShipIsCloaked(ship) && (gWeapons[weaponIndex].flags2 & kWeapFireWhileCloaked) == 0)
        return 0;
    if ((gWeapons[weaponIndex].flags2 & 0x80) != 0 && !KeyCarriedShipTypeOnboard(ship))
        return 0;
    if (gWeapons[weaponIndex].guidance == kGuidanceBay) {
        if (ship->weap[weaponIndex].ammo > 0)
            goto success;
        return 0;
    }
    ammoType = gWeapons[weaponIndex].ammoType;
    if (ammoType >= 0 && ammoType < 256) {
        if (ship->index == 0) {
            if (ship->weap[ammoType].ammo > 0)
                goto success;
            return 0;
        }
        if (ship->weap[weaponIndex].ammo > 0)
            goto success;
        return 0;
    }
    if (ammoType > -kWeapAmmoFuelBase)
        goto success;
    if (ship->fuel < (abs(ammoType) - kWeapAmmoFuelBase) * 0.1)
        return 0;
success:
    return 1;
}

/* inhabited = at least one stellar that is populated and not a hypergate or wormhole */

/* SystemIsInhabited (mac _SystemIsInhabited). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468af0
char SystemIsInhabited(short sys)
{
    short i;
    short spob;

    for (i = 0; i <= 3; i++) {
        spob = gSystems[sys].navDefault[i];
        if (spob != -1) {
            if ((stellarData[spob].flags & kSpobUninhabited) == 0) {
                if (!SpobIsHyperOrWorm(&stellarData[spob]))
                    return 1;
            }
        }
    }
    return 0;
}

/* StellarsAreDistant. STATE: MATCH 100%, exe 191 / obj 191; relocregcheck 0 of 12 differ;
   byte identity awaits the next regate (bytediff reads the baseline).
   OPEN: none. The nav scan counts on the link loop's `i', and `link' was invented. */
// FUNCTION: EVNOVA 0x00468b50
char StellarsAreDistant(short a, short b)
{
    short i;
    SpobRecord *pa;
    short sysA;
    short sysB;
    SpobRecord *pb;
    short curSys;
    short found;
    short count;

    if (a < 0 || a >= kMaxStellars)
        return 1;
    pa = &stellarData[a];
    if (!pa->prepared)
        return 0;

    if (b >= 0 && b < kMaxStellars) {
        pb = &stellarData[b];
        if (pa->system < 0 || pa->system >= kMaxSystems ||
                pb->system < 0 || pb->system >= kMaxSystems)
            return 0;
        if (pa->system == pb->system)
            return 0;

        sysA = FindFirstCoLocatedSystem(pa->system);
        sysB = FindFirstCoLocatedSystem(pb->system);
        if (sysA == sysB)
            return 0;

        for (i = 0; i < kNavSlots; i++) {
            if (gSystems[pa->system].links[i] != -1 &&
                    FindFirstCoLocatedSystem(gSystems[pa->system].links[i]) == sysB)
                return 0;
            if (gSystems[pb->system].links[i] != -1 &&
                    FindFirstCoLocatedSystem(gSystems[pb->system].links[i]) == sysA)
                return 0;
        }
    } else {
        if (pa->system < 0 || pa->system >= kMaxSystems)
            return 0;
        sysA = FindFirstCoLocatedSystem(pa->system);
    }

    count = 0;
    found = 0;
    curSys = FindFirstCoLocatedSystem(sysA);
    do {
        count++;
        for (i = 0; i < kNavSlots; i++) {
            if (gSystems[curSys].navDefault[i] == pa->index) {
                found++;
                break;
            }
        }
        curSys = gSystems[curSys].colocNext;
    } while (curSys != -1);

    if (found != count)
        return 0;
    return 1;
}

/* DrawLegalStatusString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468d90
void DrawLegalStatusString(short sys)
{
    long base;
    long status;
    short haven;
    short clean;
    short j;
    short rating;

    rating = 0;
    status = playerLegalStatus[sys];
    if (gSystems[sys].govt >= 0 && gSystems[sys].govt < kMaxGovts)
        base = govtData[gSystems[sys].govt].crimeTol;
    else
        base = govtData[0].crimeTol;

    if (status < 0)
        rating = 2;
    if (status < -base)
        rating = 3;
    if (status < -base * 4)
        rating = 4;
    if (status < -base * 16)
        rating = 5;
    if (status < -base * 64)
        rating = 6;
    if (status < -base * 256)
        rating = 7;
    if (status < -base * 1024)
        rating = 8;
    if (status < -base * 4096)
        rating = 9;
    if (status == 0)
        rating = 1;
    if (status > 0)
        rating = 0xa;
    if (status > base * 4)
        rating = 0xb;
    if (status > base * 16)
        rating = 0xc;
    if (status > base * 64)
        rating = 0xd;
    if (status > base * 256)
        rating = 0xe;
    if (status > base * 1024)
        rating = 0xf;

    haven = clean = 0;
    for (j = 0; j < 3; j++) {
        if (gSystems[sys].navDefault[j] != -1 && stellarData[gSystems[sys].navDefault[j]].loaded &&
                !SpobUninhabited(&stellarData[gSystems[sys].navDefault[j]]) &&
                StellarIsNormalLandable(&stellarData[gSystems[sys].navDefault[j]])) {
            if (stellarData[gSystems[sys].navDefault[j]].dominated != 0)
                haven++;
            else
                clean++;
        }
    }
    if (haven > 0) {
        if (clean <= 0)
            rating = 0x11;
        else
            rating = 0x10;
    }

    if (gSystems[sys].govt >= 0 && (govtData[gSystems[sys].govt].flags & kGovtXenophobic) != 0)
        rating = 0;

    if (rating == 0)
        DrawIndString(kGameStrList, 0x18c);
    else
        DrawString(legalStrings[rating]);
}

/* DrawCombatRatingString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469030
void DrawCombatRatingString(void)
{
    short rating;

    rating = 0;
    if (playerKills > 0)
        rating = 1;
    if (playerKills >= 100)
        rating = 2;
    if (playerKills >= 200)
        rating = 3;
    if (playerKills >= 400)
        rating = 4;
    if (playerKills >= 800)
        rating = 5;
    if (playerKills >= 1600)
        rating = 6;
    if (playerKills >= 3200)
        rating = 7;
    if (playerKills >= 6400)
        rating = 8;
    if (playerKills >= 12800)
        rating = 9;
    if (playerKills >= 25600)
        rating = 10;
    DrawString(ratingStrings[rating]);
}

/* PlayerShipTradeInPrice. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469100
long PlayerShipTradeInPrice(void)
{
    short i;
    long value;
    long cost;

    value = (gShipTypes[shipData->type].cost * 0.25);
    for (i = 0; i < 512; i++) {
        if (shipSystems[i] > 0 && gOutfits[i].persistent == 0) {
            cost = AdjustedItemCost(&gOutfits[i], &gShipTypes[shipData->type]);
            value = value + cost * shipSystems[i] * 0.5;
        }
    }
    if (value < 0)
        value = 0;
    return value;
}

/* PlayerCanUseAmmoType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469230
char PlayerCanUseAmmoType(short weapIndex)
{
    short i;

    for (i = 0; i < 256; i++) {
        if (shipData->weap[i].count > 0) {
            long w = weapIndex;

            if (gWeapons[i].guidance == kGuidanceBay) {
                if (gWeapons[i].ammoType == gWeapons[w].ammoType)
                    return 1;
                return 0;
            }
            if (weapIndex == gWeapons[i].ammoType)
                return 1;
        }
    }
    return 0;
}

#define kPanCutoffSq   40000
#define kPanNear      722500
#define kPanHalfWidth    200

/* PlaySoundDistance. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004692e0
char PlaySoundDistance(long unused, void *sound, short flags, FloatPoint *pos,
                     FloatPoint *listener)
{
    int dy;
    int dist;
    int dx;
    short floorVol;
    short leftVol;
    short rightVol;

    leftVol = rightVol = gSoundVolume;
    if (gSoundVolume > 0) {
        dx = (pos->x - listener->x);
        dy = (pos->y - listener->y);
        dist = dx * dx + dy * dy;
        if (dist > 0 && dist > kPanCutoffSq) {
            if (dx > kPanHalfWidth) {
                leftVol = (gSoundVolume * kPanNear / dist);
                rightVol = (gSoundVolume * kPanCutoffSq / dist);
            } else if (dx < -kPanHalfWidth) {
                leftVol = (gSoundVolume * kPanCutoffSq / dist);
                rightVol = (gSoundVolume * kPanNear / dist);
            } else {
                rightVol = (gSoundVolume * kPanNear / dist);
                leftVol = rightVol;
            }
            floorVol = (gSoundVolume / 8);
            if (floorVol < 1)
                floorVol = 1;
            if (leftVol < floorVol)
                leftVol = floorVol;
            if (rightVol < floorVol)
                rightVol = floorVol;
            if (leftVol > gSoundVolume)
                leftVol = gSoundVolume;
            if (rightVol > gSoundVolume)
                rightVol = gSoundVolume;
        }
    }
    PlaySound(sound, flags, leftVol, rightVol);
    return 1;
}

/* CheckSpeechMgr (mac _CheckSpeechMgr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469480
void CheckSpeechMgr(void)
{
}

/* SpeakIt (mac _SpeakIt). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469490
void SpeakIt(char *msg, short flag)
{
}

#define kEscortFighter   5

/* CanBuyFighter (mac _CanBuyFighter). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004694a0
char CanBuyFighter(short shipType, short outfitIndex, short weapIndex)
{
    short i;
    short j;
    short k;
    short m;
    short cap;
    short limit;
    short count;

    if (outfitIndex == -1 && weapIndex == -1) {
        for (i = 0; i < 256; i++) {
            if (shipData->weap[i].count > 0 &&
                gWeapons[i].guidance == kGuidanceBay &&
                shipType == gWeapons[i].ammoType - 128) {
                weapIndex = i;
                break;
            }
        }
        if (weapIndex == -1) {
            for (j = 0; j < 256; j++) {
                if (shipData->weap[j].count > 0 &&
                    gWeapons[j].guidance == kGuidanceBay &&
                    gShipTypes[shipType].spriteIndex ==
                        gShipTypes[gWeapons[j].ammoType - 128].spriteIndex) {
                    weapIndex = j;
                    break;
                }
            }
        }
        if (weapIndex == -1)
            return 0;
        for (k = 0; k < 512; k++) {
            for (m = 0; m < kOutfMods; m++) {
                if (gOutfits[k].techLevel < kOutfTechAbsent &&
                    gOutfits[k].modType[m] == kOutfModAmmo &&
                    weapIndex == gOutfits[k].modVal[m]) {
                    outfitIndex = k;
                    break;
                }
            }
        }
        if (outfitIndex == -1)
            return 0;
        count = shipData->weap[weapIndex].ammo;
    } else {
        if (outfitIndex == -1 || weapIndex == -1)
            return 0;
        count = shipSystems[outfitIndex];
    }

    cap = gWeapons[weapIndex].maxAmmo;
    if (cap > 0)
        limit = cap * shipData->weap[weapIndex].count;
    else
        limit = gOutfits[outfitIndex].maxCount;

    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 &&
            shipType == shipData[i].type &&
            shipData[i].parentShip == 0 &&
            shipData[i].aiType == kEscortFighter &&
            shipData[i].specialMission == -1)
            count++;
    }
    if (count < limit)
        return 1;
    return 0;
}

#define kMaxHolds 32000

/* TotalFleetHolds (mac _TotalFleetHolds). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469760
short TotalFleetHolds(void)
{
    short i;
    long total;

    total = ShipTotalHolds();
    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 && !IsDying(&shipData[i])
                && shipData[i].parentShip == 0 && shipData[i].aiType == 6
                && shipData[i].specialMission == -1
                && gShipTypes[shipData[i].type].inherentAI <= 2)
            total += gShipTypes[shipData[i].type].cargo;
    }
    if (total > kMaxHolds)
        total = kMaxHolds;
    return total;
}

/* DestroyPartialFleetCargo (mac _DestroyPartialFleetCargo). STATE: MATCH 100.0%, exe 287 /
   obj 287, byte-identical yes (bytediff after the 2026-09-15 regate). OPEN: none.
   The dead cargo sum survives DSE because its one reader is itself a dead assignment; notes. */
// FUNCTION: EVNOVA 0x00469810
void DestroyPartialFleetCargo(short shipIndex)
{
    double newShipHolds;
    double fleetHolds;
    double totalCargo;
    double ratio;
    long rounded;
    short i;

    if (shipIndex == 0) {
        newShipHolds = ShipTotalHolds();
    } else {
        newShipHolds = gShipTypes[shipData[shipIndex].type].cargo;
    }
    fleetHolds = ShipTotalHolds();
    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 && (!IsDying(&shipData[i]) || i == shipIndex)
                && shipData->curSystem == shipData[i].curSystem
                && shipData[i].parentShip == 0 && shipData[i].aiType == 6
                && shipData[i].specialMission == -1
                && gShipTypes[shipData[i].type].inherentAI <= 2)
            fleetHolds = gShipTypes[shipData[i].type].cargo + fleetHolds;
    }
    totalCargo = 0.0;
    for (i = 0; i < 6; i++)
        totalCargo += shipData->cargo[i];
    for (i = 0; i < kMaxJunks; i++)
        totalCargo += junkData[i].held;
    ratio = newShipHolds / totalCargo;
    ratio = newShipHolds / fleetHolds;
    if (ratio > 1.0)
        ratio = 1.0;

    for (i = 0; i <= 5; i++) {
        rounded = (shipData->cargo[i] * ratio);
        if (shipData->cargo[i] > rounded) {
            shipData->cargo[i] -= rounded;
        } else {
            shipData->cargo[i] = 0;
        }
        shipData[shipIndex].cargo[i] = rounded;
        if (shipData->cargo[i] < 0)
            shipData->cargo[i] = 0;
    }
    for (i = 0; i < kMaxJunks; i++) {
        rounded = (junkData[i].held * ratio);
        if (junkData[i].held < rounded) {
            junkData[i].held = 0;
        } else {
            junkData[i].held -= rounded;
        }
        if (junkData[i].held < 0)
            junkData[i].held = 0;
    }
    statusRedraw[2] = 1;
    ResetPlayerPrecalcedValues();
}

/* The flag is tested (with its companion at 0x007354b2) right before the
   shift/control/alt scancodes are scanned, so clearing it re-arms those. */

/* QuickKeyCheckHandler. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469c90
void QuickKeyCheckHandler(void)
{
    keyComboBlocked = 0;
}

/* QuickKeyCheck (mac _QuickKeyCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469ca0
unsigned char QuickKeyCheck(short scanCode)
{
    short i;

    if (ignoreKeys)
        return 0;
    if (escortMenuOnscreen > 0) {
        for (i = 0; i < 5; i++) {
            if (scanCode == keyPrefs.quickKey[i])
                return 0;
        }
    }
    return KeyIsDown(scanCode);
}

/*
 * The 1.0.10 body carries no escort logic, so the name stays descriptive rather
 * than adopting the mac's _QuickKeyCheckPlusEscorts.
 */
/* RawKeyCheck. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469cf0
unsigned char RawKeyCheck(short scanCode)
{
    if (ignoreKeys)
        return 0;
    return KeyIsDown(scanCode);
}

/* KeyCheck (mac _KeyCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469d10
unsigned char KeyCheck(short scanCode)
{
    KeyboardKbhit();
    return KeyIsDown(scanCode);
}

#define kDemandMedium 2

#define kSpobDemandMedium0 0x20000000UL
#define kSpobDemandMedium1 0x02000000UL
#define kSpobDemandMedium2 0x00200000UL
#define kSpobDemandMedium3 0x00020000UL
#define kSpobDemandMedium4 0x00002000UL
#define kSpobDemandMedium5 0x00000200UL

/* CalcPortDemand. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469d30
short CalcPortDemand(short commodity, unsigned long flags)
{
    switch (commodity) {
    case 0:
        if ((flags & kSpobDemandLow0) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium0) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh0) != 0)
            return kDemandHigh;
        break;
    case 1:
        if ((flags & kSpobDemandLow1) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium1) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh1) != 0)
            return kDemandHigh;
        break;
    case 2:
        if ((flags & kSpobDemandLow2) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium2) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh2) != 0)
            return kDemandHigh;
        break;
    case 3:
        if ((flags & kSpobDemandLow3) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium3) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh3) != 0)
            return kDemandHigh;
        break;
    case 4:
        if ((flags & kSpobDemandLow4) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium4) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh4) != 0)
            return kDemandHigh;
        break;
    case 5:
        if ((flags & kSpobDemandLow5) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium5) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh5) != 0)
            return kDemandHigh;
        break;
    }
    return 0;
}

#define kPortSpecialTech    8

#define kShipFlags3Requirements 0x0200
#define kShipFlags3BitTest      0x0100
#define kShipFlags3Unique       0x4000


/* SetupPortAvailableShipTypes (mac _SetupPortAvailableShipTypes). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469e90
void SetupPortAvailableShipTypes(SpobRecord *port)
{
    short count;
    short threshold;
    short i;
    short j;
    short k;
    short n;
    short best;
    short value;
    short a;
    char avail[kMaxShipTypes];

    if (port == 0)
        return;

    for (k = 0; k < kMaxShipTypes; k++)
        dockedPortShipTypes[k] = -1;

    count = 0;
    for (i = 0; i < kMaxShipTypes; i++) {
        avail[i] = 0;
        if (gShipTypes[i].techLevel >= 0) {
            if (gShipTypes[i].techLevel <= port->techLevel)
                avail[i] = 1;
            else {
                for (j = 0; j < kPortSpecialTech; j++) {
                    if (port->specialTech[j] == gShipTypes[i].techLevel) {
                        avail[i] = 1;
                        break;
                    }
                }
            }
        }

        if (avail[i]) {
            if (shipBuyMode == 0) {
                if (gShipTypes[i].buyRandom == 0
                        || gShipTypes[i].dailySaleRoll > gShipTypes[i].buyRandom)
                    avail[i] = 0;
            } else {
                if (gShipTypes[i].hireRandom == 0
                        || gShipTypes[i].dailyHireRoll
                                * (gShipTypes[gameIsRegisteredIndex].registered != 0 ? 1 : 0)
                                > gShipTypes[i].hireRandom)
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (gShipTypes[i].flags3 & kShipFlags3Requirements) {
                if (!PlayerMeetsRequirements(gShipTypes[i].requires[0],
                        gShipTypes[i].requires[1]))
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (gShipTypes[i].flags3 & kShipFlags3BitTest) {
                if (!EvalMissionBitTestString(gShipTypes[i].availability))
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            value = gShipTypes[i].dispWeight;
            if (value > threshold || count == 0)
                threshold = value;
            count++;
        }
    }

    for (a = 0; a < kMaxShipTypes; a++) {
        if (avail[a] && (gShipTypes[a].flags3 & kShipFlags3Unique)) {
            for (k = a + 1; k < kMaxShipTypes; k++) {
                if (gShipTypes[k].dispWeight == gShipTypes[a].dispWeight)
                    avail[k] = 0;
            }
        }
    }

    k = 0;
    do {
        count = 0;
        best = 0;
        for (n = 0; n < kMaxShipTypes; n++) {
            if (avail[n]) {
                value = gShipTypes[n].dispWeight;
                if (value >= threshold) {
                    dockedPortShipTypes[k++] = n;
                    avail[n] = 0;
                } else if (value > best || count == 0) {
                    best = value;
                }
                count++;
            }
        }
        threshold = best;
    } while (count > 0);
}


#define kSpobFlags2OwnedItems   0x0400


/* SetupPortAvailableItems (mac _SetupPortAvailableItems). STATE: MATCH, 208/208,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a220
char SetupPortAvailableItems(SpobRecord *port)
{
    short count;
    short i;
    short threshold;
    short j;
    short best;
    short value;
    short saved[kMaxOutfits];
    char avail[kMaxOutfits];

    if (port == 0)
        return 1;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        saved[i] = dockedPortItems[i];
        dockedPortItems[i] = -1;
        avail[i] = 0;
        if ((port->flags2 & kSpobFlags2OwnedItems) && shipSystems[i] > 0
                && !(gOutfits[i].flags & kOutfCantSell))
            avail[i] = 1;
        if (gOutfits[i].techLevel >= 0
                && gOutfits[i].techLevel < kOutfTechAbsent) {
            if (gOutfits[i].techLevel <= port->techLevel)
                avail[i] = 1;
            else {
                for (j = 0; j < kPortSpecialTech; j++) {
                    if (port->specialTech[j] == gOutfits[i].techLevel) {
                        avail[i] = 1;
                        break;
                    }
                }
            }
        }

        if (avail[i]) {
            if ((gOutfits[i].flags & kOutfTestAvailability)
                    && shipSystems[i] <= 0)
                avail[i] = EvalMissionBitTestString(gOutfits[i].availability);
        }

        if (avail[i]) {
            if ((gOutfits[i].flags & kOutfCheckRequire)
                    && shipSystems[i] <= 0)
                avail[i] = PlayerMeetsRequirements(gOutfits[i].requires[0],
                        gOutfits[i].requires[1]);
        }

        if (avail[i]) {
            if (shipSystems[i] > 0)
                gOutfits[i].dailyRoll = 0;
            else if (gOutfits[i].buyRandom <= 0
                    || gOutfits[i].dailyRoll
                            > gOutfits[i].buyRandom)
                avail[i] = 0;
        }

        if (gOutfits[i].flags & kOutfSellAnywhere) {
            if (shipSystems[i] > 0)
                avail[i] = 1;
        }

        if (avail[i]) {
            value = gOutfits[i].dispWeight;
            if (value > threshold || count == 0)
                threshold = value;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (avail[i] && (gOutfits[i].flags & kOutfSupersedes)) {
            for (j = i + 1; j < kMaxOutfits; j++) {
                if (gOutfits[j].dispWeight == gOutfits[i].dispWeight)
                    avail[j] = 0;
            }
        }
    }

    count = 0;
    do {
        j = 0;
        best = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (avail[i]) {
                value = gOutfits[i].dispWeight;
                if (value >= threshold) {
                    dockedPortItems[count++] = i;
                    avail[i] = 0;
                } else if (value > best || j == 0) {
                    best = value;
                }
                j++;
            }
        }
        threshold = best;
    } while (j > 0);

    for (i = 0; i < kMaxOutfits; i++) {
        if (saved[i] != dockedPortItems[i])
            return 1;
    }
    return 0;
}

/* TotalCargo (mac _TotalCargo). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a5d0
short TotalCargo(ShipObj *ship)
{
    short total;
    short i;

    total = 0;
    for (i = 0; i < 6; i++)
        total += ship->cargo[i];
    if (ship->index == 0) {
        for (i = 0; i < kMaxActiveMissions; i++) {
            if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                    && currentMissions[i].cargoQty >= 0)
                total += currentMissions[i].cargoQty;
        }
        for (i = 0; i < kMaxJunks; i++) {
            if (junkData[i].held > 0)
                total += junkData[i].held;
        }
    }
    return total;
}

/* PlayerHasAnyCargo (mac _PlayerHasAnyCargo). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a680
char PlayerHasAnyCargo(void)
{
    int i;

    for (i = 0; (short)i <= 5; i++) {
        if (shipData[0].cargo[(short)i] > 0)
            return 1;
    }
    for (i = 0; (short)i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0)
            return 1;
    }
    for (i = 0; (short)i < kMaxJunks; i++) {
        if (junkData[i].held > 0)
            return 1;
    }
    return 0;
}

/* the player's own hold capacity, outfit modifiers included */
/* ShipTotalHolds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a730
short ShipTotalHolds(void)
{
    short holds;
    short i;
    short j;

    holds = gShipTypes[shipData->type].cargo;
    for (i = 0; i < 512; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (gOutfits[i].modType[j] == kOutfModCargo)
                    holds += shipSystems[i] * gOutfits[i].modVal[j];
            }
        }
    }
    return holds;
}

/* TotalMissionCargoSpace. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a7c0
short TotalMissionCargoSpace(void)
{
    short holds;
    short fleetHolds;
    short cargo;
    short missionUnits;
    int i;
    short spare;
    short result;

    holds = ShipTotalHolds();
    fleetHolds = TotalFleetHolds();
    cargo = TotalCargo(shipData);
    missionUnits = 0;
    for (i = 0; (short)i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoQty >= 0)
            missionUnits += currentMissions[i].cargoQty;
    }
    if (fleetHolds > holds) {
        spare = cargo - missionUnits - (fleetHolds - holds);
        if (spare < 0)
            spare = 0;
        spare += missionUnits;
        result = holds - spare;
    } else {
        result = holds - cargo;
    }
    return result;
}

#define kStatusStripWidth 194

/* RefreshSAT (mac _RefreshSAT). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a870
void RefreshSAT(void)
{
    Rect *r;
    Rect box;
    Rect strip;

    if (creatingPilotFile != 0 || gPause != 0)
        return;
    if (!sw_depth_ok(gSpriteWorld))
        return;

    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    RGBForeColor(blackColor);
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);

    if (playerIsLanded == 0)
        SWUpdateSpriteWorld(gSpriteWorld, 0);

    if (commCount >= 0) {
        RedrawComm(1);
        SWSetPortToWindow(gSpriteWorld);
        BlitPixieFloatingRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                     &commRect, &commRect, 0, gSpriteWorld->backgroundValue);
    }

    CopyBits((PixMapPtr)GetPortBitMapForCopyBits(*(CGrafPtr *)gSpriteWorld->workFrameP),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);

    r = GetCWindowPortRect(gWind);
    box.top = r->top;
    box.left = r->left;
    box.bottom = r->bottom;
    box.right = r->right;
    box.left = box.right - kStatusStripWidth;
    strip.top = box.top;
    strip.left = box.left;
    strip.bottom = box.bottom;
    strip.right = box.right;
    OffsetRect(&strip, -strip.left, -strip.top);

    RedrawStatus(0);
    DrawStatusRadar(0);
    SWSetPortToWindow(gSpriteWorld);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &box, &box, 0, 0);

    if (playerIsLanded == 0) {
        RedrawComm(0);
        FloaterDrawCallback(gSpriteWorld);
    }
    SWSetPortToWindow(gSpriteWorld);
    ScreenEndScene();
}

/* PlaySound (mac _PlaySound). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046aad0
void PlaySound(void *a, short b, short lo, short hi)
{
    ST_PlaySound(a, b, (lo + hi) / 2);
}

/* SetSoundRate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ab00
void SetSoundRate(double rate)
{
    unsigned short vol;

    warpUpSoundSpec.rate = (65536.0 / rate);
    vol = gSoundVolume;
    warpUpSoundSpec.leftLevel = vol;
    warpUpSoundSpec.rightLevel = vol;
}

/* eight steps per unit, clamped to a byte's worth */
/* AdjustSoundVolume. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ab60
void AdjustSoundVolume(void)
{
    gSoundVolume = volumePref * 8;
    if (gSoundVolume > 0x100)
        gSoundVolume = 0x100;
    if (gSoundVolume < 0)
        gSoundVolume = 0;
}

/*
 * Cached in a short that is below -100 until computed and clamped to -100..100
 * once it is.
 */
#define kOutfModType24 0x18

/* ShipAntiInterference. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046abb0
short ShipAntiInterference(void)
{
    int i;
    short j;
    short total;

    if (gCachedMod24 < -100) {
        total = 0;
        for (i = 0; (short)i < 512; i++) {
            if (shipSystems[(short)i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType24)
                        total += shipSystems[(short)i] * gOutfits[i].modVal[j];
                }
            }
        }
        if (total > 100)
            total = 100;
        if (total < -100)
            total = -100;
        gCachedMod24 = total;
        return total;
    }
    return gCachedMod24;
}

/* SpawnNagShip. STATE: MATCH, 327/327; byte check pending the next regate. OPEN: the
   `(short)' on the Hector scan's bound is an unexplained device -- the exe compares that
   counter at 16 bits and indexes the weapon loop with the same long `w'. see notes */
// FUNCTION: EVNOVA 0x0046ac50
void SpawnNagShip(void)
{
    long chance;
    short slot;
    long w;
    short heading;
    float mag;
    float step;
    float total;
    FloatPoint origin;
    unsigned char msg[256];

    if (personData[kPersCaptainHector].shipType < 0)
        return;

    if (gSharewareDays <= 14)
        chance = 25000;
    else if (gSharewareDays < 30)
        chance = 10000;
    else if (gSharewareDays < 60)
        chance = 4000;
    else
        chance = 500;

    chance = chance * (1 - ((gShipTypes[gameIsRegisteredIndex].registered != 0) ? 1 : 0));

    if (chance == 0) {
        if (((gShipTypes[gameIsHackedIndex].hacked != 0) ? 1 : 0) == 0)
            return;
    }

    for (w = 1; (short)w < kMaxShips; w++) {
        if (shipData[w].active != 0 && shipData[w].persType == kPersCaptainHector)
            return;
    }

    if ((chance > 0 && Rand(chance) == 0) ||
        ((gShipTypes[gameIsHackedIndex].hacked != 0) ? 1 : 0)) {
        slot = GenericRandomShipSpawn(shipData->curSystem, 2);
        if (slot != -1) {
            if (gSharewareDays > 30) {
            ST_PlaySound(beepSnd[4], 1, gSoundVolume);
            GetIndString(msg, 30000, Rand(3) + 5);
            DisplayComm(msg, 0x168, 0, 12, &rgbWhite, 0, 0);
            }

            shipData[slot].persType = kPersCaptainHector;
            shipData[slot].type = personData[kPersCaptainHector].shipType;
            shipData[slot].aiType = 4;

            total = 0.0f;
            if (gSharewareDays >= 60)
                step = 31.0f;
            else
                step = 50.0f;
            while (step > 0.0f) {
                total += step;
                step -= 1.165;
            }

            shipData[slot].pos.x = shipData->pos.x;
            shipData[slot].pos.y = shipData->pos.y;

            if (gSharewareDays >= 60) {
            heading = shipData->heading;
            } else {
            origin.x = origin.y = 0.0f;
            heading = CalcInterceptAngle(&shipData->pos, &origin);
            }
            mag = -(total + 500.0);
            Accel(heading, mag, &shipData[slot].pos);

            shipData[slot].heading = heading;
            shipData[slot].dude = -1;
            shipData[slot].aggression = 4;
            shipData[slot].govt = -1;

            for (w = 0; w < 256; w++) {
                shipData[slot].weap[w].count = gShipTypes[shipData[slot].type].weapCount[w];
                shipData[slot].weap[w].ammo = gShipTypes[shipData[slot].type].weapAmmo[w];
                shipData[slot].weap[w].count += personData[kPersCaptainHector].weapCount[w];
                shipData[slot].weap[w].ammo += personData[kPersCaptainHector].weapAmmo[w];
            }

            shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
            shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
            if (personData[kPersCaptainHector].shieldMod > 0.0f) {
            shipData[slot].shield *= personData[kPersCaptainHector].shieldMod;
            shipData[slot].armor *= personData[kPersCaptainHector].shieldMod;
            }

            AISetup(&shipData[slot]);
            AIMakeShipJumpIn(&shipData[slot]);
            shipData[slot].lastInspected = -1;
            shipData[slot].aiTimer = -1.0f;
        }
    }
}

/* the shorter way round a 360-unit circle */

/* AngularDifference. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b210
short AngularDifference(short a, short b)
{
    int diff;

    diff = abs(a - b);
    if ((a >= 0xb4) != (b >= 0xb4))
        diff = 0x168 - diff;
    if ((short)diff > 0xb4)
        diff = 0x168 - diff;
    return diff;
}

#define kShipFlagBit10 0x400
#define kShipFlagBit6  0x40
#define kShipFlagBit5  0x20

/* AIHasAfterburner. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b260
char AIHasAfterburner(ShipObj *ship)
{
    short i;
    int flags;

    if (ship->index != 0) {
        for (i = 1; i < 64; i++) {
            if (i != ship->index && shipData[i].active != 0
                    && ship->index == shipData[i].swarmLeader)
                return 0;
        }
    }
    flags = gShipTypes[ship->type].flags;
    if ((flags & kShipFlagBit10) != 0)
        return 0;
    if ((flags & kShipFlagBit6) != 0)
        return 1;
    if ((flags & kShipFlagBit5) != 0
            && playerKills / gShipTypes[0].strength
               >= Rand(0x540) + 0x100)
        return 1;
    return 0;
}

/* true for all but the top few hundred of the 0x540 range */
/* AIPerformsComplexManeuvers (mac _AIPerformsComplexManeuvers). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b330
char AIPerformsComplexManeuvers(ShipObj *ship)
{
    if (playerKills >= Rand(0x540) + 0x100)
        return 1;
    return 0;
}

#define kWeapArcFront 0x1000
#define kWeapArcSide  0x2000
#define kWeapArcRear  0x4000

/* TurretBlindSpot. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b360
char TurretBlindSpot(ShipObj *ship, short heading, short weap)
{
    short facing;
    short diff;
    char blocked;

    blocked = 0;
    facing = ship->heading;
    diff = AngularDifference(facing, heading);
    if (diff <= 0x2d) {
        if (gWeapons[weap].flags & kWeapArcFront)
            blocked = 1;
        if (gShipTypes[ship->type].flags & kWeapArcFront)
            blocked = 1;
    } else if (diff <= 0x87) {
        if (gWeapons[weap].flags & kWeapArcSide)
            blocked = 1;
        if (gShipTypes[ship->type].flags & kWeapArcSide)
            blocked = 1;
    } else {
        if (gWeapons[weap].flags & kWeapArcRear)
            blocked = 1;
        if (gShipTypes[ship->type].flags & kWeapArcRear)
            blocked = 1;
    }
    return blocked;
}

#define kDudeSlots 16

/* SelectShipFieldFromDude. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b4b0
short SelectShipFieldFromDude(DudeRecord *dude, unsigned char allowAll)
{
    short cum[kDudeSlots];
    short total;
    short count;
    short i;
    short pick;
    short j;
    short roll;

    pick = -1;
    total = 0;
    count = 0;
    for (i = 0; i < kDudeSlots; i++) {
        cum[i] = 0;
        if (dude->shipTypes[i] != -1
            && (gShipTypes[dude->shipTypes[i]].canAppear != 0 || allowAll != 0)) {
            total += dude->shipProbs[i];
            for (j = 0; j <= i; j++) {
                if (dude->shipTypes[j] != -1
                    && (gShipTypes[dude->shipTypes[j]].canAppear != 0
                        || allowAll != 0)) {
                    cum[i] += dude->shipProbs[j];
                    count++;
                }
            }
        }
    }
    if (total <= 0 || count == 0)
        return -1;
    do {
        roll = Rand(total) + 1;
        for (i = kDudeSlots - 1; i >= 0; i--) {
            if (roll <= cum[i] && dude->shipTypes[i] != -1
                && (gShipTypes[dude->shipTypes[i]].canAppear != 0
                    || allowAll != 0))
                pick = i;
        }
    } while (pick == -1);
    return pick;
}

#define kSystDudeSlots 8

/* SelectDudeFieldFromSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b600
short SelectDudeFieldFromSystem(SystRecord *sys)
{
    short cum[kSystDudeSlots];
    short total;
    short pick;
    short i;
    short roll;
    short j;

    pick = -1;
    total = 0;
    for (i = 0; i < kSystDudeSlots; i++) {
        cum[i] = 0;
        if (sys->dudeTypes[i] >= 0 && sys->dudeTypes[i] < kMaxDudes) {
            total += sys->dudeProbs[i];
            for (j = 0; j <= i; j++) {
                if (sys->dudeTypes[j] >= 0 && sys->dudeTypes[j] < kMaxDudes)
                    cum[i] += sys->dudeProbs[j];
            }
        }
    }
    if (total <= 0)
        return -1;
    do {
        roll = Rand(total) + 1;
        for (i = kSystDudeSlots - 1; i >= 0; i--) {
            if (roll <= cum[i] && sys->dudeTypes[i] >= 0
                && sys->dudeTypes[i] < kMaxDudes)
                pick = i;
        }
    } while (pick == -1);
    return pick;
}

/* SelectFleetFromSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b6d0
short SelectFleetFromSystem(SystRecord *syst)
{
    short total;
    short i;
    short probs[8];
    short picks[8];
    short g;
    short k;
    short prob;
    short pick;

    if (syst->fleetCount <= 0)
        return -1;

    g = 0;
    total = 0;
    for (i = 0; i < syst->fleetCount; i++) {
        if (fleetData[syst->fleets[i]].leadShipType != -1 && fleetData[syst->fleets[i]].usable != 0) {
            picks[g] = syst->fleets[i];
            prob = syst->fleetProbs[i];
            probs[g] = prob;
            total += prob;
            for (k = 0; k < g; k++)
                probs[g] += probs[k];
            g++;
        }
    }

    if (g == 0 || total == 0)
        return -1;

    pick = Rand(total) + 1;
    for (k = 0; k < g; k++) {
        if (pick <= probs[k])
            return picks[k];
    }
    return -1;
}

/* the shipyard's asking-price jitter, as a fraction around 1.00 */

/* RandomSkillLevel. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b870
float RandomSkillLevel(short type)
{
    return (Rand(gShipTypes[type].skillVar * 2 + 1)
            + (100 - gShipTypes[type].skillVar)) * 0.01;
}

/* the player, or something escorting the player directly or one level down */
/* AlliedWithPlayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b8d0
char AlliedWithPlayer(ShipObj *ship)
{
    short parent;

    if (ship->index == 0)
        return 1;
    parent = ship->parentShip;
    if (parent == 0)
        return 1;
    if (parent > 0 && parent < kMaxShips && shipData[parent].parentShip == 0)
        return 1;
    return 0;
}

/* the first system of the group that has actually been loaded */
/* FindActiveCoLocatedSystem (mac _FindActiveCoLocatedSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b920
short FindActiveCoLocatedSystem(short sys)
{
    short next;

    if (sys < 0 || sys >= 2048)
        return -1;
    next = gSystems[sys].colocGroup;
    if (next == -1) {
        if (gSystems[sys].loaded != 0)
            return sys;
        return -1;
    }
    do {
        if (gSystems[next].loaded != 0)
            return next;
        next = gSystems[next].colocNext;
    } while (next != -1);
    return -1;
}

/* FindFirstCoLocatedSystem (mac _FindFirstCoLocatedSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b9b0
short FindFirstCoLocatedSystem(short sys)
{
    short first;

    if (sys == -1)
        return -1;
    first = gSystems[sys].colocGroup;
    if (first != -1)
        return first;
    return sys;
}

/* HideTheCursor (mac _HideTheCursor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b9f0
void HideTheCursor(void)
{
    OSCursorHide();
    cursorVisible = (OSCursorIsVisible() == 0);
}

/* ShowTheCursor (mac _ShowTheCursor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ba10
void ShowTheCursor(void)
{
    OSCursorShow();
    cursorVisible = (OSCursorIsVisible() == 0);
}

/* FindNearestValidShotTarget. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ba30
short FindNearestValidShotTarget(ShotObj *shot)
{
    short best;
    short bestDist;
    short dx;
    short dy;
    short dist;
    short i;

    best = -1;
    for (i = 0; i < 64; i++) {
        if (shipData[i].active != 0 &&
            shipData[i].curSystem == shipData->curSystem &&
            ShotCanHitShip(shot, &shipData[i])) {
            dx = abs((long)shipData[i].pos.x - (long)shot->pos.x);
            dy = abs((long)shipData[i].pos.y - (long)shot->pos.y);
            dist = dx * dx + dy * dy;
            if (dist < bestDist || best == -1) {
                best = i;
                bestDist = dist;
            }
        }
    }
    return best;
}

/* paint in the current system's colour, and tell SpriteWorld about it too */

/* SetSystemBackgroundColor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046bbf0
void SetSystemBackgroundColor(void)
{
    short sys;

    sys = shipData[0].curSystem;
    if (sys < 0)
        sys = 0;
    RGBForeColor(&gSystems[sys].rgb);
    SWSetBackgroundColor(gSpriteWorld, &gSystems[sys].rgb);
}

/* SetSystemBackgroundBackColor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046bc50
void SetSystemBackgroundBackColor(void)
{
    if (shipData[0].curSystem >= 0) {
        BackColor(&gSystems[shipData[0].curSystem].rgb);
        return;
    }
    BackColor(blackColor);
}

/* GovtAllies (mac _GovtAllies). STATE: MATCH, exe 113 / obj 113, byte-identical yes
   (relocs masked). OPEN: none. Guard order `a,a,b,b' is the exe's -- do not normalise it
   to the twin's. see notes */
// FUNCTION: EVNOVA 0x0046bc90
char GovtAllies(short a, short b)
{
    short other;
    short i;
    short j;
    short other2;

    if (a == b)
        return 1;
    if (a >= 0 && a < kMaxGovts && b >= 0 && b < kMaxGovts) {
        if ((govtData[a].flags & kGovtAlwaysDisabled) != 0 ||
            (govtData[b].flags & kGovtAlwaysDisabled) != 0)
            return 0;

        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    other = govtData[b].allies[j];
                    if (other >= 0 && other == govtData[a].classes[i])
                        return 1;
                }
            }
            if (govtData[b].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    other2 = govtData[a].allies[j];
                    if (other2 >= 0 && other2 == govtData[b].classes[i])
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* GovtEnemies (mac _GovtEnemies). STATE: MATCH, exe 159 / obj 159, byte-identical yes
   (relocs masked). OPEN: none. Guard order `a,b,a,b' is the exe's -- do not normalise it
   to the twin's. see notes */
// FUNCTION: EVNOVA 0x0046bdf0
char GovtEnemies(short a, short b)
{
    short other;
    short i;
    short j;
    short other2;

    if (a == b)
        return 0;
    if (a >= 0 && b >= 0 && a < kMaxGovts && b < kMaxGovts) {
        if ((govtData[a].flags & kGovtAlwaysDisabled) != 0 ||
            (govtData[b].flags & kGovtAlwaysDisabled) != 0)
            return 0;

        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    other = govtData[b].enemies[j];
                    if (other >= 0 && other == govtData[a].classes[i])
                        return 1;
                }
            }
            if (govtData[b].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    other2 = govtData[a].enemies[j];
                    if (other2 >= 0 && other2 == govtData[b].classes[i])
                        return 1;
                }
            }
        }
    }

    if (!GovtAllies(a, b)) {
        if (a >= 0 && a < kMaxGovts && (govtData[a].flags & kGovtXenophobic) != 0)
            return 1;
        if (b >= 0 && b < kMaxGovts && (govtData[b].flags & kGovtXenophobic) != 0)
            return 1;
    }
    return 0;
}

/* GovtSharedClass. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046bff0
char GovtSharedClass(short a, short b)
{
    short i;

    if (a == b)
        return 1;
    if (a >= 0 && a < 256 && b >= 0 && b < 256) {
        for (i = 0; i < 4; i++) {
            if (govtData[a].classes[i] >= 0 &&
                govtData[a].classes[i] == govtData[b].classes[i])
                return 1;
        }
    }
    return 0;
}

#define kOutfModType39 0x27

/* ShipDeionizationRate (mac _ShipDeionizationRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c080
float ShipDeionizationRate(ShipObj *ship)
{
    short i;
    short j;
    float value;

    value = gShipTypes[ship->type].deionize;
    if (ship->index == 0) {
        if (gCachedMod39 >= 0.0f) {
            value = gCachedMod39;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType39)
                            value = value + gOutfits[i].modVal[j] * 0.01 *
                                    shipSystems[i];
                    }
                }
            }
            gCachedMod39 = value;
        }
    }
    return value;
}

#define kOutfModType40 0x28

/* ShipIonizationFactor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c160
float ShipIonizationFactor(ShipObj *ship)
{
    short i;
    short j;
    float total;

    total = gShipTypes[ship->type].ionizeMax;
    if (ship->index == 0) {
        if (gCachedMod40 >= 0.0f) {
            total = gCachedMod40;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType40)
                            total = total + gOutfits[i].modVal[j] *
                                    shipSystems[i];
                    }
                }
            }
            gCachedMod40 = total;
        }
    }
    if (total > 0.0f)
        return ship->ionization / total;
    return 0.0f;
}

#define kOutfModType28 0x1c

/* PlayerEffectiveMurk (mac _PlayerEffectiveMurk). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c250
short PlayerEffectiveMurk(void)
{
    int i;
    short j;
    short level;

    level = gSystems[shipData[0].curSystem].murk;
    if (level < 0)
        level = 0;
    for (i = 0; (short)i < 512; i++) {
        if (shipSystems[(short)i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (gOutfits[i].modType[j] == kOutfModType28)
                    level += shipSystems[(short)i] * gOutfits[i].modVal[j];
            }
        }
    }
    if (level < 0)
        level = 0;
    if (level > 100)
        level = 100;
    return level;
}

#define kWeapFlag1000 0x1000

/* WeaponDisableOnly. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c2f0
char WeaponDisableOnly(short weap)
{
    if ((gWeapons[weap].flags2 & kWeapFlag1000) != 0)
        return 1;
    return 0;
}

/* ModifyShotStartPosition (mac _ModifyShotStartPosition). STATE: MATCH 100%, exe 130 / obj 130,
   byte-identical yes (relocs masked). OPEN: none. */
// FUNCTION: EVNOVA 0x0046c320
short ModifyShotStartPosition(ShipObj *ship, FloatPoint *base, short weapType, FloatPoint *target)
{
    short group;
    short divisor;
    short angle;
    short result;
    short lane;

    if (ship == 0 || base == 0)
        return -1;

    result = -1;
    group = gWeapons[weapType].exitType;
    if (group >= 0 && group <= 3) {
        divisor = gShipTypes[ship->type].framesPer;
        angle = *(long *)(ship->sprite + 0x68) % divisor;
        angle *= 360.0 / divisor;

        if ((gWeapons[weapType].flags3 & kWeapClosestExit) && target != 0) {
            ship->weapGroupLane[group] = SelectClosestShotStartPosition(
                ship, base, weapType, angle, group, target);
        }

        if (ship->weapGroupLane[group] < 0 || ship->weapGroupLane[group] > 3)
            ship->weapGroupLane[group] = Rand(4);

        ModifyShotStartPosition2(ship, base, angle, group, ship->weapGroupLane[group]);

        lane = ship->weapGroupLane[group];
        result = lane;
        ship->weapGroupLane[group] = ((lane + 1) % 4);

        if (ship->weapGroupLane[group] < 0 || ship->weapGroupLane[group] > 3)
            ship->weapGroupLane[group] = Rand(4);
    }

    return result;
}

/* SelectClosestShotStartPosition (mac _SelectClosestShotStartPosition). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c4e0
short SelectClosestShotStartPosition(ShipObj *ship, FloatPoint *base, long unused,
                                    short weapType, short mount,
                                    register FloatPoint *target)
{
    int best;
    int i;
    FloatPoint pos;
    FloatPoint start;
    float d;
    float bestDist;

    if (ship == 0 || base == 0 || target == 0 || mount < 0 || mount > 3)
        return 0;
    start.x = base->x;
    start.y = base->y;
    best = -1;
    for (i = 0; (short)i <= 3; i++) {
        pos.x = start.x;
        pos.y = start.y;
        ModifyShotStartPosition2(ship, &pos, weapType, mount, i);
        d = XYFloatSquaredDist(&pos, target);
        if (d < bestDist || (short)best == -1) {
            best = i;
            bestDist = d;
        }
    }
    return best;
}

/*
 * Added to the caller's point rather than replacing it, which is what lets the
 * caller run all four lanes from one base.
 */
/* ModifyShotStartPosition2 (mac _ModifyShotStartPosition2). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c5c0
void ModifyShotStartPosition2(ShipObj *ship, FloatPoint *pos, short angle,
                              short mount, short lane)
{
    FloatPoint d;

    if (ship == 0 || pos == 0 || mount < 0 || mount > 3 || lane < 0 || lane > 3)
        return;

    d.x = d.y = 0;
    Accel(angle, gShipTypes[ship->type].weapPos[mount][lane][1], &d);
    Accel(((angle + 90) % 360),
          gShipTypes[ship->type].weapPos[mount][lane][0], &d);

    if (d.y < 0.0) {
        d.x = d.x * gShipTypes[ship->type].compress[0][0];
        d.y = d.y * gShipTypes[ship->type].compress[0][1];
    } else {
        d.x = d.x * gShipTypes[ship->type].compress[1][0];
        d.y = d.y * gShipTypes[ship->type].compress[1][1];
    }
    d.y = d.y - gShipTypes[ship->type].weapPosZ[mount][lane];

    pos->x = pos->x + d.x;
    pos->y = pos->y + d.y;
}

/* ShipIsCloaked (mac _ShipIsCloaked). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c7a0
char ShipIsCloaked(ShipObj *ship)
{
    float cloak;

    cloak = ship->cloak;
    if (cloak > 24.0f && ship->cloakDir >= 0)
        return 1;
    if (cloak > 8.0f && ship->cloakDir < 0)
        return 1;
    if (cloak > 16.0f)
        return 1;
    return 0;
}

/* the running game date (invented names) */

/* (invented name — no mac symbol; likely static in the mac build) */

/* CronDateIsValid. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c800
char CronDateIsValid(CronRecord *cron)
{
    unsigned short m;
    unsigned short d;
    unsigned short d2;

    if (cron->firstYear > 0 && gameTime.year < cron->firstYear)
        return 0;
    m = cron->firstMonth;
    if (m == 0) {
        d = cron->firstDay;
        if ((short)d <= 0)
            goto firstShared;
        if (gameTime.day < (short)d)
            return 0;
    } else {
firstShared:
        if ((short)m <= 0 || cron->firstDay != 0) {
            if ((short)m > 0) {
                d2 = cron->firstDay;
                if ((short)d2 > 0) {
                    if (gameTime.month * 0x20 + gameTime.day <
                            ((short)m * 0x20 + (short)d2))
                        return 0;
                }
            }
        } else if (gameTime.month < (short)m) {
            return 0;
        }
    }
    if (cron->lastYear > 0 && gameTime.year > cron->lastYear)
        return 0;
    m = cron->lastMonth;
    if (m == 0) {
        d = cron->lastDay;
        if ((short)d <= 0)
            goto lastShared;
        if (gameTime.day > (short)d)
            return 0;
    } else {
lastShared:
        if ((short)m <= 0 || cron->lastDay != 0) {
            if ((short)m > 0) {
                d2 = cron->lastDay;
                if ((short)d2 > 0) {
                    if (gameTime.month * 0x20 + gameTime.day >
                            ((short)m * 0x20 + (short)d2))
                        return 0;
                }
            }
        } else if (gameTime.month > (short)m) {
            return 0;
        }
    }
    return 1;
}

/* ShipCanTargetUntargetableShips. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c930
char ShipCanTargetUntargetableShips(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (gCachedMod30 >= 0) {
            if (gCachedMod30 != 0)
                return 1;
        } else {
            gCachedMod30 = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType30 &&
                            (gOutfits[i].modVal[j] & kOutfModVal30Bit2) != 0) {
                            gCachedMod30 = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType30 &&
                        (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal30Bit2) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* the twin of the above: bit 3 of the value rather than bit 2, its own cache */

/* ShipCanTargetCloakedShips. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ca60
char ShipCanTargetCloakedShips(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (gCachedMod30b >= 0) {
            if (gCachedMod30b != 0)
                return 1;
        } else {
            gCachedMod30b = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType30 &&
                            (gOutfits[i].modVal[j] & kOutfModVal30Bit3) != 0) {
                            gCachedMod30b = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType30 &&
                        (gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal30Bit3) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

#define kOutfModType31 0x1f

/* ShipCanScoop. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cb90
char ShipCanScoop(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType31)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType31)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* PutSpriteInLayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cc60
void PutSpriteInLayer(SpriteRec *sprite, SpriteLayerRec *layer)
{
    if (sprite == NULL || layer == NULL)
        return;
    if (sprite->parentSpriteLayerP == layer)
        return;
    SWRemoveSprite(sprite);
    SWAddSprite(layer, sprite);
}

/* GetPlayerContributeBits (mac _GetPlayerContributeBits). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cca0
void GetPlayerContributeBits(unsigned long *lo, unsigned long *hi)
{
    unsigned long bitsLo;
    unsigned long bitsHi;
    int i;

    bitsLo = gShipTypes[shipData->type].contributes[0];
    bitsHi = gShipTypes[shipData->type].contributes[1];
    i = 0;
    do {
        if (rankData[i].active != '\0' && rankData[i].loaded != '\0') {
            bitsLo |= rankData[i].contributes[0];
            bitsHi |= rankData[i].contributes[1];
        }
        i = i + 1;
    } while ((short)i < kMaxRanks);
    i = 0;
    do {
        if (gCrons[i].active != '\0' && gCrons[i].loaded != '\0'
                && gCrons[i].holdoffLeft <= 0) {
            bitsLo |= gCrons[i].contributes[0];
            bitsHi |= gCrons[i].contributes[1];
        }
        i = i + 1;
    } while ((short)i < kMaxCrons);
    i = 0;
    do {
        if (shipSystems[(short)i] > 0) {
            bitsLo |= gOutfits[i].contributes[0];
            bitsHi |= gOutfits[i].contributes[1];
        }
        i = i + 1;
    } while ((short)i < kMaxOutfits);
    *lo = bitsLo;
    *hi = bitsHi;
}

/* PlayerMeetsRequirements (mac _PlayerMeetsRequirements). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cd80
char PlayerMeetsRequirements(unsigned long reqLo, unsigned long reqHi)
{
    unsigned long hi;
    unsigned long lo;
    unsigned long gotLo;
    unsigned long gotHi;

    GetPlayerContributeBits(&lo, &hi);
    gotLo = reqLo & lo;
    gotHi = hi & reqHi;
    if (reqLo == gotLo && gotHi == reqHi)
        return 1;
    return 0;
}

#define kOutfModType32 0x20

/* ShipJumpsPerJump (mac _ShipJumpsPerJump). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cdd0
short ShipJumpsPerJump(ShipObj *ship)
{
    short i;
    short j;
    short total;

    total = 1;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType32)
                        total += gOutfits[i].modVal[j];
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType32)
                        total += gOutfits[gShipTypes[ship->type].defOutfID[i]].modVal[j];
                }
            }
        }
    }
    if (total < 1)
        total = 1;
    return total;
}

/* ShipMaxWeaponRange. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cec0
short ShipMaxWeaponRange(ShipObj *ship)
{
    short i;
    long best;
    long range;
    short guidance;

    best = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count > 0 && WeaponHasAmmo(ship, i) != 0) {
            guidance = gWeapons[i].guidance;
            if (guidance == kGuidanceBeam || guidance == kGuidanceTurretBeam) {
                range = gWeapons[i].beamLength;
            } else if (guidance == kGuidanceUnguided || guidance == kGuidanceTurret
                       || guidance == kGuidanceFrontQuad || guidance == kGuidanceFrontQuad) {
                range = gWeapons[i].totalRange;
            } else if (guidance == kGuidanceHoming) {
                range = (gWeapons[i].totalRange * 0.85);
            } else if (guidance == kGuidanceRocket) {
                range = (gWeapons[i].totalRange * 0.5);
            } else {
                range = 0;
            }
            if (range > best)
                best = range;
        }
    }
    return (best >= 0x7fff) ? 0x7fff : best;
}

#define kOutfModType37 0x25
#define kShipFlags2Bit5 0x20

/* ShipCanExpiditeJumps (mac _ShipCanExpiditeJumps). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d080
char ShipCanExpiditeJumps(ShipObj *ship)
{
    short i;
    short j;

    if ((gShipTypes[ship->type].flags2 & kShipFlags2Bit5) != 0)
        return 1;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType37)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType37)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* ShipsShareParent (mac _ShipsShareParent). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d190
char ShipsShareParent(ShipObj *a, ShipObj *b)
{
    short rootA;
    short rootB;
    short next;

    if (a == 0 || b == 0)
        return 0;
    rootA = a->index;
    while (1) {
        if (shipData[rootA].active == '\0')
            break;
        next = shipData[rootA].parentShip;
        if (next < 0)
            break;
        if (next >= 64)
            break;
        if (rootA == next)
            break;
        rootA = next;
    }
    rootB = b->index;
    while (1) {
        if (shipData[rootB].active == '\0')
            break;
        next = shipData[rootB].parentShip;
        if (next < 0)
            break;
        if (next >= 64)
            break;
        if (rootB == next)
            break;
        rootB = next;
    }
    if (rootA == rootB)
        return 1;
    return 0;
}

/* PtoCstr (mac _PtoCstr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d250
char *PtoCstr(unsigned char *str)
{
    short len = *str;
    short i;

    for (i = 0; i < len; i++)
        str[i] = str[i + 1];
    str[len] = 0;
    return (char *)str;
}

/* CtoPstr (mac _CtoPstr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d380
unsigned char *CtoPstr(char *str)
{
    short len = strlen(str);
    short i;

    for (i = len; i >= 1; i--)
        str[i] = str[i - 1];
    *str = len;
    return (unsigned char *)str;
}

/* junkData's record, padded to its true 0x526 stride */

/* the rank record, padded to its true 0x120 stride */

#define kRankBlesses  0x100
#define kRankCondemns 0x200

#define kJunkFlagA 1
#define kJunkFlagB 2

/* the modType values this routine reacts to */
#define kModCloak  0x1f
#define kModJam2C  0x2c
#define kModJam30  0x30
#define kModScan2F 0x2f
#define kModScan32 0x32


/* ResetPlayerPrecalcedValues. STATE: MATCH 100%, exe 587 / obj 587; relocregcheck 0 of 114
   differ; byte identity awaits the next regate. OPEN: none. The licence code is read at each
   of the three checks; a `key' copy local was invented (2026-09-24). */
// FUNCTION: EVNOVA 0x0046d4b0
void ResetPlayerPrecalcedValues(void)
{
    float ratio;
    short holds;
    short cargo;
    short i;
    short j;
    short k;
    short m;
    char keyed[3];

    playerShieldCapacity = -1.0f;
    gCachedShieldRech = -1.0f;
    gCachedArmorCap = -1.0f;
    gCachedArmorRech = -1.0f;
    gCachedTurnRate = -1.0f;
    gCachedAccelRate = -1.0f;
    gCachedMaxSpeed = -1.0f;
    gCachedFuelCap = -1.0f;
    gCachedMod40 = -1.0f;
    gCachedMod39 = -1.0f;
    playerHasAfterburner = -1;
    gHasModType13 = -1;
    gHasModType14 = -1;
    gCachedMod38 = -1;
    gCachedMod24 = 0x8300;
    gMaxSoundRange = -1;
    gCachedMod30 = -1;
    gCachedMod30b = -1;
    gCachedMod15 = -1.0f;
    gCachedMod41 = -1;
    gCachedMod42 = -1;
    playerHasAreaCloak = -1;
    playerHasIllegalJunk = 0;
    playerHasIllegalOutfits = 0;
    playerSabotaged = 0;
    playerHasTribbles = 0;
    playerHasPerishableJunk = 0;
    shipData->fuelRegenState = -1;
    shipData->canScoop = 0;

    keyed[0] = ((*(unsigned long long *)((char *)shipData + 0x4b7a0) ^ 0x6e1bbb2b46dfcc17LL) & kKeyMask) != 0x0c9f3b380ff9e811LL;
    keyed[1] = ((*(unsigned long long *)((char *)shipData + 0x4b7a0) ^ 0x1f1e62f77679a8c9LL) & kKeyMask) != 0x0e1f82a0c71946cbLL;
    keyed[2] = ((*(unsigned long long *)((char *)shipData + 0x4b7a0) ^ 0xa63b2518c45f7e5bLL) & kKeyMask) != 0x010d795bcf7c9f6aLL;
    for (i = 0; i < 3; i++) {
        if (keyed[i] == 0) {
            playerShieldCapacity = 1.0f;
            gCachedArmorCap = 1.0f;
        }
    }

    for (i = 0; i < 0x80; i++) {
        if (junkData[i].held > 0) {
            if (junkData[i].scanMask != 0)
                playerHasIllegalJunk = 1;
            if ((junkData[i].flags & kJunkFlagA) != 0)
                playerHasTribbles = 1;
            else if ((junkData[i].flags & kJunkFlagB) != 0)
                playerHasPerishableJunk = 1;
        }
    }

    for (i = 0; i < 0x200; i++) {
        if (shipSystems[i] > 0 && gOutfits[i].scanMask != 0) {
            playerHasIllegalOutfits = 1;
            break;
        }
    }

    for (i = 0; i < 0x200; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (gOutfits[i].modType[j] == kModScan2F) {
                    playerSabotaged = 1;
                    sabotageCounter = 0.0f;
                }
                if (gOutfits[i].modType[j] == kModScan32) {
                    if (playerSabotaged < 1)
                        playerSabotaged = 2;
                    sabotageCounter = 0.0f;
                }
                if (gOutfits[i].modType[j] == kModJam2C) {
                    if (gOutfits[i].modVal[j] == -1) {
                        allReinforcementsInhibited = 1;
                    } else {
                        for (k = 0; k < 0x100; k++) {
                            for (m = 0; m < 4; m++) {
                                if (govtData[k].classes[m] ==
                                        gOutfits[i].modVal[j] &&
                                    govtData[k].classes[m] != -1) {
                                    govtData[k].reinfInhibited = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (gOutfits[i].modType[j] == kModJam30) {
                    for (k = 0; k < 0x100; k++) {
                        for (m = 0; m < 4; m++) {
                            if (govtData[k].classes[m] ==
                                    gOutfits[i].modVal[j] &&
                                govtData[k].classes[m] != -1) {
                                govtData[k].iffScrambled = 1;
                                break;
                            }
                        }
                    }
                }
                if (gOutfits[i].modType[j] == kModCloak)
                    shipData->canScoop = 1;
            }
        }
    }

    if (playerSabotaged != 0 && sabotageCounter <= 0.0f)
        sabotageCounter = Rand(100);
    if (shipData->canScoop != 0) {
        if (TotalFleetHolds() - TotalCargo(shipData) <= 0)
            shipData->canScoop = 0;
    }
    shipData->jamming[0] = -1;
    shipData->jamming[1] = -1;
    shipData->jamming[2] = -1;
    shipData->jamming[3] = -1;
    holds = TotalFleetHolds();
    cargo = TotalCargo(shipData);
    if (cargo > holds) {
        ratio = holds / (float)cargo;
        shipData->cargo[0] *= ratio;
        shipData->cargo[1] *= ratio;
        shipData->cargo[2] *= ratio;
        shipData->cargo[3] *= ratio;
        shipData->cargo[4] *= ratio;
        shipData->cargo[5] *= ratio;
        statusRedraw[2] = 1;
    }
    for (i = 0; i <= 5; i++) {
        if (shipData->cargo[i] < 0) {
            shipData->cargo[i] = 0;
            statusRedraw[2] = 1;
        }
    }
    for (i = 0; i < 0x80; i++) {
        if (junkData[i].held < 0) {
            junkData[i].held = 0;
            statusRedraw[2] = 1;
        }
    }
    shipData->seesCloaked = -1;
    shipData->seesCloakedRadar = -1;
    shipData->decloaksWhenHit = -1;

    for (i = 0; i < 0x100; i++) {
        govtData[i].blessesPlayer[0] = 0;
        govtData[i].blessesPlayer[1] = 0;
    }
    for (i = 0; i < 0x80; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            (rankData[i].flags & (kRankBlesses | kRankCondemns)) != 0 &&
            rankData[i].govt != -1) {
            for (j = 0; j < 0x100; j++) {
                if (GovtAllies(j, rankData[i].govt) != 0) {
                    if ((rankData[i].flags & kRankBlesses) != 0)
                        govtData[j].blessesPlayer[0] = 1;
                    if ((rankData[i].flags & kRankCondemns) != 0)
                        govtData[j].blessesPlayer[1] = 1;
                }
            }
        }
    }
    playerRepairInhibit = -1.0f;
    gMaxSoundRange = PlayerEffectiveMurk();
}

#define kShipFlags3Bit5 0x20

#define kOutfModType38 0x26
#define kShipFlags2Bit6 0x40

/* ShipIsInertialess. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046df70
char ShipIsInertialess(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index != 0 && ship->parentShip != -1 && IsShipInCloseForm(ship))
        return 0;
    if ((gShipTypes[ship->type].flags2 & kShipFlags2Bit6) != 0)
        return 1;
    if (ship->index == 0) {
        if (gCachedMod38 >= 0) {
            if (gCachedMod38 != 0)
                return 1;
        } else {
            gCachedMod38 = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType38) {
                            gCachedMod38 = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * Memoised in a float that is negative until computed, and the modifier's
 * value is scaled by 0.0333, which is one thirtieth to four figures.
 */

/* ShipAfterburnerFuelUsage (mac _ShipAfterburnerFuelUsage). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e060
float ShipAfterburnerFuelUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index != 0)
        return 0.0f;
    if (gCachedMod15 < 0.0) {
        gCachedMod15 = 0.0f;
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType15) {
                        gCachedMod15 = gOutfits[i].modVal[j] * 0.0333;
                        break;
                    }
                }
            }
        }
    }
    return gCachedMod15;
}

#define kOutfModType41 0x29

/* ShipResistsGravity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e120
short ShipResistsGravity(ShipObj *ship)
{
    short i;
    short j;

    if ((gShipTypes[ship->type].flags3 & kShipFlags3Bit5) != 0)
        return 1;
    if (ShipIsInertialess(ship))
        return 1;
    if (ship->index == 0) {
        if (gCachedMod41 == -1) {
            gCachedMod41 = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType41) {
                            gCachedMod41 = 1;
                            break;
                        }
                    }
                }
            }
        } else if (gCachedMod41 != 0) {
            return 1;
        }
        return gCachedMod41;
    }
    return 0;
}

#define kOutfModType42 0x2a

/* ShipResistsDeadlyStellars. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e210
short ShipResistsDeadlyStellars(ShipObj *ship)
{
    short i;
    short j;

    if ((gShipTypes[ship->type].flags3 & kShipFlags3Bit5) != 0)
        return 1;
    if (ship->index == 0) {
        if (gCachedMod42 == -1) {
            gCachedMod42 = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (gOutfits[i].modType[j] == kOutfModType42) {
                            gCachedMod42 = 1;
                            break;
                        }
                    }
                }
            }
        } else if (gCachedMod42 != 0) {
            return 1;
        }
        return gCachedMod42;
    }
    return 0;
}

/*
 * The distances are taken in hundredths and folded to positive by multiplying
 * by -1 rather than negating, and the squared distance is floored at 30 so a
 * near-coincident pair cannot produce an unbounded push.
 */

/* ApplyGravity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e2f0
void ApplyGravity(FloatPoint *a, FloatPoint *b, float force, FloatPoint *out)
{
    float dx;
    float dy;
    dx = (a->x - b->x) * 0.01;
    dy = (a->y - b->y) * 0.01;
    if (dx < 0)
        dx = dx * -1.0;
    if (dy < 0)
        dy = dy * -1.0;
    dy = dx * dx + dy * dy;
    if (dy < 30)
        dy = 30;
    Accel(CalcInterceptAngle(a, b), force / dy, out);
}

/* StellarIsDestroyed (mac _StellarIsDestroyed). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e3c0
char StellarIsDestroyed(SpobRecord *spob)
{
    if (spob->strength > 0 && (spob->deadCounter < 0 || spob->deadDaysLeft > 0))
        return 1;
    return 0;
}

/* StellarIsLandable (mac _StellarIsLandable). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e3f0
char StellarIsLandable(SpobRecord *spob)
{
    if (SpobCanLand(spob) &&
        ((StellarIsDestroyed(spob) && SpobLandOnlyWhenDestroyed(spob)) ||
         (!StellarIsDestroyed(spob) && !SpobLandOnlyWhenDestroyed(spob))))
        return 1;
    return 0;
}

/* landable and an actual place, i.e. not a hypergate or a wormhole */
/* StellarIsNormalLandable (mac _StellarIsNormalLandable). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e440
char StellarIsNormalLandable(SpobRecord *spob)
{
    if (StellarIsLandable(spob) && !SpobIsHyperOrWorm(spob))
        return 1;
    return 0;
}

#define kNeutralComponent 0x20

/* GetShipPaintColor (mac _GetShipPaintColor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e470
void GetShipPaintColor(ShipObj *ship, unsigned short *red, unsigned short *green,
                  unsigned short *blue)
{
    unsigned short b;
    unsigned short g;
    unsigned short r;
    unsigned short *rgb;

    r = g = b = kNeutralComponent;
    if (ship->index == 0) {
        r = playerPaint.red;
        g = playerPaint.green;
        b = playerPaint.blue;
    } else if (ship->persType != -1) {
        rgb = personData[ship->persType].rgb5;
        r = rgb[0];
        g = rgb[1];
        b = rgb[2];
    } else if (ship->govt != -1) {
        rgb = (unsigned short *)&govtData[ship->govt].rgb2;
        r = rgb[0];
        g = rgb[1];
        b = rgb[2];
    }
    if (r == 0 && g == 0 && b == 0)
        r = g = b = kNeutralComponent;
    *red = r;
    *green = g;
    *blue = b;
}

#define kOutfModType49 0x31

#define kShipDefOutfits 8

/* ShipCanSelfRepair (mac _ShipCanSelfRepair). STATE: MATCH 100.0%, exe 132 / obj 132, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0046e540
char ShipCanSelfRepair(ShipObj *ship)
{
    short i;
    short j;

    if (gTimeScale > 0.0) {
        if (Rand((500.0 / gTimeScale)) != 0)
            return 0;
    } else {
        if (Rand(500) != 0)
            return 0;
    }
    if (IsDying(ship))
        return 0;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[i].modType[j] == kOutfModType49)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < kShipDefOutfits; i++) {
            if (gShipTypes[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (gOutfits[gShipTypes[ship->type].defOutfID[i]].modType[j]
                            == kOutfModType49)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* SplitTwoLineString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e6d0
char SplitTwoLineString(const unsigned char *src, unsigned char *first,
                  unsigned char *second)
{
    int len;
    int out;
    short i;
    char skipped;
    char afterBreak;

    afterBreak = 0;
    len = src[0];
    out = 1;
    first[0] = 0;
    *second = 0;
    for (i = 1; i <= (short)len; i++) {
        skipped = 0;
        if (i < (short)len && src[i] == '\\'
                && ((int)src[i + 1] == 'n' || (int)src[i + 1] == 'N')) {
            i++;
            skipped = 1;
            afterBreak = 1;
        }
        if (!skipped) {
            if (afterBreak) {
                second[(short)out] = src[i];
                second[0]++;
                out++;
            } else {
                first[i] = src[i];
                first[0]++;
            }
        }
    }
    return afterBreak;
}

/* FindSystemFromStellar (mac _FindSystemFromStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e790
short FindSystemFromStellar(short stellarID)
{
    short i;
    short j;

    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].loaded != 0) {
            for (j = 0; j < kNavSlots; j++) {
                if (gSystems[i].navDefault[j] != -1
                        && stellarID == gSystems[i].navDefault[j])
                    return i;
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        for (j = 0; j < kNavSlots; j++) {
            if (gSystems[i].navDefault[j] != -1
                    && stellarID == gSystems[i].navDefault[j])
                return i;
        }
    }
    return -1;
}

/* BusyIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e840
void BusyIdle(void)
{
}

/* CheckLoMem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e850
void CheckLoMem(void)
{
}

/* PlayerBlessedByGovt (mac _PlayerBlessedByGovt). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e860
char PlayerBlessedByGovt(short govt, short which)
{
    if (govt < 0 || govt >= 256)
        return 0;
    if (which < 0 || which > 1)
        return 0;
    return govtData[govt].blessesPlayer[which];
}

/* ShowWindowNoFlash (mac _ShowWindowNoFlash). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e8b0
void ShowWindowNoFlash(void *arg)
{
    int saved;

    saved = gDisableUpdatesMode;
    gDisableUpdatesMode = 1;
    ShowMacWindow(arg);
    gDisableUpdatesMode = saved;
}

/* DisposeDialogNoFlash (mac _DisposeDialogNoFlash). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e8e0
void DisposeDialogNoFlash(void *dlg)
{
    int saved;

    saved = gDisableUpdatesMode;
    gDisableUpdatesMode = 1;
    DisposeDialog((DialogRecord *)dlg);
    gDisableUpdatesMode = saved;
}

/* AdjustedItemCost (mac _AdjustedItemCost). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e910
long AdjustedItemCost(OutfRecord *outf, ShipRec *ship)
{
    long cost;
    long adjusted;

    cost = outf->cost;
    adjusted = cost;
    if (cost > 0) {
        if (OutfCostScaled(outf)) {
            adjusted = adjusted * ship->mass;
            if (adjusted < cost)
                adjusted = cost;
        }
        return adjusted;
    }
    return 0;
}

/* AdjustedItemMass (mac _AdjustedItemMass). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e950
long AdjustedItemMass(OutfRecord *outf, ShipRec *ship)
{
    short mass;
    long adjusted;

    mass = outf->mass;
    adjusted = mass;
    if (adjusted > 0) {
        if (OutfMassScaled(outf)) {
            adjusted = adjusted * ship->mass;
            adjusted = adjusted * 0.01;
            if (adjusted < mass)
                adjusted = mass;
        }
        return adjusted;
    }
    return adjusted;
}

/* mac _PickEmergeStellar. */

/* PickEmergeStellar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e9e0
short PickEmergeStellar(ShipObj *ship)
{
    int pick;
    short spob;

    if (Rand(3) == 0)
        pick = 1;
    else
        pick = 0;
    spob = SelectRandomStellarDest(ship, pick, 0);
    if (spob >= 0 && spob < 2048 && SpobIsHyperOrWorm(&stellarData[spob]))
        return spob;
    return -1;
}

/* PlayerHasFightersOfOutfitType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ea40
short PlayerHasFightersOfOutfitType(short outfitIndex)
{
    short bayType = -1;
    short i;
    short k;
    short weapon;
    short count;

    for (i = 0; i < kOutfMods; i++) {
        if (gOutfits[outfitIndex].modType[i] == kOutfModAmmo) {
            weapon = gOutfits[outfitIndex].modVal[i];
            if (weapon >= 0 && weapon < 256) {
                if (gWeapons[weapon].guidance == kGuidanceBay)
                    bayType = gWeapons[weapon].ammoType - 128;
            }
        }
        if (bayType != -1)
            break;
    }
    if (bayType == -1)
        return 0;

    count = 0;
    for (k = 1; k < 64; k++) {
        if (shipData[k].active == 0)
            continue;
        if (shipData[k].parentShip != 0)
            continue;
        if (shipData[k].aiType != 5)
            continue;
        if (bayType != shipData[k].type)
            continue;
        if (IsDisabled(&shipData[k]))
            continue;
        count++;
    }
    if (count > 0)
        return count;

    for (k = 0; k < 256; k++) {
        if (gWeapons[k].guidance != kGuidanceBay)
            continue;
        if (shipData[0].weap[k].count <= 0)
            continue;
        if (gWeapons[k].ammoType - 128 != bayType)
            continue;
        if (shipData[0].weap[k].ammo > 0)
            return shipData[0].weap[k].ammo;
    }
    return 0;
}

/* StellarNumHyperLinks. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ebb0
short StellarNumHyperLinks(SpobRecord *spob)
{
    short i;
    int count;

    if (spob != NULL) {
        count = 0;
        for (i = 0; i < kSpobHyperLinks; i++) {
            if (spob->hyperLinks[i] >= 0 && spob->hyperLinks[i] < 2048)
                count = count + 1;
        }
        return count;
    }
    return 0;
}

#define kStarLayers 20


/*
 * The mac twin reaches this byte as settings->[+0xb] through a POINTER; the
 * win build has it as an absolute, so the win's own base is unresolved and a
 * raw name is the honest one. Note 0x005914cc, which would be that base, is
 * already named missionBit (a char *) in setup.c -- one of the two readings
 * is wrong, or missionBit is the settings record's first member.
 */

/* RandomizeStars. STATE: MATCH 100.0%, exe 134 / obj 134, relocregcheck 23 of 23; byte-identical
   pending (bytediff reads the adopted baseline; re-run after regate --update).
   OPEN: none. The unfolded short twenty is `count = kStarLayers; count *= height / 600.0;' --
   the compound assignment reads its own lvalue, which the front end does not substitute. */
// FUNCTION: EVNOVA 0x0046ebf0
void RandomizeStars(void)
{
    short i;
    short count;
    short height;
    short width;

    if (gSystems[shipData->curSystem].murk < 0) {
        ClearStarfield();
    } else if (gSpriteWorld != NULL) {
        width = gSpriteWorld->windRect.right - gSpriteWorld->windRect.left;
        height = gSpriteWorld->windRect.bottom - gSpriteWorld->windRect.top;
        count = kStarLayers;
        count *= height / 600.0;
        for (i = 0; i < kStarLayers; i++) {
            if (i < count) {
                starData[i].live = 1;
                starData[i].frame = Rand(masterStarSprite->numFrames);
                starData[i].pos.x = Rand(width) + shipData->pos.x - width / 2;
                starData[i].pos.y = Rand(height) + shipData->pos.y - height / 2;
                if (parallaxStarsPref)
                    starData[i].depth = Rand(0x23) * 0.01;
                else
                    starData[i].depth = 0;
            } else {
                starData[i].live = 1;
            }
        }
    }
}

/* clears every record's active flag (mac symtab has no name here — likely
   a static in the mac build; keep the address name) */
/* ClearStarfield (mac _ClearStarfield). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ede0
void ClearStarfield(void)
{
    short i;

    for (i = 0; i < kStarLayers; i++)
        starData[i].live = 0;
}

/* ScrollStarfield. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ee50
void ScrollStarfield(float dx, float dy)
{
    int i;
    float depth;

    for (i = 0; (short)i < kStarLayers; i++) {
        if (starData[i].live != 0) {
            depth = starData[i].depth;
            if (depth > 0.0) {
                starData[i].pos.x = starData[i].pos.x - dx * depth;
                starData[i].pos.y = starData[i].pos.y - dy * starData[i].depth;
            }
        }
    }
}

/* pstrcpy (mac _pstrcpy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046eeb0
void pstrcpy(unsigned char *dst, unsigned char *src)
{
    short i;

    dst[0] = src[0];
    for (i = 1; i <= src[0]; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

/*
 * The comparison next to it at 0x0046ef00, and it is odd on purpose: the loop
 * bound is a logical AND of the two length bytes, so it is 1 for any non-empty
 * pair and the body only ever re-checks index 0. The length equality test in
 * front of it already decided the answer.
 */
/* pstrcmp. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ef00
short pstrcmp(const unsigned char *a, const unsigned char *b)
{
    short i;
    char empty;

    if (a[0] != b[0])
        return -1;
    empty = (a[0] == 0);
    for (i = 0; i < (!empty && b[0] != 0); i++) {
        if (a[i] != b[i])
            return -1;
    }
    return 0;
}

/* HideCursorSprite. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ef80
void HideCursorSprite(void)
{
    if (cursorSprite != NULL) {
        SWSetSpriteVisible(cursorSprite, 0);
        cursorAlpha = 0;
    }
}

/* WarpSoundDuration. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046efb0
short WarpSoundDuration(void)
{
    if (doubleTime != 0)
        return jumpSoundLength[1];
    return jumpSoundLength[0];
}

/* StellarsAreIdentical (mac _StellarsAreIdentical). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046efd0
char StellarsAreIdentical(short a, short b)
{
    Str255 nameA;
    Str255 nameB;

    if (a >= 0 && a < kMaxStellars && b >= 0 && b < kMaxStellars) {
        if (a == b)
            return 1;
        if (stellarData[a].pos.h == stellarData[b].pos.h &&
            stellarData[a].pos.v == stellarData[b].pos.v &&
            stellarData[a].name[0] == stellarData[b].name[0]) {
            pstrcpy(nameA, stellarData[a].name);
            pstrcpy(nameB, stellarData[b].name);
            PtoCstr(nameA);
            PtoCstr(nameB);
            if (strcmp((char *)nameA, (char *)nameB) == 0)
                return 1;
        }
    }
    return 0;
}

/* the ships of some governments always read as disabled */

/* ShipIsDerelict. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f100
char ShipIsDerelict(ShipObj *ship)
{
    short govt;

    govt = ship->govt;
    if (govt >= 0 && govt < 256 && (govtData[govt].flags & kGovtAlwaysDisabled) != 0)
        return 1;
    return 0;
}

#define kWeapFlag40 0x40

/* PlayerHasJettisonableCargo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f140
char PlayerHasJettisonableCargo(void)
{
    int i;

    for (i = 0; (short)i < 6; i++) {
        if (shipData[0].cargo[(short)i] > 0)
            return 1;
    }
    for (i = 0; (short)i < kMaxJunks; i++) {
        if (junkData[i].held > 0)
            return 1;
    }
    for (i = 0; (short)i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0
                && currentMissions[i].canAbort != 0)
            return 1;
    }
    return 0;
}

/* AddToPlayerKills (mac _AddToPlayerKills). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f1e0
void AddToPlayerKills(int amount)
{
    if (playerKills < 10000000) {
        if (amount >= 5)
            playerKills += amount * 0.2;
        else
            playerKills += 1;
    } else {
        playerKills = 10000000;
    }
}

/* WeaponBurstInterval. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f270
short WeaponBurstInterval(short weap, short count)
{
    if ((gWeapons[weap].flags & kWeapFlag40) != 0)
        return gWeapons[weap].burstCount;
    return count * gWeapons[weap].burstCount;
}

#define kWeapFlagUsesAmmo   0x40

/* WeaponMaxSimultShots. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f2c0
short WeaponMaxSimultShots(ShipObj *ship, short w)
{
    short count;
    short ammo;
    short type;
    float fuel;

    if ((gWeapons[w].flags & kWeapFlagUsesAmmo) != 0) {
        count = ship->weap[w].count;
        if ((gWeapons[w].flags & kWeapFlagUsesAmmo) != 0
            && (gWeapons[w].flags3 & kWeapAmmoFlag) != 0) {
            if (gWeapons[w].guidance == kGuidanceBay) {
                ammo = ship->weap[w].ammo;
                if (ammo < count)
                    count = ammo;
            } else {
                type = gWeapons[w].ammoType;
                if (type >= 0 && type < 256) {
                    ammo = ship->weap[type].ammo;
                    if (ammo < count)
                        count = ammo;
                } else if (type < -kWeapAmmoFuelBase) {
                    fuel = ship->fuel;
                    if (fuel > 0.0f) {
                        ammo = (fuel
                               / ((abs(type) - kWeapAmmoFuelBase) * 0.1));
                        if (ammo < count)
                            count = ammo;
                    } else {
                        count = 0;
                    }
                }
            }
        }
    } else {
        count = 1;
    }
    return count;
}

/* IonizeShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f3f0
void IonizeShip(ShipObj *ship, WeapRecord *weap, FloatPoint *pos)
{
    short falloff;
    float dist2;
    float rangeSq;

    falloff = weap->ionization;
    if (pos != 0) {
        dist2 = XYFloatSquaredDist(&ship->pos, pos);
        rangeSq = (weap->blastRadius * weap->blastRadius);
        if (dist2 > rangeSq) {
            falloff = 0;
        } else if (dist2 > 0.0) {
            falloff = (falloff * (1.0 - dist2 / rangeSq));
        }
    }
    if (falloff > 0) {
        ship->ionization += falloff;
        if (gSpriteWorld->pixelDepth == 8) {
            if (ship->ionColor == 0)
                ship->ionColor = weap->ionizeColor;
        } else {
            ship->ionColor |= weap->ionizeColor;
        }
    }
}

}
