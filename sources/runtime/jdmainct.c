// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jdmainct.c -- IJG libjpeg 6b jdmainct.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

typedef struct {
  struct jpeg_d_main_controller pub;

  JSAMPARRAY buffer[MAX_COMPONENTS];

  boolean buffer_full;
  JDIMENSION rowgroup_ctr;

  JSAMPIMAGE xbuffer[2];

  int whichptr;
  int context_state;
  JDIMENSION rowgroups_avail;
  JDIMENSION iMCU_row_ctr;
} my_main_controller;

typedef my_main_controller * my_main_ptr;

#define CTX_PREPARE_FOR_IMCU	0
#define CTX_PROCESS_IMCU	1
#define CTX_POSTPONED_ROW	2

METHODDEF(void) process_data_simple_main
	JPP((j_decompress_ptr cinfo, JSAMPARRAY output_buf,
	     JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail));
METHODDEF(void) process_data_context_main
	JPP((j_decompress_ptr cinfo, JSAMPARRAY output_buf,
	     JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail));
METHODDEF(void) process_data_crank_post
	JPP((j_decompress_ptr cinfo, JSAMPARRAY output_buf,
	     JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail));

LOCAL(void)
alloc_funny_pointers (j_decompress_ptr cinfo)

{
  my_main_ptr main = (my_main_ptr) cinfo->main;
  int ci, rgroup;
  int M = cinfo->min_DCT_scaled_size;
  jpeg_component_info *compptr;
  JSAMPARRAY xbuf;

  main->xbuffer[0] = (JSAMPIMAGE)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				cinfo->num_components * 2 * SIZEOF(JSAMPARRAY));
  main->xbuffer[1] = main->xbuffer[0] + cinfo->num_components;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->DCT_scaled_size) /
      cinfo->min_DCT_scaled_size;

    xbuf = (JSAMPARRAY)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  2 * (rgroup * (M + 4)) * SIZEOF(JSAMPROW));
    xbuf += rgroup;
    main->xbuffer[0][ci] = xbuf;
    xbuf += rgroup * (M + 4);
    main->xbuffer[1][ci] = xbuf;
  }
}

/* make_funny_pointers. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00567450
LOCAL(void)
make_funny_pointers (j_decompress_ptr cinfo)

{
  my_main_ptr main = (my_main_ptr) cinfo->main;
  int ci, i, rgroup;
  int M = cinfo->min_DCT_scaled_size;
  jpeg_component_info *compptr;
  JSAMPARRAY buf, xbuf0, xbuf1;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->DCT_scaled_size) /
      cinfo->min_DCT_scaled_size;
    xbuf0 = main->xbuffer[0][ci];
    xbuf1 = main->xbuffer[1][ci];

    buf = main->buffer[ci];
    for (i = 0; i < rgroup * (M + 2); i++) {
      xbuf0[i] = xbuf1[i] = buf[i];
    }

    for (i = 0; i < rgroup * 2; i++) {
      xbuf1[rgroup*(M-2) + i] = buf[rgroup*M + i];
      xbuf1[rgroup*M + i] = buf[rgroup*(M-2) + i];
    }

    for (i = 0; i < rgroup; i++) {
      xbuf0[i - rgroup] = xbuf0[0];
    }
  }
}

/* set_wraparound_pointers. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0056737d
LOCAL(void)
set_wraparound_pointers (j_decompress_ptr cinfo)

{
  my_main_ptr main = (my_main_ptr) cinfo->main;
  int ci, i, rgroup;
  int M = cinfo->min_DCT_scaled_size;
  jpeg_component_info *compptr;
  JSAMPARRAY xbuf0, xbuf1;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->DCT_scaled_size) /
      cinfo->min_DCT_scaled_size;
    xbuf0 = main->xbuffer[0][ci];
    xbuf1 = main->xbuffer[1][ci];
    for (i = 0; i < rgroup; i++) {
      xbuf0[i - rgroup] = xbuf0[rgroup*(M+1) + i];
      xbuf1[i - rgroup] = xbuf1[rgroup*(M+1) + i];
      xbuf0[rgroup*(M+2) + i] = xbuf0[i];
      xbuf1[rgroup*(M+2) + i] = xbuf1[i];
    }
  }
}

/* set_bottom_pointers. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005672d6
LOCAL(void)
set_bottom_pointers (j_decompress_ptr cinfo)

{
  my_main_ptr main = (my_main_ptr) cinfo->main;
  int ci, i, rgroup, iMCUheight, rows_left;
  jpeg_component_info *compptr;
  JSAMPARRAY xbuf;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {

    iMCUheight = compptr->v_samp_factor * compptr->DCT_scaled_size;
    rgroup = iMCUheight / cinfo->min_DCT_scaled_size;

    rows_left = (int) (compptr->downsampled_height % (JDIMENSION) iMCUheight);
    if (rows_left == 0) rows_left = iMCUheight;

    if (ci == 0) {
      main->rowgroups_avail = (JDIMENSION) ((rows_left-1) / rgroup + 1);
    }

    xbuf = main->xbuffer[main->whichptr][ci];
    for (i = 0; i < rgroup * 2; i++) {
      xbuf[rows_left + i] = xbuf[rows_left-1];
    }
  }
}

/* start_pass_main. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00567263
METHODDEF(void)
start_pass_main (j_decompress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_main_ptr main = (my_main_ptr) cinfo->main;

  switch (pass_mode) {
  case JBUF_PASS_THRU:
    if (cinfo->upsample->need_context_rows) {
      main->pub.process_data = process_data_context_main;
      make_funny_pointers(cinfo);
      main->whichptr = 0;
      main->context_state = CTX_PREPARE_FOR_IMCU;
      main->iMCU_row_ctr = 0;
    } else {

      main->pub.process_data = process_data_simple_main;
    }
    main->buffer_full = FALSE;
    main->rowgroup_ctr = 0;
    break;
  case JBUF_CRANK_DEST:

    main->pub.process_data = process_data_crank_post;
    break;
  default:
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    break;
  }
}

/* process_data_simple_main. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005671fd
METHODDEF(void)
process_data_simple_main (j_decompress_ptr cinfo,
			  JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			  JDIMENSION out_rows_avail)
{
  my_main_ptr main = (my_main_ptr) cinfo->main;
  JDIMENSION rowgroups_avail;

  if (! main->buffer_full) {
    if (! (*cinfo->coef->decompress_data) (cinfo, main->buffer))
      return;
    main->buffer_full = TRUE;
  }

  rowgroups_avail = (JDIMENSION) cinfo->min_DCT_scaled_size;

  (*cinfo->post->post_process_data) (cinfo, main->buffer,
				     &main->rowgroup_ctr, rowgroups_avail,
				     output_buf, out_row_ctr, out_rows_avail);

  if (main->rowgroup_ctr >= rowgroups_avail) {
    main->buffer_full = FALSE;
    main->rowgroup_ctr = 0;
  }
}

/* process_data_context_main. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005670dc
METHODDEF(void)
process_data_context_main (j_decompress_ptr cinfo,
			   JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			   JDIMENSION out_rows_avail)
{
  my_main_ptr main = (my_main_ptr) cinfo->main;

  if (! main->buffer_full) {
    if (! (*cinfo->coef->decompress_data) (cinfo,
					   main->xbuffer[main->whichptr]))
      return;
    main->buffer_full = TRUE;
    main->iMCU_row_ctr++;
  }

  switch (main->context_state) {
  case CTX_POSTPONED_ROW:

    (*cinfo->post->post_process_data) (cinfo, main->xbuffer[main->whichptr],
			&main->rowgroup_ctr, main->rowgroups_avail,
			output_buf, out_row_ctr, out_rows_avail);
    if (main->rowgroup_ctr < main->rowgroups_avail)
      return;
    main->context_state = CTX_PREPARE_FOR_IMCU;
    if (*out_row_ctr >= out_rows_avail)
      return;

  case CTX_PREPARE_FOR_IMCU:

    main->rowgroup_ctr = 0;
    main->rowgroups_avail = (JDIMENSION) (cinfo->min_DCT_scaled_size - 1);

    if (main->iMCU_row_ctr == cinfo->total_iMCU_rows)
      set_bottom_pointers(cinfo);
    main->context_state = CTX_PROCESS_IMCU;

  case CTX_PROCESS_IMCU:

    (*cinfo->post->post_process_data) (cinfo, main->xbuffer[main->whichptr],
			&main->rowgroup_ctr, main->rowgroups_avail,
			output_buf, out_row_ctr, out_rows_avail);
    if (main->rowgroup_ctr < main->rowgroups_avail)
      return;

    if (main->iMCU_row_ctr == 1)
      set_wraparound_pointers(cinfo);

    main->whichptr ^= 1;
    main->buffer_full = FALSE;

    main->rowgroup_ctr = (JDIMENSION) (cinfo->min_DCT_scaled_size + 1);
    main->rowgroups_avail = (JDIMENSION) (cinfo->min_DCT_scaled_size + 2);
    main->context_state = CTX_POSTPONED_ROW;
  }
}

/* process_data_crank_post. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005670b6
METHODDEF(void)
process_data_crank_post (j_decompress_ptr cinfo,
			 JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			 JDIMENSION out_rows_avail)
{
  (*cinfo->post->post_process_data) (cinfo, (JSAMPIMAGE) NULL,
				     (JDIMENSION *) NULL, (JDIMENSION) 0,
				     output_buf, out_row_ctr, out_rows_avail);
}

/* jinit_d_main_controller. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00566f3b
GLOBAL(void)
jinit_d_main_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
  my_main_ptr main;
  int ci, rgroup, ngroups;
  jpeg_component_info *compptr;

  main = (my_main_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_main_controller));
  cinfo->main = (struct jpeg_d_main_controller *) main;
  main->pub.start_pass = start_pass_main;

  if (need_full_buffer)
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);

  if (cinfo->upsample->need_context_rows) {
    if (cinfo->min_DCT_scaled_size < 2)
      ERREXIT(cinfo, JERR_NOTIMPL);
    alloc_funny_pointers(cinfo);
    ngroups = cinfo->min_DCT_scaled_size + 2;
  } else {
    ngroups = cinfo->min_DCT_scaled_size;
  }

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    rgroup = (compptr->v_samp_factor * compptr->DCT_scaled_size) /
      cinfo->min_DCT_scaled_size;
    main->buffer[ci] = (*cinfo->mem->alloc_sarray)
			((j_common_ptr) cinfo, JPOOL_IMAGE,
			 compptr->width_in_blocks * compptr->DCT_scaled_size,
			 (JDIMENSION) (rgroup * ngroups));
  }
}
