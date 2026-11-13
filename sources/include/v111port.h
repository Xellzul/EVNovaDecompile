/* v111port.h -- what the 1.1.1 hunks call that the win port lacks; read only by `tools/build.ps1 -V111` (-prefix), never by the default build. Notes: Research/mac-1.1.1-plan.md P4 */
#ifndef V111PORT_H
#define V111PORT_H
#include "int32.h"

typedef int32_t CGDisplayFadeReservationToken;
typedef int32_t CGError;
#define kCGErrorSuccess 0
#define kCGDisplayBlendNormal 0.0f
#define kCGDisplayBlendSolidColor 1.0f

#ifdef __cplusplus
extern "C" {
#endif
extern CGError CGAcquireDisplayFadeReservation(float seconds, CGDisplayFadeReservationToken *token);
extern CGError CGDisplayFade(CGDisplayFadeReservationToken token, float duration, float startBlend,
                             float endBlend, float r, float g, float b, int synchronous);
extern CGError CGReleaseDisplayFadeReservation(CGDisplayFadeReservationToken token);
/* the dialog updates' QDFlushPortBuffer target (the mac engine's EVCarbonHack wrapper) */
extern struct CGrafPort *GetWindowGrafPort(struct CGrafPort *window);
#ifdef __cplusplus
}
#endif

#endif
