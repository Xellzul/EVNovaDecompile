// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgermisc6.c -- burgermisc, exe run 6. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"
#include "burgerplat.h"

struct joyinfoex_tag {
    Word32 dwSize;
    Word32 dwFlags;
    Word32 dwXpos;
    Word32 dwYpos;
    Word32 dwZpos;
    Word32 dwRpos;
    Word32 dwUpos;
    Word32 dwVpos;
    Word32 dwButtons;
    Word32 dwButtonNumber;
    Word32 dwPOV;
    Word32 dwReserved1;
    Word32 dwReserved2;
};

/* The member's 0x38 bytes of .bss; no live code reads them. */
static joyinfoex_tag PrivJoyInfo;
static Word PrivJoyNumber;

#ifdef __cplusplus
extern "C" {
#endif

/* JoystickDestroy (BurgerLib InJoystickWin95 JoystickDestroy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00502490
void JoystickDestroy(void)
{
    JoystickPresent = 0;
}

#ifdef __cplusplus
}
#endif
