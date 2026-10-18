// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jdmaster.c -- IJG libjpeg 6b jdmaster.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

typedef struct {
  struct jpeg_decomp_master pub;

  int pass_number;

  boolean using_merged_upsample;

  struct jpeg_color_quantizer * quantizer_1pass;
  struct jpeg_color_quantizer * quantizer_2pass;
} my_decomp_master;

typedef my_decomp_master * my_master_ptr;

/* use_merged_upsample. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f50c
LOCAL(boolean)
use_merged_upsample (j_decompress_ptr cinfo)
{

  if (cinfo->do_fancy_upsampling || cinfo->CCIR601_sampling)
    return FALSE;

  if (cinfo->jpeg_color_space != JCS_YCbCr || cinfo->num_components != 3 ||
      cinfo->out_color_space != JCS_RGB ||
      cinfo->out_color_components != RGB_PIXELSIZE)
    return FALSE;

  if (cinfo->comp_info[0].h_samp_factor != 2 ||
      cinfo->comp_info[1].h_samp_factor != 1 ||
      cinfo->comp_info[2].h_samp_factor != 1 ||
      cinfo->comp_info[0].v_samp_factor >  2 ||
      cinfo->comp_info[1].v_samp_factor != 1 ||
      cinfo->comp_info[2].v_samp_factor != 1)
    return FALSE;

  if (cinfo->comp_info[0].DCT_scaled_size != cinfo->min_DCT_scaled_size ||
      cinfo->comp_info[1].DCT_scaled_size != cinfo->min_DCT_scaled_size ||
      cinfo->comp_info[2].DCT_scaled_size != cinfo->min_DCT_scaled_size)
    return FALSE;

  return TRUE;
}

/* jpeg_calc_output_dimensions. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f31c
GLOBAL(void)
jpeg_calc_output_dimensions (j_decompress_ptr cinfo)

{
  int ci;
  jpeg_component_info *compptr;

  if (cinfo->global_state != DSTATE_READY)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  if (cinfo->scale_num * 8 <= cinfo->scale_denom) {

    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, 8L);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, 8L);
    cinfo->min_DCT_scaled_size = 1;
  } else if (cinfo->scale_num * 4 <= cinfo->scale_denom) {

    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, 4L);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, 4L);
    cinfo->min_DCT_scaled_size = 2;
  } else if (cinfo->scale_num * 2 <= cinfo->scale_denom) {

    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, 2L);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, 2L);
    cinfo->min_DCT_scaled_size = 4;
  } else {

    cinfo->output_width = cinfo->image_width;
    cinfo->output_height = cinfo->image_height;
    cinfo->min_DCT_scaled_size = DCTSIZE;
  }

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    int ssize = cinfo->min_DCT_scaled_size;
    while (ssize < DCTSIZE &&
	   (compptr->h_samp_factor * ssize * 2 <=
	    cinfo->max_h_samp_factor * cinfo->min_DCT_scaled_size) &&
	   (compptr->v_samp_factor * ssize * 2 <=
	    cinfo->max_v_samp_factor * cinfo->min_DCT_scaled_size)) {
      ssize = ssize * 2;
    }
    compptr->DCT_scaled_size = ssize;
  }

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {

    compptr->downsampled_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width *
		    (long) (compptr->h_samp_factor * compptr->DCT_scaled_size),
		    (long) (cinfo->max_h_samp_factor * DCTSIZE));
    compptr->downsampled_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height *
		    (long) (compptr->v_samp_factor * compptr->DCT_scaled_size),
		    (long) (cinfo->max_v_samp_factor * DCTSIZE));
  }

  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    cinfo->out_color_components = 1;
    break;
  case JCS_RGB:
  case JCS_YCbCr:
    cinfo->out_color_components = 3;
    break;
  case JCS_CMYK:
  case JCS_YCCK:
    cinfo->out_color_components = 4;
    break;
  default:
    cinfo->out_color_components = cinfo->num_components;
    break;
  }
  cinfo->output_components = (cinfo->quantize_colors ? 1 :
			      cinfo->out_color_components);

  if (use_merged_upsample(cinfo))
    cinfo->rec_outbuf_height = cinfo->max_v_samp_factor;
  else
    cinfo->rec_outbuf_height = 1;
}

/* prepare_range_limit_table. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f289
LOCAL(void)
prepare_range_limit_table (j_decompress_ptr cinfo)

{
  JSAMPLE * table;
  int i;

  table = (JSAMPLE *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
		(5 * (MAXJSAMPLE+1) + CENTERJSAMPLE) * SIZEOF(JSAMPLE));
  table += (MAXJSAMPLE+1);
  cinfo->sample_range_limit = table;

  MEMZERO(table - (MAXJSAMPLE+1), (MAXJSAMPLE+1) * SIZEOF(JSAMPLE));

  for (i = 0; i <= MAXJSAMPLE; i++)
    table[i] = (JSAMPLE) i;
  table += CENTERJSAMPLE;

  for (i = CENTERJSAMPLE; i < 2*(MAXJSAMPLE+1); i++)
    table[i] = MAXJSAMPLE;

  MEMZERO(table + (2 * (MAXJSAMPLE+1)),
	  (2 * (MAXJSAMPLE+1) - CENTERJSAMPLE) * SIZEOF(JSAMPLE));
  MEMCOPY(table + (4 * (MAXJSAMPLE+1) - CENTERJSAMPLE),
	  cinfo->sample_range_limit, CENTERJSAMPLE * SIZEOF(JSAMPLE));
}

/* master_selection. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054f083
LOCAL(void)
master_selection (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;
  boolean use_c_buffer;
  long samplesperrow;
  JDIMENSION jd_samplesperrow;

  jpeg_calc_output_dimensions(cinfo);
  prepare_range_limit_table(cinfo);

  samplesperrow = (long) cinfo->output_width * (long) cinfo->out_color_components;
  jd_samplesperrow = (JDIMENSION) samplesperrow;
  if ((long) jd_samplesperrow != samplesperrow)
    ERREXIT(cinfo, JERR_WIDTH_OVERFLOW);

  master->pass_number = 0;
  master->using_merged_upsample = use_merged_upsample(cinfo);

  master->quantizer_1pass = NULL;
  master->quantizer_2pass = NULL;

  if (! cinfo->quantize_colors || ! cinfo->buffered_image) {
    cinfo->enable_1pass_quant = FALSE;
    cinfo->enable_external_quant = FALSE;
    cinfo->enable_2pass_quant = FALSE;
  }
  if (cinfo->quantize_colors) {
    if (cinfo->raw_data_out)
      ERREXIT(cinfo, JERR_NOTIMPL);

    if (cinfo->out_color_components != 3) {
      cinfo->enable_1pass_quant = TRUE;
      cinfo->enable_external_quant = FALSE;
      cinfo->enable_2pass_quant = FALSE;
      cinfo->colormap = NULL;
    } else if (cinfo->colormap != NULL) {
      cinfo->enable_external_quant = TRUE;
    } else if (cinfo->two_pass_quantize) {
      cinfo->enable_2pass_quant = TRUE;
    } else {
      cinfo->enable_1pass_quant = TRUE;
    }

    if (cinfo->enable_1pass_quant) {
      jinit_1pass_quantizer(cinfo);
      master->quantizer_1pass = cinfo->cquantize;
    }

    if (cinfo->enable_2pass_quant || cinfo->enable_external_quant) {
      jinit_2pass_quantizer(cinfo);
      master->quantizer_2pass = cinfo->cquantize;
    }

  }

  if (! cinfo->raw_data_out) {
    if (master->using_merged_upsample) {
      jinit_merged_upsampler(cinfo);
    } else {
      jinit_color_deconverter(cinfo);
      jinit_upsampler(cinfo);
    }
    jinit_d_post_controller(cinfo, cinfo->enable_2pass_quant);
  }

  jinit_inverse_dct(cinfo);

  if (cinfo->arith_code) {
    ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
  } else {
    if (cinfo->progressive_mode) {
      jinit_phuff_decoder(cinfo);
    } else
      jinit_huff_decoder(cinfo);
  }

  use_c_buffer = cinfo->inputctl->has_multiple_scans || cinfo->buffered_image;
  jinit_d_coef_controller(cinfo, use_c_buffer);

  if (! cinfo->raw_data_out)
    jinit_d_main_controller(cinfo, FALSE  );

  (*cinfo->mem->realize_virt_arrays) ((j_common_ptr) cinfo);

  (*cinfo->inputctl->start_input_pass) (cinfo);

  if (cinfo->progress != NULL && ! cinfo->buffered_image &&
      cinfo->inputctl->has_multiple_scans) {
    int nscans;

    if (cinfo->progressive_mode) {

      nscans = 2 + 3 * cinfo->num_components;
    } else {

      nscans = cinfo->num_components;
    }
    cinfo->progress->pass_counter = 0L;
    cinfo->progress->pass_limit = (long) cinfo->total_iMCU_rows * nscans;
    cinfo->progress->completed_passes = 0;
    cinfo->progress->total_passes = (cinfo->enable_2pass_quant ? 3 : 2);

    master->pass_number++;
  }
}

/* prepare_for_output_pass. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054ef35
METHODDEF(void)
prepare_for_output_pass (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  if (master->pub.is_dummy_pass) {

    master->pub.is_dummy_pass = FALSE;
    (*cinfo->cquantize->start_pass) (cinfo, FALSE);
    (*cinfo->post->start_pass) (cinfo, JBUF_CRANK_DEST);
    (*cinfo->main->start_pass) (cinfo, JBUF_CRANK_DEST);
  } else {
    if (cinfo->quantize_colors && cinfo->colormap == NULL) {

      if (cinfo->two_pass_quantize && cinfo->enable_2pass_quant) {
	cinfo->cquantize = master->quantizer_2pass;
	master->pub.is_dummy_pass = TRUE;
      } else if (cinfo->enable_1pass_quant) {
	cinfo->cquantize = master->quantizer_1pass;
      } else {
	ERREXIT(cinfo, JERR_MODE_CHANGE);
      }
    }
    (*cinfo->idct->start_pass) (cinfo);
    (*cinfo->coef->start_output_pass) (cinfo);
    if (! cinfo->raw_data_out) {
      if (! master->using_merged_upsample)
	(*cinfo->cconvert->start_pass) (cinfo);
      (*cinfo->upsample->start_pass) (cinfo);
      if (cinfo->quantize_colors)
	(*cinfo->cquantize->start_pass) (cinfo, master->pub.is_dummy_pass);
      (*cinfo->post->start_pass) (cinfo,
	    (master->pub.is_dummy_pass ? JBUF_SAVE_AND_PASS : JBUF_PASS_THRU));
      (*cinfo->main->start_pass) (cinfo, JBUF_PASS_THRU);
    }
  }

  if (cinfo->progress != NULL) {
    cinfo->progress->completed_passes = master->pass_number;
    cinfo->progress->total_passes = master->pass_number +
				    (master->pub.is_dummy_pass ? 2 : 1);

    if (cinfo->buffered_image && ! cinfo->inputctl->eoi_reached) {
      cinfo->progress->total_passes += (cinfo->enable_2pass_quant ? 2 : 1);
    }
  }
}

/* finish_output_pass. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054ef12
METHODDEF(void)
finish_output_pass (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  if (cinfo->quantize_colors)
    (*cinfo->cquantize->finish_pass) (cinfo);
  master->pass_number++;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(void)
jpeg_new_colormap (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  if (cinfo->global_state != DSTATE_BUFIMAGE)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  if (cinfo->quantize_colors && cinfo->enable_external_quant &&
      cinfo->colormap != NULL) {

    cinfo->cquantize = master->quantizer_2pass;

    (*cinfo->cquantize->new_color_map) (cinfo);
    master->pub.is_dummy_pass = FALSE;
  } else
    ERREXIT(cinfo, JERR_MODE_CHANGE);
}

/* jinit_master_decompress. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054eedd
GLOBAL(void)
jinit_master_decompress (j_decompress_ptr cinfo)
{
  my_master_ptr master;

  master = (my_master_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(my_decomp_master));
  cinfo->master = (struct jpeg_decomp_master *) master;
  master->pub.prepare_for_output_pass = prepare_for_output_pass;
  master->pub.finish_output_pass = finish_output_pass;

  master->pub.is_dummy_pass = FALSE;

  master_selection(cinfo);
}
