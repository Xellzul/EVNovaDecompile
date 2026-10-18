// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgermemory.c -- BurgerLib MMMemoryManager, the handle-based memory manager. Notes: Research/notes/burgermemory.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BurgerLib's memory manager handle record (0x1c bytes). A Handle is the address of one of
   these, so its first field is the master pointer. On the free memory list Flags holds the
   used handle that precedes the free range instead. */
typedef struct MyHandle {
    void *MemPtr;                                /* +0x00 */
    Word32 Length;                               /* +0x04 */
    Word32 Flags;                                /* +0x08 */
    struct MyHandle *NextHandle;                 /* +0x0c */
    struct MyHandle *PrevHandle;                 /* +0x10 */
    struct MyHandle *NextPurge;                  /* +0x14 */
    struct MyHandle *PrevPurge;                  /* +0x18 */
} MyHandle;

#define kHandleLocked   0x80
#define kHandleMalloc   0x20                     /* allocated outside the heap, never purged */
#define kMemoryIDReserved 0xFFFE0000UL           /* the ID in the high half of Flags */
#define kMemoryIDUnused   0xABCD0000UL
#define kAlignMask      15
#define kHandleChunk    500                      /* records per MakeNewHandles block */
#define kMinSuperChunk  0x10000
#define kSearchGranule  1024
#define kPointerHandle  -4                       /* AllocAPointer's handle, 16 bytes back */
#define kPointerHeader  0x10
#define GMEM_NOT_BANKED 0x1000
#define MMStageCompact  0
#define MMStagePurge    1
#define kHexNoTerminator  0x4000
#define kHexLeadingZeros  0x8000

extern MyHandle UsedHand2;
extern MyHandle PurgeHands;
extern MyHandle FreeMemHands;
extern MyHandle PurgeHandleFiFo;

Word32 MaxMemSize = 0x400000;                    /* 0x00576c50 */
Word32 MinReserveSize = 0x10000;                 /* 0x00576c54 */
Word MinHandles = kHandleChunk;                  /* 0x00576c58 */
MyHandle UsedHand1 = {0, 0, kMemoryIDReserved | kHandleLocked | kHandleFixed,
    &UsedHand2, &UsedHand2, 0, 0};               /* 0x00576c5c */
MyHandle UsedHand2 = {0, 0, kMemoryIDReserved | kHandleLocked | kHandleFixed,
    &UsedHand1, &UsedHand1, 0, 0};               /* 0x00576c78 */
MyHandle PurgeHands = {0, 0, kMemoryIDReserved, &PurgeHands, &PurgeHands, 0, 0};
MyHandle FreeMemHands = {0, 0, kMemoryIDReserved, &FreeMemHands, &FreeMemHands, 0, 0};
MyHandle PurgeHandleFiFo = {0, 0, kMemoryIDReserved, &PurgeHandleFiFo, &PurgeHandleFiFo,
    &PurgeHandleFiFo, &PurgeHandleFiFo};         /* 0x00576ccc */

/* The debug source-line table (BurgerLib MMMemoryManager): which file and line allocated a
   handle. No game code reaches it; its two bodies are dropped by the linker and only their
   literals reach the exe, but the member's .bss, these four first, is the exe's. */
typedef struct DebugMe {
    Handle Owner;                                /* +0x00 */
    Word32 Line;                                 /* +0x04 high bit: pointer */
    const char *File;                            /* +0x08 */
} DebugMe;

static Word DebugRecurse;                        /* 0x00865ab8 */
static Word DebugCount;
static Word DebugMaxCount;
static DebugMe **DebugMeHand;
static Word32 AllocatedMemSize;                  /* 0x00865ac8 */
static MyHandle *FreeHands;                      /* 0x00865acc */
static Word8 *MemoryBlockList;                   /* 0x00865ad0 */
MemPurgeProcPtr MemPurgeCallBack;                /* 0x00870164 */

static void *FooAlloc(Word32 size)
{
    void *block;

    block = GlobalAlloc(GMEM_NOT_BANKED, size);
    if (block != 0)
        VirtualLock(block, size);
    return block;
}

static void FooFree(void *block)
{
    Word32 length;

    if ((Word32)block != 0) {
        length = GlobalSize(block);
        VirtualUnlock(block, length);
        GlobalFree(block);
    }
}

/* FooFree with its guard as a pointer test: the exe keeps this one inline where the
   integer-guard FooFree above is parked (notes). */
static void FooFreeNonNull(void *block)
{
    Word32 length;

    if (block != 0) {
        length = GlobalSize(block);
        VirtualUnlock(block, length);
        GlobalFree(block);
    }
}

/* DeInitMemory (BurgerLib MMMemoryManager, the atexit handler InitMemory registers).
   STATE: MATCH 100%, exe 30 / obj 30, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7c80
void DeInitMemory(void)
{
    void *block;
    void *next;

    if (UsedHand1.NextHandle == &UsedHand2) {
        block = MemoryBlockList;
        if (block != 0) {
            do {
                next = *(void **)block;
                FooFree(block);
                block = next;
            } while (block != 0);
            MemoryBlockList = 0;
        }
    }
}

/* MakeNewHandles (BurgerLib MMMemoryManager): another block of 500 handle records onto the
   free record list, the first one returned. STATE: MATCH 100.0%, exe 40 / obj 40,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7d00
static MyHandle *MakeNewHandles(void)
{
    Word8 *block;
    MyHandle *handle;
    MyHandle *next;
    Word i;

    block = (Word8 *)FooAlloc(sizeof(MyHandle) * kHandleChunk + sizeof(Word8 *));
    if (block != 0) {
        *(Word8 **)block = MemoryBlockList;
        handle = (MyHandle *)(block + sizeof(Word8 *));
        MemoryBlockList = block;
        MinHandles += kHandleChunk;
        i = kHandleChunk - 1;
        next = 0;
        handle += kHandleChunk - 1;
        do {
            handle->Flags = kMemoryIDUnused;
            handle->NextHandle = next;
            next = handle;
            --handle;
        } while (--i);
        FreeHands = next;
        return handle;
    }
    Fatal("Out of malloc memory for Handles!\n");
    return 0;
}

/* ReleaseMemRange (BurgerLib MMMemoryManager): a range back onto the address-ordered free
   list, merged with the free range on either side it touches. STATE: MATCH 100.0%,
   exe 105 / obj 105, byte-identical yes (375 bytes). OPEN: none; End is built and advanced
   in place (`End = p; End += n') */
// FUNCTION: EVNOVA 0x004d7da0
static void ReleaseMemRange(void *MemPtr, Word32 Length, MyHandle *Parent)
{
    MyHandle *Next;
    MyHandle *Prev;
    MyHandle *New;
    Word8 *End;

    Length = (Length + kAlignMask) & ~kAlignMask;
    Next = &FreeMemHands;
    Prev = FreeMemHands.PrevHandle;
    if (Prev != Next) {
        do {
            if ((Word8 *)MemPtr >= (Word8 *)Prev->MemPtr)
                break;
            Prev = Prev->PrevHandle;
        } while (Prev != Next);
        Next = Prev->NextHandle;
        End = (Word8 *)Prev->MemPtr;
        End += Prev->Length;
        if (End == (Word8 *)MemPtr) {
            Prev->Flags = (Word32)Parent;
            Prev->Length += Length;
            if (Next != &FreeMemHands) {
                End += Length;
                if (End == (Word8 *)Next->MemPtr) {
                    Prev->Length += Next->Length;
                    Prev->NextHandle = Next->NextHandle;
                    Next->NextHandle->PrevHandle = Prev;
                    Next->Flags = kMemoryIDUnused;
                    Next->NextHandle = FreeHands;
                    FreeHands = Next;
                }
            }
            return;
        }
        if ((Word8 *)MemPtr + Length == (Word8 *)Next->MemPtr) {
            Next->Flags = (Word32)Parent;
            Next->Length += Length;
            Next->MemPtr = MemPtr;
            return;
        }
        New = FreeHands;
        if (New != 0)
            FreeHands = New->NextHandle;
        else
            New = MakeNewHandles();
        New->MemPtr = MemPtr;
        New->Length = Length;
        New->Flags = (Word32)Parent;
        New->NextHandle = Next;
        New->PrevHandle = Prev;
        New->NextPurge = 0;
        New->PrevPurge = 0;
        Prev->NextHandle = New;
        Next->PrevHandle = New;
        return;
    }
    New = FreeHands;
    if (New != 0)
        FreeHands = New->NextHandle;
    else
        New = MakeNewHandles();
    New->MemPtr = MemPtr;
    New->Length = Length;
    New->Flags = (Word32)Parent;
    New->NextHandle = Next;
    New->PrevHandle = Next;
    New->NextPurge = 0;
    New->PrevPurge = 0;
    Next->NextHandle = New;
    Next->PrevHandle = New;
}

/* InitMemory (BurgerLib MMMemoryManager): the super chunk (MaxMemSize, or the largest block
   the OS will give, binary searched), the first handle records at its bottom, the rest one
   free range. STATE: MATCH 100.0%, exe 213 / obj 213, byte-identical yes (935 bytes).
   OPEN: none; the search loop's free is FooFreeNonNull, the alignment step reuses misalign */
// FUNCTION: EVNOVA 0x004d7f20
void InitMemory(void)
{
    Word32 handleSize;
    void *reserve;
    Word32 swing;
    Word32 tryme;
    Word8 *start;
    Word32 size;
    Word32 misalign;
    MyHandle *handle;
    Word i;
    MyHandle *next;

    AllocatedMemSize = 0;
    if (MemoryBlockList != 0) {
        NonFatal("Memory Manager already started!\n");
        return;
    }
    reserve = FooAlloc(MinReserveSize);
    if (reserve == 0)
        Fatal("Can't allocate minimum OS memory chunk\n");
    size = MaxMemSize;
    MemoryBlockList = (Word8 *)FooAlloc(size);
    if (MemoryBlockList == 0) {
        swing = MaxMemSize >> 1;
        size = 0;
        do {
            tryme = size + swing;
            MemoryBlockList = (Word8 *)FooAlloc(tryme);
            if (MemoryBlockList != 0) {
                FooFreeNonNull(MemoryBlockList);
                size = tryme;
            } else {
                MaxMemSize = tryme;
            }
            swing = (MaxMemSize - size) >> 1;
        } while (swing >= kSearchGranule);
        MemoryBlockList = (Word8 *)FooAlloc(size);
    }
    FooFree(reserve);
    if (MemoryBlockList == 0)
        Fatal("Can't allocate super chunk\n");
    atexit(DeInitMemory);
    if (size < kMinSuperChunk)
        Fatal("Super chunk is less than 64K bytes\n");
    *(Word8 **)MemoryBlockList = 0;
    size -= sizeof(Word8 *);
    start = MemoryBlockList + sizeof(Word8 *);
    misalign = (Word32)start & kAlignMask;
    if (misalign != 0) {
        size -= misalign;
        misalign = (kAlignMask + 1) - misalign;
        start += misalign;
    }
    size &= ~kAlignMask;
    MaxMemSize = size;
    handleSize = MinHandles * sizeof(MyHandle);
    if (handleSize >= size)
        Fatal("Can't allocate default handle array\n");
    next = 0;
    i = MinHandles;
    handle = (MyHandle *)start;
    handle += i - 1;
    do {
        handle->Flags = kMemoryIDUnused;
        handle->NextHandle = next;
        next = handle;
        --handle;
    } while (--i);
    FreeHands = next;
    handle += MinHandles + 1;
    size -= handleSize;
    UsedHand1.MemPtr = 0;
    UsedHand1.Length = (Word32)handle;
    UsedHand1.NextHandle = &UsedHand2;
    UsedHand1.PrevHandle = &UsedHand2;
    UsedHand2.MemPtr = (Word8 *)handle + size;
    UsedHand2.Length = 0xFFFFFFFFUL - (Word32)UsedHand2.MemPtr;
    UsedHand2.NextHandle = &UsedHand1;
    UsedHand2.PrevHandle = &UsedHand1;
    PurgeHands.NextHandle = &PurgeHands;
    PurgeHands.PrevHandle = &PurgeHands;
    FreeMemHands.NextHandle = &FreeMemHands;
    FreeMemHands.PrevHandle = &FreeMemHands;
    PurgeHandleFiFo.NextHandle = &PurgeHandleFiFo;
    PurgeHandleFiFo.PrevHandle = &PurgeHandleFiFo;
    PurgeHandleFiFo.NextPurge = &PurgeHandleFiFo;
    PurgeHandleFiFo.PrevPurge = &PurgeHandleFiFo;
    ReleaseMemRange(handle, size, &UsedHand1);
}

/* GrabMemRange (BurgerLib MMMemoryManager, inlined into AllocAHandle2 and CompactHandles):
   take a range off the start or the end of the free range that holds it. */
static inline void GrabMemRange(void *MemPtr, Word32 Length, MyHandle *Parent, MyHandle *Scan)
{
    Word8 *Chunk;
    MyHandle *Prev;
    MyHandle *Next;

    Length = (Length + kAlignMask) & ~kAlignMask;
    if (Scan == 0) {
        Scan = FreeMemHands.NextHandle;
        for (;;) {
            if ((Word8 *)MemPtr >= (Word8 *)Scan->MemPtr) {
                Chunk = (Word8 *)Scan->MemPtr;
                Chunk += Scan->Length;
                if ((Word8 *)MemPtr < Chunk)
                    break;
            }
            Scan = Scan->NextHandle;
            if (Scan == &FreeMemHands) {
                Fatal("Requested memory range to free is not in the free list\n");
                break;
            }
        }
    }
    Scan->Flags = (Word32)Parent;
    if (Scan->MemPtr == MemPtr) {
        if (Scan->Length == Length) {
            Prev = Scan->PrevHandle;
            Next = Scan->NextHandle;
            Next->PrevHandle = Prev;
            Prev->NextHandle = Next;
            Scan->Flags = kMemoryIDUnused;
            Scan->NextHandle = FreeHands;
            FreeHands = Scan;
            return;
        }
        Scan->Length -= Length;
        Scan->MemPtr = (Word8 *)MemPtr + Length;
        return;
    }
    Scan->Flags = (Word32)Parent;
    Scan->Length = (Word8 *)MemPtr - (Word8 *)Scan->MemPtr;
}

/* AllocAHandle2 (BurgerLib MMMemoryManager): fixed memory from the top of the heap, movable
   from the bottom; compact and purge until it fits, else give the record back and complain.
   Before InitMemory, straight from the OS. STATE: MATCH 100.0%, exe 269 / obj 269,
   byte-identical yes (1018 bytes). OPEN: none; the body sits under `if (MemSize != 0)' with a
   trailing `return 0' (the exe's last epilogue) */
// FUNCTION: EVNOVA 0x004d82d0
void *AllocAHandle2(Word32 MemSize, Word Flag)
{
    MyHandle *NewHand;
    MyHandle *Scan;
    MyHandle *Parent;
    MyHandle *Next;
    Word Stage;

    if (MemSize != 0) {
        if (MemoryBlockList != 0) {
            NewHand = FreeHands;
            if (NewHand != 0) {
                FreeHands = NewHand->NextHandle;
            } else {
                NewHand = MakeNewHandles();
                if (NewHand == 0)
                    return 0;
            }
            NewHand->NextPurge = 0;
            NewHand->PrevPurge = 0;
            NewHand->Length = MemSize;
            NewHand->Flags = Flag & ~kHandleMalloc;
            Stage = 0;
            MemSize = (MemSize + kAlignMask) & ~kAlignMask;
            if (Flag & kHandleFixed) {
                for (;;) {
                    Scan = FreeMemHands.PrevHandle;
                    while (Scan != &FreeMemHands) {
                        if (Scan->Length >= MemSize) {
                            Parent = (MyHandle *)Scan->Flags;
                            Next = Parent->NextHandle;
                            NewHand->PrevHandle = Parent;
                            NewHand->NextHandle = Next;
                            Parent->NextHandle = NewHand;
                            Next->PrevHandle = NewHand;
                            NewHand->MemPtr = (Word8 *)Scan->MemPtr + (Scan->Length - MemSize);
                            GrabMemRange(NewHand->MemPtr, MemSize, Parent, Scan);
                            AllocatedMemSize += NewHand->Length;
                            return NewHand;
                        }
                        Scan = Scan->PrevHandle;
                    }
                    if (Stage & 1) {
                        if (!PurgeHandles(MemSize))
                            break;
                    } else {
                        CompactHandles();
                    }
                    ++Stage;
                }
            } else {
                for (;;) {
                    Scan = FreeMemHands.NextHandle;
                    while (Scan != &FreeMemHands) {
                        if (Scan->Length >= MemSize) {
                            Parent = (MyHandle *)Scan->Flags;
                            Next = Parent->NextHandle;
                            NewHand->PrevHandle = Parent;
                            NewHand->NextHandle = Next;
                            Parent->NextHandle = NewHand;
                            Next->PrevHandle = NewHand;
                            NewHand->MemPtr = Scan->MemPtr;
                            GrabMemRange(Scan->MemPtr, MemSize, NewHand, Scan);
                            AllocatedMemSize += NewHand->Length;
                            return NewHand;
                        }
                        Scan = Scan->NextHandle;
                    }
                    if (!PurgeHandles(MemSize)) {
                        if (Stage)
                            break;
                        CompactHandles();
                        Stage = 1;
                    }
                }
            }
            NewHand->Flags = kMemoryIDUnused;
            NewHand->NextHandle = FreeHands;
            FreeHands = NewHand;
            if (BombFlag)
                DumpHandles();
        } else {
            NewHand = (MyHandle *)FooAlloc(MemSize + sizeof(MyHandle));
            if (NewHand != 0) {
                AllocatedMemSize += MemSize;
                NewHand->Length = MemSize;
                NewHand->Flags = Flag | kHandleMalloc;
                NewHand->PrevHandle = 0;
                NewHand->NextHandle = 0;
                NewHand->NextPurge = 0;
                NewHand->PrevPurge = 0;
                NewHand->MemPtr = NewHand + 1;
                return NewHand;
            }
        }
        NonFatal("Out of memory, I need %ld bytes\n", MemSize);
        return 0;
    }
    return 0;
}

/* AllocAPointer (BurgerLib MMMemoryManager): a fixed handle, its own address in the block's
   first word, the caller's memory 16 bytes in. STATE: MATCH 100.0%, exe 18 / obj 18,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d86d0
void *AllocAPointer(Word32 size)
{
    void **handle;
    void **block;

    if (size != 0) {
        handle = (void **)AllocAHandle2(size + kPointerHeader, kHandleFixed);
        if (handle != 0) {
            block = (void **)*handle;
            *block = handle;
            return (char *)block + kPointerHeader;
        }
    }
    return 0;
}

/* DeallocAHandle (BurgerLib MMMemoryManager): off the purge and used lists, its range back to
   the free list, the record onto the free record list. STATE: MATCH 100.0%, exe 52 / obj 52,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8710
void DeallocAHandle(void *MemHandle)
{
    MyHandle *h;
    MyHandle *Next;
    MyHandle *Prev;

    h = (MyHandle *)MemHandle;
    if (h != 0) {
        AllocatedMemSize -= h->Length;
        if (!(h->Flags & kHandleMalloc)) {
            if (MemoryBlockList != 0) {
                Next = h->NextPurge;
                if (Next != 0) {
                    Prev = h->PrevPurge;
                    Prev->NextPurge = Next;
                    Next->PrevPurge = Prev;
                }
                Next = h->NextHandle;
                Prev = h->PrevHandle;
                Prev->NextHandle = Next;
                Next->PrevHandle = Prev;
                if (h->MemPtr != 0)
                    ReleaseMemRange(h->MemPtr, h->Length, Prev);
                h->Flags = kMemoryIDUnused;
                h->NextHandle = FreeHands;
                FreeHands = h;
            }
            return;
        }
        FooFree(h);
    }
}

/* DeallocAPointer (BurgerLib MMMemoryManager). STATE: MATCH 100.0%, exe 9 / obj 9,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d87d0
void DeallocAPointer(void *MemPtr)
{
    if (MemPtr != 0)
        DeallocAHandle(((void **)MemPtr)[kPointerHandle]);
}

/* ResizeAPointer (BurgerLib MMMemoryManager): AllocAPointer for no pointer, DeallocAPointer for
   no size (both inlined), else the handle behind the pointer resized. STATE: MATCH 100%,
   exe 52 / obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d87f0
void *ResizeAPointer(void *mem, unsigned long size)
{
    void **block;

    if (mem == 0) {
        if (size != 0)
            return AllocAPointer(size);
        return 0;
    }
    if (size == 0) {
        DeallocAPointer(mem);
        return 0;
    }
    mem = ResizeAHandle((Handle)((void **)mem)[kPointerHandle], size + kPointerHeader);
    if (mem != 0) {
        block = *(void ***)mem;
        *block = mem;
        return (char *)block + kPointerHeader;
    }
    return 0;
}

/* ResizeAHandle (BurgerLib MMMemoryManager): shrink in place, else allocate, copy, free, and
   move the new memory into the OLD record so the caller's handle stays valid.
   STATE: MATCH 100%, exe 166 / obj 166, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8890
Handle ResizeAHandle(Handle h, unsigned long size)
{
    MyHandle *OldHand;
    MyHandle *NewHand;
    MyHandle *Scan;
    MyHandle *Next;
    MyHandle *Prev;
    MyHandle *FreeNext;
    Word32 OldSize;
    Word8 *Start;

    OldHand = (MyHandle *)h;
    if (OldHand == 0) {
        if (size != 0)
            return (Handle)AllocAHandle2(size, 0);
        return 0;
    }
    if (size == 0) {
        DeallocAHandle(OldHand);
        return 0;
    }
    OldSize = OldHand->Length;
    if (size == OldSize)
        return (Handle)OldHand;
    if (size < OldSize && !(OldHand->Flags & kHandleMalloc)) {
        OldHand->Length = size;
        size = (size + kAlignMask) & ~kAlignMask;
        OldSize = (OldSize + kAlignMask) & ~kAlignMask;
        OldSize -= size;
        if (OldSize != 0) {
            Start = (Word8 *)OldHand->MemPtr;
            Start += size;
            ReleaseMemRange(Start, OldSize, OldHand);
        }
        return (Handle)OldHand;
    }
    NewHand = (MyHandle *)AllocAHandle2(size, OldHand->Flags);
    if (NewHand != 0) {
        if (size < OldSize)
            OldSize = size;
        memcpy(NewHand->MemPtr, OldHand->MemPtr, OldSize);
    }
    DeallocAHandle(OldHand);
    /* BUG: reads OldHand after DeallocAHandle; a malloc'd handle's record is already freed
       (exe 0x004d89e8) */
    if (NewHand != 0 && !(OldHand->Flags & kHandleMalloc) && !(NewHand->Flags & kHandleMalloc)) {
        FreeNext = OldHand->NextHandle;
        OldHand->MemPtr = NewHand->MemPtr;
        OldHand->Length = NewHand->Length;
        OldHand->Flags = NewHand->Flags;
        Next = NewHand->NextHandle;
        Prev = NewHand->PrevHandle;
        OldHand->NextHandle = Next;
        OldHand->PrevHandle = Prev;
        if (Next != 0)
            Next->PrevHandle = OldHand;
        if (Prev != 0)
            Prev->NextHandle = OldHand;
        Next = NewHand->NextPurge;
        if (Next != 0) {
            Prev = NewHand->PrevPurge;
            Next->PrevPurge = Prev;
            Prev->NextPurge = Next;
            NewHand->NextPurge = 0;
            NewHand->PrevPurge = 0;
            OldHand->PrevPurge = &PurgeHandleFiFo;
            OldHand->NextPurge = PurgeHandleFiFo.NextPurge;
            PurgeHandleFiFo.NextPurge->PrevPurge = OldHand;
            PurgeHandleFiFo.NextPurge = OldHand;
        } else {
            OldHand->NextPurge = 0;
            OldHand->PrevPurge = 0;
        }
        Scan = FreeMemHands.PrevHandle;
        while (Scan != &FreeMemHands) {
            if (Scan->Flags == (Word32)NewHand)
                Scan->Flags = (Word32)OldHand;
            Scan = Scan->PrevHandle;
        }
        NewHand->Flags = kMemoryIDUnused;
        NewHand->NextHandle = FreeNext;
        NewHand->PrevHandle = 0;
        FreeHands = NewHand;
        NewHand = OldHand;
    }
    return (Handle)NewHand;
}

/* FindAHandle (BurgerLib MMMemoryManager): the used handle whose block holds the address.
   Not linked. */
Handle FindAHandle(void *Input)
{
    MyHandle *Scan;

    Scan = UsedHand1.NextHandle;
    while (Scan != &UsedHand2) {
        if ((Word32)Input < (Word32)Scan->MemPtr)
            break;
        if ((Word32)Input < (Word32)Scan->MemPtr + Scan->Length)
            return (Handle)Scan;
        Scan = Scan->NextHandle;
    }
    NonFatal("Can't find handle\n");
    return 0;
}

/* GetAHandleSize (BurgerLib MMMemoryManager). STATE: MATCH 100.0%, exe 7 / obj 7,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8ae0
unsigned long GetAHandleSize(Handle h)
{
    if (h != 0)
        return ((MyHandle *)h)->Length;
    return 0;
}

/* GetAPointerSize (BurgerLib MMMemoryManager). STATE: MATCH 100.0%, exe 8 / obj 8,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8b00
unsigned long GetAPointerSize(void *mem)
{
    if (mem != 0)
        return ((MyHandle **)mem)[kPointerHandle]->Length;
    return 0;
}

/* GetTotalFreeMem (BurgerLib MMMemoryManager): the free list plus every unlocked purgeable
   handle. STATE: MATCH 100.0%, exe 23 / obj 23, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8b20
Word32 GetTotalFreeMem(void)
{
    Word32 Free;
    MyHandle *Scan;
    Word32 Temp;

    Free = 0;
    Scan = FreeMemHands.NextHandle;
    if (Scan != &FreeMemHands) {
        do {
            Free += Scan->Length;
            Scan = Scan->NextHandle;
        } while (Scan != &FreeMemHands);
    }
    Scan = UsedHand1.NextHandle;
    if (Scan != &UsedHand2) {
        do {
            if (!(Scan->Flags & kHandleLocked) && Scan->NextPurge != 0) {
                Temp = Scan->Length;
                Temp = (Temp + kAlignMask) & ~kAlignMask;
                Free += Temp;
            }
            Scan = Scan->NextHandle;
        } while (Scan != &UsedHand2);
    }
    return Free;
}

/* LockAHandle (BurgerLib MMMemoryManager). STATE: MATCH 100.0%, exe 8 / obj 8,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8b80
char *LockAHandle(Handle h)
{
    if (h != 0) {
        ((MyHandle *)h)->Flags |= kHandleLocked;
        return *h;
    }
    return 0;
}

/* UnlockAHandle (BurgerLib MMMemoryManager). STATE: MATCH 100.0%, exe 5 / obj 5,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8ba0
void UnlockAHandle(Handle h)
{
    if (h != 0)
        ((MyHandle *)h)->Flags &= ~kHandleLocked;
}

/* SetHandlePurgeFlag (BurgerLib MMMemoryManager): move a handle to the head of the purge list,
   or off it. STATE: MATCH 100.0%, exe 30 / obj 30, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8bb0
void SetHandlePurgeFlag(void **handle, Word flag)
{
    MyHandle *h;

    h = (MyHandle *)handle;
    if (!(h->Flags & kHandleMalloc)) {
        if (h->NextPurge != 0) {
            h->PrevPurge->NextPurge = h->NextPurge;
            h->NextPurge->PrevPurge = h->PrevPurge;
        }
        if (flag != 0) {
            h->PrevPurge = &PurgeHandleFiFo;
            h->NextPurge = PurgeHandleFiFo.NextPurge;
            PurgeHandleFiFo.NextPurge->PrevPurge = h;
            PurgeHandleFiFo.NextPurge = h;
            return;
        }
        h->NextPurge = 0;
        h->PrevPurge = 0;
    }
}

/* PurgeAHandle (BurgerLib MMMemoryManager): the memory released, the record moved to the
   purged list with a null master pointer. STATE: MATCH 100.0%, exe 44 / obj 44,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8c20
void PurgeAHandle(void *MemHandle)
{
    MyHandle *h;
    MyHandle *Prev;
    MyHandle *Next;

    h = (MyHandle *)MemHandle;
    if (h != 0 && h->MemPtr != 0 && !(h->Flags & kHandleMalloc)) {
        if (MemPurgeCallBack != 0)
            MemPurgeCallBack(MMStagePurge);
        h->Flags &= ~kHandleLocked;
        Next = h->NextPurge;
        if (Next != 0) {
            Prev = h->PrevPurge;
            Next->PrevPurge = Prev;
            Prev->NextPurge = Next;
            h->NextPurge = 0;
            h->PrevPurge = 0;
        }
        Next = h->NextHandle;
        Prev = h->PrevHandle;
        Next->PrevHandle = Prev;
        Prev->NextHandle = Next;
        ReleaseMemRange(h->MemPtr, h->Length, Prev);
        Prev = PurgeHands.NextHandle;
        h->MemPtr = 0;
        h->PrevHandle = &PurgeHands;
        h->NextHandle = Prev;
        Prev->PrevHandle = h;
        PurgeHands.NextHandle = h;
    }
}

/* PurgeHandles (BurgerLib MMMemoryManager): purge the oldest purgeable handles until MemSize
   bytes are back. STATE: MATCH 100.0%, exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8ce0
Word PurgeHandles(Word32 MemSize)
{
    MyHandle *Scan;
    Word Result;
    MyHandle *Prev;
    Word32 Size;

    Result = 0;
    Scan = PurgeHandleFiFo.PrevPurge;
    if (Scan != &PurgeHandleFiFo) {
        do {
            Size = (Scan->Length + kAlignMask) & ~kAlignMask;
            Prev = Scan->PrevPurge;
            PurgeAHandle(Scan);
            Result = 1;
            if (Size >= MemSize)
                return 1;
            MemSize -= Size;
            Scan = Prev;
        } while (Scan != &PurgeHandleFiFo);
    }
    return Result;
}

/* CompactHandles (BurgerLib MMMemoryManager): slide every unlocked movable handle down onto
   the one before it. STATE: MATCH 100.0%, exe 107 / obj 107, byte-identical yes (382 bytes).
   OPEN: none; the inlined free-list search advances Chunk in place */
// FUNCTION: EVNOVA 0x004d8d60
void CompactHandles(void)
{
    MyHandle *Scan;
    MyHandle *Prev;
    Word8 *StartMem;
    void *OldMem;
    Word Called;
    Word32 Size;

    Scan = UsedHand1.NextHandle;
    if (Scan == &UsedHand2)
        return;
    {
        Called = 1;
        if (MemPurgeCallBack != 0)
            Called = 0;
        do {
            if (!(Scan->Flags & (kHandleLocked | kHandleFixed))) {
                Prev = Scan->PrevHandle;
                StartMem = (Word8 *)Prev->MemPtr + ((Prev->Length + kAlignMask) & ~kAlignMask);
                Size = (Word8 *)Scan->MemPtr - StartMem;
                if (Size != 0) {
                    if (!Called) {
                        Called = 1;
                        MemPurgeCallBack(MMStageCompact);
                    }
                    OldMem = Scan->MemPtr;
                    Scan->MemPtr = StartMem;
                    ReleaseMemRange(OldMem, Scan->Length, Prev);
                    GrabMemRange(StartMem, Scan->Length, Scan, 0);
                    memmove(StartMem, OldMem, Scan->Length);
                }
            }
            Scan = Scan->NextHandle;
        } while (Scan != &UsedHand2);
    }
}

/* PrintHandles (BurgerLib MMMemoryManager): one line per record from First up to Last.
   STATE: MATCH 100.0%, exe 98 / obj 98, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d8ee0
static void PrintHandles(MyHandle *First, MyHandle *Last, Word NoCheck)
{
    char FooBar[256];
    Word Count;

    Count = 1;
    DebugXString("#     Handle    Addr   Attr  ID    Size     Prev     Next\n");
    if (NoCheck || First != Last) {
        do {
            NumberToAsciiHex(&FooBar[0], Count, kHexNoTerminator | kHexLeadingZeros | 4);
            FooBar[4] = ' ';
            NumberToAsciiHex(&FooBar[5], (Word32)First, kHexNoTerminator | kHexLeadingZeros | 8);
            FooBar[13] = ' ';
            NumberToAsciiHex(&FooBar[14], (Word32)First->MemPtr,
                kHexNoTerminator | kHexLeadingZeros | 8);
            FooBar[22] = ' ';
            NumberToAsciiHex(&FooBar[23], First->Flags, kHexNoTerminator | kHexLeadingZeros | 4);
            FooBar[27] = ' ';
            NumberToAsciiHex(&FooBar[28], First->Flags >> 16,
                kHexNoTerminator | kHexLeadingZeros | 4);
            FooBar[32] = ' ';
            NumberToAsciiHex(&FooBar[33], First->Length, kHexNoTerminator | kHexLeadingZeros | 8);
            FooBar[41] = ' ';
            NumberToAsciiHex(&FooBar[42], (Word32)First->PrevHandle,
                kHexNoTerminator | kHexLeadingZeros | 8);
            FooBar[50] = ' ';
            NumberToAsciiHex(&FooBar[51], (Word32)First->NextHandle,
                kHexNoTerminator | kHexLeadingZeros | 8);
            FooBar[59] = '\n';
            FooBar[60] = 0;
            DebugXString(FooBar);
            First = First->NextHandle;
            ++Count;
        } while (First != Last);
    }
}

/* DumpHandles (BurgerLib MMMemoryManager). STATE: MATCH 100.0%, exe 37 / obj 37,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9090
void DumpHandles(void)
{
    Word32 Size;

    Size = GetTotalFreeMem();
    DebugXString("Total free mem with purging ");
    DebugXLongWord(Size);
    DebugXString("\nUsed handle list\n");
    PrintHandles(&UsedHand1, &UsedHand1, 1);
    DebugXString("Purged handle list\n");
    PrintHandles(PurgeHands.NextHandle, &PurgeHands, 0);
    DebugXString("Free memory list\n");
    PrintHandles(FreeMemHands.NextHandle, &FreeMemHands, 0);
}

Word DebugRemoveSourceLine(Handle Input, const char *File, Word Line)
{
    DebugMe *Entry;
    DebugMe *Last;
    Word Count;

    if (DebugRecurse)
        return 0;
    ++DebugRecurse;
    Count = DebugCount;
    if (Count) {
        Entry = &(*DebugMeHand)[Count - 1];
        do {
            if (Entry->Owner == Input) {
                --DebugCount;
                Last = &(*DebugMeHand)[DebugCount];
                Entry->Owner = Last->Owner;
                Entry->File = Last->File;
                Entry->Line = Last->Line;
                if (DebugCount == 0) {
                    DeallocAHandle((Handle)DebugMeHand);
                    DebugMeHand = 0;
                    DebugMaxCount = 0;
                }
                --DebugRecurse;
                return 0;
            }
            --Entry;
        } while (--Count);
    }
    NonFatal("Handle %08X was not in the debug list, File: %s, Line %d\n", Input, File, Line);
    --DebugRecurse;
    return 1;
}

void DebugGetSourceLineInfo(Handle Input, const char **File, Word *Line)
{
    DebugMe *Entry;
    Word Count;

    Count = DebugCount;
    if (Count) {
        Entry = *DebugMeHand;
        do {
            if (Entry->Owner == Input) {
                *File = Entry->File;
                *Line = Entry->Line;
                return;
            }
            ++Entry;
        } while (--Count);
    }
    if (Input != (Handle)DebugMeHand) {
        *File = 0;
        *Line = 0;
        return;
    }
    *File = "MMMemoryManager.c";
    *Line = 800;
}

#ifdef __cplusplus
}
#endif
