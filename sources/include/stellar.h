/* stellar.h -- systems, stellar objects, governments and the map. Notes: Research/notes/stellar.md */
#ifndef STELLAR_H
#define STELLAR_H
#include "gametypes.h"
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the stellar the player is docked at, or landed on */
#pragma cplusplus on
extern "C++" {
extern SpobRecord *dockedPort;
}
#pragma cplusplus reset

extern char AddSystemInfluenceToMap(short sys, Rect *r);
extern void AdvanceJumpRoute(void);
extern char AlliedWithPlayer(ShipObj *ship);
extern void AutoSetExploration(short sys, short maxDepth, short level);
extern short CalcPortDemand(short commodity, unsigned long flags);
extern void CheckJumpRouteArrival(void);
extern void DrawMapInFloatingFrame(void);
extern void ColorCodeShip(ShipObj *ship, RGBColor *color);
extern void ColorCodeStellar(SpobRecord *spob, RGBColor *color);
extern void ColorCodeSystem(short sys, RGBColor *color);
#pragma cplusplus on
extern "C++" {
extern short nagCount;
extern void *mapCursorSwitchRgn;
}
#pragma cplusplus reset
extern void DisplayMessage(short id);
extern void DisposeMapWorld(void);
extern void DoSystemMap(void);
extern void DrawEscortMenuInFrame(void);
extern void DrawMap(Rect r);
extern void DrawMapInfluence(Rect r);
extern short EscapeStellarSearch(short system);
extern void FadeScreenIn(void);
extern void FadeScreenOut(void);
extern short FindActiveCoLocatedSystem(short sys);
extern short FindNearestStellar(ShipObj *ship);
extern short FindNearestThreatToPlayer(void);
extern long FixedMultiply(long a, long b);
extern void HandleStatus(void);
extern void HandleStellarSystemVisibility(void);
extern void InitMapWorld(void);
extern void InitStatus(void);
extern void MapDialogUpdate(void);
extern float RandomSkillLevel(short type);
extern unsigned long ReadTickMilliseconds(void);
extern void RecalcAllMapInfluence(Rect *r);
extern void RecalcMapInfluence(long left, long top, long right, long bottom,
                               Rect *r);
extern void RecalcMissionSystsForMap(void);
void RecursiveAutoExplore(short sys, short depth, short level);
short RecursiveEscapeStellarSearch(short sys);
extern void ResetMapZoom(void);
extern void RestoreOrigClut(void);
extern void SetSystemBackgroundBackColor(void);
extern void SetSystemBackgroundColor(void);
extern char SetupPortAvailableItems(SpobRecord *spob);
extern void SetupPortAvailableShipTypes(SpobRecord *port);
extern void StartHyperJump(void);
extern char StellarsAreDistant(short a, short b);
extern char StellarsAreIdentical(short a, short b);
extern char SystemHasService(short sysID, short kind);
extern char SystemIsInhabited(short sys);
extern char SystemTradesGoods(short sysID, short commodity);
extern char ValidAuxShipSystem(short sys, short mission);
#pragma cplusplus on
extern "C++" {
extern char canZoomIn;
extern char canZoomOut;
}
#pragma cplusplus reset
extern RGBColor *cyanColor;
#pragma cplusplus on
extern "C++" {
extern char didUpdateMap;
extern short exploreDepthLimit;
extern RGBColor floatingBorderColor;
extern double floatingMapZoom;
extern char partialMapDialogUpdate;
extern short hypergateMapModeStellar;
extern char influenceCoordsCached;
extern unsigned char influenceOwner[256][256][4];
extern short influenceStepSize;
extern unsigned char influenceValue[256][256][4];
extern PicHandle mapDialogPict;
extern PicHandle mapNebulaPicts[32][7];
extern char mapSystemDrawn[];
}
#pragma cplusplus reset
extern double mapZoom;
#pragma cplusplus on
extern "C++" {
extern short missionSyst[];
extern short nextInfluenceSyst;
extern PictCacheSlot pictCache[128];
extern short selSystem;
extern char showClearRouteButton;
}
#pragma cplusplus reset

#pragma cplusplus on
extern "C++" {
extern DialogPtr shipDescDialog;
extern DialogPtr newsDialog;
}
#pragma cplusplus reset

extern char GovtAllies(short a, short b);
extern char GovtEnemies(short a, short b);
extern short SpawnStellarDefenseShip(short stellar);

#ifdef __cplusplus
}
#endif

#endif
