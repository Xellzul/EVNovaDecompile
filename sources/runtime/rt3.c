// COMPILE: -opt level=2,speed -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: RT3 sits in the AmbrosiaTools run (its exe run 0x004d4420-0x004d5cd0 follows the atplat*/atticks run directly and its calls go into that framework's platform, string and handle code), so it takes the library's build 335 and the speed line of the Sound Toolkit next door; the mac has real _RT3_* bodies (isThunk=0), only gRT3_IsRegisteredPtr1/2 are imported, so the mac is no evidence; Research/runtime-compiler-ceiling.md, notes/rt3.md
/* rt3.c -- RT3, the registration toolkit: the license file, the usage log in the registry, the license code. Notes: Research/notes/rt3.md */

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#include "win32.h"
#include "game.h"
#include "missions.h"

#define kModeWindows        3
#define kStrUnlicensedName  0x3b6
#define kStrUnlicensedCopies 0x3b7
#define kStrReserved        0x3b8
#define kTextSize           0x100
#define kSecondsPerHour     3600
#define kSecondsPerDay      86400
#define kLogGrace           (7 * kSecondsPerDay)
#define kLogStale           (28 * kSecondsPerDay)
#define kTamperedAge        (89 * kSecondsPerDay)
#define kTamperedSeconds    (100 * kSecondsPerHour)
#define kTamperedLaunches   100
#define kPortableMachine    0x86868686
#define kPackTag            'zlib'
#define kLicenseType        'Alic'
#define kLicenseCreator     'Areg'
#define kLicenseMaxSize     0x5000
#define kFolderCount        1
#define kCodeDigits         12
#define kCodeBits           5
#define kNoDigit            0xff
#define kNibble             0xf

/* days the trial is taken to have run when the clock is before the first launch */
#define kElapsedDefault 90

/* the product hash every path recomputes from the product name */
#define HASH_PRODUCT(h, p)                                          \
    do {                                                            \
        const unsigned char *q_ = (const unsigned char *)(p);       \
        (h) = 0;                                                    \
        for (; *q_ != 0; q_++)                                      \
            (h) = ((h) >> 29) ^ ((h) << 2) ^ *q_;                   \
    } while (0)

char gDefaultLicenseCode[kTextSize];             /* 0x0086d6a0 */
char gPrivateProductName[kTextSize];             /* 0x0086d7b0 product */
char gDefaultLicenseName[kTextSize];             /* 0x0086d900 unlicensed name */
char gPrivateLicenseName[kTextSize];             /* 0x0086da60 license name */
char gDisplayProductName[kTextSize];             /* 0x0086db70 company */
RT3License gRT3_License;                         /* 0x0086e118 the loaded license */
char gDefaultLicenseCopies[kTextSize];           /* 0x0086e668 unlicensed copies text */
char gDisplayLicenseCopies[kTextSize];           /* 0x0086e768 copies text */
ULongLong gMainHash;                             /* 0x0086e8c0 main hash */
void **gRT3_LicenseHdl;                          /* 0x0086fff0 license handle */
ULong gSavedUsageClock;                          /* 0x00870134 most seconds seen */
ULong gRT3_StartupTime;                          /* 0x0087016c clock at open */
ULong gSavedNumLaunches;                         /* 0x008707b4 most launches seen */
RT3LogEntry gLogEntry;                           /* 0x00870800 this product's log */
unsigned char gLogStuff;                         /* 0x00870e0d usage tracked */
unsigned char gRT3_Open;                         /* 0x00870e0e RT3 open */

static unsigned char gRT3_EightBitToFiveBitChars[256]; /* 0x00865751 digit values */
static unsigned char gInited;                    /* 0x00865750 digit table built */
static ULong lastCheckSpare;                     /* 0x0086574c unreferenced */
static ULong lastCheck;                          /* 0x00865748 last stir */

long gRT3_LicensePaths[kFolderCount] = { -3 };   /* 0x00576258 license folders */
ULong gRT3_LicenseDates[kFolderCount] = { 0 };   /* 0x0057625c their dates */
short gRT3_CodeVersion = kModeWindows;           /* 0x00576260 the open mode */
ULong gSavedWhenFirstRun = 0xffffffff;           /* 0x00576264 earliest first launch */
char *gRT3_IsRegisteredPtr3 = gRT3_License.code; /* 0x00576268 */
ULong gLicenseFileReset = 1;                     /* 0x0057626c date of a missing file */

/* the usage log's registry names, XORed on use: the exe's four copies, in this order */
typedef struct LogName {
    char text[kTextSize];
} LogName;

static const LogName kReadKeyName = { "rKA^ZQAS" };
static const LogName kReadValueName = { "`@QKCSVR" };
static const LogName kWriteKeyName = { "rKA^ZQAS" };
static const LogName kWriteValueName = { "`@QKCSVR" };

/* 0x00576670: a random even-parity nibble for the top of a license code */
static const ULongLong kCodeTags[8] = { 0, 3, 5, 6, 9, 10, 12, 15 };

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

static inline ULong RT3_CalcHashedStamp(void)
{
    long err = 0;
    ULong stamp;

    stamp = TimerGetUTCSeconds() ^ RT3_HashProduct(gPrivateProductName);
    if (stamp == 0)
        stamp = 1;
    return stamp;
}

static inline ULong RT3_CalcLoginHash(void)
{
    long err = 0;
    const char *userName = 0;
    ULong userHash = 0;
    char user[kTextSize] = { 0 };
    ULong userSize = kTextSize;

    if (GetUserNameA(user, &userSize))
        userName = user;
    if (userName != 0) {
        userHash = MemoryChecksum((void *)userName,
                                StringGetLength((const unsigned char *)userName));
        if (userHash == 0)
            userHash = 0xffffffff;
    }
    return userHash;
}

static inline ULong RT3_CalcIdentifier(long folder)
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

static inline ULong RT3_CalcFolderIdentifier(long folder)
{
    long err = 0;
    ULong hash = 0;
    FTFileInfo info;

    hash = RT3_HashProduct(gPrivateProductName);
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

static inline long RT3_LoadResString(unsigned id, char *dst)
{
    long err = 0;
    long result;

    dst[0] = 0;
    if (LoadStringA(GetModuleHandleA(0), id, dst, kTextSize) == 0) {
        err = GetLastError();
        err = kBadArgument;
    }
    result = err;
    return result;
}

/* RT3_GetLicenseCopies's body, written again where RT3_Close expands it */
static inline long RT3_LicenseCopies(void)
{
    long err = 0;
    long result = 0;

    return gRT3_License.copies > 0 ? gRT3_License.copies : 0;
}

long _RT3_WriteLogEntry(RT3LogEntry *entry);
long _RT3_ReadLogEntry(RT3LogEntry *entry);
long _RT3_LicenseSave(void **license, ULong key, ULong when);
long _RT3_LicenseLoad(void ***license, unsigned char *created);
void RT3_LicenseTextToCode(const char *text, ULongLong *code);
ULongLong RT3_CalcMainHash(const char *product, const char *name, long copies);
void RT3_FixLicenseName(char *out, const char *src);
void RT3_InitTables(void);

/* RT3_GetNumLaunches (mac _RT3_GetNumLaunches). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4420
ULong RT3_GetNumLaunches(void)
{
    long err = 0;
    ULong result = 0;

    result = gLogEntry.launches;
    return result;
}

/* RT3_GetHoursUsed (mac _RT3_GetHoursUsed). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4430
ULong RT3_GetHoursUsed(void)
{
    long err = 0;
    ULong result = 0;
    ULong now;

    now = TimerGetElapsedSeconds();
    if (gRT3_StartupTime != 0)
        result = (gLogEntry.secondsUsed + now - gRT3_StartupTime) / kSecondsPerHour;
    else
        result = gLogEntry.secondsUsed / kSecondsPerHour;
    return result;
}

/* RT3_GetDaysHad. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4480
ULong RT3_GetDaysHad(void)
{
    long err = 0;
    ULong result = 0;
    ULong now;

    now = TimerGetUTCSeconds();
    if (now < gLogEntry.firstLaunch)
        result = kElapsedDefault;
    else
        result = (now - gLogEntry.firstLaunch) / kSecondsPerDay + 1;
    return result;
}

/* RT3_IsRegistered. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d44c0
long RT3_IsRegistered(void)
{
    long err = 0;
    long result = 0;

    if (gRT3_License.copies > 0)
        result = 1 << gRT3_License.level;
    return result;
}

/* RT3_GetLicenseCopies (mac _RT3_GetLicenseCopies). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d44e0
long RT3_GetLicenseCopies(void)
{
    long err = 0;
    long result = 0;

    return gRT3_License.copies > 0 ? gRT3_License.copies : 0;
}

/* RT3_GetLicenseName (mac _RT3_GetLicenseName). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4500
void *RT3_GetLicenseName(void)
{
    long err = 0;
    void *result = 0;

    result = gPrivateLicenseName;
    return result;
}

/* RT3_GetLicenseCode (mac _RT3_GetLicenseCode). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4510
long long RT3_GetLicenseCode(void)
{
    long err = 0;
    ULongLong code = 0;

    if (gRT3_License.code[0] != 0) {
        RT3_LicenseTextToCode(gRT3_License.code, &code);
        code = (code ^ gMainHash) | kCodeTags[(ULong)TimerGetMicroseconds() & 7] << 60;
    } else {
        code = 0;
    }
    return code;
}

/* RT3_GetDisplayName (mac _RT3_GetDisplayName). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d45a0
char *RT3_GetDisplayName(void)
{
    long err = 0;
    char *result = 0;

    if (gRT3_License.name[0] != 0)
        result = gRT3_License.name;
    else
        result = gDefaultLicenseName;
    return result;
}

/* RT3_Close (mac _RT3_Close). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d45c0
void RT3_Close(void)
{
    long err = 0;
    long e;

    if (gRT3_Open != 0) {
        if (gLogStuff != 0) {
            e = _RT3_ReadLogEntry(&gLogEntry);
            if (e != 0)
                err = e;
            gLogEntry.launches++;
            gSavedNumLaunches++;
            if (gRT3_StartupTime != 0 && TimerGetElapsedSeconds() > gRT3_StartupTime) {
                gLogEntry.secondsUsed += TimerGetElapsedSeconds() - gRT3_StartupTime;
                gSavedUsageClock += TimerGetElapsedSeconds() - gRT3_StartupTime;
            }
            if (!RT3_LicenseCopies())
                gLogEntry.closeStamp = RT3_CalcHashedStamp();
            e = _RT3_WriteLogEntry(&gLogEntry);
            if (e != 0)
                err = e;
        }
        gRT3_IsRegisteredPtr1 = "";
        if (gRT3_LicenseHdl != 0) {
            IndirectDeallocate(gRT3_LicenseHdl);
            gRT3_LicenseHdl = 0;
        }
        MemoryClear(DAT_00576ce8, 1);
        gRT3_Open = 0;
    }
}

/* RT3_Idle (the entropy stir between RT3_Close and RT3_Open). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d46e0
void RT3_Idle(void)
{
    long err = 0;
    long e;
    ULong elapsed;

    if (gRT3_Open != 0) {
        elapsed = TimerGetTicks() - lastCheck;
        if (lastCheck == 0 || elapsed > TimerGetTickRate() * 2) {
            e = _RT3_LicenseRefresh();
            if (e != 0)
                err = e;
            lastCheck = TimerGetTicks();
        }
        *(ULong *)(DAT_00576ce8 - kStirPointerBackoff) ^= lastCheck >> 8;
    }
}

/* RT3_Open (mac _RT3_Open). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4740
long RT3_Open(unsigned char tracked, short mode, const char *product,
                  const char *company)
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
    if (mode != kModeWindows) {
        err = kBadArgument;
        goto done;
    }
    if (tracked) {
        e = RT3_LoadResString(kStrUnlicensedName, gDefaultLicenseName);
        if (e) {
            err = e;
            goto done;
        }
        e = RT3_LoadResString(kStrUnlicensedCopies, gDefaultLicenseCopies);
        if (e) {
            err = e;
            goto done;
        }
        e = RT3_LoadResString(kStrReserved, gDefaultLicenseCode);
        if (e) {
            err = e;
            goto done;
        }
    }
    gRT3_CodeVersion = mode;
    StringCopy(gDisplayProductName, company);
    StringCopy(gDisplayLicenseCopies, gDefaultLicenseCopies);
    StringCopy(gPrivateProductName, product);
    MemoryClear(gPrivateLicenseName, kTextSize);
    gLogStuff = tracked;
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
    gRT3_Open = 1;
done:
    return err;
}

/* _RT3_WriteLogEntry (mac __RT3_WriteLogEntry). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4900
long _RT3_WriteLogEntry(RT3LogEntry *entry)
{
    long err = 0;
    void *key = 0;
    LogName keyName = kWriteKeyName;
    LogName valueName = kWriteValueName;
    void **log = 0;
    char product[kTextSize];
    ULong size;
    ULong i;
    long e;

    for (i = 0; keyName.text[i] != 0; i++)
        keyName.text[i] ^= i * 3 + 0x21;
    for (i = 0; valueName.text[i] != 0; i++)
        valueName.text[i] ^= i * 3 + 0x21;

    e = RegCreateKeyExA(HKEY_CURRENT_USER, keyName.text, 0, "", 0, KEY_ALL_ACCESS, 0, &key, 0);
    if (e != 0)
        err = e;
    if (err == 0) {
        e = RegQueryValueExA(key, valueName.text, 0, 0, 0, &size);
        if (e != 0)
            err = e;
    }
    if (err == 0) {
        log = IndirectAllocate(size);
        if (log == 0)
            err = kOutOfMemory;
    }
    if (err == 0) {
        e = RegQueryValueExA(key, valueName.text, 0, 0, *log, &size);
        if (e != 0)
            err = e;
    }
    if (err == 0) {
        err = IndirectDecompress(log, kPackTag, 0);
        if (err != 0 && err != kBadArgument)
            goto done;
    }

    if (err == 0) {
        size = IndirectGetSize(log) / sizeof(RT3LogEntry);
        for (i = 0; i < size; i++) {
            StringCopySafe(product, ((RT3LogEntry *)*log)[i].product, kTextSize);
            if (StringCompare(gPrivateProductName, product) == 0)
                break;
        }
        if (i == size) {
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
    if (err == 0) {
        e = RegSetValueExA(key, valueName.text, 0, REG_BINARY, *log, IndirectGetSize(log));
        if (e != 0)
            err = e;
    }
    IndirectDeallocate(log);
    log = 0;
done:
    if (log != 0)
        IndirectDeallocate(log);
    if (key != 0) {
        RegFlushKey(key);
        RegCloseKey(key);
    }
    return err;
}

/* _RT3_ReadLogEntry (mac __RT3_ReadLogEntry). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4ba0
long _RT3_ReadLogEntry(RT3LogEntry *entry)
{
    long err = 0;
    void *key = 0;
    LogName keyName = kReadKeyName;
    LogName valueName = kReadValueName;
    unsigned char tampered = 0;
    unsigned char reset = 0;
    void **log = 0;
    char product[kTextSize];
    ULong size;
    ULong i;
    long e;

    for (i = 0; keyName.text[i] != 0; i++)
        keyName.text[i] ^= i * 3 + 0x21;
    for (i = 0; valueName.text[i] != 0; i++)
        valueName.text[i] ^= i * 3 + 0x21;

    e = RegOpenKeyExA(HKEY_CURRENT_USER, keyName.text, 0, KEY_READ, &key);
    if (e != 0)
        err = e;
    if (err == 0) {
        e = RegQueryValueExA(key, valueName.text, 0, 0, 0, &size);
        if (e != 0)
            err = e;
    }
    if (err == 0) {
        log = IndirectAllocate(size);
        if (log == 0)
            err = kOutOfMemory;
    }
    if (err == 0) {
        e = RegQueryValueExA(key, valueName.text, 0, 0, *log, &size);
        if (e != 0)
            err = e;
    }
    if (err == 0) {
        err = IndirectDecompress(log, kPackTag, 0);
        if (err != 0 && err != kBadArgument)
            goto done;
    }

    if (err == 0) {
        size = IndirectGetSize(log) / sizeof(RT3LogEntry);
        for (i = 0; i < size; i++) {
            StringCopySafe(product, ((RT3LogEntry *)*log)[i].product, kTextSize);
            if (StringCompare(gPrivateProductName, product) == 0)
                break;
        }
        if (i == size)
            reset = 1;
        else
            *entry = ((RT3LogEntry *)*log)[i];
        IndirectDeallocate(log);
        log = 0;
    } else if (err == kFTErrPermission) {
        err = 0;
        tampered = 1;
    } else {
        err = 0;
        reset = 1;
    }

    if (reset) {
        StringCopySafe(entry->product, gPrivateProductName, kTextSize);
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
        if (e != 0)
            err = e;
    } else if (tampered) {
        StringCopySafe(entry->product, gPrivateProductName, kTextSize);
        entry->firstLaunch = TimerGetUTCSeconds() - kTamperedAge;
        entry->secondsUsed = kTamperedSeconds;
        entry->launches = kTamperedLaunches;
        entry->closeStamp = 0;
        entry->saveStamp = 0;
        entry->reserved[0] = 0;
        entry->reserved[1] = 0;
    } else {
        if (gSavedWhenFirstRun > entry->firstLaunch)
            gSavedWhenFirstRun = entry->firstLaunch;
        if (gSavedNumLaunches < entry->launches)
            gSavedNumLaunches = entry->launches;
        if (gSavedUsageClock < entry->secondsUsed)
            gSavedUsageClock = entry->secondsUsed;
    }
done:
    if (key != 0)
        RegCloseKey(key);
    if (log != 0)
        IndirectDeallocate(log);
    return err;
}

/* Stand-in: the linker strips this; the exe keeps its literals (body not recoverable). */
void RT3_PathJoinStub(char *path)
{
    StringCopy(path, "\\");
}

/* _RT3_LicenseRefresh (mac __RT3_LicenseRefresh). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d4f70
long _RT3_LicenseRefresh(void)
{
    long err;
    long e;
    ULong dates[kFolderCount];
    unsigned char changed;
    unsigned char created;
    FTFileInfo info;
    char fileName[kTextSize];
    ULong i;

    err = 0;
    dates[0] = gLicenseFileReset;
    changed = 0;
    created = 0;
    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe((unsigned char *)fileName, (const unsigned char *)" License.lcs", kTextSize);
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
        e = _RT3_LicenseLoad(&gRT3_LicenseHdl, &created);
        if (e != 0) {
            err = e;
            goto done;
        }
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
        *DAT_00576ce8 = gRT3_License.copies > 0 ? 1 : 0;
        gMainHash = RT3_CalcMainHash(gPrivateProductName, gPrivateLicenseName, gRT3_License.copies);
        if (created) {
            err = _RT3_LicenseSave(gRT3_LicenseHdl, 0, 0);
            if (err != 0)
                e = err;
            err = 0;
        }
    }
done:
    return err;
}

/* _RT3_LicenseSave (mac __RT3_LicenseSave). STATE: MATCH 100%, exe 218 / obj 218 instrs,
   byte-identical yes. OPEN: none; the `const void *p' walked as `(const char *)p + 1' is
   the matching spelling, not necessarily the author's (Research/notes/rt3.md). */
// FUNCTION: EVNOVA 0x004d5180
long _RT3_LicenseSave(void **license, ULong key, ULong when)
{
    long err = 0;
    long e;
    unsigned char ok = 0;
    ULong hash = 0;
    long folder = -3;
    void **copy = 0;
    unsigned char licensed;
    char fileName[kTextSize];
    long skew;
    ULong k;
    ULong s;
    ULong t;
    ULong stamp;

    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe((unsigned char *)fileName, (const unsigned char *)" License.lcs", kTextSize);
    if (((RT3License *)*license)->copies != 0)
        licensed = 1;
    else
        licensed = 0;
    ok = licensed;
    copy = IndirectDuplicate(license);
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }

    if (key != 0) {
        unsigned char c;
        const void *p;

        t = 0;
        p = gPrivateProductName;
        k = 0;
        while ((c = *(const unsigned char *)p) != 0) {
            s = (k << 2) ^ (k >> 29) ^ c;
            k = s;
            p = (const char *)p + 1;
        }
        t = k;
        ((RT3License *)*copy)->machine = key ^ k;
    } else {
        ((RT3License *)*copy)->machine = hash = RT3_CalcIdentifier(folder);
    }

    if (key != 0)
        ((RT3License *)*copy)->user = 0;
    else
        ((RT3License *)*copy)->user = RT3_CalcLoginHash();

    e = IndirectCompress(copy, kPackTag, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = FT_FileSave(folder, fileName, 1, kLicenseType, kLicenseCreator, copy);
    if (e != 0) {
        err = e;
        goto done;
    }
    gRT3_LicenseDates[0] = 0;

    e = _RT3_ReadLogEntry(&gLogEntry);
    if (e != 0)
        err = e;
    skew = when - TimerGetUTCSeconds();
    if (skew >= -kSecondsPerDay && skew < kSecondsPerDay) {
        gLogEntry.saveStamp = 0;
        gLogEntry.closeStamp = gLogEntry.saveStamp;
    } else {
        if (licensed) {
            unsigned char c;
            const unsigned char *p;

            t = 0;
            hash = 0;
            p = (const unsigned char *)gPrivateProductName;
            s = 0;
            while ((c = *p) != 0) {
                s = (s << 2) ^ (s >> 29) ^ c;
                p++;
            }
            t = s;
            stamp = TimerGetUTCSeconds() ^ s;
            if (stamp == 0)
                stamp = 1;
        } else {
            stamp = 0;
        }
        gLogEntry.saveStamp = stamp;
    }
    e = _RT3_WriteLogEntry(&gLogEntry);
    if (e != 0)
        err = e;
    if (err != 0)
        err = 0;
done:
    if (copy != 0)
        IndirectDeallocate(copy);
    return err;
}

/* _RT3_LicenseLoad (mac __RT3_LicenseLoad). STATE: MATCH 100%, exe 378 / obj 378 instrs,
   byte-identical yes. OPEN: the identifier is RT3_CalcFolderIdentifier here and the committed
   RT3_CalcIdentifier in _RT3_LicenseSave (one helper for both is not found; Research/notes/rt3.md) */
// FUNCTION: EVNOVA 0x004d5460
long _RT3_LicenseLoad(void ***license, unsigned char *created)
{
    long err;
    void **h;
    unsigned char valid;
    ULong machine;
    ULong productHash;
    ULong next;
    long bestFolder;
    void **best;
    ULong t;
    const unsigned char *p;
    unsigned char c;
    ULong hash;
    long folder;
    ULong u;
    ULong pm;
    long e;
    char fileName[kTextSize];
    ULong i;

    err = 0;
    h = 0;
    valid = 0;
    machine = 0;
    best = 0;
    bestFolder = 0;
    *license = 0;
    *created = 0;
    t = 0;
    p = (const unsigned char *)gPrivateProductName;
    productHash = 0;
    while ((c = *p) != 0) {
        next = productHash >> 29;
        next ^= productHash <<= 2;
        next ^= c;
        productHash = next;
        p++;
    }
    t = productHash;
    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe((unsigned char *)fileName, (const unsigned char *)" License.lcs", kTextSize);

    for (i = 0; i < kFolderCount; i++) {
        err = 0;
        valid = 1;
        err = FT_FileGetFlags(gRT3_LicensePaths[i], fileName, 0, 0);
        if (err == 0)
            err = FT_FileLoad(gRT3_LicensePaths[i], fileName, kLicenseMaxSize, 0, &h);
        if (err == kOutOfMemory)
            goto done;
        if (err == 0) {
            err = IndirectDecompress(h, kPackTag, 0);
            if (err != 0 && err != kBadArgument)
                goto done;
        }
        if (err != 0 || h == 0) {
            valid = 0;
            goto checked;
        }
        if (IndirectGetSize(h) >= sizeof(RT3License) && ((RT3License *)*h)->machine != 0) {
            machine = ((RT3License *)*h)->machine;
            hash = RT3_CalcFolderIdentifier(gRT3_LicensePaths[i]);
            pm = productHash ^ kPortableMachine;
            if (machine != pm && machine != hash)
                valid = 0;
            if (!valid || ((RT3License *)*h)->user == 0)
                goto checked;
            u = RT3_CalcLoginHash();
            if (u == 0 || u == ((RT3License *)*h)->user)
                goto checked;
        }
        valid = 0;
    checked:
        if (valid) {
            if (best == 0
                || (((RT3License *)*best)->copies == 0 && ((RT3License *)*h)->copies != 0)
                || ((RT3License *)*best)->level < ((RT3License *)*h)->level) {
                if (best != 0)
                    IndirectDeallocate(best);
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

    if (gLogStuff != 0 && valid && ((RT3License *)*h)->copies != 0) {
        ULong closeStamp = 0;
        ULong saveStamp = 0;
        long sinceClose = 0;
        long sinceSave = 0;
        unsigned char renew = 0;
        ULong now;

        e = _RT3_ReadLogEntry(&gLogEntry);
        if (e != 0)
            err = e;
        closeStamp = gLogEntry.closeStamp;
        saveStamp = gLogEntry.saveStamp;
        now = TimerGetUTCSeconds();
        if (closeStamp != 0)
            closeStamp ^= productHash;
        if (saveStamp != 0)
            saveStamp ^= productHash;
        sinceClose = now - closeStamp;
        sinceSave = now - saveStamp;
        if (closeStamp != 0 && saveStamp != 0) {
            if (sinceClose >= 0 && sinceClose <= kLogGrace
                && (saveStamp + kSecondsPerDay < closeStamp || saveStamp > now))
                valid = 0;
            else if (sinceClose > kLogGrace)
                renew = 1;
            else
                renew = 0;
        } else if (closeStamp == 0 && saveStamp != 0) {
            if (sinceSave >= kLogStale)
                valid = 0;
            else if (sinceSave > 0)
                renew = 1;
            else
                renew = 0;
        }
        if (valid && renew) {
            gLogEntry.closeStamp = 0;
            gLogEntry.saveStamp = 0;
            e = _RT3_WriteLogEntry(&gLogEntry);
            if (e != 0)
                err = e;
        }
    }

    if (!valid) {
        if (h == 0) {
            h = IndirectAllocate(sizeof(RT3License));
            if (h == 0) {
                err = kOutOfMemory;
                goto done;
            }
        } else {
            e = IndirectSetSize(h, sizeof(RT3License));
            if (e != 0) {
                err = e;
                goto done;
            }
        }
        MemoryClear(*h, sizeof(RT3License));
        ((RT3License *)*h)->copies = 0;
        ((RT3License *)*h)->level = 0;
        ((RT3License *)*h)->user = 0;
        *created = 1;
    }
done:
    if (err != 0 && h != 0) {
        IndirectDeallocate(h);
        h = 0;
    }
    if (h != 0) {
        *license = h;
        IndirectSetLock(h, 1);
        gRT3_IsRegisteredPtr1 = *h;
    }
    return err;
}

/* Four nibbles of a license code, as a 16-bit check value. */
/* DRHash64. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d5990
unsigned long DRHash64(unsigned long long lic)
{
    ULong r;

    r = 0;
    r |= ((ULong)(lic >> 8) & 0xf) << 4;
    r |= (ULong)(lic >> 22) & 0xf;
    r |= ((ULong)(lic >> 36) & 0xf) << 12;
    r |= ((ULong)(lic >> 50) & 0xf) << 8;
    return r;
}

/* DRHash64's twin: four nibbles, bits 13, 27, 41 and 55 the low ends, in another order. */
/* RT3_ExtractLicenseBlock1. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d5a00
unsigned short RT3_ExtractLicenseBlock1(ULongLong v)
{
    ULong r;

    r = 0;
    r |= ((ULong)(v >> 13) & kNibble) << 8;
    r |= ((ULong)(v >> 27) & kNibble) << 4;
    r |= (ULong)(v >> 41) & kNibble;
    r |= ((ULong)(v >> 55) & kNibble) << 12;
    return (unsigned short)r;
}

/* RT3_LicenseTextToCode (mac _RT3_LicenseTextToCode). STATE: MATCH, byte-identical (exe 49 / obj 49). OPEN: none */
// FUNCTION: EVNOVA 0x004d5a70
void RT3_LicenseTextToCode(const char *text, ULongLong *code)
{
    ULongLong value;
    ULong digits;
    unsigned char d;

    value = 0;
    RT3_InitTables();
    if (text != 0 && code != 0) {
        for (digits = 0; digits < kCodeDigits && *text != 0; text++) {
            d = gRT3_EightBitToFiveBitChars[*(const unsigned char *)text];
            if (d != kNoDigit) {
                value = value << kCodeBits | d;
                digits++;
            }
        }
    }
    *code = value;
}

/* RT3_CalcMainHash (mac _RT3_CalcMainHash). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d5b10
ULongLong RT3_CalcMainHash(const char *product, const char *name, long copies)
{
    ULongLong h;
    ULong i, j;
    char c, d;

    h = 0;
    for (i = 0; (c = *product) != 0; product++, i++) {
        h = h >> 60 | h << 4;
        h ^= (long long)c;
        h = h >> 63 | h << 1;
        h ^= (ULong)((i + 7) * copies);
    }
    for (j = 0; (d = *name) != 0; name++, j++) {
        h = h >> 61 | h << 3;
        h ^= (long long)d;
        h = h >> 63 | h << 1;
        h ^= (ULong)((j + 13) * copies * 3);
    }
    if (!h)
        h = 1;
    return h & 0x0fffffffffffffffULL;
}

/* Copies a name as upper-case letters only, at most 255 of them. */
/* RT3_FixLicenseName (mac _RT3_FixLicenseName). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d5c80
void RT3_FixLicenseName(char *out, const char *src)
{
    char *start;
    ULong n;
    char c;

    start = out;
    while ((n = out - start) < kTextSize - 1 && (*out = *src++) != 0) {
        c = *out;
        if (c >= 'a' && c <= 'z') {
            *out += 'A' - 'a';
            out++;
        } else if (c >= 'A' && c <= 'Z') {
            out++;
        }
    }
    if (n == kTextSize - 1)
        *out = 0;
}

static const char kCodeDigitSet[32] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

/* RT3_InitTables (mac _RT3_InitTables). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d5cd0
void RT3_InitTables(void)
{
    ULong c;
    ULong i;

    if (gInited == 0) {
        for (c = 0; c < 0x100; c++) {
            gRT3_EightBitToFiveBitChars[c] = kNoDigit;
            for (i = 0; i < 0x20; i++) {
                if (c == kCodeDigitSet[i])
                    gRT3_EightBitToFiveBitChars[c] = i;
            }
        }
        for (c = 'a'; c <= 'z'; c++)
            gRT3_EightBitToFiveBitChars[c] = gRT3_EightBitToFiveBitChars[c - ('a' - 'A')];
        gInited = 1;
    }
}
