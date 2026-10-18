// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* dispose8bitalphatable.c -- Dispose8BitAlphaTable (win 0x00474300), then CrtInitHook3, the .CRT hook that ends its object. Notes: Research/notes/dispose8bitalphatable.md */
/* Frees the 5 x 256 array of pointers Construct8BitAlphaTable (0x004740d0) builds: the
   mac's `_alphaTable'. Both loop counters are SHORTS (`cmp bp, 0x100' and `cmp si, 5',
   both signed) while the row base is mwcc's own strength-reduced induction variable. */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"
#include "game.h"

unsigned char DAT_007cdab8[4112];
unsigned long alpha;
unsigned long alphaR;
unsigned long alphaG;
unsigned long alphaB;
unsigned long overR;
unsigned long overG;
unsigned long overB;
unsigned long overBias;
short scaleR;
short scaleG;
short scaleB;
unsigned long biasColor;
unsigned char *alphaTable[5][256];
unsigned char DAT_007cfef4[1547];

/* Dispose8BitAlphaTable (mac _Dispose8BitAlphaTable). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474300
void Dispose8BitAlphaTable(void)
{
    short i, j;

    for (i = 0; i < 5; i++)
        for (j = 0; j < 256; j++)
            DeallocAPointer(alphaTable[i][j]);
}

/* CrtInitHook3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00474340
void CrtInitHook3(void)
{
}

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
