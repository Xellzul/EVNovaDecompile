// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jdapimin.c -- IJG libjpeg 6b jdapimin.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

/* jpeg_CreateDecompress. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00535193
GLOBAL(void)
jpeg_CreateDecompress (j_decompress_ptr cinfo, int version, size_t structsize)
{
  int i;

  cinfo->mem = NULL;
  if (version != JPEG_LIB_VERSION)
    ERREXIT2(cinfo, JERR_BAD_LIB_VERSION, JPEG_LIB_VERSION, version);
  if (structsize != SIZEOF(struct jpeg_decompress_struct))
    ERREXIT2(cinfo, JERR_BAD_STRUCT_SIZE,
	     (int) SIZEOF(struct jpeg_decompress_struct), (int) structsize);

  {
    struct jpeg_error_mgr * err = cinfo->err;
    void * client_data = cinfo->client_data;
    MEMZERO(cinfo, SIZEOF(struct jpeg_decompress_struct));
    cinfo->err = err;
    cinfo->client_data = client_data;
  }
  cinfo->is_decompressor = TRUE;

  jinit_memory_mgr((j_common_ptr) cinfo);

  cinfo->progress = NULL;
  cinfo->src = NULL;

  for (i = 0; i < NUM_QUANT_TBLS; i++)
    cinfo->quant_tbl_ptrs[i] = NULL;

  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    cinfo->dc_huff_tbl_ptrs[i] = NULL;
    cinfo->ac_huff_tbl_ptrs[i] = NULL;
  }

  cinfo->marker_list = NULL;
  jinit_marker_reader(cinfo);

  jinit_input_controller(cinfo);

  cinfo->global_state = DSTATE_START;
}

/* jpeg_destroy_decompress. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00535188
GLOBAL(void)
jpeg_destroy_decompress (j_decompress_ptr cinfo)
{
  jpeg_destroy((j_common_ptr) cinfo);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(void)
jpeg_abort_decompress (j_decompress_ptr cinfo)
{
  jpeg_abort((j_common_ptr) cinfo);
}

/* default_decompress_parms. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00534ff1
LOCAL(void)
default_decompress_parms (j_decompress_ptr cinfo)
{

  switch (cinfo->num_components) {
  case 1:
    cinfo->jpeg_color_space = JCS_GRAYSCALE;
    cinfo->out_color_space = JCS_GRAYSCALE;
    break;

  case 3:
    if (cinfo->saw_JFIF_marker) {
      cinfo->jpeg_color_space = JCS_YCbCr;
    } else if (cinfo->saw_Adobe_marker) {
      switch (cinfo->Adobe_transform) {
      case 0:
	cinfo->jpeg_color_space = JCS_RGB;
	break;
      case 1:
	cinfo->jpeg_color_space = JCS_YCbCr;
	break;
      default:
	WARNMS1(cinfo, JWRN_ADOBE_XFORM, cinfo->Adobe_transform);
	cinfo->jpeg_color_space = JCS_YCbCr;
	break;
      }
    } else {

      int cid0 = cinfo->comp_info[0].component_id;
      int cid1 = cinfo->comp_info[1].component_id;
      int cid2 = cinfo->comp_info[2].component_id;

      if (cid0 == 1 && cid1 == 2 && cid2 == 3)
	cinfo->jpeg_color_space = JCS_YCbCr;
      else if (cid0 == 82 && cid1 == 71 && cid2 == 66)
	cinfo->jpeg_color_space = JCS_RGB;
      else {
	TRACEMS3(cinfo, 1, JTRC_UNKNOWN_IDS, cid0, cid1, cid2);
	cinfo->jpeg_color_space = JCS_YCbCr;
      }
    }

    cinfo->out_color_space = JCS_RGB;
    break;

  case 4:
    if (cinfo->saw_Adobe_marker) {
      switch (cinfo->Adobe_transform) {
      case 0:
	cinfo->jpeg_color_space = JCS_CMYK;
	break;
      case 2:
	cinfo->jpeg_color_space = JCS_YCCK;
	break;
      default:
	WARNMS1(cinfo, JWRN_ADOBE_XFORM, cinfo->Adobe_transform);
	cinfo->jpeg_color_space = JCS_YCCK;
	break;
      }
    } else {

      cinfo->jpeg_color_space = JCS_CMYK;
    }
    cinfo->out_color_space = JCS_CMYK;
    break;

  default:
    cinfo->jpeg_color_space = JCS_UNKNOWN;
    cinfo->out_color_space = JCS_UNKNOWN;
    break;
  }

  cinfo->scale_num = 1;
  cinfo->scale_denom = 1;
  cinfo->output_gamma = 1.0;
  cinfo->buffered_image = FALSE;
  cinfo->raw_data_out = FALSE;
  cinfo->dct_method = JDCT_DEFAULT;
  cinfo->do_fancy_upsampling = TRUE;
  cinfo->do_block_smoothing = TRUE;
  cinfo->quantize_colors = FALSE;

  cinfo->dither_mode = JDITHER_FS;
  cinfo->two_pass_quantize = TRUE;
  cinfo->desired_number_of_colors = 256;
  cinfo->colormap = NULL;

  cinfo->enable_1pass_quant = FALSE;
  cinfo->enable_external_quant = FALSE;
  cinfo->enable_2pass_quant = FALSE;
}

/* jpeg_read_header. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00534f7c
GLOBAL(int)
jpeg_read_header (j_decompress_ptr cinfo, boolean require_image)
{
  int retcode;

  if (cinfo->global_state != DSTATE_START &&
      cinfo->global_state != DSTATE_INHEADER)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  retcode = jpeg_consume_input(cinfo);

  switch (retcode) {
  case JPEG_REACHED_SOS:
    retcode = JPEG_HEADER_OK;
    break;
  case JPEG_REACHED_EOI:
    if (require_image)
      ERREXIT(cinfo, JERR_NO_IMAGE);

    jpeg_abort((j_common_ptr) cinfo);
    retcode = JPEG_HEADER_TABLES_ONLY;
    break;
  case JPEG_SUSPENDED:

    break;
  }

  return retcode;
}

/* jpeg_consume_input. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00534ee2
GLOBAL(int)
jpeg_consume_input (j_decompress_ptr cinfo)
{
  int retcode = JPEG_SUSPENDED;

  switch (cinfo->global_state) {
  case DSTATE_START:

    (*cinfo->inputctl->reset_input_controller) (cinfo);

    (*cinfo->src->init_source) (cinfo);
    cinfo->global_state = DSTATE_INHEADER;

  case DSTATE_INHEADER:
    retcode = (*cinfo->inputctl->consume_input) (cinfo);
    if (retcode == JPEG_REACHED_SOS) {

      default_decompress_parms(cinfo);

      cinfo->global_state = DSTATE_READY;
    }
    break;
  case DSTATE_READY:

    retcode = JPEG_REACHED_SOS;
    break;
  case DSTATE_PRELOAD:
  case DSTATE_PRESCAN:
  case DSTATE_SCANNING:
  case DSTATE_RAW_OK:
  case DSTATE_BUFIMAGE:
  case DSTATE_BUFPOST:
  case DSTATE_STOPPING:
    retcode = (*cinfo->inputctl->consume_input) (cinfo);
    break;
  default:
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  }
  return retcode;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(boolean)
jpeg_input_complete (j_decompress_ptr cinfo)
{

  if (cinfo->global_state < DSTATE_START ||
      cinfo->global_state > DSTATE_STOPPING)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  return cinfo->inputctl->eoi_reached;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(boolean)
jpeg_has_multiple_scans (j_decompress_ptr cinfo)
{

  if (cinfo->global_state < DSTATE_READY ||
      cinfo->global_state > DSTATE_STOPPING)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  return cinfo->inputctl->has_multiple_scans;
}

/* jpeg_finish_decompress. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00534e3f
GLOBAL(boolean)
jpeg_finish_decompress (j_decompress_ptr cinfo)
{
  if ((cinfo->global_state == DSTATE_SCANNING ||
       cinfo->global_state == DSTATE_RAW_OK) && ! cinfo->buffered_image) {

    if (cinfo->output_scanline < cinfo->output_height)
      ERREXIT(cinfo, JERR_TOO_LITTLE_DATA);
    (*cinfo->master->finish_output_pass) (cinfo);
    cinfo->global_state = DSTATE_STOPPING;
  } else if (cinfo->global_state == DSTATE_BUFIMAGE) {

    cinfo->global_state = DSTATE_STOPPING;
  } else if (cinfo->global_state != DSTATE_STOPPING) {

    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  }

  while (! cinfo->inputctl->eoi_reached) {
    if ((*cinfo->inputctl->consume_input) (cinfo) == JPEG_SUSPENDED)
      return FALSE;
  }

  (*cinfo->src->term_source) (cinfo);

  jpeg_abort((j_common_ptr) cinfo);
  return TRUE;
}
