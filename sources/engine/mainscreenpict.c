// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* mainscreenpict.c -- composite the main screen's background picture and its three animated button/logo sprites into loadPort each tick (win 0x0048c3c0). Notes: Research/notes/mainscreenpict.md */
#include <stddef.h>

#include "macport.h"

#include "gametypes.h"
#include "render.h"
#include "game.h"
#include "shipobj.h"

/* MainScreenAnimDraw. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c3c0
void MainScreenAnimDraw(void)
{
    Rect rect;
    short i;

    SetGWorldContainer(&loadPort);
    if (gDlgWorld.world != NULL) {
        rect.left = gDlgWorld.bounds.left;
        rect.top = gDlgWorld.bounds.top + gMainPicHeightDelta * gMainPicFrame;
        rect.bottom = rect.top + gMainPicHeightDelta;
        rect.right = rect.left + gMainPicWidthDelta;

        logoDestRect.right = (rect.right - rect.left) + logoDestRect.left;
        logoDestRect.bottom = (rect.bottom - rect.top) + logoDestRect.top;

        RGBForeColor(blackColor);
        CopyBits(GetGWCPixMap(&gDlgWorld), (BitMapPtr)GetGWCPixMap(&loadPort),
                 &rect, &logoDestRect, 0, NULL);
    }

    for (i = 0; i < 3; i++) {
        if (gDlgWorlds[i].world != NULL) {
            if (gAnimStep[i] >= 0 && gAnimStep[i] < gAnimEnd[i]) {
                rect.left = gDlgWorlds[i].bounds.left;
                rect.top = gDlgWorlds[i].bounds.top + gAnimDelta[i].height * gAnimStep[i];
                rect.bottom = rect.top + gAnimDelta[i].height;
                rect.right = rect.left + gAnimDelta[i].width;

                slideDestRect[i].right = (rect.right - rect.left) + slideDestRect[i].left;
                slideDestRect[i].bottom = (rect.bottom - rect.top) + slideDestRect[i].top;

                RGBForeColor(blackColor);
                CopyBits((PixMapPtr)GetPortBitMapForCopyBits(gDlgWorlds[i].world), (BitMapPtr)GetGWCPixMap(&loadPort),
                         &rect, &slideDestRect[i], 0, NULL);
            }
        }
    }
}
