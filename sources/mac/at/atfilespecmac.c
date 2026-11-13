/* atfilespecmac.c -- 1.0.10: the file tool: FT_Open, FT_Close, its public calls. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#define FT_FileSave FT_FileSaveLong
#define FT_FileGetFlags FT_FileGetFlagsLong
#include "burger.h"
#undef FT_FileSave
#undef FT_FileGetFlags
#include "ambrosia.h"
#include "burgerplat.h"
#include "atfiletoolmac.h"
#include "atfilestr.h"

#define kArchiveType        'CHNK'

extern void **_FT_FileTypesCreate(void);                /* atfiletypes.c */

FileProc gFT_FileHooks[6];                       /* 0x0086e868 */
FileProc gFT_ChunkHooks[6];                      /* 0x0086fa98 */
FileProc gFT_BufferHooks[6];                     /* 0x0086dc70 */
FileTypeMap **gFT_FileTypes;                     /* 0x00870930 */
FTLink gFT_FileList;                             /* 0x00870138 open files */
FolderSpec **gFT_PathList;                       /* 0x008701b4 registered specs */
FolderSpec **gFT_Path;                       /* 0x008700f8 */
unsigned char gFT_Open;                      /* 0x00870e08 */
static unsigned char sStirByte;
unsigned char *gStirPointer = &sStirByte;
#define kContextSize        0x59                 /* up to the first byte of names */

int32_t FT_Open(void)
{
    int32_t err, e;

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

    if ((gFT_Path = (FolderSpec **)IndirectAllocate(kContextSize)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(*gFT_Path, 0, kContextSize);

    e = _FT_PathGoTo(gFT_Path, -2);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((gFT_FileTypes = (FileTypeMap **)_FT_FileTypesCreate()) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if ((gStirPointer = MemoryAllocate(4)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    gStirPointer = gStirPointer + kStirPointerBackoff;
    *gStirPointer = sStirByte;
    sStirByte = 0;
    gFT_Open = 1;

done:
    if (err != 0)
        FT_Close();
    return err;
}

void FT_Close(void)
{
    int32_t err;
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

    while ((file = gFT_FileList.next) != 0)
        FT_FileClose(file);

    if (gStirPointer != 0 && gStirPointer != &sStirByte) {
        sStirByte = *gStirPointer;
        gStirPointer -= kStirPointerBackoff;
        MemoryDeallocate(gStirPointer);
        gStirPointer = &sStirByte;
    }

    if (gIndirectBusy == _FT_IsIndirectBusy)
        gIndirectBusy = 0;
    gFT_Open = 0;
}

int32_t FT_FileCreate(int32_t where, FTFileInfo *info);
int32_t FT_FileOpen(int32_t where, const char *name, unsigned char write, FTFile **result);
int32_t FT_FileSetFlags(int32_t where, const char *name, FTFileInfo *info);
int32_t FT_FileFlush(void *ref);
int32_t FT_FileRead(void *ref, ULong count, void *buffer);
int32_t FT_FileWrite(FTFile *file, ULong count, const void *buffer);
int32_t FT_FileGetEOF(FTFile *file, ULong *size);
int32_t FT_FileSetEOF(FTFile *file, ULong size);
int32_t FT_FileSetMark(FTFile *file, ULong position);
int32_t _FT_FileSaveAppend(int32_t where, const char *name, unsigned char replace, unsigned char append,
                  ULong type, ULong creator, void **handle);

int32_t FT_VirtualFileOpen(void **handle, unsigned char mode, FTFile **result);

int32_t FT_PathCreate(FolderSpec **where, const char *name, FolderSpec ***result)
{
    int32_t err;
    int32_t e;
    FolderSpec **spec;

    err = 0;
    spec = 0;
    *result = 0;
    if ((int32_t)where >= kFolderShared && (int32_t)where <= kFolderApplication &&
        (int32_t)name != -1 && (name == 0 || *name == 0)) {
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

int32_t FT_PathCurrent(const char *first, const char *second)
{
    int32_t err;
    int32_t e;

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

int32_t FT_PathDispose(FolderSpec **spec)
{
    if (IsSpecID(spec) || _FT_PathLookup(spec))
        _FT_PathDispose(spec);
}

int32_t FT_FileCreate(int32_t where, FTFileInfo *info)
{
    int32_t err, e;
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
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0 || (*spec)->names[0] != 0) {
        e = _FT_PathModify(spec, info, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
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
    if (info->flags[kFTInfoArchive]) {
        if (info->flags[kFTInfoFolder] && info->size != 0 && info->size <= kMaxEntries)
            entries = info->size;
        copy = *info;
        copy.flags[kFTInfoArchive] = 0;
        copy.flags[kFTInfoFolder] = 0;
        copy.size = 0;
        copy.type = kArchiveType;
        copy.creator = kArchiveType;
        e = _FT_FileCreate(spec, &copy);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
            e = _FT_ChunkOpenPath(spec, 1, 0, 0, &archive, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = _FT_ChunkFormat(archive, entries);
            if (e != 0) {
                err = e;
                goto done;
            }
            goto done;
        }
        e = _FT_FileOpenWrapper(spec, 1, &file);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _FT_ChunkFormat(file, entries);
        if (e != 0) {
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

int32_t FT_FileDelete(int32_t where, const char *name)
{
    int32_t err, e, e2;
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
        e = _FT_ChunkOpenPath(spec, 1, 1, 0, &archive, leaf);
        if (e != 0) {
            err = e;
            goto done;
        }
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

int32_t FT_FileGetFlags(int32_t where, const char *name, FTFileInfo *info, unsigned char flags)
{
    int32_t err, e;
    int isFile;
    unsigned char probe;
    char leaf[256];
    FTFile *file;
    FolderSpec **spec;

    err = 0;
    file = 0;
    isFile = 0;
    spec = 0;
    if (info != 0)
        memset(info, 0, sizeof(FTFileInfo));
    probe = AsByte(info == 0);
    err = _FT_PathModify(gFT_Path, (void *)where, &spec, probe);
    if (err != 0) {
        if (probe != 0)
            err = FT_ReportError(err);
        goto done;
    }
    err = _FT_PathModify(spec, (void *)name, 0, probe);
    if (err != 0) {
        if (probe != 0)
            err = FT_ReportError(err);
        goto done;
    }
    if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
        e = _FT_ChunkOpenPath(spec, 0, 1, 0, &file, leaf);
        if (e != 0) {
            err = e;
            goto done;
        }
        err = _FT_ChunkGetFlags(file, leaf, 0, info, spec);
        goto done;
    } else if ((*spec)->file != 0) {
        if (info == 0)
            goto done;
        FTStrCpy(info->name, "_Virtual_");
        info->flags[kFTInfoReadable] = 1;
        info->flags[kFTInfoWritable] = (*spec)->file->mode;
        info->flags[kFTInfoFolder] = 1;
        info->flags[kFTInfoHidden] = 0;
        info->flags[kFTInfoArchive] = 1;
        info->size = 0;
        info->created = 0;
        info->modified = 0;
        e = _FT_ChunkOpenPath(spec, 0, 0, 0, &file, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _FT_ChunkGetCount(file, 0, &info->size);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else {
        err = _FT_FileGetFlags(spec, info, flags);
    }
done:
    if (file != 0)
        FT_FileClose(file);
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

int32_t FT_FileSetFlags(int32_t where, const char *name, FTFileInfo *info)
{
    int32_t err, e;
    FTFile *file;
    FolderSpec **spec;
    char leaf[256];

    err = 0;
    file = 0;
    spec = 0;
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
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0 && (*spec)->names[0] == 0) {
        err = kFTErrNotAFile;
        goto done;
    }
    if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
        e = _FT_ChunkOpenPath(spec, 1, 1, 0, &file, leaf);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = _FT_ChunkSetFlags(file, leaf, info);
        if (e != 0)
            err = e;
    } else {
        e = _FT_FileSetFlags(spec, info);
        if (e != 0)
            err = e;
    }
done:
    if (file != 0)
        FT_FileClose(file);
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}

#if TARGET_API_MAC_CARBON
int32_t FT_DirGetEntry(int32_t where, ULong index, FTFileInfo *info, unsigned char flags)
{
    int32_t err, e;
    FTFile *file;
    FolderSpec **spec;

    file = 0;
    spec = 0;
    e = _FT_PathModify(gFT_Path, (void *)where, &spec, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (((*spec)->file != 0 ? 1 : 0) != 0 || (*spec)->names[0] != 0 || _FT_IsPathAFile(spec)) {
        e = _FT_ChunkOpenPath(spec, 0, 0, 0, &file, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        err = _FT_ChunkDirEntry(file, index, info, spec);
    } else {
        err = _FT_FileDirEntry(spec, index, info, 0, flags);
    }
done:
    if (file != 0)
        FT_FileClose(file);
    if (spec != 0)
        _FT_PathDispose(spec);
    return err;
}
#endif

int32_t FT_FileOpen(int32_t where, const char *name, unsigned char write, FTFile **result)
{
    int32_t err, e;
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

int32_t FT_FileClose(void *ref)
{
    FTFile *file;
    FTLink *link;
    int32_t err;
    int32_t result;

    file = ref;
    err = 0;
    if (_FT_FileUpdateRefCount(file, -1) == 0) {
        link = &gFT_FileList;
        while (link->next != 0 && link->next != file)
            link = &link->next->link;
        *link = file->link;
        file->link.next = 0;

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
            file->link = gFT_FileList;
            gFT_FileList.next = file;
        } else {
            MemoryDeallocate(file);
        }
    }
    return err;
}

int32_t _FT_FileFlush(FTFile *file);
int32_t _FT_BufferFlush(FTFile *file);

int32_t FT_FileFlush(void *ref)
{
    FTFile *file;
    int32_t err, e, result;

    file = ref;
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

int32_t FT_FileRead(void *ref, ULong count, void *buffer)
{
    FTFile *file;
    int32_t err, e;

    file = ref;
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

int32_t FT_FileWrite(FTFile *file, ULong count, const void *buffer)
{
    int32_t err = 0, e = -1;

    e = file->procs.proc[kFTWrite](file, count, buffer);
    if (e != 0)
        err = e;
    return err;
}

int32_t FT_FileGetEOF(FTFile *file, ULong *size)
{
    int32_t err, e;

    err = 0;
    e = file->procs.proc[kFTSize](file, size);
    if (e != 0)
        err = e;
    return err;
}

int32_t FT_FileSetEOF(FTFile *file, ULong size)
{
    int32_t err, e;

    err = 0;
    e = file->procs.proc[kFTSetSize](file, size);
    if (e != 0)
        err = e;
    return err;
}

int32_t FT_FileSetMark(FTFile *file, ULong position)
{
    int32_t err, e;

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

int32_t FT_FileLoad(int32_t where, const char *name, ULong maxSize, int32_t flags, void ***handle)
{
    int32_t err, e;
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

int32_t _FT_FileSaveAppend(int32_t where, const char *name, unsigned char replace, unsigned char append,
                  ULong type, ULong creator, void **handle)
{
    int32_t err, e;
    unsigned char create;
    unsigned char locked;
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
        FTStrCpy(info.name, name);
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

int32_t FT_FileSave(int32_t where, const char *name, unsigned char flags, ULong type, ULong creator,
                  void **handle)
{
    return _FT_FileSaveAppend(where, name, flags, 0, type, creator, handle);
}

int32_t _FT_BufferOpenWrapper(void **handle, unsigned char mode, FTFile **result);
int32_t FT_VirtualFileOpen(void **handle, unsigned char mode, FTFile **result);

#define kFolderSpecSize     0x59

int32_t FT_VirtualPathMount(void **handle, unsigned char mode, FolderSpec ***result)
{
    int32_t err, e;
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

int32_t FT_VirtualFileOpen(void **handle, unsigned char mode, FTFile **result)
{
    int32_t err, e;

    err = 0;
    *result = 0;
    e = _FT_BufferOpenWrapper(handle, mode, result);
    if (e != 0)
        err = e;
    return err;
}
