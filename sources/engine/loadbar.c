// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* loadbar.c -- the startup progress bar (win 0x004ab1b0..0x004ab3d0). Notes: Research/notes/loadbar.md */
#include <stdlib.h>
#include "macport.h"
#include "burger.h"
#include "resources.h"
#include "game.h"
#include "render.h"
#include "dialogs.h"
#include "sound.h"
#include "missions.h"
#include "player.h"
#ifdef MACBUILD
extern GWorldContainer progressLoadPort;
#endif

/* the bar's rect comes out of the colr resource */

/* InitIntroProgressBar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab1b0
// MAC: _InitIntroProgressBar i386 0x00061263 ppc 0x0006904c
void InitIntroProgressBar(void)
{
    Rect line;
    Rect fill;
    short midH;
    short midV;
#ifndef MACBUILD
    RGBColor *barColor;
#endif

    progressMax = CountResources(kShipType);
    progressCount = 0;

    midH = ((GetCWindowPortRect((WindowRef)gWind)->left
                    + GetCWindowPortRect((WindowRef)gWind)->right) / 2);
    midV = ((GetCWindowPortRect((WindowRef)gWind)->top
                    + GetCWindowPortRect((WindowRef)gWind)->bottom) / 2);

#ifdef MACBUILD
    progressRect = progressDefRect;
    OffsetRect(&progressRect, midH, midV);

    line = progressRect;
    InsetRect(&line, 0, (line.bottom - line.top) / 2);

    while (line.bottom <= progressRect.bottom) {
        RGBForeColor(&progressColor[2]);
        FrameRect(&line);
        ForeColor(blackColor);
        fill = line;
        InsetRect(&fill, 1, 1);
        PaintRect(&fill);
        InsetRect(&line, 0, -1);
#ifdef MAC1010
        HandleIntroMusic(0);
#endif
        Delay(2, &tix);
    }
#else
    progressRect.top = progressDefRect.top;
    progressRect.left = progressDefRect.left;
    progressRect.bottom = progressDefRect.bottom;
    progressRect.right = progressDefRect.right;
    OffsetRect(&progressRect, midH, midV);

    line.top = progressRect.top;
    line.left = progressRect.left;
    line.bottom = progressRect.bottom;
    line.right = progressRect.right;
    InsetRect(&line, 0, (line.bottom - line.top) / 2);

    if (line.bottom <= progressRect.bottom) {
        barColor = &progressColor[2];
        do {
        RGBForeColor(barColor);
        FrameRect(&line);
        ForeColor(blackColor);
        fill.top = line.top;
        fill.left = line.left;
        fill.bottom = line.bottom;
        fill.right = line.right;
        InsetRect(&fill, 1, 1);
        PaintRect(&fill);
        InsetRect(&line, 0, -1);
        HandleIntroMusic(0);
        Delay(2, &tix);
        } while (line.bottom <= progressRect.bottom);
    }
#endif
    DrawIntroProgressBar();
}

/* DisposeIntroProgressBar (mac _DisposeIntroProgressBar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab3a0
// MAC: _DisposeIntroProgressBar i386 0x000613ef ppc 0x00069200
void DisposeIntroProgressBar(void)
{
}

/* DeltaIntroProgressBar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab3b0
// MAC: _DeltaIntroProgressBar i386 0x000613f4 ppc 0x00069204
void DeltaIntroProgressBar(double amount)
{
    progressCount = progressCount + amount;
    DrawIntroProgressBar();
}

/* DrawIntroProgressBar. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab3d0
// MAC: _DrawIntroProgressBar i386 0x000610ba ppc 0x00068e7c
void DrawIntroProgressBar(void)
{
    Rect box;
#ifdef MAC1010
    Rect bar;
    Rect start;

    start = progressLoadPort.bounds;
    bar = start;
    bar.right = (progressCount / progressMax * 198.0 + bar.left);
    if (bar.right > progressLoadPort.bounds.right)
        bar.right = progressLoadPort.bounds.right;
    box = start;
#elif defined(MACBUILD)
    box = progressLoadPort.bounds;
#else
    box.left = gLoadBarBase.left;
    box.top = gLoadBarBase.top;
    box.bottom = gLoadBarBase.bottom;
    box.right = gLoadBarBase.right;
#endif
    box.right = box.left + 30;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    RGBForeColor(&progressColor[2]);
    FrameRect(&progressRect);

#ifdef MACBUILD
    box = progressRect;
    InsetRect(&box, 1, 1);

    box.right = (box.left + progressCount / progressMax * 198.0);
#else
    box.top = progressRect.top;
    box.left = progressRect.left;
    box.bottom = progressRect.bottom;
    box.right = progressRect.right;
    InsetRect(&box, 1, 1);

    box.right = (progressCount / progressMax * 198.0 + box.left);
#endif
    if (box.right >= progressRect.right)
        box.right = progressRect.right - 1;

    RGBForeColor(&progressColor[0]);
    InsetRect(&box, 1, 1);
    PaintRect(&box);
    RGBForeColor(&progressColor[1]);
    InsetRect(&box, -1, -1);
    FrameRect(&box);
    ForeColor(blackColor);
    box.left = box.right;
    box.right = progressRect.right - 1;
    PaintRect(&box);

    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             &progressRect, &progressRect, 0, 0);
#ifndef MACBUILD
    ScreenEndScene();
#endif
}

/* not linked, not verified against the exe (mac _ShowScrollingText) */
#define kStrListType        0x53545223          /* 'STR#' */
#define kQDWhiteColor       30
#define kQDBlackColor       33
#define kMaxScrollLines     128
#define kScrollLineHeight   30
#define kScrollFadeRange    32
#define kScanReturn         MacKey(0x24)
#define kScanSpace          MacKey(0x31)
#define kScanDown           MacKey(0x7d)
#define kScanUp             MacKey(0x7e)

/* ShowScrollingText (mac _ShowScrollingText): mac only. */
// MAC: _ShowScrollingText i386 0x0006140f ppc 0x0006921c
unsigned char ShowScrollingText(short strListID)
{
    Handle strs;
    short count;
    Rect view;
    Rect box;
    Rect screen;
    RGBColor shade;
    unsigned char text[256];
    char *line[kMaxScrollLines];
    short offset;
    short width;
    short i;
    short y;
    short maxWidth;
    short fade;
    uint32_t startTicks;
    short delay;
    unsigned char shown;
    unsigned char finished;

    strs = GetResource(kStrListType, strListID);
    if (strs == 0)
        return 0;
    HLock(strs);
    memmove(&count, *strs, 2);
    HUnlock(strs);
    HPurge(strs);
    ReleaseResource(strs);
    if (count <= 0)
        return 0;

    SWSetPortToWindow(gSpriteWorld);
    (ForeColor)(kQDBlackColor);
    screen = *GetCWindowPortRect((WindowRef)gWind);
    screen.top -= 20;
    PaintRect(&screen);
    PaintRect(&screen);
    TextFont(20);
    TextSize(24);

    if (count > kMaxScrollLines)
        count = kMaxScrollLines;
    maxWidth = 0;
    for (i = 0; i < count; i++) {
        GetIndString(text, strListID, i + 1);
        width = StringWidth(text);
        if (maxWidth < width)
            maxWidth = width;
        line[i] = 0;
        line[i] = (char *)NewPtr(256);
        if (line[i] != 0) {
            if (text[1] == '<' && text[2] == 'R' && text[3] == 'E' && text[4] == 'G'
                && text[5] == '>') {
                strcpy((char *)text, "   ");
                strcat((char *)text, RT3_GetDisplayName());
                CtoPstr((char *)text);
            }
            strncpy(line[i], (char *)text, 250);
        }
    }
    if (maxWidth < 32)
        maxWidth = 32;

    box.left = box.right = (GetCWindowPortRect((WindowRef)gWind)->left
                            + GetCWindowPortRect((WindowRef)gWind)->right) / 2;
    box.top = box.bottom = (GetCWindowPortRect((WindowRef)gWind)->top
                            + GetCWindowPortRect((WindowRef)gWind)->bottom) / 2;
    InsetRect(&box, -8 - maxWidth / 2, -160);
    OffsetRect(&box, -10, 0);
    view = box;

    SetGWorldContainer(&loadPort);
    TextFont(20);
    TextSize(24);
    (ForeColor)(kQDBlackColor);
    PaintRect(&loadPort.bounds);

    finished = 1;
    for (offset = 0; offset <= count * kScrollLineHeight + (view.bottom - view.top); offset++) {
        startTicks = TickCount();
        for (i = 0; i < count; i++) {
            y = view.bottom - offset + i * kScrollLineHeight;
            if (y < box.top || y > box.bottom + 24)
                continue;
            (ForeColor)(kQDWhiteColor);
            shown = 1;
            if (y - box.bottom >= -kScrollFadeRange && y - box.bottom <= kScrollFadeRange) {
                shown = 0;
                if (y < box.bottom) {
                    fade = abs(y - box.bottom) / 32.0 * 65535.0;
                    shade.green = shade.blue = shade.red = fade;
                    RGBForeColor(&shade);
                    shown = 1;
                }
            }
            if (y - box.top >= -8 && y - box.top <= 56) {
                if (y <= box.top + 24)
                    continue;
                fade = abs(y - box.top - 24) / 32.0 * 65535.0;
                shade.green = shade.blue = shade.red = fade;
                RGBForeColor(&shade);
            }
            if (!shown)
                continue;
            MoveTo(view.left + 10, y);
            DrawString((unsigned char *)line[i]);
        }
        SWSetPortToWindow(gSpriteWorld);
        (ForeColor)(kQDBlackColor);
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)), &view, &box, 0, 0);
        SetGWorldContainer(&loadPort);
        (ForeColor)(kQDBlackColor);
        PaintRect(&view);
        QDFlushPortBuffer(GetWindowPort(gWind), 0);
        QuickKeyCheckHandler();
        BusyIdle();
        if (Button() || QuickKeyCheck(kScanReturn) || QuickKeyCheck(kScanSpace)) {
            finished = 0;
            break;
        }
        if (QuickKeyCheck(kScanDown))
            delay = 4;
        else if (QuickKeyCheck(kScanUp))
            delay = 0;
        else
            delay = 2;
        while (TickCount() - startTicks < delay)
            ;
    }

    for (i = 0; i < count; i++) {
        if (line[i] != 0)
            DisposePtr(line[i]);
    }
    SWSetPortToWindow(gSpriteWorld);
    return finished;
}
