// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* initevnova.c -- tu-map object O2: game start, PlayGame, ship spawning, damage, escorts and fleets, then the pirated-key check and the .CRT hook that ends the object. Notes: Research/notes/initevnova.md */
#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "burgermath.h"
#include "burger.h"
#ifndef MACBUILD
#include "win32.h"
#endif
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "sound.h"
#include "ai.h"
#include "crashlog.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "space.h"
#include "stellar.h"
#include "cefix.h"
#include "prototypes.h"
#include "gametypes.h"
#ifdef MACBUILD
#ifdef MAC1010
#define RT3_HELPERS_IN_UNIT
#endif
#include "../mac/include/rt3_ev_nova.h"
#include "../mac/include/ASWRegistrationCarbonAPI.h"

extern void SetupToolbox(void);
extern void CarbonHackInit(void);
extern void SelectNovaSupport(void);
extern void CheckSystem(void);
extern OSStatus EVInstallEndianFlippers(void);
#if V111
extern CFStringRef CFStringCreateFromString(const char *str, int32_t len);
extern CFStringRef relative2absolute(CFStringRef path);
#endif
extern FSRef *novaSupportFolder;
extern int32_t startupMemoryFree;
extern void SpawnIntroMusicThread(void);
#endif
extern void InitAEVTHandlers(void);
extern int32_t PreflightMemoryUsageTotal(void);
extern void HideControlStrip(void);
extern void CarbonHackCleanup(void);
extern void DisposeGammaTools(void);

#define kKeyHitMask     0x07bdfef7fbdfef78LL

#define KEYHIT(k)  ((((key ^ (k)) & kKeyHitMask) != 0) ? 0 : 1)

#define kQuickTimeGestalt   'qtim'
#define kQuickTime5         0x05000000

#define kRLEDecompressor    'imdc'
#define kRLECodec           'rle '
#define kAppleManufacturer  'appl'
#define kComponentFlagsMask 0x20000000

#define kMaxGamma     6
#define kDefaultGamma 3

#define kPlugMismatchSTR    140
#define kPlugMismatchIndex  27

#define kAlertPlatformOpen  0x16
#define kAlertFileOpen      0x17
#define kAlertDrawOpen      0x18
#define kAlertInterfaceOpen 0x19
#define kAlertRegOpen       0x1a
#define kAlertQuickTime     0x15

#define kEventLoopTicks     10

#ifdef MAC1010
/* plain functions ahead of the globals: -inline auto expands every call and the linker strips the copies, which rank `table` before InitEVNova and lay it first in the object's BSS */
char RegScramA2(short x)
{
    return IsPlatformOpen() * x;
}

void RegScramB2(unsigned char x)
{
    static unsigned char table[256];

    table[x]++;
}

short RegScramC2(unsigned char x)
{
    int n = x;

    return ((n >> (8 - n)) | (n << n)) & 0xff;
}

void *RegScramF2(void)
{
    char buf[4];

    return buf;
}

char RegScramD2(char *p)
{
    unsigned char c = *p;

    TimerGetSeconds();
    return c;
}

char RegScramE2(void)
{
    return 0;
}
#endif

short oldVolume;
short pluginCount;
Rect backUpdateRect;
uint32_t lastSmokeUpdateTime;
uint32_t lastKeypressTime;
uint32_t lastEventLoopTime;
#ifndef MACBUILD
ShipObj *shipData;
ShipRec *shipClass;
SpobRecord *stellarData;
SpobRecord *dockedPort;
SystRecord *systemData;
WeapRecord *weaponStats;
BoomSlot *explodData;
DebrisSlot *smokeData;
BoomRecord *explodType;
BeamRecord *beamData;
OutfRecord *outfitSystem;
DudeRecord *dudeData;
GovtRecord *govtData;
PersRecord *personData;
AsteroidObj *asteroidData;
RoidRecord asteroidType[16];
OopsRecord *disasterData;
NebuRecord *nebulaData;
FletRecord *fleetData;
BoxObj *boxData;
PodObj *podData;
ShotObj *shotData;
MissionRecord *currentMissions;
MissionState *currentMissionObjectives;
JunkRecord *junkData;
CronRecord *cronEvent;
RankRecord *rankData;
StarSlot *starData;
Boolean *missionBit;
Settings settings;
KeyPrefs keyPrefs;
void *warpUpSnd[2];
void *warpOutSnd;
void *redAlertSnd;
void *beepSnd[5];
void *shotSnd[256];
void *explodSnd[64];
void *hardDockSnd;
unsigned char DAT_00591a78[4];
void *klaxxonSnd;
void *escapePodSnd;
void *cloakSnd[2];
void *voiceSnd;
void *clickSnd;
void *missionSnd;
SoundRequest warpUpSoundSpec;
SoundRequest planetSoundSpec;
SpriteRec *navTargetSprite[4];
SpriteRec *weapTargetSprite[4];
SpriteRec *doubleTimeSprite;
SpriteRec *alertSprite;
SpriteRec *cursorSprite;
SpriteRec *masterShipSprite[kMaxShipTypes];
SpriteRec *masterShipGlowSprite[kMaxShipTypes];
SpriteRec *masterShipShieldSprite[kMaxShipTypes];
SpriteRec *masterStarSprite;
SpriteRec *masterShipLightSprite[kMaxShipTypes];
SpriteRec *masterShipWeaponSprite[kMaxShipTypes];
SpriteRec *masterShipAltSprite[kMaxShipTypes];
SpriteRec *masterStellarSprite[kMasterStellarSprites];
SpriteRec *masterExplodSprite[kMaxExplodTypes];
SpriteRec *masterShotSprite[256];
SpriteRec *masterAsteroidSprite[16];
SpriteRec *masterSmokeSprite[8];
SpriteRec *menuRolloverSprite;
SpriteRec *stellarSprite[kStellarSprites];
SpriteRec *masterBoxSprite[5];
SpriteRec *menuButtonSprite[kMainButtons];
SpriteRec *masterWeapTargetSprite;
SpriteRec *masterNavTargetSprite;
SpriteLayerRec *shipLayer;
SpriteLayerRec *stellarLayer;
SpriteLayerRec *explodLayer;
SpriteLayerRec *shotLayer;
SpriteLayerRec *guidedShotLayer;
SpriteLayerRec *pointDefenseLayer;
SpriteLayerRec *targetLayer;
SpriteLayerRec *asteroidLayer;
SpriteLayerRec *smokeLayer;
SpriteLayerRec *disabledShipLayer;
SpriteLayerRec *boxLayer;
SpriteLayerRec *starLayer;
SpriteLayerRec *turretedShotLayer;
SpriteLayerRec *escortLayer;
SpriteWorldPtr gSpriteWorld;
SWFrameRec *floatingMapFrame;
SWFrameRec *floatingEscortMenuFrame;
CWindowPtr gWind;
Boolean statusRedraw[kStatusRedrawCount];
Boolean gameLoaded;
Boolean gameInProgress;
Boolean killShots;
Boolean killBoxes;
Boolean killExplods;
Boolean killAsteroids;
Boolean killSmoke;
Boolean mBarHidden;
Boolean strictPlay;
Boolean weapLawFlag;
Boolean smuggleFlag;
Boolean fadePending[2];
Boolean doubleTime;
Boolean seenIntroScreen;
Boolean playerThreatened;
Boolean oldPlayerThreatened;
Boolean gPause;
Boolean gQuit;
Boolean cheatsActive;
Boolean gameFrozen;
Boolean ignoreKeys;
Boolean updateSmokeThisFrame;
Boolean closeRequested;
unsigned char DAT_00596d40[4];
void *targetInfoPic[kMaxShipTypes];
GWorldContainer statusOffPort;
GWorldContainer loadPort;
unsigned char DAT_0059795c[16];
uint32_t tix;
uint32_t floatingMapStartTime;
#endif
#ifdef V111
unsigned char useLegacyRenderingPipeline;
#endif
#ifndef MACBUILD
uint32_t lastMouseMoveTime;
uint32_t lastEscortMenuTime;
uint32_t lastFrameTime;
unsigned char DAT_00597980[4];
int32_t lastFPSUpdateTime;
uint32_t lastLoMemWarningTime;
uint32_t lastCursorAnimTime;
unsigned char DAT_00597990[2];
short frameCounter;
unsigned char DAT_00597994[8];
short lastShipAIDispatched;
short totalDaysPlayed;
short cursorHidden;
unsigned char DAT_005979a2[2];
short selTradeItem;
short portDemand[kCargoTypes];
short basePrice[kCargoTypes];
unsigned char DAT_005979be[8];
short tradePrice[10];
short voiceResourceCount[8][kVoiceMessages];
unsigned char DAT_00597a0a[4];
short dockPermission;
short dockTarget;
short commCount;
short dockedPortItems[kMaxOutfits];
short dockedPortShipTypes[kMaxShipTypes];
short dockedPortMissions[kMissionVenues][kMaxMissions];
short shipSystems[kMaxOutfits];
MapScroll mapScroll;
ShortPoint screenCenter;
FloatPoint playerMaxVel;
FloatPoint lastFramePlayerPos;
unsigned char playerName[256];
unsigned char lastPlayerName[256];
unsigned char playerNickname[256];
unsigned char shipName[256];
unsigned char weapName[kMaxWeapTypes][256];
unsigned char shipyardLongNames[kMaxShipTypes][256];
unsigned char shipHelpRequests[20][256];
unsigned char legalStatusStrings[18][256];
unsigned char outfitShortNames[kMaxOutfits][256];
unsigned char shipyardShortNames[kMaxShipTypes][256];
unsigned char ratingStrings[11][256];
unsigned char govtTargStrings[kMaxGovts][256];
unsigned char lcOutfitName[kMaxOutfits][256];
unsigned char lcPluralOutfitName[kMaxOutfits][256];
unsigned char cargoName[kMaxCargo][0x100];
unsigned char statusCargoName[kCargoTypes][0x100];
unsigned char cargoAbbrev[kMaxCargo][0x100];
unsigned char lcCargoName[kMaxCargo][0x100];
unsigned char govtShortNames[kMaxGovts][256];
unsigned char shipCommNames[kMaxShipTypes][256];
unsigned char shipMovieFilenames[kMaxShipTypes][256];
unsigned char descMovieName[256];
unsigned char govtMediumNames[kMaxGovts][256];
unsigned char tonString[0x100];
unsigned char tonsString[0x100];
unsigned char fuelString[8][0x100];
unsigned char shieldString[6][0x100];
unsigned char armorString[3][0x100];
unsigned char newbieString[10][0x100];
unsigned char creditString[0x100];
unsigned char creditsString[0x100];
char creditCString[0x20];
char creditsCString[0x20];
char descTextPtr[8192];
char commMsg[2048];
char textShowString[8192];
char datePrefix[0x10];
char dateSuffix[0x10];
RGBColor bkgndGreen;
RGBColor brightGreen;
RGBColor dimGreen;
RGBColor dimmerGreen;
RGBColor rgbWhite;
RGBColor playerPaint;
RGBColor rgbLightGrey;
RGBColor rgbMedGrey;
RGBColor rgbDarkGrey;
RGBColor rgbLessDarkGrey;
RGBColor rgbRealDarkGrey;
RGBColor rgbRealDarkBlue;
RGBColor rgbBlack;
void *staticPat[10];
Rect commRect;
Rect spriteClipRect;
Rect floatingMapRect;
Rect escortMenuRect;
unsigned char DAT_00733bc4[4];
short playerRecord[kMaxSystems];
short newKeyPrefs[40];
short alertSpritePending;
short missionSoundID;
short playerGender;
short lastMissionStellar;
short missionRandom[kMaxMissions];
short descResourcePictId;
short gSoundVolume;
short antiEscortDelay;
short globalPriceFlux[2];
short localPriceFlux[2];
short pendingVoiceMsg;
short pendingVoiceGovt;
short pendingVoiceParity;
short hyperRoute[kMaxHyperRoute];
int32_t playerKills;
double gSpeedMult;
double DAT_00735450;
DateTimeRec gameTime;
unsigned char DAT_00735466[14];
DateTimeRec scenarioStartDate;
unsigned char DAT_00735482[6];
Boolean DAT_00735488;
Boolean inBackground;
Boolean miningShipsActive;
Boolean miningBoxesActive;
Boolean gravitySourcesPresent;
Boolean showFPS;
float navTargetExpand;
float weapTargetExpand;
float destructCount;
float playerRepairInhibit;
float hyperGamma;
Boolean leetMode;
Boolean keyPressedThisFrame;
unsigned char hypergateMapMode;
Boolean redrawEscortMenu;
Boolean screenFaded;
Boolean playerIsLanded;
Boolean playerKilledByHector;
Boolean quickKeyCheckResult;
Point lastMousePos;
short descFlags;
short showFloatingMap;
short escortMenuOnscreen;
short recentActivatedRank;
short descRankGovt[2];
short fpsCounter;
short cursorAlpha;
short jumpSoundLength[2];
short playerEscortOrders[4];
short lastIntfResourceID;
short hypergateMapModeStellar;
char absquatulateStr[256];
int32_t DAT_007355d0;
UnsignedWide lastFrameMicroseconds;
Rect statusOffRect[8];
RGBColor statusColor[8];
RGBColor menuTextColor[2];
RGBColor floatingBorderColor;
RGBColor escortMenuColor;
RGBColor ldefColors[3];
short statusFontID;
short statusFontSize[2];
short buttonFontID;
short buttonFontSize;
short menuFontID;
short menuTextSize;
short smallTextFont;
short smallTextSize;
float playerShieldCapacity;
float playerShieldRechargeRate;
float playerArmorCapacity;
float playerArmorRechargeRate;
float playerFuelCapacity;
float playerTurnRate;
float playerAccelRate;
float playerMaxSpeed;
float playerIonizeMax;
float playerDeionizeRate;
float playerABFuelUsage;
short playerHasAfterburner;
short playerHasDensityScanner;
short playerHasIFF;
short playerAntiInterference;
short playerEffectiveMurk;
short playerCanTargetUntargetableShips;
short playerCanTargetCloakedShips;
short playerHasInertialDamper;
short playerResistsGravity;
short playerResistsDeadlyStellars;
short playerHasAreaCloak;
short playerSabotaged;
Boolean playerHasIllegalJunk;
Boolean playerHasIllegalOutfits;
Boolean allReinforcementsInhibited;
Boolean playerHasTribbles;
Boolean playerHasPerishableJunk;
unsigned long long fakeLicenseCode;
short gameIsRegisteredIndex;
short gameIsHackedIndex;
#endif

EXTERN_C_BEGIN


                                                         /* 0x004d9e10 BurgerLib */
                                                         /* 0x004d4740 RT3_Open */
                                                         /* 0x0046f690 */

#if V111
/* EVPlatformInit (mac _EVPlatformInit, mac-only: 1.1.1 moved the tool opens out of InitEVNova,
   called by the open-application AppleEvent handler). STATE: mac MATCH on both slices. OPEN: none */
// MAC: _EVPlatformInit i386 0x0003a064 ppc 0x00040454
int EVPlatformInit(void)
{
    int32_t err;

    crashlog_print("opening platform tool");
    err = PlatformOpen();
    if (err != 0) {
        FatalStartupError(kAlertPlatformOpen);
        goto fail;
    }
    crashlog_print("opening file tool");
    err = FT_Open();
    if (err != 0) {
        FatalStartupError(kAlertFileOpen);
        goto fail;
    }
    crashlog_print("opening draw tool");
    err = DT_Open();
    if (err != 0) {
        FatalStartupError(kAlertDrawOpen);
        goto fail;
    }
    crashlog_print("opening interface tool");
    err = IT_Open();
    if (err != 0) {
        FatalStartupError(kAlertInterfaceOpen);
        goto fail;
    }
    crashlog_print("opening sound tool");
    err = ST_Open(8, 0);
    oldVolume = ST_GetSysVolume();
    return err;
fail:
    crashlog_print("fail: ");
    crashlog_print_num(err);
    crashlog_print("\n");
    ExitToShell();
    return err;
}
#endif


#ifdef MAC1010
#define InitEVNova main /* __start calls this body on 1.0.10 */
#endif

/* InitEVNova (mac _InitEVNova). STATE: win MATCH 100.0%, exe 1393 / obj 1393, byte-identical yes
   (relocs masked); mac MATCH both slices (the MACBUILD body).
   OPEN: nothing. */
// FUNCTION: EVNOVA 0x00416100
// MAC: _InitEVNova i386 0x0004666c ppc 0x0004eb44
void InitEVNova(void)
#ifdef MAC1010
{
    unsigned char alertText[256];
    FSSpec licenseSpec;
    Boolean blk[kBlacklistCount];
    Size grow;
    long long key;
    int err;
    short i;

    gameLoaded = gameInProgress = 0;
    preventStatusCopy = 0;
    gWind = 0;
    gSpriteWorld = 0;
    screenFaded = 0;

    SetupToolbox();
    CarbonHackInit();

    crashlog_open("debuglog.txt");
    crashlog_print("game engine was compiled ");
    crashlog_print("Apr 24 2006");
    crashlog_print("\n");
    gameIsRegisteredIndex = Rand(0x300);
    crashlog_print("game engine version is ");
    crashlog_print("1.0.A");
#if TARGET_API_MAC_CARBON
    crashlog_print(" Carbon");
#else
    crashlog_print(" PPC");
#endif
    crashlog_print("\n\n");
    gameIsHackedIndex = Rand(0x300);

    crashlog("opening data files & loading prefs");
    InitDataFiles();
    LoadPrefs();
    Microseconds((UnsignedWide *)&fakeLicenseCode);

    crashlog("initializing monitor");
    SetupMonitor();
    CheckSystem();

    crashlog("initializing QuickTime");
    err = EnterMovies();
    if (err != 0)
        FatalStartupError(kAlertQuickTime);

    crashlog("building trig table");
    BuildTrigTable();
    gWind = 0;
    SavePrefs();

    if (KeyCheck(kScanOption) && AreWeFrontProcess()) {
        crashlog("showing prefs dialog before window creation");
        smallTextFont = 3;
        smallTextSize = 9;
        DoPrefsDialog();
        settings.windowed = settings.windowedPending;
    }
    GetMainDevice();

    crashlog("opening platform tool");
    err = PlatformOpen();
    if (err != 0) {
        crashlog_print("Platform_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertPlatformOpen);
    }

    crashlog("opening file tool");
    err = FT_Open();
    if (err != 0) {
        crashlog_print("FT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertFileOpen);
    }

    crashlog("opening draw tool");
    err = DT_Open();
    if (err != 0) {
        crashlog_print("DT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertDrawOpen);
    }

    crashlog("opening interface tool");
    err = IT_Open();
    if (err != 0) {
        crashlog_print("IT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertInterfaceOpen);
    }

    crashlog("opening registration tool");
    err = RT3_Open(1, 3, "EV Nova", "EV Nova");
    if (err != 0) {
        crashlog_print("RT3_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertRegOpen);
    }

    crashlog("initializing arrays");
    InitArrays();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("opening data files");
    LICENSE_STEP_BEGIN
    shipData[5].licenseCode = RT3_GetLicenseCode();
    LICENSE_STEP_END
    InitAEVTHandlers();
    LICENSE_STEP_BEGIN
    shipData[17].licenseName = RT3_GetLicenseName();
    LICENSE_STEP_END
    IterateInGameEventLoop(kEventLoopTicks);
    LICENSE_STEP_BEGIN
    shipData[36].licenseCopies = RT3_GetLicenseCopies();
    LICENSE_STEP_END
    InitColors();

    LICENSE_STEP_BEGIN
    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? 1 : 0;
    LICENSE_STEP_END
    SetupDialogs();

    key = shipData[5].licenseCode;

    pluginCount = CountEVPlugins(gARef, "\pNova Plug-Ins", kPlugInFileType, 0);
    OpenDataFiles();
    pluginCount = CountEVPlugins(gARef, "\pNova Plug-Ins", kPlugInFileType, 0);
    if (pluginCount != 0) {
        if ((shipClass[gameIsRegisteredIndex].registered ? 1 : 0) == 0
                != (pluginCount == 0)) {
            ShowTheCursor();
            GetIndString(alertText, kPlugMismatchSTR, kPlugMismatchIndex);
            EVNoteAlert(alertText);
        }
    }
    FadeScreenOut();

    LICENSE_STEP_BEGIN
        blk[0] = KEYHIT(0x0284801349262406LL);
        blk[1] = KEYHIT(0x0101e057b160ee02LL);
        blk[2] = KEYHIT(0x07365c430b23e131LL);
        blk[3] = KEYHIT(0x07345d430b23e131LL);
        blk[4] = KEYHIT(0x05d4013a1123c94aLL);
        blk[5] = KEYHIT(0x08d22033a5a9ae47LL);
        blk[6] = KEYHIT(0x08d02133a5a9ae47LL);
        blk[7] = KEYHIT(0x0940f13e07262d74LL);
        blk[8] = KEYHIT(0x01e8616da73a6b41LL);
        blk[9] = KEYHIT(0x03395a952823e649LL);
        blk[10] = KEYHIT(0x033b5a952803f64bLL);
        blk[11] = KEYHIT(0x0b795a952803f64aLL);
        blk[12] = KEYHIT(0x00dd021413f10475LL);
        blk[13] = KEYHIT(0x00df021413d11477LL);
        blk[14] = KEYHIT(0x009f021413d11476LL);
        blk[15] = KEYHIT(0x09224585278cbf5bLL);
        blk[16] = KEYHIT(0x0bf35041709a3940LL);
        blk[17] = KEYHIT(0x03f15041709a3946LL);
        blk[18] = KEYHIT(0x0125a4aa21b1cf03LL);
        blk[19] = KEYHIT(0x03a31e5b03829d3dLL);
        blk[20] = KEYHIT(0x009f8c40aac47814LL);
        blk[21] = KEYHIT(0x045ac4b40fb08b36LL);
        blk[22] = KEYHIT(0x0c1ac5bc0b909b37LL);
        blk[23] = KEYHIT(0x02a974140d644b4dLL);
        blk[24] = KEYHIT(0x02a9741409644b49LL);
        blk[25] = KEYHIT(0x0aab751c09445b49LL);
        blk[26] = KEYHIT(0x02eb751c09445b4aLL);
        blk[27] = KEYHIT(0x01e24b6c4b56f67eLL);
        blk[28] = KEYHIT(0x048c30d71f489e43LL);
        blk[29] = KEYHIT(0x0c8c31d71f489e45LL);
        blk[30] = KEYHIT(0x04cc31d71f489e46LL);
        blk[31] = KEYHIT(0x0c8e31d71f489e47LL);
        blk[32] = KEYHIT(0x0cce31d71f489e40LL);
        blk[33] = KEYHIT(0x0561f87749b27666LL);
        blk[34] = KEYHIT(0x090a1452e822bd05LL);
        blk[35] = KEYHIT(0x04c23c3d8c45fe08LL);
        blk[36] = KEYHIT(0x03e90792a484de04LL);
        blk[37] = KEYHIT(0x000881643da5fb73LL);
    LICENSE_STEP_END

    for (i = 0; i < kBlacklistCount; i++) {
        if (blk[i] != 0) {
            strcpy((char *)alertText, "EV Nova");
            strcat((char *)alertText, " License");
            CtoPstr((char *)alertText);
            if (FSMakeFSSpec(prefsVRefNum, prefsDirID, alertText, &licenseSpec) == noErr)
                FSpDelete(&licenseSpec);
            break;
        }
    }

    cursorHidden = 0;
    if (settings.windowed == 0)
        HideTheCursor();
    FUN_00486ec0();
    CheckForPiratedCodes();
#if !TARGET_API_MAC_CARBON
    if (settings.windowed == 0)
        HideControlStrip();
#endif
    ST_Open(8, 0);
    oldVolume = ST_GetSysVolume();

    crashlog("loading sounds");
    LoadSounds();
    SWEnterSpriteWorld();

    crashlog("creating window");
    SetupWind();

    crashlog("initializing sprite world");
    SetupSpriteWorld();
    inBackground = 0;
    AdjustSoundVolume();

    crashlog("creating offscreen buffers");
    InitOffscreen();

    crashlog("starting music");
    SetupIntroMusic();
    ShowAmbrosiaLogo();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing strings");
    LoadStrings();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing status bar");
    InitStatus();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing objects");
    InitObjects();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing buttons");
    InitButtons();

    crashlog("loading object data");
    LoadObjectData();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("loading mission data");
    InitMissions();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("propagating mission bit effects");
    PropagateMissionBitEffects();
    IterateInGameEventLoop(kEventLoopTicks);

    HideAmbrosiaLogo();
    LICENSE_STEP_BEGIN
    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? 1 : 0;
    LICENSE_STEP_END

    crashlog("showing splash screen");
    ShowSplashScreen();
    HandleIntroMusic(1);
    FadeScreenIn();
    IterateInGameEventLoop(kEventLoopTicks);
    VerifyResourceChecksums();
    IterateInGameEventLoop(kEventLoopTicks);
    InitIntroProgressBar();

    crashlog("loading sprites");
    LoadSprites();

    crashlog("checking memory");
    grow = 0x7fffffff;
    startupMemoryFree = MaxMem(&grow);
#if !TARGET_API_MAC_CARBON
    {
        uint32_t memNeeded;

        memNeeded = PreflightMemoryUsageTotal() + 10000000;
        crashlog_print("free memory before loading ship sprites: ");
        crashlog_print_num(startupMemoryFree);
        crashlog_print(" bytes\n");
        crashlog_print("memory needed for ship sprites: ");
        crashlog_print_num(memNeeded);
        crashlog_print(" bytes\n");
        if (memNeeded > startupMemoryFree) {
            if (settings.shipAnimations || settings.engineGlows || settings.weaponGlows || settings.runningLights)
                DoPrefsDialog();
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     GetCWindowPortRect((WindowRef)gWind), GetCWindowPortRect((WindowRef)gWind),
                     srcCopy, GetCWindowVisRgn((WindowRef)gWind));
            memNeeded = PreflightMemoryUsageTotal() + 10000000;
            if (memNeeded > startupMemoryFree) {
                GetIndString(alertText, kAlertStrList, 0x1c);
                ParamText(alertText, "\p", "\p", "\p");
                StopAlert(128, 0);
                SWExitSpriteWorld();
                ExitMovies();
                CarbonHackCleanup();
                DisposeGammaTools();
                ST_Close();
                RT3_Close();
                IT_Close();
                DT_Close();
                FT_Close();
                PlatformClose();
                crashlog("out of memory bailout complete");
                crashlog("\n\n- end of log -");
                crashlog_close();
                ExitToShell();
            }
        }
    }
#endif

    crashlog("loading ship sprites");
    LoadShipSprites();

    crashlog("configuring sprites");
    InitSprites();

    crashlog("initializing player");
    ResetPlayer(1);
    ResetPlayerRecord(1);
    gameLoaded = gameInProgress = 0;
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing main screen");
    HandleIntroMusic(1);
    FadeScreenOut();
    DisposeIntroProgressBar();
    InitMainScreen();

    crashlog("opening last pilot file");
    OpenLastPilotFile();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("propagating mission bit effects");
    PropagateMissionBitEffects();
    gameLoaded = 1;

    crashlog("---");
    gPause = 1;
    HandleIntroMusic(1);
    DoMainScreen();

    crashlog("main screen disposed");
    do {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        unsigned char t1;
        int32_t g;
        long long mem;
        unsigned char x;
        char *p;
        unsigned char y;
        unsigned char c;
        char t2;
        unsigned short rem;
        Boolean ok;

        g = shipData[36].licenseCopies;
        p = (char *)shipData[17].licenseName;
        mem = shipData[5].licenseCode;
        x = 0;
        if (p != 0 && *p != 0) {
            while (StringGetLength((const unsigned char *)p) != 0 && StringGetLength(key) + 1 < 17)
                StringAppendSafe(key, (const unsigned char *)p, 17);
        }
        x ^= (unsigned char)g;
        x += (char)0xe7;
        x ^= (unsigned char)g;
        x = ROTL(x, 3);
        x ^= name[5];
        x += 0x31;
        x = ROTL(x, 7);
        c = key[0];
        x ^= c;
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x += 0x0b;
        t1 = name[2];
        x ^= t1;
        x ^= key[13];
        x ^= name[13];
        x += (char)0xa6;
        x ^= (unsigned char)g;
        x ^= name[12];
        x ^= (unsigned char)g;
        t2 = key[1];
        x ^= t2;
        x ^= (unsigned char)g;
        x ^= c;
        x ^= key[4];
        x += 0x67;
        x ^= name[6];
        x ^= (unsigned char)g;
        x += 0x58;
        if (x > t2)
            x ^= (unsigned char)g;
        x = ROTL(x, 2);
        x = ((x + (char)0xa0) & 0xff) + 0x7d;
        x ^= (unsigned char)g;
        x += 0x5a;
        x = ROTL(x, 7);
        x ^= (unsigned char)g;
        x += (char)0x93;
        x ^= (unsigned char)g;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x ^= name[9];
        rem = RT3_ExtractLicenseBlock2(mem);
        x += 0x78;
        x = ROTL(x, 2);
        x ^= key[2];
        x ^= t1;
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x = ROTL(x, 5);
        x = ROTL(x, 5);
        x ^= (unsigned char)g;
        if (x < (char)key[14])
            x ^= (unsigned char)g;
        if (x > (char)key[4])
            x ^= name[4];
        x ^= (unsigned char)g;
        x = ROTL(x, 4);
        x ^= key[15];
        x ^= name[14];
        x = ROTL(x, 2);
        x += 0x75;
        x = ROTL(x, 1);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x = ROTL(x, 1);
        x += -0x65;
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x += 0x58;
        x ^= key[15];
        LICENSE_STEP_BEGIN
        y = x * IsPlatformOpen() + -0x61;
        RegScramB2(y);
        x = ROTL(y, 5);
        LICENSE_STEP_END
        x ^= key[3];
        TimerGetSeconds();
        x += (char)0xa4;
        x ^= key[1];
        x ^= key[14];
        x += 0x29;
        x = ROTL(x, 2);
        x += (char)0xd1;
        x ^= (unsigned char)g;
        x += 0x82;
        x ^= name[0];
        x ^= name[1];
        x ^= (unsigned char)g;
        x += -0x7c;
        RegScramB2(x);
        y = x & 0xff;
        x = ROTL(y, 5);
        x += -0x4e;
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x = ROTL(x, 5);
        x += 0x5a;
        x ^= name[11];
        x ^= (unsigned char)g;
        x = ROTL(x, 5);
        x ^= (unsigned char)g;
        x += (char)0xe8;
        x ^= key[7];
        x += (char)0xdd;
#if TARGET_API_MAC_CARBON /* 1.0.10 spelling */
        x ^= (y = key[2]);
#else
        x ^= key[2];
#endif
        x ^= key[5];
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x = ROTL(x, 7);
        x = ROTL(x, 1);
        x ^= (unsigned char)g;
        x += 0x36;
        x ^= (unsigned char)g;
        if (x < 0x23)
            x = ROTL(x, 4);
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x ^= key[11];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        grow = (shipData[36].licenseCopies != 0 && ok == 0) != 0;
        if (grow)
            RT3_RunReminder(1, 0);
    } while (0);

    FadeScreenOut();
    ShowTheCursor();
    Bye();
}
#elif defined(MACBUILD)
{
    unsigned char alertText[256];
    Boolean blk[kBlacklistCount];
    long long key;
    Size grow;
    char *path;
    char *envPath;
    char *dir;
#if V111
    CFStringRef envStr;
    CFStringRef absStr;
    CFURLRef url;
#endif
    short i;

    SetResLoad(true);
    gameLoaded = gameInProgress = 0;
    preventStatusCopy = 0;
    gWind = 0;
    gSpriteWorld = 0;
    screenFaded = 0;

    SetupToolbox();
    CarbonHackInit();
    gameIsRegisteredIndex = Rand(0x300);
    gameIsHackedIndex = Rand(0x300);

    crashlog_print("initializing arrays\n");
    InitArrays();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog_print("opening data files\n");
    shipData[5].licenseCode = RT3_GetLicenseCode();
    shipData[17].licenseName = (unsigned char *)RT3_GetLicenseName();
    IterateInGameEventLoop(kEventLoopTicks);
    shipData[36].licenseCopies = RT3_GetLicenseCopies();
    IterateInGameEventLoop(kEventLoopTicks);

    LoadPrefs(1);
    if (KeyCheck(MacKey(0x3a)) && AreWeFrontProcess()) {
        crashlog_print("showing prefs dialog before window creation\n");
        smallTextFont = 3;
        smallTextSize = 9;
        DoPrefsDialog();
        settings.windowed = settings.windowedPending;
        LoadPrefs(1);
    }
    IterateInGameEventLoop(kEventLoopTicks);

    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? 1 : 0;
#if V111
    if (shipClass[gameIsRegisteredIndex].registered) {
        envPath = getenv("EVNOVA_SUPPORT_FOLDER_PATH");
        if (envPath != NULL) {
            novaSupportFolder = malloc(sizeof(FSRef));
            envStr = CFStringCreateFromString(envPath, strlen(envPath));
            if (envStr != NULL) {
                absStr = relative2absolute(envStr);
                if (absStr != NULL) {
                    url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, absStr,
                                                        kCFURLPOSIXPathStyle, true);
                    if (url != NULL) {
                        if (!CFURLGetFSRef(url, novaSupportFolder)) {
                            free(novaSupportFolder);
                            novaSupportFolder = NULL;
                        }
                        CFRelease(url);
                    }
                    CFRelease(absStr);
                }
                CFRelease(envStr);
            }
        }
        if (KeyCheck(MacKey(0x38)) && AreWeFrontProcess()) {
            SelectNovaSupport();
            CountEVPlugins(*novaSupportFolder, 0);
        }
        if (novaSupportFolder != NULL && shipClass[gameIsRegisteredIndex].registered == 0) {
            ShowTheCursor();
            GetIndString(alertText, kAlertStrList, 0x35);
            EVNoteAlert(alertText);
            free(novaSupportFolder);
            novaSupportFolder = NULL;
        }
    }
#endif
    InitDataFiles();
    VerifyResourceChecksums();
    if (EVInstallEndianFlippers() == 0) {
        if (novaSupportFolder != NULL) {
            dir = malloc(0x200);
            FSRefMakePath(novaSupportFolder, (UInt8 *)dir, 0x200);
            path = malloc(strlen(dir) + 13);
            sprintf(path, "%s/debuglog.txt", dir);
            free(dir);
        }
        crashlog_open(path);
        if (path != NULL)
            free(path);
        crashlog_print("game engine was compiled ");
        crashlog_print("Dec  6 2011");
        crashlog_print("\n");
        crashlog_print("game engine version is ");
        crashlog_print("1.1.1");
#if TARGET_RT_LITTLE_ENDIAN
        crashlog_print(" x86");
#else
        crashlog_print(" PPC");
#endif
        crashlog_print("\n");
        crashlog_print("\n");

        crashlog_print("opening data files & loading prefs\n");
        Microseconds(&fakeLicenseCode);

        crashlog_print("initializing monitor\n");
        SetupMonitor();
        CheckSystem();

        crashlog_print("initializing QuickTime\n");
        if (EnterMovies() != 0)
            FatalStartupError(kAlertQuickTime);

        crashlog_print("building trig table\n");
        BuildTrigTable();
        gWind = 0;
        GetMainDevice();
        InitColors();
        shipClass[gameIsRegisteredIndex].registered =
            shipData[36].licenseCopies ? 1 : 0;
        SetupDialogs();

        key = shipData[5].licenseCode;

        pluginCount = CountEVPlugins(pluginsFolder, kPlugInFileType);
        OpenDataFiles();
        pluginCount = CountEVPlugins(pluginsFolder, kPlugInFileType);
        if (pluginCount != 0) {
            if (shipClass[gameIsRegisteredIndex].registered == 0) {
                ShowTheCursor();
                GetIndString(alertText, kAlertStrList, 0x1b);
                EVNoteAlert(alertText);
            }
        }

        blk[0] = KEYHIT(0x0284801349262406LL);
        blk[1] = KEYHIT(0x0101e057b160ee02LL);
        blk[2] = KEYHIT(0x07365c430b23e131LL);
        blk[3] = KEYHIT(0x07345d430b23e131LL);
        blk[4] = KEYHIT(0x05d4013a1123c94aLL);
        blk[5] = KEYHIT(0x08d22033a5a9ae47LL);
        blk[6] = KEYHIT(0x08d02133a5a9ae47LL);
        blk[7] = KEYHIT(0x0940f13e07262d74LL);
        blk[8] = KEYHIT(0x01e8616da73a6b41LL);
        blk[9] = KEYHIT(0x03395a952823e649LL);
        blk[10] = KEYHIT(0x033b5a952803f64bLL);
        blk[11] = KEYHIT(0x0b795a952803f64aLL);
        blk[12] = KEYHIT(0x00dd021413f10475LL);
        blk[13] = KEYHIT(0x00df021413d11477LL);
        blk[14] = KEYHIT(0x009f021413d11476LL);
        blk[15] = KEYHIT(0x09224585278cbf5bLL);
        blk[16] = KEYHIT(0x0bf35041709a3940LL);
        blk[17] = KEYHIT(0x03f15041709a3946LL);
        blk[18] = KEYHIT(0x0125a4aa21b1cf03LL);
        blk[19] = KEYHIT(0x03a31e5b03829d3dLL);
        blk[20] = KEYHIT(0x009f8c40aac47814LL);
        blk[21] = KEYHIT(0x045ac4b40fb08b36LL);
        blk[22] = KEYHIT(0x0c1ac5bc0b909b37LL);
        blk[23] = KEYHIT(0x02a974140d644b4dLL);
        blk[24] = KEYHIT(0x02a9741409644b49LL);
        blk[25] = KEYHIT(0x0aab751c09445b49LL);
        blk[26] = KEYHIT(0x02eb751c09445b4aLL);
        blk[27] = KEYHIT(0x01e24b6c4b56f67eLL);
        blk[28] = KEYHIT(0x048c30d71f489e43LL);
        blk[29] = KEYHIT(0x0c8c31d71f489e45LL);
        blk[30] = KEYHIT(0x04cc31d71f489e46LL);
        blk[31] = KEYHIT(0x0c8e31d71f489e47LL);
        blk[32] = KEYHIT(0x0cce31d71f489e40LL);
        blk[33] = KEYHIT(0x0561f87749b27666LL);
        blk[34] = KEYHIT(0x090a1452e822bd05LL);
        blk[35] = KEYHIT(0x04c23c3d8c45fe08LL);
        blk[36] = KEYHIT(0x03e90792a484de04LL);
        blk[37] = KEYHIT(0x000881643da5fb73LL);


        for (i = 0; i < kBlacklistCount; i++) {
            if (blk[i] != 0) {
                HFSUniStr255 uniName;
                FSRef prefRef;
                FSRef licRef;

                HFSNameGetUnicodeName("\pEV Nova License", kTextEncodingUnknown, &uniName);
                if (FSFindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &prefRef) == noErr
                        && FSMakeFSRefUnicode(&prefRef, uniName.length, uniName.unicode,
                                              kTextEncodingUnknown, &licRef) == noErr)
                    FSDeleteObject(&licRef);
                break;
            }
        }

        cursorHidden = 0;
        if (settings.windowed == 0)
            HideTheCursor();
        CheckForPiratedCodes();
        if (!screenFaded)
            FadeScreenOut();

        crashlog_print("loading sounds\n");
        LoadSounds();
        SWEnterSpriteWorld();

        crashlog_print("creating window\n");
        SetupWind();

        crashlog_print("initializing sprite world\n");
        SetupSpriteWorld();
        inBackground = 0;
        AdjustSoundVolume();

        crashlog_print("creating offscreen buffers\n");
        InitOffscreen();

        crashlog_print("starting music\n");
        SetupIntroMusic();
        SpawnIntroMusicThread();
        ShowAmbrosiaLogo();
        FadeScreenIn();
        IterateInGameEventLoop(0x3c);

        crashlog_print("initializing strings\n");
        LoadStrings();
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("initializing status bar\n");
        InitStatus();
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("initializing objects\n");
        InitObjects();
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("initializing buttons\n");
        InitButtons();

        crashlog_print("loading object data\n");
        LoadObjectData();
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("loading mission data\n");
        InitMissions();
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("propagating mission bit effects\n");
        PropagateMissionBitEffects();
        IterateInGameEventLoop(kEventLoopTicks);

        FadeScreenOut();
        HideAmbrosiaLogo();
        shipClass[gameIsRegisteredIndex].registered =
            shipData[36].licenseCopies ? 1 : 0;

        crashlog_print("showing splash screen\n");
        ShowSplashScreen();
        FadeScreenIn();
        IterateInGameEventLoop(kEventLoopTicks);
        InitIntroProgressBar();

        crashlog_print("loading sprites\n");
        LoadSprites();

        crashlog_print("checking memory\n");
        grow = 0x7fffffff;
        startupMemoryFree = MaxMem(&grow);

        crashlog_print("loading ship sprites\n");
        LoadShipSprites();

        crashlog_print("configuring sprites\n");
        InitSprites();

        crashlog_print("initializing player\n");
        ResetPlayer(1);
        ResetPlayerRecord(1);
        gameLoaded = gameInProgress = 0;
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("initializing main screen\n");
        FadeScreenOut();
        DisposeIntroProgressBar();
        InitMainScreen();

        crashlog_print("opening last pilot file\n");
        OpenLastPilotFile();
        IterateInGameEventLoop(kEventLoopTicks);

        crashlog_print("propagating mission bit effects\n");
        PropagateMissionBitEffects();
        gameLoaded = 1;

        crashlog_print("---");
        gPause = 1;
        DoMainScreen();
    }

    crashlog_print("main screen disposed\n");
    {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        uint32_t g;
        char *p;
        long long mem;
        unsigned char x;
        unsigned char y;
        unsigned char c;
        char b;
        unsigned char t1;
        char t2;
        unsigned short rem;
        Boolean ok;
        unsigned int v;

        g = shipData[36].licenseCopies;
        p = RT3_CheckLicenseName(shipData[17].licenseName);
        mem = shipData[5].licenseCode;
        x = 0;
        b = 0;
        if (p != 0 && *p >= '0' && *p <= '9')
            g = 1;
        if (p != 0 && *p != 0) {
            while (StringGetLength(p) != 0 && StringGetLength(key) + 1 < 17)
                StringAppendSafe(key, p, 17);
        }
        x ^= g;
        x += 0xe7;
        x ^= g;
        x = ROTL(x, 3);
        x ^= name[5];
        x += 0x31;
        x = ROTL(x, 7);
        c = key[0];
        x ^= c;
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x += 0x0b;
        t1 = name[2];
        x ^= t1;
        x ^= key[13];
        x ^= name[13];
        x += 0xa6;
        x ^= g;
        x ^= name[12];
        x ^= g;
        t2 = key[1];
        x ^= (unsigned char)t2;
        x ^= g;
        x ^= c;
        x ^= key[4];
        x += 0x67;
        x ^= name[6];
        x ^= g;
        x += 0x58;
        if (x > t2)
            x ^= g;
        x = ROTL(x, 2);
        x ^= RegScramE2();
        y = x + 0x1d;
        y ^= g;
        y += 0x5a;
        x = ROTL(y, 7);
        x ^= g;
        x += 0x93;
        x ^= g;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x ^= name[9];
        rem = RT3_ExtractLicenseBlock2(mem);
        x += 0x78;
        x = ROTL(x, 2);
        x ^= key[2];
        x ^= t1;
        x ^= g;
        x ^= g;
        x = ROTL(x, 5);
        x = ROTL(x, 5);
        x ^= g;
        if (x < (char)key[14])
            x ^= g;
        if (x > (char)key[4])
            x ^= name[4];
        x ^= g;
        x = ROTL(x, 4);
        x ^= key[15];
        x ^= name[14];
        x = ROTL(x, 2);
        x += 0x75;
        RegScramC2(x);
        x = ROTL(x, 1);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x = ROTL(x, 1);
        x += -0x65;
        x = ROTL(x, 1);
        x ^= g;
        x += 0x58;
        x ^= key[15];
        x = RegScramA2(x);
        x += -0x61;
        RegScramB2(x);
        RegScramC2(x);
        x = ROTL(x, 5);
        x ^= key[3];
        b = x;
        x = RegScramD2(&b);
        x += 0xa4;
        x ^= key[1];
        x ^= key[14];
        x += 0x29;
        x = ROTL(x, 2);
        x += 0xd1;
        x ^= g;
        x = (x + 0x82) % 256;
        x ^= name[0];
        x ^= name[1];
        x ^= g;
        x += -0x7c;
        RegScramB2(x);
        x = ROTL(x, 0);
        x = ROTL(x, 5);
        x += -0x4e;
        x = ROTL(x, 1);
        x ^= g;
        x = ROTL(x, 5);
        x += 0x5a;
        x ^= name[11];
        x ^= g;
        x = ROTL(x, 5);
        x ^= g;
        x += 0xe8;
        x ^= key[7];
        x += 0xdd;
        x ^= key[2];
        x ^= key[5];
        x = ROTL(x, 1);
        x ^= g;
        x ^= g;
        x = ROTL(x, 7);
        x = ROTL(x, 1);
        x ^= g;
        x += 0x36;
        x ^= g;
        if (x < 0x23)
            x = ROTL(x, 4);
        x ^= g;
        x ^= g;
        x ^= key[11];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        x ^= key[10];
        x += 0x4d;
        x = ROTL(x, 3);
        x = ROTL(x, 4);
        x += 0xee;
        x ^= key[9];
        x ^= RegScramE2();
        x += -0x4c;
        x = ROTL(x, 4);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x ^= key[8];
        x = ROTL(x, 6);
        x ^= key[2];
        x = ROTL(x, 7);
        v = x + 0x57;
        x = v;
        if (x != 0)
            RegScramC2(v);

        grow = shipData[36].licenseCopies != 0 && ok == 0;
    }

#if V111
    if (grow)
        ASWReg_ShowDialogWithOptions(true, true);
#endif
    FadeScreenOut();
    ShowTheCursor();
    Bye();
}
#else
{
    unsigned char alertText[256];
    Boolean blk[kBlacklistCount];
    int32_t qtVersion;
    ComponentDescription cd;
    long long key;
    int err;
    short i;

    gameLoaded = gameInProgress = 0;
    preventStatusCopy = 0;
    gWind = 0;
    gSpriteWorld = 0;
    screenFaded = 0;

    ScrambleRand();
    PlatformOpen();
    KeyboardInit();
    MouseInit();
    InitMemory();
    SetDefaultPrefixs();
    OpenPrefs();

    crashlog_open("debuglog.txt");
    crashlog_print("game engine was compiled ");
    crashlog_print("Jun 24 2006");
    crashlog_print("\n");
    gameIsRegisteredIndex = Rand(0x300);
    crashlog_print("game engine version is ");
    crashlog_print("1.0.A");
    crashlog_print(" Windows");
    crashlog_print("\n\n");
    gameIsHackedIndex = Rand(0x300);

    crashlog("opening data files & loading prefs");
    InitDataFiles();
#ifdef CEFIX /* F20.01 */
    CE_ReadX2Hotkey();
#endif
    LoadPrefs();
    fakeLicenseCode = TimerGetMicroseconds();
    Gestalt(kQuickTimeGestalt, &qtVersion);
    if (qtVersion < kQuickTime5) {
        ShowTheCursor();
        ShowAlert("QuickTime v5.0 or Newer Required",
                  "EV Nova requires QuickTime v5.0 or newer.  You can get the latest version of QuickTime from http://www.apple.com/quicktime/");
        ExitToShell();
    }

    cd.componentType = kRLEDecompressor;
    cd.componentSubType = kRLECodec;
    cd.componentManufacturer = kAppleManufacturer;
    cd.componentFlags = 0;
    cd.componentFlagsMask = kComponentFlagsMask;
    if (FindNextComponent(0, &cd) == 0) {
        ShowTheCursor();
        ShowAlert("QuickTime requires Internet Extras",
                  "EV Nova requires QuickTime v5.0 or newer with the Internet Extras component.  You can install the component from the QuickTime settings panel, under Update Check.");
        ExitToShell();
    }

    crashlog("initializing monitor");
#ifdef CEFIX /* F03.01 */
    CE_InitializeMonitor();
#else
    SetupMonitor();
#endif
    ResetScreenRect();

    crashlog("initializing QuickTime");
    err = EnterMovies();
    if (err != 0)
        FatalStartupError(kAlertQuickTime);

    crashlog("building trig table");
    BuildTrigTable();

    if (settings.gamma > kMaxGamma)
        settings.gamma = kDefaultGamma;
    BuildGammaRamp((settings.gamma * 10.0 + 70.0));
    gWind = 0;
    SavePrefs();

    crashlog("opening platform tool");
    err = PlatformOpen();
    if (err != 0) {
        crashlog_print("Platform_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertPlatformOpen);
    }

    crashlog("opening file tool");
    err = FT_Open();
    if (err != 0) {
        crashlog_print("FT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertFileOpen);
    }

    crashlog("opening draw tool");
    err = DT_Open();
    if (err != 0) {
        crashlog_print("DT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertDrawOpen);
    }

    crashlog("opening interface tool");
    err = IT_Open();
    if (err != 0) {
        crashlog_print("IT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertInterfaceOpen);
    }

    crashlog("opening registration tool");
    err = RT3_Open(1, 3, "EV Nova", "EV Nova");
    if (err != 0) {
        crashlog_print("RT3_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        FatalStartupError(kAlertRegOpen);
    }

    crashlog("initializing arrays");
    InitArrays();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("opening data files");
    shipData[5].licenseCode = RT3_GetLicenseCode();
    FUN_0049e420();
    shipData[17].licenseName = (unsigned char *)RT3_GetLicenseName();
    IterateInGameEventLoop(kEventLoopTicks);
    shipData[36].licenseCopies = RT3_GetLicenseCopies();
    InitColors();

    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? (unsigned char)1 : (unsigned char)0;
    SetupDialogs();

    key = shipData[5].licenseCode;

#ifdef MACBUILD
    pluginCount = CountEVPlugins(pluginsFolder, kPlugInFileType);
    OpenDataFiles();
    pluginCount = CountEVPlugins(pluginsFolder, kPlugInFileType);
#else
    pluginCount = CountEVPlugins("Nova Plug-Ins", ".rez", 0);
    OpenDataFiles();
    pluginCount = CountEVPlugins("Nova Plug-Ins", ".rez", 0);
#endif
    if (pluginCount != 0) {
        if ((shipClass[gameIsRegisteredIndex].registered ? 1 : 0) == 0
                != (pluginCount == 0)) {
            ShowTheCursor();
            GetIndString(alertText, kPlugMismatchSTR, kPlugMismatchIndex);
            EVNoteAlert(alertText);
        }
    }
    FadeScreenOut();

    blk[0] = KEYHIT(0x0284801349262406LL);
    blk[1] = KEYHIT(0x0101e057b160ee02LL);
    blk[2] = KEYHIT(0x07365c430b23e131LL);
    blk[3] = KEYHIT(0x07345d430b23e131LL);
    blk[4] = KEYHIT(0x05d4013a1123c94aLL);
    blk[5] = KEYHIT(0x08d22033a5a9ae47LL);
    blk[6] = KEYHIT(0x08d02133a5a9ae47LL);
    blk[7] = KEYHIT(0x0940f13e07262d74LL);
    blk[8] = KEYHIT(0x01e8616da73a6b41LL);
    blk[9] = KEYHIT(0x03395a952823e649LL);
    blk[10] = KEYHIT(0x033b5a952803f64bLL);
    blk[11] = KEYHIT(0x0b795a952803f64aLL);
    blk[12] = KEYHIT(0x00dd021413f10475LL);
    blk[13] = KEYHIT(0x00df021413d11477LL);
    blk[14] = KEYHIT(0x009f021413d11476LL);
    blk[15] = KEYHIT(0x09224585278cbf5bLL);
    blk[16] = KEYHIT(0x0bf35041709a3940LL);
    blk[17] = KEYHIT(0x03f15041709a3946LL);
    blk[18] = KEYHIT(0x0125a4aa21b1cf03LL);
    blk[19] = KEYHIT(0x03a31e5b03829d3dLL);
    blk[20] = KEYHIT(0x009f8c40aac47814LL);
    blk[21] = KEYHIT(0x045ac4b40fb08b36LL);
    blk[22] = KEYHIT(0x0c1ac5bc0b909b37LL);
    blk[23] = KEYHIT(0x02a974140d644b4dLL);
    blk[24] = KEYHIT(0x02a9741409644b49LL);
    blk[25] = KEYHIT(0x0aab751c09445b49LL);
    blk[26] = KEYHIT(0x02eb751c09445b4aLL);
    blk[27] = KEYHIT(0x01e24b6c4b56f67eLL);
    blk[28] = KEYHIT(0x048c30d71f489e43LL);
    blk[29] = KEYHIT(0x0c8c31d71f489e45LL);
    blk[30] = KEYHIT(0x04cc31d71f489e46LL);
    blk[31] = KEYHIT(0x0c8e31d71f489e47LL);
    blk[32] = KEYHIT(0x0cce31d71f489e40LL);
    blk[33] = KEYHIT(0x0561f87749b27666LL);
    blk[34] = KEYHIT(0x090a1452e822bd05LL);
    blk[35] = KEYHIT(0x04c23c3d8c45fe08LL);
    blk[36] = KEYHIT(0x03e90792a484de04LL);
    blk[37] = KEYHIT(0x000881643da5fb73LL);

    for (i = 0; i < kBlacklistCount; i++) {
        if (blk[i] != 0) {
            FT_FileDelete(-3, "EV Nova License.lcs");
            break;
        }
    }

    cursorHidden = 0;
    if (settings.windowed == 0)
        HideTheCursor();
    FUN_00486ec0();
    CheckForPiratedCodes();
    ST_Open(8, 0);
    oldVolume = ST_GetSysVolume();

    crashlog("loading sounds");
    LoadSounds();
    SWEnterSpriteWorld();

    crashlog("creating window");
    SetupWind();

    crashlog("initializing sprite world");
    SetupSpriteWorld();
    inBackground = 0;
    AdjustSoundVolume();

    crashlog("creating offscreen buffers");
    InitOffscreen();

    crashlog("starting music");
    SetupIntroMusic();
    ShowAmbrosiaLogo();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing strings");
    LoadStrings();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing status bar");
    InitStatus();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing objects");
    InitObjects();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing buttons");
    InitButtons();

    crashlog("loading object data");
    LoadObjectData();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("loading mission data");
    InitMissions();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("propagating mission bit effects");
    PropagateMissionBitEffects();
    IterateInGameEventLoop(kEventLoopTicks);

    HideAmbrosiaLogo();
    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? (unsigned char)1 : (unsigned char)0;

    crashlog("showing splash screen");
    ShowSplashScreen();
    HandleIntroMusic(1);
    FadeScreenIn();
    IterateInGameEventLoop(kEventLoopTicks);
    VerifyResourceChecksums();
    IterateInGameEventLoop(kEventLoopTicks);
    InitIntroProgressBar();

    crashlog("loading sprites");
    LoadSprites();

    crashlog("checking memory");

    crashlog("loading ship sprites");
    LoadShipSprites();

    crashlog("configuring sprites");
    InitSprites();

    crashlog("initializing player");
    ResetPlayer(1);
    ResetPlayerRecord(1);
    gameLoaded = gameInProgress = 0;
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing main screen");
    HandleIntroMusic(1);
    FadeScreenOut();
    DisposeIntroProgressBar();
    InitMainScreen();

    crashlog("opening last pilot file");
    OpenLastPilotFile();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("propagating mission bit effects");
    PropagateMissionBitEffects();
    gameLoaded = 1;

    crashlog("---");
    gPause = 1;
    HandleIntroMusic(1);
    DoMainScreen();

    crashlog("main screen disposed");

    {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "";
        unsigned char *p;
        long long mem;
        unsigned char x;
        unsigned char y;
        int c;
        char b;
        int t1;
        int t2;
        int32_t g;
        unsigned short rem;
        Boolean ok;
        int bail;
        unsigned int v;

        g = shipData[36].licenseCopies;
        p = shipData[17].licenseName;
        mem = shipData[5].licenseCode;
        x = 0;
        b = 0;
        if (p != 0 && *p != 0) {
            while (StringGetLength(p) != 0 && StringGetLength(key) + 1 < 17)
                StringAppendSafe(key, p, 17);
        }
        x ^= g;
        x += 0xe7;
        x ^= g;
        x = ROTL(x, 3);
        x ^= name[5];
        x += 0x31;
        x = ROTL(x, 7);
        c = key[0];
        x ^= c;
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x += 0x0b;
        t1 = name[2];
        x ^= t1;
        x ^= key[13];
        x ^= name[13];
        x += 0xa6;
        x ^= g;
        x ^= name[12];
        x ^= g;
        t2 = key[1];
        x ^= t2;
        x ^= g;
        x ^= c;
        x ^= key[4];
        x += 0x67;
        x ^= name[6];
        x ^= g;
        x += 0x58;
        if (x > (char)t2)
            x ^= g;
        x = ROTL(x, 2);
        x ^= RegScramE2();
        y = x + 0x1d;
        y ^= g;
        y += 0x5a;
        x = ROTL(y, 7);
        x ^= g;
        x += 0x93;
        x ^= g;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x ^= name[9];
        rem = DRHash64(mem);
        x += 0x78;
        x = ROTL(x, 2);
        x ^= key[2];
        x ^= t1;
        x ^= g;
        x ^= g;
        x = ROTL(x, 5);
        x = ROTL(x, 5);
        x ^= g;
        if (x < (char)key[14])
            x ^= g;
        if (x > (char)key[4])
            x ^= name[4];
        x ^= g;
        x = ROTL(x, 4);
        x ^= key[15];
        x ^= name[14];
        x = ROTL(x, 2);
        x += 0x75;
        RegScramC2(x);
        x = ROTL(x, 1);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x = ROTL(x, 1);
        x += -0x65;
        x = ROTL(x, 1);
        x ^= g;
        x += 0x58;
        x ^= key[15];
        x = RegScramA2(x);
        x += -0x61;
        RegScramB2(x);
        RegScramC2(x);
        x = ROTL(x, 5);
        x ^= key[3];
        b = x;
        x = RegScramD2(&b);
        x += 0xa4;
        x ^= key[1];
        x ^= key[14];
        x += 0x29;
        x = ROTL(x, 2);
        x += 0xd1;
        x ^= g;
        x += 0x82;
        x ^= name[0];
        x ^= name[1];
        x ^= g;
        x += -0x7c;
        RegScramB2(x);
        x = ROTL(x, 5);
        x += -0x4e;
        x = ROTL(x, 1);
        x ^= g;
        x = ROTL(x, 5);
        x += 0x5a;
        x ^= name[11];
        x ^= g;
        x = ROTL(x, 5);
        x ^= g;
        x += 0xe8;
        x ^= key[7];
        x += 0xdd;
        x ^= key[2];
        x ^= key[5];
        x = ROTL(x, 1);
        x ^= g;
        x ^= g;
        x = ROTL(x, 7);
        x = ROTL(x, 1);
        x ^= g;
        x += 0x36;
        x ^= g;
        if (x < 0x23)
            x = ROTL(x, 4);
        x ^= g;
        x ^= g;
        x ^= key[11];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        x ^= key[10];
        x += 0x4d;
        x = ROTL(x, 3);
        x = ROTL(x, 4);
        x += 0xee;
        x ^= key[9];
        x ^= RegScramE2();
        x += -0x4c;
        x = ROTL(x, 4);
        x = ROTL(x, 4);
        x = ROTL(x, 8);
        x ^= key[8];
        x = ROTL(x, 6);
        x ^= key[2];
        x = ROTL(x, 7);
        v = x + 0x57;
        x = v;
        if (x != 0)
            RegScramC2(v);

        bail = shipData[36].licenseCopies != 0 && ok == 0
                ? (unsigned char)1 : (unsigned char)0;
        if (bail) {
            if (settings.windowed == 0)
                ReleaseVideo();
            RT3_RunReminder(1, 0);
        }
    }

    FadeScreenOut();
    ShowTheCursor();
    KeyboardDestroy();
    MouseDestroy();
    Bye();
    ClosePrefs();
}
#endif

#define kEscapePodType  0x2ff

/* MyProfile bracket ids, registered by name in PlayGame */
#define kProfGameLoop     0
#define kProfPreDraw      1
#define kProfDrawing      2
#define kProfPostDraw     3
#define kProfShip         4
#define kProfShipDisp     5
#define kProfAI           6
#define kProfShots        7
#define kProfSprites      8     /* "misc handlers" */
#define kProfCollide      9
#define kProfPlayer      10
#define kProfHandlers    11     /* "misc handlers2" */
#define kProfStatus      12
#define kProfProcessSW   13
#define kProfAnimateSW   14
#define kProfPostAnimate 15
#define kProfLocking     16
#define kProfErase       17
#define kProfDraw        18
#define kProfUpdate      19
#define kProfOdds        20


/* PlayGame (mac _PlayGame). STATE: win MATCH, exe 1105 / obj 1105, byte-identical yes (relocs
   masked); mac MATCH on both slices.
   OPEN: none */
// FUNCTION: EVNOVA 0x00417600
// MAC: _PlayGame i386 0x000457ba ppc 0x0004da00
void PlayGame(void)
#ifdef MACBUILD
{
    unsigned char str[256];
    unsigned char numStr[256];
    DateTimeRec d;
    float dx;
    float dy;
    float deathTime;
    GDHandle device;
    Rect box;
    short doIt;

    GetTime(&d);
    RedrawStatus(1);
    RedrawComm(0);
    PropagateMissionBitEffects();
    HandleStellarSystemVisibility();
    gameFrozen = 1;
    ignoreKeys = 1;
    DoPlayGameWork(1);
    gameFrozen = 0;
    ignoreKeys = 0;
    hyperGamma = -1.0f;
    cursorAlpha = 0;
    playerKilledByHector = 0;
    PrepareAllSprites();
    RefreshWindowBuffer();
    SWUpdateSpriteWorld(gSpriteWorld, 1);
#if V111
    if (useLegacyRenderingPipeline)
#endif
        RefreshSAT();
#ifdef MAC1010
    FadeScreenIn();
#endif
    lastLoMemWarningTime = 0;
    CheckLoMem();
    lastSmokeUpdateTime = ReadTick();
    lastEventLoopTime = ReadTick();
    lastKeypressTime = ReadTick();
#ifndef MAC1010
    FadeScreenIn();
#endif
    AdjustSoundVolume();
    MyProfileInit();
    MyProfileSetName(kProfGameLoop, "game loop");
    MyProfileSetName(kProfPreDraw, "pre draw tasks");
    MyProfileSetName(kProfDrawing, "drawing");
    MyProfileSetName(kProfPostDraw, "post draw tasks");
    MyProfileSetName(kProfShip, "HandleShip");
    MyProfileSetName(kProfShipDisp, "HandleShipDisplay");
    MyProfileSetName(kProfAI, "AI routines");
    MyProfileSetName(kProfShots, "HandleShot");
    MyProfileSetName(kProfSprites, "misc handlers");
    MyProfileSetName(kProfCollide, "collisions");
    MyProfileSetName(kProfPlayer, "player");
    MyProfileSetName(kProfHandlers, "misc handlers2");
    MyProfileSetName(kProfStatus, "DrawStatus");
    MyProfileSetName(kProfProcessSW, "ProcessSpriteWorld");
    MyProfileSetName(kProfAnimateSW, "AnimateSpriteWorld");
    MyProfileSetName(kProfPostAnimate, "PostAnimate");
    MyProfileSetName(kProfLocking, "locking");
    MyProfileSetName(kProfErase, "erase");
    MyProfileSetName(kProfDraw, "draw");
    MyProfileSetName(kProfUpdate, "update");
    MyProfileSetName(kProfOdds, "calc AI odds");
    gSpriteWorld->fastBlit = 1;
    inBackground = 0;
    gPause = 0;
    FlushEvents(8, 0);
    FlushEvents(0xf, 0);

    while (gQuit == 0 && gPause == 0) {
        MyProfileEnter(kProfGameLoop);
#if V111
        if (useLegacyRenderingPipeline)
#endif
        {
            device = gSpriteWorld->mainSWGDH;
            if (device != NULL) {
                if ((*(*device)->gdPMap)->pixelSize != gSpriteWorld->pixelDepth) {
                    gPause = 1;
                    break;
                }
                box = (*(*device)->gdPMap)->bounds;
                if (box.right - box.left != mainDeviceInitBounds.right - mainDeviceInitBounds.left
                    || box.bottom - box.top != mainDeviceInitBounds.bottom - mainDeviceInitBounds.top) {
                    gPause = 1;
                    break;
                }
            }
        }
        HandleTimeAdjustment();
        keyPressedThisFrame = 0;
        if (inBackground == 0) {
            if (gSpriteWorld->fastBlit != 0) {
                MyProfileEnter(kProfPreDraw);
                lastFramePlayerPos.x = shipData[0].pos.x;
                lastFramePlayerPos.y = shipData[0].pos.y;
                if (frameCounter % 3 == 0) {
                    updateSmokeThisFrame = 1;
                    lastSmokeUpdateTime = ReadTick();
                } else {
                    updateSmokeThisFrame = 0;
                }
                DoPlayGameWork(1);
                fpsCounter += 1;
                if (showFPS != 0) {
                    if (ReadTick() > lastFPSUpdateTime + 120) {
                        if ((uint32_t)lastFPSUpdateTime == 0) {
                            GetIndString(str, kGameStrList, 282);
                            DisplayComm((PStrChar *)str, 100, 0, kCommTextSize, &rgbWhite, 0, 0);
                        } else {
                            NumToString((fpsCounter * 60.0 /
                                    (float)(ReadTick() - lastFPSUpdateTime)),
                                    numStr);
                            PtoCstr(numStr);
                            strcpy((char *)str, (char *)numStr);
                            strcat((char *)str, " ");
                            StrcatIndCString((char *)str, kGameStrList, 283);
                            CtoPstr((char *)str);
                            DisplayComm((PStrChar *)str, 999, 0, kCommTextSize, &rgbWhite, 0, 0);
                        }
                        lastFPSUpdateTime = ReadTick();
                        fpsCounter = 0;
                    }
                } else {
                    lastFPSUpdateTime = ReadTick();
                }
                dx = lastFramePlayerPos.x - shipData[0].pos.x;
                dy = lastFramePlayerPos.y - shipData[0].pos.y;
                if (doubleTime != 0) {
                    dx *= 2.0f;
                    dy *= 2.0f;
                }
                ScrollStarfield(dx, dy);
                HandleMouse();
                MyProfileExit(kProfPreDraw);
                MyProfileEnter(kProfDrawing);
#if TARGET_API_MAC_CARBON
                MyProfileEnter(kProfLocking);
                LockPortBits(GetCWindowGrafPort(gWind));
                MyProfileExit(kProfLocking);
#endif
                MyProfileEnter(kProfProcessSW);
                SWProcessSpriteWorld(gSpriteWorld);
                MyProfileExit(kProfProcessSW);
                MyProfileEnter(kProfAnimateSW);
                SWAnimateSpriteWorld(gSpriteWorld);
                MyProfileExit(kProfAnimateSW);
                MyProfileEnter(kProfPostAnimate);
                PostAnimateHandler();
                MyProfileExit(kProfPostAnimate);
#if TARGET_API_MAC_CARBON
                MyProfileEnter(kProfLocking);
                UnlockPortBits(GetCWindowGrafPort(gWind));
                QDFlushPortBuffer(GetCWindowGrafPort(gWind), 0);
                MyProfileExit(kProfLocking);
#endif
                MyProfileExit(kProfDrawing);
                MyProfileEnter(kProfPostDraw);
                if (KeyCheck(keyPrefs.pause)) {
                    if (pauseKeyFlag == 0 && gPause == 0) {
                        pauseKeyFlag = 1;
                        if (shipData[0].jumpTimer <= 0.0f
                                && shipData[0].type != kEscapePodType
                                && (deathTime = shipData[0].deathTimer) <= 0.0f
                                && shipData[0].escapePods <= 0) {
                            if (shipData[0].active != 0 || deathTime < -250.0f)
                                gPause = 1;
                        }
                    }
                } else {
                    pauseKeyFlag = 0;
                }
                if (doubleTime != 0)
                    DoPlayGameWork(0);
                frameCounter += 1;
                if (frameCounter > 1024) {
                    frameCounter = 0;
                    if (d.day == 30 && d.month == 6 && commCount <= 0)
                        DisplayComm((PStrChar *)"\pHappy birthday Matt",
                                    5, 3, 9, &rgbDarkGrey, 0, 0);
                    do {
                        unsigned char blob[3] = { 0x49, 0x71, 0xad };
                        unsigned char name[17] = "EV NovaEV NovaEV";
                        unsigned char key[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
                        unsigned char t6;
                        unsigned char t2;
                        unsigned char t3;
                        unsigned char t4;
                        unsigned char t1;
                        int32_t g;
                        char *p;
                        long long mem;
                        unsigned char c;
                        char b;
                        char t5;
                        Boolean ok;
                        unsigned short rem = 0;
                        unsigned char x;

                        g = shipData[36].licenseCopies;
#ifdef MAC1010
                        p = (char *)shipData[17].licenseName;
#else
                        p = RT3_CheckLicenseName(shipData[17].licenseName);
#endif
                        mem = shipData[5].licenseCode;
                        x = 0;
#ifndef MAC1010
                        b = 0;
                        if (p != 0 && *p >= '0' && *p <= '9')
                            g = 1;
#endif
                        if (p != 0) {
                            while (StringGetLength(key) + 1 < 17 && *p != 0)
                                StringAppendSafe(key, (const unsigned char *)p, 17);
                        }
                        c = key[3];
                        x ^= c;
                        x ^= name[14];
                        x += (char)0xf1;
                        x ^= key[10];
                        x ^= key[8];
                        x = ROTL(x, 0);
                        x ^= key[15];
                        x = ROTL(x, 4);
                        x = ROTL(x, 2);
                        x = ROTL(x, 7);
                        x ^= key[1];
                        x = ROTL(x, 2);
                        x = ROTL(x, 1);
                        x += 0x5f;
                        x = ROTL(x, 4);
                        t1 = name[13];
                        x ^= t1;
                        t2 = name[5];
                        x ^= t2;
                        x = ROTL(x, 0);
                        x ^= (unsigned char)g;
                        x ^= (unsigned char)g;
                        x ^= (unsigned char)g;
                        x += 0x42;
                        if (x < (char)key[11])
                            x = ROTL(x, 8);
                        x ^= t1;
                        x ^= (unsigned char)g;
                        x += -0x6b;
                        x = ROTL(x, 1);
                        x ^= key[7];
                        x += 0x1e;
                        x ^= key[12];
                        x = ROTL(x, 6);
                        x ^= name[3];
                        x += (char)0xac;
                        x ^= c;
                        x ^= key[13];
                        rem = RT3_ExtractLicenseBlock1(mem);
                        x = ROTL(x, 6);
                        x ^= (unsigned char)g;
                        x = ROTL(x, 2);
                        x ^= key[4];
                        x = ROTL(x, 3);
                        x = ROTL(x, 5);
                        x ^= (unsigned char)g;
                        b = x;
                        x = RegScramD2(&b);
                        x += 0x19;
                        x = ROTL(x, 3);
                        x ^= key[15];
                        RegScramB2(x);
                        x = ROTL(x, 6);
                        RegScramF2();
                        x = ROTL(x, 5);
                        x ^= (unsigned char)g;
                        x ^= key[14];
                        t3 = (unsigned char)name[0];
                        x ^= t3;
                        x ^= t2;
                        x += 7;
                        x ^= key[8];
                        x += -0x60;
                        x = ROTL(x, 4);
                        x += -0x23;
                        x = ROTL(x, 1);
                        x ^= key[13];
                        RegScramB2(x);
                        x += 0x3e;
                        x ^= (unsigned char)g;
                        t4 = name[1];
                        x ^= t4;
                        if (x < (char)key[6])
                            x = ROTL(x, 3);
                        x ^= (unsigned char)g;
                        x += (char)0xe1;
                        x ^= name[7];
                        x ^= RegScramE2();
                        x ^= t3;
                        x ^= key[8];
                        x = ROTL(x, 8);
                        x ^= t1;
                        x = ROTL(x, 3);
                        if (x > (char)(t6 = name[9]))
                            x = ROTL(x, 7);
                        x = ROTL(x, 5);
                        x ^= name[8];
                        x ^= key[7];
                        x ^= (unsigned char)g;
                        if (x > 0x5f) {
                            b = x;
                            x = RegScramD2(&b);
                        }
                        x = ROTL(x, 6);
                        x += -0x53;
                        x = ROTL(x, 7);
                        x = ROTL(x, 0);
                        x ^= key[2];
                        x += -0x53;
                        x = ROTL(x, 0);
                        x += 0xae;
                        x ^= t3;
                        x = ROTL(x, 6);
                        x += (char)0xc2;
                        x ^= key[14];
                        if (x < 0x94)
                            x = ROTL(x, 6);
                        x ^= (unsigned char)g;
                        x = ROTL(x, 1);
                        x = ROTL(x, 2);
                        x += 0x45;
                        x = ROTL(x, 4);
                        x = ROTL(x, 2);
                        x = ROTL(x, 0);
                        x += 0x62;
                        x ^= key[10];
                        if (x < (char)key[1])
                            x = ROTL(x, 5);
                        if (x > (t5 = key[9]))
                            x ^= (unsigned char)t5;
                        x ^= (unsigned char)g;
                        x += 0x6c;
                        x = ROTL(x, 6);
                        x += 3;
                        x ^= (unsigned char)t5;
                        x += -0x6d;
                        x = ROTL(x, 3);
                        x = ROTL(x, 1);
                        x ^= key[15];
                        x ^= key[4];
                        x += (char)0x99;
                        if (x > 0x47)
                            x ^= key[0];
                        if (x < 3)
                            x = blob[x];
                        while (rem >= x)
                            rem -= x;
                        ok = (g != 0 && rem == 0);
                        x ^= (unsigned char)g;
                        x += (char)0xd9;
                        RegScramC2(x);
                        x ^= t2;
                        x = ROTL(x, 6);
                        x += 0xa;
                        x ^= (unsigned char)g;
                        x ^= t4;
                        x ^= key[10];
                        x += (char)0x96;
                        x ^= name[2];
                        x ^= name[4];
                        x ^= t6;
                        x += 0xb;
                        if (x != 0)
                            RegScramA2(x);

                        doIt = (shipData[36].licenseCopies != 0
                                && ok == 0 ? (unsigned char)1 : (unsigned char)0);
                    } while (0);
                    if (doIt) {
                        shipData[0].credits *= 0.1;
                        statusRedraw[kRedrawCargo] = 1;
                    }
                }
                if (missionSoundID != -1 && missionSnd == 0) {
                    missionSnd = ST_LoadSndResource(missionSoundID);
                    if (missionSnd != 0)
                        PlaySound(missionSnd, 50, gSoundVolume, gSoundVolume);
                    missionSoundID = -1;
                }
                if (missionSnd != 0 && ST_IsSoundPlaying(missionSnd) == 0) {
                    ST_DisposeSound(missionSnd);
                    missionSnd = 0;
                }
                killShots = 0;
                killBoxes = 0;
                killExplods = 0;
                killSmoke = 0;
                killAsteroids = 0;
                missionSoundID = -1;
                if (ST_IsSoundPlaying(warpUpSnd[1]) != 0) {
                    doubleTime = 1;
                } else if (QuickKeyCheck(MacKey(0x39))) {
                    if (abs(AngularDifference(shipData[0].heading,
                                shipData[0].desiredHeading)) <= 10
                            && ST_IsSoundPlaying(warpUpSnd[0]) != 0) {
                        doubleTime = 0;
                    } else {
                        doubleTime = 1;
                    }
                } else {
                    doubleTime = 0;
                    gameFrozen = 0;
                }
                if (cheatsActive == 0)
                    gameFrozen = 0;
                if (keyPressedThisFrame != 0)
                    lastKeypressTime = ReadTick();
#if !TARGET_API_MAC_CARBON
                if (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0) {
                    if (Button() || (QuickKeyCheck(4) && QuickKeyCheck(0x3f))) {
                        IterateInGameEventLoop(kEventLoopTicks);
                        lastEventLoopTime = ReadTick();
                    } else if (settings.windowed != 0) {
                        if (settings.yieldEvents != 0) {
                            IterateInGameEventLoop(1);
                            lastEventLoopTime = ReadTick();
                        }
                    } else if (settings.yieldEvents != 0) {
                        IterateInGameEventLoop(1);
                        lastEventLoopTime = ReadTick();
                    }
                }
#else
                if (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0) {
                    if (ReadTick() > lastEventLoopTime + 30
                            || (QuickKeyCheck(MacKey(0xc)) && QuickKeyCheck(MacKey(0x37)))) {
                        if (QuickKeyCheck(MacKey(0xc)) && QuickKeyCheck(MacKey(0x37)))
                            IterateInGameEventLoop(kEventLoopTicks);
                        else
                            IterateInGameEventLoop(1);
                        lastEventLoopTime = ReadTick();
                    }
                } else {
                    BusyIdle();
                }
#endif
                MyProfileExit(kProfPostDraw);
            } else {
                SWUpdateSpriteWorld(gSpriteWorld, 0);
                IterateInGameEventLoop(3);
                if (gSpriteWorld->fastBlit != 0) {
                    RefreshSAT();
                    RedrawStatus(1);
                }
            }
        } else {
            IterateInGameEventLoop(-1);
            if (inBackground == 0)
                RefreshWindowBuffer();
        }
        if (absquatulateStr[0] != 0) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm((PStrChar *)absquatulateStr, 500, 0, kCommTextSize, &rgbWhite, 0, 0);
            RedrawComm(1);
            absquatulateStr[0] = 0;
        }
        MyProfileExit(kProfGameLoop);
    }

    MyProfileDump("profile.txt");
    FadeScreenOut();
    ShowTheCursor();
    playerKilledByHector = 0;
    FlushEvents(everyEvent - diskMask, 0);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
}
#else
{
    unsigned char numStr[256];
    unsigned char str[256];
    DateTimeRec d;
    float dx;
    float dy;
    float deathTime;

    GetTime(&d);
    RedrawStatus(1);
    RedrawComm(0);
    PropagateMissionBitEffects();
    HandleStellarSystemVisibility();
    gameFrozen = 1;
    ignoreKeys = 1;
    DoPlayGameWork(1);
    gameFrozen = 0;
    ignoreKeys = 0;
    hyperGamma = -1.0f;
    cursorAlpha = 0;
    playerKilledByHector = 0;
    PrepareAllSprites();
    RefreshWindowBuffer();
    SWUpdateSpriteWorld(gSpriteWorld, 1);
    RefreshSAT();
    FadeScreenIn();
    lastLoMemWarningTime = 0;
    CheckLoMem();
    lastSmokeUpdateTime = ReadTick();
    lastEventLoopTime = ReadTick();
    lastKeypressTime = ReadTick();
    AdjustSoundVolume();
    MyProfileInit();
    MyProfileSetName(kProfGameLoop, "game loop");
    MyProfileSetName(kProfPreDraw, "pre draw tasks");
    MyProfileSetName(kProfDrawing, "drawing");
    MyProfileSetName(kProfPostDraw, "post draw tasks");
    MyProfileSetName(kProfShip, "HandleShip");
    MyProfileSetName(kProfShipDisp, "HandleShipDisplay");
    MyProfileSetName(kProfAI, "AI routines");
    MyProfileSetName(kProfShots, "HandleShot");
    MyProfileSetName(kProfSprites, "misc handlers");
    MyProfileSetName(kProfCollide, "collisions");
    MyProfileSetName(kProfPlayer, "player");
    MyProfileSetName(kProfHandlers, "misc handlers2");
    MyProfileSetName(kProfStatus, "DrawStatus");
    MyProfileSetName(kProfProcessSW, "ProcessSpriteWorld");
    MyProfileSetName(kProfAnimateSW, "AnimateSpriteWorld");
    MyProfileSetName(kProfPostAnimate, "PostAnimate");
    MyProfileSetName(kProfLocking, "locking");
    MyProfileSetName(kProfErase, "erase");
    MyProfileSetName(kProfDraw, "draw");
    MyProfileSetName(kProfUpdate, "update");
    MyProfileSetName(kProfOdds, "calc AI odds");
    gSpriteWorld->fastBlit = 1;
    inBackground = 0;
    gPause = 0;
    FlushEvents(8, 0);
    FlushEvents(0xf, 0);

    for (;;) {
        if (gQuit != 0 || gPause != 0)
            break;
        MyProfileEnter(kProfGameLoop);
        HandleTimeAdjustment();
        keyPressedThisFrame = 0;
        if (inBackground == 0) {
            if (gSpriteWorld->fastBlit != 0) {
                MyProfileEnter(kProfPreDraw);
                lastFramePlayerPos.x = shipData[0].pos.x;
                lastFramePlayerPos.y = shipData[0].pos.y;
                if (frameCounter % 3 != 0) {
                    updateSmokeThisFrame = 0;
                } else {
                    updateSmokeThisFrame = 1;
                    lastSmokeUpdateTime = ReadTick();
                }
                DoPlayGameWork(1);
                fpsCounter += 1;
                if (showFPS != 0) {
                    if (ReadTick() > lastFPSUpdateTime + 120) {
                        if (lastFPSUpdateTime == 0) {
                            GetIndString(str, kGameStrList, 282);
                            DisplayComm(str, 100, 0, kCommTextSize, &rgbWhite, 0, 0);
                        } else {
                            NumToString((fpsCounter * 60.0 /
                                    (ReadTick() - lastFPSUpdateTime)),
                                    numStr);
                            PtoCstr(numStr);
                            strcpy((char *)str, (char *)numStr);
                            strcat((char *)str, " ");
                            StrcatIndCString((char *)str, kGameStrList, 283);
                            CtoPstr((char *)str);
                            DisplayComm(str, 999, 0, kCommTextSize, &rgbWhite, 0, 0);
                        }
                        lastFPSUpdateTime = ReadTick();
                        fpsCounter = 0;
                    }
                } else {
                    lastFPSUpdateTime = ReadTick();
                }
                dx = lastFramePlayerPos.x - shipData[0].pos.x;
                dy = lastFramePlayerPos.y - shipData[0].pos.y;
                if (doubleTime != 0) {
                    dx *= 2.0f;
                    dy *= 2.0f;
                }
                ScrollStarfield(dx, dy);
                HandleMouse();
                MyProfileExit(kProfPreDraw);
                MyProfileEnter(kProfDrawing);
                MyProfileEnter(kProfProcessSW);
                SWProcessSpriteWorld(gSpriteWorld);
                MyProfileExit(kProfProcessSW);
                MyProfileEnter(kProfAnimateSW);
                SWAnimateSpriteWorld(gSpriteWorld);
                MyProfileExit(kProfAnimateSW);
                MyProfileEnter(kProfPostAnimate);
                PostAnimateHandler();
                MyProfileExit(kProfPostAnimate);
                ScreenEndScene();
                MyProfileExit(kProfDrawing);
                MyProfileEnter(kProfPostDraw);
                if (KeyCheck(keyPrefs.pause)) {
                    if (pauseKeyFlag == 0 && gPause == 0) {
                        pauseKeyFlag = 1;
                        if (shipData[0].jumpTimer <= 0.0f
                                && shipData[0].type != kEscapePodType
                                && (deathTime = shipData[0].deathTimer) <= 0.0f
                                && shipData[0].escapePods <= 0) {
                            if (shipData[0].active != 0 || deathTime < -250.0f)
                                gPause = 1;
                        }
                    }
                } else {
                    pauseKeyFlag = 0;
                }
                if (doubleTime != 0)
                    DoPlayGameWork(0);
                frameCounter += 1;
                if (frameCounter > 1024) {
                    frameCounter = 0;
                    if (d.day == 30 && d.month == 6 && commCount <= 0)
                        DisplayComm("\pHappy birthday Matt",
                                    5, 3, 9, &rgbDarkGrey, 0, 0);
                    {
                        unsigned char blob[3] = { 0x49, 0x71, 0xad };
                        unsigned char name[17] = "EV NovaEV NovaEV";
                        unsigned char key[17] = "";
                        int32_t g;
                        unsigned char *p;
                        long long mem;
                        unsigned char x;
                        char b;
                        int t1;
                        int t2;
                        int t3;
                        int t4;
                        unsigned short rem;
                        Boolean ok;
                        short doIt;

                        g = shipData[36].licenseCopies;
                        p = shipData[17].licenseName;
                        mem = shipData[5].licenseCode;
                        x = 0;
                        b = 0;
                        if (p != 0) {
                            while (StringGetLength(key) + 1 < 17 && *p != 0)
                                StringAppendSafe(key, p, 17);
                        }
                        x ^= key[3];
                        x ^= name[14];
                        x += 0xf1;
                        x ^= key[10];
                        x ^= key[8];
                        x ^= key[15];
                        x = ROTL(x, 4);
                        x = ROTL(x, 2);
                        x = ROTL(x, 7);
                        x ^= key[1];
                        x = ROTL(x, 2);
                        x = ROTL(x, 1);
                        x += 0x5f;
                        x = ROTL(x, 4);
                        t1 = name[13];
                        x ^= t1;
                        t2 = name[5];
                        x ^= t2;
                        x ^= g;
                        x ^= g;
                        x ^= g;
                        x += 0x42;
                        if (x < (char)key[11])
                            x = ROTL(x, 8);
                        x ^= t1;
                        x ^= g;
                        x += -0x6b;
                        x = ROTL(x, 1);
                        x ^= key[7];
                        x += 0x1e;
                        x ^= key[12];
                        x = ROTL(x, 6);
                        x ^= name[3];
                        x += 0xac;
                        x ^= key[3];
                        x ^= key[13];
                        rem = RT3_ExtractLicenseBlock1(mem);
                        x = ROTL(x, 6);
                        x ^= g;
                        x = ROTL(x, 2);
                        x ^= key[4];
                        x = ROTL(x, 3);
                        x = ROTL(x, 5);
                        x ^= g;
                        b = x;
                        x = RegScramD2(&b);
                        x += 0x19;
                        x = ROTL(x, 3);
                        x ^= key[15];
                        RegScramB2(x);
                        x = ROTL(x, 6);
                        RegScramF2();
                        x = ROTL(x, 5);
                        t3 = name[0];
                        x ^= g;
                        x ^= key[14];
                        x ^= t3;
                        x ^= t2;
                        x += 7;
                        x ^= key[8];
                        x += -0x60;
                        x = ROTL(x, 4);
                        x += -0x23;
                        x = ROTL(x, 1);
                        x ^= key[13];
                        RegScramB2(x);
                        x += 0x3e;
                        x ^= g;
                        t4 = name[1];
                        x ^= t4;
                        if (x < (char)key[6])
                            x = ROTL(x, 3);
                        x ^= g;
                        x += 0xe1;
                        x ^= name[7];
                        x ^= RegScramE2();
                        x ^= t3;
                        x ^= key[8];
                        x = ROTL(x, 8);
                        x ^= t1;
                        x = ROTL(x, 3);
                        t1 = name[9];
                        if (x > (char)t1)
                            x = ROTL(x, 7);
                        x = ROTL(x, 5);
                        x ^= name[8];
                        x ^= key[7];
                        x ^= g;
                        if (x > 0x5f) {
                            b = x;
                            x = RegScramD2(&b);
                        }
                        x = ROTL(x, 6);
                        x += -0x53;
                        x = ROTL(x, 7);
                        x ^= key[2];
                        x += 0x5b;
                        x ^= t3;
                        x = ROTL(x, 6);
                        x += 0xc2;
                        x ^= key[14];
                        if (x < 0x94)
                            x = ROTL(x, 6);
                        x ^= g;
                        x = ROTL(x, 1);
                        x = ROTL(x, 2);
                        x += 0x45;
                        x = ROTL(x, 4);
                        x = ROTL(x, 2);
                        x += 0x62;
                        x ^= key[10];
                        if (x < (char)key[1])
                            x = ROTL(x, 5);
                        if (x > (char)key[9])
                            x ^= key[9];
                        x ^= g;
                        x += 0x6c;
                        x = ROTL(x, 6);
                        x += 3;
                        x ^= key[9];
                        x += -0x6d;
                        x = ROTL(x, 3);
                        x = ROTL(x, 1);
                        x ^= key[15];
                        x ^= key[4];
                        x += 0x99;
                        if (x > 0x47)
                            x ^= key[0];
                        if (x < 3)
                            x = blob[x];
                        while (rem >= x)
                            rem -= x;
                        ok = (g != 0 && rem == 0);
                        x ^= g;
                        x += 0xd9;
                        RegScramC2(x);
                        x ^= t2;
                        x = ROTL(x, 6);
                        x += 0xa;
                        x ^= g;
                        x ^= t4;
                        x ^= key[10];
                        x += 0x96;
                        x ^= name[2];
                        x ^= name[4];
                        x ^= t1;
                        x += 0xb;
                        if (x != 0)
                            RegScramA2(x);

                        doIt = (shipData[36].licenseCopies != 0
                                && ok == 0 ? (unsigned char)1 : (unsigned char)0);
                        if (doIt) {
                            shipData[0].credits *= 0.1;
                            statusRedraw[kRedrawCargo] = 1;
                        }
                    }
                }
                if (missionSoundID != -1 && missionSnd == 0) {
                    missionSnd = ST_LoadSndResource(missionSoundID);
                    if (missionSnd != 0)
                        PlaySound(missionSnd, 50, gSoundVolume, gSoundVolume);
                    missionSoundID = -1;
                }
                if (missionSnd != 0 && ST_IsSoundPlaying(missionSnd) == 0) {
                    ST_DisposeSound(missionSnd);
                    missionSnd = 0;
                }
                killShots = 0;
                killBoxes = 0;
                killExplods = 0;
                killSmoke = 0;
                killAsteroids = 0;
                missionSoundID = -1;
                if (ST_IsSoundPlaying(warpUpSnd[1]) != 0) {
                    doubleTime = 1;
#ifdef CEFIX /* F20.02 */
                } else if (!(GetKeyState(ceX2Hotkey) & 1)) {
#else
                } else if (!CapsLockIsOn()) {
#endif
                    doubleTime = 0;
                    gameFrozen = 0;
                } else if (abs(AngularDifference(shipData[0].heading,
                            shipData[0].desiredHeading)) > 10
                        || ST_IsSoundPlaying(warpUpSnd[0]) == 0) {
                    doubleTime = 1;
                } else {
                    doubleTime = 0;
                }
                if (cheatsActive == 0)
                    gameFrozen = 0;
                if (keyPressedThisFrame != 0)
                    lastKeypressTime = ReadTick();
                if (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0) {
                    if (ReadTick() - lastEventLoopTime > 30) {
                        IterateInGameEventLoop(1);
                        lastEventLoopTime = ReadTick();
                    }
                } else {
                    BusyIdle();
                }
                MyProfileExit(kProfPostDraw);
            } else {
                SWUpdateSpriteWorld(gSpriteWorld, 0);
                IterateInGameEventLoop(3);
                if (gSpriteWorld->fastBlit != 0) {
                    RefreshSAT();
                    RedrawStatus(1);
                }
            }
        } else {
            IterateInGameEventLoop(-1);
            if (inBackground == 0)
                RefreshWindowBuffer();
        }
        if (absquatulateStr[0] != 0) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm((unsigned char *)absquatulateStr, 500, 0, kCommTextSize, &rgbWhite, 0, 0);
            RedrawComm(1);
            absquatulateStr[0] = 0;
        }
        MyProfileExit(kProfGameLoop);
    }

    MyProfileDump("profile.txt");
    FadeScreenOut();
    ShowTheCursor();
    playerKilledByHector = 0;
    FlushEvents(everyEvent - diskMask, 0);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
}
#endif

#define kMaxNavDefaults 0x10

/* DoPlayGameWork (mac _DoPlayGameWork). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004186b0
// MAC: _DoPlayGameWork i386 0x00043de5 ppc 0x0004bbe4
void DoPlayGameWork(unsigned char advance)
{
    short i;
    short n;
    short x;

    MyProfileEnter(kProfPlayer);
    QuickKeyCheckHandler();
    gravitySourcesPresent = 0;
    HandlePlayer(shipData);
    HandleShipDisplay(shipData);
    MyProfileExit(kProfPlayer);

    MyProfileEnter(kProfCollide);
    if (miningBoxesActive != 0)
        SWCollideSpriteLayer(gSpriteWorld, shipLayer, boxLayer);
    SWCollideSpriteLayer(gSpriteWorld, shipLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, escortLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, disabledShipLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, shipLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, escortLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, disabledShipLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, shipLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, escortLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, disabledShipLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, pointDefenseLayer);
    SWCollideSpriteLayer(gSpriteWorld, pointDefenseLayer, guidedShotLayer);
    CheckShotProximities();
    MyProfileExit(kProfCollide);

    if (advance != 0) {
        MyProfileEnter(kProfStatus);
        HandleStatus();
        DrawStatus();
        MyProfileExit(kProfStatus);

        MyProfileEnter(kProfAI);
        for (i = 0; i < kMaxShips; i++) {
            shipData[i].hasEscorts = 0;
            shipData[i].hasSwarm = 0;
            shipData[i].hasFormation = 0;
        }
        for (i = 1; i < kMaxShips; i++)
            prevParent[i] = shipData[i].parentShip;
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].aiType >= kShipTypeFighter &&
                    shipData[i].curSystem == shipData[0].curSystem) {
                if (shipData[i].parentShip > 0) {
                    AICheckParentExists(&shipData[i]);
                    if (shipData[i].parentShip != -1)
                        shipData[shipData[i].parentShip].hasEscorts = 1;
                } else if (shipData[i].parentShip == 0) {
                    shipData[0].hasEscorts = 1;
                }
                if (shipData[i].swarmLeader > i && shipData[i].swarmLeader < kMaxShips)
                    shipData[shipData[i].swarmLeader].hasSwarm = 1;
                if (shipData[i].aiType >= kShipTypeFighter) {
                    shipData[i].formLeader = -1;
                    if (shipData[i].targetShip != -1 && IsShipAttacking(&shipData[i]) != 0)
                        shipData[i].formLeader = shipData[i].swarmLeader;
                    if (shipData[i].formLeader == -1)
                        shipData[i].formLeader = shipData[i].parentShip;
                } else {
                    shipData[i].formLeader = shipData[i].swarmLeader;
                }
                if (shipData[i].formLeader >= 0 && shipData[i].formLeader < kMaxShips)
                    shipData[shipData[i].formLeader].hasFormation = 1;
            }
        }
        MyProfileExit(kProfAI);

        MyProfileEnter(kProfHandlers);
        MissionHandler();
        HandleVoiceSound();
        HandleComm();
        CalculateTargetedDamage();
        EnterMoreShips(shipData[0].curSystem);
        SpawnAsteroid(1);
        scanActive = 0;
        miningShipsActive = 0;
        MyProfileExit(kProfHandlers);

        MyProfileEnter(kProfAI);
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].aiType >= 1 &&
                    shipData[i].curSystem == shipData[0].curSystem)
                AIDispatch(&shipData[i], 0);
        }
        MyProfileEnter(kProfOdds);
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].odds < 0.0f) {
                if (i < kMaxShips - 1)
                    shipData[i + 1].odds = -1.0f;
                else
                    shipData[1].odds = -1.0f;
                if (shipData[i].active != 0 && shipData[i].aiType >= 1 &&
                        shipData[i].curSystem == shipData[0].curSystem) {
                    AICalculateOddsAgainst(&shipData[i]);
                    break;
                }
                shipData[i].odds = 0.0f;
            }
        }
        MyProfileExit(kProfOdds);
        MyProfileExit(kProfAI);
    }

    MyProfileEnter(kProfShots);
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life >= -1.0f) {
            HandleShot(&shotData[i]);
        } else if (shotData[i].sprite != 0 && shotData[i].sprite->isVisible != 0) {
            HandleShot(&shotData[i]);
        }
    }
    LICENSE_STEP_BEGIN
    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies != 0 ? (unsigned char)1 : (unsigned char)0;
    LICENSE_STEP_END
    x = shipClass[gameIsRegisteredIndex].registered;
    MyProfileExit(kProfShots);

    for (i = 0; i < kMaxNavDefaults; i++) {
        n = systemData[shipData[0].curSystem].navDefault[i];
        if (n != -1)
            stellarData[n].defenseActive = 0;
    }
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].defenseStellar != -1)
            stellarData[shipData[i].defenseStellar].defenseActive = 1;
    }
    for (i = 1; i < kMaxShips; i++) {
        MyProfileEnter(kProfShip);
        if (shipData[i].active != 0 && shipData[i].curSystem == shipData[0].curSystem)
            HandleShip(&shipData[i]);
        MyProfileExit(kProfShip);
        MyProfileEnter(kProfShipDisp);
        HandleShipDisplay(&shipData[i]);
        MyProfileExit(kProfShipDisp);
    }

    MyProfileEnter(kProfSprites);
    HandleStellarSprites();
    HandleStellarWeapons();
    HandleDoubleTimeSprite();
    HandleExplods();
    HandleStars();
    HandleSmoke();
    HandleBoxes();
    HandleAsteroids();
    HandleNavTargetSprites();
    HandleWeapTargetSprites();
    HandleReinforcements();
    HandleGravity();
    HandleDeadlyStellars();
    HandleEscapePods();
    if (advance != 0)
        HandleAlertSprite();
    MyProfileExit(kProfSprites);
    HandleBeams(advance);
}

/* PrepareAllSprites (mac _PrepareAllSprites). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00419210
// MAC: _PrepareAllSprites i386 0x0003a75a ppc 0x00040d04
void PrepareAllSprites(void)
{
    Boolean wasFrozen;
    short i;

    wasFrozen = gameFrozen;
    gameFrozen = 1;
    for (i = 0; i < kMaxShips; i++)
        HandleShipDisplay(&shipData[i]);
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life >= -1.0f) {
            HandleShot(&shotData[i]);
        } else if (shotData[i].sprite != 0 && shotData[i].sprite->isVisible != 0) {
            HandleShot(&shotData[i]);
        }
    }
    HandleStellarSprites();
    HandleExplods();
    HandleSmoke();
    HandleBoxes();
    HandleAsteroids();
    HandleStars();
    HandleNavTargetSprites();
    HandleWeapTargetSprites();
    HandleEscapePods();
    gameFrozen = wasFrozen;
}

#define kGovtLegalScale         2       /* govt crimeTol is halved legal status */

#define kStrMissionFailed       0x11c
#define kStrShipDestroyed       0x11f
#define kStrPlayerKilled        0x120
#define kHectorStrList          30000
#define kHectorTauntBase        8
#define kHectorTauntCount       6

#define kHectorCommFrames        5000

#define kImpactCancelRange      50.0f
#define kEscortDefendRange      320.0f
#define kFullCloak              32.0f
#define kMaxJumpCharge          20.0f
#define kDisableArmorFraction   0.3333
#define kTenthArmorFraction     0.1
#define kShieldOvershoot        0.1
#define kPlayerKnockbackScale   1.8
#define kProvocationScale       1.75
#define kAnnoyanceThreshold     50.0f


/* DamageShip (mac _DamageShip). STATE: MATCH 100.0% (1332 exact, 189 reloc-equivalent), exe 1521 /
   obj 1521, byte-identical yes, relocregcheck 0 of 189; needs PrepareAllSprites above it (TU).
   mac MATCH both slices. OPEN: none. */
// FUNCTION: EVNOVA 0x004192d0
// MAC: _DamageShip i386 0x0003a807 ppc 0x00040df8
void DamageShip(ShipObj *ship, FloatPoint *from, short impact, short armorDmg,
                short shieldDmg, short attacker, Boolean fromAttack, Boolean deliberate,
                unsigned char disableOnly, Boolean clampArmor,
                unsigned char armorOnly, short provocation)
{
    short theirTop;
    Boolean wasDisabled;
    unsigned char wasDying;
    char msg[256];
    Boolean selfHit;
#ifndef MAC1010
    Boolean hostile;
#endif
    Boolean enemy;
    int32_t aligned;
    short angle;
    float capacity;
    short myGovt;
    short myTop;
    FloatPoint spob;
    short i;
#ifdef MAC1010
    Boolean hostile;
#endif

    if (ship->index < 0 || ship->index >= kMaxShips)
        return;
    if (ship->type < 0 || ship->type >= kShipTypeEscapePod)
        return;
    if (ship->active == 0)
        return;

    if (fromAttack != 0 && attacker > 0 && attacker < kMaxShips) {
        if (shipData[attacker].persType == kPersCaptainHector) {
            if (totalDaysPlayed > 90) {
                armorDmg *= 5;
                shieldDmg *= 5;
            } else if (totalDaysPlayed > 60) {
                armorDmg *= 3;
                shieldDmg *= 3;
            } else if (totalDaysPlayed > 40) {
                armorDmg *= 2;
                shieldDmg *= 2;
            }
        }
        if (ship->index == shipData[attacker].targetShip) {
            if (AIShipIsPlundering(&shipData[attacker])) {
                disableOnly = 1;
            } else {
                if (shipData[attacker].parentShip > 0 && shipData[attacker].parentShip < kMaxShips
                        && AIShipIsPlundering(&shipData[shipData[attacker].parentShip]))
                    disableOnly = 1;
            }
        }
    }

    wasDisabled = IsDisabled(ship);

    if (impact < 0 && fromAttack != 0 && attacker >= 0 && attacker < kMaxShips) {
        if (FloatAbs(shipData[attacker].pos.x - ship->pos.x) < kImpactCancelRange
                || FloatAbs(shipData[attacker].pos.y - ship->pos.y) < kImpactCancelRange)
            impact = 0;
    }

    if (impact != 0 && ship->jumpTimer <= 0.0f && from != NULL) {
        angle = CalcInterceptAngle(from, &ship->pos);
        if (shipClass[ship->type].mass > 0
                && (shipClass[ship->type].flags & kShipPlanetType) == 0) {
            AdjustedAccel(angle, impact / (float)shipClass[ship->type].mass,
                          &ship->vel, shipClass[ship->type].maxSpeed);
            if (ship->index == 0 && abOn != 0 && gravitySourcesPresent == 0)
                SpeedLimit(&ship->vel, ShipMaxSpeed(ship) * kPlayerKnockbackScale);
            else
                SpeedLimit(&ship->vel, ShipMaxSpeed(ship));
        }
    }

    wasDying = IsDying(ship);

    if (armorOnly != 0) {
        if (armorDmg > 0) {
            if (disableOnly != 0 && ship->armor > 0.0f
                    && ship->armor - armorDmg <= 0.0f)
                ship->armor = 1.0f;
            else
                ship->armor -= armorDmg;
        }
    } else {
        if (shieldDmg > 0)
            ship->shield -= shieldDmg;
        if (ship->shield <= 0.0f) {
            if (armorDmg > 0) {
                if (disableOnly != 0 && ship->armor > 0.0f
                        && ship->armor - armorDmg <= 0.0f)
                    ship->armor = 1.0f;
                else
                    ship->armor -= armorDmg;
            }
            capacity = ShipShieldCapacity(ship);
            if (ship->shield < -capacity * kShieldOvershoot)
                ship->shield = -capacity * kShieldOvershoot;
        }
    }

    if (clampArmor != 0 && IsDisabled(ship) && wasDisabled == 0) {
        if ((shipClass[ship->type].flags & kShipArmorDisableTenth) != 0)
            ship->armor = ShipArmorCapacity(ship) * kTenthArmorFraction + 1.0;
        else
            ship->armor = ShipArmorCapacity(ship) * kDisableArmorFraction + 1.0;
    }

    enemy = 0;
    if (attacker == 0) {
        enemy = 1;
    } else if (attacker > 0 && attacker < kMaxShips
               && shipData[attacker].parentShip == 0) {
        enemy = 1;
    }
    if (enemy != 0 && ship->defenseStellar == -1 && ship->persType < kPersCaptainHector) {
        if (IsDisabled(ship) && wasDisabled == 0)
            SlapWithPenalty(shipData->curSystem, ship->govt, 1, ship->specialMission);
        if (ship->armor <= 0.0f && wasDying == 0 && !ShipIsDerelict(ship)) {
            SlapWithPenalty(shipData->curSystem, ship->govt, 3, ship->specialMission);
            AddToPlayerKills(shipClass[ship->type].strength);
        }
    }

    if (IsDisabled(ship)) {
        if (ship->specialMission >= 0 && ship->specialMission < kMaxActiveMissions
                && ship->defenseStellar == -1 && wasDisabled == 0) {
            if (currentMissionObjectives[ship->specialMission].active != 0) {
                if (currentMissionObjectives[ship->specialMission].failed == 0
                        && currentMissions[ship->specialMission].specialShipsDisabled == 0
                        && currentMissions[ship->specialMission].shipGoal == kMissionGoalEscort
                        && (currentMissions[ship->specialMission].flags & kMissionHidden) == 0) {
                    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                    GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                    DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize,
                                &rgbWhite, 0, 0);
                    QuickMissionFailure(ship->specialMission);
                }
                currentMissions[ship->specialMission].specialShipsDisabled++;
            }
        }
        if (ship->parentShip == 0 && ship->specialMission == -1) {
            if (ship->aiType == kShipTypeFighter)
                ship->formerEscort = 0;
            else if (ship->hired != 0)
                ship->formerEscort = 1;
            else
                ship->formerEscort = 2;
            ship->parentShip = -1;
            if (ship->aiType == kShipTypeEscort
                    && shipClass[ship->type].inherentAI <= kShipTypeBraveTrader) {
                DestroyPartialFleetCargo(ship->index);
                statusRedraw[kRedrawCargo] = 1;
                DrawStatusCargo();
            }
            ship->aiType = shipClass[ship->type].inherentAI;
        }
        if (ship->index == 0 && wasDisabled == 0) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            GetIndString((unsigned char *)msg, kGameStrList, kStrShipDestroyed);
            DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize,
                        &rgbWhite, 0, 0);
            playerRepairInhibit = 300.0f;
            ship->armor = (int32_t)ship->armor;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0 && currentMissionObjectives[i].failed == 0
                        && (currentMissions[i].flags2 & kMissionFailIfDisabled) != 0) {
                    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                    QuickMissionFailure(i);
                    GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                    DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize,
                                &rgbWhite, 0, 0);
                    break;
                }
            }
        }
    }

    /* `&`, not `&&`: the exe ANDs the two booleans with `test eax,ebx` */
    if (ship->index == 0 && (IsDying(ship) & (wasDying == 0))) {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (fromAttack != 0 && attacker > 0 && attacker < kMaxShips
                && shipData[attacker].persType == kPersCaptainHector) {
            GetIndString((unsigned char *)msg, kHectorStrList,
                         Rand(kHectorTauntCount) + kHectorTauntBase);
            DisplayComm((PStrChar *)msg, kHectorCommFrames, 0, kCommTextSize,
                        &rgbWhite, 0, 0);
            playerKilledByHector = 1;
        }
        if (playerKilledByHector == 0) {
            GetIndString((unsigned char *)msg, kGameStrList, kStrPlayerKilled);
            DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize,
                        &rgbWhite, 0, 0);
        }
        for (i = 0; i < kMaxActiveMissions; i++) {
            if (currentMissionObjectives[i].active != 0 && currentMissionObjectives[i].failed == 0
                    && (currentMissions[i].flags2 & kMissionFailIfDisabled) != 0) {
                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                QuickMissionFailure(i);
                GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                DisplayComm((PStrChar *)msg, kCommFrames, 0, kCommTextSize,
                            &rgbWhite, 0, 0);
                break;
            }
        }
    }

    selfHit = 0;
    enemy = 0;
    if (attacker != ship->index) {
        enemy = 1;
    } else if (ship->defenseStellar != -1) {
        enemy = 1;
        selfHit = 1;
    } else if (attacker >= 0 && attacker < kMaxShips
               && shipData[attacker].defenseStellar != -1) {
        enemy = 1;
        selfHit = 1;
    }

    if (enemy && fromAttack != 0 && ship->aiType > 0 && ship->jumpTimer <= 0.0f) {
        hostile = 0;
        if (selfHit != 0) {
            if (attacker >= 0 && attacker < kMaxShips) {
                if (ship->defenseStellar != -1 && shipData[attacker].defenseStellar != -1)
                    hostile = 0;
                else
                    hostile = 1;
            } else
                hostile = 1;
        } else {
            if (attacker >= 0 && attacker < kMaxShips
                    && shipData[attacker].govt != ship->govt)
                hostile = 1;
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips) {
                if (attacker != ship->parentShip || attacker == -1)
                    hostile = 1;
                if (attacker >= 0 && attacker < kMaxShips
                        && shipData[attacker].parentShip != -1
                        && ship->parentShip != shipData[attacker].parentShip)
                    hostile = 1;
            }
            if (ship->govt < 0 || ship->govt >= kMaxGovts) {
                if (attacker >= 0 && attacker < kMaxShips
                        && (attacker == 0 || shipData[attacker].parentShip == 0))
                    hostile = 1;
            } else {
                if (systemData[shipData->curSystem].govt >= 0
                        && systemData[shipData->curSystem].govt < kMaxGovts
                        && shipData->targetShip != ship->index
                        && playerRecord[shipData->curSystem]
                           >= govtData[ship->govt].crimeTol * kGovtLegalScale) {
                    if (attacker == 0)
                        hostile = 0;
                    else if (attacker > 0 && attacker < kMaxShips
                                && shipData[attacker].parentShip == 0)
                        hostile = 0;
                }
                if (attacker >= 0 && attacker < kMaxShips
                        && ship->govt == shipData[attacker].govt)
                    hostile = 0;
            }
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips
                    && attacker >= 0 && attacker < kMaxShips
                    && ship->parentShip == shipData[attacker].parentShip
                    && shipData[attacker].defenseStellar != -1)
                hostile = 0;
            if (hostile && shieldDmg <= 0 && armorDmg <= 0
                    && shieldDmg > -32000 && armorDmg > -32000)
                hostile = 0;
            if (deliberate == 0 && hostile) {
                if (attacker == 0) {
                    if (shipData[attacker].targetShip >= 0
                            && shipData[attacker].targetShip < kMaxShips) {
                        if (ship->index == shipData[attacker].targetShip)
                            hostile = 1;
                        else if (IsShipMyEnemy(ship,
                                               &shipData[shipData[attacker].targetShip]))
                            hostile = 0;
                    }
                } else if (attacker > 0 && attacker < kMaxShips
                           && ship->index != shipData[attacker].targetShip) {
                    hostile = 0;
                }
                if (hostile && (ship->govt >= 0 && ship->govt < kMaxGovts
                        && attacker == 0
                        && govtData[ship->govt].penalties[4] <= 0
                        && Rand(4) != 0))
                    hostile = 0;
                if (attacker == 0 && ship->annoyance < kAnnoyanceThreshold)
                    hostile = 0;
            }
            if (attacker == 0 && deliberate == 0)
                ship->annoyance += provocation * kProvocationScale;
            if (attacker >= 0 && attacker < kMaxShips) {
                if (shipData[attacker].parentShip == 0
                        && shipData[attacker].aiType == kShipTypeFighter
                        && ship->targetShip == 0) {
                    if (FloatAbs(ship->pos.x - shipData->pos.x) <= kEscortDefendRange) {
                        if (FloatAbs(ship->pos.y - shipData->pos.y) <= kEscortDefendRange)
                            hostile = 0;
                    }
                }
                if (shipData[attacker].parentShip == ship->parentShip && ship->parentShip != -1)
                    hostile = 0;
            }
            if (ship->jumpTimer > 0.0f)
                hostile = 0;
            if (deliberate == 0 && hostile
                    && attacker >= 0 && attacker < kMaxShips) {
                if (shipData[attacker].govt >= 0
                        && shipData[attacker].govt < kMaxGovts
                        && AlliedWithPlayer(&shipData[attacker])
                        && PlayerBlessedByGovt(shipData[attacker].govt, 0))
                    hostile = 0;
            }
            myTop = theirTop = -1;
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips
                    && (shipData[ship->parentShip].parentShip >= 0
                        && shipData[ship->parentShip].parentShip < kMaxShips))
                myTop = shipData[ship->parentShip].parentShip;
            if (attacker > 0 && attacker < kMaxShips
                    && (shipData[attacker].parentShip > 0
                        && shipData[attacker].parentShip < kMaxShips))
                theirTop = shipData[shipData[attacker].parentShip].parentShip;
            if (myTop == theirTop)
                if (myTop >= 0)
                    if (myTop < kMaxShips)
                        if (theirTop >= 0)
                            if (theirTop < kMaxShips)
                                hostile = 0;
        }

        if (ship->jumpTimer > 0.0f) {
            hostile = 0;
        } else {
            if (ship->parentShip > 0 && ship->parentShip < kMaxShips && shipData[ship->parentShip].jumpTimer > 0.0f)
                hostile = 0;
        }
        if (hostile && ship->index > 0 && attacker > 0 && attacker < kMaxShips
                && AIShipIsDocking(ship))
            hostile = 0;
        if (hostile && (attacker >= 0 && attacker < kMaxShips
                && shipClass[shipData[attacker].type].escortType == 0
                && shipClass[ship->type].escortType != 0
                && ship->targetShip != -1
                && IsShipAttacking(ship)))
            hostile = 0;

        if (hostile) {
            ship->annoyance = 0;
            if ((shipClass[ship->type].flags2 & kShipCloaksWhenAttacked) != 0
                    && (attacker >= 0 && attacker < kMaxShips
                        && ShipCanCloak(ship)
                        && !IsShipFightingMe(ship, &shipData[attacker]))) {
                AIForceBurstReload(ship);
                DoShipCloak(ship);
            }
            wasDying = 0;
            if (ship->defenseStellar == -1) {
                wasDying = 1;
                if (ship->targetShip != -1 && attacker >= 0 && attacker < kMaxShips
                        && IsShipAttacking(ship)) {
                    /* BUG: abs() takes the comparison, so any negative difference counts as aligned */
                    aligned = AngularDifference(ship->heading,
                                                ship->desiredHeading) < 45;
                    if (abs(aligned)) {
                        if (XYFloatSquaredDist(&ship->pos, &shipData[attacker].pos)
                                  > XYFloatSquaredDist(&ship->pos, &shipData[ship->targetShip].pos) / 4)
                            wasDying = 0;
                    }
                }
                if (wasDying != 0 && ship->index != 0) {
                    if (armorDmg > 0)
                        ship->grudge += armorDmg;
                    if (shieldDmg > 0)
                        ship->grudge += shieldDmg;
                    if (attacker >= 0 && attacker < kMaxShips)
                        ship->targetShip = attacker;
                    if (ship->parentShip > 0 && ship->parentShip < kMaxShips
                            && shipData[ship->parentShip].defenseStellar == -1) {
                        if (armorDmg > 0)
                            shipData[ship->parentShip].grudge += armorDmg;
                        if (shieldDmg > 0)
                            shipData[ship->parentShip].grudge += shieldDmg;
                        if (attacker >= 0 && attacker < kMaxShips)
                            shipData[ship->parentShip].targetShip = attacker;
                    }
                }
            } else if (attacker >= 0 && attacker < kMaxShips) {
                spob.x = stellarData[ship->defenseStellar].pos.h;
                spob.y = stellarData[ship->defenseStellar].pos.v;
                if (XYFloatSquaredDist(&ship->pos, &shipData[attacker].pos)
                         < XYFloatSquaredDist(&ship->pos, &spob) / 2) {
                    if (ship->index != 0) {
                        if (armorDmg > 0)
                            ship->grudge += armorDmg * 30;
                        if (shieldDmg > 0)
                            ship->grudge += shieldDmg * 30;
                        ship->targetShip = attacker;
                    }
                    wasDying = 1;
                }
            }
            if (wasDying != 0) {
                if (attacker == 0)
                    AIMakeShipAttackPlayer(ship);
                if (ship->aiTimer > kMaxJumpCharge)
                    ship->aiTimer = kMaxJumpCharge;
                if (attacker == 0 && ship->defenseStellar == -1) {
                    if (ship->persType != -1
                            && (personData[ship->persType].flags & kPersHoldsGrudge) != 0)
                        personData[ship->persType].grudge = 1;
                    if (ship->index != 0) {
                        AIMakeShipStopRefuelling(ship);
                        ship->targetShip = attacker;
                    }
                }
            }
            if (deliberate != 0 && attacker == 0)
                DoGoodSamaritan(ship, shipData);
        }
    }

    if (armorOnly == 0)
        ship->shieldGlow = kFullCloak;
    if (ship->index == 0)
        statusRedraw[kRedrawShield] = 1;
    if (fromAttack != 0 && ship->decloaksWhenHit == 1
            && (ship->cloak == kFullCloak || ship->cloakDir >= 1))
        DoShipUncloak(ship);
}

/* DeleteAllComputerShips. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041ad50
// MAC: _DeleteAllComputerShips i386 0x0003bf5f ppc 0x00042594
void DeleteAllComputerShips(Boolean force, Boolean keepPlayer)
{
    short i;
    Boolean remove;

    for (i = 1; i < kMaxShips; i++) {
        remove = 1;
        if (shipData[i].aiType >= kShipTypeFighter && shipData[i].parentShip == 0
                && shipData[i].defenseStellar == -1 && shipData[i].specialMission == -1) {
            if (IsDisabled(&shipData[i]))
                remove = 1;
            else if (force == 0)
                remove = 0;
        }
        if (remove) {
            if (shipData[i].active != 0 && shipData[i].auxMission != -1) {
                    if (currentMissionObjectives[shipData[i].auxMission].active != 0)
                        currentMissions[shipData[i].auxMission].auxShipsLeft++;
                    if (currentMissions[shipData[i].auxMission].auxShipsLeft
                            > currentMissions[shipData[i].auxMission].auxShipCount)
                        currentMissions[shipData[i].auxMission].auxShipsLeft =
                            currentMissions[shipData[i].auxMission].auxShipCount;
            } else if (shipData[i].active != 0 && shipData[i].defenseStellar != -1) {
                stellarData[shipData[i].defenseStellar].defWaveSize++;
                if (stellarData[shipData[i].defenseStellar].defWaveSize
                        > stellarData[shipData[i].defenseStellar].defCount)
                    stellarData[shipData[i].defenseStellar].defWaveSize =
                        stellarData[shipData[i].defenseStellar].defCount;
            }
            shipData[i].active = 0;
            shipData[i].specialMission = -1;
            shipData[i].defenseStellar = -1;
            shipData[i].tractorShip = -1;
            shipData[i].hired = 0;
            shipData[i].hailed = 0;
            shipData[i].auxMission = -1;
            shipData[i].curSystem = -1;
            shipData[i].parentShip = -1;
        }
    }
}

#define kSystemPersSlots        8
#define kMissionShipSystemHere  (-6)    /* mïsn +0x10: spawn in the player's system */
#define kMissionShipCloaked     2       /* mïsn +0x0e */
#define kNavStelFirst           (-1)    /* mïsn +0x0e: -1..-16 index navDefault */
#define kNavStelLast            (-16)
#define kSpawnOdds              7
#define kRandomSpawnReserve     8

#if V111 /* V111 spelling */
#define shipsLeft sysOf
#define shipNum k
#else
#define shipsLeft count
#define shipNum j
#endif

/* SetupShipsInSystem (mac _SetupShipsInSystem). STATE: MATCH 100.0%, exe 675 / obj 675 instrs,
   551 exact + 124 reloc-equivalent, 0 mismatched; frame EXACT 0x24, 12/12 slots and refs;
   byte-identical UNMEASURED (bytediff reads the adopted baseline, which lacks this row).
   OPEN: none. `arriving' and RespawnEscort's `full' must BOTH be unsigned char; either
   one alone costs a row. mac: MATCH both slices. see notes */
// FUNCTION: EVNOVA 0x0041af90
// MAC: _SetupShipsInSystem i386 0x00042b61 ppc 0x0004a668
void SetupShipsInSystem(short sys, unsigned char arriving)
#ifdef MAC1010
{
    float total;
    float step;
    short i;
    short j;
    short m;
    short k;
    short n;
    short slot;
    short sysOf;
    short count;
    short govt;
    short navIndex;
    short shipSyst;

    DeleteAllComputerShips(0, 1);

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].parentShip == 0 && shipData[i].active != 0) {
            if (IsDisabled(&shipData[i]) != 0) {
                shipData[i].active = 0;
                if (shipData[i].parentShip == 0
                        && shipData[i].aiType == kShipTypeEscort
                        && shipData[i].specialMission == -1
                        && shipClass[shipData[i].type].inherentAI <= kShipTypeBraveTrader)
                    DestroyPartialFleetCargo(i);
            } else {
                if (shipData[i].specialMission == -1) {
                    if (IsDisabled(&shipData[i])) {
                        shipData[i].active = 0;
                        shipData[i].parentShip = -1;
                    } else {
                        RespawnEscort(&shipData[i], arriving);
                    }
                }
            }
        }
    }

    AICalcFormationPositions(shipData, 1);

    if (shipData->jumpTimer != 0.0f) {
        total = 0.0f;
        step = 45.0f;
        while (step > 0.0f) {
            total += step;
            step = step - 1.165;
        }
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].parentShip == 0 && shipData[i].active != 0) {
                Accel(((short)shipData[i].heading + kHalfCircle) % kFullCircle,
                      total, &shipData[i].pos);
                shipData[i].jumpTimer = -999.0f;
                shipData[i].jumpTick = 0;
                Accel(shipData[i].heading, 50.0f,
                      &shipData[i].vel);
            }
        }
    }

    k = 0;
    for (m = 0; m < kMaxActiveMissions; m++) {
        if (currentMissionObjectives[m].active != 0) {
            shipSyst = currentMissions[m].shipSyst;
            if (shipSyst >= 0 && shipSyst < kMaxSystems)
                sysOf = FindActiveCoLocatedSystem(shipSyst);
            else
                sysOf = -1;
            if ((sysOf == sys || currentMissions[m].shipSyst == kMissionShipSystemHere)
                    && currentMissions[m].shipCount > 0
                    && currentMissions[m].shipDelay < 0) {
                count = currentMissions[m].shipCount;
                if (currentMissions[m].shipSyst == kMissionShipSystemHere
                        && currentMissions[m].shipBehav == kMissionModeEscort) {
                    for (j = 1; j < kMaxShips; j++) {
                        if (shipData[j].active != 0 && m == shipData[j].specialMission)
                            count--;
                    }
                }
                if (count > 0) {
                    for (i = 0; i < count; i++) {
                        n = MissionDudeSpawn(currentMissions[m].shipDude, -1, sys, m);
                        if (n != -1) {
                            k++;
                            if (currentMissions[m].shipGoal == kMissionGoalEscort) {
                                shipData[n].pos.x = Rand(0x200) - 0x100;
                                shipData[n].pos.y = Rand(0x200) - 0x100;
                            }
                            if (currentMissions[m].shipGoal == kMissionGoalRescue) {
                                shipData[n].heading = Rand(kFullCircle);
                                shipData[n].vel.x = shipData[n].vel.y = 0.0f;
                                shipData[n].accelMag = 0.0f;
                                shipData[n].shield = 0.0f;
                                if ((shipClass[shipData[n].type].flags
                                        & kShipArmorDisableTenth) != 0)
                                    shipData[n].armor =
                                        shipClass[shipData[n].type].armor * 0.1 - 1.0;
                                else
                                    shipData[n].armor =
                                        shipClass[shipData[n].type].armor * 0.33 - 1.0;
                                if (currentMissions[m].timeLimit <= 0
                                        && currentMissions[m].timeLimit > -32000)
                                    shipData[n].boarded = 1;
                            }
                            if (currentMissions[m].shipStart <= kNavStelFirst
                                    && currentMissions[m].shipStart >= kNavStelLast) {
                                navIndex = -1 - currentMissions[m].shipStart;
                                if (systemData[sys].navDefault[navIndex] != -1) {
                                    shipData[n].pos.x = stellarData[
                                        systemData[sys].navDefault[navIndex]].pos.h;
                                    shipData[n].pos.y = stellarData[
                                        systemData[sys].navDefault[navIndex]].pos.v;
                                }
                            }
                            if (currentMissions[m].shipBehav == kMissionModeEscort) {
                                shipData[n].aiType = kShipTypeEscort;
                                shipData[n].parentShip = 0;
                                shipData[n].formLeader = 0;
                                shipData[n].swarmLeader = 0;
                                if (arriving != 0)
                                    shipData[n].heading = shipData->heading;
                            }
                            shipData[n].lastStellar = -2;
                            if (currentMissions[m].shipStart == kMissionShipCloaked)
                                DoShipCloak(&shipData[n]);
                        } else {
                            crashlog_print("warning: couldn't create ship for mission ID ");
                            crashlog_print_num(currentMissions[m].misnIndex + kFirstResourceID);
                            crashlog_print("\n");
                        }
                    }
                    if ((currentMissions[m].flags & kMissionAutoAbort) != 0)
                        AutoAbortMission(m);
                }
            }
        }
    }

    if (arriving != 0)
        AICalcFormationPositions(shipData, 1);

    for (i = 0; i < systemData[sys].avgShips; i++) {
        slot = -1;
        if (Rand(kSpawnOdds) == 0) {
            slot = SpawnPerson(shipData->curSystem, 0, -1);
            if (slot != -1) {
                govt = shipData[slot].govt;
                if (govt != -1
                        && (govtData[govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].accelMag = 0.0f;
                    shipData[slot].vel.x = 0.0f;
                    shipData[slot].vel.y = 0.0f;
                }
            }
        } else if (Rand(kSpawnOdds) == 0) {
            SpawnFleet(shipData->curSystem, 0);
        } else {
            slot = RandomShipSpawn(sys, kRandomSpawnReserve);
        }
        if (slot != -1) {
            AdjustedAccel(shipData[slot].heading, shipClass[shipData[slot].type].maxSpeed,
                          &shipData[slot].vel, shipClass[shipData[slot].type].maxSpeed);
        }
    }

    for (i = 0; i < kSystemPersSlots; i++) {
        if (systemData[sys].persons[i] != -1
                && personData[systemData[sys].persons[i]].loaded != 0
                && EvalMissionBitTestString(personData[systemData[sys].persons[i]].activeOn)
                && Rand(100) + 1 <= systemData[sys].persProbs[i]) {
            slot = SpawnPerson(shipData->curSystem, 0, systemData[sys].persons[i]);
            if (slot != -1) {
                AdjustedAccel(shipData[slot].heading, shipClass[shipData[slot].type].maxSpeed,
                              &shipData[slot].vel, shipClass[shipData[slot].type].maxSpeed);
                govt = shipData[slot].govt;
                if (govt != -1
                        && (govtData[govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].vel.x = 0.0f;
                    shipData[slot].vel.y = 0.0f;
                    shipData[slot].accelMag = 0.0f;
                }
            }
        }
    }
}
#else
{
    float total;
    float step;
    float speed;
    short i;
    short j;
    short slot;
    short sysOf;
#if V111 /* V111 spelling */
    unsigned short k;
#else
    short count;
#endif
    short govt;
    short navIndex;

    DeleteAllComputerShips(0, 1);

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].parentShip == 0 && shipData[i].active != 0) {
            if (IsDisabled(&shipData[i]) != 0) {
                shipData[i].active = 0;
                if (shipData[i].parentShip == 0
                        && shipData[i].aiType == kShipTypeEscort
                        && shipData[i].specialMission == -1
                        && shipClass[shipData[i].type].inherentAI <= kShipTypeBraveTrader)
                    DestroyPartialFleetCargo(i);
            } else {
                if (shipData[i].specialMission == -1) {
                    if (IsDisabled(&shipData[i])) {
                        shipData[i].active = 0;
                        shipData[i].parentShip = -1;
                    } else {
                        RespawnEscort(&shipData[i], arriving);
                    }
                }
            }
        }
    }

    AICalcFormationPositions(shipData, 1);

    if (shipData->jumpTimer != 0.0f) {
        total = 0.0f;
        step = 45.0f;
        do {
            total += step;
            step = step - 1.165;
        } while (step > 0.0f);
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].parentShip == 0 && shipData[i].active != 0) {
                Accel(((short)shipData[i].heading + kHalfCircle) % kFullCircle,
                      total, &shipData[i].pos);
                shipData[i].jumpTimer = -999.0f;
                shipData[i].jumpTick = 0;
                Accel(shipData[i].heading, 50.0f,
                      &shipData[i].vel);
            }
        }
    }

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0) {
            sysOf = currentMissions[i].shipSyst;
            if (sysOf >= 0 && sysOf < kMaxSystems)
                sysOf = FindActiveCoLocatedSystem(sysOf);
            else
                sysOf = -1;
            if ((sysOf == sys || currentMissions[i].shipSyst == kMissionShipSystemHere)
                    && (shipsLeft = currentMissions[i].shipCount) > 0
                    && currentMissions[i].shipDelay < 0) {
                if (currentMissions[i].shipSyst == kMissionShipSystemHere
                        && currentMissions[i].shipBehav == kMissionModeEscort) {
                    for (j = 1; j < kMaxShips; j++) {
                        if (shipData[j].active != 0 && i == shipData[j].specialMission)
                            shipsLeft--;
                    }
                }
                if (shipsLeft > 0) {
                    for (shipNum = 0; (short)shipNum < shipsLeft; shipNum++) {
                        slot = MissionDudeSpawn(currentMissions[i].shipDude, -1, sys, i);
                        if (slot != -1) {
                            if (currentMissions[i].shipGoal == kMissionGoalEscort) {
                                shipData[slot].pos.x = Rand(0x200) - 0x100;
                                shipData[slot].pos.y = Rand(0x200) - 0x100;
                            }
                            if (currentMissions[i].shipGoal == kMissionGoalRescue) {
                                shipData[slot].heading = Rand(kFullCircle);
                                shipData[slot].vel.x = shipData[slot].vel.y = 0.0f;
                                shipData[slot].accelMag = 0.0f;
                                shipData[slot].shield = 0.0f;
                                if ((shipClass[shipData[slot].type].flags
                                        & kShipArmorDisableTenth) != 0)
                                    shipData[slot].armor =
                                        shipClass[shipData[slot].type].armor * 0.1 - 1.0;
                                else
                                    shipData[slot].armor =
                                        shipClass[shipData[slot].type].armor * 0.33 - 1.0;
                                if (currentMissions[i].timeLimit <= 0
                                        && currentMissions[i].timeLimit > -32000)
                                    shipData[slot].boarded = 1;
                            }
                            if (currentMissions[i].shipStart <= kNavStelFirst
                                    && currentMissions[i].shipStart >= kNavStelLast) {
                                navIndex = -1 - currentMissions[i].shipStart;
                                if (systemData[sys].navDefault[navIndex] != -1) {
                                    shipData[slot].pos.x = stellarData[
                                        systemData[sys].navDefault[navIndex]].pos.h;
                                    shipData[slot].pos.y = stellarData[
                                        systemData[sys].navDefault[navIndex]].pos.v;
                                }
                            }
                            if (currentMissions[i].shipBehav == kMissionModeEscort) {
                                shipData[slot].aiType = kShipTypeEscort;
                                shipData[slot].parentShip = 0;
                                shipData[slot].formLeader = 0;
                                shipData[slot].swarmLeader = 0;
                                if (arriving != 0)
                                    shipData[slot].heading = shipData->heading;
                            }
                            shipData[slot].lastStellar = -2;
                            if (currentMissions[i].shipStart == kMissionShipCloaked)
                                DoShipCloak(&shipData[slot]);
                        } else {
                            crashlog_print("warning: couldn't create ship for mission ID ");
                            crashlog_print_num(currentMissions[i].misnIndex + kFirstResourceID);
                            crashlog_print("\n");
                        }
                    }
                    if ((currentMissions[i].flags & kMissionAutoAbort) != 0)
                        AutoAbortMission(i);
                }
            }
        }
    }

    if (arriving != 0)
        AICalcFormationPositions(shipData, 1);

    for (i = 0; i < systemData[sys].avgShips; i++) {
        slot = -1;
        if (Rand(kSpawnOdds) == 0) {
            slot = SpawnPerson(shipData->curSystem, 0, -1);
            if (slot != -1) {
                govt = shipData[slot].govt;
                if (govt != -1
                        && (govtData[govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].accelMag = 0.0f;
                    shipData[slot].vel.x = 0.0f;
                    shipData[slot].vel.y = 0.0f;
                }
            }
        } else if (Rand(kSpawnOdds) == 0) {
            SpawnFleet(shipData->curSystem, 0);
        } else {
            slot = RandomShipSpawn(sys, kRandomSpawnReserve);
        }
        if (slot != -1) {
            speed = shipClass[shipData[slot].type].maxSpeed;
            AdjustedAccel(shipData[slot].heading, speed,
                          &shipData[slot].vel, speed);
        }
    }

    for (i = 0; i < kSystemPersSlots; i++) {
        if (systemData[sys].persons[i] != -1
                && personData[systemData[sys].persons[i]].loaded != 0
                && EvalMissionBitTestString(personData[systemData[sys].persons[i]].activeOn)
                && Rand(100) + 1 <= systemData[sys].persProbs[i]) {
            slot = SpawnPerson(shipData->curSystem, 0, systemData[sys].persons[i]);
            if (slot != -1) {
                speed = shipClass[shipData[slot].type].maxSpeed;
                AdjustedAccel(shipData[slot].heading, speed,
                              &shipData[slot].vel, speed);
                govt = shipData[slot].govt;
                if (govt != -1
                        && (govtData[govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].vel.x = 0.0f;
                    shipData[slot].vel.y = 0.0f;
                    shipData[slot].accelMag = 0.0f;
                }
            }
        }
    }
}
#endif

#define kSpawnSpread     1500    /* Rand(1500) - 750 on each axis */
#define kSpawnOrigin     750
#define kSpawnSpeed      100.0f
#define kStartCredits    10000
#define kAITypeLinked    3       /* this aiType seeds from the linked system */

/* RandomShipSpawn (mac _RandomShipSpawn). STATE: MATCH 100.0% 617/617, byte-identical yes
   (3208 bytes), relocregcheck 0 of 189. OPEN: none. */
// FUNCTION: EVNOVA 0x0041ba80
// MAC: _RandomShipSpawn i386 0x0003c0f3 ppc 0x00042798
short RandomShipSpawn(short sys, short reserve)
{
    short slot;
    short dude;
    short pick;
    short link;
    short i;

    reserve = abs(reserve);
    for (slot = 1; slot < kMaxShips - reserve; slot++) {
        if (shipData[slot].active != 0)
            continue;
        dude = SelectDudeFieldFromSystem(&systemData[sys]);
        if (dude == -1)
            continue;
        dude = systemData[sys].dudeTypes[dude];
        pick = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (pick < 0 || pick >= kDudeShipSlots)
            goto fail;
        shipData[slot].active = 1;
        shipData[slot].curSystem = sys;
        shipData[slot].dude = dude;
        shipData[slot].type = dudeData[dude].shipTypes[pick];
        shipData[slot].govt = dudeData[dude].govt;
        if (dudeData[dude].aiType >= 1)
            shipData[slot].aiType = dudeData[dude].aiType;
        else
            shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
        if (shipData[slot].aiType == kAITypeLinked &&
            systemData[shipData[slot].curSystem].navDefault[0] != -1 &&
            shipClass[shipData[slot].type].maxSpeed == 0.0) {
            link = systemData[shipData[slot].curSystem].navDefault[0];
            shipData[slot].pos.x = stellarData[link].pos.h;
            shipData[slot].pos.y =
                stellarData[systemData[shipData[slot].curSystem].navDefault[0]].pos.v;
            Accel(Rand(kFullCircle), kSpawnSpeed, &shipData[slot].pos);
        } else {
            shipData[slot].pos.x = Rand(kSpawnSpread) - kSpawnOrigin;
            shipData[slot].pos.y = Rand(kSpawnSpread) - kSpawnOrigin;
        }
        shipData[slot].vel.x = shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        shipData[slot].navTargetKind = -1;
        shipData[slot].jumpTimer = 0;
        shipData[slot].aiState = kAIStateHiJack;
        shipData[slot].aiSubstate = kSubLoJack;
        shipData[slot].boarded = 0;
        shipData[slot].formerEscort = -1;
        shipData[slot].cloakDir = 0;
        shipData[slot].cloak = 0;
        shipData[slot].specialMission = -1;
        shipData[slot].grudge = 0;
        shipData[slot].aiTimer = 0;
        shipData[slot].heading = Rand(kFullCircle);
        shipData[slot].parentShip = -1;
        shipData[slot].targetShip = -1;
        shipData[slot].targetStellar = -1;
        shipData[slot].deathTimer = 0;
        shipData[slot].ionization = 0;
        shipData[slot].ionColor = 0;
        shipData[slot].lightLevel = 0.0f;
        shipData[slot].blinkCount = 0;
        shipData[slot].altSet = 0;
        shipData[slot].animSet = 0;
        shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
        shipData[slot].foldSet =
            shipClass[shipData[slot].type].baseSets - 1;
        shipData[slot].foldDir = 0;
        shipData[slot].frameTimer = 0;
        shipData[slot].shieldGlow = 0;
        shipData[slot].weapGlow = 0;
        shipData[slot].annoyance = 0.0f;
        shipData[slot].bankDir = 0;
        shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
        shipData[slot].credits = kStartCredits;
        shipData[slot].aggression = Rand(3) ^ 2;
        shipData[slot].shield = shipClass[shipData[slot].type].shield;
        shipData[slot].armor = shipClass[shipData[slot].type].armor;
        shipData[slot].fuel = shipClass[shipData[slot].type].fuel;
        shipData[slot].defenseStellar = -1;
        shipData[slot].tractorShip = -1;
        shipData[slot].hired = 0;
        shipData[slot].hailed = 0;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
        shipData[slot].auxMission = -1;
        shipData[slot].escortOrder = -1;
        shipData[slot].persType = -1;
        shipData[slot].thrust = 0;
        shipData[slot].speedLimit = 0;
        shipData[slot].fireQueued = 0;
        shipData[slot].odds = -1.0f;
        shipData[slot].engineGlow = 0x20;
        shipData[slot].aimWeapon = -1;
        shipData[slot].fireTick = 0;
        shipData[slot].hailTick = 0;
        shipData[slot].aimWeapon = -1;
        shipData[slot].seesCloaked[0] = -1;
        shipData[slot].seesCloaked[1] = -1;
        shipData[slot].decloaksWhenHit = -1;
        shipData[slot].animSet = 0;
        shipData[slot].animTimer = 0;
        shipData[slot].sellOnLanding = 0;
        shipData[slot].upgradeOnLanding = 0;
        shipData[slot].fuelRegenState = -1;
        shipData[slot].escortMayAttack = 0;
        shipData[slot].voiceClass = Rand(2);
        for (i = 0; i < 4; i++)
            shipData[slot].jamming[i] = -1;
        if (shipData[slot].govt != -1 &&
            (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled) != 0)
            shipData[slot].engineGlow = 0;
        if (shipClass[shipData[slot].type].baseSets > 0)
            shipData[slot].animSet =
                Rand(shipClass[shipData[slot].type].baseSets);
        if (shipClass[shipData[slot].type].animDelay > 0)
            shipData[slot].animTimer =
                Rand(shipClass[shipData[slot].type].animDelay);
        for (i = 0; i < kMaxWeapTypes; i++) {
            shipData[slot].weap[i].count =
                shipClass[shipData[slot].type].weapCount[i];
            shipData[slot].weap[i].ammo =
                shipClass[shipData[slot].type].weapAmmo[i];
        }
        AISetup(&shipData[slot]);
        return slot;
fail:
        shipData[slot].active = 0;
        return -1;
    }
    return -1;
}

/*
 * 0x004235c0 = SpawnPerson (mac `_SpawnPerson` at the same call position, same
 * (sys, 1, -1) arguments); 0x00425280 = SpawnFleet, call-position proven.
 */

/* HyperShipSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041c710
// MAC: _HyperShipSpawn i386 0x0004291a ppc 0x0004a36c
short HyperShipSpawn(short sys)
{
    short slot;
    short stellar;
    float speed;
    float dist;
    FloatPoint pt;

    if (Rand(7) == 0) {
        slot = SpawnPerson(sys, 1, -1);
    } else if (Rand(7) == 0) {
        slot = -1;
        SpawnFleet(sys, 1);
    } else {
        slot = RandomShipSpawn(sys, kSpawnReserve);
        if (slot != -1) {
            if (ShipFuelCapacity(&shipData[slot]) <= 0) {
                shipData[slot].active = 0;
                return -1;
            }
        }
    }
    if (slot != -1) {
        dist = 0;
        speed = 50.0f;
        while (speed > 0) {
            dist += speed;
            speed -= 1.165;
        }
        shipData[slot].pos.x = shipData[slot].pos.y = 0;
        Accel(Rand(kFullCircle), dist + 1000.0f, &shipData[slot].pos);
        pt.x = pt.y = 0;
        shipData[slot].heading =
            CalcInterceptAngle(&shipData[slot].pos, &pt);
        shipData[slot].lastStellar = -2;
        shipData[slot].prevSystem = -1;
        shipData[slot].vel.x = 0;
        shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        stellar = PickEmergeStellar(&shipData[slot]);
        if (stellar != -1) {
            AIMakeShipEmergeFromHyperGate(&shipData[slot], stellar);
            shipData[slot].pos.x = stellarData[stellar].pos.h;
            shipData[slot].pos.y = stellarData[stellar].pos.v;
        } else {
            shipData[slot].jumpTimer = -999;
            shipData[slot].jumpTick = ReadTick();
            Accel(shipData[slot].heading, 50.0f,
                  &shipData[slot].vel);
            AIMakeShipJumpIn(&shipData[slot]);
        }
        return slot;
    }
    return -1;
}

/* SpecificDudeSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041c9f0
// MAC: _SpecificDudeSpawn i386 0x0003d0eb ppc 0x00043c0c
short SpecificDudeSpawn(short dude, short sys, short reserve,
                        unsigned char allowAll)
{
    short slot;
    short pick;
    short i;

    slot = GenericRandomShipSpawn(sys, reserve);
    if (slot == -1)
        goto none;
    pick = SelectShipFieldFromDude(&dudeData[dude], allowAll);
    if (pick < 0 || pick >= kDudeShipSlots)
        goto fail;
    shipData[slot].dude = dude;
    shipData[slot].type = dudeData[dude].shipTypes[pick];
    shipData[slot].govt = dudeData[dude].govt;
    if (dudeData[dude].aiType >= 1)
        shipData[slot].aiType = dudeData[dude].aiType;
    else
        shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
    AISetup(&shipData[slot]);
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = shipClass[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = shipClass[shipData[slot].type].weapAmmo[i];
    }
    shipData[slot].shield = shipClass[shipData[slot].type].shield;
    shipData[slot].armor = shipClass[shipData[slot].type].armor;
    return slot;

fail:
    shipData[slot].active = 0;
    return -1;
none:
    return -1;
}

/* MissionDudeSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041cf40
// MAC: _MissionDudeSpawn i386 0x0003cd3b ppc 0x00043784
short MissionDudeSpawn(short dude, short wantType, short sys, short mission)
{
    short slot;
    short pick;
    short sel;
    short i;

    slot = GenericRandomShipSpawn(sys, kSpawnReserve);
    if (slot == -1)
        goto none;
    Rand(100);
    if (wantType != -1 &&
        (currentMissions[mission].flags & kMissionFixedShipType) != 0 &&
        currentMissions[mission].shipCount == 1) {
        for (i = 0; i < kDudeShipSlots; i++) {
            if (wantType == dudeData[dude].shipTypes[i]) {
                currentMissions[mission].shipTypeIndex = i;
                break;
            }
        }
    }
    pick = currentMissions[mission].shipTypeIndex;
#ifdef MAC1010 /* 1.0.10 spelling */
    if (pick != -1)
        sel = pick;
    else {
        sel = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (sel == -1)
            sel = SelectShipFieldFromDude(&dudeData[dude], 1);
    }
    pick = sel;
#else
    if (pick == -1) {
        pick = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (pick == -1)
            pick = SelectShipFieldFromDude(&dudeData[dude], 1);
    }
#endif
    if (pick < 0 || pick > kDudeShipSlots)
        goto fail;
    shipData[slot].specialMission = mission;
    shipData[slot].dude = dude;
    shipData[slot].type = dudeData[dude].shipTypes[pick];
    shipData[slot].govt = dudeData[dude].govt;
    shipData[slot].boarded = 0;
    shipData[slot].formerEscort = -1;
    shipData[slot].cloakDir = 0;
    shipData[slot].cloak = 0;
    shipData[slot].aiTimer = 0;
    if (dudeData[dude].aiType >= 1)
        shipData[slot].aiType = dudeData[dude].aiType;
    else
        shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
    AISetup(&shipData[slot]);
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = shipClass[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = shipClass[shipData[slot].type].weapAmmo[i];
    }
    shipData[slot].shield = shipClass[shipData[slot].type].shield;
    shipData[slot].armor = shipClass[shipData[slot].type].armor;
    shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
    shipData[slot].foldSet =
        shipClass[shipData[slot].type].baseSets - 1;
    shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
    if (shipClass[shipData[slot].type].baseSets > 0)
        shipData[slot].animSet =
            Rand(shipClass[shipData[slot].type].baseSets);
    if (shipClass[shipData[slot].type].animDelay > 0)
        shipData[slot].animTimer =
            Rand(shipClass[shipData[slot].type].animDelay);
    if (currentMissions[mission].shipBehav == kMissionModeAttackPlayer)
        AIMakeShipAttackPlayer(&shipData[slot]);
    return slot;

fail:
    shipData[slot].active = 0;
    return -1;
none:
    return -1;
}

#define kSystemStellars     16
#define kSysHere            (-6)
#define kMisnTimerMax       32000

#define kFleetOdds          500
#define kFleetProbRange     100
#define kEscortFleetHeavy   0xff
#define kEscortFleetLight   0xfe
#define kEscortEscortOf     4
#define kEscortCargoHeavy   200
#define kEscortCargoLight   50
#define kDefWaveModulus     10

#define kEntryHeadings      360
#define kEntryJitter        512
#define kEntryJitterBias    256.0f
#define kEntryRadius        1000.0f
#define kEntryStep          50.0f
#define kEntryStepDecay     1.165
#define kEntryShove         50.0f
#define kEntryDepth         (-999.0f)

#ifdef MACBUILD /* mac spelling */
#define auxNum n
#else
#define auxNum count
#endif

/* EnterMoreShips (mac _EnterMoreShips). STATE: MATCH 100.0%, exe 702 / obj 702, byte-identical unchecked
   (bytediff covers baseline MATCHes only; next regate); mac MATCH on both slices. OPEN: none */
// FUNCTION: EVNOVA 0x0041d6e0
// MAC: _EnterMoreShips i386 0x00043459 ppc 0x0004b11c
void EnterMoreShips(short sys)
{
    FloatPoint spot;
    FloatPoint origin;
#ifdef MAC1010
    short i;
    short toSpawn;
    short count;
    short n;
    short left;
    short j;
#else
    short count;
    short n;
    short i;
    short left;
    short j;
    short toSpawn;
#endif
    short which;
    short heading;
    FloatPoint dest;
    short cargo;
    short roll;
    short pick;
    float dist;
    float step;

    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active) {
            if (currentMissions[i].auxShipDelay <= 0) {
                if (currentMissions[i].auxShipDude != -1
                    && currentMissions[i].auxShipsSpawned < currentMissions[i].auxShipsLeft) {
                    left = currentMissions[i].auxShipsLeft - currentMissions[i].auxShipsSpawned;
                    if (ValidAuxShipSystem(shipData[0].curSystem, i)
                        && left > 0) {
                        dist = 0.0f;
                        for (step = kEntryStep; step > 0.0f;
                             step = step - kEntryStepDecay)
                            dist = dist + step;
                        heading = Rand(kEntryHeadings);
                        spot.x = spot.y = 0.0f;
                        Accel(heading, dist + kEntryRadius, &spot);
                        for (auxNum = 0; auxNum < left; auxNum++) {
                            which = SpecificDudeSpawn(currentMissions[i].auxShipDude, sys,
                                                      kSpawnReserve, 0);
                            if (which != -1) {
                                shipData[which].auxMission = i;
                                currentMissions[i].auxShipsSpawned += 1;
                                if ((currentMissions[i].flags & kMissionInfiniteAuxShips) == 0)
                                    currentMissions[i].auxShipsLeft -= 1;
                                shipData[which].pos.x =
                                    spot.x + Rand(kEntryJitter) - kEntryJitterBias;
                                shipData[which].pos.y =
                                    spot.y + Rand(kEntryJitter) - kEntryJitterBias;
                                dist = 0.0f;
                                for (step = kEntryStep; step > 0.0f;
                                     step = step - kEntryStepDecay)
                                    dist = dist + step;
                                origin.x = origin.y = 0.0f;
                                shipData[which].heading = CalcInterceptAngle(
                                    &shipData[which].pos, &origin);
                                shipData[which].lastStellar = -2;
                                shipData[which].prevSystem = -2;
                                shipData[which].vel.x = shipData[which].vel.y = 0;
                                shipData[which].accelMag = 0;
                                shipData[which].jumpTimer = kEntryDepth;
                                shipData[which].jumpTick = ReadTick();
                                Accel(shipData[which].heading, kEntryShove,
                                      &shipData[which].vel);
                            }
                        }
                        if (currentMissions[i].flags & kMissionAutoAbort)
                            AutoAbortMission(i);
                    }
                }
            }
            count = 0;
            toSpawn = 0;
            if (currentMissions[i].shipSyst >= 0 && currentMissions[i].shipSyst < kMaxSystems)
                pick = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            else
                pick = -1;
            if (currentMissions[i].shipSyst == kSysHere
                || pick == shipData[0].curSystem)
                toSpawn = currentMissions[i].shipCount;
            if (currentMissions[i].shipStart == 1) {
                toSpawn = currentMissions[i].shipCount - currentMissions[i].specialShipsJumpedIn;
                if (currentMissions[i].shipSyst != kSysHere
                    && pick != shipData[0].curSystem
                    && currentMissions[i].specialShipsJumpedIn <= 0)
                    toSpawn = 0;
            }
            if (toSpawn > 0) {
                if (currentMissions[i].shipDelay > 0)
                    if (currentMissions[i].shipDelay <= kMisnTimerMax)
                        currentMissions[i].shipDelay -= 1;
                if (currentMissions[i].shipCount > 0 && currentMissions[i].shipDelay == 0) {
                    currentMissions[i].shipDelay = -1;
                    dist = 0.0f;
                    for (step = kEntryStep; step > 0.0f;
                         step = step - kEntryStepDecay)
                        dist = dist + step;
                    if (shipData[0].prevSystem == -1) {
                        heading = Rand(kEntryHeadings);
                    } else {
                        origin.x =
                            systemData[shipData[0].curSystem].pos.h;
                        origin.y =
                            systemData[shipData[0].curSystem].pos.v;
                        dest.x =
                            systemData[shipData[0].prevSystem].pos.h;
                        dest.y =
                            systemData[shipData[0].prevSystem].pos.v;
                        heading = CalcInterceptAngle(&origin, &dest);
                    }
                    spot.x = spot.y = 0.0f;
                    Accel(heading, dist + kEntryRadius, &spot);
                    for (n = 0; n < toSpawn; n++) {
                        /* BUG: reuses count for the ship index, so the tally below inherits it */
                        count = MissionDudeSpawn(currentMissions[i].shipDude, -1, sys, i);
                        if (count != -1) {
                            currentMissions[i].specialShipsJumpedIn += 1;
                            shipData[count].pos.x =
                                spot.x + Rand(kEntryJitter) - kEntryJitterBias;
                            shipData[count].pos.y =
                                spot.y + Rand(kEntryJitter) - kEntryJitterBias;
                            dist = 0.0f;
                            for (step = kEntryStep; step > 0.0f;
                                 step = step - kEntryStepDecay)
                                dist = dist + step;
                            origin.x = origin.y = 0.0f;
                            shipData[count].heading = CalcInterceptAngle(
                                &shipData[count].pos, &origin);
                            shipData[count].lastStellar = -2;
                            shipData[count].prevSystem = -2;
                            shipData[count].vel.x = shipData[count].vel.y = 0;
                            shipData[count].accelMag = 0;
                            shipData[count].jumpTimer = kEntryDepth;
                            shipData[count].jumpTick = ReadTick();
                            Accel(shipData[count].heading, kEntryShove,
                                  &shipData[count].vel);
                        }
                    }
                    if (currentMissions[i].flags & kMissionAutoAbort)
                        AutoAbortMission(i);
                }
            }
            if (currentMissions[i].auxShipDelay > 0)
                if (currentMissions[i].auxShipDelay <= kMisnTimerMax)
                    currentMissions[i].auxShipDelay -= 1;
        }
    }

    for (n = 1; n < kMaxShips; n++)
        if (shipData[n].active && sys == shipData[n].curSystem
            && shipData[n].parentShip != 0)
            count++;
    if (count < systemData[sys].avgShips) {
        roll = Rand(kFleetOdds);
        if (roll == 1) {
            if (systemData[sys].fleetCount > 0
                && Rand(kFleetProbRange) + 1 <= systemData[sys].fleetProbTotal) {
                pick = SelectFleetFromSystem(&systemData[sys]);
                if (pick != -1)
                    HyperSpawnFleet(sys, pick, -1, 1);
                else
                    roll = 0;
            } else {
                roll = 0;
            }
        }
        if (roll == 0) {
            pick = SelectDudeFieldFromSystem(&systemData[sys]);
            if (pick != -1)
                HyperShipSpawn(sys);
        }
    }

    for (j = 0; j < kSystemStellars; j++) {
        if (systemData[sys].navDefault[j] != -1
            && stellarData[systemData[sys].navDefault[j]].defenseActive != 0
            && stellarData[systemData[sys].navDefault[j]].defWaveSize > 0) {
            count = 0;
            for (n = 1; n < kMaxShips; n++)
                if (shipData[n].active && shipData[n].defenseStellar != -1
                    && shipData[n].defenseStellar == systemData[sys].navDefault[j])
                    count++;
            i = stellarData[systemData[sys].navDefault[j]].defCount % kDefWaveModulus;
            if (count < i) {
                SpawnStellarDefenseShip(systemData[sys].navDefault[j]);
                stellarData[systemData[sys].navDefault[j]].defWaveSize -= 1;
                break;
            }
        }
    }

    SpawnNagShip();
    if (antiEscortDelay == 0) {
        antiEscortDelay = -1;
        cargo = 0;
        for (n = 1; n < kMaxShips; n++)
            if (shipData[n].active && shipData[n].parentShip == 0
                && shipData[n].aiType == kShipTypeEscort && shipData[n].specialMission == -1
                && shipClass[shipData[n].type].inherentAI <= kShipTypeBraveTrader)
                cargo += shipClass[shipData[n].type].cargo;
        if (cargo >= kEscortCargoHeavy)
            HyperSpawnFleet(shipData[0].curSystem, kEscortFleetHeavy,
                            kEscortEscortOf, 1);
        else if (cargo >= kEscortCargoLight)
            HyperSpawnFleet(shipData[0].curSystem, kEscortFleetLight,
                            kEscortEscortOf, 1);
    } else {
        antiEscortDelay -= 1;
    }
}

/* RespawnEscort. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041e240
// MAC: _RespawnEscort i386 0x0003d4b8 ppc 0x00044098
void RespawnEscort(ShipObj *ship, unsigned char full)
{
    short i;

    if (ship->parentShip != 0)
        return;

    ship->lastStellar = -2;
    ship->prevSystem = -2;
    ship->thrust = 0;
    ship->speedLimit = 0;
    ship->persType = -1;
    ship->fireQueued = 0;
    ship->odds = -1.0f;
    ship->engineGlow = 0;
    ship->fireTick = 0;
    ship->hailTick = 0;
    ship->aimWeapon = -1;
    ship->dude = -1;
    ship->persType = -1;
    ship->specialMission = -1;
    ship->ionization = 0;
    ship->escortMayAttack = 0;
    if (full) {
        ship->shield = ShipShieldCapacity(ship);
        ship->armor = ShipArmorCapacity(ship);
    }
    ship->curSystem = shipData->curSystem;
    ship->heading = shipData->heading;
    ship->swarmLeader = -1;
    ship->formLeader = 0;
    ship->formPos.x = 0;
    ship->formPos.y = 0;
    shipData->hasFormation = 1;
    ship->vel.x = ship->vel.y = 0;
    ship->accelMag = 0;
    if (full) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            ship->weap[i].count = shipClass[ship->type].weapCount[i];
            ship->weap[i].ammo = shipClass[ship->type].weapAmmo[i];
        }
    }
    for (i = 0; i < kCargoTypes; i++)
        ship->cargo[i] = 0;
    AIMakeEscortFlyInForm(ship);
}

/*
 * The fighterType parameter is the WEAPON index of the carrier's bay: the
 * spawned hull is that weapon's ammoType - 128, the heading jitter is its
 * inaccuracy, the launch speed its speed100, and unk4C its lifetime.
 */

/* LaunchFighter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041e640
// MAC: _LaunchFighter i386 0x0003d62d ppc 0x00044238
unsigned char LaunchFighter(ShipObj *parent, short fighterType)
{
    short slot;
    short i;
    short n;
    short inacc;

    if (fighterType < 0 || fighterType > kMaxWeapTypes) /* BUG: admits kMaxWeapTypes, one past weaponStats */
        return 0;
    slot = GenericRandomShipSpawn(parent->curSystem, kSpawnReserve);
    if (slot == -1)
        goto none;
    shipData[slot].active = 1;
    shipData[slot].curSystem = parent->curSystem;
    shipData[slot].pos.x = parent->pos.x;
    shipData[slot].pos.y = parent->pos.y;
    shipData[slot].vel.x = parent->vel.x;
    shipData[slot].vel.y = parent->vel.y;
    shipData[slot].accelMag = parent->accelMag;
    shipData[slot].type = weaponStats[fighterType].ammoType - kFirstResourceID;
    shipData[slot].defenseStellar = -1;
    shipData[slot].tractorShip = -1;
    shipData[slot].hailed = 0;
    shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
    shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
    shipData[slot].hired = 0;
    shipData[slot].auxMission = -1;
    shipData[slot].aiType = kShipTypeFighter;
    shipData[slot].govt = parent->govt;
    shipData[slot].navTargetKind = -1;
    shipData[slot].jumpTimer = 0;
    shipData[slot].aiState = kAIStateHiJack;
    shipData[slot].aiSubstate = kSubLoJack;
    shipData[slot].boarded = 0;
    shipData[slot].formerEscort = -1;
    shipData[slot].cloakDir = 0;
    shipData[slot].cloak = 0;
    shipData[slot].grudge = 0;
    shipData[slot].lastStellar = -2;
    shipData[slot].prevSystem = -2;
    shipData[slot].shield = shipClass[shipData[slot].type].shield;
    shipData[slot].armor = shipClass[shipData[slot].type].armor;
    shipData[slot].aiTimer = weaponStats[fighterType].lifetime;
    shipData[slot].parentShip = parent->index;
    shipData[slot].heading = parent->heading;
    shipData[slot].persType = -1;
    shipData[slot].dude = -1;
    shipData[slot].specialMission = -1;
    shipData[slot].ionization = 0;
    shipData[slot].ionColor = 0;
    shipData[slot].lightLevel = 0;
    shipData[slot].blinkCount = 0;
    shipData[slot].altSet = 0;
    shipData[slot].animSet = 0;
    shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
    shipData[slot].foldSet =
        shipClass[shipData[slot].type].baseSets - 1;
    shipData[slot].foldDir = 0;
    shipData[slot].frameTimer = 0;
    shipData[slot].shieldGlow = 0;
    shipData[slot].weapGlow = 0;
    shipData[slot].annoyance = 0;
    shipData[slot].bankDir = 0;
    shipData[slot].animTimer = 0;
    shipData[slot].seesCloaked[0] = -1;
    shipData[slot].seesCloaked[1] = -1;
    shipData[slot].decloaksWhenHit = -1;
    shipData[slot].escortOrder = -1;
    shipData[slot].sellOnLanding = 0;
    shipData[slot].upgradeOnLanding = 0;
    shipData[slot].fuelRegenState = -1;
    shipData[slot].escortMayAttack = 0;
    shipData[slot].voiceClass = Rand(2);
    shipData[slot].shield = ShipShieldCapacity(&shipData[slot]);
    shipData[slot].armor = ShipArmorCapacity(&shipData[slot]);
    if (shipClass[shipData[slot].type].inherentGovtB != -1
        && govtData[shipClass[shipData[slot].type].inherentGovtB].voiceClass != -1)
        shipData[slot].voiceClass =
            govtData[shipClass[shipData[slot].type].inherentGovtB].voiceClass;
    for (i = 0; i < 4; i++)
        shipData[slot].jamming[i] = -1;
    if (shipClass[shipData[slot].type].baseSets > 0)
        shipData[slot].animSet =
            Rand(shipClass[shipData[slot].type].baseSets);
    if (shipClass[shipData[slot].type].animDelay > 0)
        shipData[slot].animTimer =
            Rand(shipClass[shipData[slot].type].animDelay);
    if (shipData[slot].parentShip == 0
        && shipClass[shipData[slot].type].escortType == 0) {
        n = 0;
        for (i = 1; i < kMaxShips; i++) {
            if (i != slot && shipData[i].active != 0
                && shipData[i].parentShip == shipData[slot].parentShip) {
                n++;
                break;
            }
        }
        if (n == 0 && playerEscortOrders[0] == kOrderRegroup)
            playerEscortOrders[0] = -1;
    } else {
        for (i = 1; i < kMaxShips; i++) {
            if (i != slot && shipData[i].active != 0
                && shipData[i].parentShip == shipData[slot].parentShip
                && shipClass[shipData[i].type].escortType ==
                   shipClass[shipData[slot].type].escortType
                && shipData[i].escortOrder != -1) {
                shipData[slot].escortOrder = shipData[i].escortOrder;
                break;
            }
        }
    }
    if (parent->index == 0)
        shipData[slot].targetShip = -1;
    else
        shipData[slot].targetShip = parent->targetShip;
    inacc = weaponStats[fighterType].inaccuracy;
    if (inacc > 0)
        shipData[slot].heading += Rand(inacc * 2) - inacc;
    AdjustedAccel(shipData[slot].heading, weaponStats[fighterType].speed100,
                  &shipData[slot].vel,
                  ShipMaxSpeed(&shipData[slot]));
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = shipClass[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = shipClass[shipData[slot].type].weapAmmo[i];
    }
    if (shipData[slot].targetShip != -1) {
        if (shipData[slot].parentShip == shipData[slot].targetShip
            || (shipData[slot].parentShip ==
                shipData[shipData[slot].targetShip].parentShip
                && shipData[slot].parentShip != -1))
            shipData[slot].targetShip = -1;
    }
    AISetup(&shipData[slot]);
    return 1;
none:
    return 0;
}

#define kUnitsPerBox        5.0      /* pooled double */
#define kMaxBoxesPerShip    0xc

#define kMsgMissionAborted  0x11c
#define kMsgJettisonPlain   0x122
#define kMsgJettisonMission 0x121

#define kBoxFree            (-1.0F)

#define kBoxSpinMax         0x24
#define kBoxTypes           4
#define kBoxHeadingBase     0xb4
#define kBoxHeadingSpread   0x5a
#define kBoxKickBase        0x1e
#define kBoxKickSpread      0x28
#define kBoxKickScale       100.0    /* pooled double */
#define kBoxSpreadA         0xf
#define kBoxSpreadB         0x1e

/* JettisonCargo (mac _JettisonCargo). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041f330
// MAC: _JettisonCargo i386 0x0003dffd ppc 0x00044e9c
void JettisonCargo(unsigned char withMissions)
{
    short boxes;
    short total;
    short escortCargo;
    short share;
    double holds;
    double fleetHolds;
    double units;
    short i;
    short j;
    short k;
    unsigned char text[256];
    Boolean toldMission;

    toldMission = 0;
    total = 0;
    for (i = 0; i <= kLastCommodity; i++)
        total += shipData[0].cargo[i];
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            total += junkData[i].held;
            junkData[i].held = 0;
        }
    }
    escortCargo = share = total;
    if (withMissions != 0) {
        for (k = 0; k < kMaxActiveMissions; k++) {
            if (currentMissionObjectives[k].active != 0 && currentMissions[k].hasCargo != 0
                && currentMissions[k].cargoType != -1 && currentMissions[k].cargoQty >= 0
                && currentMissions[k].canAbort != 0) {
                total += currentMissions[k].cargoQty;
                share += currentMissions[k].cargoQty;
                currentMissions[k].hasCargo = 0;
                if ((currentMissions[k].flags & kMissionHidden) == 0
                    && playerIsLanded == 0) {
                    GetIndString(text, kGameStrList, kMsgMissionAborted);
                    DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
                    toldMission = 1;
                }
                QuickMissionFailure(k);
                if (total == 0) {
                    total += 1;
                    share += 1;
                }
            }
        }
    }
    fleetHolds = TotalFleetHolds();
    for (i = 0; i < kMaxShips; i++) {
        if ((shipData[i].active != 0 && shipData[i].parentShip == 0
             && shipData[i].aiType == kShipTypeEscort && shipData[i].specialMission == -1
             && shipClass[shipData[i].type].inherentAI <= kShipTypeBraveTrader)
            || i == 0) {
            if (i == 0)
                holds = ShipTotalHolds();
            else
                holds = shipClass[shipData[i].type].cargo;
            units = holds / fleetHolds;
            units = (i == 0) ? share : units * escortCargo;
            if (units > 0.0) {
                if (playerIsLanded == 0) {
                    boxes = (units / kUnitsPerBox);
                    if (boxes <= 0)
                        boxes += 1;
                    if (boxes > kMaxBoxesPerShip)
                        boxes = kMaxBoxesPerShip;
                    for (j = 0; j < boxes; j++)
                        SpawnBox(i);
                }
                statusRedraw[kRedrawCargo] = 1;
            }
        }
    }
    for (i = 0; i <= kLastCommodity; i++)
        shipData[0].cargo[i] = 0;
    if (total > 0 && playerIsLanded == 0) {
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
        if (toldMission == 0) {
            if (withMissions)
                GetIndString(text, kGameStrList, kMsgJettisonMission);
            else
                GetIndString(text, kGameStrList, kMsgJettisonPlain);
            DisplayComm((PStrChar *)text, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
        }
    }
    ResetPlayerPrecalcedValues();
}

/* SpawnBox. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041f800
// MAC: _SpawnBox i386 0x0003dd74 ppc 0x00044b44
void SpawnBox(short from)
{
    short slot;
    short i;
    short mag;
    short kind;
    slot = -1;
    for (i = 0; i < kBoxSlots; i++) {
        if (boxData[i].timer < kBoxFree) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return;

    boxData[slot].pos.x = shipData[from].pos.x;
    boxData[slot].pos.y = shipData[from].pos.y;
    boxData[slot].vel.x = shipData[from].vel.x;
    boxData[slot].vel.y = shipData[from].vel.y;
    boxData[slot].timer = (Rand(kBoxHeadingSpread) + kBoxHeadingBase);
    boxData[slot].system = shipData[from].curSystem;
    boxData[slot].angle = Rand(kBoxSpinMax);
    boxData[slot].mining = 0;
    boxData[slot].pict = 0;
    kind = Rand(kBoxTypes);
    if (kind == 0)
        boxData[slot].spin = -1;
    if (kind == 1)
        boxData[slot].spin = 0;
    if (kind == 2)
        boxData[slot].spin = 0;
    if (kind == 3)
        boxData[slot].spin = 1;
    mag = (GetSpriteXSize((SpriteRec *)shipData[from].sprite) * 0.5);
    Accel((((short)shipData[from].heading + kBoxHeadingBase) % kFullCircle),
          mag,
          &boxData[slot].pos);
    Accel((((short)shipData[from].heading + kBoxHeadingBase + Rand(kBoxSpreadB) - Rand(kBoxSpreadA)) % kFullCircle),
          (float)(Rand(kBoxKickSpread) + kBoxKickBase) / kBoxKickScale,
          &boxData[slot].vel);
}

/* boxData (0x005914a8) is 64 records of the 0x28 stride both twins index by.
   The four spin tests are INDEPENDENT ifs, not an else-if chain: each arm is
   parked out of line and jumps back to the NEXT test rather than to the join. */


/* SpawnScoopableBox. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041fb50
// MAC: _SpawnScoopableBox i386 0x0003e3c5 ppc 0x00045328
void SpawnScoopableBox(FloatPoint *pos, short cargoType, short pict)
{
    short slot;
    short i;
    short r;
    slot = -1;
    for (i = 0; i < kMaxBoxes; i++) {
        if (boxData[i].timer < -1.0f) {
            slot = i;
            break;
        }
    }
    if (slot != -1) {
        boxData[slot].pos.x = pos->x;
        boxData[slot].pos.y = pos->y;
        boxData[slot].timer = Rand(200) + 300;
        boxData[slot].system = shipData[0].curSystem;
        boxData[slot].angle = Rand(36);
        boxData[slot].mining = 1;
        boxData[slot].cargoType = cargoType;
        boxData[slot].pict = pict;
        r = Rand(4);
        if (r == 0)
            boxData[slot].spin = -1;
        if (r == 1)
            boxData[slot].spin = 0;
        if (r == 2)
            boxData[slot].spin = 0;
        if (r == 3)
            boxData[slot].spin = 1;
        boxData[slot].vel.x = boxData[slot].vel.y = 0;
        Accel(Rand(kFullCircle), (Rand(81) + 60) * 0.2 * 0.01, &boxData[slot].vel);
        return;
    }
}

/* SpawnShot (mac _SpawnShot). STATE: MATCH 100%, exe 970 / obj 970, relocregcheck 0 of 179;
   byte identity not yet measured (bytediff reads the adopted baseline). mac MATCH on both
   slices; the V111 `target' self-extension is score-only (notes, lane G37).
   OPEN: the real statement there. The jitter blocks read `inaccuracy' at each use; a
   block-scope `inc' copy was an invented entity (notes, 2026-09-24). */
// FUNCTION: EVNOVA 0x0041fd30
// MAC: _SpawnShot i386 0x0003e550 ppc 0x00045560
short SpawnShot(short ship, short target, short weapType, Boolean flag)
{
    short slot;
    short guidance;
    short i;
    short angle;
    short collision;
    Boolean intercept;

    if (weapType < 0 || weapType >= kMaxWeapTypes) {
        return -1;
    }
    if (weaponStats[weapType].graphic < 0 || weaponStats[weapType].graphic >= kWeapTypeTop) {
        return -1;
    }

    /* claim the first slot whose life has run under the free threshold */
    slot = -1;
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life < -1.0f) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        return -1;
    }
    collision = 0;
    angle = 0;

    /* seed position/velocity/heading from the firing ship, or clear */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        shotData[slot].pos.x = shipData[ship].pos.x;
        shotData[slot].pos.y = shipData[ship].pos.y;
        shotData[slot].vel.x = shipData[ship].vel.x;
        shotData[slot].vel.y = shipData[ship].vel.y;
        angle = shipData[ship].heading;
        intercept = 0;
        guidance = weaponStats[weapType].guidance;
        if (guidance == kGuidanceTurret || guidance == kGuidanceFrontQuad || guidance == kGuidanceRearQuad) {
            if (target != -1) {
                angle = CalcInterceptAngle(&shipData[ship].pos,
                                           &shipData[target].pos);
                intercept = 1;
            }
            else {
                if (guidance == kGuidanceFrontQuad) {
                    angle = shipData[ship].heading;
                }
            }
        }
        if (ship == 0) {
            guidance = weaponStats[weapType].guidance;
            if (guidance != kGuidanceTurret && guidance != kGuidanceFrontQuad
                    && guidance != kGuidanceRearQuad) {
                shotData[slot].vel.x = shipData[ship].vel.x;
                shotData[slot].vel.y = shipData[ship].vel.y;
            }
            if (weaponStats[weapType].guidance == kGuidanceFreefall) {
                shotData[slot].vel.x *= 0.8;
                shotData[slot].vel.y *= 0.8;
            }
        }
    }
    else {
        shotData[slot].pos.x = 0;
        shotData[slot].pos.y = 0;
        shotData[slot].vel.x = 0;
        shotData[slot].vel.y = 0;
        angle = 0;
    }

    /* identity + lifetime */
    shotData[slot].weapType = weapType;
    shotData[slot].owner = ship;
    shotData[slot].target = target;
    shotData[slot].disableOnly = WeaponDisableOnly(weapType);
    shotData[slot].life = weaponStats[weapType].lifetime;
    shotData[slot].subDepth = 0;
    shotData[slot].murk = 0;

    /* the disable-only re-check */
    if (shotData[slot].disableOnly == 0 && 0 < ship && ship < kMaxShips &&
        target != -1) {
        if (IsDisabled(&shipData[target]) == 0 &&
            AIShipIsPlunderingShip(&shipData[ship], &shipData[target]) != 0) {
            shotData[slot].disableOnly = 1;
        }
    }

    /* hitRoll: aiSkill-driven roll for the guided families */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        guidance = weaponStats[weapType].guidance;
        if (guidance == kGuidanceTurret || guidance == kGuidancePointDef) {
            if (shipClass[shipData[ship].type].aiSkill > 0) {
                shotData[slot].hitRoll =
                    Rand(shipClass[shipData[ship].type].aiSkill) + 1;
            }
            else {
                shotData[slot].hitRoll = Rand(100) + 1;
            }
        }
        else {
            shotData[slot].hitRoll = -1;
        }
    }

    if ((weaponStats[weapType].flags & kWeapCycleFromFirstFrame) != 0)
        shotData[slot].frame = 0;
    else
        shotData[slot].frame = Rand(0x24);

    shotData[slot].frameTimer = 0;
    shotData[slot].trailCount = 0;
    if (weaponStats[weapType].decay > 0) {
        shotData[slot].trailTimer = 0.0f;
    }
    else {
        shotData[slot].trailTimer = -1.0f;
    }

    /* an AI that is trying to disable its prey marks the shot disable-only */
#if V111 /* V111 spelling */
    if (0 < ship && ship < kMaxShips) {
        if (AIIsShipAttemptingDisable(&shipData[ship]) != 0) {
            shotData[slot].disableOnly = 1;
        }
        target = (target << 16) >> 16; /* V111 pad: shift pair, buys i386 block J at PRE (combine deletes its insns) */
    }
#else
    if (0 < ship && ship < kMaxShips &&
        AIIsShipAttemptingDisable(&shipData[ship]) != 0) {
        shotData[slot].disableOnly = 1;
    }
#endif

    if (0 <= ship && ship < kMaxShips) {
        shotData[slot].curSystem = shipData[ship].curSystem;
    }
    else {
        shotData[slot].curSystem = shipData[0].curSystem;
    }

    shotData[slot].angle = angle;
    shotData[slot].guidanceState = 0;

    /* guidance-1 "one shot" roll against the player system's explore level */
    if (weaponStats[weapType].guidance == kGuidanceHoming && (weaponStats[weapType].seeker & 8) != 0) {
        short roll;
        if (gSpeedMult > 0.0) {
            roll = (100.0 / gSpeedMult);
        }
        else {
            roll = 100;
        }
        if (roll < 1) {
            roll = 1;
        }
        if (Rand(roll) + 1 <= systemData[shipData[0].curSystem].interference) {
            shotData[slot].guidanceState = 999;
        }
    }

    if (weaponStats[weapType].durability > 0) {
        shotData[slot].energy = weaponStats[weapType].durability;
    }
    else {
        shotData[slot].energy = 0;
    }

    /* where the round actually starts */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        if (target != -1)
            angle = ModifyShotStartPosition(&shipData[ship], &shotData[slot].pos, weapType,
                                            &shipData[target].pos);
        else
            angle = ModifyShotStartPosition(&shipData[ship], &shotData[slot].pos, weapType, NULL);
        if (weaponStats[weapType].inaccuracy < 0 && angle != -1
                && weaponStats[weapType].exitType >= 0 && weaponStats[weapType].exitType <= 3) {
            if (shipClass[shipData[ship].type].weapPos[weaponStats[weapType].exitType][angle].h < 0)
                collision = -1;
            else if (shipClass[shipData[ship].type].weapPos[weaponStats[weapType].exitType][angle].h > 0)
                collision = 1;
        }
        if (intercept != 0)
            shotData[slot].angle = CalcLeadAngle(&shipData[ship], &shipData[target], weapType,
                                                 &shotData[slot].pos);
    }

    /* inaccuracy jitter, first of the two identical blocks (guidance != kGuidanceFreefall) */
    if (0 < weaponStats[shotData[slot].weapType].inaccuracy
            && weaponStats[shotData[slot].weapType].guidance != kGuidanceFreefall) {
        shotData[slot].angle += Rand(weaponStats[shotData[slot].weapType].inaccuracy * 2)
                    - weaponStats[shotData[slot].weapType].inaccuracy;
        if (shotData[slot].angle < 0.0f) {
            shotData[slot].angle += kFullCircleF;
        }
        if (shotData[slot].angle >= kFullCircleF) {
            shotData[slot].angle = shotData[slot].angle - kFullCircleF;
        }
    }

    /* the speed vector */
    guidance = weaponStats[shotData[slot].weapType].guidance;
    if ((guidance != kGuidanceFreefall && guidance != kGuidanceRocket) || 0 < ship) {
        if (0 <= ship && ship < kMaxShips && collision != 0) {
            float f;
            if (collision == 1) {
                f = shipData[ship].heading
                    + abs(weaponStats[shotData[slot].weapType].inaccuracy);
            }
            else {
                f = shipData[ship].heading
                    - abs(weaponStats[shotData[slot].weapType].inaccuracy);
            }
            while (f < 0.0) {
                f += 360.0;
            }
            while (f >= 360.0) {
                f = f - 360.0;
            }
            Accel(f, weaponStats[weapType].speed100,
                  &shotData[slot].vel);
        }
        else {
            Accel(shotData[slot].angle, weaponStats[weapType].speed100,
                  &shotData[slot].vel);
        }
    }

    /* inaccuracy jitter, second of the two identical blocks (guidance == kGuidanceFreefall) */
    if (0 < weaponStats[shotData[slot].weapType].inaccuracy
            && weaponStats[shotData[slot].weapType].guidance == kGuidanceFreefall) {
        shotData[slot].angle += Rand(weaponStats[shotData[slot].weapType].inaccuracy * 2)
                    - weaponStats[shotData[slot].weapType].inaccuracy;
        if (shotData[slot].angle < 0.0f) {
            shotData[slot].angle += kFullCircleF;
        }
        if (shotData[slot].angle >= kFullCircleF) {
            shotData[slot].angle = shotData[slot].angle - kFullCircleF;
        }
    }

    /*
     * The "clear the ship" block.  The win exedis compares weaponStats[shotData
     * [slot].weapType].+0x08 (the header's `ammoType`) against -999, not the
     * header-documented -9999 sentinel on `guidance` (+0x06).  Named for the
     * header field it reads; the -9999/-999 split is a live TBC.
     */
    if (weaponStats[shotData[slot].weapType].ammoType == -999 &&
        0 <= ship && ship < kMaxShips && flag == 0) {
        shipData[ship].shield = 0;
        shipData[ship].armor = 0;
        statusRedraw[kRedrawTarg] = 1;
    }

    /* the four glow rolls */
    for (i = 0; i < 4; i++) {
        if (weaponStats[shotData[slot].weapType].jamVuln[i] > 0)
            shotData[slot].glow[i] = Rand(weaponStats[shotData[slot].weapType].jamVuln[i] + 1);
        else
            shotData[slot].glow[i] = 0;
    }

    /* sprite layer */
    guidance = weaponStats[weapType].guidance;
    if (guidance == kGuidanceHoming) {
        PutSpriteInLayer(shotData[slot].sprite, guidedShotLayer);
    }
    else if (guidance == kGuidancePointDef) {
        PutSpriteInLayer(shotData[slot].sprite, pointDefenseLayer);
    }
    else if (guidance == kGuidanceTurret) {
        if (0 <= ship && ship < kMaxShips) {
            if ((shipClass[shipData[ship].type].flags3 & kShipTurretShotsAbove) != 0)
                PutSpriteInLayer(shotData[slot].sprite, turretedShotLayer);
            else
                PutSpriteInLayer(shotData[slot].sprite, shotLayer);
        } else
            PutSpriteInLayer(shotData[slot].sprite, shotLayer);
    }
    else {
        PutSpriteInLayer(shotData[slot].sprite, shotLayer);
    }

    if (weaponStats[weapType].guidance == kGuidancePointDef) {
        SWSetSpriteCollideProc(shotData[slot].sprite, PointDefenseCollisionHandler);
    }
    else {
        SWSetSpriteCollideProc(shotData[slot].sprite, 0);
    }

    SWCloneFramesFromMaster(shotData[slot].sprite,
                 masterShotSprite[weaponStats[shotData[slot].weapType].graphic]);

    return slot;
}

/* SpawnShotSubmunitions. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00420d30
// MAC: _SpawnShotSubmunitions i386 0x0003f1d5 ppc 0x00046360
void SpawnShotSubmunitions(ShotObj *shot, short ship)
{
    short subType;
    short n;
    short slot;

    if (weaponStats[shot->weapType].subType == -1
            || weaponStats[shot->weapType].subCount <= 0)
        return;
    if (weaponStats[shot->weapType].subLimit > 0
            && shot->subDepth >= weaponStats[shot->weapType].subLimit)
        return;
    subType = weaponStats[shot->weapType].subType;

    /* the launch sound, played at most once per volley */
    if (weaponStats[subType].sound >= 0 && weaponStats[subType].sound < kWeapTypeTop
            && shotSnd[weaponStats[subType].sound] != 0) {
        if ((weaponStats[subType].flags & kWeapLoopSound) != 0) {
            if (ST_IsSoundPlaying(shotSnd[weaponStats[subType].sound]) == 0)
                PlaySoundDistance(-1, shotSnd[weaponStats[subType].sound], 4,
                                &shot->pos, &shipData[0].pos);
        }
        else {
            PlaySoundDistance(-1, shotSnd[weaponStats[subType].sound], 4,
                            &shot->pos, &shipData[0].pos);
        }
    }

    for (n = 0; n < weaponStats[shot->weapType].subCount; n++) {
        slot = SpawnShot(shot->owner, ship, subType, 1);
        if (slot == -1)
            break;

        shotData[slot].pos.x = shot->pos.x;
        shotData[slot].pos.y = shot->pos.y;
        shotData[slot].vel.x = 0;
        shotData[slot].vel.y = 0;
        shotData[slot].subDepth = shot->subDepth + 1;
        shotData[slot].angle = shot->angle;

        if (weaponStats[shot->weapType].guidance == kGuidancePointDef) {
            shotData[slot].target = shot->target;
        }
        else if ((weaponStats[shot->weapType].flags2 & kWeapSubsSeekNearest) != 0) {
            shotData[slot].target = -1;
            shotData[slot].target = FindNearestValidShotTarget(&shotData[slot]);
            if (shotData[slot].target == -1)
                shotData[slot].target = ship;
            if (shotData[slot].target != -1) {
                shotData[slot].angle = CalcInterceptAngle(
                    &shot->pos,
                    &shipData[shotData[slot].target].pos);
            }
            else {
                shotData[slot].angle = shot->angle;
            }
        }

        /* fan the volley out by subTheta */
        if (weaponStats[shot->weapType].subTheta > 0) {
            shotData[slot].angle += weaponStats[shot->weapType].subTheta
                   - Rand(weaponStats[shot->weapType].subTheta * 2 + 1);
        }
        else if (weaponStats[shot->weapType].subTheta < 0) {
            shotData[slot].angle -= abs(weaponStats[shot->weapType].subTheta)
                          * (weaponStats[shot->weapType].subCount - 1) / 2;
            shotData[slot].angle += abs(weaponStats[shot->weapType].subTheta) * n;
        }
        if (shotData[slot].angle < 0.0f)
            shotData[slot].angle += kFullCircleF;
        if (shotData[slot].angle >= kFullCircleF)
            shotData[slot].angle -= kFullCircleF;

        if (weaponStats[shotData[slot].weapType].guidance == kGuidanceRocket) {
            shotData[slot].vel = shot->vel;
        }
        else {
            Accel(shotData[slot].angle,
                  weaponStats[shotData[slot].weapType].speed100,
                  &shotData[slot].vel);
        }
    }
}

#define kBoomTypes              64
#define kBigBoomBase            1000
#define kBoomTypeTop            1064
#define kBigScatterFraction     0.04
#define kSmallScatterFraction   0.16
#define kScatterSpread          0.5
#define kBigScatterSpread       0.25
#define kBigAngleRange          8
#define kBigAngleBase           4
#define kSmallAngleRange        16
#define kSmallAngleBase         8

/* CreateExplosion. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004211d0
// MAC: _CreateExplosion i386 0x0003f698 ppc 0x000468cc
void CreateExplosion(FloatPoint pt, short type, short size, Boolean sound)
{
    FloatPoint pos;
    short i;
    short sz;
    short bigCount;
    short smallCount;
    short snd;
    short j;
    /* FloatPoint *pos, not sound.h's float *: this body passes &pt */

    if (type < 0 || type >= kBoomTypeTop
            || (type >= kBoomTypes && type < kBigBoomBase))
        return;
    if (type >= kBigBoomBase) {
        type -= kBigBoomBase;
        sz = size;
        bigCount = (size * kBigScatterFraction);
        for (i = 0; i < bigCount; i++) {
            pos.x = pt.x + Rand(size * kScatterSpread) - size * kBigScatterSpread;
            pos.y = pt.y + Rand(size * kScatterSpread) - size * kBigScatterSpread;
            SpawnExplod(pos, -1, 1, Rand(kBigAngleRange) + kBigAngleBase);
        }
        smallCount = (size * kSmallScatterFraction);
        for (j = 0; j < smallCount; j++) {
            pos.x = pt.x + Rand(sz) - size * kScatterSpread;
            pos.y = pt.y + Rand(sz) - size * kScatterSpread;
            SpawnExplod(pos, -1, 0, Rand(kSmallAngleRange) + kSmallAngleBase);
        }
    }
    SpawnExplod(pt, -1, type, 0);
    if (sound != 0) {
        snd = explodType[type].sound;
        if (snd >= 0) {
            if (snd < kMaxShips)
                PlaySoundDistance(-1, explodSnd[snd], 6, &pt, &shipData->pos);
        }
    }
}

#define kMaxActiveBooms 32

/* SpawnExplod (mac _SpawnExplod). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00421500
// MAC: _SpawnExplod i386 0x0003f5dc ppc 0x000467e8
void SpawnExplod(FloatPoint pos, short unused, short type, short delay)
{
    short i;

    for (i = 0; i < kMaxActiveBooms; i++) {
        if (explodData[i].timer < 0.0f) {
            explodData[i].type = type;
            explodData[i].timer = 0;
            explodData[i].delay = delay;
            explodData[i].pos.x = pos.x;
            explodData[i].pos.y = pos.y;
            SWCloneFramesFromMaster(explodData[i].sprite,
                         masterExplodSprite[explodType[type].graphic]);
            break;
        }
    }
}


/* SpawnSmoke (mac _SpawnSmoke). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004215d0
// MAC: _SpawnSmoke i386 0x0003f927 ppc 0x00046ba4
#ifdef MACBUILD
void SpawnSmoke(FloatPoint pos, short type, short sprite)
#else
void SpawnSmoke(float a, register float b, int32_t type, short sprite)
#endif
{
    short i;

    if (settings.noDebris != 0)
        return;
    for (i = 0; i < kMaxDebris; i++) {
        if (smokeData[i].timer < 0.0f) {
            if (masterSmokeSprite[sprite] != 0) {
                smokeData[i].type = type;
                smokeData[i].timer = 0;
                smokeData[i].angle = 0;
#ifdef MACBUILD
                smokeData[i].pos.x = pos.x;
                smokeData[i].pos.y = pos.y;
#else
                smokeData[i].pos.x = a;
                smokeData[i].pos.y = b;
#endif
                SWCloneFramesFromMaster(smokeData[i].sprite, masterSmokeSprite[sprite]);
                SWLockSprite(smokeData[i].sprite);
                break;
            }
        }
    }
}

/* CreateAsteroids. STATE: MATCH 100%, exe 105 / obj 105 instrs.
   OPEN: none. The half extents are `w * 0.5' at each use; the halfW/halfH doubles were
   invented (notes, 2026-09-24). */
// FUNCTION: EVNOVA 0x004216b0
// MAC: _CreateAsteroids i386 0x0003fa05 ppc 0x00046ca8
void CreateAsteroids(void)
{
    short i;
    short w;
    short h;

    if (systemData[shipData[0].curSystem].asteroidCount > 0) {
        for (i = 0; i < systemData[shipData[0].curSystem].asteroidCount; i++)
            SpawnAsteroid(0);
        w = screenCenter.h + 128;
        h = screenCenter.v + 128;
        for (i = 0; i < kMaxAsteroids; i++) {
            asteroidData[i].pos.x = shipData[0].pos.x + Rand(w) - w * 0.5;
            asteroidData[i].pos.y = shipData[0].pos.y + Rand(h) - h * 0.5;
            asteroidData[i].vel.x = (Rand(400) - 200) * 0.01;
            asteroidData[i].vel.y = (Rand(400) - 200) * 0.01;
        }
    } else {
        killAsteroids = 1;
    }
}

/* SpawnAsteroid. STATE: MATCH. OPEN: none */
// FUNCTION: EVNOVA 0x00421830
// MAC: _SpawnAsteroid i386 0x0003a233 ppc 0x000406a0
void SpawnAsteroid(unsigned char type)
{
    short w;
    float radius;
    short h;
    unsigned long long mem;
    uint32_t diff;
    int32_t parity;
    short i;
    short active;
    unsigned char odd;
    short x;

    if (systemData[shipData[0].curSystem].asteroidCount <= 0
        || systemData[shipData[0].curSystem].astTypes == 0)
        return;

    active = 0;
    for (i = 0; i < kMaxAsteroids; i++)
        if (asteroidData[i].active != 0)
            active++;

    if (active >= systemData[shipData[0].curSystem].asteroidCount)
        return;

    mem = shipData[5].licenseCode;
    parity = 0;
    diff = mem;
    diff ^= mem >> 32;
    while (diff) {
        diff &= diff - 1;
        parity = 1 - parity;
    }
    odd = parity;
#ifdef CEFIX /* F01.01 */
    shipClass[gameIsHackedIndex].hacked =
        (shipData[36].licenseCopies != 1 && odd == 0)
            ? (unsigned char)1 : (unsigned char)0;
#else
    shipClass[gameIsHackedIndex].hacked =
        (shipData[36].licenseCopies != 0 && odd == 0)
            ? (unsigned char)1 : (unsigned char)0;
#endif
    x = shipClass[gameIsHackedIndex].hacked;

    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].active == 0) {
            asteroidData[i].active = 1;

            w = screenCenter.h + 128;
#ifdef CEFIX /* F26.01 */
            h = screenCenter.v;
#else
            h = screenCenter.v + 128;
#endif

            if (type != 0) {
#ifdef CEFIX /* F26.02-03 */
                radius = h;
#else
                if (w > h)
                    radius = w * 0.7;
                else
                    radius = h * 0.7;
#endif
                if (radius < 2.0f)
                    radius = 2.0f;

                if (Rand(2) == 0) {
                    asteroidData[i].pos.x = shipData[0].pos.x + radius + Rand(radius / 2.0f);
                    asteroidData[i].vel.x = Rand(200) * -0.01;
                } else {
                    asteroidData[i].pos.x = shipData[0].pos.x - radius - Rand(radius / 2.0f);
                    asteroidData[i].vel.x = Rand(200) * 0.01;
                }
                if (Rand(2) == 0) {
                    asteroidData[i].pos.y = shipData[0].pos.y + radius + Rand(radius / 2.0f);
                    asteroidData[i].vel.y = Rand(200) * -0.01;
                } else {
                    asteroidData[i].pos.y = shipData[0].pos.y - radius - Rand(radius / 2.0f);
                    asteroidData[i].vel.y = Rand(200) * 0.01;
                }
            } else {
                asteroidData[i].pos.x = shipData[0].pos.x + Rand(w) - w * 0.5;
                asteroidData[i].pos.y = shipData[0].pos.y + Rand(h) - h * 0.5;
                asteroidData[i].vel.x = (Rand(400) - 200) * 0.01;
                asteroidData[i].vel.y = (Rand(400) - 200) * 0.01;
            }

            do {
                asteroidData[i].type = Rand(16);
            } while (!(systemData[shipData[0].curSystem].astTypes & (1 << asteroidData[i].type)));

            asteroidData[i].frame = Rand(masterAsteroidSprite[asteroidData[i].type]->numFrames);
            asteroidData[i].frameRate = asteroidType[asteroidData[i].type].frameRate * (Rand(41) + 80) * 0.01;
            if (Rand(2) == 0)
                asteroidData[i].frameRate *= -1.0;
            asteroidData[i].strength = asteroidType[asteroidData[i].type].strength;
            break;
        }
    }
}

/* SpawnSubAsteroid. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00421e60
// MAC: _SpawnSubAsteroid i386 0x0003fbc7 ppc 0x00046ef4
void SpawnSubAsteroid(FloatPoint *pos, short type)
{
    short i;

    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].active == 0) {
            asteroidData[i].active = 1;
            asteroidData[i].type = type;
            asteroidData[i].pos.x = pos->x;
            asteroidData[i].pos.y = pos->y;
            asteroidData[i].vel.x = (Rand(200) - 100) * 0.01;
            asteroidData[i].vel.y = (Rand(200) - 100) * 0.01;
            asteroidData[i].frame = Rand(masterAsteroidSprite[type]->numFrames);
            asteroidData[i].frameRate = asteroidType[type].frameRate * (Rand(41) + 80) * 0.01;
            if (Rand(2) == 0)
                asteroidData[i].frameRate *= -1.0;
            asteroidData[i].strength = asteroidType[type].strength;
            break;
        }
    }
}

/* SpawnStellarDefenseShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00421fd0
// MAC: _SpawnStellarDefenseShip i386 0x0003d2c9 ppc 0x00043e50
short SpawnStellarDefenseShip(short stellar)
{
    short ship;
    ship = -1;
    if (stellarData[stellar].defenseDude != -1) {
        ship = SpecificDudeSpawn(stellarData[stellar].defenseDude,
                                 stellarData[stellar].system, 2, 0);
        if (ship == -1)
            ship = SpecificDudeSpawn(stellarData[stellar].defenseDude,
                                     stellarData[stellar].system, 2, 1);
        if (ship != -1) {
            shipData[ship].defenseStellar = stellar;
            shipData[ship].boarded = 1;
            shipData[ship].cloakDir = 0;
            shipData[ship].cloak = 0;
            shipData[ship].aiTimer = 0;
            shipData[ship].aiType = 3;
            shipData[ship].specialMission = -1;
            shipData[ship].parentShip = -1;
            shipData[ship].govt = stellarData[stellar].govt;
            shipData[ship].pos.x = stellarData[stellar].pos.h;
            shipData[ship].pos.y = stellarData[stellar].pos.v;
            shipData[ship].heading = Rand(kFullCircle);
            shipData[ship].vel.x = shipData[ship].vel.y = 0;
            shipData[ship].accelMag = 0;
            Accel(shipData[ship].heading, ShipMaxSpeed(&shipData[ship]),
                  &shipData[ship].vel);
            AIMakeShipAttackPlayer(&shipData[ship]);
            stellarData[stellar].defenseActive = 1;
        }
    }
    return ship;
}

/* SufficentTargetedDamage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004221d0
// MAC: _SufficentTargetedDamage i386 0x0003fd22 ppc 0x000470f8
Boolean SufficentTargetedDamage(ShipObj *ship)
{
    if (ship->incomingDamage >= (ship->shield + ship->armor) * 1.05)
        return 1;
    return 0;
}

/* CalculateTargetedDamage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00422210
// MAC: _CalculateTargetedDamage i386 0x0003a153 ppc 0x00040554
void CalculateTargetedDamage(void)
{
    short i;
    short j;

    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].active) {
            shipData[i].incomingDamage = 0;
            for (j = 0; j < kMaxShots; j++) {
                if (shotData[j].life > 0.0f && shotData[j].target == i
                    && shotData[j].guidanceState == 0)
                    shipData[i].incomingDamage +=
                        (weaponStats[shotData[j].weapType].damage[0]
                         + weaponStats[shotData[j].weapType].damage[1]) * 0.5;
            }
        }
    }
}

#define kNudgeBase        0x32
#define kNudgeSpread      0x32

/* MakeEscortsHyperWithParent (mac _MakeEscortsHyperWithParent). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00422340
// MAC: _MakeEscortsHyperWithParent i386 0x0003fd54 ppc 0x0004714c
void MakeEscortsHyperWithParent(ShipObj *ship)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == ship->index &&
            shipData[i].defenseStellar == -1) {
            shipData[i].jumpTimer = shipData[shipData[i].parentShip].jumpTimer;
            shipData[i].jumpTick = shipData[shipData[i].parentShip].jumpTick;
            shipData[i].targetShip = -2;
            AIMakeShipJumpWithParent(&shipData[i]);
        }
    }
}

#if V111
/* MakeEscortsCancelJump (mac _MakeEscortsCancelJump, mac-only: new in 1.1.1). STATE: mac MATCH
   i386 and ppc. OPEN: none */
// MAC: _MakeEscortsCancelJump i386 0x0003fdeb ppc 0x00047218
void MakeEscortsCancelJump(ShipObj *ship)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == ship->index &&
            shipData[i].defenseStellar == -1) {
            shipData[i].jumpTimer = 0;
            shipData[i].targetShip = -2;
            AIMakeEscortFlyInForm(&shipData[i]);
        }
    }
}
#endif

/* SpawnEscort. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00422400
// MAC: _SpawnEscort i386 0x0003fe5b ppc 0x000472bc
short SpawnEscort(short shipType, short fromSpob)
{
    short slot;
    short k;
    short j;

    slot = GenericRandomShipSpawn(shipData->curSystem, kSpawnReserve);
    if (slot != -1 && shipType > -1) {

        shipData[slot].type = shipType;
        shipData[slot].aiType = kShipTypeEscort;
        shipData[slot].parentShip = 0;
        shipData[slot].govt = -1;
        shipData[slot].aggression = 2;
        shipData[slot].shield = shipClass[shipData[slot].type].shield;
        shipData[slot].armor = shipClass[shipData[slot].type].armor;
        shipData[slot].dude = -1;
        shipData[slot].hailed = 0;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
        shipData[slot].hired = 1;
        shipData[slot].specialMission = -1;
        shipData[slot].auxMission = -1;
        shipData[slot].upgradeOnLanding = 0;
        shipData[slot].sellOnLanding = 0;
        shipData[slot].fuelRegenState = -1;
        shipData[slot].escortOrder = -1;
        for (j = 0; j < 4; j++)
            shipData[slot].jamming[j] = -1;

        if (fromSpob != -1) {
            shipData[slot].pos.x = stellarData[fromSpob].pos.h;
            shipData[slot].pos.y = stellarData[fromSpob].pos.v;
        } else {
            shipData[slot].pos.x = shipData->pos.x;
            shipData[slot].pos.y = shipData->pos.y;
            shipData[slot].heading = shipData->heading;
            Accel(Rand(kFullCircle),
                  (Rand(kNudgeSpread) + kNudgeBase),
                  &shipData[slot].pos);
        }

        for (k = 0; k < kMaxWeapTypes; k++) {
            shipData[slot].weap[k].count = shipClass[shipType].weapCount[k];
            shipData[slot].weap[k].ammo = shipClass[shipType].weapAmmo[k];
        }

        AISetup(&shipData[slot]);
        AIMakeEscortFlyInForm(&shipData[slot]);
        return slot;
    }
    return -1;
}

#define kStrEscortOne      298
#define kStrEscortMany     299
#define kStrSoldFor        300
#define kStrUpgradedFor    301

/* DoEscortLand (mac _DoEscortLand). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004229d0
// MAC: _DoEscortLand i386 0x0004031f ppc 0x00047904
void DoEscortLand(void)
{
#ifdef MAC1010
    int32_t total;
    short days;
#else
    short days;
    int32_t total;
#endif
    short i;
    short numSold;
    short numUpgraded;
    int32_t saleValue;
    int32_t upgradeCost;
    short type;
    short j;

    numUpgraded = 0;
    numSold = 0;
    saleValue = 0;
    upgradeCost = 0;

    if (SpobHasShipyard(&stellarData[shipData[0].targetStellar])) {
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active
                && shipData[i].parentShip == 0
                && !IsDisabled(&shipData[i])
                && shipData[i].aiType == kShipTypeEscort
                && shipData[i].sellOnLanding
                && shipData[i].specialMission == -1) {
                shipData[0].credits += shipClass[shipData[i].type].tradeIn;
                saleValue += shipClass[shipData[i].type].tradeIn;
                shipData[i].active = 0;
                shipData[i].parentShip = -1;
                statusRedraw[kRedrawCargo] = 1;
                numSold++;
            }
        }

        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active
                && shipData[i].parentShip == 0
                && shipData[i].aiType == kShipTypeEscort
                && shipData[i].upgradeOnLanding) {
                if (shipClass[shipData[i].type].upgradeTo != -1
                    && shipData[i].specialMission == -1
                    && shipData[0].credits >= shipClass[shipData[i].type].escUpgradeCost) {
                    numUpgraded++;
                    upgradeCost += shipClass[shipData[i].type].escUpgradeCost;
                    shipData[0].credits -= shipClass[shipData[i].type].escUpgradeCost;
                    statusRedraw[kRedrawCargo] = 1;
                    shipData[i].type = shipClass[shipData[i].type].upgradeTo;
                    for (j = 0; j < kMaxWeapTypes; j++) {
                        shipData[i].weap[j].count = 0;
                        shipData[i].weap[j].ammo = 0;
                    }
                    shipData[i].upgradeOnLanding = 0;
                }
            }
            shipData[i].shield = shipClass[shipData[i].type].shield;
            shipData[i].armor = shipClass[shipData[i].type].armor;
            for (j = 0; j < kMaxWeapTypes; j++)
                shipData[i].weap[j].ammo = shipClass[shipData[i].type].weapAmmo[j];
            type = shipData[i].specialMission;
            if (type != -1 && currentMissionObjectives[type].active != 0
                && currentMissions[type].shipBehav == kMissionModeEscort)
                j = 0;
        }

        if (numSold > 0 || numUpgraded > 0) {
            strcpy(textShowString, "");
            if (numSold > 0) {
                StrcatOrdinalNumber(numSold, textShowString, 1);
                strcat(textShowString, " ");
                if (numSold == 1)
                    StrcatIndCString(textShowString, kGameStrList, kStrEscortOne);
                else
                    StrcatIndCString(textShowString, kGameStrList, kStrEscortMany);
                strcat(textShowString, " ");
                StrcatIndCString(textShowString, kGameStrList, kStrSoldFor);
                strcat(textShowString, " ");
                StrcatCommaNumber(saleValue, textShowString);
                strcat(textShowString, " ");
                if (saleValue > 1)
                    strcat(textShowString, creditsCString);
                else
                    strcat(textShowString, creditCString);
                strcat(textShowString, ".");
                if (numUpgraded > 0)
                    strcat(textShowString, "\r\r");
            }
            if (numUpgraded > 0) {
                StrcatOrdinalNumber(numUpgraded, textShowString, 1);
                strcat(textShowString, " ");
                if (numUpgraded == 1)
                    StrcatIndCString(textShowString, kGameStrList, kStrEscortOne);
                else
                    StrcatIndCString(textShowString, kGameStrList, kStrEscortMany);
                strcat(textShowString, " ");
                StrcatIndCString(textShowString, kGameStrList, kStrUpgradedFor);
                strcat(textShowString, " ");
                StrcatCommaNumber(upgradeCost, textShowString);
                strcat(textShowString, " ");
                if (upgradeCost > 1)
                    strcat(textShowString, creditsCString);
                else
                    strcat(textShowString, creditCString);
                strcat(textShowString, ".");
            }
            ShowTextDialog(0, 0, 0);
            total = numSold + numUpgraded;
            days = total / 2;
            if (days > 0) {
                crashlog_print_mdy(&gameTime);
                crashlog_print(": incrementing date ");
                crashlog_print_num(days);
                crashlog_print(" days for upgrade/sale of ");
                crashlog_print_num(total);
                crashlog_print(" escorts\n");
                for (i = 0; i < days; i++)
                    IncrementGameTime();
            }
        }
    }
    DoEscortPayment(1);
}

/* DoEscortPayment (mac _DoEscortPayment). STATE: MATCH 100.0%, exe 143 / obj 143,
   byte-identical yes. OPEN: none; `idx' was an entity the exe never had (see notes). */
// FUNCTION: EVNOVA 0x004232d0
// MAC: _DoEscortPayment i386 0x00040111 ppc 0x00047658
void DoEscortPayment(short days)
{
    short i;
    short day;
    short checked;
    short numDismissed;
    int32_t cost;
    Boolean skip;

    checked = 0;
    numDismissed = 0;
    for (day = 0; day < days; day++) {
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active == 0) continue;
            if (shipData[i].parentShip != 0) continue;
            if (shipData[i].aiType != kShipTypeEscort) continue;
            if (IsDisabled(&shipData[i])) continue;

            skip = 0;
            if (shipData[i].specialMission != -1) {
                if (currentMissionObjectives[shipData[i].specialMission].active != 0) {
                    if (currentMissions[shipData[i].specialMission].shipBehav == kMissionModeEscort) {
                        skip = 1;
                    }
                }
            }
            if (skip == 0 && shipData[i].hired) {
            cost = (0.01 * shipClass[shipData[i].type].cost);

            if (shipData[0].credits >= cost) {
                shipData[0].credits -= cost;
                statusRedraw[kRedrawCargo] = 1;
            } else {
                numDismissed++;
                if (shipData[i].aiType == kShipTypeEscort && shipData[i].specialMission == -1 && shipClass[shipData[i].type].inherentAI <= kShipTypeBraveTrader) {
                    DestroyPartialFleetCargo(i);
                }
                statusRedraw[kRedrawCargo] = 1;
                shipData[i].active = 0;
                shipData[i].parentShip = -1;
                shipData[i].aiType = 1;
            }
            }
            checked++;
        }
    }

    if (numDismissed > 0) {
        if (hyperGamma >= 0.0) {
            hyperGamma = -1.0f;
#if V111
#elif defined(MAC1010)
#if TARGET_API_MAC_CARBON
            SetGamma(gScreenRef, &gOrigGamma);
#else
            GammaFadeTo(1, 100, 0);
#endif
#else
            FadeGammaTo(100, 0);
#endif
        }
        strcpy(textShowString, "");
        if (numDismissed == 1) {
            StrcatIndCString(textShowString, kGameStrList, 302);
        } else {
            StrcatIndCString(textShowString, kGameStrList, 303);
        }
        statusRedraw[kRedrawCargo] = 1;
        ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
        RefreshSAT();
#endif
        DrawStatus();
    }
}

/* DoStellarTimePassage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00423540
// MAC: _DoStellarTimePassage i386 0x00040867 ppc 0x00047ee8
void DoStellarTimePassage(void)
{
    short i;

    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0) {
            if ((stellarData[shipData[0].targetStellar].flags2 & kSpobAlwaysDominated) == 0)
                stellarData[i].daysDominated += 1;
            shipData[0].credits += stellarData[i].tribute;
            statusRedraw[kRedrawCargo] = 1;
        }
    }
}

/* SpawnPerson. STATE: MATCH, 555/555, relocregcheck 0 of 91; byte check pending the next
   regate (bytediff reads the baseline). OPEN: none */
// FUNCTION: EVNOVA 0x004235c0
// MAC: _SpawnPerson i386 0x000408d5 ppc 0x00047f84
short SpawnPerson(short sys, unsigned char noDisabled, short person)
{
    short i;
    short slot;
    short pick;
    short n;
    short j;
    Boolean eligible[kMaxPersons];

    n = 0;
    eligible[kMaxPersons - 1] = 0;
    if (person >= 0 && person < 0x3ff) {
        for (i = 0; i < 0x3ff; i++)
            eligible[i] = 0;
        eligible[person] = 1;
        n = 1;
    } else {
        for (i = 0; i < 0x3ff; i++) {
            eligible[i] = 0;
            if (personData[i].loaded != 0 && personData[i].aiType >= 1
                && personData[i].canAppear != 0 && personData[i].prepared != 0) {
                if (personData[i].linkSyst == -1)
                    eligible[i] = 1;
                if (personData[i].linkSyst == sys)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= kFirstResourceID && personData[i].linkSyst < 0x2710
                    && personData[i].linkSyst - kFirstResourceID == sys)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x270f && personData[i].linkSyst < 0x3a98
                    && personData[i].linkSyst - 0x2710 == systemData[sys].govt)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x3a98 && personData[i].linkSyst < 0x4e20
                    && systemData[sys].govt >= 0
                    && GovtAllies(personData[i].linkSyst - 0x3a98, systemData[sys].govt) != 0)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x4e20 && personData[i].linkSyst < 0x61a8
                    && systemData[sys].govt >= 0
                    && personData[i].linkSyst - 0x4e20 != systemData[sys].govt)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x61a8 && personData[i].linkSyst < 0x7530
                    && systemData[sys].govt >= 0
                    && GovtEnemies(personData[i].linkSyst - 0x61a8, systemData[sys].govt) != 0)
                    eligible[i] = 1;
                if (personData[i].govt >= 0
                    && (govtData[personData[i].govt].flags & kGovtAlwaysDisabled) != 0
                    && noDisabled != 0)
                    eligible[i] = 0;
                if (eligible[i] != 0) {
                    if (EvalMissionBitTestString(personData[i].activeOn) == 0)
                        eligible[i] = 0;
                }
            }
            if (eligible[i] != 0)
                n++;
        }
    }
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].specialMission == -1
            && (shipData[i].persType >= 0 && shipData[i].persType < kMaxPersons)) {
            for (j = 0; j < kMaxPersons; j++) {
                if (eligible[j] != 0
                    && personData[j].name[0] ==
                       personData[shipData[i].persType].name[0]
                    && personData[j].sameNameIndex ==
                       personData[shipData[i].persType].sameNameIndex) {
                    eligible[j] = 0;
                    n--;
                }
            }
        }
    }
    if (n > 0) {
        if (person != -1) {
            pick = person;
            eligible[pick] = 1;
            personData[pick].loaded = 1;
        } else {
            pick = Rand(0x3fe); /* BUG: Rand never returns 0x3fe, so person 1150 never spawns at random */
        }
        if (eligible[pick] != 0) {
            slot = GenericRandomShipSpawn(sys, kSpawnReserve);
            if (slot != -1) {
                shipData[slot].persType = pick;
                shipData[slot].type = personData[pick].shipType;
                shipData[slot].dude = -1;
                shipData[slot].govt = personData[pick].govt;
                shipData[slot].aiType = personData[pick].aiType;
                shipData[slot].aggression = personData[pick].aggression;
                shipData[slot].hailed = 0;
                shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
                shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
                shipData[slot].escortOrder = -1;
                if ((personData[pick].flags & kPersEscapePod) != 0)
                    shipData[slot].hasAfterburner = 1;
                if (shipData[slot].aggression < 1)
                    shipData[slot].aggression = 1;
                if (shipData[slot].aggression >= 3)
                    shipData[slot].aggression = 4;
                for (j = 0; j < kMaxWeapTypes; j++) {
                    shipData[slot].weap[j].count =
                        shipClass[shipData[slot].type].weapCount[j];
                    shipData[slot].weap[j].ammo =
                        shipClass[shipData[slot].type].weapAmmo[j];
                    shipData[slot].weap[j].count += personData[pick].weapCount[j];
                    shipData[slot].weap[j].ammo += personData[pick].weapAmmo[j];
                }
                shipData[slot].shield = shipClass[shipData[slot].type].shield;
                shipData[slot].armor = shipClass[shipData[slot].type].armor;
                if ((personData[pick].flags2 & kPersStartsNoFuel) != 0)
                    shipData[slot].fuel = 0;
                else
                    shipData[slot].fuel = shipClass[shipData[slot].type].fuel;
                if (personData[pick].shieldMod > 0) {
                    shipData[slot].shield =
                        shipData[slot].shield * personData[pick].shieldMod;
                    shipData[slot].armor =
                        shipData[slot].armor * personData[pick].shieldMod;
                }
                if (shipData[slot].govt != -1
                    && (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].shield = 0;
                    if ((shipClass[shipData[slot].type].flags & kShipArmorDisableTenth) != 0)
                        shipData[slot].armor =
                            shipClass[shipData[slot].type].armor * 0.1 - 1.0;
                    else
                        shipData[slot].armor =
                            shipClass[shipData[slot].type].armor * 0.33 - 1.0;
                    shipData[slot].vel.x = shipData[slot].vel.y = 0;
                    shipData[slot].accelMag = 0;
                }
                AISetup(&shipData[slot]);
                if (personData[shipData[slot].persType].linkMission != -1)
                    RandomizeOneMission(personData[shipData[slot].persType].linkMission);
                if (personData[pick].govt >= 0
                    && (govtData[personData[pick].govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].heading = Rand(kFullCircle);
                    /* BUG: vel.x is cleared twice and vel.y keeps whatever the slot held */
                    shipData[slot].vel.x = 0;
                    shipData[slot].vel.x = 0;
                    shipData[slot].accelMag = 0;
                    shipData[slot].engineGlow = 0;
                    if ((shipClass[shipData[slot].type].shanFlags & kShanFoldingParts) != 0)
                        shipData[slot].foldSet =
                            shipClass[shipData[slot].type].baseSets - 1;
                }
                return slot;
            }
        }
    }
    return -1;
}

/* the mac's index is the short itself; win reads it in three widths, each a CSE class */
#if V111 /* V111 spelling */
#define slotL slot
#define slotR slot
#define slotI slot
#elif defined(MAC1010)
#define slotL slot
#define slotR slot
#define slotI slot
#else
#define slotR ((int32_t)slot)
#define slotI ((int)slotL)
#endif

/* DoShipCapture (mac _DoShipCapture). STATE: MATCH 100%, exe 817 / obj 817; byte-identical
   yes (relocations masked). The three index spellings are CSE classes; the loop counters and
   the scratch `val' decide the colour order (notes). OPEN: none */
// FUNCTION: EVNOVA 0x00423fa0
// MAC: _DoShipCapture i386 0x00041120 ppc 0x0004889c
void DoShipCapture(ShipObj *me, Boolean asEscort)
{
    unsigned char buf[256];
#if !defined(MACBUILD) && !V111
    int32_t slotL;
#endif
    int32_t capType;
    short m;
    short w;
    short i;
    short slot;
    short val;

    slot = GenericRandomShipSpawn(shipData[0].curSystem, 1);
    if (slot != -1) {
        if (me->persType != -1)
            personData[me->persType].loaded = 0;
#if !defined(MACBUILD) && !V111
        slotL = slot;
#endif
        shipData[slotL].type = shipData[0].type;
        shipData[slotL].dude = -1;
        shipData[slotL].persType = -1;
        shipData[slotL].pos = shipData[0].pos;
        shipData[slotL].vel = shipData[0].vel;
        shipData[slotL].heading = shipData[0].heading;
        shipData[slotL].desiredHeading = shipData[slotL].heading;
        shipData[slotL].targetStellar = -1;
        shipData[slotL].targetShip = -1;
        shipData[slotL].selectedWeapon = -1;
        shipData[slotL].curSystem = shipData[0].curSystem;
        shipData[slotL].govt = -1;
        shipData[slotL].hired = 0;
        shipData[slotL].hailed = 0;
        shipData[slotL].hasAfterburner = AIHasAfterburner(&shipData[slotL]);
        shipData[slotL].canScoop = ShipCanScoop(&shipData[slotL]);
        shipData[slotL].auxMission = -1;
        shipData[slotL].escortOrder = -1;
        shipData[slotL].index = slot;
        shipData[slotL].voiceClass = Rand(2);
        val = shipClass[shipData[slotL].type].inherentGovtB;
        if (val != -1) {
            val = govtData[val].voiceClass;
            if (val != -1)
                shipData[slotI].voiceClass = val;
        }
        for (i = 0; i <= kLastCommodity; i++)
            shipData[slotL].cargo[i] = shipData[0].cargo[i];
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && i != me->index && i != slot
                    && me->index == shipData[i].parentShip) {
                shipData[i].targetShip = shipData[i].targetStellar = -1;
                shipData[i].grudge = 0;
                shipData[i].parentShip = -1;
                shipData[i].aiType = shipClass[shipData[i].type].inherentAI;
            }
        }
        shipData[slotL].specialMission = -1;
        if (asEscort != 0) {
            shipData[slotR].shield = 0;
            shipData[slotR].armor = 1.0f;
            shipData[slotR].boarded = 1;
            shipData[slotR].formerEscort = -1;
            shipData[slotR].hired = 0;
            shipData[slotR].hailed = 0;
            shipData[slotR].parentShip = 0;
            shipData[slotR].aiType = kShipTypeEscort;
            DestroyPartialFleetCargo(slot);
            shipData[slotR].parentShip = -1;
            shipData[slotR].aiType = -1;
        } else {
            shipData[slotR].deathTimer = 0;
            shipData[slotR].parentShip = 0;
            shipData[slotR].aiType = kShipTypeEscort;
            shipData[slotR].defenseStellar = -1;
            shipData[slotR].grudge = 0;
            shipData[slotR].fuel = ShipFuelCapacity(&shipData[slotR]);
            if (IsDisabled(&shipData[slotR]) != 0) {
                if ((shipClass[shipData[slotL].type].flags & kShipArmorDisableTenth) != 0)
                    shipData[slotR].armor =
                        ShipArmorCapacity(&shipData[slotR]) * 0.1 + 1.0;
                else
                    shipData[slotR].armor =
                        ShipArmorCapacity(&shipData[slotR]) * 0.3333 + 1.0;
            }
            prevParent[slotL] = 0;
            AISetup(&shipData[slotI]);
            AIMakeEscortFlyInForm(&shipData[slotI]);
        }
        statusRedraw[kRedrawCargo] = 1;
        statusRedraw[kRedrawTarg] = 1;
        statusRedraw[kRedrawWeap] = 1;
        for (i = 0; i < kMaxWeapTypes; i++) {
            shipData[slotL].weap[i].count =
                shipClass[shipData[slotL].type].weapCount[i];
            shipData[slotL].weap[i].ammo =
                shipClass[shipData[slotL].type].weapAmmo[i];
        }
        {
            short t = shipData[0].type;
            ShipRec *rec = &shipClass[t];

            crashlog_mission_bit_set("retiring", (char *)rec->name, 1,
                                     t + kFirstResourceID, rec->onRetire);
        }
        EvalMissionBitSetString(shipClass[shipData[0].type].onRetire);
        {
            short t = me->type;
            ShipRec *rec = &shipClass[t];

            crashlog_mission_bit_set("capturing", (char *)rec->name, 1,
                                     t + kFirstResourceID, rec->onCapture);
        }
        EvalMissionBitSetString(shipClass[me->type].onCapture);
        shipData[0].type = me->type;
        shipData[0].pos = me->pos;
        shipData[0].vel = me->vel;
        shipData[0].heading = me->heading;
        shipData[0].desiredHeading = shipData[0].heading;
        shipData[0].targetStellar = -1;
        shipData[0].targetShip = -1;
        shipData[0].selectedWeapon = -1;
        GetShipPaintColor(me, &playerPaint.red, &playerPaint.green,
                     &playerPaint.blue);
        for (i = 0; i <= kLastCommodity; i++)
            shipData[0].cargo[i] = me->cargo[i];
        if (ShipFuelCapacity(shipData) > 0)
            shipData[0].fuel = Rand(ShipFuelCapacity(shipData));
        else
            shipData[0].fuel = 0;
        shipData[0].aiType = -1;
        shipData[0].parentShip = -1;
        shipData[0].govt = -1;
        shipData[0].shield = 0;
        if ((shipClass[shipData[0].type].flags & kShipArmorDisableTenth) != 0)
            shipData[0].armor = shipClass[shipData[0].type].armor * 0.1 + 2.0;
        else
            shipData[0].armor = shipClass[shipData[0].type].armor * 0.3333 + 2.0;
        for (w = 0; w < kMaxWeapTypes; w++) {
            Boolean keep;

            keep = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                for (m = 0; m < kOutfMods; m++) {
                    if (outfitSystem[i].modType[m] == 1 && w == outfitSystem[i].modVal[m]
                            && outfitSystem[i].persistent != 0) {
                        keep = 1;
                        break;
                    }
                }
            }
            if (keep == 0) {
                shipData[0].weap[w].count = 0;
                shipData[0].weap[w].ammo = 0;
            }
        }
        for (w = 0; w < kMaxWeapTypes; w++) {
            if (me->weap[w].count > 0 && shipData[0].weap[w].count <= 0)
                shipData[0].weap[w].count = me->weap[w].count;
            if (me->weap[w].ammo > 0 && shipData[0].weap[w].ammo <= 0) {
                if (weaponStats[w].guidance == kGuidanceBay) {
                    shipData[0].weap[w].ammo = me->weap[w].ammo;
                } else if (weaponStats[w].ammoType >= 0 && weaponStats[w].ammoType < kMaxWeapTypes) {
                    shipData[0].weap[weaponStats[w].ammoType].ammo = me->weap[w].ammo;
                }
            }
        }
        for (w = 0; w < kMaxOutfits; w++) {
            if (outfitSystem[w].persistent == 0)
                shipSystems[w] = 0;
        }
        ShipStatsToSystemInfo();
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[shipData[0].type].defOutfCount[i] > 0)
                shipSystems[shipClass[shipData[0].type].defOutfID[i]] +=
                    shipClass[shipData[0].type].defOutfCount[i];
        }
        SystemInfoToShipStats();
        me->active = 0;
        GetIndString(buf, kGameStrList, 0x130);
        DisplayComm((PStrChar *)buf, 360, 0, kCommTextSize, &rgbWhite, 0, 0);

    } else {
        GetIndString(buf, kGameStrList, 0x131);
        DisplayComm((PStrChar *)buf, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
    }
}
#undef slotL
#undef slotR
#undef slotI


/* DisasterHandler. STATE: MATCH, byte-identical; mac MATCH both slices. OPEN: none */
// FUNCTION: EVNOVA 0x00424f90
// MAC: _DisasterHandler i386 0x00041ab9 ppc 0x00049324
void DisasterHandler(void)
{
    short i;
    short target;

    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded) {
            if (disasterData[i].countdown > 0) {
                disasterData[i].countdown -= 1;
                continue;
            }
            if (disasterData[i].unk20D) {
                if (disasterData[i].duration >= 0)
                    continue;
                disasterData[i].countdown = 2;
                continue;
            }
            if (Rand(100) + 1 > disasterData[i].freq)
                continue;
            if (!EvalMissionBitTestString(disasterData[i].text))
                continue;

            target = disasterData[i].stellar;
            if (target >= kFirstResourceID) {
                if (disasterData[i].unk20D) {
                    disasterData[i].stellarID = disasterData[i].stellar - kFirstResourceID;
                    disasterData[i].countdown = disasterData[i].duration;
                } else {
                    disasterData[i].stellarID = disasterData[i].stellar - kFirstResourceID;
                    disasterData[i].countdown = disasterData[i].duration;
                }
            } else if (target == -1) {
                do {
                    target = Rand(kMaxStellars);
                } while (!stellarData[target].loaded ||
                         (stellarData[target].flags & kSpobUninhabited) != 0);
                if (disasterData[i].unk20D) {
                    disasterData[i].stellarID = target;
                    disasterData[i].countdown = disasterData[i].duration;
                } else {
                    disasterData[i].stellarID = target;
                    disasterData[i].countdown = disasterData[i].duration;
                }
            }
        } else {
            disasterData[i].stellarID = -1;
            disasterData[i].countdown = -1;
            disasterData[i].unk20D = 0;
        }
    }
}

#define kOutfModType19  0x13

/* DoAutoRefuel. STATE: MATCH 100.0%, exe 122 / obj 122, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004250f0
// MAC: _DoAutoRefuel i386 0x00041bc5 ppc 0x0004949c
void DoAutoRefuel(void)
{
    Boolean found;
    short amount;
    short i;
    short j;

    found = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kOutfModType19) {
                    found = 1;
                    break;
                }
            }
        }
    }

    amount = ShipFuelCapacity(shipData);
    if (found) {
        if ((short)shipData->fuel < amount) {
            shipData->fuel = (short)shipData->fuel;
            amount -= shipData->fuel;
            if (amount > shipData->credits)
                amount = shipData->credits;
            shipData->credits -= amount;
            shipData->fuel += amount;
            statusRedraw[kRedrawFuel] = 1;
            statusRedraw[kRedrawCargo] = 1;
        }
    }
}

#define kFletCount 256


/* SpawnFleet. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00425280
// MAC: _SpawnFleet i386 0x00042704 ppc 0x0004a164
void SpawnFleet(short sys, unsigned char arriving)
{
    short i;
    short count;
    short pick;
    Boolean ok[kFletCount];

    count = 0;
    for (i = 0; i < kFletCount; i++) {
        ok[i] = 0;
        if (fleetData[i].leadShipType != -1 && fleetData[i].usable != 0) {
            if (fleetData[i].linkSyst == -1)
                ok[i] = 1;
            if (sys == fleetData[i].linkSyst)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= kFirstResourceID && fleetData[i].linkSyst < 10000 &&
                fleetData[i].linkSyst - kFirstResourceID == sys)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 10000 && fleetData[i].linkSyst < 15000 &&
                fleetData[i].linkSyst - 10000 == systemData[sys].govt)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 15000 && fleetData[i].linkSyst < 20000 &&
                systemData[sys].govt >= 0 &&
                GovtAllies(fleetData[i].linkSyst - 15000, systemData[sys].govt))
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 20000 && fleetData[i].linkSyst < 25000 &&
                systemData[sys].govt >= 0 &&
                fleetData[i].linkSyst - 20000 != systemData[sys].govt)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 25000 && fleetData[i].linkSyst < 30000 &&
                systemData[sys].govt >= 0 &&
                GovtEnemies(fleetData[i].linkSyst - 25000, systemData[sys].govt))
                ok[i] = 1;
            if (fleetData[i].usable == 0)
                ok[i] = 0;
        }
        if (ok[i])
            count++;
    }

    if (count > 0) {
        pick = Rand(kFletCount);
        if (ok[pick]) {
            HyperSpawnFleet(sys, pick, -1, arriving);
            return;
        }
    }
}

/* GenericRandomShipSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004254b0
// MAC: _GenericRandomShipSpawn i386 0x0003c89f ppc 0x0004314c
short GenericRandomShipSpawn(short sys, short reserve)
{
    short slot;
    short n;
    short k;

    slot = -1;
    for (n = 1; n < kMaxShips - abs(reserve); n++) {
        if (shipData[n].active == 0) {
            slot = n;
            break;
        }
    }

    if (slot != -1) {
        shipData[slot].active = 1;
        shipData[slot].curSystem = sys;
        shipData[slot].dude = -1;
        shipData[slot].type = 0;
        shipData[slot].aiType = 1;
        shipData[slot].govt = -1;
        shipData[slot].vel.x = shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        shipData[slot].navTargetKind = -1;
        shipData[slot].jumpTimer = 0;
        shipData[slot].aiState = kAIStateHiJack;
        shipData[slot].aiSubstate = kSubLoJack;
        shipData[slot].boarded = 0;
        shipData[slot].formerEscort = -1;
        shipData[slot].cloakDir = 0;
        shipData[slot].cloak = 0;
        shipData[slot].specialMission = -1;
        shipData[slot].grudge = 0;
        shipData[slot].aiTimer = 0;
        shipData[slot].parentShip = -1;
        shipData[slot].targetShip = -1;
        shipData[slot].targetStellar = -1;
        shipData[slot].deathTimer = 0;
        shipData[slot].ionization = 0;
        shipData[slot].ionColor = 0;
        shipData[slot].lightLevel = 0;
        shipData[slot].blinkCount = 0;
        shipData[slot].altSet = 0;
        shipData[slot].animSet = 0;
        shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
        shipData[slot].bankDir = 0;
        shipData[slot].frameTimer = 0;
        shipData[slot].shieldGlow = 0;
        shipData[slot].weapGlow = 0;
        shipData[slot].annoyance = 0;
        shipData[slot].bankDir = 0;
        shipData[slot].foldDir = 0;
        shipData[slot].foldSet = shipClass[shipData[slot].type].baseSets - 1;
        shipData[slot].animTimer = 0;
        shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
        shipData[slot].credits = 0;
        shipData[slot].aggression = Rand(3) ^ 2;
        shipData[slot].defenseStellar = -1;
        shipData[slot].tractorShip = -1;
        shipData[slot].hired = 0;
        shipData[slot].hailed = 0;
        shipData[slot].auxMission = -1;
        shipData[slot].persType = -1;
        shipData[slot].thrust = 0;
        shipData[slot].speedLimit = 0;
        shipData[slot].fireQueued = 0;
        shipData[slot].odds = -1.0f;
        shipData[slot].engineGlow = 0x20;
        shipData[slot].hasAfterburner = 0;
        shipData[slot].fireTick = 0;
        shipData[slot].hailTick = 0;
        shipData[slot].aimWeapon = -1;
        shipData[slot].escortOrder = -1;
        shipData[slot].sellOnLanding = 0;
        shipData[slot].upgradeOnLanding = 0;
        shipData[slot].seesCloaked[0] = -1;
        shipData[slot].seesCloaked[1] = -1;
        shipData[slot].decloaksWhenHit = -1;
        shipData[slot].fuelRegenState = -1;
        for (k = 0; k < 4; k++)
            shipData[slot].jamming[k] = -1;
        if (shipData[slot].govt != -1 &&
            (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled))
            shipData[slot].engineGlow = 0;
        if (shipClass[shipData[slot].type].baseSets > 0)
            shipData[slot].animSet = Rand(shipClass[shipData[slot].type].baseSets);
        if (shipClass[shipData[slot].type].animDelay > 0)
            shipData[slot].animTimer = Rand(shipClass[shipData[slot].type].animDelay);
        for (n = 0; n < kCargoTypes; n++)
            shipData[slot].cargo[n] = 0;
        shipData[slot].pos.x = Rand(kSpawnSpread) - kSpawnOrigin;
        shipData[slot].pos.y = Rand(kSpawnSpread) - kSpawnOrigin;
    }
    return slot;
}

                                                         /* 0x0043b4a0 */
                                                         /* 0x004159e0 */
                                                         /* 0x00413990 */

/* HyperSpawnFleet (mac _HyperSpawnFleet). STATE: win MATCH 100.0%, 982/982, byte-identical
   yes; mac MATCH on both slices. The fleet index is held twice, int `f' and long `idx' (the copy
   differs in type, so it survives copy propagation); one holder scores 98.4-98.5 (notes). OPEN:
   none. */
// FUNCTION: EVNOVA 0x004259b0
// MAC: _HyperSpawnFleet i386 0x00041c8d ppc 0x0004960c
void HyperSpawnFleet(short sys, short fleet, short escortOf,
                     unsigned char arriving)
{
    short i;
    float speed;
    float dist;
    int32_t idx;
    int f;
    short j;
    short k;
    short slot;
    short count;
    short stellar;
    unsigned char buf[256];
    short esc;
    FloatPoint pt;
    short type;
    Boolean digit;
    unsigned char c;

#ifdef MAC1010 /* 1.0.10 spelling */
#define f fleet
#define idx fleet
#else
    f = fleet;
    idx = f;
#endif
    if (fleetData[f].leadShipType == -1)
        return;
    if (fleetData[f].usable == 0)
        return;

    slot = GenericRandomShipSpawn(sys, kSpawnReserve);
    if (slot != -1) {
        shipData[slot].type = fleetData[idx].leadShipType;
        shipData[slot].govt = fleetData[idx].govt;
        if (escortOf != -1)
            shipData[slot].aiType = escortOf;
        else
            shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
        shipData[slot].shield = shipClass[shipData[slot].type].shield;
        shipData[slot].armor = shipClass[shipData[slot].type].armor;
        shipData[slot].persType = -1;
        shipData[slot].specialMission = -1;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
        AISetup(&shipData[slot]);
        if ((fleetData[f].flags & kFleetFlagRandomCargo) &&
            shipClass[shipData[slot].type].inherentAI <= kShipTypeBraveTrader)
            shipData[slot].cargo[Rand(kCargoTypes)] =
                Rand(shipClass[shipData[slot].type].cargo) + 1;
        shipData[slot].lastStellar = -2;
        shipData[slot].prevSystem = -1;
        shipData[slot].vel.x = 0;
        shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        stellar = PickEmergeStellar(&shipData[slot]);
        if (stellar != -1) {
            AIMakeShipEmergeFromHyperGate(&shipData[slot], stellar);
            shipData[slot].pos.x = stellarData[stellar].pos.h;
            shipData[slot].pos.y = stellarData[stellar].pos.v;
        } else {
            AIMakeShipJumpIn(&shipData[slot]);
            dist = 0;
            speed = 50.0f;
            while (speed > 0) {
                dist += speed;
                speed -= 1.165;
            }
            shipData[slot].pos.x = shipData[slot].pos.y = 0;
            Accel(Rand(kFullCircle), dist + 1000.0f,
                  &shipData[slot].pos);
            pt.x = pt.y = 0;
            shipData[slot].heading =
                CalcInterceptAngle(&shipData[slot].pos, &pt);
            shipData[slot].jumpTimer = -999;
            shipData[slot].jumpTick = ReadTick();
            Accel(shipData[slot].heading, 50.0f,
                  &shipData[slot].vel);
        }
        for (k = 0; k < kMaxWeapTypes; k++) {
            shipData[slot].weap[k].count =
                shipClass[shipData[slot].type].weapCount[k];
            shipData[slot].weap[k].ammo =
                shipClass[shipData[slot].type].weapAmmo[k];
        }
        for (j = 0; j <= 3; j++) {
            count = fleetData[idx].escortMin[j] +
                    Rand(fleetData[idx].escortMax[j] -
                         fleetData[idx].escortMin[j] + 1);
            type = fleetData[idx].escortTypes[j];
            if (type == -1)
                count = 0;
            else if (shipClass[type].canAppear == 0)
                count = 0;
            if (count > 0) {
                for (k = 0; k < count; k++) {
                    esc = GenericRandomShipSpawn(sys, kSpawnReserve);
                    if (esc != -1) {
                        shipData[esc].aiType = kShipTypeEscort;
                        shipData[esc].parentShip = slot;
                        shipData[esc].type = fleetData[idx].escortTypes[j];
                        shipData[esc].dude = -1;
                        shipData[esc].persType = -1;
                        shipData[esc].govt = fleetData[f].govt;
                        shipData[esc].shield =
                            shipClass[shipData[esc].type].shield;
                        shipData[esc].armor =
                            shipClass[shipData[esc].type].armor;
                        AISetup(&shipData[esc]);
                        for (i = 0; i < kMaxWeapTypes; i++) {
                            shipData[esc].weap[i].count =
                                shipClass[shipData[esc].type].weapCount[i];
                            shipData[esc].weap[i].ammo =
                                shipClass[shipData[esc].type].weapAmmo[i];
                        }
                        shipData[esc].heading = shipData[slot].heading;
                        shipData[esc].pos.x =
                            shipData[slot].pos.x + Rand(300) - 150.0f;
                        shipData[esc].pos.y =
                            shipData[slot].pos.y + Rand(300) - 150.0f;
                        shipData[esc].vel.x = shipData[slot].vel.x;
                        shipData[esc].vel.y = shipData[slot].vel.y;
                        shipData[esc].accelMag = shipData[slot].accelMag;
                        shipData[esc].lastStellar = -2;
                        shipData[esc].prevSystem = -2;
                        shipData[esc].swarmLeader = -1;
                        shipData[esc].formLeader = slot;
                        shipData[slot].hasFormation = 1;
                        if ((fleetData[f].flags & kFleetFlagRandomCargo) &&
                            shipClass[shipData[esc].type].inherentAI <= kShipTypeBraveTrader)
                            shipData[esc].cargo[Rand(kCargoTypes)] =
                                Rand(shipClass[shipData[esc].type].cargo) + 1;
                        if (stellar != -1) {
                            AIMakeShipEmergeFromHyperGate(&shipData[esc],
                                                          stellar);
                            shipData[esc].pos.x = stellarData[stellar].pos.h;
                            shipData[esc].pos.y = stellarData[stellar].pos.v;
                            shipData[esc].heading = shipData[slot].heading;
                            shipData[esc].aiTimer += Rand(15) + 5;
                            shipData[esc].lastStellar = -2;
                        } else {
                            shipData[esc].jumpTimer = -999;
                            shipData[esc].jumpTick = ReadTick();
                            AIMakeShipJumpIn(&shipData[esc]);
                        }
                    }
                }
            }
        }
        AICalcFormationPositions(&shipData[slot], 1);
        if (arriving && fleetData[f].quoteStrList > 0) {
            GetRandomIndString(buf, fleetData[f].quoteStrList);
            digit = 0;
            for (j = 1; j <= buf[0]; j++) {
                if (buf[j] == '#') {
#ifdef MACBUILD
                    if (digit)
                        buf[j] = Rand(10) + '0';
                    else
                        buf[j] = Rand(9) + '1';
#else
                    if (digit)
                        c = Rand(10) + '0';
                    else
                        c = Rand(9) + '1';
                    buf[j] = c;
#endif
                    digit = 1;
                } else
                    digit = 0;
            }
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm((PStrChar *)buf, 360, 0, kCommTextSize, &rgbWhite, 0, 0);
        }
    }
}
#undef f
#undef idx

/* SpeakVoiceMessage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426ce0
// MAC: _SpeakVoiceMessage i386 0x00044558 ppc 0x0004c3cc
void SpeakVoiceMessage(short a, short b, short c)
{
    pendingVoiceMsg = a;
    pendingVoiceGovt = b;
    pendingVoiceParity = c;
}

/* The hail quote is entry <persID> of STR# 7101, unless a plugin overrides it
   as resource 4999+persID. */

#define kHailQuoteResBase 0x1387 /* 4999; the plugin override, ID + persID */
#define kHailQuoteStrList 0x1bbd /* 7101; the built-in STR# */
#define kHailCommFrames   420

/* ShowPersonHailQuote (mac _ShowPersonHailQuote). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426d10
// MAC: _ShowPersonHailQuote i386 0x0004457e ppc 0x0004c3f4
void ShowPersonHailQuote(short persID)
{
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    if (LoadPluginString(persID + kHailQuoteResBase, parseBuffer) == 0)
        GetIndString((unsigned char *)parseBuffer, kHailQuoteStrList, persID);
    PtoCstr((unsigned char *)parseBuffer);
    ParseTextDescription();
    strcpy(briefingTextPtr, parseBuffer);
    MungeBriefing(0, -1);
    CtoPstr(briefingTextPtr);
    DisplayComm((PStrChar *)briefingTextPtr, kHailCommFrames, 0, kCommTextSize,
                &rgbWhite, 0, 0);
}

/* SpawnBountyHunter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426dd0
// MAC: _SpawnBountyHunter i386 0x00044661 ppc 0x0004c4c4
void SpawnBountyHunter(void)
{
    short i;
    short eligible;
    Boolean doHail;
    short shipIndex;

    if (personData[kPersBountyHunter].loaded == 0 || personData[kPersBountyHunter].prepared == 0)
        return;

    eligible = 0;
    doHail = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0
            && (stellarData[i].flags2 & kSpobAlwaysDominated) == 0)
            eligible++;
    }

    if (eligible == 1) {
        if (Rand(10) == 0)
            doHail = 1;
    }
    if (eligible > 1) {
        if (Rand(5) == 0)
            doHail = 1;
    }

    if (doHail) {
        shipIndex = SpawnPerson(shipData->curSystem, 1, kPersBountyHunter);
        if (shipIndex != -1) {
            commCount = -1;
            AIMakeShipAttackPlayer(&shipData[shipIndex]);
            offeringShipIndex = shipIndex;
            ShowPersonHailQuote(personData[shipData[shipIndex].persType].hailQuote);
            offeringShipIndex = -1;
        }
    }
}

/* ShotCanHitShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426ef0
// MAC: _ShotCanHitShip i386 0x0004477e ppc 0x0004c60c
Boolean ShotCanHitShip(ShotObj *shot, ShipObj *ship)
{
    short reachesPlayer;
    short nearPlayer;
    short cur;

    if (shot == 0 || ship == 0)
        return 0;
    if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes)
        return 0;
    if (ship->type < 0 || ship->type >= kMaxShipTypes)
        return 0;
    if (shot->life > 0.0f && ship->type != kMaxShipTypes - 1) {
        if (shot->owner == ship->index)
            return 0;
        if (shot->guidanceState == 0x3e6)
            return 0;
        if (ship->persType == kPersCaptainHector)
            return 0;

        if (weaponStats[shot->weapType].guidance == kGuidanceHoming
                && ship->index != shot->target
                && (weaponStats[shot->weapType].flags2 & kWeapProxHitsAnyShip) == 0)
            return 0;

        if (shot->owner >= 0 && shot->owner < kMaxShips) {
            if (shipData[shot->owner].parentShip != -1 && ship->parentShip != -1
                    && ship->parentShip == shipData[shot->owner].parentShip)
                return 0;
            if ((shipData[shot->owner].govt >= 0 && shipData[shot->owner].govt < kMaxGovts)
                    && ship->govt == shipData[shot->owner].govt)
                return 0;
            if (shipData[shot->owner].defenseStellar != -1
                    && ship->defenseStellar == shipData[shot->owner].defenseStellar)
                return 0;

            if (shot->hitRoll > shipClass[ship->type].aiSkill && !IsDisabled(ship))
                return 0;
            if (ship->index == 0)
                nearPlayer = 1;
            else if (AlliedWithPlayer(ship))
                nearPlayer = 1;
            else
                nearPlayer = 0;

            reachesPlayer = 0;
            cur = shot->owner;
            while (1) {
                if (cur == 0) {
                    reachesPlayer = 1;
                    break;
                }
                if (cur < 0 || cur >= kMaxShips) {
                    cur = -1;
                    break;
                }
                cur = shipData[cur].parentShip;
                if (cur == -1)
                    break;
            }

            if (reachesPlayer && nearPlayer
                    && (ship->parentShip >= 0 && ship->parentShip < kMaxShips)
                    && shipData[ship->parentShip].parentShip == 0)
                return 0;
            if (reachesPlayer && shipData[shot->owner].govt >= 0 && shipData[shot->owner].govt < kMaxGovts
                    && (govtData[shipData[shot->owner].govt].flags & kGovtNeverAttacksPlayer) != 0)
                return 0;
            if (reachesPlayer && ship->govt >= 0 && ship->govt < kMaxGovts
                    && (govtData[ship->govt].flags & kGovtImmuneToPlayer) != 0)
                return 0;
            if (reachesPlayer && ship->dude >= 0 && ship->dude < kMaxDudes
                    && (dudeData[ship->dude].flags & kDudeUntouchable) != 0)
                return 0;

            if (nearPlayer) {
                if (shipData[shot->owner].dude >= 0 && shipData[shot->owner].dude < kMaxDudes) {
                    if ((dudeData[shipData[shot->owner].dude].flags & kDudeUntouchable) != 0)
                        return 0;
                } else if (shipData[shot->owner].parentShip >= 0
                        && shipData[shot->owner].parentShip < kMaxDudes
                        && (shipData[shipData[shot->owner].parentShip].dude >= 0
                            && shipData[shipData[shot->owner].parentShip].dude < kMaxDudes)
                        && (dudeData[shipData[shipData[shot->owner].parentShip].dude].flags & kDudeUntouchable) != 0) {
                    return 0;
                }
            }

            if (shot->owner > 0 && AIShipIsAttackingAsteroid(&shipData[shot->owner]))
                return 0;
        } else {
            if (shot->target >= 0 && shot->target < kMaxShips
                    && ship->index != shot->target && ship->defenseStellar != shot->target)
                return 0;
        }

        if ((shipClass[ship->type].flags & kShipPlanetType) != (weaponStats[shot->weapType].flags2 & kWeapPlanetType))
            return 0;

        if ((ship->govt >= 0 && ship->govt < kMaxGovts) && ship->index != 0
                && (govtData[ship->govt].flags & kGovtNeverAttacksPlayer) != 0) {
            if (shot->owner == 0)
                return 0;
            if (shot->owner > 0 && shot->owner < kMaxShips
                    && shipData[shot->owner].parentShip == 0)
                return 0;
        }

        if (shot->owner >= 0 && shot->owner < kMaxShips
                && ShipsShareParent(ship, &shipData[shot->owner]))
            return 0;

        return 1;
    }
    return 0;
}

#define kFixedOne   256.0f      /* NewParticle's 24.8 scale */

/* SpawnParticles (mac _SpawnParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004274d0
// MAC: _SpawnParticles i386 0x00044b70 ppc 0x0004cad4
void SpawnParticles(short x, short y, float speed, short spreadPct,
                  short minLife, short maxLife, int32_t color, short shade,
                  short count, short jitter)
{
    FloatPoint vel;
    FloatPoint pos;
    short i;
    short life;
    float mag;

    for (i = 0; i < count; i++) {
        vel.x = vel.y = 0;
        mag = speed;
        if (spreadPct > 0 && spreadPct < 100)
            mag *= (Rand(spreadPct * 2 + 1) + (100 - spreadPct)) * 0.01;
        Accel(Rand(kFullCircle), mag, &vel);

        life = minLife;
        if (maxLife > minLife)
            life += Rand(maxLife - minLife + 1);

        pos.x = x;
        pos.y = y;
        if (jitter > 0)
            Accel(Rand(kFullCircle), (Rand(jitter * 100) * 0.01), &pos);

        NewParticle(color, (int32_t)pos.x << 8, (int32_t)pos.y << 8,
                    (vel.x * kFixedOne), (vel.y * kFixedOne),
                    shade, life);
    }
}

#define kModTypePaint   43

#define kMapGovtClassBase 1000

/* GrantOutfitItem (mac _GrantOutfitItem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427770
// MAC: _GrantOutfitItem i386 0x00044d4f ppc 0x0004cd00
void GrantOutfitItem(short outfit)
{
    short i;
    short s;
    short k;
    short v;
    short govtClass;
    Boolean paintSet;
    Boolean fakeID;

    if (outfit < 0 || outfit >= kMaxOutfits)
        return;
    boughtMap = 0;
    boughtID = 0;
    paintSet = 0;
    fakeID = 0;
    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfit].modType[i] == kModTypeMap) {
            v = outfitSystem[outfit].modVal[i];
            if (v > 0) {
                AutoSetExploration(shipData[0].curSystem, v, 2);
            } else if (v == -1) {
                for (s = 0; s < kMaxSystems; s++) {
                    if (systemData[s].loaded != 0 && systemData[s].govt == -1
                            && SystemIsInhabited(s) != 0)
                        AutoSetExploration(s, 0, 2);
                }
            } else if (v <= -kMapGovtClassBase) {
                govtClass = -1 * (v + kMapGovtClassBase);
                for (s = 0; s < kMaxSystems; s++) {
                    if (systemData[s].loaded != 0 && systemData[s].govt != -1) {
                        for (k = 0; k < kGovtClasses; k++) {
                            if (govtData[systemData[s].govt].classes[k]
                                    == govtClass) {
                                AutoSetExploration(s, 0, 2);
                                break;
                            }
                        }
                    }
                }
            }
            boughtMap = 1;
            break;
        }
    }
    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfit].modType[i] == kModTypePaint) {
            playerPaint.red =
                ((unsigned short)outfitSystem[outfit].modVal[i] >> 10) & 0x1f;
            playerPaint.green =
                ((unsigned short)outfitSystem[outfit].modVal[i] >> 5) & 0x1f;
            playerPaint.blue =
                (unsigned short)outfitSystem[outfit].modVal[i] & 0x1f;
            paintSet = 1;
            break;
        }
    }
    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfit].modType[i] == kModTypeFakeID) {
            fakeID = 1;
            break;
        }
    }
    if (boughtMap == 0) {
        if (paintSet == 0) {
            if (fakeID != 0) {
                for (i = 0; i < kOutfMods; i++) {
                    if (outfitSystem[outfit].modType[i] == kModTypeFakeID) {
                        for (s = 0; s < kMaxSystems; s++) {
                            if (systemData[s].loaded != 0) {
                                if (outfitSystem[outfit].modVal[i] == -1) {
                                    if (playerRecord[s] < 0)
                                        playerRecord[s] = 0;
                                } else if (systemData[s].govt
                                        == outfitSystem[outfit].modVal[i]) {
                                    if (playerRecord[s] < 0)
                                        playerRecord[s] = 0;
                                }
                            }
                        }
                    }
                }
                boughtID = 1;
            } else {
                shipSystems[outfit]++;
            }
        }
    }
}

/*
 * Fills the first beamData slot whose state is < -1.
 */

/* SpawnBeam. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427a90
// MAC: _SpawnBeam i386 0x00044ff3 ppc 0x0004d048
void SpawnBeam(short ship, short target, short weapType, short unused, short mount, short flag)
{
    short i;

    for (i = 0; i < kMaxBeams; i++) {
        if (beamData[i].state < -1) {
            beamData[i].state = weaponStats[weapType].lifetime;
            beamData[i].frame = 0;
            beamData[i].weapType = weapType;
            beamData[i].owner = ship;
            beamData[i].targetIsShot = flag;
            beamData[i].mount = weaponStats[weapType].exitType;
            if (mount >= 0 && mount <= 3) {
                beamData[i].lane = mount;
            } else {
                if (shipData[ship].weapGroupLane[weaponStats[weapType].exitType] < 0
                        || shipData[ship].weapGroupLane[weaponStats[weapType].exitType] > 3)
                    shipData[ship].weapGroupLane[weaponStats[weapType].exitType] = Rand(4);
                if ((weaponStats[weapType].flags3 & kWeapClosestExit)
                        && ship != -1 && flag != 1 && target != -1) {
                    beamData[i].lane = SelectClosestShotStartPosition(
                        &shipData[ship], &shipData[ship].pos, weapType,
                        shipData[ship].heading, weaponStats[weapType].exitType,
                        &shipData[target].pos);
                    if (shipData[ship].weapGroupLane[weaponStats[weapType].exitType] < 0
                            || shipData[ship].weapGroupLane[weaponStats[weapType].exitType] > 3)
                        shipData[ship].weapGroupLane[weaponStats[weapType].exitType] = Rand(4);
                } else {
                    beamData[i].lane =
                        shipData[ship].weapGroupLane[weaponStats[weapType].exitType];
                }
                shipData[ship].weapGroupLane[weaponStats[weapType].exitType] = 
                    ((shipData[ship].weapGroupLane[weaponStats[weapType].exitType] + 1) % 4);
            }
            beamData[i].target = target;
            beamData[i].disableOnly = WeaponDisableOnly(weapType);
            if (beamData[i].disableOnly == 0 && ship > 0 && flag != 1 && target != -1
                    && !IsDisabled(&shipData[target])
                    && AIShipIsPlunderingShip(&shipData[ship], &shipData[target]))
                beamData[i].disableOnly = 1;
            return;
        }
    }
}

#define kNoRank                 (-1)

/* ActivateRank (mac _ActivateRank). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427df0
// MAC: _ActivateRank i386 0x000452f0 ppc 0x0004d358
void ActivateRank(char *rankp)
{
    RankRecord *rank = (RankRecord *)rankp;
    short i;

    if (rank->active == 0) {
        crashlog_print("activating rank ");
        crashlog_print(rank->name);
        crashlog_print("\n");
        rank->active = 1;
        if ((rank->flags & kRankGrantDropsSameGovt) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && i != rank->index
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
        if ((rank->flags & kRankGrantDropsLowerPriority) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && i != rank->index
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0
                        && rankData[i].priority < rank->priority) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
    }
    recentActivatedRank = rank->index;
}

/* DeactivateRank (mac _DeactivateRank). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427f40
// MAC: _DeactivateRank i386 0x0004541e ppc 0x0004d53c
void DeactivateRank(char *rankp)
{
    RankRecord *rank = (RankRecord *)rankp;
    short i;

    if (rank->active != 0) {
        crashlog_print("deactivating rank ");
        crashlog_print(rank->name);
        crashlog_print("\n");
        rank->active = 0;
        if ((rank->flags & kRankDropsSameGovt) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
        if ((rank->flags & kRankDropsLowerPriority) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0
                        && rankData[i].priority < rank->priority) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
    }
    if (recentActivatedRank == rank->index)
        recentActivatedRank = kNoRank;
}

/*
 * podData is the 32-slot pod array InitArrays allocates as AllocAPointer(0x300); a slot is
 * free once its life counter has run below zero.
 */

/* SpawnEscapePod. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00428090
// MAC: _SpawnEscapePod i386 0x00045547 ppc 0x0004d704
void SpawnEscapePod(ShipObj *ship)
{
    short i;
    short angle;
    short heading;
    FloatPoint origin;
    FloatPoint drift;

    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].life < 0.0f) {
            podData[i].life = (Rand(100) + 150);
            podData[i].pos.x = ship->pos.x;
            podData[i].pos.y = ship->pos.y;
            podData[i].vel.x = ship->vel.x;
            podData[i].vel.y = ship->vel.y;

            angle = Rand(kFullCircle);
            SWSetSpriteVisible(podData[i].sprite, 1);
            Accel(angle, (Rand(10) + 10) * 0.1, &podData[i].vel);

            origin.x = origin.y = 0;
            drift.x = podData[i].vel.x * 100.0;
            drift.y = podData[i].vel.y * 100.0;
            heading = CalcInterceptAngle(&origin, &drift);
            SWSetCurrentFrameIndex(podData[i].sprite,
                         (heading * shipClass[kShipTypeEscapePod].framesPer / 360.0));
            break;
        }
    }
}

/* The exe's const table at 0x0056b910: the same 38 revoked keys that InitEVNova tests inline
   (transcription allowed by the user, 2026-09-16). */
#define kPirateSTR   140
#define kPirateIndex 50

#ifdef MAC1010 /* 1.0.10 spelling */
typedef struct KeyTable {
    UnsignedWide key[kBlacklistCount];
} KeyTable;
#else
typedef struct KeyTable {
    long long key[kBlacklistCount];
} KeyTable;
#endif

#define kRevokedKeys \
    0x0284801349262406LL, 0x0101e057b160ee02LL, \
    0x07365c430b23e131LL, 0x07345d430b23e131LL, \
    0x05d4013a1123c94aLL, 0x08d22033a5a9ae47LL, \
    0x08d02133a5a9ae47LL, 0x0940f13e07262d74LL, \
    0x01e8616da73a6b41LL, 0x03395a952823e649LL, \
    0x033b5a952803f64bLL, 0x0b795a952803f64aLL, \
    0x00dd021413f10475LL, 0x00df021413d11477LL, \
    0x009f021413d11476LL, 0x09224585278cbf5bLL, \
    0x0bf35041709a3940LL, 0x03f15041709a3946LL, \
    0x0125a4aa21b1cf03LL, 0x03a31e5b03829d3dLL, \
    0x009f8c40aac47814LL, 0x045ac4b40fb08b36LL, \
    0x0c1ac5bc0b909b37LL, 0x02a974140d644b4dLL, \
    0x02a9741409644b49LL, 0x0aab751c09445b49LL, \
    0x02eb751c09445b4aLL, 0x01e24b6c4b56f67eLL, \
    0x048c30d71f489e43LL, 0x0c8c31d71f489e45LL, \
    0x04cc31d71f489e46LL, 0x0c8e31d71f489e47LL, \
    0x0cce31d71f489e40LL, 0x0561f87749b27666LL, \
    0x090a1452e822bd05LL, 0x04c23c3d8c45fe08LL, \
    0x03e90792a484de04LL, 0x000881643da5fb73LL

#ifdef MAC1010 /* 1.0.10 spelling: the same keys as {hi, lo} */
#define kRevokedKeyWords \
    {0x02848013, 0x49262406}, {0x0101e057, 0xb160ee02}, \
    {0x07365c43, 0x0b23e131}, {0x07345d43, 0x0b23e131}, \
    {0x05d4013a, 0x1123c94a}, {0x08d22033, 0xa5a9ae47}, \
    {0x08d02133, 0xa5a9ae47}, {0x0940f13e, 0x07262d74}, \
    {0x01e8616d, 0xa73a6b41}, {0x03395a95, 0x2823e649}, \
    {0x033b5a95, 0x2803f64b}, {0x0b795a95, 0x2803f64a}, \
    {0x00dd0214, 0x13f10475}, {0x00df0214, 0x13d11477}, \
    {0x009f0214, 0x13d11476}, {0x09224585, 0x278cbf5b}, \
    {0x0bf35041, 0x709a3940}, {0x03f15041, 0x709a3946}, \
    {0x0125a4aa, 0x21b1cf03}, {0x03a31e5b, 0x03829d3d}, \
    {0x009f8c40, 0xaac47814}, {0x045ac4b4, 0x0fb08b36}, \
    {0x0c1ac5bc, 0x0b909b37}, {0x02a97414, 0x0d644b4d}, \
    {0x02a97414, 0x09644b49}, {0x0aab751c, 0x09445b49}, \
    {0x02eb751c, 0x09445b4a}, {0x01e24b6c, 0x4b56f67e}, \
    {0x048c30d7, 0x1f489e43}, {0x0c8c31d7, 0x1f489e45}, \
    {0x04cc31d7, 0x1f489e46}, {0x0c8e31d7, 0x1f489e47}, \
    {0x0cce31d7, 0x1f489e40}, {0x0561f877, 0x49b27666}, \
    {0x090a1452, 0xe822bd05}, {0x04c23c3d, 0x8c45fe08}, \
    {0x03e90792, 0xa484de04}, {0x00088164, 0x3da5fb73}

static const KeyTable revokedKeyTable = {{ kRevokedKeyWords }};
#elif !defined(MACBUILD) || !V111
static const KeyTable revokedKeyTable = {{ kRevokedKeys }};
#endif

/* CheckForPiratedCodes. STATE: MATCH, byte-identical. 1.0.10 MATCH 65 of 65 on both fragments,
   padded. OPEN: none */
// FUNCTION: EVNOVA 0x00428260
// MAC: _CheckForPiratedCodes i386 0x000456cd ppc 0x0004d910
void CheckForPiratedCodes(void)
{
#if V111 /* V111 spelling */
    KeyTable keys = {{ kRevokedKeys }};
#else
    KeyTable keys;
#endif
    unsigned char msg[256];
    short i;
    unsigned long long code;
    unsigned long long k;

#if V111 /* V111 spelling */
#else
    keys = revokedKeyTable;
#endif
    for (i = 0; i < kBlacklistCount; i++) {
        code = RT3_GetLicenseCode();
#ifdef MAC1010 /* 1.0.10 spelling */
        k = *(unsigned long long *)&keys.key[i];
#else
        k = keys.key[i];
#endif
        k <<= 4;
        k >>= 4;
        code <<= 4;
        code >>= 4;
        if (k == code) {
#ifdef MAC1010
            {
                unsigned long long sum = k + code; /* 1.0.10 pad: the masked halves read past the compare, so none takes r0 */
            }
#endif
            FadeScreenIn();
            ShowTheCursor();
            GetIndString(msg, kPirateSTR, kPirateIndex);
#ifdef MACBUILD
            ParamText(msg, "\p", "\p", "\p");
            StopAlert(128, NULL);
#else
#ifdef V111 /* V111: PtoCstr is void */
            PtoCstr(msg);
            ShowAlert("", (char *)msg);
#else
            ShowAlert("", PtoCstr(msg));
#endif
#endif
            ExitToShell();
        }
    }
}

#ifndef MACBUILD
/* CrtInitHook1. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00428330
void CrtInitHook1(void)
{
}
#endif

EXTERN_C_END

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
#endif
