// COMPILE: -opt level=2,speed -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: RT3's own line (rt3.c): the reminder is part of the RT3 library, one compiler and one line for the whole library; its 16-aligned run 0x004ee7a0-0x004eecdf is the speed idiom of line C; Research/runtime-compiler-ceiling.md, notes/rt3nag.md
/* rt3nag.c -- the RT3 trial reminder: the dialog run, its page callback and text variables. Notes: Research/notes/rt3nag.md */

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "win32.h"

struct FolderSpec;

__declspec(dllimport) unsigned long __stdcall SearchPathA(const char *path, const char *file,
                                                          const char *ext, unsigned long size,
                                                          char *buffer, char **filePart);

#define kReminderChunk      950
#define kChunkType          0x43484e4bUL    /* 'CHNK' */
#define kTrialGraceDays     7
#define kLevel2Days         30
#define kLevel2Hours        6
#define kLevel3Days         60
#define kLevel3Hours        12
#define kSecondsPerLevel    5
#define kEventOpen          2
#define kEventShow          3
#define kShellFailed        32
#define SW_SHOWNORMAL       1
#define kBeeps              5
#define kBeepGap            2000

long RT3_SetReminderVariables(void *window);
long RT3_ReminderCallback(WindowNode *window, long refcon, long event, void *where, long item,
                  const char *name);

static unsigned char delayPending;               /* 0x00867174 the NOTYET delay not started */
static ULong delayStart;                         /* 0x00867170 when it started */

static inline long ReminderLaunch(const char *command)
{
    ULong shell;
    long err;
    char path[0x208];
    char *filePart;

    err = 0;
    shell = 0;
    shell = (ULong)ShellExecuteA(_PlatformGetWindow(), "open", command, 0, 0, SW_SHOWNORMAL);
    if (shell <= kShellFailed
        && SearchPathA(0, command, 0, sizeof(path), path, &filePart) != 0)
        shell = (ULong)ShellExecuteA(_PlatformGetWindow(), "open", path, 0, 0, SW_SHOWNORMAL);
    if (shell <= kShellFailed)
        err = shell;
    return err;
}

static inline long ReminderRegister(void)
{
    long err = 0;
    long e;
    char command[256];

    StringCopySafe(command, "Register ", sizeof(command));
    StringAppendSafe((unsigned char *)command, (unsigned char *)gPrivateProductName, sizeof(command));
    StringAppendSafe((unsigned char *)command, (const unsigned char *)".EXE", sizeof(command));
    e = ReminderLaunch(command);
    if (e)
        err = e;
    return err;
}

static inline long ReminderLevel(void)
{
    long level = 1;
    ULong days;
    ULong hours;

    days = RT3_GetDaysHad();
    hours = RT3_GetHoursUsed();
    if (days > kLevel2Days && hours > kLevel2Hours)
        level++;
    if (days > kLevel3Days && hours > kLevel3Hours)
        level++;
    return level;
}

static inline long ReminderRun(struct FolderSpec **folder, unsigned char *registered)
{
    long err = 0;
    long e = 0;
    long result;
    long launch;
    char button[256];

    delayPending = 1;
    e = IT_ViewCreateModal(folder, "classic.iml", 0, RT3_ReminderCallback, ReminderLevel(), button);
    if (e != 0)
        err = e;
    else if (StringCompare(String2Upper(button), "NOTYET") != 0) {
        launch = ReminderRegister();
        if (launch)
            err = launch;
        else if (registered != 0)
            *registered = 1;
    }
    result = err;
    return err;
}

/* Stand-in: the linker strips this; the exe keeps its literals (body not recoverable). */
long RT3_LaunchRegister(void)
{
    return ReminderRegister();
}

/* RT3_RunReminder (the RT3 trial reminder: after the grace week, unless the product is
   registered, or always when forced, run classic.iml from resource CHNK 950 as a dialog at a
   level 1..3 by days and hours used; a button other than NOTYET launches `Register
   <product>.EXE', found through the search path when the shell cannot; `registered' says it
   launched; any failure beeps five times). STATE: MATCH 100%, exe 220 / obj 220,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004ee7a0
long RT3_RunReminder(unsigned char force, unsigned char *registered)
{
    long err = 0;
    void **chunk = 0;
    struct FolderSpec **folder = 0;
    long e;
    ULong i;

    if (registered != 0)
        *registered = 0;
    if (!force && (RT3_GetDaysHad() < kTrialGraceDays || RT3_IsRegistered() != 0))
        goto done;
    if (!DT_IsOpen() && (e = DT_Open()) != 0) {
        err = e;
        goto done;
    }
    if (!IT_IsOpen() && (e = IT_Open()) != 0) {
        err = e;
        goto done;
    }
    e = FT_ResourceLoad(FT_GetResFile(), kReminderChunk, "CHNK", kChunkType, kChunkMax, 0, &chunk);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FT_VirtualPathMount(chunk, 1, &folder);
    if (e != 0) {
        err = e;
        goto done;
    }
    IT_SetCursor(0);
    e = ReminderRun(folder, registered);
    if (e)
        err = e;
done:
    if (folder != 0)
        FT_PathDispose(folder);
    if (chunk != 0)
        IndirectDeallocate(chunk);
    if (err != 0) {
        for (i = 0; i < kBeeps; i++) {
            if (i != 0)
                SystemWait(kBeepGap);
            MessageBeep(0);
        }
    }
    return err;
}

/* RT3_ReminderCallback (the reminder page's callback: its text variables when it opens; when shown,
   NOTYET disabled and shown, HOWLONG shown when there is a delay, the delay clock started on
   the first showing; while idle, NOTYET enabled once refcon x 5 seconds have passed; -7002
   when the page lacks a button). STATE: MATCH 100%, exe 118 / obj 118, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004eea80
long RT3_ReminderCallback(WindowNode *window, long refcon, long event, void *where, long item,
                  const char *name)
{
    long err = 0;
    ULong delay = refcon;
    long index = -1;
    long e;

    switch (event) {
    case kEventOpen:
        RT3_SetReminderVariables(window);
        break;
    case kEventShow:
        index = IT_ViewGetNamedItem(window, "REGISTER");
        if (index == -1) {
            err = kNotAWindow;
            goto done;
        }
        index = IT_ViewGetNamedItem(window, "NOTYET");
        if (index == -1) {
            err = kNotAWindow;
            goto done;
        }
        e = IT_ViewSetItemEnabled(window, index, 0);
        if (e != 0) {
            err = e;
            goto done;
        }
        e = IT_ViewSetItemVisible(window, index, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        index = IT_ViewGetNamedItem(window, "HOWLONG");
        if (index == -1) {
            err = kNotAWindow;
            goto done;
        }
        e = IT_ViewSetItemVisible(window, index, delay != 0 ? (unsigned char)1 : (unsigned char)0);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (delayPending) {
            delayPending = 0;
            delayStart = TimerGetTicks();
        }
    case kEventIdle:
        if (delayPending == 0
            && TimerGetTicks() - delayStart >= TimerGetTickRate() * (delay * kSecondsPerLevel)) {
            index = IT_ViewGetNamedItem(window, "NOTYET");
            if (index == -1) {
                err = kNotAWindow;
                goto done;
            }
            e = IT_ViewSetItemEnabled(window, index, 1);
            if (e != 0)
                err = e;
        }
        break;
    }
done:
    return err;
}

/* RT3_SetReminderVariables (set the reminder dialog's global text variables: the product, the days
   used and the launches, each with its singular or plural word). STATE: MATCH 100%, exe 81 /
   obj 81, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004eebe0
long RT3_SetReminderVariables(void *window)
{
    long err = 0;
    long e;
    ULong n;
    char text[256];

    e = IT_ViewParamText(0, "RT3_PRODUCT", gDisplayProductName);
    if (e != 0) {
        err = e;
        goto done;
    }
    n = RT3_GetDaysHad();
    StringFromNumber(text, n);
    e = IT_ViewParamText(0, "RT3_DAY_COUNT", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ViewParamText(0, "RT3_DAY_WORD", n == 1 ? "day" : "days");
    if (e != 0) {
        err = e;
        goto done;
    }
    n = RT3_GetNumLaunches();
    StringFromNumber(text, n);
    e = IT_ViewParamText(0, "RT3_TIME_COUNT", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ViewParamText(0, "RT3_TIME_WORD", n == 1 ? "time" : "times");
    if (e != 0)
        err = e;
done:
    return err;
}
