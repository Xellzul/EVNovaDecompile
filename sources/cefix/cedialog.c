// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* cedialog.c -- EV Nova CE: the rounding helpers of the shipyard and player-info stat
   displays, also used by the map borders (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include "cefix.h"

/* The value rounded to float precision, then truncated: CE's fstp dword and cvttss2si.
   The float parameter is the rounding, so a caller passes the product unconverted. */
long CE_TruncF32(float value)
{
    return value;
}

/* FIST's rounding in its default mode, to nearest with ties to even, where a C cast
   truncates. */
long CE_RoundD(double value)
{
    long whole;
    double rest;

    whole = value;
    rest = value - whole;
    if (rest > 0.5 || (rest == 0.5 && (whole & 1) != 0))
        whole++;
    else if (rest < -0.5 || (rest == -0.5 && (whole & 1) != 0))
        whole--;
    return whole;
}

#endif /* CEFIX */
