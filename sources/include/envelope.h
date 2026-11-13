/* envelope.h -- libvorbis 1.0rc3 lib/envelope.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_ENVELOPE_
#define _V_ENVELOPE_

#include "iir.h"
#include "smallft.h"

typedef struct {
  int ch;
  int winlength;
  int searchstep;
  float minenergy;

  IIR_state *iir;
  float    **filtered;

  int32_t storage;
  int32_t current;
  int32_t mark;
  int32_t prevmark;
  int32_t cursor;
} envelope_lookup;

#ifdef __cplusplus
extern "C" {
#endif

extern void _ve_envelope_init(envelope_lookup *e,vorbis_info *vi);
extern void _ve_envelope_clear(envelope_lookup *e);
extern int32_t _ve_envelope_search(vorbis_dsp_state *v);
extern void _ve_envelope_shift(envelope_lookup *e,int32_t shift);
extern int  _ve_envelope_mark(vorbis_dsp_state *v);

#ifdef __cplusplus
}
#endif

#endif
