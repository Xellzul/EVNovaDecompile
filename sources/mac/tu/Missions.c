/* Missions.c -- the mac 1.1.1 Missions.o: mission.c, missionbits.c, missionbiteffects.c, offermissionfromport.c and listdefdraw.c in win link order. Notes: Research/notes/mission.md */
#include "int32.h"
static void MissionListDefinition(short lMessage, Boolean lSelect, Rect *lRect, int32_t lCell,
                         short lDataOffset, short lDataLen, ListHandle lHandle);
#include "../../engine/mission.c"
#include "../../engine/missionbits.c"
#include "../../engine/missionbiteffects.c"
#include "../../engine/offermissionfromport.c"
#include "../../engine/listdefdraw.c"
