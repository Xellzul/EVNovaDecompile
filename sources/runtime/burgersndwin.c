// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgersndwin.c -- BurgerLib's SnSoundManager: shutting the voices down, the AIFF extended, the voice sweep and parsing a sound file image. Notes: Research/notes/burgersndwin.md */

#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "gameutil.h"
#include "win32.h"
#include "burgerdsound.h"

#ifdef __cplusplus
extern "C" {
#endif

ULong BurgerVoiceCount = 0x20;                       /* 0x00578670, .data BurgerVoiceCount */
Word MusicVolume = 255;                          /* 0x00578674 */
Word SystemState = 3;                            /* 0x00578678 */
static Word8 ExitProc;

#define kSecondaryFlag      2

PrivSound BurgerSamples[kMaxVoices + 1];         /* 0x0086e8c8 */
DSound *MyDirectSoundDevice;                     /* 0x0086fff8 */

/* KillSoundPlayer (BurgerLib SnSoundManager). STATE: MATCH 100%, exe 17 / obj 17 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc3f0
void KillSoundPlayer(void)
{
    ULong i;

    i = 0;
    do {
        StopASound(i | kCookieIsVoice);
        i = i + 1;
    } while (i < BurgerVoiceCount);
    BurgerSndKillProcs[2] = 0;
    BurgerSndExitIn = BurgerSndExitIn & ~kSecondaryFlag;
    KillDigitalDriver();
}

#define kExtendedSign   0x8000                   /* sign bit of the 80-bit exponent word */
#define kExponentRebias 0x3c00                   /* 16383 - 1023: extended to double bias */
#define kExponentMask   0x7ff
#define kExplicitOne    0x7f                     /* drops the extended's explicit integer bit */

/* ConvertAiffExtended (BurgerLib SnSoundManager): an AIFF 80-bit big-endian extended to a
   double. STATE: MATCH 100.0%, exe 54 / obj 54, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc430
double ConvertAiffExtended(const void *Input)
{
    Word Sign;
    Word8 *Out;
    double Result;
    Word Exponent;
    Word Value;
    Word i;
    const Word8 *In;

    In = (const Word8 *)Input;
    Out = (Word8 *)&Result;
    Exponent = (In[0] << 8) | In[1];
    Sign = Exponent & kExtendedSign;
    Exponent = (Exponent - kExponentRebias) & kExponentMask;
    Out[7] = (Word8)(Exponent >> 4);
    Out[6] = (Word8)(Exponent << 4);
    if (Sign)
        Out[7] |= 0x80;
    i = 2;
    do {
        Value = In[i];
        if (i == 2)
            Value &= kExplicitOne;
        Out[8 - i] |= (Word8)(Value >> 3);
        if (i != 8)
            Out[7 - i] = (Word8)(Value << 5);
    } while (++i < 8);
    return Result;
}

/* SoundIterator (BurgerLib SnSoundManager). STATE: MATCH 100%, exe 45 / obj 45,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fc4e0
long SoundIterator(SoundProc proc, void *data, long cookie, long firstOnly)
{
    PrivSound *sound;
    ULong count;
    long result;

    result = 0;
    sound = BurgerSamples;
    if ((cookie & kCookieIsVoice) != 0) {
        cookie &= ~kCookieIsVoice;
        if (cookie < BurgerVoiceCount)
            result = proc(&sound[cookie], data);
    } else {
        for (count = BurgerVoiceCount; count != 0; count--) {
            if (sound->SampleResource == cookie) {
                result = proc(sound, data);
                if (firstOnly != 0)
                    break;
            }
            sound++;
        }
    }
    return result;
}

#define kVocSignature     "Creative Voice File\x1a"
#define kVocSignatureLen  0x14

#define kRiff   0x46464952UL                     /* 'RIFF' */
#define kWave   0x45564157UL                     /* 'WAVE' */
#define kData   0x61746164UL                     /* 'data' */
#define kForm   0x4d524f46UL                     /* 'FORM' */
#define kAiff   0x46464941UL                     /* 'AIFF' */
#define kAifc   0x43464941UL                     /* 'AIFC' */
#define kComm   0x434f4d4dUL                     /* 'COMM' */
#define kSsnd   0x53534e44UL                     /* 'SSND' */
#define kMac3   0x3343414dUL                     /* 'MAC3' */
#define kMac6   0x3643414dUL                     /* 'MAC6' */
#define kOggS   0x5367674fUL                     /* 'OggS' */

#define kWavePcm        1                        /* WAVE_FORMAT_PCM */
#define kWaveAdpcm      2                        /* WAVE_FORMAT_ADPCM */
#define kWaveALaw       6                        /* WAVE_FORMAT_ALAW */
#define kWaveMuLaw      7                        /* WAVE_FORMAT_MULAW */

#define kFmt8Unsigned   0
#define kFmt8Signed     1
#define kFmt16          2
#define kFmt16Swapped   3
#define kFmtAdpcm       4
#define kFmtMuLaw       7
#define kFmtALaw        8
#define kFmtMac3        9
#define kFmtMac6        10
#define kFmtOgg         11
#define kFmtStereo      0x8000

#define kMinRiff   0x2c
#define kMinForm   0x0c
#define kMinVoc    0x2e
#define kMinOgg    0x2c
#define kVocSound  9

/* `static __inline' is the only spelling that inlines an asm block in C, and this pair is
   taken BY VALUE rather than through a pointer because the exe swaps a constant search
   key as well as a load. */
static __inline ULong SwapLong(ULong v)
{
    register ULong t;

    t = v;
    asm { bswap t }
    return t;
}

static __inline unsigned short SwapShort(unsigned short v)
{
    register unsigned short t;

    t = v;
    asm { ror t, 8 }
    return t;
}

/* An IFF chunk by its (big-endian) name, or 0 after saying so. */
static __inline Word8 *FindIffChunk(Word8 *data, ULong size, ULong name)
{
    ULong step;
    char text[8];

    if (size >= 0x14) {
        step = kMinForm;
        name = SwapLong(name);
        do {
            size -= step;
            data += step;
            if (*(ULong *)data == name)
                return data;
            step = (SwapLong(*(ULong *)(data + 4)) + 9) & ~1UL;
        } while (step < size);
    }
    *(ULong *)text = name;
    text[4] = 0;
    NonFatal("IFF Chunk %s was not found\n", text);
    return 0;
}

/* The same walk for the sound data, its parameters in the other order: the exe numbers this
   expansion's key before its length (see the notes). */
static __inline Word8 *FindSoundChunk(Word8 *data, ULong name, ULong size)
{
    ULong step;
    char text[8];

    if (size >= 0x14) {
        step = kMinForm;
        name = SwapLong(name);
        do {
            size -= step;
            data += step;
            if (*(ULong *)data == name)
                return data;
            step = (SwapLong(*(ULong *)(data + 4)) + 9) & ~1UL;
        } while (step < size);
    }
    *(ULong *)text = name;
    text[4] = 0;
    NonFatal("IFF Chunk %s was not found\n", text);
    return 0;
}

/* ParseSoundFileImage (BurgerLib SnSoundManager). STATE: MATCH 100.0%, exe 331 / obj 331,
   byte-identical yes (1200 bytes). OPEN: none; the two chunk walks are two inline helpers with
   their parameters in different orders (see notes) */
// FUNCTION: EVNOVA 0x004fc570
long ParseSoundFileImage(AudioDesc *desc, Word8 *data, ULong size)
{
    Word8 *chunk;
    Word8 *comm;
    Word8 *ssnd;
    Word8 *block;
    ULong channels;
    ULong frames;
    float rate;
    long failed;

    FastMemSet(desc, 0, sizeof(AudioDesc));
    failed = 1;

    if (size >= kMinRiff && *(ULong *)data == kRiff
        && *(ULong *)(data + 8) == kWave) {
        chunk = data + *(ULong *)(data + 0x10) + 0x14;
        if (*(ULong *)chunk != kData)
            chunk = chunk + *(ULong *)(chunk + 4) + 8;

        desc->SoundLength = *(ULong *)(chunk + 4);
        desc->SoundPtr = chunk + 8;
        desc->SampleRate = *(ULong *)(data + 0x18);
        channels = *(unsigned short *)(data + 0x16);

        switch (*(unsigned short *)(data + 0x14)) {
        case kWavePcm:
        default:
            if (*(unsigned short *)(data + 0x22) == 8)
                desc->DataType = kFmt8Unsigned;
            else
                desc->DataType = kFmt16;
            failed = 0;
            break;
        case kWaveAdpcm:
            if (*(unsigned short *)(data + 0x22) != 4)
                break;
            desc->DataType = kFmtAdpcm;
            desc->Extra1 = desc->SoundLength;
            desc->Extra2 = *(unsigned short *)(data + 0x26);
            desc->Extra3 = *(unsigned short *)(data + 0x20);
            desc->SoundLength = (desc->Extra1 / desc->Extra3)
                           * desc->Extra2 * channels;
            desc->SoundLength = desc->SoundLength
                + (desc->Extra1 - (desc->Extra1 / desc->Extra3)
                   * desc->Extra3 - channels * 6) * channels;
            desc->SoundLength = desc->SoundLength << 1;
            failed = 0;
            break;
        case kWaveALaw:
            if (*(unsigned short *)(data + 0x22) != 8)
                break;
            desc->DataType = kFmtALaw;
            desc->Extra1 = desc->SoundLength;
            desc->SoundLength = desc->SoundLength << 1;
            failed = 0;
            break;
        case kWaveMuLaw:
            if (*(unsigned short *)(data + 0x22) != 8)
                break;
            desc->DataType = kFmtMuLaw;
            desc->Extra1 = desc->SoundLength;
            desc->SoundLength = desc->SoundLength << 1;
            failed = 0;
            break;
        }

        if (channels == 2)
            desc->DataType = desc->DataType | kFmtStereo;

    } else if (size >= kMinForm && *(ULong *)data == kForm
               && (*(ULong *)(data + 8) == kAiff
                   || *(ULong *)(data + 8) == kAifc)) {
        comm = FindIffChunk(data, size, kComm);
        if (comm != 0) {
            ssnd = FindSoundChunk(data, kSsnd, size);
            if (ssnd != 0) {
                desc->SoundPtr = ssnd + 0x10;
                desc->SoundLength = SwapLong(*(ULong *)(ssnd + 4)) - 8;
                desc->SampleRate = (long)ConvertAiffExtended(comm + 0x12);
                if (SwapShort(*(unsigned short *)(comm + 0x10)) == 8)
                    desc->DataType = kFmt8Signed;
                else
                    desc->DataType = kFmt16Swapped;
                if (*(ULong *)(data + 8) == kAifc) {
                    if (*(ULong *)(comm + 0x1c) == kMac6) {
                        desc->DataType = kFmtMac6;
                        desc->Extra1 = desc->SoundLength;
                        desc->SoundLength = desc->SoundLength * 6;
                    } else if (*(ULong *)(comm + 0x1c) == kMac3) {
                        desc->DataType = kFmtMac3;
                        desc->Extra1 = desc->SoundLength;
                        desc->SoundLength = desc->SoundLength * 3;
                    } else {
                        return 1;
                    }
                }
                if (SwapShort(*(unsigned short *)(comm + 8)) == 2) {
                    desc->DataType |= kFmtStereo;
                    desc->DataType &= 0xffff;
                }
                failed = 0;
            }
        }

    } else if (size >= kMinVoc
               && memcmp(data, kVocSignature, kVocSignatureLen) == 0
               && (ULong)*(unsigned short *)(data + 0x16)
                  == (ULong)*(unsigned short *)(data + 0x18) + 0x1234) {
        block = data + *(unsigned short *)(data + 0x14);
        if (*block == kVocSound) {
            desc->SoundPtr = block + 0x10;
            desc->SoundLength = ((ULong)*(unsigned short *)(block + 2) << 8)
                           + block[1] - 0xc;
            desc->SampleRate = *(ULong *)(block + 4);
            if (block[8] == 8)
                desc->DataType = kFmt8Signed;
            else
                desc->DataType = kFmt16;
            if (block[9] == 2)
                desc->DataType = desc->DataType | kFmtStereo;
            failed = 0;
        }

    } else if (size >= kMinOgg && *(ULong *)data == kOggS) {
        desc->DataType = kFmtOgg;
        desc->SoundPtr = data;
        desc->SoundLength = 0xffffffff;
        desc->SampleRate = *(ULong *)(data + 0x28);
        if (data[0x27] == 2)
            desc->DataType = desc->DataType | kFmtStereo;
        failed = 0;
    }

    if (failed == 0) {
        frames = desc->SoundLength;
        if ((desc->DataType & kFmtStereo) != 0)
            frames = frames >> 1;
        switch (desc->DataType & 0xff) {
        case kFmt16:
        case kFmt16Swapped:
            frames = frames >> 1;
        }
        if (frames != 0) {
            desc->SampleCount = frames;
            rate = desc->SampleRate;
            if (rate)
                desc->TimeInSeconds = frames / rate;
        }
    }
    return failed;
}

#ifdef __cplusplus
}
#endif
