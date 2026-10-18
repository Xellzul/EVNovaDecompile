// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* fadeblack.c -- fade out and paint the window black (win 0x004ab180), and the two startup splash screens that ride the same GWorldContainer/CopyBits plumbing as gworld.c. Notes: Research/notes/fadeblack.md */
#include "macport.h"
#include "gameutil.h"
#include "render.h"
#include "stellar.h"
#include "dialogs.h"
#include "game.h"

#define kSplashPictID       0x83
#define kAmbrosiaLogoPictID 0x1fa4

/* ShowSplashScreen. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aaf60
void ShowSplashScreen(void)
{
    void *pic;
    Rect *rect;
    Rect box;

    pic = GetPicture(kSplashPictID);
    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    rect = &loadPort.bounds;
    PaintRect(rect);
    box.top = loadPort.bounds.top;
    box.left = loadPort.bounds.left;
    box.bottom = loadPort.bounds.bottom;
    box.right = loadPort.bounds.right;
    if (pic != 0) {
        CenterPict(pic, &box);
        DrawPicHandle(pic, &box);
        QDReleaseResource(pic);
    }
    UnsetGWorldContainer(&loadPort);
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             rect, GetCWindowPortRect(gWind), 0, 0);
    ScreenEndScene();
}

/* ShowAmbrosiaLogo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab070
void ShowAmbrosiaLogo(void)
{
    void *pic;
    Rect *rect;
    Rect box;

    pic = GetPicture(kAmbrosiaLogoPictID);
    SetGWorldContainer(&loadPort);
    box.top = loadPort.bounds.top;
    box.left = loadPort.bounds.left;
    box.bottom = loadPort.bounds.bottom;
    box.right = loadPort.bounds.right;
    CenterPict(pic, &box);
    RGBForeColor(blackColor);
    PaintRect(&loadPort.bounds);
    DrawPicHandle(pic, &box);
    UnsetGWorldContainer(&loadPort);
    SWSetPortToWindow(gSpriteWorld);
    rect = GetCWindowPortRect(gWind);
    box.top = rect->top;
    box.left = rect->left;
    box.bottom = rect->bottom;
    box.right = rect->right;
    CenterPict(pic, &box);
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    DrawPicHandle(pic, &box);
    if (pic != 0)
        QDReleaseResource(pic);
    FadeScreenIn();
}

/* FadeToBlack. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab180
void FadeToBlack(void)
{
    FadeScreenOut();
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
}
