/* psy.h -- libvorbis 1.0rc3 lib/psy.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_PSY_H_
#define _V_PSY_H_
#include "smallft.h"

#include "backends.h"

#ifndef EHMER_MAX
#define EHMER_MAX 56
#endif

#define MAX_BARK 27
#define P_BANDS 17
#define P_LEVELS 11

typedef struct vp_couple{
  int limit;

  int outofphase_redundant_flip_p;
  float outofphase_requant_limit;

  float amppost_point;

} vp_couple;

typedef struct vp_couple_pass{
  float granulem;
  float igranulem;

  vp_couple couple_pass[8];
} vp_couple_pass;

typedef struct vp_attenblock{
  float block[P_BANDS][P_LEVELS];
} vp_attenblock;

#define NOISE_COMPAND_LEVELS 40
typedef struct vorbis_info_psy{
  float  ath[27];

  float  ath_adjatt;
  float  ath_maxatt;

  float tone_masteratt;
  float tone_guard;
  float tone_abs_limit;
  vp_attenblock toneatt;

  int peakattp;
  int curvelimitp;
  vp_attenblock peakatt;

  int noisemaskp;
  float noisemaxsupp;
  float noisewindowlo;
  float noisewindowhi;
  int   noisewindowlomin;
  int   noisewindowhimin;
  int   noisewindowfixed;
  float noiseoff[P_BANDS];
  float noisecompand[NOISE_COMPAND_LEVELS];

  float max_curve_dB;

  vp_couple_pass couple_pass[8];

} vorbis_info_psy;

typedef struct{
  int       eighth_octave_lines;

  float     preecho_thresh[4];
  float     postecho_thresh[4];
  float     preecho_minenergy;

  float     ampmax_att_per_sec;

  int       delaycache;
} vorbis_info_psy_global;

typedef struct {
  float   ampmax;
  int     channels;

  vorbis_info_psy_global *gi;
} vorbis_look_psy_global;

typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  float ***tonecurves;
  float *noisethresh;
  float *noiseoffset;

  float *ath;
  long  *octave;
  long  *bark;

  long  firstoc;
  long  shiftoc;
  int   eighth_octave_lines;
  int   total_octave_lines;
  long  rate;
} vorbis_look_psy;

#ifdef __cplusplus
extern "C" {
#endif

extern void   _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
			   vorbis_info_psy_global *gi,int n,long rate);
extern void   _vp_psy_clear(vorbis_look_psy *p);
extern void  *_vi_psy_dup(void *source);

extern void   _vi_psy_free(vorbis_info_psy *i);
extern vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i);

extern void _vp_remove_floor(vorbis_look_psy *p,
			     float *mdct,
			     float *codedflr,
			     float *residue);

extern void   _vp_compute_mask(vorbis_look_psy *p,
			       float *fft,
			       float *mdct,
			       float *mask,
			       float global_specmax,
			       float local_specmax,
			       float bitrate_noise_offset);

extern void _vp_quantize_couple(vorbis_look_psy *p,
			 vorbis_info_mapping0 *vi,
			 float **pcm,
			 float **sofar,
			 float **quantized,
			 int   *nonzero,
			 int   passno);

extern float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd);

#ifdef __cplusplus
}
#endif

#endif
