/* game.h -- starting, running and stopping a game. Notes: Research/notes/game.md */
#ifndef GAME_H
#define GAME_H
#include "gametypes.h"
#include "macport.h"
#include "resources.h"
#include "shipobj.h"
#include "gameconst.h"
#include "spriteworld.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char AIHasAfterburner(ShipObj *ship);
extern void AbortMission(short mission, char evalBits);
void ActivateCron(short index);
extern void AutoAbortMission(short mission);
extern void CleanUp(void);
extern void ClearParticles(char on);
extern short Construct8BitAlphaTable(void);
extern void CosLookup(short angle);
#pragma cplusplus on
extern "C++" {
extern short commCount;
extern unsigned long gJumpStartTick;
extern short oldVolume;
extern SpriteRec *masterStellarSprite[kMasterStellarSprites];
extern SpriteRec *masterShotSprite[256];
}
#pragma cplusplus reset
extern void CreateExplosion(FloatPoint pt, short type, short size, char sound);
extern void SpawnExplod(float a, register float b, short unused, short type,
                        short delay);
#pragma cplusplus on
extern "C++" {
extern void *stringHandle;
}
#pragma cplusplus reset
extern short ConvertSpriteToRLE(SpriteRec *set);
#pragma cplusplus on
extern "C++" {
extern SpriteRec *masterNavTargetSprite;
extern SpriteRec *masterWeapTargetSprite;
extern unsigned short salvageAmmoType;
extern SpriteRec *masterStarSprite;
extern SpriteRec *menuRolloverSprite;
extern SpriteRec *masterAsteroidSprite[16];
extern SpriteRec *gMainButtonSprites[6];
extern SpriteRec *alertSprite;
extern SpriteRec *cursorSprite;
extern SpriteRec *doubleTimeSprite;
extern void *redAlertSnd;
extern void *explodSnd[];
extern void *klaxxonSnd;
extern void *clickSnd;
extern short lastShipAIDispatched;
extern short gameIsHackedIndex;
}
#pragma cplusplus reset
extern unsigned int DeleteAFile(const char *path);
extern void DisplayComm(unsigned char *msg, short count, short font, short size,
                        RGBColor *color, char bold, unsigned char noBlit);
extern void Dispose8BitAlphaTable(void);
extern void DoNewPilot(void);
extern void DoShipCloak(ShipObj *ship);
extern void DoStellarTimePassage(void);
extern char DoesPilotExist(const char *path);
extern void DrawParticlesOffscreen(SpriteWorldPtr spriteWorldP);
extern void EnterGameFromMainScreen(void);
extern short EscapeShipWeapType(ShipObj *ship);
extern char EvalMissionBitTestString(char *expr);
extern void EvalMissionBitSetString(char *expr);
extern short SelectNearestAnyStellar(ShipObj *ship, short exclude);
extern char AIShouldLeaveSystem(ShipObj *ship, short sys);
extern char SufficentTargetedDamage(ShipObj *ship);
extern void MakeEscortsHyperWithParent(ShipObj *ship);
extern void HandleLocalPriceFlux(void);
extern void PointDefenseCollisionHandler(SpriteRec *shotSpr, SpriteRec *otherSpr,
                         Rect *sectRect);
extern void BeamDrawCallback(SpriteWorldPtr world);
extern void BeamDrawOffscreenCallback(SpriteWorldPtr world);
extern void PostDrawCallback(SpriteWorldPtr world);
extern void PostEraseCallback(SpriteWorldPtr world);
extern short SelectMissionCargoType(short v);
extern short SelectMissionCargoQty(short v);
extern void ResetMissionOfferVenue(void);
extern void SkipSpaces(short *pos);
extern char HasAutoEject(ShipObj *ship);
extern char IsKeySlotInUse(short code, short excludeRow);
extern void CheckSpeechMgr(void);
extern char PlayerHasAnyCargo(void);
extern void AddToPlayerKills(int strength);
extern void FrameDefaultButton(void *dlg, short flag);
extern void SWExitSpriteWorld(void);
extern void RestoreControlStrip(void);
extern short InitParticles(int numElements, int gravity);
extern void DisposeParticles(void);
extern char RegScramD3(char *p);
extern char RegScramE3(void);
extern void DoAboutBox(void);
extern void InitMainScreen(void);
extern void FUN_00486ec0(void);
extern void ShowTheMenuBar(void);
extern void FUN_0049e420(void);
extern void FUN_004a51e0(void);
extern void FUN_004a74e0(void);
extern void DisposeIntroProgressBar(void);
extern void RT3_Close(void);
extern long RT3_Open(unsigned char tracked, short mode, const char *product,
                         const char *company);
extern unsigned long ST_GetSysVolume(void);
extern long RT3_RunReminder(unsigned char force, unsigned char *registered);
extern void *ReggieVersionCheck(void *ref, const char *name);
extern long FirstKeyDown(void);
extern char GovtSharedClass(short a, short b);
extern void HandleAlertSprite(void);
extern void HandleDoubleTimeSprite(void);
extern void HandleGlobalPriceFlux(void);
extern void HandleMouse(void);
extern void HandleTimeAdjustment(void);
extern void HideTheMenuBar(void);
extern void IncrementGameTime(void);
extern void InitArrays(void);
extern void InitEVNova(void);
extern void InitMemory(void);
extern void InitMissions(void);
extern void InitObjects(void);
extern void InitSprites(void);
extern void KillVoiceSound(void);
extern void MainScreenIdlePoll(void);
extern void MissionHandlePlayerEnteredNewSystem(void);
extern unsigned int MouseInit(void);
extern void MyProfileDump(const char *path);
extern void MyProfileEnter(short slot);
extern void MyProfileExit(short slot);
extern void MyProfileInit(void);
void MyProfileReset(void);
extern void MyProfileSetName(short slot, const char *name);
extern char OfferMissionFromPort(short venue);
extern void PlayGame(void);
extern char PlayerCanUseAmmoType(short weapon);
extern char PlayerHasJettisonableCargo(void);
extern long long RT3_GetLicenseCode(void);
extern void RefreshWindowBuffer(void);
extern void RegScramB(unsigned char x);
extern char RegScramD(char *p);
extern char RegScramE(void);
extern void ReportSpriteLoadError(short label, short value, short special);
extern void ResetGameData(void);
extern short SelectDudeFieldFromSystem(SystRecord *syst);
extern void SetSoundRate(double rate);
extern void SetupSpriteWorld(void);
extern void SetupWind(void);
extern short ShipTotalHolds(void);
extern void SinLookup(short angle);
extern short SpawnPerson(short sys, unsigned char noDisabled, short person);
extern void SpeakVoiceMessage(short kind, short govt, short voice);
extern void SpeedLimit(FloatPoint *p, float mag);
extern char SuitableMissileType(short weapon, ShipObj *target);
extern char SystemIsLoaded(short sys);
void TerminateCron(short index);
extern short TotalCargo(ShipObj *ship);
extern short TrackMainScreenButtons(Point where);
extern void UpdateAllPortDialogs(void);
extern short WeaponBurstInterval(short weap, short count);
extern char WeaponDisableOnly(short weap);
#pragma cplusplus on
extern "C++" {
extern char absquatulateStr[];
extern SpriteLayerRec *asteroidLayer;
extern SpriteLayerRec *boxLayer;
extern float cosTable[360];
extern short cursorAlpha;
extern short cursorVisible;
extern SpriteLayerRec *disabledShipLayer;
/* the sprite-detail prefs, mac _settings+0x0e..0x11 */
extern unsigned char loadShipSetsPref;
extern unsigned char loadShipGlowPref;
extern unsigned char loadShipWeaponsPref;
extern unsigned char loadShipLightsPref;
extern char doubleTime;
extern SpriteLayerRec *escortLayer;
extern short escortMenuOnscreen;
extern SpriteLayerRec *explodLayer;
extern short firstShipAltIndex;
extern short firstShipGlowIndex;
extern short firstShipLightIndex;
extern unsigned long long fakeLicenseCode;
extern short firstShipShieldIndex;
extern short firstShipWeapIndex;
extern char forceRandom;
extern short fpsCounter;
extern short frameCounter;
extern long fullScreenRestore;
extern void *escapePodSnd;
extern void *cloakSnd[2];
extern char gCommDone;
extern char gLatestVersion[];
extern void *warpUpSnd[2];
extern short gMaxSoundRange;
extern char warpSoundDone;
extern char gPause;
extern short gPluginCount;
extern char majorPrefsChange;
extern char gVersionCheckDone;
extern char didSpeak;
extern char doMainScreenAnim;
extern Rect standardPicRect;
extern short gViewSlotDuration[];
extern char gameFrozen;
extern short gameIsRegisteredIndex;
extern DateTimeRec gameTime;
extern Image_t *greenMapArrow;
extern char ignoreKeys;
extern char keyPressedThisFrame;
extern unsigned long lastEventLoopTime;
extern UnsignedWide lastFrameMicroseconds;
extern FloatPoint lastFramePlayerPos;
extern unsigned long lastKeypressTime;
extern unsigned long lastLoMemWarningTime;
extern unsigned long lastMouseMoveTime;
extern Point lastMousePos;
extern short lastPortMissionLoc;
extern unsigned long lastSmokeUpdateTime;
extern char mBarHidden;
extern Rect mainWindRect;
extern short menuFontID;
extern RGBColor menuTextColor[2];
extern void *missionSnd;
extern short missionSoundID;
extern Str255 newPilotPopupChoice[2];
extern Str255 newPlayerName[2];
extern char pauseKeyFlag;
extern Rect progressDefRect;
extern Image_t *redMapArrow;
extern SpriteLayerRec *shipLayer;
extern char showFPS;
extern float sinTable[360];
extern SpriteLayerRec *smokeLayer;
extern Rect spriteClipRect;
extern SpriteLayerRec *starLayer;
extern SpriteLayerRec *stellarLayer;
extern char suppressMissionOffer[];
extern SpriteLayerRec *targetLayer;
extern float trigResult;
extern void *voiceSnd;
extern void *warpOutSnd;
extern char windowedPref;

extern void *introMusicMovie;

extern unsigned char lastPlayerName[256];
}
#pragma cplusplus reset

extern void IterateInGameEventLoop(short tickCount);
extern short CalcInterceptAngle(FloatPoint *from, FloatPoint *to);
extern short AngularDifference(short a, short b);
extern char AIPerformsComplexManeuvers(ShipObj *ship);
extern void AIFireTurret(ShipObj *ship);
extern void DateAfterDays(DateTimeRec *out, short days);
extern void ResumePlay(short unused);
extern void SwitchedOut(short unused);
extern void WarpUpCallback(int which, int a, int b);
extern void SpeakIt(char *msg, short flag);
extern char CanBuyGoods(short item);
extern char CanSellGoods(short item);
extern void PropagateMissionBitEffects(void);
extern void OnTheFlyWhiteFade(short pct);
extern void ExploreNebulaeFromSystem(short sys);
#pragma cplusplus on
extern "C++" {
extern short gViewSlot[5];
}
#pragma cplusplus reset
#pragma cplusplus on
extern "C++" {
extern short menuTextSize;
extern unsigned long lastCursorAnimTime;
extern unsigned long lastFrameTime;
}
#pragma cplusplus reset
extern short TotalFleetHolds(void);
extern void DeleteAllComputerShips(char force, char keepPlayer);
#pragma cplusplus on
extern "C++" {
extern void *beepSnd[5];

extern SpriteRec *stellarSprite[kStellarSprites];
}
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
