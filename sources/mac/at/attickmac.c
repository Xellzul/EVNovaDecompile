/* attickmac.c -- 1.0.10: the clocks on Mac: the date and the zone, the tick, Microseconds or UpTime for the fine clock. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#if TARGET_API_MAC_CARBON
#include <DriverServices.h>
#endif

#define kSecondsTo1970  2082844800
#define kTicksPerSecond 60

extern ULongLong gPlatformTime;

extern long _TimerGetZoneOffset(unsigned char *daylight);

long TimerGetSeconds(void)
{
    return TimerGetLocalSeconds();
}

#if TARGET_API_MAC_CARBON

ULong TimerGetUTCSeconds(void)
{
    UTCDateTime now;

    GetUTCDateTime(&now, kUTCDefaultOptions);
    return now.lowSeconds - kSecondsTo1970;
}

ULong TimerGetLocalSeconds(void)
{
    LocalDateTime now;

    GetLocalDateTime(&now, kUTCDefaultOptions);
    return now.lowSeconds - kSecondsTo1970;
}

#else

ULong TimerGetUTCSeconds(void)
{
    ULong seconds;

    seconds = 0;
    ReadDateTime(&seconds);
    seconds -= kSecondsTo1970;
    seconds -= _TimerGetZoneOffset(0);
    return seconds;
}

ULong TimerGetLocalSeconds(void)
{
    ULong seconds;

    seconds = 0;
    ReadDateTime(&seconds);
    return seconds - kSecondsTo1970;
}

#endif

unsigned long TimerGetElapsedSeconds(void)
{
    double seconds;

    seconds = TimerGetMilliseconds() - gPlatformTime;
    seconds = seconds / kMillisPerSecond;
    return seconds;
}

#if !TARGET_API_MAC_CARBON

long _TimerGetZoneOffset(unsigned char *daylight)
{
    MachineLocation where;
    long delta;

    if (daylight != 0)
        *daylight = 0;
    ReadLocation(&where);
    delta = where.u.gmtDelta & 0xffffff;
    if ((where.u.gmtDelta & 0x800000) != 0)
        delta |= 0xff000000;
    return -delta;
}

#endif

ULong TimerGetTicks(void)
{
    return TickCount();
}

ULong TimerGetTickRate(void)
{
    return kTicksPerSecond;
}

typedef UnsignedWide AbsTime;

extern AbsTime (*gUpTimeProc)(void);
extern AbsTime (*gAbsoluteToNanosecondsProc)(AbsTime t);
extern unsigned char gHasTimeBase;
extern unsigned char gHasRTC;
extern double gMicrosPerNano;
extern double gMillisPerNano;

#if TARGET_API_MAC_CARBON
#define kTickCount      TickCount()
#else
#define kTickCount      *(long *)0x16a
#endif

static ULongLong _TimerReadMicros(void);
static ULongLong _TimerReadMillis(void);
extern UnsignedWide ReadRTC(void);
extern UnsignedWide ReadTB(void);

ULongLong TimerGetMicroseconds(void)
{
    return _TimerReadMicros();
}

ULongLong TimerGetMilliseconds(void)
{
    return _TimerReadMillis();
}

static ULongLong _TimerReadMicros(void)
{
    static ULong TICKS = 0;
    static ULongLong USEC;
    ULongLong now;
    AbsTime t;
    long tick;
    long delta;

#if TARGET_API_MAC_CARBON
    if (UpTime != 0) {
        AbsTime up;

        up = UpTime();
        t = AbsoluteToNanoseconds(up);
        now = *(ULongLong *)&t * gMicrosPerNano + 0.5;
    } else
#endif
    if (gUpTimeProc != 0) {
        AbsTime a;

        a = gUpTimeProc();
        t = gAbsoluteToNanosecondsProc(a);
        now = *(ULongLong *)&t * gMicrosPerNano + 0.5;
    } else if (gHasTimeBase) {
        t = ReadTB();
        now = *(ULongLong *)&t * gMicrosPerNano + 0.5;
    } else if (gHasRTC) {
        t = ReadRTC();
        now = (t.hi * 1000000000.0 + t.lo) * gMicrosPerNano + 0.5;
    } else {
        Microseconds(&t);
        now = *(ULongLong *)&t;
    }
    tick = kTickCount;
    delta = tick - TICKS;
    if (TICKS != 0 && delta >= -2 && delta <= 2 && now < USEC)
        now = USEC;
    TICKS = tick;
    USEC = now;
    return now;
}

static ULongLong _TimerReadMillis(void)
{
    static ULong TICKS = 0;
    static ULongLong MSEC;
    ULongLong now;
    AbsTime t;
    long tick;
    long delta;

#if TARGET_API_MAC_CARBON
    if (UpTime != 0) {
        AbsTime up;

        up = UpTime();
        t = AbsoluteToNanoseconds(up);
        now = *(ULongLong *)&t * gMillisPerNano + 0.5;
    } else
#endif
    if (gUpTimeProc != 0) {
        AbsTime a;

        a = gUpTimeProc();
        t = gAbsoluteToNanosecondsProc(a);
        now = *(ULongLong *)&t * gMillisPerNano + 0.5;
    } else if (gHasTimeBase) {
        t = ReadTB();
        now = *(ULongLong *)&t * gMillisPerNano + 0.5;
    } else if (gHasRTC) {
        t = ReadRTC();
        now = (t.hi * 1000000000.0 + t.lo) * gMillisPerNano + 0.5;
    } else {
        Microseconds(&t);
        now = (500.0 + *(ULongLong *)&t) / 1000.0;
    }
    tick = kTickCount;
    delta = tick - TICKS;
    if (TICKS != 0 && delta >= -2 && delta <= 2 && now < MSEC)
        now = MSEC;
    TICKS = tick;
    MSEC = now;
    return now;
}

asm UnsignedWide ReadRTC(void)
{
    nofralloc
loop:
    mfspr r4, 4
    mfspr r5, 5
    mfspr r6, 4
    cmpw r4, r6
    bne loop
    stw r4, 0(r3)
    stw r5, 4(r3)
    blr
}

asm UnsignedWide ReadTB(void)
{
    nofralloc
loop:
    mftb r4, 269
    mftb r5, 268
    mftb r6, 269
    cmpw r4, r6
    bne loop
    stw r4, 0(r3)
    stw r5, 4(r3)
    blr
}

