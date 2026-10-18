// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics (inert in this file: it has no constant-size memset); -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atsndrate.c -- the sound driver's 16.16 rate helpers: to whole hertz, and the saturating multiply and divide. Notes: Research/notes/atsnddrv.md */

#include "burgertypes.h"
#include "burger.h"


/* A 16.16 rate to whole hertz, rounded. */
/* UFix32_To_UInt32. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00507d0b
ULong UFix32_To_UInt32(ULong rate)
{
    return ((rate >> 15) + 1) >> 1;
}

/* 16.16 multiply and divide, saturating; 0 if either side is 0. */
/* UFix32_Multiply. STATE: MATCH 100%, exe 47 / obj 47, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507d16
ULong UFix32_Multiply(ULong ratio, ULong rate)
{
    unsigned long long a;
    unsigned long long b;
    unsigned long long product;

    if (ratio != 0 && rate != 0) {
        a = ratio;
        b = rate;
        product = (a * b) >> kFixedShift;
        if (product > 0xffffffffUL)
            return 0xffffffffUL;
        return (ULong)product;
    }
    return 0;
}

/* UFix32_Divide. STATE: MATCH 100%, exe 48 / obj 48, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507d96
ULong UFix32_Divide(ULong device, ULong rate)
{
    unsigned long long a;
    unsigned long long b;
    unsigned long long quotient;

    if (device != 0 && rate != 0) {
        a = device;
        b = rate;
        quotient = (a << kFixedShift) / b;
        if (quotient > 0xffffffffUL)
            return 0xffffffffUL;
        return (ULong)quotient;
    }
    return 0;
}
