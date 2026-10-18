// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* nvresourceexists.c -- nv_ResourceExists; exe 0x004ce640. Notes: Research/notes/resmgr.md */

#include <stddef.h>
#include "macport.h"
#include "gametypes.h"
#include "gameutil.h"
#include "burger.h"

/* nv_ResourceExists (mac (port layer)). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ce640
char nv_ResourceExists(unsigned long type, short id)
{
    return nv_FindResourceRecord(type, id) != NULL;
}
