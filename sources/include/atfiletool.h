/* atfiletool.h -- AmbrosiaTools' file tool (FT_*): its records, the open-file and folder-spec
   lists, and the calls its objects (atftopen.c, atfilespec.c, atfilespec2.c, atfilespec3.c,
   atfilearchive.c, atfiletypes.c) make into one another. Notes: Research/notes/atfilespec.md */
#ifndef ATFILETOOL_H
#define ATFILETOOL_H

#include "burgertypes.h"
#include "burgerplat.h"
#include "win32.h"

typedef int32_t (*FileProc)();

typedef struct FTProcs {
    FileProc proc[6];                            /* indexed by kFTRead .. kFTSeek */
} FTProcs;

typedef struct FTFile {
    struct FTFile *next;                         /* +0x000 open-file list */
    ULong size;                                  /* +0x004 */
    ULong position;                              /* +0x008 */
    int32_t refCount;                               /* +0x00c */
    unsigned char mode;                          /* +0x010 */
    FTProcs procs;                               /* +0x014 one of the three FT_Open tables */
    void *hFile;                                 /* +0x02c native file, or kNoFile */
    char path[260];                              /* +0x030 */
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
    char drive[4];                               /* +0x00c "C:" */
    char path[260];                              /* +0x010 the rest, from the root */
    unsigned char names[1];                      /* +0x114 Pascal names, 0-terminated */
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

/* the special-folder lookups, resolved from shell32 / advapi32 / userenv at run time */
typedef int32_t (__stdcall *SHGetFolderPathProc)(void *hwnd, int csidl, void *token,
                                              ULong flags, char *path);
typedef int (__stdcall *OpenProcessTokenProc)(void *process, ULong access, void **token);
typedef int (__stdcall *UserProfileDirProc)(void *token, char *path, ULong *size);
typedef int (__stdcall *AllUsersProfileDirProc)(char *path, ULong *size);

typedef struct BY_HANDLE_FILE_INFORMATION {
    uint32_t dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    uint32_t dwVolumeSerialNumber;
    uint32_t nFileSizeHigh;
    uint32_t nFileSizeLow;
    uint32_t nNumberOfLinks;
    uint32_t nFileIndexHigh;
    uint32_t nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION;

/* atftopen.c */
extern FolderSpec **gFT_PathList;                /* the registered specs */
extern FTFile *gFT_FileList;                     /* the open files */
extern FolderSpec **gFT_Path;                    /* the base spec */
extern FileProc gFT_FileHooks[6];                /* the native procedures */
extern FileProc gFT_ChunkHooks[6];               /* the member procedures */

/* atfilespec.c */
int32_t FT_FileClose(FTFile *file);

/* atfilespec2.c */
int32_t _FT_PathModify(FolderSpec **base, void *where, FolderSpec ***result, unsigned char create);
void _FT_PathInsert(FolderSpec **spec);
unsigned char _FT_PathLookup(FolderSpec **spec);
int32_t _FT_PathDispose(void *ref);
int32_t _FT_PathAppend(FolderSpec **spec, const unsigned char *name);
ULong _FT_PathWalk(FolderSpec **spec, ULong offset);
int32_t _FT_PathNameInput(unsigned char *out, const char *in, ULong length);
int32_t _FT_PathNameOutput(char *out, const unsigned char *name);
int32_t _FT_FileOpenWrapper(FolderSpec **spec, unsigned char write, FTFile **result);
unsigned char _FT_FileIsChunky(FolderSpec **spec, ULong *count);
unsigned char _FT_IndirectIsChunky(void **handle);
int32_t _FT_FileUpdateRefCount(FTFile *file, int32_t delta);

/* atfilespec3.c */
int32_t _FT_PathGoTo(void *ref, int32_t folder);
unsigned char _FT_IsPathAFile(FolderSpec **spec);
int32_t _FT_PathGoToDrive(FolderSpec **spec, char drive, const char *dirPath);
int32_t _FT_FileCreate(FolderSpec **spec, FTFileInfo *info);
int32_t _FT_FileDelete(FolderSpec **spec);
int32_t _FT_FileGetDates(FolderSpec **spec, ULong *created, ULong *modified);
int32_t _FT_FileGetFlags(FolderSpec **spec, FTFileInfo *info, unsigned char checkArchive);
FTFile *_FT_IsFileOpen(FolderSpec **spec);
int32_t _FT_FileOpen(FolderSpec **spec, unsigned char write, FTFile *file);
int32_t _FT_FileClose(FTFile *file);
int32_t _FT_RemapError(ULong winError);
ULong _FT_FileTimeToSeconds(const FILETIME *time);
int32_t _FT_PathFullPath(char *dir, const char *relPath);
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
int32_t _FT_ChunkGetFlags(FTFile *archive, const char *name, const char *member, FTFileInfo *info,
                  FolderSpec **spec);
int32_t _FT_ChunkGetCount(FTFile *archive, const char *name, ULong *count);
FTFile *_FT_IsBufferOpen(void **handle);
int32_t _FT_BufferClose(FTFile *file);

#define kCopyChunk          0x8000
#define kArchiveEntrySize   sizeof(FTArchiveEntry)
#define ArchiveEntry(f, i)  ((FTArchiveEntry *)((char *)*(f)->buffer2 \
                             + ATSwap32(((FTArchiveHeader *)*(f)->buffer)->entrySize) * (i)))

#endif
