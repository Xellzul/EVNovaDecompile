// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; no veto here, so the library's one line); Research/notes/burgervorbis.md
/* iir.c -- libvorbis 1.0rc3 lib/iir.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (IIR_clear renamed IIRStateDestroy, FastMemSet); altered: comments, the self-test and the functions the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "iir.h"

/* IIRStateDestroy (rc3 IIR_clear). STATE: BurgerLib's libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055bd10
void IIRStateDestroy(IIR_state *s){
  if(s){
    _ogg_free(s->coeff_A);
    _ogg_free(s->coeff_B);
    _ogg_free(s->z_A);
    FastMemSet(s,0,sizeof(*s));
  }
}
