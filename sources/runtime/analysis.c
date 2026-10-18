// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); no body here the band census can class, so it rides its library's line: listing-identical; BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; no veto here, so the library's one line); Research/notes/burgervorbis.md
/* analysis.c -- libvorbis 1.0rc3 lib/analysis.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. Stock in BurgerLib's copy; altered: comments, the ANALYSIS branch (not built: the exe's _analysis_output is a bare ret) and what the exe does not link (vorbis_analysis, _analysis_output_always) removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "scales.h"
#include "os.h"
#include "misc.h"

int analysis_noisy=1;

/* _analysis_output. STATE: genuine libvorbis 1.0rc3 (runtime band, a floor).
   OPEN: none. */
// FUNCTION: EVNOVA 0x0055bd00
void _analysis_output(char *base,int i,float *v,int n,int bark,int dB){
}
