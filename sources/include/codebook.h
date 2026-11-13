/* codebook.h -- libvorbis 1.0rc3 lib/codebook.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_CODEBOOK_H_
#define _V_CODEBOOK_H_

#include "ogg.h"

typedef struct static_codebook{
  int32_t   dim;
  int32_t   entries;
  int32_t  *lengthlist;

  int    maptype;

  int32_t     q_min;
  int32_t     q_delta;
  int      q_quant;
  int      q_sequencep;

  int32_t     *quantlist;

  struct encode_aux_nearestmatch *nearest_tree;
  struct encode_aux_threshmatch  *thresh_tree;
  struct encode_aux_pigeonhole  *pigeon_tree;

  int allocedp;
} static_codebook;

typedef struct encode_aux_nearestmatch{

  int32_t   *ptr0;
  int32_t   *ptr1;

  int32_t   *p;
  int32_t   *q;
  int32_t   aux;
  int32_t   alloc;
} encode_aux_nearestmatch;

typedef struct encode_aux_threshmatch{
  float *quantthresh;
  int32_t   *quantmap;
  int     quantvals;
  int     threshvals;
} encode_aux_threshmatch;

typedef struct encode_aux_pigeonhole{
  float min;
  float del;

  int  mapentries;
  int  quantvals;
  int32_t *pigeonmap;

  int32_t fittotal;
  int32_t *fitlist;
  int32_t *fitmap;
  int32_t *fitlength;
} encode_aux_pigeonhole;

typedef struct decode_aux{
  int32_t   *tab;
  int    *tabl;
  int    tabn;

  int32_t   *ptr0;
  int32_t   *ptr1;
  int32_t   aux;
} decode_aux;

typedef struct codebook{
  int32_t dim;
  int32_t entries;
  const static_codebook *c;

  float  *valuelist;
  int32_t   *codelist;
  struct decode_aux *decode_tree;

  int32_t zeroentry;
} codebook;

#ifdef __cplusplus
extern "C" {
#endif

extern void vorbis_staticbook_clear(static_codebook *b);
extern void vorbis_staticbook_destroy(static_codebook *b);
extern int vorbis_book_init_encode(codebook *dest,const static_codebook *source);
extern int vorbis_book_init_decode(codebook *dest,const static_codebook *source);
extern void vorbis_book_clear(codebook *b);

extern float *_book_unquantize(const static_codebook *b);
extern float *_book_logdist(const static_codebook *b,float *vals);
extern float _float32_unpack(int32_t val);
extern int32_t   _float32_pack(float val);
extern int  _best(codebook *book, float *a, int step);
extern int _ilog(unsigned int v);
extern int32_t _book_maptype1_quantvals(const static_codebook *b);

extern int vorbis_book_besterror(codebook *book,float *a,int step,int addmul);
extern int32_t vorbis_book_codeword(codebook *book,int entry);
extern int32_t vorbis_book_codelen(codebook *book,int entry);

extern int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *b);
extern int vorbis_staticbook_unpack(oggpack_buffer *b,static_codebook *c);

extern int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);
extern int vorbis_book_errorv(codebook *book, float *a);
extern int vorbis_book_encodev(codebook *book, int best,float *a,
			       oggpack_buffer *b);
extern int vorbis_book_encodevs(codebook *book, float *a, oggpack_buffer *b,
				int step,int stagetype);

extern int32_t vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern int32_t vorbis_book_decodevs_add(codebook *book, float *a,
				     oggpack_buffer *b,int n);
extern int32_t vorbis_book_decodev_set(codebook *book, float *a,
				    oggpack_buffer *b,int n);
extern int32_t vorbis_book_decodev_add(codebook *book, float *a,
				    oggpack_buffer *b,int n);
extern int32_t vorbis_book_decodevv_add(codebook *book, float **a,
				     int32_t off,int ch,
				    oggpack_buffer *b,int n);

#ifdef __cplusplus
}
#endif

#endif
