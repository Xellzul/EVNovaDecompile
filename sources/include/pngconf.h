/* pngconf.h -- libpng 1.2.7 configuration, Copyright (c) 1998-2004 Glenn Randers-Pehrson;
   libpng licence in png.h. Altered: reduced to the configuration the exe was built with
   (PNG_NO_STDIO); png_memcpy and its siblings, malloc, free, strtod and abs through pngcrt.c. Notes:
   Research/notes/png.md */

#ifndef PNGCONF_H
#define PNGCONF_H

#define PNG_NO_STDIO

#define PNG_ZBUF_SIZE 8192

#define PNG_READ_SUPPORTED

#define PNG_WRITE_SUPPORTED

#define PNG_MNG_FEATURES_SUPPORTED

#define PNG_FLOATING_POINT_SUPPORTED

#define PNG_NO_CONSOLE_IO

#define PNGARG(arglist) OF(arglist)

#include <sys/types.h>

#define PNG_SETJMP_SUPPORTED

#include <setjmp.h>

#include <string.h>

#ifdef PNG_INTERNAL

#  include <stdlib.h>

#  define PNG_EXTERN

#  include <math.h>

#  define PNG_ALWAYS_EXTERN

#  define PNG_DITHER_RED_BITS 5
#  define PNG_DITHER_GREEN_BITS 5
#  define PNG_DITHER_BLUE_BITS 5

#  define PNG_MAX_GAMMA_8 11

#  define PNG_GAMMA_THRESHOLD 0.05

#endif

#define PNG_CONST const

#define PNG_NO_READ_iTXt
#define PNG_NO_WRITE_iTXt

#define PNG_FIXED_POINT_SUPPORTED

#define PNG_FREE_ME_SUPPORTED

#define PNG_READ_TRANSFORMS_SUPPORTED

#define PNG_READ_EXPAND_SUPPORTED
#define PNG_READ_SHIFT_SUPPORTED
#define PNG_READ_PACK_SUPPORTED
#define PNG_READ_BGR_SUPPORTED
#define PNG_READ_SWAP_SUPPORTED
#define PNG_READ_PACKSWAP_SUPPORTED
#define PNG_READ_INVERT_SUPPORTED
#define PNG_READ_DITHER_SUPPORTED
#define PNG_READ_BACKGROUND_SUPPORTED
#define PNG_READ_16_TO_8_SUPPORTED
#define PNG_READ_FILLER_SUPPORTED
#define PNG_READ_GAMMA_SUPPORTED
#define PNG_READ_GRAY_TO_RGB_SUPPORTED
#define PNG_READ_SWAP_ALPHA_SUPPORTED
#define PNG_READ_INVERT_ALPHA_SUPPORTED
#define PNG_READ_STRIP_ALPHA_SUPPORTED
#define PNG_READ_USER_TRANSFORM_SUPPORTED
#define PNG_READ_RGB_TO_GRAY_SUPPORTED

#define PNG_PROGRESSIVE_READ_SUPPORTED

#define PNG_READ_INTERLACING_SUPPORTED

#define PNG_READ_COMPOSITE_NODIV_SUPPORTED

#define PNG_READ_EMPTY_PLTE_SUPPORTED

#define PNG_WRITE_TRANSFORMS_SUPPORTED

#define PNG_WRITE_SHIFT_SUPPORTED
#define PNG_WRITE_PACK_SUPPORTED
#define PNG_WRITE_BGR_SUPPORTED
#define PNG_WRITE_SWAP_SUPPORTED
#define PNG_WRITE_PACKSWAP_SUPPORTED
#define PNG_WRITE_INVERT_SUPPORTED
#define PNG_WRITE_FILLER_SUPPORTED
#define PNG_WRITE_SWAP_ALPHA_SUPPORTED
#define PNG_WRITE_INVERT_ALPHA_SUPPORTED
#define PNG_WRITE_USER_TRANSFORM_SUPPORTED

#define PNG_WRITE_INTERLACING_SUPPORTED

#define PNG_WRITE_WEIGHTED_FILTER_SUPPORTED

#define PNG_WRITE_FLUSH_SUPPORTED

#define PNG_WRITE_EMPTY_PLTE_SUPPORTED

#define PNG_ERROR_NUMBERS_SUPPORTED

#define PNG_USER_TRANSFORM_PTR_SUPPORTED

#define PNG_EASY_ACCESS_SUPPORTED

#define PNG_ASSEMBLER_CODE_SUPPORTED
#define PNG_MMX_CODE_SUPPORTED

#define PNG_USER_MEM_SUPPORTED

#define PNG_SET_USER_LIMITS_SUPPORTED

#define PNG_USER_WIDTH_MAX 1000000L
#define PNG_USER_HEIGHT_MAX 1000000L

#define PNG_READ_ANCILLARY_CHUNKS_SUPPORTED

#define PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED

#define PNG_READ_bKGD_SUPPORTED
#define PNG_bKGD_SUPPORTED
#define PNG_READ_cHRM_SUPPORTED
#define PNG_cHRM_SUPPORTED
#define PNG_READ_gAMA_SUPPORTED
#define PNG_gAMA_SUPPORTED
#define PNG_READ_hIST_SUPPORTED
#define PNG_hIST_SUPPORTED
#define PNG_READ_iCCP_SUPPORTED
#define PNG_iCCP_SUPPORTED
#define PNG_READ_oFFs_SUPPORTED
#define PNG_oFFs_SUPPORTED
#define PNG_READ_pCAL_SUPPORTED
#define PNG_pCAL_SUPPORTED
#define PNG_READ_sCAL_SUPPORTED
#define PNG_sCAL_SUPPORTED
#define PNG_READ_pHYs_SUPPORTED
#define PNG_pHYs_SUPPORTED
#define PNG_READ_sBIT_SUPPORTED
#define PNG_sBIT_SUPPORTED
#define PNG_READ_sPLT_SUPPORTED
#define PNG_sPLT_SUPPORTED
#define PNG_READ_sRGB_SUPPORTED
#define PNG_sRGB_SUPPORTED
#define PNG_READ_tEXt_SUPPORTED
#define PNG_tEXt_SUPPORTED
#define PNG_READ_tIME_SUPPORTED
#define PNG_tIME_SUPPORTED
#define PNG_READ_tRNS_SUPPORTED
#define PNG_tRNS_SUPPORTED
#define PNG_READ_zTXt_SUPPORTED
#define PNG_zTXt_SUPPORTED
#define PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#define PNG_READ_USER_CHUNKS_SUPPORTED
#define PNG_USER_CHUNKS_SUPPORTED
#define PNG_READ_OPT_PLTE_SUPPORTED
#define PNG_READ_TEXT_SUPPORTED
#define PNG_TEXT_SUPPORTED

#define PNG_WRITE_bKGD_SUPPORTED
#define PNG_WRITE_cHRM_SUPPORTED
#define PNG_WRITE_gAMA_SUPPORTED
#define PNG_WRITE_hIST_SUPPORTED
#define PNG_WRITE_iCCP_SUPPORTED
#define PNG_WRITE_oFFs_SUPPORTED
#define PNG_WRITE_pCAL_SUPPORTED
#define PNG_WRITE_sCAL_SUPPORTED
#define PNG_WRITE_pHYs_SUPPORTED
#define PNG_WRITE_sBIT_SUPPORTED
#define PNG_WRITE_sPLT_SUPPORTED
#define PNG_WRITE_sRGB_SUPPORTED
#define PNG_WRITE_tEXt_SUPPORTED
#define PNG_WRITE_tIME_SUPPORTED
#define PNG_WRITE_tRNS_SUPPORTED
#define PNG_WRITE_zTXt_SUPPORTED
#define PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
#define PNG_WRITE_TEXT_SUPPORTED

#define PNG_INFO_IMAGE_SUPPORTED

#include <time.h>

typedef unsigned long png_uint_32;
typedef long png_int_32;
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

   typedef size_t png_size_t;
#define png_sizeof(x) sizeof (x)

#define FARDATA

typedef png_int_32 png_fixed_point;

typedef void            FAR * png_voidp;
typedef png_byte        FAR * png_bytep;
typedef png_uint_32     FAR * png_uint_32p;
typedef png_int_32      FAR * png_int_32p;
typedef png_uint_16     FAR * png_uint_16p;
typedef png_int_16      FAR * png_int_16p;
typedef PNG_CONST char  FAR * png_const_charp;
typedef char            FAR * png_charp;
typedef png_fixed_point FAR * png_fixed_point_p;

typedef double          FAR * png_doublep;

typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;
typedef png_fixed_point FAR * FAR * png_fixed_point_pp;
typedef double          FAR * FAR * png_doublepp;

typedef char            FAR * FAR * FAR * png_charppp;

typedef charf *         png_zcharp;
typedef charf * FAR *   png_zcharpp;
typedef z_stream FAR *  png_zstreamp;

#define PNG_USE_GLOBAL_ARRAYS

#define PNGAPI _cdecl

#define PNG_IMPEXP

#define PNG_EXPORT(type,symbol) PNG_IMPEXP type PNGAPI symbol

#define PNG_EXPORT_VAR(type) extern PNG_IMPEXP type

#define PNG_ABORT() abort()

#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)

#define CVT_PTR(ptr)         (ptr)
#define CVT_PTR_NOCHECK(ptr) (ptr)
#define png_strcpy  pngcrt_strcpy
#define png_strncpy strncpy
#define png_strlen  pngcrt_strlen
#define png_memcmp  pngcrt_memcmp
#define png_memcpy  pngcrt_memcpy
#define png_memset  pngcrt_memset

extern void *pngcrt_malloc(size_t size);
extern void pngcrt_free(void *ptr);
extern void *pngcrt_memcpy(void *dest, const void *source, size_t len);
extern size_t pngcrt_strlen(const char *s);
extern char *pngcrt_strcpy(char *dest, const char *source);
extern int pngcrt_memcmp(const void *s1, const void *s2, size_t len);
extern void *pngcrt_memset(void *dest, int c, size_t len);
extern double pngcrt_strtod(const char *s, char **endptr);
extern int pngcrt_abs(int x);

#ifdef PNG_INTERNAL
#  define malloc pngcrt_malloc
#  define free   pngcrt_free
#  define strtod pngcrt_strtod
#  define abs    pngcrt_abs
#endif

#if defined(PNG_INTERNAL)

#  define PNG_MMX_ROWBYTES_THRESHOLD_DEFAULT  128
#  define PNG_MMX_BITDEPTH_THRESHOLD_DEFAULT  9

#endif

#endif
