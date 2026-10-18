// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreateregionfromgworldandrectfinish.c -- SWCreateRegionFromGWorldAndRectFinish (win 0x00478f20, donor SpriteWorldUtils.c line 279). Notes: Research/notes/swcreateregionfromgworldandrectfinish.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* SWCreateRegionFromGWorldAndRectFinish (mac _SWCreateRegionFromGWorldAndRectFinish). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478f20
void SWCreateRegionFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld)
{
    if (tempMaskGWorld != 0)
        DisposeGWorld(tempMaskGWorld);
}
