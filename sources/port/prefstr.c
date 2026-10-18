// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* prefstr.c -- the prefs file: OpenPrefs, ClosePrefs, GetIniIndString, FlushPrefs (win 0x004bcfa0..0x004bd0c0, tu-map object O28; the next function starts the loaddata.c object). Notes: Research/notes/prefstr.md */
/*
 * prefstr.c — read one indexed string out of the port's prefs file
 * (win 0x004bd000).
 *
 * The engine's STR# resources became numbered groups in EVNova.ini: the group
 * is named by the resource id and each string by "S" plus its index, so this
 * is what GetIndString ends up calling.
 */
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

/* the prefs and data-file object's globals: each resource type's file-data record, then its handle */
PrefsFileRec gPrefs;
unsigned char DAT_0085fb30[256];
unsigned char stellarFileData[1120];
Handle gSpobHandle;
unsigned char systemFileData[428];
Handle gSystHandle;
unsigned char shipClassFileData[1860];
Handle gShipHandle;
unsigned char weaponFileData[136];
Handle gWeapHandle;
unsigned char outfitSystemFileData[1028];
Handle gOutfHandle;
unsigned char dudeFileData[88];
Handle gDudeHandle;
unsigned char govtFileData[192];
Handle gGovtHandle;
unsigned char personFileData[400];
Handle gPersHandle;
unsigned char disasterFileData[284];
Handle gOopsHandle;
unsigned char nebulaFileData[520];
Handle gNebuHandle;
unsigned char boomFileData[8];
Handle gBoomHandle;
unsigned char fleetFileData[308];
Handle gFletHandle;
unsigned char junkFileData[676];
Handle gJunkHandle;
unsigned char textFileData[8192];
Handle gDescHandle;
unsigned char cronFileData[824];
Handle gCronHandle;
unsigned char asteroidFileData[40];
Handle gRoidHandle;
unsigned char rankFileData[152];
Handle gRankHandle;
unsigned char charFileData[364];
Handle charFileHand;
unsigned char intfFileData[168];
Handle gIntfFileHand;
unsigned char colorFileData[244];
Handle colorFileHand;
unsigned char DAT_00863f08[1];
char noSavePilotFiles;
char noUpdatePilotData;
unsigned char DAT_00863f0b[9];
PilotRec **playerFileHand;
PilotRec2 **altPlayerFileHand;
PrefsRecord **gPrefsHandle;
unsigned char DAT_00863f20[18];
char pilotsFolder[2048];
char gPrefsDirectory[2054];

#define kPrefStringMax 0x100

static void FlushPrefs(void);

static const char kPrefsFileName[] = "9:EVNova.ini";

/* opened on first use, so nothing has to sequence it against startup */
/* OpenPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcfa0
void OpenPrefs(void)
{
    PrefsFileRec *p;

    p = &gPrefs;
    if (p->file == NULL) {
        p->file = (struct PrefsNode *)OpenPrefsFile(kPrefsFileName);
        p->dirty = 0;
    }
}

/* ClosePrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcfd0
void ClosePrefs(void)
{
    PrefsFileRec *p;

    FlushPrefs();
    p = &gPrefs;
    if (p->file != NULL) {
        ClosePrefsFile(p->file);
        p->file = NULL;
    }
}

/* GetIniIndString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bd000
void GetIniIndString(char *out, long listID, long index)
{
    char group[kPrefStringMax];
    char key[0x40];
    struct PrefsNode *found;

    if (out != NULL) {
        out[0] = 0;
        NumberToAscii(group, listID, 0);
        found = PrefsFindGroup(gPrefs.file, group);
        if (found != NULL) {
            key[0] = 'S';
            NumberToAscii(&key[1], index, 0);
            PrefsGetString(found, key, "", out, kPrefStringMax);
        }
    }
}

/* only written back when something actually changed it */
/* FlushPrefs. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bd090
static void FlushPrefs(void)
{
    PrefsFileRec *p;

    p = &gPrefs;
    if (p->file != NULL && p->dirty != 0) {
        WritePrefsFile(p->file, kPrefsFileName);
        p->dirty = 0;
    }
}
