// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jcomapi.c -- IJG libjpeg 6b jcomapi.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

/* jpeg_abort. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054dd01
GLOBAL(void)
jpeg_abort (j_common_ptr cinfo)
{
  int pool;

  if (cinfo->mem == NULL)
    return;

  for (pool = JPOOL_NUMPOOLS-1; pool > JPOOL_PERMANENT; pool--) {
    (*cinfo->mem->free_pool) (cinfo, pool);
  }

  if (cinfo->is_decompressor) {
    cinfo->global_state = DSTATE_START;

    ((j_decompress_ptr) cinfo)->marker_list = NULL;
  } else {
    cinfo->global_state = CSTATE_START;
  }
}

/* jpeg_destroy. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054dce2
GLOBAL(void)
jpeg_destroy (j_common_ptr cinfo)
{

  if (cinfo->mem != NULL)
    (*cinfo->mem->self_destruct) (cinfo);
  cinfo->mem = NULL;
  cinfo->global_state = 0;
}

/* jpeg_alloc_quant_table. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054dcc3
GLOBAL(JQUANT_TBL *)
jpeg_alloc_quant_table (j_common_ptr cinfo)
{
  JQUANT_TBL *tbl;

  tbl = (JQUANT_TBL *)
    (*cinfo->mem->alloc_small) (cinfo, JPOOL_PERMANENT, SIZEOF(JQUANT_TBL));
  tbl->sent_table = FALSE;
  return tbl;
}

/* jpeg_alloc_huff_table. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054dca4
GLOBAL(JHUFF_TBL *)
jpeg_alloc_huff_table (j_common_ptr cinfo)
{
  JHUFF_TBL *tbl;

  tbl = (JHUFF_TBL *)
    (*cinfo->mem->alloc_small) (cinfo, JPOOL_PERMANENT, SIZEOF(JHUFF_TBL));
  tbl->sent_table = FALSE;
  return tbl;
}
