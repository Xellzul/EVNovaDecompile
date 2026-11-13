/*
 * sound.h -- sound effects, music and voices.
 *
 * One declaration per name for the whole tree.
 * Addresses are the win 1.0.10 exe's.
 */
#ifndef SOUND_H
#define SOUND_H
#include "macport.h"
#include "gameconst.h"
#include "burgertypes.h"

#ifdef __cplusplus
extern "C" {
#endif


/* --- one declaration per name --- */
extern Boolean PlaySoundDistance(int32_t unused, void *sound, short flags,
                              FloatPoint *pos, FloatPoint *listener);
                                                      /* 0x004692e0 (engineutils.c) */
extern void AdjustSoundVolume(void);                  /* 0x0046ab60 (mac _AdjustSoundVolume) */
extern void HandleVoiceSound(void);              /* 0x004311f0 */
extern void LoadSounds(void);                            /* 0x004b0740 */
extern void PlaySound(void *snd, short id, short leftVol, short rightVol);      /* 0x0046aad0 */
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern uint32_t lastMoviesTaskTime;
extern short pendingVoiceGovt;          /* 0x00735400 (globaldefs.c) */
extern short voiceResourceCount[kVoiceSets][kVoiceMessages]; /* 0x005979da gameloop.c */
extern short pendingVoiceMsg;           /* 0x007353fe (globaldefs.c) */
extern short pendingVoiceParity;        /* 0x00735402 (globaldefs.c) */
EXTERN_CPP_END
#pragma cplusplus reset


/* --- one declaration per name --- */
#ifndef MAC1010
extern void SetMovieVolume(Movie movie, short volume); /* 0x004d9fb0 */
#endif


extern void SetupIntroMusic(void);                   /* 0x004ab5d0 */
extern void StopIntroMusic(Boolean fadeOut);            /* 0x004ab820 */
extern void HandleIntroMusic(Boolean force);            /* 0x004ab8d0 */
/* the engine's calls that keep the intro music fed: 1.1.1 plays it on its own thread */
#if V111 /* POTENTIAL BUG (win V111): nothing services the intro music on win */
#define ServiceIntroMusic(force)
#else
#define ServiceIntroMusic(force) HandleIntroMusic(force)
#endif
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern SoundRequest warpUpSoundSpec;                 /* 0x00591a98 (mac name) */
extern SoundRequest planetSoundSpec;                 /* 0x00591ab4 (mac name) */
EXTERN_CPP_END
#pragma cplusplus reset
extern short ScrambleRand(void);                     /* 0x004ab970 (licensescramble.c) */

#ifdef __cplusplus
}
#endif

#endif /* SOUND_H */
