/* atfilestr.h -- the byte and string helpers the file tool (atfilespec*.c, atfilearchive.c)
   inlines in place of the platform calls. A TU asks for the groups it uses before including
   this: an unused static inline still takes compiler labels, which the objects carry.
   Notes: Research/notes/atfilespec.md */
#ifndef ATFILESTR_H
#define ATFILESTR_H

#include <string.h>
#include "burgertypes.h"

/* the indices of FTProcs.proc; an enum takes a label too, so it lives here with the helpers */
enum { kFTRead, kFTWrite, kFTSize, kFTSetSize, kFTPosition, kFTSeek };

#ifdef ATSTR_MOVE
/* MemoryCopy's body (atplatheap.c), which the file tool inlines */
static inline void ATMemMove(void *dst, const void *src, ULong n)
{
    if (n != 0)
        memmove(dst, src, n);
}
#endif

#ifdef ATSTR_LEN
/* StringGetLength's body (atplatstr.c), which the file tool inlines */
static inline ULong ATStrLen(const char *s)
{
    ULong n;

    n = -1;
    do {
        n++;
    } while (*s++ != 0);
    return n;
}
#endif

#ifdef ATSTR_JOIN
/* StringCopy's body (atplatstr.c), which the file tool inlines */
static inline char *ATStrCpy(char *dst, const char *src)
{
    char *start;

    start = dst;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}

/* the platform strcat body, which the file tool inlines */
static inline char *ATStrCat(char *dst, const char *src)
{
    char *start;

    start = dst;
    while (*dst++ != 0)
        ;
    dst--;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}
#endif

#ifdef ATSTR_CMP
/* StringCompare's body (atplatstr.c), which the file tool inlines */
static inline long ATStrCmp(const char *a, const char *b)
{
    ULong ca;
    ULong cb;
    const unsigned char *s1;
    const unsigned char *s2;

    s1 = (const unsigned char *)a;
    s2 = (const unsigned char *)b;
    while ((ca = *s1++) == (cb = *s2++)) {
        if (ca == 0)
            return 0;
    }
    return ca < cb ? (signed char)-1 : (signed char)1;
}

/* a Pascal string to a C string, inlined (no out-of-line copy is linked) */
static inline char *ATPToCStr(char *dst, const unsigned char *src)
{
    ULong n = *src++;
    char *start = dst;

    while (n--)
        *dst++ = *src++;
    *dst = 0;
    dst = start;
    return dst;
}

/* a C string to a Pascal string of at most 255 characters, inlined likewise */
static inline unsigned char *ATCToPStr(unsigned char *dst, const char *src)
{
    ULong n = -1;
    long c;
    unsigned char *start = dst;

    while ((dst++, ++n < 0xff) && (c = *src++) != 0)
        *dst = c;
    dst = start;
    *dst = n;
    return dst;
}
#endif

#ifdef ATSTR_JOIN
/* a C string to a Pascal string in place, at most 255 characters */
static inline unsigned char *ATCToPInPlace(char *s)
{
    ULong n;
    long next, c;
    char *p;

    if ((c = *s) != 0) {
        p = s + 1;
        n = 0;
        while (n++ < 0xfe && (next = *(unsigned char *)p) != 0) {
            *p++ = c;
            c = next;
        }
        *p = c;
        *s = n;
    }
    return (unsigned char *)s;
}
#endif

#endif
