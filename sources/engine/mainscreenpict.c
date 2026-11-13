// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* mainscreenpict.c -- composite the main screen's background picture and its three animated button/logo sprites into loadPort each tick (win 0x0048c3c0). Notes: Research/notes/mainscreenpict.md */
#include <stddef.h>

#include "macport.h"

#include "gametypes.h"
#include "render.h"
#include "game.h"
#include "shipobj.h"

/* MainScreenAnimDraw. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c3c0
// MAC: _MainScreenAnimDraw i386 0x00016146 ppc 0x00019eb4
void MainScreenAnimDraw(void)
{
    Rect rect;
    short i;

    SetGWorldContainer(&loadPort);
    if (logoPort.world != NULL) {
#ifdef MACBUILD
        rect.left = logoPort.bounds.left;
        rect.top = logoPort.bounds.top + logoFrameSize.height * logoIndex;
        rect.bottom = rect.top + logoFrameSize.height;
        rect.right = rect.left + logoFrameSize.width;
#else
        rect.left = logoPort.bounds.left;
        rect.top = logoPort.bounds.top + gMainPicHeightDelta * logoIndex;
        rect.bottom = rect.top + gMainPicHeightDelta;
        rect.right = rect.left + gMainPicWidthDelta;
#endif

        logoDestRect.right = (rect.right - rect.left) + logoDestRect.left;
        logoDestRect.bottom = (rect.bottom - rect.top) + logoDestRect.top;

        ForeColor(blackColor);
        CopyBits(GWorldContainerBits(&logoPort), (BitMapPtr)GWorldContainerBits(&loadPort),
                 &rect, &logoDestRect, 0, NULL);
    }

    for (i = 0; i < 3; i++) {
        if (slidePort[i].world != NULL) {
            if (slideIndex[i] >= 0 && slideIndex[i] < slideFrameCount[i]) {
                rect.left = slidePort[i].bounds.left;
                rect.top = slidePort[i].bounds.top + slideFrameSize[i].height * slideIndex[i];
                rect.bottom = rect.top + slideFrameSize[i].height;
                rect.right = rect.left + slideFrameSize[i].width;

                slideDestRect[i].right = slideDestRect[i].left + (rect.right - rect.left);
                slideDestRect[i].bottom = slideDestRect[i].top + (rect.bottom - rect.top);

                ForeColor(blackColor);
                CopyBits(GetPortBitMapForCopyBits(slidePort[i].world), (BitMapPtr)GWorldContainerBits(&loadPort),
                         &rect, &slideDestRect[i], 0, NULL);
            }
        }
    }
}
