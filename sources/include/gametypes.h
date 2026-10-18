/* gametypes.h -- shared record and runtime-object layouts; types only. Notes: Research/notes/gametypes.md */
#ifndef GAMETYPES_H
#define GAMETYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FloatPoint {
    float                  x;
    float                  y;
} FloatPoint;

typedef struct MapScroll {
    short                  x;
    short                  y;
} MapScroll;
#pragma cplusplus on
extern "C++" {
extern MapScroll mapScroll;
}
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

typedef unsigned char Str255[256];

typedef struct DialogRecord *DialogPtr;

typedef struct PictCacheSlot {
    short                  picID;
    char pad02[0x2];
    unsigned long          stamp;
} PictCacheSlot;

typedef struct ResRecord {
    struct RezFile         *rez;
    long                   index;
    unsigned long          type;
    short                  id;
    char                   name[258];
} ResRecord;

/* one resource type in a RezFile's type table, stride 0x0c. */
typedef struct RezTypeEntry {
    unsigned long          type;
    ResRecord              *first;
    unsigned long          count;
} RezTypeEntry;

/* BurgerLib's RezHeader_t (0x24 bytes), then the port's type table */
typedef struct RezFile {
    void                   (*decompress[3])(unsigned char *dst, unsigned char *src,
                                            unsigned long length, unsigned long packLength);
    unsigned long          groupCount;
    unsigned long          nameCount;
    void                   *file;
    struct RezGroup        **groups;
    struct RezName         **names;
    unsigned long          flags;
    RezTypeEntry           *types;
    unsigned long          typeCount;
} RezFile;

/* one parallax star slot (starData[], stride 0x14) */
typedef struct StarSlot {
    struct SpriteRec       *sprite;
    char                   live;
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

/* one smoke slot in gDebrisSlots, stride 0x18 */
typedef struct DebrisSlot {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    short                  type;
    float                  timer;
    float                  angle;
} DebrisSlot;
#pragma cplusplus on
extern "C++" {
extern DebrisSlot *gDebrisSlots;
}
#pragma cplusplus reset

/* one escape pod, stride 0x18, 32 entries */
typedef struct PodObj {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    FloatPoint             vel;
    float                  life;
} PodObj;
#pragma cplusplus on
extern "C++" {
extern PodObj *podData;
}
#pragma cplusplus reset

typedef struct UnsignedWide {
    unsigned long          lo;
    unsigned long          hi;
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

typedef struct ShortPoint {
    short                  h;
    short                  v;
} ShortPoint;
#pragma cplusplus on
extern "C++" {
extern ShortPoint menuButtonPos[];
}
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

typedef struct WeapSlot {
    short                  count;
    char                   unk2[6];
    short                  ammo;
    char                   unkA[0x26];
    float                  reloadTimer;
    char                   unk34[0x80];
    short                  burstCount;
    char                   unkB6[0x12];
} WeapSlot;

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
    long                   requires[2];
    RGBColor               rgb;
    RGBColor               rgb2;
    unsigned short         rgbBytes[3];
    char                   reinfInhibited;
    char                   iffScrambled;
    char                   blessesPlayer[2];
    char                   loaded;
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
    unsigned long          flags;
    unsigned short         flags2;
    char                   pad36[2];
    long                   landingFee;
    long                   deadCounter;
    long                   strength;
    char                   loaded;
    char                   prepared;
    char                   dominated;
    char                   defenseActive;
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
    char                   active;
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
    char                   mining;
    char                   pad25[3];
} BoxObj;

#pragma options align=packed
typedef struct PrefsRecord {
    short sig;
    short musicEnabled;
    short unk4;
    short volume;
    short keys[36];
    short unk50;
    char unk52[0xe];
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
    short gamma;
    short unk7E;
    short unk80;
    short unk82;
    short unk84;
    short unk86;
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
    char                   avail;
    char pad017;
    unsigned short         flags;
    unsigned short         flags2;
    char                   availBits[256];
    unsigned long          requires[2];
    long                   payment;
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
    long                   payment;
    DateTimeRec            dueDate;
    short                  unk1E;
} BriefMissionRec;

#pragma options align=packed
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
    long                   payment;
    short                  specialShipsKilled;
    short                  specialShipsBoarded;
    short                  specialShipsDisabled;
    short                  specialShipsJumpedIn;
    short                  specialShipsJumpedOut;
    short                  initialShipCount;
    char                   canAbort;
    char                   hasCargo;
    char                   unk34;
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
    long                   unk59;
    long                   unk5D;
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
#pragma options align=reset

typedef struct MissionState {
    char                   active;
    char                   returnDone;
    char                   objectivesDone;
    char                   failed;
    unsigned short         flags;
    DateTimeRec            dueDate;
} MissionState;

#pragma cplusplus on
extern "C++" {
extern MissionState *currentMissionObjectives;
}
#pragma cplusplus reset

typedef struct WeapRecord {
    short                  lifetime;
    short                  massDmg;
    short                  energyDmg;
    short                  guidance;
    short                  ammoType;
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
    float                  reload;
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
    unsigned long          beamColor;
    unsigned long          coronaColor;
    unsigned long          partColor[8];
    unsigned long          hitPartColor;
    unsigned long          ionizeColor;
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
    unsigned long          color;
} RoidRecord;

typedef struct PrefsFileRec {
    struct PrefsNode       *file;
    long                   dirty;
} PrefsFileRec;

/* the four shot sprite layers, in address order */
#pragma cplusplus on
extern "C++" {
extern struct SpriteLayerRec *shotLayer;
extern struct SpriteLayerRec *guidedShotLayer;
extern struct SpriteLayerRec *pointDefenseLayer;
extern struct SpriteLayerRec *turretedShotLayer;

extern BoomSlot *explodData;
extern BriefMissionRec briefMissionInfo[];
extern unsigned char hypergateMapMode;
extern char *missionBit;
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
extern unsigned char legalStrings[18][256];
extern unsigned char govtTargStrings[256][256];
#define kButtonLabelCount 61
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
extern struct Image_t *missionBackgroundPic[3];
extern DialogPtr alertDialog;
}
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
