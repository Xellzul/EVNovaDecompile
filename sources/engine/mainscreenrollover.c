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
PicHandle keyDialogPict;
unsigned char DAT_007d1f14[40];
short gSelectedBinding;
unsigned char DAT_007d1f3e[4];
short gViewSlot[5];
short gViewSlotDuration[4];
DialogPtr prefsDialog;
DialogPtr keyDialog;
unsigned char DAT_007d1f5c[4];
Rect standardPicRect;
Rect interfacePicRect;
unsigned char DAT_007d1f70[8];
Rect logoDestRect;
Rect rolloverDestRect;
Rect slideDestRect[3];
char didSpeak;
char doMainScreenAnim;
char majorPrefsChange;
char creatingPilotFile;
char gUpdateFlag;
char regNoticeShown;
char showingIntroText;
unsigned char DAT_007d1fa7[272];
Str255 newPlayerName[2];
Str255 newPilotPopupChoice[2];
void *mainScreenSnd[4];
ShortPoint mainMenuCenter;
ShortPoint menuButtonPos[6];
GWorldContainer gDlgWorld;
GWorldContainer gDlgWorlds[3];
short gMainPicFrame;
short gHilitedButton;
short gAnimStep[3];
short gHiliteLevel;
short gMainPicWidthDelta;
short gMainPicHeightDelta;
ShortPoint logoDestPoint;
ShortPoint rolloverDestPoint;
AnimDelta gAnimDelta[3];
ShortPoint slideDestPoint[3];
ShortPoint menuTopCorner;
short gLogoAnimEnd;
short gAnimEnd[3];
unsigned long gLastLogoAnimTick;
unsigned long gHiliteLastTick;
unsigned long gLastSlideAnimTick;
char mainScreenBackgroundDirty;
long gVersionCheckStart;
char gLatestVersion[256];
char gVersionCheckDone;
unsigned char DAT_007d2665[258];

/* MainScreenRolloverDraw. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c580
void MainScreenRolloverDraw(void)
{
    short idx;
    Rect *frameRect;
    Rect srcRect;
    short width;
    short height;

    if (menuRolloverSprite != NULL) {
        idx = gHilitedButton;
        if (gHilitedButton < 0)
            idx = 0;
        if (idx >= menuRolloverSprite->numFrames)
            idx = menuRolloverSprite->numFrames - 1;
        frameRect = &menuRolloverSprite->frameArray[idx]->frameRect;
        srcRect.top = frameRect->top;
        srcRect.left = frameRect->left;
        srcRect.bottom = frameRect->bottom;
        srcRect.right = frameRect->right;
        width = srcRect.right - srcRect.left;
        height = srcRect.bottom - srcRect.top;
        rolloverDestRect.right = rolloverDestRect.left + width;
        rolloverDestRect.bottom = rolloverDestRect.top + height;
        SWSetSpriteLocation(menuRolloverSprite, rolloverDestRect.left,
                            rolloverDestRect.top);
        menuRolloverSprite->translucency = kHiliteFull;
        menuRolloverSprite->tintRed = gHiliteLevel;
        menuRolloverSprite->tintGreen = gHiliteLevel;
        menuRolloverSprite->tintBlue = gHiliteLevel;
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

/* CrtInitHook4. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c720
void CrtInitHook4(void)
{
}

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
