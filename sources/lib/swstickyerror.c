// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swstickyerror.c -- SWClearStickyError (win 0x004794a0, donor SpriteWorldUtils.c line 1820) and SWSetStickyIfError (win 0x004794b0, donor line 1838). Notes: Research/notes/swstickyerror.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* SWClearStickyError (mac _SWClearStickyError). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794a0
// MAC: _SWClearStickyError i386 0x000a7a36 ppc 0x000b2834
void SWClearStickyError(void)
{
    gSWStickyError = 0;
}

#ifdef MACBUILD
/* SWStickyError (mac _SWStickyError, mac-only). STATE: see the mac gate. */
// MAC: _SWStickyError i386 0x000a7a44 ppc 0x000b2844
OSErr SWStickyError(void)
{
    return gSWStickyError;
}
#endif

/* SWSetStickyIfError (mac _SWSetStickyIfError). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794b0
// MAC: _SWSetStickyIfError i386 0x000a6f77 ppc 0x000b1ba4
#ifdef MACBUILD
void SWSetStickyIfError(short errNum)
{
    if (errNum != noErr)
        gSWStickyError = errNum;
}
#else
void SWSetStickyIfError(short errNum)
{
    int err = errNum;

    if (err != 0)
        gSWStickyError = errNum;
}
#endif

#ifdef MACBUILD
/* SWHasSystem7 (mac _SWHasSystem7, mac-only). STATE: see the mac gate. */
// MAC: _SWHasSystem7 i386 0x000a81d3 ppc 0x000b2fd0
Boolean SWHasSystem7(void)
{
    long versionNumber;
    OSErr err;

    err = Gestalt(gestaltSystemVersion, &versionNumber);

    SWSetStickyIfError(err);
    return (err == noErr) && (versionNumber >= 0x0700);
}
#endif
