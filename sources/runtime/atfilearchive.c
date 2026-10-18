// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atfilearchive.c -- the file tool's files of files: archive members opened, read, written, renamed and listed through the CHNK directory. Notes: Research/notes/atfilespec.md */

#include <stdlib.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "macport.h"
#include "win32.h"
#include "atfiletool.h"
#define ATSTR_CMP
#include "atfilestr.h"

/* _FT_IsChunkOpen (the open FTFile for archive member `name`, else NULL). STATE: MATCH 100%,
   exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050be8a
FTFile *_FT_IsChunkOpen(FTFile *archive, const char *name)
{
    long err;
    FTFile **link;
    FTFile *f;
    ULong index;

    err = 0;
    link = 0;
    if (!_FT_ChunkLookup(archive, name, &index, 0)) {
        err = kFTErrFileNotFound;
        goto done;
    }
    link = &gFT_FileList;
    while ((f = *link) != 0) {
        if (f->parent == archive && (ULong)f->origin == index)
            break;
        link = &f->next;
    }
done:
    return link != 0 ? *link : 0;
}

/* _FT_ChunkOpenPath (open the member a spec's names lead to, through nested files of files; with
   toLast, stop at the one holding the last name and return that name escaped in `leaf`).
   STATE: MATCH 100%, exe 275 / obj 275, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050beeb
long _FT_ChunkOpenPath(FolderSpec **spec, unsigned char write, unsigned char toLast,
                  unsigned char mustBeArchive, FTFile **result, char *leaf)
{
    long err, e;
    unsigned char locked;
    FTFile *parent;
    ULong offset, limit;
    unsigned char *name;
    long saved;
    FTFile *root;
    FTFile *file;
    char cname[256];
    ULong index;
    ULong flags;

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
    while ((offset = _FT_PathWalk(spec, offset)) != 0) {
        name = (unsigned char *)*spec + offset;
        ATPToCStr(cname, name);
        parent = *result;
        *result = 0;
        if (toLast && name[0] + offset >= limit) {
            if (leaf != 0)
                _FT_PathNameOutput(leaf, name);
            *result = parent;
            goto done;
        }
        if ((*result = _FT_IsChunkOpen(parent, cname)) == 0) {
            if ((*result = MemoryAllocate(kFTFileSize)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            memset(*result, 0, kFTFileSize);
            if (!_FT_ChunkLookup(parent, cname, &index, 0)) {
                err = kFTErrFileNotFound;
                goto done;
            }
            flags = ATSwap32(ArchiveEntry(parent, index)->flags);
            err = _FT_ChunkOpen(parent, *result, cname, write,
                               (flags & (1 << kFTInfoFolder)));
            if (err != 0) {
                if (_FT_PathWalk(spec, offset) != 0)
                    err = kFTErrDirNotFound;
                goto done;
            }
            (file = *result)->next = gFT_FileList;
            gFT_FileList = *result;
        }
        /* BUG: the reference the previous step took on the parent is never dropped */
        _FT_FileUpdateRefCount(*result, 1);
    }
done:
    if (root != 0 && saved != -1)
        root->procs.proc[kFTSeek](root, saved);
    IndirectSetLock((void **)spec, locked);
    if (err != 0) {
        if (*result != 0) {
            MemoryDeallocate(*result);
            *result = 0;
        }
        if (parent != 0) {
            FT_FileClose(parent);
            return err;
        } else if (root != 0) {
            FT_FileClose(root);
            return err;
        }
    }
    return err;
}

/* _FT_ChunkOpen (set up `file` as the archive's member `name`: size, the member procedures;
   refused when the entry's flags forbid; `nested` loads its own directory). STATE:
   MATCH 100%, exe 176 / obj 176, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050c226
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
    if (!AtomicMaskTest(&flags, kFTInfoReadable) || (write && archive->mode == 0)
        || (write && !AtomicMaskTest(&flags, kFTInfoWritable))) {
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
    if (archive->size < file->position || archive->size < file->position + file->size) {
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

/* _FT_ChunkClose (FT file buffers release). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050c433
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

/* _FT_ChunkLoad (load a file of files' header and entry table into its two handles; a bad
   header is kFTErrBadEntry). STATE: MATCH 100%, exe 236 / obj 236, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050c472
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
    if (file->size < kArchiveHeaderSize)
        goto bad;
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
    h.headerSize = ATSwap32(h.headerSize);
    h.tableOffset = ATSwap32(h.tableOffset);
    h.entrySize = ATSwap32(h.entrySize);
    h.entryCount = ATSwap32(h.entryCount);
    if (!IsSignature(h.signature, sig) || (size = h.headerSize) < kArchiveHeaderSize
        || (fileSize = file->size) < (table = h.tableOffset)
        || fileSize < h.entrySize * h.entryCount + table) {
bad:
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

/* _FT_ChunkLookup (find archive member `name` in the big-endian directory: its index and a
   copy of its entry). STATE: MATCH 100%, exe 138 / obj 138, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050c729
unsigned char _FT_ChunkLookup(FTFile *archive, const char *name, ULong *index,
                           FTArchiveEntry *entry)
{
    unsigned char found;
    unsigned char locked, locked2;
    FTArchiveEntry *e;
    long long stride;
    ULong size;
    FTArchiveHeader *header;
    ULong count;
    ULong i;

    count = 0;
    e = 0;
    locked = 0;
    locked2 = 0;
    found = 0;
    if (index != 0)
        *index = 0xffffffff;
    if (entry != 0)
        memset(entry, 0, sizeof(FTArchiveEntry));
    locked = IndirectSetLock(archive->buffer, 1);
    locked2 = IndirectSetLock(archive->buffer2, 1);
    header = (FTArchiveHeader *)*archive->buffer;
    count = ATSwap32(header->entryCount);
    stride = ATSwap32(header->entrySize);
    size = stride;
    for (i = 0; i < count; i++) {
        e = (FTArchiveEntry *)((char *)*archive->buffer2 + i * stride);
        if (ATStrCmp(e->name, name) == 0) {
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

/* _FT_ChunkFormat (write an empty file of files into an empty file: the header and `entries`
   free entries, kDefaultEntries when 0 or over kMaxEntries). STATE: MATCH 100%, exe 133 /
   obj 133, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050c8b9
long _FT_ChunkFormat(FTFile *file, ULong entries)
{
    long err, e;
    char *table;
    ULong size;
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

/* _FT_ChunkResize (FT resize a member's extent inside an archive). STATE: MATCH 100%, exe 188 /
   obj 188, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0050ca24
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

/* _FT_ChunkCreate (add member info->name to a file of files: the first free entry, else the
   table doubled at the end of the file and the old one zeroed; a folder becomes a nested
   file of files). STATE: MATCH 100%, exe 671 / obj 671, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050cc24
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
    n = *(unsigned char *)&name;
    while (n--) {
        *s = s[1];
        s++;
    }
    *s = 0;
    IndirectSetLock(archive->buffer, 1);
    IndirectSetLock(archive->buffer2, 1);
    memmove(&h, *archive->buffer, sizeof(h));
    h.headerSize = Swap32(h.headerSize);
    h.tableOffset = Swap32(h.tableOffset);
    h.entrySize = Swap32(h.entrySize);
    h.entryCount = Swap32(h.entryCount);
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
        ((FTArchiveHeader *)*archive->buffer)->tableOffset = Swap32(h.tableOffset);
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
        ((FTArchiveHeader *)*archive->buffer)->entryCount = Swap32(h.entryCount);
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
    slot->flags = Swap32((info->flags[kFTInfoReadable] ? 1UL << kFTInfoReadable : 0)
                         | (info->flags[kFTInfoWritable] ? 1UL << kFTInfoWritable : 0)
                         | (info->flags[kFTInfoFolder] ? 1UL << kFTInfoFolder : 0)
                         | (info->flags[kFTInfoHidden] ? 1UL << kFTInfoHidden : 0));
    slot->offset = 0;
    slot->length = 0;
    slot->type = Swap32(info->type);
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
    if ((dir = MemoryAllocate(kFTFileSize)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(dir, 0, kFTFileSize);
    e = _FT_ChunkOpen(archive, dir, (char *)name, 1, 0);
    if (e == 0) {
        for (p = dir; (p = p->parent) != 0; )
            _FT_FileUpdateRefCount(p, 1);
        dir->next = gFT_FileList;
        gFT_FileList = dir;
        _FT_FileUpdateRefCount(dir, 1);
        e = _FT_ChunkFormat(dir, entries);
        if (e == 0)
            goto done;
    }
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

/* _FT_ChunkDelete (remove member `name` from a file of files: its entry zeroed and written,
   its extent released; kFTErrBusy while it is open). STATE: MATCH 100%, exe 269 / obj 269,
   byte-identical. OPEN: none (an early failure returns to an unset mark, as the exe) */
// FUNCTION: EVNOVA 0x0050d53a
long _FT_ChunkDelete(FTFile *archive, const char *name)
{
    long err, e;
    unsigned char member[256];
    FTArchiveHeader h;
    FTMark mark;
    ULong offset;
    ULong index;
    ULong length;
    ULong n;
    ULong size;
    FTArchiveEntry *entry;
    unsigned char *p;

    err = 0;
    entry = 0;
    e = _FT_PathNameInput(member, name, 0xff);
    if (e != 0) {
        err = e;
        goto done;
    }
    p = member;
    n = *(unsigned char *)&member;
    while (n--) {
        *p = p[1];
        p++;
    }
    *p = 0;
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

/* _FT_ChunkGetFlags (FT info for a member, named escaped (`name`) or plain (`member`): flags from
   the entry, dates from the file holding it, size, type and the creator by type; a nested
   file of files reads as a folder of its entry count). STATE: MATCH 100%, exe 322 / obj 322,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050d89f
long _FT_ChunkGetFlags(FTFile *archive, const char *name, const char *member, FTFileInfo *info,
                  FolderSpec **spec)
{
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
        member = ATPToCStr(cname, pname);
    } else
        ATCToPStr(pname, member);
    /* BUG: both handles are locked here and never unlocked */
    IndirectSetLock(archive->buffer, 1);
    IndirectSetLock(archive->buffer2, 1);
    memmove(&h, *archive->buffer, sizeof(h));
    h.headerSize = Swap32(h.headerSize);
    h.tableOffset = Swap32(h.tableOffset);
    h.entrySize = Swap32(h.entrySize);
    h.entryCount = Swap32(h.entryCount);
    if (!_FT_ChunkLookup(archive, member, 0, &entry)) {
        if (info == 0) {
            err = kFTErrFileNotFound;
            goto done;
        }
        err = kFTErrFileNotFound;
        goto done;
    }
    entry.flags = Swap32(entry.flags);
    entry.offset = Swap32(entry.offset);
    entry.length = Swap32(entry.length);
    entry.type = Swap32(entry.type);
    if (info != 0) {
        _FT_PathNameOutput(info->name, pname);
        info->flags[kFTInfoReadable] = AtomicMaskTest(&entry.flags, kFTInfoReadable);
        info->flags[kFTInfoWritable] = AtomicMaskTest(&entry.flags, kFTInfoWritable);
        info->flags[kFTInfoFolder] = AtomicMaskTest(&entry.flags, kFTInfoFolder);
        info->flags[kFTInfoHidden] = AtomicMaskTest(&entry.flags, kFTInfoHidden);
        info->flags[kFTInfoArchive] = AtomicMaskTest(&entry.flags, kFTInfoFolder);
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

/* _FT_ChunkRead (FT member read). STATE: MATCH 100%, exe 106 / obj 106, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050dcf2
long _FT_ChunkRead(FTFile *file, ULong count, void *buffer)
{
    FTMark mark;
    long err;
    FTFile *parent;
    long result;

    err = 0;
    parent = file->parent;
    result = _FT_ChunkFilePreflight(parent, &mark);
    if (result != 0)
        goto failed;
    if (file->position + count > file->size) {
        err = kFTErrPosition;
        goto done;
    }
    result = parent->procs.proc[kFTSeek](parent,
        (ULong)(file->position + ATSwap32(ArchiveEntry(parent, file->origin)->offset)));
    if (result != 0)
        goto failed;
    result = parent->procs.proc[kFTRead](parent, count, buffer);
    if (result != 0) {
failed:
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

/* _FT_ChunkWrite (FT member write). STATE: MATCH 100%, exe 119 / obj 119, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050de0c
long _FT_ChunkWrite(FTFile *file, ULong count, const void *buffer)
{
    FTMark mark;
    long err;
    FTFile *parent;
    long result;

    err = 0;
    parent = file->parent;
    result = _FT_ChunkFilePreflight(parent, &mark);
    if (result != 0)
        goto failed;
    if (file->mode == 0) {
        err = kFTErrPermission;
        goto done;
    }
    if (file->position + count > file->size) {
        result = _FT_ChunkSetEOF(file, file->position + count);
        if (result != 0)
            goto failed;
    }
    result = parent->procs.proc[kFTSeek](parent,
        (ULong)(file->position + ATSwap32(ArchiveEntry(parent, file->origin)->offset)));
    if (result != 0)
        goto failed;
    result = parent->procs.proc[kFTWrite](parent, count, buffer);
    if (result != 0) {
failed:
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

/* _FT_ChunkGetEOF (FT member size). STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050df49
long _FT_ChunkGetEOF(FTFile *file, ULong *size)
{
    long err;
    FTFile *parent;

    err = 0;
    parent = file->parent;
    *size = file->size;
    return err;
}

/* _FT_ChunkSetEOF (FT member set size). STATE: MATCH 100%, exe 271 / obj 271, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050df65
long _FT_ChunkSetEOF(FTFile *file, ULong size)
{
    FTArchiveHeader header;
    FTMark mark;
    FTFile *parent;
    FTArchiveEntry *entry;
    ULong offset;
    ULong length;
    long err;
    long result;

    err = 0;
    parent = file->parent;
    entry = 0;
    IndirectSetLock(parent->buffer, 1);
    IndirectSetLock(parent->buffer2, 1);
    memmove(&header, *parent->buffer, sizeof(header));
    header.headerSize = ATSwap32(header.headerSize);
    header.tableOffset = ATSwap32(header.tableOffset);
    header.entrySize = ATSwap32(header.entrySize);
    header.entryCount = ATSwap32(header.entryCount);
    result = _FT_ChunkFilePreflight(parent, &mark);
    if (result != 0)
        goto failed;
    entry = (FTArchiveEntry *)((char *)*parent->buffer2
            + ATSwap32(((FTArchiveHeader *)*parent->buffer)->entrySize) * file->origin);
    offset = ATSwap32(entry->offset);
    length = ATSwap32(entry->length);
    result = _FT_ChunkResize(parent, offset, length, size, &offset);
    if (result != 0)
        goto failed;
    entry->offset = ATSwap32(offset);
    entry->length = ATSwap32(size);
    result = parent->procs.proc[kFTSeek](parent,
                                    file->origin * header.entrySize + header.tableOffset);
    if (result != 0)
        goto failed;
    result = parent->procs.proc[kFTWrite](parent, header.entrySize, entry);
    if (result != 0) {
failed:
        err = result;
    } else {
        file->size = size;
        if (file->position > size)
            file->position = size;
    }
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

/* _FT_ChunkGetMark (FT member position). STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e2a9
long _FT_ChunkGetMark(FTFile *file, ULong *position)
{
    long err;
    FTFile *parent;

    err = 0;
    parent = file->parent;
    *position = file->position;
    return err;
}

/* _FT_ChunkSetMark (FT member seek). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e2c5
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

/* _FT_ChunkGetCount (count the used entries of a file of files, or of its nested member `name`).
   STATE: MATCH 100%, exe 221 / obj 221, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e2e7
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
        dir = MemoryAllocate(kFTFileSize);
        if (dir == 0) {
            err = kOutOfMemory;
            goto done;
        }
        memset(dir, 0, kFTFileSize);
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
        dir->next = gFT_FileList;
        gFT_FileList = dir;
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
    h.headerSize = ATSwap32(h.headerSize);
    h.tableOffset = ATSwap32(h.tableOffset);
    h.entrySize = ATSwap32(h.entrySize);
    h.entryCount = ATSwap32(h.entryCount);
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

extern FileProc gFT_BufferHooks[6];                 /* atftopen.c: the memory procedures */

long _FT_BufferOpen(void **handle, unsigned char mode, FTFile *file);

/* _FT_BufferOpenWrapper (FT file over a memory handle: the open one, ref-counted, or a new one on
   the open-file list). STATE: MATCH 100%, exe 60 / obj 60, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e575
long _FT_BufferOpenWrapper(void **handle, unsigned char mode, FTFile **result)
{
    long err, e;

    err = 0;
    if ((*result = _FT_IsBufferOpen(handle)) == 0) {
        if ((*result = MemoryAllocate(kFTFileSize)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        memset(*result, 0, kFTFileSize);
        e = _FT_BufferOpen(handle, mode, *result);
        if (e != 0) {
            err = e;
            goto done;
        }
        (*result)->next = gFT_FileList;
        gFT_FileList = *result;
    }
    _FT_FileUpdateRefCount(*result, 1);
done:
    if (err != 0 && *result != 0) {
        MemoryDeallocate(*result);
        *result = 0;
    }
    return err;
}

/* _FT_IsBufferOpen (FT open file over a memory handle). STATE: MATCH 100%, exe 17 / obj 17,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e604
FTFile *_FT_IsBufferOpen(void **handle)
{
    FTFile **link;
    FTFile *file;
    long err;

    err = 0;
    link = 0;
    link = &gFT_FileList;
    while ((file = *link) != 0 && file->memHandle != handle)
        link = &file->next;
    return link != 0 ? file : 0;
}

/* _FT_BufferOpen (set an FTFile over a memory handle: size, mode, the memory procedures).
   STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e62e
long _FT_BufferOpen(void **handle, unsigned char mode, FTFile *file)
{
    long err;

    err = 0;
    file->memHandle = handle;
    file->position = 0;
    file->size = IndirectGetSize(file->memHandle);
    file->mode = mode;
    file->procs = *(FTProcs *)gFT_BufferHooks;
    return 0;
}

/* _FT_BufferClose (FT memory file close). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e670
long _FT_BufferClose(FTFile *file)
{
    long err;

    err = 0;
    file->memHandle = 0;
    return 0;
}

/* _FT_BufferFlush (FT memory flush: nothing to do). STATE: MATCH 100%, exe 4 / obj 4, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e683
long _FT_BufferFlush(FTFile *file)
{
    long err;

    err = 0;
    return 0;
}

/* _FT_BufferRead (FT memory read). STATE: MATCH 100%, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e68a
long _FT_BufferRead(FTFile *file, ULong count, void *buffer)
{
    char *src;
    long err;

    err = 0;
    if (file->position + count > file->size) {
        err = kFTErrPosition;
    } else {
        src = (char *)*file->memHandle + file->position;
        if (count != 0)
            memmove(buffer, src, count);
        file->position += count;
    }
    return err;
}

/* _FT_BufferWrite (FT memory write). STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e6ce
long _FT_BufferWrite(FTFile *file, ULong count, const void *buffer)
{
    char *dst;
    long err;
    long result;

    err = 0;
    if (file->size < file->position + count) {
        result = IndirectSetSize(file->memHandle, file->position + count);
        if (result != 0) {
            err = result;
            goto done;
        }
        file->size = file->position + count;
    }
    dst = (char *)*file->memHandle + file->position;
    if (count != 0)
        memmove(dst, buffer, count);
    file->position += count;
done:
    return err;
}

/* _FT_BufferGetEOF (FT memory size). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e729
long _FT_BufferGetEOF(FTFile *file, ULong *size)
{
    long err;

    err = 0;
    *size = IndirectGetSize(file->memHandle);
    return 0;
}

/* _FT_BufferSetEOF (FT memory set size). STATE: MATCH 100%, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e748
long _FT_BufferSetEOF(FTFile *file, ULong size)
{
    long err;
    long result;

    err = 0;
    result = IndirectSetSize(file->memHandle, size);
    if (result != 0) {
        err = result;
    } else {
        file->size = size;
        if (file->position > size)
            file->position = size;
    }
    return err;
}

/* _FT_BufferGetMark (FT memory position). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e77c
long _FT_BufferGetMark(FTFile *file, ULong *position)
{
    long err;

    err = 0;
    *position = file->position;
    return 0;
}

/* _FT_BufferSetMark (FT memory seek). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e792
long _FT_BufferSetMark(FTFile *file, ULong position)
{
    long err;

    err = 0;
    file->position = position;
    return 0;
}

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
