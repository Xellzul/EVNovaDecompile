/* jconfig.h -- IJG libjpeg 6b jconfig.vc (Microsoft Visual C++ on Windows), the configuration the exe was built with, plus NO_GETENV (install.doc); licence in jpeglib.h; altered: comments and the #if branches this build does not take removed. Notes: Research/notes/jpeg.md */
#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT

#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

#ifndef __RPCNDR_H__
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED
#define NO_GETENV

#endif
