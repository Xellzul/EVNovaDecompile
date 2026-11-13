/* stellar.h -- systems, stellar objects, governments and the map. Notes: Research/notes/stellar.md */
#ifndef STELLAR_H
#define STELLAR_H
#include "gametypes.h"
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#ifdef CEFIX
#include "cefix.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* the stellar the player is docked at, or landed on */
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern SpobRecord *dockedPort;
EXTERN_CPP_END
#pragma cplusplus reset

extern Boolean AddSystemInfluenceToMap(short sys, Rect *r);
extern void CompactHyperRoute(void);
extern Boolean AlliedWithPlayer(ShipObj *ship);
extern void AutoSetExploration(short sys, short maxDepth, short level);
extern short CalcPortDemand(short commodity, uint32_t flags);
extern void SpoolNextHyperRouteDest(void);
extern void DrawMapInFloatingFrame(void);
extern void ColorCodeShip(ShipObj *ship, RGBColor *color);
extern void ColorCodeStellar(SpobRecord *spob, RGBColor *color);
extern void ColorCodeSystem(short sys, RGBColor *color);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short nagCount;
extern void *mapCursorSwitchRgn;
EXTERN_CPP_END
#pragma cplusplus reset
extern void DisplayMessage(short id);
extern void DisposeDialogs(void);
extern void DoSystemMap(void);
#ifdef MACBUILD
extern void DrawEscortMenuInFrame();
#else
extern void DrawEscortMenuInFrame(void);
#endif
extern void DrawMap(Rect r);
extern void DrawMapInfluence(Rect r);
extern short EscapeStellarSearch(short system);
extern void FadeScreenIn(void);
extern void FadeScreenOut(void);
extern short FindActiveCoLocatedSystem(short sys);
extern short FindNearestStellar(ShipObj *ship);
extern short FindNearestThreatToPlayer(void);
extern int32_t FixedMultiply(int32_t a, int32_t b);
extern void HandleStatus(void);
extern void HandleStellarSystemVisibility(void);
extern void SetupDialogs(void);
extern void InitStatus(void);
extern void MapDialogUpdate(void);
extern float RandomSkillLevel(short type);
extern uint32_t ReadTickMilliseconds(void);
extern void RecalcAllMapInfluence(Rect *r);
extern void RecalcMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom,
                               Rect *r);
extern void RecalcMissionSystsForMap(void);
void RecursiveAutoExplore(short sys, short depth, short level);
short RecursiveEscapeStellarSearch(short sys);
extern void ShowEscortMenu(void);
extern void RestoreOrigClut(void);
extern void SetSystemBackgroundBackColor(void);
extern void SetSystemBackgroundColor(void);
extern Boolean SetupPortAvailableItems(SpobRecord *spob);
extern void SetupPortAvailableShipTypes(SpobRecord *port);
extern void ShowFloatingMap(void);
extern Boolean StellarsAreDistant(short a, short b);
extern Boolean StellarsAreIdentical(short a, short b);
extern Boolean SystemHasService(short sysID, short kind);
extern Boolean SystemIsInhabited(short sys);
extern Boolean SystemTradesGoods(short sysID, short commodity);
extern Boolean ValidAuxShipSystem(short sys, short mission);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean canZoomIn;
extern Boolean canZoomOut;
#ifndef MACBUILD
EXTERN_CPP_END
#pragma cplusplus reset
extern RGBColor *cyanColor;
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean didUpdateMap;
extern short maxAutoExploreJumps;
extern RGBColor floatingBorderColor;
extern double floatingMapZoom;
extern Boolean partialMapDialogUpdate;
extern short hypergateMapModeStellar;
extern Boolean influenceCoordsCached;
#ifdef CEFIX
extern unsigned char influenceOwner[kCEBordersWidth][kCEBordersHeight][4];
#else
extern unsigned char influenceOwner[256][256][4];
#endif
extern short influenceStepSize;
#ifdef CEFIX
extern unsigned char influenceValue[kCEBordersWidth][kCEBordersHeight][4];
#else
extern unsigned char influenceValue[256][256][4];
#endif
extern PicHandle mapBackgroundPic;
extern PicHandle nebulaPic[32][7];
extern Boolean doneFlag[];
EXTERN_CPP_END
#pragma cplusplus reset
extern double mapZoom;
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short missionSyst[];
extern short nextInfluenceSyst;
extern PictCacheSlot iconCache[128];
extern short selSystem;
extern Boolean showClearRouteButton;
EXTERN_CPP_END
#pragma cplusplus reset

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern DialogPtr shipDescDialog;
extern DialogPtr newsDialog;
EXTERN_CPP_END
#pragma cplusplus reset

extern Boolean GovtAllies(short a, short b);
extern Boolean GovtEnemies(short a, short b);
extern short SpawnStellarDefenseShip(short stellar);

#ifdef __cplusplus
}
#endif

#endif
