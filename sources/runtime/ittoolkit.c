// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* ittoolkit.c -- the interface tool proper, from 0x0051a038: regions, colours and rectangles, the window records, page loading, focus, painting and the window and dialog procedures (to 0x0051c936). Notes: Research/notes/ittoolkit.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"
#include "ittoolkit.h"

static char kNoText[] = "";
static char kCloseBoxClass[] = "interface_tool_closebox";
static char kToolClass[] = "interface_tool";

static ITRect gIT_ShowPageZeroRect;                      /* 0x0086bb00 _IT_ViewGoToPage's empty clip */

typedef struct RegionData {
    ULong headerSize;                            /* +0x00 RGNDATAHEADER */
    ULong type;                                  /* +0x04 */
    ULong count;                                 /* +0x08 */
    ULong bufferSize;                            /* +0x0c */
    ITRect bound;                                /* +0x10, the rectangles follow */
} RegionData;

extern long IT_ViewGetItemRect(void *windowRef, long index, ITRect *r);     /* interfacetool.c */
extern long IT_ViewGetItemEnabled(void *windowRef, long index,
                         unsigned char *enabled);                        /* interfacetool.c */
extern long IT_ViewSetItemRect(WindowNode *window, long index, const ITRect *r);  /* interfacetool.c */
long _IT_ViewChange(WindowNode *window, struct FolderSpec **folder, const char *file,
                  const char *path, const char *anchor, void *unused);
long _IT_ParseTupleCreate(WindowNode *window, ITSource *source, long where, const char *file,
                  void *unused);
extern long FT_PathCurrent(const char *first, const char *second);        /* atfilespec.c */
extern unsigned char IndirectIsNormalizedEncoding(void **handle);                        /* atcharset.c */
extern long IndirectNormalizeEncoding(void **handle, const char *charset);            /* atcharset.c */
extern ITPageTable *_IT_PageListCreate(ITElement *root);                       /* itattr.c */
extern ITPage *_IT_GetPageByIndex(ITPageTable *table, ULong index);            /* itattr.c */
void _IT_ParseTupleDispose(ITSource *source);
void _IT_ViewUnload(WindowNode *window);
long _IT_ViewPreloadIdle(WindowNode *window);
long _IT_ViewPreload(WindowNode *window, ITElement *root);
long _IT_ViewCheckPage(WindowNode *window, unsigned char *may);
long _IT_ViewGoToPage(WindowNode *window, ITPage *page);
extern ITPage *_IT_GetPageByName(ITPageTable *table, const char *name);      /* itattr.c */

extern ITImage *DT_ContextCreate(void *window, unsigned char save);          /* itgfx.c */
extern ITImage *DT_SurfaceCreateFromWindow(void *window);                              /* itgfx.c */

extern long DT_AddValidRect(ITImage *image, const ITRect *r);               /* itgfx.c */
extern long DT_SurfaceBlitRects(ITImage *image, ULong count, const ITRect *rects); /* itgfx.c */
extern long DT_SurfaceDispose(ITImage *image);                                /* itgfx.c */

long __stdcall IT_ViewCreateParam(void *hwnd, unsigned int msg, unsigned int wParam, long lParam);
extern unsigned char IT_HandleWinEvent(void *hwnd, void *dc, unsigned int msg, unsigned int wParam,
                                  long lParam, long *result);            /* atcursor.c */
extern ITRect *DT_RectCenter(ITRect *r, const ITRect *size, const ITRect *bounds);  /* itrect.c */

/* Win32's DLGTEMPLATE, which Windows reads packed: 18 bytes */
typedef struct DialogTemplate {
    ULong style;
    ULong exStyle;
    unsigned short items;
    short x;
    short y;
    short cx;
    short cy;
} DialogTemplate;

/* what the dialog procedure receives as its creation parameter */
typedef struct DialogParams {
    void *folder;
    const char *file;
    const char *path;
    ITWindowProc proc;
    long refcon;
    char *result;
} DialogParams;

#define DS_CENTER          0x0800
#define DS_MODALFRAME       0x0080
#define kDialogStyle        (WS_POPUP | WS_VISIBLE | WS_CAPTION | DS_CENTER | DS_MODALFRAME)
#define kDialogTemplateSize 18
#define kOffScreen          32000
#define kDialogWidth        400
#define kDialogHeight       300
#define kDialogFailed       -8400

WindowNode *gIT_CurrentView;                        /* 0x00870084 the window a control proc serves */
ITControl *gIT_CurrentItem;                         /* 0x00870028 the control it serves */

#define kExternalLink       -8400
#define kEventCreate        0
#define kEventPageOpen      2
#define kEventPageShown     3
#define kEventClose         1
#define kNoHwnd             ((void *)-1)
#define kEventLeave         6
#define kEventFocus        7
#define kEventBlur          11
#define kFocusRefused       -8404
#define RGN_COPY            5
#define NULLREGION          1
#define kGdiMaxExtent       32000
#define kGdiNearOrigin      8000
#define kPreloadGap         500     /* ms between two preloads */
#define kIndexFile          "index.iml"
#define kInfoFolder         2       /* FTFileInfo.flags: a folder */
#define kNoPage             -8401
#define kNoIndex            -8405
#define kNoConverter        -8406
#define DM_REPOSITION       0x0402
#define kIdleTimer          0x4d4a5321  /* 'MJS!' */
#define kIdleTimerMs        100

/* DT_RegionCreate (a new empty region). STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051a038
void *DT_RegionCreate(void)
{
    void *region;
    long err;

    err = 0;
    region = 0;
    region = CreateRectRgn(0, 0, 0, 0);
    if (region == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    }
    return region;
}

/* DT_RegionClone (a copy of a region). STATE: MATCH 100%, exe 24 / obj 24, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051a064
void *DT_RegionClone(void *region)
{
    void *copy;
    long err;

    err = 0;
    copy = 0;
    copy = CreateRectRgn(0, 0, 0, 0);
    if (copy == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    } else {
        CombineRgn(copy, region, 0, RGN_COPY);
    }
    return copy;
}

/* DT_RegionDispose (dispose a region). STATE: MATCH 100%, exe 5 / obj 5, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x0051a0a1
void DT_RegionDispose(void *region)
{
    long err;

    err = 0;
    DeleteObject(region);
}

/* DT_GetRegionRect (a region's bounding box). STATE: MATCH 100%, exe 10 / obj 10, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x0051a0b0
ITRect *DT_GetRegionRect(ITRect *box, void *region)
{
    long err;

    err = 0;
    GetRgnBox(region, (WinRect *)box);
    return box;
}

/* DT_SetRegionRect (set a region to a rectangle, clamped to GDI's reach). STATE: MATCH 100%, exe 24 /
   obj 24, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a0c8
long DT_SetRegionRect(void *region, const ITRect *r)
{
    long err;
    ITRect clamped;

    err = 0;
    CopyDT_2SysRect(&clamped, r);
    if (SetRectRgn(region, clamped.left, clamped.top, clamped.right, clamped.bottom) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    }
    return err;
}

/* DT_RegionScale (a region carried from rectangle `from' to rectangle `to': an offset when the
   sizes agree, else every rectangle of its data scaled, unsigned, by from's extent over to's, and
   rebuilt; -7004 on failure). The rebuild passes the rectangle count where ExtCreateRegion wants
   the data's byte size, as the exe does. STATE: MATCH 100%, exe 136 / obj 136, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a110
long DT_RegionScale(void *region, const ITRect *from, const ITRect *to)
{
    void *rebuilt;
    long err;
    RegionData *data;
    ULong size;
    ULong i;
    ULong fromW;
    ULong fromH;
    ULong toH;
    ULong toW;
    ULong fromLeft;
    ULong fromTop;
    ULong toLeft;
    ULong toTop;
    ULong dx;
    ULong dy;
    ITRect *r;

    err = 0;
    rebuilt = 0;
    data = 0;
    fromLeft = from->left;
    fromW = from->right - fromLeft;
    fromTop = from->top;
    fromH = from->bottom - fromTop;
    toLeft = to->left;
    toW = to->right - toLeft;
    toTop = to->top;
    toH = to->bottom - toTop;
    dx = toLeft - fromLeft;
    dy = toTop - fromTop;
    if (fromW == toW && fromH == toH) {
        OffsetRgn(region, dx, dy);
        goto done;
    }
    size = GetRegionData(region, 0, 0);
    if (size == 0) {
        err = GetLastError();
        goto nomem;
    }
    data = MemoryAllocate(size);
    if (data == 0)
        goto nomem;
    if (GetRegionData(region, size, data) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    for (i = 0, r = &data->bound; i < data->count + 1; i++, r++) {
        r->left = r->left * fromW / toW + dx;
        r->top = r->top * fromH / toH + dy;
        r->right = r->right * fromW / toW + dx;
        r->bottom = r->bottom * fromH / toH + dy;
    }
    rebuilt = ExtCreateRegion(0, data->count, data);
    if (rebuilt == 0) {
        err = GetLastError();
nomem:
        err = kOutOfMemory;
        goto done;
    }
    if (CombineRgn(region, rebuilt, 0, RGN_COPY) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
    }
done:
    if (rebuilt != 0)
        DeleteObject(rebuilt);
    if (data != 0)
        MemoryDeallocate(data);
    return err;
}

/* DT_IsRegionEmpty (whether a region is empty). STATE: MATCH 100%, exe 17 / obj 17, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a2c0
unsigned char DT_IsRegionEmpty(void *region)
{
    long err;
    unsigned char result;

    err = 0;
    result = 0;
    result = CombineRgn(region, region, 0, RGN_COPY) == NULLREGION ? true : false;
    return result;
}

/* Sys2DT_Point (a point by value). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x0051a2e6
ITPoint Sys2DT_Point(const ITPoint *pt)
{
    ITPoint copy;
    long err;

    err = 0;
    copy.h = pt->h;
    copy.v = pt->v;
    return copy;
}

/* DT_2SysColor (a colour as a Windows COLORREF). STATE: MATCH 100%, exe 14 / obj 14, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a307
ULong DT_2SysColor(const ITColor *color)
{
    long err;

    err = 0;
    return RGB(color->red, color->green, color->blue);
}

/* CopySys2DT_Point (copy a point). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a32b
ITPoint *CopySys2DT_Point(ITPoint *dst, const ITPoint *src)
{
    long err;

    err = 0;
    dst->h = src->h;
    dst->v = src->v;
    return dst;
}

/* CopyDT_2SysPoint (copy a point). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a346
ITPoint *CopyDT_2SysPoint(ITPoint *dst, const ITPoint *src)
{
    long err;

    err = 0;
    dst->h = src->h;
    dst->v = src->v;
    return dst;
}

/* CopySys2DT_Rect (copy a rectangle). STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN:
   none */
// FUNCTION: EVNOVA 0x0051a361
ITRect *CopySys2DT_Rect(ITRect *dst, const ITRect *src)
{
    long err;

    err = 0;
    dst->left = src->left;
    dst->top = src->top;
    dst->right = src->right;
    dst->bottom = src->bottom;
    return dst;
}

/* CopyDT_2SysRect (copy a rectangle, clamped to GDI's 16-bit reach: an extent over 32000 is cut to
   32000 from whichever edge lies within 8000 of the origin, else centred; each axis has its own
   lo / hi). STATE: MATCH 100%, exe 79 / obj 79, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a38a
ITRect *CopyDT_2SysRect(ITRect *dst, const ITRect *src)
{
    long err;
    long lo;
    long hi;
    long lo2;
    long hi2;

    err = 0;
    dst->left = src->left;
    dst->top = src->top;
    dst->right = src->right;
    dst->bottom = src->bottom;
    lo = dst->left;
    hi = dst->right;
    if (hi - lo > kGdiMaxExtent) {
        if (dst->left > -kGdiNearOrigin && dst->left < kGdiNearOrigin) {
            dst->right = lo + kGdiMaxExtent;
        } else if (dst->right > -kGdiNearOrigin && dst->right < kGdiNearOrigin) {
            dst->left = hi - kGdiMaxExtent;
        } else {
            dst->left = (lo + hi) / 2 - kGdiMaxExtent / 2;
            dst->right = dst->left + kGdiMaxExtent;
        }
    }
    lo2 = dst->top;
    hi2 = dst->bottom;
    if (hi2 - lo2 > kGdiMaxExtent) {
        if (dst->top > -kGdiNearOrigin && dst->top < kGdiNearOrigin) {
            dst->bottom = lo2 + kGdiMaxExtent;
        } else if (dst->bottom > -kGdiNearOrigin && dst->bottom < kGdiNearOrigin) {
            dst->top = hi2 - kGdiMaxExtent;
        } else {
            dst->top = (lo2 + hi2) / 2 - kGdiMaxExtent / 2;
            dst->bottom = dst->top + kGdiMaxExtent;
        }
    }
    return dst;
}

/* CopySys2DT_Color (a Windows COLORREF as an opaque colour). STATE: MATCH 100%, exe 22 / obj 22,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a48e
ITColor *CopySys2DT_Color(ITColor *color, const ULong *ref)
{
    long err;

    err = 0;
    color->alpha = 0xff;
    color->red = *ref & 0xff;
    color->green = (*ref >> 8) & 0xff;
    color->blue = (*ref >> 16) & 0xff;
    return color;
}

/* CopyDT_2SysColor (a colour stored as a COLORREF). STATE: MATCH 100%, exe 19 / obj 19, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a4c0
ULong *CopyDT_2SysColor(ULong *ref, const ITColor *color)
{
    long err;

    err = 0;
    *ref = RGB(color->red, color->green, color->blue);
    return ref;
}

/* IT_ViewCreate (a window record for a Windows window, or with hwnd -1 the record in `*out' set up
   again: a child of the window a control proc is serving, its DC, its bounds (the client area
   unless given), black on white, dirty, the page callback, the focusable list, the focus record
   kept by the root, the port image (a child shares the root's), an offscreen buffer for a
   buffered root; the callback told (event 0); on failure an untitled record is destroyed). STATE:
   MATCH 100%, exe 145 / obj 145, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a4ee
long IT_ViewCreate(void *hwnd, const ITRect *bounds, ITWindowProc proc, long refcon,
                  long unused, WindowNode **out)
{
    long err;
    long e;
    WindowNode *window;
    WindowNode *root;
    WinRect client;

    err = 0;
    window = 0;
    root = 0;
    if (hwnd == kNoHwnd) {
        window = *out;
        hwnd = window->hwnd;
    } else {
        *out = window = 0;
        window = MemoryAllocateClear(sizeof(WindowNode));
        if (window == 0) {
            err = kOutOfMemory;
            goto done;
        }
        window->next = gIT_ViewList;
        gIT_ViewList = window;
        window->parent = gIT_CurrentView;
        window->hwnd = hwnd;
        window->dc = GetDC(window->hwnd);
        if (window->dc == 0 || GetClientRect(window->hwnd, &client) == 0) {
            err = GetLastError();
            goto nomem;
        }
        if (bounds != 0)
            window->bounds = *bounds;
        else
            CopySys2DT_Rect(&window->bounds, (ITRect *)&client);
    }
    window->fore = gDT_ColorBlack;
    window->back = gDT_ColorWhite;
    window->visible = 1;
    window->enabled = 1;
    window->dirty = 1;
    window->proc = proc;
    window->refcon = refcon;
    window->focusables = IndirectAllocate(0);
    if (window->focusables == 0)
        goto nomem;
    for (root = window; root->parent != 0; root = root->parent)
        ;
    if (window == root) {
        window->focusRecord.root = window;
        window->focusRecord.index = -1;
    }
    window->focus = &root->focusRecord;
    if (window != root) {
        window->image = root->image;
    } else {
        window->image = DT_ContextCreate(window->hwnd, 0);
        if (window->image == 0)
            goto nomem;
    }
    if (window->parent == 0 && !window->unbuffered) {
        window->buffer = DT_SurfaceCreateFromWindow(window->hwnd);
        if (window->buffer == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
    }
    e = _IT_ViewCallback(window, 0, 0, kEventCreate);
    if (e != 0)
        err = e;
done:
    if (err != 0 && window != 0 && !window->titled) {
        IT_ViewDispose(window);
        window = 0;
    }
    *out = window;
    return err;
}

/* IT_ViewCreateModal (run a modal interface-tool dialog on a page file: an empty dialog template placed
   off screen, 400 x 300 dialog units, whose procedure 0x0051c664 builds the window; the button
   that closed it is copied to `result'; -8400 when Windows refuses the dialog). STATE: MATCH
   100%, exe 147 / obj 147, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051a695
long IT_ViewCreateModal(void *folder, const char *file, const char *path, ITWindowProc proc,
                  long refcon, char *result)
{
    long err;
    long e;
    long spare;
    unsigned short none;
    long rc;
    void **handle;
    void **spare2;
    DialogTemplate dlg;
    DialogParams params;

    err = 0;
    spare = 0;
    none = 0;
    rc = -1;
    handle = 0;
    spare2 = handle = 0;
    if (result != 0)
        *result = 0;
    handle = IndirectAllocate(0);
    if (handle == 0) {
        err = kOutOfMemory;
        goto done;
    }
    dlg.style = kDialogStyle;
    dlg.exStyle = 0;
    dlg.items = 0;
    dlg.x = MulDiv(kOffScreen, 4, GetDialogBaseUnits());
    dlg.y = MulDiv(kOffScreen, 8, GetDialogBaseUnits());
    dlg.cx = MulDiv(kDialogWidth, 4, GetDialogBaseUnits());
    dlg.cy = MulDiv(kDialogHeight, 8, GetDialogBaseUnits());
    if ((e = IndirectAppendData(handle, &dlg, kDialogTemplateSize)) != 0
        || (e = IndirectAppendData(handle, &none, sizeof(none))) != 0
        || (e = IndirectAppendData(handle, &none, sizeof(none))) != 0
        || (e = IndirectAppendData(handle, &none, sizeof(none))) != 0
        || (e = IndirectAppendData(handle, &none, sizeof(none))) != 0
        || (e = IndirectAppendData(handle, &none, sizeof(none))) != 0) {
        err = e;
        goto done;
    }
    params.folder = folder;
    params.file = file;
    params.path = path;
    params.proc = proc;
    params.refcon = refcon;
    params.result = result;
    IndirectSetLock(handle, 1);
    if (DialogBoxIndirectParamA(GetModuleHandleA(0), *handle, _PlatformGetWindow(),
                                (void *)IT_ViewCreateParam, (long)&params) == -1) {
        err = GetLastError();
        err = kDialogFailed;
    }
done:
    if (handle != 0)
        IndirectDeallocate((void **)handle);
    return err;
}

/* _IT_ViewChange (load a page file into a window when the current page may be left: drawing clipped
   to nothing meanwhile, the old page dropped, colours and title reset; the file read and parsed
   (0x0051c3c1), its first INTERFACE element taken, its frame, title, colours and BACKGROUND image
   read, its pages tabled and `anchor' (else the first) shown; with no folder and no file only the
   reset and the refresh). STATE: MATCH 100%, exe 396 / obj 396, byte-identical yes. OPEN: none;
   the sixth argument is only passed on to 0x0051c3c1 */
// FUNCTION: EVNOVA 0x0051a847
long _IT_ViewChange(WindowNode *window, struct FolderSpec **folder, const char *file,
                  const char *path, const char *anchor, void *unused)
{
    static ITRect nowhere;
    long err;
    long e;
    long spare;
    void *saved;
    void **data;
    unsigned char may;
    unsigned char found;
    ITElement *element;
    ITPage *page;
    char *value;
    ITRect clip;
    char upper[256];
    char name[256];
    char message[256];
    ITRect frame;

    err = 0;
    element = 0;
    value = 0;
    spare = 0;
    data = 0;
    may = 1;
    found = 0;
    e = 0;
    saved = 0;
    clip = nowhere;
    e = _IT_ViewCheckPage(window, &may);
    if (e != 0)
        goto fail;
    if (!may)
        goto done;
    saved = DT_GetClipRegion(window->image);
    if (saved == 0)
        goto nomem;
    e = DT_SetClipRect(window->image, &clip);
    if (e != 0)
        goto fail;
    _IT_ViewUnload(window);
    window->fore = gDT_ColorBlack;
    window->back = gDT_ColorWhite;
    window->title[0] = 0;
    if (folder != 0 || file != 0) {
        if (path != 0 && path != window->path)
            String2UpperCopySafe(window->path, path, sizeof(window->path));
        else if (path == 0)
            window->path[0] = 0;
        e = _IT_ParseTupleCreate(window, &window->source, (long)folder, file, unused);
        if (e != 0)
            goto fail;
        element = window->source.interfaceElement;
        while ((element = _IT_ParseTreeFindNextTag(window->source.root, element, "INTERFACE",
                                       AsByte(element != 0))) != 0) {
            found = 1;
            if (window->path[0] == 0)
                break;
            value = HashLookup(element->attributes, "NAME", 0);
            if (value == 0 || *value != 0)
                break;
            /* BUG: upper is never filled, so the NAME test upper-cases an uninitialised buffer */
            if (StringCompare(String2Upper(upper), window->path) != 0)
                break;
        }
        if (!found) {
            err = kNoPage;
            goto done;
        }
        window->source.interfaceElement = element;
        if (window->source.interfaceElement == 0) {
            err = kNotFound;
            goto done;
        }
        if (_IT_ParseTreeFindNextTag(window->source.root, element, "PAGE", 0) == 0) {
            err = kNoPage;
            goto done;
        }
        value = _IT_SimpleFindAttribute(window->source.interfaceElement, "RECT");
        if (value != 0) {
            _IT_ParseAttributeGetRect(&frame, window->source.interfaceElement->wide, value);
            window->frame = frame;
        } else {
            DT_SetRect(&window->frame, 0, 0, kDialogWidth, kDialogHeight);
            value = _IT_SimpleFindAttribute(window->source.interfaceElement, "WIDTH");
            if (value != 0)
                window->frame.right = _IT_ParseAttributeGetNumber(window->source.interfaceElement->wide,
                                                   value);
            value = _IT_SimpleFindAttribute(window->source.interfaceElement, "HEIGHT");
            if (value != 0)
                window->frame.bottom = _IT_ParseAttributeGetNumber(window->source.interfaceElement->wide,
                                                    value);
        }
        value = _IT_SimpleFindAttribute(window->source.interfaceElement, "TITLE");
        if (value != 0)
            StringCopySafe(window->title, value, sizeof(window->title));
        value = _IT_SimpleFindAttribute(window->source.interfaceElement, "FGCOLOR");
        if (value != 0)
            window->fore = _IT_ParseAttributeGetColor(window->source.interfaceElement->wide, value);
        value = _IT_SimpleFindAttribute(window->source.interfaceElement, "BGCOLOR");
        if (value != 0)
            window->back = _IT_ParseAttributeGetColor(window->source.interfaceElement->wide, value);
        value = _IT_SimpleFindAttribute(window->source.interfaceElement, "BACKGROUND");
        if (value != 0) {
            StringCopySafe(name, value, sizeof(name));
            err = FT_FileGetFlags((long)window->source.folder, name, 0, 0);
            if (err == 0) {
                e = FT_FileLoad((long)window->source.folder, name, kChunkMax, 1, &data);
                if (e != 0)
                    goto fail;
                e = DT_ImageFileLoad(data, &window->backdrop, 0, 0, 0);
                if (e != 0)
                    goto fail;
                IndirectDeallocate(data);
                data = 0;
                e = DT_ImagePreflightTile(window->backdrop, 0);
                if (e == 0) {
                } else
                    goto fail;
            } else if (err == kFTErrFileNotFound) {
                window->backdrop = 0;
                StringFormatSafe(message, sizeof(message), "Unable to load background '%s'", name);
                if (err == kFTErrFileNotFound || err == kFTErrBadName
                    || err == kFTErrDirNotFound)
                    err = 0;
                else
                    goto done;
            }
        }
        if ((window->pages = _IT_PageListCreate(window->source.interfaceElement->child)) == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
        if (anchor != 0 && *anchor != 0)
            page = _IT_GetPageByName(window->pages, anchor);
        else
            page = _IT_GetPageByIndex(window->pages, 0);
        e = _IT_ViewGoToPage(window, page);
        if (e != 0)
            goto fail;
    }
    e = DT_SetClipRegion(window->image, saved);
    if (e != 0)
        goto fail;
    DT_RegionDispose(saved);
    saved = 0;
    e = _IT_ViewValidate(window);
    if (e != 0) {
fail:
        err = e;
    }
done:
    if (err != 0)
        _IT_ViewUnload(window);
    if (saved != 0) {
        DT_SetClipRegion(window->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* IT_ViewChange (the same, the entry the dialog and link code call). STATE: MATCH 100%, exe 9 /
   obj 9, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ad55
long IT_ViewChange(WindowNode *window, struct FolderSpec **folder, const char *file,
                  const char *path, const char *anchor)
{
    return _IT_ViewChange(window, folder, file, path, anchor, 0);
}

/* IT_ViewGoToPage (go to the page named `name' when the current one may be left; the window
   refreshed either way). STATE: MATCH 100%, exe 44 / obj 44, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ad74
long IT_ViewGoToPage(WindowNode *window, const char *name)
{
    long err;
    long e;
    unsigned char may;

    err = 0;
    may = 0;
    e = 0;
    e = _IT_ViewCheckPage(window, &may);
    if (e != 0)
        goto fail;
    if (may) {
        e = _IT_ViewGoToPage(window, _IT_GetPageByName(window->pages, name));
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
    }
    e = _IT_ViewValidate(window);
    if (e != 0)
        err = e;
done:
    return err;
}

/* IT_ViewDispose (destroy a window: hidden, its callback told the page is left (6) and the window
   closes (1), its content dropped, the Windows window destroyed (EndDialog for a modal one), its
   variables, buffer, port image (a root's own), focusables, control types and preloaded images
   freed, and the record unlinked and freed). STATE: MATCH 100%, exe 148 / obj 148, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051add8
long IT_ViewDispose(WindowNode *window)
{
    ControlType *type;
    ITPreload *preload;
    WindowNode **link;
    long err, err2;
    long e;

    err = err2 = 0;
    if (window->hwnd != 0 && window->titled)
        ShowWindow(window->hwnd, SW_HIDE);
    e = _IT_ViewCallback(window, window->page, 0, kEventLeave);
    if (e != 0)
        err = e;
    e = _IT_ViewCallback(window, 0, 0, kEventClose);
    if (e != 0)
        err = e;
    _IT_ViewUnload(window);
    if (window->hwnd != 0 && window->titled) {
        window->closing = 1;
        if (window->modal) {
            if (EndDialog(window->hwnd, 0) == 0)
                err = GetLastError();
        } else {
            if (DestroyWindow(window->hwnd) == 0)
                err = GetLastError();
        }
        window->closing = 0;
    }
    window->hwnd = 0;
    window->dc = 0;
    window->titled = 0;
    if (window->variables != 0) {
        HashDispose(window->variables);
        window->variables = 0;
    }
    if (window->buffer != 0) {
        DT_SurfaceDispose(window->buffer);
        window->buffer = 0;
    }
    if (window->image != 0) {
        if (window->parent == 0)
            DT_ContextDispose(window->image);
        window->image = 0;
    }
    if (window->focusables != 0) {
        IndirectDeallocate(window->focusables);
        window->focusables = 0;
    }
    while ((type = window->controlTypes) != 0) {
        window->controlTypes = type->next;
        MemoryDeallocate(type);
    }
    while ((preload = window->preloads) != 0) {
        window->preloads = preload->next;
        if (preload->image != 0)
            DT_ImageDispose(preload->image);
        if (preload->mask != 0)
            DT_ImageDispose(preload->mask);
        MemoryDeallocate(preload);
    }
    for (link = &gIT_ViewList; *link != 0; link = &(*link)->next) {
        if (*link == window) {
            *link = window->next;
            window->next = 0;
            break;
        }
    }
    MemoryDeallocate(window);
    window = 0;
    return 0;
}

/* IT_ViewExists (whether a window is on the window list). STATE: MATCH 100%, exe 18 / obj 18,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051af54
unsigned char IT_ViewExists(void *windowRef)
{
    WindowNode *window = (WindowNode *)windowRef;
    WindowNode *w;
    long err;

    err = 0;
    w = 0;
    for (w = gIT_ViewList; w != 0 && w != window; w = w->next)
        ;
    return w != 0 ? true : false;
}

/* IT_WindowFindView (the window record of a Windows window: a titled one first, then a child, then
   any). STATE: MATCH 100%, exe 37 / obj 37, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051af7d
WindowNode *IT_WindowFindView(void *hwnd)
{
    WindowNode *w;
    long err;

    err = 0;
    w = 0;
    for (w = gIT_ViewList; w != 0; w = w->next)
        if (w->titled && hwnd == w->hwnd)
            break;
    if (w == 0) {
        for (w = gIT_ViewList; w != 0; w = w->next)
            if (w->parent != 0 && hwnd == w->hwnd)
                break;
    }
    if (w == 0) {
        for (w = gIT_ViewList; w != 0; w = w->next)
            if (hwnd == w->hwnd)
                break;
    }
    return w;
}

/* _IT_ViewHandleIdle (a window's idle turn: its callback told (event 4), message 0x13 to its controls,
   and for a root window with images to preload, the next one loaded at most every half second).
   STATE: MATCH 100%, exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051afd6
long _IT_ViewHandleIdle(WindowNode *window)
{
    long err;
    long e;
    ULong elapsed;

    err = 0;
    if (window->page != 0) {
        e = _IT_ViewCallback(window, window->page, 0, kEventIdle);
        if (e != 0)
            err = e;
    }
    if (window->controls != 0) {
        e = _IT_ItemListHandleIdle(window, window->controls);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (window->parent == 0 && window->preloads != 0) {
        elapsed = TimerGetTicks() - window->preloadTicks;
        elapsed = elapsed * kMillisPerSecond / TimerGetTickRate();
        if (elapsed >= kPreloadGap) {
            _IT_ViewPreloadIdle(window);
            window->preloadTicks = TimerGetTicks();
        }
    }
done:
    return err;
}

/* _IT_ViewHandleKey (a key to a window's controls; a link it resolves to is followed from a top
   window, and a window still alive and dirty is refreshed). STATE: MATCH 100%, exe 68 / obj 68,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b060
long _IT_ViewHandleKey(WindowNode *window, const ITKeyEvent *key, ITClick *click)
{
    long err;
    long e;
    unsigned char ok;
    unsigned char isChild;

    err = 0;
    ok = 0;
    MemoryClear(click, sizeof(ITClick));
    if (window->parent != 0)
        isChild = true;
    else
        isChild = false;
    ok = isChild;
    if (window->controls != 0) {
        e = _IT_ItemListHandleKey(window, window->controls, key, click);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (isChild)
        goto done;
    if (click->window != 0 && click->control != 0) {
        e = _IT_ItemExecute(click->window, click->control, click->link);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (IT_ViewExists(window) && window->dirty) {
        e = _IT_ViewValidate(window);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _IT_ViewHandleEdit (an edit command, event 1..8, to the focused control of a window tree, then as
   _IT_ViewHandleKey). STATE: MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b0f7
long _IT_ViewHandleEdit(WindowNode *window, short event, ITClick *click)
{
    long err;
    long e;
    long result;
    unsigned char ok;
    unsigned char isChild;

    err = 0;
    ok = 0;
    MemoryClear(click, sizeof(ITClick));
    if (window->parent != 0)
        isChild = true;
    else
        isChild = false;
    ok = isChild;
    if (window->controls != 0 && window->focus->control != 0) {
        e = _IT_ItemListHandleEdit(window->focus->window, window->focus->window->controls, event, click);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else if (window->controls != 0) {
        result = _IT_ItemListHandleEdit(window, window->controls, event, click);
        if (result != 0) {
            err = result;
            goto done;
        }
    }
    if (isChild)
        goto done;
    if (click->window != 0 && click->control != 0) {
        e = _IT_ItemExecute(click->window, click->control, click->link);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (IT_ViewExists(window) && window->dirty) {
        e = _IT_ViewValidate(window);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _IT_ViewHandleClick (a click to a window's controls, then as _IT_ViewHandleKey). STATE: MATCH 100%, exe 68
   / obj 68, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b1b1
long _IT_ViewHandleClick(WindowNode *window, ITInputEvent *event, ITClick *click)
{
    long err;
    long e;
    unsigned char ok;
    unsigned char isChild;

    err = 0;
    ok = 0;
    MemoryClear(click, sizeof(ITClick));
    if (window->parent != 0)
        isChild = true;
    else
        isChild = false;
    ok = isChild;
    if (window->controls != 0) {
        e = _IT_ItemListHandleClick(window, window->controls, event, click);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (isChild)
        goto done;
    if (click->window != 0 && click->control != 0) {
        e = _IT_ItemExecute(click->window, click->control, click->link);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (IT_ViewExists(window) && window->dirty) {
        e = _IT_ViewValidate(window);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _IT_ViewHandleHit (activate the control holding a focusable item, then as _IT_ViewHandleKey). STATE:
   MATCH 100%, exe 73 / obj 73, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b248
long _IT_ViewHandleHit(WindowNode *window, long item, ITClick *click)
{
    long err;
    long e;
    unsigned char ok;
    unsigned char isChild;

    err = 0;
    ok = 0;
    MemoryClear(click, sizeof(ITClick));
    if (window->parent != 0)
        isChild = true;
    else
        isChild = false;
    ok = isChild;
    if (window->controls != 0) {
        e = _IT_GetObjectItem(window, window->controls, item, &click->window, &click->control);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (isChild)
        goto done;
    if (click->window != 0 && click->control != 0) {
        e = _IT_ItemExecute(click->window, click->control, click->link);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (IT_ViewExists(window) && window->dirty) {
        e = _IT_ViewValidate(window);
        if (e != 0)
            err = e;
    }
done:
    return err;
}

/* _IT_ViewHandleTest (whether any control of a window claims an item). STATE: MATCH 100%, exe 21 / obj
   21, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b2ec
long _IT_ViewHandleTest(WindowNode *window, ULong item, unsigned char *hit)
{
    long err;
    long e;

    err = 0;
    *hit = 0;
    if (window->controls != 0) {
        e = _IT_ItemListHandleTest(window, window->controls, item, hit);
        if (e != 0)
            err = e;
    }
    return err;
}

/* _IT_ViewHandleDraw (paint a window, or `area' of it: into the root's buffer when it has one (not when
   a control proc is running: then into the window itself), the backdrop tiled or the background
   colour, then the controls meeting the area; the buffer copied to the screen unless `noBlit';
   the area validated). STATE: MATCH 100%, exe 183 / obj 183, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b31e
long _IT_ViewHandleDraw(WindowNode *window, const ITRect *area, unsigned char noBlit)
{
    long err;
    long e;
    unsigned char rebuilt;
    WindowNode *root;
    ITImage *target;
    ITImage *image;
    void *saved;
    ITPixelInfo info;
    ITColor back;

    err = 0;
    root = window;
    rebuilt = 0;
    saved = 0;
    if (gIT_CurrentItem == 0) {
        for (root = window; root->parent != 0; root = root->parent)
            ;
    }
    if (root->buffer != 0) {
        e = DT_ImageLock(root->buffer->pixelsSelf, 0, &info, &rebuilt);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    target = root->buffer != 0 ? root->buffer->portSelf : window->image;
    image = target;
    saved = DT_GetClipRegion(target);
    if (saved == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_SetClipRect(target, area != 0 ? area : &window->bounds);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (window->backdrop != 0) {
        DT_DrawImageTile(target, area, 0, window->backdrop, 0, 0, 0);
    } else {
        e = DT_GetFillBackColor(target, &back);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_SetFillBackColor(target, &window->back);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_EraseRect(target, &window->bounds);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_SetFillBackColor(target, &back);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (window->controls != 0) {
        e = _IT_ItemListHandleDraw(window, window->controls, area);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (saved != 0) {
        e = DT_SetClipRegion(target, saved);
        if (e != 0) {
            err = e;
            goto done;
        }
        DT_RegionDispose(saved);
        saved = 0;
    }
    if (root->buffer != 0) {
        if (!noBlit) {
            e = DT_SurfaceBlitRects(root->buffer, 1, area != 0 ? area : &window->bounds);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        DT_ImageUnlock(root->buffer->pixelsSelf, 0);
    }
    e = DT_AddValidRect(window->image, area != 0 ? area : &window->bounds);
    if (e != 0)
        err = e;
done:
    if (saved != 0) {
        DT_SetClipRegion(image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* _IT_ViewHandleActivate (tell every control of a window that it activates or deactivates). STATE: MATCH
   100%, exe 17 / obj 17, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b520
long _IT_ViewHandleActivate(WindowNode *window, unsigned char active)
{
    long err;
    long e;

    err = 0;
    if (window->controls != 0) {
        e = _IT_ItemListHandleActivate(window, window->controls, active);
        if (e != 0)
            err = e;
    }
    return err;
}

/* _IT_ViewHandleCursor (hand a mouse event to the topmost active control under it; the event's modifiers
   are cleared when it falls inside the window). STATE: MATCH 100%, exe 37 / obj 37, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b549
long _IT_ViewHandleCursor(WindowNode *window, ITInputEvent *event, unsigned short *modifiers)
{
    long err;
    long e;

    err = 0;
    if (DT_IsPointInRect(&event->where, &window->bounds))
        *modifiers = 0;
    if (window->controls != 0) {
        e = _IT_ItemListHandleCursor(window, window->controls, event, modifiers);
        if (e != 0)
            err = e;
    }
    return err;
}

/* _IT_ViewGetFocus (a window's focus: its control, index and item when the focus is in this window,
   else none, -1 and none; each only when asked for). STATE: MATCH 100%, exe 50 / obj 50, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b596
long _IT_ViewGetFocus(WindowNode *window, ITControl **control, long *index, long *item)
{
    long err;

    err = 0;
    _IT_ViewValidateFocus(window, 0);
    if (window->focus->window == window) {
        if (control != 0)
            *control = window->focus->control;
        if (index != 0)
            *index = window->focus->index;
        if (item != 0)
            *item = window->focus->item;
    } else {
        if (control != 0)
            *control = 0;
        if (index != 0)
            *index = -1;
        if (item != 0)
            *item = 0;
    }
    return 0;
}

/* _IT_ViewSetFocus (move the focus of a window tree to `item', found in `control' or, with no
   control, among the window's controls; the old holder is told first and may refuse, in which
   case `keep' leaves the focus where it was). STATE: MATCH 100%, exe 163 / obj 163, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b60a
long _IT_ViewSetFocus(WindowNode *window, ITControl *control, long item, unsigned char keep)
{
    WindowNode *owner;
    long held;
    ITFocus *focus;
    long err;
    long e;
    long e2;

    err = 0;
    owner = window;
    _IT_ViewValidateFocus(window, 0);
    if (control != 0 && item == 0 && IndirectGetSize(control->items) != 0) {
        item = *(long *)*control->items;
        e = _IT_GetObjectItem(window, window->controls, item, &owner, &control);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else if (item != 0 && control == 0) {
        e2 = _IT_GetObjectItem(window, window->controls, item, &owner, &control);
        if (e2 != 0) {
            err = e2;
            goto done;
        }
    }
    for (; window->parent != 0; window = window->parent)
        ;
    if (owner->controls != 0) {
        focus = window->focus;
        held = focus->item;
        if (held != 0 && item != held) {
            err = _IT_ViewCallback(focus->window, 0, focus->control, kEventBlur);
            if (err == kFocusRefused) {
                err = 0;
                if (keep) {
                    control = window->focus->control;
                    item = window->focus->item;
                }
            }
            e = err;
            if (err != 0)
                goto done;
            e = _IT_ItemCallback(window->focus->window, window->focus->control, kMsgFocus, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            window->focus->window = 0;
            window->focus->control = 0;
            window->focus->item = 0;
            window->focus->index = -1;
        }
        _IT_ViewValidateFocus(window, 0);
        if (item != 0 && item != window->focus->item) {
            e = _IT_ItemCallback(owner, control, kMsgFocus, item, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            window->focus->window = owner;
            window->focus->control = control;
            window->focus->item = item;
            window->focus->index = _IT_GetItemIndex(owner->controls, control);
            e = _IT_ViewCallback(owner, 0, control, kEventFocus);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    if (window->dirty != 0) {
        e = _IT_ViewValidate(window);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    _IT_ViewValidateFocus(window, 0);
done:
    return err;
}

/* _IT_ViewMoveFocus (move the focus to the next or previous focusable item of the window tree,
   wrapping, skipping the items of `skipWindow' and `skipControl' and, when `only' is given, every
   other item; the focus is cleared when nothing qualifies). STATE: MATCH 100%, exe 152 / obj 152,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b7fc
long _IT_ViewMoveFocus(WindowNode *window, unsigned char forward, WindowNode *skipWindow,
                  ITControl *skipControl, long only)
{
    long err;
    long e;
    long e2;
    ULong count;
    long item;
    void *current;
    ULong start;
    ULong i;
    ULong j;
    ULong n;
    unsigned char clear;

    err = 0;
    clear = 0;
    _IT_ViewValidateFocus(window, 0);
    for (; window->parent != 0; window = window->parent)
        ;
    if (window->controls != 0 && IndirectGetSize(window->focusables) != 0) {
        count = IndirectGetSize(window->focusables) >> 2;
        i = count;
        current = (void *)window->focus->item;
        if (current != 0) {
            for (i = 0; i < count; i++)
                if (((void **)*window->focusables)[i] == current)
                    break;
        }
        if (i >= count) {
            if (forward != 0)
                i = count - 1;
            else
                i = 0;
        }
        start = i;
        while (start != (i = (i + count + (forward != 0 ? 1 : -1)) % count)) {
            item = ((long *)*window->focusables)[i];
            if (only != 0 && only != item)
                continue;
            if (skipControl != 0) {
                n = IndirectGetSize(skipControl->items) >> 2;
                for (j = 0; j < n; j++)
                    if (item == ((long *)*skipControl->items)[j])
                        break;
                if (j < n)
                    continue;
            }
            if (skipWindow == 0)
                break;
            n = IndirectGetSize(skipWindow->focusables) >> 2;
            for (j = 0; j < n; j++)
                if (item == ((long *)*skipWindow->focusables)[j])
                    break;
            if (j >= n)
                break;
        }
        if (start != i) {
            e = _IT_ViewSetFocus(window, 0, ((long *)*window->focusables)[i], 0);
            if (e != 0) {
                err = e;
                goto out;
            }
        } else {
            if (skipWindow == 0 && skipControl == 0 && only == 0)
                goto done;
            e2 = _IT_ViewSetFocus(window, 0, 0, 1);
            if (e2 != 0) {
                err = e2;
                goto out;
            }
        }
    }
done:
    _IT_ViewValidateFocus(window, 0);
out:
    return err;
}

/* _IT_ViewQueryFocus (rebuild a window's list of focusable items from its controls, and append it to
   `out' when given). STATE: MATCH 100%, exe 42 / obj 42, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051b9a9
long _IT_ViewQueryFocus(WindowNode *window, void **out)
{
    long err;
    long e;

    err = 0;
    _IT_ViewValidateFocus(window, 0);
    if ((e = IndirectSetSize(window->focusables, 0)) != 0 ||
        (e = _IT_ItemListQueryFocus(window, window->controls, window->focusables)) != 0) {
        err = e;
    } else if (out != 0) {
        e = IndirectAppend(out, window->focusables);
        if (e != 0)
            err = e;
    }
    return err;
}

/* _IT_ViewValidateFocus (a window tree's consistency probe: the window, or its root, has its focus record;
   always true). STATE: MATCH 100%, exe 22 / obj 22, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ba0e
unsigned char _IT_ViewValidateFocus(WindowNode *window, long self)
{
    WindowNode *root;
    ITFocus *focus;
    long err;
    unsigned char found;

    err = 0;
    found = false;
    root = 0;
    for (root = window; root->parent != 0; root = root->parent)
        ;
    if (self == 0)
        window = root;
    focus = window->focus;
    found = true;
    return found;
}

/* _IT_ViewInvalidate (mark a window dirty and its whole area to be redrawn). STATE: MATCH 100%, exe 28
   / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ba41
long _IT_ViewInvalidate(WindowNode *window)
{
    long err;
    long e;
    ITRect everything;

    err = 0;
    window->dirty = 1;
    DT_SetRect(&everything, kMinLong, kMinLong, kMaxLong, kMaxLong);
    e = DT_AddInvalidRect(window->image, &everything);
    if (e != 0)
        err = e;
    return err;
}

/* _IT_ViewValidate (bring a window up to date: its area invalidated, its caption (title, then " / "
   and the page's title) and colours applied to the Windows window, the dirty flag cleared).
   STATE: MATCH 100%, exe 109 / obj 109, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ba8d
long _IT_ViewValidate(WindowNode *window)
{
    long err;
    void *hwnd;
    void *dc;
    long e;
    ULong textColor;
    ULong backColor;
    char current[256];
    char caption[256];

    err = 0;
    hwnd = 0;
    hwnd = window->hwnd;
    if (hwnd != 0) {
        e = DT_AddInvalidRect(window->image, &window->bounds);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (window->titled) {
        StringCopySafe(caption, window->title, sizeof(caption));
        if (window->page != 0) {
            if (caption[0] != 0 && window->page->title[0] != 0)
                StringAppendSafe((unsigned char *)caption, (const unsigned char *)" / ", sizeof(caption));
            StringAppendSafe((unsigned char *)caption, (const unsigned char *)window->page->title,
                      sizeof(caption));
        }
        GetWindowTextA(hwnd, current, sizeof(current) - 1);
        if (StringCompare(caption, current) != 0 && SetWindowTextA(hwnd, caption) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        dc = GetDC(window->hwnd);
        if (dc != 0) {
            textColor = DT_2SysColor(&window->fore);
            backColor = DT_2SysColor(&window->back);
            SetTextColor(dc, textColor);
            SetBkColor(dc, backColor);
            ReleaseDC(window->hwnd, dc);
        }
    }
    window->dirty = 0;
done:
    return err;
}

/* _IT_ViewCheckPage (ask whether a window may leave its page: its callback (event 6) and the focus
   holder (a blur) may refuse with -8404, which clears `may'). STATE: MATCH 100%, exe 42 / obj 42,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051bbec
long _IT_ViewCheckPage(WindowNode *window, unsigned char *may)
{
    long err;
    long e;

    err = 0;
    *may = 1;
    if (window->page != 0) {
        if (*may) {
            err = _IT_ViewCallback(window, window->page, 0, kEventLeave);
            if (err == kFocusRefused) {
                err = 0;
                *may = 0;
            }
            if (err != 0)
                goto done;
        }
        if (*may) {
            err = _IT_ViewSetFocus(window, 0, 0, 0);
            if (err == kFocusRefused) {
                err = 0;
                *may = 0;
            }
            if (err != 0)
                e = err;
        }
    }
done:
    return err;
}

/* _IT_ViewUnload (drop a window's content: its pages, its controls, its backdrop and its source
   document). STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051bc51
void _IT_ViewUnload(WindowNode *window)
{
    long err;

    err = 0;
    if (window->pages != 0) {
        _IT_PageListDispose(window->pages);
        window->pages = 0;
        window->page = 0;
    }
    if (window->controls != 0) {
        _IT_ItemListDispose(window, window->controls);
        window->controls = 0;
    }
    if (window->backdrop != 0) {
        DT_ImageDispose(window->backdrop);
        window->backdrop = 0;
    }
    _IT_ParseTupleDispose(&window->source);
}

/* _IT_ViewGoToPage (show a page in a window, drawing clipped to nothing meanwhile: the old page's
   controls dropped, the callback told before (2) and after (3) the new page's controls are built
   from its element's children, the focus taken when nothing holds it, its images queued for
   preloading; the window refreshed when dirty). STATE: MATCH 100%, exe 162 / obj 162, byte-
   identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051bcbe
long _IT_ViewGoToPage(WindowNode *window, ITPage *page)
{
    long err;
    ITRect clip;
    void *saved;
    long e;
    long result;
    unsigned char first;

    err = 0;
    result = 0;
    saved = 0;
    clip = gIT_ShowPageZeroRect;
    first = true;
    saved = DT_GetClipRegion(window->image);
    if (saved == 0)
        goto nomem;
    e = DT_SetClipRect(window->image, &clip);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (window->page != 0 && window->page != page) {
        e = DT_AddInvalidRect(window->image, &window->bounds);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (window->controls != 0)
            _IT_ItemListDispose(window, window->controls);
        window->controls = 0;
        window->page = 0;
    }
    if (page != 0 && page->element != 0 && window->page != page) {
        window->page = page;
        e = _IT_ViewCallback(window, page, 0, kEventPageOpen);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_AddInvalidRect(window->image, &window->bounds);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (_IT_ItemListCreate(window, page->element->child) == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
        e = _IT_ViewCallback(window, page, 0, kEventPageShown);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (gIT_CurrentView == 0 || gIT_CurrentView->focus->control == 0) {
            e = _IT_ViewMoveFocus(window, 1, 0, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        e = _IT_ViewPreload(window, page->element->child);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = DT_SetClipRegion(window->image, saved);
    if (e != 0) {
        err = e;
        goto done;
    }
    DT_RegionDispose(saved);
    saved = 0;
    if (window->dirty && (e = _IT_ViewValidate(window)) != 0)
        err = e;
done:
    if (saved != 0) {
        DT_SetClipRegion(window->image, saved);
        DT_RegionDispose(saved);
    }
    return err;
}

/* _IT_ViewSetRect (move a window: when its origin changes every control is moved by the same amount,
   last first). STATE: MATCH 100%, exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051be77
long _IT_ViewSetRect(WindowNode *window, const ITRect *bounds)
{
    long n;
    ITRect r;
    long err;

    err = 0;
    if (bounds->left != window->bounds.left || bounds->top != window->bounds.top) {
        n = IT_ViewGetItemCount(window);
        while (n--) {
            IT_ViewGetItemRect(window, n, &r);
            DT_RectOffset(&r, bounds->left - window->bounds.left,
                         bounds->top - window->bounds.top);
            IT_ViewSetItemRect(window, n, &r);
        }
    }
    window->bounds = *bounds;
    return 0;
}

/* _IT_ViewSetEnabled (show and enable a window: each control's enabled state re-applied, last first).
   STATE: MATCH 100%, exe 33 / obj 33, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051bef0
long _IT_ViewSetEnabled(WindowNode *window, unsigned char visible, unsigned char enabled)
{
    long n;
    unsigned char on;
    long err;

    err = 0;
    window->visible = visible;
    window->enabled = enabled;
    n = IT_ViewGetItemCount(window);
    while (n--) {
        IT_ViewGetItemEnabled(window, n, &on);
        IT_ViewSetItemEnabled(window, n, on);
    }
    return 0;
}

/* _IT_ViewGetItemColor (offer an item to a window's controls: the first that claims it gets messages 0x4c
   and 0x4e). STATE: MATCH 100%, exe 23 / obj 23, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051bf40
long _IT_ViewGetItemColor(WindowNode *window, ULong item, void *a, void *b, unsigned char *hit)
{
    long err;
    long e;

    err = 0;
    *hit = 0;
    if (window->controls != 0) {
        e = _IT_GetItemColor(window, window->controls, item, a, b, hit);
        if (e != 0)
            err = e;
    }
    return err;
}

/* _IT_ViewCallback (tell a window's callback about an event on a control, or on the window's page:
   where, the control's index, the control's NAME; the current window and control are hidden from
   it meanwhile). STATE: MATCH 100%, exe 81 / obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051bf7a
long _IT_ViewCallback(WindowNode *window, ITPage *page, ITControl *control, long event)
{
    char *name;
    WindowNode *savedWindow;
    ITControl *savedControl;
    long result;
    long index;
    void *where;
    char buffer[256];

    result = 0;
    index = -1;
    where = 0;
    if (window->proc != 0) {
        savedWindow = gIT_CurrentView;
        savedControl = gIT_CurrentItem;
        gIT_CurrentView = 0;
        gIT_CurrentItem = 0;
        if (page != 0)
            where = page->name;
        else if (window->page != 0)
            where = window->page->name;
        if (control != 0) {
            index = _IT_GetItemIndex(window->controls, control);
            name = _IT_SimpleFindAttribute(control->element, "NAME");
            if (name != 0)
                StringCopySafe(buffer, name, sizeof(buffer));
        }
        result = window->proc(window, window->refcon, event, where, index,
                              control != 0 && name != 0 ? buffer : 0);
        gIT_CurrentView = savedWindow;
        gIT_CurrentItem = savedControl;
    }
    return result;
}

/* _IT_ViewPreload (queue the images a page asks to preload: every PRELOAD element with a SRC and an
   IMG or IMAGE attribute not queued yet goes on the root's list, pending). STATE: MATCH 100%, exe
   106 / obj 106, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051c06e
long _IT_ViewPreload(WindowNode *window, ITElement *root)
{
    long n;
    char *src;
    long err;
    ITPreload *p;
    ITElement *e;
    WindowNode *top;
    char name[256];

    err = 0;
    src = 0;
    n = 0;
    e = 0;
    p = 0;
    for (top = window; top->parent != 0; top = top->parent)
        ;
    while ((e = _IT_ParseTreeFindNextTag(root, e, "PRELOAD", 1)) != 0) {
        src = _IT_SimpleFindAttribute(e, "SRC");
        if (src == 0 || *src == 0)
            continue;
        StringCopySafe(name, src, sizeof(name));
        if (_IT_SimpleFindAttribute(e, "IMG") == 0 && _IT_SimpleFindAttribute(e, "IMAGE") == 0)
            continue;
        for (p = top->preloads; p != 0 && StringCompare(name, p->name) != 0; p = p->next)
            ;
        if (p == 0) {
            p = MemoryAllocateClear(StringGetLength((unsigned char *)name) + sizeof(ITPreload));
            if (p == 0) {
                err = kOutOfMemory;
                break;
            }
            StringCopy(p->name, name);
            p->next = top->preloads;
            top->preloads = p;
            p->pending = 1;
        }
        p = 0;
    }
    return err;
}

/* _IT_ViewPreloadIdle (preload the first pending image of a window from its page folder; a missing file
   (-7201..-7203) only drops the request, any other error is answered). STATE: MATCH 100%, exe 90
   / obj 90, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051c181
long _IT_ViewPreloadIdle(WindowNode *window)
{
    ITPreload *p;
    void **data;
    long err;
    long e;
    char message[256];

    err = 0;
    p = 0;
    data = 0;
    for (p = window->preloads; p != 0 && !p->pending; p = p->next)
        ;
    if (p != 0 && p->pending) {
        err = FT_FileGetFlags((long)window->source.folder, p->name, 0, 0);
        if (err == 0) {
            e = FT_FileLoad((long)window->source.folder, p->name, kChunkMax, 1, &data);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_ImageFileLoad(data, &p->image, &p->mask, &p->transparent, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            p->pending = 0;
            p->image = 0;
            StringFormatSafe(message, sizeof(message), "Unable to preload image '%s'", p->name);
            if (err != kFTErrFileNotFound && err != kFTErrBadName
                && err != kFTErrDirNotFound)
                goto done;
            err = 0;
        }
        p->stamp = TimerGetElapsedSeconds();
        p->pending = 0;
    }
done:
    if (data != 0)
        IndirectDeallocate(data);
    return err;
}

/* _IT_ViewDissectLink (split a link `file#anchor' into fresh strings; `#anchor' alone names the window's
   current file; -8400 for a URL, which the tool does not follow). STATE: MATCH 100%, exe 119 /
   obj 119, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051c289
long _IT_ViewDissectLink(WindowNode *window, const char *link, char **file, char **anchor)
{
    char *hash;
    ULong size;
    long err;

    err = 0;
    hash = 0;
    *file = 0;
    *anchor = 0;
    if (link != 0 && StringFindString(link, "://") != 0) {
        err = kExternalLink;
        goto done;
    }
    if (link != 0 && *link == '#') {
        *file = StringDuplicate(window->source.file);
        if (*file == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (link[1] == 0)
            goto done;
        *anchor = StringDuplicate(link + 1);
        if (*anchor == 0) {
            err = kOutOfMemory;
            goto done;
        }
        goto done;
    }
    if (link == 0 || *link == 0)
        goto done;
    hash = StringFindDelimiter(link, "#");
    if (hash != 0) {
        size = hash - link + 1;
        *file = MemoryAllocate(size);
        if (*file == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if (hash[1] != 0) {
            *anchor = StringDuplicate(hash + 1);
            if (*anchor == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
        StringCopySafe(*file, link, size);
        goto done;
    }
    *file = StringDuplicate(link);
    if (*file == 0)
        err = kOutOfMemory;
done:
    if (err != 0 && *file != 0) {
        MemoryDeallocate(*file);
        *file = 0;
    }
    if (err != 0 && *anchor != 0) {
        MemoryDeallocate(*anchor);
        *anchor = 0;
    }
    return err;
}

/* _IT_ParseTupleCreate (open a page source: a folder named by `file' gives its index.iml (-8405 when it
   has none), a file is read from its own folder (the current folder swapped in around the lookup
   and restored); the text converted when its encoding is known (-8406 when it is Unicode without
   a converter) and parsed with SCRIPT raw and LI, BR, P empty; a source that already has a
   document only takes the folder). STATE: MATCH 100%, exe 173 / obj 173, byte-identical yes.
   OPEN: none (the first and fifth arguments are never read) */
// FUNCTION: EVNOVA 0x0051c3c1
long _IT_ParseTupleCreate(WindowNode *window, ITSource *source, long where, const char *file,
                  void *unused)
{
    long err;
    long e;
    void **data;
    struct FolderSpec **current;
    FTFileInfo info;

    err = 0;
    data = 0;
    current = 0;
    if (source->root != 0) {
        e = FT_PathCreate((struct FolderSpec **)where, 0, &source->folder);
        if (e != 0)
            goto fail;
        source->file = StringDuplicate(kIndexFile);
        if (source->file != 0)
            goto done;
        err = kOutOfMemory;
        goto done;
    }
    e = FT_FileGetFlags(where, file, &info, 1);
    if (e != 0)
        goto fail;
    if (info.flags[kInfoFolder]) {
        e = FT_PathCreate((struct FolderSpec **)where, file, &source->folder);
        if (e != 0)
            goto fail;
        err = FT_FileGetFlags((long)source->folder, kIndexFile, &info, 0);
        if (err == kFTErrFileNotFound) {
            err = kNoIndex;
            goto done;
        }
        e = err;
        if (err == 0) {
        } else
            goto done;
    } else {
        e = FT_PathCreate(0, 0, &current);
        if (e != 0)
            goto fail;
        e = FT_PathCurrent((const char *)where, 0);
        if (e != 0)
            goto fail;
        e = FT_PathCreate((struct FolderSpec **)file, (const char *)kNoLength, &source->folder);
        if (e != 0)
            goto fail;
        e = FT_PathCurrent((const char *)current, 0);
        if (e != 0)
            goto fail;
        FT_PathDispose(current);
        current = 0;
    }
    if ((source->file = StringDuplicate(info.name)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = FT_FileLoad((long)source->folder, source->file, kChunkMax, 1, &data);
    if (e != 0)
        goto fail;
    if (!IndirectIsNormalizedEncoding(data)) {
        if (IndirectIsUnicode(data, 0)) {
            err = kNoConverter;
            goto done;
        }
        e = IndirectNormalizeEncoding(data, 0);
        if (e != 0)
            goto fail;
    }
    e = PT_ParseTreeLoad(data, "SCRIPT", "LI BR P", 0, &source->root);
    if (e != 0)
        goto fail;
    IndirectDeallocate(data);
    data = 0;
done:
    if (current != 0) {
        e = FT_PathCurrent((const char *)current, 0);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        FT_PathDispose(current);
    }
    return err;
}

/* _IT_ParseTupleDispose (dispose a page source: its document tree, its folder, its data). STATE: MATCH
   100%, exe 32 / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051c5c9
void _IT_ParseTupleDispose(ITSource *source)
{
    long err;

    err = 0;
    if (source->root != 0) {
        PT_ParseTreeDispose(source->root);
        source->root = 0;
        source->interfaceElement = 0;
    }
    if (source->folder != 0) {
        FT_PathDispose(source->folder);
        source->folder = 0;
    }
    if (source->file != 0) {
        MemoryDeallocate(source->file);
        source->file = 0;
    }
}

/* IT_WindowProc (the window procedure of the interface tool's window classes: the body 0x004eae3e,
   else Windows' default). STATE: MATCH 100%, exe 37 / obj 37, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051c610
long __stdcall IT_WindowProc(void *hwnd, unsigned int msg, unsigned int wParam, long lParam)
{
    long result;
    long err;

    err = 0;
    result = 0;
    if (!IT_HandleWinEvent(hwnd, 0, msg, wParam, lParam, &result))
        result = DefWindowProcA(hwnd, msg, wParam, lParam);
    return result;
}

/* IT_ViewCreateParam (the dialog procedure 0x0051a695 runs: WM_INITDIALOG makes the modal root window
   record (0x0051a4ee), loads the page file, sizes the dialog to the INTERFACE frame a sixth of
   the screen above centre, buffers and paints it and starts a 100 ms idle timer (the interface
   tool's idle turn, 0x004d335c); everything else goes to 0x004eae3e; the answer is whether that
   handled it). STATE: MATCH 100%, exe 237 / obj 237, byte-identical yes. OPEN: none; a failed
   WM_INITDIALOG leaves the dialog up with no page, as in the exe */
// FUNCTION: EVNOVA 0x0051c664
long __stdcall IT_ViewCreateParam(void *hwnd, unsigned int msg, unsigned int wParam, long lParam)
{
    long handled;
    long err;
    long e;
    long result;
    WindowNode *window;
    DialogParams *params;
    ITRect r;
    WinRect client;

    err = handled = 0;
    result = 0;
    window = 0;
    params = 0;
    switch (msg) {
    case WM_INITDIALOG:
        window = MemoryAllocateClear(sizeof(WindowNode));
        if (window == 0) {
            err = kOutOfMemory;
            goto done;
        }
        window->next = gIT_ViewList;
        gIT_ViewList = window;
        window->hwnd = hwnd;
        if ((window->dc = GetDC(window->hwnd)) == 0 || !GetClientRect(window->hwnd, &client))
            goto gle;
        CopySys2DT_Rect(&window->bounds, (ITRect *)&client);
        window->titled = 1;
        window->modal = 1;
        window->unbuffered = 1;
        params = (DialogParams *)lParam;
        window->result = params->result;
        e = IT_ViewCreate(kNoHwnd, 0, params->proc, params->refcon, 0, &window);
        if (e != 0)
            goto fail;
        e = IT_ViewChange(window, params->folder, params->file, params->path, 0);
        if (e != 0)
            goto fail;
        if (!GetClientRect(GetDesktopWindow(), &client)) {
gle:
            err = GetLastError();
            goto nomem;
        }
        CopySys2DT_Rect(&r, (ITRect *)&client);
        DT_RectCenter(&r, &window->frame, &r);
        DT_RectOffset(&r, 0, (client.top - client.bottom) / 6);
        CopyDT_2SysRect((ITRect *)&client, &r);
        if (!AdjustWindowRect(&client, WS_CAPTION, 0))
            goto gle2;
        CopySys2DT_Rect(&r, (ITRect *)&client);
        MoveWindow(window->hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 0);
        SendMessageA(hwnd, DM_REPOSITION, 0, 0);
        if (!GetClientRect(window->hwnd, &client)) {
gle2:
            err = GetLastError();
            err = kDialogFailed;
            goto done;
        }
        CopySys2DT_Rect(&window->bounds, (ITRect *)&client);
        if ((window->buffer = DT_SurfaceCreateFromWindow(window->hwnd)) == 0) {
nomem:
            err = kOutOfMemory;
            goto done;
        }
        e = _IT_ViewHandleDraw(window, 0, 0);
        if (e != 0) {
fail:
            err = e;
            goto done;
        }
        SetForegroundWindow(window->hwnd);
        goto timer;
    case WM_TIMER:
        if (wParam == kIdleTimer) {
            SystemIdle();
timer:
            SetTimer(hwnd, kIdleTimer, kIdleTimerMs, 0);
            break;
        }
        /* any other timer is the window's */
    default:
        IT_HandleWinEvent(hwnd, 0, msg, wParam, lParam, &result);
        if (result != 0)
            handled = 1;
        break;
    }
done:
    return handled;
}
