/* evplugins.c -- OpenEVPlugins and CountEVPlugins over an FSRef folder, the mac 1.1.1 EV Plugins.o (mac only; win: engine/filescan.c). Notes: Research/notes/filescan.md */
#include "macport.h"
#include "gameutil.h"
#include "crashlog.h"

#ifdef MAC1010
extern OSErr GetPluginSubfolder(short refNum, ConstStr255Param name, long *dirID, short *vRefNum);

short OpenEVPlugins(short refNum, ConstStr255Param folderName, OSType type, FILE *log)
{
    Boolean isFolder;
    Boolean wasAliased;
    short vRefNum;
    long dirID;
    FInfo info;
    FSSpec spec;
    Str255 name;
    char cName[255];
    OSErr result;
    OSErr err;
    Boolean logged;
    CInfoPBRec *pb;

    result = noErr;
    err = GetPluginSubfolder(refNum, folderName, &dirID, &vRefNum);
    if (err == noErr) {
        name[0] = 0;
        pb = (CInfoPBRec *)NewPtrClear(sizeof(CInfoPBRec));
        if (pb != NULL) {
            pb->hFileInfo.ioFDirIndex = 0;
            pb->hFileInfo.ioCompletion = NULL;
            pb->hFileInfo.ioNamePtr = name;
            pb->hFileInfo.ioVRefNum = vRefNum;
            pb->hFileInfo.ioDirID = dirID;
            while (err == noErr) {
                pb->hFileInfo.ioDirID = dirID;
                pb->hFileInfo.ioCompletion = NULL;
                pb->hFileInfo.ioNamePtr = name;
                pb->hFileInfo.ioFDirIndex++;
                err = PBGetCatInfoSync(pb);
                if (err == noErr) {
                    spec.vRefNum = pb->hFileInfo.ioVRefNum;
                    spec.parID = dirID;
                    BlockMove(name, spec.name, name[0] + 1);
                    ResolveAliasFile(&spec, true, &isFolder, &wasAliased);
                    err = FSpGetFInfo(&spec, &info);
                    if (isFolder)
                        err = noErr;
                    else if (info.fdType == type || type == kAnyPluginType) {
                        logged = 0;
                        if (log != NULL && spec.name[0] != 0 && spec.name[spec.name[0]] != '.'
                                && spec.name[spec.name[0]] != '\r' && spec.name[spec.name[0]] != '\n'
                                && info.fdType != 0 && info.fdCreator != 0) {
                            logged = 1;
                            strncpy(cName, (char *)&spec.name[1], spec.name[0]);
                            cName[spec.name[0]] = 0;
                            fprintf(log, "  %s", cName);
                            fflush(log);
                        }
                        refNum = FSpOpenResFile(&spec, fsCurPerm);
                        if (logged) {
                            if (refNum == -1)
                                fprintf(log, " - failed or has no resource fork");
                            fprintf(log, "\n");
                            fflush(log);
                        }
                    }
                }
            }
            DisposePtr((Ptr)pb);
        } else
            result = memFullErr;
    } else
        result = err;
    return result;
}

short CountEVPlugins(short refNum, ConstStr255Param folderName, OSType type, FILE *log)
{
    Boolean isFolder;
    Boolean wasAliased;
    short vRefNum;
    long dirID;
    FInfo info;
    FSSpec spec;
    Str255 name;
    char cName[255];
    OSErr result;
    OSErr err;
    short count;
    CInfoPBRec *pb;

    result = noErr;
    count = 0;
    err = GetPluginSubfolder(refNum, folderName, &dirID, &vRefNum);
    if (err == noErr) {
        name[0] = 0;
        pb = (CInfoPBRec *)NewPtrClear(sizeof(CInfoPBRec));
        if (pb != NULL) {
            pb->hFileInfo.ioFDirIndex = 0;
            pb->hFileInfo.ioCompletion = NULL;
            pb->hFileInfo.ioNamePtr = name;
            pb->hFileInfo.ioVRefNum = vRefNum;
            pb->hFileInfo.ioDirID = dirID;
            while (err == noErr) {
                pb->hFileInfo.ioDirID = dirID;
                pb->hFileInfo.ioCompletion = NULL;
                pb->hFileInfo.ioNamePtr = name;
                pb->hFileInfo.ioFDirIndex++;
                err = PBGetCatInfoSync(pb);
                if (err == noErr) {
                    spec.vRefNum = pb->hFileInfo.ioVRefNum;
                    spec.parID = dirID;
                    BlockMove(name, spec.name, name[0] + 1);
                    ResolveAliasFile(&spec, true, &isFolder, &wasAliased);
                    err = FSpGetFInfo(&spec, &info);
                    if (isFolder)
                        err = noErr;
                    else if (info.fdType == type || type == kAnyPluginType) {
                        count++;
                        if (log != NULL && spec.name[0] != 0 && spec.name[spec.name[0]] != '.'
                                && spec.name[spec.name[0]] != '\r' && spec.name[spec.name[0]] != '\n'
                                && info.fdType != 0 && info.fdCreator != 0) {
                            strncpy(cName, (char *)&spec.name[1], spec.name[0]);
                            cName[spec.name[0]] = 0;
                            fprintf(log, "  %s\n", cName);
                            fflush(log);
                        }
                    }
                }
            }
            DisposePtr((Ptr)pb);
        } else
            result = memFullErr;
    } else
        result = err;
    return result == noErr ? count : (short)0;
}

OSErr GetPluginSubfolder(short refNum, ConstStr255Param name, long *dirID, short *vRefNum)
{
    FCBPBRec fcb;
    FSSpec spec;
    Boolean wasAliased;
    Boolean isFolder;
    Str255 fileName;
    CInfoPBRec *pb;
    OSErr err;
    OSErr aliasErr;

    fcb.ioRefNum = refNum;
    fcb.ioNamePtr = fileName;
    fcb.ioFCBIndx = 0;
    err = PBGetFCBInfoSync(&fcb);
    if (err == noErr) {
        fileName[0] = 0;
        pb = (CInfoPBRec *)NewPtrClear(sizeof(CInfoPBRec));
        if (pb != NULL) {
            spec.vRefNum = fcb.ioFCBVRefNum;
            spec.parID = fcb.ioFCBParID;
            BlockMove(name, spec.name, name[0] + 1);
            BlockMove(name, fileName, name[0] + 1);
            aliasErr = ResolveAliasFile(&spec, true, &isFolder, &wasAliased);
            if (aliasErr == noErr && wasAliased) {
                pb->dirInfo.ioNamePtr = spec.name;
                pb->dirInfo.ioVRefNum = spec.vRefNum;
                pb->dirInfo.ioDrDirID = spec.parID;
            } else {
                pb->dirInfo.ioNamePtr = fileName;
                pb->dirInfo.ioVRefNum = fcb.ioFCBVRefNum;
                pb->dirInfo.ioDrDirID = fcb.ioFCBParID;
            }
            pb->dirInfo.ioFDirIndex = 0;
            pb->dirInfo.ioCompletion = NULL;
            err = PBGetCatInfoSync(pb);
            if (err == noErr) {
                *dirID = pb->dirInfo.ioDrDirID;
                *vRefNum = pb->dirInfo.ioVRefNum;
            }
            DisposePtr((Ptr)pb);
        }
    }
    return err;
}
#else
/* OpenEVPlugins (mac _OpenEVPlugins, mac-only). STATE: see the mac gate. */
// MAC: _OpenEVPlugins i386 0x000a40e7 ppc 0x000afb10
short OpenEVPlugins(FSRef folder, OSType type)
{
    FSIterator iterator;
    ItemCount count;
    FSCatalogInfo info;
    FSRef ref;
    OSErr err;
    OSType wantedType;

    wantedType = type;
    count = 1;
    err = FSOpenIterator(&folder, kFSIterateFlat, &iterator);
    require_noerr(err, bail);
    while (err == noErr && count != 0) {
        err = FSGetCatalogInfoBulk(iterator, 1, &count, NULL, kFSCatInfoFinderInfo, &info, &ref, NULL, NULL);
        if (err == noErr && count == 1) {
            if (wantedType == ((FileInfo *)info.finderInfo)->fileType || wantedType == kAnyPluginType) {
                if (EVOpenResourceFile(&ref, fsRdPerm) == -1)
                    crashlog_print(" - failed or has no resource fork");
                crashlog_print("\n");
            }
        }
    }
    FSCloseIterator(iterator);
    return err;
bail:
    return err;
}

/* CountEVPlugins (mac _CountEVPlugins, mac-only). STATE: see the mac gate. OPEN: none */
// MAC: _CountEVPlugins i386 0x000a41f1 ppc 0x000afc44
short CountEVPlugins(FSRef folder, OSType type)
{
    FSIterator iterator;
    ItemCount count;
    FSCatalogInfo info;
    FSRef ref;
    OSErr err;
    int total;

    total = 0;
    if (total == 0 && type != 0) /* V111 pad: total is still 0 here, folds away */
        total = 0;
    count = 1;
    err = FSOpenIterator(&folder, kFSIterateFlat, &iterator);
    require_noerr(err, bail);
    while (err == noErr && count != 0) {
        err = FSGetCatalogInfoBulk(iterator, 1, &count, NULL, kFSCatInfoFinderInfo, &info, &ref, NULL, NULL);
        if (err == noErr && count == 1) {
            if (type == ((FileInfo *)info.finderInfo)->fileType || type == kAnyPluginType)
                total++;
        }
    }
    FSCloseIterator(iterator);
    return total;
bail:
    return 0;
}
#endif
