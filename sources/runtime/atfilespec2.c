// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atfilespec2.c -- atfilespec, exe run 2. Notes: Research/notes/atfilespec.md */

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
#include "atfilestr.h"

long _FT_PathString(FolderSpec **spec, const char *path, unsigned char create);
long _FT_PathShorten(FolderSpec **spec, char *name);
long _FT_PathGoToVolume(FolderSpec **spec, ULong index);
long _FT_PathGoToParent(FolderSpec **spec, unsigned char *leaf);
long _FT_PathValidate(FolderSpec **spec);

#define kParentFolder       -1
#define kSpecialFolderMin   -7
#define kSpecialFolderMax   -2
#define kVolumeIndexMax     100

/* _FT_PathModify (resolve `where` against `base`: nothing, another spec, the parent (-1), a
   special folder (-7..-2), the n-th volume (1..100) or a '/' path; into a new spec at
   `result`, or over `base` when result is NULL). STATE: MATCH 100%, exe 266 / obj 266,
   byte-identical. OPEN: none (a volume-index error still hands back the spec, as the exe) */
// FUNCTION: EVNOVA 0x00508fbd
long _FT_PathModify(FolderSpec **base, void *where, FolderSpec ***result, unsigned char create)
{
    long err, e;
    FolderSpec **copy;
    ULong size;
    long len;

    err = 0;
    copy = 0;
    len = 0;
    IndirectGetSize((void **)base);
    if (result != 0)
        *result = 0;
    if (((*base)->file != 0 ? 1 : 0) == 0) {
        e = _FT_PathValidate(base);
        if (e != 0) {
            err = e;
            err = kFTErrDirNotFound;
            goto done;
        }
    }
    if (where == 0) {
        if (result == 0)
            goto done;
        if ((*result = (FolderSpec **)IndirectDuplicate((void **)base)) == 0)
            goto nomem;
        if ((**result)->file != 0)
            _FT_FileUpdateRefCount((**result)->file, 1);
        (**result)->dir = 0;
        goto done;
    }
    if (_FT_PathLookup((FolderSpec **)where)) {
        if (result != 0) {
            if ((*result = (FolderSpec **)IndirectDuplicate((void **)where)) == 0)
                goto nomem;
            (**result)->next = 0;
            if ((**result)->file != 0)
                _FT_FileUpdateRefCount((**result)->file, 1);
            (**result)->dir = 0;
            goto done;
        }
        copy = (*base)->next;
        size = IndirectGetSize((void **)where);
        e = IndirectSetSize((void **)base, size);
        if (e != 0) {
            err = e;
            goto done;
        }
        ATMemMove(*base, *(FolderSpec **)where, size);
        (*base)->next = copy;
        copy = 0;
        goto done;
    }
    if (result == 0 && !IsSpecID(where) && *(char *)where == 0)
        goto done;
    copy = (FolderSpec **)IndirectDuplicate((void **)base);
    if (copy == 0) {
nomem:
        err = kOutOfMemory;
        goto done;
    }
    if (result != 0)
        (*copy)->next = 0;
    if (where == (void *)kParentFolder) {
        if ((((FolderSpec *)*(void **)copy)->file != 0 ? 1 : 0) != 0
            && ((*copy)->names[0] != 0 ? 1 : 0) == 0) {
            err = kFTErrDirNotFound;
            goto done;
        }
        if (((*copy)->names[0] != 0 ? 1 : 0) != 0) {
            e = _FT_PathShorten(copy, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = _FT_PathGoToParent(copy, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    } else if ((long)where <= kSpecialFolderMax && (long)where >= kSpecialFolderMin) {
        e = _FT_PathGoTo(copy, (long)where);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else if ((ULong)where <= kVolumeIndexMax) {
        err = _FT_PathGoToVolume(copy, (ULong)where);
    } else {
        err = _FT_PathString(copy, (const char *)where, create);
        if (err != 0) {
            if (!create || err != kFTErrFileNotFound)
                err = err;              /* the release build's empty error report */
            goto done;
        }
    }
    if (result != 0) {
        *result = copy;
        copy = 0;
        if ((**result)->file != 0)
            _FT_FileUpdateRefCount((**result)->file, 1);
        (**result)->dir = 0;
        goto done;
    }
    size = IndirectGetSize((void **)copy);
    e = IndirectSetSize((void **)base, size);
    if (e != 0) {
        err = e;
        goto done;
    }
    ATMemMove(*base, *copy, size);
done:
    if (copy != 0)
        IndirectDeallocate((void **)copy);
    return err;
}

/* _FT_PathInsert (register a spec; one naming a file of files on disk keeps it open as its
   `dir`, shared with a copy's by reference count). STATE: MATCH 100%, exe 79 / obj 79,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509241
void _FT_PathInsert(FolderSpec **spec)
{
    long err, e;
    FTFile *archive;
    FTFileInfo info;

    err = 0;
    IndirectGetSize((void **)spec);
    if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) == 0
        && ((*spec)->names[0] != 0 ? 1 : 0) == 0
        && _FT_IsPathAFile(spec)) {
        archive = 0;
        if ((*spec)->dir != 0) {
            _FT_FileUpdateRefCount((*spec)->dir, 1);
        } else {
            e = _FT_FileGetFlags(spec, &info, 0);
            if (e != 0)
                err = e;
            if (err == 0) {
                e = _FT_ChunkOpenPath(spec, 0, 0, 1, &archive, 0);
                if (e != 0)
                    err = e;
            }
            if (err == 0)
                (*spec)->dir = archive;
        }
    }
    (*spec)->next = gFT_PathList;
    gFT_PathList = spec;
}

/* _FT_PathLookup (is a registered spec handle). STATE: MATCH 100%, exe 15 / obj 15,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509301
unsigned char _FT_PathLookup(FolderSpec **spec)
{
    long err;
    unsigned char found;
    FolderSpec ***link;
    FolderSpec **p;

    err = 0;
    found = 0;
    link = &gFT_PathList;
    while ((p = *link) != 0 && p != spec)
        link = &(*p)->next;
    if (p != 0)
        found = 1;
    return found;
}

/* _FT_PathDispose (FT spec dispose). STATE: MATCH 100%, exe 44 / obj 44, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00509323
long _FT_PathDispose(void *ref)
{
    long err;
    FolderSpec **spec;
    FolderSpec ***link;

    spec = (FolderSpec **)ref;
    err = 0;
    if (IsSpecID(ref))
        return 0;

    if ((*spec)->file != 0)
        FT_FileClose((*spec)->file);
    if ((*spec)->dir != 0)
        FT_FileClose((*spec)->dir);

    link = &gFT_PathList;
    while (*link != 0 && *link != ref)
        link = &(**link)->next;
    if (*link != 0)
        *link = (*spec)->next;
    return IndirectDeallocate(ref);
}

#define kFirstName          0x114                /* offsetof(FolderSpec, names) */

long _FT_PathGoToVolumeName(FolderSpec **spec, const unsigned char *volume);
long _FT_PathGoToChild(FolderSpec **spec, const unsigned char *name, unsigned char last);

/* _FT_PathString (walk a spec along a '/'-separated %XX-encoded path; a leading '/' names a
   volume). STATE: MATCH 100%, exe 134 / obj 134, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00509382
long _FT_PathString(FolderSpec **spec, const char *path, unsigned char create)
{
    long err, e;
    unsigned char absolute;
    unsigned char last;
    const char *slash;
    ULong length;
    unsigned char name[256];

    err = 0;
    last = create;
    absolute = path[0] == '/';
    slash = 0;
    IndirectGetSize((void **)spec);
    if (path == 0)
        goto done;
    do {
        if (absolute)
            path++;
        slash = StringFindDelimiter(path, "/");
        length = slash != 0 ? slash - path : ATStrLen(path);
        last = create && !slash;
        if (length != 0) {
            e = _FT_PathNameInput(name, path, length);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (absolute) {
                e = _FT_PathGoToVolumeName(spec, name);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            } else if ((((FolderSpec *)*(void **)spec)->file != 0 ? 1 : 0) != 0) {
                e = _FT_PathAppend(spec, name);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            } else if (((*spec)->names[0] != 0 ? 1 : 0) != 0) {
                e = _FT_PathAppend(spec, name);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            } else {
                err = _FT_PathGoToChild(spec, name, last);
                if (err != 0) {
                    if (last && err == kFTErrFileNotFound)
                        break;
                    goto done;
                }
            }
        }
        path = slash + 1;
        absolute = 0;
    } while (slash != 0);
done:
    return err;
}

/* _FT_PathAppend (append a Pascal name to a spec). STATE: MATCH 100%, exe 51 / obj 51,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005094d9
long _FT_PathAppend(FolderSpec **spec, const unsigned char *name)
{
    long err, e;
    ULong size;

    err = 0;
    IndirectGetSize((void **)spec);
    size = IndirectGetSize((void **)spec);
    e = IndirectSetSize((void **)spec, size + name[0] + 1);
    if (e != 0) {
        err = e;
    } else {
        BlockMoveData(name, *(char **)spec + size - 1, name[0] + 1);
        (*(unsigned char **)spec)[size + name[0]] = 0;
    }
    return err;
}

/* _FT_PathShorten (drop a spec's last name, returning it escaped). STATE: MATCH 100%, exe 48 /
   obj 48, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050953f
long _FT_PathShorten(FolderSpec **spec, char *name)
{
    long err, e;
    long last;
    ULong next;

    err = 0;
    IndirectGetSize((void **)spec);
    next = 0;
    do {
        last = next;
        next = _FT_PathWalk(spec, last);
    } while (next != 0);
    if (name != 0)
        _FT_PathNameOutput(name, (unsigned char *)*spec + last);
    e = IndirectSetSize((void **)spec, last + 1);
    if (e != 0)
        err = e;
    else
        (*(unsigned char **)spec)[last] = 0;
    return err;
}

/* _FT_PathWalk (the offset of the name after `offset`, the first for 0; 0 past the last).
   STATE: MATCH 100%, exe 34 / obj 34, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509598
ULong _FT_PathWalk(FolderSpec **spec, ULong offset)
{
    long err;

    err = 0;
    IndirectGetSize((void **)spec);
    IndirectGetSize((void **)spec);
    if (offset == 0)
        offset = kFirstName;
    else
        offset += (*(unsigned char **)spec)[offset] + 1;
    IndirectGetSize((void **)spec);
    if ((*(unsigned char **)spec)[offset] == 0) {
        IndirectGetSize((void **)spec);
        offset = 0;
    }
    return offset;
}

/* _FT_PathNameInput (a C name to a Pascal name, decoding %XX; a bare '/', ':' or '\' is
   kFTErrBadName). STATE: MATCH 100%, exe 115 / obj 115, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005095e5
long _FT_PathNameInput(unsigned char *out, const char *in, ULong length)
{
    long err;
    ULong n, i;
    char c;
    ULong d;
    int v;

    err = 0;
    i = n = 0;
    while (i < length && (c = in[i]) != 0 && n < 0xff) {
        switch (c) {
        case '/':
        case ':':
        case '\\':
            err = kFTErrBadName;
            goto done;
        case '%':
            d = in[i + 1];
            if (d >= '0' && d <= '9')
                v = (d - '0') << 4;
            else {
                if (d >= 'A' && d <= 'F')
                    v = d - 'A' + 10;
                else if (d >= 'a' && d <= 'f')
                    v = d - 'a' + 10;
                else
                    goto invalid;
                v <<= 4;
            }
            goto second;
        invalid:
            out[++n] = c;
            i++;
            break;
        second:
            d = in[i + 2];
            if (d >= '0' && d <= '9')
                v |= d - '0';
            else if (d >= 'A' && d <= 'F')
                v |= d - 'A' + 10;
            else if (d >= 'a' && d <= 'f')
                v |= d - 'a' + 10;
            else {
                out[++n] = c;
                i++;
                break;
            }
            if (v == ':' || v == '\\') {
                out[++n] = c;
                i++;
            } else {
                out[++n] = (unsigned char)v;
                i += 3;
            }
            break;
        default:
            out[++n] = c;
            i++;
            break;
        }
    }
done:
    if (err != 0)
        n = 0;
    out[0] = (unsigned char)n;
    return err;
}

/* _FT_PathNameOutput (a Pascal name to a C string, %XX for NUL, tab, LF, CR, '%', '/', ':' and
   '\'). STATE: MATCH 100%, exe 67 / obj 67, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509749
long _FT_PathNameOutput(char *out, const unsigned char *name)
{
    const char *hex;
    ULong j;
    ULong k;
    ULong i, o;
    unsigned char c;

    hex = "0123456789ABCDEF";
    j = 0;
    i = 1;
    o = 0;
    while (i <= name[0] && o < 0xff) {
        c = name[i];
        switch (c) {
        case 0:
        case '\t':
        case '\n':
        case '\r':
        case '%':
        case '/':
        case ':':
        case '\\':
            out[o] = '%';
            out[j = o + 1] = hex[(name[i] & 0xf0) >> 4];
            out[k = o + 2] = hex[name[i] & 0x0f];
            i++;
            o += 3;
            break;
        default:
            out[o] = c;
            i++;
            o++;
            break;
        }
    }
    j = o;
    out[j] = 0;
    return 0;
}

/* _FT_FileOpenWrapper (the native FTFile for a spec: the spec's own file, an open one -- reopened
   for writing when asked and it is read-only -- or a newly opened one). STATE: MATCH 100%,
   exe 169 / obj 169, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005097f3
long _FT_FileOpenWrapper(FolderSpec **spec, unsigned char write, FTFile **result)
{
    long err = 0;
    FTFile *file;
    FTFile *open;
    long e, rw;
    long refCount;
    FTFileInfo info;
    ULong position;

    file = 0;
    open = (*spec)->file;
    if ((open != 0 ? 1 : 0) != 0) {
        *result = open;
    } else {
        file = _FT_IsFileOpen(spec);
        if (file != 0) {
            if (file->mode != 0 || !write) {
                *result = file;
            } else {
                rw = 0;
                position = 0;
                refCount = 0;
                e = _FT_FileGetFlags(spec, &info, 0);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                if (write && !info.flags[kFTInfoWritable]) {
                    err = kFTErrPermission;
                    goto done;
                }
                e = _FT_FileGetMark(file, &position);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                refCount = file->refCount;
                file->refCount = 0;
                e = _FT_FileClose(file);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                rw = _FT_FileOpen(spec, 1, file);
                if (rw != 0 && (err = _FT_FileOpen(spec, 0, file)) != 0) {
                    file->refCount = 1;
                    e = FT_FileClose(file);
                } else {
                    file->refCount = refCount;
                    e = _FT_FileSetMark(file, position);
                }
                if (e != 0)
                    err = e;
                e = rw;
                if (0 != e) {
                    err = e;
                    goto done;
                }
                *result = file;
            }
        } else {
            if ((*result = MemoryAllocate(kFTFileSize)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            memset(*result, 0, kFTFileSize);
            (*result)->hFile = kNoFile;
            e = _FT_FileGetFlags(spec, &info, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (write && !info.flags[kFTInfoWritable]) {
                err = kFTErrPermission;
                goto done;
            }
            e = _FT_FileOpen(spec, write, *result);
            if (e != 0) {
                err = e;
                goto done;
            }
            (*result)->next = gFT_FileList;
            gFT_FileList = *result;
        }
    }
    _FT_FileUpdateRefCount(*result, 1);
done:
    if (err != 0 && *result != 0) {
        MemoryDeallocate(*result);
        *result = 0;
    }
    return err;
}

/* _FT_FileIsChunky (is the spec's file a file of files; its entry count). STATE: MATCH 100%, exe
   162 / obj 162, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509a06
unsigned char _FT_FileIsChunky(FolderSpec **spec, ULong *count)
{
    long err;
    const char *sig;
    unsigned char isArchive;
    FTFile *file;
    FTArchiveHeader h;
    ULong saved;

    err = 0;
    sig = "CHNK";
    isArchive = 0;
    file = 0;
    saved = 0xffffffff;
    if (count != 0)
        *count = 0;
    if (_FT_FileOpenWrapper(spec, 0, &file) == 0 && file->size >= kArchiveHeaderSize
        && file->procs.proc[kFTPosition](file, &saved) == 0
        && file->procs.proc[kFTSeek](file, 0) == 0
        && file->procs.proc[kFTRead](file, kArchiveHeaderSize, &h) == 0) {
        h.headerSize = ATSwap32(h.headerSize);
        h.tableOffset = ATSwap32(h.tableOffset);
        h.entrySize = ATSwap32(h.entrySize);
        h.entryCount = ATSwap32(h.entryCount);
        if (IsSignature(h.signature, sig) && h.headerSize >= kArchiveHeaderSize
            && !(file->size < h.entrySize * h.entryCount + h.tableOffset)) {
            isArchive = 1;
            if (count != 0)
                *count = h.entryCount;
        }
    }
    if (file != 0) {
        if (saved != 0xffffffff)
            file->procs.proc[kFTSeek](file, saved);
        FT_FileClose(file);
    }
    return isArchive;
}

/* _FT_IndirectIsChunky (does a handle hold a file of files). STATE: MATCH 100%, exe 125 / obj 125,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00509bfb
unsigned char _FT_IndirectIsChunky(void **handle)
{
    long err;
    const char *sig;
    unsigned char isArchive;
    FTArchiveHeader h;

    err = 0;
    sig = "CHNK";
    isArchive = 0;
    if (IndirectGetSize(handle) >= kArchiveHeaderSize) {
        memmove(&h, *handle, sizeof(h));
        h.headerSize = ATSwap32(h.headerSize);
        h.tableOffset = ATSwap32(h.tableOffset);
        h.entrySize = ATSwap32(h.entrySize);
        h.entryCount = ATSwap32(h.entryCount);
        if (IsSignature(h.signature, sig) && h.headerSize >= kArchiveHeaderSize
            && !(IndirectGetSize(handle) < h.entrySize * h.entryCount + h.tableOffset))
            isArchive = 1;
    }
    return isArchive;
}

/* _FT_FileUpdateRefCount (FT file reference count). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00509d74
long _FT_FileUpdateRefCount(FTFile *file, long delta)
{
    long err;

    err = 0;
    file->refCount += delta;
    return file->refCount;
}
