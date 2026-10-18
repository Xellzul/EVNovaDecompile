/* jmorecfg.h -- IJG libjpeg 6b jmorecfg.h, Copyright (C) 1991-1998, Thomas G. Lane; licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */
#define BITS_IN_JSAMPLE  8

#define MAX_COMPONENTS  10

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	255
#define CENTERJSAMPLE	128

typedef short JCOEF;

typedef unsigned char JOCTET;
#define GETJOCTET(value)  (value)

typedef unsigned char UINT8;

typedef unsigned short UINT16;

typedef short INT16;

typedef long INT32;

typedef unsigned int JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L

#define METHODDEF(type)		static type

#define LOCAL(type)		static type

#define GLOBAL(type)		type

#define EXTERN(type)		extern type

#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist

#define FAR

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

#ifdef JPEG_INTERNALS
#define JPEG_INTERNAL_OPTIONS
#endif

#ifdef JPEG_INTERNAL_OPTIONS

#define DCT_ISLOW_SUPPORTED
#define DCT_IFAST_SUPPORTED
#define DCT_FLOAT_SUPPORTED

#undef  C_ARITH_CODING_SUPPORTED
#define C_MULTISCAN_FILES_SUPPORTED
#define C_PROGRESSIVE_SUPPORTED
#define ENTROPY_OPT_SUPPORTED

#define INPUT_SMOOTHING_SUPPORTED

#undef  D_ARITH_CODING_SUPPORTED
#define D_MULTISCAN_FILES_SUPPORTED
#define D_PROGRESSIVE_SUPPORTED
#define SAVE_MARKERS_SUPPORTED
#define BLOCK_SMOOTHING_SUPPORTED
#define IDCT_SCALING_SUPPORTED
#undef  UPSAMPLE_SCALING_SUPPORTED
#define UPSAMPLE_MERGING_SUPPORTED
#define QUANT_1PASS_SUPPORTED
#define QUANT_2PASS_SUPPORTED

#define RGB_RED		0
#define RGB_GREEN	1
#define RGB_BLUE	2
#define RGB_PIXELSIZE	3

#define INLINE

#define MULTIPLIER  int

#define FAST_FLOAT  float

#endif
