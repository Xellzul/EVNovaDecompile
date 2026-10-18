// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
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
#define kIntroDescID   0x7fff

/*
 * Not gameview.c code: the only caller is EnterGameFromMainScreen, the
 * fifth member of the RT3 registration-library scramble family whose other
 * four sit in commdlg.c (0x004860b0-0x004860f0, right before this address).
 * No mac symbol exists for it.
 */

/* RegScramD3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486100
char RegScramD3(char *p)
{
    TimerGetSeconds();
    return *p;
}

/* RegScramE3. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486110
char RegScramE3(void)
{
    return 0;
}

/*
 * Both log calls are stubs in the shipped build, so holding shift only
 * suppresses the briefing.
 */
/* DoAboutBox (mac _DoAboutBox). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486120
void DoAboutBox(void)
{
    StopIntroMusic(1);
    if ((KeyCheck(kKeyShift) != 0 || KeyCheck(kKeyRightShift) != 0)
        && didSpeak == 0 && gPause == 0) {
        CheckSpeechMgr();
        SpeakIt("Greetings professor Follkinn. Would you like to play a game?", 1);
        didSpeak = 1;
        return;
    }
    LoadDescription(briefingTextPtr, kIntroDescID);
    MungeBriefing(0, -1);
    strcpy(gTextShowString, briefingTextPtr);
    ShowTextDialog(0, 0, 1);
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
        SWSetSpriteLocation(gMainButtonSprites[i],
                            (menuButtonPos[i].h + menuTopCorner.h),
                            (menuButtonPos[i].v + menuTopCorner.v));
    hit = -1;
    for (i = 0; i < kMainButtons; i++) {
        if (gMainButtonSprites[i] != NULL
            && SWRLECollisionWithPoint(gMainButtonSprites[i], &where)) {
            hit = i;
            break;
        }
    }
    if (hit != -1) {
        PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                  gSoundVolume, gSoundVolume);
        for (over = 0; over < kMainButtons; over++) {
            if (gMainButtonSprites[over] != NULL) {
                fr = &gMainButtonSprites[over]->frameArray[0]->frameRect;
                box.top = fr->top;
                box.left = fr->left;
                box.bottom = fr->bottom;
                box.right = fr->right;
                OffsetRect(&box, menuButtonPos[over].h + menuTopCorner.h,
                           menuButtonPos[over].v + menuTopCorner.v);
                SWSetSpriteLocation(gMainButtonSprites[over],
                                    (menuButtonPos[over].h + menuTopCorner.h),
                                    (menuButtonPos[over].v + menuTopCorner.v));
                if (hit == over)
                    BlitPixieRLEDrawProc(gMainButtonSprites[over]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                else
                    BlitPixieRLEDrawProc(gMainButtonSprites[over]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                RGBForeColor(blackColor);
                CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
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
            CopyBits(GetGWCPixMap(&loadPort),
                     GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            TrackMainScreenHilite();
            MainScreenRolloverDraw();
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     &rolloverDestRect, &rolloverDestRect, 0, NULL);
            over = -1;
            for (i = 0; i < kMainButtons; i++) {
                if (gMainButtonSprites[i] != NULL
                    && SWRLECollisionWithPoint(gMainButtonSprites[i], &pt)) {
                    over = i;
                    break;
                }
            }
            if (over != hit) {
                if (over <= kMainButtons - 1 && over >= 0) {
                    PlaySound(mainScreenSnd[kButtonDownSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                    fr2 = &gMainButtonSprites[over]->frameArray[0]->frameRect;
                    box.top = fr2->top;
                    box.left = fr2->left;
                    box.bottom = fr2->bottom;
                    box.right = fr2->right;
                    OffsetRect(&box, menuButtonPos[over].h + menuTopCorner.h,
                               menuButtonPos[over].v + menuTopCorner.v);
                    SWSetSpriteLocation(gMainButtonSprites[over],
                                        (menuButtonPos[over].h + menuTopCorner.h),
                                        (menuButtonPos[over].v + menuTopCorner.v));
                    BlitPixieRLEDrawProc(gMainButtonSprites[over]->frameArray[1],
                                         gSpriteWorld->workFrameP, &box, &box);
                    RGBForeColor(blackColor);
                    CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                             &box, &box, 0, NULL);
                    ScreenEndScene();
                } else {
                    PlaySound(mainScreenSnd[kButtonUpSnd], kSoundChannel,
                              gSoundVolume, gSoundVolume);
                }
                if (hit <= kMainButtons - 1 && hit >= 0) {
                    fr = &gMainButtonSprites[hit]->frameArray[0]->frameRect;
                    box.top = fr->top;
                    box.left = fr->left;
                    box.bottom = fr->bottom;
                    box.right = fr->right;
                    OffsetRect(&box, menuButtonPos[hit].h + menuTopCorner.h,
                               menuButtonPos[hit].v + menuTopCorner.v);
                    SWSetSpriteLocation(gMainButtonSprites[hit],
                                        (menuButtonPos[hit].h + menuTopCorner.h),
                                        (menuButtonPos[hit].v + menuTopCorner.v));
                    BlitPixieRLEDrawProc(gMainButtonSprites[hit]->frameArray[0],
                                         gSpriteWorld->workFrameP, &box, &box);
                    RGBForeColor(blackColor);
                    CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
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

/* InitMainScreen (mac _InitMainScreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00486790
void InitMainScreen(void)
{
    didSpeak = 0;
    onPlanet = 0;
    doMainScreenAnim = 1;

    standardPicRect.left = (GetCWindowPortRect(gWind)->left
                      + GetCWindowPortRect(gWind)->right) / 2 - kViewWidth / 2;
    standardPicRect.top = (GetCWindowPortRect(gWind)->top
                     + GetCWindowPortRect(gWind)->bottom) / 2 - kViewHeight / 2;
    standardPicRect.right = standardPicRect.left + kViewWidth;
    standardPicRect.bottom = standardPicRect.top + kViewHeight;

    gViewSlot[0] = -1;
    gViewSlot[1] = -1;
    gViewSlot[2] = -1;
    gViewSlot[3] = -1;
    gViewSlot[4] = -1;

    creatingPilotFile = 0;
    majorPrefsChange = 0;
}
