// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; no veto here, so the library's one line); Research/notes/burgervorbis.md
/* codebook.c -- libvorbis 1.0rc3 lib/codebook.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (oggpack_read1 for 1-bit reads, FastMemSet, the decodevs_add scratch from the heap); altered: comments and the functions the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codebook.h"
#include "scales.h"
#include "misc.h"
#include "os.h"

/* vorbis_staticbook_unpack. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054cc80
int vorbis_staticbook_unpack(oggpack_buffer *opb,static_codebook *s){
  long i,j;
  FastMemSet(s,0,sizeof(*s));
  s->allocedp=1;

  if(oggpack_read(opb,24)!=0x564342)goto _eofout;

  s->dim=oggpack_read(opb,16);
  s->entries=oggpack_read(opb,24);
  if(s->entries==-1)goto _eofout;

  switch((int)oggpack_read1(opb)){
  case 0:
    s->lengthlist=(long *)_ogg_malloc(sizeof(*s->lengthlist)*s->entries);

    if(oggpack_read1(opb)){
      for(i=0;i<s->entries;i++){
	if(oggpack_read1(opb)){
	  long num=oggpack_read(opb,5);
	  if(num==-1)goto _eofout;
	  s->lengthlist[i]=num+1;
	}else
	  s->lengthlist[i]=0;
      }
    }else{
      for(i=0;i<s->entries;i++){
	long num=oggpack_read(opb,5);
	if(num==-1)goto _eofout;
	s->lengthlist[i]=num+1;
      }
    }

    break;
  case 1:
    {
      long length=oggpack_read(opb,5)+1;
      s->lengthlist=(long *)_ogg_malloc(sizeof(*s->lengthlist)*s->entries);

      for(i=0;i<s->entries;){
	long num=oggpack_read(opb,_ilog(s->entries-i));
	if(num==-1)goto _eofout;
	for(j=0;j<num && i<s->entries;j++,i++)
	  s->lengthlist[i]=length;
	length++;
      }
    }
    break;
  default:
    return(-1);
  }

  switch((s->maptype=oggpack_read(opb,4))){
  case 0:
    break;
  case 1: case 2:
    s->q_min=oggpack_read(opb,32);
    s->q_delta=oggpack_read(opb,32);
    s->q_quant=oggpack_read(opb,4)+1;
    s->q_sequencep=oggpack_read1(opb);

    {
      int quantvals=0;
      switch(s->maptype){
      case 1:
	quantvals=_book_maptype1_quantvals(s);
	break;
      case 2:
	quantvals=s->entries*s->dim;
	break;
      }

      s->quantlist=(long *)_ogg_malloc(sizeof(*s->quantlist)*quantvals);
      for(i=0;i<quantvals;i++)
	s->quantlist[i]=oggpack_read(opb,s->q_quant);

      if(quantvals&&s->quantlist[quantvals-1]==-1)goto _eofout;
    }
    break;
  default:
    goto _errout;
  }

  return(0);

 _errout:
 _eofout:
  vorbis_staticbook_clear(s);
  return(-1);
}

/* vorbis_book_encode. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054d000
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  oggpack_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

/* vorbis_book_decode. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054d050
long vorbis_book_decode(codebook *book, oggpack_buffer *b){
  long ptr=0;
  decode_aux *t=book->decode_tree;
  int lok = oggpack_look(b, t->tabn);

  if (lok >= 0) {
    ptr = t->tab[lok];
    oggpack_adv(b, t->tabl[lok]);
    if (ptr <= 0)
      return -ptr;
  }

  do{
    switch((int)oggpack_read1(b)){
    case 0:
      ptr=t->ptr0[ptr];
      break;
    case 1:
      ptr=t->ptr1[ptr];
      break;
    case -1:
      return(-1);
    }
  }while(ptr>0);
  return(-ptr);
}

/* vorbis_book_decodevs_add. STATE: BurgerLib's libvorbis 1.0rc3, entry[] and t[] from one
   block sized by a byte count (MATCH, byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x0054d110
long vorbis_book_decodevs_add(codebook *book,float *a,oggpack_buffer *b,int n){
  int step=n/book->dim;
  if(step){
    long bytes=step*sizeof(long);
    long *entry = (long *)_ogg_malloc(bytes*2);
    if(entry){
      int i,j,o;
      float **t = (float **)((char *)entry+bytes);

      for (i = 0; i < step; i++) {
	entry[i]=vorbis_book_decode(book,b);
	if(entry[i]==-1){
	  _ogg_free(entry);
	  return(-1);
	}
	t[i] = book->valuelist+entry[i]*book->dim;
      }
      for(i=0,o=0;i<book->dim;i++,o+=step)
	for (j=0;j<step;j++)
	  a[o+j]+=t[j][i];
      _ogg_free(entry);
    }
  }
  return(0);
}

/* vorbis_book_decodev_add. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054d3a0
long vorbis_book_decodev_add(codebook *book,float *a,oggpack_buffer *b,int n){
  int i,j,entry;
  float *t;

  if(book->dim>8){
    for(i=0;i<n;){
      entry = vorbis_book_decode(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      for (j=0;j<book->dim;)
	a[i++]+=t[j++];
    }
  }else{
    for(i=0;i<n;){
      entry = vorbis_book_decode(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      j=0;
      switch((int)book->dim){
      case 8:
	a[i++]+=t[j++];
      case 7:
	a[i++]+=t[j++];
      case 6:
	a[i++]+=t[j++];
      case 5:
	a[i++]+=t[j++];
      case 4:
	a[i++]+=t[j++];
      case 3:
	a[i++]+=t[j++];
      case 2:
	a[i++]+=t[j++];
      case 1:
	a[i++]+=t[j++];
      case 0:
	break;
      }
    }
  }
  return(0);
}

/* vorbis_book_decodev_set. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054d520
long vorbis_book_decodev_set(codebook *book,float *a,oggpack_buffer *b,int n){
  int i,j,entry;
  float *t;

  for(i=0;i<n;){
    entry = vorbis_book_decode(book,b);
    if(entry==-1)return(-1);
    t     = book->valuelist+entry*book->dim;
    for (j=0;j<book->dim;)
      a[i++]=t[j++];
  }
  return(0);
}

/* vorbis_book_decodevv_add. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0054d5b0
long vorbis_book_decodevv_add(codebook *book,float **a,long offset,int ch,
			      oggpack_buffer *b,int n){
  long i,j,entry;
  int chptr=0;

  for(i=offset/ch;i<(offset+n)/ch;){
    entry = vorbis_book_decode(book,b);
    if(entry==-1)return(-1);
    {
      const float *t = book->valuelist+entry*book->dim;
      for (j=0;j<book->dim;j++){
	a[chptr++][i]+=t[j];
	if(chptr==ch){
	  chptr=0;
	  i++;
	}
      }
    }
  }
  return(0);
}
