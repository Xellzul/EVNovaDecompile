/* itframemac.c -- 1.0.10: the interface tool's FRAME control on QuickDraw: a view inside a view, clipped to its bounds. Notes: Research/notes/maclibs.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"

extern long IT_ViewCreate(void *hwnd, const ITRect *bounds, ITWindowProc proc, long refcon,
                         long unused, WindowNode **out);                 /* ittoolkit.c */
extern long _IT_ViewHandleEdit(WindowNode *window, short event, ITClick *click);  /* ittoolkit.c */
extern long _IT_ViewGetFocus(WindowNode *window, ITControl **control, long *index,
                         long *item);                                    /* ittoolkit.c */
extern long _IT_ViewInvalidate(WindowNode *window);                            /* ittoolkit.c */
extern long _IT_ViewSetRect(WindowNode *window, const ITRect *bounds);      /* ittoolkit.c */
extern long _IT_ViewSetEnabled(WindowNode *window, unsigned char visible,
                         unsigned char enabled);                         /* ittoolkit.c */

/* the control's data handle */
#pragma options align=mac68k
typedef struct ITFrame {
    WindowNode *window;                          /* +0x00 the frame's own window */
    WindowNode *shown;                           /* +0x04 the same, once its page is loaded */
    ITRect bounds;                               /* +0x08 */
    unsigned char active;                        /* +0x18 */
    unsigned char visible;                       /* +0x19 */
} ITFrame;
#pragma options align=reset


#if TARGET_API_MAC_CARBON
#define kEmptyW L""
#endif

long IT_FrameHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    ITControl *focused;
    char *file;
    ITFrame *frame;
    char *link;
    char *anchor;
    long err;
    long e;
    Point pt;
    ITClick click;
    CGrafPtr savePort;
    GDHandle saveDevice;
    RgnHandle saveClip;
    RgnHandle clip;
#if TARGET_API_MAC_CARBON
    unsigned char wide = element->wide;
    char text[256];
#endif

    frame = *data;
    err = 0;
    saveClip = 0;
#if TARGET_API_MAC_CARBON
    clip = 0;
#endif
    focused = 0;
    file = 0;
    anchor = 0;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    GetGWorld(&savePort, &saveDevice);
    if (dc != 0) {
        SetGWorld(dc, GetMainDevice());
        if (msg > kMsgDispose) {
            if ((saveClip = NewRgn()) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            GetClip(saveClip);
#if TARGET_API_MAC_CARBON
            if ((clip = NewRgn()) == 0) {
                err = kOutOfMemory;
                goto done;
            }
            SetRectRgn(clip, frame->bounds.left + 1, frame->bounds.top + 1,
                       frame->bounds.right, frame->bounds.bottom);
            SectRgn(saveClip, clip, clip);
            SetClip(clip);
#else
            SetRectRgn(((CGrafPtr)dc)->clipRgn, frame->bounds.left + 1, frame->bounds.top + 1,
                       frame->bounds.right, frame->bounds.bottom);
            SectRgn(saveClip, ((CGrafPtr)dc)->clipRgn, ((CGrafPtr)dc)->clipRgn);
#endif
        }
    }
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITFrame));
        if (e != 0) {
            err = e;
            break;
        }
        IndirectSetLock(data, 1);
        frame = *data;
        MemoryClear(*data, sizeof(ITFrame));
        break;
    case kMsgOpen:
        b = 0;
        link = IT_ItemCallbackGetParam("SOURCE", 0);
        if (link == 0)
            link = IT_ItemCallbackGetParam("SRC", 0);
#if TARGET_API_MAC_CARBON
        if (wide)
            link = (char *)U2CStringCopySafe((unsigned char *)text, link != 0 ? (const unsigned short *)link : kEmptyW, sizeof(text), -1);
#endif
        e = _IT_ViewDissectLink(gIT_CurrentView, link, &file, &anchor);
        if (e != 0) {
            err = e;
            break;
        }
        e = IT_ViewCreate(hwnd, &frame->bounds, gIT_CurrentView->proc, gIT_CurrentView->refcon, 0,
                         &frame->window);
        if (e != 0) {
            err = e;
            break;
        }
        e = IT_ViewChange(frame->window, gIT_CurrentView->source.folder, file,
                         gIT_CurrentView->path, anchor);
        if (e != 0) {
            err = e;
            break;
        }
        frame->shown = frame->window;
        frame->visible = 1;
        break;
    case kMsgClose:
        if (frame->window != 0) {
            IT_ViewDispose(frame->window);
            frame->window = 0;
        }
        break;
    case kMsgDispose:
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
        e = _IT_ViewHandleTest(frame->shown, a, b);
        if (e != 0)
            err = e;
        break;
    case kMsgClick:
        if (!DT_IsRectEmpty(&frame->bounds) &&
            DT_IsPointInRect(&((ITInputEvent *)a)->where, &frame->bounds))
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        if (frame->window != 0) {
            WindowNode *window = gIT_CurrentView;
            ITControl *control = gIT_CurrentItem;
            gIT_CurrentView = 0;
            gIT_CurrentItem = 0;
            err = _IT_ViewHandleDraw(frame->shown, (const ITRect *)a, 1);
            gIT_CurrentView = window;
            gIT_CurrentItem = control;
            if (err != 0)
                e = err;
        }
        break;
    case kMsgIdle:
        if (frame->window != 0) {
            e = _IT_ViewHandleIdle(frame->shown);
            if (e != 0)
                err = e;
        }
        break;
    case 0x14:
        break;
    case kMsgWhat:
        CopyDT_2SysPoint(&pt, &((ITInputEvent *)a)->where);
        if (frame->window != 0) {
            e = _IT_ViewHandleClick(frame->shown, (ITInputEvent *)a, b);
            if (e != 0)
                err = e;
        }
        break;
    case kMsgKey:
        if (frame->window != 0) {
            e = _IT_ViewHandleKey(frame->shown, (const ITKeyEvent *)a, b);
            if (e != 0)
                err = e;
        }
        break;
    case 0x17:
        break;
    case kMsgMouse:
        if (frame->window != 0) {
            e = _IT_ViewHandleCursor(frame->shown, (ITInputEvent *)a, b);
            if (e != 0)
                err = e;
        }
        break;
    case kMsgEdit + 1:
    case kMsgEdit + 3:
    case kMsgEdit + 4:
    case kMsgEdit + 5:
    case kMsgEdit + 6:
    case kMsgEdit + 8:
        if (frame->window != 0) {
            e = _IT_ViewGetFocus(frame->shown, &focused, 0, 0);
            if (e != 0) {
                err = e;
                break;
            }
            if (focused != 0) {
                e = _IT_ViewHandleEdit(frame->shown, msg - kMsgEdit, &click);
                if (e != 0)
                    err = e;
            }
        }
        break;
    case kMsgActivate:
    case kMsgDeactivate:
        frame->active = AsByte(msg == kMsgActivate);
        if (frame->window != 0) {
            e = _IT_ViewHandleActivate(frame->shown, msg == kMsgActivate);
            if (e != 0)
                err = e;
        }
        break;
    case kMsgItems:
        e = IndirectSetSize(b, 0);
        if (e != 0)
            err = e;
        e = _IT_ViewQueryFocus(frame->shown, b);
        if (e != 0)
            err = e;
        break;
    case kMsgFocus:
        e = _IT_ViewSetFocus(frame->shown, 0, a, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgGetBounds:
        *(ITRect *)b = frame->bounds;
        break;
    case kMsgBounds:
        if (frame->shown != 0) {
            WindowNode *window = gIT_CurrentView;
            ITControl *control = gIT_CurrentItem;
            gIT_CurrentView = 0;
            gIT_CurrentItem = 0;
            err = _IT_ViewSetRect(frame->shown, (const ITRect *)a);
            gIT_CurrentView = window;
            gIT_CurrentItem = control;
            if (err != 0)
                break;
        }
        frame->bounds = *(const ITRect *)a;
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
        e = IndirectSetSize(b, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgSetText:
        IndirectGetLock((void **)a);
        err = kUnsupported;
        break;
    case kMsgChild:
        *(WindowNode **)b = frame->window;
        break;
    case kMsgSetValue:
        err = kUnsupported;
        break;
    case kMsgVisible:
        *(unsigned char *)b = frame->visible;
        break;
    case kMsgShow:
        frame->visible = AsByte(a != 0);
        if (frame->shown != 0) {
            WindowNode *window = gIT_CurrentView;
            ITControl *control = gIT_CurrentItem;
            gIT_CurrentView = 0;
            gIT_CurrentItem = 0;
            err = _IT_ViewSetEnabled(frame->shown, AsByte(a != 0), frame->shown->enabled);
            gIT_CurrentView = window;
            gIT_CurrentItem = control;
            if (err != 0)
                e = err;
        }
        break;
    case kMsgEnabled:
        *(unsigned char *)b = frame->shown->enabled;
        break;
    case kMsgEnable:
        if (frame->shown != 0) {
            WindowNode *window = gIT_CurrentView;
            ITControl *control = gIT_CurrentItem;
            gIT_CurrentView = 0;
            gIT_CurrentItem = 0;
            err = _IT_ViewSetEnabled(frame->shown, frame->shown->visible, AsByte(a != 0));
            gIT_CurrentView = window;
            gIT_CurrentItem = control;
            if (err != 0)
                e = err;
        }
        break;
    case kMsgGetFore:
    case kMsgGetBack:
        *(ITColor *)b = *(msg == kMsgGetFore ? &frame->shown->fore : &frame->shown->back);
        break;
    case kMsgFore:
    case kMsgBack:
        if (msg == kMsgFore)
            frame->shown->fore = *(ITColor *)b;
        else
            frame->shown->back = *(ITColor *)b;
        e = _IT_ViewInvalidate(frame->shown);
        if (e != 0)
            err = e;
        break;
    }
done:
    if (file != 0)
        MemoryDeallocate(file);
    if (anchor != 0)
        MemoryDeallocate(anchor);
#if TARGET_API_MAC_CARBON
    if (clip != 0)
        DisposeRgn(clip);
#endif
    if (saveClip != 0) {
        SetClip(saveClip);
        DisposeRgn(saveClip);
    }
    SetGWorld(savePort, saveDevice);
    return err;
}
