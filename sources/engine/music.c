// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* music.c -- the intro-music song and CrtInitHook8, the .CRT hook that ends its object (win 0x004ab5d0..0x004ab960). Notes: Research/notes/music.md */
/*
 * Both volume computations read volumePref through movzx even though the pref
 * is a signed short, because the product is truncated back to a short and the
 * upper half cannot matter.
 */
#include <stdio.h>
#include "macport.h"
#include "game.h"
#include "sound.h"
#include "player.h"
#include "render.h"
#include "burger.h"
#include "gameutil.h"

#define kMusicIdleTicks   0x28   /* HandleIntroMusic services at most this often */
#define kMaxVolume        0x100
#define kVolumeScale      0x20   /* pref units -> QuickTime volume */
#define kFadeStep         8      /* volume shed per tick while fading out */
#define kStartVolumeScale 0x30   /* SetupIntroMusic starts louder than the fade */

#define kMusicStrings     130    /* STR# holding the intro song's file name */
#define kIntroSongName    2
#define kExtensionCount   3

#define fsRdPerm          1
#define newMovieActive    1

/* the exe's C++ static initializer (.CRT$XCU, CrtInitHook8) */
struct CrtHook { CrtHook() {} };

/* the load-bar and intro-music object's globals */
static CrtHook crtHook;
Rect progressRect;
unsigned char fireOffRect[144];
Rect progressDefRect;
double progressCount;
double progressMax;
unsigned char DAT_0085d150[44];
Rect gLoadBarBase;
RGBColor progressColor[3];
unsigned long introMusicLastServiced;
Movie introMusicMovie;

/* "", ".mov", ".mp3" — tried in turn under each folder */
char *gMusicExtensions[kExtensionCount] = { "", ".mov", ".mp3" };  /* 0x00575a70 (name invented) */

/* SetupIntroMusic (mac _SetupIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab5d0
void SetupIntroMusic(void)
{
    short result;
    short i;
    short volume;
    int err;
    int specErr;
    short resID;
    char path[256];
    FSSpec spec;
    short refNum;
    char name[256];
    Str255 songName;

    if (musicEnabledPref == '\0')
        return;

    GetIndString(songName, kMusicStrings, kIntroSongName);
    introMusicMovie = 0;
    i = 0;
    do {
        PStr2CStr(name, songName);
        sprintf(path, "9:Nova Plug-ins:%s%s", name, gMusicExtensions[i]);
        ExpandAPathToBufferNative(path, path);
        CStr2PStr(path, path);
        result = FSMakeFSSpec(0, 0, path, &spec);
        specErr = result;
        if (specErr == 0)
            result = OpenMovieFile(&spec, &refNum, fsRdPerm);
        err = result;
    } while (err != 0 && ++i < kExtensionCount);

    if (err != 0) {
        i = 0;
        do {
            PStr2CStr(name, songName);
            sprintf(path, "9:Nova Files:%s%s", name, gMusicExtensions[i]);
            ExpandAPathToBufferNative(path, path);
            CStr2PStr(path, path);
            result = FSMakeFSSpec(0, 0, path, &spec);
            specErr = result;
            if (specErr == 0)
                result = OpenMovieFile(&spec, &refNum, fsRdPerm);
            err = result;
        } while (err != 0 && ++i < kExtensionCount);
        if (err != 0)
            return;
    }

    resID = 0;
    NewMovieFromFile(&introMusicMovie, refNum, &resID, 0, newMovieActive, 0);
    CloseMovieFile(refNum);
    if (err != 0)
        return;

    volume = volumePref * kStartVolumeScale;
    if (volume > kMaxVolume)
        volume = kMaxVolume;
    SetMovieVolume(introMusicMovie, volume);
    StartMovie(introMusicMovie);
    introMusicLastServiced = 0;
}

/* StopIntroMusic (mac _StopIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab820
void StopIntroMusic(char fade)
{
    short volume;
    unsigned long start;

    if (introMusicMovie != 0) {
        if (fade != 0 && IsMovieDone(introMusicMovie) == 0) {
            volume = volumePref * kVolumeScale;
            if (volume > kMaxVolume)
                volume = kMaxVolume;
            while (volume > 0) {
                SetMovieVolume(introMusicMovie, volume);
                start = ReadTick();
                do {
                    BusyIdle();
                } while (start == ReadTick());
                volume -= kFadeStep;
                if (volume < 0)
                    volume = 0;
            }
        }
        StopMovie(introMusicMovie);
        DisposeMovie(introMusicMovie);
        introMusicMovie = 0;
    }
}

/* HandleIntroMusic (mac _HandleIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab8d0
void HandleIntroMusic(char force)
{
    short volume;

    if (introMusicMovie != 0) {
        if (force != 0 || ReadTick() >= introMusicLastServiced + kMusicIdleTicks) {
            if (IsMovieDone(introMusicMovie) != 0) {
                DisposeMovie(introMusicMovie);
                introMusicMovie = 0;
            } else {
                volume = volumePref * kVolumeScale;
                if (volume > kMaxVolume)
                    volume = kMaxVolume;
                SetMovieVolume(introMusicMovie, volume);
                MoviesTask(introMusicMovie, 0);
                introMusicLastServiced = ReadTick();
            }
        }
    }
}

/* CrtInitHook8. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab960
void CrtInitHook8(void)
{
}
