// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jidctflt.c -- IJG libjpeg 6b jidctflt.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"

#define DEQUANTIZE(coef,quantval)  (((FAST_FLOAT) (coef)) * (quantval))

/* jpeg_idct_float. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00569c9c
GLOBAL(void)
jpeg_idct_float (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
  FAST_FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  FAST_FLOAT tmp10, tmp11, tmp12, tmp13;
  FAST_FLOAT z5, z10, z11, z12, z13;
  JCOEFPTR inptr;
  FLOAT_MULT_TYPE * quantptr;
  FAST_FLOAT * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  FAST_FLOAT workspace[DCTSIZE2];
  SHIFT_TEMPS

  inptr = coef_block;
  quantptr = (FLOAT_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; ctr--) {

    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
	inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*4] == 0 &&
	inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*6] == 0 &&
	inptr[DCTSIZE*7] == 0) {

      FAST_FLOAT dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;
      wsptr[DCTSIZE*4] = dcval;
      wsptr[DCTSIZE*5] = dcval;
      wsptr[DCTSIZE*6] = dcval;
      wsptr[DCTSIZE*7] = dcval;

      inptr++;
      quantptr++;
      wsptr++;
      continue;
    }

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp1 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    tmp2 = DEQUANTIZE(inptr[DCTSIZE*4], quantptr[DCTSIZE*4]);
    tmp3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp10 = tmp0 + tmp2;
    tmp11 = tmp0 - tmp2;

    tmp13 = tmp1 + tmp3;
    tmp12 = (tmp1 - tmp3) * ((FAST_FLOAT) 1.414213562) - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    tmp4 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    tmp5 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp6 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp7 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);

    z13 = tmp6 + tmp5;
    z10 = tmp6 - tmp5;
    z11 = tmp4 + tmp7;
    z12 = tmp4 - tmp7;

    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * ((FAST_FLOAT) 1.414213562);

    z5 = (z10 + z12) * ((FAST_FLOAT) 1.847759065);
    tmp10 = ((FAST_FLOAT) 1.082392200) * z12 - z5;
    tmp12 = ((FAST_FLOAT) -2.613125930) * z10 + z5;

    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 + tmp5;

    wsptr[DCTSIZE*0] = tmp0 + tmp7;
    wsptr[DCTSIZE*7] = tmp0 - tmp7;
    wsptr[DCTSIZE*1] = tmp1 + tmp6;
    wsptr[DCTSIZE*6] = tmp1 - tmp6;
    wsptr[DCTSIZE*2] = tmp2 + tmp5;
    wsptr[DCTSIZE*5] = tmp2 - tmp5;
    wsptr[DCTSIZE*4] = tmp3 + tmp4;
    wsptr[DCTSIZE*3] = tmp3 - tmp4;

    inptr++;
    quantptr++;
    wsptr++;
  }

  wsptr = workspace;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    outptr = output_buf[ctr] + output_col;

    tmp10 = wsptr[0] + wsptr[4];
    tmp11 = wsptr[0] - wsptr[4];

    tmp13 = wsptr[2] + wsptr[6];
    tmp12 = (wsptr[2] - wsptr[6]) * ((FAST_FLOAT) 1.414213562) - tmp13;

    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;

    z13 = wsptr[5] + wsptr[3];
    z10 = wsptr[5] - wsptr[3];
    z11 = wsptr[1] + wsptr[7];
    z12 = wsptr[1] - wsptr[7];

    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * ((FAST_FLOAT) 1.414213562);

    z5 = (z10 + z12) * ((FAST_FLOAT) 1.847759065);
    tmp10 = ((FAST_FLOAT) 1.082392200) * z12 - z5;
    tmp12 = ((FAST_FLOAT) -2.613125930) * z10 + z5;

    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 + tmp5;

    outptr[0] = range_limit[(int) DESCALE((INT32) (tmp0 + tmp7), 3)
			    & RANGE_MASK];
    outptr[7] = range_limit[(int) DESCALE((INT32) (tmp0 - tmp7), 3)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE((INT32) (tmp1 + tmp6), 3)
			    & RANGE_MASK];
    outptr[6] = range_limit[(int) DESCALE((INT32) (tmp1 - tmp6), 3)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE((INT32) (tmp2 + tmp5), 3)
			    & RANGE_MASK];
    outptr[5] = range_limit[(int) DESCALE((INT32) (tmp2 - tmp5), 3)
			    & RANGE_MASK];
    outptr[4] = range_limit[(int) DESCALE((INT32) (tmp3 + tmp4), 3)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE((INT32) (tmp3 - tmp4), 3)
			    & RANGE_MASK];

    wsptr += DCTSIZE;
  }
}
