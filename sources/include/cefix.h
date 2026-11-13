/* cefix.h -- the EV Nova Community Edition fixes, built only with -DCEFIX
   (tools/build.ps1 -Cefix). Declares nothing outside #ifdef CEFIX, so a TU that
   includes it compiles to the same object without the define.
   Plan, conventions, site list: Research/cefix-plan.md, Research/cefix-sites.json */
#ifndef CEFIX_H
#define CEFIX_H

#ifdef CEFIX
#include "int32.h"

#ifdef __cplusplus
#include <stdio.h>
using std::_FILE;
extern "C" {
#endif

struct Rect;
struct DialogRecord;

/* the faults CE's own code takes (the CE bug sites) */
#define kCEIntDivideByZero      0xc0000094UL    /* STATUS_INTEGER_DIVIDE_BY_ZERO */
#define kCEIntOverflow          0xc0000095UL    /* STATUS_INTEGER_OVERFLOW: a DIV quotient too wide */
#define kCEPrivilegedInstruction 0xc0000096UL   /* STATUS_PRIVILEGED_INSTRUCTION */

__declspec(dllimport) void __stdcall RaiseException(uint32_t code, uint32_t flags,
    uint32_t count, const uint32_t *args);

/* cestartup.c */
#define kCEMaxPath          260
#define kCEIniSection       "EV Nova"
#define kCEMemoryPoolSize   (2040UL * 1024 * 1024)  /* WinMain's pool: under 2 GiB, no size reads negative */

extern char ceIniPath[kCEMaxPath];
extern int ceX2Hotkey;
extern unsigned char ceRunInAWindow;

int CE_Startup(char *cmdLine, int cmdShow);
double CE_IniDouble(const char *key, const char *def);
void CE_ReadX2Hotkey(void);
void CE_InitializeMonitor(void);
unsigned char CE_IsWindowed(void);
void CE_ToggleFullscreen(void);
int32_t __stdcall CE_WndProc(void *hwnd, uint32_t msg, uint32_t wParam, int32_t lParam);

/* cescale.c */
extern double ceScaleFactor;
extern unsigned char ceScaleEnabled;
extern int ceGridCellWidth;
extern int ceGridCellHeight;
extern int ceGridThumbSize;
extern int ceStatusBarWidth;
extern int ceListItemBaseHeight;

void CE_InitScaleFactor(void);
int CE_Round(double value);
int CE_Scale(int value);
int CE_GridDivide(int offset, int cell);
void CE_ScaleRect(struct Rect *r);
void CE_SetPen(short h, short v);
void CE_ScaleOffsetRect(struct Rect *r, int dh, int dv);
void CE_ScaleOffsetRectBottom(struct Rect *r, int dh, int dv);
int32_t CE_SetDialogTemplateBounds(struct DialogRecord *dlg, const unsigned short *tmpl);

/* cepaths.c */
#ifndef __cplusplus
struct _FILE;
#endif

int32_t CE_DecryptIfNecessary(void *buf, uint32_t len, uint32_t key);
short CE_OpenNovaFilesFallback(const char *path, const char *ext, struct _FILE *log);
short CE_OpenPlugInsFallback(const char *path, const char *ext, struct _FILE *log);
void CE_ExpandFilesPath(char *dst, const char *path);
void CE_ExpandPlugInsPath(char *dst, const char *path);

/* cetext.c */
#define kCEFontMapCount     1           /* Times -> Times New Roman only */

struct WinRect;

int CE_MacRomanToWide(const void *text, uint32_t count, char *out, uint32_t outCount);
int CE_DrawTextW(void *hdc, const char *text, int count, struct WinRect *r, unsigned int format);
void CE_DrawVersion(void);

/* cedialog.c */
int32_t CE_TruncF32(float value);
int32_t CE_RoundD(double value);

/* ceblit.c */
struct PixMap;
struct CGrafPort;

void CE_Blit16(struct PixMap *src, struct PixMap *dst, struct Rect *srcRect, struct Rect *dstRect);
void CE_Blit24(struct PixMap *src, struct PixMap *dst, struct Rect *srcRect, struct Rect *dstRect);
void CE_Blit32(struct PixMap *src, struct PixMap *dst, struct Rect *srcRect, struct Rect *dstRect);
void CE_Blit16Mask(struct PixMap *src, struct PixMap *mask, struct PixMap *dst,
                   struct Rect *srcRect, struct Rect *maskRect, struct Rect *dstRect);
short CE_NewGWorld(struct CGrafPort **out, int depth, const struct Rect *bounds, void *theGDH);

/* cedebug.c */
#define kCEScanF6           0x40
#define kCEScanF7           0x41
#define kCEScanF10          0x44
#define kCEScanBackspace    0x0e
#define kCEScanHome         0x60
#define kCEScanPageUp       0x62
#define kCEScanEnd          0x65
#define kCEScanPageDown     0x67
#define kCEScanNumEnter     0x6a
#define kCEScanNone         0xff        /* no key: KeyIsDown is false from 0x80 up */

struct RGBColor;

unsigned char CE_CheckDebugKeys(short scanCode);
char *CE_NumberToAscii(char *dst, int32_t value, uint32_t flags);
void CE_SetFlashColor(const struct RGBColor *color);

/* ceppat.c */
struct Image_t;

int CE_ImageParsePPat(struct Image_t *image, const void *ppat);

/* cemap.c */
#define kCEBordersWidth     860         /* the border grid in 2 px cells: maps up to 1720x1024 */
#define kCEBordersHeight    512

extern int ceMapBordersVisibleWidth;
extern int ceMapBordersVisibleHeight;

void CE_RecalcMapInfluence(int32_t left, int32_t top, int32_t right, int32_t bottom, struct Rect *r);

/* cehud.c */
void CE_ScaleStatusRects(void);
void CE_MoveTo(short h, short v);

#ifdef __cplusplus
}
#endif

#endif /* CEFIX */

#endif /* CEFIX_H */
