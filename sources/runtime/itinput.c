// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* itinput.c -- the interface tool's input state (0x00524337..0x00524721): modifier keys, mouse buttons and where the mouse is. Notes: Research/notes/itinput.md */

#include "burgertypes.h"
#include "burgerplat.h"
#include "burger.h"
#include "ambrosia.h"
#include "win32.h"

extern ITPoint Sys2DT_Point(const ITPoint *pt);                          /* ittoolkit.c */

#define HIWORD(l)           ((unsigned short)((ULong)(l) >> 16))
#define MK_LBUTTON          0x0001
#define MK_RBUTTON          0x0002
#define MK_SHIFT            0x0004
#define MK_CONTROL          0x0008
#define MK_MBUTTON          0x0010
#define kModDoubleClick     0x40000000
#define LOWORD(l)           ((unsigned short)((ULong)(l) & 0xffff))
#define LOBYTE(w)           ((unsigned char)((ULong)(w) & 0xff))
#define PM_REMOVE           2
#define kKeyExtended        0x00010000
#define kKeyRepeat          0x40000000
#define PM_NOREMOVE         0
#define SM_SWAPBUTTON       23
#define SM_MOUSEPRESENT     19
#define CWP_SKIPINVISIBLE   0x0001
#define CWP_SKIPTRANSPARENT 0x0004

#define kAllWindows         ((void *)-1)

/* virtual-key code to the tool's key code (0x80.. function and editing keys, 0xa1.. F1..,
   0xc0 unmapped) */
unsigned char gIT_SysKey2VirtKey[256] = {
    0x00, 0xff, 0xfe, 0xc0, 0xfd, 0xc0, 0xc0, 0x00, 0x93, 0x92, 0x00, 0x00, 0x94, 0x91, 0x00, 0x00,
    0x81, 0x82, 0x83, 0xc0, 0x80, 0xc0, 0x00, 0xc0, 0xc0, 0xc0, 0x00, 0xa0, 0xc0, 0xc0, 0xc0, 0xc0,
    0x90, 0x9a, 0x9b, 0x99, 0x98, 0x9c, 0x9f, 0x9e, 0x9d, 0xc0, 0xc0, 0xc0, 0xc0, 0x96, 0x97, 0xc0,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0b, 0x18, 0x16, 0x0d, 0x03, 0x0e, 0x0f, 0x10, 0x08, 0x11, 0x12, 0x13, 0x1a, 0x19, 0x09,
    0x0a, 0x01, 0x04, 0x0c, 0x05, 0x07, 0x17, 0x02, 0x15, 0x06, 0x14, 0x84, 0x8c, 0xc0, 0x00, 0xc0,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x52, 0x54, 0x50, 0x53, 0x55, 0x51,
    0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
    0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x88, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x81, 0x89, 0x82, 0x8a, 0x83, 0x8b, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0x00, 0x46, 0x42, 0x48, 0x41, 0x49, 0x4a,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x45, 0x44, 0x47, 0xc0,
    0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x94
};

/* the tool's key code back to the virtual-key code; nothing reads it */
unsigned char gIT_VirtKey2SysKey[256] = {
    0x00, 0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x61, 0x73, 0x64, 0x66, 0x67,
    0x68, 0x6a, 0x6b, 0x6c, 0x7a, 0x78, 0x63, 0x76, 0x62, 0x6e, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0xbd, 0xbb, 0xdb, 0xdd, 0xdc, 0xba, 0xde, 0xbc, 0xbe, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x6c, 0x6f, 0x6a, 0x6d, 0x6b, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x14, 0xa0, 0xa2, 0xa4, 0x5b, 0x00, 0x00, 0x00, 0x90, 0xa1, 0xa3, 0xa5, 0x5c, 0x00, 0x00, 0x00,
    0x20, 0x0d, 0x09, 0x08, 0xfe, 0x0d, 0x2d, 0x2e, 0x24, 0x23, 0x21, 0x22, 0x25, 0x28, 0x27, 0x26,
    0x1b, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e,
    0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x01
};

static char kUnknownKey[] = "<Unknown>";

long __stdcall IT_ButtonsUpEnumProc(void *hwnd, long lParam);

/* IT_ButtonsUpEnumProc (an EnumWindows callback: a button-up message waiting for the window, or for
   any child of a top-level one, means the buttons are no longer down; enumeration goes on
   while they still are). STATE: MATCH 100%, exe 55 / obj 55, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524337
long __stdcall IT_ButtonsUpEnumProc(void *hwnd, long lParam)
{
    ULong *flags;
    WinMsg msg;

    flags = (ULong *)lParam;
    if (PeekMessageA(&msg, hwnd, WM_LBUTTONUP, WM_LBUTTONUP, PM_NOREMOVE) != 0
        || PeekMessageA(&msg, hwnd, WM_RBUTTONUP, WM_RBUTTONUP, PM_NOREMOVE) != 0
        || PeekMessageA(&msg, hwnd, WM_MBUTTONUP, WM_MBUTTONUP, PM_NOREMOVE) != 0)
        *flags &= ~kModStillDown;
    if (GetParent(hwnd) == 0)
        EnumChildWindows(hwnd, (void *)IT_ButtonsUpEnumProc, lParam);
    return (*flags & kModStillDown) != 0 ? true : false;
}

/* _InputReadModifiers (the modifier keys, lock keys and mouse buttons as flags; with a button down,
   the button-up messages already queued for `hwnd' (every window for -1) are honoured). STATE:
   MATCH 100%, exe 89 / obj 89, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005243d4
ULong _InputReadModifiers(void *hwnd)
{
    ULong flags;
    long lParam;

    flags = 0;
    if (GetAsyncKeyState(VK_LWIN) < 0 || GetAsyncKeyState(VK_RWIN) < 0)
        flags |= kModWindows;
    if (GetAsyncKeyState(VK_MENU) < 0)
        flags |= kModAlt;
    if (GetAsyncKeyState(VK_CONTROL) < 0)
        flags |= kModControl;
    if (GetAsyncKeyState(VK_SHIFT) < 0)
        flags |= kModShift;
    if (GetAsyncKeyState(VK_CAPITAL) < 0)
        flags |= kModCapsLock;
    if (GetAsyncKeyState(VK_NUMLOCK) < 0)
        flags |= kModNumLock;
    if (GetAsyncKeyState(VK_SCROLL) < 0)
        flags |= kModScrollLock;
    if (GetSystemMetrics(SM_SWAPBUTTON) < 0)
        flags |= kModSwapped;
    if (GetAsyncKeyState(VK_LBUTTON) < 0)
        flags |= kModLeftButton;
    if (GetAsyncKeyState(VK_RBUTTON) < 0)
        flags |= kModRightButton;
    if (GetAsyncKeyState(VK_MBUTTON) < 0)
        flags |= kModMiddleButton;
    if (GetSystemMetrics(SM_MOUSEPRESENT) != 0)
        flags |= kModMouse;
    if ((flags & kModButtons) != 0) {
        lParam = (long)&flags;
        flags |= kModStillDown;
        if (hwnd == kAllWindows)
            EnumWindows((void *)IT_ButtonsUpEnumProc, lParam);
        else if (hwnd != 0)
            IT_ButtonsUpEnumProc(hwnd, lParam);
    }
    return flags;
}

/* InputReadMouseState (the mouse now: screen position, position in the window under it (in `hwnd'
   when that is the child there), that window, the modifier flags; whether a button is down).
   STATE: MATCH 100%, exe 79 / obj 79, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052451e
unsigned char InputReadMouseState(void *hwnd, ITInputEvent *event)
{
    POINT pt;
    void *under;
    void *child;
    long err;

    err = 0;
    MemoryClear(event, sizeof(ITInputEvent));
    GetCursorPos(&pt);
    event->where = Sys2DT_Point((ITPoint *)&pt);
    event->screen = event->where;
    under = WindowFromPoint(pt);
    if (under != 0) {
        ScreenToClient(under, &pt);
        child = ChildWindowFromPointEx(under, pt, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
        if (child != 0 && child == hwnd) {
            ClientToScreen(under, &pt);
            under = child;
            ScreenToClient(child, &pt);
        }
        event->where = Sys2DT_Point((ITPoint *)&pt);
    }
    event->hwnd = under;
    event->modifiers = _InputReadModifiers(hwnd);
    return (event->modifiers & kModButtons) != 0 ? true : false;
}

/* MacEventToMouseEvent (a Windows mouse message as an event: where in the window and on the screen, the
   button and key flags; whether it is a button press, a double click also flagged). The flags are
   read off the same word as the position, as the exe does. STATE: MATCH 100%, exe 100 / obj 100,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005245e7
unsigned char MacEventToMouseEvent(void *hwnd, unsigned int msg, unsigned int unused, long lParam,
                           ITInputEvent *event)
{
    long err;
    unsigned char press;
    POINT pt;

    err = 0;
    press = 0;
    MemoryClear(event, sizeof(ITInputEvent));
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        press = 1;
        break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        event->modifiers |= kModDoubleClick;
        press = 1;
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        break;
    }
    event->hwnd = hwnd;
    event->where.v = HIWORD(lParam);
    pt.y = event->where.v;
    event->where.h = LOWORD(lParam);
    pt.x = event->where.h;
    ClientToScreen(hwnd, &pt);
    event->screen.v = pt.y;
    event->screen.h = pt.x;
    if (lParam & MK_LBUTTON)
        event->modifiers |= kModLeftButton;
    if (lParam & MK_RBUTTON)
        event->modifiers |= kModRightButton;
    if (lParam & MK_MBUTTON)
        event->modifiers |= kModMiddleButton;
    if (lParam & MK_SHIFT)
        event->modifiers |= kModShift;
    if (lParam & MK_CONTROL)
        event->modifiers |= kModControl;
    if (GetKeyState(VK_MENU) < 0)
        event->modifiers |= kModAlt;
    if (GetKeyState(VK_CAPITAL) & 1)
        event->modifiers |= kModCapsLock;
    if (GetKeyState(VK_NUMLOCK) & 1)
        event->modifiers |= kModNumLock;
    if (GetKeyState(VK_SCROLL) & 1)
        event->modifiers |= kModScrollLock;
    if (GetSystemMetrics(SM_SWAPBUTTON) != 0)
        event->modifiers |= kModSwapped;
    return press;
}

/* MacEventToKeyEvent (a key-down message as a key event: the button and key flags, and when the
   matching WM_CHAR can be taken off the queue, the character, the virtual key, the tool's key
   code (keypad Enter 0x95) and the auto-repeat bit; whether a character came). STATE: MATCH
   100%, exe 115 / obj 115, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00524721
unsigned char MacEventToKeyEvent(void *hwnd, unsigned int msg, unsigned int vk, ULong lParam,
                           ITKeyEvent *key)
{
    unsigned char got;
    WinMsg chr;
    long err;

    err = 0;
    got = 0;
    MemoryClear(key, sizeof(ITKeyEvent));
    if (GetKeyState(VK_LBUTTON) < 0)
        key->modifiers |= kModLeftButton;
    if (GetKeyState(VK_RBUTTON) < 0)
        key->modifiers |= kModRightButton;
    if (GetKeyState(VK_MBUTTON) < 0)
        key->modifiers |= kModMiddleButton;
    if (GetKeyState(VK_SHIFT) < 0)
        key->modifiers |= kModShift;
    if (GetKeyState(VK_CONTROL) < 0)
        key->modifiers |= kModControl;
    if (GetKeyState(VK_MENU) < 0)
        key->modifiers |= kModAlt;
    if (GetKeyState(VK_CAPITAL) & 1)
        key->modifiers |= kModCapsLock;
    if (GetKeyState(VK_NUMLOCK) & 1)
        key->modifiers |= kModNumLock;
    if (GetKeyState(VK_SCROLL) & 1)
        key->modifiers |= kModScrollLock;
    if (GetSystemMetrics(SM_SWAPBUTTON) != 0)
        key->modifiers |= kModSwapped;
    switch (msg) {
    case WM_KEYDOWN:
        got = PeekMessageA(&chr, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE);
        break;
    case WM_KEYUP:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
        break;
    }
    if (got) {
        key->ch = LOBYTE(LOWORD(chr.wParam));
        key->vk = vk;
        if (vk == VK_RETURN && (lParam & kKeyExtended) != 0)
            key->key = kKeyEnter;
        else
            key->key = gIT_SysKey2VirtKey[key->vk];
        key->code = LOWORD(chr.wParam);
        if (lParam & kKeyRepeat)
            key->modifiers |= kModRepeat;
    }
    return got;
}
