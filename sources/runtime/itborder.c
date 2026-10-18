// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itborder.c -- the interface tool's BORDER control (0x0051fe38..0x005210da): a titled frame or rule, solid or dotted, bevelled by INDENT, optionally CHIPPED. Notes: Research/notes/itborder.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"
#include "ittoolkit.h"

extern unsigned char DT_IsRectReallyEmpty(const ITRect *r);                      /* itrect.c */
extern void DT_ColorLighten(ITColor *color);                                /* itrect.c */
extern void DT_ColorDarken(ITColor *color);                                /* itrect.c */
extern long DT_GetFillColor(ITImage *image, ITColor *fore);                 /* itgfx.c */
extern long DT_SetTextColor(ITImage *image, const ITColor *fore);           /* itgfx.c */
extern long DT_SetTextSize(ITImage *image, long size);                     /* itgfx.c */
extern long DT_GetTextInfo(ITImage *image, long *metrics);                 /* itgfx.c */
extern long DT_SetPixel(ITImage *image, long x, long y, const ITColor *color);  /* itgfx.c */
extern long DT_SetPenLocation(ITImage *image, long x, long y);                /* itgfx.c */
extern long DT_DrawLineTo(ITImage *image, long x, long y);                /* itgfx.c */
extern long DT_GetTextWidth(ITImage *image, const char *text, long length); /* itgfx.c */
extern long DT_DrawText(ITImage *image, const char *text, long length); /* itgfx.c */

/* the control's data handle */
typedef struct ITBorder {
    void **title;                                /* +0x00 */
    ITColor fore;                                /* +0x04 FGCOLOR */
    ITColor back;                                /* +0x08 BGCOLOR */
    unsigned char foreSet;                       /* +0x0c */
    unsigned char backSet;                       /* +0x0d */
    unsigned char chipped;                       /* +0x0e CHIPPED */
    unsigned char pad0f;
    short line;                                  /* +0x10 LINE */
    short indent;                                /* +0x12 INDENT: raised above 0, sunk below */
    long align;                                  /* +0x14 the title's ALIGN */
    ITRect bounds;                               /* +0x18 */
    unsigned char active;                        /* +0x28 */
    unsigned char visible;                       /* +0x29 */
    unsigned char pad2a[2];
} ITBorder;

#define kLineNone           0
#define kLineDotted         1
#define kLineSolid          2
#define kTitleFontSize      9
#define kTitleMinWidth      20
#define kTitleInset         12      /* the gap in the top edge starts this far in */
#define kTitleMargin        2       /* between the gap's ends and the title */
#define kGapSlack           4
#define kChipDepth          16
#define SM_CYVTHUMB         9
#define SM_CXHTHUMB         10

#define STEP(indent)        ((indent) < 0 ? -1 : 1)

long _IT_BorderDraw(void *hwnd, void *dc, ITBorder *border, unsigned char fill);
long IT_DrawBorderFrame(ITImage *image, ITRect *r, long indent, long gapLeft, long gapRight,
                  unsigned char chipped, unsigned char dotted, unsigned char fill,
                  const ITColor *lineColor, const ITColor *portBack, const ITColor *boxBack);

/* IT_BorderHandler (the BORDER control proc: 0 makes its data (LINE NONE / DOTTED / DASHED,
   ALIGN, INDENT, CHIPPED, FGCOLOR, BGCOLOR), 1 takes the unset colours from the DC, 3 frees
   the data, 0x12 paints; bounds, title and colours; no value, text or input). STATE: MATCH
   100%, exe 385 / obj 385, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051fe38
long IT_BorderHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    long err;
    long e;
    ITBorder *border;
    char *value;
    unsigned char enable;
    unsigned char dead;
    char text[256];
    ITPoint pt;
    ULong color;

    border = *data;
    dead = 0;
    err = 0;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITBorder));
        if (e != 0) {
            err = e;
            break;
        }
        IndirectSetLock(data, 1);
        border = *data;
        MemoryClear(*data, sizeof(ITBorder));
        border->title = IndirectAllocate(0);
        if (border->title == 0) {
            err = kOutOfMemory;
            break;
        }
        border->fore = gDT_ColorBlack;
        border->back = gDT_ColorWhite;
        border->line = kLineSolid;
        border->align = 0;
        border->chipped = 0;
        border->indent = 0;
        border->visible = 1;
        border->active = 1;
        value = IT_ItemCallbackGetParam("LINE", 0);
        if (value != 0) {
            String2UpperCopySafe(text, value, sizeof(text));
            if (StringCompare(text, "NONE") == 0)
                border->line = kLineNone;
            else if (StringCompare(text, "DOTTED") == 0 || StringCompare(text, "DASHED") == 0)
                border->line = kLineDotted;
        }
        IT_ItemCallbackGetAlign(&border->align);
        value = IT_ItemCallbackGetParam("INDENT", 0);
        if (value != 0)
            border->indent = (short)IT_ItemCallbackGetNumber(value);
        value = IT_ItemCallbackGetParam("CHIPPED", 0);
        if (value != 0)
            border->chipped = IT_ItemCallbackGetFlag(value);
        value = IT_ItemCallbackGetParam("FGCOLOR", 1);
        if (value != 0) {
            border->fore = IT_ItemCallbackGetColor(value);
            border->foreSet = 1;
        }
        value = IT_ItemCallbackGetParam("BGCOLOR", 1);
        if (value != 0) {
            border->back = IT_ItemCallbackGetColor(value);
            border->backSet = 1;
        }
        break;
    case kMsgOpen:
        color = GetTextColor(dc);
        if (color == CLR_INVALID) {
            err = GetLastError();
            err = kOutOfMemory;
            break;
        }
        if (!border->foreSet)
            CopySys2DT_Color(&border->fore, &color);
        color = GetBkColor(dc);
        if (color == CLR_INVALID) {
            err = GetLastError();
            err = kOutOfMemory;
            break;
        }
        if (!border->backSet)
            CopySys2DT_Color(&border->back, &color);
        break;
    case kMsgDispose:
        if (border->title != 0) {
            IndirectDeallocate(border->title);
            border->title = 0;
        }
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgPaint:
        if (!DT_IsRectReallyEmpty(&border->bounds)) {
            e = _IT_BorderDraw(hwnd, dc, border, 1);
            if (e != 0)
                err = e;
        }
        break;
    case kMsgWhat:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        break;
    case kMsgMouse:
        *(unsigned short *)b = 0;
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        border->active = msg == kMsgActivate ? (unsigned char)1 : (unsigned char)0;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = border->bounds;
        break;
    case kMsgBounds:
        border->bounds = *(ITRect *)a;
        break;
    case kMsgGetTitle:
        e = IndirectReplace(b, border->title);
        if (e != 0)
            err = e;
        break;
    case kMsgTitle:
        e = IndirectReplace(border->title, (void **)a);
        if (e != 0)
            err = e;
        break;
    case kMsgGetText:
        e = IndirectSetSize(b, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgSetText:
        err = kUnsupported;
        break;
    case kMsgGetValue:
        *(long *)b = 0;
        break;
    case kMsgSetValue:
        err = kUnsupported;
        break;
    case kMsgVisible:
        *(unsigned char *)b = border->visible;
        break;
    case kMsgShow:
        border->visible = a != 0 ? (unsigned char)1 : (unsigned char)0;
        break;
    case kMsgEnabled:
        *(unsigned char *)b = 0;
        break;
    case kMsgEnable:
        if (a != 0)
            enable = 1;
        else
            enable = 0;
        break;
    case kMsgGetFore:
    case kMsgGetBack:
        *(ITColor *)b = msg == kMsgGetFore ? border->fore : border->back;
        break;
    case kMsgFore:
    case kMsgBack:
        if (msg == kMsgFore) {
            border->fore = *(ITColor *)b;
            border->foreSet = 1;
        } else {
            border->back = *(ITColor *)b;
            border->backSet = 1;
        }
        break;
    case 0x02:
    case 0x10:
    case 0x11:
    case 0x13:
    case 0x14:
    case 0x16:
    case 0x17:
    case 0x21:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x28:
    case 0x32:
    case 0x33:
        break;
    }
    return err;
}

/* _IT_BorderDraw (paint a border: its colour halved towards white when inactive, the title in
   9-point text shortened with "..." to two thirds of the width and set into a gap of the top edge
   by ALIGN, the frame drawn by 0x0052061c). STATE: MATCH 100%, exe 272 / obj 272, byte-identical
   yes (785 bytes). OPEN: none; gapLeft holds the widest title first, and a centred title is
   placed from 0, as in the exe */
// FUNCTION: EVNOVA 0x0052030b
long _IT_BorderDraw(void *hwnd, void *dc, ITBorder *border, unsigned char fill)
{
    long err;
    long e;
    long result;
    ITImage *image;
    const ITColor *lineColor;
    ULong count;
    long ltr;
    long width;
    long offset;
    long gapLeft;
    ULong length;
    short line;
    long middle;
    long half;
    ITColor color;
    ITColor back;
    long metrics[5];

    err = 0;
    image = 0;
    image = DT_ContextCreateRaw(dc, 1);
    if (image == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = DT_GetFillColor(image, &color);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GetFillBackColor(image, &back);
    if (e != 0) {
        err = e;
        goto done;
    }
    color = border->fore;
    if (!border->active) {
        color.red = (color.red + 0xffU) >> 1;
        color.green = (color.green + 0xffU) >> 1;
        color.blue = (color.blue + 0xffU) >> 1;
    }
    e = DT_ResetAttributes(image, 1, 1, 1, 0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_SetTextSize(image, kTitleFontSize);
    if (e != 0) {
        err = e;
        goto done;
    }
    length = IndirectGetSize(border->title);
    if (length != 0 && border->bounds.right - border->bounds.left > kTitleMinWidth) {
        ltr = 1;
        gapLeft = (border->bounds.right - border->bounds.left) * 2 / 3;
        width = DT_GetTextWidth(image, *border->title, length);
        while (width > gapLeft && length != 0) {
            width = DT_GetTextWidth(image, "...", 3);
            length--;
            width += DT_GetTextWidth(image, *border->title, length);
        }
        count = length;
        switch (border->align) {
        case kAlignLeft:
            gapLeft = border->bounds.left + kTitleInset;
            length = border->bounds.left + kTitleInset + 2 * kTitleMargin + (half = width);
            width = border->bounds.left + kTitleInset + kTitleMargin + (ltr >= 0 ? 0 : width);
            break;
        case kAlignRight:
            gapLeft = border->bounds.right - kTitleInset - 2 * kTitleMargin - width;
            length = border->bounds.right - kTitleInset;
            width = border->bounds.right - kTitleInset - kTitleMargin - (offset = ltr < 0 ? 0 : width);
            break;
        case 0:
        default:
            half = width / 2;
            middle = (border->bounds.right - border->bounds.left) / 2;
            gapLeft = middle - half - kTitleMargin;
            length = width + middle - half + kTitleMargin;
            width /= 2;
            width = middle - width;
            break;
        }
    } else {
        width = gapLeft = length = count = 0;
    }
    line = border->line;
    if (line == kLineNone)
        lineColor = 0;
    else
        lineColor = &color;
    e = IT_DrawBorderFrame(image, &border->bounds, border->indent, gapLeft, length,
                     border->chipped, line == kLineDotted ? (unsigned char)1 : (unsigned char)0,
                     fill, lineColor, &back, &border->back);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_GetTextInfo(image, metrics);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_SetTextColor(image, &color);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (count != 0) {
        IndirectSetLock(border->title, 1);
        e = DT_DrawTextAt(image, width, ((ULong)(metrics[0] - metrics[1]) >> 1) + border->bounds.top,
                         *border->title, count);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (count < IndirectGetSize(border->title)) {
            e = DT_DrawText(image, "...", 3);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        IndirectSetLock(border->title, 0);
    }
done:
    if (image != 0)
        DT_ContextDispose(image);
    result = err;
    return result;
}

/* IT_DrawBorderFrame (draw a frame into a second port on the image's DC: filled with the box's
   colour (the bottom-right corner cut away when chipped), the outline in the line colour
   (dotted or solid) with a gap in the top edge for a title, a zero-width or zero-height
   box as a rule; then `indent' bevel passes, lit and shaded from the port's background
   colour outside the outline and from the box's inside it). STATE: MATCH 100%, exe 884 / obj
   884, byte-identical yes. OPEN: none; the inner bevel's last gap pixel takes the outer shade,
   as in the exe */
// FUNCTION: EVNOVA 0x0052061c
long IT_DrawBorderFrame(ITImage *image, ITRect *r, long indent, long gapLeft, long gapRight,
                  unsigned char chipped, unsigned char dotted, unsigned char fill,
                  const ITColor *lineColor, const ITColor *portBack, const ITColor *boxBack)
{
    long err;
    long e;
    ULong i;
    ITImage *port;
    unsigned char rule;
    ULong last;
    ITColor lit;
    ITColor shade;
    ITRect box;

    err = 0;
    port = 0;
    rule = 0;
    port = DT_ContextCreateRaw(DT_PeekAtSysContext(image), 1);
    if (port == 0) {
        err = kOutOfMemory;
        goto done;
    }
    if (gapLeft == gapRight) {
        gapLeft = r->right;
        gapRight = r->left;
    } else {
        gapLeft = gapLeft - kGapSlack <= r->left ? r->left : gapLeft - kGapSlack;
        gapRight = gapRight + kGapSlack < r->right ? gapRight + kGapSlack : r->right;
    }
    rule = DT_IsRectEmpty(r);
    e = DT_ResetAttributes(port, 1, 1, 0, 0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (fill) {
        e = DT_SetFillBackColor(port, boxBack);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (!chipped) {
            e = DT_EraseRect(port, r);
        } else {
            box = *r;
            box.right -= GetSystemMetrics(SM_CXHTHUMB);
            e = DT_EraseRect(port, &box);
            if (e != 0) {
                err = e;
                goto done;
            }
            box = *r;
            box.left = box.right - GetSystemMetrics(SM_CXHTHUMB);
            box.bottom -= GetSystemMetrics(SM_CYVTHUMB);
            e = DT_EraseRect(port, &box);
        }
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (lineColor != 0) {
        e = DT_SetLineAttributes(port, lineColor,
                         dotted || portBack != 0 ? portBack : boxBack != 0 ? boxBack : 0,
                         dotted ? gDT_PatternMdGray : gDT_PatternBlack, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (rule && r->top != r->bottom) {
            e = DT_SetPenLocation(port, r->left, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, r->left, r->bottom);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else if (rule) {
            e = DT_SetPenLocation(port, r->left, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (gapLeft != gapRight) {
                e = DT_DrawLineTo(port, gapLeft, r->top);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_SetPenLocation(port, gapRight, r->top);
                if (e != 0) {
                    err = e;
                    goto done;
                }
            }
            e = DT_DrawLineTo(port, r->right, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = DT_SetPenLocation(port, gapLeft, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, r->left, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, r->left, r->bottom);
            if (e != 0) {
                err = e;
                goto done;
            }
            if (chipped) {
                e = DT_DrawLineTo(port, r->right - GetSystemMetrics(SM_CXHTHUMB), r->bottom);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_DrawLineTo(port, r->right - GetSystemMetrics(SM_CXHTHUMB),
                                   r->bottom - kChipDepth);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_DrawLineTo(port, r->right, r->bottom - GetSystemMetrics(SM_CYVTHUMB));
            } else {
                e = DT_DrawLineTo(port, r->right, r->bottom);
            }
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, r->right, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, gapRight, r->top);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    if (indent != 0 && portBack != 0) {
        lit = shade = *portBack;
        DT_ColorLighten(&lit);
        DT_ColorDarken(&shade);
        if (indent < 0)
            portBack = &lit;
        else
            portBack = &shade;
        e = DT_SetLineAttributes(port, portBack, 0, gDT_PatternBlack, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (rule) {
            e = DT_SetPixel(port, r->left - 1, r->top, portBack);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (rule && gapLeft != gapRight) {
            e = DT_SetPixel(port, gapRight - 1, r->top, portBack);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (lineColor != 0)
            DT_RectInset(r, -1, -1);
        for (i = 0; i != (last = indent); i += STEP(indent)) {
            DT_RectInset(r, -i, -i);
            if (rule) {
                e = DT_SetPenLocation(port, r->left + 1 + i * 2, r->top);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                if (gapLeft != gapRight) {
                    e = DT_DrawLineTo(port, gapLeft - i, r->top);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    e = DT_SetPenLocation(port, gapRight + i, r->top);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                }
                e = DT_DrawLineTo(port, r->right - 1 - i * 2, r->top);
            } else {
                e = DT_SetPenLocation(port, gapLeft - i, r->top);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_DrawLineTo(port, r->left, r->top);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_DrawLineTo(port, r->left, r->bottom - 1);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_SetPenLocation(port, r->right - 1, r->top);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_DrawLineTo(port, gapRight + i, r->top);
            }
            if (e != 0) {
                err = e;
                goto done;
            }
            DT_RectInset(r, i, i);
        }
        if (lineColor != 0)
            DT_RectInset(r, 1, 1);
        if (indent < 0)
            portBack = &shade;
        else
            portBack = &lit;
        e = DT_SetLineAttributes(port, portBack, 0, gDT_PatternBlack, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (rule) {
            e = DT_SetPixel(port, r->right + 1, r->top, portBack);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (rule && gapLeft != gapRight) {
            e = DT_SetPixel(port, gapLeft + 1, r->top, portBack);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        if (lineColor != 0)
            DT_RectInset(r, -1, -1);
        for (i = 0; i != last; i += STEP(indent)) {
            DT_RectInset(r, -i, -i);
            if (rule) {
                e = DT_SetPenLocation(port, r->left + 1 + i * 2, r->bottom);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                if (gapLeft != gapRight) {
                    e = DT_DrawLineTo(port, gapLeft - i, r->bottom);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    e = DT_SetPenLocation(port, gapRight + i, r->bottom);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                }
                e = DT_DrawLineTo(port, r->right - 1 - i * 2, r->bottom);
            } else {
                e = DT_SetPenLocation(port, r->left + 1, r->bottom);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                if (chipped) {
                    e = DT_DrawLineTo(port, r->right - GetSystemMetrics(SM_CXHTHUMB),
                                       r->bottom);
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    e = DT_DrawLineTo(port, r->right - GetSystemMetrics(SM_CXHTHUMB),
                                       r->bottom - GetSystemMetrics(SM_CYVTHUMB));
                    if (e != 0) {
                        err = e;
                        goto done;
                    }
                    e = DT_DrawLineTo(port, r->right, r->bottom - GetSystemMetrics(SM_CYVTHUMB));
                } else {
                    e = DT_DrawLineTo(port, r->right, r->bottom);
                }
                if (e != 0) {
                    err = e;
                    goto done;
                }
                e = DT_DrawLineTo(port, r->right, r->top + 1);
            }
            if (e != 0) {
                err = e;
                goto done;
            }
            DT_RectInset(r, i, i);
        }
        if (lineColor != 0)
            DT_RectInset(r, 1, 1);
    }
    if (indent == 0 || rule || boxBack == 0)
        goto done;
    lit = shade = *boxBack;
    DT_ColorLighten(&lit);
    DT_ColorDarken(&shade);
    if (indent < 0)
        boxBack = &lit;
    else
        boxBack = &shade;
    e = DT_SetLineAttributes(port, boxBack, 0, gDT_PatternBlack, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (lineColor != 0 && gapLeft != gapRight) {
        e = DT_SetPixel(port, gapRight - 1, r->top, boxBack);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    DT_RectInset(r, 1, 1);
    for (i = 0; i != indent; i += STEP(indent)) {
        DT_RectInset(r, i, i);
        e = DT_SetPenLocation(port, r->left + 1, r->bottom);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (chipped) {
            e = DT_DrawLineTo(port, r->right - GetSystemMetrics(SM_CXHTHUMB), r->bottom);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, r->right - GetSystemMetrics(SM_CXHTHUMB),
                               r->bottom - GetSystemMetrics(SM_CYVTHUMB));
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_DrawLineTo(port, r->right, r->bottom - GetSystemMetrics(SM_CYVTHUMB));
        } else {
            e = DT_DrawLineTo(port, r->right, r->bottom);
        }
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_DrawLineTo(port, r->right, r->top + 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        DT_RectInset(r, -i, -i);
    }
    DT_RectInset(r, -1, -1);
    if (indent < 0)
        boxBack = &shade;
    else
        boxBack = &lit;
    e = DT_SetLineAttributes(port, boxBack, 0, gDT_PatternBlack, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (lineColor != 0 && gapLeft != gapRight) {
        e = DT_SetPixel(port, gapLeft + 1, r->top, portBack);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    DT_RectInset(r, 1, 1);
    for (i = 0; i != indent; i += STEP(indent)) {
        DT_RectInset(r, i, i);
        e = DT_SetPenLocation(port, gapLeft - i, r->top);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_DrawLineTo(port, r->left, r->top);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_DrawLineTo(port, r->left, r->bottom - 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_SetPenLocation(port, r->right - 1, r->top);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_DrawLineTo(port, gapRight + i, r->top);
        if (e != 0) {
            err = e;
            goto done;
        }
        DT_RectInset(r, -i, -i);
    }
    DT_RectInset(r, -1, -1);
done:
    if (port != 0)
        DT_ContextDispose(port);
    return err;
}
