/*
 * missions.h -- missions, crons, mission bits and the comm traffic they drive.
 *
 * One declaration per name for the whole tree.
 * Addresses are the win 1.0.10 exe's.
 */
#ifndef MISSIONS_H
#define MISSIONS_H
#include "burger.h"
#include "macport.h"
#include "resources.h"
#include "shipobj.h"
#include "gametypes.h"

#ifdef __cplusplus
extern "C" {
#endif


/* --- one declaration per name --- */
extern char CronDateIsValid(CronRecord *cron);           /* 0x0046c800 */
extern void CronEventHandler(void);                         /* 0x00439500 */
extern void DisasterHandler(void);                         /* 0x00424f90 */
extern char DoMissionAccept(short mission);                    /* 0x0043f100 */
extern void DoMissionFailure(short slot);                   /* 0x00440930 */
extern void DoMissionSuccess(short slot);                 /* 0x00440410 */
extern void DrawHighGoalAbbreviation(short state);       /* 0x0040db70 */
extern void DrawLowGoalAbbreviation(short state);        /* 0x0040ddc0 */
extern void EvalCurrentMissionBitSetString(char *expr, short mission); /* 0x00448050 */
extern void EvalSetExp(void);     /* 0x00449370 */
extern short EvalTestExp(short *err);  /* 0x00449020 */
extern char DoMissionCargoPickup(short cargo, short needed);        /* 0x00440370 cargo room */
extern void ApplyMissionPay(long amount);  /* 0x00440750 */
extern void HandleComm(void);                  /* 0x0042f1b0 (gameloop.c; mac _HandleComm) */
extern void LoadCurrentMissionData(short mission, short slot);   /* 0x0043f8c0 */
extern void MissionHandler(void);                /* 0x00443760 */
void MissionLandCargoCheck(short slot, short stel);
extern void MissionLandCheck(void);                      /* 0x00443780 */
extern void MissionObjectivesCheck(short slot);           /* 0x00443c60 */
extern void MissionPostLoadValidCheck(void);     /* 0x0043c3e0 */
extern short OfferOneMission(short mission);                  /* 0x00442510 */
extern char PlayerMeetsRequirements(unsigned long reqLo, unsigned long reqHi); /* 0x0046cd80 */
extern void QuickMissionFailure(short mission);                  /* 0x00440bf0 */
extern char *RT3_GetDisplayName(void);                       /* 0x004d45a0 */
extern void RandomizeMissionData(void);                         /* 0x0043cf00 */
extern void RandomizeOneMission(short mission);          /* 0x0043d240 mac _RandomizeOneMission */
extern void RestoreBuffer(void *saved);                 /* 0x0044a9d0 */
extern short SelectMissionStellar(short sel, short loc, short avoid, short mission); /* 0x0043d510 */
extern short SelectMissionSystem(short which, short curSystem);  /* 0x0043e6f0 */
extern void SetupMissionList(void);                            /* 0x0043cc20 */
#pragma cplusplus on
extern "C++" {
extern char commMsg[2048];                      /* 0x0073130c */
extern short currentMissionIndexForSetParser; /* 0x00776b00 (mac name, 2 uses both sides) */
extern short lastInteger;               /* 0x007caa10 mac _lastInteger */
extern Handle missionFileHand;                  /* 0x0077efb8 (mac _missionFileHand) */
extern unsigned char missionName[][256];        /* 0x007356e8 (mac _missionName) */
extern char offeringFromShip;                    /* 0x00773eea (mac name) */
extern short offeringShipIndex;                    /* 0x0077430e (mac _offeringShipIndex) */
extern char playerKilledByHector;                /* 0x007354aa */
}
#pragma cplusplus reset


/* --- one declaration per name --- */
extern char IsMissionAvailable(short mission, char offer); /* 0x00441b40 */
#pragma cplusplus on
extern "C++" {
extern MissionAvailRec missionAvailData[];              /* 0x0077f62c (corroborated, matched-globals.csv) */
extern short activeMissions[16];
extern ScrollRec **missionInfoList;
extern short tempMissions[];
}
#pragma cplusplus reset

/* the two mission-list ModalDialog filter procs; RunModalDialog takes the address
   as void *, so nothing at a call site checks these */
extern char MissionFilter(long unused, EventRecord *ev, short *itemHit);      /* 0x00440c90 */
extern char SingleMissionFilter(long unused, EventRecord *ev,
                                short *itemHit);                              /* 0x00447170 */

#ifdef __cplusplus
}
#endif

#endif /* MISSIONS_H */
