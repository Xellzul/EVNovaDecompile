// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* ittextbox.c -- the interface tool's TEXTBOX control (0x00521fd0..0x00524337): a read-only rich edit showing the element's marked-up text with followable links, or a plain EDIT. Notes: Research/notes/ittextbox.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"

extern long IT_ItemCallbackGrabFocus(long item);                                     /* interfacetool.c */
extern void IT_ItemCallbackGetFont(char **face, long *size, ULong *style, ITColor *color,
                         char **href);                                   /* interfacetool.c */
extern char *IT_ItemCallbackGetNextRun(const char *after, char **face, long *size, ULong *style,
                          ITColor *color, char **href, ULong *order);    /* interfacetool.c */
extern ITElement *IT_ItemCallbackGetNextNode(ITElement *after, char **face, long *size, ULong *style,
                               ITColor *color, char **href,
                               ULong *order);                            /* interfacetool.c */
extern ITColor DT_GetColorARGB(ULong argb);                                 /* itrect.c */
extern char *_DT_GetFontByNameList(void *dc, const char *faces, char *out);      /* itgfx.c */

/* user32's InvertRect; not in win32.h, where it would meet macport.h's Toolbox InvertRect */
__declspec(dllimport) int __stdcall InvertRect(void *hdc, const WinRect *r);

/* richedit.h's CHARFORMATA */
typedef struct CHARFORMATA {
    ULong cbSize;                                /* +0x00 */
    ULong dwMask;                                /* +0x04 */
    ULong dwEffects;                             /* +0x08 */
    long yHeight;                                /* +0x0c */
    long yOffset;                                /* +0x10 */
    ULong crTextColor;                           /* +0x14 */
    unsigned char bCharSet;                      /* +0x18 */
    unsigned char bPitchAndFamily;               /* +0x19 */
    char szFaceName[32];                         /* +0x1a */
} CHARFORMATA;                                   /* 0x3c */

/* richedit.h's FORMATRANGE */
typedef struct FORMATRANGE {
    void *hdc;
    void *hdcTarget;
    WinRect rc;
    WinRect rcPage;
    long cpMin;
    long cpMax;
} FORMATRANGE;

/* richedit.h's GETTEXTLENGTHEX */
typedef struct GETTEXTLENGTHEX {
    ULong flags;
    ULong codepage;
} GETTEXTLENGTHEX;

/* one style run: the rich edit's character format and the font the plain EDIT uses */
typedef struct ITTextFormat {
    CHARFORMATA cf;
    LOGFONTA lf;
} ITTextFormat;

/* a followable run of the text: characters [start, start + length) and its HREF */
typedef struct ITTextLink {
    ULong start;
    ULong length;
    char *href;
} ITTextLink;

/* the control's data handle */
typedef struct ITTextBox {
    ITColor fore;                                /* +0x000 FGCOLOR */
    ITColor back;                                /* +0x004 BGCOLOR */
    ITColor link;                                /* +0x008 */
    unsigned char foreSet;                       /* +0x00c */
    unsigned char backSet;                       /* +0x00d */
    unsigned char editable;                      /* +0x00e EDITABLE: a plain EDIT */
    unsigned char convert;                       /* +0x00f never set: text through 0x0050586e */
    unsigned char single;                        /* +0x010 SINGLE, or too low for two lines */
    unsigned char pad11[3];
    long align;                                  /* +0x014 ALIGN */
    unsigned char aligned;                       /* +0x018 */
    unsigned char allow[256];                    /* +0x019 ALLOW: the characters accepted */
    unsigned char pad119[3];
    ITRect bounds;                               /* +0x11c */
    unsigned char focused;                       /* +0x12c */
    unsigned char active;                        /* +0x12d */
    unsigned char visible;                       /* +0x12e */
    unsigned char enabled;                       /* +0x12f */
    void **links;                                /* +0x130 ITTextLink */
    void *edit;                                  /* +0x134 the edit window */
    void *font;                                  /* +0x138 the plain EDIT's font */
    long depth;                                  /* +0x13c the masks' bits per pixel */
    WinRect maskRect;                            /* +0x140 */
    void *textDC;                                /* +0x150 */
    void *maskDC;                                /* +0x154 */
    void *textBitmap;                            /* +0x158 */
    void *oldTextBitmap;                         /* +0x15c */
    void *maskBitmap;                            /* +0x160 */
    void *oldMaskBitmap;                         /* +0x164 */
} ITTextBox;

#define kNotFocusable       -7002
#define kLinkColour         0xff0000eeUL
#define kFirstChildId       0x26ac
#define kDefaultPoints      12
#define kTwipsPerInch       1440
#define kLinkMax            0x400
#define kModButtonDown      0x10000000
#define kEventTrack         12
#define kEventOut           13
#define kEventIn            14
#define kEventRelease       15
#define kAlignCenter        0

#define WS_EX_TRANSPARENT   0x00000020
#define ES_CENTER           0x0001
#define ES_RIGHT            0x0002
#define ES_MULTILINE        0x0004
#define ES_AUTOVSCROLL      0x0040
#define ES_AUTOHSCROLL      0x0080
#define ES_SAVESEL          0x8000
#define EM_SETSEL           0x00b1
#define EM_GETLINECOUNT     0x00ba
#define EM_LINEINDEX        0x00bb
#define EM_REPLACESEL       0x00c2
#define EM_SETREADONLY      0x00cf
#define EM_POSFROMCHAR      0x00d6
#define EM_CHARFROMPOS      0x00d7
#define EM_EXLINEFROMCHAR   0x0436
#define EM_FORMATRANGE      0x0439
#define EM_SETBKGNDCOLOR    0x0443
#define EM_SETCHARFORMAT    0x0444
#define EM_SETTARGETDEVICE  0x0448
#define EM_GETTEXTLENGTHEX  0x045f
#define GTL_PRECISE         2
#define SCF_SELECTION       1
#define CFM_BOLD            0x00000001UL
#define CFM_ITALIC          0x00000002UL
#define CFM_UNDERLINE       0x00000004UL
#define CFM_FACE            0x20000000UL
#define CFM_COLOR           0x40000000UL
#define CFM_SIZE            0x80000000UL
#define CFE_UNDERLINE       0x00000004UL
#define FIXED_PITCH         1

unsigned char gControlsInitialized;                      /* 0x00870e0b */
unsigned char gRichEdit20;                      /* 0x00870e05 */
void *gTBRichEditModule;                              /* 0x00870704 */

long IT_TextBoxMakeBuffers(ITTextBox *tb);
long IT_TextBoxPaint(ITTextBox *tb, void *dc);
void IT_TextBoxFreeBuffers(ITTextBox *tb);
long IT_TextBoxCreateEdit(void *hwnd, void *dc, ITTextBox *tb);
long _IT_TextFormat(void *hwnd, void *dc, ITTextBox *tb, ITTextFormat *format, const char *face,
                  long size, ULong style, const ITColor *color, ULong order);
long IT_AppendText(void **out, unsigned char upper, const char *text, unsigned char convert,
                  unsigned char collapse, unsigned char afterSpace);
long _IT_TextCheck(void *hwnd, void *dc, ITTextBox *tb, ITPoint *pt, unsigned char track,
                  ITClick *click);
void _IT_HiliteLink(void *hwnd, void *dc, ITTextBox *tb, const ITRect *r1, const ITRect *r2,
                  const ITRect *r3);

/* the characters ALLOW may name */
static char sAllowable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz `1234567890~!@#$%^&*()-=_+[]\\{}|;':\",./<>?";

/* IT_EditIdEnumProc (EnumChildWindows's callback: a child already has the id in `*probe', which
   then becomes 0; go on while it is not). STATE: MATCH 100%, exe 16 / obj 16, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x00521fd0
int __stdcall IT_EditIdEnumProc(void *child, long probe)
{
    long *id;

    id = (long *)probe;
    if (*id == GetDlgCtrlID(child))
        *id = 0;
    return *id != 0 ? true : false;
}

/* IT_FindFreeChildId (a child window id for the edit). STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00521ff9
long IT_FindFreeChildId(void *parent)
{
    long id;
    long probe;

    id = kFirstChildId;
    /* BUG: probe starts at 0 and is overwritten after the first enumeration whatever it
       found, so the answer is always kFirstChildId + 1 */
    for (probe = 0; probe == 0; probe = ++id)
        EnumChildWindows(parent, IT_EditIdEnumProc, (long)&probe);
    return probe;
}

/* IT_EditTextLength (the edit's text length: EM_GETTEXTLENGTHEX on a RichEdit 2.0, else
   WM_GETTEXTLENGTH). STATE: MATCH 100%, exe 22 / obj 22, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00522033
long IT_EditTextLength(void *edit, unsigned char richEdit20)
{
    GETTEXTLENGTHEX how = { GTL_PRECISE, 0 };

    if (richEdit20)
        return SendMessageA(edit, EM_GETTEXTLENGTHEX, (unsigned int)&how, 0);
    return SendMessageA(edit, WM_GETTEXTLENGTH, 0, 0);
}

/* IT_EditCharPos (where a character of the edit sits: packed into the answer on a RichEdit
   2.0, else through a POINTL). STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00522079
ITPoint IT_EditCharPos(void *edit, ULong index, unsigned char packed)
{
    ITPoint pt;
    ULong pos;

    if (packed) {
        pos = SendMessageA(edit, EM_POSFROMCHAR, index, 0);
        pt.v = (pos >> 16) & 0xffff;
        pt.h = pos & 0xffff;
    } else {
        SendMessageA(edit, EM_POSFROMCHAR, (unsigned int)&pt, index);
    }
    return pt;
}

/* IT_TextHandler (the TEXTBOX control proc: 0 makes its data (EDITABLE, FGCOLOR, BGCOLOR,
   ALLOW, ALIGN), 1 builds the edit, 2 destroys it, 3 frees the data; hit tests, painting,
   link tracking and the hand cursor, keys, focus, bounds, text, show / enable, colours).
   STATE: MATCH 100%, exe 713 / obj 713, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005220d0
long IT_TextHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    long err;
    long e;
    unsigned char dead;
    unsigned char update;
    ITTextBox *tb;
    ULong i;
    ULong n;
    ITRect r;
    ITPoint pt;
    POINT origin;
    ITClick click;
    char nul;

    tb = *data;
    dead = 0;
    err = 0;
    update = 0;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITTextBox));
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(data, 1);
        tb = *data;
        MemoryClear(*data, sizeof(ITTextBox));
        tb->fore = gDT_ColorBlack;
        tb->back = gDT_ColorWhite;
        tb->link = DT_GetColorARGB(kLinkColour);
        tb->enabled = 1;
        tb->visible = 1;
        tb->active = 1;
        tb->focused = 0;
        tb->convert = 0;
        tb->aligned = IT_ItemCallbackGetAlign(&tb->align);
        {
            char *value;

            value = IT_ItemCallbackGetParam("EDITABLE", 0);
            if (value != 0)
                tb->editable = IT_ItemCallbackGetFlag(value);
            value = IT_ItemCallbackGetParam("FGCOLOR", 1);
            if (value != 0) {
                tb->fore = IT_ItemCallbackGetColor(value);
                tb->foreSet = 1;
            }
            value = IT_ItemCallbackGetParam("BGCOLOR", 1);
            if (value != 0) {
                tb->back = IT_ItemCallbackGetColor(value);
                tb->backSet = 1;
            }
            value = IT_ItemCallbackGetParam("ALLOW", 0);
            if (value != 0) {
                MemoryClear(tb->allow, sizeof(tb->allow));
                while ((i = *(unsigned char *)value++) != 0) {
                    for (n = 0; sAllowable[n] != 0 && sAllowable[n] != i; n++)
                        ;
                    tb->allow[i] = AsByte(sAllowable[n] != 0);
                }
            } else {
                MemoryFill(tb->allow, 1, sizeof(tb->allow));
            }
        }
        break;
    case kMsgOpen:
        e = IT_TextBoxCreateEdit(hwnd, dc, tb);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kMsgClose:
        if (tb->links != 0) {
            char *href;

            n = IndirectGetSize(tb->links) / sizeof(ITTextLink);
            for (i = 0; i < n; i++) {
                href = ((ITTextLink *)*tb->links)[i].href;
                if (href != 0)
                    MemoryDeallocate(href);
            }
            IndirectDeallocate(tb->links);
            tb->links = 0;
        }
        IT_TextBoxFreeBuffers(tb);
        if (tb->edit != 0)
            DestroyWindow(tb->edit);
        tb->edit = 0;
        if (tb->font != 0)
            DeleteObject(tb->font);
        tb->font = 0;
        break;
    case kMsgDispose:
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
        if (tb->edit == (void *)a)
            *(unsigned char *)b = 1;
        break;
    case kMsgClick:
        if (!DT_IsRectEmpty(&tb->bounds) &&
            DT_IsPointInRect(&((ITInputEvent *)a)->where, &tb->bounds))
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        if (tb->edit != 0) {
            void *windowDC;

            windowDC = GetDC(hwnd);
            SetViewportOrgEx(dc, tb->bounds.left, tb->bounds.top, &origin);
            if (tb->editable) {
                SendMessageA(tb->edit, dc == windowDC ? WM_PAINT : WM_PRINT, (unsigned int)dc,
                             PRF_CHECKVISIBLE | PRF_CLIENT | PRF_ERASEBKGND);
            } else {
                e = IT_TextBoxPaint(tb, dc);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
            SetViewportOrgEx(dc, origin.x, origin.y, &origin);
            ReleaseDC(hwnd, windowDC);
        }
        break;
    case 0x13:
    case 0x14:
        break;
    case kMsgWhat:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (!tb->editable) {
            e = _IT_TextCheck(hwnd, dc, tb, &pt, 1, (ITClick *)b);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else if (tb->enabled) {
            IT_ItemCallbackIsViewEnabled();
        }
        break;
    case kMsgKey:
        if (!tb->editable || !tb->visible || !tb->focused || !tb->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        {
            ITKeyEvent *event;
            unsigned char ok;

            event = (ITKeyEvent *)a;
            ok = 1;
            if (event->key == kKeyReturn) {
                ok = tb->single ? (unsigned char)0 : (unsigned char)1;
            } else if (event->key == kKeyEscape) {
                ok = 0;
            } else if ((event->key >= 0x96 && event->key <= 0x9f) || event->key == 0x93) {
                ok = 1;
            }
            if (ok)
                *(unsigned char *)b = 1;
        }
        break;
    case 0x17:
        break;
    case kMsgMouse:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (tb->enabled && !tb->editable && tb->links != 0) {
            MemoryClear(&click, sizeof(click));
            e = _IT_TextCheck(hwnd, dc, tb, &pt, 0, &click);
            if (e != 0) {
                err = e;
                goto done;
            }
            *(unsigned short *)b = click.followed ? IDC_HAND : 0;
        } else {
            *(unsigned short *)b = 0;
        }
        break;
    case 0x21:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x28:
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        tb->active = AsByte(msg == kMsgActivate);
        if (tb->edit != 0)
            update = 1;
        break;
    case kMsgItems:
        if (tb->editable && tb->enabled && IT_ItemCallbackIsViewEnabled()) {
            e = IndirectAppendData(b, &tb->edit, sizeof(tb->edit));
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        break;
    case kMsgFocus:
        if (a != 0) {
            if (!tb->editable) {
                err = kNotFocusable;
                goto done;
            }
            SetFocus(tb->edit);
            tb->focused = 1;
        } else {
            tb->focused = 0;
        }
        if (tb->edit != 0)
            update = 1;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = tb->bounds;
        break;
    case kMsgBounds:
        tb->bounds = *(ITRect *)a;
        CopyDT_2SysRect(&r, &tb->bounds);
        if (tb->edit != 0 &&
            !MoveWindow(tb->edit, r.left, r.top, r.right - r.left, r.bottom - r.top, 1)) {
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        break;
    case kMsgGetTitle:
        e = IndirectSetSize(b, 0);
        if (e != 0)
            err = e;
        break;
    case 0x43:
        break;
    case kMsgGetText:
        IndirectGetLock(b);
        if (tb->edit == 0)
            break;
        i = GetWindowTextLengthA(tb->edit);
        if (i == 0)
            break;
        n = IndirectGetSize(b);
        e = IndirectSetSize(b, i + n + 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(b, 1);
        if (GetWindowTextA(tb->edit, *(char **)b + n, i + 1) == 0)
            err = GetLastError();
        IndirectSetLock(b, 0);
        e = err;
        if (e != 0) {
            err = kOutOfMemory;
            goto done;
        }
        e = IndirectSetSize(b, n + i);
        if (e != 0)
            err = e;
        break;
    case kMsgSetText:
        IndirectGetLock((void **)a);
        if (tb->edit == 0)
            break;
        nul = 0;
        e = IndirectAppendData((void **)a, &nul, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock((void **)a, 1);
        if (!SetWindowTextA(tb->edit, *(char **)a)) {
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        IndirectSetLock((void **)a, 0);
        e = IndirectSetSize((void **)a, IndirectGetSize((void **)a) - 1);
        if (e != 0)
            err = e;
        break;
    case kMsgGetValue:
        *(long *)b = 0;
        break;
    case 0x47:
        break;
    case kMsgVisible:
        *(unsigned char *)b = tb->visible;
        break;
    case kMsgShow:
        tb->visible = AsByte(a != 0);
        if (!tb->visible && tb->focused) {
            e = IT_ItemCallbackGrabFocus(0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (tb->edit != 0)
            ShowWindow(tb->edit, tb->visible ? SW_SHOW : SW_HIDE);
        break;
    case kMsgEnabled:
        *(unsigned char *)b = tb->enabled;
        break;
    case kMsgEnable:
        tb->enabled = AsByte(a != 0);
        if (!tb->enabled && tb->focused) {
            e = IT_ItemCallbackGrabFocus(0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (tb->edit != 0)
            update = 1;
        break;
    case kMsgGetFore:
    case kMsgGetBack:
        *(ITColor *)b = *(msg == kMsgGetFore ? &tb->fore : &tb->back);
        break;
    case kMsgFore:
    case kMsgBack:
        if (msg == kMsgFore) {
            tb->fore = *(ITColor *)b;
            tb->foreSet = 1;
        } else {
            tb->back = *(ITColor *)b;
            tb->backSet = 1;
        }
        break;
    }
    if (update && tb->active && tb->editable && tb->focused && tb->visible && tb->enabled)
        IT_ItemCallbackIsViewEnabled();
done:
    return err;
}

static void *sMaskBlt = (void *)-1;              /* 0x0057bc1c, looked up and never used */

/* IT_TextBoxMakeBuffers (the offscreen text and mask bitmaps at the edit's client size and the screen's
   depth, rebuilt when either changed). STATE: MATCH 100%, exe 106 / obj 106, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00522a7f
long IT_TextBoxMakeBuffers(ITTextBox *tb)
{
    long err;
    void *windowDC;
    long depth;
    long width;
    long height;
    WinRect client;

    err = 0;
    windowDC = 0;
    windowDC = GetDC(tb->edit);
    if (windowDC == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    depth = GetDeviceCaps(windowDC, BITSPIXEL);
    GetClientRect(tb->edit, &client);
    width = client.right - client.left;
    height = client.bottom - client.top;
    if (depth != tb->depth || !EqualRect(&client, &tb->maskRect))
        IT_TextBoxFreeBuffers(tb);
    if (tb->textDC != 0)
        goto done;
    if ((tb->textDC = CreateCompatibleDC(windowDC)) == 0)
        goto failed;
    if ((tb->textBitmap = CreateBitmap(width, height, 1, depth, 0)) == 0)
        goto failed;
    tb->oldTextBitmap = SelectObject(tb->textDC, tb->textBitmap);
    if ((tb->maskDC = CreateCompatibleDC(windowDC)) == 0)
        goto failed;
    if ((tb->maskBitmap = CreateBitmap(width, height, 1, 1, 0)) == 0) {
failed:
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    tb->oldMaskBitmap = SelectObject(tb->maskDC, tb->maskBitmap);
    tb->maskRect = client;
    tb->depth = depth;
done:
    if (err != 0)
        IT_TextBoxFreeBuffers(tb);
    if (windowDC != 0)
        ReleaseDC(tb->edit, windowDC);
    return err;
}

/* IT_TextBoxPaint (paint the read-only rich edit transparently: its text formatted into the
   offscreen bitmap on white, a mask made from it, the background kept under the mask and the
   text drawn over it). STATE: MATCH 100%, exe 182 / obj 182, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00522bfc
long IT_TextBoxPaint(ITTextBox *tb, void *dc)
{
    long err;
    long e;
    void *windowDC;
    void *gdi;
    ULong width;
    long height;
    WinRect area;
    FORMATRANGE range;

    err = 0;
    windowDC = 0;
    if (sMaskBlt == (void *)-1) {
        gdi = LoadLibraryA("GDI32.DLL");
        if (gdi != 0)
            sMaskBlt = GetProcAddress(gdi, "MaskBlt");
    }
    e = IT_TextBoxMakeBuffers(tb);
    if (e != 0) {
        err = e;
        goto done;
    }
    area = tb->maskRect;
    width = area.right - area.left;
    height = area.bottom - area.top;
    windowDC = GetDC(tb->edit);
    if (windowDC == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    SetBkColor(tb->textDC, 0xffffff);
    SetTextColor(tb->textDC, 0);
    SetBkColor(tb->maskDC, 0xffffff);
    SetTextColor(tb->maskDC, 0);
    FillRect(tb->textDC, &area, GetStockObject(WHITE_BRUSH));
    FillRect(tb->maskDC, &area, GetStockObject(WHITE_BRUSH));
    range.hdc = tb->textDC;
    range.hdcTarget = windowDC;
    range.rc.left = 0;
    range.rc.top = 0;
    range.rc.right = width * kTwipsPerInch / (ULong)GetDeviceCaps(tb->textDC, LOGPIXELSX);
    range.rc.bottom = (ULong)(height * kTwipsPerInch) /
                      (ULong)GetDeviceCaps(tb->textDC, LOGPIXELSY);
    range.rcPage = range.rc;
    range.cpMin = 0;
    range.cpMax = -1;
    SendMessageA(tb->edit, EM_FORMATRANGE, 1, (long)&range);
    SendMessageA(tb->edit, EM_FORMATRANGE, 0, 0);
    SetBkColor(tb->textDC, 0xffffff);
    SetTextColor(tb->textDC, 0);
    SetBkColor(tb->maskDC, 0xffffff);
    SetTextColor(tb->maskDC, 0);
    BitBlt(tb->maskDC, 0, 0, width, height, tb->textDC, 0, 0, SRCCOPY);
    BitBlt(tb->textDC, 0, 0, width, height, tb->maskDC, 0, 0, ROP_DSNA);
    BitBlt(dc, 0, 0, width, height, tb->maskDC, 0, 0, SRCAND);
    BitBlt(dc, 0, 0, width, height, tb->textDC, 0, 0, SRCPAINT);
done:
    if (windowDC != 0)
        ReleaseDC(tb->edit, windowDC);
    return err;
}

/* IT_TextBoxFreeBuffers (release the offscreen mask and text bitmaps and their DCs). STATE:
   MATCH 100%, exe 50 / obj 50, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00522eae
void IT_TextBoxFreeBuffers(ITTextBox *tb)
{
    if (tb->oldMaskBitmap != 0)
        SelectObject(tb->maskDC, tb->oldMaskBitmap);
    if (tb->maskBitmap != 0)
        DeleteObject(tb->maskBitmap);
    if (tb->maskDC != 0)
        DeleteDC(tb->maskDC);
    tb->oldMaskBitmap = 0;
    tb->maskBitmap = 0;
    tb->maskDC = 0;
    if (tb->oldTextBitmap != 0)
        SelectObject(tb->textDC, tb->oldTextBitmap);
    if (tb->textBitmap != 0)
        DeleteObject(tb->textBitmap);
    if (tb->textDC != 0)
        DeleteDC(tb->textDC);
    tb->oldTextBitmap = 0;
    tb->textBitmap = 0;
    tb->textDC = 0;
    tb->depth = 0;
}

/* IT_TextBoxCreateEdit (build the edit: SINGLE, or a box too low for two lines of its text; a plain
   EDIT when EDITABLE, else a read-only RichEdit 2.0 (RichEdit 1.0 failing that) filled run
   by run from the element's text and P / BR / LI / DT / DD, each run formatted, A HREF runs
   recorded as links). STATE: MATCH 100%, exe 682 / obj 682, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00522f5f
long IT_TextBoxCreateEdit(void *hwnd, void *dc, ITTextBox *tb)
{
    long err;
    long e;
    unsigned char block;
    ITElement *node;
    unsigned char spareA;
    unsigned char spareB;
    unsigned char spareC;
    ULong alignStyle;
    ULong lineStyle;
    ULong count;
    void **text;
    char *lastHref;
    unsigned char afterSpace;
    char *piece;
    long length;
    long start;
    long end;
    void *edit;
    long editDC;
    void *windowDC;
    unsigned char c;
    ITTextLink link;
    char *face;
    long size;
    ULong style;
    ITColor color;
    char *href;
    ULong order;
    ITTextFormat format;
    char tag[256];

    err = 0;
    text = 0;
    lastHref = 0;
    order = 0;
    block = 0;
    afterSpace = 1;
    spareA = 1;
    spareB = 1;
    edit = 0;
    editDC = 0;
    spareC = 0;
    windowDC = 0;
    piece = IT_ItemCallbackGetParam("SINGLE", 0);
    if (piece != 0) {
        tb->single = IT_ItemCallbackGetFlag(piece);
    } else {
        do
            piece = IT_ItemCallbackGetNextRun(piece, 0, &size, 0, 0, 0, 0);
        while (piece != 0 && *piece == 0);
        if (piece == 0)
            IT_ItemCallbackGetFont(0, &size, 0, 0, 0);
        if (tb->bounds.bottom - tb->bounds.top <= size * 2)
            tb->single = 1;
    }
    if (!gControlsInitialized) {
        InitCommonControls();
        gControlsInitialized = 1;
    }
    alignStyle = 0;
    if (tb->aligned) {
        switch (tb->align) {
        case kAlignCenter:
            alignStyle = ES_CENTER;
            break;
        case kAlignRight:
            alignStyle = ES_RIGHT;
            break;
        case kAlignLeft:
            alignStyle = 0;
            break;
        }
    }
    lineStyle = alignStyle | WS_CHILD | ES_SAVESEL |
                (tb->single ? ES_AUTOHSCROLL : ES_MULTILINE | ES_AUTOVSCROLL);
    if (tb->editable) {
        if ((tb->edit = CreateWindowExA(
                 0, "EDIT", "",
                 alignStyle | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_SAVESEL |
                     (tb->single ? ES_AUTOHSCROLL : ES_MULTILINE | ES_AUTOVSCROLL),
                 tb->bounds.left, tb->bounds.top, tb->bounds.right - tb->bounds.left,
                 tb->bounds.bottom - tb->bounds.top + 2, hwnd, (void *)IT_FindFreeChildId(hwnd),
                 (void *)GetWindowLongA(hwnd, GWL_HINSTANCE), 0)) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
    } else {
        gTBRichEditModule = LoadLibraryA("RICHED20.DLL");
        if (gTBRichEditModule != 0) {
            if ((tb->edit = CreateWindowExA(
                     WS_EX_TRANSPARENT, "RichEdit20A", "", lineStyle, tb->bounds.left,
                     tb->bounds.top, tb->bounds.right - tb->bounds.left,
                     tb->bounds.bottom - tb->bounds.top + 2, hwnd, (void *)IT_FindFreeChildId(hwnd),
                     (void *)GetWindowLongA(hwnd, GWL_HINSTANCE), 0)) == 0) {
                err = GetLastError();
                err = kOutOfMemory;
                goto done;
            }
            EnableWindow(tb->edit, 0);
            GetLastError();
            gRichEdit20 = 1;
        } else {
            gTBRichEditModule = LoadLibraryA("RICHED32.DLL");
            if (gTBRichEditModule != 0) {
                if ((tb->edit = CreateWindowExA(
                         0, "RICHEDIT", "", lineStyle, tb->bounds.left, tb->bounds.top,
                         tb->bounds.right - tb->bounds.left, tb->bounds.bottom - tb->bounds.top + 2,
                         hwnd, (void *)IT_FindFreeChildId(hwnd),
                         (void *)GetWindowLongA(hwnd, GWL_HINSTANCE), 0)) == 0) {
                    err = GetLastError();
                    err = kOutOfMemory;
                    goto done;
                }
                EnableWindow(tb->edit, 0);
                GetLastError();
            } else {
                err = kOutOfMemory;
                goto done;
            }
        }
    }
    SendMessageA(tb->edit, EM_SETBKGNDCOLOR, 0, DT_2SysColor(&tb->back));
    windowDC = GetDC(tb->edit);
    if (windowDC == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    edit = tb->edit;
    editDC = (long)windowDC;
    tb->links = IndirectAllocate(0);
    if (tb->links == 0) {
        err = kOutOfMemory;
        goto done;
    }
    text = IndirectAllocate(0);
    if (text == 0) {
        err = kOutOfMemory;
        goto done;
    }
    node = 0;
    while ((node = IT_ItemCallbackGetNextNode(node, &face, &size, &style, &color, &href, &order)) != 0) {
        switch (node->tag) {
        case kTagText1:
            piece = 0;
            block = 0;
            break;
        case kTagText:
            piece = (char *)node->attributes;
            block = 0;
            break;
        case 0:
            piece = 0;
            block = 0;
            break;
        case kTagElement:
        case kTagEnd:
            StringCopySafe(tag, node->name, sizeof(tag));
            if (StringCompare(tag, "P") == 0) {
                piece = "\n\n";
                block = 1;
            } else if (StringCompare(tag, "BR") == 0) {
                piece = "\n";
                block = 1;
            } else if (StringCompare(tag, "LI") == 0) {
                piece = "\n\225 ";
                block = 1;
            } else if (StringCompare(tag, "DT") == 0) {
                piece = "\n";
                block = 1;
            } else if (StringCompare(tag, "DD") == 0) {
                piece = "\n";
                block = 1;
            } else {
                piece = 0;
            }
            break;
        default:
            piece = 0;
            block = 0;
            break;
        }
        if (piece != 0) {
            IndirectSetLock(text, 0);
            IndirectSetSize(text, 0);
            e = IT_AppendText(text, 0, piece, tb->convert,
                             ((style & kStylePre) != 0 || block) ? (unsigned char)0 : (unsigned char)1, afterSpace);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(text, 1);
            piece = *text;
        }
        if (piece == 0)
            continue;
        length = StringGetLength((unsigned char *)piece);
        if (length == 0)
            continue;
        start = IT_EditTextLength(tb->edit, gRichEdit20);
        link.start = start;
        SendMessageA(tb->edit, EM_SETSEL, start, start);
        SendMessageA(tb->edit, EM_REPLACESEL, 0, (long)piece);
        SendMessageA(tb->edit, EM_SETSEL, 0, 0);
        end = IT_EditTextLength(tb->edit, gRichEdit20);
        if (!tb->editable) {
            if ((style & kStyleLink) != 0 && href != 0) {
                if (href == lastHref) {
                    count = IndirectGetSize(tb->links) / sizeof(ITTextLink);
                    ((ITTextLink *)*tb->links)[count - 1].length += end - start;
                } else {
                    if ((link.href = MemoryAllocate(StringGetLength((unsigned char *)href) + 1)) == 0) {
                        err = kOutOfMemory;
                        goto done;
                    }
                    StringCopy(link.href, href);
                    link.length = end - start;
                    err = IndirectAppendData(tb->links, &link, sizeof(link));
                    if (err != 0) {
                        MemoryDeallocate(link.href);
                        goto done;
                    }
                }
            }
            e = _IT_TextFormat(edit, (void *)editDC, tb, &format, face, size, style, &color,
                             order);
            if (e != 0) {
                err = e;
                goto done;
            }
            count = IT_EditTextLength(tb->edit, gRichEdit20);
            SendMessageA(tb->edit, EM_SETSEL, start, end);
            SendMessageA(tb->edit, EM_SETCHARFORMAT, SCF_SELECTION, (long)&format.cf);
            SendMessageA(tb->edit, EM_SETSEL, 0, 0);
        }
        lastHref = href;
        // COMPILE-WHY: only this pointee cast gives the exe's [eax+ebx-1]; 15 other spellings keep [ebx+eax-1]
        c = ((unsigned char *)piece)[length - 1];
        afterSpace = (c == '\t' || c == '\n' || c == '\r' || c == ' ') ? (unsigned char)1
                                                                        : (unsigned char)0;
    }
    if (tb->editable) {
        piece = 0;
        do
            piece = IT_ItemCallbackGetNextRun(piece, &face, &size, &style, &color, &href, &order);
        while (piece != 0 && *piece == 0);
        if (piece == 0)
            IT_ItemCallbackGetFont(&face, &size, &style, &color, &href);
        style &= ~kStyleLink;
        e = _IT_TextFormat(edit, (void *)editDC, tb, &format, face, size, style, &color, order);
        if (e != 0) {
            err = e;
            goto done;
        }
        if ((tb->font = CreateFontIndirectA(&format.lf)) == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        SendMessageA(tb->edit, WM_SETFONT, (unsigned int)tb->font, 0);
        SendMessageA(tb->edit, EM_SETSEL, 0, IT_EditTextLength(tb->edit, gRichEdit20));
    } else {
        count = IT_EditTextLength(tb->edit, gRichEdit20);
        SendMessageA(tb->edit, EM_SETSEL, count, count);
        SendMessageA(tb->edit, EM_REPLACESEL, 0, (long)"\r\n");
        SendMessageA(tb->edit, EM_SETSEL, 0, 0);
        SendMessageA(tb->edit, EM_SETREADONLY, 1, 0);
    }
    ShowWindow(tb->edit, SW_SHOW);
done:
    if (text != 0)
        IndirectDeallocate(text);
    if (windowDC != 0)
        ReleaseDC(tb->edit, windowDC);
    return err;
}

/* _IT_TextFormat (a run's format: face (a FACE list through 0x0050f09f, else "system"), size in
   points (12 when none) and twips, bold / italic / underline, fixed pitch for TT and PRE, its
   FGCOLOR or the box's, a link's colour and underline). STATE: MATCH 100%, exe 154 / obj 154,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052386d
long _IT_TextFormat(void *hwnd, void *dc, ITTextBox *tb, ITTextFormat *format, const char *face,
                  long size, ULong style, const ITColor *color, ULong order)
{
    CHARFORMATA *cf;
    LOGFONTA *lf;
    long points;
    char family[256];
    unsigned char flag;

    points = 0;
    cf = (CHARFORMATA *)format;
    lf = &format->lf;
    MemoryClear(cf, sizeof(CHARFORMATA));
    cf->cbSize = sizeof(CHARFORMATA);
    cf->dwMask = CFM_SIZE | CFM_COLOR | CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
    cf->dwEffects = style & (kStyleBold | kStyleItalic | kStyleUnderline);
    if (size != 0)
        points = size;
    else
        points = kDefaultPoints;
    cf->yHeight = points * kTwipsPerInch / GetDeviceCaps(dc, LOGPIXELSY);
    cf->yOffset = 0;
    cf->crTextColor = DT_2SysColor((style & kStyleColor) != 0 ? color : &tb->fore);
    cf->bCharSet = 0;
    cf->bPitchAndFamily = 0;
    StringCopy(cf->szFaceName, "system");
    MemoryClear(lf, sizeof(LOGFONTA));
    lf->lfHeight = -size;
    lf->lfWidth = 0;
    lf->lfEscapement = 0;
    lf->lfOrientation = 0;
    lf->lfWeight = (style & kStyleBold) != 0 ? FW_BOLD : FW_NORMAL;
    if (style & kStyleItalic)
        flag = 1;
    else
        flag = 0;
    lf->lfItalic = flag;
    if (style & kStyleUnderline)
        flag = 1;
    else
        flag = 0;
    lf->lfUnderline = flag;
    lf->lfStrikeOut = 0;
    lf->lfCharSet = 0;
    lf->lfOutPrecision = 0;
    lf->lfClipPrecision = 0;
    lf->lfQuality = 0;
    lf->lfPitchAndFamily = 0;
    lf->lfFaceName[0] = 0;
    if (face != 0 && _DT_GetFontByNameList(dc, face, family) != 0) {
        cf->dwMask |= CFM_FACE;
        StringCopySafe(cf->szFaceName, family, sizeof(cf->szFaceName));
        StringCopySafe(lf->lfFaceName, family, sizeof(lf->lfFaceName));
    }
    if ((style & (kStyleTeletype | kStylePre)) != 0) {
        cf->bPitchAndFamily = FIXED_PITCH;
        lf->lfPitchAndFamily = FIXED_PITCH;
    }
    if ((style & kStyleLink) != 0) {
        cf->crTextColor = DT_2SysColor(&tb->link);
        cf->dwEffects |= CFE_UNDERLINE;
        lf->lfUnderline = 1;
    }
    return 0;
}

/* IT_AppendText (append `text' to the handle: collapsed, runs of white space one space and none at
   the start after one; else tabs four spaces and line ends CR LF; NUL-terminated). STATE: MATCH
   100%, exe 106 / obj 106, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00523a35
long IT_AppendText(void **out, unsigned char upper, const char *text, unsigned char convert,
                  unsigned char collapse, unsigned char afterSpace)
{
    long err;
    long e;
    long c;
    unsigned char space;
    const char *here;
    const char *piece;
    ULong length;
    ULong n;

    err = 0;
    do {
        here = text;
        piece = text;
        space = 0;
        {
        char one[2] = { 0, 0 };

        c = *text++;
        length = 1;
        if (c == 0)
            break;
        switch (c) {
        case '\t':
            if (collapse)
                space = 1;
            else
                piece = "    ";
            break;
        case '\r':
            if (*text == '\n')
                continue;
        case '\n':
            if (collapse)
                space = 1;
            else
                piece = "\r\n";
            break;
        case ' ':
            if (collapse)
                space = 1;
        default:
            if (convert && !upper) {
                one[0] = UnicodePoint2Char8(c);
                piece = one;
            } else if (!convert && upper) {
                one[0] = c;
                piece = one;
            }
            break;
        }
        if (space) {
            n = IndirectGetSize(out);
            if (n != 0) {
                if ((c = ((char *)*out)[(long)n - 1]) == ' ')
                    continue;
            } else if (afterSpace) {
                continue;
            }
            piece = " ";
        }
        if (piece != here)
            length = StringGetLength((const unsigned char *)piece);
        e = IndirectAppendData(out, piece, length);
        if (e != 0) {
            err = e;
            goto done;
        }
        }
    } while (c != 0);
    e = IndirectAppendData(out, "", 1);
    if (e != 0)
        err = e;
done:
    return err;
}

/* _IT_TextCheck (the link under a point of the read-only rich edit: its outline as up to
   three rectangles over the lines it spans; without `track' only whether there is one, with
   it the outline inverted while the button is down and inside (events 12..15); the HREF into
   the click when it is released over the link). STATE: MATCH 100%, exe 523 / obj 523,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00523b77
long _IT_TextCheck(void *hwnd, void *dc, ITTextBox *tb, ITPoint *pt, unsigned char track,
                  ITClick *click)
{
    long err;
    long e;
    long windowDC;
    ULong count;
    ULong i;
    ULong index;
    ULong start;
    ULong end;
    ULong firstLine;
    ULong lastLine;
    ULong lineStart;
    ULong next;
    ULong lineEnd;
    long lastHeight;
    unsigned char dead;
    unsigned char lit;
    unsigned char inside;
    long near;
    long firstHeight;
    WinRect client;
    POINT pos;
    ITPoint a;
    ITPoint b;
    ITPoint p1;
    ITPoint p2;
    ITRect r1;
    ITRect r2;
    ITRect r3;
    WinRect client2;
    ITRect box;
    ITInputEvent event;
    ULong selStart;
    ULong selEnd;

    err = 0;
    windowDC = 0;
    if (!ClientToScreen(hwnd, (POINT *)pt) || !ScreenToClient(tb->edit, (POINT *)pt))
        goto failed;
    e = IT_TextBoxMakeBuffers(tb);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((windowDC = (long)GetDC(tb->edit)) == 0) {
failed:
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }
    GetClientRect(tb->edit, &client);
    SendMessageA(tb->edit, EM_SETTARGETDEVICE, (unsigned int)windowDC,
                 (client.right - client.left) * kTwipsPerInch /
                 GetDeviceCaps(tb->textDC, LOGPIXELSX));
    pos.x = pt->h;
    pos.y = pt->v;
    index = SendMessageA(tb->edit, EM_CHARFROMPOS, 0, (long)&pos);
    count = IndirectGetSize(tb->links) / sizeof(ITTextLink);
    for (i = 0; i < count; i++) {
        dead = 0;
        start = ((ITTextLink *)*tb->links)[i].start;
        end = start + ((ITTextLink *)*tb->links)[i].length;
        if (index >= end)
            continue;
        if (index < start)
            break;
        DT_SetRect(&r1, 0, 0, 0, 0);
        DT_SetRect(&r2, 0, 0, 0, 0);
        DT_SetRect(&r3, 0, 0, 0, 0);
        dc = (void *)windowDC;
        firstLine = SendMessageA(tb->edit, EM_EXLINEFROMCHAR, 0, start);
        lastLine = SendMessageA(tb->edit, EM_EXLINEFROMCHAR, 0, end);
        a = IT_EditCharPos(tb->edit, start, gRichEdit20);
        b = IT_EditCharPos(tb->edit, end, gRichEdit20);
        /* BUG: EM_LINEINDEX where EM_GETSEL was meant, so selStart / selEnd stay uninitialised */
        SendMessageA(tb->edit, EM_LINEINDEX, (unsigned int)&selStart, (long)&selEnd);
        lineStart = SendMessageA(tb->edit, EM_LINEINDEX, lastLine, 0);
        if (lastLine < SendMessageA(tb->edit, EM_GETLINECOUNT, 0, 0))
            lineEnd = SendMessageA(tb->edit, EM_LINEINDEX, lastLine + 1, 0);
        else
            lineEnd = IT_EditTextLength(tb->edit, gRichEdit20);
        next = lineEnd;
        a = IT_EditCharPos(tb->edit, lineStart, gRichEdit20);
        b = IT_EditCharPos(tb->edit, next, gRichEdit20);
        lastHeight = b.v - a.v;
        firstHeight = lastHeight;
        if (firstLine < lastLine) {
            lineStart = SendMessageA(tb->edit, EM_LINEINDEX, firstLine, 0);
            next = SendMessageA(tb->edit, EM_LINEINDEX, firstLine + 1, 0);
            a = IT_EditCharPos(tb->edit, lineStart, gRichEdit20);
            b = IT_EditCharPos(tb->edit, next, gRichEdit20);
            firstHeight = b.v - a.v;
        }
        SendMessageA(tb->edit, EM_SETSEL, selStart, selEnd);
        a = IT_EditCharPos(tb->edit, start, gRichEdit20);
        b = IT_EditCharPos(tb->edit, end, gRichEdit20);
        GetClientRect(tb->edit, &client2);
        CopySys2DT_Rect(&box, (ITRect *)&client2);
        CopySys2DT_Point(&p1, &a);
        CopySys2DT_Point(&p2, &b);
        if (p1.v < p2.v) {
            DT_SetRect(&r1, p1.h - 2, p1.v, box.right, p1.v + firstHeight);
            DT_SetRect(&r2, box.left - 2, p1.v + firstHeight, box.right, p2.v);
            DT_SetRect(&r3, box.left - 2, p2.v, p2.h, p2.v + lastHeight);
        } else {
            DT_SetRect(&r1, 0, 0, 0, 0);
            DT_SetRect(&r2, p1.h - 2, p1.v, p2.h, p1.v + firstHeight);
            DT_SetRect(&r3, 0, 0, 0, 0);
        }
        if (track) {
            ITPoint offset = { 0, 0 };

            MapWindowPoints(hwnd, tb->edit, (POINT *)&offset, 1);
            IT_ItemCallbackSoundEvent(kEventTrack);
            _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
            lit = inside = 1;
            while (InputReadMouseState(hwnd, &event) && (event.modifiers & kModButtonDown) != 0) {
                event.where.h += offset.h;
                event.where.v += offset.v;
                inside = (near = DT_IsPointInRect(&event.where, &r1) || DT_IsPointInRect(&event.where, &r2))
                         || DT_IsPointInRect(&event.where, &r3);
                if (inside != lit) {
                    IT_ItemCallbackSoundEvent(inside ? kEventIn : kEventOut);
                    _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
                }
                lit = inside;
            }
            IT_ItemCallbackSoundEvent(kEventRelease);
            if (inside) {
                _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
                StringCopySafe(click->link, ((ITTextLink *)*tb->links)[i].href, kLinkMax);
                click->followed = 1;
            }
        } else {
            click->followed = 1;
            StringCopySafe(click->link, ((ITTextLink *)*tb->links)[i].href, kLinkMax);
        }
        break;
    }
done:
    GetClientRect(tb->edit, &client);
    SendMessageA(tb->edit, EM_SETTARGETDEVICE, (unsigned int)tb->textDC,
                 (client.right - client.left) * kTwipsPerInch /
                 GetDeviceCaps(tb->textDC, LOGPIXELSX));
    if (windowDC != 0)
        ReleaseDC(tb->edit, (void *)windowDC);
    return err;
}

/* _IT_HiliteLink (invert a link's outline). STATE: MATCH 100%, exe 33 / obj 33, byte-identical yes.
   OPEN: the window and the text box are never read */
// FUNCTION: EVNOVA 0x005242e2
void _IT_HiliteLink(void *hwnd, void *dc, ITTextBox *tb, const ITRect *r1, const ITRect *r2,
                  const ITRect *r3)
{
    ITRect r;

    InvertRect(dc, (WinRect *)CopyDT_2SysRect(&r, r1));
    InvertRect(dc, (WinRect *)CopyDT_2SysRect(&r, r2));
    InvertRect(dc, (WinRect *)CopyDT_2SysRect(&r, r3));
}
