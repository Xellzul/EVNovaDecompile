/* os.h -- libvorbis 1.0rc3 lib/os.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments and the #if branches this build (_WIN32, not __GNUC__) does not take removed. Notes: Research/notes/burgervorbis.md */
#ifndef _OS_H
#define _OS_H

#include <math.h>
#include "os_types.h"

#ifndef _V_IFDEFJAIL_H_
#  define _V_IFDEFJAIL_H_

#  define STIN static __inline

#ifndef M_PI
#  define M_PI (3.1415926536f)
#endif

#  include <malloc.h>
#  define rint(x)   (floor((x)+0.5f))
#  define NO_FLOAT_MATH_LIB
#  define FAST_HYPOT(a, b) sqrt((a)*(a) + (b)*(b))

#endif

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

#ifndef max
#  define max(x,y)  ((x)<(y)?(y):(x))
#endif

#  define VORBIS_FPU_CONTROL

typedef ogg_int16_t vorbis_fpu_control;

static __inline int vorbis_ftoi(double f){
	int i;
	__asm{
		fld f
		fistp i
	}
	return i;
}

static __inline void vorbis_fpu_setround(vorbis_fpu_control *fpu){
}

static __inline void vorbis_fpu_restore(vorbis_fpu_control fpu){
}

#endif
