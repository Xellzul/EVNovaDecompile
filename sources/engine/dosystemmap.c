// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* dosystemmap.c -- tu-map object O25: the galaxy map dialog, its drawing, filters and jump routes. Notes: Research/notes/dosystemmap.md */
#include <stddef.h>
#include <string.h>

#include "gametypes.h"
#include "macport.h"
#include "burger.h"
#include "resources.h"
#include "shipobj.h"
#include "gameconst.h"
#include "prototypes.h"
#include "ships.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "missions.h"
#include "render.h"
#include "sound.h"
#include "stellar.h"
#include "space.h"

/* mapZoom and floatingMapZoom: one .data run, in address order. */
double mapZoom = 0.5625;                         /* 0x005759d8 (mac name) */
double floatingMapZoom = 0.5625;                 /* 0x005759e0 (see mapZoom above) */


#define kMapDlogID     2000
#define kMapPictID     0x213d
#define kNebulaPictBase 0x251c
#define kShiftScanCode  0x2a
#define kRShiftScanCode 0x36

/* DoSystemMap (mac _DoSystemMap). STATE: MATCH 100.0%, exe 1413 / obj 1413. OPEN: `long last'
   is a colour handle (inline `routeLen - 1' 97.9); see notes. */
// FUNCTION: EVNOVA 0x004a3aa0
void DoSystemMap(void)
{
    char bad[4];
    unsigned long long key;
    short i;
    short j;
    short sys;
    short found;
    short routeLen;
    long last;
    short prev;
    short s2;
    short s3;
    short prevV;
    short prevH;
    short sy;
    short savedStellar;
    unsigned char savedShow;
    short x;
    short y;
    short w;
    short h;
    short savedX;
    short savedY;
    short itemType;
    void *item;
    char done;
    char inRoute;
    char selected;
    char hyperlinked;
    Rect box;
    Rect box2;
    Point mouse;
    short itemHit;
    Rect r;
    Point anchor;
    Rect *pr;

    done = 0;
    if (screenFaded != 0)
        return;
    ShowTheCursor();
    mapDialog = NULL;
    mapDialog = GetNewDialog(kMapDlogID, MapDialogUpdate, MapFilter);
    if (mapDialog == NULL)
        return;
    key = RT3_GetLicenseCode();
    bad[0] = ((key ^ 0xcb0b794232805675ULL) & 0x0fffffffffffffffULL)
                 != 0x0b947b5621514203ULL;
    bad[1] = ((key ^ 0xaa30db1ee9c7913aULL) & 0x0fffffffffffffffULL)
                 != 0x01c38b5f995da87aULL;
    bad[2] = ((key ^ 0xe5a3c8161b574a9fULL) & 0x0fffffffffffffffULL)
                 != 0x065298576bcd73d9ULL;
    bad[3] = ((key ^ 0x77c04b34263c359eULL) & 0x0fffffffffffffffULL)
                 != 0x0ee20eb101b08ac5ULL;
    for (x = 0; x < 4; x++) {
        if (bad[x] == 0) {
            for (j = 0; j < kMaxSystemLinks; j++)
                gSystems[shipData->curSystem].links[j] = -1;
            break;
        }
    }
    savedShow = gShowInfluence;
    influenceCoordsCached = 0;
    savedStellar = shipData->targetStellar;
    if (mapZoom <= 0.0)
        mapZoom = 1.0;
    AdvanceJumpRoute();
    showClearRouteButton = 0;
    if (hypergateMapMode == 0) {
        for (i = 1; i < kMaxHyperRoute; i++) {
            if (hyperRoute[i] != -1)
                showClearRouteButton = 1;
        }
    }
    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].loaded != 0 && nebulaData[i].explored != 0) {
            for (j = 0; j < 7; j++)
                mapNebulaPicts[i][j] = GetPicture(kNebulaPictBase + i * 7 + j);
        }
    }
    mapCursorSwitchRgn = 0;
    partialMapDialogUpdate = 0;
    for (y = 0; y < 256; y++) {
        for (sys = 0; sys < 256; sys++) {
            influenceOwner[sys][y][0] = 0;
            influenceValue[sys][y][0] = 0;
            influenceOwner[sys][y][1] = 0;
            influenceValue[sys][y][1] = 0;
            influenceOwner[sys][y][2] = 0;
            influenceValue[sys][y][2] = 0;
            influenceOwner[sys][y][3] = 0;
            influenceValue[sys][y][3] = 0;
        }
    }
    RecalcMissionSystsForMap();
    if (shipData->navTargetKind == 3 && shipData->targetStellar != -1 && onPlanet == 0)
        selSystem = FindActiveCoLocatedSystem(
            gSystems[shipData->curSystem].links[shipData->targetStellar]);
    else
        selSystem = shipData->curSystem;
    FUN_004a74e0();
    mapDialogPict = GetPicture(kMapPictID);
    CenterWindow(FrontWindow(mapDialog), gWind);
    ShowWindowNoFlash(FrontWindow(mapDialog));
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(mapDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(mapDialog);
    MapDialogUpdate();
    FrontWindow(mapDialog);
    FrontWindow(mapDialog);
    FUN_004a51e0();
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    if (gShowInfluence != 0)
        RecalcAllMapInfluence(&box);
    do {
        if (gQuit)
            done = 1;
        RunModalDialog(mapDialog, MapFilter, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 7) {
            cursorVisible = 0;
            ShowTheCursor();
        }
        if (itemHit == 9) {
            gShowInfluence = (gShowInfluence == 0);
            InvalDialogItem(mapDialog, 9);
            InvalDialogItem(mapDialog, kMapPictItem);
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box2);
        }
        if (itemHit == 4 && canZoomOut != 0) {
            mapZoom = mapZoom * 1.333333;
            InvalDialogItem(mapDialog, 2);
            InvalDialogItem(mapDialog, 3);
            InvalDialogItem(mapDialog, 4);
            InvalDialogItem(mapDialog, 5);
            InvalDialogItem(mapDialog, 6);
            FUN_004a74e0();
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        }
        if (itemHit == 5 && canZoomIn != 0) {
            mapZoom = mapZoom * 0.75;
            InvalDialogItem(mapDialog, 2);
            InvalDialogItem(mapDialog, 3);
            InvalDialogItem(mapDialog, 4);
            InvalDialogItem(mapDialog, 5);
            InvalDialogItem(mapDialog, 6);
            FUN_004a74e0();
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        }
        if (itemHit == 8) {
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            hyperRoute[0] = shipData->curSystem;
            routeLen = 1;
            showClearRouteButton = 0;
            InvalDialogItem(mapDialog, 3);
            InvalDialogItem(mapDialog, 6);
            InvalDialogItem(mapDialog, 8);
            MapDialogUpdate();
            shipData->targetStellar = -1;
            statusRedraw[0] = 1;
            DrawStatus();
        }
        if (itemHit == 10)
            DoSystemSearch();
        if (itemHit == 3) {
            GetMouse(&mouse);
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box2);
            w = box2.right - box2.left;
            h = box2.bottom - box2.top;
            found = -1;
            for (sys = 0; sys < kMaxSystems; sys++) {
                if (found != -1)
                    break;
                found = -1;
                if (gSystems[sys].loaded != 0) {
                    sy = gSystems[sys].yPos;
                    r.left = (box2.left + w / 2
                                     + gSystems[sys].xPos / mapZoom
                                     - mapScroll.x / mapZoom);
                    r.top = (box2.top + h / 2
                                    + sy / mapZoom
                                    - mapScroll.y / mapZoom);
                    r.right = r.left;
                    r.bottom = r.top;
                    InsetRect(&r, -10, -10);
                    if (PtInRect(mouse.h, mouse.v, &r)) {
                        inRoute = selected = hyperlinked = 0;
                        for (s3 = 0; s3 < 32; s3++) {
                            if (sys == missionSyst[s3] && missionSyst[s3] != -1) {
                                inRoute = 1;
                                break;
                            }
                        }
                        if (greenMissionSyst == sys)
                            selected = 1;
                        if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                            && hypergateMapModeStellar < kMaxStellars) {
                            for (i = 0; i < kSpobHyperLinks; i++) {
                                s2 = stellarData[hypergateMapModeStellar].hyperLinks[i];
                                if (s2 != -1) {
                                    s3 = stellarData[s2].system;
                                    if (s3 < 0 || s3 >= kMaxSystems)
                                        s3 = FindSystemFromStellar(s2);
                                    s3 = FindActiveCoLocatedSystem(s3);
                                    if (s3 == sys) {
                                        hyperlinked = 1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (hypergateMapMode == 0 || hyperlinked != 0
                            || sys == shipData->curSystem) {
                            if (gSystems[sys].known != 0 || inRoute != 0
                                || selected != 0 || hyperlinked != 0) {
                                found = sys;
                                break;
                            }
                            if ((KeyCheck(kShiftScanCode) == 0
                                 && KeyCheck(kRShiftScanCode) == 0)
                                || routeLen <= 1) {
                                prev = shipData->curSystem;
                            } else {
                                prev = hyperRoute[routeLen - 1];
                                if (gSystems[prev].xPos == gSystems[sys].xPos
                                    && gSystems[prev].yPos == gSystems[sys].yPos) {
                                    found = sys;
                                    break;
                                }
                            }
                            for (i = 0; i < kMaxSystemLinks; i++) {
                                s2 = gSystems[prev].links[i];
                                if (s2 != -1) {
                                    s2 = FindActiveCoLocatedSystem(s2);
                                    if (s2 == sys) {
                                        found = s2;
                                        break;
                                    }
                                    if (gSystems[s2].xPos == gSystems[sys].xPos
                                        && gSystems[s2].yPos == gSystems[sys].yPos) {
                                        found = s2;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (found != -1)
                found = FindActiveCoLocatedSystem(found);
            if (found != -1) {
                InvalDialogItem(mapDialog, 2);
                InvalDialogItem(mapDialog, 3);
                InvalDialogItem(mapDialog, 6);
                if ((KeyCheck(kShiftScanCode) != 0
                     || KeyCheck(kRShiftScanCode) != 0)
                    && hypergateMapMode == 0) {
                    AdvanceJumpRoute();
                    routeLen = -1;
                    if (FindFirstCoLocatedSystem(shipData->curSystem)
                        == FindFirstCoLocatedSystem(found)) {
                        for (i = 0; i < kMaxHyperRoute; i++)
                            hyperRoute[i] = -1;
                    }
                    for (i = 1; i < kMaxHyperRoute; i++) {
                        if (hyperRoute[i] == -1)
                            break;
                        if (FindFirstCoLocatedSystem(hyperRoute[i])
                            == FindFirstCoLocatedSystem(found)) {
                            for (j = i; j < kMaxHyperRoute; j++)
                                hyperRoute[j] = -1;
                            break;
                        }
                    }
                    for (i = 1; i < kMaxHyperRoute; i++) {
                        if (hyperRoute[i] == -1) {
                            routeLen = i;
                            break;
                        }
                    }
                    if (routeLen == -1) {
                        if (hyperRoute[0] == -1)
                            hyperRoute[0] = shipData->curSystem;
                        routeLen = 1;
                    }
                    inRoute = 0;
                    if (routeLen > 1) {
                        prev = hyperRoute[routeLen - 1];
                        s2 = FindActiveCoLocatedSystem(prev);
                        if (s2 != prev && s2 != -1)
                            prev = s2;
                    } else
                        prev = shipData->curSystem;
                    if (gSystems[prev].exploreLevel > 0
                        || gSystems[found].known != 0) {
                        for (i = 0; i < kMaxSystemLinks; i++) {
                            s2 = gSystems[prev].links[i];
                            if (s2 != -1) {
                                s2 = FindActiveCoLocatedSystem(s2);
                                if (s2 == found) {
                                    inRoute = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (routeLen > 0) {
                        last = routeLen - 1;
                        s2 = hyperRoute[last];
                        if (s2 >= 0 && s2 < kMaxSystems) {
                            if (gSystems[found].xPos == gSystems[s2].xPos
                                && gSystems[found].yPos == gSystems[s2].yPos) {
                                hyperRoute[last] = -1;
                                routeLen--;
                                inRoute = 0;
                            }
                        }
                    }
                    if (inRoute != 0) {
                        if (routeLen != -1) {
                            if (hyperRoute[0] == -1)
                                hyperRoute[0] = shipData->curSystem;
                            hyperRoute[routeLen] = found;
                            routeLen++;
                            showClearRouteButton = 1;
                        } else {
                            hyperRoute[0] = shipData->curSystem;
                            hyperRoute[1] = found;
                            routeLen = 2;
                            showClearRouteButton = 1;
                        }
                        InvalDialogItem(mapDialog, 3);
                        InvalDialogItem(mapDialog, 6);
                        InvalDialogItem(mapDialog, 8);
                        SetGWorldContainer(&loadPort);
                        RGBForeColor(blackColor);
                        PaintRect(GetDialogPortRect(mapDialog));
                        if (mapDialogPict != NULL)
                            DrawPicHandle(mapDialogPict, GetDialogPortRect(mapDialog));
                        UnsetGWorldContainer(&loadPort);
                        SetDialogGWorld(mapDialog);
                        DrawMapButtons(-1);
                        selSystem = found;
                    }
                } else {
                    selSystem = found;
                    if (onPlanet == 0) {
                        if (hypergateMapMode != 0) {
                            if (hyperlinked != 0) {
                                shipData->navTargetKind = 4;
                                shipData->targetStellar = selSystem;
                            } else {
                                shipData->navTargetKind = -1;
                                shipData->targetStellar = -1;
                            }
                        } else {
                            s2 = -1;
                            for (i = 0; i < kMaxSystemLinks; i++) {
                                s3 = gSystems[shipData->curSystem].links[i];
                                if (s3 != -1) {
                                    if (found == FindActiveCoLocatedSystem(s3)) {
                                        s2 = i;
                                        break;
                                    }
                                }
                            }
                            if (s2 != -1) {
                                shipData->navTargetKind = 3;
                                shipData->targetStellar = s2;
                                statusRedraw[0] = 1;
                                DrawStatus();
                                SetDialogGWorld(mapDialog);
                            } else {
                                shipData->targetStellar = -1;
                                statusRedraw[0] = 1;
                                DrawStatus();
                                SetDialogGWorld(mapDialog);
                            }
                        }
                    }
                }
            } else {
                if (StillDown()) {
                    GetMouse(&anchor);
                    savedX = mapScroll.x;
                    savedY = mapScroll.y;
                    while (StillDown()) {
                        BusyIdle();
                        prevV = mouse.v;
                        prevH = mouse.h;
                        GetMouse(&mouse);
                        mapScroll.x = (savedX
                                              - (mouse.h - anchor.h) * mapZoom);
                        mapScroll.y = (savedY
                                              - (mouse.v - anchor.v) * mapZoom);
                        ScrollMapDialog(mouse.h - prevH, mouse.v - prevV);
                        nv_UpdateDialogWindow(0);
                    }
                    if (gShowInfluence != 0)
                        RecalcAllMapInfluence(&box);
                    if (mapScroll.x != savedX
                        || mapScroll.y != savedY)
                        MapDialogUpdate();
                }
            }
        }
    } while (done == 0);
    for (x = 0; x < kMaxNebulae; x++) {
        for (j = 0; j < 7; j++) {
            if (mapNebulaPicts[x][j] != NULL) {
                QDReleaseResource(mapNebulaPicts[x][j]);
                mapNebulaPicts[x][j] = 0;
            }
        }
    }
    if (mapDialogPict != NULL) {
        QDReleaseResource(mapDialogPict);
        mapDialogPict = NULL;
    }
    x = (GetDialogPortRect(mapDialog)->right - GetDialogPortRect(mapDialog)->left) / 2;
    y = (GetDialogPortRect(mapDialog)->bottom - GetDialogPortRect(mapDialog)->top) / 2;
    while (KeyCheck(keyPrefs.map))
        BusyIdle();
    DisposeDialogNoFlash(mapDialog);
    mapDialog = NULL;
    SWSetPortToWorkArea(gSpriteWorld);
    RGBForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    RGBForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    w = (GetCWindowPortRect(gWind)->right - GetCWindowPortRect(gWind)->left) / 2;
    i = (GetCWindowPortRect(gWind)->bottom - GetCWindowPortRect(gWind)->top) / 2;
    RGBForeColor(blackColor);
    SetRect(&r, w - x, i - y, w + x, i + y);
    pr = GetCWindowPortRect(gWind);
    box2.top = pr->top;
    box2.left = pr->left;
    box2.bottom = pr->bottom;
    box2.right = pr->right;
    box2.left = box2.right - kTargPanelInset;
    if (SectRect(&r, &box2, &box2))
        RedrawStatus(1);
    RedrawDialogs(1);
    OSCursorReset();
    cursorVisible = 0;
    ShowTheCursor();
    if (portDialog != NULL && playerIsLanded != 0) {
        shipData->targetStellar = savedStellar;
    } else {
        if (hypergateMapMode == 0)
            SelectHyperLinkToSelection();
    }
    if (portDialog == NULL && playerIsLanded == 0
        && gShowInfluence != savedShow)
        SavePrefs();
    FlushEvents(7, 0);
}

/* FUN_004a51e0. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a51e0
void FUN_004a51e0(void)
{
}


                                                 /* 0x004cd1a0 (mac _StrcatIndCString) */
                                                 /* 0x004b8df0 */
                                                 /* 0x00468210 */

/* MapDialogUpdate. STATE: MATCH 100.0%, 2173/2173. `w / 2' and `h / 2' are written at
   each use (DrawMap's spelling); the service count is a later web of the long `temp'.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004a51f0
void MapDialogUpdate(void)
{
    unsigned char text[256];
    Rect box6;
    void *item;
    short i;
    short savedY;
    long temp;
    short itemType;
    Rect nebR;
    Rect box;
    unsigned char buf[256];
    Rect r2;
    short sel;
    short j;
    short fit;
    short govt;
    short st2;
    short found;
    char ok;
    char linked;
    double sy;
    short w, h;
    unsigned char *name;
    short k;
    short st;
    short n;

    for (i = 0; i < kMaxSystems; i++)
        mapSystemDrawn[i] = 0;
    temp = mapScroll.x;
    savedY = mapScroll.y;
    SetGWorldContainer(&loadPort);
    PenSize(1, 1);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(mapDialog));
    if (partialMapDialogUpdate == 0 && mapDialogPict != NULL)
        DrawPicHandle(mapDialogPict, GetDialogPortRect(mapDialog));
    if (partialMapDialogUpdate == 0) {
        if (mapZoom > 0.5)
            canZoomIn = 1;
        else
            canZoomIn = 0;
        if (mapZoom < 2.0)
            canZoomOut = 1;
        else
            canZoomOut = 0;
        DrawMapButtons(-1);
    }
    if (partialMapDialogUpdate != 0) {
        ok = 1;
    } else {
        GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
        ok = RectInRgn(&box, GetCWindowVisRgn(FrontWindow(mapDialog)));
    }
    if (ok != 0) {
        if (partialMapDialogUpdate != 0) {
            SetGWorldContainer(&loadPort);
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
        } else {
            SWSetPortToWorkArea(gSpriteWorld);
            PenSize(1, 1);
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
        }
        w = box.right - box.left;
        h = box.bottom - box.top;
        RGBForeColor(blackColor);
        PaintRect(&box);
        if (partialMapDialogUpdate == 0 && gShowInfluence != 0) {
            SWSetPortToWorkArea(gSpriteWorld);
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &r2);
            DrawMapInfluence(r2);
        }
        TextFont(smallTextFont);
        TextSize(smallTextSize - 2);
        if (mapZoom >= 1.1)
            TextSize(smallTextSize - 2);
        else
            TextSize(smallTextSize);
        for (i = 0; i < 32; i++) {
            if (nebulaData[i].loaded != 0 && nebulaData[i].explored != 0) {
                nebR.left = (box.left + w / 2
                                    + nebulaData[i].xPos / (float)mapZoom
                                    - temp / mapZoom);
                nebR.top = (box.top + h / 2
                                   + nebulaData[i].yPos / (float)mapZoom
                                   - savedY / mapZoom);
                nebR.right = (nebR.left
                                     + nebulaData[i].width / (float)mapZoom);
                nebR.bottom = (nebR.top
                                      + nebulaData[i].height / (float)mapZoom);
                if (SectRect(&nebR, &box, &box6)) {
                    fit = 0;
                    for (j = 0; j < 7; j++) {
                        if (mapNebulaPicts[i][j] != 0) {
                            QDGetPictureBounds(mapNebulaPicts[i][j], &r2);
                            if (r2.right - r2.left <= nebR.right - nebR.left
                                && r2.bottom - r2.top <= nebR.bottom - nebR.top)
                                fit = j;
                        }
                    }
                    if (mapNebulaPicts[i][fit] != 0) {
                        SectRect(&box, &nebR, &r2);
                        if (partialMapDialogUpdate != 0) {
                            SWSetPortToWorkArea(gSpriteWorld);
                            RGBForeColor(blackColor);
                            DrawPicHandle(mapNebulaPicts[i][fit], &nebR);
                            SetGWorldContainer(&loadPort);
                            RGBForeColor(blackColor);
                            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                                         gSpriteWorld->workFrameP->framePort),
                                     (BitMapPtr)GetGWCPixMap(&loadPort),
                                     &r2, &r2, 0, 0);
                        } else {
                            SWSetPortToBackground(gSpriteWorld);
                            RGBForeColor(blackColor);
                            DrawPicHandle(mapNebulaPicts[i][fit], &nebR);
                            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                                         gSpriteWorld->backFrameP->framePort),
                                     GetPortBitMapForCopyBits(
                                         gSpriteWorld->workFrameP->framePort),
                                     &r2, &r2, 0x25, 0);
                            SWSetPortToWorkArea(gSpriteWorld);
                            RGBForeColor(blackColor);
                        }
                    }
                }
            }
        }
        DrawMap(box);
        if (selSystem != -1) {
            sy = gSystems[selSystem].yPos;
            r2.left = (box.left + w / 2
                              + gSystems[selSystem].xPos / mapZoom
                              - temp / mapZoom);
            r2.top = (box.top + h / 2
                             + sy / mapZoom
                             - savedY / mapZoom);
            r2.right = r2.left;
            r2.bottom = r2.top;
            if (mapZoom <= 0.5)
                InsetRect(&r2, -8, -8);
            else
                InsetRect(&r2, -6, -6);
            RGBForeColor(&brightGreen);
            PenSize(1, 1);
            MoveTo(r2.left + 1, r2.top);
            LineTo(r2.left + 4, r2.top);
            MoveTo(r2.left + 1, r2.bottom);
            LineTo(r2.left + 4, r2.bottom);
            MoveTo(r2.right - 1, r2.top);
            LineTo(r2.right - 4, r2.top);
            MoveTo(r2.right - 1, r2.bottom);
            LineTo(r2.right - 4, r2.bottom);
            MoveTo(r2.left, r2.top + 1);
            LineTo(r2.left, r2.top + 4);
            MoveTo(r2.left, r2.bottom - 1);
            LineTo(r2.left, r2.bottom - 4);
            MoveTo(r2.right, r2.top + 1);
            LineTo(r2.right, r2.top + 4);
            MoveTo(r2.right, r2.bottom - 1);
            LineTo(r2.right, r2.bottom - 4);
            RGBForeColor(blackColor);
        }
        RGBForeColor(whiteColor);
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].loaded != 0 && gSystems[i].exploreLevel > 0
                && (mapZoom <= 1.1 || i == selSystem)) {
                MoveTo((box.left + w / 2 + gSystems[i].xPos / mapZoom
                               - temp / mapZoom + 7.0),
                       (box.top + h / 2 + gSystems[i].yPos / mapZoom
                               - savedY / mapZoom + 4.0));
                DrawString(gSystems[i].name);
            }
        }
        RGBForeColor(blackColor);
        didUpdateMap = 1;
        if (partialMapDialogUpdate != 0) {
            SWSetPortToWindow(gSpriteWorld);
            SetDialogGWorld(mapDialog);
            RGBForeColor(blackColor);
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(loadPort.world),
                     GetPortBitMapForCopyBits(GetDialogPort(mapDialog)),
                     &box, &box, 0, 0);
        } else {
            SetGWorldContainer(&loadPort);
            RGBForeColor(blackColor);
            CopyBits((PixMapPtr)GetPortBitMapForCopyBits(
                         gSpriteWorld->workFrameP->framePort),
                     (BitMapPtr)GetGWCPixMap(&loadPort),
                     &box, &box, 0, 0);
        }
    }
    if (partialMapDialogUpdate == 0) {
        GetDialogItem(mapDialog, 2, &itemType, &item, &box);
        GetDialogItem(mapDialog, 6, &itemType, &item, &box6);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(box6.left + 1, box.top);
        LineTo(box6.right - 2, box.top);
        sel = selSystem;
        if (hypergateMapMode != 0
            && (selSystem == shipData->curSystem
                || selSystem == -1)) {
            RGBForeColor(&rgbLightGrey);
            GetIndString(text, kGameStrList, 0x134);
            MoveTo(box.left + box.right / 2 - StringWidth(text) / 2,
                   box.top + 0x18);
            DrawString(text);
        } else {
            RGBForeColor(&rgbLightGrey);
            MoveTo(box.left + 10, box.top + 0xc);
            DrawIndString(kGameStrList, 0x135);
            RGBForeColor(whiteColor);
            MoveTo(box.left + 0x2d, box.top + 0xc);
            if (gSystems[sel].exploreLevel > 0) {
                short count;
                short drawn;
                short xoff;
                short sw;
                count = 0;
                drawn = 0;
                xoff = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (gSystems[sel].navDefault[i] != -1
                        && (stellarData[gSystems[sel].navDefault[i]].flags & 0x20) == 0
                        && StellarIsLandable(&stellarData[gSystems[sel].navDefault[i]]) != 0)
                        count++;
                }
                if (count > 0) {
                    for (i = 0; i < kNavSlots; i++) {
                        if (gSystems[sel].navDefault[i] != -1
                            && (stellarData[gSystems[sel].navDefault[i]].flags & 0x20) == 0
                            && StellarIsLandable(&stellarData[gSystems[sel].navDefault[i]]) != 0) {
                            sw = StringWidth(stellarData[gSystems[sel].navDefault[i]].name);
                            if (sw + (box.left + 0x2d) + xoff > box6.right - 0x14) {
                                MoveTo(box.left + 0x2d, box.top + 0x18);
                                xoff = 0;
                            } else {
                                xoff += sw;
                            }
                            DrawString(stellarData[gSystems[sel].navDefault[i]].name);
                            drawn++;
                            if (count > drawn) {
                                DrawString("\p, ");
                                xoff += StringWidth("\p, ");
                            }
                        }
                    }
                } else {
                    DrawIndString(kGameStrList, 0x14f);
                }
            } else {
                if (cheatsActive != 0) {
                    NumToString(sel + 128, buf);
                    DrawString(buf);
                } else {
                    DrawIndString(kGameStrList, 0x136);
                }
            }
            RGBForeColor(&rgbLightGrey);
            MoveTo(box.left + 10, box.top + 0x24);
            DrawIndString(kGameStrList, 0x137);
            RGBForeColor(whiteColor);
            MoveTo(box.left + 0x6e, box.top + 0x24);
            if (gSystems[sel].exploreLevel > 0) {
                strcpy((char *)buf, "");
                for (j = 0; j < kNavSlots; j++) {
                    if (gSystems[sel].navDefault[j] != -1
                        && stellarData[gSystems[sel].navDefault[j]].gravity != 0) {
                        AppendIndString((char *)buf, kGameStrList, 0x138);
                        if (gSystems[sel].asteroidCount > 0
                            || gSystems[sel].interference > 0
                            || gSystems[sel].murk > 0)
                            strcat((char *)buf, ", ");
                        break;
                    }
                }
                if (gSystems[sel].asteroidCount > 0) {
                    if (gSystems[sel].asteroidCount <= 3)
                        AppendIndString((char *)buf, kGameStrList, 0x139);
                    else if (gSystems[sel].asteroidCount <= 6)
                        AppendIndString((char *)buf, kGameStrList, 0x13a);
                    else
                        AppendIndString((char *)buf, kGameStrList, 0x13b);
                    strcat((char *)buf, " ");
                    AppendIndString((char *)buf, kGameStrList, 0x13c);
                    if (gSystems[sel].interference > 0
                        || gSystems[sel].murk > 0)
                        strcat((char *)buf, ", ");
                }
                if (gSystems[sel].interference > 0) {
                    if (gSystems[sel].interference <= 0x21)
                        AppendIndString((char *)buf, kGameStrList, 0x13d);
                    else if (gSystems[sel].interference <= 0x42)
                        AppendIndString((char *)buf, kGameStrList, 0x13e);
                    else
                        AppendIndString((char *)buf, kGameStrList, 0x13f);
                    strcat((char *)buf, " ");
                    AppendIndString((char *)buf, kGameStrList, 0x140);
                    if (gSystems[sel].murk > 0)
                        strcat((char *)buf, ", ");
                }
                if (gSystems[sel].murk > 0) {
                    if (gSystems[sel].murk <= 0x1e)
                        AppendIndString((char *)buf, kGameStrList, 0x141);
                    else if (gSystems[sel].murk <= 0x3c)
                        AppendIndString((char *)buf, kGameStrList, 0x142);
                    else
                        AppendIndString((char *)buf, kGameStrList, 0x143);
                    strcat((char *)buf, " ");
                    AppendIndString((char *)buf, kGameStrList, 0x144);
                }
                if ((int)buf[0] == 0)  /* (int): exe movzx+test; bare ==0 folds to cmp byte */
                    AppendIndString((char *)buf, kGameStrList, 0x14f);
                buf[0] = toupper(buf[0]);
                CtoPstr((char *)buf);
                DrawString(buf);
            } else {
                if (cheatsActive != 0) {
                    NumToString(sel + 128, buf);
                    DrawString(buf);
                } else {
                    DrawIndString(kGameStrList, 0x136);
                }
            }
        }
        RGBForeColor(&rgbDarkGrey);
        if (cheatsActive != 0) {
            NumToString((1.0 / mapZoom * 1000.0), buf);
        } else {
            DateToMedString(gameTime, (char *)buf);
            CtoPstr((char *)buf);
        }
        MoveTo(box.right - StringWidth(buf) - 5, box.top + 0x24);
        DrawString(buf);
        RGBForeColor(blackColor);
        didUpdateMap = 1;
        GetDialogItem(mapDialog, 6, &itemType, &item, &r2);
        RGBForeColor(blackColor);
        MoveTo(r2.left + 1, r2.bottom - 1);
        LineTo(r2.right - 2, r2.bottom - 1);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(r2.left + 10, r2.top + 0xc);
        RGBForeColor(&rgbLightGrey);
        linked = 0;
        if (shipData->navTargetKind == 3) {
            if (shipData->targetStellar == -1) {
                if (selSystem == shipData->curSystem) {
                    DrawIndString(kGameStrList, 0x151);
                } else {
                    DrawIndString(kGameStrList, 0x152);
                }
            } else {
                DrawIndString(kGameStrList, 0x153);
            }
        } else {
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < 0x800) {
                for (j = 0; j < kSpobHyperLinks; j++) {
                    st = stellarData[hypergateMapModeStellar].hyperLinks[j];
                    /* BUG: i is the finished name loop's counter (0x800), so an empty link is never skipped */
                    if (-i != st) {
                        st2 = stellarData[st].system;
                        if (st2 < 0 || st2 >= 0x800)
                            st2 = FindSystemFromStellar(st);
                        found = FindActiveCoLocatedSystem(st2);
                        if (found == selSystem) {
                            linked = 1;
                            break;
                        }
                    }
                }
            }
            if (selSystem == shipData->curSystem) {
                DrawIndString(kGameStrList, 0x151);
            } else if (linked != 0) {
                DrawIndString(kGameStrList, 0x154);
            } else {
                DrawIndString(kGameStrList, 0x152);
            }
        }
        MoveTo(r2.left + 0xf, r2.top + 0x1a);
        RGBForeColor(whiteColor);
        if (gSystems[sel].exploreLevel <= 0) {
            if (cheatsActive != 0) {
                NumToString(sel + 128, buf);
                DrawString(buf);
            } else {
                DrawIndString(kGameStrList, 0x136);
            }
        } else {
            if (cheatsActive != 0) {
                NumToString(sel + 128, buf);
                DrawString(buf);
            } else {
                DrawString(gSystems[sel].name);
            }
            if (SystemIsInhabited(sel) != 0) {
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0x4b);
                DrawIndString(kGameStrList, 0x145);
                RGBForeColor(whiteColor);
                MoveTo(r2.left + 0xf, r2.top + 0x58);
                govt = gSystems[sel].govt;
                if (govt != -1) {
                    if (cheatsActive != 0) {
                        NumToString(govt + 128, buf);
                        DrawString("\pID ");
                        DrawString(buf);
                    } else {
                        DrawString(govtData[govt].name);
                    }
                } else {
                    DrawIndString(kGameStrList, 0x14d);
                }
                MoveTo(r2.left + 10, r2.top + 0x77);
                RGBForeColor(&rgbLightGrey);
                DrawIndString(kGameStrList, 0x146);
                RGBForeColor(whiteColor);
                MoveTo(r2.left + 0xf, r2.top + 0x84);
                if (cheatsActive != 0) {
                    NumToString(playerLegalStatus[sel], buf);
                    DrawString(buf);
                } else {
                    DrawLegalStatusString(sel);
                }
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0x9b);
                DrawIndString(kGameStrList, 0x147);
                RGBForeColor(whiteColor);
                if (gSystems[sel].exploreLevel < 2) {
                    MoveTo(r2.left + 0xf, r2.top + 0xa8);
                    DrawIndString(kGameStrList, 0x136);
                } else {
                    short yoff;
                    yoff = 0;
                    for (k = 0, name = cargoName[0]; k <= 5; k++, name += sizeof(cargoName[0])) {
                        if (SystemTradesGoods(sel, k) != 0) {
                            MoveTo(r2.left + 0xf, r2.top + 0xa8 + yoff);
                            DrawString(name);
                            yoff += 0xc;
                        }
                    }
                    if (yoff == 0) {
                        MoveTo(r2.left + 0xf, r2.top + 0xa8);
                        DrawIndString(kGameStrList, 0x14f);
                    }
                }
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0xfa);
                DrawIndString(kGameStrList, 0x148);
                temp = 0;
                for (n = 0; n <= 2; n++) {
                    if (SystemHasService(sel, n) != 0)
                        temp++;
                }
                if (temp > 0) {
                    RGBForeColor(whiteColor);
                    if (gSystems[sel].exploreLevel < 2) {
                        MoveTo(r2.left + 0xf, r2.top + 0x107);
                        DrawIndString(kGameStrList, 0x136);
                    } else {
                        short yoff;
                        yoff = 0;
                        for (j = 0; j <= 2; j++) {
                            if (SystemHasService(sel, j) != 0) {
                                MoveTo(r2.left + 0xf, r2.top + 0x107 + yoff);
                                DrawIndString(kGameStrList, j + 0x149);
                                yoff += 0xc;
                            }
                        }
                    }
                } else {
                    if (gSystems[sel].exploreLevel >= 2) {
                        MoveTo(r2.left + 0xf, r2.top + 0x106);
                        RGBForeColor(whiteColor);
                        DrawIndString(kGameStrList, 0x14c);
                    } else if (gSystems[sel].exploreLevel >= 1) {
                        MoveTo(r2.left + 0xf, r2.top + 0x106);
                        RGBForeColor(whiteColor);
                        DrawIndString(kGameStrList, 0x136);
                    }
                }
            } else {
                MoveTo(r2.left + 10, r2.top + 0x4b);
                RGBForeColor(whiteColor);
                DrawIndString(kGameStrList, 0x14e);
            }
        }
        didUpdateMap = 1;
        SetGWorldContainer(&loadPort);
        DrawMapButtons(-1);
        UnsetGWorldContainer(&loadPort);
        SWSetPortToWindow(gSpriteWorld);
        SetDialogGWorld(mapDialog);
        RGBForeColor(blackColor);
        CopyBits((PixMapPtr)GetPortBitMapForCopyBits(loadPort.world),
                 GetPortBitMapForCopyBits(GetDialogPort(mapDialog)),
                 GetDialogPortRect(mapDialog), GetDialogPortRect(mapDialog),
                 0, 0);
    }
}


/* ScrollMapDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a7470
void ScrollMapDialog(short dh, short dv)
{
    short itemType;
    void *item;
    Rect box;

    if (dh == 0 && dv == 0)
        return;
    SetDialogGWorld(mapDialog);
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    box.left += 1;
    box.top += 1;
    box.right -= 1;
    box.bottom -= 1;
    partialMapDialogUpdate = 1;
    MapDialogUpdate();
    partialMapDialogUpdate = 0;
}

/* FUN_004a74e0. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a74e0
void FUN_004a74e0(void)
{
}


/* MapFilter (mac _MapFilter). STATE: MATCH 100.0% 670/670, byte-identical yes (2440 bytes),
   relocregcheck 0 of 138. OPEN: none. */
// FUNCTION: EVNOVA 0x004a74f0
char MapFilter(DialogPtr dlg, EventRecord *event, short *itemHit)
{
    void *item;
    int key;
    short i;
    short j;
    short k;
    short n;
    short count;
    short cur;
    short sys;
    short list[kSpobHyperLinks];
    char ok[kMaxSystemLinks];
    Rect box;
    short itemType;
    int what;
    Point pt;

    if (mapZoom > 0.5)
        canZoomIn = 1;
    else
        canZoomIn = 0;
    if (mapZoom < 2.0)
        canZoomOut = 1;
    else
        canZoomOut = 0;
    didUpdateMap = 0;
    what = event->what;
    if (what == kKeyDown || what == kAutoKey) {
        key = tolower((unsigned char)event->message);
        if (what == kKeyDown && (key == kReturnChar || key == kEscapeChar)) {
            *itemHit = 1;
            return 1;
        } else if ((key == kTabChar || key == 0x5c) && onPlanet == 0) {
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < kMaxSystems) {
                n = 0;
                for (i = 0; i < kSpobHyperLinks; i++)
                    list[i] = -1;
                for (i = 0; i < kSpobHyperLinks; i++) {
                    sys = stellarData[hypergateMapModeStellar].hyperLinks[i];
                    if (sys >= 0 && sys < kMaxSystems) {
                        list[n] = FindSystemFromStellar(sys);
                        n++;
                    }
                }
                if (n > 0) {
                    cur = -1;
                    for (i = 0; i < n; i++) {
                        if (selSystem == list[i]) {
                            cur = i;
                            break;
                        }
                    }
                    if (cur == -1)
                        cur = 0;
                    else if ((*(unsigned short *)&event->modifiers & shiftKey) != 0) {
                        if (cur != 0)
                            cur -= 1;
                        else
                            cur = n - 1;
                    } else
                        cur = (cur + 1) % n;
                    shipData[0].navTargetKind = 4;
                    shipData[0].targetStellar = list[cur];
                    selSystem = list[cur];
                    InvalDialogItem(mapDialog, 2);
                    InvalDialogItem(mapDialog, 3);
                    InvalDialogItem(mapDialog, 6);
                    SetDialogGWorld(mapDialog);
                }
            } else {
                shipData[0].navTargetKind = 3;
                count = 0;
                for (i = 0; i < kMaxSystemLinks; i++) {
                    ok[i] = 0;
                    if (gSystems[shipData[0].curSystem].links[i] != -1) {
                        if (FindActiveCoLocatedSystem(
                                gSystems[shipData[0].curSystem].links[i]) != -1) {
                            ok[i] = 1;
                            count++;
                        }
                    }
                }
                if (count > 0) {
                    do {
                        shipData[0].targetStellar += 1;
                        if (shipData[0].targetStellar >= kMaxSystemLinks)
                            shipData[0].targetStellar = 0;
                        if (shipData[0].targetStellar < 0)
                            shipData[0].targetStellar = kMaxSystemLinks - 1;
                    } while (ok[shipData[0].targetStellar] == 0);
                } else
                    shipData[0].targetStellar = -1;
                statusRedraw[0] = 1;
                InvalDialogItem(mapDialog, 2);
                InvalDialogItem(mapDialog, 3);
                InvalDialogItem(mapDialog, 6);
                DrawStatus();
                SetDialogGWorld(mapDialog);
                if (shipData[0].navTargetKind == 3 && shipData[0].targetStellar != -1)
                    selSystem = FindActiveCoLocatedSystem(
                        gSystems[shipData[0].curSystem]
                            .links[shipData[0].targetStellar]);
                else
                    selSystem = shipData[0].curSystem;
            }
        } else if (what == kKeyDown && (key == 'c' || key == ' ')) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            mapScroll.x = gSystems[shipData[0].curSystem].xPos;
            mapScroll.y = gSystems[shipData[0].curSystem].yPos;
            selSystem = shipData[0].curSystem;
            InvalDialogItem(mapDialog, 2);
            InvalDialogItem(mapDialog, 6);
            InvalDialogItem(mapDialog, 3);
            GetDialogItem(mapDialog, 3, &itemType, &item, &box);
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == '-') {
            *itemHit = 4;
            return 1;
        } else if (key == '+' || key == '=') {
            *itemHit = 5;
            return 1;
        } else if (key == 0x96) {
            mapScroll.x -= 10;
            InvalDialogItem(mapDialog, 3);
            GetDialogItem(mapDialog, 3, &itemType, &item, &box);
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == 0x15) {
            mapScroll.x += 10;
            InvalDialogItem(mapDialog, 3);
            GetDialogItem(mapDialog, 3, &itemType, &item, &box);
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == 0xb) {
            mapScroll.y -= 10;
            InvalDialogItem(mapDialog, 3);
            GetDialogItem(mapDialog, 3, &itemType, &item, &box);
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == 0xa) {
            mapScroll.y += 10;
            InvalDialogItem(mapDialog, 3);
            GetDialogItem(mapDialog, 3, &itemType, &item, &box);
            if (gShowInfluence != 0)
                RecalcAllMapInfluence(&box);
        } else if (what == kKeyDown && (key == 'f' || key == 's')) {
            *itemHit = 0xa;
            return 1;
        } else if (what == kKeyDown && KeyCheck(keyPrefs.map) != 0) {
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)event->what == kMouseDown) {
        pt.v = event->where.v;
        pt.h = event->where.h;
        GetDialogItem(mapDialog, 3, &itemType, &item, &box);
        if (PtInRect(pt.h, pt.v, &box) != 0) {
            *itemHit = 3;
            return 1;
        }
        MapDialogUpdate();
        SetDialogGWorld(mapDialog);
        sys = TrackMapButtons(pt);
        if (sys == 0) {
            *itemHit = 1;
            return 1;
        } else if (sys == 1) {
            *itemHit = 4;
            return 1;
        } else if (sys == 2) {
            *itemHit = 5;
            return 1;
        } else if (sys == 3) {
            *itemHit = 8;
            return 1;
        } else if (sys == 4) {
            *itemHit = 9;
            return 1;
        } else if (sys == 5) {
            *itemHit = 0xa;
            return 1;
        } else {
            *itemHit = -1;
            return 1;
        }
    }
    if ((int)event->what == kUpdateEvt)
        MapDialogUpdate();
    if ((int)event->what == kNullEvent && gShowInfluence != 0
        && influenceCoordsCached != 0) {
        if (nextInfluenceSyst < 0) {
            nextInfluenceSyst = 0;
            influenceStepSize = 1;
            for (i = 0; i < 256; i++) {
                for (j = 0; j < 256; j++) {
                    for (k = 0; k < 4; k++) {
                        influenceOwner[i][j][k] = 0;
                        influenceValue[i][j][k] = 0;
                    }
                }
            }
        } else {
            GetDialogItem(mapDialog, 3, &itemType, &item, &box);
            while (nextInfluenceSyst < kMaxSystems) {
                nextInfluenceSyst += 1;
                sys = FindActiveCoLocatedSystem(nextInfluenceSyst - 1);
                if (sys != -1) {
                    if (AddSystemInfluenceToMap(sys, &box) != 0)
                        break;
                }
            }
            if (nextInfluenceSyst == kMaxSystems) {
                nextInfluenceSyst = kMaxSystems + 1;
                SetDialogGWorld(mapDialog);
                InvalDialogItem(mapDialog, 3);
            }
        }
    }
    return 0;
}


/* AdvanceJumpRoute. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a7e80
void AdvanceJumpRoute(void)
{
    short i;

    if (hyperRoute[0] == -1) {
        for (i = 0; i < kMaxHyperRoute - 1; i++)
            hyperRoute[i] = hyperRoute[i + 1];
        hyperRoute[kMaxHyperRoute - 1] = -1;
    }
    if (hyperRoute[1] == -1) {
        for (i = 0; i < kMaxHyperRoute; i++)
            hyperRoute[i] = -1;
    }
}

/* CheckJumpRouteArrival. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a7fc0
void CheckJumpRouteArrival(void)
{
    short i;
    short group;

    if (hyperRoute[1] == -1)
        group = -1;
    else
        group = FindActiveCoLocatedSystem(hyperRoute[1]);
    if (group == FindActiveCoLocatedSystem(shipData[0].curSystem)) {
        hyperRoute[0] = -1;
        AdvanceJumpRoute();
    } else if (hyperRoute[1] == -1) {
        for (i = 0; i < kMaxHyperRoute; i++)
            hyperRoute[i] = -1;
    }
}

/*
 * This writes the link INDEX into +0x6c, the same field shipobj.h calls
 * targetStellar - so that word is a destination whose meaning depends on the
 * mode in +0x6a, not a stellar index outright.
 */

/* SelectHyperLinkToSelection. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a8080
void SelectHyperLinkToSelection(void)
{
    short i;
    short sys;
    short target;
    short found;

    if ((sys = hyperRoute[1]) != -1) {
        target = FindActiveCoLocatedSystem(sys);
        for (i = 0; i < kMaxSystemLinks; i++) {
            if (gSystems[shipData[0].curSystem].links[i] != -1) {
                found = FindActiveCoLocatedSystem(gSystems[shipData[0].curSystem].links[i]);
                if (found == target) {
                    shipData[0].navTargetKind = 3;
                    shipData[0].targetStellar = i;
                    statusRedraw[0] = 1;
                    break;
                }
            }
        }
    }
}

/* DrawMap (mac _DrawMap). STATE: MATCH 100.0%, 1758/1758, byte-identical yes. The aggregates'
   declaration order (arrowBox before sysBox) is load-bearing: the frame heapsort it feeds breaks
   savedX's use tie with the left + w/2 slot (notes). OPEN: none. */
// FUNCTION: EVNOVA 0x004a8100
void DrawMap(Rect box)
{
    FloatPoint destPt;
    FloatPoint curPt;
    Rect arrowBox;
    Rect sysBox;
    RGBColor color;
    short savedX;
    short savedY;
    short w;
    short h;
    short i;
    short k;
    short cur;
    short dest;
    short st;
    short angle;
    short rx2;
    short linkX;
    short j;
    short rx1;
    short destX;
    short ry1;
    short linkY;
    short sysY;
    char show;
    char redArrow;
    char greenArrow;

    w = box.right - box.left;
    h = box.bottom - box.top;
    savedX = mapScroll.x;
    savedY = mapScroll.y;
    for (i = 0; i < kMaxSystems; i++)
        mapSystemDrawn[i] = 0;
    if (hypergateMapMode == 0) {
        for (j = 0; j < kMaxHyperRoute - 1; j++) {
            if (hyperRoute[j] != -1 && hyperRoute[j + 1] != -1) {
                rx1 = gSystems[hyperRoute[j]].xPos;
                ry1 = gSystems[hyperRoute[j]].yPos;
                rx2 = gSystems[hyperRoute[j + 1]].xPos;
                linkY = gSystems[hyperRoute[j + 1]].yPos;
                PenSize(2, 2);
                RGBForeColor(&brightGreen);
                MoveTo((box.left + w / 2 + rx1 / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + ry1 / mapZoom
                               - savedY / mapZoom));
                LineTo((box.left + w / 2 + rx2 / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + linkY / mapZoom
                               - savedY / mapZoom));
                PenSize(1, 1);
            }
        }
    }
    cur = shipData->curSystem;
    for (j = 0; j < kMaxSystemLinks; j++) {
        if (gSystems[cur].links[j] != -1) {
            dest = FindActiveCoLocatedSystem(gSystems[cur].links[j]);
            if (dest != -1) {
                if (hypergateMapMode != 0)
                    RGBForeColor(&rgbRealDarkGrey);
                else
                    RGBForeColor(&rgbLessDarkGrey);
                linkX = gSystems[dest].xPos;
                linkY = gSystems[dest].yPos;
                MoveTo((box.left + w / 2 + gSystems[cur].xPos / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + gSystems[cur].yPos / mapZoom
                               - savedY / mapZoom));
                LineTo((box.left + w / 2 + linkX / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + linkY / mapZoom
                               - savedY / mapZoom));
            }
        }
    }
    mapSystemDrawn[shipData->curSystem] = 1;
    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].loaded != 0 && gSystems[i].exploreLevel > 0
            && gSystems[i].known != 0) {
            for (j = 0; j < kMaxSystemLinks; j++) {
                if (gSystems[i].links[j] != -1) {
                    dest = FindActiveCoLocatedSystem(gSystems[i].links[j]);
                    if (dest != -1 && mapSystemDrawn[dest] == 0) {
                        if (i == shipData->curSystem && shipData->navTargetKind == 3
                            && j == shipData->targetStellar)
                            RGBForeColor(&dimGreen);
                        else if (hypergateMapMode != 0)
                            RGBForeColor(&rgbRealDarkGrey);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        destX = gSystems[dest].xPos;
                        linkY = gSystems[dest].yPos;
                        MoveTo((box.left + w / 2
                                       + gSystems[i].xPos / mapZoom
                                       - savedX / mapZoom),
                               (box.top + h / 2
                                       + gSystems[i].yPos / mapZoom
                                       - savedY / mapZoom));
                        LineTo((box.left + w / 2 + destX / mapZoom
                                       - savedX / mapZoom),
                               (box.top + h / 2 + linkY / mapZoom
                                       - savedY / mapZoom));
                    }
                }
            }
            mapSystemDrawn[i] = 1;
        }
    }
    if (shipData->navTargetKind == 3) {
        cur = shipData->curSystem;
        if (shipData->targetStellar >= 0 && shipData->targetStellar < 16
            && gSystems[cur].links[shipData->targetStellar] != -1) {
            dest = FindActiveCoLocatedSystem(
                       gSystems[cur].links[shipData->targetStellar]);
            if (dest != -1) {
                RGBForeColor(&dimGreen);
                ry1 = gSystems[cur].yPos;
                rx2 = gSystems[dest].xPos;
                linkY = gSystems[dest].yPos;
                MoveTo((box.left + w / 2 + gSystems[cur].xPos / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + ry1 / mapZoom
                               - savedY / mapZoom));
                LineTo((box.left + w / 2 + rx2 / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + linkY / mapZoom
                               - savedY / mapZoom));
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        redArrow = 0;
        greenArrow = 0;
        if (gSystems[i].loaded != 0 && gSystems[i].prepared != 0) {
            if (shipData->curSystem == i) {
                show = 1;
            } else {
                show = 0;
                if (FindActiveCoLocatedSystem(i) != -1
                    && gSystems[i].exploreLevel > 0)
                    show = 1;
                if (gSystems[i].known != 0)
                    show = 1;
            }
            for (k = 0; k < 32; k++) {
                if (missionSyst[k] != -1
                    && FindFirstCoLocatedSystem(i)
                       == FindFirstCoLocatedSystem(missionSyst[k])) {
                    redArrow = 1;
                    break;
                }
            }
            if (greenMissionSyst != -1
                && FindFirstCoLocatedSystem(i)
                   == FindFirstCoLocatedSystem(greenMissionSyst)) {
                greenArrow = 1;
                if (multiArrowsOnSingleSystem == 0)
                    redArrow = 0;
            }
            if (show != 0 || redArrow != 0 || greenArrow != 0) {
                linkY = gSystems[i].yPos;
                sysBox.left = (box.left + w / 2
                                      + gSystems[i].xPos / mapZoom
                                      - savedX / mapZoom);
                sysBox.top = (box.top + h / 2 + linkY / mapZoom
                                     - savedY / mapZoom);
                sysBox.right = sysBox.left;
                sysBox.bottom = sysBox.top;
                if (mapZoom <= 0.5) {
                    PenSize(2, 2);
                    InsetRect(&sysBox, -6, -6);
                } else {
                    PenSize(1, 1);
                    InsetRect(&sysBox, -4, -4);
                }
                RGBForeColor(blackColor);
                PaintOval(&sysBox);
                ColorCodeSystem(i, &color);
                RGBForeColor(&color);
                FrameOval(&sysBox);
                RGBForeColor(blackColor);
                if (redArrow != 0) {
                    SetRect(&arrowBox, sysBox.left - 0x10, sysBox.top - 0x10,
                            sysBox.left, sysBox.top);
                    if (redMapArrow != NULL)
                        nv_BlitImageOr(&arrowBox, redMapArrow);
                }
                if (greenArrow != 0) {
                    SetRect(&arrowBox, sysBox.right, sysBox.top - 0x10,
                            sysBox.right + 0x10, sysBox.top);
                    if (greenMapArrow != NULL)
                        nv_BlitImageOr(&arrowBox, greenMapArrow);
                }
            }
        }
    }
    PenSize(1, 1);
    cur = shipData->curSystem;
    if (cur >= 0 && cur < kMaxSystems) {
        sysY = gSystems[cur].yPos;
        sysBox.left = (box.left + w / 2 + gSystems[cur].xPos / mapZoom
                              - savedX / mapZoom);
        sysBox.top = (box.top + h / 2 + sysY / mapZoom
                             - savedY / mapZoom);
        sysBox.right = sysBox.left;
        sysBox.bottom = sysBox.top;
        if (mapZoom <= 0.5)
            InsetRect(&sysBox, -3, -3);
        else
            InsetRect(&sysBox, -2, -2);
        RGBForeColor(cyanColor);
        PaintOval(&sysBox);
        RGBForeColor(blackColor);
    }
    /* BUG: the upper bound tests hypergateMapMode, not the stellar, and can never fail
       (exe 0x004a921e) */
    if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
        && hypergateMapMode < kMaxSystems) {
        for (j = 0; j < kSpobHyperLinks; j++) {
            if (stellarData[hypergateMapModeStellar].hyperLinks[j] != -1) {
                st = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[j]]
                         .system;
                if (st < 0 || st >= kMaxSystems)
                    st = FindSystemFromStellar(
                             stellarData[hypergateMapModeStellar].hyperLinks[j]);
                dest = FindActiveCoLocatedSystem(st);
                if (dest >= 0 && dest < kMaxSystems) {
                    if (dest == selSystem) {
                        RGBForeColor(whiteColor);
                        PenSize(3, 3);
                    } else {
                        RGBForeColor(cyanColor);
                        PenSize(2, 2);
                    }
                    ry1 = gSystems[shipData->curSystem].yPos;
                    linkX = gSystems[dest].xPos;
                    sysY = gSystems[dest].yPos;
                    curPt.x = (box.left + w / 2
                                      + gSystems[shipData->curSystem].xPos
                                        / mapZoom
                                      - savedX / mapZoom);
                    curPt.y = (box.top + h / 2 + ry1 / mapZoom
                                      - savedY / mapZoom);
                    destPt.x = (box.left + w / 2 + linkX / mapZoom
                                       - savedX / mapZoom);
                    destPt.y = (box.top + h / 2 + sysY / mapZoom
                                       - savedY / mapZoom);
                    MoveTo(destPt.x, destPt.y);
                    LineTo(curPt.x, curPt.y);
                    angle = CalcInterceptAngle(&destPt, &curPt);
                    curPt.x = destPt.x;
                    curPt.y = destPt.y;
                    Accel(angle + 45, 8.0f, &curPt);
                    MoveTo(destPt.x, destPt.y);
                    LineTo(curPt.x, curPt.y);
                    curPt.x = destPt.x;
                    curPt.y = destPt.y;
                    Accel((angle + 315) % 360, 8.0f, &curPt);
                    MoveTo(destPt.x, destPt.y);
                    LineTo(curPt.x, curPt.y);
                    gSystems[dest].known = 1;
                }
            }
        }
        PenSize(1, 1);
        RGBForeColor(blackColor);
    }
    RGBForeColor(whiteColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    for (i = 0; i < kMaxSystems; i++) {
        if (gSystems[i].loaded != 0 && gSystems[i].exploreLevel > 0
            && (mapZoom <= 1.1 || i == selSystem)) {
            MoveTo((box.left + w / 2 + gSystems[i].xPos / mapZoom
                           - savedX / mapZoom + 7.0),
                   (box.top + h / 2 + gSystems[i].yPos / mapZoom
                           - savedY / mapZoom + 4.0));
            DrawString(gSystems[i].name);
        }
    }
}


/* DrawMapInFloatingFrame (mac _DrawMapInFloatingFrame; was named ClearStarfield). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a99f0
void DrawMapInFloatingFrame(void)
{
    double savedZoom;

    SWSetPortToFrame(floatingMapFrame);
    mapScroll.x = gSystems[shipData[0].curSystem].xPos;
    mapScroll.y = gSystems[shipData[0].curSystem].yPos;

    savedZoom = mapZoom;
    mapZoom = floatingMapZoom;

    RGBForeColor(blackColor);
    PaintRect(&floatingMapFrame->frameRect);

    if (gSpriteWorld->pixelDepth == 8) {
        RGBForeColor(blackColor);
        DrawMap(floatingMapFrame->frameRect);
        RGBForeColor(&floatingBorderColor);
        FrameRect(&floatingMapFrame->frameRect);
        RGBForeColor(blackColor);
    } else {
        RGBForeColor(&floatingBorderColor);
        FrameRect(&floatingMapFrame->frameRect);
        RGBForeColor(blackColor);
        DrawMap(floatingMapFrame->frameRect);
    }

    mapZoom = savedZoom;
    SWSetPortToWindow(gSpriteWorld);
}

/*
 * The 1.0 stored back into the time scale is the exe's two-dword constant store,
 * not an FPU one. 0x005759e8 is NOT a global -- it is the pooled literal 0.0;
 * its neighbour 0x005759e0 IS a variable despite sitting in the same pooled run.
 */

/* StartHyperJump. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9b30
void StartHyperJump(void)
{
    short link;

    if (floatingMapZoom <= 0.0)
        floatingMapZoom = 1.0;
    selSystem = -1;
    if (shipData->navTargetKind == kDepartureRoutePicked) {
        link = shipData->targetStellar;
        if (link >= 0 && link < 16) {
            link = gSystems[shipData->curSystem].links[link];
            if (gSystems[link].loaded != 0)
                selSystem = link;
        }
    }
    hypergateMapMode = 0;
    showFloatingMap = 1;
    gJumpStartTick = ReadTick();
    RecalcMissionSystsForMap();
    DrawMapInFloatingFrame();
}


#define kInfluenceCoarseStep 8
#define kInfluenceGridMax 255
#define kInfluenceGridSize 256
#define kInfluenceLayers 4

/* MapPosToInfluenceCoords (mac _MapPosToInfluenceCoords). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9bf0
void MapPosToInfluenceCoords(short x, short y, Rect *r, short *outX, short *outY)
{
    short sx;
    short sy;

    sx = ((x - mapScroll.x) / mapZoom);
    sy = ((y - mapScroll.y) / mapZoom);
    *outX = sx / 2;
    *outY = sy / 2;
    *outX += (r->right - r->left) / 2 / 2;
    *outY += (r->bottom - r->top) / 2 / 2;
}

/* RecalcAllMapInfluence (mac _RecalcAllMapInfluence). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9d10
void RecalcAllMapInfluence(Rect *r)
{
    influenceStepSize = kInfluenceCoarseStep;
    nextInfluenceSyst = -1;
    influenceCoordsCached = 0;
    RecalcMapInfluence(0, 0, kInfluenceGridMax, kInfluenceGridMax, r);
}

/* RecalcMapInfluence (mac _RecalcMapInfluence). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9d50
void RecalcMapInfluence(long left, long top, long right, long bottom, Rect *r)
{
    short i;
    short x;
    short dy;
    short dx;
    short k;
    short y;
    short group;
    short syst;
    char seen[kMaxSystems];

    if (influenceStepSize < 1)
        influenceStepSize = 1;

    for (x = 0; x < kInfluenceGridSize; x += influenceStepSize) {
        for (y = 0; y < kInfluenceGridSize; y += influenceStepSize) {
            for (k = 0; k < kInfluenceLayers; k++) {
                influenceOwner[x][y][k] = 0;
                influenceValue[x][y][k] = 0;
            }
        }
    }

    for (i = 0; i < kMaxSystems; i++)
        seen[i] = 0;

    for (i = 0; i < kMaxSystems; i++) {
        group = gSystems[i].colocGroup;
        if (group == -1) {
            AddSystemInfluenceToMap(i, r);
        } else if (seen[group] == 0) {
            seen[group] = 1;
            syst = FindActiveCoLocatedSystem(i);
            if (syst != -1)
                AddSystemInfluenceToMap(syst, r);
        }
    }

    if (influenceStepSize > 1) {
        for (x = 0; x < kInfluenceGridSize; x += influenceStepSize) {
            for (y = 0; y < kInfluenceGridSize; y += influenceStepSize) {
                for (dx = 0; dx < influenceStepSize; dx++) {
                    for (dy = 0; dy < influenceStepSize; dy++) {
                        if (x + dx < kInfluenceGridSize
                            && y + dy < kInfluenceGridSize) {
                            for (k = 0; k < kInfluenceLayers; k++) {
                                influenceOwner[x + dx][y + dy][k] =
                                    influenceOwner[x][y][k];
                                influenceValue[x + dx][y + dy][k] =
                                    influenceValue[x][y][k];
                            }
                        }
                    }
                }
            }
        }
    }

    influenceCoordsCached = 1;
}

/* govtData +0x22 (gövt res+0x04) - the same word buttons.c and commdlg.c read */
#define kGovt2TightInfluence 0x2 /* half the radius, double the falloff (name TBC) */
#define kGovt2NoInfluence 0x4    /* keeps the govt off the overlay (name TBC) */

#define kInfluenceSlots 4
#define kInfluenceRadius 30.72
#define kTightInfluenceRadius 15.36
#define kInfluenceFalloff 0.2
#define kTightInfluenceFalloff 0.4

/* AddSystemInfluenceToMap (mac _AddSystemInfluenceToMap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aa070
char AddSystemInfluenceToMap(short sys, Rect *r)
{
    short mapX;
    short mapY;
    short govt;
    short radius;
    short gx;
    short gy;
    short k;
    short lowest;
    short lowestSlot;
    short distX;
    short distY;
    long r2;
    long d2;
    long strength;
    char placed;

    if (gSystems[sys].loaded != 0
        && gSystems[sys].known != 0
        && gSystems[sys].exploreLevel > 0
        && gSystems[sys].govt != -1
        && (govtData[gSystems[sys].govt].flags2 & kGovt2NoInfluence) == 0
        && SystemIsInhabited(sys) != 0) {
        MapPosToInfluenceCoords(gSystems[sys].xPos, gSystems[sys].yPos, r,
                                &mapX, &mapY);
        gSystems[sys].influenceX = mapX;
        gSystems[sys].influenceY = mapY;
        if (influenceStepSize > 1) {
            mapX -= mapX % influenceStepSize;
            mapY -= mapY % influenceStepSize;
        }

        govt = gSystems[sys].govt;
        if ((govtData[govt].flags2 & kGovt2TightInfluence) != 0)
            radius = (kTightInfluenceRadius / mapZoom);
        else
            radius = (kInfluenceRadius / mapZoom);
        if (influenceStepSize > 1)
            radius -= radius % influenceStepSize;
        r2 = radius * radius;

        if (mapX >= -radius && mapX <= radius + kInfluenceGridSize
            && mapY >= -radius && mapY <= radius + kInfluenceGridSize) {
            for (gx = mapX - radius; gx <= mapX + radius; gx += influenceStepSize) {
                for (gy = mapY - radius; gy <= mapY + radius;
                     gy += influenceStepSize) {
                    if (gx < 0 || gx >= kInfluenceGridSize
                        || gy < 0 || gy >= kInfluenceGridSize)
                        continue;
                    distX = gx - mapX;
                    distY = gy - mapY;
                    if (distX < 0)
                        distX *= -1;
                    if (distY < 0)
                        distY *= -1;
                    d2 = distX * distX + distY * distY;
                    if (d2 > r2)
                        continue;

                    if ((govtData[govt].flags2 & kGovt2TightInfluence) != 0)
                        strength = ((r2 - d2) * kTightInfluenceFalloff
                                          * mapZoom);
                    else
                        strength = ((r2 - d2) * kInfluenceFalloff * mapZoom);
                    if (strength < 1)
                        strength = 1;
                    if (strength > 255)
                        strength = 255;

                    placed = 0;
                    for (k = 0; k < kInfluenceSlots; k++) {
                        if (influenceValue[gx][gy][k] != 0) {
                            if (GovtAllies(govt, influenceOwner[gx][gy][k]) != 0) {
                                if ((unsigned char)strength
                                    > influenceValue[gx][gy][k]) {
                                    influenceOwner[gx][gy][k] = govt;
                                    influenceValue[gx][gy][k] =
                                        strength;
                                }
                                placed = 1;
                                break;
                            }
                        }
                    }
                    if (placed == 0) {
                        for (k = 0; k < kInfluenceSlots; k++) {
                            if (influenceValue[gx][gy][k] == 0) {
                                influenceValue[gx][gy][k] = strength;
                                influenceOwner[gx][gy][k] = govt;
                                placed = 1;
                                break;
                            }
                        }
                    }
                    if (placed == 0) {
                        lowest = 0;
                        lowestSlot = 3;
                        for (k = 0; k < kInfluenceSlots; k++) {
                            if (lowest == 0 || influenceValue[gx][gy][k] < lowest) {
                                lowest = influenceValue[gx][gy][k];
                                lowestSlot = k;
                            }
                        }
                        if (lowest != 0) {
                            influenceValue[gx][gy][lowestSlot] =
                                strength;
                            influenceOwner[gx][gy][lowestSlot] = govt;
                        }
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

/* DrawMapInfluence. STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x004aa620
void DrawMapInfluence(Rect bounds)
{
    Rect cell;
    short y;
    short k;
    unsigned char best;
    unsigned char v;
    unsigned char govt;
    RGBColor color;
    short x;

    if (influenceStepSize < 1)
        influenceStepSize = 1;

    for (x = 0; x < kInfluenceGridSize; x += influenceStepSize) {
        for (y = 0; y < kInfluenceGridSize; y += influenceStepSize) {
            if (influenceValue[x][y][0] == 0)
                continue;

            cell.left = x * 2 + bounds.left;
            cell.right = cell.left + influenceStepSize * 2;
            cell.top = y * 2 + bounds.top;
            cell.bottom = cell.top + influenceStepSize * 2;

            if (cell.right < bounds.left)
                continue;
            if (cell.left > bounds.right)
                continue;
            if (cell.bottom < bounds.top)
                continue;
            if (cell.top > bounds.bottom)
                continue;

            if (cell.left < bounds.left)
                cell.left = bounds.left;
            if (cell.right > bounds.right)
                cell.right = bounds.right;
            if (cell.top < bounds.top)
                cell.top = bounds.top;
            if (cell.bottom > bounds.bottom)
                cell.bottom = bounds.bottom;

            best = 0;
            for (k = 0; k < kInfluenceLayers; k++) {
                v = influenceValue[x][y][k];
                if (v > best) {
                    govt = influenceOwner[x][y][k];
                    color.red = (govtData[govt].rgbBytes[0] * (v * 0.5));
                    color.green = (govtData[govt].rgbBytes[1] * (v * 0.5));
                    color.blue = (govtData[govt].rgbBytes[2] * (v * 0.5));
                    best = v;
                }
            }

            RGBForeColor(&color);
            PaintRect(&cell);
        }
    }
    RGBForeColor(blackColor);
}
