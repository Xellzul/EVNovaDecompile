// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto or -align 4 loses MATCHes here; inlined: ilog, ilog2, render_point, vorbis_dBquant, post_Y); Research/notes/burgervorbis.md
/* floor1.c -- libvorbis 1.0rc3 lib/floor1.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (FastMemSet, oggpack_read1 for the nonzero flag); altered: comments and the TRAIN_FLOOR1 branches (not built) removed. Notes: Research/notes/burgervorbis.md */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"
#include "scales.h"

#include <stdio.h>

#define floor1_rangedB 140

typedef struct vorbis_look_floor{
  int sorted_index[VIF_POSIT+2];
  int forward_index[VIF_POSIT+2];
  int reverse_index[VIF_POSIT+2];

  int hineighbor[VIF_POSIT];
  int loneighbor[VIF_POSIT];
  int posts;

  int n;
  int quant_q;
  vorbis_info_floor1 *vi;

  long phrasebits;
  long postbits;
  long frames;
} vorbis_look_floor1;

typedef struct lsfit_acc{
  long x0;
  long x1;

  long xa;
  long ya;
  long x2a;
  long y2a;
  long xya;
  long n;
  long an;
  long un;
  long edgey0;
  long edgey1;
} lsfit_acc;

/* floor1_copy_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552ed0
static vorbis_info_floor *floor1_copy_info (vorbis_info_floor *i){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  vorbis_info_floor1 *ret=(vorbis_info_floor1 *)_ogg_malloc(sizeof(*ret));
  memcpy(ret,info,sizeof(*ret));
  return((vorbis_info_floor *)ret);
}

/* floor1_free_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552f20
static void floor1_free_info(vorbis_info_floor *i){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  if(info){
    FastMemSet(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

/* floor1_free_look. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552f60
static void floor1_free_look(vorbis_look_floor *i){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)i;
  if(look){
    FastMemSet(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* floor1_pack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552fa0
static void floor1_pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  int j,k;
  int count=0;
  int rangebits;
  int maxposit=info->postlist[1];
  int maxclass=-1;

  oggpack_write(opb,info->partitions,5);
  for(j=0;j<info->partitions;j++){
    oggpack_write(opb,info->partitionclass[j],4);
    if(maxclass<info->partitionclass[j])maxclass=info->partitionclass[j];
  }

  for(j=0;j<maxclass+1;j++){
    oggpack_write(opb,info->class_dim[j]-1,3);
    oggpack_write(opb,info->class_subs[j],2);
    if(info->class_subs[j])oggpack_write(opb,info->class_book[j],8);
    for(k=0;k<(1<<info->class_subs[j]);k++)
      oggpack_write(opb,info->class_subbook[j][k]+1,8);
  }

  oggpack_write(opb,info->mult-1,2);
  oggpack_write(opb,ilog2(maxposit),4);
  rangebits=ilog2(maxposit);

  for(j=0,k=0;j<info->partitions;j++){
    count+=info->class_dim[info->partitionclass[j]];
    for(;k<count;k++)
      oggpack_write(opb,info->postlist[k+2],rangebits);
  }
}

/* floor1_unpack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00553230
static vorbis_info_floor *floor1_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  int j,k,count=0,maxclass=-1,rangebits;

  vorbis_info_floor1 *info=(vorbis_info_floor1 *)_ogg_calloc(1,sizeof(*info));

  info->partitions=oggpack_read(opb,5);
  for(j=0;j<info->partitions;j++){
    info->partitionclass[j]=oggpack_read(opb,4);
    if(maxclass<info->partitionclass[j])maxclass=info->partitionclass[j];
  }

  for(j=0;j<maxclass+1;j++){
    info->class_dim[j]=oggpack_read(opb,3)+1;
    info->class_subs[j]=oggpack_read(opb,2);
    if(info->class_subs[j]<0)
      goto err_out;
    if(info->class_subs[j])info->class_book[j]=oggpack_read(opb,8);
    if(info->class_book[j]<0 || info->class_book[j]>=ci->books)
      goto err_out;
    for(k=0;k<(1<<info->class_subs[j]);k++){
      info->class_subbook[j][k]=oggpack_read(opb,8)-1;
      if(info->class_subbook[j][k]<-1 || info->class_subbook[j][k]>=ci->books)
	goto err_out;
    }
  }

  info->mult=oggpack_read(opb,2)+1;
  rangebits=oggpack_read(opb,4);

  for(j=0,k=0;j<info->partitions;j++){
    count+=info->class_dim[info->partitionclass[j]];
    for(;k<count;k++){
      int t=info->postlist[k+2]=oggpack_read(opb,rangebits);
      if(t<0 || t>=(1<<rangebits))
	goto err_out;
    }
  }
  info->postlist[0]=0;
  info->postlist[1]=1<<rangebits;

  return((vorbis_info_floor *)info);

 err_out:
  floor1_free_info((vorbis_info_floor *)info);
  return(NULL);
}

/* icomp (qsort comparator). STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00553550
static int icomp(const void *a,const void *b){
  return(**(int **)a-**(int **)b);
}

/* floor1_look. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00553570
static vorbis_look_floor *floor1_look(vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *in){

  int *sortpointer[VIF_POSIT+2];
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)in;
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)_ogg_calloc(1,sizeof(*look));
  int i,j,n=0;

  look->vi=info;
  look->n=info->postlist[1];

  for(i=0;i<info->partitions;i++)n+=info->class_dim[info->partitionclass[i]];
  n+=2;
  look->posts=n;

  for(i=0;i<n;i++)sortpointer[i]=info->postlist+i;
  qsort(sortpointer,n,sizeof(*sortpointer),icomp);

  for(i=0;i<n;i++)look->forward_index[i]=sortpointer[i]-info->postlist;

  for(i=0;i<n;i++)look->reverse_index[look->forward_index[i]]=i;

  for(i=0;i<n;i++)look->sorted_index[i]=info->postlist[look->forward_index[i]];

  switch(info->mult){
  case 1:
    look->quant_q=256;
    break;
  case 2:
    look->quant_q=128;
    break;
  case 3:
    look->quant_q=86;
    break;
  case 4:
    look->quant_q=64;
    break;
  }

  for(i=0;i<n-2;i++){
    int lo=0;
    int hi=1;
    int lx=0;
    int hx=look->n;
    int currentx=info->postlist[i+2];
    for(j=0;j<i+2;j++){
      int x=info->postlist[j];
      if(x>lx && x<currentx){
	lo=j;
	lx=x;
      }
      if(x<hx && x>currentx){
	hi=j;
	hx=x;
      }
    }
    look->loneighbor[i]=lo;
    look->hineighbor[i]=hi;
  }

  return(look);
}

static int render_point(int x0,int x1,int y0,int y1,int x){
  y0&=0x7fff;
  y1&=0x7fff;

  {
    int dy=y1-y0;
    int adx=x1-x0;
    int ady=abs(dy);
    int err=ady*(x-x0);

    int off=err/adx;
    if(dy<0)return(y0-off);
    return(y0+off);
  }
}

static int vorbis_dBquant(const float *x){
  int i= *x*7.3142857f+1023.5f;
  if(i>1023)return(1023);
  if(i<0)return(0);
  return i;
}

static float FLOOR_fromdB_LOOKUP[256]={
	1.0649863e-07F, 1.1341951e-07F, 1.2079015e-07F, 1.2863978e-07F,
	1.3699951e-07F, 1.4590251e-07F, 1.5538408e-07F, 1.6548181e-07F,
	1.7623575e-07F, 1.8768855e-07F, 1.9988561e-07F, 2.128753e-07F,
	2.2670913e-07F, 2.4144197e-07F, 2.5713223e-07F, 2.7384213e-07F,
	2.9163793e-07F, 3.1059021e-07F, 3.3077411e-07F, 3.5226968e-07F,
	3.7516214e-07F, 3.9954229e-07F, 4.2550680e-07F, 4.5315863e-07F,
	4.8260743e-07F, 5.1396998e-07F, 5.4737065e-07F, 5.8294187e-07F,
	6.2082472e-07F, 6.6116941e-07F, 7.0413592e-07F, 7.4989464e-07F,
	7.9862701e-07F, 8.5052630e-07F, 9.0579828e-07F, 9.6466216e-07F,
	1.0273513e-06F, 1.0941144e-06F, 1.1652161e-06F, 1.2409384e-06F,
	1.3215816e-06F, 1.4074654e-06F, 1.4989305e-06F, 1.5963394e-06F,
	1.7000785e-06F, 1.8105592e-06F, 1.9282195e-06F, 2.0535261e-06F,
	2.1869758e-06F, 2.3290978e-06F, 2.4804557e-06F, 2.6416497e-06F,
	2.8133190e-06F, 2.9961443e-06F, 3.1908506e-06F, 3.3982101e-06F,
	3.6190449e-06F, 3.8542308e-06F, 4.1047004e-06F, 4.3714470e-06F,
	4.6555282e-06F, 4.9580707e-06F, 5.2802740e-06F, 5.6234160e-06F,
	5.9888572e-06F, 6.3780469e-06F, 6.7925283e-06F, 7.2339451e-06F,
	7.7040476e-06F, 8.2047000e-06F, 8.7378876e-06F, 9.3057248e-06F,
	9.9104632e-06F, 1.0554501e-05F, 1.1240392e-05F, 1.1970856e-05F,
	1.2748789e-05F, 1.3577278e-05F, 1.4459606e-05F, 1.5399272e-05F,
	1.6400004e-05F, 1.7465768e-05F, 1.8600792e-05F, 1.9809576e-05F,
	2.1096914e-05F, 2.2467911e-05F, 2.3928002e-05F, 2.5482978e-05F,
	2.7139006e-05F, 2.8902651e-05F, 3.0780908e-05F, 3.2781225e-05F,
	3.4911534e-05F, 3.7180282e-05F, 3.9596466e-05F, 4.2169667e-05F,
	4.4910090e-05F, 4.7828601e-05F, 5.0936773e-05F, 5.4246931e-05F,
	5.7772202e-05F, 6.1526565e-05F, 6.5524908e-05F, 6.9783085e-05F,
	7.4317983e-05F, 7.9147585e-05F, 8.4291040e-05F, 8.9768747e-05F,
	9.5602426e-05F, 0.00010181521F, 0.00010843174F, 0.00011547824F,
	0.00012298267F, 0.00013097477F, 0.00013948625F, 0.00014855085F,
	0.00015820453F, 0.00016848555F, 0.00017943469F, 0.00019109536F,
	0.00020351382F, 0.00021673929F, 0.00023082423F, 0.00024582449F,
	0.00026179955F, 0.00027881276F, 0.00029693158F, 0.00031622787F,
	0.00033677814F, 0.00035866388F, 0.00038197188F, 0.00040679456F,
	0.00043323036F, 0.00046138411F, 0.00049136745F, 0.00052329927F,
	0.00055730621F, 0.00059352311F, 0.00063209358F, 0.00067317058F,
	0.00071691700F, 0.00076350630F, 0.00081312324F, 0.00086596457F,
	0.00092223983F, 0.00098217216F, 0.0010459992F, 0.0011139742F,
	0.0011863665F, 0.0012634633F, 0.0013455702F, 0.0014330129F,
	0.0015261382F, 0.0016253153F, 0.0017309374F, 0.0018434235F,
	0.0019632195F, 0.0020908006F, 0.0022266726F, 0.0023713743F,
	0.0025254795F, 0.0026895994F, 0.0028643847F, 0.0030505286F,
	0.0032487691F, 0.0034598925F, 0.0036847358F, 0.0039241906F,
	0.0041792066F, 0.0044507950F, 0.0047400328F, 0.0050480668F,
	0.0053761186F, 0.0057254891F, 0.0060975636F, 0.0064938176F,
	0.0069158225F, 0.0073652516F, 0.0078438871F, 0.0083536271F,
	0.0088964928F, 0.009474637F, 0.010090352F, 0.010746080F,
	0.011444421F, 0.012188144F, 0.012980198F, 0.013823725F,
	0.014722068F, 0.015678791F, 0.016697687F, 0.017782797F,
	0.018938423F, 0.020169149F, 0.021479854F, 0.022875735F,
	0.024362330F, 0.025945531F, 0.027631618F, 0.029427276F,
	0.031339626F, 0.033376252F, 0.035545228F, 0.037855157F,
	0.040315199F, 0.042935108F, 0.045725273F, 0.048696758F,
	0.051861348F, 0.055231591F, 0.058820850F, 0.062643361F,
	0.066714279F, 0.071049749F, 0.075666962F, 0.080584227F,
	0.085821044F, 0.091398179F, 0.097337747F, 0.10366330F,
	0.11039993F, 0.11757434F, 0.12521498F, 0.13335215F,
	0.14201813F, 0.15124727F, 0.16107617F, 0.17154380F,
	0.18269168F, 0.19456402F, 0.20720788F, 0.22067342F,
	0.23501402F, 0.25028656F, 0.26655159F, 0.28387361F,
	0.30232132F, 0.32196786F, 0.34289114F, 0.36517414F,
	0.38890521F, 0.41417847F, 0.44109412F, 0.46975890F,
	0.50028648F, 0.53279791F, 0.56742212F, 0.60429640F,
	0.64356699F, 0.68538959F, 0.72993007F, 0.77736504F,
	0.82788260F, 0.88168307F, 0.9389798F, 1.F,
};

/* render_line. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00553c70
static void render_line(int x0,int x1,int y0,int y1,float *d){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;

  ady-=abs(base*adx);

  d[x]*=FLOOR_fromdB_LOOKUP[y];
  while(++x<x1){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }
    d[x]*=FLOOR_fromdB_LOOKUP[y];
  }
}

/* render_line0. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00553d70
static void render_line0(int x0,int x1,int y0,int y1,float *d){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;

  ady-=abs(base*adx);

  d[x]=FLOOR_fromdB_LOOKUP[y];
  while(++x<x1){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }
    d[x]=FLOOR_fromdB_LOOKUP[y];
  }
}

/* accumulate_fit. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00553e60
static int accumulate_fit(const float *flr,const float *mdct,
			  int x0, int x1,lsfit_acc *a,
			  int n,vorbis_info_floor1 *info){
  long i;
  int quantized=vorbis_dBquant(flr+x0);

  long xa=0,ya=0,x2a=0,y2a=0,xya=0,na=0, xb=0,yb=0,x2b=0,y2b=0,xyb=0,nb=0;

  FastMemSet(a,0,sizeof(*a));
  a->x0=x0;
  a->x1=x1;
  a->edgey0=quantized;
  if(x1>n)x1=n;

  for(i=x0;i<x1;i++){
    int quantized=vorbis_dBquant(flr+i);
    if(quantized){
      if(mdct[i]+info->twofitatten>=flr[i]){
	xa  += i;
	ya  += quantized;
	x2a += i*i;
	y2a += quantized*quantized;
	xya += i*quantized;
	na++;
      }else{
	xb  += i;
	yb  += quantized;
	x2b += i*i;
	y2b += quantized*quantized;
	xyb += i*quantized;
	nb++;
      }
    }
  }

  xb+=xa;
  yb+=ya;
  x2b+=x2a;
  y2b+=y2a;
  xyb+=xya;
  nb+=na;

  {
    int weight;
    if(nb<info->twofitminsize || na<info->twofitminused){
      weight=0;
    }else{
      weight=nb*info->twofitweight/na;
    }
    a->xa=xa*weight+xb;
    a->ya=ya*weight+yb;
    a->x2a=x2a*weight+x2b;
    a->y2a=y2a*weight+y2b;
    a->xya=xya*weight+xyb;
    a->an=na*weight+nb;
    a->n=nb;
    a->un=na;
    if(nb>=info->unusedminsize)a->un++;
  }

  a->edgey1=-200;
  if(x1<n){
    int quantized=vorbis_dBquant(flr+i);
    a->edgey1=quantized;
  }
  return(a->n);
}

/* fit_line. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00554250
static int fit_line(lsfit_acc *a,int fits,int *y0,int *y1){
  long x=0,y=0,x2=0,y2=0,xy=0,n=0,an=0,i;
  long x0=a[0].x0;
  long x1=a[fits-1].x1;

  for(i=0;i<fits;i++){
    if(a[i].un){
      x+=a[i].xa;
      y+=a[i].ya;
      x2+=a[i].x2a;
      y2+=a[i].y2a;
      xy+=a[i].xya;
      n+=a[i].n;
      an+=a[i].an;
    }
  }

  if(*y0>=0){
    x+=   x0;
    y+=  *y0;
    x2+=  x0 *  x0;
    y2+= *y0 * *y0;
    xy+= *y0 *  x0;
    n++;
    an++;
  }

  if(*y1>=0){
    x+=   x1;
    y+=  *y1;
    x2+=  x1 *  x1;
    y2+= *y1 * *y1;
    xy+= *y1 *  x1;
    n++;
    an++;
  }

  if(n<2)return(n-2);

  {
    double fx=x;
    double fy=y;
    double fx2=x2;
    double fxy=xy;
    double denom=1./(an*fx2-fx*fx);
    double a=(fy*fx2-fxy*fx)*denom;
    double b=(an*fxy-fx*fy)*denom;
    *y0=rint(a+b*x0);
    *y1=rint(a+b*x1);

    if(*y0>1023)*y0=1023;
    if(*y1>1023)*y1=1023;
    if(*y0<0)*y0=0;
    if(*y1<0)*y1=0;

    return(0);
  }
}

/* inspect_error. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00554500
static int inspect_error(int x0,int x1,int y0,int y1,const float *mask,
			 const float *mdct,
			 vorbis_info_floor1 *info){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;
  int val=vorbis_dBquant(mask+x);
  int mse=0;
  int n=0;

  ady-=abs(base*adx);

  if(mdct[x]+info->twofitatten>=mask[x]){
    if(y+info->maxover<val)return(1);
    if(y-info->maxunder>val)return(1);
    mse=(y-val);
    mse*=mse;
    n++;
  }

  while(++x<x1){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }

    if(mdct[x]+info->twofitatten>=mask[x]){
      val=vorbis_dBquant(mask+x);
      if(val){
	if(y+info->maxover<val)return(1);
	if(y-info->maxunder>val)return(1);
	mse+=((y-val)*(y-val));
	n++;
      }
    }
  }

  if(n){
    if(info->maxover*info->maxover/n>info->maxerr)return(0);
    if(info->maxunder*info->maxunder/n>info->maxerr)return(0);
    if(mse/n>info->maxerr)return(1);
  }
  return(0);
}

static int post_Y(int *A,int *B,int pos){
  if(A[pos]<0)
    return B[pos];
  if(B[pos]<0)
    return A[pos];

  return (A[pos]+B[pos])>>1;
}

/* floor1_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00554960
static int floor1_forward(vorbis_block *vb,vorbis_look_floor *in,
			  float *mdct, const float *logmdct,
			  const float *logmask, const float *logmax,
			  float *codedflr){
  static int seq=0;
  long i,j,k,l;
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;
  long n=info->n;
  long posts=look->posts;
  long nonzero=0;
  lsfit_acc fits[VIF_POSIT+1];
  int fit_valueA[VIF_POSIT+2];
  int fit_valueB[VIF_POSIT+2];
  int fit_flag[VIF_POSIT+2];

  int loneighbor[VIF_POSIT+2];
  int hineighbor[VIF_POSIT+2];
  int memo[VIF_POSIT+2];
  codec_setup_info *ci=(codec_setup_info *)vb->vd->vi->codec_setup;
  static_codebook **sbooks=ci->book_param;
  codebook *books=NULL;
  int writeflag=0;

  if(vb->vd->backend_state){
    books=((backend_lookup_state *)(vb->vd->backend_state))->
      fullbooks;
    writeflag=1;
  }

  FastMemSet(fit_flag,0,sizeof(fit_flag));
  for(i=0;i<posts;i++)loneighbor[i]=0;
  for(i=0;i<posts;i++)hineighbor[i]=1;
  for(i=0;i<posts;i++)memo[i]=-1;

  for(;n>info->unusedmin_n;n--)
    if(logmdct[n-1]>-floor1_rangedB &&
       logmdct[n-1]+info->twofitatten>logmask[n-1])break;

  if(posts==0){
    nonzero+=accumulate_fit(logmask,logmax,0,n,fits,n,info);
  }else{
    for(i=0;i<posts-1;i++)
      nonzero+=accumulate_fit(logmask,logmax,look->sorted_index[i],
			      look->sorted_index[i+1],fits+i,
			      n,info);
  }

  if(nonzero){
    int y0=-200;
    int y1=-200;
    int mse=fit_line(fits,posts-1,&y0,&y1);
    if(mse<0){
      y0=-200;
      y1=0;
      fit_line(fits,posts-1,&y0,&y1);
    }

    fit_flag[0]=1;
    fit_flag[1]=1;
    fit_valueA[0]=y0;
    fit_valueB[0]=y0;
    fit_valueB[1]=y1;
    fit_valueA[1]=y1;

    if(mse>=0){
      for(i=2;i<posts;i++){
	int sortpos=look->reverse_index[i];
	int ln=loneighbor[sortpos];
	int hn=hineighbor[sortpos];

	if(memo[ln]!=hn){
	  int lsortpos=look->reverse_index[ln];
	  int hsortpos=look->reverse_index[hn];
	  memo[ln]=hn;

	  for(j=lsortpos;j<hsortpos;j++)
	    if(fits[j].un)break;
	  if(j==hsortpos){
	    fit_valueB[ln]=-200;
	    if(fit_valueA[ln]<0)
	      fit_flag[ln]=0;
	    fit_valueA[hn]=-200;
	    if(fit_valueB[hn]<0)
	      fit_flag[hn]=0;
	  }else{
	    int lx=info->postlist[ln];
	    int hx=info->postlist[hn];
	    int ly=post_Y(fit_valueA,fit_valueB,ln);
	    int hy=post_Y(fit_valueA,fit_valueB,hn);

	    if(inspect_error(lx,hx,ly,hy,logmask,logmdct,info)){
	      int ly0=-200;
	      int ly1=-200;
	      int hy0=-200;
	      int hy1=-200;
	      int lmse=fit_line(fits+lsortpos,sortpos-lsortpos,&ly0,&ly1);
	      int hmse=fit_line(fits+sortpos,hsortpos-sortpos,&hy0,&hy1);

	      switch(lmse){
	      case -2:
		break;
	      case -1:
		ly0=fits[lsortpos].edgey0;
		break;
	      }

	      switch(hmse){
	      case -2:
		break;
	      case -1:
		hy0=fits[sortpos].edgey0;
		break;
	      }

	      fit_valueB[ln]=ly0;
	      if(ln==0 && ly0>=0)fit_valueA[ln]=ly0;
	      fit_valueA[i]=ly1;
	      fit_valueB[i]=hy0;
	      fit_valueA[hn]=hy1;
	      if(hn==1 && hy1>=0)fit_valueB[hn]=hy1;

	      if(ly0<0 && fit_valueA[ln]<0)
		fit_flag[ln]=0;
	      if(hy1<0 && fit_valueB[hn]<0)
		fit_flag[hn]=0;

	      if(ly1>=0 || hy0>=0){
		for(j=sortpos-1;j>=0;j--)
		  if(hineighbor[j]==hn)
		  hineighbor[j]=i;
		  else
		    break;
		for(j=sortpos+1;j<posts;j++)
		  if(loneighbor[j]==ln)
		    loneighbor[j]=i;
		  else
		    break;

		fit_flag[i]=1;
	      }
	    }
	  }
	}
      }
    }

    switch(info->mult){
    case 1:
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)>>2;
      break;
    case 2:
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)>>3;
      break;
    case 3:
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)/12;
      break;
    case 4:
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)>>4;
      break;
    }

    for(i=2;i<posts;i++){
      int sp=look->reverse_index[i];
      int ln=look->loneighbor[i-2];
      int hn=look->hineighbor[i-2];
      int x0=info->postlist[ln];
      int x1=info->postlist[hn];
      int y0=fit_valueA[ln];
      int y1=fit_valueA[hn];

      int predicted=render_point(x0,x1,y0,y1,info->postlist[i]);

      if(fit_flag[i]){
	int headroom=(look->quant_q-predicted<predicted?
		      look->quant_q-predicted:predicted);

	int val=fit_valueA[i]-predicted;

	if(val<0)
	  if(val<-headroom)
	    val=headroom-val-1;
	  else
	    val=-1-(val<<1);
	else
	  if(val>=headroom)
	    val= val+headroom;
	  else
	    val<<=1;

	fit_valueB[i]=val;

	for(j=sp+1;j<posts;j++)
	  if(loneighbor[j]==i)
	    loneighbor[j]=loneighbor[sp];
	  else
	    break;
	for(j=sp-1;j>=0;j--)
	  if(hineighbor[j]==i)
	    hineighbor[j]=hineighbor[sp];
	  else
	    break;

      }else{
	fit_valueA[i]=predicted;
	fit_valueB[i]=0;
      }
      if(fit_valueB[i]==0)
	fit_valueA[i]|=0x8000;
      else{
	fit_valueA[look->loneighbor[i-2]]&=0x7fff;
	fit_valueA[look->hineighbor[i-2]]&=0x7fff;
      }
    }

    if(writeflag){
      oggpack_write(&vb->opb,1,1);

      look->frames++;
      look->postbits+=ilog(look->quant_q-1)*2;
      oggpack_write(&vb->opb,fit_valueA[0],ilog(look->quant_q-1));
      oggpack_write(&vb->opb,fit_valueA[1],ilog(look->quant_q-1));

      for(i=0,j=2;i<info->partitions;i++){
	int class_=info->partitionclass[i];
	int cdim=info->class_dim[class_];
	int csubbits=info->class_subs[class_];
	int csub=1<<csubbits;
	int bookas[8]={0,0,0,0,0,0,0,0};
	int cval=0;
	int cshift=0;

	if(csubbits){
	  int maxval[8];
	  for(k=0;k<csub;k++){
	    int booknum=info->class_subbook[class_][k];
	    if(booknum<0){
	      maxval[k]=1;
	    }else{
	      maxval[k]=sbooks[info->class_subbook[class_][k]]->entries;
	    }
	  }
	  for(k=0;k<cdim;k++){
	    for(l=0;l<csub;l++){
	      int val=fit_valueB[j+k];
	      if(val<maxval[l]){
		bookas[k]=l;
		break;
	      }
	    }
	    cval|= bookas[k]<<cshift;
	    cshift+=csubbits;
	  }

	  look->phrasebits+=
	  vorbis_book_encode(books+info->class_book[class_],cval,&vb->opb);
	}

	for(k=0;k<cdim;k++){
	  int book=info->class_subbook[class_][bookas[k]];
	  if(book>=0){
	    if(fit_valueB[j+k]<(books+book)->entries)
	      look->postbits+=vorbis_book_encode(books+book,
						 fit_valueB[j+k],&vb->opb);
	  }
	}
	j+=cdim;
      }
    }

    {
      int hx;
      int lx=0;
      int ly=fit_valueA[0]*info->mult;

      for(j=1;j<posts;j++){
	int current=look->forward_index[j];
	if(!(fit_valueA[current]&0x8000)){
	  int hy=(fit_valueA[current]&0x7fff)*info->mult;
	  hx=info->postlist[current];

	  render_line0(lx,hx,ly,hy,codedflr);

	  lx=hx;
	  ly=hy;
	}
      }
      for(j=lx;j<vb->pcmend/2;j++)codedflr[j]=codedflr[j-1];

      for(j=0;j<n;j++)
	if(logmdct[j]+info->twofitatten<logmask[j])
	  mdct[j]=0.f;
      for(j=n;j<vb->pcmend/2;j++)mdct[j]=0.f;
    }

  }else{
    if(writeflag)oggpack_write(&vb->opb,0,1);
    FastMemSet(codedflr,0,n*sizeof(*codedflr));
    FastMemSet(mdct,0,n*sizeof(*mdct));
  }
  seq++;
  return(nonzero);
}

/* floor1_inverse1. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00555c40
static void *floor1_inverse1(vorbis_block *vb,vorbis_look_floor *in){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;

  int i,j,k;
  codebook *books=((backend_lookup_state *)(vb->vd->backend_state))->
    fullbooks;

  if(oggpack_read1(&vb->opb)==1){
    int *fit_value=(int *)_vorbis_block_alloc(vb,(look->posts)*sizeof(*fit_value));

    fit_value[0]=oggpack_read(&vb->opb,ilog(look->quant_q-1));
    fit_value[1]=oggpack_read(&vb->opb,ilog(look->quant_q-1));

    for(i=0,j=2;i<info->partitions;i++){
      int class_=info->partitionclass[i];
      int cdim=info->class_dim[class_];
      int csubbits=info->class_subs[class_];
      int csub=1<<csubbits;
      int cval=0;

      if(csubbits){
	cval=vorbis_book_decode(books+info->class_book[class_],&vb->opb);

	if(cval==-1)goto eop;
      }

      for(k=0;k<cdim;k++){
	int book=info->class_subbook[class_][cval&(csub-1)];
	cval>>=csubbits;
	if(book>=0){
	  if((fit_value[j+k]=vorbis_book_decode(books+book,&vb->opb))==-1)
	    goto eop;
	}else{
	  fit_value[j+k]=0;
	}
      }
      j+=cdim;
    }

    for(i=2;i<look->posts;i++){
      int predicted=render_point(info->postlist[look->loneighbor[i-2]],
				 info->postlist[look->hineighbor[i-2]],
				 fit_value[look->loneighbor[i-2]],
				 fit_value[look->hineighbor[i-2]],
				 info->postlist[i]);
      int hiroom=look->quant_q-predicted;
      int loroom=predicted;
      int room=(hiroom<loroom?hiroom:loroom)<<1;
      int val=fit_value[i];

      if(val){
	if(val>=room){
	  if(hiroom>loroom){
	    val = val-loroom;
	  }else{
	  val = -1-(val-hiroom);
	  }
	}else{
	  if(val&1){
	    val= -((val+1)>>1);
	  }else{
	    val>>=1;
	  }
	}

	fit_value[i]=val+predicted;
	fit_value[look->loneighbor[i-2]]&=0x7fff;
	fit_value[look->hineighbor[i-2]]&=0x7fff;

      }else{
	fit_value[i]=predicted|0x8000;
      }
    }

    return(fit_value);
  }
 eop:
  return(NULL);
}

/* floor1_inverse2. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556000
static int floor1_inverse2(vorbis_block *vb,vorbis_look_floor *in,void *memo,
			  float *out){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;

  codec_setup_info   *ci=(codec_setup_info *)vb->vd->vi->codec_setup;
  int                  n=ci->blocksizes[vb->mode]/2;
  int j;

  if(memo){
    int *fit_value=(int *)memo;
    int hx=0;
    int lx=0;
    int ly=fit_value[0]*info->mult;
    for(j=1;j<look->posts;j++){
      int current=look->forward_index[j];
      int hy=fit_value[current]&0x7fff;
      if(hy==fit_value[current]){
	hy*=info->mult;
	hx=info->postlist[current];

	render_line(lx,hx,ly,hy,out);

	lx=hx;
	ly=hy;
      }
    }
    for(j=hx;j<n;j++)out[j]*=ly;
    return(1);
  }
  FastMemSet(out,0,sizeof(*out)*n);
  return(0);
}

vorbis_func_floor floor1_exportbundle={
  &floor1_pack,&floor1_unpack,&floor1_look,&floor1_copy_info,&floor1_free_info,
  &floor1_free_look,&floor1_forward,&floor1_inverse1,&floor1_inverse2
};
