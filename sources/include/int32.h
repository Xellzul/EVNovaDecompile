/* int32.h -- the 32-bit integer names: long to every original compiler, the real fixed-width types to any other. Notes: tools/transforms/README.md */
#ifndef INT32_H
#define INT32_H

#if defined(__MWERKS__)
typedef long          int32_t;
typedef unsigned long uint32_t;
#elif defined(MACBUILD) && defined(V111)
#include <stdint.h>
#define int32_t  long             /* Apple gcc's own int32_t is int; long keeps the 1.1.1 code */
#define uint32_t unsigned long
#else
#include <stdint.h>               /* any modern compiler: the real fixed-width types */
#endif

#endif
