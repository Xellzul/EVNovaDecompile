/* EVNovaPrefix.h -- the mac build's prefix header, force-included by the mac compile line
   (tools/mac/regate_all.py mac_cflags). Notes: Research/mac-1.1.1-feasibility.md section 9,
   Research/notes/macport.md */
/* The mac build takes the Carbon and QuickTime types and prototypes from the real 10.4u SDK;
   the win emulation in macport.h and gametypes.h steps aside (#ifndef MACBUILD) */
#include <Carbon/Carbon.h>
#ifndef RT3_LIBRARY
#include <QuickTime/QuickTime.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* the win port's calling-convention keywords */
#define __stdcall
#define __cdecl
#define __declspec(x)

/* the win headers name the C library's FILE `struct _FILE' (MSL); the SDK's is struct __sFILE */
#define _FILE __sFILE

/* win placeholder names of C library calls: FUN_004eed20 is MSL's out-of-line sqrtf, which the win
   tree cannot call sqrtf because MSL's <math.h> inlines it (Research/notes/burgermisc.md) */
#define FUN_004eed20 sqrtf

/* the win port reads the tick count through BurgerLib; the mac engine calls the Toolbox */
#define ReadTick TickCount

/* gcc's __FILE__ for the engine sources on the 1.1.1 build machine, as the SWAssertFail sites
   spell it (the win port's mwcc passed the bare file name) */
#define kEngineSourceDir "/Users/jerome/Ambrosia/ev_nova1.1.0/sources/3rdparty/ev_nova/source/"
