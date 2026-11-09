/* rt3_superuser.c -- RT3, the system-wide license file: the path of the system preferences folder and a write through the authorization services; the mac 1.1.1 librt3_nonag.a(rt3_superuser.o) (mac only). Notes: Research/notes/rt3mac.md */
#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#include <stdio.h>
#include "rt3_mac.h"

extern void *MemoryAllocate(unsigned long size);
extern void MemoryDeallocate(void *p);

// MAC: _GetPathToSystemPreferences i386 0x000d5ed0 ppc 0x000e0fa4
char *GetPathToSystemPreferences(const char *name)
{
    CFURLRef fileURL = 0;
    CFStringRef nameString = 0;
    CFStringRef pathString = 0;
    char *path = 0;
    CFIndex length;
    unsigned long size;
    CFURLRef folderURL = 0;
    FSRef folder;

    if (FSFindFolder(kLocalDomain, kPreferencesFolderType, true, &folder) == noErr) {
        folderURL = CFURLCreateFromFSRef(0, &folder);
        if (folderURL != 0) {
            nameString = CFStringCreateWithCString(0, name, kCFStringEncodingASCII);
            if (nameString != 0) {
                fileURL = CFURLCreateCopyAppendingPathComponent(0, folderURL, nameString, false);
                if (fileURL != 0) {
                    pathString = CFURLCopyFileSystemPath(fileURL, kCFURLPOSIXPathStyle);
                    if (pathString != 0) {
                        length = CFStringGetLength(pathString);
                        size = 1;
                        do {
                            size += length;
                            if (path != 0) {
                                MemoryDeallocate(path);
                                path = 0;
                            }
                            if (size > length + 5)
                                break;
                            path = MemoryAllocate(size);
                            if (path == 0)
                                break;
                        } while (!CFStringGetCString(pathString, path, size, kCFStringEncodingUTF8));
                    }
                }
            }
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

// MAC: _WriteToFileAsSuperUser i386 0x000d607f ppc 0x000e1130
OSStatus WriteToFileAsSuperUser(char *path, const void *data, long size)
{
    OSStatus err = 0;
    OSStatus e;
    AuthorizationRef authRef = 0;
    AuthorizationItem item = { kAuthorizationRightExecute, 0, 0, 0 };
    AuthorizationRights rights;
    char *args[2] = { "", 0 };
    FILE *pipe = 0;
    long written;

    rights.count = 1;
    rights.items = &item;
    args[0] = path;
    e = AuthorizationCreate(0, 0, 0, &authRef);
    if (e != 0)
        err = e;
    if (err == 0) {
        e = AuthorizationCopyRights(authRef, &rights, 0, kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights | kAuthorizationFlagPreAuthorize, 0);
        if (e != 0)
            err = e;
    }
    if (err == 0) {
        e = AuthorizationExecuteWithPrivileges(authRef, "/usr/bin/tee", 0, args, &pipe);
        if (e != 0)
            err = e;
    }
    if (err == 0) {
        written = fwrite(data, 1, size, pipe);
        err = (size <= written) - 1;
    }
    if (pipe != 0)
        fclose(pipe);
    if (authRef != 0)
        AuthorizationFree(authRef, 0);
    if (err == errAuthorizationCanceled)
        err = -7007;
    return err;
}
