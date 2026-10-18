/* jdct.h -- IJG libjpeg 6b jdct.h, Copyright (C) 1991-1998, Thomas G. Lane; licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */
typedef int DCTELEM;

typedef JMETHOD(void, forward_DCT_method_ptr, (DCTELEM * data));
typedef JMETHOD(void, float_DCT_method_ptr, (FAST_FLOAT * data));

typedef MULTIPLIER ISLOW_MULT_TYPE;
typedef MULTIPLIER IFAST_MULT_TYPE;
#define IFAST_SCALE_BITS  2
typedef FAST_FLOAT FLOAT_MULT_TYPE;

#define IDCT_range_limit(cinfo)  ((cinfo)->sample_range_limit + CENTERJSAMPLE)

#define RANGE_MASK  (MAXJSAMPLE * 4 + 3)

EXTERN(void) jpeg_fdct_islow JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_ifast JPP((DCTELEM * data));
EXTERN(void) jpeg_fdct_float JPP((FAST_FLOAT * data));

EXTERN(void) jpeg_idct_islow
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_ifast
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_float
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x4
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_1x1
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

#define ONE	((INT32) 1)
#define CONST_SCALE (ONE << CONST_BITS)

#define FIX(x)	((INT32) ((x) * CONST_SCALE + 0.5))

#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)

#define MULTIPLY16C16(var,const)  ((var) * (const))

#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))
