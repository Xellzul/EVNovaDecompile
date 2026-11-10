/* rt3nagmac.c -- 1.0.10: the RT3 trial reminder on the Mac: win's rt3nag.c, the page from a resource, Register launched through the Process Manager. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include <CFBundle.h>
#include <CFURL.h>

struct FolderSpec;

#define kReminderChunk      950
#define kChunkType          'CHNK'
#define kTrialGraceDays     7
#define kLevel2Days         30
#define kLevel2Hours        6
#define kLevel3Days         60
#define kLevel3Hours        12
#define kSecondsPerLevel    5
#define kEventOpen          2
#define kEventShow          3
#define kBeeps              5
#define kBeepGap            2000
#define kBeepLength         7
#define kRegisterType       'APPL'
#define kRegisterCreator    'Areg'
#define kMaxMatches         20

#if TARGET_API_MAC_CARBON
#define kReminderPage       0
extern long IT_ViewGetPageElement(WindowNode *window, ITElement **element);
extern long IT_StringTableLoad(ITElement *root, unsigned char keepWide, struct HashTable **out);
extern unsigned char IT_ViewIsWide(WindowNode *window);
extern long IT_ViewParamTextWide(WindowNode *window, const char *key, const unsigned short *value);
extern unsigned char *StringPascalAppend(unsigned char *dst, const unsigned char *src);
extern Boolean CFURLGetFSRef(CFURLRef url, FSRef *fsRef);
#else
#define kReminderPage       "classic.iml"
#endif

extern OSErr NameFileSearch(const unsigned char *volName, short vRefNum, const unsigned char *fileName,
                            FSSpec *matches, long reqMatchCount, long *actMatchCount,
                            Boolean newSearch, Boolean partial);

static long RT3_ReminderCallback(WindowNode *window, long refcon, long event, void *where, long item,
                  const char *name);
static long RT3_LaunchRegister(const char *name, unsigned char search);

static unsigned char delayPending;
static ULong delayStart;

static inline long ReminderRegister(void)
{
    long err = 0;
    long e;
    char command[256];

    StringCopySafe(command, "Register ", sizeof(command));
    StringAppendSafe((unsigned char *)command, (unsigned char *)gPrivateProductName, sizeof(command));
    e = RT3_LaunchRegister(command, 1);
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
    long launch;
    char button[256];

    delayPending = 1;
    e = IT_ViewCreateModal(folder, kReminderPage, 0, RT3_ReminderCallback, ReminderLevel(), button);
    if (e != 0)
        err = e;
    else if (StringCompare(String2Upper(button), "NOTYET") != 0) {
        launch = ReminderRegister();
        if (launch)
            err = launch;
        else if (registered != 0)
            *registered = 1;
    }
    return err;
}

static long RT3_LaunchRegister(const char *name, unsigned char search)
{
    long err = 0;
    short vRefNum;
    long dirID;
    long count;
    ProcessSerialNumber psn;
    FInfo finfo;
    LaunchParamBlockRec launch;
    ProcessInfoRec process;
#if TARGET_API_MAC_CARBON
    FSSpec spec;
    FSSpec bundleSpec;
    FSRef bundleRef;
    Str255 pname;
    Str255 tail;
#else
    FSSpec spec;
    Str255 pname;
#endif
    FSSpec matches[kMaxMatches];

    if (search) {
        unsigned char found = 0;

        StringC2PCopy(pname, name);
        err = HGetVol(0, &vRefNum, &dirID);
        if (err == 0)
            err = FSMakeFSSpec(vRefNum, dirID, pname, &spec);
        if (err == 0)
            found = 1;
        if (!found) {
            process.processAppSpec = &spec;
            psn.highLongOfPSN = 0;
            psn.lowLongOfPSN = kCurrentProcess;
            process.processInfoLength = sizeof(ProcessInfoRec);
            process.processName = pname;
            err = GetProcessInformation(&psn, &process);
            if (err == 0) {
                StringC2PCopy(pname, name);
                vRefNum = spec.vRefNum;
                dirID = spec.parID;
                err = FSMakeFSSpec(spec.vRefNum, spec.parID, pname, &spec);
            }
            if (err == 0)
                found = 1;
        }
#if TARGET_API_MAC_CARBON
        if (!found) {
            CFURLRef url = 0;
            CFBundleRef bundle = CFBundleGetMainBundle();

            if (bundle != 0
                && (url = CFBundleCopyBundleURL(bundle)) != 0
                && CFURLGetFSRef(url, &bundleRef)) {
                err = FSGetCatalogInfo(&bundleRef, 0, 0, 0, &bundleSpec, 0);
                if (err == 0) {
                    StringC2PCopy(pname, name);
                    vRefNum = bundleSpec.vRefNum;
                    dirID = bundleSpec.parID;
                    err = FSMakeFSSpec(bundleSpec.vRefNum, bundleSpec.parID, pname, &spec);
                    if (err == 0) {
                        found = 1;
                    } else {
                        StringC2PCopy(pname, ":");
                        StringPascalAppend(pname, bundleSpec.name);
                        StringPascalAppend(pname, StringC2PCopy(tail, ":Contents:Resources:"));
                        StringPascalAppend(pname, StringC2PCopy(tail, name));
                        vRefNum = bundleSpec.vRefNum;
                        dirID = bundleSpec.parID;
                        err = FSMakeFSSpec(bundleSpec.vRefNum, bundleSpec.parID, pname, &spec);
                        if (err == 0)
                            found = 1;
                    }
                }
            }
            if (url != 0)
                CFRelease(url);
        }
#endif
        if (found) {
            launch.launchFileFlags = 0;
            launch.launchBlockID = extendedBlock;
            launch.launchEPBLength = extendedBlockLen;
            launch.launchControlFlags = launchContinue | launchNoFileFlags;
            launch.launchAppSpec = &spec;
            launch.launchAppParameters = 0;
            err = LaunchApplication(&launch);
            if (err == 0)
                goto done;
        }
    }
    if (search && err == 0)
        goto done;
    count = 0;
    {
        CursHandle cursor = GetCursor(watchCursor);

        if (cursor != 0)
            SetCursor(*cursor);
    }
    StringC2PCopy(pname, name);
    err = NameFileSearch(0, -1, pname, matches, kMaxMatches, &count, 1, 0);
    InitCursor();
    if (count > 0) {
        short i;
        FSSpec *match = matches;
        short best = -1;

        for (i = 0; i < count; match++, i++) {
            FSpGetFInfo(match, &finfo);
            if (finfo.fdType == kRegisterType && finfo.fdCreator == kRegisterCreator
                && (finfo.fdFlags & kIsAlias) == 0)
                best = i;
        }
        if (best != -1) {
            launch.launchBlockID = extendedBlock;
            launch.launchEPBLength = extendedBlockLen;
            launch.launchFileFlags = 0;
            launch.launchAppSpec = &matches[best];
            launch.launchControlFlags = launchContinue | launchNoFileFlags;
            launch.launchAppParameters = 0;
            err = LaunchApplication(&launch);
        }
    }
done:
    return err;
}

#if TARGET_API_MAC_CARBON
/* the word for a count: the window's language table has it wide or plain, else the English one */
#define SET_WORD(name, plural, wide, wides, plain, plains, english, englishes, next)    \
    if (IT_ViewIsWide(window)) {                                                        \
        const unsigned short *w = (const unsigned short *)HashLookupUnicode(strings, (const unsigned short *)((plural) == 1 ? wide : wides), 0); \
        if (w != 0) {                                                                   \
            e = IT_ViewParamTextWide(0, name, w != 0 ? w : kEmptyW);                    \
            if (e != 0) {                                                               \
                err = e;                                                                \
                goto done;                                                              \
            }                                                                           \
            goto next;                                                                  \
        }                                                                               \
    }                                                                                   \
    if (!IT_ViewIsWide(window)) {                                                       \
        const char *s = HashLookup(strings, (plural) == 1 ? plain : plains, 0);         \
        if (s != 0) {                                                                   \
            e = IT_ViewParamText(0, name, s != 0 ? s : "");                             \
            if (e != 0) {                                                               \
                err = e;                                                                \
                goto done;                                                              \
            }                                                                           \
            goto next;                                                                  \
        }                                                                               \
    }                                                                                   \
    e = IT_ViewParamText(0, name, (plural) == 1 ? english : englishes);                 \
    if (e != 0) {                                                                       \
        err = e;                                                                        \
        goto done;                                                                      \
    }                                                                                   \
next:

#define kDayW L"DAY"
#define kDaysW L"DAYS"
#define kTimeW L"TIME"
#define kTimesW L"TIMES"
#define kEmptyW L""

static long RT3_SetReminderVariables(void *window)
{
    long err = 0;
    long e;
    ULong days;
    ULong launches;
    ITElement *index;
    struct HashTable *strings;
    char text[256];

    index = 0;
    strings = 0;
    e = IT_ViewParamText(0, "RT3_PRODUCT", gDisplayProductName);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ViewGetPageElement(window, &index);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_StringTableLoad(index, 0, &strings);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (IT_ViewIsWide(window))
        HashGetFlags(strings);
    if (!IT_ViewIsWide(window))
        HashGetFlags(strings);
    days = RT3_GetDaysHad();
    StringFromNumber(text, days);
    e = IT_ViewParamText(0, "RT3_DAY_COUNT", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    SET_WORD("RT3_DAY_WORD", days, kDayW, kDaysW, "DAY", "DAYS", "day", "days", dayDone)
    launches = RT3_GetNumLaunches();
    StringFromNumber(text, launches);
    e = IT_ViewParamText(0, "RT3_TIME_COUNT", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    SET_WORD("RT3_TIME_WORD", launches, kTimeW, kTimesW, "TIME", "TIMES", "time", "times", timeDone)
done:
    if (strings != 0)
        HashDispose(strings);
    return err;
}
#else
static long RT3_SetReminderVariables(void *window)
{
    long err = 0;
    long e;
    ULong days;
    ULong launches;
    char text[256];

    e = IT_ViewParamText(0, "RT3_PRODUCT", gDisplayProductName);
    if (e != 0) {
        err = e;
        goto done;
    }
    days = RT3_GetDaysHad();
    StringFromNumber(text, days);
    e = IT_ViewParamText(0, "RT3_DAY_COUNT", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ViewParamText(0, "RT3_DAY_WORD", days == 1 ? "day" : "days");
    if (e != 0) {
        err = e;
        goto done;
    }
    launches = RT3_GetNumLaunches();
    StringFromNumber(text, launches);
    e = IT_ViewParamText(0, "RT3_TIME_COUNT", text);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = IT_ViewParamText(0, "RT3_TIME_WORD", launches == 1 ? "time" : "times");
    if (e != 0)
        err = e;
done:
    return err;
}
#endif

static long RT3_ReminderCallback(WindowNode *window, long refcon, long event, void *where, long item,
                  const char *name)
{
    long err = 0;
    long delay = refcon;
    ULong index = -1;
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
        e = IT_ViewSetItemVisible(window, index, AsByte(delay != 0));
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

long RT3_RunReminder(unsigned char force, unsigned char *registered)
{
    long err = 0;
    void **chunk = 0;
    struct FolderSpec **folder = 0;
    Handle resource;
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
    resource = GetResource(kChunkType, kReminderChunk);
    if (resource == 0) {
        ResError();
        err = kOutOfMemory;
        goto done;
    }
    chunk = IndirectDuplicate((void **)resource);
    ReleaseResource(resource);
    if (chunk == 0) {
        err = kOutOfMemory;
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
            SysBeep(kBeepLength);
        }
    }
    return err;
}

/* Stand-in: the linker strips this; the exe keeps its literal (body not recoverable). */
long RT3_Register(void)
{
    return ReminderRegister();
}
