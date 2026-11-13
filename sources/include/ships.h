/* ships.h -- the ship object: its rates, damage, outfits, cargo and spawning. Notes: Research/notes/ships.md */
#ifndef SHIPS_H
#define SHIPS_H
#include "burger.h"
#include "gametypes.h"
#include "shipobj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void AICalculateOddsAgainst(ShipObj *ship);
extern void CalcShipCanBuy(void);
extern void CalculateTargetedDamage(void);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean abOn;
EXTERN_CPP_END
#pragma cplusplus reset
extern void DamageShip(ShipObj *ship, FloatPoint *from, short impact, short armorDmg,
                       short shieldDmg, short attacker, Boolean fromAttack, Boolean deliberate,
                       unsigned char disableOnly, Boolean clampArmor,
                       unsigned char armorOnly, short provocation);
extern void DoAutoRefuel(void);
extern void DoShipCapture(ShipObj *ship, Boolean keepShip);
extern void DoShipUncloak(ShipObj *ship);
extern void EnterMoreShips(short system);
extern void SetupShipsInSystem(short sys, unsigned char arriving);
extern float ShipShieldRechargeRate(ShipObj *ship);
extern float ShipArmorRechargeRate(ShipObj *ship);
extern Boolean ShipFuelGenRate(ShipObj *ship, float *out);
extern Boolean HasEscapePod(ShipObj *ship);
extern Boolean HasEscapeShip(ShipObj *ship);
extern short EscapeShipType(ShipObj *ship);
extern Boolean KeyCarriedShipTypeOnboard(ShipObj *ship);
extern short ShipECM(ShipObj *ship, short kind);
extern short CloakFuelUsage(ShipObj *ship);
extern void FirePlayerWeapon(short weapon);
extern void GrantOutfitItem(short outfit);
extern void HandleReinforcements(void);
extern void HandleShip(ShipObj *ship);
extern void HandleShipDisplay(ShipObj *ship);
extern Boolean HasMaxOfItem(short outfit, short *countOut,
                         short *maxOut);
extern short HyperShipSpawn(short sys);
extern void HyperSpawnFleet(short sys, short fleet, short escortOf,
                            unsigned char arriving);
extern void IonizeShip(ShipObj *ship, WeapRecord *weap, FloatPoint *pos);
extern void SpawnEscapePod(ShipObj *ship);
extern short MissionDudeSpawn(short dude, short wantType, short sys,
                              short mission);
extern int32_t PlayerShipTradeInPrice(void);
extern short RandomShipSpawn(short sys, short reserve);
extern float ShipArmorCapacity(ShipObj *ship);
extern short ShipFuelCapacity(ShipObj *ship);
extern Boolean ShipIsCloaked(ShipObj *ship);
extern short ShipMaxWeaponRange(ShipObj *ship);
extern float ShipShieldCapacity(ShipObj *ship);
extern void ShipStatsToSystemInfo(void);
extern Boolean ShipsShareParent(ShipObj *a, ShipObj *b);
extern void SpawnBox(short slot);
extern short SpawnEscort(short shipType, short stellar);
extern void SpawnFleet(short sys, unsigned char arriving);
extern void SpawnNagShip(void);
extern void SpawnScoopableBox(FloatPoint *pos, short cargoType, short pict);
extern void SystemInfoToShipStats(void);
extern short TotalMissionCargoSpace(void);
extern void VerifyResourceChecksums(void);
extern Boolean WeaponHasAmmo(ShipObj *ship, short weaponIndex);
extern short WeaponMaxSimultShots(ShipObj *ship, short w);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean allReinforcementsInhibited;
extern Handle charFileHand;
extern float playerAccelRate;
extern float playerArmorCapacity;
extern float playerArmorRechargeRate;
extern float playerFuelCapacity;
extern float playerMaxSpeed;
extern float playerTurnRate;
extern unsigned char govtMediumNames[256][256];
extern short totalDaysPlayed;
extern Boolean gravitySourcesPresent;
extern DateTimeRec scenarioStartDate;
extern short tradePrice[];
extern short portDemand[6];
EXTERN_CPP_END
#pragma cplusplus reset

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short junkType[2];
EXTERN_CPP_END
#pragma cplusplus reset

extern unsigned char IsDying(ShipObj *ship);
extern Boolean IsDisabled(ShipObj *ship);
extern Boolean ShipVisibleToShip(ShipObj *ship, ShipObj *viewer);
extern float ShipAccelRate(ShipObj *ship);
extern float ShipMaxSpeed(ShipObj *ship);
extern float ShipTurnRate(ShipObj *ship);
extern FloatPoint XYShortToFloat(ShortPoint pt);
extern Boolean CanBuyFighter(short shipType, short outfitIndex, short weapIndex);
extern int32_t ApplyPriceAndTechnologyFlux(int32_t cost, float multiplier, short itemTech,
                            short portTech);
extern void JettisonCargo(unsigned char withMissions);
extern Boolean ShipCanCloak(ShipObj *ship);
extern Boolean CanBuyOutfitItem(short outfit);
extern unsigned char TurretBlindSpot(ShipObj *ship, short heading, short weap);
/* statusRedraw[] indices: the status bar panel DrawStatus redraws */
#define kRedrawNav          0
#define kRedrawShield       1
#define kRedrawCargo        2
#define kRedrawWeap         3
#define kRedrawFuel         4
#define kRedrawRadar        5
#define kRedrawTarg         6
#define kStatusRedrawCount  7
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean statusRedraw[kStatusRedrawCount];
EXTERN_CPP_END
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
