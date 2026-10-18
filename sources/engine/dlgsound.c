#include "gametypes.h"

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* dlgsound.c -- the dialog sound bank's stop-all and its two-part chime (win 0x0048bbb0 and 0x0048bc20). Notes: Research/notes/dlgsound.md */
#include <stddef.h>

#include "macport.h"
#include "burger.h"
#include "render.h"
#include "game.h"
#include "gameutil.h"
#include "sound.h"
#include "crashlog.h"
#include "dialogs.h"



/* StopDialogSounds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bbb0
void StopDialogSounds(void)
{
    short i;

    for (i = 0; i < 4; i++) {
        if (mainScreenSnd[i] != NULL) {
            while (ST_IsSoundPlaying(mainScreenSnd[i]) != 0)
                BusyIdle();
            ST_HaltSound(mainScreenSnd[i]);
            ST_DisposeSound(mainScreenSnd[i]);
            mainScreenSnd[i] = NULL;
        }
    }
}

/* PlayMainScreenClickSound. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bc20
void PlayMainScreenClickSound(void)
{
    PlaySound(mainScreenSnd[0], 10, gSoundVolume, gSoundVolume);
    while (ST_IsSoundPlaying(mainScreenSnd[0]) != 0)
        BusyIdle();
    PlaySound(mainScreenSnd[1], 10, gSoundVolume, gSoundVolume);
}

#include "prototypes.h"   /* inBackground */




/* LoadMainScreenAnim (mac _LoadMainScreenAnim). STATE: MATCH 100.0%, exe 165 / obj 165
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0048bc90
void LoadMainScreenAnim(void)
{
    short spriteID;
    short xTiles;
    short i;
    unsigned long tick;

    if (GetSpriteInfo(0x25e, &spriteID, NULL, &gMainPicWidthDelta, &gMainPicHeightDelta,
                       &xTiles, &gLogoAnimEnd)) {
        if (xTiles != 1)
            crashlog("warning: logo animation image has wrong dimensions");
        CreateGWorldContainerFromPictResource(&gDlgWorld, GetMainDevice(), 0, spriteID);
    }

    for (i = 0; i < 3; i++) {
        if (GetSpriteInfo(0x260 + i, &spriteID, NULL, &gAnimDelta[i].width, &gAnimDelta[i].height,
                           &xTiles, &gAnimEnd[i])) {
            if (xTiles != 1)
                crashlog("warning: button slide animation image has wrong dimensions");
            CreateGWorldContainerFromPictResource(&gDlgWorlds[i], GetMainDevice(), 0, spriteID);
        }
        if (inBackground) {
            gAnimStep[i] = gAnimEnd[i];
        } else if (i == 0) {
            gAnimStep[i] = -1;
        } else {
            gAnimStep[i] = (gAnimStep[i - 1] - gAnimEnd[i - 1] * 0.5);
        }
    }

    if (inBackground) {
        if (menuRolloverSprite != NULL)
            gHilitedButton = menuRolloverSprite->numFrames - 1;
        else
            gHilitedButton = 0;
        gHiliteLevel = 0x20;
    } else {
        gHiliteLevel = 0;
    }

    logoDestRect.left = menuTopCorner.h + logoDestPoint.h;
    logoDestRect.top = menuTopCorner.v + logoDestPoint.v;
    rolloverDestRect.left = menuTopCorner.h + rolloverDestPoint.h;
    rolloverDestRect.top = menuTopCorner.v + rolloverDestPoint.v;
    for (i = 0; i < 3; i++) {
        slideDestRect[i].left = menuTopCorner.h + slideDestPoint[i].h;
        slideDestRect[i].top = menuTopCorner.v + slideDestPoint[i].v;
    }

    tick = ReadTick();
    gLastLogoAnimTick = gLastSlideAnimTick = tick;
    gHiliteLastTick = 0;
}

/* DisposeDialogGWorlds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bf40
void DisposeDialogGWorlds(void)
{
    short i;

    if (gDlgWorld.world != NULL) {
        DisposeGWorldContainer(&gDlgWorld);
        gDlgWorld.world = NULL;
    }
    for (i = 0; i < 3; i++) {
        if (gDlgWorlds[i].world) {
            DisposeGWorldContainer(&gDlgWorlds[i]);
            gDlgWorlds[i].world = NULL;
        }
    }
}



/* TrackMainScreenAnim. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bfb0
void TrackMainScreenAnim(void)
{
    short i;
    short oldFrame;

    if (!inBackground) {
        if (ReadTick() >= gLastLogoAnimTick + 5) {
            gLastLogoAnimTick = ReadTick();
            oldFrame = gMainPicFrame;
            do {
                gMainPicFrame = Rand(gLogoAnimEnd);
            } while (gMainPicFrame == oldFrame);
        }
    }

    if (!inBackground) {
        if (ReadTick() >= gLastSlideAnimTick + 1) {
            gLastSlideAnimTick = ReadTick();
            for (i = 0; i < 3; i++) {
                if (gAnimStep[i] < gAnimEnd[i]) {
                    if (gAnimStep[i] == 0)
                        PlaySound(mainScreenSnd[2], 0x32, gSoundVolume, gSoundVolume);
                    gAnimStep[i]++;
                    if (gAnimStep[i] == gAnimEnd[i]) {
                        PlaySound(mainScreenSnd[3], 0x32, gSoundVolume, gSoundVolume);
                        if (i == 2)
                            MainScreenUpdate(1);
                    }
                }
            }
        }
    }

    TrackMainScreenHilite();
    MainScreenAnimDraw();
    MainScreenUpdate(2);

    SetPortWindowPort(gWind);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort), GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &logoDestRect, &logoDestRect, 0, NULL);

    CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &rolloverDestRect, &rolloverDestRect, 0, NULL);

    for (i = 0; i < 3; i++) {
        if (gDlgWorlds[i].world != NULL && gAnimStep[i] < 0x7d00) {
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     &slideDestRect[i], &slideDestRect[i], 0, NULL);
            if (gAnimStep[i] >= gAnimEnd[i])
                gAnimStep[i] = 0x7d00;
        }
    }

    ScreenEndScene();
}
