// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto or -align 4 loses MATCHes here; inlined: ilog, icount); Research/notes/burgervorbis.md
/* res0.c -- libvorbis 1.0rc3 lib/res0.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (the decode map one AllocAPointerClear block, unconditional frees, oggpack_read1, FastMemSet); altered: comments, the TRAIN_RES and #if 0 branches (not built) removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"
#include "os.h"

typedef struct vorbis_look_residue{
  vorbis_info_residue0 *info;
  int         map;

  int         parts;
  int         stages;
  codebook   *fullbooks;
  codebook   *phrasebook;
  codebook ***partbooks;

  int         partvals;
  int       **decodemap;

  long      postbits;
  long      phrasebits;
  long      frames;

  int       qoffsets[BITTRACK_DIVISOR+1];

} vorbis_look_residue0;

/* res0_copy_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557630
vorbis_info_residue *res0_copy_info(vorbis_info_residue *vr){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  vorbis_info_residue0 *ret=(vorbis_info_residue0 *)_ogg_malloc(sizeof(*ret));
  memcpy(ret,info,sizeof(*ret));
  return(ret);
}

/* res0_free_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557690
void res0_free_info(vorbis_info_residue *i){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)i;
  if(info){
    FastMemSet(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

/* res0_free_look. STATE: MATCH, byte-identical (130 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005576d0
void res0_free_look(vorbis_look_residue *i){
  int j;
  if(i){
    vorbis_look_residue0 *look=(vorbis_look_residue0 *)i;

    for(j=0;j<look->parts;j++)
      _ogg_free(look->partbooks[j]);
    _ogg_free(look->partbooks);
    _ogg_free(look->decodemap);

    FastMemSet(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

static int ilog(unsigned int v){
  int ret=0;
  if(v){
    do{
      ret++;
      v>>=1;
    }while(v);
  }
  return(ret);
}

static int icount(unsigned int v){
  int ret=0;
  if(v){
    do{
      ret+=v&1;
      v>>=1;
    }while(v);
  }
  return(ret);
}

/* res0_pack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557760
void res0_pack(vorbis_info_residue *vr,oggpack_buffer *opb){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  int j,acc=0;
  oggpack_write(opb,info->begin,24);
  oggpack_write(opb,info->end,24);

  oggpack_write(opb,info->grouping-1,24);

  oggpack_write(opb,info->partitions-1,6);
  oggpack_write(opb,info->groupbook,8);

  for(j=0;j<info->partitions;j++){
    if(ilog(info->secondstages[j])>3){
      oggpack_write(opb,info->secondstages[j],3);
      oggpack_write(opb,1,1);
      oggpack_write(opb,info->secondstages[j]>>3,5);
    }else
      oggpack_write(opb,info->secondstages[j],4);
    acc+=icount(info->secondstages[j]);
  }
  for(j=0;j<acc;j++)
    oggpack_write(opb,info->booklist[j],8);
}

/* res0_unpack. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557910
vorbis_info_residue *res0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int j,acc=0;
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)_ogg_calloc(1,sizeof(*info));
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;

  info->begin=oggpack_read(opb,24);
  info->end=oggpack_read(opb,24);
  info->grouping=oggpack_read(opb,24)+1;
  info->partitions=oggpack_read(opb,6)+1;
  info->groupbook=oggpack_read(opb,8);

  for(j=0;j<info->partitions;j++){
    int cascade=oggpack_read(opb,3);
    if(oggpack_read1(opb))
      cascade|=(oggpack_read(opb,5)<<3);
    info->secondstages[j]=cascade;

    acc+=icount(cascade);
  }
  for(j=0;j<acc;j++)
    info->booklist[j]=oggpack_read(opb,8);

  if(info->groupbook>=ci->books)goto errout;
  for(j=0;j<acc;j++)
    if(info->booklist[j]>=ci->books)goto errout;

  return(info);
 errout:
  res0_free_info(info);
  return(NULL);
}

/* res0_look. STATE: BurgerLib's libvorbis 1.0rc3, MATCH, byte-identical (1350 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00557af0
vorbis_look_residue *res0_look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_residue *vr){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)_ogg_calloc(1,sizeof(*look));
  backend_lookup_state *be=(backend_lookup_state *)vd->backend_state;

  int j,k,acc=0;
  int dim;
  int maxstage=0;
  int *entry;
  look->info=info;
  look->map=vm->mapping;

  look->parts=info->partitions;
  look->fullbooks=be->fullbooks;
  look->phrasebook=be->fullbooks+info->groupbook;
  dim=look->phrasebook->dim;

  look->partbooks=(codebook ***)_ogg_calloc(look->parts,sizeof(*look->partbooks));

  for(j=0;j<look->parts;j++){
    int stages=ilog(info->secondstages[j]);
    if(stages){
      if(stages>maxstage)maxstage=stages;
      look->partbooks[j]=(codebook **)_ogg_calloc(stages,sizeof(*look->partbooks[j]));
      for(k=0;k<stages;k++)
	if(info->secondstages[j]&(1<<k)){
	  look->partbooks[j][k]=be->fullbooks+info->booklist[acc++];
	}
    }
  }

  look->partvals=rint(pow((float)look->parts,(float)dim));
  look->stages=maxstage;
  look->decodemap=(int **)AllocAPointerClear(look->partvals*dim*sizeof(int)+look->partvals*sizeof(int *));
  entry=(int *)(look->decodemap+look->partvals);
  for(j=0;j<look->partvals;j++){
    long val=j;
    long mult=look->partvals/look->parts;
    look->decodemap[j]=entry;
    for(k=0;k<dim;k++){
      long deco=val/mult;
      val-=deco*mult;
      mult/=look->parts;
      entry[k]=deco;
    }
    entry+=dim;
  }

  {
    int samples_per_partition=info->grouping;
    int n=info->end-info->begin,i;
    int partvals=n/samples_per_partition;

    for(i=0;i<BITTRACK_DIVISOR;i++)
      look->qoffsets[i]=partvals*(i+1)/BITTRACK_DIVISOR;

    look->qoffsets[i]=9999999;
  }

  return(look);
}

/* _testhack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558040
static int _testhack(float *vec,int n,vorbis_look_residue0 *look,
		     int auxparts,int auxpartnum){
  vorbis_info_residue0 *info=look->info;
  int i;
  float max=0.f;
  float temp[128];
  float entropy=0.f;

  for(i=0;i<n;i++)temp[i]=fabs(vec[i]);

  for(i=0;i<n;i++)
    if(temp[i]>max)max=temp[i];

  for(i=0;i<n;i++)temp[i]=rint(temp[i]);

  for(i=0;i<n;i++)
    entropy+=temp[i];

  for(i=0;i<auxparts-1;i++)
    if(auxpartnum<info->blimit[i] &&
       entropy<=info->entmax[i] &&
       max<=info->ampmax[i])
      break;

  return(i);
}

/* _interleaved_encodepart. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005581e0
static int _interleaved_encodepart(oggpack_buffer *opb,float *vec, int n,
				   codebook *book,long *acc){
  int i,bits=0;
  int dim=book->dim;
  int step=n/dim;

  for(i=0;i<step;i++){
    int entry=vorbis_book_besterror(book,vec+i,step,0);

    bits+=vorbis_book_encode(book,entry,opb);
  }

  return(bits);
}

/* _encodepart. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558270
static int _encodepart(oggpack_buffer *opb,float *vec, int n,
		       codebook *book,long *acc){
  int i,bits=0;
  int dim=book->dim;
  int step=n/dim;

  for(i=0;i<step;i++){
    int entry=vorbis_book_besterror(book,vec+i*dim,1,0);

    bits+=vorbis_book_encode(book,entry,opb);
  }

  return(bits);
}

/* _01class. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558310
static long **_01class(vorbis_block *vb,vorbis_look_residue *vl,
		       float **in,int ch,
		       int (*classify)(float *,int,vorbis_look_residue0 *,
				       int,int)){
  long i,j;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  long **partword=(long **)_vorbis_block_alloc(vb,ch*sizeof(*partword));

  for(i=0;i<ch;i++){
    partword[i]=(long *)_vorbis_block_alloc(vb,n/samples_per_partition*sizeof(*partword[i]));
    FastMemSet(partword[i],0,n/samples_per_partition*sizeof(*partword[i]));
  }

  for(i=0;i<partvals;i++){
    for(j=0;j<ch;j++)
      partword[j][i]=
	classify(in[j]+i*samples_per_partition+info->begin,
		 samples_per_partition,look,possible_partitions,i);
  }

  look->frames++;

  return(partword);
}

/* _2class. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005584b0
static long **_2class(vorbis_block *vb,vorbis_look_residue *vl,
		       float **in,int ch,
		       int (*classify)(float *,int,vorbis_look_residue0 *,
				       int,int)){
  long i,j,k,l;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  long **partword=(long **)_vorbis_block_alloc(vb,sizeof(*partword));
  float *work=(float *)alloca(sizeof(*work)*samples_per_partition);

  partword[0]=(long *)_vorbis_block_alloc(vb,n*ch/samples_per_partition*sizeof(*partword[0]));
  FastMemSet(partword[0],0,n*ch/samples_per_partition*sizeof(*partword[0]));

  for(i=0,j=0,k=0,l=info->begin;i<partvals;i++){
    for(k=0;k<samples_per_partition;k++){
      work[k]=in[j][l];
      j++;
      if(j>=ch){
	j=0;
	l++;
      }
    }

    partword[0][i]=
      classify(work,samples_per_partition,look,possible_partitions,i);
  }

  look->frames++;

  return(partword);
}

/* _01forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558600
static int _01forward(vorbis_block *vb,vorbis_look_residue *vl,
		      float **in,int ch,
		      int pass,long **partword,
		      int (*encode)(oggpack_buffer *,float *,int,
				    codebook *,long *),
		      ogg_uint32_t *stats){
  long i,j,k,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  vorbis_dsp_state      *vd=vb->vd;
  vorbis_info           *vi=vd->vi;
  codec_setup_info      *ci=(codec_setup_info *)vi->codec_setup;

  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  long resbits[128];
  long resvals[128];

  FastMemSet(resbits,0,sizeof(resbits));
  FastMemSet(resvals,0,sizeof(resvals));

  for(s=(pass==0?0:ci->passlimit[pass-1]);s<ci->passlimit[pass];s++){
    int bin=0;
    ogg_uint32_t *qptr=NULL;
    if(stats)qptr=stats+s*BITTRACK_DIVISOR;

    for(i=0;i<partvals;){

      if(s==0){
	for(j=0;j<ch;j++){
	  long val=partword[j][i];
	  for(k=1;k<partitions_per_word;k++){
	    val*=possible_partitions;
	    if(i+k<partvals)
	      val+=partword[j][i+k];
	  }

	  if(val<look->phrasebook->entries)
	    look->phrasebits+=vorbis_book_encode(look->phrasebook,val,&vb->opb);
	}
      }

      for(k=0;k<partitions_per_word && i<partvals;k++,i++){
	long offset=i*samples_per_partition+info->begin;

	if(qptr)while(i>=look->qoffsets[bin])
	  qptr[bin++]=oggpack_bits(&vb->opb);

	for(j=0;j<ch;j++){
	  if(s==0)resvals[partword[j][i]]+=samples_per_partition;
	  if(info->secondstages[partword[j][i]]&(1<<s)){
	    codebook *statebook=look->partbooks[partword[j][i]][s];
	    if(statebook){
	      int ret;
	      long *accumulator=NULL;

	      ret=encode(&vb->opb,in[j]+offset,samples_per_partition,
			 statebook,accumulator);

	      look->postbits+=ret;
	      resbits[partword[j][i]]+=ret;
	    }
	  }
	}
      }
      if(qptr)while(i>=look->qoffsets[bin])
	qptr[bin++]=oggpack_bits(&vb->opb);
    }
  }

  return(0);
}

/* _01inverse. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558b10
static int _01inverse(vorbis_block *vb,vorbis_look_residue *vl,
		      float **in,int ch,
		      long (*decodepart)(codebook *, float *,
					 oggpack_buffer *,int)){

  long i,j,k,l,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  int samples_per_partition=info->grouping;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int ***partword=(int ***)alloca(ch*sizeof(*partword));

  for(j=0;j<ch;j++)
    partword[j]=(int **)_vorbis_block_alloc(vb,partwords*sizeof(*partword[j]));

  for(s=0;s<look->stages;s++){

    for(i=0,l=0;i<partvals;l++){
      if(s==0){
	for(j=0;j<ch;j++){
	  int temp=vorbis_book_decode(look->phrasebook,&vb->opb);
	  if(temp==-1)goto eopbreak;
	  partword[j][l]=look->decodemap[temp];
	  if(partword[j][l]==NULL)goto errout;
	}
      }

      for(k=0;k<partitions_per_word && i<partvals;k++,i++)
	for(j=0;j<ch;j++){
	  long offset=info->begin+i*samples_per_partition;
	  if(info->secondstages[partword[j][l][k]]&(1<<s)){
	    codebook *stagebook=look->partbooks[partword[j][l][k]][s];
	    if(stagebook){
	      if(decodepart(stagebook,in[j]+offset,&vb->opb,
			    samples_per_partition)==-1)goto eopbreak;
	    }
	  }
	}
    }
  }

 errout:
 eopbreak:
  return(0);
}

/* res0_class. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558d60
long **res0_class(vorbis_block *vb,vorbis_look_residue *vl,
		  float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01class(vb,vl,in,used,_testhack));
  else
    return(0);
}

/* res0_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00558df0
int res0_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,float **out,int *nonzero,int ch,
		 int pass, long **partword,ogg_uint32_t *stats){

  int i,j,used=0,n=vb->pcmend/2;
  for(i=0;i<ch;i++)
    if(nonzero[i]){
      for(j=0;j<n;j++)
	out[i][j]+=in[i][j];
      in[used++]=in[i];
    }
  if(used){
    int ret=_01forward(vb,vl,in,used,pass,partword,
		      _interleaved_encodepart,stats);
    used=0;
    for(i=0;i<ch;i++)
      if(nonzero[i]){
	for(j=0;j<n;j++)
	  out[i][j]-=in[used][j];
	used++;
      }
    return(ret);
  }else{
    for(i=0;i<vorbis_bitrate_maxmarkers();i++)
      stats[i]=oggpack_bits(&vb->opb);

    return(0);
  }
}

/* res0_inverse. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005591d0
int res0_inverse(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01inverse(vb,vl,in,used,vorbis_book_decodevs_add));
  else
    return(0);
}

/* res1_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559260
int res1_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,float **out,int *nonzero,int ch,
		 int pass, long **partword, ogg_uint32_t *stats){
  int i,j,used=0,n=vb->pcmend/2;
  for(i=0;i<ch;i++)
    if(nonzero[i]){
      for(j=0;j<n;j++)
	out[i][j]+=in[i][j];
      in[used++]=in[i];
    }

  if(used){
    int ret=_01forward(vb,vl,in,used,pass,partword,_encodepart,stats);
    used=0;
    for(i=0;i<ch;i++)
      if(nonzero[i]){
	for(j=0;j<n;j++)
	  out[i][j]-=in[used][j];
	used++;
      }
    return(ret);
  }else{
    for(i=0;i<vorbis_bitrate_maxmarkers();i++)
      stats[i]=oggpack_bits(&vb->opb);

    return(0);
  }
}

/* res1_class. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559640
long **res1_class(vorbis_block *vb,vorbis_look_residue *vl,
		  float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01class(vb,vl,in,used,_testhack));
  else
    return(0);
}

/* res1_inverse. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005596d0
int res1_inverse(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01inverse(vb,vl,in,used,vorbis_book_decodev_add));
  else
    return(0);
}

/* res2_class. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559760
long **res2_class(vorbis_block *vb,vorbis_look_residue *vl,
		  float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_2class(vb,vl,in,used,_testhack));
  else
    return(0);
}

/* res2_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005597f0
int res2_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,float **out,int *nonzero,int ch,
		 int pass,long **partword,ogg_uint32_t *stats){
  long i,j,k,n=vb->pcmend/2,used=0;

  float *work=(float *)_vorbis_block_alloc(vb,ch*n*sizeof(*work));
  for(i=0;i<ch;i++){
    float *pcm=in[i];
    if(nonzero[i])used++;
    for(j=0,k=i;j<n;j++,k+=ch)
      work[k]=pcm[j];
  }

  if(used){
    int ret=_01forward(vb,vl,&work,1,pass,partword,_encodepart,stats);

    for(i=0;i<ch;i++){
      float *pcm=in[i];
      float *sofar=out[i];
      for(j=0,k=i;j<n;j++,k+=ch)
	sofar[j]+=pcm[j]-work[k];
    }
    return(ret);
  }else{
    for(i=0;i<vorbis_bitrate_maxmarkers();i++)
      stats[i]=oggpack_bits(&vb->opb);

    return(0);
  }
}

/* res2_inverse. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00559b70
int res2_inverse(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int *nonzero,int ch){
  long i,k,l,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  int samples_per_partition=info->grouping;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int **partword=(int **)_vorbis_block_alloc(vb,partwords*sizeof(*partword));

  for(i=0;i<ch;i++)if(nonzero[i])break;
  if(i==ch)return(0);

  for(s=0;s<look->stages;s++){
    for(i=0,l=0;i<partvals;l++){

      if(s==0){
	int temp=vorbis_book_decode(look->phrasebook,&vb->opb);
	if(temp==-1)goto eopbreak;
	partword[l]=look->decodemap[temp];
	if(partword[l]==NULL)goto errout;
      }

      for(k=0;k<partitions_per_word && i<partvals;k++,i++)
	if(info->secondstages[partword[l][k]]&(1<<s)){
	  codebook *stagebook=look->partbooks[partword[l][k]][s];

	  if(stagebook){
	    if(vorbis_book_decodevv_add(stagebook,in,
					i*samples_per_partition+info->begin,ch,
					&vb->opb,samples_per_partition)==-1)
	      goto eopbreak;
	  }
	}
    }
  }

 errout:
 eopbreak:
  return(0);
}

vorbis_func_residue residue0_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res0_class,
  &res0_forward,
  &res0_inverse
};

vorbis_func_residue residue1_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res1_class,
  &res1_forward,
  &res1_inverse
};

vorbis_func_residue residue2_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res2_class,
  &res2_forward,
  &res2_inverse
};
