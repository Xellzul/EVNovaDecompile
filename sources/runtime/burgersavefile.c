// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgersavefile.c -- BurgerLib MMMemoryGeneric's cleared allocators. Notes: Research/notes/burgersavefile.md */

#include <stdio.h>
#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate a handle and clear what it points at. */
/* AllocAHandleClear. STATE: MATCH 100%, exe 23 / obj 23 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2970
void **AllocAHandleClear(ULong size)
{
    void **h;

    h = (void **)AllocAHandle2(size, 0);
    if (h != 0)
        FastMemSet(*h, 0, size);
    return h;
}

/* AllocAPointerClear (BurgerLib MMMemoryGeneric). STATE: MATCH 100.0%, exe 21 / obj 21,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f29c0
void *AllocAPointerClear(unsigned long size)
{
    void *mem;

    mem = AllocAPointer(size);
    if (mem != 0)
        FastMemSet(mem, 0, size);
    return mem;
}

#ifdef __cplusplus
}
#endif
