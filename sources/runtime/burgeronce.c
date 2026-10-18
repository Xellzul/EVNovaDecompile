// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgeronce.c -- BurgerLib W9Win95Code: the single-instance guard, the active flag and the DirectSound volume table. Notes: Research/notes/burgeronce.md */

#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

void *Burger::Win32::m_pInstance;                /* 0x008676a8 */

#ifdef __cplusplus
extern "C" {
#endif

Word Win95ApplsActive = 1;                       /* 0x00578248 */

/* 0x0057824c: a 0..255 volume as DirectSound hundredths of a dB */
short Win95DirectSoundVolumes[256] = {
    -10000, -8000, -7000, -6415, -6000, -5678, -5415, -5192, -5000, -4830,
    -4678, -4540, -4415, -4299, -4192, -4093, -4000, -3912, -3830, -3752,
    -3678, -3607, -3540, -3476, -3415, -3356, -3299, -3245, -3192, -3142,
    -3093, -3045, -3000, -2955, -2912, -2870, -2830, -2790, -2752, -2714,
    -2678, -2642, -2607, -2573, -2540, -2508, -2476, -2445, -2415, -2385,
    -2356, -2327, -2299, -2272, -2245, -2218, -2192, -2167, -2142, -2117,
    -2093, -2069, -2045, -2022, -2000, -1977, -1955, -1933, -1912, -1891,
    -1870, -1850, -1830, -1810, -1790, -1771, -1752, -1733, -1714, -1696,
    -1678, -1660, -1642, -1624, -1607, -1590, -1573, -1557, -1540, -1524,
    -1508, -1492, -1476, -1460, -1445, -1430, -1415, -1400, -1385, -1370,
    -1356, -1341, -1327, -1313, -1299, -1285, -1272, -1258, -1245, -1231,
    -1218, -1205, -1192, -1179, -1167, -1154, -1142, -1129, -1117, -1105,
    -1093, -1081, -1069, -1057, -1045, -1034, -1022, -1011, -1000, -988,
    -977, -966, -955, -944, -933, -923, -912, -901, -891, -881,
    -870, -860, -850, -840, -830, -820, -810, -800, -790, -780,
    -771, -761, -752, -742, -733, -723, -714, -705, -696, -687,
    -678, -669, -660, -651, -642, -633, -624, -616, -607, -599,
    -590, -582, -573, -565, -557, -548, -540, -532, -524, -516,
    -508, -500, -492, -484, -476, -468, -460, -453, -445, -437,
    -430, -422, -415, -407, -400, -392, -385, -377, -370, -363,
    -356, -348, -341, -334, -327, -320, -313, -306, -299, -292,
    -285, -278, -272, -265, -258, -251, -245, -238, -231, -225,
    -218, -212, -205, -199, -192, -186, -179, -173, -167, -160,
    -154, -148, -142, -135, -129, -123, -117, -111, -105, -99,
    -93, -87, -81, -75, -69, -63, -57, -51, -45, -39,
    -34, -28, -22, -17, -11, 0
};

static void *InstanceLock;                       /* 0x008676ac, mapping */
static long Once;                                /* 0x008676b0, registered */

#define kMappingProtect     2                    /* PAGE_READONLY   */
#define kMappingSize        0x20
#define kAlreadyExists      0xb7                 /* ERROR_ALREADY_EXISTS */

/* UnlockMe. STATE: MATCH 100%, exe 8 / obj 8 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9bd0
void UnlockMe(void)
{
    if (InstanceLock != 0) {
        CloseHandle(InstanceLock);
        InstanceLock = 0;
    }
}

/* Win95Allow1Instance. STATE: MATCH 100%, exe 27 / obj 27 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f9c00
long Win95Allow1Instance(const char *name)
{
    InstanceLock = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, kMappingProtect, 0,
                                      kMappingSize, name);
    if (InstanceLock != 0 && GetLastError() != kAlreadyExists) {
        if (Once == 0) {
            atexit(UnlockMe);
            Once = 1;
        }
        return 0;
    }
    return 1;
}

#ifdef __cplusplus
}
#endif
