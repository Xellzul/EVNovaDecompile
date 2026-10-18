// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgeroscursor.c -- BurgerLib OCWin95Code: showing, hiding and resetting the system cursor. Notes: Research/notes/burgeroscursor.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

Word OSCursorLastCursor;                         /* 0x00870dc8 */

/* OSCursorShow (BurgerLib OCWin95Code). STATE: MATCH 100.0%, exe 10 / obj 10,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1a20
void OSCursorShow(void)
{
    if (OSCursorVisibleFlag == 0) {
        OSCursorVisibleFlag = 1;
        while (ShowCursor(1) < 0)
            ;
    }
}

/* OSCursorHide (BurgerLib OCWin95Code). STATE: MATCH 100.0%, exe 10 / obj 10,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1a50
void OSCursorHide(void)
{
    if (OSCursorVisibleFlag != 0) {
        OSCursorVisibleFlag = 0;
        while (ShowCursor(0) >= 0)
            ;
    }
}

/* OSCursorReset (BurgerLib OCWin95Code). STATE: MATCH 100.0%, exe 27 / obj 27,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1a80
void OSCursorReset(void)
{
    void *cursor;

    if (OSCursorLastCursor != IDC_ARROW) {
        cursor = LoadCursorA(Burger::Win32::m_pInstance,(const char *)IDC_ARROW);
        if (cursor != 0 || (cursor = LoadCursorA(0, (const char *)IDC_ARROW)) != 0) {
            OSCursorLastCursor = IDC_ARROW;
            SetCursor(cursor);
        }
    }
    OSCursorVisibleFlag = 1;
    while (ShowCursor(1) < 0)
        ;
}

#ifdef __cplusplus
}
#endif
