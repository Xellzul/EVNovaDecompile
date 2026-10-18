// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerrswin.c -- BurgerLib's threaded read-ahead file stream (rswin32): a reader thread fills a ring of chunks the music player drains. Notes: Research/notes/burgerrswin.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kReaderStackSize        0x400
#define kIdleSleepMs            16
#define kMaxPathName            2048

typedef struct CriticalSection {
    ULong words[6];
} CriticalSection;

typedef struct ReadFileStream_t {
    Word8 *StreamBuffer;                         /* +0x00 ChunkCount chunks of ChunkSize */
    Word32 BufferSize;                           /* +0x04 */
    Word32 ChunkSize;                            /* +0x08 a power of two */
    Word32 ChunkCount;                           /* +0x0c a power of two */
    Word32 ChunkSizeMask;                        /* +0x10 ChunkSize-1 */
    Word32 ChunkCountMask;                       /* +0x14 ChunkCount-1 */
    Word32 ChunkShift;                           /* +0x18 log2(ChunkSize) */
    volatile Word32 CurrentIndex;                /* +0x1c the chunk the reader of the data is in */
    volatile Word32 EndIndex;                    /* +0x20 the chunk the thread fills next */
    Word32 CurrentOffset;                        /* +0x24 bytes the thread has read */
    Word32 FileSize;                             /* +0x28 */
    Word32 LastTaken;                            /* +0x2c within the CurrentIndex chunk */
    Word32 LastLength;                           /* +0x30 handed out by the last GetData */
    Word32 LastOffset;                           /* +0x34 */
    Word32 FileOffset;                           /* +0x38 the OS file position */
    void *FileRef;                               /* +0x3c */
    void *ThreadHandle;                          /* +0x40 */
    ULong ThreadID;                              /* +0x44 */
    CriticalSection Block;                       /* +0x48 */
    Word8 Stop;                                  /* +0x60 */
    volatile Word8 PendingRead;                  /* +0x61 */
    Word8 Exit;                                  /* +0x62 */
    volatile Word8 ExitAck;                      /* +0x63 set by the thread as it exits */
} ReadFileStream_t;

/* ReadFileStreamCallback (BurgerLib rswin32, static there). STATE: MATCH 100.0%, exe 99 /
   obj 99, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054d690
static unsigned long __stdcall ReadFileStreamCallback(void *Data)
{
    ReadFileStream_t *Input;
    Word32 Length;
    Word Idle;
    Word32 Next;
    Word32 Size;
    Word32 Offset;
    ULong Got;

    Input = (ReadFileStream_t *)Data;
    do {
        EnterCriticalSection(&Input->Block);
        Size = Input->FileSize;
        Idle = 1;
        Offset = Input->CurrentOffset;
        if (Offset < Size && !Input->Stop) {
            Next = (Input->EndIndex + 1) & Input->ChunkCountMask;
            if (Next != Input->CurrentIndex) {
                Length = Input->ChunkSize;
                if (Length > Size - Offset)
                    Length = Size - Offset;
                if (Length) {
                    if (Input->FileOffset != Offset) {
                        if (SetFilePointer(Input->FileRef, Offset, 0, FILE_BEGIN) == Offset)
                            Input->FileOffset = Offset;
                    }
                    Input->PendingRead = 1;
                    if (ReadFile(Input->FileRef,
                            Input->StreamBuffer + Input->EndIndex * Input->ChunkSize, Length,
                            &Got, 0)) {
                        Input->FileOffset += Got;
                        if (Length != Got)
                            Length = Got & ~Input->ChunkSizeMask;
                        if (Length) {
                            Input->CurrentOffset += Length;
                            Input->EndIndex = (((Length + Input->ChunkSizeMask) >>
                                Input->ChunkShift) + Input->EndIndex) & Input->ChunkCountMask;
                            Idle = 0;
                        }
                    }
                    Input->PendingRead = 0;
                }
            }
        }
        LeaveCriticalSection(&Input->Block);
        if (Idle)
            SleepEx(kIdleSleepMs, 0);
    } while (!Input->Exit);
    Input->ExitAck = 1;
    return 0;
}

/* ReadFileStreamNew (BurgerLib rswin32). STATE: MATCH 100%, exe 132 / obj 132, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0054d7e0
ReadFileStream_t *ReadFileStreamNew(const char *FileName, Word32 ChunkCount, Word32 ChunkSize)
{
    ReadFileStream_t *Output;
    Word8 *Buffer;
    Word32 Temp;
    Word32 Shift;
    ULong High;
    char PathName[kMaxPathName];

    ChunkCount = PowerOf2(ChunkCount);
    ChunkSize = PowerOf2(ChunkSize);
    if (ChunkCount && ChunkSize) {
        Output = (ReadFileStream_t *)AllocAPointerClear(sizeof(ReadFileStream_t));
        if (Output) {
            Output->ChunkSize = ChunkSize;
            Output->ChunkCount = ChunkCount;
            Output->ChunkSizeMask = ChunkSize - 1;
            Output->ChunkCountMask = ChunkCount - 1;
            Temp = ChunkSize;
            Shift = (Word32)-1;
            do {
                Shift++;
                Temp >>= 1;
            } while (Temp);
            Output->ChunkShift = Shift;
            InitializeCriticalSection(&Output->Block);
            ExpandAPathToBufferNative(PathName, FileName);
            Output->FileRef = CreateFileA(PathName, GENERIC_READ, 0, 0, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, 0);
            if (Output->FileRef != INVALID_HANDLE_VALUE) {
                Temp = ChunkCount * ChunkSize;
                Output->BufferSize = Temp;
                Buffer = (Word8 *)AllocAPointer(Temp);
                if (Buffer) {
                    Output->StreamBuffer = Buffer;
                    High = 0;
                    Output->FileSize = GetFileSize(Output->FileRef, &High);
                    if (High)
                        Output->FileSize = (Word32)-1;
                    Output->Stop = 1;
                    Output->ThreadHandle = CreateThread(0, kReaderStackSize,
                        ReadFileStreamCallback, Output, 0, &Output->ThreadID);
                    if (Output->ThreadHandle != INVALID_HANDLE_VALUE) {
                        SetThreadPriority(Output->ThreadHandle, THREAD_PRIORITY_HIGHEST);
                        return Output;
                    }
                    DeallocAPointer(Buffer);
                }
                CloseHandle(Output->FileRef);
            }
            DeleteCriticalSection(&Output->Block);
            DeallocAPointer(Output);
        }
    }
    return 0;
}

/* ReadFileStreamDelete (BurgerLib rswin32). STATE: MATCH 100%, exe 40 / obj 40, byte-identical
   yes. OPEN: none; `volatile ExitAck' keeps the exe's reloading wait, see notes */
// FUNCTION: EVNOVA 0x0054da00
void ReadFileStreamDelete(ReadFileStream_t *Input)
{
    if (Input) {
        if (Input->ThreadHandle != INVALID_HANDLE_VALUE) {
            Input->Exit = 1;
            while (!Input->ExitAck) {
            }
            CloseHandle(Input->ThreadHandle);
            Input->ThreadHandle = INVALID_HANDLE_VALUE;
        }
        if (Input->FileRef != INVALID_HANDLE_VALUE) {
            CloseHandle(Input->FileRef);
            Input->FileRef = INVALID_HANDLE_VALUE;
        }
        if (Input->StreamBuffer) {
            DeallocAPointer(Input->StreamBuffer);
            Input->StreamBuffer = 0;
        }
        DeleteCriticalSection(&Input->Block);
        DeallocAPointer(Input);
    }
}

/* ReadFileStreamActive (BurgerLib rswin32). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0054da90
Word ReadFileStreamActive(ReadFileStream_t *Input)
{
    if (Input && !Input->Stop && Input->CurrentOffset < Input->FileSize)
        return 1;
    return 0;
}

/* ReadFileStreamStop (BurgerLib rswin32). STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0054dac0
void ReadFileStreamStop(ReadFileStream_t *Input)
{
    if (Input && !Input->Stop) {
        EnterCriticalSection(&Input->Block);
        Input->Stop = 1;
        LeaveCriticalSection(&Input->Block);
    }
}

/* ReadFileStreamStart (BurgerLib rswin32): the exe carries ReadFileStreamStop's body inline.
   STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054db00
void ReadFileStreamStart(ReadFileStream_t *Input, Word32 Offset)
{
    if (Input) {
        if (Input && !Input->Stop) {
            EnterCriticalSection(&Input->Block);
            Input->Stop = 1;
            LeaveCriticalSection(&Input->Block);
        }
        EnterCriticalSection(&Input->Block);
        Input->CurrentIndex = 0;
        Input->EndIndex = 0;
        Input->CurrentOffset = Offset;
        Input->LastTaken = 0;
        Input->LastLength = 0;
        Input->LastOffset = Offset;
        Input->Stop = 0;
        LeaveCriticalSection(&Input->Block);
    }
}

/* ReadFileStreamGetData (BurgerLib rswin32): ReadFileStreamAcceptData's body inline.
   STATE: MATCH 100.0%, exe 63 / obj 63, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0054db90
Word8 *ReadFileStreamGetData(ReadFileStream_t *Input, Word32 *LengthPtr, Word32 Length)
{
    Word32 Offset;
    Word32 Remaining;
    Word32 Avail;
    Word32 Chunk;

    if (Input) {
        Input->LastOffset += Input->LastLength;
        Offset = Input->LastTaken + Input->LastLength;
        Input->CurrentIndex = ((Offset >> Input->ChunkShift) + Input->CurrentIndex) & Input->ChunkCountMask;
        Input->LastTaken = Offset & Input->ChunkSizeMask;
        Input->LastLength = 0;
        Chunk = Input->EndIndex;
        if (Chunk < Input->CurrentIndex)
            Chunk = Input->ChunkCount;
        Chunk -= Input->CurrentIndex;
        Avail = (Chunk << Input->ChunkShift) - Input->LastTaken;
        Remaining = Input->FileSize - Input->LastOffset;
        if (Avail > Remaining)
            Avail = Remaining;
        if (Length > Avail)
            Length = Avail;
        Input->LastLength = Length;
        if (LengthPtr)
            *LengthPtr = Length;
        if (Length)
            return Input->StreamBuffer + ((Input->CurrentIndex << Input->ChunkShift) + Input->LastTaken);
    }
    if (LengthPtr)
        *LengthPtr = 0;
    return 0;
}

#ifdef __cplusplus
}
#endif
