/* prototypes.h -- the four names that cannot yet have a subsystem home. Notes: Research/notes/prototypes.md */
#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void AppendIndString(char *dst, short strListID, short index);   /* 0x004cd1a0 mac _StrcatIndCString */
extern void RedrawComm(unsigned char noBlit);        /* 0x0047e430 comm.c */
#pragma cplusplus on
extern "C++" {
extern MissionRecord *currentMissions;               /* 0x005914b4 (mac _currentMissions), mission.c */
extern BoxObj *boxData;                              /* 0x005914a8 */
}
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif /* PROTOTYPES_H */
