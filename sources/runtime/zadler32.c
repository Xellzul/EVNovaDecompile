// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: zlib 1.2.1 on libjpeg's line and compiler (-align 8 is object-inert on all ten zlib TUs, Research/flag-census.md); build 335 has no -proc switch, its default is generic x86; Research/runtime-compiler-ceiling.md
/* zadler32.c -- zlib 1.2.1 adler32.c, Copyright (C) 1995-2003 Mark Adler; zlib licence in zlib.h; altered: comments removed. Notes: Research/notes/zdeflate.md */

#define ZLIB_INTERNAL
#include "zlib.h"

#define BASE 65521UL
#define NMAX 5552

#define DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

#define MOD(a) a %= BASE

/* adler32. STATE: MATCH 100%, exe 107 / obj 107, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00543278
uLong ZEXPORT adler32(adler, buf, len)
    uLong adler;
    const Bytef *buf;
    uInt len;
{
    unsigned long s1 = adler & 0xffff;
    unsigned long s2 = (adler >> 16) & 0xffff;
    int k;

    if (buf == Z_NULL) return 1L;

    while (len > 0) {
        k = len < NMAX ? (int)len : NMAX;
        len -= k;
        while (k >= 16) {
            DO16(buf);
            buf += 16;
            k -= 16;
        }
        if (k != 0) do {
            s1 += *buf++;
            s2 += s1;
        } while (--k);
        MOD(s1);
        MOD(s2);
    }
    return (s2 << 16) | s1;
}
