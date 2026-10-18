/* registry.h -- libvorbis 1.0rc3 lib/registry.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments and the VORBISDLL_IMPORT branch (not built) removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_REG_H_
#define _V_REG_H_

#define VI_TRANSFORMB 1
#define VI_WINDOWB 1
#define VI_TIMEB 1
#define VI_FLOORB 2
#define VI_RESB 3
#define VI_MAPB 1

# define EXTERN extern

#ifdef __cplusplus
extern "C" {
#endif

EXTERN vorbis_func_time      *_time_P[];
EXTERN vorbis_func_floor     *_floor_P[];
EXTERN vorbis_func_residue   *_residue_P[];
EXTERN vorbis_func_mapping   *_mapping_P[];

#ifdef __cplusplus
}
#endif

#endif
