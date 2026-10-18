// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* loaddata.c -- pilot-folder setup, OpenDataFiles, LoadObjectData, LoadStrings, LoadPrefs/SavePrefs, the pilot file, the resource utilities, the chär record readers and VerifyResourceChecksums. Notes: Research/notes/loaddata.md */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "gameconst.h"
#include "gametypes.h"
#include "render.h"
#include "crashlog.h"
#include "player.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "stellar.h"
#include "burger.h"
#include "macport.h"
#include "resources.h"
#include "sound.h"
#include "ships.h"
#include "ai.h"
#include "shipobj.h"
#include "missions.h"
#include "space.h"
#include "pilotfile.h"
#include "prototypes.h"

#define kPrefStringMax 0x100

/*
 * Set up the pilot-file directory at startup (win 0x004bd0c0): "9:" plus
 * string 3 of list 130 plus ":", which is the port's volume notation for a
 * folder beside the application. The two debug switches are read here because
 * this is the first thing that runs after the resource file opens.
 */
/* InitDataFiles. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bd0c0
void InitDataFiles(void)
{
    char name[kPrefStringMax];

    noSavePilotFiles = EVBugResourceFieldTest(12);
    noUpdatePilotData = EVBugResourceFieldTest(13);
    GetIniIndString(name, kPilotDirList, 3);
    strcpy(pilotsFolder, "9:");
    strcat(pilotsFolder, name);
    strcat(pilotsFolder, ":");
    strcpy(gPrefsDirectory, "@:");
    lastIntfResourceID = -1;
}

/* prints/logs the Pascal string GetIndString just filled; NOT the same
   address as crashlog_print_pstr (0x00470150) despite the similar role */
/* called with a constant 0 at every site here; tail-calls through a lazy
   singleton getter (0x52ce50) into 0x4fdf10 */
/* CreateDirectoryPath is BurgerLib's CreateDirectoryPath: ExpandAPathToBufferNative
   (0x004d9680) then CreateDirectoryPathNative (0x004d9530); nonzero on failure */

static __inline short SwapEndianShort(const unsigned short *p)
{
    register unsigned short t;

    t = *p;
    asm { ror t, 8 }
    return t;
}

#define kAlertStrList 0x8c

/* OpenDataFiles (mac _OpenDataFiles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bd160
void OpenDataFiles(void)
{
    char name[kPrefStringMax];
    char path[0x800];
    unsigned char str[kPrefStringMax];
    Handle h;
    short count;

    GetIniIndString(name, kPilotDirList, 3);
    strcpy(path, "9:");
    strcat(path, name);
    if (CreateDirectoryPath(path) != 0) {
        FadeScreenIn();
        GetIndString(str, kAlertStrList, 0x1d);
        EVNoteAlert(str);
        exit(0);
    }

    crashlog_print("opening files in ");
    crashlog_print("Nova Files");
    crashlog_print(" folder:\n");

    if (OpenEVPlugins("Nova Files", ".rez", NULL) != 0) {
        CreateDirectoryPath("Nova Files");
        if (OpenEVPlugins("Nova Files", ".rez", NULL) != 0) {
            FadeScreenIn();
            GetIndString(str, kAlertStrList, 0x1e);
            EVNoteAlert(str);
            exit(0);
        }
    }

    if (gShipTypes[gameIsRegisteredIndex].registered != 0 ? 1 : 0) {
        if (gPluginCount > 0) {
            crashlog_print("opening files in ");
            crashlog_print("Nova Plug-Ins");
            crashlog_print(" folder:\n");
        }
        if (OpenEVPlugins("Nova Plug-Ins", ".rez", NULL) != 0) {
            CreateDirectoryPath("Nova Plug-Ins");
            if (OpenEVPlugins("Nova Plug-Ins", ".rez", NULL) != 0) {
                FadeScreenIn();
                GetIndString(str, kAlertStrList, 0x1f);
                EVNoteAlert(str);
                exit(0);
            }
        }
    }

    if (crashlog_isactive()) {
        h = GetResource(kStringListType, kGameStrList);
        if (h != NULL) {
            count = SwapEndianShort(&((StringListRec *)*h)->count);
            ReleaseResource(h);
            if (count != 0x18c) {
                crashlog_print("warning: STR# resource ");
                crashlog_print_num(kGameStrList);
                crashlog_print(" is missing strings (");
                crashlog_print_num(0x18c - count);
                crashlog_print(")\n");
            }
        }
    }
}

/* NebuRecord is loaded without LockAHandle or AdjustHandleSize. */

#define SpobHasTradeCenter(s) ((s)->flags & kSpobTradeCenter)


/* LoadObjectData (mac _LoadObjectData). STATE: MATCH 100.0% (6170 exact, 2139 reloc-equivalent,
   0 mismatched; exe 8309 / obj 8309), byte-identical (39297 B, 2139 relocated fields masked).
   OPEN: none. Declaration order and roles carry the exe's spill numbering: the junk counter
   is its own `loaded', declared first; the weap and pers totals stay on `n'. The colour
   extracts are masks, not casts (see notes, lanes RB, RD, RE, CL3). */
// FUNCTION: EVNOVA 0x004bd3c0
void LoadObjectData(void)
{
    short loaded;
    short n;
    short j;
    short i;
    short found;
    short resID;
    unsigned long resType;
    short count;
    short mountCount[8];
    short mountID[8];
    RGBColor rgb;
    short mountAmmo[8];
    unsigned char name[256];
    char used[kMaxStellars];

    if (EVBugResourceFieldTest(0))
        return;
    SWSetPortToWindow(gSpriteWorld);

    /* ---------------- spöb: stellar objects ---------------- */
    {

        found = 0;
        count = CountResources(kSpobType);
    if (count == 0)
        crashlog("warning: no sp\x9a" "b resources found");
    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].loaded = 0;
        gSpobHandle = GetResource(kSpobType, i + kFirstResourceID);
        if (gSpobHandle != NULL) {
            LockAHandle(gSpobHandle);
            found++;
            GetResInfo(gSpobHandle, &resID, &resType, name);
            StripNameComment(name);
            strncpy((char *)stellarData[i].name, (char *)name, 31);
            stellarData[i].pos.h = *(short *)(*gSpobHandle + 0x00);
            stellarData[i].pos.v = *(short *)(*gSpobHandle + 0x02);
            stellarData[i].type = *(short *)(*gSpobHandle + 0x04);
            stellarData[i].flags = *(unsigned long *)(*gSpobHandle + 0x06);
            stellarData[i].flags2 = *(unsigned short *)(*gSpobHandle + 0x20);
            stellarData[i].govt = *(short *)(*gSpobHandle + 0x14);
            stellarData[i].minStatus = *(short *)(*gSpobHandle + 0x16);
            stellarData[i].techLevel = *(short *)(*gSpobHandle + 0x0c);
            stellarData[i].customPict = *(short *)(*gSpobHandle + 0x18);
            stellarData[i].customSound = *(short *)(*gSpobHandle + 0x1a);
            stellarData[i].landingFee = *(long *)(*gSpobHandle + 0x234);
            stellarData[i].gravity = *(short *)(*gSpobHandle + 0x238);
            stellarData[i].weapon = *(short *)(*gSpobHandle + 0x23a);
            if (stellarData[i].govt >= kFirstResourceID)
                stellarData[i].govt -= kFirstResourceID;
            else
                stellarData[i].govt = -1;
            stellarData[i].strength = *(long *)(*gSpobHandle + 0x23c);
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadType = *(short *)(*gSpobHandle + 0x240);
            stellarData[i].deadDays = *(short *)(*gSpobHandle + 0x242);
            stellarData[i].deadDaysLeft = 0;
            stellarData[i].explodType = *(short *)(*gSpobHandle + 0x244);
            if (stellarData[i].explodType < 0)
                stellarData[i].explodType = -1;
            if (stellarData[i].deadType < 0 || stellarData[i].deadType >= 256)
                stellarData[i].deadType = stellarData[i].type;
            strcpy(stellarData[i].onDestroy, *gSpobHandle + 0x246);
            strcpy(stellarData[i].onRegen, *gSpobHandle + 0x345);
            for (j = 0; j < 3; j++)
                stellarData[i].specialTech[j] = *(short *)(*gSpobHandle + 0x0e + j * 2);
            for (j = 0; j < 5; j++)
                stellarData[i].specialTech[j + 3] = *(short *)(*gSpobHandle + 0x444 + j * 2);
            stellarData[i].loaded = 1;
            stellarData[i].prepared = 1;
            stellarData[i].dominated = 0;
            stellarData[i].tribute = *(short *)(*gSpobHandle + 0x0a);
            if (stellarData[i].tribute <= 0)
                stellarData[i].tribute = stellarData[i].techLevel * 1000;
            stellarData[i].defCount = *(short *)(*gSpobHandle + 0x1e);
            stellarData[i].defenseDude = *(short *)(*gSpobHandle + 0x1c);
            stellarData[i].animDelay = *(short *)(*gSpobHandle + 0x22);
            stellarData[i].frame0Bias = *(short *)(*gSpobHandle + 0x24);
            for (j = 0; j < kSpobHyperLinks; j++) {
                stellarData[i].hyperLinks[j] = ((short *)(*gSpobHandle + 0x26))[j];
                if (stellarData[i].hyperLinks[j] >= kFirstResourceID)
                    stellarData[i].hyperLinks[j] -= kFirstResourceID;
                else
                    stellarData[i].hyperLinks[j] = -1;
            }
            if (stellarData[i].defCount <= 1000)
                stellarData[i].defWaveSize = stellarData[i].defCount;
            else if (stellarData[i].defCount > 10000)
                stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 1000;
            else
                stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 100;
            if (stellarData[i].defenseDude >= kFirstResourceID)
                stellarData[i].defenseDude -= kFirstResourceID;
            else
                stellarData[i].defenseDude = -1;
            if (stellarData[i].techLevel < 0)
                stellarData[i].techLevel = 0;
            strcpy(stellarData[i].onDominate, *gSpobHandle + 0x36);
            strcpy(stellarData[i].onRelease, *gSpobHandle + 0x135);
            if (stellarData[i].weapon >= kFirstResourceID)
                stellarData[i].weapon -= kFirstResourceID;
            else
                stellarData[i].weapon = -1;
            AdjustHandleSize(gSpobHandle, 0x45e);
            UnlockAHandle(gSpobHandle);
            ReleaseResource(gSpobHandle);
            if (SpobCanLand(&stellarData[i]) && !SpobIsHyperOrWorm(&stellarData[i]) &&
                crashlog_isactive()) {
                if (stellarData[i].customPict >= kFirstResourceID) {
                    if (!ResourceExists(kPictType, stellarData[i].customPict)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(stellarData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") custom landing picture not found\n");
                    }
                } else {
                    if (!ResourceExists(kPictType, stellarData[i].type + 10000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(stellarData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks landing picture for stellar type\n");
                    }
                }
                if (!ResourceExists(kDescType, i + kFirstResourceID)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") lacks landing description\n");
                }
                if (SpobHasBar(&stellarData[i]) &&
                    !ResourceExists(kDescType, i + 10000)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a bar but no bar description\n");
                }
            }
            HandleIntroMusic(0);
        } else {
            stellarData[i].loaded = 0;
            stellarData[i].prepared = 0;
        }
        if (found >= count)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
    }

    /* ---------------- sÿst: systems ---------------- */
    {
        short link, target, total;

        found = 0;
        count = CountResources(kSystType);
        if (count == 0)
            crashlog("warning: no s\xd8" "st resources found");
        for (i = 0; i < kMaxSystems; i++) {
            gSystems[i].loaded = 0;
            gSystems[i].prepared = 0;
            gSystems[i].govt = -32767;
            for (j = 0; j < kMaxSystemLinks; j++)
                gSystems[i].links[j] = -1;
            gSystHandle = GetResource(kSystType, i + kFirstResourceID);
            if (gSystHandle != NULL) {
                LockAHandle(gSystHandle);
                found++;
                GetResInfo(gSystHandle, &resID, &resType, name);
                StripNameComment(name);
                strncpy((char *)gSystems[i].name, (char *)name, 31);
                gSystems[i].loaded = 1;
                gSystems[i].prepared = 1;
                for (j = 0; j < 8; j++) {
                    gSystems[i].dudeTypes[j] = *(short *)(*gSystHandle + 0x44 + j * 2);
                    gSystems[i].dudeProbs[j] = *(short *)(*gSystHandle + 0x54 + j * 2);
                }
                gSystems[i].avgShips = *(short *)(*gSystHandle + 0x64);
                gSystems[i].govt = *(short *)(*gSystHandle + 0x66);
                gSystems[i].message = *(short *)(*gSystHandle + 0x68);
                gSystems[i].asteroidCount = *(short *)(*gSystHandle + 0x6a);
                gSystems[i].interference = *(short *)(*gSystHandle + 0x6c);
                if (gSystems[i].asteroidCount > 0) {
                    gSystems[i].asteroidCount *=
                        (GetCWindowPortRect(gWind)->right -
                                 GetCWindowPortRect(gWind)->left) / 640.0;
                    if (gSystems[i].asteroidCount > 16)
                        gSystems[i].asteroidCount = 16;
                } else {
                    gSystems[i].asteroidCount = 0;
                }
                gSystems[i].astTypes = *(short *)(*gSystHandle + 0x94);
                gSystems[i].xPos = *(short *)(*gSystHandle + 0x00);
                gSystems[i].yPos = *(short *)(*gSystHandle + 0x02);
                if (gSystems[i].govt >= kFirstResourceID && gSystems[i].govt < kFirstResourceID + kMaxGovts)
                    gSystems[i].govt -= kFirstResourceID;
                else
                    gSystems[i].govt = -1;
                gSystems[i].rgb.red = ((*(unsigned long *)(*gSystHandle + 0x8e) >> 16) & 0xff) << 8;
                gSystems[i].rgb.green = ((*(unsigned long *)(*gSystHandle + 0x8e) >> 8) & 0xff) << 8;
                gSystems[i].rgb.blue = (*(short *)(*gSystHandle + 0x8e) & 0xff) << 8;
                if (gSpriteWorld->pixelDepth == 8) {
                    gSystems[i].mapPixel = Color2Index(&gSystems[i].rgb);
                } else {
                    gSystems[i].rgb.red &= 0xfc00;
                    gSystems[i].rgb.green &= 0xfc00;
                    gSystems[i].rgb.blue &= 0xfc00;
                    gSystems[i].mapPixel = ((((gSystems[i].rgb.red + 0x801) >> 11) << 10) & 0x7c00) |
                                    ((((gSystems[i].rgb.green + 0x801) >> 11) << 5) & 0x3e0) |
                                    (((gSystems[i].rgb.blue + 0x801) >> 11) & 0x1f);
                }
                gSystems[i].murk = *(short *)(*gSystHandle + 0x92);
                if (gSystems[i].murk <= 100) {
                    if (gSystems[i].murk < 8 && gSystems[i].murk > 0)
                        gSystems[i].murk = 0;
                    else if (gSystems[i].murk < 0)
                        gSystems[i].murk = -1;
                } else {
                    gSystems[i].murk = 100;
                }
                total = 0;                       /* valid dude prob total */
                gSystems[i].fleetCount = 0;
                gSystems[i].fleetProbTotal = 0;
                for (j = 0; j < 8; j++) {
                    short dude = gSystems[i].dudeTypes[j];
                    if (dude >= kFirstResourceID && dude < 640) {
                        gSystems[i].dudeTypes[j] -= kFirstResourceID;
                        total += gSystems[i].dudeProbs[j];
                    } else {
                        if (dude <= -128 && dude > -384) {
                            /* negative dude entries reference fleets */
                            gSystems[i].fleets[gSystems[i].fleetCount] = abs(dude) - kFirstResourceID;
                            gSystems[i].fleetProbs[gSystems[i].fleetCount] = gSystems[i].dudeProbs[j];
                            gSystems[i].fleetCount++;
                            gSystems[i].fleetProbTotal += gSystems[i].dudeProbs[j];
                        }
                        gSystems[i].dudeTypes[j] = -1;
                        gSystems[i].dudeProbs[j] = 0;
                    }
                }
                if (total != 100 && total > 0) {
                    double scale = 100.0 / total;
                    for (j = 0; j < 8; j++)
                        gSystems[i].dudeProbs[j] *= scale;
                }
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = ((short *)(*gSystHandle + 0x04))[j];
                    if (link >= kFirstResourceID && link < kFirstResourceID + kMaxSystems)
                        gSystems[i].links[j] = link - kFirstResourceID;
                    else
                        gSystems[i].links[j] = -1;
                }
                for (j = 0; j < kNavSlots; j++) {
                    target = ((short *)(*gSystHandle + 0x24))[j];
                    if (target >= kFirstResourceID && target < kFirstResourceID + kMaxStellars) {
                        gSystems[i].navDefault[j] = target - kFirstResourceID;
                        if (!stellarData[gSystems[i].navDefault[j]].prepared) {
                            crashlog_print("warning: ignoring reference to nonexistent stellar ID ");
                            crashlog_print_num(gSystems[i].navDefault[j] + kFirstResourceID);
                            crashlog_print(" in system ");
                            crashlog_print_pstr(gSystems[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(")\n");
                            gSystems[i].navDefault[j] = -1;
                        }
                    } else {
                        gSystems[i].navDefault[j] = -1;
                    }
                }
                for (j = 0; j < 8; j++) {
                    gSystems[i].persons[j] = ((short *)(*gSystHandle + 0x6e))[j];
                    gSystems[i].persProbs[j] = ((short *)(*gSystHandle + 0x7e))[j];
                    if (gSystems[i].persons[j] >= kFirstResourceID && gSystems[i].persons[j] < 1151)
                        gSystems[i].persons[j] -= kFirstResourceID;
                    else
                        gSystems[i].persons[j] = -1;
                    if (gSystems[i].persProbs[j] < 0)
                        gSystems[i].persProbs[j] = 0;
                    if (gSystems[i].persProbs[j] > 100)
                        gSystems[i].persProbs[j] = 100;
                }
                gSystems[i].reinforceFleet = *(short *)(*gSystHandle + 0x196);
                gSystems[i].reinforceTime = *(short *)(*gSystHandle + 0x198);
                gSystems[i].reinforceInterval = *(short *)(*gSystHandle + 0x19a);
                if (gSystems[i].reinforceFleet >= kFirstResourceID) {
                    gSystems[i].reinforceFleet -= kFirstResourceID;
                    if (gSystems[i].reinforceInterval < 1)
                        gSystems[i].reinforceInterval = 1;
                } else {
                    gSystems[i].reinforceFleet = -1;
                }
                gSystems[i].reinforceTimer = 0;
                gSystems[i].reinforceDaysLeft = 0;
                strcpy(gSystems[i].visibility, *gSystHandle + 0x96);
                AdjustHandleSize(gSystHandle, 0x1ac);
                UnlockAHandle(gSystHandle);
                ReleaseResource(gSystHandle);
                HandleIntroMusic(0);
            } else {
                strcpy(gSystems[i].visibility, "");
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* co-location chains: systems sharing a map position form linked groups */
        for (i = 0; i < kMaxSystems; i++) {
            gSystems[i].colocGroup = -1;
            gSystems[i].colocNext = -1;
        }
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].loaded && gSystems[i].colocGroup == -1) {
                short prev = i;
                gSystems[i].colocGroup = i;
                for (j = 0; j < kMaxSystems; j++) {
                    if (gSystems[j].loaded &&
                        gSystems[j].xPos == gSystems[i].xPos &&
                        gSystems[j].yPos == gSystems[i].yPos &&
                        (gSystems[j].colocGroup == -1 || j > i)) {
                        gSystems[prev].colocNext = j;
                        gSystems[j].colocGroup = i;
                        prev = j;
                    }
                }
            }
        }

        /* drop links to nonexistent systems */
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = gSystems[i].links[j];
                    if (gSystems[i].links[j] != -1 && !gSystems[gSystems[i].links[j]].prepared) {
                        gSystems[i].links[j] = -1;
                        crashlog_print("warning: ignoring link from ");
                        crashlog_print_pstr(gSystems[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") to nonexistent system ");
                        crashlog_print_num(link + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
        }

        /* canonicalize links to group leaders, drop self- and duplicate links */
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].prepared) {
                for (n = 0; n < kMaxSystemLinks; n++) {
                    link = gSystems[i].links[n];
                    if (link != -1) {
                        gSystems[i].links[n] = FindFirstCoLocatedSystem(link);
                        if (gSystems[i].links[n] == FindFirstCoLocatedSystem(i))
                            gSystems[i].links[n] = -1;
                    }
                }
                for (j = 0; j < kMaxSystemLinks; j++) {
                    if (gSystems[i].links[j] != -1) {
                        total = 0;
                        for (n = 0; n <= j; n++)
                            if (gSystems[i].links[n] == gSystems[i].links[j])
                                total++;
                        if (total > 1)
                            gSystems[i].links[j] = -1;
                    }
                }
            }
        }

        /* assume implicit reciprocal links */
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = gSystems[i].links[j];
                    if (link != -1) {
                        target = FindFirstCoLocatedSystem(link);
                        if (target >= 0 && target < kMaxSystems && target != i) {
                            char haveBack = 0;
                            for (n = 0; n < kMaxSystemLinks; n++) {
                                if (gSystems[target].links[n] != -1 &&
                                    FindFirstCoLocatedSystem(gSystems[target].links[n]) ==
                                        FindFirstCoLocatedSystem(i)) {
                                    haveBack = 1;
                                    break;
                                }
                            }
                            if (!haveBack) {
                                crashlog_print("warning: assuming implicit reciprocal link from ");
                                crashlog_print_pstr(gSystems[i].name);
                                crashlog_print(" (");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(") to ");
                                crashlog_print_pstr(gSystems[target].name);
                                crashlog_print(" (");
                                crashlog_print_num(target + kFirstResourceID);
                                crashlog_print(")\n");
                                for (n = 0; n < kMaxSystemLinks; n++) {
                                    if (gSystems[target].links[n] == -1) {
                                        gSystems[target].links[n] = FindFirstCoLocatedSystem(i);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /* orphan stellars: every prepared spob must appear in some navDefault */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxStellars; i++)
                used[i] = 0;
            for (i = 0; i < kMaxSystems; i++) {
                for (j = 0; j < kNavSlots; j++) {
                    if (gSystems[i].navDefault[j] >= 0 && gSystems[i].navDefault[j] < kMaxStellars)
                        used[gSystems[i].navDefault[j]] = 1;
                }
            }
            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared && !used[i]) {
                    stellarData[i].loaded = 0;
                    stellarData[i].prepared = 0;
                    crashlog_print("warning: stellar ");
                    if (stellarData[i].name[0] == 0)
                        crashlog_print("\"\"");
                    else
                        crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") does not exist in any system\n");
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- oütf: outfit items ---------------- */
    {

        found = 0;
        count = CountResources(kOutfType);
        for (i = 0; i < kMaxOutfits; i++) {
            gOutfHandle = GetResource(kOutfType, i + kFirstResourceID);
            if (gOutfHandle != NULL) {
                found++;
                LockAHandle(gOutfHandle);
                gOutfits[i].techLevel = *(short *)(*gOutfHandle + 0x04);
                gOutfits[i].cost = *(long *)(*gOutfHandle + 0x0e);
                gOutfits[i].mass = *(short *)(*gOutfHandle + 0x02);
                gOutfits[i].modType[0] = *(short *)(*gOutfHandle + 0x06);
                gOutfits[i].modVal[0] = *(short *)(*gOutfHandle + 0x08);
                gOutfits[i].maxCount = *(short *)(*gOutfHandle + 0x0a);
                gOutfits[i].itemClass = *(short *)(*gOutfHandle + 0x3ec);
                gOutfits[i].dispWeight = *(short *)(*gOutfHandle + 0x00);
                gOutfits[i].scanMask = *(short *)(*gOutfHandle + 0x3ee);
                gOutfits[i].buyRandom = *(short *)(*gOutfHandle + 0x3f0);
                if (gOutfits[i].buyRandom < 0)
                    gOutfits[i].buyRandom = 0;
                if (gOutfits[i].buyRandom > 100)
                    gOutfits[i].buyRandom = 100;
                gOutfits[i].govtRestrict = *(short *)(*gOutfHandle + 0x3f2);
                if ((gOutfits[i].govtRestrict < ResClassBase(0)
                        || gOutfits[i].govtRestrict >= ResClassBase(0) + kMaxGovts) &&
                    (gOutfits[i].govtRestrict < ResClassBase(1)
                        || gOutfits[i].govtRestrict >= ResClassBase(1) + kMaxGovts) &&
                    (gOutfits[i].govtRestrict < ResClassBase(2)
                        || gOutfits[i].govtRestrict >= ResClassBase(2) + kMaxGovts) &&
                    (gOutfits[i].govtRestrict < ResClassBase(3)
                        || gOutfits[i].govtRestrict >= ResClassBase(3) + kMaxGovts))
                    gOutfits[i].govtRestrict = -1;
                strcpy(gOutfits[i].availability, *gOutfHandle + 0x2e);
                strcpy(gOutfits[i].onPurchase, *gOutfHandle + 0x12d);
                strcpy(gOutfits[i].onSell, *gOutfHandle + 0x22c);
                AdjustHandleSize(gOutfHandle, 0x404);
                gOutfits[i].persistent = 0;
                gOutfits[i].flags = *(short *)(*gOutfHandle + 0x0c);
                if (OutfPersistent(&gOutfits[i]))
                    gOutfits[i].persistent = 1;
                GetResInfo(gOutfHandle, &resID, &resType, name);
                StripNameComment(name);
                strncpy((char *)gOutfits[i].name, (char *)name, 63);
                gOutfits[i].modType[1] = *(short *)(*gOutfHandle + 0x12);
                gOutfits[i].modVal[1] = *(short *)(*gOutfHandle + 0x14);
                gOutfits[i].modType[2] = *(short *)(*gOutfHandle + 0x16);
                gOutfits[i].modVal[2] = *(short *)(*gOutfHandle + 0x18);
                gOutfits[i].modType[3] = *(short *)(*gOutfHandle + 0x1a);
                gOutfits[i].modVal[3] = *(short *)(*gOutfHandle + 0x1c);
                for (j = 0; j < kOutfMods; j++) {
                    if ((gOutfits[i].modType[j] == 1 || gOutfits[i].modType[j] == 3 ||
                         gOutfits[i].modType[j] == 0x15) && gOutfits[i].modVal[j] >= kFirstResourceID)
                        gOutfits[i].modVal[j] -= kFirstResourceID;
                }
                gOutfits[i].contributes[0] = *(long *)(*gOutfHandle + 0x1e);
                gOutfits[i].contributes[1] = *(long *)(*gOutfHandle + 0x22);
                gOutfits[i].requires[0] = *(long *)(*gOutfHandle + 0x26);
                gOutfits[i].requires[1] = *(long *)(*gOutfHandle + 0x2a);
                strcpy((char *)gOutfShortNames[i], *gOutfHandle + 0x32b);
                strcpy((char *)lcOutfitName[i], *gOutfHandle + 0x36b);
                strcpy((char *)lcPluralOutfitName[i], *gOutfHandle + 0x3ab);
                CtoPstr((char *)gOutfShortNames[i]);
                CtoPstr((char *)lcOutfitName[i]);
                CtoPstr((char *)lcPluralOutfitName[i]);
                UnlockAHandle(gOutfHandle);
                ReleaseResource(gOutfHandle);
                if (gOutfits[i].buyRandom > 0 && crashlog_isactive()) {
                    if (!ResourceExists(kPictType, i + 6000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gOutfits[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks outfit picture\n");
                    }
                    if (!ResourceExists(kDescType, i + 3000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gOutfits[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks description\n");
                    }
                }
                HandleIntroMusic(0);
            } else {
                strcpy(gOutfits[i].availability, "");
                strcpy(gOutfits[i].onPurchase, "");
                strcpy(gOutfits[i].onSell, "");
                gOutfits[i].techLevel = kOutfTechAbsent;
                gOutfits[i].mass = 0;
                for (j = 0; j < kOutfMods; j++) {
                    gOutfits[i].modType[j] = 0;
                    gOutfits[i].modVal[j] = 0;
                }
            }
            if (found >= count)
                break;
        }

        /* alias outfits that share a lowercase display name */
        for (i = 0; i < kMaxOutfits; i++) {
            gOutfits[i].sameNameIndex = i;
            if (gOutfits[i].techLevel < kOutfTechAbsent) {
                /* no `i > 0` guard in the original: the for-entry test covers it
                 * (the exe has a single i-compare here, ours had two) */
                if (lcOutfitName[i][0] != 0) {
                    for (j = 0; j < i; j++) {
                        if (gOutfits[j].techLevel < kOutfTechAbsent &&
                            lcOutfitName[j][0] != 0 && lcOutfitName[i][0] == lcOutfitName[j][0]) {
                            n = 0;
                            while (n < lcOutfitName[i][0] &&
                                   lcOutfitName[i][1 + n] == lcOutfitName[j][1 + n])
                                n++;
                            if (n == lcOutfitName[i][0]) {
                                gOutfits[i].sameNameIndex = j;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- wëap: weapons ---------------- */
    {
        RGBColor shade;
        float base, factor;
        short g;

        found = 0;
        n = CountResources(kWeapType);
        for (i = 0; i < kMaxWeapTypes; i++)
            gWeapons[i].guidance = kResAbsent;
        for (i = 0; i < kMaxWeapTypes; i++) {
            gWeapHandle = GetResource(kWeapType, i + kFirstResourceID);
            if (gWeapHandle != NULL) {
                found++;
                LockAHandle(gWeapHandle);
                gWeapons[i].reload = *(short *)(*gWeapHandle + 0x00);
                gWeapons[i].lifetime = *(short *)(*gWeapHandle + 0x02);
                gWeapons[i].massDmg = *(short *)(*gWeapHandle + 0x04);
                gWeapons[i].energyDmg = *(short *)(*gWeapHandle + 0x06);
                gWeapons[i].guidance = *(short *)(*gWeapHandle + 0x08);
                gWeapons[i].speed100 = *(short *)(*gWeapHandle + 0x0a) / 100.0;
                gWeapons[i].ammoType = *(short *)(*gWeapHandle + 0x0c);
                gWeapons[i].graphic = *(short *)(*gWeapHandle + 0x0e);
                gWeapons[i].inaccuracy = *(short *)(*gWeapHandle + 0x10);
                gWeapons[i].sound = *(short *)(*gWeapHandle + 0x12);
                gWeapons[i].impact = *(short *)(*gWeapHandle + 0x14);
                gWeapons[i].recoil = *(short *)(*gWeapHandle + 0x56);
                gWeapons[i].explodType = *(short *)(*gWeapHandle + 0x16);
                gWeapons[i].proxRadius = *(short *)(*gWeapHandle + 0x18);
                gWeapons[i].blastRadius = *(short *)(*gWeapHandle + 0x1a);
                gWeapons[i].flags = *(short *)(*gWeapHandle + 0x1c);
                gWeapons[i].flags2 = *(short *)(*gWeapHandle + 0x48);
                gWeapons[i].flags3 = *(short *)(*gWeapHandle + 0x66);
                gWeapons[i].durability = *(short *)(*gWeapHandle + 0x68);
                gWeapons[i].maxAmmo = *(short *)(*gWeapHandle + 0x6c);
                gWeapons[i].seeker = *(short *)(*gWeapHandle + 0x1e);
                gWeapons[i].ionization = *(short *)(*gWeapHandle + 0x4a);
                gWeapons[i].exitType = *(short *)(*gWeapHandle + 0x58);
                gWeapons[i].burstCount = *(short *)(*gWeapHandle + 0x5a);
                gWeapons[i].burstReload = *(short *)(*gWeapHandle + 0x5c);
                gWeapons[i].guidedTurn = *(short *)(*gWeapHandle + 0x6a) * 0.1;
                if (gWeapons[i].inaccuracy == -1)
                    gWeapons[i].inaccuracy = 0;
                for (j = 0; j < 4; j++) {
                    gWeapons[i].jamVuln[j] = ((short *)(*gWeapHandle + 0x5e))[j];
                    if (gWeapons[i].jamVuln[j] < 0)
                        gWeapons[i].jamVuln[j] = 0;
                    if (gWeapons[i].jamVuln[j] > 100)
                        gWeapons[i].jamVuln[j] = 100;
                }
                if (*(unsigned long *)(*gWeapHandle + 0x72) == 0) {
                    rgb.red = rgb.green = 0x200;
                    rgb.blue = 0xffff;
                } else {
                    rgb.red = ((*(unsigned long *)(*gWeapHandle + 0x72) >> 16) & 0xff) << 8;
                    rgb.green = ((*(unsigned long *)(*gWeapHandle + 0x72) >> 8) & 0xff) << 8;
                    rgb.blue = (*(short *)(*gWeapHandle + 0x72) & 0xff) << 8;
                }
                gWeapons[i].ionizeColor = Color2Index(&rgb);
                if (gWeapons[i].burstCount <= 0)
                    gWeapons[i].burstCount = -1;
                gWeapons[i].beamLength = *(short *)(*gWeapHandle + 0x30);
                gWeapons[i].beamWidth = *(short *)(*gWeapHandle + 0x32);
                gWeapons[i].coronaFalloff = *(short *)(*gWeapHandle + 0x34);
                gWeapons[i].beamColor = *(unsigned long *)(*gWeapHandle + 0x36);
                gWeapons[i].coronaColor = *(unsigned long *)(*gWeapHandle + 0x3a);
                if (gWeapons[i].beamLength > 0 && gWeapons[i].coronaFalloff <= 0)
                    gWeapons[i].coronaFalloff = 16;
                gWeapons[i].lightningDensity = *(short *)(*gWeapHandle + 0x6e);
                gWeapons[i].lightningAmplitude = *(short *)(*gWeapHandle + 0x70);
                if (gWeapons[i].lightningDensity > 0) {
                    if (gWeapons[i].lightningDensity < 2)
                        gWeapons[i].lightningDensity = 2;
                    gWeapons[i].coronaFalloff = 0;
                    if (gWeapons[i].beamWidth < 1)
                        gWeapons[i].beamWidth = 1;
                    if (gWeapons[i].lightningAmplitude < 1)
                        gWeapons[i].lightningAmplitude = 1;
                }
                gWeapons[i].subCount = *(short *)(*gWeapHandle + 0x3e);
                gWeapons[i].subType = *(short *)(*gWeapHandle + 0x40);
                gWeapons[i].subTheta = *(short *)(*gWeapHandle + 0x42);
                gWeapons[i].subLimit = *(short *)(*gWeapHandle + 0x44);
                if (gWeapons[i].subTheta == -1)
                    gWeapons[i].subTheta = 0;
                if (gWeapons[i].subType >= kFirstResourceID && gWeapons[i].subType < kFirstResourceID + kMaxWeapTypes)
                    gWeapons[i].subType -= kFirstResourceID;
                else
                    gWeapons[i].subType = -1;
                if (gWeapons[i].guidance == kGuidanceBeam || gWeapons[i].guidance == kGuidanceTurretBeam
                    || gWeapons[i].guidance == kGuidancePDBeam)
                    gWeapons[i].decayShots = 0;
                else
                    gWeapons[i].decayShots = gWeapons[i].coronaFalloff;
                gWeapons[i].proxSafety = *(short *)(*gWeapHandle + 0x46);
                if (gWeapons[i].subCount <= 0 || gWeapons[i].subType < 0) {
                    gWeapons[i].subCount = 0;
                    gWeapons[i].subType = -1;
                }
                if (gWeapons[i].guidance == kGuidanceBeam || gWeapons[i].guidance == kGuidanceTurretBeam
                    || gWeapons[i].guidance == kGuidancePDBeam) {
                    gWeapons[i].rgbScratch.red = ((gWeapons[i].beamColor >> 16) & 0xff) << 8;
                    gWeapons[i].rgbScratch.green = ((gWeapons[i].beamColor >> 8) & 0xff) << 8;
                    gWeapons[i].rgbScratch.blue = (gWeapons[i].beamColor & 0xff) << 8;
                    if (gSpriteWorld->pixelDepth == 16) {
                        gWeapons[i].beamColor = (gWeapons[i].beamColor >> 9) & 0x7c00 |
                                          (gWeapons[i].beamColor >> 6) & 0x3e0 |
                                          (gWeapons[i].beamColor >> 3) & 0x1f;
                        gWeapons[i].coronaColor = (gWeapons[i].coronaColor >> 9) & 0x7c00 |
                                            (gWeapons[i].coronaColor >> 6) & 0x3e0 |
                                            (gWeapons[i].coronaColor >> 3) & 0x1f;
                    } else {
                        gWeapons[i].beamColor = Color2Index(&gWeapons[i].rgbScratch);
                        rgb.red = ((gWeapons[i].coronaColor >> 16) & 0xff) << 7;
                        rgb.green = ((gWeapons[i].coronaColor >> 8) & 0xff) << 7;
                        rgb.blue = (gWeapons[i].coronaColor & 0xff) << 7;
                        gWeapons[i].coronaColor = Color2Index(&rgb);
                    }
                }
                gWeapons[i].hitParticles = *(short *)(*gWeapHandle + 0x4c);
                rgb.red = ((*(unsigned long *)(*gWeapHandle + 0x52) >> 16) & 0xff) << 8;
                rgb.green = ((*(unsigned long *)(*gWeapHandle + 0x52) >> 8) & 0xff) << 8;
                rgb.blue = (*(short *)(*gWeapHandle + 0x52) & 0xff) << 8;
                gWeapons[i].hitPartColor = Color2Index(&rgb);
                gWeapons[i].hitPartLife = *(short *)(*gWeapHandle + 0x4e);
                gWeapons[i].hitPartVel = *(short *)(*gWeapHandle + 0x50) * 0.01;
                if (gSpriteWorld->pixelDepth == 8)
                    gWeapons[i].hitPartLife /= 2;
                GetResInfo(gWeapHandle, &resID, &resType, name);
                StripNameComment(name);
                pstrcpy(gWeapNames[i], name);
                gWeapons[i].smokeSet = *(short *)(*gWeapHandle + 0x20);
                gWeapons[i].decay = *(short *)(*gWeapHandle + 0x22);
                gWeapons[i].particles = *(short *)(*gWeapHandle + 0x24);
                gWeapons[i].partLifeMin = *(short *)(*gWeapHandle + 0x28);
                gWeapons[i].partLifeMax = *(short *)(*gWeapHandle + 0x2a);
                base = *(short *)(*gWeapHandle + 0x26) * 0.01;
                rgb.red = ((*(unsigned long *)(*gWeapHandle + 0x2c) >> 16) & 0xff) << 8;
                rgb.green = ((*(unsigned long *)(*gWeapHandle + 0x2c) >> 8) & 0xff) << 8;
                rgb.blue = (*(short *)(*gWeapHandle + 0x2c) & 0xff) << 8;
                for (j = 0; j < 8; j++) {
                    factor = (Rand(0x51) + 60) * 0.01;
                    shade.red = rgb.red * factor;
                    shade.green = rgb.green * factor;
                    shade.blue = rgb.blue * factor;
                    gWeapons[i].partColor[j] = Color2Index(&shade);
                    gWeapons[i].partVel[j] = base * factor;
                }
                if (gWeapons[i].smokeSet >= 8)
                    gWeapons[i].smokeSet = 0;
                if (gWeapons[i].guidance == 2) {
                    gWeapons[i].guidance = kGuidanceHoming;
                    gWeapons[i].seeker = 0x30fe;
                }
                UnlockAHandle(gWeapHandle);
                AdjustHandleSize(gWeapHandle, 0x86);
                ReleaseResource(gWeapHandle);
                HandleIntroMusic(0);
            }
            if (found >= n)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* ammo-source sanity warnings */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (gWeapons[i].guidance != kResAbsent) {
                    short ammo = gWeapons[i].ammoType;
                    if (ammo >= 0 && ammo < kMaxWeapTypes &&
                        gWeapons[i].guidance != kGuidanceBay &&
                        (ammo != gWeapons[ammo].ammoType ||
                         gWeapons[ammo].guidance == kGuidanceBay)) {
                        crashlog_print("warning: weapon ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" probably shouldn't draw ammo from weapon ");
                        crashlog_print_num(ammo + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
            for (i = 0; i < kMaxOutfits; i++) {
                if (gOutfits[i].techLevel < kOutfTechAbsent) {
                    for (count = 0; count < kOutfMods; count++) {
                        if (gOutfits[i].modType[count] == 3) {
                            j = gOutfits[i].modVal[count];
                            g = gWeapons[j].guidance;
                            if (g != kResAbsent) {
                                if (g != kGuidanceBay && j != gWeapons[j].ammoType) {
                                    crashlog_print("warning: outfit item ");
                                    crashlog_print_num(i + kFirstResourceID);
                                    crashlog_print(" grants ammo to weapon ");
                                    crashlog_print_num(j + kFirstResourceID);
                                    if (gWeapons[j].ammoType < 0 ||
                                        gWeapons[j].ammoType >= kMaxWeapTypes)
                                        crashlog_print(", which doesn't use ammo\n");
                                    else
                                        crashlog_print(", which doesn't use its own ammo\n");
                                }
                            } else {
                                crashlog_print("warning: outfit item ");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(" grants ammo to nonexisent weapon\n");
                            }
                        }
                    }
                }
            }
        }

        /* total range: beams use beam length, shots walk the submunition chain */
        for (i = 0; i < kMaxWeapTypes; i++) {
            gWeapons[i].totalRange = 0;
            g = gWeapons[i].guidance;
            if (g == kGuidanceUnguided || g == kGuidanceHoming || g == kGuidanceTurret
                    || (unsigned short)(g - kGuidanceRocket) <= 3) {
                n = i;
                do {
                    gWeapons[i].totalRange += (double)(gWeapons[n].speed100 * gWeapons[n].lifetime);
                    if (gWeapons[n].subCount > 0) {
                        short nxt = gWeapons[n].subType;
                        if (n == nxt) {
                            short lim = gWeapons[n].subLimit;
                            if (lim > 0)
                                gWeapons[i].totalRange += (double)(gWeapons[n].lifetime *
                                                    (lim * gWeapons[n].speed100));
                            n = -1;
                        } else {
                            n = nxt;
                        }
                    } else {
                        n = -1;
                    }
                } while (n != -1);
            } else if (g == kGuidanceBeam || g == kGuidanceTurretBeam || g == kGuidancePDBeam) {
                gWeapons[i].totalRange = gWeapons[i].beamLength;
            }
        }
    }

    /* ---------------- shïp: ship classes ---------------- */
    {
        short shipCount;
        Handle h;
        char *res;
        long tradeEst;
        long mass;
        short gunTally, turretTally;
        short maxGuns, maxTurrets;
        short k, j;
        short weapID;

        shipCount = CountResources(kShipType);
        if (shipCount == 0)
            crashlog("warning: no sh\x95" "p resources found");
        for (i = 0; i < kMaxShipTypes;i++) {
            gShipHandle = GetResource(kShipType, i + kFirstResourceID);
            if (gShipHandle != NULL) {
                LockAHandle(gShipHandle);
                gShipTypes[i].cargo = *(short *)(*gShipHandle + 0x00);
                gShipTypes[i].shield = *(short *)(*gShipHandle + 0x02);
                gShipTypes[i].accel = *(short *)(*gShipHandle + 0x04) / 10000.0;
                gShipTypes[i].maxSpeed = *(short *)(*gShipHandle + 0x06) / 100.0;
                gShipTypes[i].turnRate = *(short *)(*gShipHandle + 0x08) * 0.1;
                gShipTypes[i].fuel = *(short *)(*gShipHandle + 0x0a);
                gShipTypes[i].freeMass = *(short *)(*gShipHandle + 0x0c);
                gShipTypes[i].armor = *(short *)(*gShipHandle + 0x0e);
                gShipTypes[i].maxGuns = *(short *)(*gShipHandle + 0x2a);
                gShipTypes[i].maxTurrets = *(short *)(*gShipHandle + 0x2c);
                gShipTypes[i].techLevel = *(short *)(*gShipHandle + 0x2e);
                gShipTypes[i].cost = *(long *)(*gShipHandle + 0x30);
                gShipTypes[i].deathDelay = *(short *)(*gShipHandle + 0x34);
                gShipTypes[i].mass = *(short *)(*gShipHandle + 0x3e);
                gShipTypes[i].shipLength = *(short *)(*gShipHandle + 0x40);
                gShipTypes[i].inherentAI = *(short *)(*gShipHandle + 0x42);
                gShipTypes[i].crew = *(short *)(*gShipHandle + 0x44);
                gShipTypes[i].strength = *(short *)(*gShipHandle + 0x46);
                gShipTypes[i].flags = *(short *)(*gShipHandle + 0x4a);
                gShipTypes[i].flags2 = *(short *)(*gShipHandle + 0x62);
                gShipTypes[i].flags3 = *(short *)(*gShipHandle + 0x726);
                gShipTypes[i].dispWeight = *(short *)(*gShipHandle + 0x3c);
                gShipTypes[i].podCount = *(short *)(*gShipHandle + 0x4c);
                gShipTypes[i].buyRandom = *(short *)(*gShipHandle + 0x388);
                gShipTypes[i].hireRandom = *(short *)(*gShipHandle + 0x38a);
                if (InGovtClass(*(short *)(*gShipHandle + 0x48), 0)) {
                    gShipTypes[i].inherentGovt = *(short *)(*gShipHandle + 0x48) - ResClassBase(0);
                    gShipTypes[i].inherentGovtB = gShipTypes[i].inherentGovt;
                } else if (InGovtClass(*(short *)(*gShipHandle + 0x48), 1)) {
                    gShipTypes[i].inherentGovt = -1;
                    gShipTypes[i].inherentGovtB = *(short *)(*gShipHandle + 0x48) - ResClassBase(1);
                } else if (InGovtClass(*(short *)(*gShipHandle + 0x48), 2)) {
                    gShipTypes[i].inherentGovt = *(short *)(*gShipHandle + 0x48) - ResClassBase(2);
                    gShipTypes[i].inherentGovtB = -1;
                } else {
                    gShipTypes[i].inherentGovt = -1;
                    gShipTypes[i].inherentGovtB = -1;
                }
                if (gShipTypes[i].buyRandom > 100)
                    gShipTypes[i].buyRandom = 100;
                if (gShipTypes[i].hireRandom > 100)
                    gShipTypes[i].hireRandom = 100;
                if (gShipTypes[i].strength < 1)
                    gShipTypes[i].strength = 1;
                gShipTypes[i].upgradeTo = *(short *)(*gShipHandle + 0x728);
                gShipTypes[i].escUpgradeCost = *(long *)(*gShipHandle + 0x72a);
                gShipTypes[i].tradeIn = *(long *)(*gShipHandle + 0x72e);
                if (gShipTypes[i].upgradeTo >= kFirstResourceID)
                    gShipTypes[i].upgradeTo -= kFirstResourceID;
                else
                    gShipTypes[i].upgradeTo = -1;
                if (gShipTypes[i].escUpgradeCost < 0)
                    gShipTypes[i].escUpgradeCost = 0;
                if (gShipTypes[i].tradeIn <= 0)
                    gShipTypes[i].tradeIn = gShipTypes[i].cost * 0.1;
                gShipTypes[i].escortType = *(short *)(*gShipHandle + 0x732);
                if (gShipTypes[i].escortType < 0 || gShipTypes[i].escortType > 3) {
                    if (gShipTypes[i].inherentAI >= 3) {
                        if (gShipTypes[i].mass < 50)
                            gShipTypes[i].escortType = 0;
                        else if (gShipTypes[i].mass < 200)
                            gShipTypes[i].escortType = 1;
                        else
                            gShipTypes[i].escortType = 2;
                    } else {
                        gShipTypes[i].escortType = 3;
                    }
                }
                if (gShipTypes[i].escortType == 0)
                    gShipTypes[i].aiSkill = 80;
                else if (gShipTypes[i].escortType == 1)
                    gShipTypes[i].aiSkill = 90;
                else
                    gShipTypes[i].aiSkill = 100;
                gShipTypes[i].explode1 = *(short *)(*gShipHandle + 0x38);
                gShipTypes[i].explode2 = *(short *)(*gShipHandle + 0x3a);
                gShipTypes[i].shieldRech = *(short *)(*gShipHandle + 0x10);
                gShipTypes[i].armorRech = *(short *)(*gShipHandle + 0x36);
                if (gShipTypes[i].shieldRech > 0.0f)
                    gShipTypes[i].shieldRech *= 0.001;
                else
                    gShipTypes[i].shieldRech = 0;
                if (gShipTypes[i].armorRech > 0.0f)
                    gShipTypes[i].armorRech *= 0.001;
                else
                    gShipTypes[i].armorRech = 0;
                gShipTypes[i].deionize = *(short *)(*gShipHandle + 0x36a);
                gShipTypes[i].ionizeMax = *(short *)(*gShipHandle + 0x36c);
                if (gShipTypes[i].deionize > 0.0f)
                    gShipTypes[i].deionize *= 0.01;
                else
                    gShipTypes[i].deionize = 1.0f;
                if (gShipTypes[i].ionizeMax <= 0)
                    gShipTypes[i].ionizeMax = 0;
                gShipTypes[i].keyCarried = *(short *)(*gShipHandle + 0x36e);
                if (gShipTypes[i].keyCarried >= kFirstResourceID)
                    gShipTypes[i].keyCarried -= kFirstResourceID;
                else
                    gShipTypes[i].keyCarried = -1;
                if (gShipTypes[i].strength < 1)
                    gShipTypes[i].strength = 1;
                if (gShipTypes[i].cargo < 0) {
                    gShipTypes[i].cargo *= -1;
                    gShipTypes[i].cargoNotFixed = 0;
                } else {
                    gShipTypes[i].cargoNotFixed = 1;
                }
                if (gShipTypes[i].shield < 0)
                    gShipTypes[i].shield *= -5;
                if (gShipTypes[i].crew < 0)
                    gShipTypes[i].crew = 0;
                for (j = 0; j < 4; j++) {
                    h = gShipHandle;
                    mountID[j + 4] = -1;
                    mountID[j] = mountID[j + 4];
                    weapID = ((short *)((res = *gShipHandle) + 0x12))[j];
                    if (weapID >= kFirstResourceID && weapID < kFirstResourceID + kMaxWeapTypes) {
                        char *p = *h;

                        mountID[j] = weapID;
                        mountCount[j] = ((short *)(p + 0x1a))[j];
                        mountAmmo[j] = ((short *)(p + 0x22))[j];
                    } else {
                        mountID[j] = -1;
                    }
                    weapID = ((short *)(res + 0x6ce))[j];
                    if (weapID >= kFirstResourceID && weapID < kFirstResourceID + kMaxWeapTypes) {
                        char *p = *h;

                        mountID[j + 4] = weapID;
                        mountCount[j + 4] = ((short *)(p + 0x6d6))[j];
                        mountAmmo[j + 4] = ((short *)(p + 0x6de))[j];
                    } else {
                        mountID[j + 4] = -1;
                    }
                }
                gShipTypes[i].defOutfID[0] = *(short *)(*gShipHandle + 0x4e);
                gShipTypes[i].defOutfID[1] = *(short *)(*gShipHandle + 0x50);
                gShipTypes[i].defOutfID[2] = *(short *)(*gShipHandle + 0x52);
                gShipTypes[i].defOutfID[3] = *(short *)(*gShipHandle + 0x54);
                gShipTypes[i].defOutfID[4] = *(short *)(*gShipHandle + 0x370);
                gShipTypes[i].defOutfID[5] = *(short *)(*gShipHandle + 0x372);
                gShipTypes[i].defOutfID[6] = *(short *)(*gShipHandle + 0x374);
                gShipTypes[i].defOutfID[7] = *(short *)(*gShipHandle + 0x376);
                gShipTypes[i].defOutfCount[0] = *(short *)(*gShipHandle + 0x56);
                gShipTypes[i].defOutfCount[1] = *(short *)(*gShipHandle + 0x58);
                gShipTypes[i].defOutfCount[2] = *(short *)(*gShipHandle + 0x5a);
                gShipTypes[i].defOutfCount[3] = *(short *)(*gShipHandle + 0x5c);
                gShipTypes[i].defOutfCount[4] = *(short *)(*gShipHandle + 0x378);
                gShipTypes[i].defOutfCount[5] = *(short *)(*gShipHandle + 0x37a);
                gShipTypes[i].defOutfCount[6] = *(short *)(*gShipHandle + 0x37c);
                gShipTypes[i].defOutfCount[7] = *(short *)(*gShipHandle + 0x37e);
                for (count = 0; count < 8; count++) {
                    if (gShipTypes[i].defOutfID[count] < kFirstResourceID || gShipTypes[i].defOutfID[count] >= kFirstResourceID + kMaxOutfits) {
                        gShipTypes[i].defOutfID[count] = -1;
                        gShipTypes[i].defOutfCount[count] = 0;
                    } else {
                        gShipTypes[i].defOutfID[count] -= kFirstResourceID;
                        if (gShipTypes[i].defOutfCount[count] < 0)
                            gShipTypes[i].defOutfCount[count] = 0;
                    }
                }
                gShipTypes[i].fuelRegen = *(short *)(*gShipHandle + 0x5e);
                gShipTypes[i].skillVar = *(short *)(*gShipHandle + 0x60);
                if (gShipTypes[i].skillVar < 1)
                    gShipTypes[i].skillVar = 1;
                if (gShipTypes[i].skillVar > 50)
                    gShipTypes[i].skillVar = 50;
                gShipTypes[i].contributes[0] = *(long *)(*gShipHandle + 0x64);
                gShipTypes[i].contributes[1] = *(long *)(*gShipHandle + 0x68);
                gShipTypes[i].requires[0] = *(long *)(*gShipHandle + 0x380);
                gShipTypes[i].requires[1] = *(long *)(*gShipHandle + 0x384);
                tradeEst = gShipTypes[i].cost * 0.25;
                gunTally = 0;
                turretTally = 0;
                for (k = 0; k < 8; k++) {
                    if (mountID[k] >= kFirstResourceID && mountID[k] < kFirstResourceID + kMaxWeapTypes) {
                        short qty = mountCount[k];
                        short ammo;

                        gShipTypes[i].weapCount[mountID[k] - kFirstResourceID] = qty;
                        ammo = mountAmmo[k];
                        gShipTypes[i].weapAmmo[mountID[k] - kFirstResourceID] = ammo;
                        if (qty > 0) {
                            for (j = 0; j < kMaxOutfits; j++) {
                                if (gOutfits[j].modType[0] == 1 &&
                                    gOutfits[j].modVal[0] == mountID[k] - kFirstResourceID &&
                                    gOutfits[j].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&gOutfits[j], &gShipTypes[i]) * qty;
                                    gShipTypes[i].freeMass += mass;
                                    if (!gOutfits[j].persistent)
                                        tradeEst = tradeEst +
                                            (AdjustedItemCost(&gOutfits[j], &gShipTypes[i]) * qty) * 0.5;
                                    if (gOutfits[j].flags & kOutfFixedGun)
                                        gunTally += qty;
                                    if (gOutfits[j].flags & kOutfTurret)
                                        turretTally += qty;
                                    break;
                                }
                            }
                        }
                        if (ammo > 0) {
                            if (gWeapons[mountID[k] - kFirstResourceID].guidance == kGuidanceBay)
                                count = mountID[k] - kFirstResourceID;
                            else {
                                if ((count = gWeapons[mountID[k] - kFirstResourceID].ammoType) < 0 ||
                                    count >= kMaxWeapTypes)
                                    count = -1;
                            }
                            if (count != -1) {
                                for (j = 0; j < kMaxOutfits; j++) {
                                    if (gOutfits[j].modType[0] == 3 &&
                                        gOutfits[j].modVal[0] == count &&
                                        gOutfits[j].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&gOutfits[j], &gShipTypes[i]) * ammo;
                                    gShipTypes[i].freeMass += mass;
                                        if (!gOutfits[j].persistent)
                                            tradeEst = tradeEst +
                                                (AdjustedItemCost(&gOutfits[j], &gShipTypes[i]) * qty) * 0.5;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                maxGuns = gShipTypes[i].maxGuns;
                maxTurrets = gShipTypes[i].maxTurrets;
                for (k = 0; k < 8; k++) {
                    if (gShipTypes[i].defOutfID[k] != -1 && gShipTypes[i].defOutfCount[k] > 0 &&
                        gOutfits[gShipTypes[i].defOutfID[k]].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&gOutfits[gShipTypes[i].defOutfID[k]], &gShipTypes[i]) *
                               gShipTypes[i].defOutfCount[k];
                        gShipTypes[i].freeMass += mass;
                        if (!gOutfits[gShipTypes[i].defOutfID[k]].persistent)
                            tradeEst = tradeEst +
                                (AdjustedItemCost(&gOutfits[gShipTypes[i].defOutfID[k]], &gShipTypes[i]) *
                                 gShipTypes[i].defOutfCount[k]) * 0.5;
                        if (crashlog_isactive()) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (gOutfits[gShipTypes[i].defOutfID[k]].modType[j] == 45)
                                    maxGuns += gOutfits[gShipTypes[i].defOutfID[k]].modVal[j];
                                if (gOutfits[gShipTypes[i].defOutfID[k]].modType[j] == 46)
                                    maxTurrets += gOutfits[gShipTypes[i].defOutfID[k]].modVal[j];
                            }
                        }
                    }
                }
                if (gShipTypes[i].flags & 1)
                    gShipTypes[i].mobilityFactor = 0.7f;
                else if (gShipTypes[i].flags & 2)
                    gShipTypes[i].mobilityFactor = 1.3f;
                else if (gShipTypes[i].flags & 4)
                    gShipTypes[i].mobilityFactor = 1.6f;
                else
                    gShipTypes[i].mobilityFactor = 1.0f;
                gShipTypes[i].mobilityFactor *= 1.3;
                if (gShipTypes[i].mobilityFactor < 0.0f)
                    gShipTypes[i].mobilityFactor = 0.5f;
                strcpy(gShipTypes[i].availability, *gShipHandle + 0x6c);
                strcpy(gShipTypes[i].appearOn, *gShipHandle + 0x16b);
                strcpy(gShipTypes[i].onPurchase, *gShipHandle + 0x26a);
                strcpy(gShipTypes[i].onCapture, *gShipHandle + 0x3d0);
                strcpy(gShipTypes[i].onRetire, *gShipHandle + 0x4cf);
                AdjustHandleSize(gShipHandle, 0x744);
                GetResInfo(gShipHandle, &resID, &resType, name);
                StripNameComment(name);
                strncpy((char *)gShipTypes[i].name, (char *)name, 63);
                strcpy((char *)gShipShortNames[i], *gShipHandle + 0x5ce);
                strcpy((char *)gShipLongNames[i], *gShipHandle + 0x62e);
                strcpy((char *)gShipCommNames[i], *gShipHandle + 0x60e);
                strcpy((char *)gShipMovieFiles[i], *gShipHandle + 0x6ae);
                strcpy((char *)gShipTypes[i].subtitle, *gShipHandle + 0x6e6);
                CtoPstr((char *)gShipShortNames[i]);
                CtoPstr((char *)gShipLongNames[i]);
                CtoPstr((char *)gShipCommNames[i]);
                CtoPstr((char *)gShipMovieFiles[i]);
                CtoPstr((char *)gShipTypes[i].subtitle);
                if (crashlog_isactive()) {
                    if (maxGuns < 0) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gShipTypes[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has a maximum gun count of ");
                        crashlog_print_num(maxGuns);
                        crashlog_print("\n");
                    }
                    if (maxTurrets < 0) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gShipTypes[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has a maximum turret count of ");
                        crashlog_print_num(maxTurrets);
                        crashlog_print("\n");
                    }
                    if (gunTally > maxGuns) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gShipTypes[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has ");
                        crashlog_print_num(gunTally);
                        crashlog_print(" gun-type weapon");
                        if (gunTally > 1)
                            crashlog_print("s");
                        crashlog_print(" but its maximum is ");
                        crashlog_print_num(maxGuns);
                        crashlog_print("\n");
                    }
                    if (turretTally > maxTurrets) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gShipTypes[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has ");
                        crashlog_print_num(turretTally);
                        crashlog_print(" turreted weapon");
                        if (turretTally > 1)
                            crashlog_print("s");
                        crashlog_print(" but its maximum is ");
                        crashlog_print_num(maxTurrets);
                        crashlog_print("\n");
                    }
                }
                if (tradeEst > gShipTypes[i].cost && gShipTypes[i].buyRandom != 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(gShipTypes[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a trade-in value of ");
                    crashlog_print_num(tradeEst);
                    crashlog_print(" but costs only ");
                    crashlog_print_num(gShipTypes[i].cost);
                    crashlog_print("\n");
                }
                UnlockAHandle(gShipHandle);
                ReleaseResource(gShipHandle);
                if (crashlog_isactive()) {
                    if (gShipTypes[i].buyRandom != 0) {
                        if (!ResourceExists(kDescType, i + 13000)) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(gShipTypes[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(") lacks purchase description\n");
                        }
                        for (count = 0; count < kMaxWeapTypes; count++) {
                            if (gShipTypes[i].weapCount[count] > 0) {
                                short granted = 0;

                                for (j = 0; j < kMaxOutfits; j++) {
                                    for (k = 0; k < kOutfMods; k++) {
                                        if (gOutfits[j].modType[k] == 1 &&
                                            gOutfits[j].modVal[k] == count) {
                                            granted++;
                                            break;
                                        }
                                    }
                                    if (granted > 0)
                                        break;
                                }
                                if (granted == 0) {
                                    crashlog_print("warning: weapon ID ");
                                    crashlog_print_num(count + kFirstResourceID);
                                    crashlog_print(" on ship type ");
                                    crashlog_print_num(i + kFirstResourceID);
                                    crashlog_print(" lacks corresponding o\x9f" "tf resource\n");
                                }
                            }
                        }
                    }
                    if (gShipTypes[i].hireRandom != 0 &&
                        !ResourceExists(kDescType, i + 14000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(gShipTypes[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks hiring description\n");
                    }
                }
                HandleIntroMusic(0);
            } else {
                strcpy(gShipTypes[i].availability, "");
                strcpy(gShipTypes[i].appearOn, "");
                strcpy(gShipTypes[i].onPurchase, "");
                strcpy(gShipTypes[i].onCapture, "");
                strcpy(gShipTypes[i].onRetire, "");
                gShipTypes[i].techLevel = kResAbsent;
                gShipTypes[i].shield = 1;
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- düde: dudes ---------------- */
    {
        short i;

        found = 0;
        count = CountResources(kDudeType);
        for (i = 0; i < 512; i++) {
            gDudeHandle = GetResource(kDudeType, i + kFirstResourceID);
            if (gDudeHandle != NULL) {
                found++;
                LockAHandle(gDudeHandle);
                dudeData[i].aiType = *(short *)(*gDudeHandle + 0x00);
                dudeData[i].govt = *(short *)(*gDudeHandle + 0x02);
                dudeData[i].flags = *(short *)(*gDudeHandle + 0x04);
                dudeData[i].adviceFlags = *(short *)(*gDudeHandle + 0x06);
                if (dudeData[i].govt >= kFirstResourceID && dudeData[i].govt < kFirstResourceID + kMaxGovts)
                    dudeData[i].govt -= kFirstResourceID;
                else
                    dudeData[i].govt = -1;
                for (j = 0; j < 16; j++) {
                    dudeData[i].shipTypes[j] = ((short *)(*gDudeHandle + 0x08))[j];
                    dudeData[i].shipProbs[j] = ((short *)(*gDudeHandle + 0x28))[j];
                    if (dudeData[i].shipTypes[j] >= kFirstResourceID && dudeData[i].shipTypes[j] < kFirstResourceID + kMaxShipTypes) {
                        dudeData[i].shipTypes[j] -= kFirstResourceID;
                        if (gShipTypes[dudeData[i].shipTypes[j]].techLevel == kResAbsent) {
                            crashlog_print("warning: d\x9f" "de resource ");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(" refers to nonexistent ship type ");
                            crashlog_print_num(dudeData[i].shipTypes[j] + kFirstResourceID);
                            crashlog_print("\n");
                            dudeData[i].shipTypes[j] = -1;
                        }
                    } else {
                        dudeData[i].shipTypes[j] = -1;
                        dudeData[i].shipProbs[j] = 0;
                    }
                }
                UnlockAHandle(gDudeHandle);
                AdjustHandleSize(gDudeHandle, 0x58);
                ReleaseResource(gDudeHandle);
                dudeData[i].loaded = 1;
                HandleIntroMusic(0);
            } else {
                dudeData[i].loaded = 0;
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- gövt: governments ---------------- */
    {
        long k;

        found = 0;
        count = CountResources(kGovtType);
        for (i = 0; i < kMaxGovts; i++) {
            gGovtHandle = GetResource(kGovtType, i + kFirstResourceID);
            if (gGovtHandle != NULL) {
                found++;
                LockAHandle(gGovtHandle);
                govtData[i].loaded = 1;
                govtData[i].flags = *(short *)(*gGovtHandle + 0x02);
                govtData[i].flags2 = *(short *)(*gGovtHandle + 0x04);
                govtData[i].crimeTol = *(short *)(*gGovtHandle + 0x08);
                govtData[i].scanFine = *(short *)(*gGovtHandle + 0x06);
                govtData[i].initialRec = *(short *)(*gGovtHandle + 0x14);
                govtData[i].voiceType = *(short *)(*gGovtHandle + 0x00);
                if (govtData[i].voiceType >= 0 && govtData[i].voiceType < 8) {
                    govtData[i].voiceClass = -1;
                } else if (govtData[i].voiceType >= 1000 && govtData[i].voiceType < 1008) {
                    govtData[i].voiceType -= 1000;
                    govtData[i].voiceClass = 1;
                } else if (govtData[i].voiceType >= 2000 && govtData[i].voiceType < 2008) {
                    govtData[i].voiceType -= 2000;
                    govtData[i].voiceClass = 0;
                } else {
                    govtData[i].voiceType = -1;
                    govtData[i].voiceClass = -1;
                }
                for (j = 0; j < 4; j++) {
                    govtData[i].inhJam[j] = ((short *)(*gGovtHandle + 0x5c))[j];
                    if (govtData[i].inhJam[j] > 100)
                        govtData[i].inhJam[j] = 100;
                    if (govtData[i].inhJam[j] < 0)
                        govtData[i].inhJam[j] = 0;
                }
                if (*(short *)(*gGovtHandle + 0x30) <= 0)
                    govtData[i].skillMult = 1.0f;
                else
                    govtData[i].skillMult = *(short *)(*gGovtHandle + 0x30) * 0.01;
                govtData[i].maxOdds = *(short *)(*gGovtHandle + 0x16) * 0.01;
                if (govtData[i].maxOdds < 0.01)
                    govtData[i].maxOdds = 0.01f;
                for (j = 0; j < 5; j++)
                    govtData[i].penalties[j] = ((short *)(*gGovtHandle + 0x0a))[j];
                for (k = 0; k < 4; k++) {
                    govtData[i].classes[k] = ((short *)(*gGovtHandle + 0x18))[k];
                    govtData[i].allies[k] = ((short *)(*gGovtHandle + 0x20))[k];
                    govtData[i].enemies[k] = ((short *)(*gGovtHandle + 0x28))[k];
                }
                govtData[i].scanMask = *(short *)(*gGovtHandle + 0x32);
                govtData[i].requires[0] = *(long *)(*gGovtHandle + 0x54);
                govtData[i].requires[1] = *(long *)(*gGovtHandle + 0x58);
                govtData[i].rgb.red = ((*(unsigned long *)(*gGovtHandle + 0xa4) >> 16) & 0xff) << 8;
                govtData[i].rgb.green = ((*(unsigned long *)(*gGovtHandle + 0xa4) >> 8) & 0xff) << 8;
                govtData[i].rgb.blue = (*(short *)(*gGovtHandle + 0xa4) & 0xff) << 8;
                govtData[i].rgb2.red = ((*(unsigned long *)(*gGovtHandle + 0xa8) >> 16) & 0xff) << 8;
                govtData[i].rgb2.green = ((*(unsigned long *)(*gGovtHandle + 0xa8) >> 8) & 0xff) << 8;
                govtData[i].rgb2.blue = (*(short *)(*gGovtHandle + 0xa8) & 0xff) << 8;
                govtData[i].rgbBytes[0] = govtData[i].rgb.red >> 8;
                govtData[i].rgbBytes[1] = govtData[i].rgb.green >> 8;
                govtData[i].rgbBytes[2] = govtData[i].rgb.blue >> 8;
                govtData[i].interfaceID = *(short *)(*gGovtHandle + 0xac);
                if (govtData[i].interfaceID < kFirstResourceID)
                    govtData[i].interfaceID = -1;
                govtData[i].newsPict = *(short *)(*gGovtHandle + 0xae);
                if (govtData[i].newsPict < kFirstResourceID)
                    govtData[i].newsPict = -1;
                GetResInfo(gGovtHandle, &resID, &resType, name);
                StripNameComment(name);
                strncpy((char *)govtData[i].name, (char *)name, 31);
                strcpy((char *)govtTargStrings[i], *gGovtHandle + 0x44);
                strcpy((char *)govtShortNames[i], *gGovtHandle + 0x34);
                strcpy((char *)gGovtNames3[i], *gGovtHandle + 0x64);
                CtoPstr((char *)govtTargStrings[i]);
                CtoPstr((char *)govtShortNames[i]);
                CtoPstr((char *)gGovtNames3[i]);
                UnlockAHandle(gGovtHandle);
                AdjustHandleSize(gGovtHandle, 0xc0);
                ReleaseResource(gGovtHandle);
                HandleIntroMusic(0);
            } else {
                govtData[i].loaded = 0;
            }
            if (found >= count)
                break;
        }

        /* allies/enemies consistency check over all pairs */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxGovts; i++) {
                if (govtData[i].loaded) {
                    for (j = i + 1; j < kMaxGovts; j++) {
                        if (govtData[j].loaded &&
                            GovtEnemies(i, j) && GovtAllies(i, j)) {
                            crashlog_print("warning: govts ");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(" and ");
                            crashlog_print_num(j + kFirstResourceID);
                            crashlog_print(" are simultaneously allies and enemies\n");
                        }
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- përs: persons ---------------- */
    {
        long shieldSum;

        found = 0;
        n = CountResources(kPersType);
        for (i = 0; i < kMaxPersons;i++) {
            personData[i].loaded = 0;
            personData[i].aiType = -1;
            gPersHandle = GetResource(kPersType, i + kFirstResourceID);
            if (gPersHandle != NULL) {
                found++;
                LockAHandle(gPersHandle);
                personData[i].linkSyst = *(short *)(*gPersHandle + 0x00);
                personData[i].govt = *(short *)(*gPersHandle + 0x02);
                personData[i].aiType = *(short *)(*gPersHandle + 0x04);
                personData[i].aggression = *(short *)(*gPersHandle + 0x06);
                personData[i].coward = *(short *)(*gPersHandle + 0x08);
                personData[i].shipType = *(short *)(*gPersHandle + 0x0a);
                if (personData[i].shipType >= kFirstResourceID && personData[i].shipType < kFirstResourceID + kMaxShipTypes) {
                    personData[i].shipType -= kFirstResourceID;
                    if (gShipTypes[personData[i].shipType].techLevel == kResAbsent) {
                        crashlog_print("warning: p\x91" "rs resource ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" refers to nonexistent ship type ");
                        crashlog_print_num(personData[i].shipType + kFirstResourceID);
                        crashlog_print("\n");
                        personData[i].shipType = -1;
                    }
                } else {
                    personData[i].shipType = 0;
                }
                personData[i].credits = *(long *)(*gPersHandle + 0x24);
                personData[i].shieldMod = *(short *)(*gPersHandle + 0x28);
                personData[i].shieldMod = personData[i].shieldMod / 100.0;
                personData[i].commQuote = *(short *)(*gPersHandle + 0x2c);
                personData[i].hailQuote = *(short *)(*gPersHandle + 0x2e);
                personData[i].linkMission = *(short *)(*gPersHandle + 0x30);
                personData[i].flags = *(short *)(*gPersHandle + 0x32);
                personData[i].flags2 = *(short *)(*gPersHandle + 0x17e);
                personData[i].hailPict = *(short *)(*gPersHandle + 0x2a);
                personData[i].rgb5[0] = (*(unsigned long *)(*gPersHandle + 0x17a) >> 19) & 0x1f;
                personData[i].rgb5[1] = (*(unsigned long *)(*gPersHandle + 0x17a) >> 5) & 0x1f;
                personData[i].rgb5[2] = (*(unsigned long *)(*gPersHandle + 0x17a) >> 3) & 0x1f;
                if (personData[i].linkMission >= kFirstResourceID)
                    personData[i].linkMission -= kFirstResourceID;
                else
                    personData[i].linkMission = -1;
                if (personData[i].hailPict < kFirstResourceID)
                    personData[i].hailPict = -1;
                if (personData[i].govt >= kFirstResourceID && personData[i].govt < kFirstResourceID + kMaxGovts)
                    personData[i].govt -= kFirstResourceID;
                else
                    personData[i].govt = -1;
                personData[i].grantClass = *(short *)(*gPersHandle + 0x134);
                personData[i].grantCount = *(short *)(*gPersHandle + 0x136);
                personData[i].grantProb = *(short *)(*gPersHandle + 0x138);
                if (personData[i].grantClass <= 0) {
                    personData[i].grantClass = -1;
                    personData[i].grantCount = 0;
                    personData[i].grantProb = 0;
                } else {
                    if (personData[i].grantProb < 0)
                        personData[i].grantProb = 0;
                    if (personData[i].grantCount < 0)
                        personData[i].grantCount = 0;
                    if (personData[i].grantProb > 100)
                        personData[i].grantProb = 100;
                }
                mountID[1] = *(short *)(*gPersHandle + 0x0e);
                mountID[0] = *(short *)(*gPersHandle + 0x0c);
                mountID[3] = *(short *)(*gPersHandle + 0x12);
                mountID[2] = *(short *)(*gPersHandle + 0x10);
                mountCount[1] = *(short *)(*gPersHandle + 0x16);
                mountCount[0] = *(short *)(*gPersHandle + 0x14);
                mountCount[3] = *(short *)(*gPersHandle + 0x1a);
                mountCount[2] = *(short *)(*gPersHandle + 0x18);
                mountAmmo[1] = *(short *)(*gPersHandle + 0x1e);
                mountAmmo[0] = *(short *)(*gPersHandle + 0x1c);
                mountAmmo[3] = *(short *)(*gPersHandle + 0x22);
                mountAmmo[2] = *(short *)(*gPersHandle + 0x20);
                for (j = 0; j < kMaxWeapTypes; j++) {
                    personData[i].weapCount[j] = 0;
                    personData[i].weapAmmo[j] = 0;
                }
                for (count = 0; count <= 3; count++) {
                    if (mountID[count] >= kFirstResourceID) {
                        personData[i].weapCount[mountID[count] - kFirstResourceID] = mountCount[count];
                        personData[i].weapAmmo[mountID[count] - kFirstResourceID] = mountAmmo[count];
                    }
                }
                strcpy(personData[i].subtitle, *gPersHandle + 0x13a);
                strcpy(personData[i].activeOn, *gPersHandle + 0x34);
                CtoPstr(personData[i].subtitle);
                AdjustHandleSize(gPersHandle, 400);
                personData[i].loaded = 1;
                GetResInfo(gPersHandle, &resID, &resType, name);
                StripNameComment(name);
                strncpy((char *)personData[i].name, (char *)name, 30);
                UnlockAHandle(gPersHandle);
                ReleaseResource(gPersHandle);
                personData[i].prepared = 1;
                HandleIntroMusic(0);
            } else {
                strcpy(personData[i].activeOn, "");
                personData[i].name[0] = 0;
                personData[i].shipType = 0;
                personData[i].govt = -1;
                personData[i].prepared = 0;
            }
            if (found >= n)
                break;
        }

        /* Captain Hector, the shareware enforcer */
        personData[kPersCaptainHector].linkSyst = -1;
        personData[kPersCaptainHector].govt = -1;
        personData[kPersCaptainHector].aiType = 3;
        personData[kPersCaptainHector].aggression = 4;
        personData[kPersCaptainHector].coward = -1;
        personData[kPersCaptainHector].credits = 0;
        personData[kPersCaptainHector].shieldMod = -1.0f;
        personData[kPersCaptainHector].commQuote = 8;
        personData[kPersCaptainHector].hailQuote = -1;
        personData[kPersCaptainHector].linkMission = -1;
        personData[kPersCaptainHector].flags = 2;
        personData[kPersCaptainHector].rgb5[0] = personData[kPersCaptainHector].rgb5[1] = personData[kPersCaptainHector].rgb5[2] = 0;
        personData[kPersCaptainHector].loaded = 1;
        CStr2PStr((char *)personData[kPersCaptainHector].name, "Captain Hector");
        CStr2PStr(personData[kPersCaptainHector].subtitle, "Shareware Enforcer");

        /* pick him a ship: sufficiently shielded and agile vs the fleet average */
        shieldSum = 0;
        n = 0;
        for (i = 0; i < kMaxShipTypes;i++) {
            if (gShipTypes[i].techLevel != kResAbsent && gShipTypes[i].turnRate > 2.0f) {
                shieldSum += gShipTypes[i].shield;
                n++;
            }
        }
        if (n > 0)
            shieldSum /= n;
        else
            shieldSum = 300;
        personData[kPersCaptainHector].shipType = -1;
        for (i = 0; i < kMaxShipTypes;i++) {
            if (gShipTypes[i].techLevel != kResUnloaded && gShipTypes[i].inherentAI >= 2 &&
                gShipTypes[i].shield >= shieldSum * 0.88 &&
                gShipTypes[i].turnRate > 2.0f) {
                personData[kPersCaptainHector].shipType = i;
                break;
            }
        }
        if (personData[kPersCaptainHector].shipType == -1)
            personData[kPersCaptainHector].shipType = 0;

        /* arm him: escalate with the shareware timer */
        for (count = 0; count < kMaxWeapTypes; count++)
            personData[kPersCaptainHector].weapCount[count] = 0;
        j = 0;
        for (count = 0; count < kMaxWeapTypes; count++) {
            if (gWeapons[count].guidance >= kGuidanceUnguided &&
                gWeapons[count].massDmg > 0 && gWeapons[count].energyDmg > 0 &&
                gWeapons[count].subCount <= 0 &&
                (gWeapons[count].massDmg >= shieldSum / 40.0 ||
                 gWeapons[count].energyDmg >= shieldSum / 40.0)) {
                personData[kPersCaptainHector].weapCount[count] = 10;
                personData[kPersCaptainHector].weapAmmo[count] = 1000;
                j++;
                if ((gSharewareDays < 35 && j > 2) ||
                    (gSharewareDays < 40 && j > 3) ||
                    (gSharewareDays < 45 && j > 4) ||
                    (gSharewareDays < 50 && j > 5) ||
                    (gSharewareDays < 55 && j > 6))
                    break;
            }
        }

        /* alias persons that share a name prefix */
        for (count = 0; count < kMaxPersons;count++)
            personData[count].sameNameIndex = count;
        for (i = 0; i < kMaxPersons;i++) {
            if ((char)personData[i].name[0] > 0) {
                for (j = 0; j < i; j++) {
                    if ((char)personData[j].name[0] > 0 &&
                        j == personData[j].sameNameIndex &&
                        strncmp((char *)&personData[i].name[1],
                                (char *)&personData[j].name[1],
                                (char)personData[i].name[0]) == 0) {
                        personData[i].sameNameIndex = j;
                        break;
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* systems must reference existing dudes and persons */
        for (i = 0; i < kMaxSystems; i++) {
            if (gSystems[i].prepared) {
                for (n = 0; n < 8; n++) {
                    short d = gSystems[i].dudeTypes[n];
                    if (d != -1 && !dudeData[d].loaded) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent d\x9f"
                                       "de resource ");
                        crashlog_print_num(gSystems[i].dudeTypes[n] + kFirstResourceID);
                        crashlog_print("\n");
                        gSystems[i].dudeTypes[n] = -1;
                    }
                }
                for (n = 0; n < 8; n++) {
                    short p = gSystems[i].persons[n];
                    if (p != -1 && !personData[p].prepared) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent p\x91"
                                       "rs resource ");
                        crashlog_print_num(gSystems[i].persons[n] + kFirstResourceID);
                        crashlog_print("\n");
                        gSystems[i].persons[n] = -1;
                    }
                }
            }
        }
    }

    /* ---------------- öops: news ---------------- */
    {
        short count;

        found = 0;
        count = CountResources(kOopsType);
        for (i = 0; i < kMaxDisasters; i++) {
            disasterData[i].freq = -1;
            disasterData[i].countdown = -1;
            disasterData[i].duration = -1;
            disasterData[i].stellarID = -1;
            disasterData[i].stellar = -32767;
            disasterData[i].loaded = 0;
        }
        for (i = 0; i < kMaxDisasters; i++) {
            gOopsHandle = GetResource(kOopsType, i + kFirstResourceID);
            if (gOopsHandle != NULL) {
                found++;
                LockAHandle(gOopsHandle);
                disasterData[i].stellar = *(short *)(*gOopsHandle + 0x00);
                disasterData[i].commodity = *(short *)(*gOopsHandle + 0x02);
                disasterData[i].amount = *(short *)(*gOopsHandle + 0x04);
                disasterData[i].duration = *(short *)(*gOopsHandle + 0x06);
                disasterData[i].freq = *(short *)(*gOopsHandle + 0x08);
                disasterData[i].countdown = 0;
                if (disasterData[i].commodity > 5)
                    disasterData[i].commodity = 5;
                if (disasterData[i].commodity < 0)
                    disasterData[i].commodity = -1;
                strcpy(disasterData[i].text, *gOopsHandle + 0x0a);
                disasterData[i].loaded = 1;
                AdjustHandleSize(gOopsHandle, 0x11a);
                GetResInfo(gOopsHandle, &resID, &resType, name);
                StripNameComment(name);
                pstrcpy(disasterData[i].name, name);
                UnlockAHandle(gOopsHandle);
                ReleaseResource(gOopsHandle);
                HandleIntroMusic(0);
            } else {
                strcpy(disasterData[i].text, "");
            }
            if (found >= count)
                break;
        }
    }

    /* ---------------- nëbu: nebulae ---------------- */
    for (i = 0; i < kMaxNebulae; i++) {

        nebulaData[i].width = nebulaData[i].height = 0;
        nebulaData[i].explored = 0;
        gNebuHandle = GetResource(kNebuType, i + kFirstResourceID);
        if (gNebuHandle != NULL) {
            nebulaData[i].xPos = *(short *)(*gNebuHandle + 0x00);
            nebulaData[i].yPos = *(short *)(*gNebuHandle + 0x02);
            nebulaData[i].width = *(short *)(*gNebuHandle + 0x04);
            nebulaData[i].height = *(short *)(*gNebuHandle + 0x06);
            strcpy(nebulaData[i].activeOn, *gNebuHandle + 0x08);
            strcpy(nebulaData[i].onExplore, *gNebuHandle + 0x107);
            ReleaseResource(gNebuHandle);
            HandleIntroMusic(0);
        }
    }

    /* ---------------- bööm: explosions ---------------- */
    for (i = 0; i < 64; i++) {
        explodType[i].frameRate = 0.25f;
        explodType[i].sound = -1;
        explodType[i].graphic = i;
        gBoomHandle = GetResource(kBoomType, i + kFirstResourceID);
        if (gBoomHandle != NULL) {
            explodType[i].frameRate = *(short *)(*gBoomHandle + 0x00);
            explodType[i].sound = *(short *)(*gBoomHandle + 0x02);
            explodType[i].graphic = *(short *)(*gBoomHandle + 0x04);
            explodType[i].frameRate *= 0.01;
            ReleaseResource(gBoomHandle);
            HandleIntroMusic(0);
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);

    /* ---------------- flët: fleets ---------------- */
    {

        found = 0;
        count = CountResources(kFletType);
        for (i = 0; i < kMaxFleets; i++) {
            fleetData[i].leadShipType = -1;
            gFletHandle = GetResource(kFletType, i + kFirstResourceID);
            if (gFletHandle != NULL) {
                found++;
                LockAHandle(gFletHandle);
                fleetData[i].leadShipType = *(short *)(*gFletHandle + 0x00);
                if (fleetData[i].leadShipType >= kFirstResourceID)
                    fleetData[i].leadShipType -= kFirstResourceID;
                else
                    fleetData[i].leadShipType = 0;
                fleetData[i].govt = *(short *)(*gFletHandle + 0x1a);
                fleetData[i].linkSyst = *(short *)(*gFletHandle + 0x1c);
                fleetData[i].quoteStrList = *(short *)(*gFletHandle + 0x11e);
                fleetData[i].flags = *(short *)(*gFletHandle + 0x120);
                if (fleetData[i].govt >= kFirstResourceID)
                    fleetData[i].govt -= kFirstResourceID;
                else
                    fleetData[i].govt = -1;
                for (j = 0; j <= 3; j++) {   /* exe's back-edge is `cmp ax,3 / jle` */
                    fleetData[i].escortTypes[j] = ((short *)(*gFletHandle + 0x02))[j];
                    if (fleetData[i].escortTypes[j] >= kFirstResourceID) {
                        fleetData[i].escortTypes[j] -= kFirstResourceID;
                        fleetData[i].escortMin[j] = ((short *)(*gFletHandle + 0x0a))[j];
                        fleetData[i].escortMax[j] = ((short *)(*gFletHandle + 0x12))[j];
                    } else {
                        fleetData[i].escortTypes[j] = -1;
                        fleetData[i].escortMin[j] = 0;
                        fleetData[i].escortMax[j] = 0;
                    }
                }
                strcpy(fleetData[i].activateOn, *gFletHandle + 0x1e);
                AdjustHandleSize(gFletHandle, 0x132);
                UnlockAHandle(gFletHandle);
                ReleaseResource(gFletHandle);
                HandleIntroMusic(0);
            } else {
                strcpy(fleetData[i].activateOn, "");
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);
    }

    /* ---------------- jünk: junk commodities ---------------- */
    {

        loaded = 0;
        count = CountResources(kJunkType);
        for (i = 0; i < kMaxJunks; i++) {
            for (j = 0; j < 8; j++) {
                junkData[i].boughtAt[j] = -1;
                junkData[i].soldAt[j] = -1;
            }
            junkData[i].basePrice = -1;
        }
        for (i = 0; i < kMaxJunks; i++) {
            junkData[i].held = 0;
            gJunkHandle = GetResource(kJunkType, i + kFirstResourceID);
            if (gJunkHandle != NULL) {
                loaded++;
                LockAHandle(gJunkHandle);
                junkData[i].basePrice = *(short *)(*gJunkHandle + 0x20);
                junkData[i].flags = *(short *)(*gJunkHandle + 0x22);
                junkData[i].scanMask = *(short *)(*gJunkHandle + 0x24);
                if (junkData[i].basePrice < 0) {
                    crashlog_print("warning: junk type ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" has invalid base price\n");
                    junkData[i].basePrice = 0;
                }
                for (j = 0; j < 8; j++) {
                    junkData[i].boughtAt[j] = ((short *)(*gJunkHandle + 0x10))[j];
                    junkData[i].soldAt[j] = ((short *)(*gJunkHandle + 0x00))[j];
                    if (junkData[i].boughtAt[j] >= kFirstResourceID)
                        junkData[i].boughtAt[j] -= kFirstResourceID;
                    else
                        junkData[i].boughtAt[j] = -1;
                    if (junkData[i].soldAt[j] >= kFirstResourceID)
                        junkData[i].soldAt[j] -= kFirstResourceID;
                    else
                        junkData[i].soldAt[j] = -1;
                }
                GetResInfo(gJunkHandle, &resID, &resType, name);
                StripNameComment(name);
                strncpy((char *)junkData[i].name, (char *)name, 63);
                strcpy(junkData[i].lcName, *gJunkHandle + 0x26);
                strcpy(junkData[i].abbrev, *gJunkHandle + 0x66);
                strcpy(junkData[i].buyOn, *gJunkHandle + 0xa6);
                strcpy(junkData[i].sellOn, *gJunkHandle + 0x1a5);
                UnlockAHandle(gJunkHandle);
                ReleaseResource(gJunkHandle);
                HandleIntroMusic(0);
            }
            if (loaded >= count)
                break;
        }

        /* trade centers must trade something */
        for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared &&
                (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) ==
                    (kSpobCanLand | kSpobTradeCenter) &&
                !SpobIsHyperOrWorm(&stellarData[i]) &&
                (stellarData[i].flags & 0x77777700) == 0) {
                n = 0;
                for (count = 0; count < kMaxJunks; count++) {
                    for (j = 0; j < 8; j++) {
                        if (i == junkData[count].boughtAt[j] || i == junkData[count].soldAt[j]) {
                            n++;
                            break;
                        }
                    }
                }
                if (n == 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a trade center but trades no goods\n");
                    stellarData[i].flags = stellarData[i].flags & ~kSpobTradeCenter;
                }
            }
        }
    }

    /* ---------------- crön: cron events ---------------- */
    {

        CountResources(kCronType);   /* result unused; the loop walks every slot */
        for (i = 0; i < kMaxCrons; i++) {
            gCrons[i].active = 0;
            gCrons[i].loaded = 0;
            gCrons[i].random = -1;
            gCrons[i].duration = -1;
            gCrons[i].daysLeft = 0;
            gCrons[i].holdoffLeft = 0;
            gCronHandle = GetResource(kCronType, i + kFirstResourceID);
            if (gCronHandle != NULL) {
                LockAHandle(gCronHandle);
                gCrons[i].firstDay = *(short *)(*gCronHandle + 0x00);
                gCrons[i].firstMonth = *(short *)(*gCronHandle + 0x02);
                gCrons[i].firstYear = *(short *)(*gCronHandle + 0x04);
                gCrons[i].lastDay = *(short *)(*gCronHandle + 0x06);
                gCrons[i].lastMonth = *(short *)(*gCronHandle + 0x08);
                gCrons[i].lastYear = *(short *)(*gCronHandle + 0x0a);
                gCrons[i].random = *(short *)(*gCronHandle + 0x0c);
                gCrons[i].duration = *(short *)(*gCronHandle + 0x0e);
                gCrons[i].preHoldoff = *(short *)(*gCronHandle + 0x10);
                gCrons[i].postHoldoff = *(short *)(*gCronHandle + 0x12);
                gCrons[i].flags = *(short *)(*gCronHandle + 0x16);
                for (j = 0; j < 4; j++) {
                    gCrons[i].govts[j] = ((short *)(*gCronHandle + 0x326))[j];
                    gCrons[i].govtValues[j] = ((short *)(*gCronHandle + 0x32e))[j];
                    if (gCrons[i].govts[j] >= kFirstResourceID) {
                        gCrons[i].govts[j] -= kFirstResourceID;
                    } else {
                        gCrons[i].govts[j] = -1;
                        gCrons[i].govtValues[j] = -1;
                    }
                    if (gCrons[i].govtValues[j] < -1)
                        gCrons[i].govtValues[j] = -1;
                }
                gCrons[i].indNewsStr = *(short *)(*gCronHandle + 0x14);
                for (j = 0; j < 2; j++) {
                    gCrons[i].contributes[j] = ((long *)(*gCronHandle + 0x316))[j];
                    gCrons[i].requires[j] = ((long *)(*gCronHandle + 0x31e))[j];
                }
                strcpy(gCrons[i].enableOn, *gCronHandle + 0x18);
                strcpy(gCrons[i].onStart, *gCronHandle + 0x117);
                strcpy(gCrons[i].onEnd, *gCronHandle + 0x216);
                gCrons[i].loaded = 1;
                UnlockAHandle(gCronHandle);
                AdjustHandleSize(gCronHandle, 0x336);
                ReleaseResource(gCronHandle);
                HandleIntroMusic(0);
            }
        }
    }

    /* ---------------- ränk: ranks ---------------- */
    {

        for (i = 0; i < kMaxRanks; i++) {
            rankData[i].govt = -1;
            rankData[i].active = 0;
            gRankHandle = GetResource(kRankType, i + kFirstResourceID);
            if (gRankHandle != NULL) {
                LockAHandle(gRankHandle);
                rankData[i].loaded = 1;
                rankData[i].govt = *(short *)(*gRankHandle + 0x02);
                if (rankData[i].govt >= kFirstResourceID)
                    rankData[i].govt -= kFirstResourceID;
                else
                    rankData[i].govt = -1;
                rankData[i].priority = *(short *)(*gRankHandle + 0x00);
                rankData[i].contributes[0] = *(long *)(*gRankHandle + 0x0e);
                rankData[i].contributes[1] = *(long *)(*gRankHandle + 0x12);
                rankData[i].flags = *(short *)(*gRankHandle + 0x16);
                rankData[i].salary = *(long *)(*gRankHandle + 0x06);
                rankData[i].salaryCap = *(long *)(*gRankHandle + 0x0a);
                rankData[i].priceMod = *(short *)(*gRankHandle + 0x04);
                if (rankData[i].priceMod <= 0)
                    rankData[i].priceMod = 100;
                GetResInfo(gRankHandle, &resID, &resType, name);
                StripNameComment(name);
                PtoCstr(name);
                strncpy(rankData[i].name, (char *)name, 127);
                strcpy(rankData[i].convName, *gRankHandle + 0x18);
                strcpy(rankData[i].shortName, *gRankHandle + 0x58);
                UnlockAHandle(gRankHandle);
                AdjustHandleSize(gRankHandle, 0x98);
                ReleaseResource(gRankHandle);
                HandleIntroMusic(0);
            } else {
                rankData[i].loaded = 0;
                strcpy(rankData[i].name, "");
                strcpy(rankData[i].convName, "");
                strcpy(rankData[i].shortName, "");
            }
        }
    }

    /* ---------------- röid: asteroids ---------------- */
    {

        for (i = 0; i < kMaxAsteroids; i++) {
            gRoidHandle = GetResource(kRoidType, i + kFirstResourceID);
            if (gRoidHandle != NULL) {
                LockAHandle(gRoidHandle);
                asteroidType[i].strength = *(short *)(*gRoidHandle + 0x00);
                if (asteroidType[i].strength < 1)
                    asteroidType[i].strength = 1;
                asteroidType[i].frameRate = *(short *)(*gRoidHandle + 0x02) * 0.01;
                asteroidType[i].yieldType = *(short *)(*gRoidHandle + 0x04);
                asteroidType[i].yieldQty = *(short *)(*gRoidHandle + 0x06);
                if (asteroidType[i].yieldQty < 0) {
                    asteroidType[i].yieldQty = 0;
                    asteroidType[i].yieldType = -1;
                }
                if (asteroidType[i].yieldType < 0 ||
                    (asteroidType[i].yieldType > 6 && asteroidType[i].yieldType < 1000) ||
                    asteroidType[i].yieldType >= 1128) {
                    asteroidType[i].yieldType = -1;
                    asteroidType[i].yieldQty = 0;
                }
                asteroidType[i].partCount = *(short *)(*gRoidHandle + 0x08);
                if (asteroidType[i].partCount < 0)
                    asteroidType[i].partCount = 0;
                asteroidType[i].color = *(unsigned long *)(*gRoidHandle + 0x0a);
                rgb.red = ((asteroidType[i].color >> 16) & 0xff) << 8;
                rgb.green = ((asteroidType[i].color >> 8) & 0xff) << 8;
                rgb.blue = (asteroidType[i].color & 0xff) << 8;
                if (gSpriteWorld->pixelDepth == 8) {
                    asteroidType[i].color = Color2Index(&rgb);
                } else {
                    asteroidType[i].color = (((rgb.red >> 11) << 10) & 0x7c00) |
                                  (((rgb.green >> 11) << 5) & 0x3e0) |
                                  ((rgb.blue >> 11) & 0x1f);
                }
                asteroidType[i].fragCount = *(short *)(*gRoidHandle + 0x12);
                if (asteroidType[i].fragCount < 0)
                    asteroidType[i].fragCount = 0;
                for (j = 0; j < 2; j++) {
                    asteroidType[i].fragType[j] = ((short *)(*gRoidHandle + 0x0e))[j];
                    if (asteroidType[i].fragType[j] >= kFirstResourceID && asteroidType[i].fragType[j] < 144)
                        asteroidType[i].fragType[j] -= kFirstResourceID;
                    else if (asteroidType[i].fragType[j] < 0 || asteroidType[i].fragType[j] >= 16)
                        asteroidType[i].fragType[j] = -1;
                }
                asteroidType[i].explodType = *(short *)(*gRoidHandle + 0x14);
                if (asteroidType[i].explodType < 0 ||
                    (asteroidType[i].explodType >= 64 && asteroidType[i].explodType < 1000) ||
                    asteroidType[i].explodType >= 1064)
                    asteroidType[i].explodType = -1;
                asteroidType[i].mass = *(short *)(*gRoidHandle + 0x16);
                UnlockAHandle(gRoidHandle);
                AdjustHandleSize(gRoidHandle, 0x28);
                ReleaseResource(gRoidHandle);
                HandleIntroMusic(0);
            }
        }
    }

    /* ---------------- cölr: interface colors ---------------- */
    colorFileHand = GetIndResource(kColrType, 1);
    if (colorFileHand != NULL) {
        long k;
        LockAHandle(colorFileHand);
        for (k = 0; k < 2; k++) {
            menuTextColor[k].blue = (*(short *)(*colorFileHand + 0x4e + k * 4) & 0xff) << 8;
            menuTextColor[k].green = ((*(unsigned long *)(*colorFileHand + 0x4e + k * 4) >> 8) & 0xff) << 8;
            menuTextColor[k].red = ((*(unsigned long *)(*colorFileHand + 0x4e + k * 4) >> 16) & 0xff) << 8;
        }
        for (k = 0; k < 2; k++) {
            shopGridColor[k].blue = (*(short *)(*colorFileHand + 0x56 + k * 4) & 0xff) << 8;
            shopGridColor[k].green = ((*(unsigned long *)(*colorFileHand + 0x56 + k * 4) >> 8) & 0xff) << 8;
            shopGridColor[k].red = ((*(unsigned long *)(*colorFileHand + 0x56 + k * 4) >> 16) & 0xff) << 8;
        }
        for (k = 0; k < 3; k++) {
            progressColor[k].blue = (*(short *)(*colorFileHand + 0x66 + k * 4) & 0xff) << 8;
            progressColor[k].green = ((*(unsigned long *)(*colorFileHand + 0x66 + k * 4) >> 8) & 0xff) << 8;
            progressColor[k].red = ((*(unsigned long *)(*colorFileHand + 0x66 + k * 4) >> 16) & 0xff) << 8;
        }
        for (k = 0; k < 3; k++) {
            buttonColor[k].blue = (*(short *)(*colorFileHand + 0x00 + k * 4) & 0xff) << 8;
            buttonColor[k].green = ((*(unsigned long *)(*colorFileHand + 0x00 + k * 4) >> 8) & 0xff) << 8;
            buttonColor[k].red = ((*(unsigned long *)(*colorFileHand + 0x00 + k * 4) >> 16) & 0xff) << 8;
        }
        for (k = 0; k < 6; k++) {
            menuButtonPos[k].h = *(short *)(*colorFileHand + 0x72 + k * 4);
            menuButtonPos[k].v = *(short *)(*colorFileHand + 0x74 + k * 4);
        }
        floatingBorderColor.blue = (*(short *)(*colorFileHand + 0x8a) & 0xff) << 8;
        floatingBorderColor.green = ((*(unsigned long *)(*colorFileHand + 0x8a) >> 8) & 0xff) << 8;
        floatingBorderColor.red = ((*(unsigned long *)(*colorFileHand + 0x8a) >> 16) & 0xff) << 8;
        escortMenuColor.blue = (*(short *)(*colorFileHand + 0x9a) & 0xff) << 8;
        escortMenuColor.green = ((*(unsigned long *)(*colorFileHand + 0x9a) >> 8) & 0xff) << 8;
        escortMenuColor.red = ((*(unsigned long *)(*colorFileHand + 0x9a) >> 16) & 0xff) << 8;
        ldefColors[0].blue = (*(short *)(*colorFileHand + 0x8e) & 0xff) << 8;
        ldefColors[0].green = ((*(unsigned long *)(*colorFileHand + 0x8e) >> 8) & 0xff) << 8;
        ldefColors[0].red = ((*(unsigned long *)(*colorFileHand + 0x8e) >> 16) & 0xff) << 8;
        ldefColors[1].blue = (*(short *)(*colorFileHand + 0x92) & 0xff) << 8;
        ldefColors[1].green = ((*(unsigned long *)(*colorFileHand + 0x92) >> 8) & 0xff) << 8;
        ldefColors[1].red = ((*(unsigned long *)(*colorFileHand + 0x92) >> 16) & 0xff) << 8;
        ldefColors[2].blue = (*(short *)(*colorFileHand + 0x96) & 0xff) << 8;
        ldefColors[2].green = ((*(unsigned long *)(*colorFileHand + 0x96) >> 8) & 0xff) << 8;
        ldefColors[2].red = ((*(unsigned long *)(*colorFileHand + 0x96) >> 16) & 0xff) << 8;
        progressDefRect.top = *(short *)(*colorFileHand + 0x5e);
        progressDefRect.left = *(short *)(*colorFileHand + 0x60);
        progressDefRect.bottom = *(short *)(*colorFileHand + 0x62);
        progressDefRect.right = *(short *)(*colorFileHand + 0x64);
        strcpy((char *)name, *colorFileHand + 0x0c);
        CtoPstr((char *)name);
        GetFNum(name, &menuFontID);
        menuTextSize = *(short *)(*colorFileHand + 0x4c);
        strcpy((char *)name, *colorFileHand + 0x9e);
        CtoPstr((char *)name);
        GetFNum(name, &buttonFontID);
        buttonFontSize = *(short *)(*colorFileHand + 0xde);
        logoDestPoint.h = *(short *)(*colorFileHand + 0xe0);
        logoDestPoint.v = *(short *)(*colorFileHand + 0xe2);
        rolloverDestPoint.h = *(short *)(*colorFileHand + 0xe4);
        rolloverDestPoint.v = *(short *)(*colorFileHand + 0xe6);
        for (k = 0; k < 3; k++) {
            slideDestPoint[k].h = *(short *)(*colorFileHand + 0xe8 + k * 4);
            slideDestPoint[k].v = *(short *)(*colorFileHand + 0xea + k * 4);
        }
        AdjustHandleSize(colorFileHand, 0xf4);
        UnlockAHandle(colorFileHand);
        ReleaseResource(colorFileHand);
        HandleIntroMusic(0);
    }

    /* ---------------- l33t: easter egg ---------------- */
    if (CountResources(kLeetType) > 0)
        leetMode = 1;
}


/* LoadDescription. STATE: MATCH 100%, 188/188, byte-identical yes (masked relocs, by hand;
   relocregcheck 0 of 40).
   OPEN: none. gDescHandle's VA is unproven (no other MATCHed body names it). see notes */
// FUNCTION: EVNOVA 0x004c6d50
void LoadDescription(char *dst, short id)
{
    short size;
    short count;
    int p;

    gDescHandle = GetResource(kDescType, id);
    if (gDescHandle != NULL) {
        extern char *strcpy(char *dst, const char *src); /* 0x004d6010 MSL; block-scope keeps THIS block's two calls out of
                                                            line -- the else arm's strcpy(dst, "") inlines */
        strcpy(parseBuffer, *gDescHandle);
        size = strlen(*gDescHandle);
        if (GetAHandleSize(gDescHandle) > size) {
            descResourcePictId = ((unsigned char)(*gDescHandle)[size + 1] << 8);
            descResourcePictId += (unsigned char)(*gDescHandle)[size + 2];

            descMovieName[0] = 0;
            count = 0;
            p = size + 3;
            do {
                if ((*gDescHandle)[p] == 0)
                    break;
                descMovieName[0]++;
                descMovieName[count + 1] = (*gDescHandle)[p];
                count++;
                p++;
            } while (count < 0x20);

            gDescFlags = ((unsigned char)(*gDescHandle)[size + 0x23] << 8);
            gDescFlags += (unsigned char)(*gDescHandle)[size + 0x24];
        } else {
            pstrcpy(descMovieName, (unsigned char *)"");
            descResourcePictId = 0;
            gDescFlags = 0;
        }

        ReleaseResource(gDescHandle);
        gDescRankGovt[0] = -1;
        gDescRankGovt[1] = -1;

        ParseTextDescription();
        strcpy(dst, parseBuffer);

        if (leetMode) {
            for (count = 0; count < size; count++) {
                if (dst[count] == 'a' || dst[count] == 'A') {
                    if (Rand(3) < 2)
                        dst[count] = '4';
                }
                if (dst[count] == 'e' || dst[count] == 'E') {
                    if (Rand(3) < 2)
                        dst[count] = '3';
                }
                if (dst[count] == 'i' || dst[count] == 'I') {
                    if (Rand(3) < 2)
                        dst[count] = '1';
                }
                if (dst[count] == 'o' || dst[count] == 'O') {
                    if (Rand(3) < 2)
                        dst[count] = '0';
                }
                if (count < size - 4 && (dst[count] == 'y' || dst[count] == 'Y')
                        && dst[count + 1] == 'o' && dst[count + 2] == 'u') {
                    dst[count] = 'j';
                    dst[count + 1] = '0';
                    dst[count + 2] = '0';
                }
            }
        }
    } else {
        strcpy(dst, "");
        pstrcpy(descMovieName, (unsigned char *)"");
        descResourcePictId = 0;
    }
}

/*
 * Every table is 256-byte Pascal string slots. The `<` and `<=` loop bounds are
 * transcribed as the exe spells them (`jl` against `jle`); they are not
 * interchangeable to mwcc and the difference is per-site.
 */
#define kCargoPluginBase  0x2328
#define kCargoFallback     0xfa0
#define kCargoCount       0x100

#define kGovtPluginBase   0x24b8
#define kGovtFallback      0xfa3
#define kGovtLast              5

#define kLcCargoPluginBase 0x238c
#define kLcCargoFallback    0xfa1
#define kLcCargoCount      0x100

#define kMiscPluginBase   0x23f0
#define kMiscFallback      0xfa2
#define kMiscCount        0x100

#define kHelpList        0x138b
#define kHelpCount        0x14
#define kLegalList          0x86
#define kLegalLast          0x11
#define kRatingList         0x8a
#define kRatingLast          0xa

#define kStrTon                1
#define kStrTons               2
#define kStrCredit          0x20
#define kStrCredits         0x21
#define kFuelFirst             3
#define kFuelCount             8
#define kSecondFirst         0xb
#define kSecondCount           6
#define kThirdFirst         0x10
#define kThirdCount            3
#define kFourthFirst        0x16
#define kFourthCount         0xa

/* LoadStrings. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7040
void LoadStrings(void)
{
    Str255 buf;
    short i;

    for (i = 0; i < kCargoCount; i++)
        if (LoadPluginString((i + kCargoPluginBase), cargoName[i]) == 0) {
            GetIndString(buf, kCargoFallback, i + 1);
            pstrcpy(cargoName[i], buf);
        }

    for (i = 0; i <= kGovtLast; i++)
        if (LoadPluginString((i + kGovtPluginBase), statusCargoName[i]) == 0) {
            GetIndString(buf, kGovtFallback, i + 1);
            pstrcpy(statusCargoName[i], buf);
        }

    for (i = 0; i < kLcCargoCount; i++)
        if (LoadPluginString((i + kLcCargoPluginBase), lcCargoName[i]) == 0) {
            GetIndString(buf, kLcCargoFallback, i + 1);
            pstrcpy(lcCargoName[i], buf);
        }

    for (i = 0; i < kHelpCount; i++) {
        GetIndString(buf, kHelpList, i + 1);
        pstrcpy(shipHelpRequests[i], buf);
    }

    for (i = 0; i <= kLegalLast; i++) {
        GetIndString(buf, kLegalList, i + 1);
        pstrcpy(legalStrings[i], buf);
    }

    for (i = 0; i <= kRatingLast; i++) {
        GetIndString(buf, kRatingList, i + 1);
        pstrcpy(ratingStrings[i], buf);
    }

    for (i = 0; i < kMiscCount; i++)
        if (LoadPluginString((i + kMiscPluginBase), cargoAbbrev[i]) == 0) {
            GetIndString(buf, kMiscFallback, i + 1);
            pstrcpy(cargoAbbrev[i], buf);
        }

    GetIndString(tonString, kGameStrList, kStrTon);
    GetIndString(tonsString, kGameStrList, kStrTons);
    GetIndString(creditString, kGameStrList, kStrCredit);
    GetIndString(creditsString, kGameStrList, kStrCredits);
    strcpy(creditCString, (char *)creditString);
    strcpy(creditsCString, (char *)creditsString);
    PtoCstr((unsigned char *)creditCString);
    PtoCstr((unsigned char *)creditsCString);

    for (i = 0; i < kFuelCount; i++)
        GetIndString(fuelString[i], kGameStrList, i + kFuelFirst);

    for (i = 0; i < kSecondCount; i++)
        GetIndString(shieldString[i], kGameStrList, i + kSecondFirst);

    for (i = 0; i < kThirdCount; i++)
        GetIndString(armorString[i], kGameStrList, i + kThirdFirst);

    for (i = 0; i < kFourthCount; i++)
        GetIndString(newbieString[i], kGameStrList, i + kFourthFirst);
}

/* LoadPluginString (mac _LoadPluginString). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c73b0
char LoadPluginString(short id, void *dst)
{
    Handle h;

    h = GetResource(kStringType, id);
    if (h == NULL)
        return 0;
    memcpy(dst, *h, GetAHandleSize(h));
    ReleaseResource(h);
    return 1;
}

/* LoadPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7400
void LoadPrefs(void)
{
    DateTimeRec dtr;
    char text[256];
    char name[256];
    char path[2048];
    short sig;
    short i;

    GetTime(&dtr);
    GetIniIndString(name, kPrefsStrList, 1);
    strcpy(path, gPrefsDirectory);
    strcat(path, name);
    strcat(path, ".prf");
    gPrefsHandle = (PrefsRecord **)LoadAFileHandle(path);
    if (gPrefsHandle != NULL) {
        LockAHandle((Handle)gPrefsHandle);
        musicEnabledPref = 0;
        DAT_005914d1 = 0;
        DAT_005914d2 = 0;
        gEarlyOutFlag = 0;
        gNoDebris = 0;
        parallaxStarsPref = 0;
        yieldEventsPref = 0;
        windowedPref = 0;
        planetSoundsPref = 0;
        gShowInfluence = 0;
        noHyperGammaPref = 0;
        noVersionCheckPref = 0;
        loadShipSetsPref = 0;
        loadShipGlowPref = 0;
        loadShipWeaponsPref = 0;
        loadShipLightsPref = 0;
        sig = (*gPrefsHandle)->sig;
        if (sig == kPrefsSig) {
            if ((*gPrefsHandle)->musicEnabled != 0)
                musicEnabledPref = 1;
            if ((*gPrefsHandle)->unk4 != 0)
                DAT_005914d1 = 1;
            if ((*gPrefsHandle)->earlyOut != 0)
                gEarlyOutFlag = 1;
            if ((*gPrefsHandle)->noDebris != 0)
                gNoDebris = 1;
            if ((*gPrefsHandle)->windowed != 0)
                windowedPref = 1;
            if ((*gPrefsHandle)->loadShipSets != 0)
                loadShipSetsPref = 1;
            if ((*gPrefsHandle)->loadShipGlow != 0)
                loadShipGlowPref = 1;
            if ((*gPrefsHandle)->loadShipWeapons != 0)
                loadShipWeaponsPref = 1;
            if ((*gPrefsHandle)->loadShipLights != 0)
                loadShipLightsPref = 1;
            if ((*gPrefsHandle)->parallaxStars != 0)
                parallaxStarsPref = 1;
            if ((*gPrefsHandle)->planetSounds != 0)
                planetSoundsPref = 1;
            if ((*gPrefsHandle)->showInfluence != 0)
                gShowInfluence = 1;
            if ((*gPrefsHandle)->noHyperGamma != 0)
                noHyperGammaPref = 1;
            if ((*gPrefsHandle)->noVersionCheck != 0)
                noVersionCheckPref = 1;
            if ((*gPrefsHandle)->unk64 != 0)
                yieldEventsPref = 1;
            volumePref = (*gPrefsHandle)->volume;
            gammaPref = (*gPrefsHandle)->gamma;
            if (gammaPref > 6)
                gammaPref = 3;
            if ((*gPrefsHandle)->unk60 == 0)
                DAT_00735488 = 0;
            else
                DAT_00735488 = 1;
            windowedPrefPending = windowedPref;
            SetDefaultKeys();
            for (i = 0; i <= 35; i++)
                gSlotTable[i] = (*gPrefsHandle)->keys[i];
            SlotTableToKeyPrefs();
            DAT_00735450 = (*gPrefsHandle)->unk50;
            DAT_00735450 *= 0.01;
        }
        UnlockAHandle((Handle)gPrefsHandle);
        DeallocAHandle((Handle)gPrefsHandle);
        if (sig != kPrefsSig) {
            SetDefaultPrefs();
            SetDefaultKeys();
            GetIniIndString(text, kAlertStrList, 0x20);
            ShowAlert("", text);
        }
    } else {
        SetDefaultPrefs();
        SetDefaultKeys();
    }
}

#define kPrefsRecSize   0x8c

/* SavePrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7820
void SavePrefs(void)
{
    char name[256];
    char path[2048];
    short i;

    gPrefsHandle = NULL;
    GetIniIndString(name, kPrefsStrList, 1);
    strcpy(path, gPrefsDirectory);
    strcat(path, name);
    strcat(path, ".prf");
    gPrefsHandle = (PrefsRecord **)AllocAHandleClear(kPrefsRecSize);
    if (gPrefsHandle != NULL) {
        LockAHandle((Handle)gPrefsHandle);
        (*gPrefsHandle)->sig = kPrefsSig;
        if (musicEnabledPref != 0)
            (*gPrefsHandle)->musicEnabled = 1;
        else
            (*gPrefsHandle)->musicEnabled = 0;
        if (DAT_005914d1 != 0)
            (*gPrefsHandle)->unk4 = 1;
        else
            (*gPrefsHandle)->unk4 = 0;
        if (gEarlyOutFlag != 0)
            (*gPrefsHandle)->earlyOut = 1;
        else
            (*gPrefsHandle)->earlyOut = 0;
        if (gNoDebris != 0)
            (*gPrefsHandle)->noDebris = 1;
        else
            (*gPrefsHandle)->noDebris = 0;
        if (windowedPrefPending != 0)
            (*gPrefsHandle)->windowed = 1;
        else
            (*gPrefsHandle)->windowed = 0;
        if (loadShipSetsPref != 0)
            (*gPrefsHandle)->loadShipSets = 1;
        else
            (*gPrefsHandle)->loadShipSets = 0;
        if (loadShipGlowPref != 0)
            (*gPrefsHandle)->loadShipGlow = 1;
        else
            (*gPrefsHandle)->loadShipGlow = 0;
        if (loadShipWeaponsPref != 0)
            (*gPrefsHandle)->loadShipWeapons = 1;
        else
            (*gPrefsHandle)->loadShipWeapons = 0;
        if (loadShipLightsPref != 0)
            (*gPrefsHandle)->loadShipLights = 1;
        else
            (*gPrefsHandle)->loadShipLights = 0;
        if (yieldEventsPref != 0)
            (*gPrefsHandle)->unk64 = 1;
        else
            (*gPrefsHandle)->unk64 = 0;
        if (parallaxStarsPref != 0)
            (*gPrefsHandle)->parallaxStars = 1;
        else
            (*gPrefsHandle)->parallaxStars = 0;
        if (planetSoundsPref != 0)
            (*gPrefsHandle)->planetSounds = 1;
        else
            (*gPrefsHandle)->planetSounds = 0;
        if (gShowInfluence != 0)
            (*gPrefsHandle)->showInfluence = 1;
        else
            (*gPrefsHandle)->showInfluence = 0;
        if (noHyperGammaPref != 0)
            (*gPrefsHandle)->noHyperGamma = 1;
        else
            (*gPrefsHandle)->noHyperGamma = 0;
        if (noVersionCheckPref != 0)
            (*gPrefsHandle)->noVersionCheck = 1;
        else
            (*gPrefsHandle)->noVersionCheck = 0;
        (*gPrefsHandle)->volume = volumePref;
        (*gPrefsHandle)->gamma = gammaPref;
        KeyPrefsToSlotTable();
        for (i = 0; i <= 35; i++)
            (*gPrefsHandle)->keys[i] = gSlotTable[i];
        (*gPrefsHandle)->unk50 = (DAT_00735450 * 100.0);
        if (DAT_00735488 != 0)
            (*gPrefsHandle)->unk60 = 1;
        else
            (*gPrefsHandle)->unk60 = 0;
        (*gPrefsHandle)->unk7E = 0;
        (*gPrefsHandle)->unk80 = 0;
        (*gPrefsHandle)->unk82 = 0;
        (*gPrefsHandle)->unk84 = 0;
        (*gPrefsHandle)->unk86 = 0;
        (*gPrefsHandle)->unk88 = 0;
        UnlockAHandle((Handle)gPrefsHandle);
    }
    if (gPrefsHandle != NULL) {
        LockAHandle((Handle)gPrefsHandle);
        SaveAFile(path, *gPrefsHandle, GetAHandleSize((Handle)gPrefsHandle));
        UnlockAHandle((Handle)gPrefsHandle);
        DeallocAHandle((Handle)gPrefsHandle);
    } else {
        void *f;

        f = FOpenExpandedPath(path, "wb");
        if (f != NULL)
            fclose(f);
    }
}

/* MakeAlias. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7d40
void MakeAlias(char *name)
{
    char base[0x100];
    char path[0x800];

    GetIniIndString(base, kPilotDirList, 4);
    strcpy(path, pilotsFolder);
    strcat(path, base);
    SaveAFile(path, name, strlen(name) + 1);
}

/* mac twin _SavePilotFile. 0x004c7dd0 demonstrably WRITES the .plt (see
   WritePilotData), so the older `LoadPilot' / `LoadPilotFile' names were
   both wrong. */

/* SavePilotFile. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7db0
short SavePilotFile(short stellar)
{
    if (noSavePilotFiles != 0)
        return 0;
    return WritePilotData(stellar);
}

/* WritePilotData. STATE: MATCH 100% (1367 exact, 356 reloc), 1723/1723, byte-identical yes
   (8380 bytes, relocated fields masked), relocregcheck 0 of 356. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004c7dd0
short WritePilotData(short stellar)
{
    char name[0x100];
    char path[0x800];
    void *f;
    MissionState *od;
    MissionState *os;
    MissionRec *md;
    MissionRec *ms;
    RGBColor *paint;
    char sig[4];
    unsigned long long key;
    short i;

    if (noUpdatePilotData != 0)
        return 0;

    playerFileHand = NULL;
    altPlayerFileHand = NULL;
    strcpy(path, pilotsFolder);
    PStr2CStr(name, playerName);
    strcat(path, name);
    strcat(path, ".plt");
    playerFileHand = AllocAHandle2(sizeof(PilotRec), 0);
    altPlayerFileHand = AllocAHandle2(sizeof(PilotRec2), 0);
    f = FOpenExpandedPath(path, "wb");
    if (f != NULL && playerFileHand != NULL && altPlayerFileHand != NULL) {
        LockAHandle((Handle)playerFileHand);
        LockAHandle((Handle)altPlayerFileHand);

        (*playerFileHand)->stellar = stellar;
        (*playerFileHand)->shipType = shipData[0].type;

        key = RT3_GetLicenseCode();
        sig[0] = ((key ^ 0xb3deefe6846a1331ULL) & 0x0fffffffffffffffULL)
                     != 0x00e5b573ac69e57aULL;
        sig[1] = ((key ^ 0x01175258fee9b4f0ULL) & 0x0fffffffffffffffULL)
                     != 0x01ca504ced18b085ULL;
        sig[2] = ((key ^ 0xd9eab6006f7c65a8ULL) & 0x0fffffffffffffffULL)
                     != 0x0935b4147cad71dfULL;
        sig[3] = ((key ^ 0x847ba0013f907235ULL) & 0x0fffffffffffffffULL)
                     != 0x0f02fa941793847fULL;
        for (i = 0; i < 4; i++) {
            if (sig[i] == 0)
                (*playerFileHand)->shipType = 0;
        }

        i = 0;
        do {
            (*playerFileHand)->cargo[i] = shipData[0].cargo[i];
        } while (++i < 6);
        (*playerFileHand)->shield = shipData[0].shield;
        (*playerFileHand)->fuel = shipData[0].fuel;
        (*playerFileHand)->month = gameTime.month;
        (*playerFileHand)->day = gameTime.day;
        (*playerFileHand)->year = gameTime.year;
        for (i = 0; i < 0x800; i++)
            (*playerFileHand)->explore[i] = gSystems[i].exploreLevel;
        for (i = 0; i < 0x200; i++)
            (*playerFileHand)->outfits[i] = shipSystems[i];
        for (i = 0; i < 0x800; i++)
            (*playerFileHand)->legalStatus[i] = playerLegalStatus[i];
        for (i = 0; i < 0x100; i++) {
            (*playerFileHand)->weapCount[i] = shipData[0].weap[i].count;
            (*playerFileHand)->weapAmmo[i] = shipData[0].weap[i].ammo;
        }
        (*playerFileHand)->credits = shipData[0].credits;
        for (i = 0; i < 16; i++) {
            os = &currentMissionObjectives[i];
            od = &(*playerFileHand)->objectives[i];
            od->active = os->active;
            od->returnDone = os->returnDone;
            od->objectivesDone = os->objectivesDone;
            od->failed = os->failed;
            od->flags = os->flags;
            od->dueDate = os->dueDate;
            ms = (MissionRec *)&currentMissions[i];
            md = &(*playerFileHand)->missions[i];
            md->travelStellar = ms->travelStellar;
            md->unk2 = ms->unk2;
            md->returnStellar = ms->returnStellar;
            md->shipCount = ms->shipCount;
            md->shipDude = ms->shipDude;
            md->shipGoal = ms->shipGoal;
            md->shipBehav = ms->shipBehav;
            md->shipStart = ms->shipStart;
            md->shipSyst = ms->shipSyst;
            md->cargoType = ms->cargoType;
            md->cargoQty = ms->cargoQty;
            md->pickupMode = ms->pickupMode;
            md->dropoffMode = ms->dropoffMode;
            md->scanMask = ms->scanMask;
            md->compGovt = ms->compGovt;
            md->compReward = ms->compReward;
            md->datePostIncrement = ms->datePostIncrement;
            md->payment = ms->payment;
            md->specialShipsKilled = ms->specialShipsKilled;
            md->specialShipsBoarded = ms->specialShipsBoarded;
            md->specialShipsDisabled = ms->specialShipsDisabled;
            md->specialShipsJumpedIn = ms->specialShipsJumpedIn;
            md->specialShipsJumpedOut = ms->specialShipsJumpedOut;
            md->initialShipCount = ms->initialShipCount;
            md->canAbort = ms->canAbort;
            md->hasCargo = ms->hasCargo;
            md->unk34 = ms->unk34;
            md->briefDesc = ms->briefDesc;
            md->quickBriefDesc = ms->quickBriefDesc;
            md->loadCargoDesc = ms->loadCargoDesc;
            md->dropCargoDesc = ms->dropCargoDesc;
            md->compDesc = ms->compDesc;
            md->failDesc = ms->failDesc;
            md->refuseDesc = ms->refuseDesc;
            md->shipDoneDesc = ms->shipDoneDesc;
            md->timeLimit = ms->timeLimit;
            md->destStrList = ms->destStrList;
            md->destStrIndex = ms->destStrIndex;
            md->shipDelay = ms->shipDelay;
            md->misnIndex = ms->misnIndex;
            md->shipStrList = ms->shipStrList;
            md->shipStrIndex = ms->shipStrIndex;
            md->shipTypeIndex = ms->shipTypeIndex;
            md->flags = ms->flags;
            md->flags2 = ms->flags2;
            md->unk59 = ms->unk59;
            md->auxShipCount = ms->auxShipCount;
            md->auxShipDude = ms->auxShipDude;
            md->auxShipSyst = ms->auxShipSyst;
            md->auxShipsSpawned = ms->auxShipsSpawned;
            md->auxShipDelay = ms->auxShipDelay;
            md->auxShipsLeft = ms->auxShipsLeft;
            md->shipName = ms->shipName;
            md->shipSubtitle = ms->shipSubtitle;
            md->unkED = ms->unkED;
            md->beginSetString = ms->beginSetString;
            md->setString2 = ms->setString2;
            md->succSetString = ms->succSetString;
            md->failSetString = ms->failSetString;
            md->abortSetString = ms->abortSetString;
            md->shipDoneSetString = ms->shipDoneSetString;
            md->name = ms->name;
        }
        for (i = 0; i < 10000; i++)
            (*playerFileHand)->missionBits[i] = missionBit[i];
        for (i = 0; i < 0x800; i++) {
            if (stellarData[i].prepared != 0)
                (*playerFileHand)->spobFlag[i] = stellarData[i].dominated;
            else
                (*playerFileHand)->spobFlag[i] = 0;
        }
        for (i = 0; i < 0x40; i++) {
            (*playerFileHand)->escortType[i] = -1;
            (*playerFileHand)->fighterType[i] = -1;
            (*playerFileHand)->fighterVoice[i] = -1;
            (*playerFileHand)->escortB[i] = 0;
            (*playerFileHand)->escortA[i] = 0;
        }
        for (i = 0; i < 0x40; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType == 5)
                (*playerFileHand)->fighterType[i] = shipData[i].type;
            else
                (*playerFileHand)->fighterType[i] = -1;
        }
        for (i = 0; i < 0x40; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType == 6 && shipData[i].specialMission == -1) {
                (*playerFileHand)->escortType[i] = shipData[i].type;
                if (shipData[i].hired != 0)
                    (*playerFileHand)->escortType[i] += 1000;
                if (shipData[i].sellOnLanding != 0)
                    (*playerFileHand)->escortB[i] = 1;
                if (shipData[i].upgradeOnLanding != 0)
                    (*playerFileHand)->escortA[i] = 1;
            } else {
                (*playerFileHand)->escortType[i] = -1;
            }
        }
        for (i = 0; i < 0x40; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType >= 5 && shipData[i].specialMission == -1)
                (*playerFileHand)->fighterVoice[i] = shipData[i].voiceClass;
        }
        (*playerFileHand)->kills = playerKills;

        (*altPlayerFileHand)->version = 300;
        if (strictPlay != 0)
            (*altPlayerFileHand)->strict = 1;
        else
            (*altPlayerFileHand)->strict = 0;
        (*altPlayerFileHand)->gender = playerGender;
        strncpy((*altPlayerFileHand)->nickname, (char *)playerNickname, 0x40);
        paint = &(*altPlayerFileHand)->paint;
        paint->red = playerPaint.red;
        paint->green = playerPaint.green;
        paint->blue = playerPaint.blue;
        for (i = 0; i < 0x800; i++) {
            if (stellarData[i].loaded == 0 || stellarData[i].dominated == 0) {
                (*altPlayerFileHand)->defWave[i] = stellarData[i].defWaveSize;
                (*altPlayerFileHand)->tribute[i] = 0;
            } else {
                (*altPlayerFileHand)->defWave[i] = stellarData[i].defWaveSize;
                (*altPlayerFileHand)->tribute[i] = stellarData[i].daysDominated;
            }
        }
        for (i = 0; i < 0x400; i++) {
            if (personData[i].loaded == 0 || personData[i].aiType < 1) {
                (*altPlayerFileHand)->persActive[i] = 0;
                (*altPlayerFileHand)->persB[i] = 0;
            } else {
                (*altPlayerFileHand)->persActive[i] = 1;
                if (personData[i].grudge != 0)
                    (*altPlayerFileHand)->persB[i] = 1;
                else
                    (*altPlayerFileHand)->persB[i] = 0;
            }
        }
        for (i = 0; i < 0x80; i++) {
            if (rankData[i].active != 0)
                (*altPlayerFileHand)->rankOwned[i] = 1;
            else
                (*altPlayerFileHand)->rankOwned[i] = 0;
        }
        (*altPlayerFileHand)->seenIntro = seenIntroScreen;
        for (i = 0; i < 0x100; i++) {
            if (disasterData[i].loaded == 0) {
                (*altPlayerFileHand)->newsC[i] = 0;
                (*altPlayerFileHand)->newsB[i] = -1;
            } else {
                (*altPlayerFileHand)->newsC[i] = disasterData[i].countdown;
                (*altPlayerFileHand)->newsB[i] = disasterData[i].stellarID;
            }
        }
        for (i = 0; i < 0x80; i++)
            (*altPlayerFileHand)->junkHeld[i] = junkData[i].held;
        for (i = 0; i < 0x200; i++) {
            if (gCrons[i].active == 0) {
                (*altPlayerFileHand)->cronA[i] = -1;
                (*altPlayerFileHand)->cronB[i] = -1;
            } else {
                (*altPlayerFileHand)->cronA[i] = gCrons[i].daysLeft;
                (*altPlayerFileHand)->cronB[i] = gCrons[i].holdoffLeft;
            }
        }
        for (i = 0; i < 0x800; i++)
            (*altPlayerFileHand)->systState[i] = gSystems[i].reinforceDaysLeft;
        for (i = 0; i < 0x800; i++)
            (*altPlayerFileHand)->spobDead[i] = stellarData[i].deadDaysLeft;
        (*altPlayerFileHand)->escortOrders[0] = playerEscortOrders[0];
        (*altPlayerFileHand)->escortOrders[1] = playerEscortOrders[1];
        (*altPlayerFileHand)->escortOrders[2] = playerEscortOrders[2];
        (*altPlayerFileHand)->escortOrders[3] = playerEscortOrders[3];
        (*altPlayerFileHand)->globalFlux[0] = globalPriceFlux[0];
        (*altPlayerFileHand)->localFlux[0] = localPriceFlux[0];
        (*altPlayerFileHand)->globalFlux[1] = globalPriceFlux[1];
        (*altPlayerFileHand)->localFlux[1] = localPriceFlux[1];
        strncpy((*altPlayerFileHand)->datePrefix, datePrefix, 0xf);
        strncpy((*altPlayerFileHand)->dateSuffix, dateSuffix, 0xf);
        (*altPlayerFileHand)->datePrefix[0xf] = 0;
        (*altPlayerFileHand)->dateSuffix[0xf] = 0;
        for (i = 0; i < 0x40; i++)
            (*altPlayerFileHand)->unk2006[i] = 0;
        for (i = 0; i < 0x400; i++)
            (*altPlayerFileHand)->unk5efe[i] = 0;

        PStr2CStr(name, shipName);
        LockAHandle((Handle)playerFileHand);
        DoEncryption(*playerFileHand, GetAHandleSize((Handle)playerFileHand),
                     kPilotCryptKey);
        LockAHandle((Handle)altPlayerFileHand);
        DoEncryption(*altPlayerFileHand, GetAHandleSize((Handle)altPlayerFileHand),
                     kPilotCryptKey);
        fwritelong(GetAHandleSize((Handle)playerFileHand), f);
        fwrite(*playerFileHand, GetAHandleSize((Handle)playerFileHand), 1, f);
        fwritelong(GetAHandleSize((Handle)altPlayerFileHand), f);
        fwrite(*altPlayerFileHand, GetAHandleSize((Handle)altPlayerFileHand), 1, f);
        fwritestr(name, f);
        UnlockAHandle((Handle)playerFileHand);
        UnlockAHandle((Handle)altPlayerFileHand);
        DeallocAHandle(playerFileHand);
        DeallocAHandle(altPlayerFileHand);
        fclose(f);
    }
    MakeAlias(path);
    return 0;
}

/* VideoGetCurrentMode fills a four-long display mode: width, height, depth, flags.
   mainscreen.c reads the same block the same way. */

/* OpenPilotFile (mac _OpenPilotFile). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c9e90
char OpenPilotFile(char *missingResource)
{
    OPENFILENAMEA ofn;
    long mode[4];
    char initDir[0x800];
    char path[0x800];
    unsigned char text[256];
    long result;
    void *data;
    int gotFile;

    *missingResource = 0;
    memset(&ofn, 0, sizeof(ofn));

    path[0] = 0;
    ExpandAPathToBufferNative(initDir, pilotsFolder);

    ofn.lStructSize = 0x4c;
    ofn.hwndOwner = Win95MainWindow;
    ofn.lpstrFilter = "EV Nova Pilot\0*.PLT\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = 0x800;
    ofn.lpstrInitialDir = initDir;
    ofn.Flags = 0x1804;
    ofn.lpstrDefExt = "PLT";

    VideoGetCurrentMode(mode);
    if ((mode[3] & kModeFlagOwned) != 0) {
        ReleaseScreenDC();
        SetDisplayToSize(mode[0], mode[1], mode[2], mode[3] & ~kModeFlagOwned);
        AcquireScreenDC();
        MainScreenUpdate(1);
        ScreenEndScene();
    }

    gotFile = GetOpenFileNameA(&ofn);

    while (Win95ApplsActive == 0) {
        Sleep(0);
        KeyboardKbhit();
    }

    if ((mode[3] & kModeFlagOwned) != 0) {
        ReleaseScreenDC();
        VideoSetMode(mode);
        AcquireScreenDC();
    }

    if (gotFile != 0) {

        crashlog("open file setup 1");
        DeleteAllComputerShips(1, 1);
        ResetPlayer(1);
        ResetPlayerRecord(1);
        DeleteAllComputerShips(1, 1);
        crashlog("loading pilot data");
        data = ConvertNativePathToPath(path);
        result = LoadPilotData(data);
        DeallocAPointer(data);
        OSCursorReset();
        ShowTheCursor();
        crashlog("open file setup 2");

        if (result == 0) {
            shipData[0].deathTimer = -1.0f;
            shipData[0].escapePods = -1;
            return 1;
        }
        if (result == -0x2a) {
            GetIndString(text, kAlertStrList, 0x22);
            EVNoteAlert(text);
        } else if (result == -0x2e) {
            *missingResource = 1;
            return 1;
        }
        return 0;
    } else {
        CommDlgExtendedError();
        OSCursorReset();
        ShowTheCursor();
        return 0;
    }
}

/* OpenLastPilotFile (mac _OpenLastPilotFile). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ca120
void OpenLastPilotFile(void)
{
    char name[256];
    char path[0x800];
    unsigned long len;
    char *data;
    short i;
    short j;

    GetIniIndString(name, kPilotDirList, 4);
    strcpy(path, pilotsFolder);
    strcat(path, name);
    data = LoadAFile(path, &len);
    if (data != NULL && data[len - 1] != 0) {
        data = ResizeAPointer(data, len + 1);
        data[len] = 0;
    }
    if (data != NULL && DoesPilotExist(data) != 0) {
        DeleteAllComputerShips(1, 1);
        ResetPlayer(1);
        ResetPlayerRecord(1);
        if (LoadPilotData(data) == 0) {
            gameInProgress = 1;
            for (i = 0; i < 0x800; i++) {
                if (gSystems[i].loaded != 0 && gSystems[i].exploreLevel > 0)
                    ExploreNebulaeFromSystem(i);
            }
            for (j = 0; j < 16; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissions[j].flags & kMissionFlagRearmShips) != 0)
                        currentMissions[j].auxShipsLeft = currentMissions[j].auxShipCount;
                    currentMissions[j].auxShipDelay = (Rand(0x46) + 0x46);
                    currentMissions[j].auxShipsSpawned = 0;
                }
            }
        }
    }
    if (data != NULL)
        DeallocAPointer(data);
}

/* CountEVPlugins counts+logs files of one extension in a directory (filescan.c) */

/* DumpPilotLog (mac _DumpPilotLog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ca2c0
void DumpPilotLog(void)
{
    FILE *log;
    DateTimeRec dtr;
    unsigned char name[256];
    short i;
    short mi;
    short found;

    if ((gShipTypes[gameIsRegisteredIndex].registered != 0 ? 1 : 0) == 0)
        return;

    log = fopen("pilotlog.txt", "r");
    if (log == NULL)
        return;
    fclose(log);
    log = fopen("pilotlog.txt", "w");
    if (log == NULL)
        return;

    fprintf(log, "EV Nova pilot data dump\n\n");
    GetTime(&dtr);
    fprintf(log, "Output on %i/%i/%i at ", dtr.month, dtr.day, dtr.year);
    if (dtr.hour < 12)
        fprintf(log, "%i:%02i AM", dtr.hour, dtr.minute);
    else if (dtr.hour == 12)
        fprintf(log, "%i:%02i PM", dtr.hour, dtr.minute);
    else
        fprintf(log, "%i:%02i PM", dtr.hour - 12, dtr.minute);
    fprintf(log, "\n\n");
    fprintf(log, "Game version:      %s", "1.0.A");
    fprintf(log, "\n\n");

    fprintf(log, "Plugins loaded:\n");
    if (CountEVPlugins("Nova Plug-Ins", ".rez", log) == 0)
        fprintf(log, "  none\n");
    fprintf(log, "\n");

    pstrcpy(name, playerName);
    PtoCstr(name);
    fprintf(log, "Pilot name:        %s\n", name);
    pstrcpy(name, playerNickname);
    PtoCstr(name);
    fprintf(log, "Pilot nickname:    %s\n", name);
    fprintf(log, "Pilot gender:      %s\n", playerGender != 0 ? "male" : "female");
    fprintf(log, "Strict play:       %s\n", strictPlay != 0 ? "yes" : "no");
    fprintf(log, "Seen intro screen: %s\n", seenIntroScreen != 0 ? "yes" : "no");

    pstrcpy(name, shipName);
    PtoCstr(name);
    fprintf(log, "Ship name:         %s\n", name);

    pstrcpy(name, gShipTypes[shipData->type].name);
    PtoCstr(name);
    fprintf(log, "Ship type:         %s (%i)\n", name, shipData->type + 0x80);

    pstrcpy(name, gSystems[shipData->curSystem].name);
    PtoCstr(name);
    fprintf(log, "Current system:    %s (%i)\n", name, shipData->curSystem + 0x80);

    fprintf(log, "Legal status:      %i (in system %i)\n",
            playerLegalStatus[shipData->curSystem], shipData->curSystem + 0x80);

    fprintf(log, "Game date:         %i/%i/%i\n",
            gameTime.month, gameTime.day, gameTime.year);

    fprintf(log, "Fuel:              %0.2f\n", shipData->fuel);
    fprintf(log, "Cash:              %i\n", shipData->credits);
    fprintf(log, "Kills:             %i\n", playerKills);

    fprintf(log, "\nCurrent cargo:\n");
    found = 0;
    for (i = 0; i < kFirstJunkItem; i++) {
        if (shipData->cargo[i] > 0) {
            fprintf(log, "  commodity %i: %i\n", i, shipData->cargo[i]);
            found++;
        }
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            fprintf(log, "  junk type %i: %i\n", i + 0x80, junkData[i].held);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive missions:\n");
    found = 0;
    for (mi = 0; mi < kMaxActiveMissions; mi++) {
        if (currentMissionObjectives[mi].active != 0) {
            found++;
            pstrcpy(name, (unsigned char *)currentMissions[mi].name);
            PtoCstr(name);
            fprintf(log, "  %s (%i):\n", name, currentMissions[mi].misnIndex + 0x80);
            fprintf(log, "    travelObjComplete:        %s\n",
                    currentMissionObjectives[mi].returnDone != 0 ? "yes" : "no");
            fprintf(log, "    shipObjComplete:          %s\n",
                    currentMissionObjectives[mi].objectivesDone != 0 ? "yes" : "no");
            fprintf(log, "    missionFailed:            %s\n",
                    currentMissionObjectives[mi].failed != 0 ? "yes" : "no");
            fprintf(log, "    flags:                    %04X\n", currentMissions[mi].flags);
            fprintf(log, "    flags2:                   %04X\n", currentMissions[mi].flags2);
            fprintf(log, "    canAbort:                 %s\n",
                    currentMissions[mi].canAbort != 0 ? "yes" : "no");
            fprintf(log, "    timeLeft:                 %i\n", currentMissions[mi].timeLimit);
            fprintf(log, "    destStellar:              %i\n",
                    currentMissions[mi].travelStellar == -1 ? -1 : currentMissions[mi].travelStellar + 0x80);
            fprintf(log, "    returnStellar:            %i\n",
                    currentMissions[mi].returnStellar == -1 ? -1 : currentMissions[mi].returnStellar + 0x80);
            fprintf(log, "    initialShipCount:         %i\n", currentMissions[mi].initialShipCount);
            fprintf(log, "    specialShipCount:         %i\n", currentMissions[mi].shipCount);
            fprintf(log, "    specialShipDude:          %i\n",
                    currentMissions[mi].shipDude == -1 ? -1 : currentMissions[mi].shipDude + 0x80);
            fprintf(log, "    specialShipGoal:          %i\n", currentMissions[mi].shipGoal);
            fprintf(log, "    specialShipBehavior:      %i\n", currentMissions[mi].shipBehav);
            fprintf(log, "    specialShipStart:         %i\n", currentMissions[mi].shipStart);
            fprintf(log, "    specialShipSyst:          %i\n", currentMissions[mi].shipSyst);
            fprintf(log, "    specialShipsKilled:       %i\n", currentMissions[mi].specialShipsKilled);
            fprintf(log, "    specialShipsBoarded:      %i\n", currentMissions[mi].specialShipsBoarded);
            fprintf(log, "    specialShipsDisabled:     %i\n", currentMissions[mi].specialShipsDisabled);
            fprintf(log, "    specialShipsJumpedIn:     %i\n", currentMissions[mi].specialShipsJumpedIn);
            fprintf(log, "    specialShipsJumpedOut:    %i\n", currentMissions[mi].specialShipsJumpedOut);
            fprintf(log, "    specialShipNameIndex:     %i\n", currentMissions[mi].destStrIndex);
            fprintf(log, "    specialShipSubtitleIndex: %i\n", currentMissions[mi].shipStrIndex);
            fprintf(log, "    specialShipPreselectType: %i\n", currentMissions[mi].shipTypeIndex);
            fprintf(log, "    auxShipCount:             %i\n", currentMissions[mi].auxShipCount);
            fprintf(log, "    auxShipsLeft:             %i\n", currentMissions[mi].auxShipsLeft);
            fprintf(log, "    auxShipDude:              %i\n",
                    currentMissions[mi].auxShipDude == -1 ? -1 : currentMissions[mi].auxShipDude + 0x80);
            fprintf(log, "    auxShipSyst:              %i\n", currentMissions[mi].auxShipSyst);
            fprintf(log, "    cargoType:                %i\n", currentMissions[mi].cargoType);
            fprintf(log, "    cargoQty:                 %i\n", currentMissions[mi].cargoQty);
            fprintf(log, "    pickupMode:               %i\n", currentMissions[mi].pickupMode);
            fprintf(log, "    dropoffMode:              %i\n", currentMissions[mi].dropoffMode);
            fprintf(log, "    cargoLoaded:              %s\n",
                    currentMissions[mi].hasCargo != 0 ? "yes" : "no");
            fprintf(log, "    scanMask:                 %04X\n", currentMissions[mi].scanMask);
            fprintf(log, "    compGovt:                 %i\n",
                    currentMissions[mi].compGovt == -1 ? -1 : currentMissions[mi].compGovt + 0x80);
            fprintf(log, "    compReward:               %i\n", currentMissions[mi].compReward);
            fprintf(log, "    datePostIncrement:        %i\n", currentMissions[mi].datePostIncrement);
            fprintf(log, "    pay:                      %i\n", currentMissions[mi].payment);
            fprintf(log, "\n");
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nMission bits currently set:\n");
    found = 0;
    for (i = 0; i < kMaxMissionBits; i++) {
        if (missionBit[i] != 0) {
            fprintf(log, "  b%i\n", i);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nEscorts:\n");
    found = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0) {
            found++;
            pstrcpy(name, gShipTypes[shipData[i].type].name);
            PtoCstr(name);
            fprintf(log, "  %s (%i) - ", name, shipData[i].type + 0x80);
            if (shipData[i].aiType == 5)
                fprintf(log, "fighter");
            else if (shipData[i].specialMission != -1)
                fprintf(log, "mission escort");
            else if (shipData[i].hired != 0)
                fprintf(log, "hired");
            else
                fprintf(log, "captured");
            if (shipData[i].upgradeOnLanding != 0)
                fprintf(log, ", marked for upgrade");
            if (shipData[i].sellOnLanding != 0)
                fprintf(log, ", marked for sale");
            fprintf(log, "\n");
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nItems currently owned:\n");
    found = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            pstrcpy(name, gOutfits[i].name);
            PtoCstr(name);
            fprintf(log, "  %i %s (%i)\n", shipSystems[i], name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nStellars currently dominated:\n");
    found = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared != 0 && stellarData[i].dominated != 0) {
            pstrcpy(name, stellarData[i].name);
            PtoCstr(name);
            fprintf(log, "  %s (%i)\n", name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nStellars currently destroyed:\n");
    found = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared != 0 && StellarIsDestroyed(&stellarData[i])) {
            pstrcpy(name, stellarData[i].name);
            PtoCstr(name);
            fprintf(log, "  %s (%i)\n", name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive ranks:\n");
    found = 0;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].loaded != 0 && rankData[i].active != 0) {
            /* the exe passes the PASCAL string straight to %s here, with no
               pstrcpy/PtoCstr conversion (unlike every other name section) -
               read directly off the call site, not a guess */
            fprintf(log, "  %s (%i)\n", rankData[i].name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive disasters:\n");
    found = 0;
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].stellarID != -1) {
            pstrcpy(name, disasterData[i].name);
            PtoCstr(name);
            fprintf(log, "  %s (%i) on ", name, i + 0x80);
            pstrcpy(name, stellarData[disasterData[i].stellarID].name);
            PtoCstr(name);
            fprintf(log, "%s (%i) - %i days left\n",
                    name, disasterData[i].stellarID + 0x80, disasterData[i].countdown);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive cron events:\n");
    found = 0;
    for (i = 0; i < kMaxCrons; i++) {
        if (gCrons[i].loaded != 0 && gCrons[i].active != 0) {
            fprintf(log, "  %i - durationCounter: %i\tholdoffCounter: %i\n",
                    i + 0x80, gCrons[i].daysLeft, gCrons[i].holdoffLeft);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\n\n- end of log -\n");
    fflush(log);
    fclose(log);
}

/* LoadPilotData (mac _LoadPilotData). STATE: MATCH 100.0% (1279 exact, 379 reloc-equivalent);
   exe 1658 / obj 1658; byte-identical yes; relocregcheck 0 of 379.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004cb260
short LoadPilotData(char *path)
{
    char buf[0x40];
    Str255 text;
    void *f;
    MissionState *od;
    MissionState *os;
    MissionRec *md;
    MissionRec *ms;
    RGBColor *paint;
    char *tail;
    short i;
    short j;
    short k;
    short n;
    short err;
    unsigned long len;
    char junk;

    playerFileHand = NULL;
    altPlayerFileHand = NULL;
    junk = 0;

    f = FOpenExpandedPath(path, "rb");
    if (f == NULL)
        goto badFile;

    crashlog("load pilot data 1");
    len = fgetlong(f);
    if (len > 0) {
        playerFileHand = AllocAHandle2(len, 0);
        LockAHandle((Handle)playerFileHand);
        fread(*playerFileHand, len, 1, f);
        UnlockAHandle((Handle)playerFileHand);
    }
    if (playerFileHand != NULL) {
        LockAHandle((Handle)playerFileHand);
        err = DoEncryption(*playerFileHand, GetAHandleSize((Handle)playerFileHand),
                           kPilotCryptKey);
        UnlockAHandle((Handle)playerFileHand);

        if (err == 0) {
            crashlog("load pilot data 2");
            PropagateMissionBitEffects();
            HandleStellarSystemVisibility();

            k = (*playerFileHand)->stellar;
            shipData[0].lastStellar = k;
            if (k == -1) {
                junk = 1;
                crashlog("warning: pilot file stellar ID out of range");
                k = 0;
                for (i = 0; i < kMaxStellars; i++) {
                    if (stellarData[i].prepared != 0) {
                        k = i;
                        break;
                    }
                }
            }
            shipData[0].curSystem = stellarData[k].system;
            if (shipData[0].curSystem < 0 || shipData[0].curSystem >= kMaxSystems)
                shipData[0].curSystem = FindSystemFromStellar(k);
            else if (gSystems[shipData[0].curSystem].prepared == 0)
                shipData[0].curSystem = -1;
            if (shipData[0].curSystem < 0 || shipData[0].curSystem > kMaxSystems) {
                junk = 1;
                crashlog("warning: pilot file system ID out of range");
                shipData[0].curSystem = 0;
                for (i = 0; i < kMaxSystems; i++) {
                    if (gSystems[i].prepared != 0) {
                        shipData[0].curSystem = i;
                        break;
                    }
                }
            }
            PropagateMissionBitEffects();
            HandleStellarSystemVisibility();

            shipData[0].pos.x = stellarData[k].pos.h;
            shipData[0].pos.y = stellarData[k].pos.v;
            shipData[0].heading = Rand(kFullCircle);
            mapScroll.x = gSystems[shipData[0].curSystem].xPos;
            mapScroll.y = gSystems[shipData[0].curSystem].yPos;

            shipData[0].type = (*playerFileHand)->shipType;
            if (gShipTypes[shipData[0].type].techLevel == kResAbsent) {
                junk = 1;
                crashlog("warning: pilot file ship class out of range");
                shipData[0].type = 0;
                for (i = 0; i < kMaxShipTypes; i++) {
                    if (gShipTypes[i].techLevel != kResAbsent) {
                        shipData[0].type = i;
                        break;
                    }
                }
            }

            shipData[0].cargo[0] = (*playerFileHand)->cargo[0];
            shipData[0].cargo[1] = (*playerFileHand)->cargo[1];
            shipData[0].cargo[2] = (*playerFileHand)->cargo[2];
            shipData[0].cargo[3] = (*playerFileHand)->cargo[3];
            shipData[0].cargo[4] = (*playerFileHand)->cargo[4];
            shipData[0].cargo[5] = (*playerFileHand)->cargo[5];

            for (i = 0; i < kMaxOutfits; i++) {
                shipSystems[i] = (*playerFileHand)->outfits[i];
                if (shipSystems[i] > 0 && gOutfits[i].techLevel == kOutfTechAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid outfit item ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipSystems[i] = 0;
                }
            }

            shipData[0].shield = ShipShieldCapacity(shipData);
            shipData[0].armor = ShipArmorCapacity(shipData);
            shipData[0].fuel = (*playerFileHand)->fuel;
            gameTime.month = (*playerFileHand)->month;
            gameTime.day = (*playerFileHand)->day;
            gameTime.year = (*playerFileHand)->year;

            for (i = 0; i < kMaxSystems; i++)
                gSystems[i].exploreLevel = (*playerFileHand)->explore[i];
            for (i = 0; i < kMaxSystems; i++)
                playerLegalStatus[i] = (*playerFileHand)->legalStatus[i];

            for (i = 0; i < kMaxWeapTypes; i++) {
                shipData[0].weap[i].count = (*playerFileHand)->weapCount[i];
                shipData[0].weap[i].ammo = (*playerFileHand)->weapAmmo[i];
                if (shipData[0].weap[i].count > 0 && gWeapons[i].guidance == kResAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid weapon ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipData[0].weap[i].count = 0;
                }
                if (shipData[0].weap[i].ammo > 0 && gWeapons[i].guidance == kResAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid weapon ammo ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipData[0].weap[i].ammo = 0;
                }
            }

            shipData[0].credits = (*playerFileHand)->credits;
            playerKills = (*playerFileHand)->kills;

            crashlog("load pilot data 3");
            for (i = 0; i < 16; i++) {
                os = &(*playerFileHand)->objectives[i];
                od = &currentMissionObjectives[i];
                od->active = os->active;
                od->returnDone = os->returnDone;
                od->objectivesDone = os->objectivesDone;
                od->failed = os->failed;
                od->flags = os->flags;
                od->dueDate = os->dueDate;
                ms = &(*playerFileHand)->missions[i];
                md = (MissionRec *)&currentMissions[i];
                md->travelStellar = ms->travelStellar;
                md->unk2 = ms->unk2;
                md->returnStellar = ms->returnStellar;
                md->shipCount = ms->shipCount;
                md->shipDude = ms->shipDude;
                md->shipGoal = ms->shipGoal;
                md->shipBehav = ms->shipBehav;
                md->shipStart = ms->shipStart;
                md->shipSyst = ms->shipSyst;
                md->cargoType = ms->cargoType;
                md->cargoQty = ms->cargoQty;
                md->pickupMode = ms->pickupMode;
                md->dropoffMode = ms->dropoffMode;
                md->scanMask = ms->scanMask;
                md->compGovt = ms->compGovt;
                md->compReward = ms->compReward;
                md->datePostIncrement = ms->datePostIncrement;
                md->payment = ms->payment;
                md->specialShipsKilled = ms->specialShipsKilled;
                md->specialShipsBoarded = ms->specialShipsBoarded;
                md->specialShipsDisabled = ms->specialShipsDisabled;
                md->specialShipsJumpedIn = ms->specialShipsJumpedIn;
                md->specialShipsJumpedOut = ms->specialShipsJumpedOut;
                md->initialShipCount = ms->initialShipCount;
                md->canAbort = ms->canAbort;
                md->hasCargo = ms->hasCargo;
                md->unk34 = ms->unk34;
                md->briefDesc = ms->briefDesc;
                md->quickBriefDesc = ms->quickBriefDesc;
                md->loadCargoDesc = ms->loadCargoDesc;
                md->dropCargoDesc = ms->dropCargoDesc;
                md->compDesc = ms->compDesc;
                md->failDesc = ms->failDesc;
                md->refuseDesc = ms->refuseDesc;
                md->shipDoneDesc = ms->shipDoneDesc;
                md->timeLimit = ms->timeLimit;
                md->destStrList = ms->destStrList;
                md->destStrIndex = ms->destStrIndex;
                md->shipDelay = ms->shipDelay;
                md->misnIndex = ms->misnIndex;
                md->shipStrList = ms->shipStrList;
                md->shipStrIndex = ms->shipStrIndex;
                md->shipTypeIndex = ms->shipTypeIndex;
                md->flags = ms->flags;
                md->flags2 = ms->flags2;
                md->unk59 = ms->unk59;
                md->auxShipCount = ms->auxShipCount;
                md->auxShipDude = ms->auxShipDude;
                md->auxShipSyst = ms->auxShipSyst;
                md->auxShipsSpawned = ms->auxShipsSpawned;
                md->auxShipDelay = ms->auxShipDelay;
                md->auxShipsLeft = ms->auxShipsLeft;
                md->shipName = ms->shipName;
                md->shipSubtitle = ms->shipSubtitle;
                md->unkED = ms->unkED;
                md->beginSetString = ms->beginSetString;
                md->setString2 = ms->setString2;
                md->succSetString = ms->succSetString;
                md->failSetString = ms->failSetString;
                md->abortSetString = ms->abortSetString;
                md->shipDoneSetString = ms->shipDoneSetString;
                md->name = ms->name;
                if (currentMissionObjectives[i].active != 0) {
                    DateAfterDays(&currentMissionObjectives[i].dueDate,
                                  currentMissions[i].timeLimit);
                    currentMissions[i].shipName[0] = 0;
                    currentMissions[i].shipSubtitle[0] = 0;
                    if (currentMissions[i].destStrList >= kFirstResourceID &&
                        currentMissions[i].destStrIndex >= 1) {
                        GetIndString(text, currentMissions[i].destStrList,
                                     currentMissions[i].destStrIndex);
                        strncpy(currentMissions[i].shipName, (char *)text, 0x3f);
                    }
                    if (currentMissions[i].shipStrList >= kFirstResourceID &&
                        currentMissions[i].shipStrIndex >= 1) {
                        GetIndString(text, currentMissions[i].shipStrList,
                                     currentMissions[i].shipStrIndex);
                        strncpy(currentMissions[i].shipSubtitle, (char *)text, 0x3f);
                    }
                    if ((currentMissions[i].flags & kMissionFlagRearmShips) != 0)
                        currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
                    currentMissions[i].auxShipDelay = (Rand(0x46) + 0x46);
                    currentMissions[i].auxShipsSpawned = 0;
                }
            }

            i = 0;
            do {
                missionBit[i] = (*playerFileHand)->missionBits[i];
            } while (++i < kMaxMissionBits);

            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared != 0)
                    stellarData[i].dominated = (*playerFileHand)->spobFlag[i];
                else
                    stellarData[i].dominated = 0;
            }

            DeleteAllComputerShips(1, 1);

            for (i = 0; i < 0x40; i++) {
                if ((*playerFileHand)->escortType[i] >= 0) {
                    n = ((*playerFileHand)->escortType[i] % 1000);
                    if (gShipTypes[n].techLevel == kResAbsent) {
                        junk = 1;
                        crashlog("warning: pilot file has invalid escort ship type");
                    } else {
                        j = SpawnEscortOfType(n, -1);
                        if (j != -1) {
                            if ((*playerFileHand)->escortType[i] >= 1000) {
                                shipData[j].hired = 1;
                            } else {
                                shipData[j].hired = 0;
                                if ((*playerFileHand)->escortB[i] != 0)
                                    shipData[j].sellOnLanding = 1;
                            }
                            if ((*playerFileHand)->escortA[i] != 0)
                                shipData[j].upgradeOnLanding = 1;
                        }
                    }
                }
                if ((*playerFileHand)->fighterType[i] >= 0 &&
                    (*playerFileHand)->fighterType[i] < kMaxShipTypes) {
                    n = (*playerFileHand)->fighterType[i];
                    if (gShipTypes[n].techLevel == kResAbsent) {
                        junk = 1;
                        crashlog("warning: pilot file has invalid fighter ship type");
                    } else {
                        j = SpawnEscortOfType(n, -1);
                        if (j != -1) {
                            shipData[j].aiType = 5;
                            shipData[j].hired = 0;
                            AISetup(&shipData[j]);
                            if ((*playerFileHand)->fighterVoice[i] != -1)
                                shipData[j].voiceClass = (*playerFileHand)->fighterVoice[i];
                        }
                    }
                }
            }

            DeallocAHandle(playerFileHand);
            crashlog("load pilot data 4");
        }
    } else {
        fclose(f);
        return kPilotUnreadable;
    }

    crashlog("load pilot data 5");
    len = fgetlong(f);
    if (len > 0) {
        altPlayerFileHand = AllocAHandle2(len, 0);
        LockAHandle((Handle)altPlayerFileHand);
        fread(*altPlayerFileHand, len, 1, f);
        UnlockAHandle((Handle)altPlayerFileHand);
    }
    if (altPlayerFileHand != NULL) {
        LockAHandle((Handle)altPlayerFileHand);
        err = DoEncryption(*altPlayerFileHand, GetAHandleSize((Handle)altPlayerFileHand),
                           kPilotCryptKey);
        UnlockAHandle((Handle)altPlayerFileHand);

        if (err == 0) {
            crashlog("load pilot data 6");
            if ((*altPlayerFileHand)->version == kBetaPilotVersion) {
                UnlockAHandle((Handle)altPlayerFileHand);
                DeallocAHandle(altPlayerFileHand);
                fclose(f);
                return kPilotBetaFile;
            }
            if ((*altPlayerFileHand)->version < kMinPilotVersion) {
                UnlockAHandle((Handle)altPlayerFileHand);
                DeallocAHandle(altPlayerFileHand);
                fclose(f);
                return kPilotWrongVersion;
            }

            if ((*altPlayerFileHand)->strict == 1)
                strictPlay = 1;
            else
                strictPlay = 0;
            if ((*altPlayerFileHand)->gender == 1)
                playerGender = 1;
            else
                playerGender = 0;

            strncpy((char *)playerNickname, (*altPlayerFileHand)->nickname, 0x40);
            paint = &(*altPlayerFileHand)->paint;
            playerPaint.red = paint->red;
            playerPaint.green = paint->green;
            playerPaint.blue = paint->blue;

            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared != 0 && stellarData[i].dominated == 0) {
                    stellarData[i].defWaveSize = (*altPlayerFileHand)->defWave[i];
                    stellarData[i].daysDominated = (*altPlayerFileHand)->tribute[i];
                } else {
                    stellarData[i].defWaveSize = 0;
                    stellarData[i].daysDominated = 0;
                }
            }

            for (i = 0; i < kMaxPersons; i++) {
                if (personData[i].aiType >= 1) {
                    if (personData[i].prepared != 0 &&
                        (*altPlayerFileHand)->persActive[i] != 0) {
                        personData[i].loaded = 1;
                        if ((*altPlayerFileHand)->persB[i] != 0)
                            personData[i].grudge = 1;
                        else
                            personData[i].grudge = 0;
                    } else {
                        personData[i].loaded = 0;
                        personData[i].grudge = 0;
                    }
                } else {
                    personData[i].loaded = 0;
                }
            }

            for (i = 0; i < kMaxRanks; i++) {
                if ((*altPlayerFileHand)->rankOwned[i] == 0) {
                    rankData[i].active = 0;
                } else if (rankData[i].loaded != 0) {
                    rankData[i].active = 1;
                } else {
                    junk = 1;
                    crashlog("warning: pilot file contains invalid rank");
                    rankData[i].active = 0;
                }
            }

            crashlog("load pilot data 7");
            seenIntroScreen = (*altPlayerFileHand)->seenIntro;
            lastMissionStellar = -1;

            for (i = 0; i < kMaxDisasters; i++) {
                if (disasterData[i].loaded != 0) {
                    disasterData[i].countdown = (*altPlayerFileHand)->newsC[i];
                    disasterData[i].stellarID = (*altPlayerFileHand)->newsB[i];
                } else {
                    disasterData[i].countdown = 0;
                    disasterData[i].stellarID = -1;
                }
            }

            for (i = 0; i < kMaxJunks; i++) {
                junkData[i].held = (*altPlayerFileHand)->junkHeld[i];
                if (junkData[i].held > 0 && junkData[i].basePrice < 0) {
                    junk = 1;
                    crashlog("warning: pilot file contains invalid junk type");
                    junkData[i].held = 0;
                }
            }

            for (i = 0; i < kMaxCrons; i++) {
                if (gCrons[i].loaded != 0) {
                    gCrons[i].daysLeft = (*altPlayerFileHand)->cronA[i];
                    gCrons[i].holdoffLeft = (*altPlayerFileHand)->cronB[i];
                    if (gCrons[i].daysLeft >= 0 || gCrons[i].holdoffLeft >= 0)
                        gCrons[i].active = 1;
                    else
                        gCrons[i].active = 0;
                } else {
                    gCrons[i].active = 0;
                }
            }

            i = 0;
            do {
                gSystems[i].reinforceDaysLeft = (*altPlayerFileHand)->systState[i];
            } while (++i < kMaxSystems);

            for (i = 0; i < kMaxStellars; i++) {
                if ((*altPlayerFileHand)->spobDead[i] > 0) {
                    stellarData[i].deadDaysLeft = (*altPlayerFileHand)->spobDead[i];
                    stellarData[i].deadCounter = -1;
                } else {
                    stellarData[i].deadDaysLeft = -1;
                    stellarData[i].deadCounter = stellarData[i].strength;
                }
            }

            for (i = 0; i < 4; i++) {
                playerEscortOrders[i] = -1;
                if ((*altPlayerFileHand)->escortOrders[i] >= 0)
                    playerEscortOrders[i] = (*altPlayerFileHand)->escortOrders[i];
            }

            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active != 0 && shipData[i].parentShip == 0)
                    shipData[i].escortOrder =
                        playerEscortOrders[gShipTypes[shipData[i].type].escortType];
            }

            i = 0;
            do {
                globalPriceFlux[i] = (*altPlayerFileHand)->globalFlux[i];
                localPriceFlux[i] = (*altPlayerFileHand)->localFlux[i];
            } while (++i < 2);

            strncpy(datePrefix, (*altPlayerFileHand)->datePrefix, 0xf);
            strncpy(dateSuffix, (*altPlayerFileHand)->dateSuffix, 0xf);
            datePrefix[0xf] = 0;
            dateSuffix[0xf] = 0;

            DeallocAHandle(altPlayerFileHand);
            crashlog("load pilot data 8");
        }
    } else {
        fclose(f);
        return kPilotUnreadable;
    }

    crashlog("load pilot data 9");
    fgetstr(buf, 0x40, f);
    CStr2PStr((char *)shipName, buf);

    if (path[strlen(path) - 1] == ':')
        path[strlen(path) - 1] = 0;
    tail = strrchr(path, ':');
    strncpy((char *)playerName, tail + 1, 0x1f);
    if (strrchr((char *)playerName, '.'))
        *strrchr((char *)playerName, '.') = 0;
    CStr2PStr((char *)playerName, (char *)playerName);

    crashlog("making alias");
    MakeAlias(path);

    shipData[0].foldDir = 0;
    shipData[0].frameTimer = 0;
    shipData[0].foldSet = gShipTypes[shipData[0].type].baseSets - 1;
    shipData[0].shieldGlow = 0;
    shipData[0].weapGlow = 0;
    shipData[0].annoyance = 0;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0) {
            if ((gShipTypes[shipData[i].type].shanFlags & kShipKeepFireGroup) != 0)
                shipData[i].foldSet =
                    gShipTypes[shipData[0].type].baseSets - 1;
            else
                shipData[i].foldSet = 0;
        }
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        gShipTypes[i].dailyHireRoll = (Rand(100) + 1);
        gShipTypes[i].dailySaleRoll = (Rand(100) + 1);
    }
    for (i = 0; i < kMaxOutfits; i++)
        gOutfits[i].dailyRoll = (Rand(100) + 1);

    crashlog("dumping pilot log");
    DumpPilotLog();

    if (junk == 0)
        goto ok;
    return kPilotHasJunk;

badFile:
    return kPilotUnreadable;

ok:
    fclose(f);
    return kPilotOK;
}

/* DoesPilotExist. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd030
char DoesPilotExist(const char *path)
{
    return DoesFileExist(path);
}

/* DeletePilotFile. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd040
void DeletePilotFile(const char *name)
{
    char path[2048];

    strcpy(path, pilotsFolder);
    strcat(path, name);
    strcat(path, ".plt");
    if (!DoesPilotExist(path))
        return;
    DeleteAFile(path);
}

#define kEBugLastField 0x1f

#define kCharRecordSize 0x16a
#define kCharIsDefault 0x0001      /* chär Flags bit 0: offer this one first */

/* one debug switch per field index; absent resource or out-of-range index = off */
/* EVBugResourceFieldTest (mac _EVBugResourceFieldTest). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd0b0
char EVBugResourceFieldTest(short field)
{
    Handle h;
    char on;

    on = 0;
    if (field >= 0 && field <= kEBugLastField) {
        h = GetResource(kEBugType, 128);
        if (h != NULL) {
            if (((short *)*h)[field] != 0)
                on = 1;
            ReleaseResource(h);
        }
        return on;
    }
    return 0;
}

/* The memset is inlined by mwcc (rep stosb): it needs -opt level=4,intrinsics with <string.h>
   included. */
/* AdjustHandleSize (mac _AdjustHandleSize). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd100
void AdjustHandleSize(Handle h, long newSize)
{
    long size;

    if (h == NULL)
        return;
    size = GetAHandleSize(h);
    if (size != newSize) {
        ResizeAHandle(h, newSize);
        if (size < newSize)
            memset(*h + size, 0, newSize - size);
    }
}

/* leaves the buffer empty if the list is missing or has no entries */
/* GetRandomIndString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd140
void GetRandomIndString(unsigned char *str, short strListID)
{
    Handle h;
    short count;

    str[0] = 0;
    h = GetResource(kStringListType, strListID);
    if (h != NULL) {
        count = SwapEndianShort(&((StringListRec *)*h)->count);
        ReleaseResource(h);
        if (count > 0)
            GetIndString(str, strListID, Rand(count) + 1);
    }
}

/* AppendIndString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd1a0
void AppendIndString(char *dst, short strListID, short index)
{
    unsigned char str[256];

    GetIndString(str, strListID, index);
    PtoCstr(str);
    strcat(dst, (char *)str);
}

/* DrawIndString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd1f0
void DrawIndString(short strListID, short index)
{
    unsigned char str[256];

    GetIndString(str, strListID, index);
    DrawString(str);
}

/*
 * Pascal resource names carry a ';'-introduced comment (Nova Bible
 * convention); truncate at the last one and trim the spaces before it.
 */
/* StripNameComment. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd230
void StripNameComment(unsigned char *name)
{
    short i;
    short len;
    char seen;
    int c;

    len = *name;
    seen = 0;
    for (i = *name; i >= 1; i--) {
        if (seen == 0) {
            c = name[i];
            if (c == ';') {
                seen = 1;
                len = i - 1;
            }
        } else {
            c = name[i];
            if (c == ' ') {
                len = i - 1;
            } else if (c != ';') {
                len = i;
                break;
            }
        }
    }
    *name = len;
}

/* the New Pilot screen starts on whichever character carries the flag */
/* GetDefaultCharacterName. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd290
void GetDefaultCharacterName(unsigned char *name)
{
    short i;
    short count;
    short id;
    unsigned long type;

    count = CountResources(kCharType);
    pstrcpy(name, (unsigned char *)"");
    for (i = 0; i < count; i++) {
        if (name[0] == 0) {
            /* cleared first so a failed load leaves no stale handle behind */
            charFileHand = NULL;
            charFileHand = GetIndResource(kCharType, (i + 1));
            if (charFileHand != NULL) {
                if ((((CharRecord *)*charFileHand)->flags & kCharIsDefault) != 0)
                    GetResInfo(charFileHand, &id, &type, name);
                AdjustHandleSize(charFileHand, kCharRecordSize);
                ReleaseResource(charFileHand);
            }
        }
    }
}

#define kIntroSlides    4
#define kMaxIntroDelay  300

/* GetCharacterShipIndex. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd350
short GetCharacterShipIndex(const unsigned char *name)
{
    short index;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
        index = ((CharRecord *)*charFileHand)->shipType;
        if (index >= kFirstResourceID)
            index -= kFirstResourceID;
        else
            index = 0;
        AdjustHandleSize(charFileHand, kCharRecordSize);
        ReleaseResource(charFileHand);
    } else {
        index = 0;
    }
    return index;
}

/* GetCharResourceIntroIDs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd3b0
void GetCharResourceIntroIDs(const unsigned char *name, short *introText, short *pics,
                  short *delays)
{
    short i;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
        *introText = ((CharRecord *)*charFileHand)->introTextID;
        for (i = 0; i < kIntroSlides; i++) {
            pics[i] = ((CharRecord *)*charFileHand)->introPICT[i];
            delays[i] = ((CharRecord *)*charFileHand)->introDelay[i];
            if (pics[i] >= kFirstResourceID) {
                if (delays[i] < 0)
                    delays[i] = 0;
                if (delays[i] > kMaxIntroDelay)
                    delays[i] = kMaxIntroDelay;
            } else {
                pics[i] = -1;
                delays[i] = 0;
            }
        }
        AdjustHandleSize(charFileHand, kCharRecordSize);
        ReleaseResource(charFileHand);
    } else {
        *introText = 0x7ffd;
        pics[0] = 0x2008;
        delays[0] = 10;
        pics[1] = -1;
        delays[1] = 0;
        pics[2] = -1;
        delays[2] = 0;
        pics[3] = -1;
        delays[3] = 0;
    }
}

#define kStartSystSlots       4
#define kGovtSlots            4
#define kFallbackCredits 0x2710
#define kFallbackDate     0x8ca

/* ActivateCharResource (mac _ActivateCharResource). STATE: MATCH 100.0% (145 exact, 56
   reloc-equivalent, 0 mismatched), exe 201 / obj 201, blind LCS 201 (100.0%), 0 clusters;
   relocregcheck 0 of 56 reloc rows differ; byte identity NOT CHECKED (baseline unadopted).
   OPEN: none. */
// FUNCTION: EVNOVA 0x004cd4b0
void ActivateCharResource(const unsigned char *name, char applyStats)
{
    short n;
    short j;
    short i;
    short filled;
    short pick;
    short govt;
    CharRecord **cr;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
    LockAHandle(charFileHand);
    if (applyStats != 0) {
        shipData->credits = (*(CharRecord **)charFileHand)->cash;
        if (shipData->credits < 0)
            shipData->credits = 0;
        shipData->type = (*(CharRecord **)charFileHand)->shipType;
        if (shipData->type >= kFirstResourceID)
            shipData->type -= kFirstResourceID;
        else
            shipData->type = 0;
        cr = (CharRecord **)charFileHand;
        playerKills = (*cr)->kills;

        filled = 0;
        for (j = 0; j < kStartSystSlots; j++)
            if ((*cr)->startSyst[j] >= kFirstResourceID)
                filled++;
        if (filled > 0) {
            do {
                pick = Rand(kStartSystSlots);
            } while ((*(CharRecord **)charFileHand)->startSyst[pick] < kFirstResourceID);
            shipData->curSystem =
                ((*(CharRecord **)charFileHand)->startSyst[pick] - kFirstResourceID);
        } else {
            shipData->curSystem = 0;
        }

        for (i = 0; i < kGovtSlots; i++) {
            govt = (*(CharRecord **)charFileHand)->govt[i];
            if (govt >= kFirstResourceID) {
                govt -= kFirstResourceID;
                for (n = 0; n < kMaxSystems; n++) {
                    if (GovtAllies(gSystems[n].govt, govt))
                        playerLegalStatus[n] =
                            (*(CharRecord **)charFileHand)->standing[i];
                    else if (GovtEnemies(gSystems[n].govt, govt))
                        playerLegalStatus[n] =
                            -(*(CharRecord **)charFileHand)->standing[i];
                }
            }
        }

        cr = (CharRecord **)charFileHand;
        scenarioStartDate.day = (*cr)->startDay;
        scenarioStartDate.month = (*cr)->startMonth;
        scenarioStartDate.year = (*cr)->startYear;
        strcpy(datePrefix, (*cr)->prefix);
        strcpy(dateSuffix, (*(CharRecord **)charFileHand)->suffix);
    } else {
        crashlog_mission_bit_set("starting character", (char *)name, 1, -1,
                                 (*(CharRecord **)charFileHand)->onStart);
        EvalMissionBitSetString((*(CharRecord **)charFileHand)->onStart);
    }

    UnlockAHandle(charFileHand);
    AdjustHandleSize(charFileHand, kCharRecordSize);
    ReleaseResource(charFileHand);
    } else {
        shipData->credits = kFallbackCredits;
        shipData->type = 0;
        shipData->curSystem = 0;
        playerKills = 0;
        scenarioStartDate.day = 1;
        scenarioStartDate.month = 1;
        scenarioStartDate.year = kFallbackDate;
        strcpy(datePrefix, "");
        strcpy(dateSuffix, "");
    }
}

/* ---- VerifyResourceChecksums (0x004cd7e0) ---- */

#define kCheckedTypes        0x17
#define kChecksumResType 0x63739f6d   /* 'cs.m' */
#define kStrBadChecksum         7

/*
 * The type list is twenty-three separate assignments into a local array, not
 * an aggregate initialiser -- the exe stores twenty-three immediates rather
 * than copying a .rdata blob.
 */
/* VerifyResourceChecksums. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd7e0
void VerifyResourceChecksums(void)
{
    unsigned char alert[256];
    unsigned long resType[kCheckedTypes];
    unsigned long type;
    long expected;
    short t;
    short i;
    short n;
    long sum;
    Handle h;

    resType[0] = kBoomType;
    resType[1] = kCharType;
    resType[2] = kColrType;
    resType[3] = kCronType;
    resType[4] = kDudeType;
    resType[5] = kFletType;
    resType[6] = kGovtType;
    resType[7] = kIntfType;
    resType[8] = kJunkType;
    resType[9] = kMisnType;
    resType[10] = kNebuType;
    resType[11] = kOutfType;
    resType[12] = kOopsType;
    resType[13] = kPersType;
    resType[14] = kRankType;
    resType[15] = kRoidType;
    resType[16] = kShipType;
    resType[17] = kSpinType;
    resType[18] = kSpobType;
    resType[19] = kSystType;
    resType[20] = kWeapType;
    resType[21] = 0x79918aa8;
    resType[22] = kIntfType;

    if (gShipTypes[gameIsRegisteredIndex].registered != 0 ? 1 : 0)
        return;

    sum = 0;
    expected = 0;
    for (t = 0; t < kCheckedTypes; t++) {
        IterateInGameEventLoop(1);
        type = resType[t];
        n = CountResources(type);
        if (n > 0) {
            for (i = 0; i < n; i++) {
                h = GetIndResource(type, (i + 1));
                if (h != NULL) {
                    LockAHandle(h);
                    sum += MemoryChecksum(*h, GetAHandleSize(h));
                    UnlockAHandle(h);
                    ReleaseResource(h);
                }
            }
        }
    }

    h = GetIndResource(kChecksumResType, 1);
    if (h != NULL) {
        LockAHandle(h);
        expected = **(long **)h;
        UnlockAHandle(h);
        ReleaseResource(h);
    }
    if (sum != expected) {
        ShowTheCursor();
        GetIndString(alert, kAlertStrList, kStrBadChecksum);
        EVNoteAlert(alert);
        ExitToShell();
    }
}
