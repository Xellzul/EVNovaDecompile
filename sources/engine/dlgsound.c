#include "gametypes.h"

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
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



/* DisposeMainScreenSounds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bbb0
// MAC: _DisposeMainScreenSounds i386 0x00016cae ppc 0x0001a960
void DisposeMainScreenSounds(void)
{
    short i;

    for (i = 0; i < 4; i++) {
        if (mainScreenSnd[i] != NULL) {
#if V111
#else
            while (ST_IsSoundPlaying(mainScreenSnd[i]) != 0)
                BusyIdle();
#endif
            ST_HaltSound(mainScreenSnd[i]);
            ST_DisposeSound(mainScreenSnd[i]);
            mainScreenSnd[i] = NULL;
        }
    }
}

/* PlayMainScreenClickSound. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bc20
// MAC: _PlayMainScreenClickSound i386 0x000177af ppc 0x0001b38c
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
// MAC: _LoadMainScreenAnim i386 0x0001781d ppc 0x0001b404
void LoadMainScreenAnim(void)
{
    short spriteID;
    short xTiles;
    short i;
    unsigned long tick;

#ifdef MACBUILD
    if (GetSpriteInfo(0x25e, &spriteID, NULL, &logoFrameSize.width, &logoFrameSize.height,
                       &xTiles, &logoFrameCount)) {
#else
    if (GetSpriteInfo(0x25e, &spriteID, NULL, &gMainPicWidthDelta, &gMainPicHeightDelta,
                       &xTiles, &logoFrameCount)) {
#endif
        if (xTiles != 1)
            crashlog("warning: logo animation image has wrong dimensions");
        CreateGWorldContainerFromPictResource(&logoPort, GetMainDevice(), 0, spriteID);
    }

    for (i = 0; i < 3; i++) {
        if (GetSpriteInfo(0x260 + i, &spriteID, NULL, &slideFrameSize[i].width, &slideFrameSize[i].height,
                           &xTiles, &slideFrameCount[i])) {
            if (xTiles != 1)
                crashlog("warning: button slide animation image has wrong dimensions");
            CreateGWorldContainerFromPictResource(&slidePort[i], GetMainDevice(), 0, spriteID);
        }
        if (inBackground) {
            slideIndex[i] = slideFrameCount[i];
        } else if (i == 0) {
            slideIndex[i] = -1;
        } else {
            slideIndex[i] = (slideIndex[i - 1] - slideFrameCount[i - 1] * 0.5);
        }
    }

    if (inBackground) {
        if (menuRolloverSprite != NULL)
            rolloverIndex = menuRolloverSprite->numFrames - 1;
        else
            rolloverIndex = 0;
        rolloverAlpha = 0x20;
    } else {
        rolloverAlpha = 0;
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
    lastLogoAnimTime = lastSlideAnimTime = tick;
    lastRolloverAnimTime = 0;
}

/* DisposeMainScreenAnim. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bf40
// MAC: _DisposeMainScreenAnim i386 0x00017a81 ppc 0x0001b6ec
void DisposeMainScreenAnim(void)
{
    short i;

    if (logoPort.world != NULL) {
        DisposeGWorldContainer(&logoPort);
        logoPort.world = NULL;
    }
    for (i = 0; i < 3; i++) {
        if (slidePort[i].world) {
            DisposeGWorldContainer(&slidePort[i]);
            slidePort[i].world = NULL;
        }
    }
}



/* MainScreenAnimIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048bfb0
// MAC: _MainScreenAnimIdle i386 0x0001883c ppc 0x0001c414
void MainScreenAnimIdle(void)
{
    short i;
    short oldFrame;

    if (!inBackground) {
        if (ReadTick() >= lastLogoAnimTime + 5) {
            lastLogoAnimTime = ReadTick();
            oldFrame = logoIndex;
            do {
                logoIndex = Rand(logoFrameCount);
            } while (logoIndex == oldFrame);
        }
    }

    if (!inBackground) {
        if (ReadTick() >= lastSlideAnimTime + 1) {
            lastSlideAnimTime = ReadTick();
            for (i = 0; i < 3; i++) {
                if (slideIndex[i] < slideFrameCount[i]) {
                    if (slideIndex[i] == 0)
                        PlaySound(mainScreenSnd[2], 0x32, gSoundVolume, gSoundVolume);
                    slideIndex[i]++;
                    if (slideIndex[i] == slideFrameCount[i]) {
                        PlaySound(mainScreenSnd[3], 0x32, gSoundVolume, gSoundVolume);
                        if (i == 2)
                            MainScreenUpdate(1);
                    }
                }
            }
        }
    }

    MainScreenRolloverIdle();
    MainScreenAnimDraw();
    MainScreenUpdate(2);

    SetPortWindowPort((WindowRef)gWind);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort), GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             &logoDestRect, &logoDestRect, 0, NULL);

    CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
             &rolloverDestRect, &rolloverDestRect, 0, NULL);

    for (i = 0; i < 3; i++) {
        if (slidePort[i].world != NULL && slideIndex[i] < 0x7d00) {
            CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &slideDestRect[i], &slideDestRect[i], 0, NULL);
            if (slideIndex[i] >= slideFrameCount[i])
                slideIndex[i] = 0x7d00;
        }
    }

#ifndef MACBUILD
    ScreenEndScene();
#endif
}
