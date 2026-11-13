/* pilotfile.h -- the pilot save file's on-disk records. Notes: Research/notes/pilotfile.md */
#include "gametypes.h"

#ifndef PILOTFILE_H
#define PILOTFILE_H

#ifdef MACBUILD
#pragma options align=mac68k
#else
#pragma options align=packed
#endif

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
    int32_t credits;               /* +0x0281a */
    MissionState objectives[16];   /* +0x0281e */
    MissionRecord missions[16];        /* +0x0295e */
    char missionBits[0x2710];   /* +0x0b7be */
    Boolean spobFlag[0x800];       /* +0x0dece */
    short escortType[0x40];     /* +0x0e6ce */
    short fighterType[0x40];    /* +0x0e74e */
    short escortA[0x40];        /* +0x0e7ce */
    short escortB[0x40];        /* +0x0e84e */
    short fighterVoice[0x40];   /* +0x0e8ce */
    int32_t kills;                 /* +0x0e94e */
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
    Boolean seenIntro;             /* +0x3086 */
    Boolean unk3087;               /* +0x3087 */
    short newsC[0x100];         /* +0x3088 */
    short newsB[0x100];         /* +0x3288 */
    short junkHeld[0x80];       /* +0x3488 */
    short priceFlux[2][2];      /* +0x3588 global, +0x358c local */
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
