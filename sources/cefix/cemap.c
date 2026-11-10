// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* cemap.c -- EV Nova CE: the galaxy map's border grid, recomputed over its visible part only
   (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include "macport.h"
#include "stellar.h"
#include "cefix.h"

int ceMapBordersVisibleWidth = kCEBordersWidth;
int ceMapBordersVisibleHeight = kCEBordersHeight;

/* The map item's size in grid cells, taken before each rebuild. As in CE it only shrinks. */
void CE_RecalcMapInfluence(long left, long top, long right, long bottom, Rect *r)
{
    short width;
    short height;

    width = (r->right - r->left) / 2;
    if (width < ceMapBordersVisibleWidth)
        ceMapBordersVisibleWidth = width;
    height = (r->bottom - r->top) / 2;
    if (height < ceMapBordersVisibleHeight)
        ceMapBordersVisibleHeight = height;
    RecalcMapInfluence(left, top, right, bottom, r);
}

#endif /* CEFIX */
