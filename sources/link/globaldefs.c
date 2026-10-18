// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* globaldefs.c -- the data globals whose type and extent the bucket run has SETTLED, defined here so the tree carries them instead of a generated placeholder. Notes: Research/notes/globaldefs.md */
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "burger.h"
#include "burgerchan.h"
#include "burgerddraw.h"
#include "reggiesession.h"
#include "crashlog.h"
#include "render.h"
#include "ships.h"
#include "game.h"
#include "dialogs.h"
#include "player.h"
#include "ai.h"
#include "gameutil.h"
#include "missions.h"
#include "stellar.h"

void *Win95MainWindow;
int gDisableUpdatesMode;
unsigned long ScreenWidth;
unsigned long ScreenHeight;
unsigned long VideoWidth;
SpriteRec *gSWCurrentSpriteBeingDrawn;
char gNeedUpdateFlag;
Word8 ResourceJustLoaded;

/* COMMON data of BurgerLib members the exe links: no live code reads it, the link keeps it. */
#define kJoystickCount  4
#define kJoystickAxes   6
#define kAxisEntries    5
Word JoystickPercent[kJoystickCount][kJoystickAxes];
Word JoystickPresent;
Word32 JoystickLastButtons[kJoystickCount];
Word32 JoystickLastButtonsDown[kJoystickCount];
Word JoystickBoundaries[kJoystickCount][kAxisEntries * kJoystickAxes];
Word Table8To16[256];
Word VideoTextureTypesAllowed;
Word VideoTextureRules;
Word VideoTextureMinWidth;
Word VideoTextureMaxWidth;
Word VideoTextureMinHeight;
Word VideoTextureMaxHeight;
Word VideoVertexMaxCount;
Word VideoVertexCount;
Word VideoUseColorZero;
Word VideoAPIInUse;
Word VideoPixelDoubleMode;
void *Direct3DPtr;
void *Direct3DDevicePtr;
void *Direct3DExecBufPtr;
Word8 *Direct3DExecDataBuf;
Word8 *Direct3DExecInstBuf;
Word8 *Direct3DExecDataPtr;
Word8 *Direct3DExecInstStartPtr;
Word32 Direct3DExecBuffSize;
void *Direct3DZBufferPtr;
Word Direct3DZBufferBitDepth;
Word BurgerPaletteDirty;
Word PaletteVSync;
void *PaletteFadeCallBack;
Word *SoundCookiePtr;
int SoundFrequencyAdjust;
void *MyDirectSoundBuffer;
Word DebugTraceFlag;
Word32 MasterRezHeader[9];

/* COMMON cells the exe's tail holds that no code reads; names and their evidence: Research/notes/globaldefs.md */
void *gNT_LookupCache;
void *gIT_EventProc;
void *BurgerSndGameApp;
long gLegacyVar;
char gUnusedFlag;
