// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -D_MSL_USE_INLINE=0 -lang c++ -bool off
// COMPILE-WHY: -D_MSL_USE_INLINE=0 (Win32Headers.pch): <ctype.h> declares tolower pure, so the name loop's systemData load ranks invariant.
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
#include "cefix.h"

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is CrtInitHook7.  An empty constructor makes the compiler emit the hook, after every
   function wherever the object is defined; the object's byte sits in the block below. */
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
#endif

/* the map object's globals */
Image_t *redMapArrow;
Image_t *greenMapArrow;
void *mapCursorSwitchRgn;
PicHandle nebulaPic[32][7];
PicHandle mapBackgroundPic;
Boolean partialMapDialogUpdate;
Boolean didUpdateMap;
Boolean canZoomIn;
Boolean canZoomOut;
Boolean showClearRouteButton;
Boolean multiArrowsOnSingleSystem;
Boolean doneFlag[2048];
Boolean influenceCoordsCached;
short missionSyst[32];
short greenMissionSyst;
short selSystem;
#ifndef MACBUILD
unsigned char DAT_007dcf8c[2];
#endif
short influenceStepSize;
short nextInfluenceSyst;
DialogPtr mapDialog;
#ifndef MACBUILD
static CrtHook crtHook;
#endif
#ifndef MACBUILD
unsigned char DAT_007dcf99[255];
#endif
#ifdef CEFIX
unsigned char influenceValue[kCEBordersWidth][kCEBordersHeight][4];
unsigned char influenceOwner[kCEBordersWidth][kCEBordersHeight][4];
#else
unsigned char influenceValue[256][256][4];
unsigned char influenceOwner[256][256][4];
#endif

#define kSearchDlogID 0xbbd
#define kSearchTextItem 5

/* DoSystemSearch. STATE: MATCH, 280/280, byte-identical (1042 bytes), relocregcheck 0 differ.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004aab30
// MAC: _DoSystemSearch i386 0x00011c86 ppc 0x000156dc
void DoSystemSearch(void)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    short itemHit;
    short itemType;
#else
    short itemType;
    short itemHit;
#endif
    Handle item;
    Rect box;
    DialogPtr dlg;
    Boolean searchNow;
    unsigned char rawText[256];
    unsigned char typedText[256];
    unsigned char candText[256];
    short i;
    short c;
    short runLen;
    short matchCount;
    short bestRun;
    short bestSystem;
    short bestLen;
    char ch;

    dlg = GetNewDialog(kSearchDlogID, NULL, NULL);
    if (dlg == NULL)
        return;

    SetDialogDefaultItem(dlg, 1);
    CenterWindow(FrontWindow(dlg), FrontWindow(mapDialog));
    ShowMacWindow(FrontWindow(dlg));
    SelectWindow(FrontWindow(dlg));
    SetDialogGWorld(dlg);
    FrameDefaultButton(dlg, 1);

    GetDialogItem(dlg, kSearchTextItem, &itemType, &item, &box);
    SetDialogItemText((Handle)item, (const unsigned char *)"\p");
    SelectDialogItemText(dlg, kSearchTextItem, 0, 1);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();

    rawText[0] = 0;
    searchNow = 0;
    do {
        ModalDialog(0, &itemHit);
        if (itemHit == 1) {
            GetDialogItem(dlg, kSearchTextItem, &itemType, &item, &box);
            GetDialogItemText((Handle)item, rawText);
            searchNow = 1;
        }
        if (itemHit == 3) {
            searchNow = 1;
        }
    } while (!searchNow);

    DisposeDialog(dlg);
    RedrawDialog(mapDialog);

    if (rawText[0] == 0)
        return;

    bestRun = -1;
    bestSystem = -1;
    bestLen = 0;
    matchCount = 0;
    typedText[0] = 0;
    for (c = 0; c < rawText[0]; c++) {
        ch = tolower(rawText[c + 1]);
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            typedText[0]++;
            typedText[typedText[0]] = ch;
        }
    }

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded == 0)
            continue;
        if (systemData[i].exploreLevel <= 0)
            continue;
        if (systemData[i].known == 0)
            continue;

        candText[0] = 0;
        c = 0;
        while (c < (char)systemData[i].name[0]) {
            ch = tolower((char)systemData[i].name[c + 1]);
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
        mapScroll.h = systemData[bestSystem].pos.h;
        mapScroll.v = systemData[bestSystem].pos.v;
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    } else {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    }
}

#ifndef MACBUILD
/* CrtInitHook7. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aaf50
void CrtInitHook7(void)
{
}
#endif
