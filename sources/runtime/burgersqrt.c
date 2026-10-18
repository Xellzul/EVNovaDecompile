// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
// COMPILE-WHY: `#pragma inline_intrinsics off` is MSL's own, from math_x87.c (MSL 9.0.3), which builds this wrapper out of line around an out-of-line sqrt; MSL's own line (Research/msl-version.md) is this one plus -lang c99 and the avoid_ebp prefix, both object-inert here
/* burgersqrt.c -- MSL's single-precision square root wrapper (sqrtf, math_x87.c) in the aligned run 0x004ee7a0-0x004eeff0. Notes: Research/notes/burgermisc.md */

#pragma inline_intrinsics off

extern double sqrt(double x);

/* FUN_004eed20 (MSL sqrtf). STATE: MATCH, byte-identical. OPEN: the name; see notes */
// FUNCTION: EVNOVA 0x004eed20
float FUN_004eed20(float x)
{
    return (float)(sqrt)((double)(x));
}
