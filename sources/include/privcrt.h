/* privcrt.h -- the character-class bits of the private C runtime that crt4.c, jpegcrt.c, pngcrt.c and
   zlibcrt.c each carry a copy of. */
#ifndef PRIVCRT_H
#define PRIVCRT_H

#define CT_SPACE  1
#define CT_DIGIT  2
#define CT_UXDIG  4     /* A-F */
#define CT_UPPER  8     /* G-Z */
#define CT_LXDIG  16    /* a-f */
#define CT_LOWER  32    /* g-z */

#endif /* PRIVCRT_H */
