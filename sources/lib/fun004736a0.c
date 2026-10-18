// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* fun004736a0.c -- ClampAlphaLevel (win 0x004736a0). Notes: Research/notes/fun004736a0.md */
/* Clamp an UNSIGNED argument to 31: `cmp eax, 0x20 / jae' is an unsigned compare, so the
   guard is `>= 32u' and a negative level would wrap and clamp rather than pass through.
   Kept FUN_: the mac's 8-bit translucent blitter calls no clamp at all. */
#include "macport.h"
#include "shipobj.h"

/* ClampAlphaLevel. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004736a0
unsigned long ClampAlphaLevel(unsigned long level)
{
    if (level >= 32)
        return 31;

    return level;
}
