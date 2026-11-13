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

extern Boolean AIHasAfterburner(ShipObj *ship);
extern void AbortMission(short mission, Boolean evalBits);
void ActivateCron(short index);
extern void AutoAbortMission(short mission);
extern void Bye(void);
extern void ClearParticles(Boolean on);
#if !defined(MACBUILD) || !V111 /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short Construct8BitAlphaTable(void);
#endif
extern void TableCos(short angle);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short commCount;
extern uint32_t floatingMapStartTime;
extern short oldVolume;
extern SpriteRec *masterStellarSprite[kMasterStellarSprites];
extern SpriteRec *masterShotSprite[256];
EXTERN_CPP_END
#pragma cplusplus reset
extern void CreateExplosion(FloatPoint pt, short type, short size, Boolean sound);
extern void SpawnExplod(FloatPoint pos, short unused, short type, short delay);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Handle stringHandle;
EXTERN_CPP_END
#pragma cplusplus reset
extern short ConvertSpriteToRLE(SpriteRec *set);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern SpriteRec *masterNavTargetSprite;
extern SpriteRec *masterWeapTargetSprite;
extern SalvageAmmo salvageAmmoType;
extern SpriteRec *masterStarSprite;
extern SpriteRec *menuRolloverSprite;
extern SpriteRec *masterAsteroidSprite[16];
extern SpriteRec *menuButtonSprite[6];
extern SpriteRec *alertSprite;
extern SpriteRec *cursorSprite;
extern SpriteRec *doubleTimeSprite;
extern void *redAlertSnd;
extern void *explodSnd[];
extern void *klaxxonSnd;
extern void *clickSnd;
extern short lastShipAIDispatched;
extern short gameIsHackedIndex;
EXTERN_CPP_END
#pragma cplusplus reset
extern unsigned int DeleteAFile(const char *path);
extern void DisplayComm(PStrChar *msg, short count, short font, short size,
                        RGBColor *color, Boolean bold, unsigned char noBlit);
extern void Dispose8BitAlphaTable(void);
extern void DoNewPilot(void);
extern void DoShipCloak(ShipObj *ship);
extern void DoStellarTimePassage(void);
#ifdef MACBUILD
extern Boolean DoesPilotExist(ConstStr255Param name);
#else
extern Boolean DoesPilotExist(const char *path);
#endif
extern void DrawParticlesOffscreen(SpriteWorldPtr spriteWorldP);
extern void EnterGameFromMainScreen(void);
extern short EscapeShipWeapType(ShipObj *ship);
extern Boolean EvalMissionBitTestString(char *expr);
extern void EvalMissionBitSetString(char *expr);
extern short SelectNearestAnyStellar(ShipObj *ship, short exclude);
#ifndef MACBUILD /* static in ai.c on 1.1.1 */
extern Boolean AIShouldLeaveSystem(ShipObj *ship, short sys);
#elif defined(MAC1010)
extern Boolean AIShouldLeaveSystem(ShipObj *ship, short sys);
#endif
extern Boolean SufficentTargetedDamage(ShipObj *ship);
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
extern void Trim(short *pos);
extern Boolean HasAutoEject(ShipObj *ship);
extern Boolean DupKeyCheck(short code, short excludeRow);
extern void CheckSpeechMgr(void);
extern Boolean PlayerHasAnyCargo(void);
extern void AddToPlayerKills(int strength);
extern void FrameDefaultButton(void *dlg, short flag);
extern void SWExitSpriteWorld(void);
extern void RestoreControlStrip(void);
#if !defined(MACBUILD) || !V111 /* the 1.1.1 Setup.c calls it undeclared (implicit int) */
extern short InitParticles(int numElements, int gravity);
#endif
extern void DisposeParticles(void);
#ifndef MACBUILD /* inline on mac: sources/mac/include/rt3_ev_nova.h */
extern Boolean RegScramD3(char *p);
extern Boolean RegScramE3(void);
#endif
extern void DoAboutBox(void);
extern void InitMainScreen(void);
extern void FUN_00486ec0(void);
extern void ShowTheMenuBar(void);
extern void FUN_0049e420(void);
extern void CreateMapTriggerRgn(void);
extern void RedrawNebulaeOffscreen(void);
extern void DisposeIntroProgressBar(void);
extern void RT3_Close(void);
#ifdef MACBUILD
extern int32_t RT3_Open(unsigned char tracked, unsigned short mode, const char *product,
                         const char *company);
#else
extern int32_t RT3_Open(unsigned char tracked, short mode, const char *product,
                         const char *company);
#endif
extern uint32_t ST_GetSysVolume(void);
extern int32_t RT3_RunReminder(unsigned char force, unsigned char *registered);
extern void *ReggieVersionCheck(void *ref, const char *name);
extern short FirstKeyDown(void);
extern Boolean GovtSharedClass(short a, short b);
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
extern void MainScreenIdle(void);
extern void MissionHandlePlayerEnteredNewSystem(void);
extern unsigned int MouseInit(void);
extern void MyProfileDump(const char *path);
extern void MyProfileEnter(short slot);
extern void MyProfileExit(short slot);
extern void MyProfileInit(void);
void MyProfileReset(void);
extern void MyProfileSetName(short slot, const char *name);
extern Boolean OfferMissionFromPort(short venue);
extern void PlayGame(void);
#ifndef MAC1010 /* 1.0.10's definition returns long, commdlg.c holds the callers' view */
extern Boolean PlayerCanUseAmmoType(short weapon);
#endif
extern Boolean PlayerHasJettisonableCargo(void);
#if defined(MACBUILD) && !defined(MAC1010) /* the mac librt3 returns ULongLong (sources/mac/rt3/reg_tool_3.c) */
extern unsigned long long RT3_GetLicenseCode(void);
#else
extern long long RT3_GetLicenseCode(void);
#endif
extern void RefreshWindowBuffer(void);
extern void RegScramB(unsigned char x);
extern Boolean RegScramD(char *p);
extern Boolean RegScramE(void);
extern void ReportSpriteLoadError(short label, short value, short special);
extern void ResetGameData(void);
extern short SelectDudeFieldFromSystem(SystRecord *syst);
extern void AdjustWarpSoundSpeed(double rate);
extern void SetupSpriteWorld(void);
extern void SetupWind(void);
extern short ShipTotalHolds(void);
extern void TableSin(short angle);
extern short SpawnPerson(short sys, unsigned char noDisabled, short person);
extern void SpeakVoiceMessage(short kind, short govt, short voice);
extern void SpeedLimit(FloatPoint *p, float mag);
extern Boolean SuitableMissileType(short weapon, ShipObj *target);
extern Boolean SystemVisible(short sys);
void TerminateCron(short index);
extern short TotalCargo(ShipObj *ship);
extern short TrackMainScreenButtons(Point where);
extern void UpdateAllPortDialogs(); /* unprototyped: the 1.1.1 DoShipyardDialog passes it the port dialog */
extern short WeaponBurstInterval(short weap, short count);
extern Boolean WeaponDisableOnly(short weap);
#pragma cplusplus on
EXTERN_CPP_BEGIN
#ifdef MACBUILD
extern unsigned char absquatulateStr[];
#else
extern char absquatulateStr[];
#endif
extern SpriteLayerRec *asteroidLayer;
extern SpriteLayerRec *boxLayer;
extern float tableCos[360];
extern short cursorAlpha;
extern short cursorHidden;
extern SpriteLayerRec *disabledShipLayer;
extern Boolean doubleTime;
extern SpriteLayerRec *escortLayer;
extern short escortMenuOnscreen;
extern SpriteLayerRec *explodLayer;
extern short firstShipAltIndex;
extern short firstShipGlowIndex;
extern short firstShipLightIndex;
extern unsigned long long fakeLicenseCode;
extern short firstShipShieldIndex;
extern short firstShipWeapIndex;
extern Boolean forceRandom;
extern short fpsCounter;
extern short frameCounter;
extern int32_t fullScreenRestore;
extern void *escapePodSnd;
extern void *cloakSnd[2];
extern Boolean redrawEscortMenu;
extern char reggieVersion[];
extern void *warpUpSnd[2];
extern short playerEffectiveMurk;
extern Boolean warpSoundDone;
extern Boolean gPause;
extern short pluginCount;
extern Boolean majorPrefsChange;
extern Boolean reggieSuccess;
extern Boolean didSpeak;
extern Boolean doMainScreenAnim;
extern Rect standardPicRect;
extern short gViewSlotDuration[];
extern Boolean gameFrozen;
extern short gameIsRegisteredIndex;
extern DateTimeRec gameTime;
extern Image_t *greenMapArrow;
extern Boolean ignoreKeys;
extern Boolean keyPressedThisFrame;
extern uint32_t lastEventLoopTime;
extern UnsignedWide lastFrameMicroseconds;
extern FloatPoint lastFramePlayerPos;
extern uint32_t lastKeypressTime;
extern uint32_t lastLoMemWarningTime;
extern uint32_t lastMouseMoveTime;
extern Point lastMousePos;
extern short lastPortMissionLoc;
extern uint32_t lastSmokeUpdateTime;
extern Boolean mBarHidden;
extern Rect mainWindRect;
extern short menuFontID;
extern RGBColor menuTextColor[2];
extern void *missionSnd;
extern short missionSoundID;
extern Str255 newPilotPopupChoice[2];
extern Str255 newPlayerName[2];
extern Boolean pauseKeyFlag;
extern Rect progressDefRect;
extern Image_t *redMapArrow;
extern SpriteLayerRec *shipLayer;
extern Boolean showFPS;
extern float tableSin[360];
extern SpriteLayerRec *smokeLayer;
extern Rect spriteClipRect;
extern SpriteLayerRec *starLayer;
extern SpriteLayerRec *stellarLayer;
extern Boolean suppressMissionOffer[];
extern SpriteLayerRec *targetLayer;
extern float trigResult;
extern void *voiceSnd;
extern void *warpOutSnd;

extern Movie introMusicMovie;

extern unsigned char lastPlayerName[256];
EXTERN_CPP_END
#pragma cplusplus reset

extern void IterateInGameEventLoop(short tickCount);
extern short CalcInterceptAngle(FloatPoint *from, FloatPoint *to);
extern short AngularDifference(short a, short b);
extern Boolean AIPerformsComplexManeuvers(ShipObj *ship);
extern void AIFireTurret(ShipObj *ship);
extern void SetMissionDeadline(DateTimeRec *out, short days);
extern void SwitchedIn(EventRecord *ev);
extern void SwitchedOut(EventRecord *ev);
extern void WarpUpCallback(int which, int a, int b);
extern void SpeakIt(char *msg, short flag);
extern Boolean CanBuyGoods(short item);
extern Boolean CanSellGoods(short item);
extern void PropagateMissionBitEffects(void);
extern void OnTheFlyWhiteFade(short pct);
extern void ExploreNebulaeFromSystem(short sys);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short gViewSlot[5];
EXTERN_CPP_END
#ifdef MACBUILD
extern EventRecord gEvent;
extern void MainScreenMouse(EventRecord *event);
extern void DoMenuClick(int32_t menuChoice);
extern void MainScreenActivate(Boolean activate);
extern void AdjustMenus(void);
extern void MainScreenClose(void);
extern void MainScreenClobber(void);
extern void CERunPrefsDialog(void);
extern void GetWindowDragBounds(Rect *bounds, Point where);
extern void ForceWindowOnscreen(void);
extern void FlushKeys(void);
extern void CancelMusicThread(void);
extern short ScanForKeys(void);
extern OSStatus MoreAECreateAppleEventSelfTarget(AEEventClass eventClass, AEEventID eventID,
                                                 AppleEvent *event);
extern short introTextID;
extern short introPictID[4];
extern short introPictDelay[4];
#endif
#pragma cplusplus reset
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short menuTextSize;
extern uint32_t lastCursorAnimTime;
extern uint32_t lastFrameTime;
EXTERN_CPP_END
#pragma cplusplus reset
extern short TotalFleetHolds(void);
extern void DeleteAllComputerShips(Boolean force, Boolean keepPlayer);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern void *beepSnd[5];

extern SpriteRec *stellarSprite[kStellarSprites];
EXTERN_CPP_END
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
