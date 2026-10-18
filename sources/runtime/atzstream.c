// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -g -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; -g = this member built with debug symbols (CodeWarrior's per-file debug checkbox; it adds one IR position per source line, and the 335 scorer gives EAX only at an IR gap >= 3 between the two zero stores, here exactly 2 apart), as libpng png_free_data (0x0053711c, a -g object) is byte-identical in the same idiom; across the AmbrosiaTools files -g gains only here and loses MATCHes elsewhere, so it is this file's setting alone; provisional (Research/notes/atzstream.md); Research/runtime-compiler-ceiling.md
/* atzstream.c -- a handle deflated or inflated in place through a zlib stream that persists between calls (the REGGIE client's TCP data). Notes: Research/notes/atzstream.md */

#include "zlib.h"
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "macport.h"

#define kDeflating      0x87654321
#define kInflating      0x23456789
#define kBroken         0x1be50bad
#define kWrongState     -7001
#define kChunk          0x1000

/* ProgressiveDispose (end a stream state). STATE: MATCH 100%, exe 34 / obj 34, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0052bbe4
void ProgressiveDispose(ZState *state)
{
    long err;

    err = 0;
    if (state->magic == kDeflating && state->strm != 0) {
        deflateEnd(state->strm);
        MemoryDeallocate(state->strm);
    } else if (state->magic == kInflating && state->strm != 0) {
        inflateEnd(state->strm);
        MemoryDeallocate(state->strm);
    }
    MemoryClear(state, sizeof(ZState));
}

/* ProgressiveDecompress (inflate a handle in place, the stream kept in state). STATE: MATCH 100%
   (with the file's `-g`), exe 225 / obj 225, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052bc39
long ProgressiveDecompress(void **h, ZState *state)
{
    void **out;
    z_stream *strm;
    ULong size;
    long ret;
    long err;
    long e;

    err = 0;
    out = 0;
    strm = 0;
    if (state != 0 && state->magic != 0 && state->magic != kInflating) {
        err = kWrongState;
        goto done;
    }
    if (state->magic == 0) {
        MemoryClear(state, sizeof(ZState));
        state->magic = kInflating;
        strm = MemoryAllocate(sizeof(z_stream));
        if (strm == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryClear(strm, sizeof(z_stream));
        ret = inflateInit(strm);
        switch (ret) {
        case Z_OK:
            state->strm = strm;
            break;
        case Z_MEM_ERROR:
            err = ret;
            err = kOutOfMemory;
            goto done;
        case Z_VERSION_ERROR:
        default:
            err = ret;
            err = kBadArgument;
            goto done;
        }
    } else {
        strm = state->strm;
    }

    size = IndirectGetSize(h) * 2;
    if (size < kChunk)
        size = kChunk;
    out = IndirectAllocate(size);
    if (out == 0) {
        err = kOutOfMemory;
        goto done;
    }
    IndirectSetLock(out, 1);
    IndirectSetLock(h, 1);
    strm->total_in = 0;
    strm->total_out = 0;
    strm->next_in = *h;
    strm->avail_in = IndirectGetSize(h);
    strm->next_out = *out;
    strm->avail_out = IndirectGetSize(out);
    /* BUG: ret is read uninitialised when the state already existed */
    while (strm->avail_in != 0 && ret != Z_STREAM_END) {
        ret = inflate(strm, Z_SYNC_FLUSH);
        switch (ret) {
        case Z_OK:
        case Z_STREAM_END:
            break;
        case Z_STREAM_ERROR:
        case Z_BUF_ERROR:
        default:
            err = ret;
            err = kBadArgument;
            goto done;
        }
        if (ret != Z_STREAM_END) {
            IndirectSetLock(out, 0);
            size = strm->total_out + kChunk;
            e = IndirectSetSize(out, size);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(out, 1);
            strm->next_out = (Bytef *)*out + strm->total_out;
            strm->avail_out = size - strm->total_out;
        }
    }
    IndirectSetLock(h, 0);
    IndirectSetLock(out, 0);
    e = IndirectSetSize(out, strm->total_out);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (out != 0) {
        size = IndirectGetSize(out);
        e = IndirectSetSize(h, size);
        if (e != 0) {
            err = e;
            goto done;
        }
        MemoryCopy(*h, *out, size);
    }
done:
    IndirectSetLock(h, 0);
    if (out != 0)
        IndirectDeallocate(out);
    if (err != 0 && state != 0 && state->magic == kInflating) {
        if (state->strm != 0)
            inflateEnd(state->strm);
        if (strm != 0)
            MemoryDeallocate(strm);
        state->magic = kBroken;
        state->strm = 0;
    }
    return err;
}

/* ProgressiveCompress (deflate a handle in place, the stream kept in state). STATE: MATCH 100%
   (with the file's `-g`), exe 205 / obj 205, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052bea5
long ProgressiveCompress(void **h, ZState *state)
{
    void **out;
    z_stream *strm;
    ULong size;
    unsigned char finished;
    long ret;
    long err;
    long e;

    err = 0;
    out = 0;
    strm = 0;
    finished = 0;
    if (state != 0 && state->magic != 0 && state->magic != kDeflating) {
        err = kWrongState;
        goto done;
    }
    if (state->magic == 0) {
        MemoryClear(state, sizeof(ZState));
        state->magic = kDeflating;
        strm = MemoryAllocate(sizeof(z_stream));
        if (strm == 0) {
            err = kOutOfMemory;
            goto done;
        }
        MemoryClear(strm, sizeof(z_stream));
        ret = deflateInit(strm, Z_DEFAULT_COMPRESSION);
        switch (ret) {
        case Z_OK:
            state->strm = strm;
            break;
        case Z_MEM_ERROR:
            err = ret;
            err = kOutOfMemory;
            goto done;
        case Z_STREAM_ERROR:
        case Z_VERSION_ERROR:
        default:
            err = ret;
            err = kBadArgument;
            goto done;
        }
    } else {
        strm = state->strm;
    }

    out = IndirectAllocate(0);
    if (out == 0) {
        err = kOutOfMemory;
        goto done;
    }
    strm->total_in = 0;
    strm->total_out = 0;
    while (finished == 0) {
        e = IndirectSetSize(out, IndirectGetSize(out) + kChunk);
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(h, 1);
        IndirectSetLock(out, 1);
        strm->next_in = (Bytef *)*h + strm->total_in;
        strm->avail_in = IndirectGetSize(h) - strm->total_in;
        strm->next_out = (Bytef *)*out + strm->total_out;
        strm->avail_out = IndirectGetSize(out) - strm->total_out;
        ret = deflate(strm, Z_SYNC_FLUSH);
        switch (ret) {
        case Z_OK:
            if (strm->avail_in == 0)
                finished = 1;
            break;
        case Z_STREAM_END:
            finished = 1;
            break;
        case Z_STREAM_ERROR:
        case Z_BUF_ERROR:
        default:
            err = ret;
            err = kBadArgument;
            goto done;
        }
        IndirectSetLock(h, 0);
        IndirectSetLock(out, 0);
    }
    e = IndirectSetSize(out, strm->total_out);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (out != 0) {
        size = IndirectGetSize(out);
        e = IndirectSetSize(h, size);
        if (e != 0) {
            err = e;
            goto done;
        }
        MemoryCopy(*h, *out, size);
    }
done:
    if (out != 0)
        IndirectDeallocate(out);
    if (err != 0 && state != 0 && state->magic == kDeflating) {
        if (state->strm != 0)
            deflateEnd(state->strm);
        if (strm != 0)
            MemoryDeallocate(strm);
        state->magic = kBroken;
        state->strm = 0;
    }
    return err;
}
