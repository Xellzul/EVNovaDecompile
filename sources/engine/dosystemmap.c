// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
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
#include "cefix.h"
#include "space.h"

EXTERN_C_BEGIN

#ifdef MACBUILD
extern void SaveMapBordersState(void);
#endif

/* the mac engine invalidates a dialog item through the caller's own GetDialogItem locals */
#ifdef MACBUILD
#define InvalDialogItem(dlg, n)     (GetDialogItem(dlg, n, &itemType, &item, &invalRect), InvalWindowRect(GetDialogWindow(dlg), &invalRect))
#define invalRect box
static inline void InvalDialogPortRect(DialogPtr dlg, const Rect *r)
{
    InvalWindowRect(GetDialogWindow(dlg), r);
}
#define InvalDialogItemRect(dlg, n) (GetDialogItem(dlg, n, &itemType, &item, &invalRect), InvalDialogPortRect(dlg, &invalRect))
#endif

/* the mac plots the map arrows as colour icons */
#ifdef MACBUILD
#define nv_BlitImageOr(box, icon)   PlotCIcon(box, (CIconHandle)(icon))
#endif

/* mapZoom and floatingMapZoom: one .data run, in address order. */
double mapZoom = 0.5625;                         /* 0x005759d8 (mac name) */
double floatingMapZoom = 0.5625;                 /* 0x005759e0 (see mapZoom above) */


#define kMapDlogID     2000
#define kMapPictID     0x213d
#define kNebulaPictBase 0x251c
#define kShiftScanCode  0x2a
#define kRShiftScanCode 0x36
#define kMacShiftKey    MacKey(0x38)    /* the mac KeyMap's one shift key */

/* DLOG 2000's items; 1 is kItemAccept and 3 kMapPictItem */
#define kMapInfoItem        2   /* the selected system's panel */
#define kMapZoomOutItem     4
#define kMapZoomInItem      5
#define kMapRouteItem       6   /* the destination and route panel */
#define kMapCursorItem      7
#define kMapClearRouteItem  8
#define kMapBordersItem     9
#define kMapSearchItem      10

#ifndef MAC1010
static inline unsigned long long KeyLow60(unsigned long long k)
{
    return KeyLow60Bits(k);
}
#endif

/* DoSystemMap (mac _DoSystemMap). STATE: win MATCH 100.0%, exe 1413 / obj 1413; mac MATCH on both
   slices, byte-identical. OPEN: none; two pads and five spellings carry free-stack counts, see notes. */
// FUNCTION: EVNOVA 0x004a3aa0
// MAC: _DoSystemMap i386 0x000121a3 ppc 0x00015bbc
void DoSystemMap(void)
#ifdef MAC1010
{
    unsigned long long key;
    short routeLen;
    short i;
    short j;
    short prev;
    short y;
    short found;
    short s2;
    short w;
    short h;
    short s3;
    int32_t sy;
    short savedStellar;
    Boolean inRoute;
    Boolean selected;
    Boolean canAdd;
    unsigned char savedShow;
    Boolean done;
    Rect box2;
    Rect r;
    Rect box;
    Handle item;
    MapScroll savedScroll;
    Point mouse;
    Point prevMouse;
    Point anchor;
    KeyCheckFlag bad[4];
    short itemHit;
    short itemType;
    Rect *pr;
    Boolean hyperlinked;

    done = 0;
    if (screenFaded != 0)
        return;
    ShowTheCursor();
    mapDialog = NULL;
    mapDialog = GetNewDialog(kMapDlogID, MapDialogUpdate, MapFilter);
    if (mapDialog == NULL)
        return;
    key = RT3_GetLicenseCode();
    KeyMismatch(bad[0], key, key, 0xcb0b794232805675ULL, 0x0b947b5621514203ULL);
    KeyMismatch(bad[1], key, key, 0xaa30db1ee9c7913aULL, 0x01c38b5f995da87aULL);
    KeyMismatch(bad[2], key, key, 0xe5a3c8161b574a9fULL, 0x065298576bcd73d9ULL);
    KeyMismatch(bad[3], key, key, 0x77c04b34263c359eULL, 0x0ee20eb101b08ac5ULL);
    for (i = 0; i < 4; i++) {
        if (bad[i] == 0) {
            for (j = 0; j < kMaxSystemLinks; j++)
                systemData[shipData->curSystem].links[j] = -1;
            break;
        }
    }
    savedShow = settings.showMapBorders;
    influenceCoordsCached = 0;
    savedStellar = shipData->targetStellar;
    if (mapZoom <= 0.0)
        mapZoom = 1.0;
    CompactHyperRoute();
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
                nebulaPic[i][j] = GetPicture(kNebulaPictBase + i * 7 + j);
        }
    }
    mapCursorSwitchRgn = 0;
    partialMapDialogUpdate = 0;
    for (y = 0; y < 256; y++) {
        for (found = 0; found < 256; found++) {
            for (s3 = 0; s3 < 4; s3++) {
                influenceOwner[found][y][s3] = 0;
                influenceValue[found][y][s3] = 0;
            }
        }
    }
    RecalcMissionSystsForMap();
    if (shipData->navTargetKind == kNavTargetLink && shipData->targetStellar != -1 && onPlanet == 0)
        selSystem = FindActiveCoLocatedSystem(
            systemData[shipData->curSystem].links[shipData->targetStellar]);
    else
        selSystem = shipData->curSystem;
    RedrawNebulaeOffscreen();
    mapBackgroundPic = GetPicture(kMapPictID);
    CenterWindow(FrontWindow(mapDialog), gWind);
    ShowWindowNoFlash(FrontWindow(mapDialog));
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(mapDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(mapDialog);
    MapDialogUpdate();
    SelectWindow(FrontWindow(mapDialog));
    BringToFront(FrontWindow(mapDialog));
    CreateMapTriggerRgn();
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    if (settings.showMapBorders != 0)
        RecalcAllMapInfluence(&box);
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(mapDialog, MapFilter, &itemHit, 0);
        if (itemHit == kItemAccept)
            done = 1;
        if (itemHit == kMapCursorItem) {
            OSCursorReset();
            cursorHidden = 0;
            ShowTheCursor();
        }
#undef invalRect
#define invalRect box2
        if (itemHit == kMapBordersItem) {
            settings.showMapBorders = (settings.showMapBorders == 0);
            InvalDialogItem(mapDialog, kMapBordersItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box2);
        }
        if (itemHit == kMapZoomOutItem && canZoomOut != 0) {
            mapZoom *= 1.333333;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapZoomOutItem);
            InvalDialogItem(mapDialog, kMapZoomInItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            RedrawNebulaeOffscreen();
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        }
        if (itemHit == kMapZoomInItem && canZoomIn != 0) {
            mapZoom *= 0.75;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapZoomOutItem);
            InvalDialogItem(mapDialog, kMapZoomInItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            RedrawNebulaeOffscreen();
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        }
        if (itemHit == kMapClearRouteItem) {
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            hyperRoute[0] = shipData->curSystem;
            routeLen = 1;
            showClearRouteButton = 0;
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            InvalDialogItem(mapDialog, kMapClearRouteItem);
            MapDialogUpdate();
            shipData->targetStellar = -1;
            statusRedraw[kRedrawNav] = 1;
            DrawStatus();
        }
        if (itemHit == kMapSearchItem)
            DoSystemSearch();
        if (itemHit == kMapPictItem) {
            GetMouse(&mouse);
            prevMouse = mouse;
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box2);
            sy = box2.left;
            s3 = box2.top;
            w = box2.right - sy;
            h = box2.bottom - s3;
            found = -1;
            for (i = 0; i < kMaxSystems; i++) {
                if (found != -1)
                    break;
                found = -1;
                if (systemData[i].loaded != 0) {
                    s3 = systemData[i].pos.h;
                    sy = systemData[i].pos.v;
                    r.left = (box2.left + w / 2
                                     + s3 / mapZoom
                                     - mapScroll.h / mapZoom);
                    r.top = (box2.top + h / 2
                                    + sy / mapZoom
                                    - mapScroll.v / mapZoom);
                    r.right = r.left;
                    r.bottom = r.top;
                    InsetRect(&r, -10, -10);
                    if (PtInRectPoint(mouse, &r)) {
                        inRoute = selected = hyperlinked = 0;
                        for (s3 = 0; s3 < 32; s3++) {
                            if (i == missionSyst[s3] && missionSyst[s3] != -1) {
                                inRoute = 1;
                                break;
                            }
                        }
                        if (greenMissionSyst == i)
                            selected = 1;
                        if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                            && hypergateMapModeStellar < kMaxStellars) {
                            for (j = 0; j < kSpobHyperLinks; j++) {
                                if (stellarData[hypergateMapModeStellar].hyperLinks[j] != -1) {
                                    s3 = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[j]].system;
                                    if (s3 < 0 || s3 >= kMaxSystems)
                                        s3 = FindSystemFromStellar(stellarData[hypergateMapModeStellar].hyperLinks[j]);
                                    s3 = FindActiveCoLocatedSystem(s3);
                                    if (s3 == i) {
                                        hyperlinked = 1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (hypergateMapMode != 0 && hyperlinked == 0
                            && i != shipData->curSystem)
                            continue;
                        if (systemData[i].known != 0) {
                            found = i;
                            break;
                        }
                        if (inRoute != 0 || selected != 0 || hyperlinked != 0) {
                            found = i;
                            break;
                        }
                        if (KeyCheck(kMacShiftKey) != 0 && routeLen > 1) {
                            prev = hyperRoute[routeLen - 1];
                            if (systemData[prev].pos.h == systemData[i].pos.h
                                && systemData[prev].pos.v == systemData[i].pos.v) {
                                found = i;
                                break;
                            }
                        } else {
                            prev = shipData->curSystem;
                        }
                        for (j = 0; j < kMaxSystemLinks; j++) {
                            if (systemData[prev].links[j] != -1) {
                                s2 = FindActiveCoLocatedSystem(systemData[prev].links[j]);
                                if (s2 == i) {
                                    found = s2;
                                    break;
                                }
                                if (systemData[s2].pos.h == systemData[i].pos.h
                                    && systemData[s2].pos.v == systemData[i].pos.v) {
                                    found = s2;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (found != -1)
                found = FindActiveCoLocatedSystem(found);
#undef invalRect
#define invalRect r
            if (found != -1) {
                InvalDialogItemRect(mapDialog, kMapInfoItem);
                InvalDialogItemRect(mapDialog, kMapPictItem);
                InvalDialogItemRect(mapDialog, kMapRouteItem);
                if (KeyCheck(kMacShiftKey) != 0 && hypergateMapMode == 0) {
                    CompactHyperRoute();
                    routeLen = -1;
                    if (FindFirstCoLocatedSystem(shipData->curSystem)
                        == FindFirstCoLocatedSystem(found)) {
                        for (j = 0; j < kMaxHyperRoute; j++)
                            hyperRoute[j] = -1;
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
                    canAdd = 0;
                    if (routeLen > 1) {
                        prev = hyperRoute[routeLen - 1];
                        s2 = FindActiveCoLocatedSystem(prev);
                        if (s2 != prev && s2 != -1)
                            prev = s2;
                    } else
                        prev = shipData->curSystem;
                    if (systemData[prev].exploreLevel > 0
                        || systemData[found].known != 0) {
                        for (i = 0; i < kMaxSystemLinks; i++) {
                            if (systemData[prev].links[i] != -1) {
                                s2 = FindActiveCoLocatedSystem(systemData[prev].links[i]);
                                if (s2 == found) {
                                    canAdd = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (routeLen > 0) {
                        s3 = hyperRoute[routeLen - 1];
                        if (s3 >= 0 && s3 < kMaxSystems) {
                            if (systemData[found].pos.h == systemData[s3].pos.h
                                && systemData[found].pos.v == systemData[s3].pos.v) {
                                hyperRoute[routeLen - 1] = -1;
                                routeLen--;
                                canAdd = 0;
                            }
                        }
                    }
                    if (canAdd != 0) {
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
                        InvalDialogItemRect(mapDialog, kMapPictItem);
                        InvalDialogItemRect(mapDialog, kMapRouteItem);
                        InvalDialogItemRect(mapDialog, kMapClearRouteItem);
                        SetGWorldContainer(&loadPort);
                        ForeColor(blackColor);
                        PaintRect(GetDialogPortRect(mapDialog));
                        if (mapBackgroundPic != NULL)
                            DrawPicHandle(mapBackgroundPic, GetDialogPortRect(mapDialog));
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
                                shipData->navTargetKind = kNavTargetGate;
                                shipData->targetStellar = selSystem;
                            } else {
                                shipData->navTargetKind = -1;
                                shipData->targetStellar = -1;
                            }
                        } else {
                            s2 = -1;
                            for (i = 0; i < kMaxSystemLinks; i++) {
                                if (systemData[shipData->curSystem].links[i] != -1) {
                                    s3 = FindActiveCoLocatedSystem(systemData[shipData->curSystem].links[i]);
                                    if (found == s3) {
                                        s2 = i;
                                        break;
                                    }
                                }
                            }
                            if (s2 != -1) {
                                shipData->navTargetKind = kNavTargetLink;
                                shipData->targetStellar = s2;
                                statusRedraw[kRedrawNav] = 1;
                                DrawStatus();
                                SetDialogGWorld(mapDialog);
                            } else {
                                shipData->targetStellar = -1;
                                statusRedraw[kRedrawNav] = 1;
                                DrawStatus();
                                SetDialogGWorld(mapDialog);
                            }
                        }
                    }
                }
            } else {
                if (StillDown()) {
                    GetMouse(&anchor);
                    prevMouse = anchor;
                    savedScroll = mapScroll;
                    while (StillDown()) {
                        BusyIdle();
                        prevMouse = mouse;
                        GetMouse(&mouse);
                        mapScroll.h = (savedScroll.h
                                              - (mouse.h - anchor.h) * mapZoom);
                        mapScroll.v = (savedScroll.v
                                              - (mouse.v - anchor.v) * mapZoom);
                        ScrollMapDisplay(mouse.h - prevMouse.h, mouse.v - prevMouse.v);
                        nv_UpdateDialogWindow(0);
                    }
                    if (settings.showMapBorders != 0)
                        RecalcAllMapInfluence(&box);
                    if (mapScroll.h != savedScroll.h
                        || mapScroll.v != savedScroll.v)
                        MapDialogUpdate();
                }
            }
        }
    } while (done == 0);
    for (i = 0; i < kMaxNebulae; i++) {
        for (j = 0; j < 7; j++) {
            if (nebulaPic[i][j] != NULL) {
                HPurge((Handle)nebulaPic[i][j]);
                QDReleaseResource(nebulaPic[i][j]);
                nebulaPic[i][j] = 0;
            }
        }
    }
    if (mapBackgroundPic != NULL) {
        QDReleaseResource(mapBackgroundPic);
        mapBackgroundPic = NULL;
    }
    w = (GetDialogPortRect(mapDialog)->right - GetDialogPortRect(mapDialog)->left) / 2;
    h = (GetDialogPortRect(mapDialog)->bottom - GetDialogPortRect(mapDialog)->top) / 2;
    if (mapCursorSwitchRgn != NULL)
        DisposeRgn(mapCursorSwitchRgn);
    while (KeyCheck(keyPrefs.map))
        BusyIdle();
    DisposeDialogNoFlash(mapDialog);
    mapDialog = NULL;
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    found = (GetCWindowPortRect((WindowRef)gWind)->right - GetCWindowPortRect((WindowRef)gWind)->left) / 2;
    y = (GetCWindowPortRect((WindowRef)gWind)->bottom - GetCWindowPortRect((WindowRef)gWind)->top) / 2;
    ForeColor(blackColor);
    SetRect(&r, found - w, y - h, found + w, y + h);
    pr = GetCWindowPortRect((WindowRef)gWind);
    box2 = *pr;
    box2.left = box2.right - kTargPanelInset;
    if (SectRect(&r, &box2, &box2))
        RedrawStatus(1);
    RefreshSAT();
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    if (portDialog != NULL && playerIsLanded != 0) {
        shipData->targetStellar = savedStellar;
    } else {
        if (hypergateMapMode == 0)
            CueNextHyperRouteDest();
    }
    if (portDialog == NULL && playerIsLanded == 0
        && settings.showMapBorders != savedShow)
        SavePrefs();
    FlushEvents(7, 0);
}
#else
{
    KeyCheckFlag bad[4];
    unsigned long long key;
    short sys;
    short found;
    short i;
    short j;
    short routeLen;
    int32_t last;
    short prev;
    short s2;
    short s3;
    short prevV;
    short prevH;
    short sy;
    short savedStellar;
    short x;
    short y;
    short w;
    short h;
#if V111 /* V111 spelling */
    short itemType;
    short itemHit;
    Handle item;
    Boolean inRoute;
    Boolean selected;
    Boolean hyperlinked;
    unsigned char savedShow;
    Boolean done;
    Rect r;
    Rect box2;
    Rect box;
    Point mouse;
    Point anchor;
    Point prevMouse;
    MapScroll savedScroll;
#else
    short savedX;
    short savedY;
    unsigned char savedShow;
    short itemType;
    Handle item;
    Boolean done;
    Boolean inRoute;
    Boolean selected;
    Boolean hyperlinked;
    Rect box;
    Rect box2;
    Point mouse;
    short itemHit;
    Rect r;
    Point anchor;
#endif
    Rect *pr;
#if V111
    short (*route)[kMaxHyperRoute] = &hyperRoute; /* V111 pad: 11 fewer into-SSA PHIs, with ms savedScroll's two names */
    MapScroll *ms = &mapScroll; /* V111 pad: 3 fewer into-SSA PHIs, with route savedScroll's two names */
#define hyperRoute (*route)
#endif

    done = 0;
    if (screenFaded != 0)
        return;
    ShowTheCursor();
    mapDialog = NULL;
    mapDialog = GetNewDialog(kMapDlogID, MapDialogUpdate, MapFilter);
    if (mapDialog == NULL)
        return;
    key = RT3_GetLicenseCode();
#ifdef MAC1010
    KeyMismatch(bad[0], key, key, 0xcb0b794232805675ULL, 0x0b947b5621514203ULL);
    KeyMismatch(bad[1], key, key, 0xaa30db1ee9c7913aULL, 0x01c38b5f995da87aULL);
    KeyMismatch(bad[2], key, key, 0xe5a3c8161b574a9fULL, 0x065298576bcd73d9ULL);
    KeyMismatch(bad[3], key, key, 0x77c04b34263c359eULL, 0x0ee20eb101b08ac5ULL);
#else /* 1.0.10 spelling */
    bad[0] = KeyLow60(key ^ 0xcb0b794232805675ULL) != 0x0b947b5621514203ULL;
    bad[1] = KeyLow60(key ^ 0xaa30db1ee9c7913aULL) != 0x01c38b5f995da87aULL;
    bad[2] = KeyLow60(key ^ 0xe5a3c8161b574a9fULL) != 0x065298576bcd73d9ULL;
    bad[3] = KeyLow60(key ^ 0x77c04b34263c359eULL) != 0x0ee20eb101b08ac5ULL;
#endif
    for (x = 0; x < 4; x++) {
        if (bad[x] == 0) {
            for (j = 0; j < kMaxSystemLinks; j++)
                systemData[shipData->curSystem].links[j] = -1;
            break;
        }
    }
    savedShow = settings.showMapBorders;
    influenceCoordsCached = 0;
    savedStellar = shipData->targetStellar;
    if (mapZoom <= 0.0)
        mapZoom = 1.0;
    CompactHyperRoute();
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
                nebulaPic[i][j] = GetPicture(kNebulaPictBase + i * 7 + j);
        }
    }
    mapCursorSwitchRgn = 0;
    partialMapDialogUpdate = 0;
    y = 0;
    do {
#ifdef CEFIX /* F15.01-05 */
        for (sys = 0; sys < kCEBordersWidth; sys++) {
#else
        for (sys = 0; sys < 256; sys++) {
#endif
            for (i = 0; i < 4; i++) {
                influenceOwner[sys][y][i] = 0;
                influenceValue[sys][y][i] = 0;
            }
        }
#ifdef CEFIX /* F15.01-05 */
    } while (++y < kCEBordersHeight);
#else
    } while (++y < 256);
#endif
    RecalcMissionSystsForMap();
    if (shipData->navTargetKind == kNavTargetLink && shipData->targetStellar != -1 && onPlanet == 0)
        selSystem = FindActiveCoLocatedSystem(
            systemData[shipData->curSystem].links[shipData->targetStellar]);
    else
        selSystem = shipData->curSystem;
    RedrawNebulaeOffscreen();
    mapBackgroundPic = GetPicture(kMapPictID);
    CenterWindow(FrontWindow(mapDialog), gWind);
    ShowWindowNoFlash(FrontWindow(mapDialog));
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(mapDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(mapDialog);
    MapDialogUpdate();
    SelectWindow(FrontWindow(mapDialog));
    BringToFront(FrontWindow(mapDialog));
    CreateMapTriggerRgn();
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    if (settings.showMapBorders != 0)
        RecalcAllMapInfluence(&box);
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(mapDialog, MapFilter, &itemHit, 0);
        if (itemHit == kItemAccept)
            done = 1;
        if (itemHit == kMapCursorItem) {
#ifdef MACBUILD
            OSCursorReset();
#endif
            cursorHidden = 0;
            ShowTheCursor();
        }
#ifdef MACBUILD
#undef invalRect
#define invalRect box2
#endif
        if (itemHit == kMapBordersItem) {
            settings.showMapBorders = (settings.showMapBorders == 0);
            InvalDialogItem(mapDialog, kMapBordersItem);
            InvalDialogItem(mapDialog, kMapPictItem);
#ifdef CEFIX /* F16.01 */
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
#else
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box2);
#endif
        }
        if (itemHit == kMapZoomOutItem && canZoomOut != 0) {
            mapZoom = mapZoom * 1.333333;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapZoomOutItem);
            InvalDialogItem(mapDialog, kMapZoomInItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            RedrawNebulaeOffscreen();
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        }
        if (itemHit == kMapZoomInItem && canZoomIn != 0) {
            mapZoom = mapZoom * 0.75;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapZoomOutItem);
            InvalDialogItem(mapDialog, kMapZoomInItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            RedrawNebulaeOffscreen();
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        }
        if (itemHit == kMapClearRouteItem) {
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            hyperRoute[0] = shipData->curSystem;
            routeLen = 1;
            showClearRouteButton = 0;
            InvalDialogItem(mapDialog, kMapPictItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            InvalDialogItem(mapDialog, kMapClearRouteItem);
            MapDialogUpdate();
            shipData->targetStellar = -1;
            statusRedraw[kRedrawNav] = 1;
            DrawStatus();
        }
        if (itemHit == kMapSearchItem)
            DoSystemSearch();
        if (itemHit == kMapPictItem) {
            GetMouse(&mouse);
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box2);
            w = box2.right - box2.left;
            h = box2.bottom - box2.top;
            found = -1;
            for (sys = 0; sys < kMaxSystems; sys++) {
                if (found != -1)
                    break;
                found = -1;
                if (systemData[sys].loaded != 0) {
                    sy = systemData[sys].pos.v;
                    r.left = (box2.left + w / 2
                                     + systemData[sys].pos.h / mapZoom
                                     - mapScroll.h / mapZoom);
                    r.top = (box2.top + h / 2
                                    + sy / mapZoom
                                    - mapScroll.v / mapZoom);
                    r.right = r.left;
                    r.bottom = r.top;
                    InsetRect(&r, -10, -10);
                    if (PtInRectPoint(mouse, &r)) {
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
                                if (stellarData[hypergateMapModeStellar].hyperLinks[i] != -1) {
                                    s3 = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[i]].system;
                                    if (s3 < 0 || s3 >= kMaxSystems)
                                        s3 = FindSystemFromStellar(stellarData[hypergateMapModeStellar].hyperLinks[i]);
                                    s3 = FindActiveCoLocatedSystem(s3);
                                    if (s3 == sys) {
                                        hyperlinked = 1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (hypergateMapMode != 0 && hyperlinked == 0
                            && sys != shipData->curSystem)
                            continue;
                        if (systemData[sys].known != 0) {
                            found = sys;
                            break;
                        }
                        if (inRoute != 0 || selected != 0 || hyperlinked != 0) {
                            found = sys;
                            break;
                        }
#ifdef MACBUILD
                        if (KeyCheck(kMacShiftKey) != 0 && routeLen > 1) {
                            prev = hyperRoute[routeLen - 1];
                            if (systemData[prev].pos.h == systemData[sys].pos.h
                                && systemData[prev].pos.v == systemData[sys].pos.v) {
                                found = sys;
                                break;
                            }
                        } else {
                            prev = shipData->curSystem;
                        }
#else
                        if ((KeyCheck(kShiftScanCode) == 0
                             && KeyCheck(kRShiftScanCode) == 0)
                            || routeLen <= 1) {
                            prev = shipData->curSystem;
                        } else {
                            prev = hyperRoute[routeLen - 1];
                            if (systemData[prev].pos.h == systemData[sys].pos.h
                                && systemData[prev].pos.v == systemData[sys].pos.v) {
                                found = sys;
                                break;
                            }
                        }
#endif
                        for (i = 0; i < kMaxSystemLinks; i++) {
                            if (systemData[prev].links[i] != -1) {
                                s2 = FindActiveCoLocatedSystem(systemData[prev].links[i]);
                                if (s2 == sys) {
                                    found = s2;
                                    break;
                                }
                                if (systemData[s2].pos.h == systemData[sys].pos.h
                                    && systemData[s2].pos.v == systemData[sys].pos.v) {
                                    found = s2;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (found != -1)
                found = FindActiveCoLocatedSystem(found);
#ifdef MACBUILD
#undef invalRect
#define invalRect r
#endif
            if (found != -1) {
#ifdef MACBUILD
                InvalDialogItemRect(mapDialog, kMapInfoItem);
                InvalDialogItemRect(mapDialog, kMapPictItem);
                InvalDialogItemRect(mapDialog, kMapRouteItem);
#else
                InvalDialogItem(mapDialog, kMapInfoItem);
                InvalDialogItem(mapDialog, kMapPictItem);
                InvalDialogItem(mapDialog, kMapRouteItem);
#endif
#ifdef MACBUILD
                if (KeyCheck(kMacShiftKey) != 0 && hypergateMapMode == 0) {
#else
                if ((KeyCheck(kShiftScanCode) != 0
                     || KeyCheck(kRShiftScanCode) != 0)
                    && hypergateMapMode == 0) {
#endif
                    CompactHyperRoute();
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
                    if (systemData[prev].exploreLevel > 0
                        || systemData[found].known != 0) {
                        for (i = 0; i < kMaxSystemLinks; i++) {
                            if (systemData[prev].links[i] != -1) {
                                s2 = FindActiveCoLocatedSystem(systemData[prev].links[i]);
                                if (s2 == found) {
                                    inRoute = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (routeLen > 0) {
#if V111
                        s2 = hyperRoute[routeLen - 1];
#else
                        last = routeLen - 1;
                        s2 = hyperRoute[last];
#endif
                        if (s2 >= 0 && s2 < kMaxSystems) {
#if V111
                            if (systemData[s2].pos.h == systemData[found].pos.h
                                && systemData[s2].pos.v == systemData[found].pos.v) {
                                hyperRoute[routeLen - 1] = -1;
#else
                            if (systemData[found].pos.h == systemData[s2].pos.h
                                && systemData[found].pos.v == systemData[s2].pos.v) {
                                hyperRoute[last] = -1;
#endif
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
#ifdef MACBUILD
                        InvalDialogItemRect(mapDialog, kMapPictItem);
                        InvalDialogItemRect(mapDialog, kMapRouteItem);
                        InvalDialogItemRect(mapDialog, kMapClearRouteItem);
#else
                        InvalDialogItem(mapDialog, kMapPictItem);
                        InvalDialogItem(mapDialog, kMapRouteItem);
                        InvalDialogItem(mapDialog, kMapClearRouteItem);
#endif
                        SetGWorldContainer(&loadPort);
                        ForeColor(blackColor);
                        PaintRect(GetDialogPortRect(mapDialog));
                        if (mapBackgroundPic != NULL)
                            DrawPicHandle(mapBackgroundPic, GetDialogPortRect(mapDialog));
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
                                shipData->navTargetKind = kNavTargetGate;
                                shipData->targetStellar = selSystem;
                            } else {
                                shipData->navTargetKind = -1;
                                shipData->targetStellar = -1;
                            }
                        } else {
                            s2 = -1;
                            for (i = 0; i < kMaxSystemLinks; i++) {
                                if (systemData[shipData->curSystem].links[i] != -1) {
                                    if (found == FindActiveCoLocatedSystem(systemData[shipData->curSystem].links[i])) {
                                        s2 = i;
                                        break;
                                    }
                                }
                            }
                            if (s2 != -1) {
                                shipData->navTargetKind = kNavTargetLink;
                                shipData->targetStellar = s2;
                                statusRedraw[kRedrawNav] = 1;
                                DrawStatus();
                                SetDialogGWorld(mapDialog);
                            } else {
                                shipData->targetStellar = -1;
                                statusRedraw[kRedrawNav] = 1;
                                DrawStatus();
                                SetDialogGWorld(mapDialog);
                            }
                        }
                    }
                }
            } else {
                if (StillDown()) {
                    GetMouse(&anchor);
#if V111 /* V111 spelling */
                    prevMouse = anchor;
                    savedScroll = mapScroll;
#else
                    savedX = mapScroll.h;
                    savedY = mapScroll.v;
#endif
                    while (StillDown()) {
                        BusyIdle();
#if V111
                        prevMouse = mouse;
                        GetMouse(&mouse);
                        ms->h = (savedScroll.h
                                              - (mouse.h - anchor.h) * mapZoom);
                        ms->v = (savedScroll.v
                                              - (mouse.v - anchor.v) * mapZoom);
                        ScrollMapDisplay(mouse.h - prevMouse.h, mouse.v - prevMouse.v);
#else
                        prevV = mouse.v;
                        prevH = mouse.h;
                        GetMouse(&mouse);
                        mapScroll.h = (savedX
                                              - (mouse.h - anchor.h) * mapZoom);
                        mapScroll.v = (savedY
                                              - (mouse.v - anchor.v) * mapZoom);
                        ScrollMapDisplay(mouse.h - prevH, mouse.v - prevV);
#endif
                        nv_UpdateDialogWindow(0);
                    }
                    if (settings.showMapBorders != 0)
                        RecalcAllMapInfluence(&box);
#if V111
                    if (mapScroll.h != savedScroll.h
                        || mapScroll.v != savedScroll.v)
#else
                    if (mapScroll.h != savedX
                        || mapScroll.v != savedY)
#endif
                        MapDialogUpdate();
                }
            }
        }
    } while (done == 0);
    for (x = 0; x < kMaxNebulae; x++) {
        for (j = 0; j < 7; j++) {
            if (nebulaPic[x][j] != NULL) {
#ifdef MACBUILD
                HPurge((Handle)nebulaPic[x][j]);
#endif
                QDReleaseResource(nebulaPic[x][j]);
                nebulaPic[x][j] = 0;
            }
        }
    }
    if (mapBackgroundPic != NULL) {
        QDReleaseResource(mapBackgroundPic);
        mapBackgroundPic = NULL;
    }
    x = (GetDialogPortRect(mapDialog)->right - GetDialogPortRect(mapDialog)->left) / 2;
    y = (GetDialogPortRect(mapDialog)->bottom - GetDialogPortRect(mapDialog)->top) / 2;
#ifdef MACBUILD
    if (mapCursorSwitchRgn != NULL)
        DisposeRgn(mapCursorSwitchRgn);
#endif
    while (KeyCheck(keyPrefs.map))
        BusyIdle();
    DisposeDialogNoFlash(mapDialog);
    mapDialog = NULL;
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
    found = (GetCWindowPortRect(gWind)->right - GetCWindowPortRect(gWind)->left) / 2;
    sys = (GetCWindowPortRect(gWind)->bottom - GetCWindowPortRect(gWind)->top) / 2;
    ForeColor(blackColor);
    SetRect(&r, found - x, sys - y, found + x, sys + y);
    pr = GetCWindowPortRect(gWind);
#if V111 /* V111 spelling */
    box2 = *pr;
#else
    box2.top = pr->top;
    box2.left = pr->left;
    box2.bottom = pr->bottom;
    box2.right = pr->right;
#endif
#ifdef CEFIX /* F14.17 */
    box2.left = box2.right - ceStatusBarWidth;
#else
    box2.left = box2.right - kTargPanelInset;
#endif
    if (SectRect(&r, &box2, &box2))
        RedrawStatus(1);
#ifdef MACBUILD
    RefreshSAT();
#else
    RedrawDialogs(1);
#endif
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    if (portDialog != NULL && playerIsLanded != 0) {
        shipData->targetStellar = savedStellar;
    } else {
        if (hypergateMapMode == 0)
            CueNextHyperRouteDest();
    }
    if (portDialog == NULL && playerIsLanded == 0
        && settings.showMapBorders != savedShow)
#if V111
#ifdef MACBUILD
        SaveMapBordersState();
#else
        SavePrefs();
#endif
#else
        SavePrefs();
#endif
    FlushEvents(7, 0);
#if V111
#undef hyperRoute
#endif
}
#endif

/* CreateMapTriggerRgn. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a51e0
// MAC: _CreateMapTriggerRgn i386 0x0000e2d7 ppc 0x00011a98
void CreateMapTriggerRgn(void)
{
}


                                                 /* 0x004cd1a0 (mac _StrcatIndCString) */
                                                 /* 0x004b8df0 */
                                                 /* 0x00468210 */

static inline void MapPoint(Rect *r, float y, float x, Rect *b, short w, short h, int32_t sx, short sy)
{
    r->left = (b->left + w / 2 + x / mapZoom - sx / mapZoom);
    r->top = (b->top + h / 2 + y / mapZoom - sy / mapZoom);
}

/* MapDialogUpdate. STATE: MATCH 100.0%, 2173/2173. `w / 2' and `h / 2' are written at
   each use (DrawMap's spelling); the service count is a later web of the long `temp'.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004a51f0
// MAC: _MapDialogUpdate i386 0x0000f5d1 ppc 0x00012fe8
void MapDialogUpdate(void)
#ifdef MAC1010
{
    unsigned char buf[256];
    Rect r2;
    Handle item;
    short i;
    short fit;
    short xoff;
    short j;
    short savedY;
    short sel;
    short itemType;
    Rect box;
    Rect box6;
    Rect nebR;
    unsigned char text[256];
    short count;
    short found;
    Boolean ok;
    Boolean linked;
    short w, h;
    int32_t temp;
    short st;
    short sw;

    for (i = 0; i < kMaxSystems; i++)
        doneFlag[i] = 0;
    temp = mapScroll.h;
    savedY = mapScroll.v;
    SetGWorldContainer(&loadPort);
    PenSize(1, 1);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(mapDialog));
    if (partialMapDialogUpdate == 0 && mapBackgroundPic != NULL)
        DrawPicHandle(mapBackgroundPic, GetDialogPortRect(mapDialog));
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
        ok = RectInRgn(&box, GetWindowVisRgn(FrontWindow(mapDialog)));
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
        ForeColor(blackColor);
        PaintRect(&box);
        if (partialMapDialogUpdate == 0 && settings.showMapBorders != 0) {
            SWSetPortToWorkArea(gSpriteWorld);
            GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &r2);
            DrawMapInfluence(r2);
        }
        TextFont(smallTextFont);
        TextSize(smallTextSize - 2);
        if (mapZoom >= 1.1)
            TextSize(smallTextSize - 2);
        else if (mapZoom >= 0.9)
            TextSize(smallTextSize);
        else
            TextSize(smallTextSize);
        for (i = 0; i < kMaxNebulae; i++) {
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
                        if (nebulaPic[i][j] != 0) {
                            QDGetPictureBounds(nebulaPic[i][j], &r2);
                            if (r2.right - r2.left <= nebR.right - nebR.left
                                && r2.bottom - r2.top <= nebR.bottom - nebR.top)
                                fit = j;
                        }
                    }
                    if (nebulaPic[i][fit] != 0) {
                        SectRect(&box, &nebR, &r2);
                        if (partialMapDialogUpdate != 0) {
                            SWSetPortToWorkArea(gSpriteWorld);
                            ForeColor(blackColor);
                            DrawPicHandle(nebulaPic[i][fit], &nebR);
                            SetGWorldContainer(&loadPort);
                            ForeColor(blackColor);
                            CopyBits(GetPortPixMapBits(
                                         gSpriteWorld->workFrameP->framePort),
                                     GWorldContainerBits(&loadPort),
                                     &r2, &r2, 0, 0);
                        } else {
                            SWSetPortToBackground(gSpriteWorld);
                            ForeColor(blackColor);
                            DrawPicHandle(nebulaPic[i][fit], &nebR);
                            CopyBits(GetPortPixMapBits(
                                         gSpriteWorld->backFrameP->framePort),
                                     GetPortPixMapBits(
                                         gSpriteWorld->workFrameP->framePort),
                                     &r2, &r2, 0x25, 0);
                            SWSetPortToWorkArea(gSpriteWorld);
                            ForeColor(blackColor);
                        }
                    }
                }
            }
        }
        DrawMap(box);
        if (selSystem != -1) {
            MapPoint(&r2, systemData[selSystem].pos.v, systemData[selSystem].pos.h, &box, w, h, temp, savedY);
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
            ForeColor(blackColor);
        }
        ForeColor(whiteColor);
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
                && (mapZoom <= 1.1 || i == selSystem)) {
                MoveTo((box.left + w / 2 + systemData[i].pos.h / mapZoom
                               - temp / mapZoom + 7.0),
                       (box.top + h / 2 + systemData[i].pos.v / mapZoom
                               - savedY / mapZoom + 4.0));
                DrawString(systemData[i].name);
            }
        }
        ForeColor(blackColor);
        didUpdateMap = 1;
        if (partialMapDialogUpdate != 0) {
            SWSetPortToWindow(gSpriteWorld);
            SetDialogGWorld(mapDialog);
            ForeColor(blackColor);
            CopyBits(GetPortPixMapBits(loadPort.world),
                     GetPortBitMapForCopyBits(GetDialogPort(mapDialog)),
                     &box, &box, 0, 0);
        } else {
            SetGWorldContainer(&loadPort);
            ForeColor(blackColor);
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     GWorldContainerBits(&loadPort),
                     &box, &box, 0, 0);
        }
    }
    if (partialMapDialogUpdate == 0) {
        GetDialogItem(mapDialog, kMapInfoItem,&itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(mapDialog)))) {
        GetDialogItem(mapDialog, kMapRouteItem,&itemType, &item, &box6);
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
            ForeColor(whiteColor);
            MoveTo(box.left + 0x2d, box.top + 0xc);
            if (systemData[sel].exploreLevel > 0) {
                count = 0;
                xoff = 0;
                j = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[sel].navDefault[i] != -1
                        && (stellarData[systemData[sel].navDefault[i]].flags & kSpobUninhabited) == 0
                        && StellarIsLandable(&stellarData[systemData[sel].navDefault[i]]) != 0)
                        count++;
                }
                if (count > 0) {
                    for (i = 0; i < kNavSlots; i++) {
                        if (systemData[sel].navDefault[i] != -1
                            && (stellarData[systemData[sel].navDefault[i]].flags & kSpobUninhabited) == 0
                            && StellarIsLandable(&stellarData[systemData[sel].navDefault[i]]) != 0) {
                            sw = StringWidth(stellarData[systemData[sel].navDefault[i]].name);
                            if (box.left + 0x2d + sw + xoff > box6.right - 0x14) {
                                MoveTo(box.left + 0x2d, box.top + 0x18);
                                xoff = 0;
                            } else {
                                xoff += sw;
                            }
                            DrawString(stellarData[systemData[sel].navDefault[i]].name);
                            j++;
                            if (count > j) {
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
                    NumToString(sel + kFirstResourceID, buf);
                    DrawString(buf);
                } else {
                    DrawIndString(kGameStrList, 0x136);
                }
            }
            RGBForeColor(&rgbLightGrey);
            MoveTo(box.left + 10, box.top + 0x24);
            DrawIndString(kGameStrList, 0x137);
            ForeColor(whiteColor);
            MoveTo(box.left + 0x6e, box.top + 0x24);
            if (systemData[sel].exploreLevel > 0) {
                strcpy((char *)buf, "");
                for (j = 0; j < kNavSlots; j++) {
                    if (systemData[sel].navDefault[j] != -1
                        && stellarData[systemData[sel].navDefault[j]].gravity != 0) {
                        StrcatIndCString((char *)buf, kGameStrList, 0x138);
                        if (systemData[sel].asteroidCount > 0
                            || systemData[sel].interference > 0
                            || systemData[sel].murk > 0)
                            strcat((char *)buf, ", ");
                        break;
                    }
                }
                if (systemData[sel].asteroidCount > 0) {
                    if (systemData[sel].asteroidCount <= 3)
                        StrcatIndCString((char *)buf, kGameStrList, 0x139);
                    else if (systemData[sel].asteroidCount <= 6)
                        StrcatIndCString((char *)buf, kGameStrList, 0x13a);
                    else
                        StrcatIndCString((char *)buf, kGameStrList, 0x13b);
                    strcat((char *)buf, " ");
                    StrcatIndCString((char *)buf, kGameStrList, 0x13c);
                    if (systemData[sel].interference > 0
                        || systemData[sel].murk > 0)
                        strcat((char *)buf, ", ");
                }
                if (systemData[sel].interference > 0) {
                    if (systemData[sel].interference <= 0x21)
                        StrcatIndCString((char *)buf, kGameStrList, 0x13d);
                    else if (systemData[sel].interference <= 0x42)
                        StrcatIndCString((char *)buf, kGameStrList, 0x13e);
                    else
                        StrcatIndCString((char *)buf, kGameStrList, 0x13f);
                    strcat((char *)buf, " ");
                    StrcatIndCString((char *)buf, kGameStrList, 0x140);
                    if (systemData[sel].murk > 0)
                        strcat((char *)buf, ", ");
                }
                if (systemData[sel].murk > 0) {
                    if (systemData[sel].murk <= 0x1e)
                        StrcatIndCString((char *)buf, kGameStrList, 0x141);
                    else if (systemData[sel].murk <= 0x3c)
                        StrcatIndCString((char *)buf, kGameStrList, 0x142);
                    else
                        StrcatIndCString((char *)buf, kGameStrList, 0x143);
                    strcat((char *)buf, " ");
                    StrcatIndCString((char *)buf, kGameStrList, 0x144);
                }
                if (buf[0] == 0)
                    StrcatIndCString((char *)buf, kGameStrList, 0x14f);
                buf[0] = toupper(buf[0]);
                CtoPstr((char *)buf);
                DrawString(buf);
            } else {
                if (cheatsActive != 0) {
                    NumToString(sel + kFirstResourceID, buf);
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
        ForeColor(blackColor);
        didUpdateMap = 1;
        }
        GetDialogItem(mapDialog, kMapRouteItem,&itemType, &item, &r2);
        if (RectInRgn(&r2, GetWindowVisRgn(FrontWindow(mapDialog)))) {
        ForeColor(blackColor);
        MoveTo(r2.left + 1, r2.bottom - 1);
        LineTo(r2.right - 2, r2.bottom - 1);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(r2.left + 10, r2.top + 0xc);
        RGBForeColor(&rgbLightGrey);
        linked = 0;
        if (shipData->navTargetKind == kNavTargetLink) {
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
                && hypergateMapModeStellar < kMaxStellars) {
                for (j = 0; j < kSpobHyperLinks; j++) {
                    /* BUG: i is the finished name loop's counter (0x800), so an empty link is never skipped */
                    if (-i != stellarData[hypergateMapModeStellar].hyperLinks[j]) {
                        count = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[j]].system;
                        if (count < 0 || count >= kMaxSystems)
                            count = FindSystemFromStellar(stellarData[hypergateMapModeStellar].hyperLinks[j]);
                        found = FindActiveCoLocatedSystem(count);
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
        ForeColor(whiteColor);
        if (systemData[sel].exploreLevel <= 0) {
            if (cheatsActive != 0) {
                NumToString(sel + kFirstResourceID, buf);
                DrawString(buf);
            } else {
                DrawIndString(kGameStrList, 0x136);
            }
        } else {
            if (cheatsActive != 0) {
                NumToString(sel + kFirstResourceID, buf);
                DrawString(buf);
            } else {
                DrawString(systemData[sel].name);
            }
            if (SystemIsInhabited(sel) != 0) {
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0x4b);
                DrawIndString(kGameStrList, 0x145);
                ForeColor(whiteColor);
                MoveTo(r2.left + 0xf, r2.top + 0x58);
                count = systemData[sel].govt;
                if (count == -1) {
                    DrawIndString(kGameStrList, 0x14d);
                } else if (cheatsActive != 0) {
                    NumToString(count + kFirstResourceID, buf);
                    DrawString("\pID ");
                    DrawString(buf);
                } else {
                    DrawString(govtData[count].name);
                }
                MoveTo(r2.left + 10, r2.top + 0x77);
                RGBForeColor(&rgbLightGrey);
                DrawIndString(kGameStrList, 0x146);
                ForeColor(whiteColor);
                MoveTo(r2.left + 0xf, r2.top + 0x84);
                if (cheatsActive != 0) {
                    NumToString(playerRecord[sel], buf);
                    DrawString(buf);
                } else {
                    DrawLegalStatusString(sel);
                }
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0x9b);
                DrawIndString(kGameStrList, 0x147);
                ForeColor(whiteColor);
                if (systemData[sel].exploreLevel < 2) {
                    MoveTo(r2.left + 0xf, r2.top + 0xa8);
                    DrawIndString(kGameStrList, 0x136);
                } else {
                    j = 0;
                    for (i = 0; i <= 5; i++) {
                        if (SystemTradesGoods(sel, i) != 0) {
                            MoveTo(r2.left + 0xf, r2.top + 0xa8 + j);
                            DrawString(cargoName[i]);
                            j += 0xc;
                        }
                    }
                    if (j == 0) {
                        MoveTo(r2.left + 0xf, r2.top + 0xa8);
                        DrawIndString(kGameStrList, 0x14f);
                    }
                }
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0xfa);
                DrawIndString(kGameStrList, 0x148);
                temp = 0;
                for (i = 0; i <= 2; i++) {
                    if (SystemHasService(sel, i) != 0)
                        temp++;
                }
                if (temp > 0) {
                    ForeColor(whiteColor);
                    if (systemData[sel].exploreLevel < 2) {
                        MoveTo(r2.left + 0xf, r2.top + 0x107);
                        DrawIndString(kGameStrList, 0x136);
                    } else {
                        j = 0;
                        for (i = 0; i <= 2; i++) {
                            if (SystemHasService(sel, i) != 0) {
                                MoveTo(r2.left + 0xf, r2.top + 0x107 + j);
                                DrawIndString(kGameStrList, i + 0x149);
                                j += 0xc;
                            }
                        }
                    }
                } else {
                    if (systemData[sel].exploreLevel >= 2) {
                        MoveTo(r2.left + 0xf, r2.top + 0x106);
                        ForeColor(whiteColor);
                        DrawIndString(kGameStrList, 0x14c);
                    } else if (systemData[sel].exploreLevel >= 1) {
                        MoveTo(r2.left + 0xf, r2.top + 0x106);
                        ForeColor(whiteColor);
                        DrawIndString(kGameStrList, 0x136);
                    }
                }
            } else {
                MoveTo(r2.left + 10, r2.top + 0x4b);
                ForeColor(whiteColor);
                DrawIndString(kGameStrList, 0x14e);
            }
        }
        didUpdateMap = 1;
        }
        SetGWorldContainer(&loadPort);
        DrawMapButtons(-1);
        UnsetGWorldContainer(&loadPort);
        SWSetPortToWindow(gSpriteWorld);
        SetDialogGWorld(mapDialog);
        ForeColor(blackColor);
        CopyBits(GetPortPixMapBits(loadPort.world),
                 GetPortBitMapForCopyBits(GetDialogPort(mapDialog)),
                 GetDialogPortRect(mapDialog), GetDialogPortRect(mapDialog),
                 0, 0);
    }
}
#else
{
    unsigned char text[256];
    Rect r2;
    Handle item;
    short i;
#if V111 /* V111 spelling */
    short drawn;
    short fit;
    short xoff;
#endif
    short savedY;
    short sel;
    int32_t temp;
    short itemType;
    Rect box;
    Rect nebR;
    unsigned char buf[256];
    Rect box6;
    short j;
    short count;
#if V111 /* V111 spelling */
#else
    short drawn;
    short fit;
#endif
    short st2;
    short found;
    Boolean ok;
    Boolean linked;
    float sy;
    short w, h;
    unsigned char *name;
#if V111 /* V111 spelling */
#else
    short xoff;
#endif
    short k;
    short st;
    short n;
    short sw;

    for (i = 0; i < kMaxSystems; i++)
        doneFlag[i] = 0;
    temp = mapScroll.h;
    savedY = mapScroll.v;
    SetGWorldContainer(&loadPort);
    PenSize(1, 1);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(mapDialog));
    if (partialMapDialogUpdate == 0 && mapBackgroundPic != NULL)
        DrawPicHandle(mapBackgroundPic, GetDialogPortRect(mapDialog));
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
        ok = RectInRgn(&box, GetWindowVisRgn(FrontWindow(mapDialog)));
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
        ForeColor(blackColor);
        PaintRect(&box);
        if (partialMapDialogUpdate == 0 && settings.showMapBorders != 0) {
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
        for (i = 0; i < kMaxNebulae; i++) {
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
#ifdef CEFIX /* F17.01-02 */
                    for (j = 0; j < 7; j++) {
                        if (nebulaPic[i][j] != 0) {
                            QDGetPictureBounds(nebulaPic[i][j], &r2);
                            fit = j;
                            if (r2.right - r2.left >= nebR.right - nebR.left)
                                break;
                            /* CE bug: CE's height jge (7d 0d) lands inside a movsx, on out dx,eax:
                               a fault when a picture is narrower than the rect but as tall. */
                            if (r2.bottom - r2.top >= nebR.bottom - nebR.top)
                                RaiseException(kCEPrivilegedInstruction, 0, 0, NULL);
                        }
                    }
#else
                    for (j = 0; j < 7; j++) {
                        if (nebulaPic[i][j] != 0) {
                            QDGetPictureBounds(nebulaPic[i][j], &r2);
                            if (r2.right - r2.left <= nebR.right - nebR.left
                                && r2.bottom - r2.top <= nebR.bottom - nebR.top)
                                fit = j;
                        }
                    }
#endif
                    if (nebulaPic[i][fit] != 0) {
                        SectRect(&box, &nebR, &r2);
                        if (partialMapDialogUpdate != 0) {
                            SWSetPortToWorkArea(gSpriteWorld);
                            ForeColor(blackColor);
                            DrawPicHandle(nebulaPic[i][fit], &nebR);
                            SetGWorldContainer(&loadPort);
                            ForeColor(blackColor);
                            CopyBits(GetPortPixMapBits(
                                         gSpriteWorld->workFrameP->framePort),
                                     (BitMapPtr)GWorldContainerBits(&loadPort),
                                     &r2, &r2, 0, 0);
                        } else {
                            SWSetPortToBackground(gSpriteWorld);
                            ForeColor(blackColor);
                            DrawPicHandle(nebulaPic[i][fit], &nebR);
                            CopyBits(GetPortPixMapBits(
                                         gSpriteWorld->backFrameP->framePort),
                                     GetPortPixMapBits(
                                         gSpriteWorld->workFrameP->framePort),
                                     &r2, &r2, 0x25, 0);
                            SWSetPortToWorkArea(gSpriteWorld);
                            ForeColor(blackColor);
                        }
                    }
                }
            }
        }
        DrawMap(box);
        if (selSystem != -1) {
            sy = systemData[selSystem].pos.v;
#ifdef MACBUILD
            r2.left = (box.left + w / 2
                              + (float)systemData[selSystem].pos.h / mapZoom
                              - temp / mapZoom);
#else
            r2.left = (box.left + w / 2
                              + systemData[selSystem].pos.h / mapZoom
                              - temp / mapZoom);
#endif
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
            ForeColor(blackColor);
        }
        ForeColor(whiteColor);
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
                && (mapZoom <= 1.1 || i == selSystem)) {
                MoveTo((box.left + w / 2 + systemData[i].pos.h / mapZoom
                               - temp / mapZoom + 7.0),
                       (box.top + h / 2 + systemData[i].pos.v / mapZoom
                               - savedY / mapZoom + 4.0));
                DrawString(systemData[i].name);
            }
        }
        ForeColor(blackColor);
        didUpdateMap = 1;
        if (partialMapDialogUpdate != 0) {
            SWSetPortToWindow(gSpriteWorld);
            SetDialogGWorld(mapDialog);
            ForeColor(blackColor);
            CopyBits(GetPortPixMapBits(loadPort.world),
                     GetPortBitMapForCopyBits(GetDialogPort(mapDialog)),
                     &box, &box, 0, 0);
        } else {
            SetGWorldContainer(&loadPort);
            ForeColor(blackColor);
            CopyBits(GetPortPixMapBits(
                         gSpriteWorld->workFrameP->framePort),
                     (BitMapPtr)GWorldContainerBits(&loadPort),
                     &box, &box, 0, 0);
        }
    }
    if (partialMapDialogUpdate == 0) {
        GetDialogItem(mapDialog, kMapInfoItem,&itemType, &item, &box);
#ifdef MACBUILD
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(mapDialog)))) {
#endif
        GetDialogItem(mapDialog, kMapRouteItem,&itemType, &item, &box6);
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
            ForeColor(whiteColor);
            MoveTo(box.left + 0x2d, box.top + 0xc);
            if (systemData[sel].exploreLevel > 0) {
#if V111 /* V111 spelling */
#else
                short xoff;
#endif
                count = 0;
                drawn = 0;
                xoff = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[sel].navDefault[i] != -1
                        && (stellarData[systemData[sel].navDefault[i]].flags & kSpobUninhabited) == 0
                        && StellarIsLandable(&stellarData[systemData[sel].navDefault[i]]) != 0)
                        count++;
                }
                if (count > 0) {
                    for (i = 0; i < kNavSlots; i++) {
                        if (systemData[sel].navDefault[i] != -1
                            && (stellarData[systemData[sel].navDefault[i]].flags & kSpobUninhabited) == 0
                            && StellarIsLandable(&stellarData[systemData[sel].navDefault[i]]) != 0) {
                            sw = StringWidth(stellarData[systemData[sel].navDefault[i]].name);
                            if (box.left + 0x2d + sw + xoff > box6.right - 0x14) {
                                MoveTo(box.left + 0x2d, box.top + 0x18);
                                xoff = 0;
                            } else {
                                xoff += sw;
                            }
                            DrawString(stellarData[systemData[sel].navDefault[i]].name);
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
                    NumToString(sel + kFirstResourceID, buf);
                    DrawString(buf);
                } else {
                    DrawIndString(kGameStrList, 0x136);
                }
            }
            RGBForeColor(&rgbLightGrey);
            MoveTo(box.left + 10, box.top + 0x24);
            DrawIndString(kGameStrList, 0x137);
            ForeColor(whiteColor);
            MoveTo(box.left + 0x6e, box.top + 0x24);
            if (systemData[sel].exploreLevel > 0) {
                strcpy((char *)buf, "");
                for (j = 0; j < kNavSlots; j++) {
                    if (systemData[sel].navDefault[j] != -1
                        && stellarData[systemData[sel].navDefault[j]].gravity != 0) {
                        StrcatIndCString((char *)buf, kGameStrList, 0x138);
                        if (systemData[sel].asteroidCount > 0
                            || systemData[sel].interference > 0
                            || systemData[sel].murk > 0)
                            strcat((char *)buf, ", ");
                        break;
                    }
                }
                if (systemData[sel].asteroidCount > 0) {
                    if (systemData[sel].asteroidCount <= 3)
                        StrcatIndCString((char *)buf, kGameStrList, 0x139);
                    else if (systemData[sel].asteroidCount <= 6)
                        StrcatIndCString((char *)buf, kGameStrList, 0x13a);
                    else
                        StrcatIndCString((char *)buf, kGameStrList, 0x13b);
                    strcat((char *)buf, " ");
                    StrcatIndCString((char *)buf, kGameStrList, 0x13c);
                    if (systemData[sel].interference > 0
                        || systemData[sel].murk > 0)
                        strcat((char *)buf, ", ");
                }
                if (systemData[sel].interference > 0) {
                    if (systemData[sel].interference <= 0x21)
                        StrcatIndCString((char *)buf, kGameStrList, 0x13d);
                    else if (systemData[sel].interference <= 0x42)
                        StrcatIndCString((char *)buf, kGameStrList, 0x13e);
                    else
                        StrcatIndCString((char *)buf, kGameStrList, 0x13f);
                    strcat((char *)buf, " ");
                    StrcatIndCString((char *)buf, kGameStrList, 0x140);
                    if (systemData[sel].murk > 0)
                        strcat((char *)buf, ", ");
                }
                if (systemData[sel].murk > 0) {
                    if (systemData[sel].murk <= 0x1e)
                        StrcatIndCString((char *)buf, kGameStrList, 0x141);
                    else if (systemData[sel].murk <= 0x3c)
                        StrcatIndCString((char *)buf, kGameStrList, 0x142);
                    else
                        StrcatIndCString((char *)buf, kGameStrList, 0x143);
                    strcat((char *)buf, " ");
                    StrcatIndCString((char *)buf, kGameStrList, 0x144);
                }
                if ((int32_t)buf[0] == 0)
                    StrcatIndCString((char *)buf, kGameStrList, 0x14f);
                buf[0] = toupper(buf[0]);
                CtoPstr((char *)buf);
                DrawString(buf);
            } else {
                if (cheatsActive != 0) {
                    NumToString(sel + kFirstResourceID, buf);
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
#ifdef CEFIX /* F12.78 */
        CE_SetPen(box.right - StringWidth(buf) + CE_Scale(-5), box.top + CE_Scale(0x24));
#else
        MoveTo(box.right - StringWidth(buf) - 5, box.top + 0x24);
#endif
        DrawString(buf);
        ForeColor(blackColor);
        didUpdateMap = 1;
#ifdef MACBUILD
        }
#endif
        GetDialogItem(mapDialog, kMapRouteItem,&itemType, &item, &r2);
#ifdef MACBUILD
        if (RectInRgn(&r2, GetWindowVisRgn(FrontWindow(mapDialog)))) {
#endif
        ForeColor(blackColor);
        MoveTo(r2.left + 1, r2.bottom - 1);
        LineTo(r2.right - 2, r2.bottom - 1);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        MoveTo(r2.left + 10, r2.top + 0xc);
        RGBForeColor(&rgbLightGrey);
        linked = 0;
        if (shipData->navTargetKind == kNavTargetLink) {
            if (shipData->targetStellar == -1) {
                if (selSystem == shipData->curSystem) {
                    DrawIndString(kGameStrList, 0x151);
                } else {
                    DrawIndString(kGameStrList, 0x152);
                }
            } else {
                DrawIndString(kGameStrList, 0x153);
            }
#if V111 /* V111 spelling */
            MoveTo(r2.left + 0xf, r2.top + 0x1a);
            ForeColor(whiteColor);
#endif
        } else {
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < kMaxStellars) {
                for (j = 0; j < kSpobHyperLinks; j++) {
                    st = stellarData[hypergateMapModeStellar].hyperLinks[j];
                    /* BUG: i is the finished name loop's counter (0x800), so an empty link is never skipped */
#if V111 /* V111 spelling */
                    if (st != -i) {
#else
                    if (-i != st) {
#endif
                        st2 = stellarData[st].system;
                        if (st2 < 0 || st2 >= kMaxSystems)
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
#if V111 /* V111 spelling */
            MoveTo(r2.left + 0xf, r2.top + 0x1a);
            ForeColor(whiteColor);
        }
#else
        }
        MoveTo(r2.left + 0xf, r2.top + 0x1a);
        ForeColor(whiteColor);
#endif
        if (systemData[sel].exploreLevel <= 0) {
            if (cheatsActive != 0) {
                NumToString(sel + kFirstResourceID, buf);
                DrawString(buf);
            } else {
                DrawIndString(kGameStrList, 0x136);
            }
        } else {
            if (cheatsActive != 0) {
                NumToString(sel + kFirstResourceID, buf);
                DrawString(buf);
            } else {
                DrawString(systemData[sel].name);
            }
            if (SystemIsInhabited(sel) != 0) {
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0x4b);
                DrawIndString(kGameStrList, 0x145);
                ForeColor(whiteColor);
                MoveTo(r2.left + 0xf, r2.top + 0x58);
                count = systemData[sel].govt;
                if (count == -1) {
                    DrawIndString(kGameStrList, 0x14d);
                } else if (cheatsActive != 0) {
                    NumToString(count + kFirstResourceID, buf);
                    DrawString("\pID ");
                    DrawString(buf);
                } else {
                    DrawString(govtData[count].name);
                }
                MoveTo(r2.left + 10, r2.top + 0x77);
                RGBForeColor(&rgbLightGrey);
                DrawIndString(kGameStrList, 0x146);
                ForeColor(whiteColor);
                MoveTo(r2.left + 0xf, r2.top + 0x84);
                if (cheatsActive != 0) {
                    NumToString(playerRecord[sel], buf);
                    DrawString(buf);
                } else {
                    DrawLegalStatusString(sel);
                }
                RGBForeColor(&rgbLightGrey);
                MoveTo(r2.left + 10, r2.top + 0x9b);
                DrawIndString(kGameStrList, 0x147);
                ForeColor(whiteColor);
                if (systemData[sel].exploreLevel < 2) {
                    MoveTo(r2.left + 0xf, r2.top + 0xa8);
                    DrawIndString(kGameStrList, 0x136);
                } else {
                    xoff = 0;
                    for (k = 0, name = cargoName[0]; k <= 5; k++, name += sizeof(cargoName[0])) {
                        if (SystemTradesGoods(sel, k) != 0) {
                            MoveTo(r2.left + 0xf, r2.top + 0xa8 + xoff);
                            DrawString(name);
                            xoff += 0xc;
                        }
                    }
                    if (xoff == 0) {
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
                    ForeColor(whiteColor);
                    if (systemData[sel].exploreLevel < 2) {
                        MoveTo(r2.left + 0xf, r2.top + 0x107);
                        DrawIndString(kGameStrList, 0x136);
                    } else {
                        xoff = 0;
                        for (j = 0; j <= 2; j++) {
                            if (SystemHasService(sel, j) != 0) {
                                MoveTo(r2.left + 0xf, r2.top + 0x107 + xoff);
                                DrawIndString(kGameStrList, j + 0x149);
                                xoff += 0xc;
                            }
                        }
                    }
                } else {
                    if (systemData[sel].exploreLevel >= 2) {
                        MoveTo(r2.left + 0xf, r2.top + 0x106);
                        ForeColor(whiteColor);
                        DrawIndString(kGameStrList, 0x14c);
                    } else if (systemData[sel].exploreLevel >= 1) {
                        MoveTo(r2.left + 0xf, r2.top + 0x106);
                        ForeColor(whiteColor);
                        DrawIndString(kGameStrList, 0x136);
                    }
                }
            } else {
                MoveTo(r2.left + 10, r2.top + 0x4b);
                ForeColor(whiteColor);
                DrawIndString(kGameStrList, 0x14e);
            }
        }
        didUpdateMap = 1;
#ifdef MACBUILD
        }
#endif
        SetGWorldContainer(&loadPort);
        DrawMapButtons(-1);
        UnsetGWorldContainer(&loadPort);
        SWSetPortToWindow(gSpriteWorld);
        SetDialogGWorld(mapDialog);
        ForeColor(blackColor);
        CopyBits(GetPortPixMapBits(loadPort.world),
                 GetPortBitMapForCopyBits(GetDialogPort(mapDialog)),
                 GetDialogPortRect(mapDialog), GetDialogPortRect(mapDialog),
                 0, 0);
    }
}
#endif


/* ScrollMapDisplay. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a7470
// MAC: _ScrollMapDisplay i386 0x0001128d ppc 0x00014b8c
void ScrollMapDisplay(short dh, short dv)
{
    short itemType;
    Handle item;
    Rect box;

    if (dh == 0 && dv == 0)
        return;
    SetDialogGWorld(mapDialog);
    GetDialogItem(mapDialog, kMapPictItem, &itemType, &item, &box);
    box.left += 1;
    box.top += 1;
    box.right -= 1;
    box.bottom -= 1;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    BackPat(&qd.black);
    ScrollRect(&box, dh, dv, tempRgn);
    BackPat(&qd.white);
#endif
    partialMapDialogUpdate = 1;
    MapDialogUpdate();
    partialMapDialogUpdate = 0;
}

/* RedrawNebulaeOffscreen. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a74e0
// MAC: _RedrawNebulaeOffscreen i386 0x0000e2d2 ppc 0x00011a94
void RedrawNebulaeOffscreen(void)
{
}


/* MapFilter (mac _MapFilter). STATE: MATCH 100.0% 670/670, byte-identical yes (2440 bytes),
   relocregcheck 0 of 138. OPEN: none. */
// FUNCTION: EVNOVA 0x004a74f0
// MAC: _MapFilter i386 0x00013c63 ppc 0x000175a8
#ifdef MACBUILD
#undef invalRect
#define invalRect box
#endif
Boolean MapFilter(DialogPtr dlg, EventRecord *event, short *itemHit)
#ifdef MAC1010
{
    Handle item;
    short count;
    short n;
    short j;
    short k;
    int what;
    EventChar key;
    short cur;
    short sys;
    short i;
    Boolean ok[kMaxSystemLinks];
    short list[kSpobHyperLinks];
    Rect box;
    Point pt;
    short itemType;

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
        key = EventCharOf(event);
        key = tolower(key);
        if (what == kKeyDown && (IsReturnChar(key) || key == kEscapeChar)) {
            *itemHit = kItemAccept;
            return 1;
        } else if ((key == kTabChar || key == '\\') && onPlanet == 0) {
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < kMaxStellars) {
                for (i = 0; i < kSpobHyperLinks; i++)
                    list[i] = -1;
                n = 0;
                for (i = 0; i < kSpobHyperLinks; i++) {
                    sys = stellarData[hypergateMapModeStellar].hyperLinks[i];
                    if (sys >= 0 && sys < kMaxSystems) {
                        list[n] = FindSystemFromStellar(sys);
                        n++;
                    }
                }
                count = n;
                if (count > 0) {
                    cur = -1;
                    for (i = 0; i < count; i++) {
                        if (selSystem == list[i]) {
                            cur = i;
                            break;
                        }
                    }
                    if (cur == -1)
                        cur = 0;
                    else if ((* &event->modifiers & shiftKey) != 0) {
                        if (cur == 0)
                            cur = n - 1;
                        else
                            cur -= 1;
                    } else
                        cur = (cur + 1) % n;
                    shipData[0].navTargetKind = kNavTargetGate;
                    shipData[0].targetStellar = list[cur];
                    selSystem = list[cur];
                    InvalDialogItem(mapDialog, kMapInfoItem);
                    InvalDialogItem(mapDialog, kMapPictItem);
                    InvalDialogItem(mapDialog, kMapRouteItem);
                    SetDialogGWorld(mapDialog);
                }
            } else {
                shipData[0].navTargetKind = kNavTargetLink;
                count = 0;
                for (i = 0; i < kMaxSystemLinks; i++) {
                    ok[i] = 0;
                    if (systemData[shipData[0].curSystem].links[i] != -1) {
                        if (FindActiveCoLocatedSystem(
                                systemData[shipData[0].curSystem].links[i]) != -1) {
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
                statusRedraw[kRedrawNav] = 1;
                InvalDialogItem(mapDialog, kMapInfoItem);
                InvalDialogItem(mapDialog, kMapPictItem);
                InvalDialogItem(mapDialog, kMapRouteItem);
                DrawStatus();
                SetDialogGWorld(mapDialog);
                if (shipData[0].navTargetKind == kNavTargetLink && shipData[0].targetStellar != -1)
                    selSystem = FindActiveCoLocatedSystem(
                        systemData[shipData[0].curSystem]
                            .links[shipData[0].targetStellar]);
                else
                    selSystem = shipData[0].curSystem;
            }
        } else if (what == kKeyDown && (key == 'c' || key == ' ')) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            mapScroll.h = systemData[shipData[0].curSystem].pos.h;
            mapScroll.v = systemData[shipData[0].curSystem].pos.v;
            selSystem = shipData[0].curSystem;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == '-') {
            *itemHit = kMapZoomOutItem;
            return 1;
        } else if (key == '+' || key == '=') {
            *itemHit = kMapZoomInItem;
            return 1;
        } else if (key == kLeftArrowChar) {
            mapScroll.h -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kRightArrowChar) {
            mapScroll.h += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kUpArrowChar) {
            mapScroll.v -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kDownArrowChar) {
            mapScroll.v += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (what == kKeyDown && (key == 'f' || key == 's')) {
            *itemHit = kMapSearchItem;
            return 1;
        } else if (what == kKeyDown && KeyCheck(keyPrefs.map) != 0) {
            *itemHit = kItemAccept;
            return 1;
        }
    }
    if ((int)event->what == kMouseDown) {
        pt = event->where;
        GlobalToLocal(&pt);
        GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
        if (PtInRectPoint(pt, &box) == 0) {
            MapDialogUpdate();
            SetDialogGWorld(mapDialog);
            sys = TrackMapButtons(pt);
            if (sys == 0) {
                *itemHit = kItemAccept;
                return 1;
            } else if (sys == 1) {
                *itemHit = kMapZoomOutItem;
                return 1;
            } else if (sys == 2) {
                *itemHit = kMapZoomInItem;
                return 1;
            } else if (sys == 3) {
                *itemHit = kMapClearRouteItem;
                return 1;
            } else if (sys == 4) {
                *itemHit = kMapBordersItem;
                return 1;
            } else if (sys == 5) {
                *itemHit = kMapSearchItem;
                return 1;
            } else {
                *itemHit = -1;
                return 1;
            }
        }
        *itemHit = kMapPictItem;
        return 1;
    }
    if ((int)event->what == kUpdateEvt) {
        BeginUpdate(GetDialogWindow(mapDialog));
        MapDialogUpdate();
        EndUpdate(GetDialogWindow(mapDialog));
    }
    if ((int)event->what == kNullEvent && settings.showMapBorders != 0
        && influenceCoordsCached != 0) {
        if (nextInfluenceSyst < 0) {
            nextInfluenceSyst = 0;
            influenceStepSize = 1;
            for (n = 0; n < 256; n++) {
                for (j = 0; j < 256; j++) {
                    for (k = 0; k < 4; k++) {
                        influenceOwner[n][j][k] = 0;
                        influenceValue[n][j][k] = 0;
                    }
                }
            }
        } else {
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
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
                InvalDialogItem(mapDialog, kMapPictItem);
            }
        }
    }
    return 0;
}
#else
{
    Handle item;
    EventKeyByte key;
    short i;
    short j;
    short k;
    short n;
    short count;
    short cur;
    short sys;
    Boolean ok[kMaxSystemLinks];
    short list[kSpobHyperLinks];
    Rect box;
    short itemType;
    EventWhat what;
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
#if V111
        key = (event->message & kKeyCodeMask) >> 8;
        if (what == kKeyDown
            && (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode)) {
            *itemHit = kItemAccept;
            return 1;
        } else if ((key == kTabKeyCode || key == kBackslashKeyCode) && onPlanet == 0) {
#else
        key = tolower(EventCharOf(event));
        if (what == kKeyDown && (IsReturnChar(key) || key == kEscapeChar)) {
            *itemHit = kItemAccept;
            return 1;
        } else if ((key == kTabChar || key == '\\') && onPlanet == 0) {
#endif
            if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
                && hypergateMapModeStellar < kMaxStellars) {
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
                count = n;
                if (count > 0) {
                    cur = -1;
                    for (i = 0; i < count; i++) {
                        if (selSystem == list[i]) {
                            cur = i;
                            break;
                        }
                    }
                    if (cur == -1)
                        cur = 0;
                    else if ((*(unsigned short *)&event->modifiers & shiftKey) != 0) {
                        if (cur == 0)
                            cur = n - 1;
                        else
                            cur -= 1;
                    } else
                        cur = (cur + 1) % n;
                    shipData[0].navTargetKind = kNavTargetGate;
                    shipData[0].targetStellar = list[cur];
                    selSystem = list[cur];
                    InvalDialogItem(mapDialog, kMapInfoItem);
                    InvalDialogItem(mapDialog, kMapPictItem);
                    InvalDialogItem(mapDialog, kMapRouteItem);
                    SetDialogGWorld(mapDialog);
                }
            } else {
                shipData[0].navTargetKind = kNavTargetLink;
                count = 0;
                for (i = 0; i < kMaxSystemLinks; i++) {
                    ok[i] = 0;
                    if (systemData[shipData[0].curSystem].links[i] != -1) {
                        if (FindActiveCoLocatedSystem(
                                systemData[shipData[0].curSystem].links[i]) != -1) {
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
                statusRedraw[kRedrawNav] = 1;
                InvalDialogItem(mapDialog, kMapInfoItem);
                InvalDialogItem(mapDialog, kMapPictItem);
                InvalDialogItem(mapDialog, kMapRouteItem);
                DrawStatus();
                SetDialogGWorld(mapDialog);
                if (shipData[0].navTargetKind == kNavTargetLink && shipData[0].targetStellar != -1)
                    selSystem = FindActiveCoLocatedSystem(
                        systemData[shipData[0].curSystem]
                            .links[shipData[0].targetStellar]);
                else
                    selSystem = shipData[0].curSystem;
            }
#if V111
        } else if (what == kKeyDown && key == kSpaceKeyCode) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            mapScroll.h = systemData[shipData[0].curSystem].pos.h;
            mapScroll.v = systemData[shipData[0].curSystem].pos.v;
            selSystem = shipData[0].curSystem;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kBKeyCode) {
            *itemHit = kMapBordersItem;
            return 1;
        } else if (key == kCKeyCode) {
            *itemHit = kMapClearRouteItem;
            return 1;
        } else if (key == kMinusKeyCode || key == kPadMinusKeyCode) {
            *itemHit = kMapZoomOutItem;
            return 1;
        } else if (key == kPadPlusKeyCode || key == kEqualKeyCode) {
            *itemHit = kMapZoomInItem;
            return 1;
        } else if (key == kLeftArrowKeyCode) {
            mapScroll.h -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kRightArrowKeyCode) {
            mapScroll.h += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kUpArrowKeyCode) {
            mapScroll.v -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kDownArrowKeyCode) {
            mapScroll.v += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (what == kKeyDown && (key == kFKeyCode || key == kSKeyCode)) {
            *itemHit = kMapSearchItem;
            return 1;
#else
#ifdef CEFIX /* F19.03 */
        } else if (what == kKeyDown && key == 'c') {
            *itemHit = kMapClearRouteItem;
            return 1;
        } else if (what == kKeyDown && key == 'b') {
            *itemHit = kMapBordersItem;
            return 1;
        } else if (what == kKeyDown && key == ' ') {
#else
        } else if (what == kKeyDown && (key == 'c' || key == ' ')) {
#endif
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            mapScroll.h = systemData[shipData[0].curSystem].pos.h;
            mapScroll.v = systemData[shipData[0].curSystem].pos.v;
            selSystem = shipData[0].curSystem;
            InvalDialogItem(mapDialog, kMapInfoItem);
            InvalDialogItem(mapDialog, kMapRouteItem);
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == '-') {
            *itemHit = kMapZoomOutItem;
            return 1;
        } else if (key == '+' || key == '=') {
            *itemHit = kMapZoomInItem;
            return 1;
        } else if (key == kLeftArrowChar) {
            mapScroll.h -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kRightArrowChar) {
            mapScroll.h += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kUpArrowChar) {
            mapScroll.v -= 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (key == kDownArrowChar) {
            mapScroll.v += 10;
            InvalDialogItem(mapDialog, kMapPictItem);
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            if (settings.showMapBorders != 0)
                RecalcAllMapInfluence(&box);
        } else if (what == kKeyDown && (key == 'f' || key == 's')) {
            *itemHit = kMapSearchItem;
            return 1;
#endif
        } else if (what == kKeyDown && KeyCheck(keyPrefs.map) != 0) {
            *itemHit = kItemAccept;
            return 1;
        }
    }
    if ((int)event->what == kMouseDown) {
#if V111
        pt = event->where;
#ifdef MACBUILD
        GlobalToLocal(&pt);
#endif
        GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
        if (PtInRectPoint(pt, &box) == 0) {
            MapDialogUpdate();
            SetDialogGWorld(mapDialog);
            sys = TrackMapButtons(pt);
            if (sys == 0) {
                *itemHit = kItemAccept;
                return 1;
            } else if (sys == 1) {
                *itemHit = kMapZoomOutItem;
                return 1;
            } else if (sys == 2) {
                *itemHit = kMapZoomInItem;
                return 1;
            } else if (sys == 3) {
                *itemHit = kMapClearRouteItem;
                return 1;
            } else if (sys == 4) {
                *itemHit = kMapBordersItem;
                return 1;
            } else if (sys == 5) {
                *itemHit = kMapSearchItem;
                return 1;
            } else {
                *itemHit = -1;
                return 1;
            }
        }
        *itemHit = kMapPictItem;
        return 1;
    }
#else
        pt.v = event->where.v;
        pt.h = event->where.h;
        GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
        if (PtInRect(pt.h, pt.v, &box) != 0) {
            *itemHit = kMapPictItem;
            return 1;
        }
        MapDialogUpdate();
        SetDialogGWorld(mapDialog);
        sys = TrackMapButtons(pt);
        if (sys == 0) {
            *itemHit = kItemAccept;
            return 1;
        } else if (sys == 1) {
            *itemHit = kMapZoomOutItem;
            return 1;
        } else if (sys == 2) {
            *itemHit = kMapZoomInItem;
            return 1;
        } else if (sys == 3) {
            *itemHit = kMapClearRouteItem;
            return 1;
        } else if (sys == 4) {
            *itemHit = kMapBordersItem;
            return 1;
        } else if (sys == 5) {
            *itemHit = kMapSearchItem;
            return 1;
        } else {
            *itemHit = -1;
            return 1;
        }
    }
#endif
    if ((int)event->what == kUpdateEvt) {
#ifdef MACBUILD
        BeginUpdate(GetDialogWindow(mapDialog));
        MapDialogUpdate();
        EndUpdate(GetDialogWindow(mapDialog));
#else
        MapDialogUpdate();
#endif
    }
    if ((int)event->what == kNullEvent && settings.showMapBorders != 0
        && influenceCoordsCached != 0) {
#ifdef CEFIX /* F15.06-10 F16.02-04 */
        if (nextInfluenceSyst < 0) {
            nextInfluenceSyst = 0;
        } else if (nextInfluenceSyst <= kMaxSystems) {
            influenceStepSize = 1;
            for (i = 0; i < kCEBordersWidth; i++) {
                for (j = 0; j < kCEBordersHeight; j++) {
                    for (k = 0; k < 4; k++) {
                        influenceOwner[i][j][k] = 0;
                        influenceValue[i][j][k] = 0;
                    }
                }
            }
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
            while (nextInfluenceSyst < kMaxSystems) {
                nextInfluenceSyst += 1;
                sys = FindActiveCoLocatedSystem(nextInfluenceSyst - 1);
                if (sys != -1)
                    AddSystemInfluenceToMap(sys, &box);
            }
            if (nextInfluenceSyst == kMaxSystems) {
                nextInfluenceSyst = kMaxSystems + 1;
                SetDialogGWorld(mapDialog);
                InvalDialogItem(mapDialog, kMapPictItem);
            }
        }
#else
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
            GetDialogItem(mapDialog, kMapPictItem,&itemType, &item, &box);
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
                InvalDialogItem(mapDialog, kMapPictItem);
            }
        }
#endif
    }
    return 0;
}
#endif


/* CompactHyperRoute. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a7e80
// MAC: _CompactHyperRoute i386 0x0000e16a ppc 0x000118c0
void CompactHyperRoute(void)
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

/* SpoolNextHyperRouteDest. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a7fc0
// MAC: _SpoolNextHyperRouteDest i386 0x00012103 ppc 0x00015adc
void SpoolNextHyperRouteDest(void)
{
    short i;
    short group;

    if (hyperRoute[1] != -1)
        group = FindActiveCoLocatedSystem(hyperRoute[1]);
    else
        group = -1;
    if (group == FindActiveCoLocatedSystem(shipData[0].curSystem)) {
        hyperRoute[0] = -1;
        CompactHyperRoute();
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

/* CueNextHyperRouteDest. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a8080
// MAC: _CueNextHyperRouteDest i386 0x0000e2dc ppc 0x00011a9c
void CueNextHyperRouteDest(void)
{
    short i;
    short sys;
    short target;
    short found;

    if ((sys = hyperRoute[1]) != -1) {
        target = FindActiveCoLocatedSystem(sys);
        for (i = 0; i < kMaxSystemLinks; i++) {
            if (systemData[shipData[0].curSystem].links[i] != -1) {
                found = FindActiveCoLocatedSystem(systemData[shipData[0].curSystem].links[i]);
                if (found == target) {
                    shipData[0].navTargetKind = kNavTargetLink;
                    shipData[0].targetStellar = i;
                    statusRedraw[kRedrawNav] = 1;
                    break;
                }
            }
        }
    }
}

#define gateLink j

/* DrawMap (mac _DrawMap). STATE: MATCH 100.0%, 1758/1758, byte-identical yes; mac MATCH on both
   slices; 1.0.10 MATCH on both fragments. The aggregates' declaration order (arrowBox before sysBox)
   is load-bearing: the frame heapsort it feeds breaks savedX's use tie with the left + w/2 slot
   (notes). OPEN: none. */
// FUNCTION: EVNOVA 0x004a8100
// MAC: _DrawMap i386 0x0000e568 ppc 0x00011df8
void DrawMap(Rect box)
{
    FloatPoint destPt;
    FloatPoint curPt;
    RGBColor color;
    Rect sysBox;
    Rect arrowBox;
    short angle;
    short i;
    short j;
    short savedX;
    short savedY;
    short w;
    short h;
    short cur;
    short dest;
    short st;
    MapScroll from;
    MapScroll to;
    short k;
    Boolean show;
    Boolean redArrow;
    Boolean greenArrow;

    w = box.right - box.left;
    h = box.bottom - box.top;
    savedX = mapScroll.h;
    savedY = mapScroll.v;
    for (i = 0; i < kMaxSystems; i++)
        doneFlag[i] = 0;
    if (hypergateMapMode == 0) {
        for (i = 0; i < kMaxHyperRoute - 1; i++) {
            if (hyperRoute[i] != -1 && hyperRoute[i + 1] != -1) {
                from.h = systemData[hyperRoute[i]].pos.h;
                from.v = systemData[hyperRoute[i]].pos.v;
                to.h = systemData[hyperRoute[i + 1]].pos.h;
                to.v = systemData[hyperRoute[i + 1]].pos.v;
                PenSize(2, 2);
                RGBForeColor(&brightGreen);
                MoveTo((box.left + w / 2 + from.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + from.v / mapZoom
                               - savedY / mapZoom));
                LineTo((box.left + w / 2 + to.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + to.v / mapZoom
                               - savedY / mapZoom));
                PenSize(1, 1);
            }
        }
    }
    i = shipData->curSystem;
    for (j = 0; j < kMaxSystemLinks; j++) {
        if (systemData[i].links[j] != -1) {
            dest = FindActiveCoLocatedSystem(systemData[i].links[j]);
            if (dest != -1) {
                if (hypergateMapMode != 0)
                    RGBForeColor(&rgbRealDarkGrey);
                else
                    RGBForeColor(&rgbLessDarkGrey);
                from.h = systemData[i].pos.h;
                from.v = systemData[i].pos.v;
                to.h = systemData[dest].pos.h;
                to.v = systemData[dest].pos.v;
                MoveTo((box.left + w / 2 + from.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + from.v / mapZoom
                               - savedY / mapZoom));
                LineTo((box.left + w / 2 + to.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + to.v / mapZoom
                               - savedY / mapZoom));
            }
        }
    }
    doneFlag[shipData->curSystem] = 1;
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
            && systemData[i].known != 0) {
            for (j = 0; j < kMaxSystemLinks; j++) {
                if (systemData[i].links[j] != -1) {
                    dest = FindActiveCoLocatedSystem(systemData[i].links[j]);
                    if (dest != -1 && doneFlag[dest] == 0) {
                        if (i == shipData->curSystem && shipData->navTargetKind == kNavTargetLink
                            && j == shipData->targetStellar)
                            RGBForeColor(&dimGreen);
                        else if (hypergateMapMode != 0)
                            RGBForeColor(&rgbRealDarkGrey);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        from.h = systemData[i].pos.h;
                        from.v = systemData[i].pos.v;
                        to.h = systemData[dest].pos.h;
                        to.v = systemData[dest].pos.v;
                        MoveTo((box.left + w / 2
                                       + from.h / mapZoom
                                       - savedX / mapZoom),
                               (box.top + h / 2
                                       + from.v / mapZoom
                                       - savedY / mapZoom));
                        LineTo((box.left + w / 2 + to.h / mapZoom
                                       - savedX / mapZoom),
                               (box.top + h / 2 + to.v / mapZoom
                                       - savedY / mapZoom));
                    }
                }
            }
            doneFlag[i] = 1;
        }
    }
    if (shipData->navTargetKind == kNavTargetLink) {
        i = shipData->curSystem;
        j = shipData->targetStellar;
        if (j >= 0 && j < kMaxSystemLinks && systemData[i].links[j] != -1) {
            dest = FindActiveCoLocatedSystem(systemData[i].links[j]);
            if (dest != -1) {
                RGBForeColor(&dimGreen);
                from.h = systemData[i].pos.h;
                from.v = systemData[i].pos.v;
                to.h = systemData[dest].pos.h;
                to.v = systemData[dest].pos.v;
                MoveTo((box.left + w / 2 + from.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + from.v / mapZoom
                               - savedY / mapZoom));
                LineTo((box.left + w / 2 + to.h / mapZoom
                               - savedX / mapZoom),
                       (box.top + h / 2 + to.v / mapZoom
                               - savedY / mapZoom));
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        redArrow = 0;
        greenArrow = 0;
        if (systemData[i].loaded != 0 && systemData[i].prepared != 0) {
            if (shipData->curSystem == i) {
                show = 1;
            } else {
                show = 0;
                if (FindActiveCoLocatedSystem(i) != -1
                    && systemData[i].exploreLevel > 0)
                    show = 1;
                if (systemData[i].known != 0)
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
                from.h = systemData[i].pos.h;
                from.v = systemData[i].pos.v;
                sysBox.left = (box.left + w / 2
                                      + from.h / mapZoom
                                      - savedX / mapZoom);
                sysBox.top = (box.top + h / 2 + from.v / mapZoom
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
                ForeColor(blackColor);
                PaintOval(&sysBox);
                ColorCodeSystem(i, &color);
                RGBForeColor(&color);
                FrameOval(&sysBox);
                ForeColor(blackColor);
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
        from.h = systemData[cur].pos.h;
        from.v = systemData[cur].pos.v;
        sysBox.left = (box.left + w / 2 + from.h / mapZoom
                              - savedX / mapZoom);
        sysBox.top = (box.top + h / 2 + from.v / mapZoom
                             - savedY / mapZoom);
        sysBox.right = sysBox.left;
        sysBox.bottom = sysBox.top;
        if (mapZoom <= 0.5)
            InsetRect(&sysBox, -3, -3);
        else
            InsetRect(&sysBox, -2, -2);
        ForeColor(cyanColor);
        PaintOval(&sysBox);
#ifdef CEFIX /* F15.11 */
        FrameOval(&sysBox);
#endif
        ForeColor(blackColor);
    }
    /* BUG: the upper bound tests hypergateMapMode, not the stellar, and can never fail
       (exe 0x004a921e) */
    if (hypergateMapMode != 0 && hypergateMapModeStellar >= 0
        && hypergateMapMode < kMaxSystems) {
        for (gateLink = 0; gateLink < kSpobHyperLinks; gateLink++) {
            if (stellarData[hypergateMapModeStellar].hyperLinks[gateLink] != -1) {
                st = stellarData[stellarData[hypergateMapModeStellar].hyperLinks[gateLink]]
                         .system;
                if (st < 0 || st >= kMaxSystems)
                    st = FindSystemFromStellar(
                             stellarData[hypergateMapModeStellar].hyperLinks[gateLink]);
                dest = FindActiveCoLocatedSystem(st);
                if (dest >= 0 && dest < kMaxSystems) {
                    if (dest == selSystem) {
                        ForeColor(whiteColor);
                        PenSize(3, 3);
                    } else {
                        ForeColor(cyanColor);
                        PenSize(2, 2);
                    }
                    from.h = systemData[shipData->curSystem].pos.h;
                    from.v = systemData[shipData->curSystem].pos.v;
                    to.h = systemData[dest].pos.h;
                    to.v = systemData[dest].pos.v;
                    curPt.x = (box.left + w / 2
                                      + from.h / mapZoom
                                      - savedX / mapZoom);
                    curPt.y = (box.top + h / 2 + from.v / mapZoom
                                      - savedY / mapZoom);
                    destPt.x = (box.left + w / 2 + to.h / mapZoom
                                       - savedX / mapZoom);
                    destPt.y = (box.top + h / 2 + to.v / mapZoom
                                       - savedY / mapZoom);
                    MoveTo(destPt.x, destPt.y);
                    LineTo(curPt.x, curPt.y);
                    angle = CalcInterceptAngle(&destPt, &curPt);
#ifdef MACBUILD
                    curPt = destPt;
#else
                    curPt.x = destPt.x;
                    curPt.y = destPt.y;
#endif
                    Accel(angle + 45, 8.0f, &curPt);
                    MoveTo(destPt.x, destPt.y);
                    LineTo(curPt.x, curPt.y);
#ifdef MACBUILD
                    curPt = destPt;
#else
                    curPt.x = destPt.x;
                    curPt.y = destPt.y;
#endif
                    Accel((angle + 315) % kFullCircle, 8.0f, &curPt);
                    MoveTo(destPt.x, destPt.y);
                    LineTo(curPt.x, curPt.y);
                    systemData[dest].known = 1;
                }
            }
        }
        PenSize(1, 1);
        ForeColor(blackColor);
    }
    ForeColor(whiteColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0
            && (mapZoom <= 1.1 || i == selSystem)) {
            MoveTo((box.left + w / 2 + systemData[i].pos.h / mapZoom
                           - savedX / mapZoom + 7.0),
                   (box.top + h / 2 + systemData[i].pos.v / mapZoom
                           - savedY / mapZoom + 4.0));
            DrawString(systemData[i].name);
        }
    }
}


/* DrawMapInFloatingFrame (mac _DrawMapInFloatingFrame; was named ClearStarfield). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a99f0
// MAC: _DrawMapInFloatingFrame i386 0x00011304 ppc 0x00014c38
void DrawMapInFloatingFrame(void)
{
    double savedZoom;

    SWSetPortToFrame(floatingMapFrame);
    mapScroll.h = systemData[shipData[0].curSystem].pos.h;
    mapScroll.v = systemData[shipData[0].curSystem].pos.v;

    savedZoom = mapZoom;
    mapZoom = floatingMapZoom;

    ForeColor(blackColor);
    PaintRect(&floatingMapFrame->frameRect);

    if (gSpriteWorld->pixelDepth == 8) {
        ForeColor(blackColor);
        DrawMap(floatingMapFrame->frameRect);
        RGBForeColor(&floatingBorderColor);
        FrameRect(&floatingMapFrame->frameRect);
        ForeColor(blackColor);
    } else {
        RGBForeColor(&floatingBorderColor);
        FrameRect(&floatingMapFrame->frameRect);
        ForeColor(blackColor);
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

/* ShowFloatingMap. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9b30
// MAC: _ShowFloatingMap i386 0x00011440 ppc 0x00014d78
void ShowFloatingMap(void)
{
    short link;
    short route;

    if (floatingMapZoom <= 0.0)
        floatingMapZoom = 1.0;
    selSystem = -1;
    if (shipData->navTargetKind == kNavTargetLink) {
        route = shipData->targetStellar;
        if (route >= 0 && route < kMaxSystemLinks) {
            link = systemData[shipData->curSystem].links[route];
            if (systemData[link].loaded != 0)
                selSystem = link;
        }
    }
    hypergateMapMode = 0;
    showFloatingMap = 1;
    floatingMapStartTime = ReadTick();
    RecalcMissionSystsForMap();
    DrawMapInFloatingFrame();
}


#define kInfluenceCoarseStep 8
#define kInfluenceGridMax 255

/* MapPosToInfluenceCoords (mac _MapPosToInfluenceCoords). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9bf0
// MAC: _MapPosToInfluenceCoords i386 0x000114f7 ppc 0x00014e78
void MapPosToInfluenceCoords(short x, short y, Rect *r, short *outX, short *outY)
{
    short sx;
    short sy;

    sx = ((x - mapScroll.h) / mapZoom);
    sy = ((y - mapScroll.v) / mapZoom);
    *outX = sx / 2;
    *outY = sy / 2;
    *outX += (r->right - r->left) / 2 / 2;
    *outY += (r->bottom - r->top) / 2 / 2;
}

/* RecalcAllMapInfluence (mac _RecalcAllMapInfluence). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9d10
// MAC: _RecalcAllMapInfluence i386 0x00011c37 ppc 0x00015694
void RecalcAllMapInfluence(Rect *r)
{
    influenceStepSize = kInfluenceCoarseStep;
    nextInfluenceSyst = -1;
    influenceCoordsCached = 0;
#ifdef CEFIX /* F15.12 */
    CE_RecalcMapInfluence(0, 0, kInfluenceGridMax, kInfluenceGridMax, r);
#else
    RecalcMapInfluence(0, 0, kInfluenceGridMax, kInfluenceGridMax, r);
#endif
}

/* RecalcMapInfluence (mac _RecalcMapInfluence). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004a9d50
// MAC: _RecalcMapInfluence i386 0x000119ee ppc 0x0001545c
void RecalcMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom, Rect *r)
{
    short i;
    short dy;
    short x;
    short y;
    short group;
    short syst;
    Boolean seen[kMaxSystems];

    if (influenceStepSize < 1)
        influenceStepSize = 1;

#ifdef CEFIX /* F15.13-23 */
    for (x = 0; x < kCEBordersWidth; x += influenceStepSize) {
        for (y = 0; y < kCEBordersHeight; y += influenceStepSize) {
            for (group = 0; group < kInfluenceLayers; group++) {
                influenceOwner[x][y][group] = 0;
                influenceValue[x][y][group] = 0;
            }
        }
    }
#else
    for (x = 0; x < kInfluenceGridSize; x += influenceStepSize) {
        for (y = 0; y < kInfluenceGridSize; y += influenceStepSize) {
            for (group = 0; group < kInfluenceLayers; group++) {
                influenceOwner[x][y][group] = 0;
                influenceValue[x][y][group] = 0;
            }
        }
    }
#endif

    for (i = 0; i < kMaxSystems; i++)
        seen[i] = 0;

    for (i = 0; i < kMaxSystems; i++) {
        group = systemData[i].colocGroup;
        if (group != -1) {
            if (seen[group] == 0) {
                seen[group] = 1;
                syst = FindActiveCoLocatedSystem(i);
                if (syst != -1)
                    AddSystemInfluenceToMap(syst, r);
            }
        } else
            AddSystemInfluenceToMap(i, r);
    }

    if (influenceStepSize > 1) {
#ifdef CEFIX /* F15.24-31 F16.05-06 */
        for (x = 0; x < kCEBordersWidth; x += influenceStepSize) {
            for (y = 0; y < kCEBordersHeight; y += influenceStepSize) {
                for (i = 0; i < influenceStepSize; i++) {
                    for (dy = 0; dy < influenceStepSize; dy++) {
                        if (x + i < ceMapBordersVisibleWidth
                            && y + dy < ceMapBordersVisibleHeight) {
                            for (group = 0; group < kInfluenceLayers; group++) {
                                influenceOwner[x + i][y + dy][group] =
                                    influenceOwner[x][y][group];
                                influenceValue[x + i][y + dy][group] =
                                    influenceValue[x][y][group];
                            }
                        }
                    }
                }
            }
        }
#else
        for (x = 0; x < kInfluenceGridSize; x += influenceStepSize) {
            for (y = 0; y < kInfluenceGridSize; y += influenceStepSize) {
                for (i = 0; i < influenceStepSize; i++) {
                    for (dy = 0; dy < influenceStepSize; dy++) {
                        if (x + i < kInfluenceGridSize
                            && y + dy < kInfluenceGridSize) {
                            for (group = 0; group < kInfluenceLayers; group++) {
                                influenceOwner[x + i][y + dy][group] =
                                    influenceOwner[x][y][group];
                                influenceValue[x + i][y + dy][group] =
                                    influenceValue[x][y][group];
                            }
                        }
                    }
                }
            }
        }
#endif
    }

    influenceCoordsCached = 1;
}

#define kInfluenceSlots 4
#ifdef CEFIX /* F15.52-53 */
#define kInfluenceRadius 22.0
#define kTightInfluenceRadius 11.0
#define kInfluenceMargin 12         /* cells added to the rounded radius */
#define kTightInfluenceMargin 9
#else
#define kInfluenceRadius 30.72
#define kTightInfluenceRadius 15.36
#endif
#define kInfluenceFalloff 0.2
#define kTightInfluenceFalloff 0.4

/* AddSystemInfluenceToMap (mac _AddSystemInfluenceToMap). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004aa070
// MAC: _AddSystemInfluenceToMap i386 0x00011592 ppc 0x00014f4c
Boolean AddSystemInfluenceToMap(short sys, Rect *r)
{
    short mapX;
    short mapY;
    short k;
    short gx;
    short gy;
    short radius;
    short govt;
    short lowestSlot;
    short lowest;
    short distX;
    short distY;
    int32_t r2;
    int32_t d2;
    int32_t level;
    unsigned char strength;
    Boolean placed;

    if (systemData[sys].loaded != 0
        && systemData[sys].known != 0
        && systemData[sys].exploreLevel > 0
        && systemData[sys].govt != -1
        && (govtData[systemData[sys].govt].flags2 & kGovtNoInfluence) == 0
        && SystemIsInhabited(sys) != 0) {
        MapPosToInfluenceCoords(systemData[sys].pos.h, systemData[sys].pos.v, r,
                                &mapX, &mapY);
        systemData[sys].influenceX = mapX;
        systemData[sys].influenceY = mapY;
        if (influenceStepSize > 1) {
            mapX -= mapX % influenceStepSize;
            mapY -= mapY % influenceStepSize;
        }

        govt = systemData[sys].govt;
#ifdef CEFIX /* F15.32-33,52-53 */
        if ((govtData[govt].flags2 & kGovtMinor) != 0)
            radius = CE_RoundD(kTightInfluenceRadius / mapZoom) + kTightInfluenceMargin;
        else
            radius = CE_RoundD(kInfluenceRadius / mapZoom) + kInfluenceMargin;
#else
        if ((govtData[govt].flags2 & kGovtMinor) != 0)
            radius = (kTightInfluenceRadius / mapZoom);
        else
            radius = (kInfluenceRadius / mapZoom);
#endif
        if (influenceStepSize > 1)
            radius -= radius % influenceStepSize;
        r2 = radius * radius;

#ifdef CEFIX /* F15.34-45 F16.07-09 */
        if (mapX >= -radius && mapX <= radius + ceMapBordersVisibleWidth
            && mapY >= -radius && mapY <= radius + ceMapBordersVisibleHeight) {
            for (gx = mapX - radius; gx <= mapX + radius; gx += influenceStepSize) {
                for (gy = mapY - radius; gy <= mapY + radius;
                     gy += influenceStepSize) {
                    if (gx >= 0 && gx < ceMapBordersVisibleWidth
                        && gy >= 0 && gy < ceMapBordersVisibleHeight) {
                        distX = gx - mapX;
                        distY = gy - mapY;
                        if (distX < 0)
                            distX *= -1;
                        if (distY < 0)
                            distY *= -1;
                        d2 = distX * distX + distY * distY;
                        if (d2 > r2)
                            continue;

                        if ((govtData[govt].flags2 & kGovtMinor) != 0)
                            level = ((r2 - d2) * kTightInfluenceFalloff
                                              * mapZoom);
                        else
                            level = ((r2 - d2) * kInfluenceFalloff * mapZoom);
                        if (level < 1)
                            level = 1;
                        if (level > 255)
                            level = 255;
                        strength = level;

                        placed = 0;
                        for (k = 0; k < kInfluenceSlots; k++) {
                            if (influenceValue[gx][gy][k] != 0) {
                                if (GovtAllies(govt, influenceOwner[gx][gy][k]) != 0) {
                                    if (strength
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
            }
            return 1;
        }
#else
        if (mapX >= -radius && mapX <= radius + kInfluenceGridSize
            && mapY >= -radius && mapY <= radius + kInfluenceGridSize) {
            for (gx = mapX - radius; gx <= mapX + radius; gx += influenceStepSize) {
                for (gy = mapY - radius; gy <= mapY + radius;
                     gy += influenceStepSize) {
                    if (gx >= 0 && gx < kInfluenceGridSize
                        && gy >= 0 && gy < kInfluenceGridSize) {
                        distX = gx - mapX;
                        distY = gy - mapY;
                        if (distX < 0)
                            distX *= -1;
                        if (distY < 0)
                            distY *= -1;
                        d2 = distX * distX + distY * distY;
                        if (d2 > r2)
                            continue;

                        if ((govtData[govt].flags2 & kGovtMinor) != 0)
                            level = ((r2 - d2) * kTightInfluenceFalloff
                                              * mapZoom);
                        else
                            level = ((r2 - d2) * kInfluenceFalloff * mapZoom);
                        if (level < 1)
                            level = 1;
                        if (level > 255)
                            level = 255;
                        strength = level;

                        placed = 0;
                        for (k = 0; k < kInfluenceSlots; k++) {
                            if (influenceValue[gx][gy][k] != 0) {
                                if (GovtAllies(govt, influenceOwner[gx][gy][k]) != 0) {
                                    if (strength
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
            }
            return 1;
        }
#endif
    }
    return 0;
}

/* DrawMapInfluence. STATE: MATCH, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x004aa620
// MAC: _DrawMapInfluence i386 0x0000e360 ppc 0x00011b5c
void DrawMapInfluence(Rect bounds)
{
    Rect cell;
    short x;
    short k;
    unsigned char best;
    unsigned char v;
    unsigned char govt;
    RGBColor color;
    short y;

    if (influenceStepSize < 1)
        influenceStepSize = 1;

#ifdef CEFIX /* F15.46-51 */
    for (x = 0; x < kCEBordersWidth; x += influenceStepSize) {
        for (y = 0; y < kCEBordersHeight; y += influenceStepSize) {
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
                if (v > 255)
                    v = 255;
                if (v > best) {
                    color.red = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.red);
                    color.green = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.green);
                    color.blue = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.blue);
                    best = v;
                }
            }

            RGBForeColor(&color);
            PaintRect(&cell);
        }
    }
#else
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
                if (v > 255)
                    v = 255;
                if (v > best) {
                    color.red = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.red);
                    color.green = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.green);
                    color.blue = ((v * 0.5) * govtData[influenceOwner[x][y][k]].rgbBytes.blue);
                    best = v;
                }
            }

            RGBForeColor(&color);
            PaintRect(&cell);
        }
    }
#endif
    ForeColor(blackColor);
}

EXTERN_C_END
