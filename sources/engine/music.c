// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* music.c -- the intro-music song and CrtInitHook8, the .CRT hook that ends its object (win 0x004ab5d0..0x004ab960). Notes: Research/notes/music.md */
/*
 * Both volume computations read settings.volume through movzx even though the pref
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
#include "cefix.h"
#ifdef MACBUILD
#include <assert.h>
#if V111
#include <pthread.h>
#endif
#endif

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
#ifndef MACBUILD
struct CrtHook { CrtHook() {} };
#endif

/* the load-bar and intro-music object's globals */
#ifndef MACBUILD
static CrtHook crtHook;
#endif
Rect progressRect;
#ifdef MACBUILD
Rect fireOffRect[18];
#else
unsigned char fireOffRect[144];
#endif
Rect progressDefRect;
double progressCount;
double progressMax;
#ifdef MACBUILD
GWorldContainer progressLoadPort;
#else
unsigned char DAT_0085d150[44];
Rect gLoadBarBase;
#endif
RGBColor progressColor[3];
unsigned long lastMoviesTaskTime;
#ifdef MACBUILD
Boolean introMusicPaused = false;
Movie introMusicMovie = NULL;
static Boolean gStopMusic;
#else
Movie introMusicMovie;

/* "", ".mov", ".mp3" — tried in turn under each folder */
char *gMusicExtensions[kExtensionCount] = { "", ".mov", ".mp3" };  /* 0x00575a70 (name invented) */
#endif

#ifdef MACBUILD
#if V111
static void *MusicThread(void *arg);

/* <assert.h>'s assert as the 1.1.1 Intro.c expands it: the build machine's __FILE__, the original's line */
#define IntroAssert(e, line) \
    ((__builtin_expect(!(e), 0) ? __assert(#e, kEngineSourceDir "Intro.c", line) : 0))

/* SpawnIntroMusicThread (mac _SpawnIntroMusicThread): mac only. */
// MAC: _SpawnIntroMusicThread i386 0x00061a5c ppc 0x00069810
void SpawnIntroMusicThread(void)
{
    pthread_attr_t attr;
    pthread_t thread;

    if (settings.introMusic) {
        IntroAssert(!pthread_attr_init(&attr), 468);
        IntroAssert(!pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED), 469);
        pthread_create(&thread, &attr, MusicThread, NULL);
        pthread_attr_destroy(&attr);
    }
}
#endif
#endif

/* SetupIntroMusic (mac _SetupIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab5d0
// MAC: _SetupIntroMusic i386 0x00061af8 ppc 0x000698d4
#ifdef MAC1010
void SetupIntroMusic(void)
{
    short fileRef;
    short resID;
    short vRefNum;
    long dirID;
    FSSpec spec;
    Str255 songName;
    OSErr err;
    short volume;

    if (settings.introMusic == 0)
        return;
    GetIndString(songName, kMusicStrings, kIntroSongName);
    introMusicMovie = NULL;
    err = GetPluginSubfolder(gARef, "\pNova Plug-Ins", &dirID, &vRefNum);
    if (err != noErr)
        err = GetPluginSubfolder(gARef, "\pNova Files", &dirID, &vRefNum);
    if (err != noErr)
        return;
    FSMakeFSSpec(vRefNum, dirID, songName, &spec);
    err = OpenMovieFile(&spec, &fileRef, fsRdPerm);
    if (err != noErr) {
        if (GetPluginSubfolder(gARef, "\pNova Files", &dirID, &vRefNum) != noErr)
            return;
        FSMakeFSSpec(vRefNum, dirID, songName, &spec);
        err = OpenMovieFile(&spec, &fileRef, fsRdPerm);
        if (err != noErr)
            return;
    }
    resID = 0;
    NewMovieFromFile(&introMusicMovie, fileRef, &resID, NULL, newMovieActive, NULL);
    CloseMovieFile(fileRef);
    if (err != noErr)
        return;
    volume = settings.volume * kStartVolumeScale;
    if (volume > kMaxVolume)
        volume = kMaxVolume;
    SetMovieVolume(introMusicMovie, volume);
    StartMovie(introMusicMovie);
    lastMoviesTaskTime = 0;
}
#elif defined(MACBUILD)
void SetupIntroMusic(void)
{
    Handle dataRef = NULL;
    OSType dataRefType;
    short resID = -1;
    FSRef ref;
    Str255 songName;
    HFSUniStr255 uniName;

    introMusicMovie = NULL;
    if (settings.introMusic == '\0')
        return;

    GetIndString(songName, kMusicStrings, kIntroSongName);
    HFSNameGetUnicodeName(songName, kTextEncodingUnknown, &uniName);
    if (FSMakeFSRefUnicode(&pluginsFolder, uniName.length, uniName.unicode, kTextEncodingUnknown, &ref) == noErr
        || FSMakeFSRefUnicode(&novaFilesFolder, uniName.length, uniName.unicode, kTextEncodingUnknown,
                              &ref) == noErr) {
        if (QTNewDataReferenceFromFSRef(&ref, 0, &dataRef, &dataRefType) == noErr)
            NewMovieFromDataRef(&introMusicMovie, newMovieActive, &resID, dataRef, dataRefType);
    }
    if (dataRef != NULL)
        DisposeHandle(dataRef);
}

/* StartIntroMusic (mac _StartIntroMusic): mac only. */
// MAC: _StartIntroMusic i386 0x00061c29 ppc 0x000699ec
void StartIntroMusic(void)
{
    short volume;

    volume = settings.volume * kStartVolumeScale;
    if (volume > kMaxVolume)
        volume = kMaxVolume;
    SetMovieVolume(introMusicMovie, volume);
    StartMovie(introMusicMovie);
    lastMoviesTaskTime = 0;
}

/* CancelMusicThread (mac _CancelMusicThread): mac only. */
// MAC: _CancelMusicThread i386 0x00061c76 ppc 0x00069a60
void CancelMusicThread(void)
{
    gStopMusic = true;
}
#else
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

    if (settings.introMusic == '\0')
        return;

    GetIndString(songName, kMusicStrings, kIntroSongName);
    introMusicMovie = 0;
    i = 0;
    do {
        PStr2CStr(name, songName);
        sprintf(path, "9:Nova Plug-ins:%s%s", name, gMusicExtensions[i]);
#ifdef CEFIX /* F07.05 */
        CE_ExpandPlugInsPath(path, path);
#else
        ExpandAPathToBufferNative(path, path);
#endif
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
#ifdef CEFIX /* F07.06 */
            CE_ExpandFilesPath(path, path);
#else
            ExpandAPathToBufferNative(path, path);
#endif
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

    volume = settings.volume * kStartVolumeScale;
    if (volume > kMaxVolume)
        volume = kMaxVolume;
    SetMovieVolume(introMusicMovie, volume);
    StartMovie(introMusicMovie);
    lastMoviesTaskTime = 0;
}
#endif

/* StopIntroMusic (mac _StopIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab820
// MAC: _StopIntroMusic i386 0x00061c82 ppc 0x00069a70
void StopIntroMusic(Boolean fade)
{
    short volume;
    long start;
#if defined(MACBUILD) && V111
    EventRecord event;
#endif

    if (introMusicMovie != 0) {
        if (fade != 0 && IsMovieDone(introMusicMovie) == 0) {
            volume = settings.volume * kVolumeScale;
            if (volume > kMaxVolume)
                volume = kMaxVolume;
            while (volume > 0) {
                SetMovieVolume(introMusicMovie, volume);
                start = ReadTick();
#ifdef MAC1010
                while (TickCount() <= start)
                    BusyIdle();
#elif defined(MACBUILD)
                while (TickCount() <= start)
                    WaitNextEvent(0, &event, (gameLoaded && inBackground) ? 30 : 0, NULL);
#else
                do {
                    BusyIdle();
                } while (start == ReadTick());
#endif
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
// MAC: _HandleIntroMusic i386 0x00061d68 ppc 0x00069b78
void HandleIntroMusic(Boolean force)
{
    short volume;

    if (introMusicMovie != 0) {
        if (force != 0 || ReadTick() >= lastMoviesTaskTime + kMusicIdleTicks) {
            if (IsMovieDone(introMusicMovie) != 0) {
                DisposeMovie(introMusicMovie);
                introMusicMovie = 0;
            } else {
                volume = settings.volume * kVolumeScale;
                if (volume > kMaxVolume)
                    volume = kMaxVolume;
                SetMovieVolume(introMusicMovie, volume);
                MoviesTask(introMusicMovie, 0);
                lastMoviesTaskTime = ReadTick();
            }
        }
    }
}

#ifdef MACBUILD
/* MusicThread (mac _MusicThread): mac only. */
// MAC: _MusicThread i386 0x00061e10 ppc 0x00069c40
static void *MusicThread(void *arg)
{
    EnterMoviesOnThread(0);
    StartIntroMusic();
    while (!IsMovieDone(introMusicMovie) && !gStopMusic && !introMusicPaused)
        HandleIntroMusic(1);
    StopIntroMusic(1);
    ExitMoviesOnThread();
    return NULL;
}
#endif

#ifndef MACBUILD
/* CrtInitHook8. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab960
void CrtInitHook8(void)
{
}
#endif
