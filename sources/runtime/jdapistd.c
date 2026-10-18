// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jdapistd.c -- IJG libjpeg 6b jdapistd.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments and the #if branches this build does not take removed (the functions the exe does not link stay, unmarked). Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

LOCAL(boolean) output_pass_setup JPP((j_decompress_ptr cinfo));

/* jpeg_start_decompress. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536ce9
GLOBAL(boolean)
jpeg_start_decompress (j_decompress_ptr cinfo)
{
  if (cinfo->global_state == DSTATE_READY) {

    jinit_master_decompress(cinfo);
    if (cinfo->buffered_image) {

      cinfo->global_state = DSTATE_BUFIMAGE;
      return TRUE;
    }
    cinfo->global_state = DSTATE_PRELOAD;
  }
  if (cinfo->global_state == DSTATE_PRELOAD) {

    if (cinfo->inputctl->has_multiple_scans) {
      for (;;) {
	int retcode;

	if (cinfo->progress != NULL)
	  (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);

	retcode = (*cinfo->inputctl->consume_input) (cinfo);
	if (retcode == JPEG_SUSPENDED)
	  return FALSE;
	if (retcode == JPEG_REACHED_EOI)
	  break;

	if (cinfo->progress != NULL &&
	    (retcode == JPEG_ROW_COMPLETED || retcode == JPEG_REACHED_SOS)) {
	  if (++cinfo->progress->pass_counter >= cinfo->progress->pass_limit) {

	    cinfo->progress->pass_limit += (long) cinfo->total_iMCU_rows;
	  }
	}
      }
    }
    cinfo->output_scan_number = cinfo->input_scan_number;
  } else if (cinfo->global_state != DSTATE_PRESCAN)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  return output_pass_setup(cinfo);
}

/* output_pass_setup. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536c3d
LOCAL(boolean)
output_pass_setup (j_decompress_ptr cinfo)
{
  if (cinfo->global_state != DSTATE_PRESCAN) {

    (*cinfo->master->prepare_for_output_pass) (cinfo);
    cinfo->output_scanline = 0;
    cinfo->global_state = DSTATE_PRESCAN;
  }

  while (cinfo->master->is_dummy_pass) {

    while (cinfo->output_scanline < cinfo->output_height) {
      JDIMENSION last_scanline;

      if (cinfo->progress != NULL) {
	cinfo->progress->pass_counter = (long) cinfo->output_scanline;
	cinfo->progress->pass_limit = (long) cinfo->output_height;
	(*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
      }

      last_scanline = cinfo->output_scanline;
      (*cinfo->main->process_data) (cinfo, (JSAMPARRAY) NULL,
				    &cinfo->output_scanline, (JDIMENSION) 0);
      if (cinfo->output_scanline == last_scanline)
	return FALSE;
    }

    (*cinfo->master->finish_output_pass) (cinfo);
    (*cinfo->master->prepare_for_output_pass) (cinfo);
    cinfo->output_scanline = 0;
  }

  cinfo->global_state = cinfo->raw_data_out ? DSTATE_RAW_OK : DSTATE_SCANNING;
  return TRUE;
}

/* jpeg_read_scanlines. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00536ba8
GLOBAL(JDIMENSION)
jpeg_read_scanlines (j_decompress_ptr cinfo, JSAMPARRAY scanlines,
		     JDIMENSION max_lines)
{
  JDIMENSION row_ctr;

  if (cinfo->global_state != DSTATE_SCANNING)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  if (cinfo->output_scanline >= cinfo->output_height) {
    WARNMS(cinfo, JWRN_TOO_MUCH_DATA);
    return 0;
  }

  if (cinfo->progress != NULL) {
    cinfo->progress->pass_counter = (long) cinfo->output_scanline;
    cinfo->progress->pass_limit = (long) cinfo->output_height;
    (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
  }

  row_ctr = 0;
  (*cinfo->main->process_data) (cinfo, scanlines, &row_ctr, max_lines);
  cinfo->output_scanline += row_ctr;
  return row_ctr;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(JDIMENSION)
jpeg_read_raw_data (j_decompress_ptr cinfo, JSAMPIMAGE data,
		    JDIMENSION max_lines)
{
  JDIMENSION lines_per_iMCU_row;

  if (cinfo->global_state != DSTATE_RAW_OK)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  if (cinfo->output_scanline >= cinfo->output_height) {
    WARNMS(cinfo, JWRN_TOO_MUCH_DATA);
    return 0;
  }

  if (cinfo->progress != NULL) {
    cinfo->progress->pass_counter = (long) cinfo->output_scanline;
    cinfo->progress->pass_limit = (long) cinfo->output_height;
    (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
  }

  lines_per_iMCU_row = cinfo->max_v_samp_factor * cinfo->min_DCT_scaled_size;
  if (max_lines < lines_per_iMCU_row)
    ERREXIT(cinfo, JERR_BUFFER_SIZE);

  if (! (*cinfo->coef->decompress_data) (cinfo, data))
    return 0;

  cinfo->output_scanline += lines_per_iMCU_row;
  return lines_per_iMCU_row;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(boolean)
jpeg_start_output (j_decompress_ptr cinfo, int scan_number)
{
  if (cinfo->global_state != DSTATE_BUFIMAGE &&
      cinfo->global_state != DSTATE_PRESCAN)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  if (scan_number <= 0)
    scan_number = 1;
  if (cinfo->inputctl->eoi_reached &&
      scan_number > cinfo->input_scan_number)
    scan_number = cinfo->input_scan_number;
  cinfo->output_scan_number = scan_number;

  return output_pass_setup(cinfo);
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
GLOBAL(boolean)
jpeg_finish_output (j_decompress_ptr cinfo)
{
  if ((cinfo->global_state == DSTATE_SCANNING ||
       cinfo->global_state == DSTATE_RAW_OK) && cinfo->buffered_image) {

    (*cinfo->master->finish_output_pass) (cinfo);
    cinfo->global_state = DSTATE_BUFPOST;
  } else if (cinfo->global_state != DSTATE_BUFPOST) {

    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  }

  while (cinfo->input_scan_number <= cinfo->output_scan_number &&
	 ! cinfo->inputctl->eoi_reached) {
    if ((*cinfo->inputctl->consume_input) (cinfo) == JPEG_SUSPENDED)
      return FALSE;
  }
  cinfo->global_state = DSTATE_BUFIMAGE;
  return TRUE;
}
