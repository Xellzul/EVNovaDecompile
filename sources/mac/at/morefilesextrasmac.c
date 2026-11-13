/* morefilesextrasmac.c -- 1.0.10: the few calls of Apple's MoreFilesExtras.c the file search needs, read off the words. Notes: Research/notes/maclibs.md */
#include <Files.h>
#include <Memory.h>
#include <Timer.h>
#include <OSUtils.h>
#include <TextUtils.h>
#include <Gestalt.h>
#include <MixedMode.h>
#include "int32.h"

#define kMinBuffer      0x400
#define kBufferRound    0xfffffc00
#define kTempSlop       0x8000

OSErr HGetVolParms(ConstStr255Param volName, short vRefNum, GetVolParmsInfoBuffer *volParmsInfo, int32_t *infoSize)
{
    HParamBlockRec pb;
    OSErr error;

    pb.ioParam.ioNamePtr = (StringPtr)volName;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.ioParam.ioBuffer = (Ptr)volParmsInfo;
    pb.ioParam.ioReqCount = *infoSize;
    error = PBHGetVolParmsSync(&pb);
    if (error == noErr)
        *infoSize = pb.ioParam.ioActCount;
    return error;
}

Boolean hasCatSearch(GetVolParmsInfoBuffer *volParms)
{
    return (volParms->vMAttrib & (1L << bHasCatSearch)) != 0;
}

Ptr GetTempBuffer(int32_t buffReqSize, int32_t *buffActSize)
{
    Ptr tempPtr;

    buffReqSize = buffReqSize & kBufferRound;
    if (buffReqSize < kMinBuffer)
        buffReqSize = kMinBuffer;
    tempPtr = NewPtr(buffReqSize);
    if (tempPtr == 0 && buffReqSize > kMinBuffer) {
        int32_t freeMemory = (FreeMem() - kTempSlop) & kBufferRound;

        buffReqSize = MaxBlock() & kBufferRound;
        if (buffReqSize > freeMemory)
            buffReqSize = freeMemory;
        if (buffReqSize == 0)
            buffReqSize = kMinBuffer;
        tempPtr = NewPtr(buffReqSize);
    }
    if (tempPtr != 0)
        *buffActSize = buffReqSize;
    else
        *buffActSize = 0;
    return tempPtr;
}

OSErr GetVolumeInfoNoName(ConstStr255Param pathname, short vRefNum, HParmBlkPtr pb)
{
    Str255 tempPathname;
    OSErr error;

    if (pb != 0) {
        pb->volumeParam.ioVRefNum = vRefNum;
        if (pathname == 0) {
            pb->volumeParam.ioNamePtr = 0;
            pb->volumeParam.ioVolIndex = 0;
        } else {
            BlockMoveData(pathname, tempPathname, pathname[0] + 1);
            pb->volumeParam.ioNamePtr = tempPathname;
            pb->volumeParam.ioVolIndex = -1;
        }
        error = PBHGetVInfoSync(pb);
        pb->volumeParam.ioNamePtr = 0;
    } else {
        error = paramErr;
    }
    return error;
}

OSErr DetermineVRefNum(ConstStr255Param pathname, short vRefNum, short *realVRefNum)
{
    HParamBlockRec pb;
    OSErr error;

    error = GetVolumeInfoNoName(pathname, vRefNum, &pb);
    if (error == noErr)
        *realVRefNum = pb.volumeParam.ioVRefNum;
    return error;
}
