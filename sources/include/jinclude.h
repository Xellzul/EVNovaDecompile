/* jinclude.h -- IJG libjpeg 6b jinclude.h, Copyright (C) 1991-1998, Thomas G. Lane; licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */
#include "jconfig.h"
#define JCONFIG_INCLUDED

#include <stddef.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>
#define MEMZERO(target,size)	jpegcrt_memset((void *)(target), 0, (size_t)(size))
#define MEMCOPY(dest,src,size)	jpegcrt_memcpy((void *)(dest), (const void *)(src), (size_t)(size))

extern void *jpegcrt_memset(void *dest, int c, size_t len);
extern void *jpegcrt_memcpy(void *dest, const void *source, size_t len);
extern void jpegcrt_free(void *ptr);
extern void *jpegcrt_malloc(size_t size);
#define malloc jpegcrt_malloc
#define free   jpegcrt_free

#define SIZEOF(object)	((size_t) sizeof(object))

#define JFREAD(file,buf,sizeofbuf)  \
  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))
#define JFWRITE(file,buf,sizeofbuf)  \
  ((size_t) fwrite((const void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))
