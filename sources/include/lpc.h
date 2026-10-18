/* lpc.h -- libvorbis 1.0rc3 lib/lpc.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_LPC_H_
#define _V_LPC_H_

#include "codec.h"
#include "smallft.h"

typedef struct lpclook{

  drft_lookup fft;

  int ln;
  int m;

} lpc_lookup;

#ifdef __cplusplus
extern "C" {
#endif

extern void lpc_init(lpc_lookup *l,long mapped, int m);
extern void lpc_clear(lpc_lookup *l);

extern float vorbis_lpc_from_data(float *data,float *lpc,int n,int m);
extern float vorbis_lpc_from_curve(float *curve,float *lpc,lpc_lookup *l);

extern void vorbis_lpc_predict(float *coeff,float *prime,int m,
			       float *data,long n);

#ifdef __cplusplus
}
#endif

#endif
