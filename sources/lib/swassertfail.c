// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swassertfail.c -- SWAssertFail (win 0x004794d0, donor SpriteWorldUtils.c line 1866). Notes: Research/notes/swassertfail.md */
#include "macport.h"
#include "burger.h"
#include "shipobj.h"

#define kAssertAlertID 128

typedef unsigned char Str15[16];

typedef void (*CleanUpCallBackPtr)(void);

CleanUpCallBackPtr gSWCleanUpCallBackP;              /* 0x008700d0 */

/* SWAssertFail (mac _SWAssertFail). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794d0
void SWAssertFail(char *filename, int lineNum)
{
    Str255 fileString = "\p";
    Str15 lineString;
    short n, result;
    unsigned long junkTime;

    NumToString(lineNum, lineString);

    for (n = 0; filename[n] != 0; n++)
        fileString[n + 1] = filename[n];

    fileString[0] += n;

    if (gSWCleanUpCallBackP != 0)
        (*gSWCleanUpCallBackP)();

    OSCursorReset();
    ParamText("\pAssertion failure! Nova will now quit. The failure occurred here:",
              fileString, lineString, "\pN/A");
    result = Alert(kAssertAlertID, 0);

    if (result < 0) {
        SysBeep(1);
        Delay(15, &junkTime);
    }

    ExitToShell();
}
