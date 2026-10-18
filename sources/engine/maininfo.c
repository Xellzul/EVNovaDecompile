// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
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
#include "space.h"

/* gameview.c's type, and its +0x4c is why the exe derefs twice here */

/* MainScreenUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004873b0
void MainScreenUpdate(short which)
{
    unsigned char text[256];
    Rect box;
    char shown[8];
    Rect dst;
    Rect *wr;
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
    RGBForeColor(blackColor);
    if (which == 2) {
        CopyBits(GetGWCPixMap(&loadPort),
                 GetPortBitMapForCopyBits(gSpriteWorld->workFrameP->framePort),
                 &rolloverDestRect, &rolloverDestRect, 0, NULL);
        for (i = 0; i < 3; i++) {
            if (gDlgWorlds[i].world != NULL && gAnimStep[i] < gAnimEnd[i]) {
                CopyBits(GetGWCPixMap(&loadPort),
                         GetPortBitMapForCopyBits(
                             gSpriteWorld->workFrameP->framePort),
                         &slideDestRect[i], &slideDestRect[i], 0, NULL);
            }
        }
    } else {
        CopyBits(GetGWCPixMap(&loadPort),
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
        if (gameInProgress == 0) {
            if (gAnimStep[2] >= gAnimEnd[2]) {
                RGBForeColor(labelColor);
                GetIndString(text, kGameStrList, 0x114);
                DrawStringCentered(text, (mainMenuCenter.h - 0x96),
                                   (mainMenuCenter.h + 0x96),
                                   (mainMenuCenter.v + 0x136));
            }
        } else if (IsDying(shipData) == 0) {
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
            DrawString(gShipTypes[shipData->type].name);
            MoveTo((mainMenuCenter.h - 0xb9), (mainMenuCenter.v + 0x15a));
            RGBForeColor(valueColor);
            DrawString(gShipTypes[shipData->type].subtitle);
            RGBForeColor(labelColor);
            MoveTo((mainMenuCenter.h + 0x78), (mainMenuCenter.v + 0xfa));
            DrawIndString(kGameStrList, 0x116);
            MoveTo((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x106));
            DrawIndString(kGameStrList, 0x117);
            MoveTo((mainMenuCenter.h + 0x7d), (mainMenuCenter.v + 0x112));
            RGBForeColor(valueColor);
            if (SystemIsInhabited(shipData->curSystem) == 0)
                DrawIndString(kGameStrList, 0x18c);
            else
                DrawLegalStatusString(shipData->curSystem);
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
            RGBForeColor(blackColor);
            i = gShipTypes[shipData->type].targetPictID;
            if (targetInfoPic[i] != NULL) {
                SWSetPortToBackground(gSpriteWorld);
                RGBForeColor(blackColor);
                QDGetPictureBounds(targetInfoPic[i], &box);
                OffsetRect(&box, -box.left, -box.top);
                OffsetRect(&box,
                           mainMenuCenter.h - (box.right - box.left) / 2,
                           mainMenuCenter.v + 0x118);
                DrawPicHandle(targetInfoPic[i], &box);
                SWSetPortToWorkArea(gSpriteWorld);
                RGBForeColor(blackColor);
                CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                             gSpriteWorld->backFrameP->framePort),
                         GetPortBitMapForCopyBits(
                             gSpriteWorld->workFrameP->framePort),
                         &box, &box, 0x22, NULL);
            }
        } else {
            RGBForeColor(labelColor);
            if ((int)playerName[1] == 'K' && (int)playerName[2] == 'e' &&
                (int)playerName[3] == 'n' && (int)playerName[4] == 'n' &&
                (int)playerName[5] == 'y') {
                DrawStringCentered("\pOh my God! They killed Kenny!",
                                   (mainMenuCenter.h - 0x96),
                                   (mainMenuCenter.h + 0x96),
                                   (mainMenuCenter.v + 0x136));
            } else {
                pstrcpy(text, playerName);
                PtoCstr(text);
                strcat((char *)text, " ");
                AppendIndString((char *)text, kGameStrList, 0x115);
                CtoPstr((char *)text);
                DrawStringCentered(text, (mainMenuCenter.h - 0x96),
                                   (mainMenuCenter.h + 0x96),
                                   (mainMenuCenter.v + 0x136));
            }
        }
    }

    if (gAnimStep[0] >= gAnimEnd[0])
        shown[0] = shown[3] = 1;
    else
        shown[0] = shown[3] = 0;
    if (gAnimStep[1] >= gAnimEnd[1])
        shown[1] = shown[4] = 1;
    else
        shown[1] = shown[4] = 0;
    if (gAnimStep[2] >= gAnimEnd[2])
        shown[2] = shown[5] = 1;
    else
        shown[2] = shown[5] = 0;

    for (i = 0; i < 6; i++) {
        if (gMainButtonSprites[i] != NULL && shown[i] != 0) {
            wr = &gMainButtonSprites[i]->frameArray[0]->frameRect;
            box.top = wr->top;
            box.left = wr->left;
            box.bottom = wr->bottom;
            box.right = wr->right;
            OffsetRect(&box, menuButtonPos[i].h + menuTopCorner.h,
                       menuButtonPos[i].v + menuTopCorner.v);
            SWSetSpriteLocation(gMainButtonSprites[i],
                         (menuButtonPos[i].h + menuTopCorner.h),
                         (menuButtonPos[i].v + menuTopCorner.v));
            BlitPixieRLEDrawProc(
                gMainButtonSprites[i]->frameArray[0],
                gSpriteWorld->workFrameP, &box, &box);
        }
    }
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
    for (i = 0; i < 6; i++) {
        if (gMainButtonSprites[i] != NULL && shown[i] != 0) {
            wr = &gMainButtonSprites[i]->frameArray[0]->frameRect;
            box.top = wr->top;
            box.left = wr->left;
            box.bottom = wr->bottom;
            box.right = wr->right;
            OffsetRect(&box, menuButtonPos[i].h + menuTopCorner.h,
                       menuButtonPos[i].v + menuTopCorner.v);
            SWSetSpriteLocation(gMainButtonSprites[i],
                         (menuButtonPos[i].h + menuTopCorner.h),
                         (menuButtonPos[i].v + menuTopCorner.v));
            BlitPixieRLEDrawProc(
                gMainButtonSprites[i]->frameArray[0],
                gSpriteWorld->workFrameP, &box, &box);
        }
    }
    MainScreenRolloverDraw();
    if (which == 1) {
        CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                     gSpriteWorld->workFrameP->framePort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                 &gSpriteWorld->workFrameP->frameRect,
                 &gSpriteWorld->workFrameP->frameRect, 0, NULL);
        /* and the 0xc2-wide status strip, loadPort -> the window's right edge. */
        box.top = loadPort.bounds.top;
        box.left = loadPort.bounds.left;
        box.bottom = loadPort.bounds.bottom;
        box.right = loadPort.bounds.right;
        box.left = (box.right - 0xc2);
        wr = GetCWindowPortRect(gWind);
        dst.top = wr->top;
        dst.left = wr->left;
        dst.bottom = wr->bottom;
        dst.right = wr->right;
        dst.left = (dst.right - 0xc2);
        CopyBits(GetGWCPixMap(&loadPort),
                 GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                 &box, &dst, 0, NULL);
    }
}

#define kShippingVer    "1.0.A"
#define kVersionStrList 0x8c
#define kUpgradePrompt  0x33
#define kUrlStrList     0x82
#define kUrlIndex       6
#define kPollTimeout    5
#define kHotStripHeight 0x14
#define kBarHeight      0x1e

/* MainScreenIdlePoll. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00488080
void MainScreenIdlePoll(void)
{
    char wanted;
    void *reply;
    char *latest;
    Rect *portRect;
    int now;
    Point mouse;
    unsigned char text[256];
    Rect strip;

    if (gVersionCheckStart != 0 && gLatestVersion[0] == 0 &&
        gAnimStep[2] >= kAnimDone && inBackground == 0) {
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
                strcpy(gLatestVersion, latest);
                gVersionCheckDone = 1;
                gVersionCheckStart = 0;
                if (memcmp(gLatestVersion, kShippingVer,
                           sizeof(kShippingVer)) > 0) {
                    GetIndString(text, kVersionStrList, kUpgradePrompt);
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
        if (gVersionCheckDone == 0) {
            now = TimerGetSeconds();
            if ((unsigned int)(now - gVersionCheckStart) > kPollTimeout) {
                crashlog("timeout contacting version server");
                gVersionCheckStart = 0;
            }
        }
    }
    HandleIntroMusic(0);
    TrackMainScreenAnim();
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
               windowedPref == 0) {
        HideTheMenuBar();
        GetCWindowPortRect(gWind);
        gNeedUpdateFlag = 1;
        SetPortWindowPort(gWind);
        RGBForeColor(blackColor);
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

/* SetupInterfaceBackground (mac _SetupInterfaceBackground). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00488380
void SetupInterfaceBackground(void)
{
    void *pic;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    pic = GetPicture(kInterfacePictID);
    if (pic != 0) {
        interfacePicRect.top = loadPort.bounds.top;
        interfacePicRect.left = loadPort.bounds.left;
        interfacePicRect.bottom = loadPort.bounds.bottom;
        interfacePicRect.right = loadPort.bounds.right;
        CenterPict(pic, &interfacePicRect);
        if (loadPort.bounds.bottom - loadPort.bounds.top < kMinFullHeight)
            OffsetRect(&interfacePicRect, 0, -kSmallScreenNudge);
        if (loadPort.bounds.right - loadPort.bounds.left <= kMinFullWidth)
            OffsetRect(&interfacePicRect, -kSmallScreenNudge, 0);
        mainMenuCenter.h = (interfacePicRect.left + interfacePicRect.right) / 2;
        mainMenuCenter.v = (interfacePicRect.top + interfacePicRect.bottom) / 2;
        menuTopCorner.h = mainMenuCenter.h - kMenuHalfWidth;
        menuTopCorner.v = mainMenuCenter.v - kMenuHalfHeight;
        RGBForeColor(blackColor);
        PaintRect(&loadPort.bounds);
        DrawPicHandle(pic, &interfacePicRect);
        QDReleaseResource(pic);
    }
    SWSetPortToWindow(gSpriteWorld);
}


#define kPlainStrIdx  0x118
#define kItem3StrList 0x8c
#define kItem3StrIdx  0x10

/* PrefsDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004884f0
void PrefsDialogUpdate(void)
{
    Rect box;

    SetDialogGWorld(prefsDialog);
    GetDialogItem(prefsDialog, 0x13, NULL, NULL, &box);
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
        DrawIndString(kItem3StrList, kItem3StrIdx);
    }
}

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
    char done;
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

    savedShipSets = loadShipSetsPref;
    savedShipGlow = loadShipGlowPref;
    savedShipWeapons = loadShipWeaponsPref;
    savedShipLights = loadShipLightsPref;
    done = 0;
    savedGamma = gammaPref;

    GetDialogItem(prefsDialog, kItemFullScreen, NULL, &item, &box);
    DisableControl(item);
    GetDialogItem(prefsDialog, kItemMessages, NULL, &item, &box);
    DisableControl(item);

    do {
        SetDialogGWorld(prefsDialog);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(blackColor);

        GetDialogItem(prefsDialog, kItemFullScreen, &itemType, &item, &box);
        ctl = item;
        if (DAT_007355d0 >= kTrueColorDepth) {
            SetControlValue(ctl, 1);
        } else if (yieldEventsPref != 0) {
            SetControlValue(ctl, 1);
        } else {
            SetControlValue(ctl, 0);
        }

        if (windowedPref != 0 && gameLoaded != 0) {
            GetDialogItem(prefsDialog, kItemMessages, &itemType, &item, &box);
            SetControlValue(item, 0);
        }

        GetDialogItem(prefsDialog, kItemMusic, &itemType, &item, &box);
        ctl = item;
        if (musicEnabledPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemSmoothing, &itemType, &item, &box);
        ctl = item;
        if (windowedPrefPending != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemEarlyOut, &itemType, &item, &box);
        ctl = item;
        if (gEarlyOutFlag == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemNoDebris, &itemType, &item, &box);
        ctl = item;
        if (gNoDebris == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipSets, &itemType, &item, &box);
        ctl = item;
        if (loadShipSetsPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipGlow, &itemType, &item, &box);
        ctl = item;
        if (loadShipGlowPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipLights, &itemType, &item, &box);
        ctl = item;
        if (loadShipLightsPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemShipWeapons, &itemType, &item, &box);
        ctl = item;
        if (loadShipWeaponsPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemStatusBar, &itemType, &item, &box);
        ctl = item;
        if (parallaxStarsPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemAutoAim, &itemType, &item, &box);
        ctl = item;
        if (planetSoundsPref != 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemMessages, &itemType, &item, &box);
        ctl = item;
        if (noHyperGammaPref == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);
        GetDialogItem(prefsDialog, kItemNetwork, &itemType, &item, &box);
        ctl = item;
        if (noVersionCheckPref == 0)
            SetControlValue(ctl, 1);
        else
            SetControlValue(ctl, 0);

        GetDialogItem(prefsDialog, kItemMusicVolText, &itemType, &item, &box);
        musicVol = volumePref;
        if ((short)musicVol < 0 || (short)musicVol > kMaxMusicVol) {
            SetDialogItemText(item, "\p???");
        } else {
            GetIndString(volText, kMusicVolStrList, ((short)musicVol + 1));
            SetDialogItemText(item, volText);
        }

        GetDialogItem(prefsDialog, kItemGammaText, NULL, &item, &box);
        if (item != NULL) {
            gammaLevel = gammaPref;
            if ((short)gammaLevel > kMaxGamma) {
                SetDialogItemText(item, "\p???");
            } else {
                GetIndString(volText, kGammaStrList, ((short)gammaLevel + 1));
                SetDialogItemText(item, volText);
            }
        }

        majorPrefsChange = 0;
        if (windowedPrefPending != windowedPref)
            majorPrefsChange = 1;
        if (gameLoaded != 0) {
            if (loadShipSetsPref != savedShipSets || loadShipGlowPref != savedShipGlow
                || loadShipLightsPref != savedShipLights || loadShipWeaponsPref != savedShipWeapons
                || gammaPref != savedGamma)
                majorPrefsChange = 1;
        }

        ModalDialog(NULL, &itemHit);

        if (itemHit == kItemDone) {
            SavePrefs();
            done = 1;
        }
        if (itemHit == kItemMusic) {
            musicEnabledPref = (musicEnabledPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemFullScreen && DAT_007355d0 < kTrueColorDepth) {
            yieldEventsPref = (yieldEventsPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemSmoothing) {
            windowedPrefPending = (windowedPrefPending == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemEarlyOut) {
            gEarlyOutFlag = (gEarlyOutFlag == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemNoDebris) {
            gNoDebris = (gNoDebris == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipSets) {
            loadShipSetsPref = (loadShipSetsPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipGlow) {
            loadShipGlowPref = (loadShipGlowPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipLights) {
            loadShipLightsPref = (loadShipLightsPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemShipWeapons) {
            loadShipWeaponsPref = (loadShipWeaponsPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemStatusBar) {
            parallaxStarsPref = (parallaxStarsPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemAutoAim) {
            planetSoundsPref = (planetSoundsPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemMessages) {
            noHyperGammaPref = (noHyperGammaPref == 0);
            RedrawDialog(prefsDialog);
        }
        if (itemHit == kItemNetwork) {
            noVersionCheckPref = (noVersionCheckPref == 0);
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
            if (volumePref < kMaxMusicVol) {
                volumePref += 1;
                RedrawDialog(prefsDialog);
            } else {
                volumePref = kMaxMusicVol;
            }
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        }
        if (itemHit == kItemMusicDown) {
            if (volumePref > 0) {
                volumePref -= 1;
                RedrawDialog(prefsDialog);
            } else {
                volumePref = 0;
            }
            HandleIntroMusic(1);
            AdjustSoundVolume();
            PlaySound(beepSnd[4], kBeepChannel, gSoundVolume, gSoundVolume);
        }
        if (itemHit == kItemGammaUp) {
            if (gammaPref < kMaxGamma) {
                gammaPref += 1;
                RedrawDialog(prefsDialog);
            } else {
                gammaPref = kMaxGamma;
            }
        }
        if (itemHit == kItemGammaDown) {
            if (gammaPref > 0) {
                gammaPref -= 1;
                RedrawDialog(prefsDialog);
            } else {
                gammaPref = 0;
            }
        }
    } while (done == 0);

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    DisposeDialog(prefsDialog);
    prefsDialog = NULL;
    if (musicEnabledPref == 0)
        StopIntroMusic(0);
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* entergame -- EnterGameFromMainScreen and DoNewPilot. Notes: Research/notes/entergame.md */

/* far arrays inside the one big 0x005912a0 allocation, reached at raw offsets
   because no header names them yet */

/* RT3 library string helpers, NOT MSL: the game's strlen is BurgerLib's FastStrLen
   0x004eedb0, so these keep FUN_ names. */

/* EnterGameFromMainScreen (mac _EnterGameFromMainScreen). STATE: MATCH 100.0%, exe 763 / obj 763,
   byte-identical yes (2912 bytes, relocs masked), relocregcheck 0 of 194.
   OPEN: nothing. */
// FUNCTION: EVNOVA 0x00489210
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
    *(void **)((char *)shipData + kLicenseName) = RT3_GetLicenseName();
    StopDialogSounds();
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
    *(long long *)((char *)shipData + kLicenseCodePair) = RT3_GetLicenseCode();
    crashlog("drawing stuff");
    LoadIntfResource();
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    RGBForeColor(blackColor);
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    if (!windowedPref)
        HideTheMenuBar();
    RandomizeStars();
    PropagateMissionBitEffects();
    if (gSystems[shipData->curSystem].murk < 0)
        ClearStarfield();
    DrawMapInFloatingFrame();
    *(long *)((char *)shipData + kLicenseCopies) = RT3_GetLicenseCopies();
    crashlog("drawing more stuff");
    gShipTypes[gameIsRegisteredIndex].registered =
        *(long *)((char *)shipData + kLicenseCopies) ? (unsigned char)1 : (unsigned char)0;
    ResetPlayerPrecalcedValues();

    {
        unsigned char blob[3] = { 0x49, 0x71, 0xad };
        unsigned char name[17] = "EV NovaEV NovaEV";
        unsigned char key[17] = "";
        long g;
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
        char ok;

        g = *(long *)((char *)shipData + kLicenseCopies);
        p = *(unsigned char **)((char *)shipData + kLicenseName);
        mem = *(long long *)((char *)shipData + kLicenseCodePair);
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

        gShipTypes[gameIsHackedIndex].hacked =
            (*(long *)((char *)shipData + kLicenseCopies) != 0 && !ok)
                ? (unsigned char)1 : (unsigned char)0;
    }

    HandleStellarSystemVisibility();
    HandleStellarSprites();
    gSharewareDays = RT3_GetDaysHad();
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
    statusRedraw[2] = 1;
    escortMenuOnscreen = 0;
    mBarHidden = 1;
    gPause = 0;
    SWSetPortToWindow(gSpriteWorld);
    AdjustSoundVolume();
    fpsCounter = 0;
    lastFPSUpdateTime = ReadTick();
    DisposeDialogGWorlds();
    OSCursorReset();
    cursorVisible = 0;
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
    if (!windowedPref)
        HideTheMenuBar();
    if (strictPlay && IsDying(shipData) && shipData->type != kShipTypeEscapePod) {
        crashlog("deleting pilot file due to strict play death");
        pstrcpy(pilotName, playerName);
        DeletePilotFile(PtoCstr(pilotName));
    }
    crashlog("restoring stuff");
    KillVoiceSound();
    RestoreOrigClut();
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
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
    if (!windowedPref)
        HideTheMenuBar();
    SWSetPortToWindow(gSpriteWorld);
    RGBForeColor(blackColor);
    PaintRect(GetCWindowPortRect(gWind));
    if (!gQuit) {
        crashlog("showing main screen again");
        GetCWindowPortRect(gWind);
        gNeedUpdateFlag = 1;
        MainScreenIdle();
        LoadMainScreenSounds();
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

#define kNewNameStrList  128
#define kOverwriteList   140

/* DoNewPilot (mac _DoNewPilot). STATE: MATCH 100.0%, 416 exact / 208 reloc-equivalent /
   0 mismatched, exe 624 / obj 624, relocregcheck 0 of 208; byte-identical yes (2669 bytes,
   relocs masked).
   OPEN: none */
// FUNCTION: EVNOVA 0x00489d70
void DoNewPilot(void)
{
    unsigned long resType;
    char wantStrictPlay;
    Str255 newShipName;
    char path[2048];
    Str255 charName;
    Str255 prompt;
    char **h;
    unsigned char *shipNameP;
    short i;
    short j;
    short sys;
    short k;

    SWSetPortToWindow(gSpriteWorld);
    GetCWindowPortRect(gWind);
    wantStrictPlay = 0;
    gNeedUpdateFlag = 1;
    GetIndString(newPlayerName[0], kNewNameStrList, Rand(3) + 1);
    shipNameP = newPlayerName[1];
    GetIndString(shipNameP, kNewNameStrList, Rand(3) + 4);
    if (!DoNewPilotNameDialog(&wantStrictPlay))
        return;
    MainScreenAnimDraw();
    MainScreenUpdate(1);
    CullNameString(newPlayerName[0]);
    CullNameString(shipNameP);
    StripNameComment(newPlayerName[0]);
    StripNameComment(shipNameP);
    strcpy(path, pilotsFolder);
    PStr2CStr((char *)prompt, newPlayerName[0]);
    strcat(path, (char *)prompt);
    strcat(path, ".plt");
    if (DoesPilotExist(path)) {
        SWSetPortToWindow(gSpriteWorld);
        MainScreenAnimDraw();
        MainScreenUpdate(1);
        GetIndString(prompt, kOverwriteList, 9);
        if (!EVYesNoDialog(prompt))
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
    i = GetCharacterShipIndex(newPilotPopupChoice[0]);
    strcpy((char *)prompt, "");
    AppendIndString((char *)prompt, kGameStrList, 121);
    strcat((char *)prompt, " ");
    pstrcpy(charName, gShipLongNames[i]);
    PtoCstr(charName);
    strcat((char *)prompt, (char *)charName);
    strcat((char *)prompt, ": ");
    CtoPstr((char *)prompt);
    GetIndString(charName, kNewNameStrList, Rand(3) + 7);
    if (!EVTextInputDialog(prompt, charName, 64))
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
    i = 0;
    for (j = 0; j < 512; j++)
        shipSystems[j] = 0;
    for (i = 0; i < 256; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }
    ActivateCharResource(newPilotPopupChoice[0], 1);
    for (i = 0; i < 2048; i++)
        gSystems[i].exploreLevel = 0;
    for (i = 0; i < 256; i++) {
        shipData->weap[i].count = gShipTypes[shipData->type].weapCount[i];
        if (gShipTypes[shipData->type].weapAmmo[i] > 0) {
            if (gWeapons[i].ammoType >= 0 && gWeapons[i].ammoType < 256)
                shipData->weap[gWeapons[i].ammoType].ammo =
                    gShipTypes[shipData->type].weapAmmo[i];
            else
                shipData->weap[i].ammo = gShipTypes[shipData->type].weapAmmo[i];
        }
    }
    ShipStatsToSystemInfo();
    for (k = 0; k < 8; k++) {
        if (gShipTypes[shipData->type].defOutfCount[k] > 0)
            shipSystems[gShipTypes[shipData->type].defOutfID[k]] +=
                gShipTypes[shipData->type].defOutfCount[k];
    }
    for (i = 0; i < kMaxSystemLinks; i++) {
        if (gSystems[shipData->curSystem].links[i] != -1
            && gSystems[gSystems[shipData->curSystem].links[i]].loaded != 0) {
            sys = FindFirstCoLocatedSystem(gSystems[shipData->curSystem].links[i]);
            if (sys >= 0 && sys < 2048)
                gSystems[sys].exploreLevel = 1;
        }
    }
    sys = FindFirstCoLocatedSystem(shipData->curSystem);
    if (sys >= 0 && sys < 2048)
        gSystems[sys].exploreLevel = 1;
    mapScroll.x = gSystems[shipData->curSystem].xPos;
    mapScroll.y = gSystems[shipData->curSystem].yPos;
    gSystems[shipData->curSystem].reinforceTimer = -1.0f;
    gSystems[shipData->curSystem].reinforceDaysLeft = 0;
    shipData->shield = ShipShieldCapacity(shipData);
    shipData->armor = ShipArmorCapacity(shipData);
    shipData->fuel = ShipFuelCapacity(shipData);
    LoadObjectData();
    DeleteAllComputerShips(1, 1);
    ExploreNebulaeFromSystem(shipData->curSystem);
    mapScroll.x = gSystems[shipData->curSystem].xPos;
    mapScroll.y = gSystems[shipData->curSystem].yPos;
    missionSoundID = -1;
    killShots = killBoxes = killExplods = killSmoke =
        killAsteroids = 1;
    pstrcpy(newShipName, gTextInputResult);
    CullNameString(newShipName);
    pstrcpy(shipName, newShipName);
    pstrcpy(playerName, newPlayerName[0]);
    pstrcpy(playerNickname, shipNameP);
    gameTime.year = scenarioStartDate.year;
    gameTime.month = scenarioStartDate.month;
    gameTime.day = scenarioStartDate.day;
    gameTime.hour = scenarioStartDate.hour;
    gameTime.minute = scenarioStartDate.minute;
    gameTime.second = scenarioStartDate.second;
    gameTime.dayOfWeek = scenarioStartDate.dayOfWeek;
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
            if (gSystems[shipData->curSystem].navDefault[k] != -1) {
                sys = gSystems[shipData->curSystem].navDefault[k];
                break;
            }
        }
    }
    for (i = 0; i < 2048; i++) {
        if ((stellarData[i].flags2 & 0x20) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
        if ((stellarData[i].flags2 & 0x40) != 0) {
            stellarData[i].deadCounter = -1;
            if (stellarData[i].deadDays < 0)
                stellarData[i].deadDaysLeft = 1;
            else
                stellarData[i].deadDaysLeft = stellarData[i].deadDays;
        } else {
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadDaysLeft = 0;
        }
    }
    ActivateCharResource(newPilotPopupChoice[0], 0);
    if (sys == -1)
        SavePilotFile(0);
    else
        SavePilotFile(sys);
    SetupShipsInSystem(shipData->curSystem, 1);
    HandleStellarSystemVisibility();
    CreateAsteroids();
    GetCharResourceIntroIDs(newPilotPopupChoice[0], gViewSlot, &gViewSlot[1], gViewSlotDuration);
    SWHideAllSprites(gSpriteWorld);
    creatingPilotFile = 0;
    SetupInterfaceBackground();
    MainScreenAnimDraw();
    MainScreenUpdate(1);
}
