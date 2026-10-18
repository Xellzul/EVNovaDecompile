// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgertextbuf.c -- a text buffer that grows its handle 8 KB at a time (the prefs writer's output), and a 16-bit fill. Notes: Research/notes/burgertextbuf.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kTextGrowth 0x2000

/* StreamHandleInitPut. STATE: MATCH 100%, exe 7 / obj 7 instrs. OPEN: none */
// FUNCTION: EVNOVA 0x005271d0
void StreamHandleInitPut(TextBuilder *b)
{
    b->handle = 0;
    b->length = 0;
    b->capacity = 0;
    b->extent = 0;
    b->failed = 0;
}

/* StreamHandleDestroy. STATE: MATCH 100%, exe 13 / obj 13 instrs. OPEN: none */
// FUNCTION: EVNOVA 0x00527200
void StreamHandleDestroy(TextBuilder *b)
{
    if (b != 0) {
        DeallocAHandle(b->handle);
        b->handle = 0;
    }
}

/* StreamHandleEndSave. STATE: MATCH 100%, exe 7 / obj 7 instrs. OPEN: none */
// FUNCTION: EVNOVA 0x00527230
void StreamHandleEndSave(TextBuilder *b)
{
    if (b->failed == 0)
        b->extent = b->length;
}

/* StreamHandlePutByte. STATE: MATCH 100%, exe 42 / obj 42 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00527250
void StreamHandlePutByte(TextBuilder *b, long ch)
{
    ULong pos;
    ULong end;

    pos = b->length;
    end = pos + 1;
    for (;;) {
        if (b->capacity >= end) {
            b->length = end;
            if (b->extent < end)
                b->extent = end;
            (*b->handle)[pos] = ch;
            return;
        }
        b->capacity += kTextGrowth;
        b->handle = ResizeAHandle(b->handle, b->capacity);
        if (b->handle == 0) {
            b->capacity = 0;
            b->length = 0;
            b->extent = 0;
            b->failed = 1;
            return;
        }
    }
}

/* StreamHandlePutStringNoZero. STATE: MATCH 100%, exe 60 / obj 60 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005272e0
void StreamHandlePutStringNoZero(TextBuilder *b, const char *s)
{
    ULong n;
    ULong pos;
    ULong end;

    n = strlen(s);
    pos = b->length;
    end = pos + n;
    for (;;) {
        if (b->capacity >= end) {
            b->length = end;
            if (b->extent < end)
                b->extent = end;
            memcpy(*b->handle + pos, s, n);
            return;
        }
        b->capacity += n + kTextGrowth;
        b->handle = ResizeAHandle(b->handle, b->capacity);
        if (b->handle == 0) {
            b->capacity = 0;
            b->length = 0;
            b->extent = 0;
            b->failed = 1;
            return;
        }
    }
}

/* Fill with a 16-bit value; an odd last byte takes its low half. */
/* FastMemSet16. STATE: MATCH 100%, exe 15 / obj 15 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005273c0
void FastMemSet16(void *dst, ULong value, ULong bytes)
{
    while (bytes >= 2) {
        *(unsigned short *)dst = value;
        dst = (unsigned short *)dst + 1;
        bytes -= 2;
    }
    if (bytes != 0)
        *(unsigned char *)dst = value;
}

#ifdef __cplusplus
}
#endif
