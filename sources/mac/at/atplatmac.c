/* atplatmac.c -- 1.0.10: the platform layer on the Toolbox: open and close, the idle and quit tables, the callback depth. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include <LowMem.h>
#if TARGET_API_MAC_CARBON
#include <Multiprocessing.h>
#include <CFBundle.h>
#include <CFURL.h>
#include <Folders.h>
#include <CFString.h>
#include <CodeFragments.h>
#include <Threads.h>
#include <DriverServices.h>

/* Carbon Events, which the Interfaces this project builds against do not have */
typedef struct OpaqueEventLoopRef *EventLoopRef;
typedef struct OpaqueEventLoopTimerRef *EventLoopTimerRef;
typedef struct OpaqueEventLoopTimerUPP *EventLoopTimerUPP;
extern pascal EventLoopRef GetMainEventLoop(void);
extern pascal OSStatus InstallEventLoopTimer(EventLoopRef loop, double delay, double interval, EventLoopTimerUPP proc, void *ref, EventLoopTimerRef *timer);
extern pascal OSStatus RemoveEventLoopTimer(EventLoopTimerRef timer);
extern pascal EventLoopTimerUPP NewEventLoopTimerUPP(pascal void (*proc)(EventLoopTimerRef timer, void *ref));
extern pascal void DisposeEventLoopTimerUPP(EventLoopTimerUPP proc);
#endif

#define kCallbackSlots  50

typedef struct AbsTime {
    ULong hi;
    ULong lo;
} AbsTime;

typedef void (*PlatformCallback)(void);

extern void _TimerRunDeferred(void);
extern long _SystemPatchTraps(void);
extern ULong ThreadsCount(void);
extern void _AtomicResetCritical(void);
extern void _AtomicEnter2(void);
extern void _AtomicLeave2(void);
extern void ThreadYield(void);
void SystemIdle(void);
extern void *_PlatformFindSymbol(const char *library, const char *name);
extern long ThreadsPreflight(long count, ULong stackSize);
#if !TARGET_API_MAC_CARBON
void SystemEnterCallback(int lock);
void SystemLeaveCallback(int lock);
#endif
unsigned char SystemCheckCallback(void);
unsigned char _PlatformIsThreaded(void);
static void _SystemQuitCallback(void);
void _SystemUnpatchTraps(void);
void _SystemIdleTick(void);

extern ULong gThreadFlags;
extern struct ThreadRec *gThreadCurrent;
extern struct ThreadRec gMainThread;

static PlatformCallback gSystemIdleProcs[kCallbackSlots];
static unsigned char gSystemIdleRecurse[kCallbackSlots];
static PlatformCallback gSystemQuitProcs[kCallbackSlots];
#if TARGET_API_MAC_CARBON
static FSRef sFrameworks;
static unsigned char sFound;
#endif
static unsigned char sInQuit;
static void *sPatchB;
static void *sPatchA;
static ULong gSystemQuitCount;
static ULong gSystemIdleCount;
#if TARGET_API_MAC_CARBON
long *gErrno;
long (*gExit)(long code);
long (*gSignal)(long sig, void *handler);
long (*gAtExit)(void *proc);
#else
void (*gLeaveNotifier)(void *provider);
unsigned char (*gEnterNotifier)(void *provider);
#endif
UnsignedWide (*gUpTimeProc)(void);
UnsignedWide (*gAbsoluteToNanosecondsProc)(UnsignedWide t);
#if TARGET_API_MAC_CARBON
void *gUnholdMemory;
void *gHoldMemory;
void (*gNSetTrapAddress)(void *address, ULong trap, long kind);
ULong (*gNGetTrapAddress)(ULong trap, long kind);
long (*gCallUniversalProc)(UniversalProcPtr proc, ULong info, ...);
void *(*gNewRoutineDescriptor)(void *proc, ULong procInfo, long isa);
unsigned char gSignalsInstalled;
unsigned char gAtExitInstalled;
unsigned char gOTContextOpen;
EventLoopTimerRef gIdleTimer;
EventLoopTimerUPP gIdleTimerUPP;
void *gPlatformMutex;
#else
PlatformCallback gIdleHook;
void (*gMemoryClearProc)(void *p, ULong n);
#endif
unsigned char gHasTimeBase;
unsigned char gHasRTC;
unsigned char gHasUpTimeOnly;
ULong gLastIdleTick;
unsigned char gInIdleTick;
unsigned char gIdleHookEnabled;
unsigned char gPlatformThreaded;
unsigned char (*gIndirectBusy)(void *p);
ULong gRandomSeed;
ULong gCallbackDepth;
ULongLong gPlatformTime;
unsigned char gPlatformOpen;
#if TARGET_API_MAC_CARBON

#define kNoTaskIndex        0xffffffff
#define kFrameworksFolderType 'fram'
#define kDurationForever    0x7fffffff
#define kSystemTen          0x1000
#define kIdleTimerSeconds   0.1

extern long _MutexDispose(struct TaskMutex *mutex);
extern long _MutexEnter(struct TaskMutex *mutex, long timeout);
extern long _MutexLeave(struct TaskMutex *mutex);
static long _SystemInstallSignals(void);
void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName);
void *_MachOForCFM(void *function);
pascal void _SystemIdleTimer(EventLoopTimerRef timer, void *ref);
void SystemEnterCallback(unsigned char lock);
void SystemLeaveCallback(unsigned char lock);

TaskStorageIndex gCallbackIndex = kNoTaskIndex;

#endif
const void *gRT3_IsRegisteredPtr1 = "";
double gMicrosPerNano = 0.001;
double gMillisPerNano = 0.000001;
static unsigned short sTrapPatch[12] = { 0x203a, 0x000e, 0x6704, 0x2040, 0x4e90, 0x207a, 0x0008, 0x4ed0 };

#define kNoMemory       -7004
#define kPatchBytes     0x18
#define kPatchRoutine   4
#define kPatchOldTrap   5
#define kTrapExitToShell 0xa9f4
#define kTrapSystemTask 0xa9b2

#define kNoGestalt          -7000
#define kOldSystem          -7003
#define kMinSystem          0x700
#define kMinQuickDraw       0x100
#define kPpc601             0x101
#define kCalibrationTicks   30.0
#define kTicksPerSecond     60.15
#define kPlatformBackoff    1000
#define kSeedBits           32

extern long _EncodingPreflight(void);
extern UnsignedWide ReadRTC(void);
extern UnsignedWide ReadTB(void);

#if !TARGET_API_MAC_CARBON

static pascal void _PlatformBeep(short count)
{
    while (count--)
        SysBeep(3);
}

static RoutineDescriptor sBeepDescriptor = BUILD_ROUTINE_DESCRIPTOR(uppSoundProcInfo, _PlatformBeep);
static SoundUPP sBeepUPP = (SoundUPP)&sBeepDescriptor;

static void _PlatformInitDialogs(UniversalProcPtr resume)
{
    LMSetResumeProc(resume);
    LMSetACount(0);
    LMSetANumber(-1);
    ParamText(0, 0, 0, 0);
    ErrorSound(sBeepUPP);
    LMSetDlgFont(0);
}

#endif

#define CLOCK_READ(t) ((t) = gUpTimeProc != 0 ? gAbsoluteToNanosecondsProc(gUpTimeProc()) : (gHasRTC ? ReadRTC() : ReadTB()), \
    (ULongLong)(gHasRTC ? (double)(t).hi * 1000000000.0 + (double)(t).lo : (double)*(ULongLong *)&(t)))

#if TARGET_API_MAC_CARBON

extern long _MutexCreate(struct TaskMutex **out);
extern void _PlatformOpenLibraries(void);

#define BUNDLE_SYMBOL(name) _PlatformFindBundleSymbol(CFSTR("System.framework"), CFSTR(name))

long PlatformOpen(void)
{
    long err;
    long response;

    err = 0;
    SystemCheckCallback();
    if (gPlatformOpen)
        goto done;
    if (Gestalt(gestaltSystemVersion, &response) != 0) {
        err = kNoGestalt;
        goto done;
    }
    if (response < kMinSystem) {
        err = kOldSystem;
        goto done;
    }
    if (response >= kSystemTen)
        gPlatformThreaded = 1;
    InitCursor();
    if (gPlatformThreaded) {
        if (_MutexCreate(&gPlatformMutex) != 0) {
            err = kNoMemory;
            goto done;
        }
        if (MPAllocateTaskStorageIndex(&gCallbackIndex) != 0) {
            err = kNoMemory;
            goto done;
        }
        if ((gErrno = BUNDLE_SYMBOL("errno")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gExit = BUNDLE_SYMBOL("exit")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gSignal = BUNDLE_SYMBOL("signal")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gAtExit = BUNDLE_SYMBOL("atexit")) == 0) {
            err = kOldSystem;
            goto done;
        }
    } else {
        if ((gNewRoutineDescriptor = _PlatformFindSymbol("InterfaceLib", "NewRoutineDescriptor")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gCallUniversalProc = _PlatformFindSymbol("InterfaceLib", "CallUniversalProc")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gNGetTrapAddress = _PlatformFindSymbol("InterfaceLib", "NGetTrapAddress")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gNSetTrapAddress = _PlatformFindSymbol("InterfaceLib", "NSetTrapAddress")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gHoldMemory = _PlatformFindSymbol("InterfaceLib", "HoldMemory")) == 0) {
            err = kOldSystem;
            goto done;
        }
        if ((gUnholdMemory = _PlatformFindSymbol("InterfaceLib", "UnholdMemory")) == 0) {
            err = kOldSystem;
            goto done;
        }
    }
    if ((void *)UpTime == 0 && (gUpTimeProc = _PlatformFindSymbol("DriverServicesLib", "UpTime")) != 0)
        gAbsoluteToNanosecondsProc = _PlatformFindSymbol("DriverServicesLib", "AbsoluteToNanoseconds");
    gHasUpTimeOnly = gHasRTC = gHasTimeBase = 0;
    if ((void *)UpTime != 0 || gUpTimeProc != 0) {
        if (Gestalt(gestaltTimeMgrVersion, &response) == 0 && (ULong)response > 3)
            gHasUpTimeOnly = 1;
    } else if (Gestalt(gestaltNativeCPUtype, &response) == 0) {
        if ((ULong)response == kPpc601)
            gHasRTC = 1;
        else if ((ULong)response > kPpc601)
            gHasTimeBase = 1;
    }
    if ((((void *)UpTime != 0 || gUpTimeProc != 0) && gHasUpTimeOnly == 0) || gHasRTC || gHasTimeBase) {
        ULongLong start;
        ULongLong end;
        UnsignedWide t;

        if ((void *)UpTime != 0) {
            ULongLong tick;
            double until;

            tick = TickCount() + 1;
            while (TickCount() < tick)
                ;
            t = AbsoluteToNanoseconds(UpTime());
            start = *(ULongLong *)&t;
            until = kCalibrationTicks + (double)(ULongLong)tick;
            while (until > (double)TickCount())
                ;
            t = AbsoluteToNanoseconds(UpTime());
            end = *(ULongLong *)&t;
        } else {
            ULong tick;
            double until;

            tick = TickCount() + 1;
            while ((ULongLong)TickCount() < (ULongLong)tick)
                ;
            start = CLOCK_READ(t);
            until = kCalibrationTicks + (double)(ULongLong)tick;
            while (until > (double)TickCount())
                ;
            end = CLOCK_READ(t);
        }
        gMicrosPerNano = kCalibrationTicks * 1000000.0 / ((double)(end - start) * kTicksPerSecond);
        gMillisPerNano = kCalibrationTicks * 1000.0 / ((double)(end - start) * kTicksPerSecond);
    }
    if (Gestalt(gestaltThreadMgrAttr, &response) == 0 && (response & 1) != 0)
        gIdleHookEnabled = 1;
    gPlatformOpen = 1;
    gPlatformTime = TimerGetMilliseconds() - kPlatformBackoff;
    {
        ULong shift;

        shift = TickCount();
        shift %= kSeedBits;
        gRandomSeed = (ULong)TimerGetMicroseconds();
        gRandomSeed = (gRandomSeed << shift) | (gRandomSeed >> (kSeedBits - shift));
    }
    {
        long e;

        e = _EncodingPreflight();
        if (e != 0)
            err = e;
    }
done:
    if (err != 0 && gPlatformOpen)
        PlatformClose();
    return err;
}

#else

long PlatformOpen(void)
{
    long err;
    long e;
    long response;

    err = 0;
    SystemCheckCallback();
    if (gPlatformOpen)
        goto done;
    e = Gestalt(gestaltSystemVersion, &response);
    if (e != 0) {
        err = kNoGestalt;
        goto done;
    }
    if (response < kMinSystem) {
        err = kOldSystem;
        goto done;
    }
    e = Gestalt(gestaltQuickdrawVersion, &response);
    if (e != 0) {
        err = kNoGestalt;
        goto done;
    }
    if ((ULong)response < kMinQuickDraw) {
        err = kOldSystem;
        goto done;
    }
    if (LMGetMenuList() == 0) {
        MaxApplZone();
        InitGraf(&qd.thePort);
        InitFonts();
        InitWindows();
        InitMenus();
        TEInit();
        _PlatformInitDialogs(0);
    }
    InitCursor();
    gUpTimeProc = _PlatformFindSymbol("DriverServicesLib", "UpTime");
    if (gUpTimeProc != 0)
        gAbsoluteToNanosecondsProc = _PlatformFindSymbol("DriverServicesLib", "AbsoluteToNanoseconds");
    if (gUpTimeProc != 0)
        gMemoryClearProc = _PlatformFindSymbol("DriverServicesLib", "BlockZero");
    gHasUpTimeOnly = gHasRTC = gHasTimeBase = 0;
    if (gUpTimeProc != 0) {
        if (Gestalt(gestaltTimeMgrVersion, &response) == 0 && (ULong)response > 3)
            gHasUpTimeOnly = 1;
    } else if (Gestalt(gestaltNativeCPUtype, &response) == 0) {
        if ((ULong)response == kPpc601)
            gHasRTC = 1;
        else if ((ULong)response > kPpc601)
            gHasTimeBase = 1;
    }
    if ((gUpTimeProc != 0 && gHasUpTimeOnly == 0) || gHasRTC || gHasTimeBase) {
        ULongLong tick;
        ULongLong start;
        ULongLong end;
        double until;
        UnsignedWide t;

        tick = LMGetTicks() + 1;
        while (LMGetTicks() < tick)
            ;
        start = CLOCK_READ(t);
        until = kCalibrationTicks + (double)(ULongLong)tick;
        while (until > (double)LMGetTicks())
            ;
        end = CLOCK_READ(t);
        gMicrosPerNano = kCalibrationTicks * 1000000.0 / ((double)(end - start) * kTicksPerSecond);
        gMillisPerNano = kCalibrationTicks * 1000.0 / ((double)(end - start) * kTicksPerSecond);
    }
    if (Gestalt(gestaltThreadMgrAttr, &response) == 0 && (response & 1) != 0) {
        gIdleHook = _PlatformFindSymbol("ThreadsLib", "YieldToAnyThread");
        gIdleHookEnabled = 1;
    }
    gPlatformOpen = 1;
    gPlatformTime = TimerGetMilliseconds() - kPlatformBackoff;
    {
        ULong shift;

        shift = TickCount();
        shift %= kSeedBits;
        gRandomSeed = (ULong)TimerGetMicroseconds();
        gRandomSeed = (gRandomSeed << shift) | (gRandomSeed >> (kSeedBits - shift));
    }
    e = ThreadsPreflight(0, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _EncodingPreflight();
    if (e != 0)
        err = e;
done:
    if (err != 0 && gPlatformOpen)
        PlatformClose();
    return err;
}

#endif

void PlatformClose(void)
{
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
    if (gPlatformOpen) {
        _SystemQuitCallback();
        gPlatformOpen = 0;
    }
}

unsigned char IsPlatformOpen(void)
{
    return gPlatformOpen;
}

void SystemIdleEvents(void)
{
    EventRecord event;

    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
#if TARGET_API_MAC_CARBON
    if (_PlatformIsThreaded()) {
        AbsoluteTime until = AddDurationToAbsolute(durationMillisecond, UpTime());

        MPDelayUntil(&until);
    } else
#endif
    EventAvail(0, &event);
    SystemIdle();
}

void SystemIdle(void)
{
    unsigned char *busy;
    PlatformCallback *proc;
    ULong j;
    ULong i;

    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
    busy = gSystemIdleRecurse;
    proc = gSystemIdleProcs;
    for (i = 0; i < gSystemIdleCount; i++) {
        if (*busy == 0) {
            *busy = 1;
            (*proc)();
            *busy = 0;
        }
        busy++;
        proc++;
    }
    if (AtomicMaskTest(&gThreadFlags, 1)) {
        ULong count;

        if (gThreadCurrent == &gMainThread)
            count = ThreadsCount();
        else
            count = 1;
        for (j = 0; j < 3 && j < count; j++)
            ThreadYield();
    }
    if (gInIdleTick) {
#if TARGET_API_MAC_CARBON
        if (gIdleHookEnabled)
            YieldToAnyThread();
        MPYield();
#else
        if (gIdleHookEnabled && gIdleHook != 0)
            gIdleHook();
        SystemTask();
#endif
    }
    gLastIdleTick = TickCount();
}

#define kWaitSliceMs    50

void SystemWait(ULong ms)
{
#if !TARGET_API_MAC_CARBON
    ULong deadline;

#endif
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    SystemCheckCallback();
#if TARGET_API_MAC_CARBON
    if (ms != 0) {
        if (_PlatformIsThreaded()) {
            while (ms != 0) {
                ULong step = ms >= kWaitSliceMs ? kWaitSliceMs : ms;
                AbsoluteTime until = AddDurationToAbsolute(step, UpTime());

                SystemIdle();
                MPDelayUntil(&until);
                ms -= step;
            }
        } else {
            ms += (ULong)TimerGetMilliseconds();
            while (ms >= (ULong)TimerGetMilliseconds())
                SystemIdle();
        }
    }
#else
    if (ms != 0) {
        deadline = ms + (ULong)TimerGetMilliseconds();
        while (deadline >= (ULong)TimerGetMilliseconds())
            SystemIdle();
    }
#endif
}

void SystemSetIdleProc(PlatformCallback fn)
{
    ULong i;

    if (gSystemIdleCount != kCallbackSlots) {
        for (i = 0; i < gSystemIdleCount; i = i + 1) {
            if (gSystemIdleProcs[i] == fn)
                break;
        }
        if (i == gSystemIdleCount)
            gSystemIdleProcs[gSystemIdleCount++] = fn;
    }
}

void SystemSetQuitProc(PlatformCallback fn)
{
    ULong i;

    if (gSystemQuitCount != kCallbackSlots) {
        for (i = 0; i < gSystemQuitCount; i = i + 1) {
            if (gSystemQuitProcs[i] == fn)
                break;
        }
        if (i == gSystemQuitCount && _SystemPatchTraps() == 0)
            gSystemQuitProcs[gSystemQuitCount++] = fn;
    }
}

#if TARGET_API_MAC_CARBON

extern void _CriticalDisposeMP(void *mutex);

static void _SystemQuitCallback(void)
{
    PlatformCallback fn;

    if (!sInQuit && (gAtExitInstalled || !_PlatformIsThreaded())) {
        sInQuit = 1;
        gCallbackDepth = 0;
        if (SystemCheckCallback()) {
            while (SystemCheckCallback())
                SystemLeaveCallback(1);
        }
        if (_AtomicCheckCritical())
            _AtomicResetCritical();
        _ThreadCheckCritical();
        while (gSystemQuitCount != 0) {
            fn = gSystemQuitProcs[--gSystemQuitCount];
            if (fn != 0)
                fn();
        }
        if (gPlatformThreaded && gCallbackIndex != kNoTaskIndex) {
            MPDeallocateTaskStorageIndex(gCallbackIndex);
            gCallbackIndex = kNoTaskIndex;
        }
        if (gPlatformThreaded && gPlatformMutex != 0) {
            _MutexDispose(gPlatformMutex);
            gPlatformMutex = 0;
        }
        _SystemUnpatchTraps();
        sInQuit = 0;
    }
}

void SystemEnterCallback(unsigned char lock)
{
    if (gPlatformThreaded) {
        if (gCallbackIndex != kNoTaskIndex)
            MPSetTaskStorageValue(gCallbackIndex, MPGetTaskStorageValue(gCallbackIndex) + 1);
        if (lock && gPlatformMutex != 0)
            _MutexEnter(gPlatformMutex, kDurationForever);
    } else {
        if (gCallbackDepth == 0)
            _AtomicEnter2();
        AtomicIncrement(&gCallbackDepth, 1);
    }
}

void SystemLeaveCallback(unsigned char lock)
{
    if (gPlatformThreaded) {
        TaskStorageValue depth;

        MPGetTaskStorageValue(gCallbackIndex);
        if (lock && gPlatformMutex != 0)
            _MutexLeave(gPlatformMutex);
        if (gCallbackIndex != kNoTaskIndex && (depth = MPGetTaskStorageValue(gCallbackIndex)) != 0)
            MPSetTaskStorageValue(gCallbackIndex, depth - 1);
    } else {
        if (gCallbackDepth != 0)
            AtomicIncrement(&gCallbackDepth, -1);
        if (gCallbackDepth == 0)
            _AtomicLeave2();
    }
}

unsigned char SystemCheckCallback(void)
{
    unsigned char result;

    if (gPlatformThreaded) {
        TaskStorageIndex index;
        ULong inside;

        inside = 0;
        index = gCallbackIndex;
        if (index != kNoTaskIndex && MPGetTaskStorageValue(index) != 0)
            inside = 1;
        result = inside != 0;
    } else
        result = gCallbackDepth != 0 ? true : false;
    return result;
}

unsigned char _PlatformIsThreaded(void)
{
    unsigned char threaded;
    long response;

    threaded = 0;
    if (gPlatformOpen)
        threaded = gPlatformThreaded;
    else if (Gestalt(gestaltSystemVersion, &response) == 0 && response >= kSystemTen)
        threaded = 1;
    return threaded;
}

#else

static void _SystemQuitCallback(void)
{
    PlatformCallback fn;

    if (!sInQuit) {
        sInQuit = 1;
        gCallbackDepth = 0;
        if (SystemCheckCallback()) {
            while (SystemCheckCallback())
                SystemLeaveCallback(1);
        }
        if (_AtomicCheckCritical())
            _AtomicResetCritical();
        _ThreadCheckCritical();
        while (gSystemQuitCount != 0) {
            fn = gSystemQuitProcs[--gSystemQuitCount];
            if (fn != 0)
                fn();
        }
        _SystemUnpatchTraps();
        sInQuit = 0;
    }
}

void SystemEnterCallback(int lock)
{
    if (gCallbackDepth == 0)
        _AtomicEnter2();
    AtomicIncrement(&gCallbackDepth, 1);
}

void SystemLeaveCallback(int lock)
{
    if (gCallbackDepth != 0)
        AtomicIncrement(&gCallbackDepth, -1);
    if (gCallbackDepth == 0)
        _AtomicLeave2();
}

unsigned char SystemCheckCallback(void)
{
    return gCallbackDepth != 0;
}

unsigned char _PlatformIsThreaded(void)
{
    unsigned char threaded;

    threaded = 0;
    if (gPlatformOpen)
        threaded = gPlatformThreaded;
    return threaded;
}

#endif

void *_PlatformFindSymbol(const char *library, const char *name)
{
    CFragSymbolClass symClass;
    Ptr address;
    Ptr main;
    CFragConnectionID connection;
    Str255 fragment;
    Str255 symbol;
    Str255 error;

    address = 0;
    main = 0;
    StringC2PCopy(fragment, library);
    StringC2PCopy(symbol, name);
    if (GetSharedLibrary(fragment, kPowerPCCFragArch, kLoadCFrag, &connection, &main, error) != 0)
        address = 0;
    else if (FindSymbol(connection, symbol, &address, &symClass) != 0)
        address = 0;
    return address;
}

#if TARGET_API_MAC_CARBON

void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName)
{
    OSStatus err;
    CFBundleRef bundle;
    CFURLRef url;
    CFURLRef base;
    void *symbol;

    err = 0;
    bundle = 0;
    url = 0;
    base = 0;
    symbol = 0;
    if (!sFound) {
        long e;

        e = FSFindFolder(kOnAppropriateDisk, kFrameworksFolderType, kCreateFolder, &sFrameworks);
        if ((short)e != 0) {
            err = (short)e;
            goto done;
        }
        sFound = 1;
    }
    if ((base = CFURLCreateFromFSRef(0, &sFrameworks)) == 0) {
        err = coreFoundationUnknownErr;
        goto done;
    }
    if ((url = CFURLCreateCopyAppendingPathComponent(0, base, bundleName, false)) == 0) {
        err = coreFoundationUnknownErr;
        goto done;
    }
    if ((bundle = CFBundleCreate(0, url)) == 0) {
        err = coreFoundationUnknownErr;
        goto done;
    }
    if (!CFBundleLoadExecutable(bundle)) {
        err = coreFoundationUnknownErr;
        goto done;
    }
    if ((symbol = CFBundleGetFunctionPointerForName(bundle, symbolName)) == 0)
        err = cfragNoSymbolErr;
done:
    if ((void *)CFRelease != 0) {
        if (base != 0)
            CFRelease(base);
        if (url != 0)
            CFRelease(url);
        if (err != 0 && bundle != 0)
            CFRelease(bundle);
    }
    return symbol;
}

static ULong sCFMGlue[6] = { 0x3d800000, 0x618c0000, 0x800c0000, 0x804c0004, 0x7c0903a6, 0x4e800420 };

void *_MachOForCFM(void *function)
{
    ULong *glue;

    glue = MemoryAllocate(sizeof(sCFMGlue));
    if (glue != 0) {
        glue[0] = sCFMGlue[0] | ((ULong)function >> 16);
        glue[1] = sCFMGlue[1] | ((ULong)function & 0xffff);
        glue[2] = sCFMGlue[2];
        glue[3] = sCFMGlue[3];
        glue[4] = sCFMGlue[4];
        glue[5] = sCFMGlue[5];
        MakeDataExecutable(glue, sizeof(sCFMGlue));
    }
    return glue;
}

long _SystemPatchTraps(void)
{
    long err, e;
    ULong *patch;
    void *routine;

    err = 0;
    patch = 0;
    routine = 0;
    if (gPlatformThreaded) {
        if (!gAtExitInstalled) {
            void *glue;

            glue = _MachOForCFM(_SystemQuitCallback);
            if (glue == 0) {
                err = kNoMemory;
                goto done;
            }
            if (gAtExit(glue) != 0) {
                err = kNoMemory;
                goto done;
            }
            gAtExitInstalled = 1;
        }
        if (!gSignalsInstalled) {
            e = _SystemInstallSignals();
            if (e != 0) {
                err = e;
                goto done;
            }
            gSignalsInstalled = 1;
        }
    }
    if (!gPlatformThreaded && sPatchA == 0) {
        if ((patch = MemoryAllocate(kPatchBytes)) == 0) {
            err = kNoMemory;
            goto done;
        }
        if ((routine = gNewRoutineDescriptor(_SystemQuitCallback, 1, 1)) == 0) {
            err = kNoMemory;
            goto done;
        }
        MemoryCopy(patch, sTrapPatch, kPatchBytes);
        patch[kPatchRoutine] = (ULong)routine;
        patch[kPatchOldTrap] = gNGetTrapAddress(kTrapExitToShell, 1);
        gNSetTrapAddress(patch, kTrapExitToShell, 1);
        sPatchA = patch;
        routine = 0;
        patch = 0;
    }
    if (!gPlatformThreaded && sPatchB == 0) {
        if ((patch = MemoryAllocate(kPatchBytes)) == 0) {
            err = kNoMemory;
            goto done;
        }
        if ((routine = gNewRoutineDescriptor(_SystemIdleTick, 1, 1)) == 0) {
            err = kNoMemory;
            goto done;
        }
        MemoryCopy(patch, sTrapPatch, kPatchBytes);
        patch[kPatchRoutine] = (ULong)routine;
        patch[kPatchOldTrap] = gNGetTrapAddress(kTrapSystemTask, 1);
        gNSetTrapAddress(patch, kTrapSystemTask, 1);
        sPatchB = patch;
        routine = 0;
        patch = 0;
    }
    if (gPlatformThreaded && gIdleTimer == 0) {
        if (gIdleTimerUPP == 0) {
            gIdleTimerUPP = NewEventLoopTimerUPP(_SystemIdleTimer);
            if (gIdleTimerUPP == 0) {
                err = kNoMemory;
                goto done;
            }
        }
        if (InstallEventLoopTimer(GetMainEventLoop(), kIdleTimerSeconds, kIdleTimerSeconds, gIdleTimerUPP, 0, &gIdleTimer) != 0)
            err = kNoGestalt;
    }
done:
    if (patch != 0)
        MemoryDeallocate(patch);
    if (routine != 0)
        MemoryDeallocate(routine);
    return err;
}

void _SystemUnpatchTraps(void)
{
    if (gIdleTimer != 0) {
        RemoveEventLoopTimer(gIdleTimer);
        gIdleTimer = 0;
    }
    if (gIdleTimerUPP != 0) {
        DisposeEventLoopTimerUPP(gIdleTimerUPP);
        gIdleTimerUPP = 0;
    }
    gAtExitInstalled = 0;
}

void _SystemIdleTick(void)
{
    if (TickCount() - gLastIdleTick >= 6) {
        gInIdleTick = 1;
        SystemIdle();
        gInIdleTick = 0;
    }
}

pascal void _SystemIdleTimer(EventLoopTimerRef timer, void *ref)
{
    if (TickCount() - gLastIdleTick >= 6) {
        gInIdleTick = 1;
        SystemIdle();
        gInIdleTick = 0;
    }
}

#else

long _SystemPatchTraps(void)
{
    long err;
    ULong *patch;
    void *routine;

    err = 0;
    patch = 0;
    routine = 0;
    if (!gPlatformThreaded && sPatchA == 0) {
        if ((patch = MemoryAllocate(kPatchBytes)) == 0) {
            err = kNoMemory;
            goto done;
        }
        if ((routine = NewRoutineDescriptor((ProcPtr)_SystemQuitCallback, 1, 1)) == 0) {
            err = kNoMemory;
            goto done;
        }
        MemoryCopy(patch, sTrapPatch, kPatchBytes);
        patch[kPatchRoutine] = (ULong)routine;
        patch[kPatchOldTrap] = (ULong)NGetTrapAddress(kTrapExitToShell, ToolTrap);
        NSetTrapAddress((UniversalProcPtr)patch, kTrapExitToShell, ToolTrap);
        sPatchA = patch;
        routine = 0;
        patch = 0;
    }
    if (!gPlatformThreaded && sPatchB == 0) {
        if ((patch = MemoryAllocate(kPatchBytes)) == 0) {
            err = kNoMemory;
            goto done;
        }
        if ((routine = NewRoutineDescriptor((ProcPtr)_SystemIdleTick, 1, 1)) == 0) {
            err = kNoMemory;
            goto done;
        }
        MemoryCopy(patch, sTrapPatch, kPatchBytes);
        patch[kPatchRoutine] = (ULong)routine;
        patch[kPatchOldTrap] = (ULong)NGetTrapAddress(kTrapSystemTask, ToolTrap);
        NSetTrapAddress((UniversalProcPtr)patch, kTrapSystemTask, ToolTrap);
        sPatchB = patch;
        routine = 0;
        patch = 0;
    }
done:
    if (patch != 0)
        MemoryDeallocate(patch);
    if (routine != 0)
        MemoryDeallocate(routine);
    return err;
}

void _SystemUnpatchTraps(void)
{
    if (sPatchA != 0) {
        ((long *)sPatchA)[4] = 0;
        sPatchA = 0;
    }
    if (sPatchB != 0) {
        ((long *)sPatchB)[4] = 0;
        sPatchB = 0;
    }
}

void _SystemIdleTick(void)
{
    if (TickCount() - gLastIdleTick >= 6) {
        gInIdleTick = 1;
        SystemIdle();
        gInIdleTick = 0;
    }
}

#endif

void _AtomicEnter(void)
{
}

void _AtomicLeave(void)
{
}

void _AtomicEnter2(void)
{
}

void _AtomicLeave2(void)
{
}

#if TARGET_API_MAC_CARBON

void _SystemSignalHandler(ULong sig)
{
    long saved;
    long i;

    saved = *gErrno;
    for (i = 33; i != 0; i--)
        ;
    SystemEnterCallback(1);
    switch (sig) {
    case 1:
        gExit(0);
        break;
    case 2:
        gExit(0);
        break;
    case 4:
    case 6:
    case 10:
    case 11:
        gExit(-1);
        break;
    }
    *gErrno = saved;
    SystemLeaveCallback(1);
}

static long _SystemInstallSignals(void)
{
    long err;
    ULong sig;
    void *glue;

    err = 0;
    glue = _MachOForCFM(_SystemSignalHandler);
    if (glue == 0) {
        err = kNoMemory;
    } else {
        for (sig = 1; sig <= 31; sig++) {
            if (sig - 1 <= 1 || sig == 4 || sig == 10 || sig == 11)
                gSignal(sig, glue);
        }
    }
    return err;
}

#endif
