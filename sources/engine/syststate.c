// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* syststate.c -- two small pieces of the system-entry state machine (win 0x0045cfc0 and 0x0045d030, from the run below 0x0045d0a0). Notes: Research/notes/syststate.md */
#include <stddef.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "player.h"
#include "render.h"
#include "gameutil.h"

EXTERN_C_BEGIN


/* InitStatus. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045cfc0
// MAC: _InitStatus i386 0x00049470 ppc 0x00051e08
void InitStatus(void)
{
    statusBrightColor = &statusColor[0];
    statusDimColor = &statusColor[1];
    radarStatusColor[1] = &statusColor[2];
    radarStatusColor[0] = &statusColor[3];
    shieldStatusColor = &statusColor[4];
    armorStatusColor = &statusColor[5];
    fuelStatusColor[0] = &statusColor[6];
    fuelStatusColor[1] = &statusColor[7];
    lastRadarUpdateTime = 0;
    radarCount = 0;
}

/* HandleStatus. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0045d030
// MAC: _HandleStatus i386 0x000494d7 ppc 0x00051eb0
void HandleStatus(void)
{
    short chance;

    chance = systemData[shipData->curSystem].interference - ShipAntiInterference();
    if (chance > 100)
        chance = 100;
    if (chance < 0)
        chance = 0;
    if (Rand(100) + 1 <= chance)
        interferenceThisFrame = 1;
    else
        interferenceThisFrame = 0;
}

#if defined(MACBUILD) && V111
#define kStatusPanelWidth 194

/* BlitStatusArea (mac-only, new in 1.1.1). STATE: MATCH both slices. OPEN: none */
// MAC: _BlitStatusArea i386 0x00049550 ppc 0x00051f4c
void BlitStatusArea(void)
{
    Rect box;
    PixMapHandle srcPix;
    PixMapHandle dstPix;
    int32_t srcRowBytes;
    int32_t dstRowBytes;
    char *src;
    char *srcBase;
    int32_t width;
    int32_t height;
    int32_t top;
    short left;
    FramePtr frame;

    if (useLegacyRenderingPipeline) {
        SWSetPortToWindow(gSpriteWorld);
        box = *GetCWindowPortRect(gWind);
        box.left = box.right - kStatusPanelWidth;
        ForeColor(blackColor);
        if (gSpriteWorld->fastBlit) {
            srcPix = GetPortPixMap(loadPort.world);
            dstPix = GetCWindowPixMap(gWind);
            srcRowBytes = GetPixRowBytes(srcPix);
            dstRowBytes = GetPixRowBytes(dstPix);
            srcBase = (*srcPix)->baseAddr;
            top = box.top;
            left = box.left;
            height = box.bottom - top;
            frame = gSpriteWorld->windowFrameP;
            if (frame->frameDepth == 16) {
                src = srcBase + left * 2;
                width = (box.right - left) * 2;
            } else {
                src = srcBase + left;
                width = box.right - left;
            }
            BlitPixieRect((unsigned char *)src,
                          (unsigned char *)SW_PIXELPTR(frame, left, top),
                          srcRowBytes, dstRowBytes, width, height);
        } else {
            CopyBits(GetPortBitMapForCopyBits(loadPort.world),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
                     &box, &box, srcCopy, NULL);
        }
    }
}
#endif

EXTERN_C_END
