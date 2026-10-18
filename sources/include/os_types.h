/* os_types.h -- libogg 1.0rc3 include/ogg/os_types.h, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in ogg.h. BurgerLib's copy: the allocator routed to its memory manager, a C library header first so that MSL's prefix defines __int32/__int64 for a TU whose first include is codec.h; altered: comments and the #if branches this build does not take removed. Notes: Research/notes/burgervorbis.md */
#ifndef _OS_TYPES_H
#define _OS_TYPES_H

#include <stddef.h>
#include "burger.h"

#define _ogg_malloc  AllocAPointer
#define _ogg_calloc(count,size) AllocAPointerClear((count)*(size))
#define _ogg_realloc ResizeAPointer
#define _ogg_free    DeallocAPointer

typedef __int64 ogg_int64_t;
typedef __int32 ogg_int32_t;
typedef unsigned __int32 ogg_uint32_t;
typedef __int16 ogg_int16_t;

#endif
