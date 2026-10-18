// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model (floor0_look's rate/2.f as fmul 0.5); C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto or -align 4 loses MATCHes here); Research/notes/burgervorbis.md
/* floor0.c -- libvorbis 1.0rc3 lib/floor0.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (FastMemSet); altered: comments and the TRAIN_LSP branches (not built) removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "lpc.h"
#include "lsp.h"
#include "codebook.h"
#include "scales.h"
#include "misc.h"
#include "os.h"

#include "misc.h"
#include <stdio.h>

typedef struct vorbis_look_floor{
  long n;
  int ln;
  int  m;
  int *linearmap;

  vorbis_info_floor0 *vi;
  lpc_lookup lpclook;
  float *lsp_look;

  long bits;
  long frames;
} vorbis_look_floor0;

/* _f0_fit. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556260
static long _f0_fit(codebook *book,
		    float *orig,
		    float *workfit,
		    int cursor){
  int dim=book->dim;
  float norm,base=0.f;
  int i,best=0;
  float *lsp=workfit+cursor;

  if(cursor)base=workfit[cursor-1];
  norm=orig[cursor+dim-1]-base;

  for(i=0;i<dim;i++)
    lsp[i]=(orig[i+cursor]-base);
  best=_best(book,lsp,1);

  memcpy(lsp,book->valuelist+best*dim,dim*sizeof(*lsp));
  for(i=0;i<dim;i++)
    lsp[i]+=base;
  return(best);
}

/* floor0_copy_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556450
static vorbis_info_floor *floor0_copy_info (vorbis_info_floor *i){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_info_floor0 *ret=(vorbis_info_floor0 *)_ogg_malloc(sizeof(*ret));
  memcpy(ret,info,sizeof(*ret));
  return(ret);
}

/* floor0_free_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005564a0
static void floor0_free_info(vorbis_info_floor *i){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  if(info){
    FastMemSet(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

/* floor0_free_look. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005564e0
static void floor0_free_look(vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  if(look){
    if(look->linearmap)_ogg_free(look->linearmap);
    if(look->lsp_look)_ogg_free(look->lsp_look);
    lpc_clear(&look->lpclook);
    FastMemSet(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

/* floor0_pack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556560
static void floor0_pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  int j;
  oggpack_write(opb,info->order,8);
  oggpack_write(opb,info->rate,16);
  oggpack_write(opb,info->barkmap,16);
  oggpack_write(opb,info->ampbits,6);
  oggpack_write(opb,info->ampdB,8);
  oggpack_write(opb,info->numbooks-1,4);
  for(j=0;j<info->numbooks;j++)
    oggpack_write(opb,info->books[j],8);
}

/* floor0_unpack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556650
static vorbis_info_floor *floor0_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  int j;

  vorbis_info_floor0 *info=(vorbis_info_floor0 *)_ogg_malloc(sizeof(*info));
  info->order=oggpack_read(opb,8);
  info->rate=oggpack_read(opb,16);
  info->barkmap=oggpack_read(opb,16);
  info->ampbits=oggpack_read(opb,6);
  info->ampdB=oggpack_read(opb,8);
  info->numbooks=oggpack_read(opb,4)+1;

  if(info->order<1)goto err_out;
  if(info->rate<1)goto err_out;
  if(info->barkmap<1)goto err_out;
  if(info->numbooks<1)goto err_out;

  for(j=0;j<info->numbooks;j++){
    info->books[j]=oggpack_read(opb,8);
    if(info->books[j]<0 || info->books[j]>=ci->books)goto err_out;
  }
  return(info);

 err_out:
  floor0_free_info(info);
  return(NULL);
}

/* floor0_look. STATE: MATCH 100.0, 209/209 (rc3's rate/2.f under the band's noieeefp).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005567b0
static vorbis_look_floor *floor0_look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *i){
  int j;
  float scale;
  vorbis_info        *vi=vd->vi;
  codec_setup_info   *ci=(codec_setup_info *)vi->codec_setup;
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)_ogg_calloc(1,sizeof(*look));
  look->m=info->order;
  look->n=ci->blocksizes[mi->blockflag]/2;
  look->ln=info->barkmap;
  look->vi=info;

  if(vd->analysisp)
    lpc_init(&look->lpclook,look->ln,look->m);

  scale=look->ln/toBARK(info->rate/2.f);

  look->linearmap=(int *)_ogg_malloc((look->n+1)*sizeof(*look->linearmap));
  for(j=0;j<look->n;j++){
    int val=floor( toBARK((info->rate/2.f)/look->n*j)
		   *scale);
    if(val>=look->ln)val=look->ln;
    look->linearmap[j]=val;
  }
  look->linearmap[j]=-1;

  look->lsp_look=(float *)_ogg_malloc(look->ln*sizeof(*look->lsp_look));
  for(j=0;j<look->ln;j++)
    look->lsp_look[j]=2*cos(M_PI/look->ln*j);

  return look;
}

/* _curve_to_lpc. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556a20
float _curve_to_lpc(float *curve,float *lpc,
		     vorbis_look_floor0 *l){
  int mapped=l->ln;
  float *work=(float *)alloca(sizeof(*work)*mapped);
  int i,j,last=0;
  int bark=0;
  static int seq=0;

  FastMemSet(work,0,sizeof(*work)*mapped);

  for(i=0;i<l->n;i++){
    bark=l->linearmap[i];
    if(work[bark]<curve[i])work[bark]=curve[i];
    if(bark>last+1){
      long span=bark-last;
      for(j=1;j<span;j++){
	float del=(float)j/span;
	work[j+last]=work[bark]*del+work[last]*(1.f-del);
      }
    }
    last=bark;
  }

  for(i=bark+1;i<mapped;i++)
    work[i]=work[i-1];

  for(i=0;i<l->n;i++)
    curve[i]-=150;

  _analysis_output("barkfloor",seq,work,bark,0,0);
  _analysis_output("barkcurve",seq++,curve,l->n,1,0);

  for(i=0;i<l->n;i++)
    curve[i]+=150;

  return vorbis_lpc_from_curve(work,lpc,&(l->lpclook));
}

/* floor0_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00556ec0
static int floor0_forward(vorbis_block *vb,vorbis_look_floor *in,
			  float *mdct, const float *logmdct,
			  const float *logmask, const float *logmax,
			  float *codedflr){
  long j;
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)in;
  vorbis_info_floor0 *info=look->vi;
  float amp;
  long val=0;
  static int seq=0;

  seq++;

  for(j=0;j<look->n;j++)
    codedflr[j]=logmask[j]+info->ampdB;

  amp=sqrt(_curve_to_lpc(codedflr,codedflr,look));

  {
    long maxval=(1L<<info->ampbits)-1;

    val=rint(amp/info->ampdB*maxval);

    if(val<0)val=0;
    if(val>maxval)val=maxval;

    if(val>0)
      amp=(float)val/maxval*info->ampdB;
    else
      amp=0;
  }

  if(val){
    _analysis_output("lpc",seq-1,codedflr,look->m,0,0);
    if(vorbis_lpc_to_lsp(codedflr,codedflr,look->m))
      val=0;
  }

  oggpack_write(&vb->opb,val,info->ampbits);
  look->bits+=info->ampbits+1;
  look->frames++;

  if(val){
    float *lspwork=(float *)alloca(look->m*sizeof(*lspwork));

    backend_lookup_state *be=(backend_lookup_state *)vb->vd->backend_state;
    codebook *b;
    int booknum;

    _analysis_output("lsp",seq-1,codedflr,look->m,0,0);

    if(info->numbooks>1){
      float last=0.;
      for(j=0;j<look->m;j++){
	float val=codedflr[j]-last;
	if(val<info->lessthan || val>info->greaterthan)break;
	last=codedflr[j];
      }
      if(j<look->m)
	booknum=0;
      else
	booknum=1;
    }else
      booknum=0;

    b=be->fullbooks+info->books[booknum];
    oggpack_write(&vb->opb,booknum,_ilog(info->numbooks));
    look->bits+=_ilog(info->numbooks);

    for(j=0;j<look->m;j+=b->dim){
      int entry=_f0_fit(b,codedflr,lspwork,j);
      look->bits+=vorbis_book_encode(b,entry,&vb->opb);
    }

    _analysis_output("lsp2",seq-1,lspwork,look->m,0,0);

    for(j=0;j<look->n;j++)
      codedflr[j]=1.f;
    vorbis_lsp_to_curve(codedflr,look->linearmap,look->n,look->ln,
			lspwork,look->m,amp,(float)info->ampdB);

    _analysis_output("barklsp",seq-1,codedflr,look->n,1,1);
    _analysis_output("lsp3",seq-1,codedflr,look->n,0,1);

    return(val);
  }

  FastMemSet(codedflr,0,sizeof(*codedflr)*look->n);
  FastMemSet(mdct,0,sizeof(*mdct)*look->n);
  return(val);
}

/* floor0_inverse1. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557400
static void *floor0_inverse1(vorbis_block *vb,vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  int j,k;

  int ampraw=oggpack_read(&vb->opb,info->ampbits);
  if(ampraw>0){
    long maxval=(1<<info->ampbits)-1;
    float amp=(float)ampraw/maxval*info->ampdB;
    int booknum=oggpack_read(&vb->opb,_ilog(info->numbooks));

    if(booknum!=-1 && booknum<info->numbooks){
      backend_lookup_state *be=(backend_lookup_state *)vb->vd->backend_state;
      codebook *b=be->fullbooks+info->books[booknum];
      float last=0.f;
      float *lsp=(float *)_vorbis_block_alloc(vb,sizeof(*lsp)*(look->m+1));

      for(j=0;j<look->m;j+=b->dim)
	if(vorbis_book_decodev_set(b,lsp+j,&vb->opb,b->dim)==-1)goto eop;
      for(j=0;j<look->m;){
	for(k=0;k<b->dim;k++,j++)lsp[j]+=last;
	last=lsp[j-1];
      }

      lsp[look->m]=amp;
      return(lsp);
    }
  }
 eop:
  return(NULL);
}

/* floor0_inverse2. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557590
static int floor0_inverse2(vorbis_block *vb,vorbis_look_floor *i,
			   void *memo,float *out){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;

  if(memo){
    float *lsp=(float *)memo;
    float amp=lsp[look->m];

    vorbis_lsp_to_curve(out,look->linearmap,look->n,look->ln,
			lsp,look->m,amp,(float)info->ampdB);
    return(1);
  }
  FastMemSet(out,0,sizeof(*out)*look->n);
  return(0);
}

vorbis_func_floor floor0_exportbundle={
  &floor0_pack,&floor0_unpack,&floor0_look,&floor0_copy_info,&floor0_free_info,
  &floor0_free_look,&floor0_forward,&floor0_inverse1,&floor0_inverse2
};
