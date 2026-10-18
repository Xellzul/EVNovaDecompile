// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atplatopen.c -- the platform layer: open and close, the per-thread depth, the idle and quit callback tables, the idle sleep, and the window with its process-level handlers. Notes: Research/notes/atplatopen.md */

#include "burgerplat.h"
#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

typedef void (*PlatformCallback)(void);
typedef long (__stdcall *WindowProc)(void *window, ULong message,
                                     ULong wparam, long lparam);

typedef struct MessageHook {
    struct MessageHook *next;
    WindowProc proc;
    ULong message;
} MessageHook;

extern void exit(int code);

void _ClockTimerProc(PlatformTimer *timer, void *arg);         /* 0x004d3927 */
extern long ThreadsPreflight(long count, ULong stackSize);      /* atthreadtbl.c */
extern long TimerCreate(void *ref, void *arg, TimerProc proc);
extern long _EncodingPreflight(void);
extern void _AtomicResetCritical(void);                  /* atplatlock2.c */

extern PlatformTimer gClockTimer;               /* atticks.c */

long _PlatformCreateWindow(void);                         /* 0x004d376d */
int __stdcall SystemSignalHandle(ULong type);          /* 0x004d38c2 */
long __stdcall _SystemExceptionFilter(void *info);         /* 0x004d38f1 */
void SystemLeaveCallback(int unlock);                   /* 0x004d3589 */
long __stdcall _PlatformWindowProc(void *window, ULong message, ULong wparam,
                            long lparam);       /* 0x004d3667 */
void *_PlatformFindConsoleWindow(void);                        /* 0x004d36b9 */

#define kSeedBits     32
#define kEpochBackoff 1000

#define kCallbackSlots  50
#define kIdleSliceMs    50

#define kNameBufferSize     0x400
#define kUniqueLen          0xff
#define kFindTries          10
#define kFindWaitMs         20
#define kOffscreenPos       0x7d00
#define kHiddenSize         0x64

#define COLOR_WINDOW        5
#define CTRL_C_EVENT        0
#define CTRL_BREAK_EVENT    1
#define CTRL_CLOSE_EVENT    2
#define CTRL_LOGOFF_EVENT   5
#define CTRL_SHUTDOWN_EVENT 6
#define STATUS_CONTROL_C_EXIT       0xc000013a
#define EXCEPTION_EXECUTE_HANDLER   1
#define EXCEPTION_CONTINUE_EXECUTION (-1)

ULong gInterruptIndex = kNoTlsIndex;                /* 0x00576140 TLS index */
const void *gRT3_IsRegisteredPtr1 = "";          /* 0x00576148 license data, rt3.c's */
void *gPlatWindow = (void *)0xffffffff;         /* 0x0057614c window */
ULong gPlatMainThreadId = kNoTlsIndex;                /* 0x00576150 main thread */

static ULong gSystemIdleCount = 0;                   /* 0x00865048 idle count */
static ULong gSystemQuitCount = 0;                   /* 0x0086504c close count */
static unsigned char gSystemQuitRecurse = 0;           /* 0x00865050 closing */
static PlatformCallback gSystemIdleProcs[kCallbackSlots];   /* 0x00865550 idle table */
static unsigned char gSystemIdleRecurse[kCallbackSlots];      /* 0x0086551c idle busy */
static PlatformCallback gSystemQuitProcs[kCallbackSlots];   /* 0x00865454 close table */

void *gProcessHeap;                              /* 0x00870108 heap */
char gPlatCritSec[24];                           /* 0x0086e408 critsec */
unsigned char gCriticalSectionReady;                      /* 0x00870e0f */
void *(__stdcall *gPlatSetUnhandledFilterFn)(void *filter);   /* 0x008705d4 */
void *gPlatInitCSSpin;                              /* 0x0086fff4 */
void *gPlatCmpXchgProc;                              /* 0x008705cc */
void *gPlatExchangeAddProc;                              /* 0x00870104 */
void *gPlatSavedExceptionFilterProc;                              /* 0x00870710 old filter */
ULongLong gPlatformTime;                          /* 0x0086e660 */
ULong gRandomSeed;                              /* 0x00870058 seed */
unsigned char gPlatformOpen;                      /* 0x00870e10, 1 B */
long gInterruptCount;                            /* 0x00870088 */
MessageHook *gMsgHooksList;                       /* 0x00870718 hook list */

/* PlatformOpen. STATE: MATCH 100%, exe 117 / obj 117 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d3163
long PlatformOpen(void)
{
    long err, e;
    ULong shift;
    ULong seed;

    err = 0;
    SystemCheckCallback();
    if (gPlatformOpen != 0)
        goto done;

    gInterruptIndex = TlsAlloc();
    if (gInterruptIndex == kNoTlsIndex) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }

    gProcessHeap = GetProcessHeap();
    gPlatMainThreadId = GetCurrentThreadId();
    InitializeCriticalSection(gPlatCritSec);
    gCriticalSectionReady = 1;

    gPlatSetUnhandledFilterFn = (void *(__stdcall *)(void *))GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "SetUnhandledExceptionFilter");
    gPlatInitCSSpin = GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                  "InitializeCriticalSectionAndSpinCount");
    gPlatCmpXchgProc = GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                  "InterlockedCompareExchange");
    gPlatExchangeAddProc = GetProcAddress(GetModuleHandleA("kernel32.dll"),
                                  "InterlockedExchangeAdd");

    if (gPlatWindow == (void *)-1) {
        e = _PlatformCreateWindow();
        if (e != 0) {
            err = e;
            goto done;
        }
    }

    e = atexit(_SystemQuitCallback);
    if (e != 0) {
        err = e;
        err = kOutOfMemory;
        goto done;
    }

    if (SetConsoleCtrlHandler(SystemSignalHandle, 1) == 0) {
        err = GetLastError();
        err = kOutOfMemory;
        goto done;
    }

    if (gPlatSetUnhandledFilterFn != 0)
        gPlatSavedExceptionFilterProc = gPlatSetUnhandledFilterFn(_SystemExceptionFilter);

    gPlatformOpen = 1;
    gPlatformTime = TimerGetMilliseconds() - kEpochBackoff;

    shift = GetTickCount();
    shift %= kSeedBits;
    seed = (ULong)TimerGetMicroseconds();
    gRandomSeed = seed;
    gRandomSeed = (seed << shift) | (seed >> (kSeedBits - shift));

    e = ThreadsPreflight(0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = TimerCreate(&gClockTimer, 0, _ClockTimerProc);
    if (e != 0) {
        err = e;
        goto done;
    }
    gClockReadyFlag = 1;
    e = _EncodingPreflight();
    if (e != 0)
        err = e;

done:
    if (err != 0 && gPlatformOpen != 0)
        PlatformClose();
    return err;
}

/* PlatformClose. STATE: MATCH 100%, exe 11 / obj 11, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d332d
void PlatformClose(void)
{
    long err;

    err = 0;
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
    if (gPlatformOpen != 0) {
        _SystemQuitCallback();
        gPlatformOpen = 0;
    }
}

/* IsPlatformOpen. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d3356
unsigned char IsPlatformOpen(void)
{
    return gPlatformOpen;
}

/* SystemIdle. STATE: MATCH 100%, exe 19 / obj 19, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d335c
void SystemIdle(void)
{
    ULong i;
    long err;

    err = 0;
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
    for (i = 0; i < gSystemIdleCount; i = i + 1) {
        if (gSystemIdleRecurse[i] == 0) {
            gSystemIdleRecurse[i] = 1;
            gSystemIdleProcs[i]();
            gSystemIdleRecurse[i] = 0;
        }
    }
}

/* Sleeps in alertable slices of at most 50 ms, running the idle table
   between them, until the millisecond clock passes the deadline. */
/* SystemWait. STATE: MATCH 100%, exe 60 / obj 60, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d339e
void SystemWait(ULong ms)
{
    ULongLong deadline;
    long long remaining;
    ULongLong slice;
    long err;

    err = 0;
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
    if (ms != 0) {
        deadline = TimerGetMilliseconds() + ms;
        remaining = ms;
        do {
            SystemIdle();
            if (remaining < kIdleSliceMs)
                slice = remaining;
            else
                slice = kIdleSliceMs;
            SleepEx(slice, 1);
            remaining = deadline - TimerGetMilliseconds();
        } while (remaining > 0);
    }
}

/* SystemSetIdleProc. STATE: MATCH 100%, exe 20 / obj 20, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d3457
void SystemSetIdleProc(PlatformCallback fn)
{
    long err;
    ULong i;

    err = 0;
    if (gSystemIdleCount == kCallbackSlots) {
        err = kTableFull;
    } else {
        for (i = 0; i < gSystemIdleCount; i = i + 1) {
            if (gSystemIdleProcs[i] == fn)
                break;
        }
        if (i == gSystemIdleCount) {
            gSystemIdleProcs[gSystemIdleCount] = fn;
            gSystemIdleCount = gSystemIdleCount + 1;
        }
    }
}

/* SystemSetQuitProc. STATE: MATCH 100%, exe 15 / obj 15, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d34a0
void SystemSetQuitProc(PlatformCallback fn)
{
    long err;
    ULong i;

    err = 0;
    if (gSystemQuitCount == kCallbackSlots) {
        err = kTableFull;
    } else {
        for (i = 0; i < gSystemQuitCount; i = i + 1) {
            if (gSystemQuitProcs[i] == fn)
                break;
        }
    }
}

/* _SystemQuitCallback. STATE: MATCH 100%, exe 49 / obj 49, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d34cf
void _SystemQuitCallback(void)
{
    PlatformCallback fn;
    long err;

    err = 0;
    if (gSystemQuitRecurse != 0)
        return;
    gSystemQuitRecurse = 1;
    gInterruptCount = 0;

    if (SystemCheckCallback() != 0) {
        while (SystemCheckCallback() != 0)
            SystemLeaveCallback(1);
    }

    if (_AtomicCheckCritical() != 0)
        _AtomicResetCritical();
    _ThreadCheckCritical();

    while (gSystemQuitCount != 0) {
        gSystemQuitCount = gSystemQuitCount - 1;
        fn = gSystemQuitProcs[gSystemQuitCount];
        if (fn != 0)
            fn();
    }

    if (gInterruptIndex != kNoTlsIndex) {
        TlsFree(gInterruptIndex);
        gInterruptIndex = kNoTlsIndex;
    }
    if (gCriticalSectionReady != 0) {
        gCriticalSectionReady = 0;
        DeleteCriticalSection(gPlatCritSec);
    }
    gClockReadyFlag = 0;
    gSystemQuitRecurse = 0;
}

/* Leaves one level of the calling thread's depth; with `unlock' set it
   also releases the section first. */
/* SystemLeaveCallback. STATE: MATCH 100%, exe 26 / obj 26, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3589
void SystemLeaveCallback(int unlock)
{
    ULong depth;
    long err;

    err = 0;
    depth = 0;
    TlsGetValue(gInterruptIndex);
    if ((unsigned char)unlock != 0 && gCriticalSectionReady != 0)
        LeaveCriticalSection(gPlatCritSec);
    if (gInterruptIndex != kNoTlsIndex) {
        depth = (ULong)TlsGetValue(gInterruptIndex);
        if (depth != 0) {
            if (TlsSetValue(gInterruptIndex, (void *)(depth - 1)) == 0)
                GetLastError();
        }
    }
}

/* True while the calling thread is inside the layer. */
/* SystemCheckCallback. STATE: MATCH 100%, exe 22 / obj 22, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d35ea
unsigned char SystemCheckCallback(void)
{
    long err;
    unsigned char result;
    int inside;

    err = 0;
    result = 0;
    inside = 0;
    if (gInterruptIndex != kNoTlsIndex) {
        if (TlsGetValue(gInterruptIndex) != 0)
            inside = 1;
    }
    result = inside != 0 ? true : false;
    return result;
}

/* _PlatformGetWindow. STATE: MATCH 100%, 6/6, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d361f
void *_PlatformGetWindow(void)
{
    return gPlatWindow == (void *)-1 ? 0 : gPlatWindow;
}

/* _PlatformSetWindow. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d3632
void _PlatformSetWindow(void *window)
{
    gPlatWindow = window;
}

/* _PlatformFindMessageHook. STATE: MATCH 100%, exe 18 / obj 18, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d363c
WindowProc _PlatformFindMessageHook(ULong message)
{
    long err;
    MessageHook *hook;

    err = 0;
    hook = 0;
    hook = gMsgHooksList;
    while (hook != 0 && hook->message != message)
        hook = hook->next;
    return hook != 0 ? hook->proc : 0;
}

/* _PlatformWindowProc. STATE: MATCH 100%, exe 38 / obj 38, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d3667
long __stdcall _PlatformWindowProc(void *window, ULong message, ULong wparam,
                            long lparam)
{
    WindowProc proc;

    proc = 0;
    switch (message) {
    case WM_ENDSESSION:
        if (wparam != 0)
            _SystemQuitCallback();
        break;
    default:
        proc = _PlatformFindMessageHook(message);
        break;
    }
    if (proc != 0)
        return proc(window, message, wparam, lparam);
    return DefWindowProcA(window, message, wparam, lparam);
}

/* The console window: retitle it to something unique, FindWindow that,
   then put the title back. */
/* _PlatformFindConsoleWindow. STATE: MATCH 100%, exe 52 / obj 52, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d36b9
void *_PlatformFindConsoleWindow(void)
{
    char title[kNameBufferSize];
    char probe[kNameBufferSize];
    char unique[kUniqueLen + 1];
    void *window;
    ULong tries;

    window = 0;
    GetConsoleTitleA(title, kNameBufferSize);
    MemoryFill(unique, ' ', kUniqueLen);
    unique[kUniqueLen] = 0;
    StringFormatSafe(probe, kNameBufferSize, "%s%s%ud", title, unique,
                 (ULong)TimerGetMicroseconds());
    SetConsoleTitleA(probe);
    tries = kFindTries;
    while (tries-- && (window = FindWindowA(0, probe)) == 0)
        Sleep(kFindWaitMs);
    SetConsoleTitleA(title);
    return window;
}

/* _PlatformCreateWindow. STATE: MATCH 100%, exe 94 / obj 94 instrs, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d376d
long _PlatformCreateWindow(void)
{
    long err = 0;
    char name[kNameBufferSize] = "";
    WNDCLASSA wc;
    void *window;
    char *base;
    char *sep;

    window = 0;
    window = GetForegroundWindow();
    if (window != 0)
        goto found;
    window = _PlatformFindConsoleWindow();
    if (window != 0)
        goto found;
    MemoryClear(&wc, sizeof(WNDCLASSA));
    wc.lpszClassName = "PLATFORM_WINDOW_CLASS";
    wc.hInstance = GetModuleHandleA(0);
    wc.lpfnWndProc = _PlatformWindowProc;
    wc.hCursor = CopyIcon(LoadCursorA(0, (const char *)IDC_ARROW));
    wc.hIcon = 0;
    wc.lpszMenuName = 0;
    wc.hbrBackground = (void *)(COLOR_WINDOW + 1);
    wc.style = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    if (RegisterClassA(&wc) == 0)
        goto failed;
    GetModuleFileNameA(GetModuleHandleA(0), base = name, kNameBufferSize);
    while ((sep = StringFindDelimiter(base, "\\")) != 0)
        base = sep + 1;
    window = CreateWindowExA(0, wc.lpszClassName, base, 0, kOffscreenPos, kOffscreenPos,
                             kHiddenSize, kHiddenSize, 0, 0, GetModuleHandleA(0), 0);
    if (window != 0)
        goto found;
failed:
    err = GetLastError();
    err = kOSFailure;
    goto done;
found:
    gPlatWindow = window;
done:
    return err;
}

/* SystemSignalHandle. STATE: MATCH 100%, exe 19 / obj 19 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d38c2
int __stdcall SystemSignalHandle(ULong type)
{
    switch (type) {
    case CTRL_C_EVENT:
        exit(0);
        break;
    case CTRL_BREAK_EVENT:
        break;
    case CTRL_CLOSE_EVENT:
        exit(0);
        break;
    case CTRL_LOGOFF_EVENT:
        exit(0);
        break;
    case CTRL_SHUTDOWN_EVENT:
        exit(0);
        break;
    }
    return 0;
}

/* _SystemExceptionFilter. STATE: MATCH 100%, exe 20 / obj 20, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d38f1
long __stdcall _SystemExceptionFilter(void *info)
{
    ULong *code;
    long result;

    code = (ULong *)info;
    result = EXCEPTION_EXECUTE_HANDLER;
    if (gPlatSavedExceptionFilterProc != 0)
        result = ((long (__stdcall *)(void *))gPlatSavedExceptionFilterProc)(info);
    if (code[0] != STATUS_CONTROL_C_EXIT || result != EXCEPTION_CONTINUE_EXECUTION)
        PlatformClose();
    return result;
}

/* The clock's proc: touch the counter, ask for another 4 s. */

/* _ClockTimerProc. STATE: MATCH 100%, exe 14 / obj 14 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3927
void _ClockTimerProc(PlatformTimer *timer, void *arg)
{
    long long counter;

    QueryPerformanceCounter(&counter);
    TimerPrime(timer, kClockPeriodMs);
}

/* _PlatformSuspendThread. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d394b
ULong _PlatformSuspendThread(void *thread)
{
    return SuspendThread(thread);
}

/* _PlatformResumeThread. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d3956
ULong _PlatformResumeThread(void *thread)
{
    return ResumeThread(thread);
}
