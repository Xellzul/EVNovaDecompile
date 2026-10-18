// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto or -align 4 loses MATCHes here; inlined: _v_readstring and vorbis_comment_clear into _vorbis_unpack_comment); Research/notes/burgervorbis.md
/* info.c -- libvorbis 1.0rc3 lib/info.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (oggpack_read1 for 1-bit reads, FastMemSet); altered: comments and the functions the exe does not link (the comment editor and the encoder's header packers) removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "codebook.h"
#include "registry.h"
#include "window.h"
#include "psy.h"
#include "misc.h"
#include "os.h"

static void _v_readstring(oggpack_buffer *o,char *buf,int bytes){
  while(bytes--){
    *buf++=oggpack_read(o,8);
  }
}

/* vorbis_comment_init. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532d10
void vorbis_comment_init(vorbis_comment *vc){
  FastMemSet(vc,0,sizeof(*vc));
}

/* not linked */
void vorbis_comment_add_tag(vorbis_comment *vc,char *tag,char *contents){
  char *comment=(char *)alloca(strlen(tag)+strlen(contents)+2);
  strcpy(comment,tag);
  strcat(comment,"=");
  strcat(comment,contents);
  vorbis_comment_add(vc,comment);
}

/* vorbis_comment_clear. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532d40
void vorbis_comment_clear(vorbis_comment *vc){
  if(vc){
    long i;
    for(i=0;i<vc->comments;i++)
      if(vc->user_comments[i])_ogg_free(vc->user_comments[i]);
    if(vc->user_comments)_ogg_free(vc->user_comments);
	if(vc->comment_lengths)_ogg_free(vc->comment_lengths);
    if(vc->vendor)_ogg_free(vc->vendor);
  }
  FastMemSet(vc,0,sizeof(*vc));
}

/* vorbis_info_init. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532de0
void vorbis_info_init(vorbis_info *vi){
  FastMemSet(vi,0,sizeof(*vi));
  vi->codec_setup=_ogg_calloc(1,sizeof(codec_setup_info));
}

/* vorbis_info_clear. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532e20
void vorbis_info_clear(vorbis_info *vi){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  int i;

  if(ci){

    for(i=0;i<ci->modes;i++)
      if(ci->mode_param[i])_ogg_free(ci->mode_param[i]);

    for(i=0;i<ci->maps;i++)
      _mapping_P[ci->map_type[i]]->free_info(ci->map_param[i]);

    for(i=0;i<ci->times;i++)
      _time_P[ci->time_type[i]]->free_info(ci->time_param[i]);

    for(i=0;i<ci->floors;i++)
      _floor_P[ci->floor_type[i]]->free_info(ci->floor_param[i]);

    for(i=0;i<ci->residues;i++)
      _residue_P[ci->residue_type[i]]->free_info(ci->residue_param[i]);

    for(i=0;i<ci->books;i++){
      if(ci->book_param[i]){
	vorbis_staticbook_destroy(ci->book_param[i]);
      }
    }

    for(i=0;i<ci->psys;i++)
      _vi_psy_free(ci->psy_param[i]);

    _ogg_free(ci);
  }

  FastMemSet(vi,0,sizeof(*vi));
}

/* _vorbis_unpack_info. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00532ff0
static int _vorbis_unpack_info(vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  if(!ci)return(OV_EFAULT);

  vi->version=oggpack_read(opb,32);
  if(vi->version!=0)return(OV_EVERSION);

  vi->channels=oggpack_read(opb,8);
  vi->rate=oggpack_read(opb,32);

  vi->bitrate_upper=oggpack_read(opb,32);
  vi->bitrate_nominal=oggpack_read(opb,32);
  vi->bitrate_lower=oggpack_read(opb,32);

  ci->blocksizes[0]=1<<oggpack_read(opb,4);
  ci->blocksizes[1]=1<<oggpack_read(opb,4);

  if(vi->rate<1)goto err_out;
  if(vi->channels<1)goto err_out;
  if(ci->blocksizes[0]<8)goto err_out;
  if(ci->blocksizes[1]<ci->blocksizes[0])goto err_out;

  if(oggpack_read1(opb)!=1)goto err_out;

  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(OV_EBADHEADER);
}

/* _vorbis_unpack_comment. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00533150
static int _vorbis_unpack_comment(vorbis_comment *vc,oggpack_buffer *opb){
  int i;
  int vendorlen=oggpack_read(opb,32);
  if(vendorlen<0)goto err_out;
  vc->vendor=(char *)_ogg_calloc(vendorlen+1,1);
  _v_readstring(opb,vc->vendor,vendorlen);
  vc->comments=oggpack_read(opb,32);
  if(vc->comments<0)goto err_out;
  vc->user_comments=(char **)_ogg_calloc(vc->comments+1,sizeof(*vc->user_comments));
  vc->comment_lengths=(int *)_ogg_calloc(vc->comments+1, sizeof(*vc->comment_lengths));

  for(i=0;i<vc->comments;i++){
    int len=oggpack_read(opb,32);
    if(len<0)goto err_out;
	vc->comment_lengths[i]=len;
    vc->user_comments[i]=(char *)_ogg_calloc(len+1,1);
    _v_readstring(opb,vc->user_comments[i],len);
  }
  if(oggpack_read1(opb)!=1)goto err_out;

  return(0);
 err_out:
  vorbis_comment_clear(vc);
  return(OV_EBADHEADER);
}

/* _vorbis_unpack_books. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00533370
static int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  int i;
  if(!ci)return(OV_EFAULT);

  ci->books=oggpack_read(opb,8)+1;

  for(i=0;i<ci->books;i++){
    ci->book_param[i]=(static_codebook *)_ogg_calloc(1,sizeof(*ci->book_param[i]));
    if(vorbis_staticbook_unpack(opb,ci->book_param[i]))goto err_out;
  }

  ci->times=oggpack_read(opb,6)+1;

  for(i=0;i<ci->times;i++){
    ci->time_type[i]=oggpack_read(opb,16);
    if(ci->time_type[i]<0 || ci->time_type[i]>=VI_TIMEB)goto err_out;
    ci->time_param[i]=_time_P[ci->time_type[i]]->unpack(vi,opb);
    if(!ci->time_param[i])goto err_out;
  }

  ci->floors=oggpack_read(opb,6)+1;

  for(i=0;i<ci->floors;i++){
    ci->floor_type[i]=oggpack_read(opb,16);
    if(ci->floor_type[i]<0 || ci->floor_type[i]>=VI_FLOORB)goto err_out;
    ci->floor_param[i]=_floor_P[ci->floor_type[i]]->unpack(vi,opb);
    if(!ci->floor_param[i])goto err_out;
  }

  ci->residues=oggpack_read(opb,6)+1;

  for(i=0;i<ci->residues;i++){
    ci->residue_type[i]=oggpack_read(opb,16);
    if(ci->residue_type[i]<0 || ci->residue_type[i]>=VI_RESB)goto err_out;
    ci->residue_param[i]=_residue_P[ci->residue_type[i]]->unpack(vi,opb);
    if(!ci->residue_param[i])goto err_out;
  }

  ci->maps=oggpack_read(opb,6)+1;

  for(i=0;i<ci->maps;i++){
    ci->map_type[i]=oggpack_read(opb,16);
    if(ci->map_type[i]<0 || ci->map_type[i]>=VI_MAPB)goto err_out;
    ci->map_param[i]=_mapping_P[ci->map_type[i]]->unpack(vi,opb);
    if(!ci->map_param[i])goto err_out;
  }

  ci->modes=oggpack_read(opb,6)+1;

  for(i=0;i<ci->modes;i++){
    ci->mode_param[i]=(vorbis_info_mode *)_ogg_calloc(1,sizeof(*ci->mode_param[i]));
    ci->mode_param[i]->blockflag=oggpack_read1(opb);
    ci->mode_param[i]->windowtype=oggpack_read(opb,16);
    ci->mode_param[i]->transformtype=oggpack_read(opb,16);
    ci->mode_param[i]->mapping=oggpack_read(opb,8);

    if(ci->mode_param[i]->windowtype>=VI_WINDOWB)goto err_out;
    if(ci->mode_param[i]->transformtype>=VI_WINDOWB)goto err_out;
    if(ci->mode_param[i]->mapping>=ci->maps)goto err_out;
  }

  if(oggpack_read1(opb)!=1)goto err_out;

  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(OV_EBADHEADER);
}

/* vorbis_synthesis_headerin. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x005337c0
int vorbis_synthesis_headerin(vorbis_info *vi,vorbis_comment *vc,ogg_packet *op){
  oggpack_buffer opb;

  if(op){
    oggpack_readinit(&opb,op->packet,op->bytes);

    {
      char buffer[6];
      int packtype=oggpack_read(&opb,8);
      FastMemSet(buffer,0,6);
      _v_readstring(&opb,buffer,6);
      if(memcmp(buffer,"vorbis",6)){
	return(OV_ENOTVORBIS);
      }
      switch(packtype){
      case 0x01:
	if(!op->b_o_s){
	  return(OV_EBADHEADER);
	}
	if(vi->rate!=0){
	  return(OV_EBADHEADER);
	}

	return(_vorbis_unpack_info(vi,&opb));

      case 0x03:
	if(vi->rate==0){
	  return(OV_EBADHEADER);
	}

	return(_vorbis_unpack_comment(vc,&opb));

      case 0x05:
	if(vi->rate==0 || vc->vendor==NULL){
	  return(OV_EBADHEADER);
	}

	return(_vorbis_unpack_books(vi,&opb));

      default:
	return(OV_EBADHEADER);
	break;
      }
    }
  }
  return(OV_EBADHEADER);
}
