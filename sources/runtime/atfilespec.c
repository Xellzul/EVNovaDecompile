// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atfilespec.c -- the file tool: FT_Open and FT_Close, the folder specs and open-file records, the special-folder lookup FT_Open runs, the handle-release hook, and the close path FT_Close runs. Notes: Research/notes/atfilespec.md */

#include <stdlib.h>
#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "macport.h"
#include "win32.h"
#include "atfiletool.h"
#define ATSTR_MOVE
#define ATSTR_LEN
#define ATSTR_JOIN
#define ATSTR_CMP
#include "atfilestr.h"

#define kArchiveType        'CHNK'

extern void **_FT_FileTypesCreate(void);                /* atfiletypes.c */

FolderSpec **gFT_Path;                       /* 0x008700f8 */
FileTypeMap **gFT_FileTypes;                     /* 0x00870930 */
FolderSpec **gFT_PathList;                       /* 0x008701b4 registered specs */
FTFile *gFT_FileList;                            /* 0x00870138 open files */
unsigned char gFT_Open;                      /* 0x00870e08 */
static unsigned char DAT_00865b00[8];            /* 0x00865b00 entropy */
unsigned char *DAT_00576ce8 = DAT_00865b00;      /* 0x00576ce8 the stir pointer */
static const char kSlash[] = "/";                /* 0x00576cec, unreferenced */

FileProc gFT_FileHooks[6];                       /* 0x0086e868 */
FileProc gFT_ChunkHooks[6];                      /* 0x0086fa98 */
FileProc gFT_BufferHooks[6];                     /* 0x0086dc70 */
/* Placement only: no reader in the exe. */
void *gFT_CopyBuffer;
ULong gFT_CopyLength;

#define kContextSize        0x115

/* FT_Open. STATE: MATCH 100%, exe 79 / obj 79, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004da12a
long FT_Open(void)
{
    long err, e;

    err = 0;
    if (gFT_Open != 0)
        goto done;

    e = PlatformOpen();
    if (e != 0) {
        err = e;
        goto done;
    }
    if (gIndirectBusy == 0)
        gIndirectBusy = _FT_IsIndirectBusy;
    SystemSetQuitProc(FT_Close);

    gFT_FileHooks[0] = _FT_FileRead;
    gFT_FileHooks[1] = _FT_FileWrite;
    gFT_FileHooks[2] = _FT_FileGetEOF;
    gFT_FileHooks[3] = _FT_FileSetEOF;
    gFT_FileHooks[4] = _FT_FileGetMark;
    gFT_FileHooks[5] = _FT_FileSetMark;

    gFT_ChunkHooks[0] = _FT_ChunkRead;
    gFT_ChunkHooks[1] = _FT_ChunkWrite;
    gFT_ChunkHooks[2] = _FT_ChunkGetEOF;
    gFT_ChunkHooks[3] = _FT_ChunkSetEOF;
    gFT_ChunkHooks[4] = _FT_ChunkGetMark;
    gFT_ChunkHooks[5] = _FT_ChunkSetMark;

    gFT_BufferHooks[0] = _FT_BufferRead;
    gFT_BufferHooks[1] = _FT_BufferWrite;
    gFT_BufferHooks[2] = _FT_BufferGetEOF;
    gFT_BufferHooks[3] = _FT_BufferSetEOF;
    gFT_BufferHooks[4] = _FT_BufferGetMark;
    gFT_BufferHooks[5] = _FT_BufferSetMark;

    gFT_Path = (FolderSpec **)IndirectAllocate(kContextSize);
    if (gFT_Path == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(*gFT_Path, 0, kContextSize);

    e = _FT_PathGoTo(gFT_Path, -2);
    if (e != 0) {
        err = e;
        goto done;
    }
    gFT_FileTypes = (FileTypeMap **)_FT_FileTypesCreate();
    if (gFT_FileTypes == 0) {
        err = kOutOfMemory;
        goto done;
    }
    DAT_00576ce8 = MemoryAllocate(4);
    if (DAT_00576ce8 == 0) {
        err = kOutOfMemory;
        goto done;
    }
    DAT_00576ce8 = DAT_00576ce8 + kStirPointerBackoff;
    *DAT_00576ce8 = DAT_00865b00[0];
    DAT_00865b00[0] = 0;
    gFT_Open = 1;

done:
    if (err != 0)
        FT_Close();
    return err;
}

/* FT_Close. STATE: MATCH 100%, exe 53 / obj 53, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004da2ad
void FT_Close(void)
{
    long err;
    FolderSpec **folder;
    FTFile *file;

    err = 0;
    file = 0;
    folder = 0;
    if (gFT_Path != 0) {
        _FT_PathDispose(gFT_Path);
        gFT_Path = 0;
    }
    if (gFT_FileTypes != 0) {
        IndirectDeallocate((void **)gFT_FileTypes);
        gFT_FileTypes = 0;
    }

    while ((folder = gFT_PathList) != 0)
        _FT_PathDispose(folder);

    while ((file = gFT_FileList) != 0)
        FT_FileClose(file);

    if (DAT_00576ce8 != 0 && DAT_00576ce8 != DAT_00865b00) {
        DAT_00865b00[0] = *DAT_00576ce8;
        DAT_00576ce8 -= kStirPointerBackoff;
        MemoryDeallocate(DAT_00576ce8);
        DAT_00576ce8 = DAT_00865b00;
    }

    if (gIndirectBusy == _FT_IsIndirectBusy)
        gIndirectBusy = 0;
    gFT_Open = 0;
}

long FT_FileCreate(long where, FTFileInfo *info);
long FT_FileOpen(long where, const char *name, unsigned char write, FTFile **result);
long FT_FileFlush(void *ref);
long FT_FileRead(void *ref, ULong count, void *buffer);
long FT_FileWrite(FTFile *file, ULong count, const void *buffer);
long FT_FileGetEOF(FTFile *file, ULong *size);
long FT_FileSetEOF(FTFile *file, ULong size);
long FT_FileSetMark(FTFile *file, ULong position);
long _FT_FileSaveAppend(long where, const char *name, unsigned char replace, unsigned char append,
                  ULong type, ULong creator, void **handle);

long FT_VirtualFileOpen(void **handle, unsigned char mode, FTFile **result);

/* FT_PathCreate (FT a registered spec for `name' in folder `where'; a folder number alone
   when no name is given). STATE: MATCH 100%, exe 62 / obj 62, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004da370
long FT_PathCreate(FolderSpec **where, const char *name, FolderSpec ***result)
{
    long err;
    long e;
    FolderSpec **spec;

    err = 0;
    spec = 0;
    *result = 0;
    if ((long)where >= kFolderShared && (long)where <= kFolderApplication &&
        name != (const char *)kNoLength && (name == 0 || *name == 0)) {
        *result = where;
    } else {
        e = _FT_PathModify(gFT_Path, where, &spec, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _FT_PathModify(spec, (void *)name, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        _FT_PathInsert(spec);
        *result = spec;
        spec = 0;
    }
done:
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

/* FT_PathCurrent (FT do both names resolve against the base spec). STATE: MATCH 100%, exe 24 / obj 24, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004da40f
long FT_PathCurrent(const char *first, const char *second)
{
    long err;
    long e;

    err = 0;
    e = _FT_PathModify(gFT_Path, (void *)first, 0, 0);
    if (e != 0) {
        err = e;
    } else {
        e = _FT_PathModify(gFT_Path, (void *)second, 0, 0);
        if (e != 0)
            err = e;
    }
    return err;
}

/* FT_PathDispose (FT dispose a spec; kNotFound for a handle that is not a registered spec).
   STATE: MATCH 100%, exe 21 / obj 21, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004da450
long FT_PathDispose(FolderSpec **spec)
{
    long err;

    err = 0;
    if (!IsSpecID(spec) && !_FT_PathLookup(spec))
        err = kNotFound;
    else
        err = _FT_PathDispose(spec);
    return err;
}

/* FT_FileCreate (FT create: a file or folder named by info in folder `where`, or a member of
   a file of files; a folder inside a file of files is a nested one). STATE: MATCH 100%,
   exe 197 / obj 197, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004da480
long FT_FileCreate(long where, FTFileInfo *info)
{
    long err, e;
    ULong entries;
    char leaf[256];
    FTFile *archive;
    FTFile *file;
    FolderSpec **spec;
    FTFileInfo copy;

    err = 0;
    entries = kDefaultEntries;
    archive = 0;
    file = 0;
    spec = 0;
    e = _FT_PathModify(gFT_Path, (void *)where, &spec, 0);
    if (e != 0)
        goto fail;
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0
        || ((*spec)->names[0] != 0 ? 1 : 0) != 0) {
        e = _FT_PathModify(spec, info, 0, 0);
        if (e != 0)
            goto fail;
        e = _FT_ChunkOpenPath(spec, 1, 1, 0, &archive, leaf);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _FT_ChunkCreate(archive, info);
        if (e != 0) {
            err = e;
            goto done;
        }
        goto done;
    }
    if (_FT_IsPathAFile(spec)) {
        e = _FT_ChunkOpenPath(spec, 1, 0, 0, &archive, 0);
        if (e != 0)
            goto fail;
        e = _FT_ChunkCreate(archive, info);
        if (e != 0) {
            err = e;
            goto done;
        }
        goto done;
    }
    if (info->flags[kFTInfoArchive]) {
        if (info->flags[kFTInfoFolder] && info->size != 0 && info->size <= kMaxEntries)
            entries = info->size;
        copy = *info;
        copy.flags[kFTInfoArchive] = 0;
        copy.flags[kFTInfoFolder] = 0;
        copy.size = 0;
        copy.type = kArchiveType;
        e = _FT_FileCreate(spec, &copy);
        if (e != 0)
            goto fail;
        if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
            e = _FT_ChunkOpenPath(spec, 1, 0, 0, &archive, 0);
            if (e != 0)
                goto fail;
            e = _FT_ChunkFormat(archive, entries);
            if (e != 0) {
                err = e;
                goto done;
            }
            goto done;
        }
        e = _FT_FileOpenWrapper(spec, 1, &file);
        if (e != 0)
            goto fail;
        e = _FT_ChunkFormat(file, entries);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
    } else {
        e = _FT_FileCreate(spec, info);
        if (e != 0)
            err = e;
    }
done:
    if (archive != 0)
        FT_FileClose(archive);
    if (file != 0) {
        FT_FileFlush(file);
        FT_FileClose(file);
        if (err != 0)
            _FT_FileDelete(spec);
    }
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

/* FT_FileDelete (FT delete: a file or empty folder, or a member of a file of files; an open
   native file held as a spec's `dir` is closed first). STATE: MATCH 100%, exe 127 / obj 127,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004da738
long FT_FileDelete(long where, const char *name)
{
    long err, e, e2;
    int isFile;
    char leaf[256];
    FTFile *archive;
    FTFile *file;
    FolderSpec **spec;
    FolderSpec **s;

    err = 0;
    archive = 0;
    isFile = 0;
    spec = 0;
    e = _FT_PathModify(gFT_Path, (void *)where, &spec, 0);
    if (e != 0)
        goto fail;
    e = _FT_PathModify(spec, (void *)name, 0, 0);
    if (e != 0)
        goto fail;
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0
        && ((*spec)->names[0] != 0 ? 1 : 0) == 0) {
        err = kFTErrNotAFile;
        goto done;
    }
    if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
        e = _FT_ChunkOpenPath(spec, 1, 1, 0, &archive, leaf);
        if (e != 0)
            goto fail;
        e2 = _FT_ChunkDelete(archive, leaf);
        if (e2 != 0) {
            err = e2;
            goto done;
        }
        goto done;
    } else {
        file = 0;
        s = 0;
        file = _FT_IsFileOpen(spec);
        if (file != 0) {
            for (s = gFT_PathList; s != 0; s = (*s)->next) {
                if (file == (*s)->dir) {
                    FT_FileClose(file);
                    (*s)->dir = 0;
                }
            }
        }
        e = _FT_FileDelete(spec);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        goto done;
    }
done:
    if (archive != 0)
        FT_FileClose(archive);
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

/* FT_FileGetFlags (FT file info: name, flags, size, dates, type and creator; a file of files
   reads as the folder "_Virtual_"). STATE: MATCH 100%, exe 162 / obj 162, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x004da8c8
long FT_FileGetFlags(long where, const char *name, FTFileInfo *info, long flags)
{
    long err, e;
    int isFile;
    unsigned char probe, quiet;
    char leaf[256];
    FTFile *file;
    FolderSpec **spec;

    err = 0;
    file = 0;
    isFile = 0;
    spec = 0;
    if (info != 0)
        memset(info, 0, sizeof(FTFileInfo));
    if (info != 0)
        probe = 0;
    else
        probe = 1;
    quiet = probe;
    if ((err = _FT_PathModify(gFT_Path, (void *)where, &spec, probe)) != 0
        || (err = _FT_PathModify(spec, (void *)name, 0, probe)) != 0) {
        if (!probe || err != kFTErrFileNotFound)
            err = err;              /* the release build's empty error report */
        goto done;
    }
    if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
        e = _FT_ChunkOpenPath(spec, 0, 1, 0, &file, leaf);
        if (e == 0)
            e = _FT_ChunkGetFlags(file, leaf, 0, info, spec);
    } else if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0) {
        if (info == 0)
            goto done;
        ATStrCpy(info->name, "_Virtual_");
        info->flags[kFTInfoReadable] = 1;
        info->flags[kFTInfoWritable] = (*spec)->file->mode;
        info->flags[kFTInfoFolder] = 1;
        info->flags[kFTInfoHidden] = 0;
        info->flags[kFTInfoArchive] = 1;
        info->size = 0;
        info->created = 0;
        info->modified = 0;
        e = _FT_ChunkOpenPath(spec, 0, 0, 0, &file, 0);
        if (e == 0) {
            e = _FT_ChunkGetCount(file, 0, &info->size);
            if (e == 0)
                goto done;
        }
    } else {
        e = _FT_FileGetFlags(spec, info, (unsigned char)flags);
    }
    err = e;
done:
    if (file != 0)
        FT_FileClose(file);
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

/* FT_FileOpen (FT open a file, or a member of a file of files, for reading or writing).
   STATE: MATCH 100%, exe 96 / obj 96, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004daaf0
long FT_FileOpen(long where, const char *name, unsigned char write, FTFile **result)
{
    long err, e;
    int isFile, hasName;
    FolderSpec **spec;

    err = 0;
    isFile = 0;
    hasName = 0;
    spec = 0;
    *result = 0;
    e = _FT_PathModify(gFT_Path, (void *)where, &spec, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _FT_PathModify(spec, (void *)name, 0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0
        && ((*spec)->names[0] != 0 ? 1 : 0) == 0) {
        err = kFTErrNotAFile;
        goto done;
    }
    if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
        e = _FT_ChunkOpenPath(spec, write, 0, 0, result, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else {
        e = _FT_FileOpenWrapper(spec, write, result);
        if (e != 0)
            err = e;
    }
done:
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

/* FT_FileClose (FT file close). STATE: MATCH 100%, exe 65 / obj 65, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004dabe9
long FT_FileClose(FTFile *file)
{
    FTFile **link;
    long err;
    long result;

    err = 0;
    if (_FT_FileUpdateRefCount(file, -1) == 0) {
        link = &gFT_FileList;
        while (*link != 0 && *link != file)
            link = &(*link)->next;
        *link = file->next;
        file->next = 0;

        if (file->buffer != 0) {
            result = _FT_ChunkClose(file);
            if (result != 0)
                err = result;
        }
        if (file->parent != 0) {
            result = FT_FileClose(file->parent);
            if (result != 0)
                err = result;
        } else if (file->memHandle != 0) {
            result = _FT_BufferClose(file);
            if (result != 0)
                err = result;
        } else {
            result = _FT_FileClose(file);
            if (result != 0)
                err = result;
        }

        if (err != 0) {
            file->next = gFT_FileList;
            gFT_FileList = file;
        } else {
            MemoryDeallocate(file);
        }
    }
    return err;
}

long _FT_FileFlush(FTFile *file);
long _FT_BufferFlush(FTFile *file);

/* FT_FileFlush (FT flush: through the parent file, else the memory or native flush). STATE:
   MATCH 100%, exe 27 / obj 27, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004dac8a
long FT_FileFlush(void *ref)
{
    FTFile *file;
    long err, e, result;

    file = (FTFile *)ref;
    err = 0;
    if (file->parent != 0) {
        result = FT_FileFlush(file->parent);
        if (result != 0)
            err = result;
    } else if (file->memHandle != 0) {
        e = _FT_BufferFlush(file);
        if (e != 0)
            err = e;
    } else {
        e = _FT_FileFlush(file);
        if (e != 0)
            err = e;
    }
    return err;
}

/* FT_FileRead (FT read; kFTErrPosition past the end). STATE: MATCH 100%, exe 23 / obj 23,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004daccc
long FT_FileRead(void *ref, ULong count, void *buffer)
{
    FTFile *file;
    long err, e;

    file = (FTFile *)ref;
    err = 0;
    if (count > file->size - file->position)
        err = kFTErrPosition;
    else {
        e = file->procs.proc[kFTRead](file, count, buffer);
        if (e != 0)
            err = e;
    }
    return err;
}

/* FT_FileWrite (FT write). STATE: MATCH 100%, exe 17 / obj 17, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dad00
long FT_FileWrite(FTFile *file, ULong count, const void *buffer)
{
    long err = 0, e = -1;

    e = file->procs.proc[kFTWrite](file, count, buffer);
    if (e != 0)
        err = e;
    return err;
}

/* FT_FileGetEOF (FT through the size procedure). STATE: MATCH 100%, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004dad25
long FT_FileGetEOF(FTFile *file, ULong *size)
{
    long err, e;

    err = 0;
    e = file->procs.proc[kFTSize](file, size);
    if (e != 0)
        err = e;
    return err;
}

/* FT_FileSetEOF (FT through the set-size procedure). STATE: MATCH 100%, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dad42
long FT_FileSetEOF(FTFile *file, ULong size)
{
    long err, e;

    err = 0;
    e = file->procs.proc[kFTSetSize](file, size);
    if (e != 0)
        err = e;
    return err;
}

/* FT_FileSetMark (FT seek; kFTErrPosition when already past the end). STATE: MATCH 100%,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dad5f
long FT_FileSetMark(FTFile *file, ULong position)
{
    long err, e;

    err = 0;
    if (file->position > file->size)
        err = kFTErrPosition;
    else {
        e = file->procs.proc[kFTSeek](file, position);
        if (e != 0)
            err = e;
    }
    return err;
}

/* FT_FileLoad (FT load a whole file into a new handle, refusing one over maxSize). STATE:
   MATCH 100%, exe 84 / obj 84, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004dad8b
long FT_FileLoad(long where, const char *name, ULong maxSize, long flags, void ***handle)
{
    long err, e;
    FTFile *file;
    ULong size;
    void **(*alloc)(ULong size);

    err = 0;
    file = 0;
    *handle = 0;
    e = FT_FileOpen(where, name, 0, &file);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FT_FileGetEOF(file, &size);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (size > maxSize) {
        err = kOutOfMemory;
        goto done;
    }
    if ((unsigned char)flags != 0)
        alloc = IndirectAllocateTempMem;
    else
        alloc = IndirectAllocate;
    if ((*handle = alloc(size)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    IndirectSetLock(*handle, 1);
    e = FT_FileRead(file, size, **handle);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectSetLock(*handle, 0);
done:
    if (file != 0)
        FT_FileClose(file);
    if (err != 0 && *handle != 0) {
        IndirectDeallocate(*handle);
        *handle = 0;
    }
    return err;
}

/* _FT_FileSaveAppend (FT save a handle as a file: replace or append; a new file takes its type
   and creator from the arguments, else from the name's extension; a handle holding a file
   of files is saved as one). STATE: MATCH 100%, exe 219 / obj 219, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004dae5f
long _FT_FileSaveAppend(long where, const char *name, unsigned char replace, unsigned char append,
                  ULong type, ULong creator, void **handle)
{
    long err, e;
    unsigned char locked;
    unsigned char create;
    unsigned char created;
    FTFile *file;
    ULong size;
    FileTypeMap map;
    FTFileInfo info;

    err = 0;
    created = 0;
    create = 0;
    locked = 0;
    file = 0;
    size = 0;
    err = FT_FileGetFlags(where, name, 0, 0);
    if (err != 0 && err != kFTErrFileNotFound)
        goto done;
    if (err == 0) {
        if (!replace && !append) {
            err = kFTErrExists;
            goto done;
        }
        e = FT_FileGetFlags(where, name, &info, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (info.flags[kFTInfoFolder]) {
            e = FT_FileDelete(where, name);
            if (e != 0) {
                err = e;
                goto done;
            }
            create = 1;
        }
    } else {
        create = 1;
    }
    if (create) {
        memset(&info, 0, sizeof(info));
        ATStrCpy(info.name, name);
        info.flags[kFTInfoReadable] = 1;
        info.flags[kFTInfoWritable] = 1;
        info.flags[kFTInfoFolder] = 0;
        info.flags[kFTInfoHidden] = 0;
        info.flags[kFTInfoArchive] = 0;
        if (_FT_IndirectIsChunky(handle)) {
            info.flags[kFTInfoFolder] = info.flags[kFTInfoArchive] = 1;
            if (creator == 0)
                creator = kArchiveType;
            if (type == 0)
                type = kArchiveType;
        }
        if (type == 0 || creator == 0)
            _FT_FileTypesLookup(info.name, type, &map);
        info.type = type != 0 ? type : map.type;
        info.creator = creator != 0 ? creator : map.creator;
        e = FT_FileCreate(where, &info);
        if (e != 0) {
            err = e;
            goto done;
        }
        created = 1;
    }
    e = FT_FileOpen(where, name, 1, &file);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (append) {
        e = FT_FileGetEOF(file, &size);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = FT_FileSetMark(file, size);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    size += IndirectGetSize(handle);
    e = FT_FileSetEOF(file, size);
    if (e != 0) {
        err = e;
        goto done;
    }
    locked = IndirectSetLock(handle, 1);
    err = FT_FileWrite(file, IndirectGetSize(handle), *handle);
    IndirectSetLock(handle, locked);
    e = err;
    if (0 != e)
        err = e;
done:
    if (file != 0) {
        if (err != 0)
            FT_FileSetEOF(file, 0);
        FT_FileFlush(file);
        FT_FileClose(file);
    }
    if (err != 0 && created)
        FT_FileDelete(where, name);
    return err;
}

/* FT_FileSave (FT save, replacing). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db160
long FT_FileSave(long where, const char *name, long flags, ULong type, ULong creator,
                  void **handle)
{
    return _FT_FileSaveAppend(where, name, (unsigned char)flags, 0, type, creator, handle);
}

/* FT_GetResFile (the application's module handle). STATE: MATCH 100%, exe 3 / obj 3, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db183
void *FT_GetResFile(void)
{
    return GetModuleHandleA(0);
}

/* FT_ResourceLoad (FT load a Windows resource into a new handle, refusing one over maxSize;
   the Mac type and the sixth argument are not read). STATE: MATCH 100%, exe 68 / obj 68,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db18c
long FT_ResourceLoad(void *module, unsigned short id, const char *resType, ULong macType,
                  ULong maxSize, long unused, void ***handle)
{
    long err;
    void *res;
    void *data;
    void *bytes;
    ULong size;

    err = 0;
    res = 0;
    data = 0;
    bytes = 0;
    size = 0;
    *handle = 0;
    res = FindResourceA(module, (const char *)(ULong)id, resType);
    if (res == 0) {
        err = GetLastError();
        err = kFTErrFileNotFound;
        goto done;
    }
    size = SizeofResource(module, res);
    if (size == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (size > maxSize) {
        err = kOutOfMemory;
        goto done;
    }
    data = LoadResource(module, res);
    if (data == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    bytes = LockResource(data);
    if (bytes == 0) {
        err = GetLastError();
        err = kFTErrBadPath;
        goto done;
    }
    if ((*handle = IndirectInitialize(bytes, size)) == 0)
        err = kOutOfMemory;
done:
    return err;
}

long _FT_BufferOpenWrapper(void **handle, unsigned char mode, FTFile **result);
long FT_VirtualFileOpen(void **handle, unsigned char mode, FTFile **result);

#define kFolderSpecSize     0x115                /* up to the first byte of names */

/* FT_VirtualPathMount (mount a memory handle as a volume: a spec whose `file` is the handle opened
   as a file of files, an empty handle first made an empty one). STATE: MATCH 100%, exe 76 /
   obj 76, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004db23e
long FT_VirtualPathMount(void **handle, unsigned char mode, FolderSpec ***result)
{
    long err, e;
    FTFile *file;
    FolderSpec **spec, **s;

    err = 0;
    spec = 0;
    file = 0;
    s = 0;
    IndirectGetSize(handle);
    *result = 0;
    e = FT_VirtualFileOpen(handle, mode, &file);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (IndirectGetSize(handle) == 0) {
        e = _FT_ChunkFormat(file, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    spec = (FolderSpec **)IndirectAllocate(kFolderSpecSize);
    if (spec == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(*spec, 0, kFolderSpecSize);
    (*spec)->file = file;
    _FT_PathInsert(spec);
    *result = spec;
    spec = 0;
done:
    if (err != 0 && file != 0)
        FT_FileClose(file);
    return err;
}

/* FT_VirtualFileOpen (FT open a memory handle as a file). STATE: MATCH 100%, exe 16 / obj 16, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004db2f5
long FT_VirtualFileOpen(void **handle, unsigned char mode, FTFile **result)
{
    long err, e;

    err = 0;
    *result = 0;
    e = _FT_BufferOpenWrapper(handle, mode, result);
    if (e != 0)
        err = e;
    return err;
}
