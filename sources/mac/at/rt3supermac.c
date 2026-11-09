/* rt3supermac.c -- 1.0.10: RT3's system-wide license on Carbon: writing it as the superuser through Authorization Services and /usr/bin/tee, found by name since CarbonLib has neither. Notes: Research/notes/maclibs.md */
#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "burgerplat.h"
#if TARGET_API_MAC_CARBON
#include <CFBundle.h>
#include <CFString.h>
#include <CFURL.h>
#include <Files.h>
#include <Folders.h>
#include <MacErrors.h>

#define kNoSymbolFound      -7004
#define kUserCanceled       -7007
#define kAuthCanceled       -60006
#define kAuthRightExecute   "system.privilege.admin"
#define kAuthAllowInteraction 1
#define kAuthExtendRights   2
#define kAuthPreAuthorize   0x10
#define kMaxUtf8Growth      5
#define kUtf8Encoding       0x08000100
#define kFrameworksFolderType 'fram'

typedef struct AuthItem {
    const char *name;
    ULong valueLength;
    void *value;
    ULong flags;
} AuthItem;

typedef struct AuthRights {
    ULong count;
    AuthItem *items;
} AuthRights;

typedef long (*AuthCreateProc)(const AuthRights *rights, const void *environment, ULong flags, void **authRef);
typedef long (*AuthCopyRightsProc)(void *authRef, const AuthRights *rights, const void *environment, ULong flags, AuthRights **authorized);
typedef long (*AuthExecuteProc)(void *authRef, const char *tool, ULong options, char *const *args, void **pipe);
typedef long (*AuthFreeProc)(void *authRef, ULong flags);
typedef ULong (*FwriteProc)(const void *data, ULong size, ULong count, void *file);
typedef long (*FcloseProc)(void *file);
typedef long (*GetEuidProc)(void);

extern unsigned char _PlatformIsThreaded(void);

static AuthCreateProc sAuthCreate = 0;
static AuthCopyRightsProc sAuthCopyRights = 0;
static AuthExecuteProc sAuthExecute = 0;
static AuthFreeProc sAuthFree = 0;
static FwriteProc sFwrite = 0;
static FcloseProc sFclose = 0;

static void *_FindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName)
{
    static FSRef sFrameworks;
    static unsigned char sFound;
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

char *GetPathToSystemPreferences(const char *name)
{
    CFURLRef folderURL;
    CFURLRef fileURL;
    CFStringRef nameString;
    CFStringRef pathString;
    char *path;
    ULong length;
    ULong size;
    FSRef folder;

    folderURL = 0;
    fileURL = 0;
    nameString = 0;
    pathString = 0;
    path = 0;
    size = 1;
    if (FSFindFolder(kLocalDomain, kPreferencesFolderType, kCreateFolder, &folder) == 0) {
        if ((folderURL = CFURLCreateFromFSRef(0, &folder)) != 0
            && (nameString = CFStringCreateWithCString(0, name, kCFStringEncodingASCII)) != 0
            && (fileURL = CFURLCreateCopyAppendingPathComponent(0, folderURL, nameString, false)) != 0
            && (pathString = CFURLCopyFileSystemPath(fileURL, kCFURLPOSIXPathStyle)) != 0) {
            length = CFStringGetLength(pathString);
            do {
                size += length;
                if (path != 0) {
                    MemoryDeallocate(path);
                    path = 0;
                }
                if (size > length + kMaxUtf8Growth)
                    break;
                path = MemoryAllocate(size);
                if (path == 0)
                    break;
            } while (!CFStringGetCString(pathString, path, size, kUtf8Encoding));
        }
    }
    if (folderURL != 0)
        CFRelease(folderURL);
    if (fileURL != 0)
        CFRelease(fileURL);
    if (nameString != 0)
        CFRelease(nameString);
    if (pathString != 0)
        CFRelease(pathString);
    return path;
}

long WriteToFileAsSuperUser(char *path, const void *data, long size)
{
    ULong err = 0;
    long e;
    void *authRef = 0;
    AuthItem item = { kAuthRightExecute, 0, 0, 0 };
    AuthRights rights;
    const char *tool = "/usr/bin/tee";
    char *args[2] = { "", 0 };
    void *pipe = 0;

    rights.count = 1;
    rights.items = &item;
    args[0] = path;
    if ((sAuthCreate = (AuthCreateProc)_FindBundleSymbol(CFSTR("Security.framework"), CFSTR("AuthorizationCreate"))) == 0) {
        err = kNoSymbolFound;
        goto done;
    }
    if ((sAuthCopyRights = (AuthCopyRightsProc)_FindBundleSymbol(CFSTR("Security.framework"), CFSTR("AuthorizationCopyRights"))) == 0) {
        err = kNoSymbolFound;
        goto done;
    }
    if ((sAuthExecute = (AuthExecuteProc)_FindBundleSymbol(CFSTR("Security.framework"), CFSTR("AuthorizationExecuteWithPrivileges"))) == 0) {
        err = kNoSymbolFound;
        goto done;
    }
    if ((sAuthFree = (AuthFreeProc)_FindBundleSymbol(CFSTR("Security.framework"), CFSTR("AuthorizationFree"))) == 0) {
        err = kNoSymbolFound;
        goto done;
    }
    if ((sFwrite = (FwriteProc)_FindBundleSymbol(CFSTR("System.framework"), CFSTR("fwrite"))) == 0) {
        err = kNoSymbolFound;
        goto done;
    }
    if ((sFclose = (FcloseProc)_FindBundleSymbol(CFSTR("System.framework"), CFSTR("fclose"))) == 0) {
        err = kNoSymbolFound;
        goto done;
    }
    e = sAuthCreate(0, 0, 0, &authRef);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = sAuthCopyRights(authRef, &rights, 0, kAuthAllowInteraction | kAuthExtendRights | kAuthPreAuthorize, 0);
    if (e != 0) {
        err = e;
        goto done;
    }
    e = sAuthExecute(authRef, tool, 0, args, &pipe);
    if (e != 0) {
        err = e;
        goto done;
    }
    if ((long)sFwrite(data, 1, size, pipe) < size)
        err = -1;
done:
    if (sFclose != 0 && pipe != 0)
        sFclose(pipe);
    if (sAuthFree != 0 && authRef != 0)
        sAuthFree(authRef, 0);
    if (err == (ULong)kAuthCanceled)
        err = kUserCanceled;
    return err;
}

unsigned char RT3_CanSetSystemLicense(void)
{
    GetEuidProc geteuid;

    if (_PlatformIsThreaded()) {
        geteuid = (GetEuidProc)_FindBundleSymbol(CFSTR("System.framework"), CFSTR("geteuid"));
        if (geteuid != 0 && geteuid() == 0)
            return 1;
    }
    return 0;
}

#endif
