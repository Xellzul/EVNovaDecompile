/* dialogs.h -- the game's own dialogs and the widgets they share. Notes: Research/notes/dialogs.md */
#ifndef DIALOGS_H
#define DIALOGS_H
#include "gametypes.h"
#include "macport.h"
#include "shipobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* desc-resource display flags */
#define kDescMovieAfter     0x0001
#define kDescMovieDoubled   0x0002
#define kDescMovieFade      0x0004

void CommDialogUpdate(void);
extern void MissionListDefinition(short lMessage, Boolean lSelect, Rect *lRect, long lCell,
                         short lDataOffset, short lDataLen, ScrollRec **lHandle);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern long DAT_007355d0;
EXTERN_CPP_END
#pragma cplusplus reset
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean reenterMissionDialog;
extern short adviceDisasterRandom;
extern Boolean willTakeBribe;
extern Boolean hasDemandedTribute;
extern Boolean commShipXeno;
extern short commDialogMode;
extern Boolean shipyardEgg;
EXTERN_CPP_END
#pragma cplusplus reset
extern void DisposeButtons(void);
extern void CenterWindow(void *front, void *window);
extern void DoBarDialog(void);
extern Boolean DoCapturedShipAssignmentDialog(void);
extern void DoCommDialog(short ship);
extern void DoEscortDialog(ShipObj *ship);
extern void DoKeyDialog(void);
extern void DoMissionDialog(void);
extern void DoMissionInfoDialog(void);
extern void DoOutfitDialog(void);
extern void DoPlayerInfoDialog(void);
extern void DoPlunderDialog(void);
extern void DoPortDialog(SpobRecord *spob, short stellar);
extern void DoPrefsDialog(void);
extern void DoRaceDialog(void);
void DoShipyardDescription(short ship);
extern void DoShipyardDialog(void);
extern void DoSystemSearch(void);
extern void DoTradeDialog(void);
extern void DrawBarButtons(short hilited);
extern void DrawButton(CGrafPtr port, Rect *bounds, unsigned char *label,
                       short state, short selected, PicHandle pic);
extern void DrawCapAssignButtons(short hilited);
extern void DrawCommDialogButtons(short hilited);
extern void DrawEscortButtons(short hilited);
extern void DrawHaggleButtons(short hilited);
extern void DrawMapButtons(short hilited);
extern void DrawMissionButtons(short hilited);
extern void DrawMissionInfoButtons(short hilited);
extern void DrawOutfitButtons(short hilited);
extern void DrawPlanetButtons(short selected);
extern void DrawPlanetCommDialogButtons(short hilited);
extern void DrawPlayerInfoButtons(short page, short hilited);
extern void DrawPlunderButtons(short hilited);
extern void DrawRaceButtons(short hilited);
void DrawShipyardButtons(short hilited);
extern void DrawSingleMissionButtons(short hilited);
extern void DrawOkButton(Rect box, Boolean hilited);
extern void DrawTextButtons(short hilited);
extern void DrawTradeButtons(short hilited);
extern Boolean EVTextInputDialog(unsigned char *prompt, unsigned char *initial,
                              short maxLen);
#ifndef MACBUILD /* inline on mac: sources/mac/include/rt3_ev_nova.h */
extern Boolean RegScramA3(unsigned int x);
extern void RegScramB3(unsigned char x);
extern short RegScramC3(unsigned char x);
extern void *RegScramF3(void);
#endif
extern Boolean DoNewPilotNameDialog(Boolean *wantStrictPlay);
void HaggleDialogUpdate(void);
extern void InitButtons(void);
extern void MissionDialogUpdate(void);
short DoShipCommPayment(void);
void OutfitDialogUpdate(void);
void PlunderDialogUpdate(void);
extern void PrefsDialogUpdate(void);
extern void RandomizeNewsDialog(void);
extern void ScrollMapDisplay(short dh, short dv);
extern void CueNextHyperRouteDest(void);
extern void MyModalDialog(DialogPtr dlg, void *eventProc, short *itemHit,
                           short *modifiers);
#ifndef MACBUILD
extern void SetDialogGWorld(DialogPtr dlg);
#endif
#ifndef MAC1010
extern void DrawDialog(DialogPtr dlg);
#endif
void SetPlunderValues(void);
void SetupMissionInfoList(void);
extern void SetupPlayerInfoText(void);
extern void ShowTextDialog(Boolean allowMap, Boolean redrawMission,
                           Boolean showMovie);
extern void ShipDescDialogUpdate(void);
void ShipyardDialogUpdate(); /* unprototyped: the 1.1.1 DoShipyardDialog passes it the dialog */
extern void DoNewsDialog(void);
extern short TrackBarButtons(Point where);
extern short TrackCapAssignButtons(Point where);
extern short TrackCommDialogButtons(Point where);
extern short TrackEscortButtons(Point where);
extern short TrackHaggleButtons(Point where);
extern short TrackMapButtons(Point where);
extern short TrackMissionButtons(Point where);
extern short TrackMissionInfoButtons(Point where);
extern short TrackOutfitButtons(Point where);
extern short TrackPlanetButtons(Point where);
extern short TrackPlanetCommDialogButtons(Point where);
extern short TrackPlayerInfoButtons(short page, Point where);
extern short TrackPlunderButtons(Point where);
extern short TrackRaceButtons(Point where);
extern short TrackShipyardButtons(Point where);
extern short TrackSingleMissionButtons(Point where);
extern Boolean TrackOkButton(Rect box);
extern short TrackTextButtons(Point where);
extern short TrackTradeButtons(Point where);
extern void BarDialogUpdate(void);
void CapturedAssignmentDialogUpdate(void);
void NewsDialogUpdate(void);
void EscortDialogUpdate(void);
void KeyDialogUpdate(void);
extern void SingleMissionDialogUpdate(void);
void MissionInfoDialogUpdate(void);
void PlayerInfoDialogUpdate(void);
void RaceDialogUpdate(void);
void PlanetCommDialogUpdate(void);
extern void DoPlanetCommDialog(short stellar);
extern void PortDialogUpdate(void);
extern void TextDialogUpdate(void);
void TradeDialogUpdate(void);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern unsigned char adviceString[];
extern PicHandle barBackgroundPic;
extern char barDescText[];
extern PicHandle barPic;
extern short basePrice[];
extern Boolean boughtID;
extern Boolean boughtMap;
extern RGBColor buttonColor[3];
extern short buttonFontID;
extern short buttonFontSize;
extern GWorldContainer buttonImagePort;
extern GWorldContainer buttonMaskPort;
#define kButtonPieceCount 9
extern Rect buttonPieceRect[kButtonPieceCount];
extern GWorldContainer buttonPort;
extern short mapButtonTable[5];
extern short shipyardButtonTable[5];
extern short outfitButtonTable[5];
extern short planetButtonTable[];
extern Boolean canBuy;
extern Boolean canRefuseMission;
extern Boolean canScrollTextDown;
extern Boolean canScrollTextUp;
extern Boolean canSell;
extern Boolean canSellEscort;
extern Boolean canUpgradeEscort;
extern PicHandle captureBackgroundPic;
extern short captureOdds;
extern PicHandle commBackgroundPic;
extern unsigned char commResponseString[];
extern ShipObj *commShip;
extern PicHandle commShipPic;
extern short descResourcePictId;
extern Boolean didBuyShip;
extern Boolean didOutfit;
extern short dockedPortItems[];
extern short dockedPortShipTypes[];
extern unsigned long ambientNoiseDelay;
extern void *planetSnd;
extern long initialHaggleVal;
extern Rect listScreenRect[];
extern unsigned char stellarTypeStr[];
extern float bribeFlux;
extern char descTextPtr[];
extern PicHandle newsPic;
extern Rect listPicScreenRect[];
extern char dialogDescTextPtr[];
extern short lastClickItem;
extern short selMissionInfoListItem;
extern PicHandle missionInfoBackgroundPic;
extern PicHandle outfitBackgroundPic;
extern short selOutfitListItem;
extern unsigned char gRT3Table3[];
extern short keyDialogSel;
extern short selOutfitItem;
extern short selShipyardItem;
extern unsigned char shipCommNames[768][256];
extern PicHandle shipDescBackgroundPic;
extern PicHandle bigShipPic;
extern short selShipListItem;
extern short shipyardScroll;
extern unsigned char shipyardLongNames[768][256];
extern PicHandle shipyardBackgroundPic;
extern Boolean canDoMissions;
extern DialogPtr portDialog;
extern PicHandle portBackgroundPic;
extern GWorldContainer planetPicContainer;
extern unsigned char textInputString[];
extern char textShowString[];
#ifdef MACBUILD
#define gTimesFont kFontIDTimes
#else
extern short gTimesFont;
#endif
extern unsigned char govtShortNames[256][256];
extern short greenMissionSyst;
extern PicHandle haggleBackgroundPic;
extern short haggleMode;
extern long haggleVal;
extern Boolean hatesPlayer;
extern PicHandle keyPic;
extern short lastRaceWinner;
extern unsigned long lastTextScrollTime;
extern unsigned char lcOutfitName[512][256];
extern unsigned char lcPluralOutfitName[512][256];
extern Boolean mainScreenBackgroundDirty;
extern DialogPtr missionDialog;
extern short missionsOutside;
extern unsigned long missionOfferTime;
extern Boolean multiArrowsOnSingleSystem;
extern short nextPortMission;
extern DialogPtr outfitDialog;
extern short outfitScroll;
extern PicHandle playerInfoBackgroundPic[3];
extern short selPlayerInfoMode;
extern char playerInfoText[3][4096];
extern PicHandle plunderBackgroundPic;
extern Boolean portBuysNoOutfits;
extern PicHandle previewPic;
extern short previewPicID;
extern float priceFluxMult[2];
extern long raceBetAmount;
extern short raceCount;
extern PicHandle raceBackgroundPic;
extern short racePick;
extern short raceWinner;
#ifndef MACBUILD
EXTERN_CPP_END
#pragma cplusplus reset
extern RGBColor *redColor;
#endif
extern RGBColor redRGB;
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean releaseCommEscort;
extern short responseRandom;
extern short salvageAmmoQty;
extern DialogPtr tradeDialog;
extern long salvageCredits;
extern short salvageFuelQty;
extern short salvageQty;
extern short salvageType;
extern short selMissionListItem;
EXTERN_CPP_END
#pragma cplusplus reset
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short selTradeItem;
extern short shipBuyMode;
extern unsigned char shipClassStr[];
extern unsigned char shipGovtStr[];
extern DialogPtr shipyardDialog;
extern RGBColor shopGridColor[2];
extern Boolean showAbortMissionButton;
extern short singleMissionButtonTable[4];
extern PicHandle textDialogBackgroundPic[3];
extern PicHandle tradeBackgroundPic;
EXTERN_CPP_END
#pragma cplusplus reset

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern DialogPtr barDialog;
extern DialogPtr captureDialog;
extern DialogPtr commDialog;
extern DialogPtr haggleDialog;
extern DialogPtr infoDialog;
extern DialogPtr mapDialog;
extern DialogPtr missionInfoDialog;
extern DialogPtr plunderDialog;
extern DialogPtr raceDialog;
extern DialogPtr keyDialog;
extern DialogPtr prefsDialog;
extern DialogPtr textDialog;
extern DialogPtr qtyDialog;
extern DialogPtr depthDialog;
extern DialogPtr loadingDialog;
extern DialogPtr outfitDescDialog;
EXTERN_CPP_END
#pragma cplusplus reset

extern void EVNoteAlert(const unsigned char *text);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern PicHandle raceButtonPic[4][4];
EXTERN_CPP_END
#pragma cplusplus reset

extern Boolean EVYesNoDialog(unsigned char *prompt);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern Boolean gQuit;
extern unsigned long lastClickTime;
extern Boolean screenFaded;
EXTERN_CPP_END
#pragma cplusplus reset

Boolean ShipDescFilter(long unused, EventRecord *ev, short *itemHit);
Boolean NewsDialogFilter(long unused, EventRecord *ev, short *itemHit);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern ShortPoint menuTopCorner;
EXTERN_CPP_END
#pragma cplusplus reset

extern Boolean BarFilter(long unused, EventRecord *ev, short *itemHit);
Boolean CommFilter(long unused, EventRecord *ev, short *itemHit);
extern Boolean EscortFilter(long unused, EventRecord *ev, short *itemHit);
Boolean HaggleFilter(long unused, EventRecord *ev, short *itemHit);
extern Boolean MapFilter(DialogPtr dlg, EventRecord *event, short *itemHit);
Boolean PlanetCommFilter(long unused, EventRecord *ev, short *itemHit);
extern Boolean PortFilter(long unused, EventRecord *ev, short *itemHit);
extern Boolean TextDialogFilter(long unused, EventRecord *ev, short *itemHit);
#ifdef MACBUILD
Boolean KeyFilter(DialogPtr dialog, EventRecord *ev, short *itemHit);
#else
Boolean KeyFilter(long unused, EventRecord *ev, short *itemHit);
#endif
Boolean MissionInfoFilter(long unused, EventRecord *ev, short *itemHit);
Boolean OutfitFilter(long unused, EventRecord *ev, short *itemHit);
Boolean PlayerInfoFilter(long unused, EventRecord *ev, short *itemHit);
Boolean PlunderFilter(long unused, EventRecord *ev, short *itemHit);
Boolean RaceFilter(long unused, EventRecord *ev, short *itemHit);
Boolean ShipyardFilter(long unused, EventRecord *ev, short *itemHit);
extern Boolean TradeFilter(long unused, EventRecord *ev, short *itemHit);

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern TEHandle offerTextHandle;
EXTERN_CPP_END
#pragma cplusplus reset
#define kMissionButtonLabelCount 2
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern unsigned char missionButtonLabel[kMissionButtonLabelCount][256];
EXTERN_CPP_END
#pragma cplusplus reset

#pragma cplusplus on
EXTERN_CPP_BEGIN
extern TEHandle textDialogTextHandle;
EXTERN_CPP_END
#pragma cplusplus reset
extern short DoHaggleDialog(Boolean raise, short chance);
extern void ScrollDialogText(DialogPtr dlg, short item, TEHandle hTE,
                             short dv, Boolean force);
extern void PlayQTMovie(unsigned char *name, unsigned char doubled,
                        unsigned char fade);
extern long DoQuantityDialog(long maxCount, short mode);
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern ScrollRec **missionList;
EXTERN_CPP_END
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
