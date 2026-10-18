// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* drawtext.c -- centred Pascal-string drawing (win 0x004622f0). Notes: Research/notes/drawtext.md */
/*
 * Both halves of the centring divide by two as signed ints, which is why the
 * exe carries the cmp/sbb/sar rounding idiom twice.
 */
#include "macport.h"


/* DrawStringCentered. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004622f0
void DrawStringCentered(const unsigned char *str, short left, short right,
                        short v)
{
    short width;

    width = StringWidth(str);
    MoveTo(((left + right) / 2 - width / 2), v);
    DrawString(str);
}
