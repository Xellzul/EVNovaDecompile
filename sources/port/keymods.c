// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* keymods.c -- the modifier-key scan the port has to synthesise (win 0x004cea20). Notes: Research/notes/keymods.md */
#include "macport.h"

/* GetKeyModifiers. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cea20
long GetKeyModifiers(void)
{
    long mods;

    mods = 0;
    if (KeyIsDown(kScanLShift) || KeyIsDown(kScanRShift))
        mods |= shiftKey;
    if (KeyIsDown(kScanLCtrl) || KeyIsDown(kScanRCtrl))
        mods |= controlKey;
    if (KeyIsDown(kScanLAlt) || KeyIsDown(kScanRAlt))
        mods |= optionKey;
    return mods;
}
