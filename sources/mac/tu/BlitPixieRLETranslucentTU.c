/* BlitPixieRLETranslucentTU.c -- the mac 1.1.1 BlitPixieRLETranslucent.o. Notes: Research/notes/spriteworld-mac.md */
#pragma optimization_level 4
#ifdef MAC1010
#include "../../lib/blitpixierletranslucent.c"
#include "../../lib/blitpixierleaddover.c"
#include "../../lib/blitpixierletranslucent8.c"
#include "../../lib/blitpixierleaddover8.c"
#define MAC_RT_PICK 1
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#define MAC_RT_PICK 2
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#define MAC_RT_PICK 3
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#define MAC_RT_PICK 4
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#include "../../lib/fixpoweroftwo.c"
#include "../../lib/blitpixietranslucentcopy.c"
#include "../../lib/blitpixieaddovercopy.c"
#include "../../lib/blitpixietranslucentcopy8.c"
#include "../../lib/blitpixietranslucentclearcopy8.c"
#include "../../lib/blitpixietranslucentbiascopy8.c"
#include "../../lib/blitpixieaddovercopy8.c"
#include "../../lib/construct8bitalphatable.c"
#include "../../lib/dispose8bitalphatable.c"
#else
#include "../../lib/blitpixietranslucentcopy.c"
#include "../../lib/blitpixierletranslucent.c"
#include "../../lib/blitpixieaddovercopy.c"
#include "../../lib/fixpoweroftwo.c"
#include "../../lib/blitpixietranslucentbiascopy8.c"
#include "../../lib/blitpixietranslucentcopy8.c"
#include "../../lib/blitpixieaddovercopy8.c"
#define MAC_RT_PICK 1
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#include "../../lib/blitpixietranslucentclearcopy8.c"
#include "../../lib/construct8bitalphatable.c"
#include "../../lib/dispose8bitalphatable.c"
#include "../../lib/blitpixierletranslucent8.c"
#define MAC_RT_PICK 3
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#include "../../lib/blitpixierleaddover8.c"
#define MAC_RT_PICK 4
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#include "../../lib/blitpixierleaddover.c"
#define MAC_RT_PICK 2
#include "../../lib/blitpixierletranslucentclipped.c"
#undef MAC_RT_PICK
#endif
RGBColor systemColors[256];
