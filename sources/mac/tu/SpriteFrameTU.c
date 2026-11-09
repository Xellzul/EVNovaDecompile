/* SpriteFrameTU.c -- the mac 1.1.1 SpriteFrame.o: spriteframe.c. Notes: Research/notes/spriteframe.md */
#define SW_FILE "SpriteFrame.c"
#ifdef MAC1010
// COMPILE-WHY: 1.0.10's SpriteFrame.o alone folds the re-extension of a read short, 6 of its 15 bodies on both fragments, no spelling at level 2 (Research/notes/spriteframe.md); its neighbours SpriteWorldUtils, Tiling, BlitPixieMask keep level 2. Level 4, the 1.0.10 level of every SpriteWorld pragma object (Research/notes/drawbeam.md); 3 gives the same words here.
#pragma optimization_level 4
#endif
#define SW_LINE1010(n) ((n) < 100 ? (n) : (n) < 230 ? (n) - 7 : (n) < 440 ? (n) - 10 : (n) < 840 ? (n) - 12 : (n) < 1100 ? (n) - 18 : (n) < 1430 ? (n) - 25 : (n) - 28)
#include "../../lib/spriteframe.c"
