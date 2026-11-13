// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* cehud.c -- EV Nova CE: the scaled status bar items and the scaled pen (CEFIX only).
   Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <stddef.h>
#include "macport.h"
#include "dialogs.h"
#include "render.h"
#include "stellar.h"
#include "cefix.h"

#define kStatusRectCount    8
#define kFirstScaledStatus  6       /* the target and cargo panels */
#define kEdgeMargin         2       /* the last two pixels of an item are not scaled */

/* Every status bar item rect of the intf resource, scaled as its background is. */
void CE_ScaleStatusRects(void)
{
    short i;

    for (i = 0; i < kStatusRectCount; i++)
        CE_ScaleRect(&statusOffRect[i]);
}

/* A point inside box moves to its scaled offset from box's top-left corner. */
static int ScalePointInRect(short *h, short *v, const Rect *box)
{
    if (*h >= box->left && *h < box->right - kEdgeMargin
            && *v >= box->top && *v < box->bottom - kEdgeMargin) {
        *h = CE_Scale(*h - box->left) + box->left;
        *v = CE_Scale(*v - box->top) + box->top;
        return 1;
    }
    return 0;
}

/* MoveTo into the main buffer scales a point that falls inside one of the text areas
   of the dialog on top, or inside the target or cargo panel when no dialog is up, so
   the many fixed text offsets drawn there need no site of their own. */
void CE_MoveTo(short h, short v)
{
    int32_t items[3];
    short count;
    short i;
    int barLeft;
    Rect box;

    if (ceScaleEnabled && CurrentGrafPort == loadPort.world) {
        if (gCurDialog != NULL) {
            count = 0;
            if (gCurDialog == commDialog) {
                items[0] = 12;          /* ship info, not in planet comms */
                items[1] = 6;           /* planet info, off screen in ship comms */
                items[2] = 10;          /* ship response and escort costs */
                count = 3;
            } else if (gCurDialog == plunderDialog) {
                items[0] = 5;
                count = 1;
            } else if (gCurDialog == shipyardDialog || gCurDialog == outfitDialog) {
                items[0] = 9;           /* purchase details */
                count = 1;
            } else if (gCurDialog == shipDescDialog) {
                items[0] = 5;           /* ship specs */
                count = 1;
            } else if (gCurDialog == infoDialog) {
                items[0] = 6;
                count = 1;
            } else if (gCurDialog == mapDialog) {
                items[0] = 2;           /* ports and hazards */
                items[1] = 6;           /* system info */
                count = 2;
            }
            for (i = 0; i < count; i++) {
                GetDialogItem(gCurDialog, items[i], NULL, NULL, &box);
                if (ScalePointInRect(&h, &v, &box))
                    break;
            }
        } else {
            barLeft = GetCWindowPortRect(gWind)->right - ceStatusBarWidth;
            for (i = kFirstScaledStatus; i < kStatusRectCount; i++) {
                box = statusOffRect[i];
                OffsetRect(&box, barLeft, 0);
                ScalePointInRect(&h, &v, &box);
            }
        }
    }
    CE_SetPen(h, v);
}

#endif /* CEFIX */
