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
extern void AICalcFormationPositions(ShipObj *ship, char recalc);
extern void AICallForHelp(ShipObj *ship);
extern void AICheckParentExists(ShipObj *ship);
extern char AIDoesShipLikePlayer(ShipObj *ship);
extern char AIEvalHopelessChase(ShipObj *ship);
extern void AIFightEnemyThreatToPlayer(ShipObj *ship);
extern void AIFightThreatToParent(ShipObj *ship);
extern void AIFindShipToPlunder(ShipObj *ship);
extern short AIFindSwarmLeader(ShipObj *ship);
extern void AIForceBurstReload(ShipObj *ship);
extern void AIHandleCloaking(ShipObj *ship);
extern char AIIsFighterReturning(ShipObj *ship);
extern char AIIsShipAttemptingDisable(ShipObj *ship);
extern char AIIsShipDoingCountWait(ShipObj *ship);
extern char AIIsShipLeaving(ShipObj *ship);
extern void AIIssueEscortOrders(ShipObj *ship);
extern void AIMakeEscortFlyInForm(ShipObj *ship);
extern void AIMakeShipEmergeFromHyperGate(ShipObj *ship, short stellar);
extern void AIMakeShipJumpIn(ShipObj *ship);
extern void AIMakeShipLeave(ShipObj *ship);
extern void AIMakeShipStopRefuelling(ShipObj *ship);
extern void AISetup(ShipObj *ship);
extern void AIShipCapturedByPlayer(ShipObj *ship);
extern char AIShipHasFuelForJump(ShipObj *ship);
extern char AIShipIsAttackingAsteroid(ShipObj *ship);
extern char AIShipIsDockedWithShip(ShipObj *ship, ShipObj *player);
extern char AIShipIsDocking(ShipObj *ship);
extern char AIShipIsEmergingFromHypergate(ShipObj *ship);
extern char AIShipIsEnteringHypergate(ShipObj *ship);
extern char AIShipIsPlundering(ShipObj *ship);
extern char AIShipIsPlunderingShip(ShipObj *ship, ShipObj *other);
extern char AIShipIsShipGoingToStellar(ShipObj *ship, short stellar);
extern char AIVerifySwarmLeader(ShipObj *ship);
extern void BraveTraderAI(ShipObj *ship);
extern char CanHireEscorts(void);
extern void ClearTargetsOnShip(ShipObj *ship);
#pragma cplusplus on
extern "C++" {
extern void *hardDockSnd;
}
#pragma cplusplus reset
extern void DeathStarAI(ShipObj *ship);
extern void DefenseShipAI(ShipObj *ship);
extern void DoEscortLand(void);
extern void DoEscortPayment(short days);
extern void DoGoodSamaritan(ShipObj *victim, ShipObj *attacker);
extern void EscortAI(ShipObj *ship);
extern char ExtendedIsThreatToPlayer(ShipObj *ship);
extern char IsRefuellingPlayer(ShipObj *ship);
extern char IsRepairingPlayer(ShipObj *ship);
extern char IsPlayerThreatened(void);
extern void AIMakeShipJumpWithParent(ShipObj *ship);
extern void AIMakeShipRefuelPlayer(ShipObj *ship);
extern void AIMakeShipRepairPlayer(ShipObj *ship);
extern void AIMakeShipStopJumping(ShipObj *ship);
extern char AIIsShipJumping(ShipObj *ship);
extern char AIIsShipJumpingIn(ShipObj *ship);
extern char IsShipInCloseForm(ShipObj *ship);
extern char AIIsShipBusy(ShipObj *ship);
extern char AIInGunRange(ShipObj *me, ShipObj *target,
                         short weapType);
extern char AIFollowSwarmLeader(ShipObj *ship);
extern char AIShipIsScooping(ShipObj *ship);
extern char RegScramA2(unsigned int x);
extern void RegScramB2(unsigned char x);
extern int RegScramC2(unsigned char x);
extern void *RegScramF2(void);
extern char RegScramD2(char *p);
extern char RegScramE2(void);
extern short FindNearestShipToPlayer(void);
extern short FindNearestThreatToParent(ShipObj *me, short spob);
extern long IsShipANearbyThreatToParent(ShipObj *ship, ShipObj *me,
                                        short range);
extern short FindNextTarget(short cur, short system);
extern short FindPrevTarget(short cur, short system);
extern void FireAIShipWeapon(ShipObj *ship);
extern void HandleNavTargetSprites(void);
extern void HandlePlayerBoardAttempt(void);
extern void HandlePlayerDockApproach(void);
extern void HandleWeapTargetSprites(void);
extern void HighLevelAIHandler(ShipObj *ship);
extern void InterceptorAI(ShipObj *ship);
extern char IsPlayerThreatenedByMyEnemy(ShipObj *ship);
extern char IsShipAttacking(ShipObj *ship);
extern char IsShipFightingMe(ShipObj *ship, ShipObj *other);
extern char IsShipThreatened(ShipObj *ship);
extern char IsShipThreatenedByMyEnemy(ShipObj *ship,
                                      ShipObj *me);
extern char IsThreatToPlayer(ShipObj *ship);
extern char IsThreatToStellar(ShipObj *ship, SpobRecord *spob);
extern void LowLevelAIHandler(ShipObj *ship);
extern void MinerAI(ShipObj *ship);
extern void PirateWarshipAI(ShipObj *ship);
extern void RespawnEscort(ShipObj *ship, unsigned char full);
extern short SelectFleetFromSystem(SystRecord *syst);
extern void SelectWarshipTarget(ShipObj *ship);
extern void WarshipAI(ShipObj *ship);
extern void WimpyTraderAI(ShipObj *ship);
#pragma cplusplus on
extern "C++" {
extern char boardTryFlag;
extern char creditCString[0x20];
extern char dockTryFlag;
extern float hyperGamma;
extern char miningBoxesActive;
extern char miningShipsActive;
}
#pragma cplusplus reset

#pragma cplusplus on
extern "C++" {
extern char scanActive;
extern short gFrameStep;
extern unsigned char gRT3Table2[];
}
#pragma cplusplus reset

extern void AIMakeShipAttackPlayer(ShipObj *ship);
extern void AIFightThreatToPlayer(ShipObj *ship);
extern short AIShipFriendStrength(ShipObj *ship);
extern short AIShipIsOutOfAmmo(ShipObj *ship);
extern char AIHasDestroyingWeapons(ShipObj *ship);
extern char IsShipMyEnemy(ShipObj *ship, ShipObj *other);
extern char ExtendedIsThreatToShip(ShipObj *self, ShipObj *other);
extern void AIFireMissile(ShipObj *ship);
extern void AIFireGun(ShipObj *ship, char allowUnguided);
extern void AIFireGunUntargeted(ShipObj *ship);
extern void AIFirePlanetGun(ShipObj *ship);
extern void AILaunchFighter(ShipObj *ship);
extern void AIEscortFireUnprovoked(ShipObj *ship);
extern void AIPlunderShipContents(ShipObj *pirate, ShipObj *victim);
extern void AIMaintainFormation(ShipObj *ship, char snap);
extern void AIAddFighterToParent(ShipObj *fighter);

extern short FindNearestValidShotTarget(ShotObj *shot);
extern char AICallForReinforcements(ShipObj *ship, char force);
extern void AIDispatch(ShipObj *ship, char noBrains);
extern void SpawnBountyHunter(void);
extern char IssueNewEscortCommand(short aiTypeFilter, short cmd, char quiet);
extern short SelectRandomStellarDest(ShipObj *ship, unsigned char allowSpecial,
                                     char plainOnly);

#ifdef __cplusplus
}
#endif

#endif
