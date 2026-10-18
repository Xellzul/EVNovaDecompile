/*
 * COPYRIGHT NOTICE, DISCLAIMER, and LICENSE:
 *
 * If you modify libpng you may insert additional notices immediately following
 * this sentence.
 *
 * This copy was altered for the EV Nova decompilation: comments, the #if branches
 * the EV Nova build does not take and the declarations of functions these
 * sources do not define (the write side) were removed.
 *
 * libpng versions 1.2.6, August 15, 2004, through 1.2.7, September 12, 2004, are
 * Copyright (c) 2004 Glenn Randers-Pehrson, and are
 * distributed according to the same disclaimer and license as libpng-1.2.5
 * with the following individual added to the list of Contributing Authors:
 *
 *    Cosmin Truta
 *
 * libpng versions 1.0.7, July 1, 2000, through 1.2.5, October 3, 2002, are
 * Copyright (c) 2000-2002 Glenn Randers-Pehrson, and are
 * distributed according to the same disclaimer and license as libpng-1.0.6
 * with the following individuals added to the list of Contributing Authors:
 *
 *    Simon-Pierre Cadieux
 *    Eric S. Raymond
 *    Gilles Vollant
 *
 * and with the following additions to the disclaimer:
 *
 *    There is no warranty against interference with your enjoyment of the
 *    library or against infringement.  There is no warranty that our
 *    efforts or the library will fulfill any of your particular purposes
 *    or needs.  This library is provided with all faults, and the entire
 *    risk of satisfactory quality, performance, accuracy, and effort is with
 *    the user.
 *
 * libpng versions 0.97, January 1998, through 1.0.6, March 20, 2000, are
 * Copyright (c) 1998, 1999, 2000 Glenn Randers-Pehrson, and are
 * distributed according to the same disclaimer and license as libpng-0.96,
 * with the following individuals added to the list of Contributing Authors:
 *
 *    Tom Lane
 *    Glenn Randers-Pehrson
 *    Willem van Schaik
 *
 * libpng versions 0.89, June 1996, through 0.96, May 1997, are
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Distributed according to the same disclaimer and license as libpng-0.88,
 * with the following individuals added to the list of Contributing Authors:
 *
 *    John Bowler
 *    Kevin Bracey
 *    Sam Bushell
 *    Magnus Holmgren
 *    Greg Roelofs
 *    Tom Tanner
 *
 * libpng versions 0.5, May 1995, through 0.88, January 1996, are
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 *
 * For the purposes of this copyright and license, "Contributing Authors"
 * is defined as the following set of individuals:
 *
 *    Andreas Dilger
 *    Dave Martindale
 *    Guy Eric Schalnat
 *    Paul Schmidt
 *    Tim Wegner
 *
 * The PNG Reference Library is supplied "AS IS".  The Contributing Authors
 * and Group 42, Inc. disclaim all warranties, expressed or implied,
 * including, without limitation, the warranties of merchantability and of
 * fitness for any purpose.  The Contributing Authors and Group 42, Inc.
 * assume no liability for direct, indirect, incidental, special, exemplary,
 * or consequential damages, which may result from the use of the PNG
 * Reference Library, even if advised of the possibility of such damage.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * source code, or portions hereof, for any purpose, without fee, subject
 * to the following restrictions:
 *
 * 1. The origin of this source code must not be misrepresented.
 *
 * 2. Altered versions must be plainly marked as such and
 * must not be misrepresented as being the original source.
 *
 * 3. This Copyright notice may not be removed or altered from
 *    any source or altered source distribution.
 *
 * The Contributing Authors and Group 42, Inc. specifically permit, without
 * fee, and encourage the use of this source code as a component to
 * supporting the PNG file format in commercial products.  If you use this
 * source code in a product, acknowledgment is not required but would be
 * appreciated.
 */

#ifndef PNG_H
#define PNG_H

#define PNG_LIBPNG_VER_STRING "1.2.7"
#define PNG_HEADER_VERSION_STRING \
   " libpng version 1.2.7 - September 12, 2004 (header)\n"

#define PNG_LIBPNG_VER_SONUM   0
#define PNG_LIBPNG_VER_DLLNUM  %DLLNUM%

#define PNG_LIBPNG_VER_MAJOR   1
#define PNG_LIBPNG_VER_MINOR   2
#define PNG_LIBPNG_VER_RELEASE 7

#define PNG_LIBPNG_VER_BUILD  0

#define PNG_LIBPNG_BUILD_ALPHA    1
#define PNG_LIBPNG_BUILD_BETA     2
#define PNG_LIBPNG_BUILD_RC       3
#define PNG_LIBPNG_BUILD_STABLE   4
#define PNG_LIBPNG_BUILD_TYPEMASK 7
#define PNG_LIBPNG_BUILD_PATCH    8
#define PNG_LIBPNG_BUILD_TYPE 4

#define PNG_LIBPNG_VER 10207

#include "zlib.h"

#include "pngconf.h"

#define int_p_NULL                (int *)NULL
#define png_bytep_NULL            (png_bytep)NULL
#define png_bytepp_NULL           (png_bytepp)NULL
#define png_doublep_NULL          (png_doublep)NULL
#define png_error_ptr_NULL        (png_error_ptr)NULL
#define png_flush_ptr_NULL        (png_flush_ptr)NULL
#define png_free_ptr_NULL         (png_free_ptr)NULL
#define png_infopp_NULL           (png_infopp)NULL
#define png_malloc_ptr_NULL       (png_malloc_ptr)NULL
#define png_read_status_ptr_NULL  (png_read_status_ptr)NULL
#define png_rw_ptr_NULL           (png_rw_ptr)NULL
#define png_structp_NULL          (png_structp)NULL
#define png_uint_16p_NULL         (png_uint_16p)NULL
#define png_voidp_NULL            (png_voidp)NULL
#define png_write_status_ptr_NULL (png_write_status_ptr)NULL

PNG_EXPORT_VAR (const char) png_libpng_ver[18];

PNG_EXPORT_VAR (const int FARDATA) png_pass_start[7];
PNG_EXPORT_VAR (const int FARDATA) png_pass_inc[7];
PNG_EXPORT_VAR (const int FARDATA) png_pass_ystart[7];
PNG_EXPORT_VAR (const int FARDATA) png_pass_yinc[7];
PNG_EXPORT_VAR (const int FARDATA) png_pass_mask[7];
PNG_EXPORT_VAR (const int FARDATA) png_pass_dsp_mask[7];

typedef struct png_color_struct
{
   png_byte red;
   png_byte green;
   png_byte blue;
} png_color;
typedef png_color FAR * png_colorp;
typedef png_color FAR * FAR * png_colorpp;

typedef struct png_color_16_struct
{
   png_byte index;
   png_uint_16 red;
   png_uint_16 green;
   png_uint_16 blue;
   png_uint_16 gray;
} png_color_16;
typedef png_color_16 FAR * png_color_16p;
typedef png_color_16 FAR * FAR * png_color_16pp;

typedef struct png_color_8_struct
{
   png_byte red;
   png_byte green;
   png_byte blue;
   png_byte gray;
   png_byte alpha;
} png_color_8;
typedef png_color_8 FAR * png_color_8p;
typedef png_color_8 FAR * FAR * png_color_8pp;

typedef struct png_sPLT_entry_struct
{
   png_uint_16 red;
   png_uint_16 green;
   png_uint_16 blue;
   png_uint_16 alpha;
   png_uint_16 frequency;
} png_sPLT_entry;
typedef png_sPLT_entry FAR * png_sPLT_entryp;
typedef png_sPLT_entry FAR * FAR * png_sPLT_entrypp;

typedef struct png_sPLT_struct
{
   png_charp name;
   png_byte depth;
   png_sPLT_entryp entries;
   png_int_32 nentries;
} png_sPLT_t;
typedef png_sPLT_t FAR * png_sPLT_tp;
typedef png_sPLT_t FAR * FAR * png_sPLT_tpp;

typedef struct png_text_struct
{
   int  compression;

   png_charp key;
   png_charp text;

   png_size_t text_length;
} png_text;
typedef png_text FAR * png_textp;
typedef png_text FAR * FAR * png_textpp;

#define PNG_TEXT_COMPRESSION_NONE_WR -3
#define PNG_TEXT_COMPRESSION_zTXt_WR -2
#define PNG_TEXT_COMPRESSION_NONE    -1
#define PNG_TEXT_COMPRESSION_zTXt     0
#define PNG_ITXT_COMPRESSION_NONE     1
#define PNG_ITXT_COMPRESSION_zTXt     2
#define PNG_TEXT_COMPRESSION_LAST     3

typedef struct png_time_struct
{
   png_uint_16 year;
   png_byte month;
   png_byte day;
   png_byte hour;
   png_byte minute;
   png_byte second;
} png_time;
typedef png_time FAR * png_timep;
typedef png_time FAR * FAR * png_timepp;

typedef struct png_unknown_chunk_t
{
    png_byte name[5];
    png_byte *data;
    png_size_t size;

    png_byte location;
}
png_unknown_chunk;
typedef png_unknown_chunk FAR * png_unknown_chunkp;
typedef png_unknown_chunk FAR * FAR * png_unknown_chunkpp;

typedef struct png_info_struct
{

   png_uint_32 width;
   png_uint_32 height;
   png_uint_32 valid;
   png_uint_32 rowbytes;
   png_colorp palette;
   png_uint_16 num_palette;
   png_uint_16 num_trans;
   png_byte bit_depth;
   png_byte color_type;

   png_byte compression_type;
   png_byte filter_type;
   png_byte interlace_type;

   png_byte channels;
   png_byte pixel_depth;
   png_byte spare_byte;
   png_byte signature[8];

   float gamma;

   png_byte srgb_intent;

   int num_text;
   int max_text;
   png_textp text;

   png_time mod_time;

   png_color_8 sig_bit;

   png_bytep trans;
   png_color_16 trans_values;

   png_color_16 background;

   png_int_32 x_offset;
   png_int_32 y_offset;
   png_byte offset_unit_type;

   png_uint_32 x_pixels_per_unit;
   png_uint_32 y_pixels_per_unit;
   png_byte phys_unit_type;

   png_uint_16p hist;

   float x_white;
   float y_white;
   float x_red;
   float y_red;
   float x_green;
   float y_green;
   float x_blue;
   float y_blue;

   png_charp pcal_purpose;
   png_int_32 pcal_X0;
   png_int_32 pcal_X1;
   png_charp pcal_units;
   png_charpp pcal_params;
   png_byte pcal_type;
   png_byte pcal_nparams;

   png_uint_32 free_me;

   png_unknown_chunkp unknown_chunks;
   png_size_t unknown_chunks_num;

   png_charp iccp_name;
   png_charp iccp_profile;

   png_uint_32 iccp_proflen;
   png_byte iccp_compression;

   png_sPLT_tp splt_palettes;
   png_uint_32 splt_palettes_num;

   png_byte scal_unit;
   double scal_pixel_width;
   double scal_pixel_height;
   png_charp scal_s_width;
   png_charp scal_s_height;

   png_bytepp row_pointers;

   png_fixed_point int_gamma;

   png_fixed_point int_x_white;
   png_fixed_point int_y_white;
   png_fixed_point int_x_red;
   png_fixed_point int_y_red;
   png_fixed_point int_x_green;
   png_fixed_point int_y_green;
   png_fixed_point int_x_blue;
   png_fixed_point int_y_blue;

} png_info;

typedef png_info FAR * png_infop;
typedef png_info FAR * FAR * png_infopp;

#define PNG_UINT_31_MAX ((png_uint_32)0x7fffffffL)
#define PNG_UINT_32_MAX (~((png_uint_32)0))
#define PNG_SIZE_MAX (~((png_size_t)0))

#define PNG_MAX_UINT PNG_UINT_31_MAX

#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)

#define PNG_COLOR_TYPE_RGBA  PNG_COLOR_TYPE_RGB_ALPHA
#define PNG_COLOR_TYPE_GA  PNG_COLOR_TYPE_GRAY_ALPHA

#define PNG_COMPRESSION_TYPE_BASE 0
#define PNG_COMPRESSION_TYPE_DEFAULT PNG_COMPRESSION_TYPE_BASE

#define PNG_FILTER_TYPE_BASE      0
#define PNG_INTRAPIXEL_DIFFERENCING 64
#define PNG_FILTER_TYPE_DEFAULT   PNG_FILTER_TYPE_BASE

#define PNG_INTERLACE_NONE        0
#define PNG_INTERLACE_ADAM7       1
#define PNG_INTERLACE_LAST        2

#define PNG_OFFSET_PIXEL          0
#define PNG_OFFSET_MICROMETER     1
#define PNG_OFFSET_LAST           2

#define PNG_EQUATION_LINEAR       0
#define PNG_EQUATION_BASE_E       1
#define PNG_EQUATION_ARBITRARY    2
#define PNG_EQUATION_HYPERBOLIC   3
#define PNG_EQUATION_LAST         4

#define PNG_SCALE_UNKNOWN         0
#define PNG_SCALE_METER           1
#define PNG_SCALE_RADIAN          2
#define PNG_SCALE_LAST            3

#define PNG_RESOLUTION_UNKNOWN    0
#define PNG_RESOLUTION_METER      1
#define PNG_RESOLUTION_LAST       2

#define PNG_sRGB_INTENT_PERCEPTUAL 0
#define PNG_sRGB_INTENT_RELATIVE   1
#define PNG_sRGB_INTENT_SATURATION 2
#define PNG_sRGB_INTENT_ABSOLUTE   3
#define PNG_sRGB_INTENT_LAST       4

#define PNG_KEYWORD_MAX_LENGTH     79

#define PNG_MAX_PALETTE_LENGTH    256

#define PNG_INFO_gAMA 0x0001
#define PNG_INFO_sBIT 0x0002
#define PNG_INFO_cHRM 0x0004
#define PNG_INFO_PLTE 0x0008
#define PNG_INFO_tRNS 0x0010
#define PNG_INFO_bKGD 0x0020
#define PNG_INFO_hIST 0x0040
#define PNG_INFO_pHYs 0x0080
#define PNG_INFO_oFFs 0x0100
#define PNG_INFO_tIME 0x0200
#define PNG_INFO_pCAL 0x0400
#define PNG_INFO_sRGB 0x0800
#define PNG_INFO_iCCP 0x1000
#define PNG_INFO_sPLT 0x2000
#define PNG_INFO_sCAL 0x4000
#define PNG_INFO_IDAT 0x8000L

typedef struct png_row_info_struct
{
   png_uint_32 width;
   png_uint_32 rowbytes;
   png_byte color_type;
   png_byte bit_depth;
   png_byte channels;
   png_byte pixel_depth;
} png_row_info;

typedef png_row_info FAR * png_row_infop;
typedef png_row_info FAR * FAR * png_row_infopp;

typedef struct png_struct_def png_struct;
typedef png_struct FAR * png_structp;

typedef void (PNGAPI *png_error_ptr) PNGARG((png_structp, png_const_charp));
typedef void (PNGAPI *png_rw_ptr) PNGARG((png_structp, png_bytep, png_size_t));
typedef void (PNGAPI *png_flush_ptr) PNGARG((png_structp));
typedef void (PNGAPI *png_read_status_ptr) PNGARG((png_structp, png_uint_32,
   int));
typedef void (PNGAPI *png_write_status_ptr) PNGARG((png_structp, png_uint_32,
   int));

typedef void (PNGAPI *png_progressive_info_ptr) PNGARG((png_structp, png_infop));
typedef void (PNGAPI *png_progressive_end_ptr) PNGARG((png_structp, png_infop));
typedef void (PNGAPI *png_progressive_row_ptr) PNGARG((png_structp, png_bytep,
   png_uint_32, int));

typedef void (PNGAPI *png_user_transform_ptr) PNGARG((png_structp,
    png_row_infop, png_bytep));

typedef int (PNGAPI *png_user_chunk_ptr) PNGARG((png_structp, png_unknown_chunkp));
typedef void (PNGAPI *png_unknown_chunk_ptr) PNGARG((png_structp));

#define PNG_TRANSFORM_IDENTITY       0x0000
#define PNG_TRANSFORM_STRIP_16       0x0001
#define PNG_TRANSFORM_STRIP_ALPHA    0x0002
#define PNG_TRANSFORM_PACKING        0x0004
#define PNG_TRANSFORM_PACKSWAP       0x0008
#define PNG_TRANSFORM_EXPAND         0x0010
#define PNG_TRANSFORM_INVERT_MONO    0x0020
#define PNG_TRANSFORM_SHIFT          0x0040
#define PNG_TRANSFORM_BGR            0x0080
#define PNG_TRANSFORM_SWAP_ALPHA     0x0100
#define PNG_TRANSFORM_SWAP_ENDIAN    0x0200
#define PNG_TRANSFORM_INVERT_ALPHA   0x0400
#define PNG_TRANSFORM_STRIP_FILLER   0x0800

#define PNG_FLAG_MNG_EMPTY_PLTE     0x01
#define PNG_FLAG_MNG_FILTER_64      0x04
#define PNG_ALL_MNG_FEATURES        0x05

typedef png_voidp (*png_malloc_ptr) PNGARG((png_structp, png_size_t));
typedef void (*png_free_ptr) PNGARG((png_structp, png_voidp));

struct png_struct_def
{
   jmp_buf jmpbuf;
   png_error_ptr error_fn;
   png_error_ptr warning_fn;
   png_voidp error_ptr;
   png_rw_ptr write_data_fn;
   png_rw_ptr read_data_fn;
   png_voidp io_ptr;

   png_user_transform_ptr read_user_transform_fn;

   png_user_transform_ptr write_user_transform_fn;

   png_voidp user_transform_ptr;
   png_byte user_transform_depth;
   png_byte user_transform_channels;

   png_uint_32 mode;
   png_uint_32 flags;
   png_uint_32 transformations;

   z_stream zstream;
   png_bytep zbuf;
   png_size_t zbuf_size;
   int zlib_level;
   int zlib_method;
   int zlib_window_bits;
   int zlib_mem_level;
   int zlib_strategy;

   png_uint_32 width;
   png_uint_32 height;
   png_uint_32 num_rows;
   png_uint_32 usr_width;
   png_uint_32 rowbytes;
   png_uint_32 irowbytes;
   png_uint_32 iwidth;
   png_uint_32 row_number;
   png_bytep prev_row;
   png_bytep row_buf;
   png_bytep sub_row;
   png_bytep up_row;
   png_bytep avg_row;
   png_bytep paeth_row;
   png_row_info row_info;

   png_uint_32 idat_size;
   png_uint_32 crc;
   png_colorp palette;
   png_uint_16 num_palette;
   png_uint_16 num_trans;
   png_byte chunk_name[5];
   png_byte compression;
   png_byte filter;
   png_byte interlaced;
   png_byte pass;
   png_byte do_filter;
   png_byte color_type;
   png_byte bit_depth;
   png_byte usr_bit_depth;
   png_byte pixel_depth;
   png_byte channels;
   png_byte usr_channels;
   png_byte sig_bytes;

   png_uint_16 filler;

   png_byte background_gamma_type;
   float background_gamma;
   png_color_16 background;
   png_color_16 background_1;

   png_flush_ptr output_flush_fn;
   png_uint_32 flush_dist;
   png_uint_32 flush_rows;

   int gamma_shift;
   float gamma;
   float screen_gamma;

   png_bytep gamma_table;
   png_bytep gamma_from_1;
   png_bytep gamma_to_1;
   png_uint_16pp gamma_16_table;
   png_uint_16pp gamma_16_from_1;
   png_uint_16pp gamma_16_to_1;

   png_color_8 sig_bit;

   png_color_8 shift;

   png_bytep trans;
   png_color_16 trans_values;

   png_read_status_ptr read_row_fn;
   png_write_status_ptr write_row_fn;
   png_progressive_info_ptr info_fn;
   png_progressive_row_ptr row_fn;
   png_progressive_end_ptr end_fn;
   png_bytep save_buffer_ptr;
   png_bytep save_buffer;
   png_bytep current_buffer_ptr;
   png_bytep current_buffer;
   png_uint_32 push_length;
   png_uint_32 skip_length;
   png_size_t save_buffer_size;
   png_size_t save_buffer_max;
   png_size_t buffer_size;
   png_size_t current_buffer_size;
   int process_mode;
   int cur_palette;

     png_size_t current_text_size;
     png_size_t current_text_left;
     png_charp current_text;
     png_charp current_text_ptr;

   png_bytep palette_lookup;
   png_bytep dither_index;

   png_uint_16p hist;

   png_byte heuristic_method;
   png_byte num_prev_filters;
   png_bytep prev_filters;
   png_uint_16p filter_weights;
   png_uint_16p inv_filter_weights;
   png_uint_16p filter_costs;
   png_uint_16p inv_filter_costs;

   png_uint_32 free_me;

   png_voidp user_chunk_ptr;
   png_user_chunk_ptr read_user_chunk_fn;

   int num_chunk_list;
   png_bytep chunk_list;

   png_byte rgb_to_gray_status;

   png_uint_16 rgb_to_gray_red_coeff;
   png_uint_16 rgb_to_gray_green_coeff;
   png_uint_16 rgb_to_gray_blue_coeff;

   png_uint_32 mng_features_permitted;

   png_fixed_point int_gamma;

   png_byte filter_type;

   png_byte     mmx_bitdepth_threshold;
   png_uint_32  mmx_rowbytes_threshold;
   png_uint_32  asm_flags;

   png_voidp mem_ptr;
   png_malloc_ptr malloc_fn;
   png_free_ptr free_fn;

   png_bytep big_row_buf;

   png_bytep dither_sort;
   png_bytep index_to_palette;

   png_bytep palette_to_index;

   png_byte compression_type;

   png_uint_32 user_width_max;
   png_uint_32 user_height_max;

};

typedef png_structp version_1_2_7;

typedef png_struct FAR * FAR * png_structpp;

extern PNG_EXPORT(png_uint_32,png_access_version_number) PNGARG((void));

extern PNG_EXPORT(void,png_set_sig_bytes) PNGARG((png_structp png_ptr,
   int num_bytes));

extern PNG_EXPORT(int,png_sig_cmp) PNGARG((png_bytep sig, png_size_t start,
   png_size_t num_to_check));

extern PNG_EXPORT(int,png_check_sig) PNGARG((png_bytep sig, int num));

extern PNG_EXPORT(png_structp,png_create_read_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));

extern PNG_EXPORT(png_structp,png_create_write_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));

extern PNG_EXPORT(png_uint_32,png_get_compression_buffer_size)
   PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_compression_buffer_size)
   PNGARG((png_structp png_ptr, png_uint_32 size));

extern PNG_EXPORT(int,png_reset_zstream) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(png_structp,png_create_read_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn));
extern PNG_EXPORT(png_structp,png_create_write_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn));

extern PNG_EXPORT(void,png_write_chunk) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_bytep data, png_size_t length));

extern PNG_EXPORT(void,png_write_chunk_start) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_uint_32 length));

extern PNG_EXPORT(void,png_write_chunk_data) PNGARG((png_structp png_ptr,
   png_bytep data, png_size_t length));

extern PNG_EXPORT(void,png_write_chunk_end) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(png_infop,png_create_info_struct)
   PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_info_init) PNGARG((png_infop info_ptr));
#undef png_info_init
#define png_info_init(info_ptr) png_info_init_3(&info_ptr,\
    png_sizeof(png_info));
extern PNG_EXPORT(void,png_info_init_3) PNGARG((png_infopp info_ptr,
    png_size_t png_info_struct_size));

extern PNG_EXPORT(void,png_write_info_before_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr));
extern PNG_EXPORT(void,png_write_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

extern PNG_EXPORT(void,png_read_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

extern PNG_EXPORT(void,png_convert_from_struct_tm) PNGARG((png_timep ptime,
   struct tm FAR * ttime));

extern PNG_EXPORT(void,png_convert_from_time_t) PNGARG((png_timep ptime,
   time_t ttime));

extern PNG_EXPORT(void,png_set_expand) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(void,png_set_gray_1_2_4_to_8) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(void,png_set_palette_to_rgb) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(void,png_set_tRNS_to_alpha) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_bgr) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_gray_to_rgb) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_rgb_to_gray) PNGARG((png_structp png_ptr,
   int error_action, double red, double green ));
extern PNG_EXPORT(void,png_set_rgb_to_gray_fixed) PNGARG((png_structp png_ptr,
   int error_action, png_fixed_point red, png_fixed_point green ));
extern PNG_EXPORT(png_byte,png_get_rgb_to_gray_status) PNGARG((png_structp
   png_ptr));

extern PNG_EXPORT(void,png_build_grayscale_palette) PNGARG((int bit_depth,
   png_colorp palette));

extern PNG_EXPORT(void,png_set_strip_alpha) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_swap_alpha) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_invert_alpha) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_filler) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));

#define PNG_FILLER_BEFORE 0
#define PNG_FILLER_AFTER 1

extern PNG_EXPORT(void,png_set_add_alpha) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));

extern PNG_EXPORT(void,png_set_swap) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_packing) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_packswap) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_shift) PNGARG((png_structp png_ptr,
   png_color_8p true_bits));

extern PNG_EXPORT(int,png_set_interlace_handling) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_invert_mono) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_background) PNGARG((png_structp png_ptr,
   png_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma));
#define PNG_BACKGROUND_GAMMA_UNKNOWN 0
#define PNG_BACKGROUND_GAMMA_SCREEN  1
#define PNG_BACKGROUND_GAMMA_FILE    2
#define PNG_BACKGROUND_GAMMA_UNIQUE  3

extern PNG_EXPORT(void,png_set_strip_16) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_dither) PNGARG((png_structp png_ptr,
   png_colorp palette, int num_palette, int maximum_colors,
   png_uint_16p histogram, int full_dither));

extern PNG_EXPORT(void,png_set_gamma) PNGARG((png_structp png_ptr,
   double screen_gamma, double default_file_gamma));

extern PNG_EXPORT(void,png_permit_empty_plte) PNGARG((png_structp png_ptr,
   int empty_plte_permitted));

extern PNG_EXPORT(void,png_set_flush) PNGARG((png_structp png_ptr, int nrows));

extern PNG_EXPORT(void,png_write_flush) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_start_read_image) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_read_update_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

extern PNG_EXPORT(void,png_read_rows) PNGARG((png_structp png_ptr,
   png_bytepp row, png_bytepp display_row, png_uint_32 num_rows));

extern PNG_EXPORT(void,png_read_row) PNGARG((png_structp png_ptr,
   png_bytep row,
   png_bytep display_row));

extern PNG_EXPORT(void,png_read_image) PNGARG((png_structp png_ptr,
   png_bytepp image));

extern PNG_EXPORT(void,png_write_row) PNGARG((png_structp png_ptr,
   png_bytep row));

extern PNG_EXPORT(void,png_write_rows) PNGARG((png_structp png_ptr,
   png_bytepp row, png_uint_32 num_rows));

extern PNG_EXPORT(void,png_write_image) PNGARG((png_structp png_ptr,
   png_bytepp image));

extern PNG_EXPORT(void,png_write_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

extern PNG_EXPORT(void,png_read_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

extern PNG_EXPORT(void,png_destroy_info_struct) PNGARG((png_structp png_ptr,
   png_infopp info_ptr_ptr));

extern PNG_EXPORT(void,png_destroy_read_struct) PNGARG((png_structpp
   png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr));

extern void png_read_destroy PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_infop end_info_ptr));

extern PNG_EXPORT(void,png_destroy_write_struct)
   PNGARG((png_structpp png_ptr_ptr, png_infopp info_ptr_ptr));

extern void png_write_destroy PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_crc_action) PNGARG((png_structp png_ptr,
   int crit_action, int ancil_action));

#define PNG_CRC_DEFAULT       0
#define PNG_CRC_ERROR_QUIT    1
#define PNG_CRC_WARN_DISCARD  2
#define PNG_CRC_WARN_USE      3
#define PNG_CRC_QUIET_USE     4
#define PNG_CRC_NO_CHANGE     5

extern PNG_EXPORT(void,png_set_filter) PNGARG((png_structp png_ptr, int method,
   int filters));

#define PNG_NO_FILTERS     0x00
#define PNG_FILTER_NONE    0x08
#define PNG_FILTER_SUB     0x10
#define PNG_FILTER_UP      0x20
#define PNG_FILTER_AVG     0x40
#define PNG_FILTER_PAETH   0x80
#define PNG_ALL_FILTERS (PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | \
                         PNG_FILTER_AVG | PNG_FILTER_PAETH)

#define PNG_FILTER_VALUE_NONE  0
#define PNG_FILTER_VALUE_SUB   1
#define PNG_FILTER_VALUE_UP    2
#define PNG_FILTER_VALUE_AVG   3
#define PNG_FILTER_VALUE_PAETH 4
#define PNG_FILTER_VALUE_LAST  5

extern PNG_EXPORT(void,png_set_filter_heuristics) PNGARG((png_structp png_ptr,
   int heuristic_method, int num_weights, png_doublep filter_weights,
   png_doublep filter_costs));

#define PNG_FILTER_HEURISTIC_DEFAULT    0
#define PNG_FILTER_HEURISTIC_UNWEIGHTED 1
#define PNG_FILTER_HEURISTIC_WEIGHTED   2
#define PNG_FILTER_HEURISTIC_LAST       3

extern PNG_EXPORT(void,png_set_compression_level) PNGARG((png_structp png_ptr,
   int level));

extern PNG_EXPORT(void,png_set_compression_mem_level)
   PNGARG((png_structp png_ptr, int mem_level));

extern PNG_EXPORT(void,png_set_compression_strategy)
   PNGARG((png_structp png_ptr, int strategy));

extern PNG_EXPORT(void,png_set_compression_window_bits)
   PNGARG((png_structp png_ptr, int window_bits));

extern PNG_EXPORT(void,png_set_compression_method) PNGARG((png_structp png_ptr,
   int method));

extern PNG_EXPORT(void,png_set_error_fn) PNGARG((png_structp png_ptr,
   png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warning_fn));

extern PNG_EXPORT(png_voidp,png_get_error_ptr) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_write_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn));

extern PNG_EXPORT(void,png_set_read_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr read_data_fn));

extern PNG_EXPORT(png_voidp,png_get_io_ptr) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_read_status_fn) PNGARG((png_structp png_ptr,
   png_read_status_ptr read_row_fn));

extern PNG_EXPORT(void,png_set_write_status_fn) PNGARG((png_structp png_ptr,
   png_write_status_ptr write_row_fn));

extern PNG_EXPORT(void,png_set_mem_fn) PNGARG((png_structp png_ptr,
   png_voidp mem_ptr, png_malloc_ptr malloc_fn, png_free_ptr free_fn));

extern PNG_EXPORT(png_voidp,png_get_mem_ptr) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_read_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr read_user_transform_fn));

extern PNG_EXPORT(void,png_set_write_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr write_user_transform_fn));

extern PNG_EXPORT(void,png_set_user_transform_info) PNGARG((png_structp
   png_ptr, png_voidp user_transform_ptr, int user_transform_depth,
   int user_transform_channels));

extern PNG_EXPORT(png_voidp,png_get_user_transform_ptr)
   PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_read_user_chunk_fn) PNGARG((png_structp png_ptr,
   png_voidp user_chunk_ptr, png_user_chunk_ptr read_user_chunk_fn));
extern PNG_EXPORT(png_voidp,png_get_user_chunk_ptr) PNGARG((png_structp
   png_ptr));

extern PNG_EXPORT(png_voidp,png_malloc) PNGARG((png_structp png_ptr,
   png_uint_32 size));

extern PNG_EXPORT(png_voidp,png_malloc_warn) PNGARG((png_structp png_ptr,
   png_uint_32 size));

extern PNG_EXPORT(void,png_free) PNGARG((png_structp png_ptr, png_voidp ptr));

extern PNG_EXPORT(void,png_free_data) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 free_me, int num));

extern PNG_EXPORT(void,png_data_freer) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int freer, png_uint_32 mask));

#define PNG_DESTROY_WILL_FREE_DATA 1
#define PNG_SET_WILL_FREE_DATA 1
#define PNG_USER_WILL_FREE_DATA 2

#define PNG_FREE_HIST 0x0008
#define PNG_FREE_ICCP 0x0010
#define PNG_FREE_SPLT 0x0020
#define PNG_FREE_ROWS 0x0040
#define PNG_FREE_PCAL 0x0080
#define PNG_FREE_SCAL 0x0100
#define PNG_FREE_UNKN 0x0200
#define PNG_FREE_LIST 0x0400
#define PNG_FREE_PLTE 0x1000
#define PNG_FREE_TRNS 0x2000
#define PNG_FREE_TEXT 0x4000
#define PNG_FREE_ALL  0x7fff
#define PNG_FREE_MUL  0x4220

extern PNG_EXPORT(png_voidp,png_malloc_default) PNGARG((png_structp png_ptr,
   png_uint_32 size));
extern PNG_EXPORT(void,png_free_default) PNGARG((png_structp png_ptr,
   png_voidp ptr));

extern PNG_EXPORT(png_voidp,png_memcpy_check) PNGARG((png_structp png_ptr,
   png_voidp s1, png_voidp s2, png_uint_32 size));

extern PNG_EXPORT(png_voidp,png_memset_check) PNGARG((png_structp png_ptr,
   png_voidp s1, int value, png_uint_32 size));

extern PNG_EXPORT(void,png_error) PNGARG((png_structp png_ptr,
   png_const_charp error_message));

extern PNG_EXPORT(void,png_chunk_error) PNGARG((png_structp png_ptr,
   png_const_charp error_message));

extern PNG_EXPORT(void,png_warning) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));

extern PNG_EXPORT(void,png_chunk_warning) PNGARG((png_structp png_ptr,
   png_const_charp warning_message));

extern PNG_EXPORT(png_uint_32,png_get_valid) PNGARG((png_structp png_ptr,
png_infop info_ptr, png_uint_32 flag));

extern PNG_EXPORT(png_bytepp,png_get_rows) PNGARG((png_structp png_ptr,
png_infop info_ptr));

extern PNG_EXPORT(void,png_set_rows) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers));

extern PNG_EXPORT(png_byte,png_get_channels) PNGARG((png_structp png_ptr,
png_infop info_ptr));

extern PNG_EXPORT(png_uint_32, png_get_image_width) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_uint_32, png_get_image_height) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_byte, png_get_bit_depth) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_byte, png_get_color_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_byte, png_get_filter_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_byte, png_get_interlace_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_byte, png_get_compression_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_uint_32, png_get_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32, png_get_x_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32, png_get_y_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(float, png_get_pixel_aspect_ratio) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_int_32, png_get_x_offset_pixels) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_int_32, png_get_y_offset_pixels) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_int_32, png_get_x_offset_microns) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_int_32, png_get_y_offset_microns) PNGARG((png_structp
png_ptr, png_infop info_ptr));

extern PNG_EXPORT(png_bytep,png_get_signature) PNGARG((png_structp png_ptr,
png_infop info_ptr));

extern PNG_EXPORT(png_uint_32,png_get_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p *background));

extern PNG_EXPORT(void,png_set_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p background));

extern PNG_EXPORT(png_uint_32,png_get_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double *white_x, double *white_y, double *red_x,
   double *red_y, double *green_x, double *green_y, double *blue_x,
   double *blue_y));
extern PNG_EXPORT(png_uint_32,png_get_cHRM_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point *int_white_x, png_fixed_point
   *int_white_y, png_fixed_point *int_red_x, png_fixed_point *int_red_y,
   png_fixed_point *int_green_x, png_fixed_point *int_green_y, png_fixed_point
   *int_blue_x, png_fixed_point *int_blue_y));

extern PNG_EXPORT(void,png_set_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double white_x, double white_y, double red_x,
   double red_y, double green_x, double green_y, double blue_x, double blue_y));
extern PNG_EXPORT(void,png_set_cHRM_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point int_white_x, png_fixed_point int_white_y,
   png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
   int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
   png_fixed_point int_blue_y));

extern PNG_EXPORT(png_uint_32,png_get_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double *file_gamma));
extern PNG_EXPORT(png_uint_32,png_get_gAMA_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point *int_file_gamma));

extern PNG_EXPORT(void,png_set_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double file_gamma));
extern PNG_EXPORT(void,png_set_gAMA_fixed) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_fixed_point int_file_gamma));

extern PNG_EXPORT(png_uint_32,png_get_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p *hist));

extern PNG_EXPORT(void,png_set_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p hist));

extern PNG_EXPORT(png_uint_32,png_get_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *width, png_uint_32 *height,
   int *bit_depth, int *color_type, int *interlace_method,
   int *compression_method, int *filter_method));

extern PNG_EXPORT(void,png_set_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_method, int compression_method,
   int filter_method));

extern PNG_EXPORT(png_uint_32,png_get_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_int_32 *offset_x, png_int_32 *offset_y,
   int *unit_type));

extern PNG_EXPORT(void,png_set_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_int_32 offset_x, png_int_32 offset_y,
   int unit_type));

extern PNG_EXPORT(png_uint_32,png_get_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp *purpose, png_int_32 *X0, png_int_32 *X1,
   int *type, int *nparams, png_charp *units, png_charpp *params));

extern PNG_EXPORT(void,png_set_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp purpose, png_int_32 X0, png_int_32 X1,
   int type, int nparams, png_charp units, png_charpp params));

extern PNG_EXPORT(png_uint_32,png_get_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type));

extern PNG_EXPORT(void,png_set_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 res_x, png_uint_32 res_y, int unit_type));

extern PNG_EXPORT(png_uint_32,png_get_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp *palette, int *num_palette));

extern PNG_EXPORT(void,png_set_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp palette, int num_palette));

extern PNG_EXPORT(png_uint_32,png_get_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p *sig_bit));

extern PNG_EXPORT(void,png_set_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p sig_bit));

extern PNG_EXPORT(png_uint_32,png_get_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int *intent));

extern PNG_EXPORT(void,png_set_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
extern PNG_EXPORT(void,png_set_sRGB_gAMA_and_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));

extern PNG_EXPORT(png_uint_32,png_get_iCCP) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charpp name, int *compression_type,
   png_charpp profile, png_uint_32 *proflen));

extern PNG_EXPORT(void,png_set_iCCP) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp name, int compression_type,
   png_charp profile, png_uint_32 proflen));

extern PNG_EXPORT(png_uint_32,png_get_sPLT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_sPLT_tpp entries));

extern PNG_EXPORT(void,png_set_sPLT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_sPLT_tp entries, int nentries));

extern PNG_EXPORT(png_uint_32,png_get_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp *text_ptr, int *num_text));

extern PNG_EXPORT(void,png_set_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp text_ptr, int num_text));

extern PNG_EXPORT(png_uint_32,png_get_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep *mod_time));

extern PNG_EXPORT(void,png_set_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep mod_time));

extern PNG_EXPORT(png_uint_32,png_get_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep *trans, int *num_trans,
   png_color_16p *trans_values));

extern PNG_EXPORT(void,png_set_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep trans, int num_trans,
   png_color_16p trans_values));

extern PNG_EXPORT(png_uint_32,png_get_sCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int *unit, double *width, double *height));

extern PNG_EXPORT(void,png_set_sCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int unit, double width, double height));

extern PNG_EXPORT(void, png_set_keep_unknown_chunks) PNGARG((png_structp
   png_ptr, int keep, png_bytep chunk_list, int num_chunks));
extern PNG_EXPORT(void, png_set_unknown_chunks) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_unknown_chunkp unknowns, int num_unknowns));
extern PNG_EXPORT(void, png_set_unknown_chunk_location)
   PNGARG((png_structp png_ptr, png_infop info_ptr, int chunk, int location));
extern PNG_EXPORT(png_uint_32,png_get_unknown_chunks) PNGARG((png_structp
   png_ptr, png_infop info_ptr, png_unknown_chunkpp entries));
PNG_EXPORT(int,png_handle_as_unknown) PNGARG((png_structp png_ptr, png_bytep
   chunk_name));

extern PNG_EXPORT(void, png_set_invalid) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int mask));

extern PNG_EXPORT(void, png_read_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));
extern PNG_EXPORT(void, png_write_png) PNGARG((png_structp png_ptr,
                        png_infop info_ptr,
                        int transforms,
                        png_voidp params));

#define png_debug(l, m)
#define png_debug1(l, m, p1)
#define png_debug2(l, m, p1, p2)

extern PNG_EXPORT(png_charp,png_get_copyright) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(png_charp,png_get_header_ver) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(png_charp,png_get_header_version) PNGARG((png_structp png_ptr));
extern PNG_EXPORT(png_charp,png_get_libpng_ver) PNGARG((png_structp png_ptr));

extern PNG_EXPORT(png_uint_32,png_permit_mng_features) PNGARG((png_structp
   png_ptr, png_uint_32 mng_features_permitted));

#define PNG_HANDLE_CHUNK_AS_DEFAULT   0
#define PNG_HANDLE_CHUNK_NEVER        1
#define PNG_HANDLE_CHUNK_IF_SAFE      2
#define PNG_HANDLE_CHUNK_ALWAYS       3

#define PNG_ASM_FLAG_MMX_SUPPORT_COMPILED  0x01
#define PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU    0x02
#define PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  0x04
#define PNG_ASM_FLAG_MMX_READ_INTERLACE    0x08
#define PNG_ASM_FLAG_MMX_READ_FILTER_SUB   0x10
#define PNG_ASM_FLAG_MMX_READ_FILTER_UP    0x20
#define PNG_ASM_FLAG_MMX_READ_FILTER_AVG   0x40
#define PNG_ASM_FLAG_MMX_READ_FILTER_PAETH 0x80
#define PNG_ASM_FLAGS_INITIALIZED          0x80000000

#define PNG_MMX_READ_FLAGS ( PNG_ASM_FLAG_MMX_READ_COMBINE_ROW  \
                           | PNG_ASM_FLAG_MMX_READ_INTERLACE    \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_SUB   \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_UP    \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_AVG   \
                           | PNG_ASM_FLAG_MMX_READ_FILTER_PAETH )
#define PNG_MMX_WRITE_FLAGS ( 0 )

#define PNG_MMX_FLAGS ( PNG_ASM_FLAG_MMX_SUPPORT_COMPILED \
                      | PNG_ASM_FLAG_MMX_SUPPORT_IN_CPU   \
                      | PNG_MMX_READ_FLAGS                \
                      | PNG_MMX_WRITE_FLAGS )

#define PNG_SELECT_READ   1
#define PNG_SELECT_WRITE  2

extern PNG_EXPORT(png_uint_32,png_get_mmx_flagmask)
   PNGARG((int flag_select, int *compilerID));

extern PNG_EXPORT(png_uint_32,png_get_asm_flagmask)
   PNGARG((int flag_select));

extern PNG_EXPORT(png_uint_32,png_get_asm_flags)
   PNGARG((png_structp png_ptr));

extern PNG_EXPORT(png_byte,png_get_mmx_bitdepth_threshold)
   PNGARG((png_structp png_ptr));

extern PNG_EXPORT(png_uint_32,png_get_mmx_rowbytes_threshold)
   PNGARG((png_structp png_ptr));

extern PNG_EXPORT(void,png_set_asm_flags)
   PNGARG((png_structp png_ptr, png_uint_32 asm_flags));

extern PNG_EXPORT(void,png_set_mmx_thresholds)
   PNGARG((png_structp png_ptr, png_byte mmx_bitdepth_threshold,
   png_uint_32 mmx_rowbytes_threshold));

extern PNG_EXPORT(int,png_mmx_support) PNGARG((void));

extern PNG_EXPORT(void,png_set_strip_error_numbers) PNGARG((png_structp
   png_ptr, png_uint_32 strip_mode));

extern PNG_EXPORT(void,png_set_user_limits) PNGARG((png_structp
   png_ptr, png_uint_32 user_width_max, png_uint_32 user_height_max));
extern PNG_EXPORT(png_uint_32,png_get_user_width_max) PNGARG((png_structp
   png_ptr));
extern PNG_EXPORT(png_uint_32,png_get_user_height_max) PNGARG((png_structp
   png_ptr));

#define png_composite(composite, fg, alpha, bg)                            \
     { png_uint_16 temp = (png_uint_16)((png_uint_16)(fg) * (png_uint_16)(alpha) \
                        +        (png_uint_16)(bg)*(png_uint_16)(255 -       \
                        (png_uint_16)(alpha)) + (png_uint_16)128);           \
       (composite) = (png_byte)((temp + (temp >> 8)) >> 8); }

#define png_composite_16(composite, fg, alpha, bg)                         \
     { png_uint_32 temp = (png_uint_32)((png_uint_32)(fg) * (png_uint_32)(alpha) \
                        + (png_uint_32)(bg)*(png_uint_32)(65535L -           \
                        (png_uint_32)(alpha)) + (png_uint_32)32768L);        \
       (composite) = (png_uint_16)((temp + (temp >> 16)) >> 16); }

#if defined(PNG_INTERNAL)

#  define PNG_HAVE_IHDR               0x01
#  define PNG_HAVE_PLTE               0x02
#  define PNG_HAVE_IDAT               0x04
#  define PNG_AFTER_IDAT              0x08
#  define PNG_HAVE_IEND               0x10
#  define PNG_HAVE_gAMA               0x20
#  define PNG_HAVE_cHRM               0x40
#  define PNG_HAVE_sRGB               0x80
#  define PNG_HAVE_CHUNK_HEADER      0x100
#  define PNG_WROTE_tIME             0x200
#  define PNG_WROTE_INFO_BEFORE_PLTE 0x400
#  define PNG_BACKGROUND_IS_GRAY     0x800
#  define PNG_HAVE_PNG_SIGNATURE    0x1000

#  define PNG_BGR                0x0001
#  define PNG_INTERLACE          0x0002
#  define PNG_PACK               0x0004
#  define PNG_SHIFT              0x0008
#  define PNG_SWAP_BYTES         0x0010
#  define PNG_INVERT_MONO        0x0020
#  define PNG_DITHER             0x0040
#  define PNG_BACKGROUND         0x0080
#  define PNG_BACKGROUND_EXPAND  0x0100

#  define PNG_16_TO_8            0x0400
#  define PNG_RGBA               0x0800
#  define PNG_EXPAND             0x1000
#  define PNG_GAMMA              0x2000
#  define PNG_GRAY_TO_RGB        0x4000
#  define PNG_FILLER             0x8000L
#  define PNG_PACKSWAP          0x10000L
#  define PNG_SWAP_ALPHA        0x20000L
#  define PNG_STRIP_ALPHA       0x40000L
#  define PNG_INVERT_ALPHA      0x80000L
#  define PNG_USER_TRANSFORM   0x100000L
#  define PNG_RGB_TO_GRAY_ERR  0x200000L
#  define PNG_RGB_TO_GRAY_WARN 0x400000L
#  define PNG_RGB_TO_GRAY      0x600000L
#  define PNG_ADD_ALPHA       0x1000000L

#  define PNG_STRUCT_PNG   0x0001
#  define PNG_STRUCT_INFO  0x0002

#  define PNG_WEIGHT_SHIFT 8
#  define PNG_WEIGHT_FACTOR (1<<(PNG_WEIGHT_SHIFT))
#  define PNG_COST_SHIFT 3
#  define PNG_COST_FACTOR (1<<(PNG_COST_SHIFT))

#  define PNG_FLAG_ZLIB_CUSTOM_STRATEGY     0x0001
#  define PNG_FLAG_ZLIB_CUSTOM_LEVEL        0x0002
#  define PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL    0x0004
#  define PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS  0x0008
#  define PNG_FLAG_ZLIB_CUSTOM_METHOD       0x0010
#  define PNG_FLAG_ZLIB_FINISHED            0x0020
#  define PNG_FLAG_ROW_INIT                 0x0040
#  define PNG_FLAG_FILLER_AFTER             0x0080
#  define PNG_FLAG_CRC_ANCILLARY_USE        0x0100
#  define PNG_FLAG_CRC_ANCILLARY_NOWARN     0x0200
#  define PNG_FLAG_CRC_CRITICAL_USE         0x0400
#  define PNG_FLAG_CRC_CRITICAL_IGNORE      0x0800
#  define PNG_FLAG_FREE_PLTE                0x1000
#  define PNG_FLAG_FREE_TRNS                0x2000
#  define PNG_FLAG_FREE_HIST                0x4000
#  define PNG_FLAG_KEEP_UNKNOWN_CHUNKS      0x8000L
#  define PNG_FLAG_KEEP_UNSAFE_CHUNKS       0x10000L
#  define PNG_FLAG_LIBRARY_MISMATCH         0x20000L
#  define PNG_FLAG_STRIP_ERROR_NUMBERS      0x40000L
#  define PNG_FLAG_STRIP_ERROR_TEXT         0x80000L
#  define PNG_FLAG_MALLOC_NULL_MEM_OK       0x100000L

#  define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)

#  define PNG_FLAG_CRC_CRITICAL_MASK  (PNG_FLAG_CRC_CRITICAL_USE | \
                                     PNG_FLAG_CRC_CRITICAL_IGNORE)

#  define PNG_FLAG_CRC_MASK           (PNG_FLAG_CRC_ANCILLARY_MASK | \
                                     PNG_FLAG_CRC_CRITICAL_MASK)

#  define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))

#  define PNG_ROWBYTES(pixel_bits, width) \
    ((pixel_bits) >= 8 ? \
    ((width) * (((png_uint_32)(pixel_bits)) >> 3)) : \
    (( ((width) * ((png_uint_32)(pixel_bits))) + 7) >> 3) )

#  define PNG_OUT_OF_RANGE(value, ideal, delta) \
        ( (value) < (ideal)-(delta) || (value) > (ideal)+(delta) )

   PNG_EXPORT_VAR (const png_byte FARDATA) png_sig[8];

#  define PNG_IHDR const png_byte png_IHDR[5] = { 73,  72,  68,  82, '\0'}
#  define PNG_IDAT const png_byte png_IDAT[5] = { 73,  68,  65,  84, '\0'}
#  define PNG_IEND const png_byte png_IEND[5] = { 73,  69,  78,  68, '\0'}
#  define PNG_PLTE const png_byte png_PLTE[5] = { 80,  76,  84,  69, '\0'}
#  define PNG_bKGD const png_byte png_bKGD[5] = { 98,  75,  71,  68, '\0'}
#  define PNG_cHRM const png_byte png_cHRM[5] = { 99,  72,  82,  77, '\0'}
#  define PNG_gAMA const png_byte png_gAMA[5] = {103,  65,  77,  65, '\0'}
#  define PNG_hIST const png_byte png_hIST[5] = {104,  73,  83,  84, '\0'}
#  define PNG_iCCP const png_byte png_iCCP[5] = {105,  67,  67,  80, '\0'}
#  define PNG_iTXt const png_byte png_iTXt[5] = {105,  84,  88, 116, '\0'}
#  define PNG_oFFs const png_byte png_oFFs[5] = {111,  70,  70, 115, '\0'}
#  define PNG_pCAL const png_byte png_pCAL[5] = {112,  67,  65,  76, '\0'}
#  define PNG_sCAL const png_byte png_sCAL[5] = {115,  67,  65,  76, '\0'}
#  define PNG_pHYs const png_byte png_pHYs[5] = {112,  72,  89, 115, '\0'}
#  define PNG_sBIT const png_byte png_sBIT[5] = {115,  66,  73,  84, '\0'}
#  define PNG_sPLT const png_byte png_sPLT[5] = {115,  80,  76,  84, '\0'}
#  define PNG_sRGB const png_byte png_sRGB[5] = {115,  82,  71,  66, '\0'}
#  define PNG_tEXt const png_byte png_tEXt[5] = {116,  69,  88, 116, '\0'}
#  define PNG_tIME const png_byte png_tIME[5] = {116,  73,  77,  69, '\0'}
#  define PNG_tRNS const png_byte png_tRNS[5] = {116,  82,  78,  83, '\0'}
#  define PNG_zTXt const png_byte png_zTXt[5] = {122,  84,  88, 116, '\0'}

PNG_EXPORT_VAR (const png_byte FARDATA) png_IHDR[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_IDAT[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_IEND[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_PLTE[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_bKGD[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_cHRM[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_gAMA[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_hIST[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_iCCP[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_iTXt[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_oFFs[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_pCAL[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_sCAL[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_pHYs[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_sBIT[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_sPLT[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_sRGB[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_tEXt[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_tIME[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_tRNS[5];
PNG_EXPORT_VAR (const png_byte FARDATA) png_zTXt[5];

PNG_EXTERN png_int_32 png_get_int_32 PNGARG((png_bytep buf));
PNG_EXTERN png_uint_32 png_get_uint_32 PNGARG((png_bytep buf));
PNG_EXTERN png_uint_16 png_get_uint_16 PNGARG((png_bytep buf));
PNG_EXTERN png_uint_32 png_get_uint_31 PNGARG((png_structp png_ptr,
  png_bytep buf));

extern PNG_EXPORT(void,png_read_init) PNGARG((png_structp png_ptr));
#  undef png_read_init
#  define png_read_init(png_ptr) png_read_init_3(&png_ptr, \
    PNG_LIBPNG_VER_STRING,  png_sizeof(png_struct));
extern PNG_EXPORT(void,png_read_init_3) PNGARG((png_structpp ptr_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size));
extern PNG_EXPORT(void,png_read_init_2) PNGARG((png_structp png_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size, png_size_t
    png_info_size));

extern PNG_EXPORT(void,png_write_init) PNGARG((png_structp png_ptr));
#  undef png_write_init
#  define png_write_init(png_ptr) png_write_init_3(&png_ptr, \
    PNG_LIBPNG_VER_STRING, png_sizeof(png_struct));
extern PNG_EXPORT(void,png_write_init_3) PNGARG((png_structpp ptr_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size));
extern PNG_EXPORT(void,png_write_init_2) PNGARG((png_structp png_ptr,
    png_const_charp user_png_ver, png_size_t png_struct_size, png_size_t
    png_info_size));

PNG_EXTERN png_voidp png_create_struct PNGARG((int type));

PNG_EXTERN void png_destroy_struct PNGARG((png_voidp struct_ptr));

PNG_EXTERN png_voidp png_create_struct_2 PNGARG((int type, png_malloc_ptr
  malloc_fn, png_voidp mem_ptr));
PNG_EXTERN void png_destroy_struct_2 PNGARG((png_voidp struct_ptr,
   png_free_ptr free_fn, png_voidp mem_ptr));

PNG_EXTERN void png_info_destroy PNGARG((png_structp png_ptr,
   png_infop info_ptr));

PNG_EXTERN voidpf png_zalloc PNGARG((voidpf png_ptr, uInt items, uInt size));

PNG_EXTERN void png_zfree PNGARG((voidpf png_ptr, voidpf ptr));

PNG_EXTERN void png_reset_crc PNGARG((png_structp png_ptr));

PNG_EXTERN void png_write_data PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

PNG_EXTERN void png_read_data PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

PNG_EXTERN void png_crc_read PNGARG((png_structp png_ptr, png_bytep buf,
   png_size_t length));

PNG_EXTERN png_charp png_decompress_chunk PNGARG((png_structp png_ptr,
   int comp_type, png_charp chunkdata, png_size_t chunklength,
   png_size_t prefix_length, png_size_t *data_length));

PNG_EXTERN int png_crc_finish PNGARG((png_structp png_ptr, png_uint_32 skip));

PNG_EXTERN int png_crc_error PNGARG((png_structp png_ptr));

PNG_EXTERN void png_calculate_crc PNGARG((png_structp png_ptr, png_bytep ptr,
   png_size_t length));

PNG_EXTERN void png_flush PNGARG((png_structp png_ptr));

PNG_EXTERN void png_save_uint_32 PNGARG((png_bytep buf, png_uint_32 i));

PNG_EXTERN void png_save_int_32 PNGARG((png_bytep buf, png_int_32 i));

PNG_EXTERN void png_save_uint_16 PNGARG((png_bytep buf, unsigned int i));

PNG_EXTERN void png_write_sig PNGARG((png_structp png_ptr));

PNG_EXTERN void png_write_IHDR PNGARG((png_structp png_ptr, png_uint_32 width,
   png_uint_32 height,
   int bit_depth, int color_type, int compression_method, int filter_method,
   int interlace_method));

PNG_EXTERN void png_write_PLTE PNGARG((png_structp png_ptr, png_colorp palette,
   png_uint_32 num_pal));

PNG_EXTERN void png_write_IDAT PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

PNG_EXTERN void png_write_IEND PNGARG((png_structp png_ptr));

PNG_EXTERN void png_write_gAMA PNGARG((png_structp png_ptr, double file_gamma));
PNG_EXTERN void png_write_gAMA_fixed PNGARG((png_structp png_ptr, png_fixed_point
    file_gamma));

PNG_EXTERN void png_write_sBIT PNGARG((png_structp png_ptr, png_color_8p sbit,
   int color_type));

PNG_EXTERN void png_write_cHRM PNGARG((png_structp png_ptr,
   double white_x, double white_y,
   double red_x, double red_y, double green_x, double green_y,
   double blue_x, double blue_y));
PNG_EXTERN void png_write_cHRM_fixed PNGARG((png_structp png_ptr,
   png_fixed_point int_white_x, png_fixed_point int_white_y,
   png_fixed_point int_red_x, png_fixed_point int_red_y, png_fixed_point
   int_green_x, png_fixed_point int_green_y, png_fixed_point int_blue_x,
   png_fixed_point int_blue_y));

PNG_EXTERN void png_write_sRGB PNGARG((png_structp png_ptr,
   int intent));

PNG_EXTERN void png_write_iCCP PNGARG((png_structp png_ptr,
   png_charp name, int compression_type,
   png_charp profile, int proflen));

PNG_EXTERN void png_write_sPLT PNGARG((png_structp png_ptr,
   png_sPLT_tp palette));

PNG_EXTERN void png_write_tRNS PNGARG((png_structp png_ptr, png_bytep trans,
   png_color_16p values, int number, int color_type));

PNG_EXTERN void png_write_bKGD PNGARG((png_structp png_ptr,
   png_color_16p values, int color_type));

PNG_EXTERN void png_write_hIST PNGARG((png_structp png_ptr, png_uint_16p hist,
   int num_hist));

PNG_EXTERN png_size_t png_check_keyword PNGARG((png_structp png_ptr,
   png_charp key, png_charpp new_key));

PNG_EXTERN void png_write_tEXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_size_t text_len));

PNG_EXTERN void png_write_zTXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_size_t text_len, int compression));

PNG_EXTERN int png_set_text_2 PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp text_ptr, int num_text));

PNG_EXTERN void png_write_oFFs PNGARG((png_structp png_ptr,
   png_int_32 x_offset, png_int_32 y_offset, int unit_type));

PNG_EXTERN void png_write_pCAL PNGARG((png_structp png_ptr, png_charp purpose,
   png_int_32 X0, png_int_32 X1, int type, int nparams,
   png_charp units, png_charpp params));

PNG_EXTERN void png_write_pHYs PNGARG((png_structp png_ptr,
   png_uint_32 x_pixels_per_unit, png_uint_32 y_pixels_per_unit,
   int unit_type));

PNG_EXTERN void png_write_tIME PNGARG((png_structp png_ptr,
   png_timep mod_time));

PNG_EXTERN void png_write_sCAL_s PNGARG((png_structp png_ptr,
   int unit, png_charp width, png_charp height));

PNG_EXTERN void png_write_finish_row PNGARG((png_structp png_ptr));

PNG_EXTERN void png_write_start_row PNGARG((png_structp png_ptr));

PNG_EXTERN void png_build_gamma_table PNGARG((png_structp png_ptr));

PNG_EXTERN void png_combine_row PNGARG((png_structp png_ptr, png_bytep row,
   int mask));

PNG_EXTERN void png_do_read_interlace PNGARG((png_structp png_ptr));

PNG_EXTERN void png_do_write_interlace PNGARG((png_row_infop row_info,
   png_bytep row, int pass));

PNG_EXTERN void png_read_filter_row PNGARG((png_structp png_ptr,
   png_row_infop row_info, png_bytep row, png_bytep prev_row, int filter));

PNG_EXTERN void png_write_find_filter PNGARG((png_structp png_ptr,
   png_row_infop row_info));

PNG_EXTERN void png_write_filtered_row PNGARG((png_structp png_ptr,
   png_bytep filtered_row));

PNG_EXTERN void png_read_finish_row PNGARG((png_structp png_ptr));

PNG_EXTERN void png_read_start_row PNGARG((png_structp png_ptr));

PNG_EXTERN void png_read_transform_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));

PNG_EXTERN void png_do_read_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 filler, png_uint_32 flags));

PNG_EXTERN void png_do_read_swap_alpha PNGARG((png_row_infop row_info,
   png_bytep row));

PNG_EXTERN void png_do_read_invert_alpha PNGARG((png_row_infop row_info,
   png_bytep row));

PNG_EXTERN void png_do_strip_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 flags));

PNG_EXTERN void png_do_swap PNGARG((png_row_infop row_info, png_bytep row));

PNG_EXTERN void png_do_packswap PNGARG((png_row_infop row_info, png_bytep row));

PNG_EXTERN int png_do_rgb_to_gray PNGARG((png_structp png_ptr, png_row_infop
   row_info, png_bytep row));

PNG_EXTERN void png_do_gray_to_rgb PNGARG((png_row_infop row_info,
   png_bytep row));

PNG_EXTERN void png_do_unpack PNGARG((png_row_infop row_info, png_bytep row));

PNG_EXTERN void png_do_unshift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p sig_bits));

PNG_EXTERN void png_do_invert PNGARG((png_row_infop row_info, png_bytep row));

PNG_EXTERN void png_do_chop PNGARG((png_row_infop row_info, png_bytep row));

PNG_EXTERN void png_do_dither PNGARG((png_row_infop row_info,
   png_bytep row, png_bytep palette_lookup, png_bytep dither_lookup));

PNG_EXTERN void png_do_bgr PNGARG((png_row_infop row_info, png_bytep row));

PNG_EXTERN void png_do_background PNGARG((png_row_infop row_info, png_bytep row,
   png_color_16p trans_values, png_color_16p background,
   png_color_16p background_1,
   png_bytep gamma_table, png_bytep gamma_from_1, png_bytep gamma_to_1,
   png_uint_16pp gamma_16, png_uint_16pp gamma_16_from_1,
   png_uint_16pp gamma_16_to_1, int gamma_shift));

PNG_EXTERN void png_do_gamma PNGARG((png_row_infop row_info, png_bytep row,
   png_bytep gamma_table, png_uint_16pp gamma_16_table,
   int gamma_shift));

PNG_EXTERN void png_do_expand_palette PNGARG((png_row_infop row_info,
   png_bytep row, png_colorp palette, png_bytep trans, int num_trans));
PNG_EXTERN void png_do_expand PNGARG((png_row_infop row_info,
   png_bytep row, png_color_16p trans_value));

PNG_EXTERN void png_handle_IHDR PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_PLTE PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_IEND PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_bKGD PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_cHRM PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_gAMA PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_hIST PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

extern void png_handle_iCCP PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_oFFs PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_pCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_pHYs PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_sBIT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_sCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

extern void png_handle_sPLT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_sRGB PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_tEXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_tIME PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_tRNS PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_zTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

PNG_EXTERN void png_handle_unknown PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));

PNG_EXTERN void png_check_chunk_name PNGARG((png_structp png_ptr,
   png_bytep chunk_name));

PNG_EXTERN void png_do_read_transformations PNGARG((png_structp png_ptr));
PNG_EXTERN void png_do_write_transformations PNGARG((png_structp png_ptr));

PNG_EXTERN void png_init_read_transformations PNGARG((png_structp png_ptr));

PNG_EXTERN void png_do_read_intrapixel PNGARG((png_row_infop row_info,
   png_bytep row));
PNG_EXTERN void png_do_write_intrapixel PNGARG((png_row_infop row_info,
   png_bytep row));

PNG_EXTERN void png_init_mmx_flags PNGARG((png_structp png_ptr));

#endif

#endif
