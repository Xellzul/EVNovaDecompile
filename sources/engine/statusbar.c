// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* statusbar.c -- the status-bar driver pair (win link order 0x0045d0a0, 0x0045d320; mac 1.1.1 twins _RedrawStatus @0x4cac0, _DrawStatus @0x4c85d). Notes: Research/notes/statusbar.md */
#include "macport.h"
#include "shipobj.h"
#include "game.h"
#include "dialogs.h"
#include "ships.h"
#include "player.h"
#include "render.h"
#include "burger.h"

unsigned short lastTargetedShipClass;
short lastTargetedShipShields;
short radarCount;
unsigned char lastDebugTargInfoState;
unsigned char targKeyOnNow;
unsigned char DAT_007cab70[4];
unsigned long lastRadarUpdateTime;
unsigned char DAT_007cab78[4];
char preventStatusCopy;
RGBColor *statusBrightColor;
RGBColor *statusDimColor;
RGBColor *shieldStatusColor;
RGBColor *armorStatusColor;
RGBColor *fuelStatusColor[2];
RGBColor *radarStatusColor[2];
short interferenceThisFrame;


/* RedrawStatus (mac _RedrawStatus). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045d0a0
void RedrawStatus(char copyToScreen)
{
    Rect box;
    Rect box2;
    Rect *pr;

    if (creatingPilotFile || gPause)
        return;
    SetRect(&box, GetCWindowPortRect(gWind)->right - 0xc2,
            GetCWindowPortRect(gWind)->top, GetCWindowPortRect(gWind)->right,
            GetCWindowPortRect(gWind)->bottom);
    CopyBits(GetGWCPixMap(&statusOffPort), (BitMapPtr)GetGWCPixMap(&loadPort),
             &statusOffPort.bounds, &box, 0, 0);
    if (HasIFF(shipData)) {
        SetGWorldContainer(&loadPort);
        box2.top = statusOffRect[0].top;
        box2.left = statusOffRect[0].left;
        box2.bottom = statusOffRect[0].bottom;
        box2.right = statusOffRect[0].right;
        OffsetRect(&box, GetCWindowPortRect(gWind)->right - 0xc2, 0);
        RGBForeColor(blackColor);
        PaintRect(&box2);
        SWSetPortToWindow(gSpriteWorld);
    }
    statusRedraw[0] = 1;
    statusRedraw[1] = 1;
    statusRedraw[2] = 1;
    statusRedraw[3] = 1;
    statusRedraw[4] = 1;
    statusRedraw[5] = 1;
    statusRedraw[6] = 1;
    lastTargetedShipClass = 0x8001;
    lastTargetedShipShields = 0x8001;
    lastDebugTargInfoState = 0;
    preventStatusCopy = 1;
    DrawStatus();
    preventStatusCopy = 0;
    if (copyToScreen) {
        pr = GetCWindowPortRect(gWind);
        box.top = pr->top;
        box.left = pr->left;
        box.bottom = pr->bottom;
        box.right = pr->right;
        box.left = box.right - 0xc2;
        if (gSpriteWorld->fastBlit) {
            CopyBits(GetGWCPixMap(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     &box, &box, 0, 0);
        } else {
            CopyBits(GetGWCPixMap(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     &box, &box, 0, GetCWindowVisRgn(gWind));
        }
    }
}

/* DrawStatus (mac _DrawStatus). STATE: MATCH 100.0%, exe 150 / obj 150, relocregcheck
   0 of 59. The radar call's landed arm is written second (`!onPlanet && !playerIsLanded');
   that arm order, not a spelling at the site, decided the lastRadarUpdateTime+15 register.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0045d320
void DrawStatus(void)
{
    if (fadePending[0] || fadePending[1] || creatingPilotFile)
        return;
    if (shipData[0].navTargetKind != -1 && ReadTick() % 30 == 0)
        statusRedraw[0] = 1;
    if (shipData[0].targetShip != -1) {
        if (ReadTick() % 45 == 0)
            statusRedraw[6] = statusRedraw[2] = 1;
    }
    if (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl))
        targKeyOnNow = 1;
    else
        targKeyOnNow = 0;
    if (ReadTick() >= lastRadarUpdateTime + 15) {
        lastRadarUpdateTime = ReadTick();
        radarCount = (radarCount + 1) % 2;
        if (shipData[0].targetShip != -1) {
            if (lastTargetedShipShields != shipData[shipData[0].targetShip].shield ||
                lastTargetedShipClass != (unsigned short)shipData[shipData[0].targetShip].type)
                statusRedraw[6] = 1;
            if (lastDebugTargInfoState != targKeyOnNow || targKeyOnNow)
                statusRedraw[6] = 1;
        } else {
            lastTargetedShipClass = lastTargetedShipShields = 0x8001;
        }
        statusRedraw[5] = 1;
    }
    if (statusRedraw[6]) {
        DrawStatusTarg();
        statusRedraw[6] = 0;
    }
    if (statusRedraw[5]) {
        if (!onPlanet && !playerIsLanded)
            DrawStatusRadar(0);
        else
            DrawStatusRadar(1);
        statusRedraw[5] = 0;
    }
    if (statusRedraw[3]) {
        DrawStatusWeap();
        statusRedraw[3] = 0;
    }
    if (statusRedraw[0]) {
        DrawStatusNav();
        statusRedraw[0] = 0;
    }
    if (statusRedraw[1]) {
        DrawStatusShield();
        statusRedraw[1] = 0;
    }
    if (statusRedraw[4]) {
        DrawStatusFuel();
        statusRedraw[4] = 0;
    }
    if (statusRedraw[2]) {
        DrawStatusCargo();
        statusRedraw[2] = 0;
    }
}
