/* atfiletoolmac.h -- 1.0.10: AmbrosiaTools' file tool on Mac: its records and the calls its objects make into one another. Notes: Research/notes/maclibs.md */
#ifndef ATFILETOOLMAC_H
#define ATFILETOOLMAC_H
#include <string.h>
#define memset(p, c, n) MemoryClear(p, n)
#define memmove(d, s, n) BlockMoveData(s, d, n)
#define memcpy(d, s, n) BlockMoveData(s, d, n)

#include "burgertypes.h"
#include "burgerplat.h"
#if TARGET_API_MAC_CARBON
#define MemoryClear(p, n) BlockZero(p, n)
#endif

/* the release build's empty error report */
#define FT_ReportError(e) ((int32_t)(e))

typedef int32_t (*FileProc)();

typedef struct FTProcs {
    FileProc proc[6];                            /* indexed by kFTRead .. kFTSeek */
} FTProcs;

typedef struct FTLink {
    struct FTFile *next;
} FTLink;                                        /* a file's place in the open-file list, and the list's head */

typedef struct FTFile {
    FTLink link;                                 /* +0x000 open-file list */
    ULong size;                                  /* +0x004 */
    ULong position;                              /* +0x008 */
    int32_t refCount;                               /* +0x00c */
    unsigned char mode;                          /* +0x010 */
    FTProcs procs;                               /* +0x014 one of the three FT_Open tables */
    short refNum;                                /* +0x02c */
    short vRefNum;                               /* +0x02e */
    int32_t dirID;                                  /* +0x030 */
    void **memHandle;                            /* +0x134 a file over a memory handle */
    struct FTFile *parent;                       /* +0x138 a file inside another file */
    int32_t origin;                                 /* +0x13c */
    void **buffer;                               /* +0x140 */
    void **buffer2;                              /* +0x144 */
} FTFile;

typedef struct FolderSpec {
    struct FolderSpec **next;                    /* +0x000 registered-spec list */
    FTFile *dir;                                 /* +0x004 */
    FTFile *file;                                /* +0x008 */
    int32_t volumeDate;                             /* +0x00c the volume's creation date */
    FSSpec spec;                                 /* +0x010 */
    short pad56;                                 /* +0x056 */
    unsigned char names[1];                      /* +0x058 Pascal names, 0-terminated */
} FolderSpec;

/* The directory of a file that holds files; big-endian, in the parent's buffer handles. */
typedef struct FTArchiveHeader {
    char signature[4];                           /* +0x00 "CHNK" */
    ULong headerSize;                            /* +0x04 */
    ULong tableOffset;                           /* +0x08 */
    ULong entrySize;                             /* +0x0c */
    ULong entryCount;                            /* +0x10 */
} FTArchiveHeader;

typedef struct FTArchiveEntry {
    char name[0x40];                             /* +0x00 "" for a free slot */
    ULong flags;                                 /* +0x40 kEntryReadable.. */
    ULong offset;                                /* +0x44 */
    ULong length;                                /* +0x48 */
    ULong type;                                  /* +0x4c */
} FTArchiveEntry;

typedef struct FTMark {
    FTFile *file;
    ULong position;
} FTMark;

#define kArchiveHeaderSize  sizeof(FTArchiveHeader)

/* atftopen.c */
extern FolderSpec **gFT_PathList;                /* the registered specs */
extern FTLink gFT_FileList;                      /* the open files */
extern FolderSpec **gFT_Path;                    /* the base spec */
extern FileProc gFT_FileHooks[6];                /* the native procedures */
extern FileProc gFT_ChunkHooks[6];               /* the member procedures */

/* atfilespec.c */
int32_t FT_FileClose(void *ref);

/* atfilespec2.c */
int32_t _FT_PathModify(FolderSpec **base, void *where, FolderSpec ***result, unsigned char create);
void _FT_PathInsert(FolderSpec **spec);
unsigned char _FT_PathLookup(FolderSpec **spec);
void _FT_PathDispose(void *ref);
int32_t _FT_PathAppend(FolderSpec **spec, const unsigned char *name);
ULong _FT_PathWalk(FolderSpec **spec, ULong offset);
int32_t _FT_PathNameInput(unsigned char *out, const char *in, ULong length);
int32_t _FT_PathNameOutput(char *out, const unsigned char *name);
int32_t _FT_FileOpenWrapper(FolderSpec **spec, unsigned char write, FTFile **result);
unsigned char _FT_FileIsChunky(FolderSpec **spec, ULong *count);
unsigned char _FT_IndirectIsChunky(void **handle);
int32_t _FT_FileUpdateRefCount(FTFile *file, int32_t delta);

/* atfilespec3.c */
int32_t _FT_PathGoTo(FolderSpec **spec, int32_t folder);
int32_t _FT_PathGoToVolume(FolderSpec **spec, ULong index);
int32_t _FT_PathGoToVolumeName(FolderSpec **spec, const unsigned char *volume);
#define kFTErrNoMoreEntries -7203
typedef struct FTDirEntry {
    int32_t id;
    short vRefNum;
} FTDirEntry;

int32_t _FT_PathGoToParent(FolderSpec **spec, unsigned char *leaf);
int32_t _FT_PathGoToChild(FolderSpec **spec, const unsigned char *child, unsigned char last);
int32_t _FT_PathValidate(FolderSpec **spec);
int32_t _FT_PathGoToID(FolderSpec **spec, short vRefNum, int32_t dirID);
int32_t _FT_PathClearNames(FolderSpec **spec);
int32_t _FT_FileSetFlags(FolderSpec **spec, FTFileInfo *info);
unsigned char _FT_IsPathAFile(FolderSpec **spec);
int32_t _FT_FileCreate(FolderSpec **spec, FTFileInfo *info);
int32_t _FT_FileDelete(FolderSpec **spec);
int32_t _FT_FileGetDates(FolderSpec **spec, ULong *created, ULong *modified);
int32_t _FT_FileGetFlags(FolderSpec **spec, FTFileInfo *info, unsigned char checkArchive);
FTFile *_FT_IsFileOpen(FolderSpec **spec);
int32_t _FT_FileOpen(FolderSpec **spec, unsigned char write, FTFile *file);
int32_t _FT_FileClose(FTFile *file);
#if TARGET_API_MAC_CARBON
int32_t FT_DirGetEntry(int32_t where, ULong index, FTFileInfo *info, unsigned char flags);
int32_t _FT_ChunkDirEntry(FTFile *archive, ULong index, FTFileInfo *info, FolderSpec **spec);
int32_t _FT_FileDirEntry(FolderSpec **spec, ULong index, FTFileInfo *info, FTDirEntry *out, unsigned char flags);
#endif
int32_t _FT_RemapError(int32_t err);
int32_t _FT_ChunkFilePreflight(FTFile *file, FTMark *mark);
int32_t _FT_ChunkFilePostflight(FTFile *file, FTMark *mark);

/* atfilearchive.c */
FTFile *_FT_IsChunkOpen(FTFile *archive, const char *name);
int32_t _FT_ChunkOpenPath(FolderSpec **spec, unsigned char write, unsigned char toLast,
                  unsigned char mustBeArchive, FTFile **result, char *leaf);
int32_t _FT_ChunkOpen(FTFile *archive, FTFile *file, const char *name, unsigned char write,
                  unsigned char nested);
int32_t _FT_ChunkClose(FTFile *file);
int32_t _FT_ChunkLoad(FTFile *file);
unsigned char _FT_ChunkLookup(FTFile *archive, const char *name, ULong *index,
                           FTArchiveEntry *entry);
int32_t _FT_ChunkFormat(FTFile *file, ULong entries);
int32_t _FT_ChunkResize(FTFile *archive, ULong offset, ULong length, ULong newLength,
                  ULong *newOffset);
int32_t _FT_ChunkCreate(FTFile *archive, FTFileInfo *info);
int32_t _FT_ChunkDelete(FTFile *archive, const char *name);
int32_t _FT_ChunkSetFlags(FTFile *archive, const char *name, FTFileInfo *info);
int32_t _FT_ChunkGetFlags(FTFile *archive, const char *name, const char *member, FTFileInfo *info,
                  FolderSpec **spec);
int32_t _FT_ChunkGetCount(FTFile *archive, const char *name, ULong *count);
FTFile *_FT_IsBufferOpen(void **handle);
int32_t _FT_BufferClose(FTFile *file);

#define kSecondsPerDay      86400
static inline ULong FTStrLen(const char *s)
{
    const unsigned char *p;
    ULong n;

    n = -1;
    p = (const unsigned char *)s - 1;
    do {
        n++;
    } while (*++p != 0);
    return n;
}

static inline char *FTStrCpy(char *dst, const char *src)
{
    unsigned char *q;
    const unsigned char *p;

    q = (unsigned char *)dst - 1;
    p = (const unsigned char *)src - 1;
    while ((*++q = *++p) != 0)
        ;
    return dst;
}

#undef ATSwap32
#define ATSwap32(v)         (v)                  /* the directory is big-endian, as this machine */

static inline char *FTPToCStr(char *dst, const unsigned char *src)
{
    ULong n;
    unsigned char *q;
    const unsigned char *p;

    n = src[0] + 1;
    q = (unsigned char *)dst - 1;
    p = src;
    while (--n != 0)
        *++q = *++p;
    q[1] = 0;
    return dst;
}

static inline int32_t FTStrCmp(const char *a, const char *b)
{
    ULong ca;
    const unsigned char *s1;
    const unsigned char *s2;
    ULong cb;

    s1 = (const unsigned char *)a - 1;
    s2 = (const unsigned char *)b - 1;
    while ((ca = *++s1) == (cb = *++s2)) {
        if (ca == 0)
            return 0;
    }
    return ca < cb ? (signed char)-1 : (signed char)1;
}

#define kCopyChunk          0x8000
#define kArchiveEntrySize   sizeof(FTArchiveEntry)
#define ArchiveEntry(f, i)  ((FTArchiveEntry *)((char *)*(f)->buffer2 \
                             + (i) * ATSwap32(((FTArchiveHeader *)*(f)->buffer)->entrySize)))

#endif
