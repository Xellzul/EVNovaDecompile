// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335 + MSL 2001
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itrect.c -- the interface tool's rectangle helpers (left, top, right, bottom as longs). Notes: Research/notes/itrect.md */

#include "burgertypes.h"
#include "burgerplat.h"

/* DT_GetColorARGB (an ITColor from a 0xAARRGGBB value). STATE: MATCH 100%, exe 22 / obj 22, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519cf0
ITColor DT_GetColorARGB(ULong argb)
{
    ITColor color;
    int32_t err;

    err = 0;
    color.alpha = (argb >> 24) & 0xff;
    color.red = (argb >> 16) & 0xff;
    color.green = (argb >> 8) & 0xff;
    color.blue = argb & 0xff;
    return color;
}

/* DT_SetRect (SetRect). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519d30
ITRect *DT_SetRect(ITRect *r, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    int32_t err;

    err = 0;
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
    return r;
}

/* DT_IsPointInRect (PtInRect). STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519d58
unsigned char DT_IsPointInRect(const ITPoint *pt, const ITRect *r)
{
    unsigned char inside;
    int32_t err;

    err = 0;
    inside = 0;
    if (pt->h >= r->left && pt->v >= r->top && pt->h < r->right && pt->v < r->bottom)
        inside = 1;
    return inside;
}

/* DT_IsRectReallyEmpty (nothing to draw: turned inside out, or a single point). STATE:
   MATCH 100%, exe 22 / obj 22, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519d83
unsigned char DT_IsRectReallyEmpty(const ITRect *r)
{
    unsigned char none;
    int32_t err;

    err = 0;
    none = 0;
    if (r->right < r->left || r->bottom < r->top ||
        (r->left == r->right && r->top == r->bottom))
        none = 1;
    return none;
}

/* DT_IsRectEmpty (EmptyRect). STATE: MATCH 100%, exe 14 / obj 14, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519daf
unsigned char DT_IsRectEmpty(const ITRect *r)
{
    unsigned char empty;
    int32_t err;

    err = 0;
    empty = 0;
    if (r->left >= r->right || r->top >= r->bottom)
        empty = 1;
    return empty;
}

/* DT_IsRectEqual (EqualRect). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519dcd
unsigned char DT_IsRectEqual(const ITRect *a, const ITRect *b)
{
    unsigned char equal;
    int32_t err;

    err = 0;
    equal = 0;
    if (a->left == b->left && a->top == b->top && a->right == b->right &&
        a->bottom == b->bottom)
        equal = 1;
    return equal;
}

/* DT_RectOffset (OffsetRect). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519e00
ITRect *DT_RectOffset(ITRect *r, int32_t dh, int32_t dv)
{
    int32_t err;

    err = 0;
    r->left += dh;
    r->top += dv;
    r->right += dh;
    r->bottom += dv;
    return r;
}

/* DT_RectInset (InsetRect). STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519e22
ITRect *DT_RectInset(ITRect *r, int32_t dh, int32_t dv)
{
    int32_t err;

    err = 0;
    r->left += dh;
    r->top += dv;
    r->right -= dh;
    r->bottom -= dv;
    return r;
}

/* DT_RectCenter (`r' sized like `size', centred in `bounds' with bounds' origin at 0). STATE:
   MATCH 100%, exe 43 / obj 43, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519e44
ITRect *DT_RectCenter(ITRect *r, const ITRect *size, const ITRect *bounds)
{
    int32_t width;
    int32_t height;
    int32_t err;

    err = 0;
    width = size->right - size->left;
    height = size->bottom - size->top;
    r->right = width + (r->left = (bounds->right - bounds->left) / 2 - width / 2);
    r->bottom = height + (r->top = (bounds->bottom - bounds->top) / 2 - height / 2);
    return r;
}

/* DT_RectIntersect (SectRect, without the empty test). STATE: MATCH 100%, exe 35 / obj 35, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00519eb6
ITRect *DT_RectIntersect(ITRect *r, const ITRect *a, const ITRect *b)
{
    int32_t err;

    err = 0;
    r->left = a->left > b->left ? a->left : b->left;
    r->top = a->top > b->top ? a->top : b->top;
    r->right = a->right < b->right ? a->right : b->right;
    r->bottom = a->bottom < b->bottom ? a->bottom : b->bottom;
    return r;
}

/* a bevel's light and dark shades of one component; the middle arm of LIGHTER never runs */
#define LIGHTER(c)  ((c) <= 0xa0 ? (c) + 0x40 : (c) <= 0xa0 ? (c) + (0xff - (c)) / 2 : 0xff)
#define DARKER(c)   ((c) >= 0x60 ? (c) - 0x30 : (c) >= 0x30 ? (c) / 2 : 0)

/* DT_ColorLighten (a colour lightened for a bevel's lit edge; alpha kept). STATE: MATCH 100%, exe 58
   / obj 58, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519f06
void DT_ColorLighten(ITColor *color)
{
    int32_t err, err2;

    err2 = err = 0;
    color->red = LIGHTER(color->red);
    color->green = LIGHTER(color->green);
    color->blue = LIGHTER(color->blue);
}

/* DT_ColorDarken (a colour darkened for a bevel's shaded edge; alpha kept). STATE: MATCH 100%, exe
   49 / obj 49, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00519fb1
void DT_ColorDarken(ITColor *color)
{
    int32_t err, err2;

    err2 = err = 0;
    color->red = DARKER(color->red);
    color->green = DARKER(color->green);
    color->blue = DARKER(color->blue);
}

#ifdef MAC1010
#include "burger.h"

#define kRegionLimit 0x7d00

RgnHandle DT_RegionCreate(void)
{
    return NewRgn();
}

void DT_RegionDispose(void *region)
{
    DisposeRgn(region);
}

ITRect *DT_GetRegionRect(ITRect *box, void *region)
{
#if TARGET_API_MAC_CARBON
    Rect bounds;

    CopySys2DT_Rect(box, GetRegionBounds(region, &bounds));
#else
    CopySys2DT_Rect(box, &(*(RgnHandle)region)->rgnBBox);
#endif
    return box;
}

int32_t DT_SetRegionRect(void *region, const ITRect *r)
{
    int32_t bottom;
    int32_t right;
    int32_t err;
    int32_t left;
    int32_t top;

    err = 0;
    bottom = r->bottom;
    if (bottom < -kRegionLimit)
        bottom = -kRegionLimit;
    else if (bottom > kRegionLimit)
        bottom = kRegionLimit;
    right = r->right;
    if (right < -kRegionLimit)
        right = -kRegionLimit;
    else if (right > kRegionLimit)
        right = kRegionLimit;
    top = r->top;
    if (top < -kRegionLimit)
        top = -kRegionLimit;
    else if (top > kRegionLimit)
        top = kRegionLimit;
    left = r->left;
    if (left < -kRegionLimit)
        left = -kRegionLimit;
    else if (left > kRegionLimit)
        left = kRegionLimit;
    SetRectRgn(region, left, top, right, bottom);
    if (QDError() != 0)
        err = kOutOfMemory;
    return err;
}

ITPoint Sys2DT_Point(const Point *pt)
{
    ITPoint copy;

    copy.h = pt->h;
    copy.v = pt->v;
    return copy;
}

ITRect Sys2DT_Rect(const Rect *r)
{
    ITRect copy;

    copy.left = r->left;
    copy.top = r->top;
    copy.right = r->right;
    copy.bottom = r->bottom;
    return copy;
}

#if TARGET_API_MAC_CARBON
Rect DT_2SysRect(const ITRect *r)
{
    Rect copy;
    int32_t n;

    n = r->left;
    copy.left = n > -32768 ? n : -32768;
    n = r->top;
    copy.top = n > -32768 ? n : -32768;
    n = r->right;
    copy.right = n < 32767 ? n : 32767;
    n = r->bottom;
    copy.bottom = n < 32767 ? n : 32767;
    return copy;
}
#endif

RGBColor DT_2SysColor(const ITColor *color)
{
    RGBColor c;

    c.red = (color->red << 8) | color->red;
    c.green = (color->green << 8) | color->green;
    c.blue = (color->blue << 8) | color->blue;
    return c;
}

ITPoint *CopySys2DT_Point(ITPoint *dst, const Point *src)
{
    dst->h = src->h;
    dst->v = src->v;
    return dst;
}

Point *CopyDT_2SysPoint(Point *dst, const ITPoint *src)
{
    int32_t n;

    n = src->h;
    dst->h = n < -32768 ? -32768 : n > 32767 ? 32767 : n;
    n = src->v;
    dst->v = n < -32768 ? -32768 : n > 32767 ? 32767 : n;
    return dst;
}

ITRect *CopySys2DT_Rect(ITRect *dst, const Rect *src)
{
    dst->left = src->left;
    dst->top = src->top;
    dst->right = src->right;
    dst->bottom = src->bottom;
    return dst;
}

Rect *CopyDT_2SysRect(Rect *dst, const ITRect *src)
{
    int32_t n;

    n = src->left;
    dst->left = n > -32768 ? n : -32768;
    n = src->top;
    dst->top = n > -32768 ? n : -32768;
    n = src->right;
    dst->right = n < 32767 ? n : 32767;
    n = src->bottom;
    dst->bottom = n < 32767 ? n : 32767;
    return dst;
}

ITColor *CopySys2DT_Color(ITColor *color, const RGBColor *ref)
{
    color->alpha = 0xff;
    color->red = (ref->red >> 8) & 0xff;
    color->green = (ref->green >> 8) & 0xff;
    color->blue = (ref->blue >> 8) & 0xff;
    return color;
}

RGBColor *CopyDT_2SysColor(RGBColor *ref, const ITColor *color)
{
    ref->red = (color->red << 8) | color->red;
    ref->green = (color->green << 8) | color->green;
    ref->blue = (color->blue << 8) | color->blue;
    return ref;
}
#endif
