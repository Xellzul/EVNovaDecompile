// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto loses a MATCH here; inlined: cheby); Research/notes/burgervorbis.md
/* lsp.c -- libvorbis 1.0rc3 lib/lsp.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. Stock in BurgerLib's copy; altered: comments, the FLOAT_LOOKUP and INT_LOOKUP branches (#undef'd by the file itself) and with them lookup.h removed. Notes: Research/notes/burgervorbis.md */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "lsp.h"
#include "os.h"
#include "misc.h"
#include "scales.h"

#undef   FLOAT_LOOKUP
#undef    INT_LOOKUP

/* vorbis_lsp_to_curve. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00567560
void vorbis_lsp_to_curve(float *curve,int *map,int n,int ln,float *lsp,int m,
			    float amp,float ampoffset){
  int i;
  float wdel=M_PI/ln;
  for(i=0;i<m;i++)lsp[i]=2.f*cos(lsp[i]);

  i=0;
  while(i<n){
    int j,k=map[i];
    float p=.5f;
    float q=.5f;
    float w=2.f*cos(wdel*k);
    for(j=1;j<m;j+=2){
      q *= w-lsp[j-1];
      p *= w-lsp[j];
    }
    if(j==m){
      q*=w-lsp[j-1];
      p*=p*(4.f-w*w);
      q*=q;
    }else{
      p*=p*(2.f-w);
      q*=q*(2.f+w);
    }

    q=fromdB(amp/sqrt(p+q)-ampoffset);

    curve[i]*=q;
    while(map[++i]==k)curve[i]*=q;
  }
}

static void cheby(float *g, int ord) {
  int i, j;

  g[0] *= .5f;
  for(i=2; i<= ord; i++) {
    for(j=ord; j >= i; j--) {
      g[j-2] -= g[j];
      g[j] += g[j];
    }
  }
}

/* comp (qsort comparator). STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00567870
static int comp(const void *a,const void *b){
  if(*(float *)a<*(float *)b)
    return(1);
  else
    return(-1);
}

#define EPSILON 10e-7
/* Laguerre_With_Deflation. STATE: BurgerLib's libvorbis 1.0rc3, the convergence test on a
   float (MATCH, byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x005678a0
static int Laguerre_With_Deflation(float *a,int ord,float *r){
  int i,m;
  double lastdelta=0.f;
  double *defl=(double *)alloca(sizeof(*defl)*(ord+1));
  for(i=0;i<=ord;i++)defl[i]=a[i];

  for(m=ord;m>0;m--){
    double new_=0.f,delta;

    while(1){
      double p=defl[m],pp=0.f,ppp=0.f,denom;

      for(i=m;i>0;i--){
	ppp = new_*ppp + pp;
	pp  = new_*pp  + p;
	p   = new_*p   + defl[i-1];
      }

      denom=(m-1) * ((m-1)*pp*pp - m*p*ppp);
      if(denom<0)
	return(-1);

      if(pp>0){
	denom = pp + sqrt(denom);
	if(denom<EPSILON)denom=EPSILON;
      }else{
	denom = pp - sqrt(denom);
	if(denom>-(EPSILON))denom=-(EPSILON);
      }

      delta  = m*p/denom;
      new_  -= delta;

      if(delta<0.f)delta*=-1;

      if((float)fabs(delta/new_)<10e-12)break;
      lastdelta=delta;
    }

    r[m-1]=new_;

    for(i=m;i>0;i--)
      defl[i-1]+=new_*defl[i];
    defl++;
  }
  return(0);
}

/* Newton_Raphson. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00567d90
static int Newton_Raphson(float *a,int ord,float *r){
  int i, k, count=0;
  double error=1.f;
  double *root=(double *)alloca(ord*sizeof(*root));

  for(i=0; i<ord;i++) root[i] = r[i];

  while(error>1e-20){
    error=0;

    for(i=0; i<ord; i++) {
      double pp=0.,delta;
      double rooti=root[i];
      double p=a[ord];
      for(k=ord-1; k>= 0; k--) {
	pp= pp* rooti + p;
	p = p * rooti + a[k];
      }

      delta = p/pp;
      root[i] -= delta;
      error+= delta*delta;
    }

    if(count>40)return(-1);

    count++;
  }

  for(i=0; i<ord;i++) r[i] = root[i];
  return(0);
}

/* vorbis_lpc_to_lsp (cheby inlined). STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00568110
int vorbis_lpc_to_lsp(float *lpc,float *lsp,int m){
  int order2=(m+1)>>1;
  int g1_order,g2_order;
  float *g1=(float *)alloca(sizeof(*g1)*(order2+1));
  float *g2=(float *)alloca(sizeof(*g2)*(order2+1));
  float *g1r=(float *)alloca(sizeof(*g1r)*(order2+1));
  float *g2r=(float *)alloca(sizeof(*g2r)*(order2+1));
  int i;

  g1_order=(m+1)>>1;
  g2_order=(m)  >>1;

  g1[g1_order] = 1.f;
  for(i=1;i<=g1_order;i++) g1[g1_order-i] = lpc[i-1]+lpc[m-i];
  g2[g2_order] = 1.f;
  for(i=1;i<=g2_order;i++) g2[g2_order-i] = lpc[i-1]-lpc[m-i];

  if(g1_order>g2_order){
    for(i=2; i<=g2_order;i++) g2[g2_order-i] += g2[g2_order-i+2];
  }else{
    for(i=1; i<=g1_order;i++) g1[g1_order-i] -= g1[g1_order-i+1];
    for(i=1; i<=g2_order;i++) g2[g2_order-i] += g2[g2_order-i+1];
  }

  cheby(g1,g1_order);
  cheby(g2,g2_order);

  if(Laguerre_With_Deflation(g1,g1_order,g1r) ||
     Laguerre_With_Deflation(g2,g2_order,g2r))
    return(-1);

  Newton_Raphson(g1,g1_order,g1r);
  Newton_Raphson(g2,g2_order,g2r);

  qsort(g1r,g1_order,sizeof(*g1r),comp);
  qsort(g2r,g2_order,sizeof(*g2r),comp);

  for(i=0;i<g1_order;i++)
    lsp[i*2] = acos(g1r[i]);

  for(i=0;i<g2_order;i++)
    lsp[i*2+1] = acos(g2r[i]);
  return(0);
}
