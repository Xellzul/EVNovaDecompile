// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* showintroscreen.c -- ShowIntroScreen, MainScreenIdle, LoadMainScreenSounds; exe 0x0048adc0..0x0048b250. Notes: Research/notes/mainscreen.md */

#include <stddef.h>
#include "prototypes.h"
#define kMainScreenSoundCount 4
#define kFirstMainScreenSound 0x258
#include "resources.h"
#include "shipobj.h"
#include "gametypes.h"
#include "gameconst.h"
#include "macport.h"
#include "dialogs.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "burger.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "crashlog.h"
#include "game.h"

/*
 * gViewSlot itself is gameview.c's array at 0x007d1f42 - do NOT re-derive a
 * second name/array at 0x007d1f44 for the loop's [ebp*2+0x7d1f44] reads, that
 * is gViewSlot[i+1]. The exe's two "push 0" are CopyBits' own trailing
 * mode/maskRgn arguments, not a separate zeroed local.
 */
#define kIntroPictSlots 4



/* ShowIntroScreen. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048adc0
void ShowIntroScreen(void)
{
    short i;
    char pictSkipped;
    char hit;
    void *picHandle;
    void *snd;
    Rect rect;
    Point pt;
    unsigned long startTick;

    HandleIntroMusic(1);
    FadeScreenOut();

    pictSkipped = 0;
    if (KeyCheck(keyPrefs.pause))
        pictSkipped = 1;

    if (!windowedPref)
        HideTheCursor();

    for (i = 0; i < kIntroPictSlots; i++) {
        if ((gViewSlot + 1)[i] > 0)
            picHandle = GetPicture((gViewSlot + 1)[i]);
        else
            picHandle = NULL;

        SetGWorldContainer(&loadPort);
        RGBForeColor(blackColor);
        PaintRect(&loadPort.bounds);

        rect.top = loadPort.bounds.top;
        rect.left = loadPort.bounds.left;
        rect.bottom = loadPort.bounds.bottom;
        rect.right = loadPort.bounds.right;

        if (picHandle != NULL) {
            CenterPict(picHandle, &rect);
            DrawPicHandle(picHandle, &rect);
            QDReleaseResource(picHandle);
        }

        UnsetGWorldContainer(&loadPort);

        SWSetPortToWindow(gSpriteWorld);
        RGBForeColor(blackColor);
        PaintRect(GetCWindowPortRect(gWind));
        CopyBits(GetGWCPixMap(&loadPort), GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                 &rect, &rect, 0, NULL);
        ScreenEndScene();

        SWSetPortToWindow(gSpriteWorld);

        if ((gViewSlot + 1)[i] > 0) {
            snd = LoadSoundResource(0x7533);
            if (snd != NULL)
                PlaySound(snd, 0xa, gSoundVolume, gSoundVolume);

            HandleIntroMusic(1);
            FadeScreenIn();

            while (Button() || KeyCheck(0x1c) || KeyCheck(0x39) || KeyCheck(keyPrefs.pause)) {
                BusyIdle();
                HandleIntroMusic(0);
            }

            hit = 0;
            startTick = ReadTick();

            while (ReadTick() - startTick < (gViewSlotDuration[i] * 0x3c)
                   && !hit && !KeyCheck(0x1c) && !KeyCheck(0x39) && !pictSkipped) {
                BusyIdle();
                HandleIntroMusic(0);
                if (Button()) {
                    Rect *winRect = GetCWindowPortRect(gWind);
                    rect.top = winRect->top;
                    rect.left = winRect->left;
                    rect.bottom = winRect->bottom;
                    rect.right = winRect->right;
                    GetGlobalMouse(&pt);
                    if (PtInRect(pt.h, pt.v, &rect)) {
                        hit = 1;
                    } else {
                        while (Button()) {
                            BusyIdle();
                            HandleIntroMusic(0);
                            if (ReadTick() - startTick > (gViewSlotDuration[i] * 0x3c))
                                break;
                            if (KeyCheck(0x1c) || KeyCheck(0x39))
                                break;
                            if (KeyCheck(keyPrefs.pause)) {
                                pictSkipped = 1;
                                break;
                            }
                        }
                    }
                }
                if (KeyCheck(keyPrefs.pause))
                    pictSkipped = 1;
            }

            if (snd != NULL) {
                ST_HaltSound(snd);
                ST_DisposeSound(snd);
            }

            HandleIntroMusic(1);
            FadeScreenOut();

            if (pictSkipped)
                break;
        }
    }

    HandleIntroMusic(1);
    if (!pictSkipped) {
        StopIntroMusic(1);
        if (gViewSlot[0] != -1) {
            ShowTheCursor();
            showingIntroText = 1;
            LoadDescription(briefingTextPtr, gViewSlot[0]);
            MungeBriefing(0, -1);
            strcpy(gTextShowString, briefingTextPtr);
            ShowTextDialog(0, 0, 1);
            showingIntroText = 0;
        }
    }

    FlushEvents(3, 0);
    FlushEvents(5, 0);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
}

/* the main screen's idle tick */
/* MainScreenIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b230
void MainScreenIdle(void)
{
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    HandleIntroMusic(1);
    FadeScreenIn();
}

/* LoadMainScreenSounds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048b250
void LoadMainScreenSounds(void)
{
    short i;

    for (i = 0; i < kMainScreenSoundCount; i++)
        mainScreenSnd[i] = LoadSoundResource((i + kFirstMainScreenSound));
}
