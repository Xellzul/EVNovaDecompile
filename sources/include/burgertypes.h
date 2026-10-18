/*
 * burgertypes.h -- BurgerLib's scalar typedefs, which every TU that used one
 * had been spelling for itself: `typedef unsigned long ULong;' alone appeared
 * 46 times. Types and pure constants only -- nothing with linkage -- so
 * including this exposes a TU to nothing that can move its codegen.
 */
#ifndef BURGERTYPES_H
#define BURGERTYPES_H

typedef unsigned char Word8;
typedef unsigned short Word16;
typedef unsigned int Word;
typedef unsigned long Word32;
typedef unsigned long ULong;
typedef unsigned long long ULongLong;

/* a hand-assembled body: no prologue, no epilogue */
#define NAKED __declspec(naked)

#define kStirPointerBackoff 3
#define kNoLength           0xffffffff
#define kClockPeriodMs      4000

/* UTF-16 surrogate decoding */
#define kSurrogateMask      0xfffffc00
#define kSurrogateHigh      0xd800
#define kSurrogateBias      0xfca02400

/* ST_PlaySoundParam's request, 0x1c bytes: the engine keeps two of these
   (warpUpSoundSpec, planetSoundSpec) and atsndplay.c clamps one in place.
   rate is signed: SetSoundRate converts a double into it. */
typedef struct SoundRequest {
    void **handle;                               /* +0x00 */
    unsigned long id;                            /* +0x04, 0 = assign one */
    long rate;                                   /* +0x08, 0x10000 = unity */
    void (*done)(long reason, void *sound, void *ref);   /* +0x0c */
    long unk10;                                  /* +0x10 */
    unsigned short priority;                     /* +0x14 */
    unsigned short leftLevel;                    /* +0x16 */
    unsigned short rightLevel;                   /* +0x18 */
    unsigned short totalSamples;                 /* +0x1a, the mixer's notice */
} SoundRequest;

/* the decoded picture: BurgerLib's image record, and the Windows port's PicHandle */
typedef struct Image_t {
    Word8 *ImagePtr;                             /* +0x00 */
    Word8 *PalettePtr;                           /* +0x04 RGB triplets, 8 BPP only */
    Word8 *AlphaPtr;                             /* +0x08 */
    Word Width;                                  /* +0x0c */
    Word Height;                                 /* +0x10 */
    Word RowBytes;                               /* +0x14 */
    int DataType;                                /* +0x18 bits per pixel */
} Image_t;

typedef enum {IMAGE332=4,IMAGE8ALPHA=5,IMAGE8_PAL_ALPHA_PAL=6,IMAGE8_PAL_ALPHA=7,IMAGE8_PAL=8,
    IMAGE4444=13,IMAGE1555=14,IMAGE555=15,IMAGE565=16,IMAGE888=24,IMAGE8888=32} ImageTypes_e;

#endif
