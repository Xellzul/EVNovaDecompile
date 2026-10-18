// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerfatal2.c -- burgerfatal, exe run 2. Notes: Research/notes/burgerfatal.md */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kFatalExitCode 0x37

Word IAmExiting;                                 /* 0x00870d98 */

/* Fatal (BurgerLib DbDebugManager). STATE: MATCH 100%, exe 35 / obj 35 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503df0
void Fatal(const char *fmt, ...)
{
    va_list args;

    if (fmt != 0 && fmt != ErrorMsg) {
        va_start(args, fmt);
        vsprintf(ErrorMsg, fmt, args);
        va_end(args);
    }

    if (IAmExiting == 0) {
        IAmExiting = 1;
        if (fmt != 0)
            DebugXString(ErrorMsg);
        exit(kFatalExitCode);
    }
}

/* NonFatal (BurgerLib DbDebugManager). STATE: MATCH 100%, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503e80
void NonFatal(const char *fmt, ...)
{
    va_list args;

    if (fmt != 0) {
        va_start(args, fmt);
        vsprintf(ErrorMsg, fmt, args);
        va_end(args);
        if (BombFlag != 0)
            Fatal(ErrorMsg);
    }
}

/* SetErrBombFlag (BurgerLib DbDebugManager). STATE: MATCH 100%, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503ee0
Word SetErrBombFlag(Word flag)
{
    Word old;

    old = BombFlag;
    BombFlag = flag;
    return old;
}

/* DebugXLongWord (BurgerLib DbDebugManager). STATE: MATCH 100%, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503ef0
void DebugXLongWord(Word32 value)
{
    char text[16];

    NumberToAscii(text, value, 0);
    DebugXString(text);
}

#ifdef __cplusplus
}
#endif
