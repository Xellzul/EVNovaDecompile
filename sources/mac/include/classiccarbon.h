/* classiccarbon.h -- the fragments' Toolbox layer at the end of the 1.0.10 prefix. Both call the accessors; Classic defines them in EVCarbonHack.o and imports InterfaceLib's string calls. Notes: Research/mac-1.0.10-pin.md section 7 */
#include <Endian.h>

#if !TARGET_API_MAC_CARBON

/* the engine's dialog and control records are the win port's, and these names are real here */
#define ControlRecord EVControlRecord
#define DialogRecord EVDialogRecord
#define ControlPtr EVControlPtr

#define GetDialogWindow(dialog) ((WindowRef)(dialog))
#define GetWindowPort(window) ((CGrafPtr)(window))
#define QDFlushPortBuffer(port, region)

/* InterfaceLib has them; Carbon's are the engine's own */
#define PtoCstr p2cstr
#define CtoPstr c2pstr

/* InterfaceLib's older names */
#define DisableMenuItem DisableItem
#define EnableMenuItem EnableItem

/* the low-memory cursor-visible flag */
#define CrsrVis (*(unsigned char *)0x8cc)

/* the gamma library linked into this fragment alone */
extern void DisposeGammaTools(void);
extern long GammaFadeTo(unsigned long id, unsigned long percent, unsigned long millis);
extern long GammaFadeToColor(unsigned long id, void *color, unsigned long percent, unsigned long ticks, unsigned char linear);

#else

/* CarbonLib's gamma calls; Universal Interfaces 3.4.1 has no header for them */
typedef long ScreenRef;
typedef struct ScreenGamma {
    float red[256];
    float green[256];
    float blue[256];
} ScreenGamma;
extern void DisposeScreenRef(ScreenRef screen);
extern void FadeToColor(ScreenRef screen, const RGBColor *color, float seconds);
extern void FadeToGamma(ScreenRef screen, const ScreenGamma *gamma, float seconds);
extern void SetGamma(ScreenRef screen, const ScreenGamma *gamma);
extern ScreenRef gScreenRef;
extern ScreenGamma gOrigGamma;

#endif
