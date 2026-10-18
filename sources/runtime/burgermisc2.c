// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgermisc2.c -- burgermisc, exe run 2. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char OSCursorVisibleFlag = 1;           /* 0x00577a34 */

/* OSCursorIsVisible (BurgerLib OCOSCursorManager's: returns the flag, 1 after OSCursorShow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004f1ca0
unsigned int OSCursorIsVisible(void)
{
    return OSCursorVisibleFlag;
}

#ifdef __cplusplus
}
#endif
