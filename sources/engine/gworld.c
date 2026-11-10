// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* gworld.c -- the GWorldContainer wrapper around an offscreen drawing world (win TU region 0x0046f740..0x0046f95a). Notes: Research/notes/gworld.md */
#include "macport.h"
#include "render.h"
#include "cefix.h"

#ifdef MACBUILD
CGrafPtr prevGWorld;
GDHandle prevGDevice;
#else
static void *savedDevice;
static CGrafPtr savedPort;
#endif


/* CreateGWorldContainer (mac _CreateGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f740
// MAC: _CreateGWorldContainer i386 0x000a4517 ppc 0x000b001c
#ifdef MACBUILD
void CreateGWorldContainer(GWorldContainer *gwc, GDHandle device, short depth,
                           Rect *bounds)
{
    PixMapHandle pixMap;
    short pixelDepth;

    GetGWorld(&prevGWorld, &prevGDevice);
    if (depth == 0) {
        if (device != NULL)
            pixelDepth = (*(*device)->gdPMap)->pixelSize;
        else
            pixelDepth = (*(*GetMainDevice())->gdPMap)->pixelSize;
    } else if (depth > 0) {
        pixelDepth = depth;
    } else {
#ifdef MAC1010
        pixelDepth = 8;
#else
        pixelDepth = 16;
#endif
    }
#ifdef MAC1010 /* 1.0.10 real: 0x100b6918 r7 is the device, r8 is 2, noNewDevice */
    if ((NewGWorld)(&gwc->world, pixelDepth, bounds, NULL, device, noNewDevice) != noErr) {
#else
    if ((NewGWorld)(&gwc->world, pixelDepth, bounds, NULL, NULL, kNativeEndianPixMap) != noErr) {
#endif
        gwc->world = NULL;
        return;
    }
    gwc->bounds = *bounds;
    pixMap = GetGWorldPixMap(gwc->world);
    LockPixels(pixMap);
    HLockHi((Handle)pixMap);
    gwc->pixMap = *pixMap;
#ifdef MAC1010 /* 1.0.10 real: 0x100b697c r4 loads the saved device for SetGWorld */
    SetGWorld(prevGWorld, prevGDevice);
#else
    SetGWorld(prevGWorld, NULL);
#endif
}
#else
void CreateGWorldContainer(GWorldContainer *gwc, long flags, short depth,
                           Rect *bounds)
{
    if (gwc != 0) {
        RGBColor fore = { 0, 0, 0 };
        RGBColor back = { 0xffff, 0xffff, 0xffff };
        short useDepth;

        GetPort(&savedPort);
        if (depth < 8)
            useDepth = 16;
        else if (depth == 0)
            useDepth = 16;
        else if (depth <= 0)
            useDepth = 8;
        else
            useDepth = depth;
#ifdef CEFIX /* F11.01 */
        if (CE_NewGWorld(&gwc->world, useDepth, bounds, 0) != 0) {
            gwc->world = 0;
            return;
        }
#else
        if (NewGWorld(&gwc->world, useDepth, bounds, 0) != 0) {
            gwc->world = 0;
            return;
        }
#endif
        gwc->bounds.top = bounds->top;
        gwc->bounds.left = bounds->left;
        gwc->bounds.bottom = bounds->bottom;
        gwc->bounds.right = bounds->right;
        SetPort(gwc->world);
        RGBForeColor(&fore);
        RGBBackColor(&back);
        TextFont(smallTextFont);
        TextFace(0);
        TextSize(smallTextSize);
        SetPort(savedPort);
    }
}
#endif

/* CreateGWorldContainerFromPictResource (mac _CreateGWorldContainerFromPictResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f850
// MAC: _CreateGWorldContainerFromPictResource i386 0x000a46b7 ppc 0x000b0208
#ifdef MACBUILD
void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, GDHandle device,
                                           short depth, short picID)
{
    PicHandle pic;
    Rect bounds;

    pic = GetPicture(picID);
    if (pic != NULL) {
        QDGetPictureBounds(pic, &bounds);
        OffsetRect(&bounds, -bounds.left, -bounds.top);
        CreateGWorldContainer(gwc, device, depth, &bounds);
        SetGWorldContainer(gwc);
        DrawPicture(pic, &gwc->bounds);
        UnsetGWorldContainer(gwc);
        ReleaseResource((Handle)pic);
    } else {
        gwc->world = NULL;
    }
}
#else
void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, long flags,
                                           short depth, short picID)
{
    PicHandle pic;
    Rect bounds;

    pic = GetPicture(picID);
    if (pic != 0) {
        QDGetPictureBounds(pic, &bounds);
        OffsetRect(&bounds, -bounds.left, -bounds.top);
        CreateGWorldContainer(gwc, flags, depth, &bounds);
        SetGWorldContainer(gwc);
        DrawPicHandle(pic, &gwc->bounds);
        UnsetGWorldContainer(gwc);
        QDReleaseResource(pic);
    } else {
        gwc->world = 0;
    }
}
#endif

/* SetGWorldContainer (mac _SetGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f8e0
// MAC: _SetGWorldContainer i386 0x000a4613 ppc 0x000b0120
#ifdef MACBUILD
void SetGWorldContainer(GWorldContainer *gwc)
{
    GetGWorld(&prevGWorld, &prevGDevice);
    if (gwc->world != NULL)
        SetGWorld(gwc->world, NULL);
}
#else
void SetGWorldContainer(GWorldContainer *gwc)
{
    GetPort(&savedPort);
    savedDevice = GetGWorldDevice(savedPort);
    if (gwc->world != 0)
        SetPort(gwc->world);
}
#endif

/* UnsetGWorldContainer (mac _UnsetGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f910
// MAC: _UnsetGWorldContainer i386 0x000a464a ppc 0x000b0180
#ifdef MACBUILD
void UnsetGWorldContainer(GWorldContainer *gwc)
{
#ifdef MAC1010 /* 1.0.10 real: 0x100b6ae4 r4 loads the saved device for SetGWorld */
    SetGWorld(prevGWorld, prevGDevice);
#else
    SetGWorld(prevGWorld, NULL);
#endif
}
#else
void UnsetGWorldContainer(GWorldContainer *gwc)
{
    SetPort(savedPort);
}
#endif

/* DisposeGWorldContainer (mac _DisposeGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f920
// MAC: _DisposeGWorldContainer i386 0x000a4669 ppc 0x000b0194
#ifdef MACBUILD
void DisposeGWorldContainer(GWorldContainer *gwc)
{
    if (gwc->world != NULL) {
        UnlockPixels(GetGWorldPixMap(gwc->world));
        DisposeGWorld(gwc->world);
    }
}
#else
void DisposeGWorldContainer(GWorldContainer *gwc)
{
    if (gwc->world != 0)
        DisposeGWorld(gwc->world);
}
#endif

/* GetGWCPixMap (mac _GetGWCPixMap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f940
// MAC: _GetGWCPixMap i386 0x000a469e ppc 0x000b01f0
#ifdef MACBUILD
PixMapHandle GetGWCPixMap(GWorldContainer *gwc)
{
    if (gwc->world != NULL)
        return GetGWorldPixMap(gwc->world);
    return NULL;
}
#else
PixMapPtr GetGWCPixMap(GWorldContainer *gwc)
{
    if (gwc != 0 && gwc->world != 0)
        return GetCWindowPixMap(gwc->world);
    return 0;
}
#endif
