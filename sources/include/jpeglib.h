/*
 * The Independent JPEG Group's JPEG software, release 6b of 27-Mar-1998.
 *
 * This copy was altered for the EV Nova decompilation: comments, the #if branches the
 * EV Nova build does not take and the declarations of functions these sources do not
 * define were removed (the functions it does not link stay, unmarked); jinclude.h routes MEMZERO, MEMCOPY, malloc and free to the private runtime the
 * exe links (jpegcrt.c). The LEGAL ISSUES section of the IJG README follows unaltered.
 *
 * LEGAL ISSUES
 * ============
 *
 * In plain English:
 *
 * 1. We don't promise that this software works.  (But if you find any bugs,
 *    please let us know!)
 * 2. You can use this software for whatever you want.  You don't have to pay us.
 * 3. You may not pretend that you wrote this software.  If you use it in a
 *    program, you must acknowledge somewhere in your documentation that
 *    you've used the IJG code.
 *
 * In legalese:
 *
 * The authors make NO WARRANTY or representation, either express or implied,
 * with respect to this software, its quality, accuracy, merchantability, or
 * fitness for a particular purpose.  This software is provided "AS IS", and you,
 * its user, assume the entire risk as to its quality and accuracy.
 *
 * This software is copyright (C) 1991-1998, Thomas G. Lane.
 * All Rights Reserved except as specified below.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * software (or portions thereof) for any purpose, without fee, subject to these
 * conditions:
 * (1) If any part of the source code for this software is distributed, then this
 * README file must be included, with this copyright and no-warranty notice
 * unaltered; and any additions, deletions, or changes to the original files
 * must be clearly indicated in accompanying documentation.
 * (2) If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the work of
 * the Independent JPEG Group".
 * (3) Permission for use of this software is granted only if the user accepts
 * full responsibility for any undesirable consequences; the authors accept
 * NO LIABILITY for damages of any kind.
 *
 * These conditions apply to any software derived from or based on the IJG code,
 * not just to the unmodified library.  If you use our work, you ought to
 * acknowledge us.
 *
 * Permission is NOT granted for the use of any IJG author's name or company name
 * in advertising or publicity relating to this software or products derived from
 * it.  This software may be referred to only as "the Independent JPEG Group's
 * software".
 *
 * We specifically permit and encourage the use of this software as the basis of
 * commercial products, provided that all warranty or liability claims are
 * assumed by the product vendor.
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 */
#ifndef JPEGLIB_H
#define JPEGLIB_H

#ifndef JCONFIG_INCLUDED
#  include "jconfig.h"
#endif
#include "jmorecfg.h"

#define JPEG_LIB_VERSION  62

#define DCTSIZE		    8
#define DCTSIZE2	    64
#define NUM_QUANT_TBLS      4
#define NUM_HUFF_TBLS       4
#define NUM_ARITH_TBLS      16
#define MAX_COMPS_IN_SCAN   4
#define MAX_SAMP_FACTOR     4

#define C_MAX_BLOCKS_IN_MCU   10
#define D_MAX_BLOCKS_IN_MCU   10

typedef JSAMPLE FAR *JSAMPROW;
typedef JSAMPROW *JSAMPARRAY;
typedef JSAMPARRAY *JSAMPIMAGE;

typedef JCOEF JBLOCK[DCTSIZE2];
typedef JBLOCK FAR *JBLOCKROW;
typedef JBLOCKROW *JBLOCKARRAY;
typedef JBLOCKARRAY *JBLOCKIMAGE;

typedef JCOEF FAR *JCOEFPTR;

typedef struct {

  UINT16 quantval[DCTSIZE2];

  boolean sent_table;
} JQUANT_TBL;

typedef struct {

  UINT8 bits[17];

  UINT8 huffval[256];

  boolean sent_table;
} JHUFF_TBL;

typedef struct {

  int component_id;
  int component_index;
  int h_samp_factor;
  int v_samp_factor;
  int quant_tbl_no;

  int dc_tbl_no;
  int ac_tbl_no;

  JDIMENSION width_in_blocks;
  JDIMENSION height_in_blocks;

  int DCT_scaled_size;

  JDIMENSION downsampled_width;
  JDIMENSION downsampled_height;

  boolean component_needed;

  int MCU_width;
  int MCU_height;
  int MCU_blocks;
  int MCU_sample_width;
  int last_col_width;
  int last_row_height;

  JQUANT_TBL * quant_table;

  void * dct_table;
} jpeg_component_info;

typedef struct {
  int comps_in_scan;
  int component_index[MAX_COMPS_IN_SCAN];
  int Ss, Se;
  int Ah, Al;
} jpeg_scan_info;

typedef struct jpeg_marker_struct FAR * jpeg_saved_marker_ptr;

struct jpeg_marker_struct {
  jpeg_saved_marker_ptr next;
  UINT8 marker;
  unsigned int original_length;
  unsigned int data_length;
  JOCTET FAR * data;

};

typedef enum {
	JCS_UNKNOWN,
	JCS_GRAYSCALE,
	JCS_RGB,
	JCS_YCbCr,
	JCS_CMYK,
	JCS_YCCK
} J_COLOR_SPACE;

typedef enum {
	JDCT_ISLOW,
	JDCT_IFAST,
	JDCT_FLOAT
} J_DCT_METHOD;

#define JDCT_DEFAULT  JDCT_ISLOW
#define JDCT_FASTEST  JDCT_IFAST

typedef enum {
	JDITHER_NONE,
	JDITHER_ORDERED,
	JDITHER_FS
} J_DITHER_MODE;

#define jpeg_common_fields \
  struct jpeg_error_mgr * err;	 \
  struct jpeg_memory_mgr * mem;	 \
  struct jpeg_progress_mgr * progress;  \
  void * client_data;		 \
  boolean is_decompressor;	 \
  int global_state

struct jpeg_common_struct {
  jpeg_common_fields;

};

typedef struct jpeg_common_struct * j_common_ptr;
typedef struct jpeg_compress_struct * j_compress_ptr;
typedef struct jpeg_decompress_struct * j_decompress_ptr;

struct jpeg_compress_struct {
  jpeg_common_fields;

  struct jpeg_destination_mgr * dest;

  JDIMENSION image_width;
  JDIMENSION image_height;
  int input_components;
  J_COLOR_SPACE in_color_space;

  double input_gamma;

  int data_precision;

  int num_components;
  J_COLOR_SPACE jpeg_color_space;

  jpeg_component_info * comp_info;

  JQUANT_TBL * quant_tbl_ptrs[NUM_QUANT_TBLS];

  JHUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
  JHUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];

  UINT8 arith_dc_L[NUM_ARITH_TBLS];
  UINT8 arith_dc_U[NUM_ARITH_TBLS];
  UINT8 arith_ac_K[NUM_ARITH_TBLS];

  int num_scans;
  const jpeg_scan_info * scan_info;

  boolean raw_data_in;
  boolean arith_code;
  boolean optimize_coding;
  boolean CCIR601_sampling;
  int smoothing_factor;
  J_DCT_METHOD dct_method;

  unsigned int restart_interval;
  int restart_in_rows;

  boolean write_JFIF_header;
  UINT8 JFIF_major_version;
  UINT8 JFIF_minor_version;

  UINT8 density_unit;
  UINT16 X_density;
  UINT16 Y_density;
  boolean write_Adobe_marker;

  JDIMENSION next_scanline;

  boolean progressive_mode;
  int max_h_samp_factor;
  int max_v_samp_factor;

  JDIMENSION total_iMCU_rows;

  int comps_in_scan;
  jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN];

  JDIMENSION MCUs_per_row;
  JDIMENSION MCU_rows_in_scan;

  int blocks_in_MCU;
  int MCU_membership[C_MAX_BLOCKS_IN_MCU];

  int Ss, Se, Ah, Al;

  struct jpeg_comp_master * master;
  struct jpeg_c_main_controller * main;
  struct jpeg_c_prep_controller * prep;
  struct jpeg_c_coef_controller * coef;
  struct jpeg_marker_writer * marker;
  struct jpeg_color_converter * cconvert;
  struct jpeg_downsampler * downsample;
  struct jpeg_forward_dct * fdct;
  struct jpeg_entropy_encoder * entropy;
  jpeg_scan_info * script_space;
  int script_space_size;
};

struct jpeg_decompress_struct {
  jpeg_common_fields;

  struct jpeg_source_mgr * src;

  JDIMENSION image_width;
  JDIMENSION image_height;
  int num_components;
  J_COLOR_SPACE jpeg_color_space;

  J_COLOR_SPACE out_color_space;

  unsigned int scale_num, scale_denom;

  double output_gamma;

  boolean buffered_image;
  boolean raw_data_out;

  J_DCT_METHOD dct_method;
  boolean do_fancy_upsampling;
  boolean do_block_smoothing;

  boolean quantize_colors;

  J_DITHER_MODE dither_mode;
  boolean two_pass_quantize;
  int desired_number_of_colors;

  boolean enable_1pass_quant;
  boolean enable_external_quant;
  boolean enable_2pass_quant;

  JDIMENSION output_width;
  JDIMENSION output_height;
  int out_color_components;
  int output_components;

  int rec_outbuf_height;

  int actual_number_of_colors;
  JSAMPARRAY colormap;

  JDIMENSION output_scanline;

  int input_scan_number;
  JDIMENSION input_iMCU_row;

  int output_scan_number;
  JDIMENSION output_iMCU_row;

  int (*coef_bits)[DCTSIZE2];

  JQUANT_TBL * quant_tbl_ptrs[NUM_QUANT_TBLS];

  JHUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
  JHUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];

  int data_precision;

  jpeg_component_info * comp_info;

  boolean progressive_mode;
  boolean arith_code;

  UINT8 arith_dc_L[NUM_ARITH_TBLS];
  UINT8 arith_dc_U[NUM_ARITH_TBLS];
  UINT8 arith_ac_K[NUM_ARITH_TBLS];

  unsigned int restart_interval;

  boolean saw_JFIF_marker;

  UINT8 JFIF_major_version;
  UINT8 JFIF_minor_version;
  UINT8 density_unit;
  UINT16 X_density;
  UINT16 Y_density;
  boolean saw_Adobe_marker;
  UINT8 Adobe_transform;

  boolean CCIR601_sampling;

  jpeg_saved_marker_ptr marker_list;

  int max_h_samp_factor;
  int max_v_samp_factor;

  int min_DCT_scaled_size;

  JDIMENSION total_iMCU_rows;

  JSAMPLE * sample_range_limit;

  int comps_in_scan;
  jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN];

  JDIMENSION MCUs_per_row;
  JDIMENSION MCU_rows_in_scan;

  int blocks_in_MCU;
  int MCU_membership[D_MAX_BLOCKS_IN_MCU];

  int Ss, Se, Ah, Al;

  int unread_marker;

  struct jpeg_decomp_master * master;
  struct jpeg_d_main_controller * main;
  struct jpeg_d_coef_controller * coef;
  struct jpeg_d_post_controller * post;
  struct jpeg_input_controller * inputctl;
  struct jpeg_marker_reader * marker;
  struct jpeg_entropy_decoder * entropy;
  struct jpeg_inverse_dct * idct;
  struct jpeg_upsampler * upsample;
  struct jpeg_color_deconverter * cconvert;
  struct jpeg_color_quantizer * cquantize;
};

struct jpeg_error_mgr {

  JMETHOD(void, error_exit, (j_common_ptr cinfo));

  JMETHOD(void, emit_message, (j_common_ptr cinfo, int msg_level));

  JMETHOD(void, output_message, (j_common_ptr cinfo));

  JMETHOD(void, format_message, (j_common_ptr cinfo, char * buffer));
#define JMSG_LENGTH_MAX  200

  JMETHOD(void, reset_error_mgr, (j_common_ptr cinfo));

  int msg_code;
#define JMSG_STR_PARM_MAX  80
  union {
    int i[8];
    char s[JMSG_STR_PARM_MAX];
  } msg_parm;

  int trace_level;

  long num_warnings;

  const char * const * jpeg_message_table;
  int last_jpeg_message;

  const char * const * addon_message_table;
  int first_addon_message;
  int last_addon_message;
};

struct jpeg_progress_mgr {
  JMETHOD(void, progress_monitor, (j_common_ptr cinfo));

  long pass_counter;
  long pass_limit;
  int completed_passes;
  int total_passes;
};

struct jpeg_destination_mgr {
  JOCTET * next_output_byte;
  size_t free_in_buffer;

  JMETHOD(void, init_destination, (j_compress_ptr cinfo));
  JMETHOD(boolean, empty_output_buffer, (j_compress_ptr cinfo));
  JMETHOD(void, term_destination, (j_compress_ptr cinfo));
};

struct jpeg_source_mgr {
  const JOCTET * next_input_byte;
  size_t bytes_in_buffer;

  JMETHOD(void, init_source, (j_decompress_ptr cinfo));
  JMETHOD(boolean, fill_input_buffer, (j_decompress_ptr cinfo));
  JMETHOD(void, skip_input_data, (j_decompress_ptr cinfo, long num_bytes));
  JMETHOD(boolean, resync_to_restart, (j_decompress_ptr cinfo, int desired));
  JMETHOD(void, term_source, (j_decompress_ptr cinfo));
};

#define JPOOL_PERMANENT	0
#define JPOOL_IMAGE	1
#define JPOOL_NUMPOOLS	2

typedef struct jvirt_sarray_control * jvirt_sarray_ptr;
typedef struct jvirt_barray_control * jvirt_barray_ptr;

struct jpeg_memory_mgr {

  JMETHOD(void *, alloc_small, (j_common_ptr cinfo, int pool_id,
				size_t sizeofobject));
  JMETHOD(void FAR *, alloc_large, (j_common_ptr cinfo, int pool_id,
				     size_t sizeofobject));
  JMETHOD(JSAMPARRAY, alloc_sarray, (j_common_ptr cinfo, int pool_id,
				     JDIMENSION samplesperrow,
				     JDIMENSION numrows));
  JMETHOD(JBLOCKARRAY, alloc_barray, (j_common_ptr cinfo, int pool_id,
				      JDIMENSION blocksperrow,
				      JDIMENSION numrows));
  JMETHOD(jvirt_sarray_ptr, request_virt_sarray, (j_common_ptr cinfo,
						  int pool_id,
						  boolean pre_zero,
						  JDIMENSION samplesperrow,
						  JDIMENSION numrows,
						  JDIMENSION maxaccess));
  JMETHOD(jvirt_barray_ptr, request_virt_barray, (j_common_ptr cinfo,
						  int pool_id,
						  boolean pre_zero,
						  JDIMENSION blocksperrow,
						  JDIMENSION numrows,
						  JDIMENSION maxaccess));
  JMETHOD(void, realize_virt_arrays, (j_common_ptr cinfo));
  JMETHOD(JSAMPARRAY, access_virt_sarray, (j_common_ptr cinfo,
					   jvirt_sarray_ptr ptr,
					   JDIMENSION start_row,
					   JDIMENSION num_rows,
					   boolean writable));
  JMETHOD(JBLOCKARRAY, access_virt_barray, (j_common_ptr cinfo,
					    jvirt_barray_ptr ptr,
					    JDIMENSION start_row,
					    JDIMENSION num_rows,
					    boolean writable));
  JMETHOD(void, free_pool, (j_common_ptr cinfo, int pool_id));
  JMETHOD(void, self_destruct, (j_common_ptr cinfo));

  long max_memory_to_use;

  long max_alloc_chunk;
};

typedef JMETHOD(boolean, jpeg_marker_parser_method, (j_decompress_ptr cinfo));

#define JPP(arglist)	arglist

EXTERN(struct jpeg_error_mgr *) jpeg_std_error
	JPP((struct jpeg_error_mgr * err));

#define jpeg_create_compress(cinfo) \
    jpeg_CreateCompress((cinfo), JPEG_LIB_VERSION, \
			(size_t) sizeof(struct jpeg_compress_struct))
#define jpeg_create_decompress(cinfo) \
    jpeg_CreateDecompress((cinfo), JPEG_LIB_VERSION, \
			  (size_t) sizeof(struct jpeg_decompress_struct))
EXTERN(void) jpeg_CreateDecompress JPP((j_decompress_ptr cinfo,
					int version, size_t structsize));

EXTERN(void) jpeg_destroy_decompress JPP((j_decompress_ptr cinfo));

EXTERN(void) jpeg_set_defaults JPP((j_compress_ptr cinfo));

EXTERN(void) jpeg_set_colorspace JPP((j_compress_ptr cinfo,
				      J_COLOR_SPACE colorspace));
EXTERN(void) jpeg_default_colorspace JPP((j_compress_ptr cinfo));
EXTERN(void) jpeg_set_quality JPP((j_compress_ptr cinfo, int quality,
				   boolean force_baseline));
EXTERN(void) jpeg_set_linear_quality JPP((j_compress_ptr cinfo,
					  int scale_factor,
					  boolean force_baseline));
EXTERN(void) jpeg_add_quant_table JPP((j_compress_ptr cinfo, int which_tbl,
				       const unsigned int *basic_table,
				       int scale_factor,
				       boolean force_baseline));
EXTERN(int) jpeg_quality_scaling JPP((int quality));
EXTERN(void) jpeg_simple_progression JPP((j_compress_ptr cinfo));
EXTERN(JQUANT_TBL *) jpeg_alloc_quant_table JPP((j_common_ptr cinfo));
EXTERN(JHUFF_TBL *) jpeg_alloc_huff_table JPP((j_common_ptr cinfo));

EXTERN(int) jpeg_read_header JPP((j_decompress_ptr cinfo,
				  boolean require_image));

#define JPEG_SUSPENDED		0
#define JPEG_HEADER_OK		1
#define JPEG_HEADER_TABLES_ONLY	2

EXTERN(boolean) jpeg_start_decompress JPP((j_decompress_ptr cinfo));
EXTERN(JDIMENSION) jpeg_read_scanlines JPP((j_decompress_ptr cinfo,
					    JSAMPARRAY scanlines,
					    JDIMENSION max_lines));
EXTERN(boolean) jpeg_finish_decompress JPP((j_decompress_ptr cinfo));

EXTERN(JDIMENSION) jpeg_read_raw_data JPP((j_decompress_ptr cinfo,
					   JSAMPIMAGE data,
					   JDIMENSION max_lines));

EXTERN(boolean) jpeg_has_multiple_scans JPP((j_decompress_ptr cinfo));
EXTERN(boolean) jpeg_start_output JPP((j_decompress_ptr cinfo,
				       int scan_number));
EXTERN(boolean) jpeg_finish_output JPP((j_decompress_ptr cinfo));
EXTERN(boolean) jpeg_input_complete JPP((j_decompress_ptr cinfo));
EXTERN(void) jpeg_new_colormap JPP((j_decompress_ptr cinfo));
EXTERN(int) jpeg_consume_input JPP((j_decompress_ptr cinfo));

#define JPEG_REACHED_SOS	1
#define JPEG_REACHED_EOI	2
#define JPEG_ROW_COMPLETED	3
#define JPEG_SCAN_COMPLETED	4

EXTERN(void) jpeg_calc_output_dimensions JPP((j_decompress_ptr cinfo));

EXTERN(void) jpeg_save_markers
	JPP((j_decompress_ptr cinfo, int marker_code,
	     unsigned int length_limit));

EXTERN(void) jpeg_set_marker_processor
	JPP((j_decompress_ptr cinfo, int marker_code,
	     jpeg_marker_parser_method routine));

EXTERN(void) jpeg_abort_decompress JPP((j_decompress_ptr cinfo));

EXTERN(void) jpeg_abort JPP((j_common_ptr cinfo));
EXTERN(void) jpeg_destroy JPP((j_common_ptr cinfo));

EXTERN(boolean) jpeg_resync_to_restart JPP((j_decompress_ptr cinfo,
					    int desired));

#define JPEG_RST0	0xD0
#define JPEG_EOI	0xD9
#define JPEG_APP0	0xE0
#define JPEG_COM	0xFE

#ifdef JPEG_INTERNALS
#  include "jpegint.h"
#  include "jerror.h"
#endif

#endif
