/* itgfxmac.c -- 1.0.10: the draw tool's QuickDraw layer, win's itgfx.c on the Mac. Notes: Research/notes/maclibs.md */
#undef OPAQUE_TOOLBOX_STRUCTS
#define OPAQUE_TOOLBOX_STRUCTS 1
#if !TARGET_API_MAC_CARBON
#include <Printing.h>
#endif
#undef OPAQUE_TOOLBOX_STRUCTS
#define OPAQUE_TOOLBOX_STRUCTS 0

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

#define kTransferKey 5

/* not in these Interfaces */
typedef void *PMPrintSession;
#define kPMCancel 0x80
extern OSStatus PMSessionEndPage(PMPrintSession session);
extern OSStatus PMSessionEndDocument(PMPrintSession session);
extern OSStatus PMSessionError(PMPrintSession session);
extern OSStatus PMRelease(void *object);

#if TARGET_API_MAC_CARBON
#define PortFont(p)         GetPortTextFont(p)
#define PortSize(p)         GetPortTextSize(p)
#define PortFace(p)         GetPortTextFace(p)
#define PortMode(p)         GetPortTextMode(p)
#define PortPenMode(p)      GetPortPenMode(p)
#define PortBounds(p, r)    GetPortBounds(p, r)
#else
#define PortFont(p)         ((p)->txFont)
#define PortSize(p)         ((p)->txSize)
#define PortFace(p)         ((p)->txFace)
#define PortMode(p)         ((p)->txMode)
#define PortPenMode(p)      ((p)->pnMode)
#define PortBounds(p, r)    (*(r) = (p)->portRect)
#endif
#define kAlignCentre 0
#define kPrintAborted -7007
#define kPrintFailed -7352
unsigned char gDT_TempMem;
ULong gDT_Printers;
unsigned char gDT_Open;

long DT_DrawTextAt(ITImage *image, long x, long y, const char *text, long length);
#define kByteOrderMark  0xfeff
#define kTextFailed     -7300

extern long ATSU_Measure(const unsigned short *text, ULong start, ULong length, float *width);
extern long ATSU_DrawText(const unsigned short *text, ULong start, ULong length);

extern RGBColor *CopyDT_2SysColor(RGBColor *ref, const ITColor *color);
extern RgnHandle DT_RegionCreate(void);

long _DT_ContextCreate(ITPort *port, void *window, CGrafPtr dc, unsigned char save);
void *DT_GetClipRegion(ITImage *image);
long DT_SetClipRegion(ITImage *image, void *region);

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

long DT_ContextDispose(ITImage *image)
{
    return _DT_SurfaceDispose(image, 1, 0, 0);
}

long _DT_ContextCreate(ITPort *port, void *window, CGrafPtr dc, unsigned char save)
{
    long err;
    CGrafPtr oldWorld;
    GDHandle oldDevice;

    err = 0;
    oldWorld = 0;
    oldDevice = 0;
    if (dc != 0) {
        port->dc = dc;
        port->window = 0;
    } else {
        port->window = window;
        port->dc = GetWindowPort(window);
        dc = port->dc;
    }
#if TARGET_API_MAC_CARBON
    if (dc != 0)
        IsPortColor(dc);
#endif
    if (save != 0) {
        GetGWorld(&oldWorld, &oldDevice);
        SetGWorld(port->dc, GetMainDevice());
        GetForeColor(&port->savedFore);
        GetBackColor(&port->savedBack);
        GetPenState(&port->savedPen);
        port->savedFont = PortFont(dc);
        port->savedSize = PortSize(dc);
        port->savedFace = PortFace(dc);
        port->savedMode = PortMode(dc);
        SetGWorld(oldWorld, oldDevice);
    }
    port->savedDC = save;
    if ((port->clip = DT_GetClipRegion(port->link.self)) == 0)
        err = kOutOfMemory;
    return err;
}

void _DT_ContextDispose(ITPort *port)
{
    CGrafPtr oldWorld;
    GDHandle oldDevice;

    oldWorld = 0;
    oldDevice = 0;
    if (port->savedDC != 0) {
        GetGWorld(&oldWorld, &oldDevice);
        SetGWorld(port->dc, GetMainDevice());
        if (port->clip != 0)
            SetClip(port->clip);
        RGBForeColor(&port->savedFore);
        RGBBackColor(&port->savedBack);
        SetPenState(&port->savedPen);
        TextFont(port->savedFont);
        TextSize(port->savedSize);
        TextFace(port->savedFace);
        TextMode(port->savedMode);
        SetGWorld(oldWorld, oldDevice);
    }
    if (port->clip != 0)
        DT_RegionDispose(port->clip);
    port->window = 0;
    port->dc = 0;
}

long _DT_ContextLock(ITPort *port)
{
    ITPixels *pixels;
    ULong locks;

    if (port->link.owner2->pixelsSelf != 0) {
        pixels = _DT_SurfaceGetImage(port->link.owner2);
        locks = pixels->locks;
        _DT_ImageLock(pixels->link.owner2->pixelsSelf, 0, 0);
        if (locks != 0 || (pixels->flags & kPixelsBitsChanged) != 0)
            pixels->flags &= ~kPixelsBitsChanged;
    }
    return 0;
}

void _DT_ContextUnlock(ITPort *port)
{
    ITPixels *pixels;

    if (port->link.owner2->pixelsSelf != 0) {
        pixels = _DT_SurfaceGetImage(port->link.owner2);
        _DT_ImageUnlock(pixels->link.owner2->pixelsSelf, 0);
        if (pixels->locks != 0)
            pixels->flags &= ~kPixelsInBitmap;
        else
            pixels->flags |= kPixelsInBitmap;
    }
}

long DT_ResetAttributes(ITImage *image, unsigned char pen, unsigned char brush, unsigned char font,
                        unsigned char clip, unsigned char origin)
{
    ITPort *port;
    long err;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    FontInfo info;
    long e;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    if (pen || brush) {
        ForeColor(blackColor);
        BackColor(whiteColor);
        PenNormal();
    }
    if (font) {
        TextFont(1);
        TextSize(0);
        TextFace(0);
        TextMode(1);
        GetFontInfo(&info);
        GetFontName(PortFont(port->dc), port->fontFace);
        port->textAlign = -1;
        port->maxWidth = info.widMax;
        port->height = (port->ascent = info.ascent) + (port->descent = info.descent) + (port->leading = info.leading);
    }
    if (clip && port->clip != 0) {
        e = DT_SetClipRegion(image, port->clip);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (origin)
        SetOrigin(0, 0);
    SetGWorld(oldWorld, oldDevice);
done:
    return err;
}

long DT_SetFillAttributes(ITImage *image, const ITColor *fore, const ITColor *back, const unsigned char *pattern)
{
    ITPort *port;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    RGBColor color;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    if (fore != 0)
        RGBForeColor(CopyDT_2SysColor(&color, fore));
    if (back != 0)
        RGBBackColor(CopyDT_2SysColor(&color, back));
    if (pattern != 0)
        PenPat((const Pattern *)pattern);
    SetGWorld(oldWorld, oldDevice);
    return 0;
}

long DT_GetFillColor(ITImage *image, ITColor *fore)
{
    ITPort *port;
#if TARGET_API_MAC_CARBON
    RGBColor rgb;
#endif

    port = _DT_ContextGetContext(image);
#if TARGET_API_MAC_CARBON
    CopySys2DT_Color(fore, GetPortForeColor(port->dc, &rgb));
#else
    CopySys2DT_Color(fore, &port->dc->rgbFgColor);
#endif
    return 0;
}

long DT_GetFillBackColor(ITImage *image, ITColor *back)
{
    ITPort *port;
#if TARGET_API_MAC_CARBON
    RGBColor rgb;
#endif

    port = _DT_ContextGetContext(image);
#if TARGET_API_MAC_CARBON
    CopySys2DT_Color(back, GetPortBackColor(port->dc, &rgb));
#else
    CopySys2DT_Color(back, &port->dc->rgbBkColor);
#endif
    return 0;
}

long DT_SetFillBackColor(ITImage *image, const ITColor *back)
{
    ITPort *port;

    port = _DT_ContextGetContext(image);
    return DT_SetFillAttributes(image, 0, back, 0);
}

long DT_SetLineAttributes(ITImage *image, const ITColor *fore, const ITColor *back, const unsigned char *pattern, long width)
{
    ITPort *port;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    RGBColor color;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    if (fore != 0)
        RGBForeColor(CopyDT_2SysColor(&color, fore));
    if (back != 0)
        RGBBackColor(CopyDT_2SysColor(&color, back));
    if (pattern != 0)
        PenPat((const Pattern *)pattern);
    if (width != (ULong)-1) {
        if ((ULong)width == 0)
            width = 1;
        PenSize(width, width);
    }
    SetGWorld(oldWorld, oldDevice);
    return 0;
}

static inline void ATC2PStr(unsigned char *dst, const char *src)
{
    ULong n;
    unsigned char c;
    unsigned char *p;
    const unsigned char *s;

    p = dst;
    s = (const unsigned char *)src - 1;
    n = -1;
    while (++n < 0xff && (c = *++s) != 0)
        *++p = c;
    *dst = n;
}

static inline long ATStrCmp(const char *a, const char *b)
{
    ULong ca;
    ULong cb;
    const unsigned char *s1;
    const unsigned char *s2;

    s1 = (const unsigned char *)a - 1;
    s2 = (const unsigned char *)b - 1;
    while ((ca = *++s1) == (cb = *++s2)) {
        if (ca == 0)
            return 0;
    }
    return ca < cb ? (signed char)-1 : (signed char)1;
}

/* Stand-in: the linker strips this; the exe keeps its literals (body not recoverable). */
unsigned char DT_IsGenericFontName(const char *name)
{
    return ATStrCmp(name, "SYSTEM") == 0 || ATStrCmp(name, "MONOSPACE") == 0 || ATStrCmp(name, "FIXED") == 0 ||
           ATStrCmp(name, "SANS-SERIF") == 0 || ATStrCmp(name, "SANS") == 0 || ATStrCmp(name, "SERIF") == 0;
}

/* MSL's strcpy, which the exe has inline */
static inline char *MslCopy(char *dst, const char *src)
{
    unsigned char *q = (unsigned char *)dst - 1;
    const unsigned char *p = (const unsigned char *)src - 1;

    while ((*++q = *++p) != 0)
        ;
    return dst;
}

char *_DT_GetFontByNameList(CGrafPtr dc, const char *faces, char *out)
{
    const char *p;
    ULong length;
    ULong i;
    char name[256];
    Str255 pname;
    Str255 found;
    short family;

    for (p = faces; *faces != 0; p++) {
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == ' ' || *p == '_' ||
            *p == '-')
            continue;
        if (p != faces) {
            length = p - faces < 0xff ? p - faces : 0xff;
            if (length != 0)
                BlockMoveData(faces, name, length);
            name[length] = 0;
            for (i = 0; i < length; i++) {
                if (name[i] == '_')
                    name[i] = ' ';
            }
            ATC2PStr(pname, name);
            GetFNum(pname, &family);
            if (family != 0)
                break;
            GetFontName(family, found);
            if (EqualString(pname, found, 0, 0))
                break;
        }
        String2UpperCopySafe(name, name, -1);
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
    MslCopy(out, *faces != 0 ? name : "");
    return *out != 0 ? out : 0;
}

long DT_SetTextAttributes(ITImage *image, const ITColor *color, const char *face, long fontId,
                          long size, ULong style, long align)
{
    ITPort *port;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    short family;
    RGBColor rgb;
    FontInfo info;
    char name[256];

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    if (face != 0)
        face = _DT_GetFontByNameList(port->dc, face, name);
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    if (face != 0 && *face != 0) {
        ATC2PStr(port->fontFace, face);
        GetFNum(port->fontFace, &family);
        fontId = family;
    }
    if (color != 0)
        RGBForeColor(CopyDT_2SysColor(&rgb, color));
    TextFont(fontId);
    TextSize(size);
    TextFace(style);
    TextMode(1);
    port->textAlign = align;
    GetFontInfo(&info);
    GetFontName(fontId, port->fontFace);
    port->maxWidth = info.widMax;
    port->height = (port->ascent = info.ascent) + (port->descent = info.descent) + (port->leading = info.leading);
    SetGWorld(oldWorld, oldDevice);
    return 0;
}

long DT_SetTextColor(ITImage *image, const ITColor *fore)
{
    ITPort *port;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    RGBColor color;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    RGBForeColor(CopyDT_2SysColor(&color, fore));
    SetGWorld(oldWorld, oldDevice);
    return 0;
}

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
    if (size != PortSize(port->dc))
        changed = 1;
    font = PortFont(port->dc);
    style = PortFace(port->dc);
    flags = port->textAlign;
    if (changed)
        err = DT_SetTextAttributes(image, 0, 0, font, size, style, flags);
    return err;
}

long DT_GetTextInfo(ITImage *image, long *metrics)
{
    ITPort *port;

    port = _DT_ContextGetContext(image);
    BlockMoveData(&port->ascent, metrics, 0x14);
    return 0;
}

void *DT_GetClipRegion(ITImage *image)
{
    ITPort *port;
    long err;
    RgnHandle region;

    port = _DT_ContextGetContext(image);
    err = 0;
    region = DT_RegionCreate();
    if (region == 0)
        err = kOutOfMemory;
    else
#if TARGET_API_MAC_CARBON
        GetPortClipRegion(port->dc, region);
#else
        CopyRgn(port->dc->clipRgn, region);
#endif
    if (err != 0 && region != 0) {
        DT_RegionDispose(region);
        region = 0;
    }
    return region;
}

long DT_AddClipRegion(ITImage *image, RgnHandle region)
{
    ITPort *port;
#if TARGET_API_MAC_CARBON
    long err;
    RgnHandle clip;

    port = _DT_ContextGetContext(image);
    err = 0;
    clip = NewRgn();
    if (clip == 0) {
        err = kOutOfMemory;
    } else {
        GetPortClipRegion(port->dc, clip);
        SectRgn(region, clip, clip);
        SetPortClipRegion(port->dc, clip);
        DisposeRgn(clip);
    }
    return err;
#else

    port = _DT_ContextGetContext(image);
    SectRgn(region, port->dc->clipRgn, port->dc->clipRgn);
    return 0;
#endif
}

long DT_AddClipRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    RgnHandle region;

    port = _DT_ContextGetContext(image);
    err = 0;
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

long DT_SetClipRegion(ITImage *image, void *region)
{
    ITPort *port;

    port = _DT_ContextGetContext(image);
#if TARGET_API_MAC_CARBON
    SetPortClipRegion(port->dc, region);
#else
    CopyRgn(region, port->dc->clipRgn);
#endif
    return 0;
}

long DT_SetClipRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    Rect box;
#if TARGET_API_MAC_CARBON
    GrafPtr old;
#endif

    port = _DT_ContextGetContext(image);
#if TARGET_API_MAC_CARBON
    GetPort(&old);
    SetPort(port->dc);
    ClipRect(CopyDT_2SysRect(&box, r));
    SetPort(old);
#else
    RectRgn(port->dc->clipRgn, CopyDT_2SysRect(&box, r));
#endif
    return 0;
}

long DT_AddInvalidRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
#if !TARGET_API_MAC_CARBON
    GrafPtr old;
#endif
    Rect box;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (port->window == 0) {
        err = kNotAWindow;
    } else {
#if TARGET_API_MAC_CARBON
        InvalWindowRect(port->window, CopyDT_2SysRect(&box, r));
#else
        GetPort(&old);
        SetPort((GrafPtr)port->dc);
        InvalRect(CopyDT_2SysRect(&box, r));
        SetPort(old);
#endif
    }
    return err;
}

long DT_AddValidRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
#if !TARGET_API_MAC_CARBON
    GrafPtr old;
#endif
    Rect box;

    port = _DT_ContextGetContext(image);
    err = 0;
    if (port->window == 0) {
        err = kNotAWindow;
    } else {
#if TARGET_API_MAC_CARBON
        ValidWindowRect(port->window, CopyDT_2SysRect(&box, r));
#else
        GetPort(&old);
        SetPort((GrafPtr)port->dc);
        ValidRect(CopyDT_2SysRect(&box, r));
        SetPort(old);
#endif
    }
    return err;
}

long DT_SetOrigin(ITImage *image, long h, long v, ITPoint *oldOrigin)
{
    ITPort *port;
    Point origin;
    GrafPtr old;
    Rect box;

    port = _DT_ContextGetContext(image);
    GetPort(&old);
    SetPort((GrafPtr)port->dc);
    PortBounds(port->dc, &box);
    SetOrigin(box.left, box.top);
    SetPort(old);
    if (oldOrigin != 0)
        CopySys2DT_Point(oldOrigin, &origin);
    return 0;
}

long DT_DrawImage(ITImage *image, const ITRect *dstRect, ITImage *src, ITImage *mask,
                  const ITTransparent *trans, const ITRect *srcRect)
{
    ITPort *port;
    ITPixels *srcPixels;
    ITPixels *maskPixels;
    long err;
    long e;
    Rect from;
    Rect to;
    ITPixelInfo info;
    unsigned char locked;
    unsigned char stored;
    unsigned char rebuilt;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    RGBColor oldFore;
    RGBColor oldBack;
    RGBColor key;

    port = _DT_ContextGetContext(image);
    srcPixels = _DT_ImageGetImage(src);
    maskPixels = _DT_ImageGetAlpha(mask);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    locked = 0;
    stored = 0;
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
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    GetForeColor(&oldFore);
    GetBackColor(&oldBack);
    ForeColor(blackColor);
    BackColor(whiteColor);
    if (dstRect != 0)
        CopyDT_2SysRect(&to, dstRect);
    else
        PortBounds(port->dc, &to);
    CopyDT_2SysRect(&from, srcRect != 0 ? srcRect : &info.bounds);
    if (maskPixels != 0) {
        SInt8 srcState;
        SInt8 maskState;
#if !TARGET_API_MAC_CARBON
        SInt8 dstState;
#endif

        srcState = HGetState((Handle)srcPixels->dib);
        maskState = HGetState((Handle)maskPixels->dib);
#if !TARGET_API_MAC_CARBON
        dstState = HGetState((Handle)port->dc->portPixMap);
#endif
        HLock((Handle)srcPixels->dib);
        HLock((Handle)maskPixels->dib);
#if TARGET_API_MAC_CARBON
        CopyDeepMask((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*(PixMapHandle)maskPixels->dib,
                     GetPortBitMapForCopyBits(port->dc), &from, &from, &to, 0, 0);
        HSetState((Handle)srcPixels->dib, srcState);
        HSetState((Handle)maskPixels->dib, maskState);
#else
        HLock((Handle)port->dc->portPixMap);
        CopyDeepMask((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*(PixMapHandle)maskPixels->dib,
                     (BitMap *)*port->dc->portPixMap, &from, &from, &to, 0, 0);
        HSetState((Handle)srcPixels->dib, srcState);
        HSetState((Handle)maskPixels->dib, maskState);
        HSetState((Handle)port->dc->portPixMap, dstState);
#endif
    } else if (trans != 0 && trans->mode == kTransferKey) {
        SInt8 srcState;
#if !TARGET_API_MAC_CARBON
        SInt8 dstState;
#endif

        srcState = HGetState((Handle)srcPixels->dib);
#if !TARGET_API_MAC_CARBON
        dstState = HGetState((Handle)port->dc->portPixMap);
#endif
        HLock((Handle)srcPixels->dib);
#if !TARGET_API_MAC_CARBON
        HLock((Handle)port->dc->portPixMap);
#endif
        if ((*(PixMapHandle)srcPixels->dib)->pixelType == RGBDirect)
            CopyDT_2SysColor(&key, &trans->key);
        else
            key = (*(*(PixMapHandle)srcPixels->dib)->pmTable)->ctTable[trans->key.alpha].rgb;
        RGBBackColor(&key);
#if TARGET_API_MAC_CARBON
        CopyBits((BitMap *)*(PixMapHandle)srcPixels->dib, GetPortBitMapForCopyBits(port->dc), &from, &to, transparent, 0);
        HSetState((Handle)srcPixels->dib, srcState);
    } else {
        SInt8 srcState;

        srcState = HGetState((Handle)srcPixels->dib);
        HLock((Handle)srcPixels->dib);
        CopyBits((BitMap *)*(PixMapHandle)srcPixels->dib, GetPortBitMapForCopyBits(port->dc), &from, &to, srcCopy, 0);
        HSetState((Handle)srcPixels->dib, srcState);
    }
#else
        CopyBits((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*port->dc->portPixMap, &from, &to, transparent, 0);
        HSetState((Handle)srcPixels->dib, srcState);
        HSetState((Handle)port->dc->portPixMap, dstState);
    } else {
        SInt8 srcState;
        SInt8 dstState;

        srcState = HGetState((Handle)srcPixels->dib);
        dstState = HGetState((Handle)port->dc->portPixMap);
        HLock((Handle)srcPixels->dib);
        HLock((Handle)port->dc->portPixMap);
        CopyBits((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*port->dc->portPixMap, &from, &to, srcCopy, 0);
        HSetState((Handle)srcPixels->dib, srcState);
        HSetState((Handle)port->dc->portPixMap, dstState);
    }
#endif
    RGBForeColor(&oldFore);
    RGBBackColor(&oldBack);
    SetGWorld(oldWorld, oldDevice);
done:
    if (locked)
        DT_ImageUnlock(src, mask);
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_DrawImageTile(ITImage *image, const ITRect *clip, const ITPoint *origin, ITImage *src,
                      ITImage *mask, const ITTransparent *trans, const ITRect *srcRect)
{
    ITPort *port;
    ITPixels *srcPixels;
    ITPixels *maskPixels;
    long err;
    long e;
    unsigned char locked;
    unsigned char stored;
    unsigned char moved;
    long col;
    long col0;
    long row0;
    long col1;
    void *saved;
    long row;
    long row1;
    short mode;
    ITPixelInfo info;
    ITRect box;
    Rect tile;
    Rect area;
    ITPoint was;
    RGBColor oldFore;
    RGBColor oldBack;
    RGBColor key;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    unsigned char rebuilt;

    port = _DT_ContextGetContext(image);
    srcPixels = _DT_ImageGetImage(src);
    maskPixels = _DT_ImageGetAlpha(mask);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    locked = 0;
    stored = 0;
    moved = 0;
    saved = 0;
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
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    GetForeColor(&oldFore);
    GetBackColor(&oldBack);
    ForeColor(blackColor);
    BackColor(whiteColor);
    if (clip != 0)
        CopyDT_2SysRect(&area, clip);
    else
        PortBounds(port->dc, &area);
    CopyDT_2SysRect(&tile, srcRect != 0 ? srcRect : &info.bounds);
    if (maskPixels == 0 && trans != 0 && trans->mode == kTransferKey) {
        if ((*(PixMapHandle)srcPixels->dib)->pixelType == RGBDirect)
            CopyDT_2SysColor(&key, &trans->key);
        else
            key = (*(*(PixMapHandle)srcPixels->dib)->pmTable)->ctTable[trans->key.alpha].rgb;
        RGBBackColor(&key);
        mode = transparent;
    } else {
        mode = srcCopy;
    }
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
#if TARGET_API_MAC_CARBON
            if (maskPixels != 0)
                CopyDeepMask((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*(PixMapHandle)maskPixels->dib,
                             GetPortBitMapForCopyBits(port->dc), &tile, &tile, &area, 0, 0);
            else
                CopyBits((BitMap *)*(PixMapHandle)srcPixels->dib, GetPortBitMapForCopyBits(port->dc), &tile, &area, mode, 0);
#else
            if (maskPixels != 0)
                CopyDeepMask((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*(PixMapHandle)maskPixels->dib,
                             (BitMap *)*port->dc->portPixMap, &tile, &tile, &area, 0, 0);
            else
                CopyBits((BitMap *)*(PixMapHandle)srcPixels->dib, (BitMap *)*port->dc->portPixMap, &tile, &area, mode, 0);
#endif
            area.left = area.right;
            area.right += tile.right;
        }
        area.top = area.bottom;
        area.bottom += tile.bottom;
    }
    RGBForeColor(&oldFore);
    RGBBackColor(&oldBack);
    SetGWorld(oldWorld, oldDevice);
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

long DT_SetPixel(ITImage *image, long x, long y, const ITColor *color)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    RGBColor rgb;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    SetCPixel(x, y, CopyDT_2SysColor(&rgb, color));
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_SetPenLocation(ITImage *image, long x, long y)
{
    ITPort *port;
#if TARGET_API_MAC_CARBON
    GrafPtr old;
#endif

    port = _DT_ContextGetContext(image);
#if TARGET_API_MAC_CARBON
    GetPort(&old);
    SetPort(port->dc);
    MoveTo(x, y);
    SetPort(old);
#else
    port->dc->pnLoc.h = x;
    port->dc->pnLoc.v = y;
#endif
    return 0;
}

long DT_DrawLineTo(ITImage *image, long x, long y)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    short oldMode;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    oldMode = PortPenMode(port->dc);
    PenMode(patCopy + 1);
    LineTo(x, y);
    PenMode(oldMode);
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_FrameRect(ITImage *image, const ITRect *r)
{
    int in;
    int out;
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    short oldMode;
    Point pen;
    unsigned short size;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    oldMode = PortPenMode(port->dc);
    PenMode(patCopy + 1);
#if TARGET_API_MAC_CARBON
    GetPortPenSize(port->dc, &pen);
#else
    pen = port->dc->pnSize;
#endif
    size = (pen.h + pen.v) / 2;
    in = size / 2;
    out = (size + 1) / 2;
    MoveTo(r->left + in, r->bottom - out);
    LineTo(r->left + in, r->top + in);
    LineTo(r->right - out, r->top + in);
    LineTo(r->right - out, r->bottom - out);
    LineTo(r->left + in, r->bottom - out);
    PenMode(oldMode);
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_PaintRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    Rect box;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    PaintRect(CopyDT_2SysRect(&box, r));
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_EraseRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    Rect box;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    EraseRect(CopyDT_2SysRect(&box, r));
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_InvertRect(ITImage *image, const ITRect *r)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    Rect box;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    InvertRect(CopyDT_2SysRect(&box, r));
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_GetTextWidth(ITImage *image, const char *text, long length)
{
    ITPort *port;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    long width;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    width = TextWidth(text, 0, length);
    SetGWorld(oldWorld, oldDevice);
    return width;
}

long DT_DrawText(ITImage *image, const char *text, long length)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    Point pen;

    port = _DT_ContextGetContext(image);
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
#if TARGET_API_MAC_CARBON
    GetPortPenLocation(port->dc, &pen);
#else
    pen = port->dc->pnLoc;
#endif
    e = DT_DrawTextAt(image, pen.h, pen.v, text, length);
    if (e != 0)
        err = e;
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

long DT_DrawTextAt(ITImage *image, long x, long y, const char *text, long length)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    switch (port->textAlign) {
    case kAlignRight:
        MoveTo(x - TextWidth(text, 0, length), y);
        DrawText(text, 0, length);
        MoveTo(x - TextWidth(text, 0, length), y);
        break;
    case kAlignCentre:
        MoveTo(x - TextWidth(text, 0, length) / 2, y);
        DrawText(text, 0, length);
        MoveTo(x, y);
        break;
    case -1:
    default:
        MoveTo(x, y);
        DrawText(text, 0, length);
        break;
    }
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

#if TARGET_API_MAC_CARBON
long DT_GetTextWidthUnicode(ITImage *image, const unsigned short *text, long length)
{
    ITPort *port;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    float width;
    long result;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    width = 0.0f;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    if (*text == kByteOrderMark) {
        text++;
        length--;
    }
    ATSU_Measure(text, 0, length, &width);
    result = width;
    SetGWorld(oldWorld, oldDevice);
    return result;
}

long DT_DrawTextAtUnicode(ITImage *image, long x, long y, const unsigned short *text, ULong length)
{
    ITPort *port;
    long err;
    long e;
    unsigned char stored;
    CGrafPtr oldWorld;
    GDHandle oldDevice;
    float width;

    port = _DT_ContextGetContext(image);
    oldWorld = 0;
    oldDevice = 0;
    err = 0;
    stored = 0;
    e = _DT_ContextLock(port);
    if (e != 0) {
        err = e;
        goto done;
    }
    stored = 1;
    GetGWorld(&oldWorld, &oldDevice);
    SetGWorld(port->dc, GetMainDevice());
    if (*text == kByteOrderMark) {
        text++;
        length--;
    }
    if (ATSU_Measure(text, 0, length, &width) != 0) {
        err = kTextFailed;
        goto done;
    }
    switch (port->textAlign) {
    case kAlignRight:
        MoveTo(width > 0.0f ? x - width : x, y);
        if (ATSU_DrawText(text, 0, length) != 0) {
            err = kTextFailed;
            goto done;
        }
        if (width > 0.0f)
            MoveTo(x - width, y);
        break;
    case kAlignCentre:
        MoveTo(x - width / 2.0f, y);
        if (ATSU_DrawText(text, 0, length) != 0) {
            err = kTextFailed;
            goto done;
        }
        MoveTo(x, y);
        break;
    case -1:
    default:
        MoveTo(x, y);
        if (ATSU_DrawText(text, 0, length) != 0) {
            err = kTextFailed;
            goto done;
        }
        break;
    }
    SetGWorld(oldWorld, oldDevice);
done:
    if (stored)
        _DT_ContextUnlock(port);
    return err;
}

void _DT_PrinterDispose(ITDevice *device)
{
    GrafPtr old;
    OSStatus e;

    old = 0;
    if (device->dc != 0) {
        GetPort(&old);
        SetPort(device->dc);
    }
    if (device->pageOpen) {
        e = PMSessionEndPage(device->record);
        if (device->status == 0 && e != 0)
            device->status = kPrintFailed;
        device->pageOpen = 0;
    }
    if (device->docOpen) {
        e = PMSessionEndDocument(device->record);
        if (device->status == 0 && e != 0)
            device->status = kPrintFailed;
        device->docOpen = 0;
    }
    if (PMSessionError(device->record) != 0 && device->status == 0)
        device->status = (short)(PMSessionError(device->record) == kPMCancel ? kPrintAborted : kPrintFailed);
    if (device->record != 0) {
        e = PMRelease(device->record);
        if (device->status == 0 && e != 0)
            device->status = kPrintFailed;
        device->record = 0;
    }
    if (old != 0)
        SetPort(old);
    gDT_Printers--;
}
#else
void _DT_PrinterDispose(ITDevice *device)
{
    GrafPtr old;
    TPrStatus status;

    old = 0;
    if (device->dc != 0) {
        GetPort(&old);
        SetPort((GrafPtr)device->dc);
    }
    if (device->pageOpen) {
        PrClosePage(device->printPort);
        if (device->status == 0 && PrError() != 0) {
            PrError();
            device->status = kPrintFailed;
        }
        device->pageOpen = 0;
    }
    if (device->docOpen) {
        PrCloseDoc(device->printPort);
        if (device->status == 0 && PrError() != 0) {
            PrError();
            device->status = kPrintFailed;
        }
        device->docOpen = 0;
        device->printPort = 0;
    }
    if (device->status == 0 && (*(THPrint)device->record)->prJob.bJDocLoop == bSpoolLoop) {
        PrPicFile(device->record, 0, 0, 0, &status);
        if (device->status == 0 && PrError() != 0) {
            PrError();
            device->status = kPrintFailed;
        }
    }
    if (PrError() != noErr && device->status == 0)
        device->status = (short)(PrError() == userCanceledErr ? kPrintAborted : kPrintFailed);
    if (device->record != 0) {
        IndirectDeallocate(device->record);
        device->record = 0;
    }
    if (old != 0)
        SetPort(old);
    if (gDT_Printers == 1)
        PrClose();
    gDT_Printers--;
}
#endif
