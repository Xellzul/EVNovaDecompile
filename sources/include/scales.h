/* scales.h -- libvorbis 1.0rc3 lib/scales.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. altered: comments and the VORBIS_IEEE_FLOAT32 lookup branch (not built: the exe has no todB_LOOKUP table and pools 4.34294480f) removed. Notes: Research/notes/burgervorbis.md */
#ifndef _V_SCALES_H_
#define _V_SCALES_H_

#include <math.h>
#include "os.h"

#define todB(x)   (*(x)==0?-400.f:log(*(x)**(x))*4.34294480f)
#define todB_nn(x)   (*(x)==0.f?-400.f:log(*(x))*8.6858896f)

#define fromdB(x) (exp((x)*.11512925f))

#define toBARK(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))
#define fromBARK(z) (102.f*(z)-2.f*pow(z,2.f)+.4f*pow(z,3.f)+pow(1.46f,z)-1.f)
#define toMEL(n)    (log(1.f+(n)*.001f)*1442.695f)
#define fromMEL(m)  (1000.f*exp((m)/1442.695f)-1000.f)

#define toOC(n)     (log(n)*1.442695f-5.965784f)
#define fromOC(o)   (exp(((o)+5.965784f)*.693147f))

#endif
