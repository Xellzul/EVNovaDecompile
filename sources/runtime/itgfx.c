// COMPILE: -opt level=2,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: the draw and sound tools call memset/memcpy at constant sizes, so no ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itgfx.c -- the interface tool's GDI layer (0x0050e7a4..0x00512411): ports, printer devices, DIB sections and pixel-map locking; only the bodies the image core needs so far. Notes: Research/notes/itgfx.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"

extern void *memset(void *dst, int c, unsigned int n);
extern char *strcpy(char *dst, const char *src);
extern int strcmp(const char *a, const char *b);

extern void *DT_RegionCreate(void);                 /* ittoolkit.c */
extern long DT_RegionScale(void *region, const ITRect *from, const ITRect *to);  /* ittoolkit.c */
extern void *DT_RegionClone(void *region);                                     /* ittoolkit.c */
extern ITRect *DT_GetRegionRect(ITRect *box, void *region);                      /* ittoolkit.c */
extern long DT_SetRegionRect(void *region, const ITRect *r);                     /* ittoolkit.c */

/* user32's SetRect; not in win32.h, where it would meet macport.h's Toolbox SetRect */
__declspec(dllimport) int __stdcall SetRect(WinRect *r, int left, int top, int right,
                                            int bottom);
__declspec(dllimport) int __stdcall LineTo(void *hdc, int x, int y);
__declspec(dllimport) int __stdcall InvertRect(void *hdc, const WinRect *r);

typedef struct DibInfo {
    ULong size;                                  /* +0x00 BITMAPINFOHEADER */
    long width;                                  /* +0x04 */
    long height;                                 /* +0x08 */
    unsigned short planes;                       /* +0x0c */
    unsigned short bitCount;                     /* +0x0e */
    ULong compression;                           /* +0x10 */
    char unk14[0x28 - 0x14];
    ULong masks[256];                            /* +0x28 bit fields, or the colours */
} DibInfo;

typedef void *(__stdcall *CreateDIBSectionProc)(void *dc, const void *info, unsigned int usage,
                                                void **bits, void *section, ULong offset);

extern ITPalette gDT_DefaultPalette;                   /* atpalinit.c: the system palette */

void DT_DisposeMacPixMap(ITDib *dib);
void _DT_PaletteToMacCTab(RGBQuad *quads, ULong *count, const ITPalette *palette);
ULong DT_DCFormatKey(void *dc);

#define BI_BITFIELDS        3
#define kDibHeaderSize      0x28
#define BLACK_PEN           7
#define ANSI_VAR_FONT       12
#define DIB_RGB_COLORS      0
#define kNoWindow           ((void *)-1)
#define kPrintAborted       -7007
#define kDibFailed          -7300
#define kPixelsLockBit      3
#define kPixelsWholeWindow  0x01
#define OBJ_MEMDC           10
#define MM_TEXT             1
#define OPAQUE              2
#define TRANSPARENT         1
#define R2_COPYPEN          13
#define TA_UPDATECP         1
#define TA_BASELINE         24
#define GDI_ERROR           0xffffffffU
#define PS_SOLID            0
#define PS_GEOMETRIC        0x00010000
#define OBJ_BRUSH           2
#define kResetFontSize      12
#define kFaceMax            255
#define kMetricsSize        0x14    /* ITPort ascent .. maxWidth */
#define kAlignCentre        0
#define TA_RIGHT            2
#define TA_CENTER           6
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define SYSTEM_FIXED_FONT   16
#define kTransferCopy       0       /* ITTransparent.mode */
#define kTransferMask       1
#define kTransferAlpha      3       /* index is the alpha: 0 draws nothing, 0xff is opaque */
#define kTransferAlphaMask  4
#define kTransferKey        5       /* index (or the colour) is the transparent key */
#define kWhite              0x00ffffff
#define GRAY_BRUSH          2
#define HORZRES             8
#define VERTRES             10
#define RGN_AND             1
#define kBlack             0x00000000
#define kGray7f             0x007f7f7f
#define kGray80             0x00808080

ULong gDT_Printers;                              /* 0x008705c8 open printer devices */

long _DT_ContextCreate(ITPort *port, void *window, void *dc, unsigned char save);
long DT_DrawDib(void *dc, const ITRect *dst, ITDib *src, ITDib *mask,
                  const ITTransparent *trans, const ITRect *srcRect);
ITImage *_DT_StuffMacPixMap(ITDib *dib, ITImage *image);
ITDib *DT_CreateMacPixMap(void *dc, const ITRect *bounds, ULong format, const RGBQuad *colors,
                    int withBitmap);
extern ULong *CopyDT_2SysColor(ULong *ref, const ITColor *color);           /* ittoolkit.c */
ITImage *DT_SurfaceCreateLikeWindow(void *window, const ITRect *area);
ULong DT_DibHeaderFormatKey(const BitmapInfoHeader *info);
void _DT_MacCTabToPalette(ITPalette *palette, const RGBQuad *quads, ULong count);
void _DT_MacCTabToInverse(unsigned char *table, const RGBQuad *quads, ULong count);
long DT_SurfaceCopyRects(ITImage *image, ULong count, const ITRect *src, const ITRect *dst);
ULong DT_DibFormatKey(const ITDib *dib);
long DT_DrawTextAt(ITImage *image, long x, long y, const char *text, long length);

#define kPixelsBlockSize    (sizeof(ITImage) + sizeof(ITPixels))

/* a DIB's colour table follows its BITMAPINFOHEADER (0x34 from the record) */
#define DIB_COLORS(dib)     ((ULong *)(&(dib)->header + 1))

/* MemoryCopy's body (atplatheap.c), which the interface tool inlines */
static inline void ATMemMove(void *dst, const void *src, ULong n)
{
    if (n != 0)
        memmove(dst, src, n);
}

/* StringCopy's body (atplatstr.c), which the interface tool inlines */
static inline char *ATStrCpy(char *dst, const char *src)
{
    char *start;

    start = dst;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}

/* StringCompare's body (atplatstr.c), which the interface tool inlines */
static inline long ATStrCmp(const char *a, const char *b)
{
    ULong ca;
    ULong cb;
    const unsigned char *s1;
    const unsigned char *s2;

    s1 = (const unsigned char *)a;
    s2 = (const unsigned char *)b;
    while ((ca = *s1++) == (cb = *s2++)) {
        if (ca == 0)
            return 0;
    }
    return ca < cb ? (signed char)-1 : (signed char)1;
}

/* DT_ContextCreate (a new image with a port on a window's DC, the DC state saved when asked; NULL
   on failure). STATE: MATCH 100%, exe 52 / obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e7a4
ITImage *DT_ContextCreate(void *window, unsigned char save)
{
    ITImage *image;
    ITPort *port;
    long err;
    long e;

    image = 0;
    port = 0;
    err = 0;
    image = _DT_SurfaceCreate(&port, 0, 0);
    if (image == 0) {
        err = kOutOfMemory;
    } else {
        e = _DT_ContextCreate(port, window, 0, save);
        if (e != 0)
            err = e;
    }
    if (err != 0 && image != 0) {
        _DT_SurfaceDispose(image, 1, 0, 0);
        image = 0;
        port = 0;
    }
    return port != 0 ? port->link.self : 0;
}

/* DT_ContextCreateRaw (an image whose port draws into an existing DC; NULL when it cannot be made).
   STATE: MATCH 100%, exe 52 / obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050e828
ITImage *DT_ContextCreateRaw(void *dc, unsigned char save)
{
    ITImage *image;
    ITPort *port;
    long err;
    long e;

    image = 0;
    port = 0;
    err = 0;
    image = _DT_SurfaceCreate(&port, 0, 0);
    if (image == 0) {
        err = kOutOfMemory;
    } else {
        e = _DT_ContextCreate(port, 0, dc, save);
        if (e != 0)
            err = e;
    }
    if (err != 0 && image != 0) {
        _DT_SurfaceDispose(image, 1, 0, 0);
        image = 0;
        port = 0;
    }
    return port != 0 ? port->link.self : 0;
}

/* DT_ContextDispose (dispose a window image, as its port's disposer). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e8ac
long DT_ContextDispose(ITImage *image)
{
    return _DT_SurfaceDispose(image, 1, 0, 0);
}

/* _DT_ContextCreate (open a port on a DC, or on a window's DC when none is given; optionally with
   the DC state saved; its clip region read into the port). STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e8bf
long _DT_ContextCreate(ITPort *port, void *window, void *dc, unsigned char save)
{
    long err;

    err = 0;
    if (dc != 0) {
        port->dc = dc;
        port->window = kNoWindow;
    } else {
        port->window = window;
        port->dc = GetDC(window);
        dc = port->dc;
        if (dc == 0) {
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
    }
    if (save != 0) {
        port->savedIndex = SaveDC(dc);
        if (port->savedIndex == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
    }
    port->savedDC = save;
    port->unk154 = 0;
    port->unk158 = kWhite;
    port->clip = DT_GetClipRegion(port->link.self);
    if (port->clip == 0)
        err = kOutOfMemory;
done:
    return err;
}

/* _DT_ContextDispose (release a port's DC: its selected stock objects back, the saved DC state
   restored, a window DC released, its clip region deleted). STATE: MATCH 100%, exe 58 / obj 58, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050e965
void _DT_ContextDispose(ITPort *port)
{
    long err;

    err = 0;
    if (port->ownPen != 0)
        DeleteObject(SelectObject(port->dc, GetStockObject(BLACK_PEN)));
    if (port->ownFont != 0)
        DeleteObject(SelectObject(port->dc, GetStockObject(ANSI_VAR_FONT)));
    if (port->ownBrush != 0)
        DeleteObject(SelectObject(port->dc, GetStockObject(BLACK_BRUSH)));
    if (port->savedDC != 0) {
        if (RestoreDC(port->dc, port->savedIndex) == 0)
            GetLastError();
    }
    if (port->window != kNoWindow) {
        ReleaseDC(port->window, port->dc);
        GetLastError();
    }
    if (port->clip != 0)
        DT_RegionDispose(port->clip);
    port->window = 0;
    port->dc = 0;
}

/* _DT_ContextLock (before GDI draws into a port: its pixel map locked, and changed or locked
   pixels stored into the bitmap). STATE: MATCH 100%, exe 43 / obj 43, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050ea1f
long _DT_ContextLock(ITPort *port)
{
    long err;
    long e;
    ITPixels *pixels;
    long locks;

    err = 0;
    pixels = 0;
    locks = 0;
    if (port->link.owner2->pixelsSelf != 0) {
        pixels = _DT_SurfaceGetImage(port->link.owner2);
        locks = pixels->locks;
        _DT_ImageLock(pixels->link.owner2->pixelsSelf, 0, 0);
        if (locks != 0 || (pixels->flags & kPixelsBitsChanged) != 0) {
            e = DT_SyncDib(pixels->dib, 1, 0);
            if (e != 0)
                err = e;
            else
                pixels->flags &= ~kPixelsBitsChanged;
        }
    }
    return err;
}

/* _DT_ContextUnlock (after GDI drew into a port: the pixel map unlocked, and read back from the
   bitmap when it stays locked, else marked as living in the bitmap). STATE: MATCH 100%, exe 33
   / obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050ea84
void _DT_ContextUnlock(ITPort *port)
{
    long err;
    ITPixels *pixels;

    err = 0;
    pixels = 0;
    if (port->link.owner2->pixelsSelf != 0) {
        pixels = _DT_SurfaceGetImage(port->link.owner2);
        _DT_ImageUnlock(pixels->link.owner2->pixelsSelf, 0);
        if (pixels->locks != 0) {
            err = DT_SyncDib(pixels->dib, 0, 1);
            if (err == 0)
                pixels->flags &= ~kPixelsInBitmap;
        } else {
            pixels->flags |= kPixelsInBitmap;
        }
    }
}

/* DT_ResetAttributes (reset a port: MM_TEXT, opaque; with `pen' or `brush' white background,
   R2_COPYPEN, the black pen and brush and the solid pattern; with `font' black text, baseline
   alignment moving the current position, the ANSI variable font and its metrics; with `clip' the
   saved clip region back; with `origin' the viewport origin at 0). STATE: MATCH 100%, exe 166 /
   obj 166, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050ead7
long DT_ResetAttributes(ITImage *image, unsigned char pen, unsigned char brush, unsigned char font,
                  unsigned char clip, unsigned char origin)
{
    ITPort *port;
    long err;
    long e;
    void *oldPen;
    void *oldFont;
    void *oldBrush;
    TEXTMETRICA tm;

    port = _DT_ContextGetContext(image);
    err = 0;
    oldPen = 0;
    oldFont = 0;
    oldBrush = 0;
    if (!SetMapMode(port->dc, MM_TEXT) || !SetBkMode(port->dc, OPAQUE))
        goto failed;
    if (pen || brush) {
        if (SetBkColor(port->dc, kWhite) == CLR_INVALID || !SetROP2(port->dc, R2_COPYPEN))
            goto failed;
        oldPen = SelectObject(port->dc, GetStockObject(BLACK_PEN));
        if (oldPen == 0)
            goto failed;
        oldBrush = SelectObject(port->dc, GetStockObject(BLACK_BRUSH));
        if (oldBrush == 0)
            goto failed;
        port->pattern = gDT_PatternBlack;
        port->penPattern = 0;
        port->penWidth = 0;
        if (!port->ownPen) {
            oldPen = 0;
            port->ownPen = 1;
        }
        if (!port->ownBrush) {
            oldBrush = 0;
            port->ownBrush = 1;
        }
    }
    if (font) {
        if (SetTextColor(port->dc, kBlack) == CLR_INVALID)
            goto failed;
        if (SetTextAlign(port->dc, TA_BASELINE | TA_UPDATECP) == GDI_ERROR) {
            err = GDI_ERROR;
            goto nomem;
        }
        oldFont = SelectObject(port->dc, GetStockObject(ANSI_VAR_FONT));
        if (oldFont == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        port->fontId = ANSI_VAR_FONT;
        port->fontSize = kResetFontSize;
        port->fontStyle = 0;
        port->textAlign = -1;
        if (!GetTextMetricsA(port->dc, &tm)) {
failed:
            err = GetLastError();
nomem:
            err = kOutOfMemory;
            goto done;
        }
        port->ascent = tm.tmAscent;
        port->descent = tm.tmDescent;
        port->leading = tm.tmExternalLeading;
        port->height = tm.tmHeight;
        port->maxWidth = tm.tmMaxCharWidth;
        /* BUG: 0 where the pen and brush branches set 1, so the old font is never deleted */
        if (!port->ownFont) {
            oldFont = 0;
            port->ownFont = 0;
        }
    }
    if (clip && port->clip != 0) {
        e = DT_SetClipRegion(image, port->clip);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (origin && !SetViewportOrgEx(port->dc, 0, 0, 0)) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (oldPen != 0)
        DeleteObject(oldPen);
    if (oldFont != 0)
        DeleteObject(oldFont);
    if (oldBrush != 0)
        DeleteObject(oldBrush);
    return err;
}

/* DT_SetFillAttributes (a port's text colour, background colour and fill pattern, each only when given;
   the brush follows the pattern, or the text colour for the solid pattern). STATE: MATCH 100%,
   exe 112 / obj 112, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050ecfa
long DT_SetFillAttributes(ITImage *image, const ITColor *fore, const ITColor *back,
                  const unsigned char *pattern)
{
    ITPort *port;
    long err;
    void *brush;
    void *bitmap;
    ULong color;

    port = _DT_ContextGetContext(image);
    err = 0;
    brush = 0;
    bitmap = 0;
    if (fore != 0 && SetTextColor(port->dc, DT_2SysColor(fore)) == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (back != 0 && SetBkColor(port->dc, DT_2SysColor(back)) == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (pattern != 0)
        port->pattern = pattern;
    if (port->pattern == gDT_PatternBlack) {
        color = GetTextColor(port->dc);
        switch (color) {
        case kBlack:
            brush = GetStockObject(BLACK_BRUSH);
            break;
        case kGray7f:
        case kGray80:
            brush = GetStockObject(GRAY_BRUSH);
            break;
        case kWhite:
            brush = GetStockObject(WHITE_BRUSH);
            break;
        case CLR_INVALID:
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        default:
            brush = CreateSolidBrush(color);
            if (brush == 0) {
                err = GetLastError();
                err = kOutOfMemory;
                goto done;
            }
            break;
        }
    } else {
        bitmap = CreateBitmap(8, 8, 1, 1, port->pattern);
        if (bitmap == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        brush = CreatePatternBrush(bitmap);
        if (brush == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
    }
    brush = SelectObject(port->dc, brush);
    if (brush == 0)
        err = GetLastError();
    if (port->ownBrush == 0) {
        brush = 0;
        port->ownBrush = 1;
    }
done:
    if (brush != 0)
        DeleteObject(brush);
    if (bitmap != 0)
        DeleteObject(bitmap);
    return err;
}

/* DT_GetFillColor (a port's text colour). STATE: MATCH 100%, exe 25 / obj 25, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050ee3a
long DT_GetFillColor(ITImage *image, ITColor *fore)
{
    ITPort *port;
    long err;
    ULong color;

    port = _DT_ContextGetContext(image);
    err = 0;
    color = GetTextColor(port->dc);
    if (color == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
    } else {
        CopySys2DT_Color(fore, &color);
    }
    return err;
}

/* DT_GetFillBackColor (a port's background colour). STATE: MATCH 100%, exe 25 / obj 25, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050ee82
long DT_GetFillBackColor(ITImage *image, ITColor *back)
{
    ITPort *port;
    long err;
    ULong color;

    port = _DT_ContextGetContext(image);
    err = 0;
    color = GetBkColor(port->dc);
    if (color == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
    } else {
        CopySys2DT_Color(back, &color);
    }
    return err;
}

/* DT_SetFillBackColor (set a port's background colour). STATE: MATCH 100%, exe 15 / obj 15,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050eeca
long DT_SetFillBackColor(ITImage *image, const ITColor *back)
{
    ITPort *port;
    long err;

    port = _DT_ContextGetContext(image);
    err = 0;
    return DT_SetFillAttributes(image, 0, back, 0);
}

/* DT_SetLineAttributes (a port's pen: text and background colours when given, the width unless -1, the
   pen pattern when given; a plain pen in the text colour for no pattern or the solid one, else a
   geometric pen and a pattern brush from the 8 x 8 pattern). STATE: MATCH 100%, exe 138 / obj
   138, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050eeed
long DT_SetLineAttributes(ITImage *image, const ITColor *fore, const ITColor *back,
                  const unsigned char *pattern, long width)
{
    ITPort *port;
    long err;
    void *pen;
    void *brush;
    void *bitmap;

    port = _DT_ContextGetContext(image);
    err = 0;
    pen = 0;
    brush = 0;
    bitmap = 0;
    if (width != -1)
        port->penWidth = width;
    if (fore != 0 && SetTextColor(port->dc, DT_2SysColor(fore)) == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (back != 0 && SetBkColor(port->dc, DT_2SysColor(back)) == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (pattern != 0)
        port->penPattern = pattern;
    if (port->penPattern == 0 || port->penPattern == gDT_PatternBlack) {
        pen = CreatePen(PS_SOLID, port->penWidth, GetTextColor(port->dc));
        if (pen == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        pen = SelectObject(port->dc, pen);
        /* BUG: a Win32 error code returned as the toolkit's (here and below) */
        if (pen == 0)
            err = GetLastError();
        if (!port->ownPen) {
            pen = 0;
            port->ownPen = 1;
        }
        port->penPattern = 0;
    } else {
        pen = CreatePen(PS_GEOMETRIC, port->penWidth, kBlack);
        if (pen == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        pen = SelectObject(port->dc, pen);
        if (pen == 0)
            err = GetLastError();
        /* BUG: deleted here and again at done */
        if (pen != 0)
            DeleteObject(pen);
        bitmap = CreateBitmap(8, 8, 1, 1, port->penPattern);
        if (bitmap == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        brush = CreatePatternBrush(bitmap);
        if (brush == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        brush = SelectObject(port->dc, brush);
        if (brush == 0)
            err = GetLastError();
        if (!port->ownBrush) {
            brush = 0;
            port->ownBrush = 1;
        }
    }
done:
    if (pen != 0)
        DeleteObject(pen);
    if (brush != 0)
        DeleteObject(brush);
    if (bitmap != 0)
        DeleteObject(bitmap);
    return err;
}

/* DT_FontEnumProc (EnumFontFamiliesA's callback: a family by that name exists; stop).
   STATE: MATCH 100%, exe 4 / obj 4, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f093
int __stdcall DT_FontEnumProc(const void *font, const void *metric, ULong type, long found)
{
    *(unsigned char *)found = 1;
    return 0;
}

/* _DT_GetFontByNameList (the first installed family of a FACE list, `_' read as a space; the
   generic names SYSTEM, MONOSPACE, FIXED, SANS-SERIF, SANS and SERIF stand for Mac and
   Windows lists of their own; NULL when none is installed). STATE: MATCH 100%, exe 292 /
   obj 292, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f09f
char *_DT_GetFontByNameList(void *dc, const char *faces, char *out)
{
    const char *p;
    char c;
    ULong length;
    ULong i;
    char name[256];
    unsigned char found;

    i = 0;
    p = 0;
    found = 0;
    for (p = faces; *faces != 0; p++) {
        c = *p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ' || c == '_' ||
            c == '-')
            continue;
        if (p != faces) {
            length = p - faces < kFaceMax ? p - faces : kFaceMax;
            ATMemMove(name, faces, length);
            name[length] = 0;
            for (i = 0; i < length; i++) {
                if (name[i] == '_')
                    name[i] = ' ';
            }
            EnumFontFamiliesA(dc, name, DT_FontEnumProc, (long)&found);
            if (found)
                break;
        }
        String2UpperCopySafe(name, name, (ULong)-1);
        if (ATStrCmp(name, "SYSTEM") == 0 &&
            _DT_GetFontByNameList(dc, "Charcoal,Lucida_Grande,Chicago", out) != 0)
            return out;
        if (ATStrCmp(name, "MONOSPACE") == 0 && _DT_GetFontByNameList(dc, "Monaco,Courier", out) != 0)
            return out;
        if (ATStrCmp(name, "FIXED") == 0 && _DT_GetFontByNameList(dc, "Monaco,Courier", out) != 0)
            return out;
        if (ATStrCmp(name, "SANS-SERIF") == 0 &&
            _DT_GetFontByNameList(dc, "Geneva,Arial,Verdana,Helvetica", out) != 0)
            return out;
        if (ATStrCmp(name, "SANS") == 0 &&
            _DT_GetFontByNameList(dc, "Geneva,Arial,Verdana,Helvetica", out) != 0)
            return out;
        if (ATStrCmp(name, "SERIF") == 0 &&
            _DT_GetFontByNameList(dc, "Palatino,Times New Roman,Times", out) != 0)
            return out;
        if (*p != 0)
            faces = p + 1;
        else
            faces = p;
    }
    ATStrCpy(out, *faces != 0 ? name : "");
    return *out != 0 ? out : 0;
}

static const unsigned char DAT_00579ae4[8] = { 0 };   /* 0x00579ae4, unreferenced */

/* DT_SetTextAttributes (a port's font: text colour when given, alignment on the baseline, a stock font by
   number or one made from the FACE list, size and style bits (bold, italic, underline), the
   metrics kept in the port). STATE: MATCH 100%, exe 178 / obj 178, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x0050f3bd
long DT_SetTextAttributes(ITImage *image, const ITColor *color, const char *face, ULong fontId,
                  long size, ULong style, long align)
{
    ITPort *port;
    long err;
    void *font;
    char family[256];
    TEXTMETRICA tm;

    port = _DT_ContextGetContext(image);
    err = 0;
    font = 0;
    if (face != 0)
        face = _DT_GetFontByNameList(port->dc, face, family);
    if (color != 0 && SetTextColor(port->dc, DT_2SysColor(color)) == CLR_INVALID)
        goto failed;
    switch (align) {
    case kAlignRight:
        if (SetTextAlign(port->dc, TA_BASELINE | TA_UPDATECP | TA_RIGHT) == GDI_ERROR) {
            err = -1;
            goto nomem;
        }
        break;
    case kAlignCentre:
        if (SetTextAlign(port->dc, TA_BASELINE | TA_UPDATECP | TA_CENTER) == GDI_ERROR) {
            err = -1;
            goto nomem;
        }
        break;
    case -1:
    default:
        if (SetTextAlign(port->dc, TA_BASELINE | TA_UPDATECP) == GDI_ERROR) {
            err = -1;
            goto nomem;
        }
        break;
    }
    switch (fontId) {
    case OEM_FIXED_FONT:
    case ANSI_FIXED_FONT:
    case ANSI_VAR_FONT:
    case SYSTEM_FONT:
    case DEVICE_DEFAULT_FONT:
    case SYSTEM_FIXED_FONT:
        font = GetStockObject(fontId);
        size = kResetFontSize;
        break;
    default:
        font = CreateFontA(size, 0, 0, 0, (style & kFontBold) ? FW_BOLD : FW_NORMAL,
                           (style & kFontItalic) ? (unsigned char)1 : (unsigned char)0,
                           (style & kFontUnderline) ? (unsigned char)1 : (unsigned char)0, 0, 0, 0,
                           0, 0, 0, face != 0 && *face != 0 ? face : 0);
        if (font == 0) {
            err = GetLastError();
            font = GetStockObject(ANSI_VAR_FONT);
        }
        break;
    }
    font = SelectObject(port->dc, font);
    if (font == 0)
        err = GetLastError();
    if (!port->ownFont) {
        font = 0;
        port->ownFont = 1;
    }
    StringCopySafe(port->fontFace, face != 0 ? face : "", sizeof(port->fontFace));
    port->fontId = fontId;
    port->fontSize = size;
    port->fontStyle = style;
    port->textAlign = align;
    if (GetTextMetricsA(port->dc, &tm) == 0) {
failed:
        err = GetLastError();
nomem:
        err = kOutOfMemory;
    } else {
        port->ascent = tm.tmAscent;
        port->descent = tm.tmDescent;
        port->leading = tm.tmExternalLeading;
        port->height = tm.tmHeight;
        port->maxWidth = tm.tmMaxCharWidth;
    }
    if (font != 0)
        DeleteObject(font);
    return err;
}

/* DT_SetTextColor (set a port's text colour). STATE: MATCH 100%, exe 22 / obj 22, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050f601
long DT_SetTextColor(ITImage *image, const ITColor *fore)
{
    ITPort *port;
    long err;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (SetTextColor(port->dc, DT_2SysColor(fore)) == CLR_INVALID) {
        err = GetLastError();
        err = kOutOfMemory;
    }
    return err;
}

/* DT_SetTextSize (a port's font size: the font remade at that size when it changed, dropping its
   FACE). STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f63c
long DT_SetTextSize(ITImage *image, long size)
{
    ITPort *port;
    long err;
    unsigned char changed;
    long font;
    ULong style;
    long flags;

    port = _DT_ContextGetContext(image);
    err = 0;
    changed = 0;
    if (size != port->fontSize)
        changed = 1;
    font = port->fontId;
    style = port->fontStyle;
    flags = port->textAlign;
    if (changed)
        err = DT_SetTextAttributes(image, 0, 0, font, size, style, flags);
    return err;
}

/* DT_GetTextInfo (a port's font metrics: ascent, descent, leading, height, widest character).
   STATE: MATCH 100%, exe 13 / obj 13, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f681
long DT_GetTextInfo(ITImage *image, long *metrics)
{
    ITPort *port;
    long err;

    port = _DT_ContextGetContext(image);
    err = 0;
    memmove(metrics, &port->ascent, kMetricsSize);
    return 0;
}

/* DT_GetClipRegion (an image port's clip region in logical units: everything when the DC has none;
   NULL on failure). STATE: MATCH 100%, exe 97 / obj 97, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f6a4
void *DT_GetClipRegion(ITImage *image)
{
    ITPort *port;
    long err;
    long e;
    void *region;
    WinRect box;
    ITRect device;
    ITRect logical;

    port = _DT_ContextGetContext(image);
    err = 0;
    region = 0;
    region = DT_RegionCreate();
    if (region == 0)
        goto nomem;
    switch (GetClipRgn(port->dc, region)) {
    case 1:
        GetRgnBox(region, &box);
        CopySys2DT_Rect(&device, (ITRect *)&box);
        DPtoLP(port->dc, &box, 2);
        CopySys2DT_Rect(&logical, (ITRect *)&box);
        if (DT_IsRectEqual(&device, &logical) == 0) {
            e = DT_RegionScale(region, &logical, &device);
            if (e != 0)
                err = e;
        }
        break;
    case 0:
        if (SetRectRgn(region, kMinLong, kMinLong, kMaxLong, kMaxLong) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        break;
    case -1:
        err = GetLastError();
    nomem:
        err = kOutOfMemory;
        goto done;
    default:
        err = kDibFailed;
        break;
    }
done:
    if (err != 0 && region != 0) {
        DT_RegionDispose(region);
        region = 0;
    }
    return region;
}

/* DT_AddClipRegion (intersect a port's clip with a region given in logical units, carried to
   device units when the two differ). STATE: MATCH 100%, exe 92 / obj 92, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050f7ae
long DT_AddClipRegion(ITImage *image, void *region)
{
    ITPort *port;
    long err;
    long e;
    void *copy;
    WinRect box;
    ITRect logical;
    ITRect device;

    port = _DT_ContextGetContext(image);
    err = 0;
    copy = 0;
    GetRgnBox(region, &box);
    CopySys2DT_Rect(&logical, (ITRect *)&box);
    LPtoDP(port->dc, (POINT *)&box, 2);
    CopySys2DT_Rect(&device, (ITRect *)&box);
    if (DT_IsRectEqual(&logical, &device) == 0) {
        copy = DT_RegionClone(region);
        if (copy == 0)
            goto nomem;
        e = DT_RegionScale(copy, &device, &logical);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (ExtSelectClipRgn(port->dc, copy, RGN_AND) == 0) {
            err = GetLastError();
nomem:
            err = kOutOfMemory;
            goto done;
        }
    } else if (ExtSelectClipRgn(port->dc, region, RGN_AND) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (copy != 0)
        DT_RegionDispose(copy);
    return err;
}

/* DT_AddClipRect (intersect a port's clip with a rectangle). STATE: MATCH 100%, exe 39 / obj 39,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f892
long DT_AddClipRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    void *region;
    long e;

    port = _DT_ContextGetContext(image);
    err = 0;
    region = 0;
    region = DT_RegionCreate();
    if (region == 0) {
        err = kOutOfMemory;
    } else {
        DT_SetRegionRect(region, r);
        e = DT_AddClipRegion(image, region);
        if (e != 0)
            err = e;
    }
    if (region != 0)
        DT_RegionDispose(region);
    return err;
}

/* DT_SetClipRegion (set a port's clip to a region given in logical units). STATE: MATCH 100%, exe
   90 / obj 90, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f8e4
long DT_SetClipRegion(ITImage *image, void *region)
{
    ITPort *port;
    long err;
    long e;
    void *copy;
    WinRect box;
    ITRect logical;
    ITRect device;

    port = _DT_ContextGetContext(image);
    err = 0;
    copy = 0;
    GetRgnBox(region, &box);
    CopySys2DT_Rect(&logical, (ITRect *)&box);
    LPtoDP(port->dc, (POINT *)&box, 2);
    CopySys2DT_Rect(&device, (ITRect *)&box);
    if (DT_IsRectEqual(&logical, &device) == 0) {
        copy = DT_RegionClone(region);
        if (copy == 0)
            goto nomem;
        e = DT_RegionScale(copy, &device, &logical);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (SelectClipRgn(port->dc, copy) == 0) {
            err = GetLastError();
nomem:
            err = kOutOfMemory;
            goto done;
        }
    } else if (SelectClipRgn(port->dc, region) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (copy != 0)
        DT_RegionDispose(copy);
    return err;
}

/* DT_SetClipRect (set a port's clip to a rectangle given in logical units). STATE: MATCH 100%, exe
   63 / obj 63, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050f9c4
long DT_SetClipRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    void *region;
    ITRect device;
    WinRect box;

    port = _DT_ContextGetContext(image);
    err = 0;
    region = 0;
    CopyDT_2SysRect((ITRect *)&box, r);
    LPtoDP(port->dc, (POINT *)&box, 2);
    CopySys2DT_Rect(&device, (ITRect *)&box);
    r = &device;
    region = DT_RegionCreate();
    if (region == 0) {
        err = kOutOfMemory;
    } else {
        DT_SetRegionRect(region, r);
        if (SelectClipRgn(port->dc, region) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
        }
    }
    if (region != 0)
        DT_RegionDispose(region);
    return err;
}

/* DT_AddInvalidRect (invalidate a rectangle of a port's visible window; -7002 for a port on no
   window). STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050fa5d
long DT_AddInvalidRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    ITRect clamped;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (port->window == kNoWindow) {
        err = kNotAWindow;
    } else if (IsWindowVisible(port->window) != 0) {
        CopyDT_2SysRect(&clamped, r);
        if (InvalidateRect(port->window, (WinRect *)&clamped, 0) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
        }
    }
    return err;
}

/* DT_AddValidRect (validate a rectangle of a port's visible window). STATE: MATCH 100%, exe 37 / obj 37, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050fac3
long DT_AddValidRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    ITRect clamped;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (port->window == kNoWindow) {
        err = kNotAWindow;
    } else if (IsWindowVisible(port->window) != 0) {
        CopyDT_2SysRect(&clamped, r);
        if (ValidateRect(port->window, (WinRect *)&clamped) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
        }
    }
    return err;
}

/* DT_SetOrigin (move a port's origin, answering the old one when asked). STATE: MATCH 100%, exe 33 / obj 33, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050fb26
long DT_SetOrigin(ITImage *image, long h, long v, ITPoint *old)
{
    ITPort *port;
    long err;
    POINT was;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (SetViewportOrgEx(port->dc, h, v, &was) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    } else if (old != 0) {
        CopySys2DT_Point(old, (ITPoint *)&was);
    }
    return err;
}

/* DT_DrawImage (draw an image, with an optional mask or transparency, into a port: through the
   blitter when the port's image has its own pixels, else as a DIB into the port's DC; the
   rectangles default to the source's bounds). STATE: MATCH 100%, exe 121 / obj 121, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0050fb7b
long DT_DrawImage(ITImage *image, const ITRect *dstRect, ITImage *src, ITImage *mask,
                  const ITTransparent *trans, const ITRect *srcRect)
{
    ITPort *port;
    ITPixels *srcPixels;
    ITPixels *maskPixels;
    long err;
    long e;
    ITRect from;
    ITRect to;
    ITPixelInfo info;
    unsigned char locked;
    unsigned char stored;
    unsigned char rebuilt;

    port = _DT_ContextGetContext(image);
    srcPixels = _DT_ImageGetImage(src);
    maskPixels = _DT_ImageGetAlpha(mask);
    err = 0;
    locked = 0;
    stored = 0;
    if (maskPixels != 0)
        DT_IsRectEqual(&srcPixels->bounds, &maskPixels->bounds);
    e = _DT_ContextLock(port);
    if (e != 0)
        goto fail;
    stored = 1;
    e = DT_ImageLock(src, mask, &info, &rebuilt);
    if (e != 0)
        goto fail;
    locked = 1;
    if (port->link.owner2->pixelsSelf != 0) {
        e = DT_ImageCopy(port->link.owner2->pixelsSelf, dstRect, 0, src, mask, trans, srcRect);
        if (e == 0)
            goto done;
    } else {
        CopyDT_2SysRect(&to, dstRect != 0 ? dstRect : &info.bounds);
        CopyDT_2SysRect(&from, srcRect != 0 ? srcRect : &info.bounds);
        e = DT_DrawDib(port->dc, &to, srcPixels->dib,
                         maskPixels != 0 ? maskPixels->dib : 0, trans, &from);
        if (e == 0)
            goto done;
    }
fail:
    err = e;
done:
    if (locked)
        DT_ImageUnlock(src, mask);
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

/* DT_DrawImageTile (draw an image, with an optional mask, tiled over a port: clipped to `clip'
   when given, else the whole device; origin moved while drawing when given; the tile is
   `srcRect' or the source's bounds). STATE: MATCH 100%, exe 266 / obj 266, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0050fd00
long DT_DrawImageTile(ITImage *image, const ITRect *clip, const ITPoint *origin, ITImage *src,
                  ITImage *mask, long unused, const ITRect *srcRect)
{
    ITPort *port;
    ITPixels *srcPixels;
    ITPixels *maskPixels;
    long err;
    long e;
    unsigned char locked;
    unsigned char stored;
    unsigned char moved;
    long col0;
    long col1;
    long row0;
    long row1;
    void *saved;
    long col;
    long row;
    ITRect tile;
    ITRect area;
    ITPoint was;
    ITRect box;
    ITPixelInfo info;
    unsigned char rebuilt;

    port = _DT_ContextGetContext(image);
    srcPixels = _DT_ImageGetImage(src);
    maskPixels = _DT_ImageGetAlpha(mask);
    err = 0;
    locked = 0;
    stored = 0;
    moved = 0;
    saved = 0;
    /* BUG: the mask's bounds are compared with the source's and the answer dropped */
    if (maskPixels != 0)
        DT_IsRectEqual(&srcPixels->bounds, &maskPixels->bounds);
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    e = DT_ImageLock(src, mask, &info, &rebuilt);
    if (e != 0) {
        err = e;
        goto done;
    }
    locked = 1;
    if (clip != 0) {
        saved = DT_GetClipRegion(image);
        if (saved == 0) {
            err = kOutOfMemory;
            goto done;
        }
        DT_GetRegionRect(&box, saved);
        e = DT_AddClipRect(image, clip);
        if (e != 0) {
            err = e;
            goto done;
        }
        clip = DT_RectIntersect(&box, &box, clip);
    }
    if (origin != 0) {
        e = DT_SetOrigin(image, origin->h, origin->v, &was);
        if (e != 0) {
            err = e;
            goto done;
        }
        moved = 1;
    }
    if (clip != 0)
        CopyDT_2SysRect(&area, clip);
    else
        SetRect((WinRect *)&area, 0, 0, GetDeviceCaps(port->dc, HORZRES),
                GetDeviceCaps(port->dc, VERTRES));
    CopyDT_2SysRect(&tile, srcRect != 0 ? srcRect : &info.bounds);
    if (area.left < 0)
        col0 = (area.left + 1) / (tile.right - tile.left) - 1;
    else
        col0 = area.left / (tile.right - tile.left);
    col1 = (area.right + tile.right - tile.left - 1) / (tile.right - tile.left);
    if (area.top < 0)
        row0 = (area.top + 1) / (tile.bottom - tile.top) - 1;
    else
        row0 = area.top / (tile.bottom - tile.top);
    row1 = (area.bottom + tile.bottom - tile.top - 1) / (tile.bottom - tile.top);
    area.top = row0 * tile.bottom;
    area.bottom = area.top + tile.bottom;
    for (row = row0; row < row1; row++) {
        area.left = col0 * tile.right;
        area.right = area.left + tile.right;
        for (col = col0; col < col1; col++) {
            e = DT_DrawDib(port->dc, &area, srcPixels->dib,
                               maskPixels != 0 ? maskPixels->dib : 0, 0, &tile);
            if (e != 0) {
                err = e;
                goto done;
            }
            area.left = area.right;
            area.right += tile.right;
        }
        area.top = area.bottom;
        area.bottom += tile.bottom;
    }
done:
    if (saved != 0) {
        DT_SetClipRegion(image, saved);
        DT_RegionDispose(saved);
    }
    if (locked)
        DT_ImageUnlock(src, 0);
    if (stored)
        _DT_ContextUnlock(port);
    if (moved)
        DT_SetOrigin(image, was.h, was.v, 0);
    return err;
}

/* DT_SetPixel (set one pixel of a port). STATE: MATCH 100%, exe 40 / obj 40, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00510042
long DT_SetPixel(ITImage *image, long x, long y, const ITColor *color)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    if (!SetPixelV(port->dc, x, y, DT_2SysColor(color))) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

/* DT_SetPenLocation (move a port's pen). STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x005100a4
long DT_SetPenLocation(ITImage *image, long x, long y)
{
    ITPort *port;
    long err;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (!MoveToEx(port->dc, x, y, 0)) {
        err = GetLastError();
        err = kOutOfMemory;
    }
    return err;
}

/* DT_DrawLineTo (draw a line with a port's pen to a point; a patterned pen is stroked as a
   widened path; a failure restoring the background mode retries the restore, as in the exe).
   STATE: MATCH 100%, exe 76 / obj 76, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005100d9
long DT_DrawLineTo(ITImage *image, long x, long y)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    int oldMode;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    oldMode = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    oldMode = SetBkMode(port->dc, TRANSPARENT);
    if (oldMode == 0)
        goto failed;
    if (port->penPattern != 0 && !BeginPath(port->dc))
        goto failed;
    if (!LineTo(port->dc, x, y))
        goto failed;
    if (port->penPattern != 0) {
        if (!EndPath(port->dc) || !WidenPath(port->dc))
            goto failed;
        if (!FillPath(port->dc)) {
            err = GetLastError();
            err = kOutOfMemory;
        }
    }
done:
    if (stored)
        _DT_ContextUnlock(port);
    if (oldMode != 0 && !SetBkMode(port->dc, oldMode)) {
failed:
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    return err;
}

/* DT_FrameRect (frame a rectangle with the port's pen, the pen centred on the inside of the
   edges). STATE: MATCH 100%, exe 118 / obj 118, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005101a6
long DT_FrameRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    int oldMode;
    ULong width;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    oldMode = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    width = port->penWidth;
    if (width == 0)
        width = 1;
    oldMode = SetBkMode(port->dc, TRANSPARENT);
    if (oldMode == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    if (!MoveToEx(port->dc, r->left + (unsigned short)width / 2, r->top + (unsigned short)width / 2, 0)
        || !LineTo(port->dc, r->right - ((unsigned short)width + 1) / 2, r->top + (unsigned short)width / 2)
        || !LineTo(port->dc, r->right - ((unsigned short)width + 1) / 2, r->bottom - ((unsigned short)width + 1) / 2)
        || !LineTo(port->dc, r->left + (unsigned short)width / 2, r->bottom - ((unsigned short)width + 1) / 2))
        goto failed;
    if (!LineTo(port->dc, r->left + (unsigned short)width / 2, r->top + (unsigned short)width / 2)) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (stored)
        _DT_ContextUnlock(port);
    /* BUG: a failed restore unlocks the port again and retries for as long as it fails */
    if (oldMode != 0 && !SetBkMode(port->dc, oldMode)) {
failed:
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    return err;
}

/* DT_PaintRect (fill a rectangle of a port with its current brush). STATE: MATCH 100%, exe 47 /
   obj 47, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00510302
long DT_PaintRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    ITRect clamped;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
    } else {
        stored = 1;
        if (!FillRect(port->dc, (WinRect *)CopyDT_2SysRect(&clamped, r),
                      GetCurrentObject(port->dc, OBJ_BRUSH))) {
            err = GetLastError();
            err = kOutOfMemory;
        }
    }
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

/* DT_EraseRect (erase a rectangle of a port with its background colour). STATE: MATCH 100%,
   exe 83 / obj 83, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00510374
long DT_EraseRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    void *brush;
    ULong color;
    ITRect clamped;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    brush = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    color = GetBkColor(port->dc);
    if (color == CLR_INVALID)
        goto fail;
    switch (color) {
    case kBlack:
        brush = GetStockObject(BLACK_BRUSH);
        break;
    case kGray7f:
    case kGray80:
        brush = GetStockObject(GRAY_BRUSH);
        break;
    case kWhite:
        brush = GetStockObject(WHITE_BRUSH);
        break;
    default:
        brush = CreateSolidBrush(color);
        if (brush == 0) {
fail:
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        break;
    }
    if (FillRect(port->dc, (WinRect *)CopyDT_2SysRect(&clamped, r), brush) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (brush != 0)
        DeleteObject(brush);
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

/* DT_InvertRect (invert a rectangle of a port). STATE: MATCH 100%, exe 43 / obj 43, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051044d
long DT_InvertRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    ITRect clamped;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
    } else {
        stored = 1;
        if (!InvertRect(port->dc, (WinRect *)CopyDT_2SysRect(&clamped, r))) {
            err = GetLastError();
            err = kOutOfMemory;
        }
    }
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

/* DT_GetTextWidth (the width of a run of text in a port's font; 0 on failure). STATE: MATCH 100%,
   exe 25 / obj 25, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005104b3
long DT_GetTextWidth(ITImage *image, const char *text, long length)
{
    ITPort *port;
    long err;
    long width;
    POINT size;

    port = _DT_ContextGetContext(image);
    err = 0;
    width = 0;
    if (!GetTextExtentPoint32A(port->dc, text, length, &size)) {
        err = GetLastError();
        err = kOutOfMemory;
    } else {
        width = size.x;
    }
    return width;
}

/* DT_DrawText (draw a run of text at a port's pen). STATE: MATCH 100%, exe 55 / obj 55,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005104f9
long DT_DrawText(ITImage *image, const char *text, long length)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    POINT pen;
    ITPoint where;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    if (!GetCurrentPositionEx(port->dc, &pen)) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    where.h = pen.x;
    where.v = pen.y;
    e = DT_DrawTextAt(image, where.h, where.v, text, length);
    if (e != 0)
        err = e;
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

/* DT_DrawTextAt (draw a run of text at a point of a port, background left alone). STATE: MATCH
   100%, exe 69 / obj 69, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00510579
long DT_DrawTextAt(ITImage *image, long x, long y, const char *text, long length)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    int oldMode;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    oldMode = SetBkMode(port->dc, TRANSPARENT);
    if (oldMode == 0)
        goto failed;
    if (!MoveToEx(port->dc, x, y, 0))
        goto failed;
    if (!TextOutA(port->dc, x, y, text, length)) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (stored)
        _DT_ContextUnlock(port);
    /* BUG: oldMode is read uninitialised when the port lock fails; a failed restore unlocks
       the port again and retries for as long as it fails */
    if (oldMode != 0 && !SetBkMode(port->dc, oldMode)) {
failed:
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    return err;
}

/* _DT_PrinterDispose (close a printer device: end or abort its document, unlock its DEVMODE, delete
   its DC; the open-device count drops either way). STATE: MATCH 100%, exe 42 / obj 42,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00510634
void _DT_PrinterDispose(ITDevice *device)
{
    long err;

    err = 0;
    if (device->dc != 0) {
        if (device->printing != 0) {
            if (device->status == kPrintAborted) {
                if (AbortDoc(device->dc) <= 0)
                    err = GetLastError();
            } else {
                if (EndDoc(device->dc) <= 0)
                    err = GetLastError();
            }
        }
        if (device->devMode != 0) {
            GlobalUnlock(device->devMode);
            device->devMode = 0;
        }
        if (DeleteDC(device->dc) == 0)
            err = GetLastError();
        device->dc = 0;
    }
    gDT_Printers--;
}

/* DT_CreateMacPixMap (a DIB record for `bounds' in a format key: its colour table from the DC, the
   given RGBQUADs or the system palette; bit-field masks where the DC's own format differs; a
   DIB section when CreateDIBSection is there, else a compatible bitmap when asked, else bits
   only; NULL on failure). STATE: MATCH 100%, exe 254 / obj 254, byte-identical yes. OPEN: none;
   rowBytes is one statement and both arms push a plain product (the first arm's store is the
   merged group's write-back, the second arm's temp is not merged: notes/itgfx.md) */
// FUNCTION: EVNOVA 0x005106b0
ITDib *DT_CreateMacPixMap(void *dc, const ITRect *bounds, ULong format, const RGBQuad *colors,
                    int withBitmap)
{
    ULong colorBytes;
    long err;
    void *bitmap;
    ULong screenKey;
    ITDib *dib;
    ULong depth;
    long width;
    long height;
    ULong rowBytes;
    unsigned char flag[4];
    unsigned char bits[4];
    unsigned char shift[4];
    unsigned char mask[4];
    unsigned short scale[4];

    err = 0;
    bitmap = 0;
    dib = 0;
    dib = 0;
    screenKey = 0;
    if (dc != 0)
        screenKey = DT_DCFormatKey(dc);
    depth = _DT_ImageCalcFormat(format, flag, bits, shift, mask, scale) * 8;
    width = bounds->right;
    width -= bounds->left;
    height = bounds->bottom;
    height -= bounds->top;
    rowBytes = ((width * depth + 31) & ~31) >> 3;
    colorBytes = 1 << depth;
    dib = MemoryAllocate(sizeof(ITDib));
    if (dib == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(dib, 0, sizeof(*dib));
    dib->header.biSize = kDibHeaderSize;
    dib->header.biWidth = width;
    dib->header.biHeight = -height;
    dib->header.biPlanes = 1;
    dib->header.biBitCount = depth;
    dib->header.biCompression = 0;
    dib->header.biSizeImage = 0;
    dib->header.biClrUsed = 0;
    dib->header.biClrImportant = 0;
    if (depth <= 8) {
        if (dc != 0) {
            if (GetDIBColorTable(dc, 0, 256, DIB_COLORS(dib)) == 0) {
                err = GetLastError();
                err = kDibFailed;
                goto done;
            }
        } else if (colors != 0) {
            if ((colorBytes <<= 2) != 0)
                memmove(DIB_COLORS(dib), colors, colorBytes);
        } else {
            _DT_PaletteToMacCTab((RGBQuad *)DIB_COLORS(dib), 0, &gDT_DefaultPalette);
        }
    } else if (depth == 16 && screenKey != format) {
        dib->header.biCompression = BI_BITFIELDS;
        DIB_COLORS(dib)[0] = (ULong)mask[1] << shift[1];
        DIB_COLORS(dib)[1] = (ULong)mask[2] << shift[2];
        DIB_COLORS(dib)[2] = (ULong)mask[3] << shift[3];
    } else if (depth == 32 && screenKey != format) {
        dib->header.biCompression = BI_BITFIELDS;
        DIB_COLORS(dib)[0] = (ULong)mask[1] << shift[1];
        DIB_COLORS(dib)[1] = (ULong)mask[2] << shift[2];
        DIB_COLORS(dib)[2] = (ULong)mask[3] << shift[3];
    }
    if (dc != 0 && gGDIDIBSectionFunc != 0 && dib->header.biCompression == 0) {
        if ((dib->dc = CreateCompatibleDC(dc)) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        dib->borrowedBits = 1;
        bitmap = ((CreateDIBSectionProc)gGDIDIBSectionFunc)(dib->dc, &dib->header, DIB_RGB_COLORS,
                                                      &dib->bits, 0, 0);
        if (bitmap == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        if ((dib->original = SelectObject(dib->dc, bitmap)) == 0) {
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
        bitmap = 0;
        goto done;
    }
    if (dc != 0 && withBitmap != 0) {
        if ((dib->dc = CreateCompatibleDC(dc)) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        bitmap = CreateCompatibleBitmap(dc, width, height);
        if (bitmap == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        if ((dib->original = SelectObject(dib->dc, bitmap)) == 0) {
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
        bitmap = 0;
        if ((dib->bits = MemoryAllocate(rowBytes * height)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        goto done;
    }
    if ((dib->bits = MemoryAllocate(rowBytes * height)) == 0)
        err = kOutOfMemory;
done:
    if (err != 0 && dib != 0) {
        DT_DisposeMacPixMap(dib);
        dib = 0;
    }
    if (bitmap != 0)
        DeleteObject(bitmap);
    return dib;
}

/* DT_DisposeMacPixMap (dispose a DIB record: its bitmap and DC, its bits unless borrowed). STATE:
   MATCH 100%, exe 37 / obj 37, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00510a09
void DT_DisposeMacPixMap(ITDib *dib)
{
    long err;

    err = 0;
    if (dib->dc != 0) {
        if (dib->original != 0)
            DeleteObject(SelectObject(dib->dc, dib->original));
        DeleteDC(dib->dc);
        if (dib->borrowedBits != 0)
            dib->bits = 0;
    }
    if (dib->bits != 0)
        MemoryDeallocate(dib->bits);
    memset(dib, 0, sizeof(*dib));
    MemoryDeallocate(dib);
}

/* the caller's block 0x005113c6 fills */
typedef struct ITImageBlock {
    ITImage image;
    ITPixels pixels;
} ITImageBlock;

/* DT_DrawDib (draw a DIB, or part of it, into a DC: copied (SetDIBitsToDevice, or
   StretchDIBits when the sizes differ or the map mode is not MM_TEXT); through a mask by
   reading the destination back into a DIB, blending with the blitter 0x004e8b5f and drawing
   that; through a transparent key colour with the AND / OR mask pair; an alpha of 0 draws
   nothing, 0xff is opaque and anything between is not drawn at all). STATE: MATCH 100%, exe
   618 / obj 618, byte-identical yes (2395 bytes). OPEN: none; the sizes are ULong (each arm's
   int arguments are one CSE'd conversion, stored at its first push) and every failure is
   written out at its site; notes/itgfx.md */
// FUNCTION: EVNOVA 0x00510a6b
long DT_DrawDib(void *dc, const ITRect *dst, ITDib *src, ITDib *mask,
                  const ITTransparent *trans, const ITRect *srcRect)
{
    long err;
    long e;
    long kind;
    ULong srcWidth;
    ULong srcHeight;
    ULong dstWidth;
    ULong dstHeight;
    ULong depth;
    ULong scans;
    long y;
    char *bits;
    int ok;
    ITDib *dib;
    void *bitmap;
    void *saved;
    void *memDC;
    void *colourDC;
    void *colourBitmap;
    void *colourSaved;
    void *maskDC;
    void *maskBitmap;
    void *maskSaved;
    ITRect whole;
    ITRect original;
    ITImageBlock dstBlock;
    ITImageBlock srcBlock;
    ITImageBlock maskBlock;
    ITRect from;
    ITRect to;
    ULong key;
    RGBQuad quad;

    err = 0;
    srcWidth = srcRect->right - srcRect->left;
    dstWidth = dst->right - dst->left;
    srcHeight = srcRect->bottom - srcRect->top;
    dstHeight = dst->bottom - dst->top;
    dib = 0;
    memDC = 0;
    bitmap = 0;
    saved = 0;
    colourDC = 0;
    colourBitmap = 0;
    colourSaved = 0;
    maskDC = 0;
    maskBitmap = 0;
    maskSaved = 0;
    if (trans != 0) {
        kind = trans->mode;
        switch (kind) {
        case kTransferCopy:
            mask = 0;
            break;
        case kTransferAlpha:
            if (trans->key.alpha == 0)
                goto done;
            if (trans->key.alpha == 0xff)
                kind = kTransferCopy;
            break;
        case kTransferAlphaMask:
            if (trans->key.alpha == 0)
                goto done;
            if (trans->key.alpha == 0xff)
                kind = kTransferMask;
            break;
        case kTransferKey:
            if (src->header.biBitCount == 8 && src->header.biClrUsed != 0) {
                (void)0;
            }
            break;
        case 1:
        case 2:
            break;
        }
    } else {
        kind = mask != 0 ? kTransferMask : kTransferCopy;
    }
    if (kind == kTransferMask) {
        SetRect((WinRect *)&whole, 0, 0, srcWidth, srcHeight);
        original = *srcRect;
        srcRect = &whole;
        if (srcWidth == dstWidth && srcHeight == dstHeight && GetObjectType(dc) == OBJ_MEMDC) {
            depth = GetDeviceCaps(dc, BITSPIXEL) <= 16 ? 16 : 24;
            bitmap = CreateCompatibleBitmap(dc, 1, 1);
            if (bitmap == 0) {
                err = GetLastError();
                err = kOutOfMemory;
                goto done;
            }
            saved = SelectObject(dc, bitmap);
            dib = DT_CreateMacPixMap(0, srcRect, depth, 0, 0);
            if (dib == 0) {
                err = kOutOfMemory;
                goto done;
            }
            if (!GetDIBits(dc, saved, 0, srcHeight, dib->bits, &dib->header, DIB_RGB_COLORS)) {
                err = GetLastError();
                err = kDibFailed;
                goto done;
            }
            SelectObject(dc, saved);
            saved = 0;
            DeleteObject(bitmap);
            bitmap = 0;
        } else {
            depth = GetDeviceCaps(dc, BITSPIXEL) <= 16 ? 16 : 24;
            memDC = CreateCompatibleDC(dc);
            if (memDC == 0) {
                err = GetLastError();
                err = kOutOfMemory;
                goto done;
            }
            bitmap = CreateBitmap(srcWidth, srcHeight, 1, depth, 0);
            if (bitmap == 0) {
                err = GetLastError();
                err = kOutOfMemory;
                goto done;
            }
            saved = SelectObject(memDC, bitmap);
            if (!GdiFlush()) {
                err = GetLastError();
                err = kDibFailed;
                goto done;
            }
            if (srcWidth == dstWidth && srcHeight == dstHeight)
                ok = BitBlt(memDC, srcRect->left, srcRect->top, srcWidth, srcHeight, dc,
                            dst->left, dst->top, SRCCOPY);
            else
                /* BUG: the source origin and size arguments are (dstWidth, dstHeight, left, top) */
                ok = StretchBlt(memDC, srcRect->left, srcRect->top, srcWidth, srcHeight, dc,
                                dstWidth, dstHeight, dst->left, dst->top, SRCCOPY);
            if (!ok) {
                err = GetLastError();
                err = kDibFailed;
                goto done;
            }
            SelectObject(memDC, saved);
            saved = 0;
            dib = DT_CreateMacPixMap(0, srcRect, depth, 0, 0);
            if (dib == 0) {
                err = kOutOfMemory;
                goto done;
            }
            if (!GetDIBits(memDC, bitmap, 0, srcHeight, dib->bits, &dib->header,
                           DIB_RGB_COLORS)) {
                err = GetLastError();
                err = kDibFailed;
                goto done;
            }
            DeleteObject(bitmap);
            bitmap = 0;
            DeleteDC(memDC);
            memDC = 0;
        }
        CopySys2DT_Rect(&from, &original);
        CopySys2DT_Rect(&to, &whole);
        e = DT_ImageCopy(_DT_StuffMacPixMap(dib, &dstBlock.image), &to, 0,
                         _DT_StuffMacPixMap(src, &srcBlock.image),
                         _DT_StuffMacPixMap(mask, &maskBlock.image), 0, &from);
        if (e != 0) {
            err = e;
            goto done;
        }
        src = dib;
    }
    if (kind == kTransferCopy || kind == kTransferMask) {
        if (!GdiFlush()) {
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
        scans = src->header.biHeight < 0 ? -src->header.biHeight : src->header.biHeight;
        y = scans - srcHeight - srcRect->top;
        if (srcWidth == dstWidth && srcHeight == dstHeight && GetMapMode(dc) == MM_TEXT) {
            ok = SetDIBitsToDevice(dc, dst->left, dst->top, dstWidth, dstHeight, srcRect->left,
                                   y, 0, scans, src->bits, &src->header, DIB_RGB_COLORS);
        } else {
            bits = (char *)src->bits +
                   (src->header.biBitCount * src->header.biWidth + 31) / 32 * 32 / 8 * srcRect->top;
            ok = StretchDIBits(dc, dst->left, dst->top, dstWidth, dstHeight, srcRect->left, 0,
                               srcWidth, srcHeight, bits, &src->header, DIB_RGB_COLORS, SRCCOPY);
        }
        if (!ok) {
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
        goto done;
    }
    if (kind != kTransferKey)
        goto done;
    if (src->header.biBitCount <= 8) {
        quad = src->colors[trans->key.alpha];
        key = RGB(quad.red, quad.green, quad.blue);
    } else {
        CopyDT_2SysColor(&key, &trans->key);
    }
    depth = GetDeviceCaps(dc, BITSPIXEL);
    colourDC = CreateCompatibleDC(dc);
    if (colourDC == 0) {
        err = kOutOfMemory;
        goto done;
    }
    colourBitmap = CreateBitmap(srcWidth, srcHeight, 1, depth, 0);
    if (colourBitmap == 0) {
        err = kOutOfMemory;
        goto done;
    }
    colourSaved = SelectObject(colourDC, colourBitmap);
    maskDC = CreateCompatibleDC(colourDC);
    if (maskDC == 0) {
        err = kOutOfMemory;
        goto done;
    }
    maskBitmap = CreateBitmap(srcWidth, srcHeight, 1, 1, 0);
    if (maskBitmap == 0) {
        err = kOutOfMemory;
        goto done;
    }
    maskSaved = SelectObject(maskDC, maskBitmap);
    if (!GdiFlush()) {
        err = GetLastError();
        err = kDibFailed;
        goto done;
    }
    scans = src->header.biHeight < 0 ? -src->header.biHeight : src->header.biHeight;
    y = scans - srcHeight - srcRect->top;
    if (!SetDIBitsToDevice(colourDC, 0, 0, srcWidth, srcHeight, srcRect->left, y, 0, scans,
                           src->bits, &src->header, DIB_RGB_COLORS)) {
        err = GetLastError();
        err = kDibFailed;
        goto done;
    }
    SetBkColor(colourDC, key);
    if (!GdiFlush()) {
        err = GetLastError();
        err = kDibFailed;
        goto done;
    }
    BitBlt(maskDC, 0, 0, srcWidth, srcHeight, colourDC, 0, 0, SRCCOPY);
    BitBlt(colourDC, 0, 0, srcWidth, srcHeight, maskDC, 0, 0, ROP_DSNA);
    if (srcWidth == dstWidth && srcHeight == dstHeight) {
        BitBlt(dc, dst->left, dst->top, dstWidth, dstHeight, maskDC, 0, 0, SRCAND);
        BitBlt(dc, dst->left, dst->top, dstWidth, dstHeight, colourDC, 0, 0, SRCPAINT);
    } else {
        StretchBlt(dc, dst->left, dst->top, dstWidth, dstHeight, maskDC, 0, 0, srcWidth,
                   srcHeight, SRCAND);
        StretchBlt(dc, dst->left, dst->top, dstWidth, dstHeight, colourDC, 0, 0, srcWidth,
                   srcHeight, SRCPAINT);
    }
done:
    if (dib != 0)
        DT_DisposeMacPixMap(dib);
    /* BUG: restores into memDC even when saved came from `dc', so the 1x1 stand-in stays
       selected there */
    if (saved != 0)
        SelectObject(memDC, saved);
    if (bitmap != 0)
        DeleteObject(bitmap);
    /* BUG: memDC is deleted under the bitmap's test */
    if (bitmap != 0)
        DeleteDC(memDC);
    if (colourSaved != 0)
        SelectObject(colourDC, colourSaved);
    if (colourBitmap != 0)
        DeleteObject(colourBitmap);
    if (colourDC != 0)
        DeleteDC(colourDC);
    if (maskSaved != 0)
        SelectObject(maskDC, maskSaved);
    if (maskBitmap != 0)
        DeleteObject(maskBitmap);
    if (maskDC != 0)
        DeleteDC(maskDC);
    return err;
}

/* _DT_StuffMacPixMap (an image in the caller's 0x28a4-byte block whose pixel map describes an existing
   DIB: format, depth, bounds, row bytes, palette and inverse table for indexed depths, key 0x8889
   when the palette is the greyscale one). STATE: MATCH 100%, exe 136 / obj 136.
   OPEN: none */
// FUNCTION: EVNOVA 0x005113c6
ITImage *_DT_StuffMacPixMap(ITDib *dib, ITImage *image)
{
    ITPixels *pixels;
    long count;
    ULong i;
    unsigned char grey;

    count = 0;
    pixels = 0;
    i = 0;
    grey = 0;
    memset(image, 0, kPixelsBlockSize);
    image->pixels = (ITPixels *)(image + 1);
    pixels = image->pixels;
    image->pixelsSelf = image;
    pixels->link.owner2 = image;
    pixels->link.owner = image;
    pixels->bytesPerPixel = _DT_ImageCalcFormat(DT_DibFormatKey(dib), &pixels->formatFlag, pixels->bits,
                                         pixels->shift, pixels->mask, pixels->scale);
    pixels->depth = dib->header.biBitCount;
    pixels->flags = 0;
    DT_SetRect(&pixels->bounds, 0, 0, dib->header.biWidth,
                 dib->header.biHeight < 0 ? -dib->header.biHeight : dib->header.biHeight);
    pixels->locks = 0;
    pixels->unk54 = 0;
    pixels->window = 0;
    pixels->drawDC = (long)dib->dc;
    pixels->dib = dib;
    pixels->rowBytes = ((pixels->bounds.right * pixels->depth + 31) & ~31) >> 3;
    pixels->baseAddr = 0;
    if (pixels->depth > 8) {
        pixels->palette.count = 0;
    } else {
        count = dib->header.biClrUsed;
        if (count == 0)
            count = 1 << dib->header.biBitCount;
        i = count;
        _DT_MacCTabToPalette(&pixels->palette, dib->colors, count);
        _DT_MacCTabToInverse(pixels->inverse, dib->colors, count);
        for (i = 0; i < gDT_AlphaPalette.count; i++) {
            if (gDT_AlphaPalette.entry[i].red != pixels->palette.entry[i].red
                || gDT_AlphaPalette.entry[i].green != pixels->palette.entry[i].green
                || gDT_AlphaPalette.entry[i].blue != pixels->palette.entry[i].blue)
                break;
        }
        if (i == gDT_AlphaPalette.count)
            grey = 1;
    }
    pixels->key = grey ? kPixelsKey : _DT_ImageCalcStamp(pixels);
    return image;
}

/* DT_SyncDib (flush GDI and move a DIB's pixels between its bitmap and its bits: into the
   bits, or with `store' out of them; kDibFailed when GDI refuses; `load', always !store at
   the call sites, is never read). STATE: MATCH 100%, exe 69 / obj 69, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00511546
long DT_SyncDib(ITDib *dib, unsigned char store, unsigned char load)
{
    long err;
    long width;
    unsigned int lines;
    void *old;

    err = 0;
    if (dib->dc != 0 && dib->borrowedBits == 0) {
        width = dib->header.biWidth;
        if (dib->header.biHeight < 0)
            lines = -dib->header.biHeight;
        else
            lines = dib->header.biHeight;
        if (GdiFlush() == 0)
            goto fail;
        if (store != 0) {
            old = SelectObject(dib->dc, dib->original);
            if (SetDIBits(dib->dc, old, 0, lines, dib->bits, &dib->header,
                          DIB_RGB_COLORS) == 0)
                goto fail;
            SelectObject(dib->dc, old);
        } else {
            old = SelectObject(dib->dc, dib->original);
            if (GetDIBits(dib->dc, old, 0, lines, dib->bits, &dib->header,
                          DIB_RGB_COLORS) == 0) {
fail:
                err = GetLastError();
                err = kDibFailed;
            } else {
                SelectObject(dib->dc, old);
            }
        }
    }
    return err;
}

/* _DT_PaletteToMacCTab (a palette as Windows RGBQUADs; the entry count through `count'). STATE:
   MATCH 100%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00511600
void _DT_PaletteToMacCTab(RGBQuad *quads, ULong *count, const ITPalette *palette)
{
    long err;
    ULong i;

    err = 0;
    if (count != 0)
        *count = palette->count;
    for (i = 0; i < palette->count; i++) {
        quads[i].blue = palette->entry[i].blue;
        quads[i].green = palette->entry[i].green;
        quads[i].red = palette->entry[i].red;
        quads[i].reserved = 0;
    }
}

/* _DT_MacCTabToPalette (Windows RGBQUADs as an opaque palette). STATE: MATCH 100%, exe 25 / obj 25, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00511643
void _DT_MacCTabToPalette(ITPalette *palette, const RGBQuad *quads, ULong count)
{
    ULong i;
    long err;

    err = 0;
    palette->count = count;
    for (i = 0; i < count; i++) {
        palette->entry[i].alpha = 0xff;
        palette->entry[i].red = quads[i].red;
        palette->entry[i].green = quads[i].green;
        palette->entry[i].blue = quads[i].blue;
    }
}

/* _DT_MacCTabToInverse (the 4-4-4 inverse table over Windows RGBQUADs; itblit.c's _DT_PaletteToInverse over
   a palette). STATE: MATCH 100%, exe 78 / obj 78, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00511681
void _DT_MacCTabToInverse(unsigned char *table, const RGBQuad *quads, ULong count)
{
    ULong i;
    ULong j;
    ULong best;
    long err;
    ULong bestDist;
    ULong dist;
    unsigned short qb;
    unsigned short qg;
    unsigned short qr;
    unsigned short r;
    unsigned short g;
    unsigned short b;

    err = 0;
    for (i = 0; i < kInverseTableSize; i++) {
        r = (i >> 8) & 0xf;
        r = r << 4 | r;
        g = (i >> 4) & 0xf;
        g = g << 4 | g;
        b = i & 0xf;
        b = b << 4 | b;
        j = best = 0;
        bestDist = 0xffffffff;
        for (; j < count; j++) {
            qr = quads[j].red;
            qg = quads[j].green;
            qb = quads[j].blue;
            dist = (g - qg) * (g - qg) + (r - qr) * (r - qr) + (b - qb) * (b - qb);
            if (dist < bestDist) {
                best = j;
                bestDist = dist;
            }
        }
        table[i] = best;
    }
}

/* the set bits of one bit-field mask, which follows the 0x28-byte header */
#define COUNT_BITS(n, i)                        \
    m = ((const ULong *)(info + 1))[i];         \
    if ((n = m) != 0) {                         \
        n = 0;                                  \
        do {                                    \
            n++;                                \
            m &= m - 1;                         \
        } while (m != 0);                       \
    }

/* DT_DibHeaderFormatKey (a DIB header's format key: the bit count, or for 16 / 32-bit bit-field DIBs
   the mask widths as nibbles, alpha taking the rest). STATE: MATCH 100%, exe 123 / obj 123,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00511774
ULong DT_DibHeaderFormatKey(const BitmapInfoHeader *info)
{
    ULong key;
    ULong total;
    ULong r;
    ULong g;
    ULong b;
    ULong m;

    key = 0;
    key = info->biBitCount;
    total = key;
    switch (key) {
    case 0:
    case 1:
    case 2:
    case 4:
    case 8:
    case 24:
        break;
    case 16:
        switch (info->biCompression) {
        case 0:
            break;
        case BI_BITFIELDS:
            COUNT_BITS(r, 0)
            COUNT_BITS(g, 1)
            COUNT_BITS(b, 2)
            key = r << 8 | g << 4 | b;
            total = 16;
            goto tail;
        }
        break;
    case 32:
        switch (info->biCompression) {
        case 0:
            break;
        case BI_BITFIELDS:
            COUNT_BITS(r, 0)
            COUNT_BITS(g, 1)
            COUNT_BITS(b, 2)
            key = r << 8 | g << 4 | b;
            total = 32;
        tail:
            key |= (total - r - g - b) << 12;
            break;
        }
        break;
    }
    return key;
}

/* DT_DibFormatKey (a DIB record's format key). STATE: MATCH 100%, exe 8 / obj 8, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00511899
ULong DT_DibFormatKey(const ITDib *dib)
{
    long err;

    err = 0;
    return DT_DibHeaderFormatKey(&dib->header);
}

/* DT_DCFormatKey (the format key of a DC's own bitmaps, read off a 1 x 1 compatible bitmap; 0
   when GDI refuses). STATE: MATCH 100%, exe 67 / obj 67, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005118ac
ULong DT_DCFormatKey(void *dc)
{
    ULong key;
    void *bitmap;
    DibInfo info;
    long err;

    err = 0;
    key = 0;
    bitmap = 0;
    bitmap = CreateCompatibleBitmap(dc, 1, 1);
    if (bitmap == 0) {
        err = kOutOfMemory;
        goto done;
    }
    memset(&info, 0, sizeof(info));
    info.size = kDibHeaderSize;
    if (GetDIBits(dc, bitmap, 0, 1, 0, &info, DIB_RGB_COLORS) == 0) {
        err = GetLastError();
        err = kDibFailed;
        goto done;
    }
    if (GetDIBits(dc, bitmap, 0, 1, 0, &info, DIB_RGB_COLORS) == 0) {
        err = GetLastError();
        err = kDibFailed;
        goto done;
    }
    key = DT_DibHeaderFormatKey((const BitmapInfoHeader *)&info);
done:
    if (bitmap != 0)
        DeleteObject(bitmap);
    return key;
}

/* DT_SurfaceCreateFromWindow (a new image on a window's whole client area). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051195d
ITImage *DT_SurfaceCreateFromWindow(void *window)
{
    return DT_SurfaceCreateLikeWindow(window, 0);
}

/* DT_SurfaceCreateLikeWindow (a new image on a window: its client area, or `area' clamped, in the window
   DC's own format, a compatible DIB behind it, the DC's colour table taken for indexed depths,
   and a port opened on the DIB's DC; NULL on failure). STATE: MATCH 100%, exe 240 / obj 240,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x0051196b
ITImage *DT_SurfaceCreateLikeWindow(void *window, const ITRect *area)
{
    ITImage *image;
    ITPort *port;
    ITPixels *pixels;
    void *owner;
    long err;
    void *dc;
    long e;
    ULong key;
    ULong n;
    long width;
    long height;
    RGBQuad quads[256];
    ITRect rect;

    image = 0;
    port = 0;
    pixels = 0;
    err = 0;
    owner = window;
    dc = 0;
    image = _DT_SurfaceCreate(&port, 0, &pixels);
    if (image == 0) {
        err = kOutOfMemory;
        goto done;
    }
    dc = GetDC(window);
    if (dc == 0)
        goto lastError;
    if (area != 0)
        CopyDT_2SysRect(&rect, area);
    else
        GetClientRect(window, (WinRect *)&rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    key = DT_DCFormatKey(dc);
    if (key == 0)
        goto failed;
    pixels->bytesPerPixel = _DT_ImageCalcFormat(key, &pixels->formatFlag, pixels->bits,
                                         pixels->shift, pixels->mask, pixels->scale);
    pixels->depth = pixels->bytesPerPixel << 3;
    pixels->flags = 1;
    CopySys2DT_Rect(&pixels->bounds, &rect);
    pixels->locks = 0;
    if ((pixels->dib = DT_CreateMacPixMap(dc, &rect, key, quads, 1)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    pixels->unk54 = (long)window;
    pixels->window = window;
    pixels->drawDC = (long)pixels->dib->dc;
    pixels->rowBytes = (((rect.right - rect.left) * pixels->depth + 31) & ~31) >> 3;
    pixels->baseAddr = 0;
    ATMemMove(pixels->info, &pixels->dib->header, sizeof(pixels->info));
    if (pixels->depth > 8) {
        pixels->palette.count = 0;
    } else {
        n = GetDIBColorTable((void *)pixels->drawDC, 0, 256, pixels->dib->colors);
        if (n == 0) {
lastError:
            err = GetLastError();
failed:
            err = kDibFailed;
            goto done;
        }
        pixels->dib->header.biClrUsed = n;
        _DT_MacCTabToPalette(&pixels->palette, pixels->dib->colors, n);
        _DT_MacCTabToInverse(pixels->inverse, pixels->dib->colors, n);
    }
    pixels->key = _DT_ImageCalcStamp(pixels);
    e = _DT_ContextCreate(port, 0, (void *)pixels->drawDC, 0);
    if (e != 0)
        err = e;
done:
    if (err != 0 && image != 0) {
        if (port->dc == 0) {
            image->port = 0;
            image->portSelf = 0;
        }
        _DT_SurfaceDispose(image, 0, 0, 0);
        image = 0;
        port = 0;
        pixels = 0;
    }
    if (dc != 0)
        ReleaseDC(window, dc);
    return pixels != 0 ? pixels->link.owner2 : 0;
}

/* DT_SurfaceDispose (dispose an image with no disposer named). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00511c66
long DT_SurfaceDispose(ITImage *image)
{
    return _DT_SurfaceDispose(image, 0, 0, 0);
}

/* DT_ImageCreate (a new offscreen image of a format key with an optional palette: its pixel map
   described, a DIB behind it, the palette and its inverse table kept for indexed depths, a
   format key taken (0x8889 for the greyscale palette); NULL on failure). STATE: MATCH 100%,
   exe 179 / obj 179, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00511c79
ITImage *DT_ImageCreate(const ITRect *bounds, ULong format, const ITPalette *palette)
{
    ITImage *image;
    ITPixels *pixels;
    long err;
    RGBQuad quads[256];
    ITRect clamped;

    image = 0;
    pixels = 0;
    err = 0;
    image = _DT_SurfaceCreate(0, 0, &pixels);
    if (image == 0) {
        err = kOutOfMemory;
        goto done;
    }
    pixels->bytesPerPixel = _DT_ImageCalcFormat(format, &pixels->formatFlag, pixels->bits,
                                         pixels->shift, pixels->mask, pixels->scale);
    pixels->depth = pixels->bytesPerPixel << 3;
    pixels->flags = 0;
    pixels->bounds = *bounds;
    pixels->locks = 0;
    CopyDT_2SysRect(&clamped, bounds);
    if (palette != 0 && palette->count != 0)
        _DT_PaletteToMacCTab(quads, 0, palette);
    /* BUG: quads is passed even when no palette filled it (uninitialised colours) */
    if ((pixels->dib = DT_CreateMacPixMap(0, &clamped, pixels->depth, quads, 0)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    pixels->unk54 = 0;
    pixels->window = 0;
    pixels->drawDC = 0;
    pixels->rowBytes = (((bounds->right - bounds->left) * pixels->depth + 31) & ~31) >> 3;
    pixels->baseAddr = 0;
    ATMemMove(pixels->info, &pixels->dib->header, sizeof(pixels->info));
    if (pixels->depth > 8) {
        pixels->palette.count = 0;
    } else {
        ATMemMove(&pixels->palette, palette, sizeof(pixels->palette));
        _DT_PaletteToInverse(pixels->inverse, palette);
    }
    pixels->key = palette == &gDT_AlphaPalette ? kPixelsKey : _DT_ImageCalcStamp(pixels);
done:
    if (err != 0 && pixels != 0) {
        _DT_SurfaceDispose(image, 0, 0, 0);
        image = 0;
        pixels = 0;
    }
    return pixels != 0 ? pixels->link.self : 0;
}

/* DT_ImageDispose (dispose an offscreen image, as its pixel map's disposer). STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00511ea4
long DT_ImageDispose(ITImage *image)
{
    return _DT_SurfaceDispose(image, 0, 0, 1);
}

/* _DT_ImageDispose (drop a pixel map's DIB). STATE: MATCH 100%, exe 22 / obj 22, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00511eb7
void _DT_ImageDispose(ITPixels *pixels)
{
    long err;

    err = 0;
    if (pixels->dib != 0)
        DT_DisposeMacPixMap(pixels->dib);
    pixels->unk54 = 0;
    pixels->window = 0;
    pixels->drawDC = 0;
    pixels->dib = 0;
    pixels->baseAddr = 0;
}

/* DT_SurfaceBlitRects (copy rectangles of a window image's pixels to the same places on its
   window). STATE: MATCH 100%, exe 8 / obj 8, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00511ee9
long DT_SurfaceBlitRects(ITImage *image, ULong count, const ITRect *rects)
{
    return DT_SurfaceCopyRects(image, count, rects, rects);
}

/* DT_SurfaceCopyRects (copy `count' rectangles of a window image's pixels to its window, each `src'
   to the top left of the matching `dst'; -7300 when GDI refuses). STATE: MATCH 100%, exe 110 /
   obj 110, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00511f00
long DT_SurfaceCopyRects(ITImage *image, ULong count, const ITRect *src, const ITRect *dst)
{
    ITPort *port;
    ITPixels *pixels;
    long err;
    long e;
    long width;
    void *dc;
    unsigned char locked;
    long top;
    long srcY;
    long lines;
    long left;
    long height;

    port = _DT_SurfaceGetContext(image);
    pixels = _DT_SurfaceGetImage(image);
    err = 0;
    locked = 0;
    dc = 0;
    if (count == 0)
        goto done;
    e = DT_ImageLock(image->pixelsSelf, 0, 0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    locked = 1;
    dc = GetDC(pixels->window);
    if (dc == 0 || GdiFlush() == 0)
        goto fail;
    while (count--) {
        left = src->left;
        width = src->right - left;
        top = src->top;
        height = src->bottom - top;
        lines = pixels->dib->header.biHeight < 0 ? -pixels->dib->header.biHeight : pixels->dib->header.biHeight;
        lines = pixels->bounds.bottom - pixels->bounds.top;
        srcY = lines - height - top;
        if (SetDIBitsToDevice(dc, dst->left, dst->top, width, height, left, srcY, 0, lines,
                              (void *)pixels->baseAddr, &pixels->dib->header,
                              DIB_RGB_COLORS) == 0) {
fail:
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
        src++;
        dst++;
    }
done:
    if (dc != 0)
        ReleaseDC(pixels->window, dc);
    if (locked)
        DT_ImageUnlock(image->pixelsSelf, 0);
    return err;
}

/* DT_SurfaceCheck (rebuild an image whose DIB no longer fits it: a window-backed one when the
   window's client area or the screen depth moved, an offscreen one when the DIB and the pixel map
   disagree; the fresh image's state is swapped in and the fresh shell disposed; `rebuilt' says
   whether it happened). STATE: MATCH 100%, exe 225 / obj 225, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00512061
long DT_SurfaceCheck(ITImage *image, unsigned char *rebuilt)
{
    ITPixels *pixels;
    long err;
    unsigned char changed;
    unsigned char swapped;
    ITImage *fresh;
    void *window;
    void *owner;
    void *dc;
    ULong depth;
    ITPalette palette;
    ITRect rect;
    WinRect client;
    RGBQuad quads[256];

    pixels = _DT_SurfaceGetImage(image);
    err = 0;
    changed = 0;
    swapped = 0;
    fresh = 0;
    if (rebuilt != 0)
        *rebuilt = 0;
    window = pixels->window;
    if (window != 0) {
        dc = 0;
        dc = GetDC(window);
        if (dc == 0) {
            err = GetLastError();
            err = kDibFailed;
            goto done;
        }
        GetClientRect(pixels->window, &client);
        CopySys2DT_Rect(&rect, (ITRect *)&client);
        depth = GetDeviceCaps(dc, BITSPIXEL);
        if (pixels->depth != depth || DT_IsRectEqual(&pixels->bounds, &rect) == 0)
            changed = 1;
        if (pixels->depth != (ULong)GetDeviceCaps((void *)pixels->drawDC, BITSPIXEL))
            changed = 1;
        if (pixels->depth != pixels->dib->header.biBitCount ||
            pixels->dib->header.biWidth != rect.right - rect.left ||
            rect.bottom - rect.top != (pixels->dib->header.biHeight < 0 ? -pixels->dib->header.biHeight : pixels->dib->header.biHeight))
            changed = 1;
        if (changed != 0 && depth == 8)
            _DT_MacCTabToPalette(&palette, quads, GetDIBColorTable(dc, 0, 256, quads));
        else
            palette.count = 0;
        ReleaseDC(pixels->window, dc);
    } else {
        DT_SetRect(&rect, 0, 0, pixels->dib->header.biWidth,
                     pixels->dib->header.biHeight < 0 ? -pixels->dib->header.biHeight : pixels->dib->header.biHeight);
        depth = pixels->dib->header.biBitCount;
        if (pixels->depth != depth ||
            pixels->bounds.right - pixels->bounds.left != rect.right - rect.left ||
            pixels->bounds.bottom - pixels->bounds.top != rect.bottom - rect.top)
            changed = 1;
        if (changed != 0 && depth == 8)
            _DT_MacCTabToPalette(&palette, pixels->dib->colors,
                         pixels->dib->header.biClrUsed != 0 ? pixels->dib->header.biClrUsed : 1 << depth);
        else
            palette.count = 0;
    }
    if (changed != 0) {
        owner = pixels->window;
        if (owner != 0) {
            fresh = DT_SurfaceCreateLikeWindow(owner, (pixels->flags & kPixelsWholeWindow) != 0 ? 0 : &pixels->bounds);
            if (fresh == 0)
                goto nomem;
            _DT_SurfaceReplace(image, fresh);
        } else {
            fresh = DT_ImageCreate(&rect, depth, &palette);
            if (fresh == 0) {
nomem:
                err = kOutOfMemory;
                goto done;
            }
            _DT_ImageReplace(image->pixelsSelf, fresh->pixelsSelf);
            swapped = 1;
        }
    }
done:
    if (fresh != 0)
        _DT_SurfaceDispose(fresh, 0, 0, swapped);
    if (rebuilt != 0)
        *rebuilt = changed;
    return err;
}

/* _DT_ImageLock (lock an image's pixels, and `other''s if it is an image: GDI flushed when DIB
   sections are in use, the first lock rebuilding a stale DIB (0x00512061) and refreshing an
   inverse table whose lock bit was set; the lock count up and the bits pointer published at
   +0x4c). STATE: MATCH 100%, exe 98 / obj 98, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051231a
long _DT_ImageLock(ITImage *image, ITImage *other, unsigned char *rebuilt)
{
    ITPixels *pixels;
    ITPixels *otherPixels;
    long err;
    long e;
    long result;

    pixels = _DT_ImageGetImage(image);
    otherPixels = _DT_ImageGetAlpha(other);
    err = 0;
    result = 0;
    if (rebuilt != 0)
        *rebuilt = 0;
    if (gGDIDIBSectionFunc != 0 && GdiFlush() == 0) {
        err = GetLastError();
        err = kDibFailed;
        goto done;
    }
    if (pixels->locks == 0) {
        e = DT_SurfaceCheck(image, rebuilt);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (AtomicMaskClear(&pixels->flags, kPixelsLockBit) != 0)
            _DT_PaletteToInverse(pixels->inverse, &pixels->palette);
        if (otherPixels != 0 && AtomicMaskClear(&otherPixels->flags, kPixelsLockBit) != 0)
            _DT_PaletteToInverse(otherPixels->inverse, &otherPixels->palette);
    }
    AtomicIncrement(&pixels->locks, 1);
    pixels->baseAddr = (long)pixels->dib->bits;
    if (otherPixels != 0) {
        AtomicIncrement(&otherPixels->locks, 1);
        otherPixels->baseAddr = (long)otherPixels->dib->bits;
    }
done:
    return err;
}

/* _DT_ImageUnlock (unlock two images' pixel maps, the second only if it is one; the last unlock
   clears the lock bit). STATE: MATCH 100%, exe 49 / obj 49, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00512411
void _DT_ImageUnlock(ITImage *image, ITImage *other)
{
    ITPixels *pixels;
    ITPixels *otherPixels;
    long err;

    pixels = _DT_ImageGetImage(image);
    otherPixels = _DT_ImageGetAlpha(other);
    err = 0;
    if (otherPixels != 0 && AtomicIncrement(&otherPixels->locks, -1) == 0) {
        otherPixels->baseAddr = 0;
        AtomicMaskClear(&otherPixels->flags, kPixelsLockBit);
    }
    if (AtomicIncrement(&pixels->locks, -1) == 0) {
        pixels->baseAddr = 0;
        AtomicMaskClear(&pixels->flags, kPixelsLockBit);
    }
}
