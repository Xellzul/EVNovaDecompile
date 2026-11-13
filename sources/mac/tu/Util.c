/* Util.c -- the mac 1.1.1 Util.o: gameutils.c (CenterWindow), nexttarget.c, prevtarget.c, sprites.c and engineutils.c in win link order; the mac-only functions between the twins are MACBUILD hunks in place. Notes: Research/notes/engineutils.md */
#ifdef MAC1010 /* Rand is in Util.o here; assert lines from Research/notes/spriteworld-mac.md */
#define SW_LINE1010(n) ((n) - 18)
#else
#define MAC_NO_FASTRANDOM
#endif
#include "../../engine/gameutils.c"
#include "../../engine/nexttarget.c"
#include "../../engine/prevtarget.c"
#include "../../engine/sprites.c"
#include "../../engine/engineutils.c"
CTabHandle colorCluts[7];
short mBarHiddenFlag;
int32_t oldMBarHeight;
CTabHandle origClut;
CTabHandle pureWhiteClut;
DialogPtr warningDialog;
RgnHandle oldGrayRgn = NULL;
RgnHandle screensRgn = NULL;
