/* make_alias.c -- FSSpec/POSIX path conversion and the relative alias file writer, the mac 1.1.1 make_alias.o (mac only). The first two are Apple's Kerberos FSpUtils.c (MIT licence). Notes: Research/notes/morefilesx.md */
#if V111

#define dprintf(...)

/* FSSpecToPOSIXPath (mac _FSSpecToPOSIXPath, Kerberos FSpUtils.c). STATE: see the mac gate. */
// MAC: _FSSpecToPOSIXPath i386 0x000cce45 ppc 0x000d8b88
OSStatus FSSpecToPOSIXPath(const FSSpec *inSpec, char *ioPath, unsigned long inPathLength)
{
    OSStatus err = noErr;
    FSRef ref;
    FSSpec spec;
    CFStringRef nameString = NULL;
    CFStringRef pathString = NULL;
    CFURLRef pathURL = NULL;
    CFURLRef parentURL = NULL;
    int i;

    dprintf("FSSpecToPOSIXPath called on volID %d, parID %d and name '", inSpec->vRefNum, inSpec->parID);
    for (i = 0; i < inSpec->name[0]; i++) { dprintf("%c", inSpec->name[i+1]); }
    dprintf("'\n");

    if (err == noErr) {
        err = FSpMakeFSRef(inSpec, &ref);
    }

    if (err == noErr) {
        err = FSRefMakePath(&ref, (UInt8 *)ioPath, inPathLength);
    } else {
        err = noErr;

        if (err == noErr) {
            Str31 name;
            name[0] = 0;

            err = FSMakeFSSpec(inSpec->vRefNum, inSpec->parID, name, &spec);
        }

        if (err == noErr) {
            err = FSpMakeFSRef(&spec, &ref);
        }

        if (err == noErr) {
            parentURL = CFURLCreateFromFSRef(CFAllocatorGetDefault(), &ref);
            if (parentURL == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            nameString = CFStringCreateWithPascalString(CFAllocatorGetDefault(), inSpec->name,
                                                        kCFStringEncodingMacRoman);
            if (nameString == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            pathURL = CFURLCreateCopyAppendingPathComponent(CFAllocatorGetDefault(),
                                                            parentURL, nameString,
                                                            false);
            if (pathURL == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            pathString = CFURLCopyFileSystemPath(pathURL, kCFURLPOSIXPathStyle);
            if (pathString == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            Boolean converted = CFStringGetCString(pathString, ioPath, inPathLength, CFStringGetSystemEncoding());
            if (!converted) { err = fnfErr; }
        }
    }

    if (parentURL != NULL)  { CFRelease(parentURL);  }
    if (nameString != NULL) { CFRelease(nameString); }
    if (pathURL != NULL)    { CFRelease(pathURL);    }
    if (pathString != NULL) { CFRelease(pathString); }

    if (err == noErr) {
        dprintf("FSSpecToPOSIXPath returned path '%s'\n", ioPath);
    } else {
        dprintf("FSSpecToPOSIXPath returned error %d\n", err);
    }

    return err;
}

/* POSIXPathToFSSpec (mac _POSIXPathToFSSpec, Kerberos FSpUtils.c). STATE: see the mac gate. */
// MAC: _POSIXPathToFSSpec i386 0x000cd03c ppc 0x000d8da0
OSStatus POSIXPathToFSSpec(const char *inPath, FSSpec *outSpec)
{
    OSStatus err = noErr;
    FSRef ref;
    Boolean isDirectory;
    FSCatalogInfo info;
    CFStringRef pathString = NULL;
    CFURLRef pathURL = NULL;
    CFURLRef parentURL = NULL;
    CFStringRef nameString = NULL;

    dprintf("FSSpecToPOSIXPath called on '%s'\n", inPath);

    if (err == noErr) {
        err = FSPathMakeRef((UInt8 *)inPath, &ref, &isDirectory);
    }

    if (err == noErr) {
        err = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, outSpec, NULL);
    } else {
        err = noErr;

        if (err == noErr) {
            pathString = CFStringCreateWithCString(CFAllocatorGetDefault(), inPath, CFStringGetSystemEncoding());
            if (pathString == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            pathURL = CFURLCreateWithFileSystemPath(CFAllocatorGetDefault(),
                                                    pathString, kCFURLPOSIXPathStyle,
                                                    false);
            if (pathURL == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            parentURL = CFURLCreateCopyDeletingLastPathComponent(CFAllocatorGetDefault(), pathURL);
            if (parentURL == NULL) { err = memFullErr; }
        }

        if (err == noErr) {
            Boolean converted = CFURLGetFSRef(parentURL, &ref);
            if (!converted) { err = fnfErr; }
        }

        if (err == noErr) {
            err = FSGetCatalogInfo(&ref, kFSCatInfoNodeFlags | kFSCatInfoNodeID, &info, NULL, outSpec, NULL);
        }

        if (err == noErr) {
            nameString = CFURLCopyLastPathComponent(pathURL);
            if (nameString == NULL) { err = memFullErr; }
        }

        /* BUG: copies the whole path, not nameString, into the FSSpec name */
        if (err == noErr) {
            Boolean converted = CFStringGetPascalString(pathString, outSpec->name, sizeof(outSpec->name),
                                                        CFStringGetSystemEncoding());
            if (!converted) { err = fnfErr; }
        }

        if (err == noErr) {
            outSpec->parID = info.nodeID;
        }
    }

    if (err == noErr) {
        int i;

        dprintf("POSIXPathToFSSpec returned volID %d, parID %d and name '", outSpec->vRefNum, outSpec->parID);
        for (i = 0; i < outSpec->name[0]; i++) { dprintf("%c", outSpec->name[i+1]); }
        dprintf("'\n");
    } else {
        dprintf("POSIXPathToFSSpec returned error %d\n", err);
    }

    if (pathURL != NULL)    { CFRelease(pathURL);    }
    if (pathString != NULL) { CFRelease(pathString); }
    if (parentURL != NULL)  { CFRelease(parentURL);  }
    if (nameString != NULL) { CFRelease(nameString); }

    return err;
}

/* MakeRelativeAliasFile (mac _MakeRelativeAliasFile, mac-only). STATE: see the mac gate. */
// MAC: _MakeRelativeAliasFile i386 0x000cd296 ppc 0x000d8fe4
OSErr MakeRelativeAliasFile(const FSSpec *fileSpec, const FSSpec *aliasSpec)
{
    OSErr err;
    FInfo fndrInfo;
    AliasHandle alias = NULL;
    short rsrc;

    err = FSpGetFInfo(fileSpec, &fndrInfo);
    if (err != noErr)
        goto bail;
    if (fndrInfo.fdType == 'APPL')
        fndrInfo.fdType = 'adrp';
    fndrInfo.fdFlags = 0x8000;
    FSpCreateResFile(aliasSpec, 'TEMP', 'TEMP', smSystemScript);
    err = ResError();
    if (err != noErr)
        goto bail;
    err = FSpSetFInfo(aliasSpec, &fndrInfo);
    if (err != noErr)
        goto del;
    err = NewAlias(aliasSpec, fileSpec, &alias);
    if (err != noErr)
        goto del;
    rsrc = FSpOpenResFile(aliasSpec, fsRdWrPerm);
    if (rsrc == -1) {
        err = ResError();
    del:
        FSpDelete(aliasSpec);
    } else {
        UseResFile(rsrc);
        AddResource((Handle)alias, rAliasType, 0, aliasSpec->name);
        err = ResError();
        if (err != noErr)
            goto closeIt;
        alias = NULL;
        CloseResFile(rsrc);
        err = ResError();
        if (err != noErr)
            goto del;
        return noErr;
    }
bail:
    if (alias != NULL)
        DisposeHandle((Handle)alias);
    return err;
closeIt:
    CloseResFile(rsrc);
    goto del;
}
#endif
