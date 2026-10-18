// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jddctmgr.c -- IJG libjpeg 6b jddctmgr.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"

typedef struct {
  struct jpeg_inverse_dct pub;

  int cur_method[MAX_COMPONENTS];
} my_idct_controller;

typedef my_idct_controller * my_idct_ptr;

typedef union {
  ISLOW_MULT_TYPE islow_array[DCTSIZE2];
  IFAST_MULT_TYPE ifast_array[DCTSIZE2];
  FLOAT_MULT_TYPE float_array[DCTSIZE2];
} multiplier_table;

#define PROVIDE_ISLOW_TABLES

/* start_pass. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0056475f
METHODDEF(void)
start_pass (j_decompress_ptr cinfo)
{
  my_idct_ptr idct = (my_idct_ptr) cinfo->idct;
  int ci, i;
  jpeg_component_info *compptr;
  int method = 0;
  inverse_DCT_method_ptr method_ptr = NULL;
  JQUANT_TBL * qtbl;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {

    switch (compptr->DCT_scaled_size) {
    case 1:
      method_ptr = jpeg_idct_1x1;
      method = JDCT_ISLOW;
      break;
    case 2:
      method_ptr = jpeg_idct_2x2;
      method = JDCT_ISLOW;
      break;
    case 4:
      method_ptr = jpeg_idct_4x4;
      method = JDCT_ISLOW;
      break;
    case DCTSIZE:
      switch (cinfo->dct_method) {
      case JDCT_ISLOW:
	method_ptr = jpeg_idct_islow;
	method = JDCT_ISLOW;
	break;
      case JDCT_IFAST:
	method_ptr = jpeg_idct_ifast;
	method = JDCT_IFAST;
	break;
      case JDCT_FLOAT:
	method_ptr = jpeg_idct_float;
	method = JDCT_FLOAT;
	break;
      default:
	ERREXIT(cinfo, JERR_NOT_COMPILED);
	break;
      }
      break;
    default:
      ERREXIT1(cinfo, JERR_BAD_DCTSIZE, compptr->DCT_scaled_size);
      break;
    }
    idct->pub.inverse_DCT[ci] = method_ptr;

    if (! compptr->component_needed || idct->cur_method[ci] == method)
      continue;
    qtbl = compptr->quant_table;
    if (qtbl == NULL)
      continue;
    idct->cur_method[ci] = method;
    switch (method) {
    case JDCT_ISLOW:
      {

	ISLOW_MULT_TYPE * ismtbl = (ISLOW_MULT_TYPE *) compptr->dct_table;
	for (i = 0; i < DCTSIZE2; i++) {
	  ismtbl[i] = (ISLOW_MULT_TYPE) qtbl->quantval[i];
	}
      }
      break;
    case JDCT_IFAST:
      {

	IFAST_MULT_TYPE * ifmtbl = (IFAST_MULT_TYPE *) compptr->dct_table;
#define CONST_BITS 14
	static const INT16 aanscales[DCTSIZE2] = {

	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	SHIFT_TEMPS

	for (i = 0; i < DCTSIZE2; i++) {
	  ifmtbl[i] = (IFAST_MULT_TYPE)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST_BITS-IFAST_SCALE_BITS);
	}
      }
      break;
    case JDCT_FLOAT:
      {

	FLOAT_MULT_TYPE * fmtbl = (FLOAT_MULT_TYPE *) compptr->dct_table;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fmtbl[i] = (FLOAT_MULT_TYPE)
	      ((double) qtbl->quantval[i] *
	       aanscalefactor[row] * aanscalefactor[col]);
	    i++;
	  }
	}
      }
      break;
    default:
      ERREXIT(cinfo, JERR_NOT_COMPILED);
      break;
    }
  }
}

/* jinit_inverse_dct. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005646e3
GLOBAL(void)
jinit_inverse_dct (j_decompress_ptr cinfo)
{
  my_idct_ptr idct;
  int ci;
  jpeg_component_info *compptr;

  idct = (my_idct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_idct_controller));
  cinfo->idct = (struct jpeg_inverse_dct *) idct;
  idct->pub.start_pass = start_pass;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {

    compptr->dct_table =
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(multiplier_table));
    MEMZERO(compptr->dct_table, SIZEOF(multiplier_table));

    idct->cur_method[ci] = -1;
  }
}
