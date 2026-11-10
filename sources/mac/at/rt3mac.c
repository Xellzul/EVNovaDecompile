/* rt3mac.c -- 1.0.10: RT3's reg tool on the Mac: the license and the usage log in files, its strings in resources. Notes: Research/notes/maclibs.md */
#define _RT3_LicenseRefresh RT3_HeaderLicenseRefresh
#define DAT_00576ce8 gStirPointer
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#undef _RT3_LicenseRefresh

#define kModeClassic        3
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
#define kPortableMachine    0xa5a5a5a5
#define kPackTag            'zlib'
#define kLicenseType        'Alic'
#define kLicenseCreator     'Areg'
#define kLogType            '????'
#define kLogCreator         '????'
#define kLicenseMaxSize     0x5000
#define kLogMaxSize         0x32000
#define kPreferences        -3
#if TARGET_API_MAC_CARBON
#define kFolderCount        2
#define kSystemFolder       -7
#else
#define kFolderCount        1
#endif

/* the log's file name, ".DS_Store ", XORed on use */
#define kLogName "T3'.=\037\007\027\007\177"

/* days the trial is taken to have run when the clock is before the first launch */
#define kElapsedDefault 90

/* FTFileInfo.flags */
#define kFlagReadable 0
#define kFlagWritable 1
#define kFlagHidden   3

extern long FT_FileSetFlags(long folder, const char *name, struct FTFileInfo *info);
extern ULongLong gMainHash;
extern void RT3_LicenseTextToCode(const char *text, ULongLong *code);
extern ULongLong RT3_CalcMainHash(const char *product, const char *name, long copies);
extern void RT3_FixLicenseName(char *out, const char *src);

RT3License gRT3_License;
char gDisplayProductName[kTextSize];
char gDisplayLicenseCopies[kTextSize];
char gPrivateProductName[kTextSize];
char gPrivateLicenseName[kTextSize];
char gDefaultLicenseName[kTextSize];
char gDefaultLicenseCode[kTextSize];
char gDefaultLicenseCopies[kTextSize];
RT3LogEntry gLogEntry;
unsigned char gLogStuff;
ULongLong gMainHash;
ULong gSavedUsageClock;
ULong gSavedNumLaunches;
ULong gRT3_StartupTime;
void **gRT3_LicenseHdl;
unsigned char gRT3_Open;

static ULong lastCheck;
static ULong gLicenseFileReset = 1;

#if TARGET_API_MAC_CARBON
ULong gRT3_LicensePaths[kFolderCount] = { kPreferences, kSystemFolder };
ULong gRT3_LicenseDates[kFolderCount] = { 0, 0 };
#else
ULong gRT3_LicensePaths[kFolderCount] = { kPreferences };
ULong gRT3_LicenseDates[kFolderCount] = { 0 };
#endif
short gRT3_CodeVersion = kModeClassic;
ULong gSavedWhenFirstRun = 0xffffffff;


/* a random even-parity nibble for the top of a license code */
static ULongLong kCodeTags[8] = { 0, 3, 5, 6, 9, 10, 12, 15 };

static long _RT3_WriteLogEntry(RT3LogEntry *entry);
static long _RT3_ReadLogEntry(RT3LogEntry *entry);
#if TARGET_API_MAC_CARBON
static ULong _RT3_CalcLoginHash(void);
extern unsigned char RT3_CanSetSystemLicense(void);
extern long WriteToFileAsSuperUser(char *path, const void *data, long size);
extern char *GetPathToSystemPreferences(const char *name);
extern unsigned char _PlatformIsThreaded(void);
extern void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName);
#endif

static inline ULong RT3_HashProduct(const char *name)
{
    ULong hash = 0;
    const unsigned char *p = (const unsigned char *)name;
    ULong c;

    while ((c = *p) != 0) {
        hash = (hash << 2) ^ (hash >> 29) ^ c;
        p++;
    }
    return hash;
}

static inline ULong RT3_CalcHashedStamp(void)
{
    ULong stamp = TimerGetUTCSeconds() ^ RT3_HashProduct(gPrivateProductName);

    if (stamp == 0)
        stamp = 1;
    return stamp;
}

static inline ULong RT3_CalcIdentifier(ULong folder)
{
    ULong hash = RT3_HashProduct(gPrivateProductName);
    FTFileInfo info;

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

static inline long RT3_LoadResString(short id, char *dst)
{
    long err = 0;
    Handle h;

    dst[0] = 0;
    h = GetResource('STR ', id);
    if (h == 0) {
        ResError();
        err = kBadArgument;
    } else {
        StringP2CCopy(dst, *h);
    }
    if (h != 0)
        ReleaseResource(h);
    return err;
}

static inline ULong RT3_LicenseCopies(void)
{
    return gRT3_License.copies > 0 ? gRT3_License.copies : 0;
}

static long _RT3_LicenseLoad(void ***license, unsigned char *created)
{
    ULong pm;
    ULong hash;
    void **h;
    long err;
    long e;
    char fileName[kTextSize];
    ULong machine;
    unsigned char valid;
    void **best;
    ULong productHash;
    unsigned char ok;
#if TARGET_API_MAC_CARBON
    ULong bestFolder;
    ULong folder = 0;
#endif
    ULong i;

    err = 0;
    h = 0;
    best = 0;
#if TARGET_API_MAC_CARBON
    bestFolder = 0;
#endif
    *license = 0;
    *created = 0;
    productHash = RT3_HashProduct(gPrivateProductName);
    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe((unsigned char *)fileName, (const unsigned char *)" License", kTextSize);

    for (i = 0; i < kFolderCount; i++) {
        pm = productHash ^ kPortableMachine;
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
        if (IndirectGetSize(h) < sizeof(RT3License)) {
            valid = 0;
            goto checked;
        }
        machine = ((RT3License *)*h)->machine;
        if (machine == 0) {
            valid = 0;
            goto checked;
        }
        hash = RT3_CalcIdentifier(gRT3_LicensePaths[i]);
#if TARGET_API_MAC_CARBON
        if (machine != pm && machine != hash) {
            long diff = (machine ^ productHash) - (hash ^ productHash);

            if (diff < 0)
                diff *= -1;
            if (diff > kSecondsPerDay || (diff % 15) * 60 != 0)
                valid = 0;
        }
        if (valid && gRT3_LicensePaths[i] != kSystemFolder && ((RT3License *)*h)->user != 0) {
            ULong u = _RT3_CalcLoginHash();

            if (u != 0 && u != ((RT3License *)*h)->user)
                valid = 0;
        }
#else
        if (machine != pm && machine != hash)
            valid = 0;
#endif
    checked:
        if (valid) {
            if (best == 0
                || (((RT3License *)*best)->copies == 0 && ((RT3License *)*h)->copies != 0)
                || ((RT3License *)*best)->level < ((RT3License *)*h)->level) {
                if (best != 0)
                    IndirectDeallocate(best);
#if TARGET_API_MAC_CARBON
                bestFolder = gRT3_LicensePaths[i];
#endif
                best = h;
                h = 0;
            }
        }
    }

    err = 0;
    if (best != 0) {
        h = best;
#if TARGET_API_MAC_CARBON
        folder = bestFolder;
#endif
        ok = 1;
    } else {
        ok = 0;
    }

    if (gLogStuff != 0 && ok && ((RT3License *)*h)->copies != 0) {
        ULong closeStamp;
        ULong saveStamp;
        long sinceClose;
        long sinceSave;
        unsigned char renew = 0;
        ULong now;

        RT3LogEntry *log = &gLogEntry;

        e = _RT3_ReadLogEntry(log);
        if (e != 0)
            err = e;
        closeStamp = log->closeStamp;
        saveStamp = log->saveStamp;
        now = TimerGetUTCSeconds();
        if (closeStamp != 0)
            closeStamp ^= productHash;
        if (saveStamp != 0)
            saveStamp ^= productHash;
        sinceClose = now - closeStamp;
        sinceSave = now - saveStamp;
#if TARGET_API_MAC_CARBON
        if (folder == kSystemFolder && (closeStamp != 0 || saveStamp != 0)) {
            renew = 1;
        } else
#endif
        if (closeStamp != 0 && saveStamp != 0) {
            if (sinceClose >= 0 && sinceClose <= kLogGrace
                && (saveStamp + kSecondsPerDay < closeStamp || saveStamp > now))
                ok = 0;
            else
                renew = AsByte(sinceClose > kLogGrace);
        } else if (closeStamp == 0 && saveStamp != 0) {
            if (sinceSave >= kLogStale)
                ok = 0;
            else
                renew = AsByte(sinceSave > 0);
        }
        if (ok && renew) {
            gLogEntry.closeStamp = 0;
            gLogEntry.saveStamp = 0;
            e = _RT3_WriteLogEntry(&gLogEntry);
            if (e != 0)
                err = e;
        }
    }

    if (!ok) {
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

#if TARGET_API_MAC_CARBON
static long _RT3_LicenseSave(void **license, ULong key, ULong when, unsigned char systemWide)
#else
static long _RT3_LicenseSave(void **license, ULong key, ULong when)
#endif
{
    ULong licensed;
    long e;
    long err = 0;
    long folder = kPreferences;
    void **copy;
    char fileName[kTextSize];
    long skew;
    ULong stamp;

    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe((unsigned char *)fileName, (const unsigned char *)" License", kTextSize);
    licensed = ((RT3License *)*license)->copies != 0;
#if TARGET_API_MAC_CARBON
    if (systemWide)
        folder = kSystemFolder;
#endif
    copy = IndirectDuplicate(license);
    if (copy == 0) {
        err = kOutOfMemory;
        goto done;
    }

    if (key != 0)
        ((RT3License *)*copy)->machine = key ^ RT3_HashProduct(gPrivateProductName);
    else
        ((RT3License *)*copy)->machine = RT3_CalcIdentifier(folder);

    if (key != 0)
        ((RT3License *)*copy)->user = 0;
#if TARGET_API_MAC_CARBON
    else if (systemWide)
        ((RT3License *)*copy)->user = 0;
    else
        ((RT3License *)*copy)->user = _RT3_CalcLoginHash();
#else
    else
        ((RT3License *)*copy)->user = 0;
#endif

    e = IndirectCompress(copy, kPackTag, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
#if TARGET_API_MAC_CARBON
    if (systemWide && !RT3_CanSetSystemLicense()) {
        char *path;

        IndirectSetLock(copy, 1);
        path = GetPathToSystemPreferences(fileName);
        if (path == 0) {
            err = kOutOfMemory;
            goto done;
        }
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
#else
    e = FT_FileSave(folder, fileName, 1, kLicenseType, kLicenseCreator, copy);
    if (e != 0) {
        err = e;
        goto done;
    }
#endif
    gRT3_LicenseDates[0] = 0;

    e = _RT3_ReadLogEntry(&gLogEntry);
    if (e != 0)
        err = e;
    skew = when - TimerGetUTCSeconds();
#if TARGET_API_MAC_CARBON
    if (systemWide) {
        gLogEntry.saveStamp = 0;
        gLogEntry.closeStamp = 0;
    } else
#endif
    if (skew >= -kSecondsPerDay && skew < kSecondsPerDay) {
        gLogEntry.saveStamp = 0;
        gLogEntry.closeStamp = 0;
    } else {
        stamp = licensed ? RT3_CalcHashedStamp() : 0;
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

static long _RT3_LicenseRefresh(void)
{
    long err;
    long e;
#if TARGET_API_MAC_CARBON
    ULong dates[kFolderCount] = { 1, 1 };
#else
    ULong dates[kFolderCount];
#endif
    unsigned char changed;
    unsigned char created;
    FTFileInfo info;
    char fileName[kTextSize];
    ULong i;
    RT3License *license;

    err = 0;
#if !TARGET_API_MAC_CARBON
    dates[0] = gLicenseFileReset;
#endif
    changed = 0;
    created = 0;
    StringCopySafe(fileName, gPrivateProductName, kTextSize);
    StringAppendSafe((unsigned char *)fileName, (const unsigned char *)" License", kTextSize);
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
        license = &gRT3_License;
        if (gRT3_License.copies > 0)
            StringFromNumber(gDisplayLicenseCopies, gRT3_License.copies);
        else
            StringCopy(gDisplayLicenseCopies, gDefaultLicenseCopies);
        *DAT_00576ce8 = license->copies > 0;
        gMainHash = RT3_CalcMainHash(gPrivateProductName, gPrivateLicenseName, license->copies);
        if (created) {
#if TARGET_API_MAC_CARBON
            _RT3_LicenseSave(gRT3_LicenseHdl, 0, 0, 0);
#else
            _RT3_LicenseSave(gRT3_LicenseHdl, 0, 0);
#endif
            err = 0;
        }
    }
done:
    return err;
}

#if TARGET_API_MAC_CARBON
/* the login's hash, through System.framework: Mac OS X only */
static ULong _RT3_CalcLoginHash(void)
{
    typedef unsigned long (*GetUidProc)(void);
    typedef char **(*GetPwUidProc)(unsigned long);
    char *name;
    ULong hash;
    GetUidProc getUid;
    GetPwUidProc getPwUid;
    char **pw;

    name = 0;
    hash = 0;
    if (_PlatformIsThreaded()) {
        getUid = _PlatformFindBundleSymbol(CFSTR("System.framework"), CFSTR("getuid"));
        getPwUid = _PlatformFindBundleSymbol(CFSTR("System.framework"), CFSTR("getpwuid"));
        if (getUid != 0 && getPwUid != 0) {
            pw = getPwUid(getUid());
            if (pw != 0)
                name = *pw;
        }
    }
    if (name != 0) {
        hash = MemoryChecksum(name, StringGetLength((const unsigned char *)name));
        if (hash == 0)
            hash = 0xffffffff;
    }
    return hash;
}
#endif

static long _RT3_ReadLogEntry(RT3LogEntry *entry)
{
    long err;
    void **log;
    unsigned char tampered;
    unsigned char reset;
    ULong i;
    ULong size;
    long e;
    FTFileInfo info;
    char name[kTextSize] = kLogName;
    char product[kTextSize];

    log = 0;
    tampered = 0;
    reset = 0;
    for (i = 0; name[i] != 0; i++)
        name[i] ^= 0x7a - i * 3;

    err = FT_FileLoad(kPreferences, name, kLogMaxSize, 0, &log);
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
        if (FT_FileGetFlags(kPreferences, name, &info, 0) == 0 && !reset && info.flags[kFlagHidden] == 0) {
            info.flags[kFlagReadable] = 1;
            info.flags[kFlagWritable] = 1;
            info.flags[kFlagHidden] = 1;
            FT_FileSetFlags(kPreferences, name, &info);
        }
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
    if (log != 0)
        IndirectDeallocate(log);
    return err;
}

static long _RT3_WriteLogEntry(RT3LogEntry *entry)
{
    long err;
    void **log;
    ULong i;
    ULong size;
    long e;
    FTFileInfo info;
    char name[kTextSize] = kLogName;
    char product[kTextSize];

    log = 0;
    for (i = 0; name[i] != 0; i++)
        name[i] ^= 0x7a - i * 3;

    err = FT_FileLoad(kPreferences, name, kLogMaxSize, 0, &log);
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
    e = FT_FileSave(kPreferences, name, 1, kLogType, kLogCreator, log);
    if (e != 0) {
        err = e;
        goto done;
    }
    IndirectDeallocate(log);
    log = 0;
    if (FT_FileGetFlags(kPreferences, name, &info, 0) == 0 && info.flags[kFlagHidden] == 0) {
        info.flags[kFlagReadable] = 1;
        info.flags[kFlagWritable] = 1;
        info.flags[kFlagHidden] = 1;
        FT_FileSetFlags(kPreferences, name, &info);
    }
done:
    if (log != 0)
        IndirectDeallocate(log);
    return err;
}

long RT3_Open(unsigned char tracked, unsigned short mode, const char *product,
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
    if (mode != kModeClassic) {
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
        *(ULong *)(DAT_00576ce8 - kStirPointerBackoff) ^= lastCheck << 8;
    }
}

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
            if (RT3_LicenseCopies() == 0)
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

long RT3_IncNumLaunches(void)
{
    long err = 0;

    _RT3_ReadLogEntry(&gLogEntry);
    gSavedNumLaunches++;
    gLogEntry.launches++;
    err = _RT3_WriteLogEntry(&gLogEntry);
    return err;
}

void RT3_EnableCrackDetection(unsigned char enable)
{
    long err = 0;

    if (enable) {
        gRT3_IsRegisteredPtr1 = "";
        MemoryClear(&gRT3_License, sizeof(RT3License));
        MemoryClear(gRT3_LicenseDates, sizeof(gRT3_LicenseDates));
        MemoryClear(gPrivateLicenseName, kTextSize);
    } else {
        err = _RT3_LicenseRefresh();
        if (err != 0)
            goto done;
    }
done:
    ;
}

char *RT3_GetDisplayCode(void)
{
    if (gRT3_License.code[0] != 0)
        return gRT3_License.code;
    return gDefaultLicenseCode;
}

char *RT3_GetDisplayName(void)
{
    if (gRT3_License.name[0] != 0)
        return gRT3_License.name;
    return gDefaultLicenseName;
}

char *RT3_GetDisplayCopies(void)
{
    char *result = 0;

    if (gRT3_License.copies > 0)
        result = StringFromNumber(gDisplayLicenseCopies, gRT3_License.copies);
    else
        result = StringCopy(gDisplayLicenseCopies, gDefaultLicenseCopies);
    return result;
}

long long RT3_GetLicenseCode(void)
{
    ULongLong code = 0;
    ULongLong now;

    if (gRT3_License.code[0] != 0) {
        RT3_LicenseTextToCode(gRT3_License.code, &code);
        now = TimerGetMicroseconds();
        code = (code ^ gMainHash) | kCodeTags[now & 7] << 60;
    } else {
        code = 0;
    }
    return code;
}

void *RT3_GetLicenseName(void)
{
    return gPrivateLicenseName;
}

long RT3_GetLicenseCopies(void)
{
    long copies;

    if (gRT3_License.copies > 0)
        copies = gRT3_License.copies;
    else
        copies = 0;
    return copies;
}

ULong RT3_IsRegistered(void)
{
    long result = 0;

    if (gRT3_License.copies > 0)
        result = 1 << gRT3_License.level;
    return result;
}

ULong RT3_GetDaysHad(void)
{
    ULong result = 0;
    ULong now;

    now = TimerGetUTCSeconds();
    if (now < gLogEntry.firstLaunch)
        result = kElapsedDefault;
    else
        result = (now - gLogEntry.firstLaunch) / kSecondsPerDay + 1;
    return result;
}

ULong RT3_GetHoursUsed(void)
{
    ULong result = 0;
    ULong now;

    now = TimerGetElapsedSeconds();
    if (gRT3_StartupTime != 0)
        result = (gLogEntry.secondsUsed + now - gRT3_StartupTime) / kSecondsPerHour;
    else
        result = gLogEntry.secondsUsed / kSecondsPerHour;
    return result;
}

ULong RT3_GetNumLaunches(void)
{
    return gLogEntry.launches;
}

ULong _RT3_CalcHashedStamp(void)
{
    return RT3_CalcHashedStamp();
}

unsigned char RT3_IsOpen(void)
{
    return gRT3_Open;
}
