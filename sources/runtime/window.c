// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; C++ with bool support off, BurgerLib's own build of it (mangled statics in the archive members; float fabs/floor overloads and int-typed && results in the exe); runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); BurgerLib's Ogg/Vorbis is one build unit in forward source order with its small same-TU callees inlined (framing.c's line; no veto here, so the library's one line); Research/notes/burgervorbis.md
/* window.c -- libvorbis 1.0rc3 lib/window.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (allocates only for type 0, checks the allocation, and computes the slopes with a running phase and one walking pointer); altered: comments removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <math.h>
#include "os.h"
#include "misc.h"
#include "window.h"

/* _vorbis_window. STATE: BurgerLib's rewrite of libvorbis 1.0rc3's: one walking pointer,
   running float phase, one NULL return (MATCH, byte-identical). OPEN: none. */
// FUNCTION: EVNOVA 0x00547500
float *_vorbis_window(int type, int window,int left,int right){
  float *ret;
  int leftbegin,rightbegin,i;
  float x,step,fi,fr;
  float *p;

  if(type==0){
    ret=(float *)_ogg_calloc(window,sizeof(*ret));
    if(ret){
      leftbegin=window/4-left/2;
      rightbegin=window-window/4-right/2;
      p=ret+leftbegin;

      step=1.f/left*(M_PI/2.f);
      fi=.5f;
      for(i=0;i<left;i++){
	x=fi*step;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	*p++=x;
	fi+=1.f;
      }

      for(i=leftbegin+left;i<rightbegin;i++)
	*p++=1.f;

      fr=right;
      fi=fr-.5f;
      step=1.f/fr*(M_PI/2.f);
      for(i=0;i<right;i++){
	x=fi*step;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	*p++=x;
	fi-=1.f;
      }
      return(ret);
    }
  }
  return(NULL);
}
