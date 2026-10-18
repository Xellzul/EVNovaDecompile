// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreategworldfrompictresource.c -- SWCreateGWorldFromPictResource (win 0x00478f30, donor SpriteWorldUtils.c line 356). Notes: Research/notes/swcreategworldfrompictresource.md */
/* The fork's fourth parameter is a byte flag choosing between 0x004b9050 and 0x004b9060
   -- both print as `LoadPictResource(id, 0)' in a decompiler export and the exe bytes
   say one passes gamma 1. The error arm is a flat resNotFound, -192. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"



/* SWCreateGWorldFromPictResource (mac _SWCreateGWorldFromPictResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478f30
short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                     CGrafPtr *pictGWorldP,
                                     short pictResID,
                                     char useGetPicture)
{
    short err = noErr;
    void *newPictH;

    if (useGetPicture)
        newPictH = GetPicture(pictResID);
    else
        newPictH = GetPictureNoGamma(pictResID);

    if (newPictH != 0) {
        err = SWCreateGWorldFromPict(destSpriteWorld, pictGWorldP, newPictH);

        QDReleaseResource(newPictH);
    } else {
        err = resNotFound;
    }

    SWSetStickyIfError(err);

    return err;
}
