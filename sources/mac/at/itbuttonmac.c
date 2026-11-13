/* itbuttonmac.c -- 1.0.10: the interface tool's BUTTON, IMAGE and IMG control on Mac: the image faces, a face drawn and the control procedure, a QuickDraw control where win makes a window. Notes: Research/notes/maclibs.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "ittoolkit.h"
#include <Appearance.h>
#include <ControlDefinitions.h>

#define kFacesPerValue      3

extern int32_t IT_ItemCallbackDrawRect(void *unused, const ITRect *area);              /* interfacetool.c */
extern int32_t IT_ItemCallbackGetImage(const char *name, ITImage **image, ITImage **mask,
                         ITTransparent *trans);                          /* interfacetool.c */
extern int32_t DT_SetFillAttributes(ITImage *image, const ITColor *fore, const ITColor *back,
                         const unsigned char *pattern);                  /* itgfx.c */
extern int32_t DT_DrawImage(ITImage *image, const ITRect *dstRect, ITImage *src, ITImage *mask,
                         const ITTransparent *trans, const ITRect *srcRect);  /* itgfx.c */
extern int32_t DT_FrameRect(ITImage *image, const ITRect *r);               /* itgfx.c */
extern int32_t DT_PaintRect(ITImage *image, const ITRect *r);               /* itgfx.c */
extern int32_t DT_InvertRect(ITImage *image, const ITRect *r);               /* itgfx.c */
extern unsigned char InputReadMouseState(void *hwnd, ITInputEvent *event);          /* itinputmac.c */
extern unsigned char gIT_HasAppearance;                                  /* atcursormac.c */
extern OSErr (*gIT_SetControlData)(ControlHandle control, short part, ResType tag, int32_t size, void *data);
extern CQDProcs gIT_StdCProcs;

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
    int32_t value;
    int32_t face;
    unsigned char active;
    unsigned char visible;
    unsigned char enabled;                       /* it has an HREF or a SCRIPT */
    unsigned char pad27;
    ControlHandle control;
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
#define kPressTicks         0x78
#define kCursorHand         5
#define kControlInactive    0xff
#define kControlDisabled    0xfe
#if TARGET_API_MAC_CARBON
#define SET_HILITE(h) do { \
        shown = IsControlVisible(button->control); \
        HiliteControl(button->control, (h)); \
        if ((int32_t)(UInt16)GetControlHilite(button->control) != (h)) \
            HiliteControl(button->control, (h)); \
        if (shown) \
            SetControlVisibility(button->control, true, false); \
    } while (0)
#define FLUSH_PORT(port) do { \
        if (QDIsPortBuffered(port)) { \
            if ((region = DT_RegionCreate()) != 0) \
                DT_SetRegionRect(region, &button->bounds); \
            QDFlushPortBuffer(port, region); \
            if (region != 0) \
                DT_RegionDispose(region); \
        } \
    } while (0)
#define TAIL_NAME face.name
#define TAIL_FIRST
#define COPY_NAME(d, s) do { \
        if (element->wide) \
            U2CStringCopySafe((unsigned char *)(d), (const unsigned short *)(s), sizeof(face.name), -1); \
        else \
            StringCopySafe((d), (s), sizeof(face.name)); \
    } while (0)
extern CFStringRef IndirectCopyCFString(void **handle);
extern RgnHandle DT_RegionCreate(void);
extern int32_t DT_SetRegionRect(void *region, const ITRect *r);
extern void DT_RegionDispose(void *region);
#else
#define SET_HILITE(h) ((*button->control)->contrlHilite = (h))
#define FLUSH_PORT(port)
#define TAIL_NAME name
#define TAIL_FIRST name = face.name
#define COPY_NAME(d, s) StringCopySafe((d), (s), sizeof(face.name))
#endif

int32_t IT_ButtonLoadFaces(void *hwnd, void *dc, ITElement *element, void ***faces)
{
    int32_t err;
    int32_t e;
    ITElement *image;
    char *value;
    ITButtonFace face;
    ITButtonFace normal;
    char *name;
    char *normalName;

    err = 0;
    value = 0;
    image = 0;
    *faces = 0;
    if (element->child != 0) {
        name = face.name;
        normalName = normal.name;
        while ((image = _IT_ParseTreeFindNextTag(element->child, image, "IMAGE", AsByte(image != 0))) != 0) {
            if (image == element)
                continue;
            if (*faces == 0) {
                if ((*faces = IndirectAllocate(0)) == 0) {
                    err = kOutOfMemory;
                    goto done;
                }
            }
            MemoryClear(&face, sizeof(face));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "SRC", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC", 0)) != 0)
                COPY_NAME(name, value);
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "SRC_RECT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC_RECT", 0)) != 0)
                _IT_ParseAttributeGetRect(&face.src, element->wide, value);
            MemoryCopy(&normal, &face, sizeof(face));
            e = IndirectAppendData(*faces, &face, sizeof(face));
            if (e != 0) {
                err = e;
                goto done;
            }
            MemoryClear(&face, sizeof(face));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "DIM", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM", 0)) != 0)
                COPY_NAME(name, value);
            else
                StringCopy(name, normalName);
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "DIM_RECT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM_RECT", 0)) != 0)
                _IT_ParseAttributeGetRect(&face.src, element->wide, value);
            else
                face.src = normal.src;
            e = IndirectAppendData(*faces, &face, sizeof(face));
            if (e != 0) {
                err = e;
                goto done;
            }
            MemoryClear(&face, sizeof(face));
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "LIT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT", 0)) != 0)
                COPY_NAME(name, value);
            else
                StringCopy(name, normalName);
            if ((value = _IT_ParseTreeFindAttribute(element, image, 0, "LIT_RECT", 0)) != 0
                || (value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT_RECT", 0)) != 0)
                _IT_ParseAttributeGetRect(&face.src, element->wide, value);
            else
                face.src = normal.src;
            e = IndirectAppendData(*faces, &face, sizeof(face));
            if (e != 0) {
                err = e;
                goto done;
            }
        }
    }
    if (*faces != 0 || (value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC", 0)) == 0)
        goto done;
    if ((*faces = IndirectAllocate(0)) == 0) {
        err = kOutOfMemory;
        goto done;
    }
    MemoryClear(&face, sizeof(face));
    TAIL_FIRST;
    COPY_NAME(TAIL_NAME, value);
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "SRC_RECT", 0)) != 0)
        _IT_ParseAttributeGetRect(&face.src, element->wide, value);
    MemoryCopy(&normal, &face, sizeof(face));
    e = IndirectAppendData(*faces, &face, sizeof(face));
    if (e != 0) {
        err = e;
        goto done;
    }
    MemoryClear(&face, sizeof(face));
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM", 0)) != 0)
        COPY_NAME(TAIL_NAME, value);
    else
        StringCopy(TAIL_NAME, normal.name);
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "DIM_RECT", 0)) != 0)
        _IT_ParseAttributeGetRect(&face.src, element->wide, value);
    else
        face.src = normal.src;
    e = IndirectAppendData(*faces, &face, sizeof(face));
    if (e != 0) {
        err = e;
        goto done;
    }
    MemoryClear(&face, sizeof(face));
    if ((value = _IT_ParseTreeFindAttribute(element, element, 0, "LIT", 0)) != 0)
        COPY_NAME(TAIL_NAME, value);
    else
        StringCopy(TAIL_NAME, normal.name);
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

static const ITColor kPlaceholderFore = { 0xff, 0xc0, 0x00, 0x00 };
static const ITColor kPlaceholderBack = { 0xff, 0xff, 0xff, 0xff };
static const ITColor kPlaceholderFrame = { 0xff, 0x00, 0x00, 0x00 };

int32_t _IT_ButtonDraw(void *dc, ITImage *image, const ITRect *r, void **faces, int32_t value,
                  ULong variant)
{
    int32_t err;
    int32_t e;
    unsigned char made;
    ULong count;
    ITImage *picture;
    ITImage *mask;
    ITTransparent trans;
    ITButtonFace face;

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
        if (count != 0 && (value * kFacesPerValue + variant) < count) {
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
        ITColor fore;
        ITColor back;
        ITColor frame;

        fore = kPlaceholderFore;
        back = kPlaceholderBack;
        frame = kPlaceholderFrame;
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

int32_t IT_ButtonHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    ITButton *button;
    int32_t err;
    int32_t e;
    unsigned char update;
#if TARGET_API_MAC_CARBON
    unsigned char wide;
#endif
    unsigned char now;
    char *value;
    ULong length;
    int32_t kind;
    ULong maximum;
    ITPoint *where;
    void *owner;
    void *procs;
    unsigned char prev;
    short hilite;
    int32_t on;
    char text[256];
    Str255 name;
    char digits[256];
    ITInputEvent event;
    Rect r;
    Point pt;
    CGrafPtr savedPort;
    GDHandle savedDevice;
    int32_t number;
    Boolean flag;
#if TARGET_API_MAC_CARBON
    CFStringRef string;
    Boolean shown;
    void *region;
#endif

    button = *data;
    err = 0;
    update = 0;
#if TARGET_API_MAC_CARBON
    string = 0;
    wide = element->wide;
#endif
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    GetGWorld(&savedPort, &savedDevice);
    if (dc != 0)
        SetGWorld(dc, GetMainDevice());
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITButton));
        if (e != 0) {
            err = e;
            goto done;
        }
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
#if TARGET_API_MAC_CARBON
            if (wide)
                value = (char *)U2CStringCopySafe((unsigned char *)text, (const unsigned short *)value, sizeof(text), -1);
#endif
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
#if TARGET_API_MAC_CARBON
        if (value != 0) {
            if (wide)
                value = (char *)U2CStringCopySafe((unsigned char *)text, (const unsigned short *)value, sizeof(text), -1);
            if (StringFindDelimiter(value, "+") != 0)
                button->isDefault = 1;
        }
#else
        if (value != 0 && StringFindDelimiter(value, "+") != 0)
            button->isDefault = 1;
#endif
        if (IT_ItemCallbackGetFlag("HOVER"))
            button->hover = 1;
        break;
    case kMsgOpen:
        e = IT_ButtonLoadFaces(hwnd, dc, element, &button->faces);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (button->faces != 0)
            break;
        kind = pushButProc;
        maximum = 0;
        if (button->checkbox) {
            kind = checkBoxProc;
            maximum = 1;
        }
        if (button->radio) {
            kind = radioButProc;
            maximum = 1;
        }
#if TARGET_API_MAC_CARBON
        name[0] = 0;
        if (!IndirectIsUnicode(button->title, 0))
#endif
        {
            length = IndirectGetSize(button->title) + 1;
            if (length > kTextMax - 1)
                length = kTextMax - 1;
            StringCopySafe(text, *button->title, length);
            StringC2PCopy(name, text);
        }
        CopyDT_2SysRect(&r, &button->bounds);
        if (button->value > maximum)
            button->value = maximum;
        if ((button->control = NewControl(hwnd, &r, name, button->visible, button->value, 0, maximum,
                                          kind, 0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
#if TARGET_API_MAC_CARBON
        if (IndirectIsUnicode(button->title, 0)) {
            if ((string = IndirectCopyCFString(button->title)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = SetControlTitleWithCFString(button->control, string);
            if (e != 0)
                err = e;
            CFRelease(string);
            string = 0;
        }
#endif
        if (button->isDefault && !button->checkbox && !button->radio && gIT_HasAppearance) {
            flag = 1;
#if TARGET_API_MAC_CARBON
            e = SetControlData(button->control, 0, kControlPushButtonDefaultTag, sizeof(flag), &flag);
#else
            e = (*gIT_SetControlData)(button->control, 0, kControlPushButtonDefaultTag, sizeof(flag), &flag);
#endif
            if (e != 0)
                err = e;
        }
        update = 1;
        break;
    case kMsgClose:
        if (button->control != 0)
            DisposeControl(button->control);
        button->control = 0;
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
        if (button->faces == 0 && button->control == (ControlHandle)a)
            *(unsigned char *)b = 1;
        break;
    case kMsgClick:
        where = &((ITInputEvent *)a)->where;
        CopyDT_2SysPoint(&pt, where);
        if (button->faces != 0) {
            *(unsigned char *)b = DT_IsPointInRect(where, &button->bounds);
            break;
        }
        if (TestControl(button->control, pt) != 0)
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        if (button->faces != 0) {
            on = 0;
            if (button->enabled && IT_ItemCallbackIsViewEnabled())
                on = 1;
            e = _IT_ButtonDraw(dc, 0, &button->bounds, button->faces, button->value,
                             on ? button->face : kFaceDim);
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
#if TARGET_API_MAC_CARBON
            procs = GetPortGrafProcs(dc);
            SetPortGrafProcs(dc, &gIT_StdCProcs);
            DrawControlInCurrentPort(button->control);
            SetPortGrafProcs(dc, procs);
#else
            owner = (*button->control)->contrlOwner;
            (*button->control)->contrlOwner = dc;
            procs = ((CGrafPtr)dc)->grafProcs;
            ((CGrafPtr)dc)->grafProcs = &gIT_StdCProcs;
            Draw1Control(button->control);
            ((CGrafPtr)dc)->grafProcs = procs;
            (*button->control)->contrlOwner = owner;
#endif
        }
        break;
    case kMsgTrack:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (button->faces == 0) {
            if (TrackControl(button->control, pt, 0) != 0)
                *(unsigned char *)b = 1;
            break;
        }
        if (!button->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        IT_ItemCallbackSoundEvent(kEventPressed);
        button->face = kFaceLit;
        e = IT_ItemCallbackDrawRect(dc, &button->bounds);
        if (e != 0) {
            err = e;
            goto done;
        }
        where = &event.where;
        prev = now = 1;
        while (InputReadMouseState(hwnd, &event) && (event.modifiers & kModButton)) {
            now = DT_IsPointInRect(where, &button->bounds);
            if (now != prev) {
                button->face = now ? kFaceLit : kFaceNormal;
                e = IT_ItemCallbackDrawRect(dc, &button->bounds);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                IT_ItemCallbackSoundEvent(now ? kEventEntered : kEventLeft);
            }
            prev = now;
        }
        IT_ItemCallbackSoundEvent(kEventReleased);
        if (!now)
            break;
        button->face = kFaceNormal;
        e = IT_ItemCallbackDrawRect(dc, &button->bounds);
        if (e != 0) {
            err = e;
            goto done;
        }
        *(unsigned char *)b = 1;
        break;
    case kMsgPress:
        if (button->faces == 0) {
#if TARGET_API_MAC_CARBON
            hilite = GetControlHilite(button->control);
#else
            hilite = (*button->control)->contrlHilite;
#endif
            HiliteControl(button->control, kControlButtonPart);
            SystemWait(kPressTicks);
            HiliteControl(button->control, hilite);
            break;
        }
        if (!button->enabled || !IT_ItemCallbackIsViewEnabled())
            break;
        IT_ItemCallbackSoundEvent(kEventPressed);
        e = _IT_ButtonDraw(dc, 0, &button->bounds, button->faces, button->value, kFaceLit);
        if (e != 0) {
            err = e;
            goto done;
        }
        FLUSH_PORT(dc);
        SystemWait(kPressTicks);
        IT_ItemCallbackSoundEvent(kEventReleased);
        e = _IT_ButtonDraw(dc, 0, &button->bounds, button->faces, button->value, kFaceNormal);
        if (e != 0) {
            err = e;
            goto done;
        }
        FLUSH_PORT(dc);
        break;
    case kMsgCursor:
        if (button->faces != 0 && button->visible && button->enabled)
            *(short *)b = kCursorHand;
        else
            *(short *)b = 0;
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        button->active = AsByte(msg == kMsgActivate);
        if (button->control != 0)
            update = 1;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = button->bounds;
        break;
    case kMsgBounds:
        button->bounds = *(const ITRect *)a;
        CopyDT_2SysRect(&r, &button->bounds);
        if (button->control != 0) {
#if TARGET_API_MAC_CARBON
            shown = IsControlVisible(button->control);
            if (shown)
                SetControlVisibility(button->control, false, false);
            SetControlBounds(button->control, &r);
            if (shown)
                SetControlVisibility(button->control, true, false);
#else
            (*button->control)->contrlRect = r;
#endif
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
        if (e != 0) {
            err = e;
            goto done;
        }
        if (button->control == 0)
            break;
#if TARGET_API_MAC_CARBON
        shown = IsControlVisible(button->control);
        if (shown)
            SetControlVisibility(button->control, false, false);
        if (IndirectIsUnicode(button->title, 0)) {
            if ((string = IndirectCopyCFString(button->title)) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            e = SetControlTitleWithCFString(button->control, string);
            if (e != 0)
                err = e;
            CFRelease(string);
            string = 0;
        } else {
            length = IndirectGetSize(button->title) + 1;
            if (length > kTextMax - 1)
                length = kTextMax - 1;
            StringCopySafe(text, *button->title, length);
            SetControlTitle(button->control, StringC2PCopy(name, text));
        }
        if (shown)
            SetControlVisibility(button->control, true, false);
#else
        length = IndirectGetSize(button->title) + 1;
        if (length > kTextMax - 1)
            length = kTextMax - 1;
        StringCopySafe(text, *button->title, length);
        StringC2PCopy((*button->control)->contrlTitle, text);
#endif
        break;
    case kMsgGetText:
        e = IT_ButtonHandler(hwnd, dc, element, data, kMsgGetValue, 0, &number);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (button->control != 0) {
            StringFromNumber(digits, number);
            e = IndirectReplaceData(b, digits, StringGetLength((unsigned char *)digits));
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            e = IndirectSetSize(b, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        break;
    case kMsgSetText:
        err = kUnsupported;
        goto done;
    case kMsgGetValue:
        *(int32_t *)b = button->value;
        if (button->control != 0)
            *(int32_t *)b = GetControlValue(button->control);
        break;
    case kMsgSetValue:
        button->value = a;
        if (button->control != 0) {
#if TARGET_API_MAC_CARBON
            shown = IsControlVisible(button->control);
            if (shown)
                SetControlVisibility(button->control, false, false);
            SetControlValue(button->control, button->value != 0);
            if (shown)
                SetControlVisibility(button->control, true, false);
#else
            (*button->control)->contrlValue = button->value != 0;
#endif
        }
        break;
    case kMsgVisible:
        *(unsigned char *)b = button->visible;
        break;
    case kMsgShow:
        button->visible = AsByte(a != 0);
        if (button->control != 0)
#if TARGET_API_MAC_CARBON
            SetControlVisibility(button->control, button->visible, false);
#else
            (*button->control)->contrlVis = button->visible ? 0xff : 0;
#endif
        break;
    case kMsgEnabled:
        *(unsigned char *)b = button->enabled;
        break;
    case kMsgEnable:
        button->enabled = AsByte(a != 0);
        if (button->control != 0)
            update = 1;
        break;
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:
        break;
    }
    if (update) {
        if (!button->active) {
            SET_HILITE(kControlInactive);
        } else if (!button->enabled || !IT_ItemCallbackIsViewEnabled()) {
            SET_HILITE(kControlDisabled);
        } else {
            SET_HILITE(0);
        }
    }
done:
#if TARGET_API_MAC_CARBON
    if (string != 0)
        CFRelease(string);
#endif
    SetGWorld(savedPort, savedDevice);
    return err;
}
