// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* centerrect.c -- CenterRect (win 0x0047b710, donor SWGameUtils.c line 82). Notes: Research/notes/centerrect.md */
/* `/2' on a signed short is the `cmp 0x80000000 / sbb -1 / sar 1' triple, four times
   over: that is what fixes the widths here, since an unsigned spelling would have given
   a plain `shr'. */
#include "macport.h"

/* CenterRect (mac _CenterRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b710
void CenterRect(Rect *rectA, Rect *rectB)
{
    short width = (rectA->right - rectA->left);
    short height = (rectA->bottom - rectA->top);

    rectA->left = rectB->left + (((rectB->right - rectB->left) / 2) - (width / 2));
    rectA->top = rectB->top + (((rectB->bottom - rectB->top) / 2) - (height / 2));
    rectA->right = rectA->left + width;
    rectA->bottom = rectA->top + height;
}
