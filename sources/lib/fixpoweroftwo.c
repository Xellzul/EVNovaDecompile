// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* fixpoweroftwo.c -- fix_power_of_two (win 0x004736a0). Notes: Research/notes/fixpoweroftwo.md */
/* Clamp an UNSIGNED argument to 31: `cmp eax, 0x20 / jae' is an unsigned compare, so the
   guard is `>= 32u' and a negative level would wrap and clamp rather than pass through. */
#include "macport.h"
#include "shipobj.h"

/* fix_power_of_two. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004736a0
// MAC: _fix_power_of_two i386 0x000c17d3 ppc 0x000cd830
uint32_t fix_power_of_two(uint32_t level)
{
    if (level >= 32)
        return 31;

    return level;
}
