/* shipobj.h -- the runtime ship object (shipData, stride 0xc948) and the shot record. Notes: Research/notes/shipobj.md */
#include "gametypes.h"

#ifndef SHIPOBJ_H
#define SHIPOBJ_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShipObj {
    char *sprite;
    char *glowSprite;
    char *lightSprite;
    char *weapSprite;
    char *altSprite;
    char *shieldSprite;
    FloatPoint pos;
    FloatPoint vel;
    float formPosX;
    float formPosY;
    float thrust;
    float speedLimit;
    float fuel;
    float deathTimer;
    float skill;
    float heading;
    float accelMag;
    float aiTimer;
    float jumpTimer;
    float shield;
    float armor;
    float ionization;
    float lightLevel;
    float cloak;
    short desiredHeading;
    short navTargetKind;
    short targetStellar;
    char unk6e[2];
    short targetShip;
    short selectedWeapon;
    short curSystem;
    short type;
    short dude;
    short cargo[6];
    short index;
    short aiType;
    short auxMission;
    short defenseStellar;
    short veerHeading;
    short lastInspected;
    short lastStellar;
    short prevSystem;
    short grudge;
    short govt;
    short parentShip;
    char unk9C[4];
    long credits;
    unsigned long jumpTick;
    unsigned long fireTick;
    unsigned long hailTick;
    long ionColor;
    unsigned long tractorTick;
    char active;
    char boarded;
    char fireQueued;
    char hired;
    char hailed;
    char hasAfterburner;
    char sellOnLanding;
    char upgradeOnLanding;
    char hasEscorts;
    char hasSwarm;
    char hasFormation;
    char canScoop;
    char escortMayAttack;
    char unkC5[3];
    WeapSlot weap[256];
    short aiState;
    short aiSubstate;
    short aggression;
    short incomingDamage;
    short persType;
    short specialMission;
    short engineGlow;
    short blinkCount;
    short cloakDir;
    short aimWeapon;
    short tractorShip;
    short formerEscort;
    float animTimer;
    float frameTimer;
    float weapGlow;
    float lightTimer;
    float shieldGlow;
    short altSet;
    short animSet;
    short bankDir;
    short foldDir;
    short foldSet;
    short weapGroupLane[4];
    short swarmLeader;
    short escapePods;
    short escortOrder;
    float searchTimer;
    float annoyance;
    float odds;
    float fuelRegenRate;
    short seesCloaked;
    short seesCloakedRadar;
    short decloaksWhenHit;
    short voiceClass;
    short fuelRegenState;
    short jamming[4];
    short formLeader;
    char *unkC930;
    long unkC934;
    unsigned long unkC938;
    unsigned long unkC93C;
    long unkC940;
    long unkC944;
} ShipObj;

typedef struct ShotObj {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    FloatPoint             vel;
    float                  life;
    float                  trailTimer;
    float                  frameTimer;
    float                  angle;
    float                  murk;
    short                  weapType;
    short                  target;
    short                  owner;
    short                  curSystem;
    short                  guidanceState;
    short                  trailCount;
    short                  frame;
    short                  subDepth;
    short                  glow[4];
    short                  energy;
    short                  hitRoll;
    unsigned char          disableOnly;
    char pad45[3];
} ShotObj;

typedef struct ShipSprites {
    struct SpriteRec *sprite;
    struct SpriteRec *glowSprite;
    struct SpriteRec *lightSprite;
    struct SpriteRec *weapSprite;
    struct SpriteRec *altSprite;
    struct SpriteRec *shieldSprite;
} ShipSprites;

#pragma cplusplus on
extern "C++" {
extern ShipObj *shipData;
extern short shipSystems[512];
}
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
