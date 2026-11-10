/* atsearchmac.c -- 1.0.10: the file search behind the reminder's Register launch: Apple's MoreFiles Search.c, read off the words. Notes: Research/notes/maclibs.md */
#include <Files.h>
#include <Memory.h>
#include <Timer.h>
#include <OSUtils.h>
#include <TextUtils.h>
#include <Gestalt.h>
#include <MixedMode.h>

#define kAdditionalDirs 20
#define kPBDirBit       0x10
#define kDefaultSearch  0x4000

/* a level of the directory walk, kept on a handle between calls */
typedef struct LevelRec {
    long dirModDate;
    long dirID;
    short index;
} LevelRec;
typedef LevelRec **LevelRecHandle;

typedef struct CatPosition {
    long initialize;
    unsigned short stackDepth;
    short priv[5];
} CatPosition;

typedef struct ExtendedTMTask {
    TMTask task;
    Boolean stopSearch;
} ExtendedTMTask;

static LevelRecHandle searchStack = 0;
static Size searchStackSize = 0;
static short lastVRefNum[1];   /* 1.0.10 pad: a word-aligned hunk, as the exe's (b199 aligns an array, not a scalar short, on a word) */
static CatPositionRec catPosition;
static RoutineDescriptor sTimeOutDescriptor;

extern OSErr HGetVolParms(ConstStr255Param volName, short vRefNum, GetVolParmsInfoBuffer *volParmsInfo, long *infoSize);
extern Boolean hasCatSearch(GetVolParmsInfoBuffer *volParms);
extern Ptr GetTempBuffer(long buffReqSize, long *buffActSize);
extern OSErr GetVolumeInfoNoName(ConstStr255Param pathname, short vRefNum, HParmBlkPtr pb);
extern OSErr DetermineVRefNum(ConstStr255Param pathname, short vRefNum, short *realVRefNum);

static OSErr CheckVol(ConstStr255Param pathname, short vRefNum, short *realVRefNum, long *volID)
{
    HParamBlockRec pb;
    OSErr error;

    error = GetVolumeInfoNoName(pathname, vRefNum, &pb);
    if (error == noErr) {
        *realVRefNum = pb.volumeParam.ioVRefNum;
        *volID = pb.volumeParam.ioVCrDate + pb.volumeParam.ioVRefNum + pb.volumeParam.ioVNmAlBlks + pb.volumeParam.ioVAlBlkSiz + pb.volumeParam.ioVFSID;
    }
    return error;
}

static OSErr CheckStack(unsigned short stackDepth, LevelRecHandle searchStack, Size *searchStackSize)
{
    OSErr result;

    if ((*searchStackSize / sizeof(LevelRec)) == (stackDepth + 1)) {
        SetHandleSize((Handle)searchStack, *searchStackSize + (kAdditionalDirs * sizeof(LevelRec)));
        result = MemError();
        *searchStackSize = GetHandleSize((Handle)searchStack);
    } else {
        result = noErr;
    }
    return result;
}

static OSErr VerifyUserPB(CSParamPtr userPB, Boolean *includeFiles, Boolean *includeDirs, Boolean *includeNames)
{
    CInfoPBRec *searchInfo1;
    CInfoPBRec *searchInfo2;

    searchInfo1 = userPB->ioSearchInfo1;
    searchInfo2 = userPB->ioSearchInfo2;
    if (userPB->ioMatchPtr == 0)
        goto ParamErrExit;
    if (searchInfo1 == 0)
        goto ParamErrExit;
    if ((userPB->ioSearchBits & ~(fsSBPartialName | fsSBFullName | fsSBNegate)) != 0) {
        if (searchInfo2 == 0)
            goto ParamErrExit;
    }
    *includeFiles = false;
    *includeDirs = false;
    *includeNames = false;
    if ((userPB->ioSearchBits & (fsSBPartialName | fsSBFullName)) != 0) {
        if (searchInfo1->hFileInfo.ioNamePtr == 0 || searchInfo1->hFileInfo.ioNamePtr[0] == 0 || searchInfo1->hFileInfo.ioNamePtr[0] > (sizeof(Str63) - 1))
            goto ParamErrExit;
        *includeNames = true;
    }
    if ((userPB->ioSearchBits & fsSBFlAttrib) != 0) {
        if ((searchInfo2->hFileInfo.ioFlAttrib & ~(kioFlAttribDirMask | kioFlAttribLockedMask)) != 0)
            goto ParamErrExit;
        if ((searchInfo2->hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0) {
            if ((searchInfo1->hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0)
                *includeDirs = true;
            else
                *includeFiles = true;
        } else {
            *includeDirs = true;
            *includeFiles = true;
        }
    } else {
        *includeDirs = true;
        *includeFiles = true;
    }
    if (*includeDirs && (userPB->ioSearchBits & fsSBFlAttrib) != 0
        && (searchInfo2->hFileInfo.ioFlAttrib & kioFlAttribLockedMask) != 0)
        goto ParamErrExit;
    if (*includeFiles && (userPB->ioSearchBits & fsSBDrNmFls) != 0)
        goto ParamErrExit;
    if (*includeDirs && (userPB->ioSearchBits & (fsSBFlLgLen | fsSBFlPyLen | fsSBFlRLgLen | fsSBFlRPyLen)) != 0)
        goto ParamErrExit;
    return noErr;
ParamErrExit:
    return paramErr;
}

static Boolean IsSubString(ConstStr255Param aStringPtr, ConstStr255Param subStringPtr)
{
    short strLength;
    short subStrLength;
    Boolean found;
    short index;

    found = false;
    strLength = aStringPtr[0];
    subStrLength = subStringPtr[0];
    if (subStrLength <= strLength) {
        register short count;
        register short strIndex;
        register short subStrIndex;

        index = 1;
        count = strLength - subStrLength + 1;
        do {
            strIndex = index;
            subStrIndex = 1;
            while (!found && (aStringPtr[strIndex] == subStringPtr[subStrIndex])) {
                if (subStrIndex == subStrLength) {
                    found = true;
                } else {
                    ++subStrIndex;
                    ++strIndex;
                }
            }
            if (!found) {
                ++index;
                --count;
            }
        } while (count != 0 && (!found));
    }
    return (found);
}

static Boolean CompareMasked(const long *vals, const long *cvals, const long *mvals, short nvals)
{
    Boolean result = true;

    for (; nvals != 0 && result == true; nvals--) {
        if ((*vals ^ *cvals) & *mvals)
            result = false;
        vals++;
        cvals++;
        mvals++;
    }
    return result;
}

static void CheckForMatches(CInfoPBPtr cPB, CSParamPtr pb, const Str63 matchName, Boolean includeFiles, Boolean includeDirs)
{
    long searchBits;
    CInfoPBRec *searchInfo1;
    CInfoPBRec *searchInfo2;
    Str63 itemName;
    Boolean foundMatch;

    foundMatch = false;
    searchBits = pb->ioSearchBits;
    searchInfo1 = pb->ioSearchInfo1;
    searchInfo2 = pb->ioSearchInfo2;
    if ((cPB->hFileInfo.ioFlAttrib & kioFlAttribDirMask) == 0) {
        if (!includeFiles)
            goto check;
    } else {
        if (!includeDirs)
            goto check;
    }
    if ((searchBits & fsSBPartialName) != 0) {
        if (cPB->hFileInfo.ioNamePtr[0] > 0 && cPB->hFileInfo.ioNamePtr[0] <= (sizeof(Str63) - 1)) {
            BlockMoveData(cPB->hFileInfo.ioNamePtr, itemName, cPB->hFileInfo.ioNamePtr[0] + 1);
            UpperString(itemName, true);
            if (!IsSubString(itemName, matchName))
                goto check;
            if (searchBits == fsSBPartialName)
                goto hit;
        } else {
            goto check;
        }
    }
    if ((searchBits & fsSBFullName) != 0) {
        if (!EqualString(cPB->hFileInfo.ioNamePtr, matchName, false, true))
            goto check;
        if (searchBits == fsSBFullName)
            goto hit;
    }
    if ((searchBits & fsSBFlParID) != 0) {
        if (cPB->hFileInfo.ioFlParID < (unsigned long)searchInfo1->hFileInfo.ioFlParID)
            goto check;
        if (cPB->hFileInfo.ioFlParID > (unsigned long)searchInfo2->hFileInfo.ioFlParID)
            goto check;
    }
    if ((searchBits & fsSBFlAttrib) != 0) {
        if (((cPB->hFileInfo.ioFlAttrib ^ searchInfo1->hFileInfo.ioFlAttrib) & searchInfo2->hFileInfo.ioFlAttrib) != 0)
            goto check;
    }
    if ((searchBits & fsSBDrNmFls) != 0) {
        if (cPB->dirInfo.ioDrNmFls < searchInfo1->dirInfo.ioDrNmFls)
            goto check;
        if (cPB->dirInfo.ioDrNmFls > searchInfo2->dirInfo.ioDrNmFls)
            goto check;
    }
    if ((searchBits & fsSBFlFndrInfo) != 0) {
        if (!CompareMasked((long *)&cPB->hFileInfo.ioFlFndrInfo, (long *)&searchInfo1->hFileInfo.ioFlFndrInfo, (long *)&searchInfo2->hFileInfo.ioFlFndrInfo, 4))
            goto check;
    }
    if ((searchBits & fsSBFlXFndrInfo) != 0) {
        if (!CompareMasked((long *)&cPB->hFileInfo.ioFlXFndrInfo, (long *)&searchInfo1->hFileInfo.ioFlXFndrInfo, (long *)&searchInfo2->hFileInfo.ioFlXFndrInfo, 4))
            goto check;
    }
#define RANGE(bit, field)                                                                   \
    if ((searchBits & bit) != 0) {                                                          \
        if ((unsigned long)cPB->hFileInfo.field < (unsigned long)searchInfo1->hFileInfo.field) \
            goto check;                                                                     \
        if ((unsigned long)cPB->hFileInfo.field > (unsigned long)searchInfo2->hFileInfo.field) \
            goto check;                                                                     \
    }
    RANGE(fsSBFlLgLen, ioFlLgLen)
    RANGE(fsSBFlPyLen, ioFlPyLen)
    RANGE(fsSBFlRLgLen, ioFlRLgLen)
    RANGE(fsSBFlRPyLen, ioFlRPyLen)
    RANGE(fsSBFlCrDat, ioFlCrDat)
    RANGE(fsSBFlMdDat, ioFlMdDat)
    RANGE(fsSBFlBkDat, ioFlBkDat)
hit:
    foundMatch = true;
check:
    if ((searchBits & fsSBNegate) != 0)
        foundMatch = !foundMatch;
    if (foundMatch) {
        pb->ioMatchPtr[pb->ioActMatchCount].vRefNum = cPB->hFileInfo.ioVRefNum;
        pb->ioMatchPtr[pb->ioActMatchCount].parID = cPB->hFileInfo.ioFlParID;
        if (cPB->hFileInfo.ioNamePtr[0] > 63)
            cPB->hFileInfo.ioNamePtr[0] = 63;
        BlockMoveData(cPB->hFileInfo.ioNamePtr, pb->ioMatchPtr[pb->ioActMatchCount].name, cPB->hFileInfo.ioNamePtr[0] + 1);
        pb->ioActMatchCount++;
    }
}

static void TimeOutTask(ExtendedTMTask *task)
{
    task->stopSearch = true;
}

static long GetDirModDate(short vRefNum, long dirID)
{
    CInfoPBRec pb;
    Str31 tempName;
    long modDate;

    tempName[0] = 0;
    pb.dirInfo.ioNamePtr = tempName;
    pb.dirInfo.ioVRefNum = vRefNum;
    pb.dirInfo.ioDrDirID = dirID;
    pb.dirInfo.ioFDirIndex = -1;
    if (PBGetCatInfoSync(&pb) == noErr)
        modDate = pb.dirInfo.ioDrMdDat;
    else
        modDate = -1;
    return modDate;
}

static OSErr IndexedSearch(CSParamPtr pb, long dirID)
{
    CatPosition *position;
    short index;
    OSErr result;
    Boolean includeFiles;
    Boolean includeDirs;
    Boolean includeNames;
    short realVRefNum;
    long volID;
    long modDate;
    ExtendedTMTask timerTask;
    Str63 itemName;
    Str63 upperName;
    CInfoPBRec cPB;

    timerTask.stopSearch = false;
    if (pb->ioSearchTime != 0) {
        timerTask.task.tmAddr = NewTimerUPP((TimerProcPtr)TimeOutTask);
        InsTime((QElemPtr)&timerTask.task);
        PrimeTime((QElemPtr)&timerTask.task, pb->ioSearchTime);
    }
    result = VerifyUserPB(pb, &includeFiles, &includeDirs, &includeNames);
    if (result != noErr)
        goto stop;
    pb->ioActMatchCount = 0;
    if (includeNames) {
        BlockMoveData(pb->ioSearchInfo1->hFileInfo.ioNamePtr, upperName, pb->ioSearchInfo1->hFileInfo.ioNamePtr[0] + 1);
        UpperString(upperName, true);
    }
    position = (CatPosition *)&pb->ioCatPosition;
    if (searchStack == 0)
        searchStack = (LevelRecHandle)NewHandle(kAdditionalDirs * sizeof(LevelRec));
    if (searchStack == 0)
        goto noMemory;
    searchStackSize = GetHandleSize((Handle)searchStack);
    if (position->initialize == 0) {
        result = CheckVol(pb->ioNamePtr, pb->ioVRefNum, &realVRefNum, &position->initialize);
        if (result == noErr) {
            position->stackDepth = 0;
            index = -1;
        }
    } else {
        result = CheckVol(pb->ioNamePtr, pb->ioVRefNum, &realVRefNum, &volID);
        if (result == noErr) {
            if (position->initialize == volID) {
                if (position->stackDepth != 0) {
                    position->stackDepth--;
                    dirID = (*searchStack)[position->stackDepth].dirID;
                    index = (*searchStack)[position->stackDepth].index;
                    modDate = GetDirModDate(realVRefNum, dirID);
                    if (modDate != (*searchStack)[position->stackDepth].dirModDate)
                        result = catChangedErr;
                } else {
                    result = paramErr;
                }
            } else {
                result = catChangedErr;
            }
        }
    }
    if (result != noErr)
        goto stop;
    cPB.dirInfo.ioNamePtr = itemName;
    cPB.dirInfo.ioVRefNum = realVRefNum;
    do {
        cPB.dirInfo.ioFDirIndex = index;
        cPB.dirInfo.ioDrDirID = dirID;
        result = PBGetCatInfoSync(&cPB);
        if (index != -1) {
            if (result == noErr) {
                CheckForMatches(&cPB, pb, upperName, includeFiles, includeDirs);
                index++;
                if ((cPB.dirInfo.ioFlAttrib & kioFlAttribDirMask) != 0) {
                    result = CheckStack(position->stackDepth, searchStack, &searchStackSize);
                    if (result == noErr) {
                        (*searchStack)[position->stackDepth].dirID = dirID;
                        (*searchStack)[position->stackDepth].index = index;
                        (*searchStack)[position->stackDepth].dirModDate = GetDirModDate(realVRefNum, dirID);
                        index = 1;
                        position->stackDepth++;
                        dirID = cPB.dirInfo.ioDrDirID;
                    }
                }
            } else {
                if (position->stackDepth != 0) {
                    position->stackDepth--;
                    dirID = (*searchStack)[position->stackDepth].dirID;
                    index = (*searchStack)[position->stackDepth].index;
                    modDate = GetDirModDate(realVRefNum, dirID);
                    if (modDate != (*searchStack)[position->stackDepth].dirModDate)
                        result = catChangedErr;
                    else
                        result = noErr;
                } else {
                    result = eofErr;
                }
            }
        } else {
            if (result == noErr) {
                CheckForMatches(&cPB, pb, upperName, includeFiles, includeDirs);
                index = 1;
            }
        }
    } while (!timerTask.stopSearch && result == noErr && pb->ioReqMatchCount > pb->ioActMatchCount);
    if (result != noErr)
        goto stop;
    result = CheckStack(position->stackDepth, searchStack, &searchStackSize);
    if (result == noErr) {
        (*searchStack)[position->stackDepth].dirID = dirID;
        (*searchStack)[position->stackDepth].index = index;
        (*searchStack)[position->stackDepth].dirModDate = GetDirModDate(realVRefNum, dirID);
        position->stackDepth++;
    }
    goto stop;
noMemory:
    result = memFullErr;
stop:
    if (pb->ioSearchTime != 0) {
        RmvTime((QElemPtr)&timerTask.task);
        DisposeTimerUPP(timerTask.task.tmAddr);
    }
    return result;
}

static OSErr PBCatSearchSyncCompat(CSParamPtr paramBlock)
{
    OSErr result;
    GetVolParmsInfoBuffer volParmsInfo;
    long infoSize;
    Boolean supportsCatSearch;

    supportsCatSearch = false;
    infoSize = 0x1c;
    result = HGetVolParms(paramBlock->ioNamePtr, paramBlock->ioVRefNum, &volParmsInfo, &infoSize);
    if (result == noErr)
        supportsCatSearch = hasCatSearch(&volParmsInfo);
    if (result == noErr || result == paramErr) {
        if (supportsCatSearch)
            result = PBCatSearchSync(paramBlock);
        else
            result = IndexedSearch(paramBlock, fsRtDirID);
    }
    return result;
}

OSErr NameFileSearch(ConstStr255Param volName, short vRefNum, ConstStr255Param fileName, FSSpecPtr matches, long reqMatchCount, long *actMatchCount, Boolean newSearch, Boolean partial)
{
    CInfoPBRec searchInfo1;
    CInfoPBRec searchInfo2;
    HParamBlockRec pb;
    OSErr error;

    error = DetermineVRefNum(volName, vRefNum, &vRefNum);
    if (error != noErr)
        return error;
    pb.csParam.ioNamePtr = 0;
    pb.csParam.ioVRefNum = vRefNum;
    pb.csParam.ioMatchPtr = matches;
    pb.csParam.ioReqMatchCount = reqMatchCount;
    if (partial)
        pb.csParam.ioSearchBits = fsSBPartialName | fsSBFlAttrib;
    else
        pb.csParam.ioSearchBits = fsSBFullName | fsSBFlAttrib;
    pb.csParam.ioSearchInfo1 = &searchInfo1;
    pb.csParam.ioSearchInfo2 = &searchInfo2;
    pb.csParam.ioSearchTime = 0;
    if (newSearch || lastVRefNum[0] != vRefNum)
        catPosition.initialize = 0;
    pb.csParam.ioCatPosition = catPosition;
    pb.csParam.ioOptBuffer = GetTempBuffer(kDefaultSearch, &pb.csParam.ioOptBufSize);
    searchInfo1.hFileInfo.ioNamePtr = (StringPtr)fileName;
    searchInfo2.hFileInfo.ioNamePtr = 0;
    searchInfo1.hFileInfo.ioFlAttrib = 0;
    searchInfo2.hFileInfo.ioFlAttrib = kioFlAttribDirMask;
    error = PBCatSearchSyncCompat((CSParamPtr)&pb);
    if (error == noErr || error == eofErr)
        *actMatchCount = pb.csParam.ioActMatchCount;
    else
        *actMatchCount = 0;
    if (error == noErr || error == catChangedErr) {
        catPosition = pb.csParam.ioCatPosition;
        lastVRefNum[0] = vRefNum;
    } else {
        catPosition.initialize = 0;
    }
    if (pb.csParam.ioOptBuffer != 0)
        DisposePtr(pb.csParam.ioOptBuffer);
    return error;
}
