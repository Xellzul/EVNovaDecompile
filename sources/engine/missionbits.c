// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* missionbits.c -- mission-bit set-expression entry points (win TU region around 0x00448020). Notes: Research/notes/missionbits.md */
#include "burger.h"
#include "missions.h"
#include "player.h"
#include "game.h"


/* EvalMissionBitSetString (mac _EvalMissionBitSetString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448020
void EvalMissionBitSetString(char *expr)
{
    if (strlen(expr) == 0)
        return;
    strcpy(parseBuffer, expr);
    EvalSetExp();
    ResetPlayerPrecalcedValues();
}

/* EvalCurrentMissionBitSetString (mac _EvalCurrentMissionBitSetString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448050
void EvalCurrentMissionBitSetString(char *expr, short mission)
{
    if (strlen(expr) == 0)
        return;
    strcpy(parseBuffer, expr);
    currentMissionIndexForSetParser = mission;
    EvalSetExp();
    currentMissionIndexForSetParser = -1;
    ResetPlayerPrecalcedValues();
}
