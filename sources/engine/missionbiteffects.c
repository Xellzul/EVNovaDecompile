// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* missionbiteffects.c -- PropagateMissionBitEffects (win 0x00448090-0x0044864c). Notes: Research/notes/missionbiteffects.md */
#include "space.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "game.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "missions.h"

/* PropagateMissionBitEffects (mac _PropagateMissionBitEffects). STATE: MATCH 100.0%, exe 345 /
   obj 345, byte-identical yes.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00448090
// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
void PropagateMissionBitEffects(void)
{
    short i;
    short j;
    short k;
    short navDest;
    char seen[kMaxSystems];

    for (i = 0; i < kMaxStellars; i++)
        stellarData[i].system = -1;

    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].prepared)
            gSystems[i].loaded = EvalMissionBitTestString(gSystems[i].visibility);
        else
            gSystems[i].loaded = 0;
    }

    for (i = 0; i < kMaxSystems; i++)
        seen[i] = 0;

    for (i = 0; i < kMaxSystems; i++) {
        if (!gSystems[i].prepared)
            continue;
        if (!gSystems[i].loaded)
            continue;
        if (seen[i])
            continue;
        for (k = 0; k < kNavSlots; k++) {
            navDest = gSystems[i].navDefault[k];
            if (navDest != -1 && stellarData[navDest].system == -1)
                stellarData[navDest].system = i;
        }
        k = gSystems[i].colocNext;
        seen[i] = 1;
        while (k != -1) {
            seen[k] = 1;
            k = gSystems[k].colocNext;
        }
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        if (gShipTypes[i].techLevel != kResAbsent)
            gShipTypes[i].canAppear = EvalMissionBitTestString(gShipTypes[i].appearOn);
        else
            gShipTypes[i].canAppear = 0;
    }

    for (i = 0; i < kMaxPersons; i++) {
        if (personData[i].prepared)
            personData[i].canAppear = EvalMissionBitTestString(personData[i].activeOn);
        else
            personData[i].canAppear = 0;
    }

    for (i = 0; i < kMaxFleets; i++) {
        if (fleetData[i].leadShipType != -1)
            fleetData[i].usable = EvalMissionBitTestString(fleetData[i].activateOn);
        else
            fleetData[i].usable = 0;
    }

    for (i = 0; i < kMaxMissions; i++) {
        if (missionAvailData[i].availStel == (short)0x8300)
            missionAvailData[i].avail = 0;
        else
            missionAvailData[i].avail = EvalMissionBitTestString(missionAvailData[i].availBits);
    }

    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].width > 0 && nebulaData[i].height > 0)
            nebulaData[i].loaded = EvalMissionBitTestString(nebulaData[i].activeOn);
        else
            nebulaData[i].loaded = 0;
    }

    if (shipData[0].curSystem >= 0 && shipData[0].curSystem < kMaxSystems &&
        !gSystems[shipData[0].curSystem].loaded) {
        navDest = FindActiveCoLocatedSystem(shipData[0].curSystem);
        if (navDest == -1) {
            navDest = FindFirstCoLocatedSystem(shipData[0].curSystem);
            if (navDest == -1)
                navDest = shipData[0].curSystem;
            crashlog_print("warning: player is in inactive system ");
            crashlog_print_num(shipData[0].curSystem + 128);
            crashlog_print(" and no active equivalent could be found - reactivating system ");
            crashlog_print_num(navDest + 128);
            crashlog_print(" instead\n");
            gSystems[navDest].loaded = 1;
        }
        if (navDest != -1) {
            shipData[0].curSystem = navDest;
            for (j = 1; j < kMaxShips; j++) {
                if (shipData[j].active)
                    shipData[j].curSystem = navDest;
            }
            for (i = 0; i < kMaxShots; i++) {
                if (shotData[i].life > -2.0f)
                    shotData[i].curSystem = navDest;
            }
            for (i = 0; i < kMaxBoxes; i++) {
                if (boxData[i].timer >= 0.0f)
                    boxData[i].system = navDest;
            }
        }
    }
}
