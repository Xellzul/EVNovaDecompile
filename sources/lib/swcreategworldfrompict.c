// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* swcreategworldfrompict.c -- SWCreateGWorldFromPict (win 0x00478fa0, donor SpriteWorldUtils.c line 392). Notes: Research/notes/swcreategworldfrompict.md */
/* The port's NewGWorld takes four arguments, not six: with no GDevice list the donor's
   device query and its first arm both go. The `err == noErr' test goes through an int
   copy, because mwcc 418 always narrows a short compare. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#include "cefix.h"
#ifdef MACBUILD
#include "render.h"
#endif


/* SWCreateGWorldFromPict (mac _SWCreateGWorldFromPict). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478fa0
// MAC: _SWCreateGWorldFromPict i386 0x000a7dc7 ppc 0x000b2bf4
#ifdef MACBUILD
short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld, CGrafPtr *pictGWorld, void *pictH)
{
    OSErr err;
    GWorldPtr saveGWorld;
    GDHandle saveGDevice;
    GWorldPtr tempGWorld;
    Rect pictRect;
    short depth;
    GDHandle theGDH;

    *pictGWorld = NULL;

    GetGWorld(&saveGWorld, &saveGDevice);

    depth = destSpriteWorld->pixelDepth;
    if (useLegacyRenderingPipeline)
        theGDH = destSpriteWorld->mainSWGDH;

    QDGetPictureBounds(pictH, &pictRect);
    OffsetRect(&pictRect, -pictRect.left, -pictRect.top);

    if (useLegacyRenderingPipeline) {
        if ((**((**theGDH).gdPMap)).pixelSize == depth)
            err = (NewGWorld)(&tempGWorld, depth, &pictRect, NULL, theGDH, noNewDevice);
        else
            err = (NewGWorld)(&tempGWorld, depth, &pictRect, NULL, NULL, 0);
    } else {
        err = (NewGWorld)(&tempGWorld, 16, &pictRect, NULL, NULL, kNativeEndianPixMap);
    }

    if (err == noErr) {
        *pictGWorld = tempGWorld;

        SetGWorld(tempGWorld, NULL);

        LockPixels(GetGWorldPixMap(tempGWorld));
        EraseRect(&pictRect);
        DrawPicHandle(pictH, &pictRect);
        UnlockPixels(GetGWorldPixMap(tempGWorld));
    }

    if (useLegacyRenderingPipeline)
        SetGWorld(saveGWorld, saveGDevice);
    else
        SetGWorld(saveGWorld, NULL);

    SWSetStickyIfError(err);
    return err;
}
#else
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

#ifdef CEFIX /* F11.03 */
    err = CE_NewGWorld(&tempGWorld, depth, &pictRect, 0);
#else
    err = NewGWorld(&tempGWorld, depth, &pictRect, 0);
#endif

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
#endif
