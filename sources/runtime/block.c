// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto or -align 4 loses MATCHes here; inlined: ilog2); Research/notes/burgervorbis.md
/* block.c -- libvorbis 1.0rc3 lib/block.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (FastMemSet); altered: comments and the functions the exe does not link (the encoder's analysis front end) removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"

#include "window.h"
#include "mdct.h"
#include "lpc.h"
#include "registry.h"
#include "misc.h"

static int ilog2(unsigned int v){
  int ret=0;
  if(v>1){
    do{
      ret++;
      v>>=1;
    }while(v>1);
  }
  return(ret);
}

#ifndef WORD_ALIGN
#define WORD_ALIGN 8
#endif

/* vorbis_block_init. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: -proc pentium4 MATCH (pending the pin). */
// FUNCTION: EVNOVA 0x00531c90
int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  FastMemSet(vb,0,sizeof(*vb));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;
  if(v->analysisp){
    vorbis_block_internal *vbi=(vorbis_block_internal *)_ogg_calloc(1,sizeof(vorbis_block_internal));
    vb->internal=vbi;
    oggpack_writeinit(&vb->opb);
    vbi->ampmax=-9999;
    vbi->packet_markers=(ogg_uint32_t *)_ogg_malloc(vorbis_bitrate_maxmarkers()*
			       sizeof(*vbi->packet_markers));
  }

  return(0);
}

/* _vorbis_block_alloc. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00531d20
void *_vorbis_block_alloc(vorbis_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    if(vb->localstore){
      struct alloc_chain *link=(struct alloc_chain *)_ogg_malloc(sizeof(*link));
      vb->totaluse+=vb->localtop;
      link->next=vb->reap;
      link->ptr=vb->localstore;
      vb->reap=link;
    }

    vb->localalloc=bytes;
    vb->localstore=_ogg_malloc(vb->localalloc);
    vb->localtop=0;
  }
  {
    void *ret=(void *)(((char *)vb->localstore)+vb->localtop);
    vb->localtop+=bytes;
    return ret;
  }
}

/* _vorbis_block_ripcord. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00531da0
void _vorbis_block_ripcord(vorbis_block *vb){
  struct alloc_chain *reap=vb->reap;
  while(reap){
    struct alloc_chain *next=reap->next;
    _ogg_free(reap->ptr);
    FastMemSet(reap,0,sizeof(*reap));
    _ogg_free(reap);
    reap=next;
  }

  if(vb->totaluse){
    vb->localstore=_ogg_realloc(vb->localstore,vb->totaluse+vb->localalloc);
    vb->localalloc+=vb->totaluse;
    vb->totaluse=0;
  }

  vb->localtop=0;
  vb->reap=NULL;
}

/* vorbis_block_clear. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00531e40
int vorbis_block_clear(vorbis_block *vb){
  if(vb->vd)
    if(vb->vd->analysisp)
      oggpack_writeclear(&vb->opb);
  _vorbis_block_ripcord(vb);
  if(vb->localstore)_ogg_free(vb->localstore);

  if(vb->internal){
    vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;
    if(vbi->packet_markers)_ogg_free(vbi->packet_markers);

    _ogg_free(vb->internal);
  }

  FastMemSet(vb,0,sizeof(*vb));
  return(0);
}

/* _vds_shared_init. STATE: MATCH, byte-identical (1104 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00531f70
static int _vds_shared_init(vorbis_dsp_state *v,vorbis_info *vi,int encp){
  int i;
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;
  backend_lookup_state *b=NULL;

  FastMemSet(v,0,sizeof(*v));
  v->backend_state=b=(backend_lookup_state *)_ogg_calloc(1,sizeof(*b));

  v->vi=vi;
  b->modebits=ilog2(ci->modes);

  b->transform[0]=(vorbis_look_transform **)_ogg_calloc(VI_TRANSFORMB,sizeof(*b->transform[0]));
  b->transform[1]=(vorbis_look_transform **)_ogg_calloc(VI_TRANSFORMB,sizeof(*b->transform[1]));

  b->transform[0][0]=_ogg_calloc(1,sizeof(mdct_lookup));
  b->transform[1][0]=_ogg_calloc(1,sizeof(mdct_lookup));
  mdct_init((mdct_lookup *)b->transform[0][0],ci->blocksizes[0]);
  mdct_init((mdct_lookup *)b->transform[1][0],ci->blocksizes[1]);

  b->window[0][0][0]=(float **)_ogg_calloc(VI_WINDOWB,sizeof(*b->window[0][0][0]));
  b->window[0][0][1]=b->window[0][0][0];
  b->window[0][1][0]=b->window[0][0][0];
  b->window[0][1][1]=b->window[0][0][0];
  b->window[1][0][0]=(float **)_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][0][0]));
  b->window[1][0][1]=(float **)_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][0][1]));
  b->window[1][1][0]=(float **)_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][1][0]));
  b->window[1][1][1]=(float **)_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][1][1]));

  for(i=0;i<VI_WINDOWB;i++){
    b->window[0][0][0][i]=
      _vorbis_window(i,ci->blocksizes[0],ci->blocksizes[0]/2,ci->blocksizes[0]/2);
    b->window[1][0][0][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[0]/2,ci->blocksizes[0]/2);
    b->window[1][0][1][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[0]/2,ci->blocksizes[1]/2);
    b->window[1][1][0][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[1]/2,ci->blocksizes[0]/2);
    b->window[1][1][1][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[1]/2,ci->blocksizes[1]/2);
  }

  if(encp){
    b->fullbooks=(codebook *)_ogg_calloc(ci->books,sizeof(*b->fullbooks));
    for(i=0;i<ci->books;i++)
      vorbis_book_init_encode(b->fullbooks+i,ci->book_param[i]);
    v->analysisp=1;
  }else{
    b->fullbooks=(codebook *)_ogg_calloc(ci->books,sizeof(*b->fullbooks));
    for(i=0;i<ci->books;i++)
      vorbis_book_init_decode(b->fullbooks+i,ci->book_param[i]);
  }

  v->pcm_storage=8192;

  v->pcm=(float **)_ogg_malloc(vi->channels*sizeof(*v->pcm));
  v->pcmret=(float **)_ogg_malloc(vi->channels*sizeof(*v->pcmret));
  {
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=(float *)_ogg_calloc(v->pcm_storage,sizeof(*v->pcm[i]));
  }

  v->lW=0;
  v->W=0;

  v->centerW=ci->blocksizes[1]/2;

  v->pcm_current=v->centerW;

  b->mode=(vorbis_look_mapping **)_ogg_calloc(ci->modes,sizeof(*b->mode));
  for(i=0;i<ci->modes;i++){
    int mapnum=ci->mode_param[i]->mapping;
    int maptype=ci->map_type[mapnum];
    b->mode[i]=_mapping_P[maptype]->look(v,ci->mode_param[i],
					 ci->map_param[mapnum]);
  }

  return(0);
}

/* vorbis_dsp_clear. STATE: MATCH, byte-identical (819 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005323c0
void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i,j,k;
  if(v){
    vorbis_info *vi=v->vi;
    codec_setup_info *ci=vi?(codec_setup_info *)vi->codec_setup:NULL;
    backend_lookup_state *b=(backend_lookup_state *)v->backend_state;

    if(b){
      if(b->window[0][0][0]){
	for(i=0;i<VI_WINDOWB;i++)
	  if(b->window[0][0][0][i])_ogg_free(b->window[0][0][0][i]);
	_ogg_free(b->window[0][0][0]);

	for(j=0;j<2;j++)
	  for(k=0;k<2;k++){
	    for(i=0;i<VI_WINDOWB;i++)
	      if(b->window[1][j][k][i])_ogg_free(b->window[1][j][k][i]);
	    _ogg_free(b->window[1][j][k]);
	  }
      }

      if(b->ve){
	_ve_envelope_clear(b->ve);
	_ogg_free(b->ve);
      }

      if(b->transform[0]){
	mdct_clear((mdct_lookup *)b->transform[0][0]);
	_ogg_free(b->transform[0][0]);
	_ogg_free(b->transform[0]);
      }
      if(b->transform[1]){
	mdct_clear((mdct_lookup *)b->transform[1][0]);
	_ogg_free(b->transform[1][0]);
	_ogg_free(b->transform[1]);
      }
      if(b->psy_g_look)_vp_global_free(b->psy_g_look);
      vorbis_bitrate_clear(&b->bms);
    }

    if(v->pcm){
      for(i=0;i<vi->channels;i++)
	if(v->pcm[i])_ogg_free(v->pcm[i]);
      _ogg_free(v->pcm);
      if(v->pcmret)_ogg_free(v->pcmret);
    }

    if(ci){
      for(i=0;i<ci->modes;i++){
	int mapnum=ci->mode_param[i]->mapping;
	int maptype=ci->map_type[mapnum];
	if(b && b->mode)_mapping_P[maptype]->free_look(b->mode[i]);
      }

      for(i=0;i<ci->books;i++)
	if(b && b->fullbooks)vorbis_book_clear(b->fullbooks+i);
    }

    if(b){
      if(b->mode)_ogg_free(b->mode);
      if(b->fullbooks)_ogg_free(b->fullbooks);

      if(b->header)_ogg_free(b->header);
      if(b->header1)_ogg_free(b->header1);
      if(b->header2)_ogg_free(b->header2);
      _ogg_free(b);
    }

    FastMemSet(v,0,sizeof(*v));
  }
}

/* vorbis_synthesis_init. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532700
int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_shared_init(v,vi,0);

  v->pcm_returned=-1;
  v->granulepos=-1;
  v->sequence=-1;

  return(0);
}

/* vorbis_synthesis_blockin. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532750
int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;

  if(v->centerW>ci->blocksizes[1]/2 &&
     v->pcm_returned>8192){

    int shiftPCM=v->centerW-ci->blocksizes[1]/2;
    shiftPCM=(v->pcm_returned<shiftPCM?v->pcm_returned:shiftPCM);

    v->pcm_current-=shiftPCM;
    v->centerW-=shiftPCM;
    v->pcm_returned-=shiftPCM;

    if(shiftPCM){
      int i;
      for(i=0;i<vi->channels;i++)
	memmove(v->pcm[i],v->pcm[i]+shiftPCM,
		v->pcm_current*sizeof(*v->pcm[i]));
    }
  }

  v->lW=v->W;
  v->W=vb->W;
  v->nW=-1;

  v->glue_bits+=vb->glue_bits;
  v->time_bits+=vb->time_bits;
  v->floor_bits+=vb->floor_bits;
  v->res_bits+=vb->res_bits;

  if(v->sequence+1 != vb->sequence)v->granulepos=-1;

  v->sequence=vb->sequence;

  {
    int sizeW=ci->blocksizes[v->W];
    int centerW=v->centerW+ci->blocksizes[v->lW]/4+sizeW/4;
    int beginW=centerW-sizeW/2;
    int endW=beginW+sizeW;
    int beginSl;
    int endSl;
    int i,j;

    if(endW>v->pcm_storage){
      v->pcm_storage=endW+ci->blocksizes[1];

      for(i=0;i<vi->channels;i++)
	v->pcm[i]=(float *)_ogg_realloc(v->pcm[i],v->pcm_storage*sizeof(*v->pcm[i]));
    }

    switch(v->W){
    case 0:
      beginSl=0;
      endSl=ci->blocksizes[0]/2;
      break;
    case 1:
      beginSl=ci->blocksizes[1]/4-ci->blocksizes[v->lW]/4;
      endSl=beginSl+ci->blocksizes[v->lW]/2;
      break;
    default:
      return(-1);
    }

    for(j=0;j<vi->channels;j++){
      float *pcm=v->pcm[j]+beginW;
      float *p=vb->pcm[j];

      for(i=beginSl;i<endSl;i++)
	pcm[i]+=p[i];

      for(;i<sizeW;i++)
	pcm[i]=p[i];
    }

    if(v->pcm_returned==-1)
      v->pcm_returned=centerW;

    if(v->granulepos==-1)
      if(vb->granulepos==-1){
	v->granulepos=0;
      }else{
	v->granulepos=vb->granulepos;
      }
    else{
      v->granulepos+=(centerW-v->centerW);
      if(vb->granulepos!=-1 && v->granulepos!=vb->granulepos){

	if(v->granulepos>vb->granulepos){
	  long extra=v->granulepos-vb->granulepos;

	  if(vb->eofflag){
	    centerW-=extra;
	  }else if(vb->sequence == 1){
	    v->pcm_returned+=extra;
	    if(v->pcm_returned>centerW)v->pcm_returned=centerW;
	  }
	}

	v->granulepos=vb->granulepos;
      }
    }

    v->centerW=centerW;
    v->pcm_current=endW;

    if(vb->eofflag)v->eofflag=1;
  }

  return(0);
}

/* vorbis_synthesis_pcmout. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532c60
int vorbis_synthesis_pcmout(vorbis_dsp_state *v,float ***pcm){
  vorbis_info *vi=v->vi;
  if(v->pcm_returned>-1 && v->pcm_returned<v->centerW){
    if(pcm){
      int i;
      for(i=0;i<vi->channels;i++)
	v->pcmret[i]=v->pcm[i]+v->pcm_returned;
      *pcm=v->pcmret;
    }
    return(v->centerW-v->pcm_returned);
  }
  return(0);
}

/* vorbis_synthesis_read. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532ce0
int vorbis_synthesis_read(vorbis_dsp_state *v,int bytes){
  if(bytes && v->pcm_returned+bytes>v->centerW)return(OV_EINVAL);
  v->pcm_returned+=bytes;
  return(0);
}
