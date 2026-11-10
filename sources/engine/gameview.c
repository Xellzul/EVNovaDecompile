// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* gameview.c -- centre the 640x480 play area in the window (win 0x00486790). Notes: Research/notes/gameview.md */
#include <stddef.h>

#include "macport.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "render.h"
#include "sound.h"

#define kViewWidth  640
#define kViewHeight 480
#define kViewSlots  5


#define kKeyShift      0x2a
#define kKeyRightShift 0x36
#define kMacKeyShift   MacKey(0x38)
#define kIntroDescID   0x7fff

#ifdef MAC1010
/* a plain function: -inline auto expands each call and the linker strips the copy; the first reference to `table` in the unit */
#define RT3_HELPERS3_IN_UNIT
void RegScramB3(unsigned char x)
{
    static unsigned char table[256];

    table[x]++;
}
#endif

/*
 * Not gameview.c code: the only caller is EnterGameFromMainScreen, the
 * fifth member of the RT3 registration-library scramble family whose other
 * four sit in commdlg.c (0x004860b0-0x004860f0, right before this address).
 * No mac symbol exists for it.
 */

#ifndef MACBUILD
/* RegScramD3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486100
Boolean RegScramD3(char *p)
{
    TimerGetSeconds();
    return *p;
}

/* RegScramE3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486110
Boolean RegScramE3(void)
{
    return 0;
}
#endif

/*
 * Both log calls are stubs in the shipped build, so holding shift only
 * suppresses the briefing.
 */
/* DoAboutBox (mac _DoAboutBox). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486120
// MAC: _DoAboutBox i386 0x00015fb7 ppc 0x00019cc8
void DoAboutBox(void)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    assert(didSpeak == 0 || didSpeak == 1);
#endif
#if V111
#ifdef MACBUILD
    CancelMusicThread();
#endif
#else
    StopIntroMusic(1);
#endif
#ifdef MACBUILD
    if (KeyCheck(kMacKeyShift) && didSpeak == 0 && gPause == 0) {
        CheckSpeechMgr();
#ifdef MAC1010
        SpeakIt("Greetings professor Follkinn. Would you like to play a game?", 1);
#else
        SpeakIt("\pGreetings professor Follkinn. Would you like to play a game?", 1);
#endif
#else
    if ((KeyCheck(kKeyShift) != 0 || KeyCheck(kKeyRightShift) != 0)
        && didSpeak == 0 && gPause == 0) {
        CheckSpeechMgr();
        SpeakIt("Greetings professor Follkinn. Would you like to play a game?", 1);
#endif
        didSpeak = 1;
        return;
    }
#ifdef MACBUILD
    mainScreenBackgroundDirty = 1;
#endif
    LoadDescription(briefingTextPtr, kIntroDescID);
    MungeBriefing(0, -1);
    strcpy(textShowString, briefingTextPtr);
    ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
    mainScreenBackgroundDirty = 1;
#endif
}

/* --- TrackMainScreenButtons (0x004861b0) --- */

/* h first, v second -- NOT a Point, whose v comes first */


#define kButtonDownSnd 0
#define kButtonUpSnd   1
#define kSoundChannel  10

/*
 * Returns the button under the mouse at release, or -1. Each redraw reads the
 * frame rect out of buttonArray[0] even when it draws buttonArray[1], because
 * the two frames share a rect.
 */
/* TrackMainScreenButtons. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004861b0
// MAC: _TrackMainScreenButtons i386 0x000195b9 ppc 0x0001d30c
#ifdef MACBUILD
short TrackMainScreenButtons(Point where)
{
    Rect box;
    Point pt;
    short i;
    short over;
    short hit;

    SWSetPortToWindow(gSpriteWorld);
    for (i = 0; i < kMainButtons; i++)
        SWSetSpriteLocation(menuButtonSprite[i],
                            (menuTopCorner.h + menuButtonPos[i].h),
                            (menuTopCorner.v + menuButtonPos[i].v));
    hit = -1;
    for (i = 0; i < kMainButtons; i++) {
        if (menuButtonSprite[i] != NULL
            && SWRLECollisionWithPoint(menuButtonSprite[i], &where)) {
            hit = i;
            break;
        }
    }
    if (hit != -1) {
        PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
        for (i = 0; i < kMainButtons; i++) {
            if (menuButtonSprite[i] != NULL) {
                box = menuButtonSprite[i]->frameArray[0]->frameRect;
                OffsetRect(&box, menuTopCorner.h + menuButtonPos[i].h,
                           menuTopCorner.v + menuButtonPos[i].v);
                SWSetSpriteLocation(menuButtonSprite[i],
                                    (menuTopCorner.h + menuButtonPos[i].h),
                                    (menuTopCorner.v + menuButtonPos[i].v));
                if (hit == i)
                    BlitPixieRLEDrawProc(menuButtonSprite[i]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                else
                    BlitPixieRLEDrawProc(menuButtonSprite[i]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                ForeColor(blackColor);
                CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                         GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                         &box, &box, 0, NULL);
                QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
            }
        }
        SWSetPortToWindow(gSpriteWorld);
        while (Button()) {
            BusyIdle();
            GetMouse(&pt);
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            MainScreenRolloverIdle();
            MainScreenRolloverDraw();
            CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            over = -1;
            for (i = 0; i < kMainButtons; i++) {
                if (menuButtonSprite[i] != NULL
                    && SWRLECollisionWithPoint(menuButtonSprite[i], &pt)) {
                    over = i;
                    break;
                }
            }
            if (over != hit) {
                if (over <= kMainButtons - 1 && over >= 0) {
                    PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                    box = menuButtonSprite[over]->frameArray[0]->frameRect;
                    OffsetRect(&box, menuTopCorner.h + menuButtonPos[over].h,
                               menuTopCorner.v + menuButtonPos[over].v);
                    SWSetSpriteLocation(menuButtonSprite[over],
                                        (menuTopCorner.h + menuButtonPos[over].h),
                                        (menuTopCorner.v + menuButtonPos[over].v));
                    BlitPixieRLEDrawProc(menuButtonSprite[over]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                    ForeColor(blackColor);
                    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                             &box, &box, 0, NULL);
                    QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
                } else {
                    PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                }
                if (hit <= kMainButtons - 1 && hit >= 0) {
                    box = menuButtonSprite[hit]->frameArray[0]->frameRect;
                    OffsetRect(&box, menuTopCorner.h + menuButtonPos[hit].h,
                               menuTopCorner.v + menuButtonPos[hit].v);
                    SWSetSpriteLocation(menuButtonSprite[hit],
                                        (menuTopCorner.h + menuButtonPos[hit].h),
                                        (menuTopCorner.v + menuButtonPos[hit].v));
                    BlitPixieRLEDrawProc(menuButtonSprite[hit]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                    ForeColor(blackColor);
                    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                             &box, &box, 0, NULL);
                    QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
                }
            }
            hit = over;
        }
    }
    if (hit != -1)
        PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
    return hit;
}

#else
short TrackMainScreenButtons(Point where)
{
    Rect box;
    Point pt;
    Rect *fr;
    Rect *fr2;
    short i;
    short hit;
    short over;

    SWSetPortToWindow(gSpriteWorld);
    for (i = 0; i < kMainButtons; i++)
        SWSetSpriteLocation(menuButtonSprite[i],
                            (menuButtonPos[i].h + menuTopCorner.h),
                            (menuButtonPos[i].v + menuTopCorner.v));
    hit = -1;
    for (i = 0; i < kMainButtons; i++) {
        if (menuButtonSprite[i] != NULL
            && SWRLECollisionWithPoint(menuButtonSprite[i], &where)) {
            hit = i;
            break;
        }
    }
    if (hit != -1) {
        PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
        for (over = 0; over < kMainButtons; over++) {
            if (menuButtonSprite[over] != NULL) {
                fr = &menuButtonSprite[over]->frameArray[0]->frameRect;
                box.top = fr->top;
                box.left = fr->left;
                box.bottom = fr->bottom;
                box.right = fr->right;
                OffsetRect(&box, menuButtonPos[over].h + menuTopCorner.h,
                           menuButtonPos[over].v + menuTopCorner.v);
                SWSetSpriteLocation(menuButtonSprite[over],
                                    (menuButtonPos[over].h + menuTopCorner.h),
                                    (menuButtonPos[over].v + menuTopCorner.v));
                if (hit == over)
                    BlitPixieRLEDrawProc(menuButtonSprite[over]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                else
                    BlitPixieRLEDrawProc(menuButtonSprite[over]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                ForeColor(blackColor);
                CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                         GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                         &box, &box, 0, NULL);
                ScreenEndScene();
            }
        }
        SWSetPortToWindow(gSpriteWorld);
        while (Button()) {
            BusyIdle();
            HandleIntroMusic(0);
            GetGlobalMouse(&pt);
            CopyBits((BitMapPtr)GetGWCPixMap(&loadPort),
                     GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            MainScreenRolloverIdle();
            MainScreenRolloverDraw();
            CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            over = -1;
            for (i = 0; i < kMainButtons; i++) {
                if (menuButtonSprite[i] != NULL
                    && SWRLECollisionWithPoint(menuButtonSprite[i], &pt)) {
                    over = i;
                    break;
                }
            }
            if (over != hit) {
                if (over <= kMainButtons - 1 && over >= 0) {
                    PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                    fr2 = &menuButtonSprite[over]->frameArray[0]->frameRect;
                    box.top = fr2->top;
                    box.left = fr2->left;
                    box.bottom = fr2->bottom;
                    box.right = fr2->right;
                    OffsetRect(&box, menuButtonPos[over].h + menuTopCorner.h,
                               menuButtonPos[over].v + menuTopCorner.v);
                    SWSetSpriteLocation(menuButtonSprite[over],
                                        (menuButtonPos[over].h + menuTopCorner.h),
                                        (menuButtonPos[over].v + menuTopCorner.v));
                    BlitPixieRLEDrawProc(menuButtonSprite[over]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                    ForeColor(blackColor);
                    CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                             &box, &box, 0, NULL);
                    ScreenEndScene();
                } else {
                    PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                }
                if (hit <= kMainButtons - 1 && hit >= 0) {
                    fr = &menuButtonSprite[hit]->frameArray[0]->frameRect;
                    box.top = fr->top;
                    box.left = fr->left;
                    box.bottom = fr->bottom;
                    box.right = fr->right;
                    OffsetRect(&box, menuButtonPos[hit].h + menuTopCorner.h,
                               menuButtonPos[hit].v + menuTopCorner.v);
                    SWSetSpriteLocation(menuButtonSprite[hit],
                                        (menuButtonPos[hit].h + menuTopCorner.h),
                                        (menuButtonPos[hit].v + menuTopCorner.v));
                    BlitPixieRLEDrawProc(menuButtonSprite[hit]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                    ForeColor(blackColor);
                    CopyBits(GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                             &box, &box, 0, NULL);
                    ScreenEndScene();
                }
            }
            hit = over;
        }
    }
    if (hit != -1)
        PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
    return hit;
}
#endif

/* InitMainScreen (mac _InitMainScreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486790
// MAC: _InitMainScreen i386 0x00016071 ppc 0x00019d9c
void InitMainScreen(void)
{
#ifdef MACBUILD
    short i;

#endif
    didSpeak = 0;
    onPlanet = 0;
    doMainScreenAnim = 1;

    standardPicRect.left = (GetCWindowPortRect((WindowRef)gWind)->left
                      + GetCWindowPortRect((WindowRef)gWind)->right) / 2 - kViewWidth / 2;
    standardPicRect.top = (GetCWindowPortRect((WindowRef)gWind)->top
                     + GetCWindowPortRect((WindowRef)gWind)->bottom) / 2 - kViewHeight / 2;
    standardPicRect.right = standardPicRect.left + kViewWidth;
    standardPicRect.bottom = standardPicRect.top + kViewHeight;

#ifdef MACBUILD
    introTextID = -1;
    for (i = 0; i < 4; i++)
        introPictID[i] = -1;
#else
    gViewSlot[0] = -1;
    gViewSlot[1] = -1;
    gViewSlot[2] = -1;
    gViewSlot[3] = -1;
    gViewSlot[4] = -1;
#endif

    creatingPilotFile = 0;
    majorPrefsChange = 0;
}
