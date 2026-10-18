/* codec_internal.h -- libvorbis 1.0rc3 lib/codec_internal.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (the void look and info placeholders are struct types, as its C++ mangled names show); altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_CODECI_H_
#define _V_CODECI_H_

#include "envelope.h"
#include "codebook.h"

#define BLOCKTYPE_IMPULSE    0
#define BLOCKTYPE_PADDING    1
#define BLOCKTYPE_TRANSITION 0
#define BLOCKTYPE_LONG       1

typedef struct vorbis_block_internal{
  float  **pcmdelay;
  float  ampmax;
  int    blocktype;

  ogg_uint32_t *packet_markers;
} vorbis_block_internal;

typedef struct vorbis_look_time vorbis_look_time;
typedef struct vorbis_look_mapping vorbis_look_mapping;
typedef struct vorbis_look_floor vorbis_look_floor;
typedef struct vorbis_look_residue vorbis_look_residue;
typedef void vorbis_look_transform;

typedef struct {
  int blockflag;
  int windowtype;
  int transformtype;
  int mapping;
} vorbis_info_mode;

typedef struct vorbis_info_time vorbis_info_time;
typedef struct vorbis_info_floor vorbis_info_floor;
typedef struct vorbis_info_residue vorbis_info_residue;
typedef struct vorbis_info_mapping vorbis_info_mapping;

#include "psy.h"
#include "bitrate.h"

typedef struct backend_lookup_state {

  envelope_lookup        *ve;
  float                 **window[2][2][2];
  vorbis_look_transform **transform[2];
  codebook               *fullbooks;
  vorbis_look_psy_global *psy_g_look;

  int                     modebits;
  vorbis_look_mapping   **mode;

  unsigned char *header;
  unsigned char *header1;
  unsigned char *header2;

  bitrate_manager_state bms;

} backend_lookup_state;

typedef struct highlevel_block {
  double tone_mask_quality;
  double tone_peaklimit_quality;

  double noise_bias_quality;
  double noise_compand_quality;

  double ath_quality;

} highlevel_block;

typedef struct highlevel_encode_setup {
  double base_quality;
  double base_quality_short;
  double base_quality_long;

  int short_block_p;
  int long_block_p;
  int impulse_block_p;

  int stereo_couple_p;
  int stereo_backfill_p;
  int residue_backfill_p;

  int    stereo_point_dB;
  double stereo_point_kHz[2];
  double lowpass_kHz[2];

  double ath_floating_dB;
  double ath_absolute_dB;

  double amplitude_track_dBpersec;
  double trigger_quality;

  highlevel_block blocktype[4];

} highlevel_encode_setup;

typedef struct codec_setup_info {

  long blocksizes[2];

  int        modes;
  int        maps;
  int        times;
  int        floors;
  int        residues;
  int        books;
  int        psys;

  vorbis_info_mode       *mode_param[64];
  int                     map_type[64];
  vorbis_info_mapping    *map_param[64];
  int                     time_type[64];
  vorbis_info_time       *time_param[64];
  int                     floor_type[64];
  vorbis_info_floor      *floor_param[64];
  int                     residue_type[64];
  vorbis_info_residue    *residue_param[64];
  static_codebook        *book_param[256];

  vorbis_info_psy        *psy_param[64];
  vorbis_info_psy_global psy_g_param;

  bitrate_manager_info   bi;
  highlevel_encode_setup hi;

  int    passlimit[32];
  int    coupling_passes;
} codec_setup_info;

#ifdef __cplusplus
extern "C" {
#endif

extern vorbis_look_psy_global *_vp_global_look(vorbis_info *vi);
extern void _vp_global_free(vorbis_look_psy_global *look);

#ifdef __cplusplus
}
#endif

#endif
