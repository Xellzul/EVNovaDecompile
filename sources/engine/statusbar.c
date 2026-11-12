// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
#include "cefix.h"

ShipClassID lastTargetedShipClass;
short lastTargetedShipShields;
short radarCount;
unsigned char lastDebugTargInfoState;
unsigned char targKeyOnNow;
#ifndef MACBUILD
unsigned char DAT_007cab70[4];
#endif
unsigned long lastRadarUpdateTime;
#ifndef MACBUILD
unsigned char DAT_007cab78[4];
#endif
Boolean preventStatusCopy;
RGBColor *statusBrightColor;
RGBColor *statusDimColor;
RGBColor *shieldStatusColor;
RGBColor *armorStatusColor;
RGBColor *fuelStatusColor[2];
RGBColor *radarStatusColor[2];
short interferenceThisFrame;


/* RedrawStatus (mac _RedrawStatus). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045d0a0
// MAC: _RedrawStatus i386 0x0004cac0 ppc 0x00055a18
void RedrawStatus(Boolean copyToScreen)
{
    Rect box;
    Rect box2;

    if (creatingPilotFile || gPause)
        return;
#ifdef CEFIX /* F14.03 */
    SetRect(&box, GetCWindowPortRect(gWind)->right - ceStatusBarWidth,
            GetCWindowPortRect(gWind)->top, GetCWindowPortRect(gWind)->right,
            GetCWindowPortRect(gWind)->bottom);
#else
    SetRect(&box, GetCWindowPortRect((WindowRef)gWind)->right - kTargPanelInset,
            GetCWindowPortRect((WindowRef)gWind)->top, GetCWindowPortRect((WindowRef)gWind)->right,
            GetCWindowPortRect((WindowRef)gWind)->bottom);
#endif
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &statusOffPort.bounds, &box, 0, 0);
    if (HasIFF(shipData)) {
        SetGWorldContainer(&loadPort);
        box2 = statusOffRect[0];
#ifdef CEFIX /* F14.04 */
        OffsetRect(&box, GetCWindowPortRect(gWind)->right - ceStatusBarWidth, 0);
#else
        OffsetRect(&box, GetCWindowPortRect((WindowRef)gWind)->right - kTargPanelInset, 0);
#endif
        ForeColor(blackColor);
        PaintRect(&box2);
        SWSetPortToWindow(gSpriteWorld);
    }
    statusRedraw[kRedrawNav] = 1;
    statusRedraw[kRedrawShield] = 1;
    statusRedraw[kRedrawCargo] = 1;
    statusRedraw[kRedrawWeap] = 1;
    statusRedraw[kRedrawFuel] = 1;
    statusRedraw[kRedrawRadar] = 1;
    statusRedraw[kRedrawTarg] = 1;
    lastTargetedShipClass = 0x8001;
    lastTargetedShipShields = 0x8001;
    lastDebugTargInfoState = 0;
    preventStatusCopy = 1;
    DrawStatus();
    preventStatusCopy = 0;
    if (copyToScreen) {
        box = *GetCWindowPortRect((WindowRef)gWind);
#ifdef CEFIX /* F14.05 */
        box.left = box.right - ceStatusBarWidth;
#else
        box.left = box.right - kTargPanelInset;
#endif
        if (gSpriteWorld->fastBlit) {
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &box, &box, 0, 0);
        } else {
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &box, &box, 0, GetCWindowVisRgn((WindowRef)gWind));
        }
    }
#if V111
    if (!useLegacyRenderingPipeline)
        QDFlushPortBuffer(GetWindowPort(gWind), NULL);
#endif
}

/* DrawStatus (mac _DrawStatus). STATE: MATCH 100.0%, exe 150 / obj 150, relocregcheck
   0 of 59. The radar call's landed arm is written second (`!onPlanet && !playerIsLanded');
   that arm order, not a spelling at the site, decided the lastRadarUpdateTime+15 register.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0045d320
// MAC: _DrawStatus i386 0x0004c85d ppc 0x00055688
void DrawStatus(void)
{
#ifdef MACBUILD
    Boolean drew;

#endif
    if (fadePending[0] || fadePending[1] || creatingPilotFile)
        return;
    if (shipData[0].navTargetKind != -1 && ReadTick() % 30 == 0)
        statusRedraw[kRedrawNav] = 1;
    if (shipData[0].targetShip != -1) {
        if (ReadTick() % 45 == 0)
            statusRedraw[kRedrawTarg] = statusRedraw[kRedrawCargo] = 1;
    }
#ifdef MACBUILD
    if (QuickKeyCheck(kScanControl))
#else
    if (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl))
#endif
        targKeyOnNow = 1;
    else
        targKeyOnNow = 0;
#ifdef MACBUILD
    drew = 0;
#endif
    if (ReadTick() >= lastRadarUpdateTime + 15) {
        lastRadarUpdateTime = ReadTick();
        radarCount = (radarCount + 1) % 2;
        if (shipData[0].targetShip != -1) {
            if (shipData[shipData[0].targetShip].shield != lastTargetedShipShields ||
                (ShipClassID)shipData[shipData[0].targetShip].type != lastTargetedShipClass)
                statusRedraw[kRedrawTarg] = 1;
            if (lastDebugTargInfoState != targKeyOnNow || targKeyOnNow)
                statusRedraw[kRedrawTarg] = 1;
        } else {
            lastTargetedShipClass = lastTargetedShipShields = 0x8001;
        }
        statusRedraw[kRedrawRadar] = 1;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawTarg]) {
        DrawStatusTarg();
        statusRedraw[kRedrawTarg] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawRadar]) {
        if (!onPlanet && !playerIsLanded)
            DrawStatusRadar(0);
        else
            DrawStatusRadar(1);
        statusRedraw[kRedrawRadar] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawWeap]) {
        DrawStatusWeap();
        statusRedraw[kRedrawWeap] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawNav]) {
        DrawStatusNav();
        statusRedraw[kRedrawNav] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawShield]) {
        DrawStatusShield();
        statusRedraw[kRedrawShield] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawFuel]) {
        DrawStatusFuel();
        statusRedraw[kRedrawFuel] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
    if (statusRedraw[kRedrawCargo]) {
        DrawStatusCargo();
        statusRedraw[kRedrawCargo] = 0;
#ifdef MACBUILD
        drew = 1;
#endif
    }
#ifdef MACBUILD
    if (drew)
        QDFlushPortBuffer(GetCWindowGrafPort(gWind), NULL);
#endif
}
