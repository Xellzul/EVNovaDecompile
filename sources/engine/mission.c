// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* mission.c -- the mission object (tu-map O5, win 0x0043bbb0..0x00447f20). Notes: Research/notes/mission.md */
#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "gametypes.h"
#include "gameconst.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "burger.h"
#include "macport.h"
#include "gameutil.h"
#include "player.h"
#include "crashlog.h"
#include "missions.h"
#include "shipobj.h"
#include "resources.h"
#include "render.h"
#include "ships.h"
#include "cefix.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "prototypes.h"
#include "endianswap.h"

#if V111 /* V111: the 1.1.1 source file is UTF-8 */
#define kDudeResName "d\xc3\xbc" "de"
#define kSpobResName "sp\xc3\xb6" "b"
#else
#define kDudeResName "d\x9f" "de"
#define kSpobResName "sp\x9a" "b"
#endif

/* the resource through the handle's own cast: 1.0.10 keeps one deref, win and 1.1.1 reread it */
#ifdef MAC1010
#define MisnFileRec rec
#else
#define MisnFileRec ((MisnRes *)*missionFileHand)
#endif

/* InitMissions (mac _InitMissions). STATE: MATCH 100.0%, exe 509 / obj 509
   instrs, byte-identical yes. OPEN: nothing. */
// FUNCTION: EVNOVA 0x0043bbb0
// MAC: _InitMissions i386 0x00097b97 ppc 0x000a37ac
void InitMissions(void)
{
    MisnRes *rec;
    short i;
    short j;
    short resID;
    uint32_t resType;
    short numLoaded;
    short numMisns;
    unsigned char name[256];
    int32_t k;

    lastMissionStellar = -1;
    offeringFromShip = 0;
    offeringShipIndex = -1;
    missionDialog = NULL;
    greenMissionSyst = -1;
    multiArrowsOnSingleSystem = 1;
    forceRandom = EVBugResourceFieldTest(15);
    currentMissionIndexForSetParser = -1;
    if (EVBugResourceFieldTest(1))
        return;

    if (!EVBugResourceFieldTest(9)) {
        for (j = 0; j < kMaxActiveMissions; j++) {
            currentMissionObjectives[j].active = 0;
            currentMissionObjectives[j].failed = 0;
        }
        for (i = 0; i < kMaxMissionBits; i++)
            missionBit[i] = 0;
        for (i = 0; i < kMaxMissions; i++) {
            missionAvailData[i].availStel = -32000;
            missionAvailData[i].sourceStellar = -1;
            missionAvailData[i].destStellar = -1;
            missionAvailData[i].availRandom = 0;
            missionAvailData[i].availLoc = -1;
            missionAvailData[i].availShipType = -1;
            missionAvailData[i].cargoType = -1;
            missionAvailData[i].cargoQty = -1;
            missionAvailData[i].requires[0] = 0;
            missionAvailData[i].requires[1] = 0;
            missionAvailData[i].payment = 0;
            missionAvailData[i].dispWeight = 0;
            strcpy(missionAvailData[i].availBits, "");
        }
    }

    numLoaded = 0;
    numMisns = CountResources(kMisnType);
    for (i = 0; i < kMaxMissions; i++) {
        missionFileHand = NULL;
        missionFileHand = GetResource(kMisnType, i + kFirstResourceID);
        if (missionFileHand != NULL) {
            numLoaded++;
            rec = (MisnRes *)*missionFileHand;
            missionAvailData[i].availStel = ((MisnRes *)*missionFileHand)->availStel;
            missionAvailData[i].availLoc = MisnFileRec->availLoc;
            missionAvailData[i].availRecord = MisnFileRec->availRecord;
            missionAvailData[i].availRating = MisnFileRec->availRating;
            missionAvailData[i].availRandom = MisnFileRec->availRandom;
            missionAvailData[i].cargoQty = MisnFileRec->cargoQty;
            missionAvailData[i].cargoType = MisnFileRec->cargoType;
            missionAvailData[i].sourceStellar = MisnFileRec->travelStel;
            missionAvailData[i].destStellar = MisnFileRec->returnStel;
            missionAvailData[i].timeLimit = MisnFileRec->timeLimit;
            missionAvailData[i].flags = MisnFileRec->flags;
            missionAvailData[i].flags2 = MisnFileRec->flags2;
            missionAvailData[i].availShipType = MisnFileRec->availShipType;
            missionAvailData[i].requires[0] = MisnFileRec->require1;
            missionAvailData[i].requires[1] = MisnFileRec->require2;
            missionAvailData[i].payment = MisnFileRec->payVal;
            missionAvailData[i].dispWeight = MisnFileRec->dispWeight;
            if (missionAvailData[i].availStel < -1) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" has invalid AvailStel value\n");
                missionAvailData[i].availStel = -1;
            }
            if (((MisnRes *)*missionFileHand)->shipDude >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->shipDude < kFirstResourceID + kMaxDudes) {
                if (dudeData[((MisnRes *)*missionFileHand)->shipDude - kFirstResourceID].loaded == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->shipDude);
                    crashlog_print("\n");
                }
            } else if (((MisnRes *)*missionFileHand)->shipCount > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                crashlog_print_num(((MisnRes *)*missionFileHand)->shipDude);
                crashlog_print("\n");
            }
            if (((MisnRes *)*missionFileHand)->auxShipDude >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->auxShipDude < kFirstResourceID + kMaxDudes) {
                if (dudeData[((MisnRes *)*missionFileHand)->auxShipDude - kFirstResourceID].loaded == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->auxShipDude);
                    crashlog_print("\n");
                }
            } else if (*(short *)(*missionFileHand + 0x48) > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                crashlog_print_num(((MisnRes *)*missionFileHand)->auxShipDude);
                crashlog_print("\n");
            }
            if (((MisnRes *)*missionFileHand)->travelStel >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->travelStel < kFirstResourceID + kMaxStellars) {
                if (stellarData[((MisnRes *)*missionFileHand)->travelStel - kFirstResourceID].prepared == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kSpobResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->travelStel);
                    crashlog_print("\n");
                }
            }
            if (((MisnRes *)*missionFileHand)->returnStel >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->returnStel < kFirstResourceID + kMaxStellars) {
                if (stellarData[((MisnRes *)*missionFileHand)->returnStel - kFirstResourceID].prepared == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kSpobResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->returnStel);
                    crashlog_print("\n");
                }
            }
            strcpy(missionAvailData[i].availBits, ((MisnRes *)*missionFileHand)->availBits);
            AdjustHandleSize(missionFileHand, kMisnResSize);
            if (missionAvailData[i].availLoc < 0)
                missionAvailData[i].availLoc = 0;
            GetResInfo(missionFileHand, &resID, &resType, name);
            TrimResourceName(name);
            strncpy((char *)missionName[i], (char *)name, 250);
            ReleaseResource(missionFileHand);
            if (i % 5 == 0)
                IterateInGameEventLoop(10);
        }
        if (numLoaded >= numMisns)
            break;
    }

    for (i = 0; i < kMaxMissions; i++)
        suppressMissionOffer[i] = 0;
    lastPortMissionLoc = -1;
}

#define kRandomStellarA  -2
#define kRandomStellarB  -3

/* MissionPostLoadValidCheck (mac _MissionPostLoadValidCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043c3e0
// MAC: _MissionPostLoadValidCheck i386 0x0009b8f2 ppc 0x000a78ec
void MissionPostLoadValidCheck(void)
{
    short i;

    if (crashlog_isactive() != 0) {
        for (i = 0; i < kMaxMissions; i++) {
            if (missionAvailData[i].sourceStellar == kRandomStellarA
                    || missionAvailData[i].sourceStellar == kRandomStellarB
                    || missionAvailData[i].sourceStellar > 0)
                SelectMissionStellar(missionAvailData[i].sourceStellar, -1, -1, i);
            if (missionAvailData[i].destStellar == kRandomStellarA
                    || missionAvailData[i].destStellar == kRandomStellarB
                    || missionAvailData[i].destStellar > 0)
                SelectMissionStellar(missionAvailData[i].destStellar, -1, -1, i);
        }
    }
}

#define kStrTooManyA            351
#define kStrTooManyB            352
#define kStrNoMissions          353
#define kMissionDlogID         1006
#define kMissionPictID       0x2139
#define kShowStringMax        0x0fa

#define kItemDone                 7
#define kItemMap                  6
#define kItemMissionInfo         10
#define kItemPlayerInfo           9

/* DoMissionDialog (mac _DoMissionDialog). STATE: MATCH 100.0%, exe 514 / obj 514, relocregcheck
   0/183. OPEN: none -- one `count' serves the free-slot scan and the mission-info scan. */
// FUNCTION: EVNOVA 0x0043c470
// MAC: _DoMissionDialog i386 0x000a37cf ppc 0x000af2b8
void DoMissionDialog(void)
{
    Str255 msg;
    short itemHit;
    Str255 numBuf;
    short i;
    short j;
    short count;
    short savedTarget;
    short savedRoute;
    Boolean accepted;
    Boolean done;

    done = 0;
    missionsOutside = 0;
    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0)
            count++;
    }
    if (count <= 0) {
        if (reenterMissionDialog != 0) {
            reenterMissionDialog = 0;
        } else {
            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, kStrTooManyA);
            strcat((char *)msg, " ");
            NumToString(kMaxActiveMissions, numBuf);
            PtoCstr(numBuf);
            strcat((char *)msg, (char *)numBuf);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, kStrTooManyB);
            strncpy(textShowString, (char *)msg, kShowStringMax);
            ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
            RefreshSAT();
#endif
            SetDialogGWorld(portDialog);
        }
        return;
    }
    greenMissionSyst = -1;
    multiArrowsOnSingleSystem = 1;
    count = 0;
    for (j = 0; j < kMaxMissions; j++) {
        if (dockedPortMissions[0][j] != -1) {
            if (IsMissionAvailable(dockedPortMissions[0][j], 1))
                count++;
        }
    }
    if (count <= 0) {
        if (reenterMissionDialog != 0) {
            reenterMissionDialog = 0;
        } else {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, kStrNoMissions);
            ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
            RefreshSAT();
#endif
        }
        SetDialogGWorld(portDialog);
        return;
    }
    missionDialog = NULL;
    missionDialog = GetNewDialog(kMissionDlogID, MissionDialogUpdate,
                                 MissionFilter);
    if (missionDialog == NULL)
        return;
    missionBackgroundPic[0] = GetPicture(kMissionPictID);
    CenterWindow(FrontWindow(missionDialog), gWind);
    ShowWindowNoFlash(FrontWindow(missionDialog));
    SetDialogGWorld(missionDialog);
    FlushEvents(everyEvent - diskMask, 0);
    reenterMissionDialog = 0;
    ForeColor(blackColor);
    RedrawDialog(missionDialog);
    SetupMissionList();
    ShowTheCursor();
    strncpy(briefingTextPtr, "", 3);
    if (selMissionListItem != -1) {
        LoadDescription(briefingTextPtr,
                        (dockedPortMissions[0][selMissionListItem]
                                + kMissionDescBase));
        MungeBriefing(1, dockedPortMissions[0][selMissionListItem]);
    }
    MissionDialogUpdate();
    SelectWindow(FrontWindow(missionDialog));
    BringToFront(FrontWindow(missionDialog));
    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(missionDialog, MissionFilter, &itemHit, 0);
        if (itemHit == kItemAccept) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            if (selMissionListItem != -1)
                accepted = DoMissionAccept(
                    dockedPortMissions[0][selMissionListItem]);
            if (selMissionListItem != -1)
                LoadDescription(briefingTextPtr,
                                (dockedPortMissions[0][selMissionListItem]
                                        + kMissionDescBase));
            if (accepted != 0) {
                reenterMissionDialog = 1;
                done = 1;
            } else if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                (dockedPortMissions[0][selMissionListItem]
                                        + kMissionDescBase));
                MungeBriefing(1,
                              dockedPortMissions[0][selMissionListItem]);
            }
            MissionDialogUpdate();
        }
        if (itemHit == kItemDone) {
            reenterMissionDialog = 0;
            done = 1;
        }
        if (itemHit == kItemMap) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 1;
            if (selMissionListItem != -1) {
                if ((missionAvailData[dockedPortMissions[0][selMissionListItem]].flags
                     & kMissionBriefingArrow) != 0) {
                    if (briefMissionInfo[dockedPortMissions[0][selMissionListItem]].destStellar
                        != -1) {
                        greenMissionSyst = stellarData[
                            briefMissionInfo[dockedPortMissions[0][selMissionListItem]].destStellar].system;
                    } else if (briefMissionInfo[dockedPortMissions[0][selMissionListItem]].returnStellar
                               != -1) {
                        greenMissionSyst = stellarData[
                            briefMissionInfo[dockedPortMissions[0][selMissionListItem]].returnStellar].system;
                    }
                }
            }
            savedTarget = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            SetDialogGWorld(missionDialog);
            DoSystemMap();
            greenMissionSyst = -1;
            shipData->targetStellar = savedTarget;
            shipData->navTargetKind = savedRoute;
            statusRedraw[kRedrawNav] = 1;
            DrawStatus();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            PortDialogUpdate();
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kItemMissionInfo) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            count = 0;
            for (j = 0; j < kMaxActiveMissions; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissionObjectives[j].flags & kMissionHidden) == 0)
                        count++;
                }
            }
            if (count > 0)
                DoMissionInfoDialog();
            if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                (dockedPortMissions[0][selMissionListItem]
                                        + kMissionDescBase));
                MungeBriefing(1,
                              dockedPortMissions[0][selMissionListItem]);
            }
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            PortDialogUpdate();
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kItemPlayerInfo) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            PortDialogUpdate();
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
    } while (done == 0);
    if (missionBackgroundPic[0] != NULL) {
        QDReleaseResource(missionBackgroundPic[0]);
        missionBackgroundPic[0] = NULL;
    }
    if (missionList != NULL)
        LDispose(missionList);
    DisposeDialogNoFlash(missionDialog);
    missionDialog = NULL;
#ifdef MACBUILD
    RefreshSAT();
#endif
    return;
}

/* SetupMissionList. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043cc20
// MAC: _SetupMissionList i386 0x000a069e ppc 0x000ac360
void SetupMissionList(void)
{
#ifdef MAC1010
    Str255 buf;
    short i;
    short count;
    int32_t len;
    Rect box3;
    Rect box;
    Rect cellBounds;
    FontInfo fontInfo;
#if TARGET_API_MAC_CARBON
    ListDefSpec ldefSpec;
#endif
    Handle itemHandle;
    unsigned char visible;
    Point cellSize;
    Point rowPt;
    short itemType;
#elif defined(MACBUILD)
    Str255 buf;
    short i;
    short count;
    int32_t len;
    FontInfo fontInfo;
    Rect cellBounds;
    ListDefSpec ldefSpec;
    Rect box3;
    Handle itemHandle;
    Rect box;
    unsigned char visible;
    Point cellSize;
    Point rowPt;
    short itemType;
#else
    char buf[256];
    short i;
    short count;
    int32_t len;
    Rect box3;
    Rect cellBounds;
    void *itemHandle;
    Rect box;
    unsigned char visible;
    Point cellSize;
    Point rowPt;
    short itemType;
#endif

    selMissionListItem = 0;

    count = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[0][i] != -1)
            count++;
    }

    SetDialogGWorld(missionDialog);
    GetDialogItem(missionDialog, 2, &itemType, (Handle *)&itemHandle, &box);
    SetRect(&cellBounds, 0, 0, 1, count);

    TextFont(smallTextFont);
    TextSize(smallTextSize);

#ifdef MACBUILD
    GetFontInfo(&fontInfo);
    cellSize.v = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
    if (cellSize.v * count <= box.bottom - box.top) {
        visible = 0;
        GetDialogItem(missionDialog, 3, &itemType, &itemHandle, &box3);
        box.right = box3.right;
    } else {
        visible = 1;
    }
    cellSize.h = box.right - box.left;

#if TARGET_API_MAC_OS8
    missionList = LNew(&box, &cellBounds, cellSize, 128, missionDialog,
                       false, false, false, visible);
#else
    ldefSpec.defType = kListDefUserProcType;
    ldefSpec.u.userProc = NewListDefUPP((ListDefProcPtr)MissionListDefinition);
    CreateCustomList(&box, &cellBounds, cellSize, &ldefSpec, GetDialogWindow(missionDialog),
                     true, false, false, visible, &missionList);
#endif
    HLock((Handle)missionList);
    (*missionList)->selFlags = lOnlyOne;
    (*missionList)->refCon = (int32_t)ldefColors;
    HUnlock((Handle)missionList);
#else
#ifdef CEFIX /* F12.01 */
    cellSize.v = (ceListItemBaseHeight * 1.5);
#else
    cellSize.v = (smallTextSize * 1.5);
#endif

    if (cellSize.v * count <= box.bottom - box.top) {
        visible = 0;
        GetDialogItem(missionDialog, 3, &itemType, (Handle *)&itemHandle, &box3);
        box.right = box3.right;
    } else {
        visible = 1;
    }
    cellSize.h = box.right - box.left;

    missionList = nv_NewDialog(&box, &cellBounds, cellSize,
        (ListDrawProcPtr)MissionListDefinition, (CGrafPtr)missionDialog, visible);
    LockAHandle((Handle)missionList);
    UnlockAHandle((Handle)missionList);
#endif
    LSetDrawingMode(0, missionList);

    rowPt.h = 0;
    for (rowPt.v = 0; rowPt.v < count; rowPt.v++) {
        if (dockedPortMissions[0][rowPt.v] != -1) {
            strncpy((char *)buf, (char *)missionName[dockedPortMissions[0][rowPt.v]], 0xfa);
            PtoCstr((unsigned char *)buf);
            strncpy(briefingTextPtr, (char *)buf, 0xfa);
            MungeBriefing(1, dockedPortMissions[0][rowPt.v]);
            strncpy((char *)buf, briefingTextPtr, 0xfa);
        } else {
            strncpy((char *)buf, "", 3);
        }
        len = strlen((char *)buf);
        LSetCell(buf, len, rowPt, missionList);
    }

    rowPt.h = rowPt.v = 0;
    LSetSelect(1, rowPt, missionList);
    LSetDrawingMode(1, missionList);
    LAutoScroll(missionList);
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* randmissions -- rebuild both mission-computer lists from scratch and sort; then the two choice resolvers SelectMissionCargoType/SelectMissionCargoQty. Notes: Research/notes/randmissions.md */

/*
 * The generated half of one offered mission (mac _briefMissionInfo). Stride
 * 0x20, indexed by mission the same way missionAvailData is. The two stellar
 * fields are named because the code range-checks them against 2048 and then
 * indexes stellarData with them; the two system fields because they hold what
 * FindActiveCoLocatedSystem and 0x0046e790 return. The rest keep raw names.
 */


/*
 * This TU declares its own types rather than including the shared headers, as
 * the three records above already do. Each window is padded to the record's
 * TRUE stride, because systemData and stellarData are both INDEXED here and a
 * short window would silently step by the wrong size.
 */

/*
 * Four helpers of this TU's own, kept FUN_-named because the mac
 * correspondence is by call ORDER and not proven per address.
 */

/* RandomizeMissionData (mac _RandomizeMissionData). STATE: MATCH, exe 186 / obj 186,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043cf00
// MAC: _RandomizeMissionData i386 0x000a1756 ppc 0x000ad45c
void RandomizeMissionData(void)
{
    short i;
    short m;
    short venue;
    short count;
    short saved;
    short best;
    short next;
    short n;
    short j;
    short v;
    Boolean hit;
    short p;
    short k;

    PropagateMissionBitEffects();
    saved = missionsOutside;
    for (venue = 0; venue <= kMissionVenues - 1; venue++) {
        missionsOutside = venue;
        for (i = 0; i < kMaxMissions; i++)
            dockedPortMissions[venue][i] = -1;
        count = 0;
        for (m = 0; m < kMaxMissions; m++) {
            hit = 0;
            for (j = 0; j < kMaxActiveMissions; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if (currentMissions[j].misnIndex == m) {
                        hit = 1;
                        break;
                    }
                }
            }
            if (!hit) {
                if (IsMissionAvailable(m, 0)) {
                    dockedPortMissions[venue][count] = m;
                    count++;
                }
            }
        }
        for (i = 0; i < kMaxMissions; i++) {
            if (dockedPortMissions[venue][i] != -1)
                RandomizeOneMission(dockedPortMissions[venue][i]);
        }
    }
    missionsOutside = saved;

    for (i = 0; i < kMaxMissions; i++)
        tempMissions[i] = -1;

    for (venue = 0; venue <= kMissionVenues - 1; venue++) {
        n = 0;
        for (i = 0; i < kMaxMissions; i++) {
            if (dockedPortMissions[venue][i] != -1) {
                tempMissions[n] = dockedPortMissions[venue][i];
                dockedPortMissions[venue][i] = -1;
                /* BUG: the slot was just set to -1, so this reads missionAvailData[-1] */
                if (missionAvailData[dockedPortMissions[venue][i]].dispWeight > best || n == 0)
                    best = missionAvailData[dockedPortMissions[venue][i]].dispWeight;
                n++;
            }
        }
        k = 0;
        do {
            n = 0;
            for (i = 0; i < kMaxMissions; i++) {
                v = tempMissions[i];
                if (v != -1) {
                    p = missionAvailData[v].dispWeight;
                    if (p >= best) {
                        dockedPortMissions[venue][k] = v;
                        k++;
                        tempMissions[i] = -1;
                    } else if (p > next || n == 0) {
                        next = p;
                    }
                    n++;
                }
            }
            best = next;
        } while (n > 0);
    }

    n = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[1][i] != -1)
            n++;
    }
    if (n > 0)
        nextPortMission = 0;
    else
        nextPortMission = -1;
}

/*
 * The source and destination halves are NOT symmetric: the source stellar's
 * system goes through FindActiveCoLocatedSystem while the destination's is
 * read straight out of stellarData. The out-of-range arm skips the fallback,
 * which is why it is written as an else rather than as a pre-set.
 */
/* RandomizeOneMission (mac _RandomizeOneMission). STATE: MATCH, byte-identical. 1.0.10 MATCH 150 of
   150 on both fragments, padded. OPEN: none */
// FUNCTION: EVNOVA 0x0043d240
// MAC: _RandomizeOneMission i386 0x000a14d1 ppc 0x000ad1ac
void RandomizeOneMission(short mission)
{
    short i;
    short loc;
#ifdef MAC1010
    short s;
#endif

    if (offeringFromShip != 0) {
        loc = 0;
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[shipData->curSystem].navDefault[i] != -1) {
                loc = systemData[shipData->curSystem].navDefault[i];
                break;
            }
        }
    } else {
        loc = shipData->targetStellar;
    }

    if (missionAvailData[mission].sourceStellar == -1) {
        briefMissionInfo[mission].destStellar = -1;
        briefMissionInfo[mission].destSystem = -1;
    } else {
        briefMissionInfo[mission].destStellar =
            SelectMissionStellar(missionAvailData[mission].sourceStellar, loc, -1, mission);
        if (briefMissionInfo[mission].destStellar >= 0
                && briefMissionInfo[mission].destStellar < kMaxStellars) {
            briefMissionInfo[mission].destSystem = FindActiveCoLocatedSystem(
                stellarData[briefMissionInfo[mission].destStellar].system);
            if (briefMissionInfo[mission].destSystem == -1)
                briefMissionInfo[mission].destSystem =
                    FindSystemFromStellar(briefMissionInfo[mission].destStellar);
        } else {
            briefMissionInfo[mission].destSystem = -1;
        }
    }

    if (missionAvailData[mission].destStellar == -1) {
        briefMissionInfo[mission].returnStellar = briefMissionInfo[mission].destStellar;
    } else {
        briefMissionInfo[mission].returnStellar =
            SelectMissionStellar(missionAvailData[mission].destStellar, loc,
                         briefMissionInfo[mission].destStellar, mission);
    }
#ifdef MAC1010
    s = briefMissionInfo[mission].returnStellar;
    briefMissionInfo[mission].returnStellar = briefMissionInfo[mission].returnStellar; /* 1.0.10 pad: the address's second use, so it is the optimiser's temp and the arm's store does not share its add; deleted, no word */
    if (s >= 0 && s < kMaxStellars) {
        briefMissionInfo[mission].returnSystem = stellarData[s].system;
        if (briefMissionInfo[mission].returnSystem == -1)
            briefMissionInfo[mission].returnSystem = FindSystemFromStellar(s);
    } else {
        briefMissionInfo[mission].returnSystem = -1;
    }
#else
    if (briefMissionInfo[mission].returnStellar >= 0
            && briefMissionInfo[mission].returnStellar < kMaxStellars) {
        briefMissionInfo[mission].returnSystem =
            stellarData[briefMissionInfo[mission].returnStellar].system;
        if (briefMissionInfo[mission].returnSystem == -1)
            briefMissionInfo[mission].returnSystem =
                FindSystemFromStellar(briefMissionInfo[mission].returnStellar);
    } else {
        briefMissionInfo[mission].returnSystem = -1;
    }
#endif

    briefMissionInfo[mission].cargoType = SelectMissionCargoType(missionAvailData[mission].cargoType);
    briefMissionInfo[mission].cargoUnits = SelectMissionCargoQty(missionAvailData[mission].cargoQty);
    briefMissionInfo[mission].payment = missionAvailData[mission].payment;
    SetMissionDeadline(&briefMissionInfo[mission].dueDate,
                  missionAvailData[mission].timeLimit);
}

/* SelectMissionCargoType (mac _SelectMissionCargoType). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043d490
// MAC: _SelectMissionCargoType i386 0x000981d8 ppc 0x000a3e68
short SelectMissionCargoType(short choice)
{
    if (choice >= 0 && choice < 1000)
        return choice;
    if (choice == 1000)
        return Rand(kCargoTypes);
    return -1;
}


/* SelectMissionCargoQty (mac _SelectMissionCargoQty). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043d4c0
// MAC: _SelectMissionCargoQty i386 0x00098191 ppc 0x000a3e00
short SelectMissionCargoQty(short choice)
{
    short spread;

    if (choice >= 0)
        return choice;
    if (choice <= -2) {
        spread = __builtin_abs(choice);
        return Rand(spread) + spread / 2;
    }
    return 0;
}

#define kStellarNone        -1
#define kStellarRandom      -2
#define kStellarRandomUninh -3
#define kStellarNoneAlt     -4

#define kStellarGovt        10000
#define kStellarAllied      15000
#define kStellarNotGovt     20000
#define kStellarEnemy       25000
#define kStellarClass       30000
#define kStellarNotClass    31000

#define kSystemRandom       -2
#define kSystemAdjacent     -5      /* a random hyperlink out of `loc` (name TBC) */

#define kSystemGovt         10000
#define kSystemAllied       15000
#define kSystemNotGovt      20000
#define kSystemEnemy        25000
#define kSystemClass        30000
#define kSystemNotClass     31000
#define kSystemBandLimit    32000

/* SelectMissionStellar (mac _SelectMissionStellar). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043d510
// MAC: _SelectMissionStellar i386 0x0009a26a ppc 0x000a609c
short SelectMissionStellar(short sel, short loc, short avoid, short mission)
{
    short i;
    short stellar;
    short count;
    Boolean allowUninhabited;
    Boolean ok;

    stellar = loc;
    if (sel == kStellarNone || sel == kStellarNoneAlt) {
        stellar = loc;
    } else {
        if (sel == kStellarRandom) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && StellarIsNormalLandable(&stellarData[i])
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                /* BUG: the distance term tests the scan's leftover i, not the rolled stellar */
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || (stellarData[stellar].flags & kSpobIsStation) != 0
                        || !StellarsAreDistant(i, loc));
            }
        }
        if (sel == kStellarRandomUninh) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && (stellarData[i].flags & kSpobUninhabited) != 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && (stellarData[i].flags & kSpobIsStation) == 0
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random uninhabited stellar");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || (stellarData[stellar].flags & kSpobUninhabited) == 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || (stellarData[stellar].flags & kSpobIsStation) != 0
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel > 0 && sel < kFirstResourceID) {
            crashlog_print("warning: mission ");
            crashlog_print_num(mission + kFirstResourceID);
            crashlog_print(" failed to find stellar with ID of ");
            crashlog_print_num(sel);
            crashlog_print("\n");
            return loc;
        }
        if (sel >= kFirstResourceID && sel < kFirstResourceID + kMaxStellars) {
            stellar = sel;
            stellar -= kFirstResourceID;
        }
        /* BUG: off by one, 9999 enters the govt band as if it were one */
        if (sel >= kStellarGovt - 1 && sel < kStellarAllied) {
            allowUninhabited = 0;
            if (sel >= kStellarGovt
                    && (govtData[sel - kStellarGovt].flags & kGovtAlwaysDisabled) != 0)
                allowUninhabited = 1;
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && stellarData[i].govt == sel - kStellarGovt
                        && ((stellarData[i].flags & kSpobUninhabited) == 0 || allowUninhabited)
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar of govt ");
                crashlog_print_num(sel - kStellarGovt + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || stellarData[stellar].govt != sel - kStellarGovt
                        || ((stellarData[stellar].flags & kSpobUninhabited) != 0 && !allowUninhabited)
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarAllied && sel < kStellarNotGovt) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarAllied) {
                    if (systemData[i].govt == sel - kStellarAllied)
                        ok = 1;
                    if (GovtAllies(stellarData[i].govt, sel - kStellarAllied))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar allied with govt ");
                crashlog_print_num(sel - kStellarAllied + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && stellarData[stellar].govt != sel - kStellarAllied) {
                        if (systemData[stellar].govt == sel - kStellarAllied)
                            ok = 1;
                        if (GovtAllies(stellarData[stellar].govt, sel - kStellarAllied))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarNotGovt && sel < kStellarEnemy) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && stellarData[i].govt != sel - kStellarNotGovt
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar not of govt ");
                crashlog_print_num(sel - kStellarNotGovt + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
            do {
                stellar = Rand(kMaxStellars);
            } while (stellar == avoid
                    || !stellarData[stellar].loaded
                    || !SystemVisible(stellarData[stellar].system)
                    || stellarData[stellar].govt == sel - kStellarNotGovt
                    || (stellarData[stellar].flags & kSpobUninhabited) != 0
                    || !StellarIsNormalLandable(&stellarData[stellar])
                    || !StellarsAreDistant(stellar, loc));
        }
        if (sel >= kStellarEnemy && sel < kStellarClass) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarEnemy) {
                    if (GovtEnemies(stellarData[i].govt, sel - kStellarEnemy))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar that is enemy of govt ");
                crashlog_print_num(sel - kStellarEnemy + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && stellarData[stellar].govt != sel - kStellarEnemy) {
                        if (GovtEnemies(stellarData[stellar].govt, sel - kStellarEnemy))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarClass && sel < kStellarNotClass) {
            /* BUG: an exact govt match is rejected, so the band never picks the govt it names */
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarClass) {
                    if (GovtSharedClass(stellarData[i].govt, sel - kStellarClass))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar of govt ");
                crashlog_print_num(sel - kStellarClass + kFirstResourceID);
                crashlog_print(" or a govt of same class");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && stellarData[stellar].govt != sel - kStellarClass) {
                        if (GovtSharedClass(stellarData[stellar].govt, sel - kStellarClass))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarNotClass && sel < 32000) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarNotClass) {
                    if (!GovtSharedClass(stellarData[i].govt, sel - kStellarNotClass))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar not of govt ");
                crashlog_print_num(sel - kStellarNotClass + kFirstResourceID);
                crashlog_print(" or a govt of same class");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
            do {
                stellar = Rand(kMaxStellars);
                ok = 0;
                if (stellarData[stellar].govt != -1
                        && stellarData[stellar].govt != sel - kStellarNotClass) {
                    if (!GovtSharedClass(stellarData[stellar].govt,
                            sel - kStellarNotClass))
                        ok = 1;
                }
            } while (stellar == avoid || stellar == loc
                    || !stellarData[stellar].loaded
                    || !SystemVisible(stellarData[stellar].system)
                    || !ok
                    || (stellarData[stellar].flags & kSpobUninhabited) != 0
                    || !StellarIsNormalLandable(&stellarData[stellar])
                    || !StellarsAreDistant(stellar, loc));
        }
    }
    return stellar;
}

/* SelectMissionSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043e6f0
// MAC: _SelectMissionSystem i386 0x0009b993 ppc 0x000a7998
short SelectMissionSystem(short sel, short loc)
{
    short sys;
    short i;
    short count;
    Boolean ok;
    short link;

    sys = loc;
    if (sel == kSystemRandom) {
        count = 0;
        for (sys = 0; sys < kMaxSystems; sys++) {
            if (systemData[sys].loaded
                    && SystemVisible(sys)
                    && FindFirstCoLocatedSystem(sys)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel == kSystemAdjacent) {
        do {
            i = Rand(kMaxSystemLinks);
        } while (systemData[loc].links[i] == -1
                || (link = FindActiveCoLocatedSystem(systemData[loc].links[i])) == -1
                || !SystemVisible(link));
        sys = link;
    } else if (sel >= kFirstResourceID && sel < kFirstResourceID + kMaxSystems) {
        sys = sel;
        sys -= kFirstResourceID;
    } else if (sel >= kSystemGovt - 1 && sel < kSystemAllied) {
        /* BUG: off by one, 9999 enters the govt band as if it were one */
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && systemData[i].govt == sel - kSystemGovt
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || systemData[sys].govt != sel - kSystemGovt
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemAllied && sel < kSystemNotGovt) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtAllies(systemData[i].govt, sel - kSystemAllied))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (systemData[sys].govt >= 0
                        && systemData[sys].govt != sel - kSystemAllied) {
                    if (GovtAllies(systemData[sys].govt, sel - kSystemAllied))
                        ok = 1;
                }
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemNotGovt && sel < kSystemEnemy) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && systemData[i].govt != sel - kSystemNotGovt
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || systemData[sys].govt == sel - kSystemNotGovt
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemEnemy && sel < kSystemClass) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtEnemies(systemData[i].govt, sel - kSystemEnemy))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (GovtEnemies(systemData[sys].govt, sel - kSystemEnemy))
                    ok = 1;
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemClass && sel < kSystemNotClass) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtSharedClass(systemData[i].govt, sel - kSystemClass))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (GovtSharedClass(systemData[sys].govt, sel - kSystemClass))
                    ok = 1;
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemNotClass && sel < kSystemBandLimit) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (!GovtSharedClass(systemData[i].govt, sel - kSystemNotClass))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (!GovtSharedClass(systemData[sys].govt, sel - kSystemNotClass))
                    ok = 1;
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    }
    return sys;
}

/* the date `days` days from today; nothing is written for a non-positive count */

/* SetMissionDeadline. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043f080
// MAC: _SetMissionDeadline i386 0x00098226 ppc 0x000a3ec4
void SetMissionDeadline(DateTimeRec *out, short days)
{
    DateTimeRec d;
    short i;

    d.month = gameTime.month;
    d.day = gameTime.day;
    d.year = gameTime.year;
    if (days <= 0)
        return;
    for (i = 0; i < days; i++)
        IncrementDate(&d, 0);
    out->month = d.month;
    out->day = d.day;
    out->year = d.year;
}

#define kStrNoHoldsAtAll       355
#define kStrHoldsAreFull       356
#define kBriefingTextMax    0x1ffe
#define kMissionNameMax       0x7f

/* mission Pay values below this encode a COST of (kMissionCostFloor - pay) */
#define kMissionCostFloor   (-50000L)

/* DoMissionAccept (mac _DoMissionAccept). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043f100
// MAC: _DoMissionAccept i386 0x000a1b11 ppc 0x000ad8bc
Boolean DoMissionAccept(short mission)
{
    short i;
    short n;
    short savedVenue;
    short slot;
    short list[kMaxMissions];

    slot = -1;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return 0;

    if (briefMissionInfo[mission].cargoUnits > 0) {
        if (ShipTotalHolds() < briefMissionInfo[mission].cargoUnits) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, kStrNoHoldsAtAll);
            ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
            RefreshSAT();
#endif
            return 0;
        }
        if (TotalMissionCargoSpace() < briefMissionInfo[mission].cargoUnits) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, kStrHoldsAreFull);
            ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
            RefreshSAT();
#endif
            return 0;
        }
    }

    currentMissionObjectives[slot].active = 1;
    currentMissionObjectives[slot].failed = 0;
    currentMissionObjectives[slot].returnDone = 0;
    currentMissionObjectives[slot].objectivesDone = 0;
    currentMissions[slot].specialShipsKilled = 0;
    currentMissions[slot].specialShipsJumpedOut = 0;
    currentMissions[slot].specialShipsJumpedIn = 0;
    currentMissions[slot].specialShipsBoarded = 0;
    currentMissions[slot].specialShipsDisabled = 0;
    strncpy(currentMissions[slot].name, (char *)missionName[mission], kMissionNameMax);
    LoadCurrentMissionData(mission, slot);
    currentMissions[slot].misnIndex = mission;
    if (currentMissions[slot].timeLimit > 0)
        SetMissionDeadline(&currentMissionObjectives[slot].dueDate, currentMissions[slot].timeLimit);
    currentMissionObjectives[slot].flags = currentMissions[slot].flags;
    crashlog_mission_bit_set("beginning mission", currentMissions[slot].name, 1,
                             currentMissions[slot].misnIndex + kFirstResourceID,
                             currentMissions[slot].beginSetString);
    EvalCurrentMissionBitSetString(currentMissions[slot].beginSetString, slot);
    if (currentMissions[slot].payment < kMissionCostFloor) {
        shipData->credits -= kMissionCostFloor - currentMissions[slot].payment;
        if (shipData->credits < 0)
            shipData->credits = 0;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (missionDialog != NULL)
        SetDialogVisible(missionDialog, 0);
    if (currentMissions[slot].briefDesc != -1) {
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[slot].briefDesc);
        MungeBriefing(0, slot);
        strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
        if (missionDialog != NULL)
            SetDialogVisible(missionDialog, 0);
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 1;
        if (currentMissions[slot].flags & kMissionBriefingArrow) {
            if (currentMissions[slot].travelStellar != -1)
                greenMissionSyst = stellarData[currentMissions[slot].travelStellar].system;
            else if (currentMissions[slot].returnStellar != -1)
                greenMissionSyst = stellarData[currentMissions[slot].returnStellar].system;
        }
        if (portDialog != NULL && !offeringFromShip)
            PortDialogUpdate();
        ShowTextDialog(1, 0, 1);
#ifdef MACBUILD
        RefreshSAT();
#endif
        greenMissionSyst = -1;
    }
    if (currentMissions[slot].pickupMode == 0) {
        currentMissions[slot].hasCargo = 1;
        if (currentMissions[slot].loadCargoDesc != -1) {
            strncpy(briefingTextPtr, "", 3);
            LoadDescription(briefingTextPtr, currentMissions[slot].loadCargoDesc);
            MungeBriefing(0, slot);
            strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
            ShowTextDialog(0, 0, 1);
            RefreshSAT();
        }
    }
    if ((currentMissions[slot].travelStellar == shipData->targetStellar && missionDialog != NULL)
        || currentMissions[slot].travelStellar == -1)
        currentMissionObjectives[slot].returnDone = 1;
    if (!offeringFromShip && missionDialog != NULL) {
        savedVenue = missionsOutside;
        for (missionsOutside = 0; missionsOutside <= 1; missionsOutside++) {
            n = 0;
            for (i = 0; i < kMaxMissions; i++)
                list[i] = -1;
            for (i = 0; i < kMaxMissions; i++) {
                if (mission != dockedPortMissions[missionsOutside][i]
                    && dockedPortMissions[missionsOutside][i] != -1) {
                    if (IsMissionAvailable(dockedPortMissions[missionsOutside][i], 1)) {
                        list[n] = dockedPortMissions[missionsOutside][i];
                        n++;
                    } else {
                        crashlog_print("suppressing previously available mission ");
                        crashlog_print_num(dockedPortMissions[missionsOutside][i] + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
            for (i = 0; i < kMaxMissions; i++)
                dockedPortMissions[missionsOutside][i] = list[i];
        }
        missionsOutside = savedVenue;
    }
    if (playerIsLanded && !offeringFromShip
        && (currentMissions[slot].flags & kMissionAutoAbort)
        && currentMissionObjectives[slot].returnDone
        && currentMissions[slot].shipCount == 0
        && currentMissions[slot].returnStellar == -1)
        AutoAbortMission(slot);
    missionRandom[mission] = 0;
    return 1;
}

/* LoadCurrentMissionData (mac _LoadCurrentMissionData). STATE: MATCH, exe 692 / obj 692,
   relocregcheck 195 rows 0 differ. OPEN: none */
// FUNCTION: EVNOVA 0x0043f8c0
// MAC: _LoadCurrentMissionData i386 0x000a0a38 ppc 0x000ac638
void LoadCurrentMissionData(short mission, short slot)
{
    Str255 buf;
    Handle h;
    short strCount;
    short n;

    currentMissions[slot].travelStellar = briefMissionInfo[mission].destStellar;
    currentMissions[slot].returnStellar = briefMissionInfo[mission].returnStellar;
    currentMissions[slot].cargoType = briefMissionInfo[mission].cargoType;
    currentMissions[slot].cargoQty = briefMissionInfo[mission].cargoUnits;

    missionFileHand = GetResource(kMisnType, mission + kFirstResourceID);
    if (missionFileHand != NULL) {
        LockAHandle(missionFileHand);
        currentMissions[slot].flags = ((MisnRes *)*missionFileHand)->flags;
        currentMissions[slot].flags2 = ((MisnRes *)*missionFileHand)->flags2;
        currentMissions[slot].pickupMode = ((MisnRes *)*missionFileHand)->pickupMode;
        currentMissions[slot].dropoffMode = ((MisnRes *)*missionFileHand)->dropOffMode;
        currentMissions[slot].scanMask = ((MisnRes *)*missionFileHand)->scanMask;
        currentMissions[slot].payment = ((MisnRes *)*missionFileHand)->payVal;
        currentMissions[slot].shipCount = ((MisnRes *)*missionFileHand)->shipCount;
        currentMissions[slot].initialShipCount = currentMissions[slot].shipCount;
        currentMissions[slot].shipDude = ((MisnRes *)*missionFileHand)->shipDude;
        if (currentMissions[slot].shipDude > 0)
            currentMissions[slot].shipDude -= kFirstResourceID;
        currentMissions[slot].shipGoal = ((MisnRes *)*missionFileHand)->shipGoal;
        currentMissions[slot].shipBehav = ((MisnRes *)*missionFileHand)->shipBehav;
        currentMissions[slot].shipStart = ((MisnRes *)*missionFileHand)->shipStart;
        if ((currentMissions[slot].flags & kMissionFixedShipType)
                && currentMissions[slot].shipDude >= 0) {
            currentMissions[slot].shipTypeIndex = SelectShipFieldFromDude(&dudeData[currentMissions[slot].shipDude], 0);
            if (currentMissions[slot].shipTypeIndex == -1)
                currentMissions[slot].shipTypeIndex = SelectShipFieldFromDude(&dudeData[currentMissions[slot].shipDude], 1);
        } else {
            currentMissions[slot].shipTypeIndex = -1;
        }
        currentMissions[slot].compGovt = ((MisnRes *)*missionFileHand)->compGovt;
        currentMissions[slot].compReward = ((MisnRes *)*missionFileHand)->compReward;
        if (currentMissions[slot].compGovt >= kFirstResourceID && currentMissions[slot].compGovt < kFirstResourceID + kMaxGovts)
            currentMissions[slot].compGovt -= kFirstResourceID;
        else
            currentMissions[slot].compGovt = -1;
        if (currentMissions[slot].compGovt < 0 || currentMissions[slot].compGovt >= kMaxGovts)
            currentMissions[slot].compReward = 0;
        currentMissions[slot].briefDesc = ((MisnRes *)*missionFileHand)->briefText;
        currentMissions[slot].quickBriefDesc = ((MisnRes *)*missionFileHand)->quickBrief;
        currentMissions[slot].loadCargoDesc = ((MisnRes *)*missionFileHand)->loadCargText;
        currentMissions[slot].dropCargoDesc = ((MisnRes *)*missionFileHand)->dumpCargoText;
        currentMissions[slot].compDesc = ((MisnRes *)*missionFileHand)->compText;
        currentMissions[slot].failDesc = ((MisnRes *)*missionFileHand)->failText;
        currentMissions[slot].shipDoneDesc = ((MisnRes *)*missionFileHand)->shipDoneText;
        currentMissions[slot].refuseDesc = ((MisnRes *)*missionFileHand)->refuseText;
        currentMissions[slot].timeLimit = ((MisnRes *)*missionFileHand)->timeLimit;
        currentMissions[slot].datePostIncrement = ((MisnRes *)*missionFileHand)->datePostInc;
        if (currentMissions[slot].briefDesc <= 0)
            currentMissions[slot].briefDesc = -1;
        if (currentMissions[slot].quickBriefDesc <= 0)
            currentMissions[slot].quickBriefDesc = -1;
        if (currentMissions[slot].loadCargoDesc <= 0)
            currentMissions[slot].loadCargoDesc = -1;
        if (currentMissions[slot].dropCargoDesc <= 0)
            currentMissions[slot].dropCargoDesc = -1;
        if (currentMissions[slot].compDesc <= 0)
            currentMissions[slot].compDesc = -1;
        if (currentMissions[slot].failDesc <= 0)
            currentMissions[slot].failDesc = -1;
        if (currentMissions[slot].shipDoneDesc <= 0)
            currentMissions[slot].shipDoneDesc = -1;
        if (currentMissions[slot].refuseDesc <= 0)
            currentMissions[slot].refuseDesc = -1;
        currentMissions[slot].auxShipDude = ((MisnRes *)*missionFileHand)->auxShipDude;
        currentMissions[slot].auxShipCount = ((MisnRes *)*missionFileHand)->auxShipCount;
        currentMissions[slot].auxShipSyst = ((MisnRes *)*missionFileHand)->auxShipSyst;
        currentMissions[slot].auxShipDelay = Rand(70) + 70;
        currentMissions[slot].auxShipsLeft = currentMissions[slot].auxShipCount;
        if (currentMissions[slot].auxShipDude >= kFirstResourceID && currentMissions[slot].auxShipDude < kFirstResourceID + kMaxDudes) {
            currentMissions[slot].auxShipDude -= kFirstResourceID;
        } else {
            currentMissions[slot].auxShipDude = -1;
            currentMissions[slot].auxShipCount = 0;
        }
        if (currentMissions[slot].auxShipCount <= 0) {
            currentMissions[slot].auxShipCount = -1;
            currentMissions[slot].auxShipsLeft = -1;
        }
        if (currentMissions[slot].shipStart > 0) {
            if (currentMissions[slot].shipBehav == kMissionModeEscort && currentMissions[slot].shipGoal == kMissionGoalEscort)
                currentMissions[slot].shipDelay = 30;
            else
                currentMissions[slot].shipDelay = Rand(100) + 100;
        } else {
            currentMissions[slot].shipDelay = -1;
        }
        currentMissions[slot].destStrList = -1;
        currentMissions[slot].destStrIndex = -1;
        currentMissions[slot].shipStrList = -1;
        currentMissions[slot].shipStrIndex = -1;
        if (currentMissions[slot].timeLimit <= 0)
            currentMissions[slot].timeLimit = -32000;
        if (((MisnRes *)*missionFileHand)->canAbort != 0)
            currentMissions[slot].canAbort = 1;
        else
            currentMissions[slot].canAbort = 0;
        n = ((MisnRes *)*missionFileHand)->shipSyst;
        if (n == -1) {
            currentMissions[slot].shipSyst = shipData->curSystem;
        } else if (n == kSystemRandom) {
            currentMissions[slot].shipSyst = SelectMissionSystem(n, shipData->curSystem);
        } else if (n == -3) {
            if (briefMissionInfo[mission].destStellar >= 0
                    && briefMissionInfo[mission].destStellar < kMaxStellars) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].destStellar].system;
            } else if (briefMissionInfo[mission].returnStellar >= 0
                    && briefMissionInfo[mission].returnStellar < kMaxStellars) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].returnStellar].system;
            } else {
                currentMissions[slot].shipSyst = -1;
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a suitable system for special ships (TravelStel doesn't exist!)\n");
            }
        } else if (n == -4) {
            if (briefMissionInfo[mission].returnStellar >= 0
                    && briefMissionInfo[mission].returnStellar < kMaxStellars) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].returnStellar].system;
            } else {
                currentMissions[slot].shipSyst = -1;
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a suitable system for special ships (ReturnStel doesn't exist!)\n");
            }
        } else if (n == -6) {
            currentMissions[slot].shipSyst = -6;
        } else if (n == kSystemAdjacent || n >= kSystemGovt - 1) {
            currentMissions[slot].shipSyst = SelectMissionSystem(n, shipData->curSystem);
        } else if (n >= kFirstResourceID) {
            currentMissions[slot].shipSyst = n - kFirstResourceID;
        } else {
            if (currentMissions[slot].shipCount > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" contained invalid special ship system reference\n");
            }
            currentMissions[slot].shipSyst = -1;
        }
        strncpy(currentMissions[slot].shipName, (char *)"\p", 3);
        strncpy(currentMissions[slot].shipSubtitle, (char *)"\p", 3);
        if (((MisnRes *)*missionFileHand)->shipNameID != -1) {
            currentMissions[slot].destStrList = ((MisnRes *)*missionFileHand)->shipNameID;
            h = GetResource(kStringListType, currentMissions[slot].destStrList);
            if (h != NULL) {
                LockAHandle(h);
#ifdef MACBUILD
                BlockMove(*h, &strCount, sizeof(strCount));
                UnlockAHandle(h);
                HPurge(h);
#else
                strCount = SwapEndianShort((const unsigned short *)*h);
                UnlockAHandle(h);
#endif
                ReleaseResource(h);
                n = Rand(strCount) + 1;
                GetIndString(buf, ((MisnRes *)*missionFileHand)->shipNameID, n);
                strncpy(currentMissions[slot].shipName, (char *)buf, 0x3f);
                currentMissions[slot].destStrIndex = n;
            }
        }
        if (((MisnRes *)*missionFileHand)->shipSubtitle != -1) {
            currentMissions[slot].shipStrList = ((MisnRes *)*missionFileHand)->shipSubtitle;
            h = GetResource(kStringListType, currentMissions[slot].shipStrList);
            if (h != NULL) {
                LockAHandle(h);
#ifdef MACBUILD
                BlockMove(*h, &strCount, sizeof(strCount));
                UnlockAHandle(h);
                HPurge(h);
#else
                strCount = SwapEndianShort((const unsigned short *)*h);
                UnlockAHandle(h);
#endif
                ReleaseResource(h);
                n = Rand(strCount) + 1;
                GetIndString(buf, ((MisnRes *)*missionFileHand)->shipSubtitle, n);
                strncpy(currentMissions[slot].shipSubtitle, (char *)buf, 0x3f);
                currentMissions[slot].shipStrIndex = n;
            }
        }
        strcpy(currentMissions[slot].beginSetString, ((MisnRes *)*missionFileHand)->onAccept);
        strcpy(currentMissions[slot].setString2, ((MisnRes *)*missionFileHand)->onRefuse);
        strcpy(currentMissions[slot].succSetString, ((MisnRes *)*missionFileHand)->onSuccess);
        strcpy(currentMissions[slot].failSetString, ((MisnRes *)*missionFileHand)->onFailure);
        strcpy(currentMissions[slot].abortSetString, ((MisnRes *)*missionFileHand)->onAbort);
        strcpy(currentMissions[slot].shipDoneSetString, ((MisnRes *)*missionFileHand)->onShipDone);
        currentMissions[slot].hasCargo = 0;
        currentMissions[slot].specialShipsKilled = 0;
        currentMissions[slot].specialShipsBoarded = 0;
        currentMissions[slot].specialShipsDisabled = 0;
        currentMissions[slot].specialShipsJumpedOut = 0;
        if (currentMissions[slot].shipStart == 1)
            currentMissions[slot].specialShipsJumpedIn = 0;
        else
            currentMissions[slot].specialShipsJumpedIn = currentMissions[slot].initialShipCount;
        currentMissions[slot].auxShipsSpawned = 0;
        UnlockAHandle(missionFileHand);
#ifdef MACBUILD
        HPurge(missionFileHand);
#endif
        ReleaseResource(missionFileHand);
    }
    currentMissions[slot].misnIndex = mission;
}

/* the capacity gate a mission offer has to clear before it can be accepted.
   Both callers pass the mission's +0x12 and +0x14 counts, but only the second
   is ever read -- both free-space figures are checked against it, and each
   shortfall has its own STR# 2002 line. */

/* DoMissionCargoPickup (mac _DoMissionCargoPickup). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440370
// MAC: _DoMissionCargoPickup i386 0x00098292 ppc 0x000a3f50
Boolean DoMissionCargoPickup(short unused, short needed)
{
    if (needed > 0) {
        if (ShipTotalHolds() < needed) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, 357);
            ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
            RefreshSAT();
#endif
            return 0;
        }
        if (TotalMissionCargoSpace() < needed) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, 358);
            ShowTextDialog(0, 0, 0);
#ifdef MACBUILD
            RefreshSAT();
#endif
            return 0;
        }
    }
    statusRedraw[kRedrawCargo] = 1;
    ResetPlayerPrecalcedValues();
    return 1;
}

/* DoMissionSuccess (mac _DoMissionSuccess). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440410
// MAC: _DoMissionSuccess i386 0x000a03fc ppc 0x000abff8
void DoMissionSuccess(short mission)
{
    short i;

    if (currentMissions[mission].compDesc != -1) {
        ShowTheCursor();
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[mission].compDesc);
        MungeBriefing(0, mission);
        strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
        ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
        RefreshSAT();
#endif
    }
    currentMissionObjectives[mission].active = 0;
    crashlog_mission_bit_set("mission success", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].succSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].succSetString, mission);
    if (currentMissions[mission].datePostIncrement > 0) {
        crashlog_print_mdy(&gameTime);
        crashlog_print(": incrementing date ");
        crashlog_print_num(currentMissions[mission].datePostIncrement);
        crashlog_print(" days for mission ");
        crashlog_print_num(currentMissions[mission].misnIndex + kFirstResourceID);
        crashlog_print(" completion post increment\n");
        for (i = 0; i < currentMissions[mission].datePostIncrement; i++)
            IncrementGameTime();
    }
    if (currentMissions[mission].compGovt >= 0 && currentMissions[mission].compGovt < kMaxGovts) {
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].govt == currentMissions[mission].compGovt)
                playerRecord[i] += currentMissions[mission].compReward;
            else if (systemData[i].govt != -1) {
                if (GovtEnemies(systemData[i].govt, currentMissions[mission].compGovt))
                    playerRecord[i] -= currentMissions[mission].compReward * 0.5;
                else if (GovtAllies(systemData[i].govt, currentMissions[mission].compGovt))
                    playerRecord[i] += currentMissions[mission].compReward * 0.5;
            }
        }
    }
    ApplyMissionPay(currentMissions[mission].payment);
    lastMissionStellar = -1;
}

#define kPayClearGovt   10000   /* pay -(10000 + id): clear the record with govt id */
#define kPayClearAllies 20000
#define kPayClearClass  30000
#define kPctOfCashBase  40000   /* pay -40001..-40099 is N% of the player's cash */

/* The mission payment field doubles as a command channel. Positive values are
   cash; four negative bands instead clear the player's criminal record -- with
   one government, with its allies, with the third relation -- or write a
   percentage off their money. Government ids arrive biased by the resource
   base, which is where the 10128/20128/30128 constants come from. */
/* ApplyMissionPay (mac _ApplyMissionPay). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440750
// MAC: _ApplyMissionPay i386 0x00098326 ppc 0x000a3ffc
void ApplyMissionPay(int32_t amount)
{
    int32_t i;
    int32_t govt;

    if (amount > 0) {
        shipData[0].credits += amount;
        statusRedraw[kRedrawCargo] = 1;
    } else if (amount <= -kPayClearGovt && amount > -kPayClearAllies) {
        govt = abs(amount) - (kPayClearGovt + kFirstResourceID);
        for (i = 0; i < kMaxSystems; i++) {
            if (govt == systemData[i].govt && playerRecord[i] < 0)
                playerRecord[i] = 0;
        }
    } else if (amount <= -kPayClearAllies && amount > -kPayClearClass) {
        govt = abs(amount) - (kPayClearAllies + kFirstResourceID);
        for (i = 0; i < kMaxSystems; i++) {
            if (GovtAllies(systemData[i].govt, govt) && playerRecord[i] < 0)
                playerRecord[i] = 0;
        }
    } else if (amount <= -kPayClearClass && amount > -kPctOfCashBase) {
        govt = abs(amount) - (kPayClearClass + kFirstResourceID);
        for (i = 0; i < kMaxSystems; i++) {
            if (GovtSharedClass(systemData[i].govt, govt) && playerRecord[i] < 0)
                playerRecord[i] = 0;
        }
    } else if (amount <= -(kPctOfCashBase + 1) && amount > -(kPctOfCashBase + 100)) {
#ifdef MACBUILD
        i = abs(amount) - kPctOfCashBase;
#else
        i = abs(amount);
        i -= kPctOfCashBase;
#endif
        shipData[0].credits -= shipData[0].credits * ((float)i * 0.01);
    }
}

/* a failed mission costs legal standing in every system of the offended
   government, then shows its failure desc and aborts itself */
/* DoMissionFailure (mac _DoMissionFailure). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440930
// MAC: _DoMissionFailure i386 0x000a0285 ppc 0x000abe54
void DoMissionFailure(short mission)
{
    short i;

    crashlog_mission_bit_set("mission failure", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].failSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].failSetString, mission);
    if (currentMissions[mission].compGovt != -1) {
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].govt == currentMissions[mission].compGovt)
                playerRecord[i] -= currentMissions[mission].compReward / 2;
        }
    }
    currentMissionObjectives[mission].active = 0;
    if (currentMissions[mission].failDesc != -1) {
        ShowTheCursor();
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[mission].failDesc);
        MungeBriefing(0, mission);
        strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
        ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
        RefreshSAT();
#endif
    }
    AbortMission(mission, 0);
    lastMissionStellar = -1;
}

/* the ships an aborted mission owned are detached and sent away; `evalBits`
   is what a deliberate abort passes to run the mission's abort expression,
   and a failure path passes 0 so that only the failure expression runs */
/* AbortMission (mac _AbortMission). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440aa0
// MAC: _AbortMission i386 0x00099a92 ppc 0x000a57c4
void AbortMission(short mission, Boolean evalBits)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].specialMission == mission) {
            shipData[i].specialMission = -1;
            if (shipData[i].parentShip != -1) {
                shipData[i].aiType = shipClass[shipData[i].type].inherentAI;
                shipData[i].parentShip = -1;
                AIMakeShipLeave(&shipData[i]);
            }
            if (portDialog != NULL)
                shipData[i].active = 0;
        }
    }
    if (evalBits != 0) {
        crashlog_mission_bit_set("aborting mission", currentMissions[mission].name, 1,
                                 currentMissions[mission].misnIndex + kFirstResourceID,
                                 currentMissions[mission].abortSetString);
        EvalCurrentMissionBitSetString(currentMissions[mission].abortSetString, mission);
    }
    currentMissions[mission].hasCargo = 0;
    currentMissionObjectives[mission].active = 0;
    lastMissionStellar = -1;
}

/* QuickMissionFailure (mac _QuickMissionFailure). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440bf0
// MAC: _QuickMissionFailure i386 0x00099d1b ppc 0x000a5ac8
void QuickMissionFailure(short mission)
{
    crashlog_mission_bit_set("quick mission failure", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].failSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].failSetString, mission);
    currentMissionObjectives[mission].failed = 1;
    if (currentMissions[mission].canAbort != 0)
        AbortMission(mission, 0);
    lastMissionStellar = -1;
}

/* the port's scrolling list */

#define kListCaptionIdx  0x167

/* the port's own codes */


/* the port's list entry points */

/* MissionFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440c90
// MAC: _MissionFilter i386 0x0009f7a9 ppc 0x000ab3fc
Boolean MissionFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
    EventKeyByte key;
    Rect box;
    short itemType;
    Point where;
    Point cell;
#ifdef MACBUILD
    Handle item;
#else
    void *item;
#endif
    short old;
    short button;

    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = kItemDone;
        return 1;
    }
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = kItemMap;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = kItemPlayerInfo;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = kItemMissionInfo;
            return 1;
        }
#if V111
        key = (ev->message & kKeyCodeMask) >> 8;
        if ((int)ev->what == kKeyDown) {
            if (key == kReturnKeyCode || key == kEnterKeyCode || key == kEscapeKeyCode) {
                *itemHit = kItemDone;
                return 1;
            }
            if (key == kLKeyCode) {
                *itemHit = kItemDone;
                return 1;
            }
            if (key == kAKeyCode && selMissionListItem != -1) {
                *itemHit = kItemAccept;
                return 1;
            }
        }
        if (key == kTabKeyCode) {
#else
        key = EventCharOf(ev);
        key = tolower(key);
        if ((int)ev->what == kKeyDown) {
            if (IsReturnChar(key) || key == kEscapeChar) {
                *itemHit = kItemDone;
                return 1;
            }
            if (key == 'l') {
                *itemHit = kItemDone;
                return 1;
            }
            if (key == 'a' && selMissionListItem != -1) {
                *itemHit = kItemAccept;
                return 1;
            }
        }
        if (key == kTabChar) {
#endif
            cell.h = 0;
            cell.v = selMissionListItem;
            if (selMissionListItem != -1) {
                LSetSelect(0, cell, missionList);
                GetListCellRect(&box, cell, missionList);
                if (box.bottom >= (*missionList)->rView.bottom)
                    LScroll(0, 1, missionList);
            }
            old = selMissionListItem;
            selMissionListItem++;
            if (dockedPortMissions[missionsOutside][selMissionListItem] == -1)
                selMissionListItem = 0;
            strncpy(briefingTextPtr, "", 3);
            if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                dockedPortMissions[missionsOutside][selMissionListItem]
                                    + kMissionDescBase);
                MungeBriefing(1,
                              dockedPortMissions[missionsOutside][selMissionListItem]);
                cell.v = selMissionListItem;
                LSetSelect(1, cell, missionList);
                LAutoScroll(missionList);
            }
            if (selMissionListItem != old) {
#ifdef MACBUILD
                GetDialogItem(missionDialog, 4, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionDialog), &box);
                GetDialogItem(missionDialog, 5, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionDialog), &box);
#else
                InvalDialogItem(missionDialog, 4);
                InvalDialogItem(missionDialog, 5);
#endif
            }
            cell.v = selMissionListItem;
            LSetSelect(1, cell, missionList);
            LAutoScroll(missionList);
        }
#if V111
        if (key == kDownArrowKeyCode) {
#else
        if (key == kDownArrowChar) {
#endif
            if (selMissionListItem + 1 < kMaxMissions) {
                cell.h = 0;
                cell.v = selMissionListItem;
                if (selMissionListItem != -1) {
                    LSetSelect(0, cell, missionList);
                    GetListCellRect(&box, cell, missionList);
                    if (box.bottom >= (*missionList)->rView.bottom)
                        LScroll(0, 1, missionList);
                }
                old = selMissionListItem;
                selMissionListItem++;
                if (dockedPortMissions[missionsOutside][selMissionListItem] == -1)
                    selMissionListItem = 0;
                strncpy(briefingTextPtr, "", 3);
                if (selMissionListItem != -1) {
                    LoadDescription(briefingTextPtr,
                                    dockedPortMissions[missionsOutside]
                                                      [selMissionListItem]
                                        + kMissionDescBase);
                    MungeBriefing(1,
                                  dockedPortMissions[missionsOutside]
                                                    [selMissionListItem]);
                    cell.v = selMissionListItem;
                    LSetSelect(1, cell, missionList);
                    LAutoScroll(missionList);
                }
                if (selMissionListItem != old) {
#ifdef MACBUILD
                    GetDialogItem(missionDialog, 4, &itemType, &item, &box);
                    InvalWindowRect(FrontWindow(missionDialog), &box);
                    GetDialogItem(missionDialog, 5, &itemType, &item, &box);
                    InvalWindowRect(FrontWindow(missionDialog), &box);
#else
                    InvalDialogItem(missionDialog, 4);
                    InvalDialogItem(missionDialog, 5);
#endif
                }
            }
        }
#if V111
        if (key == kUpArrowKeyCode) {
#else
        if (key == kUpArrowChar) {
#endif
            if (selMissionListItem == -1) {
                selMissionListItem = kMaxMissions - 1;
                while (dockedPortMissions[missionsOutside]
                                         [selMissionListItem - 1] == -1
                       && selMissionListItem > 1)
                    selMissionListItem--;
            }
            if (selMissionListItem != -1) {
                cell.h = 0;
                cell.v = selMissionListItem;
                if (selMissionListItem != -1) {
                    LSetSelect(0, cell, missionList);
                    GetListCellRect(&box, cell, missionList);
                    if (box.top <= (*missionList)->rView.top)
                        LScroll(0, -1, missionList);
                }
                old = selMissionListItem;
                selMissionListItem--;
                if (selMissionListItem < 0) {
                    selMissionListItem = kMaxMissions - 1;
                    while (dockedPortMissions[missionsOutside]
                                             [selMissionListItem] == -1
                           && selMissionListItem > 0)
                        selMissionListItem--;
                } else if (dockedPortMissions[missionsOutside]
                                             [selMissionListItem] == -1) {
                    selMissionListItem = -1;
                }
                strncpy(briefingTextPtr, "", 3);
                if (selMissionListItem != -1) {
                    LoadDescription(briefingTextPtr,
                                    dockedPortMissions[missionsOutside]
                                                      [selMissionListItem]
                                        + kMissionDescBase);
                    MungeBriefing(1,
                                  dockedPortMissions[missionsOutside]
                                                    [selMissionListItem]);
                    cell.v = selMissionListItem;
                    LSetSelect(1, cell, missionList);
                    LAutoScroll(missionList);
                }
                if (selMissionListItem != old) {
#ifdef MACBUILD
                    GetDialogItem(missionDialog, 4, &itemType, &item, &box);
                    InvalWindowRect(FrontWindow(missionDialog), &box);
                    GetDialogItem(missionDialog, 5, &itemType, &item, &box);
                    InvalWindowRect(FrontWindow(missionDialog), &box);
#else
                    InvalDialogItem(missionDialog, 4);
                    InvalDialogItem(missionDialog, 5);
#endif
                }
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
#ifdef MACBUILD
        where = ev->where;
        GlobalToLocal(&where);
        GetDialogItem(missionDialog, 2, &itemType, &item, &box);
        if ((PtInRect)(where, &box)) {
#else
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(missionDialog, 2, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box)) {
#endif
            LClick(where, ev->modifiers, missionList);
            old = selMissionListItem;
            selMissionListItem = -1;
            cell.h = 0;
            for (cell.v = 0; cell.v < (*missionList)->dataBounds.bottom; cell.v++) {
                if (LGetSelect(0, &cell, missionList)) {
                    selMissionListItem = cell.v;
                    break;
                }
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                dockedPortMissions[missionsOutside][selMissionListItem]
                                    + kMissionDescBase);
                MungeBriefing(1,
                              dockedPortMissions[missionsOutside][selMissionListItem]);
            }
            if (selMissionListItem != old) {
#ifdef MACBUILD
                GetDialogItem(missionDialog, 4, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionDialog), &box);
                GetDialogItem(missionDialog, 5, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionDialog), &box);
#else
                InvalDialogItem(missionDialog, 4);
                InvalDialogItem(missionDialog, 5);
#endif
            }
        }
        GetDialogItem(missionDialog, 3, &itemType, (Handle *)&item, &box);
#ifdef MACBUILD
        if ((PtInRect)(where, &box))
#else
        if (PtInRect(where.h, where.v, &box))
#endif
            LClick(where, ev->modifiers, missionList);
        MissionDialogUpdate();
        button = TrackMissionButtons(where);
        if (button == 0) {
            *itemHit = kItemAccept;
            return 1;
        }
        if (button == 1) {
            *itemHit = kItemDone;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        SetDialogGWorld(missionDialog);
#ifdef MACBUILD
        BeginUpdate(FrontWindow(missionDialog));
#endif
        MissionDialogUpdate();
#ifdef MACBUILD
        EndUpdate(FrontWindow(missionDialog));
#endif
    }
    return 0;
}

/* MissionDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00441620
// MAC: _MissionDialogUpdate i386 0x0009f156 ppc 0x000aaf3c
void MissionDialogUpdate(void)
{
#ifdef MACBUILD
    char savedBriefing[0x2000];
    Handle item;
    short itemType;
    Str255 text;
    Rect box;
#else
    char savedBriefing[0x2000];
    void *item;
    short itemType;
    char text[256];
    Rect box;
#endif

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionDialog));
    if (missionBackgroundPic[0] != NULL)
        DrawPicHandle(missionBackgroundPic[0], GetDialogPortRect(missionDialog));
    GetDialogItem(missionDialog, 8, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(&rgbLightGrey);
#ifdef CEFIX /* F12.02 */
        CE_SetPen(box.left, box.top + CE_Scale(12));
#else
        MoveTo(box.left, (box.top + 12));
#endif
        DrawIndString(kGameStrList, kListCaptionIdx);
        ForeColor(blackColor);
    }
    GetDialogItem(missionDialog, 5, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionDialog)))) {
        if (selMissionListItem != -1) {
#ifdef CEFIX /* F23.01 */
            ForeColor(whiteColor);
#else
            ForeColor(blackColor);
#endif
            TextFont(gTimesFont);
            TextSize(18);
            strncpy((char *)text, (char *)missionName[dockedPortMissions[missionsOutside]
                                                               [selMissionListItem]], 0xfa);
#ifdef MACBUILD
            TruncString(box.right - box.left - 1, text, smTruncEnd);
#endif
            PtoCstr((unsigned char *)text);
            strncpy(savedBriefing, briefingTextPtr, kBriefingTextMax);
            strncpy(briefingTextPtr, (char *)text, 0xfa);
            MungeBriefing(1, dockedPortMissions[missionsOutside][selMissionListItem]);
            strncpy((char *)text, briefingTextPtr, 0xfa);
            strncpy(briefingTextPtr, savedBriefing, kBriefingTextMax);
#ifdef CEFIX /* F23.02-03 */
            CE_SetPen(box.left, box.top + CurrentGrafPort->txSize);
            DrawTextRun(text, strlen(text), NULL, 0, 0);
#else
            TETextBox(text, strlen((char *)text), &box, 0);
            InvertRect(&box);
#endif
        } else {
            ForeColor(blackColor);
            PaintRect(&box);
        }
    }
    GetDialogItem(missionDialog, 4, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionDialog)))) {
        if (selMissionListItem != -1) {
            ForeColor(blackColor);
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            TETextBox(briefingTextPtr, strlen(briefingTextPtr), &box, 0);
            InvertRect(&box);
        } else {
            ForeColor(blackColor);
            PaintRect(&box);
        }
    }
    GetDialogItem(missionDialog, 11, &itemType, (Handle *)&item, &box);
    RGBForeColor(&rgbDarkGrey);
    DateToMedString(gameTime, (char *)text);
    CtoPstr((char *)text);
    MoveTo(box.left, box.bottom);
    DrawString((unsigned char *)text);
    ForeColor(blackColor);
    DrawMissionButtons(-1);
    if (missionList != NULL)
        ListUpdate(NULL, missionList);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(missionDialog);
    ForeColor(blackColor);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(missionDialog)),
             GetDialogPortRect(missionDialog), GetDialogPortRect(missionDialog), 0,
             GetWindowVisRgn(FrontWindow(missionDialog)));
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (missionList != NULL) {
        SetDialogGWorld(missionDialog);
        ListUpdate(GetWindowVisRgn(FrontWindow(missionDialog)), missionList);
#ifdef MACBUILD
        DrawControls(FrontWindow(missionDialog));
#endif
    }
}

#define kMissionConds        10

#define kAvailStelSentinel  -32000  /* availStel at or below this is "never" */
#define kAvailLocShip            2  /* availLoc: offered by a ship, not a place */

#define kAvailLinkBase        5000
#define kAvailOwnerBase      10000  /* 10000 + govt; 9999 is the independent govt, -1 */
#define kAvailAlliedBase     15000
#define kAvailGovtBase       20000
#define kAvailEnemyBase      25000
#define kAvailClassBase      30000
#define kAvailNotClassBase   31000

#define kMinFuelForMission 100.0f
#define kLargeShipClass          3

/* mïsn ship-type requirement classes (ResClassBase) */
#define kReqNotShipType     1
#define kReqShipGovt        2
#define kReqNotShipGovt     3

#define kCostFloor          -50000

/* IsMissionAvailable (mac _IsMissionAvailable). STATE: MATCH 100.0%, exe 649 / obj 649
   instrs, byte-identical yes (2507 bytes, 88 relocs masked).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00441b40
// MAC: _IsMissionAvailable i386 0x0009b152 ppc 0x000a7000
Boolean IsMissionAvailable(short mission, Boolean offer)
{
    int want;
    Boolean cond[kMissionConds];
    short n;
    short syst;
    short loc;

    if (missionAvailData[mission].availStel <= kAvailStelSentinel)
        return 0;
    if (missionAvailData[mission].availRandom <= 0)
        return 0;
    if (missionAvailData[mission].availLoc == -1)
        return 0;
    if (missionAvailData[mission].availLoc != kAvailLocShip) {
        if (offeringFromShip != 0)
            return 0;
        if (missionsOutside != 0 && missionAvailData[mission].availLoc == 0)
            return 0;
        if (missionsOutside == 0 && missionAvailData[mission].availLoc != 0)
            return 0;
    } else {
        if (offeringFromShip == 0)
            return 0;
    }

    if (missionAvailData[mission].availStel < kFirstResourceID
        || missionAvailData[mission].availStel >= kFirstResourceID + kMaxStellars) {
        if (missionAvailData[mission].sourceStellar >= kFirstResourceID
            && missionAvailData[mission].sourceStellar < kFirstResourceID + kMaxStellars) {
            syst = stellarData[missionAvailData[mission].sourceStellar - kFirstResourceID].system;
            if (syst < 0 || syst >= kMaxSystems)
                syst = FindSystemFromStellar(
                    missionAvailData[mission].sourceStellar - kFirstResourceID);
            if (syst >= 0 && syst < kMaxSystems) {
                if (FindFirstCoLocatedSystem(syst)
                    == FindFirstCoLocatedSystem(shipData->curSystem))
                    return 0;
            }
        }
        if (missionAvailData[mission].destStellar >= kFirstResourceID
            && missionAvailData[mission].destStellar < kFirstResourceID + kMaxStellars) {
            syst = stellarData[missionAvailData[mission].destStellar - kFirstResourceID].system;
            if (syst < 0 || syst >= kMaxSystems)
                syst = FindSystemFromStellar(
                    missionAvailData[mission].destStellar - kFirstResourceID);
            if (syst >= 0 && syst < kMaxSystems) {
                if (FindFirstCoLocatedSystem(syst)
                    == FindFirstCoLocatedSystem(shipData->curSystem))
                    return 0;
            }
        }
    }

    if ((missionAvailData[mission].flags & kMissionTakesFuel) != 0
        && shipData->fuel < kMinFuelForMission)
        return 0;

    for (n = 0; n < kMissionConds; n++)
        cond[n] = 0;

    loc = missionAvailData[mission].availStel;
    if (loc == -1 || offeringFromShip != 0) {
        cond[0] = 1;
    } else if (loc >= kFirstResourceID && loc < kAvailLinkBase) {
#if V111 /* V111 spelling */
        want = loc - kFirstResourceID;
        if (shipData->targetStellar == want)
#else
        if (shipData->targetStellar == loc - kFirstResourceID)
#endif
            cond[0] = 1;
    } else if (loc >= kAvailLinkBase && loc < kAvailOwnerBase - 1) {
        for (n = 0; n < kNavSlots; n++) {
            if (systemData[shipData->curSystem].links[n]
                == missionAvailData[mission].availStel - kAvailLinkBase) {
                cond[0] = 1;
                break;
            }
        }
    } else if (loc >= kAvailOwnerBase - 1 && loc < kAvailAlliedBase) {
#if V111 /* V111 spelling */
        want = loc - kAvailOwnerBase;
        if (dockedPort->govt == want)
#else
        if (dockedPort->govt == loc - kAvailOwnerBase)
#endif
            cond[0] = 1;
    } else if (loc >= kAvailAlliedBase && loc < kAvailGovtBase) {
        if (dockedPort->govt != -1
            && GovtAllies(loc - kAvailAlliedBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (loc >= kAvailGovtBase && loc < kAvailEnemyBase) {
#if V111 /* V111 spelling */
        want = loc - kAvailGovtBase;
        if (dockedPort->govt != want)
#else
        if (dockedPort->govt != loc - kAvailGovtBase)
#endif
            cond[0] = 1;
    } else if (loc >= kAvailEnemyBase && loc < kAvailClassBase) {
        if (dockedPort->govt != -1
            && GovtEnemies(loc - kAvailEnemyBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (loc >= kAvailClassBase && loc < kAvailNotClassBase) {
        if (GovtSharedClass(loc - kAvailClassBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (loc >= kAvailNotClassBase && loc < 32000) {
        if (dockedPort->govt != -1
            && GovtSharedClass(loc - kAvailClassBase, dockedPort->govt) == 0)
            cond[0] = 1;
    }

    if (offer != 0)
        missionAvailData[mission].avail =
            EvalMissionBitTestString(missionAvailData[mission].availBits);
    if (missionAvailData[mission].avail != 0)
        cond[1] = 1;

    cond[6] = PlayerMeetsRequirements(missionAvailData[mission].requires[0],
                                      missionAvailData[mission].requires[1]);

    if (missionAvailData[mission].availRecord == 0) {
        cond[2] = 1;
    } else if (missionAvailData[mission].availRecord <= kAvailStelSentinel
               && offeringFromShip == 0) {
        if (missionAvailData[mission].availRecord == kAvailStelSentinel
            && stellarData[shipData->targetStellar].dominated != 0) {
            cond[2] = 1;
        } else if (missionAvailData[mission].availRecord == kAvailStelSentinel - 1) {
            for (n = 0; n < kMaxSystems; n++) {
                if (stellarData[n].loaded != 0 && stellarData[n].dominated != 0) {
                    cond[2] = 1;
                    break;
                }
            }
        }
    } else if (missionAvailData[mission].availRecord > 0) {
        if (playerRecord[shipData->curSystem]
            >= missionAvailData[mission].availRecord)
            cond[2] = 1;
    } else if (playerRecord[shipData->curSystem]
               <= missionAvailData[mission].availRecord) {
        cond[2] = 1;
    }

    if (missionAvailData[mission].availRating <= 0)
        cond[3] = 1;
    else if (playerKills >= missionAvailData[mission].availRating)
        cond[3] = 1;

    if (missionAvailData[mission].availRandom >= kDailyRollRange)
        cond[4] = 1;
    else if (missionRandom[mission] <= missionAvailData[mission].availRandom)
        cond[4] = 1;

    if (offeringFromShip != 0) {
        if (missionAvailData[mission].cargoQty <= 0)
            cond[5] = 1;
        else if (TotalMissionCargoSpace() < missionAvailData[mission].cargoQty)
            cond[5] = 0;
        else
            cond[5] = 1;
    } else if ((missionAvailData[mission].flags2 & kMissionNeedsCargoSpace) != 0) {
        if (TotalMissionCargoSpace() < missionAvailData[mission].cargoQty)
            cond[5] = 0;
        else
            cond[5] = 1;
    } else {
        cond[5] = 1;
    }

    if (forceRandom != 0)
        cond[4] = 1;

    if (missionAvailData[mission].availShipType >= kFirstResourceID
        && missionAvailData[mission].availShipType <= kFirstResourceID + kMaxShipTypes) {
        if (shipData->type == missionAvailData[mission].availShipType - kFirstResourceID)
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqNotShipType)
               && missionAvailData[mission].availShipType
                  <= ResClassBase(kReqNotShipType) + kMaxShipTypes) {
        if (shipData->type != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipType))
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqShipGovt)
               && missionAvailData[mission].availShipType <= ResClassBase(kReqShipGovt) + kMaxGovts) {
        if (shipClass[shipData->type].inherentGovt
                == missionAvailData[mission].availShipType - ResClassBase(kReqShipGovt)
            || shipClass[shipData->type].inherentGovtB
                == missionAvailData[mission].availShipType - ResClassBase(kReqShipGovt))
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqNotShipGovt)
               && missionAvailData[mission].availShipType <= ResClassBase(kReqNotShipGovt) + kMaxGovts) {
        if (shipClass[shipData->type].inherentGovt
                != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipGovt)
            && shipClass[shipData->type].inherentGovtB
                != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipGovt))
            cond[7] = 1;
    } else {
        cond[7] = 1;
    }

    cond[8] = 1;
    if ((missionAvailData[mission].flags & kMissionNotForFreighters) != 0
        && shipClass[shipData->type].inherentAI <= kLargeShipClass - 1)
        cond[8] = 0;
    if ((missionAvailData[mission].flags & kMissionNotForWarships) != 0
        && shipClass[shipData->type].inherentAI >= kLargeShipClass)
        cond[8] = 0;

    if (missionAvailData[mission].payment < kCostFloor) {
        if (shipData->credits >= kCostFloor - missionAvailData[mission].payment)
            cond[9] = 1;
        else
            cond[9] = 0;
    } else {
        cond[9] = 1;
    }

    for (n = 0; n < kMissionConds; n++) {
        if (cond[n] == 0)
            return 0;
    }

    if ((missionAvailData[mission].sourceStellar == -2
         || missionAvailData[mission].sourceStellar == -3
         || missionAvailData[mission].sourceStellar > 0)
        && SelectMissionStellar(missionAvailData[mission].sourceStellar,
                                -1, -1, mission) == -1)
        return 0;
    if ((missionAvailData[mission].destStellar == -2
         || missionAvailData[mission].destStellar == -3
         || missionAvailData[mission].destStellar > 0)
        && SelectMissionStellar(missionAvailData[mission].destStellar,
                                -1, -1, mission) == -1)
        return 0;

    for (n = 0; n < kMaxActiveMissions; n++) {
        if (currentMissionObjectives[n].active != 0
            && currentMissions[n].misnIndex == mission)
            return 0;
    }
    return 1;
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* offeronemission -- the mission BRIEFING/offer dialog (win 0x00442510, mac _OfferOneMission). Notes: Research/notes/offeronemission.md */


#define kWideDlogID               0x03fc
#define kNarrowDlogID             0x03f8
#define kWidePictID               0x2150
#define kNarrowPictBase           0x2149
#define kMissionPictCount              3

#define kStrAcceptRefusable           50
#define kStrAcceptOnly                27
#define kStrRefuse                    51
#define kButtonPadding                40
#define kMinButtonWidth              100
#define kMinTextHeight              0x30
#define kTextMargin                   16

#define kItemRefuse                    2
#define kItemBriefText                 3
#define kBriefItemMap                  4
#define kBriefItemPlayerInfo           5
#define kItemDivider                   6
#define kBriefItemMissionInfo          7
#define kItemScrollUp                  9
#define kItemScrollDown               10

#define kBriefScrollStep               10


/* OfferOneMission (mac _OfferOneMission). STATE: MATCH, 1186/1186, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00442510
// MAC: _OfferOneMission i386 0x000a21ac ppc 0x000adfd8
short OfferOneMission(short mission)
#ifdef MAC1010
;
#else
#include "../engine/offeronemission.inc"
#endif

/* MissionHandler (mac _MissionHandler). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00443760
// MAC: _MissionHandler i386 0x0009eda8 ppc 0x000aab58
void MissionHandler(void)
{
    short i;

    for (i = 0; i < kMaxActiveMissions; i++)
        MissionObjectivesCheck(i);
}

/* MissionLandCheck (mac _MissionLandCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00443780
// MAC: _MissionLandCheck i386 0x000a19cc ppc 0x000ad724
void MissionLandCheck(void)
{
    short i;
    short stel;
    Boolean anySuccess;

    anySuccess = 0;
    lastPortMissionLoc = -1;
    stel = shipData[0].targetStellar;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0) {
            MissionObjectivesCheck(i);
            MissionLandCargoCheck(i, stel);
            if (StellarsAreIdentical(currentMissions[i].returnStellar, stel)) {
                if (currentMissions[i].returnStellar == currentMissions[i].travelStellar)
                    currentMissionObjectives[i].returnDone = 1;
                if (currentMissions[i].travelStellar == -1)
                    currentMissionObjectives[i].returnDone = 1;
                if (currentMissionObjectives[i].failed != 0) {
                    DoMissionFailure(i);
                } else {
                    if (currentMissions[i].shipGoal == kMissionGoalEscort && currentMissions[i].specialShipsKilled <= 0)
                        currentMissionObjectives[i].objectivesDone = 1;
                    if (currentMissionObjectives[i].returnDone != 0
                            && currentMissionObjectives[i].objectivesDone != 0) {
                        DoMissionSuccess(i);
                        anySuccess = 1;
                    }
                }
            }
            MissionObjectivesCheck(i);
        }
    }
    PropagateMissionBitEffects();
    if (anySuccess)
        RandomizeMissionData();
}

/* MissionLandCargoCheck (mac _MissionLandCargoCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004438d0
// MAC: _MissionLandCargoCheck i386 0x0009edc5 ppc 0x000aab94
void MissionLandCargoCheck(short slot, short stel)
{
    if (StellarsAreIdentical(currentMissions[slot].travelStellar, stel)) {
        if (currentMissions[slot].pickupMode == 1 && currentMissions[slot].hasCargo == 0) {
            if (DoMissionCargoPickup(currentMissions[slot].cargoType, currentMissions[slot].cargoQty)) {
                currentMissions[slot].hasCargo = 1;
                currentMissionObjectives[slot].returnDone = 1;
                crashlog_mission_bit_set("doing mission cargo pickup",
                                         currentMissions[slot].name, 1,
                                         currentMissions[slot].misnIndex + kFirstResourceID, "");
                if (currentMissions[slot].loadCargoDesc != -1) {
                    ShowTheCursor();
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, currentMissions[slot].loadCargoDesc);
                    MungeBriefing(0, slot);
                    strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                    ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                }
            }
        } else {
            currentMissionObjectives[slot].returnDone = 1;
        }
        if (currentMissions[slot].dropoffMode == 0 && currentMissions[slot].hasCargo != 0) {
            currentMissions[slot].hasCargo = 0;
            crashlog_mission_bit_set("doing mission cargo dropoff",
                                     currentMissions[slot].name, 1,
                                     currentMissions[slot].misnIndex + kFirstResourceID, "");
            if (currentMissions[slot].dropCargoDesc != -1) {
                ShowTheCursor();
                strncpy(briefingTextPtr, "", 3);
                LoadDescription(briefingTextPtr, currentMissions[slot].dropCargoDesc);
                MungeBriefing(0, slot);
                strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                ShowTextDialog(0, 0, 1);
                ResetPlayerPrecalcedValues();
#ifdef MACBUILD
                RefreshSAT();
#endif
            }
            statusRedraw[kRedrawCargo] = 1;
        }
    }
    if (StellarsAreIdentical(currentMissions[slot].returnStellar, stel)) {
        if (currentMissions[slot].dropoffMode == 1 && currentMissions[slot].hasCargo != 0) {
            if (currentMissionObjectives[slot].objectivesDone != 0 || currentMissions[slot].shipGoal == kMissionGoalNone
                    || (currentMissions[slot].shipGoal == kMissionGoalEscort && currentMissions[slot].specialShipsKilled <= 0)) {
                currentMissions[slot].hasCargo = 0;
                crashlog_mission_bit_set("doing mission cargo dropoff",
                                         currentMissions[slot].name, 1,
                                         currentMissions[slot].misnIndex + kFirstResourceID, "");
                if (currentMissions[slot].dropCargoDesc != -1) {
                    ShowTheCursor();
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, currentMissions[slot].dropCargoDesc);
                    MungeBriefing(0, slot);
                    strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                    ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
                    RefreshSAT();
#endif
                }
                statusRedraw[kRedrawCargo] = 1;
            }
        }
    }
}

/* MissionObjectivesCheck (mac _MissionObjectivesCheck). STATE: MATCH 100.0%, exe 477
   / obj 477, 385 exact + 92 reloc-equivalent, 0 mismatched; blind LCS 477 (100.0%), 0 clusters,
   relocregcheck 0 of 92 rows differ. Closed by nesting the chain under an initialShipCount > 0
   guard with a TRAILING else, which queues arm 2's block after the big arm as the exe does.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00443c60
// MAC: _MissionObjectivesCheck i386 0x0009e79e ppc 0x000aa434
void MissionObjectivesCheck(short slot)
{
    Str255 buf;
    Rect vis;
    Rect shipRect;
    Boolean prevDone;
    short i;
    short count;
    short syst;
#ifndef MACBUILD
    Rect *vr;
    Rect *sr;
#endif

    if (currentMissionObjectives[slot].active == 0)
        return;
    if (currentMissions[slot].shipGoal == kMissionGoalNone && currentMissions[slot].shipCount > 0
            && currentMissions[slot].specialShipsJumpedIn <= 0
            && (currentMissions[slot].flags & kMissionAutoAbort) != 0)
        currentMissionObjectives[slot].objectivesDone = 0;
    prevDone = currentMissionObjectives[slot].objectivesDone;
    if (currentMissions[slot].shipGoal == kMissionGoalNone && currentMissions[slot].initialShipCount <= 0) {
        currentMissionObjectives[slot].objectivesDone = 1;
    } else if (currentMissions[slot].initialShipCount > 0) {
        if (currentMissions[slot].shipGoal == kMissionGoalNone) {
            if ((currentMissions[slot].flags & kMissionAutoAbort) != 0) {
                if (currentMissions[slot].specialShipsJumpedIn > 0
                        || currentMissions[slot].shipStart != 1)
                    currentMissionObjectives[slot].objectivesDone = 1;
            } else {
                currentMissionObjectives[slot].objectivesDone = 1;
            }
        } else if (currentMissions[slot].specialShipsJumpedIn >= currentMissions[slot].initialShipCount
                || currentMissions[slot].shipCount == 0) {
            if (currentMissions[slot].shipGoal == kMissionGoalDestroy
                    && currentMissions[slot].specialShipsKilled >= currentMissions[slot].initialShipCount)
                currentMissionObjectives[slot].objectivesDone = 1;
            if (currentMissions[slot].shipGoal == kMissionGoalDisable) {
                if (currentMissions[slot].specialShipsKilled > 0)
                    currentMissionObjectives[slot].failed = 1;
                else if (currentMissions[slot].specialShipsDisabled >= currentMissions[slot].initialShipCount)
                    currentMissionObjectives[slot].objectivesDone = 1;
            }
            if ((currentMissions[slot].shipGoal == kMissionGoalBoard || currentMissions[slot].shipGoal == kMissionGoalRescue)
                    && currentMissions[slot].specialShipsBoarded >= currentMissions[slot].initialShipCount)
                currentMissionObjectives[slot].objectivesDone = 1;
            if (currentMissions[slot].shipGoal == kMissionGoalEscort) {
                if (currentMissions[slot].specialShipsKilled > 0
                        || currentMissions[slot].specialShipsDisabled > 0) {
                    currentMissionObjectives[slot].failed = 1;
                } else {
                    count = 0;
                    for (i = 1; i < kMaxShips; i++) {
                        if (slot == shipData[i].specialMission
                                && shipData[i].active != 0)
                            count++;
                    }
                    if (count == 0)
                        currentMissionObjectives[slot].objectivesDone = 0;
                    else
                        currentMissionObjectives[slot].objectivesDone = 1;
                }
            }
            if (currentMissions[slot].shipGoal == kMissionGoalObserve
                    && currentMissionObjectives[slot].objectivesDone == 0
                    && gPause == 0) {
                syst = currentMissions[slot].shipSyst;
                if (syst >= 0 && syst < kMaxSystems)
                    syst = FindActiveCoLocatedSystem(syst);
                else
                    syst = -1;
                if ((syst == shipData[0].curSystem
                        || currentMissions[slot].shipSyst == -6)
                        && currentMissions[slot].specialShipsJumpedIn >= currentMissions[slot].shipCount) {
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active != 0
                                && slot == shipData[i].specialMission
                                && shipData[i].sprite != 0
                                && ShipVisibleToShip(&shipData[i], shipData) != 0) {
#ifdef MACBUILD
                            if (ShipCanCloak(&shipData[i]) != 0) {
                                vis = gSpriteWorld->windRect;
                                shipRect = shipData[i].sprite->destFrameRect;
                                InsetRect(&vis,
                                          ((shipRect.right - shipRect.left) * 0.75),
                                          ((shipRect.bottom - shipRect.top) * 0.75));
                                if (SectRect(&vis, &shipRect, &shipRect) != 0) {
                                    currentMissionObjectives[slot].objectivesDone = 1;
                                    break;
                                }
                            } else {
                                currentMissionObjectives[slot].objectivesDone = 1;
                                break;
                            }
#else
                            if (ShipCanCloak(&shipData[i]) == 0) {
                                currentMissionObjectives[slot].objectivesDone = 1;
                                break;
                            }
                            vr = &gSpriteWorld->windRect;
                            vis.top = vr->top;
                            vis.left = vr->left;
                            vis.bottom = vr->bottom;
                            vis.right = vr->right;
                            sr = &shipData[i].sprite->destFrameRect;
                            shipRect.top = sr->top;
                            shipRect.left = sr->left;
                            shipRect.bottom = sr->bottom;
                            shipRect.right = sr->right;
                            InsetRect(&vis,
                                      ((shipRect.right - shipRect.left) * 0.75),
                                      ((shipRect.bottom - shipRect.top) * 0.75));
                            if (SectRect(&vis, &shipRect, &shipRect) != 0) {
                                currentMissionObjectives[slot].objectivesDone = 1;
                                break;
                            }
#endif
                        }
                    }
                }
            }
            if (currentMissions[slot].shipGoal == kMissionGoalChaseOff) {
                if (currentMissions[slot].specialShipsKilled + currentMissions[slot].specialShipsJumpedOut
                        >= currentMissions[slot].initialShipCount)
                    currentMissionObjectives[slot].objectivesDone = 1;
                else
                    currentMissionObjectives[slot].objectivesDone = 0;
            }
        } else {
            currentMissionObjectives[slot].objectivesDone = 0;
        }
    } else {
        currentMissionObjectives[slot].objectivesDone = 1;
    }
    if (currentMissionObjectives[slot].failed == 0 && currentMissions[slot].timeLimit <= 0
            && currentMissions[slot].timeLimit > -32000 && portDialog == 0) {
        currentMissionObjectives[slot].failed = 1;
        if ((currentMissionObjectives[slot].flags & kMissionHidden) == 0) {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, 285);
            DisplayComm((PStrChar *)buf, kCommFrames, 0, kCommTextSize, &rgbWhite, 0, 0);
        }
        QuickMissionFailure(slot);
    }
    if (currentMissionObjectives[slot].failed == 0
            && currentMissionObjectives[slot].objectivesDone != 0
            && prevDone != currentMissionObjectives[slot].objectivesDone) {
        if (currentMissions[slot].shipDoneDesc != -1) {
            ShowTheCursor();
            strncpy(briefingTextPtr, "", 3);
            LoadDescription(briefingTextPtr, currentMissions[slot].shipDoneDesc);
            MungeBriefing(0, slot);
            strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
            ShowTextDialog(0, 0, 1);
#ifdef MACBUILD
            RefreshSAT();
#endif
        }
        if (currentMissions[slot].shipGoal != kMissionGoalNone) {
            crashlog_mission_bit_set("mission ship goal complete",
                                     currentMissions[slot].name, 1,
                                     currentMissions[slot].misnIndex + kFirstResourceID,
                                     currentMissions[slot].shipDoneSetString);
            EvalCurrentMissionBitSetString(currentMissions[slot].shipDoneSetString, slot);
        }
        if ((currentMissions[slot].flags & kMissionAutoAbort) != 0)
            AutoAbortMission(slot);
    }
}

/* Mac OS DateTimeRec: 14 bytes, passed to DateToLongString BY VALUE, which is
   why the exe pushes three dwords and one word for it. */

/* the offered-mission summary, stride 0x20 */



#define kBriefingSize   0x2000
#define kNameLen        0x3f    /* strncpy budget for a 64-byte token buffer */
#define kNoRankIndex    341
#define kOwedBase       50000   /* pay below -50000 is a debt of the excess */

/* MungeBriefing (mac _MungeBriefing). STATE: MATCH 100.0%, exe 1523 / obj 1523 instrs,
   byte-identical (masked, 6268 bytes). OPEN: none.
   see notes */
// FUNCTION: EVNOVA 0x004444f0
// MAC: _MungeBriefing i386 0x0009c11d ppc 0x000a81c0
void MungeBriefing(Boolean briefing, short which)
{
    char tokenStr[64];
    char destStelStr[64];
    char destSysStr[64];
    char retStelStr[64];
    char retSysStr[64];
    char cargoStr[64];
    char cargoQtyStr[64];
    char shipMsgStr[64];
    char deadlineStr[64];
    char playerNameStr[64];
    char nickStr[64];
    char playerShipStr[64];
    char personStr[64];
    char shipTypeStr[64];
    char prkStr[64];
    char srkStr[64];
    char payStr[64];
    char rank3Str[64];
    char rank4Str[64];
    char rrkStr[128];
    Str255 numStr;
    short i;
    short sys;
    short bestIdx;
    short bestVal;
    int32_t pay;
    int32_t pct;

    stringHandle = NULL;
    stringHandle = AllocAHandle2(strlen(briefingTextPtr), 0);
    if (stringHandle == NULL)
        return;
#ifdef MACBUILD
    BlockMoveData(briefingTextPtr, *(Handle)stringHandle, GetAHandleSize(stringHandle));
#else
    memmove(*(Handle)stringHandle, briefingTextPtr, GetAHandleSize(stringHandle));
#endif
    for (i = 0; i < kBriefingSize; i++)
        briefingTextPtr[i] = '\0';

    strncpy(destStelStr, "[Error]", 10);
    strncpy(destSysStr, "[Error]", 10);
    strncpy(retStelStr, "[Error]", 10);
    strncpy(retSysStr, "[Error]", 10);
    strncpy(cargoStr, "[Error]", 10);
    strncpy(cargoQtyStr, "[Error]", 10);
    strncpy(shipMsgStr, "[Error]", 10);
    strncpy(deadlineStr, "[Error]", 10);
    strncpy(personStr, "[Error]", 10);
    strncpy(shipTypeStr, "[Error]", 10);
    strncpy(prkStr, "[Error]", 10);
    strncpy(srkStr, "[Error]", 10);
    strncpy(rank3Str, "[Error]", 10);
    strncpy(rank4Str, "[Error]", 10);
    strncpy(rrkStr, "[Error]", 10);
    strncpy(payStr, "[Error]", 10);

    if (briefing != 0) {
        if (which == -1)
            goto tokens;

        if (briefMissionInfo[which].destStellar >= 0 &&
            briefMissionInfo[which].destStellar < kMaxStellars) {
            strncpy(destStelStr,
                    (char *)stellarData[briefMissionInfo[which].destStellar].name, kNameLen);
            PtoCstr((unsigned char *)destStelStr);
        }
        if (briefMissionInfo[which].destSystem >= 0 &&
            briefMissionInfo[which].destSystem < kMaxSystems) {
            sys = FindActiveCoLocatedSystem(briefMissionInfo[which].destSystem);
            if (sys == -1)
                sys = FindFirstCoLocatedSystem(briefMissionInfo[which].destSystem);
            if (sys != -1) {
                strncpy(destSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)destSysStr);
            }
        }
        if (briefMissionInfo[which].returnStellar >= 0 &&
            briefMissionInfo[which].returnStellar < kMaxStellars) {
            strncpy(retStelStr,
                    (char *)stellarData[briefMissionInfo[which].returnStellar].name, kNameLen);
            PtoCstr((unsigned char *)retStelStr);
        }
        if (briefMissionInfo[which].returnSystem >= 0 &&
            briefMissionInfo[which].returnSystem < kMaxSystems) {
            sys = FindActiveCoLocatedSystem(briefMissionInfo[which].returnSystem);
            if (sys == -1)
                sys = FindFirstCoLocatedSystem(briefMissionInfo[which].returnSystem);
            if (sys != -1) {
                strncpy(retSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)retSysStr);
            }
        }

        if (strcmp(destStelStr, "[Error]") == 0) {
            if (strcmp(retStelStr, "[Error]") != 0)
                strcpy(destStelStr, retStelStr);
        }
        if (strcmp(destSysStr, "[Error]") == 0) {
            if (strcmp(retSysStr, "[Error]") != 0)
                strcpy(destSysStr, retSysStr);
        }

        if (briefMissionInfo[which].cargoType >= 0 &&
            briefMissionInfo[which].cargoType < kMaxCargo) {
            strncpy(cargoStr, (char *)lcCargoName[briefMissionInfo[which].cargoType], kNameLen);
            PtoCstr((unsigned char *)cargoStr);
            if (cargoStr[0] == '*') {
                sys = strlen(cargoStr);
                for (i = 0; i < sys; i++)
                    cargoStr[i] = cargoStr[i + 1];
            }
            NumToString(briefMissionInfo[which].cargoUnits, numStr);
            strncpy(cargoQtyStr, (char *)numStr, kNameLen);
            PtoCstr((unsigned char *)cargoQtyStr);
        }

        if (briefMissionInfo[which].dueDate.month != gameTime.month ||
            briefMissionInfo[which].dueDate.day != gameTime.day ||
            briefMissionInfo[which].dueDate.year != gameTime.year)
            DateToLongString(briefMissionInfo[which].dueDate, deadlineStr);

        pay = 0;
        if (briefMissionInfo[which].payment > 0) {
            pay = briefMissionInfo[which].payment;
        } else if (briefMissionInfo[which].payment <= -(kPctOfCashBase + 1) &&
                   briefMissionInfo[which].payment > -(kPctOfCashBase + 100)) {
#ifdef MAC1010
            pay = abs(briefMissionInfo[which].payment) - kPctOfCashBase;
            pay = (shipData[0].credits * (pay * 0.01));
#else
            pct = abs(briefMissionInfo[which].payment);
            pct -= kPctOfCashBase;
            pay = (shipData[0].credits * (pct * 0.01));
#endif
        } else if (briefMissionInfo[which].payment < -kOwedBase) {
            pay = -kOwedBase - briefMissionInfo[which].payment;
        }
        if (pay >= 0) {
            payStr[0] = '\0';
            StrcatCommaNumber(pay, payStr);
        }
    } else {
        if (which == -1)
            goto tokens;
        if (currentMissionObjectives[which].active == 0)
            goto tokens;

        if (currentMissions[which].travelStellar >= 0 &&
            currentMissions[which].travelStellar < kMaxStellars) {
            strncpy(destStelStr,
                    (char *)stellarData[currentMissions[which].travelStellar].name, kNameLen);
            PtoCstr((unsigned char *)destStelStr);
            if (stellarData[currentMissions[which].travelStellar].system >= 0 &&
                stellarData[currentMissions[which].travelStellar].system < kMaxSystems) {
                sys = FindActiveCoLocatedSystem(
                          stellarData[currentMissions[which].travelStellar].system);
                if (sys == -1)
                    sys = FindFirstCoLocatedSystem(
                              stellarData[currentMissions[which].travelStellar].system);
            } else {
                sys = -1;
            }
            if (sys == -1)
                sys = FindSystemFromStellar(currentMissions[which].travelStellar);
            if (sys != -1) {
                strncpy(destSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)destSysStr);
            }
        }
        if (currentMissions[which].returnStellar >= 0 &&
            currentMissions[which].returnStellar < kMaxStellars) {
            strncpy(retStelStr,
                    (char *)stellarData[currentMissions[which].returnStellar].name, kNameLen);
            PtoCstr((unsigned char *)retStelStr);
            if (stellarData[currentMissions[which].returnStellar].system >= 0 &&
                stellarData[currentMissions[which].returnStellar].system < kMaxSystems) {
                sys = FindActiveCoLocatedSystem(
                          stellarData[currentMissions[which].returnStellar].system);
                if (sys == -1)
                    sys = FindFirstCoLocatedSystem(
                              stellarData[currentMissions[which].returnStellar].system);
            } else {
                sys = -1;
            }
            if (sys == -1)
                sys = FindSystemFromStellar(currentMissions[which].returnStellar);
            if (sys != -1) {
                strncpy(retSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)retSysStr);
            }
        }

        if (strcmp(destStelStr, "[Error]") == 0) {
            if (strcmp(retStelStr, "[Error]") != 0)
                strcpy(destStelStr, retStelStr);
        }
        if (strcmp(destSysStr, "[Error]") == 0) {
            if (strcmp(retSysStr, "[Error]") != 0)
                strcpy(destSysStr, retSysStr);
        }

        if (currentMissions[which].cargoType >= 0 &&
            currentMissions[which].cargoType < kMaxCargo) {
            strncpy(cargoStr, (char *)lcCargoName[currentMissions[which].cargoType], kNameLen);
            PtoCstr((unsigned char *)cargoStr);
            if (cargoStr[0] == '*') {
                sys = strlen(cargoStr);
                for (i = 0; i < sys; i++)
                    cargoStr[i] = cargoStr[i + 1];
            }
            NumToString(currentMissions[which].cargoQty, numStr);
            strncpy(cargoQtyStr, (char *)numStr, kNameLen);
            PtoCstr((unsigned char *)cargoQtyStr);
        }

        if (currentMissionObjectives[which].dueDate.month != gameTime.month ||
            currentMissionObjectives[which].dueDate.day != gameTime.day ||
            currentMissionObjectives[which].dueDate.year != gameTime.year)
            DateToLongString(currentMissionObjectives[which].dueDate, deadlineStr);

        strncpy(shipMsgStr, currentMissions[which].shipName, kNameLen);
        PtoCstr((unsigned char *)shipMsgStr);

        if (currentMissions[which].payment > 0) {
            pay = currentMissions[which].payment;
        } else if (currentMissions[which].payment <= -(kPctOfCashBase + 1) &&
                   currentMissions[which].payment > -(kPctOfCashBase + 100)) {
#ifdef MAC1010
            pay = abs(currentMissions[which].payment) - kPctOfCashBase;
            pay = (shipData[0].credits * (pay * 0.01));
#else
            pct = abs(currentMissions[which].payment);
            pct -= kPctOfCashBase;
            pay = (shipData[0].credits * (pct * 0.01));
#endif
        } else if (currentMissions[which].payment < -kOwedBase) {
            pay = -kOwedBase - currentMissions[which].payment;
        }
        if (pay >= 0) {
            payStr[0] = '\0';
            StrcatCommaNumber(pay, payStr);
        }
    }

tokens:
    if (offeringShipIndex >= 0 && offeringShipIndex < kMaxShips &&
        (shipData[offeringShipIndex].persType >= 0 &&
         shipData[offeringShipIndex].persType < kMaxPersons)) {
        strncpy(personStr,
                personData[shipData[offeringShipIndex].persType].name, kNameLen);
        PtoCstr((unsigned char *)personStr);
    }

    strncpy(playerNameStr, (char *)playerName, kNameLen);
    PtoCstr((unsigned char *)playerNameStr);
    if (playerNickname[0] != '\0') {
        strncpy(nickStr, (char *)playerNickname, kNameLen);
        PtoCstr((unsigned char *)nickStr);
    } else {
        strncpy(nickStr, (char *)playerName, kNameLen);
    }
    strncpy(playerShipStr, (char *)shipName, kNameLen);
    PtoCstr((unsigned char *)playerShipStr);
    strncpy(shipTypeStr, (char *)shipClass[shipData[0].type].name, kNameLen);
    PtoCstr((unsigned char *)shipTypeStr);

    bestIdx = -1;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            strlen(rankData[i].convName) != 0 &&
            (rankData[i].priority > bestVal || bestIdx == -1)) {
            bestIdx = i;
            bestVal = rankData[i].priority;
        }
    }
    if (bestIdx != -1) {
        strcpy(prkStr, rankData[bestIdx].convName);
    } else {
        strcpy(prkStr, "");
        StrcatIndCString(prkStr, kGameStrList, kNoRankIndex);
    }

    bestIdx = -1;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            strlen(rankData[i].shortName) != 0 &&
            (rankData[i].priority > bestVal || bestIdx == -1)) {
            bestIdx = i;
            bestVal = rankData[i].priority;
        }
    }
    if (bestIdx != -1) {
        strcpy(srkStr, rankData[bestIdx].shortName);
    } else {
        strcpy(srkStr, "");
        StrcatIndCString(srkStr, kGameStrList, kNoRankIndex);
    }

    bestIdx = -1;
    if (descRankGovt[0] != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                descRankGovt[0] == rankData[i].govt &&
                strlen(rankData[i].shortName) != 0 &&
                (rankData[i].priority > bestVal || bestIdx == -1)) {
                bestIdx = i;
                bestVal = rankData[i].priority;
            }
        }
    }
    if (bestIdx != -1) {
        strcpy(rank3Str, rankData[bestIdx].shortName);
    } else {
        strcpy(rank3Str, "");
        StrcatIndCString(rank3Str, kGameStrList, kNoRankIndex);
    }

    bestIdx = -1;
    if (descRankGovt[1] != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                descRankGovt[1] == rankData[i].govt &&
                strlen(rankData[i].shortName) != 0 &&
                (rankData[i].priority > bestVal || bestIdx == -1)) {
                bestIdx = i;
                bestVal = rankData[i].priority;
            }
        }
    }
    if (bestIdx != -1) {
        strcpy(rank4Str, rankData[bestIdx].shortName);
    } else {
        strcpy(rank4Str, "");
        StrcatIndCString(rank4Str, kGameStrList, kNoRankIndex);
    }

    if (recentActivatedRank != -1) {
        strcpy(rrkStr, rankData[recentActivatedRank].name);
    } else {
        strcpy(rrkStr, "");
        StrcatIndCString(rrkStr, kGameStrList, kNoRankIndex);
    }

    MungeForSymbol("<DST>", destStelStr);
    MungeForSymbol("<DSY>", destSysStr);
    MungeForSymbol("<RST>", retStelStr);
    MungeForSymbol("<RSY>", retSysStr);
    MungeForSymbol("<CT>", cargoStr);
    MungeForSymbol("<CQ>", cargoQtyStr);
    MungeForSymbol("<SN>", shipMsgStr);
    MungeForSymbol("<DL>", deadlineStr);
    MungeForSymbol("<PN>", playerNameStr);
    MungeForSymbol("<PNN>", nickStr);
    MungeForSymbol("<PSN>", playerShipStr);
    MungeForSymbol("<PST>", shipTypeStr);
    MungeForSymbol("<OSN>", personStr);
    MungeForSymbol("<PRK>", prkStr);
    MungeForSymbol("<SRK>", srkStr);
    MungeForSymbol("<RRK>", rrkStr);
    MungeForSymbol("<PAY>", payStr);
    MungeForSymbol("<REG>", RT3_GetDisplayName());

    if (descRankGovt[0] != -1) {
        sprintf(tokenStr, "<PRK%i>", descRankGovt[0] + kFirstResourceID);
        MungeForSymbol(tokenStr, rank4Str);
    }
    if (descRankGovt[1] != -1) {
        sprintf(tokenStr, "<SRK%i>", descRankGovt[1] + kFirstResourceID);
        MungeForSymbol(tokenStr, rank3Str);
    }

    if (leetMode != 0) {
        MungeForSymbol("you", "j00");
        MungeForSymbol("You", "j00");
        MungeForSymbol("ck", "x0r");
        MungeForSymbol("i", "1");
        MungeForSymbol("I", "1");
        MungeForSymbol("e", "3");
        MungeForSymbol("E", "3");
        MungeForSymbol("l", "7");
        MungeForSymbol("L", "7");
    }

#ifdef MACBUILD
    if ((uint32_t)GetAHandleSize(stringHandle) <= kBriefingSize) {
        BlockMoveData(*(Handle)stringHandle, briefingTextPtr, GetAHandleSize(stringHandle));
    } else {
        BlockMoveData(*(Handle)stringHandle, briefingTextPtr, kBriefingSize);
    }
    HPurge(stringHandle);
#else
    if (GetAHandleSize(stringHandle) <= kBriefingSize) {
        memmove(briefingTextPtr, *(Handle)stringHandle, GetAHandleSize(stringHandle));
    } else {
        memmove(briefingTextPtr, *(Handle)stringHandle, kBriefingSize);
    }
#endif
    DeallocAHandle(stringHandle);
    stringHandle = NULL;
}

/* MungeForSymbol. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00445d70
// MAC: _MungeForSymbol i386 0x000984a3 ppc 0x000a4230
void MungeForSymbol(char *needle, void *replacement)
{
    short needleLen;
    short replLen;
    short at;

    needleLen = strlen(needle);
    replLen = strlen(replacement);
    at = 0;
    do {
        at = ReplaceInHandle(stringHandle, at, needle, needleLen, replacement, replLen);
    } while (at >= 0);
}

/* the port's scrolling list */




/* both of these take an argument in this TU that their own bodies ignore */


/* the port's list entry points */


#define kDateCaptionIdx     0x168

/* the port's own codes */

#define kScrollStep   250

/* MissionState fields beyond +0x00, from SetupMissionInfoList (TBC per its
   header): +0x03 gates the bullet prefix, +0x04 bit 0x400 excludes. */


#define kMissionInfoTextSize 0xfa

/* SetupMissionInfoList (mac _SetupMissionInfoList). STATE: MATCH 100.0%, exe 225 / obj 225,
   relocregcheck 0 of 52; the scrollbar test is written <= first. OPEN: none */
// FUNCTION: EVNOVA 0x00445dc0
// MAC: _SetupMissionInfoList i386 0x0009de7a ppc 0x000a9a9c
void SetupMissionInfoList(void)
{
#ifdef MAC1010
    char marker[256];
    char nameBuf[255];
    short i;
    short count;
    int32_t len;
    Rect box;
    Rect cellBounds;
#if TARGET_API_MAC_CARBON
    ListDefSpec ldefSpec;
#endif
    FontInfo fontInfo;
    Handle itemHandle;
    unsigned char hasScrollbar;
    Point cellSize;
    Point rowPt;
    short itemType;
#elif defined(MACBUILD)
    char bullet[2];
    short i;
    short count;
    short itemType;
    Handle itemHandle;
    ListDefSpec ldefSpec;
    FontInfo fontInfo;
    Rect cellBounds;
    Rect box;
    unsigned char hasScrollbar;
    Point cellSize;
    Point rowPt;
#else
    char marker[256];
    char nameBuf[256];
    short i;
    short count;
    short itemType;
    void *itemHandle;
    Rect cellBounds;
    Rect box;
    unsigned char hasScrollbar;
    Point cellSize;
    Point rowPt;
    int32_t len;
#endif

    for (i = 0; i < kMaxActiveMissions; i++)
        activeMissions[i] = -1;

    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0
                && (currentMissionObjectives[i].flags & kMissionHidden) == 0) {
            activeMissions[count] = i;
            count++;
        }
    }

    SetDialogGWorld(missionInfoDialog);
    GetDialogItem(missionInfoDialog, 2, &itemType, (Handle *)&itemHandle, &box);
    SetRect(&cellBounds, 0, 0, 1, count);

    TextFont(smallTextFont);
    TextSize(smallTextSize);

#ifdef MACBUILD
    GetFontInfo(&fontInfo);
    cellSize.v = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
    if (cellSize.v * count <= box.bottom - box.top) {
        hasScrollbar = 0;
    } else {
        box.right -= 15;
        hasScrollbar = 1;
    }
    cellSize.h = box.right - box.left;
#if TARGET_API_MAC_OS8
    missionInfoList = LNew(&box, &cellBounds, cellSize, 128, missionInfoDialog,
                           false, false, false, hasScrollbar);
#else
    ldefSpec.defType = kListDefUserProcType;
    ldefSpec.u.userProc = NewListDefUPP((ListDefProcPtr)MissionListDefinition);
    CreateCustomList(&box, &cellBounds, cellSize, &ldefSpec, GetDialogWindow(missionInfoDialog),
                     true, false, false, hasScrollbar, &missionInfoList);
#endif
    HLock((Handle)missionInfoList);
    (*missionInfoList)->selFlags = lOnlyOne;
    (*missionInfoList)->refCon = (int32_t)ldefColors;
    HUnlock((Handle)missionInfoList);

    rowPt.h = rowPt.v = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0
                || (currentMissionObjectives[i].flags & kMissionHidden) != 0)
            continue;

#ifdef MAC1010
        strncpy(marker, "", 3);
        if (currentMissionObjectives[i].failed != 0)
            strncpy(marker, "\xa5 ", 4);

        strncpy(nameBuf, currentMissions[i].name, kMissionInfoTextSize);
        PtoCstr((unsigned char *)nameBuf);
        strncpy(briefingTextPtr, nameBuf, kMissionInfoTextSize);
        MungeBriefing(0, i);
        strncpy(nameBuf, briefingTextPtr, kMissionInfoTextSize);
        strcat(marker, nameBuf);
        len = strlen(marker);
        LSetCell(marker, len, rowPt, missionInfoList);
#else
        LClrCell(rowPt, missionInfoList);
        if (currentMissionObjectives[i].failed != 0) {
            bullet[0] = '\xa5';
            bullet[1] = ' ';
            LAddToCell(bullet, 2, rowPt, missionInfoList);
        }
        memcpy(briefingTextPtr, &currentMissions[i].name[1], (unsigned char)currentMissions[i].name[0]);
        briefingTextPtr[(unsigned char)currentMissions[i].name[0]] = 0;
        MungeBriefing(0, i);
        LAddToCell(briefingTextPtr, strlen(briefingTextPtr), rowPt, missionInfoList);
#endif
        rowPt.v++;
    }
#else
#ifdef CEFIX /* F12.03 */
    cellSize.v = (ceListItemBaseHeight * 1.5);
#else
    cellSize.v = (smallTextSize * 1.5);
#endif

    if (cellSize.v * count <= box.bottom - box.top) {
        hasScrollbar = 0;
    } else {
        box.right -= 15;
        hasScrollbar = 1;
    }
    cellSize.h = box.right - box.left;
    missionInfoList = nv_NewDialog(&box, &cellBounds, cellSize,
        (ListDrawProcPtr)MissionListDefinition, (CGrafPtr)missionInfoDialog, hasScrollbar);
    LockAHandle((Handle)missionInfoList);
    UnlockAHandle((Handle)missionInfoList);

    rowPt.h = rowPt.v = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0
                || (currentMissionObjectives[i].flags & kMissionHidden) != 0)
            continue;

        strncpy(marker, "", 3);
        if (currentMissionObjectives[i].failed != 0)
            strncpy(marker, "\xa5 ", 4);

        strncpy(nameBuf, currentMissions[i].name, kMissionInfoTextSize);
        PtoCstr((unsigned char *)nameBuf);
        strncpy(briefingTextPtr, nameBuf, kMissionInfoTextSize);
        MungeBriefing(0, i);
        strncpy(nameBuf, briefingTextPtr, kMissionInfoTextSize);
        strcat(marker, nameBuf);
        len = strlen(marker);
        LSetCell(marker, len, rowPt, missionInfoList);
        rowPt.v++;
    }
#endif

    rowPt.h = rowPt.v = 0;
    LSetSelect(1, rowPt, missionInfoList);
    LSetDrawingMode(1, missionInfoList);
    LAutoScroll(missionInfoList);
}

/* --- DoMissionInfoDialog (0x00446150) --- */

/* the stellar record, stride 0x498; only the field the map jump reads */

/* the system record, stride 0x1fc; only the id this scan matches on */

/* the player ship, only the two screen-position fields the map disturbs */


#define kMissionInfoDlogID   0x3f4
#define kMissionInfoPictID   0x2145

/* DoMissionInfoDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00446150
// MAC: _DoMissionInfoDialog i386 0x0009e19b ppc 0x000a9dd4
void DoMissionInfoDialog(void)
{
    Point cell;
    short itemHit;
    short i;
    short count;
    short n;
    short stellar;
    short savedX;
    short savedY;
    Boolean done;
    short halfW;
    short halfH;

    done = 0;
    if (screenFaded != 0)
        return;
    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    missionInfoDialog = NULL;
    missionInfoDialog = GetNewDialog(kMissionInfoDlogID, MissionInfoDialogUpdate,
                                     MissionInfoFilter);
    if (missionInfoDialog == NULL)
        return;
    missionInfoBackgroundPic = GetPicture(kMissionInfoPictID);
    CenterWindow(FrontWindow(missionInfoDialog), gWind);
    ShowWindowNoFlash(FrontWindow(missionInfoDialog));
    SetDialogGWorld(missionInfoDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(missionInfoDialog);
    SetupMissionInfoList();
    selMissionInfoListItem = 0;
    LoadDescription(briefingTextPtr,
                    currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
    MungeBriefing(0, activeMissions[selMissionInfoListItem]);
    showAbortMissionButton =
        currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
    MissionInfoDialogUpdate();
    SelectWindow(FrontWindow(missionInfoDialog));
    BringToFront(FrontWindow(missionInfoDialog));
    do {
        if (gQuit != 0)
            done = 1;
        MyModalDialog(missionInfoDialog, MissionInfoFilter, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 6 && missionDialog == NULL) {
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 0;
            if (selMissionInfoListItem != -1) {
                if ((currentMissions[activeMissions[selMissionInfoListItem]].flags & kMissionBriefingArrow) != 0) {
                    if (currentMissions[activeMissions[selMissionInfoListItem]].travelStellar != -1) {
                        stellar = currentMissions[activeMissions[selMissionInfoListItem]].travelStellar;
                        greenMissionSyst = stellarData[stellar].system;
                    } else if (currentMissions[activeMissions[selMissionInfoListItem]].returnStellar != -1) {
                        stellar = currentMissions[activeMissions[selMissionInfoListItem]].returnStellar;
                        greenMissionSyst = stellarData[stellar].system;
                    }
                }
            }
            savedX = shipData[0].targetStellar;
            savedY = shipData[0].navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            if (missionDialog != NULL) {
                shipData[0].targetStellar = savedX;
                shipData[0].navTargetKind = savedY;
            }
            if (portDialog != NULL) {
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                PortDialogUpdate();
            }
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 1;
            SetDialogGWorld(missionInfoDialog);
            RedrawDialog(missionInfoDialog);
            MissionInfoDialogUpdate();
        }
        if (itemHit == 5) {
            if (selMissionInfoListItem == -1) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                DrawMissionInfoButtons(-1);
            } else {
                if (currentMissions[activeMissions[selMissionInfoListItem]].canAbort != 0) {
                    if ((currentMissions[activeMissions[selMissionInfoListItem]].flags & kMissionAbortPenalty) != 0
                        && currentMissions[activeMissions[selMissionInfoListItem]].compGovt != -1) {
                        for (i = 0; i < kMaxSystems; i++) {
                            if (systemData[i].govt == currentMissions[activeMissions[selMissionInfoListItem]].compGovt)
                                playerRecord[i] -=
                                    currentMissions[activeMissions[selMissionInfoListItem]].compReward * 5;
                        }
                    }
                    AbortMission(activeMissions[selMissionInfoListItem], 1);
                    count = 0;
                    for (i = 0; i < kMaxActiveMissions; i++) {
                        if (currentMissionObjectives[i].active != 0)
                            count++;
                    }
                    if (count > 0) {
                        if (missionInfoList != NULL)
                            LDispose(missionInfoList);
                        SetupMissionInfoList();
                        selMissionInfoListItem = -1;
                        RedrawDialog(missionInfoDialog);
                        cell.h = cell.v = 0;
                        while (cell.v < (*missionInfoList)->dataBounds.bottom) {
                            LSetSelect(0, cell, missionInfoList);
                            cell.v++;
                        }
                        LAutoScroll(missionInfoList);
                        for (i = 0; i < kMaxActiveMissions; i++)
                            activeMissions[i] = -1;
                        n = 0;
                        for (i = 0; i < kMaxActiveMissions; i++) {
                            if (currentMissionObjectives[i].active != 0) {
                                activeMissions[n] = i;
                                n++;
                            }
                        }
                    } else {
                        done = 1;
                    }
                    DrawStatusCargo();
                    SetDialogGWorld(missionInfoDialog);
                }
            }
        }
    } while (done == 0);
    if (missionInfoList != NULL)
        LDispose(missionInfoList);
    halfW = (GetDialogPortRect(missionInfoDialog)->right - GetDialogPortRect(missionInfoDialog)->left) / 2;
    halfH = (GetDialogPortRect(missionInfoDialog)->bottom - GetDialogPortRect(missionInfoDialog)->top) / 2;
    DisposeDialogNoFlash(missionInfoDialog);
    missionInfoDialog = NULL;
    if (missionInfoBackgroundPic != NULL) {
        QDReleaseResource(missionInfoBackgroundPic);
        missionInfoBackgroundPic = NULL;
    }
#ifdef MACBUILD
    RefreshSAT();
#else
    RedrawDialogs(1);
#endif
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
}

/* MissionInfoFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00446770
// MAC: _MissionInfoFilter i386 0x0009d6e5 ppc 0x000a9304
Boolean MissionInfoFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    Point cell;
#ifdef MACBUILD
    Handle item;
#else
    void *item;
#endif
    EventKeyByte c;
    short old;
    short i;
    short count;
    short button;

    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map)) {
            *itemHit = 6;
            return 1;
        }
#if V111
        c = (ev->message & kKeyCodeMask) >> 8;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnKeyCode || c == kEnterKeyCode || c == kEscapeKeyCode
                    || KeyCheck(keyPrefs.missions)) {
                *itemHit = 1;
                return 1;
            }
        }
        if (c == kTabKeyCode || c == kDownArrowKeyCode) {
#else
        c = EventCharOf(ev);
        if ((int)ev->what == kKeyDown) {
            if (IsReturnChar(c) || c == kEscapeChar || KeyCheck(keyPrefs.missions)) {
                *itemHit = 1;
                return 1;
            }
        }
        if (c == kTabChar || c == kDownArrowChar) {
#endif
            cell.h = 0;
            cell.v = selMissionInfoListItem;
            if (selMissionInfoListItem != -1) {
                LSetSelect(0, cell, missionInfoList);
                GetListCellRect(&box, cell, missionInfoList);
                if (box.bottom >= (*missionInfoList)->rView.bottom)
                    LScroll(0, 1, missionInfoList);
            }
            old = selMissionInfoListItem;
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0)
                    count++;
            }
            if (count > 0) {
                do {
                    selMissionInfoListItem++;
                    if (selMissionInfoListItem >= kMaxActiveMissions)
                        selMissionInfoListItem = 0;
                } while (activeMissions[selMissionInfoListItem] == -1);
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionInfoListItem != -1) {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
                MungeBriefing(0, activeMissions[selMissionInfoListItem]);
                cell.v = selMissionInfoListItem;
                LSetSelect(1, cell, missionInfoList);
                LAutoScroll(missionInfoList);
                showAbortMissionButton =
                    currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
            } else {
                showAbortMissionButton = 0;
            }
            if (selMissionInfoListItem != old) {
#ifdef MACBUILD
                GetDialogItem(missionInfoDialog, 4, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionInfoDialog), &box);
                GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionInfoDialog), &box);
#else
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
#endif
            }
        }
#if V111
        if (c == kUpArrowKeyCode) {
#else
        if (c == kUpArrowChar) {
#endif
            cell.h = 0;
            cell.v = selMissionInfoListItem;
            if (selMissionInfoListItem != -1) {
                LSetSelect(0, cell, missionInfoList);
                GetListCellRect(&box, cell, missionInfoList);
                if (box.top <= (*missionInfoList)->rView.top)
                    LScroll(0, -1, missionInfoList);
            }
            old = selMissionInfoListItem;
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0)
                    count++;
            }
            if (count > 0) {
                do {
                    selMissionInfoListItem--;
                    if (selMissionInfoListItem < 0)
                        selMissionInfoListItem = kMaxActiveMissions - 1;
                } while (activeMissions[selMissionInfoListItem] == -1);
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionInfoListItem != -1) {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
                MungeBriefing(0, activeMissions[selMissionInfoListItem]);
                cell.v = selMissionInfoListItem;
                LSetSelect(1, cell, missionInfoList);
                LAutoScroll(missionInfoList);
                showAbortMissionButton =
                    currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
            } else {
                showAbortMissionButton = 0;
            }
            if (selMissionInfoListItem != old) {
#ifdef MACBUILD
                GetDialogItem(missionInfoDialog, 4, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionInfoDialog), &box);
                GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionInfoDialog), &box);
#else
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
#endif
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
#ifdef MACBUILD
        where = ev->where;
        GlobalToLocal(&where);
        GetDialogItem(missionInfoDialog, 2, &itemType, &item, &box);
        if ((PtInRect)(where, &box)) {
#else
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(missionInfoDialog, 2, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box)) {
#endif
            LClick(where, ev->modifiers, missionInfoList);
            old = selMissionInfoListItem;
            selMissionInfoListItem = -1;
            cell.h = 0;
            cell.v = 0;
            for (; cell.v < (*missionInfoList)->dataBounds.bottom; cell.v++) {
                if (LGetSelect(0, &cell, missionInfoList)) {
                    selMissionInfoListItem = cell.v;
                    break;
                }
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionInfoListItem != -1) {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
                MungeBriefing(0, activeMissions[selMissionInfoListItem]);
                showAbortMissionButton =
                    currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
            } else {
                showAbortMissionButton = 0;
            }
            if (selMissionInfoListItem != old) {
#ifdef MACBUILD
                GetDialogItem(missionInfoDialog, 4, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionInfoDialog), &box);
                GetDialogItem(missionInfoDialog, 5, &itemType, &item, &box);
                InvalWindowRect(FrontWindow(missionInfoDialog), &box);
#else
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
#endif
            }
        }
        MissionInfoDialogUpdate();
        button = TrackMissionInfoButtons(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 5;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        SetDialogGWorld(missionInfoDialog);
#ifdef MACBUILD
        BeginUpdate(FrontWindow(missionInfoDialog));
#endif
        MissionInfoDialogUpdate();
#ifdef MACBUILD
        EndUpdate(FrontWindow(missionInfoDialog));
#endif
    }
    return 0;
}

/* MissionInfoDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00446e00
// MAC: _MissionInfoDialogUpdate i386 0x00098508 ppc 0x000a42a0
void MissionInfoDialogUpdate(void)
{
#ifdef MACBUILD
    short itemType;
    Handle item;
    char text[256];
    Rect box;
#else
    short itemType;
    void *item;
    char text[256];
    Rect box;
#endif

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionInfoDialog));
    if (missionInfoBackgroundPic != NULL)
        DrawPicHandle(missionInfoBackgroundPic, GetDialogPortRect(missionInfoDialog));
    ForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 3, &itemType, (Handle *)&item, &box);
    RGBForeColor(&rgbMedGrey);
    MoveTo(box.left, box.bottom);
    DrawIndString(kGameStrList, kDateCaptionIdx);
    ForeColor(blackColor);
    RGBForeColor(&rgbDarkGrey);
    DateToMedString(gameTime, text);
    CtoPstr(text);
    GetDialogItem(missionInfoDialog, 7, &itemType, (Handle *)&item, &box);
#ifdef CEFIX /* F12.04 */
    CE_SetPen((box.right + box.left) / 2 - StringWidth((unsigned char *)text) / 2,
              box.top + smallTextSize - 9 + CE_Scale(9));
#else
    MoveTo(((box.right + box.left) / 2 -
                   StringWidth((unsigned char *)text) / 2),
           (box.top + smallTextSize));
#endif
    DrawString((unsigned char *)text);
    ForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 4, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionInfoDialog)))) {
        if (selMissionInfoListItem != -1) {
            ForeColor(blackColor);
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            TETextBox(briefingTextPtr, strlen(briefingTextPtr), &box, 0);
            InvertRect(&box);
        } else {
            ForeColor(blackColor);
            PaintRect(&box);
        }
    }
    DrawMissionInfoButtons(-1);
    if (missionInfoList != NULL)
        ListUpdate(NULL, missionInfoList);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(missionInfoDialog);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(missionInfoDialog)),
             GetDialogPortRect(missionInfoDialog),
             GetDialogPortRect(missionInfoDialog), 0,
             GetWindowVisRgn(FrontWindow(missionInfoDialog)));
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    if (missionInfoList != NULL) {
        SetDialogGWorld(missionInfoDialog);
        ListUpdate(GetWindowVisRgn(FrontWindow(missionInfoDialog)), missionInfoList);
    }
}

#ifdef MAC1010
short OfferOneMission(short mission)
#include "../engine/offeronemission.inc"
#endif

/* SingleMissionFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447170
// MAC: _SingleMissionFilter i386 0x00098d9a ppc 0x000a4984
Boolean SingleMissionFilter(int32_t unused, EventRecord *ev, short *itemHit)
{
#if V111
    Rect box;
    short itemType;
    Point where;
#ifdef MACBUILD
    Handle item;
#else
    void *item;
#endif
    short button;
    char c;
#else
    Rect box;
    short itemType;
    Point where;
    void *item;
    short button;
#ifdef MAC1010 /* 1.0.10 spelling */
    int acceptKey;
    int refuseKey;
    EventChar c;
#else
    EventChar c;
    int what;
    int acceptKey;
    int refuseKey;
#endif
#endif

    QuickKeyCheckHandler();
    if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
        *itemHit = kBriefItemMap;
        return 1;
    }
    if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
        *itemHit = kBriefItemPlayerInfo;
        return 1;
    }
    if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
        *itemHit = kBriefItemMissionInfo;
        return 1;
    }
#if V111
    if (ev->what == kKeyDown || ev->what == kAutoKey) {
        c = tolower((char)EventCharOf(ev));
        if (ev->what == kKeyDown) {
            if (c == kReturnChar || c == kEnterChar) {
                *itemHit = kItemAccept;
                return 1;
            }
            if (tolower(missionButtonLabel[0][1]) != tolower(missionButtonLabel[1][1])) {
                if (c == tolower(missionButtonLabel[0][1])) {
                    *itemHit = kItemAccept;
                    return 1;
                }
                if (canRefuseMission != 0 && c == tolower(missionButtonLabel[1][1])) {
                    *itemHit = kItemRefuse;
                    return 1;
                }
            }
        }
#else
#ifdef MAC1010 /* 1.0.10 spelling */
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        c = EventCharOf(ev);
        c = tolower(c);
        if ((int)ev->what == kKeyDown) {
#else
    what = ev->what;
    if (what == kKeyDown || what == kAutoKey) {
        c = tolower(EventCharOf(ev));
        if (what == kKeyDown) {
#endif
            if (IsReturnChar(c)) {
                *itemHit = kItemAccept;
                return 1;
            }
            refuseKey = missionButtonLabel[1][1];
#ifdef MAC1010 /* 1.0.10 spelling */
            if (tolower(acceptKey = missionButtonLabel[0][1]) != tolower(refuseKey)) {
#else
            acceptKey = missionButtonLabel[0][1];
            if (tolower(acceptKey) != tolower(refuseKey)) {
#endif
                if (c == tolower(acceptKey)) {
                    *itemHit = kItemAccept;
                    return 1;
                }
                if (canRefuseMission != 0 && c == tolower(refuseKey)) {
                    *itemHit = kItemRefuse;
                    return 1;
                }
            }
        }
#endif
        if (canScrollTextUp != 0) {
            if (c == kUpArrowChar) {
                *itemHit = kItemScrollUp;
                return 1;
            }
            if (c == kHomeChar) {
                ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle, kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextUp = 0;
                return 1;
            }
            if (c == kPageUpChar) {
                ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle, kScrollStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
        if (canScrollTextDown != 0) {
            if (c == kDownArrowChar) {
                *itemHit = kItemScrollDown;
                return 1;
            }
            if (c == kEndChar) {
                ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle, -kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextDown = 0;
                return 1;
            }
            if (c == kPageDownChar) {
                ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle, -kScrollStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
    }
#if V111
    if (ev->what == kMouseDown) {
        where = ev->where;
#else
#ifdef MAC1010 /* 1.0.10 spelling */
    if ((int)ev->what == kMouseDown) {
#else
    if (what == kMouseDown) {
#endif
#ifdef MACBUILD
        where = ev->where;
#else
        where.v = ev->where.v;
        where.h = ev->where.h;
#endif
#endif
#ifdef MACBUILD
        GlobalToLocal(&where);
#endif
        if (canScrollTextUp != 0) {
            GetDialogItem(missionDialog, kItemScrollUp, &itemType, (Handle *)&item, &box);
#ifdef MACBUILD
            if ((PtInRect)(where, &box) && canScrollTextUp != 0) {
#else
            if (PtInRect(where.h, where.v, &box) && canScrollTextUp != 0) {
#endif
                lastTextScrollTime = ReadTick();
                while (StillDown()) {
                    BusyIdle();
                    ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle, 1, 1);
                    SetDialogGWorld(missionDialog);
                    DrawSingleMissionButtons(2);
                    while (lastTextScrollTime == ReadTick() && StillDown())
                        BusyIdle();
                }
                SetDialogGWorld(missionDialog);
                DrawSingleMissionButtons(-1);
                *itemHit = -1;
                return 1;
            }
        }
        if (canScrollTextDown != 0) {
            GetDialogItem(missionDialog, kItemScrollDown, &itemType, (Handle *)&item, &box);
#ifdef MACBUILD
            if ((PtInRect)(where, &box) && canScrollTextDown != 0) {
#else
            if (PtInRect(where.h, where.v, &box) && canScrollTextDown != 0) {
#endif
                lastTextScrollTime = ReadTick();
                while (StillDown()) {
                    BusyIdle();
                    ScrollDialogText(missionDialog, kItemBriefText, offerTextHandle, -1, 1);
                    SetDialogGWorld(missionDialog);
                    DrawSingleMissionButtons(3);
                    while (lastTextScrollTime == ReadTick() && StillDown())
                        BusyIdle();
                }
                SetDialogGWorld(missionDialog);
                DrawSingleMissionButtons(-1);
                *itemHit = -1;
                return 1;
            }
        }
        SingleMissionDialogUpdate();
        button = TrackSingleMissionButtons(where);
        if (button == 0) {
            *itemHit = kItemAccept;
            return 1;
        }
        if (button == 1) {
            *itemHit = kItemRefuse;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
#if V111
    if (ev->what == kUpdateEvt) {
#else
#ifdef MAC1010 /* 1.0.10 spelling */
    if ((int)ev->what == kUpdateEvt) {
#else
    if (what == kUpdateEvt) {
#endif
#endif
        SetDialogGWorld(missionDialog);
#ifdef MACBUILD
        BeginUpdate(FrontWindow(missionDialog));
#endif
        SingleMissionDialogUpdate();
#ifdef MACBUILD
        EndUpdate(FrontWindow(missionDialog));
#endif
    }
    return 0;
}
/* SingleMissionDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447680
// MAC: _SingleMissionDialogUpdate i386 0x00098950 ppc 0x000a45e0
void SingleMissionDialogUpdate(void)
{
    Boolean inside;
    void *loaded;
    void *pic;
    Rect r;
    Rect r2;
    short itemType;
    void *item;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionDialog));
    if (descResourcePictId < kFirstResourceID) {
        pic = missionBackgroundPic[1];
        if (pic != NULL) {
            QDGetPictureBounds(pic, &r);
            OffsetRect(&r, -r.left, -r.top);
            if (missionBackgroundPic[0] != NULL) {
                QDGetPictureBounds(missionBackgroundPic[0], &r2);
                OffsetRect(&r2, -r2.left, -r2.top);
                OffsetRect(&r, 0, r2.bottom);
            }
#ifdef CEFIX /* F12.05 */
            CE_ScaleOffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                               GetDialogPortRect(missionDialog)->top);
#else
            OffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                       GetDialogPortRect(missionDialog)->top);
#endif
            DrawPicHandle(missionBackgroundPic[1], &r);
        }
        if (missionBackgroundPic[0] != NULL) {
            QDGetPictureBounds(missionBackgroundPic[0], &r);
            OffsetRect(&r, -r.left, -r.top);
#ifdef CEFIX /* F12.06 */
            CE_ScaleOffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                               GetDialogPortRect(missionDialog)->top);
#else
            OffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                       GetDialogPortRect(missionDialog)->top);
#endif
            DrawPicHandle(missionBackgroundPic[0], &r);
        }
        pic = missionBackgroundPic[2];
        if (pic != NULL) {
            QDGetPictureBounds(pic, &r);
            OffsetRect(&r, -r.left, -r.top);
#ifdef CEFIX /* F12.07 */
            CE_ScaleOffsetRectBottom(&r, GetDialogPortRect(missionDialog)->left,
                                     GetDialogPortRect(missionDialog)->bottom - (r.bottom - r.top));
#else
            OffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                       GetDialogPortRect(missionDialog)->bottom - (r.bottom - r.top));
#endif
            DrawPicHandle(missionBackgroundPic[2], &r);
        }
    } else if (missionBackgroundPic[0] != NULL) {
        DrawPicHandle(missionBackgroundPic[0], GetDialogPortRect(missionDialog));
    }
    DrawSingleMissionButtons(-1);
    if (descResourcePictId >= kFirstResourceID) {
        GetDialogItem(missionDialog, 8, &itemType, (Handle *)&item, &r);
        inside = RectInRgn(&r, GetWindowVisRgn(FrontWindow(missionDialog)));
        if (inside) {
            loaded = GetPicture(descResourcePictId);
            if (loaded != NULL) {
                DrawPicHandle(loaded, &r);
                QDReleaseResource(loaded);
            }
        }
    }
    (*offerTextHandle)->inPort = (GrafPtr)loadPort.world;
    GetDialogItem(missionDialog, kItemBriefText, &itemType, (Handle *)&item, &r);
    ForeColor(whiteColor);
    BackColor(blackColor);
    TEUpdate(&r, offerTextHandle);
    ForeColor(blackColor);
    BackColor(whiteColor);
    (*offerTextHandle)->inPort = (GrafPtr)GetDialogPort(missionDialog);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(missionDialog);
    CopyBits(GWorldContainerBits(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(missionDialog)),
             GetDialogPortRect(missionDialog), GetDialogPortRect(missionDialog), 0,
             GetWindowVisRgn(FrontWindow(missionDialog)));
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* validauxsys -- may a mission's auxiliary ships appear in this system? (win 0x00447a30, mac twin _ValidAuxShipSystem @ 0x000992b5). Notes: Research/notes/validauxsys.md */

/* mïsn runtime record window, stride 0x8e6; byte-packed, since the exe reads
   a word at the odd offset 0x65 (mission.c's copy carries the full layout) */

/* only the player slot is read, so no stride is needed here */

/* ValidAuxShipSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447a30
// MAC: _ValidAuxShipSystem i386 0x000992b5 ppc 0x000a4e60
Boolean ValidAuxShipSystem(short sys, short mission)
{
    short spec;
    short i;
    int32_t xenophobic;

    spec = currentMissions[mission].auxShipSyst;
    if (spec == -1 || spec == -6) {
        if (sys == shipData->curSystem)
            return 1;
    } else if (spec == -2) {
        if (currentMissions[mission].travelStellar != -1) {
            if (sys == stellarData[currentMissions[mission].travelStellar].system)
                return 1;
        }
    } else if (spec == -3) {
        if (currentMissions[mission].returnStellar != -1) {
            if (sys == stellarData[currentMissions[mission].returnStellar].system)
                return 1;
        }
    } else if (spec >= kFirstResourceID && spec < kFirstResourceID + kMaxSystems) {
        if (sys == spec - kFirstResourceID)
            return 1;
        if (sys == FindActiveCoLocatedSystem(spec - kFirstResourceID))
            return 1;
    } else if (spec >= kAvailLinkBase && spec < kSystemGovt - 1) {
        if (sys == spec - kAvailLinkBase)
            return 1;
        for (i = 0; i < kNavSlots; i++) {
            if (sys == systemData[spec - kAvailLinkBase].links[i])
                return 1;
        }
    } else if (spec >= kSystemGovt - 1 && spec < kSystemAllied) {
        if (systemData[sys].govt == spec - kSystemGovt)
            return 1;
    } else if (spec >= kSystemAllied && spec < kSystemNotGovt) {
        if (systemData[sys].govt >= 0) {
            if (systemData[sys].govt == spec - kSystemAllied)
                return 1;
            if (GovtAllies(systemData[sys].govt, spec - kSystemAllied))
                return 1;
        }
    } else if (spec >= kSystemNotGovt && spec < kSystemEnemy) {
        if (systemData[sys].govt != spec - kSystemNotGovt)
            return 1;
    } else if (spec >= kSystemEnemy && spec < kSystemClass) {
        if (systemData[sys].govt >= 0) {
#ifdef MAC1010 /* 1.0.10 spelling */
            if ((govtData[spec - kSystemEnemy].flags & kGovtXenophobic) && systemData[sys].govt != spec - kSystemEnemy &&
#else
            xenophobic = govtData[spec - kSystemEnemy].flags & kGovtXenophobic;
            if (xenophobic && systemData[sys].govt != spec - kSystemEnemy &&
#endif
                !GovtAllies(systemData[sys].govt, spec - kSystemEnemy))
                return 1;
            if (GovtEnemies(systemData[sys].govt, spec - kSystemEnemy))
                return 1;
        }
    } else if (spec >= kSystemClass && spec < kSystemNotClass) {
        if (systemData[sys].govt >= 0) {
            if (GovtSharedClass(systemData[sys].govt, spec - kSystemClass))
                return 1;
        }
    } else if (spec >= kSystemNotClass && spec < kSystemBandLimit) {
        if (systemData[sys].govt >= 0) {
            if (!GovtSharedClass(systemData[sys].govt, spec - kSystemNotClass))
                return 1;
        }
    }
    return 0;
}

/* AutoAbortMission (mac _AutoAbortMission). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447d90
// MAC: _AutoAbortMission i386 0x00099bdc ppc 0x000a5954
void AutoAbortMission(short mission)
{
    short i;

    crashlog_mission_bit_set("auto-aborting mission", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].abortSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].abortSetString, mission);
    if (currentMissions[mission].datePostIncrement > 0) {
        crashlog_print_mdy(&gameTime);
        crashlog_print(": incrementing date ");
        crashlog_print_num(currentMissions[mission].datePostIncrement);
        crashlog_print(" days for mission ");
        crashlog_print_num(currentMissions[mission].misnIndex + kFirstResourceID);
        crashlog_print(" auto-abort post increment\n");
        for (i = 0; i < currentMissions[mission].datePostIncrement; i++)
            IncrementGameTime();
    }
    if (currentMissions[mission].flags & kMissionTakesFuel) {
        shipData[0].fuel -= 100.0f;
        statusRedraw[kRedrawFuel] = 1;
    }
    if (currentMissions[mission].flags2 & kMissionPaysOnAbort)
        ApplyMissionPay(currentMissions[mission].payment);
    AbortMission(mission, 0);
}

/* SystemVisible. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447f00
// MAC: _SystemVisible i386 0x00098207 ppc 0x000a3ea8
Boolean SystemVisible(short sys)
{
    return systemData[sys].loaded;
}

/* EvalMissionBitTestString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447f20
// MAC: _EvalMissionBitTestString i386 0x0009959e ppc 0x000a5160
Boolean EvalMissionBitTestString(char *expr)
{
    short i;
    short out;
    short len;
    char prev;
    short err;

    if (strlen(expr) == 0)
        return 1;
    if (expr[0] != 'b' && expr[0] != 'B' && expr[0] != '(' && expr[0] != '!'
            && expr[0] != 'P' && expr[0] != 'p' && expr[0] != 'G' && expr[0] != 'g'
            && expr[0] != 'O' && expr[0] != 'o' && expr[0] != 'E' && expr[0] != 'e')
        return 0;
    len = strlen(expr);
    prev = ' ';
    out = 0;
    for (i = 0; i < len; i++) {
        if (expr[i] == '(' && prev == '(') {
            parseBuffer[out] = ' ';
            out++;
        }
        parseBuffer[out] = expr[i];
        prev = expr[i];
        out++;
    }
    parseBuffer[out] = 0;
    err = 0;
#ifdef MAC1010 /* 1.0.10 spelling */
    return (EvalTestExp(&err) == 1) ? true : false;
#else
    return EvalTestExp(&err) == 1 ? 1 : 0;
#endif
}
