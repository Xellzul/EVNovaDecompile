// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atfilespec3.c -- atfilespec, exe run 3. Notes: Research/notes/atfilespec.md */

#include <stdlib.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "macport.h"
#include "win32.h"
#include "atfiletool.h"
#define ATSTR_LEN
#define ATSTR_JOIN
#define ATSTR_CMP
#include "atfilestr.h"

#define kPathMax            0x208                /* two MAX_PATHs */
#define kMaxPath            260
#define kBadFilePointer     0xffffffffUL         /* INVALID_SET_FILE_POINTER */

long _FT_PathSplitLeaf(char *path, char *leaf);
long _FT_BufferOpen(void **handle, unsigned char mode, FTFile *file);

/* _FT_IsIndirectBusy (the handle-release hook FT_Open installs). STATE: MATCH 100%, 9/9,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509fa2
char _FT_IsIndirectBusy(void *handle)
{
    char released;

    if (_FT_IsBufferOpen(handle) != 0)
        released = 1;
    else
        released = 0;
    return released;
}

/* _FT_PathGoTo (FT special-folder spec). STATE: MATCH 100%, exe 290 / obj 290,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509fb7
long _FT_PathGoTo(void *ref, long folder)
{
    char path[kPathMax];
    char full[kPathMax];
    void *token;
    char *filePart;
    ULong len;
    long err;
    FolderSpec **spec = ref;
    long result;
    ULong i;
    char *home;
    SHGetFolderPathProc getFolderPath;
    UserProfileDirProc userProfileDir;
    AllUsersProfileDirProc allUsersProfileDir;
    OpenProcessTokenProc openToken;

    err = 0;
    token = 0;
    filePart = 0;
    result = 0;
    IndirectGetSize((void **)spec);
    getFolderPath = (SHGetFolderPathProc)GetProcAddress(GetModuleHandleA("shell32.dll"),
                                                        "SHGetFolderPathA");
    switch (folder) {
    case kFolderDocuments:
        if (getFolderPath != 0 &&
            getFolderPath(0, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, 0, 0, path) == 0)
            break;
        if (getFolderPath != 0)
            err = GetLastError();
        home = getenv("HOMEPATH");
        if (home != 0) {
            StringCopySafe(path, home, kPathMax);
            break;
        }
        len = GetSystemDirectoryA(path, kPathMax);
        if (len == 0) {
            err = GetLastError();
            err = _FT_RemapError(err);
            goto done;
        }
        if (len > kPathMax)
            goto badPath;
        break;

    case kFolderTemporary:
        if (getFolderPath != 0 &&
            getFolderPath(0, CSIDL_INTERNET_CACHE | CSIDL_FLAG_CREATE, 0, 0, path) == 0)
            break;
        if (getFolderPath != 0)
            err = GetLastError();
        len = GetTempPathA(kPathMax, path);
        if (len == 0) {
            err = GetLastError();
            err = _FT_RemapError(err);
            goto done;
        }
        if (len > kPathMax)
            goto badPath;
        break;

    case kFolderAppData:
    case kFolderUserData:
        if (getFolderPath != 0 &&
            getFolderPath(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, 0, 0, path) == 0)
            break;
        if (getFolderPath != 0)
            err = GetLastError();
        openToken = (OpenProcessTokenProc)GetProcAddress(GetModuleHandleA("advapi32.dll"),
                                                         "OpenProcessToken");
        if (openToken != 0 &&
            (userProfileDir = (UserProfileDirProc)GetProcAddress(
                 GetModuleHandleA("userenv.dll"), "GetUserProfileDirectoryA")) != 0) {
            if (!openToken(GetModuleHandleA(0), TOKEN_QUERY, &token)) {
                err = GetLastError();
                err = _FT_RemapError(err);
                goto done;
            }
            len = kPathMax;
            if (!userProfileDir(token, path, &len)) {
                err = GetLastError();
                err = _FT_RemapError(err);
                goto done;
            }
            if (len > kPathMax)
                goto badPath;
        } else {
            len = GetSystemDirectoryA(path, kPathMax);
            if (len == 0) {
                err = GetLastError();
                err = _FT_RemapError(err);
                goto done;
            }
            if (len > kPathMax)
                goto badPath;
        }
        break;

    case kFolderApplication:
        len = GetModuleFileNameA(GetModuleHandleA(0), path, kPathMax);
        if (len == 0) {
            err = GetLastError();
            err = _FT_RemapError(err);
            goto done;
        }
        if (len > kPathMax)
            goto badPath;
        for (i = ATStrLen(path); i >= 3; i--) {
            if (path[i] == '\\') {
                path[i] = 0;
                break;
            }
        }
        if (i < 3)
            path[3] = 0;
        break;

    case kFolderShared:
        if (getFolderPath != 0 &&
            getFolderPath(0, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, 0, 0, path) == 0)
            break;
        if (getFolderPath != 0)
            err = GetLastError();
        allUsersProfileDir = (AllUsersProfileDirProc)GetProcAddress(
            GetModuleHandleA("userenv.dll"), "GetAllUsersProfileDirectoryA");
        if (allUsersProfileDir != 0) {
            len = kPathMax;
            if (!allUsersProfileDir(path, &len)) {
                err = GetLastError();
                err = _FT_RemapError(err);
                goto done;
            }
            if (len > kPathMax)
                goto badPath;
            break;
        } else {
            len = GetSystemDirectoryA(path, kPathMax);
            if (len == 0) {
                err = GetLastError();
                err = _FT_RemapError(err);
                goto done;
            }
            if (len > kPathMax)
                goto badPath;
        }
        break;

    default:
        break;
    }
    if (GetFullPathNameA(path, kPathMax - 1, full, &filePart) == 0) {
        err = GetLastError();
        err = _FT_RemapError(err);
        goto done;
    }
    if (!((full[0] >= 'A' && full[0] <= 'Z') || (full[0] >= 'a' && full[0] <= 'z'))) {
        err = kFTErrBadPath;
        goto done;
    }
    if (full[1] != ':' || full[2] != '\\') {
badPath:
        err = kFTErrBadPath;
        goto done;
    }
    i = ATStrLen(full);
    if (i > 3 && full[i - 1] == '\\')
        full[i - 1] = 0;
    result = _FT_PathGoToDrive(spec, full[0], full + 2);
    if (result != 0)
        err = result;

done:
    if (token != 0)
        CloseHandle(token);
    return err;
}

/* The exe searches the path for "\\\\" and measures it, discarding both: a debug check. */
#define CheckPath(p)        StringFindString((p), "\\\\")

/* The same debug check's lengths: ATStrLen's body twice, the path measured and measured again
   when it is longer than a root; both lengths are dropped. */
static inline void ATCheckPathLength(const char *path)
{
    const char *s;
    ULong n;
    const char *s2;
    ULong n2;
    ULong len;
    ULong sink;

    s = path;
    n = -1;
    do {
        n++;
    } while (*s++ != 0);
    len = n;
    if (len > 1) {
        s2 = path;
        n2 = -1;
        do {
            n2++;
        } while (*s2++ != 0);
        sink = n2;
    }
}

#define SEM_FAILCRITICALERRORS  0x0001
#define DRIVE_NO_ROOT_DIR       1
#define kFirstDrive             2                /* C: first; A: last; B: never */

#define DRIVE_UNKNOWN           0
#define DRIVE_REMOVABLE         2
#define DRIVE_FIXED             3
#define DRIVE_REMOTE            4
#define DRIVE_CDROM             5
#define DRIVE_RAMDISK           6
#define FILE_ATTRIBUTE_DIRECTORY 0x10

/* _FT_PathGoToVolumeName (a spec for the root of the volume named `volume`, or of drive `volume` when
   it is one letter; the name compare is case-folded). STATE: MATCH 100%, exe 199 / obj 199,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050a3b7
long _FT_PathGoToVolumeName(FolderSpec **spec, const unsigned char *volume)
{
    long err, e;
    ULong drives, bit, pass, drive;
    unsigned int oldMode, type;
    char want[kMaxPath];
    char fsName[kMaxPath];
    char name[kMaxPath];
    unsigned long serial;
    unsigned long maxComponent;
    unsigned long fsFlags;
    char root[4];
    char *s;

    err = 0;
    IndirectGetSize((void **)spec);
    s = ATPToCStr(want, volume);
    String2UpperCopySafe(s, s, 0xffffffff);
    drives = GetLogicalDrives();
    if (drives == 0) {
        err = GetLastError();
        err = _FT_RemapError(err);
        goto done;
    }
    oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (((1UL << (want[0] - 'A')) & drives) != 0 && want[1] == 0) {
        root[0] = want[0];
        root[1] = ':';
        root[2] = '\\';
        root[3] = 0;
        if (GetVolumeInformationA(root, name, kMaxPath, &serial, &maxComponent, &fsFlags,
                                  fsName, kMaxPath) != 0) {
            drive = want[0] - 'A';
            drives = 0;
        }
    }
    for (pass = 0; drives != 0 && pass < 3; pass++) {
        for (drive = 0; drives != 0 && drive < kDriveCount - 1; drive++) {
            bit = 1UL << drive;
            if ((drives & bit) == 0)
                continue;
            drives &= ~bit;     /* BUG: before the pass test, so only pass 0 ever matches */
            root[0] = 'A' + drive;
            root[1] = ':';
            root[2] = '\\';
            root[3] = 0;
            switch (type = GetDriveTypeA(root)) {
            case DRIVE_UNKNOWN:
                if (pass != 2)
                    continue;
                break;
            case DRIVE_NO_ROOT_DIR:
                continue;
            case DRIVE_REMOVABLE:
                if (pass != 2)
                    continue;
                break;
            case DRIVE_FIXED:
                if (pass != 0)
                    continue;
                break;
            case DRIVE_REMOTE:
            case DRIVE_CDROM:
                if (pass != 1)
                    continue;
                break;
            case DRIVE_RAMDISK:
                if (pass != 0)
                    continue;
                break;
            default:
                if (pass != 2)
                    continue;
                break;
            }
            if (GetVolumeInformationA(root, name, kMaxPath, &serial, &maxComponent, &fsFlags,
                                      fsName, kMaxPath) != 0 && fsName[0] != 0) {
                /* BUG: compares the file system's name, not the volume label */
                if (ATStrCmp(String2UpperCopySafe(fsName, fsName, 0xffffffff), want) == 0)
                    break;
            }
        }
        if (drive < kDriveCount - 1)
            break;
    }
    SetErrorMode(oldMode);
    if (drive == kDriveCount - 1)
        err = kFTErrDirNotFound;
    else {
        e = _FT_PathGoToDrive(spec, (char)('A' + drive), "\\");
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _FT_PathGoToVolume (a spec for the root of the index-th mounted volume, from 1). STATE: MATCH
   100%, exe 96 / obj 96, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050a646
long _FT_PathGoToVolume(FolderSpec **spec, ULong index)
{
    long err, e;
    ULong drives, i, drive;
    unsigned int oldMode;
    char fsName[kMaxPath];
    char name[kMaxPath];
    unsigned long serial;
    unsigned long maxComponent;
    unsigned long fsFlags;
    char root[4];

    err = 0;
    IndirectGetSize((void **)spec);
    drives = GetLogicalDrives();
    if (drives == 0) {
        err = GetLastError();
        err = _FT_RemapError(err);
        goto done;
    }
    oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    for (i = 0; i < kDriveCount - 1; i++) {
        drive = (i + kFirstDrive) % kDriveCount;
        if ((1UL << drive) & drives) {
            root[0] = 'A' + drive;
            root[1] = ':';
            root[2] = '\\';
            root[3] = 0;
            if (GetDriveTypeA(root) != DRIVE_NO_ROOT_DIR &&
                GetVolumeInformationA(root, name, kMaxPath, &serial, &maxComponent,
                                      &fsFlags, fsName, kMaxPath) != 0 &&
                --index == 0)
                break;
        }
    }
    SetErrorMode(oldMode);
    if (index > 1 || index != 0)
        err = kFTErrFileNotFound;
    else {
        e = _FT_PathGoToDrive(spec, (char)('A' + drive), "\\");
        if (e != 0)
            err = e;
    }
done:
    return err;
}

long _FT_PathSplitLeaf(char *path, char *leaf);

/* _FT_PathGoToParent (a spec's parent folder, the left name into `leaf` as a Pascal string;
   kFTErrDirNotFound at a root). STATE: MATCH 100%, exe 185 / obj 185, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050a798
long _FT_PathGoToParent(FolderSpec **spec, unsigned char *leaf)
{
    long err, e;
    char path[kPathMax];

    err = 0;
    IndirectGetSize((void **)spec);
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) == 0)
        (void)((*spec)->names[0] != 0 ? 1 : 0);
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    if (ATStrLen((*spec)->path) > 1)
        e = ATStrLen((*spec)->path);
    if (leaf != 0)
        ATCToPStr(leaf, "");
    if (ATStrCmp((*spec)->path, "\\") == 0) {
        err = kFTErrDirNotFound;
        goto done;
    }
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    e = _FT_PathSplitLeaf(path, (char *)leaf);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (leaf != 0)
        ATCToPInPlace((char *)leaf);
    ATStrCpy((*spec)->path, path + 2);
done:
    return err;
}

static char kFilterAllFiles[] = "All Files (*.*)\0*.*\0";
static char kTitleSelectFolder[] = "Select a folder";
static char kTitleSelectFile[] = "Select a file";
static char kTitleSaveAs[] = "Save As";
static char kSlash[] = "/";

/* _FT_PathGoToChild (step a spec into `name`: a folder joins the path, anything else is kept as
   a Pascal name). STATE: MATCH 100%, exe 163 / obj 163, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050a94b
long _FT_PathGoToChild(FolderSpec **spec, const unsigned char *name, unsigned char last)
{
    long err, e;
    char path[kPathMax];
    unsigned long attributes;

    err = 0;
    e = 0;
    IndirectGetSize((void **)spec);
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) == 0)
        (void)((*spec)->names[0] != 0 ? 1 : 0);
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    ATCheckPathLength((*spec)->path);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    attributes = GetFileAttributesA(path);
    if (attributes == 0xffffffff) {
        err = GetLastError();
        err = _FT_RemapError(err);
        goto done;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        if ((*spec)->path[1] != 0)
            ATStrCat((*spec)->path, "\\");
        ATPToCStr(path, name);
        StringAppendSafe((unsigned char *)(*spec)->path, (unsigned char *)path, kMaxPath);
    } else {
        e = _FT_PathAppend(spec, name);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _FT_IsPathAFile (is a spec's path an existing non-folder). STATE: MATCH 100%, exe 93 / obj 93,
   byte-identical yes. OPEN: none (the mapped error is stored and dropped, as in the exe) */
// FUNCTION: EVNOVA 0x0050aad6
unsigned char _FT_IsPathAFile(FolderSpec **spec)
{
    long err;
    unsigned char isFile;
    char path[kPathMax];
    unsigned long attributes;

    err = 0;
    attributes = 0;
    isFile = 0;
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    ATCheckPathLength((*spec)->path);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    attributes = GetFileAttributesA(path);
    if (attributes == 0xffffffff) {
        err = GetLastError();
        err = _FT_RemapError(err);
    } else if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        isFile = 0;
    else
        isFile = 1;
    return isFile;
}

/* _FT_PathValidate (does a spec's folder exist). STATE: MATCH 100%, exe 86 / obj 86,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050aba9
long _FT_PathValidate(FolderSpec **spec)
{
    long err;
    unsigned long attributes;
    char path[kPathMax];

    err = 0;
    attributes = 0;
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    ATCheckPathLength((*spec)->path);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    attributes = GetFileAttributesA(path);
    if (attributes == 0xffffffff) {
        err = GetLastError();
        err = _FT_RemapError(err);
    }
    return err;
}

/* _FT_PathGoToDrive (FT spec from drive and path). STATE: MATCH 100%, exe 79 / obj 79,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050ac6d
long _FT_PathGoToDrive(FolderSpec **spec, char drive, const char *dirPath)
{
    char root[kPathMax];
    long err;
    long result;

    err = 0;
    IndirectGetSize((void **)spec);
    if (drive >= 'a' && drive <= 'z')
        drive = drive - ('a' - 'A');

    (*spec)->drive[0] = drive;
    (*spec)->drive[1] = ':';
    (*spec)->drive[2] = '\\';
    (*spec)->drive[3] = 0;
    ATStrCpy(root, (*spec)->drive);

    result = _FT_PathFullPath(root, dirPath);
    if (result != 0) {
        err = result;
    } else {
        ATStrCpy((*spec)->path, root + 2);
        (*spec)->drive[0] = drive;
        (*spec)->drive[1] = ':';
        (*spec)->drive[2] = 0;
        (*spec)->drive[3] = 0;
    }
    return err;
}

/* _FT_FileCreate (create info's file, sized and hidden as asked, or folder, inside the spec's
   folder; the spec then names it; a failure deletes the path). STATE: MATCH 100%, exe 181 /
   obj 181, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050ad3a
long _FT_FileCreate(FolderSpec **spec, FTFileInfo *info)
{
    long err;
    void *h;
    unsigned int oldMode;
    int ok;
    char path[kPathMax];

    err = 0;
    h = kNoFile;
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    ATCheckPathLength((*spec)->path);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    ATStrCat(path, "\\");
    ATStrCat(path, info->name);
    if (info->flags[kFTInfoFolder]) {
        ok = CreateDirectoryA(path, 0);
    } else {
        oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_NEW,
                        info->flags[kFTInfoHidden] ? FILE_ATTRIBUTE_HIDDEN
                                                   : FILE_ATTRIBUTE_NORMAL, 0);
        SetErrorMode(oldMode);
        if (h == kNoFile) {
            err = GetLastError();
            err = _FT_RemapError(err);
            goto cleanup;
        }
        if (SetFilePointer(h, info->size, 0, FILE_BEGIN) == kBadFilePointer
            && GetLastError() != 0) {
            err = GetLastError();
            err = _FT_RemapError(err);
            goto cleanup;
        }
        ok = SetEndOfFile(h);
    }
    if (!ok) {
        err = GetLastError();
        err = _FT_RemapError(err);
    } else {
        StringCopySafe((*spec)->path, path + 2, kMaxPath);
    }
cleanup:
    if (h != kNoFile)
        CloseHandle(h);
    /* BUG: passes the Windows path as FT_FileDelete's folder reference (exe 0x0050af1d) */
    if (err != 0)
        FT_FileDelete((long)path, info->name);
    return err;
}

/* _FT_FileDelete (delete the spec's file, or its folder when empty). STATE: MATCH 100%, exe
   105 / obj 105, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050af31
long _FT_FileDelete(FolderSpec **spec)
{
    long err;
    char path[kPathMax];
    unsigned long attributes;

    err = 0;
    attributes = 0;
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    ATCheckPathLength((*spec)->path);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    attributes = GetFileAttributesA(path);
    if (attributes == 0xffffffff)
        goto failed;
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        if (!RemoveDirectoryA(path)) {
failed:
            err = GetLastError();
            err = _FT_RemapError(err);
        }
    } else if (!DeleteFileA(path)) {
        err = GetLastError();
        err = _FT_RemapError(err);
    }
    return err;
}

/* _FT_FileGetDates (the spec's creation and modification times, seconds since 1970). STATE:
   MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050b029
long _FT_FileGetDates(FolderSpec **spec, ULong *created, ULong *modified)
{
    long err;
    void *find;
    char path[kPathMax];
    WIN32_FIND_DATAA data;

    err = 0;
    find = kNoFile;
    *created = *modified = 0;
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    find = FindFirstFileA(path, &data);
    if (find == kNoFile) {
        err = GetLastError();
        err = _FT_RemapError(err);
    } else {
        FindClose(find);
        find = kNoFile;
        *created = _FT_FileTimeToSeconds(&data.ftCreationTime);
        *modified = _FT_FileTimeToSeconds(&data.ftLastWriteTime);
    }
    return err;
}

/* _FT_FileGetFlags (FT info for a spec's path: flags from its attributes, size, dates, type and
   creator by extension; a folder's size is its entry count, and with checkArchive a file of
   files reads as a folder). STATE: MATCH 100%, exe 295 / obj 295, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b109
long _FT_FileGetFlags(FolderSpec **spec, FTFileInfo *info, unsigned char checkArchive)
{
    long err;
    unsigned long attributes;
    void *find;
    char *p;
    char path[kPathMax];
    WIN32_FIND_DATAA data;
    FileTypeMap map;
    ULong count;

    err = 0;
    find = kNoFile;
    attributes = 0xffffffff;
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    if (ATStrLen((*spec)->path) > 1)
        p = (char *)ATStrLen((*spec)->path);
    if (info != 0)
        memset(info, 0, sizeof(FTFileInfo));
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    find = FindFirstFileA(path, &data);
    if (find != kNoFile) {
        FindClose(find);
        find = kNoFile;
    } else {
        attributes = GetFileAttributesA(path);
        if (attributes != 0xffffffff) {
            memset(&data, 0, sizeof(data));
            data.dwFileAttributes = attributes;
            ATStrCpy(data.cFileName, (*spec)->drive);
            if ((p = StringFindDelimiter(data.cFileName, "\\")) != 0)
                *StringFindDelimiter(data.cFileName, "\\") = 0;
        } else {
            if (info == 0) {
                err = _FT_RemapError(GetLastError());
                goto done;
            }
            err = GetLastError();
            err = _FT_RemapError(err);
            goto done;
        }
    }
    if (info == 0)
        goto done;
    ATStrCpy(info->name, data.cFileName);
    info->flags[kFTInfoReadable] = (data.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) ? false : true;
    info->flags[kFTInfoWritable] = (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? false : true;
    info->flags[kFTInfoFolder] = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
    info->flags[kFTInfoHidden] = (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) ? true : false;
    if (checkArchive && !info->flags[kFTInfoFolder] && _FT_FileIsChunky(spec, &count))
        info->flags[kFTInfoArchive] = 1;
    if (info->flags[kFTInfoArchive]) {
        info->flags[kFTInfoFolder] = 1;
        info->size = count;
        info->created = _FT_FileTimeToSeconds(&data.ftCreationTime);
        info->modified = _FT_FileTimeToSeconds(&data.ftLastWriteTime);
    } else if (!info->flags[kFTInfoFolder]) {
        info->size = data.nFileSizeHigh != 0 ? 0xffffffff : data.nFileSizeLow;
        info->created = _FT_FileTimeToSeconds(&data.ftCreationTime);
        info->modified = _FT_FileTimeToSeconds(&data.ftLastWriteTime);
        _FT_FileTypesLookup(info->name, 0, &map);
        info->type = map.type;
        info->creator = map.creator;
    } else {
        if (attributes == 0xffffffff) {
            info->created = _FT_FileTimeToSeconds(&data.ftCreationTime);
            info->modified = _FT_FileTimeToSeconds(&data.ftLastWriteTime);
        }
        ATStrCat(path, "\\*.*");
        find = FindFirstFileA(path, &data);
        if (find != kNoFile) {
            do
                info->size++;
            while (FindNextFileA(find, &data));
            info->size -= 2;
            FindClose(find);
            find = kNoFile;
        }
    }
done:
    return err;
}

/* _FT_IsFileOpen (the open native FTFile for a spec's full path, else NULL). STATE: MATCH
   100%, exe 86 / obj 86, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0050b4af
FTFile *_FT_IsFileOpen(FolderSpec **spec)
{
    char path[kPathMax];
    long err;
    long result;
    FTFile **link;

    err = 0;
    result = -1;
    link = 0;
    IndirectGetSize((void **)spec);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    for (link = &gFT_FileList; *link != 0; link = &(*link)->next)
        if (ATStrCmp((*link)->path, path) == 0)
            break;
    if (link != 0)
        return (FTFile *)*(void **)link;
    return 0;
}

/* _FT_FileOpen (open the spec's path as a native FTFile, read-only or read-write). STATE:
   MATCH 100%, exe 159 / obj 159, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050b56a
long _FT_FileOpen(FolderSpec **spec, unsigned char write, FTFile *file)
{
    long err;
    unsigned int oldMode;
    long e;
    void *h;
    BY_HANDLE_FILE_INFORMATION info;
    char path[kPathMax];

    err = 0;
    IndirectGetSize((void **)spec);
    CheckPath((*spec)->path);
    ATCheckPathLength((*spec)->path);
    ATStrCpy(path, (*spec)->drive);
    ATStrCat(path, (*spec)->path);
    if (!write)
        oldMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    file->hFile = CreateFileA(path, (write ? GENERIC_WRITE : 0) | GENERIC_READ,
                              FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (!write)
        SetErrorMode(oldMode);
    if ((h = file->hFile) == kNoFile) {
        err = GetLastError();
        err = _FT_RemapError(err);
        goto done;
    }
    info.nFileSizeHigh = 0;
    info.nFileSizeLow = GetFileSize(h, 0);
    if (info.nFileSizeLow == 0xffffffff && (e = GetLastError()) != 0) {
        err = GetLastError();
        err = _FT_RemapError(err);
        goto done;
    }
    ATStrCpy(file->path, path);
    file->position = 0;
    file->size = info.nFileSizeHigh != 0 ? 0xffffffff : info.nFileSizeLow;
    file->mode = write;
    file->procs = *(FTProcs *)gFT_FileHooks;
done:
    if (err != 0 && file->hFile != kNoFile)
        _FT_FileClose(file);
    return err;
}

/* _FT_FileClose (FT native file close). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b722
long _FT_FileClose(FTFile *file)
{
    long err;

    err = 0;
    if (file->hFile != kNoFile) {
        if (!CloseHandle(file->hFile)) {
            err = GetLastError();
            err = _FT_RemapError(GetLastError());
        }
        file->hFile = kNoFile;
    }
    return err;
}

/* _FT_FileFlush (FT native flush). STATE: MATCH 100%, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b75d
long _FT_FileFlush(FTFile *file)
{
    long err;

    err = 0;
    if (!FlushFileBuffers(file->hFile))
        err = _FT_RemapError(GetLastError());
    return err;
}

/* _FT_FileRead (FT native read). STATE: MATCH 100%, exe 39 / obj 39, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b784
long _FT_FileRead(FTFile *file, ULong count, void *buffer)
{
    ULong got;
    long err;

    err = 0;
    got = 0;
    if (!ReadFile(file->hFile, buffer, count, &got, 0) || got != count) {
        err = GetLastError();
        SetFilePointer(file->hFile, file->position, 0, FILE_BEGIN);
        err = _FT_RemapError(err);
    } else {
        file->position += count;
    }
    return err;
}

/* _FT_FileWrite (FT native write). STATE: MATCH 100%, exe 85 / obj 85, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b7e8
long _FT_FileWrite(FTFile *file, ULong count, const void *buffer)
{
    ULong got;
    long err;

    err = 0;
    got = 0;
    if (file->size < file->position + count) {
        got = SetFilePointer(file->hFile, count, 0, FILE_CURRENT);
        if (got == kBadFilePointer && GetLastError() != 0) {
            err = GetLastError();
            goto fail;
        }
        /* BUG: the test is inverted: a failed SetEndOfFile passes, a good one reads a
           stale GetLastError (exe 0x0050b83c) */
        err = SetEndOfFile(file->hFile) ? GetLastError() : 0;
        SetFilePointer(file->hFile, file->position, 0, FILE_BEGIN);
        require_noerr(err, fail);
    }
    if (!WriteFile(file->hFile, buffer, count, &got, 0) || got != count) {
        err = GetLastError();
        SetFilePointer(file->hFile, file->size, 0, FILE_BEGIN);
        SetEndOfFile(file->hFile);
        SetFilePointer(file->hFile, file->position, 0, FILE_BEGIN);
fail:
        err = _FT_RemapError(err);
    } else {
        if (file->size < file->position + count)
            file->size = file->position + count;
        file->position += count;
    }
    return err;
}

/* _FT_FileGetEOF (FT native size). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b8d6
long _FT_FileGetEOF(FTFile *file, ULong *size)
{
    long err;
    ULong eof;

    err = 0;
    eof = 0;
    *size = file->size;
    return err;
}

/* _FT_FileSetEOF (FT native set size). STATE: MATCH 100%, exe 81 / obj 81, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050b8f0
long _FT_FileSetEOF(FTFile *file, ULong size)
{
    long err;
    ULong pos;

    err = 0;
    pos = 0;
    pos = SetFilePointer(file->hFile, size, 0, FILE_BEGIN);
    if (pos == kBadFilePointer && GetLastError() != 0)
        goto lastError;
    /* BUG: inverted as in _FT_FileWrite (exe 0x0050b929) */
    err = SetEndOfFile(file->hFile) ? GetLastError() : 0;
    if (err != 0) {
        SetFilePointer(file->hFile, file->position, 0, FILE_BEGIN);
        goto fail;
    }
    file->size = size;
    if (file->position > file->size) {
        pos = SetFilePointer(file->hFile, file->size, 0, FILE_BEGIN);
        if (pos == kBadFilePointer && GetLastError() != 0) {
lastError:
            err = GetLastError();
fail:
            err = _FT_RemapError(err);
        } else {
            file->position = file->size;
        }
    } else {
        pos = SetFilePointer(file->hFile, file->position, 0, FILE_BEGIN);
        if (pos == kBadFilePointer && GetLastError() != 0) {
            err = GetLastError();
            err = _FT_RemapError(err);
        }
    }
    return err;
}

/* _FT_FileGetMark (FT native position). STATE: MATCH 100%, exe 12 / obj 12, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050b9c8
long _FT_FileGetMark(FTFile *file, ULong *position)
{
    long err;
    ULong pos;

    err = 0;
    pos = 0;
    *position = file->position;
    return err;
}

/* _FT_FileSetMark (FT native seek). STATE: MATCH 100%, exe 31 / obj 31, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050b9e2
long _FT_FileSetMark(FTFile *file, ULong position)
{
    long err;
    ULong pos;

    err = 0;
    pos = 0;
    pos = SetFilePointer(file->hFile, position, 0, FILE_BEGIN);
    if (pos == kBadFilePointer && GetLastError() != 0) {
        err = GetLastError();
        err = _FT_RemapError(err);
    } else {
        file->position = position;
    }
    return err;
}

/* _FT_RemapError (Win32 error to FT error). STATE: MATCH 100%, exe 124 / obj 124, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050ba2c
long _FT_RemapError(ULong winError)
{
    long err;

    err = winError;
    switch (err) {
    case ERROR_SUCCESS:
        err = 0;
        break;
    case ERROR_CANCELLED:
        err = kFTErrCancelled;
        break;
    case ERROR_FILE_NOT_FOUND:
        err = kFTErrFileNotFound;
        break;
    case ERROR_PATH_NOT_FOUND:
        err = kFTErrDirNotFound;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
    case ERROR_NO_MORE_FILES:
    case ERROR_NO_MORE_SEARCH_HANDLES:
    case ERROR_TOO_MANY_DESCRIPTORS:
        err = kFTErrTooManyFiles;
        break;
    case ERROR_ACCESS_DENIED:
    case ERROR_INVALID_ACCESS:
    case ERROR_WRITE_PROTECT:
    case ERROR_LOCK_VIOLATION:
    case ERROR_CANNOT_MAKE:
    case ERROR_OPEN_FAILED:
        err = kFTErrPermission;
        break;
    case ERROR_INVALID_HANDLE:
        err = kFTErrBadPath;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
    case ERROR_OUT_OF_STRUCTURES:
        err = kOutOfMemory;
        break;
    case ERROR_INVALID_DRIVE:
    case ERROR_BAD_UNIT:
    case ERROR_REM_NOT_LIST:
    case ERROR_DEV_NOT_EXIST:
    case ERROR_DISK_CHANGE:
    case ERROR_FILE_INVALID:
    case ERROR_NO_VOLUME_ID:
        err = kFTErrNoVolume;
        break;
    case ERROR_CURRENT_DIRECTORY:
    case ERROR_SHARING_VIOLATION:
    case ERROR_DRIVE_LOCKED:
    case ERROR_DIR_NOT_EMPTY:
    case ERROR_PATH_BUSY:
    case ERROR_BUSY:
        err = kFTErrBusy;
        break;
    case ERROR_SEEK:
    case ERROR_SECTOR_NOT_FOUND:
    case ERROR_HANDLE_EOF:
    case ERROR_NEGATIVE_SEEK:
    case ERROR_SEEK_ON_DEVICE:
        err = kFTErrPosition;
        break;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
        err = kFTErrDiskFull;
        break;
    case ERROR_DUP_NAME:
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        err = kFTErrExists;
        break;
    case ERROR_BAD_NETPATH:
    case ERROR_DIR_NOT_ROOT:
    case ERROR_BAD_PATHNAME:
        err = kFTErrDirNotFound;
        break;
    case ERROR_INVALID_NAME:
    case ERROR_NO_VOLUME_LABEL:
        err = kFTErrBadName;
        break;
    default:
        err = kFTErrBadPath;
        break;
    }
    return err;
}

/* Placement only: no reader in the exe. */
static long sUnreadLong = 1;                    /* 0x00579908 */
static const char kDot[] = ".";
static const char kDotDot[] = "..";
static const char kSeparators[] = "/:\\%";

/* _FT_FileTimeToSeconds (a FILETIME as seconds since 1970-01-01, from its calendar fields; leap
   years by the four-year rule only). STATE: MATCH 100%, exe 50 / obj 50, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050bc01
ULong _FT_FileTimeToSeconds(const FILETIME *time)
{
    static const short monthDays[12] = {         /* 0x0057991c */
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    SYSTEMTIME st;
    long years, days;
    int month;

    days = 0;
    FileTimeToSystemTime(time, &st);
    years = st.wYear - kEpochYear;
    days = (years + 1 + (st.wMonth > kFebruary ? 1 : 0)) / 4 + years * kDaysPerYear;
    for (month = 1; month < st.wMonth; month++)
        days += monthDays[month - 1];
    days += st.wDay - 1;
    return ((days * 24 + st.wHour) * 60 + st.wMinute) * 60 + st.wSecond;
}

/* _FT_PathFullPath (FT full path of a directory relative to another). STATE: MATCH 100%, exe 92
   / obj 92, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050bc90
long _FT_PathFullPath(char *dir, const char *relPath)
{
    char saved[kPathMax];
    char *filePart;
    ULong savedLen;
    long len;
    long err;
    unsigned char restore;

    err = 0;
    restore = 0;
    filePart = 0;
    savedLen = GetCurrentDirectoryA(kPathMax, saved);
    if (savedLen == 0)
        goto failed;
    if (savedLen > kPathMax)
        goto badPath;
    restore = 1;
    if (!SetCurrentDirectoryA(dir))
        goto failed;
    len = GetFullPathNameA(relPath, kMaxPath, dir, &filePart);
    if (len == 0) {
failed:
        err = GetLastError();
        err = _FT_RemapError(err);
    } else if (len > kMaxPath) {
badPath:
        err = kFTErrBadPath;
    } else {
        len = ATStrLen(dir);
        if (len > 3 && dir[len - 1] == '\\')
            dir[len - 1] = 0;
        CheckPath(dir);
        if (ATStrLen(dir) > 3)
            len = ATStrLen(dir);
    }
    if (restore) {
        if (!SetCurrentDirectoryA(saved))
            err = GetLastError();
    }
    return err;
}

/* _FT_PathSplitLeaf (cut a path at its last backslash, copying what follows to `leaf`). STATE:
   MATCH 100%, exe 87 / obj 87, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050bd8e
long _FT_PathSplitLeaf(char *path, char *leaf)
{
    long len;
    char found;
    char *last;
    char *start;

    len = 0;
    found = 0;
    last = 0;
    start = path;
    CheckPath(path);
    if (ATStrLen(path) > 1)
        len = ATStrLen(path);
    last = path;
    while ((path = StringFindDelimiter(path, "\\")) != 0) {
        last = path;
        path++;
    }
    if (leaf != 0)
        StringCopySafe(leaf, last + 1, kMaxPath);
    *last = 0;
    CheckPath(start);
    if (ATStrLen(start) > 3)
        len = ATStrLen(start);
    return 0;
}

/* _FT_ChunkFilePreflight (FT mark a file's position). STATE: MATCH 100%, exe 14 / obj 14, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050be45
long _FT_ChunkFilePreflight(FTFile *file, FTMark *mark)
{
    long err;

    err = 0;
    mark->file = file;
    mark->position = file->position;
    do
        file = file->parent;
    while (file != 0);
    return 0;
}

/* _FT_ChunkFilePostflight (FT return to a mark). STATE: MATCH 100%, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050be68
long _FT_ChunkFilePostflight(FTFile *file, FTMark *mark)
{
    long err, result;
    FTFile *marked;

    err = 0;
    marked = mark->file;
    if (marked != 0) {
        result = marked->procs.proc[kFTSeek](marked, mark->position);
        if (result != 0)
            err = result;
    }
    return err;
}
