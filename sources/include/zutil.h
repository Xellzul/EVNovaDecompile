/* zutil.h -- zlib 1.2.1 internal interface, Copyright (C) 1995-2003 Jean-loup Gailly; zlib
   licence in zlib.h. Altered: the Win32 path only; memory through zlibcrt.c. */

#ifndef ZUTIL_H
#define ZUTIL_H

#define ZLIB_INTERNAL
#include "zlib.h"

#include <stddef.h>

#ifndef local
#  define local static
#endif

typedef unsigned char  uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long  ulg;

extern const char * const z_errmsg[10];

#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]

#define ERR_RETURN(strm,err) \
  return (strm->msg = (char*)ERR_MSG(err), (err))

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2

#define MIN_MATCH  3
#define MAX_MATCH  258

#define PRESET_DICT 0x20

#define OS_CODE  0x0b

void *zlib_memcpy OF((void *dest, const void *source, size_t len));
void *zlib_memset OF((void *dest, int c, size_t len));
void *zlib_malloc OF((size_t size));
void  zlib_free   OF((void *ptr));

#define zmemcpy zlib_memcpy
#define zmemzero(dest, len) zlib_memset(dest, 0, len)

#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

voidpf zcalloc OF((voidpf opaque, unsigned items, unsigned size));
void   zcfree  OF((voidpf opaque, voidpf ptr));

#define ZALLOC(strm, items, size) \
           (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}

#endif
