// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); no body here the band census can class, so it rides its library's line: listing-identical; BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; no veto here, so the library's one line); Research/notes/burgervorbis.md
/* time0.c -- libvorbis 1.0rc3 lib/time0.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (the placeholder info and look are two static words, foo and foo2, not ""); altered: comments removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"

struct vorbis_look_time{
  int dummy;
};

static vorbis_info_time foo;
static vorbis_look_time foo2;

/* time0_pack. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552e50
static void time0_pack (vorbis_info_time *i,oggpack_buffer *opb){
}

/* time0_unpack. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552e60
static vorbis_info_time *time0_unpack (vorbis_info *vi,oggpack_buffer *opb){
  return &foo;
}

/* time0_copy_info. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552e70
static vorbis_info_time *time0_copy_info (vorbis_info_time *vi){
  return &foo;
}

/* time0_look. STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552e80
static vorbis_look_time *time0_look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_time *i){
  return &foo2;
}

/* time0_free_info. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552e90
static void time0_free_info(vorbis_info_time *i){
}

/* time0_free_look. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552ea0
static void time0_free_look(vorbis_look_time *i){
}

/* time0_forward. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552eb0
static int time0_forward(vorbis_block *vb,vorbis_look_time *i,
		    float *in,float *out){
  return(0);
}

/* time0_inverse. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x00552ec0
static int time0_inverse(vorbis_block *vb,vorbis_look_time *i,
		    float *in,float *out){
  return(0);
}

vorbis_func_time time0_exportbundle={
  &time0_pack,&time0_unpack,&time0_look,&time0_copy_info,&time0_free_info,
  &time0_free_look,&time0_forward,&time0_inverse
};
