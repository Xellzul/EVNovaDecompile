// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* ittoolkit2.c -- the interface tool's BUTTON, IMAGE and IMG control: the image faces, a face drawn and the control procedure (0x0051c936..0x0051db65). Notes: Research/notes/ittoolkit.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"

#define kFacesPerValue      3

extern long IT_ItemCallbackDrawRect(void *unused, const ITRect *area);              /* interfacetool.c */

extern long IT_ItemCallbackGetImage(const char *name, ITImage **image, ITImage **mask,
                         ITTransparent *trans);                          /* interfacetool.c */

extern long DT_SetFillAttributes(ITImage *image, const ITColor *fore, const ITColor *back,
                         const unsigned char *pattern);                  /* itgfx.c */

extern long DT_DrawImage(ITImage *image, const ITRect *dstRect, ITImage *src, ITImage *mask,
                         const ITTransparent *trans, const ITRect *srcRect);  /* itgfx.c */

extern long DT_FrameRect(ITImage *image, const ITRect *r);               /* itgfx.c */

extern long DT_PaintRect(ITImage *image, const ITRect *r);               /* itgfx.c */

extern long DT_InvertRect(ITImage *image, const ITRect *r);               /* itgfx.c */


typedef struct ITButtonFace {
    ITRect src;
    char name[256];
} ITButtonFace;

typedef struct ITButton {
    void **title;
    void **faces;                                /* ITButtonFace, kFacesPerValue per value */
    unsigned char isDefault;                     /* KEYS holds `+': Return presses it */
    unsigned char hover;
    unsigned char checkbox;
    unsigned char radio;
    ITRect bounds;
    long value;
    long face;
    unsigned char active;
    unsigned char visible;
    unsigned char enabled;                       /* it has an HREF or a SCRIPT */
    unsigned char pad27;
    void *hwnd;
} ITButton;

#define kMsgTrack           0x15
#define kMsgCursor          0x18
#define kFaceNormal         0
#define kFaceDim            1
#define kFaceLit            2
#define kEventPressed       12              /* to the window callback, about the control */
#define kEventLeft          13
#define kEventEntered       14
#define kEventReleased      15
#define kModButton          0x10000000      /* ITInputEvent.modifiers: the button is down */
#define WS_CHILD_VISIBLE    0x50000000UL
#define BS_DEFPUSHBUTTON    0x0001
#define BS_CHECKBOX         0x0002
#define BS_RADIOBUTTON      0x0004
#define BM_GETCHECK         0x00f0
#define BM_SETCHECK         0x00f1
#define BST_CHECKED         1

/* IT_ButtonLoadFaces (an image button's faces: for every IMAGE element inside the control, else
   the control itself when it has a SRC, three records -- SRC, DIM, LIT with their _RECTs,
   each falling back to the control's own attribute, a missing DIM / LIT to the SRC face).
   STATE: MATCH 100%, exe 483 / obj 483, byte-identical yes. OPEN: the first two arguments are
   never read */
// FUNCTION: EVNOVA 0x0051c936
long IT_ButtonLoadFaces(void *hwnd, void *dc, ITElement *element, void ***faces)
{
    long err;
    long e;
    ITElement *image;
    char *value;
    ITButtonFace face;
    ITButtonFace normal;

    err = 0;
    value = 0;
    image = 0;
    *faces = 0;
    if (element->child != 0) {
        while ((image = _IT_ParseTreeFindNextTag(element->child, image, "IMAGE", AsByte(image != 0))) != 0) {
            if (image == element)
                continue;
            if (*faces == 0) {
                *faces = IndirectAllocate(0);
                if (*faces == 0)
                    goto nomem;
            }
            MemoryClear(&face, sizeof(face));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "SRC", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC", 0)) != 0)
                StringCopySafe(face.name, value, sizeof(face.name));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "SRC_RECT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC_RECT", 0)) != 0)
                _IT_ParseAttributeGetRect(&face.src, element->wide, value);
            MemoryCopy(&normal, &face, sizeof(face));
            e = IndirectAppendData(*faces, &face, sizeof(face));
            if (e != 0)
                goto fail;
            MemoryClear(&face, sizeof(face));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "DIM", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM", 0)) != 0)
                StringCopySafe(face.name, value, sizeof(face.name));
            else
                StringCopy(face.name, normal.name);
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "DIM_RECT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM_RECT", 0)) != 0)
                _IT_ParseAttributeGetRect(&face.src, element->wide, value);
            else
                face.src = normal.src;
            e = IndirectAppendData(*faces, &face, sizeof(face));
            if (e != 0)
                goto fail;
            MemoryClear(&face, sizeof(face));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "LIT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT", 0)) != 0)
                StringCopySafe(face.name, value, sizeof(face.name));
            else
                StringCopy(face.name, normal.name);
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "LIT_RECT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT_RECT", 0)) != 0)
                _IT_ParseAttributeGetRect(&face.src, element->wide, value);
            else
                face.src = normal.src;
            e = IndirectAppendData(*faces, &face, sizeof(face));
            if (e != 0)
                goto fail;
        }
    }
    if (*faces != 0 || (value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC", 0)) == 0)
        goto done;
    *faces = IndirectAllocate(0);
    if (*faces == 0) {
nomem:
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(&face, sizeof(face));
    StringCopySafe(face.name, value, sizeof(face.name));
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC_RECT", 0)) != 0)
        _IT_ParseAttributeGetRect(&face.src, element->wide, value);
    MemoryCopy(&normal, &face, sizeof(face));
    e = IndirectAppendData(*faces, &face, sizeof(face));
    if (e != 0)
        goto fail;
    MemoryClear(&face, sizeof(face));
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM", 0)) != 0)
        StringCopySafe(face.name, value, sizeof(face.name));
    else
        StringCopy(face.name, normal.name);
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM_RECT", 0)) != 0)
        _IT_ParseAttributeGetRect(&face.src, element->wide, value);
    else
        face.src = normal.src;
    e = IndirectAppendData(*faces, &face, sizeof(face));
    if (e != 0) {
fail:
        err = e;
        goto done;
    }
    MemoryClear(&face, sizeof(face));
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT", 0)) != 0)
        StringCopySafe(face.name, value, sizeof(face.name));
    else
        StringCopy(face.name, normal.name);
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT_RECT", 0)) != 0)
        _IT_ParseAttributeGetRect(&face.src, element->wide, value);
    else
        face.src = normal.src;
    e = IndirectAppendData(*faces, &face, sizeof(face));
    if (e != 0)
        err = e;
done:
    if (err != 0 && *faces != 0) {
        IndirectDeallocate(*faces);
        *faces = 0;
    }
    return err;
}

static const ITColor kPlaceholder[3] = {
    { 0xff, 0xc0, 0x00, 0x00 }, { 0xff, 0xff, 0xff, 0xff }, { 0xff, 0x00, 0x00, 0x00 }
};

/* _IT_ButtonDraw (draw an image button's face `variant' for `value' into `r' of `image', or of an
   image made over `dc' when none is given; a missing face image draws the placeholder, inverted
   when the variant is not the normal one). STATE: MATCH 100%, exe 176 / obj 176, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x0051ceb0
long _IT_ButtonDraw(void *dc, ITImage *image, const ITRect *r, void **faces, long value,
                  long variant)
{
    long err;
    long e;
    unsigned char made;
    ULong count;
    ITImage *picture;
    ITImage *mask;
    ITTransparent trans;
    ITButtonFace face;
    ITColor fore;
    ITColor back;
    ITColor frame;

    err = 0;
    made = 0;
    picture = 0;
    mask = 0;
    count = 0;
    if (image == 0) {
        image = DT_ContextCreateRaw(dc, 0);
        if (image == 0) {
            err = kOutOfMemory;
            goto done;
        }
        made = 1;
    }
    if (faces != 0) {
        count = IndirectGetSize(faces) / sizeof(ITButtonFace);
        if (count != 0 && (ULong)(value * kFacesPerValue + variant) < count) {
            MemoryCopy(&face, (ITButtonFace *)*faces + value * kFacesPerValue + variant,
                         sizeof(ITButtonFace));
            err = IT_ItemCallbackGetImage(face.name, &picture, &mask, &trans);
            if (err == kFTErrFileNotFound)
                err = 0;
            e = err;
            if (err != 0)
                goto done;
        }
    }
    if (picture != 0) {
        if (DT_IsRectEmpty(&face.src))
            DT_ImageGetRect(picture, &face.src);
        e = DT_DrawImage(image, r, picture, mask, &trans, &face.src);
        if (e != 0) {
            err = e;
            goto done;
        }
    } else {
        fore = kPlaceholder[0];
        back = kPlaceholder[1];
        frame = kPlaceholder[2];
        e = DT_SetFillAttributes(image, &fore, &back, gDT_PatternLtGray);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_PaintRect(image, r);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_SetLineAttributes(image, &frame, &back, gDT_PatternBlack, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = DT_FrameRect(image, r);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (variant != kFaceNormal) {
            e = DT_InvertRect(image, r);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        e = DT_ResetAttributes(image, 1, 1, 0, 0, 0);
        if (e != 0)
            err = e;
    }
done:
    if (made && image != 0)
        DT_ContextDispose(image);
    return err;
}

/* IT_ButtonHandler (the BUTTON, IMAGE and IMG control proc: 0 makes its data (KIND checkbox or
   radio, VALUE, HREF / SCRIPT make it clickable, KEYS `+' makes it the default, HOVER), 1
   builds the image faces or else creates a Windows button, 2 destroys it, 3 frees the data;
   an image button tracks the mouse itself, lit while pressed inside, and reports pressed /
   left / entered / released to the window's callback; value, title, show and enable).
   STATE: MATCH 100.0%, exe 826 / obj 826, byte-identical yes. OPEN: none; the five reply locals
   are dead (no code), kept for their IR entries (notes/itbutton.md) */
// FUNCTION: EVNOVA 0x0051d0d3
long IT_ButtonHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    ITButton *button;
    long err;
    long e;
    unsigned char dead;
    long spare;
    long size;
    long size2;
    long reply;
    long reply2;
    long reply3;
    long reply4;
    long reply5;
    unsigned char update;
    unsigned char now;
    unsigned char prev;
    char *value;
    ULong length;
    ULong style;
    POINT pt;
    ITRect r;
    char text[256];
    POINT origin;
    ITInputEvent event;
    long number;
    char digits[256];

    button = *data;
    dead = 0;
    err = 0;
    update = 0;
    spare = 0;
    if (msg == kMsgCreate)
        size = IndirectGetSize(data);
    if (msg != kMsgCreate)
        size2 = IndirectGetSize(data);
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITButton));
        if (e != 0)
            goto fail;
        IndirectSetLock(data, 1);
        button = *data;
        MemoryClear(*data, sizeof(ITButton));
        if ((button->title = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        button->checkbox = 0;
        button->radio = 0;
        button->enabled = 0;
        button->visible = 1;
        button->active = 1;
        button->value = 0;
        button->face = kFaceNormal;
        value = IT_ItemCallbackGetParam("KIND", 0);
        if (value != 0) {
            String2UpperCopySafe(text, value, sizeof(text));
            if (StringCompare(text, "CHECKBOX") == 0 || StringCompare(text, "CHECK") == 0)
                button->checkbox = 1;
            if (StringCompare(text, "RADIOBUTTON") == 0 || StringCompare(text, "RADIO") == 0)
                button->radio = 1;
        }
        value = IT_ItemCallbackGetParam("VALUE", 0);
        if (value != 0)
            button->value = IT_ItemCallbackGetNumber(value);
        if (IT_ItemCallbackGetParam("HREF", 0) != 0 || IT_ItemCallbackGetParam("SCRIPT", 0) != 0)
            button->enabled = 1;
        value = IT_ItemCallbackGetParam("KEYS", 0);
        if (value != 0 && StringFindDelimiter(value, "+") != 0)
            button->isDefault = 1;
        if (IT_ItemCallbackGetFlag("HOVER"))
            button->hover = 1;
        break;
    case kMsgOpen:
        e = IT_ButtonLoadFaces(hwnd, dc, element, &button->faces);
        if (e != 0)
            goto fail;
        if (button->faces != 0)
            break;
        style = 0;
        if (button->checkbox)
            style = BS_CHECKBOX;
        if (button->radio)
            style = BS_RADIOBUTTON;
        length = IndirectGetSize(button->title) + 1;
        if (length > kTextMax - 1)
            length = kTextMax - 1;
        StringCopySafe(text, *button->title, length);
        if ((button->hwnd = CreateWindowExA(0, "BUTTON", text,
                                            style | WS_CHILD_VISIBLE |
                                            ((!button->checkbox && !button->radio &&
                                              button->isDefault) ? BS_DEFPUSHBUTTON : 0),
                                            button->bounds.left, button->bounds.top,
                                            button->bounds.right - button->bounds.left,
                                            button->bounds.bottom - button->bounds.top, hwnd, 0,
                                            (void *)GetWindowLongA(hwnd, GWL_HINSTANCE), 0)) == 0)
            goto failed;
        if (button->checkbox || button->radio)
            SendMessageA(button->hwnd, BM_SETCHECK, button->value != 0 ? 1 : 0, 0);
        update = 1;
        break;
    case kMsgClose:
        if (button->hwnd != 0)
            DestroyWindow(button->hwnd);
        button->hwnd = 0;
        if (button->faces != 0) {
            IndirectDeallocate(button->faces);
            button->faces = 0;
        }
        break;
    case kMsgDispose:
        if (button->title != 0) {
            IndirectDeallocate(button->title);
            button->title = 0;
        }
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
        if (button->faces == 0 && button->hwnd == (void *)a)
            *(unsigned char *)b = 1;
        break;
    case kMsgClick:
        CopyDT_2SysPoint((ITPoint *)&pt, &((ITInputEvent *)a)->where);
        if (button->faces != 0) {
            *(unsigned char *)b = DT_IsPointInRect(&((ITInputEvent *)a)->where, &button->bounds);
            break;
        }
        if (!IsWindowVisible(button->hwnd))
            break;
        if (button->hwnd == ChildWindowFromPoint(hwnd, pt))
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        if (button->faces != 0) {
            e = _IT_ButtonDraw(dc, 0, &button->bounds, button->faces, button->value,
                             button->enabled && IT_ItemCallbackIsViewEnabled() ? button->face : kFaceDim);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else if (button->hwnd != 0) {
            void *windowDC;

            windowDC = GetDC(hwnd);
            SetViewportOrgEx(dc, button->bounds.left, button->bounds.top, &origin);
            SendMessageA(button->hwnd, dc == windowDC ? WM_PAINT : WM_PRINT,
                         (unsigned int)dc, PRF_CHECKVISIBLE | PRF_CLIENT);
            SetViewportOrgEx(dc, origin.x, origin.y, &origin);
            ReleaseDC(hwnd, windowDC);
        }
        break;
    case 0x13:
        if (button->faces == 0)
            err = err;
        break;
    case 0x14:
        if (button->faces == 0)
            err = err;
        break;
    case kMsgTrack:
        CopyDT_2SysPoint((ITPoint *)&pt, &((ITInputEvent *)a)->where);
        if (button->faces == 0 || !button->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        reply = IT_ItemCallbackSoundEvent(kEventPressed);
        button->face = kFaceLit;
        e = IT_ItemCallbackDrawRect(dc, &button->bounds);
        if (e != 0)
            goto fail;
        prev = now = 1;
        while (InputReadMouseState(hwnd, &event) && (event.modifiers & kModButton)) {
            now = DT_IsPointInRect(&event.where, &button->bounds);
            if (now != prev) {
                button->face = now ? kFaceLit : kFaceNormal;
                e = IT_ItemCallbackDrawRect(dc, &button->bounds);
                if (e != 0)
                    goto fail;
                reply2 = IT_ItemCallbackSoundEvent(now ? kEventEntered : kEventLeft);
            }
            prev = now;
        }
        reply3 = IT_ItemCallbackSoundEvent(kEventReleased);
        if (!now)
            break;
        button->face = kFaceNormal;
        e = IT_ItemCallbackDrawRect(dc, &button->bounds);
        if (e != 0)
            goto fail;
        *(unsigned char *)b = 1;
        break;
    case 0x16:
        break;
    case kMsgPress:
        if (button->faces == 0) {
            SendMessageA(button->hwnd, BM_SETSTATE, 1, 0);
            SystemWait(kPressMs);
            SendMessageA(button->hwnd, BM_SETSTATE, 0, 0);
            break;
        }
        if (!button->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        reply4 = IT_ItemCallbackSoundEvent(kEventPressed);
        e = _IT_ButtonDraw(dc, 0, &button->bounds, button->faces, button->value, kFaceLit);
        if (e != 0)
            goto fail;
        SystemWait(kPressMs);
        reply5 = IT_ItemCallbackSoundEvent(kEventReleased);
        e = _IT_ButtonDraw(dc, 0, &button->bounds, button->faces, button->value, kFaceNormal);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kMsgCursor:
        if (button->faces != 0 && button->visible && button->enabled)
            *(short *)b = IDC_HAND;
        else
            *(short *)b = 0;
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
        button->active = AsByte(msg == kMsgActivate);
        if (button->hwnd != 0)
            update = 1;
        break;
    case 0x32:
    case 0x33:
        break;
    case kMsgGetBounds:
        *(ITRect *)b = button->bounds;
        break;
    case kMsgBounds:
        button->bounds = *(const ITRect *)a;
        CopyDT_2SysRect(&r, &button->bounds);
        if (button->hwnd != 0
            && !MoveWindow(button->hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 1)) {
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        break;
    case kMsgGetTitle:
        e = IndirectReplace(b, button->title);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kMsgTitle:
        e = IndirectReplace(button->title, (void **)a);
        if (e != 0)
            goto fail;
        if (button->hwnd == 0)
            break;
        length = IndirectGetSize(button->title) + 1;
        if (length > kTextMax - 1)
            length = kTextMax - 1;
        StringCopySafe(text, *button->title, length);
        if (!SetWindowTextA(button->hwnd, text)) {
failed:
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        break;
    case kMsgGetText:
        e = IT_ButtonHandler(hwnd, dc, element, data, kMsgGetValue, 0, &number);
        if (e != 0)
            goto fail;
        if (button->hwnd != 0) {
            StringFromNumber(digits, number);
            e = IndirectReplaceData(b, digits, StringGetLength((unsigned char *)digits));
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = IndirectSetSize(b, 0);
            if (e != 0) {
fail:
                err = e;
                goto done;
            }
        }
        break;
    case kMsgSetText:
        err = kUnsupported;
        goto done;
    case kMsgGetValue:
        *(long *)b = button->value;
        if (button->hwnd != 0)
            *(long *)b = SendMessageA(button->hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
        break;
    case kMsgSetValue:
        button->value = a;
        if (button->hwnd != 0)
            SendMessageA(button->hwnd, BM_SETCHECK, button->value != 0 ? 1 : 0, 0);
        break;
    case kMsgVisible:
        *(unsigned char *)b = button->visible;
        break;
    case kMsgShow:
        button->visible = AsByte(a != 0);
        if (button->hwnd != 0)
            ShowWindow(button->hwnd, button->visible ? SW_SHOW : SW_HIDE);
        break;
    case kMsgEnabled:
        *(unsigned char *)b = button->enabled;
        break;
    case kMsgEnable:
        button->enabled = AsByte(a != 0);
        if (button->hwnd != 0)
            update = 1;
        break;
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:
        break;
    }
    if (update)
        if (!(button->active && button->enabled && IT_ItemCallbackIsViewEnabled())) {
            EnableWindow(button->hwnd, 0);
            return err;
        } else {
            EnableWindow(button->hwnd, 1);
            return err;
        }
done:
    return err;
}
