// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* listdefdraw.c -- the mission-list TU: per-arrival mission housekeeping and the mission list's LDEF (win 0x00448910..0x00448b85), then CrtInitHook2, the .CRT hook that ends the object. Notes: Research/notes/listdefdraw.md */
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "render.h"
#include "gameutil.h"
#include "prototypes.h"
#include "stellar.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "missions.h"
#include "cefix.h"

/* the mission object's globals; this file ends that object (the .CRT hook) */
unsigned char missionName[1000][256];
Boolean forceRandom;
Boolean canRefuseMission;
Boolean offeringFromShip;
Boolean showAbortMissionButton;
Boolean reenterMissionDialog;
Boolean suppressMissionOffer[1000];
PicHandle missionBackgroundPic[3];
PicHandle missionInfoBackgroundPic;
short activeMissions[16];
short missionsOutside;
short selMissionListItem;
short selMissionInfoListItem;
short offeringShipIndex;
short tempMissions[kMaxMissions];
short nextPortMission;
short lastPortMissionLoc;
#ifndef MACBUILD
DialogPtr missionDialog;
#endif
DialogPtr missionInfoDialog;
ScrollRec **missionList;
ScrollRec **missionInfoList;
char briefingTextPtr[0x2000];
unsigned long missionOfferTime;
void *stringHandle;
TEHandle offerTextHandle;
short currentMissionIndexForSetParser;
#ifndef MACBUILD
BriefMissionRec briefMissionInfo[1000];
unsigned char DAT_0077e804[1972];
Handle missionFileHand;
unsigned char DAT_0077efbc[1648];
MissionAvailRec missionAvailData[kMaxMissions];
#else
/* tentative here too; ResourceTypes.o's equal-size definitions come later in link order and win */
BriefMissionRec briefMissionInfo[1000];
Handle missionFileHand;
MissionAvailRec missionAvailData[kMaxMissions];
#endif

/*
 * Per-arrival mission housekeeping (win 0x00448910). A mission wanting the
 * system the player just reached -- or the wildcard -6 -- has its shipDelay countdown
 * restarted, short for the one kind that asks for it and random otherwise, and
 * every active mission gets a fresh auxShipDelay.
 */

/* MissionHandlePlayerEnteredNewSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448910
// MAC: _MissionHandlePlayerEnteredNewSystem i386 0x00099f11 ppc 0x000a5d14
void MissionHandlePlayerEnteredNewSystem(void)
{
    short i;
    short sys;

    for (i = 0; i < 16; i++) {
        if (currentMissionObjectives[i].active != 0) {
            if (currentMissions[i].shipSyst >= 0 && currentMissions[i].shipSyst < 2048)
                sys = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            else
                sys = -1;
            if (currentMissions[i].shipCount > 0) {
                if (sys == shipData[0].curSystem ||
                    currentMissions[i].shipSyst == -6) {
                    if (currentMissions[i].shipStart == 1) {
                        if (currentMissions[i].shipBehav == kMissionModeEscort && currentMissions[i].shipGoal == kMissionGoalEscort)
                            currentMissions[i].shipDelay = 30;
                        else
                            currentMissions[i].shipDelay = Rand(100) + 100;
                        currentMissions[i].specialShipsJumpedIn = 0;
                    }
                }
            }
            if ((currentMissions[i].flags & kMissionInfiniteAuxShips) != 0)
                currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
            currentMissions[i].auxShipDelay = Rand(70) + 70;
            currentMissions[i].auxShipsSpawned = 0;
        }
    }
}

/* MissionListDefinition (mac _MissionListDefinition). STATE: MATCH, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x00448a30
// MAC: _MissionListDefinition i386 0x0009a018 ppc 0x000a5e68
void MissionListDefinition(short lMessage, Boolean lSelect, Rect *lRect, long lCell,
                   short lDataOffset, short lDataLen, ScrollRec **lHandle)
{
#ifdef MACBUILD
    SInt32 penMode;
    short h;
    void *text;
    short v;
    FontInfo fontInfo;
    GrafPtr port;

    text = *(*lHandle)->cells;
    switch (lMessage) {
    case lDrawMsg:
        GetFontInfo(&fontInfo);
        if (lSelect)
            RGBForeColor((RGBColor *)&ldefColors[2]);
        else
            RGBForeColor((RGBColor *)&ldefColors[1]);
        PaintRect(lRect);
        ForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        h = lRect->left + (*lHandle)->indent.h + 4;
        v = lRect->top + (*lHandle)->indent.v;
        GetFontInfo(&fontInfo);
        MoveTo(h, v + fontInfo.ascent);
        TextFace(0);
        if (TextWidth(text, lDataOffset, lDataLen) > lRect->right - h)
            TextFace(condense);
        RGBForeColor((RGBColor *)&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        ForeColor(blackColor);
        break;
    case lHiliteMsg:
        GetPort(&port);
        penMode = GetPortPenMode((CGrafPtr)port);
        if (lSelect)
            RGBForeColor((RGBColor *)&ldefColors[2]);
        else
            RGBForeColor((RGBColor *)&ldefColors[1]);
        PaintRect(lRect);
        ForeColor(blackColor);
        if (lDataLen > 0) {
            h = lRect->left + (*lHandle)->indent.h + 4;
            v = lRect->top + (*lHandle)->indent.v;
            GetFontInfo(&fontInfo);
            MoveTo(h, v + fontInfo.ascent);
            TextFace(0);
            if (TextWidth(text, lDataOffset, lDataLen) > lRect->right - h)
                TextFace(condense);
            RGBForeColor((RGBColor *)&ldefColors[0]);
            DrawText(text, lDataOffset, lDataLen);
            ForeColor(blackColor);
        }
        SetPortPenMode((CGrafPtr)port, penMode);
        break;
    }
#else
    void *text;
    short v;

    text = *(*lHandle)->cells;
    switch (lMessage) {
    case 1:
        if (lSelect)
            RGBForeColor(&ldefColors[2]);
        else
            RGBForeColor(&ldefColors[1]);
        PaintRect(lRect);
        ForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        v = lRect->top + (*lHandle)->indent.v;
#ifdef CEFIX /* F12.08 */
        CE_SetPen(lRect->left + (*lHandle)->indent.h + CE_Scale(4),
                  v + smallTextSize - 9 + CE_Scale(9));
#else
        MoveTo(lRect->left + (*lHandle)->indent.h + 4, v + smallTextSize);
#endif
        TextFace(0);
        RGBForeColor(&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        ForeColor(blackColor);
        break;
    case 2:
        if (lSelect)
            RGBForeColor(&ldefColors[2]);
        else
            RGBForeColor(&ldefColors[1]);
        PaintRect(lRect);
        ForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        v = lRect->top + (*lHandle)->indent.v;
#ifdef CEFIX /* F12.09 */
        CE_SetPen(lRect->left + (*lHandle)->indent.h + CE_Scale(4),
                  v + smallTextSize - 9 + CE_Scale(9));
#else
        MoveTo(lRect->left + (*lHandle)->indent.h + 4, v + smallTextSize);
#endif
        TextFace(0);
        RGBForeColor(&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        ForeColor(blackColor);
        break;
    }
#endif
}

#ifndef MACBUILD
/* CrtInitHook2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448b90
void CrtInitHook2(void)
{
}
#endif

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
#endif
