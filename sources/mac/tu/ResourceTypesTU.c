/* ResourceTypesTU.c -- the mac 1.1.1 ResourceTypes.o (mac only). Notes: Research/notes/resourcetypes.md */
#include "../port/resourcetypes.c"
#include "macport.h"
#include "gameconst.h"
#include "gametypes.h"
#include "missions.h"
BriefMissionRec briefMissionInfo[1000];
unsigned char bugData[64];
Handle bugHandle;
MissionAvailRec missionAvailData[kMaxMissions];
unsigned char missionFileData[1972];
Handle missionFileHand;
unsigned char oldMissionFileData[1644];
Handle oldMissionFileHand;
