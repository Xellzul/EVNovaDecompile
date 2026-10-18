/* pilotfile.h -- the pilot save file's on-disk records. Notes: Research/notes/pilotfile.md */
#include "gametypes.h"

#ifndef PILOTFILE_H
#define PILOTFILE_H

#pragma options align=packed

/* one entry of the mission table, stride 0x8e6 (currentMissions' element) */
typedef struct MissionRec {
    short travelStellar;
    short unk2;
    short returnStellar;
    short shipCount;
    short shipDude;
    short shipGoal;
    short shipBehav;
    short shipStart;
    short shipSyst;
    short cargoType;
    short cargoQty;
    short pickupMode;
    short dropoffMode;
    short scanMask;
    short compGovt;
    short compReward;
    short datePostIncrement;
    long payment;
    short specialShipsKilled;
    short specialShipsBoarded;
    short specialShipsDisabled;
    short specialShipsJumpedIn;
    short specialShipsJumpedOut;
    short initialShipCount;
    char canAbort;
    char hasCargo;
    char unk34;
    short briefDesc;
    short quickBriefDesc;
    short loadCargoDesc;
    short dropCargoDesc;
    short compDesc;
    short failDesc;
    short refuseDesc;
    short shipDoneDesc;
    short timeLimit;           /* +0x045 fed to DateAfterDays */
    short destStrList;          /* +0x047 STR# id of the destination name */
    short destStrIndex;         /* +0x049 */
    short shipDelay;
    short misnIndex;
    short shipStrList;          /* +0x04f STR# id of the mission ship name */
    short shipStrIndex;         /* +0x051 */
    short shipTypeIndex;
    unsigned short flags;       /* +0x055 */
    short flags2;
    struct { long a; long b; } unk59;
    short auxShipCount;            /* +0x061 */
    short auxShipDude;
    short auxShipSyst;
    short auxShipsSpawned;
    short auxShipDelay;
    short auxShipsLeft;            /* +0x06b */
    MStr64 shipName;            /* +0x06d rebuilt from destStrList/Index */
    MStr64 shipSubtitle;        /* +0x0ad rebuilt from shipStrList/Index */
    MStr255 unkED;              /* +0x0ed */
    MStr255 beginSetString;     /* +0x1ec */
    MStr255 setString2;         /* +0x2eb */
    MStr255 succSetString;      /* +0x3ea */
    MStr255 failSetString;      /* +0x4e9 */
    MStr255 abortSetString;     /* +0x5e8 */
    MStr255 shipDoneSetString;  /* +0x6e7 */
    MStr256 name;               /* +0x7e6 */
} MissionRec;

/* first resource: the player record, 0xe952 bytes */
typedef struct PilotRec {
    short stellar;              /* +0x00000 spob the player is parked at */
    short shipType;             /* +0x00002 */
    short cargo[6];             /* +0x00004 */
    short shield;               /* +0x00010 */
    short fuel;                 /* +0x00012 */
    short month;                /* +0x00014 */
    short day;                  /* +0x00016 */
    short year;                 /* +0x00018 */
    short explore[0x800];       /* +0x0001a */
    short outfits[0x200];       /* +0x0101a */
    short legalStatus[0x800];   /* +0x0141a */
    short weapCount[0x100];     /* +0x0241a */
    short weapAmmo[0x100];      /* +0x0261a */
    long credits;               /* +0x0281a */
    MissionState objectives[16];   /* +0x0281e */
    MissionRec missions[16];        /* +0x0295e */
    char missionBits[0x2710];   /* +0x0b7be */
    char spobFlag[0x800];       /* +0x0dece */
    short escortType[0x40];     /* +0x0e6ce */
    short fighterType[0x40];    /* +0x0e74e */
    short escortA[0x40];        /* +0x0e7ce */
    short escortB[0x40];        /* +0x0e84e */
    short fighterVoice[0x40];   /* +0x0e8ce */
    long kills;                 /* +0x0e94e */
} PilotRec;

/* second resource: the galaxy record, 0x5efe bytes */
typedef struct PilotRec2 {
    short version;              /* +0x0000 */
    short strict;               /* +0x0002 */
    short gender;               /* +0x0004 */
    short defWave[0x800];       /* +0x0006 */
    short persActive[0x400];    /* +0x1006 */
    short persB[0x400];         /* +0x1806 */
    short unk2006[0x40];        /* +0x2006 */
    short tribute[0x800];       /* +0x2086 */
    char seenIntro;             /* +0x3086 */
    char unk3087;               /* +0x3087 */
    short newsC[0x100];         /* +0x3088 */
    short newsB[0x100];         /* +0x3288 */
    short junkHeld[0x80];       /* +0x3488 */
    short globalFlux[2];        /* +0x3588 */
    short localFlux[2];         /* +0x358c */
    short cronA[0x200];         /* +0x3590 */
    short cronB[0x200];         /* +0x3990 */
    short systState[0x800];     /* +0x3d90 */
    short spobDead[0x800];      /* +0x4d90 */
    short escortOrders[4];      /* +0x5d90 */
    char nickname[0x40];        /* +0x5d98 */
    RGBColor paint;             /* +0x5dd8 */
    short rankOwned[0x80];      /* +0x5dde */
    char datePrefix[0x10];      /* +0x5ede */
    char dateSuffix[0x10];      /* +0x5eee */
    short unk5efe[0x400];       /* +0x5efe written zeroed, never read */
} PilotRec2;


#pragma options align=reset

#endif
