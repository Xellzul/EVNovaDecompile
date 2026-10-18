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
extern "C++" {
extern char DAT_005914d1;
extern char DAT_005914d2;
extern char DAT_005914d4;
extern char yieldEventsPref;
extern char parallaxStarsPref;
extern char planetSoundsPref;
extern unsigned char gShowInfluence;
extern char noHyperGammaPref;
extern char noVersionCheckPref;
extern short recentActivatedRank;
extern short gammaPref;
extern KeyPrefs keyPrefs;
extern unsigned long lastEscortMenuTime;
extern double DAT_00735450;
extern char DAT_00735488;
extern float playerRepairInhibit;
extern char playerHasIllegalJunk;
extern char playerHasIllegalOutfits;
extern char playerHasTribbles;
extern char playerHasPerishableJunk;
extern char playerTurnedThisFrame;
extern float sabotageCounter;
extern char lastJumpDistOk;
extern char ackCommFlag;
extern char navModeSelectFlag;
extern char navTargetSelectFlag;
extern char navDefaultSelectFlag;
extern char helpKeyFlag;
extern char cloakFlag;
extern char fighterCommandFlag;
extern char weapModeSelectFlag;
extern char weapTargetSelectFlag;
extern char weapDefaultSelectFlag;
extern char playerInfoFlag;
extern char mapTryFlag;
extern char jumpTryFlag;
extern char weapSelectFlag;
extern char spawnShipFlag;
extern char weapCloseTargFlag;
extern char missionInfoFlag;
extern char commTryFlag;
extern char mapZoomFlag;
extern char framerateFlag;
extern char escortMenuFlag;
extern char escortCommandFlag[];
extern short lastEscortCount;
}
#pragma cplusplus reset
extern void DrawCombatRatingString(void);
extern void DrawLegalStatusString(short curSystem);
extern void DumpPilotLog(void);
extern char HasCloak(ShipObj *ship);
extern char HasAreaCloak(ShipObj *ship);
extern char CloakZeroesShields(ShipObj *ship);
extern char ShipDecloaksWhenHit(ShipObj *ship);
extern short CloakShieldUsage(ShipObj *ship);
extern char ShipCanSeeCloakedShips(ShipObj *ship, short which);
extern short ShipHyperTransitTime(ShipObj *ship);
extern void KeyPrefsToSlotTable(void);
extern void SlotTableToKeyPrefs(void);
extern float ShipDeionizationRate(ShipObj *ship);
extern short PlayerEffectiveMurk(void);
extern short ShipJumpsPerJump(ShipObj *ship);
extern char ShipCanExpiditeJumps(ShipObj *ship);
extern float ShipAfterburnerFuelUsage(ShipObj *ship);
extern void SetDefaultPrefs(void);
extern void SetDefaultKeys(void);
extern long FreeMass(void);
extern void GetHaggleReply(short which);
extern void HandlePlayer(ShipObj *ship);
extern void HandlePlayerDockRequest(void);
extern char HasAfterburner(ShipObj *ship);
extern char HasDensityScanner(ShipObj *ship);
extern char HasIFF(ShipObj *ship);
extern void LoadPrefs(void);
extern void LoadResponse(short which);
extern long DoEncryption(void *buf, unsigned long len, unsigned long key);
extern long SaveAFile(const char *path, const void *data, unsigned long length);
extern short LoadPilotData(char *path);
extern void OpenLastPilotFile(void);
extern char OpenPilotFile(char *missingResource);
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
extern char ShipCanScoop(ShipObj *ship);
extern char ShipCanTargetCloakedShips(ShipObj *ship);
extern char ShipCanTargetUntargetableShips(ShipObj *ship);
extern char ShipCloakVisibleOnRadar(ShipObj *ship);
extern long ShipHyperSafeDist(ShipObj *ship);
extern float ShipIonizationFactor(ShipObj *ship);
extern short ShipResistsDeadlyStellars(ShipObj *ship);
extern short ShipResistsGravity(ShipObj *ship);
extern void ShowAlert(const char *title, const char *message);
extern void SlapWithPenalty(short sys, short govt, short kind,
                            short mission);
extern short WritePilotData(short stellar);
#pragma cplusplus on
extern "C++" {
extern short adviceRandom;
extern short alertSpritePending;
extern short antiEscortDelay;
extern char autoDockFlag;
extern short bribeRandom;
extern char cheatsActive;
extern float destructCount;
extern short dockPermission;
extern short dockTarget;
extern char dontMovePlayerAfterLanding;
extern char fadePending[2];
extern float gCachedMod15;
extern short playerHasAreaCloak;
extern short gCachedMod24;
extern short gCachedMod30;
extern short gCachedMod30b;
extern short gCachedMod38;
extern float gCachedMod39;
extern float gCachedMod40;
extern short gCachedMod41;
extern short gCachedMod42;
extern float gCachedShieldRech;
extern char gEarlyOutFlag;
extern short gHasModType13;
extern short gHasModType14;
extern char gNoDebris;
extern char gPrefsDirectory[];
extern short gSelectedQuickKey;
extern short globalPriceFlux[2];
extern char keyComboBlocked;
extern char killAsteroids;
extern char killBoxes;
extern char killExplods;
extern char killSmoke;
extern short lastMissionStellar;
extern char leetMode;
extern short localPriceFlux[2];
extern char musicEnabledPref;
extern float navTargetExpand;
extern short newbieFlag;
extern char noSavePilotFiles;
extern char noUpdatePilotData;
extern char oldPlayerThreatened;
extern char pilotsFolder[];
extern short playerGender;
extern short playerHasAfterburner;
extern char playerIsLanded;
extern long playerKills;
extern FloatPoint playerMaxVel;
extern short playerSabotaged;
extern float playerShieldCapacity;
extern char playerThreatened;
extern char seenIntroScreen;
extern char smuggleFlag;
extern char strictPlay;
extern char systemVisited[kMaxSystems];
extern short volumePref;
extern char weapLawFlag;
extern float weapTargetExpand;
extern char windowedPrefPending;

extern char escortGroupFlag[5];
extern short gSlotTable[36];
}
#pragma cplusplus reset

extern char ShipIsInertialess(ShipObj *ship);
#pragma cplusplus on
extern "C++" {
extern double gTimeScale;
extern unsigned char shipName[256];
extern PilotRec2 **altPlayerFileHand;
extern PrefsRecord **gPrefsHandle;
extern PilotRec **playerFileHand;
}
#pragma cplusplus reset

extern short SavePilotFile(short stellar);
extern void HandlePlayerCommunication(void);
extern void ActivateCharResource(const unsigned char *name, char applyStats);
extern void DeletePilotFile(const char *name);
extern void ActivateRank(char *rank);
extern void DeactivateRank(char *rank);
#pragma cplusplus on
extern "C++" {
extern short playerLegalStatus[0x800];
}
#pragma cplusplus reset
extern void ResetPlayer(char newPilot);
extern void ResetPlayerRecord(char newPilot);

#ifdef __cplusplus
}
#endif

#endif
