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
// MAC: _ShowSplashScreen i386 0x00060e68 ppc 0x00068bfc
void ShowSplashScreen(void)
{
    void *pic;
    Rect *rect;
    Rect box;

    pic = GetPicture(kSplashPictID);
    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    rect = &loadPort.bounds;
    PaintRect(rect);
#ifdef MACBUILD
    box = loadPort.bounds;
#else
    box.top = loadPort.bounds.top;
    box.left = loadPort.bounds.left;
    box.bottom = loadPort.bounds.bottom;
    box.right = loadPort.bounds.right;
#endif
    if (pic != 0) {
        CenterPict(pic, &box);
        DrawPicHandle(pic, &box);
#ifdef MACBUILD
        HPurge(pic);
#endif
        QDReleaseResource(pic);
    }
    UnsetGWorldContainer(&loadPort);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
#ifdef MAC1010
             &loadPort.bounds, GetCWindowPortRect(gWind), 0, 0);
#else
             rect, GetCWindowPortRect(gWind), 0, 0);
#endif
#ifndef MACBUILD
    ScreenEndScene();
#endif
}

#ifdef MACBUILD
/* HideSplashScreen (mac _HideSplashScreen): mac only. */
// MAC: _HideSplashScreen i386 0x00060f76 ppc 0x00068d04
void HideSplashScreen(void)
{
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
}
#endif

/* ShowAmbrosiaLogo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab070
// MAC: _ShowAmbrosiaLogo i386 0x00060fb0 ppc 0x00068d54
void ShowAmbrosiaLogo(void)
{
    void *pic;
    Rect *rect;
    Rect box;

    pic = GetPicture(kAmbrosiaLogoPictID);
    SetGWorldContainer(&loadPort);
#ifdef MACBUILD
    box = loadPort.bounds;
#else
    box.top = loadPort.bounds.top;
    box.left = loadPort.bounds.left;
    box.bottom = loadPort.bounds.bottom;
    box.right = loadPort.bounds.right;
#endif
    CenterPict(pic, &box);
    ForeColor(blackColor);
    PaintRect(&loadPort.bounds);
    DrawPicHandle(pic, &box);
    UnsetGWorldContainer(&loadPort);
    SWSetPortToWindow(gSpriteWorld);
#ifdef MACBUILD
    box = *GetCWindowPortRect(gWind);
#else
    rect = GetCWindowPortRect(gWind);
    box.top = rect->top;
    box.left = rect->left;
    box.bottom = rect->bottom;
    box.right = rect->right;
#endif
    CenterPict(pic, &box);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    DrawPicHandle(pic, &box);
    if (pic != 0)
        QDReleaseResource(pic);
#if V111
#else
    FadeScreenIn();
#endif
}

/* HideAmbrosiaLogo. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab180
// MAC: _HideAmbrosiaLogo i386 0x0006108f ppc 0x00068e40
void HideAmbrosiaLogo(void)
{
#if V111
#else
    FadeScreenOut();
#endif
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
}
