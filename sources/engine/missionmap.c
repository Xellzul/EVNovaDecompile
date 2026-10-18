// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* missionmap.c -- rebuild the starmap's mission-marker system list (win TU region around 0x004aa980, after the map-influence code at 0x004aa070). Notes: Research/notes/missionmap.md */
#include "gametypes.h"
#include "resources.h"
#include "gameconst.h"
#include "prototypes.h"
#include "stellar.h"

#define kMissionSystMax    32

#define kMissionSkipMapMark 0x0002 /* flags55; suppresses both marks below (name TBC) */
#define kMissionSystemMark  0x0200 /* flags55; also mark the mission's system field (name TBC) */



/* RecalcMissionSystsForMap. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aa980
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
        if (result != -1 && (currentMissions[i].flags & kMissionSkipMapMark) == 0) {
            missionSyst[count++] = result;
        }

        if ((currentMissions[i].flags & kMissionSystemMark) != 0 &&
            (currentMissions[i].flags & kMissionSkipMapMark) == 0 &&
            currentMissions[i].shipCount > 0 &&
            currentMissions[i].shipSyst >= 0) {
            sys = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            if (sys != -1)
                missionSyst[count++] = sys;
        }
    }
}
