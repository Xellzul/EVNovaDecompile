// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* missionmap.c -- rebuild the starmap's mission-marker system list (win TU region around 0x004aa980, after the map-influence code at 0x004aa070). Notes: Research/notes/missionmap.md */
#include "gametypes.h"
#include "resources.h"
#include "gameconst.h"
#include "prototypes.h"
#include "stellar.h"

#define kMissionSystMax    32

/* RecalcMissionSystsForMap. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aa980
// MAC: _RecalcMissionSystsForMap i386 0x0000e1b5 ppc 0x00011924
void RecalcMissionSystsForMap(void)
{
    short i;
    short count;
    short result;
    short sys;

    for (i = 0; i < kMissionSystMax; i++)
        missionSyst[i] = -1;

    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0)
            continue;

        result = -1;
        if (currentMissions[i].travelStellar >= 0 &&
            stellarData[currentMissions[i].travelStellar].loaded != 0) {
            result = stellarData[currentMissions[i].travelStellar].system;
        }
        if (currentMissions[i].returnStellar >= 0 &&
            currentMissions[i].travelStellar != currentMissions[i].returnStellar &&
            currentMissionObjectives[i].returnDone != 0 &&
            stellarData[currentMissions[i].returnStellar].loaded != 0) {
            result = stellarData[currentMissions[i].returnStellar].system;
        }
        if (result != -1 && (currentMissions[i].flags & kMissionNoDestArrows) == 0) {
            missionSyst[count] = result;
            count++;
        }

        if ((currentMissions[i].flags & kMissionExtraArrow) != 0 &&
            (currentMissions[i].flags & kMissionNoDestArrows) == 0 &&
            currentMissions[i].shipCount > 0 &&
            currentMissions[i].shipSyst >= 0) {
            sys = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            if (sys != -1) {
                missionSyst[count] = sys;
                count++;
            }
        }
    }
}
