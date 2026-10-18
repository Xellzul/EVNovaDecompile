// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreatepixelmaskfromgworld.c -- SWCreatePixelMaskFromGWorld (win 0x00479250, donor SpriteWorldUtils.c line 930). Notes: Research/notes/swcreatepixelmaskfromgworld.md */
/* A four-byte stub, `or eax,-1 / ret': EV Nova asks for a pixel mask nowhere, so the
   porter replaced the donor's 780-byte body with a failure return rather than deleting
   the entry point its callers still reference. -1 is not an OSErr, it is "did not happen". */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* SWCreatePixelMaskFromGWorld (mac _SWCreatePixelMaskFromGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00479250
short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld,
                                  Rect *rectP)
{
    return -1;
}
