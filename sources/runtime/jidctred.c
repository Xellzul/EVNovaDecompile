// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jidctred.c -- IJG libjpeg 6b jidctred.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"

#define CONST_BITS  13
#define PASS1_BITS  2

#define FIX_0_211164243  ((INT32)  1730)
#define FIX_0_509795579  ((INT32)  4176)
#define FIX_0_601344887  ((INT32)  4926)
#define FIX_0_720959822  ((INT32)  5906)
#define FIX_0_765366865  ((INT32)  6270)
#define FIX_0_850430095  ((INT32)  6967)
#define FIX_0_899976223  ((INT32)  7373)
#define FIX_1_061594337  ((INT32)  8697)
#define FIX_1_272758580  ((INT32)  10426)
#define FIX_1_451774981  ((INT32)  11893)
#define FIX_1_847759065  ((INT32)  15137)
#define FIX_2_172734803  ((INT32)  17799)
#define FIX_2_562915447  ((INT32)  20995)
#define FIX_3_624509785  ((INT32)  29692)

#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)

#define DEQUANTIZE(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))

/* jpeg_idct_4x4. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00568e23
GLOBAL(void)
jpeg_idct_4x4 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp2, tmp10, tmp12;
  INT32 z1, z2, z3, z4;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE*4];
  SHIFT_TEMPS

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--) {

    if (ctr == DCTSIZE-4)
      continue;
    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*2] == 0 &&
	inptr[DCTSIZE*3] == 0 && inptr[DCTSIZE*5] == 0 &&
	inptr[DCTSIZE*6] == 0 && inptr[DCTSIZE*7] == 0) {

      int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;
      wsptr[DCTSIZE*2] = dcval;
      wsptr[DCTSIZE*3] = dcval;

      continue;
    }

    tmp0 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp0 <<= (CONST_BITS+1);

    z2 = DEQUANTIZE(inptr[DCTSIZE*2], quantptr[DCTSIZE*2]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*6], quantptr[DCTSIZE*6]);

    tmp2 = MULTIPLY(z2, FIX_1_847759065) + MULTIPLY(z3, - FIX_0_765366865);

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    z1 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    z2 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    z3 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    z4 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);

    tmp0 = MULTIPLY(z1, - FIX_0_211164243)
	 + MULTIPLY(z2, FIX_1_451774981)
	 + MULTIPLY(z3, - FIX_2_172734803)
	 + MULTIPLY(z4, FIX_1_061594337);

    tmp2 = MULTIPLY(z1, - FIX_0_509795579)
	 + MULTIPLY(z2, - FIX_0_601344887)
	 + MULTIPLY(z3, FIX_0_899976223)
	 + MULTIPLY(z4, FIX_2_562915447);

    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp2, CONST_BITS-PASS1_BITS+1);
    wsptr[DCTSIZE*3] = (int) DESCALE(tmp10 - tmp2, CONST_BITS-PASS1_BITS+1);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp12 + tmp0, CONST_BITS-PASS1_BITS+1);
    wsptr[DCTSIZE*2] = (int) DESCALE(tmp12 - tmp0, CONST_BITS-PASS1_BITS+1);
  }

  wsptr = workspace;
  for (ctr = 0; ctr < 4; ctr++) {
    outptr = output_buf[ctr] + output_col;

    if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 &&
	wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0) {

      JSAMPLE dcval = range_limit[(int) DESCALE((INT32) wsptr[0], PASS1_BITS+3)
				  & RANGE_MASK];

      outptr[0] = dcval;
      outptr[1] = dcval;
      outptr[2] = dcval;
      outptr[3] = dcval;

      wsptr += DCTSIZE;
      continue;
    }

    tmp0 = ((INT32) wsptr[0]) << (CONST_BITS+1);

    tmp2 = MULTIPLY((INT32) wsptr[2], FIX_1_847759065)
	 + MULTIPLY((INT32) wsptr[6], - FIX_0_765366865);

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    z1 = (INT32) wsptr[7];
    z2 = (INT32) wsptr[5];
    z3 = (INT32) wsptr[3];
    z4 = (INT32) wsptr[1];

    tmp0 = MULTIPLY(z1, - FIX_0_211164243)
	 + MULTIPLY(z2, FIX_1_451774981)
	 + MULTIPLY(z3, - FIX_2_172734803)
	 + MULTIPLY(z4, FIX_1_061594337);

    tmp2 = MULTIPLY(z1, - FIX_0_509795579)
	 + MULTIPLY(z2, - FIX_0_601344887)
	 + MULTIPLY(z3, FIX_0_899976223)
	 + MULTIPLY(z4, FIX_2_562915447);

    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp2,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    outptr[3] = range_limit[(int) DESCALE(tmp10 - tmp2,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp12 + tmp0,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];
    outptr[2] = range_limit[(int) DESCALE(tmp12 - tmp0,
					  CONST_BITS+PASS1_BITS+3+1)
			    & RANGE_MASK];

    wsptr += DCTSIZE;
  }
}

/* jpeg_idct_2x2. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00568c40
GLOBAL(void)
jpeg_idct_2x2 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  INT32 tmp0, tmp10, z1;
  JCOEFPTR inptr;
  ISLOW_MULT_TYPE * quantptr;
  int * wsptr;
  JSAMPROW outptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  int ctr;
  int workspace[DCTSIZE*2];
  SHIFT_TEMPS

  inptr = coef_block;
  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  wsptr = workspace;
  for (ctr = DCTSIZE; ctr > 0; inptr++, quantptr++, wsptr++, ctr--) {

    if (ctr == DCTSIZE-2 || ctr == DCTSIZE-4 || ctr == DCTSIZE-6)
      continue;
    if (inptr[DCTSIZE*1] == 0 && inptr[DCTSIZE*3] == 0 &&
	inptr[DCTSIZE*5] == 0 && inptr[DCTSIZE*7] == 0) {

      int dcval = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]) << PASS1_BITS;

      wsptr[DCTSIZE*0] = dcval;
      wsptr[DCTSIZE*1] = dcval;

      continue;
    }

    z1 = DEQUANTIZE(inptr[DCTSIZE*0], quantptr[DCTSIZE*0]);
    tmp10 = z1 << (CONST_BITS+2);

    z1 = DEQUANTIZE(inptr[DCTSIZE*7], quantptr[DCTSIZE*7]);
    tmp0 = MULTIPLY(z1, - FIX_0_720959822);
    z1 = DEQUANTIZE(inptr[DCTSIZE*5], quantptr[DCTSIZE*5]);
    tmp0 += MULTIPLY(z1, FIX_0_850430095);
    z1 = DEQUANTIZE(inptr[DCTSIZE*3], quantptr[DCTSIZE*3]);
    tmp0 += MULTIPLY(z1, - FIX_1_272758580);
    z1 = DEQUANTIZE(inptr[DCTSIZE*1], quantptr[DCTSIZE*1]);
    tmp0 += MULTIPLY(z1, FIX_3_624509785);

    wsptr[DCTSIZE*0] = (int) DESCALE(tmp10 + tmp0, CONST_BITS-PASS1_BITS+2);
    wsptr[DCTSIZE*1] = (int) DESCALE(tmp10 - tmp0, CONST_BITS-PASS1_BITS+2);
  }

  wsptr = workspace;
  for (ctr = 0; ctr < 2; ctr++) {
    outptr = output_buf[ctr] + output_col;

    if (wsptr[1] == 0 && wsptr[3] == 0 && wsptr[5] == 0 && wsptr[7] == 0) {

      JSAMPLE dcval = range_limit[(int) DESCALE((INT32) wsptr[0], PASS1_BITS+3)
				  & RANGE_MASK];

      outptr[0] = dcval;
      outptr[1] = dcval;

      wsptr += DCTSIZE;
      continue;
    }

    tmp10 = ((INT32) wsptr[0]) << (CONST_BITS+2);

    tmp0 = MULTIPLY((INT32) wsptr[7], - FIX_0_720959822)
	 + MULTIPLY((INT32) wsptr[5], FIX_0_850430095)
	 + MULTIPLY((INT32) wsptr[3], - FIX_1_272758580)
	 + MULTIPLY((INT32) wsptr[1], FIX_3_624509785);

    outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp0,
					  CONST_BITS+PASS1_BITS+3+2)
			    & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp10 - tmp0,
					  CONST_BITS+PASS1_BITS+3+2)
			    & RANGE_MASK];

    wsptr += DCTSIZE;
  }
}

/* jpeg_idct_1x1. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00568bf0
GLOBAL(void)
jpeg_idct_1x1 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
	       JCOEFPTR coef_block,
	       JSAMPARRAY output_buf, JDIMENSION output_col)
{
  int dcval;
  ISLOW_MULT_TYPE * quantptr;
  JSAMPLE *range_limit = IDCT_range_limit(cinfo);
  SHIFT_TEMPS

  quantptr = (ISLOW_MULT_TYPE *) compptr->dct_table;
  dcval = DEQUANTIZE(coef_block[0], quantptr[0]);
  dcval = (int) DESCALE((INT32) dcval, 3);

  output_buf[0][output_col] = range_limit[dcval & RANGE_MASK];
}
