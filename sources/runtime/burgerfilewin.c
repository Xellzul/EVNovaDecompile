// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgerfilewin.c -- BurgerLib FmWin95Code: directory scans, native paths and volume names. Notes: Research/notes/burgerfilewin.md */

#include <stdio.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The Win32 search record BurgerLib keeps as bytes in DirectorySearch_t */
#define FindData(dir) ((WIN32_FIND_DATAA *)(dir)->MyFindT)

#define kFileAttrReadOnly   0x01
#define kFileAttrHidden     0x02
#define kFileAttrSystem     0x04
#define kFileAttrDirectory  0x10
#define kSearchOpen         1
#define kSearchStepping     2
#define kFailCriticalErrors 1
#define kVolumeNameSize     0x40
#define kErrorAlreadyExists 183
#define kVolumeColon        ':'
#define kPathSeparator      '\\'
#define kPathBuffer         0x2000
#define kBackslash          '\\'
#define kColon              ':'
#define kLowerA             'a'
#define kAboveLowerZ        '{'
#define kUpperMask          0xdf

/* DoesFileExistNative (BurgerLib FmWin95Code): 1 for a file, 0 for a directory or nothing.
   STATE: MATCH 100.0%, exe 10 / obj 10, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9150
Word DoesFileExistNative(const char *name)
{
    if (GetFileAttributesA(name) & kFileAttrDirectory)
        return 0;
    return 1;
}

/* OpenADirectory (BurgerLib FmWin95Code). STATE: MATCH 100.0%, exe 60 / obj 60,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9180
Word OpenADirectory(DirectorySearch_t *dir, const char *name)
{
    char path[kPathBufferSize];
    char pattern[kPathBufferSize];
    Word length;

    ExpandAPathToBufferNative(path, name);
    length = strlen(path);
    strcpy(pattern, path);
    if (length != 0 && path[length - 1] != '\\')
        strcat(pattern, "\\");
    strcat(pattern, "*.*");
    dir->FindHandle = FindFirstFileA(pattern, (WIN32_FIND_DATAA *)dir->MyFindT);
    if (dir->FindHandle != INVALID_HANDLE_VALUE) {
        dir->HandleOk = kSearchOpen;
        return 0;
    }
    return 1;
}

static void ConvertDosDate(TimeDate_t *out, FILETIME *in)
{
    SYSTEMTIME st;

    FileTimeToLocalFileTime(in, in);
    FileTimeToSystemTime(in, &st);
    out->Milliseconds = st.wMilliseconds;
    out->Second = st.wSecond;
    out->Minute = st.wMinute;
    out->Hour = st.wHour;
    out->Day = st.wDay;
    out->DayOfWeek = st.wDayOfWeek;
    out->Month = st.wMonth;
    out->Year = st.wYear;
}

/* GetADirectoryEntry (BurgerLib FmWin95Code): the next entry, skipping "." and "..";
   1 at the end. STATE: MATCH 100%, exe 134 / obj 134, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9280
Word GetADirectoryEntry(DirectorySearch_t *dir)
{
    Word32 flags;

    for (;;) {
        if (dir->HandleOk == kSearchStepping && FindNextFileA(dir->FindHandle, FindData(dir)) == 0)
            return 1;
        dir->HandleOk = kSearchStepping;
        strcpy(dir->Name, FindData(dir)->cFileName);
        flags = FindData(dir)->dwFileAttributes;
        dir->Dir = 0;
        dir->Hidden = 0;
        dir->System = 0;
        dir->Locked = 0;
        if (!(flags & kFileAttrDirectory))
            break;
        dir->Dir = 1;
        if (strcmp(".", dir->Name) != 0 && strcmp("..", dir->Name) != 0)
            break;
    }
    if (flags & kFileAttrHidden)
        dir->Hidden = 1;
    if (flags & kFileAttrSystem)
        dir->System = 1;
    if (flags & kFileAttrReadOnly)
        dir->Locked = 1;
    ConvertDosDate(&dir->Create, &FindData(dir)->ftCreationTime);
    ConvertDosDate(&dir->Modify, &FindData(dir)->ftLastWriteTime);
    /* BUG: the size halves are swapped: any file with a non-zero low word reads as 0xffffffff */
    if (FindData(dir)->nFileSizeLow != 0)
        dir->FileSize = 0xffffffff;
    else
        dir->FileSize = FindData(dir)->nFileSizeHigh;
    return 0;
}

/* CloseADirectory (BurgerLib FmWin95Code). STATE: MATCH 100.0%, exe 14 / obj 14,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d94b0
void CloseADirectory(struct DirectorySearch_t *dir)
{
    if (dir->HandleOk != 0) {
        FindClose(dir->FindHandle);
        dir->HandleOk = 0;
        dir->FindHandle = 0;
    }
}

/* DeleteAFileNative (BurgerLib FmWin95Code): a file, or failing that an empty directory;
   answers 0 on success. STATE: MATCH 100.0%, exe 20 / obj 20, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x004d94f0
Word DeleteAFileNative(const char *name)
{
    if (DeleteFileA(name) != 0 || RemoveDirectoryA(name) != 0)
        return 0;
    return 1;
}

/* BurgerLib's DirCreate, inlined twice below: 0 when the directory is there afterwards. */
static long DirCreate(const char *path)
{
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = 0;
    if (CreateDirectoryA(path, &sa) != 0 || GetLastError() == kErrorAlreadyExists)
        return 0;
    return 1;
}

/* Create every directory along a native path; 0 once the last exists. */
/* CreateDirectoryPathNative (BurgerLib CreateDirectoryPathNative). STATE: MATCH 100%, exe 96 / obj 96
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9530
long CreateDirectoryPathNative(char *path)
{
    char *p;
    unsigned char saved;
    long failed;

    if (DirCreate(path) == 0)
        return 0;
    if (path[0] != 0) {
        p = path;
        if (path[0] != 0 && path[1] == kVolumeColon)
            p = p + 2;
        if (*p == kPathSeparator)
            p = p + 1;
        do {
            p = (char *)strchr((const char *)p, kPathSeparator);
            if (p == 0)
                p = strchr(path, 0);
            saved = *p;
            *p = 0;
            failed = DirCreate(path);
            *p = saved;
            ++p;
        } while (saved != 0);
        if (failed == 0)
            return 0;
    }
    return 1;
}

/* ExpandAPathToBufferNative (BurgerLib FmWin95Code): ":volume:dir:file" becomes "X:\dir\file"
   (or "\\volume\dir\file" for a volume no drive carries), ".Dn:" drive n. STATE: MATCH
   100.0%, exe 116 / obj 116, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9680
void ExpandAPathToBufferNative(char *out, const char *name)
{
    char path[kPathBufferSize];
    Word8 c;
    Word8 next;
    Word t;
    Word index;
    Word device;
    char *src;

    ExpandAPathToBuffer(path, name);
    device = kNoVolume;
    src = path;
    if ((Word8)path[0] == ':') {
        index = 0;
        do {
            ++index;
            c = src[index];
        } while (c != ':' && c != 0);
        next = src[index + 1];
        src[index] = ':';
        src[index + 1] = 0;
        device = FindAVolumeByName(src);
        src[index] = c;
        src[index + 1] = next;
        if (device == kNoVolume) {
            *out++ = '\\';
        } else {
            src += index;
            if (c != 0)
                ++src;
        }
    } else if ((Word8)src[0] == '.') {
        t = (Word8)src[1];
        if (t >= 'a' && t < 'z' + 1)
            t &= 0xdf;
        if (t == 'D') {
            index = 2;
            device = 0;
            for (;;) {
                t = (Word8)src[index];
                if (t == ':') {
                    ++index;
                    break;
                }
                if (t < '0' || t >= '9' + 1) {
                    index = 0;
                    device = kNoVolume;
                    break;
                }
                ++index;
                device = device * 10 + (t - '0');
            }
            src += index;
        }
    }
    if (device != kNoVolume) {
        out[0] = (char)(device + 'A');
        out[1] = ':';
        out[2] = '\\';
        out += 3;
    }
    if (*src != 0) {
        do {
            c = *src++;
            if (c == ':')
                c = '\\';
            *out++ = c;
        } while (c != 0);
        out -= 2;
        if ((Word8)*out != '\\')
            ++out;
    }
    *out = 0;
}

/* GetAVolumeName (BurgerLib FmWin95Code): ":label:" of drive driveNum in a new pointer, or 0.
   STATE: MATCH 100.0%, exe 68 / obj 68, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9820
char *GetAVolumeName(Word driveNum)
{
    char label[kVolumeNameSize];
    char root[8];
    char *result;
    Word oldMode;
    Word length;

    root[0] = driveNum + 'A';
    root[1] = ':';
    root[2] = '\\';
    root[3] = 0;
    oldMode = SetErrorMode(kFailCriticalErrors);
    result = 0;
    if (GetVolumeInformationA(root, label, sizeof(label), 0, 0, 0, 0, 0) != 0) {
        length = strlen(label);
        if (length != 0) {
            result = (char *)AllocAPointer(length + 3);
            if (result != 0) {
                result[0] = ':';
                memcpy(result + 1, label, length);
                result[length + 1] = ':';
                result[length + 2] = 0;
            }
        }
    }
    SetErrorMode(oldMode);
    return result;
}

/* The length of an open file, its position left where it was. */
/* fgetfilesize. STATE: MATCH 100%, exe 29 / obj 29 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9920
ULong fgetfilesize(FILE *fp)
{
    long here;
    long length;

    here = ftell(fp);
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, here, SEEK_SET);
    return length;
}

/* BurgerLib publishes PREFIXBOOT 32, PREFIXPREFS 33 and PREFIXSYSTEM 34, which are
   this file's 0x20, 0x21 and 0x22. */

extern char *getenv(const char *name);           /* 0x00508e00, MSL */

#define kDirBuffer      0x104
#define kVolumeNameLen  3

/* SetDefaultPrefixs. STATE: MATCH 100%, exe 97 / obj 97 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9990
void SetDefaultPrefixs(void)
{
    char dir[kDirBuffer];
    char *path;
    char *env;

    path = ConvertNativePathToPath("");
    if (path != 0) {
        SetAPrefix(kPrefixCurrent, path);
        DeallocAPointer(path);
    }

    path = ConvertNativePathToPath(__argv[0]);
    if (path != 0) {
        SetAPrefix(kPrefixApp, path);
        DeallocAPointer(path);
        PopAPrefix(kPrefixApp);
    }

    GetSystemDirectoryA(dir, kDirBuffer);
    path = ConvertNativePathToPath(dir);
    if (path != 0) {
        SetAPrefix(PREFIXSYSTEM, path);
        path[kVolumeNameLen] = 0;
        SetAPrefix(PREFIXBOOT, path);
        DeallocAPointer(path);
    }

    env = getenv("APPDATA");
    if (env != 0) {
        path = ConvertNativePathToPath(env);
    } else {
        GetWindowsDirectoryA(dir, kDirBuffer);
        path = ConvertNativePathToPath(dir);
    }
    if (path != 0) {
        SetAPrefix(PREFIXPREFS, path);
        DeallocAPointer(path);
    }
}

/* ConvertNativePathToPath (BurgerLib ConvertNativePathToPath). STATE: MATCH 100%, exe 95 / obj 95
   instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d9b10
char *ConvertNativePathToPath(const char *path)
{
    char full[kPathBuffer];
    char out[kPathBuffer];
    char *filePart;
    char *dst;
    char *src;
    Word c;
    Word len;
    char *result;

    dst = out;
    src = full;
    if (*path == 0)
        path = ".";
    GetFullPathNameA(path, kPathBuffer, src, &filePart);
    if (src[0] == kBackslash && src[1] == kBackslash) {
        *dst++ = kColon;
        src += 2;
    } else {
        c = src[0];
        if (c >= kLowerA && c < kAboveLowerZ)
            c &= kUpperMask;
        src += 3;
        dst[0] = '.';
        dst[1] = 'D';
        NumberToAscii(dst + 2, c - 'A', 0);
        dst += strlen(dst);
        *dst++ = kColon;
    }
    c = *src;
    while (c != 0) {
        if (c == kBackslash)
            c = kColon;
        *dst = (char)c;
        ++src;
        c = *src;
        ++dst;
    }
    len = dst - out;
    if (len != 0 && out[len - 1] != kColon)
        out[len++] = kColon;

    result = (char *)AllocAPointer(len + 1);
    if (result != 0) {
        if (len != 0)
            memcpy(result, out, len);
        result[len] = 0;
    }
    return result;
}

#ifdef __cplusplus
}
#endif
