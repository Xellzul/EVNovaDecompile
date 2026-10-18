// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jutils.c -- IJG libjpeg 6b jutils.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

const int jpeg_natural_order[DCTSIZE2+16] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
 63, 63, 63, 63, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 63, 63, 63
};

/* jdiv_round_up. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054eece
GLOBAL(long)
jdiv_round_up (long a, long b)

{
  return (a + b - 1L) / b;
}

/* jround_up. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054eeb3
GLOBAL(long)
jround_up (long a, long b)

{
  a += b - 1L;
  return a - (a % b);
}

#define FMEMCOPY(dest,src,size)	MEMCOPY(dest,src,size)
#define FMEMZERO(target,size)	MEMZERO(target,size)

/* jcopy_sample_rows. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054ee6a
GLOBAL(void)
jcopy_sample_rows (JSAMPARRAY input_array, int source_row,
		   JSAMPARRAY output_array, int dest_row,
		   int num_rows, JDIMENSION num_cols)

{
  register JSAMPROW inptr, outptr;
  register size_t count = (size_t) (num_cols * SIZEOF(JSAMPLE));
  register int row;

  input_array += source_row;
  output_array += dest_row;

  for (row = num_rows; row > 0; row--) {
    inptr = *input_array++;
    outptr = *output_array++;
    FMEMCOPY(outptr, inptr, count);
  }
}

/* jcopy_block_row. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054ee51
GLOBAL(void)
jcopy_block_row (JBLOCKROW input_row, JBLOCKROW output_row,
		 JDIMENSION num_blocks)

{
  FMEMCOPY(output_row, input_row, num_blocks * (DCTSIZE2 * SIZEOF(JCOEF)));
}

/* jzero_far. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054ee3e
GLOBAL(void)
jzero_far (void FAR * target, size_t bytestozero)

{
  FMEMZERO(target, bytestozero);
}
