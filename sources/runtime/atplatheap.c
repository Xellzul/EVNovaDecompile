// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atplatheap.c -- the platform layer's heap and memory primitives, and the handle manager: allocate, copy, release, resize, lock, append/insert/replace, pack and unpack, the heap check. Notes: Research/notes/atplatheap.md */

#include <stdlib.h>
#include <string.h>
#include "burgertypes.h"
#ifdef MAC1010
#define IndirectGetLock IndirectGetLockByte
#define IndirectSetLock IndirectSetLockByte
#endif
#include "burger.h"
#ifdef MAC1010
#undef IndirectGetLock
#undef IndirectSetLock
extern int32_t IndirectGetLock(void **handle);
extern int32_t IndirectSetLock(void **handle, unsigned char lock);
#define LockResult int32_t /* the Mac returns the bit whole; callers' prototype is a byte */
#else
#define LockResult unsigned char
#endif
#include "burgerplat.h"
#ifndef MAC1010
#include "win32.h"
#endif
#include "zlib.h"


extern void *gProcessHeap;                       /* atplatopen.c */

#define kHeapZeroMemory 8

/* MemoryAllocate. STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe1f9
void *MemoryAllocate(ULong size)
{
    int32_t err;
    void *p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
#ifdef MAC1010
    p = NewPtr(size);
#else
    p = HeapAlloc(gProcessHeap, 0, size);
#endif
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

/* MemoryAllocateClear. STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe22c
void *MemoryAllocateClear(ULong size)
{
    int32_t err;
    void *p;

    err = 0;
    p = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
#ifdef MAC1010
    p = NewPtrClear(size);
#else
    p = HeapAlloc(gProcessHeap, kHeapZeroMemory, size);
#endif
    if (p == 0)
        err = kOutOfMemory;
    return p;
}

/* MemoryDeallocate. STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe25f
void MemoryDeallocate(void *p)
{
    int32_t err;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
#ifdef MAC1010
    DisposePtr(p);
#else
    HeapFree(gProcessHeap, 0, p);
#endif
}

/* MemoryChecksum. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe285
int32_t MemoryChecksum(void *data, uint32_t size)
{
    int32_t crc;
    int32_t err;

    err = 0;
    crc = crc32(0, 0, 0);
    if (size != 0)
        crc = crc32(crc, data, size);
    return crc;
}

#ifdef MAC1010
extern void (*gMemoryClearProc)(void *p, ULong n);

#if TARGET_API_MAC_CARBON
void MemoryClear(void *p, ULong n)
{
    BlockZero(p, n);
}
#else
void MemoryClear(void *p, ULong n)
{
    ULong i;
    unsigned char *d;

    if (gMemoryClearProc != 0) {
        gMemoryClearProc(p, n);
        return;
    }
    d = p;
    while (((ULong)d & 3) != 0 && n != 0) {
        *d++ = 0;
        n--;
    }
    i = n >> 2;
    while (i--) {
        *(ULong *)d = 0;
        d += 4;
    }
    i = n & 3;
    while (i--)
        *d++ = 0;
}
#endif
#else
/* MemoryClear. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe2b0
void MemoryClear(void *p, ULong n)
{
    int32_t err;

    err = 0;
    memset(p, 0, n);
}
#endif

/* Byte-fills up to the first dword boundary, then dwords, then the tail. */
/* MemoryFill. STATE: MATCH 100%, exe 40 / obj 40, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe2c3
void MemoryFill(void *p, int c, ULong n)
{
    int32_t err;
#ifdef MAC1010 /* 1.0.10 spelling */
    unsigned char b;
    ULong fill;
    ULong count;
    ULong i;
    unsigned char *d;
#else
    ULong fill;
    ULong count;
    ULong i;
    unsigned char *d;
    unsigned char b;
#endif

    err = 0;
    b = c;
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
    int32_t err;

    err = 0;
    if (n != 0)
#ifdef MAC1010
        BlockMoveData(src, dst, n);
#else
        memmove(dst, src, n);
#endif
}

/* MemoryMove. STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe338
void MemoryMove(void *dst, const void *src, ULong n)
{
    int32_t err;

    err = 0;
    if (n != 0)
#ifdef MAC1010
        BlockMoveData(src, dst, n);
#else
        memmove(dst, src, n);
#endif
}

#ifndef MAC1010
#define kRecordsPerBlock  0x100

static MemBlock *gMemBlockList;                   /* 0x008676f0 block list */

/* Add a block of `count` free records (256 when 0) to the list. */
/* IndirectPreflight. STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe356
int32_t IndirectPreflight(ULong count)
{
    int32_t err;
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
    int32_t err, e;
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

#else
void **IndirectAllocate(ULong size)
{
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    return (void **)NewHandle(size);
}
#endif

/* IndirectAllocateTempMem (a new handle through IndirectAllocate, after the same three discarded checks;
   the FT loader's alternative allocator). STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe493
void **IndirectAllocateTempMem(ULong size)
{
    int32_t err;
    void **result;
#ifdef MAC1010
    OSErr e;

    e = 0;
#endif

    err = 0;
    result = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
#ifdef MAC1010
    result = (void **)TempNewHandle(size, &e);
#else
    result = IndirectAllocate(size);
#endif
    return result;
}

/* A new handle holding a copy of another's bytes. */
/* IndirectDuplicate. STATE: MATCH 100%, exe 32 / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe4b5
void **IndirectDuplicate(void **handle)
#ifdef MAC1010
{
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (HandToHand((Handle *)&handle) != 0)
        handle = 0;
    return handle;
}
#else
{
    int32_t err;
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
#endif


AT_EXTERN unsigned char (*gIndirectBusy)(void *p);         /* 0x008707f8, release hook */

/* IndirectInitialize. STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe501
void *IndirectInitialize(const void *src, ULong size)
{
    int32_t err;
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
int32_t IndirectDeallocate(void **p)
{
    int32_t err;
    int32_t e;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();

    if (gIndirectBusy != 0)
        gIndirectBusy(p);
#ifdef MAC1010
    if (gIndirectBusy == 0 || gIndirectBusy(p) == 0)
        DisposeHandle((Handle)p);
#else
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
#endif
    /* BUG: no return here: the caller reads eax, _IndirectValidate's code on its failure and a
       zero left by the stores on success */
}

/* IndirectGetSize. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe5b8
ULong IndirectGetSize(void **handle)
#ifdef MAC1010
{
    return GetHandleSize((Handle)handle);
}
#else
{
    int32_t err;
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
#endif

/* IndirectSetSize. STATE: MATCH 100%, exe 45 / obj 45, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe5cd
int32_t IndirectSetSize(void **handle, ULong size)
#ifdef MAC1010
{
    int32_t err;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
    } else {
        SetHandleSize((Handle)handle, size);
        if (MemError() != 0)
            err = kOutOfMemory;
    }
    return err;
}
#else
{
    int32_t err;
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
#endif

/* IndirectGetLock. STATE: MATCH 100%, exe 9 / obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe636
LockResult IndirectGetLock(void **handle)
#ifdef MAC1010
{
    LockResult result;
    int32_t state;

    result = 0;
    state = HGetState((Handle)handle);
    if (MemError() != 0)
        MemError();
    else
        result = (state >> 7) & 1;
    return result;
}
#else
{
    int32_t err;
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
#endif

/* IndirectSetLock. STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004fe64b
LockResult IndirectSetLock(void **handle, unsigned char lock)
#ifdef MAC1010
{
    LockResult old;
    int32_t state;

    old = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    state = HGetState((Handle)handle);
    if (MemError() != 0)
        MemError();
    else
        old = (state >> 7) & 1;
    if (lock != 0)
        HLock((Handle)handle);
    else
        HUnlock((Handle)handle);
    return old;
}
#else
{
    unsigned char old;
    int32_t err;
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
#endif

/* IndirectAppend (append one handle's bytes to another). STATE: MATCH 100%, exe 53 / obj 53, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004fe674
int32_t IndirectAppend(void **handle, void **from)
{
    int32_t err, e;
    ULong old;
    ULong length;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
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
int32_t IndirectReplace(void **handle, void **from)
{
    int32_t err, e;
    ULong length;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
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
int32_t IndirectInsertData(void **handle, const void *data, ULong length)
{
    int32_t err, e;
    ULong old;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
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
int32_t IndirectAppendData(void **handle, const void *data, ULong length)
{
    int32_t err, e;
    ULong old;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
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
int32_t IndirectReplaceData(void **handle, const void *data, ULong length)
{
    int32_t err, e;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
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
int32_t IndirectSpliceData(void **handle, ULong offset, ULong oldLength, const void *data,
                  ULong newLength)
{
    int32_t err, e;
    ULong size;
    ULong len;

    err = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    size = IndirectGetSize(handle);
    len = newLength;
    if (offset + oldLength > size) {
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
                 (char *)*handle + offset + oldLength, size - (offset + oldLength));
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
int32_t IndirectCompress(void **handle, ULong tag, PackInfo *info)
{
    int32_t err;
    void **out;
    unsigned char locked;
    unsigned char zopen;
    unsigned char finished;
    z_stream z;
    int zerr;
    int32_t e;
    ULong size, pos, outPos, run, cap, n, i;
    unsigned int a, b, a1, b1, b2;
    unsigned short width;
    unsigned char *src;
    unsigned char repeat;

    err = 0;
    out = 0;
    locked = 0;
    zopen = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (info != 0)
        MemoryClear(info, sizeof(PackInfo));
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    locked = 1;
    if (IndirectGetSize(handle) == 0)
        tag = kTagNone;
    else if (tag == kTagAny)
        tag = kTagZlib;
    else if (tag == kTagBest)
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
                    ((unsigned char *)*out)[outPos++] = 0x101 - i;
                    ((unsigned char *)*out)[outPos++] = a;
                    if (width == 2)
                        ((unsigned char *)*out)[outPos++] = b;
                    run -= i;
                }
            } else {
                if (width == 1) {
                    i = a1 = ~a;
                    for (run = 1; run < kMaxRun && pos + run < size; run++) {
                        if (a == a1 && a == i)
                            break;
                        i = a1;
                        a1 = a;
                        a = src[pos + run];
                    }
                } else {
                    i = a1 = ~a;
                    b2 = b1 = ~b;
                    for (run = 1; run < kMaxRun && pos + run * 2 < size; run++) {
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
                n = run * width;
                if (!(cap - outPos >= n + 1)) {
                    cap += kGrowStep;
                    e = IndirectSetSize(out, cap);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                }
                ((unsigned char *)*out)[outPos++] = run - 1;
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
int32_t IndirectDecompress(void **handle, ULong tag, PackInfo *info)
#ifdef MAC1010
#include "../runtime/indirectdecompress1010.inc"
#else
{
    int32_t err;
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
    int32_t r;

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
#endif

#ifndef MAC1010
/* _IndirectValidate. STATE: MATCH 100%, exe 70 / obj 70, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ff774
int32_t _IndirectValidate(void **handle)
{
    int32_t err;
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
#endif
