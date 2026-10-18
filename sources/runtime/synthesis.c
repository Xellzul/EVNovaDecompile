// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -align 4 loses the MATCH here); Research/notes/burgervorbis.md
/* synthesis.c -- libvorbis 1.0rc3 lib/synthesis.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (oggpack_read1 for 1-bit reads); altered: comments and vorbis_packet_blocksize (not linked) removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"
#include "os.h"

/* vorbis_synthesis. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00533a30
int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  vorbis_dsp_state     *vd=vb->vd;
  backend_lookup_state *b=(backend_lookup_state *)vd->backend_state;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  oggpack_buffer       *opb=&vb->opb;
  int                   type,mode,i;

  _vorbis_block_ripcord(vb);
  oggpack_readinit(opb,op->packet,op->bytes);

  if(oggpack_read1(opb)!=0){
    return(OV_ENOTAUDIO);
  }

  mode=oggpack_read(opb,b->modebits);
  if(mode==-1)return(OV_EBADPACKET);

  vb->mode=mode;
  vb->W=ci->mode_param[mode]->blockflag;
  if(vb->W){
    vb->lW=oggpack_read1(opb);
    vb->nW=oggpack_read1(opb);
    if(vb->nW==-1)   return(OV_EBADPACKET);
  }else{
    vb->lW=0;
    vb->nW=0;
  }

  vb->granulepos=op->granulepos;
  vb->sequence=op->packetno-3;
  vb->eofflag=op->e_o_s;

  vb->pcmend=ci->blocksizes[vb->W];
  vb->pcm=(float **)_vorbis_block_alloc(vb,sizeof(*vb->pcm)*vi->channels);
  for(i=0;i<vi->channels;i++)
    vb->pcm[i]=(float *)_vorbis_block_alloc(vb,vb->pcmend*sizeof(*vb->pcm[i]));

  type=ci->map_type[ci->mode_param[mode]->mapping];

  return(_mapping_P[type]->inverse(vb,b->mode[mode]));
}
