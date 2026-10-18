// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
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

#define kMissionKeepsUnk61 0x0010 /* flags55 */

/* the mission object's globals; this file ends that object (the .CRT hook) */
unsigned char missionName[1000][256];
char forceRandom;
char canRefuseMission;
char offeringFromShip;
char showAbortMissionButton;
char reenterMissionDialog;
char suppressMissionOffer[1000];
PicHandle missionBackgroundPic[3];
PicHandle gMissionInfoPict;
short activeMissions[16];
short missionListVenue;
short selMissionListItem;
short gMissionInfoIndex;
short offeringShipIndex;
short tempMissions[kMaxMissions];
short nextPortMission;
short lastPortMissionLoc;
DialogPtr missionDialog;
DialogPtr missionInfoDialog;
ScrollRec **missionList;
ScrollRec **missionInfoList;
char briefingTextPtr[0x2000];
unsigned long missionOfferTime;
void *stringHandle;
TEHandle gMissionTE;
short currentMissionIndexForSetParser;
BriefMissionRec briefMissionInfo[1000];
unsigned char DAT_0077e804[1972];
Handle missionFileHand;
unsigned char DAT_0077efbc[1648];
MissionAvailRec missionAvailData[kMaxMissions];

/*
 * Per-arrival mission housekeeping (win 0x00448910). A mission wanting the
 * system the player just reached -- or the wildcard -6 -- has its shipDelay countdown
 * restarted, short for the one kind that asks for it and random otherwise, and
 * every active mission gets a fresh auxShipDelay.
 */

/* MissionHandlePlayerEnteredNewSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448910
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
                        if (currentMissions[i].shipBehav == 1 && currentMissions[i].shipGoal == 3)
                            currentMissions[i].shipDelay = 30;
                        else
                            currentMissions[i].shipDelay = Rand(100) + 100;
                        currentMissions[i].specialShipsJumpedIn = 0;
                    }
                }
            }
            if ((currentMissions[i].flags & kMissionKeepsUnk61) != 0)
                currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
            currentMissions[i].auxShipDelay = Rand(70) + 70;
            currentMissions[i].auxShipsSpawned = 0;
        }
    }
}

/* MissionListDefinition (mac _MissionListDefinition). STATE: MATCH, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x00448a30
void MissionListDefinition(short lMessage, char lSelect, Rect *lRect, long lCell,
                   short lDataOffset, short lDataLen, ScrollRec **lHandle)
{
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
        RGBForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        v = lRect->top + (*lHandle)->indent.v;
        MoveTo(lRect->left + (*lHandle)->indent.h + 4, v + smallTextSize);
        TextFace(0);
        RGBForeColor(&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        RGBForeColor(blackColor);
        break;
    case 2:
        if (lSelect)
            RGBForeColor(&ldefColors[2]);
        else
            RGBForeColor(&ldefColors[1]);
        PaintRect(lRect);
        RGBForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        v = lRect->top + (*lHandle)->indent.v;
        MoveTo(lRect->left + (*lHandle)->indent.h + 4, v + smallTextSize);
        TextFace(0);
        RGBForeColor(&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        RGBForeColor(blackColor);
        break;
    }
}

/* CrtInitHook2. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00448b90
void CrtInitHook2(void)
{
}

/* The exe ends this object with a C++ static initializer, reached through .CRT$XCU; its body
   is the function above.  An empty constructor makes the compiler emit the hook. */
struct CrtHook { CrtHook() {} };
static CrtHook crtHook;
