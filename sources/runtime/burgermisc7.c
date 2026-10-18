// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgermisc7.c -- burgermisc, exe run 7. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FastMemSet. STATE: DIFF 0.0%, exe 9 / obj 6 instrs.
   OPEN: the exe's body is an inline rep-stosb memset; see notes */
// FUNCTION: EVNOVA 0x00503dd0
void FastMemSet(void *DestPtr, Word Fill, Word32 Length)
{
    memset(DestPtr, Fill, Length);
}

#ifdef __cplusplus
}
#endif
