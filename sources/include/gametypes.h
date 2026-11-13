/* gametypes.h -- shared record and runtime-object layouts; types only. Notes: Research/notes/gametypes.md */
#ifndef GAMETYPES_H
#define GAMETYPES_H
#include "int32.h"

/* the C++-linkage declarations of the win build's C++ TUs; the mac build is C only */
#if defined(__MWERKS__) && !defined(MACBUILD)
#define EXTERN_CPP_BEGIN extern "C++" {
#define EXTERN_CPP_END }
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_CPP_BEGIN
#define EXTERN_CPP_END
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

/* the Toolbox flag type: the port's is signed, Carbon's unsigned char */
#ifndef MACBUILD
typedef char Boolean;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FloatPoint {
    float                  x;
    float                  y;
} FloatPoint;

#ifdef MACBUILD
typedef struct ShortPoint {
    short                  h;
    short                  v;
} ShortPoint;
typedef ShortPoint MapScroll;
#define SalvageAmmo short
#define ShipClassID short
#define PStrChar char
#else
typedef struct MapScroll {
    short                  h;
    short                  v;
} MapScroll;
#define SalvageAmmo unsigned short
#define ShipClassID unsigned short
#define PStrChar unsigned char
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern MapScroll mapScroll;
EXTERN_CPP_END
#pragma cplusplus reset

typedef struct StringListRec {
    unsigned short         count;
} StringListRec;

typedef struct AnimDelta {
    short                  width;
    short                  height;
} AnimDelta;

typedef struct MStr252 {
    char                   c[0xfc];
} MStr252;

typedef struct MStr254 {
    char                   c[0xfe];
} MStr254;

typedef struct MStr255 {
    char                   c[0xff];
} MStr255;

typedef struct MStr256 {
    char                   c[0x100];
} MStr256;

typedef struct MStr63 {
    char                   c[0x3f];
} MStr63;

typedef struct MStr64 {
    char                   c[0x40];
} MStr64;

#ifndef MACBUILD
typedef unsigned char Str255[256];

typedef struct DialogRecord *DialogPtr;
#endif

typedef struct PictCacheSlot {
    short                  picID;
    char pad02[0x2];
    uint32_t          stamp;
} PictCacheSlot;

typedef struct ResRecord {
    struct RezFile         *rez;
    int32_t                   index;
    uint32_t          type;
    short                  id;
    char                   name[258];
} ResRecord;

/* one resource type in a RezFile's type table, stride 0x0c. */
typedef struct RezTypeEntry {
    uint32_t          type;
    ResRecord              *first;
    uint32_t          count;
} RezTypeEntry;

/* BurgerLib's RezHeader_t (0x24 bytes), then the port's type table */
typedef struct RezFile {
    void                   (*decompress[3])(unsigned char *dst, unsigned char *src,
                                            uint32_t length, uint32_t packLength);
    uint32_t          groupCount;
    uint32_t          nameCount;
    void                   *file;
    struct RezGroup        **groups;
    struct RezName         **names;
    uint32_t          flags;
    RezTypeEntry           *types;
    uint32_t          typeCount;
} RezFile;

/* one parallax star slot (starData[], stride 0x14) */
typedef struct StarSlot {
    struct SpriteRec       *sprite;
    Boolean                   live;
    char pad05[0x1];
    short                  frame;
    float                  depth;
    FloatPoint             pos;
} StarSlot;

/* one explosion in explodData, stride 0x18, 32 entries */
typedef struct BoomSlot {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    short                  type;
    float                  timer;
    float                  delay;
} BoomSlot;

/* one smoke slot in smokeData, stride 0x18 */
typedef struct DebrisSlot {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    short                  type;
    float                  timer;
    float                  angle;
} DebrisSlot;
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern DebrisSlot *smokeData;
EXTERN_CPP_END
#pragma cplusplus reset

/* one escape pod, stride 0x18, 32 entries */
typedef struct PodObj {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    FloatPoint             vel;
    float                  life;
} PodObj;
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern PodObj *podData;
EXTERN_CPP_END
#pragma cplusplus reset

#ifndef MACBUILD
typedef struct UnsignedWide {
    uint32_t          lo;
    uint32_t          hi;
} UnsignedWide;

typedef struct DateTimeRec {
    short                  year;
    short                  month;
    short                  day;
    short                  hour;
    short                  minute;
    short                  second;
    short                  dayOfWeek;
} DateTimeRec;

typedef struct RGBColor {
    unsigned short         red;
    unsigned short         green;
    unsigned short         blue;
} RGBColor;

#pragma options align=packed
typedef struct Rect {
    short                  top;
    short                  left;
    short                  bottom;
    short                  right;
} Rect;
#pragma options align=reset
#endif

#ifndef MACBUILD
typedef struct ShortPoint {
    short                  h;
    short                  v;
} ShortPoint;
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern ShortPoint menuButtonPos[];
EXTERN_CPP_END
#pragma cplusplus reset

/* one beam in beamData, stride 0x22, 64 entries */
typedef struct BeamRecord {
    ShortPoint             head;
    ShortPoint             tail;
    ShortPoint             lastHead;
    ShortPoint             lastTail;
    short                  state;
    short                  frame;
    short                  weapType;
    short                  owner;
    short                  target;
    short                  lane;
    short                  mount;
    short                  targetIsShot;
    unsigned char          disableOnly;
    char pad21;
} BeamRecord;

typedef struct UpdateRectStruct {
    Rect                   updateRect;
    struct UpdateRectStruct *nextRectStructP;
} UpdateRectStruct, *UpdateRectStructPtr;

typedef struct GovtRecord {
    unsigned char          name[32];
    unsigned short         flags;
    unsigned short         flags2;
    unsigned short         scanMask;
    short                  classes[4];
    short                  allies[4];
    short                  enemies[4];
    short                  voiceType;
    short                  voiceClass;
    short                  interfaceID;
    short                  newsPict;
    short                  crimeTol;
    short                  penalties[5];
    short                  initialRec;
    short                  scanFine;
    short                  inhJam[4];
    char                   pad5E[2];
    float                  maxOdds;
    float                  skillMult;
    int32_t                   requires[2];
    RGBColor               rgb;
    RGBColor               rgb2;
    RGBColor               rgbBytes;
    Boolean                   reinfInhibited;
    Boolean                   iffScrambled;
    unsigned char          blessesPlayer[2];
    Boolean                   loaded;
    char                   pad87;
} GovtRecord;

typedef struct SpobRecord {
    struct SpriteRec       *sprite;
    ShortPoint             pos;
    short                  index;
    short                  govt;
    short                  minStatus;
    short                  type;
    short                  deadType;
    short                  techLevel;
    short                  system;
    short                  specialTech[8];
    short                  customPict;
    short                  customSound;
    short                  daysDominated;
    short                  weapon;
    short                  burstShots;
    uint32_t          flags;
    unsigned short         flags2;
    char                   pad36[2];
    int32_t                   landingFee;
    int32_t                   deadCounter;
    int32_t                   strength;
    Boolean                   loaded;
    Boolean                prepared;
    Boolean                dominated;
    Boolean                   defenseActive;
    unsigned char          name[32];
    char                   onDominate[255];
    char                   onRelease[255];
    char                   onDestroy[255];
    char                   onRegen[255];
    short                  defenseDude;
    short                  defWaveSize;
    short                  defCount;
    short                  tribute;
    short                  gravity;
    short                  explodType;
    short                  animDelay;
    short                  frame0Bias;
    short                  numFrames;
    short                  curFrame;
    short                  nextFrame;
    short                  deadDays;
    short                  deadDaysLeft;
    short                  hyperLinks[8];
    char                   pad48e[2];
    float                  frameTimer;
    float                  reloadTimer;
} SpobRecord;

/* one explosion type (explodType[], stride 8) */
typedef struct BoomRecord {
    float                  frameRate;
    short                  sound;
    short                  graphic;
} BoomRecord;

typedef struct AsteroidObj {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    FloatPoint             vel;
    float                  frame;
    float                  frameRate;
    short                  strength;
    short                  type;
    Boolean                   active;
    char                   pad21[3];
} AsteroidObj;

typedef struct BoxObj {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    FloatPoint             vel;
    float                  timer;
    float                  angle;
    short                  system;
    short                  pict;
    short                  spin;
    short                  cargoType;
    Boolean                   mining;
    char                   pad25[3];
} BoxObj;

/* the live preferences; PrefsRecord is their file image */
typedef struct Settings {
#if V111
    int32_t reserved;
#endif
    Boolean introMusic;
    Boolean voicesOn;
    Boolean clipping;
    Boolean noMovies;
    Boolean unused;
    Boolean noDebris;
    Boolean yieldEvents;
    Boolean parallaxStars;
    Boolean windowed;
    Boolean windowedPending;
    unsigned char shipAnimations;
    unsigned char engineGlows;
    unsigned char weaponGlows;
    unsigned char runningLights;
    Boolean ambientSounds;
    unsigned char showMapBorders;
    Boolean noHyperGamma;
    Boolean noVersionCheck;
    short volume;
#ifndef MAC1010
    short gamma;
#endif
} Settings;

#pragma options align=packed
typedef struct PrefsRecord {
    short sig;
    short musicEnabled;
    short unk4;
    short volume;
    short keys[36];
    short unk50;
    DateTimeRec date;
    short unk60;
    short earlyOut;
    short unk64;
    short noDebris;
    short windowed;
    short loadShipSets;
    short loadShipGlow;
    short loadShipWeapons;
    short loadShipLights;
    short parallaxStars;
    short planetSounds;
    short showInfluence;
    short noHyperGamma;
    short noVersionCheck;
#ifdef MAC1010
    short reserved[6];
#else
    short gamma;
    short unk7E;
    short unk80;
    short unk82;
    short unk84;
    short unk86;
#endif
    short unk88;
} PrefsRecord;
#pragma options align=reset

/* one offerable mission in missionAvailData, stride 0x12c */
typedef struct MissionAvailRec {
    short                  availStel;
    short                  availLoc;
    short                  availRecord;
    short                  availRating;
    short                  availRandom;
    short                  cargoQty;
    short                  cargoType;
    short                  availShipType;
    short                  sourceStellar;
    short                  destStellar;
    short                  timeLimit;
    Boolean                   avail;
    char pad017;
    unsigned short         flags;
    unsigned short         flags2;
    char                   availBits[256];
    uint32_t          requires[2];
    int32_t                   payment;
    short                  dispWeight;
    char pad12A[2];
} MissionAvailRec;

typedef struct BriefMissionRec {
    short                  destStellar;
    short                  destSystem;
    short                  cargoType;
    short                  cargoUnits;
    short                  returnStellar;
    short                  returnSystem;
    int32_t                   payment;
    DateTimeRec            dueDate;
    short                  unk1E;
} BriefMissionRec;

/* one entry of the mission table, stride 0x8e6; the pilot file holds sixteen */
#ifndef MACBUILD
#pragma options align=packed
#endif
typedef struct MissionRecord {
    short                  travelStellar;
    short                  unk2;
    short                  returnStellar;
    short                  shipCount;
    short                  shipDude;
    short                  shipGoal;
    short                  shipBehav;
    short                  shipStart;
    short                  shipSyst;
    short                  cargoType;
    short                  cargoQty;
    short                  pickupMode;
    short                  dropoffMode;
    short                  scanMask;
    short                  compGovt;
    short                  compReward;
    short                  datePostIncrement;
    int32_t                   payment;
    short                  specialShipsKilled;
    short                  specialShipsBoarded;
    short                  specialShipsDisabled;
    short                  specialShipsJumpedIn;
    short                  specialShipsJumpedOut;
    short                  initialShipCount;
    Boolean                   canAbort;
    Boolean                   hasCargo;
    Boolean                   unk34;
    short                  briefDesc;
    short                  quickBriefDesc;
    short                  loadCargoDesc;
    short                  dropCargoDesc;
    short                  compDesc;
    short                  failDesc;
    short                  refuseDesc;
    short                  shipDoneDesc;
    short                  timeLimit;
    short                  destStrList;
    short                  destStrIndex;
    short                  shipDelay;
    short                  misnIndex;
    short                  shipStrList;
    short                  shipStrIndex;
    short                  shipTypeIndex;
    unsigned short         flags;
    unsigned short         flags2;
    int32_t                   unk59[2];
    short                  auxShipCount;
    short                  auxShipDude;
    short                  auxShipSyst;
    short                  auxShipsSpawned;
    short                  auxShipDelay;
    short                  auxShipsLeft;
    char                   shipName[0x40];
    char                   shipSubtitle[0x40];
    char                   unkED[0xff];
    char                   beginSetString[0xff];
    char                   setString2[0xff];
    char                   succSetString[0xff];
    char                   failSetString[0xff];
    char                   abortSetString[0xff];
    char                   shipDoneSetString[0xff];
    char                   name[0x100];
} MissionRecord;
#ifndef MACBUILD
#pragma options align=reset
#endif

typedef struct MissionState {
    Boolean                active;
    Boolean                   returnDone;
    Boolean                objectivesDone;
    Boolean                failed;
    unsigned short         flags;
    DateTimeRec            dueDate;
} MissionState;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern MissionState *currentMissionObjectives;
EXTERN_CPP_END
#pragma cplusplus reset

/* one type: weaponStats[] and each ship's weap[], whose copy keeps count, ammo and reload countdown here */
typedef struct WeapRecord {
    union { short lifetime; short count; };
    short                  damage[2];      /* mass, energy */
    short                  guidance;
    union { short ammoType; short ammo; };
    short                  graphic;
    short                  inaccuracy;
    short                  sound;
    short                  impact;
    short                  recoil;
    short                  explodType;
    short                  proxRadius;
    short                  blastRadius;
    short                  decay;
    short                  proxSafety;
    short                  maxAmmo;
    unsigned short         flags;
    unsigned short         flags2;
    unsigned short         flags3;
    unsigned short         seeker;
    unsigned short         smokeSet;
    char pad2A[0x2];
    float                  speed100;
    union { float reload; float reloadTimer; };
    float                  partVel[8];
    float                  hitPartVel;
    float                  guidedTurn;
    float                  totalRange;
    short                  particles;
    short                  partLifeMin;
    short                  partLifeMax;
    short                  hitParticles;
    short                  hitPartLife;
    short                  ionization;
    short                  exitType;
    short                  durability;
    short                  beamLength;
    short                  beamWidth;
    short                  coronaFalloff;
    short                  lightningDensity;
    short                  lightningAmplitude;
    short                  subCount;
    short                  subType;
    short                  subTheta;
    short                  subLimit;
    char pad82[0x2];
    uint32_t          beamColor;
    uint32_t          coronaColor;
    uint32_t          partColor[8];
    uint32_t          hitPartColor;
    uint32_t          ionizeColor;
    short                  burstCount;
    short                  burstReload;
    short                  decayShots;
    short                  jamVuln[4];
    RGBColor               rgbScratch;
} WeapRecord;

typedef struct RoidRecord {
    short                  strength;
    short                  yieldQty;
    short                  yieldType;
    short                  fragType[2];
    short                  fragCount;
    short                  partCount;
    short                  mass;
    short                  explodType;
    char pad12[0x2];
    float                  frameRate;
    uint32_t          color;
} RoidRecord;

typedef struct PrefsFileRec {
    struct PrefsNode       *file;
    int32_t                   dirty;
} PrefsFileRec;

/* the four shot sprite layers, in address order */
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern struct SpriteLayerRec *shotLayer;
extern struct SpriteLayerRec *guidedShotLayer;
extern struct SpriteLayerRec *pointDefenseLayer;
extern struct SpriteLayerRec *turretedShotLayer;

extern BoomSlot *explodData;
extern BriefMissionRec briefMissionInfo[];
extern unsigned char hypergateMapMode;
extern Boolean *missionBit;
extern char parseBuffer[];
extern char briefingTextPtr[0x2000];
extern unsigned char playerName[256];
extern char creditsCString[0x20];
extern short playerEscortOrders[4];
extern short hyperRoute[32];
extern unsigned char tonString[0x100];
extern unsigned char tonsString[0x100];
extern unsigned char lcCargoName[256][0x100];
extern unsigned char cargoName[256][0x100];
extern unsigned char cargoAbbrev[256][0x100];
extern unsigned char creditString[0x100];
extern unsigned char creditsString[0x100];
extern unsigned char legalStatusStrings[18][256];
extern unsigned char govtTargStrings[256][256];
#define kButtonLabelCount 61
/* buttonLabel[] indices: STR# 150 (kButtonStrList) entry - 1 */
#define kLeaveButtonLabel           0
#define kBuyButtonLabel             1
#define kSellButtonLabel            2
#define kBuyShipButtonLabel         3
#define kDoneButtonLabel            4
#define kRechargeButtonLabel        5
#define kTradeCenterButtonLabel     6
#define kOutfitterButtonLabel       7
#define kShipyardButtonLabel        8
#define kBarButtonLabel             9
#define kGambleButtonLabel          10
#define kHolovidButtonLabel         11
#define kHireEscortButtonLabel      12
#define kBet1000ButtonLabel         13
#define kBet5000ButtonLabel         14
#define kMissionBBSButtonLabel      15
#define kMinusButtonLabel           16      /* "-" */
#define kPlusButtonLabel            17      /* "+" */
#define kScrollUpButtonLabel        18      /* "^" */
#define kScrollDownButtonLabel      19      /* "&" */
#define kCloseChannelButtonLabel    20
#define kGreetingsButtonLabel       21
#define kRequestAssistButtonLabel   22
#define kOfferBribeButtonLabel      23
#define kBegMercyButtonLabel        24
#define kAcceptButtonLabel          25
#define kOkButtonLabel              26
#define kAcceptPaymentButtonLabel   27
#define kDemandMoreButtonLabel      28
#define kAcceptPriceButtonLabel     29
#define kLowerPriceButtonLabel      30
#define kReleaseButtonLabel         31
#define kSelectButtonLabel          32
#define kCancelButtonLabel          33
#define kAbortButtonLabel           34
#define kGeneralButtonLabel         35
#define kCargoButtonLabel           36
#define kExtrasButtonLabel          37
#define kHonorsButtonLabel          38
#define kPlunderCargoButtonLabel    39      /* "Cargo", the plunder dialog's */
#define kCreditsButtonLabel         40
#define kAmmoButtonLabel            41
#define kEnergyButtonLabel          42
#define kCaptureShipButtonLabel     43
#define kDemandTributeButtonLabel   44
#define kUseAsEscortButtonLabel     45
#define kUseAsMyShipButtonLabel     46
#define kInfoButtonLabel            47
#define kClearRouteButtonLabel      48
#define kYesButtonLabel             49
#define kNoButtonLabel              50
#define kUpgradeEscortButtonLabel   51
#define kCancelUpgradeButtonLabel   52
#define kSellEscortButtonLabel      53
#define kCancelSaleButtonLabel      54
#define kShowBordersButtonLabel     55
#define kHideBordersButtonLabel     56
#define kHelpButtonLabel            57
#define kBetButtonLabel             58
#define kFindButtonLabel            59
#define kJettisonButtonLabel        60
/* DrawButton states */
#define kButtonDim                  (-1)
#define kButtonDisabled             -2
#define kButtonPicture              (-3)
extern unsigned char buttonLabel[kButtonLabelCount][256];
extern char datePrefix[0x10];
extern char dateSuffix[0x10];
extern RoidRecord asteroidType[16];
extern short dockedPortMissions[2][1000];
extern unsigned char fuelString[8][0x100];
extern unsigned char ratingStrings[11][256];
extern unsigned char shipHelpRequests[20][256];
extern unsigned char statusCargoName[6][0x100];
extern unsigned char playerNickname[256];
extern short missionRandom[1000];
extern short prevParent[64];
#ifndef MACBUILD
typedef struct Image_t *PicHandle;
#endif
extern PicHandle missionBackgroundPic[3];
extern DialogPtr alertDialog;
EXTERN_CPP_END
#pragma cplusplus reset

typedef struct KeyPrefs {
    short                  selectSecondary;
    short                  deselectSecondary;
    short                  firePrimary;
    short                  fireSecondary;
    short                  hail;
    short                  land;
    short                  dismiss;
    short                  faceTarget;
    short                  clearTarget;
    short                  map;
    short                  targetNext;
    short                  targetNearest;
    short                  hyperspace;
    short                  nextSystem;
    short                  jump;
    short                  jettison;
    short                  board;
    short                  eject;
    short                  selfDestruct;
    short                  turnLeft;
    short                  turnRight;
    short                  accelerate;
    short                  reverse;
    short                  pause;
    short                  afterburner;
    short                  playerInfo;
    short                  navPreset[4];
    short                  cheatSpawn;
    short                  cheatKill;
    short                  cheatRevive;
    short                  cheatArmor;
    short                  cheatDamage;
    short                  cheatAmmo;
    short                  cheatCredits;
    short                  cheatSpare;
    short                  cheatShipNext;
    short                  cheatShipPrev;
    short                  missions;
    short                  cloak;
    short                  escortMenu;
    short                  quickKey[5];
    short                  escortOrder[4];
    short                  toggleFPS;
} KeyPrefs;

#ifdef __cplusplus
}
#endif

#endif
