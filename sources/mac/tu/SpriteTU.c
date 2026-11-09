/* SpriteTU.c -- the mac 1.1.1 Sprite.o. Notes: Research/notes/spriteworld-mac.md */
#pragma optimization_level 4
#define SW_FILE "Sprite.c"
#define SW_LINE1010(n) ((n) < 1000 ? (n) : (n) >= 2300 && (n) < 2400 ? (n) - 1 : (n) - 2)
#include "../../lib/swcreatesprite.c"
#include "../../lib/swcreateemptysprite.c"
#include "../../lib/sprite.c"
#include "../../lib/swcloneframesfrommaster.c"
