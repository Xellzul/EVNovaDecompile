// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* swcreateregionfromgworldandrectfinish.c -- SWCreateRegionFromGWorldAndRectFinish (win 0x00478f20, donor SpriteWorldUtils.c line 279). Notes: Research/notes/swcreateregionfromgworldandrectfinish.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


#ifdef MACBUILD
/* SWCreateRegionFromCIconMask (mac _SWCreateRegionFromCIconMask, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateRegionFromCIconMask i386 0x000a7b3d ppc 0x000b2958
OSErr SWCreateRegionFromCIconMask(RgnHandle *maskRgn, CIconHandle cIconH)
{
    OSErr err = noErr;
    RgnHandle tempMaskRgn;
    char saveState;
    BitMap iconMask;

    *maskRgn = NULL;

    saveState = HGetState((Handle)cIconH);
    HLock((Handle)cIconH);

    iconMask.rowBytes = (**cIconH).iconMask.rowBytes;
    iconMask.bounds = (**cIconH).iconMask.bounds;
    iconMask.baseAddr = (Ptr)(**cIconH).iconMaskData;

    tempMaskRgn = NewRgn();

    if (tempMaskRgn != NULL) {
        err = BitMapToRegion(tempMaskRgn, &iconMask);

        if (err == noErr) {
            *maskRgn = tempMaskRgn;
        } else {
            DisposeRgn(tempMaskRgn);
        }
    } else {
        err = MemError();
    }

    HSetState((Handle)cIconH, saveState);

    SWSetStickyIfError(err);
    return err;
}

/* SWCreateRegionFromGWorldAndRect (mac _SWCreateRegionFromGWorldAndRect, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateRegionFromGWorldAndRect i386 0x000a8a69 ppc 0x000b3a4c
OSErr SWCreateRegionFromGWorldAndRect(RgnHandle *maskRgn, GWorldPtr maskGWorld, Rect *frameRect)
{
    OSErr err = noErr;
    GDHandle saveGDH;
    GWorldPtr saveGWorld, tempMaskGWorld;
    Rect tempMaskRect;

    *maskRgn = NULL;

    GetGWorld(&saveGWorld, &saveGDH);

    tempMaskRect = *frameRect;
    OffsetRect(&tempMaskRect, -tempMaskRect.left, -tempMaskRect.top);

    err = SWCreateRegionFromGWorldAndRectStart(&tempMaskGWorld, tempMaskRect.right,
            tempMaskRect.bottom);

    if (err == noErr) {
        err = SWCreateRegionFromGWorldAndRectPartial(maskRgn, maskGWorld,
                tempMaskGWorld, frameRect);

        SWCreateRegionFromGWorldAndRectFinish(tempMaskGWorld);
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWCreateRegionFromGWorldAndRectStart (mac _SWCreateRegionFromGWorldAndRectStart, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateRegionFromGWorldAndRectStart i386 0x000a7bf2 ppc 0x000b2a30
OSErr SWCreateRegionFromGWorldAndRectStart(GWorldPtr *tempMaskGWorld, short maxWidth, short maxHeight)
{
    OSErr err = noErr;
    Rect tempMaskRect;

    SetRect(&tempMaskRect, 0, 0, maxWidth, maxHeight);
    err = (NewGWorld)(tempMaskGWorld, 1, &tempMaskRect, NULL, NULL, 0);
    if (err == noErr) {
        LockPixels(GetGWorldPixMap(*tempMaskGWorld));
    }

    SWSetStickyIfError(err);
    return err;
}
#endif

/* SWCreateRegionFromGWorldAndRectFinish (mac _SWCreateRegionFromGWorldAndRectFinish). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478f20
// MAC: _SWCreateRegionFromGWorldAndRectFinish i386 0x000a6f90 ppc 0x000b1bb8
void SWCreateRegionFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld)
{
    if (tempMaskGWorld != 0)
        DisposeGWorld(tempMaskGWorld);
}

#ifdef MACBUILD
/* SWCreateRegionFromGWorldAndRectPartial (mac _SWCreateRegionFromGWorldAndRectPartial, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateRegionFromGWorldAndRectPartial i386 0x000a7c8b ppc 0x000b2ab0
OSErr SWCreateRegionFromGWorldAndRectPartial(RgnHandle *maskRgn, GWorldPtr maskGWorld,
        GWorldPtr tempMaskGWorld, Rect *frameRect)
{
    OSErr err = noErr;
    GDHandle saveGDH;
    GWorldPtr saveGWorld;
    Rect tempMaskRect;
    RgnHandle tempRgnH;

    *maskRgn = NULL;

    GetGWorld(&saveGWorld, &saveGDH);

    tempMaskRect = *frameRect;
    OffsetRect(&tempMaskRect, -tempMaskRect.left, -tempMaskRect.top);

    LockPixels(GetGWorldPixMap(maskGWorld));
    SetGWorld(tempMaskGWorld, NULL);

    CopyBits(
        (BitMap *)*GetGWorldPixMap(maskGWorld),
        (BitMap *)*GetGWorldPixMap(tempMaskGWorld),
        frameRect,
        &tempMaskRect,
        srcCopy,
        NULL);

    tempRgnH = NewRgn();
    if (tempRgnH != NULL) {
        err = BitMapToRegion(tempRgnH, (BitMap *)*GetGWorldPixMap(tempMaskGWorld));

        UnlockPixels(GetGWorldPixMap(maskGWorld));

        if (err == noErr) {
            *maskRgn = tempRgnH;
        } else {
            DisposeRgn(tempRgnH);
        }
    } else {
        err = MemError();
    }
    SetGWorld(saveGWorld, NULL);

    SWSetStickyIfError(err);
    return err;
}
#endif
