// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; dropping -inline auto loses MATCHes here; inlined: _float32_unpack, _dist and vorbis_book_clear); Research/notes/burgervorbis.md
/* sharedbook.c -- libvorbis 1.0rc3 lib/sharedbook.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (allocation checks, unconditional frees, FastMemSet); altered: comments and the functions the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ogg.h"
#include "os.h"
#include "codec.h"
#include "codebook.h"
#include "scales.h"

/* _ilog. STATE: BurgerLib's libvorbis 1.0rc3 (a guarded do-while), MATCH.
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544280
int _ilog(unsigned int v){
  int ret=0;
  if(v){
    do{
      ret++;
      v>>=1;
    }while(v);
  }
  return(ret);
}

#define VQ_FEXP 10
#define VQ_FMAN 21
#define VQ_FEXP_BIAS 768

/* not linked */
long _float32_pack(float val){
  int sign=0;
  long exp;
  long mant;
  if(val<0){
    sign=0x80000000;
    val= -val;
  }
  exp= floor(log(val)/log(2));
  mant=rint(ldexp(val,(VQ_FMAN-1)-exp));
  exp=(exp+VQ_FEXP_BIAS)<<VQ_FMAN;
  return(sign|exp|mant);
}

float _float32_unpack(long val){
  double mant=val&0x1fffff;
  int    sign=val&0x80000000;
  long   exp =(val&0x7fe00000L)>>VQ_FMAN;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-(VQ_FMAN-1)-VQ_FEXP_BIAS));
}

/* _make_words. STATE: BurgerLib's libvorbis 1.0rc3, the body under if(r) so a failed
   allocation leaves through return(r) (MATCH, byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x005442a0
long *_make_words(long *l,long n){
  long i,j;
  long marker[33];
  long *r=(long *)_ogg_malloc(n*sizeof(*r));
  if(r){
    FastMemSet(marker,0,sizeof(marker));

    for(i=0;i<n;i++){
      long length=l[i];
      if(length>0){
	long entry=marker[length];

	if(length<32 && (entry>>length)){
	  _ogg_free(r);
	  return(NULL);
	}
	r[i]=entry;

	{
	  for(j=length;j>0;j--){

	    if(marker[j]&1){
	      if(j==1)
		marker[1]++;
	      else
		marker[j]=marker[j-1]<<1;
	      break;
	    }
	    marker[j]++;
	  }
	}

	for(j=length+1;j<33;j++)
	  if((marker[j]>>1) == entry){
	    entry=marker[j];
	    marker[j]=marker[j-1]<<1;
	  }else
	    break;
      }
    }

    for(i=0;i<n;i++){
      long temp=0;
      for(j=0;j<l[i];j++){
	temp<<=1;
	temp|=(r[i]>>j)&1;
      }
      r[i]=temp;
    }
  }

  return(r);
}

/* _make_decode_tree. STATE: BurgerLib's libvorbis 1.0rc3, MATCH, byte-identical (835 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544500
decode_aux *_make_decode_tree(codebook *c){
  const static_codebook *s=c->c;
  long top=0,i,j,n;
  decode_aux *t=(decode_aux *)_ogg_malloc(sizeof(*t));
  if(t){
    long *ptr0=(long *)_ogg_calloc(c->entries*2,sizeof(*ptr0));
    t->ptr0=ptr0;
    if(ptr0){
      long *ptr1=(long *)_ogg_calloc(c->entries*2,sizeof(*ptr1));
      t->ptr1=ptr1;
      if(ptr1){
        long *codelist=_make_words(s->lengthlist,s->entries);
        if(codelist){
          t->aux=c->entries*2;

          for(i=0;i<c->entries;i++){
            if(s->lengthlist[i]>0){
              long ptr=0;
              for(j=0;j<s->lengthlist[i]-1;j++){
                int bit=(codelist[i]>>j)&1;
                if(!bit){
                  if(!ptr0[ptr])
                    ptr0[ptr]= ++top;
                  ptr=ptr0[ptr];
                }else{
                  if(!ptr1[ptr])
                    ptr1[ptr]= ++top;
                  ptr=ptr1[ptr];
                }
              }
              if(!((codelist[i]>>j)&1))
                ptr0[ptr]=-i;
              else
                ptr1[ptr]=-i;
            }
          }
          _ogg_free(codelist);

          t->tabn = _ilog(c->entries)-4;
          if(t->tabn<5)t->tabn=5;
          n = 1<<t->tabn;
          t->tab = (long *)_ogg_malloc(n*sizeof(*t->tab));
          t->tabl = (int *)_ogg_malloc(n*sizeof(*t->tabl));
          if(t->tab && t->tabl){
            for (i = 0; i < n; i++) {
              long p = 0;
              for (j = 0; j < t->tabn && (p > 0 || j == 0); j++) {
                if (i & (1 << j))
                  p = ptr1[p];
                else
                  p = ptr0[p];
              }
              t->tab[i] = p;
              t->tabl[i] = j;
            }
            return(t);
          }
          _ogg_free(t->tab);
          _ogg_free(t->tabl);
        }
        _ogg_free(ptr1);
      }
      _ogg_free(ptr0);
    }
    _ogg_free(t);
  }
  return(NULL);
}

/* _book_maptype1_quantvals. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544850
long _book_maptype1_quantvals(const static_codebook *b){
  long vals=floor(pow((float)b->entries,1.f/b->dim));

  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<b->dim;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=b->entries && acc1>b->entries){
      return(vals);
    }else{
      if(acc>b->entries){
	vals--;
      }else{
	vals++;
      }
    }
  }
}

/* _book_unquantize. STATE: BurgerLib's libvorbis 1.0rc3, the allocation checked, a failure
   joining the final return(NULL) (MATCH, byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x005449c0
float *_book_unquantize(const static_codebook *b){
  long j,k;
  if(b->maptype==1 || b->maptype==2){
    int quantvals;
    float mindel=_float32_unpack(b->q_min);
    float delta=_float32_unpack(b->q_delta);
    float *r=(float *)_ogg_calloc(b->entries*b->dim,sizeof(*r));

    if(r){
      switch(b->maptype){
      case 1:
	quantvals=_book_maptype1_quantvals(b);
	for(j=0;j<b->entries;j++){
	  float last=0.f;
	  int indexdiv=1;
	  for(k=0;k<b->dim;k++){
	    int index= (j/indexdiv)%quantvals;
	    float val=b->quantlist[index];
	    val=fabs(val)*delta+mindel+last;
	    if(b->q_sequencep)last=val;
	    r[j*b->dim+k]=val;
	    indexdiv*=quantvals;
	  }
	}
	break;
      case 2:
	for(j=0;j<b->entries;j++){
	  float last=0.f;
	  for(k=0;k<b->dim;k++){
	    float val=b->quantlist[j*b->dim+k];
	    val=fabs(val)*delta+mindel+last;
	    if(b->q_sequencep)last=val;
	    r[j*b->dim+k]=val;
	  }
	}
	break;
      }
      return(r);
    }
  }
  return(NULL);
}

/* vorbis_staticbook_clear. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544c00
void vorbis_staticbook_clear(static_codebook *b){
  if(b->allocedp){
    _ogg_free(b->quantlist);
    _ogg_free(b->lengthlist);
    if(b->nearest_tree){
      _ogg_free(b->nearest_tree->ptr0);
      _ogg_free(b->nearest_tree->ptr1);
      _ogg_free(b->nearest_tree->p);
      _ogg_free(b->nearest_tree->q);
      FastMemSet(b->nearest_tree,0,sizeof(*b->nearest_tree));
      _ogg_free(b->nearest_tree);
    }
    if(b->thresh_tree){
      _ogg_free(b->thresh_tree->quantthresh);
      _ogg_free(b->thresh_tree->quantmap);
      FastMemSet(b->thresh_tree,0,sizeof(*b->thresh_tree));
      _ogg_free(b->thresh_tree);
    }

    FastMemSet(b,0,sizeof(*b));
  }
}

/* vorbis_staticbook_destroy. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544d10
void vorbis_staticbook_destroy(static_codebook *b){
  if(b->allocedp){
    vorbis_staticbook_clear(b);
    _ogg_free(b);
  }
}

/* vorbis_book_clear. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544d40
void vorbis_book_clear(codebook *b){
  if(b->decode_tree){
    _ogg_free(b->decode_tree->tab);
    _ogg_free(b->decode_tree->tabl);

    _ogg_free(b->decode_tree->ptr0);
    _ogg_free(b->decode_tree->ptr1);
    FastMemSet(b->decode_tree,0,sizeof(*b->decode_tree));
    _ogg_free(b->decode_tree);
  }
  _ogg_free(b->valuelist);
  _ogg_free(b->codelist);
  FastMemSet(b,0,sizeof(*b));
}

/* vorbis_book_init_encode. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544df0
int vorbis_book_init_encode(codebook *c,const static_codebook *s){
  long j,k;
  FastMemSet(c,0,sizeof(*c));
  c->c=s;
  c->entries=s->entries;
  c->dim=s->dim;
  c->codelist=_make_words(s->lengthlist,s->entries);
  c->valuelist=_book_unquantize(s);

  c->zeroentry=-1;
  if(c->valuelist){
    for(j=0;j<s->entries;j++){
      int flag=1;
      for(k=0;k<s->dim;k++){
	if(fabs(c->valuelist[j*s->dim+k])>1e-12f){
	  flag=0;
	  break;
	}
      }
      if(flag)
	c->zeroentry=j;
    }
  }

  return(0);
}

/* vorbis_book_init_decode. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00544f10
int vorbis_book_init_decode(codebook *c,const static_codebook *s){
  FastMemSet(c,0,sizeof(*c));
  c->c=s;
  c->entries=s->entries;
  c->dim=s->dim;
  c->valuelist=_book_unquantize(s);
  c->decode_tree=_make_decode_tree(c);
  if(c->decode_tree==NULL)goto err_out;
  return(0);
 err_out:
  vorbis_book_clear(c);
  return(-1);
}

static float _dist(int el,float *ref, float *b,int step){
  int i;
  float acc=0.f;
  for(i=0;i<el;i++){
    float val=(ref[i]-b[i*step]);
    acc+=val*val;
  }
  return(acc);
}

/* _best. STATE: BurgerLib's libvorbis 1.0rc3 (.5f), MATCH, byte-identical (2349 B).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00545030
int _best(codebook *book, float *a, int step){
  encode_aux_nearestmatch *nt=book->c->nearest_tree;
  encode_aux_threshmatch *tt=book->c->thresh_tree;
  encode_aux_pigeonhole *pt=book->c->pigeon_tree;
  int dim=book->dim;
  int ptr=0,k,o;

  if(tt){
    int index=0;

    for(k=0,o=step*(dim-1);k<dim;k++,o-=step){
      int i;

      for(i=0;i<tt->threshvals-1;i++)
	if(a[o]<tt->quantthresh[i])break;

      index=(index*tt->quantvals)+tt->quantmap[i];
    }

    if(book->c->lengthlist[index]>0)
      return(index);
  }

  if(pt){
    const static_codebook *c=book->c;
    int i,besti=-1;
    float best=0.f;
    int entry=0;

    if(c->q_sequencep){
      int pv;
      long mul=1;
      float qlast=0;
      for(k=0,o=0;k<dim;k++,o+=step){
	pv=(int)((a[o]-qlast-pt->min)/pt->del);
	if(pv<0 || pv>=pt->mapentries)break;
	entry+=pt->pigeonmap[pv]*mul;
	mul*=pt->quantvals;
	qlast+=pv*pt->del+pt->min;
      }
    }else{
      for(k=0,o=step*(dim-1);k<dim;k++,o-=step){
	int pv=(int)((a[o]-pt->min)/pt->del);
	if(pv<0 || pv>=pt->mapentries)break;
	entry=entry*pt->quantvals+pt->pigeonmap[pv];
      }
    }

    if(k==dim && pt->fitlength[entry]){
      long *list=pt->fitlist+pt->fitmap[entry];
      for(i=0;i<pt->fitlength[entry];i++){
	float this_=_dist(dim,book->valuelist+list[i]*dim,a,step);
	if(besti==-1 || this_<best){
	  best=this_;
	  besti=list[i];
	}
      }

      return(besti);
    }
  }

  if(nt){
    while(1){
      float c=0.f;
      float *p=book->valuelist+nt->p[ptr];
      float *q=book->valuelist+nt->q[ptr];

      for(k=0,o=0;k<dim;k++,o+=step)
	c+=(p[k]-q[k])*(a[o]-(p[k]+q[k])*.5f);

      if(c>0.f)
	ptr= -nt->ptr0[ptr];
      else
	ptr= -nt->ptr1[ptr];
      if(ptr<=0)break;
    }
    return(-ptr);
  }

  {
    const static_codebook *c=book->c;
    int i,besti=-1;
    float best=0.f;
    float *e=book->valuelist;
    for(i=0;i<book->entries;i++){
      if(c->lengthlist[i]>0){
	float this_=_dist(dim,e,a,step);
	if(besti==-1 || this_<best){
	  best=this_;
	  besti=i;
	}
      }
      e+=dim;
    }

    return(besti);
  }
}

/* vorbis_book_besterror. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00545960
int vorbis_book_besterror(codebook *book,float *a,int step,int addmul){
  int dim=book->dim,i,o;
  int best=_best(book,a,step);
  switch(addmul){
  case 0:
    for(i=0,o=0;i<dim;i++,o+=step)
      a[o]-=(book->valuelist+best*dim)[i];
    break;
  case 1:
    for(i=0,o=0;i<dim;i++,o+=step){
      float val=(book->valuelist+best*dim)[i];
      if(val==0){
	a[o]=0;
      }else{
	a[o]/=val;
      }
    }
    break;
  }
  return(best);
}
