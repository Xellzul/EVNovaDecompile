// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* swdepthok.c -- sw_depth_ok (win 0x00478f10), in the SpriteWorldUtils.c block. Notes: Research/notes/swdepthok.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* sw_depth_ok. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00478f10
// MAC: _sw_depth_ok i386 0x000b7f97 ppc 0x000c3b44
#ifdef MACBUILD
Boolean sw_depth_ok(SpriteWorldPtr spriteWorldP)
{
    if (useLegacyRenderingPipeline && spriteWorldP->mainSWGDH != NULL) {
        PixMapHandle basePixMap = (**(GDHandle)spriteWorldP->mainSWGDH).gdPMap;
        Rect deviceRect;

        if ((**basePixMap).pixelSize != spriteWorldP->pixelDepth)
            return false;
        deviceRect = (**basePixMap).bounds;
        if (deviceRect.right - deviceRect.left !=
                mainDeviceInitBounds.right - mainDeviceInitBounds.left ||
            deviceRect.bottom - deviceRect.top !=
                mainDeviceInitBounds.bottom - mainDeviceInitBounds.top)
            return false;
    }
    return true;
}
#else
Boolean sw_depth_ok(SpriteWorldPtr spriteWorldP)
{
    return 1;
}
#endif
