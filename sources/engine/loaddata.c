// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
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

/* a resource handle's record: 1.0.10 casts the handle, an array field then indexes off the record */
#ifdef MAC1010
#define ResRec(T, h) (*(T **)(h))
#else
#define ResRec(T, h) ((T *)*(h))
#endif
#define ClampRGB(c) \
    if ((c).red > 0xffff) (c).red = 0xffff; \
    if ((c).green > 0xffff) (c).green = 0xffff; \
    if ((c).blue > 0xffff) (c).blue = 0xffff
#include "gameutil.h"
#include "stellar.h"
#include "burger.h"
#include "macport.h"
#include "resources.h"
#include "cefix.h"
#include "sound.h"
#include "ships.h"
#include "ai.h"
#include "shipobj.h"
#include "missions.h"
#include "space.h"
#include "pilotfile.h"
#include "prototypes.h"
#include "endianswap.h"

#define kPrefStringMax 0x100

#if V111 /* V111: the 1.1.1 source file is UTF-8 */
#define kSpobResName "sp\xc3\xb6" "b"
#define kSystResName "s\xc3\xbf" "st"
#define kShipResName "sh\xc3\xaf" "p"
#define kOutfResName "o\xc3\xbc" "tf"
#define kDudeResName "d\xc3\xbc" "de"
#define kPersResName "p\xc3\xab" "rs"
#else
#define kSpobResName "sp\x9a" "b"
#define kSystResName "s\xd8" "st"
#define kShipResName "sh\x95" "p"
#define kOutfResName "o\x9f" "tf"
#define kDudeResName "d\x9f" "de"
#define kPersResName "p\x91" "rs"
#endif

#ifdef MACBUILD
#define kLdefRed 0
#define kLdefGreen 1
#define kLdefBlue 2
/* a pilot file's type 'Np' 0x95 'L' and the game's creator 'N' 0x9a 'v' 0x8a (MacRoman) */
#define kPilotFileType 0x4e70954c
#define kNovaCreator 0x4e9a768a
/* CoreGraphics' private window-server calls (navEventHandler) */
extern int _CGSDefaultConnection(void);
extern int CGSGetWindowCount(int cid, int owner, int *count);
extern int CGSGetWindowList(int cid, int owner, int size, int *list, int *count);
extern int CGSSetWindowLevel(int cid, int window, int level);
extern Boolean ColorSearchProc(RGBColor *rgb, int32_t *position);
extern OSErr MyOpenDocument(const FSSpec *defaultSpec, FSSpec *outSpec);
extern short pilotResNum;
#define kPilotResID 128
extern OSStatus FlipNpil(short resID, void *data, int32_t size, Boolean toDisk);
/* AmbrosiaTools */
extern OSErr FSRefMakeFSSpec(const FSRef *ref, FSSpec *spec);
extern short gARef;
/* the file type of a pilot file, MacRoman */
#define kPilotFileType 0x4e70954c
FSRef *novaSupportFolder = NULL;
#endif

/*
 * Set up the pilot-file directory at startup (win 0x004bd0c0): "9:" plus
 * string 3 of list 130 plus ":", which is the port's volume notation for a
 * folder beside the application. The two debug switches are read here because
 * this is the first thing that runs after the resource file opens.
 */
#ifdef MACBUILD
#if V111
/* SelectNovaSupport (mac _SelectNovaSupport): mac only. Asks for the EV Nova support folder,
   offering Application Support/EV Nova (created if missing). */
// MAC: _SelectNovaSupport i386 0x0007111e ppc 0x00079b80
void SelectNovaSupport(void)
{
    FSRef novaRef;
    FSRef asupRef;
    HFSUniStr255 uniName;
    NavDialogCreationOptions options;
    AEDesc defaultLocation;
    NavDialogRef dialog;
    Size actualSize;
    NavReplyRecord reply;
    OSErr err;
    DescType actualType;
    AEKeyword keyword;
    int32_t count;

    if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &asupRef) != noErr)
        return;
    HFSNameGetUnicodeName("\pEV Nova", kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&asupRef, uniName.length, uniName.unicode, kTextEncodingUnknown,
                           &novaRef) != noErr)
        FSCreateDirectoryUnicode(&asupRef, uniName.length, uniName.unicode, kFSCatInfoNone, NULL,
                                 &novaRef, NULL, NULL);
    err = NavGetDefaultDialogCreationOptions(&options);
    if (err != noErr)
        return;
    options.optionFlags |= kNavNoTypePopup | kNavSelectDefaultLocation;
    if (AECreateDesc(typeFSRef, &novaRef, sizeof(FSRef), &defaultLocation) != noErr)
        return;
    err = NavCreateChooseFolderDialog(&options, NULL, NULL, NULL, &dialog);
    if (err == noErr) {
        NavCustomControl(dialog, kNavCtlSetLocation, &defaultLocation);
        NavDialogRun(dialog);
        err = NavDialogGetReply(dialog, &reply);
        if (err == noErr && reply.validRecord
            && AECountItems(&reply.selection, &count) == noErr && count == 1) {
            novaSupportFolder = malloc(sizeof(FSRef));
            AEGetNthPtr(&reply.selection, 1, typeFSRef, &keyword, &actualType, novaSupportFolder,
                        sizeof(FSRef), &actualSize);
        }
        NavDialogDispose(dialog);
    }
    AEDisposeDesc(&defaultLocation);
}
#endif
#endif

/* InitDataFiles. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bd0c0
// MAC: _InitDataFiles i386 0x0007138b ppc 0x00079dc4
void InitDataFiles(void)
#ifdef MAC1010
{
    Str255 name;

    gARef = CurResFile();
    noSavePilotFiles = EVBugResourceFieldTest(12);
    noUpdatePilotData = EVBugResourceFieldTest(13);
    GetIndString(name, kPilotDirList, 3);
    GetSubfolderOfFile(gARef, name, &pilotsDirID, &pilotsVRefNum);
    FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &prefsVRefNum, &prefsDirID);
    lastIntfResourceID = -1;
}
#elif defined(MACBUILD)
{
    FSRef asupRef;
    FSRef ref;
    Str255 unusedName;
    Str255 name;
    HFSUniStr255 uniName;
    CFURLRef url;
    CFURLRef filesURL;
    int32_t err;
    int32_t count;

    url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("Nova-DF"), CFSTR("rsrc"), NULL);
    if (url != NULL) {
        CFURLGetFSRef(url, &ref);
        gARef = EVOpenResourceFile(&ref, 1);
        UseResFile(gARef);
        CFRelease(url);
    }
    noSavePilotFiles = EVBugResourceFieldTest(12);
    noUpdatePilotData = EVBugResourceFieldTest(13);
    if (novaSupportFolder == NULL) {
        GetIndString(unusedName, kPilotDirList, 3);
        if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &asupRef) != noErr)
            return;
        novaSupportFolder = malloc(sizeof(FSRef));
        HFSNameGetUnicodeName("\pEV Nova", kTextEncodingUnknown, &uniName);
        if (FSMakeFSRefUnicode(&asupRef, uniName.length, uniName.unicode, kTextEncodingUnknown,
                               novaSupportFolder) != noErr)
            FSCreateDirectoryUnicode(&asupRef, uniName.length, uniName.unicode, kFSCatInfoNone, NULL,
                                     novaSupportFolder, NULL, NULL);
    }
    GetIndString(name, kPilotDirList, 3);
    HFSNameGetUnicodeName(name, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(novaSupportFolder, uniName.length, uniName.unicode, kTextEncodingUnknown,
                           &pilotsFolder) != noErr)
        FSCreateDirectoryUnicode(novaSupportFolder, uniName.length, uniName.unicode, kFSCatInfoNone,
                                 NULL, &pilotsFolder, NULL, NULL);
    HFSNameGetUnicodeName("\pPlug-Ins", kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(novaSupportFolder, uniName.length, uniName.unicode, kTextEncodingUnknown,
                           &pluginsFolder) != noErr)
        FSCreateDirectoryUnicode(novaSupportFolder, uniName.length, uniName.unicode, kFSCatInfoNone,
                                 NULL, &pluginsFolder, NULL, NULL);
    HFSNameGetUnicodeName("\pNova Files", kTextEncodingUnknown, &uniName);
    err = FSMakeFSRefUnicode(novaSupportFolder, uniName.length, uniName.unicode,
                             kTextEncodingUnknown, &novaFilesFolder);
    count = 0;
    if (shipClass[gameIsRegisteredIndex].registered != 0)
        count = CountEVPlugins(novaFilesFolder, kAnyPluginType);
    if (err != noErr || count == 0) {
        url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
        if (url != NULL) {
            filesURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, url,
                                                             CFSTR("Nova Files"), true);
            if (filesURL != NULL) {
                if (CFURLGetFSRef(filesURL, &novaFilesFolder))
                    OpenEVPlugins(novaFilesFolder, kAnyPluginType);
                CFRelease(filesURL);
            }
            CFRelease(url);
        }
    } else
        OpenEVPlugins(novaFilesFolder, kAnyPluginType);
    lastIntfResourceID = -1;
}
#else
{
    char name[kPrefStringMax];

    noSavePilotFiles = EVBugResourceFieldTest(12);
    noUpdatePilotData = EVBugResourceFieldTest(13);
    GetIniIndString(name, kPilotDirList, 3);
#ifdef CEFIX /* F07.07 */
    strcpy(pilotsFolder, "");
#else
    strcpy(pilotsFolder, "9:");
#endif
    strcat(pilotsFolder, name);
    strcat(pilotsFolder, ":");
    strcpy(gPrefsDirectory, "@:");
    lastIntfResourceID = -1;
}
#endif

/* prints/logs the Pascal string GetIndString just filled; NOT the same
   address as crashlog_print_pstr (0x00470150) despite the similar role */
/* called with a constant 0 at every site here; tail-calls through a lazy
   singleton getter (0x52ce50) into 0x4fdf10 */
/* CreateDirectoryPath is BurgerLib's CreateDirectoryPath: ExpandAPathToBufferNative
   (0x004d9680) then CreateDirectoryPathNative (0x004d9530); nonzero on failure */



/* OpenDataFiles (mac _OpenDataFiles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bd160
// MAC: _OpenDataFiles i386 0x00076a76 ppc 0x0007f85c
void OpenDataFiles(void)
#ifdef MAC1010
{
    Str255 alertText;
    Str255 name;
    short count;
    short vRefNum;
    int32_t dirID;
    FSSpec spec;
    Str255 path;
    Handle h;

    if (HGetVol(alertText, &vRefNum, &dirID) == noErr) {
        GetIndString(name, kPilotDirList, 3);
        path[1] = ':';
        path[0] = name[0] + 1;
        for (count = 1; count <= name[0]; count++)
            path[count + 1] = name[count];
        if (FSMakeFSSpec(vRefNum, 0, path, &spec) != noErr) {
            DirCreate(0, 0, name, &dirID);
            if (FSMakeFSSpec(vRefNum, 0, path, &spec) != noErr) {
                FadeScreenIn();
                GetIndString(alertText, kAlertStrList, 0x1d);
                EVNoteAlert(alertText);
                ExitToShell();
            }
        }
    }

    crashlog_print("opening files in ");
    crashlog_print_pstr("\pNova Files");
    crashlog_print(" folder:\n");
    if (OpenEVPlugins(gARef, "\pNova Files", kAnyPluginType, crashlog_get_file()) != noErr) {
        DirCreate(0, 0, "\pNova Files", &dirID);
        if (OpenEVPlugins(gARef, "\pNova Files", kAnyPluginType, crashlog_get_file()) != noErr) {
            FadeScreenIn();
            GetIndString(alertText, kAlertStrList, 0x1e);
            EVNoteAlert(alertText);
            ExitToShell();
        }
    }
    if (shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0) {
        if (pluginCount > 0) {
            crashlog_print("opening files in ");
            crashlog_print_pstr("\pNova Plug-Ins");
            crashlog_print(" folder:\n");
        }
        if (OpenEVPlugins(gARef, "\pNova Plug-Ins", kPlugInFileType, crashlog_get_file()) != noErr) {
            DirCreate(0, 0, "\pNova Plug-Ins", &dirID);
            if (OpenEVPlugins(gARef, "\pNova Plug-Ins", kPlugInFileType, crashlog_get_file()) != noErr) {
                FadeScreenIn();
                GetIndString(alertText, kAlertStrList, 0x1f);
                EVNoteAlert(alertText);
                ExitToShell();
            }
        }
    }
    if (crashlog_isactive()) {
        h = GetResource(kStringListType, kGameStrList);
        if (h != NULL) {
            HLock(h);
            BlockMove(*h, &count, sizeof(short));
            HUnlock(h);
            HPurge(h);
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
#elif defined(MACBUILD)
{
    Handle h;
    short count;

    crashlog_print("opening files in ");
    crashlog_print_pstr("\pNova Files");
    crashlog_print(" folder:\n");
    if (shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0) {
        if (pluginCount > 0) {
            crashlog_print("opening files in ");
            crashlog_print_pstr("\pPlug-Ins");
            crashlog_print(" folder:\n");
        }
        OpenEVPlugins(pluginsFolder, kPlugInFileType);
    }
    if (crashlog_isactive()) {
        h = GetResource(kStringListType, kGameStrList);
        if (h != NULL) {
            HLock(h);
            BlockMove(*h, &count, sizeof(short));
            HUnlock(h);
            HPurge(h);
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
#else
{
    char name[kPrefStringMax];
    char path[0x800];
    unsigned char str[kPrefStringMax];
    Handle h;
    short count;

    GetIniIndString(name, kPilotDirList, 3);
#ifdef CEFIX /* F07.08 */
    strcpy(path, "");
#else
    strcpy(path, "9:");
#endif
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
#ifdef CEFIX /* F07.09-10 */
        if (CE_OpenNovaFilesFallback("Nova Files", ".rez", NULL) != 0) {
#else
        CreateDirectoryPath("Nova Files");
        if (OpenEVPlugins("Nova Files", ".rez", NULL) != 0) {
#endif
            FadeScreenIn();
            GetIndString(str, kAlertStrList, 0x1e);
            EVNoteAlert(str);
            exit(0);
        }
    }

    if (shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0) {
        if (pluginCount > 0) {
            crashlog_print("opening files in ");
            crashlog_print("Nova Plug-Ins");
            crashlog_print(" folder:\n");
        }
#ifdef CEFIX /* F07.11 */
        if (CE_OpenPlugInsFallback("Nova Plug-Ins", ".rez", NULL) != 0) {
#else
        if (OpenEVPlugins("Nova Plug-Ins", ".rez", NULL) != 0) {
#endif
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
            count = SwapEndianShort(&ResRec(StringListRec, h)->count);
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
#endif

/* NebuRecord is loaded without LockAHandle or AdjustHandleSize. */

#define SpobHasTradeCenter(s) ((s)->flags & kSpobTradeCenter)

/* the mac keeps a resource unpurgeable while it is read and purgeable again after; the port's
   resources never purge */
#ifdef MACBUILD
#define ResourceNoPurge(h) HNoPurge(h)
#define ResourcePurge(h) HPurge(h)
#else
#define ResourceNoPurge(h)
#define ResourcePurge(h)
#endif
/* 1.1.1 no longer services the intro music between resource loads */
#if V111
#define LoadingIntroMusic()
#else
#define LoadingIntroMusic() HandleIntroMusic(0)
#endif


#if V111 /* V111 spelling */
#define mountSlot k
#define junksLoaded found
#else
#define mountSlot count
#define junksLoaded loaded
#endif

/* LoadObjectData (mac _LoadObjectData). STATE: MATCH 100.0% (6170 exact, 2139 reloc-equivalent,
   0 mismatched; exe 8309 / obj 8309), byte-identical (39297 B, 2139 relocated fields masked).
   OPEN: none. Declaration order and roles carry the exe's spill numbering: the junk counter
   is its own `loaded', declared first; the weap and pers totals stay on `n'. The colour
   extracts are masks, not casts (see notes, lanes RB, RD, RE, CL3).
   Mac 1.0.10: its body is loadobjectdata1010.inc, MATCH 8469 / 8469 on both fragments. */
// FUNCTION: EVNOVA 0x004bd3c0
// MAC: _LoadObjectData i386 0x000771b0 ppc 0x00080100
void LoadObjectData(void)
#ifdef MAC1010
#include "../engine/loadobjectdata1010.inc"
#else
{
    short loaded;
    short n;
    short j;
    short i;
    short prev;
    short found;
    short resID;
    uint32_t resType;
    short count;
    short k;
    short gunTally, turretTally;
    short maxGuns, maxTurrets;
    float base;
    int32_t tradeEst;
#ifdef MACBUILD
    ColorSearchUPP colorSearch;
#endif
#if V111 /* V111 spelling */
    short mountCount[8];
    short mountID[8];
    short mountAmmo[8];
    RGBColor rgb;
#else
    short mountCount[8];
    short mountID[8];
    RGBColor rgb;
    short mountAmmo[8];
#endif
    unsigned char name[256];
    Boolean used[kMaxStellars];

    if (EVBugResourceFieldTest(0))
        return;
    SWSetPortToWindow(gSpriteWorld);

    /* ---------------- spöb: stellar objects ---------------- */
    {

        found = 0;
        count = CountResources(kSpobType);
    if (count == 0)
        crashlog("warning: no " kSpobResName " resources found");
    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].loaded = 0;
        stellarFileHand = GetResource(kSpobType, i + kFirstResourceID);
        if (stellarFileHand != NULL) {
            LockAHandle(stellarFileHand);
            ResourceNoPurge(stellarFileHand);
            found++;
            GetResInfo(stellarFileHand, &resID, &resType, name);
            TrimResourceName(name);
            strncpy((char *)stellarData[i].name, (char *)name, 31);
            stellarData[i].pos.h = ResRec(SpobRes, stellarFileHand)->xPos;
            stellarData[i].pos.v = ResRec(SpobRes, stellarFileHand)->yPos;
            stellarData[i].type = ResRec(SpobRes, stellarFileHand)->type;
            stellarData[i].flags = ResRec(SpobRes, stellarFileHand)->flags;
            stellarData[i].flags2 = ResRec(SpobRes, stellarFileHand)->flags2;
            stellarData[i].govt = ResRec(SpobRes, stellarFileHand)->govt;
            stellarData[i].minStatus = ResRec(SpobRes, stellarFileHand)->minStatus;
            stellarData[i].techLevel = ResRec(SpobRes, stellarFileHand)->techLevel;
            stellarData[i].customPict = ResRec(SpobRes, stellarFileHand)->custPicID;
            stellarData[i].customSound = ResRec(SpobRes, stellarFileHand)->custSndID;
            stellarData[i].landingFee = ResRec(SpobRes, stellarFileHand)->fee;
            stellarData[i].gravity = ResRec(SpobRes, stellarFileHand)->gravity;
            stellarData[i].weapon = ResRec(SpobRes, stellarFileHand)->weapon;
            if (stellarData[i].govt >= kFirstResourceID)
                stellarData[i].govt -= kFirstResourceID;
            else
                stellarData[i].govt = -1;
            stellarData[i].strength = ResRec(SpobRes, stellarFileHand)->strength;
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadType = ResRec(SpobRes, stellarFileHand)->deadType;
            stellarData[i].deadDays = ResRec(SpobRes, stellarFileHand)->deadTime;
            stellarData[i].deadDaysLeft = 0;
            stellarData[i].explodType = ResRec(SpobRes, stellarFileHand)->explodType;
            if (stellarData[i].explodType < 0)
                stellarData[i].explodType = -1;
            if (stellarData[i].deadType < 0 || stellarData[i].deadType >= kMasterStellarSprites)
                stellarData[i].deadType = stellarData[i].type;
            strcpy(stellarData[i].onDestroy, ResRec(SpobRes, stellarFileHand)->onDestroy);
            strcpy(stellarData[i].onRegen, ResRec(SpobRes, stellarFileHand)->onRegen);
            for (j = 0; j < 3; j++)
                stellarData[i].specialTech[j] = ResRec(SpobRes, stellarFileHand)->specialTech[j];
            for (j = 0; j < 5; j++)
                stellarData[i].specialTech[j + 3] = ResRec(SpobRes, stellarFileHand)->specialTech2[j];
            stellarData[i].loaded = 1;
            stellarData[i].prepared = 1;
            stellarData[i].dominated = 0;
            stellarData[i].tribute = ResRec(SpobRes, stellarFileHand)->tribute;
            if (stellarData[i].tribute <= 0)
                stellarData[i].tribute = stellarData[i].techLevel * 1000;
            stellarData[i].defCount = ResRec(SpobRes, stellarFileHand)->defCount;
            stellarData[i].defenseDude = ResRec(SpobRes, stellarFileHand)->defenseDude;
            stellarData[i].animDelay = ResRec(SpobRes, stellarFileHand)->animDelay;
            stellarData[i].frame0Bias = ResRec(SpobRes, stellarFileHand)->frame0Bias;
            for (j = 0; j < kSpobHyperLinks; j++) {
                stellarData[i].hyperLinks[j] = ResRec(SpobRes, stellarFileHand)->hyperLink[j];
                if (stellarData[i].hyperLinks[j] >= kFirstResourceID)
                    stellarData[i].hyperLinks[j] -= kFirstResourceID;
                else
                    stellarData[i].hyperLinks[j] = -1;
            }
            if (stellarData[i].defCount > 1000) {
                if (stellarData[i].defCount > 10000)
                    stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 1000;
                else
                    stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 100;
            } else
                stellarData[i].defWaveSize = stellarData[i].defCount;
            if (stellarData[i].defenseDude >= kFirstResourceID)
                stellarData[i].defenseDude -= kFirstResourceID;
            else
                stellarData[i].defenseDude = -1;
            if (stellarData[i].techLevel < 0)
                stellarData[i].techLevel = 0;
            strcpy(stellarData[i].onDominate, ResRec(SpobRes, stellarFileHand)->onDominate);
            strcpy(stellarData[i].onRelease, ResRec(SpobRes, stellarFileHand)->onRelease);
            if (stellarData[i].weapon >= kFirstResourceID)
                stellarData[i].weapon -= kFirstResourceID;
            else
                stellarData[i].weapon = -1;
            AdjustHandleSize(stellarFileHand, 0x45e);
            UnlockAHandle(stellarFileHand);
            ResourcePurge(stellarFileHand);
            ReleaseResource(stellarFileHand);
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
            LoadingIntroMusic();
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
            crashlog("warning: no " kSystResName " resources found");
        for (i = 0; i < kMaxSystems; i++) {
            systemData[i].loaded = 0;
            systemData[i].prepared = 0;
            systemData[i].govt = -32767;
            for (j = 0; j < kMaxSystemLinks; j++)
                systemData[i].links[j] = -1;
            systemFileHand = GetResource(kSystType, i + kFirstResourceID);
            if (systemFileHand != NULL) {
                LockAHandle(systemFileHand);
                ResourceNoPurge(systemFileHand);
                found++;
                GetResInfo(systemFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)systemData[i].name, (char *)name, 31);
                systemData[i].loaded = 1;
                systemData[i].prepared = 1;
                for (j = 0; j < 8; j++) {
                    systemData[i].dudeTypes[j] = ResRec(SystRes, systemFileHand)->dudeTypes[j];
                    systemData[i].dudeProbs[j] = ResRec(SystRes, systemFileHand)->probs[j];
                }
                systemData[i].avgShips = ResRec(SystRes, systemFileHand)->avgShips;
                systemData[i].govt = ResRec(SystRes, systemFileHand)->govt;
                systemData[i].message = ResRec(SystRes, systemFileHand)->message;
                systemData[i].asteroidCount = ResRec(SystRes, systemFileHand)->asteroids;
                systemData[i].interference = ResRec(SystRes, systemFileHand)->interference;
                if (systemData[i].asteroidCount > 0) {
                    systemData[i].asteroidCount *=
                        (GetCWindowPortRect(gWind)->right -
                                 GetCWindowPortRect(gWind)->left) / 640.0;
                    if (systemData[i].asteroidCount > kMaxAsteroids)
                        systemData[i].asteroidCount = kMaxAsteroids;
                } else {
                    systemData[i].asteroidCount = 0;
                }
                systemData[i].astTypes = ResRec(SystRes, systemFileHand)->astTypes;
                systemData[i].pos.h = ResRec(SystRes, systemFileHand)->xPos;
                systemData[i].pos.v = ResRec(SystRes, systemFileHand)->yPos;
                if (systemData[i].govt >= kFirstResourceID && systemData[i].govt < kFirstResourceID + kMaxGovts)
                    systemData[i].govt -= kFirstResourceID;
                else
                    systemData[i].govt = -1;
                systemData[i].rgb.red = ((ResRec(SystRes, systemFileHand)->bkgndColor >> 16) & 0xff) * 256;
                systemData[i].rgb.green = ((ResRec(SystRes, systemFileHand)->bkgndColor >> 8) & 0xff) * 256;
                systemData[i].rgb.blue = (ResRec(SystRes, systemFileHand)->bkgndColor & 0xff) * 256;
                if (gSpriteWorld->pixelDepth == 8) {
                    systemData[i].mapPixel = Color2Index(&systemData[i].rgb);
                } else {
                    systemData[i].rgb.red &= 0xfc00;
                    systemData[i].rgb.green &= 0xfc00;
                    systemData[i].rgb.blue &= 0xfc00;
                    systemData[i].mapPixel = ((((systemData[i].rgb.red + 0x801) >> 11) << 10) & 0x7c00) |
                                    ((((systemData[i].rgb.green + 0x801) >> 11) << 5) & 0x3e0) |
                                    (((systemData[i].rgb.blue + 0x801) >> 11) & 0x1f);
                }
                systemData[i].murk = ResRec(SystRes, systemFileHand)->murk;
                if (systemData[i].murk > 100)
                    systemData[i].murk = 100;
                else if (systemData[i].murk < 8 && systemData[i].murk > 0)
                    systemData[i].murk = 0;
                else if (systemData[i].murk < 0)
                    systemData[i].murk = -1;
                total = 0;                       /* valid dude prob total */
                systemData[i].fleetCount = 0;
                systemData[i].fleetProbTotal = 0;
                for (j = 0; j < 8; j++) {
                    short dude = systemData[i].dudeTypes[j];
                    if (dude >= kFirstResourceID && dude < kFirstResourceID + kMaxDudes) {
                        systemData[i].dudeTypes[j] -= kFirstResourceID;
                        total += systemData[i].dudeProbs[j];
                    } else {
                        if (dude <= -kFirstResourceID && dude > -(kFirstResourceID + kMaxFleets)) {
                            /* negative dude entries reference fleets */
                            systemData[i].fleets[systemData[i].fleetCount] = abs(dude) - kFirstResourceID;
                            systemData[i].fleetProbs[systemData[i].fleetCount] = systemData[i].dudeProbs[j];
                            systemData[i].fleetCount++;
                            systemData[i].fleetProbTotal += systemData[i].dudeProbs[j];
                        }
                        systemData[i].dudeTypes[j] = -1;
                        systemData[i].dudeProbs[j] = 0;
                    }
                }
                if (total != 100 && total > 0) {
                    for (j = 0; j < 8; j++)
                        systemData[i].dudeProbs[j] *= 100.0 / (float)total;
                }
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = total = ResRec(SystRes, systemFileHand)->con[j];
                    if (link >= kFirstResourceID && link < kFirstResourceID + kMaxSystems)
                        systemData[i].links[j] = link - kFirstResourceID;
                    else
                        systemData[i].links[j] = -1;
                }
                for (j = 0; j < kNavSlots; j++) {
                    target = ResRec(SystRes, systemFileHand)->nav[j];
                    if (target >= kFirstResourceID && target < kFirstResourceID + kMaxStellars) {
                        systemData[i].navDefault[j] = target - kFirstResourceID;
                        if (!stellarData[systemData[i].navDefault[j]].prepared) {
                            crashlog_print("warning: ignoring reference to nonexistent stellar ID ");
                            crashlog_print_num(systemData[i].navDefault[j] + kFirstResourceID);
                            crashlog_print(" in system ");
                            crashlog_print_pstr(systemData[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(")\n");
                            systemData[i].navDefault[j] = -1;
                        }
                    } else {
                        systemData[i].navDefault[j] = -1;
                    }
                }
                for (j = 0; j < 8; j++) {
                    systemData[i].persons[j] = ResRec(SystRes, systemFileHand)->person[j];
                    systemData[i].persProbs[j] = ResRec(SystRes, systemFileHand)->personProb[j];
                    if (systemData[i].persons[j] >= kFirstResourceID && systemData[i].persons[j] < kFirstResourceID + kMaxPersons - 1)
                        systemData[i].persons[j] -= kFirstResourceID;
                    else
                        systemData[i].persons[j] = -1;
                    if (systemData[i].persProbs[j] < 0)
                        systemData[i].persProbs[j] = 0;
                    if (systemData[i].persProbs[j] > 100)
                        systemData[i].persProbs[j] = 100;
                }
                systemData[i].reinforceFleet = ResRec(SystRes, systemFileHand)->reinfFleet;
                systemData[i].reinforceTime = ResRec(SystRes, systemFileHand)->reinfTime;
                systemData[i].reinforceInterval = ResRec(SystRes, systemFileHand)->reinfInterval;
                if (systemData[i].reinforceFleet >= kFirstResourceID) {
                    systemData[i].reinforceFleet -= kFirstResourceID;
                    if (systemData[i].reinforceInterval < 1)
                        systemData[i].reinforceInterval = 1;
                } else {
                    systemData[i].reinforceFleet = -1;
                }
                systemData[i].reinforceTimer = 0;
                systemData[i].reinforceDaysLeft = 0;
                strcpy(systemData[i].visibility, ResRec(SystRes, systemFileHand)->visibility);
                AdjustHandleSize(systemFileHand, 0x1ac);
                UnlockAHandle(systemFileHand);
                ResourcePurge(systemFileHand);
                ReleaseResource(systemFileHand);
                LoadingIntroMusic();
            } else {
                strcpy(systemData[i].visibility, "");
            }
            if (found >= count)
                break;
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* co-location chains: systems sharing a map position form linked groups */
        for (i = 0; i < kMaxSystems; i++) {
            systemData[i].colocGroup = -1;
            systemData[i].colocNext = -1;
        }
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded && systemData[i].colocGroup == -1) {
                prev = i;
                systemData[i].colocGroup = i;
                for (j = 0; j < kMaxSystems; j++) {
                    if (systemData[j].loaded &&
                        systemData[j].pos.h == systemData[i].pos.h &&
                        systemData[j].pos.v == systemData[i].pos.v &&
                        (systemData[j].colocGroup == -1 || j > i)) {
                        systemData[prev].colocNext = j;
                        systemData[j].colocGroup = i;
                        prev = j;
                    }
                }
            }
        }

        /* drop links to nonexistent systems */
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = systemData[i].links[j];
                    if (systemData[i].links[j] != -1 && !systemData[systemData[i].links[j]].prepared) {
                        systemData[i].links[j] = -1;
                        crashlog_print("warning: ignoring link from ");
                        crashlog_print_pstr(systemData[i].name);
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
            if (systemData[i].prepared) {
                for (n = 0; n < kMaxSystemLinks; n++) {
                    link = systemData[i].links[n];
                    if (link != -1) {
                        systemData[i].links[n] = FindFirstCoLocatedSystem(link);
                        if (systemData[i].links[n] == FindFirstCoLocatedSystem(i))
                            systemData[i].links[n] = -1;
                    }
                }
                for (j = 0; j < kMaxSystemLinks; j++) {
                    if (systemData[i].links[j] != -1) {
                        total = 0;
                        for (n = 0; n <= j; n++)
                            if (systemData[i].links[n] == systemData[i].links[j])
                                total++;
                        if (total > 1)
                            systemData[i].links[j] = -1;
                    }
                }
            }
        }

        /* assume implicit reciprocal links */
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].prepared) {
                for (j = 0; j < kMaxSystemLinks; j++) {
                    link = systemData[i].links[j];
                    if (link != -1) {
                        link = FindFirstCoLocatedSystem(link);
                        if (link >= 0 && link < kMaxSystems && link != i) {
                            Boolean haveBack = 0;
                            for (n = 0; n < kMaxSystemLinks; n++) {
                                if (systemData[link].links[n] != -1 &&
                                    FindFirstCoLocatedSystem(systemData[link].links[n]) ==
                                        FindFirstCoLocatedSystem(i)) {
                                    haveBack = 1;
                                    break;
                                }
                            }
                            if (!haveBack) {
                                crashlog_print("warning: assuming implicit reciprocal link from ");
                                crashlog_print_pstr(systemData[i].name);
                                crashlog_print(" (");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(") to ");
                                crashlog_print_pstr(systemData[link].name);
                                crashlog_print(" (");
                                crashlog_print_num(link + kFirstResourceID);
                                crashlog_print(")\n");
                                for (n = 0; n < kMaxSystemLinks; n++) {
                                    if (systemData[link].links[n] == -1) {
                                        systemData[link].links[n] = FindFirstCoLocatedSystem(i);
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
                    if (systemData[i].navDefault[j] >= 0 && systemData[i].navDefault[j] < kMaxStellars)
                        used[systemData[i].navDefault[j]] = 1;
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
            outfitSystemFileHand = GetResource(kOutfType, i + kFirstResourceID);
            if (outfitSystemFileHand != NULL) {
                found++;
                LockAHandle(outfitSystemFileHand);
                ResourceNoPurge(outfitSystemFileHand);
                outfitSystem[i].techLevel = ResRec(OutfRes, outfitSystemFileHand)->techLevel;
                outfitSystem[i].cost = ResRec(OutfRes, outfitSystemFileHand)->cost;
                outfitSystem[i].mass = ResRec(OutfRes, outfitSystemFileHand)->mass;
                outfitSystem[i].modType[0] = ResRec(OutfRes, outfitSystemFileHand)->modType;
                outfitSystem[i].modVal[0] = ResRec(OutfRes, outfitSystemFileHand)->modVal;
                outfitSystem[i].maxCount = ResRec(OutfRes, outfitSystemFileHand)->max;
                outfitSystem[i].itemClass = ResRec(OutfRes, outfitSystemFileHand)->itemClass;
                outfitSystem[i].dispWeight = ResRec(OutfRes, outfitSystemFileHand)->dispWeight;
                outfitSystem[i].scanMask = ResRec(OutfRes, outfitSystemFileHand)->scanMask;
                outfitSystem[i].buyRandom = ResRec(OutfRes, outfitSystemFileHand)->buyRandom;
                if (outfitSystem[i].buyRandom < 0)
                    outfitSystem[i].buyRandom = 0;
                if (outfitSystem[i].buyRandom > 100)
                    outfitSystem[i].buyRandom = 100;
                outfitSystem[i].govtRestrict = ResRec(OutfRes, outfitSystemFileHand)->require3;
                if ((outfitSystem[i].govtRestrict < ResClassBase(0)
                        || outfitSystem[i].govtRestrict >= ResClassBase(0) + kMaxGovts) &&
                    (outfitSystem[i].govtRestrict < ResClassBase(1)
                        || outfitSystem[i].govtRestrict >= ResClassBase(1) + kMaxGovts) &&
                    (outfitSystem[i].govtRestrict < ResClassBase(2)
                        || outfitSystem[i].govtRestrict >= ResClassBase(2) + kMaxGovts) &&
                    (outfitSystem[i].govtRestrict < ResClassBase(3)
                        || outfitSystem[i].govtRestrict >= ResClassBase(3) + kMaxGovts))
                    outfitSystem[i].govtRestrict = -1;
                strcpy(outfitSystem[i].availability, ResRec(OutfRes, outfitSystemFileHand)->availability);
                strcpy(outfitSystem[i].onPurchase, ResRec(OutfRes, outfitSystemFileHand)->onPurchase);
                strcpy(outfitSystem[i].onSell, ResRec(OutfRes, outfitSystemFileHand)->onSell);
                AdjustHandleSize(outfitSystemFileHand, 0x404);
                outfitSystem[i].persistent = 0;
                outfitSystem[i].flags = ResRec(OutfRes, outfitSystemFileHand)->flags;
                if (OutfPersistent(&outfitSystem[i]))
                    outfitSystem[i].persistent = 1;
                GetResInfo(outfitSystemFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)outfitSystem[i].name, (char *)name, 63);
                outfitSystem[i].modType[1] = ResRec(OutfRes, outfitSystemFileHand)->modType2;
                outfitSystem[i].modVal[1] = ResRec(OutfRes, outfitSystemFileHand)->modVal2;
                outfitSystem[i].modType[2] = ResRec(OutfRes, outfitSystemFileHand)->modType3;
                outfitSystem[i].modVal[2] = ResRec(OutfRes, outfitSystemFileHand)->modVal3;
                outfitSystem[i].modType[3] = ResRec(OutfRes, outfitSystemFileHand)->modType4;
                outfitSystem[i].modVal[3] = ResRec(OutfRes, outfitSystemFileHand)->modVal4;
                for (j = 0; j < kOutfMods; j++) {
                    if ((outfitSystem[i].modType[j] == kOutfModWeapon || outfitSystem[i].modType[j] == kOutfModAmmo ||
                         outfitSystem[i].modType[j] == kModTypeFakeID) &&outfitSystem[i].modVal[j] >= kFirstResourceID)
                        outfitSystem[i].modVal[j] -= kFirstResourceID;
                }
                outfitSystem[i].contributes[0] = ResRec(OutfRes, outfitSystemFileHand)->contribute1;
                outfitSystem[i].contributes[1] = ResRec(OutfRes, outfitSystemFileHand)->contribute2;
                outfitSystem[i].requires[0] = ResRec(OutfRes, outfitSystemFileHand)->require1;
                outfitSystem[i].requires[1] = ResRec(OutfRes, outfitSystemFileHand)->require2;
                strcpy((char *)outfitShortNames[i], ResRec(OutfRes, outfitSystemFileHand)->shortName);
                strcpy((char *)lcOutfitName[i], ResRec(OutfRes, outfitSystemFileHand)->lcName);
                strcpy((char *)lcPluralOutfitName[i], ResRec(OutfRes, outfitSystemFileHand)->lcPlural);
                CtoPstr((char *)outfitShortNames[i]);
                CtoPstr((char *)lcOutfitName[i]);
                CtoPstr((char *)lcPluralOutfitName[i]);
                UnlockAHandle(outfitSystemFileHand);
                ResourcePurge(outfitSystemFileHand);
                ReleaseResource(outfitSystemFileHand);
                if (outfitSystem[i].buyRandom > 0 && crashlog_isactive()) {
                    if (!ResourceExists(kPictType, i + 6000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(outfitSystem[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks outfit picture\n");
                    }
                    if (!ResourceExists(kDescType, i + 3000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(outfitSystem[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks description\n");
                    }
                }
                LoadingIntroMusic();
            } else {
                strcpy(outfitSystem[i].availability, "");
                strcpy(outfitSystem[i].onPurchase, "");
                strcpy(outfitSystem[i].onSell, "");
                outfitSystem[i].techLevel = kOutfTechAbsent;
                outfitSystem[i].mass = 0;
                for (j = 0; j < kOutfMods; j++) {
                    outfitSystem[i].modType[j] = 0;
                    outfitSystem[i].modVal[j] = 0;
                }
            }
            if (found >= count)
                break;
        }

        /* alias outfits that share a lowercase display name */
        for (i = 0; i < kMaxOutfits; i++) {
            outfitSystem[i].sameNameIndex = i;
            if (outfitSystem[i].techLevel < kOutfTechAbsent) {
                /* no `i > 0` guard in the original: the for-entry test covers it
                 * (the exe has a single i-compare here, ours had two) */
                if (lcOutfitName[i][0] != 0) {
                    for (j = 0; j < i; j++) {
                        if (outfitSystem[j].techLevel < kOutfTechAbsent &&
                            lcOutfitName[j][0] != 0 && lcOutfitName[i][0] == lcOutfitName[j][0]) {
                            n = 0;
                            while (n < lcOutfitName[i][0] &&
                                   lcOutfitName[i][1 + n] == lcOutfitName[j][1 + n])
                                n++;
                            if (n == lcOutfitName[i][0]) {
                                outfitSystem[i].sameNameIndex = j;
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
        float factor;
        short g;

        found = 0;
        n = count = CountResources(kWeapType);
        for (i = 0; i < kMaxWeapTypes; i++)
            weaponStats[i].guidance = kResAbsent;
#if defined(MACBUILD) && V111
        colorSearch = NewColorSearchUPP(ColorSearchProc);
        AddSearch(colorSearch);
#endif
        for (i = 0; i < kMaxWeapTypes; i++) {
            weaponFileHand = GetResource(kWeapType, i + kFirstResourceID);
            if (weaponFileHand != NULL) {
                found++;
                LockAHandle(weaponFileHand);
                ResourceNoPurge(weaponFileHand);
                weaponStats[i].reload = ResRec(WeapRes, weaponFileHand)->reload;
                weaponStats[i].lifetime = ResRec(WeapRes, weaponFileHand)->count;
                weaponStats[i].damage[0] = ResRec(WeapRes, weaponFileHand)->massDmg[0];
                weaponStats[i].damage[1] = ResRec(WeapRes, weaponFileHand)->massDmg[1];
                weaponStats[i].guidance = ResRec(WeapRes, weaponFileHand)->guidance;
                weaponStats[i].speed100 = ResRec(WeapRes, weaponFileHand)->speed / 100.0;
                weaponStats[i].ammoType = ResRec(WeapRes, weaponFileHand)->ammoType;
                weaponStats[i].graphic = ResRec(WeapRes, weaponFileHand)->graphic;
                weaponStats[i].inaccuracy = ResRec(WeapRes, weaponFileHand)->inaccuracy;
                weaponStats[i].sound = ResRec(WeapRes, weaponFileHand)->sound;
                weaponStats[i].impact = ResRec(WeapRes, weaponFileHand)->impact;
                weaponStats[i].recoil = ResRec(WeapRes, weaponFileHand)->recoil;
                weaponStats[i].explodType = ResRec(WeapRes, weaponFileHand)->explodType;
                weaponStats[i].proxRadius = ResRec(WeapRes, weaponFileHand)->proxRadius;
                weaponStats[i].blastRadius = ResRec(WeapRes, weaponFileHand)->blastRadius;
                weaponStats[i].flags = ResRec(WeapRes, weaponFileHand)->flags;
                weaponStats[i].flags2 = ResRec(WeapRes, weaponFileHand)->flags2;
                weaponStats[i].flags3 = ResRec(WeapRes, weaponFileHand)->flags3;
                weaponStats[i].durability = ResRec(WeapRes, weaponFileHand)->durability;
                weaponStats[i].maxAmmo = ResRec(WeapRes, weaponFileHand)->maxAmmo;
                weaponStats[i].seeker = ResRec(WeapRes, weaponFileHand)->seeker;
                weaponStats[i].ionization = ResRec(WeapRes, weaponFileHand)->ionization;
                weaponStats[i].exitType = ResRec(WeapRes, weaponFileHand)->exitType;
                weaponStats[i].burstCount = ResRec(WeapRes, weaponFileHand)->burstCount;
                weaponStats[i].burstReload = ResRec(WeapRes, weaponFileHand)->burstReload;
                weaponStats[i].guidedTurn = ResRec(WeapRes, weaponFileHand)->guidedTurn * 0.1;
                if (weaponStats[i].inaccuracy == -1)
                    weaponStats[i].inaccuracy = 0;
                for (j = 0; j < 4; j++) {
                    weaponStats[i].jamVuln[j] = ResRec(WeapRes, weaponFileHand)->jamVuln[j];
                    if (weaponStats[i].jamVuln[j] < 0)
                        weaponStats[i].jamVuln[j] = 0;
                    if (weaponStats[i].jamVuln[j] > 100)
                        weaponStats[i].jamVuln[j] = 100;
                }
                if (ResRec(WeapRes, weaponFileHand)->liColor == 0) {
                    rgb.red = rgb.green = 0x200;
                    rgb.blue = 0xffff;
                } else {
                    rgb.red = ((ResRec(WeapRes, weaponFileHand)->liColor >> 16) & 0xff) * 256;
                    rgb.green = ((ResRec(WeapRes, weaponFileHand)->liColor >> 8) & 0xff) * 256;
                    rgb.blue = (ResRec(WeapRes, weaponFileHand)->liColor & 0xff) * 256;
                }
                ClampRGB(rgb);
                weaponStats[i].ionizeColor = Color2Index(&rgb);
                if (weaponStats[i].burstCount <= 0)
                    weaponStats[i].burstCount = -1;
                weaponStats[i].beamLength = ResRec(WeapRes, weaponFileHand)->beamLength;
                weaponStats[i].beamWidth = ResRec(WeapRes, weaponFileHand)->beamWidth;
                weaponStats[i].coronaFalloff = ResRec(WeapRes, weaponFileHand)->falloff;
                weaponStats[i].beamColor = ResRec(WeapRes, weaponFileHand)->beamColor;
                weaponStats[i].coronaColor = ResRec(WeapRes, weaponFileHand)->coronaColor;
                if (weaponStats[i].beamLength > 0 && weaponStats[i].coronaFalloff <= 0)
                    weaponStats[i].coronaFalloff = 16;
                weaponStats[i].lightningDensity = ResRec(WeapRes, weaponFileHand)->liCohesive;
                weaponStats[i].lightningAmplitude = ResRec(WeapRes, weaponFileHand)->liAmplitude;
                if (weaponStats[i].lightningDensity > 0) {
                    if (weaponStats[i].lightningDensity < 2)
                        weaponStats[i].lightningDensity = 2;
                    weaponStats[i].coronaFalloff = 0;
                    if (weaponStats[i].beamWidth < 1)
                        weaponStats[i].beamWidth = 1;
                    if (weaponStats[i].lightningAmplitude < 1)
                        weaponStats[i].lightningAmplitude = 1;
                }
                weaponStats[i].subCount = ResRec(WeapRes, weaponFileHand)->subCount;
                weaponStats[i].subType = ResRec(WeapRes, weaponFileHand)->subType;
                weaponStats[i].subTheta = ResRec(WeapRes, weaponFileHand)->subTheta;
                weaponStats[i].subLimit = ResRec(WeapRes, weaponFileHand)->subLimit;
                if (weaponStats[i].subTheta == -1)
                    weaponStats[i].subTheta = 0;
                if (weaponStats[i].subType >= kFirstResourceID && weaponStats[i].subType < kFirstResourceID + kMaxWeapTypes)
                    weaponStats[i].subType -= kFirstResourceID;
                else
                    weaponStats[i].subType = -1;
                if (weaponStats[i].guidance != kGuidanceBeam && weaponStats[i].guidance != kGuidanceTurretBeam
                    && weaponStats[i].guidance != kGuidancePDBeam)
                    weaponStats[i].decayShots = weaponStats[i].coronaFalloff;
                else
                    weaponStats[i].decayShots = 0;
                weaponStats[i].proxSafety = ResRec(WeapRes, weaponFileHand)->proxSafety;
                if (weaponStats[i].subCount <= 0 || weaponStats[i].subType < 0) {
                    weaponStats[i].subCount = 0;
                    weaponStats[i].subType = -1;
                }
                if (weaponStats[i].guidance == kGuidanceBeam || weaponStats[i].guidance == kGuidanceTurretBeam
                    || weaponStats[i].guidance == kGuidancePDBeam) {
                    weaponStats[i].rgbScratch.red = ((weaponStats[i].beamColor >> 16) & 0xff) * 256;
                    weaponStats[i].rgbScratch.green = ((weaponStats[i].beamColor >> 8) & 0xff) * 256;
                    weaponStats[i].rgbScratch.blue = (weaponStats[i].beamColor & 0xff) * 256;
                    if (gSpriteWorld->pixelDepth == 16) {
                        weaponStats[i].beamColor = (weaponStats[i].beamColor >> 9) & 0x7c00 |
                                          (weaponStats[i].beamColor >> 6) & 0x3e0 |
                                          (weaponStats[i].beamColor >> 3) & 0x1f;
                        weaponStats[i].coronaColor = (weaponStats[i].coronaColor >> 9) & 0x7c00 |
                                            (weaponStats[i].coronaColor >> 6) & 0x3e0 |
                                            (weaponStats[i].coronaColor >> 3) & 0x1f;
                    } else {
                        ClampRGB(weaponStats[i].rgbScratch);
                        weaponStats[i].beamColor = Color2Index(&weaponStats[i].rgbScratch);
                        rgb.red = ((weaponStats[i].coronaColor >> 16) & 0xff) << 7;
                        rgb.green = ((weaponStats[i].coronaColor >> 8) & 0xff) << 7;
                        rgb.blue = (weaponStats[i].coronaColor & 0xff) << 7;
                        ClampRGB(rgb);
                        weaponStats[i].coronaColor = Color2Index(&rgb);
                    }
                }
                weaponStats[i].hitParticles = ResRec(WeapRes, weaponFileHand)->hitParticles;
                rgb.red = ((ResRec(WeapRes, weaponFileHand)->hitPartColor >> 16) & 0xff) * 256;
                rgb.green = ((ResRec(WeapRes, weaponFileHand)->hitPartColor >> 8) & 0xff) * 256;
                rgb.blue = (ResRec(WeapRes, weaponFileHand)->hitPartColor & 0xff) * 256;
                ClampRGB(rgb);
                weaponStats[i].hitPartColor = Color2Index(&rgb);
                weaponStats[i].hitPartLife = ResRec(WeapRes, weaponFileHand)->hitPartLife;
                weaponStats[i].hitPartVel = ResRec(WeapRes, weaponFileHand)->hitPartVel * 0.01;
#if V111
                if (useLegacyRenderingPipeline) {
                    if (gSpriteWorld->pixelDepth == 8)
                        weaponStats[i].hitPartLife /= 2;
                    else
                        weaponStats[i].hitPartLife /= 2; /* POTENTIAL BUG (win V111): hitPartLife halved at all depths, 1.0.10 only at 8-bit */
                }
#else
                if (gSpriteWorld->pixelDepth == 8)
                    weaponStats[i].hitPartLife /= 2;
#endif
                GetResInfo(weaponFileHand, &resID, &resType, name);
                TrimResourceName(name);
                pstrcpy(weapName[i], name);
                weaponStats[i].smokeSet = ResRec(WeapRes, weaponFileHand)->smokeSet;
                weaponStats[i].decay = ResRec(WeapRes, weaponFileHand)->decay;
                weaponStats[i].particles = ResRec(WeapRes, weaponFileHand)->particles;
                weaponStats[i].partLifeMin = ResRec(WeapRes, weaponFileHand)->partLifeMin;
                weaponStats[i].partLifeMax = ResRec(WeapRes, weaponFileHand)->partLifeMax;
                base = ResRec(WeapRes, weaponFileHand)->partVel * 0.01;
                rgb.red = ((ResRec(WeapRes, weaponFileHand)->partColor >> 16) & 0xff) * 256;
                rgb.green = ((ResRec(WeapRes, weaponFileHand)->partColor >> 8) & 0xff) * 256;
                rgb.blue = (ResRec(WeapRes, weaponFileHand)->partColor & 0xff) * 256;
                for (j = 0; j < 8; j++) {
                    factor = (Rand(0x51) + 60) * 0.01;
                    shade.red = rgb.red * factor;
                    shade.green = rgb.green * factor;
                    shade.blue = rgb.blue * factor;
                    weaponStats[i].partColor[j] = Color2Index(&shade);
                    weaponStats[i].partVel[j] = base * factor;
                }
                if (weaponStats[i].smokeSet >= 8)
                    weaponStats[i].smokeSet = 0;
                if (weaponStats[i].guidance == 2) {
                    weaponStats[i].guidance = kGuidanceHoming;
                    weaponStats[i].seeker = 0x30fe;
                }
                UnlockAHandle(weaponFileHand);
                AdjustHandleSize(weaponFileHand, 0x86);
                ResourcePurge(weaponFileHand);
                ReleaseResource(weaponFileHand);
                LoadingIntroMusic();
            }
            if (found >= n)
                break;
        }
#if defined(MACBUILD) && V111
        DelSearch(colorSearch);
#endif
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* ammo-source sanity warnings */
        if (crashlog_isactive()) {
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (weaponStats[i].guidance != kResAbsent) {
                    short ammo = weaponStats[i].ammoType;
                    if (ammo >= 0 && ammo < kMaxWeapTypes &&
                        weaponStats[i].guidance != kGuidanceBay &&
                        (ammo != weaponStats[ammo].ammoType ||
                         weaponStats[ammo].guidance == kGuidanceBay)) {
                        crashlog_print("warning: weapon ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" probably shouldn't draw ammo from weapon ");
                        crashlog_print_num(ammo + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
            for (i = 0; i < kMaxOutfits; i++) {
                if (outfitSystem[i].techLevel < kOutfTechAbsent) {
                    for (count = 0; count < kOutfMods; count++) {
                        if (outfitSystem[i].modType[count] == kOutfModAmmo) {
                            j = outfitSystem[i].modVal[count];
                            g = weaponStats[j].guidance;
                            if (g != kResAbsent) {
                                if (g != kGuidanceBay && j != weaponStats[j].ammoType) {
                                    crashlog_print("warning: outfit item ");
                                    crashlog_print_num(i + kFirstResourceID);
                                    crashlog_print(" grants ammo to weapon ");
                                    crashlog_print_num(j + kFirstResourceID);
                                    if (weaponStats[j].ammoType >= 0 &&
                                        weaponStats[j].ammoType < kMaxWeapTypes)
                                        crashlog_print(", which doesn't use its own ammo\n");
                                    else
                                        crashlog_print(", which doesn't use ammo\n");
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
            weaponStats[i].totalRange = 0;
            g = weaponStats[i].guidance;
            if (weaponStats[i].guidance == kGuidanceUnguided || weaponStats[i].guidance == kGuidanceHoming || weaponStats[i].guidance == kGuidanceTurret || weaponStats[i].guidance == kGuidanceRocket || weaponStats[i].guidance == kGuidanceFrontQuad || weaponStats[i].guidance == kGuidanceRearQuad || weaponStats[i].guidance == kGuidancePointDef) {
                n = i;
                do {
                    weaponStats[i].totalRange += (double)(weaponStats[n].speed100 * weaponStats[n].lifetime);
                    if (weaponStats[n].subCount > 0) {
                        short nxt = weaponStats[n].subType;
                        if (n == nxt) {
                            short lim = weaponStats[n].subLimit;
                            if (lim > 0)
#if V111 /* V111 spelling */
                                weaponStats[i].totalRange += (double)(lim * weaponStats[n].speed100 *
                                                                      weaponStats[n].lifetime);
#else
                                weaponStats[i].totalRange += (double)(weaponStats[n].lifetime *
                                                    (lim * weaponStats[n].speed100));
#endif
                            n = -1;
                        } else {
                            n = nxt;
                        }
                    } else {
                        n = -1;
                    }
                } while (n != -1);
            } else if (g == kGuidanceBeam || g == kGuidanceTurretBeam || g == kGuidancePDBeam) {
                weaponStats[i].totalRange = weaponStats[i].beamLength;
            }
        }
    }

    /* ---------------- shïp: ship classes ---------------- */
    {
        short shipCount;
        Handle h;
        char *res;
        int32_t mass;
        short j;
        short weapID;

        shipCount = CountResources(kShipType);
        if (shipCount == 0)
            crashlog("warning: no " kShipResName " resources found");
        for (i = 0; i < kMaxShipTypes;i++) {
            shipClassFileHand = GetResource(kShipType, i + kFirstResourceID);
            if (shipClassFileHand != NULL) {
                LockAHandle(shipClassFileHand);
                ResourceNoPurge(shipClassFileHand);
                shipClass[i].cargo = ResRec(ShipRes, shipClassFileHand)->cargo;
                shipClass[i].shield = ResRec(ShipRes, shipClassFileHand)->shield;
                shipClass[i].accel = ResRec(ShipRes, shipClassFileHand)->accel / 10000.0;
                shipClass[i].maxSpeed = ResRec(ShipRes, shipClassFileHand)->speed / 100.0;
                shipClass[i].turnRate = ResRec(ShipRes, shipClassFileHand)->turn * 0.1;
                shipClass[i].fuel = ResRec(ShipRes, shipClassFileHand)->energy;
                shipClass[i].freeMass = ResRec(ShipRes, shipClassFileHand)->freeSpace;
                shipClass[i].armor = ResRec(ShipRes, shipClassFileHand)->armor;
                shipClass[i].maxGuns = ResRec(ShipRes, shipClassFileHand)->maxGun;
                shipClass[i].maxTurrets = ResRec(ShipRes, shipClassFileHand)->maxTur;
                shipClass[i].techLevel = ResRec(ShipRes, shipClassFileHand)->techLevel;
                shipClass[i].cost = ResRec(ShipRes, shipClassFileHand)->cost;
                shipClass[i].deathDelay = ResRec(ShipRes, shipClassFileHand)->deathDelay;
                shipClass[i].mass = ResRec(ShipRes, shipClassFileHand)->mass;
                shipClass[i].shipLength = ResRec(ShipRes, shipClassFileHand)->length;
                shipClass[i].inherentAI = ResRec(ShipRes, shipClassFileHand)->inherentAI;
                shipClass[i].crew = ResRec(ShipRes, shipClassFileHand)->crew;
                shipClass[i].strength = ResRec(ShipRes, shipClassFileHand)->strength;
                shipClass[i].flags = ResRec(ShipRes, shipClassFileHand)->flags;
                shipClass[i].flags2 = ResRec(ShipRes, shipClassFileHand)->flags2;
                shipClass[i].flags3 = ResRec(ShipRes, shipClassFileHand)->flags3;
                shipClass[i].dispWeight = ResRec(ShipRes, shipClassFileHand)->dispWeight;
                shipClass[i].podCount = ResRec(ShipRes, shipClassFileHand)->podCount;
                shipClass[i].buyRandom = ResRec(ShipRes, shipClassFileHand)->buyRandom;
                shipClass[i].hireRandom = ResRec(ShipRes, shipClassFileHand)->hireRandom;
                if (InGovtClass(ResRec(ShipRes, shipClassFileHand)->inherentGovt, 0)) {
                    shipClass[i].inherentGovt = ResRec(ShipRes, shipClassFileHand)->inherentGovt - ResClassBase(0);
                    shipClass[i].inherentGovtB = shipClass[i].inherentGovt;
                } else if (InGovtClass(ResRec(ShipRes, shipClassFileHand)->inherentGovt, 1)) {
                    shipClass[i].inherentGovt = -1;
                    shipClass[i].inherentGovtB = ResRec(ShipRes, shipClassFileHand)->inherentGovt - ResClassBase(1);
                } else if (InGovtClass(ResRec(ShipRes, shipClassFileHand)->inherentGovt, 2)) {
                    shipClass[i].inherentGovt = ResRec(ShipRes, shipClassFileHand)->inherentGovt - ResClassBase(2);
                    shipClass[i].inherentGovtB = -1;
                } else {
                    shipClass[i].inherentGovt = -1;
                    shipClass[i].inherentGovtB = -1;
                }
                if (shipClass[i].buyRandom > 100)
                    shipClass[i].buyRandom = 100;
                if (shipClass[i].hireRandom > 100)
                    shipClass[i].hireRandom = 100;
                if (shipClass[i].strength < 1)
                    shipClass[i].strength = 1;
                shipClass[i].upgradeTo = ResRec(ShipRes, shipClassFileHand)->upgradeTo;
                shipClass[i].escUpgradeCost = ResRec(ShipRes, shipClassFileHand)->escUpgrdCost;
                shipClass[i].tradeIn = ResRec(ShipRes, shipClassFileHand)->escSellValue;
                if (shipClass[i].upgradeTo >= kFirstResourceID)
                    shipClass[i].upgradeTo -= kFirstResourceID;
                else
                    shipClass[i].upgradeTo = -1;
                if (shipClass[i].escUpgradeCost < 0)
                    shipClass[i].escUpgradeCost = 0;
                if (shipClass[i].tradeIn <= 0)
                    shipClass[i].tradeIn = shipClass[i].cost * 0.1;
                shipClass[i].escortType = ResRec(ShipRes, shipClassFileHand)->escortType;
                if (shipClass[i].escortType < 0 || shipClass[i].escortType > 3) {
                    if (shipClass[i].inherentAI >= 3) {
                        if (shipClass[i].mass < 50)
                            shipClass[i].escortType = 0;
                        else if (shipClass[i].mass < 200)
                            shipClass[i].escortType = 1;
                        else
                            shipClass[i].escortType = 2;
                    } else {
                        shipClass[i].escortType = 3;
                    }
                }
                if (shipClass[i].escortType == 0)
                    shipClass[i].aiSkill = 80;
                else if (shipClass[i].escortType == 1)
                    shipClass[i].aiSkill = 90;
                else
                    shipClass[i].aiSkill = 100;
                shipClass[i].explode1 = ResRec(ShipRes, shipClassFileHand)->explode[0];
                shipClass[i].explode2 = ResRec(ShipRes, shipClassFileHand)->explode[1];
                shipClass[i].shieldRech = ResRec(ShipRes, shipClassFileHand)->shieldRe;
                shipClass[i].armorRech = ResRec(ShipRes, shipClassFileHand)->armorRe;
                if (shipClass[i].shieldRech > 0.0f)
                    shipClass[i].shieldRech *= 0.001;
                else
                    shipClass[i].shieldRech = 0;
                if (shipClass[i].armorRech > 0.0f)
                    shipClass[i].armorRech *= 0.001;
                else
                    shipClass[i].armorRech = 0;
                shipClass[i].deionize = ResRec(ShipRes, shipClassFileHand)->deionize;
                shipClass[i].ionizeMax = ResRec(ShipRes, shipClassFileHand)->ionizeMax;
                if (shipClass[i].deionize > 0.0f)
                    shipClass[i].deionize *= 0.01;
                else
                    shipClass[i].deionize = 1.0f;
                if (shipClass[i].ionizeMax <= 0)
                    shipClass[i].ionizeMax = 0;
                shipClass[i].keyCarried = ResRec(ShipRes, shipClassFileHand)->keyCarried;
                if (shipClass[i].keyCarried >= kFirstResourceID)
                    shipClass[i].keyCarried -= kFirstResourceID;
                else
                    shipClass[i].keyCarried = -1;
                if (shipClass[i].strength < 1)
                    shipClass[i].strength = 1;
                if (shipClass[i].cargo < 0) {
                    shipClass[i].cargo *= -1;
                    shipClass[i].cargoNotFixed = 0;
                } else {
                    shipClass[i].cargoNotFixed = 1;
                }
                if (shipClass[i].shield < 0)
                    shipClass[i].shield *= -5;
                if (shipClass[i].crew < 0)
                    shipClass[i].crew = 0;
                for (j = 0; j < 4; j++) {
                    h = shipClassFileHand;
                    mountID[j + 4] = -1;
                    mountID[j] = mountID[j + 4];
                    weapID = ((ShipRes *)(res = *shipClassFileHand))->wType[j];
                    if (weapID >= kFirstResourceID && weapID < kFirstResourceID + kMaxWeapTypes) {
                        char *p = *h;

                        mountID[j] = weapID;
                        mountCount[j] = ((ShipRes *)p)->wCount[j];
                        mountAmmo[j] = ((ShipRes *)p)->ammo[j];
                    } else {
                        mountID[j] = -1;
                    }
                    weapID = ((ShipRes *)res)->wType2[j];
                    if (weapID >= kFirstResourceID && weapID < kFirstResourceID + kMaxWeapTypes) {
                        char *p = *h;

                        mountID[j + 4] = weapID;
                        mountCount[j + 4] = ((ShipRes *)p)->wCount2[j];
                        mountAmmo[j + 4] = ((ShipRes *)p)->ammo2[j];
                    } else {
                        mountID[j + 4] = -1;
                    }
                }
                shipClass[i].defOutfID[0] = ResRec(ShipRes, shipClassFileHand)->defaultItems[0];
                shipClass[i].defOutfID[1] = ResRec(ShipRes, shipClassFileHand)->defaultItems[1];
                shipClass[i].defOutfID[2] = ResRec(ShipRes, shipClassFileHand)->defaultItems[2];
                shipClass[i].defOutfID[3] = ResRec(ShipRes, shipClassFileHand)->defaultItems[3];
                shipClass[i].defOutfID[4] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[0];
                shipClass[i].defOutfID[5] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[1];
                shipClass[i].defOutfID[6] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[2];
                shipClass[i].defOutfID[7] = ResRec(ShipRes, shipClassFileHand)->defaultItems2[3];
                shipClass[i].defOutfCount[0] = ResRec(ShipRes, shipClassFileHand)->itemCount[0];
                shipClass[i].defOutfCount[1] = ResRec(ShipRes, shipClassFileHand)->itemCount[1];
                shipClass[i].defOutfCount[2] = ResRec(ShipRes, shipClassFileHand)->itemCount[2];
                shipClass[i].defOutfCount[3] = ResRec(ShipRes, shipClassFileHand)->itemCount[3];
                shipClass[i].defOutfCount[4] = ResRec(ShipRes, shipClassFileHand)->itemCount2[0];
                shipClass[i].defOutfCount[5] = ResRec(ShipRes, shipClassFileHand)->itemCount2[1];
                shipClass[i].defOutfCount[6] = ResRec(ShipRes, shipClassFileHand)->itemCount2[2];
                shipClass[i].defOutfCount[7] = ResRec(ShipRes, shipClassFileHand)->itemCount2[3];
                for (count = 0; count < kDefaultOutfitSlots; count++) {
                    if (shipClass[i].defOutfID[count] >= kFirstResourceID && shipClass[i].defOutfID[count] < kFirstResourceID + kMaxOutfits) {
                        shipClass[i].defOutfID[count] -= kFirstResourceID;
                        if (shipClass[i].defOutfCount[count] < 0)
                            shipClass[i].defOutfCount[count] = 0;
                    } else {
                        shipClass[i].defOutfID[count] = -1;
                        shipClass[i].defOutfCount[count] = 0;
                    }
                }
                shipClass[i].fuelRegen = ResRec(ShipRes, shipClassFileHand)->fuelRegen;
                shipClass[i].skillVar = ResRec(ShipRes, shipClassFileHand)->skillVar;
                if (shipClass[i].skillVar < 1)
                    shipClass[i].skillVar = 1;
                if (shipClass[i].skillVar > 50)
                    shipClass[i].skillVar = 50;
                shipClass[i].contributes[0] = ResRec(ShipRes, shipClassFileHand)->contribute1;
                shipClass[i].contributes[1] = ResRec(ShipRes, shipClassFileHand)->contribute2;
                shipClass[i].requires[0] = ResRec(ShipRes, shipClassFileHand)->require1;
                shipClass[i].requires[1] = ResRec(ShipRes, shipClassFileHand)->require2;
                tradeEst = shipClass[i].cost * 0.25;
                gunTally = 0;
                turretTally = 0;
                for (k = 0; k < 8; k++) {
                    if (mountID[k] >= kFirstResourceID && mountID[k] < kFirstResourceID + kMaxWeapTypes) {
                        shipClass[i].weapCount[mountID[k] - kFirstResourceID] = mountCount[k];
                        shipClass[i].weapAmmo[mountID[k] - kFirstResourceID] = mountAmmo[k];
                        if (mountCount[k] > 0) {
                            for (j = 0; j < kMaxOutfits; j++) {
                                if (outfitSystem[j].modType[0] == kOutfModWeapon &&
                                    outfitSystem[j].modVal[0] == mountID[k] - kFirstResourceID &&
                                    outfitSystem[j].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&outfitSystem[j], &shipClass[i]) * mountCount[k];
                                    shipClass[i].freeMass += mass;
                                    if (!outfitSystem[j].persistent)
                                        tradeEst = tradeEst +
                                            (AdjustedItemCost(&outfitSystem[j], &shipClass[i]) * mountCount[k]) * 0.5;
                                    if (outfitSystem[j].flags & kOutfFixedGun)
                                        gunTally += mountCount[k];
                                    if (outfitSystem[j].flags & kOutfTurret)
                                        turretTally += mountCount[k];
                                    break;
                                }
                            }
                        }
                        if (mountAmmo[k] > 0) {
                            if (weaponStats[mountID[k] - kFirstResourceID].guidance == kGuidanceBay)
                                n = mountID[k] - kFirstResourceID;
                            else {
                                n = weaponStats[mountID[k] - kFirstResourceID].ammoType;
                                if (n < 0 || n >= kMaxWeapTypes)
                                    n = -1;
                            }
                            if (n != -1) {
                                for (j = 0; j < kMaxOutfits; j++) {
                                    if (outfitSystem[j].modType[0] == kOutfModAmmo &&
                                        outfitSystem[j].modVal[0] == n &&
                                        outfitSystem[j].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&outfitSystem[j], &shipClass[i]) * mountAmmo[k];
                                    shipClass[i].freeMass += mass;
                                        if (!outfitSystem[j].persistent)
                                            tradeEst = tradeEst +
                                                (AdjustedItemCost(&outfitSystem[j], &shipClass[i]) * mountCount[k]) * 0.5;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                maxGuns = shipClass[i].maxGuns;
                maxTurrets = shipClass[i].maxTurrets;
                for (k = 0; k < kDefaultOutfitSlots; k++) {
                    if (shipClass[i].defOutfID[k] != -1 && shipClass[i].defOutfCount[k] > 0 &&
                        outfitSystem[shipClass[i].defOutfID[k]].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&outfitSystem[shipClass[i].defOutfID[k]], &shipClass[i]) *
                               shipClass[i].defOutfCount[k];
                        shipClass[i].freeMass += mass;
                        if (!outfitSystem[shipClass[i].defOutfID[k]].persistent)
                            tradeEst = tradeEst +
                                (AdjustedItemCost(&outfitSystem[shipClass[i].defOutfID[k]], &shipClass[i]) *
                                 shipClass[i].defOutfCount[k]) * 0.5;
                        if (crashlog_isactive()) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (outfitSystem[shipClass[i].defOutfID[k]].modType[j] == 45)
                                    maxGuns += outfitSystem[shipClass[i].defOutfID[k]].modVal[j];
                                if (outfitSystem[shipClass[i].defOutfID[k]].modType[j] == 46)
                                    maxTurrets += outfitSystem[shipClass[i].defOutfID[k]].modVal[j];
                            }
                        }
                    }
                }
                if (shipClass[i].flags & kShipSlowJump)
                    shipClass[i].mobilityFactor = 0.7f;
                else if (shipClass[i].flags & kShipSemiFastJump)
                    shipClass[i].mobilityFactor = 1.3f;
                else if (shipClass[i].flags & kShipFastJump)
                    shipClass[i].mobilityFactor = 1.6f;
                else
                    shipClass[i].mobilityFactor = 1.0f;
                shipClass[i].mobilityFactor *= 1.3;
                if (shipClass[i].mobilityFactor < 0.0f)
                    shipClass[i].mobilityFactor = 0.5f;
                strcpy(shipClass[i].availability, ResRec(ShipRes, shipClassFileHand)->availability);
                strcpy(shipClass[i].appearOn, ResRec(ShipRes, shipClassFileHand)->appearOn);
                strcpy(shipClass[i].onPurchase, ResRec(ShipRes, shipClassFileHand)->onPurchase);
                strcpy(shipClass[i].onCapture, ResRec(ShipRes, shipClassFileHand)->onCapture);
                strcpy(shipClass[i].onRetire, ResRec(ShipRes, shipClassFileHand)->onRetire);
                AdjustHandleSize(shipClassFileHand, 0x744);
                GetResInfo(shipClassFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)shipClass[i].name, (char *)name, 63);
                strcpy((char *)shipyardShortNames[i], ResRec(ShipRes, shipClassFileHand)->shortName);
                strcpy((char *)shipyardLongNames[i], ResRec(ShipRes, shipClassFileHand)->longName);
                strcpy((char *)shipCommNames[i], ResRec(ShipRes, shipClassFileHand)->commName);
                strcpy((char *)shipMovieFilenames[i], ResRec(ShipRes, shipClassFileHand)->movieName);
                strcpy((char *)shipClass[i].subtitle, ResRec(ShipRes, shipClassFileHand)->subtitle);
                CtoPstr((char *)shipyardShortNames[i]);
                CtoPstr((char *)shipyardLongNames[i]);
                CtoPstr((char *)shipCommNames[i]);
                CtoPstr((char *)shipMovieFilenames[i]);
                CtoPstr((char *)shipClass[i].subtitle);
                if (crashlog_isactive()) {
                    if (maxGuns < 0) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has a maximum gun count of ");
                        crashlog_print_num(maxGuns);
                        crashlog_print("\n");
                    }
                    if (maxTurrets < 0) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") has a maximum turret count of ");
                        crashlog_print_num(maxTurrets);
                        crashlog_print("\n");
                    }
                    if (gunTally > maxGuns) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
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
                        crashlog_print_pstr(shipClass[i].name);
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
                if (tradeEst > shipClass[i].cost && shipClass[i].buyRandom != 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a trade-in value of ");
                    crashlog_print_num(tradeEst);
                    crashlog_print(" but costs only ");
                    crashlog_print_num(shipClass[i].cost);
                    crashlog_print("\n");
                }
                UnlockAHandle(shipClassFileHand);
                ResourcePurge(shipClassFileHand);
                ReleaseResource(shipClassFileHand);
                if (crashlog_isactive()) {
                    if (shipClass[i].buyRandom != 0) {
                        if (!ResourceExists(kDescType, i + 13000)) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(shipClass[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(") lacks purchase description\n");
                        }
                        for (count = 0; count < kMaxWeapTypes; count++) {
                            if (shipClass[i].weapCount[count] > 0) {
                                short granted = 0;

                                for (j = 0; j < kMaxOutfits; j++) {
                                    for (k = 0; k < kOutfMods; k++) {
                                        if (outfitSystem[j].modType[k] == 1 &&
                                            outfitSystem[j].modVal[k] == count) {
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
                                    crashlog_print(" lacks corresponding " kOutfResName " resource\n");
                                }
                            }
                        }
                    }
                    if (shipClass[i].hireRandom != 0 &&
                        !ResourceExists(kDescType, i + 14000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks hiring description\n");
                    }
                }
                LoadingIntroMusic();
            } else {
                strcpy(shipClass[i].availability, "");
                strcpy(shipClass[i].appearOn, "");
                strcpy(shipClass[i].onPurchase, "");
                strcpy(shipClass[i].onCapture, "");
                strcpy(shipClass[i].onRetire, "");
                shipClass[i].techLevel = kResAbsent;
                shipClass[i].shield = 1;
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
        for (i = 0; i < kMaxDudes; i++) {
            dudeFileHand = GetResource(kDudeType, i + kFirstResourceID);
            if (dudeFileHand != NULL) {
                found++;
                LockAHandle(dudeFileHand);
                ResourceNoPurge(dudeFileHand);
                dudeData[i].aiType = ResRec(DudeRes, dudeFileHand)->aiType;
                dudeData[i].govt = ResRec(DudeRes, dudeFileHand)->govt;
                dudeData[i].flags = ResRec(DudeRes, dudeFileHand)->booty;
                dudeData[i].adviceFlags = ResRec(DudeRes, dudeFileHand)->infoTypes;
                if (dudeData[i].govt >= kFirstResourceID && dudeData[i].govt < kFirstResourceID + kMaxGovts)
                    dudeData[i].govt -= kFirstResourceID;
                else
                    dudeData[i].govt = -1;
                for (j = 0; j < kDudeShipSlots; j++) {
                    dudeData[i].shipTypes[j] = ResRec(DudeRes, dudeFileHand)->shipTypes[j];
                    dudeData[i].shipProbs[j] = ResRec(DudeRes, dudeFileHand)->probs[j];
                    if (dudeData[i].shipTypes[j] >= kFirstResourceID && dudeData[i].shipTypes[j] < kFirstResourceID + kMaxShipTypes) {
                        dudeData[i].shipTypes[j] -= kFirstResourceID;
                        if (shipClass[dudeData[i].shipTypes[j]].techLevel == kResAbsent) {
                            crashlog_print("warning: " kDudeResName " resource ");
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
                UnlockAHandle(dudeFileHand);
                AdjustHandleSize(dudeFileHand, 0x58);
                ResourcePurge(dudeFileHand);
                ReleaseResource(dudeFileHand);
                dudeData[i].loaded = 1;
                LoadingIntroMusic();
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
        int32_t k;

        found = 0;
        count = CountResources(kGovtType);
        for (i = 0; i < kMaxGovts; i++) {
            govtFileHand = GetResource(kGovtType, i + kFirstResourceID);
            if (govtFileHand != NULL) {
                found++;
                LockAHandle(govtFileHand);
                ResourceNoPurge(govtFileHand);
                govtData[i].loaded = 1;
                govtData[i].flags = ResRec(GovtRes, govtFileHand)->flags;
                govtData[i].flags2 = ResRec(GovtRes, govtFileHand)->flags2;
                govtData[i].crimeTol = ResRec(GovtRes, govtFileHand)->crimeTol;
                govtData[i].scanFine = ResRec(GovtRes, govtFileHand)->scanFine;
                govtData[i].initialRec = ResRec(GovtRes, govtFileHand)->initialRec;
                govtData[i].voiceType = ResRec(GovtRes, govtFileHand)->voiceType;
                if (govtData[i].voiceType >= 0 && govtData[i].voiceType < kVoiceSets) {
                    govtData[i].voiceClass = -1;
                } else if (govtData[i].voiceType >= 1000 && govtData[i].voiceType < 1000 + kVoiceSets) {
                    govtData[i].voiceType -= 1000;
                    govtData[i].voiceClass = 1;
                } else if (govtData[i].voiceType >= 2000 && govtData[i].voiceType < 2000 + kVoiceSets) {
                    govtData[i].voiceType -= 2000;
                    govtData[i].voiceClass = 0;
                } else {
                    govtData[i].voiceType = -1;
                    govtData[i].voiceClass = -1;
                }
                for (j = 0; j < 4; j++) {
                    govtData[i].inhJam[j] = ResRec(GovtRes, govtFileHand)->inhJam[j];
                    if (govtData[i].inhJam[j] > 100)
                        govtData[i].inhJam[j] = 100;
                    if (govtData[i].inhJam[j] < 0)
                        govtData[i].inhJam[j] = 0;
                }
                if (ResRec(GovtRes, govtFileHand)->skillMult > 0)
                    govtData[i].skillMult = ResRec(GovtRes, govtFileHand)->skillMult * 0.01;
                else
                    govtData[i].skillMult = 1.0f;
                govtData[i].maxOdds = ResRec(GovtRes, govtFileHand)->maxOdds * 0.01;
                if (govtData[i].maxOdds < 0.01)
                    govtData[i].maxOdds = 0.01f;
                for (j = 0; j <= 4; j++)
                    govtData[i].penalties[j] = ResRec(GovtRes, govtFileHand)->penalty[j];
                for (k = 0; k < 4; k++) {
                    govtData[i].classes[k] = ResRec(GovtRes, govtFileHand)->classes[k];
                    govtData[i].allies[k] = ResRec(GovtRes, govtFileHand)->allies[k];
                    govtData[i].enemies[k] = ResRec(GovtRes, govtFileHand)->enemies[k];
                }
                govtData[i].scanMask = ResRec(GovtRes, govtFileHand)->scanMask;
                govtData[i].requires[0] = ResRec(GovtRes, govtFileHand)->require[0];
                govtData[i].requires[1] = ResRec(GovtRes, govtFileHand)->require[1];
                govtData[i].rgb.red = ((ResRec(GovtRes, govtFileHand)->color >> 16) & 0xff) * 256;
                govtData[i].rgb.green = ((ResRec(GovtRes, govtFileHand)->color >> 8) & 0xff) * 256;
                govtData[i].rgb.blue = (ResRec(GovtRes, govtFileHand)->color & 0xff) * 256;
#ifdef MACBUILD
                govtData[i].rgb2.red = (ResRec(GovtRes, govtFileHand)->shipColor >> 19) & 0x1f;
                govtData[i].rgb2.green = (ResRec(GovtRes, govtFileHand)->shipColor >> 11) & 0x1f;
                govtData[i].rgb2.blue = (ResRec(GovtRes, govtFileHand)->shipColor >> 3) & 0x1f;
#else
                govtData[i].rgb2.red = ((ResRec(GovtRes, govtFileHand)->shipColor >> 16) & 0xff) * 256;
                govtData[i].rgb2.green = ((ResRec(GovtRes, govtFileHand)->shipColor >> 8) & 0xff) * 256;
                govtData[i].rgb2.blue = (ResRec(GovtRes, govtFileHand)->shipColor & 0xff) * 256;
#endif
                govtData[i].rgbBytes.red = govtData[i].rgb.red >> 8;
                govtData[i].rgbBytes.green = govtData[i].rgb.green >> 8;
                govtData[i].rgbBytes.blue = govtData[i].rgb.blue >> 8;
                govtData[i].interfaceID = ResRec(GovtRes, govtFileHand)->interfaceID;
                if (govtData[i].interfaceID < kFirstResourceID)
                    govtData[i].interfaceID = -1;
                govtData[i].newsPict = ResRec(GovtRes, govtFileHand)->newsPic;
                if (govtData[i].newsPict < kFirstResourceID)
                    govtData[i].newsPict = -1;
                GetResInfo(govtFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)govtData[i].name, (char *)name, 31);
                strcpy((char *)govtTargStrings[i], ResRec(GovtRes, govtFileHand)->targetCode);
                strcpy((char *)govtShortNames[i], ResRec(GovtRes, govtFileHand)->commName);
                strcpy((char *)govtMediumNames[i], ResRec(GovtRes, govtFileHand)->mediumName);
                CtoPstr((char *)govtTargStrings[i]);
                CtoPstr((char *)govtShortNames[i]);
                CtoPstr((char *)govtMediumNames[i]);
                UnlockAHandle(govtFileHand);
                AdjustHandleSize(govtFileHand, 0xc0);
                ResourcePurge(govtFileHand);
                ReleaseResource(govtFileHand);
                LoadingIntroMusic();
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
                    for (n = i + 1; n < kMaxGovts; n++) {
                        if (govtData[n].loaded &&
                            GovtEnemies(i, n) && GovtAllies(i, n)) {
                            crashlog_print("warning: govts ");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(" and ");
                            crashlog_print_num(n + kFirstResourceID);
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
        int32_t shieldSum;

        found = 0;
        n = count = CountResources(kPersType);
        for (i = 0; i < kMaxPersons;i++) {
            personData[i].loaded = 0;
            personData[i].aiType = -1;
            personFileHand = GetResource(kPersType, i + kFirstResourceID);
            if (personFileHand != NULL) {
                found++;
                LockAHandle(personFileHand);
                ResourceNoPurge(personFileHand);
                personData[i].linkSyst = ResRec(PersRes, personFileHand)->linkSyst;
                personData[i].govt = ResRec(PersRes, personFileHand)->govt;
                personData[i].aiType = ResRec(PersRes, personFileHand)->aiType;
                personData[i].aggression = ResRec(PersRes, personFileHand)->aggress;
                personData[i].coward = ResRec(PersRes, personFileHand)->coward;
                personData[i].shipType = ResRec(PersRes, personFileHand)->shipType;
                if (personData[i].shipType >= kFirstResourceID && personData[i].shipType < kFirstResourceID + kMaxShipTypes) {
                    personData[i].shipType -= kFirstResourceID;
                    if (shipClass[personData[i].shipType].techLevel == kResAbsent) {
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
                personData[i].credits = ResRec(PersRes, personFileHand)->credits;
                personData[i].shieldMod = ResRec(PersRes, personFileHand)->shieldMod;
                personData[i].shieldMod = personData[i].shieldMod / 100.0;
                personData[i].commQuote = ResRec(PersRes, personFileHand)->commQuote;
                personData[i].hailQuote = ResRec(PersRes, personFileHand)->hailQuote;
                personData[i].linkMission = ResRec(PersRes, personFileHand)->linkMission;
                personData[i].flags = ResRec(PersRes, personFileHand)->flags;
                personData[i].flags2 = ResRec(PersRes, personFileHand)->flags2;
                personData[i].hailPict = ResRec(PersRes, personFileHand)->hailPict;
                personData[i].rgb5[0] = (ResRec(PersRes, personFileHand)->shipColor >> 19) & 0x1f;
#ifdef MACBUILD
                personData[i].rgb5[1] = (ResRec(PersRes, personFileHand)->shipColor >> 11) & 0x1f;
#else
                /* BUG: takes bits 5-9 (green low, blue high); the mac takes green's top five, >> 11 */
                personData[i].rgb5[1] = (ResRec(PersRes, personFileHand)->shipColor >> 5) & 0x1f;
#endif
                personData[i].rgb5[2] = (ResRec(PersRes, personFileHand)->shipColor >> 3) & 0x1f;
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
                personData[i].grantClass = ResRec(PersRes, personFileHand)->grantClass;
                personData[i].grantCount = ResRec(PersRes, personFileHand)->grantCount;
                personData[i].grantProb = ResRec(PersRes, personFileHand)->grantProb;
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
                mountID[1] = ResRec(PersRes, personFileHand)->weapType.left;
                mountID[0] = ResRec(PersRes, personFileHand)->weapType.top;
                mountID[3] = ResRec(PersRes, personFileHand)->weapType.right;
                mountID[2] = ResRec(PersRes, personFileHand)->weapType.bottom;
                mountCount[1] = ResRec(PersRes, personFileHand)->weapCount.left;
                mountCount[0] = ResRec(PersRes, personFileHand)->weapCount.top;
                mountCount[3] = ResRec(PersRes, personFileHand)->weapCount.right;
                mountCount[2] = ResRec(PersRes, personFileHand)->weapCount.bottom;
                mountAmmo[1] = ResRec(PersRes, personFileHand)->ammoLoad.left;
                mountAmmo[0] = ResRec(PersRes, personFileHand)->ammoLoad.top;
                mountAmmo[3] = ResRec(PersRes, personFileHand)->ammoLoad.right;
                mountAmmo[2] = ResRec(PersRes, personFileHand)->ammoLoad.bottom;
                for (j = 0; j < kMaxWeapTypes; j++) {
                    personData[i].weapCount[j] = 0;
                    personData[i].weapAmmo[j] = 0;
                }
                for (mountSlot = 0; mountSlot <= 3; mountSlot++) {
                    if (mountID[mountSlot] >= kFirstResourceID) {
                        personData[i].weapCount[mountID[mountSlot] - kFirstResourceID] = mountCount[mountSlot];
                        personData[i].weapAmmo[mountID[mountSlot] - kFirstResourceID] = mountAmmo[mountSlot];
                    }
                }
                strcpy(personData[i].subtitle, ResRec(PersRes, personFileHand)->subtitle);
                strcpy(personData[i].activeOn, ResRec(PersRes, personFileHand)->activateOn);
                CtoPstr(personData[i].subtitle);
                AdjustHandleSize(personFileHand, 400);
                personData[i].loaded = 1;
                GetResInfo(personFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy(personData[i].name, (char *)name, 30);
                UnlockAHandle(personFileHand);
                ResourcePurge(personFileHand);
                ReleaseResource(personFileHand);
                personData[i].prepared = 1;
                LoadingIntroMusic();
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
#if V111 /* V111 spelling */
        strcpy(personData[kPersCaptainHector].name, (char *)"\pCaptain Hector");
        strcpy(personData[kPersCaptainHector].subtitle, (char *)"\pShareware Enforcer");
#else
        CStr2PStr(personData[kPersCaptainHector].name, "Captain Hector");
        CStr2PStr(personData[kPersCaptainHector].subtitle, "Shareware Enforcer");
#endif

        /* pick him a ship: sufficiently shielded and agile vs the fleet average */
        shieldSum = 0;
        n = 0;
        for (i = 0; i < kMaxShipTypes;i++) {
            if (shipClass[i].techLevel != kResAbsent && shipClass[i].turnRate > 2.0f) {
                shieldSum += shipClass[i].shield;
                n++;
            }
        }
        if (n > 0)
            shieldSum /= n;
        else
            shieldSum = 300;
        personData[kPersCaptainHector].shipType = -1;
        for (i = 0; i < kMaxShipTypes;i++) {
            if (shipClass[i].techLevel != kResUnloaded && shipClass[i].inherentAI >= 2 &&
                shipClass[i].shield >= shieldSum * 0.88 &&
                shipClass[i].turnRate > 2.0f) {
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
        n = kPersCaptainHector;
        for (count = 0; count < kMaxWeapTypes; count++) {
            if (weaponStats[count].guidance >= kGuidanceUnguided &&
                weaponStats[count].damage[0] > 0 && weaponStats[count].damage[1] > 0 &&
                weaponStats[count].subCount <= 0 &&
                (weaponStats[count].damage[0] >= shieldSum / 40.0 ||
                 weaponStats[count].damage[1] >= shieldSum / 40.0)) {
                personData[n].weapCount[count] = 10;
                personData[n].weapAmmo[count] = 1000;
                j++;
                if ((totalDaysPlayed < 35 && j > 2) ||
                    (totalDaysPlayed < 40 && j > 3) ||
                    (totalDaysPlayed < 45 && j > 4) ||
                    (totalDaysPlayed < 50 && j > 5) ||
                    (totalDaysPlayed < 55 && j > 6))
                    break;
            }
        }

        /* alias persons that share a name prefix */
        for (count = 0; count < kMaxPersons;count++)
            personData[count].sameNameIndex = count;
        for (i = 0; i < kMaxPersons;i++) {
            if (personData[i].name[0] > 0) {
                for (j = 0; j < i; j++) {
                    if (personData[j].name[0] > 0 &&
                        j == personData[j].sameNameIndex &&
                        strncmp(&personData[i].name[1],
                                &personData[j].name[1],
                                personData[i].name[0]) == 0) {
                        personData[i].sameNameIndex = j;
                        break;
                    }
                }
            }
        }
        if (!gameLoaded)
            IterateInGameEventLoop(10);

        /* systems must reference existing dudes and persons */
        for (j = 0; j < kMaxSystems; j++) {
            if (systemData[j].prepared) {
                for (n = 0; n < 8; n++) {
                    if (systemData[j].dudeTypes[n] != -1 && !dudeData[systemData[j].dudeTypes[n]].loaded) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(j + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                        crashlog_print_num(systemData[j].dudeTypes[n] + kFirstResourceID);
                        crashlog_print("\n");
                        systemData[j].dudeTypes[n] = -1;
                    }
                }
                for (n = 0; n < 8; n++) {
                    if (systemData[j].persons[n] != -1) {
                    i = systemData[j].persons[n];
                    if (!personData[i].prepared) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(j + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent " kPersResName " resource ");
                        crashlog_print_num(systemData[j].persons[n] + kFirstResourceID);
                        crashlog_print("\n");
                        systemData[j].persons[n] = -1;
                    }
                    }
                }
            }
        }
    }

    /* ---------------- öops: news ---------------- */
    {

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
            disasterFileHand = GetResource(kOopsType, i + kFirstResourceID);
            if (disasterFileHand != NULL) {
                found++;
                LockAHandle(disasterFileHand);
                ResourceNoPurge(disasterFileHand);
                disasterData[i].stellar = ResRec(OopsRes, disasterFileHand)->stellar;
                disasterData[i].commodity = ResRec(OopsRes, disasterFileHand)->commodity;
                disasterData[i].amount = ResRec(OopsRes, disasterFileHand)->priceDelta;
                disasterData[i].duration = ResRec(OopsRes, disasterFileHand)->duration;
                disasterData[i].freq = ResRec(OopsRes, disasterFileHand)->freq;
                disasterData[i].countdown = 0;
                if (disasterData[i].commodity > 5)
                    disasterData[i].commodity = 5;
                if (disasterData[i].commodity < 0)
                    disasterData[i].commodity = -1;
                strcpy(disasterData[i].text, ResRec(OopsRes, disasterFileHand)->activateOn);
                disasterData[i].loaded = 1;
                AdjustHandleSize(disasterFileHand, 0x11a);
                GetResInfo(disasterFileHand, &resID, &resType, name);
                TrimResourceName(name);
                pstrcpy(disasterData[i].name, name);
                UnlockAHandle(disasterFileHand);
                ResourcePurge(disasterFileHand);
                ReleaseResource(disasterFileHand);
#ifdef MACBUILD
                if (disasterData[i].commodity != -1
                    && abs(disasterData[i].amount) < basePrice[disasterData[i].commodity] * 0.5) {
                    crashlog_print("warning: disaster ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" (");
                    crashlog_print_pstr(disasterData[i].name);
                    crashlog_print(") has an unusually small price delta\n");
                }
#endif
                LoadingIntroMusic();
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
        nebulaFileHand = GetResource(kNebuType, i + kFirstResourceID);
        if (nebulaFileHand != NULL) {
            ResourceNoPurge(nebulaFileHand);
            nebulaData[i].xPos = ResRec(NebuRes, nebulaFileHand)->xPos;
            nebulaData[i].yPos = ResRec(NebuRes, nebulaFileHand)->yPos;
            nebulaData[i].width = ResRec(NebuRes, nebulaFileHand)->xSize;
            nebulaData[i].height = ResRec(NebuRes, nebulaFileHand)->ySize;
            strcpy(nebulaData[i].activeOn, ResRec(NebuRes, nebulaFileHand)->activeOn);
            strcpy(nebulaData[i].onExplore, ResRec(NebuRes, nebulaFileHand)->onExplore);
            ResourcePurge(nebulaFileHand);
            ReleaseResource(nebulaFileHand);
            LoadingIntroMusic();
        }
    }

    /* ---------------- bööm: explosions ---------------- */
    for (i = 0; i < kMaxExplodTypes; i++) {
        explodType[i].frameRate = 0.25f;
        explodType[i].sound = -1;
        explodType[i].graphic = i;
        boomFileHand = GetResource(kBoomType, i + kFirstResourceID);
        if (boomFileHand != NULL) {
            explodType[i].frameRate = ResRec(BoomRes, boomFileHand)->frameAdvance;
            explodType[i].sound = ResRec(BoomRes, boomFileHand)->soundIndex;
            explodType[i].graphic = ResRec(BoomRes, boomFileHand)->graphicIndex;
            explodType[i].frameRate *= 0.01;
            ReleaseResource(boomFileHand);
            LoadingIntroMusic();
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
            fleetFileHand = GetResource(kFletType, i + kFirstResourceID);
            if (fleetFileHand != NULL) {
                found++;
                LockAHandle(fleetFileHand);
                ResourceNoPurge(fleetFileHand);
                fleetData[i].leadShipType = ResRec(FletRes, fleetFileHand)->leadShip;
                if (fleetData[i].leadShipType >= kFirstResourceID)
                    fleetData[i].leadShipType -= kFirstResourceID;
                else
                    fleetData[i].leadShipType = 0;
                fleetData[i].govt = ResRec(FletRes, fleetFileHand)->govt;
                fleetData[i].linkSyst = ResRec(FletRes, fleetFileHand)->linkSyst;
                fleetData[i].quoteStrList = ResRec(FletRes, fleetFileHand)->quote;
                fleetData[i].flags = ResRec(FletRes, fleetFileHand)->flags;
                if (fleetData[i].govt >= kFirstResourceID)
                    fleetData[i].govt -= kFirstResourceID;
                else
                    fleetData[i].govt = -1;
                for (j = 0; j <= 3; j++) {   /* exe's back-edge is `cmp ax,3 / jle` */
                    fleetData[i].escortTypes[j] = ResRec(FletRes, fleetFileHand)->escortShip[j];
                    if (fleetData[i].escortTypes[j] >= kFirstResourceID) {
                        fleetData[i].escortTypes[j] -= kFirstResourceID;
                        fleetData[i].escortMin[j] = ResRec(FletRes, fleetFileHand)->minCount[j];
                        fleetData[i].escortMax[j] = ResRec(FletRes, fleetFileHand)->maxCount[j];
                    } else {
                        fleetData[i].escortTypes[j] = -1;
                        fleetData[i].escortMin[j] = 0;
                        fleetData[i].escortMax[j] = 0;
                    }
                }
                strcpy(fleetData[i].activateOn, ResRec(FletRes, fleetFileHand)->activateOn);
                AdjustHandleSize(fleetFileHand, 0x132);
                UnlockAHandle(fleetFileHand);
                ResourcePurge(fleetFileHand);
                ReleaseResource(fleetFileHand);
                LoadingIntroMusic();
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

        junksLoaded = 0;
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
            junkFileHand = GetResource(kJunkType, i + kFirstResourceID);
            if (junkFileHand != NULL) {
                junksLoaded++;
                LockAHandle(junkFileHand);
                ResourceNoPurge(junkFileHand);
                junkData[i].basePrice = ResRec(JunkRes, junkFileHand)->basePrice;
                junkData[i].flags = ResRec(JunkRes, junkFileHand)->flags;
                junkData[i].scanMask = ResRec(JunkRes, junkFileHand)->scanMask;
                if (junkData[i].basePrice < 0) {
                    crashlog_print("warning: junk type ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" has invalid base price\n");
                    junkData[i].basePrice = 0;
                }
                for (j = 0; j < 8; j++) {
                    junkData[i].boughtAt[j] = ResRec(JunkRes, junkFileHand)->boughtAt[j];
                    junkData[i].soldAt[j] = ResRec(JunkRes, junkFileHand)->soldAt[j];
                    if (junkData[i].boughtAt[j] >= kFirstResourceID)
                        junkData[i].boughtAt[j] -= kFirstResourceID;
                    else
                        junkData[i].boughtAt[j] = -1;
                    if (junkData[i].soldAt[j] >= kFirstResourceID)
                        junkData[i].soldAt[j] -= kFirstResourceID;
                    else
                        junkData[i].soldAt[j] = -1;
                }
                GetResInfo(junkFileHand, &resID, &resType, name);
                TrimResourceName(name);
                strncpy((char *)junkData[i].name, (char *)name, 63);
                strcpy(junkData[i].lcName, ResRec(JunkRes, junkFileHand)->lcName);
                strcpy(junkData[i].abbrev, ResRec(JunkRes, junkFileHand)->abbrev);
                strcpy(junkData[i].buyOn, ResRec(JunkRes, junkFileHand)->buyOn);
                strcpy(junkData[i].sellOn, ResRec(JunkRes, junkFileHand)->sellOn);
                UnlockAHandle(junkFileHand);
                ResourcePurge(junkFileHand);
                ReleaseResource(junkFileHand);
                LoadingIntroMusic();
            }
            if (junksLoaded >= count)
                break;
        }

        /* trade centers must trade something */
        for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared &&
                (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) ==
                    (kSpobCanLand | kSpobTradeCenter) &&
                !SpobIsHyperOrWorm(&stellarData[i]) &&
                (stellarData[i].flags & kSpobPriceBits) == 0) {
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
            cronEvent[i].active = 0;
            cronEvent[i].loaded = 0;
            cronEvent[i].random = -1;
            cronEvent[i].duration = -1;
            cronEvent[i].daysLeft = 0;
            cronEvent[i].holdoffLeft = 0;
            cronFileHand = GetResource(kCronType, i + kFirstResourceID);
            if (cronFileHand != NULL) {
                LockAHandle(cronFileHand);
                cronEvent[i].firstDay = ResRec(CronRes, cronFileHand)->firstDay;
                cronEvent[i].firstMonth = ResRec(CronRes, cronFileHand)->firstMonth;
                cronEvent[i].firstYear = ResRec(CronRes, cronFileHand)->firstYear;
                cronEvent[i].lastDay = ResRec(CronRes, cronFileHand)->lastDay;
                cronEvent[i].lastMonth = ResRec(CronRes, cronFileHand)->lastMonth;
                cronEvent[i].lastYear = ResRec(CronRes, cronFileHand)->lastYear;
                cronEvent[i].random = ResRec(CronRes, cronFileHand)->random;
                cronEvent[i].duration = ResRec(CronRes, cronFileHand)->duration;
                cronEvent[i].preHoldoff = ResRec(CronRes, cronFileHand)->preHoldoff;
                cronEvent[i].postHoldoff = ResRec(CronRes, cronFileHand)->postHoldoff;
                cronEvent[i].flags = ResRec(CronRes, cronFileHand)->indNewsStr;
                for (j = 0; j < 4; j++) {
                    cronEvent[i].govts[j] = ResRec(CronRes, cronFileHand)->newsGovt[j];
                    cronEvent[i].govtValues[j] = ResRec(CronRes, cronFileHand)->govtNewsStr[j];
                    if (cronEvent[i].govts[j] >= kFirstResourceID) {
                        cronEvent[i].govts[j] -= kFirstResourceID;
                    } else {
                        cronEvent[i].govts[j] = -1;
                        cronEvent[i].govtValues[j] = -1;
                    }
                    if (cronEvent[i].govtValues[j] < -1)
                        cronEvent[i].govtValues[j] = -1;
                }
                cronEvent[i].indNewsStr = ResRec(CronRes, cronFileHand)->flags;
                for (j = 0; j <= 1; j++) {
                    cronEvent[i].contributes[j] = ResRec(CronRes, cronFileHand)->contribute[j];
                    cronEvent[i].requires[j] = ResRec(CronRes, cronFileHand)->require[j];
                }
                strcpy(cronEvent[i].enableOn, ResRec(CronRes, cronFileHand)->enableOn);
                strcpy(cronEvent[i].onStart, ResRec(CronRes, cronFileHand)->onStart);
                strcpy(cronEvent[i].onEnd, ResRec(CronRes, cronFileHand)->onEnd);
                cronEvent[i].loaded = 1;
                UnlockAHandle(cronFileHand);
                AdjustHandleSize(cronFileHand, 0x336);
                ResourcePurge(cronFileHand);
                ReleaseResource(cronFileHand);
                LoadingIntroMusic();
            }
        }
    }

    /* ---------------- ränk: ranks ---------------- */
    {

        for (i = 0; i < kMaxRanks; i++) {
            rankData[i].govt = -1;
            rankData[i].active = 0;
            rankFileHand = GetResource(kRankType, i + kFirstResourceID);
            if (rankFileHand != NULL) {
                LockAHandle(rankFileHand);
                rankData[i].loaded = 1;
                rankData[i].govt = ResRec(RankRes, rankFileHand)->attachTo;
                if (rankData[i].govt >= kFirstResourceID)
                    rankData[i].govt -= kFirstResourceID;
                else
                    rankData[i].govt = -1;
                rankData[i].priority = ResRec(RankRes, rankFileHand)->weight;
                rankData[i].contributes[0] = ResRec(RankRes, rankFileHand)->contribute[0];
                rankData[i].contributes[1] = ResRec(RankRes, rankFileHand)->contribute[1];
                rankData[i].flags = ResRec(RankRes, rankFileHand)->priceMod;
                rankData[i].salary = ResRec(RankRes, rankFileHand)->salary;
                rankData[i].salaryCap = ResRec(RankRes, rankFileHand)->salaryCap;
                rankData[i].priceMod = ResRec(RankRes, rankFileHand)->flags;
                if (rankData[i].priceMod <= 0)
                    rankData[i].priceMod = 100;
                GetResInfo(rankFileHand, &resID, &resType, name);
                TrimResourceName(name);
                PtoCstr(name);
                strncpy(rankData[i].name, (char *)name, 127);
                strcpy(rankData[i].convName, ResRec(RankRes, rankFileHand)->name);
                strcpy(rankData[i].shortName, ResRec(RankRes, rankFileHand)->shortName);
                UnlockAHandle(rankFileHand);
                AdjustHandleSize(rankFileHand, 0x98);
                ResourcePurge(rankFileHand);
                ReleaseResource(rankFileHand);
                LoadingIntroMusic();
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
            asteroidFileHand = GetResource(kRoidType, i + kFirstResourceID);
            if (asteroidFileHand != NULL) {
                LockAHandle(asteroidFileHand);
                asteroidType[i].strength = ResRec(RoidRes, asteroidFileHand)->strength;
                if (asteroidType[i].strength < 1)
                    asteroidType[i].strength = 1;
                asteroidType[i].frameRate = (float)ResRec(RoidRes, asteroidFileHand)->spinRate * 0.01;
                asteroidType[i].yieldType = ResRec(RoidRes, asteroidFileHand)->yieldType;
                asteroidType[i].yieldQty = ResRec(RoidRes, asteroidFileHand)->yieldQty;
                if (asteroidType[i].yieldQty < 0) {
                    asteroidType[i].yieldQty = 0;
                    asteroidType[i].yieldType = -1;
                }
                if (asteroidType[i].yieldType < 0 ||
                    (asteroidType[i].yieldType > 6 && asteroidType[i].yieldType < 1000) ||
                    asteroidType[i].yieldType >= 1000 + kMaxJunks) {
                    asteroidType[i].yieldType = -1;
                    asteroidType[i].yieldQty = 0;
                }
                asteroidType[i].partCount = ResRec(RoidRes, asteroidFileHand)->partCount;
                if (asteroidType[i].partCount < 0)
                    asteroidType[i].partCount = 0;
                asteroidType[i].color = ResRec(RoidRes, asteroidFileHand)->partColor;
                rgb.red = ((asteroidType[i].color >> 16) & 0xff) * 256;
                rgb.green = ((asteroidType[i].color >> 8) & 0xff) * 256;
                rgb.blue = (asteroidType[i].color & 0xff) * 256;
                if (gSpriteWorld->pixelDepth == 8) {
                    asteroidType[i].color = Color2Index(&rgb);
                } else {
                    asteroidType[i].color = (((rgb.red >> 11) << 10) & 0x7c00) |
                                  (((rgb.green >> 11) << 5) & 0x3e0) |
                                  ((rgb.blue >> 11) & 0x1f);
                }
                asteroidType[i].fragCount = ResRec(RoidRes, asteroidFileHand)->fragCount;
                if (asteroidType[i].fragCount < 0)
                    asteroidType[i].fragCount = 0;
                for (j = 0; j < 2; j++) {
                    asteroidType[i].fragType[j] = ResRec(RoidRes, asteroidFileHand)->fragType[j];
                    if (asteroidType[i].fragType[j] >= kFirstResourceID && asteroidType[i].fragType[j] < kFirstResourceID + kMaxAsteroids)
                        asteroidType[i].fragType[j] -= kFirstResourceID;
                    else if (asteroidType[i].fragType[j] < 0 || asteroidType[i].fragType[j] >= kMaxAsteroids)
                        asteroidType[i].fragType[j] = -1;
                }
                asteroidType[i].explodType = ResRec(RoidRes, asteroidFileHand)->explodType;
                if (asteroidType[i].explodType < 0 ||
                    (asteroidType[i].explodType >= kMaxExplodTypes && asteroidType[i].explodType < 1000) ||
                    asteroidType[i].explodType >= 1000 + kMaxExplodTypes)
                    asteroidType[i].explodType = -1;
                asteroidType[i].mass = ResRec(RoidRes, asteroidFileHand)->mass;
                UnlockAHandle(asteroidFileHand);
                AdjustHandleSize(asteroidFileHand, 0x28);
                ResourcePurge(asteroidFileHand);
                ReleaseResource(asteroidFileHand);
                LoadingIntroMusic();
            }
        }
    }

    /* ---------------- cölr: interface colors ---------------- */
    colorFileHand = GetIndResource(kColrType, 1);
    if (colorFileHand != NULL) {
        short k;
        int c;

        LockAHandle(colorFileHand);
        for (c = 0; c < 2; c++) {
            menuTextColor[c].blue = (ResRec(ColrRes, colorFileHand)->menuColor[c] & 0xff) * 256;
            menuTextColor[c].green = ((ResRec(ColrRes, colorFileHand)->menuColor[c] >> 8) & 0xff) * 256;
            menuTextColor[c].red = ((ResRec(ColrRes, colorFileHand)->menuColor[c] >> 16) & 0xff) * 256;
        }
        for (c = 0; c < 2; c++) {
            shopGridColor[c].blue = (ResRec(ColrRes, colorFileHand)->gridColor[c] & 0xff) * 256;
            shopGridColor[c].green = ((ResRec(ColrRes, colorFileHand)->gridColor[c] >> 8) & 0xff) * 256;
            shopGridColor[c].red = ((ResRec(ColrRes, colorFileHand)->gridColor[c] >> 16) & 0xff) * 256;
        }
        for (k = 0; k < 3; k++) {
            progressColor[k].blue = (ResRec(ColrRes, colorFileHand)->progressColor[k] & 0xff) * 256;
            progressColor[k].green = ((ResRec(ColrRes, colorFileHand)->progressColor[k] >> 8) & 0xff) * 256;
            progressColor[k].red = ((ResRec(ColrRes, colorFileHand)->progressColor[k] >> 16) & 0xff) * 256;
        }
        for (k = 0; k < 3; k++) {
            buttonColor[k].blue = (ResRec(ColrRes, colorFileHand)->buttonColor[k] & 0xff) * 256;
            buttonColor[k].green = ((ResRec(ColrRes, colorFileHand)->buttonColor[k] >> 8) & 0xff) * 256;
            buttonColor[k].red = ((ResRec(ColrRes, colorFileHand)->buttonColor[k] >> 16) & 0xff) * 256;
        }
        for (k = 0; k < kMainButtons; k++) {
            menuButtonPos[k].h = ResRec(ColrRes, colorFileHand)->buttonPos[k * 2];
            menuButtonPos[k].v = ResRec(ColrRes, colorFileHand)->buttonPos[k * 2 + 1];
        }
        floatingBorderColor.blue = (ResRec(ColrRes, colorFileHand)->floatingMapColor & 0xff) * 256;
        floatingBorderColor.green = ((ResRec(ColrRes, colorFileHand)->floatingMapColor >> 8) & 0xff) * 256;
        floatingBorderColor.red = ((ResRec(ColrRes, colorFileHand)->floatingMapColor >> 16) & 0xff) * 256;
        escortMenuColor.blue = (ResRec(ColrRes, colorFileHand)->escortHiliteColor & 0xff) * 256;
        escortMenuColor.green = ((ResRec(ColrRes, colorFileHand)->escortHiliteColor >> 8) & 0xff) * 256;
        escortMenuColor.red = ((ResRec(ColrRes, colorFileHand)->escortHiliteColor >> 16) & 0xff) * 256;
#ifdef MACBUILD
        ldefColors[0][kLdefBlue] = (ResRec(ColrRes, colorFileHand)->listColor[0] & 0xff) * 256;
        ldefColors[0][kLdefGreen] = ((ResRec(ColrRes, colorFileHand)->listColor[0] >> 8) & 0xff) * 256;
        ldefColors[0][kLdefRed] = ((ResRec(ColrRes, colorFileHand)->listColor[0] >> 16) & 0xff) * 256;
        ldefColors[1][kLdefBlue] = (ResRec(ColrRes, colorFileHand)->listColor[1] & 0xff) * 256;
        ldefColors[1][kLdefGreen] = ((ResRec(ColrRes, colorFileHand)->listColor[1] >> 8) & 0xff) * 256;
        ldefColors[1][kLdefRed] = ((ResRec(ColrRes, colorFileHand)->listColor[1] >> 16) & 0xff) * 256;
        ldefColors[2][kLdefBlue] = (ResRec(ColrRes, colorFileHand)->listColor[2] & 0xff) * 256;
        ldefColors[2][kLdefGreen] = ((ResRec(ColrRes, colorFileHand)->listColor[2] >> 8) & 0xff) * 256;
        ldefColors[2][kLdefRed] = ((ResRec(ColrRes, colorFileHand)->listColor[2] >> 16) & 0xff) * 256;
#else
        ldefColors[0].blue = (ResRec(ColrRes, colorFileHand)->listColor[0] & 0xff) * 256;
        ldefColors[0].green = ((ResRec(ColrRes, colorFileHand)->listColor[0] >> 8) & 0xff) * 256;
        ldefColors[0].red = ((ResRec(ColrRes, colorFileHand)->listColor[0] >> 16) & 0xff) * 256;
        ldefColors[1].blue = (ResRec(ColrRes, colorFileHand)->listColor[1] & 0xff) * 256;
        ldefColors[1].green = ((ResRec(ColrRes, colorFileHand)->listColor[1] >> 8) & 0xff) * 256;
        ldefColors[1].red = ((ResRec(ColrRes, colorFileHand)->listColor[1] >> 16) & 0xff) * 256;
        ldefColors[2].blue = (ResRec(ColrRes, colorFileHand)->listColor[2] & 0xff) * 256;
        ldefColors[2].green = ((ResRec(ColrRes, colorFileHand)->listColor[2] >> 8) & 0xff) * 256;
        ldefColors[2].red = ((ResRec(ColrRes, colorFileHand)->listColor[2] >> 16) & 0xff) * 256;
#endif
#ifdef MACBUILD
        progressDefRect = ResRec(ColrRes, colorFileHand)->progressBar;
#else
        progressDefRect.top = ResRec(ColrRes, colorFileHand)->progressBar.top;
        progressDefRect.left = ResRec(ColrRes, colorFileHand)->progressBar.left;
        progressDefRect.bottom = ResRec(ColrRes, colorFileHand)->progressBar.bottom;
        progressDefRect.right = ResRec(ColrRes, colorFileHand)->progressBar.right;
#endif
        strcpy((char *)name, ResRec(ColrRes, colorFileHand)->menuFont);
        CtoPstr((char *)name);
        GetFNum(name, &menuFontID);
        menuTextSize = ResRec(ColrRes, colorFileHand)->menuFontSize;
        strcpy((char *)name, ResRec(ColrRes, colorFileHand)->buttonFont);
        CtoPstr((char *)name);
        GetFNum(name, &buttonFontID);
        buttonFontSize = ResRec(ColrRes, colorFileHand)->buttonFontSize;
        logoDestPoint.h = ResRec(ColrRes, colorFileHand)->logoPos[0];
        logoDestPoint.v = ResRec(ColrRes, colorFileHand)->logoPos[1];
        rolloverDestPoint.h = ResRec(ColrRes, colorFileHand)->rolloverPos[0];
        rolloverDestPoint.v = ResRec(ColrRes, colorFileHand)->rolloverPos[1];
        for (k = 0; k < 3; k++) {
            slideDestPoint[k].h = ResRec(ColrRes, colorFileHand)->slidePos[k * 2];
            slideDestPoint[k].v = ResRec(ColrRes, colorFileHand)->slidePos[k * 2 + 1];
        }
        AdjustHandleSize(colorFileHand, 0xf4);
        UnlockAHandle(colorFileHand);
        ResourcePurge(colorFileHand);
        ReleaseResource(colorFileHand);
        LoadingIntroMusic();
    }

    /* ---------------- l33t: easter egg ---------------- */
    if (CountResources(kLeetType) > 0)
        leetMode = 1;
}
#endif


/* LoadDescription. STATE: MATCH 100%, 188/188, byte-identical yes (masked relocs, by hand;
   relocregcheck 0 of 40).
   OPEN: none. textFileHand's VA is unproven (no other MATCHed body names it). see notes */
// FUNCTION: EVNOVA 0x004c6d50
// MAC: _LoadDescription i386 0x000717fe ppc 0x0007a284
void LoadDescription(char *dst, short id)
{
    short size;
    short count;
    int p;

    textFileHand = GetResource(kDescType, id);
    if (textFileHand != NULL) {
        extern char *strcpy(char *dst, const char *src); /* 0x004d6010 MSL; block-scope keeps THIS block's two calls out of
                                                            line -- the else arm's strcpy(dst, "") inlines */
#ifdef MACBUILD
        HNoPurge(textFileHand);
#endif
        strcpy(parseBuffer, *textFileHand);
        size = strlen(*textFileHand);
        if (GetAHandleSize(textFileHand) > size) {
            descResourcePictId = ResRec(DescRes, textFileHand)->text[size + 1] << 8;
            descResourcePictId += ResRec(DescRes, textFileHand)->text[size + 2];

            descMovieName[0] = 0;
#if V111 /* V111 spelling */
            count = 0;
            p = size + 3;
            do {
                if (ResRec(DescRes, textFileHand)->text[p] == 0)
                    break;
                descMovieName[0]++;
                descMovieName[count + 1] = ResRec(DescRes, textFileHand)->text[p];
                count++;
                p++;
            } while (count < 0x20);
#else
            for (count = 0; count < 0x20; count++) {
                if ((*textFileHand)[size + 3 + count] == 0)
                    break;
                descMovieName[0]++;
                descMovieName[count + 1] = (*textFileHand)[size + 3 + count];
            }
#endif

            descFlags = ResRec(DescRes, textFileHand)->text[size + 0x23] << 8;
            descFlags += ResRec(DescRes, textFileHand)->text[size + 0x24];
        } else {
#ifdef MACBUILD
            pstrcpy(descMovieName, "\p");
#else
            pstrcpy(descMovieName, (unsigned char *)"");
#endif
            descResourcePictId = 0;
            descFlags = 0;
        }

#ifdef MACBUILD
        HPurge(textFileHand);
#endif
        ReleaseResource(textFileHand);
        descRankGovt[0] = -1;
        descRankGovt[1] = -1;

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
#if V111 /* V111 spelling */
        memset(dst, 0, 4);
        pstrcpy(descMovieName, "\p");
#elif defined(MACBUILD)
        strcpy(dst, "");
        pstrcpy(descMovieName, "\p");
#else
        strcpy(dst, "");
        pstrcpy(descMovieName, (unsigned char *)"");
#endif
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
// MAC: _LoadStrings i386 0x00071af5 ppc 0x0007a634
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
        pstrcpy(legalStatusStrings[i], buf);
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
// MAC: _LoadPluginString i386 0x00071a8e ppc 0x0007a5b0
Boolean LoadPluginString(short id, void *dst)
#ifdef MACBUILD
{
    Handle h;

    h = GetResource(kStringType, id);
    if (h != NULL) {
        HNoPurge(h);
        BlockMoveData(*h, dst, GetHandleSize(h));
        HPurge(h);
        ReleaseResource(h);
        return 1;
    }
    return 0;
}
#else
{
    Handle h;

    h = GetResource(kStringType, id);
    if (h == NULL)
        return 0;
    memcpy(dst, *h, GetAHandleSize(h));
    ReleaseResource(h);
    return 1;
}
#endif

#ifdef MAC1010
#define kPrefsResType 0x4d70a8c4        /* 'Mp' 0xa8 0xc4, MacRoman */
#define kPrefsFileType 0x4e70a8c4       /* 'Np' 0xa8 0xc4 */
#define kPrefsCreator 0x4e9a768a        /* 'N' 0x9a 'v' 0x8a */
#define kPrefsResID 128
#define kPrefsSig1010 0x68
#define kPrefsRecSize1010 0x8c

void LoadPrefs(void)
{
    DateTimeRec dtr;
    FSSpec spec;
    Str255 name;
    OSErr err;
    short sig;
    short i;

    GetTime(&dtr);
    GetIndString(name, kPrefsStrList, 1);
    err = FSMakeFSSpec(prefsVRefNum, prefsDirID, name, &spec);
    if (err == fnfErr) {
        SetDefaultPrefs();
        SetDefaultKeys();
    } else if (err == noErr) {
        gPrefsRefNum = FSpOpenResFile(&spec, fsRdWrPerm);
        if (gPrefsRefNum != -1) {
            gPrefsHandle = NULL;
            gPrefsHandle = (PrefsRecord **)GetResource(kPrefsResType, kPrefsResID);
            if (gPrefsHandle != NULL) {
                HNoPurge((Handle)gPrefsHandle);
                HLock((Handle)gPrefsHandle);
                settings.introMusic = 0;
                settings.voicesOn = 0;
                settings.clipping = 0;
                settings.noMovies = 0;
                settings.noDebris = 0;
                settings.parallaxStars = 0;
                settings.yieldEvents = 0;
                settings.windowed = 0;
                settings.ambientSounds = 0;
                settings.showMapBorders = 0;
                settings.noHyperGamma = 0;
                settings.noVersionCheck = 0;
                settings.shipAnimations = 0;
                settings.engineGlows = 0;
                settings.weaponGlows = 0;
                settings.runningLights = 0;
                sig = (*gPrefsHandle)->sig;
                if (sig == kPrefsSig1010) {
                    if ((*gPrefsHandle)->musicEnabled != 0)
                        settings.introMusic = 1;
                    if ((*gPrefsHandle)->unk4 != 0)
                        settings.voicesOn = 1;
                    if ((*gPrefsHandle)->earlyOut != 0)
                        settings.noMovies = 1;
                    if ((*gPrefsHandle)->noDebris != 0)
                        settings.noDebris = 1;
                    if ((*gPrefsHandle)->windowed != 0)
                        settings.windowed = 1;
                    if ((*gPrefsHandle)->loadShipSets != 0)
                        settings.shipAnimations = 1;
                    if ((*gPrefsHandle)->loadShipGlow != 0)
                        settings.engineGlows = 1;
                    if ((*gPrefsHandle)->loadShipWeapons != 0)
                        settings.weaponGlows = 1;
                    if ((*gPrefsHandle)->loadShipLights != 0)
                        settings.runningLights = 1;
                    if ((*gPrefsHandle)->parallaxStars != 0)
                        settings.parallaxStars = 1;
                    if ((*gPrefsHandle)->planetSounds != 0)
                        settings.ambientSounds = 1;
                    if ((*gPrefsHandle)->showInfluence != 0)
                        settings.showMapBorders = 1;
                    if ((*gPrefsHandle)->noHyperGamma != 0)
                        settings.noHyperGamma = 1;
                    if ((*gPrefsHandle)->noVersionCheck != 0)
                        settings.noVersionCheck = 1;
                    if ((*gPrefsHandle)->unk64 != 0)
                        settings.yieldEvents = 1;
                    settings.volume = (*gPrefsHandle)->volume;
                    DAT_00735488 = (*gPrefsHandle)->unk60 != 0;
                    settings.windowedPending = settings.windowed;
                    SetDefaultKeys();
                    for (i = 0; i <= 35; i++)
                        newKeyPrefs[i] = (*gPrefsHandle)->keys[i];
                    SlotTableToKeyPrefs();
                    DAT_00735450 = (*gPrefsHandle)->unk50;
                    DAT_00735450 *= 0.01;
                }
                HUnlock((Handle)gPrefsHandle);
                HPurge((Handle)gPrefsHandle);
                CloseResFile(gPrefsRefNum);
                if (sig != kPrefsSig1010) {
                    GetIndString(name, kPrefsStrList, 1);
                    if (FSMakeFSSpec(prefsVRefNum, prefsDirID, name, &spec) == noErr) {
                        if (FSpDelete(&spec) == noErr) {
                            SetDefaultPrefs();
                            SetDefaultKeys();
                            FadeScreenIn();
                            GetIndString(name, kAlertStrList, 0x20);
                            EVNoteAlert(name);
                        } else {
                            FadeScreenIn();
                            GetIndString(name, kAlertStrList, 0x21);
                            EVNoteAlert(name);
                            ShowTheMenuBar();
                            ST_Close();
                            ExitToShell();
                        }
                    } else {
                        FadeScreenIn();
                        GetIndString(name, kAlertStrList, 0x21);
                        EVNoteAlert(name);
                        ShowTheMenuBar();
                        ST_Close();
                        ExitToShell();
                    }
                }
            } else {
                CloseResFile(gPrefsRefNum);
                SetDefaultPrefs();
                SetDefaultKeys();
            }
        } else {
            SetDefaultPrefs();
            SetDefaultKeys();
        }
    } else {
        SetDefaultPrefs();
        SetDefaultKeys();
    }
}

void SavePrefs(void)
{
    FSSpec spec;
    Str255 name;
    short savedResFile;
    short i;

    gPrefsHandle = NULL;
    savedResFile = CurResFile();
    GetIndString(name, kPrefsStrList, 1);
    if (FSMakeFSSpec(prefsVRefNum, prefsDirID, name, &spec) == fnfErr) {
        FSpCreateResFile(&spec, kPrefsCreator, kPrefsFileType, smRoman);
        gPrefsRefNum = FSpOpenResFile(&spec, fsRdWrPerm);
        if (gPrefsRefNum == -1)
            return;
    } else if (FSpDelete(&spec) == noErr) {
        FSpCreateResFile(&spec, kPrefsCreator, kPrefsFileType, smRoman);
        gPrefsRefNum = FSpOpenResFile(&spec, fsRdWrPerm);
        if (gPrefsRefNum == -1)
            return;
    } else
        return;
    gPrefsHandle = (PrefsRecord **)NewHandle(kPrefsRecSize1010);
    if (gPrefsHandle != NULL) {
        HNoPurge((Handle)gPrefsHandle);
        HLock((Handle)gPrefsHandle);
        (*gPrefsHandle)->sig = kPrefsSig1010;
        if (settings.introMusic != 0)
            (*gPrefsHandle)->musicEnabled = 1;
        else
            (*gPrefsHandle)->musicEnabled = 0;
        if (settings.voicesOn != 0)
            (*gPrefsHandle)->unk4 = 1;
        else
            (*gPrefsHandle)->unk4 = 0;
        if (settings.noMovies != 0)
            (*gPrefsHandle)->earlyOut = 1;
        else
            (*gPrefsHandle)->earlyOut = 0;
        if (settings.noDebris != 0)
            (*gPrefsHandle)->noDebris = 1;
        else
            (*gPrefsHandle)->noDebris = 0;
        if (settings.windowedPending != 0)
            (*gPrefsHandle)->windowed = 1;
        else
            (*gPrefsHandle)->windowed = 0;
        if (settings.shipAnimations != 0)
            (*gPrefsHandle)->loadShipSets = 1;
        else
            (*gPrefsHandle)->loadShipSets = 0;
        if (settings.engineGlows != 0)
            (*gPrefsHandle)->loadShipGlow = 1;
        else
            (*gPrefsHandle)->loadShipGlow = 0;
        if (settings.weaponGlows != 0)
            (*gPrefsHandle)->loadShipWeapons = 1;
        else
            (*gPrefsHandle)->loadShipWeapons = 0;
        if (settings.runningLights != 0)
            (*gPrefsHandle)->loadShipLights = 1;
        else
            (*gPrefsHandle)->loadShipLights = 0;
        if (settings.yieldEvents != 0)
            (*gPrefsHandle)->unk64 = 1;
        else
            (*gPrefsHandle)->unk64 = 0;
        if (settings.parallaxStars != 0)
            (*gPrefsHandle)->parallaxStars = 1;
        else
            (*gPrefsHandle)->parallaxStars = 0;
        if (settings.ambientSounds != 0)
            (*gPrefsHandle)->planetSounds = 1;
        else
            (*gPrefsHandle)->planetSounds = 0;
        if (settings.showMapBorders != 0)
            (*gPrefsHandle)->showInfluence = 1;
        else
            (*gPrefsHandle)->showInfluence = 0;
        if (settings.noHyperGamma != 0)
            (*gPrefsHandle)->noHyperGamma = 1;
        else
            (*gPrefsHandle)->noHyperGamma = 0;
        if (settings.noVersionCheck != 0)
            (*gPrefsHandle)->noVersionCheck = 1;
        else
            (*gPrefsHandle)->noVersionCheck = 0;
        (*gPrefsHandle)->volume = settings.volume;
        KeyPrefsToSlotTable();
        for (i = 0; i <= 35; i++)
            (*gPrefsHandle)->keys[i] = newKeyPrefs[i];
        (*gPrefsHandle)->unk50 = (100.0 * DAT_00735450);
        if (DAT_00735488 != 0)
            (*gPrefsHandle)->unk60 = 1;
        else
            (*gPrefsHandle)->unk60 = 0;
        for (i = 0; i <= 5; i++)
            (*gPrefsHandle)->reserved[i] = 0;
        UseResFile(gPrefsRefNum);
        AddResource((Handle)gPrefsHandle, kPrefsResType, kPrefsResID, "\pEV Prefs");
        UseResFile(savedResFile);
        HUnlock((Handle)gPrefsHandle);
        HPurge((Handle)gPrefsHandle);
    }
    UpdateResFile(gPrefsRefNum);
    CloseResFile(gPrefsRefNum);
    FlushVol(NULL, prefsVRefNum);
}
#endif
#ifndef MACBUILD /* the mac keeps its preferences in EVPrefsDialogWrapper.o */
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
    gPrefsHandle = LoadAFileHandle(path);
    if (gPrefsHandle != NULL) {
        LockAHandle((Handle)gPrefsHandle);
        settings.introMusic = 0;
        settings.voicesOn = 0;
        settings.clipping = 0;
        settings.noMovies = 0;
        settings.noDebris = 0;
        settings.parallaxStars = 0;
        settings.yieldEvents = 0;
        settings.windowed = 0;
        settings.ambientSounds = 0;
        settings.showMapBorders = 0;
        settings.noHyperGamma = 0;
        settings.noVersionCheck = 0;
        settings.shipAnimations = 0;
        settings.engineGlows = 0;
        settings.weaponGlows = 0;
        settings.runningLights = 0;
        sig = (*gPrefsHandle)->sig;
        if (sig == kPrefsSig) {
            if ((*gPrefsHandle)->musicEnabled != 0)
                settings.introMusic = 1;
            if ((*gPrefsHandle)->unk4 != 0)
                settings.voicesOn = 1;
            if ((*gPrefsHandle)->earlyOut != 0)
                settings.noMovies = 1;
            if ((*gPrefsHandle)->noDebris != 0)
                settings.noDebris = 1;
#ifndef CEFIX /* F04.05 */
            if ((*gPrefsHandle)->windowed != 0)
                settings.windowed = 1;
#endif
            if ((*gPrefsHandle)->loadShipSets != 0)
                settings.shipAnimations = 1;
            if ((*gPrefsHandle)->loadShipGlow != 0)
                settings.engineGlows = 1;
            if ((*gPrefsHandle)->loadShipWeapons != 0)
                settings.weaponGlows = 1;
            if ((*gPrefsHandle)->loadShipLights != 0)
                settings.runningLights = 1;
            if ((*gPrefsHandle)->parallaxStars != 0)
                settings.parallaxStars = 1;
            if ((*gPrefsHandle)->planetSounds != 0)
                settings.ambientSounds = 1;
            if ((*gPrefsHandle)->showInfluence != 0)
                settings.showMapBorders = 1;
            if ((*gPrefsHandle)->noHyperGamma != 0)
                settings.noHyperGamma = 1;
            if ((*gPrefsHandle)->noVersionCheck != 0)
                settings.noVersionCheck = 1;
            if ((*gPrefsHandle)->unk64 != 0)
                settings.yieldEvents = 1;
            settings.volume = (*gPrefsHandle)->volume;
            settings.gamma = (*gPrefsHandle)->gamma;
            if (settings.gamma > 6)
                settings.gamma = 3;
            if ((*gPrefsHandle)->unk60 == 0)
                DAT_00735488 = 0;
            else
                DAT_00735488 = 1;
            settings.windowedPending = settings.windowed;
            SetDefaultKeys();
            for (i = 0; i <= 35; i++)
                newKeyPrefs[i] = (*gPrefsHandle)->keys[i];
            SlotTableToKeyPrefs();
            DAT_00735450 = (*gPrefsHandle)->unk50;
            DAT_00735450 *= 0.01;
        }
        UnlockAHandle((Handle)gPrefsHandle);
        DeallocAHandle(gPrefsHandle);
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
        if (settings.introMusic != 0)
            (*gPrefsHandle)->musicEnabled = 1;
        else
            (*gPrefsHandle)->musicEnabled = 0;
        if (settings.voicesOn != 0)
            (*gPrefsHandle)->unk4 = 1;
        else
            (*gPrefsHandle)->unk4 = 0;
        if (settings.noMovies != 0)
            (*gPrefsHandle)->earlyOut = 1;
        else
            (*gPrefsHandle)->earlyOut = 0;
        if (settings.noDebris != 0)
            (*gPrefsHandle)->noDebris = 1;
        else
            (*gPrefsHandle)->noDebris = 0;
        if (settings.windowedPending != 0)
            (*gPrefsHandle)->windowed = 1;
        else
            (*gPrefsHandle)->windowed = 0;
        if (settings.shipAnimations != 0)
            (*gPrefsHandle)->loadShipSets = 1;
        else
            (*gPrefsHandle)->loadShipSets = 0;
        if (settings.engineGlows != 0)
            (*gPrefsHandle)->loadShipGlow = 1;
        else
            (*gPrefsHandle)->loadShipGlow = 0;
        if (settings.weaponGlows != 0)
            (*gPrefsHandle)->loadShipWeapons = 1;
        else
            (*gPrefsHandle)->loadShipWeapons = 0;
        if (settings.runningLights != 0)
            (*gPrefsHandle)->loadShipLights = 1;
        else
            (*gPrefsHandle)->loadShipLights = 0;
        if (settings.yieldEvents != 0)
            (*gPrefsHandle)->unk64 = 1;
        else
            (*gPrefsHandle)->unk64 = 0;
        if (settings.parallaxStars != 0)
            (*gPrefsHandle)->parallaxStars = 1;
        else
            (*gPrefsHandle)->parallaxStars = 0;
        if (settings.ambientSounds != 0)
            (*gPrefsHandle)->planetSounds = 1;
        else
            (*gPrefsHandle)->planetSounds = 0;
        if (settings.showMapBorders != 0)
            (*gPrefsHandle)->showInfluence = 1;
        else
            (*gPrefsHandle)->showInfluence = 0;
        if (settings.noHyperGamma != 0)
            (*gPrefsHandle)->noHyperGamma = 1;
        else
            (*gPrefsHandle)->noHyperGamma = 0;
        if (settings.noVersionCheck != 0)
            (*gPrefsHandle)->noVersionCheck = 1;
        else
            (*gPrefsHandle)->noVersionCheck = 0;
        (*gPrefsHandle)->volume = settings.volume;
        (*gPrefsHandle)->gamma = settings.gamma;
        KeyPrefsToSlotTable();
        for (i = 0; i <= 35; i++)
            (*gPrefsHandle)->keys[i] = newKeyPrefs[i];
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
        DeallocAHandle(gPrefsHandle);
    } else {
        void *f;

        f = FOpenExpandedPath(path, "wb");
        if (f != NULL)
            fclose(f);
    }
}
#endif

/* MakeAlias. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7d40
// MAC: _MakeAlias i386 0x00071e36 ppc 0x0007a9bc
#if defined(MACBUILD) && V111
void MakeAlias(const FSRef *target)
{
    AliasHandle alias;
    FinderInfo info;
    FSRef aliasRef;
    Str255 aliasName;
    HFSUniStr255 uniName;
    HFSUniStr255 forkName;
    short savedResFile;
    short refNum;

    savedResFile = CurResFile();
    GetIndString(aliasName, kPilotDirList, 4);
    HFSNameGetUnicodeName(aliasName, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&pilotsFolder, uniName.length, uniName.unicode,
                           kTextEncodingUnknown, &aliasRef) == noErr)
        FSDeleteObject(&aliasRef);
    FSGetResourceForkName(&forkName);
    FSCreateResourceFile(&pilotsFolder, uniName.length, uniName.unicode, kFSCatInfoNone, NULL,
                         forkName.length, forkName.unicode, &aliasRef, NULL);
    FSNewAlias(NULL, target, &alias);
    if (alias != NULL) {
        FSGetFinderInfo(target, &info, NULL, NULL);
        refNum = EVOpenResourceFile(&aliasRef, 0);
        if (refNum != -1) {
            UseResFile(refNum);
            AddResource((Handle)alias, rAliasType, 0, aliasName);
            WriteResource((Handle)alias);
            CloseResFile(refNum);
            info.file.finderFlags = kIsAlias;
            FSSetFinderInfo(&aliasRef, &info, NULL);
        }
        UseResFile(savedResFile);
    }
}
#elif !defined(MACBUILD)
void MakeAlias(char *name)
{
    char base[0x100];
    char path[0x800];

    GetIniIndString(base, kPilotDirList, 4);
    strcpy(path, pilotsFolder);
    strcat(path, base);
    SaveAFile(path, name, strlen(name) + 1);
}
#endif

/* mac twin _SavePilotFile. 0x004c7dd0 demonstrably WRITES the .plt (see
   WritePilotData), so the older `LoadPilot' / `LoadPilotFile' names were
   both wrong. */

/* SavePilotFile. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c7db0
// MAC: _SavePilotFile i386 0x00076876 ppc 0x0007f6b4
short SavePilotFile(short stellar)
#ifdef MAC1010
{
    FSSpec spec;
    OSErr err;

    if (noSavePilotFiles != 0)
        return 0;
    err = FSMakeFSSpec(pilotsVRefNum, pilotsDirID, playerName, &spec);
    if (err == noErr) {
        err = FSpDelete(&spec);
        if (err == noErr) {
            err = FSMakeFSSpec(pilotsVRefNum, pilotsDirID, playerName, &spec);
            if (err == fnfErr)
                err = WritePilotData(stellar);
        }
    } else if (err == fnfErr)
        err = WritePilotData(stellar);
    return err;
}
#elif defined(MACBUILD)
{
    FSRef ref;
    FSRef tmpRef;
    UniChar tmpName[255];
    HFSUniStr255 uniName;
    CFStringRef name;
    CFMutableStringRef tmpNameStr;
    OSErr err;

    if (noSavePilotFiles != 0)
        return 0;
    HFSNameGetUnicodeName(playerName, kTextEncodingUnknown, &uniName);
    err = FSMakeFSRefUnicode(&pilotsFolder, uniName.length, uniName.unicode, kTextEncodingUnknown,
                             &ref);
    if (err == noErr) {
        name = CFStringCreateWithCharacters(kCFAllocatorDefault, uniName.unicode, uniName.length);
        if (name == NULL)
            return -1;
        tmpNameStr = CFStringCreateMutableCopy(kCFAllocatorDefault, uniName.length + 10, name);
        CFStringAppend(tmpNameStr, CFSTR("-tmp"));
        CFStringGetCharacters(tmpNameStr, CFRangeMake(0, CFStringGetLength(tmpNameStr)), tmpName);
        FSMakeFSRefUnicode(&pilotsFolder, CFStringGetLength(tmpNameStr), tmpName,
                           kTextEncodingUnknown, &tmpRef);
        FSDeleteObject(&tmpRef);
        err = FSMakeFSRefUnicode(&pilotsFolder, CFStringGetLength(tmpNameStr), tmpName,
                                 kTextEncodingUnknown, &ref);
        if (err == fnfErr)
            err = WritePilotData(stellar);
        CFRelease(name);
        CFRelease(tmpNameStr);
    } else if (err == fnfErr)
        err = WritePilotData(stellar);
    return err;
}
#else
{
    if (noSavePilotFiles != 0)
        return 0;
    return WritePilotData(stellar);
}
#endif

#ifdef MAC1010
#define recSlot i
#define CopyMissionSlot(dstObj, srcObj, dstMsn, srcMsn) \
    dstObj = srcObj; \
    dstMsn = srcMsn
#elif V111 /* V111 spelling */
#define recSlot j
#define CopyMissionSlot(dstObj, srcObj, dstMsn, srcMsn) \
    dstObj = srcObj; \
    dstMsn = srcMsn
#else
#define recSlot i
#define CopyMissionSlot(dstObj, srcObj, dstMsn, srcMsn) \
    os = &srcObj; \
    od = &dstObj; \
    od->active = os->active; \
    od->returnDone = os->returnDone; \
    od->objectivesDone = os->objectivesDone; \
    od->failed = os->failed; \
    od->flags = os->flags; \
    od->dueDate = os->dueDate; \
    ms = &srcMsn; \
    md = &dstMsn; \
    md->travelStellar = ms->travelStellar; \
    md->unk2 = ms->unk2; \
    md->returnStellar = ms->returnStellar; \
    md->shipCount = ms->shipCount; \
    md->shipDude = ms->shipDude; \
    md->shipGoal = ms->shipGoal; \
    md->shipBehav = ms->shipBehav; \
    md->shipStart = ms->shipStart; \
    md->shipSyst = ms->shipSyst; \
    md->cargoType = ms->cargoType; \
    md->cargoQty = ms->cargoQty; \
    md->pickupMode = ms->pickupMode; \
    md->dropoffMode = ms->dropoffMode; \
    md->scanMask = ms->scanMask; \
    md->compGovt = ms->compGovt; \
    md->compReward = ms->compReward; \
    md->datePostIncrement = ms->datePostIncrement; \
    md->payment = ms->payment; \
    md->specialShipsKilled = ms->specialShipsKilled; \
    md->specialShipsBoarded = ms->specialShipsBoarded; \
    md->specialShipsDisabled = ms->specialShipsDisabled; \
    md->specialShipsJumpedIn = ms->specialShipsJumpedIn; \
    md->specialShipsJumpedOut = ms->specialShipsJumpedOut; \
    md->initialShipCount = ms->initialShipCount; \
    md->canAbort = ms->canAbort; \
    md->hasCargo = ms->hasCargo; \
    md->unk34 = ms->unk34; \
    md->briefDesc = ms->briefDesc; \
    md->quickBriefDesc = ms->quickBriefDesc; \
    md->loadCargoDesc = ms->loadCargoDesc; \
    md->dropCargoDesc = ms->dropCargoDesc; \
    md->compDesc = ms->compDesc; \
    md->failDesc = ms->failDesc; \
    md->refuseDesc = ms->refuseDesc; \
    md->shipDoneDesc = ms->shipDoneDesc; \
    md->timeLimit = ms->timeLimit; \
    md->destStrList = ms->destStrList; \
    md->destStrIndex = ms->destStrIndex; \
    md->shipDelay = ms->shipDelay; \
    md->misnIndex = ms->misnIndex; \
    md->shipStrList = ms->shipStrList; \
    md->shipStrIndex = ms->shipStrIndex; \
    md->shipTypeIndex = ms->shipTypeIndex; \
    md->flags = ms->flags; \
    md->flags2 = ms->flags2; \
    *(UnsignedWide *)md->unk59 = *(UnsignedWide *)ms->unk59; \
    md->auxShipCount = ms->auxShipCount; \
    md->auxShipDude = ms->auxShipDude; \
    md->auxShipSyst = ms->auxShipSyst; \
    md->auxShipsSpawned = ms->auxShipsSpawned; \
    md->auxShipDelay = ms->auxShipDelay; \
    md->auxShipsLeft = ms->auxShipsLeft; \
    *(MStr64 *)md->shipName = *(MStr64 *)ms->shipName; \
    *(MStr64 *)md->shipSubtitle = *(MStr64 *)ms->shipSubtitle; \
    *(MStr255 *)md->unkED = *(MStr255 *)ms->unkED; \
    *(MStr255 *)md->beginSetString = *(MStr255 *)ms->beginSetString; \
    *(MStr255 *)md->setString2 = *(MStr255 *)ms->setString2; \
    *(MStr255 *)md->succSetString = *(MStr255 *)ms->succSetString; \
    *(MStr255 *)md->failSetString = *(MStr255 *)ms->failSetString; \
    *(MStr255 *)md->abortSetString = *(MStr255 *)ms->abortSetString; \
    *(MStr255 *)md->shipDoneSetString = *(MStr255 *)ms->shipDoneSetString; \
    *(MStr256 *)md->name = *(MStr256 *)ms->name
#endif

/* WritePilotData. STATE: MATCH 100% (1367 exact, 356 reloc), 1723/1723, byte-identical yes
   (8380 bytes, relocated fields masked), relocregcheck 0 of 356. OPEN: none. see notes */
// FUNCTION: EVNOVA 0x004c7dd0
// MAC: _WritePilotData i386 0x00071ffb ppc 0x0007ab1c
short WritePilotData(short stellar)
{
#ifdef MAC1010 /* 1.0.10 real: 0x1002e224 FSMakeFSSpec, 0x1002e244 FSpCreateResFile, no FSRef */
    FSSpec spec;
    short savedResFile;
#elif defined(MACBUILD)
    HFSUniStr255 uniName;
    HFSUniStr255 forkName;
    FSRef tmpRef;
    UniChar tmpUni[265];
    short savedResFile;
    CFStringRef nameStr;
    CFMutableStringRef tmpNameStr;
#endif
    char name[0x100];
    char path[0x800];
    void *f;
    MissionState *od;
    MissionState *os;
    MissionRecord *md;
    MissionRecord *ms;
    RGBColor *paint;
#ifdef MAC1010
    Boolean sig[5];
    short j;
#elif V111
    Boolean sig[5];
    int j;
#else
    KeyCheckFlag sig[4];
#endif
    unsigned long long key;
    short i;

    if (noUpdatePilotData != 0)
        return 0;

    playerFileHand = NULL;
    altPlayerFileHand = NULL;
#ifdef MACBUILD
#ifdef MAC1010
    savedResFile = CurResFile();
    FSMakeFSSpec(pilotsVRefNum, pilotsDirID, playerName, &spec);
    FSpCreateResFile(&spec, kNovaCreator, kPilotFileType, smRoman);
    pilotResNum = FSpOpenResFile(&spec, fsRdWrPerm);
#else
    HFSNameGetUnicodeName(playerName, kTextEncodingUnknown, &uniName);
    nameStr = CFStringCreateWithCharacters(kCFAllocatorDefault, uniName.unicode, uniName.length);
    if (nameStr == NULL)
        return -1;
    tmpNameStr = CFStringCreateMutableCopy(kCFAllocatorDefault, uniName.length + 10, nameStr);
    CFStringAppend(tmpNameStr, CFSTR("-tmp"));
    CFStringGetCharacters(tmpNameStr, CFRangeMake(0, CFStringGetLength(tmpNameStr)), tmpUni);
    savedResFile = CurResFile();
    FSGetResourceForkName(&forkName);
    FSCreateResourceFile(&pilotsFolder, CFStringGetLength(tmpNameStr), tmpUni, kFSCatInfoNone,
                         NULL, forkName.length, forkName.unicode, &tmpRef, NULL);
    FSChangeCreatorType(&tmpRef, kNovaCreator, kPilotFileType);
    pilotResNum = EVOpenResourceFile(&tmpRef, fsRdWrPerm);
#endif
    if (pilotResNum == -1)
        return kPilotUnreadable;
    playerFileHand = (PilotRec **)NewHandle(sizeof(PilotRec));
    altPlayerFileHand = (PilotRec2 **)NewHandle(sizeof(PilotRec2));
    if (playerFileHand != NULL && altPlayerFileHand != NULL) {
        HLock((Handle)playerFileHand);
        HLock((Handle)altPlayerFileHand);
        HNoPurge((Handle)playerFileHand);
        HNoPurge((Handle)altPlayerFileHand);
#else
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
#endif

        (*playerFileHand)->stellar = stellar;
        (*playerFileHand)->shipType = shipData[0].type;

        key = RT3_GetLicenseCode();
#ifdef MAC1010
        KeyMismatch(sig[0], key, key, 0xb3deefe6846a1331ULL, 0x00e5b573ac69e57aULL);
        {
            unsigned long long want;
            unsigned long long got;

            want = 0x01ca504ced18b085ULL;
            got = 0x01175258fee9b4f0ULL;
            got = key ^ got;
            got <<= 4;
            got >>= 4;
            want <<= 4;
            want >>= 4;
            sig[1] = want != got;
        }
        KeyMismatch(sig[2], key, key, 0xd9eab6006f7c65a8ULL, 0x0935b4147cad71dfULL);
        KeyMismatch(sig[3], key, key, 0x847ba0013f907235ULL, 0x0f02fa941793847fULL);
        KeyMismatch(sig[4], key, key, 0xf6170bf185359c3dULL, 0x068887b12ff1e429ULL);
        for (i = 0; i < 5; i++) {
            if (sig[i] == 0)
                (*playerFileHand)->shipType = 0;
        }
#elif V111
        sig[0] = KeyLow60Bits(key ^ 0xb3deefe6846a1331ULL) != 0x00e5b573ac69e57aULL;
        sig[1] = KeyLow60Bits(key ^ 0x01175258fee9b4f0ULL) != 0x01ca504ced18b085ULL;
        sig[2] = KeyLow60Bits(key ^ 0xd9eab6006f7c65a8ULL) != 0x0935b4147cad71dfULL;
        sig[3] = KeyLow60Bits(key ^ 0x847ba0013f907235ULL) != 0x0f02fa941793847fULL;
        sig[4] = KeyLow60Bits(key ^ 0xf6170bf185359c3dULL) != 0x068887b12ff1e429ULL;
        for (j = 0; j < 5; j++) {
            if (sig[j] == 0)
                (*playerFileHand)->shipType = 0;
        }
#else
        sig[0] = ((key ^ 0xb3deefe6846a1331ULL) & kKeyMask)
                     != 0x00e5b573ac69e57aULL;
        sig[1] = ((key ^ 0x01175258fee9b4f0ULL) & kKeyMask)
                     != 0x01ca504ced18b085ULL;
        sig[2] = ((key ^ 0xd9eab6006f7c65a8ULL) & kKeyMask)
                     != 0x0935b4147cad71dfULL;
        sig[3] = ((key ^ 0x847ba0013f907235ULL) & kKeyMask)
                     != 0x0f02fa941793847fULL;
        for (i = 0; i < 4; i++) {
            if (sig[i] == 0)
                (*playerFileHand)->shipType = 0;
        }
#endif

#ifdef MAC1010
        for (i = 0; i <= 5; i++)
            (*playerFileHand)->cargo[i] = shipData[0].cargo[i];
#else
        i = 0;
        do {
            (*playerFileHand)->cargo[i] = shipData[0].cargo[i];
        } while (++i < kCargoTypes);
#endif
        (*playerFileHand)->shield = shipData[0].shield;
        (*playerFileHand)->fuel = shipData[0].fuel;
        (*playerFileHand)->month = gameTime.month;
        (*playerFileHand)->day = gameTime.day;
        (*playerFileHand)->year = gameTime.year;
        for (recSlot = 0; recSlot < kMaxSystems; recSlot++)
            (*playerFileHand)->explore[recSlot] = systemData[recSlot].exploreLevel;
        for (recSlot = 0; recSlot < kMaxOutfits; recSlot++)
            (*playerFileHand)->outfits[recSlot] = shipSystems[recSlot];
        for (recSlot = 0; recSlot < kMaxSystems; recSlot++)
            (*playerFileHand)->legalStatus[recSlot] = playerRecord[recSlot];
        for (recSlot = 0; recSlot < kMaxWeapTypes; recSlot++) {
            (*playerFileHand)->weapCount[recSlot] = shipData[0].weap[recSlot].count;
            (*playerFileHand)->weapAmmo[recSlot] = shipData[0].weap[recSlot].ammo;
        }
        (*playerFileHand)->credits = shipData[0].credits;
#ifdef MAC1010 /* 1.0.10 spelling */
        for (j = 0; j < kMaxActiveMissions; j++) {
            CopyMissionSlot((*playerFileHand)->objectives[j], currentMissionObjectives[j],
                            (*playerFileHand)->missions[j], currentMissions[j]);
        }
#else
        for (i = 0; i < kMaxActiveMissions; i++) {
            CopyMissionSlot((*playerFileHand)->objectives[i], currentMissionObjectives[i],
                            (*playerFileHand)->missions[i], currentMissions[i]);
        }
#endif
        for (i = 0; i < kMaxMissionBits; i++)
            (*playerFileHand)->missionBits[i] = missionBit[i];
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].prepared != 0)
                (*playerFileHand)->spobFlag[i] = stellarData[i].dominated;
            else
                (*playerFileHand)->spobFlag[i] = 0;
        }
        for (i = 0; i < kMaxShips; i++) {
            (*playerFileHand)->escortType[i] = -1;
            (*playerFileHand)->fighterType[i] = -1;
            (*playerFileHand)->fighterVoice[i] = -1;
            (*playerFileHand)->escortB[i] = 0;
            (*playerFileHand)->escortA[i] = 0;
        }
        for (i = 0; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType == 5)
                (*playerFileHand)->fighterType[i] = shipData[i].type;
            else
                (*playerFileHand)->fighterType[i] = -1;
        }
        for (i = 0; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType == kShipTypeEscort && shipData[i].specialMission == -1) {
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
        for (i = 0; i < kMaxShips; i++) {
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
#if V111 || defined(MACBUILD) /* V111 spelling */
        (*altPlayerFileHand)->paint = playerPaint;
#else
        paint = &(*altPlayerFileHand)->paint;
        paint->red = playerPaint.red;
        paint->green = playerPaint.green;
        paint->blue = playerPaint.blue;
#endif
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0) {
                (*altPlayerFileHand)->defWave[i] = stellarData[i].defWaveSize;
                (*altPlayerFileHand)->tribute[i] = stellarData[i].daysDominated;
            } else {
                (*altPlayerFileHand)->defWave[i] = stellarData[i].defWaveSize;
                (*altPlayerFileHand)->tribute[i] = 0;
            }
        }
        for (i = 0; i < kMaxPersons; i++) {
            if (personData[i].loaded != 0 && personData[i].aiType >= 1) {
                (*altPlayerFileHand)->persActive[i] = 1;
                if (personData[i].grudge != 0)
                    (*altPlayerFileHand)->persB[i] = 1;
                else
                    (*altPlayerFileHand)->persB[i] = 0;
            } else {
                (*altPlayerFileHand)->persActive[i] = 0;
                (*altPlayerFileHand)->persB[i] = 0;
            }
        }
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0)
                (*altPlayerFileHand)->rankOwned[i] = 1;
            else
                (*altPlayerFileHand)->rankOwned[i] = 0;
        }
        (*altPlayerFileHand)->seenIntro = seenIntroScreen;
        for (i = 0; i < kMaxDisasters; i++) {
            if (disasterData[i].loaded != 0) {
                (*altPlayerFileHand)->newsC[i] = disasterData[i].countdown;
                (*altPlayerFileHand)->newsB[i] = disasterData[i].stellarID;
            } else {
                (*altPlayerFileHand)->newsC[i] = 0;
                (*altPlayerFileHand)->newsB[i] = -1;
            }
        }
        for (i = 0; i < kMaxJunks; i++)
            (*altPlayerFileHand)->junkHeld[i] = junkData[i].held;
        for (i = 0; i < kMaxCrons; i++) {
            if (cronEvent[i].active != 0) {
                (*altPlayerFileHand)->cronA[i] = cronEvent[i].daysLeft;
                (*altPlayerFileHand)->cronB[i] = cronEvent[i].holdoffLeft;
            } else {
                (*altPlayerFileHand)->cronA[i] = -1;
                (*altPlayerFileHand)->cronB[i] = -1;
            }
        }
        for (i = 0; i < kMaxSystems; i++)
            (*altPlayerFileHand)->systState[i] = systemData[i].reinforceDaysLeft;
        for (i = 0; i < kMaxStellars; i++)
            (*altPlayerFileHand)->spobDead[i] = stellarData[i].deadDaysLeft;
        for (i = 0; i < 4; i++)
            (*altPlayerFileHand)->escortOrders[i] = playerEscortOrders[i];
        for (i = 0; i <= 1; i++) {
            (*altPlayerFileHand)->priceFlux[0][i] = globalPriceFlux[i];
            (*altPlayerFileHand)->priceFlux[1][i] = localPriceFlux[i];
        }
        strncpy((*altPlayerFileHand)->datePrefix, datePrefix, 0xf);
        strncpy((*altPlayerFileHand)->dateSuffix, dateSuffix, 0xf);
        (*altPlayerFileHand)->datePrefix[0xf] = 0;
        (*altPlayerFileHand)->dateSuffix[0xf] = 0;
        for (i = 0; i < 0x40; i++)
            (*altPlayerFileHand)->unk2006[i] = 0;
        for (i = 0; i < 0x400; i++)
            (*altPlayerFileHand)->unk5efe[i] = 0;

#ifdef MACBUILD
        strncpy(name, (char *)shipName, 0x20);
        UseResFile(pilotResNum);
#if TARGET_RT_LITTLE_ENDIAN
        FlipNpil(kPilotResID, *playerFileHand, GetHandleSize((Handle)playerFileHand), 1);
        FlipNpil(kPilotResID + 1, *altPlayerFileHand, GetHandleSize((Handle)altPlayerFileHand), 1);
#endif
#ifdef MAC1010
        DoEncryption((Handle)playerFileHand, kPilotCryptKey);
        DoEncryption((Handle)altPlayerFileHand, kPilotCryptKey);
#else
        DoEncryption(*playerFileHand, GetHandleSize((Handle)playerFileHand), kPilotCryptKey);
        DoEncryption(*altPlayerFileHand, GetHandleSize((Handle)altPlayerFileHand), kPilotCryptKey);
#endif
        AddResource((Handle)playerFileHand, kPilotFileType, kPilotResID, "\pPilot Data");
        AddResource((Handle)altPlayerFileHand, kPilotFileType, kPilotResID + 1, (unsigned char *)name);
        UseResFile(savedResFile);
        UpdateResFile(pilotResNum);
        HUnlock((Handle)playerFileHand);
        HUnlock((Handle)altPlayerFileHand);
        HPurge((Handle)playerFileHand);
        HPurge((Handle)altPlayerFileHand);
        CloseResFile(pilotResNum);
    }
#ifdef MAC1010
    MakeAlias(&spec);
    UseResFile(savedResFile);
    FlushVol(NULL, pilotsVRefNum);
#else
    FSMoveObjectSync(&tmpRef, &pilotsFolder, nameStr, NULL, kFSFileOperationOverwrite);
    CFRelease(tmpNameStr);
    CFRelease(nameStr);
    MakeAlias(&tmpRef);
    UseResFile(savedResFile);
#endif
    return 0;
#else
        PStr2CStr(name, shipName);
        LockAHandle((Handle)playerFileHand);
#ifndef CEFIX /* F02.01 */
        DoEncryption(*playerFileHand, GetAHandleSize((Handle)playerFileHand),
                     kPilotCryptKey);
#endif
        LockAHandle((Handle)altPlayerFileHand);
#ifndef CEFIX /* F02.02 */
        DoEncryption(*altPlayerFileHand, GetAHandleSize((Handle)altPlayerFileHand),
                     kPilotCryptKey);
#endif
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
#endif
}

#ifdef MAC1010
void MakeAlias(const FSSpec *target)
{
    AliasHandle alias;
    FInfo info;
    FSSpec aliasSpec;
    short refNum;
    short savedResFile;

    savedResFile = CurResFile();
    aliasSpec.vRefNum = target->vRefNum;
    aliasSpec.parID = target->parID;
    GetIndString(aliasSpec.name, kPilotDirList, 4);
    FSpDelete(&aliasSpec);
    NewAlias(NULL, target, &alias);
    if (alias != NULL) {
        FSpGetFInfo(target, &info);
        FSpCreateResFile(&aliasSpec, info.fdCreator, info.fdType, smRoman);
        refNum = FSpOpenResFile(&aliasSpec, fsCurPerm);
        if (refNum != -1) {
            UseResFile(refNum);
            AddResource((Handle)alias, rAliasType, 0, aliasSpec.name);
            WriteResource((Handle)alias);
            CloseResFile(refNum);
            info.fdFlags = kIsAlias;
            FSpSetFInfo(&aliasSpec, &info);
        }
        UseResFile(savedResFile);
    }
}
#endif

/* VideoGetCurrentMode fills a four-long display mode: width, height, depth, flags.
   mainscreen.c reads the same block the same way. */

/* OpenPilotFile (mac _OpenPilotFile). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004c9e90
// MAC: _OpenPilotFile i386 0x00076644 ppc 0x0007f498
Boolean OpenPilotFile(char *missingResource)
#ifdef MAC1010
{
#if TARGET_API_MAC_CARBON
    unsigned char text[256];
    Str255 name;
    Str255 path;
    FSSpec spec;
    short i;
    short result;
    Boolean good;

    good = 0;
    *missingResource = 0;
    crashlog("building file spec");
    GetIndString(name, kPilotDirList, 3);
    path[1] = ':';
    path[0] = name[0] + 1;
    for (i = 1; i <= name[0]; i++)
        path[i + 1] = name[i];
    FSMakeFSSpec(pilotsVRefNum, pilotsDirID, path, &spec);
    crashlog("showing nav services dialog");
    if (MyOpenDocument(&spec, &spec) == noErr)
        good = 1;
#else
    unsigned char text[256];
    StandardFileReply reply;
    FSSpec spec;
    SFTypeList types;
    short result;
    Boolean good;

    *missingResource = 0;
    LMSetCurDirStore(pilotsDirID);
    types[0] = kPilotFileType;
    StandardGetFile(NULL, 1, types, &reply);
    good = reply.sfGood;
    if (good) {
        if (FSMakeFSSpec(reply.sfFile.vRefNum, reply.sfFile.parID, reply.sfFile.name, &spec) != noErr)
            good = 0;
    }
#endif
    if (good) {
        crashlog("open file setup 1");
        DeleteAllComputerShips(1, 1);
        ResetPlayer(1);
        ResetPlayerRecord(1);
        DeleteAllComputerShips(1, 1);
        crashlog("loading pilot data");
        result = LoadPilotData(&spec);
        InitCursor();
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
        InitCursor();
        ShowTheCursor();
        return 0;
    }
}
#elif defined(MACBUILD)
{
    FSSpec spec;
    FSRef ref;
    unsigned char text[256];
    short result;
    Boolean good;

    *missingResource = 0;
    crashlog("building file spec");
    FSRefMakeFSSpec(&pilotsFolder, &spec);
    crashlog("showing nav services dialog");
    if (MyOpenDocument(&spec, &spec) != noErr)
        good = false;
    else
        good = true;
    if (good) {
        crashlog("open file setup 1");
        DeleteAllComputerShips(1, 1);
        ResetPlayer(1);
        ResetPlayerRecord(1);
        DeleteAllComputerShips(1, 1);
        crashlog("loading pilot data");
#ifdef MAC1010
        result = LoadPilotData(&spec);
#else
        FSpMakeFSRef(&spec, &ref);
        result = LoadPilotData(&ref);
#endif
        InitCursor();
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
        InitCursor();
        ShowTheCursor();
        return 0;
    }
}
#else
{
    OPENFILENAMEA ofn;
    int32_t mode[4];
    char initDir[0x800];
    char path[0x800];
    unsigned char text[256];
    int32_t result;
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
#ifndef CEFIX /* F24.01 */
    if ((mode[3] & kModeFlagOwned) != 0) {
        ReleaseScreenDC();
        SetDisplayToSize(mode[0], mode[1], mode[2], mode[3] & ~kModeFlagOwned);
        AcquireScreenDC();
        MainScreenUpdate(1);
        ScreenEndScene();
    }
#endif

    gotFile = GetOpenFileNameA(&ofn);

    while (Win95ApplsActive == 0) {
        Sleep(0);
        KeyboardKbhit();
    }

#ifndef CEFIX /* F24.02 */
    if ((mode[3] & kModeFlagOwned) != 0) {
        ReleaseScreenDC();
        VideoSetMode(mode);
        AcquireScreenDC();
    }
#endif

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
#endif

/* OpenLastPilotFile (mac _OpenLastPilotFile). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ca120
// MAC: _OpenLastPilotFile i386 0x000764ad ppc 0x0007f2fc
void OpenLastPilotFile(void)
#ifdef MACBUILD
{
#ifdef MAC1010 /* 1.0.10 real: 0x1002f524 FSMakeFSSpec, no FSRef */
    FSSpec spec;
    OSErr err;
#else
    FSRef ref;
#endif
    Str255 name;
    HFSUniStr255 uniName;
    short i;
    short j;

    GetIndString(name, kPilotDirList, 4);
    if (!DoesPilotExist(name))
        return;
    DeleteAllComputerShips(1, 1);
    ResetPlayer(1);
    ResetPlayerRecord(1);
    GetIndString(name, kPilotDirList, 4);
#ifdef MAC1010
    err = FSMakeFSSpec(pilotsVRefNum, pilotsDirID, name, &spec);
    if (err == noErr)
        err = LoadPilotData(&spec);
    if (err != 0)
        return;
#else
    HFSNameGetUnicodeName(name, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&pilotsFolder, uniName.length, uniName.unicode, kTextEncodingUnknown,
                           &ref) != noErr)
        return;
    if (LoadPilotData(&ref) != 0)
        return;
#endif
    gameInProgress = 1;
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0)
            ExploreNebulaeFromSystem(i);
    }
    for (j = 0; j < kMaxActiveMissions; j++) {
        if (currentMissionObjectives[j].active != 0) {
            if ((currentMissions[j].flags & kMissionInfiniteAuxShips) != 0)
                currentMissions[j].auxShipsLeft = currentMissions[j].auxShipCount;
            currentMissions[j].auxShipDelay = (Rand(0x46) + 0x46);
            currentMissions[j].auxShipsSpawned = 0;
        }
    }
}
#else
{
    char name[256];
    char path[0x800];
    uint32_t len;
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
            for (i = 0; i < kMaxSystems; i++) {
                if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0)
                    ExploreNebulaeFromSystem(i);
            }
            for (j = 0; j < kMaxActiveMissions; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissions[j].flags & kMissionInfiniteAuxShips) != 0)
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
#endif

/* CountEVPlugins counts+logs files of one extension in a directory (filescan.c) */

/* DumpPilotLog (mac _DumpPilotLog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ca2c0
// MAC: _DumpPilotLog i386 0x000739dc ppc 0x0007c8b0
#ifndef MACBUILD
void DumpPilotLog(void)
#include "../engine/dumppilotlog.inc"
#endif

/* LoadPilotData (mac _LoadPilotData). STATE: MATCH 100.0% (1279 exact, 379 reloc-equivalent);
   exe 1658 / obj 1658; byte-identical yes; relocregcheck 0 of 379.
   OPEN: none. */
// FUNCTION: EVNOVA 0x004cb260
// MAC: _LoadPilotData i386 0x000750b5 ppc 0x0007dd04
#ifdef MAC1010
short LoadPilotData(FSSpec *spec)
#elif defined(MACBUILD)
short LoadPilotData(const FSRef *ref)
#else
short LoadPilotData(char *path)
#endif
{
#ifdef MAC1010 /* 1.0.10 real: 0x1002f6b4 ResolveAliasFile, 0x1002f6e4 FSpOpenResFile, no HFSUniStr255 */
    Boolean targetIsFolder;
    Boolean wasAliased;
    short resID;
    ResType resType;
    Handle h;
#elif defined(MACBUILD)
    Boolean wasAliased;
    Boolean targetIsFolder;
    HFSUniStr255 uniName;
    Str255 hfsName;
    short resID;
    ResType resType;
    Handle h;
#endif
    char buf[0x40];
    Str255 text;
    void *f;
    MissionState *od;
    MissionState *os;
    MissionRecord *md;
    MissionRecord *ms;
    RGBColor *paint;
    char *tail;
    short i;
    short j;
    short k;
    short n;
    short err;
    uint32_t len;
    Boolean junk;
    int32_t parID;

    playerFileHand = NULL;
    altPlayerFileHand = NULL;
    junk = 0;

#ifdef MACBUILD
#ifdef MAC1010
    parID = spec->parID;
    if (ResolveAliasFile(spec, true, &targetIsFolder, &wasAliased) == noErr
        && !targetIsFolder && spec->parID == parID) {
    pilotResNum = FSpOpenResFile(spec, fsRdWrPerm);
    if (pilotResNum != -1) {
#else
    if (FSResolveAliasFile((FSRef *)ref, true, &targetIsFolder, &wasAliased) != noErr
        || targetIsFolder)
        goto badFile;
    pilotResNum = EVOpenResourceFile(ref, fsRdWrPerm);
    if (pilotResNum == -1)
        goto badFile;
#endif
    crashlog("load pilot data 1");
    playerFileHand = (PilotRec **)GetResource(kPilotFileType, kPilotResID);
    if (playerFileHand != NULL) {
        HNoPurge((Handle)playerFileHand);
#ifdef MAC1010
        err = DoEncryption((Handle)playerFileHand, kPilotCryptKey);
#else
        err = DoEncryption(*playerFileHand, GetHandleSize((Handle)playerFileHand), kPilotCryptKey);
#endif
        if (err == 0) {
#if TARGET_RT_LITTLE_ENDIAN
            FlipNpil(kPilotResID, *playerFileHand, GetHandleSize((Handle)playerFileHand), 0);
#endif
#else
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
#ifdef CEFIX /* F02.03 */
        err = CE_DecryptIfNecessary(*playerFileHand, GetAHandleSize((Handle)playerFileHand),
                                    kPilotCryptKey);
#else
        err = DoEncryption(*playerFileHand, GetAHandleSize((Handle)playerFileHand),
                           kPilotCryptKey);
#endif
        UnlockAHandle((Handle)playerFileHand);

        if (err == 0) {
#endif
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
            else if (systemData[shipData[0].curSystem].prepared == 0)
                shipData[0].curSystem = -1;
            if (shipData[0].curSystem < 0 || shipData[0].curSystem > kMaxSystems) {
                junk = 1;
                crashlog("warning: pilot file system ID out of range");
                shipData[0].curSystem = 0;
                for (i = 0; i < kMaxSystems; i++) {
                    if (systemData[i].prepared != 0) {
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
            mapScroll.h = systemData[shipData[0].curSystem].pos.h;
            mapScroll.v = systemData[shipData[0].curSystem].pos.v;

            shipData[0].type = (*playerFileHand)->shipType;
            if (shipClass[shipData[0].type].techLevel == kResAbsent) {
                junk = 1;
                crashlog("warning: pilot file ship class out of range");
                shipData[0].type = 0;
                for (i = 0; i < kMaxShipTypes; i++) {
                    if (shipClass[i].techLevel != kResAbsent) {
                        shipData[0].type = i;
                        break;
                    }
                }
            }

            for (i = 0; i <= 5; i++)
                shipData[0].cargo[i] = (*playerFileHand)->cargo[i];

            for (i = 0; i < kMaxOutfits; i++) {
                shipSystems[i] = (*playerFileHand)->outfits[i];
                if (shipSystems[i] > 0 && outfitSystem[i].techLevel == kOutfTechAbsent) {
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
                systemData[i].exploreLevel = (*playerFileHand)->explore[i];
            for (i = 0; i < kMaxSystems; i++)
                playerRecord[i] = (*playerFileHand)->legalStatus[i];

            for (i = 0; i < kMaxWeapTypes; i++) {
                shipData[0].weap[i].count = (*playerFileHand)->weapCount[i];
                shipData[0].weap[i].ammo = (*playerFileHand)->weapAmmo[i];
                if (shipData[0].weap[i].count > 0 && weaponStats[i].guidance == kResAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid weapon ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipData[0].weap[i].count = 0;
                }
                if (shipData[0].weap[i].ammo > 0 && weaponStats[i].guidance == kResAbsent) {
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
            for (i = 0; i < kMaxActiveMissions; i++) {
                CopyMissionSlot(currentMissionObjectives[i], (*playerFileHand)->objectives[i],
                                currentMissions[i], (*playerFileHand)->missions[i]);
                if (currentMissionObjectives[i].active != 0) {
                    SetMissionDeadline(&currentMissionObjectives[i].dueDate,
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
                    if ((currentMissions[i].flags & kMissionInfiniteAuxShips) != 0)
                        currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
                    currentMissions[i].auxShipDelay = (Rand(0x46) + 0x46);
                    currentMissions[i].auxShipsSpawned = 0;
                }
            }

#ifdef MAC1010
            for (i = 0; i < kMaxMissionBits; i++) {
                missionBit[i] = (*playerFileHand)->missionBits[i];
            }
#else
            i = 0;
            do {
                missionBit[i] = (*playerFileHand)->missionBits[i];
            } while (++i < kMaxMissionBits);
#endif

            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared != 0)
                    stellarData[i].dominated = (*playerFileHand)->spobFlag[i];
                else
                    stellarData[i].dominated = 0;
            }

            DeleteAllComputerShips(1, 1);

            for (i = 0; i < kMaxShips; i++) {
                if ((*playerFileHand)->escortType[i] >= 0) {
                    n = ((*playerFileHand)->escortType[i] % 1000);
                    if (shipClass[n].techLevel == kResAbsent) {
                        junk = 1;
                        crashlog("warning: pilot file has invalid escort ship type");
                    } else {
                        j = SpawnEscort(n, -1);
                        if (j != -1) {
                            if ((*playerFileHand)->escortType[i] < 1000) {
                                shipData[j].hired = 0;
                                if ((*playerFileHand)->escortB[i] != 0)
                                    shipData[j].sellOnLanding = 1;
                            } else {
                                shipData[j].hired = 1;
                            }
                            if ((*playerFileHand)->escortA[i] != 0)
                                shipData[j].upgradeOnLanding = 1;
                        }
                    }
                }
                if ((*playerFileHand)->fighterType[i] >= 0 &&
                    (*playerFileHand)->fighterType[i] < kMaxShipTypes) {
                    n = (*playerFileHand)->fighterType[i];
                    if (shipClass[n].techLevel == kResAbsent) {
                        junk = 1;
                        crashlog("warning: pilot file has invalid fighter ship type");
                    } else {
                        j = SpawnEscort(n, -1);
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

#ifdef MACBUILD
            HPurge((Handle)playerFileHand);
            ReleaseResource((Handle)playerFileHand);
#else
            DeallocAHandle(playerFileHand);
#endif
            crashlog("load pilot data 4");
        }
    } else {
#ifdef MAC1010
        CloseResFile(pilotResNum);
        return kPilotUnreadable;
#elif defined(MACBUILD)
        CloseResFile(pilotResNum);
        goto badFile;
#else
        fclose(f);
        return kPilotUnreadable;
#endif
    }

    crashlog("load pilot data 5");
#ifdef MACBUILD
    altPlayerFileHand = (PilotRec2 **)GetResource(kPilotFileType, kPilotResID + 1);
    if (altPlayerFileHand != NULL) {
        HNoPurge((Handle)altPlayerFileHand);
#ifdef MAC1010
        err = DoEncryption((Handle)altPlayerFileHand, kPilotCryptKey);
#else
        err = DoEncryption(*altPlayerFileHand, GetHandleSize((Handle)altPlayerFileHand),
                           kPilotCryptKey);
#endif
        if (err == 0) {
#if TARGET_RT_LITTLE_ENDIAN
            FlipNpil(kPilotResID + 1, *altPlayerFileHand,
                     GetHandleSize((Handle)altPlayerFileHand), 0);
#endif
            crashlog("load pilot data 6");
            if ((*altPlayerFileHand)->version == kBetaPilotVersion) {
                HUnlock((Handle)altPlayerFileHand);
                HPurge((Handle)altPlayerFileHand);
                CloseResFile(pilotResNum);
                return kPilotBetaFile;
            }
            if ((*altPlayerFileHand)->version < kMinPilotVersion) {
                HUnlock((Handle)altPlayerFileHand);
                HPurge((Handle)altPlayerFileHand);
                CloseResFile(pilotResNum);
                return kPilotWrongVersion;
            }
#else
    len = fgetlong(f);
    if (len > 0) {
        altPlayerFileHand = AllocAHandle2(len, 0);
        LockAHandle((Handle)altPlayerFileHand);
        fread(*altPlayerFileHand, len, 1, f);
        UnlockAHandle((Handle)altPlayerFileHand);
    }
    if (altPlayerFileHand != NULL) {
        LockAHandle((Handle)altPlayerFileHand);
#ifdef CEFIX /* F02.04 */
        err = CE_DecryptIfNecessary(*altPlayerFileHand,
                                    GetAHandleSize((Handle)altPlayerFileHand), kPilotCryptKey);
#else
        err = DoEncryption(*altPlayerFileHand, GetAHandleSize((Handle)altPlayerFileHand),
                           kPilotCryptKey);
#endif
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
#endif

            if ((*altPlayerFileHand)->strict == 1)
                strictPlay = 1;
            else
                strictPlay = 0;
            if ((*altPlayerFileHand)->gender == 1)
                playerGender = 1;
            else
                playerGender = 0;

            strncpy((char *)playerNickname, (*altPlayerFileHand)->nickname, 0x40);
#if V111 || defined(MACBUILD) /* V111 spelling */
            playerPaint = (*altPlayerFileHand)->paint;
#else
            paint = &(*altPlayerFileHand)->paint;
            playerPaint.red = paint->red;
            playerPaint.green = paint->green;
            playerPaint.blue = paint->blue;
#endif

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
                if (cronEvent[i].loaded != 0) {
                    cronEvent[i].daysLeft = (*altPlayerFileHand)->cronA[i];
                    cronEvent[i].holdoffLeft = (*altPlayerFileHand)->cronB[i];
                    if (cronEvent[i].daysLeft >= 0 || cronEvent[i].holdoffLeft >= 0)
                        cronEvent[i].active = 1;
                    else
                        cronEvent[i].active = 0;
                } else {
                    cronEvent[i].active = 0;
                }
            }

#ifdef MAC1010
            for (i = 0; i < kMaxSystems; i++) {
                systemData[i].reinforceDaysLeft = (*altPlayerFileHand)->systState[i];
            }
#else
            i = 0;
            do {
                systemData[i].reinforceDaysLeft = (*altPlayerFileHand)->systState[i];
            } while (++i < kMaxSystems);
#endif

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
                        playerEscortOrders[shipClass[shipData[i].type].escortType];
            }

#ifdef MAC1010
            for (i = 0; i <= 1; i++) {
                globalPriceFlux[i] = (*altPlayerFileHand)->priceFlux[0][i];
                localPriceFlux[i] = (*altPlayerFileHand)->priceFlux[1][i];
            }
#else
            i = 0;
            do {
                globalPriceFlux[i] = (*altPlayerFileHand)->priceFlux[0][i];
                localPriceFlux[i] = (*altPlayerFileHand)->priceFlux[1][i];
            } while (++i < 2);
#endif

            strncpy(datePrefix, (*altPlayerFileHand)->datePrefix, 0xf);
            strncpy(dateSuffix, (*altPlayerFileHand)->dateSuffix, 0xf);
            datePrefix[0xf] = 0;
            dateSuffix[0xf] = 0;

#ifdef MACBUILD
            HPurge((Handle)altPlayerFileHand);
            ReleaseResource((Handle)altPlayerFileHand);
#else
            DeallocAHandle(altPlayerFileHand);
#endif
            crashlog("load pilot data 8");
        }
    } else {
#ifdef MAC1010
        CloseResFile(pilotResNum);
        return kPilotUnreadable;
#elif defined(MACBUILD)
        CloseResFile(pilotResNum);
        goto badFile;
#else
        fclose(f);
        return kPilotUnreadable;
#endif
    }

    crashlog("load pilot data 9");
#ifdef MACBUILD
    h = GetResource(kPilotFileType, kPilotResID + 1);
    if (h != NULL) {
        HNoPurge(h);
        GetResInfo(h, &resID, &resType, text);
        strncpy((char *)shipName, (char *)text, 0x40);
        HPurge(h);
        ReleaseResource(h);
    }
#ifdef MAC1010
    strncpy((char *)playerName, (char *)spec->name, 0x1f);
    CloseResFile(pilotResNum);

    crashlog("making alias");
    MakeAlias(spec);
#else
    FSGetCatalogInfo(ref, kFSCatInfoNone, NULL, &uniName, NULL, NULL);
    UnicodeNameGetHFSName(uniName.length, uniName.unicode, kTextEncodingUnknown, false, hfsName);
    strncpy((char *)playerName, (char *)hfsName, 0x1f);
    CloseResFile(pilotResNum);

    crashlog("making alias");
    MakeAlias(ref);
#endif
#else
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
#endif

    shipData[0].foldDir = 0;
    shipData[0].frameTimer = 0;
    shipData[0].foldSet = shipClass[shipData[0].type].baseSets - 1;
    shipData[0].shieldGlow = 0;
    shipData[0].weapGlow = 0;
    shipData[0].annoyance = 0;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0) {
            if ((shipClass[shipData[i].type].shanFlags & kShanUnfoldsWhenFiring) != 0)
                shipData[i].foldSet =
                    shipClass[shipData[0].type].baseSets - 1;
            else
                shipData[i].foldSet = 0;
        }
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        shipClass[i].dailyHireRoll = (Rand(kDailyRollRange) + 1);
        shipClass[i].dailySaleRoll = (Rand(kDailyRollRange) + 1);
    }
    for (i = 0; i < kMaxOutfits; i++)
        outfitSystem[i].dailyRoll = (Rand(kDailyRollRange) + 1);

    crashlog("dumping pilot log");
    DumpPilotLog();

#ifdef MAC1010
    } else
        return kPilotUnreadable;
    } else
        return kPilotUnreadable;
    return junk != 0 ? (short)kPilotHasJunk : (short)kPilotOK;
#elif defined(MACBUILD)
    return junk != 0 ? kPilotHasJunk : kPilotOK;

badFile:
    return kPilotUnreadable;
#else
    if (junk == 0)
        goto ok;
    return kPilotHasJunk; /* BUG: f is not closed on this path, only the junk == 0 path reaches fclose */

badFile:
    return kPilotUnreadable;

ok:
    fclose(f);
    return kPilotOK;
#endif
}

/* DoesPilotExist. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd030
// MAC: _DoesPilotExist i386 0x00072def ppc 0x0007bc7c
#ifdef MAC1010
Boolean DoesPilotExist(ConstStr255Param name)
{
    short vRefNum;
    int32_t dirID;
    FSSpec spec;
    Str255 pname;
    OSErr err;

    if (HGetVol(pname, &vRefNum, &dirID) == noErr) {
        pstrcpy(pname, (unsigned char *)name);
        err = FSMakeFSSpec(pilotsVRefNum, pilotsDirID, pname, &spec);
        if (err == fnfErr)
            return 0;
        if (err == noErr)
            return 1;
    }
    return 0;
}
#elif defined(MACBUILD)
Boolean DoesPilotExist(ConstStr255Param name)
{
    FSRef ref;
    Str255 pname;
    HFSUniStr255 uniName;

    pstrcpy(pname, (unsigned char *)name);
    HFSNameGetUnicodeName(pname, kTextEncodingUnknown, &uniName);
    return FSMakeFSRefUnicode(&pilotsFolder, uniName.length, uniName.unicode,
                              kTextEncodingUnknown, &ref) == noErr;
}
#else
Boolean DoesPilotExist(const char *path)
{
    return DoesFileExist(path);
}
#endif

/* DeletePilotFile. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd040
// MAC: _DeletePilotFile i386 0x00072e6b ppc 0x0007bcec
#ifdef MAC1010
void DeletePilotFile(ConstStr255Param name)
{
    FSSpec spec;
    Str255 pname;

    if (!DoesPilotExist(name))
        return;
    pstrcpy(pname, (unsigned char *)name);
    if (FSMakeFSSpec(pilotsVRefNum, pilotsDirID, pname, &spec) == noErr)
        FSpDelete(&spec);
}
#elif defined(MACBUILD)
void DeletePilotFile(ConstStr255Param name)
{
    FSRef ref;
    Str255 pname;
    HFSUniStr255 uniName;

    if (!DoesPilotExist(name))
        return;
    pstrcpy(pname, (unsigned char *)name);
    HFSNameGetUnicodeName(pname, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&pilotsFolder, uniName.length, uniName.unicode,
                           kTextEncodingUnknown, &ref) == noErr)
        FSDeleteObject(&ref);
}
#else
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
#endif

#define kEBugLastField 0x1f
typedef struct {
    short flag[kEBugLastField + 1];
} EBugRec;
#if V111
/* the ebug resource of the last EVBugResourceFieldTest */
Handle bugHandle;
#endif

#define kCharRecordSize 0x16a

/* one debug switch per field index; absent resource or out-of-range index = off */
/* EVBugResourceFieldTest (mac _EVBugResourceFieldTest). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd0b0
// MAC: _EVBugResourceFieldTest i386 0x0007132c ppc 0x00079d48
Boolean EVBugResourceFieldTest(short field)
{
    Handle h;
    Boolean on;

    on = 0;
    if (field < 0 || field > kEBugLastField)
        return 0;
#if V111
    bugHandle = GetResource(kEBugType, 128);
    h = bugHandle;
#else
    h = GetResource(kEBugType, 128);
#endif
    if (h != NULL) {
        if ((*(EBugRec **)h)->flag[field] != 0)
            on = 1;
        ReleaseResource(h);
    }
    return on;
}

/* The memset is inlined by mwcc (rep stosb): it needs -opt level=4,intrinsics with <string.h>
   included. */
#ifdef MAC1010
OSErr GetSubfolderOfFile(short refNum, ConstStr255Param name, int32_t *dirID, short *vRefNum)
{
    FCBPBRec fcb;
    FSSpec spec;
    Boolean wasAliased;
    Boolean isFolder;
    Str255 fileName;
    CInfoPBRec *pb;
    OSErr err;
    OSErr aliasErr;

    fcb.ioRefNum = refNum;
    fcb.ioNamePtr = fileName;
    fcb.ioFCBIndx = 0;
    err = PBGetFCBInfoSync(&fcb);
    if (err == noErr) {
        fileName[0] = 0;
        pb = (CInfoPBRec *)NewPtrClear(sizeof(CInfoPBRec));
        if (pb != NULL) {
            spec.vRefNum = fcb.ioFCBVRefNum;
            spec.parID = fcb.ioFCBParID;
            BlockMove(name, spec.name, name[0] + 1);
            BlockMove(name, fileName, name[0] + 1);
            aliasErr = ResolveAliasFile(&spec, true, &isFolder, &wasAliased);
            if (aliasErr == noErr && wasAliased) {
                pb->dirInfo.ioNamePtr = spec.name;
                pb->dirInfo.ioVRefNum = spec.vRefNum;
                pb->dirInfo.ioDrDirID = spec.parID;
            } else {
                pb->dirInfo.ioNamePtr = fileName;
                pb->dirInfo.ioVRefNum = fcb.ioFCBVRefNum;
                pb->dirInfo.ioDrDirID = fcb.ioFCBParID;
            }
            pb->dirInfo.ioFDirIndex = 0;
            pb->dirInfo.ioCompletion = NULL;
            err = PBGetCatInfoSync(pb);
            if (err == noErr) {
                *dirID = pb->dirInfo.ioDrDirID;
                *vRefNum = pb->dirInfo.ioVRefNum;
            }
            DisposePtr((Ptr)pb);
        }
    }
    return err;
}
#endif

/* AdjustHandleSize (mac _AdjustHandleSize). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd100
// MAC: _AdjustHandleSize i386 0x00076bca ppc 0x0007f9fc
void AdjustHandleSize(Handle h, int32_t newSize)
#ifdef MACBUILD
{
    int32_t oldSize;
    Handle temp;

    if (h == NULL)
        return;
    if (GetHandleSize(h) == newSize)
        return;
    oldSize = GetHandleSize(h);
    temp = NewHandleClear(newSize);
    if (temp == NULL)
        return;
    HLock(temp);
    HLock(h);
    BlockMove(*h, *temp, oldSize < newSize ? oldSize : newSize);
    HUnlock(h);
    SetHandleSize(h, newSize);
    HLock(h);
    BlockMove(*temp, *h, newSize);
    HUnlock(h);
    HUnlock(temp);
    DisposeHandle(temp);
    ChangedResource(h);
    WriteResource(h);
}
#else
{
    int32_t size;

    if (h == NULL)
        return;
    size = GetAHandleSize(h);
    if (size != newSize) {
        ResizeAHandle(h, newSize);
        if (size < newSize)
            memset(*h + size, 0, newSize - size);
    }
}
#endif

/* leaves the buffer empty if the list is missing or has no entries */
/* GetRandomIndString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd140
// MAC: _GetRandomIndString i386 0x00072ef7 ppc 0x0007bd74
void GetRandomIndString(unsigned char *str, short strListID)
#ifdef MACBUILD
{
    Handle h;
    short count;

    str[0] = 0;
    h = GetResource(kStringListType, strListID);
    if (h != NULL) {
        HLock(h);
        BlockMove(*h, &count, sizeof(short));
        HUnlock(h);
        HPurge(h);
        ReleaseResource(h);
        if (count > 0)
            GetIndString(str, strListID, Rand(count) + 1);
    }
}
#else
{
    Handle h;
    short count;

    str[0] = 0;
    h = GetResource(kStringListType, strListID);
    if (h != NULL) {
        count = SwapEndianShort(&ResRec(StringListRec, h)->count);
        ReleaseResource(h);
        if (count > 0)
            GetIndString(str, strListID, Rand(count) + 1);
    }
}
#endif

/* StrcatIndCString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd1a0
// MAC: _StrcatIndCString i386 0x00072f90 ppc 0x0007be0c
void StrcatIndCString(char *dst, short strListID, short index)
{
    unsigned char str[256];

    GetIndString(str, strListID, index);
    PtoCstr(str);
    strcat(dst, (char *)str);
}

/* DrawIndString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd1f0
// MAC: _DrawIndString i386 0x00072fd8 ppc 0x0007be50
void DrawIndString(short strListID, short index)
{
    unsigned char str[256];

    GetIndString(str, strListID, index);
    DrawString(str);
}

#ifdef MACBUILD
/* DeleteResourceByID (mac _DeleteResourceByID): mac only. */
// MAC: _DeleteResourceByID i386 0x00073011 ppc 0x0007be84
OSErr DeleteResourceByID(ResType type, short id)
{
    short savedResFile;
    short homeFile;
    Handle h;

    savedResFile = CurResFile();
    h = GetResource(type, id);
    if (h != NULL) {
        homeFile = HomeResFile(h);
        UseResFile(homeFile);
        RemoveResource(h);
        UpdateResFile(homeFile);
        UseResFile(savedResFile);
    }
    return ResError();
}

/* DeleteResourceType (mac _DeleteResourceType): mac only. */
// MAC: _DeleteResourceType i386 0x0007307f ppc 0x0007bef4
void DeleteResourceType(ResType type)
{
    short i;
    short savedResFile;
    short curFile;
    short homeFile;
    Handle h;

    curFile = savedResFile = CurResFile();
    for (i = 1; CountResources(type) > 0; i++) {
        h = GetIndResource(type, i);
        if (h != NULL) {
            homeFile = HomeResFile(h);
            if (curFile != homeFile) {
                UpdateResFile(curFile);
                curFile = homeFile;
                UseResFile(homeFile);
            }
            RemoveResource(h);
            DisposeHandle(h);
        }
    }
    UpdateResFile(curFile);
    UseResFile(savedResFile);
}

/* navEventHandler (mac _navEventHandler): mac only. Lifts every window to the shielding level
   while the navigation dialog runs. */
// MAC: _navEventHandler i386 0x00073123 ppc 0x0007bf9c
void navEventHandler(NavEventCallbackMessage selector, NavCBRecPtr parms, void *userData)
{
    int count;
    int *windows;
    int i;

    CGSGetWindowCount(_CGSDefaultConnection(), _CGSDefaultConnection(), &count);
    windows = malloc(count * sizeof(int));
    CGSGetWindowList(_CGSDefaultConnection(), _CGSDefaultConnection(), count, windows, &count);
    for (i = 0; i < count; i++)
        CGSSetWindowLevel(_CGSDefaultConnection(), windows[i], CGShieldingWindowLevel());
    free(windows);
}

/* MyOpenDocument (mac _MyOpenDocument): mac only. The pilot-file open dialog. */
// MAC: _MyOpenDocument i386 0x000731c5 ppc 0x0007c058
#ifndef MAC1010
OSErr MyOpenDocument(const FSSpec *defaultSpec, FSSpec *outSpec)
{
    NavDialogCreationOptions options;
    AEDesc defaultLocation;
    NavTypeListHandle typeList;
    int32_t count;
    NavReplyRecord reply;
    DescType actualType;
    int32_t index;
    Size actualSize;
    NavDialogRef dialog;
    AEKeyword keyword;
    FSSpec spec;
    OSErr err;

    err = NavGetDefaultDialogCreationOptions(&options);
    if (err == noErr) {
        options.optionFlags |= kNavNoTypePopup | kNavSelectDefaultLocation;
        options.optionFlags ^= kNavAllowPreviews | kNavAllowMultipleFiles;
        err = AECreateDesc(typeFSS, defaultSpec, sizeof(FSSpec), &defaultLocation);
        if (err == noErr) {
            typeList = (NavTypeListHandle)NewHandleClear(sizeof(NavTypeList) + sizeof(OSType));
            HLock((Handle)typeList);
            (*typeList)->componentSignature = kNovaCreator;
            (*typeList)->osTypeCount = 1;
            (*typeList)->osType[0] = kPilotFileType;
            HUnlock((Handle)typeList);
            err = NavCreateGetFileDialog(&options, typeList, NULL, NULL, NULL, NULL, &dialog);
            NavCustomControl(dialog, kNavCtlSetLocation, &defaultLocation);
            NavDialogRun(dialog);
            NavDialogGetReply(dialog, &reply);
            DisposeHandle((Handle)typeList);
            if (err == noErr && reply.validRecord) {
                err = AECountItems(&reply.selection, &count);
                if (err == noErr) {
                    for (index = 1; index <= count; index++) {
                        if (AEGetNthPtr(&reply.selection, index, typeFSS, &keyword, &actualType,
                                        &spec, sizeof(FSSpec), &actualSize) == noErr)
                            memcpy(outSpec, &spec, sizeof(FSSpec));
                    }
                }
                err = NavDisposeReply(&reply);
            } else
                err = fnfErr;
            NavDialogDispose(dialog);
            ReleaseResource((Handle)typeList);    /* BUG: the handle was disposed above */
            AEDisposeDesc(&defaultLocation);
        }
    }
    return err;
}
#endif
#endif

/*
 * Pascal resource names carry a ';'-introduced comment (Nova Bible
 * convention); truncate at the last one and trim the spaces before it.
 */
/* TrimResourceName. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd230
// MAC: _TrimResourceName i386 0x000717ac ppc 0x0007a208
void TrimResourceName(unsigned char *name)
{
    short i;
    short len;
    Boolean seen;
    int c;

    seen = 0;
    len = *name;
    for (i = *name; i >= 1; i--) {
        if (seen == 0) {
            c = name[i];
            if ((unsigned)c == ';') {
                seen = 1;
                len = i - 1;
            }
        } else {
            c = name[i];
            if ((unsigned)c == ' ') {
                len = i - 1;
            } else if ((unsigned)c != ';') {
                len = i;
                break;
            }
        }
    }
    *name = len;
}

/* the New Pilot screen starts on whichever character carries the flag */
/* GetDefaultCharResourceName. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd290
// MAC: _GetDefaultCharResourceName i386 0x00077100 ppc 0x00080030
void GetDefaultCharResourceName(unsigned char *name)
{
    short i;
    short count;
    short id;
    uint32_t type;

    count = CountResources(kCharType);
#ifdef MACBUILD
    pstrcpy(name, "\p");
#else
    pstrcpy(name, (unsigned char *)"");
#endif
    for (i = 0; i < count; i++) {
        if (name[0] == 0) {
            /* cleared first so a failed load leaves no stale handle behind */
            charFileHand = NULL;
            charFileHand = GetIndResource(kCharType, (i + 1));
            if (charFileHand != NULL) {
                if ((ResRec(CharRecord, charFileHand)->flags & kCharIsDefault) != 0)
                    GetResInfo(charFileHand, &id, &type, name);
                AdjustHandleSize(charFileHand, kCharRecordSize);
                ReleaseResource(charFileHand);
            }
        }
    }
}

#define kIntroSlides    4
#define kMaxIntroDelay  300

/* GetCharResourceShipType. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004cd350
// MAC: _GetCharResourceShipType i386 0x000770a1 ppc 0x0007ffb4
short GetCharResourceShipType(const unsigned char *name)
{
    short index;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
        index = ResRec(CharRecord, charFileHand)->shipType;
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
// MAC: _GetCharResourceIntroIDs i386 0x00076fa9 ppc 0x0007fe5c
void GetCharResourceIntroIDs(const unsigned char *name, short *introText, short *pics,
                  short *delays)
{
    short i;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
        *introText = ResRec(CharRecord, charFileHand)->introTextID;
        for (i = 0; i < kIntroSlides; i++) {
            pics[i] = ResRec(CharRecord, charFileHand)->introPICT[i];
            delays[i] = ResRec(CharRecord, charFileHand)->introDelay[i];
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
        for (i = 1; i < kIntroSlides; i++) {
            pics[i] = -1;
            delays[i] = 0;
        }
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
// MAC: _ActivateCharResource i386 0x00076cb6 ppc 0x0007faf0
void ActivateCharResource(const unsigned char *name, Boolean applyStats)
{
    short i;
    short n;
    short pick;
    short govt;
    CharRecord **cr;
    CharRecord **startInfo;
    short filled;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
    LockAHandle(charFileHand);
    if (applyStats != 0) {
        shipData->credits = ResRec(CharRecord, charFileHand)->cash;
        if (shipData->credits < 0)
            shipData->credits = 0;
        shipData->type = ResRec(CharRecord, charFileHand)->shipType;
        if (shipData->type >= kFirstResourceID)
            shipData->type -= kFirstResourceID;
        else
            shipData->type = 0;
        cr = (CharRecord **)charFileHand;
        playerKills = (*cr)->kills;

        filled = 0;
        for (i = 0; i < kStartSystSlots; i++)
            if ((*cr)->startSyst[i] >= kFirstResourceID)
                filled++;
        if (filled > 0) {
            do {
                pick = Rand(kStartSystSlots);
            } while (ResRec(CharRecord, charFileHand)->startSyst[pick] < kFirstResourceID);
            shipData->curSystem =
                (ResRec(CharRecord, charFileHand)->startSyst[pick] - kFirstResourceID);
        } else {
            shipData->curSystem = 0;
        }

        for (i = 0; i < kGovtSlots; i++) {
            govt = ResRec(CharRecord, charFileHand)->govt[i];
            if (govt >= kFirstResourceID) {
                govt -= kFirstResourceID;
                for (n = 0; n < kMaxSystems; n++) {
                    if (GovtAllies(systemData[n].govt, govt))
                        playerRecord[n] =
                            ResRec(CharRecord, charFileHand)->standing[i];
                    else if (GovtEnemies(systemData[n].govt, govt))
                        playerRecord[n] =
                            -ResRec(CharRecord, charFileHand)->standing[i];
                }
            }
        }

        startInfo = (CharRecord **)charFileHand;
        scenarioStartDate.day = (*startInfo)->startDay;
        scenarioStartDate.month = (*startInfo)->startMonth;
        scenarioStartDate.year = (*startInfo)->startYear;
        strcpy(datePrefix, (*startInfo)->prefix);
        strcpy(dateSuffix, ResRec(CharRecord, charFileHand)->suffix);
    } else {
        crashlog_mission_bit_set("starting character", (char *)name, 1, -1,
                                 ResRec(CharRecord, charFileHand)->onStart);
        EvalMissionBitSetString(ResRec(CharRecord, charFileHand)->onStart);
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

#if defined(MAC1010) && TARGET_API_MAC_CARBON
OSErr MyOpenDocument(const FSSpec *defaultSpec, FSSpec *outSpec)
{
    NavDialogOptions options;
    AEDesc defaultLocation;
    OSErr err;
    int32_t index;
    NavReplyRecord reply;
    int32_t count;
    AEKeyword keyword;
    DescType actualType;
    Size actualSize;
    FSSpec spec;
    NavTypeListHandle typeList;

    err = NavGetDefaultDialogOptions(&options);
    if (err == noErr) {
        options.dialogOptionFlags |= kNavSelectDefaultLocation;
        options.dialogOptionFlags |= kNavNoTypePopup;
        options.dialogOptionFlags ^= kNavAllowPreviews;
        options.dialogOptionFlags ^= kNavAllowMultipleFiles;
        err = AECreateDesc(typeFSS, defaultSpec, sizeof(FSSpec), &defaultLocation);
        if (err == noErr) {
            typeList = (NavTypeListHandle)GetResource('open', 128);
            if (NavGetFile(&defaultLocation, &reply, &options, NULL, NULL, NULL, typeList, NULL) == noErr
                && reply.validRecord) {
                if (AECountItems(&reply.selection, &count) == noErr) {
                    for (index = 1; index <= count; index++) {
                        if (AEGetNthPtr(&reply.selection, index, typeFSS, &keyword, &actualType,
                                        &spec, sizeof(FSSpec), &actualSize) == noErr)
                            *outSpec = spec;
                    }
                }
                err = NavDisposeReply(&reply);
            } else
                err = fnfErr;
            if (typeList != NULL)
                ReleaseResource((Handle)typeList);
            AEDisposeDesc(&defaultLocation);
        }
    }
    return err;
}
#endif

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
// MAC: _VerifyResourceChecksums i386 0x000733f8 ppc 0x0007c224
void VerifyResourceChecksums(void)
{
    unsigned char alert[256];
    uint32_t resType[kCheckedTypes];
    uint32_t type;
    short t;
    short i;
    Handle h;
    short n;
    int32_t sum;
    uint32_t expected;

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
    resType[21] = kYearType;
    resType[22] = kIntfType;

    if (shipClass[gameIsRegisteredIndex].registered != 0 ? 1 : 0)
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
#ifdef MACBUILD
        expected = EndianU32_BtoN(**(uint32_t **)h);
#else
        expected = **(uint32_t **)h;
#endif
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
