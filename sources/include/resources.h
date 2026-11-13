/* resources.h -- decrypted in-memory forms of the Classic-Mac resource records. Notes: Research/notes/resources.md */
#include "gametypes.h"

#ifndef RESOURCES_H
#define RESOURCES_H

#ifdef __cplusplus
extern "C" {
#endif

/* oütf Flags bits (Nova Bible) */
#define kOutfFixedGun    0x001
#define kOutfTurret      0x002
#define kOutfPersistent  0x004   /* survives ship trade */
#define kOutfCantSell    0x008
#define kOutfRemoveAfterBuy 0x010
#define kOutfMissionPersistent 0x020 /* survives a mission's ship change */
#define kOutfCheckRequire 0x100
#define kOutfCostScaled  0x200
#define kOutfMassScaled  0x400   /* mass scales with the player's ship */
#define kOutfSellAnywhere 0x800
#define kOutfSupersedes  0x1000
#define kOutfShowInRanks 0x2000
#define kOutfTestAvailability 0x4000

#define OutfCostScaled(o)  ((o)->flags & kOutfCostScaled)
#define OutfMassScaled(o)  ((o)->flags & kOutfMassScaled)
#define OutfPersistent(o)  ((o)->flags & kOutfPersistent)

#define kOutfTechAbsent  0x7fff

typedef struct OutfRecord {
    short techLevel;
    short mass;
    short modType[4];
    short modVal[4];
    short maxCount;
    short flags;
    short itemClass;
    short dispWeight;
    short buyRandom;
    short dailyRoll;
    short sameNameIndex;
    short govtRestrict;
    int32_t cost;
    unsigned short scanMask;
    unsigned char name[64];
    char availability[255];
    char onPurchase[255];
    char onSell[255];
    Boolean pad367;
    int32_t contributes[2];
    int32_t requires[2];
    Boolean persistent;
    char pad379[3];
} OutfRecord;

struct ShipRec;
struct ShipObj;

extern int32_t AdjustedItemCost(OutfRecord *outf, struct ShipRec *ship);
extern int32_t AdjustedItemMass(OutfRecord *outf, struct ShipRec *ship);
extern void GetShipPaintColor(struct ShipObj *ship, unsigned short *red,
                         unsigned short *green, unsigned short *blue);
extern void MungeBriefing(Boolean briefing, short which);
extern unsigned char LaunchFighter(struct ShipObj *parent, short fighterType);
extern void SpawnSmoke(FloatPoint pos, short type, short sprite);

typedef struct ShipRec {
    short cargo;
    short fuel;
    short freeMass;
    short maxGuns;
    short maxTurrets;
    short techLevel;
    short strength;
    short escortType;
    short deathDelay;
    short inherentAI;
    short defOutfID[8];
    short defOutfCount[8];
    short fuelRegen;
    char pad36[2];
    float turnRate;
    float accel;
    float maxSpeed;
    float mobilityFactor;
    float deionize;
    float weapDecay;
    float shieldRech;
    float armorRech;
    int32_t cost;
    int32_t shield;
    int32_t armor;
    int32_t escUpgradeCost;
    int32_t tradeIn;
    unsigned char name[64];
    unsigned char subtitle[64];
    char availability[255];
    char onPurchase[255];
    char appearOn[255];
    char onCapture[255];
    char onRetire[255];
    Boolean pad5E7;
    short weapCount[256];
    short weapAmmo[256];
    short flags;
    short flags2;
    short flags3;
    short mass;
    short shipLength;
    short crew;
    short inherentGovt;
    short inherentGovtB;
    short skillVar;
    short ionizeMax;
    short altSets;
    short animDelay;
    short baseSets;
    unsigned short transparency;
    short keyCarried;
    short framesPer;
    short spriteIndex;
    short shipyardPictID;
    short targetPictID;
    short dispWeight;
    short aiSkill;
    short blinkMode;
    short blinkValue[4];
    short explode1;
    short explode2;
    short podCount;
    short upgradeTo;
    unsigned short shanFlags;
    unsigned short hireRandom;
    unsigned short buyRandom;
    unsigned short dailyHireRoll;
    unsigned short dailySaleRoll;
    char padA2E[2];
    int32_t contributes[2];
    int32_t requires[2];
    Boolean cargoNotFixed;
    unsigned char canAppear;
    ShortPoint weapPos[4][4];
    short weapPosZ[4][4];
    char padAA2[2];
    FloatPoint compress[2];
    short *frameAngles;
    short registered;
    short hacked;
} ShipRec;

/* Nova's resource types, four Mac Roman characters each */
#define kBoomType 0x629a9a6d   /* 'bööm' */
#define kCharType 0x63688a72   /* 'chär' */
#define kColrType 0x639a6c72   /* 'cölr' */
#define kCronType 0x63729a6e   /* 'crön' */
#define kDescType 0x64917363   /* 'dësc' */
#define kDudeType 0x649f6465   /* 'düde' */
#define kEBugType 0x91627567   /* 'ëbug' */
#define kFletType 0x666c9174   /* 'flët' */
#define kGovtType 0x679a7674   /* 'gövt' */
#define kIntfType 0x956e7466   /* 'ïntf' */
#define kJunkType 0x6a9f6e6b   /* 'jünk' */
#define kLeetType 0x6c333374   /* 'l33t' */
#define kMisnType 0x6d95736eUL /* 'mïsn' */
#define kMisnResSize 0x7b2     /* bytes in one mïsn resource */
#define kNebuType 0x6e916275   /* 'nëbu' */
#define kOopsType 0x9a6f7073   /* 'öops' */
#define kOutfType 0x6f9f7466   /* 'oütf' */
#define kPersType 0x70917273   /* 'përs' */
#define kRankType 0x728a6e6b   /* 'ränk' */
#define kRoidType 0x729a6964   /* 'röid' */
#define kShanType 0x73688a6eUL /* 'shän' */
#define kShipType 0x73689570   /* 'shïp' */
#define kSpinType 0x7370956eUL /* 'spïn' */
#define kSpobType 0x73709a62   /* 'spöb' */
#define kSystType 0x73d87374   /* 'sÿst' */
#define kWeapType 0x77916170   /* 'wëap' */
#define kYearType 0x79918aa8   /* 'yëä®' */

#define kResAbsent          (-9999)
#define kResUnloaded        9999

/* the resource records as stored (Nova Bible layouts) */
#pragma options align=mac68k

typedef struct SpinRec {
    short spriteID;
    short maskID;
    short xSize;
    short ySize;
    short xTiles;
    short yTiles;
} SpinRec, **SpinHandle;

/* resized to 0xc0 on the way out */
typedef struct ShanRec {
    short baseImageID;
    short baseMaskID;
    short baseSetCount;
    short baseXSize;
    short baseYSize;
    short baseTransparency;
    short altImageID;
    short altMaskID;
    short altSetCount;
    short altXSize;
    short altYSize;
    short glowImageID;
    short glowMaskID;
    short glowXSize;
    short glowYSize;
    short lightImageID;
    short lightMaskID;
    short lightXSize;
    short lightYSize;
    short weapImageID;
    short weapMaskID;
    short weapXSize;
    short weapYSize;
    short flags;
    short animDelay;
    short weapDecay;
    short framesPer;
    short blinkMode;
    short blinkValue[4];
    short shieldImageID;
    short shieldMaskID;
    short shieldXSize;
    short shieldYSize;
    struct {
        short x[4];             /* gun/turret/guided/beam X, four lanes */
        short y[4];             /* ... and Y */
    } mountPos[4];
    short upCompressX;
    short upCompressY;
    short downCompressX;
    short downCompressY;
    short mountPosZ[4][4];
    short unkB0[8];
} ShanRec;

/* one packed 24-bit colour, 3 bytes used of 4 */
typedef union RawTriple {
    unsigned short w;
    uint32_t rgb;
} RawTriple;

typedef struct IntfRec {
    RawTriple brightText;
    RawTriple dimText;
    Rect radarArea;
    RawTriple brightRadar;
    RawTriple dimRadar;
    Rect shieldArea;
    RawTriple shieldColor;
    Rect armorArea;
    RawTriple armorColor;
    Rect fuelArea;
    RawTriple fuelFull;
    RawTriple fuelPartial;
    Rect navArea;
    Rect weapArea;
    Rect targArea;
    Rect cargoArea;
    char statusFont[64];
    short statFontSize[2];
    short statusBkgnd;
} IntfRec;

typedef struct OopsRes {
    short stellar;
    short commodity;
    short priceDelta;
    short duration;
    short freq;
    char activateOn[256];
    short reserved[8];
} OopsRes;

typedef struct BoomRes {
    short frameAdvance;
    short soundIndex;
    short graphicIndex;
} BoomRes;

typedef struct NebuRes {
    short xPos;
    short yPos;
    short xSize;
    short ySize;
    char activeOn[255];
    char onExplore[255];
} NebuRes;

typedef struct MisnRes {
    short availStel;
    short unused02;
    short availLoc;
    short availRecord;
    short availRating;
    short availRandom;
    short travelStel;
    short returnStel;
    short cargoType;
    short cargoQty;
    short pickupMode;
    short dropOffMode;
    short scanMask;
    short unused1A;
    int32_t payVal;
    short shipCount;
    short shipSyst;
    short shipDude;
    short shipGoal;
    short shipBehav;
    short shipNameID;
    short shipStart;
    short compGovt;
    short compReward;
    short shipSubtitle;
    short briefText;
    short quickBrief;
    short loadCargText;
    short dumpCargoText;
    short compText;
    short failText;
    short timeLimit;
    short canAbort;
    short shipDoneText;
    short unused46;
    short auxShipCount;
    short auxShipDude;
    short auxShipSyst;
    short unused4E;
    short flags;
    short flags2;
    short unused54;
    short unused56;
    short refuseText;
    short availShipType;
    char availBits[255];
    char onAccept[255];
    char onRefuse[255];
    char onSuccess[255];
    char onFailure[255];
    char onAbort[255];
    uint32_t require1;
    uint32_t require2;
    short datePostInc;
    char onShipDone[255];
    char acceptButton[32];
    char refuseButton[32];
    short dispWeight;
    short reserved[8];
} MisnRes;

typedef struct SpobRes {
    short xPos;
    short yPos;
    short type;
    uint32_t flags;
    short tribute;
    short techLevel;
    short specialTech[3];
    short govt;
    short minStatus;
    short custPicID;
    short custSndID;
    short defenseDude;
    short defCount;
    short flags2;
    short animDelay;
    short frame0Bias;
    short hyperLink[8];
    char onDominate[255];
    char onRelease[255];
    int32_t fee;
    short gravity;
    short weapon;
    int32_t strength;
    short deadType;
    short deadTime;
    short explodType;
    char onDestroy[255];
    char onRegen[255];
    short specialTech2[5];
    short reserved[8];
} SpobRes;

typedef struct OutfRes {
    short dispWeight;
    short mass;
    short techLevel;
    short modType;
    short modVal;
    short max;
    short flags;
    int32_t cost;
    short modType2;
    short modVal2;
    short modType3;
    short modVal3;
    short modType4;
    short modVal4;
    uint32_t contribute1;
    uint32_t contribute2;
    uint32_t require1;
    uint32_t require2;
    char availability[255];
    char onPurchase[255];
    char onSell[255];
    char shortName[64];
    char lcName[64];
    char lcPlural[65];
    short itemClass;
    short scanMask;
    short buyRandom;
    short require3;
    short reserved[8];
} OutfRes;

typedef struct RankRes {
    short weight;
    short attachTo;
    short flags;
    int32_t salary;
    int32_t salaryCap;
    uint32_t contribute[2];
    short priceMod;
    char name[64];
    char shortName[64];
} RankRes;

typedef struct SystRes {
    short xPos;
    short yPos;
    short con[16];
    short nav[16];
    short dudeTypes[8];
    short probs[8];
    short avgShips;
    short govt;
    short message;
    short asteroids;
    short interference;
    short person[8];
    short personProb[8];
    uint32_t bkgndColor;
    short murk;
    short astTypes;
    char visibility[256];
    short reinfFleet;
    short reinfTime;
    short reinfInterval;
    short reserved[8];
} SystRes;

typedef struct WeapRes {
    short reload;
    short count;
    short massDmg[2];
    short guidance;
    short speed;
    short ammoType;
    short graphic;
    short inaccuracy;
    short sound;
    short impact;
    short explodType;
    short proxRadius;
    short blastRadius;
    short flags;
    short seeker;
    short smokeSet;
    short decay;
    short particles;
    short partVel;
    short partLifeMin;
    short partLifeMax;
    uint32_t partColor;
    short beamLength;
    short beamWidth;
    short falloff;
    uint32_t beamColor;
    uint32_t coronaColor;
    short subCount;
    short subType;
    short subTheta;
    short subLimit;
    short proxSafety;
    short flags2;
    short ionization;
    short hitParticles;
    short hitPartLife;
    short hitPartVel;
    uint32_t hitPartColor;
    short recoil;
    short exitType;
    short burstCount;
    short burstReload;
    short jamVuln[4];
    short flags3;
    short durability;
    short guidedTurn;
    short maxAmmo;
    short liCohesive;
    short liAmplitude;
    uint32_t liColor;
    short reserved[8];
} WeapRes;

typedef struct DudeRes {
    short aiType;
    short govt;
    short booty;
    short infoTypes;
    short shipTypes[16];
    short probs[16];
    short reserved[8];
} DudeRes;

typedef struct GovtRes {
    short voiceType;
    short flags;
    short flags2;
    short scanFine;
    short crimeTol;
    short penalty[5];
    short initialRec;
    short maxOdds;
    short classes[4];
    short allies[4];
    short enemies[4];
    short skillMult;
    short scanMask;
    char commName[16];
    char targetCode[16];
    uint32_t require[2];
    short inhJam[4];
    char mediumName[64];
    uint32_t color;
    uint32_t shipColor;
    short interfaceID;
    short newsPic;
    short reserved[8];
} GovtRes;

typedef struct FletRes {
    short leadShip;
    short escortShip[4];
    short minCount[4];
    short maxCount[4];
    short govt;
    short linkSyst;
    char activateOn[256];
    short quote;
    short flags;
    short reserved[8];
} FletRes;

typedef struct JunkRes {
    short soldAt[8];
    short boughtAt[8];
    short basePrice;
    short flags;
    short scanMask;
    char lcName[64];
    char abbrev[64];
    char buyOn[255];
    char sellOn[255];
} JunkRes;

typedef struct CronRes {
    short firstDay;
    short firstMonth;
    short firstYear;
    short lastDay;
    short lastMonth;
    short lastYear;
    short random;
    short duration;
    short preHoldoff;
    short postHoldoff;
    short flags;
    short indNewsStr;
    char enableOn[255];
    char onStart[255];
    char onEnd[255];
    uint32_t contribute[2];
    uint32_t require[2];
    short newsGovt[4];
    short govtNewsStr[4];
} CronRes;

typedef struct RoidRes {
    short strength;
    short spinRate;
    short yieldType;
    short yieldQty;
    short partCount;
    uint32_t partColor;
    short fragType[2];
    short fragCount;
    short explodType;
    short mass;
    short reserved[8];
} RoidRes;

typedef struct ColrRes {
    uint32_t buttonColor[3];
    char menuFont[64];
    short menuFontSize;
    uint32_t menuColor[2];
    uint32_t gridColor[2];
    Rect progressBar;
    uint32_t progressColor[3];
    short buttonPos[12];
    uint32_t floatingMapColor;
    uint32_t listColor[3];
    uint32_t escortHiliteColor;
    char buttonFont[64];
    short buttonFontSize;
    short logoPos[2];
    short rolloverPos[2];
    short slidePos[6];
} ColrRes;

typedef struct ShipRes {
    short cargo;
    short shield;
    short accel;
    short speed;
    short turn;
    short energy;
    short freeSpace;
    short armor;
    short shieldRe;
    short wType[4];
    short wCount[4];
    short ammo[4];
    short maxGun;
    short maxTur;
    short techLevel;
    int32_t cost;
    short deathDelay;
    short armorRe;
    short explode[2];
    short dispWeight;
    short mass;
    short length;
    short inherentAI;
    short crew;
    short strength;
    short inherentGovt;
    short flags;
    short podCount;
    short defaultItems[4];
    short itemCount[4];
    short fuelRegen;
    short skillVar;
    short flags2;
    uint32_t contribute1;
    uint32_t contribute2;
    char availability[255];
    char appearOn[255];
    char onPurchase[255];
    short deionize;
    short ionizeMax;
    short keyCarried;
    short defaultItems2[4];
    short itemCount2[4];
    uint32_t require1;
    uint32_t require2;
    short buyRandom;
    short hireRandom;
    short unused38C[34];
    char onCapture[255];
    char onRetire[255];
    char shortName[64];
    char commName[32];
    char longName[128];
    char movieName[31];
    short wType2[4];
    short wCount2[4];
    short ammo2[4];
    char subtitle[64];
    short flags3;
    short upgradeTo;
    uint32_t escUpgrdCost;
    uint32_t escSellValue;
    short escortType;
    short reserved[8];
} ShipRes;

typedef struct PersRes {
    short linkSyst;
    short govt;
    short aiType;
    short aggress;
    short coward;
    short shipType;
    Rect weapType;
    Rect weapCount;
    Rect ammoLoad;
    int32_t credits;
    short shieldMod;
    short hailPict;
    short commQuote;
    short hailQuote;
    short linkMission;
    short flags;
    char activateOn[256];
    short grantClass;
    short grantCount;
    short grantProb;
    char subtitle[64];
    uint32_t shipColor;
    short flags2;
    short reserved[8];
} PersRes;

/* the text, then (when the resource is longer) the PICT id, the movie name and the flags */
typedef struct DescRes {
    unsigned char text[1];
} DescRes;

#pragma options align=reset

/* WeapRecord.guidance, the Nova Bible's wëap Guidance values */
#define kGuidanceUnguided   (-1)
#define kGuidanceBeam       0
#define kGuidanceHoming     1
#define kGuidanceTurretBeam 3
#define kGuidanceTurret     4
#define kGuidanceFreefall   5       /* freefall bomb */
#define kGuidanceRocket     6       /* freeflight rocket */
#define kGuidanceFrontQuad  7       /* front-quadrant turret */
#define kGuidanceRearQuad   8
#define kGuidancePointDef   9       /* point-defence turret */
#define kGuidancePDBeam     10      /* point-defence beam */
#define kGuidanceBay        99      /* carried ship: a fighter bay */

typedef struct CronRecord {
    Boolean active;
    Boolean loaded;
    short firstYear;
    short firstMonth;
    short firstDay;
    char unk8[8];
    short lastYear;
    short lastMonth;
    short lastDay;
    char unk16[8];
    short random;
    short duration;
    short preHoldoff;
    short postHoldoff;
    short daysLeft;
    short holdoffLeft;
    short govts[4];
    short govtValues[4];
    short indNewsStr;
    unsigned short flags;
    char pad3E[2];
    int32_t contributes[2];
    int32_t requires[2];
    char enableOn[255];
    char onStart[255];
    char onEnd[258];
} CronRecord;

typedef struct PersRecord {
    short linkSyst;
    short govt;
    short aiType;
    short aggression;
    short coward;
    short shipType;
    short commQuote;
    short hailQuote;
    short linkMission;
    short hailPict;
    unsigned short flags;
    unsigned short flags2;
    short weapCount[256];
    char unk218[0x200];
    short weapAmmo[256];
    int32_t credits;
    float shieldMod;
    Boolean loaded;
    Boolean grudge;
    Boolean canAppear;
    Boolean prepared;
    char name[31];
    Boolean unk643;
    char activeOn[255];
    char subtitle[65];
    short grantClass;
    short grantProb;
    short grantCount;
    short sameNameIndex;
#ifdef MAC1010
    union {
        unsigned short rgb5[3];
        RGBColor shipColor;
    };
#else
    unsigned short rgb5[3];
#endif
    char pad792[2];
} PersRecord;

#define kPersCaptainHector 1023 /* the hand-built shareware enforcer */
#define kPersBountyHunter  1022

typedef struct SystRecord {
    MapScroll pos;
    short influenceX;
    short influenceY;
    short govt;
    short links[16];
    short navDefault[16];
    short dudeTypes[8];
    short dudeProbs[8];
    short fleets[8];
    short fleetProbs[8];
    short fleetCount;
    short fleetProbTotal;
    short avgShips;
    short exploreLevel;
    short message;
    short asteroidCount;
    short interference;
    short persons[8];
    short persProbs[8];
    short colocNext;
    short colocGroup;
    short murk;
    short reinforceFleet;
    short reinforceTime;
    short reinforceInterval;
    short reinforceDaysLeft;
    char padC6[2];
    float reinforceTimer;
    unsigned char name[32];
    char visibility[255];
    Boolean loaded;
    Boolean known;
    Boolean prepared;
    RGBColor rgb;
    unsigned short astTypes;
    char pad1F6[2];
    uint32_t mapPixel;
} SystRecord;

typedef struct NebuRecord {
    short xPos;
    short yPos;
    short width;
    short height;
    Boolean loaded;
    Boolean explored;
    char activeOn[255];
    char onExplore[255];
} NebuRecord;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern NebuRecord *nebulaData;
EXTERN_CPP_END
#pragma cplusplus reset

typedef struct DudeRecord {
    short aiType;
    short govt;
    short shipTypes[16];
    short shipProbs[16];
    unsigned short flags;
    unsigned short adviceFlags;
    Boolean loaded;
    Boolean pad49;
} DudeRecord;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern DudeRecord *dudeData;
EXTERN_CPP_END
#pragma cplusplus reset

typedef struct JunkRecord {
    short soldAt[8];
    short boughtAt[8];
    short basePrice;
    short held;
    unsigned short flags;
    unsigned short scanMask;
    unsigned char name[0x100];
    char abbrev[256];
    char lcName[256];
    char buyOn[255];
    char sellOn[255];
} JunkRecord;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern JunkRecord *junkData;
extern BoomRecord *explodType;
EXTERN_CPP_END
#pragma cplusplus reset

/* rank runtime record, stride 0x120 in the rankData array. */
typedef struct RankRecord {
    Boolean active;
    Boolean loaded;
    short index;
    short priority;
    short govt;
    short priceMod;
    char padA[2];
    int32_t salary;
    int32_t salaryCap;
    int32_t contributes[2];
    unsigned short flags;
    char name[128];
    char convName[64];
    char shortName[66];
} RankRecord;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern RankRecord *rankData;
EXTERN_CPP_END
#pragma cplusplus reset

typedef struct OopsRecord {
    short stellar;
    short stellarID;
    short commodity;
    short amount;
    short duration;
    short freq;
    short countdown;
    char text[255];
    unsigned char name[256];
    Boolean unk20D;
    Boolean loaded;
    Boolean pad20F;
} OopsRecord;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern OopsRecord *disasterData;
EXTERN_CPP_END
#pragma cplusplus reset

typedef struct FletRecord {
    short leadShipType;
    short govt;
    short escortTypes[4];
    short escortMin[4];
    short escortMax[4];
    short linkSyst;
    short quoteStrList;
    char activateOn[255];
    Boolean pad11F;
    unsigned short flags;
    Boolean usable;
    Boolean pad123;
} FletRecord;

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern FletRecord *fleetData;
EXTERN_CPP_END
#pragma cplusplus reset

/* chär resource, 0x16a bytes, read in place through charFileHand. */
#pragma options align=mac68k
typedef struct CharRecord {
    int32_t cash;
    short shipType;
    short startSyst[4];
    short govt[4];
    short standing[4];
    short kills;
    short introPICT[4];
    short introDelay[4];
    short introTextID;
    char onStart[0x100];
    unsigned short flags;
    short startDay;
    short startMonth;
    short startYear;
    char prefix[0x10];
    char suffix[0x10];
    short unk15a[8];
} CharRecord;
#pragma options align=reset

/* Resource flag bits, one name per bit, named for the Nova Bible's meaning.
   Table and the uses that read a bit differently: Research/flag-names.md */

/* gövt Flags */
#define kGovtXenophobic         0x0001
#define kGovtEnforcesAbroad     0x0002  /* attacks a criminal player outside its own systems */
#define kGovtAlwaysAttacksPlayer 0x0004
#define kGovtImmuneToPlayer     0x0008  /* the player's shots don't hit its ships */
#define kGovtRetreatsWhenHurt   0x0010  /* warships retreat below 25% shields */
#define kGovtIgnoredByNosy      0x0020  /* nosy non-allied ships ignore its ships under attack */
#define kGovtNeverAttacksPlayer 0x0040
#define kGovtFreighterHalfJam   0x0080  /* freighters get half the warships' jamming */
#define kGovtPersNoEscapePod    0x0100  /* përs ships act as if they used an escape pod */
#define kGovtWarshipsBribable   0x0200
#define kGovtNoComm             0x0400  /* can't hail its ships */
#define kGovtAlwaysDisabled     0x0800  /* its ships start disabled (derelicts) */
#define kGovtPlunders           0x1000
#define kGovtFreightersBribable 0x2000
#define kGovtPlanetsBribable    0x4000
#define kGovtGreedy             0x8000  /* larger bribes; its planets always take one */

/* gövt Flags2 */
#define kGovtNotTalkative       0x0001  /* no request-assistance / beg-for-mercy button */
#define kGovtMinor              0x0002  /* a minor govt on the map's political boundaries */
#define kGovtNoInfluence        0x0004  /* its systems don't affect the boundaries */
#define kGovtSilent             0x0008  /* no distress calls, no greetings */
#define kGovtRoadsideAssistance 0x0010
#define kGovtNoHypergates       0x0020
#define kGovtPrefersHypergates  0x0040
#define kGovtPrefersWormholes   0x0080

/* spöb Flags (SpobRecord.flags is unsigned long) */
#define kSpobCanLand            0x00000001UL
#define kSpobTradeCenter        0x00000002UL  /* commodity exchange */
#define kSpobHasOutfitter       0x00000004UL
#define kSpobHasShipyard        0x00000008UL
#define kSpobIsStation          0x00000010UL
#define kSpobUninhabited        0x00000020UL  /* no traffic control or refuelling */
#define kSpobHasBar             0x00000040UL
#define kSpobLandOnlyWhenDestroyed 0x00000080UL
#define kSpobPriceBits          0x77777700UL  /* the six commodities' price levels */
#define kSpobDemandLow0         0x10000000UL
#define kSpobDemandMedium0      0x20000000UL
#define kSpobDemandHigh0        0x40000000UL
#define kSpobDemandLow1         0x01000000UL
#define kSpobDemandMedium1      0x02000000UL
#define kSpobDemandHigh1        0x04000000UL
#define kSpobDemandLow2         0x00100000UL
#define kSpobDemandMedium2      0x00200000UL
#define kSpobDemandHigh2        0x00400000UL
#define kSpobDemandLow3         0x00010000UL
#define kSpobDemandMedium3      0x00020000UL
#define kSpobDemandHigh3        0x00040000UL
#define kSpobDemandLow4         0x00001000UL
#define kSpobDemandMedium4      0x00002000UL
#define kSpobDemandHigh4        0x00004000UL
#define kSpobDemandLow5         0x00000100UL
#define kSpobDemandMedium5      0x00000200UL
#define kSpobDemandHigh5        0x00000400UL

/* spöb Flags2 */
#define kSpobFrame0Between      0x0001  /* the first frame shows after each later one */
#define kSpobRandomFrames       0x0002
#define kSpobLoopSound          0x0010
#define kSpobAlwaysDominated    0x0020
#define kSpobStartsDestroyed    0x0040
#define kSpobAnimWhenDestroyed  0x0080
#define kSpobDeadly             0x0100  /* ships that touch it are destroyed */
#define kSpobFiresWhenProvoked  0x0200
#define kSpobBuysAnyOutfit      0x0400  /* buys any nonpermanent outfit, whatever the tech level */
#define kSpobHypergate          0x1000
#define kSpobWormhole           0x2000
#define kSpobGateOrWormhole     (kSpobHypergate | kSpobWormhole)

/* shïp Flags */
#define kShipSlowJump           0x0001
#define kShipSemiFastJump       0x0002
#define kShipFastJump           0x0004
#define kShipFuelRegen          0x0008  /* the player's ship uses FuelRegen */
#define kShipArmorDisableTenth  0x0010  /* disabled at 10% armor, not 33% */
#define kShipAfterburnerIfRated 0x0020  /* afterburner once the player's combat rating is high */
#define kShipAlwaysAfterburner  0x0040  /* AI ships only */
#define kShipShowArmorPercent   0x0100  /* "% armor" on the target display, not "Shields Down" */
#define kShipHideStatus         0x0200  /* no armor or shield state on the status display */
#define kShipPlanetType         0x0400  /* hit only by planet-type weapons */
#define kShipTurretBlindFront   0x1000
#define kShipTurretBlindSide    0x2000
#define kShipTurretBlindRear    0x4000
#define kShipEscapeShip         0x8000  /* ejecting flies a carried ship of this type */

/* shïp Flags2 */
#define kShipSwarms             0x0001
#define kShipStandoff           0x0002  /* prefers standoff attacks */
#define kShipUntargetable       0x0004
#define kShipPDTarget           0x0008  /* point defence may fire on it */
#define kShipNoFighterVoice     0x0010
#define kShipJumpsAtSpeed       0x0020  /* jumps without slowing down */
#define kShipInertialess        0x0040
#define kShipAmmoAware          0x0080  /* AI runs away or docks when out of ammo */
#define kShipCloaksToJump       0x0400  /* AI cloaks when hyperspacing */
#define kShipCloaksWhenAttacked 0x4000  /* AI cloaks when preemptively attacked */

/* shïp Flags3 */
#define kShipDestroysAsteroids  0x0001
#define kShipScoopsDebris       0x0002
#define kShipMiner              (kShipDestroysAsteroids | kShipScoopsDebris)
#define kShipIgnoresDeadly      0x0020  /* ignores deadly stellars */
#define kShipTurretShotsAbove   0x0040
#define kShipHideIfUnavailable  0x0100  /* not in the shipyard while Availability is false */
#define kShipHideIfRequireUnmet 0x0200
#define kShipSupersedes         0x4000  /* blocks higher-numbered ships of equal DispWeight */

/* shän Flags (ShipRec.shanFlags) */
#define kShanBanks              0x0001
#define kShanFoldingParts       0x0002
#define kShanKeyCarriedFrames   0x0004  /* second frame set while no KeyCarried ship is aboard */
#define kShanAnimatedBase       0x0008  /* extra base frames shown in sequence */
#define kShanStopAnimDisabled   0x0010
#define kShanHideAltDisabled    0x0020
#define kShanHideLightsDisabled 0x0040
#define kShanUnfoldsWhenFiring  0x0080
#define kShanSkewCorrect        0x0100  /* corrects off-axis rendering by UpCompressY/DnCompressY */

/* wëap Flags */
#define kWeapSpins              0x0001
#define kWeapSecondary          0x0002  /* fired by the second trigger */
#define kWeapCycleFromFirstFrame 0x0004
#define kWeapNoFastTargets      0x0008  /* guided: won't fire at fast ships */
#define kWeapLoopSound          0x0010
#define kWeapPassesShields      0x0020
#define kWeapFireSimultaneously 0x0040
#define kWeapNoPDTarget         0x0080  /* point defence can't target it */
#define kWeapBlastSparesPlayer  0x0100
#define kWeapSmallSmoke         0x0200
#define kWeapBigSmoke           0x0400
#define kWeapPersistentSmoke    0x0800
#define kWeapTurretBlindFront   0x1000
#define kWeapTurretBlindSide    0x2000
#define kWeapTurretBlindRear    0x4000
#define kWeapDetonatesAtEnd     0x8000  /* detonates at the end of its lifespan */

/* wëap Seeker */
#define kWeapSeekPassesAsteroids 0x0001
#define kWeapSeekDecoyedByAsteroids 0x0002
#define kWeapSeekTurnsAwayJammed 0x0010
#define kWeapSeekNoFireIonized  0x0020
#define kWeapSeekLosesLock      0x4000  /* loses lock if the target is not ahead */
#define kWeapSeekAttacksParent  0x8000  /* may attack its parent ship if jammed */

/* wëap Flags2 */
#define kWeapHoldFirstFrame     0x0001  /* cycling: first frame until ProxSafety expires */
#define kWeapStopOnLastFrame    0x0002
#define kWeapProxHitsAnyShip    0x0008
#define kWeapSubsSeekNearest    0x0010
#define kWeapNoSubsOnExpire     0x0020
#define kWeapNoAmmoDisplay      0x0040
#define kWeapNeedsKeyCarried    0x0080
#define kWeapFlagAIRestricted   0x0100  /* AI ships won't use it */
#define kWeapUsesShipSprite     0x0200
#define kWeapPlanetType         0x0400  /* hits only planet-type ships */
#define kWeapHideIfNoAmmo       0x0800  /* not selectable or shown when out of ammo */
#define kWeapNotDestroying      0x1000  /* can disable but not destroy */
#define kWeapBeamUnderShip      0x2000
#define kWeapFireWhileCloaked   0x4000
#define kWeapBigRockDamage      0x8000  /* x10 mass damage to asteroids */

/* wëap Flags3 */
#define kWeapAmmoAtBurstEnd     0x0001  /* uses ammo only at the end of a burst */
#define kWeapTranslucent        0x0002
#define kWeapOneShotAtATime     0x0004
#define kWeapClosestExit        0x0010
#define kWeapExclusive          0x0020

/* mïsn Flags (copied to MissionRecord, MissionState and MissionAvailRec .flags) */
#define kMissionAutoAbort       0x0001
#define kMissionNoDestArrows    0x0002
#define kMissionCantRefuse      0x0004
#define kMissionTakesFuel       0x0008  /* takes 100 units of fuel on success */
#define kMissionInfiniteAuxShips 0x0010
#define kMissionFailIfScanned   0x0020
#define kMissionAbortPenalty    0x0040  /* -5x CompReward on abort */
#define kMissionBriefingArrow   0x0100  /* green arrow on the briefing map */
#define kMissionExtraArrow      0x0200  /* also mark the mission's system */
#define kMissionHidden          0x0400
#define kMissionFixedShipType   0x0800  /* special ships' type picked once at start */
#define kMissionNotForFreighters 0x2000
#define kMissionNotForWarships  0x4000
#define kMissionFailIfBoarded   0x8000  /* by pirates */

/* mïsn Flags2 */
#define kMissionNeedsCargoSpace 0x0001
#define kMissionPaysOnAbort     0x0002
#define kMissionFailIfDisabled  0x0004  /* fails if the player is disabled or destroyed */

/* përs Flags */
#define kPersHoldsGrudge        0x0001
#define kPersEscapePod          0x0002  /* escape pod and afterburner */
#define kPersQuoteIfGrudge      0x0004
#define kPersQuoteIfLikes       0x0008
#define kPersQuoteOnAttack      0x0010
#define kPersQuoteIfDisabled    0x0020
#define kPersSwapsShip          0x0040  /* replaces the LinkMission's single special ship */
#define kPersQuoteOnce          0x0080
#define kPersDeactivateOnAccept 0x0100
#define kPersOfferWhenBoarded   0x0200  /* LinkMission offered on boarding, not hailing */
#define kPersQuoteNeedsMission  0x0400
#define kPersLeavesOnAccept     0x0800
#define kPersNotForWimpyFreighter 0x1000
#define kPersNotForBeefyFreighter 0x2000
#define kPersNotForWarship      0x4000
#define kPersShowsDisasters     0x8000

/* përs Flags2 */
#define kPersStartsNoFuel       0x0001

/* düde Flags */
#define kDudeCargoMask          0x003f  /* the six cargo-class bits */
#define kDudeCarriesMoney       0x0040  /* the amount depends on the ship's price */
#define kDudeUntouchable        0x0100  /* can't hit or be hit by the player */

/* düde InfoTypes (DudeRecord.adviceFlags) */
#define kDudeInfoPrices         0x1000  /* good prices */
#define kDudeInfoDisasters      0x2000
#define kDudeInfoAdvice         0x4000  /* specific advice: STR# 7500 + the low 12 bits */
#define kDudeInfoGovtHail       0x8000  /* generic govt hail messages */
#define kDudeInfoAny            (kDudeInfoPrices | kDudeInfoDisasters | kDudeInfoAdvice | kDudeInfoGovtHail)

/* ränk Flags */
#define kRankGrantDropsSameGovt 0x0001
#define kRankDropsSameGovt      0x0002
#define kRankLostOnAttack       0x0004  /* lost on destroying or disabling the govt's ships */
#define kRankNeverDropped       0x0008  /* permanent */
#define kRankGrantDropsLowerPriority 0x0010
#define kRankDropsLowerPriority 0x0020
#define kRankLostOnCrime        0x0040
#define kRankNotAttacked        0x0100
#define kRankCanAlwaysLand      0x0200
#define kRankCallsAssistance    0x0400
#define kRankFreeRepair         0x0800

/* jünk, flët, crön and chär Flags */
#define kJunkTribbles           0x0001
#define kJunkPerishable         0x0002
#define kFleetFlagRandomCargo   0x0001
#define kCronIterateOnActivate  0x0001
#define kCronIterateOnEnd       0x0002
#define kCharIsDefault          0x0001

/* flag-test wrappers (house style) */
#define SpobCanLand(s)  ((s)->flags & kSpobCanLand)
#define SpobHasBar(s)   ((s)->flags & kSpobHasBar)
#define SpobIsHyperOrWorm(s) ((s)->flags2 & (kSpobHypergate | kSpobWormhole))
#define SpobUninhabited(s) ((s)->flags & kSpobUninhabited)
#define SpobLandOnlyWhenDestroyed(s) ((s)->flags & kSpobLandOnlyWhenDestroyed)

extern Boolean StellarIsDestroyed(SpobRecord *spob);
extern Boolean StellarIsLandable(SpobRecord *spob);
extern Boolean StellarIsNormalLandable(SpobRecord *spob);
extern unsigned char PlayerBlessedByGovt(short govt, short which);

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern SpobRecord *stellarData;
extern SystRecord *systemData;
extern GovtRecord *govtData;
extern ShipRec *shipClass;
extern OutfRecord *outfitSystem;
extern PersRecord *personData;
extern WeapRecord *weaponStats;
extern CronRecord *cronEvent;
EXTERN_CPP_END
#pragma cplusplus reset

#define SpobHasShipyard(s)  ((s)->flags & kSpobHasShipyard)
#define kGameStrList        2002
#define kButtonStrList      150     /* STR# 150: the button labels */
#define kKeyNameStrList     129     /* STR# 129: the key names */
#define kAlertStrList       140
#define kSpecialValueStrList 141   /* STR# 141: the words a preference shows for its special values */
#define kHintStrList        7000
#define kHackedStrList      30000   /* STR# 30000: what the ships say to a hacked copy */
#define kFirstJunkItem     6
#define kFirstResourceID 128
/* an ID field that also carries a class: 128 + 1000 * class + index */
#define kResClassStep       1000
#define ResClassBase(cls)   (kFirstResourceID + (cls) * kResClassStep)
#define InGovtClass(id, cls) ((id) >= ResClassBase(cls) \
                              && (id) < ResClassBase(cls) + kMaxGovts)
#define kGovtClasses    4
#define kMissionDescBase     0x0fa0
/* mïsn ShipGoal (+0x0a, currentMissions[].shipGoal) */
#define kMissionGoalNone        (-1)
#define kMissionGoalDestroy     0
#define kMissionGoalDisable     1
#define kMissionGoalBoard       2
#define kMissionGoalEscort      3
#define kMissionGoalObserve     4
#define kMissionGoalRescue      5
#define kMissionGoalChaseOff    6
#define kMissionModeAttackPlayer 0      /* mïsn ShipBehav: always attack the player */
#define kMissionModeEscort     1
#define kMissionModeDestroyStellar 2    /* mïsn ShipBehav: attack the system's stellars */
#define kModTypeFakeID     21
#define kModTypeMap        16
#define kModeFlagOwned        1
#define kOrdinalsBase        28
#define kOrdinalsSTR        137
#define kOutfModAmmo     3
#define kOutfModCargo      2
#define kOutfModType17  0x11
#define kOutfModWeapon 1
#define kOutfMods           4
#define kStrOf                 0x187   /* STR# 2002: "of" */
#define kStrAnd                0x188   /* STR# 2002: "and" */
#define kPrefsStrList   0x82
#define kRankActive                 0
#define kRankFlags                  0x1c
#define kRankGovt                   6
#define kRankInUse                  1
#define kShipTypeWimpyTrader    1       /* aiType: shïp inherentAI 1..4 */
#define kShipTypeBraveTrader    2
#define kShipTypeWarship        3
#define kShipTypeInterceptor    4
#define kShipTypeFighter        5       /* aiType: launched from a bay */
#define kShipTypeEscort         6       /* aiType: a hired escort */
#define kNavTargetStellar       2       /* navTargetKind: targetStellar is a stellar */
#define kNavTargetLink          3       /* navTargetKind: targetStellar is the destination system */
#define kNavTargetGate          4       /* navTargetKind: targetStellar is a hypergate's destination system */
#define kShipTypeSpecial    0x2ff
#define kWeapAmmoFuelBase   1000
#define kWeapTypeTop            0x100

#define kDemandHigh         4
#define kDemandLow          1

#ifdef __cplusplus
}
#endif

#endif
