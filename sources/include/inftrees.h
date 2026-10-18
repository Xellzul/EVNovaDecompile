/* inftrees.h -- zlib 1.2.1 inflate table builder interface, Copyright (C) 1995-2003 Mark
   Adler; zlib licence in zlib.h. Altered: comments removed, include guard added. */

#ifndef INFTREES_H
#define INFTREES_H

typedef struct {
    unsigned char op;
    unsigned char bits;
    unsigned short val;
} code;

/* BUG: too small for the largest dynamic trees; a crafted stream overruns codes[] (CVE-2005-1849) */
#define ENOUGH 1440
#define MAXD 154

typedef enum {
    CODES,
    LENS,
    DISTS
} codetype;

extern int inflate_table OF((codetype type, unsigned short FAR *lens,
                             unsigned codes, code FAR * FAR *table,
                             unsigned FAR *bits, unsigned short FAR *work));

#endif
