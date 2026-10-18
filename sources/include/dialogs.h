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
extern void MissionListDefinition(short lMessage, char lSelect, Rect *lRect, long lCell,
                         short lDataOffset, short lDataLen, ScrollRec **lHandle);
#pragma cplusplus on
extern "C++" {
extern long DAT_007355d0;
}
#pragma cplusplus reset
#pragma cplusplus on
extern "C++" {
extern char reenterMissionDialog;
extern short adviceDisasterRandom;
extern char willTakeBribe;
extern char hasDemandedTribute;
extern char commShipXeno;
extern short commDialogMode;
extern char shipyardEgg;
}
#pragma cplusplus reset
extern void DisposeButtons(void);
extern void CenterWindow(void *front, void *window);
extern void DoBarDialog(void);
extern char DoCapturedShipAssignmentDialog(void);
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
extern void DrawTextButtons(short hilited);
extern void DrawTradeButtons(short hilited);
extern char EVTextInputDialog(unsigned char *prompt, unsigned char *initial,
                              short maxLen);
extern char RegScramA3(unsigned int x);
extern void RegScramB3(unsigned char x);
extern short RegScramC3(unsigned char x);
extern void *RegScramF3(void);
extern char DoNewPilotNameDialog(char *wantStrictPlay);
void HaggleDialogUpdate(void);
extern void InitButtons(void);
extern void MissionDialogUpdate(void);
int OfferBribe(void);
void OutfitDialogUpdate(void);
void PlunderDialogUpdate(void);
extern void PrefsDialogUpdate(void);
extern void RandomizeNewsDialog(void);
extern void ScrollMapDialog(short dh, short dv);
extern void SelectHyperLinkToSelection(void);
extern void RunModalDialog(DialogPtr dlg, void *eventProc, short *itemHit,
                           short *modifiers);
extern void SetDialogGWorld(DialogPtr dlg);
extern void DrawDialog(DialogPtr dlg);
void SetPlunderValues(void);
void SetupMissionInfoList(void);
extern void SetupPlayerInfoText(void);
extern void ShowTextDialog(char allowMap, char redrawMission,
                           char showMovie);
extern void ShipDescDialogUpdate(void);
void ShipyardDialogUpdate(void);
extern void ShowDlg1014(void);
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
extern short TrackTextButtons(Point where);
extern short TrackTradeButtons(Point where);
extern void UpdateBarDialog(void);
void UpdateCaptureDialog(void);
void UpdateDlg1014(void);
void UpdateEscortDialog(void);
void UpdateKeyDialog(void);
extern void UpdateMissionDialog(void);
void UpdateMissionInfoDialog(void);
void UpdatePlayerInfoDialog(void);
void UpdateRaceDialog(void);
void UpdateStellarCommDialog(void);
extern void DoPlanetCommDialog(short stellar);
extern void UpdateStellarDialog(void);
extern void UpdateTextDialog(void);
void UpdateTradeDialog(void);
#pragma cplusplus on
extern "C++" {
extern unsigned char adviceString[];
extern PicHandle barDialogPict;
extern char barDialogText[];
extern PicHandle barPortraitPict;
extern short basePrice[];
extern char boughtID;
extern char boughtMap;
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
extern short portButtonID[];
extern char canBuy;
extern char canRefuseMission;
extern char canScrollTextDown;
extern char canScrollTextUp;
extern char canSell;
extern char canSellEscort;
extern char canUpgradeEscort;
extern PicHandle captureDialogPict;
extern short captureOdds;
extern PicHandle commBackgroundPic;
extern unsigned char commResponseString[];
extern ShipObj *commShip;
extern PicHandle commShipPic;
extern short descResourcePictId;
extern char didBuyShip;
extern char didOutfit;
extern short dockedPortItems[];
extern short dockedPortShipTypes[];
extern unsigned long gAmbientDeadline;
extern void *gAmbientSound;
extern long gBribeAsked;
extern Rect gCellRects[];
extern unsigned char gCommGovtName[];
extern float gCommMood;
extern char gDescText[];
extern PicHandle gDlg1014Pict;
extern Rect gIconRects[];
extern char gItemDescText[];
extern short gLastClickPos;
extern short gMissionInfoIndex;
extern PicHandle gMissionInfoPict;
extern PicHandle gOutfitDialogPict;
extern short gOutfitGridPos;
extern unsigned char gRT3Table3[];
extern short gSelectedBinding;
extern short gSelectedOutfit;
extern short gSelectedShip;
extern unsigned char gShipCommNames[768][256];
extern PicHandle gShipDescBackdrop;
extern PicHandle gShipDescPict;
extern short gShipGridPos;
extern short gShipGridScroll;
extern unsigned char gShipLongNames[768][256];
extern PicHandle gShipyardDialogPict;
extern char gSpobInhabited;
extern DialogPtr portDialog;
extern PicHandle gStellarDialogPict;
extern GWorldContainer gStellarImage;
extern unsigned char gTextInputResult[];
extern char gTextShowString[];
extern short gTimesFont;
extern unsigned char govtShortNames[256][256];
extern short greenMissionSyst;
extern PicHandle haggleDialogPict;
extern short haggleMode;
extern long haggleVal;
extern char hatesPlayer;
extern PicHandle keyDialogPict;
extern short lastRaceWinner;
extern unsigned long lastTextScrollTime;
extern unsigned char lcOutfitName[512][256];
extern unsigned char lcPluralOutfitName[512][256];
extern char mainScreenBackgroundDirty;
extern DialogPtr missionDialog;
extern short missionListVenue;
extern unsigned long missionOfferTime;
extern char multiArrowsOnSingleSystem;
extern short nextPortMission;
extern DialogPtr outfitDialog;
extern short outfitScroll;
extern PicHandle playerInfoBackgroundPic[3];
extern short playerInfoPage;
extern char playerInfoText[3][4096];
extern PicHandle plunderDialogPict;
extern char portBuysNoOutfits;
extern PicHandle previewPic;
extern short previewPicID;
extern float priceFluxMult[2];
extern long raceBetAmount;
extern short raceCount;
extern PicHandle raceDialogPict;
extern short racePick;
extern short raceWinner;
}
#pragma cplusplus reset
extern RGBColor *redColor;
extern RGBColor redRGB;
#pragma cplusplus on
extern "C++" {
extern char releaseCommEscort;
extern short responseRandom;
extern short salvageAmmoQty;
extern DialogPtr tradeDialog;
extern long salvageCredits;
extern short salvageFuelQty;
extern short salvageQty;
extern short salvageType;
extern short selMissionListItem;
}
#pragma cplusplus reset
#pragma cplusplus on
extern "C++" {
extern short selTradeItem;
extern short shipBuyMode;
extern unsigned char shipClassStr[];
extern unsigned char shipGovtStr[];
extern DialogPtr shipyardDialog;
extern RGBColor shopGridColor[2];
extern char showAbortMissionButton;
extern short singleMissionButtonTable[4];
extern PicHandle textDialogBackgroundPic[3];
extern PicHandle tradeBackgroundPic;
}
#pragma cplusplus reset

#pragma cplusplus on
extern "C++" {
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
}
#pragma cplusplus reset

extern void EVNoteAlert(const unsigned char *text);
#pragma cplusplus on
extern "C++" {
extern PicHandle raceButtonPic[4][4];
}
#pragma cplusplus reset

extern char EVYesNoDialog(unsigned char *prompt);
#pragma cplusplus on
extern "C++" {
extern char gQuit;
extern unsigned long gLastClickTime;
extern char screenFaded;
}
#pragma cplusplus reset

char Dlg1005EventProc(long unused, EventRecord *ev, short *itemHit);
char Dlg1014EventProc(long unused, EventRecord *ev, short *itemHit);
#pragma cplusplus on
extern "C++" {
extern ShortPoint menuTopCorner;
}
#pragma cplusplus reset

extern char BarDialogEvent(long unused, EventRecord *ev, short *itemHit);
char CommDialogEvent(long unused, EventRecord *ev, short *itemHit);
extern char EscortDialogEvent(long unused, EventRecord *ev, short *itemHit);
char HaggleDialogEvent(long unused, EventRecord *ev, short *itemHit);
extern char MapFilter(DialogPtr dlg, EventRecord *event, short *itemHit);
char StellarCommDialogEvent(long unused, EventRecord *ev, short *itemHit);
extern char StellarDialogEvent(long unused, EventRecord *ev, short *itemHit);
extern char TextDialogEvent(long unused, EventRecord *ev, short *itemHit);
char KeyDialogEvent(long unused, EventRecord *ev, short *itemHit);
char MissionInfoFilter(long unused, EventRecord *ev, short *itemHit);
char OutfitFilter(long unused, EventRecord *ev, short *itemHit);
char PlayerInfoDialogEvent(long unused, EventRecord *ev, short *itemHit);
char PlunderDialogEvent(long unused, EventRecord *ev, short *itemHit);
char RaceDialogEventProc(long unused, EventRecord *ev, short *itemHit);
char ShipyardFilter(long unused, EventRecord *ev, short *itemHit);
extern char TradeDialogEvent(long unused, EventRecord *ev, short *itemHit);

#pragma cplusplus on
extern "C++" {
extern TEHandle gMissionTE;
}
#pragma cplusplus reset
#define kMissionButtonLabelCount 2
#pragma cplusplus on
extern "C++" {
extern unsigned char missionButtonLabel[kMissionButtonLabelCount][256];
}
#pragma cplusplus reset

#pragma cplusplus on
extern "C++" {
extern TEHandle textDialogTextHandle;
}
#pragma cplusplus reset
extern short DoHaggleDialog(char raise, short chance);
extern void ScrollDialogText(DialogPtr dlg, short item, TEHandle hTE,
                             short dv, char force);
extern void PlayQTMovie(unsigned char *name, unsigned char doubled,
                        unsigned char fade);
extern long DoQuantityDialog(long maxCount, short mode);
#pragma cplusplus on
extern "C++" {
extern ScrollRec **missionList;
}
#pragma cplusplus reset

#ifdef __cplusplus
}
#endif

#endif
