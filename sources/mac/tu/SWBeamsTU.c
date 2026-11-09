/* SWBeamsTU.c -- the mac 1.1.1 SWBeams.o. Notes: Research/notes/spriteworld-mac.md */
// COMPILE-WHY: 1.0.10's SWBeams.o is level 4 (six of seven bodies MATCH there, three at level 3); 1.1.1's gcc objects are the same at 3 and 4 (Research/notes/drawbeam.md).
#pragma optimization_level 4
#define SW_FILE "SWBeams.c"
#define SW_LINE1010(n) ((n) - 1)
#include "macport.h"
#include "spriteworld.h"
#include "../include/swutils.h"
#include "../../lib/drawbeam.c"
