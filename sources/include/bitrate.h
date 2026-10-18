/* bitrate.h -- libvorbis 1.0rc3 lib/bitrate.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_BITRATE_H_
#define _V_BITRATE_H_

#include "codec.h"
#include "codec_internal.h"
#include "os.h"

#define BITTRACK_DIVISOR 16
#define BITTRACK_BPT     6
typedef struct bitrate_manager_state {
  ogg_uint32_t  *queue_binned;
  ogg_uint32_t  *queue_actual;
  int            queue_size;

  int            queue_head;
  int            queue_bins;

  long          *avg_binacc;
  int            avg_center;
  int            avg_tail;
  ogg_uint32_t   avg_centeracc;
  ogg_uint32_t   avg_sampleacc;
  ogg_uint32_t   avg_sampledesired;
  ogg_uint32_t   avg_centerdesired;

  long          *minmax_binstack;
  long          *minmax_posstack;
  long          *minmax_limitstack;
  long           minmax_stackptr;

  long           minmax_acctotal;
  int            minmax_tail;
  ogg_uint32_t   minmax_sampleacc;
  ogg_uint32_t   minmax_sampledesired;

  int            next_to_flush;
  int            last_to_flush;

  double         avgfloat;
  double         avgnoise;
  double         noisetrigger_request;
  long           noisetrigger_postpone;

  oggpack_buffer *queue_packet_buffers;
  ogg_packet     *queue_packets;

} bitrate_manager_state;

typedef struct bitrate_manager_info{

  double queue_avg_time;
  double queue_avg_center;
  double queue_minmax_time;
  double queue_hardmin;
  double queue_hardmax;
  double queue_avgmin;
  double queue_avgmax;

  double avgfloat_initial;
  double avgfloat_minimum;
  double avgfloat_downslew_max;
  double avgfloat_upslew_max;
  double avgfloat_noise_lowtrigger;
  double avgfloat_noise_hightrigger;
  double avgfloat_noise_minval;
  double avgfloat_noise_maxval;
} bitrate_manager_info;

#ifdef __cplusplus
extern "C" {
#endif

extern void vorbis_bitrate_init(vorbis_info *vi,bitrate_manager_state *bs);
extern void vorbis_bitrate_clear(bitrate_manager_state *bs);
extern int vorbis_bitrate_managed(vorbis_block *vb);
extern int vorbis_bitrate_maxmarkers(void);
extern int vorbis_bitrate_addblock(vorbis_block *vb);
extern int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op);

#ifdef __cplusplus
}
#endif

#endif
