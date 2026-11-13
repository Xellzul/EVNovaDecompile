// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgerfilemgr.c -- BurgerLib FmFileManager: path prefixes, expansion, existence, loading and saving, the stream helpers. Notes: Research/notes/burgerfilemgr.md */

#include <stdio.h>
#include <stdlib.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kFirstHardDrive 2                        /* C: */

int32_t SaveAFileFP(evn_FILE *file, const void *data, uint32_t length);
void *LoadAFileHandleFP(evn_FILE *file);

Handle PrefixHandles[PREFIXMAX];                /* 0x0087071c */
static Word8 SetPfxInited;                       /* 0x00867488 */

/* ReleasePrefixs (BurgerLib FmFileManager, the atexit hook SetAPrefix installs).
   STATE: MATCH 100.0%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1cb0
void ReleasePrefixs(void)
{
    Word i;
    Handle *p;

    for (i = 0, p = PrefixHandles; i < PREFIXMAX; i++) {
        DeallocAHandle(*p);
        *p = 0;
        p++;
    }
}

/* DoesFileExist (BurgerLib FmFileManager). STATE: MATCH 100.0%, exe 12 / obj 12,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1cf0
Word DoesFileExist(const char *name)
{
    char path[kPathBufferSize];

    ExpandAPathToBufferNative(path, name);
    return DoesFileExistNative(path);
}

/* CreateDirectoryPath (BurgerLib CreateDirectoryPath). STATE: MATCH 100.0%, exe 12 / obj 12,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1d30
int32_t CreateDirectoryPath(const char *path)
{
    char expanded[kPathBufferSize];

    ExpandAPathToBufferNative(expanded, path);
    return CreateDirectoryPathNative(expanded);
}

/* GetADirectoryEntryExtension (BurgerLib FmFileManager): the next file (not directory, not
   empty) whose extension is ext; 1 at the end. STATE: MATCH 100.0%, exe 41 / obj 41,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1d70
Word GetADirectoryEntryExtension(DirectorySearch_t *dir, const char *ext)
{
    char *found;

    while (GetADirectoryEntry(dir) == 0) {
        if (dir->Dir == 0 && dir->FileSize != 0) {
            found = GetFileExtension(dir->Name);
            if (found != 0 && stricmp(found, ext) == 0)
                return 0;
        }
    }
    return 1;
}

/* FOpenExpandedPath. STATE: MATCH 100%, exe 13 / obj 13 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1df0
void *FOpenExpandedPath(const char *path, const char *mode)
{
    char expanded[kPathBufferSize];

    ExpandAPathToBufferNative(expanded, path);
    return fopen(expanded, mode);
}

/* CopyAFile (BurgerLib FmFileManager): a file copied in 64K pieces. Not linked. */
Word CopyAFile(const char *dest, const char *source)
{
    FILE *in;
    FILE *out;
    void *buffer;
    ULong size;
    ULong chunk;
    Word result;

    result = 1;
    buffer = AllocAPointer(0x10000);
    if (buffer != 0) {
        in = (FILE *)FOpenExpandedPath(source, "rb");
        if (in != 0) {
            out = (FILE *)FOpenExpandedPath(dest, "wb");
            if (out != 0) {
                result = 0;
                size = fgetfilesize(in);
                while (size != 0) {
                    chunk = size;
                    if (chunk > 0x10000)
                        chunk = 0x10000;
                    if (fread(buffer, 1, chunk, in) != chunk ||
                        fwrite(buffer, 1, chunk, out) != chunk) {
                        result = 1;
                        break;
                    }
                    size -= chunk;
                }
                if (fclose(out) != 0)
                    result = 1;
            }
            fclose(in);
        }
        DeallocAPointer(buffer);
    }
    return result;
}

/* DeleteAFile (BurgerLib FmFileManager). STATE: MATCH 100.0%, exe 12 / obj 12,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1e30
Word DeleteAFile(const char *name)
{
    char path[kPathBufferSize];

    ExpandAPathToBufferNative(path, name);
    return DeleteAFileNative(path);
}

/* ExpandAPathToBuffer (BurgerLib FmFileManager): a name into BurgerLib's own form, the prefix
   ("8:" when none is given) expanded, "." and ".." resolved against it, one ':' at the end.
   STATE: MATCH 100%, exe 158 / obj 158, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f1e70
void ExpandAPathToBuffer(char *out, const char *name)
{
    Handle prefix;
    Word length;
    Word index;
    Word Temp;
    Word c;

    prefix = 0;
    if (name == 0)
        name = "";
    length = strlen(name);
    if (length != 0) {
        Temp = (Word8)name[0];
        index = 1;
        if (Temp == ':')
            goto Copy;
        if (Temp == '.' && ((Word8)name[1] == 'D' || (Word8)name[1] == 'd'))
            goto Copy;
        if (Temp == '$') {
            Temp = PREFIXSYSTEM;
            goto Skip;
        }
        if (Temp == '*') {
            Temp = PREFIXBOOT;
            goto Skip;
        }
        if (Temp == '@') {
            Temp = PREFIXPREFS;
            goto Skip;
        }
        if (Temp >= '0' && Temp < '9' + 1) {
            Temp &= 0xf;
            for (;;) {
                if (index == length || (c = (Word8)name[index]) == ':') {
                    if (Temp >= PREFIXMAX)
                        break;
                    goto Skip;
                }
                if (c < '0' || c >= '9' + 1)
                    break;
                Temp = Temp * 10 + (c & 0xf);
                ++index;
            }
        }
    }
    Temp = kPrefixCurrent;
    goto Lookup;
Skip:
    if (length != index && (Word8)name[index] == ':')
        ++index;
    length -= index;
    name += index;
Lookup:
    prefix = PrefixHandles[Temp];
    index = 1;
    if (prefix != 0)
        index = GetAHandleSize(prefix);
    if ((Word8)name[0] == '.') {
        ++name;
        --length;
        --index;
        c = (Word8)name[0];
        while (c == '.') {
            if (index != 0) {
                Word8 *work;

                --index;
                work = (Word8 *)*prefix;
                do {
                    --index;
                } while (index != -1 && work[index] != ':');
                ++index;
            }
            ++name;
            --length;
            c = (Word8)name[0];
        }
        if (c == ':') {
            ++name;
            --length;
        }
        ++index;
    }
Copy:
    --index;
    if (index != 0 && prefix != 0)
        memcpy(out, *prefix, index);
    if (length != 0)
        memcpy(out + index, name, length);
    index += length;
    if (index != 0 && (Word8)out[index - 1] != ':') {
        out[index] = ':';
        ++index;
    }
    out[index] = 0;
}

/* SetAPrefix (BurgerLib FmFileManager): 0, or -1 for a bad number or no memory.
   STATE: MATCH 100.0%, exe 67 / obj 67, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f20b0
unsigned int SetAPrefix(unsigned int num, const char *name)
{
    char path[kPathBufferSize];
    Handle handle;

    if (SetPfxInited == 0) {
        SetPfxInited = 1;
        atexit(ReleasePrefixs);
    }
    if (num < PREFIXMAX) {
        handle = 0;
        if (name != 0 && name[0] != 0) {
            ExpandAPathToBuffer(path, name);
            if (path[0] != 0) {
                handle = StrCopyHandle(path);
                if (handle == 0)
                    return -1;
            }
        }
        DeallocAHandle(PrefixHandles[num]);
        PrefixHandles[num] = handle;
        return 0;
    }
    NonFatal("Invalid prefix number\n");
    return -1;
}

/* BurgerLib's GetAPrefix: a copy of prefix num in a new pointer ("" when unset). */
static char *GetAPrefix(Word num)
{
    Handle handle;
    Word length;
    char *result;

    if (num < PREFIXMAX) {
        handle = PrefixHandles[num];
        if (handle != 0) {
            length = GetAHandleSize(handle);
            result = (char *)AllocAPointer(length);
            if (result != 0)
                memcpy(result, *handle, length);
            return result;
        }
        return (char *)AllocAPointerClear(1);
    }
    NonFatal("Invalid prefix number\n");
    return 0;
}

/* PopAPrefix (BurgerLib FmFileManager): drop the last directory from prefix num.
   STATE: MATCH 100.0%, exe 73 / obj 73, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f21b0
void PopAPrefix(unsigned int num)
{
    char *path;
    char *end;
    Word length;

    path = GetAPrefix(num);
    if (path != 0) {
        length = strlen(path);
        if (length >= 3) {
            end = path + (length - 2);
            do {
                if ((Word8)*end == ':')
                    break;
                --end;
            } while (end != path);
            *end = 0;
        } else {
            path[0] = 0;
        }
        SetAPrefix(num, path);
        DeallocAPointer(path);
    }
}

/* The exe takes the address of its own PARAMETER SLOT (`lea eax,[esp+0x14]'
   is arg1, not a local), so this writes the caller's value out. */
/* fwritelong (BurgerLib's void signature). STATE: MATCH 100.0%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f22b0
void fwritelong(int32_t value, evn_FILE *stream)
{
    fwrite(&value, 1, 4, stream);
}

/* Writes the string AND its terminator -- the count is strlen + 1. */
/* fwritestr (BurgerLib's void signature). STATE: MATCH 100.0%, exe 16 / obj 16, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f22e0
void fwritestr(const char *s, evn_FILE *stream)
{
    fwrite(s, 1, strlen(s) + 1, stream);
}

/* fgetlong (BurgerLib fgetlong). STATE: MATCH 100.0%, exe 12 / obj 12, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f2310
int32_t fgetlong(evn_FILE *stream)
{
    int32_t value;

    fread(&value, 1, 4, stream);
    return value;
}

/* Reads to the NUL whatever the buffer holds; 1 on the NUL, 0 on EOF. */
/* fgetstr. STATE: MATCH 100%, exe 39 / obj 39 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2350
int32_t fgetstr(char *buf, int32_t size, FILE *stream)
{
    char *end;
    int32_t c;

    end = buf + size;
    for (;;) {
        c = fgetc(stream);
        if (c == 0 || c == -1)
            break;
        if (buf < end - 1)
            *buf++ = c;
    }
    if (size != 0)
        *buf = 0;
    if (c != 0)
        return 0;
    return 1;
}

/* SaveAFile. STATE: MATCH 100%, exe 23 / obj 23 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f23d0
int32_t SaveAFile(const char *path, const void *data, uint32_t length)
{
    FILE *file;

    file = (FILE *)FOpenExpandedPath(path, "wb");
    if (file == 0) {
        NonFatal("Can't open the file to save to\n");
        return 0;
    }
    return SaveAFileFP(file, data, length);
}

/* SaveAFileFP. STATE: MATCH 100%, exe 39 / obj 39 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2430
int32_t SaveAFileFP(evn_FILE *file, const void *data, uint32_t length)
{
    uint32_t written;
    int closed;

    if (file != 0) {
        written = fwrite(data, 1, length, file);
        closed = fclose(file);
        if (written == length && closed == 0)
            return 1;
    }
    NonFatal("Error in saving file contents\n");
    return 0;
}

/* SaveATextFile (BurgerLib FmFileManager): SaveAFile in text mode. Not linked. */
Word SaveATextFile(const char *name, const void *data, uint32_t length)
{
    FILE *file;
    uint32_t written;
    int closed;

    file = (FILE *)FOpenExpandedPath(name, "w");
    if (file == 0) {
        NonFatal("Can't open the file to save to\n");
        return 0;
    }
    written = fwrite(data, 1, length, file);
    closed = fclose(file);
    if (written == length && closed == 0)
        return 1;
    NonFatal("Error in saving file contents\n");
    return 0;
}

/* LoadAFile (BurgerLib FmFileManager). STATE: MATCH 100.0%, exe 33 / obj 33,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f24c0
void *LoadAFile(const char *name, uint32_t *length)
{
    FILE *fp;

    fp = (FILE *)FOpenExpandedPath(name, "rb");
    if (fp != 0)
        return LoadAFileFP(fp, length);
    NonFatal("File %s was not found\n", name);
    if (length != 0)
        *length = 0;
    return 0;
}

/* LoadAFileNative (BurgerLib FmFileManager): LoadAFile for a name already expanded. Not linked. */
void *LoadAFileNative(const char *name, uint32_t *length)
{
    FILE *fp;

    fp = fopen(name, "rb");
    if (fp != 0)
        return LoadAFileFP(fp, length);
    NonFatal("File not found\n");
    if (length != 0)
        *length = 0;
    return 0;
}

/* LoadAFileFP (BurgerLib FmFileManager): the whole of an open file in a new pointer; closes
   it. STATE: MATCH 100.0%, exe 60 / obj 60, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2530
void *LoadAFileFP(FILE *fp, uint32_t *length)
{
    ULong size;
    void *data;

    data = 0;
    size = 0;
    if (fp != 0) {
        size = fgetfilesize(fp);
        if (size != 0) {
            data = AllocAPointer(size);
            if (data != 0 && fread(data, 1, size, fp) != size) {
                DeallocAPointer(data);
                data = 0;
                size = 0;
                NonFatal("Error in loading file\n");
            }
        } else {
            NonFatal("Empty file\n");
        }
        fclose(fp);
    }
    if (length != 0)
        *length = size;
    return data;
}

/* LoadAFileHandle. STATE: MATCH 100.0%, exe 25 / obj 25, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f25f0
void *LoadAFileHandle(const char *path)
{
    FILE *file;

    file = (FILE *)FOpenExpandedPath(path, "rb");
    if (file == 0) {
        NonFatal("File %s was not found\n", path);
        return 0;
    }
    return LoadAFileHandleFP(file);
}

/* Read the whole file into a new handle and close it. */
/* LoadAFileHandleFP. STATE: MATCH 100.0%, exe 62 / obj 62, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2650
void *LoadAFileHandleFP(evn_FILE *file)
{
    ULong size;
    ULong got;
    void *h;

    h = 0;
    if (file != 0) {
        size = fgetfilesize(file);
        if (size == 0) {
            NonFatal("Empty file\n");
        } else {
            h = AllocAHandle2(size, 0);
            if (h != 0) {
                got = fread(LockAHandle((Handle)h), 1, size, file);
                UnlockAHandle((Handle)h);
                if (got != size) {
                    DeallocAHandle(h);
                    h = 0;
                    NonFatal("Error in loading file\n");
                }
            }
        }
        fclose(file);
    }
    return h;
}

/* FindAVolumeByName (BurgerLib FmFileManager): the drive number carrying ":name:", or -1.
   STATE: MATCH 100.0%, exe 54 / obj 54, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2720
Word FindAVolumeByName(const char *name)
{
    Word drive;
    int differ;
    Word oldFlag;
    char *volume;

    drive = kFirstHardDrive;
    oldFlag = SetErrBombFlag(0);
    do {
        volume = GetAVolumeName(drive);
        if (volume != 0) {
            differ = stricmp(volume, name);
            DeallocAPointer(volume);
            if (differ == 0) {
                SetErrBombFlag(oldFlag);
                return drive;
            }
        }
    } while (++drive < kDriveCount);
    SetErrBombFlag(oldFlag);
    return kNoVolume;
}

#ifdef __cplusplus
}
#endif
