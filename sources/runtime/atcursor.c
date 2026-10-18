// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atcursor.c -- the interface tool's open and teardown (the window classes and the control types), the cursor setter and opening a URL. Notes: Research/notes/atcursor.md */

#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"

extern WindowNode *IT_WindowFindView(void *hwnd);     /* ittoolkit.c */
extern long _IT_ViewHandleHit(WindowNode *window, long item, ITClick *click);
extern long _IT_ViewGetItemColor(WindowNode *window, ULong item, void *a, void *b, unsigned char *hit);
extern unsigned char DT_IsRegionEmpty(void *region);
extern unsigned char MacEventToKeyEvent(void *hwnd, unsigned int msg, unsigned int vk, ULong lParam,
                                  ITKeyEvent *key);                      /* itinput.c */
extern unsigned char MacEventToMouseEvent(void *hwnd, unsigned int msg, unsigned int unused,
                                  long lParam, ITInputEvent *event);     /* itinput.c */
extern long __stdcall IT_WindowProc(void *hwnd, unsigned int msg, unsigned int wParam,
                                   long lParam);  /* ittoolkit.c */

extern unsigned char gIT_Open;

void IT_Idle(void);

short gIT_CursorID = -1;                         /* 0x00577000 the cursor shown */
static long gIT_ClassMain = 0;                    /* 0x00865b10 */
static long gIT_ClassCloseBox = 0;                    /* 0x00865b14 */
static long gIT_ClassParent = 0;                    /* 0x00865b18 */

#define kIdcArrow    ((const char *)0x7f00)
#define kGrayBrush   2
#define kDoubleClicks 0x200
#define kRegisterFailed -8400


/* IT_Open. STATE: MATCH 100%, exe 183 / obj 183, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eaac3
long IT_Open(void)
{
    WNDCLASSA wc;
    long err, e;

    err = 0;
    if (gIT_Open != 0)
        goto done;

    e = PlatformOpen();
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FT_Open();
    if (e != 0) {
        err = e;
        goto done;
    }
    e = DT_Open();
    if (e != 0) {
        err = e;
        goto done;
    }

    SystemSetIdleProc(IT_Idle);
    SystemSetQuitProc(IT_Close);

    wc.lpszClassName = "interface_tool";
    wc.hInstance = GetModuleHandleA(0);
    wc.lpfnWndProc = IT_WindowProc;
    wc.hCursor = LoadCursorA(0, kIdcArrow);
    wc.hIcon = LoadIconA(GetModuleHandleA(0), "IDI_APPICON");
    wc.lpszMenuName = 0;
    wc.hbrBackground = GetStockObject(kGrayBrush);
    wc.style = kDoubleClicks;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    if (gIT_ClassMain == 0) {
        if (RegisterClassA(&wc) == 0) {
            err = GetLastError();
            err = kRegisterFailed;
            goto done;
        }
        gIT_ClassMain = 1;
    }

    wc.lpszClassName = "interface_tool_closebox";
    wc.hInstance = GetModuleHandleA(0);
    wc.lpfnWndProc = IT_WindowProc;
    wc.hCursor = LoadCursorA(0, kIdcArrow);
    wc.hIcon = LoadIconA(GetModuleHandleA(0), "IDI_APPICON");
    wc.lpszMenuName = 0;
    wc.hbrBackground = GetStockObject(kGrayBrush);
    wc.style = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    if (gIT_ClassCloseBox == 0) {
        if (RegisterClassA(&wc) == 0) {
            err = GetLastError();
            err = kRegisterFailed;
            goto done;
        }
        gIT_ClassCloseBox = 1;
    }

    wc.lpszClassName = "interface_tool_parent";
    wc.hInstance = GetModuleHandleA(0);
    wc.lpfnWndProc = IT_WindowProc;
    wc.hCursor = LoadCursorA(0, kIdcArrow);
    wc.hIcon = LoadIconA(GetModuleHandleA(0), "IDI_APPICON");
    wc.lpszMenuName = 0;
    wc.hbrBackground = GetStockObject(kGrayBrush);
    wc.style = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    if (gIT_ClassParent == 0) {
        if (RegisterClassA(&wc) == 0) {
            err = GetLastError();
            err = kRegisterFailed;
            goto done;
        }
        gIT_ClassParent = 1;
    }

    gIT_Open = 1;

    e = IT_ItemHandlerInstall(0, "BUTTON", IT_ButtonHandler);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ItemHandlerInstall(0, "IMAGE", IT_ButtonHandler);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ItemHandlerInstall(0, "IMG", IT_ButtonHandler);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ItemHandlerInstall(0, "BORDER", IT_BorderHandler);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ItemHandlerInstall(0, "FRAME", IT_FrameHandler);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ItemHandlerInstall(0, "POPUP", IT_PopupHandler);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ItemHandlerInstall(0, "TEXTBOX", IT_TextHandler);
    if (e != 0)
        err = e;

done:
    if (err != 0)
        IT_Close();
    return err;
}

/* The per-frame walk registered above: every window gets 0x0051afd6, and the
   ones flagged at +0x20 get 0x0051ba8d FIRST. The flag is not cleared here,
   so whatever 0x0051ba8d does is what clears it. */
/* IT_Idle. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ead8e
void IT_Idle(void)
{
    WindowNode *node;
    long err;

    err = 0;
    node = gIT_ViewList;
    while (node != 0) {
        if (node->dirty != 0)
            _IT_ViewValidate(node);
        _IT_ViewHandleIdle(node);
        node = node->next;
    }
}

static char kEditClass[] = "EDIT";
static char kRichEditClass[] = "RichEdit20A";

WindowNode *gIT_ViewList;                        /* 0x00870184 */
ControlType *gIT_HandlerList;                    /* 0x008700bc */
void *gIT_DefaultHash;                           /* 0x00870074 */
void *gCustomCursorH;                              /* 0x00870df8 cursor */
unsigned char gIT_Open;                          /* 0x00870e09 */

#define kAllOnes       0xffff
#define WA_ACTIVE           1
#define WA_CLICKACTIVE      2
#define HTCLIENT            1
#define SC_CLOSE            0xf060
#define kNoCursor           0xffff
#define kCommandItem        1       /* HIWORD(wParam) <= 1: a menu item or accelerator */
#define kCommandFocus       0x100
#define kCommandBlur        0x200
#define kCommandKey         0x300
#define kNoParameter        -8400

typedef struct MinMaxInfo {
    long reserved[2];
    long maxWidth;
    long maxHeight;
    long maxLeft;
    long maxTop;
    long minTrackWidth;
    long minTrackHeight;
    long maxTrackWidth;
    long maxTrackHeight;
} MinMaxInfo;
#define kWaitCursor    0x7f02
#define kShellOk       32
#define kShowNormal    1

typedef struct CursorShape {
    ITPoint size;
    ITPoint hot;
} CursorShape;

/* 0x00577098: IDC_HAND is missing before Windows 98, so the hand is built by hand. */
static const unsigned char kHandAnd[128] = {
    0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,
    0xff,0xf3,0xff,0xff, 0xff,0xe1,0xff,0xff, 0xff,0xe1,0xff,0xff, 0xff,0xe1,0xff,0xff,
    0xff,0xe0,0x7f,0xff, 0xff,0xe0,0x4f,0xff, 0xff,0xe0,0x09,0xff, 0xff,0xe0,0x00,0xff,
    0xfe,0x20,0x00,0xff, 0xfe,0x00,0x00,0xff, 0xfe,0x00,0x00,0xff, 0xff,0x00,0x00,0xff,
    0xff,0x80,0x00,0xff, 0xff,0x80,0x01,0xff, 0xff,0xc0,0x01,0xff, 0xff,0xc0,0x03,0xff,
    0xff,0xe0,0x03,0xff, 0xff,0xe0,0x07,0xff, 0xff,0xf0,0x07,0xff, 0xff,0xf0,0x07,0xff,
    0xff,0xf0,0x07,0xff, 0xff,0xf0,0x07,0xff, 0xff,0xf0,0x07,0xff, 0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff
};

/* 0x00577118 */
static const unsigned char kHandXor[128] = {
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00, 0x00,0x0c,0x00,0x00, 0x00,0x0c,0x00,0x00, 0x00,0x0c,0x00,0x00,
    0x00,0x0c,0x00,0x00, 0x00,0x0d,0x80,0x00, 0x00,0x0d,0xb0,0x00, 0x00,0x0d,0xb6,0x00,
    0x00,0x0d,0xb6,0x00, 0x00,0xcf,0xf6,0x00, 0x00,0xef,0xfe,0x00, 0x00,0x6f,0xfe,0x00,
    0x00,0x3f,0xfe,0x00, 0x00,0x3f,0xfc,0x00, 0x00,0x1f,0xfc,0x00, 0x00,0x1f,0xf8,0x00,
    0x00,0x0f,0xf8,0x00, 0x00,0x0f,0xf0,0x00, 0x00,0x07,0xf0,0x00, 0x00,0x07,0xf0,0x00,
    0x00,0x00,0x00,0x00, 0x00,0x07,0xf0,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* 0x00577198 */
static const CursorShape kHandShape = { { 32, 32 }, { 13, 5 } };

/* IT_Close. STATE: MATCH 100%, exe 39 / obj 39, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eadb7
void IT_Close(void)
{
    long err;
    ControlType *p;
    WindowNode *last;

    err = 0;
    while ((last = gIT_ViewList) != 0) {
        while (last->parent != 0)
            last = last->parent;
        IT_ViewDispose(last);
    }

    while ((p = gIT_HandlerList) != 0) {
        gIT_HandlerList = p->next;
        MemoryDeallocate(p);
    }

    if (gIT_DefaultHash != 0) {
        HashDispose(gIT_DefaultHash);
        gIT_DefaultHash = 0;
    }
    if (gCustomCursorH != 0) {
        DestroyCursor(gCustomCursorH);
        gCustomCursorH = 0;
    }
    gIT_CursorID |= kAllOnes;
    gIT_Open = 0;
}

/* IT_IsOpen (is the interface tool open). STATE: MATCH 100%, exe 2 / obj 2, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004eae38
unsigned char IT_IsOpen(void)
{
    return gIT_Open;
}

/* IT_HandleWinEvent (the interface tool's window procedure body, shared by the window classes
   (0x0051c610) and the dialog procedure (0x0051c664): close / destroy tear the root down,
   activation moves the focus, paint and erase draw the window, the cursor follows the
   control under the mouse, key, mouse and WM_COMMAND events go to the controls, the dialog
   and static colours come from the window or its callback; the answer is whether the
   message was handled, *result the brush a WM_CTLCOLOR message returns). STATE: MATCH 100%,
   exe 511 / obj 511, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eae3e
unsigned char IT_HandleWinEvent(void *hwnd, void *dc, unsigned int msg, unsigned int wParam,
                           long lParam, long *result)
{
    static ITClick noClick;
    ITClick click;
    PAINTSTRUCT ps;
    ITInputEvent event;
    ITKeyEvent key;
    WindowNode *window;
    void *ownDC;
    void *region;
    void *paintDC;
    long err;
    long e;

    err = 0;
    click = noClick;
    window = 0;
    err = 0;
    paintDC = 0;
    ownDC = 0;
    region = 0;
    *result = 0;
    if (hwnd != 0) {
        window = IT_WindowFindView(hwnd);
        if (dc == 0) {
            dc = ownDC = GetDC(hwnd);
            if (dc == 0) {
                err = GetLastError();
                err = kNoParameter;
                goto done;
            }
        }
    }
    if (window == 0)
        goto done;
    switch (msg) {
    case WM_CREATE:
        break;
    case WM_ACTIVATE: {
        unsigned char hit;
        unsigned char minimized;
        unsigned short lo;

        lo = wParam;
        minimized = AsByte((wParam & 0xffff0000) != 0);
        hit = 0;
        _IT_ViewHandleActivate(window, lo == WA_ACTIVE || lo == WA_CLICKACTIVE);
        if (window->tracking != 0)
            _IT_ViewHandleTest(window, window->tracking, &hit);
        if (hit)
            _IT_ViewSetFocus(window, 0, window->tracking, 1);
        else
            _IT_ViewMoveFocus(window, 1, 0, 0, 0);
        window->tracking = 0;
        click.followed = 1;
        break;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        click.followed = 1;
        break;
    case WM_PAINT:
        if (window->titled)
            dc = paintDC = BeginPaint(hwnd, &ps);
        _IT_ViewHandleDraw(window, 0, 0);
        click.followed = 1;
        if (paintDC != 0) {
            EndPaint(hwnd, &ps);
            paintDC = 0;
        }
        break;
    case WM_ERASEBKGND: {
        WinRect rc;
        unsigned char drawn;
        void *brush;

        if (!window->titled)
            break;
        drawn = 0;
        if (window->image != 0)
            region = DT_GetClipRegion(window->image);
        if (region != 0 && DT_IsRegionEmpty(region)) {
            click.followed = 1;
            break;
        }
        if (window->backdrop != 0) {
            DT_DrawImageTile(window->image, 0, 0, window->backdrop, 0, 0, 0);
            click.followed = 1;
            break;
        }
        if (!GetClientRect(hwnd, &rc)) {
            err = GetLastError();
            err = kNoParameter;
            goto done;
        }
        brush = CreateSolidBrush(DT_2SysColor(&window->back));
        if (brush == 0) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        FillRect((void *)wParam, &rc, brush);
        DeleteObject(brush);
        click.followed = 1;
        break;
    }
    case WM_SETCURSOR: {
        ITInputEvent mouse;
        unsigned short cursor;

        if ((lParam & 0xffff) != HTCLIENT)
            break;
        cursor = kNoCursor;
        InputReadMouseState(0, &mouse);
        e = _IT_ViewHandleCursor(window, &mouse, &cursor);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (cursor == kNoCursor)
            break;
        IT_SetCursor(cursor);
        click.followed = 1;
        break;
    }
    case WM_GETMINMAXINFO: {
        MinMaxInfo *info;
        WinRect rc;

        if (!IsWindowVisible(hwnd))
            break;
        info = (MinMaxInfo *)lParam;
        GetWindowRect(hwnd, &rc);
        info->maxWidth = rc.right - rc.left;
        info->maxHeight = rc.bottom - rc.top;
        info->maxLeft = rc.left;
        info->maxTop = rc.top;
        info->minTrackWidth = rc.right - rc.left;
        info->minTrackHeight = rc.bottom - rc.top;
        info->maxTrackWidth = rc.right - rc.left;
        info->maxTrackHeight = rc.bottom - rc.top;
        click.followed = 1;
        break;
    }
    case WM_CTLCOLORDLG:
        click.followed = 1;
        if (SetTextColor((void *)wParam, DT_2SysColor(&window->fore)) == CLR_INVALID) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        if (SetBkColor((void *)wParam, DT_2SysColor(&window->back)) == CLR_INVALID) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        *result = (long)CreateSolidBrush(DT_2SysColor(&window->back));
        break;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSCROLLBAR:
        break;
    case WM_CTLCOLORSTATIC: {
        ITColor fore;
        ITColor back;
        ULong failed;

        failed = GetTextColor((void *)wParam) == CLR_INVALID;
        if (failed) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        /* BUG: both conversions take the failed flag (0), not the DC's colour */
        CopySys2DT_Color(&fore, &failed);
        failed = GetBkColor((void *)wParam) == CLR_INVALID;
        if (failed) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        CopySys2DT_Color(&back, &failed);
        e = _IT_ViewGetItemColor(window, lParam, &fore, &back, &click.followed);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (!click.followed)
            break;
        if (SetTextColor((void *)wParam, DT_2SysColor(&fore)) == CLR_INVALID) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        if (SetBkColor((void *)wParam, DT_2SysColor(&back)) == CLR_INVALID) {
            err = GetLastError();
            err = kOutOfMemory;
            goto done;
        }
        *result = (long)CreateSolidBrush(DT_2SysColor(&back));
        break;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
        if (!MacEventToKeyEvent(hwnd, msg, wParam, lParam, &key))
            break;
        e = _IT_ViewHandleKey(window, &key, &click);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
        if (!MacEventToMouseEvent(hwnd, msg, wParam, lParam, &event))
            break;
        e = _IT_ViewHandleClick(window, &event, &click);
        if (e != 0) {
            err = e;
            goto done;
        }
        break;
    case WM_COMMAND: {
        ITKeyEvent noKey;

        switch ((unsigned short)(wParam >> 16)) {
        case 0:
        case kCommandItem:
            if (lParam == 0) {
                err = kNoParameter;
                goto done;
            }
            e = _IT_ViewHandleHit(window, lParam, &click);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        case kCommandFocus:
            if (lParam == 0) {
                err = kNoParameter;
                goto done;
            }
            window->tracking = 0;
            click.followed = 1;
            e = _IT_ViewSetFocus(window, 0, lParam, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        case kCommandBlur:
            if (lParam == 0) {
                err = kNoParameter;
                goto done;
            }
            click.followed = 1;
            e = _IT_ViewSetFocus(window, 0, 0, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        case kCommandKey:
            if (lParam == 0) {
                err = kNoParameter;
                goto done;
            }
            MemoryClear(&noKey, sizeof(noKey));
            e = _IT_ViewHandleKey(window, &noKey, &click);
            if (e != 0) {
                err = e;
                goto done;
            }
            break;
        }
        break;
    }
    case WM_SYSCOMMAND:
        switch (wParam & 0xfff0) {
        case SC_CLOSE:
            while (window->parent != 0)
                window = window->parent;
            IT_ViewDispose(window);
            break;
        }
        break;
    case WM_DESTROY:
    case WM_CLOSE:
        if (!window->closing)
            err = kBadArgument;
        if (window->closing)
            break;
        while (window->parent != 0)
            window = window->parent;
        IT_ViewDispose(window);
        break;
    }
done:
    if (paintDC != 0)
        EndPaint(hwnd, &ps);
    if (ownDC != 0)
        ReleaseDC(hwnd, ownDC);
    if (region != 0)
        DT_RegionDispose(region);
    /* BUG: the error codes gathered on the way (-8400, -7004, -7002) are never returned */
    return click.followed;
}

static ITClick DAT_00865b1c;                     /* 0x00865b1c, unreferenced */

/* IT_SetCursor. STATE: MATCH 100%, exe 70 / obj 70, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb5d4
long IT_SetCursor(short cursorId)
{
    CursorShape shape;
    long err;
    void *cursor;

    err = 0;
    shape.size = kHandShape.size;
    shape.hot = kHandShape.hot;
    cursor = 0;
    if (cursorId == gIT_CursorID || cursorId == -1)
        goto done;
    if (cursorId == 0)
        cursorId = IDC_ARROW;
    cursor = LoadCursorA(cursorId >= IDC_ARROW ? 0 : GetModuleHandleA(0),
                         (const char *)(unsigned short)cursorId);
    if (cursor == 0 && cursorId == IDC_HAND) {
        if (gCustomCursorH == 0) {
            gCustomCursorH = CreateCursor(GetModuleHandleA(0), shape.hot.h, shape.hot.v,
                                        shape.size.h, shape.size.v, kHandAnd, kHandXor);
            if (gCustomCursorH == 0) {
                err = GetLastError();
                err = kOutOfMemory;
                goto done;
            }
        }
        cursor = gCustomCursorH;
    }
    if (cursor == 0) {
        err = GetLastError();
        err = kNotFound;
        goto done;
    }
    gIT_CursorID = cursorId;
    SetCursor(cursor);
done:
    return err;
}

/* IT_OpenBrowser. STATE: MATCH 100%, exe 69 / obj 69, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eb6ca
long IT_OpenBrowser(const char *command)
{
    long err = 0;
    long e;
    void **copy;
    char *url;
    void *result;

    e = 0;
    copy = 0;
    copy = IndirectInitialize(command, StringGetLength((const unsigned char *)command));
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }
    e = PT_ParseEncode(copy, 0, 1, 1, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IndirectAppendData(copy, "", 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectSetLock(copy, 1);
    url = (char *)*copy;
    IT_SetCursor(kWaitCursor);
    result = ShellExecuteA(_PlatformGetWindow(), "open", url, 0, 0, kShowNormal);
    if ((ULong)result <= kShellOk)
        err = (long)result;
done:
    if (err != 0)
        MessageBeep(0);
    return err;
}
