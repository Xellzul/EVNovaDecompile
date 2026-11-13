/* ittextboxmac.c -- 1.0.10: the interface tool's TEXTBOX control on Mac, on TextEdit where win makes a rich edit window, and on ATSUI for Carbon's read-only text. Notes: Research/notes/maclibs.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "ittoolkit.h"
#include <TextEdit.h>
#include <Scrap.h>
#include <LowMem.h>
#include <Fonts.h>
#if TARGET_API_MAC_CARBON
#include <ATSUnicode.h>
#endif

extern void IT_ItemCallbackGetFont(char **face, int32_t *size, ULong *style, ITColor *color,
                         char **href);                                   /* interfacetool.c */
extern char *IT_ItemCallbackGetNextRun(const char *after, char **face, int32_t *size, ULong *style,
                          ITColor *color, char **href, ULong *order);    /* interfacetool.c */
extern ITElement *IT_ItemCallbackGetNextNode(ITElement *after, char **face, int32_t *size, ULong *style,
                               ITColor *color, char **href, ULong *order);  /* interfacetool.c */
extern ITColor DT_GetColorARGB(ULong argb);
extern RGBColor *CopyDT_2SysColor(RGBColor *ref, const ITColor *color);
extern char *_DT_GetFontByNameList(CGrafPtr dc, const char *faces, char *out);
extern unsigned char InputReadMouseState(void *hwnd, ITInputEvent *event);          /* itinputmac.c */
extern unsigned char gIT_HasInternetConfig;                              /* atcursormac.c */
extern CQDProcs gIT_StdCProcs;
#if TARGET_API_MAC_CARBON
extern OSStatus ATSU_FontFromFamily(FMFontFamily family, FMFontStyle style, ATSUFontID *font, FMFontStyle *fontStyle);
extern OSStatus ATSU_StyleSetByte(ATSUStyle style, unsigned char value, ATSUAttributeTag tag);
extern OSStatus ATSU_StyleSetFixed(ATSUStyle style, Fixed value, ATSUAttributeTag tag);
extern OSStatus ATSU_StyleSetFont(ATSUStyle style, ATSUFontID value, ATSUAttributeTag tag);
extern OSStatus ATSU_StyleSetFace(ATSUStyle style, short face);
extern OSStatus ATSU_StyleSetColor(ATSUStyle style, RGBColor color);
extern unsigned char UnicodeAdvancePtr(const unsigned short **p, ULong *c);
extern OSStatus ATSU_DrawTextBox(ATSUTextLayout layout, const Rect *box, Handle lines);
extern OSStatus ATSU_LayoutGetControl(ATSUTextLayout layout, ATSUAttributeTag tag, ByteCount size, ATSUAttributeValuePtr value);
extern void **IndirectAllocateU2C(void **handle);
extern void **IndirectAllocateC2U(void **handle, unsigned char tag);
extern int32_t IndirectAppend(void **to, void **from);
extern unsigned char IndirectIsNormalizedUnicode(void **handle);
extern int32_t SFix32_To_SInt32(Fixed value);
extern Fixed SInt32_To_SFix32(int32_t value);
extern Rect DT_2SysRect(const ITRect *r);
extern ITRect Sys2DT_Rect(const Rect *r);

typedef struct ATSULine {
    UniCharArrayOffset start;
    UniCharArrayOffset end;
    ATSUTextMeasurement ascent;
    ATSUTextMeasurement descent;
    ATSUTextMeasurement offset;
} ATSULine;
extern OSStatus ATSU_LayoutSetFixed(ATSUTextLayout layout, Fixed value, ATSUAttributeTag tag);
extern OSStatus ATSU_LayoutSetOptions(ATSUTextLayout layout, ATSLineLayoutOptions value);
extern unsigned char _PlatformIsThreaded(void);

typedef struct ITTextStyleNode {
    struct ITTextStyleNode *next;
    ULong order;
    ATSUStyle style;
} ITTextStyleNode;
#endif

/* a followable run of the text: characters [start, start + length) and where it goes */
typedef struct ITTextLink {
    ULong start;
    ULong length;
    char *href;
} ITTextLink;

#pragma options align=mac68k
/* the control's data handle */
typedef struct ITTextBox {
    ITColor fore;                                /* +0x000 FGCOLOR */
    ITColor back;                                /* +0x004 BGCOLOR */
    ITColor link;                                /* +0x008 */
    unsigned char foreSet;                       /* +0x00c */
    unsigned char backSet;                       /* +0x00d */
    unsigned char editable;                      /* +0x00e EDITABLE */
    unsigned char convert;                       /* +0x00f never set */
    unsigned char single;                        /* +0x010 SINGLE, or too low for two lines */
    unsigned char pad11;
    int32_t align;                                  /* +0x012 ALIGN */
    unsigned char aligned;                       /* +0x016 */
    unsigned char allow[256];                    /* +0x017 ALLOW */
    unsigned char pad117;
    ITRect bounds;                               /* +0x118 */
    unsigned char focused;                       /* +0x128 */
    unsigned char active;                        /* +0x129 */
    unsigned char visible;                       /* +0x12a */
    unsigned char enabled;                       /* +0x12b */
    void **links;                                /* +0x12c ITTextLink */
    TEHandle edit;                               /* +0x130 */
    unsigned char clicking;                      /* +0x134 */
    unsigned char pad135;
#if TARGET_API_MAC_CARBON
    ATSUTextLayout layout;                       /* +0x136 */
    void **text;                                 /* +0x13a */
    void *textPointer;                           /* +0x13e */
    void **lines;                                /* +0x142 */
    int32_t rotation;                               /* +0x146 */
    ITTextStyleNode *styles;                     /* +0x14a */
#endif
} ITTextBox;
#pragma options align=reset

#define kNotFocusable       -7002
#define kLinkColour         0xff0000eeUL
#define kDefaultPoints      12
#define kLinkMax            0x400
#define kModButtonDown      0x10000000
#define kEventTrack         12
#define kEventOut           13
#define kEventIn            14
#define kEventRelease       15
#define kAlignCenter        0
#define kBoldBit            1
#define kUnderlineBit       4
#define kMsgTrack           0x15
#define kShiftBit           2
#define kKeyReturn          0x91
#define kBackspace          8
#define kBeepTicks          7
#define kCursorIBeam        1
#define kCursorHand         5
#define kOffRight           32000
#define kHiliteBit          0x7f
#define kBoldSuffix         "SYSTEM"
#define kBoldName           "LUCIDA GRANDE"

static char sAllowable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz `1234567890~!@#$%^&*()-=_+[]\\{}|;':\",./<>?";

#if !TARGET_API_MAC_CARBON
static int32_t _IT_TextFormat(void *hwnd, void *dc, ITTextBox *tb, TextStyle *format, const char *face,
                  int32_t size, ULong style, const ITColor *color, ULong order);
#else
static int32_t _IT_TextFormat(void *hwnd, void *dc, ITTextBox *tb, void *ref, const char *face, int32_t size, ULong style, const ITColor *color, ULong order);
#endif
static int32_t IT_AppendText(void **out, unsigned char upper, const char *text, unsigned char convert,
                  unsigned char collapse, unsigned char afterSpace);
static int32_t _IT_TextCheck(void *hwnd, void *dc, ITTextBox *tb, ITPoint *pt, unsigned char track,
                  ITClick *click);
static void _IT_HiliteLink(void *hwnd, void *dc, ITTextBox *tb, const ITRect *r1, const ITRect *r2,
                  const ITRect *r3);
static int32_t IT_TextBoxCreateEdit(void *hwnd, void *dc, ITTextBox *tb);
#if TARGET_API_MAC_CARBON
#define kLineOptions        0x01000000
#define kFlushStart         0
#define kFlushCenter        0x20000000
#define kFlushEnd           0x40000000
#define kFlushTag           5
#endif

#if TARGET_API_MAC_CARBON
#define kHasText(t) ((t)->layout != 0 || (t)->edit != 0)
#define KEEP_NO_LAYOUT && tb->layout == 0
#define TEXT_EMPTY(p) ((tb->convert && *(const unsigned short *)(p) == 0) || (!tb->convert && *(p) == 0))
#define kClearScrap()
#define kLayoutFailed -8400
#else
#define kHasText(t) ((t)->edit != 0)
#define KEEP_NO_LAYOUT
#define TEXT_EMPTY(p) (*(p) == 0)
#define kClearScrap ZeroScrap
#endif
int32_t IT_TextHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
#if TARGET_API_MAC_CARBON
    ITTextBox *tb;
    int32_t err;
    int32_t e;
    void **converted;
    unsigned char wide;
    unsigned char update;
    char *value;
    ULong n;
    void **text;
    unsigned char ok;
    void *owner;
    void *procs;
    ITClick click;
    char allowText[0x100];
#else
    ITTextBox *tb;
    int32_t err;
    int32_t e;
    unsigned char update;
    char *value;
    ULong n;
    void **text;
    unsigned char ok;
    void *owner;
    void *procs;
    ITClick click;
#endif
    TextStyle format;
    Rect r;
    RGBColor savedFore;
    RGBColor savedBack;
    RGBColor rgb;
    Point pt;
    ULong i;
    CGrafPtr savedPort;
    GDHandle savedDevice;
#if TARGET_API_MAC_CARBON
    ScrapRef scrap;
#endif
    char *face;
    int32_t size;
    ULong style;
    ITColor color;
    ULong order;
    ITColor link;
    ITColor fore;
    ITColor back;

    tb = *data;
#if TARGET_API_MAC_CARBON
    converted = 0;
#endif
    err = 0;
    update = 0;
#if TARGET_API_MAC_CARBON
    wide = element->wide;
#endif
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
#if TARGET_API_MAC_CARBON
    IndirectGetSize(data);
#endif
    GetGWorld(&savedPort, &savedDevice);
    if (dc != 0)
        SetGWorld(dc, GetMainDevice());
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
        link = DT_GetColorARGB(kLinkColour);
        tb->link = link;
        tb->enabled = 1;
        tb->visible = 1;
        tb->active = 1;
        tb->focused = 0;
#if TARGET_API_MAC_CARBON
        tb->convert = wide;
#else
        tb->convert = 0;
#endif
        tb->aligned = IT_ItemCallbackGetAlign(&tb->align);
        value = IT_ItemCallbackGetParam("EDITABLE", 0);
        if (value != 0)
            tb->editable = IT_ItemCallbackGetFlag(value);
        value = IT_ItemCallbackGetParam("FGCOLOR", 1);
        if (value != 0) {
            fore = IT_ItemCallbackGetColor(value);
            tb->fore = fore;
            tb->foreSet = 1;
        }
        value = IT_ItemCallbackGetParam("BGCOLOR", 1);
        if (value != 0) {
            back = IT_ItemCallbackGetColor(value);
            tb->back = back;
            tb->backSet = 1;
        }
        value = IT_ItemCallbackGetParam("ALLOW", 0);
        if (value != 0) {
#if TARGET_API_MAC_CARBON
            if (wide)
                value = (char *)U2CStringCopySafe((unsigned char *)allowText, (const unsigned short *)value, 0x100, -1);
#endif
            MemoryClear(tb->allow, sizeof(tb->allow));
            while ((i = *(unsigned char *)value++) != 0) {
                for (n = 0; sAllowable[n] != 0 && sAllowable[n] != i; n++)
                    ;
                tb->allow[i] = AsByte(sAllowable[n] != 0);
            }
        } else {
            MemoryFill(tb->allow, 1, sizeof(tb->allow));
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
            n = IndirectGetSize(tb->links) / sizeof(ITTextLink);
            for (i = 0; i < n; i++) {
                if (((ITTextLink *)*tb->links)[i].href != 0)
                    MemoryDeallocate(((ITTextLink *)*tb->links)[i].href);
            }
            IndirectDeallocate(tb->links);
            tb->links = 0;
        }
#if !TARGET_API_MAC_CARBON
        if (tb->edit != 0)
            TEDispose(tb->edit);
        tb->edit = 0;
#else
        if (tb->layout != 0)
            ATSUDisposeTextLayout(tb->layout);
        tb->layout = 0;
        if (tb->text != 0)
            IndirectDeallocate(tb->text);
        tb->text = 0;
        tb->textPointer = 0;
        if (tb->lines != 0)
            IndirectDeallocate(tb->lines);
        tb->lines = 0;
        {
            ITTextStyleNode *node;

            while ((node = tb->styles) != 0) {
                tb->styles = node->next;
                if (node->style != 0) {
                    e = ATSUDisposeStyle(node->style);
                    if (e != 0)
                        err = e;
                }
                MemoryDeallocate(node);
            }
        }
        if (tb->edit != 0)
            TEDispose(tb->edit);
        tb->edit = 0;
#endif
        break;
    case kMsgDispose:
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0 && (ULong)&tb->layout == a) {
            *(unsigned char *)b = 1;
            break;
        }
#endif
        if (tb->edit != 0 && tb->edit == (TEHandle)a)
            *(unsigned char *)b = 1;
        break;
    case kMsgClick:
        if (!DT_IsRectEmpty(&tb->bounds) && DT_IsPointInRect(&((ITInputEvent *)a)->where, &tb->bounds))
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
#if TARGET_API_MAC_CARBON
        if (tb->layout == 0 && tb->edit == 0)
            break;
#else
        if (tb->edit == 0)
            break;
#endif
        GetForeColor(&savedFore);
        GetBackColor(&savedBack);
        if (tb->foreSet)
            RGBForeColor(CopyDT_2SysColor(&rgb, &tb->fore));
        if (tb->backSet)
            RGBBackColor(CopyDT_2SysColor(&rgb, &tb->back));
        if (tb->editable) {
            SetRect(&r, tb->bounds.left - 2, tb->bounds.top - 2, tb->bounds.right + 2, tb->bounds.bottom + 1);
            EraseRect(&r);
            FrameRect(&r);
        }
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            e = ATSU_DrawTextBox(tb->layout, CopyDT_2SysRect(&r, &tb->bounds), (Handle)tb->lines);
            if (e != 0)
                err = e;
            ATSU_LayoutGetControl(tb->layout, kATSULineRotationTag, sizeof(tb->rotation), &tb->rotation);
        } else {
            owner = (*tb->edit)->inPort;
            (*tb->edit)->inPort = dc;
            procs = GetPortGrafProcs(dc);
            SetPortGrafProcs(dc, &gIT_StdCProcs);
            CopyDT_2SysRect(&r, &tb->bounds);
            TEUpdate(&r, tb->edit);
            SetPortGrafProcs(dc, procs);
            (*tb->edit)->inPort = owner;
        }
#else
        owner = (*tb->edit)->inPort;
        (*tb->edit)->inPort = dc;
        procs = ((CGrafPtr)dc)->grafProcs;
        ((CGrafPtr)dc)->grafProcs = &gIT_StdCProcs;
        CopyDT_2SysRect(&r, &tb->bounds);
        TEUpdate(&r, tb->edit);
        ((CGrafPtr)dc)->grafProcs = procs;
        (*tb->edit)->inPort = owner;
#endif
        RGBForeColor(&savedFore);
        RGBBackColor(&savedBack);
        break;
    case kMsgIdle:
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0 && tb->editable && tb->focused)
            break;
#endif
        if (tb->edit != 0 && tb->editable && tb->focused)
            TEIdle(tb->edit);
        break;
    case kMsgTrack:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (!tb->editable) {
            e = _IT_TextCheck(hwnd, dc, tb, (ITPoint *)&pt, 1, b);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        }
        if (!tb->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        GetForeColor(&savedFore);
        GetBackColor(&savedBack);
        if (tb->foreSet)
            RGBForeColor(CopyDT_2SysColor(&rgb, &tb->fore));
        if (tb->backSet)
            RGBBackColor(CopyDT_2SysColor(&rgb, &tb->back));
        tb->clicking = 1;
        err = IT_ItemCallbackGrabFocus(tb->edit);
        tb->clicking = 0;
        if (err != 0)
            goto done;
#if TARGET_API_MAC_CARBON
        if (tb->layout == 0)
#endif
        TEClick(pt, ((ITInputEvent *)a)->modifiers & kShiftBit, tb->edit);
        RGBForeColor(&savedFore);
        RGBBackColor(&savedBack);
        break;
    case kMsgKey:
        if (!tb->editable || !tb->visible || !tb->focused || !tb->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        if (((ITKeyEvent *)a)->key == kKeyReturn) {
            ok = AsByte(!tb->single);
        } else if ((((ITKeyEvent *)a)->key >= 0x96 && ((ITKeyEvent *)a)->key <= 0x9f) || ((ITKeyEvent *)a)->key == 0x93) {
            ok = 1;
        } else {
            ok = tb->allow[((ITKeyEvent *)a)->ch];
        }
#if TARGET_API_MAC_CARBON
        if ((tb->layout == 0 && tb->edit == 0) || !ok)
            break;
#else
        if (tb->edit == 0 || !ok)
            break;
#endif
        GetForeColor(&savedFore);
        GetBackColor(&savedBack);
        if (tb->foreSet)
            RGBForeColor(CopyDT_2SysColor(&rgb, &tb->fore));
        if (tb->backSet)
            RGBBackColor(CopyDT_2SysColor(&rgb, &tb->back));
#if TARGET_API_MAC_CARBON
        if (tb->layout == 0)
#endif
        TEKey(*(char *)&((ITKeyEvent *)a)->ch, tb->edit);
        *(unsigned char *)b = 1;
        RGBForeColor(&savedFore);
        RGBBackColor(&savedBack);
        break;
    case kMsgMouse:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (tb->enabled && tb->editable) {
            *(short *)b = kCursorIBeam;
        } else if (tb->enabled && !tb->editable && tb->links != 0) {
            MemoryClear(&click, sizeof(click));
            e = _IT_TextCheck(hwnd, dc, tb, (ITPoint *)&pt, 0, &click);
            if (e != 0) {
                err = e;
                goto done;
            }
            *(short *)b = click.followed ? kCursorHand : 0;
        } else {
            *(short *)b = 0;
        }
        break;
    case 0x21:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x28:
        if (!tb->editable)
            break;
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0)
            break;
#endif
        if (tb->edit == 0)
            break;
        switch (msg) {
        case 0x21:
        case 0x22:
        case 0x27:
            break;
        case 0x23:
            TECut(tb->edit);
            kClearScrap();
            TEToScrap();
            *(unsigned char *)b = 1;
            break;
        case 0x24:
            TECopy(tb->edit);
            kClearScrap();
            TEToScrap();
            *(unsigned char *)b = 1;
            break;
        case 0x25:
#if TARGET_API_MAC_CARBON
            scrap = 0;
#endif
            if ((text = IndirectAllocate(0)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
#if TARGET_API_MAC_CARBON
            if (GetCurrentScrap(&scrap) != 0) {
                err = kOutOfMemory;
                goto done;
            }
            if (GetScrapFlavorSize(scrap, 'TEXT', (Size *)&i) != 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = IndirectSetSize(text, i);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(text, 1);
            if (GetScrapFlavorData(scrap, 'TEXT', (Size *)&i, *text) != 0) {
                err = kOutOfMemory;
                goto done;
            }
#else
            if (GetScrap((Handle)text, 'TEXT', (int32_t *)&i) < 0) {
                err = kOutOfMemory;
                goto done;
            }
#endif
            for (i = 0; i < IndirectGetSize(text); i++) {
                n = (*(unsigned char **)text)[i];
                if (!tb->allow[n])
                    break;
                if (tb->single && (n == '\r' || n == '\n'))
                    break;
            }
            if (i < IndirectGetSize(text)) {
                SysBeep(kBeepTicks);
            } else {
                TEFromScrap();
                TEPaste(tb->edit);
            }
            IndirectDeallocate(text);
            *(unsigned char *)b = 1;
            break;
        case 0x26:
            TEDelete(tb->edit);
            *(unsigned char *)b = 1;
            break;
        case 0x28:
            TESetSelect(0, (*tb->edit)->teLength, tb->edit);
            *(unsigned char *)b = 1;
            break;
        }
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        tb->active = AsByte(msg == kMsgActivate);
        if (kHasText(tb))
            update = 1;
        break;
    case kMsgItems:
        if (tb->editable && tb->enabled && IT_ItemCallbackIsViewEnabled()) {
#if TARGET_API_MAC_CARBON
            if (tb->layout != 0) {
                e = IndirectAppendData(b, tb->layout, sizeof(tb->layout));
                if (e != 0) {
                    err = e;
                    goto done;
                }
                break;
            }
#endif
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
            if (!tb->clicking KEEP_NO_LAYOUT)
                TESetSelect(0, (*tb->edit)->teLength, tb->edit);
            tb->focused = 1;
        } else {
            tb->focused = 0;
        }
        if (kHasText(tb))
            update = 1;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = tb->bounds;
        break;
    case kMsgBounds:
        tb->bounds = *(const ITRect *)a;
        CopyDT_2SysRect(&r, &tb->bounds);
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0)
            break;
#endif
        if (tb->edit != 0) {
            (*tb->edit)->viewRect = r;
            (*tb->edit)->destRect = r;
        }
        break;
    case kMsgGetTitle:
        e = IndirectSetSize(b, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgGetText:
        IndirectGetLock(b);
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            e = IndirectAppend(b, tb->text);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        }
#endif
        if (tb->edit != 0) {
            HLock((*tb->edit)->hText);
            err = IndirectAppendData(b, *(*tb->edit)->hText, (*tb->edit)->teLength);
            HUnlock((*tb->edit)->hText);
            if (err == 0)
                break;
            goto done;
        }
        break;
    case kMsgSetText:
        IndirectGetLock((void **)a);
#if TARGET_API_MAC_CARBON
        if (tb->layout == 0 && tb->edit == 0)
            break;
        if (IndirectIsNormalizedUnicode((void **)a) && tb->layout == 0) {
            converted = IndirectAllocateU2C((void **)a);
            a = (ULong)converted;
            if (a == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else if (!IndirectIsNormalizedUnicode((void **)a) && tb->layout != 0) {
            converted = IndirectAllocateC2U((void **)a, 0);
            a = (ULong)converted;
            if (a == 0) {
                err = kOutOfMemory;
                goto done;
            }
        }
#else
        if (tb->edit == 0)
            break;
#endif
        text = (void **)a;
        IndirectSetLock(text, 1);
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            IndirectSetLock(tb->text, 0);
            e = IndirectReplace(tb->text, text);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(tb->text, 1);
            n = IndirectGetSize(tb->text) >> 1;
            e = ATSUSetTextPointerLocation(tb->layout, tb->textPointer = *tb->text, 0, n, n);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else
#endif
        TESetText(*text, IndirectGetSize(text), tb->edit);
        IndirectSetLock(text, 0);
        value = 0;
        do {
            value = IT_ItemCallbackGetNextRun(value, &face, &size, &style, &color, 0, &order);
        } while (value != 0 && TEXT_EMPTY(value));
        if (value == 0)
            IT_ItemCallbackGetFont(&face, &size, &style, &color, 0);
        style &= ~(ULong)kStyleLink;
        e = _IT_TextFormat(hwnd, dc, tb, &format, face, size, style, &color, order);
        if (e != 0) {
            err = e;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            if (ATSUSetRunStyle(tb->layout, *(ATSUStyle *)&format, kATSUFromTextBeginning, kATSUToTextEnd) != 0) {
                err = kLayoutFailed;
                goto done;
            }
            break;
        }
#endif
        TESetSelect(0, (*tb->edit)->teLength, tb->edit);
        TESetStyle(doAll, &format, false, tb->edit);
        TESetSelect(0, 0, tb->edit);
        TEKey(' ', tb->edit);
        TEKey(kBackspace, tb->edit);
        TESetSelect(0, (*tb->edit)->teLength, tb->edit);
        break;
    case kMsgGetValue:
        *(int32_t *)b = 0;
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
    if (update) {
#if TARGET_API_MAC_CARBON
        if (tb->active && tb->editable && tb->focused && tb->visible && tb->enabled && IT_ItemCallbackIsViewEnabled()) {
            if (tb->layout == 0)
                TEActivate(tb->edit);
        } else if (tb->layout == 0) {
            TEDeactivate(tb->edit);
        }
        if (_PlatformIsThreaded() && tb->edit != 0) {
            r = (*tb->edit)->viewRect;
            InvalWindowRect(hwnd, &r);
        }
#else
        if (tb->active && tb->editable && tb->focused && tb->visible && tb->enabled && IT_ItemCallbackIsViewEnabled())
            TEActivate(tb->edit);
        else
            TEDeactivate(tb->edit);
#endif
    }
done:
#if TARGET_API_MAC_CARBON
    if (converted != 0)
        IndirectDeallocate(converted);
#endif
    SetGWorld(savedPort, savedDevice);
    return err;
}


#if TARGET_API_MAC_CARBON
#define PIECE_EMPTY(p) ((tb->convert && *(const unsigned short *)(p) == 0) || (!tb->convert && *(p) == 0))
#else
#define PIECE_EMPTY(p) (*(p) == 0)
#endif
#if TARGET_API_MAC_CARBON
#define KEEP_WIDE (!tb->editable)
#define kLayoutFailed -8400
#else
#define KEEP_WIDE 0
#endif
static int32_t IT_TextBoxCreateEdit(void *hwnd, void *dc, ITTextBox *tb)
{
    int32_t err;
    int32_t e;
    void **text;
    ITElement *node;
    char *lastHref;
    char *piece;
    ULong length;
    unsigned char block;
    unsigned char afterSpace;
    int32_t start;
    int32_t end;
    unsigned char face0;
    char c;
    ULong count;
    char tag[256];
#if TARGET_API_MAC_CARBON
    unsigned short wide[0x100];
#endif
    ITTextLink link;
    TextStyle format;
    Rect view;
    Rect dest;
    char *face;
    int32_t size;
    ULong style;
    ITColor color;
    char *href;
    ULong order;

    err = 0;
    text = 0;
    lastHref = 0;
    block = 0;
    afterSpace = 1;
    order = 0;
    piece = IT_ItemCallbackGetParam("SINGLE", 0);
    if (piece != 0) {
        tb->single = IT_ItemCallbackGetFlag(piece);
    } else {
        do
            piece = IT_ItemCallbackGetNextRun(piece, 0, &size, 0, 0, 0, 0);
        while (piece != 0 && PIECE_EMPTY(piece));
        if (piece == 0)
            IT_ItemCallbackGetFont(0, &size, 0, 0, 0);
        if (tb->bounds.bottom - tb->bounds.top <= size * 2)
            tb->single = 1;
    }
#if TARGET_API_MAC_CARBON
    if (!tb->editable) {
        if (ATSUCreateTextLayout(&tb->layout) != 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((tb->text = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((tb->lines = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(tb->text, 1);
        e = ATSUSetTextPointerLocation(tb->layout, tb->textPointer = *tb->text, 0, 0, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        ATSUSetTransientFontMatching(tb->layout, 1);
        ATSU_LayoutSetOptions(tb->layout, kLineOptions);
        if (tb->aligned) {
            switch (tb->align) {
            case kAlignCenter:
                ATSU_LayoutSetFixed(tb->layout, kFlushCenter, kFlushTag);
                break;
            case kAlignRight:
                ATSU_LayoutSetFixed(tb->layout, kFlushEnd, kFlushTag);
                break;
            case kAlignLeft:
                ATSU_LayoutSetFixed(tb->layout, kFlushStart, kFlushTag);
                break;
            }
        }
        err = 0;
    } else
#endif
    {
    CopyDT_2SysRect(&view, &tb->bounds);
    CopyDT_2SysRect(&dest, &tb->bounds);
    if (tb->editable && tb->single)
        dest.right = kOffRight;
    if ((tb->edit = TEStyleNew(&dest, &view)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (tb->aligned) {
        switch (tb->align) {
        case kAlignLeft:
            TESetAlignment(teFlushLeft, tb->edit);
            break;
        case kAlignCenter:
            TESetAlignment(teCenter, tb->edit);
            break;
        case kAlignRight:
            TESetAlignment(teFlushRight, tb->edit);
            break;
        }
    }
    }
    if ((tb->links = IndirectAllocate(0)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if ((text = IndirectAllocate(0)) == 0) {
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
#if TARGET_API_MAC_CARBON
            if (tb->convert)
                U2CStringCopySafe((unsigned char *)tag, (const unsigned short *)node->name, sizeof(tag), -1);
            else
#endif
            StringCopySafe(tag, node->name, sizeof(tag));
            if (StringCompare(tag, "P") == 0) {
                piece = "\n\n";
                block = 1;
            } else if (StringCompare(tag, "BR") == 0) {
                piece = "\n";
                block = 1;
            } else if (StringCompare(tag, "LI") == 0) {
                piece = "\n\245 ";
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
#if TARGET_API_MAC_CARBON
            if (tb->convert && block && piece != 0)
                piece = (char *)C2UStringCopySafe(wide, (const unsigned char *)piece, sizeof(wide), -1);
#endif
            break;
        default:
            piece = 0;
            block = 0;
            break;
        }
        if (piece != 0) {
            IndirectSetLock(text, 0);
            IndirectSetSize(text, 0);
            e = IT_AppendText(text, KEEP_WIDE, piece, tb->convert, AsByte(!((style & kStylePre) != 0 || block)), afterSpace);
            if (e != 0) {
                err = e;
                goto done;
            }
            IndirectSetLock(text, 1);
            piece = *text;
        }
        if (piece == 0)
            continue;
#if TARGET_API_MAC_CARBON
        if (tb->editable)
            length = StringGetLength((const unsigned char *)piece);
        else
            length = UnicodeGetCharCount(piece);
#else
        length = StringGetLength((const unsigned char *)piece);
#endif
        if (length == 0)
            continue;
#if TARGET_API_MAC_CARBON
        if (!_PlatformIsThreaded() && !gIT_HasInternetConfig)
#else
        if (!gIT_HasInternetConfig)
#endif
            style &= ~(ULong)kStyleLink;
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            UniCharArrayOffset from, to;

            IndirectSetLock(tb->text, 0);
            from = IndirectGetSize(tb->text) >> 1;
            err = IndirectAppendData(tb->text, piece, UnicodeGetByteCount((const unsigned short *)piece));
            IndirectSetLock(tb->text, 1);
            to = IndirectGetSize(tb->text) >> 1;
            if (tb->textPointer != *tb->text) {
                e = ATSUTextMoved(tb->layout, tb->textPointer = *tb->text);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
            e = ATSUTextInserted(tb->layout, from, to - from);
            if (e != 0) {
                err = e;
                goto done;
            }
            link.start = start = from;
            end = to;
        } else
#endif
        {
        link.start = start = (*tb->edit)->teLength;
        (*tb->edit)->selStart = start;
        (*tb->edit)->selEnd = start;
        TEInsert(piece, length, tb->edit);
        end = (*tb->edit)->teLength;
        }
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
            e = _IT_TextFormat(hwnd, dc, tb, &format, face, size, style, &color, order);
            if (e != 0) {
                err = e;
                goto done;
            }
#if TARGET_API_MAC_CARBON
            if (tb->layout != 0) {
                if (ATSUSetRunStyle(tb->layout, *(ATSUStyle *)&format, start, end - start) != 0) {
                    err = kLayoutFailed;
                    goto done;
                }
            } else
#endif
            {
            (*tb->edit)->selStart = start;
            (*tb->edit)->selEnd = end;
            face0 = format.tsFace;
            if (face0) {
                format.tsFace = 0;
                TESetStyle(doAll, &format, false, tb->edit);
                format.tsFace = face0;
            }
            TESetStyle(doAll, &format, false, tb->edit);
            }
        }
        lastHref = href;
#if TARGET_API_MAC_CARBON
        if (!tb->editable) {
            ULong u = UnicodeGetCharByIndex((const unsigned short *)piece, length - 1);

            afterSpace = AsByte(u == '\t' || u == '\n' || u == '\r' || u == ' ');
        } else
#endif
        {
        c = *(char *)(length + (ULong)piece - 1);
        afterSpace = AsByte(c == '\t' || c == '\n' || c == '\r' || c == ' ');
        }
    }
    if (tb->editable) {
        piece = 0;
        do
            piece = IT_ItemCallbackGetNextRun(piece, &face, &size, &style, &color, &href, &order);
        while (piece != 0 && PIECE_EMPTY(piece));
        if (piece == 0)
            IT_ItemCallbackGetFont(&face, &size, &style, &color, &href);
        style &= ~(ULong)kStyleLink;
        e = _IT_TextFormat(hwnd, dc, tb, &format, face, size, style, &color, order);
        if (e != 0) {
            err = e;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            if (ATSUSetRunStyle(tb->layout, *(ATSUStyle *)&format, kATSUFromTextBeginning, kATSUToTextEnd) != 0) {
                err = kLayoutFailed;
                goto done;
            }
        } else
#endif
        {
        if ((*tb->edit)->teLength == 0) {
            TEInsert(" ", 1, tb->edit);
            TESetSelect(0, (*tb->edit)->teLength, tb->edit);
            TESetStyle(doAll, &format, false, tb->edit);
            TEDelete(tb->edit);
        } else {
            TESetSelect(0, (*tb->edit)->teLength, tb->edit);
            TESetStyle(doAll, &format, false, tb->edit);
        }
        TESetSelect(0, 0, tb->edit);
        TEKey(' ', tb->edit);
        TEKey(kBackspace, tb->edit);
        TESetSelect(0, (*tb->edit)->teLength, tb->edit);
        }
    }
#if TARGET_API_MAC_CARBON
    if (tb->layout != 0)
        goto done;
#endif
    TECalText(tb->edit);
    if (tb->editable)
        TEAutoView(true, tb->edit);
done:
    if (text != 0)
        IndirectDeallocate(text);
    return err;
}

#if TARGET_API_MAC_CARBON
#define kFormatParam void *ref
#else
#define kFormatParam TextStyle *format
#endif
static int32_t _IT_TextFormat(void *hwnd, void *dc, ITTextBox *tb, kFormatParam, const char *face,
                  int32_t size, ULong style, const ITColor *color, ULong order)
{
#if TARGET_API_MAC_CARBON
    ATSUStyle *atsu;
    TextStyle *format;
    int32_t err;
    int32_t e;
    ITTextStyleNode *node;
    char name[256];
    char upper[256];
    char family[256];
    RGBColor rgb;
    ATSUFontID font;
    short fontFace;
    short number;

#else
    short number;
    char name[256];
    char upper[256];
    char family[256];

#endif
#if TARGET_API_MAC_CARBON
    atsu = ref;
    format = ref;
    err = 0;
    if (tb->layout != 0) {
        *atsu = 0;
        if (order != 0) {
            for (node = tb->styles; node != 0; node = node->next) {
                if (node->order == order) {
                    *atsu = node->style;
                    goto done;
                }
            }
        }
        e = ATSUCreateStyle(atsu);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (*atsu == 0) {
            err = kOutOfMemory;
            goto done;
        }
        if ((node = MemoryAllocateClear(sizeof(ITTextStyleNode))) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        node->order = order;
        node->style = *atsu;
        node->next = tb->styles;
        tb->styles = node;
        e = ATSU_FontFromFamily(1, style & (kStyleBold | kStyleItalic | kStyleUnderline), &font, &fontFace);
        if (e != 0)
            err = e;
        e = ATSU_StyleSetFont(*atsu, font, kATSUFontTag);
        if (e != 0)
            err = e;
        e = ATSU_StyleSetFixed(*atsu, size != 0 ? size << 16 : kDefaultPoints << 16, kATSUSizeTag);
        if (e != 0)
            err = e;
        e = ATSU_StyleSetFace(*atsu, (fontFace | style) & (kStyleBold | kStyleItalic | kStyleUnderline));
        if (e != 0)
            err = e;
        rgb = DT_2SysColor((style & kStyleColor) != 0 ? color : &tb->fore);
        e = ATSU_StyleSetColor(*atsu, rgb);
        if (e != 0)
            err = e;
    } else
#endif
    {
    MemoryClear(format, sizeof(TextStyle));
    format->tsFont = systemFont + 1;
    format->tsFace = style & (kStyleBold | kStyleItalic | kStyleUnderline);
    format->tsSize = size != 0 ? size : kDefaultPoints;
    format->tsColor = DT_2SysColor((style & kStyleColor) != 0 ? color : &tb->fore);
    }
    if (face != 0 && _DT_GetFontByNameList(dc, face, name) != 0) {
        String2UpperCopySafe(upper, face, sizeof(upper));
        String2UpperCopySafe(family, name, sizeof(family));
        if (StringFindString(upper, kBoldSuffix) != 0 && StringCompare(family, kBoldName) == 0) {
#if TARGET_API_MAC_CARBON
            if (tb->layout != 0) {
                ATSU_StyleSetByte(*atsu, 1, kATSUQDBoldfaceTag);
                err = 0;
            } else
#endif
            format->tsFace |= kBoldBit;
        }
        GetFNum((unsigned char *)StringC2P(name), &number);
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            ATSU_FontFromFamily(number, 0, &font, &fontFace);
            ATSU_StyleSetFont(*atsu, font, kATSUFontTag);
            err = 0;
        } else
#endif
        format->tsFont = number;
    }
    if ((style & (kStyleTeletype | kStylePre)) != 0) {
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            ATSU_FontFromFamily(kFontIDMonaco, 0, &font, &fontFace);
            ATSU_StyleSetFont(*atsu, font, kATSUFontTag);
            err = 0;
        } else
#endif
        format->tsFont = kFontIDMonaco;
    }
    if ((style & kStyleLink) != 0) {
#if TARGET_API_MAC_CARBON
        if (tb->layout != 0) {
            ATSU_StyleSetByte(*atsu, 1, kATSUQDUnderlineTag);
            ATSU_StyleSetColor(*atsu, DT_2SysColor(&tb->link));
            err = 0;
            goto done;
        }
#endif
        format->tsColor = DT_2SysColor(&tb->link);
        format->tsFace |= kUnderlineBit;
    }
#if TARGET_API_MAC_CARBON
done:
    return err;
#else
    return 0;
#endif
}

#if TARGET_API_MAC_CARBON
static int32_t IT_AppendText(void **out, unsigned char upper, const char *text, unsigned char convert,
                  unsigned char collapse, unsigned char afterSpace)
{
    int32_t err;
    int32_t e;
    ULong c;
    const char *here;
    const char *piece;
    unsigned char wide;
    unsigned char space;
    ULong length;
    ULong n;
    unsigned short buffer[0x100];

    err = 0;
    wide = convert;
    do {
        char one[2] = { 0, 0 };

        here = text;
        space = 0;
        piece = here;
        if (wide) {
            UnicodeAdvancePtr((const unsigned short **)&text, &c);
            length = 2;
        } else {
            c = *here;
            text = here + 1;
            length = 1;
        }
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
            if (convert) {
                if (*(const unsigned short *)text == '\n')
                    continue;
            } else {
                if (*text == '\n')
                    continue;
            }
        case '\n':
            if (collapse)
                space = 1;
            else
                piece = "\r";
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
                if (upper) {
                    n = UnicodeGetCharCountFromBytes(*out, n);
                    c = UnicodeGetCharByIndex(*out, n - 1);
                } else {
                    c = ((char *)*out)[(int32_t)n - 1];
                }
                if (c == ' ')
                    continue;
            } else if (afterSpace) {
                continue;
            }
            piece = " ";
        }
        if (piece != here) {
            if (upper) {
                C2UStringCopySafe(buffer, (const unsigned char *)piece, sizeof(buffer), -1);
                piece = (const char *)buffer;
                length = UnicodeGetByteCount((const unsigned short *)piece);
            } else {
                length = StringGetLength((const unsigned char *)piece);
            }
        }
        e = IndirectAppendData(out, piece, length);
        if (e != 0) {
            err = e;
            goto done;
        }
    } while (c != 0);
    if (upper) {
        e = IndirectAppendData(out, L"", sizeof(L""));
        if (e != 0)
            err = e;
    } else {
        e = IndirectAppendData(out, "", 1);
        if (e != 0)
            err = e;
    }
done:
    return err;
}
#else
static int32_t IT_AppendText(void **out, unsigned char upper, const char *text, unsigned char convert,
                  unsigned char collapse, unsigned char afterSpace)
{
    int32_t err;
    int32_t e;
    ULong c;
    const char *here;
    const char *piece;
    unsigned char space;
    ULong length;
    ULong n;

    err = 0;
    do {
        here = text;
        piece = text;
        space = 0;
        {
        char one[2] = { 0, 0 };

        length = 1;
        c = *text;
        text++;
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
                piece = "\r";
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
                if ((c = ((char *)*out)[(int32_t)n - 1]) == ' ')
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
#endif

#if TARGET_API_MAC_CARBON
static int32_t _IT_TextCheck(void *hwnd, void *dc, ITTextBox *tb, ITPoint *pt, unsigned char track,
                  ITClick *click)
{
    ULong index;
    ULong count;
    ULong i;
    ULong start;
    ULong end;
    ULong first;
    ULong last;
    int32_t firstHeight;
    int32_t lastHeight;
    ULong lines;
    ITTextLink *entry;
    unsigned char inside;
    ITPoint *where;
    unsigned char lit;
    int32_t err;
    ULong lineIndex;
    ULong lineCount;
    ULong line2Index;
    ULong line2Count;
    unsigned char found;
    unsigned char done;
    ATSULine line;
    ITInputEvent event;
    ATSULine line2;
    ITRect r1;
    ITRect r2;
    ITRect r3;
    ITRect box;
    ATSUCaret start1;
    ATSUCaret start2;
    ATSUCaret end1;
    ATSUCaret end2;
    Rect view;
    Rect lineRect;
    ITPoint a;
    ITPoint b;
    Rect view2;
    Rect lineRect2;
    UniCharArrayOffset offset;
    UniCharArrayOffset offset2;
    Point p1;
    Point p2;
    Boolean leading2;
    Boolean leading;

    err = 0;
    if (tb->layout != 0) {
        lineCount = 0;
        if (tb->lines != 0)
            lineCount = IndirectGetSize(tb->lines) / sizeof(ATSULine);
        view = DT_2SysRect(&tb->bounds);
        for (lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            {
                const ATSULine *from = &(*(ATSULine **)tb->lines)[lineIndex];

                line = *from;
            }
            lineRect = view;
            if (tb->rotation == 0) {
                lineRect.top = view.top + SFix32_To_SInt32(line.offset - line.ascent);
                lineRect.bottom = view.top + SFix32_To_SInt32(line.offset + line.descent);
            } else {
                lineRect.right = view.right - SFix32_To_SInt32(line.offset + line.ascent);
                lineRect.left = view.right - SFix32_To_SInt32(line.offset - line.descent);
            }
            if (PtInRect(*(Point *)pt, &lineRect))
                break;
        }
        if (lineIndex < lineCount) {
            offset = offset2 = line.start;
            if (tb->rotation == 0) {
                Fixed y = line.offset - SInt32_To_SFix32(((Point *)pt)->v - view.top);

                ATSUPositionToOffset(tb->layout, SInt32_To_SFix32(((Point *)pt)->h - lineRect.left), y, &offset, &leading2, &offset2);
            } else {
                Fixed x = SInt32_To_SFix32(((Point *)pt)->v - lineRect.right);

                ATSUPositionToOffset(tb->layout, SInt32_To_SFix32(view.right - ((Point *)pt)->h) - line.offset, x, &offset, &leading2, &offset2);
            }
            index = offset;
            err = 0;
        }
    } else {
        index = TEGetOffset(*(Point *)pt, tb->edit);
    }
    count = IndirectGetSize(tb->links) / sizeof(ITTextLink);
    for (i = 0; i < count; i++) {
        entry = (ITTextLink *)*tb->links + i;
        start = entry->start;
        end = start + entry->length;
        if (index >= end)
            continue;
        if (index < start)
            break;
        DT_SetRect(&r1, 0, 0, 0, 0);
        DT_SetRect(&r2, 0, 0, 0, 0);
        DT_SetRect(&r3, 0, 0, 0, 0);
        if (tb->layout != 0) {
            line2Count = 0;
            found = 0;
            done = 0;
            if (tb->lines != 0)
                line2Count = IndirectGetSize(tb->lines) / sizeof(ATSULine);
            view2 = DT_2SysRect(&tb->bounds);
            for (line2Index = 0; line2Index < line2Count && !done; line2Index++) {
                {
                    const ATSULine *from = &(*(ATSULine **)tb->lines)[line2Index];

                    line2 = *from;
                }
                lineRect2 = view2;
                if (tb->rotation == 0) {
                    lineRect2.top = view2.top + SFix32_To_SInt32(line2.offset - line2.ascent);
                    lineRect2.bottom = view2.top + SFix32_To_SInt32(line2.offset + line2.descent);
                } else {
                    lineRect2.right = view2.right - SFix32_To_SInt32(line2.offset + line2.ascent);
                    lineRect2.left = view2.right - SFix32_To_SInt32(line2.offset - line2.descent);
                }
                if (!found && start < line2.end) {
                    found = 1;
                    if (end <= line2.end) {
                        done = 1;
                        ATSUOffsetToPosition(tb->layout, start, true, &start1, &start2, &leading);
                        ATSUOffsetToPosition(tb->layout, end, false, &end1, &end2, &leading);
                    } else {
                        ATSUOffsetToPosition(tb->layout, start, true, &start1, &start2, &leading);
                        ATSUOffsetToPosition(tb->layout, line2.end, false, &end1, &end2, &leading);
                    }
                    if (tb->rotation == 0) {
                        if (start1.fX < end1.fX) {
                            lineRect2.left += SFix32_To_SInt32(start1.fX);
                            if (done)
                                lineRect2.right = lineRect2.left + SFix32_To_SInt32(end1.fX);
                        } else {
                            lineRect2.right = lineRect2.left + SFix32_To_SInt32(start1.fX);
                            if (done)
                                lineRect2.left += SFix32_To_SInt32(end1.fX);
                        }
                    } else {
                        if (start1.fY < end1.fY) {
                            lineRect2.top += SFix32_To_SInt32(start1.fY);
                            if (done)
                                lineRect2.bottom = lineRect2.top + SFix32_To_SInt32(start1.fY);  /* BUG: start1 for end1 */
                        } else {
                            lineRect2.bottom = lineRect2.top + SFix32_To_SInt32(start1.fY);
                            if (done)
                                lineRect2.top += SFix32_To_SInt32(end1.fY);
                        }
                    }
                    if (done)
                        r1 = Sys2DT_Rect(&lineRect2);
                } else if (!done && end <= line2.end) {
                    done = 1;
                    ATSUOffsetToPosition(tb->layout, line2.start, true, &start1, &start2, &leading);
                    ATSUOffsetToPosition(tb->layout, end, false, &end1, &end2, &leading);
                    if (tb->rotation == 0) {
                        if (start1.fX < end1.fX)
                            lineRect2.right = lineRect2.left + SFix32_To_SInt32(end1.fX);
                        else
                            lineRect2.left += SFix32_To_SInt32(end1.fX);
                    } else {
                        if (start1.fY < end1.fY)
                            lineRect2.bottom = lineRect2.top + SFix32_To_SInt32(end1.fY);
                        else
                            lineRect2.top += SFix32_To_SInt32(end1.fY);
                    }
                    if ((r1.bottom = lineRect2.top) != 0) {
                        r2 = Sys2DT_Rect(&lineRect2);
                    } else {
                        r2 = Sys2DT_Rect(&view2);
                        r3 = Sys2DT_Rect(&lineRect2);
                        if (tb->rotation == 0) {
                            r2.top = r1.bottom;
                            r2.bottom = r3.top;
                        } else {
                            r2.left = r1.right;
                            r2.right = r3.left;
                        }
                    }
                }
            }
        } else {
            lines = (*tb->edit)->nLines;
            for (first = 0; first < lines; first++) {
                if ((*tb->edit)->lineStarts[first + 1] >= start)
                    break;
            }
            for (last = first; last < lines; last++) {
                if ((*tb->edit)->lineStarts[last + 1] >= end)
                    break;
            }
            firstHeight = TEGetHeight(first, first, tb->edit);
            lastHeight = TEGetHeight(last, last, tb->edit);
            p1 = TEGetPoint(start, tb->edit);
            p2 = TEGetPoint(end, tb->edit);
            p1.v -= firstHeight;
            p2.v -= lastHeight;
            view2 = (*tb->edit)->destRect;
            CopySys2DT_Rect(&box, &view2);
            CopySys2DT_Point(&a, &p1);
            CopySys2DT_Point(&b, &p2);
            if (a.v < b.v) {
                DT_SetRect(&r1, a.h - 2, a.v, box.right, a.v + firstHeight);
                DT_SetRect(&r2, box.left - 2, a.v + firstHeight, box.right, b.v);
                DT_SetRect(&r3, box.left - 2, b.v, b.h, b.v + lastHeight);
            } else {
                DT_SetRect(&r1, 0, 0, 0, 0);
                DT_SetRect(&r2, a.h - 2, a.v, b.h, a.v + firstHeight);
                DT_SetRect(&r3, 0, 0, 0, 0);
            }
        }
        if (track) {
            IT_ItemCallbackSoundEvent(kEventTrack);
            _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
            where = &event.where;
            lit = inside = 1;
            while (InputReadMouseState(hwnd, &event) && (event.modifiers & kModButtonDown)) {
                inside = DT_IsPointInRect(where, &r1) || DT_IsPointInRect(where, &r2) || DT_IsPointInRect(where, &r3);
                if (inside != lit) {
                    IT_ItemCallbackSoundEvent(inside ? kEventIn : kEventOut);
                    _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
                }
                lit = inside;
            }
            IT_ItemCallbackSoundEvent(kEventRelease);
            if (inside) {
                _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
                StringCopySafe(click->link, (*(ITTextLink **)tb->links)[i].href, kLinkMax);
                click->followed = 1;
            }
        } else {
            click->followed = 1;
            StringCopySafe(click->link, (*(ITTextLink **)tb->links)[i].href, kLinkMax);
        }
        break;
    }
    return err;
}
#else
static int32_t _IT_TextCheck(void *hwnd, void *dc, ITTextBox *tb, ITPoint *pt, unsigned char track,
                  ITClick *click)
{
    ULong index;
    ULong count;
    ULong i;
    ULong start;
    ULong end;
    ULong first;
    ULong last;
    int32_t firstHeight;
    int32_t lastHeight;
    ULong lines;
    ITTextLink *entry;
    unsigned char inside;
    ITPoint *where;
    unsigned char lit;
    ITInputEvent event;
    ITRect r1;
    ITRect r2;
    ITRect r3;
    ITRect box;
    ITPoint a;
    ITPoint b;
    Rect rect;
    Point p1;
    Point p2;

    index = TEGetOffset(*(Point *)pt, tb->edit);
    count = IndirectGetSize(tb->links) / sizeof(ITTextLink);
    for (i = 0; i < count; i++) {
        entry = (ITTextLink *)*tb->links + i;
        start = entry->start;
        end = start + entry->length;
        if (index >= end)
            continue;
        if (index < start)
            break;
        DT_SetRect(&r1, 0, 0, 0, 0);
        DT_SetRect(&r2, 0, 0, 0, 0);
        DT_SetRect(&r3, 0, 0, 0, 0);
        lines = (*tb->edit)->nLines;
        for (first = 0; first < lines; first++) {
            if ((*tb->edit)->lineStarts[first + 1] >= start)
                break;
        }
        for (last = first; last < lines; last++) {
            if ((*tb->edit)->lineStarts[last + 1] >= end)
                break;
        }
        firstHeight = TEGetHeight(first, first, tb->edit);
        lastHeight = TEGetHeight(last, last, tb->edit);
        p1 = TEGetPoint(start, tb->edit);
        p2 = TEGetPoint(end, tb->edit);
        p1.v -= firstHeight;
        p2.v -= lastHeight;
        rect = (*tb->edit)->destRect;
        CopySys2DT_Rect(&box, &rect);
        CopySys2DT_Point(&a, &p1);
        CopySys2DT_Point(&b, &p2);
        if (a.v < b.v) {
            DT_SetRect(&r1, a.h - 2, a.v, box.right, a.v + firstHeight);
            DT_SetRect(&r2, box.left - 2, a.v + firstHeight, box.right, b.v);
            DT_SetRect(&r3, box.left - 2, b.v, b.h, b.v + lastHeight);
        } else {
            DT_SetRect(&r1, 0, 0, 0, 0);
            DT_SetRect(&r2, a.h - 2, a.v, b.h, a.v + firstHeight);
            DT_SetRect(&r3, 0, 0, 0, 0);
        }
        if (track) {
            IT_ItemCallbackSoundEvent(kEventTrack);
            _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
            where = &event.where;
            lit = inside = 1;
            while (InputReadMouseState(hwnd, &event) && (event.modifiers & kModButtonDown)) {
                inside = DT_IsPointInRect(where, &r1) || DT_IsPointInRect(where, &r2) || DT_IsPointInRect(where, &r3);
                if (inside != lit) {
                    IT_ItemCallbackSoundEvent(inside ? kEventIn : kEventOut);
                    _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
                }
                lit = inside;
            }
            IT_ItemCallbackSoundEvent(kEventRelease);
            if (inside) {
                _IT_HiliteLink(hwnd, dc, tb, &r1, &r2, &r3);
                StringCopySafe(click->link, (*(ITTextLink **)tb->links)[i].href, kLinkMax);
                click->followed = 1;
            }
        } else {
            click->followed = 1;
            StringCopySafe(click->link, (*(ITTextLink **)tb->links)[i].href, kLinkMax);
        }
        break;
    }
    return 0;
}
#endif

static void _IT_HiliteLink(void *hwnd, void *dc, ITTextBox *tb, const ITRect *r1, const ITRect *r2,
                  const ITRect *r3)
{
    Rect r;
    RGBColor rgb;
    RGBColor saved;

    GetBackColor(&saved);
#if TARGET_API_MAC_CARBON
    if (!_PlatformIsThreaded())
#endif
    {
        RGBBackColor(CopyDT_2SysColor(&rgb, &tb->link));
        HiliteColor(CopyDT_2SysColor(&rgb, &tb->fore));
    }
    LMSetHiliteMode(LMGetHiliteMode() & kHiliteBit);
    InvertRect(CopyDT_2SysRect(&r, r1));
    LMSetHiliteMode(LMGetHiliteMode() & kHiliteBit);
    InvertRect(CopyDT_2SysRect(&r, r2));
    LMSetHiliteMode(LMGetHiliteMode() & kHiliteBit);
    InvertRect(CopyDT_2SysRect(&r, r3));
    LMGetHiliteRGB(&rgb);
    HiliteColor(&rgb);
    RGBBackColor(&saved);
    QDFlushPortBuffer(GetWindowPort(hwnd), 0);
}
