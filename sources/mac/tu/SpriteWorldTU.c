/* SpriteWorldTU.c -- the mac 1.1.1 SpriteWorld.o. Notes: Research/notes/spriteworld-mac.md */
#pragma optimization_level 4
#define SW_FILE "SpriteWorld.c"
#define SW_LINE1010(n) ((n) < 750 ? (n) - 15 : (n) < 1200 ? (n) - 25 : (n) < 1500 ? (n) - 31 : (n) < 2000 ? (n) - 40 : (n) - 55)
#include "../../lib/swcreatespriteworld.c"
#include "../../lib/spriteworld.c"
#include "../../lib/swdepthok.c"
