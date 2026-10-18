// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto or -align 4 loses MATCHes here; inlined: ilog2); Research/notes/burgervorbis.md
/* mapping0.c -- libvorbis 1.0rc3 lib/mapping0.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (oggpack_read1 for 1-bit reads, FastMemSet); altered: comments removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "codebook.h"
#include "registry.h"
#include "psy.h"
#include "misc.h"

extern int analysis_noisy;

typedef struct vorbis_look_mapping{
  drft_lookup fft_look;
  vorbis_info_mode *mode;
  vorbis_info_mapping0 *map;

  vorbis_look_time **time_look;
  vorbis_look_floor **floor_look;

  vorbis_look_residue **residue_look;
  vorbis_look_psy *psy_look[2];

  vorbis_func_time **time_func;
  vorbis_func_floor **floor_func;
  vorbis_func_residue **residue_func;

  int ch;
  long lastframe;

} vorbis_look_mapping0;

/* mapping0_copy_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559d90
static vorbis_info_mapping *mapping0_copy_info(vorbis_info_mapping *vm){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;
  vorbis_info_mapping0 *ret=(vorbis_info_mapping0 *)_ogg_malloc(sizeof(*ret));
  memcpy(ret,info,sizeof(*ret));
  return(ret);
}

/* mapping0_free_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559de0
static void mapping0_free_info(vorbis_info_mapping *i){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)i;
  if(info){
    FastMemSet(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

/* mapping0_free_look. STATE: MATCH, byte-identical (319 B); BurgerLib uses the
   parameter itself, no cast copy. OPEN: none. */
// FUNCTION: EVNOVA 0x00559e20
static void mapping0_free_look(vorbis_look_mapping *l){
  int i;
  if(l){
    drft_clear(&l->fft_look);

    for(i=0;i<l->map->submaps;i++){
      l->time_func[i]->free_look(l->time_look[i]);
      l->floor_func[i]->free_look(l->floor_look[i]);
      l->residue_func[i]->free_look(l->residue_look[i]);
    }
    if(l->psy_look[1] && l->psy_look[1]!=l->psy_look[0]){
      _vp_psy_clear(l->psy_look[1]);
      _ogg_free(l->psy_look[1]);
    }
    if(l->psy_look[0]){
      _vp_psy_clear(l->psy_look[0]);
      _ogg_free(l->psy_look[0]);
    }
    _ogg_free(l->time_func);
    _ogg_free(l->floor_func);
    _ogg_free(l->residue_func);
    _ogg_free(l->time_look);
    _ogg_free(l->floor_look);
    _ogg_free(l->residue_look);
    FastMemSet(l,0,sizeof(*l));
    _ogg_free(l);
  }
}

/* mapping0_look. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559f60
static vorbis_look_mapping *mapping0_look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_mapping *m){
  int i;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)_ogg_calloc(1,sizeof(*look));
  vorbis_info_mapping0 *info=look->map=(vorbis_info_mapping0 *)m;
  look->mode=vm;

  look->time_look=(vorbis_look_time **)_ogg_calloc(info->submaps,sizeof(*look->time_look));
  look->floor_look=(vorbis_look_floor **)_ogg_calloc(info->submaps,sizeof(*look->floor_look));

  look->residue_look=(vorbis_look_residue **)_ogg_calloc(info->submaps,sizeof(*look->residue_look));

  look->time_func=(vorbis_func_time **)_ogg_calloc(info->submaps,sizeof(*look->time_func));
  look->floor_func=(vorbis_func_floor **)_ogg_calloc(info->submaps,sizeof(*look->floor_func));
  look->residue_func=(vorbis_func_residue **)_ogg_calloc(info->submaps,sizeof(*look->residue_func));

  for(i=0;i<info->submaps;i++){
    int timenum=info->timesubmap[i];
    int floornum=info->floorsubmap[i];
    int resnum=info->residuesubmap[i];

    look->time_func[i]=_time_P[ci->time_type[timenum]];
    look->time_look[i]=look->time_func[i]->
      look(vd,vm,ci->time_param[timenum]);
    look->floor_func[i]=_floor_P[ci->floor_type[floornum]];
    look->floor_look[i]=look->floor_func[i]->
      look(vd,vm,ci->floor_param[floornum]);
    look->residue_func[i]=_residue_P[ci->residue_type[resnum]];
    look->residue_look[i]=look->residue_func[i]->
      look(vd,vm,ci->residue_param[resnum]);
  }
  if(ci->psys && vd->analysisp){
    if(info->psy[0] != info->psy[1]){

      int psynum=info->psy[0];
      look->psy_look[0]=(vorbis_look_psy *)_ogg_calloc(1,sizeof(*look->psy_look[0]));
      _vp_psy_init(look->psy_look[0],ci->psy_param[psynum],
		   &ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);

      psynum=info->psy[1];
      look->psy_look[1]=(vorbis_look_psy *)_ogg_calloc(1,sizeof(*look->psy_look[1]));
      _vp_psy_init(look->psy_look[1],ci->psy_param[psynum],
		   &ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);
    }else{

      int psynum=info->psy[0];
      look->psy_look[0]=(vorbis_look_psy *)_ogg_calloc(1,sizeof(*look->psy_look[0]));
      look->psy_look[1]=look->psy_look[0];
      _vp_psy_init(look->psy_look[0],ci->psy_param[psynum],
		   &ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);
    }
  }

  look->ch=vi->channels;

  if(vd->analysisp)drft_init(&look->fft_look,ci->blocksizes[vm->blockflag]);
  return(look);
}

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* mapping0_pack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055a2d0
static void mapping0_pack(vorbis_info *vi,vorbis_info_mapping *vm,
			  oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;

  if(info->submaps>1){
    oggpack_write(opb,1,1);
    oggpack_write(opb,info->submaps-1,4);
  }else
    oggpack_write(opb,0,1);

  if(info->coupling_steps>0){
    oggpack_write(opb,1,1);
    oggpack_write(opb,info->coupling_steps-1,8);

    for(i=0;i<info->coupling_steps;i++){
      oggpack_write(opb,info->coupling_mag[i],ilog2(vi->channels));
      oggpack_write(opb,info->coupling_ang[i],ilog2(vi->channels));
    }
  }else
    oggpack_write(opb,0,1);

  oggpack_write(opb,0,2);

  if(info->submaps>1){
    for(i=0;i<vi->channels;i++)
      oggpack_write(opb,info->chmuxlist[i],4);
  }
  for(i=0;i<info->submaps;i++){
    oggpack_write(opb,info->timesubmap[i],8);
    oggpack_write(opb,info->floorsubmap[i],8);
    oggpack_write(opb,info->residuesubmap[i],8);
  }
}

/* mapping0_unpack. STATE: BurgerLib's libvorbis 1.0rc3, MATCH.
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055a550
static vorbis_info_mapping *mapping0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)_ogg_calloc(1,sizeof(*info));
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  FastMemSet(info,0,sizeof(*info));

  if(oggpack_read1(opb))
    info->submaps=oggpack_read(opb,4)+1;
  else
    info->submaps=1;

  if(oggpack_read1(opb)){
    info->coupling_steps=oggpack_read(opb,8)+1;

    for(i=0;i<info->coupling_steps;i++){
      int testM=info->coupling_mag[i]=oggpack_read(opb,ilog2(vi->channels));
      int testA=info->coupling_ang[i]=oggpack_read(opb,ilog2(vi->channels));

      if(testM<0 ||
	 testA<0 ||
	 testM==testA ||
	 testM>=vi->channels ||
	 testA>=vi->channels) goto err_out;
    }
  }

  if(oggpack_read(opb,2)!=0)goto err_out;

  if(info->submaps>1){
    for(i=0;i<vi->channels;i++){
      info->chmuxlist[i]=oggpack_read(opb,4);
      if(info->chmuxlist[i]>=info->submaps)goto err_out;
    }
  }
  for(i=0;i<info->submaps;i++){
    info->timesubmap[i]=oggpack_read(opb,8);
    if(info->timesubmap[i]>=ci->times)goto err_out;
    info->floorsubmap[i]=oggpack_read(opb,8);
    if(info->floorsubmap[i]>=ci->floors)goto err_out;
    info->residuesubmap[i]=oggpack_read(opb,8);
    if(info->residuesubmap[i]>=ci->residues)goto err_out;
  }

  return info;

 err_out:
  mapping0_free_info(info);
  return(NULL);
}

#include "os.h"
#include "lpc.h"
#include "lsp.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"
#include "scales.h"

static long seq=0;
/* mapping0_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055a860
static int mapping0_forward(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state      *vd=vb->vd;
  vorbis_info           *vi=vd->vi;
  codec_setup_info      *ci=(codec_setup_info *)vi->codec_setup;
  backend_lookup_state  *b=(backend_lookup_state *)vb->vd->backend_state;
  bitrate_manager_state *bm=&b->bms;
  vorbis_look_mapping0  *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0  *info=look->map;
  vorbis_info_mode      *mode=look->mode;
  vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;
  int                    n=vb->pcmend;
  int i,j;
  float *window=b->window[vb->W][vb->lW][vb->nW][mode->windowtype];
  int   *nonzero=(int *)alloca(sizeof(*nonzero)*vi->channels);

  float *work=(float *)_vorbis_block_alloc(vb,n*sizeof(*work));

  float global_ampmax=vbi->ampmax;
  float *local_ampmax=(float *)alloca(sizeof(*local_ampmax)*vi->channels);
  int blocktype=vbi->blocktype;

  for(i=0;i<vi->channels;i++){
    float scale=4.f/n;
    float scale_dB;

    float *pcm     =vb->pcm[i];
    float *fft     =work;
    float *logfft  =pcm+n/2;

    scale_dB=todB(&scale);
    _analysis_output("pcm",seq+i,pcm,n,0,0);

    for(j=0;j<n;j++)
      fft[j]=pcm[j]*=window[j];

    mdct_forward((mdct_lookup *)b->transform[vb->W][0],pcm,pcm);

    drft_forward(&look->fft_look,fft);
    fft[0]*=scale;
    logfft[0]=todB(fft);
    local_ampmax[i]=logfft[0];
    for(j=1;j<n-1;j+=2){
      float temp=fft[j]*fft[j]+fft[j+1]*fft[j+1];
      temp=logfft[(j+1)>>1]=scale_dB+.5f*todB(&temp);
      if(temp>local_ampmax[i])local_ampmax[i]=temp;
    }

    if(local_ampmax[i]>0.f)local_ampmax[i]=0.f;
    if(local_ampmax[i]>global_ampmax)global_ampmax=local_ampmax[i];

    _analysis_output("fft",seq+i,logfft,n/2,1,0);
  }

  for(i=0;i<vi->channels;i++){
    int submap=info->chmuxlist[i];

    float *mdct    =vb->pcm[i];
    float *res     =mdct;
    float *codedflr=mdct+n/2;
    float *logfft  =mdct+n/2;

    float *logmdct =work;
    float *logmax  =mdct+n/2;
    float *logmask =work+n/2;

    for(j=0;j<n/2;j++)
      logmdct[j]=todB(mdct+j);
    _analysis_output("mdct",seq+i,logmdct,n/2,1,0);

    _vp_compute_mask(look->psy_look[blocktype],
		     logfft,
		     logmdct,
		     logmask,
		     global_ampmax,
		     local_ampmax[i],
		     bm->avgnoise);

    _analysis_output("mask",seq+i,logmask,n/2,1,0);

    nonzero[i]=look->floor_func[submap]->
      forward(vb,look->floor_look[submap],
	      mdct,
	      logmdct,
	      logmask,
	      logmax,

	      codedflr);

    _vp_remove_floor(look->psy_look[blocktype],
		     mdct,
		     codedflr,
		     res);

    _analysis_output("codedflr",seq+i,codedflr,n/2,1,1);
  }

  vbi->ampmax=global_ampmax;

  {
    float  **pcm=vb->pcm;
    float  **quantized=(float **)alloca(sizeof(*quantized)*vi->channels);
    float  **sofar=(float **)alloca(sizeof(*sofar)*vi->channels);

    long  ***classifications=(long ***)alloca(sizeof(*classifications)*info->submaps);
    float ***qbundle=(float ***)alloca(sizeof(*qbundle)*info->submaps);
    float ***pcmbundle=(float ***)alloca(sizeof(*pcmbundle)*info->submaps);
    float ***sobundle=(float ***)alloca(sizeof(*sobundle)*info->submaps);
    int    **zerobundle=(int **)alloca(sizeof(*zerobundle)*info->submaps);
    int     *chbundle=(int *)alloca(sizeof(*chbundle)*info->submaps);
    int      chcounter=0;

    int   quant_passes=ci->coupling_passes;

    for(i=0;i<vi->channels;i++){
      quantized[i]=(float *)_vorbis_block_alloc(vb,n*sizeof(*sofar[i]));
      sofar[i]=quantized[i]+n/2;
      FastMemSet(sofar[i],0,sizeof(*sofar[i])*n/2);
    }

    qbundle[0]=(float **)alloca(sizeof(*qbundle[0])*vi->channels);
    pcmbundle[0]=(float **)alloca(sizeof(*pcmbundle[0])*vi->channels);
    sobundle[0]=(float **)alloca(sizeof(*sobundle[0])*vi->channels);
    zerobundle[0]=(int *)alloca(sizeof(*zerobundle[0])*vi->channels);

    if(info->coupling_steps==0){
      for(i=0;i<vi->channels;i++){
	float *lpcm=pcm[i];
	float *lqua=quantized[i];
	for(j=0;j<n/2;j++)
	  lqua[j]=lpcm[j];
      }
    }else{
      _vp_quantize_couple(look->psy_look[blocktype],
			  info,
			  pcm,
			  sofar,
			  quantized,
			  nonzero,
			  0);
    }

    for(i=0;i<vi->channels;i++)
      _analysis_output("quant",seq+i,quantized[i],n/2,1,0);

    for(i=0;i<info->submaps;i++){
      int ch_in_bundle=0;
      qbundle[i]=qbundle[0]+chcounter;
      sobundle[i]=sobundle[0]+chcounter;
      zerobundle[i]=zerobundle[0]+chcounter;

      for(j=0;j<vi->channels;j++){
	if(info->chmuxlist[j]==i){
	  if(nonzero[j])
	    zerobundle[i][ch_in_bundle]=1;
	  else
	    zerobundle[i][ch_in_bundle]=0;
	  qbundle[i][ch_in_bundle]=quantized[j];
	  pcmbundle[i][ch_in_bundle]=pcm[j];
	  sobundle[i][ch_in_bundle++]=sofar[j];
	}
      }
      chbundle[i]=ch_in_bundle;
      chcounter+=ch_in_bundle;

      classifications[i]=look->residue_func[i]->
	class_(vb,look->residue_look[i],pcmbundle[i],zerobundle[i],chbundle[i]);
    }

    for(i=0;i<quant_passes;){

      for(j=0;j<info->submaps;j++){
	look->residue_func[j]->
	  forward(vb,look->residue_look[j],
		  qbundle[j],sobundle[j],zerobundle[j],chbundle[j],
		  i,classifications[j],vbi->packet_markers);
      }
      i++;

      if(i<quant_passes){
	if(info->coupling_steps==0){
	  int k;
	  for(k=0;k<vi->channels;k++){
	    float *lpcm=pcm[k];
	    float *lsof=sofar[k];
	    float *lqua=quantized[k];
	    for(j=0;j<n/2;j++)
	      lqua[j]=lpcm[j]-lsof[j];
	  }
	}else{
	  _vp_quantize_couple(look->psy_look[blocktype],
			      info,
			      pcm,
			      sofar,
			      quantized,
			      nonzero,
			      i);
	}
      }
    }
    seq+=vi->channels;
  }

  look->lastframe=vb->sequence;
  return(0);
}

/* mapping0_inverse. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055b700
static int mapping0_inverse(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  backend_lookup_state *b=(backend_lookup_state *)vd->backend_state;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   i,j;
  long                  n=vb->pcmend=ci->blocksizes[vb->W];

  float *window=b->window[vb->W][vb->lW][vb->nW][mode->windowtype];
  float **pcmbundle=(float **)alloca(sizeof(*pcmbundle)*vi->channels);
  int    *zerobundle=(int *)alloca(sizeof(*zerobundle)*vi->channels);

  int   *nonzero  =(int *)alloca(sizeof(*nonzero)*vi->channels);
  void **floormemo=(void **)alloca(sizeof(*floormemo)*vi->channels);

  for(i=0;i<vi->channels;i++){
    int submap=info->chmuxlist[i];
    floormemo[i]=look->floor_func[submap]->
      inverse1(vb,look->floor_look[submap]);
    if(floormemo[i])
      nonzero[i]=1;
    else
      nonzero[i]=0;
    FastMemSet(vb->pcm[i],0,sizeof(*vb->pcm[i])*n/2);
  }

  for(i=0;i<info->coupling_steps;i++){
    if(nonzero[info->coupling_mag[i]] ||
       nonzero[info->coupling_ang[i]]){
      nonzero[info->coupling_mag[i]]=1;
      nonzero[info->coupling_ang[i]]=1;
    }
  }

  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(info->chmuxlist[j]==i){
	if(nonzero[j])
	  zerobundle[ch_in_bundle]=1;
	else
	  zerobundle[ch_in_bundle]=0;
	pcmbundle[ch_in_bundle++]=vb->pcm[j];
      }
    }

    look->residue_func[i]->inverse(vb,look->residue_look[i],
				   pcmbundle,zerobundle,ch_in_bundle);
  }

  for(i=info->coupling_steps-1;i>=0;i--){
    float *pcmM=vb->pcm[info->coupling_mag[i]];
    float *pcmA=vb->pcm[info->coupling_ang[i]];

    for(j=0;j<n/2;j++){
      float mag=pcmM[j];
      float ang=pcmA[j];

      if(mag>0)
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag-ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag+ang;
	}
      else
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag+ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag-ang;
	}
    }
  }

  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    int submap=info->chmuxlist[i];
    look->floor_func[submap]->
      inverse2(vb,look->floor_look[submap],floormemo[i],pcm);
  }

  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    mdct_backward((mdct_lookup *)b->transform[vb->W][0],pcm,pcm);
  }

  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    if(nonzero[i])
      for(j=0;j<n;j++)
	pcm[j]*=window[j];
    else
      for(j=0;j<n;j++)
	pcm[j]=0.f;
  }

  return(0);
}

vorbis_func_mapping mapping0_exportbundle={
  &mapping0_pack,
  &mapping0_unpack,
  &mapping0_look,
  &mapping0_copy_info,
  &mapping0_free_info,
  &mapping0_free_look,
  &mapping0_forward,
  &mapping0_inverse
};
