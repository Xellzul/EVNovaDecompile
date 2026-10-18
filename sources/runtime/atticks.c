// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atticks.c -- the platform layer's clocks and timers: seconds by the UTC and local clocks, the age of the layer, the tick count, the microsecond and millisecond clock, and the timer threads. Notes: Research/notes/atticks.md */

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "win32.h"

extern ULongLong gPlatformTime;                   /* atplatopen.c, clock at open */

long TimerDispose(void *ref);                    /* 0x004d41f1 */
ULong __stdcall _TimerThread(PlatformTimer *timer);     /* 0x004d435b */

#define kMicrosPerSecond 1000000
#define kDriftWindowMs   500
#define kNoRequest      -1

static unsigned char gTimerReady = 0;           /* 0x00865618 inited */
static long long gTimerFrequency = 0;               /* 0x00865620 frequency */
static long gTimerCorrection = 0;                    /* 0x00865628 correction ms */
static ULongLong gTimerLastMs = 0;               /* 0x00865630 last ms */
static ULong gTimerLastTick = 0;                   /* 0x00865638 last tick */
static PlatformTimer **gTimerList = 0;         /* 0x0086563c timer list */
static ULong gTimerCount = 0;                   /* 0x00865640 timer count */
static unsigned char gTimerInProc = 0;           /* 0x00865644 in proc */

unsigned char gClockReadyFlag;                      /* 0x00870e04 clock thread up */
/* The clock's timer: its proc re-arms it every 4 s while the thread runs. */
PlatformTimer gClockTimer;                      /* 0x0086d8b0 clock timer */

/* 0x00576200, the days before each month; its readers are not linked */
static short gMonths[13] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

/* the counter's advance since the last reading; the difference is taken in 64 bits */
static inline long MillisSince(long now)
{
    return now - gTimerLastMs;
}

/* TimerGetSeconds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d3961
long TimerGetSeconds(void)
{
    return TimerGetLocalSeconds();
}

/* Seconds since 1970 by the UTC clock; every fourth year a leap year. */
/* TimerGetUTCSeconds. STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3967
ULong TimerGetUTCSeconds(void)
{
    static short daysInMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    SYSTEMTIME st;
    long year;
    long days;
    long i;
    long err;

    err = 0;
    days = 0;
    GetSystemTime(&st);
    year = st.wYear - kEpochYear;
    days = (year + 1 + (st.wMonth > kFebruary ? 1 : 0)) / 4 + year * kDaysPerYear;
    for (i = 1; i < st.wMonth; i++)
        days += daysInMonth[i - 1];
    days += st.wDay - 1;
    return ((days * 24 + st.wHour) * 60 + st.wMinute) * 60 + st.wSecond;
}

/* The same by the local clock. */
/* TimerGetLocalSeconds. STATE: MATCH 100%, exe 51 / obj 51, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d39f6
ULong TimerGetLocalSeconds(void)
{
    static short daysInMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    SYSTEMTIME st;
    long year;
    long days;
    long i;
    long err;

    err = 0;
    days = 0;
    GetLocalTime(&st);
    year = st.wYear - kEpochYear;
    days = (year + 1 + (st.wMonth > kFebruary ? 1 : 0)) / 4 + year * kDaysPerYear;
    for (i = 1; i < st.wMonth; i++)
        days += daysInMonth[i - 1];
    days += st.wDay - 1;
    return ((days * 24 + st.wHour) * 60 + st.wMinute) * 60 + st.wSecond;
}

/* Whole seconds since the platform layer opened. */
/* TimerGetElapsedSeconds. STATE: MATCH 100%, exe 27 / obj 27, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3a85
unsigned long TimerGetElapsedSeconds(void)
{
    long err;
    double seconds;

    err = 0;
    seconds = TimerGetMilliseconds() - gPlatformTime;
    seconds = seconds / kMillisPerSecond;
    return (ULong)seconds;
}

/* Milliseconds since boot. */
/* TimerGetTicks. STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d3ae6
ULong TimerGetTicks(void)
{
    long err;
    ULong result;

    err = 0;
    result = 0;
    result = GetTickCount();
    return result;
}

/* The tick rate that goes with it. */
/* TimerGetTickRate. STATE: MATCH 100%, exe 7 / obj 7, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3af5
ULong TimerGetTickRate(void)
{
    long err;
    ULong result;

    err = 0;
    result = 0;
    result = kMillisPerSecond;
    return result;
}

/* TimerGetMicroseconds. STATE: MATCH 100%, exe 183 / obj 183, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3b08
ULongLong TimerGetMicroseconds(void)
{
    long elapsed;
    ULongLong micros;
    long long counter;
    long long freq;
    ULongLong millis;
    ULong tick;
    long drift;

    elapsed = 0;
    micros = 0;

    if (gTimerReady == 0) {
        QueryPerformanceFrequency(&freq);
        gTimerFrequency = freq;
        gTimerReady = 1;
        tick = GetTickCount();
        QueryPerformanceCounter(&counter);
        millis = gTimerFrequency == kMillisPerSecond ? counter
                                                  : counter * kMillisPerSecond / gTimerFrequency;
        gTimerLastMs = millis;
        gTimerLastTick = tick;
        gTimerCorrection = 0;
    }

    if (gTimerFrequency) {
        tick = GetTickCount();
        QueryPerformanceCounter(&counter);
        micros = gTimerFrequency == kMicrosPerSecond ? counter
                                                  : counter * kMicrosPerSecond / gTimerFrequency;
        millis = gTimerFrequency == kMillisPerSecond ? counter
                                                  : counter * kMillisPerSecond / gTimerFrequency;
        /* BUG: the products are 32-bit, so a correction past about 35 minutes overflows (exe
           0x004d3cee, 0x004d3d54) */
        if (gTimerCorrection != 0) {
            micros += gTimerCorrection * kMillisPerSecond;
            millis += gTimerCorrection;
        }

        elapsed = tick - gTimerLastTick;
        drift = elapsed - MillisSince(millis);
        if (drift < -kDriftWindowMs || drift > kDriftWindowMs) {
            gTimerCorrection += drift;
            micros += drift * kMillisPerSecond;
            millis += drift;
        }

        gTimerLastTick = tick;
        gTimerLastMs = millis;

        if (gClockReadyFlag != 0)
            TimerPrime(&gClockTimer, kClockPeriodMs);
    } else {
        micros = (ULongLong)GetTickCount() * kMillisPerSecond;
    }
    return micros;
}

/* The millisecond twin: same init, same drift correction, no microsecond
   scaling, and the freq == 0 fallback answers GetTickCount unscaled. */
/* TimerGetMilliseconds. STATE: MATCH 100%, exe 140 / obj 140, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3dd2
ULongLong TimerGetMilliseconds(void)
{
    long elapsed;
    ULongLong millis;
    long long counter;
    long long freq;
    ULong tick;
    long drift;

    elapsed = 0;
    millis = 0;

    if (gTimerReady == 0) {
        QueryPerformanceFrequency(&freq);
        gTimerFrequency = freq;
        gTimerReady = 1;
        tick = GetTickCount();
        QueryPerformanceCounter(&counter);
        millis = gTimerFrequency == kMillisPerSecond ? counter
                                                  : counter * kMillisPerSecond / gTimerFrequency;
        gTimerLastMs = millis;
        gTimerLastTick = tick;
        gTimerCorrection = 0;
    }

    if (gTimerFrequency) {
        tick = GetTickCount();
        QueryPerformanceCounter(&counter);
        millis = gTimerFrequency == kMillisPerSecond ? counter
                                                  : counter * kMillisPerSecond / gTimerFrequency;
        if (gTimerCorrection != 0)
            millis += gTimerCorrection;

        elapsed = tick - gTimerLastTick;
        drift = elapsed - MillisSince(millis);
        if (drift < -kDriftWindowMs || drift > kDriftWindowMs) {
            gTimerCorrection += drift;
            millis += drift;
        }

        gTimerLastTick = tick;
        gTimerLastMs = millis;

        if (gClockReadyFlag != 0)
            TimerPrime(&gClockTimer, kClockPeriodMs);
    } else {
        millis = GetTickCount();
    }
    return millis;
}

/* TimerCreate. STATE: MATCH 100%, exe 125 / obj 125, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d3ff5
long TimerCreate(void *ref, void *arg, TimerProc proc)
{
    char token[16];
    PlatformTimer *timer;
    TimerThread *thread;
    long err;
    PlatformTimer **list;
    PlatformTimer **old;
    ULong i;

    timer = (PlatformTimer *)ref;
    err = 0;
    thread = 0;
    old = 0;
    SystemCheckCallback();
    for (i = 0; i < gTimerCount; i = i + 1) {
    }
    MemoryClear(timer, sizeof(PlatformTimer));
    timer->arg = arg;
    timer->proc = proc;
    list = (PlatformTimer **)MemoryAllocate((gTimerCount + 1) * sizeof(PlatformTimer *));
    if (list == 0)
        goto nomem;
    AtomicEnterCritical(token);
    MemoryCopy(list, gTimerList, gTimerCount * sizeof(PlatformTimer *));
    list[gTimerCount] = timer;
    gTimerCount = gTimerCount + 1;
    old = gTimerList;
    gTimerList = list;
    AtomicLeaveCritical(token);
    if (old != 0)
        MemoryDeallocate(old);
    thread = 0;

    thread = (TimerThread *)MemoryAllocateClear(sizeof(TimerThread));
    timer->thread = thread;
    if (timer->thread == 0)
        goto nomem;
    if ((thread->mutex = CreateMutexA(0, 1, 0)) == 0
        || (thread->event = CreateEventA(0, 0, 0, 0)) == 0) {
        err = GetLastError();
        goto failed;
    }
    if ((thread->thread = CreateThread(0, kThreadStack,
                                       (LPTHREAD_START_ROUTINE)_TimerThread, ref, 0,
                                       &thread->threadId)) == 0) {
        err = GetLastError();
failed:
nomem:
        err = kOutOfMemory;
        goto done;
    }
    if (SetThreadPriority(thread->thread, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
        err = GetLastError();
        err = kOSFailure;
    } else {
        ReleaseMutex(thread->mutex);
    }
done:
    if (err != 0)
        TimerDispose(ref);
    return err;
}

/* TimerPrime. STATE: MATCH 100%, exe 58 / obj 58, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d415a
long TimerPrime(void *ref, long period)
{
    long err;
    PlatformTimer *timer;
    TimerThread *thread;
    long result;
    long request;
    long elapsed;

    timer = (PlatformTimer *)ref;
    err = 0;
    result = 0;
    thread = timer->thread;
    WaitForSingleObject(thread->mutex, INFINITE);
    if (thread->threadId == GetCurrentThreadId()) {
        result = thread->request;
        if (period != kNoRequest)
            thread->request = period;
    } else if ((request = thread->request) == 0) {
        if (thread->armed != 0) {
            elapsed = timeGetTime() - thread->armedAt;
            if (elapsed <= 0)
                elapsed = 0;
            elapsed = thread->period - elapsed;
            if (elapsed <= 0)
                elapsed = 1;
            result = elapsed;
        }
        if (period != kNoRequest) {
            thread->request = period;
            SetEvent(thread->event);
        }
    } else {
        result = request;
        if (period != kNoRequest) {
            thread->request = period;
            SetEvent(thread->event);
        }
    }
    ReleaseMutex(thread->mutex);
    return result;
}

/* TimerDispose. STATE: MATCH 100%, exe 124 / obj 124, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d41f1
long TimerDispose(void *ref)
{
    char token[16];
    PlatformTimer *timer;
    long err;
    ULong i;
    PlatformTimer **old;
    long result;
    TimerThread *dead;
    TimerThread *thread;

    timer = (PlatformTimer *)ref;
    err = 0;
    result = 0;
    old = 0;
    dead = 0;
    SystemCheckCallback();
    AtomicEnterCritical(token);
    for (i = 0; i < gTimerCount; i = i + 1) {
        if (gTimerList[i] != timer)
            continue;
        thread = timer->thread;
        if (thread != 0) {
            GetCurrentThreadId();
            thread->quit = 1;
            result = TimerPrime(ref, 0);
            if (thread->mutex != 0)
                WaitForSingleObject(thread->mutex, INFINITE);
            if (thread->thread != 0 && CloseHandle(thread->thread) == 0)
                err = GetLastError();
            if (thread->event != 0 && CloseHandle(thread->event) == 0)
                err = GetLastError();
            if (thread->mutex != 0 && CloseHandle(thread->mutex) == 0)
                err = GetLastError();
            dead = thread;
            thread = 0;
        }
        if (gTimerCount == 1) {
            old = gTimerList;
            gTimerList = 0;
            gTimerCount = 0;
        } else {
            MemoryMove(gTimerList + i, gTimerList + i + 1,
                         (gTimerCount - i - 1) * sizeof(PlatformTimer *));
            gTimerCount = gTimerCount - 1;
            gTimerList[gTimerCount] = 0;
        }
        break;
    }
    AtomicLeaveCritical(token);
    MemoryClear(ref, sizeof(PlatformTimer));
    if (old != 0)
        MemoryDeallocate(old);
    if (dead != 0)
        MemoryDeallocate(dead);
    return result;
}

/* _TimerThread. STATE: MATCH 100%, exe 70 / obj 70, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d435b
ULong __stdcall _TimerThread(PlatformTimer *timer)
{
    TimerThread *thread;
    long err;
    unsigned char armed;
    long wait;

    thread = timer->thread;
    err = 0;
    armed = false;
    wait = 0;
    while (thread->quit == 0) {
        if (armed == 0)
            wait = INFINITE;
        if (WaitForSingleObject(thread->event, wait) == WAIT_TIMEOUT) {
            WaitForSingleObject(thread->mutex, INFINITE);
            thread->period = 0;
            thread->armedAt = 0;
            thread->request = 0;
            thread->armed = 0;
            gTimerInProc = 1;
            if (timer->proc != 0)
                timer->proc(timer, timer->arg);
            gTimerInProc = 0;
        } else {
            WaitForSingleObject(thread->mutex, INFINITE);
        }
        wait = thread->request;
        if (wait != 0) {
            thread->period = wait;
            thread->armed = armed = true;
            thread->armedAt = timeGetTime();
            thread->request = 0;
        } else {
            thread->period = wait = 0;
            thread->armed = armed = false;
        }
        ReleaseMutex(thread->mutex);
    }
    return 0;
}
