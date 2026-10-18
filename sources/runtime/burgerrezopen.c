// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerrezopen.c -- BurgerLib RzResourceManager, one TU: opening a .rez container, then closing, purging, releasing and looking up. Notes: Research/notes/burgerrezopen.md */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#include "gametypes.h"
#include "burgerrez.h"

#ifdef __cplusplus
extern "C" {
#endif

static Word8 OnceHit;

#define kRezHeaderSize  0x18
#define kRezStateSize   0x24
#define kOldVersion     0x20
#define kTableOffset    0x0c
#define kFlagOldFormat  2
#define kFlagSwapped    1

typedef struct RezHeader {
    char sig[4];                                 /* +0x00 "BRGR" */
    ULong count;                                 /* +0x04 */
    ULong tableSize;                             /* +0x08 */
    unsigned char rest[12];                      /* +0x0c, [3] is 0x0f */
} RezHeader;

#define kRezMemFlags      0xfff10000UL           /* REZ_MEM_ID, high half */
#define kOldSoundType     5
#define kOldSoundIdBias   5000
#define kOldPacked        0x80000000UL
#define kOldPackedFlag    0x00200000UL
#define kOldKindMask      0x60000000UL
#define kOldKindShift     10
#define kOldOffsetMask    0x1fffffffUL
#define kNameMask         0x0007ffffUL

/* BurgerLib's SwapEndian for the x86 mwcc target. */
static __inline ULong SwapLong(ULong v)
{
    register ULong t;

    t = v;
    asm { bswap t }
    return t;
}

/* ResourceFindName (BurgerLib RzResourceManager): binary search of the sorted name index.
   1 and *out the match; 0 and *out where the name would go. A leading "number:" is skipped.
   STATE: MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00500810
Word ResourceFindName(RezFile *Input, const char *name, RezName **Result)
{
    RezName **NameHand;
    RezName *NamePtr;
    char *TextPtr;
    RezName *RezNamePtr;
    const char *AuxName;
    Word c;
    ULong MaxRec;
    ULong mid;
    ULong MinRec;
    int Test;

    RezNamePtr = 0;
    if (Input != 0 && (NameHand = Input->names) != 0) {
        c = (Word8)name[0];
        if (c >= '0' && c < '9' + 1) {
            AuxName = strchr(name, ':');
            if (AuxName != 0)
                name = AuxName + 1;
        }
        NamePtr = *NameHand;
        MinRec = 0;
        MaxRec = Input->nameCount;
        TextPtr = *(char **)Input->groups;
        do {
            mid = ((MaxRec - MinRec) >> 1) + MinRec;
            RezNamePtr = NamePtr + mid;
            Test = stricmp(TextPtr + RezNamePtr->HashOffset, name);
            if (Test == 0) {
                *Result = RezNamePtr;
                return 1;
            }
            if (Test > 0)
                MaxRec = mid;
            else
                MinRec = mid + 1;
        } while (MinRec < MaxRec);
        if (Test < 0)
            ++RezNamePtr;
    }
    *Result = RezNamePtr;
    return 0;
}

/* Expand the file's resource table into one handle, 20 bytes an entry,
   the name block copied behind it. Old tables use 12-byte records. */
/* RezGroupNew (BurgerLib RezGroupNew). STATE: MATCH 100.0%, exe 211 / obj 211,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00500910
RezGroup **RezGroupNew(ULong *in, RezHeader *header, long flags, ULong base)
{
    Word count;
    void *h;
    RezGroup *group;
    RezEntry *entry;
    ULong *scan;
    ULong groups;
    Word n;
    Word left;
    ULong type;
    ULong kind;
    ULong remaining;
    ULong extra;
    ULong i;

    i = header->count;
    if (header->count != 0) {
        extra = 0;
        scan = in;
        if (flags & kFlagOldFormat) {
            do {
                n = scan[2];
                if (flags & kFlagSwapped)
                    n = SwapLong(n);
                scan += 3;
                extra += n * 8 - 4;
                scan = (ULong *)((Word8 *)scan + n * 12);
            } while (--i);
        } else {
            do {
                n = scan[1];
                scan += 2;
                extra += n * 4;
                scan = (ULong *)((Word8 *)scan + n * 16);
            } while (--i);
        }

        remaining = header->tableSize + extra;
        h = AllocAHandle2(remaining, kRezMemFlags);
        if (h != 0) {
            group = (RezGroup *)LockAHandle((Handle)h);

            groups = header->count;
            if (flags & kFlagOldFormat) {
                do {
                    type = in[0];
                    remaining -= sizeof(RezGroup);
                    group->RezNum = in[1];
                    group->count = in[2];
                    in += 3;
                    if (flags & kFlagSwapped) {
                        group->RezNum = SwapLong(group->RezNum);
                        group->count = SwapLong(group->count);
                        type = SwapLong(type);
                    }
                    if (type == kOldSoundType)
                        group->RezNum += kOldSoundIdBias;
                    left = group->count;
                    if (left != 0) {
                        entry = (RezEntry *)(group + 1);
                        do {
                            entry->FileOffset = in[0];
                            remaining -= sizeof(RezEntry);
                            entry->length = in[1];
                            entry->nameFlags = in[2];
                            in += 3;
                            entry->MemHand = 0;
                            entry->fileLength = 0;
                            if (flags & kFlagSwapped) {
                                entry->FileOffset = SwapLong(entry->FileOffset);
                                entry->length = SwapLong(entry->length);
                                entry->nameFlags = SwapLong(entry->nameFlags);
                            }
                            if (entry->nameFlags != 0)
                                entry->nameFlags += extra;
                            if (entry->FileOffset & kOldPacked) {
                                entry->FileOffset &= ~kOldPacked;
                                entry->nameFlags |= kOldPackedFlag;
                            }
                            entry->fileLength = entry->length;
                            kind = entry->FileOffset & kOldKindMask;
                            if (kind != 0) {
                                entry->nameFlags |= kind >> kOldKindShift;
                                entry->FileOffset &= kOldOffsetMask;
                                entry->length = 0;
                            }
                            entry->FileOffset += base;
                            entry++;
                        } while (--left);
                    }
                    group = (RezGroup *)((RezEntry *)(group + 1) + group->count);
                } while (--groups);
            } else {
                do {
                    group->RezNum = in[0];
                    remaining -= sizeof(RezGroup);
                    group->count = in[1];
                    in += 2;
                    count = group->count;
                    if (count != 0) {
                        entry = (RezEntry *)(group + 1);
                        do {
                            entry->MemHand = 0;
                            remaining -= sizeof(RezEntry);
                            entry->FileOffset = in[0];
                            entry->length = in[1];
                            entry->nameFlags = in[2];
                            entry->fileLength = in[3];
                            in += 4;
                            entry->nameFlags += extra;
                            entry->FileOffset += base;
                            entry++;
                        } while (--count);
                    }
                    group = (RezGroup *)((RezEntry *)(group + 1) + group->count);
                } while (--groups);
            }

            if (remaining != 0)
                memcpy(group, in, remaining);
            UnlockAHandle((Handle)h);
            return (RezGroup **)h;
        }
    }
    return 0;
}

/* RezSortNames. STATE: DIFF 0.0%, exe 10 / obj 8 instrs. OPEN: foreign-compiler ceiling */
// FUNCTION: EVNOVA 0x00500bd0
int RezSortNames(const void *a, const void *b)
{
    return stricmp((const char *)((const RezName *)a)->HashOffset, (const char *)((const RezName *)b)->HashOffset);
}

/* Rebuild the name index: one { name, id } per named entry, sorted
   case-blind, names kept as offsets into the table. */
/* ResourceHashNames (BurgerLib ResourceHashNames). STATE: MATCH 100.0%, exe 120 / obj 120,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00500bf0
void ResourceHashNames(RezFile *rez)
{
    void *names;
    char *table;
    RezGroup *group;
    ULong n;
    RezName *out;
    ULong i;
    ULong id;
    char *walk;
    RezEntry *entry;
    ULong temp;
    ULong groups;
    ULong count;

    DeallocAHandle(rez->names);
    rez->nameCount = 0;
    rez->names = 0;
    count = 0;
    groups = rez->groupCount;
    temp = groups;
    if (groups != 0) {
        walk = *(char **)rez->groups;
        do {
            n = ((RezGroup *)walk)->count;
            walk += sizeof(RezGroup);
            do {
                if ((((RezEntry *)walk)->nameFlags & kNameMask) != 0)
                    count++;
                walk += sizeof(RezEntry);
            } while (--n);
        } while (--temp);
    }
    if (count != 0) {
        names = AllocAHandle2(count * sizeof(RezName), 0);
        if (names != 0) {
            rez->nameCount = count;
            rez->names = (RezName **)names;
            groups = rez->groupCount;
            group = (RezGroup *)LockAHandle((Handle)rez->groups);
            out = *(RezName **)names;
            table = (char *)group;
            do {
                i = group->count;
                entry = (RezEntry *)(group + 1);
                id = group->RezNum;
                do {
                    temp = entry->nameFlags & kNameMask;
                    if (temp != 0) {
                        out->HashOffset = (ULong)(table + temp);
                        out->RezNum = id;
                        out++;
                    }
                    entry++;
                    id++;
                } while (--i);
                group = (RezGroup *)entry;
            } while (--groups);
            qsort(LockAHandle((Handle)names), count, sizeof(RezName), RezSortNames);
            out = *(RezName **)names;
            do {
                out->HashOffset = out->HashOffset - (ULong)table;
                out++;
            } while (--count);
            UnlockAHandle((Handle)names);
            UnlockAHandle((Handle)rez->groups);
            return;
        }
    }
}

/* ResourceInit (BurgerLib ResourceInit). STATE: MATCH 100%, exe 137 / obj 137 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00500d80
long ResourceInit(RezFile *rez, const char *path, ULong start)
{
    RezHeader header;
    FILE *file;
    ULong *table;
    RezGroup **groups;
    long flags;

    if (rez != 0) {
        FastMemSet(rez, 0, kRezStateSize);
        if (path != 0) {
            file = (FILE *)FOpenExpandedPath(path, "rb");
            if (file != 0) {
                if (start != 0)
                    fseek(file, start, SEEK_SET);
                fread(&header, 1, kRezHeaderSize, file);
                if (memcmp(header.sig, "BRGR", sizeof(header.sig)) == 0) {
                    flags = 0;
                    if (header.rest[3] < kOldVersion) {
                        flags = kFlagOldFormat;
                        if (header.tableSize >= SwapLong(header.tableSize)) {
                            header.tableSize = SwapLong(header.tableSize);
                            header.count = SwapLong(header.count);
                            flags |= kFlagSwapped;
                        }
                        fseek(file, start + kTableOffset, SEEK_SET);
                    }
                    table = (ULong *)AllocAPointer(header.tableSize);
                    if (table != 0) {
                        if (fread(table, 1, header.tableSize, file) == header.tableSize) {
                            groups = RezGroupNew(table, &header, flags, start);
                            if (groups != 0) {
                                DeallocAPointer(table);
                                rez->file = file;
                                rez->groupCount = header.count;
                                rez->groups = groups;
                                rez->flags = 1;
                                ResourceHashNames(rez);
                                return 0;
                            }
                        }
                        DeallocAPointer(table);
                    }
                }
                fclose(file);
            }
        }
    }
    return 1;
}

extern Word ResourceAddName(struct RezFile *rez, const char *name); /* 0x005010f0 */

#define kEntryStride   0x14
#define kGroupHeader   8

#define kLoadCountOne   0x01000000UL             /* the top byte counts ResourceLoadHandle calls */
#define kLoadCountMask  0xff000000UL
#define kFlagExternalOk 1                        /* RezFile.flags: a named resource may come from its own file */
#define kFlagDontCache  2                        /* RezFile.flags: release frees at once */
#define kPurgeable      1
#define kRezPackedMask  0x00180000UL             /* 1-3: which decompressor */
#define kRezPackedShift 19
#define kRezFixed       0x00200000UL
#define kRezTriedExternal 0x00400000UL
#define kRezIsExternal  0x00800000UL
#define kMemIdShift     16                       /* the handle's memory id is the resource number */
#define kReadChunk      0x10000

typedef void (*RezDecompressor)(unsigned char *dst, unsigned char *src, unsigned long length,
                                unsigned long packLength);

#define kFirstEntry(g)  ((RezEntry *)((g) + 1))
#define kNextGroup(g)   ((RezGroup *)(kFirstEntry(g) + (g)->count))

/* BurgerLib's ResourceScan: the entry of resource rezNum, or 0; inlined at every use. */
static RezEntry *ResourceScan(RezFile *rez, ULong rezNum)
{
    ULong count;
    RezGroup *group;
    RezEntry *entry;

    entry = 0;
    if (rez != 0) {
        count = rez->groupCount;
        if (count != 0) {
            group = *rez->groups;
            do {
                ULong index = rezNum - group->RezNum;
                ULong entries = group->count;
                if (index < entries) {
                    entry = kFirstEntry(group) + index;
                    break;
                }
                group = (RezGroup *)(kFirstEntry(group) + entries);
            } while (--count);
        }
    }
    return entry;
}
#define kNoRezNum 0xffffffffU

/* ResourceDestroy (BurgerLib ResourceDestroy). STATE: MATCH 100%, exe 55 / obj 55 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00500f80
void ResourceDestroy(RezFile *rez)
{
    long groups;
    char *group;
    long entries;
    char *entry;
    /* read as `void **' here where burger.h says Handle / void * */

    if (rez != 0) {
        if (rez->file != 0) {
            fclose((FILE *)rez->file);
            rez->file = 0;
        }
        groups = rez->groupCount;
        if (groups != 0) {
            group = LockAHandle((Handle)rez->groups);
            do {
                entry = group + kGroupHeader;
                entries = ((long *)group)[1];
                do {
                    DeallocAHandle(*(void ***)entry);
                    entry = entry + kEntryStride;
                } while (--entries);
                group = entry;
            } while (--groups);
        }
        DeallocAHandle(rez->groups);
        DeallocAHandle(rez->names);
        rez->groups = 0;
        rez->names = 0;
        rez->nameCount = 0;
        rez->groupCount = 0;
    }
}

/* ResourcePurgeCache (BurgerLib RzResourceManager): free every loaded resource nobody holds.
   STATE: MATCH 100.0%, exe 56 / obj 56, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00501040
void ResourcePurgeCache(RezFile *rez)
{
    ULong groups;
    ULong entries;
    RezEntry *entry;
    RezGroup *group;

    if (rez != 0) {
        groups = rez->groupCount;
        if (groups != 0) {
            group = (RezGroup *)LockAHandle((Handle)rez->groups);
            do {
                entries = group->count;
                entry = kFirstEntry(group);
                if (entries != 0) {
                    do {
                        if (entry->MemHand != 0 && !(entry->nameFlags & kLoadCountMask)) {
                            DeallocAHandle(entry->MemHand);
                            entry->MemHand = 0;
                        }
                        ++entry;
                    } while (--entries);
                }
                group = kNextGroup(group);
            } while (--groups);
            UnlockAHandle((Handle)rez->groups);
        }
    }
}

/* ResourceAddName (BurgerLib RzResourceManager): give a name a resource number of its own (the
   first free number of the group that starts at 1), so ResourceLoadHandle will look for a file
   of that name; the number, or -1 without memory. STATE: MATCH 100.0%, exe 425 / obj 425,
   byte-identical yes (1557 bytes). OPEN: none */
// FUNCTION: EVNOVA 0x005010f0
Word ResourceAddName(RezFile *rez, const char *name)
{
    ULong tableSize;
    ULong growth;
    ULong oldSize;
    ULong nameLength;
    ULong count;
    ULong i;
    ULong entries;
    ULong span;
    ULong rest;
    ULong id;
    ULong rezNum;
    RezGroup **newHandle;
    RezName **nameHandle;
    RezGroup *group;
    RezGroup *newGroup;
    RezGroup *next;
    ULong left;
    RezEntry *entry;
    RezName *where;
    RezName *names;
    const char *colon;

    rezNum = ResourceGetRezNum(rez, name);
    if (rezNum == (Word)-1) {
        if ((Word8)name[0] >= '0' && (Word8)name[0] < (Word8)':') {
            colon = strchr(name, ':');
            if (colon != 0)
                name = colon + 1;
        }
        nameLength = strlen(name) + 1;
        if (rez->groupCount == 0) {
            newHandle = (RezGroup **)AllocAHandle2(nameLength +kGroupHeader + kEntryStride, 0);
            if (newHandle != 0) {
                rez->groupCount = 1;
                rez->nameCount = 1;
                rez->groups = newHandle;
                group = *newHandle;
                group->count = 1;
                group->RezNum = 1;
                kFirstEntry(group)->length = 0;
                kFirstEntry(group)->MemHand = 0;
                kFirstEntry(group)->FileOffset = 0;
                kFirstEntry(group)->nameFlags = kGroupHeader + kEntryStride;
                memcpy(kFirstEntry(group) + 1, name, nameLength);
                nameHandle = (RezName **)AllocAHandle2(sizeof(RezName), 0);
                rez->names = nameHandle;
                if (nameHandle != 0) {
                    names = *nameHandle;
                    names->RezNum = 1;
                    names->HashOffset = kGroupHeader + kEntryStride;
                    return 1;
                }
                DeallocAHandle(newHandle);
                rez->groups = 0;
                rez->groupCount = 0;
                rez->nameCount = 0;
            }
            return (Word)-1;
        }
        /* the number: 1 when the first group starts above it, else one past the first group */
        count = rez->groupCount;
        growth = kEntryStride;
        group = *rez->groups;
        if (group->RezNum == 1) {
            rezNum = group->count + 1;
            if (count >= 2 && kNextGroup(group)->RezNum == rezNum + 1) {
                growth -= kGroupHeader;
                --count;
            }
        } else {
            if (group->RezNum >= 3) {
                growth += kGroupHeader;
                ++count;
            }
            rezNum = 1;
        }
        oldSize = GetAHandleSize((Handle)rez->groups);
        tableSize = oldSize + growth;
        newHandle = (RezGroup **)AllocAHandle2(tableSize + nameLength, 0);
        if (newHandle == 0)
            return (Word)-1;

        /* the first group, grown by one entry (and joined to the next when they now touch) */
        i = rez->groupCount;
        group = *rez->groups;
        newGroup = *newHandle;
        entries = group->count;
        span = entries * kEntryStride;
        next = (RezGroup *)((char *)group + (span + kGroupHeader));
        if (rezNum == 1) {
            newGroup->RezNum = 1;
            if (group->RezNum < 3) {
                newGroup->count = entries + 1;
                memcpy(kFirstEntry(newGroup) + 1, kFirstEntry(group), span);
                newGroup = (RezGroup *)((char *)newGroup + span + kGroupHeader + kEntryStride);
            } else {
                newGroup->count = 1;
                next = group;
                newGroup = (RezGroup *)((char *)newGroup + kGroupHeader + kEntryStride);
            }
        } else {
            newGroup->count = entries + 1;
            newGroup->RezNum = group->RezNum;
            memcpy(kFirstEntry(newGroup), kFirstEntry(group), span);
            if (i - 1 != 0 && next->RezNum == rezNum + 1) {
                newGroup->count += next->count;
                span = next->count * kEntryStride;
                memcpy(kFirstEntry(newGroup) + (entries + 1), kFirstEntry(next), span);
                next = (RezGroup *)((char *)next + span + kGroupHeader);
            }
            newGroup = (RezGroup *)((char *)newGroup + (newGroup->count * kEntryStride + kGroupHeader));
        }

        /* the other groups and the old names as they were, the new name last */
        rest = oldSize - ((char *)next - *(char **)rez->groups);
        memcpy(newGroup, next, rest);
        newGroup = (RezGroup *)((char *)newGroup + rest);
        memcpy(newGroup, name, nameLength);

        /* every name offset moves by the growth; the new entry points at the new name */
        i = count;
        group = *newHandle;
        do {
            left = group->count;
            entry = kFirstEntry(group);
            id = group->RezNum;
            do {
                if (rezNum == id) {
                    entry->MemHand = 0;
                    entry->FileOffset = 0;
                    entry->length = 0;
                    entry->nameFlags = tableSize;
                } else if (entry->nameFlags & kNameMask) {
                    entry->nameFlags += growth;
                }
                ++id;
                ++entry;
            } while (--left);
            group = (RezGroup *)entry;
        } while (--i);
        DeallocAHandle(rez->groups);
        rez->groups = newHandle;
        rez->groupCount = count;

        /* the name index, kept sorted */
        nameHandle = rez->names;
        if (nameHandle == 0) {
            nameHandle = (RezName **)AllocAHandle2(sizeof(RezName), 0);
            if (nameHandle == 0) {
                rez->nameCount = 0;
                return rezNum;
            }
            rez->names = nameHandle;
            rez->nameCount = 1;
            names = *nameHandle;
            names->RezNum = rezNum;
            names->HashOffset = tableSize;
            return rezNum;
        }
        i = rez->nameCount;
        names = *nameHandle;
        do {
            names->HashOffset += growth;
            ++names;
        } while (--i);
        i = rez->nameCount + 1;
        nameHandle = (RezName **)ResizeAHandle((Handle)nameHandle, i * sizeof(RezName));
        rez->names = nameHandle;
        if (nameHandle != 0) {
            ResourceFindName(rez, name, &where);
            names = (RezName *)(void *)where;
            memmove(where + 1, where, (i - 1) * sizeof(RezName) - ((char *)names - (char *)*nameHandle));
            names->RezNum = rezNum;
            names->HashOffset = tableSize;
            rez->nameCount = i;
        } else {
            rez->nameCount = 0;
        }
    }
    return rezNum;
}

/* ResourceLoadByName. STATE: DIFF 0.0%, exe 11 / obj 9 instrs. OPEN: foreign-compiler ceiling; see notes */
// FUNCTION: EVNOVA 0x00501710
char *ResourceLoadByName(struct RezFile *rez, const char *name)
{
    return LockAHandle(ResourceLoadHandleByName(rez, name));
}

/* ResourceLoadHandle (BurgerLib RzResourceManager): resource rezNum in a handle, from memory,
   from a file of its own name, or read (and unpacked) from the .rez; 0 when it cannot be had.
   Sets ResourceJustLoaded when the data came off the disk. STATE: MATCH 100.0%, exe 410 /
   obj 410, byte-identical yes (1426 bytes); calls exact (23 sites). OPEN: none */
// FUNCTION: EVNOVA 0x00501740
Handle ResourceLoadHandle(RezFile *Input, long RezNum)
{
    void **BufferHand;
    ULong memFlags;
    ULong Offset;
    ULong DataLength;
    ULong flags;
    RezEntry *Entry;
    ULong PackedSize;
    unsigned char *PackedPtr;
    FILE *fp;
    ULong FileNameOffset;
    ULong ChunkSize;
    ULong size;
    RezDecompressor decompress;
    char *FileName;

    ResourceJustLoaded = 0;
    Entry = ResourceScan(Input, RezNum);
    if (Entry == 0)
        return 0;
    BufferHand = Entry->MemHand;
    Offset = Entry->FileOffset;
    DataLength = Entry->length;
    flags = Entry->nameFlags;
    PackedSize = Entry->fileLength;
    Entry->nameFlags = flags + kLoadCountOne;
    if (BufferHand != 0) {
        if (*BufferHand) {
            SetHandlePurgeFlag(BufferHand, 0);
            return (Handle)BufferHand;
        }
        Entry->MemHand = 0;
        DeallocAHandle(BufferHand);
        BufferHand = 0;
    }
    memFlags = (ULong)RezNum << kMemIdShift;
    if (flags & kRezFixed)
        memFlags |= kHandleFixed;
    FileNameOffset = flags & kNameMask;
    if (FileNameOffset != 0 && (Input->flags & kFlagExternalOk) &&
        (!(flags & kRezTriedExternal) || (flags & kRezIsExternal))) {
        FileName = LockAHandle((Handle)Input->groups) + FileNameOffset;
        fp = (FILE *)FOpenExpandedPath(FileName, "rb");
        if (fp != 0) {
            size = fgetfilesize(fp);
            if (size != 0) {
                BufferHand = (void **)AllocAHandle2(size, memFlags);
                if (BufferHand != 0 &&
                    fread(LockAHandle((Handle)BufferHand), 1, size, fp) != size) {
                    DeallocAHandle(BufferHand);
                    BufferHand = 0;
                }
            }
            fclose(fp);
        }
        UnlockAHandle((Handle)Input->groups);
        Entry = ResourceScan(Input, RezNum);
        Entry->nameFlags |= kRezTriedExternal | kRezIsExternal;
        if (BufferHand != 0)
            goto Loaded;
        Entry->nameFlags &= ~kRezIsExternal;
    }
    fp = (FILE *)Input->file;
    if (fp != 0 && Offset != 0) {
        fseek(fp, Offset, SEEK_SET);
        if (flags & kRezPackedMask) {
            decompress = Input->decompress[((flags >> kRezPackedShift) & 3) - 1];
            if (decompress == 0)
                goto Fail;
            if (DataLength == 0) {
                DataLength = fgetlong(fp);
                Entry->length = DataLength;
                Entry->FileOffset += 4;
                Entry->fileLength -= 4;
            }
            BufferHand = (void **)AllocAHandle2(DataLength, memFlags);
            if (BufferHand == 0)
                goto Fail;
            ChunkSize = PackedSize < kReadChunk ? PackedSize : kReadChunk;
            PackedPtr = (unsigned char *)AllocAPointer(ChunkSize);
            if (PackedPtr == 0)
                goto FreeFail;
            do {
                size = ChunkSize < PackedSize ? ChunkSize : PackedSize;
                if (fread(PackedPtr, 1, size, fp) != size)
                    goto FreeFail;
                decompress((unsigned char *)LockAHandle((Handle)BufferHand), PackedPtr, DataLength, size);
                DataLength = 0;
                PackedSize -= size;
            } while (PackedSize != 0);
            decompress(0, 0, 0, 0);
            DeallocAPointer(PackedPtr);
        } else {
            BufferHand = (void **)AllocAHandle2(DataLength, memFlags);
            if (BufferHand == 0 || fread(LockAHandle((Handle)BufferHand), 1, DataLength, fp) != DataLength)
                goto FreeFail;
        }
Loaded:
        UnlockAHandle((Handle)BufferHand);
        ResourceJustLoaded = 1;
        Entry = ResourceScan(Input, RezNum);
        Entry->MemHand = BufferHand;
        return (Handle)BufferHand;
    }
    goto Fail;
FreeFail:
    DeallocAHandle(BufferHand);
Fail:
    Entry = ResourceScan(Input, RezNum);
    Entry->nameFlags &= ~kLoadCountMask;
    Entry->MemHand = 0;
    return 0;
}

/* ResourceLoadHandleByName. STATE: DIFF 15.6%, exe 32 / obj 30 instrs. OPEN: see notes */
// FUNCTION: EVNOVA 0x00501ce0
Handle ResourceLoadHandleByName(struct RezFile *rez, const char *name)
{
    unsigned int num;

    num = ResourceGetRezNum(rez, name);
    if (num == kNoRezNum) {
        num = ResourceAddName(rez, name);
        if (num == kNoRezNum)
            return 0;
    }
    return ResourceLoadHandle(rez, num);
}

/* ResourceRelease (BurgerLib RzResourceManager): one load fewer; at none the handle is
   unlocked and made purgeable, or freed outright for a don't-cache file. STATE: MATCH
   100.0%, exe 73 / obj 73, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00501d50
void ResourceRelease(RezFile *rez, long index)
{
    RezEntry *entry;
    void **handle;
    ULong flags;

    entry = ResourceScan(rez, index);
    if (entry != 0) {
        flags = entry->nameFlags;
        if (flags >= kLoadCountOne) {
            entry->nameFlags = flags - kLoadCountOne;
            if (((flags - kLoadCountOne) & kLoadCountMask) == 0) {
                handle = entry->MemHand;
                if (handle != 0) {
                    UnlockAHandle((Handle)handle);
                    if (rez->flags & kFlagDontCache) {
                        entry->MemHand = 0;
                        DeallocAHandle(handle);
                    } else {
                        SetHandlePurgeFlag(handle, kPurgeable);
                    }
                }
            }
        }
    }
}

/* ResourceDetach (BurgerLib RzResourceManager): the caller keeps the handle; the file
   forgets it. STATE: MATCH 100.0%, exe 43 / obj 43, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00501e30
void ResourceDetach(RezFile *rez, long index)
{
    RezEntry *entry;

    entry = ResourceScan(rez, index);
    if (entry != 0) {
        entry->MemHand = 0;
        entry->nameFlags &= ~kLoadCountMask;
    }
}

/* ResourceGetRezNum (BurgerLib RzResourceManager): the number of a named resource, or -1.
   STATE: MATCH 100.0%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00501eb0
Word ResourceGetRezNum(RezFile *Input, const char *name)
{
    RezName *NamePtr;

    if (ResourceFindName(Input, name, &NamePtr) != 0)
        return NamePtr->RezNum;
    return (Word)-1;
}

/* ResourceGetIDFromHandle (BurgerLib RzResourceManager): which resource a loaded handle is,
   and its name; 0 when found, 1 when not. STATE: MATCH 100.0%, exe 79 / obj 79,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00501ef0
Word ResourceGetIDFromHandle(RezFile *rez, Handle h, long *idFound, char *nameBuf,
                             Word nameBufSize)
{
    char *text;
    RezGroup *group;
    ULong groups;
    ULong entries;
    char *src;
    RezEntry *entry;
    ULong length;

    if (rez != 0 && h != 0) {
        groups = rez->groupCount;
        if (groups != 0) {
            group = *rez->groups;
            text = (char *)group;
            do {
                entries = group->count;
                entry = kFirstEntry(group);
                do {
                    if (entry->MemHand == (void **)h) {
                        if (idFound != 0)
                            *idFound = group->RezNum + group->count - entries;
                        if (nameBuf != 0 && nameBufSize != 0) {
                            if ((entry->nameFlags & kNameMask) == 0) {
                                nameBuf[0] = 0;
                            } else {
                                src = text + (entry->nameFlags & kNameMask);
                                length = strlen(src);
                                if (length >= nameBufSize)
                                    length = nameBufSize - 1;
                                nameBuf[length] = 0;
                                memcpy(nameBuf, src, length);
                            }
                        }
                        return 0;
                    }
                    ++entry;
                } while (--entries);
                group = (RezGroup *)entry;
            } while (--groups);
        }
    }
    return 1;
}

#ifdef __cplusplus
}
#endif
