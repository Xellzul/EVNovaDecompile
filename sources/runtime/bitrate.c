// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -align 4 loses a MATCH here); Research/notes/burgervorbis.md
/* bitrate.c -- libvorbis 1.0rc3 lib/bitrate.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (unconditional frees, FastMemSet); altered: comments removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "os.h"
#include "bitrate.h"

#define LONG_BLOCK 0x80000000UL
#define BITS_MASK  0x7fffffff

/* Not linked: the four statics, vorbis_bitrate_init, _managed, _addblock and _flushpacket. */
static long BINBITS(bitrate_manager_state *bm,long head,long bits){
  long bin=(bits&BITS_MASK)>>6;
  long bins=bm->queue_bins;
  long lo=0;
  long hi;

  if(bin>0)lo=bm->queue_binned[head*bins+bin-1];
  if(bin<bins)
    hi=bm->queue_binned[head*bins+bin];
  else
    hi=lo;
  return lo+(((bits&0x3f)*(hi-lo))>>6);
}

static long LACING_ADJUST(long bits){
  long bytes=(bits+7)/8;
  return (bytes+(bytes+1)/256+1)*8;
}

static double floater_interpolate(bitrate_manager_state *bm,vorbis_info *vi,
				  double desired_average){
  int i=(int)(bm->avgfloat*BITTRACK_DIVISOR-1.);
  double lo=0.;
  double hi=0.;

  lo=(i?bm->avg_binacc[i-1]:0)/(double)bm->avg_sampleacc*vi->rate;
  while(lo>desired_average && i>0){
    i--;
    lo=(i?bm->avg_binacc[i-1]:0)/(double)bm->avg_sampleacc*vi->rate;
  }

  hi=(i>=bm->queue_bins?bm->avg_binacc[bm->queue_bins-1]:bm->avg_binacc[i])/(double)bm->avg_sampleacc*vi->rate;
  while(hi<desired_average && i<bm->queue_bins){
    i++;
    if(i<bm->queue_bins)
      hi=bm->avg_binacc[i]/(double)bm->avg_sampleacc*vi->rate;
  }

  if(i==bm->queue_bins)return i*(1./BITTRACK_DIVISOR);
  return (i+(desired_average-lo)/(hi-lo))*(1./BITTRACK_DIVISOR);
}

#define BINSTACK(f,k) ((k)>-bins?bm->minmax_binstack[(f)*bins*2+bins+(k)-1]:0)

static long limit_sum(bitrate_manager_state *bm,int limit){
  int bins=bm->queue_bins;
  int i=bm->minmax_stackptr;
  long ret=bm->minmax_acctotal;

  ret-=BINSTACK(i,0);
  ret+=BINSTACK(i,limit);

  for(i--;i>=0;i--){
    if(bm->minmax_limitstack[i]<=limit)break;
    ret-=BINSTACK(i,bm->minmax_limitstack[i]);
    ret+=BINSTACK(i,limit);
  }
  return ret;
}

void vorbis_bitrate_init(vorbis_info *vi,bitrate_manager_state *bm){
  int i;
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;
  bitrate_manager_info *bi=&ci->bi;

  FastMemSet(bm,0,sizeof(*bm));

  if(bi){
    bm->avg_sampledesired=bi->queue_avg_time*vi->rate;
    bm->avg_centerdesired=bi->queue_avg_time*vi->rate*bi->queue_avg_center;
    bm->minmax_sampledesired=bi->queue_minmax_time*vi->rate;

    {
      long maxlatency=max((long)(bm->avg_sampledesired-bm->avg_centerdesired),
			  (long)bm->minmax_sampledesired)+bm->avg_centerdesired;

      if(maxlatency>0 &&
	 (bi->queue_avgmin>0 || bi->queue_avgmax>0 ||
	  bi->queue_hardmax>0 || bi->queue_hardmin>0)){
	long maxpackets=maxlatency/(ci->blocksizes[0]>>1)+3;
	long bins=ci->passlimit[ci->coupling_passes-1]*BITTRACK_DIVISOR;

	bm->queue_size=maxpackets;
	bm->queue_bins=bins;
	bm->queue_binned=(ogg_uint32_t *)_ogg_malloc(maxpackets*bins*sizeof(*bm->queue_binned));
	bm->queue_actual=(ogg_uint32_t *)_ogg_malloc(maxpackets*sizeof(*bm->queue_actual));

	if((bi->queue_avgmin>0. || bi->queue_avgmax>0.) &&
	   bi->queue_avg_time>0.){
	  bm->avg_binacc=(long *)_ogg_malloc(bins*sizeof(*bm->avg_binacc));
	  bm->avgfloat=bi->avgfloat_initial;
	}else{
	  bm->avg_tail=-1;
	}

	if((bi->queue_hardmin>0. || bi->queue_hardmax>0.) &&
	   bi->queue_minmax_time>0.){
	  bm->minmax_binstack=(long *)_ogg_calloc((bins+1)*bins*2,sizeof(*bm->minmax_binstack));
	  bm->minmax_posstack=(long *)_ogg_calloc(bins+1,sizeof(*bm->minmax_posstack));
	  bm->minmax_limitstack=(long *)_ogg_calloc(bins+1,sizeof(*bm->minmax_limitstack));
	}else{
	  bm->minmax_tail=-1;
	}

	bm->queue_packet_buffers=(oggpack_buffer *)_ogg_calloc(maxpackets,sizeof(*bm->queue_packet_buffers));
	bm->queue_packets=(ogg_packet *)_ogg_calloc(maxpackets,sizeof(*bm->queue_packets));
	for(i=0;i<maxpackets;i++)
	  oggpack_writeinit(bm->queue_packet_buffers+i);
      }else{
	bm->queue_packet_buffers=(oggpack_buffer *)_ogg_calloc(1,sizeof(*bm->queue_packet_buffers));
	bm->queue_packets=(ogg_packet *)_ogg_calloc(1,sizeof(*bm->queue_packets));
	oggpack_writeinit(bm->queue_packet_buffers);
      }
    }
  }
}

/* vorbis_bitrate_clear. STATE: BurgerLib's libvorbis 1.0rc3, one free of the packet buffers
   after both arms (MATCH, byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x00544180
void vorbis_bitrate_clear(bitrate_manager_state *bm){
  int i;
  if(bm){
    _ogg_free(bm->queue_binned);
    _ogg_free(bm->queue_actual);
    _ogg_free(bm->avg_binacc);
    _ogg_free(bm->minmax_binstack);
    _ogg_free(bm->minmax_posstack);
    _ogg_free(bm->minmax_limitstack);
    if(bm->queue_packet_buffers){
      if(bm->queue_size==0){
	oggpack_writeclear(bm->queue_packet_buffers);
      }else{
	for(i=0;i<bm->queue_size;i++)
	  oggpack_writeclear(bm->queue_packet_buffers+i);
      }
      _ogg_free(bm->queue_packet_buffers);
    }
    _ogg_free(bm->queue_packets);
    FastMemSet(bm,0,sizeof(*bm));
  }
}

int vorbis_bitrate_managed(vorbis_block *vb){
  backend_lookup_state *b=(backend_lookup_state *)vb->vd->backend_state;
  bitrate_manager_state *bm=&b->bms;

  if(bm->queue_binned)return(1);
  return(0);
}

/* vorbis_bitrate_maxmarkers. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544270
int vorbis_bitrate_maxmarkers(void){
  return 8*BITTRACK_DIVISOR;
}

int vorbis_bitrate_addblock(vorbis_block *vb){
  vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;
  vorbis_dsp_state      *vd=vb->vd;
  backend_lookup_state  *b=(backend_lookup_state *)vd->backend_state;
  bitrate_manager_state *bm=&b->bms;
  vorbis_info           *vi=vd->vi;
  codec_setup_info      *ci=(codec_setup_info *)vi->codec_setup;
  bitrate_manager_info  *bi=&ci->bi;

  int eofflag=vb->eofflag;
  int head=bm->queue_head;
  int next_head=head+1;
  int bins=bm->queue_bins;
  ogg_uint32_t *this_bins;
  int start,end,pos;
  int i;

  if(!bm->queue_binned){
    if(head)return(-1);
    bm->queue_head++;
    bm->queue_packets[0].packet=oggpack_get_buffer(&vb->opb);
    bm->queue_packets[0].bytes=oggpack_bytes(&vb->opb);
    bm->queue_packets[0].b_o_s=0;
    bm->queue_packets[0].e_o_s=vb->eofflag;
    bm->queue_packets[0].granulepos=vb->granulepos;
    bm->queue_packets[0].packetno=vb->sequence;
    {
      oggpack_buffer temp;
      memcpy(&temp,bm->queue_packet_buffers,sizeof(temp));
      memcpy(bm->queue_packet_buffers,&vb->opb,sizeof(temp));
      memcpy(&vb->opb,&temp,sizeof(temp));
    }
    return(0);
  }

  if(next_head>=bm->queue_size)next_head=0;
  this_bins=bm->queue_binned+bins*head;

  if(next_head==bm->avg_tail || next_head==bm->minmax_tail)return(-1);
  bm->queue_head=next_head;
  bm->queue_actual[head]=(vb->W?LONG_BLOCK:0);

  bm->queue_packets[head].packet=oggpack_get_buffer(&vb->opb);
  bm->queue_packets[head].bytes=oggpack_bytes(&vb->opb);
  bm->queue_packets[head].b_o_s=0;
  bm->queue_packets[head].e_o_s=vb->eofflag;
  bm->queue_packets[head].granulepos=vb->granulepos;
  bm->queue_packets[head].packetno=vb->sequence;
  {
    oggpack_buffer temp;
    memcpy(&temp,bm->queue_packet_buffers+head,sizeof(temp));
    memcpy(bm->queue_packet_buffers+head,&vb->opb,sizeof(temp));
    memcpy(&vb->opb,&temp,sizeof(temp));
  }
  memcpy(this_bins,vbi->packet_markers,bins*sizeof(*this_bins));

  if(bm->avg_binacc){
    ogg_uint32_t desired_center=bm->avg_centerdesired;
    if(eofflag)desired_center=0;
    start=end=bm->avg_center;

    for(i=0;i<bins;i++)
      bm->avg_binacc[i]+=LACING_ADJUST(this_bins[i]);
    bm->avg_sampleacc+=ci->blocksizes[vb->W]>>1;
    bm->avg_centeracc+=ci->blocksizes[vb->W]>>1;

    if(bm->avg_sampleacc>bm->avg_sampledesired || eofflag){
      if(bm->avg_centeracc>desired_center){
	int samples=ci->blocksizes[vb->W]>>1;
	double avgmax=floater_interpolate(bm,vi,bi->queue_avgmax);
	double avgmin=floater_interpolate(bm,vi,bi->queue_avgmin);
	double target=bi->avgfloat_initial;
	double slew;
	int choice;

	if(avgmax>0. && avgmax<target)target=avgmax;
	if(avgmin<bi->avgfloat_minimum)avgmin=bi->avgfloat_minimum;
	if(avgmin>target)target=avgmin;

	slew=(target-bm->avgfloat)/samples*vi->rate;
	if(slew<bi->avgfloat_downslew_max)
	  target=bm->avgfloat+bi->avgfloat_downslew_max/vi->rate*samples;
	if(slew>bi->avgfloat_upslew_max)
	  target=bm->avgfloat+bi->avgfloat_upslew_max/vi->rate*samples;
	bm->avgfloat=target;
	choice=bm->avgfloat*1024.;

	while(bm->avg_centeracc>desired_center){
	  int W=(bm->queue_actual[bm->avg_center]&LONG_BLOCK)?1:0;
	  int s=ci->blocksizes[W]>>1;
	  bm->queue_actual[bm->avg_center]|=choice;
	  bm->avg_centeracc-=s;
	  bm->avg_center++;
	  if(bm->noisetrigger_postpone)bm->noisetrigger_postpone-=s;
	  if(bm->avg_center>=bm->queue_size)bm->avg_center=0;
	}
	end=bm->avg_center;

	if(bm->avgfloat<bi->avgfloat_noise_lowtrigger){
	  bm->noisetrigger_request+=1.;
	}else{
	  if(bm->noisetrigger_request>0. && bm->avgnoise>0.)bm->noisetrigger_request-=.2f;
	}
	if(bm->avgfloat>bi->avgfloat_noise_hightrigger){
	  bm->noisetrigger_request-=1.;
	}else{
	  if(bm->noisetrigger_request<0. && bm->avgnoise<0.)bm->noisetrigger_request+=.2f;
	}

	if(bm->noisetrigger_postpone<=0){
	  if(bm->noisetrigger_request<0.){
	    bm->avgnoise-=1.;
	    if(-bm->noisetrigger_request>(long)bm->avg_sampleacc/2)bm->avgnoise-=1.;
	    bm->noisetrigger_postpone=bm->avg_sampleacc/2;
	  }
	  if(bm->noisetrigger_request>0.){
	    bm->avgnoise+=1.;
	    if((long)bm->avg_sampleacc/2<bm->noisetrigger_request)bm->avgnoise+=1.;
	    bm->noisetrigger_postpone=bm->avg_sampleacc/2;
	  }
	  bm->noisetrigger_request=0.;
	  if(bm->avgnoise>0.)bm->noisetrigger_request=-1.;
	  if(bm->avgnoise<0.)bm->noisetrigger_request=1.;
	  if(bm->avgnoise<bi->avgfloat_noise_minval)bm->avgnoise=bi->avgfloat_noise_minval;
	  if(bm->avgnoise>bi->avgfloat_noise_maxval)bm->avgnoise=bi->avgfloat_noise_maxval;
	}
      }

      while(bm->avg_sampleacc>bm->avg_sampledesired){
	int W=(bm->queue_actual[bm->avg_tail]&LONG_BLOCK)?1:0;
	int s=ci->blocksizes[W]>>1;
	for(i=0;i<bm->queue_bins;i++)
	  bm->avg_binacc[i]-=LACING_ADJUST(bm->queue_binned[bm->avg_tail*bins+i]);
	bm->avg_sampleacc-=s;
	bm->avg_tail++;
	if(bm->avg_tail>=bm->queue_size)bm->avg_tail=0;
      }
    }
  }else{
    start=head;
    end=next_head;
    bm->queue_actual[head]|=(int)(bi->avgfloat_initial*1024.);
  }

  if(bm->minmax_binstack){
    ogg_uint32_t desired=bm->minmax_sampledesired;
    long limit=0;
    if(eofflag)desired=0;

    for(pos=start;pos!=end;){
      int W=(bm->queue_actual[pos]&LONG_BLOCK)?1:0;
      long samples=ci->blocksizes[W]>>1;
      unsigned j;
      for(j=0;j<bins;j++){
	ogg_uint32_t bound=(j+1)<<6;
	bm->minmax_binstack[bm->minmax_stackptr*bins*2+bins+j]+=
	  LACING_ADJUST(BINBITS(bm,pos,(bm->queue_actual[pos]&BITS_MASK)>bound?bm->queue_actual[pos]:bound));
	bm->minmax_binstack[bm->minmax_stackptr*bins*2+j]+=
	  LACING_ADJUST(BINBITS(bm,pos,(bm->queue_actual[pos]&BITS_MASK)<bound?bm->queue_actual[pos]:bound));
      }
      bm->minmax_posstack[bm->minmax_stackptr]=pos;
      bm->minmax_limitstack[bm->minmax_stackptr]=0;
      bm->minmax_sampleacc+=samples;
      bm->minmax_acctotal+=LACING_ADJUST(BINBITS(bm,pos,bm->queue_actual[pos]));
      pos++;
      if(pos>=bm->queue_size)pos=0;
    }

    if(bm->minmax_sampleacc>desired){
      long sum=bm->minmax_acctotal;
      double rate=(double)sum/bm->minmax_sampleacc*vi->rate;

      if((bi->queue_hardmax>0. && rate>bi->queue_hardmax) ||
	 (bi->queue_hardmin>0. && rate<bi->queue_hardmin)){
	int j,top;

	sum=limit_sum(bm,0);
	rate=(double)sum/bm->minmax_sampleacc*vi->rate;
	if(bi->queue_hardmax>0. && rate>bi->queue_hardmax){
	  for(limit=-1;limit>-bins;limit--){
	    sum=limit_sum(bm,limit);
	    rate=(double)sum/bm->minmax_sampleacc*vi->rate;
	    if(rate<=bi->queue_hardmax)break;
	  }
	}else if(rate<bi->queue_hardmin){
	  for(limit=1;limit<bins;limit++){
	    sum=limit_sum(bm,limit);
	    rate=(double)sum/bm->minmax_sampleacc*vi->rate;
	    if(rate>=bi->queue_hardmin)break;
	  }
	  if(rate>bi->queue_hardmax)limit--;
	}

	for(j=limit-1;j>-bins;j--)limit_sum(bm,j);
	limit_sum(bm,limit);
	for(top=bm->minmax_stackptr-1;top>=0 && bm->minmax_limitstack[top]>=limit;top--);

	if(bm->minmax_stackptr>top){
	  for(i=bm->minmax_stackptr;i>top;i--){
	    bm->minmax_acctotal-=BINSTACK(i,bm->minmax_limitstack[i]);
	    bm->minmax_acctotal+=BINSTACK(i,limit);
	    if(i<bm->minmax_stackptr){
	      for(j=0;j<bins*2;j++)
		bm->minmax_binstack[i*bins*2+j]+=bm->minmax_binstack[(i+1)*bins*2+j];
	    }
	  }
	}
	bm->minmax_posstack[top+1]=bm->minmax_posstack[bm->minmax_stackptr];
	bm->minmax_limitstack[top+1]=limit;
	bm->minmax_stackptr=top+2;
	FastMemSet(bm->minmax_binstack+(top+2)*bins*2,0,bins*2*sizeof(*bm->minmax_binstack));
	bm->minmax_limitstack[top+2]=0;
	bm->minmax_posstack[top+2]=-1;
      }
    }

    while(bm->minmax_sampleacc>desired){
      int W=(bm->queue_actual[bm->minmax_tail]&LONG_BLOCK)?1:0;
      long samples=ci->blocksizes[W]>>1;
      long bits=bm->queue_actual[bm->minmax_tail]&BITS_MASK;
      unsigned j;
      for(j=0;j<bins;j++){
	long bound=(j+1)<<6;
	bm->minmax_binstack[bins+j]-=
	  LACING_ADJUST(BINBITS(bm,bm->minmax_tail,bits>bound?bits:bound));
	bm->minmax_binstack[j]-=
	  LACING_ADJUST(BINBITS(bm,bm->minmax_tail,bits<bound?bits:bound));
      }
      if(bits<(bm->minmax_limitstack[0]<<6))bits=bm->minmax_limitstack[0]<<6;
      if(bits>((bins+bm->minmax_limitstack[0])<<6))bits=(bins+bm->minmax_limitstack[0])<<6;
      bm->minmax_acctotal-=LACING_ADJUST(BINBITS(bm,bm->minmax_tail,bits));
      bm->minmax_sampleacc-=samples;
      bm->queue_actual[bm->minmax_tail]&=LONG_BLOCK;
      bm->queue_actual[bm->minmax_tail]|=bits;
      if(bm->minmax_tail==bm->minmax_posstack[0]){
	memmove(bm->minmax_binstack,bm->minmax_binstack+bins*2,bm->minmax_stackptr*bins*2*sizeof(*bm->minmax_binstack));
	memmove(bm->minmax_posstack,bm->minmax_posstack+1,bm->minmax_stackptr*sizeof(*bm->minmax_posstack));
	memmove(bm->minmax_limitstack,bm->minmax_limitstack+1,bm->minmax_stackptr*sizeof(*bm->minmax_limitstack));
	bm->minmax_stackptr--;
      }
      bm->minmax_tail++;
      if(bm->minmax_tail>=bm->queue_size)bm->minmax_tail=0;
    }
    bm->last_to_flush=bm->minmax_tail;
  }else{
    bm->last_to_flush=bm->avg_center;
  }
  if(eofflag)bm->last_to_flush=bm->queue_head;
  return(0);
}

int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd,ogg_packet *op){
  backend_lookup_state *b=(backend_lookup_state *)vd->backend_state;
  bitrate_manager_state *bm=&b->bms;

  if(bm->queue_size==0){
    if(bm->queue_head==0)return(0);
    memcpy(op,bm->queue_packets,sizeof(*op));
    bm->queue_head=0;
  }else{
    long bits;

    if(bm->next_to_flush==bm->last_to_flush)return(0);
    bits=BINBITS(bm,bm->next_to_flush,bm->queue_actual[bm->next_to_flush]);
    memcpy(op,bm->queue_packets+bm->next_to_flush,sizeof(*op));
    if((bits+7)/8<op->bytes)op->bytes=(bits+7)/8;
    bm->next_to_flush++;
    if(bm->next_to_flush>=bm->queue_size)bm->next_to_flush=0;
  }
  return(1);
}
