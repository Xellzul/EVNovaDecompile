// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: libjpeg 6b's one line and compiler (zlib's compiler and line, plus -align 8). -align 8 puts output_gamma (double) at 0x38, sizeof(struct jpeg_decompress_struct) 0x1a0 as the exe checks (335's default -align 4 gives 0x19c); -enum min: its enums are one byte; build 335 has no -proc generic, its default is generic x86; Research/notes/jpeg.md
/* jdhuff.c -- IJG libjpeg 6b jdhuff.c, Copyright (C) 1991-1998, Thomas G. Lane; IJG licence in jpeglib.h; altered: comments, the #if branches this build does not take and the functions the exe does not link removed. Notes: Research/notes/jpeg.md */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdhuff.h"

typedef struct {
  int last_dc_val[MAX_COMPS_IN_SCAN];
} savable_state;

#define ASSIGN_STATE(dest,src)  ((dest) = (src))

typedef struct {
  struct jpeg_entropy_decoder pub;

  bitread_perm_state bitstate;
  savable_state saved;

  unsigned int restarts_to_go;

  d_derived_tbl * dc_derived_tbls[NUM_HUFF_TBLS];
  d_derived_tbl * ac_derived_tbls[NUM_HUFF_TBLS];

  d_derived_tbl * dc_cur_tbls[D_MAX_BLOCKS_IN_MCU];
  d_derived_tbl * ac_cur_tbls[D_MAX_BLOCKS_IN_MCU];

  boolean dc_needed[D_MAX_BLOCKS_IN_MCU];
  boolean ac_needed[D_MAX_BLOCKS_IN_MCU];
} huff_entropy_decoder;

typedef huff_entropy_decoder * huff_entropy_ptr;

/* start_pass_huff_decoder. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00566083
METHODDEF(void)
start_pass_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, blkn, dctbl, actbl;
  jpeg_component_info * compptr;

  if (cinfo->Ss != 0 || cinfo->Se != DCTSIZE2-1 ||
      cinfo->Ah != 0 || cinfo->Al != 0)
    WARNMS(cinfo, JWRN_NOT_SEQUENTIAL);

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;

    jpeg_make_d_derived_tbl(cinfo, TRUE, dctbl,
			    & entropy->dc_derived_tbls[dctbl]);
    jpeg_make_d_derived_tbl(cinfo, FALSE, actbl,
			    & entropy->ac_derived_tbls[actbl]);

    entropy->saved.last_dc_val[ci] = 0;
  }

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];

    entropy->dc_cur_tbls[blkn] = entropy->dc_derived_tbls[compptr->dc_tbl_no];
    entropy->ac_cur_tbls[blkn] = entropy->ac_derived_tbls[compptr->ac_tbl_no];

    if (compptr->component_needed) {
      entropy->dc_needed[blkn] = TRUE;

      entropy->ac_needed[blkn] = (compptr->DCT_scaled_size > 1);
    } else {
      entropy->dc_needed[blkn] = entropy->ac_needed[blkn] = FALSE;
    }
  }

  entropy->bitstate.bits_left = 0;
  entropy->bitstate.get_buffer = 0;
  entropy->pub.insufficient_data = FALSE;

  entropy->restarts_to_go = cinfo->restart_interval;
}

/* jpeg_make_d_derived_tbl. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00565e22
GLOBAL(void)
jpeg_make_d_derived_tbl (j_decompress_ptr cinfo, boolean isDC, int tblno,
			 d_derived_tbl ** pdtbl)
{
  JHUFF_TBL *htbl;
  d_derived_tbl *dtbl;
  int p, i, l, si, numsymbols;
  int lookbits, ctr;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  if (tblno < 0 || tblno >= NUM_HUFF_TBLS)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tblno);
  htbl =
    isDC ? cinfo->dc_huff_tbl_ptrs[tblno] : cinfo->ac_huff_tbl_ptrs[tblno];
  if (htbl == NULL)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tblno);

  if (*pdtbl == NULL)
    *pdtbl = (d_derived_tbl *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(d_derived_tbl));
  dtbl = *pdtbl;
  dtbl->pub = htbl;

  p = 0;
  for (l = 1; l <= 16; l++) {
    i = (int) htbl->bits[l];
    if (i < 0 || p + i > 256)
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    while (i--)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  numsymbols = p;

  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }

    if (((INT32) code) >= (((INT32) 1) << si))
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    code <<= 1;
    si++;
  }

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {

      dtbl->valoffset[l] = (INT32) p - (INT32) huffcode[p];
      p += htbl->bits[l];
      dtbl->maxcode[l] = huffcode[p-1];
    } else {
      dtbl->maxcode[l] = -1;
    }
  }
  dtbl->maxcode[17] = 0xFFFFFL;

  MEMZERO(dtbl->look_nbits, SIZEOF(dtbl->look_nbits));

  p = 0;
  for (l = 1; l <= HUFF_LOOKAHEAD; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {

      lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
      for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
	dtbl->look_nbits[lookbits] = l;
	dtbl->look_sym[lookbits] = htbl->huffval[p];
	lookbits++;
      }
    }
  }

  if (isDC) {
    for (i = 0; i < numsymbols; i++) {
      int sym = htbl->huffval[i];
      if (sym < 0 || sym > 15)
	ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    }
  }
}

#define MIN_GET_BITS  (BIT_BUF_SIZE-7)

/* jpeg_fill_bit_buffer. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00565cf5
GLOBAL(boolean)
jpeg_fill_bit_buffer (bitread_working_state * state,
		      register bit_buf_type get_buffer, register int bits_left,
		      int nbits)

{

  register const JOCTET * next_input_byte = state->next_input_byte;
  register size_t bytes_in_buffer = state->bytes_in_buffer;
  j_decompress_ptr cinfo = state->cinfo;

  if (cinfo->unread_marker == 0) {
    while (bits_left < MIN_GET_BITS) {
      register int c;

      if (bytes_in_buffer == 0) {
	if (! (*cinfo->src->fill_input_buffer) (cinfo))
	  return FALSE;
	next_input_byte = cinfo->src->next_input_byte;
	bytes_in_buffer = cinfo->src->bytes_in_buffer;
      }
      bytes_in_buffer--;
      c = GETJOCTET(*next_input_byte++);

      if (c == 0xFF) {

	do {
	  if (bytes_in_buffer == 0) {
	    if (! (*cinfo->src->fill_input_buffer) (cinfo))
	      return FALSE;
	    next_input_byte = cinfo->src->next_input_byte;
	    bytes_in_buffer = cinfo->src->bytes_in_buffer;
	  }
	  bytes_in_buffer--;
	  c = GETJOCTET(*next_input_byte++);
	} while (c == 0xFF);

	if (c == 0) {

	  c = 0xFF;
	} else {

	  cinfo->unread_marker = c;

	  goto no_more_bytes;
	}
      }

      get_buffer = (get_buffer << 8) | c;
      bits_left += 8;
    }
  } else {
  no_more_bytes:

    if (nbits > bits_left) {

      if (! cinfo->entropy->insufficient_data) {
	WARNMS(cinfo, JWRN_HIT_MARKER);
	cinfo->entropy->insufficient_data = TRUE;
      }

      get_buffer <<= MIN_GET_BITS - bits_left;
      bits_left = MIN_GET_BITS;
    }
  }

  state->next_input_byte = next_input_byte;
  state->bytes_in_buffer = bytes_in_buffer;
  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  return TRUE;
}

/* jpeg_huff_decode. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00565c28
GLOBAL(int)
jpeg_huff_decode (bitread_working_state * state,
		  register bit_buf_type get_buffer, register int bits_left,
		  d_derived_tbl * htbl, int min_bits)
{
  register int l = min_bits;
  register INT32 code;

  CHECK_BIT_BUFFER(*state, l, return -1);
  code = GET_BITS(l);

  while (code > htbl->maxcode[l]) {
    code <<= 1;
    CHECK_BIT_BUFFER(*state, 1, return -1);
    code |= GET_BITS(1);
    l++;
  }

  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  if (l > 16) {
    WARNMS(state->cinfo, JWRN_HUFF_BAD_CODE);
    return 0;
  }

  return htbl->pub->huffval[ (int) (code + htbl->valoffset[l]) ];
}

#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] =
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

LOCAL(boolean)
process_restart (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci;

  cinfo->marker->discarded_bytes += entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    return FALSE;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;

  entropy->restarts_to_go = cinfo->restart_interval;

  if (cinfo->unread_marker == 0)
    entropy->pub.insufficient_data = FALSE;

  return TRUE;
}

/* decode_mcu. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0056572d
METHODDEF(boolean)
decode_mcu (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int blkn;
  BITREAD_STATE_VARS;
  savable_state state;

  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  if (! entropy->pub.insufficient_data) {

    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(state, entropy->saved);

    for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      JBLOCKROW block = MCU_data[blkn];
      d_derived_tbl * dctbl = entropy->dc_cur_tbls[blkn];
      d_derived_tbl * actbl = entropy->ac_cur_tbls[blkn];
      register int s, k, r;

      HUFF_DECODE(s, br_state, dctbl, return FALSE, label1);
      if (s) {
	CHECK_BIT_BUFFER(br_state, s, return FALSE);
	r = GET_BITS(s);
	s = HUFF_EXTEND(r, s);
      }

      if (entropy->dc_needed[blkn]) {

	int ci = cinfo->MCU_membership[blkn];
	s += state.last_dc_val[ci];
	state.last_dc_val[ci] = s;

	(*block)[0] = (JCOEF) s;
      }

      if (entropy->ac_needed[blkn]) {

	for (k = 1; k < DCTSIZE2; k++) {
	  HUFF_DECODE(s, br_state, actbl, return FALSE, label2);

	  r = s >> 4;
	  s &= 15;

	  if (s) {
	    k += r;
	    CHECK_BIT_BUFFER(br_state, s, return FALSE);
	    r = GET_BITS(s);
	    s = HUFF_EXTEND(r, s);

	    (*block)[jpeg_natural_order[k]] = (JCOEF) s;
	  } else {
	    if (r != 15)
	      break;
	    k += 15;
	  }
	}

      } else {

	for (k = 1; k < DCTSIZE2; k++) {
	  HUFF_DECODE(s, br_state, actbl, return FALSE, label3);

	  r = s >> 4;
	  s &= 15;

	  if (s) {
	    k += r;
	    CHECK_BIT_BUFFER(br_state, s, return FALSE);
	    DROP_BITS(s);
	  } else {
	    if (r != 15)
	      break;
	    k += 15;
	  }
	}

      }
    }

    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(entropy->saved, state);
  }

  entropy->restarts_to_go--;

  return TRUE;
}

/* jinit_huff_decoder. STATE: genuine libjpeg 6b; MATCH, byte-identical.
   OPEN: none. */
// FUNCTION: EVNOVA 0x005656e9
GLOBAL(void)
jinit_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy;
  int i;

  entropy = (huff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(huff_entropy_decoder));
  cinfo->entropy = (struct jpeg_entropy_decoder *) entropy;
  entropy->pub.start_pass = start_pass_huff_decoder;
  entropy->pub.decode_mcu = decode_mcu;

  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->dc_derived_tbls[i] = entropy->ac_derived_tbls[i] = NULL;
  }
}
