/* SWGameUtilsTU.c -- the mac 1.1.1 SWGameUtils.o. Notes: Research/notes/spriteworld-mac.md */
#define MAC_GAMEUTILS_ONLY
#include "../spriteworld/swgameutilsrandom.c"
#include "../../lib/centerrect.c"
#include "../spriteworld/swgameutilsmisc.c"
#include "../../lib/getgdevicedepth.c"
#include "../spriteworld/swgameutilspalette.c"
#include "../../lib/swparticles.c"
#include "../spriteworld/swgameutilsdepth.c"
short gOldMBarHeight;
RgnHandle gOldVisRgn = NULL;
RgnHandle gUpdateRgn = NULL;
#if TARGET_API_MAC_OS8
Boolean gControlStripWasVisible = false;
#else
int32_t gControlStripWasVisible = 0;
#endif
