/* reg_tool_3.c -- RT3, the registration toolkit's main tool: the license file and usage log in CFPreferences, the network clock, Open and Close; the mac 1.1.1 librt3_nonag.a(reg_tool_3.o) (mac only). Notes: Research/notes/rt3mac.md */
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include "rt3_lib.h"

extern unsigned short gRT3_CodeVersion;

#define kStrUnlicensedName "NOT REGISTERED"
#define kStrUnlicensedCopies "N/A"
#define kSecondsPerHour 3600
#define kSecondsPerDay 86400
#define kLogGrace (7 * kSecondsPerDay)
#define kLogStale (28 * kSecondsPerDay)
#define kFolderCount 2
#define kPortableMachine 0xa5a5a5a5
#define kLicenseMaxSize 0x5000
#define kTamperMask 0x07bdfef7fbdfef78ULL
#define kPackTag 0x7a6c6962
#define kLicenseSuffix " License"
#define kLicenseType 0x416c6963
#define kLicenseCreator 0x41726567
#define kModeWindows 3
#define kModeMacOSX 0x8003
#define kNTPToUnixSeconds 2208988800UL
#define kBadArgument 0xffffe4a6
#define kOutOfMemory 0xffffe4a4
#define kNoSymbol 0xffffe4a5


extern void ClockQueryDispose(void);
extern long ClockQueryServer(char *server, ULong timeout, ULongLong *outA, ULongLong *outB, ULongLong *response, long *errorOut);

/* data, declared in the reverse of the exe's order (gcc emits initialised data last-declared first) */
unsigned char gRT3_Open = 0;
void **gRT3_LicenseHdl = 0;
long gRT3_LicensePaths[2] = { -3, -7 };
ULong gRT3_LicenseDates[2] = { 0, 0 };
ULong gRT3_StartupTime = 0;
ULong gSavedWhenFirstRun = 0xffffffff;
ULong gSavedNumLaunches = 0;
ULong gSavedUsageClock = 0;
ULong gLicenseFileReset = 0;
extern RT3License gRT3_License;
char *gRT3_IsRegisteredPtr3 = gRT3_License.code;
ULong gRT3_ClockLastTick = 0;
ULong gRT3_ClockLastIndex = 0;
ULong gRT3_ClockCompleted = 0;
/* the time servers' names, each byte XORed with 0xc5 (even positions) or 0xa9 (odd) */
char gRT3_ClockString0[] = "\xb1\xc0\xa8\xcc\xeb\xc8\xb5\xd9\xa9\xcc\xeb\xca\xaa\xc4";
char gRT3_ClockString1[] = "\xb1\xc0\xa8\xcc\xeb\xc7\xac\xda\xb1\x87\xa2\xc6\xb3";
char gRT3_ClockString2[] = "\xb0\xdd\xa6\xc7\xac\xda\xb1\x87\xa6\xc6\xa9\xc6\xb7\xc8\xa1\xc6\xeb\xcc\xa1\xdc";
char gRT3_ClockString3[] = "\xb1\xc0\xa6\xc2\xeb\xdc\xb6\xc7\xaa\x87\xab\xc8\xb3\xd0\xeb\xc4\xac\xc5";
char *gRT3_ClockStrings[4] = { gRT3_ClockString0, gRT3_ClockString1, gRT3_ClockString2, gRT3_ClockString3 };

char gDefaultLicenseCode[kTextSize];
char gDefaultLicenseCopies[kTextSize];
char gDefaultLicenseName[kTextSize];
char gDisplayLicenseCopies[kTextSize];
char gDisplayProductName[kTextSize];
RT3LogEntry gLogEntry;
unsigned char gLogStuff;
ULongLong gMainHash;
char gPrivateLicenseName[kTextSize];
char gPrivateProductName[kTextSize];
ULongLong gRT3_ClockResponses[4];
RT3License gRT3_License;
unsigned char gRT3_SystemWide;
extern const void *gRT3_IsRegisteredPtr1;
extern unsigned char *gRT3_IsRegisteredPtr2;

// MAC: __RT3_LicenseSwap i386 0x000d0c76 ppc 0x000dca3c
extern void _RT3_LicenseSwap(RT3License **license);
inline void _RT3_LicenseSwap(RT3License **license)
{
    (*license)->copies = SWAP32((*license)->copies);
    (*license)->machine = SWAP32((*license)->machine);
    (*license)->level = SWAP32((*license)->level);
    (*license)->user = SWAP32((*license)->user);
    (*license)->reserved[0] = SWAP32((*license)->reserved[0]);
    (*license)->reserved[1] = SWAP32((*license)->reserved[1]);
}

// MAC: __RT3_LogInfoSwap i386 0x000d0da2 ppc 0x000dca40
extern void _RT3_LogInfoSwap(RT3LogEntry *entry);
inline void _RT3_LogInfoSwap(RT3LogEntry *entry)
{
    entry->tamper[0] = SWAP64(entry->tamper[0]);
    entry->tamper[1] = SWAP64(entry->tamper[1]);
    entry->firstLaunch = SWAP32(entry->firstLaunch);
    entry->secondsUsed = SWAP32(entry->secondsUsed);
    entry->launches = SWAP32(entry->launches);
    entry->closeStamp = SWAP32(entry->closeStamp);
    entry->saveStamp = SWAP32(entry->saveStamp);
    entry->reserved[0] = SWAP32(entry->reserved[0]);
    entry->reserved[1] = SWAP32(entry->reserved[1]);
}

// MAC: __RT3_CalcLoginHash i386 0x000d10c6 ppc 0x000dca44
static ULong _RT3_CalcLoginHash(void)
{
    struct passwd *pw;
    char *name;
    ULong hash;

    hash = 0;
    pw = getpwuid(getuid());
    if (pw != 0) {
        name = pw->pw_name;
        if (name != 0) {
            hash = MemoryChecksum(name, StringGetLength(name));
            if (hash == 0)
                hash = 0xffffffff;
        }
    }
    return hash;
}

/* FSEvents exists from 10.5 on: the entry points are loaded by name when the first license folder watch is installed */
typedef struct FSEventStreamRT3 *FSEventStreamRT3Ref;

static void (*gMyFSEventStreamRelease)(FSEventStreamRT3Ref) = 0;
static void (*gMyFSEventStreamInvalidate)(FSEventStreamRT3Ref) = 0;
static void (*gMyFSEventStreamStop)(FSEventStreamRT3Ref) = 0;
static FSEventStreamRT3Ref gRT3_FSEventStream = 0;
static void *gMyFSEventCallback = 0;
static Boolean (*gMyFSEventStreamStart)(FSEventStreamRT3Ref) = 0;
static void (*gMyFSEventStreamScheduleWithRunLoop)(FSEventStreamRT3Ref, CFRunLoopRef, CFStringRef) = 0;
static FSEventStreamRT3Ref (*gMyFSEventStreamCreate)(CFAllocatorRef, void *, void *, CFArrayRef, UInt64, CFTimeInterval, UInt32) = 0;

static long _RT3_LicenseRefresh(void);
static long _RT3_ReadLogEntry(RT3LogEntry *entry);
static long _RT3_WriteLogEntry(RT3LogEntry *entry);
static long _RT3_LicenseSave(void **license, ULong key, ULong when, unsigned char systemWide);
static long _RT3_LicenseLoad(void ***license, unsigned char *created, unsigned char *systemWide);
void RT3_Idle(void);

// MAC: __RT3_LicenseRefresh i386 0x000d3dc4 ppc 0x000de8dc
static long _RT3_LicenseRefresh(void)
{
    OSStatus err;
    ULong dates[kFolderCount] = { 1, 1 };
    unsigned char changed;
    unsigned char created;
    FTFileInfo info;
    char fileName[kTextSize];
    ULong i;

    err = 0;
    changed = 0;
    created = 0;
    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe(fileName, kLicenseSuffix, kTextSize);
    for (i = 0; i < kFolderCount; i++) {
        err = FT_FileGetFlags(gRT3_LicensePaths[i], fileName, 0, 0);
        if (err == 0) {
            err = FT_FileGetFlags(gRT3_LicensePaths[i], fileName, &info, 0);
            if (err == 0)
                dates[i] = info.modified;
        }
        if (dates[i] != gRT3_LicenseDates[i])
            changed = 1;
    }
    err = 0;

    if (gRT3_LicenseHdl != 0 && changed) {
        IndirectDeallocate(gRT3_LicenseHdl);
        gRT3_LicenseHdl = 0;
    }
    if (gRT3_LicenseHdl == 0) {
        err = _RT3_LicenseLoad(&gRT3_LicenseHdl, &created, &gRT3_SystemWide);
        if (err != 0)
            goto done;
        MemoryCopy(&gRT3_License, *gRT3_LicenseHdl, sizeof(RT3License));
        MemoryCopy(gRT3_LicenseDates, dates, sizeof(dates));
        changed = 1;
    }
    if (changed) {
        if (gRT3_License.name[0] != 0)
            RT3_FixLicenseName(gPrivateLicenseName, gRT3_License.name);
        else
            StringCopy(gPrivateLicenseName, gRT3_License.name);
        if (gRT3_License.copies > 0)
            StringFromNumber(gDisplayLicenseCopies, gRT3_License.copies);
        else
            StringCopy(gDisplayLicenseCopies, gDefaultLicenseCopies);
        *gRT3_IsRegisteredPtr2 = gRT3_License.copies > 0 ? 1 : 0;
        gMainHash = RT3_CalcMainHash(gPrivateProductName, gPrivateLicenseName, gRT3_License.copies);
        if (created)
            _RT3_LicenseSave(gRT3_LicenseHdl, 0, 0, 0);
    }
done:
    return err;
}

// MAC: __RT3_FSEventCallback i386 0x000d41fb ppc 0x000dece0
static void _RT3_FSEventCallback(void *stream, void *info, size_t count, void *paths, const void *flags, const void *ids)
{
    long err = 0;

    err = _RT3_LicenseRefresh();
    if (err != 0)
        goto done;
done:
    ;
}

// MAC: __RT3_InstallFSEventCallback i386 0x000d1159 ppc 0x000dcb34
long _RT3_InstallFSEventCallback(void)
{
    CFStringRef paths[2] = { CFSTR(""), CFSTR("") };
    long err = 0;
    CFArrayRef array = 0;
    FSRef folder;
    char path[0x400];
    long e;
    long i;

    if (gMyFSEventStreamCreate == 0) {
        gMyFSEventStreamCreate = SystemLoadMachOSymbol(CFSTR("CoreServices.framework"), CFSTR("FSEventStreamCreate"));
        if (gMyFSEventStreamCreate == 0)
            goto noSymbol;
    }
    if (gMyFSEventStreamScheduleWithRunLoop == 0) {
        gMyFSEventStreamScheduleWithRunLoop = SystemLoadMachOSymbol(CFSTR("CoreServices.framework"), CFSTR("FSEventStreamScheduleWithRunLoop"));
        if (gMyFSEventStreamScheduleWithRunLoop == 0)
            goto noSymbol;
    }
    if (gMyFSEventStreamStart == 0) {
        gMyFSEventStreamStart = SystemLoadMachOSymbol(CFSTR("CoreServices.framework"), CFSTR("FSEventStreamStart"));
        if (gMyFSEventStreamStart == 0)
            goto noSymbol;
    }
    if (gMyFSEventStreamStop == 0) {
        gMyFSEventStreamStop = SystemLoadMachOSymbol(CFSTR("CoreServices.framework"), CFSTR("FSEventStreamStop"));
        if (gMyFSEventStreamStop == 0)
            goto noSymbol;
    }
    if (gMyFSEventStreamInvalidate == 0) {
        gMyFSEventStreamInvalidate = SystemLoadMachOSymbol(CFSTR("CoreServices.framework"), CFSTR("FSEventStreamInvalidate"));
        if (gMyFSEventStreamInvalidate == 0)
            goto noSymbol;
    }
    if (gMyFSEventStreamRelease == 0) {
        gMyFSEventStreamRelease = SystemLoadMachOSymbol(CFSTR("CoreServices.framework"), CFSTR("FSEventStreamRelease"));
        if (gMyFSEventStreamRelease == 0)
            goto noSymbol;
    }
    if (gMyFSEventCallback == 0) {
        gMyFSEventCallback = SystemCreateMachOWrapper(_RT3_FSEventCallback);
        if (gMyFSEventCallback == 0)
            goto noMemory;
    }

    e = FSFindFolder(kUserDomain, kPreferencesFolderType, false, &folder);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FSRefMakePath(&folder, (UInt8 *)path, sizeof(path));
    if (e != 0) {
        err = e;
        goto done;
    }
    paths[0] = CFStringCreateWithCString(0, path, kCFStringEncodingUTF8);
    if (paths[0] == 0)
        goto noMemory;
    e = FSFindFolder(kLocalDomain, kPreferencesFolderType, false, &folder);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FSRefMakePath(&folder, (UInt8 *)path, sizeof(path));
    if (e != 0) {
        err = e;
        goto done;
    }
    paths[1] = CFStringCreateWithCString(0, path, kCFStringEncodingUTF8);
    if (paths[1] == 0)
        goto noMemory;
    array = CFArrayCreate(0, (const void **)paths, 2, 0);
    if (array == 0)
        goto noMemory;
    gRT3_FSEventStream = gMyFSEventStreamCreate(0, gMyFSEventCallback, 0, array, 0xffffffffffffffffULL, 2.0, 0);
    if (gRT3_FSEventStream == 0)
        goto noMemory;
    gMyFSEventStreamScheduleWithRunLoop(gRT3_FSEventStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    gMyFSEventStreamStart(gRT3_FSEventStream);
    goto done;
noSymbol:
    err = kNoSymbol;
    goto done;
noMemory:
    err = kOutOfMemory;
done:
    for (i = 0; i < 2; i++) {
        if (paths[i] != 0)
            CFRelease(paths[i]);
    }
    if (array != 0)
        CFRelease(array);
    if (err != 0)
        _RT3_RemoveFSEventCallback();
    return err;
}

// MAC: __RT3_RemoveFSEventCallback i386 0x000d110b ppc 0x000dcaac
void _RT3_RemoveFSEventCallback(void)
{
    if (gRT3_FSEventStream != 0) {
        gMyFSEventStreamStop(gRT3_FSEventStream);
        gMyFSEventStreamInvalidate(gRT3_FSEventStream);
        gMyFSEventStreamRelease(gRT3_FSEventStream);
        gRT3_FSEventStream = 0;
    }
}

// MAC: _RT3_Idle i386 0x000d4194 ppc 0x000dec50
void RT3_Idle(void)
{
    static ULong lastCheck;
    ULong elapsed;

    if (gRT3_Open) {
        elapsed = TimerGetTicks() - lastCheck;
        if (lastCheck == 0 || elapsed > TimerGetTickRate() * 2) {
            _RT3_LicenseRefresh();
            lastCheck = TimerGetTicks();
        }
#if TARGET_RT_LITTLE_ENDIAN
        *(ULong *)(gRT3_IsRegisteredPtr2 - 3) ^= lastCheck >> 8;
#else
        *(ULong *)(gRT3_IsRegisteredPtr2 - 3) ^= lastCheck << 8;
#endif
    }
}

// MAC: _RT3_GetLicenseCopies i386 0x000d14e7 ppc 0x000dced0
long RT3_GetLicenseCopies(void)
{
    long copies = gRT3_License.copies;
    long result;

    if (copies <= 0)
        result = 0;
    else
        result = copies;
    return result;
}

// MAC: _RT3_Open i386 0x000d45df ppc 0x000dee9c
long RT3_Open(unsigned char tracked, unsigned short mode, const char *product, const char *company)
{
    long err = 0;
    long e;

    if (gRT3_Open != 0)
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
    if (mode != kModeWindows && mode != kModeMacOSX) {
        err = kBadArgument;
        goto done;
    }
    RT3_InitTables();
    if (tracked) {
        StringCopySafe(gDefaultLicenseName, kStrUnlicensedName, kTextSize);
        StringCopySafe(gDefaultLicenseCopies, kStrUnlicensedCopies, kTextSize);
        StringCopySafe(gDefaultLicenseCode, kStrUnlicensedCopies, kTextSize);
    }
    gRT3_CodeVersion = mode;
    StringCopy(gDisplayProductName, company);
    StringCopy(gDisplayLicenseCopies, gDefaultLicenseCopies);
    StringCopy(gPrivateProductName, product);
    MemoryClear(gPrivateLicenseName, kTextSize);
    gLogStuff = tracked;
    gLicenseFileReset = 0;
    if (tracked)
        gRT3_StartupTime = TimerGetElapsedSeconds();
    e = _RT3_ReadLogEntry(&gLogEntry);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _RT3_LicenseRefresh();
    if (e != 0) {
        err = e;
        goto done;
    }
    e = _RT3_InstallFSEventCallback();
    if (e != 0)
        SystemSetIdleProc(RT3_Idle);
    gRT3_Open = 1;
done:
    return err;
}

// MAC: _RT3_IsOpen i386 0x000d1505 ppc 0x000dcf00
unsigned char RT3_IsOpen(void)
{
    return gRT3_Open;
}

// MAC: _RT3_GetDisplayName i386 0x000d1517 ppc 0x000dcf1c
char *RT3_GetDisplayName(void)
{
    if (gRT3_License.name[0] != 0)
        return gRT3_License.name;
    return gDefaultLicenseName;
}

// MAC: _RT3_GetDisplayCode i386 0x000d1533 ppc 0x000dcf4c
char *RT3_GetDisplayCode(void)
{
    if (gRT3_License.code[0] != 0)
        return gRT3_License.code;
    return gDefaultLicenseCode;
}

// MAC: _RT3_GetDisplayCopies i386 0x000d155b ppc 0x000dcf84
char *RT3_GetDisplayCopies(void)
{
    char *result = 0;

    if (gRT3_License.copies > 0)
        result = StringFromNumber(gDisplayLicenseCopies, gRT3_License.copies);
    else
        result = StringCopy(gDisplayLicenseCopies, gDefaultLicenseCopies);
done:
    return result;
}

// MAC: _RT3_GetLicenseCode i386 0x000d15aa ppc 0x000dcfcc
ULongLong RT3_GetLicenseCode(void)
{
    static ULongLong table[8] = { 0, 3, 5, 6, 9, 10, 12, 15 };
    ULongLong code = 0;
    ULongLong now;

    if (gRT3_License.code[0] != 0) {
        RT3_LicenseTextToCode(gRT3_License.code, &code);
        now = TimerGetMicroseconds();
        code = (code ^ gMainHash) | table[now & 7] << 60;
    } else {
        code = 0;
    }
    return code;
}

// MAC: _RT3_GetLicenseName i386 0x000d165a ppc 0x000dd08c
char *RT3_GetLicenseName(void)
{
    return gPrivateLicenseName;
}

// MAC: _RT3_IsRegistered i386 0x000d166b ppc 0x000dd0a8
#ifdef MAC1010
unsigned long RT3_IsRegistered(void)
#else
long RT3_IsRegistered(void)
#endif
{
    long result = 0;

    if (gRT3_License.copies > 0)
        result = 1 << gRT3_License.level;
    return result;
}

// MAC: _RT3_IsRetailLicense i386 0x000d1694 ppc 0x000dd0e0
unsigned char RT3_IsRetailLicense(void)
{
    char name[kTextSize];

    RT3_FixLicenseName(name, gRT3_License.name);
    return name[0] >= '0' && name[0] <= '9';
}

// MAC: _RT3_IsSystemLicense i386 0x000d16d7 ppc 0x000dd134
unsigned char RT3_IsSystemLicense(void)
{
    return gRT3_SystemWide;
}

// MAC: _RT3_CanSetSystemLicense i386 0x000d16eb ppc 0x000dd154
unsigned char RT3_CanSetSystemLicense(void)
{
    return SystemMacOSXOrLater() != 0;
}

// MAC: _RT3_GetDaysHad i386 0x000d1700 ppc 0x000dd17c
ULong RT3_GetDaysHad(void)
{
    ULong now;

    now = TimerGetUTCSeconds();
    if (gLicenseFileReset <= 1000)
        gLicenseFileReset = 0;
    if (now < gLogEntry.firstLaunch)
        return 90;
    return (now - gLogEntry.firstLaunch) / kSecondsPerDay + 1;
}

// MAC: _RT3_GetHoursUsed i386 0x000d1759 ppc 0x000dd1f8
ULong RT3_GetHoursUsed(void)
{
    ULong now;

    now = TimerGetElapsedSeconds();
    if (gRT3_StartupTime != 0)
        return (gLogEntry.secondsUsed + now - gRT3_StartupTime) / kSecondsPerHour;
    return gLogEntry.secondsUsed / kSecondsPerHour;
}

// MAC: _RT3_GetNumLaunches i386 0x000d17aa ppc 0x000dd280
ULong RT3_GetNumLaunches(void)
{
    return gLogEntry.launches;
}

// MAC: __RT3_IdleNetworkClock i386 0x000d17c1 ppc 0x000dd2a0
static void _RT3_IdleNetworkClock(void)
{
    char query[kTextSize];
    unsigned char due;
    ULong index;
    unsigned char gotResponse;
    long i;
    char c;
    const unsigned char *src;

    if (gRT3_ClockCompleted != 0)
        return;
    i = 0;
    while (i < 4 && gRT3_ClockResponses[i] == 0)
        i++;
    if (i < 4)
        gotResponse = 1;
    else
        gotResponse = 0;
    due = gRT3_ClockLastTick == 0;
    if (TimerGetTicks() - gRT3_ClockLastTick >= TimerGetTickRate())
        due = 1;
    if ((due && gRT3_ClockLastIndex == 4) || gotResponse) {
        ClockQueryDispose();
        gRT3_ClockCompleted = TimerGetElapsedSeconds();
    } else if (due) {
        index = gRT3_ClockLastIndex;
        src = (const unsigned char *)gRT3_ClockStrings[index];
        for (i = 0; i < kTextSize && (query[i] = c = src[i]) != 0; i++)
            query[i] = c ^ ((i % 2) ? 0xa9 : 0xc5);
        ClockQueryServer(query, 0, 0, 0, &gRT3_ClockResponses[index], 0);
        gRT3_ClockLastTick = TimerGetTicks();
        gRT3_ClockLastIndex++;
    }
}

// MAC: _RT3_BeginNetworkClock i386 0x000d1919 ppc 0x000dd414
long RT3_BeginNetworkClock(void)
{
    long err = kBadArgument;

    if (gRT3_Open) {
        err = 0;
        if (NetworkStackGetActive(0)) {
            SystemSetIdleProc(_RT3_IdleNetworkClock);
            _RT3_IdleNetworkClock();
        }
    }
    return err;
}

// MAC: _RT3_GetNetworkClock i386 0x000d1963 ppc 0x000dd480
ULong RT3_GetNetworkClock(void)
{
    ULong i;
    ULong seconds;
    ULong ntp;

    if (gRT3_Open) {
        i = 0;
        while (i < 4 && gRT3_ClockResponses[i] == 0)
            i++;
        if (i < 4) {
            seconds = TimerGetElapsedSeconds() - gRT3_ClockCompleted - TimerGetUTCOffset(0);
            ntp = (ULong)(gRT3_ClockResponses[i] >> 32) - kNTPToUnixSeconds;
            return seconds + ntp;
        }
        if (gRT3_ClockCompleted != 0)
            return TimerGetLocalSeconds();
    }
    return 0;
}

// MAC: _RT3_CheckLicenseName i386 0x000d19f0 ppc 0x000dd550
char *RT3_CheckLicenseName(const char *name)
{
    static char buffer[kTextSize];
    char *result = 0;

    if (name)
        RT3_FixLicenseName(buffer, name);
    if (buffer[0])
        result = buffer;
    return result;
}

// MAC: _RT3_CheckLicenseCode i386 0x000d1a2f ppc 0x000dd5b4
ULongLong RT3_CheckLicenseCode(const char *code)
{
    ULongLong value = 0;
    ULong digits = 0;
    ULong i;
    char c;
    unsigned char idx;

    for (i = 0; (c = code[i], idx = c, c != 0); i++)
        if (gRT3_EightBitToFiveBitChars[idx] != kNoDigit)
            digits++;
    if (digits == kCodeDigits)
        RT3_LicenseTextToCode(code, &value);
    return value;
}

// MAC: __RT3_LoadCFPrefs i386 0x000d1a94 ppc 0x000dd64c
static long _RT3_LoadCFPrefs(void ***handle, CFStringRef appID, CFStringRef key)
{
    CFDictionaryRef dict;
    CFDataRef data = 0;

    if (handle == 0)
        goto badParams;
    if (appID == 0)
        goto badParams;
    if (key == 0)
        goto badParams;
    *handle = 0;
    dict = (CFDictionaryRef)CFPreferencesCopyValue(appID, kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
    if (dict != 0 && CFDictionaryGetValueIfPresent(dict, key, (const void **)&data)
        && CFDataGetBytePtr(data) != 0 && CFDataGetLength(data) != 0) {
        *handle = IndirectInitialize(CFDataGetBytePtr(data), CFDataGetLength(data));
        return *handle == 0 ? kOutOfMemory : 0;
    }
    goto notFound;
badParams:
    return 0xffffe4a7;
notFound:
    return 0xffffe3dd;
}

static inline ULong RT3_HashProduct(const char *name)
{
    ULong hash = 0;
    const unsigned char *p = (const unsigned char *)name;
    unsigned char c;

    while ((c = *p) != 0) {
        hash = (hash << 2) ^ (hash >> 29) ^ c;
        p++;
    }
    return hash;
}

// MAC: __RT3_CalcHashedStamp i386 0x000d1b82 ppc 0x000dd750
static ULong _RT3_CalcHashedStamp(void)
{
    long err = 0;
    ULong stamp;

    stamp = TimerGetUTCSeconds() ^ RT3_HashProduct(gPrivateProductName);
    if (stamp == 0)
        stamp = 1;
    return stamp;
}

// MAC: __RT3_CalcIdentifier i386 0x000d1bcf ppc 0x000dd7bc
static ULong _RT3_CalcIdentifier(long folder)
{
    long err = 0;
    ULong hash = 0;
    const unsigned char *p = (const unsigned char *)gPrivateProductName;
    unsigned char c;
    FTFileInfo info;

    while ((c = *p) != 0) {
        hash = (hash << 2) ^ (hash >> 29) ^ c;
        p++;
    }
    err = hash;
    if (folder != 0) {
        if (FT_FileGetFlags(folder, 0, &info, 0) != 0)
            goto done;
        hash ^= info.created;
    }
    if (hash == 0)
        hash = 0xffffffff;
done:
    return hash;
}

// MAC: __RT3_LicenseLoad i386 0x000d35eb ppc 0x000de2f8
static long _RT3_LicenseLoad(void ***license, unsigned char *created, unsigned char *systemWide)
{
    RT3License **h;
    unsigned char valid;
    ULong machine;
    ULong productHash;
    long folder;
    RT3License **best;
    long bestFolder;
    ULong hash;
    ULong u;
    ULong pm;
    long e;
    long diff;
    char fileName[kTextSize];
    ULong i;
    long err;

    err = 0;
    h = 0;
    valid = 0;
    machine = 0;
    best = 0;
    bestFolder = 0;
    folder = 0;
    *license = 0;
    *created = 0;
    if (systemWide != 0)
        *systemWide = 0;
    productHash = RT3_HashProduct(gPrivateProductName);
    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe(fileName, kLicenseSuffix, kTextSize);

    for (i = 0; i < kFolderCount; i++) {
        valid = 1;
        err = FT_FileGetFlags(gRT3_LicensePaths[i], fileName, 0, 0);
        if (err == 0)
            err = FT_FileLoad(gRT3_LicensePaths[i], fileName, kLicenseMaxSize, 0, (void ***)&h);
        if (err == kOutOfMemory) {
            err = kOutOfMemory;
            goto done;
        }
        if (err != 0) {
            valid = 0;
            goto checked;
        }
        err = IndirectDecompress((void **)h, kPackTag, 0);
        if (err != 0 && err != kBadArgument)
            goto done;
        if (err != 0) {
            valid = 0;
            goto checked;
        }
        goto haveH;
    validate:
        if (IndirectGetSize((void **)h) >= sizeof(RT3License) && (*h)->machine != 0) {
            _RT3_LicenseSwap(h);
            machine = (*h)->machine;
            hash = _RT3_CalcIdentifier(gRT3_LicensePaths[i]);
            pm = productHash ^ kPortableMachine;
            if (machine != pm) {
                if (machine != hash) {
                    diff = (machine ^ productHash) - (hash ^ productHash);
                    if (diff < 0)
                        diff *= -1;
                    if (diff > kSecondsPerDay || (diff % 15) * 60 != 0)
                        valid = 0;
                }
            }
            if (!valid)
                goto checked;
            if (gRT3_LicensePaths[i] != -7) {
                if ((*h)->user != 0) {
                    u = _RT3_CalcLoginHash();
                    if (u != 0 && u != (*h)->user)
                        goto invalid;
                }
            }
            goto checked;
        }
        goto invalid;
    haveH:
        if (h != 0)
            goto validate;
    invalid:
        valid = 0;
    checked:
        if (valid) {
            if (best == 0
                || ((*best)->copies == 0 && (*h)->copies != 0)
                || (*best)->level < (*h)->level) {
                if (best != 0)
                    IndirectDeallocate((void **)best);
                bestFolder = gRT3_LicensePaths[i];
                best = h;
                h = 0;
            }
        }
    }

    err = 0;
    if (best != 0) {
        h = best;
        folder = bestFolder;
        valid = 1;
    } else {
        valid = 0;
    }

    if (gLogStuff != 0 && valid && (*h)->copies != 0) {
        e = _RT3_ReadLogEntry(&gLogEntry);
        if (e != 0)
            err = e;
    }

    if (gLogStuff != 0 && valid && (*h)->copies != 0) {
        ULong closeStamp = 0;
        ULong saveStamp = 0;
        long sinceClose = 0;
        long sinceSave = 0;
        unsigned char renew = 0;
        ULong now;

        closeStamp = gLogEntry.closeStamp;
        saveStamp = gLogEntry.saveStamp;
        now = TimerGetUTCSeconds();
        if (closeStamp != 0)
            closeStamp ^= productHash;
        if (saveStamp != 0)
            saveStamp ^= productHash;
        sinceClose = now - closeStamp;
        sinceSave = now - saveStamp;
        if (folder == -7 && (closeStamp != 0 || saveStamp != 0)) {
            renew = 1;
        } else if (closeStamp != 0 && saveStamp != 0) {
            if (sinceClose >= 0 && sinceClose <= kLogGrace
                && (saveStamp + kSecondsPerDay < closeStamp || saveStamp > now))
                valid = 0;
            else
                renew = sinceClose > kLogGrace;
        } else if (closeStamp == 0 && saveStamp != 0) {
            if (sinceSave >= kLogStale)
                valid = 0;
            else
                renew = sinceSave > 0;
        }
        if (valid && renew) {
            gLogEntry.closeStamp = 0;
            gLogEntry.saveStamp = 0;
            e = _RT3_WriteLogEntry(&gLogEntry);
            if (e != 0)
                err = e;
        }
    }

    if (gLogStuff != 0 && valid && (*h)->copies != 0 && gLogEntry.launches != 0 && gLogEntry.launches % 3 == 0) {
        ULongLong code;

        RT3_LicenseTextToCode((*h)->code, &code);
        valid = ((gLogEntry.tamper[0] ^ code) & kTamperMask) != 0 && ((gLogEntry.tamper[1] ^ code) & kTamperMask) != 0;
    }

    if (!valid) {
        if (h == 0) {
            h = (RT3License **)IndirectAllocate(sizeof(RT3License));
            if (h == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else {
            e = IndirectSetSize((void **)h, sizeof(RT3License));
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        MemoryClear(*h, sizeof(RT3License));
        (*h)->copies = 0;
        (*h)->level = 0;
        (*h)->user = 0;
        *created = 1;
    }
done:
    if (err != 0 && h != 0) {
        IndirectDeallocate((void **)h);
        h = 0;
    }
    if (h != 0) {
        *license = (void **)h;
        if (systemWide != 0 && folder == -7)
            *systemWide = 1;
        IndirectSetLock((void **)h, 1);
        gRT3_IsRegisteredPtr1 = *h;
    }
    return err;
}

// MAC: __RT3_LicenseSave i386 0x000d3240 ppc 0x000de0a4
static long _RT3_LicenseSave(void **license, ULong key, ULong when, unsigned char systemWide)
{
    long err;
    long e;
    long folder;
    char fileName[kTextSize];
    ULong copies;
    void **copy;
    char *path;
    long skew;
    ULong stamp;

    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe(fileName, kLicenseSuffix, kTextSize);
    copies = ((RT3License *)*license)->copies;
    folder = systemWide ? -7 : -3;
    err = kOutOfMemory;
    copy = IndirectDuplicate(license);
    if (copy == 0)
        goto done;
    if (key != 0) {
        RT3License *rec = (RT3License *)*copy;
        ULong hash = 0;
        const unsigned char *p = (const unsigned char *)gPrivateProductName;
        unsigned char c;

        while ((c = *p) != 0) {
            hash = (hash << 2) ^ (hash >> 29) ^ c;
            p++;
        }
        rec->machine = hash ^ key;
        ((RT3License *)*copy)->user = 0;
    } else {
        ((RT3License *)*copy)->machine = _RT3_CalcIdentifier(folder);
        if (systemWide)
            ((RT3License *)*copy)->user = 0;
        else
            ((RT3License *)*copy)->user = _RT3_CalcLoginHash();
    }
    _RT3_LicenseSwap((RT3License **)copy);
    e = IndirectCompress(copy, kPackTag, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    if (systemWide && !SystemRunningAsAdmin()) {
        IndirectSetLock(copy, 1);
        err = kOutOfMemory;
        path = GetPathToSystemPreferences(fileName);
        if (path == 0)
            goto done;
        e = WriteToFileAsSuperUser(path, *copy, IndirectGetSize(copy));
        if (e != 0) {
            err = e;
            goto done;
        }
    } else {
        e = FT_FileSave(folder, fileName, 1, kLicenseType, kLicenseCreator, copy);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    gLicenseFileReset++;
    gRT3_LicenseDates[0] = 0;
    _RT3_ReadLogEntry(&gLogEntry);
    skew = when - TimerGetUTCSeconds();
    if (systemWide) {
        gLogEntry.saveStamp = 0;
        gLogEntry.closeStamp = 0;
    } else if (skew >= -kSecondsPerDay && skew < kSecondsPerDay) {
        gLogEntry.saveStamp = 0;
        gLogEntry.closeStamp = 0;
    } else {
        stamp = 0;
        if (copies != 0)
            stamp = _RT3_CalcHashedStamp();
        gLogEntry.saveStamp = stamp;
    }
    err = 0;
    _RT3_WriteLogEntry(&gLogEntry);
done:
    if (copy != 0)
        IndirectDeallocate(copy);
    return err;
}

// MAC: _RT3_Close i386 0x000d44b5 ppc 0x000df044
void RT3_Close(void)
{
    if (RT3_IsOpen()) {
        if (gLogStuff) {
            _RT3_ReadLogEntry(&gLogEntry);
            gSavedNumLaunches++;
            gLogEntry.launches++;
            if (gRT3_StartupTime != 0 && TimerGetElapsedSeconds() > gRT3_StartupTime) {
                gLogEntry.secondsUsed += TimerGetElapsedSeconds() - gRT3_StartupTime;
                gSavedUsageClock += TimerGetElapsedSeconds() - gRT3_StartupTime;
            }
            if (gRT3_License.copies <= 0)
                gLogEntry.closeStamp = _RT3_CalcHashedStamp();
            _RT3_WriteLogEntry(&gLogEntry);
        }
        _RT3_RemoveFSEventCallback();
        gRT3_IsRegisteredPtr1 = "";
        if (gRT3_LicenseHdl != 0) {
            IndirectDeallocate(gRT3_LicenseHdl);
            gRT3_LicenseHdl = 0;
        }
        MemoryClear(gRT3_IsRegisteredPtr2, 1);
        MemoryClear(gRT3_LicenseDates, sizeof(gRT3_LicenseDates));
        gRT3_Open = 0;
    }
}

// MAC: _RT3_IncNumLaunches i386 0x000d4480 ppc 0x000dee3c
long RT3_IncNumLaunches(void)
{
    long err = 0;

    _RT3_ReadLogEntry(&gLogEntry);
    gSavedNumLaunches++;
    gLogEntry.launches++;
    err = _RT3_WriteLogEntry(&gLogEntry);
done:
    return err;
}

// MAC: _RT3_SetLicenseInfo i386 0x000d4205 ppc 0x000dece8
long RT3_SetLicenseInfo(const char *name, const char *code, long copies, ULong level, ULong key, ULong when, unsigned char systemWide)
{
    long err = kBadArgument;
    void **license = 0;
    unsigned char created = 0;
    ULongLong value = 0;
    char text[kTextSize];
    long e;

    if (gRT3_Open) {
        RT3_LicenseTextToCode(code, &value);
        RT3_LicenseCodeToText(value, text);
        err = _RT3_LicenseLoad(&license, &created, 0);
        if (err == 0) {
            StringCopySafe((*(RT3License **)license)->name, name, kTextSize);
            StringCopySafe((*(RT3License **)license)->code, text, kTextSize);
            (*(RT3License **)license)->copies = copies;
            (*(RT3License **)license)->machine = _RT3_CalcIdentifier(0);
            (*(RT3License **)license)->level = level;
            (*(RT3License **)license)->user = _RT3_CalcLoginHash();
            err = _RT3_LicenseSave(license, key, when, systemWide);
            if (err == 0) {
                e = _RT3_LicenseRefresh();
                if (e != 0)
                    err = e;
            }
        }
    }
    if (license != 0)
        IndirectDeallocate(license);
    return err;
}

// MAC: _RT3_CreateLicenseFile i386 0x000d4357 ppc 0x000df19c
long RT3_CreateLicenseFile(const char *name, const char *code, long copies, ULong level, unsigned char systemWide)
{
    long err = kBadArgument;
    char fixed[kTextSize];

    if (gRT3_Open) {
        if (RT3_CheckLicenseName(name) == 0 || RT3_CheckLicenseCode(code) == 0) {
            err = 0xffffe4a7;
        } else {
            RT3_FixLicenseName(fixed, name);
            if (fixed[0] >= '0' && fixed[0] <= '9')
                copies = 1;
            err = RT3_SetLicenseInfo(name, code, copies, level, 0, TimerGetUTCSeconds(), systemWide);
        }
    }
    return err;
}

// MAC: _RT3_RenewLicenseCode i386 0x000d1c4a ppc 0x000df2e0
long RT3_RenewLicenseCode(const char *name, const char *code, long copies, char *newCode, char *message)
{
    long err = kBadArgument;
    long proxyErr;
    char fixedName[kTextSize] = "";
    char codeText[kTextSize] = "";
    char copiesText[kTextSize] = "";
    char proxy[kTextSize] = "";
    ULongLong value2;

    if (gRT3_Open) {
        *newCode = 0;
        *message = 0;
        if (RT3_CheckLicenseName(name) == 0 || RT3_CheckLicenseCode(code) == 0 || copies == 0)
            goto badArgs;
        RT3_LicenseTextToCode(code, &value2);
        RT3_LicenseCodeToText(value2, codeText);
        RT3_FixLicenseName(fixedName, name);
        StringFromNumber(copiesText, copies);
        if (fixedName[0] >= '0' && fixedName[0] <= '9')
            goto badArgs;
        proxyErr = GetHTTPProxy(proxy, 0, 0);
        err = RenewLicenseTCP("register.ambrosiasw.com:80", proxy, gPrivateProductName, name, copiesText, code, newCode, message);
        if (err == 0) {
            if (*newCode == 0)
                err = 0xffffe49e;
            else
                err = proxyErr;
        }
        goto done;
    badArgs:
        err = 0xffffe4a7;
    }
done:
    return err;
}

// MAC: _RT3_EnableCrackDetection i386 0x000d4109 ppc 0x000debb0
void RT3_EnableCrackDetection(unsigned char enable)
{
    long err = 0;

    if (enable) {
        gRT3_IsRegisteredPtr1 = "";
        MemoryClear(&gRT3_License, sizeof(RT3License));
        MemoryClear(gRT3_LicenseDates, sizeof(gRT3_LicenseDates));
        MemoryClear(gRT3_IsRegisteredPtr2, 1);
        MemoryClear(gPrivateLicenseName, kTextSize);
    } else {
        err = _RT3_LicenseRefresh();
        if (err != 0)
            goto done;
    }
done:
    ;
}

// MAC: _RT3_ResetLicenseInfo i386 0x000d408b ppc 0x000deb1c
long RT3_ResetLicenseInfo(void)
{
    long err = kBadArgument;
    long e;
    void **license;

    if (gRT3_Open) {
        err = kOutOfMemory;
        license = IndirectAllocate(sizeof(RT3License));
        if (license != 0) {
            MemoryClear(*license, sizeof(RT3License));
            err = _RT3_LicenseSave(license, 0, 0, 0);
            e = _RT3_LicenseRefresh();
            if (e != 0)
                err = e;
            IndirectDeallocate(license);
        }
    }
    return err;
}

#define kFolderUser -3
#define kLogFileMax 0x32000
#define kLogNotFound 0xffffe3dd
#define kFTErrPermission 0xffffe3d7
#define kLicenseNoType 0x3f3f3f3f
#define kTamperedAge 0x755580
#define kTamperedSeconds 0x57e40
#define kTamperedLaunches 100

/* the log file's name, stored XORed with a falling key: ".DS_Store " */
#define LOG_NAME_INIT { 0x54, 0x33, 0x27, 0x2e, 0x3d, 0x1f, 0x07, 0x17, 0x07, 0x7f }

static void _RT3_SaveCFPrefs(void **handle, CFStringRef appID, CFStringRef key)
{
    unsigned char locked;
    CFMutableDictionaryRef dict;
    CFDataRef data;

    if (handle != 0) {
        locked = IndirectSetLock(handle, 1);
        data = CFDataCreate(0, (const UInt8 *)*handle, IndirectGetSize(handle));
        if (!locked)
            IndirectSetLock(handle, 0);
        if (data != 0) {
            dict = CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            if (dict != 0) {
                CFDictionarySetValue(dict, key, data);
                CFPreferencesSetValue(appID, dict, kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
                CFPreferencesSynchronize(kCFPreferencesAnyApplication, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
                CFRelease(dict);
            }
            CFRelease(data);
        }
    }
}

// MAC: __RT3_WriteLogEntry i386 0x000d1ea5 ppc 0x000dd850
static long _RT3_WriteLogEntry(RT3LogEntry *entry)
{
    long err = 0;
    char name[kTextSize] = LOG_NAME_INIT;
    void **log = 0;
    long e;
    char product[kTextSize];
    ULong count;
    ULong i;
    FTFileInfo info;
    int one = 1; /* V111 pad: a constant local only the pad tests below read */

    _RT3_LogInfoSwap(entry);
    for (i = 0; name[i] != 0; i++) {
        name[i] ^= 0x7a - i * 3;
        one = one; /* V111 pad: rewrites one each pass, keeps it a variable */
    }
    err = FT_FileLoad(kFolderUser, name, kLogFileMax, 0, &log);
    if ((err != 0 || one) && err == kLogNotFound) /* V111 pad: one is always true */
        goto fallback;
    if (IndirectGetSize(log) <= 7) {
fallback:
        if (log != 0) {
            IndirectDeallocate(log);
            log = 0;
        }
        err = _RT3_LoadCFPrefs(&log, CFSTR("com.ambrosiasw"), CFSTR("RT3"));
    }
    if (err == 0) {
        err = IndirectDecompress(log, kPackTag, 0);
        if (err != 0 && err != kBadArgument)
            goto done;
    }
    if (err == 0) {
        count = IndirectGetSize(log) / sizeof(RT3LogEntry);
        for (i = 0; i < count; i++) {
            StringCopySafe(product, (char *)*log + i * sizeof(RT3LogEntry), kTextSize);
            if (StringCompare(gPrivateProductName, product) == 0)
                break;
        }
        if (i == count) {
            e = IndirectAppendData(log, entry, sizeof(RT3LogEntry));
            if (e != 0) {
                err = e;
                goto done;
            }
        } else {
            ((RT3LogEntry *)*log)[i] = *entry;
        }
    } else {
        err = 0;
        log = IndirectInitialize(entry, sizeof(RT3LogEntry));
        if (log == 0) {
            err = kOutOfMemory;
            goto done;
        }
    }
    e = IndirectCompress(log, kPackTag, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    _RT3_SaveCFPrefs(log, CFSTR("com.ambrosiasw"), CFSTR("RT3"));
    e = FT_FileSave(kFolderUser, name, 1, kLicenseNoType, kLicenseNoType, log);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FT_FileGetFlags(kFolderUser, name, &info, 0);
    if (e == 0 && !info.flags[3]) {
        info.flags[0] = 1;
        info.flags[1] = 1;
        info.flags[3] = 1;
        FT_FileSetFlags(kFolderUser, name, &info);
    }
done:
    if (log != 0)
        IndirectDeallocate(log);
    _RT3_LogInfoSwap(entry);
    return err;
}

// MAC: __RT3_ReadLogEntry i386 0x000d2a24 ppc 0x000ddc34
static long _RT3_ReadLogEntry(RT3LogEntry *entry)
{
    long err = 0;
    char name[kTextSize] = LOG_NAME_INIT;
    void **log = 0;
    long e;
    char product[kTextSize];
    unsigned char fromPrefs = 0;
    ULong i;
    ULong count;
    unsigned char tampered = 0;
    unsigned char reset = 0;
    FTFileInfo info;
    int one = 1; /* V111 pad: a constant local only the pad tests below read */

    for (i = 0; name[i] != 0; i++) {
        name[i] ^= 0x7a - i * 3;
        one = one != 0; /* V111 pad: rewrites one each pass, keeps it a variable */
    }
    err = FT_FileLoad(kFolderUser, name, kLogFileMax, 0, &log);
    if ((err != 0 || one) && err == kLogNotFound) /* V111 pad: one is always true */
        goto fallback;
    if (IndirectGetSize(log) <= 7) {
fallback:
        if (log != 0) {
            IndirectDeallocate(log);
            log = 0;
        }
        err = _RT3_LoadCFPrefs(&log, CFSTR("com.ambrosiasw"), CFSTR("RT3"));
        if (log != 0)
            fromPrefs = 1;
    }
    if (err == 0) {
        err = IndirectDecompress(log, kPackTag, 0);
        if (err != 0 && err != kBadArgument)
            goto done;
    }
    if (err == 0) {
        count = IndirectGetSize(log) / sizeof(RT3LogEntry);
        for (i = 0; i < count; i++) {
            StringCopySafe(product, (char *)*log + i * sizeof(RT3LogEntry), kTextSize);
            if (StringCompare(gPrivateProductName, product) == 0)
                break;
        }
        if (i == count) {
            reset = 1;
        } else {
            *entry = ((RT3LogEntry *)*log)[i];
            _RT3_LogInfoSwap(entry);
        }
        IndirectDeallocate(log);
        log = 0;
        e = FT_FileGetFlags(kFolderUser, name, &info, 0);
        if (e == 0) {
            if (!reset) {
                if (!info.flags[3]) {
                    info.flags[0] = 1;
                    info.flags[1] = 1;
                    info.flags[3] = 1;
                    FT_FileSetFlags(kFolderUser, name, &info);
                }
            }
        }
    } else {
        if (err == kFTErrPermission)
            tampered = 1;
        else
            reset = 1;
    }
    if (fromPrefs) {
        e = _RT3_WriteLogEntry(entry);
        if (e != 0 || one) { /* V111 pad: one is always true */
            switch (e) {
            case kFTErrPermission:
            case -7208:
            case -7205:
                goto tamper;
            default:
                err = e;
                goto done;
            }
        }
    } else if (reset) {
        StringCopySafe(entry->product, gPrivateProductName, sizeof(entry->product));
        entry->firstLaunch = TimerGetUTCSeconds();
        entry->secondsUsed = 0;
        entry->launches = 0;
        if (entry->firstLaunch > gSavedWhenFirstRun)
            entry->firstLaunch = gSavedWhenFirstRun;
        if (entry->launches < gSavedNumLaunches)
            entry->launches = gSavedNumLaunches;
        if (entry->secondsUsed < gSavedUsageClock)
            entry->secondsUsed = gSavedUsageClock;
        entry->closeStamp = 0;
        entry->saveStamp = 0;
        entry->reserved[0] = 0;
        entry->reserved[1] = 0;
        e = _RT3_WriteLogEntry(entry);
        if (e != 0 || one) { /* V111 pad: one is always true */
            switch (e) {
            case kFTErrPermission:
            case -7208:
            case -7205:
                goto tamper;
            default:
                err = e;
                goto done;
            }
        }
    }
    if (tampered) {
tamper:
        err = 0;
        StringCopySafe(entry->product, gPrivateProductName, sizeof(entry->product));
        entry->firstLaunch = TimerGetUTCSeconds() - kTamperedAge;
        entry->secondsUsed = kTamperedSeconds;
        entry->launches = kTamperedLaunches;
        entry->closeStamp = 0;
        entry->saveStamp = 0;
        entry->reserved[0] = 0;
        entry->reserved[1] = 0;
    } else {
        err = 0;
        if (gSavedWhenFirstRun > entry->firstLaunch)
            gSavedWhenFirstRun = entry->firstLaunch;
        if (gSavedNumLaunches < entry->launches)
            gSavedNumLaunches = entry->launches;
        if (gSavedUsageClock < entry->secondsUsed)
            gSavedUsageClock = entry->secondsUsed;
    }
done:
    if (log != 0)
        IndirectDeallocate(log);
    return err;
}
