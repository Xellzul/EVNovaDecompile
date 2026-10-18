/* zconf.h -- zlib 1.2.1 configuration, Copyright (C) 1995-2003 Jean-loup Gailly; zlib
   licence in zlib.h. Altered: reduced to the Win32 STDC static-library path the exe was
   built with. Notes: Research/notes/zdeflate.md */

#ifndef ZCONF_H
#define ZCONF_H

#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#  define WIN32
#endif

#ifndef STDC
#  define STDC
#endif

#ifndef MAX_MEM_LEVEL
#  define MAX_MEM_LEVEL 9
#endif

#ifndef MAX_WBITS
#  define MAX_WBITS   15
#endif

#ifndef OF
#  define OF(args)  args
#endif

#define ZEXTERN extern
#define ZEXPORT
#define ZEXPORTVA
#define FAR

typedef unsigned char  Byte;
typedef unsigned int   uInt;
typedef unsigned long  uLong;
typedef Byte  FAR Bytef;
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

typedef void const *voidpc;
typedef void FAR   *voidpf;
typedef void       *voidp;

#define z_off_t  long

#endif
