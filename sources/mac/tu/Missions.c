/* Missions.c -- the mac 1.1.1 Missions.o: mission.c, missionbits.c, missionbiteffects.c, offermissionfromport.c and listdefdraw.c in win link order. Notes: Research/notes/mission.md */
/* defined in Missions.o on mac (initialized: __data, read [addr] directly; Buttons.o reaches it by
   non-lazy pointer); on win defined in listdefdraw.c */
DialogPtr missionDialog = NULL;
static void MissionListDefinition(short lMessage, Boolean lSelect, Rect *lRect, long lCell,
                         short lDataOffset, short lDataLen, ListHandle lHandle);
#include "../../engine/mission.c"
#include "../../engine/missionbits.c"
#include "../../engine/missionbiteffects.c"
#include "../../engine/offermissionfromport.c"
#include "../../engine/listdefdraw.c"
