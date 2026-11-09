/* itinputmac.c -- 1.0.10: the interface tool's input state on Mac: modifier keys and mouse buttons from low memory, mouse and key events. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include <LowMem.h>

#define kModDownLeft    0x00010000
#define kModDownHeld    0x10000000
#define kModMouseMask   0x00070000
#define kModAlways      0x80000000
#define kModRepeat      0x40000000
#if TARGET_API_MAC_CARBON
#define kSetWindowPort SetPortWindowPort
#else
#define kSetWindowPort SetPort
#endif
#define kInputBytes     0x18
#define kKeyBytes       0xe

extern void _AtomicEnter(void);
extern void _AtomicLeave(void);
extern ITPoint Sys2DT_Point(const Point *pt);
extern unsigned char gPlatformThreaded;

/* the Mac's virtual key code to the tool's */
unsigned char gIT_SysKey2VirtKey[256] = {
    0x0b, 0x0c, 0x0d, 0x0e, 0x10, 0x0f, 0x14, 0x15, 0x16, 0x17, 0x00, 0x18, 0x01, 0x02, 0x03, 0x04,
    0x06, 0x05, 0x21, 0x22, 0x23, 0x24, 0x26, 0x25, 0x42, 0x29, 0x27, 0x41, 0x28, 0x20, 0x44, 0x09,
    0x07, 0x43, 0x08, 0x0a, 0x91, 0x13, 0x11, 0x47, 0x12, 0x46, 0x45, 0x48, 0x4a, 0x19, 0x1a, 0x49,
    0x92, 0x90, 0x40, 0x93, 0x00, 0xa0, 0x00, 0x84, 0x81, 0x80, 0x83, 0x82, 0x8a, 0x8b, 0x8a, 0x00,
    0x00, 0x55, 0x00, 0x52, 0x00, 0x54, 0x00, 0x94, 0x00, 0x00, 0x00, 0x51, 0x95, 0x00, 0x53, 0x00,
    0x00, 0x50, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x00, 0x38, 0x39, 0x00, 0x00, 0x00,
    0xa5, 0xa6, 0xa7, 0xa3, 0xa8, 0xa9, 0x00, 0xab, 0x00, 0xad, 0x00, 0xae, 0x00, 0xaa, 0x00, 0xac,
    0x00, 0xaf, 0x96, 0x98, 0x9a, 0x97, 0xa4, 0x99, 0xa2, 0x9b, 0xa1, 0x9c, 0x9e, 0x9d, 0x9f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static ULong _InputReadModifiers(void *hwnd)
{
    ULong modifiers;
#if TARGET_API_MAC_CARBON
    KeyMap keys;
    const char *map;
#endif
    long bits;

    modifiers = 0;
    _AtomicEnter();
#if TARGET_API_MAC_CARBON
    if (gPlatformThreaded) {
        GetKeys(keys);
        map = (const char *)keys;
    } else {
        map = (const char *)0x174;
    }
    if ((map[6] >> 7) & 1)
        modifiers |= 0x10;
    bits = map[7];
#else
    if ((*(unsigned char *)0x17a >> 7) & 1)
        modifiers |= 0x10;
    bits = *(char *)0x17b;
#endif
    if ((bits >> 2) & 1)
        modifiers |= 8;
    if ((bits >> 3) & 1)
        modifiers |= 4;
    if (bits & 1)
        modifiers |= 2;
    if ((bits >> 1) & 1)
        modifiers |= 1;
#if TARGET_API_MAC_CARBON
    if (gPlatformThreaded) {
        if (Button())
            modifiers |= kModDownLeft;
        if (StillDown())
            modifiers |= kModDownHeld;
    } else
#endif
    if ((*(unsigned char *)0x172 ^ 0x80) != 0) {
        const QHdr *queue;
        const EvQEl *el;

#if TARGET_API_MAC_CARBON
        queue = (const QHdr *)0x14a;
#else
        queue = LMGetEventQueue();
#endif
        modifiers |= kModDownLeft | kModDownHeld;
        el = (const EvQEl *)queue->qHead;
        while (el != 0) {
            if (el->evtQWhat == mouseUp) {
                modifiers &= ~kModDownHeld;
                break;
            }
            el = el == (const EvQEl *)queue->qTail ? 0 : (const EvQEl *)el->qLink;
        }
    }
    _AtomicLeave();
    return modifiers | kModAlways;
}

unsigned char InputReadMouseState(void *hwnd, ITInputEvent *event)
{
    WindowPtr window;
    GrafPtr old;
    Point pt;

    MemoryClear(event, kInputBytes);
    _AtomicEnter();
#if TARGET_API_MAC_CARBON
    if (gPlatformThreaded) {
        GetMouse(&pt);
        LocalToGlobal(&pt);
    } else
#endif
    *(long *)&pt = *(long *)0x830;
    _AtomicLeave();
    event->where = Sys2DT_Point(&pt);
    event->screen = event->where;
    window = hwnd;
    if (hwnd != 0 || FindWindow(pt, &window) == inContent) {
        GetPort(&old);
        kSetWindowPort(window);
        GlobalToLocal(&pt);
        event->where = Sys2DT_Point(&pt);
        SetPort(old);
    }
    event->hwnd = window;
    event->modifiers = _InputReadModifiers(hwnd);
    return (event->modifiers & kModMouseMask) != 0;
}

unsigned char MacEventToMouseEvent(const EventRecord *mac, ITInputEvent *event)
{
    unsigned char is;
    WindowPtr window;
    GrafPtr old;
    Point pt;

    is = 0;
    MemoryClear(event, kInputBytes);
    if (mac->what == mouseDown) {
        *(long *)&pt = *(const long *)&mac->where;
        event->where = Sys2DT_Point(&pt);
        event->screen = event->where;
        if (FindWindow(pt, &window) == inContent) {
            GetPort(&old);
            kSetWindowPort(window);
            GlobalToLocal(&pt);
            event->where = Sys2DT_Point(&pt);
            SetPort(old);
        }
        event->modifiers |= kModDownLeft;
        is = 1;
    }
    if ((mac->modifiers & alphaLock) != 0)
        event->modifiers |= 1;
    if ((mac->modifiers & shiftKey) != 0)
        event->modifiers |= 2;
    if ((mac->modifiers & controlKey) != 0)
        event->modifiers |= 4;
    if ((mac->modifiers & optionKey) != 0)
        event->modifiers |= 8;
    if ((mac->modifiers & cmdKey) != 0)
        event->modifiers |= 0x10;
    return is;
}

unsigned char MacEventToKeyEvent(const EventRecord *mac, ITKeyEvent *key)
{
    unsigned char is;

    is = 0;
    MemoryClear(key, kKeyBytes);
    if (mac->what == keyDown || mac->what == autoKey) {
        key->ch = mac->message;
        key->vk = (mac->message >> 8) & 0xff;
        key->key = gIT_SysKey2VirtKey[key->vk];
        key->code = (char)key->ch;
        if (mac->what == autoKey)
            key->modifiers |= kModRepeat;
        is = 1;
    }
    if ((mac->modifiers & alphaLock) != 0)
        key->modifiers |= 1;
    if ((mac->modifiers & shiftKey) != 0)
        key->modifiers |= 2;
    if ((mac->modifiers & controlKey) != 0)
        key->modifiers |= 4;
    if ((mac->modifiers & optionKey) != 0)
        key->modifiers |= 8;
    if ((mac->modifiers & cmdKey) != 0)
        key->modifiers |= 0x10;
    if ((mac->modifiers & btnState) != 0)
        key->modifiers |= kModDownLeft;
    return is;
}
