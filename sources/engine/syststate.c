// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* syststate.c -- two small pieces of the system-entry state machine (win 0x0045cfc0 and 0x0045d030, from the run below 0x0045d0a0). Notes: Research/notes/syststate.md */
#include <stddef.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "player.h"
#include "render.h"
#include "gameutil.h"



/* InitStatus. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045cfc0
void InitStatus(void)
{
    statusBrightColor = &statusColor[0];
    statusDimColor = &statusColor[1];
    radarStatusColor[1] = &statusColor[2];
    radarStatusColor[0] = &statusColor[3];
    shieldStatusColor = &statusColor[4];
    armorStatusColor = &statusColor[5];
    fuelStatusColor[0] = &statusColor[6];
    fuelStatusColor[1] = &statusColor[7];
    lastRadarUpdateTime = 0;
    radarCount = 0;
}

/* HandleStatus. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045d030
void HandleStatus(void)
{
    short chance;

    chance = gSystems[shipData->curSystem].interference - ShipAntiInterference();
    if (chance > 100)
        chance = 100;
    if (chance < 0)
        chance = 0;
    if (Rand(100) + 1 <= chance)
        interferenceThisFrame = 1;
    else
        interferenceThisFrame = 0;
}
