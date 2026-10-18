// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; no veto here, so the library's one line); Research/notes/burgervorbis.md
/* lpc.c -- libvorbis 1.0rc3 lib/lpc.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (FastMemSet); altered: comments and the function the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "os.h"
#include "smallft.h"
#include "lpc.h"
#include "scales.h"
#include "misc.h"

/* vorbis_lpc_from_data. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054c530
float vorbis_lpc_from_data(float *data,float *lpc,int n,int m){
  float *aut=(float *)alloca(sizeof(*aut)*(m+1));
  float error;
  int i,j;

  j=m+1;
  while(j--){
    double d=0;
    for(i=j;i<n;i++)d+=data[i]*data[i-j];
    aut[j]=d;
  }

  error=aut[0];

  for(i=0;i<m;i++){
    float r= -aut[i+1];

    if(error==0){
      FastMemSet(lpc,0,m*sizeof(*lpc));
      return 0;
    }

    for(j=0;j<i;j++)r-=lpc[j]*aut[i-j];
    r/=error;

    lpc[i]=r;
    for(j=0;j<i/2;j++){
      float tmp=lpc[j];
      lpc[j]+=r*lpc[i-1-j];
      lpc[i-1-j]+=r*tmp;
    }
    if(i%2)lpc[j]+=lpc[j]*r;

    error*=1.f-r*r;
  }

  return error;
}

/* vorbis_lpc_from_curve. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054c9e0
float vorbis_lpc_from_curve(float *curve,float *lpc,lpc_lookup *l){
  int n=l->ln;
  int m=l->m;
  float *work=(float *)alloca(sizeof(*work)*(n+n));
  float fscale=.5f/n;
  int i,j;

  for(i=0;i<n;i++){
    work[i*2]=curve[i]*fscale;
    work[i*2+1]=0;
  }
  work[n*2-1]=curve[n-1]*fscale;

  n*=2;
  drft_backward(&l->fft,work);

  for(i=0,j=n/2;i<n/2;){
    float temp=work[i];
    work[i++]=work[j];
    work[j++]=temp;
  }

  return(vorbis_lpc_from_data(work,lpc,n,m));
}

/* lpc_init. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054cc10
void lpc_init(lpc_lookup *l,long mapped, int m){
  FastMemSet(l,0,sizeof(*l));

  l->ln=mapped;
  l->m=m;

  drft_init(&l->fft,mapped*2);
}

/* lpc_clear. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054cc60
void lpc_clear(lpc_lookup *l){
  if(l){
    drft_clear(&l->fft);
  }
}
