// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -D_MSL_USE_INLINE=0 -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
// COMPILE-WHY: -D_MSL_USE_INLINE=0 (Win32Headers.pch): <ctype.h> declares tolower pure, so the name loop's gSystems load ranks invariant.
/* dosystemsearch.c -- DoSystemSearch and CrtInitHook7, the .CRT hook that ends its object; exe 0x004aab30..0x004aaf50. Notes: Research/notes/mapdlg.md */

#include <ctype.h>
#include <stddef.h>
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "sound.h"
#include "stellar.h"
#include "game.h"
#include "render.h"
#include "ships.h"
#include "macport.h"
#include "dialogs.h"

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is CrtInitHook7.  An empty constructor makes the compiler emit the hook, after every
   function wherever the object is defined; the object's byte sits in the block below. */
struct CrtHook { CrtHook() {} };

/* the map object's globals */
Image_t *redMapArrow;
Image_t *greenMapArrow;
void *mapCursorSwitchRgn;
PicHandle mapNebulaPicts[32][7];
PicHandle mapDialogPict;
char partialMapDialogUpdate;
char didUpdateMap;
char canZoomIn;
char canZoomOut;
char showClearRouteButton;
char multiArrowsOnSingleSystem;
char mapSystemDrawn[2048];
char influenceCoordsCached;
short missionSyst[32];
short greenMissionSyst;
short selSystem;
unsigned char DAT_007dcf8c[2];
short influenceStepSize;
short nextInfluenceSyst;
DialogPtr mapDialog;
static CrtHook crtHook;
unsigned char DAT_007dcf99[255];
unsigned char influenceValue[256][256][4];
unsigned char influenceOwner[256][256][4];

#define kSearchDlogID 0xbbd
#define kSearchTextItem 5

/* DoSystemSearch. STATE: MATCH, 280/280, byte-identical (1042 bytes), relocregcheck 0 differ.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004aab30
void DoSystemSearch(void)
{
    short itemHit;
    short itemType;
    void *item;
    Rect box;
    DialogPtr dlg;
    char searchNow;
    unsigned char rawText[256];
    unsigned char typedText[256];
    unsigned char candText[256];
    short i;
    short c;
    long rawLen;
    long nameLen;
    short runLen;
    short matchCount;
    short bestRun;
    short bestLen;
    short bestSystem;
    char ch;

    dlg = GetNewDialog(kSearchDlogID, NULL, NULL);
    if (dlg == NULL)
        return;

    SetDialogDefaultItem(dlg, 1);
    CenterWindow(FrontWindow(dlg), FrontWindow(mapDialog));
    ShowMacWindow(FrontWindow(dlg));
    FrontWindow(dlg);
    SetDialogGWorld(dlg);
    FrameDefaultButton(dlg, 1);

    GetDialogItem(dlg, kSearchTextItem, &itemType, &item, &box);
    SetDialogItemText((ControlPtr)item, (const unsigned char *)"\p");
    SelectDialogItemText(dlg, kSearchTextItem, 0, 1);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();

    rawText[0] = 0;
    searchNow = 0;
    while (!searchNow) {
        ModalDialog(0, &itemHit);
        if (itemHit == 1) {
            GetDialogItem(dlg, kSearchTextItem, &itemType, &item, &box);
            GetDialogItemText((ControlPtr)item, rawText);
            searchNow = 1;
        }
        if (itemHit == 3) {
            searchNow = 1;
        }
    }

    DisposeDialog(dlg);
    RedrawDialog(mapDialog);

    if (rawText[0] == 0)
        return;

    bestRun = -1;
    bestSystem = -1;
    bestLen = 0;
    matchCount = 0;
    typedText[0] = 0;
    rawLen = rawText[0];
    for (i = 0; i < rawLen; i++) {
        ch = tolower(rawText[i + 1]);
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            typedText[0]++;
            typedText[typedText[0]] = ch;
        }
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].loaded == 0)
            continue;
        if (gSystems[i].exploreLevel <= 0)
            continue;
        if (gSystems[i].known == 0)
            continue;

        candText[0] = 0;
        c = 0;
        nameLen = (char)gSystems[i].name[0];
        while (c < nameLen) {
            ch = tolower((char)gSystems[i].name[c + 1]);
            if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
                candText[0]++;
                candText[candText[0]] = ch;
            }
            c++;
        }

        runLen = 0;
        for (c = 0; c < typedText[0]; c++) {
            /* BUG: off by one, the run compare reads one character past the candidate's length */
            if (c > candText[0])
                break;
            if (typedText[c + 1] != candText[c + 1])
                break;
            runLen++;
        }

        if (runLen <= 0)
            continue;

        matchCount++;
        if (runLen > bestRun || (runLen == bestRun && candText[0] < bestLen)
            || bestSystem == -1) {
            bestSystem = i;
            bestRun = runLen;
            bestLen = candText[0];
        }
    }

    if (bestSystem != -1 && (bestRun > 1 || matchCount == 1)) {
        selSystem = bestSystem;
        mapScroll.x = gSystems[bestSystem].xPos;
        mapScroll.y = gSystems[bestSystem].yPos;
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    } else {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    }
}

/* CrtInitHook7. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aaf50
void CrtInitHook7(void)
{
}
