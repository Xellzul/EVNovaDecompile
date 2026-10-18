// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* offermissionfromport.c -- ResetMissionOfferVenue, OfferMissionFromPort; exe 0x00448660..0x00448670. Notes: Research/notes/gameutils.md */

#include <math.h>
#include <stddef.h>
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "burger.h"
#include "crashlog.h"
#include "prototypes.h"

/* ResetMissionOfferVenue (mac _ResetMissionOfferVenue). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448660
void ResetMissionOfferVenue(void)
{
    lastPortMissionLoc = -1;
}

/* OfferMissionFromPort (mac _OfferMissionFromPort). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448670
char OfferMissionFromPort(short stellar)
{
    short i;
    short slot;
    short mission;
    short first;

    if (stellar != lastPortMissionLoc) {
        if (stellar != 3) {
            for (i = 0; i < kMaxMissions; i++)
                suppressMissionOffer[i] = 0;
        }
        lastPortMissionLoc = stellar;
    }
    if ((first = dockedPortMissions[1][0]) < 0 || first >= kMaxMissions)
        return 0;

    slot = -1;
    for (i = 0; i < kMaxMissions; i++) {
        mission = dockedPortMissions[1][i];
        if (mission != -1 && stellar == missionAvailData[mission].availLoc
                && suppressMissionOffer[mission] == 0
                && IsMissionAvailable(mission, 1)) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return 0;

    if (OfferOneMission(dockedPortMissions[1][slot]) == -1) {
        suppressMissionOffer[dockedPortMissions[1][slot]] = 1;
    } else {
        for (i = slot; i < kMaxMissions - 1; i++)
            dockedPortMissions[1][i] = dockedPortMissions[1][i + 1];
    }
    missionOfferTime = ReadTick() + Rand(30) + 30;
    return 1;
}
