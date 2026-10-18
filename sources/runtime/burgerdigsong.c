// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto listing-identical here; -align 4 is BurgerLib's long struct alignment, which the Vorbis state inside OggState_t needs (vorbis_block at +0x24c); -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls)/* burgerdigsong.c -- BurgerLib's streamed-song front end (SnDigitalSong): the song file name, and the dispatch onto a format's decoder. Notes: Research/notes/burgerdigsong.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "codec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kSongDigits         2
#define kSongNumberAt       12                   /* the "00" in "9:Music:Song00.Wav" */
#define kDecoderCount       12
#define kSilence8Bit        0x80

#define kMusicInit          0                    /* a decoder's commands */
#define kMusicDestroy       1
#define kMusicDecode        2
#define kMusicReset         3

#define kSoundTypeByte      0                    /* what a decoder's init answers */
#define kSoundTypeMask      0xFF                 /* the type below the flag bits */
#define kSoundTypeLShort    2
#define kSoundTypeStereo    0x8000

/* BurgerLib's SwapEndian for the x86 mwcc target. */
static __inline Word16 SwapShort(Word16 v)
{
    register Word16 t;

    t = v;
    asm { ror t, 8 }
    return t;
}

#define kReadFailed         ((Word32)-1)
#define kLawChunk           4096
#define kMaceBuffer         4096
#define kMaceSamples        6                    /* bytes out per packed unit */
#define kADPCMMinBuffer     0x2000
#define kOggChunk           0x2000               /* bytes read per pull, and the int16 output area */

#define kOggNeedData        0                    /* OggState_t.State */
#define kOggFirstPage       1
#define kOggHeaders         2
#define kOggDecoding        3
#define kOggPCMWaiting      4
#define kOggDead            666

typedef struct MaceCodec_t {
    MACEState_t MaceData[2];                     /* +0x00 */
    Word32 LeftOver;                             /* +0x30 packed bytes carried over */
    Word32 Stale;                                /* +0x34 samples left in StaleBuffer */
    Word8 *StalePtr;                             /* +0x38 */
    Word8 DecodeBuffer[kMaceBuffer];             /* +0x3c */
    Word8 StaleBuffer[8];                        /* +0x103c */
} MaceCodec_t;

typedef struct ADPCMCodec_t {
    Word LeftOver;                               /* +0x00 */
    Word Stale;                                  /* +0x04 */
    Word8 *StalePtr;                             /* +0x08 */
    Word8 *DecodeBuffer;                         /* +0x0c */
    Word8 *StaleBuffer;                          /* +0x10 one decoded block */
    Word DecodeBufferSize;                       /* +0x14 */
    Word PackChunkSize;                          /* +0x18 */
    Word SampleCount;                            /* +0x1c */
    Word UnpackChunkSize;                        /* +0x20 */
} ADPCMCodec_t;

typedef struct OggState_t {
    ogg_sync_state OggSyncState;                 /* +0x000 */
    ogg_stream_state OggStreamState;             /* +0x01c */
    ogg_page OggPage;                            /* +0x184 */
    ogg_packet OggPacket;                        /* +0x194 */
    vorbis_info VorbisInfo;                      /* +0x1b4 */
    vorbis_comment VorbisComment;                /* +0x1d4 */
    vorbis_dsp_state VorbisDspState;             /* +0x1e4 */
    vorbis_block VorbisBlock;                    /* +0x24c */
    int convsize;                                /* +0x2b8 */
    Word32 State;                                /* +0x2bc */
    int Tempi;                                   /* +0x2c0 */
    Word8 LoadBuffer[kOggChunk];                 /* +0x2c4 */
} OggState_t;

/* DigitalMusicByte (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 34 / obj 34,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005024a0
Word32 DigitalMusicByte(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    Word32 Remaining;
    Word32 Result;

    if (Command == kMusicInit) {
        Result = kSoundTypeByte;
    } else if (Command == kMusicDecode) {
        Remaining = State->SoundLength - State->BytesPlayed;
        if (!Remaining) {
            Result = -1;
        } else {
            if (Length > Remaining)
                Length = Remaining;
            Result = State->ReadProc(State->CallBackParm, Buffer, Length);
            if (Result != -1)
                State->BytesPlayed += Result;
        }
    } else {
        Result = 0;
    }
    return Result;
}

/* DigitalMusicChar (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 56 / obj 56,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00502510
Word32 DigitalMusicChar(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    Word32 Remaining;
    Word32 Result;
    Word32 Count;

    if (Command == kMusicInit) {
        Result = kSoundTypeByte;
    } else if (Command == kMusicDecode) {
        Remaining = State->SoundLength - State->BytesPlayed;
        if (!Remaining) {
            Result = -1;
        } else {
            if (Length > Remaining)
                Length = Remaining;
            Result = State->ReadProc(State->CallBackParm, Buffer, Length);
            if (Result != -1) {
                State->BytesPlayed += Result;
                Count = Result >> 2;
                if (Count) {
                    do {
                        ((Word32 *)Buffer)[0] ^= 0x80808080;
                        Buffer += 4;
                    } while (--Count);
                }
                Count = Result & 3;
                if (Count) {
                    do {
                        Buffer[0] = Buffer[0] ^ 0x80;
                        ++Buffer;
                    } while (--Count);
                }
            }
        }
    } else {
        Result = 0;
    }
    return Result;
}

/* DigitalMusicULaw (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 83 / obj 83,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005025c0
Word32 DigitalMusicULaw(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    Word8 Packed[kLawChunk];
    Word32 Result;
    Word32 Remaining;
    Word32 Chunk;
    long Got;
    const short *Table;
    Word8 *Src;

    switch (Command) {
    case kMusicInit:
        Result = kSoundTypeLShort;
        break;
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (Remaining < 2) {
            Result = -1;
            break;
        }
        Result = 0;
        if (Length < 2)
            break;
        Length &= ~1;
        if (Length > Remaining)
            Length = Remaining;
        do {
            Chunk = Length >> 1;
            if (Chunk > kLawChunk)
                Chunk = kLawChunk;
            Got = State->ReadProc(State->CallBackParm, Packed, Chunk);
            if (!Got)
                break;
            if (Got == -1) {
                State->BytesPlayed = State->SoundLength;
                return Result;
            }
            Result += Got * 2;
            Length -= Got * 2;
            Table = UnpackULawTable;
            Src = Packed;
            do {
                ((short *)Buffer)[0] = Table[Src[0]];
                ++Src;
                Buffer += 2;
            } while (--Got);
        } while (Length);
        State->BytesPlayed += Result;
        break;
    }
    return Result;
}

/* DigitalMusicALaw (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 83 / obj 83,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00502710
Word32 DigitalMusicALaw(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    Word8 Packed[kLawChunk];
    Word32 Result;
    Word32 Remaining;
    Word32 Chunk;
    long Got;
    const short *Table;
    Word8 *Src;

    switch (Command) {
    case kMusicInit:
        Result = kSoundTypeLShort;
        break;
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (Remaining < 2) {
            Result = -1;
            break;
        }
        Result = 0;
        if (Length < 2)
            break;
        Length &= ~1;
        if (Length > Remaining)
            Length = Remaining;
        do {
            Chunk = Length >> 1;
            if (Chunk > kLawChunk)
                Chunk = kLawChunk;
            Got = State->ReadProc(State->CallBackParm, Packed, Chunk);
            if (!Got)
                break;
            if (Got == -1) {
                State->BytesPlayed = State->SoundLength;
                return Result;
            }
            Result += Got * 2;
            Length -= Got * 2;
            Table = UnpackALawTable;
            Src = Packed;
            do {
                ((short *)Buffer)[0] = Table[Src[0]];
                ++Src;
                Buffer += 2;
            } while (--Got);
        } while (Length);
        State->BytesPlayed += Result;
        break;
    }
    return Result;
}

/* DigitalMusicLShort (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 38 / obj 38,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00502860
Word32 DigitalMusicLShort(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    Word32 Remaining;
    Word32 Result;

    switch (Command) {
    case kMusicInit:
        Result = kSoundTypeLShort;
        break;
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (Remaining < 2) {
            Result = -1;
            break;
        }
        Result = 0;
        if (Length >= 2) {
            Length &= ~1;
            if (Length > Remaining)
                Length = Remaining;
            Result = State->ReadProc(State->CallBackParm, Buffer, Length);
            if (Result != -1)
                State->BytesPlayed += Result;
        }
        break;
    }
    return Result;
}

/* DigitalMusicBShort (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 51 / obj 51,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005028d0
Word32 DigitalMusicBShort(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    Word32 Remaining;
    Word32 Result;
    Word32 Count;

    switch (Command) {
    case kMusicInit:
        Result = kSoundTypeLShort;
        break;
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (Remaining < 2) {
            Result = -1;
            break;
        }
        Result = 0;
        if (Length >= 2) {
            Length &= ~1;
            if (Length > Remaining)
                Length = Remaining;
            Result = State->ReadProc(State->CallBackParm, Buffer, Length);
            if (Result != -1) {
                State->BytesPlayed += Result;
                Count = (Word32)Result >> 1;
                if (Count) {
                    do {
                        ((Word16 *)Buffer)[0] = SwapShort(((Word16 *)Buffer)[0]);
                        Buffer += 2;
                    } while (--Count);
                }
            }
        }
        break;
    }
    return Result;
}

/* DigitalMusicMace3 (BurgerLib SnDigitalSong). STATE: MATCH 100.0%, exe 219 / obj 219,
   byte-identical yes (817 bytes). OPEN: none */
// FUNCTION: EVNOVA 0x00502970
Word32 DigitalMusicMace3(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    MaceCodec_t *Mace;
    Word32 Result;
    Word32 Remaining;
    Word32 Count;
    Word Blocks;
    Word32 Want;
    Word InLength;
    Word8 *Src;
    Word32 Bytes;
    Word32 Got;

    switch (Command) {
    case kMusicInit:
        State->CompressStatePtr = AllocAPointerClear(sizeof(MaceCodec_t));
        Result = kSoundTypeByte;
        break;
    case kMusicReset:
        if (State->CompressStatePtr)
            FastMemSet(State->CompressStatePtr, 0, sizeof(MaceCodec_t));
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (!Remaining) {
            Result = -1;
            break;
        }
        Mace = (MaceCodec_t *)State->CompressStatePtr;
        if (Length > Remaining)
            Length = Remaining;
        Result = 0;
        Count = Mace->Stale;
        if (Count) {
            if (Count >= Length) {
                memcpy(Buffer, Mace->StalePtr, Length);
                Mace->Stale = Count - Length;
                Mace->StalePtr += Length;
                State->BytesPlayed += Length;
                return Length;
            }
            Result = Count;
            memcpy(Buffer, Mace->StalePtr, Count);
            Buffer += Count;
            Length -= Count;
            Mace->Stale = 0;
        }
        while (Length) {
            Want = Length / kMaceSamples;
            Blocks = Want;
            if (Length != Want * kMaceSamples)
                ++Want;
            Got = Want * 2;
            if (Got >= kMaceBuffer)
                Got = kMaceBuffer;
            InLength = Mace->LeftOver;
            Got = State->ReadProc(State->CallBackParm, Mace->DecodeBuffer + InLength, Got - InLength);
            if (!Got)
                break;
            if (Got == kReadFailed) {
                State->BytesPlayed = State->SoundLength;
                return Result;
            }
            Got += InLength;
            Mace->LeftOver = 0;
            if (Blocks > Got >> 1)
                Blocks = Got >> 1;
            Src = Mace->DecodeBuffer;
            if (Blocks) {
                MACEExp1to3(Src, Buffer, Blocks, &Mace->MaceData[0], &Mace->MaceData[0], 1, 1);
                Want = Blocks * 2;
                Got -= Want;
                Src += Want;
                Bytes = Blocks * kMaceSamples;
                Length -= Bytes;
                Buffer += Blocks * kMaceSamples;
                Result += Bytes;
            }
            if (Length < kMaceSamples && Length && Got >= 2) {
                Result += Length;
                MACEExp1to3(Src, Mace->StaleBuffer, 1, &Mace->MaceData[0], &Mace->MaceData[0], 1, 1);
                Got -= 2;
                Src += 2;
                memcpy(Buffer, Mace->StaleBuffer, Length);
                Mace->StalePtr = Mace->StaleBuffer + Length;
                Mace->Stale = kMaceSamples - Length;
                Length = 0;
            }
            if (Got) {
                Mace->LeftOver = Got;
                memcpy(Mace->DecodeBuffer, Src, Got);
            }
        }
        State->BytesPlayed += Result;
        break;
    }
    return Result;
}

/* DigitalMusicMace6 (BurgerLib SnDigitalSong). STATE: MATCH 100.0%, exe 216 / obj 216,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00502cb0
Word32 DigitalMusicMace6(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    MaceCodec_t *Mace;
    Word32 Result;
    Word32 Remaining;
    Word32 Count;
    Word Blocks;
    Word32 Want;
    Word InLength;
    Word8 *Src;
    Word32 Bytes;
    Word32 Got;

    switch (Command) {
    case kMusicInit:
        State->CompressStatePtr = AllocAPointerClear(sizeof(MaceCodec_t));
        Result = kSoundTypeByte;
        break;
    case kMusicReset:
        if (State->CompressStatePtr)
            FastMemSet(State->CompressStatePtr, 0, sizeof(MaceCodec_t));
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (!Remaining) {
            Result = -1;
            break;
        }
        Mace = (MaceCodec_t *)State->CompressStatePtr;
        if (Length > Remaining)
            Length = Remaining;
        Result = 0;
        Count = Mace->Stale;
        if (Count) {
            if (Count >= Length) {
                memcpy(Buffer, Mace->StalePtr, Length);
                Mace->Stale = Count - Length;
                Mace->StalePtr += Length;
                State->BytesPlayed += Length;
                return Length;
            }
            Result = Count;
            memcpy(Buffer, Mace->StalePtr, Count);
            Buffer += Count;
            Length -= Count;
            Mace->Stale = 0;
        }
        while (Length) {
            Want = Length / kMaceSamples;
            Blocks = Want;
            if (Length != Want * kMaceSamples)
                ++Want;
            if (Want >= kMaceBuffer)
                Want = kMaceBuffer;
            InLength = Mace->LeftOver;
            Got = State->ReadProc(State->CallBackParm, Mace->DecodeBuffer + InLength, Want - InLength);
            if (!Got)
                break;
            if (Got == kReadFailed) {
                State->BytesPlayed = State->SoundLength;
                return Result;
            }
            Got += InLength;
            Mace->LeftOver = 0;
            if (Blocks > (Word)Got)
                Blocks = Got;
            Src = Mace->DecodeBuffer;
            if (Blocks) {
                MACEExp1to6(Src, Buffer, Blocks, &Mace->MaceData[0], &Mace->MaceData[0], 1, 1);
                Got -= Blocks;
                Src += Blocks;
                Bytes = Blocks * kMaceSamples;
                Length -= Bytes;
                Buffer += Blocks * kMaceSamples;
                Result += Bytes;
            }
            if (Length < kMaceSamples && Length && Got >= 1) {
                Result += Length;
                MACEExp1to6(Src, Mace->StaleBuffer, 1, &Mace->MaceData[0], &Mace->MaceData[0], 1, 1);
                Got -= 1;
                Src += 1;
                memcpy(Buffer, Mace->StaleBuffer, Length);
                Mace->StalePtr = Mace->StaleBuffer + Length;
                Mace->Stale = kMaceSamples - Length;
                Length = 0;
            }
            if (Got) {
                Mace->LeftOver = Got;
                memcpy(Mace->DecodeBuffer, Src, Got);
            }
        }
        State->BytesPlayed += Result;
        break;
    }
    return Result;
}

/* DigitalMusicADPCM (BurgerLib SnDigitalSong). STATE: MATCH 100.0%, exe 270 / obj 270,
   byte-identical yes (936 bytes). OPEN: none */
// FUNCTION: EVNOVA 0x00502fe0
Word32 DigitalMusicADPCM(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    ADPCMUnpackState_t Unpack;
    ADPCMCodec_t *Adpcm;
    Word32 Result;
    Word32 Remaining;
    Word32 Count;
    Word8 *Src;
    Word32 Want;
    Word InLength;
    Word32 Got;
    Word32 Blocks;
    Word Channels;
    Word Step;
    Word BlockSize;

    switch (Command) {
    case kMusicInit: {
        AudioDesc *Desc;
        Word32 InSize;
        Word32 OutSize;

        Desc = (AudioDesc *)Buffer;
        OutSize = Desc->Extra2 * 2;
        InSize = Desc->Extra3;
        if (Desc->DataType & kSoundTypeStereo) {
            OutSize += OutSize;
            InSize += InSize;
        }
        if (InSize < kADPCMMinBuffer)
            InSize = kADPCMMinBuffer;
        Adpcm = (ADPCMCodec_t *)AllocAPointerClear(OutSize + InSize + sizeof(ADPCMCodec_t));
        if (Adpcm) {
            Adpcm->DecodeBufferSize = InSize;
            State->CompressStatePtr = Adpcm;
            Adpcm->PackChunkSize = Desc->Extra3;
            Adpcm->SampleCount = Desc->Extra2;
            Adpcm->UnpackChunkSize = Desc->Extra2 * 2;
            if (Desc->DataType & kSoundTypeStereo)
                Adpcm->UnpackChunkSize <<= 1;
            Adpcm->StaleBuffer = (Word8 *)(Adpcm + 1);
            Adpcm->DecodeBuffer = Adpcm->StaleBuffer + OutSize;
        }
        Result = kSoundTypeLShort;
        break;
    }
    case kMusicReset:
        Adpcm = (ADPCMCodec_t *)State->CompressStatePtr;
        if (Adpcm) {
            Adpcm->LeftOver = 0;
            Adpcm->Stale = 0;
            Adpcm->StalePtr = 0;
        }
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Remaining = State->SoundLength - State->BytesPlayed;
        if (!Remaining) {
            Result = -1;
            break;
        }
        Adpcm = (ADPCMCodec_t *)State->CompressStatePtr;
        if (Length > Remaining)
            Length = Remaining;
        Result = 0;
        Count = Adpcm->Stale;
        if (Count) {
            if (Count >= Length) {
                memcpy(Buffer, Adpcm->StalePtr, Length);
                Adpcm->Stale = Count - Length;
                Adpcm->StalePtr += Length;
                State->BytesPlayed += Length;
                return Length;
            }
            Result = Count;
            memcpy(Buffer, Adpcm->StalePtr, Count);
            Buffer += Count;
            Length -= Count;
            Adpcm->Stale = 0;
        }
        while (Length) {
            Channels = 1;
            if (State->DataType & kSoundTypeStereo)
                ++Channels;
            Unpack.Channels = Channels;
            Unpack.BlockSize = Adpcm->PackChunkSize;
            Unpack.SamplesPerBlock = Adpcm->SampleCount;
            Want = Length / Adpcm->UnpackChunkSize;
            Blocks = Want;
            if (Length != Want * Adpcm->UnpackChunkSize)
                ++Want;
            Got = Want * Adpcm->PackChunkSize;
            if (Got >= Adpcm->DecodeBufferSize)
                Got = Adpcm->DecodeBufferSize;
            InLength = Adpcm->LeftOver;
            Got = State->ReadProc(State->CallBackParm, Adpcm->DecodeBuffer + InLength, Got - InLength);
            if (!Got)
                break;
            if (Got == kReadFailed) {
                State->BytesPlayed = State->SoundLength;
                return Result;
            }
            Got += InLength;
            Adpcm->LeftOver = 0;
            if (Blocks > Got / Adpcm->PackChunkSize)
                Blocks = Got / Adpcm->PackChunkSize;
            Src = Adpcm->DecodeBuffer;
            if (Blocks) {
                Unpack.SrcPtr = Src;
                Unpack.SrcLength = Adpcm->PackChunkSize * Blocks;
                Unpack.OutputPtr = (short *)Buffer;
                Got -= Unpack.SrcLength;
                do {
                    Step = ADPCMDecodeBlock(&Unpack);
                    Result += Step;
                    Length -= Step;
                    Buffer += Step;
                } while (--Blocks);
                Src = Unpack.SrcPtr;
            }
            if (Length < Adpcm->UnpackChunkSize && Length && Got >= Adpcm->PackChunkSize) {
                BlockSize = Adpcm->PackChunkSize;
                Unpack.SrcPtr = Src;
                Unpack.SrcLength = BlockSize;
                Unpack.OutputPtr = (short *)Adpcm->StaleBuffer;
                ADPCMDecodeBlock(&Unpack);
                Src += BlockSize;
                Result += Length;
                Got -= BlockSize;
                memcpy(Buffer, Adpcm->StaleBuffer, Length);
                Adpcm->StalePtr = Adpcm->StaleBuffer + Length;
                Adpcm->Stale = Adpcm->UnpackChunkSize - Length;
                Length = 0;
            }
            if (Got) {
                Adpcm->LeftOver = Got;
                memcpy(Adpcm->DecodeBuffer, Src, Got);
            }
        }
        State->BytesPlayed += Result;
        break;
    }
    return Result;
}

/* DigitalMusicOgg (BurgerLib SnDigitalSong): libvorbis' decoder_example as a resumable
   state machine. STATE: MATCH 100%, exe 522 / obj 522, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503390
Word32 DigitalMusicOgg(DigitalMusicReadState_t *State, Word Command, Word8 *Buffer,
    Word32 Length)
{
    OggState_t *Ogg;
    Word32 Result;
    Word32 Remaining;
    char *Data;
    int i;
    int Eos;
    long Got;
    float **Pcm;
    int Samples;
    int Bout;
    int Frames;
    int j;
    ogg_int16_t *Ptr;
    float *Mono;
    int Val;

    switch (Command) {
    case kMusicInit:
        State->CompressStatePtr = AllocAPointerClear(sizeof(OggState_t));
        Result = kSoundTypeLShort;
        break;
    case kMusicReset:
        Ogg = (OggState_t *)State->CompressStatePtr;
        if (Ogg) {
            ogg_stream_clear(&Ogg->OggStreamState);
            vorbis_block_clear(&Ogg->VorbisBlock);
            vorbis_dsp_clear(&Ogg->VorbisDspState);
            vorbis_comment_clear(&Ogg->VorbisComment);
            vorbis_info_clear(&Ogg->VorbisInfo);
            ogg_sync_clear(&Ogg->OggSyncState);
            ogg_sync_init(&Ogg->OggSyncState);
            Ogg->State = kOggNeedData;
        }
        Result = 0;
        break;
    case kMusicDestroy:
        Ogg = (OggState_t *)State->CompressStatePtr;
        if (Ogg) {
            ogg_stream_clear(&Ogg->OggStreamState);
            vorbis_block_clear(&Ogg->VorbisBlock);
            vorbis_dsp_clear(&Ogg->VorbisDspState);
            vorbis_comment_clear(&Ogg->VorbisComment);
            vorbis_info_clear(&Ogg->VorbisInfo);
            ogg_sync_clear(&Ogg->OggSyncState);
        }
    default:
        Result = 0;
        break;
    case kMusicDecode:
        Ogg = (OggState_t *)State->CompressStatePtr;
        Remaining = State->SoundLength - State->BytesPlayed;
        if (!Remaining || Ogg->State == kOggDead) {
            Result = -1;
            break;
        }
        if (Length > Remaining)
            Length = Remaining;
        Result = 0;
        if (Ogg->State == kOggNeedData) {
            Data = ogg_sync_buffer(&Ogg->OggSyncState, kOggChunk);
            i = State->ReadProc(State->CallBackParm, (Word8 *)Data, kOggChunk);
            if (i == 0 || i == -1)
                return i;
            ogg_sync_wrote(&Ogg->OggSyncState, i);
            Ogg->State = kOggFirstPage;
        }
        if (Ogg->State == kOggHeaders)
            goto ReadHeaders;
        if (Ogg->State == kOggFirstPage) {
            if (ogg_sync_pageout(&Ogg->OggSyncState, &Ogg->OggPage) != 1) {
                Ogg->State = kOggDead;
                return -1;
            }
            ogg_stream_init(&Ogg->OggStreamState, ogg_page_serialno(&Ogg->OggPage));
            vorbis_info_init(&Ogg->VorbisInfo);
            vorbis_comment_init(&Ogg->VorbisComment);
            if (ogg_stream_pagein(&Ogg->OggStreamState, &Ogg->OggPage) < 0) {
                Ogg->State = kOggDead;
                return -1;
            }
            if (ogg_stream_packetout(&Ogg->OggStreamState, &Ogg->OggPacket) != 1)
                goto Decode;
            if (vorbis_synthesis_headerin(&Ogg->VorbisInfo, &Ogg->VorbisComment, &Ogg->OggPacket) < 0)
                goto Decode;
            i = 0;
            do {
                do {
                    Got = ogg_sync_pageout(&Ogg->OggSyncState, &Ogg->OggPage);
                    if (!Got)
                        break;
                    if (Got == 1) {
                        ogg_stream_pagein(&Ogg->OggStreamState, &Ogg->OggPage);
                        do {
                            Got = ogg_stream_packetout(&Ogg->OggStreamState, &Ogg->OggPacket);
                            if (!Got)
                                break;
                            if (Got < 0) {
                                Ogg->State = kOggDead;
                                return -1;
                            }
                            vorbis_synthesis_headerin(&Ogg->VorbisInfo, &Ogg->VorbisComment, &Ogg->OggPacket);
                            ++i;
                        } while (i < 2);
                    }
                } while (i < 2);
                Ogg->Tempi = i;
ReadHeaders:
                i = Ogg->Tempi;
                Data = ogg_sync_buffer(&Ogg->OggSyncState, kOggChunk);
                Got = State->ReadProc(State->CallBackParm, (Word8 *)Data, kOggChunk);
                if (Got == -1) {
                    Ogg->State = kOggDead;
                    return -1;
                }
                if (!Got && i < 2) {
                    Ogg->State = kOggHeaders;
                    return 0;
                }
                ogg_sync_wrote(&Ogg->OggSyncState, Got);
            } while (i < 2);
            Ogg->convsize = kOggChunk / Ogg->VorbisInfo.channels;
            vorbis_synthesis_init(&Ogg->VorbisDspState, &Ogg->VorbisInfo);
            vorbis_block_init(&Ogg->VorbisDspState, &Ogg->VorbisBlock);
            Ogg->State = kOggDecoding;
        }
Decode:
        if (Ogg->State >= kOggDecoding) {
            Eos = 0;
            if (Ogg->State == kOggPCMWaiting)
                goto PCMOut;
            do {
                do {
                    Got = ogg_sync_pageout(&Ogg->OggSyncState, &Ogg->OggPage);
                    if (!Got)
                        break;
                    if (Got >= 0) {
                        ogg_stream_pagein(&Ogg->OggStreamState, &Ogg->OggPage);
                        for (;;) {
                            Got = ogg_stream_packetout(&Ogg->OggStreamState, &Ogg->OggPacket);
                            if (!Got)
                                break;
                            if (Got >= 0) {
                                if (!vorbis_synthesis(&Ogg->VorbisBlock, &Ogg->OggPacket))
                                    vorbis_synthesis_blockin(&Ogg->VorbisDspState, &Ogg->VorbisBlock);
PCMOut:
                                while ((Samples = vorbis_synthesis_pcmout(&Ogg->VorbisDspState, &Pcm)) > 0) {
                                    Bout = Samples;
                                    if (Samples > Ogg->convsize)
                                        Bout = Ogg->convsize;
                                    Frames = Length / Ogg->VorbisInfo.channels;
                                    Frames >>= 1;
                                    if (Bout > Frames)
                                        Bout = Frames;
                                    for (i = 0; i < Ogg->VorbisInfo.channels; i++) {
                                        Ptr = (ogg_int16_t *)Buffer + i;
                                        Mono = Pcm[i];
                                        for (j = 0; j < Bout; j++) {
                                            Val = Mono[j] * 32767.f;
                                            if (Val > 32767)
                                                Val = 32767;
                                            if (Val < -32768)
                                                Val = -32768;
                                            *Ptr = Val;
                                            Ptr += Ogg->VorbisInfo.channels;
                                        }
                                    }
                                    vorbis_synthesis_read(&Ogg->VorbisDspState, Bout);
                                    Bout *= Ogg->VorbisInfo.channels * 2;
                                    Length -= Bout;
                                    Buffer += Bout;
                                    Result += Bout;
                                    if (!Length) {
                                        State->BytesPlayed += Result;
                                        Ogg->State = kOggPCMWaiting;
                                        return Result;
                                    }
                                }
                            }
                        }
                        if (ogg_page_eos(&Ogg->OggPage))
                            Eos = 1;
                    }
                } while (!Eos);
                if (!Eos) {
                    Data = ogg_sync_buffer(&Ogg->OggSyncState, kOggChunk);
                    i = State->ReadProc(State->CallBackParm, (Word8 *)Data, kOggChunk);
                    if (i == 0 || i == -1)
                        Eos = 1;
                    else
                        ogg_sync_wrote(&Ogg->OggSyncState, i);
                }
            } while (!Eos);
            Ogg->State = kOggNeedData;
        }
        State->BytesPlayed += Result;
        break;
    }
    return Result;
}

/* indexed by the sound's type; the three unsupported types play as unsigned bytes */
static DigitalMusicDecodeProcPtr Codecs[kDecoderCount] = {
    DigitalMusicByte, DigitalMusicChar, DigitalMusicLShort, DigitalMusicBShort,
    DigitalMusicADPCM, DigitalMusicByte, DigitalMusicByte, DigitalMusicULaw,
    DigitalMusicALaw, DigitalMusicMace3, DigitalMusicMace6, DigitalMusicOgg
};

static char SongName[] = "9:Music:Song00.Wav";

/* MakeSongFileName (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00503be0
char *MakeSongFileName(Word SongNum)
{
    NumberToAscii(SongName + kSongNumberAt, SongNum,
        kLeadingZeros | kNoTerminator | kSongDigits);
    return SongName;
}

char *(*DigitalMusicNameCallback)(Word SongNum) = MakeSongFileName;

/* DigitalMusicShutdown (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 8 / obj 8 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503c10
void DigitalMusicShutdown(void)
{
    DigitalMusicPlay(0);
    BurgerSndKillProcs[1] = 0;
    BurgerSndExitIn = BurgerSndExitIn & ~kDriverUserMusic;
    KillDigitalDriver();
}

/* DigitalMusicGetSilenceVal (BurgerLib SnDigitalSong). STATE: MATCH, exe 6 / obj 6,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503c40
Word DigitalMusicGetSilenceVal(Word32 Format)
{
    if ((Format & kSoundTypeMask) == kSoundTypeByte)
        return kSilence8Bit;
    return 0;
}

/* DigitalMusicDecode (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 21 / obj 21,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503c60
Word32 DigitalMusicDecode(DigitalMusicReadState_t *State, void *Buffer, Word32 Length)
{
    DigitalMusicDecodeProcPtr Proc;

    Proc = State->CodecProc;
    if (Proc)
        return Proc(State, kMusicDecode, (Word8 *)Buffer, Length);
    return -1;
}

/* DigitalMusicReadStateInit (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503ca0
Word8 DigitalMusicReadStateInit(DigitalMusicReadState_t *State, AudioDesc *Desc,
    const Word8 *Image, Word32 BufferSize, DigitalMusicReadProcPtr ReadProc, void *ReadData)
{
    Word Index;

    FastMemSet(State, 0, sizeof(DigitalMusicReadState_t));
    State->ReadProc = ReadProc;
    State->CallBackParm = ReadData;
    State->FileOffset = Desc->SoundPtr - Image;
    State->SoundLength = Desc->SoundLength;
    Index = Desc->DataType;
    State->DataType = Index;
    Index &= kSoundTypeMask;
    if (Index > kDecoderCount - 1)
        Index = 0;
    State->CodecProc = Codecs[Index];
    return State->CodecProc(State, kMusicInit, (Word8 *)Desc, BufferSize);
}

/* DigitalMusicReadStateDestroy (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 24 / obj 24,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503d30
void DigitalMusicReadStateDestroy(DigitalMusicReadState_t *State)
{
    DigitalMusicDecodeProcPtr Proc;

    Proc = State->CodecProc;
    if (Proc) {
        Proc(State, kMusicDestroy, 0, 0);
        State->CodecProc = 0;
    }
    DeallocAPointer(State->CompressStatePtr);
    State->CompressStatePtr = 0;
}

/* DigitalMusicReset (BurgerLib SnDigitalSong). STATE: MATCH 100%, exe 16 / obj 16,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00503d90
void DigitalMusicReset(DigitalMusicReadState_t *State)
{
    DigitalMusicDecodeProcPtr Proc;

    State->BytesPlayed = 0;
    Proc = State->CodecProc;
    if (Proc)
        Proc(State, kMusicReset, 0, 0);
}

#ifdef __cplusplus
}
#endif
