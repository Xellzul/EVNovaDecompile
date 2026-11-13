// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, as the engine TUs whose C++-linkage declarations this calls (macport.h); the file is not part of the exe
/* v111port.c -- win stand-ins for the Carbon calls of the 1.1.1 hunks; built only by `tools/build.ps1 -V111`. Notes: Research/mac-1.1.1-plan.md P4 */
#include "macport.h"
#include "burger.h"
#include "stellar.h"
#include "v111port.h"

#define kMillisPerSecond 1000
#define kGammaEntries 256

/* the ramp the display shows unfaded, saved when a fade leaves it */
static GammaTable_t normalGamma;
static float displayBlend;

/* blend 0.0 shows the saved ramp, 1.0 the solid colour */
static void SetDisplayBlend(float blend, float r, float g, float b)
{
    GammaTable_t ramp;
    float keep;
    int i;

    keep = 1.0f - blend;
    for (i = 0; i < kGammaEntries; i++) {
        ramp.Red[i] = normalGamma.Red[i] * keep + r * blend;
        ramp.Green[i] = normalGamma.Green[i] * keep + g * blend;
        ramp.Blue[i] = normalGamma.Blue[i] * keep + b * blend;
    }
    VideoOSGammaSet(&ramp);
    displayBlend = blend;
}

CGError CGAcquireDisplayFadeReservation(float seconds, CGDisplayFadeReservationToken *token)
{
    *token = 1;
    return kCGErrorSuccess;
}

/* The fade walks BurgerLib's gamma ramp (DirectDraw's gamma control, else the window DC's),
   which ReleaseVideo puts back. Always synchronous: the win V111 build calls it only so. */
CGError CGDisplayFade(CGDisplayFadeReservationToken token, float duration, float startBlend,
                      float endBlend, float r, float g, float b, int synchronous)
{
    uint32_t ms;
    uint32_t start;
    uint32_t elapsed;

    VideoOSGammaInit();
    if (displayBlend == kCGDisplayBlendNormal) {
        VideoOSGammaGet(&normalGamma);
        /* an all-zero table: BurgerLib holds no ramp, so there is nothing to fade */
        if (normalGamma.Red[kGammaEntries - 1] == 0.0f)
            return kCGErrorSuccess;
    }
    ms = (uint32_t)(duration * kMillisPerSecond);
    start = ReadTickMilliseconds();
    for (;;) {
        elapsed = ReadTickMilliseconds() - start;
        if (elapsed >= ms)
            break;
        SetDisplayBlend(startBlend + (endBlend - startBlend) * elapsed / ms, r, g, b);
    }
    SetDisplayBlend(endBlend, r, g, b);
    return kCGErrorSuccess;
}

CGError CGReleaseDisplayFadeReservation(CGDisplayFadeReservationToken token)
{
    return kCGErrorSuccess;
}

CGrafPtr GetWindowPort(CWindowPtr window)
{
    return GetCWindowGrafPort(window);
}

CGrafPtr GetWindowGrafPort(CWindowPtr window)
{
    return GetCWindowGrafPort(window);
}

/* every win port draws into the one game screen: present it, as 1.0.10 does with
   ScreenEndScene where 1.1.1 flushes */
void QDFlushPortBuffer(CGrafPtr port, void *region)
{
    ScreenEndScene();
}
