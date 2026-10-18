// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
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
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "prototypes.h"

/* InitMissions (mac _InitMissions). STATE: MATCH 100.0%, exe 509 / obj 509
   instrs, byte-identical yes. OPEN: nothing. */
// FUNCTION: EVNOVA 0x0043bbb0
void InitMissions(void)
{
    short i;
    short j;
    short resID;
    unsigned long resType;
    short numLoaded;
    short numMisns;
    unsigned char name[256];

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
        for (i = 0; i < 16; i++) {
            currentMissionObjectives[i].active = 0;
            currentMissionObjectives[i].failed = 0;
        }
        for (j = 0; j < 10000; j++)
            missionBit[j] = 0;
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
        missionFileHand = GetResource(kMisnType, i + 128);
        if (missionFileHand != NULL) {
            numLoaded++;
            missionAvailData[i].availStel = *(short *)(*missionFileHand + 0x00);
            missionAvailData[i].availLoc = *(short *)(*missionFileHand + 0x04);
            missionAvailData[i].availRecord = *(short *)(*missionFileHand + 0x06);
            missionAvailData[i].availRating = *(short *)(*missionFileHand + 0x08);
            missionAvailData[i].availRandom = *(short *)(*missionFileHand + 0x0a);
            missionAvailData[i].cargoQty = *(short *)(*missionFileHand + 0x12);
            missionAvailData[i].cargoType = *(short *)(*missionFileHand + 0x10);
            missionAvailData[i].sourceStellar = *(short *)(*missionFileHand + 0x0c);
            missionAvailData[i].destStellar = *(short *)(*missionFileHand + 0x0e);
            missionAvailData[i].timeLimit = *(short *)(*missionFileHand + 0x40);
            missionAvailData[i].flags = *(short *)(*missionFileHand + 0x50);
            missionAvailData[i].flags2 = *(short *)(*missionFileHand + 0x52);
            missionAvailData[i].availShipType = *(short *)(*missionFileHand + 0x5a);
            missionAvailData[i].requires[0] = *(long *)(*missionFileHand + 0x656);
            missionAvailData[i].requires[1] = *(long *)(*missionFileHand + 0x65a);
            missionAvailData[i].payment = *(long *)(*missionFileHand + 0x1c);
            missionAvailData[i].dispWeight = *(short *)(*missionFileHand + 0x7a0);
            if (missionAvailData[i].availStel < -1) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + 128);
                crashlog_print(" has invalid AvailStel value\n");
                missionAvailData[i].availStel = -1;
            }
            if (*(short *)(*missionFileHand + 0x24) >= 128
                    && *(short *)(*missionFileHand + 0x24) < 640) {
                if (dudeData[*(short *)(*missionFileHand + 0x24) - 128].loaded == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + 128);
                    crashlog_print(" makes reference to nonexistent d\x9f" "de resource ");
                    crashlog_print_num(*(short *)(*missionFileHand + 0x24));
                    crashlog_print("\n");
                }
            } else if (*(short *)(*missionFileHand + 0x20) > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + 128);
                crashlog_print(" makes reference to nonexistent d\x9f" "de resource ");
                crashlog_print_num(*(short *)(*missionFileHand + 0x24));
                crashlog_print("\n");
            }
            if (*(short *)(*missionFileHand + 0x4a) >= 128
                    && *(short *)(*missionFileHand + 0x4a) < 640) {
                if (dudeData[*(short *)(*missionFileHand + 0x4a) - 128].loaded == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + 128);
                    crashlog_print(" makes reference to nonexistent d\x9f" "de resource ");
                    crashlog_print_num(*(short *)(*missionFileHand + 0x4a));
                    crashlog_print("\n");
                }
            } else if (*(short *)(*missionFileHand + 0x48) > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + 128);
                crashlog_print(" makes reference to nonexistent d\x9f" "de resource ");
                crashlog_print_num(*(short *)(*missionFileHand + 0x4a));
                crashlog_print("\n");
            }
            if (*(short *)(*missionFileHand + 0x0c) >= 128
                    && *(short *)(*missionFileHand + 0x0c) < 2176) {
                if (stellarData[*(short *)(*missionFileHand + 0x0c) - 128].prepared == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + 128);
                    crashlog_print(" makes reference to nonexistent sp\x9a" "b resource ");
                    crashlog_print_num(*(short *)(*missionFileHand + 0x0c));
                    crashlog_print("\n");
                }
            }
            if (*(short *)(*missionFileHand + 0x0e) >= 128
                    && *(short *)(*missionFileHand + 0x0e) < 2176) {
                if (stellarData[*(short *)(*missionFileHand + 0x0e) - 128].prepared == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + 128);
                    crashlog_print(" makes reference to nonexistent sp\x9a" "b resource ");
                    crashlog_print_num(*(short *)(*missionFileHand + 0x0e));
                    crashlog_print("\n");
                }
            }
            strcpy(missionAvailData[i].availBits, *missionFileHand + 0x5c);
            AdjustHandleSize(missionFileHand, kMisnResSize);
            if (missionAvailData[i].availLoc < 0)
                missionAvailData[i].availLoc = 0;
            GetResInfo(missionFileHand, &resID, &resType, name);
            StripNameComment(name);
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
void DoMissionDialog(void)
{
    char accepted;
    Str255 numBuf;
    short itemHit;
    Str255 msg;
    short i;
    short j;
    short count;
    short offerCount;
    char done;

    done = 0;
    count = 0;
    missionListVenue = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0)
            count++;
    }
    if (count <= 0) {
        if (reenterMissionDialog != 0) {
            reenterMissionDialog = 0;
        } else {
            strcpy((char *)msg, "");
            AppendIndString((char *)msg, kGameStrList, kStrTooManyA);
            strcat((char *)msg, " ");
            NumToString(kMaxActiveMissions, numBuf);
            PtoCstr(numBuf);
            strcat((char *)msg, (char *)numBuf);
            strcat((char *)msg, " ");
            AppendIndString((char *)msg, kGameStrList, kStrTooManyB);
            strncpy(gTextShowString, (char *)msg, kShowStringMax);
            ShowTextDialog(0, 0, 0);
            SetDialogGWorld(portDialog);
        }
        return;
    }
    greenMissionSyst = -1;
    offerCount = 0;
    multiArrowsOnSingleSystem = 1;
    for (j = 0; j < kMaxMissions; j++) {
        if (dockedPortMissions[0][j] != -1) {
            if (IsMissionAvailable(dockedPortMissions[0][j], 1))
                offerCount++;
        }
    }
    if (offerCount <= 0) {
        if (reenterMissionDialog != 0) {
            reenterMissionDialog = 0;
        } else {
            strcpy(gTextShowString, "");
            AppendIndString(gTextShowString, kGameStrList, kStrNoMissions);
            ShowTextDialog(0, 0, 0);
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
    RGBForeColor(blackColor);
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
    FrontWindow(missionDialog);
    FrontWindow(missionDialog);
    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(missionDialog, MissionFilter, &itemHit, 0);
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
            short savedTarget;
            short savedRoute;

            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 1;
            if (selMissionListItem != -1) {
                if ((missionAvailData[dockedPortMissions[0][selMissionListItem]].flags
                     & kMissionAvailHasDest) != 0) {
                    if (briefMissionInfo[dockedPortMissions[0][selMissionListItem]].destStellar
                        == -1) {
                        if (briefMissionInfo[dockedPortMissions[0][selMissionListItem]].returnStellar
                            != -1)
                            greenMissionSyst = stellarData[
                                briefMissionInfo[dockedPortMissions[0][selMissionListItem]].returnStellar].system;
                    } else {
                        greenMissionSyst = stellarData[
                            briefMissionInfo[dockedPortMissions[0][selMissionListItem]].destStellar].system;
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
            statusRedraw[0] = 1;
            DrawStatus();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateStellarDialog();
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kItemMissionInfo) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0) {
                    if ((currentMissionObjectives[i].flags & kMissionHideFromList) == 0)
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
            UpdateStellarDialog();
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
            UpdateStellarDialog();
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
    return;
}

/* SetupMissionList. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043cc20
void SetupMissionList(void)
{
    char buf[256];
    short i;
    short count;
    long len;
    Rect box3;
    Rect cellBounds;
    void *itemHandle;
    Rect box;
    unsigned char visible;
    Point cellSize;
    Point rowPt;
    short itemType;

    selMissionListItem = 0;

    count = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[0][i] != -1)
            count++;
    }

    SetDialogGWorld(missionDialog);
    GetDialogItem(missionDialog, 2, &itemType, &itemHandle, &box);
    SetRect(&cellBounds, 0, 0, 1, count);

    TextFont(smallTextFont);
    TextSize(smallTextSize);

    cellSize.v = (smallTextSize * 1.5);

    if (cellSize.v * count <= box.bottom - box.top) {
        visible = 0;
        GetDialogItem(missionDialog, 3, &itemType, &itemHandle, &box3);
        box.right = box3.right;
    } else {
        visible = 1;
    }
    cellSize.h = box.right - box.left;

    missionList = nv_NewDialog(&box, &cellBounds, cellSize,
        (ListDrawProcPtr)MissionListDefinition, (CGrafPtr)missionDialog, visible);
    LockAHandle((Handle)missionList);
    UnlockAHandle((Handle)missionList);
    LSetDrawingMode(0, missionList);

    rowPt.h = 0;
    for (rowPt.v = 0; rowPt.v < count; rowPt.v++) {
        if (dockedPortMissions[0][rowPt.v] == -1) {
            strncpy(buf, "", 3);
        } else {
            strncpy(buf, (char *)missionName[dockedPortMissions[0][rowPt.v]], 0xfa);
            PtoCstr((unsigned char *)buf);
            strncpy(briefingTextPtr, buf, 0xfa);
            MungeBriefing(1, dockedPortMissions[0][rowPt.v]);
            strncpy(buf, briefingTextPtr, 0xfa);
        }
        len = strlen(buf);
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
 * TRUE stride, because gSystems and stellarData are both INDEXED here and a
 * short window would silently step by the wrong size.
 */

/*
 * Four helpers of this TU's own, kept FUN_-named because the mac
 * correspondence is by call ORDER and not proven per address.
 */

/* RandomizeMissionData (mac _RandomizeMissionData). STATE: MATCH, exe 186 / obj 186,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043cf00
void RandomizeMissionData(void)
{
    short saved;
    short venue;
    short i;
    short m;
    short j;
    char hit;
    short count;
    short n;
    short best;
    short next;
    short v;
    short p;

    PropagateMissionBitEffects();
    saved = missionListVenue;
    for (venue = 0; venue <= kMissionVenues - 1; venue++) {
        missionListVenue = venue;
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
    missionListVenue = saved;

    for (i = 0; i < kMaxMissions; i++)
        tempMissions[i] = -1;

    for (venue = 0; venue <= kMissionVenues - 1; venue++) {
        n = 0;
        for (i = 0; i < kMaxMissions; i++) {
            if (dockedPortMissions[venue][i] != -1) {
                tempMissions[n] = dockedPortMissions[venue][i];
                dockedPortMissions[venue][i] = -1;
                /* BUG: the slot was just set to -1, so this reads missionAvailData[-1] */
                p = missionAvailData[dockedPortMissions[venue][i]].dispWeight;
                if (p > best || n == 0)
                    best = p;
                n++;
            }
        }
        count = 0;
        do {
            n = 0;
            for (i = 0; i < kMaxMissions; i++) {
                v = tempMissions[i];
                if (v != -1) {
                    p = missionAvailData[v].dispWeight;
                    if (p >= best) {
                        dockedPortMissions[venue][count] = v;
                        count++;
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

    count = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[1][i] != -1)
            count++;
    }
    if (count > 0)
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
/* RandomizeOneMission (mac _RandomizeOneMission). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043d240
void RandomizeOneMission(short mission)
{
    short i;
    short loc;

    if (offeringFromShip != 0) {
        loc = 0;
        for (i = 0; i < kNavSlots; i++) {
            if (gSystems[shipData->curSystem].navDefault[i] != -1) {
                loc = gSystems[shipData->curSystem].navDefault[i];
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

    briefMissionInfo[mission].cargoType = SelectMissionCargoType(missionAvailData[mission].cargoType);
    briefMissionInfo[mission].cargoUnits = SelectMissionCargoQty(missionAvailData[mission].cargoQty);
    briefMissionInfo[mission].payment = missionAvailData[mission].payment;
    DateAfterDays(&briefMissionInfo[mission].dueDate,
                  missionAvailData[mission].timeLimit);
}

/* SelectMissionCargoType (mac _SelectMissionCargoType). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043d490
short SelectMissionCargoType(short choice)
{
    if (choice >= 0 && choice < 1000)
        return choice;
    if (choice != 1000)
        return -1;
    return Rand(6);
}


/* SelectMissionCargoQty (mac _SelectMissionCargoQty). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043d4c0
short SelectMissionCargoQty(short choice)
{
    short spread;

    if (choice >= 0)
        return choice;
    if (choice > -2)
        return 0;
    spread = __builtin_abs(choice);
    return Rand(spread) + spread / 2;
}

#define kSpobTransient   0x10       /* name TBC */

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
short SelectMissionStellar(short sel, short loc, short avoid, short mission)
{
    short stellar;
    short i;
    short count;
    char ok;

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
            if (count > 0) {
                /* BUG: the distance term tests the scan's leftover i, not the rolled stellar */
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || (stellarData[stellar].flags & kSpobTransient) != 0
                        || !StellarsAreDistant(i, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel == kStellarRandomUninh) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && (stellarData[i].flags & kSpobUninhabited) != 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && (stellarData[i].flags & kSpobTransient) == 0
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || (stellarData[stellar].flags & kSpobUninhabited) == 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || (stellarData[stellar].flags & kSpobTransient) != 0
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random uninhabited stellar");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel > 0 && sel < kFirstResourceID) {
            crashlog_print("warning: mission ");
            crashlog_print_num(mission + 128);
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
            ok = 0;
            if (sel >= kStellarGovt
                    && (govtData[sel - kStellarGovt].flags & kGovtAlwaysDisabled) != 0)
                ok = 1;
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && sel - kStellarGovt == stellarData[i].govt
                        && ((stellarData[i].flags & kSpobUninhabited) == 0 || ok)
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || sel - kStellarGovt != stellarData[stellar].govt
                        || ((stellarData[stellar].flags & kSpobUninhabited) != 0 && !ok)
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar of govt ");
                crashlog_print_num(sel - kStellarGovt + 128);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel >= kStellarAllied && sel < kStellarNotGovt) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && sel - kStellarAllied != stellarData[i].govt) {
                    if (sel - kStellarAllied == gSystems[i].govt)
                        ok = 1;
                    if (GovtAllies(stellarData[i].govt, sel - kStellarAllied))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && sel - kStellarAllied != stellarData[stellar].govt) {
                        if (sel - kStellarAllied == gSystems[stellar].govt)
                            ok = 1;
                        if (GovtAllies(stellarData[stellar].govt, sel - kStellarAllied))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar allied with govt ");
                crashlog_print_num(sel - kStellarAllied + 128);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel >= kStellarNotGovt && sel < kStellarEnemy) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && sel - kStellarNotGovt != stellarData[i].govt
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || sel - kStellarNotGovt == stellarData[stellar].govt
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar not of govt ");
                crashlog_print_num(sel - kStellarNotGovt + 128);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel >= kStellarEnemy && sel < kStellarClass) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && sel - kStellarEnemy != stellarData[i].govt) {
                    if (GovtEnemies(stellarData[i].govt, sel - kStellarEnemy))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && sel - kStellarEnemy != stellarData[stellar].govt) {
                        if (GovtEnemies(stellarData[stellar].govt, sel - kStellarEnemy))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar that is enemy of govt ");
                crashlog_print_num(sel - kStellarEnemy + 128);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel >= kStellarClass && sel < kStellarNotClass) {
            /* BUG: an exact govt match is rejected, so the band never picks the govt it names */
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && sel - kStellarClass != stellarData[i].govt) {
                    if (GovtSharedClass(stellarData[i].govt, sel - kStellarClass))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && sel - kStellarClass != stellarData[stellar].govt) {
                        if (GovtSharedClass(stellarData[stellar].govt, sel - kStellarClass))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar of govt ");
                crashlog_print_num(sel - kStellarClass + 128);
                crashlog_print(" or a govt of same class");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
        if (sel >= kStellarNotClass && sel < 32000) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && sel - kStellarNotClass != stellarData[i].govt) {
                    if (!GovtSharedClass(stellarData[i].govt, sel - kStellarNotClass))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemIsLoaded(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count > 0) {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && sel - kStellarNotClass != stellarData[stellar].govt) {
                        if (!GovtSharedClass(stellarData[stellar].govt,
                                sel - kStellarNotClass))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemIsLoaded(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            } else {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a valid nontransient random stellar not of govt ");
                crashlog_print_num(sel - kStellarNotClass + 128);
                crashlog_print(" or a govt of same class");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
        }
    }
    return stellar;
}

/* SelectMissionSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043e6f0
short SelectMissionSystem(short sel, short loc)
{
    short sys;
    short i;
    short count;
    char ok;
    short link;

    sys = loc;
    if (sel == kSystemRandom) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    } else if (sel == kSystemAdjacent) {
        do {
            i = Rand(kMaxSystemLinks);
            link = gSystems[loc].links[i];
        } while (link == -1
                || (sys = FindActiveCoLocatedSystem(link)) == -1
                || !SystemIsLoaded(sys));
    } else if (sel >= kFirstResourceID && sel < kFirstResourceID + kMaxSystems) {
        sys = sel;
        sys -= kFirstResourceID;
    } else if (sel >= kSystemGovt - 1 && sel < kSystemAllied) {
        /* BUG: off by one, 9999 enters the govt band as if it were one */
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && sel - kSystemGovt == gSystems[i].govt
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || sel - kSystemGovt != gSystems[sys].govt
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    } else if (sel >= kSystemAllied && sel < kSystemNotGovt) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtAllies(gSystems[i].govt, sel - kSystemAllied))
                ok = 1;
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (gSystems[sys].govt >= 0
                        && sel - kSystemAllied != gSystems[sys].govt) {
                    if (GovtAllies(gSystems[sys].govt, sel - kSystemAllied))
                        ok = 1;
                }
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    } else if (sel >= kSystemNotGovt && sel < kSystemEnemy) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && sel - kSystemNotGovt != gSystems[i].govt
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || sel - kSystemNotGovt == gSystems[sys].govt
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    } else if (sel >= kSystemEnemy && sel < kSystemClass) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtEnemies(gSystems[i].govt, sel - kSystemEnemy))
                ok = 1;
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (GovtEnemies(gSystems[sys].govt, sel - kSystemEnemy))
                    ok = 1;
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    } else if (sel >= kSystemClass && sel < kSystemNotClass) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtSharedClass(gSystems[i].govt, sel - kSystemClass))
                ok = 1;
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (GovtSharedClass(gSystems[sys].govt, sel - kSystemClass))
                    ok = 1;
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    } else if (sel >= kSystemNotClass && sel < kSystemBandLimit) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (!GovtSharedClass(gSystems[i].govt, sel - kSystemNotClass))
                ok = 1;
            if (gSystems[i].loaded
                    && SystemIsLoaded(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count > 0) {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (!GovtSharedClass(gSystems[sys].govt, sel - kSystemNotClass))
                    ok = 1;
            } while (!gSystems[sys].loaded
                    || !SystemIsLoaded(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        } else {
            return -1;
        }
    }
    return sys;
}

/* the date `days` days from today; nothing is written for a non-positive count */

/* DateAfterDays. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043f080
void DateAfterDays(DateTimeRec *out, short days)
{
    DateTimeRec d;
    short i;

    d.month = gameTime.month;
    d.day = gameTime.day;
    d.year = gameTime.year;
    if (days <= 0)
        return;
    for (i = 0; i < days; i++)
        AdvanceDate(&d, 0);
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

#define kMissionAbortsOnArrival 0x0001  /* flags55 (TBC) */
#define kMissionShowsDestOnMap  0x0100  /* flags55 (TBC) */
#define kMissionHasSpecialShips 0x0800  /* flags55; gates the dude roll (TBC) */

/* DoMissionAccept (mac _DoMissionAccept). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043f100
char DoMissionAccept(short mission)
{
    short savedVenue;
    short slot;
    short i;
    short days;
    short n;
    short j;
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
        if (ShipTotalHolds() >= briefMissionInfo[mission].cargoUnits) {
            if (TotalMissionCargoSpace() < briefMissionInfo[mission].cargoUnits) {
                strcpy(gTextShowString, "");
                AppendIndString(gTextShowString, kGameStrList, kStrHoldsAreFull);
                ShowTextDialog(0, 0, 0);
                return 0;
            }
        } else {
            strcpy(gTextShowString, "");
            AppendIndString(gTextShowString, kGameStrList, kStrNoHoldsAtAll);
            ShowTextDialog(0, 0, 0);
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
    days = currentMissions[slot].timeLimit;
    if (days > 0)
        DateAfterDays(&currentMissionObjectives[slot].dueDate, days);
    currentMissionObjectives[slot].flags = currentMissions[slot].flags;
    crashlog_mission_bit_set("beginning mission", currentMissions[slot].name, 1,
                             currentMissions[slot].misnIndex + 128,
                             currentMissions[slot].beginSetString);
    EvalCurrentMissionBitSetString(currentMissions[slot].beginSetString, slot);
    if (currentMissions[slot].payment < kMissionCostFloor) {
        shipData->credits -= kMissionCostFloor - currentMissions[slot].payment;
        if (shipData->credits < 0)
            shipData->credits = 0;
        statusRedraw[2] = 1;
    }
    if (missionDialog != NULL)
        SetDialogVisible(missionDialog, 0);
    if (currentMissions[slot].briefDesc != -1) {
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[slot].briefDesc);
        MungeBriefing(0, slot);
        strncpy(gTextShowString, briefingTextPtr, kBriefingTextMax);
        if (missionDialog != NULL)
            SetDialogVisible(missionDialog, 0);
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 1;
        if (currentMissions[slot].flags & kMissionShowsDestOnMap) {
            if (currentMissions[slot].travelStellar == -1) {
                if (currentMissions[slot].returnStellar != -1)
                    greenMissionSyst = stellarData[currentMissions[slot].returnStellar].system;
            } else {
                greenMissionSyst = stellarData[currentMissions[slot].travelStellar].system;
            }
        }
        if (portDialog != NULL && !offeringFromShip)
            UpdateStellarDialog();
        ShowTextDialog(1, 0, 1);
        greenMissionSyst = -1;
    }
    if (currentMissions[slot].pickupMode == 0) {
        currentMissions[slot].hasCargo = 1;
        if (currentMissions[slot].loadCargoDesc != -1) {
            strncpy(briefingTextPtr, "", 3);
            LoadDescription(briefingTextPtr, currentMissions[slot].loadCargoDesc);
            MungeBriefing(0, slot);
            strncpy(gTextShowString, briefingTextPtr, kBriefingTextMax);
            ShowTextDialog(0, 0, 1);
            RefreshSAT();
        }
    }
    if ((currentMissions[slot].travelStellar == shipData->targetStellar && missionDialog != NULL)
        || currentMissions[slot].travelStellar == -1)
        currentMissionObjectives[slot].returnDone = 1;
    if (!offeringFromShip && missionDialog != NULL) {
        savedVenue = missionListVenue;
        missionListVenue = 0;
        do {
            n = 0;
            for (j = 0; j < kMaxMissions; j++)
                list[j] = -1;
            for (j = 0; j < kMaxMissions; j++) {
                if (mission != dockedPortMissions[missionListVenue][j]
                    && dockedPortMissions[missionListVenue][j] != -1) {
                    if (IsMissionAvailable(dockedPortMissions[missionListVenue][j], 1)) {
                        list[n] = dockedPortMissions[missionListVenue][j];
                        n++;
                    } else {
                        crashlog_print("suppressing previously available mission ");
                        crashlog_print_num(dockedPortMissions[missionListVenue][j] + 128);
                        crashlog_print("\n");
                    }
                }
            }
            for (j = 0; j < kMaxMissions; j++)
                dockedPortMissions[missionListVenue][j] = list[j];
            missionListVenue++;
        } while (missionListVenue <= 1);
        missionListVenue = savedVenue;
    }
    if (playerIsLanded && !offeringFromShip
        && (currentMissions[slot].flags & kMissionAbortsOnArrival)
        && currentMissionObjectives[slot].returnDone
        && currentMissions[slot].shipCount == 0
        && currentMissions[slot].returnStellar == -1)
        AutoAbortMission(slot);
    missionRandom[mission] = 0;
    return 1;
}

/* BurgerLib's SwapEndian written out; the STR# entry count is big-endian because the
   resource is Mac-authored */
static __inline short SwapEndianShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

/* LoadCurrentMissionData (mac _LoadCurrentMissionData). STATE: MATCH, exe 692 / obj 692,
   relocregcheck 195 rows 0 differ. OPEN: none */
// FUNCTION: EVNOVA 0x0043f8c0
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

    missionFileHand = GetResource(kMisnType, mission + 128);
    if (missionFileHand != NULL) {
        LockAHandle(missionFileHand);
        currentMissions[slot].flags = *(short *)(*missionFileHand + 0x50);
        currentMissions[slot].flags2 = *(short *)(*missionFileHand + 0x52);
        currentMissions[slot].pickupMode = *(short *)(*missionFileHand + 0x14);
        currentMissions[slot].dropoffMode = *(short *)(*missionFileHand + 0x16);
        currentMissions[slot].scanMask = *(short *)(*missionFileHand + 0x18);
        currentMissions[slot].payment = *(long *)(*missionFileHand + 0x1c);
        currentMissions[slot].shipCount = *(short *)(*missionFileHand + 0x20);
        currentMissions[slot].initialShipCount = currentMissions[slot].shipCount;
        currentMissions[slot].shipDude = *(short *)(*missionFileHand + 0x24);
        if (currentMissions[slot].shipDude > 0)
            currentMissions[slot].shipDude -= 128;
        currentMissions[slot].shipGoal = *(short *)(*missionFileHand + 0x26);
        currentMissions[slot].shipBehav = *(short *)(*missionFileHand + 0x28);
        currentMissions[slot].shipStart = *(short *)(*missionFileHand + 0x2c);
        if ((currentMissions[slot].flags & kMissionHasSpecialShips)
                && currentMissions[slot].shipDude >= 0) {
            currentMissions[slot].shipTypeIndex = SelectShipFieldFromDude(&dudeData[currentMissions[slot].shipDude], 0);
            if (currentMissions[slot].shipTypeIndex == -1)
                currentMissions[slot].shipTypeIndex = SelectShipFieldFromDude(&dudeData[currentMissions[slot].shipDude], 1);
        } else {
            currentMissions[slot].shipTypeIndex = -1;
        }
        currentMissions[slot].compGovt = *(short *)(*missionFileHand + 0x2e);
        currentMissions[slot].compReward = *(short *)(*missionFileHand + 0x30);
        if (currentMissions[slot].compGovt >= 128 && currentMissions[slot].compGovt < 384)
            currentMissions[slot].compGovt -= 128;
        else
            currentMissions[slot].compGovt = -1;
        if (currentMissions[slot].compGovt < 0 || currentMissions[slot].compGovt >= 256)
            currentMissions[slot].compReward = 0;
        currentMissions[slot].briefDesc = *(short *)(*missionFileHand + 0x34);
        currentMissions[slot].quickBriefDesc = *(short *)(*missionFileHand + 0x36);
        currentMissions[slot].loadCargoDesc = *(short *)(*missionFileHand + 0x38);
        currentMissions[slot].dropCargoDesc = *(short *)(*missionFileHand + 0x3a);
        currentMissions[slot].compDesc = *(short *)(*missionFileHand + 0x3c);
        currentMissions[slot].failDesc = *(short *)(*missionFileHand + 0x3e);
        currentMissions[slot].shipDoneDesc = *(short *)(*missionFileHand + 0x44);
        currentMissions[slot].refuseDesc = *(short *)(*missionFileHand + 0x58);
        currentMissions[slot].timeLimit = *(short *)(*missionFileHand + 0x40);
        currentMissions[slot].datePostIncrement = *(short *)(*missionFileHand + 0x65e);
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
        currentMissions[slot].auxShipDude = *(short *)(*missionFileHand + 0x4a);
        currentMissions[slot].auxShipCount = *(short *)(*missionFileHand + 0x48);
        currentMissions[slot].auxShipSyst = *(short *)(*missionFileHand + 0x4c);
        currentMissions[slot].auxShipDelay = Rand(70) + 70;
        currentMissions[slot].auxShipsLeft = currentMissions[slot].auxShipCount;
        if (currentMissions[slot].auxShipDude >= 128 && currentMissions[slot].auxShipDude < 640) {
            currentMissions[slot].auxShipDude -= 128;
        } else {
            currentMissions[slot].auxShipDude = -1;
            currentMissions[slot].auxShipCount = 0;
        }
        if (currentMissions[slot].auxShipCount <= 0) {
            currentMissions[slot].auxShipCount = -1;
            currentMissions[slot].auxShipsLeft = -1;
        }
        if (currentMissions[slot].shipStart > 0) {
            if (currentMissions[slot].shipBehav == 1 && currentMissions[slot].shipGoal == 3)
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
        if (*(short *)(*missionFileHand + 0x42) != 0)
            currentMissions[slot].canAbort = 1;
        else
            currentMissions[slot].canAbort = 0;
        n = *(short *)(*missionFileHand + 0x22);
        if (n == -1) {
            currentMissions[slot].shipSyst = shipData->curSystem;
        } else if (n == -2) {
            currentMissions[slot].shipSyst = SelectMissionSystem(n, shipData->curSystem);
        } else if (n == -3) {
            if (briefMissionInfo[mission].destStellar >= 0
                    && briefMissionInfo[mission].destStellar < 2048) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].destStellar].system;
            } else if (briefMissionInfo[mission].returnStellar >= 0
                    && briefMissionInfo[mission].returnStellar < 2048) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].returnStellar].system;
            } else {
                currentMissions[slot].shipSyst = -1;
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a suitable system for special ships (TravelStel doesn't exist!)\n");
            }
        } else if (n == -4) {
            if (briefMissionInfo[mission].returnStellar >= 0
                    && briefMissionInfo[mission].returnStellar < 2048) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].returnStellar].system;
            } else {
                currentMissions[slot].shipSyst = -1;
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" failed to find a suitable system for special ships (ReturnStel doesn't exist!)\n");
            }
        } else if (n == -6) {
            currentMissions[slot].shipSyst = -6;
        } else if (n == -5 || n >= 9999) {
            currentMissions[slot].shipSyst = SelectMissionSystem(n, shipData->curSystem);
        } else if (n >= 128) {
            currentMissions[slot].shipSyst = n - 128;
        } else {
            if (currentMissions[slot].shipCount > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + 128);
                crashlog_print(" contained invalid special ship system reference\n");
            }
            currentMissions[slot].shipSyst = -1;
        }
        strncpy(currentMissions[slot].shipName, (char *)"\p", 3);
        strncpy(currentMissions[slot].shipSubtitle, (char *)"\p", 3);
        if (*(short *)(*missionFileHand + 0x2a) != -1) {
            currentMissions[slot].destStrList = *(short *)(*missionFileHand + 0x2a);
            h = GetResource(kStringListType, currentMissions[slot].destStrList);
            if (h != NULL) {
                LockAHandle(h);
                strCount = SwapEndianShort((const unsigned short *)*h);
                UnlockAHandle(h);
                ReleaseResource(h);
                n = Rand(strCount) + 1;
                GetIndString(buf, *(short *)(*missionFileHand + 0x2a), n);
                strncpy(currentMissions[slot].shipName, (char *)buf, 0x3f);
                currentMissions[slot].destStrIndex = n;
            }
        }
        if (*(short *)(*missionFileHand + 0x32) != -1) {
            currentMissions[slot].shipStrList = *(short *)(*missionFileHand + 0x32);
            h = GetResource(kStringListType, currentMissions[slot].shipStrList);
            if (h != NULL) {
                LockAHandle(h);
                strCount = SwapEndianShort((const unsigned short *)*h);
                UnlockAHandle(h);
                ReleaseResource(h);
                n = Rand(strCount) + 1;
                GetIndString(buf, *(short *)(*missionFileHand + 0x32), n);
                strncpy(currentMissions[slot].shipSubtitle, (char *)buf, 0x3f);
                currentMissions[slot].shipStrIndex = n;
            }
        }
        strcpy(currentMissions[slot].beginSetString, *missionFileHand + 0x15b);
        strcpy(currentMissions[slot].setString2, *missionFileHand + 0x25a);
        strcpy(currentMissions[slot].succSetString, *missionFileHand + 0x359);
        strcpy(currentMissions[slot].failSetString, *missionFileHand + 0x458);
        strcpy(currentMissions[slot].abortSetString, *missionFileHand + 0x557);
        strcpy(currentMissions[slot].shipDoneSetString, *missionFileHand + 0x660);
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
char DoMissionCargoPickup(short unused, short needed)
{
    if (needed > 0) {
        if (ShipTotalHolds() < needed) {
            strcpy(gTextShowString, "");
            AppendIndString(gTextShowString, kGameStrList, 357);
            ShowTextDialog(0, 0, 0);
            return 0;
        }
        if (TotalMissionCargoSpace() < needed) {
            strcpy(gTextShowString, "");
            AppendIndString(gTextShowString, kGameStrList, 358);
            ShowTextDialog(0, 0, 0);
            return 0;
        }
    }
    statusRedraw[2] = 1;
    ResetPlayerPrecalcedValues();
    return 1;
}

/* DoMissionSuccess (mac _DoMissionSuccess). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440410
void DoMissionSuccess(short mission)
{
    short i;

    if (currentMissions[mission].compDesc != -1) {
        ShowTheCursor();
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[mission].compDesc);
        MungeBriefing(0, mission);
        strncpy(gTextShowString, briefingTextPtr, 0x1ffe);
        ShowTextDialog(0, 0, 1);
    }
    currentMissionObjectives[mission].active = 0;
    crashlog_mission_bit_set("mission success", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + 128,
                             currentMissions[mission].succSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].succSetString, mission);
    if (currentMissions[mission].datePostIncrement > 0) {
        crashlog_print_mdy(&gameTime);
        crashlog_print(": incrementing date ");
        crashlog_print_num(currentMissions[mission].datePostIncrement);
        crashlog_print(" days for mission ");
        crashlog_print_num(currentMissions[mission].misnIndex + 128);
        crashlog_print(" completion post increment\n");
        for (i = 0; i < currentMissions[mission].datePostIncrement; i++)
            IncrementGameTime();
    }
    if (currentMissions[mission].compGovt >= 0 && currentMissions[mission].compGovt < 256) {
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].govt == currentMissions[mission].compGovt)
                playerLegalStatus[i] += currentMissions[mission].compReward;
            else if (gSystems[i].govt != -1) {
                if (GovtEnemies(gSystems[i].govt, currentMissions[mission].compGovt))
                    playerLegalStatus[i] -= currentMissions[mission].compReward * 0.5;
                else if (GovtAllies(gSystems[i].govt, currentMissions[mission].compGovt))
                    playerLegalStatus[i] += currentMissions[mission].compReward * 0.5;
            }
        }
    }
    ApplyMissionPay(currentMissions[mission].payment);
    lastMissionStellar = -1;
}

/* The mission payment field doubles as a command channel. Positive values are
   cash; four negative bands instead clear the player's criminal record -- with
   one government, with its allies, with the third relation -- or write a
   percentage off their money. Government ids arrive biased by the resource
   base, which is where the 10128/20128/30128 constants come from. */
/* ApplyMissionPay (mac _ApplyMissionPay). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440750
void ApplyMissionPay(long amount)
{
    long govt;
    long i;

    if (amount > 0) {
        shipData[0].credits += amount;
        statusRedraw[2] = 1;
    } else if (amount <= -10000 && amount > -20000) {
        govt = abs(amount) - 10128;
        for (i = 0; i < kMaxSystems; i++) {
            if (govt == gSystems[i].govt && playerLegalStatus[i] < 0)
                playerLegalStatus[i] = 0;
        }
    } else if (amount <= -20000 && amount > -30000) {
        govt = abs(amount) - 20128;
        for (i = 0; i < kMaxSystems; i++) {
            if (GovtAllies(gSystems[i].govt, govt) && playerLegalStatus[i] < 0)
                playerLegalStatus[i] = 0;
        }
    } else if (amount <= -30000 && amount > -40000) {
        govt = abs(amount) - 30128;
        for (i = 0; i < kMaxSystems; i++) {
            if (GovtSharedClass(gSystems[i].govt, govt) && playerLegalStatus[i] < 0)
                playerLegalStatus[i] = 0;
        }
    } else if (amount <= -40001 && amount > -40100) {
        govt = abs(amount);
        govt -= 40000;
        shipData[0].credits -= shipData[0].credits * (govt * 0.01);
    }
}

/* a failed mission costs legal standing in every system of the offended
   government, then shows its failure desc and aborts itself */
/* DoMissionFailure (mac _DoMissionFailure). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440930
void DoMissionFailure(short mission)
{
    short i;

    crashlog_mission_bit_set("mission failure", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + 128,
                             currentMissions[mission].failSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].failSetString, mission);
    if (currentMissions[mission].compGovt != -1) {
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].govt == currentMissions[mission].compGovt)
                playerLegalStatus[i] -= currentMissions[mission].compReward / 2;
        }
    }
    currentMissionObjectives[mission].active = 0;
    if (currentMissions[mission].failDesc != -1) {
        ShowTheCursor();
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[mission].failDesc);
        MungeBriefing(0, mission);
        strncpy(gTextShowString, briefingTextPtr, 0x1ffe);
        ShowTextDialog(0, 0, 1);
    }
    AbortMission(mission, 0);
    lastMissionStellar = -1;
}

/* the ships an aborted mission owned are detached and sent away; `evalBits`
   is what a deliberate abort passes to run the mission's abort expression,
   and a failure path passes 0 so that only the failure expression runs */
/* AbortMission (mac _AbortMission). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440aa0
void AbortMission(short mission, char evalBits)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].specialMission == mission) {
            shipData[i].specialMission = -1;
            if (shipData[i].parentShip != -1) {
                shipData[i].aiType = gShipTypes[shipData[i].type].inherentAI;
                shipData[i].parentShip = -1;
                AIMakeShipLeave(&shipData[i]);
            }
            if (portDialog != NULL)
                shipData[i].active = 0;
        }
    }
    if (evalBits != 0) {
        crashlog_mission_bit_set("aborting mission", currentMissions[mission].name, 1,
                                 currentMissions[mission].misnIndex + 128,
                                 currentMissions[mission].abortSetString);
        EvalCurrentMissionBitSetString(currentMissions[mission].abortSetString, mission);
    }
    currentMissions[mission].hasCargo = 0;
    currentMissionObjectives[mission].active = 0;
    lastMissionStellar = -1;
}

/* QuickMissionFailure (mac _QuickMissionFailure). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00440bf0
void QuickMissionFailure(short mission)
{
    crashlog_mission_bit_set("quick mission failure", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + 128,
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
char MissionFilter(long unused, EventRecord *ev, short *itemHit)
{
    int key;
    Rect box;
    short itemType;
    Point where;
    Point cell;
    void *item;
    short old;
    int c;
    short button;

    if (absquatulateStr[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 7;
        return 1;
    }
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 6;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 9;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 10;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        key = tolower(c);
        if ((int)ev->what == kKeyDown) {
            if (key == kReturnChar || key == kEscapeChar) {
                *itemHit = 7;
                return 1;
            }
            if (key == 'l') {
                *itemHit = 7;
                return 1;
            }
            if (key == 'a' && selMissionListItem != -1) {
                *itemHit = 1;
                return 1;
            }
        }
        if (key == kTabChar) {
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
            if (dockedPortMissions[missionListVenue][selMissionListItem] == -1)
                selMissionListItem = 0;
            strncpy(briefingTextPtr, "", 3);
            if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                dockedPortMissions[missionListVenue][selMissionListItem]
                                    + kMissionDescBase);
                MungeBriefing(1,
                              dockedPortMissions[missionListVenue][selMissionListItem]);
                cell.v = selMissionListItem;
                LSetSelect(1, cell, missionList);
                LAutoScroll(missionList);
            }
            if (selMissionListItem != old) {
                InvalDialogItem(missionDialog, 4);
                InvalDialogItem(missionDialog, 5);
            }
            cell.v = selMissionListItem;
            LSetSelect(1, cell, missionList);
            LAutoScroll(missionList);
        }
        if (key == kDownArrowChar) {
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
                if (dockedPortMissions[missionListVenue][selMissionListItem] == -1)
                    selMissionListItem = 0;
                strncpy(briefingTextPtr, "", 3);
                if (selMissionListItem != -1) {
                    LoadDescription(briefingTextPtr,
                                    dockedPortMissions[missionListVenue]
                                                      [selMissionListItem]
                                        + kMissionDescBase);
                    MungeBriefing(1,
                                  dockedPortMissions[missionListVenue]
                                                    [selMissionListItem]);
                    cell.v = selMissionListItem;
                    LSetSelect(1, cell, missionList);
                    LAutoScroll(missionList);
                }
                if (selMissionListItem != old) {
                    InvalDialogItem(missionDialog, 4);
                    InvalDialogItem(missionDialog, 5);
                }
            }
        }
        if (key == kUpArrowChar) {
            if (selMissionListItem == -1) {
                selMissionListItem = kMaxMissions - 1;
                while (dockedPortMissions[missionListVenue]
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
                    while (dockedPortMissions[missionListVenue]
                                             [selMissionListItem] == -1
                           && selMissionListItem > 0)
                        selMissionListItem--;
                } else if (dockedPortMissions[missionListVenue]
                                             [selMissionListItem] == -1) {
                    selMissionListItem = -1;
                }
                strncpy(briefingTextPtr, "", 3);
                if (selMissionListItem != -1) {
                    LoadDescription(briefingTextPtr,
                                    dockedPortMissions[missionListVenue]
                                                      [selMissionListItem]
                                        + kMissionDescBase);
                    MungeBriefing(1,
                                  dockedPortMissions[missionListVenue]
                                                    [selMissionListItem]);
                    cell.v = selMissionListItem;
                    LSetSelect(1, cell, missionList);
                    LAutoScroll(missionList);
                }
                if (selMissionListItem != old) {
                    InvalDialogItem(missionDialog, 4);
                    InvalDialogItem(missionDialog, 5);
                }
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(missionDialog, 2, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box)) {
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
                                dockedPortMissions[missionListVenue][selMissionListItem]
                                    + kMissionDescBase);
                MungeBriefing(1,
                              dockedPortMissions[missionListVenue][selMissionListItem]);
            }
            if (selMissionListItem != old) {
                InvalDialogItem(missionDialog, 4);
                InvalDialogItem(missionDialog, 5);
            }
        }
        GetDialogItem(missionDialog, 3, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box))
            LClick(where, ev->modifiers, missionList);
        MissionDialogUpdate();
        button = TrackMissionButtons(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 7;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if ((int)ev->what == kUpdateEvt) {
        SetDialogGWorld(missionDialog);
        MissionDialogUpdate();
    }
    return 0;
}

/* MissionDialogUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00441620
void MissionDialogUpdate(void)
{
    char savedBriefing[0x2000];
    void *item;
    short itemType;
    char text[256];
    Rect box;

    SetGWorldContainer(&loadPort);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionDialog));
    if (missionBackgroundPic[0] != NULL)
        DrawPicHandle(missionBackgroundPic[0], GetDialogPortRect(missionDialog));
    GetDialogItem(missionDialog, 8, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(missionDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(&rgbLightGrey);
        MoveTo(box.left, (box.top + 12));
        DrawIndString(kGameStrList, kListCaptionIdx);
        RGBForeColor(blackColor);
    }
    GetDialogItem(missionDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(missionDialog)))) {
        if (selMissionListItem != -1) {
            RGBForeColor(blackColor);
            TextFont(gTimesFont);
            TextSize(18);
            strncpy(text, (char *)missionName[dockedPortMissions[missionListVenue]
                                                               [selMissionListItem]], 0xfa);
            PtoCstr((unsigned char *)text);
            strncpy(savedBriefing, briefingTextPtr, 0x1ffe);
            strncpy(briefingTextPtr, text, 0xfa);
            MungeBriefing(1, dockedPortMissions[missionListVenue][selMissionListItem]);
            strncpy(text, briefingTextPtr, 0xfa);
            strncpy(briefingTextPtr, savedBriefing, 0x1ffe);
            TETextBox(text, strlen(text), &box, 0);
            InvertRect(&box);
        } else {
            RGBForeColor(blackColor);
            PaintRect(&box);
        }
    }
    GetDialogItem(missionDialog, 4, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(missionDialog)))) {
        if (selMissionListItem != -1) {
            RGBForeColor(blackColor);
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            TETextBox(briefingTextPtr, strlen(briefingTextPtr), &box, 0);
            InvertRect(&box);
        } else {
            RGBForeColor(blackColor);
            PaintRect(&box);
        }
    }
    GetDialogItem(missionDialog, 11, &itemType, &item, &box);
    RGBForeColor(&rgbDarkGrey);
    DateToMedString(gameTime, text);
    CtoPstr(text);
    MoveTo(box.left, box.bottom);
    DrawString((unsigned char *)text);
    RGBForeColor(blackColor);
    DrawMissionButtons(-1);
    if (missionList != NULL)
        LUpdate(missionList);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(missionDialog);
    RGBForeColor(blackColor);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(missionDialog)),
             GetDialogPortRect(missionDialog), GetDialogPortRect(missionDialog), 0,
             GetCWindowVisRgn(FrontWindow(missionDialog)));
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (missionList != NULL) {
        SetDialogGWorld(missionDialog);
        LUpdate(missionList);
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

#define kMissionNeedsFuel   0x0008  /* flags */
#define kMissionSmallShip   0x2000
#define kMissionLargeShip   0x4000
#define kMissionNeedsCargo  0x0001  /* flags2 */

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
char IsMissionAvailable(short mission, char offer)
{
    char cond[kMissionConds];
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
        if (missionListVenue != 0 && missionAvailData[mission].availLoc == 0)
            return 0;
        if (missionListVenue == 0 && missionAvailData[mission].availLoc != 0)
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

    if ((missionAvailData[mission].flags & kMissionNeedsFuel) != 0
        && shipData->fuel < kMinFuelForMission)
        return 0;

    for (n = 0; n < kMissionConds; n++)
        cond[n] = 0;

    loc = missionAvailData[mission].availStel;
    if (loc == -1 || offeringFromShip != 0) {
        cond[0] = 1;
    } else if (loc >= kFirstResourceID && loc < kAvailLinkBase) {
        if (shipData->targetStellar == loc - kFirstResourceID)
            cond[0] = 1;
    } else if (loc >= kAvailLinkBase && loc < kAvailOwnerBase - 1) {
        for (n = 0; n < kNavSlots; n++) {
            if (gSystems[shipData->curSystem].links[n]
                == missionAvailData[mission].availStel - kAvailLinkBase) {
                cond[0] = 1;
                break;
            }
        }
    } else if (loc >= kAvailOwnerBase - 1 && loc < kAvailAlliedBase) {
        if (dockedPort->govt == loc - kAvailOwnerBase)
            cond[0] = 1;
    } else if (loc >= kAvailAlliedBase && loc < kAvailGovtBase) {
        if (dockedPort->govt != -1
            && GovtAllies(loc - kAvailAlliedBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (loc >= kAvailGovtBase && loc < kAvailEnemyBase) {
        if (dockedPort->govt != loc - kAvailGovtBase)
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
    } else if (missionAvailData[mission].availRecord <= 0) {
        if (playerLegalStatus[shipData->curSystem]
            <= missionAvailData[mission].availRecord)
            cond[2] = 1;
    } else if (playerLegalStatus[shipData->curSystem]
               >= missionAvailData[mission].availRecord) {
        cond[2] = 1;
    }

    if (missionAvailData[mission].availRating <= 0)
        cond[3] = 1;
    else if (playerKills >= missionAvailData[mission].availRating)
        cond[3] = 1;

    if (missionAvailData[mission].availRandom < 100) {
        if (missionRandom[mission] <= missionAvailData[mission].availRandom)
            cond[4] = 1;
    } else {
        cond[4] = 1;
    }

    if (offeringFromShip != 0) {
        if (missionAvailData[mission].cargoQty <= 0)
            cond[5] = 1;
        else if (TotalMissionCargoSpace() < missionAvailData[mission].cargoQty)
            cond[5] = 0;
        else
            cond[5] = 1;
    } else if ((missionAvailData[mission].flags2 & kMissionNeedsCargo) == 0) {
        cond[5] = 1;
    } else if (TotalMissionCargoSpace() < missionAvailData[mission].cargoQty) {
        cond[5] = 0;
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
        if (gShipTypes[shipData->type].inherentGovt
                == missionAvailData[mission].availShipType - ResClassBase(kReqShipGovt)
            || gShipTypes[shipData->type].inherentGovtB
                == missionAvailData[mission].availShipType - ResClassBase(kReqShipGovt))
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqNotShipGovt)
               && missionAvailData[mission].availShipType <= ResClassBase(kReqNotShipGovt) + kMaxGovts) {
        if (gShipTypes[shipData->type].inherentGovt
                != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipGovt)
            && gShipTypes[shipData->type].inherentGovtB
                != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipGovt))
            cond[7] = 1;
    } else {
        cond[7] = 1;
    }

    cond[8] = 1;
    if ((missionAvailData[mission].flags & kMissionSmallShip) != 0
        && gShipTypes[shipData->type].inherentAI <= kLargeShipClass - 1)
        cond[8] = 0;
    if ((missionAvailData[mission].flags & kMissionLargeShip) != 0
        && gShipTypes[shipData->type].inherentAI >= kLargeShipClass)
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


#define kMissionAvailNoRefuse     0x0004

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
short OfferOneMission(short mission)
{
    Rect box;
    short refuseDescID;
    char refused;
    char done;
    long missionResID;
    void *itemHandle;
    short itemType;
    char *refuseLabel;
    short itemHit;
    char setString[256];
    Rect *wr;
    short i;
    short itemHeight;
    short shift;
    short floatZero;
    short widest;
    short otherWidth;
    short savedTarget;
    short savedRoute;
    short shown;
    short savedScroll;
    unsigned char labelChar;
    char accepted;
    char savedUp;
    char savedDown;

    refused = 0;
    accepted = 0;
    done = 0;
    if (mission < 0 || mission >= kMaxMissions)
        return 0;

    if ((missionAvailData[mission].flags & kMissionAvailNoRefuse) != 0)
        canRefuseMission = 0;
    else
        canRefuseMission = 1;

    crashlog_print("offering mission ");
    crashlog_print_pstr(missionName[mission]);
    crashlog_print(" (");
    missionResID = mission + kFirstResourceID;
    crashlog_print_num(missionResID);
    crashlog_print(")\n");

    strncpy(briefingTextPtr, "", 3);
    LoadDescription(briefingTextPtr, mission + kMissionDescBase);

    if (descMovieName[0] != 0 && gEarlyOutFlag == 0
        && (gDescFlags & kDescMovieAfter) == 0)
        PlayQTMovie(descMovieName, gDescFlags & kDescMovieDoubled,
                    gDescFlags & kDescMovieFade);

    if (screenFaded != 0) {
        UpdateAllPortDialogs();
        RefreshSAT();
        RedrawStatus(1);
        FadeScreenIn();
        LoadDescription(briefingTextPtr, mission + kMissionDescBase);
    }

    if (canRefuseMission == 0 && strlen(briefingTextPtr) == 0) {
        accepted = DoMissionAccept(mission);
        if (accepted != 0)
            return 1;
        return -1;
    }

    strcat(briefingTextPtr, "\r");

    missionDialog = NULL;
    if (descResourcePictId >= 128)
        missionDialog = GetNewDialog(kWideDlogID, UpdateMissionDialog,
                                     SingleMissionFilter);
    else
        missionDialog = GetNewDialog(kNarrowDlogID, UpdateMissionDialog,
                                     SingleMissionFilter);
    if (missionDialog == NULL)
        return 0;

    if (descResourcePictId < 128) {
        for (i = 0; i < kMissionPictCount; i++)
            missionBackgroundPic[i] = GetPicture(kNarrowPictBase + i);
    } else {
        missionBackgroundPic[0] = GetPicture(kWidePictID);
        missionBackgroundPic[1] = NULL;
        missionBackgroundPic[2] = NULL;
    }

    CenterWindow(FrontWindow(missionDialog), gWind);
    MungeBriefing(1, mission);
    SetDialogGWorld(missionDialog);

    GetDialogItem(missionDialog, kItemBriefText, &itemType, &itemHandle, &box);
    gMissionTE = TENew(&box, &box);
    if (gMissionTE != NULL) {
        (*gMissionTE)->txFont = smallTextFont;
        (*gMissionTE)->txSize = smallTextSize;
        TextFont((*gMissionTE)->txFont);
        TextSize((*gMissionTE)->txSize);
        TESetText(briefingTextPtr, strlen(briefingTextPtr), gMissionTE);

        shown = (*gMissionTE)->height;
        itemHeight = box.bottom - box.top;
        if (shown < itemHeight) {
            if (descResourcePictId < 128) {
                shown = shown < kMinTextHeight
                             ? (short)kMinTextHeight : shown;
                wr = GetDialogPortRect(missionDialog);
                box.top = wr->top;
                box.left = wr->left;
                box.bottom = wr->bottom;
                box.right = wr->right;
                shift = itemHeight - shown - kTextMargin;
                box.bottom -= shift;
                SizeDialogWindow(missionDialog, box.right - box.left,
                             box.bottom - box.top, 1);
                floatZero = 0;
                floatZero += shift * 0.3;
                OffsetDialogWindow(missionDialog, 0, floatZero, 1);

                GetDialogItem(missionDialog, kItemAccept, &itemType,
                              &itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemAccept, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemRefuse, &itemType,
                              &itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemRefuse, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemDivider, &itemType,
                              &itemHandle, &box);
                OffsetRect(&box, 0, -shift);
                SetDialogItem(missionDialog, kItemDivider, itemType,
                              itemHandle, &box);

                GetDialogItem(missionDialog, kItemBriefText, &itemType,
                              &itemHandle, &box);
                box.bottom -= shift;
                SetDialogItem(missionDialog, kItemBriefText, itemType,
                              itemHandle, &box);
            }
            GetDialogItem(missionDialog, kItemBriefText, &itemType,
                          &itemHandle, &(*gMissionTE)->destRect);
            GetDialogItem(missionDialog, kItemBriefText, &itemType,
                          &itemHandle, &(*gMissionTE)->viewRect);
            TECalText(gMissionTE);
        }
    }

    strcpy((char *)missionButtonLabel[0], "");
    refuseLabel = (char *)missionButtonLabel[1];
    strcpy(refuseLabel, "");

    missionFileHand = NULL;
    missionFileHand = GetResource(kMisnType, missionResID);
    if (missionFileHand != NULL) {
        strcpy((char *)missionButtonLabel[0], *missionFileHand + 0x75f);
        strcpy(refuseLabel, *missionFileHand + 0x77f);
        for (i = 0; i <= kMissionButtonLabelCount - 1; i++) {
            labelChar = missionButtonLabel[i][0];
            if (tolower(labelChar) < 'a' || tolower(labelChar) > 'z')
                strcpy((char *)missionButtonLabel[i], "");
        }
        refuseDescID = *(unsigned short *)(*missionFileHand + 0x58);
        strcpy(setString, *missionFileHand + 0x25a);
        ReleaseResource(missionFileHand);
    } else {
        refuseDescID = -1;
        strcpy(setString, "");
    }

    if (strlen((char *)missionButtonLabel[0]) == 0) {
        if (canRefuseMission != 0)
            GetIndString(missionButtonLabel[0], kButtonStrList,
                         kStrAcceptRefusable);
        else
            GetIndString(missionButtonLabel[0], kButtonStrList,
                         kStrAcceptOnly);
    } else {
        CtoPstr((char *)missionButtonLabel[0]);
    }
    if (strlen(refuseLabel) == 0)
        GetIndString((unsigned char *)refuseLabel, kButtonStrList, kStrRefuse);
    else
        CtoPstr(refuseLabel);

    SetDialogGWorld(missionDialog);
    TextFont(0);
    TextSize(12);
    widest = StringWidth(missionButtonLabel[0]);
    otherWidth = StringWidth(missionButtonLabel[1]);
    if (widest > otherWidth)
        otherWidth = widest;
    otherWidth += kButtonPadding;
    if (otherWidth < kMinButtonWidth)
        otherWidth = kMinButtonWidth;

    GetDialogItem(missionDialog, kItemAccept, &itemType, &itemHandle, &box);
    box.right = box.left + otherWidth;
    SetDialogItem(missionDialog, kItemAccept, itemType, itemHandle, &box);

    GetDialogItem(missionDialog, kItemRefuse, &itemType, &itemHandle, &box);
    box.left = box.right - otherWidth;
    SetDialogItem(missionDialog, kItemRefuse, itemType, itemHandle, &box);

    GetDialogItem(missionDialog, kItemDivider, &itemType, &itemHandle, &box);
    box.right = (box.left + box.right) / 2;
    box.left = box.right;
    InsetRect(&box, -otherWidth / 2, 0);
    SetDialogItem(missionDialog, kItemDivider, itemType, itemHandle, &box);

    lastTextScrollTime = ReadTick();
    canScrollTextUp = 0;
    canScrollTextDown = 1;
    savedScroll = (*gMissionTE)->destRect.bottom;
    TEPinScroll(0, -1, gMissionTE);
    if ((*gMissionTE)->destRect.bottom == savedScroll)
        canScrollTextDown = 0;
    TEPinScroll(0, 1, gMissionTE);
    savedUp = canScrollTextUp;
    savedDown = canScrollTextDown;

    ShowWindowNoFlash(FrontWindow(missionDialog));
    SetDialogGWorld(missionDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RGBForeColor(blackColor);
    RedrawDialog(missionDialog);
    UpdateMissionDialog();
    FrontWindow(missionDialog);
    FrontWindow(missionDialog);

    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(missionDialog, SingleMissionFilter, &itemHit, 0);
        if (canRefuseMission != 0) {
            if (itemHit == kItemAccept) {
                FlushEvents(3, 0);
                FlushEvents(5, 0);
                FlushEvents(1, 0);
                FlushEvents(2, 0);
                SetDialogVisible(missionDialog, 0);
                accepted = DoMissionAccept(mission);
                UpdateMissionDialog();
                LoadDescription(briefingTextPtr, mission + kMissionDescBase);
                MungeBriefing(1, mission);
                SetDialogGWorld(missionDialog);
                done = 1;
            }
            if (itemHit == kItemRefuse) {
                if (refuseDescID != -1) {
                    SetDialogVisible(missionDialog, 0);
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, refuseDescID);
                    MungeBriefing(0, -1);
                    strncpy(gTextShowString, briefingTextPtr, kBriefingTextMax);
                    ShowTextDialog(0, 0, 1);
                }
                crashlog_mission_bit_set("refusing mission", NULL, 0,
                                         mission + kFirstResourceID, setString);
                EvalMissionBitSetString(setString);
                refused = 1;
                done = 1;
            }
        } else if (itemHit == kItemAccept || itemHit == kItemRefuse) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            accepted = DoMissionAccept(mission);
            LoadDescription(briefingTextPtr, mission + kMissionDescBase);
            MungeBriefing(1, mission);
            UpdateMissionDialog();
            done = 1;
        }
        if (itemHit == kBriefItemMap) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 1;
            if ((missionAvailData[mission].flags & kMissionAvailHasDest) != 0) {
                if (briefMissionInfo[mission].destStellar != -1) {
                    greenMissionSyst =
                        stellarData[briefMissionInfo[mission].destStellar].system;
                } else {
                    if (briefMissionInfo[mission].returnStellar != -1)
                        greenMissionSyst =
                            stellarData[briefMissionInfo[mission].returnStellar].system;
                }
            }
            savedTarget = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedTarget;
            shipData->navTargetKind = savedRoute;
            greenMissionSyst = -1;
            if (offeringFromShip == 0 && playerIsLanded != 0) {
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateStellarDialog();
            }
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kBriefItemPlayerInfo) {
            FlushEvents(3, 0);
            FlushEvents(5, 0);
            FlushEvents(1, 0);
            FlushEvents(2, 0);
            DoPlayerInfoDialog();
            if (offeringFromShip == 0 && playerIsLanded != 0) {
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateStellarDialog();
            }
            SetDialogGWorld(missionDialog);
            RedrawDialog(missionDialog);
        }
        if (itemHit == kBriefItemMissionInfo) {
            shown = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0) {
                    if ((currentMissionObjectives[i].flags & kMissionHideFromList) == 0)
                        shown++;
                }
            }
            if (shown > 0) {
                FlushEvents(3, 0);
                FlushEvents(5, 0);
                FlushEvents(1, 0);
                FlushEvents(2, 0);
                DoMissionInfoDialog();
                LoadDescription(briefingTextPtr, mission + kMissionDescBase);
                MungeBriefing(1, mission);
                if (offeringFromShip == 0 && playerIsLanded != 0) {
                    SetDialogGWorld(portDialog);
                    RedrawDialog(portDialog);
                    UpdateStellarDialog();
                }
                SetDialogGWorld(missionDialog);
                RedrawDialog(missionDialog);
            }
        }
        if (itemHit == kItemScrollUp) {
            if (canScrollTextUp != 0)
                ScrollDialogText(missionDialog, kItemBriefText, gMissionTE,
                                 kBriefScrollStep, 1);
        }
        if (itemHit == kItemScrollDown) {
            if (canScrollTextDown != 0)
                ScrollDialogText(missionDialog, kItemBriefText, gMissionTE,
                                 -kBriefScrollStep, 1);
        }
        if (canScrollTextUp != savedUp) {
            InvalDialogItem(missionDialog, kItemScrollUp);
            savedUp = canScrollTextUp;
        }
        if (canScrollTextDown != savedDown) {
            InvalDialogItem(missionDialog, kItemScrollDown);
            savedDown = canScrollTextDown;
        }
    } while (done == 0);

    if (descMovieName[0] != 0 && gEarlyOutFlag == 0
        && (gDescFlags & kDescMovieAfter) != 0)
        PlayQTMovie(descMovieName, gDescFlags & kDescMovieDoubled,
                    gDescFlags & kDescMovieFade);

    for (i = 0; i < kMissionPictCount; i++) {
        if (missionBackgroundPic[i] != NULL) {
            QDReleaseResource(missionBackgroundPic[i]);
            missionBackgroundPic[i] = NULL;
        }
    }
    if (gMissionTE != NULL)
        TEDispose(gMissionTE);
    while (KeyCheck(keyPrefs.missions))
        BusyIdle();

    DisposeDialogNoFlash(missionDialog);
    missionDialog = NULL;
    if (screenFaded != 0) {
        if (playerIsLanded != 0)
            UpdateAllPortDialogs();
        RedrawStatus(1);
        FadeScreenIn();
    }
    FlushEvents(7, 0);
    if (refused != 0)
        return 0;
    if (accepted != 0)
        return 1;
    return -1;
}

/* MissionHandler (mac _MissionHandler). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00443760
void MissionHandler(void)
{
    short i;

    for (i = 0; i < kMaxActiveMissions; i++)
        MissionObjectivesCheck(i);
}

/* MissionLandCheck (mac _MissionLandCheck). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00443780
void MissionLandCheck(void)
{
    short i;
    short stel;
    char anySuccess;

    lastPortMissionLoc = -1;
    stel = shipData[0].targetStellar;
    anySuccess = 0;
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
                    if (currentMissions[i].shipGoal == 3 && currentMissions[i].specialShipsKilled <= 0)
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
void MissionLandCargoCheck(short slot, short stel)
{
    if (StellarsAreIdentical(currentMissions[slot].travelStellar, stel)) {
        if (currentMissions[slot].pickupMode == 1 && currentMissions[slot].hasCargo == 0) {
            if (DoMissionCargoPickup(currentMissions[slot].cargoType, currentMissions[slot].cargoQty)) {
                currentMissions[slot].hasCargo = 1;
                currentMissionObjectives[slot].returnDone = 1;
                crashlog_mission_bit_set("doing mission cargo pickup",
                                         currentMissions[slot].name, 1,
                                         currentMissions[slot].misnIndex + 128, "");
                if (currentMissions[slot].loadCargoDesc != -1) {
                    ShowTheCursor();
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, currentMissions[slot].loadCargoDesc);
                    MungeBriefing(0, slot);
                    strncpy(gTextShowString, briefingTextPtr, 0x1ffe);
                    ShowTextDialog(0, 0, 1);
                }
            }
        } else {
            currentMissionObjectives[slot].returnDone = 1;
        }
        if (currentMissions[slot].dropoffMode == 0 && currentMissions[slot].hasCargo != 0) {
            currentMissions[slot].hasCargo = 0;
            crashlog_mission_bit_set("doing mission cargo dropoff",
                                     currentMissions[slot].name, 1,
                                     currentMissions[slot].misnIndex + 128, "");
            if (currentMissions[slot].dropCargoDesc != -1) {
                ShowTheCursor();
                strncpy(briefingTextPtr, "", 3);
                LoadDescription(briefingTextPtr, currentMissions[slot].dropCargoDesc);
                MungeBriefing(0, slot);
                strncpy(gTextShowString, briefingTextPtr, 0x1ffe);
                ShowTextDialog(0, 0, 1);
                ResetPlayerPrecalcedValues();
            }
            statusRedraw[2] = 1;
        }
    }
    if (StellarsAreIdentical(currentMissions[slot].returnStellar, stel)) {
        if (currentMissions[slot].dropoffMode == 1 && currentMissions[slot].hasCargo != 0) {
            if (currentMissionObjectives[slot].objectivesDone != 0 || currentMissions[slot].shipGoal == -1
                    || (currentMissions[slot].shipGoal == 3 && currentMissions[slot].specialShipsKilled <= 0)) {
                currentMissions[slot].hasCargo = 0;
                crashlog_mission_bit_set("doing mission cargo dropoff",
                                         currentMissions[slot].name, 1,
                                         currentMissions[slot].misnIndex + 128, "");
                if (currentMissions[slot].dropCargoDesc != -1) {
                    ShowTheCursor();
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, currentMissions[slot].dropCargoDesc);
                    MungeBriefing(0, slot);
                    strncpy(gTextShowString, briefingTextPtr, 0x1ffe);
                    ShowTextDialog(0, 0, 1);
                }
                statusRedraw[2] = 1;
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
void MissionObjectivesCheck(short slot)
{
    Str255 buf;
    Rect vis;
    Rect shipRect;
    long prevDone;
    short i;
    short count;
    short syst;
    Rect *vr;
    Rect *sr;

    if (currentMissionObjectives[slot].active == 0)
        return;
    if (currentMissions[slot].shipGoal == -1 && currentMissions[slot].shipCount > 0
            && currentMissions[slot].specialShipsJumpedIn <= 0
            && (currentMissions[slot].flags & 1) != 0)
        currentMissionObjectives[slot].objectivesDone = 0;
    prevDone = (unsigned char)currentMissionObjectives[slot].objectivesDone;
    if (currentMissions[slot].shipGoal == -1 && currentMissions[slot].initialShipCount <= 0) {
        currentMissionObjectives[slot].objectivesDone = 1;
    } else if (currentMissions[slot].initialShipCount > 0) {
        if (currentMissions[slot].shipGoal == -1) {
            if ((currentMissions[slot].flags & 1) != 0) {
                if (currentMissions[slot].specialShipsJumpedIn > 0
                        || currentMissions[slot].shipStart != 1)
                    currentMissionObjectives[slot].objectivesDone = 1;
            } else {
                currentMissionObjectives[slot].objectivesDone = 1;
            }
        } else if (currentMissions[slot].specialShipsJumpedIn < currentMissions[slot].initialShipCount
                && currentMissions[slot].shipCount != 0) {
            currentMissionObjectives[slot].objectivesDone = 0;
        } else {
            if (currentMissions[slot].shipGoal == 0
                    && currentMissions[slot].specialShipsKilled >= currentMissions[slot].initialShipCount)
                currentMissionObjectives[slot].objectivesDone = 1;
            if (currentMissions[slot].shipGoal == 1) {
                if (currentMissions[slot].specialShipsKilled > 0)
                    currentMissionObjectives[slot].failed = 1;
                else if (currentMissions[slot].specialShipsDisabled >= currentMissions[slot].initialShipCount)
                    currentMissionObjectives[slot].objectivesDone = 1;
            }
            if ((currentMissions[slot].shipGoal == 2 || currentMissions[slot].shipGoal == 5)
                    && currentMissions[slot].specialShipsBoarded >= currentMissions[slot].initialShipCount)
                currentMissionObjectives[slot].objectivesDone = 1;
            if (currentMissions[slot].shipGoal == 3) {
                if (currentMissions[slot].specialShipsKilled > 0
                        || currentMissions[slot].specialShipsDisabled > 0) {
                    currentMissionObjectives[slot].failed = 1;
                } else {
                    count = 0;
                    for (i = 1; i < 0x40; i++) {
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
            if (currentMissions[slot].shipGoal == 4
                    && currentMissionObjectives[slot].objectivesDone == 0
                    && gPause == 0) {
                syst = currentMissions[slot].shipSyst;
                if (syst < 0 || syst >= 0x800)
                    syst = -1;
                else
                    syst = FindActiveCoLocatedSystem(syst);
                if ((syst == shipData[0].curSystem
                        || currentMissions[slot].shipSyst == -6)
                        && currentMissions[slot].specialShipsJumpedIn >= currentMissions[slot].shipCount) {
                    for (i = 1; i < 0x40; i++) {
                        if (shipData[i].active != 0
                                && slot == shipData[i].specialMission
                                && shipData[i].sprite != 0
                                && ShipVisibleToShip(&shipData[i], shipData) != 0) {
                            if (ShipCanCloak(&shipData[i]) == 0) {
                                currentMissionObjectives[slot].objectivesDone = 1;
                                break;
                            }
                            vr = &gSpriteWorld->windRect;
                            vis.top = vr->top;
                            vis.left = vr->left;
                            vis.bottom = vr->bottom;
                            vis.right = vr->right;
                            sr = (Rect *)(shipData[i].sprite + 0x1a);
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
                        }
                    }
                }
            }
            if (currentMissions[slot].shipGoal == 6) {
                if (currentMissions[slot].specialShipsKilled + currentMissions[slot].specialShipsJumpedOut
                        >= currentMissions[slot].initialShipCount)
                    currentMissionObjectives[slot].objectivesDone = 1;
                else
                    currentMissionObjectives[slot].objectivesDone = 0;
            }
        }
    } else {
        currentMissionObjectives[slot].objectivesDone = 1;
    }
    if (currentMissionObjectives[slot].failed == 0 && currentMissions[slot].timeLimit <= 0
            && currentMissions[slot].timeLimit > -32000 && portDialog == 0) {
        currentMissionObjectives[slot].failed = 1;
        if ((currentMissionObjectives[slot].flags & 0x400) == 0) {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, 285);
            DisplayComm(buf, 0xf0, 0, 0xc, &rgbWhite, 0, 0);
        }
        QuickMissionFailure(slot);
    }
    if (currentMissionObjectives[slot].failed == 0
            && currentMissionObjectives[slot].objectivesDone != 0
            && (char)prevDone != currentMissionObjectives[slot].objectivesDone) {
        if (currentMissions[slot].shipDoneDesc != -1) {
            ShowTheCursor();
            strncpy(briefingTextPtr, "", 3);
            LoadDescription(briefingTextPtr, currentMissions[slot].shipDoneDesc);
            MungeBriefing(0, slot);
            strncpy(gTextShowString, briefingTextPtr, 0x1ffe);
            ShowTextDialog(0, 0, 1);
        }
        if (currentMissions[slot].shipGoal != -1) {
            crashlog_mission_bit_set("mission ship goal complete",
                                     currentMissions[slot].name, 1,
                                     currentMissions[slot].misnIndex + 128,
                                     currentMissions[slot].shipDoneSetString);
            EvalCurrentMissionBitSetString(currentMissions[slot].shipDoneSetString, slot);
        }
        if ((currentMissions[slot].flags & 1) != 0)
            AutoAbortMission(slot);
    }
}

/* Mac OS DateTimeRec: 14 bytes, passed to DateToLongString BY VALUE, which is
   why the exe pushes three dwords and one word for it. */

/* the offered-mission summary, stride 0x20 */



#define kBriefingSize   0x2000
#define kNameLen        0x3f    /* strncpy budget for a 64-byte token buffer */
#define kNoRankIndex    341
#define kPctOfCashBase  40000   /* pay -40001..-40099 is N% of the player's cash */
#define kOwedBase       50000   /* pay below -50000 is a debt of the excess */

/* MungeBriefing (mac _MungeBriefing). STATE: MATCH 100.0%, exe 1523 / obj 1523 instrs,
   byte-identical (masked, 6268 bytes). OPEN: none.
   see notes */
// FUNCTION: EVNOVA 0x004444f0
void MungeBriefing(char briefing, short which)
{
    char playerNameStr[64];
    char playerShipStr[64];
    char nickStr[64];
    char shipMsgStr[64];
    Str255 numStr;
    char personStr[64];
    char shipTypeStr[64];
    char deadlineStr[64];
    char tokenStr[64];
    char srkStr[64];
    char rank3Str[64];
    char rank4Str[64];
    char rrkStr[128];
    char prkStr[64];
    char cargoQtyStr[64];
    char payStr[64];
    char retSysStr[64];
    char retStelStr[64];
    char destStelStr[64];
    char cargoStr[64];
    char destSysStr[64];
    short i;
    short len;
    short sys;
    short bestIdx;
    short bestVal;
    long pay;
    long pct;

    stringHandle = NULL;
    stringHandle = AllocAHandle2(strlen(briefingTextPtr), 0);
    if (stringHandle == NULL)
        return;
    memmove(*(Handle)stringHandle, briefingTextPtr, GetAHandleSize(stringHandle));
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
                strncpy(destSysStr, (char *)gSystems[sys].name, kNameLen);
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
                strncpy(retSysStr, (char *)gSystems[sys].name, kNameLen);
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
                len = strlen(cargoStr);
                for (i = 0; i < len; i++)
                    cargoStr[i] = cargoStr[i + 1];
            }
            NumToString(briefMissionInfo[which].cargoUnits, numStr);
            strncpy(cargoQtyStr, (char *)numStr, kNameLen);
            PtoCstr((unsigned char *)cargoQtyStr);
        }

        if (gameTime.month != briefMissionInfo[which].dueDate.month ||
            gameTime.day != briefMissionInfo[which].dueDate.day ||
            gameTime.year != briefMissionInfo[which].dueDate.year)
            DateToLongString(briefMissionInfo[which].dueDate, deadlineStr);

        pay = 0;
        if (briefMissionInfo[which].payment > 0) {
            pay = briefMissionInfo[which].payment;
        } else if (briefMissionInfo[which].payment <= -(kPctOfCashBase + 1) &&
                   briefMissionInfo[which].payment > -(kPctOfCashBase + 100)) {
            pct = abs(briefMissionInfo[which].payment);
            pct -= kPctOfCashBase;
            pay = (shipData[0].credits * (pct * 0.01));
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
            if (stellarData[currentMissions[which].travelStellar].system < 0 ||
                stellarData[currentMissions[which].travelStellar].system >= kMaxSystems) {
                sys = -1;
            } else {
                sys = FindActiveCoLocatedSystem(
                          stellarData[currentMissions[which].travelStellar].system);
                if (sys == -1)
                    sys = FindFirstCoLocatedSystem(
                              stellarData[currentMissions[which].travelStellar].system);
            }
            if (sys == -1)
                sys = FindSystemFromStellar(currentMissions[which].travelStellar);
            if (sys != -1) {
                strncpy(destSysStr, (char *)gSystems[sys].name, kNameLen);
                PtoCstr((unsigned char *)destSysStr);
            }
        }
        if (currentMissions[which].returnStellar >= 0 &&
            currentMissions[which].returnStellar < kMaxStellars) {
            strncpy(retStelStr,
                    (char *)stellarData[currentMissions[which].returnStellar].name, kNameLen);
            PtoCstr((unsigned char *)retStelStr);
            if (stellarData[currentMissions[which].returnStellar].system < 0 ||
                stellarData[currentMissions[which].returnStellar].system >= kMaxSystems) {
                sys = -1;
            } else {
                sys = FindActiveCoLocatedSystem(
                          stellarData[currentMissions[which].returnStellar].system);
                if (sys == -1)
                    sys = FindFirstCoLocatedSystem(
                              stellarData[currentMissions[which].returnStellar].system);
            }
            if (sys == -1)
                sys = FindSystemFromStellar(currentMissions[which].returnStellar);
            if (sys != -1) {
                strncpy(retSysStr, (char *)gSystems[sys].name, kNameLen);
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
                len = strlen(cargoStr);
                for (i = 0; i < len; i++)
                    cargoStr[i] = cargoStr[i + 1];
            }
            NumToString(currentMissions[which].cargoQty, numStr);
            strncpy(cargoQtyStr, (char *)numStr, kNameLen);
            PtoCstr((unsigned char *)cargoQtyStr);
        }

        if (gameTime.month != currentMissionObjectives[which].dueDate.month ||
            gameTime.day != currentMissionObjectives[which].dueDate.day ||
            gameTime.year != currentMissionObjectives[which].dueDate.year)
            DateToLongString(currentMissionObjectives[which].dueDate, deadlineStr);

        strncpy(shipMsgStr, currentMissions[which].shipName, kNameLen);
        PtoCstr((unsigned char *)shipMsgStr);

        if (currentMissions[which].payment > 0) {
            pay = currentMissions[which].payment;
        } else if (currentMissions[which].payment <= -(kPctOfCashBase + 1) &&
                   currentMissions[which].payment > -(kPctOfCashBase + 100)) {
            pct = abs(currentMissions[which].payment);
            pct -= kPctOfCashBase;
            pay = (shipData[0].credits * (pct * 0.01));
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
        shipData[offeringShipIndex].persType >= 0 &&
        shipData[offeringShipIndex].persType < kMaxPersons) {
        strncpy(personStr,
                (char *)personData[shipData[offeringShipIndex].persType].name, kNameLen);
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
    strncpy(shipTypeStr, (char *)gShipTypes[shipData[0].type].name, kNameLen);
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
    if (bestIdx == -1) {
        strcpy(prkStr, "");
        AppendIndString(prkStr, kGameStrList, kNoRankIndex);
    } else {
        strcpy(prkStr, rankData[bestIdx].convName);
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
    if (bestIdx == -1) {
        strcpy(srkStr, "");
        AppendIndString(srkStr, kGameStrList, kNoRankIndex);
    } else {
        strcpy(srkStr, rankData[bestIdx].shortName);
    }

    bestIdx = -1;
    if (gDescRankGovt[0] != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                gDescRankGovt[0] == rankData[i].govt &&
                strlen(rankData[i].shortName) != 0 &&
                (rankData[i].priority > bestVal || bestIdx == -1)) {
                bestIdx = i;
                bestVal = rankData[i].priority;
            }
        }
    }
    if (bestIdx == -1) {
        strcpy(rank3Str, "");
        AppendIndString(rank3Str, kGameStrList, kNoRankIndex);
    } else {
        strcpy(rank3Str, rankData[bestIdx].shortName);
    }

    bestIdx = -1;
    if (gDescRankGovt[1] != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                gDescRankGovt[1] == rankData[i].govt &&
                strlen(rankData[i].shortName) != 0 &&
                (rankData[i].priority > bestVal || bestIdx == -1)) {
                bestIdx = i;
                bestVal = rankData[i].priority;
            }
        }
    }
    if (bestIdx == -1) {
        strcpy(rank4Str, "");
        AppendIndString(rank4Str, kGameStrList, kNoRankIndex);
    } else {
        strcpy(rank4Str, rankData[bestIdx].shortName);
    }

    if (recentActivatedRank == -1) {
        strcpy(rrkStr, "");
        AppendIndString(rrkStr, kGameStrList, kNoRankIndex);
    } else {
        strcpy(rrkStr, rankData[recentActivatedRank].name);
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

    if (gDescRankGovt[0] != -1) {
        sprintf(tokenStr, "<PRK%i>", gDescRankGovt[0] + kFirstResourceID);
        MungeForSymbol(tokenStr, rank4Str);
    }
    if (gDescRankGovt[1] != -1) {
        sprintf(tokenStr, "<SRK%i>", gDescRankGovt[1] + kFirstResourceID);
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

    if (GetAHandleSize(stringHandle) <= kBriefingSize) {
        memmove(briefingTextPtr, *(Handle)stringHandle, GetAHandleSize(stringHandle));
    } else {
        memmove(briefingTextPtr, *(Handle)stringHandle, kBriefingSize);
    }
    DeallocAHandle(stringHandle);
    stringHandle = NULL;
}

/* MungeForSymbol. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00445d70
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


#define kMissionFlagExcluded 0x400   /* MissionState.flags bit (TBC) */
#define kMissionInfoTextSize 0xfa

/* SetupMissionInfoList (mac _SetupMissionInfoList). STATE: MATCH 100.0%, exe 225 / obj 225,
   relocregcheck 0 of 52; the scrollbar test is written <= first. OPEN: none */
// FUNCTION: EVNOVA 0x00445dc0
void SetupMissionInfoList(void)
{
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
    long len;

    for (i = 0; i < kMaxActiveMissions; i++)
        activeMissions[i] = -1;

    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0
                && (currentMissionObjectives[i].flags & kMissionFlagExcluded) == 0) {
            activeMissions[count] = i;
            count++;
        }
    }

    SetDialogGWorld(missionInfoDialog);
    GetDialogItem(missionInfoDialog, 2, &itemType, &itemHandle, &box);
    SetRect(&cellBounds, 0, 0, 1, count);

    TextFont(smallTextFont);
    TextSize(smallTextSize);

    cellSize.v = (smallTextSize * 1.5);

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
                || (currentMissionObjectives[i].flags & kMissionFlagExcluded) != 0)
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
#define kMissionFlagHasMap   0x100
#define kMissionFlagPenalty  0x40

/* DoMissionInfoDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00446150
void DoMissionInfoDialog(void)
{
    Point cell;
    short itemHit;
    char done;
    short i;
    short count;
    short n;
    short mission;
    short stellar;
    short savedX;
    short savedY;

    done = 0;
    if (screenFaded != 0)
        return;
    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    missionInfoDialog = NULL;
    missionInfoDialog = GetNewDialog(kMissionInfoDlogID, UpdateMissionInfoDialog,
                                     MissionInfoFilter);
    if (missionInfoDialog == NULL)
        return;
    gMissionInfoPict = GetPicture(kMissionInfoPictID);
    CenterWindow(FrontWindow(missionInfoDialog), gWind);
    ShowWindowNoFlash(FrontWindow(missionInfoDialog));
    SetDialogGWorld(missionInfoDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(missionInfoDialog);
    SetupMissionInfoList();
    gMissionInfoIndex = 0;
    LoadDescription(briefingTextPtr,
                    currentMissions[activeMissions[gMissionInfoIndex]].quickBriefDesc);
    MungeBriefing(0, activeMissions[gMissionInfoIndex]);
    showAbortMissionButton =
        currentMissions[activeMissions[gMissionInfoIndex]].canAbort;
    UpdateMissionInfoDialog();
    FrontWindow(missionInfoDialog);
    FrontWindow(missionInfoDialog);
    do {
        if (gQuit != 0)
            done = 1;
        RunModalDialog(missionInfoDialog, MissionInfoFilter, &itemHit, 0);
        if (itemHit == 1)
            done = 1;
        if (itemHit == 6 && missionDialog == NULL) {
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 0;
            if (gMissionInfoIndex != -1) {
                mission = activeMissions[gMissionInfoIndex];
                if ((currentMissions[mission].flags & kMissionFlagHasMap) != 0) {
                    if (currentMissions[mission].travelStellar != -1) {
                        stellar = currentMissions[mission].travelStellar;
                        greenMissionSyst = stellarData[stellar].system;
                    } else if (currentMissions[mission].returnStellar != -1) {
                        stellar = currentMissions[mission].returnStellar;
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
                UpdateStellarDialog();
            }
            greenMissionSyst = -1;
            multiArrowsOnSingleSystem = 1;
            SetDialogGWorld(missionInfoDialog);
            RedrawDialog(missionInfoDialog);
            UpdateMissionInfoDialog();
        }
        if (itemHit == 5) {
            if (gMissionInfoIndex == -1) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                DrawMissionInfoButtons(-1);
            } else {
                if (currentMissions[activeMissions[gMissionInfoIndex]].canAbort != 0) {
                    if ((currentMissions[activeMissions[gMissionInfoIndex]].flags & kMissionFlagPenalty) != 0
                        && currentMissions[activeMissions[gMissionInfoIndex]].compGovt != -1) {
                        for (i = 0; i < kMaxSystems; i++) {
                            if (gSystems[i].govt == currentMissions[activeMissions[gMissionInfoIndex]].compGovt)
                                playerLegalStatus[i] -=
                                    currentMissions[activeMissions[gMissionInfoIndex]].compReward * 5;
                        }
                    }
                    AbortMission(activeMissions[gMissionInfoIndex], 1);
                    count = 0;
                    for (i = 0; i < kMaxActiveMissions; i++) {
                        if (currentMissionObjectives[i].active != 0)
                            count++;
                    }
                    if (count > 0) {
                        if (missionInfoList != NULL)
                            LDispose(missionInfoList);
                        SetupMissionInfoList();
                        gMissionInfoIndex = -1;
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
    GetDialogPortRect(missionInfoDialog);
    GetDialogPortRect(missionInfoDialog);
    GetDialogPortRect(missionInfoDialog);
    GetDialogPortRect(missionInfoDialog);
    DisposeDialogNoFlash(missionInfoDialog);
    missionInfoDialog = NULL;
    if (gMissionInfoPict != NULL) {
        QDReleaseResource(gMissionInfoPict);
        gMissionInfoPict = NULL;
    }
    RedrawDialogs(1);
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
}

/* MissionInfoFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00446770
char MissionInfoFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    Point cell;
    void *item;
    short old;
    int c;
    short i;
    short slot;
    short count;
    short button;

    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map)) {
            *itemHit = 6;
            return 1;
        }
        c = ev->message & kCharCodeMask;
        if ((int)ev->what == kKeyDown) {
            if (c == kReturnChar || c == kEscapeChar || KeyCheck(keyPrefs.missions)) {
                *itemHit = 1;
                return 1;
            }
        }
        if (c == kTabChar || c == kDownArrowChar) {
            cell.h = 0;
            cell.v = gMissionInfoIndex;
            if (gMissionInfoIndex != -1) {
                LSetSelect(0, cell, missionInfoList);
                GetListCellRect(&box, cell, missionInfoList);
                if (box.bottom >= (*missionInfoList)->rView.bottom)
                    LScroll(0, 1, missionInfoList);
            }
            old = gMissionInfoIndex;
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0)
                    count++;
            }
            if (count > 0) {
                i = gMissionInfoIndex;
                do {
                    i++;
                    if (i >= kMaxActiveMissions)
                        i = 0;
                } while (activeMissions[i] == -1);
                gMissionInfoIndex = i;
            }
            strncpy(briefingTextPtr, "", 3);
            if (gMissionInfoIndex == -1) {
                showAbortMissionButton = 0;
            } else {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[gMissionInfoIndex]].quickBriefDesc);
                MungeBriefing(0, activeMissions[gMissionInfoIndex]);
                cell.v = gMissionInfoIndex;
                LSetSelect(1, cell, missionInfoList);
                LAutoScroll(missionInfoList);
                showAbortMissionButton =
                    currentMissions[activeMissions[gMissionInfoIndex]].canAbort;
            }
            if (gMissionInfoIndex != old) {
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
            }
        }
        if (c == kUpArrowChar) {
            cell.h = 0;
            cell.v = gMissionInfoIndex;
            if (gMissionInfoIndex != -1) {
                LSetSelect(0, cell, missionInfoList);
                GetListCellRect(&box, cell, missionInfoList);
                if (box.top <= (*missionInfoList)->rView.top)
                    LScroll(0, -1, missionInfoList);
            }
            old = gMissionInfoIndex;
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0)
                    count++;
            }
            if (count > 0) {
                slot = gMissionInfoIndex;
                do {
                    slot--;
                    if (slot < 0)
                        slot = kMaxActiveMissions - 1;
                } while (activeMissions[slot] == -1);
                gMissionInfoIndex = slot;
            }
            strncpy(briefingTextPtr, "", 3);
            if (gMissionInfoIndex == -1) {
                showAbortMissionButton = 0;
            } else {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[gMissionInfoIndex]].quickBriefDesc);
                MungeBriefing(0, activeMissions[gMissionInfoIndex]);
                cell.v = gMissionInfoIndex;
                LSetSelect(1, cell, missionInfoList);
                LAutoScroll(missionInfoList);
                showAbortMissionButton =
                    currentMissions[activeMissions[gMissionInfoIndex]].canAbort;
            }
            if (gMissionInfoIndex != old) {
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        GetDialogItem(missionInfoDialog, 2, &itemType, &item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            LClick(where, ev->modifiers, missionInfoList);
            cell.h = 0;
            cell.v = 0;
            old = gMissionInfoIndex;
            gMissionInfoIndex = -1;
            for (; cell.v < (*missionInfoList)->dataBounds.bottom; cell.v++) {
                if (LGetSelect(0, &cell, missionInfoList)) {
                    gMissionInfoIndex = cell.v;
                    break;
                }
            }
            strncpy(briefingTextPtr, "", 3);
            if (gMissionInfoIndex == -1) {
                showAbortMissionButton = 0;
            } else {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[gMissionInfoIndex]].quickBriefDesc);
                MungeBriefing(0, activeMissions[gMissionInfoIndex]);
                showAbortMissionButton =
                    currentMissions[activeMissions[gMissionInfoIndex]].canAbort;
            }
            if (gMissionInfoIndex != old) {
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
            }
        }
        UpdateMissionInfoDialog();
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
        UpdateMissionInfoDialog();
    }
    return 0;
}

/* UpdateMissionInfoDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00446e00
void UpdateMissionInfoDialog(void)
{
    short itemType;
    void *item;
    char text[256];
    Rect box;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionInfoDialog));
    if (gMissionInfoPict != NULL)
        DrawPicHandle(gMissionInfoPict, GetDialogPortRect(missionInfoDialog));
    RGBForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 3, &itemType, &item, &box);
    RGBForeColor(&rgbMedGrey);
    MoveTo(box.left, box.bottom);
    DrawIndString(kGameStrList, kDateCaptionIdx);
    RGBForeColor(blackColor);
    RGBForeColor(&rgbDarkGrey);
    DateToMedString(gameTime, text);
    CtoPstr(text);
    GetDialogItem(missionInfoDialog, 7, &itemType, &item, &box);
    MoveTo(((box.right + box.left) / 2 -
                   StringWidth((unsigned char *)text) / 2),
           (box.top + smallTextSize));
    DrawString((unsigned char *)text);
    RGBForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 4, &itemType, &item, &box);
    if (RectInRgn(&box, GetCWindowVisRgn(FrontWindow(missionInfoDialog)))) {
        if (gMissionInfoIndex != -1) {
            RGBForeColor(blackColor);
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            TETextBox(briefingTextPtr, strlen(briefingTextPtr), &box, 0);
            InvertRect(&box);
        } else {
            RGBForeColor(blackColor);
            PaintRect(&box);
        }
    }
    DrawMissionInfoButtons(-1);
    if (missionInfoList != NULL)
        LUpdate(missionInfoList);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(missionInfoDialog);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(missionInfoDialog)),
             GetDialogPortRect(missionInfoDialog),
             GetDialogPortRect(missionInfoDialog), 0,
             GetCWindowVisRgn(FrontWindow(missionInfoDialog)));
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
    if (missionInfoList != NULL) {
        SetDialogGWorld(missionInfoDialog);
        LUpdate(missionInfoList);
    }
}

/* SingleMissionFilter. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447170
char SingleMissionFilter(long unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    void *item;
    short button;
    int c;
    int what;
    int acceptKey;
    int refuseKey;

    QuickKeyCheckHandler();
    if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
        *itemHit = 4;
        return 1;
    }
    if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
        *itemHit = 5;
        return 1;
    }
    if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
        *itemHit = 7;
        return 1;
    }
    what = ev->what;
    if (what == kKeyDown || what == kAutoKey) {
        c = tolower(ev->message & kCharCodeMask);
        if (what == kKeyDown) {
            if (c == kReturnChar) {
                *itemHit = 1;
                return 1;
            }
            refuseKey = missionButtonLabel[1][1];
            acceptKey = missionButtonLabel[0][1];
            if (tolower(acceptKey) != tolower(refuseKey)) {
                if (c == tolower(acceptKey)) {
                    *itemHit = 1;
                    return 1;
                }
                if (canRefuseMission != 0 && c == tolower(refuseKey)) {
                    *itemHit = 2;
                    return 1;
                }
            }
        }
        if (canScrollTextUp != 0) {
            if (c == kUpArrowChar) {
                *itemHit = 9;
                return 1;
            }
            if (c == kHomeChar) {
                ScrollDialogText(missionDialog, 3, gMissionTE, kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextUp = 0;
                return 1;
            }
            if (c == kPageUpChar) {
                ScrollDialogText(missionDialog, 3, gMissionTE, kScrollStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
        if (canScrollTextDown != 0) {
            if (c == kDownArrowChar) {
                *itemHit = 10;
                return 1;
            }
            if (c == kEndChar) {
                ScrollDialogText(missionDialog, 3, gMissionTE, -kScrollToEnd, 1);
                *itemHit = -1;
                canScrollTextDown = 0;
                return 1;
            }
            if (c == kPageDownChar) {
                ScrollDialogText(missionDialog, 3, gMissionTE, -kScrollStep, 1);
                *itemHit = -1;
                return 1;
            }
        }
    }
    if (what == kMouseDown) {
        where.v = ev->where.v;
        where.h = ev->where.h;
        if (canScrollTextUp != 0) {
            GetDialogItem(missionDialog, 9, &itemType, &item, &box);
            if (PtInRect(where.h, where.v, &box) && canScrollTextUp != 0) {
                lastTextScrollTime = ReadTick();
                while (StillDown()) {
                    BusyIdle();
                    ScrollDialogText(missionDialog, 3, gMissionTE, 1, 1);
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
            GetDialogItem(missionDialog, 10, &itemType, &item, &box);
            if (PtInRect(where.h, where.v, &box) && canScrollTextDown != 0) {
                lastTextScrollTime = ReadTick();
                while (StillDown()) {
                    BusyIdle();
                    ScrollDialogText(missionDialog, 3, gMissionTE, -1, 1);
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
        UpdateMissionDialog();
        button = TrackSingleMissionButtons(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 2;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt) {
        SetDialogGWorld(missionDialog);
        UpdateMissionDialog();
    }
    return 0;
}
/* UpdateMissionDialog. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447680
void UpdateMissionDialog(void)
{
    char inside;
    void *loaded;
    void *pic;
    Rect r;
    Rect r2;
    short itemType;
    void *item;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    RGBForeColor(blackColor);
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
            OffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                       GetDialogPortRect(missionDialog)->top);
            DrawPicHandle(missionBackgroundPic[1], &r);
        }
        if (missionBackgroundPic[0] != NULL) {
            QDGetPictureBounds(missionBackgroundPic[0], &r);
            OffsetRect(&r, -r.left, -r.top);
            OffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                       GetDialogPortRect(missionDialog)->top);
            DrawPicHandle(missionBackgroundPic[0], &r);
        }
        pic = missionBackgroundPic[2];
        if (pic != NULL) {
            QDGetPictureBounds(pic, &r);
            OffsetRect(&r, -r.left, -r.top);
            OffsetRect(&r, GetDialogPortRect(missionDialog)->left,
                       GetDialogPortRect(missionDialog)->bottom - (r.bottom - r.top));
            DrawPicHandle(missionBackgroundPic[2], &r);
        }
    } else if (missionBackgroundPic[0] != NULL) {
        DrawPicHandle(missionBackgroundPic[0], GetDialogPortRect(missionDialog));
    }
    DrawSingleMissionButtons(-1);
    if (descResourcePictId >= kFirstResourceID) {
        GetDialogItem(missionDialog, 8, &itemType, &item, &r);
        inside = RectInRgn(&r, GetCWindowVisRgn(FrontWindow(missionDialog)));
        if (inside) {
            loaded = GetPicture(descResourcePictId);
            if (loaded != NULL) {
                DrawPicHandle(loaded, &r);
                QDReleaseResource(loaded);
            }
        }
    }
    (*gMissionTE)->port = loadPort.world;
    GetDialogItem(missionDialog, 3, &itemType, &item, &r);
    RGBForeColor(whiteColor);
    BackColor(blackColor);
    TEUpdate(&r, gMissionTE);
    RGBForeColor(blackColor);
    BackColor(whiteColor);
    (*gMissionTE)->port = GetDialogPort(missionDialog);
    SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(missionDialog);
    CopyBits(GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetDialogPort(missionDialog)),
             GetDialogPortRect(missionDialog), GetDialogPortRect(missionDialog), 0,
             GetCWindowVisRgn(FrontWindow(missionDialog)));
}

// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* validauxsys -- may a mission's auxiliary ships appear in this system? (win 0x00447a30, mac twin _ValidAuxShipSystem @ 0x000992b5). Notes: Research/notes/validauxsys.md */

/* mïsn runtime record window, stride 0x8e6; byte-packed, since the exe reads
   a word at the odd offset 0x65 (mission.c's copy carries the full layout) */

/* only the player slot is read, so no stride is needed here */

/* ValidAuxShipSystem. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447a30
char ValidAuxShipSystem(short sys, short mission)
{
    short spec;
    short i;
    long xenophobic;

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
    } else if (spec >= 128 && spec < 2176) {
        if (sys == spec - 128)
            return 1;
        if (sys == FindActiveCoLocatedSystem(spec - 128))
            return 1;
    } else if (spec >= 5000 && spec < 9999) {
        if (sys == spec - 5000)
            return 1;
        for (i = 0; i < kNavSlots; i++) {
            if (sys == gSystems[spec - 5000].links[i])
                return 1;
        }
    } else if (spec >= 9999 && spec < 15000) {
        if (spec - 10000 == gSystems[sys].govt)
            return 1;
    } else if (spec >= 15000 && spec < 20000) {
        if (gSystems[sys].govt >= 0) {
            if (spec - 15000 == gSystems[sys].govt)
                return 1;
            if (GovtAllies(gSystems[sys].govt, spec - 15000))
                return 1;
        }
    } else if (spec >= 20000 && spec < 25000) {
        if (spec - 20000 != gSystems[sys].govt)
            return 1;
    } else if (spec >= 25000 && spec < 30000) {
        if (gSystems[sys].govt >= 0) {
            xenophobic = govtData[spec - 25000].flags & kGovtXenophobic;
            if (xenophobic && spec - 25000 != gSystems[sys].govt &&
                !GovtAllies(gSystems[sys].govt, spec - 25000))
                return 1;
            if (GovtEnemies(gSystems[sys].govt, spec - 25000))
                return 1;
        }
    } else if (spec >= 30000 && spec < 31000) {
        if (gSystems[sys].govt >= 0) {
            if (GovtSharedClass(gSystems[sys].govt, spec - 30000))
                return 1;
        }
    } else if (spec >= 31000 && spec < 32000) {
        if (gSystems[sys].govt >= 0) {
            if (!GovtSharedClass(gSystems[sys].govt, spec - 31000))
                return 1;
        }
    }
    return 0;
}

#define kMissionCostsFuel   0x0008 /* flags55 */
#define kMissionPaysOnAbort 0x0002 /* flags57 */

/* AutoAbortMission (mac _AutoAbortMission). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447d90
void AutoAbortMission(short mission)
{
    short i;

    crashlog_mission_bit_set("auto-aborting mission", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + 128,
                             currentMissions[mission].abortSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].abortSetString, mission);
    if (currentMissions[mission].datePostIncrement > 0) {
        crashlog_print_mdy(&gameTime);
        crashlog_print(": incrementing date ");
        crashlog_print_num(currentMissions[mission].datePostIncrement);
        crashlog_print(" days for mission ");
        crashlog_print_num(currentMissions[mission].misnIndex + 128);
        crashlog_print(" auto-abort post increment\n");
        for (i = 0; i < currentMissions[mission].datePostIncrement; i++)
            IncrementGameTime();
    }
    if (currentMissions[mission].flags & kMissionCostsFuel) {
        shipData[0].fuel -= 100.0f;
        statusRedraw[4] = 1;
    }
    if (currentMissions[mission].flags2 & kMissionPaysOnAbort)
        ApplyMissionPay(currentMissions[mission].payment);
    AbortMission(mission, 0);
}

/* SystemIsLoaded. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447f00
char SystemIsLoaded(short sys)
{
    return gSystems[sys].loaded;
}

/* EvalMissionBitTestString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00447f20
char EvalMissionBitTestString(char *expr)
{
    short i;
    int out;
    char prev;
    short len;
    short err;
    int ok;

    if (strlen(expr) == 0)
        return 1;
    if (expr[0] != 'b' && expr[0] != 'B' && expr[0] != '(' && expr[0] != '!'
            && expr[0] != 'P' && expr[0] != 'p' && expr[0] != 'G' && expr[0] != 'g'
            && expr[0] != 'O' && expr[0] != 'o' && expr[0] != 'E' && expr[0] != 'e')
        return 0;
    len = strlen(expr);
    out = 0;
    prev = ' ';
    for (i = 0; i < len; i++) {
        if (expr[i] == '(' && prev == '(') {
            parseBuffer[(short)out] = ' ';
            out++;
        }
        parseBuffer[(short)out] = expr[i];
        out++;
        prev = expr[i];
    }
    parseBuffer[(short)out] = 0;
    err = 0;
    if (EvalTestExp(&err) == 1)
        ok = 1;
    else
        ok = 0;
    return ok;
}
