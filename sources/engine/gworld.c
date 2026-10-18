// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* gworld.c -- the GWorldContainer wrapper around an offscreen drawing world (win TU region 0x0046f740..0x0046f95a). Notes: Research/notes/gworld.md */
#include "macport.h"
#include "render.h"

static void *savedDevice;
static CGrafPtr savedPort;


/* CreateGWorldContainer (mac _CreateGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f740
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
        if (NewGWorld(&gwc->world, useDepth, bounds, 0) != 0) {
            gwc->world = 0;
            return;
        }
        gwc->bounds.top = bounds->top;
        gwc->bounds.left = bounds->left;
        gwc->bounds.bottom = bounds->bottom;
        gwc->bounds.right = bounds->right;
        SetPort(gwc->world);
        RGBForeColor(&fore);
        BackColor(&back);
        TextFont(smallTextFont);
        TextFace(0);
        TextSize(smallTextSize);
        SetPort(savedPort);
    }
}

/* CreateGWorldContainerFromPictResource (mac _CreateGWorldContainerFromPictResource). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f850
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

/* SetGWorldContainer (mac _SetGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f8e0
void SetGWorldContainer(GWorldContainer *gwc)
{
    GetPort(&savedPort);
    savedDevice = GetGWorldDevice(savedPort);
    if (gwc->world != 0)
        SetPort(gwc->world);
}

/* UnsetGWorldContainer (mac _UnsetGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f910
void UnsetGWorldContainer(GWorldContainer *gwc)
{
    SetPort(savedPort);
}

/* DisposeGWorldContainer (mac _DisposeGWorldContainer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f920
void DisposeGWorldContainer(GWorldContainer *gwc)
{
    if (gwc->world != 0)
        DisposeGWorld(gwc->world);
}

/* GetGWCPixMap (mac _GetGWCPixMap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f940
PixMapPtr GetGWCPixMap(GWorldContainer *gwc)
{
    if (gwc != 0 && gwc->world != 0)
        return GetCWindowPixMap(gwc->world);
    return 0;
}
