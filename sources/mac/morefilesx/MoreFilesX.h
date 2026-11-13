/* MoreFilesX.h -- the declarations of MoreFilesX.c. Notes: Research/notes/morefilesx.md */
#ifndef __MOREFILESX__
#define __MOREFILESX__

#ifndef __CARBON__
	#if defined(__MACH__)
		#include <Carbon/Carbon.h>
	#else
		#include <Carbon.h>
	#endif
#endif
#include "int32.h"

#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

union FinderInfo
{
  FileInfo				file;
  FolderInfo			folder;
};
typedef union FinderInfo FinderInfo;

union ExtendedFinderInfo
{
  ExtendedFileInfo		file;
  ExtendedFolderInfo	folder;
};
typedef union ExtendedFinderInfo ExtendedFinderInfo;

#define GetVolParmsInfoVersion(volParms) \
		((volParms)->vMVersion)
#define GetVolParmsInfoAttrib(volParms) \
		((volParms)->vMAttrib)
#define GetVolParmsInfoLocalHand(volParms) \
		((volParms)->vMLocalHand)
#define GetVolParmsInfoServerAdr(volParms) \
		((volParms)->vMServerAdr)

#define GetVolParmsInfoVolumeGrade(volParms) \
		(((volParms)->vMVersion >= 2) ? (volParms)->vMVolumeGrade : 0)
#define GetVolParmsInfoForeignPrivID(volParms) \
		(((volParms)->vMVersion >= 2) ? (volParms)->vMForeignPrivID : 0)

#define GetVolParmsInfoExtendedAttributes(volParms) \
		(((volParms)->vMVersion >= 3) ? (volParms)->vMExtendedAttributes : 0)

#define VolIsNetworkVolume(volParms) \
		((volParms)->vMServerAdr != 0)
#define VolHasLimitFCBs(volParms) \
		(((volParms)->vMAttrib & (1L << bLimitFCBs)) != 0)
#define VolHasLocalWList(volParms) \
		(((volParms)->vMAttrib & (1L << bLocalWList)) != 0)
#define VolHasNoMiniFndr(volParms) \
		(((volParms)->vMAttrib & (1L << bNoMiniFndr)) != 0)
#define VolHasNoVNEdit(volParms) \
		(((volParms)->vMAttrib & (1L << bNoVNEdit)) != 0)
#define VolHasNoLclSync(volParms) \
		(((volParms)->vMAttrib & (1L << bNoLclSync)) != 0)
#define VolHasTrshOffLine(volParms) \
		(((volParms)->vMAttrib & (1L << bTrshOffLine)) != 0)
#define VolHasNoSwitchTo(volParms) \
		(((volParms)->vMAttrib & (1L << bNoSwitchTo)) != 0)
#define VolHasNoDeskItems(volParms) \
		(((volParms)->vMAttrib & (1L << bNoDeskItems)) != 0)
#define VolHasNoBootBlks(volParms) \
		(((volParms)->vMAttrib & (1L << bNoBootBlks)) != 0)
#define VolHasAccessCntl(volParms) \
		(((volParms)->vMAttrib & (1L << bAccessCntl)) != 0)
#define VolHasNoSysDir(volParms) \
		(((volParms)->vMAttrib & (1L << bNoSysDir)) != 0)
#define VolHasExtFSVol(volParms) \
		(((volParms)->vMAttrib & (1L << bHasExtFSVol)) != 0)
#define VolHasOpenDeny(volParms) \
		(((volParms)->vMAttrib & (1L << bHasOpenDeny)) != 0)
#define VolHasCopyFile(volParms) \
		(((volParms)->vMAttrib & (1L << bHasCopyFile)) != 0)
#define VolHasMoveRename(volParms) \
		(((volParms)->vMAttrib & (1L << bHasMoveRename)) != 0)
#define VolHasDesktopMgr(volParms) \
		(((volParms)->vMAttrib & (1L << bHasDesktopMgr)) != 0)
#define VolHasShortName(volParms) \
		(((volParms)->vMAttrib & (1L << bHasShortName)) != 0)
#define VolHasFolderLock(volParms) \
		(((volParms)->vMAttrib & (1L << bHasFolderLock)) != 0)
#define VolHasPersonalAccessPrivileges(volParms) \
		(((volParms)->vMAttrib & (1L << bHasPersonalAccessPrivileges)) != 0)
#define VolHasUserGroupList(volParms) \
		(((volParms)->vMAttrib & (1L << bHasUserGroupList)) != 0)
#define VolHasCatSearch(volParms) \
		(((volParms)->vMAttrib & (1L << bHasCatSearch)) != 0)
#define VolHasFileIDs(volParms) \
		(((volParms)->vMAttrib & (1L << bHasFileIDs)) != 0)
#define VolHasBTreeMgr(volParms) \
		(((volParms)->vMAttrib & (1L << bHasBTreeMgr)) != 0)
#define VolHasBlankAccessPrivileges(volParms) \
		(((volParms)->vMAttrib & (1L << bHasBlankAccessPrivileges)) != 0)
#define VolSupportsAsyncRequests(volParms) \
		(((volParms)->vMAttrib & (1L << bSupportsAsyncRequests)) != 0)
#define VolSupportsTrashVolumeCache(volParms) \
		(((volParms)->vMAttrib & (1L << bSupportsTrashVolumeCache)) != 0)

#define VolIsEjectable(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bIsEjectable)) != 0)
#define VolSupportsHFSPlusAPIs(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsHFSPlusAPIs)) != 0)
#define VolSupportsFSCatalogSearch(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsFSCatalogSearch)) != 0)
#define VolSupportsFSExchangeObjects(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsFSExchangeObjects)) != 0)
#define VolSupports2TBFiles(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupports2TBFiles)) != 0)
#define VolSupportsLongNames(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsLongNames)) != 0)
#define VolSupportsMultiScriptNames(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsMultiScriptNames)) != 0)
#define VolSupportsNamedForks(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsNamedForks)) != 0)
#define VolSupportsSubtreeIterators(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsSubtreeIterators)) != 0)
#define VolL2PCanMapFileBlocks(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bL2PCanMapFileBlocks)) != 0)
#define VolParentModDateChanges(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bParentModDateChanges)) != 0)
#define VolAncestorModDateChanges(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bAncestorModDateChanges)) != 0)
#define VolSupportsSymbolicLinks(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bSupportsSymbolicLinks)) != 0)
#define VolIsAutoMounted(volParms) \
		((GetVolParmsInfoExtendedAttributes(volParms) & (1L << bIsAutoMounted)) != 0)

enum
{

	kioACUserAccessMask		= (kioACUserNoSeeFolderMask +
							   kioACUserNoSeeFilesMask +
							   kioACUserNoMakeChangesMask),

	kioACUserFull			= 0x00,
	kioACUserNone			= kioACUserAccessMask,
	kioACUserDropBox		= (kioACUserNoSeeFolderMask +
							   kioACUserNoSeeFilesMask),
	kioACUserBulletinBoard	= kioACUserNoMakeChangesMask
};

#define UserIsOwner(userPrivileges) \
		(((userPrivileges) & kioACUserNotOwnerMask) == 0)
#define UserHasFullAccess(userPrivileges)	\
		(((userPrivileges) & (kioACUserAccessMask)) == kioACUserFull)
#define UserHasDropBoxAccess(userPrivileges)	\
		(((userPrivileges) & kioACUserAccessMask) == kioACUserDropBox)
#define UserHasBulletinBoard(userPrivileges)	\
		(((userPrivileges) & kioACUserAccessMask) == kioACUserBulletinBoard)
#define UserHasNoAccess(userPrivileges)		\
		(((userPrivileges) & kioACUserAccessMask) == kioACUserNone)

OSErr
FSCopyFork(
	SInt16 srcRefNum,
	SInt16 dstRefNum,
	void *copyBufferPtr,
	ByteCount copyBufferSize);

OSErr
FSGetVolParms(
	FSVolumeRefNum volRefNum,
	UInt32 bufferSize,
	GetVolParmsInfoBuffer *volParmsInfo,
	UInt32 *actualInfoSize);

OSErr
FSGetVRefNum(
	const FSRef *ref,
	FSVolumeRefNum *vRefNum);

OSErr
FSGetVInfo(
	FSVolumeRefNum volume,
	HFSUniStr255 *volumeName,
	UInt64 *freeBytes,
	UInt64 *totalBytes);

OSErr
FSGetVolFileSystemID(
	FSVolumeRefNum volume,
	UInt16 *fileSystemID,
	UInt16 *signature);

OSErr
FSGetMountedVolumes(
	FSRef ***volumeRefsHandle,
	ItemCount *numVolumes);

OSErr
FSRefMakeFSSpec(
	const FSRef *ref,
	FSSpec *spec);

OSErr
FSMakeFSRef(
	FSVolumeRefNum volRefNum,
	SInt32 dirID,
	ConstStr255Param name,
	FSRef *ref);

OSStatus
FSMakePath(
	SInt16 vRefNum,
	SInt32 dirID,
	ConstStr255Param name,
	UInt8 *path,
	UInt32 maxPathSize);

OSStatus
FSPathMakeFSSpec(
	const UInt8 *path,
	FSSpec *spec,
	Boolean *isDirectory);

OSErr
UnicodeNameGetHFSName(
	UniCharCount nameLength,
	const UniChar *name,
	TextEncoding textEncodingHint,
	Boolean isVolumeName,
	Str31 hfsName);

OSErr
HFSNameGetUnicodeName(
	ConstStr31Param hfsName,
	TextEncoding textEncodingHint,
	HFSUniStr255 *unicodeName);

Boolean FSRefValid(const FSRef *ref);

OSErr
FSGetParentRef(
	const FSRef *ref,
	FSRef *parentRef);

OSErr
FSGetFileDirName(
	const FSRef *ref,
	HFSUniStr255 *outName);

OSErr
FSGetNodeID(
	const FSRef *ref,
	int32_t *nodeID,
	Boolean *isDirectory);

OSErr
FSGetUserPrivilegesPermissions(
	const FSRef *ref,
	UInt8 *userPrivileges,
	UInt32 permissions[4]);

OSErr
FSCheckLock(
	const FSRef *ref);

OSErr
FSGetForkSizes(
	const FSRef *ref,
	UInt64 *dataLogicalSize,
	UInt64 *rsrcLogicalSize);

OSErr
FSGetTotalForkSizes(
	const FSRef *ref,
	UInt64 *totalLogicalSize,
	UInt64 *totalPhysicalSize,
	ItemCount *forkCount);

OSErr
FSBumpDate(
	const FSRef *ref);

OSErr
FSGetFinderInfo(
	const FSRef *ref,
	FinderInfo *info,
	ExtendedFinderInfo *extendedInfo,
	Boolean *isDirectory);

OSErr
FSSetFinderInfo(
	const FSRef *ref,
	const FinderInfo *info,
	const ExtendedFinderInfo *extendedInfo);

OSErr
FSChangeCreatorType(
	const FSRef *ref,
	OSType fileCreator,
	OSType fileType);

OSErr
FSChangeFinderFlags(
	const FSRef *ref,
	Boolean setBits,
	UInt16 flagBits);

OSErr
FSSetInvisible(
	const FSRef *ref);

OSErr
FSClearInvisible(
	const FSRef *ref);

OSErr
FSSetNameLocked(
	const FSRef *ref);

OSErr
FSClearNameLocked(
	const FSRef *ref);

OSErr
FSSetIsStationery(
	const FSRef *ref);

OSErr
FSClearIsStationery(
	const FSRef *ref);

OSErr
FSSetHasCustomIcon(
	const FSRef *ref);

OSErr
FSClearHasCustomIcon(
	const FSRef *ref);

OSErr
FSClearHasBeenInited(
	const FSRef *ref);

OSErr
FSCopyFileMgrAttributes(
	const FSRef *sourceRef,
	const FSRef *destinationRef,
	Boolean copyLockBit);

OSErr
FSMoveRenameObjectUnicode(
	const FSRef *ref,
	const FSRef *destDirectory,
	UniCharCount nameLength,
	const UniChar *name,
	TextEncoding textEncodingHint,
	FSRef *newRef);

OSErr
FSDeleteContainerContents(
	const FSRef *container);

OSErr
FSDeleteContainer(
	const FSRef *container);

typedef CALLBACK_API( Boolean , IterateContainerFilterProcPtr ) (
	Boolean containerChanged,
	ItemCount currentLevel,
	const FSCatalogInfo *catalogInfo,
	const FSRef *ref,
	const FSSpec *spec,
	const HFSUniStr255 *name,
	void *yourDataPtr);

#define CallIterateContainerFilterProc(userRoutine, containerChanged, currentLevel, catalogInfo, ref, spec, name, yourDataPtr) \
	(*(userRoutine))((containerChanged), (currentLevel), (catalogInfo), (ref), (spec), (name), (yourDataPtr))

OSErr
FSIterateContainer(
	const FSRef *container,
	ItemCount maxLevels,
	FSCatalogInfoBitmap whichInfo,
	Boolean wantFSSpec,
	Boolean wantName,
	IterateContainerFilterProcPtr iterateFilter,
	void *yourDataPtr);

OSErr
FSGetDirectoryItems(
	const FSRef *container,
	FSRef ***refsHandle,
	ItemCount *numRefs,
	Boolean *containerChanged);

OSErr
FSExchangeObjectsCompat(
	const FSRef *sourceRef,
	const FSRef *destRef,
	FSRef *newSourceRef,
	FSRef *newDestRef);

OSErr
FSGetDirAccess(
	const FSRef *ref,
	SInt32 *ownerID,
	SInt32 *groupID,
	SInt32 *accessRights);

OSErr
FSSetDirAccess(
	const FSRef *ref,
	SInt32 ownerID,
	SInt32 groupID,
	SInt32 accessRights);

OSErr
FSGetVolMountInfoSize(
	FSVolumeRefNum volRefNum,
	SInt16 *size);

OSErr
FSGetVolMountInfo(
	FSVolumeRefNum volRefNum,
	void *volMountInfo);

OSErr
FSVolumeMount(
	const void *volMountInfo,
	FSVolumeRefNum *volRefNum);

OSErr
FSMapID(
	FSVolumeRefNum volRefNum,
	SInt32 ugID,
	SInt16 objType,
	Str31 name);

OSErr
FSMapName(
	FSVolumeRefNum volRefNum,
	ConstStr255Param name,
	SInt16 objType,
	SInt32 *ugID);

OSErr
FSCopyFile(
	const FSRef *srcFileRef,
	const FSRef *dstDirectoryRef,
	UniCharCount nameLength,
	const UniChar *copyName,
	TextEncoding textEncodingHint,
	FSRef *newRef);

OSErr
FSMoveRename(
	const FSRef *srcFileRef,
	const FSRef *dstDirectoryRef,
	UniCharCount nameLength,
	const UniChar *moveName,
	TextEncoding textEncodingHint,
	FSRef *newRef);

OSErr
FSResolveFileIDRef(
	FSVolumeRefNum volRefNum,
	SInt32 fileID,
	FSRef *ref);

OSErr
FSCreateFileIDRef(
	const FSRef *ref,
	SInt32 *fileID);

Ptr
GetTemporaryBuffer(
	ByteCount buffReqSize,
	ByteCount *buffActSize);

OSErr
FileRefNumGetFSRef(
	short refNum,
	FSRef *ref);

OSErr
FSSetDefault(
	const FSRef *newDefault,
	FSRef *oldDefault);

OSErr
FSRestoreDefault(
	const FSRef *oldDefault);

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif

