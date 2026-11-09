/* atfilearchivemac.c -- 1.0.10: the file tool: files of files. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atfiletoolmac.h"
#define ATSTR_CMP
#define ATSTR_MOVE
#include "atfilestr.h"

static char kPathBreaks[] = "/:\\%";
static char kNone[] = "";
static char kNameFormat[] = "%s ";
static char kFolderWord[] = "folder";
static char kFileWord[] = "file";
static char kSelectPrompt[] = "Select a %s: ";
static char kLineEnd[] = "\t\r\n";
static char kNoSuchMessage[] = "There is no such %s.\n\n";
static char kNotAMessage[] = "That is not a %s!\n\n";
static char kNamePrompt[] = "Enter a file name: ";
static char kBlanks[] = " \t\r\n";
static char kYesNoPrompt[] = "[y/N] : ";
static char kReplacePrompt[] = "A %s with that name already exists. Replace it? [y/N] : ";

long _FT_ChunkFilePreflight(FTFile *file, FTMark *mark)
{
    mark->file = file;
    mark->position = file->position;
    do
        file = file->parent;
    while (file != 0);
    return 0;
}

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

FTFile *_FT_IsChunkOpen(FTFile *archive, const char *name)
{
    long err;
    FTLink *link;
    FTFile *f;
    ULong index;

    err = 0;
    link = 0;
    if (!_FT_ChunkLookup(archive, name, &index, 0)) {
        err = kFTErrFileNotFound;
        goto done;
    }
    link = &gFT_FileList;
    while ((f = link->next) != 0) {
        if (f->parent == archive && f->origin == index)
            break;
        link = &f->link;
    }
done:
    return link != 0 ? link->next : 0;
}

long _FT_ChunkOpenPath(FolderSpec **spec, unsigned char write, unsigned char toLast,
                  unsigned char mustBeArchive, FTFile **result, char *leaf)
{
    long err, e;
    unsigned char *out;
    FTFile *parent;
    ULong offset, limit;
    unsigned char *name;
    long saved;
    FTFile *root;
    FTFile *file;
    unsigned char locked;
    char cname[256];
    ULong index;

    err = 0;
    locked = 0;
    saved = -1;
    parent = 0;
    root = 0;
    *result = 0;
    if (leaf != 0)
        leaf[0] = 0;
    locked = IndirectSetLock((void **)spec, 1);
    e = _FT_FileOpenWrapper(spec, write, &root);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (root->buffer == 0) {
        if (mustBeArchive && !_FT_FileIsChunky(spec, 0)) {
            err = kFTErrBadEntry;
            goto done;
        }
        e = _FT_ChunkLoad(root);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (write && root->mode == 0) {
        err = kFTErrPermission;
        goto done;
    }
    e = root->procs.proc[kFTPosition](root, &saved);
    if (e != 0) {
        err = e;
        goto done;
    }
    limit = IndirectGetSize((void **)spec) - 2;
    *result = root;
    offset = 0;
    out = (unsigned char *)cname - 1;
    while ((offset = _FT_PathWalk(spec, offset)) != 0) {
        name = (unsigned char *)*spec + offset;
        {
            const unsigned char *p;
            unsigned char *q;
            ULong n;

            n = name[0] + 1;
            q = out;
            p = name;
            while (--n != 0)
                *++q = *++p;
            q[1] = 0;
        }
        parent = *result;
        *result = 0;
        if (toLast && offset + name[0] >= limit) {
            if (leaf != 0)
                _FT_PathNameOutput(leaf, name);
            *result = parent;
            goto done;
        }
        if ((*result = _FT_IsChunkOpen(parent, cname)) != 0) {
            _FT_FileUpdateRefCount(*result, 1);
        } else {
            if ((*result = MemoryAllocate(sizeof(FTFile))) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            memset(*result, 0, sizeof(FTFile));
            if (!_FT_ChunkLookup(parent, cname, &index, 0)) {
                err = kFTErrFileNotFound;
                goto done;
            }
            err = _FT_ChunkOpen(parent, *result, cname, write,
                               ArchiveEntry(parent, index)->flags & (1 << kFTInfoFolder));
            if (err != 0) {
                if (_FT_PathWalk(spec, offset) != 0)
                    err = kFTErrDirNotFound;
                goto done;
            }
            (file = *result)->link = gFT_FileList;
            gFT_FileList.next = *result;
            /* BUG: the reference the previous step took on the parent is never dropped */
            _FT_FileUpdateRefCount(*result, 1);
        }
    }
done:
    if (root != 0 && (ULong)saved != 0xffffffff)
        root->procs.proc[kFTSeek](root, saved);
    IndirectSetLock((void **)spec, locked);
    if (err != 0) {
        if (*result != 0) {
            MemoryDeallocate(*result);
            *result = 0;
        }
        if (parent != 0)
            FT_FileClose(parent);
        else if (root != 0)
            FT_FileClose(root);
    }
    return err;
}

long _FT_ChunkOpen(FTFile *archive, FTFile *file, const char *name, unsigned char write,
                  unsigned char nested)
{
    long err, result;
    ULong index, flags;

    err = 0;
    if (!_FT_ChunkLookup(archive, name, &index, 0)) {
        err = kFTErrFileNotFound;
        goto done;
    }
    flags = ATSwap32(ArchiveEntry(archive, index)->flags);
    if (!AtomicMaskTest(&flags, kFTInfoReadable)) {
        err = kFTErrPermission;
        goto done;
    }
    if (write && archive->mode == 0) {
        err = kFTErrPermission;
        goto done;
    }
    if (write && !AtomicMaskTest(&flags, kFTInfoWritable)) {
        err = kFTErrPermission;
        goto done;
    }
    file->size = ATSwap32(ArchiveEntry(archive, index)->length);
    file->position = 0;
    file->mode = archive->mode;
    file->procs = *(FTProcs *)gFT_ChunkHooks;
    file->parent = archive;
    file->origin = index;
    /* BUG: checks the just-zeroed position, not the entry's offset, against the archive size */
    if (archive->size < file->position) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (archive->size < file->position + file->size) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (nested) {
        _FT_FileUpdateRefCount(file, 1);
        result = _FT_ChunkLoad(file);
        if (result != 0)
            err = result;
        else
            _FT_FileUpdateRefCount(file, -1);
    }
done:
    if (err != 0)
        _FT_FileUpdateRefCount(file, -file->refCount);
    return err;
}

long _FT_ChunkClose(FTFile *file)
{
    long err;

    err = 0;
    if (file->buffer != 0) {
        IndirectDeallocate(file->buffer);
        file->buffer = 0;
    }
    if (file->buffer2 != 0) {
        IndirectDeallocate(file->buffer2);
        file->buffer2 = 0;
    }
    return 0;
}

long _FT_ChunkLoad(FTFile *file)
{
    const char *sig;
    long err, e;
    FTMark mark;
    FTArchiveHeader h;
    ULong table;
    ULong fileSize;
    ULong size;

    sig = "CHNK";
    err = 0;
    e = _FT_ChunkFilePreflight(file, &mark);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (file->size < kArchiveHeaderSize) {
        err = kFTErrBadEntry;
        goto done;
    }
    e = file->procs.proc[kFTSeek](file, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = file->procs.proc[kFTRead](file, kArchiveHeaderSize, &h);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (h.signature[0] != sig[0]) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (h.signature[1] != sig[1]) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (h.signature[2] != sig[2]) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (h.signature[3] != sig[3]) {
        err = kFTErrBadEntry;
        goto done;
    }
    if ((size = h.headerSize) < kArchiveHeaderSize) {
        err = kFTErrBadEntry;
        goto done;
    }
    if ((fileSize = file->size) < (table = h.tableOffset)) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (fileSize < table + h.entrySize * h.entryCount) {
        err = kFTErrBadEntry;
        goto done;
    }
    if ((file->buffer = IndirectAllocate(size)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    IndirectSetLock(file->buffer, 1);
    e = file->procs.proc[kFTSeek](file, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = file->procs.proc[kFTRead](file, h.headerSize, *file->buffer);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectSetLock(file->buffer, 0);
    if ((file->buffer2 = IndirectAllocate(h.entrySize * h.entryCount)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    IndirectSetLock(file->buffer2, 1);
    e = file->procs.proc[kFTSeek](file, h.tableOffset);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = file->procs.proc[kFTRead](file, h.entrySize * h.entryCount, *file->buffer2);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectSetLock(file->buffer2, 0);
done:
    if (err != 0) {
        if (file->buffer != 0) {
            IndirectDeallocate(file->buffer);
            file->buffer = 0;
        }
        if (file->buffer2 != 0) {
            IndirectDeallocate(file->buffer2);
            file->buffer2 = 0;
        }
    }
    if (err != 0) {
        _FT_ChunkFilePostflight(file, &mark);
    } else {
        e = _FT_ChunkFilePostflight(file, &mark);
        if (e != 0)
            err = e;
    }
    return err;
}

#if TARGET_API_MAC_CARBON
long _FT_ChunkDirEntry(FTFile *archive, ULong index, FTFileInfo *info, FolderSpec **spec)
{
    long err, e;
    FTArchiveEntry *entry;
    FTArchiveHeader *header;
    ULong count;
    ULong i;
    unsigned char locked, locked2;

    err = 0;
    entry = 0;
    if (info != 0)
        memset(info, 0, sizeof(FTFileInfo));
    locked = IndirectSetLock(archive->buffer, 1);
    locked2 = IndirectSetLock(archive->buffer2, 1);
    header = (FTArchiveHeader *)*archive->buffer;
    count = header->entryCount;
    index++;
    for (i = 0; i < count; i++) {
        entry = (FTArchiveEntry *)((char *)*archive->buffer2 + i * header->entrySize);
        if (entry->name[0] != 0 && --index == 0)
            break;
    }
    if (index > 1) {
        err = kFTErrNoMoreEntries;
        goto done;
    }
    if (index == 1) {
        err = kFTErrNoMoreEntries;
        goto done;
    }
    if (info != 0) {
        e = _FT_ChunkGetFlags(archive, 0, entry->name, info, spec);
        if (e != 0)
            err = e;
    }
done:
    IndirectSetLock(archive->buffer, locked);
    IndirectSetLock(archive->buffer2, locked2);
    return err;
}
#endif

unsigned char _FT_ChunkLookup(FTFile *archive, const char *name, ULong *index,
                           FTArchiveEntry *entry)
{
    FTArchiveHeader *header;
    ULong i;
    unsigned char locked, locked2;
    FTArchiveEntry *e;
    unsigned char found;
    ULong count;

    found = 0;
    if (index != 0)
        *index = 0xffffffff;
    if (entry != 0)
        memset(entry, 0, sizeof(FTArchiveEntry));
    locked = IndirectSetLock(archive->buffer, 1);
    locked2 = IndirectSetLock(archive->buffer2, 1);
    header = (FTArchiveHeader *)*archive->buffer;
    count = header->entryCount;
    for (i = 0; i < count; i++) {
        e = (FTArchiveEntry *)((char *)*archive->buffer2 + i * header->entrySize);
        if (FTStrCmp(e->name, name) == 0) {
            if (entry != 0)
                memmove(entry, e, sizeof(FTArchiveEntry));
            if (index != 0)
                *index = i;
            found = 1;
            break;
        }
    }
    IndirectSetLock(archive->buffer, locked);
    IndirectSetLock(archive->buffer2, locked2);
    return found;
}

long _FT_ChunkFormat(FTFile *file, ULong entries)
{
    ULong size;
    long err, e;
    char *table;
    FTArchiveHeader h;
    FTMark mark;

    err = 0;
    table = 0;
    e = _FT_ChunkFilePreflight(file, &mark);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (file->size != 0) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (entries == 0 || entries > kMaxEntries)
        entries = kDefaultEntries;
    memset(&h, 0, sizeof(h));
    memmove(h.signature, "CHNK", sizeof(h.signature));
    h.headerSize = ATSwap32(kArchiveHeaderSize);
    h.tableOffset = ATSwap32(kArchiveHeaderSize);
    h.entrySize = ATSwap32(kArchiveEntrySize);
    h.entryCount = ATSwap32(entries);
    size = entries * kArchiveEntrySize;
    table = MemoryAllocate(size);
    if (table == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(table, 0, size);
    e = file->procs.proc[kFTSetSize](file, size + kArchiveHeaderSize);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = file->procs.proc[kFTWrite](file, kArchiveHeaderSize, &h);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = file->procs.proc[kFTWrite](file, size, table);
    if (e != 0)
        err = e;
done:
    if (table != 0)
        MemoryDeallocate(table);
    if (err != 0) {
        _FT_ChunkFilePostflight(file, &mark);
    } else {
        e = _FT_ChunkFilePostflight(file, &mark);
        if (e != 0)
            err = e;
    }
    return err;
}

long _FT_ChunkResize(FTFile *archive, ULong offset, ULong length, ULong newLength,
                  ULong *newOffset)
{
    FTMark mark;
    char *buffer;
    ULong end;
    ULong done;
    long err;
    ULong chunk;
    long result;
    long e;
    ULong pos;

    err = 0;
    buffer = 0;
    *newOffset = newLength != 0 ? offset : 0;
    result = _FT_ChunkFilePreflight(archive, &mark);
    if (result != 0) {
        err = result;
        goto done;
    }
    if (offset == 0 && length != 0) {
        err = kFTErrBadEntry;
        goto done;
    }
    if (newLength == length)
        goto done;
    if ((end = archive->size) == offset + length) {
        e = archive->procs.proc[kFTSetSize](archive, offset + newLength);
        if (e != 0) {
            err = e;
            goto done;
        }
        goto done;
    }
    if (newLength <= length)
        goto done;

    pos = end;
    chunk = 0;
    done = 0;
    result = archive->procs.proc[kFTSetSize](archive, pos + newLength);
    if (result != 0) {
        err = result;
        goto done;
    }
    chunk = newLength < kCopyChunk ? newLength : kCopyChunk;
    buffer = MemoryAllocate(chunk);
    if (buffer == 0) {
        err = kOutOfMemory;
        goto done;
    }
    while (done < length) {
        if (chunk > length - done)
            chunk = length - done;
        result = archive->procs.proc[kFTSeek](archive, offset + done);
        if (result != 0) {
            err = result;
            goto done;
        }
        result = archive->procs.proc[kFTRead](archive, chunk, buffer);
        if (result != 0) {
            err = result;
            goto done;
        }
        result = archive->procs.proc[kFTSeek](archive, end + done);
        if (result != 0) {
            err = result;
            goto done;
        }
        result = archive->procs.proc[kFTWrite](archive, chunk, buffer);
        if (result != 0) {
            err = result;
            goto done;
        }
        done += chunk;
    }
    chunk = newLength < kCopyChunk ? newLength : kCopyChunk;
    memset(buffer, 0, chunk);
    while (done < newLength) {
        if (chunk > newLength - done)
            chunk = newLength - done;
        result = archive->procs.proc[kFTSeek](archive, end + done);
        if (result != 0) {
            err = result;
            goto done;
        }
        result = archive->procs.proc[kFTWrite](archive, chunk, buffer);
        if (result != 0) {
            err = result;
            goto done;
        }
        done += chunk;
    }
    *newOffset = archive->size - newLength;
done:
    if (buffer != 0)
        MemoryDeallocate(buffer);
    if (err != 0) {
        _FT_ChunkFilePostflight(archive, &mark);
    } else {
        result = _FT_ChunkFilePostflight(archive, &mark);
        if (result != 0)
            err = result;
    }
    return err;
}

long _FT_ChunkCreate(FTFile *archive, FTFileInfo *info)
{
    long err, e;
    ULong entries;
    FTArchiveEntry *slot;
    FTArchiveEntry *entry;
    FTFile *dir;
    ULong done;
    ULong total;
    ULong chunk;
    char *zeros;
    FTFile *p;
    unsigned char name[256];
    FTMark mark;
    FTArchiveHeader h;
    ULong index;
    FTArchiveHeader old;
    ULong n;
    ULong size;
    unsigned char *s;

    err = 0;
    entries = kDefaultEntries;
    slot = 0;
    dir = 0;
    e = _FT_PathNameInput(name, info->name, 0xff);
    if (e != 0) {
        err = e;
        goto done;
    }
    s = name;
    n = *(unsigned char *)&name + 1;
    while (--n != 0) {
        *s = s[1];
        s++;
    }
    *s = 0;
    IndirectSetLock(archive->buffer, 1);
    IndirectSetLock(archive->buffer2, 1);
    memmove(&h, *archive->buffer, sizeof(h));
    h.headerSize = ATSwap32(h.headerSize);
    h.tableOffset = ATSwap32(h.tableOffset);
    h.entrySize = ATSwap32(h.entrySize);
    h.entryCount = ATSwap32(h.entryCount);
    e = _FT_ChunkFilePreflight(archive, &mark);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (_FT_ChunkLookup(archive, (char *)name, 0, 0)) {
        err = kFTErrExists;
        goto done;
    }
    if (_FT_ChunkLookup(archive, "", &index, 0)) {
        slot = ArchiveEntry(archive, index);
    } else {
        size = 0;
        done = 0;
        total = 0;
        chunk = 0;
        zeros = 0;
        old = h;
        e = _FT_ChunkResize(archive, h.tableOffset, h.entrySize * h.entryCount,
                         h.entrySize * h.entryCount * 2, &h.tableOffset);
        if (e != 0) {
            err = e;
            goto done;
        }
        ((FTArchiveHeader *)*archive->buffer)->tableOffset = ATSwap32(h.tableOffset);
        e = archive->procs.proc[kFTSeek](archive, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = archive->procs.proc[kFTWrite](archive, IndirectGetSize(archive->buffer), *archive->buffer);
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(archive->buffer2, 0);
        e = IndirectSetSize(archive->buffer2, h.entrySize * h.entryCount * 2);
        if (e != 0) {
            err = e;
            goto done;
        }
        size = h.entrySize * h.entryCount;
        entry = ArchiveEntry(archive, h.entryCount);
        memset(entry, 0, size);
        IndirectSetLock(archive->buffer2, 1);
        slot = ArchiveEntry(archive, h.entryCount);
        e = archive->procs.proc[kFTSeek](archive, h.tableOffset);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = archive->procs.proc[kFTWrite](archive, IndirectGetSize(archive->buffer2),
                                     *archive->buffer2);
        if (e != 0) {
            err = e;
            goto done;
        }
        h.entryCount *= 2;
        ((FTArchiveHeader *)*archive->buffer)->entryCount = ATSwap32(h.entryCount);
        e = archive->procs.proc[kFTSeek](archive, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = archive->procs.proc[kFTWrite](archive, IndirectGetSize(archive->buffer), *archive->buffer);
        if (e != 0) {
            err = e;
            goto done;
        }
        total = old.entryCount * old.entrySize;
        chunk = total < kCopyChunk ? total : kCopyChunk;
        /* BUG: the zero buffer is never freed */
        zeros = MemoryAllocate(chunk);
        if (zeros == 0) {
            err = kOutOfMemory;
            goto done;
        }
        memset(zeros, 0, chunk);
        for (; done < total; done += chunk) {
            if (chunk > total - done)
                chunk = total - done;
            e = archive->procs.proc[kFTSeek](archive, old.tableOffset + done);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = archive->procs.proc[kFTWrite](archive, chunk, zeros);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    StringCopySafe(slot->name, (char *)name, sizeof(slot->name));
    slot->flags = ATSwap32((info->flags[kFTInfoReadable] ? 1UL << kFTInfoReadable : 0)
                         | (info->flags[kFTInfoWritable] ? 1UL << kFTInfoWritable : 0)
                         | (info->flags[kFTInfoFolder] ? 1UL << kFTInfoFolder : 0)
                         | (info->flags[kFTInfoHidden] ? 1UL << kFTInfoHidden : 0));
    slot->offset = 0;
    slot->length = 0;
    slot->type = ATSwap32(info->type);
    e = archive->procs.proc[kFTSeek](archive,
                                (char *)slot + h.tableOffset - (char *)*archive->buffer2);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = archive->procs.proc[kFTWrite](archive, h.entrySize, slot);
    if (e != 0) {
        err = e;
        goto done;
    }
    slot = 0;
    if (!info->flags[kFTInfoFolder])
        goto done;
    if (info->size != 0 && info->size <= kMaxEntries)
        entries = info->size;
    if ((dir = MemoryAllocate(sizeof(FTFile))) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(dir, 0, sizeof(FTFile));
    e = _FT_ChunkOpen(archive, dir, (char *)name, 1, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    for (p = dir; (p = p->parent) != 0; )
        _FT_FileUpdateRefCount(p, 1);
    dir->link = gFT_FileList;
    gFT_FileList.next = dir;
    _FT_FileUpdateRefCount(dir, 1);
    e = _FT_ChunkFormat(dir, entries);
    if (e != 0)
        err = e;
done:
    if (dir != 0)
        FT_FileClose(dir);
    if (slot != 0)
        memset(slot, 0, sizeof(FTArchiveEntry));
    IndirectSetLock(archive->buffer, 0);
    IndirectSetLock(archive->buffer2, 0);
    if (err != 0) {
        /* BUG: after an early failure the mark was never taken */
        _FT_ChunkFilePostflight(archive, &mark);
    } else {
        e = _FT_ChunkFilePostflight(archive, &mark);
        if (e != 0)
            err = e;
    }
    return err;
}

long _FT_ChunkDelete(FTFile *archive, const char *name)
{
    long err, e;
    unsigned char member[256];
    FTArchiveHeader h;
    FTMark mark;
    ULong offset;
    ULong index;
    FTArchiveEntry *entry;
    ULong length;
    ULong n;
    ULong size;
    unsigned char *p;

    err = 0;
    entry = 0;
    e = _FT_PathNameInput(member, name, 0xff);
    if (e != 0) {
        err = e;
        goto done;
    }
    p = member;
    n = *(unsigned char *)&member + 1;
    while (--n != 0) {
        *p = p[1];
        p++;
    }
    *p = 0;
    IndirectSetLock(archive->buffer, 1);
    IndirectSetLock(archive->buffer2, 1);
    memmove(&h, *archive->buffer, sizeof(h));
    e = _FT_ChunkFilePreflight(archive, &mark);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (_FT_IsChunkOpen(archive, (char *)member) != 0) {
        err = kFTErrBusy;
        goto done;
    }
    if (!_FT_ChunkLookup(archive, (char *)member, &index, 0)) {
        err = kFTErrFileNotFound;
        goto done;
    }
    entry = ArchiveEntry(archive, index);
    offset = ATSwap32(entry->offset);
    length = ATSwap32(entry->length);
    size = h.entrySize;
    memset(entry, 0, size);
    e = archive->procs.proc[kFTSeek](archive, index * h.entrySize + h.tableOffset);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = archive->procs.proc[kFTWrite](archive, h.entrySize, entry);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _FT_ChunkResize(archive, offset, length, 0, &offset);
    if (e != 0)
        err = e;
done:
    IndirectSetLock(archive->buffer, 0);
    IndirectSetLock(archive->buffer2, 0);
    if (err != 0) {
        _FT_ChunkFilePostflight(archive, &mark);
    } else {
        e = _FT_ChunkFilePostflight(archive, &mark);
        if (e != 0)
            err = e;
    }
    return err;
}

long _FT_ChunkGetFlags(FTFile *archive, const char *name, const char *member, FTFileInfo *info,
                  FolderSpec **spec)
{
    ULong *flags;
    long err, e;
    unsigned char pname[256];
    char cname[256];
    FileTypeMap map;
    FTArchiveHeader h;
    FTArchiveEntry entry;

    err = 0;
    if (info != 0)
        memset(info, 0, sizeof(FTFileInfo));
    if (name != 0) {
        e = _FT_PathNameInput(pname, name, 0xff);
        if (e != 0) {
            err = e;
            goto done;
        }
        member = FTPToCStr(cname, pname);
    } else
    {
        const unsigned char *p;
        unsigned char *q;
        ULong n;
        unsigned char c;

        n = (ULong)-1;
        q = pname;
        p = (const unsigned char *)member - 1;
        while (++n < 0xff && (c = *++p) != 0)
            *++q = c;
        pname[0] = n;
    }
    /* BUG: both handles are locked here and never unlocked */
    IndirectSetLock(archive->buffer, 1);
    IndirectSetLock(archive->buffer2, 1);
    memmove(&h, *archive->buffer, sizeof(h));
    if (!_FT_ChunkLookup(archive, member, 0, &entry)) {
        if (info == 0) {
            err = kFTErrFileNotFound;
            goto done;
        }
        err = kFTErrFileNotFound;
        goto done;
    }
    flags = &entry.flags;
    if (info != 0) {
        _FT_PathNameOutput(info->name, pname);
        info->flags[kFTInfoReadable] = AtomicMaskTest(flags, kFTInfoReadable);
        info->flags[kFTInfoWritable] = AtomicMaskTest(flags, kFTInfoWritable);
        info->flags[kFTInfoFolder] = AtomicMaskTest(flags, kFTInfoFolder);
        info->flags[kFTInfoHidden] = AtomicMaskTest(flags, kFTInfoHidden);
        info->flags[kFTInfoArchive] = AtomicMaskTest(flags, kFTInfoFolder);
        if (((*spec)->file != 0 ? 1 : 0) == 0) {
            e = _FT_FileGetDates(spec, &info->created, &info->modified);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else
            info->created = info->modified = 0;
        if (info->flags[kFTInfoFolder]) {
            e = _FT_ChunkGetCount(archive, member, &info->size);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            info->size = entry.length;
            info->type = entry.type;
            _FT_FileTypesLookup(info->name, entry.type, &map);
            info->creator = map.creator;
        }
    }
done:
    return err;
}

long _FT_ChunkSetFlags(FTFile *archive, const char *name, FTFileInfo *info)
{
    long err, e;
    unsigned char *p;
    ULong n;
    ULong index;
    FTMark mark;
    FileTypeMap map;
    FTArchiveHeader h;
    FTArchiveEntry entry;
    unsigned char member[256];
    unsigned char newName[256];

    err = 0;
    /* BUG: clears the very flags it is asked to set */
    memset(info, 0, sizeof(FTFileInfo));
    e = _FT_PathNameInput(member, name, 0xff);
    if (e != 0) {
        err = e;
        goto done;
    }
    p = member;
    n = member[0] + 1;
    while (--n != 0) {
        *p = p[1];
        p++;
    }
    *p = 0;
    e = _FT_PathNameInput(newName, info->name, 0xff);
    if (e != 0) {
        err = e;
        goto done;
    }
    p = newName;
    n = newName[0] + 1;
    while (--n != 0) {
        *p = p[1];
        p++;
    }
    *p = 0;
    IndirectSetLock(archive->buffer, 1);
    IndirectSetLock(archive->buffer2, 1);
    memmove(&h, *archive->buffer, sizeof(h));
    e = _FT_ChunkFilePreflight(archive, &mark);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (!_FT_ChunkLookup(archive, (char *)member, &index, &entry)) {
        err = kFTErrFileNotFound;
        goto done;
    }
    if (FTStrCmp((char *)member, (char *)newName) != 0 && _FT_ChunkLookup(archive, (char *)newName, 0, 0)) {
        err = kFTErrExists;
        goto done;
    }
    FTStrCpy(entry.name, (char *)newName);
    entry.flags = (info->flags[kFTInfoReadable] ? 1UL << kFTInfoReadable : 0)
                | (info->flags[kFTInfoWritable] ? 1UL << kFTInfoWritable : 0)
                | (info->flags[kFTInfoHidden] ? 1UL << kFTInfoHidden : 0)
                | (entry.flags & (1UL << kFTInfoFolder));
    if ((entry.flags & (1UL << kFTInfoFolder)) == 0) {
        if (info->type != 0) {
            entry.type = info->type;
        } else {
            _FT_FileTypesLookup(info->name, 0, &map);
            entry.type = map.type;
        }
    }
    e = archive->procs.proc[kFTSeek](archive, h.tableOffset + index * h.entrySize);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = archive->procs.proc[kFTWrite](archive, h.entrySize, &entry);
    if (e != 0) {
        err = e;
        goto done;
    }
    ATMemMove(ArchiveEntry(archive, index), &entry, h.entrySize);
done:
    if (err != 0) {
        _FT_ChunkFilePostflight(archive, &mark);
    } else {
        e = _FT_ChunkFilePostflight(archive, &mark);
        if (e != 0)
            err = e;
    }
    return err;
}

long _FT_ChunkRead(FTFile *file, ULong count, void *buffer)
{
    FTMark mark;
    long err;
    FTFile *parent;
    long result;

    err = 0;
    parent = file->parent;
    result = _FT_ChunkFilePreflight(parent, &mark);
    if (result != 0) {
        err = result;
        goto done;
    }
    if (file->position + count > file->size) {
        err = kFTErrPosition;
        goto done;
    }
    result = parent->procs.proc[kFTSeek](parent,
        (ULong)(file->position + ATSwap32(ArchiveEntry(parent, file->origin)->offset)));
    if (result != 0) {
        err = result;
        goto done;
    }
    result = parent->procs.proc[kFTRead](parent, count, buffer);
    if (result != 0) {
        err = result;
    } else {
        file->position += count;
    }
done:
    if (err != 0) {
        _FT_ChunkFilePostflight(parent, &mark);
    } else {
        result = _FT_ChunkFilePostflight(parent, &mark);
        if (result != 0)
            err = result;
    }
    return err;
}

long _FT_ChunkWrite(FTFile *file, ULong count, const void *buffer)
{
    FTMark mark;
    long err;
    FTFile *parent;
    long result;

    err = 0;
    parent = file->parent;
    result = _FT_ChunkFilePreflight(parent, &mark);
    if (result != 0) {
        err = result;
        goto done;
    }
    if (file->mode == 0) {
        err = kFTErrPermission;
        goto done;
    }
    if (file->position + count > file->size) {
        result = _FT_ChunkSetEOF(file, file->position + count);
        if (result != 0) {
            err = result;
            goto done;
        }
    }
    result = parent->procs.proc[kFTSeek](parent,
        (ULong)(file->position + ATSwap32(ArchiveEntry(parent, file->origin)->offset)));
    if (result != 0) {
        err = result;
        goto done;
    }
    result = parent->procs.proc[kFTWrite](parent, count, buffer);
    if (result != 0) {
        err = result;
    } else {
        file->position += count;
        if (file->size < file->position)
            file->size = file->position;
    }
done:
    if (err != 0) {
        _FT_ChunkFilePostflight(parent, &mark);
    } else {
        result = _FT_ChunkFilePostflight(parent, &mark);
        if (result != 0)
            err = result;
    }
    return err;
}

long _FT_ChunkGetEOF(FTFile *file, ULong *size)
{
    long err;
    FTFile *parent;

    err = 0;
    parent = file->parent;
    *size = file->size;
    return err;
}

long _FT_ChunkSetEOF(FTFile *file, ULong size)
{
    FTArchiveHeader header;
    FTMark mark;
    long err;
    FTFile *parent;
    FTArchiveEntry *entry;
    ULong offset;
    ULong length;
    long result;

    err = 0;
    parent = file->parent;
    entry = 0;
    IndirectSetLock(parent->buffer, 1);
    IndirectSetLock(parent->buffer2, 1);
    memmove(&header, *parent->buffer, sizeof(header));
    result = _FT_ChunkFilePreflight(parent, &mark);
    if (result != 0) {
        err = result;
        goto done;
    }
    entry = (FTArchiveEntry *)((char *)*parent->buffer2
            + file->origin * ATSwap32(((FTArchiveHeader *)*parent->buffer)->entrySize));
    offset = ATSwap32(entry->offset);
    length = ATSwap32(entry->length);
    result = _FT_ChunkResize(parent, offset, length, size, &offset);
    if (result != 0) {
        err = result;
        goto done;
    }
    entry->offset = ATSwap32(offset);
    entry->length = ATSwap32(size);
    result = parent->procs.proc[kFTSeek](parent,
                                    file->origin * header.entrySize + header.tableOffset);
    if (result != 0) {
        err = result;
        goto done;
    }
    result = parent->procs.proc[kFTWrite](parent, header.entrySize, entry);
    if (result != 0) {
        err = result;
    } else {
        file->size = size;
        if (file->position > size)
            file->position = size;
    }
done:
    IndirectSetLock(parent->buffer, 0);
    IndirectSetLock(parent->buffer2, 0);
    if (err != 0) {
        _FT_ChunkFilePostflight(parent, &mark);
    } else {
        result = _FT_ChunkFilePostflight(parent, &mark);
        if (result != 0)
            err = result;
    }
    return err;
}

long _FT_ChunkGetMark(FTFile *file, ULong *position)
{
    long err;
    FTFile *parent;

    err = 0;
    parent = file->parent;
    *position = file->position;
    return err;
}

long _FT_ChunkSetMark(FTFile *file, ULong position)
{
    long err;
    FTFile *parent;

    err = 0;
    parent = file->parent;
    if (position > file->size)
        err = kFTErrPosition;
    else
        file->position = position;
    return err;
}

long _FT_ChunkGetCount(FTFile *archive, const char *name, ULong *count)
{
    long err, e;
    FTFile *dir;
    FTFile *p;
    ULong i;
    FTArchiveHeader h;

    err = 0;
    dir = 0;
    p = 0;
    *count = 0;
    if (name != 0 && name[0] != 0) {
        dir = MemoryAllocate(sizeof(FTFile));
        if (dir == 0) {
            err = kOutOfMemory;
            goto done;
        }
        memset(dir, 0, sizeof(FTFile));
        e = _FT_ChunkOpen(archive, dir, name, 0, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (dir->buffer == 0) {
            e = _FT_ChunkLoad(dir);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        for (p = dir; (p = p->parent) != 0; )
            _FT_FileUpdateRefCount(p, 1);
        dir->link = gFT_FileList;
        gFT_FileList.next = dir;
        _FT_FileUpdateRefCount(dir, 1);
    } else {
        dir = archive;
        if (dir->buffer == 0) {
            e = _FT_ChunkLoad(dir);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    IndirectSetLock(dir->buffer, 1);
    IndirectSetLock(dir->buffer2, 1);
    memmove(&h, *dir->buffer, sizeof(h));
    for (i = 0; i < h.entryCount; i++) {
        if (ArchiveEntry(dir, i)->name[0] != 0)
            (*count)++;
    }
done:
    /* BUG: dir is NULL here when its allocation failed, and it is dereferenced before the test */
    IndirectSetLock(dir->buffer, 0);
    IndirectSetLock(dir->buffer2, 0);
    if (dir != 0 && dir != archive)
        FT_FileClose(dir);
    return err;
}

