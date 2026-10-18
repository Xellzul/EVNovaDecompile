// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgersngwin.c -- BurgerLib's streamed music player (SnSngWin32): a polled task decodes the song into a ring the DirectSound buffer plays. Notes: Research/notes/burgersngwin.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "burgerdsound.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kChunkSize          4096                 /* one ring chunk, and one DirectSound lock */
#define kChunkShift         12
#define kChunkCount         32
#define kChunkMask          (kChunkCount - 1)
#define kRingSize           (kChunkSize * kChunkCount)
#define kPrimeChunks        16                   /* decoded before the buffer starts */
#define kHeaderSize         4096
#define kDecodesPerPoll     16

#define kMusicActive        2                    /* SystemState */
#define kStereoFlag         0x8000               /* AudioDesc.format */
#define kFormat16Bit        2                    /* DigitalMusicReadStateInit's results */
#define kFormat16BitBig     3

#define kMusicStateIdle     0                    /* DigitalMusic_t.State */
#define kMusicStateHeader   1
#define kMusicStatePlaying  2
#define kStateFailed        3

#define WAVE_FORMAT_PCM             1
#define DSBCAPS_STATIC              0x00000002
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRLFREQUENCY       0x00000020
#define DSBCAPS_CTRLPAN             0x00000040
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000
#define DSBPLAY_LOOPING             0x00000001

typedef struct DigitalMusic_t {
    DigitalMusicReadState_t ReadState;           /* +0x00 */
    DSoundBuffer *SongSampleHandle;              /* +0x20 */
    Word32 CurrentIndex;                         /* +0x24 the next chunk to hand DirectSound */
    Word32 EndIndex;                             /* +0x28 the chunk being decoded into */
    struct ReadFileStream_t *FileStreamPtr;      /* +0x2c */
    Word8 *FileBuffer;                           /* +0x30 the decoded ring */
    Word32 WorkOffset;                           /* +0x34 within EndIndex */
    Word32 State;                                /* +0x38 */
    Word32 FirstTime;                            /* +0x3c */
} DigitalMusic_t;

extern char *(*DigitalMusicNameCallback)(Word SongNum);   /* burgerdigsong.c */
static DigitalMusic_t MyState;                   /* 0x0086d230 */
Word BurgerLastSong;                             /* 0x00870db8 */
Word SoundLoopFlag;                              /* 0x00870934 */
Word BurgerSongLoops;                            /* 0x008700d8 */
Word32 BurgerSongFreq;                           /* 0x00870974 */

/* DigitalMusicSoundProc (BurgerLib SnSngWin32, static there): copies decoded chunks into
   the playing buffer. STATE: MATCH 100.0%, exe 191 / obj 191, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x00533c30
static void DigitalMusicSoundProc(DigitalMusic_t *Input)
{
    DSoundBuffer *Buffer;
    Word32 Chunk;
    Word32 Offset;
    Word32 PlayCursor;
    Word32 WriteCursor;
    Word32 PlayingChunk;
    void *Ptr2;
    ULong Len2;
    void *Ptr1;
    ULong Len1;
    Word8 *Src;

    Buffer = MyState.SongSampleHandle;
    Input = &MyState;
    if (Buffer) {
        if (Input->FirstTime) {
            if (Input->EndIndex < kPrimeChunks)
                return;
            if (Buffer->Lock(0, kPrimeChunks * kChunkSize, &Ptr1, &Len1,
                    &Ptr2, &Len2, 0) == 0) {
                Src = Input->FileBuffer;
                memcpy(Ptr1, Src, Len1);
                if (Ptr2 && Len2)
                    memcpy(Ptr2, Src + Len1, Len2);
                Buffer->Unlock(Ptr1, Len1, Ptr2, Len2);
            }
            Input->CurrentIndex = kPrimeChunks;
            Buffer->SetVolume(Win95DirectSoundVolumes[MusicVolume]);
            Buffer->SetFrequency(BurgerSongFreq);
            Buffer->Play(0, 0, DSBPLAY_LOOPING);
            Input->FirstTime = 0;
        }
        Chunk = Input->CurrentIndex;
        if (Chunk != Input->EndIndex &&
            Buffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == 0) {
            PlayingChunk = PlayCursor >> kChunkShift;
            if (PlayingChunk != Chunk) {
                do {
                    Offset = Chunk << kChunkShift;
                    Src = Input->FileBuffer + Offset;
                    if (Buffer->Lock(Offset, kChunkSize, &Ptr1, &Len1,
                            &Ptr2, &Len2, 0) == 0) {
                        memcpy(Ptr1, Src, Len1);
                        if (Ptr2 && Len2)
                            memcpy(Ptr2, Src + Len1, Len2);
                        Buffer->Unlock(Ptr1, Len1, Ptr2, Len2);
                    }
                    Chunk = (Chunk + 1) & kChunkMask;
                } while (Chunk != PlayingChunk && Chunk != Input->EndIndex);
                Input->CurrentIndex = Chunk;
            }
        }
    }
}

/* DigitalMusicReadProc (BurgerLib SnSngWin32, static there): the decoder's byte source.
   STATE: MATCH 100.0%, exe 74 / obj 74, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00533f10
static Word32 DigitalMusicReadProc(void *Data, Word8 *Buffer, Word32 Length)
{
    DigitalMusic_t *Input;
    Word8 *Source;
    Word32 Got;

    Input = (DigitalMusic_t *)Data;
    Source = ReadFileStreamGetData(Input->FileStreamPtr, &Got, Length);
    if (Source) {
        memcpy(Buffer, Source, Got);
        return Got;
    }
    if (ReadFileStreamActive(Input->FileStreamPtr))
        return 0;
    Source = ReadFileStreamGetData(Input->FileStreamPtr, &Got, Length);
    if (Source) {
        memcpy(Buffer, Source, Got);
        return Got;
    }
    return -1;
}

/* DigitalMusicForegroundTask (BurgerLib SnSngWin32, static there): polled from the keyboard
   routine list while a song plays. STATE: MATCH 100.0%, exe 210 / obj 210, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00534010
static void DigitalMusicForegroundTask(void *Data)
{
    Word8 *Header;
    DigitalMusic_t *Input;
    Word32 Got;
    Word Room;
    Word8 *Dest;
    long Result;
    Word32 Offset;
    Word Count;
    Word Temp;
    AudioDesc Desc;
    DSBufferDesc BufferDesc;
    WaveFormat Format;

    Input = (DigitalMusic_t *)Data;
    if (Input->State == kMusicStateHeader) {
        Header = ReadFileStreamGetData(Input->FileStreamPtr, &Got, kHeaderSize);
        if (Header && Got == kHeaderSize &&
            ParseSoundFileImage(&Desc, Header, kHeaderSize) == 0) {
            FastMemSet(&Format, 0, sizeof(WaveFormat));
            Format.formatTag = WAVE_FORMAT_PCM;
            Temp = 1;
            if (Desc.DataType & kStereoFlag)
                Temp++;
            Format.channels = Temp;
            Format.samplesPerSec = Desc.SampleRate;
            BurgerSongFreq = Desc.SampleRate;
            switch (DigitalMusicReadStateInit(&Input->ReadState, &Desc, Header, kRingSize,
                    DigitalMusicReadProc, Input)) {
            default:
                Format.bitsPerSample = 8;
                break;
            case kFormat16Bit:
            case kFormat16BitBig:
                Format.bitsPerSample = 16;
                break;
            }
            Format.blockAlign = (Format.bitsPerSample / 8) * Format.channels;
            Format.cbSize = 0;
            Format.avgBytesPerSec = Format.blockAlign * Format.samplesPerSec;
            ReadFileStreamStop(Input->FileStreamPtr);
            ReadFileStreamStart(Input->FileStreamPtr, Input->ReadState.FileOffset);
            FastMemSet(&BufferDesc, 0, sizeof(DSBufferDesc));
            BufferDesc.size = sizeof(DSBufferDesc);
            BufferDesc.flags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME |
                DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE | DSBCAPS_STATIC;
            BufferDesc.bufferBytes = kRingSize;
            BufferDesc.format = &Format;
            if (MyDirectSoundDevice == 0 ||
                MyDirectSoundDevice->CreateSoundBuffer(&BufferDesc,
                    &Input->SongSampleHandle, 0) != 0) {
                Input->State = kStateFailed;
                return;
            }
            Input->FirstTime = 1;
            DigitalMusicReset(&Input->ReadState);
            Input->State = kMusicStatePlaying;
            Input->SongSampleHandle->SetVolume(Win95DirectSoundVolumes[MusicVolume]);
            Input->WorkOffset = 0;
            Input->CurrentIndex = 0;
            Input->EndIndex = 0;
        }
    }
    if (Input->State == kMusicStatePlaying) {
        Count = kDecodesPerPoll;
        do {
            Temp = (Input->EndIndex + 1) & kChunkMask;
            if (Temp == Input->CurrentIndex)
                break;
            Offset = Input->WorkOffset;
            Room = kChunkSize - Offset;
            Dest = Input->FileBuffer + ((Input->EndIndex << kChunkShift) + Offset);
            Result = DigitalMusicDecode(&Input->ReadState, Dest, Room);
            if (Result == 0)
                break;
            if (Result == -1) {
                if (BurgerSongLoops) {
                    ReadFileStreamStop(Input->FileStreamPtr);
                    ReadFileStreamStart(Input->FileStreamPtr, Input->ReadState.FileOffset);
                    DigitalMusicReset(&Input->ReadState);
                } else {
                    FastMemSet(Dest, DigitalMusicGetSilenceVal(Input->ReadState.DataType), Room);
                    BurgerLastSong = (Word)-1;
                    Input->State = kMusicStateIdle;
                    break;
                }
            } else if (Result == Room) {
                Input->EndIndex = Temp;
                Input->WorkOffset = 0;
            } else {
                Input->WorkOffset += Result;
            }
        } while (--Count);
    }
    DigitalMusicSoundProc(Input);
}

/* DigitalMusicStop (BurgerLib SnSngWin32, static there): inline in DigitalMusicPlay, whose
   next store makes its last one dead. */
static void DigitalMusicStop(void)
{
    DigitalMusic_t *Input;

    Input = &MyState;
    KeyboardRemoveRoutine(DigitalMusicForegroundTask, &MyState);
    if (MyState.SongSampleHandle) {
        Input->SongSampleHandle->Stop();
        Input->SongSampleHandle->Release();
        Input->SongSampleHandle = 0;
    }
    ReadFileStreamDelete(Input->FileStreamPtr);
    Input->FileStreamPtr = 0;
    DigitalMusicReadStateDestroy(&Input->ReadState);
    if (Input->FileBuffer) {
        DeallocAPointer(Input->FileBuffer);
        Input->FileBuffer = 0;
    }
    BurgerLastSong = 0;
}

/* DigitalMusicPlay (BurgerLib SnSngWin32). STATE: MATCH, exe 108 / obj 108 (with
   0x00534550 marked), byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00534380
void DigitalMusicPlay(Word SongNum)
{
    DigitalMusic_t *Input;
    Word8 *Samples;

    if (SongNum != BurgerLastSong) {
        Input = &MyState;
        DigitalMusicStop();
        if (MyDirectSoundDevice == 0 || (SystemState & kMusicActive) == 0)
            SongNum = 0;
        BurgerLastSong = SongNum;
        if (SongNum) {
            Input->FileStreamPtr = ReadFileStreamNew(DigitalMusicNameCallback(SongNum),
                kChunkCount, kChunkSize);
            if (Input->FileStreamPtr) {
                ReadFileStreamStart(Input->FileStreamPtr, 0);
                Samples = (Word8 *)AllocAPointerClear(kRingSize);
                if (Samples == 0) {
                    ReadFileStreamDelete(Input->FileStreamPtr);
                    Input->FileStreamPtr = 0;
                } else {
                    Input->FileBuffer = Samples;
                    BurgerSongLoops = SoundLoopFlag;
                    KeyboardAddRoutine(DigitalMusicForegroundTask, Input);
                    Input->State = kMusicStateHeader;
                    SoundLoopFlag = 0;
                    return;
                }
            }
        }
    }
    SoundLoopFlag = 0;
}

#ifdef __cplusplus
}
#endif
