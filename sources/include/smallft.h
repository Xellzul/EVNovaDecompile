/* smallft.h -- libvorbis 1.0rc3 lib/smallft.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_SMFT_H_
#define _V_SMFT_H_

#include "codec.h"

typedef struct {
  int n;
  float *trigcache;
  int *splitcache;
} drft_lookup;

#ifdef __cplusplus
extern "C" {
#endif

extern void drft_forward(drft_lookup *l,float *data);
extern void drft_backward(drft_lookup *l,float *data);
extern void drft_init(drft_lookup *l,int n);
extern void drft_clear(drft_lookup *l);

#ifdef __cplusplus
}
#endif

#endif
