/* ai.h -- what an AI ship decides to do: targets, threats, escorts, orders. Notes: Research/notes/ai.md */
#ifndef AI_H
#define AI_H
#include "gametypes.h"
#include "resources.h"
#include "shipobj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void AICalcShipFormPos(ShipObj *escort, ShipObj *leader, short escortIndex,
                          short escortCount, short spacing);
extern void AICalcFormationPositions(ShipObj *ship, Boolean recalc);
extern void AICallForHelp(ShipObj *ship);
extern void AICheckParentExists(ShipObj *ship);
extern Boolean AIDoesShipLikePlayer(ShipObj *ship);
extern Boolean AIEvalHopelessChase(ShipObj *ship);
extern void AIFightEnemyThreatToPlayer(ShipObj *ship);
extern void AIFightThreatToParent(ShipObj *ship);
extern void AIFindShipToPlunder(ShipObj *ship);
extern short AIFindSwarmLeader(ShipObj *ship);
extern void AIForceBurstReload(ShipObj *ship);
extern void AIHandleCloaking(ShipObj *ship);
extern Boolean AIIsFighterReturning(ShipObj *ship);
extern Boolean AIIsShipAttemptingDisable(ShipObj *ship);
extern Boolean AIIsShipDoingCountWait(ShipObj *ship);
extern Boolean AIIsShipLeaving(ShipObj *ship);
extern void AIIssueEscortOrders(ShipObj *ship);
extern void AIMakeEscortFlyInForm(ShipObj *ship);
extern void AIMakeShipEmergeFromHyperGate(ShipObj *ship, short stellar);
extern void AIMakeShipJumpIn(ShipObj *ship);
extern void AIMakeShipLeave(ShipObj *ship);
extern void AIMakeShipStopRefuelling(ShipObj *ship);
extern void AISetup(ShipObj *ship);
extern void AIShipCapturedByPlayer(ShipObj *ship);
extern Boolean AIShipHasFuelForJump(ShipObj *ship);
extern Boolean AIShipIsAttackingAsteroid(ShipObj *ship);
extern Boolean AIShipIsDockedWithShip(ShipObj *ship, ShipObj *player);
extern Boolean AIShipIsDocking(ShipObj *ship);
extern Boolean AIShipIsEmergingFromHypergate(ShipObj *ship);
extern Boolean AIShipIsEnteringHypergate(ShipObj *ship);
extern Boolean AIShipIsPlundering(ShipObj *ship);
extern Boolean AIShipIsPlunderingShip(ShipObj *ship, ShipObj *other);
extern Boolean AIShipIsShipGoingToStellar(ShipObj *ship, short stellar);
extern Boolean AIVerifySwarmLeader(ShipObj *ship);
extern void BraveTraderAI(ShipObj *ship);
extern Boolean CanHireEscorts(void);
extern void ClearTargetsOnShip(ShipObj *ship);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern void *hardDockSnd;
EXTERN_CPP_END
#pragma cplusplus reset
extern void DeathStarAI(ShipObj *ship);
extern void DefenseShipAI(ShipObj *ship);
extern void DoEscortLand(void);
extern void DoEscortPayment(short days);
extern void DoGoodSamaritan(ShipObj *victim, ShipObj *attacker);
extern void EscortAI(ShipObj *ship);
extern Boolean ExtendedIsThreatToPlayer(ShipObj *ship);
extern Boolean IsRefuellingPlayer(ShipObj *ship);
extern Boolean IsRepairingPlayer(ShipObj *ship);
extern Boolean IsPlayerThreatened(void);
extern void AIMakeShipJumpWithParent(ShipObj *ship);
extern void AIMakeShipRefuelPlayer(ShipObj *ship);
extern void AIMakeShipRepairPlayer(ShipObj *ship);
extern void AIMakeShipStopJumping(ShipObj *ship);
extern Boolean AIIsShipJumping(ShipObj *ship);
extern Boolean AIIsShipJumpingIn(ShipObj *ship);
extern Boolean IsShipInCloseForm(ShipObj *ship);
extern Boolean AIIsShipBusy(ShipObj *ship);
extern Boolean AIInGunRange(ShipObj *me, ShipObj *target,
                         short weapType);
extern Boolean AIDoSwarming(ShipObj *ship);
extern Boolean AIShipIsScooping(ShipObj *ship);
#ifndef MACBUILD /* inline on mac: sources/mac/include/rt3_ev_nova.h */
extern Boolean RegScramA2(unsigned int x);
extern void RegScramB2(unsigned char x);
extern int RegScramC2(unsigned char x);
#endif
#ifndef MACBUILD
extern void *RegScramF2(void);
extern Boolean RegScramD2(char *p);
extern Boolean RegScramE2(void);
#endif
extern short FindNearestShipToPlayer(void);
extern short FindNearestThreatToParent(ShipObj *me, short spob);
extern long IsShipANearbyThreatToParent(ShipObj *ship, ShipObj *me,
                                        short range);
extern short FindNextShipInSystem(short cur, short system);
extern short FindPrevShipInSystem(short cur, short system);
extern void FireAIShipWeapon(ShipObj *ship);
extern void HandleNavTargetSprites(void);
extern void HandlePlayerBoardAttempt(void);
extern void HandlePlayerDockApproach(void);
extern void HandleWeapTargetSprites(void);
extern void HighLevelAIHandler(ShipObj *ship);
extern void InterceptorAI(ShipObj *ship);
extern Boolean IsPlayerThreatenedByMyEnemy(ShipObj *ship);
extern Boolean IsShipAttacking(ShipObj *ship);
extern Boolean IsShipFightingMe(ShipObj *ship, ShipObj *other);
extern Boolean IsShipThreatened(ShipObj *ship);
extern Boolean IsShipThreatenedByMyEnemy(ShipObj *ship,
                                      ShipObj *me);
extern Boolean IsThreatToPlayer(ShipObj *ship);
extern Boolean IsThreatToStellar(ShipObj *ship, SpobRecord *spob);
extern void LowLevelAIHandler(ShipObj *ship);
extern void MinerAI(ShipObj *ship);
extern void PirateWarshipAI(ShipObj *ship);
extern void RespawnEscort(ShipObj *ship, unsigned char full);
extern short SelectFleetFromSystem(SystRecord *syst);
extern void SelectWarshipTarget(ShipObj *ship);
extern void WarshipAI(ShipObj *ship);
extern void WimpyTraderAI(ShipObj *ship);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean boardTryFlag;
extern char creditCString[0x20];
extern Boolean dockTryFlag;
extern float hyperGamma;
extern Boolean miningBoxesActive;
extern Boolean miningShipsActive;
EXTERN_CPP_END
#pragma cplusplus reset

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean scanActive;
extern short aiComplexity;
extern unsigned char gRT3Table2[];
EXTERN_CPP_END
#pragma cplusplus reset

extern void AIMakeShipAttackPlayer(ShipObj *ship);
extern void AIFightThreatToPlayer(ShipObj *ship);
extern short AIShipFriendStrength(ShipObj *ship);
extern short AIShipIsOutOfAmmo(ShipObj *ship);
extern Boolean AIHasDestroyingWeapons(ShipObj *ship);
extern Boolean IsShipMyEnemy(ShipObj *ship, ShipObj *other);
extern Boolean ExtendedIsThreatToShip(ShipObj *self, ShipObj *other);
extern void AIFireMissile(ShipObj *ship);
extern void AIFireGun(ShipObj *ship, Boolean allowUnguided);
extern void AIFireGunUntargeted(ShipObj *ship);
extern void AIFirePlanetGun(ShipObj *ship);
extern void AILaunchFighter(ShipObj *ship);
extern void AIEscortFireUnprovoked(ShipObj *ship);
extern void AIPlunderShipContents(ShipObj *pirate, ShipObj *victim);
extern void AIMaintainFormation(ShipObj *ship, Boolean snap);
extern void AIAddFighterToParent(ShipObj *fighter);

extern short FindNearestValidShotTarget(ShotObj *shot);
extern Boolean AICallForReinforcements(ShipObj *ship, Boolean force);
extern void AIDispatch(ShipObj *ship, Boolean noBrains);
extern void SpawnBountyHunter(void);
extern Boolean IssueNewEscortCommand(short aiTypeFilter, short cmd, Boolean quiet);
extern short SelectRandomStellarDest(ShipObj *ship, unsigned char allowSpecial,
                                     Boolean plainOnly);

#ifdef __cplusplus
}
#endif

#endif
