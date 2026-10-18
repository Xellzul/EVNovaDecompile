/* misc.h -- libvorbis 1.0rc3 lib/misc.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments and the DEBUG_LEAKS branch (not built) removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_
#include "codec.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *_vorbis_block_alloc(vorbis_block *vb,long bytes);
extern void _vorbis_block_ripcord(vorbis_block *vb);
extern void _analysis_output(char *base,int i,float *v,int n,int bark,int dB);

#ifdef __cplusplus
}
#endif

#endif
