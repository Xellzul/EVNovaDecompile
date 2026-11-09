/* SpriteWorldUtilsTU.c -- the mac 1.1.1 SpriteWorldUtils.o. Notes: Research/notes/spriteworld-mac.md */
#define SW_FILE "SpriteWorldUtils.c"
#define SW_LINE1010(n) ((n) < 1100 ? (n) - 56 : (n) < 1300 ? (n) - 59 : (n) - 62)
#include "../../lib/swcreateregionfromgworldandrectfinish.c"
#include "../../lib/swcreategworldfrompictresource.c"
#include "../../lib/swcreategworldfrompict.c"
#include "../../lib/swcreategworldfromciconmask.c"
#include "../../lib/swinvertgworld.c"
#include "../../lib/computepixelvalues.c"
#include "../../lib/swcreatepixelmaskfromgworld.c"
#include "../../lib/swfiximagegworld.c"
#include "../../lib/swstickyerror.c"
#include "../../lib/swassertfail.c"
