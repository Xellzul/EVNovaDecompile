// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreategworldfrompict.c -- SWCreateGWorldFromPict (win 0x00478fa0, donor SpriteWorldUtils.c line 392). Notes: Research/notes/swcreategworldfrompict.md */
/* The port's NewGWorld takes four arguments, not six: with no GDevice list the donor's
   device query and its first arm both go. The `err == noErr' test goes through an int
   copy, because mwcc 418 always narrows a short compare. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* SWCreateGWorldFromPict (mac _SWCreateGWorldFromPict). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478fa0
short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld,
                             CGrafPtr *pictGWorld, void *pictH)
{
    short err;
    CGrafPtr saveGWorld;
    void *saveGDevice;
    CGrafPtr tempGWorld;
    Rect pictRect;
    short depth;

    *pictGWorld = 0;

    GetPort(&saveGWorld);
    saveGDevice = GetGWorldDevice(saveGWorld);

    depth = destSpriteWorld->pixelDepth;

    QDGetPictureBounds(pictH, &pictRect);
    OffsetRect(&pictRect, -pictRect.left, -pictRect.top);

    err = NewGWorld(&tempGWorld, depth, &pictRect, 0);

    if ((int)err == noErr) {
        *pictGWorld = tempGWorld;

        SetPort(tempGWorld);

        EraseRect(&pictRect);
        DrawPicHandle(pictH, &pictRect);
    }

    SetPort(saveGWorld);

    SWSetStickyIfError(err);

    return err;
}
