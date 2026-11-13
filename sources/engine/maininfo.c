// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* maininfo.c -- the main-screen object (tu-map O23, win 0x004873b0..0x00489d70). Notes: Research/notes/maininfo.md */
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "gametypes.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "sound.h"
#include "stellar.h"
#include "burger.h"
#include "crashlog.h"
#include "prototypes.h"
#include "ships.h"
#include "cefix.h"
#include "space.h"
#ifdef MACBUILD
#include "../mac/include/rt3_ev_nova.h"

#define kStrDepth256         17
#define kStrDepthThousands   18
#define kStrResolution       19
#define kStrResolutionTail   20
#endif

/* MainScreenUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004873b0
// MAC: _MainScreenUpdate i386 0x00017c0d ppc 0x0001b8e8
void MainScreenUpdate(short which)
{
    unsigned char text[256];
    Rect box;
    Rect dst;
    Boolean shown[6];
    short i;
    RGBColor *valueColor;
    RGBColor *labelColor;

    if (mainScreenBackgroundDirty != 0) {
        SetupInterfaceBackground();
        MainScreenAnimDraw();
        mainScreenBackgroundDirty = 0;
    }
    SetGWorldContainer(&loadPort);
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
    if (which == 2) {
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                 &rolloverDestRect, &rolloverDestRect, 0, NULL);
        for (i = 0; i < 3; i++) {
            if (slidePort[i].world != NULL && slideIndex[i] < slideFrameCount[i]) {
                CopyBits(GWorldContainerBits(&loadPort),
                         GetPortBitMapForCopyBits(
                             gSpriteWorld->workFrameP->framePort),
                         &slideDestRect[i], &slideDestRect[i], 0, NULL);
            }
        }
    } else {
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                 &gSpriteWorld->workFrameP->frameRect,
                 &gSpriteWorld->workFrameP->frameRect, 0, NULL);
    }
    SetRect(&box, standardPicRect.left + 0x115, standardPicRect.top + 0x10a,
            standardPicRect.left + 0x171, standardPicRect.top + 0x1be);
    valueColor = &menuTextColor[0];
    labelColor = &menuTextColor[1];
    TextFont(menuFontID);
    TextSize(menuTextSize);
    if (creatingPilotFile != 0) {
        RGBForeColor(labelColor);
        GetIndString(text, kGameStrList, 0x113);
        DrawStringCentered(text, (mainMenuCenter.h - 0x96),
                           (mainMenuCenter.h + 0x96),
                           (mainMenuCenter.v + 0x136));
    } else {
        if (gameInProgress) {
            if (IsDying(shipData) == 0) {
                MoveTo((mainMenuCenter.h - 0xbe), (mainMenuCenter.v + 0xfa));
                RGBForeColor(labelColor);
                DrawIndString(kGameStrList, 0xfb);
                MoveTo((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x106));
                RGBForeColor(valueColor);
                DrawString(playerName);
                MoveTo((mainMenuCenter.h - 0xbe), (mainMenuCenter.v + 0x11e));
                RGBForeColor(labelColor);
                DrawIndString(kGameStrList, 0xff);
                MoveTo((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x12a));
                RGBForeColor(valueColor);
                DrawString(shipName);
                MoveTo((mainMenuCenter.h - 0xbe), (mainMenuCenter.v + 0x142));
                RGBForeColor(labelColor);
                DrawIndString(kGameStrList, 0x100);
                MoveTo((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x14e));
                RGBForeColor(valueColor);
                DrawString(shipClass[shipData->type].name);
                MoveTo((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x15a));
                RGBForeColor(valueColor);
                DrawString(shipClass[shipData->type].subtitle);
                RGBForeColor(labelColor);
                MoveTo((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0xfa));
                DrawIndString(kGameStrList, 0x116);
                MoveTo((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x106));
                DrawIndString(kGameStrList, 0x117);
                MoveTo((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x112));
                RGBForeColor(valueColor);
                if (SystemIsInhabited(shipData->curSystem))
                    DrawLegalStatusString(shipData->curSystem);
                else
                    DrawIndString(kGameStrList, 0x18c);
                MoveTo((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0x12a));
                RGBForeColor(labelColor);
                DrawIndString(kGameStrList, 0xfe);
                MoveTo((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x136));
                RGBForeColor(valueColor);
                DrawCombatRatingString();
                MoveTo((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0x14e));
                RGBForeColor(labelColor);
                DrawIndString(kGameStrList, 0xfc);
                MoveTo((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x15a));
                RGBForeColor(valueColor);
                DateToMedString(gameTime, (char *)text);
                CtoPstr((char *)text);
                DrawString(text);
                ForeColor(blackColor);
                i = shipClass[shipData->type].targetPictID;
                if (targetInfoPic[i] != NULL) {
                    SWSetPortToBackground(gSpriteWorld);
                    ForeColor(blackColor);
                    QDGetPictureBounds((PicHandle)targetInfoPic[i], &box);
                    OffsetRect(&box, -box.left, -box.top);
                    OffsetRect(&box,
                               mainMenuCenter.h - (box.right - box.left) / 2,
                               mainMenuCenter.v + 0x118);
                    DrawPicHandle((PicHandle)targetInfoPic[i], &box);
                    SWSetPortToWorkArea(gSpriteWorld);
                    ForeColor(blackColor);
                    CopyBits(GetPortBitMapForCopyBits(
                                 gSpriteWorld->backFrameP->framePort),
                             GetPortBitMapForCopyBits(
                                 gSpriteWorld->workFrameP->framePort),
                             &box, &box, 0x22, NULL);
                }
            } else {
                RGBForeColor(labelColor);
                if (playerName[1] == 'K' && playerName[2] == 'e' &&
                    playerName[3] == 'n' && playerName[4] == 'n' &&
                    playerName[5] == 'y') {
                    DrawStringCentered("\pOh my God! They killed Kenny!",
                                       (mainMenuCenter.h - 0x96),
                                       (mainMenuCenter.h + 0x96),
                                       (mainMenuCenter.v + 0x136));
                } else {
                    pstrcpy(text, playerName);
                    PtoCstr(text);
                    strcat((char *)text, " ");
                    StrcatIndCString((char *)text, kGameStrList, 0x115);
                    CtoPstr((char *)text);
                    DrawStringCentered(text, (mainMenuCenter.h - 0x96),
                                       (mainMenuCenter.h + 0x96),
                                       (mainMenuCenter.v + 0x136));
                }
            }
        } else if (slideIndex[2] >= slideFrameCount[2]) {
            RGBForeColor(labelColor);
            GetIndString(text, kGameStrList, 0x114);
            DrawStringCentered(text, (mainMenuCenter.h - 0x96),
                               (mainMenuCenter.h + 0x96),
                               (mainMenuCenter.v + 0x136));
        }
    }

    if (slideIndex[0] >= slideFrameCount[0])
        shown[0] = shown[3] = 1;
    else
        shown[0] = shown[3] = 0;
    if (slideIndex[1] >= slideFrameCount[1])
        shown[1] = shown[4] = 1;
    else
        shown[1] = shown[4] = 0;
    if (slideIndex[2] >= slideFrameCount[2])
        shown[2] = shown[5] = 1;
    else
        shown[2] = shown[5] = 0;

    for (i = 0; i < 6; i++) {
        if (menuButtonSprite[i] != NULL && shown[i] != 0) {
            box = menuButtonSprite[i]->frameArray[0]->frameRect;
            OffsetRect(&box, menuTopCorner.h + menuButtonPos[i].h,
                       menuTopCorner.v + menuButtonPos[i].v);
            SWSetSpriteLocation(menuButtonSprite[i],
                         (menuTopCorner.h + menuButtonPos[i].h),
                         (menuTopCorner.v + menuButtonPos[i].v));
            BlitPixieRLEDrawProc(
                menuButtonSprite[i]->frameArray[0],
                gSpriteWorld->workFrameP, &box, &box);
        }
    }
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    for (i = 0; i < 6; i++) {
        if (menuButtonSprite[i] != NULL && shown[i] != 0) {
            box = menuButtonSprite[i]->frameArray[0]->frameRect;
            OffsetRect(&box, menuTopCorner.h + menuButtonPos[i].h,
                       menuTopCorner.v + menuButtonPos[i].v);
            SWSetSpriteLocation(menuButtonSprite[i],
                         (menuTopCorner.h + menuButtonPos[i].h),
                         (menuTopCorner.v + menuButtonPos[i].v));
            BlitPixieRLEDrawProc(
                menuButtonSprite[i]->frameArray[0],
                gSpriteWorld->workFrameP, &box, &box);
        }
    }
#ifdef MACBUILD
    if (!useLegacyRenderingPipeline)
        ForeColor(blackColor);
#endif
    MainScreenRolloverDraw();
    if (which == 1) {
        CopyBits(GetPortBitMapForCopyBits(
                     gSpriteWorld->workFrameP->framePort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &gSpriteWorld->workFrameP->frameRect,
                 &gSpriteWorld->workFrameP->frameRect, 0, NULL);
        /* and the 0xc2-wide status strip, loadPort -> the window's right edge. */
        box = loadPort.bounds;
#ifdef CEFIX /* F14.15 */
        box.left = (box.right - ceStatusBarWidth);
#else
        box.left = (box.right - 0xc2);
#endif
        dst = *GetCWindowPortRect((WindowRef)gWind);
#ifdef CEFIX /* F14.16 */
        dst.left = (dst.right - ceStatusBarWidth);
#else
        dst.left = (dst.right - 0xc2);
#endif
        CopyBits(GWorldContainerBits(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &box, &dst, 0, NULL);
#ifdef MACBUILD
        QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
#endif
    }
}

#define kShippingVer    "1.0.A"
#define kUpgradePrompt  0x33
#define kUrlStrList     0x82
#define kUrlIndex       6
#define kPollTimeout    5
#define kHotStripHeight 0x14
#define kBarHeight      0x1e

#ifdef MAC1010
void MainScreenActivate(Boolean activate)
#include "../engine/mainscreenactivate.inc"
#endif

/* MainScreenIdle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00488080
// MAC: _MainScreenIdle i386 0x00018ae8 ppc 0x0001c6f0
#if V111
void MainScreenIdle(void)
{
    MainScreenAnimIdle();
}
#elif defined(MAC1010)
void MainScreenIdle(void)
{
    unsigned char text[256];
    Rect strip;
    Point mouse;
    void *reply;
    char *latest;
    Boolean wanted;

    if (reggieStarted != 0 && reggieVersion[0] == 0 &&
        slideIndex[2] >= kAnimDone && inBackground == 0) {
        GetIndString(text, kUrlStrList, 5);
        PtoCstr(text);
        reply = ReggieVersionCheck(0, (char *)text);
        if (reply != NULL) {
            latest = HashLookup(reply, "$VERSION", 0);
            if (latest != NULL) {
                crashlog_print("latest version is ");
                crashlog_print(latest);
                crashlog_print("\n");
                strcpy(reggieVersion, latest);
                reggieSuccess = 1;
                reggieStarted = 0;
                if (strcmp(reggieVersion, kShippingVer) > 0) {
                    GetIndString(text, kAlertStrList, kUpgradePrompt);
                    wanted = EVYesNoDialog(text);
                    MainScreenUpdate(1);
                    if (wanted) {
                        GetIndString(text, kUrlStrList, kUrlIndex);
                        PtoCstr(text);
                        IT_OpenBrowser((char *)text);
                    }
                }
            }
        } else {
            crashlog("reggie version check returned nil");
        }
        if (reggieSuccess == 0) {
            if ((TimerGetSeconds() - reggieStarted) > kPollTimeout) {
                crashlog("timeout contacting version server");
                reggieStarted = 0;
            }
        }
    }
    HandleIntroMusic(0);
    MainScreenAnimIdle();
#if TARGET_API_MAC_OS8
    strip = *GetCWindowPortRect((WindowRef)gWind);
    strip.bottom = strip.top + kHotStripHeight;
    GetMouse(&mouse);
    if ((PtInRect)(mouse, &strip)) {
#else
    GetMouse(&mouse);
    if (!(PtInRect)(mouse, &screenMinusDockRect)) {
#endif
        if (mBarHidden != 0) {
            ShowTheMenuBar();
            InvalWindowRect((WindowRef)gWind, GetCWindowPortRect((WindowRef)gWind));
            mBarHidden = 0;
        }
    } else if (mBarHidden == 0 && inBackground == 0 &&
               settings.windowed == 0) {
        HideTheMenuBar();
        InvalWindowRect((WindowRef)gWind, GetCWindowPortRect((WindowRef)gWind));
        SetPortWindowPort((WindowRef)gWind);
        ForeColor(blackColor);
        strip = *GetCWindowPortRect((WindowRef)gWind);
        strip.bottom = strip.top + kBarHeight;
        PaintRect(&strip);
        mBarHidden = 1;
    }
}
#else
void MainScreenIdle(void)
{
    Boolean wanted;
    void *reply;
    char *latest;
    Rect *portRect;
    int now;
    Point mouse;
    unsigned char text[256];
    Rect strip;

    if (reggieStarted != 0 && reggieVersion[0] == 0 &&
        slideIndex[2] >= kAnimDone && inBackground == 0) {
        strcpy((char *)text, "EV Nova PC");
        reply = ReggieVersionCheck(0, (char *)text);
        if (reply == NULL) {
            crashlog("reggie version check returned nil");
        } else {
            latest = HashLookup(reply, "$VERSION", 0);
            if (latest != NULL) {
                crashlog_print("latest version is ");
                crashlog_print(latest);
                crashlog_print("\n");
                strcpy(reggieVersion, latest);
                reggieSuccess = 1;
                reggieStarted = 0;
                if (memcmp(reggieVersion, kShippingVer,
                           sizeof(kShippingVer)) > 0) {
                    GetIndString(text, kAlertStrList, kUpgradePrompt);
                    wanted = EVYesNoDialog(text);
                    MainScreenUpdate(1);
                    if (wanted) {
                        GetIndString(text, kUrlStrList, kUrlIndex);
                        PtoCstr(text);
                        IT_OpenBrowser((char *)text);
                    }
                }
            }
        }
        if (reggieSuccess == 0) {
            now = TimerGetSeconds();
            if ((unsigned int)(now - reggieStarted) > kPollTimeout) {
                crashlog("timeout contacting version server");
                reggieStarted = 0;
            }
        }
    }
    HandleIntroMusic(0);
    MainScreenAnimIdle();
    portRect = GetCWindowPortRect(gWind);
    strip.top = portRect->top;
    strip.left = portRect->left;
    strip.bottom = portRect->bottom;
    strip.right = portRect->right;
    strip.bottom = strip.top + kHotStripHeight;
    GetGlobalMouse(&mouse);
    if (PtInRect(mouse.h, mouse.v, &strip) != 0) {
        if (mBarHidden != 0) {
            ShowTheMenuBar();
            GetCWindowPortRect(gWind);
            gNeedUpdateFlag = 1;
            mBarHidden = 0;
        }
    } else if (mBarHidden == 0 && inBackground == 0 &&
               settings.windowed == 0) {
        HideTheMenuBar();
        GetCWindowPortRect(gWind);
        gNeedUpdateFlag = 1;
        SetPortWindowPort(gWind);
        ForeColor(blackColor);
        portRect = GetCWindowPortRect(gWind);
        strip.top = portRect->top;
        strip.left = portRect->left;
        strip.bottom = portRect->bottom;
        strip.right = portRect->right;
        strip.bottom = strip.top + kBarHeight;
        PaintRect(&strip);
        mBarHidden = 1;
    }
}
#endif

/*
 * mainMenuCenter is h-then-v, NOT QuickDraw's Point order: +0 takes
 * (left+right)/2 and +2 takes (top+bottom)/2. The two guards deliberately
 * re-read loadPort.bounds rather than interfacePicRect, and are deliberately
 * non-parallel - `< 768` on the height, `<= 640` on the width.
 */

#define kInterfacePictID    0x1f40
#define kMinFullHeight      768
#define kMinFullWidth       640
#define kSmallScreenNudge   60
#define kMenuHalfWidth      512
#define kMenuHalfHeight     384

#ifdef MACBUILD
/* MainScreenClose (mac _MainScreenClose): mac only, no caller. */
// MAC: _MainScreenClose i386 0x00016313 ppc 0x0001a0ac
void MainScreenClose(void)
{
}

/* MainScreenClobber (mac _MainScreenClobber): mac only, no caller. */
// MAC: _MainScreenClobber i386 0x00016318 ppc 0x0001a0b0
void MainScreenClobber(void)
{
}
#endif

#ifdef MAC1010
void DoMenuClick(int32_t menuChoice)
#include "../engine/domenuclick.inc"
#endif

/* SetupInterfaceBackground (mac _SetupInterfaceBackground). STATE:MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00488380
// MAC: _SetupInterfaceBackground i386 0x0001631d ppc 0x0001a0b4
void SetupInterfaceBackground(void)
{
    void *pic;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    pic = GetPicture(kInterfacePictID);
    if (pic != 0) {
        interfacePicRect = loadPort.bounds;
        CenterPict(pic, &interfacePicRect);
        if (loadPort.bounds.bottom - loadPort.bounds.top < kMinFullHeight)
            OffsetRect(&interfacePicRect, 0, -kSmallScreenNudge);
        if (loadPort.bounds.right - loadPort.bounds.left <= kMinFullWidth)
            OffsetRect(&interfacePicRect, -kSmallScreenNudge, 0);
        mainMenuCenter.h = (interfacePicRect.left + interfacePicRect.right) / 2;
        mainMenuCenter.v = (interfacePicRect.top + interfacePicRect.bottom) / 2;
        menuTopCorner.h = mainMenuCenter.h - kMenuHalfWidth;
        menuTopCorner.v = mainMenuCenter.v - kMenuHalfHeight;
        ForeColor(blackColor);
        PaintRect(&loadPort.bounds);
        DrawPicHandle((PicHandle)pic, &interfacePicRect);
#ifdef CEFIX /* F28.01 */
        CE_DrawVersion();
#endif
        QDReleaseResource((PicHandle)pic);
    }
    SWSetPortToWindow(gSpriteWorld);
}


#define kPlainStrIdx  0x118
#define kItemMemoryText 0x13
#define kItem3StrIdx  0x10
#define kBytesPerMB   1000000
#define kMemorySlack  (10 * kBytesPerMB)

/* PrefsDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004884f0
#if V111
#else
void PrefsDialogUpdate(void)
{
    Rect box;

    SetDialogGWorld(prefsDialog);
    GetDialogItem(prefsDialog, kItemMemoryText, NULL, NULL, &box);
    RGBForeColor(&rgbBlack);
    EraseRect(&box);
    MoveTo(box.left, (box.bottom - 10));
    if (gameLoaded == 0 && gWind != NULL) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
    } else {
        TextFont(0);
        TextSize(12);
        DrawIndString(kGameStrList, kPlainStrIdx);
    }
    PenSize(1, 1);
    MoveTo(box.left, box.bottom);
    LineTo(box.right, box.bottom);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (majorPrefsChange != 0 && gWind != NULL) {
        GetDialogItem(prefsDialog, 3, NULL, NULL, &box);
        EraseRect(&box);
        MoveTo(box.left, (box.bottom - 5));
        DrawIndString(kAlertStrList, kItem3StrIdx);
    }
}
#endif

/* ---- DoPrefsDialog (0x00488650) ---- */

#define kPrefsDlogID       0xfa3
#define kMusicVolStrList   0x88
#define kGammaStrList   0x8b
#define kMaxMusicVol          8
#define kMaxGamma          6
#define kTrueColorDepth  0x1000
#define kBeepChannel          1

#define kItemDone             1
#define kItemFullScreen       2
#define kItemMusic            8
#define kItemMusicDown        6
#define kItemMusicUp          7
#define kItemEarlyOut         9
#define kItemNoDebris       0xa
#define kItemSmoothing      0xb
#define kItemShipSets      0xc
#define kItemShipGlow        0xd
#define kItemShipLights      0xe
#define kItemShipWeapons     0xf
#define kItemKeys          0x10
#define kItemStatusBar     0x12
#define kItemAutoAim       0x14
#define kItemMessages      0x15
#define kItemNetwork       0x16
#define kItemGammaText  0x18
#define kItemGammaDown     0x19
#define kItemGammaUp       0x1a
#define kItemMusicVolText     5

/* DoPrefsDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00488650
// MAC: _DoPrefsDialog i386 0x00016459 ppc 0x0001a1ec
#if V111
void DoPrefsDialog(void)
{
#ifdef MACBUILD
    CERunPrefsDialog();
#endif
}
#else
#ifdef MAC1010
extern int32_t gSystemVersion;
extern int32_t startupMemoryFree;
extern void PreflightMemoryUsage(int32_t *base, int32_t *engineGlow, int32_t *runningLights, int32_t *weaponGlow);

void DoPrefsDialog(void)
{
    Str255 volText;
    Settings saved;
    Rect box;
    void *item;
    int32_t baseSize;
    int32_t engineGlowSize;
    int32_t lightsSize;
    int32_t weaponGlowSize;
    short itemHit;
    short itemType;
    void *ctl;
    Boolean done;
    int32_t needed;
    int32_t size;

    prefsDialog = NULL;
    prefsDialog = GetNewDialog(kPrefsDlogID, 0, -1);
    if (prefsDialog == NULL)
        return;

    if (gWind != NULL)
        CenterWindow(FrontWindow(prefsDialog), gWind);
    ShowMacWindow(FrontWindow(prefsDialog));
    SelectWindow(FrontWindow(prefsDialog));
    SetDialogGWorld(prefsDialog);
    RedrawDialog(prefsDialog);
    FrameDefaultButton(prefsDialog, 1);

    saved = settings;
    done = 0;
    PreflightMemoryUsage(&baseSize, &engineGlowSize, &lightsSize, &weaponGlowSize);
    GetMainDevice();

    do {
        SetDialogGWorld(prefsDialog);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(blackColor);

        GetDialogItem(prefsDialog, kItemFullScreen, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (gSystemVersion >= 0x1000) {
            SetControlValue(ctl, 1);
            HiliteControl(ctl, 255);
        } else if (settings.yieldEvents != 0) {
            SetControlValue(ctl, 1);
        } else {
            SetControlValue(ctl, 0);
        }

        if (settings.windowed != 0 && gameLoaded != 0) {
            GetDialogItem(prefsDialog, kItemMessages, &itemType, (Handle *)&item, &box);
            SetControlValue(ctl = item, 0);
            HiliteControl(ctl, 255);
        }

        GetDialogItem(prefsDialog, kItemMusic, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.introMusic != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemSmoothing, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.windowedPending != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemEarlyOut, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noMovies == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemNoDebris, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noDebris == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipSets, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.shipAnimations != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipGlow, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.engineGlows != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipLights, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.runningLights != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipWeapons, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.weaponGlows != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemStatusBar, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.parallaxStars != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemAutoAim, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.ambientSounds != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemMessages, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noHyperGamma == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemNetwork, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noVersionCheck == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemMusicVolText, &itemType, (Handle *)&item, &box);
        if (settings.volume >= 0 && settings.volume <= kMaxMusicVol) {
            GetIndString(volText, kMusicVolStrList, settings.volume + 1);
            SetDialogItemText(item, volText);
        } else {
            SetDialogItemText(item, "\p???");
        }

        majorPrefsChange = 0;
        if (settings.windowedPending != settings.windowed)
            majorPrefsChange = 1;
        if (gameLoaded != 0) {
            if (settings.shipAnimations != saved.shipAnimations || settings.engineGlows != saved.engineGlows
                || settings.runningLights != saved.runningLights || settings.weaponGlows != saved.weaponGlows)
                majorPrefsChange = 1;
        }

        SetDialogGWorld(prefsDialog);
        GetDialogItem(prefsDialog, kItemMemoryText, &itemType, (Handle *)&item, &box);
        ForeColor(blackColor);
        EraseRect(&box);
        MoveTo(box.left, box.bottom - 10);
        if (gameLoaded != 0 || gWind == NULL) {
            TextFont(0);
            TextSize(12);
            DrawIndString(kGameStrList, kPlainStrIdx);
        } else {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
#if !TARGET_API_MAC_CARBON
            size = baseSize;
            if (settings.runningLights != 0)
                size += lightsSize;
            if (settings.engineGlows != 0)
                size += engineGlowSize;
            if (settings.weaponGlows != 0)
                size += weaponGlowSize;
            needed = size + kMemorySlack;
            if ((uint32_t)needed > startupMemoryFree) {
                needed -= startupMemoryFree;
                needed = needed / kBytesPerMB + 1;
                NumToString(needed, volText);
                DrawIndString(kAlertStrList, 0xb);
                DrawString("\p ");
                DrawString(volText);
                DrawString("\p ");
                if (needed == 1)
                    DrawIndString(kAlertStrList, 0xc);
                else
                    DrawIndString(kAlertStrList, 0xd);
                DrawString("\p ");
                DrawIndString(kAlertStrList, 0xe);
            } else {
                DrawIndString(kAlertStrList, 0xf);
                DrawString("\p ");
                DrawIndString(kAlertStrList, 0xe);
            }
#endif
        }
        PenSize(1, 1);
        MoveTo(box.left, box.bottom);
        LineTo(box.right, box.bottom);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        if (majorPrefsChange != 0 && gWind != NULL) {
            GetDialogItem(prefsDialog, 3, &itemType, (Handle *)&item, &box);
            EraseRect(&box);
            MoveTo(box.left, box.bottom - 5);
            DrawIndString(kAlertStrList, kItem3StrIdx);
        }

        ModalDialog(NULL, &itemHit);

        if (itemHit == kItemDone) {
            SavePrefs();
            done = 1;
        }
        if (itemHit == kItemMusic) {
            settings.introMusic = (settings.introMusic == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemFullScreen && gSystemVersion < 0x1000) {
            settings.yieldEvents = (settings.yieldEvents == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemSmoothing) {
            settings.windowedPending = (settings.windowedPending == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemEarlyOut) {
            settings.noMovies = (settings.noMovies == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemNoDebris) {
            settings.noDebris = (settings.noDebris == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipSets) {
            settings.shipAnimations = (settings.shipAnimations == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipGlow) {
            settings.engineGlows = (settings.engineGlows == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipLights) {
            settings.runningLights = (settings.runningLights == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipWeapons) {
            settings.weaponGlows = (settings.weaponGlows == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemStatusBar) {
            settings.parallaxStars = (settings.parallaxStars == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemAutoAim) {
            settings.ambientSounds = (settings.ambientSounds == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemMessages) {
            settings.noHyperGamma = (settings.noHyperGamma == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemNetwork) {
            settings.noVersionCheck = (settings.noVersionCheck == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemKeys) {
            DoKeyDialog();
            RedrawDialog(prefsDialog);
            if (gameLoaded != 0 && gPause != 0) {
                MainScreenAnimDraw();
                MainScreenUpdate(1);
            }
            FrameDefaultButton(prefsDialog, 1);
        }
        if (itemHit == kItemMusicUp) {
            if (settings.volume < kMaxMusicVol) {
                settings.volume += 1;
                RedrawDialog(prefsDialog);
            } else {
                settings.volume = kMaxMusicVol;
            }
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        }
        if (itemHit == kItemMusicDown) {
            if (settings.volume > 0) {
                settings.volume -= 1;
                RedrawDialog(prefsDialog);
            } else {
                settings.volume = 0;
            }
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        }
    } while (done == 0);

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(prefsDialog);
    prefsDialog = NULL;
    if (settings.introMusic == 0)
        StopIntroMusic(0);
}
#else
void DoPrefsDialog(void)
{
    unsigned char savedShipSets;
    unsigned char savedShipGlow;
    unsigned char savedShipWeapons;
    unsigned char savedShipLights;
    Str255 volText;              /* +0x10 */
    Rect box;                    /* +0x110 */
    short itemType;              /* +0x118 */
    short itemHit;               /* +0x11a */
    void *item;                  /* +0x11c */
    void *ctl;
    short savedGamma;
    Boolean done;
    unsigned short gammaLevel;
    unsigned short musicVol;

    prefsDialog = NULL;
    prefsDialog = GetNewDialog(kPrefsDlogID, PrefsDialogUpdate, NULL);
    if (prefsDialog == NULL)
        return;

    if (gWind != NULL)
        CenterWindow(FrontWindow(prefsDialog), gWind);
    ShowMacWindow(FrontWindow(prefsDialog));
    FrontWindow(prefsDialog);
    SetDialogGWorld(prefsDialog);
    RedrawDialog(prefsDialog);
    FrameDefaultButton(prefsDialog, 1);

    savedShipSets = settings.shipAnimations;
    savedShipGlow = settings.engineGlows;
    savedShipWeapons = settings.weaponGlows;
    savedShipLights = settings.runningLights;
    done = 0;
    savedGamma = settings.gamma;

    GetDialogItem(prefsDialog, kItemFullScreen, NULL, (Handle *)&item, &box);
    DisableControl((ControlPtr)item);
    GetDialogItem(prefsDialog, kItemMessages, NULL, (Handle *)&item, &box);
#ifndef CEFIX /* F22.04 */
    DisableControl((ControlPtr)item);
#endif

    do {
        SetDialogGWorld(prefsDialog);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(blackColor);

        GetDialogItem(prefsDialog, kItemFullScreen, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (DAT_007355d0 >= kTrueColorDepth) {
            SetControlValue((ControlPtr)ctl, 1);
        } else if (settings.yieldEvents != 0) {
            SetControlValue((ControlPtr)ctl, 1);
        } else {
            SetControlValue((ControlPtr)ctl, 0);
        }

        if (settings.windowed != 0 && gameLoaded != 0) {
            GetDialogItem(prefsDialog, kItemMessages, &itemType, (Handle *)&item, &box);
            SetControlValue((ControlPtr)item, 0);
        }

        GetDialogItem(prefsDialog, kItemMusic, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.introMusic != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemSmoothing, &itemType, (Handle *)&item, &box);
        ctl = item;
#ifdef CEFIX /* F04.01 */
        ceRunInAWindow = CE_IsWindowed();
        SetControlValue((ControlPtr)ctl, ceRunInAWindow);
#else
        if (settings.windowedPending != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
#endif
        GetDialogItem(prefsDialog, kItemEarlyOut, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noMovies == 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemNoDebris, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noDebris == 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemShipSets, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.shipAnimations != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemShipGlow, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.engineGlows != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemShipLights, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.runningLights != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemShipWeapons, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.weaponGlows != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemStatusBar, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.parallaxStars != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemAutoAim, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.ambientSounds != 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemMessages, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noHyperGamma == 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);
        GetDialogItem(prefsDialog, kItemNetwork, &itemType, (Handle *)&item, &box);
        ctl = item;
        if (settings.noVersionCheck == 0)
            SetControlValue((ControlPtr)ctl, 1);
        else
            SetControlValue((ControlPtr)ctl, 0);

        GetDialogItem(prefsDialog, kItemMusicVolText, &itemType, (Handle *)&item, &box);
        musicVol = settings.volume;
        if ((short)musicVol < 0 || (short)musicVol > kMaxMusicVol) {
            SetDialogItemText((Handle)item, "\p???");
        } else {
            GetIndString(volText, kMusicVolStrList, ((short)musicVol + 1));
            SetDialogItemText((Handle)item, volText);
        }

        GetDialogItem(prefsDialog, kItemGammaText, NULL, (Handle *)&item, &box);
        if (item != NULL) {
            gammaLevel = settings.gamma;
            if ((short)gammaLevel > kMaxGamma) {
                SetDialogItemText((Handle)item, "\p???");
            } else {
                GetIndString(volText, kGammaStrList, ((short)gammaLevel + 1));
                SetDialogItemText((Handle)item, volText);
            }
        }

        majorPrefsChange = 0;
        if (settings.windowedPending != settings.windowed)
            majorPrefsChange = 1;
        if (gameLoaded != 0) {
            if (settings.shipAnimations != savedShipSets || settings.engineGlows != savedShipGlow
                || settings.runningLights != savedShipLights || settings.weaponGlows != savedShipWeapons
                || settings.gamma != savedGamma)
                majorPrefsChange = 1;
        }

        ModalDialog(NULL, &itemHit);

        if (itemHit == kItemDone) {
            SavePrefs();
            done = 1;
        }
        if (itemHit == kItemMusic) {
            settings.introMusic = (settings.introMusic == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemFullScreen && DAT_007355d0 < kTrueColorDepth) {
            settings.yieldEvents = (settings.yieldEvents == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemSmoothing) {
#ifdef CEFIX /* F04.02-04 */
            ceRunInAWindow = (ceRunInAWindow == 0);
            CE_ToggleFullscreen();
#else
            settings.windowedPending = (settings.windowedPending == 0);
#endif
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemEarlyOut) {
            settings.noMovies = (settings.noMovies == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemNoDebris) {
            settings.noDebris = (settings.noDebris == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipSets) {
            settings.shipAnimations = (settings.shipAnimations == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipGlow) {
            settings.engineGlows = (settings.engineGlows == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipLights) {
            settings.runningLights = (settings.runningLights == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipWeapons) {
            settings.weaponGlows = (settings.weaponGlows == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemStatusBar) {
            settings.parallaxStars = (settings.parallaxStars == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemAutoAim) {
            settings.ambientSounds = (settings.ambientSounds == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemMessages) {
            settings.noHyperGamma = (settings.noHyperGamma == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemNetwork) {
            settings.noVersionCheck = (settings.noVersionCheck == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemKeys) {
            DoKeyDialog();
            RedrawDialog(prefsDialog);
            if (gameLoaded != 0 && gPause != 0) {
                MainScreenAnimDraw();
                MainScreenUpdate(1);
            }
            FrameDefaultButton(prefsDialog, 1);
        }
        if (itemHit == kItemMusicUp) {
            if (settings.volume < kMaxMusicVol) {
                settings.volume += 1;
                RedrawDialog(prefsDialog);
            } else {
                settings.volume = kMaxMusicVol;
            }
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        }
        if (itemHit == kItemMusicDown) {
            if (settings.volume > 0) {
                settings.volume -= 1;
                RedrawDialog(prefsDialog);
            } else {
                settings.volume = 0;
            }
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        }
        if (itemHit == kItemGammaUp) {
            if (settings.gamma < kMaxGamma) {
                settings.gamma += 1;
                RedrawDialog(prefsDialog);
            } else {
                settings.gamma = kMaxGamma;
            }
        }
        if (itemHit == kItemGammaDown) {
            if (settings.gamma > 0) {
                settings.gamma -= 1;
                RedrawDialog(prefsDialog);
            } else {
                settings.gamma = 0;
            }
        }
    } while (done == 0);

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(prefsDialog);
    prefsDialog = NULL;
    if (settings.introMusic == 0)
        StopIntroMusic(0);
}
#endif
#endif

/* entergame -- EnterGameFromMainScreen and DoNewPilot. Notes: Research/notes/entergame.md */

/* far arrays inside the one big 0x005912a0 allocation, reached at raw offsets
   because no header names them yet */

/* RT3 library string helpers, NOT MSL: the game's strlen is BurgerLib's FastStrLen
   0x004eedb0, so these keep FUN_ names. */

/* EnterGameFromMainScreen (mac _EnterGameFromMainScreen). STATE: MATCH 100.0%, exe 763 / obj 763,
   byte-identical yes (2912 bytes, relocs masked), relocregcheck 0 of 194. 1.0.10 MATCH 907 of
   907 Classic, 894 of 894 Carbon, padded. OPEN: nothing. */
// FUNCTION: EVNOVA 0x00489210
// MAC: _EnterGameFromMainScreen i386 0x00019ca7 ppc 0x0001d988
#ifdef MACBUILD
void EnterGameFromMainScreen(void)
{
    Rect box;
    Size grow;
    GDHandle device;
    Str255 msg;
    Str255 num;

    crashlog("---");
    crashlog("entering game");
    if (useLegacyRenderingPipeline) {
        device = gSpriteWorld->mainSWGDH;
        if (device != NULL) {
            if ((*(*device)->gdPMap)->pixelSize != gSpriteWorld->pixelDepth) {
                if (gSpriteWorld->pixelDepth == 8)
                    GetIndString(msg, kAlertStrList, kStrDepth256);
                else
                    GetIndString(msg, kAlertStrList, kStrDepthThousands);
                EVNoteAlert(msg);
                return;
            }
            box = (*(*device)->gdPMap)->bounds;
            if (box.right - box.left != mainDeviceInitBounds.right - mainDeviceInitBounds.left
                || box.bottom - box.top != mainDeviceInitBounds.bottom - mainDeviceInitBounds.top) {
#ifdef MAC1010
                strcpy((char *)msg, "");
#else
                msg[0] = 0;
#endif
                StrcatIndCString((char *)msg, kAlertStrList, kStrResolution);
                strcat((char *)msg, " ");
                NumToString(mainDeviceInitBounds.right - mainDeviceInitBounds.left, num);
                PtoCstr(num);
                strcat((char *)msg, (char *)num);
                strcat((char *)msg, "x");
                NumToString(mainDeviceInitBounds.bottom - mainDeviceInitBounds.top, num);
                PtoCstr(num);
                strcat((char *)msg, (char *)num);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kAlertStrList, kStrResolutionTail);
                CtoPstr((char *)msg);
                EVNoteAlert(msg);
                return;
            }
        }
    }
    SWWindowMoved(gSpriteWorld);
    playerIsLanded = 0;
    crashlog("adjusting sounds");
    AdjustSoundVolume();
    shipData[17].licenseName = RT3_GetLicenseName();
    DisposeMainScreenSounds();
    crashlog("showing intro screen");
    if (!seenIntroScreen) {
        ShowIntroScreen();
        FadeScreenOut();
        seenIntroScreen = 1;
    } else {
        FadeScreenOut();
    }
    CheckForPiratedCodes();
#ifdef MAC1010
    StopIntroMusic(1);
#else
    CancelMusicThread();
#endif
    MaxMem(&grow);
    LICENSE_STEP_BEGIN
    shipData[5].licenseCode = RT3_GetLicenseCode();
    LICENSE_STEP_END
    crashlog("drawing stuff");
    LoadIntfResource();
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (!settings.windowed)
        HideTheMenuBar();
    RandomizeStars();
    PropagateMissionBitEffects();
    if (systemData[shipData->curSystem].murk < 0)
        ClearStarfield();
    DrawMapInFloatingFrame();
    LICENSE_STEP_BEGIN
    shipData[36].licenseCopies = RT3_GetLicenseCopies();
    LICENSE_STEP_END
    crashlog("drawing more stuff");
    LICENSE_STEP_BEGIN
    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? 1 : 0;
    LICENSE_STEP_END
    ResetPlayerPrecalcedValues();

    do {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
        unsigned char t2;
        int32_t g;
        unsigned char t3;
        unsigned char t4;
        unsigned char t1;
        char *p;
        long long mem;
        unsigned char c;
        char b;
        char t5;
        Boolean ok;
        unsigned short rem = 0;
        unsigned char x;
        short dummy;

        /* the entry hoists of both indexes: a second name each, no word */
        dummy = gameIsRegisteredIndex;
        dummy = gameIsHackedIndex;
        g = shipData[36].licenseCopies;
#ifdef MAC1010
        p = (char *)shipData[17].licenseName;
#else
        p = RT3_CheckLicenseName(shipData[17].licenseName);
#endif
        mem = shipData[5].licenseCode;
        x = 0;
#ifndef MAC1010
        b = 0;
        if (p != 0 && *p >= '0' && *p <= '9')
            g = 1;
#endif
        if (p != 0) {
            while (StringGetLength(key) + 1 < 17 && *p != 0)
                StringAppendSafe(key, (const unsigned char *)p, 17);
        }
        c = key[3];
        x ^= c;
        x ^= name[14];
        x += (char)0xf1;
        x ^= key[10];
        x ^= key[8];
        x = ROTL(x, 0);
        x ^= key[15];
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x = ROTL(x, 7);
        x ^= key[1];
        x = ROTL(x, 2);
        x = ROTL(x, 1);
        x += 0x5f;
        x = ROTL(x, 4);
        t1 = name[13];
        x ^= t1;
        t2 = name[5];
        x ^= t2;
        x = ROTL(x, 0);
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x ^= (unsigned char)g;
        x += 0x42;
        if (x < (char)key[11])
            x = ROTL(x, 8);
        x ^= name[13];
        x ^= (unsigned char)g;
        x += -0x6b;
        x = ROTL(x, 1);
        x ^= key[7];
        x += 0x1e;
        x ^= key[12];
        x = ROTL(x, 6);
        x ^= name[3];
        x += (char)0xac;
        x ^= c;
        x ^= key[13];
        rem = RT3_ExtractLicenseBlock1(mem);
        x = ROTL(x, 6);
        x ^= (unsigned char)g;
        x = ROTL(x, 2);
        x ^= key[4];
        x = ROTL(x, 3);
#ifdef MAC1010
        x = (unsigned)x >> 3 | x << 5; /* 1.0.10 pad: ROTL(x, 5) by a logical shift, so no srawi stands between the read below and the call */
        dummy = mem >> 32; /* 1.0.10 pad: mem's high half read past the licence call; its srawi keeps the read to the colourer, deleted after it */
#else
        x = ROTL(x, 5);
#endif
        x ^= (unsigned char)g;
        b = x;
        x = RegScramD3(&b);
        x += 0x19;
        x = ROTL(x, 3);
        x ^= key[15];
        RegScramB3(x);
        x = ROTL(x, 6);
        RegScramF3();
        x = ROTL(x, 5);
        x ^= (unsigned char)g;
        x ^= key[14];
        t3 = (unsigned char)name[0];
        x ^= t3;
        x ^= t2;
        x += 7;
        x ^= key[8];
        x += -0x60;
        x = ROTL(x, 4);
        x += -0x23;
        x = ROTL(x, 1);
        x ^= key[13];
        RegScramB3(x);
        x += 0x3e;
        x ^= (unsigned char)g;
        t4 = name[1];
        x ^= t4;
        if (x < (char)key[6])
            x = ROTL(x, 3);
        x ^= (unsigned char)g;
        x += (char)0xe1;
        x ^= name[7];
        x ^= RegScramE3();
        x ^= t3;
        x ^= key[8];
        x = ROTL(x, 8);
        x ^= t1;
        x = ROTL(x, 3);
        if (x > (char)(t1 = name[9]))
            x = ROTL(x, 7);
        x = ROTL(x, 5);
        x ^= name[8];
        x ^= key[7];
        x ^= (unsigned char)g;
        if (x > 0x5f) {
            b = x;
            x = RegScramD3(&b);
        }
        x = ROTL(x, 6);
        x += -0x53;
        x = ROTL(x, 7);
        x = ROTL(x, 0);
        x ^= key[2];
        x += -0x53;
        x = ROTL(x, 0);
        x += 0xae;
        x ^= t3;
        x = ROTL(x, 6);
        x += (char)0xc2;
        x ^= key[14];
        if (x < 0x94)
            x = ROTL(x, 6);
        x ^= (unsigned char)g;
        x = ROTL(x, 1);
        x = ROTL(x, 2);
        x += 0x45;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x = ROTL(x, 0);
        x += 0x62;
        x ^= key[10];
        if (x < (char)key[1])
            x = ROTL(x, 5);
        if (x > (t5 = key[9]))
            x ^= (unsigned char)t5;
        x ^= (unsigned char)g;
        x += 0x6c;
        x = ROTL(x, 6);
        x += 3;
        x ^= (unsigned char)t5;
        x += -0x6d;
        x = ROTL(x, 3);
        x = ROTL(x, 1);
        x ^= key[15];
        x ^= key[4];
        x += (char)0x99;
        if (x > 0x47)
            x ^= key[0];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        x ^= (unsigned char)g;
        x += (char)0xd9;
        RegScramC3(x);
        x ^= t2;
        x = ROTL(x, 6);
        x += 0xa;
        x ^= (unsigned char)g;
        x ^= name[1];
        x ^= key[10];
        x += (char)0x96;
        x ^= name[2];
        x ^= name[4];
        x ^= t1;
        x += 0xb;
        if (x != 0)
            RegScramA3(x);

        shipClass[gameIsHackedIndex].hacked =
            (shipData[36].licenseCopies != 0 && !ok)
                ? 1 : 0;
    } while (0);

    HandleStellarSystemVisibility();
    HandleStellarSprites();
    totalDaysPlayed = RT3_GetDaysHad();
    PrepareAllSprites();
    gameFrozen = 1;
    ignoreKeys = 1;
    DoPlayGameWork(1);
    ignoreKeys = 0;
    gameFrozen = 0;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    RedrawStatus(1);
    RefreshSAT();
    SWUpdateSpriteWorld(gSpriteWorld, 1);
    RedrawComm(0);
#endif
    statusRedraw[kRedrawCargo] = 1;
    escortMenuOnscreen = 0;
    mBarHidden = 1;
    gPause = 0;
    SWSetPortToWindow(gSpriteWorld);
    AdjustSoundVolume();
    fpsCounter = 0;
    lastFPSUpdateTime = TickCount();
    DisposeMainScreenAnim();
    InitCursor();
    cursorHidden = 0;
    ShowTheCursor();
    HideTheCursor();
    GetMouse(&lastMousePos);
    lastMouseMoveTime = TickCount();
    crashlog("changing window attributes");
    SetRetainedWindow(gWind, true);
    IterateInGameEventLoop(-1);
    RefreshWindowBuffer();
    crashlog("playing game");
    if (!EVBugResourceFieldTest(10))
        PlayGame();
    crashlog("paused");
    crashlog("resetting window attributes");
    SetRetainedWindow(gWind, false);
    if (!settings.windowed)
        HideTheMenuBar();
    if (strictPlay && IsDying(shipData) && shipData->type != kShipTypeEscapePod) {
        crashlog("deleting pilot file due to strict play death");
        DeletePilotFile(playerName);
    }
    crashlog("restoring stuff");
    KillVoiceSound();
    RestoreOrigClut();
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
    SetRect(&box, 0, 0, GetCWindowPortRect((WindowRef)gWind)->right, 20);
    PaintRect(&box);
    InvalWindowPortRect((WindowRef)gWind);
    if (!gQuit) {
        SetupInterfaceBackground();
        LoadMainScreenAnim();
        while (StillDown())
            BusyIdle();
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
    }
    ShowTheCursor();
    if (!settings.windowed)
        HideTheMenuBar();
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
    if (!gQuit) {
        crashlog("showing main screen again");
        InvalWindowPortRect((WindowRef)gWind);
#ifndef MAC1010
        FadeScreenIn();
#endif
        MainScreenAppearStart();
        MainScreenAppear();
        SetupInterfaceBackground();
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
    FlushEvents(5, 0);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    crashlog("---");
}
#else
void EnterGameFromMainScreen(void)
{
    Rect box;
    Str255 pilotName;

    crashlog("---");
    crashlog("entering game");
    SWWindowMoved(gSpriteWorld);
    playerIsLanded = 0;
    crashlog("adjusting sounds");
    AdjustSoundVolume();
    shipData[17].licenseName = (unsigned char *)RT3_GetLicenseName();
    DisposeMainScreenSounds();
    crashlog("showing intro screen");
    if (seenIntroScreen) {
        FadeScreenOut();
    } else {
        ShowIntroScreen();
        FadeScreenOut();
        seenIntroScreen = 1;
    }
    CheckForPiratedCodes();
    StopIntroMusic(1);
    shipData[5].licenseCode = RT3_GetLicenseCode();
    crashlog("drawing stuff");
    LoadIntfResource();
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (!settings.windowed)
        HideTheMenuBar();
    RandomizeStars();
    PropagateMissionBitEffects();
    if (systemData[shipData->curSystem].murk < 0)
        ClearStarfield();
    DrawMapInFloatingFrame();
    shipData[36].licenseCopies = RT3_GetLicenseCopies();
    crashlog("drawing more stuff");
    shipClass[gameIsRegisteredIndex].registered =
        shipData[36].licenseCopies ? (unsigned char)1 : (unsigned char)0;
    ResetPlayerPrecalcedValues();

    {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "";
        int32_t g;
        unsigned char *p;
        long long mem;
        unsigned char x;
        unsigned char c;
        char b;
        int t1;
        int t2;
        int t3;
        int t4;
        unsigned short rem;
        Boolean ok;

        g = shipData[36].licenseCopies;
        p = shipData[17].licenseName;
        mem = shipData[5].licenseCode;
        x = 0;
        b = 0;
        if (p != 0) {
            while (StringGetLength(key) + 1 < 17 && *p != 0)
                StringAppendSafe(key, p, 17);
        }
        c = key[3];
        x ^= c;
        x ^= name[14];
        x += 0xf1;
        x ^= key[10];
        x ^= key[8];
        x ^= key[15];
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x = ROTL(x, 7);
        x ^= key[1];
        x = ROTL(x, 2);
        x = ROTL(x, 1);
        x += 0x5f;
        x = ROTL(x, 4);
        t1 = name[13];
        x ^= t1;
        t2 = name[5];
        x ^= t2;
        x ^= g;
        x ^= g;
        x ^= g;
        x += 0x42;
        if (x < (char)key[11])
            x = ROTL(x, 8);
        x ^= t1;
        x ^= g;
        x += -0x6b;
        x = ROTL(x, 1);
        x ^= key[7];
        x += 0x1e;
        x ^= key[12];
        x = ROTL(x, 6);
        x ^= name[3];
        x += 0xac;
        x ^= c;
        x ^= key[13];
        rem = RT3_ExtractLicenseBlock1(mem);
        x = ROTL(x, 6);
        x ^= g;
        x = ROTL(x, 2);
        x ^= key[4];
        x = ROTL(x, 3);
        x = ROTL(x, 5);
        x ^= g;
        b = x;
        x = RegScramD3(&b);
        x += 0x19;
        x = ROTL(x, 3);
        x ^= key[15];
        RegScramB3(x);
        x = ROTL(x, 6);
        RegScramF3();
        x = ROTL(x, 5);
        x ^= g;
        x ^= key[14];
        t3 = name[0];
        x ^= t3;
        x ^= t2;
        x += 7;
        x ^= key[8];
        x += -0x60;
        x = ROTL(x, 4);
        x += -0x23;
        x = ROTL(x, 1);
        x ^= key[13];
        RegScramB3(x);
        x += 0x3e;
        x ^= g;
        t4 = name[1];
        x ^= t4;
        if (x < (char)key[6])
            x = ROTL(x, 3);
        x ^= g;
        x += 0xe1;
        x ^= name[7];
        x ^= RegScramE3();
        x ^= t3;
        x ^= key[8];
        x = ROTL(x, 8);
        x ^= t1;
        x = ROTL(x, 3);
        t1 = name[9];
        if (x > (char)t1)
            x = ROTL(x, 7);
        x = ROTL(x, 5);
        x ^= name[8];
        x ^= key[7];
        x ^= g;
        if (x > 0x5f) {
            b = x;
            x = RegScramD3(&b);
        }
        x = ROTL(x, 6);
        x += -0x53;
        x = ROTL(x, 7);
        x ^= key[2];
        x += 0x5b;
        x ^= t3;
        x = ROTL(x, 6);
        x += 0xc2;
        x ^= key[14];
        if (x < 0x94)
            x = ROTL(x, 6);
        x ^= g;
        x = ROTL(x, 1);
        x = ROTL(x, 2);
        x += 0x45;
        x = ROTL(x, 4);
        x = ROTL(x, 2);
        x += 0x62;
        x ^= key[10];
        if (x < (char)key[1])
            x = ROTL(x, 5);
        if (x > (char)key[9])
            x ^= key[9];
        x ^= g;
        x += 0x6c;
        x = ROTL(x, 6);
        x += 3;
        x ^= key[9];
        x += -0x6d;
        x = ROTL(x, 3);
        x = ROTL(x, 1);
        x ^= key[15];
        x ^= key[4];
        x += 0x99;
        if (x > 0x47)
            x ^= key[0];
        if (x < 3)
            x = blob[x];
        while (rem >= x)
            rem -= x;
        ok = (g != 0 && rem == 0);
        x ^= g;
        x += 0xd9;
        RegScramC3(x);
        x ^= t2;
        x = ROTL(x, 6);
        x += 0xa;
        x ^= g;
        x ^= t4;
        x ^= key[10];
        x += 0x96;
        x ^= name[2];
        x ^= name[4];
        x ^= t1;
        x += 0xb;
        if (x != 0)
            RegScramA3(x);

        shipClass[gameIsHackedIndex].hacked =
            (shipData[36].licenseCopies != 0 && !ok)
                ? (unsigned char)1 : (unsigned char)0;
    }

    HandleStellarSystemVisibility();
    HandleStellarSprites();
    totalDaysPlayed = RT3_GetDaysHad();
    PrepareAllSprites();
    gameFrozen = 1;
    ignoreKeys = 1;
    DoPlayGameWork(1);
    ignoreKeys = 0;
    gameFrozen = 0;
    RedrawStatus(1);
    RefreshSAT();
    SWUpdateSpriteWorld(gSpriteWorld, 1);
    RedrawComm(0);
    statusRedraw[kRedrawCargo] = 1;
    escortMenuOnscreen = 0;
    mBarHidden = 1;
    gPause = 0;
    SWSetPortToWindow(gSpriteWorld);
    AdjustSoundVolume();
    fpsCounter = 0;
    lastFPSUpdateTime = ReadTick();
    DisposeMainScreenAnim();
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    HideTheCursor();
    GetGlobalMouse(&lastMousePos);
    lastMouseMoveTime = ReadTick();
    crashlog("changing window attributes");
    IterateInGameEventLoop(-1);
    RefreshWindowBuffer();
    crashlog("playing game");
    if (!EVBugResourceFieldTest(10))
        PlayGame();
    crashlog("paused");
    crashlog("resetting window attributes");
    if (!settings.windowed)
        HideTheMenuBar();
    if (strictPlay && IsDying(shipData) && shipData->type != kShipTypeEscapePod) {
        crashlog("deleting pilot file due to strict play death");
        pstrcpy(pilotName, playerName);
#ifdef V111 /* V111: PtoCstr is void */
        PtoCstr(pilotName);
        DeletePilotFile((char *)pilotName);
#else
        DeletePilotFile(PtoCstr(pilotName));
#endif
    }
    crashlog("restoring stuff");
    KillVoiceSound();
    RestoreOrigClut();
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    SetRect(&box, 0, 0, GetCWindowPortRect(gWind)->right, 20);
    PaintRect(&box);
    GetCWindowPortRect(gWind);
    gNeedUpdateFlag = 1;
    if (!gQuit) {
        SetupInterfaceBackground();
        LoadMainScreenAnim();
        while (StillDown())
            BusyIdle();
        FlushEvents(3, 0);
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
    }
    ShowTheCursor();
    if (!settings.windowed)
        HideTheMenuBar();
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    if (!gQuit) {
        crashlog("showing main screen again");
        GetCWindowPortRect(gWind);
        gNeedUpdateFlag = 1;
        MainScreenAppearStart();
        MainScreenAppear();
        SetupInterfaceBackground();
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    FlushEvents(3, 0);
    FlushEvents(5, 0);
    FlushEvents(1, 0);
    FlushEvents(2, 0);
    crashlog("---");
}
#endif

#define kNewNameStrList  128

/* DoNewPilot (mac _DoNewPilot). STATE: MATCH 100.0%, 416 exact / 208 reloc-equivalent /
   0 mismatched, exe 624 / obj 624, relocregcheck 0 of 208; byte-identical yes (2669 bytes,
   relocs masked).
   OPEN: none */
// FUNCTION: EVNOVA 0x00489d70
// MAC: _DoNewPilot i386 0x00018b0a ppc 0x0001c720
void DoNewPilot(void)
{
    uint32_t resType;
    Boolean wantStrictPlay;
#ifdef MACBUILD
    Str255 prompt;
    Str255 charName;
    Str255 newShipName;
#else
    Str255 newShipName;
    char path[2048];
    Str255 charName;
    Str255 prompt;
#endif
    char **h;
    unsigned char *shipNameP;
    short i;
    short j;
    short sys;
    short k;
    Boolean ok;

    SWSetPortToWindow(gSpriteWorld);
    InvalWindowPortRect((WindowRef)gWind);
    wantStrictPlay = 0;
    GetIndString(newPlayerName[0], kNewNameStrList, Rand(3) + 1);
    GetIndString(newPlayerName[1], kNewNameStrList, Rand(3) + 4);
    shipNameP = newPlayerName[1];
    ok = DoNewPilotNameDialog(&wantStrictPlay);
    if (!ok)
        return;
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    CullNameString(newPlayerName[0]);
    CullNameString(shipNameP);
    TrimResourceName(newPlayerName[0]);
    TrimResourceName(shipNameP);
#ifdef MACBUILD
    if (DoesPilotExist(newPlayerName[0])) {
#else
    strcpy(path, pilotsFolder);
    PStr2CStr((char *)prompt, newPlayerName[0]);
    strcat(path, (char *)prompt);
    strcat(path, ".plt");
    if (DoesPilotExist(path)) {
#endif
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        GetIndString(prompt, kAlertStrList, 9);
        ok = EVYesNoDialog(prompt);
        if (!ok)
            return;
        MainScreenAnimDraw();
        MainScreenUpdate(1);
    }
    if (newPilotPopupChoice[0][0] == 0) {
        h = GetIndResource(kCharType, 1);
        if (h != 0) {
            GetResInfo(h, &i, &resType, newPilotPopupChoice[0]);
            ReleaseResource(h);
        }
    }
    i = GetCharResourceShipType(newPilotPopupChoice[0]);
    strcpy((char *)prompt, "");
    StrcatIndCString((char *)prompt, kGameStrList, 121);
    strcat((char *)prompt, " ");
    pstrcpy(charName, shipyardLongNames[i]);
    PtoCstr(charName);
    strcat((char *)prompt, (char *)charName);
    strcat((char *)prompt, ": ");
    CtoPstr((char *)prompt);
    GetIndString(charName, kNewNameStrList, Rand(3) + 7);
    ok = EVTextInputDialog(prompt, charName, 64);
    if (!ok)
        return;
    creatingPilotFile = 1;
    if (tolower(newPilotPopupChoice[1][1]) == 'm')
        playerGender = 1;
    else
        playerGender = 0;
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    onPlanet = 0;
    ResetPlayer(1);
    ResetPlayerRecord(1);
    ResetGameData();
    for (i = 0; i < kMaxOutfits; i++)
        shipSystems[i] = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }
    ActivateCharResource(newPilotPopupChoice[0], 1);
    for (i = 0; i < kMaxSystems; i++)
        systemData[i].exploreLevel = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = shipClass[shipData->type].weapCount[i];
        if (shipClass[shipData->type].weapAmmo[i] > 0) {
            if (weaponStats[i].ammoType >= 0 && weaponStats[i].ammoType < kMaxWeapTypes)
                shipData->weap[weaponStats[i].ammoType].ammo =
                    shipClass[shipData->type].weapAmmo[i];
            else
                shipData->weap[i].ammo = shipClass[shipData->type].weapAmmo[i];
        }
    }
    ShipStatsToSystemInfo();
    for (k = 0; k < 8; k++) {
        if (shipClass[shipData->type].defOutfCount[k] > 0)
            shipSystems[shipClass[shipData->type].defOutfID[k]] +=
                shipClass[shipData->type].defOutfCount[k];
    }
    for (i = 0; i < kMaxSystemLinks; i++) {
        if (systemData[shipData->curSystem].links[i] != -1
            && systemData[systemData[shipData->curSystem].links[i]].loaded != 0) {
            sys = FindFirstCoLocatedSystem(systemData[shipData->curSystem].links[i]);
            if (sys >= 0 && sys < kMaxSystems)
                systemData[sys].exploreLevel = 1;
        }
    }
    sys = FindFirstCoLocatedSystem(shipData->curSystem);
    if (sys >= 0 && sys < kMaxSystems)
        systemData[sys].exploreLevel = 1;
    mapScroll.h = systemData[shipData->curSystem].pos.h;
    mapScroll.v = systemData[shipData->curSystem].pos.v;
    systemData[shipData->curSystem].reinforceTimer = -1.0f;
    systemData[shipData->curSystem].reinforceDaysLeft = 0;
    shipData->shield = ShipShieldCapacity(shipData);
    shipData->armor = ShipArmorCapacity(shipData);
    shipData->fuel = ShipFuelCapacity(shipData);
    LoadObjectData();
    DeleteAllComputerShips(1, 1);
    ExploreNebulaeFromSystem(shipData->curSystem);
    mapScroll.h = systemData[shipData->curSystem].pos.h;
    mapScroll.v = systemData[shipData->curSystem].pos.v;
    missionSoundID = -1;
    killShots = killBoxes = killExplods = killSmoke =
        killAsteroids = 1;
    pstrcpy(newShipName, textInputString);
    CullNameString(newShipName);
    pstrcpy(shipName, newShipName);
    pstrcpy(playerName, newPlayerName[0]);
    pstrcpy(playerNickname, shipNameP);
    gameTime = scenarioStartDate;
    gameInProgress = 1;
    strictPlay = wantStrictPlay;
    shipData->deathTimer = -1.0f;
    shipData->ionization = 0.0f;
    shipData->ionColor = 0;
    shipData->frameTimer = 0.0f;
    shipData->bankDir = 0;
    shipData->escapePods = -1;
    newbieFlag = -3;
    PropagateMissionBitEffects();
    sys = FindNearestStellar(shipData);
    if (sys == -1) {
        for (k = 0; k < kNavSlots; k++) {
            if (systemData[shipData->curSystem].navDefault[k] != -1) {
                sys = systemData[shipData->curSystem].navDefault[k];
                break;
            }
        }
    }
    for (i = 0; i < kMaxStellars; i++) {
        if ((stellarData[i].flags2 & kSpobAlwaysDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
        if ((stellarData[i].flags2 & kSpobStartsDestroyed) != 0) {
            stellarData[i].deadCounter = -1;
            if (stellarData[i].deadDays >= 0)
                stellarData[i].deadDaysLeft = stellarData[i].deadDays;
            else
                stellarData[i].deadDaysLeft = 1;
        } else {
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadDaysLeft = 0;
        }
    }
    ActivateCharResource(newPilotPopupChoice[0], 0);
    if (sys != -1)
        SavePilotFile(sys);
    else
        SavePilotFile(0);
    SetupShipsInSystem(shipData->curSystem, 1);
    HandleStellarSystemVisibility();
    CreateAsteroids();
#ifdef MACBUILD
    GetCharResourceIntroIDs(newPilotPopupChoice[0], &introTextID, introPictID, introPictDelay);
#else
    GetCharResourceIntroIDs(newPilotPopupChoice[0], gViewSlot, &gViewSlot[1], gViewSlotDuration);
#endif
    SWHideAllSprites(gSpriteWorld);
    creatingPilotFile = 0;
    SetupInterfaceBackground();
    MainScreenAnimDraw();
    MainScreenUpdate(1);
}
