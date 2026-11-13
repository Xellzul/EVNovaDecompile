/* player.h -- the player's record, standing, pilot file and preferences. Notes: Research/notes/player.md */
#ifndef PLAYER_H
#define PLAYER_H
#include "burger.h"
#include "gameconst.h"
#include "gametypes.h"
#include "macport.h"
#include "shipobj.h"
#include "pilotfile.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void CheckForPiratedCodes(void);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Settings settings;
extern short recentActivatedRank;
extern KeyPrefs keyPrefs;
extern uint32_t lastEscortMenuTime;
extern double DAT_00735450;
extern Boolean DAT_00735488;
extern float playerRepairInhibit;
extern Boolean playerHasIllegalJunk;
extern Boolean playerHasIllegalOutfits;
extern Boolean playerHasTribbles;
extern Boolean playerHasPerishableJunk;
extern Boolean playerTurnedThisFrame;
extern float sabotageCounter;
extern Boolean lastJumpDistOk;
extern Boolean ackCommFlag;
extern Boolean navModeSelectFlag;
extern Boolean navTargetSelectFlag;
extern Boolean navDefaultSelectFlag;
extern Boolean helpKeyFlag;
extern Boolean cloakFlag;
extern Boolean fighterCommandFlag;
extern Boolean weapModeSelectFlag;
extern Boolean weapTargetSelectFlag;
extern Boolean weapDefaultSelectFlag;
extern Boolean playerInfoFlag;
extern Boolean mapTryFlag;
extern Boolean jumpTryFlag;
extern Boolean weapSelectFlag;
extern Boolean spawnShipFlag;
extern Boolean weapCloseTargFlag;
extern Boolean missionInfoFlag;
extern Boolean commTryFlag;
extern Boolean mapZoomFlag;
extern Boolean framerateFlag;
extern Boolean escortMenuFlag;
extern Boolean escortCommandFlag[];
extern short lastEscortCount;
EXTERN_CPP_END
#pragma cplusplus reset
extern void DrawCombatRatingString(void);
extern void DrawLegalStatusString(short curSystem);
extern void DumpPilotLog(void);
extern Boolean HasCloak(ShipObj *ship);
extern Boolean HasAreaCloak(ShipObj *ship);
extern Boolean CloakZeroesShields(ShipObj *ship);
extern Boolean ShipDecloaksWhenHit(ShipObj *ship);
extern short CloakShieldUsage(ShipObj *ship);
extern Boolean ShipCanSeeCloakedShips(ShipObj *ship, short which);
extern short ShipHyperTransitTime(ShipObj *ship);
extern void KeyPrefsToSlotTable(void);
extern void SlotTableToKeyPrefs(void);
extern float ShipDeionizationRate(ShipObj *ship);
extern short PlayerEffectiveMurk(void);
extern short ShipJumpsPerJump(ShipObj *ship);
extern Boolean ShipCanExpiditeJumps(ShipObj *ship);
extern float ShipAfterburnerFuelUsage(ShipObj *ship);
extern void SetDefaultPrefs(void);
extern void SetDefaultKeys(void);
extern int32_t ShipFreeMass(void);
extern void LoadStellarResponse(short which);
extern void HandlePlayer(ShipObj *ship);
extern void HandlePlayerDockRequest(void);
extern Boolean HasAfterburner(ShipObj *ship);
extern Boolean HasDensityScanner(ShipObj *ship);
extern Boolean HasIFF(ShipObj *ship);
#if defined(MACBUILD) && V111 /* the 1.1.1 preferences layer (EVPrefsDialogWrapper.o) takes a flag */
extern void LoadPrefs(Boolean loadWindowed);
#else
extern void LoadPrefs(void);
#endif
extern void LoadResponse(short which);
#ifdef MAC1010
extern OSErr DoEncryption(Handle h, uint32_t key);
#elif defined(MACBUILD)
extern OSErr DoEncryption(void *buf, uint32_t len, uint32_t key);
#else
extern int32_t DoEncryption(void *buf, uint32_t len, uint32_t key);
#endif
extern int32_t SaveAFile(const char *path, const void *data, uint32_t length);
#ifdef MAC1010
extern short LoadPilotData(FSSpec *spec);
#elif defined(MACBUILD)
extern short LoadPilotData(const FSRef *ref);
#else
extern short LoadPilotData(char *path);
#endif
extern void OpenLastPilotFile(void);
extern Boolean OpenPilotFile(char *missingResource);
extern unsigned char QuickKeyCheck(short scanCode);
extern void QuickKeyCheckHandler(void);
extern unsigned char RawKeyCheck(short scanCode);
extern void RecursivePenaltySlap(short sys, short govt, short type,
                                 double amount);
extern unsigned char RegScramA(unsigned int x);
extern int RegScramC(unsigned char x);
extern void ResetPlayerPrecalcedValues(void);
extern short SWFlagRectAsChanged(SpriteWorldPtr sw, Rect *r);
extern void SWHideAllSprites(SpriteWorldPtr sw);
extern void SavePrefs(void);
extern void ScanPlayer(ShipObj *ship);
extern void SetAllKeyCmdFlags(void);
extern short ShipAntiInterference(void);
extern Boolean ShipCanScoop(ShipObj *ship);
extern Boolean ShipCanTargetCloakedShips(ShipObj *ship);
extern Boolean ShipCanTargetUntargetableShips(ShipObj *ship);
extern Boolean ShipCloakVisibleOnRadar(ShipObj *ship);
extern int32_t ShipHyperSafeDist(ShipObj *ship);
extern float ShipIonizationFactor(ShipObj *ship);
extern short ShipResistsDeadlyStellars(ShipObj *ship);
extern short ShipResistsGravity(ShipObj *ship);
extern void ShowAlert(const char *title, const char *message);
extern void SlapWithPenalty(short sys, short govt, short kind,
                            short mission);
extern short WritePilotData(short stellar);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short adviceRandom;
extern short alertSpritePending;
extern short antiEscortDelay;
extern Boolean dockLand;
extern short bribeRandom;
extern Boolean cheatsActive;
extern float destructCount;
extern short dockPermission;
extern short dockTarget;
extern Boolean dontMovePlayerAfterLanding;
extern Boolean fadePending[2];
extern float playerABFuelUsage;
extern short playerHasAreaCloak;
extern short playerAntiInterference;
extern short playerCanTargetUntargetableShips;
extern short playerCanTargetCloakedShips;
extern short playerHasInertialDamper;
extern float playerDeionizeRate;
extern float playerIonizeMax;
extern short playerResistsGravity;
extern short playerResistsDeadlyStellars;
extern float playerShieldRechargeRate;
extern short playerHasDensityScanner;
extern short playerHasIFF;
extern char gPrefsDirectory[];
extern short selEscortGroup;
extern short globalPriceFlux[2];
extern Boolean quickKeyCheckResult;
extern Boolean killAsteroids;
extern Boolean killBoxes;
extern Boolean killExplods;
extern Boolean killSmoke;
extern short lastMissionStellar;
extern Boolean leetMode;
extern short localPriceFlux[2];
extern float navTargetExpand;
extern short newbieFlag;
extern Boolean noSavePilotFiles;
extern Boolean noUpdatePilotData;
extern Boolean oldPlayerThreatened;
#ifdef MAC1010
extern short pilotsVRefNum;
extern int32_t pilotsDirID;
extern short prefsVRefNum;
extern int32_t prefsDirID;
#endif
#ifdef MACBUILD
extern FSRef pilotsFolder;
#else
extern char pilotsFolder[];
#endif
extern short playerGender;
extern short playerHasAfterburner;
extern Boolean playerIsLanded;
extern int32_t playerKills;
extern FloatPoint playerMaxVel;
extern short playerSabotaged;
extern float playerShieldCapacity;
extern Boolean playerThreatened;
extern Boolean seenIntroScreen;
extern Boolean smuggleFlag;
extern Boolean strictPlay;
extern Boolean systemFlag[kMaxSystems];
extern Boolean weapLawFlag;
extern float weapTargetExpand;

extern Boolean escortGroupFlag[5];
extern short newKeyPrefs[40];
EXTERN_CPP_END
#pragma cplusplus reset

extern Boolean ShipIsInertialess(ShipObj *ship);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern double gSpeedMult;
extern unsigned char shipName[256];
extern PilotRec2 **altPlayerFileHand;
extern PrefsRecord **gPrefsHandle;
#ifdef MAC1010
extern short gPrefsRefNum;
#endif
extern PilotRec **playerFileHand;
EXTERN_CPP_END
#pragma cplusplus reset

extern short SavePilotFile(short stellar);
extern void HandlePlayerCommunication(void);
extern void ActivateCharResource(const unsigned char *name, Boolean applyStats);
#ifdef MACBUILD
extern void DeletePilotFile(ConstStr255Param name);
#else
extern void DeletePilotFile(const char *name);
#endif
extern void ActivateRank(char *rank);
extern void DeactivateRank(char *rank);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short playerRecord[0x800];
EXTERN_CPP_END
#pragma cplusplus reset
extern void ResetPlayer(Boolean newPilot);
extern void ResetPlayerRecord(Boolean newPilot);

#ifdef __cplusplus
}
#endif

#endif
