/* j1010crt.c -- the C-runtime calls the exe's libjpeg 6b links privately, as win's jpegcrt.c. Notes: Research/notes/maclibs.md */

#include <stddef.h>
#include <MacMemory.h>

void *jpegcrt_malloc(size_t size);
void jpegcrt_free(void *ptr);
void *jpegcrt_memcpy(void *dest, const void *source, size_t len);
void *jpegcrt_memset(void *dest, int c, size_t len);

void *jpegcrt_malloc(size_t size)
{
    return size != 0 ? NewPtr(size) : NULL;
}

void jpegcrt_free(void *ptr)
{
    if (ptr != NULL)
        DisposePtr(ptr);
}

void *jpegcrt_memcpy(void *dest, const void *source, size_t len)
{
    BlockMoveData(source, dest, len);
    return dest;
}

void *jpegcrt_memset(void *dest, int c, size_t len)
{
    unsigned char *d = dest;

    while (len--)
        *d++ = (unsigned char)c;
    return dest;
}
