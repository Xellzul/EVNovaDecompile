// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* mainscreenrollover.c -- MainScreenRolloverDraw and CrtInitHook4, the .CRT hook that ends its object; exe 0x0048c580..0x0048c720. Notes: Research/notes/mainscreen.md */

#include <stddef.h>
#include "prototypes.h"
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
/* the main-screen object's globals; this file ends that object (the .CRT hook) */
PicHandle keyPic;
#ifndef MACBUILD
unsigned char DAT_007d1f14[40];
#endif
short keyDialogSel;
#ifdef MACBUILD
short introTextID;
short introPictID[4];
short introPictDelay[4];
#else
unsigned char DAT_007d1f3e[4];
short gViewSlot[5];
short gViewSlotDuration[4];
#endif
DialogPtr prefsDialog;
DialogPtr keyDialog;
#ifndef MACBUILD
unsigned char DAT_007d1f5c[4];
#endif
Rect standardPicRect;
Rect interfacePicRect;
#ifndef MACBUILD
unsigned char DAT_007d1f70[8];
#endif
Rect logoDestRect;
Rect rolloverDestRect;
Rect slideDestRect[3];
Boolean didSpeak;
Boolean doMainScreenAnim;
#if !defined(MACBUILD) || defined(MAC1010)
Boolean majorPrefsChange;
#endif
Boolean creatingPilotFile;
#ifdef MACBUILD
#ifndef MAC1010 /* 1.0.10: at the head of Main Screen.c */
Boolean gUpdateFlag = 0;
#endif
#else
Boolean gUpdateFlag;
#endif
Boolean regNoticeShown;
Boolean showingIntroText;
#ifdef MACBUILD
EventRecord gEvent;
#else
unsigned char DAT_007d1fa7[272];
#endif
Str255 newPlayerName[2];
Str255 newPilotPopupChoice[2];
void *mainScreenSnd[4];
ShortPoint mainMenuCenter;
ShortPoint menuButtonPos[6];
GWorldContainer logoPort;
GWorldContainer slidePort[3];
short logoIndex;
short rolloverIndex;
short slideIndex[3];
short rolloverAlpha;
#ifdef MACBUILD
AnimDelta logoFrameSize;
#else
short gMainPicWidthDelta;
short gMainPicHeightDelta;
#endif
ShortPoint logoDestPoint;
ShortPoint rolloverDestPoint;
AnimDelta slideFrameSize[3];
ShortPoint slideDestPoint[3];
ShortPoint menuTopCorner;
short logoFrameCount;
short slideFrameCount[3];
unsigned long lastLogoAnimTime;
unsigned long lastRolloverAnimTime;
unsigned long lastSlideAnimTime;
Boolean mainScreenBackgroundDirty;
#ifdef MACBUILD
#ifndef MAC1010 /* 1.0.10: at the head of Main Screen.c */
long reggieStarted = 0;
char reggieVersion[256] = "";
Boolean reggieSuccess = 0;
#endif
#else
long reggieStarted;
char reggieVersion[256];
Boolean reggieSuccess;
#endif
#ifndef MACBUILD
unsigned char DAT_007d2665[258];
#endif

/* MainScreenRolloverDraw. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c580
// MAC: _MainScreenRolloverDraw i386 0x00017ad4 ppc 0x0001b76c
void MainScreenRolloverDraw(void)
{
    short idx;
    Rect srcRect;

    if (menuRolloverSprite != NULL) {
        idx = rolloverIndex;
        if (rolloverIndex < 0)
            idx = 0;
        if (idx >= menuRolloverSprite->numFrames)
            idx = menuRolloverSprite->numFrames - 1;
        srcRect = menuRolloverSprite->frameArray[idx]->frameRect;
        rolloverDestRect.right = rolloverDestRect.left + (srcRect.right - srcRect.left);
        rolloverDestRect.bottom = rolloverDestRect.top + (srcRect.bottom - srcRect.top);
        SWSetSpriteLocation(menuRolloverSprite, rolloverDestRect.left,
                            rolloverDestRect.top);
        menuRolloverSprite->translucency = kHiliteFull;
        menuRolloverSprite->tintRed = rolloverAlpha;
        menuRolloverSprite->tintGreen = rolloverAlpha;
        menuRolloverSprite->tintBlue = rolloverAlpha;
        menuRolloverSprite->tintMode = 0;
        menuRolloverSprite->tintColor = 0;
        gSWCurrentSpriteBeingDrawn = menuRolloverSprite;

        if (gSpriteWorld->pixelDepth == kDepth16) {
            BlitPixieRLETranslucentDrawProc(menuRolloverSprite->frameArray[idx],
                gSpriteWorld->workFrameP, &srcRect, &rolloverDestRect);
        } else {
            BlitPixieRLEDrawProc(menuRolloverSprite->frameArray[idx],
                gSpriteWorld->workFrameP, &srcRect, &rolloverDestRect);
        }
    }
}

#ifndef MACBUILD
/* CrtInitHook4. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c720
void CrtInitHook4(void)
{
}
#endif

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
#endif
