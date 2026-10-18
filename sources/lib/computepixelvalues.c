// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* computepixelvalues.c -- ComputePixelValues (win 0x00479210, donor SpriteWorldUtils.c line 900). Notes: Research/notes/computepixelvalues.md */
/* Names two SpriteWorldUtils.c globals outright, because one function writes both in
   both arms with the values swapped: 0x007d0500 SWMaskedRGBValue (<= 8 bit 0x0000,
   > 8 bit 0xFFFF) and 0x007d0502 SWTransparentRGBValue (the reverse). */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

unsigned short SWMaskedRGBValue;
unsigned short SWTransparentRGBValue;
short gSWStickyError;
unsigned char DAT_007d0506[18];


/* ComputePixelValues (mac _ComputePixelValues). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00479210
void ComputePixelValues(short pixelSize)
{
    if (pixelSize <= 8) {
        SWMaskedRGBValue = 0x0000;
        SWTransparentRGBValue = 0xFFFF;
    } else {
        SWMaskedRGBValue = 0xFFFF;
        SWTransparentRGBValue = 0x0000;
    }
}
