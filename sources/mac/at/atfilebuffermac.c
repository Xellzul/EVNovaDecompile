/* atfilebuffermac.c -- 1.0.10: the file tool: files over memory handles. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "atfiletoolmac.h"

extern FileProc gFT_BufferHooks[6];

int32_t _FT_BufferOpen(void **handle, unsigned char mode, FTFile *file);

int32_t _FT_BufferOpenWrapper(void **handle, unsigned char mode, FTFile **result)
{
    int32_t err, e;

    err = 0;
    if ((*result = _FT_IsBufferOpen(handle)) != 0) {
        _FT_FileUpdateRefCount(*result, 1);
    } else {
        if ((*result = MemoryAllocate(sizeof(FTFile))) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        memset(*result, 0, sizeof(FTFile));
        e = _FT_BufferOpen(handle, mode, *result);
        if (e != 0) {
            err = e;
            goto done;
        }
        (*result)->link = gFT_FileList;
        gFT_FileList.next = *result;
        _FT_FileUpdateRefCount(*result, 1);
    }
done:
    if (err != 0 && *result != 0) {
        MemoryDeallocate(*result);
        *result = 0;
    }
    return err;
}

FTFile *_FT_IsBufferOpen(void **handle)
{
    FTFile *file;
    FTLink *link;
    int32_t err;

    err = 0;
    link = 0;
    link = &gFT_FileList;
    while ((file = link->next) != 0 && file->memHandle != handle)
        link = &file->link;
    return link != 0 ? file : 0;
}

int32_t _FT_BufferOpen(void **handle, unsigned char mode, FTFile *file)
{
    int32_t err;

    err = 0;
    file->memHandle = handle;
    file->position = 0;
    file->size = IndirectGetSize(file->memHandle);
    file->mode = mode;
    file->procs = *(FTProcs *)gFT_BufferHooks;
    return 0;
}

int32_t _FT_BufferClose(FTFile *file)
{
    int32_t err;

    err = 0;
    file->memHandle = 0;
    return 0;
}

int32_t _FT_BufferFlush(FTFile *file)
{
    int32_t err;

    err = 0;
    return 0;
}

int32_t _FT_BufferRead(FTFile *file, ULong count, void *buffer)
{
    char *src;
    int32_t err;

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

int32_t _FT_BufferWrite(FTFile *file, ULong count, const void *buffer)
{
    char *dst;
    int32_t err;
    int32_t result;

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

int32_t _FT_BufferGetEOF(FTFile *file, ULong *size)
{
    int32_t err;

    err = 0;
    *size = IndirectGetSize(file->memHandle);
    return 0;
}

int32_t _FT_BufferSetEOF(FTFile *file, ULong size)
{
    int32_t err;
    int32_t result;

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

int32_t _FT_BufferGetMark(FTFile *file, ULong *position)
{
    int32_t err;

    err = 0;
    *position = file->position;
    return 0;
}

int32_t _FT_BufferSetMark(FTFile *file, ULong position)
{
    int32_t err;

    err = 0;
    file->position = position;
    return 0;
}
