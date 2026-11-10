// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
#include "cefix.h"
#ifndef MAC_FASTRANDOM_ONLY
#ifdef MACBUILD
#if V111
#include <mach/mach.h>
#endif
/* Util.o's own globals and its mac-only functions (defined below at their places) */
extern void PointToXYShort(Point pt, ShortPoint *xy);
extern void XYShortToPoint(ShortPoint xy, Point *pt);
extern short FindNextStellarInSystem(short cur, short system);
extern void DrawCenteredText(StringPtr s, short left, short right, short y);
extern StringPtr KeyCodeToString(short code);
extern long isPressed(unsigned char *keys, unsigned short k);
extern void ShowHint(void);
extern void InitCluts(void);
extern void SetOrigClut(void);
extern void FlushKeys(void);
extern void InstantFighterRecall(void);
extern void ShowFreeMem(void);
extern void *LoadFrameFromCICN(short id);
extern float FindDecayedShotPower(ShotObj *shot);
extern void DebugInt(long n);
extern Boolean isPowerPC(void);
extern Boolean isG3(void);
extern Boolean isG4(void);
extern Boolean meetsMinimumRequirements(void);
extern short ScanForKeys(void);
extern Boolean QuickKeyCheckPlusEscorts(short code);
extern void DebugCStr(char *s);
extern void DebugStrInt(StringPtr s, short n);
extern void PlaySoundWait(void *snd);
extern Boolean HasMaxFightersOnBoard(short shipType);
extern void UnsplitTwoLineString(const unsigned char *src, unsigned char *dst);
extern void FadeWhiteIn(void);
extern void FadeWhiteOut(void);
/* the AmbrosiaTools DrawTools calls DrawPicHandle makes that burger.h does not declare */
#include "burgerplat.h"
extern struct ITRect Sys2DT_Rect(const Rect *r);
extern long DT_DrawImage(struct ITImage *dst, const struct ITRect *dstRect, struct ITImage *image,
                         struct ITImage *mask, const struct ITTransparent *trans, const struct ITRect *srcRect);
extern void DateToShortString(DateTimeRec date, char *out);
extern void GraphicsEmergencyBail(void);
#endif

#define kRoidYieldBase   0x32
#define kRoidYieldSpread 0x65

EXTERN_C_BEGIN

/* DestroyAsteroid (mac _DestroyAsteroid). STATE: MATCH 100.0%, exe 223 / obj 223 instrs, relocregcheck 23/23 clean.
   OPEN: byte-identity unchecked (bytediff only covers baseline-MATCH rows); the function
   still carries its address name -- no mac twin looked up. see notes */
// FUNCTION: EVNOVA 0x00462550
// MAC: _DestroyAsteroid i386 0x0000adc9 ppc 0x0000d688
void DestroyAsteroid(AsteroidObj *roid)
{

    short expl[2];
    short i;
    short n;
    short spread;

    spread = asteroidType[roid->type].fragCount;
    expl[0] = asteroidType[roid->type].fragType[0];
    expl[1] = asteroidType[roid->type].fragType[1];

    if (asteroidType[roid->type].yieldQty > 0) {
        n = (asteroidType[roid->type].yieldQty * (Rand(kRoidYieldSpread) + kRoidYieldBase) * 0.01);
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
// MAC: _FindNearestShipToPlayer i386 0x0000923f ppc 0x0000b3e8
short FindNearestShipToPlayer(void)
{
    short i;
    short best;
    Boolean hasDetect2;
    float bestDist;
    float dx;
    float dy;
    float dist;
    Boolean hasDetect1;

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
        if (shipData[i].curSystem != shipData[0].curSystem)
            continue;
        if (AIShipIsEmergingFromHypergate(&shipData[i]))
            continue;
        if (!hasDetect1 && (shipClass[shipData[i].type].flags2 & kShipUntargetable) != 0)
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
// MAC: _IsThreatToStellar i386 0x00009ef7 ppc 0x0000c488
Boolean IsThreatToStellar(ShipObj *ship, SpobRecord *spob)
{
    short target;
    Boolean found;

    if (spob->dominated != 0)
        return 0;

    if (ship->index != 0) {
        if (AIIsShipJumpingIn(ship) != 0)
            return 0;
    }

    if (spob->flags2 & kSpobFiresWhenProvoked) {
        if (spob->defenseActive != 0) {
            if (ship->index == 0 || ship->parentShip == 0)
                return 1;
        }
        goto fail;
    }

    if (ship->index == 0 || ship->parentShip == 0) {
        target = -1;
        if (spob->govt != -1) {
            target = spob->govt;
        } else {
            if (systemData[shipData->curSystem].govt != -1)
                target = systemData[shipData->curSystem].govt;
        }

        found = 0;
        if (target != -1) {
            if (playerRecord[shipData->curSystem] < -govtData[target].crimeTol)
                found = 1;
            if (!found) {
                if (govtData[target].flags & kGovtXenophobic) {
                    if (playerRecord[shipData->curSystem] < 0)
                        found = 1;
                } else {
                    if (GovtEnemies(shipClass[shipData->type].inherentGovt, spob->govt))
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

        if (found)
            return 1;
        goto fail;
    }
    if (GovtEnemies(spob->govt, ship->govt))
        return 1;

fail:
    return 0;
}

#define kGateRange2 250000.0

/* FindNearestThreatToPlayer (mac _FindNearestThreatToPlayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00462bd0
// MAC: _FindNearestThreatToPlayer i386 0x000093f9 ppc 0x0000b5f8
short FindNearestThreatToPlayer(void)
{
    short i;
    short best;
    short target;
    Boolean hasDetect1;
    Boolean hasDetect2;
    Boolean valid;
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
        if (shipData[i].curSystem != shipData[0].curSystem)
            continue;
        if (!hasDetect1 && (shipClass[shipData[i].type].flags2 & kShipUntargetable) != 0)
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
// MAC: _FindNearestStellar i386 0x0000276c ppc 0x000031c8
short FindNearestStellar(ShipObj *ship)
{
    short i;
    short stellar;
    short best;
    Boolean inRange;
    float bestDist;
    float dx;
    float dy;
    float dist;

    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kNavSlots; i++) {
        stellar = systemData[ship->curSystem].navDefault[i];
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
// MAC: _ShipStatsToSystemInfo i386 0x0000caee ppc 0x0000fc04
void ShipStatsToSystemInfo(void)
{
    short countBuf[kMaxWeapTypes];
    short ammoBuf[kMaxWeapTypes];
    short slot;
    short i;
    short k;

    for (i = 0; i < kMaxWeapTypes; i++) {
        countBuf[i] = shipData->weap[i].count;
        ammoBuf[i] = shipData->weap[i].ammo;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] <= 0)
            continue;
        if (outfitSystem[i].techLevel >= 0x8000)
            continue;
        if (outfitSystem[i].modType[0] == kOutfModWeapon
                && outfitSystem[i].modVal[0] >= 0 && outfitSystem[i].modVal[0] < kMaxWeapTypes) {
            slot = outfitSystem[i].modVal[0];
            if (shipSystems[i] >= countBuf[slot]) {
                shipSystems[i] = countBuf[slot];
                countBuf[slot] = 0;
            } else {
                countBuf[slot] -= shipSystems[i];
            }
        } else if (outfitSystem[i].modType[0] == kOutfModAmmo
                && outfitSystem[i].modVal[0] >= 0 && outfitSystem[i].modVal[0] < kMaxWeapTypes) {
            slot = outfitSystem[i].modVal[0];
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
                if (outfitSystem[i].techLevel < 0x8000 && outfitSystem[i].modType[0] == kOutfModWeapon
                        && outfitSystem[i].modVal[0] == k) {
                    shipSystems[i] += countBuf[k];
                    countBuf[k] = 0;
                    break;
                }
            }
        }
        if (ammoBuf[k] > 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (outfitSystem[i].techLevel < 0x8000 && outfitSystem[i].modType[0] == kOutfModAmmo
                        && outfitSystem[i].modVal[0] == k) {
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
// MAC: _SystemInfoToShipStats i386 0x0000ca33 ppc 0x0000fb0c
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
        if (outfitSystem[i].techLevel >= kOutfTechAbsent)
            continue;
        if (outfitSystem[i].modType[0] == kOutfModWeapon)
            shipData->weap[outfitSystem[i].modVal[0]].count += shipSystems[i];
        if (outfitSystem[i].modType[0] == kOutfModAmmo)
            shipData->weap[outfitSystem[i].modVal[0]].ammo += shipSystems[i];
    }

    ResetPlayerPrecalcedValues();
}

/* ShipFreeMass. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463470
// MAC: _ShipFreeMass i386 0x0000b462 ppc 0x0000dee4
long ShipFreeMass(void)
{
    short i;
    short type;
    long free;
    long mass;

    type = shipData[0].type;
    if (type < 0 || type >= 768)
        return 0;
    free = shipClass[type].freeMass;
    for (i = 0; i < 512; i++) {
        if (shipSystems[i] > 0) {
            mass = AdjustedItemMass(&outfitSystem[i], &shipClass[shipData[0].type]);
            free -= mass * shipSystems[i];
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
// MAC: _XYShortToFloat i386 0x000028c3 ppc 0x000033ac
FloatPoint XYShortToFloat(ShortPoint pt)
{
    FloatPoint f;

    f.x = pt.h;
    f.y = pt.v;
    return f;
}

/* not linked, not verified against the exe (mac _XYFloatToShort, _VectorToAngle, _CalcShipStopDist) */
// MAC: _XYFloatToShort i386 0x000028ec ppc 0x00003400
ShortPoint XYFloatToShort(FloatPoint f)
{
    ShortPoint pt;

    pt.h = f.x;
    pt.v = f.y;
    return pt;
}

// MAC: _VectorToAngle i386 0x0000290b ppc 0x00003444
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

// MAC: _CalcShipStopDist i386 0x0000298a ppc 0x000034dc
float CalcShipStopDist(void)
{
    return 20000.0f;
}

/* ShipShieldCapacity (mac _ShipShieldCapacity). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463550
// MAC: _ShipShieldCapacity i386 0x00002995 ppc 0x000034e8
float ShipShieldCapacity(ShipObj *ship)
{
    long i;
    float cap;
    short j;

    cap = shipClass[ship->type].shield;
    if (ship->index == 0) {
        if (playerShieldCapacity >= 0.0f) {
            cap = playerShieldCapacity;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModShieldBoost)
                            cap += outfitSystem[i].modVal[j] * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerShieldCapacity = cap;
        }
    } else {
        if (ship->persType != -1) {
            if (personData[ship->persType].shieldMod > 0.0f)
                cap = cap * personData[ship->persType].shieldMod;
        }
    }
    if (ship->index != 0 && ship->aiType == 5)
        cap *= 1.333;
    return cap;
}

#define kOutfModShieldRech 5

/* ShipShieldRechargeRate (mac _ShipShieldRechargeRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463680
// MAC: _ShipShieldRechargeRate i386 0x00002abb ppc 0x0000367c
float ShipShieldRechargeRate(ShipObj *ship)
{
    short i;
    float cap;
    short j;
    int tmp;

    cap = shipClass[tmp = ship->type].shieldRech;
    if (ship->index == 0) {
        if (playerShieldRechargeRate >= 0.0f) {
            cap = playerShieldRechargeRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModShieldRech)
                            cap += (float)outfitSystem[i].modVal[j] * 0.001
                                   * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerShieldRechargeRate = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (ship->index != 0 && ship->aiType == 5)
        cap *= 1.333;
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
// MAC: _ShipArmorCapacity i386 0x00002bc0 ppc 0x000037e4
float ShipArmorCapacity(ShipObj *ship)
{
    short i;
    float cap;
    short j;
    short pers;

    cap = shipClass[ship->type].armor;
    if (ship->index == 0) {
        if (playerArmorCapacity >= 0.0f) {
            cap = playerArmorCapacity;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModArmor)
                            cap += outfitSystem[i].modVal[j] * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerArmorCapacity = cap;
        }
    } else {
        pers = ship->persType;
        if (pers >= 0 && pers < kMaxPersons) {
            if (personData[pers].shieldMod > 0.0f)
                cap = cap * personData[pers].shieldMod;
        }
    }
    if (ship->index != 0 && ship->aiType == 5)
        cap *= 1.333;
    return cap;
}

#define kOutfModArmorRech 0x1d

/* ShipArmorRechargeRate (mac _ShipArmorRechargeRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004638e0
// MAC: _ShipArmorRechargeRate i386 0x00002e0e ppc 0x00003acc
float ShipArmorRechargeRate(ShipObj *ship)
{
    short i;
    float cap;
    short j;

    if (IsDisabled(ship))
        return 0.0f;
    cap = shipClass[ship->type].armorRech;
    if (ship->index == 0) {
        if (playerArmorRechargeRate >= 0.0f) {
            cap = playerArmorRechargeRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModArmorRech)
                            cap += (float)outfitSystem[i].modVal[j] * 0.001
                                   * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerArmorRechargeRate = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (ship->index == 0 && cheatsActive != 0)
        cap *= 50.0f;
    return cap;
}

#define kOutfModType12  0xc
#define kMaxFuel        32000

/* ShipFuelCapacity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463a20
// MAC: _ShipFuelCapacity i386 0x00002f21 ppc 0x00003c74
short ShipFuelCapacity(ShipObj *ship)
{
    short i;
    short j;
    long total;

    total = shipClass[ship->type].fuel;
    if (ship->index == 0) {
        if (playerFuelCapacity >= 0.0f) {
            total = playerFuelCapacity;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType12)
                            total += outfitSystem[i].modVal[j]
                                     * shipSystems[i];
                    }
                }
            }
            if (total < 0)
                total = 0;
            if (total > kMaxFuel)
                total = kMaxFuel;
            playerFuelCapacity = total;
        }
    }
    return total;
}

#define kOutfModType18  0x12

/* ShipFuelGenRate (mac _ShipFuelGenRate). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463b30
// MAC: _ShipFuelGenRate i386 0x00002ffb ppc 0x00003d88
Boolean ShipFuelGenRate(ShipObj *ship, float *rate)
{
    short i;
    short j;

    if (ship->fuelRegenState < 0) {
        ship->fuelRegenState = 0;
        ship->fuelRegenRate = 0;
        if (shipClass[ship->type].fuelRegen > 0 &&
            ((shipClass[ship->type].flags & kShipFuelRegen) != 0 ||
             ship->index != 0)) {
            ship->fuelRegenState = 1;
            ship->fuelRegenRate = 1.0 / (float)shipClass[ship->type].fuelRegen;
        }
        if (ship->index == 0) {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType18) {
                            ship->fuelRegenState = 1;
                            ship->fuelRegenRate += (float)shipSystems[i] * (1.0 / (float)outfitSystem[i].modVal[j]);
                        }
                    }
                }
            }
        } else {
            for (i = 0; i < 8; i++) {
                if (shipClass[ship->type].defOutfCount[i] > 0) {
                    short id = shipClass[ship->type].defOutfID[i];

                    if (id >= 0 && id < 512) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (outfitSystem[id].modType[j] == kOutfModType18) {
                                ship->fuelRegenState = 1;
                                ship->fuelRegenRate += (float)shipClass[ship->type].defOutfCount[i]
                                    * (1.0 / (float)outfitSystem[id].modVal[j]);
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

/* SuitableMissileType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463dc0
// MAC: _SuitableMissileType i386 0x000034e1 ppc 0x00004400
Boolean SuitableMissileType(short weap, ShipObj *ship)
{
    short turns;

    turns = ShipTurnRate(ship);
    if ((weaponStats[weap].flags & kWeapNoFastTargets) != 0)
        if (turns > 3)
            return 0;
    if (weaponStats[weap].guidedTurn <= 2.0f)
        if (turns > 3)
            return 0;
    return 1;
}

#define kOutfModSpeed    8

#define kOutfModTurn    9

/* ShipTurnRate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00463e70
// MAC: _ShipTurnRate i386 0x000032f7 ppc 0x000041b8
float ShipTurnRate(ShipObj *ship)
{
    short i;
    float v;
    short j;
    float ratio;

    v = shipClass[ship->type].turnRate;
    if (ship->index == 0) {
        if (playerTurnRate >= 0.0f) {
            v = playerTurnRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModTurn)
                            v += outfitSystem[i].modVal[j]
                                 * shipSystems[i] * 0.01;
                    }
                }
            }
            if (v < 1.0f && shipClass[ship->type].turnRate > 0.0f)
                v = 1.0f;
            playerTurnRate = v;
        }
    } else {
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
    }
    if (ship->persType == kPersCaptainHector) {
        if (v < 6.0f)
            v += 1.0f;
    }
    if (shipClass[ship->type].turnRate >= 1.0f && v < 1.0f)
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
// MAC: _ShipAccelRate i386 0x0000353b ppc 0x00004490
float ShipAccelRate(ShipObj *ship)
{
    short i;
    float v;
    short j;
    float ratio;

    v = shipClass[ship->type].accel;
    if (ship->index == 0) {
        if (playerAccelRate >= 0.0f) {
            v = playerAccelRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModAccel)
                            v += (float)((float)(outfitSystem[i].modVal[j] / 10000.0)
                                 * shipSystems[i]);
                    }
                }
            }
            if (v < 0.0f)
                v = 0.0f;
            playerAccelRate = v;
        }
        v *= 2.0;
    } else {
        if ((shipClass[ship->type].flags & kShipPlanetType) != 0)
            return 0.0f;
        v *= ship->skill;
        if (ship->govt != -1)
            v *= govtData[ship->govt].skillMult;
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
        else if (ship->parentShip == 0)
            v *= 2.0;
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
// MAC: _ShipMaxSpeed i386 0x000036f2 ppc 0x000046c4
float ShipMaxSpeed(ShipObj *ship)
{
    short i;
    float v;
    short j;

    v = shipClass[ship->type].maxSpeed;
    if (ship->index == 0) {
        if (playerMaxSpeed >= 0.0f) {
            v = playerMaxSpeed;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModSpeed)
                            v += (float)((float)(outfitSystem[i].modVal[j] / 100.0)
                                 * shipSystems[i]);
                    }
                }
            }
            if (v < 0.0f)
                v = 0.0f;
            playerMaxSpeed = v;
        }
    } else {
        if ((shipClass[ship->type].flags & kShipPlanetType) != 0)
            return 0.0f;
        v *= ship->skill;
        if (ship->govt != -1)
            v *= govtData[ship->govt].skillMult;
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
    }
    if (ship->persType == kPersCaptainHector)
        v *= 2.0;
    if (ship->index == 0 || ship->parentShip == 0) {
        if (strictPlay == 0)
            v *= 1.5;
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

/* Guidance 99 is the carried-ship type. */

#define kOutfModBay      11

/* HasEscapePod (mac _HasEscapePod). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004644a0
// MAC: _HasEscapePod i386 0x0000b83d ppc 0x0000e3e0
Boolean HasEscapePod(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModBay)
                        return 1;
                }
            }
        }
    }
    if (HasEscapeShip(ship))
        return 1;
    return 0;
}

#ifdef MACBUILD
/* GraphicsEmergencyBail (mac _GraphicsEmergencyBail, mac-only). STATE: see the mac gate. */
// MAC: _GraphicsEmergencyBail i386 0x0000b8f8 ppc 0x0000e4d0
void GraphicsEmergencyBail(void)
{
    Str255 msg;

    ShowTheCursor();
    FadeScreenIn();
    GetIndString(msg, kAlertStrList, 48);
    EVNoteAlert(msg);
    ShowTheMenuBar();
    ST_Close();
    ExitToShell();
}
#endif

/* HasEscapeShip (mac _HasEscapeShip). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464520
// MAC: _HasEscapeShip i386 0x0000386e ppc 0x000048c4
Boolean HasEscapeShip(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < 256; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            ship->weap[i].ammo > 0) {
            type = weaponStats[i].ammoType - 128;
            if ((shipClass[type].flags & kShipEscapeShip) != 0)
                return 1;
        }
    }
    return 0;
}

/* EscapeShipType (mac _EscapeShipType). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464590
// MAC: _EscapeShipType i386 0x000038d0 ppc 0x00004938
short EscapeShipType(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < 256; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            ship->weap[i].ammo > 0) {
            type = weaponStats[i].ammoType - 128;
            if ((shipClass[type].flags & kShipEscapeShip) != 0)
                return type;
        }
    }
    return -1;
}

/* EscapeShipWeapType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464600
// MAC: _EscapeShipWeapType i386 0x00003936 ppc 0x000049ac
short EscapeShipWeapType(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay && ship->weap[i].ammo > 0) {
            type = weaponStats[i].ammoType - kFirstResourceID;
            if ((shipClass[type].flags & kShipEscapeShip) != 0)
                return i;
        }
    }
    return -1;
}

/* KeyCarriedShipTypeOnboard (mac _KeyCarriedShipTypeOnboard). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464670
// MAC: _KeyCarriedShipTypeOnboard i386 0x0000399c ppc 0x00004a28
Boolean KeyCarriedShipTypeOnboard(ShipObj *ship)
{
    short i;
    short escort;
    short bay;

    escort = shipClass[ship->type].keyCarried;
    if (escort == -1)
        return 0;
    for (i = 0; i < 256; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            ship->weap[i].count > 0 && ship->weap[i].ammo > 0) {
            bay = (weaponStats[i].ammoType - 128);
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
// MAC: _HasAutoEject i386 0x00003a14 ppc 0x00004ab4
Boolean HasAutoEject(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType20)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* HasAfterburner (mac _HasAfterburner). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464760
// MAC: _HasAfterburner i386 0x00003a6d ppc 0x00004b24
Boolean HasAfterburner(ShipObj *ship)
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
                        if (outfitSystem[i].modType[j] == kOutfModType15) {
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

/* ShipECM (mac _ShipECM). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464810
// MAC: _ShipECM i386 0x00003aff ppc 0x00004bdc
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

    if (shipClass[ship->type].inherentGovtB >= 0 &&
        shipClass[ship->type].inherentGovtB < 256)
        ship->jamming[kind] =
            govtData[shipClass[ship->type].inherentGovtB].inhJam[kind];
    else
        ship->jamming[kind] = 0;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType33 + kind)
                        ship->jamming[kind] += outfitSystem[i].modVal[j];
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType33 + kind)
                        ship->jamming[kind] +=
                            outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j];
                }
            }
        }
        if (ship->govt >= 0 && ship->govt < 256 &&
            (govtData[ship->govt].flags & kGovtFreighterHalfJam) != 0)
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
// MAC: _ShipVisibleToShip i386 0x0000971c ppc 0x0000ba14
Boolean ShipVisibleToShip(ShipObj *ship, ShipObj *viewer)
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
        if (viewer->seesCloaked[0] == 1) {
            d = FloatAbs(viewer->pos.x - ship->pos.x);
            if (d <= kDetectRange) {
                d = FloatAbs(viewer->pos.y - ship->pos.y);
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
// MAC: _HasCloak i386 0x000095f2 ppc 0x0000b85c
Boolean HasCloak(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17)
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
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
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
// MAC: _HasAreaCloak i386 0x00003d49 ppc 0x00004e84
Boolean HasAreaCloak(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasAreaCloak != -1) {
            if (playerHasAreaCloak == 1)
                return 1;
        } else {
            playerHasAreaCloak = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType17 &&
                            (outfitSystem[i].modVal[j] & kOutfModVal17Bit12) != 0) {
                            playerHasAreaCloak = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _CloakFuelUsage i386 0x00003e25 ppc 0x00004fb8
short CloakFuelUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModNibblePair)
                        return (outfitSystem[i].modVal[j] >> 4) & 0xf;
                }
            }
        }
    }
    return 0;
}

/* CloakZeroesShields (mac _CloakZeroesShields). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00464e30
// MAC: _CloakZeroesShields i386 0x00003e9d ppc 0x00005040
Boolean CloakZeroesShields(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17) {
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit2) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17) {
                        if ((outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _ShipDecloaksWhenHit i386 0x00003f68 ppc 0x00005180
Boolean ShipDecloaksWhenHit(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17) {
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit3) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17) {
                        if ((outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _CloakShieldUsage i386 0x00004033 ppc 0x000052c0
short CloakShieldUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17)
                        return (outfitSystem[i].modVal[j] >> 8) & 0xf;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17)
                        return (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] >>
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
// MAC: _ShipCloakVisibleOnRadar i386 0x000040f8 ppc 0x000053f8
Boolean ShipCloakVisibleOnRadar(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17 &&
                        (outfitSystem[i].modVal[j] & kOutfModVal17Bit1) != 0)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _ShipCanSeeCloakedShips i386 0x000041a9 ppc 0x000054f4
Boolean ShipCanSeeCloakedShips(ShipObj *ship, short which)
{
    short i;
    short j;

    if (ship->persType == kPersTypeAlways)
        return 1;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType30) {
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit1) != 0 &&
                            which == 0)
                            return 1;
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit0) != 0 &&
                            which == 1)
                            return 1;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType30) {
                        if (which == 0 &&
                            (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                                kOutfModVal30Bit1) != 0)
                            return 1;
                        if (which == 1 &&
                            (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _HasDensityScanner i386 0x000042c0 ppc 0x00005634
Boolean HasDensityScanner(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasDensityScanner >= 0) {
            if (playerHasDensityScanner != 0)
                return 1;
        } else {
            playerHasDensityScanner = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType13) {
                            playerHasDensityScanner = 1;
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
// MAC: _HasIFF i386 0x00004348 ppc 0x000056d4
Boolean HasIFF(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasIFF >= 0) {
            if (playerHasIFF != 0)
                return 1;
        } else {
            playerHasIFF = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType14) {
                            playerHasIFF = 1;
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
// MAC: _ShipHyperTransitTime i386 0x000043d0 ppc 0x00005774
short ShipHyperTransitTime(ShipObj *ship)
{
    short i;
    short j;
    short count;

    count = 1;
    if (shipClass[ship->type].mass >= 100)
        count++;
    if (shipClass[ship->type].mass >= 200)
        count++;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType22)
                        count += outfitSystem[i].modVal[j] * shipSystems[i];
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
// MAC: _ShipHyperSafeDist i386 0x00004485 ppc 0x00005830
long ShipHyperSafeDist(ShipObj *ship)
{
    short i;
    short j;
    long range;

    range = 1000;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType23)
                        range += outfitSystem[i].modVal[j] * shipSystems[i];
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
// MAC: _HasMaxOfItem i386 0x00004512 ppc 0x000058b4
Boolean HasMaxOfItem(short index, short *outBound, short *outCap)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    short sumOwned;
    short sumFlag1;
    short sumFlag2;
    short j;
    short k;
    short limit;
    short ammo;
#else
    short limit;
    short maxCount;
    short j;
    short k;
    short sumFlag1;
    short sumFlag2;
    short sumOwned;
#endif

    if (index < 0 || index >= 0x200) {
        if (outBound)
            *outBound = 0;
        if (outCap)
            *outCap = 0;
        return 1;
    }

#ifdef MAC1010 /* 1.0.10 spelling */
    limit = outfitSystem[index].maxCount;
    if (outCap)
        *outCap = limit;

    if (outfitSystem[index].modType[0] == kOutfModAmmo) {
        if (weaponStats[outfitSystem[index].modVal[0]].maxAmmo > 0) {
            ammo = weaponStats[outfitSystem[index].modVal[0]].maxAmmo * shipData->weap[outfitSystem[index].modVal[0]].count;
            if (outCap && ammo < *outCap)
                *outCap = ammo;
            if (shipSystems[index] >= ammo) {
                if (outBound)
                    *outBound = ammo;
                return 1;
            }
        }
    }
#else
    maxCount = outfitSystem[index].maxCount;
    if (outCap)
        *outCap = maxCount;

    if (outfitSystem[index].modType[0] == kOutfModAmmo) {
        if (weaponStats[outfitSystem[index].modVal[0]].maxAmmo > 0) {
            limit = weaponStats[outfitSystem[index].modVal[0]].maxAmmo * shipData->weap[outfitSystem[index].modVal[0]].count;
            if (outCap && limit < *outCap)
                *outCap = limit;
            if (shipSystems[index] >= limit) {
                if (outBound)
                    *outBound = limit;
                return 1;
            }
        }
    }
#endif

    sumOwned = 0;
    for (j = 0; j < 0x200; j++) {
        if (shipSystems[j] > 0) {
            for (k = 0; k < kOutfMods; k++) {
                if (outfitSystem[j].modType[k] == kModTypeMaxOwned &&
                    outfitSystem[j].modVal[k] == index + 128)
                    sumOwned += shipSystems[j];
            }
        }
    }
    if (sumOwned < 1)
        sumOwned = 1;
    limit = sumOwned * outfitSystem[index].maxCount;
    if (outCap && (limit < *outCap || sumOwned > 1))
        *outCap = limit;
    if (shipSystems[index] >= limit) {
        if (outBound)
            *outBound = limit;
        return 1;
    }

    sumFlag1 = sumFlag2 = 0;
    for (j = 0; j < 0x200; j++) {
        if (shipSystems[j] > 0) {
            if (outfitSystem[j].flags & kOutfFixedGun)
                sumFlag1 += shipSystems[j];
            if (outfitSystem[j].flags & kOutfTurret)
                sumFlag2 += shipSystems[j];
        }
    }

    if (outfitSystem[index].flags & kOutfFixedGun) {
        limit = shipClass[shipData->type].maxGuns;
        for (j = 0; j < 0x200; j++) {
            if (shipSystems[j] > 0) {
                for (k = 0; k < kOutfMods; k++) {
                    if (outfitSystem[j].modType[k] == kModTypeGunLimit) {
                        limit += outfitSystem[j].modVal[k];
                        break;
                    }
                }
            }
        }
        if (outCap && limit < *outCap)
            *outCap = limit;
        if (limit <= 0) {
            if (outBound)
                *outBound = 0;
            return 1;
        }
        if (sumFlag1 >= limit) {
            if (outBound)
                *outBound = limit;
            return 1;
        }
    }

    if (outfitSystem[index].flags & kOutfTurret) {
        limit = shipClass[shipData->type].maxTurrets;
        for (j = 0; j < 0x200; j++) {
            if (shipSystems[j] > 0) {
                for (k = 0; k < kOutfMods; k++) {
                    if (outfitSystem[j].modType[k] == kModTypeTurretLimit) {
                        limit += outfitSystem[j].modVal[k];
                        break;
                    }
                }
            }
        }
        if (outCap && limit < *outCap)
            *outCap = limit;
        if (limit <= 0) {
            if (outBound)
                *outBound = 0;
            return 1;
        }
        if (sumFlag2 >= limit) {
            if (outBound)
                *outBound = limit;
            return 1;
        }
    }

    if (outBound)
        *outBound = shipSystems[index];
    return 0;
}

/* DrawCommaNumber. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465af0
// MAC: _DrawCommaNumber i386 0x00004895 ppc 0x00005cb0
void DrawCommaNumber(long value)
{
    long group;
    unsigned char str[256];

    if (value < 1000) {
        NumToString(value, str);
        DrawString(str);
    } else if (value < 1000000) {
        NumToString(value / 1000, str);
        DrawString(str);
        DrawString("\p,");
        if (value % 1000 < 100)
            DrawString("\p0");
        if (value % 1000 < 10)
            DrawString("\p0");
        NumToString(value % 1000, str);
        DrawString(str);
    } else {
        NumToString(value / 1000000, str);
        DrawString(str);
        DrawString("\p.");
        group = value % 1000000;
        NumToString(group / 10000, str);
        if (group / 10000 < 10)
            DrawString("\p0");
        DrawString(str);
        DrawString("\pM");
    }
}

/* StrcatCommaNumber. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465c10
// MAC: _StrcatCommaNumber i386 0x00008820 ppc 0x0000a8e0
void StrcatCommaNumber(long value, char *dst)
{
    long group;
    unsigned char str[256];

    if (value < 1000) {
        NumToString(value, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
    } else if (value < 1000000) {
        NumToString(value / 1000, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
        strcat(dst, ",");
        if (value % 1000 < 100)
            strcat(dst, "0");
        if (value % 1000 < 10)
            strcat(dst, "0");
        NumToString(value % 1000, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
    } else {
        NumToString(value / 1000000, str);
        PtoCstr(str);
        strcat(dst, (char *)str);
        strcat(dst, ".");
        group = value % 1000000;
        NumToString(group / 10000, str);
        if (group / 10000 < 10)
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
// MAC: _StrcatOrdinalNumber i386 0x00008ab4 ppc 0x0000ab98
void StrcatOrdinalNumber(short n, char *dst, Boolean capitalize)
{
    unsigned char numStr[256];
    unsigned char word[256];

    if (n >= 1 && n <= 10) {
        strcpy((char *)word, "");
        StrcatIndCString((char *)word, kOrdinalsSTR, n + kOrdinalsBase);
    } else {
        NumToString(n, numStr);
        PtoCstr(numStr);
        strcpy((char *)word, (char *)numStr);
    }
    if (capitalize != 0)
        word[0] = toupper(word[0]);
    strcat(dst, (char *)word);
}

#ifdef MACBUILD
/* DateToShortString (mac _DateToShortString, mac-only). STATE: see the mac gate. */
// MAC: _DateToShortString i386 0x00008b79 ppc 0x0000ac68
void DateToShortString(DateTimeRec date, char *out)
{
    Str255 str;

    strcpy(out, datePrefix);
    NumToString(date.month, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, "/");
    NumToString(date.day, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, "/");
    NumToString(date.year, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, dateSuffix);
}
#endif

/* CanBuyGoods (mac _CanBuyGoods). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465e50
// MAC: _CanBuyGoods i386 0x0000ccec ppc 0x0000fe3c
Boolean CanBuyGoods(short item)
{
    if (shipData->credits < tradePrice[item])
        return 0;
    if (TotalCargo(shipData) >= TotalFleetHolds())
        return 0;
    return 1;
}

/* CanSellGoods (mac _CanSellGoods). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00465ea0
// MAC: _CanSellGoods i386 0x00004a94 ppc 0x00005efc
Boolean CanSellGoods(short item)
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
// MAC: _ColorCodeShip i386 0x00004ae8 ppc 0x00005f6c
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
        *color = rgbDarkGrey;
    } else if (IsThreatToPlayer(ship)) {
        color->red = 0xffff;
        color->green = 0;
        color->blue = 0;
    } else {
        parent = ship->parentShip;
        if (parent == 0 && ship->defenseStellar == -1) {
            *color = brightGreen;
        } else if (parent != -1) {
            if (shipData[parent].parentShip == 0) {
                *color = brightGreen;
            } else {
                color->red = 0;
                color->green = 0;
                color->blue = 0xffff;
            }
        } else {
            color->red = 0;
            color->green = 0;
            color->blue = 0xffff;
        }
    }
}

/* ColorCodeStellar (mac _ColorCodeStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466030
// MAC: _ColorCodeStellar i386 0x0000aa26 ppc 0x0000d1ac
void ColorCodeStellar(SpobRecord *spob, RGBColor *color)
{
    Boolean uninhabited;

    uninhabited = SpobUninhabited(spob);
    if (!StellarIsLandable(spob)) {
        *color = rgbMedGrey;
    } else if (uninhabited) {
        *color = rgbMedGrey;
    } else if (spob->flags2 & kSpobHypergate) {
        if (spob->govt != -1 && PlayerBlessedByGovt(spob->govt, 1)) {
            color->red = 0xffff;
            color->green = 0xffff;
            color->blue = 0;
        } else {
            if (spob->minStatus == 0x7fff
                    || (playerRecord[spob->system] < spob->minStatus
                        && spob->minStatus > -0x7fff)) {
                if (playerRecord[spob->system] >= 0) {
                    color->red = 0xffff;
                    color->green = 0x6666;
                    color->blue = 0;
                } else {
                    color->red = 0xffff;
                    color->green = 0;
                    color->blue = 0;
                }
            } else {
                color->red = 0xffff;
                color->green = 0xffff;
                color->blue = 0;
            }
        }
    } else if (spob->flags2 & kSpobWormhole) {
        color->red = 0;
        color->green = 0x55f0;
        color->blue = 0x55f0;
    } else if (spob->dominated == 0) {
        if (spob->govt != -1 && PlayerBlessedByGovt(spob->govt, 1)) {
            color->red = 0xffff;
            color->green = 0xffff;
            color->blue = 0;
        } else {
            if (spob->minStatus == 0x7fff
                    || (playerRecord[spob->system] < spob->minStatus
                        && spob->minStatus > -0x7fff)) {
                if (playerRecord[spob->system] >= 0) {
                    color->red = 0xffff;
                    color->green = 0x6666;
                    color->blue = 0;
                } else {
                    color->red = 0xffff;
                    color->green = 0;
                    color->blue = 0;
                }
            } else {
                color->red = 0xffff;
                color->green = 0xffff;
                color->blue = 0;
            }
        }
    } else {
        color->red = 0;
        color->green = 0xffff;
        color->blue = 0;
    }
}

/* ColorCodeSystem (mac _ColorCodeSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466260
// MAC: _ColorCodeSystem i386 0x0000d672 ppc 0x00010b04
void ColorCodeSystem(short sys, RGBColor *color)
{
    short i;
    short special;
    short friendly;
    short warned;
    short hostile;
    short spob;
    short first;

    first = FindFirstCoLocatedSystem(sys);
    sys = FindActiveCoLocatedSystem(sys);
    *color = rgbDarkGrey;
    if (sys < 0 || sys >= 2048 || first < 0 || first >= 2048)
        return;
    if (systemData[first].exploreLevel > 0) {
        if (SystemIsInhabited(sys)) {
            special = 0;
            friendly = 0;
            warned = 0;
            hostile = 0;
            for (i = 0; i < kNavSlots; i++) {
                spob = systemData[sys].navDefault[i];
                if (spob != -1 && stellarData[spob].loaded != 0 &&
                    !SpobIsHyperOrWorm(&stellarData[spob]) &&
                    !SpobUninhabited(&stellarData[spob]) &&
                    StellarIsLandable(&stellarData[spob])) {
                    if (stellarData[spob].dominated != 0) {
                        special++;
                    } else if (stellarData[spob].govt != -1 &&
                               PlayerBlessedByGovt(stellarData[spob].govt, 1)) {
                        friendly++;
                    } else if (stellarData[spob].minStatus == 0x7fff ||
                               (playerRecord[first] < stellarData[spob].minStatus &&
                                stellarData[spob].minStatus > -0x7fff)) {
                        if (playerRecord[first] >= 0)
                            warned++;
                        else
                            hostile++;
                    } else {
                        friendly++;
                    }
                }
            }
            if (special > 0) {
                *color = brightGreen;
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
                *color = rgbLightGrey;
            }
        } else {
            *color = rgbLightGrey;
        }
    } else {
        *color = rgbDarkGrey;
    }
}

/* fills `frame` with the picture's bounds (name invented) */

/* CenterPict. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466540
// MAC: _CenterPict i386 0x00008652 ppc 0x0000a6c8
#if !defined(MACBUILD) || !V111
void CenterPict(void *pict, Rect *bounds)
#include "../engine/centerpict.inc"
#endif

#ifdef MACBUILD
/* KeyCodeToString (mac _KeyCodeToString, mac-only). STATE: see the mac gate. */
// MAC: _KeyCodeToString i386 0x00004ca0 ppc 0x00006190
StringPtr KeyCodeToString(short code)
{
    Str255 name;

    GetIndString(name, kKeyNameStrList, code + 1);
    /* BUG: returns the address of a dead stack buffer */
    return name[0] ? name : "\pUnknown";
}
#endif

/* MSL <ctype.h> inlines tolower's table lookup; the exe calls the library
   function (0x004d6230), so declare it without the header. */

/* CullNameString (mac _CullNameString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004665f0
// MAC: _CullNameString i386 0x0000dbe2 ppc 0x00011150
void CullNameString(unsigned char *name)
{
    short skip;
    short len;
    short i;
    short count;
    unsigned char work[256];
    unsigned char culled[256];

    pstrcpy(work, name);
    PtoCstr(work);
    skip = 0;
    len = strlen((char *)work);
    count = len;
    if (tolower(work[0]) == 't' && tolower(work[1]) == 'h'
            && tolower(work[2]) == 'e') {
        if (work[3] == ' ') {
            skip += 4;
            count -= 4;
        }
    }
    for (i = 0; i < count; i++)
        culled[i] = work[i + skip];
    culled[i] = 0;
    CtoPstr((char *)culled);
    pstrcpy(name, culled);
}

#ifdef MACBUILD
/* DebugCStr (mac _DebugCStr, mac-only). STATE: see the mac gate. */
// MAC: _DebugCStr i386 0x0000dd2d ppc 0x000112f4
void DebugCStr(char *s)
{
    Str255 p;

    strcpy((char *)p, s);
    CtoPstr((char *)p);
    DebugStr(p);
}

/* DebugStrInt (mac _DebugStrInt, mac-only). STATE: see the mac gate. */
// MAC: _DebugStrInt i386 0x0000dd82 ppc 0x00011368
void DebugStrInt(StringPtr s, short n)
{
    Str255 str;
    Str255 num;

    pstrcpy(str, s);
    PtoCstr(str);
    NumToString(n, num);
    PtoCstr(num);
    strcat((char *)str, (char *)num);
    CtoPstr((char *)str);
    DebugStr(str);
}
#endif

#ifdef MAC1010
/* ScanForKeys (mac _ScanForKeys, mac-only): 1.0.10 defines it where win has FirstKeyDown.
   STATE: see the mac gate. */
short ScanForKeys(void)
{
    KeyMap keys;
    short i;
    short word;
    short bit;

    GetKeys(keys);
    for (i = 0; i <= 127; i++) {
        word = (i >> 4) & 0xf;
        bit = i & 0xf;
        if (i != 0x1a && i != 0x1b && i != 0x1c && i != 0x1d && i != 0x77 && i != 0x31
            && i != 0x72 && i != 0x70 && i != 0x7e && i != 0x6b && i != 0x68 && i != 0x32
            && (((unsigned short *)keys)[word] & (1 << bit)))
            return i;
    }
    return -1;
}
#endif

#ifndef MACBUILD
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
#endif

#if !defined(MACBUILD) || !V111
/* KeyPrefsToSlotTable. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466850
void KeyPrefsToSlotTable(void)
{
    newKeyPrefs[0] = keyPrefs.accelerate;
    newKeyPrefs[1] = keyPrefs.reverse;
    newKeyPrefs[2] = keyPrefs.turnRight;
    newKeyPrefs[3] = keyPrefs.turnLeft;
    newKeyPrefs[4] = keyPrefs.afterburner;
    newKeyPrefs[5] = keyPrefs.faceTarget;
    newKeyPrefs[6] = keyPrefs.hyperspace;
    newKeyPrefs[7] = keyPrefs.nextSystem;
    newKeyPrefs[8] = keyPrefs.jump;
    newKeyPrefs[9] = keyPrefs.clearTarget;
    newKeyPrefs[10] = keyPrefs.hail;
    newKeyPrefs[11] = keyPrefs.land;
    newKeyPrefs[12] = keyPrefs.firePrimary;
    newKeyPrefs[13] = keyPrefs.fireSecondary;
    newKeyPrefs[14] = keyPrefs.selectSecondary;
    newKeyPrefs[15] = keyPrefs.deselectSecondary;
    newKeyPrefs[16] = keyPrefs.targetNext;
    newKeyPrefs[17] = keyPrefs.targetNearest;
    newKeyPrefs[18] = keyPrefs.escortMenu;
    newKeyPrefs[19] = keyPrefs.escortOrder[0];
    newKeyPrefs[20] = keyPrefs.escortOrder[1];
    newKeyPrefs[21] = keyPrefs.escortOrder[2];
    newKeyPrefs[22] = keyPrefs.escortOrder[3];
    newKeyPrefs[23] = keyPrefs.pause;
    newKeyPrefs[24] = keyPrefs.dismiss;
    newKeyPrefs[25] = keyPrefs.board;
    newKeyPrefs[26] = keyPrefs.jettison;
    newKeyPrefs[27] = keyPrefs.eject;
    newKeyPrefs[28] = keyPrefs.selfDestruct;
    newKeyPrefs[29] = keyPrefs.cloak;
    newKeyPrefs[30] = keyPrefs.map;
    newKeyPrefs[31] = keyPrefs.playerInfo;
    newKeyPrefs[32] = keyPrefs.missions;
    newKeyPrefs[33] = keyPrefs.toggleFPS;
}

/* SlotTableToKeyPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466a30
void SlotTableToKeyPrefs(void)
{
    keyPrefs.accelerate = newKeyPrefs[0];
    keyPrefs.reverse = newKeyPrefs[1];
    keyPrefs.turnRight = newKeyPrefs[2];
    keyPrefs.turnLeft = newKeyPrefs[3];
    keyPrefs.afterburner = newKeyPrefs[4];
    keyPrefs.faceTarget = newKeyPrefs[5];
    keyPrefs.hyperspace = newKeyPrefs[6];
    keyPrefs.nextSystem = newKeyPrefs[7];
    keyPrefs.jump = newKeyPrefs[8];
    keyPrefs.clearTarget = newKeyPrefs[9];
    keyPrefs.hail = newKeyPrefs[10];
    keyPrefs.land = newKeyPrefs[11];
    keyPrefs.firePrimary = newKeyPrefs[12];
    keyPrefs.fireSecondary = newKeyPrefs[13];
    keyPrefs.selectSecondary = newKeyPrefs[14];
    keyPrefs.deselectSecondary = newKeyPrefs[15];
    keyPrefs.targetNext = newKeyPrefs[16];
    keyPrefs.targetNearest = newKeyPrefs[17];
    keyPrefs.escortMenu = newKeyPrefs[18];
    keyPrefs.escortOrder[0] = newKeyPrefs[19];
    keyPrefs.escortOrder[1] = newKeyPrefs[20];
    keyPrefs.escortOrder[2] = newKeyPrefs[21];
    keyPrefs.escortOrder[3] = newKeyPrefs[22];
    keyPrefs.pause = newKeyPrefs[23];
    keyPrefs.dismiss = newKeyPrefs[24];
    keyPrefs.board = newKeyPrefs[25];
    keyPrefs.jettison = newKeyPrefs[26];
    keyPrefs.eject = newKeyPrefs[27];
    keyPrefs.selfDestruct = newKeyPrefs[28];
    keyPrefs.cloak = newKeyPrefs[29];
    keyPrefs.map = newKeyPrefs[30];
    keyPrefs.playerInfo = newKeyPrefs[31];
    keyPrefs.missions = newKeyPrefs[32];
    keyPrefs.toggleFPS = newKeyPrefs[33];
}
#endif

#ifdef MACBUILD
/* isPressed (mac _isPressed, mac-only). STATE: see the mac gate. */
// MAC: _isPressed i386 0x00004d48 ppc 0x0000626c
long isPressed(unsigned char *keys, unsigned short k)
{
    unsigned char b = keys[k >> 3];

    b >>= (k & 7);
    return b & 1;
}
#endif

/* DupKeyCheck. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466c10
// MAC: _DupKeyCheck i386 0x00004d69 ppc 0x00006284
Boolean DupKeyCheck(short value, short skip)
{
    short i;

    for (i = 0; i < 0x22; i++) {
        if (value == newKeyPrefs[i] && i != skip)
            return 1;
    }
    return 0;
}

/* IncrementDate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00466c40
// MAC: _IncrementDate i386 0x00004da6 ppc 0x000062cc
void IncrementDate(DateTimeRec *date, long daysToAdd)
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
// MAC: _IncrementGameTime i386 0x0000b516 ppc 0x0000dfdc
void IncrementGameTime(void)
{
    short i;
    short j;
    short wave;
    IncrementDate(&gameTime, 0);
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
                if (stellarData[j].defCount > kDefWaveEncoded)
                    wave = stellarData[j].defCount % kDefWaveEncoded;
                else
                    wave = stellarData[j].defCount;
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

    for (j = 0; j < kMaxStellars; j++) {
        if (systemData[j].loaded != 0 && systemData[j].reinforceDaysLeft > 0)
            systemData[j].reinforceDaysLeft--;
    }

    for (j = 0; j < kMaxRanks; j++) {
        if (rankData[j].active != 0 && rankData[j].loaded != 0 && rankData[j].salary != 0) {
            if (shipData->credits < rankData[j].salaryCap || rankData[j].salaryCap <= 0)
                shipData->credits += rankData[j].salary;
            if (shipData->credits < 0)
                shipData->credits = 0;
        }
    }

    for (j = 0; j < kMaxShipTypes; j++) {
        shipClass[j].dailyHireRoll = Rand(kDailyRollRange) + 1;
        shipClass[j].dailySaleRoll = Rand(kDailyRollRange) + 1;
    }

    for (j = 0; j < kMaxOutfits; j++)
        outfitSystem[j].dailyRoll = Rand(kDailyRollRange) + 1;
}

/* SlapWithPenalty (mac _SlapWithPenalty). STATE: MATCH 100.0%, exe 112 / obj 112
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00466fc0
// MAC: _SlapWithPenalty i386 0x00009bbc ppc 0x0000c108
void SlapWithPenalty(short sys, short govt, short type, short mode)
{
    short i;

    if (govt != -1) {
        if ((govtData[govt].flags & kGovtAlwaysDisabled) != 0)
            return;
    }
    for (i = 0; i < kMaxSystems; i++)
        systemFlag[i] = 0;

    if (mode == -1) {
        RecursivePenaltySlap(sys, govt, type, 1.0);

        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0
                    && (rankData[i].flags & kRankNeverDropped) == 0
                    && GovtAllies(govt, rankData[i].govt) != 0) {
                if ((rankData[i].flags & kRankLostOnCrime) != 0)
                    DeactivateRank((char *)&rankData[i]);
                else if ((rankData[i].flags & kRankLostOnAttack) != 0
                        && (type == 1 || type == 3))
                    DeactivateRank((char *)&rankData[i]);
            }
        }
    }
}

/* RecursivePenaltySlap (mac _RecursivePenaltySlap). STATE: MATCH 100.0%, exe 387 / obj 387
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00467140
// MAC: _RecursivePenaltySlap i386 0x0000980a ppc 0x0000bb40
void RecursivePenaltySlap(short sys, short govt, short type, double amount)
{
    short startSys, i;
    short govtFlag1;
    short sysFlag1, sysFlag2;
    Boolean dirty;
    float penalty;

    if (sys < 0 || sys >= kMaxSystems)
        return;
    if (systemFlag[sys])
        return;
    systemFlag[sys] = 1;

    if (govt != -1)
        govtFlag1 = govtData[govt].flags & kGovtXenophobic;
    else
        govtFlag1 = 0;

    startSys = sys;
    dirty = 0;
    if (systemData[sys].colocGroup != -1)
        sys = systemData[sys].colocGroup;
    while (sys != -1) {
        systemFlag[sys] = 1;
        if (systemData[sys].loaded) {
            penalty = 0;
            sysFlag1 = 0;
            sysFlag2 = 0;
            if (systemData[sys].govt != -1) {
                sysFlag1 = govtData[systemData[sys].govt].flags & kGovtXenophobic;
                sysFlag2 = govtData[systemData[sys].govt].flags & kGovtEnforcesAbroad;
            }

            if (systemData[sys].govt == -1) {
                if (govt == -1) {
                    penalty = govtData[0].penalties[type] * 0.5;
                } else if (govtFlag1) {
                    penalty = -govtData[govt].penalties[type] * 0.5;
                } else {
                    penalty = govtData[govt].penalties[type] * 0.25;
                }
            } else if (govt == systemData[sys].govt) {
                penalty = govtData[govt].penalties[type];
            } else if (govt == -1) {
                if (sysFlag1)
                    penalty = -govtData[systemData[sys].govt].penalties[type] * 0.5;
                else if (sysFlag2)
                    penalty = govtData[systemData[sys].govt].penalties[type] * 0.5;
            } else if (!GovtAllies(govt, systemData[sys].govt)) {
                penalty = -govtData[systemData[sys].govt].penalties[type] * 0.5;
            } else if (!GovtEnemies(govt, systemData[sys].govt)) {
                if (GovtAllies(govt, systemData[sys].govt))
                    penalty = govtData[govt].penalties[type];
                else
                    penalty = govtData[govt].penalties[type] * 0.5;
            } else {
                penalty = -govtData[systemData[sys].govt].penalties[type] * 0.5;
            }

            penalty *= amount;
            if (penalty >= 1.0 || penalty <= -1.0) {
                dirty = 1;
                if (penalty > 0.0) {
                    if (playerRecord[sys] - penalty < -32000.0f)
                        playerRecord[sys] = -32000;
                    else
                        playerRecord[sys] -= penalty;
                } else {
                    if (playerRecord[sys] - penalty > 32000.0f)
                        playerRecord[sys] = 32000;
                    else
                        playerRecord[sys] -= penalty;
                }
                if (playerRecord[sys] > 32000)
                    playerRecord[sys] = 32000;
                if (playerRecord[sys] < -32000)
                    playerRecord[sys] = -32000;
            }
        }
        sys = systemData[sys].colocNext;
    }

    if (dirty) {
        amount *= 0.65;
        for (i = 0; i < kMaxSystemLinks; i++) {
            if (systemData[startSys].links[i] != -1)
                RecursivePenaltySlap(FindFirstCoLocatedSystem(systemData[startSys].links[i]),
                                     govt, type, amount);
        }
    }
}

#define kFadeMillis 500
#define kMessageStrings 1000    /* STR# holding the numbered player messages */


EXTERN_C_END

/* scratch recursion marks, one per system (name invented) */
Boolean systemFlag[kMaxSystems];
Boolean speechMgrPresent;
short maxAutoExploreJumps;
#ifdef MAC1010
unsigned short keyMap[8];
#elif defined(MACBUILD)
KeyMapByteArray keyMap;
#else
unsigned char DAT_007ccd94[68];
#endif
#ifndef MACBUILD
unsigned long randomSeed;
#endif

EXTERN_C_BEGIN

/* screen fade state (names invented; the mac fades via CGDisplayFade instead) */
#ifndef MACBUILD
long screenGamma = 0x00010000;           /* 0x00575878 — 16.16, 1.0 = full brightness */
#endif

/*
 * The windows port drives the palette from the DirectDraw side, so the mac
 * gamma write is compiled out here. The ramp is still walked at the original
 * rate, which is what paces the fade.
 */
#define SetScreenGamma(level)  ((void)(level))

/* EscapeStellarSearch. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467710
// MAC: _EscapeStellarSearch i386 0x0000d28f ppc 0x00010648
short EscapeStellarSearch(short sys)
{
    short i;

    for (i = 0; i < kMaxSystems; i++)
        systemFlag[i] = 0;
    systemFlag[sys] = 1;
    return RecursiveEscapeStellarSearch(sys);
}

/* RecursiveEscapeStellarSearch. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004677a0
// MAC: _RecursiveEscapeStellarSearch i386 0x0000d0fe ppc 0x00010464
short RecursiveEscapeStellarSearch(short sys)
{
    short i;
    short j;
    short link;
    short link2;
    short stellar;

    systemFlag[sys] = 1;

    for (i = 0; i < kNavSlots; i++) {
        link = systemData[sys].links[i];
        if (link == -1)
            continue;
        if (systemData[link].loaded == 0)
            continue;
        if (systemData[link].known == 0)
            continue;
        if (systemData[link].exploreLevel <= 0)
            continue;
        if (systemFlag[link] != 0)
            continue;
        for (j = 0; j < kNavSlots; j++) {
            stellar = systemData[link].navDefault[j];
            if (stellar == -1)
                continue;
            if (stellarData[stellar].loaded == 0)
                continue;
            if (!StellarIsNormalLandable(&stellarData[stellar]))
                continue;
            if ((stellarData[stellar].flags & kSpobHasShipyard) == 0)
                continue;
            if (stellarData[stellar].minStatus != 0x7fff &&
                    (playerRecord[link] >= stellarData[stellar].minStatus
                     || stellarData[stellar].minStatus == -0x7fff))
                return stellar;
        }
    }

    for (i = 0; i < kNavSlots; i++) {
        link2 = systemData[sys].links[i];
        if (link2 == -1)
            continue;
        if (systemData[link2].loaded == 0)
            continue;
        if (systemData[link2].known == 0)
            continue;
        if (systemData[link2].exploreLevel <= 0)
            continue;
        if (systemFlag[link2] != 0)
            continue;
        stellar = RecursiveEscapeStellarSearch(link2);
        if (stellar != -1)
            return stellar;
    }
    return -1;
}

/* AutoSetExploration (mac _AutoSetExploration). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467970
// MAC: _AutoSetExploration i386 0x0000daba ppc 0x00010ff4
void AutoSetExploration(short sys, short maxDepth, short level)
{
    short i;
    short j;
    short link;

    for (i = 0; i < kMaxSystems; i++)
        systemFlag[i] = 0;

    maxAutoExploreJumps = maxDepth;
    RecursiveAutoExplore(sys, 0, level);

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].prepared != 0 &&
            systemData[i].exploreLevel > 0) {
            systemData[i].known = 1;
            for (j = 0; j < kNavSlots; j++) {
                link = systemData[i].links[j];
                if (link != -1) {
                    link = FindActiveCoLocatedSystem(link);
                    if (link != -1)
                        systemData[link].known = 1;
                }
            }
        }
    }
}

/* RecursiveAutoExplore (mac _RecursiveAutoExplore). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467ab0
// MAC: _RecursiveAutoExplore i386 0x0000d942 ppc 0x00010e4c
void RecursiveAutoExplore(short sys, short depth, short level)
{
    short i;
    short coloc;
    short link;

    if (depth > maxAutoExploreJumps)
        return;
    if (systemFlag[sys] != 0)
        return;
    systemFlag[sys] = 1;

    ExploreNebulaeFromSystem(sys);

    if (systemData[sys].exploreLevel < level)
        systemData[sys].exploreLevel = level;

    coloc = FindFirstCoLocatedSystem(sys);
    if (coloc != sys && coloc >= 0 && coloc < kMaxSystems) {
        if (systemData[coloc].exploreLevel < level)
            systemData[coloc].exploreLevel = level;
    }

    for (i = 0; i < kNavSlots; i++) {
        link = systemData[sys].links[i];
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
// MAC: _ExploreNebulaeFromSystem i386 0x0000508f ppc 0x00006600
void ExploreNebulaeFromSystem(short sys)
{
    Rect box;
    Point pt;
    short top;
    short i;

    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].loaded != 0 && nebulaData[i].explored == 0) {
            top = nebulaData[i].yPos;
            SetRect(&box, nebulaData[i].xPos, top,
                    nebulaData[i].xPos + nebulaData[i].width,
                    top + nebulaData[i].height);
            InsetRect(&box, 8, 8);
            pt.h = systemData[sys].pos.h;
            pt.v = systemData[sys].pos.v;
            if (PtInRectPoint(pt, &box) != 0) {
                nebulaData[i].explored = 1;
                crashlog_mission_bit_set("exploring nebula", 0, i + 128, -1,
                                         nebulaData[i].onExplore);
                EvalMissionBitSetString(nebulaData[i].onExplore);
            }
        }
    }
}

#ifdef MACBUILD
/* ShowHint (mac _ShowHint, mac-only). STATE: see the mac gate. */
// MAC: _ShowHint i386 0x000051be ppc 0x00006724
void ShowHint(void)
{
    Handle h;
    short count;
    Str255 hint;

    h = GetResource('STR#', kHintStrList);
    if (h == NULL)
        return;
    HLock(h);
    BlockMoveData(*h, &count, 2);
    HUnlock(h);
    HPurge(h);
    ReleaseResource(h);
    GetIndString(hint, kHintStrList, Rand(count) + 1);
    DisplayComm((PStrChar *)hint, 250, 0, 12, &rgbWhite, 0, 0);
}
#endif

/* Show numbered message `index`, letting a plug-in override the built-in text. */
/* DisplayMessage (mac _DisplayMessage). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467cf0
// MAC: _DisplayMessage i386 0x00005289 ppc 0x000067d0
void DisplayMessage(short index)
{
    char text[256];

    if (LoadPluginString(index + kMessageStrings - 1, (StringPtr)text) == 0)
        GetIndString((StringPtr)text, kMessageStrings, index);
    DisplayComm((PStrChar *)text, 480, 0, 12, &rgbWhite, 0, 0);
}

/* Walk the gamma ramp to `percent` brightness over `duration` milliseconds. */
#ifndef MACBUILD
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
#endif

/* FadeScreenOut (mac _FadeScreenOut). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467de0
// MAC: _FadeScreenOut i386 0x0000530f ppc 0x00006848
#ifdef V111 /* V111: the Quartz display fade; mac 1.0.10 imports no CGDisplayFade */
/* the fade reservation FadeScreenIn/Out and FadeWhiteIn/Out share (a file static on mac) */
static CGDisplayFadeReservationToken token;

void FadeScreenOut(void)
{
    if (screenFaded || inBackground)
        return;
    if (!settings.windowed) { /* POTENTIAL BUG (win V111): windowed skips the present 1.0.10's ScreenEndScene() gave */
        if (gWind != NULL)
            QDFlushPortBuffer(GetWindowPort(gWind), NULL);
        if (CGAcquireDisplayFadeReservation(15.0, &token) == kCGErrorSuccess)
            CGDisplayFade(token, 1.0, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor,
                          0.0, 0.0, 0.0, true);
    }
    screenFaded = true;
}
#elif defined(MAC1010)
void FadeScreenOut(void)
{
#if TARGET_API_MAC_CARBON
    RGBColor black;

#endif
    if (screenFaded != 0 || inBackground != 0)
        return;
#if TARGET_API_MAC_CARBON
    if (gScreenRef != 0 && settings.windowed == 0) {
        if (gWind != NULL)
            QDFlushPortBuffer(GetWindowPort(gWind), NULL);
        black.red = black.green = black.blue = 0;
        FadeToColor(gScreenRef, &black, 0.5f);
    }
#else
    if (settings.windowed == 0)
        GammaFadeTo(1, 0, kFadeMillis);
#endif
    screenFaded = 1;
}
#else
void FadeScreenOut(void)
{
    if (screenFaded != 0 || inBackground != 0)
        return;
    ScreenEndScene();
    if (settings.windowed == 0)
        FadeGammaTo(0, kFadeMillis);
    screenFaded = 1;
}
#endif

/* FadeScreenIn (mac _FadeScreenIn). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467e20
// MAC: _FadeScreenIn i386 0x000053b2 ppc 0x00006924
#ifdef V111 /* V111: the Quartz display fade */
void FadeScreenIn(void)
{
    if (!screenFaded)
        return;
    if (!settings.windowed) { /* POTENTIAL BUG (win V111): windowed skips the present 1.0.10's ScreenEndScene() gave */
        if (gWind != NULL)
            QDFlushPortBuffer(GetWindowPort(gWind), NULL);
        if (token != 0 || CGAcquireDisplayFadeReservation(15.0, &token) == kCGErrorSuccess) {
            CGDisplayFade(token, 1.0, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal,
                          0.0, 0.0, 0.0, true);
            CGReleaseDisplayFadeReservation(token);
            token = 0;
        }
    }
    screenFaded = false;
}
#ifdef MACBUILD
/* FadeWhiteIn (mac _FadeWhiteIn, mac-only). STATE: see the mac gate. */
// MAC: _FadeWhiteIn i386 0x0000546f ppc 0x00006a04
void FadeWhiteIn(void)
{
    screenFaded = true;
    if (CGAcquireDisplayFadeReservation(15.0, &token) == kCGErrorSuccess)
        CGDisplayFade(token, 1.5, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor,
                      1.0, 1.0, 1.0, false);
}

/* FadeWhiteOut (mac _FadeWhiteOut, mac-only). STATE: see the mac gate. */
// MAC: _FadeWhiteOut i386 0x000054d1 ppc 0x00006a98
void FadeWhiteOut(void)
{
    CGDisplayFade(token, 1.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal,
                  1.0, 1.0, 1.0, false);
    CGReleaseDisplayFadeReservation(token);
    screenFaded = false;
}
#endif

#elif defined(MAC1010)
void FadeScreenIn(void)
{
    if (screenFaded == 0)
        return;
#if TARGET_API_MAC_CARBON
    if (gScreenRef != 0 && settings.windowed == 0) {
        if (gWind != NULL)
            QDFlushPortBuffer(GetWindowPort(gWind), NULL);
        FadeToGamma(gScreenRef, &gOrigGamma, 0.5f);
    }
#else
    if (settings.windowed == 0)
        GammaFadeTo(1, 100, kFadeMillis);
#endif
    screenFaded = 0;
}
#else
void FadeScreenIn(void)
{
    if (screenFaded == 0)
        return;
    ScreenEndScene();
    if (settings.windowed == 0)
        FadeGammaTo(100, kFadeMillis);
    screenFaded = 0;
}
#endif

/* OnTheFlyWhiteFade (mac _OnTheFlyWhiteFade). STATE: MATCH, byte-identical.
   OPEN: none. `pct' is pushed by the caller and never read here. */
// FUNCTION: EVNOVA 0x00467e60
// MAC: _OnTheFlyWhiteFade i386 0x00005528 ppc 0x00006b18
void OnTheFlyWhiteFade(short pct)
{
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    ScreenGamma gamma;
    float scale;
    float white;
    short i;

    if (settings.windowed != 0)
        return;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    scale = (100 - pct) * 0.01;
    white = pct * 0.01;
    for (i = 0; i < 256; i++) {
        gamma.red[i] = scale * gOrigGamma.red[i] + white;
        gamma.green[i] = gOrigGamma.green[i] * scale + white;
        gamma.blue[i] = gOrigGamma.blue[i] * scale + white;
        if (gamma.red[i] > 1.0)
            gamma.red[i] = 1.0f;
        if (gamma.green[i] > 1.0)
            gamma.green[i] = 1.0f;
        if (gamma.blue[i] > 1.0)
            gamma.blue[i] = 1.0f;
    }
    SetGamma(gScreenRef, &gamma);
#elif defined(MAC1010)
    long white[3];

    if (settings.windowed != 0)
        return;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    white[0] = white[1] = white[2] = -1;
    GammaFadeToColor(1, white, pct, 1, 1);
#endif
}

#ifdef MACBUILD
/* InitCluts (mac _InitCluts, mac-only). STATE: see the mac gate. */
// MAC: _InitCluts i386 0x0000552d ppc 0x00006b1c
void InitCluts(void)
{
}

/* SetOrigClut (mac _SetOrigClut, mac-only). STATE: see the mac gate. */
// MAC: _SetOrigClut i386 0x00005532 ppc 0x00006b20
void SetOrigClut(void)
{
}
#endif

/* RestoreOrigClut (mac _RestoreOrigClut). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00467e70
// MAC: _RestoreOrigClut i386 0x00005537 ppc 0x00006b24
void RestoreOrigClut(void)
{
}

/* ShipCanCloak (no mac symbol). STATE: MATCH 100.0%, exe 174 / obj 174 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00467e80
// MAC: _ShipCanCloak i386 0x0000553c ppc 0x00006b28
Boolean ShipCanCloak(ShipObj *ship)
{
    short idx;
    short i;
    short j;
    Boolean found;
    Boolean nibbleLo;
    Boolean nibbleHi;

    if (IsDisabled(ship))
        return 0;

    found = 0;
    idx = ship->index;
    if (idx == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17) {
                        found = 1;
                        /* BUG: both branches read modType[j] for modVal[j], so the nibbles are always 1 and 0 */
                        nibbleLo = (outfitSystem[i].modType[j] >> 4) & 0xf;
                        nibbleHi = (outfitSystem[i].modType[j] >> 8) & 0xf;
                        break;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17) {
                        found = 1;
                        nibbleLo = (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] >> 4) & 0xf;
                        nibbleHi = (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] >> 8) & 0xf;
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

        if (idx == 0 && ship->jumpTimer > 0.0f
            && (shipClass[ship->type].flags2 & kShipCloaksToJump) == 0)
            return 0;
        return 1;
    }
    return 0;
}

/* DoShipCloak. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004680d0
// MAC: _DoShipCloak i386 0x0000de73 ppc 0x00011490
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
// MAC: _DoShipUncloak i386 0x0000df55 ppc 0x000115f0
void DoShipUncloak(ShipObj *ship)
{
    if (ShipIsCloaked(ship)) {
        if (ship->index == 0 && ship->cloakDir >= 0)
            PlaySound(cloakSnd[0], 8, gSoundVolume, gSoundVolume);
        if (ship->cloak > 0.0f) {
            ship->cloakDir = -1;
        } else {
            if (ship->cloakDir > 0)
                ship->cloakDir = 0;
        }
    }
}

/* SystemTradesGoods. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468210
// MAC: _SystemTradesGoods i386 0x0000d2c4 ppc 0x0001069c
Boolean SystemTradesGoods(short sysID, short commodity)
{
    short i;

    for (i = 0; i < kNavSlots; i++) {
        if (systemData[sysID].navDefault[i] != -1
                && stellarData[systemData[sysID].navDefault[i]].loaded != 0
                && (stellarData[systemData[sysID].navDefault[i]].flags
                    & kSpobUninhabited) == 0
                && StellarIsNormalLandable(
                       &stellarData[systemData[sysID].navDefault[i]])
                && (stellarData[systemData[sysID].navDefault[i]].flags
                    & kSpobTradeCenter) != 0
                && CalcPortDemand(commodity,
                       stellarData[systemData[sysID].navDefault[i]].flags) != 0)
            return 1;
    }
    return 0;
}

/* the same scan asking for one of three services by index; a `kind` outside
   0..2 leaves the mask unset, which the original does not guard */
/* SystemHasService. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004682d0
// MAC: _SystemHasService i386 0x0000d373 ppc 0x0001077c
Boolean SystemHasService(short sysID, short kind)
{
    short i;
    long mask;

    for (i = 0; i < kNavSlots; i++) {
        if (systemData[sysID].navDefault[i] != -1
                && stellarData[systemData[sysID].navDefault[i]].loaded != 0
                && (stellarData[systemData[sysID].navDefault[i]].flags
                    & kSpobUninhabited) == 0
                && StellarIsNormalLandable(
                       &stellarData[systemData[sysID].navDefault[i]])) {
            if (kind == 0)
                mask = kSpobTradeCenter;
            if (kind == 1)
                mask = kSpobHasOutfitter;
            if (kind == 2)
                mask = kSpobHasShipyard;
            if ((stellarData[systemData[sysID].navDefault[i]].flags & mask) != 0)
                return 1;
        }
    }
    return 0;
}

#endif /* MAC_FASTRANDOM_ONLY */
#ifndef MAC_NO_FASTRANDOM
#ifdef MAC1010
static unsigned long randomSeed;
#elif defined(MACBUILD)
unsigned long randomSeed;
#endif

#define kRandMultiplier 16807

/* The 68k `swap` idiom: the >> 16 half is always zero for a value already
   masked to 15 bits and mwcc folds it away, but the PPC build emits it
   literally and mac 1.1.1 emits a `ror`, so it is in the source. */
#define SwapHalves(x) (((x) << 16) + ((x) >> 16))

/* Rand (mac _Rand). STATE: MATCH 100.0%, 45/45, byte-identical; mac MATCH on both slices.
   OPEN: none. Park-Miller 16807 in 16-bit halves, transliterated register by register
   from the 68k Random(). */
// FUNCTION: EVNOVA 0x004683b0
// MAC: _Rand i386 0x000a4c76 ppc 0x000b08b4
short Rand(short range)
{
    unsigned long lo;
    unsigned long temp;
    unsigned long hi;
    unsigned long seed;
    short result;
#ifdef V111
    unsigned long secs;
#endif

    if (range == 0) {
#ifdef V111
        GetDateTime(&secs);
        result = randomSeed = secs + ReadTick();
#else
        /* BUG: this path never sets result, so Rand(0) returns garbage */
        GetDateTime(&randomSeed);
        randomSeed += ReadTick();
#endif
    } else {
        seed = randomSeed;
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
#ifdef MACBUILD
        if (lo < 0) /* BUG: lo is unsigned, so the carry fold never runs */
            lo += 0x7fffffff;
#else
        if ((lo & 0x80000000) != 0)
            lo += 0x7fffffff;
#endif
        randomSeed = lo;
        if ((lo & 0xffff) == 0x8000)
            lo &= 0xffff0000;
        temp = range;
        temp *= lo & 0xffff;
        temp >>= 16;
        result = temp;
    }
    return result;
}

/* SetRandSeed (mac _SetRandSeed; dead on win, stripped). STATE: mac MATCH on both slices. OPEN: none */
// MAC: _SetRandSeed i386 0x000a4d08 ppc 0x000b0968
void SetRandSeed(unsigned long seed)
{
    randomSeed = seed;
}

/* GetRandSeed (mac _GetRandSeed; dead on win, stripped). STATE: mac MATCH on both slices. OPEN: none */
// MAC: _GetRandSeed i386 0x000a4d17 ppc 0x000b0978
unsigned long GetRandSeed(void)
{
    return randomSeed;
}
#endif /* MAC_NO_FASTRANDOM */
#ifndef MAC_FASTRANDOM_ONLY

#define kDateStrings   137  /* STR# holding the month names and ordinals */
#define kFirstMonthStr  12  /* month 1 is index 13 */
#define kOrdinalSt      25
#define kOrdinalNd      26
#define kOrdinalRd      27
#define kOrdinalTh      28

/* DateToMedString (mac _DateToMedString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468450
// MAC: _DateToMedString i386 0x00008cb6 ppc 0x0000adcc
void DateToMedString(DateTimeRec date, char *out)
{
    Str255 str;

    strcpy(out, datePrefix);
    GetIndString(str, kDateStrings, date.month + kFirstMonthStr);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, " ");
    NumToString(date.day, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    GetIndString(str, kDateStrings, kOrdinalTh);
    if (date.day % 10 == 1)
        GetIndString(str, kDateStrings, kOrdinalSt);
    if (date.day % 10 == 2)
        GetIndString(str, kDateStrings, kOrdinalNd);
    if (date.day % 10 == 3)
        GetIndString(str, kDateStrings, kOrdinalRd);
    if (date.day >= 11 && date.day <= 13)
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
// MAC: _DateToLongString i386 0x00008ef8 ppc 0x0000b014
void DateToLongString(DateTimeRec date, char *out)
{
    Str255 str;

    strcpy(out, datePrefix);
    GetIndString(str, kDateStrings, date.month);
    PtoCstr(str);
    strcat(out, (char *)str);
    strcat(out, " ");
    NumToString(date.day, str);
    PtoCstr(str);
    strcat(out, (char *)str);
    GetIndString(str, kDateStrings, kOrdinalTh);
    if (date.day % 10 == 1)
        GetIndString(str, kDateStrings, kOrdinalSt);
    if (date.day % 10 == 2)
        GetIndString(str, kDateStrings, kOrdinalNd);
    if (date.day % 10 == 3)
        GetIndString(str, kDateStrings, kOrdinalRd);
    if (date.day >= 11 && date.day <= 13)
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
// MAC: _IsDisabled i386 0x00002ce6 ppc 0x0000397c
Boolean IsDisabled(ShipObj *ship)
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
    if ((shipClass[ship->type].flags & kShipArmorDisableTenth) != 0) {
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
// MAC: _IsDying i386 0x0000253d ppc 0x00002ef0
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
// MAC: _CanHireEscorts i386 0x00005795 ppc 0x00006df0
Boolean CanHireEscorts(void)
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
// MAC: _WeaponHasAmmo i386 0x0000b95a ppc 0x0000e53c
Boolean WeaponHasAmmo(ShipObj *ship, short weaponIndex)
{
    short ammoType;

    if (ship->index != 0
            && (weaponStats[weaponIndex].flags2 & kWeapFlagAIRestricted) != 0)
        return 0;
    if (ShipIsCloaked(ship) && (weaponStats[weaponIndex].flags2 & kWeapFireWhileCloaked) == 0)
        return 0;
    if ((weaponStats[weaponIndex].flags2 & kWeapNeedsKeyCarried) != 0 && !KeyCarriedShipTypeOnboard(ship))
        return 0;
    if (weaponStats[weaponIndex].guidance == kGuidanceBay) {
        if (ship->weap[weaponIndex].ammo <= 0)
            return 0;
    } else {
        ammoType = weaponStats[weaponIndex].ammoType;
        if (ammoType >= 0 && ammoType < 256) {
            if (ship->index == 0) {
                if (ship->weap[ammoType].ammo <= 0)
                    return 0;
            } else if (ship->weap[weaponIndex].ammo <= 0)
                return 0;
        } else if (ammoType <= -kWeapAmmoFuelBase
                   && ship->fuel < (abs(ammoType) - kWeapAmmoFuelBase) * 0.1)
            return 0;
    }
    return 1;
}

/* inhabited = at least one stellar that is populated and not a hypergate or wormhole */

/* SystemIsInhabited (mac _SystemIsInhabited). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468af0
// MAC: _SystemIsInhabited i386 0x00004c4a ppc 0x00006124
Boolean SystemIsInhabited(short sys)
{
    short i;
    short spob;

    for (i = 0; i <= 3; i++) {
        spob = systemData[sys].navDefault[i];
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
// MAC: _StellarsAreDistant i386 0x00009cb9 ppc 0x0000c23c
Boolean StellarsAreDistant(short a, short b)
{
    short i;
    short found;
    short count;
    SpobRecord *pb;
    short sysA;
    short sysB;
    SpobRecord *pa;

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
            if (systemData[pa->system].links[i] != -1 &&
                    FindFirstCoLocatedSystem(systemData[pa->system].links[i]) == sysB)
                return 0;
            if (systemData[pb->system].links[i] != -1 &&
                    FindFirstCoLocatedSystem(systemData[pb->system].links[i]) == sysA)
                return 0;
        }
    } else {
        if (pa->system < 0 || pa->system >= kMaxSystems)
            return 0;
        sysA = FindFirstCoLocatedSystem(pa->system);
    }

    count = 0;
    found = 0;
    sysA = FindFirstCoLocatedSystem(sysA);
    do {
        count++;
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[sysA].navDefault[i] == pa->index) {
                found++;
                break;
            }
        }
        sysA = systemData[sysA].colocNext;
    } while (sysA != -1);

    if (found != count)
        return 0;
    return 1;
}

/* DrawLegalStatusString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00468d90
// MAC: _DrawLegalStatusString i386 0x0000d43f ppc 0x00010888
void DrawLegalStatusString(short sys)
{
    long base;
    short haven;
    short clean;
    long status;
    short j;
    short rating;

    status = playerRecord[sys];
    rating = 0;
    if (systemData[sys].govt >= 0 && systemData[sys].govt < kMaxGovts)
        base = govtData[systemData[sys].govt].crimeTol;
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
        if (systemData[sys].navDefault[j] != -1 && stellarData[systemData[sys].navDefault[j]].loaded &&
                !SpobUninhabited(&stellarData[systemData[sys].navDefault[j]]) &&
                StellarIsNormalLandable(&stellarData[systemData[sys].navDefault[j]])) {
            if (stellarData[systemData[sys].navDefault[j]].dominated != 0)
                haven++;
            else
                clean++;
        }
    }
    if (haven > 0) {
        if (clean > 0)
            rating = 0x10;
        else
            rating = 0x11;
    }

    if (systemData[sys].govt >= 0 && (govtData[systemData[sys].govt].flags & kGovtXenophobic) != 0)
        rating = 0;

    if (rating == 0)
        DrawIndString(kGameStrList, 0x18c);
    else
        DrawString(legalStatusStrings[rating]);
}

/* DrawCombatRatingString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469030
// MAC: _DrawCombatRatingString i386 0x000057e5 ppc 0x00006e68
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
// MAC: _PlayerShipTradeInPrice i386 0x0000b079 ppc 0x0000d9cc
long PlayerShipTradeInPrice(void)
{
    short i;
    long value;
    long cost;

    value = (shipClass[shipData->type].cost * 0.25);
    for (i = 0; i < 512; i++) {
        if (shipSystems[i] > 0 && outfitSystem[i].persistent == 0) {
            cost = AdjustedItemCost(&outfitSystem[i], &shipClass[shipData->type]);
            value = value + cost * shipSystems[i] * 0.5;
        }
    }
    if (value < 0)
        value = 0;
    return value;
}

/* PlayerCanUseAmmoType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469230
// MAC: _PlayerCanUseAmmoType i386 0x00005896 ppc 0x00006f40
#ifdef MAC1010 /* returns long here; its one caller, commdlg.c, reads a Boolean */
long PlayerCanUseAmmoType(short weapIndex)
#else
Boolean PlayerCanUseAmmoType(short weapIndex)
#endif
{
    short i;

    for (i = 0; i < 256; i++) {
        if (shipData->weap[i].count > 0) {
#ifdef MAC1010
            long w;

            if (weaponStats[i].guidance == kGuidanceBay) {
                w = weaponStats[i].ammoType != weaponStats[weapIndex].ammoType;
                return w == 0;
            }
            if (weapIndex == weaponStats[i].ammoType)
                return 1;
            w = weaponStats[weapIndex].ammoType;
#else
#ifdef V111 /* V111 spelling */
            if (weaponStats[i].guidance == kGuidanceBay) {
                if (weaponStats[i].ammoType == weaponStats[weapIndex].ammoType)
#else
            long w = weapIndex;

            if (weaponStats[i].guidance == kGuidanceBay) {
                if (weaponStats[i].ammoType == weaponStats[w].ammoType)
#endif
                    return 1;
                return 0;
            }
            if (weapIndex == weaponStats[i].ammoType)
                return 1;
#endif
        }
    }
    return 0;
}

#define kPanCutoffSq   40000
#define kPanNear      722500
#define kPanHalfWidth    200

/* PlaySoundDistance. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004692e0
// MAC: _PlaySoundDistance i386 0x0000e007 ppc 0x000116f4
Boolean PlaySoundDistance(long unused, void *sound, short flags, FloatPoint *pos,
                     FloatPoint *listener)
{
    int dy;
    int dx;
    int dist;
    short floorVol;
    short leftVol;
    short rightVol;

    leftVol = rightVol = gSoundVolume;
    if (gSoundVolume > 0) {
        dx = (pos->x - listener->x);
        dy = (pos->y - listener->y);
        dist = dx * dx + dy * dy;
        if (dist > 0) {
            if (dist > kPanCutoffSq) {
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
    }
    PlaySound(sound, flags, leftVol, rightVol);
    return 1;
}

#ifdef MACBUILD
/* PlaySoundWait (mac _PlaySoundWait, mac-only). STATE: see the mac gate. */
// MAC: _PlaySoundWait i386 0x0000e131 ppc 0x0001185c
void PlaySoundWait(void *snd)
{
    PlaySound(snd, 9999, gSoundVolume, gSoundVolume);
    while (ST_IsSoundPlaying(snd))
        ;
}
#endif

/* CheckSpeechMgr (mac _CheckSpeechMgr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469480
// MAC: _CheckSpeechMgr i386 0x00005906 ppc 0x00006fd0
#ifdef MACBUILD
void CheckSpeechMgr(void)
{
    long response;
    short voices;
    Boolean hasSpeech;
    Boolean ok;
    OSErr err;

    speechMgrPresent = false;
#ifdef MAC1010
    ok = Gestalt('vers', &response) == noErr;
    if (ok) {
        err = Gestalt(gestaltSpeechAttr, &response);
        hasSpeech = err == noErr && BitTst(&response, 31 - gestaltSpeechMgrPresent);
        if (hasSpeech ? BitTst(&response, 31 - gestaltSpeechHasPPCGlue) : hasSpeech) {
#else
    if (Gestalt('vers', &response) == noErr) {
        hasSpeech = Gestalt(gestaltSpeechAttr, &response) == noErr
                    && BitTst(&response, 31 - gestaltSpeechMgrPresent);
        if (hasSpeech) {
#endif
            CountVoices(&voices);
            if (voices > 0)
                speechMgrPresent = true;
        }
    }
}
#else
void CheckSpeechMgr(void)
{
}
#endif

/* SpeakIt (mac _SpeakIt). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469490
// MAC: _SpeakIt i386 0x0000b029 ppc 0x0000d948
#ifdef MACBUILD
void SpeakIt(char *msg, short flag)
{
    static SpeechChannel speechChannel;

    if (speechMgrPresent) {
        if (speechChannel != NULL) {
            DisposeSpeechChannel(speechChannel);
            speechChannel = NULL;
        }
#ifdef MAC1010
        CtoPstr(msg);
#endif
        SpeakString((StringPtr)msg);
        if (flag)
            while (SpeechBusy())
                BusyIdle();
    }
}
#else
void SpeakIt(char *msg, short flag)
{
}
#endif

#ifdef MACBUILD
/* HasMaxFightersOnBoard (mac _HasMaxFightersOnBoard, mac-only). STATE: MATCH both slices. OPEN: none */
// MAC: _HasMaxFightersOnBoard i386 0x00005986 ppc 0x00007080
Boolean HasMaxFightersOnBoard(short shipType)
{
    short j;
    short i;
    short k;
    short bay;
    short outf;

    bay = -1;
    outf = -1;
    for (j = 0; j < kMaxWeapTypes; j++) {
        if (weaponStats[j].guidance == kGuidanceBay
                && weaponStats[j].ammoType == shipType + kFirstResourceID
                && shipData[0].weap[j].count > 0) {
            bay = j;
            break;
        }
    }
    if (bay != -1) {
        for (i = 0; i < 512; i++) {
            for (k = 0; k < kOutfMods; k++) {
                if (outfitSystem[i].techLevel != kOutfTechAbsent && outfitSystem[i].modType[k] == 3
                        && outfitSystem[i].modVal[k] == bay) {
                    outf = i;
                    break;
                }
            }
        }
    }
    if (outf == -1)
        return true;
    if (shipData[0].weap[bay].ammo >= outfitSystem[outf].maxCount)
        return true;
    return false;
}
#endif

#define kEscortFighter   5

/* CanBuyFighter (mac _CanBuyFighter). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004694a0
// MAC: _CanBuyFighter i386 0x00005a82 ppc 0x00007190
Boolean CanBuyFighter(short shipType, short outfitIndex, short weapIndex)
{
    short limit;
    short count;
    short i;
    short m;

    if (outfitIndex == -1 && weapIndex == -1) {
        for (i = 0; i < 256; i++) {
            if (shipData->weap[i].count > 0 &&
                weaponStats[i].guidance == kGuidanceBay &&
                weaponStats[i].ammoType - 128 == shipType) {
                weapIndex = i;
                break;
            }
        }
        if (weapIndex == -1) {
            for (i = 0; i < 256; i++) {
                if (shipData->weap[i].count > 0 &&
                    weaponStats[i].guidance == kGuidanceBay &&
                    shipClass[weaponStats[i].ammoType - 128].spriteIndex ==
                        shipClass[shipType].spriteIndex) {
                    weapIndex = i;
                    break;
                }
            }
        }
        if (weapIndex == -1)
            return 0;
        for (i = 0; i < 512; i++) {
            for (m = 0; m < kOutfMods; m++) {
                if (outfitSystem[i].techLevel < kOutfTechAbsent &&
                    outfitSystem[i].modType[m] == kOutfModAmmo &&
                    weapIndex == outfitSystem[i].modVal[m]) {
                    outfitIndex = i;
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

    if (weaponStats[weapIndex].maxAmmo > 0)
        limit = weaponStats[weapIndex].maxAmmo * shipData->weap[weapIndex].count;
    else
        limit = outfitSystem[outfitIndex].maxCount;

    for (m = 1; m < 64; m++) {
        if (shipData[m].active != 0 &&
            shipType == shipData[m].type &&
            shipData[m].parentShip == 0 &&
            shipData[m].aiType == kEscortFighter &&
            shipData[m].specialMission == -1)
            count++;
    }
    if (count < limit)
        return 1;
    return 0;
}

#define kMaxHolds 32000

/* TotalFleetHolds (mac _TotalFleetHolds). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469760
// MAC: _TotalFleetHolds i386 0x0000c24d ppc 0x0000ef88
short TotalFleetHolds(void)
{
    short i;
    long total;

    total = ShipTotalHolds();
    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 && !IsDying(&shipData[i])
                && shipData[i].parentShip == 0 && shipData[i].aiType == 6
                && shipData[i].specialMission == -1
                && shipClass[shipData[i].type].inherentAI <= 2)
            total += shipClass[shipData[i].type].cargo;
    }
    if (total > kMaxHolds)
        total = kMaxHolds;
    return total;
}

/* DestroyPartialFleetCargo (mac _DestroyPartialFleetCargo). STATE: MATCH 100.0%, exe 287 /
   obj 287, byte-identical yes (bytediff after the 2026-09-15 regate). OPEN: none.
   The dead cargo sum survives DSE because its one reader is itself a dead assignment; notes. */
// FUNCTION: EVNOVA 0x00469810
// MAC: _DestroyPartialFleetCargo i386 0x0000cd32 ppc 0x0000feb0
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
        newShipHolds = shipClass[shipData[shipIndex].type].cargo;
    }
    fleetHolds = ShipTotalHolds();
    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 && (!IsDying(&shipData[i]) || i == shipIndex)
                && shipData[i].curSystem == shipData->curSystem
                && shipData[i].parentShip == 0 && shipData[i].aiType == 6
                && shipData[i].specialMission == -1
                && shipClass[shipData[i].type].inherentAI <= 2)
            fleetHolds = shipClass[shipData[i].type].cargo + fleetHolds;
    }
    totalCargo = 0.0;
    for (i = 0; i <= 5; i++)
        totalCargo += shipData->cargo[i];
    for (i = 0; i < kMaxJunks; i++)
        totalCargo += junkData[i].held;
    ratio = newShipHolds / totalCargo;
    ratio = newShipHolds / fleetHolds;
    if (ratio > 1.0)
        ratio = 1.0;

    for (i = 0; i <= 5; i++) {
        rounded = (shipData->cargo[i] * ratio);
        if (shipData->cargo[i] <= rounded) {
            shipData->cargo[i] = 0;
        } else {
            shipData->cargo[i] -= rounded;
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

#ifdef MACBUILD
/* FlushKeys (mac _FlushKeys, mac-only). STATE: see the mac gate. */
// MAC: _FlushKeys i386 0x00005d35 ppc 0x00007498
void FlushKeys(void)
{
    long *keys = (long *)keyMap;

    keys[0] = 0;
    keys[1] = 0;
    keys[2] = 0;
    keys[3] = 0;
}
#endif

/* The flag is tested (with its companion at 0x007354b2) right before the
   shift/control/alt scancodes are scanned, so clearing it re-arms those. */

/* QuickKeyCheckHandler. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469c90
// MAC: _QuickKeyCheckHandler i386 0x00005d5a ppc 0x000074b8
#ifdef MACBUILD
void QuickKeyCheckHandler(void)
{
    GetKeys((void *)keyMap);
    quickKeyCheckResult = 0;
}
#else
void QuickKeyCheckHandler(void)
{
    quickKeyCheckResult = 0;
}
#endif

#if defined(MACBUILD) && !defined(MAC1010)
/* ScanForKeys (mac _ScanForKeys, mac-only). STATE: see the mac gate. */
// MAC: _ScanForKeys i386 0x0000a0af ppc 0x0000c684
short ScanForKeys(void)
{
    KeyMap keys;
    short i;

    GetKeys(keys);
    for (i = 0; i < 128; i++)
        if (isPressed((unsigned char *)keys, i))
            return i;
    return -1;
}
#endif

/* QuickKeyCheck (mac _QuickKeyCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469ca0
// MAC: _QuickKeyCheck i386 0x0000a0f5 ppc 0x0000c6e4
#ifdef MACBUILD
unsigned char QuickKeyCheck(short scanCode)
{
#ifdef MAC1010 /* 1.0.10 real: 0x100af67c word and bit split from the scan code, no isPressed */
    short i;
    short word;
    short bit;
#else
    int i;
    unsigned char result;
#endif

    if (ignoreKeys)
        return 0;
    if (escortMenuOnscreen > 0) {
        for (i = 0; i < 5; i++) {
            if (scanCode == keyPrefs.quickKey[i])
                return 0;
        }
    }
#ifdef MAC1010
    word = (scanCode >> 4) & 0xf;
    bit = scanCode & 0xf;
    if (keyMap[word] & (1 << bit)) {
        quickKeyCheckResult = true;
        return 1;
    }
    return 0;
#else
    result = isPressed(keyMap, scanCode);
    if (result)
        quickKeyCheckResult = true;
    return result;
#endif
}
#else
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
#endif

#ifdef MACBUILD
/* QuickKeyCheckPlusEscorts (mac _QuickKeyCheckPlusEscorts, mac-only). STATE: see the mac gate. */
// MAC: _QuickKeyCheckPlusEscorts i386 0x0000a158 ppc 0x0000c774
Boolean QuickKeyCheckPlusEscorts(short code)
#ifdef MAC1010
{
    short word;
    short bit;

    if (ignoreKeys)
        return 0;
    word = (code >> 4) & 0xf;
    bit = code & 0xf;
    if (keyMap[word] & (1 << bit)) {
        quickKeyCheckResult = true;
        return 1;
    }
    return 0;
}
#else
{
    Boolean result;

    if (ignoreKeys)
        return false;
    result = isPressed(keyMap, code);
    if (result)
        quickKeyCheckResult = true;
    return result;
}
#endif
#endif

/*
 * The 1.0.10 body carries no escort logic, so the name stays descriptive rather
 * than adopting the mac's _QuickKeyCheckPlusEscorts.
 */
#ifndef MACBUILD
/* RawKeyCheck. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469cf0
unsigned char RawKeyCheck(short scanCode)
{
    if (ignoreKeys)
        return 0;
    return KeyIsDown(scanCode);
}
#endif

/* KeyCheck (mac _KeyCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469d10
// MAC: _KeyCheck i386 0x0000a19b ppc 0x0000c7c8
#ifdef MACBUILD
unsigned char KeyCheck(short scanCode)
{
    KeyMap keys;

    if (ignoreKeys)
        return 0;
    GetKeys(keys);
#ifdef MAC1010
    {
        short word = (scanCode >> 4) & 0xf;
        short bit = scanCode & 0xf;

        return (((unsigned short *)keys)[word] & (1 << bit)) != 0;
    }
#else
    return isPressed((unsigned char *)keys, scanCode);
#endif
}
#else
unsigned char KeyCheck(short scanCode)
{
    KeyboardKbhit();
    return KeyIsDown(scanCode);
}
#endif

#define kDemandMedium 2

/* CalcPortDemand. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469d30
// MAC: _CalcPortDemand i386 0x000056b1 ppc 0x00006d04
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

/* SetupPortAvailableShipTypes (mac _SetupPortAvailableShipTypes). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00469e90
// MAC: _SetupPortAvailableShipTypes i386 0x0000bbe3 ppc 0x0000e880
void SetupPortAvailableShipTypes(SpobRecord *port)
{
#if V111 /* V111 spelling */
    short count;
    short threshold;
    short i;
    short j;
    short k;
    short n;
    short best;
    short a;
#else
    short i;
    short j;
    short k;
    short threshold;
    short best;
#endif
    Boolean avail[kMaxShipTypes];

    if (port == 0)
        return;

#if V111 /* V111 spelling */
    for (k = 0; k < kMaxShipTypes; k++)
        dockedPortShipTypes[k] = -1;

    count = 0;
#else
    for (i = 0; i < kMaxShipTypes; i++)
        dockedPortShipTypes[i] = -1;

    k = 0;
#endif
    for (i = 0; i < kMaxShipTypes; i++) {
        avail[i] = 0;
        if (shipClass[i].techLevel >= 0) {
            if (shipClass[i].techLevel <= port->techLevel)
                avail[i] = 1;
            else {
                for (j = 0; j < kPortSpecialTech; j++) {
                    if (port->specialTech[j] == shipClass[i].techLevel) {
                        avail[i] = 1;
                        break;
                    }
                }
            }
        }

        if (avail[i]) {
            if (shipBuyMode == 0) {
                if (shipClass[i].buyRandom == 0
                        || shipClass[i].dailySaleRoll > shipClass[i].buyRandom)
                    avail[i] = 0;
            } else {
                if (shipClass[i].hireRandom == 0
                        || shipClass[i].dailyHireRoll
                                * (shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0)
                                > shipClass[i].hireRandom)
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (shipClass[i].flags3 & kShipHideIfRequireUnmet) {
                if (!PlayerMeetsRequirements(shipClass[i].requires[0],
                        shipClass[i].requires[1]))
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (shipClass[i].flags3 & kShipHideIfUnavailable) {
                if (!EvalMissionBitTestString(shipClass[i].availability))
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
#if V111 /* V111 spelling */
            if (shipClass[i].dispWeight > threshold || count == 0)
                threshold = shipClass[i].dispWeight;
            count++;
#else
            if (shipClass[i].dispWeight > threshold || k == 0)
                threshold = shipClass[i].dispWeight;
            k++;
#endif
        }
    }

#if V111 /* V111 spelling */
    for (a = 0; a < kMaxShipTypes; a++) {
        if (avail[a] && (shipClass[a].flags3 & kShipSupersedes)) {
            for (k = a + 1; k < kMaxShipTypes; k++) {
                if (shipClass[k].dispWeight == shipClass[a].dispWeight)
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
                if (shipClass[n].dispWeight >= threshold) {
                    dockedPortShipTypes[k++] = n;
                    avail[n] = 0;
                } else if (shipClass[n].dispWeight > best || count == 0) {
                    best = shipClass[n].dispWeight;
                }
                count++;
            }
        }
        threshold = best;
    } while (count > 0);
#else
    for (i = 0; i < kMaxShipTypes; i++) {
        if (avail[i] && (shipClass[i].flags3 & kShipSupersedes)) {
            for (k = i + 1; k < kMaxShipTypes; k++) {
                if (shipClass[k].dispWeight == shipClass[i].dispWeight)
                    avail[k] = 0;
            }
        }
    }

    k = 0;
    do {
        j = 0;
        best = 0;
        for (i = 0; i < kMaxShipTypes; i++) {
            if (avail[i]) {
                if (shipClass[i].dispWeight >= threshold) {
                    dockedPortShipTypes[k] = i;
                    avail[i] = 0;
                    k++;
                } else if (shipClass[i].dispWeight > best || j == 0) {
                    best = shipClass[i].dispWeight;
                }
                j++;
            }
        }
        threshold = best;
    } while (j > 0);
#endif
}


/* SetupPortAvailableItems (mac _SetupPortAvailableItems). STATE: MATCH, 208/208,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a220
// MAC: _SetupPortAvailableItems i386 0x0000bedb ppc 0x0000ebd0
Boolean SetupPortAvailableItems(SpobRecord *port)
{
    short i;
    short j;
    short count;
    short threshold;
    short best;
    short saved[kMaxOutfits];
    Boolean avail[kMaxOutfits];
    short n;

    if (port == 0)
        return 1;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        saved[i] = dockedPortItems[i];
        dockedPortItems[i] = -1;
        avail[i] = 0;
        if ((port->flags2 & kSpobBuysAnyOutfit) && shipSystems[i] > 0
                && !(outfitSystem[i].flags & kOutfCantSell))
            avail[i] = 1;
        if (outfitSystem[i].techLevel >= 0
                && outfitSystem[i].techLevel < kOutfTechAbsent) {
            if (outfitSystem[i].techLevel <= port->techLevel)
                avail[i] = 1;
            else {
                for (j = 0; j < kPortSpecialTech; j++) {
                    if (port->specialTech[j] == outfitSystem[i].techLevel) {
                        avail[i] = 1;
                        break;
                    }
                }
            }
        }

        if (avail[i]) {
            if ((outfitSystem[i].flags & kOutfTestAvailability)
                    && shipSystems[i] <= 0)
                avail[i] = EvalMissionBitTestString(outfitSystem[i].availability);
        }

        if (avail[i]) {
            if ((outfitSystem[i].flags & kOutfCheckRequire)
                    && shipSystems[i] <= 0)
                avail[i] = PlayerMeetsRequirements(outfitSystem[i].requires[0],
                        outfitSystem[i].requires[1]);
        }

        if (avail[i]) {
            if (shipSystems[i] <= 0) {
                if (outfitSystem[i].buyRandom <= 0
                        || outfitSystem[i].dailyRoll
                                > outfitSystem[i].buyRandom)
                    avail[i] = 0;
            } else
                outfitSystem[i].dailyRoll = 0;
        }

        if (outfitSystem[i].flags & kOutfSellAnywhere) {
            if (shipSystems[i] > 0)
                avail[i] = 1;
        }

        if (avail[i]) {
            if (outfitSystem[i].dispWeight > threshold || count == 0)
                threshold = outfitSystem[i].dispWeight;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (avail[i] && (outfitSystem[i].flags & kOutfSupersedes)) {
#if V111 /* V111 spelling */
            for (j = i + 1; j < kMaxOutfits; j++) {
                if (outfitSystem[j].dispWeight == outfitSystem[i].dispWeight)
                    avail[j] = 0;
            }
#else
            for (count = i + 1; count < kMaxOutfits; count++) {
                if (outfitSystem[count].dispWeight == outfitSystem[i].dispWeight)
                    avail[count] = 0;
            }
#endif
        }
    }

    count = 0;
    do {
        j = 0;
        best = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (avail[i]) {
                if (outfitSystem[i].dispWeight >= threshold) {
#if V111 /* V111 spelling */
                    dockedPortItems[count++] = i;
                    avail[i] = 0;
#else
                    dockedPortItems[count] = i;
                    avail[i] = 0;
                    count++;
#endif
                } else {
                    n = outfitSystem[i].dispWeight;
                    if (n > best || j == 0)
                        best = n;
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
// MAC: _TotalCargo i386 0x00004a0b ppc 0x00005e3c
short TotalCargo(ShipObj *ship)
{
    short i;
    short total;

    total = 0;
    for (i = 0; i <= 5; i++)
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
// MAC: _PlayerHasAnyCargo i386 0x00005d77 ppc 0x000074f0
Boolean PlayerHasAnyCargo(void)
{
    short i;

    for (i = 0; i <= 5; i++) {
        if (shipData[0].cargo[i] > 0)
            return 1;
    }
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0)
            return 1;
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0)
            return 1;
    }
    return 0;
}

#ifdef MACBUILD
/* InstantFighterRecall (mac _InstantFighterRecall, mac-only). STATE: see the mac gate. */
// MAC: _InstantFighterRecall i386 0x00005df3 ppc 0x000075ac
void InstantFighterRecall(void)
{
    short i;
    short j;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active && shipData[i].parentShip == 0 && shipData[i].aiType == 5
                && shipData[i].specialMission == -1) {
            for (j = 0; j < kMaxWeapTypes; j++) {
                if (weaponStats[j].ammoType == shipData[i].type + kFirstResourceID && weaponStats[j].guidance == kGuidanceBay) {
                    shipData[0].weap[j].ammo++;
                    statusRedraw[3] = 1;
                    break;
                }
            }
        }
    }
}
#endif

/* the player's own hold capacity, outfit modifiers included */
/* ShipTotalHolds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a730
// MAC: _ShipTotalHolds i386 0x00005cb3 ppc 0x0000740c
short ShipTotalHolds(void)
{
    short holds;
    short i;
    short j;

    holds = shipClass[shipData->type].cargo;
    for (i = 0; i < 512; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kOutfModCargo)
                    holds += outfitSystem[i].modVal[j] * shipSystems[i];
            }
        }
    }
    return holds;
}

/* TotalMissionCargoSpace. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046a7c0
// MAC: _TotalMissionCargoSpace i386 0x0000cffb ppc 0x00010310
short TotalMissionCargoSpace(void)
{
    short holds;
    short fleetHolds;
    short cargo;
    short missionUnits;
    short i;
    short spare;
    short result;

    holds = ShipTotalHolds();
    fleetHolds = TotalFleetHolds();
    cargo = TotalCargo(shipData);
    missionUnits = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
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
// MAC: _RefreshSAT i386 0x00006b59 ppc 0x000085e0
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
    ForeColor(blackColor);
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);

    if (playerIsLanded == 0)
        SWUpdateSpriteWorld(gSpriteWorld, 0);

    if (commCount >= 0) {
        RedrawComm(1);
        SWSetPortToWindow(gSpriteWorld);
#ifdef V111
        if (useLegacyRenderingPipeline)
#endif
            BlitPixieFloatingRectDrawProc(gSpriteWorld->backFrameP, gSpriteWorld->workFrameP,
                         &commRect, &commRect, 0, gSpriteWorld->backgroundValue);
    }

#ifdef MACBUILD
    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
             GetPortPixMapBits(GetWindowPort(gWind)),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);
#else
    CopyBits(GetPortBitMapForCopyBits(*(CGrafPtr *)gSpriteWorld->workFrameP),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);
#endif

    r = GetCWindowPortRect((WindowRef)gWind);
    box = *r;
#ifdef CEFIX /* F14.14 */
    box.left = box.right - ceStatusBarWidth;
#else
    box.left = box.right - kStatusStripWidth;
#endif
    strip = box;
    OffsetRect(&strip, -strip.left, -strip.top);

    RedrawStatus(0);
    DrawStatusRadar(0);
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortPixMapBits(GetWindowPort(gWind)),
             &box, &box, 0, 0);
#else
    CopyBits((BitMapPtr)GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &box, &box, 0, 0);
#endif

    if (playerIsLanded == 0) {
        RedrawComm(0);
        FloaterDrawCallback(gSpriteWorld);
    }
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    QDFlushPortBuffer(GetWindowPort(gWind), NULL);
#else
    ScreenEndScene();
#endif
}

/* PlaySound (mac _PlaySound). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046aad0
// MAC: _PlaySound i386 0x00005e9a ppc 0x0000768c
void PlaySound(void *a, short b, short lo, short hi)
{
    ST_PlaySound(a, b, (lo + hi) / 2);
}

#ifdef MACBUILD
/* ShowFreeMem (mac _ShowFreeMem, mac-only). STATE: see the mac gate. */
// MAC: _ShowFreeMem i386 0x00005ec3 ppc 0x000076a4
void ShowFreeMem(void)
{
    FreeMem();
}
#endif

/* AdjustWarpSoundSpeed. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ab00
// MAC: _AdjustWarpSoundSpeed i386 0x00005ecc ppc 0x000076a8
void AdjustWarpSoundSpeed(double rate)
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
// MAC: _AdjustSoundVolume i386 0x00005efb ppc 0x000076e4
void AdjustSoundVolume(void)
{
    gSoundVolume = settings.volume * 8;
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
// MAC: _ShipAntiInterference i386 0x00005f2c ppc 0x0000772c
short ShipAntiInterference(void)
{
    short i;
    short j;
    short total;

    if (playerAntiInterference < -100) {
        total = 0;
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType24)
                        total += outfitSystem[i].modVal[j] * shipSystems[i];
                }
            }
        }
        if (total > 100)
            total = 100;
        if (total < -100)
            total = -100;
        playerAntiInterference = total;
        return total;
    }
    return playerAntiInterference;
}

/* SpawnNagShip. STATE: MATCH, 327/327; byte check pending the next regate. OPEN: the
   `(short)' on the Hector scan's bound is an unexplained device -- the exe compares that
   counter at 16 bits and indexes the weapon loop with the same long `w'. see notes */
// FUNCTION: EVNOVA 0x0046ac50
// MAC: _SpawnNagShip i386 0x00005fdb ppc 0x000077d8
void SpawnNagShip(void)
{
    long chance;
    short slot;
    long w;
    short heading;
    float step;
    float total;
    FloatPoint origin;
    unsigned char msg[256];

    if (personData[kPersCaptainHector].shipType < 0)
        return;

    if (totalDaysPlayed <= 14)
        chance = 25000;
    else if (totalDaysPlayed < 30)
        chance = 10000;
    else if (totalDaysPlayed < 60)
        chance = 4000;
    else
        chance = 500;

    chance = chance * (1 - ((shipClass[gameIsRegisteredIndex].registered != 0) ? 1 : 0));

    if (chance == 0) {
        if (((shipClass[gameIsHackedIndex].hacked != 0) ? 1 : 0) == 0)
            return;
    }

#ifdef MACBUILD
    for (slot = 1; slot < kMaxShips; slot++) {
        if (shipData[slot].active != 0 && shipData[slot].persType == kPersCaptainHector)
            return;
    }
#else
    for (w = 1; (short)w < kMaxShips; w++) {
        if (shipData[w].active != 0 && shipData[w].persType == kPersCaptainHector)
            return;
    }
#endif

    if ((chance > 0 && Rand(chance) == 0) ||
        ((shipClass[gameIsHackedIndex].hacked != 0) ? 1 : 0)) {
        slot = GenericRandomShipSpawn(shipData->curSystem, 2);
        if (slot != -1) {
            if (totalDaysPlayed > 30) {
            ST_PlaySound(beepSnd[4], 1, gSoundVolume);
            GetIndString(msg, kHackedStrList, Rand(3) + 5);
            DisplayComm((PStrChar *)msg, 0x168, 0, 12, &rgbWhite, 0, 0);
            }

            shipData[slot].persType = kPersCaptainHector;
            shipData[slot].type = personData[kPersCaptainHector].shipType;
            shipData[slot].aiType = 4;

            total = 0.0f;
            if (totalDaysPlayed >= 60)
                step = 31.0f;
            else
                step = 50.0f;
            while (step > 0.0f) {
                total += step;
                step -= 1.165;
            }

            shipData[slot].pos.x = shipData->pos.x;
            shipData[slot].pos.y = shipData->pos.y;

            if (totalDaysPlayed >= 60) {
            heading = shipData->heading;
            } else {
            origin.x = origin.y = 0.0f;
            heading = CalcInterceptAngle(&shipData->pos, &origin);
            }
            Accel(heading, -(total + 500.0), &shipData[slot].pos);

            shipData[slot].heading = heading;
            shipData[slot].dude = -1;
            shipData[slot].aggression = 4;
            shipData[slot].govt = -1;

            for (chance = 0; chance < 256; chance++) {
                shipData[slot].weap[chance].count = shipClass[shipData[slot].type].weapCount[chance];
                shipData[slot].weap[chance].ammo = shipClass[shipData[slot].type].weapAmmo[chance];
                shipData[slot].weap[chance].count += personData[kPersCaptainHector].weapCount[chance];
                shipData[slot].weap[chance].ammo += personData[kPersCaptainHector].weapAmmo[chance];
            }

            shipData[slot].shield = shipClass[shipData[slot].type].shield;
            shipData[slot].armor = shipClass[shipData[slot].type].armor;
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

#ifdef MACBUILD
/* LoadFrameFromCICN (mac _LoadFrameFromCICN, mac-only). STATE: see the mac gate. */
// MAC: _LoadFrameFromCICN i386 0x00006497 ppc 0x00007d38
void *LoadFrameFromCICN(short id)
{
    return 0;
}
#endif

/* the shorter way round a 360-unit circle */

/* AngularDifference. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b210
// MAC: _AngularDifference i386 0x0000649e ppc 0x00007d40
short AngularDifference(short a, short b)
{
    short diff;

    diff = abs(a - b);
    if ((a >= 0xb4) != (b >= 0xb4))
        diff = 0x168 - diff;
    if (diff > 0xb4)
        diff = 0x168 - diff;
    return diff;
}

/* AIHasAfterburner. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b260
// MAC: _AIHasAfterburner i386 0x000064f5 ppc 0x00007d80
Boolean AIHasAfterburner(ShipObj *ship)
{
    short i;
    short flags;

    if (ship->index != 0) {
        for (i = 1; i < 64; i++) {
            if (i != ship->index && shipData[i].active != 0
                    && ship->index == shipData[i].swarmLeader)
                return 0;
        }
    }
    flags = shipClass[ship->type].flags;
    if ((flags & kShipPlanetType) != 0)
        return 0;
    if ((flags & kShipAlwaysAfterburner) != 0)
        return 1;
    if ((flags & kShipAfterburnerIfRated) != 0
            && playerKills / shipClass[0].strength
               >= Rand(0x540) + 0x100)
        return 1;
    return 0;
}

/* true for all but the top few hundred of the 0x540 range */
/* AIPerformsComplexManeuvers (mac _AIPerformsComplexManeuvers). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b330
// MAC: _AIPerformsComplexManeuvers i386 0x0000659a ppc 0x00007e78
Boolean AIPerformsComplexManeuvers(ShipObj *ship)
{
    if (playerKills >= Rand(0x540) + 0x100)
        return 1;
    return 0;
}

/* TurretBlindSpot. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b360
// MAC: _TurretBlindSpot i386 0x0000b325 ppc 0x0000ddb8
unsigned char TurretBlindSpot(ShipObj *ship, short heading, short weap)
{
    short facing;
    short diff;
    unsigned char blocked;

    blocked = 0;
    facing = ship->heading;
    diff = AngularDifference(facing, heading);
    if (diff <= 0x2d) {
        if (weaponStats[weap].flags & kWeapTurretBlindFront)
            blocked = 1;
        if (shipClass[ship->type].flags & kShipTurretBlindFront)
            blocked = 1;
    } else if (diff <= 0x87) {
        if (weaponStats[weap].flags & kWeapTurretBlindSide)
            blocked = 1;
        if (shipClass[ship->type].flags & kShipTurretBlindSide)
            blocked = 1;
    } else {
        if (weaponStats[weap].flags & kWeapTurretBlindRear)
            blocked = 1;
        if (shipClass[ship->type].flags & kShipTurretBlindRear)
            blocked = 1;
    }
    return blocked;
}

#define kDudeSlots 16

/* SelectShipFieldFromDude. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b4b0
// MAC: _SelectShipFieldFromDude i386 0x000065c2 ppc 0x00007ebc
short SelectShipFieldFromDude(DudeRecord *dude, unsigned char allowAll)
{
    short cum[kDudeSlots];
    short pick;
    short i;
    short count;
    short total;
    short roll;
    short j;

    total = 0;
    pick = -1;
    count = 0;
    for (i = 0; i < kDudeSlots; i++) {
        cum[i] = 0;
        if (dude->shipTypes[i] != -1
            && (shipClass[dude->shipTypes[i]].canAppear != 0 || allowAll != 0)) {
            total += dude->shipProbs[i];
            for (j = 0; j <= i; j++) {
                if (dude->shipTypes[j] != -1
                    && (shipClass[dude->shipTypes[j]].canAppear != 0
                        || allowAll != 0)) {
                    cum[i] += dude->shipProbs[j];
                    count++;
                }
            }
        }
    }
    if (total <= 0 || count == 0)
        return -1;
    while (pick == -1) {
        roll = Rand(total) + 1;
        for (i = kDudeSlots - 1; i >= 0; i--) {
            if (roll <= cum[i] && dude->shipTypes[i] != -1
                && (shipClass[dude->shipTypes[i]].canAppear != 0
                    || allowAll != 0))
                pick = i;
        }
    }
    return pick;
}

#define kSystDudeSlots 8

/* SelectDudeFieldFromSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b600
// MAC: _SelectDudeFieldFromSystem i386 0x0000674d ppc 0x00008070
short SelectDudeFieldFromSystem(SystRecord *sys)
{
    short cum[kSystDudeSlots];
    short pick;
    short i;
    short roll;
    short j;
    short total;

    total = 0;
    pick = -1;
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
    while (pick == -1) {
        roll = Rand(total) + 1;
        for (i = kSystDudeSlots - 1; i >= 0; i--) {
            if (roll <= cum[i] && sys->dudeTypes[i] >= 0
                && sys->dudeTypes[i] < kMaxDudes)
                pick = i;
        }
    }
    return pick;
}

/* SelectFleetFromSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b6d0
// MAC: _SelectFleetFromSystem i386 0x0000682c ppc 0x00008198
short SelectFleetFromSystem(SystRecord *syst)
{
    short i;
    short picks[8];
    short probs[8];
    short g;
    short total;
    short k;

    if (syst->fleetCount <= 0)
        return -1;

    g = 0;
    total = 0;
    for (i = 0; i < syst->fleetCount; i++) {
        if (fleetData[syst->fleets[i]].leadShipType != -1 && fleetData[syst->fleets[i]].usable != 0) {
            picks[g] = syst->fleets[i];
            probs[g] = syst->fleetProbs[i];
            total += syst->fleetProbs[i];
            for (k = 0; k < g; k++)
                probs[g] += probs[k];
            g++;
        }
    }

    if (g == 0 || total == 0)
        return -1;

    k = Rand(total) + 1;
    for (i = 0; i < g; i++) {
        if (k <= probs[i])
            return picks[i];
    }
    return -1;
}

#ifdef MACBUILD
/* FindDecayedShotPower (mac _FindDecayedShotPower, mac-only). STATE: see the mac gate. */
// MAC: _FindDecayedShotPower i386 0x0000691b ppc 0x000082d4
float FindDecayedShotPower(ShotObj *shot)
{
    return 1.0f;
}
#endif

/* the shipyard's asking-price jitter, as a fraction around 1.00 */

/* RandomSkillLevel. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b870
// MAC: _RandomSkillLevel i386 0x00006922 ppc 0x000082e0
float RandomSkillLevel(short type)
{
    return (Rand(shipClass[type].skillVar * 2 + 1)
            + (100 - shipClass[type].skillVar)) * 0.01;
}

/* the player, or something escorting the player directly or one level down */
/* AlliedWithPlayer. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b8d0
// MAC: _AlliedWithPlayer i386 0x00006985 ppc 0x00008378
Boolean AlliedWithPlayer(ShipObj *ship)
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

#ifdef MACBUILD
/* DebugInt (mac _DebugInt, mac-only). STATE: see the mac gate. */
// MAC: _DebugInt i386 0x000069d1 ppc 0x000083d8
void DebugInt(long n)
{
    Str255 s;

    NumToString(n, s);
    DebugStr(s);
}
#endif

/* the first system of the group that has actually been loaded */
/* FindActiveCoLocatedSystem (mac _FindActiveCoLocatedSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b920
// MAC: _FindActiveCoLocatedSystem i386 0x00004be0 ppc 0x000060b8
short FindActiveCoLocatedSystem(short sys)
{
    short next;

    if (sys < 0 || sys >= 2048)
        return -1;
    if (systemData[sys].colocGroup == -1) {
        if (systemData[sys].loaded != 0)
            return sys;
        return -1;
    }
    next = systemData[sys].colocGroup;
    do {
        if (systemData[next].loaded != 0)
            return next;
        next = systemData[next].colocNext;
    } while (next != -1);
    return -1;
}

/* FindFirstCoLocatedSystem (mac _FindFirstCoLocatedSystem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b9b0
// MAC: _FindFirstCoLocatedSystem i386 0x00004baa ppc 0x0000607c
short FindFirstCoLocatedSystem(short sys)
{
    if (sys == -1)
        return -1;
    if (systemData[sys].colocGroup == -1)
        return sys;
    return systemData[sys].colocGroup;
}

/* HideTheCursor (mac _HideTheCursor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046b9f0
// MAC: _HideTheCursor i386 0x00006a01 ppc 0x00008404
#ifdef MACBUILD
void HideTheCursor(void)
{
#if !TARGET_API_MAC_CARBON
    if (CrsrVis)
        cursorHidden = 0;
#endif
    if (cursorHidden < 10) {
        cursorHidden++;
        HideCursor();
    }
}
#else
void HideTheCursor(void)
{
    OSCursorHide();
    cursorHidden = (OSCursorIsVisible() == 0);
}
#endif

/* ShowTheCursor (mac _ShowTheCursor). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ba10
// MAC: _ShowTheCursor i386 0x00006a1f ppc 0x00008424
#ifdef MACBUILD
void ShowTheCursor(void)
{
    if (cursorHidden > 0) {
        cursorHidden = 0;
        InitCursor();
    }
}
#else
void ShowTheCursor(void)
{
    OSCursorShow();
    cursorHidden = (OSCursorIsVisible() == 0);
}
#endif

/* FindNearestValidShotTarget. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ba30
// MAC: _FindNearestValidShotTarget i386 0x00006a3a ppc 0x00008444
short FindNearestValidShotTarget(ShotObj *shot)
{
    short i;
    short best;
    short bestDist;
    short dx;
    short dy;
    short dist;

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
// MAC: _SetSystemBackgroundColor i386 0x00006af4 ppc 0x00008568
void SetSystemBackgroundColor(void)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    short sys, x;

    sys = x = shipData[0].curSystem;
    sys &= ~(x >> 31);
#else
    short sys;

    sys = shipData[0].curSystem;
    if (sys < 0)
        sys = 0;
#endif
    RGBForeColor(&systemData[sys].rgb);
    SWSetBackgroundColor(gSpriteWorld, &systemData[sys].rgb);
}

/* SetSystemBackgroundBackColor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046bc50
// MAC: _SetSystemBackgroundBackColor i386 0x00006e06 ppc 0x00008850
void SetSystemBackgroundBackColor(void)
{
    if (shipData[0].curSystem < 0) {
        BackColor(blackColor);
        return;
    }
    RGBBackColor(&systemData[shipData[0].curSystem].rgb);
}

/* GovtAllies (mac _GovtAllies). STATE: MATCH, exe 113 / obj 113, byte-identical yes
   (relocs masked). OPEN: none. Guard order `a,a,b,b' is the exe's -- do not normalise it
   to the twin's. see notes */
// FUNCTION: EVNOVA 0x0046bc90
// MAC: _GovtAllies i386 0x00004e3d ppc 0x00006390
Boolean GovtAllies(short a, short b)
{
    short i;
    short j;

    if (a == b)
        return 1;
    if (a >= 0 && a < kMaxGovts && b >= 0 && b < kMaxGovts) {
        if ((govtData[a].flags & kGovtAlwaysDisabled) != 0 ||
            (govtData[b].flags & kGovtAlwaysDisabled) != 0)
            return 0;

        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[b].allies[j] >= 0
                            && govtData[b].allies[j] == govtData[a].classes[i])
                        return 1;
                }
            }
            if (govtData[b].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[a].allies[j] >= 0
                            && govtData[a].allies[j] == govtData[b].classes[i])
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
// MAC: _GovtEnemies i386 0x00004f22 ppc 0x0000647c
Boolean GovtEnemies(short a, short b)
{
    short i;
    short j;

    if (a == b)
        return 0;
    if (a >= 0 && b >= 0 && a < kMaxGovts && b < kMaxGovts) {
        if ((govtData[a].flags & kGovtAlwaysDisabled) != 0 ||
            (govtData[b].flags & kGovtAlwaysDisabled) != 0)
            return 0;

        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[b].enemies[j] >= 0
                            && govtData[b].enemies[j] == govtData[a].classes[i])
                        return 1;
                }
            }
            if (govtData[b].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[a].enemies[j] >= 0
                            && govtData[a].enemies[j] == govtData[b].classes[i])
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
// MAC: _GovtSharedClass i386 0x00006e48 ppc 0x0000888c
Boolean GovtSharedClass(short a, short b)
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
// MAC: _ShipDeionizationRate i386 0x00006ec3 ppc 0x00008908
float ShipDeionizationRate(ShipObj *ship)
{
    short i;
    short j;
    float value;

    value = shipClass[ship->type].deionize;
    if (ship->index == 0) {
        if (playerDeionizeRate >= 0.0f) {
            value = playerDeionizeRate;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType39)
                            value = value + outfitSystem[i].modVal[j] * 0.01 *
                                    shipSystems[i];
                    }
                }
            }
            playerDeionizeRate = value;
        }
    }
    return value;
}

#define kOutfModType40 0x28

/* ShipIonizationFactor. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c160
// MAC: _ShipIonizationFactor i386 0x00003225 ppc 0x00004094
float ShipIonizationFactor(ShipObj *ship)
{
    short i;
    short j;
    float total;

    total = shipClass[ship->type].ionizeMax;
    if (ship->index == 0) {
        if (playerIonizeMax >= 0.0f) {
            total = playerIonizeMax;
        } else {
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType40)
                            total = total + outfitSystem[i].modVal[j] *
                                    shipSystems[i];
                    }
                }
            }
            playerIonizeMax = total;
        }
    }
    if (total > 0.0f)
        return ship->ionization / total;
    return 0.0f;
}

#define kOutfModType28 0x1c

/* PlayerEffectiveMurk (mac _PlayerEffectiveMurk). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c250
// MAC: _PlayerEffectiveMurk i386 0x00006f8f ppc 0x00008a18
short PlayerEffectiveMurk(void)
{
    short i;
    short j;
    short level;

    level = systemData[shipData[0].curSystem].murk;
    if (level < 0)
        level = 0;
    for (i = 0; i < 512; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kOutfModType28)
                    level += outfitSystem[i].modVal[j] * shipSystems[i];
            }
        }
    }
    if (level < 0)
        level = 0;
    if (level > 100)
        level = 100;
    return level;
}

/* WeaponDisableOnly. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c2f0
// MAC: _WeaponDisableOnly i386 0x0000704c ppc 0x00008ad0
Boolean WeaponDisableOnly(short weap)
{
    if ((weaponStats[weap].flags2 & kWeapNotDestroying) != 0)
        return 1;
    return 0;
}

/* ModifyShotStartPosition (mac _ModifyShotStartPosition). STATE: MATCH 100%, exe 130 / obj 130,
   byte-identical yes (relocs masked). OPEN: none. */
// FUNCTION: EVNOVA 0x0046c320
// MAC: _ModifyShotStartPosition i386 0x00007348 ppc 0x00008df0
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
    group = weaponStats[weapType].exitType;
    if (group >= 0 && group <= 3) {
        angle = ship->sprite->curFrameIndex % shipClass[ship->type].framesPer;
        angle *= 360.0 / (float)shipClass[ship->type].framesPer;

        if ((weaponStats[weapType].flags3 & kWeapClosestExit) && target != 0) {
            ship->weapGroupLane[group] = SelectClosestShotStartPosition(
                ship, base, weapType, angle, group, target);
        }

        if (ship->weapGroupLane[group] < 0 || ship->weapGroupLane[group] > 3)
            ship->weapGroupLane[group] = Rand(4);

        ModifyShotStartPosition2(ship, base, angle, group, ship->weapGroupLane[group]);

#ifdef V111 /* V111 spelling */
        result = ship->weapGroupLane[group];
        ship->weapGroupLane[group] = ((result + 1) % 4);
#else
        lane = ship->weapGroupLane[group];
        result = lane;
        ship->weapGroupLane[group] = ((lane + 1) % 4);
#endif

        if (ship->weapGroupLane[group] < 0 || ship->weapGroupLane[group] > 3)
            ship->weapGroupLane[group] = Rand(4);
    }

    return result;
}

/* SelectClosestShotStartPosition (mac _SelectClosestShotStartPosition). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c4e0
// MAC: _SelectClosestShotStartPosition i386 0x00007254 ppc 0x00008d10
short SelectClosestShotStartPosition(ShipObj *ship, FloatPoint *base, short unused,
                                    short weapType, short mount,
                                    FloatPoint *target)
{
    short best;
    short i;
    FloatPoint pos;
    FloatPoint start;
    float d;
    float bestDist;

    if (ship == 0 || base == 0 || target == 0 || mount < 0 || mount > 3)
        return 0;
    start.x = base->x;
    start.y = base->y;
    best = -1;
    for (i = 0; i <= 3; i++) {
        pos.x = start.x;
        pos.y = start.y;
        ModifyShotStartPosition2(ship, &pos, weapType, mount, i);
        d = XYFloatSquaredDist(&pos, target);
        if (d < bestDist || best == -1) {
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
// MAC: _ModifyShotStartPosition2 i386 0x0000706e ppc 0x00008af0
void ModifyShotStartPosition2(ShipObj *ship, FloatPoint *pos, short angle,
                              short mount, short lane)
{
    FloatPoint d;

    if (ship == 0 || pos == 0 || mount < 0 || mount > 3 || lane < 0 || lane > 3)
        return;

    d.x = d.y = 0;
    Accel(angle, shipClass[ship->type].weapPos[mount][lane].v, &d);
    Accel(((angle + 90) % 360),
          shipClass[ship->type].weapPos[mount][lane].h, &d);

    if (d.y < 0.0) {
        d.x = d.x * shipClass[ship->type].compress[0].x;
        d.y = d.y * shipClass[ship->type].compress[0].y;
    } else {
        d.x = d.x * shipClass[ship->type].compress[1].x;
        d.y = d.y * shipClass[ship->type].compress[1].y;
    }
    d.y = d.y - shipClass[ship->type].weapPosZ[mount][lane];

    pos->x = pos->x + d.x;
    pos->y = pos->y + d.y;
}

/* ShipIsCloaked (mac _ShipIsCloaked). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c7a0
// MAC: _ShipIsCloaked i386 0x0000255f ppc 0x00002f20
Boolean ShipIsCloaked(ShipObj *ship)
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

/* CronDateInRange. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c800
// MAC: _CronDateInRange i386 0x000074d3 ppc 0x00008fa0
Boolean CronDateInRange(CronRecord *cron)
{
    if (cron->firstYear > 0 && gameTime.year < cron->firstYear)
        return 0;
    if (cron->firstMonth == 0) {
        if (cron->firstDay <= 0)
            goto firstShared;
        if (gameTime.day < cron->firstDay)
            return 0;
    } else {
firstShared:
        if (cron->firstMonth > 0 && cron->firstDay == 0) {
            if (gameTime.month < cron->firstMonth)
                return 0;
        } else if (cron->firstMonth > 0) {
            if (cron->firstDay > 0) {
                if (gameTime.month * 32 + gameTime.day < cron->firstMonth * 32 + cron->firstDay)
                    return 0;
            }
        }
    }
    if (cron->lastYear > 0 && gameTime.year > cron->lastYear)
        return 0;
    if (cron->lastMonth == 0) {
        if (cron->lastDay <= 0)
            goto lastShared;
        if (gameTime.day > cron->lastDay)
            return 0;
    } else {
lastShared:
        if (cron->lastMonth > 0 && cron->lastDay == 0) {
            if (gameTime.month > cron->lastMonth)
                return 0;
        } else if (cron->lastMonth > 0) {
            if (cron->lastDay > 0) {
                if (gameTime.month * 32 + gameTime.day > cron->lastMonth * 32 + cron->lastDay)
                    return 0;
            }
        }
    }
    return 1;
}

/* ShipCanTargetUntargetableShips. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046c930
// MAC: _ShipCanTargetUntargetableShips i386 0x0000238f ppc 0x00002c90
Boolean ShipCanTargetUntargetableShips(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerCanTargetUntargetableShips >= 0) {
            if (playerCanTargetUntargetableShips != 0)
                return 1;
        } else {
            playerCanTargetUntargetableShips = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType30 &&
                            (outfitSystem[i].modVal[j] & kOutfModVal30Bit2) != 0) {
                            playerCanTargetUntargetableShips = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType30 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _ShipCanTargetCloakedShips i386 0x00002466 ppc 0x00002dc0
Boolean ShipCanTargetCloakedShips(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerCanTargetCloakedShips >= 0) {
            if (playerCanTargetCloakedShips != 0)
                return 1;
        } else {
            playerCanTargetCloakedShips = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType30 &&
                            (outfitSystem[i].modVal[j] & kOutfModVal30Bit3) != 0) {
                            playerCanTargetCloakedShips = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType30 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
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
// MAC: _ShipCanScoop i386 0x000075c7 ppc 0x000090dc
Boolean ShipCanScoop(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType31)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
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
// MAC: _PutSpriteInLayer i386 0x0000766c ppc 0x000091c0
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
// MAC: _GetPlayerContributeBits i386 0x000076b2 ppc 0x00009224
void GetPlayerContributeBits(unsigned long *lo, unsigned long *hi)
{
    unsigned long bits[2];
    short i;

    bits[0] = shipClass[shipData->type].contributes[0];
    bits[1] = shipClass[shipData->type].contributes[1];
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != '\0' && rankData[i].loaded != '\0') {
            bits[0] |= rankData[i].contributes[0];
            bits[1] |= rankData[i].contributes[1];
        }
    }
    for (i = 0; i < kMaxCrons; i++) {
        if (cronEvent[i].active != '\0' && cronEvent[i].loaded != '\0'
                && cronEvent[i].holdoffLeft <= 0) {
            bits[0] |= cronEvent[i].contributes[0];
            bits[1] |= cronEvent[i].contributes[1];
        }
    }
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            bits[0] |= outfitSystem[i].contributes[0];
            bits[1] |= outfitSystem[i].contributes[1];
        }
    }
    *lo = bits[0];
    *hi = bits[1];
}

/* PlayerMeetsRequirements (mac _PlayerMeetsRequirements). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cd80
// MAC: _PlayerMeetsRequirements i386 0x0000776f ppc 0x00009334
Boolean PlayerMeetsRequirements(unsigned long reqLo, unsigned long reqHi)
{
    unsigned long bits[2];
    unsigned long gotLo;
    unsigned long gotHi;

    GetPlayerContributeBits(&bits[0], &bits[1]);
    gotLo = bits[0] & reqLo;
    gotHi = bits[1] & reqHi;
    if (gotLo == reqLo)
        if (gotHi == reqHi)
            return 1;
    return 0;
}

#define kOutfModType32 0x20

/* ShipJumpsPerJump (mac _ShipJumpsPerJump). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046cdd0
// MAC: _ShipJumpsPerJump i386 0x000077b4 ppc 0x00009394
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
                    if (outfitSystem[i].modType[j] == kOutfModType32)
                        total += outfitSystem[i].modVal[j];
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType32)
                        total += outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j];
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
// MAC: _ShipMaxWeaponRange i386 0x0000bafd ppc 0x0000e734
short ShipMaxWeaponRange(ShipObj *ship)
{
    long best;
    long range;
    short i;

    best = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count > 0 && WeaponHasAmmo(ship, i) != 0) {
            if (weaponStats[i].guidance == kGuidanceBeam
                    || weaponStats[i].guidance == kGuidanceTurretBeam) {
                range = weaponStats[i].beamLength;
            } else if (weaponStats[i].guidance == kGuidanceUnguided
                       || weaponStats[i].guidance == kGuidanceTurret
                       || weaponStats[i].guidance == kGuidanceFrontQuad
                       || weaponStats[i].guidance == kGuidanceFrontQuad) {
                range = weaponStats[i].totalRange;
            } else if (weaponStats[i].guidance == kGuidanceHoming) {
                range = (weaponStats[i].totalRange * 0.85);
            } else if (weaponStats[i].guidance == kGuidanceRocket) {
                range = (weaponStats[i].totalRange * 0.5);
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

/* ShipCanExpiditeJumps (mac _ShipCanExpiditeJumps). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d080
// MAC: _ShipCanExpiditeJumps i386 0x00007872 ppc 0x00009498
Boolean ShipCanExpiditeJumps(ShipObj *ship)
{
    short i;
    short j;

    if ((shipClass[ship->type].flags2 & kShipJumpsAtSpeed) != 0)
        return 1;
    if (ship->index == 0) {
        for (i = 0; i < 512; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType37)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
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
// MAC: _ShipsShareParent i386 0x00007926 ppc 0x00009588
Boolean ShipsShareParent(ShipObj *a, ShipObj *b)
{
    short rootA;
    short rootB;
    short next;

    if (a == 0 || b == 0)
        return 0;
#ifdef MAC1010
    rootA = a->index;
    while (shipData[rootA].active != 0 && (next = shipData[rootA].parentShip) >= 0
           && next < 64 && rootA != next)
        rootA = next;
    rootB = b->index;
    while (shipData[rootB].active != 0 && (next = shipData[rootB].parentShip) >= 0
           && next < 64 && rootB != next)
        rootB = next;
    return rootA == rootB;
#else
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
#endif
}

/* PtoCstr (mac _PtoCstr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d250
// MAC: _PtoCstr i386 0x000049e2 ppc 0x00005e04
#ifdef V111 /* V111: 1.1.1 defines it void */
void PtoCstr(unsigned char *str)
{
    short len = *str;
    short i;

    for (i = 0; i < len; i++)
        str[i] = str[i + 1];
    str[len] = 0;
}
#else
#if !defined(MAC1010) || TARGET_API_MAC_CARBON /* 1.0.10 Classic imports InterfaceLib's */
char *PtoCstr(unsigned char *str)
{
    short len = *str;
    short i;

    for (i = 0; i < len; i++)
        str[i] = str[i + 1];
    str[len] = 0;
    return (char *)str;
}
#endif
#endif

/* CtoPstr (mac _CtoPstr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046d380
// MAC: _CtoPstr i386 0x00004d13 ppc 0x00006210
#ifdef V111 /* V111: 1.1.1 defines it void */
void CtoPstr(char *str)
{
    short len = strlen(str);
    short i;

    for (i = len; i >= 1; i--)
        str[i] = str[i - 1];
    *str = len;
}
#else
#ifndef MAC1010
unsigned char *CtoPstr(char *str)
{
    short len = strlen(str);
    short i;

    for (i = len; i >= 1; i--)
        str[i] = str[i - 1];
    *str = len;
    return (unsigned char *)str;
}
#elif TARGET_API_MAC_CARBON /* 1.0.10 Classic imports InterfaceLib's; Carbon's tail returns nothing */
void CtoPstr(char *str)
{
    short len = strlen(str);
    short i;

    for (i = len; i >= 1; i--)
        str[i] = str[i - 1];
    *str = len;
}
#endif
#endif

/* junkData's record, padded to its true 0x526 stride */

/* the rank record, padded to its true 0x120 stride */

/* the modType values this routine reacts to */
#define kModCloak  0x1f
#define kModJam2C  0x2c
#define kModJam30  0x30
#define kModScan2F 0x2f
#define kModScan32 0x32


/* ResetPlayerPrecalcedValues. STATE: MATCH 100%, exe 587 / obj 587; relocregcheck 0 of 114
   differ; mac MATCH on both slices. OPEN: none. The licence code is read at each
   of the three checks; a `key' copy local was invented (2026-09-24). */
// FUNCTION: EVNOVA 0x0046d4b0
// MAC: _ResetPlayerPrecalcedValues i386 0x0000c357 ppc 0x0000f0c8
void ResetPlayerPrecalcedValues(void)
{
    short holds;
    short cargo;
    short i;
    short j;
    short m;
    short k;
    KeyCheckFlag keyed[3];

    playerShieldCapacity = -1.0f;
    playerShieldRechargeRate = -1.0f;
    playerArmorCapacity = -1.0f;
    playerArmorRechargeRate = -1.0f;
    playerTurnRate = -1.0f;
    playerAccelRate = -1.0f;
    playerMaxSpeed = -1.0f;
    playerFuelCapacity = -1.0f;
    playerIonizeMax = -1.0f;
    playerDeionizeRate = -1.0f;
    playerHasAfterburner = -1;
    playerHasDensityScanner = -1;
    playerHasIFF = -1;
    playerHasInertialDamper = -1;
    playerAntiInterference = 0x8300;
    playerEffectiveMurk = -1;
    playerCanTargetUntargetableShips = -1;
    playerCanTargetCloakedShips = -1;
    playerABFuelUsage = -1.0f;
    playerResistsGravity = -1;
    playerResistsDeadlyStellars = -1;
    playerHasAreaCloak = -1;
    playerHasIllegalJunk = 0;
    playerHasIllegalOutfits = 0;
    playerSabotaged = 0;
    playerHasTribbles = 0;
    playerHasPerishableJunk = 0;
    shipData->fuelRegenState = -1;
    shipData->canScoop = 0;

    KeyMismatch(keyed[0], shipData[5].licenseCode, shipData[5].licenseCode, 0x6e1bbb2b46dfcc17ULL, 0x0c9f3b380ff9e811LL);
    KeyMismatch(keyed[1], shipData[5].licenseCode, shipData[5].licenseCode, 0x1f1e62f77679a8c9ULL, 0x0e1f82a0c71946cbLL);
    KeyMismatch(keyed[2], shipData[5].licenseCode, shipData[5].licenseCode, 0xa63b2518c45f7e5bULL, 0x010d795bcf7c9f6aLL);
    for (i = 0; i < 3; i++) {
        if (keyed[i] == 0) {
            playerShieldCapacity = 1.0f;
            playerArmorCapacity = 1.0f;
        }
    }

    for (i = 0; i < 0x80; i++) {
        if (junkData[i].held > 0) {
            if (junkData[i].scanMask != 0)
                playerHasIllegalJunk = 1;
            if ((junkData[i].flags & kJunkTribbles) != 0)
                playerHasTribbles = 1;
            else if ((junkData[i].flags & kJunkPerishable) != 0)
                playerHasPerishableJunk = 1;
        }
    }

    for (i = 0; i < 0x200; i++) {
        if (shipSystems[i] > 0 && outfitSystem[i].scanMask != 0) {
            playerHasIllegalOutfits = 1;
            break;
        }
    }

    for (i = 0; i < 0x200; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kModScan2F) {
                    playerSabotaged = 1;
                    sabotageCounter = 0.0f;
                }
                if (outfitSystem[i].modType[j] == kModScan32) {
                    if (playerSabotaged < 1)
                        playerSabotaged = 2;
                    sabotageCounter = 0.0f;
                }
                if (outfitSystem[i].modType[j] == kModJam2C) {
                    if (outfitSystem[i].modVal[j] == -1) {
                        allReinforcementsInhibited = 1;
                    } else {
                        for (k = 0; k < 0x100; k++) {
                            for (m = 0; m < 4; m++) {
                                if (govtData[k].classes[m] ==
                                        outfitSystem[i].modVal[j] &&
                                    govtData[k].classes[m] != -1) {
                                    govtData[k].reinfInhibited = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (outfitSystem[i].modType[j] == kModJam30) {
                    for (k = 0; k < 0x100; k++) {
                        for (m = 0; m < 4; m++) {
                            if (govtData[k].classes[m] ==
                                    outfitSystem[i].modVal[j] &&
                                govtData[k].classes[m] != -1) {
                                govtData[k].iffScrambled = 1;
                                break;
                            }
                        }
                    }
                }
                if (outfitSystem[i].modType[j] == kModCloak)
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
    for (i = 0; i < 4; i++)
        shipData->jamming[i] = -1;
    holds = TotalFleetHolds();
    cargo = TotalCargo(shipData);
    if (cargo > holds) {
        for (i = 0; i <= 5; i++)
            shipData->cargo[i] *= holds / (float)cargo;
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
    shipData->seesCloaked[0] = -1;
    shipData->seesCloaked[1] = -1;
    shipData->decloaksWhenHit = -1;

    for (j = 0; j < 0x100; j++) {
        govtData[j].blessesPlayer[0] = 0;
        govtData[j].blessesPlayer[1] = 0;
    }
    for (i = 0; i < 0x80; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            (rankData[i].flags & (kRankNotAttacked | kRankCanAlwaysLand)) != 0 &&
            rankData[i].govt != -1) {
            for (j = 0; j < 0x100; j++) {
                if (GovtAllies(j, rankData[i].govt) != 0) {
                    if ((rankData[i].flags & kRankNotAttacked) != 0)
                        govtData[j].blessesPlayer[0] = 1;
                    if ((rankData[i].flags & kRankCanAlwaysLand) != 0)
                        govtData[j].blessesPlayer[1] = 1;
                }
            }
        }
    }
    playerRepairInhibit = -1.0f;
    playerEffectiveMurk = PlayerEffectiveMurk();
}

#define kOutfModType38 0x26

/* ShipIsInertialess. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046df70
// MAC: _ShipIsInertialess i386 0x000079c2 ppc 0x00009640
Boolean ShipIsInertialess(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index != 0 && ship->parentShip != -1 && IsShipInCloseForm(ship))
        return 0;
    if ((shipClass[ship->type].flags2 & kShipInertialess) != 0)
        return 1;
    if (ship->index == 0) {
        if (playerHasInertialDamper >= 0) {
            if (playerHasInertialDamper != 0)
                return 1;
        } else {
            playerHasInertialDamper = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType38) {
                            playerHasInertialDamper = 1;
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
// MAC: _ShipAfterburnerFuelUsage i386 0x00007a82 ppc 0x00009758
float ShipAfterburnerFuelUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerABFuelUsage < 0.0) {
            playerABFuelUsage = 0.0f;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType15) {
                            playerABFuelUsage = outfitSystem[i].modVal[j] * 0.0333;
                            break;
                        }
                    }
                }
            }
        }
        return playerABFuelUsage;
    }
    return 0.0f;
}

#define kOutfModType41 0x29

/* ShipResistsGravity. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e120
// MAC: _ShipResistsGravity i386 0x00007b10 ppc 0x00009850
short ShipResistsGravity(ShipObj *ship)
{
    short i;
    short j;

    if ((shipClass[ship->type].flags3 & kShipIgnoresDeadly) != 0)
        return 1;
    if (ShipIsInertialess(ship))
        return 1;
    if (ship->index == 0) {
        if (playerResistsGravity != -1) {
            if (playerResistsGravity != 0)
                return 1;
        } else {
            playerResistsGravity = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType41) {
                            playerResistsGravity = 1;
                            break;
                        }
                    }
                }
            }
        }
        return playerResistsGravity;
    }
    return 0;
}

#define kOutfModType42 0x2a

/* ShipResistsDeadlyStellars. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e210
// MAC: _ShipResistsDeadlyStellars i386 0x00007bbe ppc 0x00009950
short ShipResistsDeadlyStellars(ShipObj *ship)
{
    short i;
    short j;

    if ((shipClass[ship->type].flags3 & kShipIgnoresDeadly) != 0)
        return 1;
    if (ship->index == 0) {
        if (playerResistsDeadlyStellars != -1) {
            if (playerResistsDeadlyStellars != 0)
                return 1;
        } else {
            playerResistsDeadlyStellars = 0;
            for (i = 0; i < 512; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType42) {
                            playerResistsDeadlyStellars = 1;
                            break;
                        }
                    }
                }
            }
        }
        return playerResistsDeadlyStellars;
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
// MAC: _ApplyGravity i386 0x00007c5a ppc 0x00009a24
void ApplyGravity(FloatPoint *a, FloatPoint *b, float force, FloatPoint *out)
{
    float dx;
    float dy;
    float dist;
    dx = (a->x - b->x) * 0.01;
    dy = (a->y - b->y) * 0.01;
    if (dx < 0)
        dx *= -1.0;
    if (dy < 0)
        dy *= -1.0;
    dist = dx * dx + dy * dy;
    if (dist < 30)
        dist = 30;
    Accel(CalcInterceptAngle(a, b), force / dist, out);
}

/* StellarIsDestroyed (mac _StellarIsDestroyed). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e3c0
// MAC: _StellarIsDestroyed i386 0x00004e14 ppc 0x0000635c
Boolean StellarIsDestroyed(SpobRecord *spob)
{
    if (spob->strength > 0 && (spob->deadCounter < 0 || spob->deadDaysLeft > 0))
        return 1;
    return 0;
}

/* StellarIsLandable (mac _StellarIsLandable). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e3f0
// MAC: _StellarIsLandable i386 0x0000a989 ppc 0x0000d0d0
Boolean StellarIsLandable(SpobRecord *spob)
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
// MAC: _StellarIsNormalLandable i386 0x0000a9f8 ppc 0x0000d164
Boolean StellarIsNormalLandable(SpobRecord *spob)
{
    if (StellarIsLandable(spob) && !SpobIsHyperOrWorm(spob))
        return 1;
    return 0;
}

#define kNeutralComponent 0x20

/* GetShipPaintColor (mac _GetShipPaintColor). STATE: win MATCH, byte-identical; mac MATCH on both slices
   (V111 spelling). OPEN: none */
// FUNCTION: EVNOVA 0x0046e470
// MAC: _GetShipPaintColor i386 0x00007d0c ppc 0x00009adc
void GetShipPaintColor(ShipObj *ship, unsigned short *red, unsigned short *green,
                  unsigned short *blue)
#ifdef MAC1010
{
    RGBColor c;

    c.red = c.green = c.blue = kNeutralComponent;
    if (ship->index == 0) {
        c = playerPaint;
    } else if (ship->persType != -1) {
        c = personData[ship->persType].shipColor;
    } else if (ship->govt != -1) {
        c = govtData[ship->govt].rgb2;
    }
    if (c.red == 0 && c.green == 0 && c.blue == 0)
        c.red = c.green = c.blue = kNeutralComponent;
    *red = c.red;
    *green = c.green;
    *blue = c.blue;
}
#else
{
    unsigned short b;
    unsigned short g;
    unsigned short r;
#if V111 /* V111 spelling */
#else
    unsigned short *rgb;

    r = g = b = kNeutralComponent;
#endif

    if (ship->index == 0) {
        r = playerPaint.red;
        g = playerPaint.green;
        b = playerPaint.blue;
#if V111 /* V111 spelling */
    } else if (ship->persType != -1) {
        r = personData[ship->persType].rgb5[0];
        g = personData[ship->persType].rgb5[1];
        b = personData[ship->persType].rgb5[2];
    } else if (ship->govt != -1) {
        r = govtData[ship->govt].rgb2.red;
        g = govtData[ship->govt].rgb2.green;
        b = govtData[ship->govt].rgb2.blue;
    } else {
        r = g = b = kNeutralComponent;
    }
#else
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
#endif
    if (r == 0) {
        if (g == 0) {
            if (b == 0)
                r = g = b = kNeutralComponent;
        }
    }
    *red = r;
    *green = g;
    *blue = b;
}
#endif

#define kOutfModType49 0x31

#define kShipDefOutfits 8

/* ShipCanSelfRepair (mac _ShipCanSelfRepair). STATE: MATCH 100.0%, exe 132 / obj 132, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0046e540
// MAC: _ShipCanSelfRepair i386 0x00009134 ppc 0x0000b258
Boolean ShipCanSelfRepair(ShipObj *ship)
{
    short i;
    short j;

    if (gSpeedMult > 0.0) {
        if (Rand((500.0 / gSpeedMult)) != 0)
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
                    if (outfitSystem[i].modType[j] == kOutfModType49)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < kShipDefOutfits; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j]
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
// MAC: _SplitTwoLineString i386 0x00007dc1 ppc 0x00009b98
unsigned char SplitTwoLineString(const unsigned char *src, unsigned char *first,
                  unsigned char *second)
{
    Boolean afterBreak;
    Boolean skipped;
    short i;
    short out;
    short len;

    afterBreak = 0;
    len = src[0];
    out = 1;
    first[0] = 0;
    *second = 0;
    for (i = 1; i <= len; i++) {
        skipped = 0;
        if (i < len && src[i] == '\\' && (src[i + 1] == 'n' || src[i + 1] == 'N')) {
            i++;
            skipped = 1;
            afterBreak = 1;
        }
        if (!skipped) {
            if (afterBreak) {
                second[out] = src[i];
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

#ifdef MACBUILD
/* UnsplitTwoLineString (mac _UnsplitTwoLineString, mac-only). STATE: see the mac gate. */
// MAC: _UnsplitTwoLineString i386 0x00007e4f ppc 0x00009c50
void UnsplitTwoLineString(const unsigned char *src, unsigned char *dst)
{
    short i;
    short out;
    short len;

    out = 1;
    len = src[0];
    dst[0] = 0;
    for (i = 1; i <= len; i++) {
        if (i < len && src[i] == '\\' && (src[i + 1] == 'n' || src[i + 1] == 'N')) {
            i++;
            dst[out] = ' ';
            dst[0]++;
        } else {
            dst[out] = src[i];
            dst[0]++;
        }
        out++;
    }
}
#endif

/* FindSystemFromStellar (mac _FindSystemFromStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e790
// MAC: _FindSystemFromStellar i386 0x00007ec1 ppc 0x00009cd4
short FindSystemFromStellar(short stellarID)
{
    short i;
    short j;

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0) {
            for (j = 0; j < kNavSlots; j++) {
                if (systemData[i].navDefault[j] != -1
                        && stellarID == systemData[i].navDefault[j])
                    return i;
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        for (j = 0; j < kNavSlots; j++) {
            if (systemData[i].navDefault[j] != -1
                    && stellarID == systemData[i].navDefault[j])
                return i;
        }
    }
    return -1;
}

/* BusyIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e840
// MAC: _BusyIdle i386 0x00005981 ppc 0x0000707c
void BusyIdle(void)
{
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    EventRecord ev;

    WaitNextEvent(0, &ev, (gameLoaded && inBackground) ? 30 : 0, NULL);
#endif
}

/* CheckLoMem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e850
// MAC: _CheckLoMem i386 0x00007f65 ppc 0x00009d7c
void CheckLoMem(void)
{
#if TARGET_API_MAC_OS8
    Size grow;
    Str255 text;

    grow = 0x7fffffff;
    if (MaxMem(&grow) < 3000UL * 1024
        && (lastLoMemWarningTime == 0 || TickCount() > lastLoMemWarningTime + 3 * 60 * 60)) {
        if (cursorHidden > 0) {
            cursorHidden = 0;
            InitCursor();
        }
        GetIndString(text, kAlertStrList, 49);
        EVNoteAlert(text);
        if (gameInProgress && !gPause) {
            RefreshSAT();
            RedrawStatus(1);
        }
        lastLoMemWarningTime = TickCount();
    }
#endif
}

/* PlayerBlessedByGovt (mac _PlayerBlessedByGovt). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e860
// MAC: _PlayerBlessedByGovt i386 0x00002732 ppc 0x00003188
unsigned char PlayerBlessedByGovt(short govt, short which)
{
    if (govt < 0 || govt >= 256)
        return 0;
    if (which < 0 || which > 1)
        return 0;
    return govtData[govt].blessesPlayer[which];
}

/* ShowWindowNoFlash (mac _ShowWindowNoFlash). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e8b0
// MAC: _ShowWindowNoFlash i386 0x00007f6a ppc 0x00009d80
#ifdef MACBUILD
void ShowWindowNoFlash(void *arg)
{
#if TARGET_API_MAC_OS8
    long saved;

    saved = LMGetPaintWhite();
    LMSetPaintWhite(0);
    ShowWindow(arg);
    LMSetPaintWhite(saved);
#else
    SendBehind(arg, NULL);
    ShowWindow(arg);
#endif
}
#else
void ShowWindowNoFlash(void *arg)
{
    int saved;

    saved = gDisableUpdatesMode;
    gDisableUpdatesMode = 1;
    ShowMacWindow(arg);
    gDisableUpdatesMode = saved;
}
#endif

/* DisposeDialogNoFlash (mac _DisposeDialogNoFlash). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e8e0
// MAC: _DisposeDialogNoFlash i386 0x00007f91 ppc 0x00009db4
#ifdef MACBUILD
void DisposeDialogNoFlash(void *dlg)
{
#if TARGET_API_MAC_OS8
    long saved;

    saved = LMGetPaintWhite();
    LMSetPaintWhite(0);
    DisposeDialog(dlg);
    LMSetPaintWhite(saved);
#else
    DisposeDialog(dlg);
#endif
}
#else
void DisposeDialogNoFlash(void *dlg)
{
    int saved;

    saved = gDisableUpdatesMode;
    gDisableUpdatesMode = 1;
    DisposeDialog((DialogRecord *)dlg);
    gDisableUpdatesMode = saved;
}
#endif

/* AdjustedItemCost (mac _AdjustedItemCost). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e910
// MAC: _AdjustedItemCost i386 0x00005869 ppc 0x00006f04
long AdjustedItemCost(OutfRecord *outf, ShipRec *ship)
{
    long cost;
    long adjusted;
#ifdef V111 /* V111 spelling */
    long scaled;

    cost = outf->cost;
    if (cost <= 0)
        return 0;
    scaled = OutfCostScaled(outf);
    if (!scaled)
        return cost;
    adjusted = cost * ship->mass;
    if (adjusted < cost)
        return cost;
    return adjusted;
#else

    cost = outf->cost;
    adjusted = cost;
    if (cost <= 0)
        return 0;
    if (OutfCostScaled(outf)) {
        adjusted = adjusted * ship->mass;
        if (adjusted < cost)
            adjusted = cost;
    }
    return adjusted;
#endif
}

/* AdjustedItemMass (mac _AdjustedItemMass). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e950
// MAC: _AdjustedItemMass i386 0x00002889 ppc 0x00003330
long AdjustedItemMass(OutfRecord *outf, ShipRec *ship)
{
    short mass;
    long adjusted;

    mass = outf->mass;
    adjusted = mass;
    if (adjusted <= 0)
        return adjusted;
    if (OutfMassScaled(outf)) {
        adjusted = adjusted * ship->mass;
        adjusted *= 0.01;
        if (adjusted < mass)
            adjusted = mass;
    }
    return adjusted;
}

/* mac _PickEmergeStellar. */

/* PickEmergeStellar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046e9e0
// MAC: _PickEmergeStellar i386 0x00007f9a ppc 0x00009db8
short PickEmergeStellar(ShipObj *ship)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    short spob;

    spob = SelectRandomStellarDest(ship, Rand(3) == 0 ? true : false, 0);
#else
    int pick;
    short spob;

    pick = Rand(3) == 0 ? 1 : 0;
    spob = SelectRandomStellarDest(ship, pick, 0);
#endif
    if (spob >= 0 && spob < 2048 && SpobIsHyperOrWorm(&stellarData[spob]))
        return spob;
    return -1;
}

/* PlayerHasFightersOfOutfitType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ea40
// MAC: _PlayerHasFightersOfOutfitType i386 0x00007ff7 ppc 0x00009e2c
short PlayerHasFightersOfOutfitType(short outfitIndex)
{
    short i;
    short k;
    short bayType = -1;
    short weapon;
    short count;

    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfitIndex].modType[i] == kOutfModAmmo) {
            weapon = outfitSystem[outfitIndex].modVal[i];
            if (weapon >= 0 && weapon < 256) {
                if (weaponStats[weapon].guidance == kGuidanceBay)
                    bayType = weaponStats[weapon].ammoType - 128;
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
        if (weaponStats[k].guidance != kGuidanceBay)
            continue;
        if (shipData[0].weap[k].count <= 0)
            continue;
        if (weaponStats[k].ammoType - 128 != bayType)
            continue;
        if (shipData[0].weap[k].ammo > 0)
            return shipData[0].weap[k].ammo;
    }
    return 0;
}

/* StellarNumHyperLinks. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ebb0
// MAC: _StellarNumHyperLinks i386 0x0000812a ppc 0x00009fc8
short StellarNumHyperLinks(SpobRecord *spob)
{
    short i;
    short count;

    if (spob == NULL)
        return 0;
    count = 0;
    for (i = 0; i < kSpobHyperLinks; i++) {
        if (spob->hyperLinks[i] >= 0 && spob->hyperLinks[i] < 2048)
            count = count + 1;
    }
    return count;
}

#define kStarLayers kMaxStars


/* RandomizeStars. STATE: MATCH 100.0%, exe 134 / obj 134, relocregcheck 23 of 23; byte-identical
   pending (bytediff reads the adopted baseline; re-run after regate --update).
   OPEN: none. The unfolded short twenty is `count = kStarLayers; count *= height / 600.0;' --
   the compound assignment reads its own lvalue, which the front end does not substitute. */
// FUNCTION: EVNOVA 0x0046ebf0
// MAC: _RandomizeStars i386 0x0000b145 ppc 0x0000db10
void RandomizeStars(void)
{
    short i;
    short count;
    short width;
    short height;

    if (systemData[shipData->curSystem].murk < 0) {
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
                starData[i].pos.x = shipData->pos.x + Rand(width) - width / 2;
                starData[i].pos.y = shipData->pos.y + Rand(height) - height / 2;
                if (settings.parallaxStars)
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
// MAC: _ClearStarfield i386 0x00008152 ppc 0x0000a008
void ClearStarfield(void)
{
    short i;

    for (i = 0; i < kStarLayers; i++)
        starData[i].live = 0;
}

/* ScrollStarfield. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ee50
// MAC: _ScrollStarfield i386 0x00008171 ppc 0x0000a038
void ScrollStarfield(float dx, float dy)
{
    short i;

    for (i = 0; i < kStarLayers; i++) {
        if (starData[i].live != 0) {
            if (starData[i].depth > 0.0) {
                starData[i].pos.x -= dx * starData[i].depth;
                starData[i].pos.y -= dy * starData[i].depth;
            }
        }
    }
}

/* pstrcpy (mac _pstrcpy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046eeb0
// MAC: _pstrcpy i386 0x00004ce2 ppc 0x000061d8
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
// MAC: _pstrcmp i386 0x000084d2 ppc 0x0000a554
#if !defined(MACBUILD) || !V111
short pstrcmp(const unsigned char *a, const unsigned char *b)
#include "../engine/pstrcmp.inc"
#endif

/* HideCursorSprite. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046ef80
// MAC: _HideCursorSprite i386 0x000081d6 ppc 0x0000a0a4
void HideCursorSprite(void)
{
    if (cursorSprite != NULL) {
        SWSetSpriteVisible(cursorSprite, 0);
        cursorAlpha = 0;
    }
}

/* WarpSoundDuration. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046efb0
// MAC: _WarpSoundDuration i386 0x00008203 ppc 0x0000a0f4
short WarpSoundDuration(void)
{
    if (doubleTime != 0)
        return jumpSoundLength[1];
    return jumpSoundLength[0];
}

/* StellarsAreIdentical (mac _StellarsAreIdentical). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046efd0
// MAC: _StellarsAreIdentical i386 0x0000ab35 ppc 0x0000d334
Boolean StellarsAreIdentical(short a, short b)
{
    Str255 nameA;
    Str255 nameB;

    if (a >= 0 && a < kMaxStellars && b >= 0 && b < kMaxStellars) {
        if (a == b)
            return 1;
        if (stellarData[a].pos.h == stellarData[b].pos.h &&
            stellarData[a].pos.v == stellarData[b].pos.v &&
#ifdef MAC1010 /* 1.0.10 spelling */
            (char)stellarData[a].name[0] == (char)stellarData[b].name[0]) {
#elif defined(V111) /* V111 spelling */
            *(char *)stellarData[a].name == *(char *)stellarData[b].name) {
#else
            stellarData[a].name[0] == stellarData[b].name[0]) {
#endif
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
// MAC: _ShipIsDerelict i386 0x00008225 ppc 0x0000a128
Boolean ShipIsDerelict(ShipObj *ship)
{
    short govt;

    govt = ship->govt;
    if (govt >= 0 && govt < 256 && (govtData[govt].flags & kGovtAlwaysDisabled) != 0)
        return 1;
    return 0;
}

/* PlayerHasJettisonableCargo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f140
// MAC: _PlayerHasJettisonableCargo i386 0x0000825a ppc 0x0000a164
Boolean PlayerHasJettisonableCargo(void)
{
    short i;

    for (i = 0; i < 6; i++) {
        if (shipData[0].cargo[i] > 0)
            return 1;
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0)
            return 1;
    }
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0
                && currentMissions[i].canAbort != 0)
            return 1;
    }
    return 0;
}

/* AddToPlayerKills (mac _AddToPlayerKills). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f1e0
// MAC: _AddToPlayerKills i386 0x000082dc ppc 0x0000a22c
void AddToPlayerKills(int amount)
{
    if (playerKills < 10000000) {
        if (amount < 5)
            playerKills += 1;
        else
            playerKills += amount * 0.2;
    } else {
        playerKills = 10000000;
    }
}

/* WeaponBurstInterval. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f270
// MAC: _WeaponBurstInterval i386 0x0000831c ppc 0x0000a2bc
short WeaponBurstInterval(short weap, short count)
{
    if ((weaponStats[weap].flags & kWeapFireSimultaneously) != 0)
        return weaponStats[weap].burstCount;
    return count * weaponStats[weap].burstCount;
}

/* WeaponMaxSimultShots. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f2c0
// MAC: _WeaponMaxSimultShots i386 0x00008350 ppc 0x0000a2f4
short WeaponMaxSimultShots(ShipObj *ship, short w)
{
    short count;
    short ammo;
    short type;
    float fuel;

    if ((weaponStats[w].flags & kWeapFireSimultaneously) != 0) {
        count = ship->weap[w].count;
        if ((weaponStats[w].flags & kWeapFireSimultaneously) != 0
            && (weaponStats[w].flags3 & kWeapAmmoAtBurstEnd) != 0) {
            if (weaponStats[w].guidance == kGuidanceBay) {
                ammo = ship->weap[w].ammo;
                if (ammo < count)
                    count = ammo;
            } else {
                type = weaponStats[w].ammoType;
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
// MAC: _IonizeShip i386 0x00008409 ppc 0x0000a408
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

#ifdef MACBUILD
#if V111
short pstrcmp(const unsigned char *a, const unsigned char *b)
#include "../engine/pstrcmp.inc"
#endif

/* DrawPicHandle (mac _DrawPicHandle, mac-only). STATE: see the mac gate. */
// MAC: _DrawPicHandle i386 0x0000850d ppc 0x0000a5a8
short DrawPicHandle(PicHandle pic, Rect *dstRect)
{
    short err = noErr;
    struct ITImage *image = NULL;
    struct ITImage *mask = NULL;
    GrafPtr port;
    struct ITImage *ctx;
    ITTransparent trans;
    struct ITRect dstR;

    if (pic != NULL && dstRect != NULL) {
        GetPort(&port);
        ctx = DT_ContextCreateRaw(port, 0);
        if (ctx != NULL) {
            err = DT_ImageFileLoad((void **)pic, &image, &mask, &trans, 0);
            if (err == noErr) {
                dstR = Sys2DT_Rect(dstRect);
                err = DT_DrawImage(ctx, CopySys2DT_Rect(&dstR, (DTSysRect *)dstRect), image,
                                   mask, &trans, 0);
                DT_ImageDispose(image);
                if (mask != NULL)
                    DT_ImageDispose(mask);
            } else
                DrawPicture(pic, dstRect);
            DT_ContextDispose(ctx);
        } else
            err = memFullErr;
    }
    return err;
}

#if V111
void CenterPict(void *pict, Rect *bounds)
#include "../engine/centerpict.inc"
#endif

#if V111
/* isPowerPC (mac _isPowerPC, mac-only). STATE: see the mac gate. */
// MAC: _isPowerPC i386 0x00008710 ppc 0x0000a788
Boolean isPowerPC(void)
{
    host_basic_info_data_t info;
    mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;

    return host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&info, &count) == KERN_SUCCESS
           && info.cpu_type == CPU_TYPE_POWERPC;
}

/* isG3 (mac _isG3, mac-only). STATE: see the mac gate. */
// MAC: _isG3 i386 0x00008758 ppc 0x0000a7dc
Boolean isG3(void)
{
    host_basic_info_data_t info;
    mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;

    return host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&info, &count) == KERN_SUCCESS
           && info.cpu_type == CPU_TYPE_POWERPC
           && info.cpu_subtype == CPU_SUBTYPE_POWERPC_750;
}

/* isG4 (mac _isG4, mac-only). STATE: see the mac gate. */
// MAC: _isG4 i386 0x000087a6 ppc 0x0000a83c
Boolean isG4(void)
{
    host_basic_info_data_t info;
    mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;

    return host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&info, &count) == KERN_SUCCESS
           && info.cpu_type == CPU_TYPE_POWERPC
           && (info.cpu_subtype == CPU_SUBTYPE_POWERPC_7400
               || info.cpu_subtype == CPU_SUBTYPE_POWERPC_7450);
}

/* meetsMinimumRequirements (mac _meetsMinimumRequirements, mac-only). STATE: see the mac gate. */
// MAC: _meetsMinimumRequirements i386 0x000087fc ppc 0x0000a8a0
Boolean meetsMinimumRequirements(void)
{
    return !(isG3() || isG4());
}
#endif
#endif

EXTERN_C_END
#endif /* MAC_FASTRANDOM_ONLY */
