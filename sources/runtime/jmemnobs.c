// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jmemnobs.c -- IJG libjpeg 6b jmemnobs.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"

#ifndef HAVE_STDLIB_H
extern void * malloc JPP((size_t size));
extern void free JPP((void *ptr));
#endif

/* jpeg_get_small. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005614d7
GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}

/* jpeg_free_small. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005614cc
GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  free(object);
}

/* jpeg_get_large. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005614c1
GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void FAR *) malloc(sizeofobject);
}

/* jpeg_free_large. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005614b6
GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  free(object);
}

/* jpeg_mem_available. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005614b1
GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
		    long max_bytes_needed, long already_allocated)
{
  return max_bytes_needed;
}

/* jpeg_open_backing_store. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0056149b
GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
  ERREXIT(cinfo, JERR_NO_BACKING_STORE);
}

/* jpeg_mem_init. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00561498
GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
  return 0;
}

/* jpeg_mem_term. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00561497
GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{

}
