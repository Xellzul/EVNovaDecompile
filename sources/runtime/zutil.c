// COMPILE: -opt level=2,intrinsics,space -inline auto,deferred -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: zlib 1.2.1 on libjpeg's line and compiler (-align 8 is object-inert on all ten zlib TUs, Research/flag-census.md); build 335 has no -proc switch, its default is generic x86; Research/runtime-compiler-ceiling.md
/* zutil.c -- zlib 1.2.1 zutil.c, Copyright (C) 1995-2003 Jean-loup Gailly; zlib licence in zlib.h; altered: comments removed (the functions the exe does not link stay, unmarked), the heap through zlibcrt.c. Notes: Research/notes/zdeflate.md */

#include "zutil.h"

struct internal_state      {int dummy;};

const char * const z_errmsg[10] = {
"need dictionary",
"stream end",
"",
"file error",
"stream error",
"data error",
"insufficient memory",
"buffer error",
"incompatible version",
""};

/* not linked: dead-stripped in the exe; kept for the TU's data */
const char * ZEXPORT zlibVersion()
{
    return ZLIB_VERSION;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
uLong ZEXPORT zlibCompileFlags()
{
    uLong flags;

    flags = 0;
    switch (sizeof(uInt)) {
    case 2:     break;
    case 4:     flags += 1;     break;
    case 8:     flags += 2;     break;
    default:    flags += 3;
    }
    switch (sizeof(uLong)) {
    case 2:     break;
    case 4:     flags += 1 << 2;        break;
    case 8:     flags += 2 << 2;        break;
    default:    flags += 3 << 2;
    }
    switch (sizeof(voidpf)) {
    case 2:     break;
    case 4:     flags += 1 << 4;        break;
    case 8:     flags += 2 << 4;        break;
    default:    flags += 3 << 4;
    }
    switch (sizeof(z_off_t)) {
    case 2:     break;
    case 4:     flags += 1 << 6;        break;
    case 8:     flags += 2 << 6;        break;
    default:    flags += 3 << 6;
    }
    return flags;
}

/* not linked: dead-stripped in the exe; kept for the TU's data */
const char * ZEXPORT zError(err)
    int err;
{
    return ERR_MSG(err);
}

/* zcalloc. STATE: MATCH 100%, exe 6 / obj 6, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x005433a6
voidpf zcalloc (opaque, items, size)
    voidpf opaque;
    unsigned items;
    unsigned size;
{
    if (opaque) items += size - size;
    return zlib_malloc(items * size);
}

/* zcfree. STATE: MATCH 100%, exe 7 / obj 7, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00543393
void  zcfree (opaque, ptr)
    voidpf opaque;
    voidpf ptr;
{
    zlib_free(ptr);
    if (opaque) return;
}
