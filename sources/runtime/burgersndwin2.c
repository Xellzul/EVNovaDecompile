// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgersndwin2.c -- burgersndwin, exe run 2. Notes: Research/notes/burgersndwin.md */

#include "burgertypes.h"
#include "burger.h"
#include "gameutil.h"
#include "win32.h"
#include "burgerdsound.h"

#ifdef __cplusplus
extern "C" {
#endif

static void *DirectSoundInstance;                /* 0x0086cb18 dsound.dll */
static void *DirectSoundProcPtr;                 /* 0x0086cb1c */

#define DSBSTATUS_PLAYING   0x00000001
#define DSBSTATUS_LOOPING   0x00000004

/* IsIt (BurgerLib SnWinCode, static there). STATE: MATCH, exe 27 / obj 27, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c950
static long IsIt(PrivSound *sound, void *data)
{
    ULong status;

    if (sound->SoundPtr != 0 &&
        sound->SampleHandle->GetStatus(&status) == 0 &&
        (status & (DSBSTATUS_PLAYING | DSBSTATUS_LOOPING)) != 0)
        return 1;
    return 0;
}

/* StopIt (BurgerLib SnWinCode, static there). STATE: MATCH, exe 46 / obj 46, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052c9b0
static long StopIt(PrivSound *sound, void *data)
{
    if (sound->SoundPtr != 0) {
        sound->SampleHandle->Stop();
        sound->SampleHandle->Release();
        if (sound->Proc != 0)
            sound->Proc(sound->Data);
        sound->SoundPtr = 0;
        if (sound->SampleResource != -1)
            ResourceRelease(sound->SampleRezHeader, sound->SampleResource);
        if (sound->CookiePtr != 0)
            *sound->CookiePtr = (ULong)-1;
    }
    return 0;
}

/* StopASound (BurgerLib SnWinCode). STATE: MATCH, exe 9 / obj 9. OPEN: none */
// FUNCTION: EVNOVA 0x0052ca40
void StopASound(ULong cookie)
{
    SoundIterator(StopIt, 0, cookie, 0);
}

/* DigitalSoundCheckOff (BurgerLib SnWinCode). STATE: MATCH, exe 40 / obj 40. OPEN: none */
// FUNCTION: EVNOVA 0x0052ca70
void DigitalSoundCheckOff(void *data)
{
    ULong i;
    ULong count;
    PrivSound *sound;

    count = BurgerVoiceCount;
    if (count != 0) {
        sound = BurgerSamples;
        i = 0;
        do {
            if (sound->SoundPtr != 0 &&
                SoundIterator(IsIt, 0, i | kCookieIsVoice, 1) == 0)
                SoundIterator(StopIt, 0, i | kCookieIsVoice, 0);
            sound++;
        } while (++i < count);
    }
}

/* KillDigitalDriver (BurgerLib SnWinCode). STATE: MATCH, exe 30 / obj 30. OPEN: none */
// FUNCTION: EVNOVA 0x0052cb10
void KillDigitalDriver(void)
{
    if ((BurgerSndExitIn & (kDriverUserMusic | kDriverUserSounds)) != 0)
        return;
    KeyboardRemoveRoutine(DigitalSoundCheckOff, 0);
    if (MyDirectSoundDevice != 0) {
        MyDirectSoundDevice->Release();
        MyDirectSoundDevice = 0;
    }
    if (DirectSoundInstance != 0) {
        FreeLibrary(DirectSoundInstance);
        DirectSoundInstance = 0;
    }
}

#ifdef __cplusplus
}
#endif
