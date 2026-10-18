// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atplatheap.c -- the platform layer's heap and memory primitives, and the handle manager: allocate, copy, release, resize, lock, append/insert/replace, pack and unpack, the heap check. Notes: Research/notes/atplatheap.md */

#include <stdlib.h>
#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"
#include "zlib.h"


extern void *gProcessHeap;                       /* atplatopen.c */

#define kHeapZeroMemory 8

/* MemoryAllocate. STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe1f9
void *MemoryAllocate(ULong size)
{
    long err;
    void *p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    p = HeapAlloc(gProcessHeap, 0, size);
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

/* MemoryAllocateClear. STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe22c
void *MemoryAllocateClear(ULong size)
{
    long err;
    void *p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    p = HeapAlloc(gProcessHeap, kHeapZeroMemory, size);
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

/* MemoryDeallocate. STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe25f
void MemoryDeallocate(void *p)
{
    long err;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    HeapFree(gProcessHeap, 0, p);
}

/* MemoryChecksum. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe285
long MemoryChecksum(void *data, unsigned long size)
{
    long crc;
    long err;

    err = 0;
    crc = crc32(0, 0, 0);
    if (size != 0)
        crc = crc32(crc, data, size);
    return crc;
}

/* MemoryClear. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe2b0
void MemoryClear(void *p, ULong n)
{
    long err;

    err = 0;
    memset(p, 0, n);
}

/* Byte-fills up to the first dword boundary, then dwords, then the tail. */
/* MemoryFill. STATE: MATCH 100%, exe 40 / obj 40, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe2c3
void MemoryFill(void *p, int c, ULong n)
{
    long err;
    ULong fill;
    ULong count;
    ULong i;
    unsigned char *d;
    unsigned char b;

    err = 0;
    b = (unsigned char)c;
    fill = b;
    d = p;
    count = n;
    fill |= fill << 16;
    while (((ULong)d & 3) != 0 && count != 0) {
        *d++ = b;
        count--;
    }
    fill |= fill << 8;
    i = count >> 2;
    while (i--) {
        *(ULong *)d = fill;
        d += 4;
    }
    i = count & 3;
    while (i--)
        *d++ = b;
}

/* MemoryCopy. STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe31a
void MemoryCopy(void *dst, const void *src, ULong n)
{
    long err;

    err = 0;
    if (n != 0)
        memmove(dst, src, n);
}

/* MemoryMove. STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe338
void MemoryMove(void *dst, const void *src, ULong n)
{
    long err;

    err = 0;
    if (n != 0)
        memmove(dst, src, n);
}

#define kRecordsPerBlock  0x100

static MemBlock *gMemBlockList;                   /* 0x008676f0 block list */

/* Add a block of `count` free records (256 when 0) to the list. */
/* IndirectPreflight. STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe356
long IndirectPreflight(ULong count)
{
    long err;
    MemBlock *block;
    ULong i;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (count == 0)
        count = kRecordsPerBlock;
    block = 0;
    i = sizeof(MemBlock) + (count - 1) * sizeof(MemRecord);
    block = malloc(i);
    if (block == 0) {
        err = kOutOfMemory;
    } else {
        MemoryClear(block, i);
        block->count = count;
        block->next = gMemBlockList;
        for (i = 0; i < count; i++) {
            block->records[i].guard = kRecordGuard;
            block->records[i].block = block;
        }
        gMemBlockList = block;
    }
    return err;
}

/* A new handle of `size` bytes (a zero size still gets one byte). */
/* IndirectAllocate. STATE: MATCH 100%, exe 73 / obj 73, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe3de
void **IndirectAllocate(ULong size)
{
    long err, e;
    MemBlock *block;
    MemRecord *rec;
    MemRecord *found;
    ULong i;

    err = 0;
    found = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    block = 0;
    rec = 0;
    e = _IndirectValidate(0);
    if (e != 0) {
        err = e;
        goto done;
    }
    block = gMemBlockList;
    for (;;) {
        if (block == 0) {
            e = IndirectPreflight(0);
            if (e != 0) {
                err = e;
                goto done;
            }
            block = gMemBlockList;
        }
        for (i = 0; i < block->count; i++) {
            if (block->records[i].used == 0)
                break;
        }
        if (i < block->count) {
            rec = &block->records[i];
            break;
        }
        block = block->next;
    }
    rec->ptr = malloc(size != 0 ? size : 1);
    if (rec->ptr == 0) {
        err = kOutOfMemory;
        goto done;
    }
    rec->size = size;
    rec->locked = 0;
    rec->used = 1;
    found = rec;
done:
    return (void **)found;
}

/* IndirectAllocateTempMem (a new handle through IndirectAllocate, after the same three discarded checks;
   the FT loader's alternative allocator). STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe493
void **IndirectAllocateTempMem(ULong size)
{
    long err;
    void **result;

    err = 0;
    result = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    result = IndirectAllocate(size);
    return result;
}

/* A new handle holding a copy of another's bytes. */
/* IndirectDuplicate. STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe4b5
void **IndirectDuplicate(void **handle)
{
    long err;
    void **copy;
    ULong size;
    MemRecord *rec;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    rec = (MemRecord *)handle;
    size = IndirectGetSize(handle);
    copy = IndirectAllocate(size);
    if (copy == 0)
        err = kOutOfMemory;
    else
        MemoryCopy(*copy, rec->ptr, size);
    return copy;
}


char (*gIndirectBusy)(void *p);                   /* 0x008707f8, release hook */

/* IndirectInitialize. STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe501
void *IndirectInitialize(const void *src, ULong size)
{
    long err;
    void **p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    p = IndirectAllocate(size);
    if (p == 0)
        err = kOutOfMemory;
    else
        MemoryCopy(*p, src, size);
    return p;
}

/* IndirectDeallocate. STATE: MATCH 100%, exe 41 / obj 41, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe546
long IndirectDeallocate(void **p)
{
    long err;
    long e;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();

    if (gIndirectBusy != 0)
        gIndirectBusy(p);
    if (gIndirectBusy != 0) {
        if (gIndirectBusy(p) != 0)
            return kBadArgument;
    }

    e = 0;
    if (_IndirectValidate(0) == 0) {
        free(p[0]);
        p[0] = 0;
        p[2] = 0;
        ((char *)p)[0xc] = 0;
        ((char *)p)[0xd] = 0;
    }
    /* BUG: no return here: the caller reads eax, _IndirectValidate's code on its failure and a
       zero left by the stores on success */
}

/* IndirectGetSize. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe5b8
ULong IndirectGetSize(void **handle)
{
    long err;
    ULong result;
    MemRecord *rec;
    MemBlock *block;

    err = 0;
    result = 0;
    rec = (MemRecord *)handle;
    block = rec->block;
    result = rec->size;
    return result;
}

/* IndirectSetSize. STATE: MATCH 100%, exe 45 / obj 45, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe5cd
long IndirectSetSize(void **handle, ULong size)
{
    long err;
    MemRecord *rec;
    MemBlock *block;
    void *p;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        rec = (MemRecord *)handle;
        block = rec->block;
        p = realloc(rec->ptr, size != 0 ? size : 1);
        if (p == 0) {
            err = kOutOfMemory;
        } else {
            rec->ptr = p;
            rec->size = size;
            rec->locked = 0;
            rec->used = 1;
        }
    }
    return err;
}

/* IndirectGetLock. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe636
unsigned char IndirectGetLock(void **handle)
{
    long err;
    unsigned char result;
    MemRecord *rec;
    MemBlock *block;

    err = 0;
    result = 0;
    rec = (MemRecord *)handle;
    block = rec->block;
    result = rec->locked;
    return result;
}

/* IndirectSetLock. STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe64b
unsigned char IndirectSetLock(void **handle, unsigned char lock)
{
    unsigned char old;
    long err;
    MemRecord *rec;
    MemBlock *block;

    err = 0;
    old = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();

    rec = (MemRecord *)handle;
    block = rec->block;
    old = rec->locked;
    rec->locked = lock;
    return old;
}

/* IndirectAppend (append one handle's bytes to another). STATE: MATCH 100%, exe 53 / obj 53, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe674
long IndirectAppend(void **handle, void **from)
{
    long err, e;
    ULong old;
    ULong length;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        old = IndirectGetSize(handle);
        length = IndirectGetSize(from);
        if (length != 0) {
            e = IndirectSetSize(handle, old + length);
            if (e != 0)
                err = e;
            else
                MemoryCopy((char *)*handle + old, *from, length);
        }
    }
    return err;
}

/* IndirectReplace (make a handle a copy of another). STATE: MATCH 100%, exe 41 / obj 41, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe6f1
long IndirectReplace(void **handle, void **from)
{
    long err, e;
    ULong length;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        length = IndirectGetSize(from);
        e = IndirectSetSize(handle, length);
        if (e != 0)
            err = e;
        else
            MemoryCopy(*handle, *from, length);
    }
    return err;
}

/* IndirectInsertData (insert bytes at the front of a handle). STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe74e
long IndirectInsertData(void **handle, const void *data, ULong length)
{
    long err, e;
    ULong old;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        old = IndirectGetSize(handle);
        if (length != 0) {
            e = IndirectSetSize(handle, old + length);
            if (e != 0)
                err = e;
            else {
                MemoryMove((char *)*handle + length, *handle, old);
                MemoryCopy(*handle, data, length);
            }
        }
    }
    return err;
}

/* IndirectAppendData (append bytes to a handle). STATE: MATCH 100%, exe 46 / obj 46,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe7c3
long IndirectAppendData(void **handle, const void *data, ULong length)
{
    long err, e;
    ULong old;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        old = IndirectGetSize(handle);
        if (length != 0) {
            e = IndirectSetSize(handle, old + length);
            if (e != 0)
                err = e;
            else
                MemoryCopy((char *)*handle + old, data, length);
        }
    }
    return err;
}

/* IndirectReplaceData (replace a handle's contents with length bytes of data). STATE: MATCH 100%, exe 37 / obj 37, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe82c
long IndirectReplaceData(void **handle, const void *data, ULong length)
{
    long err, e;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        e = IndirectSetSize(handle, length);
        if (e != 0)
            err = e;
        else if (length != 0)
            MemoryCopy(*handle, data, length);
    }
    return err;
}

/* Replaces oldLength bytes at offset with newLength bytes of data. */
/* IndirectSpliceData. STATE: MATCH 100%, exe 78 / obj 78, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe884
long IndirectSpliceData(void **handle, ULong offset, ULong oldLength, const void *data,
                  ULong newLength)
{
    long err, e;
    ULong size;
    ULong end;
    ULong len;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    size = IndirectGetSize(handle);
    len = newLength;
    end = offset + oldLength;
    if (end > size) {
        err = kBadArgument;
        goto done;
    }
    if (oldLength < newLength) {
        e = IndirectSetSize(handle, size + newLength - oldLength);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    MemoryMove((char *)*handle + offset + newLength,
                 (char *)*handle + offset + oldLength, size - end);
    MemoryCopy((char *)*handle + offset, data, newLength);
    if (oldLength > newLength) {
        e = IndirectSetSize(handle, size + newLength - oldLength);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

#define kPackMagic      'sqsh'
#define kTagNone        'none'
#define kTagZlib        'zlib'
#define kTagPack        'pack'      /* PackBits over bytes */
#define kTagPackWide    'PACK'      /* PackBits over 16-bit units */
#define kTagAny         '****'
#define kTagBest        'best'
#define kTagFast        'fast'
#define kGrowStep       0x1000
#define kMaxRun         0x7f

/* Packs a handle in place: 'zlib' (deflate), 'pack'/'PACK' (PackBits), 'none'; '****' and
   'best' mean zlib, 'fast' means pack. */
/* IndirectCompress. STATE: MATCH 100%, exe 571 / obj 571, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe949
long IndirectCompress(void **handle, ULong tag, PackInfo *info)
{
    long err;
    void **out;
    unsigned char locked;
    unsigned char zopen;
    unsigned char finished;
    z_stream z;
    int zerr;
    long e;
    ULong size, pos, outPos, cap, run, n, i;
    unsigned char *src;
    unsigned char repeat;
    unsigned int a, b, a1, b1, b2;
    unsigned short width;

    err = 0;
    out = 0;
    locked = 0;
    zopen = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (info != 0)
        MemoryClear(info, sizeof(PackInfo));
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    locked = 1;
    if (IndirectGetSize(handle) == 0)
        tag = kTagNone;
    else if (tag == kTagAny || tag == kTagBest)
        tag = kTagZlib;
    else if (tag == kTagFast)
        tag = kTagPack;

    if (tag == kTagZlib) {
        finished = 0;
        z.zalloc = Z_NULL;
        z.zfree = Z_NULL;
        z.opaque = Z_NULL;
        zerr = deflateInit(&z, Z_DEFAULT_COMPRESSION);
        switch (zerr) {
        case Z_OK:
            break;
        case Z_MEM_ERROR:
            err = zerr;
            err = kOutOfMemory;
            goto done;
        case Z_STREAM_ERROR:
        case Z_VERSION_ERROR:
        default:
            err = zerr;
            err = kBadArgument;
            goto done;
        }
        zopen = 1;
        out = IndirectAllocate(0);
        if (out == 0) {
            err = kOutOfMemory;
            goto done;
        }
        while (!finished) {
            e = IndirectSetSize(out, IndirectGetSize(out) + kGrowStep);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(handle, 1);
            IndirectSetLock(out, 1);
            z.next_in = (Bytef *)*handle + z.total_in;
            z.avail_in = IndirectGetSize(handle) - z.total_in;
            z.next_out = (Bytef *)*out + z.total_out;
            z.avail_out = IndirectGetSize(out) - z.total_out;
            zerr = deflate(&z, z.avail_in != 0 ? Z_NO_FLUSH : Z_FINISH);
            switch (zerr) {
            case Z_OK:
            case Z_BUF_ERROR:
                break;
            case Z_STREAM_END:
                finished = 1;
                break;
            case Z_STREAM_ERROR:
            default:
                err = zerr;
                err = kBadArgument;
                goto done;
            }
            IndirectSetLock(handle, 0);
            IndirectSetLock(out, 0);
        }
        e = IndirectSetSize(out, z.total_out);
        if (e != 0) {
            err = e;
            goto done;
        }
        zopen = 0;
        zerr = deflateEnd(&z);
        switch (zerr) {
        case Z_OK:
            break;
        case Z_STREAM_ERROR:
        case Z_DATA_ERROR:
        default:
            err = zerr;
            err = kBadArgument;
            goto done;
        }
    } else if (tag == kTagPack || tag == kTagPackWide) {
        width = 1;
        if ((IndirectGetSize(handle) & 1) != 0 && tag == kTagPackWide) {
            err = kBadArgument;
            goto done;
        }
        out = IndirectAllocate(0);
        if (out == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (tag == kTagPackWide)
            width = 2;
        IndirectSetLock(handle, 1);
        size = IndirectGetSize(handle);
        src = *handle;
        pos = outPos = run = cap = 0;
        while (pos < size) {
            repeat = 0;
            if (width == 1) {
                a = src[pos];
                if (pos + 2 < size && a == src[pos + 1] && a == src[pos + 2])
                    repeat = 1;
            } else {
                a = src[pos];
                b = src[pos + 1];
                if (pos + 4 < size && a == src[pos + 2] && b == src[pos + 3] &&
                    a == src[pos + 4] && b == src[pos + 5])
                    repeat = 1;
            }
            if (repeat) {
                if (width == 1) {
                    for (run = 3; pos + run < size && a == src[pos + run]; run++)
                        ;
                } else {
                    for (run = 3; pos + run * 2 < size && a == src[pos + run * 2] &&
                                  b == src[pos + run * 2 + 1]; run++)
                        ;
                }
                pos += run * width;
                while (run != 0) {
                    if (cap - outPos < width + 1) {
                        cap += kGrowStep;
                        e = IndirectSetSize(out, cap);
                        if (e != 0) {
                            err = e;
                            goto done;
                        }
                    }
                    if (run > 2 * (kMaxRun - 1))
                        i = kMaxRun - 1;
                    else if (run > kMaxRun - 1)
                        i = (run + 1) >> 1;
                    else
                        i = run;
                    ((unsigned char *)*out)[outPos] = 1 - i;
                    ((unsigned char *)*out)[outPos + 1] = a;
                    outPos += 2;
                    if (width == 2) {
                        ((unsigned char *)*out)[outPos] = b;
                        outPos++;
                    }
                    run -= i;
                }
            } else {
                if (width == 1) {
                    i = a1 = ~a;
                    for (run = 0; ++run < kMaxRun && pos + run < size; ) {
                        if (a == a1 && a == i)
                            break;
                        i = a1;
                        a1 = a;
                        a = src[pos + run];
                    }
                } else {
                    i = a1 = ~a;
                    b2 = b1 = ~b;
                    for (run = 0; ++run < kMaxRun && pos + run * 2 < size; ) {
                        if (a == a1 && b == b1 && a == i && b == b2)
                            break;
                        i = a1;
                        a1 = a;
                        a = src[pos + run * 2];
                        b2 = b1;
                        b1 = b;
                        b = src[pos + run * 2 + 1];
                    }
                }
                n = width * run;
                if (!(cap - outPos >= n + 1)) {
                    cap += kGrowStep;
                    e = IndirectSetSize(out, cap);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                }
                ((unsigned char *)*out)[outPos] = run - 1;
                outPos++;
                for (i = 0; i < n; i++)
                    ((unsigned char *)*out)[outPos++] = src[pos++];
            }
            run = 0;
        }
        IndirectSetLock(handle, 0);
        IndirectSetLock(out, 0);
        e = IndirectSetSize(out, outPos);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else if (tag != kTagNone) {
        err = kUnsupported;
        goto done;
    }

    if (info != 0) {
        info->magic = kPackMagic;
        info->tag = tag;
        info->size = IndirectGetSize(handle);
        info->sum = MemoryChecksum(*handle, IndirectGetSize(handle));
        if (tag == kTagNone) {
            info->packedSize = IndirectGetSize(handle);
            info->packedSum = MemoryChecksum(*handle, IndirectGetSize(handle));
        } else {
            info->packedSize = IndirectGetSize(out);
            info->packedSum = MemoryChecksum(*out, IndirectGetSize(out));
        }
    }
    if (out != 0) {
        size = IndirectGetSize(out);
        e = IndirectSetSize(handle, size);
        if (e != 0)
            err = e;
        else
            MemoryCopy(*handle, *out, size);
    }
done:
    if (zopen)
        deflateEnd(&z);
    if (locked)
        IndirectSetLock(handle, 0);
    if (out != 0)
        IndirectDeallocate(out);
    return err;
}

/* Unpacks a handle in place; the inverse of IndirectCompress. */
/* IndirectDecompress. STATE: MATCH 100%, exe 468 / obj 468, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ff112
long IndirectDecompress(void **handle, ULong tag, PackInfo *info)
{
    long err;
    void **out;
    unsigned char locked;
    unsigned char zopen;
    int zerr;
    unsigned short width;
    unsigned short count;
    ULong size;
    ULong pos;
    ULong outPos;
    ULong cap;
    ULong n;
    unsigned char *src;
    z_stream z;
    unsigned char *p;
    unsigned int c;
    unsigned int b0;
    unsigned int b1;
    long r;

    err = 0;
    out = 0;
    locked = 0;
    zopen = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (info != 0)
        MemoryClear(info, sizeof(PackInfo));
    if (IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    locked = 1;
    if (IndirectGetSize(handle) == 0)
        tag = kTagNone;

    if (tag == kTagNone) {
    } else if (tag == kTagZlib) {
        size = IndirectGetSize(handle) * 2;
        if (size < kGrowStep)
            size = kGrowStep;
        out = IndirectAllocate(size);
        if (out == 0) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(out, 1);
        IndirectSetLock(handle, 1);
        z.zalloc = Z_NULL;
        z.zfree = Z_NULL;
        z.opaque = Z_NULL;
        z.next_in = *handle;
        z.avail_in = IndirectGetSize(handle);
        z.next_out = *out;
        z.avail_out = IndirectGetSize(out);
        zerr = inflateInit(&z);
        switch (zerr) {
        case Z_OK:
            break;
        case Z_MEM_ERROR:
            err = zerr;
            err = kOutOfMemory;
            goto done;
        case Z_VERSION_ERROR:
        default:
            err = zerr;
            err = kBadArgument;
            goto done;
        }
        zopen = 1;
        while (z.avail_in != 0 && zerr != Z_STREAM_END) {
            zerr = inflate(&z, Z_NO_FLUSH);
            switch (zerr) {
            case Z_OK:
            case Z_STREAM_END:
                break;
            case Z_STREAM_ERROR:
                err = zerr;
                err = kBadArgument;
                goto done;
            case Z_BUF_ERROR:
            default:
                err = zerr;
                err = kBadArgument;
                goto done;
            }
            if (zerr != Z_STREAM_END) {
                IndirectSetLock(out, 0);
                size = z.total_out + kGrowStep;
                r = IndirectSetSize(out, size);
                if (r != 0) {
                    err = r;
                    goto done;
                }
                IndirectSetLock(out, 1);
                z.next_out = (Bytef *)*out + z.total_out;
                z.avail_out = size - z.total_out;
            }
        }
        IndirectSetLock(handle, 0);
        IndirectSetLock(out, 0);
        r =IndirectSetSize(out, z.total_out);
        if (r != 0) {
            err = r;
            goto done;
        }
        zopen = 0;
        zerr = inflateEnd(&z);
        switch (zerr) {
        case Z_OK:
            break;
        case Z_STREAM_ERROR:
        default:
            err = zerr;
            err = kBadArgument;
            goto done;
        }
    } else if (tag == kTagPack || tag == kTagPackWide) {
        width = 1;
        out = IndirectAllocate(0);
        if (out == 0) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(handle, 1);
        if (tag == kTagPackWide)
            width = 2;
        size = IndirectGetSize(handle);
        src = *handle;
        pos = outPos = cap = 0;
        while (pos < size) {
            c = src[pos];
            if (c == 0x80) {
                pos++;
                continue;
            }
            if ((c & 0x80) != 0) {
                count = 0x101 - c;
                if (pos + width + 1 > size) {
                    err = kBadArgument;
                    goto done;
                }
                if (cap - outPos < count * width) {
                    cap += kGrowStep;
                    r = IndirectSetSize(out, cap);
                    if (r != 0) {
                        err = r;
                        goto done;
                    }
                }
                if (width == 1) {
                    c = src[pos + 1];
                    while (count--)
                        ((unsigned char *)*out)[outPos++] = c;
                } else {
                    b0 = src[pos + 1];
                    b1 = src[pos + 2];
                    while (count--) {
                        ((unsigned char *)*out)[outPos] = b0;
                        ((unsigned char *)*out)[outPos + 1] = b1;
                        outPos += 2;
                    }
                }
                pos += width + 1;
            } else {
                count = c + 1;
                n = count * width;
                if (pos + n > size) {
                    err = kBadArgument;
                    goto done;
                }
                if (cap - outPos < n) {
                    cap += kGrowStep;
                    r = IndirectSetSize(out, cap);
                    if (r != 0) {
                        err = r;
                        goto done;
                    }
                }
                if (width == 1) {
                    p = src + pos + 1;
                    pos += count + 1;
                    while (count--)
                        ((unsigned char *)*out)[outPos++] = *p++;
                } else {
                    p = src + pos + 1;
                    pos += count * 2 + 1;
                    while (count--) {
                        ((unsigned char *)*out)[outPos] = p[0];
                        ((unsigned char *)*out)[outPos + 1] = p[1];
                        p += 2;
                        outPos += 2;
                    }
                }
            }
        }
        IndirectSetLock(handle, 0);
        IndirectSetLock(out, 0);
        r = IndirectSetSize(out, outPos);
        if (r != 0) {
            err = r;
            goto done;
        }
    } else {
        if (tag == kTagAny || tag == kTagFast || tag == kTagBest) {
            err = kUnsupported;
            goto done;
        }
        err = kUnsupported;
        goto done;
    }

    if (info != 0) {
        info->magic = kPackMagic;
        info->tag = tag;
        info->packedSize = IndirectGetSize(handle);
        info->packedSum = MemoryChecksum(*handle, IndirectGetSize(handle));
        if (tag == kTagNone) {
            info->size = IndirectGetSize(handle);
            info->sum = MemoryChecksum(*handle, IndirectGetSize(handle));
        } else {
            info->size = IndirectGetSize(out);
            info->sum = MemoryChecksum(*out, IndirectGetSize(out));
        }
    }
    if (out != 0) {
        size = IndirectGetSize(out);
        r = IndirectSetSize(handle, size);
        if (r != 0) {
            err = r;
            goto done;
        }
        MemoryCopy(*handle, *out, size);
    }
done:
    if (zopen)
        inflateEnd(&z);
    if (locked)
        IndirectSetLock(handle, 0);
    if (out != 0)
        IndirectDeallocate(out);
    return err;
}

/* _IndirectValidate. STATE: MATCH 100%, exe 70 / obj 70, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ff774
long _IndirectValidate(void **handle)
{
    long err;
    MemBlock *block;
    ULong i;
    unsigned char found;

    err = 0;
    block = 0;
    found = 0;
    for (block = gMemBlockList; block != 0; block = block->next) {
        IsBadReadPtr(block, block->count * sizeof(ULong));
        IsBadWritePtr(block, block->count * sizeof(ULong));
        for (i = 0; i < block->count; i++) {
            if ((void **)&block->records[i] == handle)
                found = 1;
            if (block->records[i].guard != kRecordGuard)
                err = kOSFailure;
            if (err == 0 && block->records[i].used != 0 && block->records[i].ptr == 0)
                err = kOSFailure;
        }
    }
    if (handle != 0 && IndirectGetSize(handle) != 0) {
        IsBadReadPtr(*handle, IndirectGetSize(handle));
        IsBadWritePtr(*handle, IndirectGetSize(handle));
    }
    return err;
}
