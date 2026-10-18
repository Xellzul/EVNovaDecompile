// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itpopup.c -- the interface tool's POPUP control: a Windows combo box over the ITEM elements of its markup. Notes: Research/notes/itpopup.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"

/* the control's data handle */
typedef struct ITPopup {
    void **title;
    unsigned char language;                      /* the LANGUAGE popup: never shown */
    unsigned char pad05[3];
    ITRect bounds;
    long selected;                               /* 1-based */
    unsigned char active;
    unsigned char visible;
    unsigned char enabled;
    unsigned char pad1f;
    void *combo;
} ITPopup;

#define kDropHeight         200

#define WS_COMBO            0x50010003UL    /* WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST */
#define SWP_NOACTIVATE      0x0010
#define CB_ADDSTRING        0x0143
#define CB_GETCOUNT         0x0146
#define CB_GETCURSEL        0x0147
#define CB_SELECTSTRING     0x014d
#define CB_SETCURSEL        0x014e
#define CB_ERR              (-1)

/* IT_PopupHandler (the POPUP control proc: 0 makes its data, 1 creates the combo box with the
   ITEM elements' titles (none when empty, "-" skipped, the DEFAULT one selected), 2 destroys
   it, 3 frees the data; hit tests, painting through the combo box, bounds, title, value by
   index or text, show and enable; its enabled state follows the window's). STATE: MATCH 100%,
   exe 668 / obj 668, byte-identical yes. OPEN: none (`top` is assigned inside the
   CreateWindowEx arguments, evaluated right to left: notes/itpopup.md) */
// FUNCTION: EVNOVA 0x0052174c
long IT_PopupHandler(void *hwnd, void *dc, ITElement *element, void **data, ULong msg, ULong a,
                  void *b)
{
    ITPopup *popup;
    long err;
    long e;
    unsigned char dead;
    char *title;
    void *windowDC;
    void *hit;
    unsigned char skip;
    long top;
    ITElement *first;
    ITElement *item;
    unsigned char update;
    POINT pt;
    ITRect r;
    char text[256];
    POINT origin;
    long index;

    popup = *data;
    dead = 0;
    err = 0;
    update = 0;
    if (msg == kMsgCreate)
        IndirectGetSize(data);
    if (msg != kMsgCreate)
        IndirectGetSize(data);
    switch (msg) {
    case kMsgCreate:
        e = IndirectSetSize(data, sizeof(ITPopup));
        if (e != 0) {
            err = e;
            goto done;
        }
        IndirectSetLock(data, 1);
        popup = *data;
        MemoryClear(*data, sizeof(ITPopup));
        if ((popup->title = IndirectAllocate(0)) == 0) {
            err = kOutOfMemory;
            goto done;
        }
        title = IT_ItemCallbackGetParam("LANGUAGE", 0);
        if (title != 0)
            popup->language = 1;
        popup->enabled = 1;
        popup->visible = 1;
        popup->active = 1;
        if (popup->language)
            popup->visible = 0;
        break;
    case kMsgOpen:
        StringCopySafe(text, *popup->title,
                     IndirectGetSize(popup->title) + 1 < kTextMax ? IndirectGetSize(popup->title) + 1
                                                               : kTextMax);
        if ((popup->combo = CreateWindowExA(0, "COMBOBOX", 0, WS_COMBO, popup->bounds.left, top,
                                            popup->bounds.right - popup->bounds.left,
                                            popup->bounds.bottom - (top = popup->bounds.top), hwnd,
                                            0,
                                            (void *)GetWindowLongA(hwnd, GWL_HINSTANCE), 0)) == 0) {
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        update = 1;
        popup->selected = 0;
        first = element;
        item = 0;
        while (element != 0 &&
               (item = _IT_ParseTreeFindNextTag(element, item, "ITEM", AsByte(item != 0))) != 0) {
            title = _IT_SimpleFindAttribute(item, "TITLE");
            skip = (!title || !*title) || !StringCompare(title, "-");
            if (!skip)
                SendMessageA(popup->combo, CB_ADDSTRING, 0, (long)title);
            if (!skip && _IT_SimpleFindAttribute(item, "DEFAULT") != 0)
                popup->selected = SendMessageA(popup->combo, CB_GETCOUNT, 0, 0) - 1;
        }
        if (SendMessageA(popup->combo, CB_GETCOUNT, 0, 0) == 0)
            SendMessageA(popup->combo, CB_ADDSTRING, 0, (long)"none");
        SendMessageA(popup->combo, CB_SETCURSEL, 0, 0);
        SetWindowPos(popup->combo, 0, popup->bounds.left, popup->bounds.top,
                     popup->bounds.right - popup->bounds.left,
                     popup->bounds.bottom - popup->bounds.top + kDropHeight, SWP_NOACTIVATE);
        break;
    case kMsgClose:
        if (popup->combo != 0)
            DestroyWindow(popup->combo);
        popup->combo = 0;
        break;
    case kMsgDispose:
        if (popup->title != 0) {
            IndirectDeallocate(popup->title);
            popup->title = 0;
        }
        IndirectSetLock(data, 0);
        e = IndirectSetSize(data, 0);
        if (e != 0)
            err = e;
        break;
    case kMsgHitTest:
        hit = (void *)a;
        goto compare;
    case kMsgClick:
        CopyDT_2SysPoint((ITPoint *)&pt, &((ITInputEvent *)a)->where);
        if (!IsWindowVisible(popup->combo))
            break;
        hit = ChildWindowFromPoint(hwnd, pt);
compare:
        if (popup->combo == hit)
            *(unsigned char *)b = 1;
        break;
    case kMsgPaint:
        if (popup->combo != 0) {
            windowDC = GetDC(hwnd);
            SetViewportOrgEx(dc, popup->bounds.left, popup->bounds.top, &origin);
            SendMessageA(popup->combo, dc == windowDC ? WM_PAINT : WM_PRINT, (unsigned int)dc,
                         PRF_CHECKVISIBLE | PRF_CLIENT);
            SetViewportOrgEx(dc, origin.x, origin.y, &origin);
            ReleaseDC(hwnd, windowDC);
        }
        break;
    case 0x13:
    case 0x14:
        break;
    case kMsgWhat:
        CopyDT_2SysPoint((ITPoint *)&pt, &((ITInputEvent *)a)->where);
        break;
    case kMsgKey:
        break;
    case kMsgPress:
        SendMessageA(popup->combo, BM_SETSTATE, 1, 0);
        SystemWait(kPressMs);
        SendMessageA(popup->combo, BM_SETSTATE, 0, 0);
        break;
    case kMsgMouse:
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
        popup->active = AsByte(msg == kMsgActivate);
        if (popup->combo != 0)
            update = 1;
        break;
    case kMsgItems:
        if (popup->enabled && IT_ItemCallbackIsViewEnabled()) {
            e = IndirectAppendData(b, &popup->combo, sizeof(popup->combo));
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        break;
    case kMsgFocus:
        SetFocus(popup->combo);
        break;
    case kMsgGetBounds:
        *(ITRect *)b = popup->bounds;
        break;
    case kMsgBounds:
        popup->bounds = *(const ITRect *)a;
        CopyDT_2SysRect(&r, &popup->bounds);
        if (popup->combo != 0
            && !MoveWindow(popup->combo, r.left, r.top, r.right - r.left, r.bottom - r.top, 1)) {
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        break;
    case kMsgGetTitle:
        e = IndirectReplace(b, popup->title);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case kMsgTitle:
        e = IndirectReplace(popup->title, (void **)a);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (popup->combo == 0)
            break;
        StringCopySafe(text, *popup->title,
                     IndirectGetSize(popup->title) + 1 < kTextMax ? IndirectGetSize(popup->title) + 1
                                                               : kTextMax);
        if (!SetWindowTextA(popup->combo, text)) {
            err = GetLastError();
            err = kWindowFailed;
            goto done;
        }
        break;
    case kMsgGetText:
        e = IT_PopupHandler(hwnd, dc, element, data, kMsgGetValue, 0, &index);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (popup->combo != 0) {
            ULong length;

            length = GetWindowTextLengthA(popup->combo);
            e = IndirectSetSize(b, length + 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            GetWindowTextA(popup->combo, *(char **)b, length + 1);
            e = IndirectSetSize(b, length);
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
        if (popup->combo != 0) {
            void **handle;
            ULong length;
            char value[256];

            handle = (void **)a;
            length = IndirectGetSize(handle);
            if (length > kTextMax - 1)
                length = kTextMax - 1;
            MemoryCopy(value, *handle, length);
            value[length] = 0;
            if (SendMessageA(popup->combo, CB_SELECTSTRING, (unsigned int)-1, (long)value) == CB_ERR) {
                err = CB_ERR;
                err = kNotFound;
                goto done;
            }
        }
        break;
    case kMsgGetValue:
        *(long *)b = popup->selected;
        if (popup->combo != 0)
            *(long *)b = SendMessageA(popup->combo, CB_GETCURSEL, 0, 0) + 1;
        break;
    case kMsgSetValue:
        popup->selected = a;
        if (popup->combo != 0)
            SendMessageA(popup->combo, CB_SETCURSEL, popup->selected - 1, 0);
        break;
    case kMsgVisible:
        *(unsigned char *)b = popup->visible;
        break;
    case kMsgShow:
        popup->visible = AsByte(a != 0);
        if (popup->language)
            popup->visible = 0;
        if (popup->combo != 0)
            ShowWindow(popup->combo, popup->visible ? SW_SHOW : SW_HIDE);
        break;
    case kMsgEnabled:
        *(unsigned char *)b = popup->enabled;
        break;
    case kMsgEnable:
        popup->enabled = AsByte(a != 0);
        if (popup->combo != 0)
            update = 1;
        break;
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:
        break;
    }
    if (update)
        if (!(popup->active && popup->enabled && IT_ItemCallbackIsViewEnabled())) {
            EnableWindow(popup->combo, 0);
            return err;
        } else {
            EnableWindow(popup->combo, 1);
            return err;
        }
done:
    return err;
}
