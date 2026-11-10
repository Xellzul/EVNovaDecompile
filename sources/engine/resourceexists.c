// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* resourceexists.c -- ResourceExists; exe 0x0047b7a0. Notes: Research/notes/resmgr.md */

#include <stddef.h>
#include "macport.h"
#include "gametypes.h"
#include "gameutil.h"
#include "burger.h"

/* engine-facing wrapper; lives at 0x0047b7a0, grouped here by topic */
/* ResourceExists (mac _ResourceExists). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7a0
Boolean ResourceExists(unsigned long type, short id)
{
    return nv_ResourceExists(type, id);
}
