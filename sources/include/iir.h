/* iir.h -- libvorbis 1.0rc3 lib/iir.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy: IIR_init/clear/filter/filter_Band/reset renamed IIRStateInit/Destroy/Filter/FilterBand/Reset (the archive members' symbols); altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_IIR_H_
#define _V_IIR_H_

typedef struct {
  int stages;
  float *coeff_A;
  float *coeff_B;
  float *z_A;
  int ring;
  float gain;
} IIR_state;

#ifdef __cplusplus
extern "C" {
#endif

extern void IIRStateInit(IIR_state *s,int stages,float gain, float *A, float *B);
extern void IIRStateDestroy(IIR_state *s);
extern float IIRStateFilter(IIR_state *s,float in);
extern float IIRStateFilterBand(IIR_state *s,float in);
extern void IIRStateReset(IIR_state *s);

#ifdef __cplusplus
}
#endif

#endif
