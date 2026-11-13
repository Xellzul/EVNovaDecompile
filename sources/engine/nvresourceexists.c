// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* nvresourceexists.c -- nv_ResourceExists; exe 0x004ce640. Notes: Research/notes/resmgr.md */

#include <stddef.h>
#include "macport.h"
#include "gametypes.h"
#include "gameutil.h"
#include "burger.h"

/* nv_ResourceExists (mac (port layer)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce640
Boolean nv_ResourceExists(uint32_t type, short id)
{
    return nv_FindResourceRecord(type, id) != NULL;
}
