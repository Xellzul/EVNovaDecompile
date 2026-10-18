// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerfile.c -- BurgerLib's hand-written fixed-point multiply. Notes: Research/notes/burgerfile.md */

#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

long BurgerSndKillProcs[3];                      /* 0x00870d9c */
ULong BurgerSndExitIn;                           /* 0x0086ffec */

#define kFixedMax       0x7fffffff
#define kFixedMin       0x80000000

/* FixedMultiply (BurgerLib IMFixMul, hand assembly in the library: FxMicrosoftVC5.obj).
   STATE: new. OPEN: none */
// FUNCTION: EVNOVA 0x004f1c70
NAKED long FixedMultiply(long a, long b)
{
    __asm {
        mov     eax, dword ptr [esp + 4]
        mov     edx, dword ptr [esp + 8]
        imul    edx
        cmp     edx, 0x8000
        jge     overflow
        cmp     edx, 0xffff8000
        jl      underflow
        shrd    eax, edx, kFixedShift
        ret
        align   16
    overflow:
        mov     eax, kFixedMax
        ret
    underflow:
        mov     eax, kFixedMin
        ret
    }
}

#ifdef __cplusplus
}
#endif
