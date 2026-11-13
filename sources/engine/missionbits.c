// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* missionbits.c -- mission-bit set-expression entry points (win TU region around 0x00448020). Notes: Research/notes/missionbits.md */
#include <stdio.h>
#include "burger.h"
#include "missions.h"
#include "player.h"
#include "game.h"

EXTERN_C_BEGIN

/* EvalMissionBitSetString (mac _EvalMissionBitSetString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448020
// MAC: _EvalMissionBitSetString i386 0x00099dc5 ppc 0x000a5b94
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
// MAC: _EvalCurrentMissionBitSetString i386 0x00099a43 ppc 0x000a5748
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

/* DumpMissionBitLog (mac _DumpMissionBitLog, no caller: dead on win). STATE: see the mac gate. */
// MAC: _DumpMissionBitLog i386 0x00099df1 ppc 0x000a5bf4
void DumpMissionBitLog(void)
{
    FILE *log;
    short i;

    log = fopen("ncb_log.txt", "w");
    fputs("EV Nova control bit state:\n\n", log);
    for (i = 0; i < kMaxMissionBits; i++) {
        fprintf(log, "%i   ", i);
        if (i < 10000)
            fputc(' ', log);
        if (i < 1000)
            fputc(' ', log);
        if (i < 100)
            fputc(' ', log);
        if (i < 10)
            fputc(' ', log);
        if (missionBit[i])
            fputs("1\n", log);
        else
            fputs("0\n", log);
    }
    fclose(log);
}

EXTERN_C_END
