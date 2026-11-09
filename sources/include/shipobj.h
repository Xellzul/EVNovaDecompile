/* shipobj.h -- the runtime ship object (shipData, stride 0xc948 on win, 0xc940 on mac) and the shot record. Notes: Research/notes/shipobj.md */
#include "gametypes.h"

#ifndef SHIPOBJ_H
#define SHIPOBJ_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShipObj {
    struct SpriteRec *sprite;
    struct SpriteRec *glowSprite;
    struct SpriteRec *lightSprite;
    struct SpriteRec *weapSprite;
    struct SpriteRec *altSprite;
    struct SpriteRec *shieldSprite;
    FloatPoint pos;
    FloatPoint vel;
    FloatPoint formPos;
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
    short unk6e;
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
    short unk9C;
    short unk9E;
    long credits;
    unsigned long jumpTick;
    unsigned long fireTick;
    unsigned long hailTick;
    unsigned long ionColor;
    unsigned long tractorTick;
    Boolean active;
    Boolean boarded;
    Boolean fireQueued;
    Boolean hired;
    Boolean hailed;
    Boolean hasAfterburner;
    Boolean sellOnLanding;
    Boolean upgradeOnLanding;
    Boolean hasEscorts;
    Boolean hasSwarm;
    Boolean hasFormation;
    Boolean canScoop;
    Boolean escortMayAttack;
    char padC5[3];
    WeapRecord weap[256];
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
    short seesCloaked[2];
    short decloaksWhenHit;
    short voiceClass;
    short fuelRegenState;
    short jamming[4];
    short formLeader;
    unsigned char *licenseName;
    long long licenseCode;
    unsigned long licenseCopies;
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

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern ShipObj *shipData;
extern short shipSystems[512];
EXTERN_CPP_END
#pragma cplusplus reset

/* a step of the licence checks */
#ifdef MAC1010
#define LICENSE_STEP_BEGIN do {
#define LICENSE_STEP_END   } while (0);
#else
#define LICENSE_STEP_BEGIN
#define LICENSE_STEP_END
#endif

#ifdef __cplusplus
}
#endif

#endif
