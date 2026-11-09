/* z123_zlibcrt.c -- the C-runtime calls the exe's zlib links privately, as win's zlibcrt.c. Not public zlib text; its licence is zlib's. Notes: Research/notes/maclibs.md */

#include <stddef.h>
#include <MacMemory.h>

void *zlib_malloc(size_t size);
void zlib_free(void *ptr);
void *zlib_memcpy(void *dest, const void *source, size_t len);
void *zlib_memset(void *dest, int c, size_t len);

void *zlib_malloc(size_t size)
{
    return size != 0 ? NewPtr(size) : NULL;
}

void zlib_free(void *ptr)
{
    if (ptr != NULL)
        DisposePtr(ptr);
}

void *zlib_memcpy(void *dest, const void *source, size_t len)
{
    BlockMoveData(source, dest, len);
    return dest;
}

void *zlib_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}
