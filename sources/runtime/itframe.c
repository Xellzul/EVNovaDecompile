// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itframe.c -- the interface tool's FRAME control (0x005210da..0x0052174c): a child window showing another page, SOURCE or SRC, with the events passed through to it. Notes: Research/notes/itframe.md */

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
typedef struct ITFrame {
    WindowNode *window;                          /* +0x00 the frame's own window */
    WindowNode *shown;                           /* +0x04 the same, once its page is loaded */
    ITRect bounds;                               /* +0x08 */
    unsigned char active;                        /* +0x18 */
    unsigned char visible;                       /* +0x19 */
    unsigned char pad1a[2];
} ITFrame;


/* IT_FrameHandler (the FRAME control proc: 1 creates its window as a child of the served one
   and loads the SOURCE (or SRC) page into it, 2 destroys it, 3 frees the data; painting,
   idle turns, clicks, keys, the mouse, edit commands, activation, focus, bounds, show,
   enable and colours are passed to the child window, the served window and control set to
   none around the calls that paint or move it). STATE: MATCH 100%, exe 471 / obj 471,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005210da
long IT_FrameHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    long err;
    long e;
    ITFrame *frame;
    char *link;
    ITControl *focused;
    char *file;
    char *anchor;
    ITPoint pt;
    ITClick click;
    unsigned char dead;

    frame = *data;
    dead = 0;
    err = 0;
    focused = 0;
    file = 0;
    anchor = 0;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
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
        *(ITColor *)b = msg == kMsgGetFore ? frame->shown->fore : frame->shown->back;
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
    if (file != 0)
        MemoryDeallocate(file);
    if (anchor != 0)
        MemoryDeallocate(anchor);
    return err;
}
