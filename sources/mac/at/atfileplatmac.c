/* atfileplatmac.c -- 1.0.10: the file tool on the File Manager, where win's atfilespec3.c has Win32. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#define _FT_IsPathAFile _FT_IsPathAFileByte
#include "atfiletoolmac.h"
#undef _FT_IsPathAFile

#define kSecondsTo1970      2082844800
#define kFileTooFewBytes    kFTErrBadPath
#define kClassicDomain      -32762
extern unsigned char _PlatformIsThreaded(void);
#define kFTErrChunkType     -7205
#define kFTErrResType       -7215

static unsigned char kColon[] = "\p:";

int32_t _FT_PathGoTo(FolderSpec **spec, int32_t folder)
{
    int32_t err;
    int32_t e;
    int32_t e2;
    short vRefNum;
    int32_t dirID;
    ProcessSerialNumber process;
    ProcessInfoRec info;
    FSSpec appSpec;
    Str255 name;

    err = 0;
    vRefNum = 0;
    dirID = 0;
    IndirectGetSize((void **)spec);
    switch (folder) {
    case kFolderDocuments:
        e = FindFolder(kOnSystemDisk, kDocumentsFolderType, kDontCreateFolder, &vRefNum, &dirID);
        if (e != 0) {
            e = FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder, &vRefNum, &dirID);
            if (e != 0) {
                err = _FT_RemapError(e);
                break;
            }
            dirID = fsRtDirID;
        }
        e2 = _FT_PathGoToID(spec, vRefNum, dirID);
        if (e2 != 0)
            err = e2;
        break;
    case kFolderTemporary:
        e = FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &vRefNum, &dirID);
        if (e != 0) {
            err = _FT_RemapError(e);
            break;
        }
        e2 = _FT_PathGoToID(spec, vRefNum, dirID);
        if (e2 != 0)
            err = e2;
        break;
#if TARGET_API_MAC_CARBON
    case kFolderAppData:
        if (_PlatformIsThreaded()) {
            e = FindFolder(kClassicDomain, kPreferencesFolderType, kCreateFolder, &vRefNum, &dirID);
            if (e != 0) {
                err = _FT_RemapError(e);
                break;
            }
            e2 = _FT_PathGoToID(spec, vRefNum, dirID);
            if (e2 != 0)
                err = e2;
            break;
        }
    case kFolderShared:
        if (_PlatformIsThreaded()) {
            e = FindFolder(kLocalDomain, kPreferencesFolderType, kCreateFolder, &vRefNum, &dirID);
            if (e != 0) {
                err = _FT_RemapError(e);
                break;
            }
            e2 = _FT_PathGoToID(spec, vRefNum, dirID);
            if (e2 != 0)
                err = e2;
            break;
        }
    case kFolderUserData:
#else
    case kFolderShared:
    case kFolderAppData:
    case kFolderUserData:
#endif
        e = FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &vRefNum, &dirID);
        if (e != 0) {
            err = _FT_RemapError(e);
            break;
        }
        e2 = _FT_PathGoToID(spec, vRefNum, dirID);
        if (e2 != 0)
            err = e2;
        break;
    case kFolderApplication:
        process.highLongOfPSN = 0;
        process.lowLongOfPSN = kCurrentProcess;
        info.processInfoLength = sizeof(ProcessInfoRec);
        info.processName = name;
        info.processAppSpec = &appSpec;
        e = GetProcessInformation(&process, &info);
        if (e != 0) {
            err = kFTErrBadPath;
            break;
        }
        e2 = _FT_PathGoToID(spec, appSpec.vRefNum, appSpec.parID);
        if (e2 != 0)
            err = e2;
        break;
    }
    return err;
}

int32_t _FT_PathGoToVolume(FolderSpec **spec, ULong index)
{
    int32_t err, e;
    int32_t oe;
    HParamBlockRec pb;
    Str255 name;

    err = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(HParamBlockRec));
    pb.volumeParam.ioVolIndex = index;
    pb.volumeParam.ioNamePtr = name;
    pb.volumeParam.ioVRefNum = 0;
    if (PBHGetVInfoSync(&pb) == 0) {
        e = _FT_PathGoToID(spec, pb.volumeParam.ioVRefNum, fsRtDirID);
        if (e != 0)
            err = e;
    } else {
        pb.volumeParam.ioVolIndex = index - 1;
        pb.volumeParam.ioNamePtr = name;
        pb.volumeParam.ioVRefNum = 0;
        oe = PBHGetVInfoSync(&pb);
        if (oe != 0)
            err = _FT_RemapError(oe);
        else
            err = kFTErrFileNotFound;
    }
    return err;
}

static inline void FTPStrCat(unsigned char *dst, const unsigned char *src)
{
    int32_t m;
    int32_t n;

    n = src[0];
    if (n + dst[0] <= 255)
        m = n;
    else {
        m = 255 - dst[0];
        n = m;
    }
    BlockMoveData(src + 1, dst + dst[0] + 1, n);
    dst[0] += m;
}

int32_t _FT_PathGoToVolumeName(FolderSpec **spec, const unsigned char *volume)
{
    int32_t n;
    int32_t err, e;
    int32_t oe;
    HParamBlockRec pb;
    Str255 name;

    err = 0;
    IndirectGetSize((void **)spec);
    BlockMoveData(volume, name, volume[0] + 1);
    FTPStrCat(name, kColon);
    MemoryClear(&pb, sizeof(HParamBlockRec));
    pb.volumeParam.ioVolIndex = -1;
    pb.volumeParam.ioNamePtr = name;
    pb.volumeParam.ioVRefNum = 0;
    oe = PBHGetVInfoSync(&pb);
    if (oe != 0) {
        err = _FT_RemapError(oe);
    } else {
        e = _FT_PathGoToID(spec, pb.volumeParam.ioVRefNum, fsRtDirID);
        if (e != 0)
            err = e;
    }
    return err;
}

int32_t _FT_PathGoToParent(FolderSpec **spec, unsigned char *leaf)
{
    int32_t err;
    int32_t e;
    CInfoPBRec pb;
    Str255 name;

    err = 0;
    IndirectGetSize((void **)spec);
    IndirectGetSize((void **)spec);
    if (leaf != 0)
        BlockMoveData((*spec)->spec.name, leaf, (*spec)->spec.name[0] + 1);
    if ((*spec)->spec.parID == fsRtParID) {
        err = kFTErrDirNotFound;
        goto done;
    }
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = -1;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    (*spec)->spec.vRefNum = pb.dirInfo.ioVRefNum;
    (*spec)->spec.parID = pb.dirInfo.ioDrParID;
    BlockMoveData(name, (*spec)->spec.name, name[0] + 1);
done:
    if (err != 0 && leaf != 0)
        *leaf = 0;
    return err;
}

int32_t _FT_PathGoToChild(FolderSpec **spec, const unsigned char *child, unsigned char last)
{
    int32_t err, e2;
    int32_t e;
    CInfoPBRec pb;
    Str255 name;

    err = 0;
    IndirectGetSize((void **)spec);
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    if ((pb.hFileInfo.ioFlAttrib & ioDirMask) != 0) {
        pb.dirInfo.ioNamePtr = (StringPtr)child;
        pb.dirInfo.ioFDirIndex = 0;
        err = PBGetCatInfoSync(&pb);
        if (err != 0) {
            err = _FT_RemapError(err);
            goto done;
        }
        (*spec)->spec.vRefNum = pb.dirInfo.ioVRefNum;
        (*spec)->spec.parID = pb.dirInfo.ioDrParID;
        BlockMoveData(child, (*spec)->spec.name, child[0] + 1);
    } else {
        e2 = _FT_PathAppend(spec, child);
        if (e2 != 0)
            err = e2;
    }
done:
    return err;
}

int32_t _FT_IsPathAFile(FolderSpec **spec)
{
    int32_t isFile;
    int32_t e;
    CInfoPBRec pb;
    Str255 name;

    isFile = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    e = PBGetCatInfoSync(&pb);
    if (e != 0)
        _FT_RemapError(e);
    else
        isFile = ((pb.hFileInfo.ioFlAttrib >> 4) & 1) ^ 1;
    return isFile;
}

int32_t _FT_PathValidate(FolderSpec **spec)
{
    int32_t err;
    int32_t diff;
    short index;
    HParamBlockRec pb;
    Str255 name;

    err = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(HParamBlockRec));
    if ((*spec)->spec.vRefNum != 0) {
        pb.volumeParam.ioVRefNum = (*spec)->spec.vRefNum;
        pb.volumeParam.ioNamePtr = name;
        pb.volumeParam.ioVolIndex = 0;
        if (PBHGetVInfoSync(&pb) != 0) {
            (*spec)->spec.vRefNum = 0;
        } else {
            diff = (*spec)->volumeDate - pb.volumeParam.ioVCrDate;
            if (diff < 0)
                diff = -diff;
            if (diff > kSecondsPerDay || (diff % 15) * 60 != 0)
                (*spec)->spec.vRefNum = 0;
        }
    }
    for (index = 1; (*spec)->spec.vRefNum == 0; index++) {
        pb.volumeParam.ioVRefNum = 0;
        pb.volumeParam.ioNamePtr = name;
        pb.volumeParam.ioVolIndex = index;
        if (PBHGetVInfoSync(&pb) != 0) {
            err = kFTErrNoVolume;
            break;
        }
        diff = (*spec)->volumeDate - pb.volumeParam.ioVCrDate;
        if (diff < 0)
            diff = -diff;
        if (diff < kSecondsPerDay && (diff % 15) * 60 == 0)
            (*spec)->spec.vRefNum = pb.volumeParam.ioVRefNum;
    }
    return err;
}

int32_t _FT_PathGoToID(FolderSpec **spec, short vRefNum, int32_t dirID)
{
    int32_t err, e2;
    int32_t e;
    Str255 name;
    CInfoPBRec pb;
    HParamBlockRec vol;

    err = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&vol, sizeof(HParamBlockRec));
    vol.volumeParam.ioVRefNum = vRefNum;
    vol.volumeParam.ioNamePtr = name;
    vol.volumeParam.ioVolIndex = 0;
    e = PBHGetVInfoSync(&vol);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = vRefNum;
    pb.dirInfo.ioDrDirID = dirID;
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = -1;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    e2 = _FT_PathClearNames(spec);
    if (e2 != 0) {
        err = e2;
        goto done;
    }
    (*spec)->volumeDate = vol.volumeParam.ioVCrDate;
    (*spec)->spec.vRefNum = pb.dirInfo.ioVRefNum;
    (*spec)->spec.parID = pb.dirInfo.ioDrParID;
    BlockMoveData(name, (*spec)->spec.name, name[0] + 1);
done:
    return err;
}

int32_t _FT_FileCreate(FolderSpec **spec, FTFileInfo *info)
{
    int32_t err, e2;
    int32_t e;
    unsigned char created;
    short refNum;
    int32_t count;
    FSSpec fs;
    CInfoPBRec pb;
    Str255 name;

    err = 0;
    created = 0;
    refNum = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    fs.vRefNum = pb.dirInfo.ioVRefNum;
    fs.parID = pb.dirInfo.ioDrDirID;
    e2 = _FT_PathNameInput(fs.name, info->name, 255);
    if (e2 != 0) {
        err = e2;
        goto done;
    }
    if (info->flags[kFTInfoFolder]) {
        e = FSpDirCreate(&fs, smSystemScript, &count);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        (*spec)->spec = fs;
        created = 1;
    } else {
        e = FSpCreate(&fs, info->creator, info->type, smSystemScript);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        (*spec)->spec = fs;
        created = 1;
    }
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    if (info->flags[kFTInfoWritable])
        pb.hFileInfo.ioFlAttrib &= ~1;
    else
        pb.hFileInfo.ioFlAttrib |= 1;
    if (info->flags[kFTInfoHidden])
        pb.hFileInfo.ioFlFndrInfo.fdFlags |= kIsInvisible;
    else
        pb.hFileInfo.ioFlFndrInfo.fdFlags &= ~kIsInvisible;
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBSetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    if (!info->flags[kFTInfoFolder] && info->size != 0) {
        fs = (*spec)->spec;
        e = FSpOpenDF(&fs, fsRdWrPerm, &refNum);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        count = info->size;
        AllocContig(refNum, &count);
        e = SetEOF(refNum, info->size);
        if (e != 0)
            err = _FT_RemapError(e);
    }
done:
    if (refNum != 0) {
        e2 = _FT_RemapError(FSClose(refNum));
        if (e2 != 0)
            err = e2;
    }
    if (err != 0 && created)
        _FT_FileDelete(spec);
    return err;
}

int32_t _FT_FileDelete(FolderSpec **spec)
{
    int32_t err;
    int32_t e;
    FSSpec fs;

    err = 0;
    IndirectGetSize((void **)spec);
    fs = (*spec)->spec;
    e = FSpDelete(&fs);
    if (e != 0)
        err = _FT_RemapError(e);
    return err;
}

int32_t _FT_FileGetDates(FolderSpec **spec, ULong *created, ULong *modified)
{
    int32_t err;
    int32_t e;
    CInfoPBRec pb;
    Str255 name;

    err = 0;
    *modified = 0;
    *created = 0;
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
    } else {
        *created = pb.hFileInfo.ioFlCrDat - kSecondsTo1970;
        *modified = pb.hFileInfo.ioFlMdDat - kSecondsTo1970;
    }
    return err;
}

int32_t _FT_FileGetFlags(FolderSpec **spec, FTFileInfo *info, unsigned char checkArchive)
{
    int32_t err;
    int32_t e;
    int32_t access;
    int32_t locked;
    ULong count;
    Str255 name;
    CInfoPBRec pb;
    HParamBlockRec vol;

    IndirectGetSize((void **)spec);
    if (info != 0)
        MemoryClear(info, sizeof(FTFileInfo));
    MemoryClear(&vol, sizeof(HParamBlockRec));
    vol.volumeParam.ioVolIndex = 0;
    vol.volumeParam.ioNamePtr = name;
    vol.volumeParam.ioVRefNum = (*spec)->spec.vRefNum;
    e = PBHGetVInfoSync(&vol);
    if (info == 0 && e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    err = PBGetCatInfoSync(&pb);
    if (info == 0 && err != 0) {
        err = _FT_RemapError(err);
        goto done;
    }
    if (err != 0) {
        err = _FT_RemapError(err);
        goto done;
    }
    if (info == 0)
        goto done;
    _FT_PathNameOutput(info->name, pb.hFileInfo.ioNamePtr);
    info->flags[kFTInfoReadable] = 1;
    access = pb.hFileInfo.ioACUser;
    locked = (vol.volumeParam.ioVAtrb & 0x8000) != 0 || (pb.hFileInfo.ioFlAttrib & 1) != 0 || (access & 4) != 0;
    info->flags[kFTInfoWritable] = AsByte(locked == 0);
    info->flags[kFTInfoFolder] = (pb.hFileInfo.ioFlAttrib >> 4) & 1;
    info->flags[kFTInfoHidden] = (pb.hFileInfo.ioFlFndrInfo.fdFlags >> 14) & 1;
    if (checkArchive && !info->flags[kFTInfoFolder]) {
        if (_FT_FileIsChunky(spec, &count))
            info->flags[kFTInfoArchive] = 1;
    }
    if (info->flags[kFTInfoArchive]) {
        info->flags[kFTInfoFolder] = 1;
        info->size = count;
        info->created = pb.dirInfo.ioDrCrDat - kSecondsTo1970;
        info->modified = pb.dirInfo.ioDrMdDat - kSecondsTo1970;
    } else if (info->flags[kFTInfoFolder]) {
        info->size = pb.dirInfo.ioDrNmFls;
        info->created = pb.dirInfo.ioDrCrDat - kSecondsTo1970;
        info->modified = pb.dirInfo.ioDrMdDat - kSecondsTo1970;
    } else {
        info->size = pb.hFileInfo.ioFlLgLen;
        info->created = pb.dirInfo.ioDrCrDat - kSecondsTo1970;
        info->modified = pb.dirInfo.ioDrMdDat - kSecondsTo1970;
        info->type = pb.hFileInfo.ioFlFndrInfo.fdType;
        info->creator = pb.hFileInfo.ioFlFndrInfo.fdCreator;
    }
done:
    return err;
}

int32_t _FT_FileSetFlags(FolderSpec **spec, FTFileInfo *info)
{
    int32_t err, e2;
    int32_t e;
    FileTypeMap map;
    CInfoPBRec pb;
    HParamBlockRec rename;
    Str255 newName;
    Str255 name;

    err = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    e2 = _FT_PathNameInput(newName, info->name, 255);
    if (e2 != 0) {
        err = e2;
        goto done;
    }
    if (!EqualString(name, newName, 1, 1)) {
        MemoryClear(&rename, sizeof(HParamBlockRec));
        rename.ioParam.ioNamePtr = name;
        rename.ioParam.ioVRefNum = pb.dirInfo.ioVRefNum;
        rename.fileParam.ioDirID = pb.dirInfo.ioDrParID;
        rename.ioParam.ioMisc = (Ptr)newName;
        e = PBHRenameSync(&rename);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        BlockMoveData(newName, (*spec)->spec.name, newName[0] + 1);
    }
    if (info->flags[kFTInfoWritable])
        pb.hFileInfo.ioFlAttrib &= ~1;
    else
        pb.hFileInfo.ioFlAttrib |= 1;
    if (info->flags[kFTInfoHidden])
        pb.hFileInfo.ioFlFndrInfo.fdFlags |= kIsInvisible;
    else
        pb.hFileInfo.ioFlFndrInfo.fdFlags &= ~kIsInvisible;
    if (!info->flags[kFTInfoFolder] && (pb.hFileInfo.ioFlAttrib & ioDirMask) == 0) {
        if (info->type == 0 || info->creator == 0)
            _FT_FileTypesLookup(info->name, 0, &map);
        pb.hFileInfo.ioFlFndrInfo.fdType = info->type != 0 ? info->type : map.type;
        pb.hFileInfo.ioFlFndrInfo.fdCreator = info->creator != 0 ? info->creator : map.creator;
    }
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    pb.dirInfo.ioNamePtr = newName;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBSetCatInfoSync(&pb);
    if (e != 0)
        err = _FT_RemapError(e);
done:
    return err;
}

#if TARGET_API_MAC_CARBON
int32_t _FT_FileDirEntry(FolderSpec **spec, ULong index, FTFileInfo *info, FTDirEntry *out, unsigned char flags)
{
    int32_t err, e;
    FolderSpec **child;
    CInfoPBRec pb;
    Str255 name;

    child = 0;
    IndirectGetSize((void **)spec);
    if (info != 0)
        MemoryClear(info, sizeof(FTFileInfo));
    if (out != 0)
        MemoryClear(out, sizeof(FTDirEntry));
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    if ((e = PBGetCatInfoSync(&pb)) != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = index + 1;
    if ((err = PBGetCatInfoSync(&pb)) != 0) {
        if (err != fnfErr) {
            err = _FT_RemapError(err);
            goto done;
        }
        if (index != 0) {
            pb.dirInfo.ioNamePtr = name;
            pb.dirInfo.ioFDirIndex = index;
            if ((e = PBGetCatInfoSync(&pb)) != 0) {
                err = _FT_RemapError(e);
                goto done;
            }
        }
        err = kFTErrNoMoreEntries;
        goto done;
    }
    if (info != 0) {
        e = _FT_PathModify(spec, 0, &child, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        (*child)->spec.vRefNum = pb.dirInfo.ioVRefNum;
        (*child)->spec.parID = pb.dirInfo.ioDrParID;
        BlockMoveData(name, (*child)->spec.name, name[0] + 1);
        e = _FT_FileGetFlags(child, info, flags);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (out != 0) {
        out->vRefNum = pb.dirInfo.ioVRefNum;
        out->id = pb.dirInfo.ioDrDirID;
    }
done:
    if (child != 0)
        _FT_PathDispose(child);
    return err;
}
#endif

FTFile *_FT_IsFileOpen(FolderSpec **spec)
{
    FTLink *link;
    short vRefNum;
    int32_t dirID;
    FTFile *f;
    int32_t e;
    CInfoPBRec pb;
    Str255 name;

    link = 0;
    IndirectGetSize((void **)spec);
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        _FT_RemapError(e);
        goto done;
    }
    link = &gFT_FileList;
    vRefNum = pb.dirInfo.ioVRefNum;
    dirID = pb.hFileInfo.ioDirID;
    while ((f = link->next) != 0) {
        if (f->vRefNum == vRefNum && f->dirID == dirID)
            break;
        link = &f->link;
    }
done:
    return link != 0 ? link->next : 0;
}

int32_t _FT_FileOpen(FolderSpec **spec, unsigned char write, FTFile *file)
{
    int32_t err;
    int32_t e;
    Str255 name;
    CInfoPBRec pb;
    FSSpec fs;

    err = 0;
    IndirectGetSize((void **)spec);
    fs = (*spec)->spec;
    e = FSpOpenDF(&fs, write ? fsRdWrPerm : fsRdPerm, &file->refNum);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    MemoryClear(&pb, sizeof(CInfoPBRec));
    pb.dirInfo.ioVRefNum = (*spec)->spec.vRefNum;
    pb.dirInfo.ioDrDirID = (*spec)->spec.parID;
    BlockMoveData((*spec)->spec.name, name, (*spec)->spec.name[0] + 1);
    pb.dirInfo.ioNamePtr = name;
    pb.dirInfo.ioFDirIndex = 0;
    e = PBGetCatInfoSync(&pb);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    file->vRefNum = pb.dirInfo.ioVRefNum;
    file->dirID = pb.hFileInfo.ioDirID;
    file->position = 0;
    e = GetEOF(file->refNum, (int32_t *)&file->size);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    file->mode = write;
    file->procs = *(FTProcs *)gFT_FileHooks;
done:
    if (err != 0 && file->refNum != 0)
        _FT_FileClose(file);
    return err;
}

int32_t _FT_FileClose(FTFile *file)
{
    int32_t err;
    int32_t e;

    err = 0;
    if (file->refNum != 0) {
        e = FSClose(file->refNum);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
    }
    file->refNum = 0;
    file->vRefNum = 0;
    file->dirID = 0;
done:
    return err;
}

int32_t _FT_FileFlush(FTFile *file)
{
    int32_t err;
    int32_t e;
    ParamBlockRec pb;

    err = 0;
    MemoryClear(&pb, sizeof(ParamBlockRec));
    pb.ioParam.ioRefNum = file->refNum;
    e = PBFlushFileSync(&pb);
    if (e != 0)
        err = _FT_RemapError(e);
    return err;
}

int32_t _FT_FileRead(FTFile *file, ULong count, void *buffer)
{
    int32_t err;
    int32_t e;
    int32_t n;

    err = 0;
    if (file->position + count > file->size) {
        err = kFTErrPosition;
        goto done;
    }
    n = count;
    e = FSRead(file->refNum, &n, buffer);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    if (n != count) {
        err = kFTErrBadPath;
        goto done;
    }
    file->position += count;
done:
    return err;
}

int32_t _FT_FileWrite(FTFile *file, ULong count, const void *buffer)
{
    int32_t err, e;
    int32_t n;
    FCBPBRec pb;

    err = 0;
    if (file->size < file->position + count) {
        MemoryClear(&pb, sizeof(FCBPBRec));
        pb.ioFCBIndx = 0;
        pb.ioVRefNum = 0;
        pb.ioRefNum = file->refNum;
        e = PBGetFCBInfoSync(&pb);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        if (file->position + count > pb.ioFCBPLen) {
            n = file->position + count - pb.ioFCBPLen;
            err = AllocContig(file->refNum, &n);
            if (err == dskFulErr) {
                n = file->position + count - pb.ioFCBPLen - n;
                err = Allocate(file->refNum, &n);
            }
            if (err != 0) {
                err = _FT_RemapError(err);
                goto done;
            }
        }
    }
    n = count;
    e = FSWrite(file->refNum, &n, buffer);
    if (e != 0) {
        err = _FT_RemapError(e);
        goto done;
    }
    if (n != count) {
        err = kFTErrBadPath;
        goto done;
    }
    if (file->size < file->position + count)
        file->size = file->position + count;
    file->position += count;
done:
    return err;
}

int32_t _FT_FileGetEOF(FTFile *file, ULong *size)
{
    *size = file->size;
    return 0;
}

int32_t _FT_FileSetEOF(FTFile *file, ULong size)
{
    int32_t err, e;
    int32_t n;
    FCBPBRec pb;

    err = 0;
    if (size > file->size) {
        MemoryClear(&pb, sizeof(FCBPBRec));
        pb.ioFCBIndx = 0;
        pb.ioVRefNum = 0;
        pb.ioRefNum = file->refNum;
        e = PBGetFCBInfoSync(&pb);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        if (size > pb.ioFCBPLen) {
            n = size - pb.ioFCBPLen;
            err = AllocContig(file->refNum, &n);
            if (err == dskFulErr) {
                e = PBGetFCBInfoSync(&pb);
                if (e != 0) {
                    err = _FT_RemapError(e);
                    goto done;
                }
                n = size - pb.ioFCBPLen;
                err = Allocate(file->refNum, &n);
            }
            if (err != 0) {
                err = _FT_RemapError(err);
                goto done;
            }
        }
        e = SetEOF(file->refNum, size);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        file->size = size;
    } else if (size < file->size) {
        e = SetEOF(file->refNum, size);
        if (e != 0) {
            err = _FT_RemapError(e);
            goto done;
        }
        file->size = size;
        if (file->position > size) {
            e = SetFPos(file->refNum, fsFromLEOF, 0);
            if (e != 0) {
                err = _FT_RemapError(e);
                goto done;
            }
            file->position = size;
        }
    }
done:
    return err;
}

int32_t _FT_FileGetMark(FTFile *file, ULong *position)
{
    *position = file->position;
    return 0;
}

int32_t _FT_FileSetMark(FTFile *file, ULong position)
{
    int32_t err;
    int32_t e;

    err = 0;
    e = SetFPos(file->refNum, fsFromStart, position);
    if (e != 0)
        err = _FT_RemapError(e);
    else
        file->position = position;
    return err;
}

int32_t _FT_RemapError(int32_t err)
{
    switch (err) {
    case noErr:
        return 0;
    case userCanceledErr:
        return kFTErrCancelled;
    case fsRnErr:
    case bdNamErr:
        return kFTErrBadName;
    case nsvErr:
    case fnfErr:
    case nsDrvErr:
    case dirNFErr:
    case fidNotFound:
    case -1307:
        return kFTErrFileNotFound;
    case dupFNErr:
    case fidExists:
        return kFTErrExists;
    case notAFileErr:
        return kFTErrChunkType;
    case fBsyErr:
    case opWrErr:
        return kFTErrBusy;
    case wPrErr:
    case fLckdErr:
    case vLckdErr:
    case permErr:
    case wrPermErr:
        return kFTErrPermission;
    case eofErr:
    case posErr:
    case gfpErr:
    case -1309:
        return kFTErrPosition;
    case dirFulErr:
        return kFTErrDiskFull;
    case volOffLinErr:
    case volGoneErr:
        return kFTErrNoVolume;
    case tmfoErr:
    case tmwdoErr:
        return kFTErrTooManyFiles;
    case mFulErr:
        return kOutOfMemory;
    case -199: case -198: case -197: case -196: case -195: case -194:
    case -190: case -189: case -188:
        return kFTErrBadPath;
    case -193:
    case -192:
        return kFTErrFileNotFound;
    case -186:
    case -185:
        return kFTErrResType;
    case afpAccessDenied:
        return kFTErrPermission;
    default:
        return kFTErrBadPath;
    }
}
