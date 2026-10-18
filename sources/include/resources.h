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
    long cost;
    unsigned short scanMask;
    unsigned char name[64];
    char availability[255];
    char onPurchase[255];
    char onSell[255];
    char unk367;
    long contributes[2];
    long requires[2];
    char persistent;
    char unk379[3];
} OutfRecord;

struct ShipRec;
struct ShipObj;

extern long AdjustedItemCost(OutfRecord *outf, struct ShipRec *ship);
extern long AdjustedItemMass(OutfRecord *outf, struct ShipRec *ship);
extern void GetShipPaintColor(struct ShipObj *ship, unsigned short *red,
                         unsigned short *green, unsigned short *blue);
extern void MungeBriefing(char briefing, short which);
extern unsigned char LaunchFighter(struct ShipObj *parent, short fighterType);
extern void SpawnSmoke(float x, float y, long type, short sprite);

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
    char unk36[2];
    float turnRate;
    float accel;
    float maxSpeed;
    float mobilityFactor;
    float deionize;
    float weapDecay;
    float shieldRech;
    float armorRech;
    long cost;
    long shield;
    long armor;
    long escUpgradeCost;
    long tradeIn;
    unsigned char name[64];
    unsigned char subtitle[64];
    char availability[255];
    char onPurchase[255];
    char appearOn[255];
    char onCapture[255];
    char onRetire[255];
    char unk5E7;
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
    unsigned short blinkMode;
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
    char unkA2E[2];
    long contributes[2];
    long requires[2];
    char cargoNotFixed;
    char canAppear;
    short weapPos[4][4][2];
    short weapPosZ[4][4];
    char unkAA2[2];
    float compress[2][2];
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

#define kResAbsent          (-9999)
#define kResUnloaded        9999

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
    char active;
    char loaded;
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
    char unk3E[2];
    long contributes[2];
    long requires[2];
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
    long credits;
    float shieldMod;
    char loaded;
    char grudge;
    char canAppear;
    char prepared;
    unsigned char name[31];
    char unk643;
    char activeOn[255];
    char subtitle[65];
    short grantClass;
    short grantProb;
    short grantCount;
    short sameNameIndex;
    unsigned short rgb5[3];
    char unk792[2];
} PersRecord;

#define kGovtXenophobic 0x1
#define kShipArmorDisableTenth 0x10
#define kGovtAlwaysDisabled 0x800
#define kPersCaptainHector 1023 /* the hand-built shareware enforcer */
#define kPersBountyHunter  1022

typedef struct SystRecord {
    short xPos;
    short yPos;
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
    char unkC6[2];
    float reinforceTimer;
    unsigned char name[32];
    char visibility[255];
    char loaded;
    char known;
    char prepared;
    RGBColor rgb;
    unsigned short astTypes;
    char unk1F6[2];
    unsigned long mapPixel;
} SystRecord;

typedef struct NebuRecord {
    short xPos;
    short yPos;
    short width;
    short height;
    char loaded;
    char explored;
    char activeOn[255];
    char onExplore[255];
} NebuRecord;

#pragma cplusplus on
extern "C++" {
extern NebuRecord *nebulaData;
}
#pragma cplusplus reset

typedef struct DudeRecord {
    short aiType;
    short govt;
    short shipTypes[16];
    short shipProbs[16];
    unsigned short flags;
    unsigned short adviceFlags;
    char loaded;
    char unk49;
} DudeRecord;

#pragma cplusplus on
extern "C++" {
extern DudeRecord *dudeData;
}
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
extern "C++" {
extern JunkRecord *junkData;
extern BoomRecord *explodType;
}
#pragma cplusplus reset

/* rank runtime record, stride 0x120 in the rankData array. */
typedef struct RankRecord {
    char active;
    char loaded;
    short index;
    short priority;
    short govt;
    short priceMod;
    char unkA[2];
    long salary;
    long salaryCap;
    long contributes[2];
    unsigned short flags;
    char name[128];
    char convName[64];
    char shortName[66];
} RankRecord;

#pragma cplusplus on
extern "C++" {
extern RankRecord *rankData;
}
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
    char unk20D;
    char loaded;
    char unk20F;
} OopsRecord;

#pragma cplusplus on
extern "C++" {
extern OopsRecord *disasterData;
}
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
    char unk11F;
    unsigned short flags;
    char usable;
    char unk123;
} FletRecord;

#pragma cplusplus on
extern "C++" {
extern FletRecord *fleetData;
}
#pragma cplusplus reset

/* chär resource, 0x16a bytes, read in place through charFileHand. */
typedef struct CharRecord {
    long cash;
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
    char unk15a[0x10];
} CharRecord;

#define kSpobCanLand   0x00000001UL
#define kSpobHasBar    0x00000040UL
#define kSpobHypergate 0x1000          /* flags2 */
#define kSpobWormhole  0x2000          /* flags2 */

/* flag-test wrappers (house style) */
#define SpobCanLand(s)  ((s)->flags & kSpobCanLand)
#define SpobHasBar(s)   ((s)->flags & kSpobHasBar)
#define SpobIsHyperOrWorm(s) ((s)->flags2 & (kSpobHypergate | kSpobWormhole))
#define kSpobUninhabited 0x00000020UL
#define SpobUninhabited(s) ((s)->flags & kSpobUninhabited)
#define kSpobLandOnlyWhenDestroyed 0x00000080UL
#define SpobLandOnlyWhenDestroyed(s) ((s)->flags & kSpobLandOnlyWhenDestroyed)

extern char StellarIsDestroyed(SpobRecord *spob);
extern char StellarIsLandable(SpobRecord *spob);
extern char StellarIsNormalLandable(SpobRecord *spob);
extern char PlayerBlessedByGovt(short govt, short which);

#pragma cplusplus on
extern "C++" {
extern SpobRecord *stellarData;
extern SystRecord *gSystems;
extern GovtRecord *govtData;
extern ShipRec *gShipTypes;
extern OutfRecord *gOutfits;
extern PersRecord *personData;
extern WeapRecord *gWeapons;
extern CronRecord *gCrons;
}
#pragma cplusplus reset

#define SpobHasShipyard(s)  ((s)->flags & kSpobHasShipyard)
#define kGameStrList        2002
#define kButtonStrList      150     /* STR# 150: the button labels */
#define kFirstJunkItem     6
#define kFirstResourceID 128
/* an ID field that also carries a class: 128 + 1000 * class + index */
#define kResClassStep       1000
#define ResClassBase(cls)   (kFirstResourceID + (cls) * kResClassStep)
#define InGovtClass(id, cls) ((id) >= ResClassBase(cls) \
                              && (id) < ResClassBase(cls) + kMaxGovts)
#define kGovtAlwaysAttacksPlayer   0x0004
#define kGovtClasses    4
#define kMissionAvailHasDest      0x0100
#define kMissionDescBase     0x0fa0
#define kMissionFlagRearmShips  0x10    /* loaddata.c's name */
#define kMissionHidden 0x0400
#define kMissionHideFromList      0x0400
#define kMissionKindDisable     3       /* mïsn +0x0a: disable the ship */
#define kMissionModeEscort     1
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
#define kPersNeedsFuel           1     /* PersRecord flags2: starts with no fuel */
#define kStrOf                 0x187   /* STR# 2002: "of" */
#define kStrAnd                0x188   /* STR# 2002: "and" */
#define kPrefsStrList   0x82
#define kRankActive                 0
#define kRankFlags                  0x1c
#define kRankGovt                   6
#define kRankInUse                  1
#define kRankNeverDropped            0x0008  /* immune to the sweeps below */
#define kShipAltAnim            0x0008
#define kShipAltWhenDisabled    0x0010  /* +0xa24: show it while disabled */
#define kShipAmmoAware      0x80    /* ai.c's name */
#define kShipAngleTable         0x0100  /* +0xa24: frameAngles is populated */
#define kShipBayAnim            0x0004
#define kShipFlag2NoTarget     4
#define kShipFlag2Untargetable 4
#define kShipFlagHalfArmor  0x0010      /* ShipRec flags +0x9e8: recovers 0.1 */
#define kShipFlagNoMove         0x400
#define kShipFlagNoVoice        0x10
#define kShipFlags3Trader       0x0002
#define kShipKeepFireGroup 0x80
#define kShipLightsOffDisabled  0x0040
#define kShipNoAltDisabled      0x0020
#define kShipRecRollable    0x0001
#define kShipTypeA24Bit1        0x0002
#define kShipTypeEscort         6       /* aiType: a hired escort */
#define kShipTypeSpecial    0x2ff
#define kSpobAmbientLoop    0x0010      /* flags2 (tradedlg.c's name) */
#define kSpobFlag2Bit5        0x20
#define kSpobFlagMissions 0x40
#define kSpobFlagNoShips 0x20
#define kSpobFlagOutfit     2
#define kSpobFlagShipyard   4
#define kSpobFlagTrade      8
#define kSpobHasShipyard   0x00000008UL  /* spob flags (Nova Bible); name TBC */
#define kSpobTradeCenter 0x00000002UL
#define kWeapAmmoFlag           0x1
#define kWeapAmmoFuelBase   1000
#define kWeapArcMatch           0x400
#define kWeapBigRockDamage      0x8000
#define kWeapFireWhileCloaked   0x4000
#define kWeapTranslucent       0x2
#define kWeapClosestExit  0x10
#define kWeapOneShotAtATime  0x4
#define kWeapExclusive         0x20
#define kWeapFlagAIRestricted   0x100
#define kWeapFlagAIUsable       0x0400
#define kWeapFlagHitsOwner      0x100   /* WeapRecord flags bit 8 (name TBC) */
#define kWeapFlagIonizing       0x20
#define kWeapFlagTurret     0x0002
#define kWeapSeekPassesAsteroids      1
#define kWeapNotDestroying  0x1000     /* WeapRecord flags2 bit 12: can disable but not destroy */
#define kWeapTypeTop            0x100
#define kWeapSeekNoFireIonized    0x0020

#define kDemandHigh         4
#define kDemandLow          1
#define kShipCarriesMission 0x3
#define kShipEscortBonus    0x10  /* the 1.333x disable threshold */
#define kShipInvincible     0x100
#define kShipNoGovtString   0x200 /* suppress the government line */
#define kSpobDemandHigh0    0x40000000UL
#define kSpobDemandHigh1    0x04000000UL
#define kSpobDemandHigh2    0x00400000UL
#define kSpobDemandHigh3    0x00040000UL
#define kSpobDemandHigh4    0x00004000UL
#define kSpobDemandHigh5    0x00000400UL
#define kSpobDemandLow0     0x10000000UL
#define kSpobDemandLow1     0x01000000UL
#define kSpobDemandLow2     0x00100000UL
#define kSpobDemandLow3     0x00010000UL
#define kSpobDemandLow4     0x00001000UL
#define kSpobDemandLow5     0x00000100UL
#define kSpobNoNav          0x3000
#define kWeapNoAmmoDisplay  0x40

#define kSpobHidden         0x00000020UL

#define kGovtPirateWarships 0x1000
#define kWeapPlanetType        0x400
#define kSpobDestroyed      0x00000010UL

#ifdef __cplusplus
}
#endif

#endif
