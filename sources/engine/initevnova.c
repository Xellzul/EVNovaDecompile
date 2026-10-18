// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* initevnova.c -- tu-map object O2: game start, PlayGame, ship spawning, damage, escorts and fleets, then the pirated-key check and the .CRT hook that ends the object. Notes: Research/notes/initevnova.md */
#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "burgermath.h"
#include "burger.h"
#include "win32.h"
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
#include "prototypes.h"
#include "gametypes.h"

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

short oldVolume;
short gPluginCount;
unsigned char DAT_0059128c[8];
unsigned long lastSmokeUpdateTime;
unsigned long lastKeypressTime;
unsigned long lastEventLoopTime;
ShipObj *shipData;
ShipRec *gShipTypes;
SpobRecord *stellarData;
SpobRecord *dockedPort;
SystRecord *gSystems;
WeapRecord *gWeapons;
BoomSlot *explodData;
DebrisSlot *gDebrisSlots;
BoomRecord *explodType;
BeamRecord *beamData;
OutfRecord *gOutfits;
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
CronRecord *gCrons;
RankRecord *rankData;
StarSlot *starData;
char *missionBit;
char musicEnabledPref;
char DAT_005914d1;
char DAT_005914d2;
char gEarlyOutFlag;
char DAT_005914d4;
char gNoDebris;
char yieldEventsPref;
char parallaxStarsPref;
char windowedPref;
char windowedPrefPending;
unsigned char loadShipSetsPref;
unsigned char loadShipGlowPref;
unsigned char loadShipWeaponsPref;
unsigned char loadShipLightsPref;
char planetSoundsPref;
unsigned char gShowInfluence;
char noHyperGammaPref;
char noVersionCheckPref;
short volumePref;
short gammaPref;
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
SpriteRec *masterShipSprite[768];
SpriteRec *masterShipGlowSprite[768];
SpriteRec *masterShipShieldSprite[768];
SpriteRec *masterStarSprite;
SpriteRec *masterShipLightSprite[768];
SpriteRec *masterShipWeaponSprite[768];
SpriteRec *masterShipAltSprite[768];
SpriteRec *masterStellarSprite[256];
SpriteRec *masterExplodSprite[64];
SpriteRec *masterShotSprite[256];
SpriteRec *masterAsteroidSprite[16];
SpriteRec *masterSmokeSprite[8];
SpriteRec *menuRolloverSprite;
SpriteRec *stellarSprite[kStellarSprites];
SpriteRec *masterBoxSprite[5];
SpriteRec *gMainButtonSprites[6];
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
char statusRedraw[7];
char gameLoaded;
char gameInProgress;
char killShots;
char killBoxes;
char killExplods;
char killAsteroids;
char killSmoke;
char mBarHidden;
char strictPlay;
char weapLawFlag;
char smuggleFlag;
char fadePending[2];
char doubleTime;
char seenIntroScreen;
char playerThreatened;
char oldPlayerThreatened;
char gPause;
char gQuit;
char cheatsActive;
char gameFrozen;
char ignoreKeys;
char updateSmokeThisFrame;
char closeRequested;
unsigned char DAT_00596d40[4];
void *targetInfoPic[768];
GWorldContainer statusOffPort;
GWorldContainer loadPort;
unsigned char DAT_0059795c[16];
unsigned long tix;
unsigned long gJumpStartTick;
unsigned long lastMouseMoveTime;
unsigned long lastEscortMenuTime;
unsigned long lastFrameTime;
unsigned char DAT_00597980[4];
long lastFPSUpdateTime;
unsigned long lastLoMemWarningTime;
unsigned long lastCursorAnimTime;
unsigned char DAT_00597990[2];
short frameCounter;
unsigned char DAT_00597994[8];
short lastShipAIDispatched;
short gSharewareDays;
short cursorVisible;
unsigned char DAT_005979a2[2];
short selTradeItem;
short portDemand[6];
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
short shipSystems[512];
MapScroll mapScroll;
ShortPoint screenCenter;
FloatPoint playerMaxVel;
FloatPoint lastFramePlayerPos;
unsigned char playerName[256];
unsigned char lastPlayerName[256];
unsigned char playerNickname[256];
unsigned char shipName[256];
unsigned char gWeapNames[256][256];
unsigned char gShipLongNames[768][256];
unsigned char shipHelpRequests[20][256];
unsigned char legalStrings[18][256];
unsigned char gOutfShortNames[512][256];
unsigned char gShipShortNames[768][256];
unsigned char ratingStrings[11][256];
unsigned char govtTargStrings[256][256];
unsigned char lcOutfitName[512][256];
unsigned char lcPluralOutfitName[512][256];
unsigned char cargoName[256][0x100];
unsigned char statusCargoName[6][0x100];
unsigned char cargoAbbrev[256][0x100];
unsigned char lcCargoName[256][0x100];
unsigned char govtShortNames[256][256];
unsigned char gShipCommNames[768][256];
unsigned char gShipMovieFiles[768][256];
unsigned char descMovieName[256];
unsigned char gGovtNames3[256][256];
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
char gDescText[8192];
char commMsg[2048];
char gTextShowString[8192];
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
short playerLegalStatus[kMaxSystems];
short gSlotTable[36];
unsigned char DAT_00734c10[8];
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
long playerKills;
double gTimeScale;
double DAT_00735450;
DateTimeRec gameTime;
unsigned char DAT_00735466[14];
DateTimeRec scenarioStartDate;
unsigned char DAT_00735482[6];
char DAT_00735488;
char inBackground;
char miningShipsActive;
char miningBoxesActive;
char gravitySourcesPresent;
char showFPS;
float navTargetExpand;
float weapTargetExpand;
float destructCount;
float playerRepairInhibit;
float hyperGamma;
char leetMode;
char keyPressedThisFrame;
unsigned char hypergateMapMode;
char gCommDone;
char screenFaded;
char playerIsLanded;
char playerKilledByHector;
char keyComboBlocked;
Point lastMousePos;
short gDescFlags;
short showFloatingMap;
short escortMenuOnscreen;
short recentActivatedRank;
short gDescRankGovt[2];
short fpsCounter;
short cursorAlpha;
short jumpSoundLength[2];
short playerEscortOrders[4];
short lastIntfResourceID;
short hypergateMapModeStellar;
char absquatulateStr[256];
long DAT_007355d0;
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
float gCachedShieldRech;
float gCachedArmorCap;
float gCachedArmorRech;
float gCachedFuelCap;
float gCachedTurnRate;
float gCachedAccelRate;
float gCachedMaxSpeed;
float gCachedMod40;
float gCachedMod39;
float gCachedMod15;
short playerHasAfterburner;
short gHasModType13;
short gHasModType14;
short gCachedMod24;
short gMaxSoundRange;
short gCachedMod30;
short gCachedMod30b;
short gCachedMod38;
short gCachedMod41;
short gCachedMod42;
short playerHasAreaCloak;
short playerSabotaged;
char playerHasIllegalJunk;
char playerHasIllegalOutfits;
char allReinforcementsInhibited;
char playerHasTribbles;
char playerHasPerishableJunk;
unsigned long long fakeLicenseCode;
short gameIsRegisteredIndex;
short gameIsHackedIndex;

extern "C" {


                                                         /* 0x004d9e10 BurgerLib */
                                                         /* 0x004d4740 RT3_Open */
                                                         /* 0x0046f690 */


/* InitEVNova (mac _InitEVNova). STATE: MATCH 100.0%, exe 1393 / obj 1393, byte-identical yes
   (5376 bytes, relocs masked), relocregcheck 0 of 297.
   OPEN: nothing. */
// FUNCTION: EVNOVA 0x00416100
void InitEVNova(void)
{
    unsigned char alertText[256];
    char blk[kBlacklistCount];
    long qtVersion;
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
    SetupMonitor();
    ResetScreenRect();

    crashlog("initializing QuickTime");
    err = EnterMovies();
    if (err != 0)
        ShowIndAlert(kAlertQuickTime);

    crashlog("building trig table");
    BuildTrigTable();

    if (gammaPref > kMaxGamma)
        gammaPref = kDefaultGamma;
    BuildGammaRamp((gammaPref * 10.0 + 70.0));
    gWind = 0;
    SavePrefs();

    crashlog("opening platform tool");
    err = PlatformOpen();
    if (err != 0) {
        crashlog_print("Platform_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        ShowIndAlert(kAlertPlatformOpen);
    }

    crashlog("opening file tool");
    err = FT_Open();
    if (err != 0) {
        crashlog_print("FT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        ShowIndAlert(kAlertFileOpen);
    }

    crashlog("opening draw tool");
    err = DT_Open();
    if (err != 0) {
        crashlog_print("DT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        ShowIndAlert(kAlertDrawOpen);
    }

    crashlog("opening interface tool");
    err = IT_Open();
    if (err != 0) {
        crashlog_print("IT_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        ShowIndAlert(kAlertInterfaceOpen);
    }

    crashlog("opening registration tool");
    err = RT3_Open(1, 3, "EV Nova", "EV Nova");
    if (err != 0) {
        crashlog_print("RT3_Open returned ");
        crashlog_print_num(err);
        crashlog_print("\n");
        ShowIndAlert(kAlertRegOpen);
    }

    crashlog("initializing arrays");
    InitArrays();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("opening data files");
    *(long long *)((char *)shipData + kLicenseCodePair) = RT3_GetLicenseCode();
    FUN_0049e420();
    *(void **)((char *)shipData + kLicenseName) = RT3_GetLicenseName();
    IterateInGameEventLoop(kEventLoopTicks);
    *(long *)((char *)shipData + kLicenseCopies) = RT3_GetLicenseCopies();
    InitColors();

    gShipTypes[gameIsRegisteredIndex].registered =
        *(long *)((char *)shipData + kLicenseCopies) ? (unsigned char)1 : (unsigned char)0;
    InitMapWorld();

    key = *(long long *)((char *)shipData + kLicenseCodePair);

    gPluginCount = CountEVPlugins("Nova Plug-Ins", ".rez", 0);
    OpenDataFiles();
    gPluginCount = CountEVPlugins("Nova Plug-Ins", ".rez", 0);
    if (gPluginCount != 0) {
        if ((gShipTypes[gameIsRegisteredIndex].registered ? 1 : 0) == 0
                != (gPluginCount == 0)) {
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

    cursorVisible = 0;
    if (windowedPref == 0)
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

    FadeToBlack();
    gShipTypes[gameIsRegisteredIndex].registered =
        *(long *)((char *)shipData + kLicenseCopies) ? (unsigned char)1 : (unsigned char)0;

    crashlog("showing splash screen");
    ShowSplashScreen();
    HandleIntroMusic(1);
    FadeScreenIn();
    IterateInGameEventLoop(kEventLoopTicks);
    VerifyResourceChecksums();
    IterateInGameEventLoop(kEventLoopTicks);
    InitLoadBar();

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
        long g;
        unsigned short rem;
        char ok;
        int bail;
        unsigned int v;

        g = *(long *)((char *)shipData + kLicenseCopies);
        p = *(unsigned char **)((char *)shipData + kLicenseName);
        mem = *(long long *)((char *)shipData + kLicenseCodePair);
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

        bail = *(long *)((char *)shipData + kLicenseCopies) != 0 && ok == 0
                ? (unsigned char)1 : (unsigned char)0;
        if (bail) {
            if (windowedPref == 0)
                ReleaseVideo();
            RT3_RunReminder(1, 0);
        }
    }

    FadeScreenOut();
    ShowTheCursor();
    KeyboardDestroy();
    MouseDestroy();
    CleanUp();
    ClosePrefs();
}

#define kEscapePodType  0x2ff


/* PlayGame (mac _PlayGame). STATE: MATCH, exe 1105 / obj 1105, byte-identical yes (relocs
   masked). OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00417600
void PlayGame(void)
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
    MyProfileSetName(0, "game loop");
    MyProfileSetName(1, "pre draw tasks");
    MyProfileSetName(2, "drawing");
    MyProfileSetName(3, "post draw tasks");
    MyProfileSetName(4, "HandleShip");
    MyProfileSetName(5, "HandleShipDisplay");
    MyProfileSetName(6, "AI routines");
    MyProfileSetName(7, "HandleShot");
    MyProfileSetName(8, "misc handlers");
    MyProfileSetName(9, "collisions");
    MyProfileSetName(10, "player");
    MyProfileSetName(11, "misc handlers2");
    MyProfileSetName(12, "DrawStatus");
    MyProfileSetName(13, "ProcessSpriteWorld");
    MyProfileSetName(14, "AnimateSpriteWorld");
    MyProfileSetName(15, "PostAnimate");
    MyProfileSetName(16, "locking");
    MyProfileSetName(17, "erase");
    MyProfileSetName(18, "draw");
    MyProfileSetName(19, "update");
    MyProfileSetName(20, "calc AI odds");
    gSpriteWorld->fastBlit = 1;
    inBackground = 0;
    gPause = 0;
    FlushEvents(8, 0);
    FlushEvents(0xf, 0);

    for (;;) {
        if (gQuit != 0 || gPause != 0)
            break;
        MyProfileEnter(0);
        HandleTimeAdjustment();
        keyPressedThisFrame = 0;
        if (inBackground == 0) {
            if (gSpriteWorld->fastBlit != 0) {
                MyProfileEnter(1);
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
                            DisplayComm(str, 100, 0, 12, &rgbWhite, 0, 0);
                        } else {
                            NumToString((fpsCounter * 60.0 /
                                    (ReadTick() - lastFPSUpdateTime)),
                                    numStr);
                            PtoCstr(numStr);
                            strcpy((char *)str, (char *)numStr);
                            strcat((char *)str, " ");
                            AppendIndString((char *)str, kGameStrList, 283);
                            CtoPstr((char *)str);
                            DisplayComm(str, 999, 0, 12, &rgbWhite, 0, 0);
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
                MyProfileExit(1);
                MyProfileEnter(2);
                MyProfileEnter(13);
                SWProcessSpriteWorld(gSpriteWorld);
                MyProfileExit(13);
                MyProfileEnter(14);
                SWAnimateSpriteWorld(gSpriteWorld);
                MyProfileExit(14);
                MyProfileEnter(15);
                FlushOverlayStrips();
                MyProfileExit(15);
                ScreenEndScene();
                MyProfileExit(2);
                MyProfileEnter(3);
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
                        DisplayComm((unsigned char *)"\pHappy birthday Matt",
                                    5, 3, 9, &rgbDarkGrey, 0, 0);
                    {
                        unsigned char blob[3] = { 0x49, 0x71, 0xad };
                        unsigned char name[17] = "EV NovaEV NovaEV";
                        unsigned char key[17] = "";
                        long g;
                        unsigned char *p;
                        long long mem;
                        unsigned char x;
                        char b;
                        int t1;
                        int t2;
                        int t3;
                        int t4;
                        unsigned short rem;
                        char ok;
                        short doIt;

                        g = *(long *)((char *)shipData + kLicenseCopies);
                        p = *(unsigned char **)((char *)shipData + kLicenseName);
                        mem = *(long long *)((char *)shipData + kLicenseCodePair);
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

                        doIt = (*(long *)((char *)shipData + kLicenseCopies) != 0
                                && ok == 0 ? (unsigned char)1 : (unsigned char)0);
                        if (doIt) {
                            shipData[0].credits *= 0.1;
                            statusRedraw[2] = 1;
                        }
                    }
                }
                if (missionSoundID != -1 && missionSnd == 0) {
                    missionSnd = LoadSoundResource(missionSoundID);
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
                } else if (!(GetKeyState(VK_CAPITAL) & 1)) {
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
                MyProfileExit(3);
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
            DisplayComm((unsigned char *)absquatulateStr, 500, 0, 12, &rgbWhite, 0, 0);
            RedrawComm(1);
            absquatulateStr[0] = 0;
        }
        MyProfileExit(0);
    }

    MyProfileDump("profile.txt");
    FadeScreenOut();
    ShowTheCursor();
    playerKilledByHector = 0;
    FlushEvents(everyEvent - diskMask, 0);
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
}

#define kMaxNavDefaults 0x10

/* MyProfile bracket ids, registered by name in PlayGame 0x00417600 (the table: notes) */
#define kProfPlayer     10      /* "player" */
#define kProfCollide     9      /* "collisions" */
#define kProfStatus     12      /* "DrawStatus" */
#define kProfAI          6      /* "AI routines" */
#define kProfHandlers   11      /* "misc handlers2" */
#define kProfOdds       20      /* "calc AI odds" */
#define kProfShots       7      /* "HandleShot" */
#define kProfShip        4      /* "HandleShip" */
#define kProfShipDisp    5      /* "HandleShipDisplay" */
#define kProfSprites     8      /* "misc handlers" */

/* DoPlayGameWork (mac _DoPlayGameWork). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004186b0
void DoPlayGameWork(char advance)
{
    short i;
    short n;

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
            if (shipData[i].active != 0 && shipData[i].aiType >= 5 &&
                    shipData[0].curSystem == shipData[i].curSystem) {
                if (shipData[i].parentShip > 0) {
                    AICheckParentExists(&shipData[i]);
                    if (shipData[i].parentShip != -1)
                        shipData[shipData[i].parentShip].hasEscorts = 1;
                } else if (shipData[i].parentShip == 0) {
                    shipData[0].hasEscorts = 1;
                }
                if (shipData[i].swarmLeader > i && shipData[i].swarmLeader < kMaxShips)
                    shipData[shipData[i].swarmLeader].hasSwarm = 1;
                if (shipData[i].aiType >= 5) {
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
                    shipData[0].curSystem == shipData[i].curSystem)
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
                        shipData[0].curSystem == shipData[i].curSystem) {
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
    gShipTypes[gameIsRegisteredIndex].registered =
        shipData[36].unkC940 != 0 ? (unsigned char)1 : (unsigned char)0;
    MyProfileExit(kProfShots);

    for (i = 0; i < kMaxNavDefaults; i++) {
        n = gSystems[shipData[0].curSystem].navDefault[i];
        if (n != -1)
            stellarData[n].defenseActive = 0;
    }
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].defenseStellar != -1)
            stellarData[shipData[i].defenseStellar].defenseActive = 1;
    }
    for (i = 1; i < kMaxShips; i++) {
        MyProfileEnter(kProfShip);
        if (shipData[i].active != 0 && shipData[0].curSystem == shipData[i].curSystem)
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
void PrepareAllSprites(void)
{
    char wasFrozen;
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

#define kShipFlagCloakOnHit     0x4000  /* ShipRec flags2 bit 14 (name TBC) */
#define kShipTypeFighter        5       /* aiType: launched from a bay */
#define kPersKillable           1       /* përs Flags bit 0 (name TBC) */
#define kMissionSurvivesKill    0x400   /* mïsn +0x55 bit 10 (name TBC) */
#define kMissionFailOnDeath     0x4     /* mïsn +0x57 bit 2 (name TBC) */
#define kGovtLegalScale         2       /* govt crimeTol is halved legal status */

#define kStrMissionFailed       0x11c
#define kStrShipDestroyed       0x11f
#define kStrPlayerKilled        0x120
#define kHectorStrList          30000
#define kHectorTauntBase        8
#define kHectorTauntCount       6

#define kCommTicks              0xf0
#define kHectorCommTicks        5000
#define kCommFontSize           12

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
   OPEN: none */
// FUNCTION: EVNOVA 0x004192d0
void DamageShip(ShipObj *ship, FloatPoint *from, short impact, short armorDmg,
                short shieldDmg, short attacker, char fromAttack, char deliberate,
                unsigned char disableOnly, char clampArmor,
                unsigned char armorOnly, short provocation)
{
    short theirTop;
    char wasDisabled;
    char wasDying;
    char msg[256];
    char selfHit;
    char hostile;
    float dx;
    float dy;
    float capacity;
    long aligned;
    long farther;
    long nearer;
    short angle;
    short myGovt;
    short myTop;
    FloatPoint spob;
    short i;

    if (ship->index < 0 || ship->index >= kMaxShips)
        return;
    if (ship->type < 0 || ship->type >= kShipTypeEscapePod)
        return;
    if (ship->active == 0)
        return;

    if (fromAttack != 0 && attacker > 0 && attacker < kMaxShips) {
        if (shipData[attacker].persType == kPersCaptainHector) {
            if (gSharewareDays > 90) {
                armorDmg *= 5;
                shieldDmg *= 5;
            } else if (gSharewareDays > 60) {
                armorDmg *= 3;
                shieldDmg *= 3;
            } else if (gSharewareDays > 40) {
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
        dx = FloatAbs(shipData[attacker].pos.x - ship->pos.x);
        if (dx < kImpactCancelRange
                || (dy = FloatAbs(shipData[attacker].pos.y - ship->pos.y)) < kImpactCancelRange)
            impact = 0;
    }

    if (impact != 0 && ship->jumpTimer <= 0.0f && from != NULL) {
        angle = CalcInterceptAngle(from, &ship->pos);
        if (gShipTypes[ship->type].mass > 0
                && (gShipTypes[ship->type].flags & kShipFlagNoMove) == 0) {
            AdjustedAccel(angle, impact / (float)gShipTypes[ship->type].mass,
                          &ship->vel, gShipTypes[ship->type].maxSpeed);
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
        if ((gShipTypes[ship->type].flags & kShipArmorDisableTenth) != 0)
            ship->armor = ShipArmorCapacity(ship) * kTenthArmorFraction + 1.0;
        else
            ship->armor = ShipArmorCapacity(ship) * kDisableArmorFraction + 1.0;
    }

    hostile = 0;
    if (attacker == 0) {
        hostile = 1;
    } else if (attacker > 0 && attacker < kMaxShips
               && shipData[attacker].parentShip == 0) {
        hostile = 1;
    }
    if (hostile != 0 && ship->defenseStellar == -1 && ship->persType < kPersCaptainHector) {
        if (IsDisabled(ship) && wasDisabled == 0)
            SlapWithPenalty(shipData->curSystem, ship->govt, 1, ship->specialMission);
        if (ship->armor <= 0.0f && wasDying == 0 && !ShipIsDerelict(ship)) {
            SlapWithPenalty(shipData->curSystem, ship->govt, 3, ship->specialMission);
            AddToPlayerKills(gShipTypes[ship->type].strength);
        }
    }

    if (IsDisabled(ship)) {
        if (ship->specialMission >= 0 && ship->specialMission < kMaxActiveMissions
                && ship->defenseStellar == -1 && wasDisabled == 0) {
            if (currentMissionObjectives[ship->specialMission].active != 0) {
                if (currentMissionObjectives[ship->specialMission].failed == 0
                        && currentMissions[ship->specialMission].specialShipsDisabled == 0
                        && currentMissions[ship->specialMission].shipGoal == kMissionKindDisable
                        && (currentMissions[ship->specialMission].flags & kMissionSurvivesKill) == 0) {
                    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                    GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                    DisplayComm((unsigned char *)msg, kCommTicks, 0, kCommFontSize,
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
                    && gShipTypes[ship->type].inherentAI <= 2) {
                DestroyPartialFleetCargo(ship->index);
                statusRedraw[2] = 1;
                DrawStatusCargo();
            }
            ship->aiType = gShipTypes[ship->type].inherentAI;
        }
        if (ship->index == 0 && wasDisabled == 0) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            GetIndString((unsigned char *)msg, kGameStrList, kStrShipDestroyed);
            DisplayComm((unsigned char *)msg, kCommTicks, 0, kCommFontSize,
                        &rgbWhite, 0, 0);
            playerRepairInhibit = 300.0f;
            ship->armor = (long)ship->armor;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0 && currentMissionObjectives[i].failed == 0
                        && (currentMissions[i].flags2 & kMissionFailOnDeath) != 0) {
                    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                    QuickMissionFailure(i);
                    GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                    DisplayComm((unsigned char *)msg, kCommTicks, 0, kCommFontSize,
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
            DisplayComm((unsigned char *)msg, kHectorCommTicks, 0, kCommFontSize,
                        &rgbWhite, 0, 0);
            playerKilledByHector = 1;
        }
        if (playerKilledByHector == 0) {
            GetIndString((unsigned char *)msg, kGameStrList, kStrPlayerKilled);
            DisplayComm((unsigned char *)msg, kCommTicks, 0, kCommFontSize,
                        &rgbWhite, 0, 0);
        }
        for (i = 0; i < kMaxActiveMissions; i++) {
            if (currentMissionObjectives[i].active != 0 && currentMissionObjectives[i].failed == 0
                    && (currentMissions[i].flags2 & kMissionFailOnDeath) != 0) {
                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                QuickMissionFailure(i);
                GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                DisplayComm((unsigned char *)msg, kCommTicks, 0, kCommFontSize,
                            &rgbWhite, 0, 0);
                break;
            }
        }
    }

    selfHit = 0;
    hostile = 0;
    if (attacker != ship->index) {
        hostile = 1;
    } else if (ship->defenseStellar != -1) {
        hostile = 1;
        selfHit = 1;
    } else if (attacker >= 0 && attacker < kMaxShips
               && shipData[attacker].defenseStellar != -1) {
        hostile = 1;
        selfHit = 1;
    }

    if (hostile && fromAttack != 0 && ship->aiType > 0 && ship->jumpTimer <= 0.0f) {
        hostile = 0;
        if (selfHit != 0) {
            if (attacker >= 0 && attacker < kMaxShips) {
                if (ship->defenseStellar == -1 || shipData[attacker].defenseStellar == -1)
                    hostile = 1;
                else
                    hostile = 0;
            } else
                hostile = 1;
        } else {
            if (attacker >= 0 && attacker < kMaxShips
                    && ship->govt != shipData[attacker].govt)
                hostile = 1;
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips) {
                if (attacker != ship->parentShip || attacker == -1)
                    hostile = 1;
                if (attacker >= 0 && attacker < kMaxShips
                        && shipData[attacker].parentShip != -1
                        && ship->parentShip != shipData[attacker].parentShip)
                    hostile = 1;
            }
            myGovt = ship->govt;
            if (myGovt < 0 || myGovt >= kMaxGovts) {
                if (attacker >= 0 && attacker < kMaxShips
                        && (attacker == 0 || shipData[attacker].parentShip == 0))
                    hostile = 1;
            } else {
                if (gSystems[shipData->curSystem].govt >= 0
                        && gSystems[shipData->curSystem].govt < kMaxGovts
                        && shipData->targetShip != ship->index
                        && playerLegalStatus[shipData->curSystem]
                           >= govtData[myGovt].crimeTol * kGovtLegalScale
                        && (attacker == 0
                            || (attacker > 0 && attacker < kMaxShips
                                && shipData[attacker].parentShip == 0)))
                    hostile = 0;
                if (attacker >= 0 && attacker < kMaxShips
                        && myGovt == shipData[attacker].govt)
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
                if (hostile && ship->govt >= 0 && ship->govt < kMaxGovts
                        && attacker == 0
                        && govtData[ship->govt].penalties[4] <= 0
                        && Rand(4) != 0)
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
                    dx = FloatAbs(ship->pos.x - shipData->pos.x);
                    if (dx <= kEscortDefendRange) {
                        dy = FloatAbs(ship->pos.y - shipData->pos.y);
                        if (dy <= kEscortDefendRange)
                            hostile = 0;
                    }
                }
                if (ship->parentShip == shipData[attacker].parentShip && ship->parentShip != -1)
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
                    && shipData[ship->parentShip].parentShip >= 0
                    && shipData[ship->parentShip].parentShip < kMaxShips)
                myTop = shipData[ship->parentShip].parentShip;
            if (attacker > 0 && attacker < kMaxShips
                    && shipData[attacker].parentShip > 0
                    && shipData[attacker].parentShip < kMaxShips)
                theirTop = shipData[shipData[attacker].parentShip].parentShip;
            if (myTop == theirTop && myTop >= 0 && myTop < kMaxShips
                    && theirTop >= 0 && theirTop < kMaxShips)
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
        if (hostile && attacker >= 0 && attacker < kMaxShips
                && gShipTypes[shipData[attacker].type].escortType == 0
                && gShipTypes[ship->type].escortType != 0
                && ship->targetShip != -1
                && IsShipAttacking(ship))
            hostile = 0;

        if (hostile) {
            ship->annoyance = 0;
            if ((gShipTypes[ship->type].flags2 & kShipFlagCloakOnHit) != 0
                    && attacker >= 0 && attacker < kMaxShips
                    && ShipCanCloak(ship)
                    && !IsShipFightingMe(ship, &shipData[attacker])) {
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
                        farther = XYFloatSquaredDist(&ship->pos, &shipData[attacker].pos)
                                  > XYFloatSquaredDist(&ship->pos, &shipData[ship->targetShip].pos) / 4;
                        if (farther)
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
                nearer = XYFloatSquaredDist(&ship->pos, &shipData[attacker].pos)
                         < XYFloatSquaredDist(&ship->pos, &spob) / 2;
                if (nearer) {
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
                            && (personData[ship->persType].flags & kPersKillable) != 0)
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
        statusRedraw[1] = 1;
    if (fromAttack != 0 && ship->decloaksWhenHit == 1
            && (ship->cloak == kFullCloak || ship->cloakDir >= 1))
        DoShipUncloak(ship);
}

/* DeleteAllComputerShips. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041ad50
void DeleteAllComputerShips(char force, char keepPlayer)
{
    short i;
    char remove;

    for (i = 1; i < kMaxShips; i++) {
        remove = 1;
        if (shipData[i].aiType >= 5 && shipData[i].parentShip == 0
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
#define kMissionKindDestroy     5       /* mïsn +0x0a */
#define kMissionShipEscort      1       /* mïsn +0x0c: they join the player */
#define kMissionShipCloaked     2       /* mïsn +0x0e */
#define kMissionAutoAbort       1       /* mïsn +0x55 bit 0 */
#define kNavStelFirst           (-1)    /* mïsn +0x0e: -1..-16 index navDefault */
#define kNavStelLast            (-16)
#define kSpawnOdds              7
#define kRandomSpawnReserve     8

/* SetupShipsInSystem (mac _SetupShipsInSystem). STATE: MATCH 100.0%, exe 675 / obj 675 instrs,
   551 exact + 124 reloc-equivalent, 0 mismatched; frame EXACT 0x24, 12/12 slots and refs;
   byte-identical UNMEASURED (bytediff reads the adopted baseline, which lacks this row).
   OPEN: none. `arriving' and RespawnEscort's `full' must BOTH be unsigned char; either
   one alone costs a row. see notes */
// FUNCTION: EVNOVA 0x0041af90
void SetupShipsInSystem(short sys, unsigned char arriving)
{
    float total;
    float step;
    float speed;
    short i;
    short j;
    short slot;
    short sysOf;
    short count;
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
                        && gShipTypes[shipData[i].type].inherentAI <= 2)
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
                    && (count = currentMissions[i].shipCount) > 0
                    && currentMissions[i].shipDelay < 0) {
                if (currentMissions[i].shipSyst == kMissionShipSystemHere
                        && currentMissions[i].shipBehav == kMissionShipEscort) {
                    for (j = 1; j < kMaxShips; j++) {
                        if (shipData[j].active != 0 && i == shipData[j].specialMission)
                            count--;
                    }
                }
                if (count > 0) {
                    for (j = 0; j < count; j++) {
                        slot = MissionDudeSpawn(currentMissions[i].shipDude, -1, sys, i);
                        if (slot != -1) {
                            if (currentMissions[i].shipGoal == kMissionKindDisable) {
                                shipData[slot].pos.x = Rand(0x200) - 0x100;
                                shipData[slot].pos.y = Rand(0x200) - 0x100;
                            }
                            if (currentMissions[i].shipGoal == kMissionKindDestroy) {
                                shipData[slot].heading = Rand(kFullCircle);
                                shipData[slot].vel.x = shipData[slot].vel.y = 0.0f;
                                shipData[slot].accelMag = 0.0f;
                                shipData[slot].shield = 0.0f;
                                if ((gShipTypes[shipData[slot].type].flags
                                        & kShipArmorDisableTenth) != 0)
                                    shipData[slot].armor =
                                        gShipTypes[shipData[slot].type].armor * 0.1 - 1.0;
                                else
                                    shipData[slot].armor =
                                        gShipTypes[shipData[slot].type].armor * 0.33 - 1.0;
                                if (currentMissions[i].timeLimit <= 0
                                        && currentMissions[i].timeLimit > -32000)
                                    shipData[slot].boarded = 1;
                            }
                            if (currentMissions[i].shipStart <= kNavStelFirst
                                    && currentMissions[i].shipStart >= kNavStelLast) {
                                navIndex = -1 - currentMissions[i].shipStart;
                                if (gSystems[sys].navDefault[navIndex] != -1) {
                                    shipData[slot].pos.x = stellarData[
                                        gSystems[sys].navDefault[navIndex]].pos.h;
                                    shipData[slot].pos.y = stellarData[
                                        gSystems[sys].navDefault[navIndex]].pos.v;
                                }
                            }
                            if (currentMissions[i].shipBehav == kMissionShipEscort) {
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

    for (i = 0; i < gSystems[sys].avgShips; i++) {
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
            speed = gShipTypes[shipData[slot].type].maxSpeed;
            AdjustedAccel(shipData[slot].heading, speed,
                          &shipData[slot].vel, speed);
        }
    }

    for (i = 0; i < kSystemPersSlots; i++) {
        if (gSystems[sys].persons[i] != -1
                && personData[gSystems[sys].persons[i]].loaded != 0
                && EvalMissionBitTestString(personData[gSystems[sys].persons[i]].activeOn)
                && Rand(100) + 1 <= gSystems[sys].persProbs[i]) {
            slot = SpawnPerson(shipData->curSystem, 0, gSystems[sys].persons[i]);
            if (slot != -1) {
                speed = gShipTypes[shipData[slot].type].maxSpeed;
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

#define kSpawnSpread     1500    /* Rand(1500) - 750 on each axis */
#define kSpawnOrigin     750
#define kSpawnSpeed      100.0f
#define kStartCredits    10000
#define kAITypeLinked    3       /* this aiType seeds from the linked system */

/* RandomShipSpawn (mac _RandomShipSpawn). STATE: MATCH 100.0% 617/617, byte-identical yes
   (3208 bytes), relocregcheck 0 of 189. OPEN: none. */
// FUNCTION: EVNOVA 0x0041ba80
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
        dude = SelectDudeFieldFromSystem(&gSystems[sys]);
        if (dude == -1)
            continue;
        dude = gSystems[sys].dudeTypes[dude];
        pick = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (pick < 0 || pick >= kDudeShipSlots)
            goto fail;
        shipData[slot].active = 1;
        shipData[slot].curSystem = sys;
        shipData[slot].dude = dude;
        shipData[slot].type = dudeData[dude].shipTypes[pick];
        shipData[slot].govt = dudeData[dude].govt;
        if (dudeData[dude].aiType < 1)
            shipData[slot].aiType = gShipTypes[shipData[slot].type].inherentAI;
        else
            shipData[slot].aiType = dudeData[dude].aiType;
        if (shipData[slot].aiType == kAITypeLinked &&
            gSystems[shipData[slot].curSystem].navDefault[0] != -1 &&
            gShipTypes[shipData[slot].type].maxSpeed == 0.0) {
            link = gSystems[shipData[slot].curSystem].navDefault[0];
            shipData[slot].pos.x = stellarData[link].pos.h;
            shipData[slot].pos.y =
                stellarData[gSystems[shipData[slot].curSystem].navDefault[0]].pos.v;
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
        shipData[slot].escapePods = gShipTypes[shipData[slot].type].podCount;
        shipData[slot].foldSet =
            gShipTypes[shipData[slot].type].baseSets - 1;
        shipData[slot].foldDir = 0;
        shipData[slot].frameTimer = 0;
        shipData[slot].shieldGlow = 0;
        shipData[slot].weapGlow = 0;
        shipData[slot].annoyance = 0.0f;
        shipData[slot].bankDir = 0;
        shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
        shipData[slot].credits = kStartCredits;
        shipData[slot].aggression = Rand(3) ^ 2;
        shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
        shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
        shipData[slot].fuel = gShipTypes[shipData[slot].type].fuel;
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
        shipData[slot].seesCloaked = -1;
        shipData[slot].seesCloakedRadar = -1;
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
        if (gShipTypes[shipData[slot].type].baseSets > 0)
            shipData[slot].animSet =
                Rand(gShipTypes[shipData[slot].type].baseSets);
        if (gShipTypes[shipData[slot].type].animDelay > 0)
            shipData[slot].animTimer =
                Rand(gShipTypes[shipData[slot].type].animDelay);
        for (i = 0; i < kMaxWeapTypes; i++) {
            shipData[slot].weap[i].count =
                gShipTypes[shipData[slot].type].weapCount[i];
            shipData[slot].weap[i].ammo =
                gShipTypes[shipData[slot].type].weapAmmo[i];
        }
        AISetup(&shipData[slot]);
        return slot;
    }
    return -1;

fail:
    shipData[slot].active = 0;
    return -1;
}

/*
 * 0x004235c0 = SpawnPerson (mac `_SpawnPerson` at the same call position, same
 * (sys, 1, -1) arguments); 0x00425280 = SpawnFleet, call-position proven.
 */

/* HyperShipSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041c710
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
        do {
            dist += speed;
            speed -= 1.165;
        } while (speed > 0);
        shipData[slot].pos.x = shipData[slot].pos.y = 0;
        Accel(Rand(360), dist + 1000.0f, &shipData[slot].pos);
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

#define kMisnFlagFixHull 0x800   /* +0x55: the requested hull is honoured */

/* SpecificDudeSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041c9f0
short SpecificDudeSpawn(short dude, short sys, short reserve,
                        unsigned char allowAll)
{
    short slot;
    short pick;
    short i;

    slot = GenericRandomShipSpawn(sys, reserve);
    if (slot == -1)
        return -1;
    pick = SelectShipFieldFromDude(&dudeData[dude], allowAll);
    if (pick < 0 || pick >= kDudeShipSlots)
        goto fail;
    shipData[slot].dude = dude;
    shipData[slot].type = dudeData[dude].shipTypes[pick];
    shipData[slot].govt = dudeData[dude].govt;
    if (dudeData[dude].aiType < 1)
        shipData[slot].aiType = gShipTypes[shipData[slot].type].inherentAI;
    else
        shipData[slot].aiType = dudeData[dude].aiType;
    AISetup(&shipData[slot]);
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = gShipTypes[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = gShipTypes[shipData[slot].type].weapAmmo[i];
    }
    shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
    shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
    return slot;

fail:
    shipData[slot].active = 0;
    return -1;
}

/* MissionDudeSpawn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041cf40
short MissionDudeSpawn(short dude, short wantType, short sys, short mission)
{
    short slot;
    short pick;
    short i;

    slot = GenericRandomShipSpawn(sys, kSpawnReserve);
    if (slot == -1)
        return -1;
    Rand(100);
    if (wantType != -1 &&
        (currentMissions[mission].flags & kMisnFlagFixHull) != 0 &&
        currentMissions[mission].shipCount == 1) {
        for (i = 0; i < kDudeShipSlots; i++) {
            if (wantType == dudeData[dude].shipTypes[i]) {
                currentMissions[mission].shipTypeIndex = i;
                break;
            }
        }
    }
    pick = currentMissions[mission].shipTypeIndex;
    if (pick == -1) {
        pick = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (pick == -1)
            pick = SelectShipFieldFromDude(&dudeData[dude], 1);
    }
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
    if (dudeData[dude].aiType < 1)
        shipData[slot].aiType = gShipTypes[shipData[slot].type].inherentAI;
    else
        shipData[slot].aiType = dudeData[dude].aiType;
    AISetup(&shipData[slot]);
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = gShipTypes[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = gShipTypes[shipData[slot].type].weapAmmo[i];
    }
    shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
    shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
    shipData[slot].escapePods = gShipTypes[shipData[slot].type].podCount;
    shipData[slot].foldSet =
        gShipTypes[shipData[slot].type].baseSets - 1;
    shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
    if (gShipTypes[shipData[slot].type].baseSets > 0)
        shipData[slot].animSet =
            Rand(gShipTypes[shipData[slot].type].baseSets);
    if (gShipTypes[shipData[slot].type].animDelay > 0)
        shipData[slot].animTimer =
            Rand(gShipTypes[shipData[slot].type].animDelay);
    if (currentMissions[mission].shipBehav == 0)
        AIMakeShipAttackPlayer(&shipData[slot]);
    return slot;

fail:
    shipData[slot].active = 0;
    return -1;
}

#define kSystemStellars     16
#define kSysHere            (-6)
#define kMisnTimerMax       32000

#define kMisnFlagAbort      0x0001  /* +0x55: abort the mission once it fires */
#define kMisnFlagKeepAux    0x0010  /* +0x55: do not spend an aux ship credit */

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

/* EnterMoreShips (mac _EnterMoreShips). STATE: MATCH 100.0%, exe 702 / obj 702, byte-identical unchecked
   (bytediff covers baseline MATCHes only; next regate).
   OPEN: none */
// FUNCTION: EVNOVA 0x0041d6e0
void EnterMoreShips(short sys)
{
    FloatPoint origin;
    FloatPoint spot;
    short count;
    short i;
    short toSpawn;
    short here;
    short n;
    short which;
    short heading;
    FloatPoint dest;
    short j;
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
                    toSpawn = currentMissions[i].auxShipsLeft - currentMissions[i].auxShipsSpawned;
                    if (ValidAuxShipSystem(shipData[0].curSystem, i)
                        && toSpawn > 0) {
                        dist = 0.0f;
                        for (step = kEntryStep; step > 0.0f;
                             step = step - kEntryStepDecay)
                            dist = dist + step;
                        heading = Rand(kEntryHeadings);
                        spot.x = spot.y = 0.0f;
                        Accel(heading, dist + kEntryRadius, &spot);
                        for (count = 0; count < toSpawn; count++) {
                            which = SpecificDudeSpawn(currentMissions[i].auxShipDude, sys,
                                                      kSpawnReserve, 0);
                            if (which != -1) {
                                shipData[which].auxMission = i;
                                currentMissions[i].auxShipsSpawned += 1;
                                if ((currentMissions[i].flags & kMisnFlagKeepAux) == 0)
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
                        if (currentMissions[i].flags & kMisnFlagAbort)
                            AutoAbortMission(i);
                    }
                }
            }
            toSpawn = 0;
            count = 0;
            if (currentMissions[i].shipSyst < 0 || currentMissions[i].shipSyst >= kMaxSystems)
                here = -1;
            else
                here = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            if (currentMissions[i].shipSyst == kSysHere
                || here == shipData[0].curSystem)
                toSpawn = currentMissions[i].shipCount;
            if (currentMissions[i].shipStart == 1) {
                toSpawn = currentMissions[i].shipCount - currentMissions[i].specialShipsJumpedIn;
                if (currentMissions[i].shipSyst != kSysHere
                    && here != shipData[0].curSystem
                    && currentMissions[i].specialShipsJumpedIn <= 0)
                    toSpawn = 0;
            }
            if (toSpawn > 0) {
                if (currentMissions[i].shipDelay > 0
                    && currentMissions[i].shipDelay <= kMisnTimerMax)
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
                            gSystems[shipData[0].curSystem].xPos;
                        origin.y =
                            gSystems[shipData[0].curSystem].yPos;
                        dest.x =
                            gSystems[shipData[0].prevSystem].xPos;
                        dest.y =
                            gSystems[shipData[0].prevSystem].yPos;
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
                    if (currentMissions[i].flags & kMisnFlagAbort)
                        AutoAbortMission(i);
                }
            }
            if (currentMissions[i].auxShipDelay > 0
                && currentMissions[i].auxShipDelay <= kMisnTimerMax)
                currentMissions[i].auxShipDelay -= 1;
        }
    }

    for (n = 1; n < kMaxShips; n++)
        if (shipData[n].active && sys == shipData[n].curSystem
            && shipData[n].parentShip != 0)
            count++;
    if (count < gSystems[sys].avgShips) {
        roll = Rand(kFleetOdds);
        if (roll == 1) {
            if (gSystems[sys].fleetCount > 0
                && Rand(kFleetProbRange) + 1 <= gSystems[sys].fleetProbTotal) {
                pick = SelectFleetFromSystem(&gSystems[sys]);
                if (pick != -1)
                    HyperSpawnFleet(sys, pick, -1, 1);
                else
                    roll = 0;
            } else {
                roll = 0;
            }
        }
        if (roll == 0) {
            pick = SelectDudeFieldFromSystem(&gSystems[sys]);
            if (pick != -1)
                HyperShipSpawn(sys);
        }
    }

    for (j = 0; j < kSystemStellars; j++) {
        if (gSystems[sys].navDefault[j] != -1
            && stellarData[gSystems[sys].navDefault[j]].defenseActive != 0
            && stellarData[gSystems[sys].navDefault[j]].defWaveSize > 0) {
            count = 0;
            for (n = 1; n < kMaxShips; n++)
                if (shipData[n].active && shipData[n].defenseStellar != -1
                    && shipData[n].defenseStellar == gSystems[sys].navDefault[j])
                    count++;
            pick = stellarData[gSystems[sys].navDefault[j]].defCount % kDefWaveModulus;
            if (count < pick) {
                SpawnStellarDefenseShip(gSystems[sys].navDefault[j]);
                stellarData[gSystems[sys].navDefault[j]].defWaveSize -= 1;
                break;
            }
        }
    }

    SpawnNagShip();
    if (antiEscortDelay == 0) {
        cargo = 0;
        antiEscortDelay = -1;
        for (n = 1; n < kMaxShips; n++)
            if (shipData[n].active && shipData[n].parentShip == 0
                && shipData[n].aiType == 6 && shipData[n].specialMission == -1
                && gShipTypes[shipData[n].type].inherentAI <= 2)
                cargo += gShipTypes[shipData[n].type].cargo;
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
    ship->formPosX = 0;
    ship->formPosY = 0;
    shipData->hasFormation = 1;
    ship->vel.x = ship->vel.y = 0;
    ship->accelMag = 0;
    if (full) {
        for (i = 0; i < 256; i++) {
            ship->weap[i].count = gShipTypes[ship->type].weapCount[i];
            ship->weap[i].ammo = gShipTypes[ship->type].weapAmmo[i];
        }
    }
    ship->cargo[0] = 0;
    ship->cargo[1] = 0;
    ship->cargo[2] = 0;
    ship->cargo[3] = 0;
    ship->cargo[4] = 0;
    ship->cargo[5] = 0;
    AIMakeEscortFlyInForm(ship);
}

/*
 * The fighterType parameter is the WEAPON index of the carrier's bay: the
 * spawned hull is that weapon's ammoType - 128, the heading jitter is its
 * inaccuracy, the launch speed its speed100, and unk4C its lifetime.
 */

/* LaunchFighter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041e640
unsigned char LaunchFighter(ShipObj *parent, short fighterType)
{
    short slot;
    short i;
    short n;
    short inacc;

    if (fighterType < 0 || fighterType > 256)
        return 0;
    slot = GenericRandomShipSpawn(parent->curSystem, kSpawnReserve);
    if (slot == -1)
        return 0;
    shipData[slot].active = 1;
    shipData[slot].curSystem = parent->curSystem;
    shipData[slot].pos.x = parent->pos.x;
    shipData[slot].pos.y = parent->pos.y;
    shipData[slot].vel.x = parent->vel.x;
    shipData[slot].vel.y = parent->vel.y;
    shipData[slot].accelMag = parent->accelMag;
    shipData[slot].type = gWeapons[fighterType].ammoType - 128;
    shipData[slot].defenseStellar = -1;
    shipData[slot].tractorShip = -1;
    shipData[slot].hailed = 0;
    shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
    shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
    shipData[slot].hired = 0;
    shipData[slot].auxMission = -1;
    shipData[slot].aiType = 5;
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
    shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
    shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
    shipData[slot].aiTimer = gWeapons[fighterType].lifetime;
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
    shipData[slot].escapePods = gShipTypes[shipData[slot].type].podCount;
    shipData[slot].foldSet =
        gShipTypes[shipData[slot].type].baseSets - 1;
    shipData[slot].foldDir = 0;
    shipData[slot].frameTimer = 0;
    shipData[slot].shieldGlow = 0;
    shipData[slot].weapGlow = 0;
    shipData[slot].annoyance = 0;
    shipData[slot].bankDir = 0;
    shipData[slot].animTimer = 0;
    shipData[slot].seesCloaked = -1;
    shipData[slot].seesCloakedRadar = -1;
    shipData[slot].decloaksWhenHit = -1;
    shipData[slot].escortOrder = -1;
    shipData[slot].sellOnLanding = 0;
    shipData[slot].upgradeOnLanding = 0;
    shipData[slot].fuelRegenState = -1;
    shipData[slot].escortMayAttack = 0;
    shipData[slot].voiceClass = Rand(2);
    shipData[slot].shield = ShipShieldCapacity(&shipData[slot]);
    shipData[slot].armor = ShipArmorCapacity(&shipData[slot]);
    if (gShipTypes[shipData[slot].type].inherentGovtB != -1
        && govtData[gShipTypes[shipData[slot].type].inherentGovtB].voiceClass != -1)
        shipData[slot].voiceClass =
            govtData[gShipTypes[shipData[slot].type].inherentGovtB].voiceClass;
    for (i = 0; i < 4; i++)
        shipData[slot].jamming[i] = -1;
    if (gShipTypes[shipData[slot].type].baseSets > 0)
        shipData[slot].animSet =
            Rand(gShipTypes[shipData[slot].type].baseSets);
    if (gShipTypes[shipData[slot].type].animDelay > 0)
        shipData[slot].animTimer =
            Rand(gShipTypes[shipData[slot].type].animDelay);
    if (shipData[slot].parentShip == 0
        && gShipTypes[shipData[slot].type].escortType == 0) {
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
                && gShipTypes[shipData[i].type].escortType ==
                   gShipTypes[shipData[slot].type].escortType
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
    inacc = gWeapons[fighterType].inaccuracy;
    if (inacc > 0)
        shipData[slot].heading += Rand(inacc * 2) - inacc;
    AdjustedAccel(shipData[slot].heading, gWeapons[fighterType].speed100,
                  &shipData[slot].vel,
                  ShipMaxSpeed(&shipData[slot]));
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = gShipTypes[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = gShipTypes[shipData[slot].type].weapAmmo[i];
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
}

#define kUnitsPerBox        5.0      /* pooled double */
#define kMaxBoxesPerShip    0xc

#define kMsgMissionAborted  0x11c
#define kMsgJettisonPlain   0x122
#define kMsgJettisonMission 0x121

#define kMissionFlagQuiet   0x400
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
void JettisonCargo(unsigned char withMissions)
{
    short boxes;
    short total;
    short share;
    short escortCargo;
    double holds;
    double fleetHolds;
    double units;
    short i;
    short j;
    short k;
    unsigned char text[256];
    char toldMission;

    toldMission = 0;
    total = 0;
    total += shipData[0].cargo[0];
    total += shipData[0].cargo[1];
    total += shipData[0].cargo[2];
    total += shipData[0].cargo[3];
    total += shipData[0].cargo[4];
    total += shipData[0].cargo[5];
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
                if ((currentMissions[k].flags & kMissionFlagQuiet) == 0
                    && playerIsLanded == 0) {
                    GetIndString(text, kGameStrList, kMsgMissionAborted);
                    DisplayComm(text, kCommWidth, 0, kCommHold, &rgbWhite, 0, 0);
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
             && shipData[i].aiType == 6 && shipData[i].specialMission == -1
             && gShipTypes[shipData[i].type].inherentAI <= 2)
            || i == 0) {
            if (i == 0)
                holds = ShipTotalHolds();
            else
                holds = gShipTypes[shipData[i].type].cargo;
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
                statusRedraw[2] = 1;
            }
        }
    }
    shipData[0].cargo[0] = 0;
    shipData[0].cargo[1] = 0;
    shipData[0].cargo[2] = 0;
    shipData[0].cargo[3] = 0;
    shipData[0].cargo[4] = 0;
    shipData[0].cargo[5] = 0;
    if (total > 0 && playerIsLanded == 0) {
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
        if (toldMission == 0) {
            if (withMissions)
                GetIndString(text, kGameStrList, kMsgJettisonMission);
            else
                GetIndString(text, kGameStrList, kMsgJettisonPlain);
            DisplayComm(text, kCommWidth, 0, kCommHold, &rgbWhite, 0, 0);
        }
    }
    ResetPlayerPrecalcedValues();
}

/* SpawnBox. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041f800
void SpawnBox(short from)
{
    short i;
    short slot;
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
          (Rand(kBoxKickSpread) + kBoxKickBase) / kBoxKickScale,
          &boxData[slot].vel);
}

/* boxData (0x005914a8) is 64 records of the 0x28 stride both twins index by.
   The four spin tests are INDEPENDENT ifs, not an else-if chain: each arm is
   parked out of line and jumps back to the NEXT test rather than to the join. */


/* SpawnScoopableBox. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0041fb50
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
        Accel(Rand(360), (Rand(81) + 60) * 0.2 * 0.01, &boxData[slot].vel);
        return;
    }
}

/* SpawnShot (mac _SpawnShot). STATE: MATCH 100%, exe 970 / obj 970, relocregcheck 0 of 179;
   byte identity not yet measured (bytediff reads the adopted baseline).
   OPEN: none. The jitter blocks read `inaccuracy' at each use; a block-scope `inc' copy was
   an invented entity (notes, 2026-09-24). */
// FUNCTION: EVNOVA 0x0041fd30
short SpawnShot(short ship, short target, short weapType, char flag)
{
    char intercept;
    short collision;
    short slot;
    short angle;
    short guidance;
    short i;

    if (weapType < 0 || weapType >= kMaxWeapTypes) {
        return -1;
    }
    if (gWeapons[weapType].graphic < 0 || gWeapons[weapType].graphic >= kWeapTypeTop) {
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
        guidance = gWeapons[weapType].guidance;
        if (guidance == kGuidanceTurret || (unsigned short)(guidance - kGuidanceFrontQuad) <= 1) {
            if (target == -1) {
                if (guidance == kGuidanceFrontQuad) {
                    angle = shipData[ship].heading;
                }
            }
            else {
                angle = CalcInterceptAngle(&shipData[ship].pos,
                                           &shipData[target].pos);
                intercept = 1;
            }
        }
        if (ship == 0) {
            guidance = gWeapons[weapType].guidance;
            if (guidance != kGuidanceTurret && guidance != kGuidanceFrontQuad
                    && guidance != kGuidanceRearQuad) {
                shotData[slot].vel.x = shipData[ship].vel.x;
                shotData[slot].vel.y = shipData[ship].vel.y;
            }
            if (gWeapons[weapType].guidance == kGuidanceFreefall) {
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
    shotData[slot].life = gWeapons[weapType].lifetime;
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
        guidance = gWeapons[weapType].guidance;
        if (guidance == kGuidanceTurret || guidance == kGuidancePointDef) {
            if (gShipTypes[shipData[ship].type].aiSkill > 0) {
                shotData[slot].hitRoll =
                    Rand(gShipTypes[shipData[ship].type].aiSkill) + 1;
            }
            else {
                shotData[slot].hitRoll = Rand(100) + 1;
            }
        }
        else {
            shotData[slot].hitRoll = -1;
        }
    }

    if ((gWeapons[weapType].flags & 4) == 0) {
        shotData[slot].frame = Rand(0x24);
    }
    else {
        shotData[slot].frame = 0;
    }

    shotData[slot].frameTimer = 0;
    shotData[slot].trailCount = 0;
    if (gWeapons[weapType].decay > 0) {
        shotData[slot].trailTimer = 0.0f;
    }
    else {
        shotData[slot].trailTimer = -1.0f;
    }

    /* an AI that is trying to disable its prey marks the shot disable-only */
    if (0 < ship && ship < kMaxShips &&
        AIIsShipAttemptingDisable(&shipData[ship]) != 0) {
        shotData[slot].disableOnly = 1;
    }

    if (0 <= ship && ship < kMaxShips) {
        shotData[slot].curSystem = shipData[ship].curSystem;
    }
    else {
        shotData[slot].curSystem = shipData[0].curSystem;
    }

    shotData[slot].angle = angle;
    shotData[slot].guidanceState = 0;

    /* guidance-1 "one shot" roll against the player system's explore level */
    if (gWeapons[weapType].guidance == kGuidanceHoming && (gWeapons[weapType].seeker & 8) != 0) {
        short roll;
        if (gTimeScale > 0.0) {
            roll = (100.0 / gTimeScale);
        }
        else {
            roll = 100;
        }
        if (roll < 1) {
            roll = 1;
        }
        if (Rand(roll) + 1 <= gSystems[shipData[0].curSystem].interference) {
            shotData[slot].guidanceState = 999;
        }
    }

    if (gWeapons[weapType].durability > 0) {
        shotData[slot].energy = gWeapons[weapType].durability;
    }
    else {
        shotData[slot].energy = 0;
    }

    /* where the round actually starts */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        if (target == -1) {
            angle = ModifyShotStartPosition(&shipData[ship],
                                           &shotData[slot].pos,
                                           weapType, NULL);
        }
        else {
            angle = ModifyShotStartPosition(&shipData[ship],
                                           &shotData[slot].pos,
                                           weapType,
                                           &shipData[target].pos);
        }
        if (gWeapons[weapType].inaccuracy < 0 && angle != -1) {
            short mount = gWeapons[weapType].exitType;
            if (0 <= mount && mount <= 3) {
                short v = gShipTypes[shipData[ship].type].weapPos[mount][angle][0];
                if (v < 0) {
                    collision = -1;
                }
                else if (v > 0) {
                    collision = 1;
                }
            }
        }
        if (intercept != 0) {
            short lead = CalcLeadAngle(&shipData[ship], &shipData[target],
                                       weapType,
                                       &shotData[slot].pos);
            shotData[slot].angle = lead;
        }
    }

    /* inaccuracy jitter, first of the two identical blocks (guidance != kGuidanceFreefall) */
    if (0 < gWeapons[shotData[slot].weapType].inaccuracy
            && gWeapons[shotData[slot].weapType].guidance != kGuidanceFreefall) {
        shotData[slot].angle =
            (Rand(gWeapons[shotData[slot].weapType].inaccuracy * 2)
                    - gWeapons[shotData[slot].weapType].inaccuracy)
            + shotData[slot].angle;
        if (shotData[slot].angle < 0.0f) {
            shotData[slot].angle = shotData[slot].angle + 360.0f;
        }
        if (shotData[slot].angle >= 360.0f) {
            shotData[slot].angle = shotData[slot].angle - 360.0f;
        }
    }

    /* the speed vector */
    guidance = gWeapons[shotData[slot].weapType].guidance;
    if ((guidance != kGuidanceFreefall && guidance != kGuidanceRocket) || 0 < ship) {
        if (0 <= ship && ship < kMaxShips && collision != 0) {
            float f;
            if (collision == 1) {
                f = abs(gWeapons[shotData[slot].weapType].inaccuracy)
                    + shipData[ship].heading;
            }
            else {
                f = shipData[ship].heading
                    - abs(gWeapons[shotData[slot].weapType].inaccuracy);
            }
            while (f < 0.0) {
                f = f + 360.0;
            }
            while (f >= 360.0) {
                f = f - 360.0;
            }
            Accel(f, gWeapons[weapType].speed100,
                  &shotData[slot].vel);
        }
        else {
            Accel(shotData[slot].angle, gWeapons[weapType].speed100,
                  &shotData[slot].vel);
        }
    }

    /* inaccuracy jitter, second of the two identical blocks (guidance == kGuidanceFreefall) */
    if (0 < gWeapons[shotData[slot].weapType].inaccuracy
            && gWeapons[shotData[slot].weapType].guidance == kGuidanceFreefall) {
        shotData[slot].angle =
            (Rand(gWeapons[shotData[slot].weapType].inaccuracy * 2)
                    - gWeapons[shotData[slot].weapType].inaccuracy)
            + shotData[slot].angle;
        if (shotData[slot].angle < 0.0f) {
            shotData[slot].angle = shotData[slot].angle + 360.0f;
        }
        if (shotData[slot].angle >= 360.0f) {
            shotData[slot].angle = shotData[slot].angle - 360.0f;
        }
    }

    /*
     * The "clear the ship" block.  The win exedis compares gWeapons[shotData
     * [slot].weapType].+0x08 (the header's `ammoType`) against -999, not the
     * header-documented -9999 sentinel on `guidance` (+0x06).  Named for the
     * header field it reads; the -9999/-999 split is a live TBC.
     */
    if (gWeapons[shotData[slot].weapType].ammoType == -999 &&
        0 <= ship && ship < kMaxShips && flag == 0) {
        shipData[ship].shield = 0;
        shipData[ship].armor = 0;
        statusRedraw[6] = 1;
    }

    /* the four glow rolls */
    for (i = 0; i < 4; i++) {
        if (gWeapons[shotData[slot].weapType].jamVuln[i] <= 0) {
            shotData[slot].glow[i] = 0;
        }
        else {
            shotData[slot].glow[i] =
                Rand(gWeapons[shotData[slot].weapType].jamVuln[i] + 1);
        }
    }

    /* sprite layer */
    guidance = gWeapons[weapType].guidance;
    if (guidance == kGuidanceHoming) {
        PutSpriteInLayer(shotData[slot].sprite, guidedShotLayer);
    }
    else if (guidance == kGuidancePointDef) {
        PutSpriteInLayer(shotData[slot].sprite, pointDefenseLayer);
    }
    else if (guidance == kGuidanceTurret) {
        if (ship < 0 || ship >= kMaxShips) {
            PutSpriteInLayer(shotData[slot].sprite, shotLayer);
        }
        else if ((gShipTypes[shipData[ship].type].flags3 & 0x40) != 0) {
            PutSpriteInLayer(shotData[slot].sprite, turretedShotLayer);
        }
        else {
            PutSpriteInLayer(shotData[slot].sprite, shotLayer);
        }
    }
    else {
        PutSpriteInLayer(shotData[slot].sprite, shotLayer);
    }

    if (gWeapons[weapType].guidance == kGuidancePointDef) {
        SWSetSpriteCollideProc(shotData[slot].sprite, PointDefenseCollisionHandler);
    }
    else {
        SWSetSpriteCollideProc(shotData[slot].sprite, 0);
    }

    SWCloneFramesFromMaster(shotData[slot].sprite,
                 masterShotSprite[gWeapons[shotData[slot].weapType].graphic]);

    return slot;
}

#define kWeapFlagsBit4          0x10  /* flags bit 4 gates the do-not-stack sound */
#define kWeap22Bit4             0x10  /* flags2 bit 4: the child picks its own target */

/* SpawnShotSubmunitions. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00420d30
void SpawnShotSubmunitions(ShotObj *shot, short ship)
{
    short subType;
    short n;
    short slot;

    if (gWeapons[shot->weapType].subType == -1
            || gWeapons[shot->weapType].subCount <= 0)
        return;
    if (gWeapons[shot->weapType].subLimit > 0
            && shot->subDepth >= gWeapons[shot->weapType].subLimit)
        return;
    subType = gWeapons[shot->weapType].subType;

    /* the launch sound, played at most once per volley */
    if (gWeapons[subType].sound >= 0 && gWeapons[subType].sound < kWeapTypeTop
            && shotSnd[gWeapons[subType].sound] != 0) {
        if ((gWeapons[subType].flags & kWeapFlagsBit4) != 0) {
            if (ST_IsSoundPlaying(shotSnd[gWeapons[subType].sound]) == 0)
                PlaySoundDistance(-1, shotSnd[gWeapons[subType].sound], 4,
                                &shot->pos, &shipData[0].pos);
        }
        else {
            PlaySoundDistance(-1, shotSnd[gWeapons[subType].sound], 4,
                            &shot->pos, &shipData[0].pos);
        }
    }

    for (n = 0; n < gWeapons[shot->weapType].subCount; n++) {
        slot = SpawnShot(shot->owner, ship, subType, 1);
        if (slot == -1)
            break;

        shotData[slot].pos.x = shot->pos.x;
        shotData[slot].pos.y = shot->pos.y;
        shotData[slot].vel.x = 0;
        shotData[slot].vel.y = 0;
        shotData[slot].subDepth = shot->subDepth + 1;
        shotData[slot].angle = shot->angle;

        if (gWeapons[shot->weapType].guidance == kGuidancePointDef) {
            shotData[slot].target = shot->target;
        }
        else if ((gWeapons[shot->weapType].flags2 & kWeap22Bit4) != 0) {
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
        if (gWeapons[shot->weapType].subTheta > 0) {
            shotData[slot].angle = (gWeapons[shot->weapType].subTheta
                                           - Rand(gWeapons[shot->weapType].subTheta * 2 + 1))
                                   + shotData[slot].angle;
        }
        else if (gWeapons[shot->weapType].subTheta < 0) {
            shotData[slot].angle = shotData[slot].angle
                - (abs(gWeapons[shot->weapType].subTheta)
                          * (gWeapons[shot->weapType].subCount - 1) / 2);
            shotData[slot].angle = (abs(gWeapons[shot->weapType].subTheta) * n)
                                   + shotData[slot].angle;
        }
        if (shotData[slot].angle < 0.0f)
            shotData[slot].angle = shotData[slot].angle + 360.0f;
        if (shotData[slot].angle >= 360.0f)
            shotData[slot].angle = shotData[slot].angle - 360.0f;

        if (gWeapons[shotData[slot].weapType].guidance == kGuidanceRocket) {
            {
                FloatPoint *v = &shotData[slot].vel;
                v->x = shot->vel.x;
                v->y = shot->vel.y;
            }
        }
        else {
            Accel(shotData[slot].angle,
                  gWeapons[shotData[slot].weapType].speed100,
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
void CreateExplosion(FloatPoint pt, short type, short size, char sound)
{
    /* FloatPoint spelling: the definition takes this width. */
    extern void SpawnExplod(FloatPoint pos, short unused, short type,
                            short angle);

    FloatPoint pos;
    double spread;
    double half;
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
        i = 0;
        if (bigCount > 0) {
            spread = size * kScatterSpread;
            half = size * kBigScatterSpread;
            do {
                pos.x = Rand(spread) + pt.x - half;
                pos.y = Rand(spread) + pt.y - half;
                SpawnExplod(pos, -1, 1, Rand(kBigAngleRange) + kBigAngleBase);
                i++;
            } while (i < bigCount);
        }
        smallCount = (size * kSmallScatterFraction);
        j = 0;
        if (smallCount > 0) {
            spread = size * kScatterSpread;
            do {
                pos.x = Rand(sz) + pt.x - spread;
                pos.y = Rand(sz) + pt.y - spread;
                SpawnExplod(pos, -1, 0, Rand(kSmallAngleRange) + kSmallAngleBase);
                j++;
            } while (j < smallCount);
        }
    }
    SpawnExplod(pt, -1, type, 0);
    if (sound != 0) {
        snd = explodType[type].sound;
        if (snd >= 0 && snd < kMaxShips)
            PlaySoundDistance(-1, explodSnd[snd], 6, &pt, &shipData->pos);
    }
}

#define kMaxActiveBooms 32

/* SpawnExplod (mac _SpawnExplod). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00421500
void SpawnExplod(float a, register float b, short unused, short type, short delay)
{
    short i;

    for (i = 0; i < kMaxActiveBooms; i++) {
        if (explodData[i].timer < 0.0f) {
            explodData[i].type = type;
            explodData[i].timer = 0;
            explodData[i].delay = delay;
            explodData[i].pos.x = a;
            explodData[i].pos.y = b;
            SWCloneFramesFromMaster(explodData[i].sprite,
                         masterExplodSprite[explodType[type].graphic]);
            break;
        }
    }
}


/* SpawnSmoke (mac _SpawnSmoke). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004215d0
void SpawnSmoke(float a, register float b, long type, short sprite)
{
    short i;

    if (gNoDebris != 0)
        return;
    for (i = 0; i < kMaxDebris; i++) {
        if (gDebrisSlots[i].timer < 0.0f) {
            if (masterSmokeSprite[sprite] != 0) {
                gDebrisSlots[i].type = type;
                gDebrisSlots[i].timer = 0;
                gDebrisSlots[i].angle = 0;
                gDebrisSlots[i].pos.x = a;
                gDebrisSlots[i].pos.y = b;
                SWCloneFramesFromMaster(gDebrisSlots[i].sprite, masterSmokeSprite[sprite]);
                SWLockSprite(gDebrisSlots[i].sprite);
                break;
            }
        }
    }
}

/* CreateAsteroids. STATE: MATCH 100%, exe 105 / obj 105 instrs.
   OPEN: none. The half extents are `w * 0.5' at each use; the halfW/halfH doubles were
   invented (notes, 2026-09-24). */
// FUNCTION: EVNOVA 0x004216b0
void CreateAsteroids(void)
{
    short i;
    short w;
    short h;

    if (gSystems[shipData[0].curSystem].asteroidCount > 0) {
        for (i = 0; i < gSystems[shipData[0].curSystem].asteroidCount; i++)
            SpawnAsteroid(0);
        w = screenCenter.h + 128;
        h = screenCenter.v + 128;
        for (i = 0; i < kMaxAsteroids; i++) {
            asteroidData[i].pos.x = Rand(w) + shipData[0].pos.x - w * 0.5;
            asteroidData[i].pos.y = Rand(h) + shipData[0].pos.y - h * 0.5;
            asteroidData[i].vel.x = (Rand(400) - 200) * 0.01;
            asteroidData[i].vel.y = (Rand(400) - 200) * 0.01;
        }
    } else {
        killAsteroids = 1;
    }
}

/* SpawnAsteroid. STATE: MATCH. OPEN: none */
// FUNCTION: EVNOVA 0x00421830
void SpawnAsteroid(long type)
{
    long parity;
    short w;
    short h;
    float radius;
    unsigned long long mem;
    unsigned long diff;
    short i;
    short active;

    if (gSystems[shipData[0].curSystem].asteroidCount <= 0
        || gSystems[shipData[0].curSystem].astTypes == 0)
        return;

    active = 0;
    for (i = 0; i < kMaxAsteroids; i++)
        if (asteroidData[i].active != 0)
            active++;

    if (active >= gSystems[shipData[0].curSystem].asteroidCount)
        return;

    mem = *(unsigned long long *)((char *)shipData + 0x4b7a0);
    parity = 0;
    diff = mem;
    diff ^= mem >> 32;
    while (diff) {
        diff &= diff - 1;
        parity = 1 - parity;
    }
    gShipTypes[gameIsHackedIndex].hacked =
        (*(long *)((char *)shipData + 0x1d1760) != 0 && (char)parity == 0)
            ? (unsigned char)1 : (unsigned char)0;

    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].active == 0) {
            asteroidData[i].active = 1;

            w = screenCenter.h + 128;
            h = screenCenter.v + 128;

            if ((char)type != 0) {
                if (w > h)
                    radius = w * 0.7;
                else
                    radius = h * 0.7;
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
                asteroidData[i].pos.x = Rand(w) + shipData[0].pos.x - w * 0.5;
                asteroidData[i].pos.y = Rand(h) + shipData[0].pos.y - h * 0.5;
                asteroidData[i].vel.x = (Rand(400) - 200) * 0.01;
                asteroidData[i].vel.y = (Rand(400) - 200) * 0.01;
            }

            do {
                asteroidData[i].type = Rand(16);
            } while (!(gSystems[shipData[0].curSystem].astTypes & (1 << asteroidData[i].type)));

            asteroidData[i].frame = Rand(masterAsteroidSprite[asteroidData[i].type]->numFrames);
            asteroidData[i].frameRate = (Rand(41) + 80) * asteroidType[asteroidData[i].type].frameRate * 0.01;
            if (Rand(2) == 0)
                asteroidData[i].frameRate *= -1.0;
            asteroidData[i].strength = asteroidType[asteroidData[i].type].strength;
            break;
        }
    }
}

/* SpawnSubAsteroid. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00421e60
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
            asteroidData[i].frameRate = (Rand(41) + 80) * asteroidType[type].frameRate * 0.01;
            if (Rand(2) == 0)
                asteroidData[i].frameRate *= -1.0;
            asteroidData[i].strength = asteroidType[type].strength;
            break;
        }
    }
}

/* SpawnStellarDefenseShip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00421fd0
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
            shipData[ship].heading = Rand(360);
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
char SufficentTargetedDamage(ShipObj *ship)
{
    if (ship->incomingDamage >= (ship->shield + ship->armor) * 1.05)
        return 1;
    return 0;
}

/* CalculateTargetedDamage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00422210
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
                        (gWeapons[shotData[j].weapType].massDmg
                         + gWeapons[shotData[j].weapType].energyDmg) * 0.5;
            }
        }
    }
}

#define kNudgeBase        0x32
#define kNudgeSpread      0x32

/* MakeEscortsHyperWithParent (mac _MakeEscortsHyperWithParent). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00422340
void MakeEscortsHyperWithParent(ShipObj *ship)
{
    short i;

    for (i = 1; i < 64; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == ship->index &&
            shipData[i].defenseStellar == -1) {
            shipData[i].jumpTimer = shipData[shipData[i].parentShip].jumpTimer;
            shipData[i].jumpTick = shipData[shipData[i].parentShip].jumpTick;
            shipData[i].targetShip = -2;
            AIMakeShipJumpWithParent(&shipData[i]);
        }
    }
}

/* SpawnEscortOfType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00422400
short SpawnEscortOfType(short shipType, short fromSpob)
{
    short slot;
    short k;

    slot = GenericRandomShipSpawn(shipData->curSystem, kSpawnReserve);
    if (slot != -1 && shipType > -1) {

        shipData[slot].type = shipType;
        shipData[slot].aiType = 6;
        shipData[slot].parentShip = 0;
        shipData[slot].govt = -1;
        shipData[slot].aggression = 2;
        shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
        shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
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
        for (k = 0; k < 4; k++)
            shipData[slot].jamming[k] = -1;

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
            shipData[slot].weap[k].count = gShipTypes[shipType].weapCount[k];
            shipData[slot].weap[k].ammo = gShipTypes[shipType].weapAmmo[k];
        }

        AISetup(&shipData[slot]);
        AIMakeEscortFlyInForm(&shipData[slot]);
        return slot;
    }
    return -1;
}

#define kHiredEscort         6           /* ShipObj.aiType */

#define kStrEscortOne      298
#define kStrEscortMany     299
#define kStrSoldFor        300
#define kStrUpgradedFor    301

/* DoEscortLand (mac _DoEscortLand). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004229d0
void DoEscortLand(void)
{
    short j;
    short i;
    long upgradeCost;
    long saleValue;
    short numSold;
    short numUpgraded;
    short type;
    short days;
    long total;
    short k;

    numUpgraded = 0;
    numSold = 0;
    saleValue = 0;
    upgradeCost = 0;

    if (SpobHasShipyard(&stellarData[shipData[0].targetStellar])) {
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active
                && shipData[i].parentShip == 0
                && !IsDisabled(&shipData[i])
                && shipData[i].aiType == kHiredEscort
                && shipData[i].sellOnLanding
                && shipData[i].specialMission == -1) {
                shipData[0].credits += gShipTypes[shipData[i].type].tradeIn;
                saleValue += gShipTypes[shipData[i].type].tradeIn;
                shipData[i].active = 0;
                shipData[i].parentShip = -1;
                statusRedraw[2] = 1;
                numSold++;
            }
        }

        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active
                && shipData[i].parentShip == 0
                && shipData[i].aiType == kHiredEscort
                && shipData[i].upgradeOnLanding) {
                type = shipData[i].type;
                if (gShipTypes[type].upgradeTo != -1
                    && shipData[i].specialMission == -1
                    && shipData[0].credits >= gShipTypes[type].escUpgradeCost) {
                    numUpgraded++;
                    upgradeCost += gShipTypes[type].escUpgradeCost;
                    shipData[0].credits -= gShipTypes[type].escUpgradeCost;
                    statusRedraw[2] = 1;
                    shipData[i].type = gShipTypes[shipData[i].type].upgradeTo;
                    for (j = 0; j < kMaxWeapTypes; j++) {
                        shipData[i].weap[j].count = 0;
                        shipData[i].weap[j].ammo = 0;
                    }
                    shipData[i].upgradeOnLanding = 0;
                }
            }
            shipData[i].shield = gShipTypes[shipData[i].type].shield;
            shipData[i].armor = gShipTypes[shipData[i].type].armor;
            for (j = 0; j < kMaxWeapTypes; j++)
                shipData[i].weap[j].ammo = gShipTypes[shipData[i].type].weapAmmo[j];
        }

        if (numSold > 0 || numUpgraded > 0) {
            strcpy(gTextShowString, "");
            if (numSold > 0) {
                StrcatOrdinalNumber(numSold, gTextShowString, 1);
                strcat(gTextShowString, " ");
                if (numSold == 1)
                    AppendIndString(gTextShowString, kGameStrList, kStrEscortOne);
                else
                    AppendIndString(gTextShowString, kGameStrList, kStrEscortMany);
                strcat(gTextShowString, " ");
                AppendIndString(gTextShowString, kGameStrList, kStrSoldFor);
                strcat(gTextShowString, " ");
                StrcatCommaNumber(saleValue, gTextShowString);
                strcat(gTextShowString, " ");
                if (saleValue > 1)
                    strcat(gTextShowString, creditsCString);
                else
                    strcat(gTextShowString, creditCString);
                strcat(gTextShowString, ".");
                if (numUpgraded > 0)
                    strcat(gTextShowString, "\r\r");
            }
            if (numUpgraded > 0) {
                StrcatOrdinalNumber(numUpgraded, gTextShowString, 1);
                strcat(gTextShowString, " ");
                if (numUpgraded == 1)
                    AppendIndString(gTextShowString, kGameStrList, kStrEscortOne);
                else
                    AppendIndString(gTextShowString, kGameStrList, kStrEscortMany);
                strcat(gTextShowString, " ");
                AppendIndString(gTextShowString, kGameStrList, kStrUpgradedFor);
                strcat(gTextShowString, " ");
                StrcatCommaNumber(upgradeCost, gTextShowString);
                strcat(gTextShowString, " ");
                if (upgradeCost > 1)
                    strcat(gTextShowString, creditsCString);
                else
                    strcat(gTextShowString, creditCString);
                strcat(gTextShowString, ".");
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
                for (k = 0; k < days; k++)
                    IncrementGameTime();
            }
        }
    }
    DoEscortPayment(1);
}

/* DoEscortPayment (mac _DoEscortPayment). STATE: MATCH 100.0%, exe 143 / obj 143,
   byte-identical yes. OPEN: none; `idx' was an entity the exe never had (see notes). */
// FUNCTION: EVNOVA 0x004232d0
void DoEscortPayment(short days)
{
    short day;
    short numDismissed;
    short type;
    long cost;
    char skip;
    short i;

    numDismissed = 0;
    for (day = 0; day < days; day++) {
        for (i = 1; i < 64; i++) {
            if (shipData[i].active == 0) continue;
            if (shipData[i].parentShip != 0) continue;
            if (shipData[i].aiType != 6) continue;
            if (IsDisabled(&shipData[i])) continue;

            skip = 0;
            if (shipData[i].specialMission != -1) {
                if (currentMissionObjectives[shipData[i].specialMission].active != 0) {
                    if (currentMissions[shipData[i].specialMission].shipBehav == 1) {
                        skip = 1;
                    }
                }
            }
            if (skip) continue;

            if (!shipData[i].hired) continue;

            type = shipData[i].type;
            cost = (gShipTypes[type].cost * 0.01);

            if (shipData[0].credits >= cost) {
                shipData[0].credits -= cost;
                statusRedraw[2] = 1;
            } else {
                numDismissed++;
                if (shipData[i].aiType == 6 && shipData[i].specialMission == -1 && gShipTypes[type].inherentAI <= 2) {
                    DestroyPartialFleetCargo(i);
                }
                statusRedraw[2] = 1;
                shipData[i].active = 0;
                shipData[i].parentShip = -1;
                shipData[i].aiType = 1;
            }
        }
    }

    if (numDismissed > 0) {
        if (hyperGamma >= 0.0) {
            hyperGamma = -1.0f;
            FadeGammaTo(100, 0);
        }
        strcpy(gTextShowString, "");
        if (numDismissed == 1) {
            AppendIndString(gTextShowString, kGameStrList, 302);
        } else {
            AppendIndString(gTextShowString, kGameStrList, 303);
        }
        statusRedraw[2] = 1;
        ShowTextDialog(0, 0, 0);
        DrawStatus();
    }
}

/* DoStellarTimePassage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00423540
void DoStellarTimePassage(void)
{
    short i;

    for (i = 0; i < 0x800; i++) {
        if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0) {
            if ((stellarData[shipData[0].targetStellar].flags2 & kSpobFlag2Bit5) == 0)
                stellarData[i].daysDominated += 1;
            shipData[0].credits += stellarData[i].tribute;
            statusRedraw[2] = 1;
        }
    }
}

/* SpawnPerson. STATE: MATCH, 555/555, relocregcheck 0 of 91; byte check pending the next
   regate (bytediff reads the baseline). OPEN: none */
// FUNCTION: EVNOVA 0x004235c0
short SpawnPerson(short sys, unsigned char noDisabled, short person)
{
    short slot;
    short pick;
    short n;
    short i;
    short j;
    char eligible[kMaxPersons];

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
                if (personData[i].linkSyst >= 0x80 && personData[i].linkSyst < 0x2710
                    && personData[i].linkSyst - 0x80 == sys)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x270f && personData[i].linkSyst < 0x3a98
                    && personData[i].linkSyst - 0x2710 == gSystems[sys].govt)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x3a98 && personData[i].linkSyst < 0x4e20
                    && gSystems[sys].govt >= 0
                    && GovtAllies(personData[i].linkSyst - 0x3a98, gSystems[sys].govt) != 0)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x4e20 && personData[i].linkSyst < 0x61a8
                    && gSystems[sys].govt >= 0
                    && personData[i].linkSyst - 0x4e20 != gSystems[sys].govt)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x61a8 && personData[i].linkSyst < 0x7530
                    && gSystems[sys].govt >= 0
                    && GovtEnemies(personData[i].linkSyst - 0x61a8, gSystems[sys].govt) != 0)
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
            && shipData[i].persType >= 0 && shipData[i].persType < kMaxPersons) {
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
            pick = Rand(0x3fe);
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
                if ((personData[pick].flags & 2) != 0)
                    shipData[slot].hasAfterburner = 1;
                if (shipData[slot].aggression < 1)
                    shipData[slot].aggression = 1;
                if (shipData[slot].aggression >= 3)
                    shipData[slot].aggression = 4;
                for (i = 0; i < kMaxWeapTypes; i++) {
                    shipData[slot].weap[i].count =
                        gShipTypes[shipData[slot].type].weapCount[i];
                    shipData[slot].weap[i].ammo =
                        gShipTypes[shipData[slot].type].weapAmmo[i];
                    shipData[slot].weap[i].count += personData[pick].weapCount[i];
                    shipData[slot].weap[i].ammo += personData[pick].weapAmmo[i];
                }
                shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
                shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
                if ((personData[pick].flags2 & kPersNeedsFuel) != 0)
                    shipData[slot].fuel = 0;
                else
                    shipData[slot].fuel = gShipTypes[shipData[slot].type].fuel;
                if (personData[pick].shieldMod > 0) {
                    shipData[slot].shield =
                        shipData[slot].shield * personData[pick].shieldMod;
                    shipData[slot].armor =
                        shipData[slot].armor * personData[pick].shieldMod;
                }
                if (shipData[slot].govt != -1
                    && (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].shield = 0;
                    if ((gShipTypes[shipData[slot].type].flags & 0x10) != 0)
                        shipData[slot].armor =
                            gShipTypes[shipData[slot].type].armor * 0.1 - 1.0;
                    else
                        shipData[slot].armor =
                            gShipTypes[shipData[slot].type].armor * 0.33 - 1.0;
                    shipData[slot].vel.x = shipData[slot].vel.y = 0;
                    shipData[slot].accelMag = 0;
                }
                AISetup(&shipData[slot]);
                if (personData[shipData[slot].persType].linkMission != -1)
                    RandomizeOneMission(personData[shipData[slot].persType].linkMission);
                if (personData[pick].govt >= 0
                    && (govtData[personData[pick].govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].heading = Rand(360);
                    /* BUG: vel.x is cleared twice and vel.y keeps whatever the slot held */
                    shipData[slot].vel.x = 0;
                    shipData[slot].vel.x = 0;
                    shipData[slot].accelMag = 0;
                    shipData[slot].engineGlow = 0;
                    if ((gShipTypes[shipData[slot].type].shanFlags & 2) != 0)
                        shipData[slot].foldSet =
                            gShipTypes[shipData[slot].type].baseSets - 1;
                }
                return slot;
            }
        }
    }
    return -1;
}

/* DoShipCapture (mac _DoShipCapture). STATE: MATCH 100%, exe 817 / obj 817; byte-identical
   yes (relocations masked). The three index spellings are CSE classes; the loop counters and
   the scratch `val' decide the colour order (notes). OPEN: none */
// FUNCTION: EVNOVA 0x00423fa0
void DoShipCapture(ShipObj *me, char asEscort)
{
    unsigned char buf[256];
    long slotL;
    long capType;
    short slot;
    short w;
    short i;
    short val;
    short m;

    slot = GenericRandomShipSpawn(shipData[0].curSystem, 1);
    if (slot != -1) {
        if (me->persType != -1)
            personData[me->persType].loaded = 0;
        slotL = slot;
        shipData[slotL].type = shipData[0].type;
        shipData[slotL].dude = -1;
        shipData[slotL].persType = -1;
        {
            float *dp, *sp;

            sp = &shipData[0].pos.x;
            dp = &shipData[slotL].pos.x;
            dp[0] = sp[0];
            dp[1] = sp[1];
        }
        {
            float *dq, *sq;

            sq = &shipData[0].vel.x;
            dq = &shipData[slotL].vel.x;
            dq[0] = sq[0];
            dq[1] = sq[1];
        }
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
        val = gShipTypes[shipData[slotL].type].inherentGovtB;
        if (val != -1) {
            val = govtData[val].voiceClass;
            if (val != -1)
                shipData[(int)slotL].voiceClass = val;
        }
        for (i = 0; i < 6; i++)
            shipData[slotL].cargo[i] = shipData[0].cargo[i];
        for (i = 1; i < 0x40; i++) {
            if (shipData[i].active != 0 && i != me->index && i != slot
                    && me->index == shipData[i].parentShip) {
                shipData[i].targetShip = shipData[i].targetStellar = -1;
                shipData[i].grudge = 0;
                shipData[i].parentShip = -1;
                shipData[i].aiType = gShipTypes[shipData[i].type].inherentAI;
            }
        }
        shipData[slotL].specialMission = -1;
        if (asEscort != 0) {
            shipData[(long)slot].shield = 0;
            shipData[(long)slot].armor = 1.0f;
            shipData[(long)slot].boarded = 1;
            shipData[(long)slot].formerEscort = -1;
            shipData[(long)slot].hired = 0;
            shipData[(long)slot].hailed = 0;
            shipData[(long)slot].parentShip = 0;
            shipData[(long)slot].aiType = 6;
            DestroyPartialFleetCargo(slot);
            shipData[(long)slot].parentShip = -1;
            shipData[(long)slot].aiType = -1;
        } else {
            shipData[(long)slot].deathTimer = 0;
            shipData[(long)slot].parentShip = 0;
            shipData[(long)slot].aiType = 6;
            shipData[(long)slot].defenseStellar = -1;
            shipData[(long)slot].grudge = 0;
            shipData[(long)slot].fuel = ShipFuelCapacity(&shipData[(long)slot]);
            if (IsDisabled(&shipData[(long)slot]) != 0) {
                if ((gShipTypes[shipData[slotL].type].flags & 0x10) != 0)
                    shipData[(long)slot].armor =
                        ShipArmorCapacity(&shipData[(long)slot]) * 0.1 + 1.0;
                else
                    shipData[(long)slot].armor =
                        ShipArmorCapacity(&shipData[(long)slot]) * 0.3333 + 1.0;
            }
            prevParent[slotL] = 0;
            AISetup(&shipData[(int)slotL]);
            AIMakeEscortFlyInForm(&shipData[(int)slotL]);
        }
        statusRedraw[2] = 1;
        statusRedraw[6] = 1;
        statusRedraw[3] = 1;
        for (i = 0; i < 0x100; i++) {
            shipData[slotL].weap[i].count =
                gShipTypes[shipData[slotL].type].weapCount[i];
            shipData[slotL].weap[i].ammo =
                gShipTypes[shipData[slotL].type].weapAmmo[i];
        }
        {
            ShipRec *rec = &gShipTypes[shipData[0].type];

            crashlog_mission_bit_set("retiring", (char *)rec->name, 1,
                                     shipData[0].type + 128, rec->onRetire);
        }
        EvalMissionBitSetString(gShipTypes[shipData[0].type].onRetire);
        {
            ShipRec *rec = &gShipTypes[me->type];

            crashlog_mission_bit_set("capturing", (char *)rec->name, 1,
                                     me->type + 128, rec->onCapture);
        }
        EvalMissionBitSetString(gShipTypes[me->type].onCapture);
        shipData[0].type = me->type;
        {
            float *dp = &shipData[0].pos.x;
            dp[0] = me->pos.x;
            dp[1] = me->pos.y;
        }
        {
            float *dq = &shipData[0].vel.x;
            dq[0] = me->vel.x;
            dq[1] = me->vel.y;
        }
        shipData[0].heading = me->heading;
        shipData[0].desiredHeading = shipData[0].heading;
        shipData[0].targetStellar = -1;
        shipData[0].targetShip = -1;
        shipData[0].selectedWeapon = -1;
        GetShipPaintColor(me, &playerPaint.red, &playerPaint.green,
                     &playerPaint.blue);
        for (i = 0; i < 6; i++)
            shipData[0].cargo[i] = me->cargo[i];
        if (ShipFuelCapacity(shipData) <= 0)
            shipData[0].fuel = 0;
        else
            shipData[0].fuel = Rand(ShipFuelCapacity(shipData));
        shipData[0].aiType = -1;
        shipData[0].parentShip = -1;
        shipData[0].govt = -1;
        shipData[0].shield = 0;
        capType = shipData[0].type;
        if ((gShipTypes[capType].flags & 0x10) != 0)
            shipData[0].armor = gShipTypes[capType].armor * 0.1 + 2.0;
        else
            shipData[0].armor = gShipTypes[capType].armor * 0.3333 + 2.0;
        for (w = 0; w < 0x100; w++) {
            char keep;

            keep = 0;
            for (i = 0; i < 0x200; i++) {
                for (m = 0; m < kOutfMods; m++) {
                    if (gOutfits[i].modType[m] == 1 && w == gOutfits[i].modVal[m]
                            && gOutfits[i].persistent != 0) {
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
        for (w = 0; w < 0x100; w++) {
            short n;

            n = me->weap[w].count;
            if (n > 0 && shipData[0].weap[w].count <= 0)
                shipData[0].weap[w].count = n;
            val = me->weap[w].ammo;
            if (val > 0 && shipData[0].weap[w].ammo <= 0) {
                if (gWeapons[w].guidance == kGuidanceBay) {
                    shipData[0].weap[w].ammo = val;
                } else {
                    short src;

                    src = gWeapons[w].ammoType;
                    if (src >= 0 && src < 0x100)
                        shipData[0].weap[src].ammo = val;
                }
            }
        }
        for (i = 0; i < 0x200; i++) {
            if (gOutfits[i].persistent == 0)
                shipSystems[i] = 0;
        }
        ShipStatsToSystemInfo();
        for (i = 0; i < 8; i++) {
            if (gShipTypes[shipData[0].type].defOutfCount[i] > 0)
                shipSystems[gShipTypes[shipData[0].type].defOutfID[i]] +=
                    gShipTypes[shipData[0].type].defOutfCount[i];
        }
        SystemInfoToShipStats();
        me->active = 0;
        GetIndString(buf, kGameStrList, 0x130);
        DisplayComm(buf, 360, 0, 0xc, &rgbWhite, 0, 0);

    } else {
        GetIndString(buf, kGameStrList, 0x131);
        DisplayComm(buf, 360, 0, 0xc, &rgbWhite, 0, 0);
    }
}


/* DisasterHandler. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00424f90
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
            if (target >= 0x80) {
                disasterData[i].stellarID = disasterData[i].stellar - 0x80;
                disasterData[i].countdown = disasterData[i].duration;
            } else if (target == -1) {
                do {
                    target = Rand(0x800);
                } while (!stellarData[target].loaded ||
                         (stellarData[target].flags & kSpobHidden) != 0);
                disasterData[i].stellarID = target;
                disasterData[i].countdown = disasterData[i].duration;
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
void DoAutoRefuel(void)
{
    char found;
    short amount;
    short i;
    short j;
    float fuel;

    found = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (gOutfits[i].modType[j] == kOutfModType19) {
                    found = 1;
                    break;
                }
            }
        }
    }

    amount = ShipFuelCapacity(shipData);
    if (found) {
        fuel = shipData->fuel;
        if ((short)fuel < amount) {
            shipData->fuel = (short)fuel;
            amount -= shipData->fuel;
            if (amount > shipData->credits)
                amount = shipData->credits;
            shipData->credits -= amount;
            shipData->fuel += amount;
            statusRedraw[4] = 1;
            statusRedraw[2] = 1;
        }
    }
}

#define kFletCount 256


/* SpawnFleet. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00425280
void SpawnFleet(short sys, unsigned char arriving)
{
    short count;
    short i;
    short pick;
    char ok[kFletCount];

    count = 0;
    for (i = 0; i < kFletCount; i++) {
        ok[i] = 0;
        if (fleetData[i].leadShipType != -1 && fleetData[i].usable != 0) {
            if (fleetData[i].linkSyst == -1)
                ok[i] = 1;
            if (sys == fleetData[i].linkSyst)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 128 && fleetData[i].linkSyst < 10000 &&
                sys == fleetData[i].linkSyst - 128)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 10000 && fleetData[i].linkSyst < 15000 &&
                fleetData[i].linkSyst - 10000 == gSystems[sys].govt)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 15000 && fleetData[i].linkSyst < 20000 &&
                gSystems[sys].govt >= 0 &&
                GovtAllies(fleetData[i].linkSyst - 15000, gSystems[sys].govt))
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 20000 && fleetData[i].linkSyst < 25000 &&
                gSystems[sys].govt >= 0 &&
                fleetData[i].linkSyst - 20000 != gSystems[sys].govt)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 25000 && fleetData[i].linkSyst < 30000 &&
                gSystems[sys].govt >= 0 &&
                GovtEnemies(fleetData[i].linkSyst - 25000, gSystems[sys].govt))
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
        shipData[slot].escapePods = gShipTypes[shipData[slot].type].podCount;
        shipData[slot].bankDir = 0;
        shipData[slot].frameTimer = 0;
        shipData[slot].shieldGlow = 0;
        shipData[slot].weapGlow = 0;
        shipData[slot].annoyance = 0;
        shipData[slot].bankDir = 0;
        shipData[slot].foldDir = 0;
        shipData[slot].foldSet = gShipTypes[shipData[slot].type].baseSets - 1;
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
        shipData[slot].seesCloaked = -1;
        shipData[slot].seesCloakedRadar = -1;
        shipData[slot].decloaksWhenHit = -1;
        shipData[slot].fuelRegenState = -1;
        for (k = 0; k < 4; k++)
            shipData[slot].jamming[k] = -1;
        if (shipData[slot].govt != -1 &&
            (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled))
            shipData[slot].engineGlow = 0;
        if (gShipTypes[shipData[slot].type].baseSets > 0)
            shipData[slot].animSet = Rand(gShipTypes[shipData[slot].type].baseSets);
        if (gShipTypes[shipData[slot].type].animDelay > 0)
            shipData[slot].animTimer = Rand(gShipTypes[shipData[slot].type].animDelay);
        for (k = 0; k < 6; k++)
            shipData[slot].cargo[k] = 0;
        shipData[slot].pos.x = Rand(1500) - 750;
        shipData[slot].pos.y = Rand(1500) - 750;
    }
    return slot;
}

#define kFleetFlagRandomCargo 0x0001


                                                         /* 0x0043b4a0 */
                                                         /* 0x004159e0 */
                                                         /* 0x00413990 */

/* HyperSpawnFleet (mac _HyperSpawnFleet). STATE: MATCH 100.0%, 982/982, byte-identical yes.
   The fleet index is held twice, int `f' and long `idx' (the copy differs in type, so it
   survives copy propagation); one holder scores 98.4-98.5 (notes). OPEN: none. */
// FUNCTION: EVNOVA 0x004259b0
void HyperSpawnFleet(short sys, short fleet, short escortOf,
                     unsigned char arriving)
{
    short i;
    float speed;
    float dist;
    long idx;
    int f;
    short count;
    short j;
    short stellar;
    unsigned char buf[256];
    short slot;
    short esc;
    FloatPoint pt;
    short k;
    short type;
    char digit;
    unsigned char c;

    f = fleet;
    idx = f;
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
            shipData[slot].aiType = gShipTypes[shipData[slot].type].inherentAI;
        shipData[slot].shield = gShipTypes[shipData[slot].type].shield;
        shipData[slot].armor = gShipTypes[shipData[slot].type].armor;
        shipData[slot].persType = -1;
        shipData[slot].specialMission = -1;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = ShipCanScoop(&shipData[slot]);
        AISetup(&shipData[slot]);
        if ((fleetData[f].flags & kFleetFlagRandomCargo) &&
            gShipTypes[shipData[slot].type].inherentAI <= 2)
            shipData[slot].cargo[Rand(6)] =
                Rand(gShipTypes[shipData[slot].type].cargo) + 1;
        shipData[slot].lastStellar = -2;
        shipData[slot].prevSystem = -1;
        shipData[slot].vel.x = 0;
        shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        stellar = PickEmergeStellar(&shipData[slot]);
        if (stellar == -1) {
            AIMakeShipJumpIn(&shipData[slot]);
            dist = 0;
            speed = 50.0f;
            do {
                dist += speed;
                speed -= 1.165;
            } while (speed > 0);
            shipData[slot].pos.x = shipData[slot].pos.y = 0;
            Accel(Rand(360), dist + 1000.0f,
                  &shipData[slot].pos);
            pt.x = pt.y = 0;
            shipData[slot].heading =
                CalcInterceptAngle(&shipData[slot].pos, &pt);
            shipData[slot].jumpTimer = -999;
            shipData[slot].jumpTick = ReadTick();
            Accel(shipData[slot].heading, 50.0f,
                  &shipData[slot].vel);
        } else {
            AIMakeShipEmergeFromHyperGate(&shipData[slot], stellar);
            shipData[slot].pos.x = stellarData[stellar].pos.h;
            shipData[slot].pos.y = stellarData[stellar].pos.v;
        }
        for (i = 0; i < kMaxWeapTypes; i++) {
            shipData[slot].weap[i].count =
                gShipTypes[shipData[slot].type].weapCount[i];
            shipData[slot].weap[i].ammo =
                gShipTypes[shipData[slot].type].weapAmmo[i];
        }
        for (j = 0; j <= 3; j++) {
            count = Rand(fleetData[idx].escortMax[j] -
                         fleetData[idx].escortMin[j] + 1) +
                    fleetData[idx].escortMin[j];
            type = fleetData[idx].escortTypes[j];
            if (type == -1)
                count = 0;
            else if (gShipTypes[type].canAppear == 0)
                count = 0;
            if (count > 0) {
                for (k = 0; k < count; k++) {
                    esc = GenericRandomShipSpawn(sys, kSpawnReserve);
                    if (esc != -1) {
                        shipData[esc].aiType = kEscortAI;
                        shipData[esc].parentShip = slot;
                        shipData[esc].type = fleetData[idx].escortTypes[j];
                        shipData[esc].dude = -1;
                        shipData[esc].persType = -1;
                        shipData[esc].govt = fleetData[f].govt;
                        shipData[esc].shield =
                            gShipTypes[shipData[esc].type].shield;
                        shipData[esc].armor =
                            gShipTypes[shipData[esc].type].armor;
                        AISetup(&shipData[esc]);
                        for (i = 0; i < kMaxWeapTypes; i++) {
                            shipData[esc].weap[i].count =
                                gShipTypes[shipData[esc].type].weapCount[i];
                            shipData[esc].weap[i].ammo =
                                gShipTypes[shipData[esc].type].weapAmmo[i];
                        }
                        shipData[esc].heading = shipData[slot].heading;
                        shipData[esc].pos.x =
                            Rand(300) + shipData[slot].pos.x - 150.0f;
                        shipData[esc].pos.y =
                            Rand(300) + shipData[slot].pos.y - 150.0f;
                        shipData[esc].vel.x = shipData[slot].vel.x;
                        shipData[esc].vel.y = shipData[slot].vel.y;
                        shipData[esc].accelMag = shipData[slot].accelMag;
                        shipData[esc].lastStellar = -2;
                        shipData[esc].prevSystem = -2;
                        shipData[esc].swarmLeader = -1;
                        shipData[esc].formLeader = slot;
                        shipData[slot].hasFormation = 1;
                        if ((fleetData[f].flags & kFleetFlagRandomCargo) &&
                            gShipTypes[shipData[esc].type].inherentAI <= 2)
                            shipData[esc].cargo[Rand(6)] =
                                Rand(gShipTypes[shipData[esc].type].cargo) + 1;
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
            for (i = 1; i <= buf[0]; i++) {
                idx = buf[i];
                if (idx == '#') {
                    if (digit)
                        c = Rand(10) + '0';
                    else
                        c = Rand(9) + '1';
                    buf[i] = c;
                    digit = 1;
                } else
                    digit = 0;
            }
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm(buf, 360, 0, 12, &rgbWhite, 0, 0);
        }
    }
}

/* SpeakVoiceMessage. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426ce0
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
#define kHailCommWidth    0x1a4
#define kHailTextSize     12

#define kSpobFlags2Dominated 0x20 /* name TBC */

/* ShowPersonHailQuote (mac _ShowPersonHailQuote). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426d10
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
    DisplayComm((unsigned char *)briefingTextPtr, kHailCommWidth, 0, kHailTextSize,
                &rgbWhite, 0, 0);
}

/* SpawnBountyHunter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00426dd0
void SpawnBountyHunter(void)
{
    short i;
    short eligible;
    char doHail;
    short shipIndex;

    if (((char *)personData)[0x1e46f8] == 0 || ((char *)personData)[0x1e46fb] == 0)
        return;

    eligible = 0;
    doHail = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0
            && (stellarData[i].flags2 & kSpobFlags2Dominated) == 0)
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
char ShotCanHitShip(ShotObj *shot, ShipObj *ship)
{
    short reachesPlayer;
    short nearPlayer;
    short cur;
    short govt;
    short parentIdx;

    if (shot == 0 || ship == 0)
        return 0;
    if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes)
        return 0;
    if (ship->type < 0 || ship->type >= kMaxShipTypes)
        return 0;
    if (!(shot->life > 0.0f) || ship->type == kMaxShipTypes - 1)
        goto reject;
    if (shot->owner == ship->index)
        return 0;
    if (shot->guidanceState == 0x3e6)
        return 0;
    if (ship->persType == kPersCaptainHector)
        return 0;

    if (gWeapons[shot->weapType].guidance == kGuidanceHoming
            && ship->index != shot->target
            && (gWeapons[shot->weapType].flags2 & 8) == 0)
        return 0;

    if (shot->owner >= 0 && shot->owner < kMaxShips) {
        if (shipData[shot->owner].parentShip != -1 && ship->parentShip != -1
                && ship->parentShip == shipData[shot->owner].parentShip)
            return 0;
        if (shipData[shot->owner].govt >= 0 && shipData[shot->owner].govt < kMaxGovts
                && ship->govt == shipData[shot->owner].govt)
            return 0;
        if (shipData[shot->owner].defenseStellar != -1
                && ship->defenseStellar == shipData[shot->owner].defenseStellar)
            return 0;

        if (shot->hitRoll > gShipTypes[ship->type].aiSkill) {
            if (!IsDisabled(ship))
                return 0;
        }
        if (ship->index != 0) {
            nearPlayer = AlliedWithPlayer(ship) ? 1 : 0;
        } else {
            nearPlayer = 1;
        }

        reachesPlayer = 0;
        cur = shot->owner;
walk:
        if (cur == 0) {
            reachesPlayer = 1;
        } else {
            if (cur < 0 || cur >= kMaxShips)
                goto walk_done;
            cur = shipData[cur].parentShip;
            if (cur != -1)
                goto walk;
        }
walk_done:
        if (reachesPlayer) {
            if (nearPlayer && ship->parentShip >= 0 && ship->parentShip < kMaxShips
                    && shipData[ship->parentShip].parentShip == 0)
                return 0;
        }
        if (reachesPlayer) {
            govt = shipData[shot->owner].govt;
            if (govt >= 0 && govt < kMaxGovts && (govtData[govt].flags & 0x40) != 0)
                return 0;
        }
        if (reachesPlayer) {
            if (ship->govt >= 0 && ship->govt < kMaxGovts
                    && (govtData[ship->govt].flags & 8) != 0)
                return 0;
        }
        if (reachesPlayer) {
            if (ship->dude >= 0 && ship->dude < kMaxDudes
                    && (dudeData[ship->dude].flags & 0x100) != 0)
                return 0;
        }

        if (nearPlayer) {
            if (shipData[shot->owner].dude < 0
                    || shipData[shot->owner].dude >= kMaxDudes) {
                parentIdx = shipData[shot->owner].parentShip;
                if (parentIdx >= 0 && parentIdx < kMaxDudes) {
                    if (shipData[parentIdx].dude >= 0
                            && shipData[parentIdx].dude < kMaxDudes
                            && (dudeData[shipData[parentIdx].dude].flags & 0x100) != 0)
                        return 0;
                }
            } else if ((dudeData[shipData[shot->owner].dude].flags & 0x100) != 0) {
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

    if ((gShipTypes[ship->type].flags & 0x400) != (gWeapons[shot->weapType].flags2 & 0x400))
        return 0;

    if (ship->govt >= 0 && ship->govt < kMaxGovts && ship->index != 0
            && (govtData[ship->govt].flags & 0x40) != 0) {
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
reject:
    return 0;
}

#define kFixedOne   256.0f      /* NewParticle's 24.8 scale */

/* SpawnParticles (mac _SpawnParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004274d0
void SpawnParticles(short x, short y, float speed, short spreadPct,
                  short minLife, short maxLife, long color, short shade,
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

        NewParticle(color, (long)pos.x << 8, (long)pos.y << 8,
                    (vel.x * kFixedOne), (vel.y * kFixedOne),
                    shade, life);
    }
}

#define kModTypePaint   43

#define kMapGovtClassBase 1000

/* GrantOutfitItem (mac _GrantOutfitItem). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427770
void GrantOutfitItem(short outfit)
{
    short i;
    short s;
    short k;
    short v;
    short govtClass;
    char paintSet;
    char fakeID;

    if (outfit < 0 || outfit >= kMaxOutfits)
        return;
    boughtMap = 0;
    boughtID = 0;
    paintSet = 0;
    fakeID = 0;
    for (i = 0; i < kOutfMods; i++) {
        if (gOutfits[outfit].modType[i] == kModTypeMap) {
            v = gOutfits[outfit].modVal[i];
            if (v > 0) {
                AutoSetExploration(shipData[0].curSystem, v, 2);
            } else if (v == -1) {
                for (s = 0; s < kMaxSystems; s++) {
                    if (gSystems[s].loaded != 0 && gSystems[s].govt == -1
                            && SystemIsInhabited(s) != 0)
                        AutoSetExploration(s, 0, 2);
                }
            } else if (v <= -kMapGovtClassBase) {
                govtClass = -(v + kMapGovtClassBase);
                for (s = 0; s < kMaxSystems; s++) {
                    if (gSystems[s].loaded != 0 && gSystems[s].govt != -1) {
                        for (k = 0; k < kGovtClasses; k++) {
                            if (govtData[gSystems[s].govt].classes[k]
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
        if (gOutfits[outfit].modType[i] == kModTypePaint) {
            playerPaint.red =
                ((unsigned short)gOutfits[outfit].modVal[i] >> 10) & 0x1f;
            playerPaint.green =
                ((unsigned short)gOutfits[outfit].modVal[i] >> 5) & 0x1f;
            playerPaint.blue =
                gOutfits[outfit].modVal[i] & 0x1f;
            paintSet = 1;
            break;
        }
    }
    for (i = 0; i < kOutfMods; i++) {
        if (gOutfits[outfit].modType[i] == kModTypeFakeID) {
            fakeID = 1;
            break;
        }
    }
    if (boughtMap == 0 && paintSet == 0) {
        if (fakeID != 0) {
            for (i = 0; i < kOutfMods; i++) {
                if (gOutfits[outfit].modType[i] == kModTypeFakeID) {
                    for (s = 0; s < kMaxSystems; s++) {
                        if (gSystems[s].loaded != 0) {
                            if (gOutfits[outfit].modVal[i] == -1) {
                                if (playerLegalStatus[s] < 0)
                                    playerLegalStatus[s] = 0;
                            } else if (gSystems[s].govt
                                    == gOutfits[outfit].modVal[i]) {
                                if (playerLegalStatus[s] < 0)
                                    playerLegalStatus[s] = 0;
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

/*
 * Fills the first beamData slot whose state is < -1.
 */

/* SpawnBeam. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427a90
void SpawnBeam(short ship, short target, short weapType, short unused, short mount, short flag)
{
    short i;

    for (i = 0; i < kMaxBeams; i++) {
        if (beamData[i].state < -1) {
            beamData[i].state = gWeapons[weapType].lifetime;
            beamData[i].frame = 0;
            beamData[i].weapType = weapType;
            beamData[i].owner = ship;
            beamData[i].targetIsShot = flag;
            beamData[i].mount = gWeapons[weapType].exitType;
            if (mount >= 0 && mount <= 3) {
                beamData[i].lane = mount;
            } else {
                if (shipData[ship].weapGroupLane[gWeapons[weapType].exitType] < 0
                        || shipData[ship].weapGroupLane[gWeapons[weapType].exitType] > 3)
                    shipData[ship].weapGroupLane[gWeapons[weapType].exitType] = Rand(4);
                if ((gWeapons[weapType].flags3 & kWeapClosestExit)
                        && ship != -1 && flag != 1 && target != -1) {
                    beamData[i].lane = SelectClosestShotStartPosition(
                        &shipData[ship], &shipData[ship].pos, weapType,
                        shipData[ship].heading, gWeapons[weapType].exitType,
                        &shipData[target].pos);
                    if (shipData[ship].weapGroupLane[gWeapons[weapType].exitType] < 0
                            || shipData[ship].weapGroupLane[gWeapons[weapType].exitType] > 3)
                        shipData[ship].weapGroupLane[gWeapons[weapType].exitType] = Rand(4);
                } else {
                    beamData[i].lane =
                        shipData[ship].weapGroupLane[gWeapons[weapType].exitType];
                }
                shipData[ship].weapGroupLane[gWeapons[weapType].exitType] = 
                    ((shipData[ship].weapGroupLane[gWeapons[weapType].exitType] + 1) % 4);
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

#define kRankGrantDropsSameGovt      0x0001  /* on grant, clears all same-govt ranks */
#define kRankDropsSameGovt           0x0002  /* clears all same-govt ranks */
#define kRankGrantDropsLowerPriority 0x0010  /* on grant, clears same-govt ranks ranked below */
#define kRankDropsLowerPriority      0x0020  /* clears same-govt ranks ranked below */

#define kNoRank                 (-1)

/* record layout: +0 active, +1 in use, +2 id, +4 priority, +6 government,
   +0x1c flags, +0x1e name */
#define kRankId                 2
#define kRankPriority           4
#define kRankName               0x1e

/* ActivateRank (mac _ActivateRank). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427df0
void ActivateRank(char *rank)
{
    short i;

    if (rank[kRankActive] == '\0') {
        crashlog_print("activating rank ");
        crashlog_print(rank + kRankName);
        crashlog_print("\n");
        rank[kRankActive] = 1;
        if ((*(unsigned short *)(rank + kRankFlags) & kRankGrantDropsSameGovt) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != '\0'
                        && rankData[i].loaded != '\0'
                        && i != *(short *)(rank + kRankId)
                        && *(short *)(rank + kRankGovt) == rankData[i].govt
                        && (rankData[i].flags & kRankNeverDropped) == 0) {
                    rankData[i].active = '\0';
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
        if ((*(unsigned short *)(rank + kRankFlags) & kRankGrantDropsLowerPriority) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != '\0'
                        && rankData[i].loaded != '\0'
                        && i != *(short *)(rank + kRankId)
                        && *(short *)(rank + kRankGovt) == rankData[i].govt
                        && (rankData[i].flags & kRankNeverDropped) == 0
                        && rankData[i].priority < *(short *)(rank + kRankPriority)) {
                    rankData[i].active = '\0';
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
    }
    recentActivatedRank = *(short *)(rank + kRankId);
}

/* DeactivateRank (mac _DeactivateRank). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00427f40
void DeactivateRank(char *rank)
{
    short i;

    if (rank[kRankActive] != '\0') {
        crashlog_print("deactivating rank ");
        crashlog_print(rank + kRankName);
        crashlog_print("\n");
        rank[kRankActive] = '\0';
        if ((*(unsigned short *)(rank + kRankFlags) & kRankDropsSameGovt) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != '\0'
                        && rankData[i].loaded != '\0'
                        && *(short *)(rank + kRankGovt) == rankData[i].govt
                        && (rankData[i].flags & kRankNeverDropped) == 0) {
                    rankData[i].active = '\0';
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
        if ((*(unsigned short *)(rank + kRankFlags) & kRankDropsLowerPriority) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != '\0'
                        && rankData[i].loaded != '\0'
                        && *(short *)(rank + kRankGovt) == rankData[i].govt
                        && (rankData[i].flags & kRankNeverDropped) == 0
                        && rankData[i].priority < *(short *)(rank + kRankPriority)) {
                    rankData[i].active = '\0';
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
    }
    if (recentActivatedRank == *(short *)(rank + kRankId))
        recentActivatedRank = kNoRank;
}

/*
 * podData is the 32-slot pod array InitArrays allocates as AllocAPointer(0x300); a slot is
 * free once its life counter has run below zero.
 */

/* LaunchEscapePod. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00428090
void LaunchEscapePod(ShipObj *ship)
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

            angle = Rand(360);
            SWSetSpriteVisible(podData[i].sprite, 1);
            Accel(angle, (Rand(10) + 10) * 0.1, &podData[i].vel);

            origin.x = origin.y = 0;
            drift.x = podData[i].vel.x * 100.0;
            drift.y = podData[i].vel.y * 100.0;
            heading = CalcInterceptAngle(&origin, &drift);
            SWSetCurrentFrameIndex(podData[i].sprite,
                         (heading * gShipTypes[kShipTypeEscapePod].framesPer / 360.0));
            break;
        }
    }
}

/* The exe's const table at 0x0056b910: the same 38 revoked keys that InitEVNova tests inline
   (transcription allowed by the user, 2026-09-16). */
#define kPirateKeyMask 0x0fffffffffffffffLL

#define kPirateSTR   140
#define kPirateIndex 50

typedef struct KeyTable {
    long long key[kBlacklistCount];
} KeyTable;

extern const KeyTable revokedKeyTable = {{
    0x0284801349262406LL, 0x0101e057b160ee02LL,
    0x07365c430b23e131LL, 0x07345d430b23e131LL,
    0x05d4013a1123c94aLL, 0x08d22033a5a9ae47LL,
    0x08d02133a5a9ae47LL, 0x0940f13e07262d74LL,
    0x01e8616da73a6b41LL, 0x03395a952823e649LL,
    0x033b5a952803f64bLL, 0x0b795a952803f64aLL,
    0x00dd021413f10475LL, 0x00df021413d11477LL,
    0x009f021413d11476LL, 0x09224585278cbf5bLL,
    0x0bf35041709a3940LL, 0x03f15041709a3946LL,
    0x0125a4aa21b1cf03LL, 0x03a31e5b03829d3dLL,
    0x009f8c40aac47814LL, 0x045ac4b40fb08b36LL,
    0x0c1ac5bc0b909b37LL, 0x02a974140d644b4dLL,
    0x02a9741409644b49LL, 0x0aab751c09445b49LL,
    0x02eb751c09445b4aLL, 0x01e24b6c4b56f67eLL,
    0x048c30d71f489e43LL, 0x0c8c31d71f489e45LL,
    0x04cc31d71f489e46LL, 0x0c8e31d71f489e47LL,
    0x0cce31d71f489e40LL, 0x0561f87749b27666LL,
    0x090a1452e822bd05LL, 0x04c23c3d8c45fe08LL,
    0x03e90792a484de04LL, 0x000881643da5fb73LL
}};

/* CheckForPiratedCodes. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00428260
void CheckForPiratedCodes(void)
{
    KeyTable keys;
    unsigned char msg[256];
    short i;

    keys = revokedKeyTable;
    for (i = 0; i < kBlacklistCount; i++) {
        if ((keys.key[i] & kPirateKeyMask) == (RT3_GetLicenseCode() & kPirateKeyMask)) {
            FadeScreenIn();
            ShowTheCursor();
            GetIndString(msg, kPirateSTR, kPirateIndex);
            ShowAlert("", PtoCstr(msg));
            ExitToShell();
        }
    }
}

/* CrtInitHook1. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00428330
void CrtInitHook1(void)
{
}

}

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
