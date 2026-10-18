// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgertick.c -- BurgerLib TkWin95Code: the 60 Hz tick counter kept by a timer thread. Notes: Research/notes/burgertick.md */

#include <stdlib.h>

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

static volatile Word32 TimeCount;                /* 0x00865858 */
static Word32 TimerID;                           /* 0x0086585c */
static void *ThreadHandle;                       /* 0x00865860 */
static Word8 Started;                            /* 0x00865864 */
static Word GotMilk;                             /* 0x00865868 */
Word TickWakeUpFlag;                             /* 0x00870d84 */

#define kTickFraction   8                        /* milliseconds kept in 24.8 fixed point */
#define kTickPeriod     0x10aa                   /* 1000/60 ms in 24.8: 16.664 */
#define kTickSleepMs    16
#define kWakeUpMessage  0x400                    /* WM_USER */
#define kTickStackSize  0x400
#define kTimerPriority  2                        /* THREAD_PRIORITY_HIGHEST */

/* MyIrq (BurgerLib TkWin95Code, the timer thread). STATE: MATCH 100.0%, exe 34 / obj 34,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d5d40
static Word32 __stdcall MyIrq(void *data)
{
    Word32 mark;
    Word32 elapsed;

    mark = GetTickCount() << kTickFraction;
    for (;;) {
        do {
            SleepEx(kTickSleepMs, 0);
            elapsed = (GetTickCount() << kTickFraction) - mark;
        } while (elapsed < kTickPeriod);
        do {
            ++TimeCount;
            mark += kTickPeriod;
            elapsed -= kTickPeriod;
        } while (elapsed >= kTickPeriod);
        if (TickWakeUpFlag != 0 && Win95MainWindow != 0 &&
            GetForegroundWindow() == Win95MainWindow)
            PostMessageA(Win95MainWindow, kWakeUpMessage, 0, TimeCount);
    }
}

/* MyShutdown (BurgerLib TkWin95Code, the atexit hook). STATE: MATCH 100.0%, exe 6 / obj 6,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d5df0
static void MyShutdown(void)
{
    CloseHandle(ThreadHandle);
    ThreadHandle = 0;
}

/* ReadTick (BurgerLib TkWin95Code). STATE: MATCH 100.0%, exe 24 / obj 24, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d5e10
Word32 ReadTick(void)
{
    if (Started != 0)
        return TimeCount;
    atexit(MyShutdown);
    ThreadHandle = CreateThread(0, kTickStackSize, MyIrq, 0, 0, &TimerID);
    SetThreadPriority(ThreadHandle, kTimerPriority);
    Started = 1;
    return TimeCount;
}

/* WaitTicks (BurgerLib TkWin95Code). STATE: MATCH 100.0%, exe 69 / obj 69, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d5e90
void WaitTicks(Word tickCount)
{
    Word32 now;
    Word wakeUp;

    KeyboardKbhit();
    now = ReadTick();
    while (now - LastTick < tickCount) {
        wakeUp = TickWakeUpFlag;
        TickWakeUpFlag = 1;
        WaitMessage();
        TickWakeUpFlag = wakeUp;
        KeyboardKbhit();
        now = ReadTick();
    }
    LastTick = now;
}

#define kMicrosPerSecond 1000000.0

static double WinTicks = 1.0;                    /* microseconds per counter tick */

/* ReadTickMicroseconds (BurgerLib TkWin95Code): not linked, dead-stripped in the exe; it stays
   for its 1000000.0 at 0x005766e8, the TU's one kept .data (notes) */
Word32 ReadTickMicroseconds(void)
{
    long long count;

    if (!GotMilk) {
        GotMilk = 1;
        if (QueryPerformanceFrequency(&count))
            WinTicks = kMicrosPerSecond / count;
    }
    if (QueryPerformanceCounter(&count))
        return count * WinTicks;
    return 0;
}

/* ReadTickMilliseconds. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d6000
ULong ReadTickMilliseconds(void)
{
    return timeGetTime();
}

#ifdef __cplusplus
}
#endif
