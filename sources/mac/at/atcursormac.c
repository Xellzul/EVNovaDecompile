/* atcursormac.c -- 1.0.10: the interface tool's open and close, event handler, cursor, browser and languages on Mac. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include <InternetConfig.h>
#include <AppleEvents.h>
#include <Appearance.h>
#include <CFPreferences.h>

#if TARGET_API_MAC_CARBON
#define kSetWindowPort SetPortWindowPort
#else
#define kSetWindowPort SetPort
#endif
#define kNoMemory       -7004
#define kIdleTicks      6
#define kBeepTicks      7
#define kKeyCommand     cmdKey
#define kItemsProcInfo  0x340

extern pascal void IT_FrameRectHook(signed char verb, const Rect *rect);
extern pascal void IT_FrameRgnHook(signed char verb, RgnHandle region);
extern long IT_ButtonHandler();
extern long IT_BorderHandler();
extern long IT_FrameHandler();
extern long IT_PopupHandler();
extern long IT_TextHandler();
extern WindowNode *IT_WindowFindView(void *hwnd);
extern unsigned char MacEventToMouseEvent(const EventRecord *mac, ITInputEvent *event);
extern unsigned char MacEventToKeyEvent(const EventRecord *mac, ITKeyEvent *key);
extern unsigned char InputReadMouseState(void *hwnd, ITInputEvent *event);
extern void _IT_SoundFlush(unsigned char wait);
extern long IT_ViewDispose();
extern long _IT_ViewHandleEdit(WindowNode *window, short edit, ITClick *click);
extern void *_PlatformFindSymbol(const char *library, const char *name);
extern unsigned char _PlatformIsThreaded(void);
extern long IT_ReadLanguages(char *languages);
extern void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName);
extern void _AtomicEnter(void);
extern void _AtomicLeave(void);

CQDProcs gIT_StdCProcs;
OSErr (*gIT_SetControlData)(ControlHandle control, short part, ResType tag, long size, void *data);
unsigned char gIT_Open = 0;
WindowNode *gIT_ViewList = 0;
WindowNode *gIT_CurrentView = 0;
ITControl *gIT_CurrentItem = 0;
ControlType *gIT_HandlerList = 0;
void *gIT_DefaultHash = 0;
unsigned char gIT_HasAppearance = 0;
unsigned char gIT_HasInternetConfig = 0;
Handle gIT_CursorHandle = 0;
struct ITSoundNode *gIT_SoundList = 0;
short gIT_CursorId = -1;
static ULong sLastIdle;
#if TARGET_API_MAC_CARBON
char gIT_DefaultAnchor[0x100] = "";
#endif

void IT_Idle(void);
void IT_Close(void);

long IT_Open(void)
{
    long err, e;
    long response;

    err = 0;
    if (gIT_Open)
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
    if (Gestalt(gestaltAppearanceAttr, &response) == 0) {
#if !TARGET_API_MAC_CARBON
        if ((gIT_SetControlData = _PlatformFindSymbol("AppearanceLib", "SetControlData")) != 0)
#endif
        if ((response & 1) != 0)
            gIT_HasAppearance = 1;
    }
    SetStdCProcs(&gIT_StdCProcs);
#if TARGET_API_MAC_CARBON
    if ((gIT_StdCProcs.rectProc = NewQDRectUPP((QDRectProcPtr)IT_FrameRectHook)) == 0) {
        err = kNoMemory;
        goto done;
    }
    if ((gIT_StdCProcs.rgnProc = NewQDRgnUPP(IT_FrameRgnHook)) == 0) {
        err = kNoMemory;
        goto done;
    }
    if (!_PlatformIsThreaded())
#else
    if ((gIT_StdCProcs.rectProc = NewRoutineDescriptor((ProcPtr)IT_FrameRectHook, kItemsProcInfo, kPowerPCISA)) == 0) {
        err = kNoMemory;
        goto done;
    }
    if ((gIT_StdCProcs.rgnProc = NewRoutineDescriptor((ProcPtr)IT_FrameRgnHook, kItemsProcInfo, kPowerPCISA)) == 0) {
        err = kNoMemory;
        goto done;
    }
#endif
    {
        ICInstance ic = 0;
        ProcessSerialNumber psn = { 0, kCurrentProcess };
        ProcessInfoRec info;
        FSSpec spec;
        Str255 name;

        info.processAppSpec = &spec;
        info.processInfoLength = sizeof(info);
        info.processName = name;
        if (GetProcessInformation(&psn, &info) != 0)
            info.processSignature = '????';
#if !TARGET_API_MAC_CARBON
        if (ICStart != 0)
#endif
        if (ICStart(&ic, info.processSignature) != 0)
            ic = 0;
        if (ic != 0) {
            ICStop(ic);
            gIT_HasInternetConfig = 1;
        }
    }
#if TARGET_API_MAC_CARBON
    IT_ReadLanguages(gIT_DefaultAnchor);
#endif
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

void IT_Idle(void)
{
    WindowNode *window;

#if TARGET_API_MAC_CARBON
    IT_ViewCheckWindows(0);
#endif
    _IT_SoundFlush(0);
    window = gIT_ViewList;
    while (window != 0) {
        if (window->dirty)
            _IT_ViewValidate(window);
        _IT_ViewHandleIdle(window);
        window = window->next;
    }
}

void IT_Close(void)
{
    WindowNode *window;
    ControlType *handler;

#if TARGET_API_MAC_CARBON
    IT_ViewCheckWindows(0);
#endif
    while ((window = gIT_ViewList) != 0) {
        while (window->parent != 0)
            window = window->parent;
        IT_ViewDispose(window);
    }
    while ((handler = gIT_HandlerList) != 0) {
        gIT_HandlerList = handler->next;
        MemoryDeallocate(handler);
    }
    if (gIT_DefaultHash != 0) {
        HashDispose(gIT_DefaultHash);
        gIT_DefaultHash = 0;
    }
    _IT_SoundFlush(1);
    InitCursor();
    if (gIT_CursorHandle != 0) {
        DisposeHandle(gIT_CursorHandle);
        gIT_CursorHandle = 0;
    }
    gIT_CursorId = -1;
    gIT_Open = 0;
}

unsigned char IT_IsOpen(void)
{
    return gIT_Open;
}

/* the Mac's key event is 2 bytes longer than the shared record: MacEventToKeyEvent clears 0xe */
typedef struct MacKeyEvent {
    ITKeyEvent event;
    short pad;
} MacKeyEvent;

long IT_HandleMacEvent(EventRecord *event, short edit, short *cursor)
{
    long err = 0;
    WindowNode *view;
    WindowPtr window;
    GrafPtr port;
    MacKeyEvent key;
    ITInputEvent mouse;
    ITInputEvent state;
    ITClick click = { 0 };

    window = 0;
#if TARGET_API_MAC_CARBON
    IT_ViewCheckWindows(0);
#endif
    if (gIT_Open) {
        switch (event->what) {
        case nullEvent:
            if (TickCount() - sLastIdle >= kIdleTicks) {
                sLastIdle = TickCount();
                SystemIdle();
            }
            break;
        case mouseDown:
            ShowCursor();
            switch (FindWindow(event->where, &window)) {
            case inMenuBar:
                window = FrontWindow();
                if (window != 0 && (view = IT_WindowFindView(window)) != 0) {
                    long e;

                    e = _IT_ViewHandleEdit(view, edit, &click);
                    if (e != 0) {
                        err = e;
                        goto finish;
                    }
                }
                break;
            case inContent:
                if ((view = IT_WindowFindView(window)) != 0) {
                    long e;

                    MacEventToMouseEvent(event, &mouse);
                    e = _IT_ViewHandleClick(view, &mouse, &click);
                    if (e != 0) {
                        err = e;
                        goto finish;
                    }
                }
                break;
            case inDrag:
                if ((view = IT_WindowFindView(window)) != 0 && view->titled) {
#if TARGET_API_MAC_CARBON
                    _AtomicEnter();
                    DragWindow(window, event->where, 0);
                    _AtomicLeave();
#else
                    DragWindow(window, event->where, &qd.screenBits.bounds);
#endif
                    click.followed = 1;
                }
                break;
            case inGrow:
                if (IT_WindowFindView(window) != 0)
                    SysBeep(kBeepTicks);
                break;
            case inGoAway:
                if ((view = IT_WindowFindView(window)) != 0) {
                    if (TrackGoAway(window, event->where))
                        IT_ViewDispose(view);
                    click.followed = 1;
                }
                break;
            case inZoomIn:
                if (IT_WindowFindView(window) != 0)
                    SysBeep(kBeepTicks);
                break;
            case inZoomOut:
                if (IT_WindowFindView(window) != 0)
                    SysBeep(kBeepTicks);
                break;
            }
            break;
        case keyDown:
        case autoKey:
            window = FrontWindow();
            if (window != 0 && (view = IT_WindowFindView(window)) != 0) {
                long e;

                if (event->what != autoKey && (event->modifiers & cmdKey) != 0) {
                    e = _IT_ViewHandleEdit(view, edit, &click);
                    if (e != 0) {
                        err = e;
                        goto finish;
                    }
                }
                if (!click.followed && ((event->modifiers & cmdKey) == 0 || MenuKey((unsigned char)event->message) == 0)) {
                    MacEventToKeyEvent(event, &key.event);
                    e = _IT_ViewHandleKey(view, &key.event, &click);
                    if (e != 0) {
                        err = e;
                        goto finish;
                    }
                }
            }
            break;
        case updateEvt:
            window = (WindowPtr)event->message;
            if (window != 0 && (view = IT_WindowFindView(window)) != 0) {
                GetPort(&port);
                kSetWindowPort(window);
#if TARGET_API_MAC_CARBON
                {
                    Rect bounds;

                    GetWindowPortBounds(window, &bounds);
                    InvalWindowRect(window, &bounds);
                }
#else
                InvalRect(&window->portRect);
#endif
                BeginUpdate(window);
                err = _IT_ViewHandleDraw(view, 0, 0);
                EndUpdate(window);
                SetPort(port);
                if (err != 0)
                    goto finish;
                click.followed = 1;
            }
            break;
        case activateEvt:
            window = (WindowPtr)event->message;
            if (window != 0 && (view = IT_WindowFindView(window)) != 0) {
                GetPort(&port);
                kSetWindowPort(window);
                err = _IT_ViewHandleActivate(view, event->modifiers & activeFlag);
                SetPort(port);
                if (err != 0)
                    goto finish;
                click.followed = 1;
            }
            break;
        case osEvt:
            if ((event->message & 0x01000000) != 0) {
                window = FrontWindow();
                if (window != 0 && (view = IT_WindowFindView(window)) != 0) {
                    GetPort(&port);
                    kSetWindowPort(window);
                    err = _IT_ViewHandleActivate(view, event->message & 1);
                    SetPort(port);
                    if (err != 0)
                        goto finish;
                    click.followed = 1;
                }
            }
            break;
        case kHighLevelEvent:
            AEProcessAppleEvent(event);
            click.followed = 1;
            err = 0;
            break;
        }
    }
    view = 0;
    if (cursor != 0 && FindWindow(event->where, &window) == inContent && window == FrontWindow() && (view = IT_WindowFindView(window)) != 0) {
        long e;

        InputReadMouseState(0, &state);
        e = _IT_ViewHandleCursor(view, &state, (unsigned short *)cursor);
        if (e != 0)
            err = e;
    } else if (cursor != 0 && view == 0) {
        *cursor = 0;
    }
finish:
    if (click.followed)
        event->what = nullEvent;
    return err;
}

static Cursor gIT_WatchCursor = {
    { 0x0380, 0x04c0, 0x04c0, 0x04c0, 0x04c0, 0x04c0, 0x74f8, 0x9cae,
      0x4cab, 0x240b, 0x2403, 0x1003, 0x0803, 0x0806, 0x0406, 0x0406 },
    { 0x0380, 0x07c0, 0x07c0, 0x07c0, 0x07c0, 0x07c0, 0x77f8, 0xfffe,
      0x7fff, 0x3fff, 0x3fff, 0x1fff, 0x0fff, 0x0ffe, 0x07fe, 0x07fe },
    { 0, 7 }
};

long IT_SetCursor(short cursorId)
{
    long err;
    Handle h;
    CursHandle c;

    err = 0;
    c = 0;
    if (cursorId == gIT_CursorId || cursorId == -1)
        goto done;
    if (cursorId == 0) {
        InitCursor();
    } else if (cursorId == 5) {
        SetCursor(&gIT_WatchCursor);
        gIT_CursorId = cursorId;
    } else {
        c = GetCursor(cursorId);
        if (c == 0) {
            ResError();
            err = -7005;
            goto done;
        }
        if (HandToHand((Handle *)&c) != 0) {
            err = -7000;
            goto done;
        }
        HLockHi((Handle)c);
        SetCursor(*c);
        gIT_CursorId = cursorId;
    }
    if (gIT_CursorHandle != 0)
        DisposeHandle(gIT_CursorHandle);
    gIT_CursorId = cursorId;
    gIT_CursorHandle = (Handle)c;
    c = 0;
done:
    return err;
}

#define kBrowserFailed  -8402
#define kWaitCursor     4

long IT_OpenBrowser(const char *url)
{
    long err = 0;
    long e;
    void **h;
#if TARGET_API_MAC_CARBON
    CFStringRef string = 0;
    CFURLRef location = 0;
    long (*open)(CFURLRef url, void *out);
#endif
    long selStart = 0;
    long selEnd = 0;
    ICInstance ic = 0;
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    ProcessInfoRec info;
    FSSpec spec;
    Str255 name;

#if TARGET_API_MAC_CARBON
    IT_ViewCheckWindows(0);
#endif
    h = IndirectInitialize(url, StringGetLength((const unsigned char *)url));
    if (h == 0) {
        err = kNoMemory;
        goto done;
    }
    e = PT_ParseEncode(h, 0, 1, 1, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IndirectAppendData(h, "", 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectSetLock(h, 1);
    url = *h;
    IT_SetCursor(kWaitCursor);
#if TARGET_API_MAC_CARBON
    if (_PlatformIsThreaded() && (open = _PlatformFindBundleSymbol(CFSTR("CoreServices.framework"), CFSTR("LSOpenCFURLRef"))) != 0) {
        if ((string = CFStringCreateWithCString(0, url, kCFStringEncodingASCII)) == 0) {
            err = kNoMemory;
            goto done;
        }
        if ((location = CFURLCreateWithString(0, string, 0)) == 0) {
            err = kNoMemory;
            goto done;
        }
        if (open(location, 0) != 0)
            err = kBrowserFailed;
        goto done;
    }
#endif
    if (!gIT_HasInternetConfig) {
        err = 0;
        goto done;
    }
    info.processAppSpec = &spec;
    info.processInfoLength = sizeof(info);
    info.processName = name;
    if (GetProcessInformation(&psn, &info) != 0)
        info.processSignature = '????';
    if (ICStart(&ic, info.processSignature) != 0) {
        err = kBrowserFailed;
        goto done;
    }
#if !TARGET_API_MAC_CARBON
    if (ICFindConfigFile(ic, 0, 0) != 0) {
        err = kBrowserFailed;
        goto done;
    }
#endif
    if (ICLaunchURL(ic, 0, url, StringGetLength((const unsigned char *)url), &selStart, &selEnd) != 0)
        err = kBrowserFailed;
done:
#if TARGET_API_MAC_CARBON
    if (location != 0)
        CFRelease(location);
    if (string != 0)
        CFRelease(string);
#endif
    if (ic != 0)
        ICStop(ic);
    if (err != 0)
        SysBeep(kBeepTicks);
    InitCursor();
    return err;
}

#if TARGET_API_MAC_CARBON
extern const CFStringRef kCFPreferencesCurrentApplication;

#define kLanguagesFailed    -8400
#define kMaxLanguages       5

long IT_ReadLanguages(char *languages)
{
    long err = 0;
    CFArrayRef list;
    CFIndex i;
    CFIndex count;
    CFStringRef language;
    char name[0x100];
    char *region;

    languages[0] = 0;
    list = CFPreferencesCopyAppValue(CFSTR("AppleLanguages"), kCFPreferencesCurrentApplication);
    if (list == 0) {
        err = kLanguagesFailed;
        goto done;
    }
    count = CFArrayGetCount(list);
    if ((unsigned long)count > kMaxLanguages)
        count = kMaxLanguages;
    for (i = 0; i < (unsigned long)count; i++) {
        language = CFArrayGetValueAtIndex(list, i);
        if (language != 0) {
            if (!CFStringGetCString(language, name, sizeof(name), kCFStringEncodingASCII)) {
                err = kLanguagesFailed;
                goto done;
            }
            region = StringFindDelimiter(name, "_");
            if (region != 0)
                *region = 0;
            if (StringGetLength((const unsigned char *)languages) + StringGetLength((const unsigned char *)",") + StringGetLength((const unsigned char *)name) >= sizeof(name))
                goto done;
            if (languages[0] != 0)
                StringAppendSafe((unsigned char *)languages, (const unsigned char *)",", sizeof(name));
            StringAppendSafe((unsigned char *)languages, (const unsigned char *)name, sizeof(name));
        }
    }
done:
    if (list != 0)
        CFRelease(list);
    return err;
}
#endif
