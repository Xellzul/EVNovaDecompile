// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* nexttarget.c -- cycle the player's target (win 0x00461bd0). Notes: Research/notes/nexttarget.md */
#include <stddef.h>

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "player.h"
#include "ships.h"
#include "prototypes.h"




/* FindNextTarget. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00461bd0
short FindNextTarget(short cur, short system)
{
    char isPlayerEscort[kMaxShips];
    char pad[2];
    char canUntarget;
    char canCloaked;
    char eligible[kMaxShips];
    char onEscortMission[kMaxShips];
    short i;
    char wantHostile;

    if (KeyCheck(kScanLCtrl) != 0 || KeyCheck(kScanRCtrl) != 0)
        wantHostile = 1;
    else
        wantHostile = 0;
    canUntarget = ShipCanTargetUntargetableShips(shipData);
    canCloaked = ShipCanTargetCloakedShips(shipData);

    i = 1;
    do {
        onEscortMission[i] = 0;
        isPlayerEscort[i] = 0;
        eligible[i] = 0;
        if (shipData[i].specialMission == -1) {
            if (shipData[i].parentShip != -1 && shipData[i].parentShip != 0
                && shipData[i].defenseStellar == -1
                && shipData[shipData[i].parentShip].parentShip == 0)
                isPlayerEscort[i] = 1;
        } else if (currentMissionObjectives[shipData[i].specialMission].active != 0
                   && currentMissions[shipData[i].specialMission].shipBehav == kMissionModeEscort) {
            onEscortMission[i] = 1;
        }
        if (onEscortMission[i] == 0) {
            if (shipData[i].parentShip == 0
                || (isPlayerEscort[i] != 0
                    && onEscortMission[shipData[i].parentShip] == 0))
                eligible[i] = 1;
        }
        i++;
    } while (i < kMaxShips);

    if (cur != -1) {
        i = (cur + 1);
        while (i < kMaxShips) {
            if (shipData[i].active != 0 && !IsDying(&shipData[i])) {
                if (!ShipIsCloaked(&shipData[i]) || canCloaked != 0
                    || (eligible[i] != 0 && wantHostile != 0)) {
                    if (shipData[i].curSystem == system
                        && !AIShipIsEmergingFromHypergate(&shipData[i])
                        && ((gShipTypes[shipData[i].type].flags2
                             & kShipFlag2NoTarget) == 0 || canUntarget != 0)
                        && eligible[i] == wantHostile)
                        return i;
                }
            }
            i++;
        }
    } else {
        i = 1;
        do {
            if (shipData[i].active != 0 && !IsDying(&shipData[i])) {
                if (!ShipIsCloaked(&shipData[i]) || canCloaked != 0
                    || (eligible[i] != 0 && wantHostile != 0)) {
                    if (shipData[i].curSystem == system
                        && !AIShipIsEmergingFromHypergate(&shipData[i])
                        && ((gShipTypes[shipData[i].type].flags2
                             & kShipFlag2NoTarget) == 0 || canUntarget != 0)
                        && eligible[i] == wantHostile)
                        return i;
                }
            }
            i++;
        } while (i < kMaxShips);
    }
    return cur;
}
