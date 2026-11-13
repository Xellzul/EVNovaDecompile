/* atfilespec2mac.c -- 1.0.10: the file tool: folder specs, path strings, the file type map. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atfiletoolmac.h"
#include <InternetConfig.h>
#include <CodeFragments.h>
#define ATSTR_MOVE
#include "atfilestr.h"

extern void _AtomicEnter(void);
extern void _AtomicLeave(void);

int32_t _FT_PathString(FolderSpec **spec, const char *path, unsigned char create);
int32_t _FT_PathShorten(FolderSpec **spec, char *name);
int32_t _FT_PathGoToVolume(FolderSpec **spec, ULong index);
int32_t _FT_PathGoToParent(FolderSpec **spec, unsigned char *leaf);
int32_t _FT_PathValidate(FolderSpec **spec);

#define kParentFolder       -1
#define kSpecialFolderMin   -7
#define kSpecialFolderMax   -2
#define kVolumeIndexMax     100

int32_t _FT_PathModify(FolderSpec **base, void *where, FolderSpec ***result, unsigned char create)
{
    int32_t err, e;
    FolderSpec **copy;
    ULong size;
    int32_t len;

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
        if ((*result = (FolderSpec **)IndirectDuplicate((void **)base)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((**result)->file != 0)
            _FT_FileUpdateRefCount((**result)->file, 1);
        (**result)->dir = 0;
        goto done;
    }
    if (_FT_PathLookup(where)) {
        if (result != 0) {
            if ((*result = (FolderSpec **)IndirectDuplicate((void **)where)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            (**result)->next = 0;
            if ((**result)->file != 0)
                _FT_FileUpdateRefCount((**result)->file, 1);
            (**result)->dir = 0;
            goto done;
        }
        copy = (*base)->next;
        size = IndirectGetSize(where);
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
    } else if ((int32_t)where <= kSpecialFolderMax && (int32_t)where >= kSpecialFolderMin) {
        e = _FT_PathGoTo(copy, (int32_t)where);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else if ((ULong)where <= kVolumeIndexMax) {
        err = _FT_PathGoToVolume(copy, (ULong)where);
    } else {
        err = _FT_PathString(copy, where, create);
        if (err != 0) {
            if (create != 0)
                err = FT_ReportError(err);
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

void _FT_PathInsert(FolderSpec **spec)
{
    int32_t err, e;
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

unsigned char _FT_PathLookup(FolderSpec **spec)
{
    int32_t err;
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

void _FT_PathDispose(void *ref)
{
    FolderSpec **spec;
    FolderSpec ***link;

    spec = ref;
    if (IsSpecID(ref))
        return;

    if ((*spec)->file != 0)
        FT_FileClose((*spec)->file);
    if ((*spec)->dir != 0)
        FT_FileClose((*spec)->dir);

    link = &gFT_PathList;
    while (*link != 0 && *link != ref)
        link = &(**link)->next;
    if (*link != 0)
        *link = (*spec)->next;
    IndirectDeallocate(ref);
}

#define kFirstName          0x58                 /* offsetof(FolderSpec, names) */

int32_t _FT_PathGoToVolumeName(FolderSpec **spec, const unsigned char *volume);

int32_t _FT_PathString(FolderSpec **spec, const char *path, unsigned char create)
{
    unsigned char make;
    const char *slash;
    int32_t err, e;
    unsigned char absolute;
    ULong length;
    unsigned char name[256];

    err = 0;
    absolute = path[0] == '/';
    slash = 0;
    IndirectGetSize((void **)spec);
    if (path == 0)
        goto done;
    do {
        if (absolute)
            path++;
        slash = StringFindDelimiter(path, "/");
        length = slash != 0 ? slash - path : FTStrLen(path);
        make = create && !slash;
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
            } else if ((*spec)->file != 0) {
                e = _FT_PathAppend(spec, name);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            } else if ((*spec)->names[0] != 0) {
                e = _FT_PathAppend(spec, name);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            } else {
                err = _FT_PathGoToChild(spec, name, make);
                if (err != 0) {
                    if (make != 0)
                        err = FT_ReportError(err);
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

int32_t _FT_PathClearNames(FolderSpec **spec)
{
    int32_t err, e;

    err = 0;
    IndirectGetSize((void **)spec);
    e = IndirectSetSize((void **)spec, 0x59);
    if (e != 0)
        err = e;
    else
        (*spec)->names[0] = 0;
    return err;
}

int32_t _FT_PathAppend(FolderSpec **spec, const unsigned char *name)
{
    int32_t err, e;
    ULong size;

    err = 0;
    IndirectGetSize((void **)spec);
    size = IndirectGetSize((void **)spec);
    e = IndirectSetSize((void **)spec, size + name[0] + 1);
    if (e != 0) {
        err = e;
    } else {
        ATMemMove(*(char **)spec + size - 1, name, name[0] + 1);
        (*(unsigned char **)spec)[size + name[0]] = 0;
    }
    return err;
}

int32_t _FT_PathShorten(FolderSpec **spec, char *name)
{
    int32_t err, e;
    int32_t last;
    ULong next;

    err = 0;
    IndirectGetSize((void **)spec);
    next = 0;
    do {
        next = _FT_PathWalk(spec, last = next);
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

ULong _FT_PathWalk(FolderSpec **spec, ULong offset)
{
    int32_t err;

    err = 0;
    IndirectGetSize((void **)spec);
    IndirectGetSize((void **)spec);
    if (offset != 0)
        offset += (*(unsigned char **)spec)[offset] + 1;
    else
        offset = kFirstName;
    IndirectGetSize((void **)spec);
    if ((*(unsigned char **)spec)[offset] == 0) {
        IndirectGetSize((void **)spec);
        offset = 0;
    }
    return offset;
}

int32_t _FT_PathNameInput(unsigned char *out, const char *in, ULong length)
{
    const char *p;
    int32_t err;
    unsigned char c;
    ULong d;
    ULong i;
    ULong n;
    ULong v;

    err = 0;
    n = 0;
    i = 0;
    while (i < length && (char)(c = *(p = in + i)) != 0 && n < 0xff) {
        switch ((char)c) {
        case '/':
        case ':':
        case '\\':
            err = kFTErrBadName;
            goto done;
        case '%':
            d = p[1];
            if (d >= '0' && d <= '9')
                v = (d - '0') << 4;
            else if (d >= 'A' && d <= 'F')
                v = (d - 'A' + 10) << 4;
            else if (d >= 'a' && d <= 'f')
                v = (d - 'a' + 10) << 4;
            else {
                out[++n] = ((const unsigned char *)in)[i];
                i++;
                break;
            }
            d = p[2];
            if (d >= '0' && d <= '9')
                v |= d - '0';
            else if (d >= 'A' && d <= 'F')
                v |= d - 'A' + 10;
            else if (d >= 'a' && d <= 'f')
                v |= d - 'a' + 10;
            else {
                out[++n] = ((const unsigned char *)in)[i];
                i++;
                break;
            }
            if (v == ':') {
                out[++n] = ((const unsigned char *)in)[i];
                i++;
            } else {
                out[++n] = v;
                i += 3;
            }
            break;
        default:
            out[++n] = ((const unsigned char *)in)[i];
            i++;
            break;
        }
    }
done:
    out[0] = err != 0 ? 0 : n;
    return err;
}

int32_t _FT_PathNameOutput(char *out, const unsigned char *name)
{
    char *q;
    const unsigned char *p;
    const char *hex;
    ULong i, o;

    hex = "0123456789ABCDEF";
    i = 1;
    o = 0;
    p = name + 1;
    q = out;
    while (i <= name[0] && o < 0xff) {
        switch (*p) {
        case 0:
        case '\t':
        case '\n':
        case '\r':
        case '%':
        case '/':
        case ':':
        case '\\':
            q[0] = '%';
            q[1] = hex[(*p & 0xf0) >> 4];
            q[2] = hex[*p & 0x0f];
            i++;
            p++;
            o += 3;
            q += 3;
            break;
        default:
            *q = *(const char *)p;
            i++;
            p++;
            o++;
            q++;
            break;
        }
    }
    out[o] = 0;
    return 0;
}

int32_t _FT_FileOpenWrapper(FolderSpec **spec, unsigned char write, FTFile **result)
{
    int32_t err;
    FTFile *file;
    int32_t rw;
    int32_t refCount;
    int32_t e;
    ULong writable;
    FTFileInfo info;
    ULong position;

    err = 0;
    file = 0;
    if (((*spec)->file != 0 ? 1 : 0) != 0) {
        *result = (*spec)->file;
        _FT_FileUpdateRefCount(*result, 1);
    } else {
        file = _FT_IsFileOpen(spec);
        if (file != 0) {
            if (file->mode != 0 || !write) {
                *result = file;
                _FT_FileUpdateRefCount(*result, 1);
            } else {
                rw = 0;
                position = 0;
                refCount = 0;
                e = _FT_FileGetFlags(spec, &info, 0);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                if (write) {
                    writable = info.flags[kFTInfoWritable];
                    if (writable == 0U) {
                        err = kFTErrPermission;
                        goto done;
                    }
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
                    if (e != 0)
                        err = e;
                } else {
                    file->refCount = refCount;
                    e = _FT_FileSetMark(file, position);
                    if (e != 0)
                        err = e;
                }
                if (rw != 0) {
                    err = rw;
                    goto done;
                }
                *result = file;
                _FT_FileUpdateRefCount(*result, 1);
            }
        } else {
            if ((*result = MemoryAllocate(sizeof(FTFile))) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            memset(*result, 0, sizeof(FTFile));
            e = _FT_FileGetFlags(spec, &info, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (write) {
                writable = info.flags[kFTInfoWritable];
                if (writable == 0U) {
                    err = kFTErrPermission;
                    goto done;
                }
            }
            e = _FT_FileOpen(spec, write, *result);
            if (e != 0) {
                err = e;
                goto done;
            }
            (*result)->link = gFT_FileList;
            gFT_FileList.next = *result;
            _FT_FileUpdateRefCount(*result, 1);
        }
    }
done:
    if (err != 0 && *result != 0) {
        MemoryDeallocate(*result);
        *result = 0;
    }
    return err;
}

unsigned char _FT_FileIsChunky(FolderSpec **spec, ULong *count)
{
    int32_t err;
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

unsigned char _FT_IndirectIsChunky(void **handle)
{
    int32_t err;
    const char *sig;
    unsigned char isArchive;
    FTArchiveHeader h;

    err = 0;
    sig = "CHNK";
    isArchive = 0;
    if (IndirectGetSize(handle) >= kArchiveHeaderSize) {
        memmove(&h, *handle, sizeof(h));
        if (IsSignature(h.signature, sig) && h.headerSize >= kArchiveHeaderSize
            && !(IndirectGetSize(handle) < h.entrySize * h.entryCount + h.tableOffset))
            isArchive = 1;
    }
    return isArchive;
}

int32_t _FT_FileUpdateRefCount(FTFile *file, int32_t delta)
{
    int32_t err;

    err = 0;
    file->refCount += delta;
    return file->refCount;
}

#define kExt(a, b, c, d) (((ULong)(a) << 24) | ((ULong)(b) << 16) | \
                          ((ULong)(c) << 8) | (ULong)(d))

#define kPreferred 1
#define kTypeMapCount 19
#define kTypeMapResource 'FMap'

ULong _FT_Suffix2Value(const char *name);

void **_FT_FileTypesCreate(void)
{
    int32_t err;
    void **h;
    Handle prefs;
    unsigned char began;
    ICInstance instance;
    ICAttr attr;
    int32_t pos;
    int32_t count;
    int32_t index;
    int32_t n;
    unsigned char *p;
    FileTypeMap entry;
    ProcessInfoRec info;
    FSSpec appSpec;
    Str255 name;
    Str255 key;
    ICMapEntry mapEntry;

    err = 0;
    {
    FileTypeMap map[kTypeMapCount] = {
        { 'TEXT', 'ttxt', kExt('t', 'x', 't', 0), 1 },
        { 'TEXT', 'ttxt', kExt('t', 'e', 'x', 't'), 0 },
        { 'TEXT', 'CWIE', kExt('c', 0, 0, 0), 0 },
        { 'TEXT', 'CWIE', kExt('c', 'p', 0, 0), 0 },
        { 'TEXT', 'CWIE', kExt('c', 'p', 'p', 0), 0 },
        { 'TEXT', 'CWIE', kExt('h', 0, 0, 0), 0 },
        { 'TEXT', 'CWIE', kExt('h', 'p', 0, 0), 0 },
        { 'TEXT', 'CWIE', kExt('h', 'p', 'p', 0), 0 },
        { 'PNGf', 'ogle', kExt('p', 'n', 'g', 0), 1 },
        { 'PNG ', 'ogle', kExt('p', 'n', 'g', 0), 0 },
        { 'GIFf', 'ogle', kExt('g', 'i', 'f', 0), 1 },
        { 'GIF ', 'ogle', kExt('g', 'i', 'f', 0), 0 },
        { 'JPEG', 'ogle', kExt('j', 'p', 'g', 0), 1 },
        { 'JPEG', 'ogle', kExt('j', 'p', 'e', 'g'), 0 },
        { 'BMPf', 'ogle', kExt('b', 'm', 'p', 0), 1 },
        { 'BMP ', 'ogle', kExt('b', 'm', 'p', 0), 0 },
        { 'PICT', 'ttxt', kExt('p', 'c', 't', 0), 1 },
        { 'PICT', 'ttxt', kExt('p', 'i', 'c', 't'), 0 },
        { '????', '????', 0, 0 }
    };

    h = IndirectAllocate(sizeof(map));
    if (h == 0) {
        err = kOutOfMemory;
        goto done;
    }
    BlockMoveData(map, *h, sizeof(map));
    }
    {
    ProcessSerialNumber process = { 0, kCurrentProcess };

    instance = 0;
    began = 0;
    prefs = NewHandle(0);
    if (prefs == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(&info, sizeof(ProcessInfoRec));
    info.processInfoLength = sizeof(ProcessInfoRec);
    info.processName = name;
    info.processAppSpec = &appSpec;
    if (GetProcessInformation(&process, &info) != 0)
        info.processSignature = 'CWIE';
    }
#if !TARGET_API_MAC_CARBON
    if (ICStart != (void *)kUnresolvedCFragSymbolAddress)
#endif
    {
        err = ICStart(&instance, info.processSignature);
        if (err != 0) {
            err = 0;
            goto closed;
        }
#if !TARGET_API_MAC_CARBON
        err = ICFindConfigFile(instance, 0, 0);
        if (err != 0) {
            err = 0;
            goto closed;
        }
#endif
        err = ICBegin(instance, icReadOnlyPerm);
        if (err != 0) {
            err = 0;
            goto closed;
        }
        BlockMoveData(kICMapping, key, kICMapping[0] + 1);
        began = 1;
        err = ICFindPrefHandle(instance, key, &attr, prefs);
        if (err != 0) {
            err = 0;
            goto closed;
        }
        if (ICCountMapEntries(instance, prefs, &count) == 0) {
            index = 1;
            pos = 0;
            for (; index <= count && err == 0; index++) {
                if (ICGetIndMapEntry(instance, prefs, index, &pos, &mapEntry) == 0) {
                    entry.type = mapEntry.fileType;
                    entry.creator = mapEntry.fileCreator;
                    p = mapEntry.extension;
                    n = mapEntry.extension[0] + 1;
                    while (--n != 0) {
                        *p = p[1];
                        p++;
                    }
                    *p = 0;
                    entry.extension = _FT_Suffix2Value((char *)mapEntry.extension);
                    entry.preferred = 1;
                    err = IndirectInsertData(h, &entry, sizeof(FileTypeMap));
                }
            }
        }
    }
closed:
    if (instance != 0) {
        if (began)
            ICEnd(instance);
        ICStop(instance);
        instance = 0;
    }
    if (prefs != 0)
        DisposeHandle(prefs);
    if (err == 0) {
        _AtomicEnter();
        began = 0;
        prefs = Get1Resource(kTypeMapResource, 0);
        if (prefs != 0) {
            HLock(prefs);
            if ((GetHandleSize(prefs) & 0xf) == 0)
                err = IndirectInsertData(h, *prefs, GetHandleSize(prefs));
            ReleaseResource(prefs);
        }
        _AtomicLeave();
    }
done:
    if (err != 0) {
        IndirectDeallocate(h);
        h = 0;
    }
    return h;
}

void _FT_FileTypesLookup(const char *name, ULong type, FileTypeMap *entry)
{
    ULong extension;
    ULong i;
    ULong count;

    if (name != 0 && !IsSpecID(name))
        _FT_PathLookup((FolderSpec **)name);
    IndirectGetSize((void **)gFT_FileTypes);
    extension = _FT_Suffix2Value(name);
    if (extension != 0) {
        count = IndirectGetSize((void **)gFT_FileTypes) / sizeof(FileTypeMap);
        for (i = 0; i < count; i++)
            if (extension == (*gFT_FileTypes)[i].extension && ((*gFT_FileTypes)[i].preferred & kPreferred))
                break;
        if (i == count)
            for (i = 0; i < count; i++)
                if (extension == (*gFT_FileTypes)[i].extension)
                    break;
        if (i == count)
            i = count - 1;
    } else {
        count = IndirectGetSize((void **)gFT_FileTypes) / sizeof(FileTypeMap);
        for (i = 0; i < count; i++)
            if (type == (*gFT_FileTypes)[i].type && ((*gFT_FileTypes)[i].preferred & kPreferred))
                break;
        if (i == count)
            for (i = 0; i < count; i++)
                if (type == (*gFT_FileTypes)[i].type)
                    break;
        if (i == count)
            i = count - 1;
    }
    *entry = (*gFT_FileTypes)[i];
}

ULong _FT_Suffix2Value(const char *name)
{
    int32_t err;
    int32_t upper;
    ULong code;
    const char *s;
    ULong i;

    err = 0;
    code = 0;
    s = 0;
    if (name != 0 && !IsSpecID(name))
        _FT_PathLookup((FolderSpec **)name);
    if (name != 0 && name[0] != 0) {
        s = name;
        while ((s = StringFindDelimiter(s, ".")) != 0) {
            name = s;
            s++;
        }
        if (*name == '.')
            name++;
        for (i = 0; i < 4 && name[i] != 0; i++) {
            upper = name[i] >= 'A' && name[i] <= 'Z';
            ((char *)&code)[3 - i] = name[i] + (upper ? 0x20 : 0);
        }
    }
    return code;
}

unsigned char _FT_IsIndirectBusy(void *handle)
{
    return _FT_IsBufferOpen(handle) != 0;
}
