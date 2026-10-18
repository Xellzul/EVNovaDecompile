/* mdct.h -- libvorbis 1.0rc3 lib/mdct.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments and the MDCT_INTEGERIZED branch (not built: the exe's mdct is x87) removed. Notes: Research/notes/burgervorbis.md */
#ifndef _OGG_mdct_H_
#define _OGG_mdct_H_

#include "codec.h"

#define DATA_TYPE float
#define REG_TYPE  float
#define cPI3_8 .38268343236508977175F
#define cPI2_8 .70710678118654752441F
#define cPI1_8 .92387953251128675613F

#define FLOAT_CONV(x) (x)
#define MULT_NORM(x) (x)
#define HALVE(x) ((x)*.5f)

typedef struct {
  int n;
  int log2n;

  DATA_TYPE *trig;
  int       *bitrev;

  DATA_TYPE scale;
} mdct_lookup;

#ifdef __cplusplus
extern "C" {
#endif

extern void mdct_init(mdct_lookup *lookup,int n);
extern void mdct_clear(mdct_lookup *l);
extern void mdct_forward(mdct_lookup *init, DATA_TYPE *in, DATA_TYPE *out);
extern void mdct_backward(mdct_lookup *init, DATA_TYPE *in, DATA_TYPE *out);

#ifdef __cplusplus
}
#endif

#endif
