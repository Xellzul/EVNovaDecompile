// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); here data only, and C++ so that the export bundles it names link by their C++ names; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); data only, so it rides its library's line: listing-identical; BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; data only, no function to gate); Research/notes/burgervorbis.md
// LINK: after window; data EVNOVA 0x005832d4
/* registry.c -- libvorbis 1.0rc3 lib/registry.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. Stock in BurgerLib's copy (the four arrays at .data 0x005832d4); altered: comments and residue_free_info (not linked) removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "registry-api.h"
#include "misc.h"

extern vorbis_func_time      time0_exportbundle;
extern vorbis_func_floor     floor0_exportbundle;
extern vorbis_func_floor     floor1_exportbundle;
extern vorbis_func_residue   residue0_exportbundle;
extern vorbis_func_residue   residue1_exportbundle;
extern vorbis_func_residue   residue2_exportbundle;
extern vorbis_func_mapping   mapping0_exportbundle;

vorbis_func_time      *_time_P[]={
  &time0_exportbundle,
};

vorbis_func_floor     *_floor_P[]={
  &floor0_exportbundle,
  &floor1_exportbundle,
};

vorbis_func_residue   *_residue_P[]={
  &residue0_exportbundle,
  &residue1_exportbundle,
  &residue2_exportbundle,
};

vorbis_func_mapping   *_mapping_P[]={
  &mapping0_exportbundle,
};
