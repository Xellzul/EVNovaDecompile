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
extern "C++" {
extern char abOn;
}
#pragma cplusplus reset
extern void DamageShip(ShipObj *ship, FloatPoint *from, short impact, short armorDmg,
                       short shieldDmg, short attacker, char fromAttack, char deliberate,
                       unsigned char disableOnly, char clampArmor,
                       unsigned char armorOnly, short provocation);
extern void DoAutoRefuel(void);
extern void DoShipCapture(ShipObj *ship, char keepShip);
extern void DoShipUncloak(ShipObj *ship);
extern void EnterMoreShips(short system);
extern void SetupShipsInSystem(short sys, unsigned char arriving);
extern float ShipShieldRechargeRate(ShipObj *ship);
extern float ShipArmorRechargeRate(ShipObj *ship);
extern char ShipFuelGenRate(ShipObj *ship, float *out);
extern char HasEscapePod(ShipObj *ship);
extern char HasEscapeShip(ShipObj *ship);
extern short EscapeShipType(ShipObj *ship);
extern char KeyCarriedShipTypeOnboard(ShipObj *ship);
extern short ShipECM(ShipObj *ship, short kind);
extern short CloakFuelUsage(ShipObj *ship);
extern void FirePlayerWeapon(short weapon);
extern void GrantOutfitItem(short outfit);
extern void HandleReinforcements(void);
extern void HandleShip(ShipObj *ship);
extern void HandleShipDisplay(ShipObj *ship);
extern char HasMaxOfItem(short outfit, short *countOut,
                         short *maxOut);
extern short HyperShipSpawn(short sys);
extern void HyperSpawnFleet(short sys, short fleet, short escortOf,
                            unsigned char arriving);
extern void IonizeShip(ShipObj *ship, WeapRecord *weap, FloatPoint *pos);
extern void LaunchEscapePod(ShipObj *ship);
extern short MissionDudeSpawn(short dude, short wantType, short sys,
                              short mission);
extern long PlayerShipTradeInPrice(void);
extern short RandomShipSpawn(short sys, short reserve);
extern float ShipArmorCapacity(ShipObj *ship);
extern short ShipFuelCapacity(ShipObj *ship);
extern char ShipIsCloaked(ShipObj *ship);
extern short ShipMaxWeaponRange(ShipObj *ship);
extern float ShipShieldCapacity(ShipObj *ship);
extern void ShipStatsToSystemInfo(void);
extern char ShipsShareParent(ShipObj *a, ShipObj *b);
extern void SpawnBox(short slot);
extern short SpawnEscortOfType(short shipType, short stellar);
extern void SpawnFleet(short sys, unsigned char arriving);
extern void SpawnNagShip(void);
extern void SpawnScoopableBox(FloatPoint *pos, short cargoType, short pict);
extern void SystemInfoToShipStats(void);
extern short TotalMissionCargoSpace(void);
extern void VerifyResourceChecksums(void);
extern char WeaponHasAmmo(ShipObj *ship, short weaponIndex);
extern short WeaponMaxSimultShots(ShipObj *ship, short w);
#pragma cplusplus on
extern "C++" {
extern char allReinforcementsInhibited;
extern Handle charFileHand;
extern float gCachedAccelRate;
extern float gCachedArmorCap;
extern float gCachedArmorRech;
extern float gCachedFuelCap;
extern float gCachedMaxSpeed;
extern float gCachedTurnRate;
extern unsigned char gGovtNames3[256][256];
extern short gSharewareDays;
extern char gravitySourcesPresent;
extern DateTimeRec scenarioStartDate;
extern short tradePrice[];
extern short portDemand[6];
}
#pragma cplusplus reset

#pragma cplusplus on
extern "C++" {
extern short junkType[2];
}
#pragma cplusplus reset

extern unsigned char IsDying(ShipObj *ship);
extern char IsDisabled(ShipObj *ship);
extern char ShipVisibleToShip(ShipObj *ship, ShipObj *viewer);
extern float ShipAccelRate(ShipObj *ship);
extern float ShipMaxSpeed(ShipObj *ship);
extern float ShipTurnRate(ShipObj *ship);
extern FloatPoint XYShortToFloat(ShortPoint pt);
extern char CanBuyFighter(short shipType, short outfitIndex, short weapIndex);
extern long ApplyPriceAndTechnologyFlux(long cost, float multiplier, short itemTech,
                            short portTech);
extern void JettisonCargo(unsigned char withMissions);
extern char ShipCanCloak(ShipObj *ship);
extern char CanBuyOutfitItem(short outfit);
extern char TurretBlindSpot(ShipObj *ship, short heading, short weap);
#pragma cplusplus on
extern "C++" {
extern char statusRedraw[7];
}
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
