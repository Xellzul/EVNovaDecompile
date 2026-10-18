// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgerkbwin.c -- BurgerLib InKeyboardWin95: the key queue, and the Win32 message pump that fills it and the mouse state. Notes: Research/notes/burgerkbwin.md */

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

static ULong RWMMouseWheel;                      /* 0x008658a0 */
static long BurgerKeyStart;                      /* 0x008658a4 */
static long BurgerKeyEnd;                        /* 0x008658a8 */
static Word32 BurgerKeyBuffer[128];              /* 0x008658ac */
Word (*KeyboardGetchCallBack)(Word key);         /* 0x0087070c */
Word Win95NoProcess;                             /* 0x00870de0 */
static Word Recurse;                             /* 0x00865aac */
Word32 Win95MouseButton;                         /* 0x008701a0 */
long Win95MouseWheel;                            /* 0x008700a8 */
long Win95LastMouseX;                            /* 0x008700b0 */
long Win95LastMouseY;                            /* 0x008700b4 */
long Win95MouseXDelta;                           /* 0x00870190 */
long Win95MouseYDelta;                           /* 0x00870180 */

/* Windows scan code (bit 7 = extended) to BurgerLib scan code. */
Word8 NormalScan[256] = {                        /* 0x00576b50 */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x6e, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6a, 0x6b, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x60, 0x61, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00, 0x65,
    0x66, 0x67, 0x68, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 0x70, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define kKeyQueueMask   0x7f
#define kKeyRelease     0x8000                   /* the queue entry is a key-up */
#define kKeyAsciiMask   0xff
#define kKeyHeld        1                        /* KeyArray: held now */
#define kKeyPressed     2                        /* KeyArray: pressed since last read */
#define kScanMask       0x7f
#define kScanExtended   0x80
#define kLParamExtended 0x1000000
#define kPMRemove       1                        /* PM_REMOVE */

#define VK_F10          0x79

#define kLeftButton     1
#define kRightButton    2
#define kMiddleButton   4

/* Queue one key; a full queue drops it. */
static void BurgerPostKey(Word32 key)
{
    long end;
    long next;

    if (key != 0) {
        end = BurgerKeyEnd;
        BurgerKeyBuffer[end] = key;
        next = (end + 1) & kKeyQueueMask;
        if (next != BurgerKeyStart)
            BurgerKeyEnd = next;
    }
}

/* The Windows scan code of a key message, bit 7 set for the extended keys. */
static Word ScanOf(const WinMsg *msg)
{
    Word32 lParam;
    Word scan;

    lParam = msg->lParam;
    scan = (lParam >> 16) & kScanMask;
    if (lParam & kLParamExtended)
        scan |= kScanExtended;
    return scan;
}

/* The keys that post a code of their own rather than a character. */
static inline Word32 SpecialKey(Word32 code, const WinMsg *msg)
{
    switch (code) {
    case 0x24: return 0x6091;                    /* VK_HOME */
    case 0x26: return 0x610b;                    /* VK_UP */
    case 0x21: return 0x6292;                    /* VK_PRIOR */
    case 0x25: return 0x6396;                    /* VK_LEFT */
    case 0x27: return 0x6415;                    /* VK_RIGHT */
    case 0x23: return 0x6593;                    /* VK_END */
    case 0x28: return 0x660a;                    /* VK_DOWN */
    case 0x22: return 0x6794;                    /* VK_NEXT */
    case 0x2d: return 0x6895;                    /* VK_INSERT */
    case 0x2e: return 0x6997;                    /* VK_DELETE */
    case 0x13: return 0x6e80;                    /* VK_PAUSE */
    case 0x70: return 0x3b81;                    /* VK_F1 */
    case 0x71: return 0x3c82;
    case 0x72: return 0x3d83;
    case 0x73: return 0x3e84;
    case 0x74: return 0x3f85;
    case 0x75: return 0x4086;
    case 0x76: return 0x4187;
    case 0x77: return 0x4288;
    case 0x78: return 0x4389;
    case 0x79: return 0x448a;
    case 0x7a: return 0x578b;
    case 0x7b: return 0x588c;                    /* VK_F12 */
    case 0x91: return 0x468d;                    /* VK_SCROLL */
    case 0x2a: return 0x6d8f;                    /* VK_PRINT */
    case VK_CONTROL:
        if ((msg->lParam & 0x1ff0000) == 0x1d0000)
            return 0x1d00;                       /* left control */
        return 0x6b00;
    case VK_SHIFT:
        if ((msg->lParam & 0xff0000) == 0x360000)
            return 0x3600;                       /* right shift */
        return 0x2a00;
    case VK_MENU:
        if ((msg->lParam & 0x1ff0000) == 0x380000)
            return 0x3800;                       /* left alt */
        return 0x6f00;
    }
    return 0;
}

/* KeyboardGetch (BurgerLib InKeyboardWin95). STATE: MATCH 100%, exe 23 / obj 23,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x004d7240
unsigned int KeyboardGetch(void)
{
    long i;
    Word32 key;

    do {
        if (BurgerKeyStart == BurgerKeyEnd) {
            while (KeyboardKbhit() == 0)
                ;
        }
        i = BurgerKeyStart;
        key = BurgerKeyBuffer[i];
        BurgerKeyStart = (i + 1) & kKeyQueueMask;
    } while (key & kKeyRelease);
    key &= kKeyAsciiMask;
    if (KeyboardGetchCallBack != 0)
        key = KeyboardGetchCallBack(key);
    return key;
}

/* KeyboardKbhit (BurgerLib InKeyboardWin95). STATE: MATCH 100%, exe 19 / obj 19,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x004d72a0
unsigned int KeyboardKbhit(void)
{
    long i;
    Word32 key;

    ProcessSystemEvents();
    KeyboardCallPollingProcs();
    for (i = BurgerKeyStart; i != BurgerKeyEnd; i = (i + 1) & kKeyQueueMask) {
        key = BurgerKeyBuffer[i];
        if ((key & kKeyRelease) == 0) {
            key &= kKeyAsciiMask;
            if (key != 0)
                return key;
        }
    }
    return 0;
}

/* KeyboardGet2 (BurgerLib InKeyboardWin95). STATE: MATCH 100%, exe 14 / obj 14,
   byte-identical yes. OPEN: none. */
// FUNCTION: EVNOVA 0x004d72f0
long KeyboardGet2(void)
{
    long i;

    ProcessSystemEvents();
    KeyboardCallPollingProcs();
    i = BurgerKeyStart;
    if (i != BurgerKeyEnd) {
        BurgerKeyStart = (i + 1) & kKeyQueueMask;
        return BurgerKeyBuffer[i];
    }
    return 0;
}

/* Win95ProcessMessage (BurgerLib InKeyboardWin95). STATE: MATCH 100%, exe 361 / obj 361,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7330
Word Win95ProcessMessage(WinMsg *msg)
{
    Word32 key;

    if (msg->message == RWMMouseWheel)
        Win95MouseWheel += (short)msg->wParam;
    switch (msg->message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        key = SpecialKey(msg->wParam, msg);
        if (key != 0)
            BurgerPostKey(key);
        KeyArray[NormalScan[ScanOf(msg)]] |= kKeyHeld | kKeyPressed;
        if (msg->message == WM_SYSKEYDOWN &&
            (msg->wParam == VK_MENU || msg->wParam == VK_F10))
            return 1;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = SpecialKey(msg->wParam, msg);
        if (key != 0)
            BurgerPostKey(key | kKeyRelease);
        else
            BurgerPostKey((NormalScan[ScanOf(msg)] << 8) + (msg->wParam & kKeyAsciiMask) + kKeyRelease);
        KeyArray[NormalScan[ScanOf(msg)]] &= ~kKeyHeld;
        break;
    case WM_CHAR:
    case WM_SYSCHAR:
        BurgerPostKey((NormalScan[ScanOf(msg)] << 8) + (msg->wParam & kKeyAsciiMask));
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        Win95MouseButton |= kRightButton;
        MouseClicked |= kRightButton;
        if (msg->hwnd != 0)
            SetCapture(msg->hwnd);
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
        Win95MouseButton |= kMiddleButton;
        MouseClicked |= kMiddleButton;
        if (msg->hwnd != 0)
            SetCapture(msg->hwnd);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        Win95MouseButton |= kLeftButton;
        MouseClicked |= kLeftButton;
        if (msg->hwnd != 0)
            SetCapture(msg->hwnd);
        break;
    case WM_RBUTTONUP:
        Win95MouseButton &= ~kRightButton;
        ReleaseCapture();
        break;
    case WM_MBUTTONUP:
        Win95MouseButton &= ~kMiddleButton;
        ReleaseCapture();
        break;
    case WM_LBUTTONUP:
        Win95MouseButton &= ~kLeftButton;
        ReleaseCapture();
        break;
    case WM_MOUSEWHEEL:
        Win95MouseWheel += (long)msg->wParam >> 16;
        break;
    case WM_MOUSEMOVE:
        Win95MouseX = (Word16)msg->lParam;
        Win95MouseY = msg->lParam >> 16;
        Win95MouseXDelta += Win95MouseX - Win95LastMouseX;
        Win95MouseYDelta += Win95MouseY - Win95LastMouseY;
        Win95LastMouseX = Win95MouseX;
        Win95LastMouseY = Win95MouseY;
        break;
    }
    return 0;
}

/* ProcessSystemEvents (BurgerLib InKeyboardWin95). STATE: MATCH 100.0%, exe 47 / obj 47,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d79a0
void ProcessSystemEvents(void)
{
    WinMsg msg;

    if (Win95NoProcess == 0 && Recurse == 0 && VideoPageLocked == 0) {
        Recurse = 1;
        while (PeekMessageA(&msg, 0, 0, 0, kPMRemove) != 0) {
            if (Win95ProcessMessage(&msg) == 0) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        Recurse = 0;
    }
}

/* Reset the input subsystem: forget every key, drop the wheel state and
   re-register the mouse-wheel message. */
/* KeyboardInit (BurgerLib KeyboardInit). STATE: MATCH 100.0%, exe 13 / obj 13,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7a80
void KeyboardInit(void)
{
    FastMemSet(KeyArray, 0, kKeyTableSize);
    BurgerKeyStart = 0;
    BurgerKeyEnd = 0;
    MouseClicked = 0;
    RWMMouseWheel = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
}

/* KeyboardDestroy (BurgerLib KeyboardDestroy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d7ae0
void KeyboardDestroy(void)
{
}

#ifdef __cplusplus
}
#endif
