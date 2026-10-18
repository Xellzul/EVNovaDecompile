/*
 * libogg 1.0rc3, include/ogg/ogg.h. THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001 by
 * the XIPHOPHORUS Company http://www.xiph.org/
 *
 * This copy was altered for the EV Nova decompilation: it is BurgerLib's (Research/notes/
 * burgervorbis.md: oggpack_buffer reordered and unsigned, bit counts unsigned), and comments
 * and the #if branches the build does not take were removed.
 * libogg's COPYING follows unaltered.
 *
 * Copyright (c) 2001, Xiphophorus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiphophorus nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "os_types.h"

typedef struct {
  unsigned char *ptr;
  unsigned char *buffer;
  unsigned long endbyte;
  unsigned long storage;
  unsigned long endbit;
} oggpack_buffer;

typedef struct {
  unsigned char *header;
  unsigned char *body;
  long header_len;
  long body_len;
} ogg_page;

typedef struct {
  unsigned char   *body_data;
  long    body_storage;
  long    body_fill;
  long    body_returned;

  int     *lacing_vals;
  ogg_int64_t *granule_vals;

  long    lacing_storage;
  long    lacing_fill;
  long    lacing_packet;
  long    lacing_returned;

  unsigned char    header[282];
  int              header_fill;

  int     e_o_s;

  int     b_o_s;

  long    serialno;
  long    pageno;
  ogg_int64_t  packetno;

  ogg_int64_t   granulepos;

} ogg_stream_state;

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  ogg_int64_t  granulepos;

  ogg_int64_t  packetno;

} ogg_packet;

typedef struct {
  unsigned char *data;
  int storage;
  int fill;
  int returned;

  int unsynced;
  int headerbytes;
  int bodybytes;
} ogg_sync_state;

extern void  oggpack_writeinit(oggpack_buffer *b);
extern void  oggpack_reset(oggpack_buffer *b);
extern void  oggpack_writeclear(oggpack_buffer *b);
extern void  oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);
extern void  oggpack_write(oggpack_buffer *b,unsigned long value,unsigned int bits);
extern long  oggpack_look(oggpack_buffer *b,unsigned int bits);
extern long  oggpack_look_huff(oggpack_buffer *b,int bits);
extern long  oggpack_look1(oggpack_buffer *b);
extern void  oggpack_adv(oggpack_buffer *b,unsigned int bits);
extern int   oggpack_adv_huff(oggpack_buffer *b,int bits);
extern void  oggpack_adv1(oggpack_buffer *b);
extern long  oggpack_read(oggpack_buffer *b,unsigned int bits);
extern long  oggpack_read1(oggpack_buffer *b);
extern long  oggpack_bytes(oggpack_buffer *b);
extern long  oggpack_bits(oggpack_buffer *b);
extern unsigned char *oggpack_get_buffer(oggpack_buffer *b);

extern int      ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op);
extern int      ogg_stream_pageout(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_flush(ogg_stream_state *os, ogg_page *og);

extern int      ogg_sync_init(ogg_sync_state *oy);
extern int      ogg_sync_clear(ogg_sync_state *oy);
extern int      ogg_sync_reset(ogg_sync_state *oy);
extern int	ogg_sync_destroy(ogg_sync_state *oy);

extern char    *ogg_sync_buffer(ogg_sync_state *oy, long size);
extern int      ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern long     ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
extern int      ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
extern int      ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);

extern int      ogg_stream_init(ogg_stream_state *os,int serialno);
extern void     ogg_stream_clear(ogg_stream_state *os);
extern int      ogg_stream_reset(ogg_stream_state *os);
extern int      ogg_stream_destroy(ogg_stream_state *os);
extern int      ogg_stream_eos(ogg_stream_state *os);

extern void     ogg_page_checksum_set(ogg_page *og);

extern int      ogg_page_version(ogg_page *og);
extern int      ogg_page_continued(ogg_page *og);
extern int      ogg_page_bos(ogg_page *og);
extern int      ogg_page_eos(ogg_page *og);
extern ogg_int64_t  ogg_page_granulepos(ogg_page *og);
extern int      ogg_page_serialno(ogg_page *og);
extern long     ogg_page_pageno(ogg_page *og);
extern int      ogg_page_packets(ogg_page *og);

extern void     ogg_packet_clear(ogg_packet *op);

#ifdef __cplusplus
}
#endif

#endif
