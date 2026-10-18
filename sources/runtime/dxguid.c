// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); runtime P band's line, data only: the GUIDs are Microsoft's dxguid.lib, linked inside BurgerLib's run (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
// LINK: before atfilespec2; data EVNOVA 0x005713e8
/* dxguid.c -- the three DirectDraw interface IDs the game links from dxguid.lib. Notes: Research/notes/burgervideomode.md */

#include "burgerddraw.h"

#ifdef __cplusplus
extern "C" {
#endif

const Guid IID_IDirectDrawGammaControl = {      /* 0x005713e8 */
    0x69c11c3e, 0xb46b, 0x11d1,
    { 0xad, 0x7a, 0x00, 0xc0, 0x4f, 0xc2, 0x9b, 0x4e }
};
const Guid IID_IDirectDraw4 = {                  /* 0x005713f8 */
    0x9c59509a, 0x39bd, 0x11d1,
    { 0x8c, 0x4a, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc5 }
};
const Guid IID_IDirectDraw2 = {                  /* 0x00571408 */
    0xb3a6f3e0, 0x2b43, 0x11cf,
    { 0xa2, 0xde, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56 }
};

#ifdef __cplusplus
}
#endif
