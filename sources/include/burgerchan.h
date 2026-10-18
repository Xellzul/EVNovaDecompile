/* burgerchan.h -- the sound channel table. Notes: Research/notes/burgerchan.md */

#ifndef BURGERCHAN_H
#define BURGERCHAN_H

#define kSoundChannels 16
#define kChannelBytes  0x38
#define kChannelTable  (kSoundChannels * kChannelBytes)   /* 0x380 */

/* Jack Jansen's IMA ADPCM coder state (adpcm.c). */
struct adpcm_state {
    short valprev;
    char index;
};

/* A playing channel's decoder position. */
typedef struct AdpcmVoice {
    short valprev;                               /* +0x00 */
    unsigned short index;                        /* +0x02 */
    unsigned long pos;                           /* +0x04 */
    unsigned long total;                         /* +0x08 */
    long lastLeft;                               /* +0x0c */
    long lastRight;                              /* +0x10 */
} AdpcmVoice;

typedef struct SoundChannel {
    unsigned long id;                            /* +0x00 */
    void *sound;                                 /* +0x04 */
    unsigned long stream;                        /* +0x08, 16.16 step */
    unsigned long device;                        /* +0x0c, 16.16 */
    void (*done)(long reason, void *sound, void *ref);   /* +0x10 */
    void *data;                                  /* +0x14 */
    AdpcmVoice voice;                            /* +0x18 */
    long unk2C;                                /* +0x2c */
    unsigned short leftLevel;                    /* +0x30 */
    unsigned short rightLevel;                   /* +0x32 */
    unsigned short priority;                     /* +0x34 */
    unsigned char finished;                         /* +0x36 */
    unsigned char pad37;                         /* +0x37 */
} SoundChannel;

#ifdef __cplusplus
extern "C" {
#endif

extern SoundChannel gTheVoices[kSoundChannels];      /* atsound.c */

#ifdef __cplusplus
}
#endif

#endif
