// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swstickyerror.c -- SWClearStickyError (win 0x004794a0, donor SpriteWorldUtils.c line 1820) and SWSetStickyIfError (win 0x004794b0, donor line 1838). Notes: Research/notes/swstickyerror.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* SWClearStickyError (mac _SWClearStickyError). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794a0
void SWClearStickyError(void)
{
    gSWStickyError = 0;
}

/* SWSetStickyIfError (mac _SWSetStickyIfError). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794b0
void SWSetStickyIfError(short errNum)
{
    int err = errNum;

    if (err != 0)
        gSWStickyError = errNum;
}
