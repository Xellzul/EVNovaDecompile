/* burger.h -- the statically linked BurgerLib (a cdecl build) and the runtime band. Notes: Research/notes/burger.md */
#ifndef BURGER_H
#define BURGER_H

#ifdef MACBUILD
typedef char **Handle;
#endif
#include "burgertypes.h"

/* C++ (BurgerLib's own build) takes stdio from MSL and its FILE by name. */
#ifdef __cplusplus
#include <stdio.h>
using std::_FILE;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* memory manager (MMMemoryManager) */
#ifdef MACBUILD
/* the mac engine allocates with the Memory Manager */
#define AllocAPointer(size) NewPtr(size)
#else
extern void * AllocAPointer(Word32 MemSize);
#endif
#ifdef MACBUILD
/* the mac engine frees with the Memory Manager */
#define DeallocAPointer(p) DisposePtr((Ptr)(p))
#else
extern void DeallocAPointer(void *MemPtr);
#endif
extern void * FOpenExpandedPath(const char *path, const char *mode);
#ifdef MACBUILD
#define AllocAPointerClear(size) NewPtrClear(size)
#else
extern void * AllocAPointerClear(uint32_t size);
#endif
extern void * ResizeAPointer(void *mem, uint32_t size);
extern int32_t IndirectDeallocate(void **p);               /* atplatheap.c */

/* the platform heap (atplatheap.c) */
extern void * MemoryAllocate(uint32_t size);
extern void MemoryClear(void *p, uint32_t n);
extern void MemoryDeallocate(void *p);
extern void MemoryCopy(void *dst, const void *src, uint32_t n);
extern void MemoryFill(void *p, int c, uint32_t n);
extern void SystemSetQuitProc(void (*fn)(void));
extern void SystemSetIdleProc(void (*fn)(void));
extern void _SystemQuitCallback(void);
#ifndef MACBUILD /* the mac's Escape Velocity.c calls it undeclared */
extern int32_t PlatformOpen(void);
#endif
extern int32_t FT_Open(void);
extern int32_t SetDisplayToSize(uint32_t w, uint32_t h, uint32_t depth, uint32_t flags);
extern unsigned char SystemCheckCallback(void);
extern void SystemWait(ULong ms);
extern ULong TimerGetLocalSeconds(void);
extern int32_t TimerPrime(void *ref, int32_t period);
extern ULong TimerGetTicks(void);                 /* atticks.c: milliseconds since boot */
extern ULong TimerGetTickRate(void);                 /* atticks.c: the tick rate that goes with it */
extern void *_PlatformGetWindow(void);                 /* atplatopen.c */
extern int32_t _IndirectValidate(void **handle);         /* atplatheap.c: the heap check */
extern unsigned char _AtomicCheckCritical(void);
extern unsigned char _ThreadCheckCritical(void);
extern void * MemoryAllocateClear(uint32_t size);
extern void FastMemSet(void *DestPtr, Word Fill, Word32 Length);
extern void DebugXString(const char *string);

/* the hash table (athashtbl.c) */
struct HashTable;
extern struct HashTable * HashCreate(uint32_t flags);
extern void HashDispose(void *ref);
extern struct HashTable *HashLoad(ULong flags, void **text);
extern int32_t _HashInsertAppend(void *ref, const void *key, ULong keyLen,
    const void *value, ULong valueLen, unsigned char tag, unsigned char front, ULong kind);
extern struct HashTable *HashInitialize(ULong flags, const char *text);
extern struct HashTable *HashInitializeUnicode(ULong flags, const unsigned short *text);
extern char *HashLookupUnicode(void *table, const unsigned short *key, void *after);
extern char *HashIndex(struct HashTable *t, ULong index, char **key, char **value);
extern char *HashIndexUnicode(struct HashTable *t, ULong index, char **key, char **value);
extern void **HashSave(void *ref);
extern struct HashTable *HashClone(void *ref);
extern unsigned char HashIsEmpty(void *ref);
extern char *HashLookup(void *reply, const char *key, void *after);
extern ULong HashGetFlags(void *ref);
extern int32_t HashAppendData(struct HashTable *t, const char *key, const void *value, ULong valueLen);
extern int32_t HashInsertData(struct HashTable *t, const char *key, const void *value, ULong valueLen);
extern int32_t HashAppendUnicodeData(struct HashTable *t, const unsigned short *key, const void *value,
    ULong valueLen);

/* the interface tool's image layer (itblit.c) */
struct ITStream;
struct ITImage;
struct ITPort;
struct ITPixels;
extern int32_t DT_BufferRead(struct ITStream *stream, void *dst, ULong count);
extern struct ITPort *_DT_SurfaceGetContext(void *image);
extern struct ITPort *_DT_ContextGetContext(struct ITImage *image);
extern struct ITPixels *_DT_SurfaceGetImage(void *image);
extern struct ITPixels *_DT_ImageGetImage(struct ITImage *image);
extern struct ITPixels *_DT_ImageGetAlpha(struct ITImage *image);
struct ITDib;
struct ITDevice;
extern int32_t DT_SyncDib(struct ITDib *dib, unsigned char store, unsigned char load);
extern void _DT_ContextDispose(struct ITPort *port);
extern void _DT_PrinterDispose(struct ITDevice *device);
extern void _DT_ImageDispose(struct ITPixels *pixels);
extern void _DT_ImageUnlock(struct ITImage *image, struct ITImage *other);
struct ITPalette;
struct ITRect;
extern struct ITImage *_DT_SurfaceCreate(struct ITPort **portOut, struct ITDevice **deviceOut,
                                    struct ITPixels **pixelsOut);
extern int32_t _DT_SurfaceDispose(struct ITImage *image, unsigned char fromPort,
                         unsigned char fromDevice, unsigned char fromPixels);
extern void _DT_PaletteToInverse(unsigned char *table, const struct ITPalette *palette);
extern int32_t _DT_ImageCalcFormat(ULong format, unsigned char *flag, unsigned char *bits,
                         unsigned char *shift, unsigned char *mask, unsigned short *scale);
extern ULong _DT_ImageCalcStamp(const struct ITPixels *info);
#ifdef MAC1010
extern struct Rect *CopyDT_2SysRect(struct Rect *dst, const struct ITRect *src);
#else
extern struct ITRect *CopyDT_2SysRect(struct ITRect *dst, const struct ITRect *src);
#endif
extern int32_t _DT_ImageLock(struct ITImage *image, struct ITImage *other,
                         unsigned char *rebuilt);
struct ITPixelInfo;
extern int32_t DT_ImageLock(struct ITImage *image, struct ITImage *other,
                         struct ITPixelInfo *info, unsigned char *rebuilt);
extern void DT_ImageUnlock(struct ITImage *image, struct ITImage *other);
extern void _DT_ImageReplace(struct ITImage *a, struct ITImage *b);
extern void _DT_SurfaceReplace(struct ITImage *a, struct ITImage *b);
extern struct ITRect *DT_SetRect(struct ITRect *r, int32_t left, int32_t top, int32_t right,
                                   int32_t bottom);
struct ITTransparent;
struct ITBackground;
struct ITColor;
extern void DT_AlphaInfoDefaults(struct ITTransparent *t);
extern void DT_FileInfoDefaults(struct ITBackground *bg);
extern void DT_ImageSetRawColors(struct ITImage *image, struct ITImage *mask, ULong row,
                         const struct ITColor *src);
extern struct ITImage *DT_ImageCreate(const struct ITRect *bounds, ULong format,
                                    const struct ITPalette *palette);
extern int32_t DT_ImageDispose(struct ITImage *image);
extern void DT_ImageSetRawPixels(struct ITImage *image, struct ITImage *mask, ULong row,
                         const unsigned char *src);
struct PictColorTable;
extern int32_t _PICT_ReadRect(struct ITStream *stream, short *rect);
extern int32_t _PICT_ReadColorTable(struct ITStream *stream, struct PictColorTable *table);
extern struct ITPalette gDT_AlphaPalette;        /* atpalinit.c: the greyscale palette */
extern uint32_t gDT_Mults[9];            /* atpalinit.c: channel width to 16-bit factor */

/* the interface tool's image loaders, one per file format: (stream, image, mask, transparency
   answer, background answer) */
extern int32_t BMP_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern int32_t GIF_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern int32_t JPEG_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern int32_t PNG_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern int32_t PICT_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern int32_t PPAT_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern int32_t TGA_Decode(struct ITStream *stream, struct ITImage **imageOut,
                         struct ITImage **maskOut, struct ITTransparent *trans,
                         struct ITBackground *bg);
extern struct ITRect *DT_ImageGetRect(struct ITImage *image, struct ITRect *bounds);
extern struct ITRect *DT_RectOffset(struct ITRect *r, int32_t dh, int32_t dv);
extern int32_t DT_ImageResize(struct ITImage *image, struct ITImage *mask, const struct ITRect *bounds);
extern unsigned char DT_IsRectEqual(const struct ITRect *a, const struct ITRect *b);
extern int32_t DT_ImageCopy(struct ITImage *dst, const struct ITRect *dstRect, unsigned char copy,
                         struct ITImage *src, struct ITImage *srcMask,
                         const struct ITTransparent *trans, const struct ITRect *srcRect);
struct ITPoint;
extern int32_t DT_ImageTile(struct ITImage *dst, const struct ITRect *dstRect,
                         const struct ITPoint *origin, unsigned char copy, struct ITImage *src,
                         struct ITImage *srcMask, const struct ITTransparent *trans);
extern int32_t DT_ImagePreflightTile(struct ITImage *image, struct ITImage *mask);

/* the REGGIE client (reggie.c) */
struct GlobalEntry;
extern void _ReggiePrivateTickle(void);
extern struct GlobalEntry *_ReggieVersionInsert(const char *name, struct HashTable *reply);

/* string helpers (atplatstr.c) */
extern uint32_t StringGetLength(const unsigned char *s);
extern unsigned char *StringAppendSafe(unsigned char *dst, const unsigned char *src, uint32_t n);
extern void * IndirectInitialize(const void *src, uint32_t size);
extern int32_t StringCompare(const char *a, const char *b);
extern ULong unicode_toupper(ULong c);
extern char *StringFormatSafe(char *out, int32_t size, const char *fmt, ...);
extern char *StringFormat(char *out, const char *fmt, ...);
#ifdef va_start                 /* <stdarg.h> included */
extern char *StringFormatVarg(char *out, ULong size, const char *fmt, va_list args);   /* atformat.c */
#endif
extern int32_t StringCompareSafe(const char *a, const char *b, ULong n);
extern char *StringTokenize(char *s, int delim, char **next);
extern ULong StringToNumber(const char *s);
extern char *StringFindDelimiter(const char *s, const char *set);

/* the mutex-guarded list (atmutexlist.c) and the atomic bit set (atplatlock2.c) */
struct MutexList;
struct MutexNode;
extern int32_t QueueCreate(void *ref);
extern void QueueDispose(struct MutexList *list);
extern void QueueInsert(void *ref, void *element);
extern struct MutexNode *QueueRemove(struct MutexList *list);
extern struct MutexNode *QueueExtract(void *ref, void *element);
extern void QueueTransfer(void *dstRef, void *srcRef);
extern void QueueEmpty(struct MutexList *list);
extern ULong QueueGetCount(void *ref);
extern unsigned char QueueIsEmpty(void *ref);
extern unsigned char AtomicMaskApply(ULong *word, ULong bit);
extern unsigned char AtomicMaskClear(ULong *word, ULong bit);

/* the network layer (atnet.c): on Mac the first argument of a Create names the stack */
#ifdef MAC1010
typedef unsigned short NetStackID;
#else
typedef void *NetStackID;
#endif
/* the Winsock layer (atnet.c) */
struct NetSocket;
struct NetAddress;
struct NetPacket;
extern void NetworkMakeCallback(void (*proc)(struct NetSocket *s, void *ref, unsigned short event),
                         void *ref, struct NetSocket *s, unsigned short event);
extern void NetworkQueueEmpty(struct MutexList *list);
extern void _EndpointUnregister(struct NetSocket *s, unsigned short kind);
extern unsigned char AddressCompare(const void *refA, const void *refB);
extern void DatagramDispose(void *ref);
extern void DatagramClose(void *ref, int32_t unused);
extern int32_t NetworkQueueFill(struct MutexList *list, unsigned short kind, ULong count, ULong size);
extern struct MutexNode *NetworkQueuePoll(struct MutexList *list);
extern void _EndpointRegister(struct NetSocket *s, unsigned short kind);
extern unsigned short AddressToNetworkStack(const struct NetAddress *a);
extern void AddressDuplicate(void *to, const struct NetAddress *src);
extern unsigned char AddressIsEmpty(const struct NetAddress *a);
extern void AddressComposeEmpty(struct NetAddress *a);
extern void AddressComposeTCPIP(void *ref, unsigned short port, ULong ip);
extern void AddressDecomposeTCPIP(const void *ref, unsigned short *port, ULong *ip);
extern void AddressTranscribe(const void *address, char *out);
extern struct NetSocket *DatagramCreate(NetStackID stack, void *ref,
    void (*proc)(struct NetSocket *s, void *ref, unsigned short event),
    struct MutexList *sentList, struct MutexList *freeList, struct MutexList *recvList,
    struct MutexList *recvList2);
extern void DatagramTickle(struct NetSocket *s);
extern int32_t DatagramControl(void *ref, void *out, ULong option, ULong value);
extern int32_t DatagramOpen(void *ref, struct NetAddress *address);
extern int32_t DatagramSend(void *ref, struct NetPacket *packet);
extern int32_t DatagramGetAddress(void *ref, struct NetAddress *out);
extern int32_t _EndpointGetError(struct NetSocket *s);
extern unsigned char NetworkStackGetAvail(unsigned short mode);
extern unsigned char _AddressFromString(struct NetAddress *address, const char *text);
extern struct MutexNode *NetworkQueueWait(struct MutexList *list, ULong seconds);
extern int32_t SimpleResolve(NetStackID stack, const char *host, struct NetAddress *out);

/* the TCP stream socket (atnet.c) */
struct NetStream;
struct NetChunk;
extern struct NetStream *StreamCreate(NetStackID stack, void *ref,
    void (*proc)(struct NetSocket *s, void *ref, unsigned short event),
    struct MutexList *sentList, struct MutexList *freeList, struct MutexList *recvList,
    struct MutexList *recvList2);
extern void StreamTickle(struct NetStream *s);
extern void StreamDispose(struct NetSocket *sock);
extern void StreamClose(struct NetSocket *sock, unsigned char graceful);
extern int32_t StreamAcceptServer(void *ref, void *other);
extern int32_t StreamOpenClient(void *ref, struct NetAddress *local,
    const struct NetAddress *remote, ULong timeout);
extern int32_t StreamSend(void *ref, struct NetChunk *chunk);
extern int32_t StreamGetAddress(void *ref, struct NetAddress *local,
    struct NetAddress *remote);

/* a TCP stream cut into frames (atnet.c) and the socket accessors (reggie.c) */
extern struct NetStream *StreamCreateBuffered(NetStackID stack, void *ref,
    void (*proc)(struct NetSocket *s, void *ref, unsigned short event), int32_t frameMode,
    struct MutexList *sentList, struct MutexList *freeList, struct MutexList *outList,
    struct MutexList *outList2);
extern unsigned short BufferedStreamStatus(struct NetSocket *s, unsigned short state);
extern void _EndpointTickleAll(void);
extern void _EndpointDisposeAll(void);
extern int32_t NetworkStackLoad(ULong wanted);
extern ULong NetworkStackGetAddressList(unsigned short kind, struct NetAddress *out, ULong max);
extern int32_t SimpleSend(struct NetSocket *s, const struct NetAddress *to, void **h);
extern unsigned short EndpointGetElem(void *ref);
extern unsigned short EndpointGetStack(void *ref);
extern unsigned short EndpointGetStatus(void *ref);
extern void EndpointGetCallback(void *handle,
    void (**proc)(struct NetSocket *s, void *ref, unsigned short event), void **ref);
extern void DatagramGetQueues(void *ref, struct MutexList **sentList,
    struct MutexList **freeList, struct MutexList **recvList, struct MutexList **recvList2);
extern void StreamGetQueues(void *ref, struct MutexList **sentList,
    struct MutexList **freeList, struct MutexList **recvList, struct MutexList **recvList2);

/* the host-name resolver (atresolve.c) */
struct NetResolver;
struct NetLookup;
extern struct NetResolver *ResolverCreate(NetStackID stack, void *ref,
    void (*proc)(struct NetSocket *s, void *ref, unsigned short event),
    struct MutexList *doneList, struct MutexList *freeList);
extern void ResolverTickle(struct NetSocket *s);
extern void ResolverDispose(void *ref);
extern int32_t ResolverOpen(void *ref);
extern int32_t ResolverControl(void *ref, struct NetLookup *lookup);
extern void ResolverClose(void *ref);
extern int32_t CriticalDispose(void *lock);
extern ULong _PlatformResumeThread(void *thread);
extern ULong _PlatformSuspendThread(void *thread);
extern int32_t CriticalCreate(void *lock);
extern int32_t AtomicIncrement(int32_t *value, int32_t delta);

/* a handle deflated / inflated through a kept zlib stream (atzstream.c) */
struct ZState;
extern void ProgressiveDispose(struct ZState *state);
extern int32_t ProgressiveDecompress(void **h, struct ZState *state);
extern int32_t ProgressiveCompress(void **h, struct ZState *state);

/* UTF-16 / UTF-8 (atcharset.c, atutf162.c, convertutf.c) */
extern int32_t UnicodeCompare(const unsigned short *a, const unsigned short *b);
extern unsigned short *Unicode2UpperCopySafe(unsigned short *dst, const unsigned short *src,
                                    ULong dstBytes, ULong maxChars);
extern ULong UnicodeGetCharCountFromBytes(const unsigned short *s, ULong bytes);
extern ULong UnicodeGetCharByIndex(const unsigned short *s, ULong index);
extern ULong count_UTF8_to_UTF16(const unsigned char *s, ULong limit);
extern ULong count_UTF16_to_UTF8(const unsigned short *s, ULong limit);
extern unsigned short *convert_UTF8_to_UTF16(unsigned short *dst, const unsigned char *src,
                                    ULong dstBytes, int32_t budget, int32_t terminate);
extern unsigned char *convert_UTF16_to_UTF8(unsigned char *dst, const unsigned short *src,
                                   ULong dstBytes, int32_t budget, int32_t terminate);
/* the converters' character budget and NUL-stop flag: 1.0.10 counts the budget unsigned and takes a signed flag */
#ifdef MAC1010
#define UTFBudget ULong
#define UTFFlags int32_t
#else
#define UTFBudget int32_t
#define UTFFlags ULong
#endif
extern int32_t ConvertUTF32toUTF16(const ULong **sourceStart, const ULong *sourceEnd,
                         unsigned short **targetStart, unsigned short *targetEnd,
                         UTFBudget *budget, UTFFlags flags);
extern int32_t ConvertUTF16toUTF8(const unsigned short **sourceStart, const unsigned short *sourceEnd,
                         unsigned char **targetStart, unsigned char *targetEnd,
                         UTFBudget *budget, UTFFlags flags);
extern int32_t ConvertUTF8toUTF16(const unsigned char **sourceStart, const unsigned char *sourceEnd,
                         unsigned short **targetStart, unsigned short *targetEnd,
                         UTFBudget *budget, UTFFlags flags);

/* lists, images, directories, paths and the prefs file */
struct AudioDesc;
struct DirectorySearch_t;
struct Image_t;
struct ListNode;
struct ListRec;
struct PrefsNode;
struct RezFile;
struct SessionContext;
extern uint32_t GetAPointerSize(void *mem);
#ifndef MAC1010
extern void DetachResource(Handle h);
#endif
extern void LinkedListInit(struct ListRec *list);
extern void LinkedListAddNewEntryEnd(struct ListRec *list, void *data);
extern void *LinkedListGetEntryData(struct ListRec *list, ULong index);
extern void LinkedListAddNewEntryBegin(struct ListRec *list, void *data);   /* prepend */
extern int ImageParseCicn(struct Image_t *out, const Word8 *in);
extern Word ImageParsePict(struct Image_t *ImagePtr, const Word8 *InputPtr);
extern Word ImageParsePPat(struct Image_t *image, const Word8 *input);
extern Word OpenADirectory(struct DirectorySearch_t *dir, const char *name);
extern Word GetADirectoryEntryExtension(struct DirectorySearch_t *dir,
                                        const char *ext);
extern void CloseADirectory(struct DirectorySearch_t *dir);
extern void ImageRemapIndexed(struct Image_t *img, const Word8 *remap);
extern Word PaletteFindColorIndex(const Word8 *pal, Word red, Word green,
                                  Word blue, Word count);
extern int32_t ParseSoundFileImage(struct AudioDesc *info, Word8 *data,
                                ULong size);
extern void ExpandAPathToBuffer(char *dst, const char *path);   /* BurgerLib form */
extern void ExpandAPathToBufferNative(char *dst, const char *path);   /* a Windows path */
extern Word DoesFileExistNative(const char *name);
extern Word GetADirectoryEntry(struct DirectorySearch_t *dir);
extern Word DeleteAFileNative(const char *name);
extern char *GetAVolumeName(Word driveNum);
extern Word FindAVolumeByName(const char *name);
extern Word ResourceGetRezNum(struct RezFile *rez, const char *name);
extern Handle ResourceLoadHandle(struct RezFile *rez, int32_t index);
extern Handle ResourceLoadHandleByName(struct RezFile *rez, const char *name);
extern unsigned int SetAPrefix(unsigned int prefix, const char *path);
extern void PopAPrefix(unsigned int prefix);
extern void *LoadAFile(const char *fileName, uint32_t *length);
extern char *GetFileExtension(const char *name);
extern Handle StrCopyHandle(const char *text);
extern char *StrCopy(const char *text);
extern void StripLeadingAndTrailingWhiteSpace(char *text);
extern void ExtractAString2(const char *src, ULong *size, Word flags, char *dst, ULong dstSize);
extern Word SetErrBombFlag(Word flag);
extern Word BombFlag;
extern char ErrorMsg[512];                       /* burgerfatal.c */
extern Word IAmExiting;                          /* burgerfatal2.c */
extern void DebugXLongWord(Word32 value);
extern char *NumberToAsciiHex(char *dst, ULong value, ULong flags);
extern void *PrefFileNewFromMemory(const char *text, uint32_t size);
extern void ClosePrefsFile(struct PrefsNode *file);
extern void *OpenPrefsFile(const char *name);
extern struct PrefsNode *PrefsFindGroup(struct PrefsNode *file, const char *name);
extern void PrefsGetString(struct PrefsNode *group, const char *key, const char *dflt,
                           char *out, int32_t outSize);
extern int stricmp(const char *s1, const char *s2);   /* MSL Extras */
extern int32_t WritePrefsFile(struct PrefsNode *file, const char *path);

/* runtime-block globals */
extern void *gPlatWindow;
extern uint32_t gSaveLeftVolume;
extern uint32_t gSaveMaxVolume;
extern int32_t MouseClicked;
extern char gPlatCritSec[24];
extern char gCriticalSection[16];
extern unsigned char gSoundToolOpen;
extern struct FileTypeMap **gFT_FileTypes;       /* atftopen.c: the file-type map */
extern Word8 ResourceJustLoaded;                 /* 0x00870e11, set by ResourceLoadHandle */
extern Word32 MaxMemSize;                        /* 0x00576c50, read by InitMemory */
/* handles, spelled void * */
#ifdef MACBUILD
#define AllocAHandle2(size, flag) NewHandle(size)
#else
extern void * AllocAHandle2(Word32 MemSize, Word Flag);
#endif
#ifdef MACBUILD
#define DeallocAHandle(h) DisposeHandle((Handle)(h))
#else
extern void DeallocAHandle(void *MemHandle);
#endif
#ifdef MACBUILD
#define GetAHandleSize(h) GetHandleSize((Handle)(h))
#else
extern uint32_t GetAHandleSize(Handle h);
#endif
extern Handle ResizeAHandle(Handle h, uint32_t size);
#ifdef MACBUILD
/* the mac engine locks with the Memory Manager */
#define LockAHandle(h) HLock((Handle)(h))
#define UnlockAHandle(h) HUnlock((Handle)(h))
#else
extern char * LockAHandle(Handle h);
extern void UnlockAHandle(Handle h);
#endif
typedef void (*MemPurgeProcPtr)(Word Stage);
extern Word PurgeHandles(Word32 MemSize);
extern void CompactHandles(void);
extern void DumpHandles(void);
extern void SetHandlePurgeFlag(void **handle, Word flag);

/* QuickTime calls, linked from qtmlClient.lib */
#ifndef MAC1010
extern void HLock(Handle h);                                         /* 0x004f1470 */
extern void HUnlock(Handle h);
extern void HPurge(Handle h);
#endif
#ifndef MACBUILD
extern void Gestalt(int32_t selector, int32_t *result);
#endif
#ifndef MAC1010
extern short EnterMovies(void);
extern void ExitMovies(void);
#endif
extern void FUN_004fc310(void *gworld);
#ifndef MACBUILD
extern void SetTrackGWorld(void *track, void *movieData, int32_t a3,
                           short (*proc)(void), int32_t a5);
extern int32_t GetTimeBaseFlags(void *timeBase);
extern void SetTimeBaseFlags(void *timeBase, int32_t flags);
#endif

extern ULong RT3_GetDaysHad(void);
extern int32_t RT3_GetLicenseCopies(void);
#if defined(MACBUILD) && !defined(MAC1010)
extern char *RT3_GetLicenseName(void);
#else
extern void *RT3_GetLicenseName(void);
#endif
extern void KeyboardInit(void);
extern void KeyboardDestroy(void);
extern void MouseDestroy(void);
extern void SetDefaultPrefixs(void);
extern void DT_Close(void);
extern void ST_DisposeSound(void *snd);
extern unsigned short ST_IsSoundPlaying(void *snd);

#ifdef MAC1010
extern int32_t RT3_ExtractLicenseBlock1(unsigned long long v);
#elif defined(MACBUILD)
extern ULong RT3_ExtractLicenseBlock1(unsigned long long v);
#else
extern unsigned short RT3_ExtractLicenseBlock1(unsigned long long v);
#endif
extern int32_t DT_Open(void);
extern int32_t IT_Open(void);
extern int32_t IT_OpenBrowser(const char *command);

/* the resource manager (engine/resmgr.c) */
#ifndef MAC1010
extern Handle GetResource(uint32_t type, short id);
#endif
#ifndef MACBUILD
extern int32_t CountResources(uint32_t type);
#endif
#ifndef MAC1010
extern Handle GetIndResource(uint32_t type, short index);
extern void ReleaseResource(Handle h);
extern void GetResInfo(Handle h, short *id, uint32_t *type,
                       unsigned char *name);
#endif

extern unsigned long long TimerGetMicroseconds(void);
extern void LockVideoPage(void);
extern void UnlockVideoPage(void);
extern void ST_HaltSound(void *sound);
extern void PlatformClose(void);
extern void ST_Close(void);
extern void FT_Close(void);
extern void IT_Close(void);
extern uint32_t DRHash64(unsigned long long lic);
extern ULong UFix32_Divide(ULong device, ULong rate);
extern ULong UFix32_Multiply(ULong ratio, ULong rate);
extern unsigned char (*gIndirectBusy)(void *p);
extern unsigned int DoesFileExist(const char *path);
extern char *NumberToAscii(char *dst, ULong value, ULong flags);
extern void OSCursorShow(void);
extern void OSCursorHide(void);
#ifdef MACBUILD
/* the mac engine resets the cursor through the Toolbox */
#define OSCursorReset() InitCursor()
#else
extern void OSCursorReset(void);
#endif
extern unsigned char OSCursorVisibleFlag;        /* burgermisc2.c */
extern void ReleaseVideo(void);
/* a display gamma ramp, each entry 0.0 (black) to 1.0 (full) */
typedef struct GammaTable_t {
    float Red[256];
    float Green[256];
    float Blue[256];
} GammaTable_t;
extern void VideoOSGammaInit(void);
extern void VideoOSGammaDestroy(void);
extern void VideoOSGammaSet(const GammaTable_t *);
extern void VideoOSGammaGet(GammaTable_t *);
extern void Video555To565(void);
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
/* the Monitor Tool's display-mode request (atvideomac.c) */
typedef struct VTRequest {
    ULong sizeRule;
    ULong width;
    ULong height;
    ULong depthRule;
    ULong depth;
    ULong colorRule;
    unsigned char color;
    ULong rateRule;
    ULong rate;
} VTRequest;
extern int32_t MT_Open(void);
extern int32_t MT_GetMode(ULong id, VTRequest *info);
extern int32_t MT_SetMode(ULong id, VTRequest *req);
#endif
extern void DrawShapeLowLevel(Word x, Word y, Word width, Word height, Word skip,
                              void *shape);
extern Word32 LastTick;
#ifndef MAC1010
extern Word32 ReadTick(void);
#endif
extern void WaitTicks(Word tickCount);
#define ResetLastTick() LastTick=ReadTick()

/* BurgerLib's own constants */
#define kNoTlsIndex     0xffffffff
#define kOSFailure      -7000
#define kTableFull      -7001
#define kBadArgument    -7002
#define kUnsupported    -7003
#define kOutOfMemory    -7004
#define kNotFound       -7005
#define kDuplicateKey   -7006
#define kNoDefault      -7400      /* sound: no default output */
#define kNoDevice       -7401      /* sound: no output device */
#define kVolumeFailed   -7403      /* sound: volume not set */
#define kMillisPerSecond 1000
#define kKeyTableSize   0x80
#define kPathBufferSize 0x800
#define kFoldLow        0x61       /* 'a' */
#define kFoldHigh       0x7a       /* 'z' */

/* the file tool: a spec from kSpecIDMin to kSpecIDMax is a folder number, not a handle */
#define kSpecIDMin          -5
#define kSpecIDMax          100
#define IsSpecID(spec)      ((int32_t)(spec) >= kSpecIDMin && (int32_t)(spec) <= kSpecIDMax)

#define Swap32(v)  ((((v) >> 24) & 0xffUL) | (((v) >> 8) & 0xff00UL) \
                    | (((v) & 0xff00UL) << 8) | (((v) & 0xffUL) << 24))
#ifdef MAC1010
#define Swap16(v)  (v)
#else
#define Swap16(v)  ((((v) >> 8) & 0xff) | (((v) & 0xff) << 8))
#endif

/* BurgerLib's file manager: the prefix table, the drive letters */
#define PREFIXMAX           35
#define PREFIXBOOT          32                   /* "*:" */
#define PREFIXPREFS         33                   /* "@:" */
#define PREFIXSYSTEM        34                   /* "$:" */
#define kPrefixCurrent      8
#define kPrefixApp          9
#define kDriveCount         26
#define kNoVolume           ((Word)-1)

/* BurgerLib's memory manager, number and string helpers, fixed-point maths */
#define kHandleFixed        0x40                 /* fixed memory flag */
#define kLeadingZeros       0x8000               /* NumberToAscii flags */
#define kNoTerminator       0x4000
#define kExtractStrDelimitLF 1                   /* ExtractAString2: stop at the end of a line */
#define kFixedShift         16                   /* Fixed32 is 16.16 */
#define kUnityRate          0x10000              /* a sound rate of 1.0 */

/* QuickDraw's Style bits, as the font code of the port and of the interface tool spells them */
#define kFontBold           0x01
#define kFontItalic         0x02
#define kFontUnderline      0x04

/* BurgerLib's sound drivers */
#define kMaxVoices          32
#define kCookieIsVoice      0x40000000           /* a cookie naming a voice slot */
#define kDriverUserMusic    1                    /* BurgerSndExitIn: the music player */
#define kDriverUserSounds   2                    /* BurgerSndExitIn: the sound player */

/* BurgerLib's palette and PixMap code (QuickDraw's rowBytes: bit 15 marks a PixMap) */
#define kPaletteEntries     0x100
#define kPaletteBytes       (kPaletteEntries * 3)
#define kPixMapFlag         0x8000
#define kPixMapFlagMask     0xc000
#define kRowBytesMask       0x3fff

/* MacTypes.h's booleans for C, byte-wide (an enum under -enum min; spelled as macros here
   so that no TU's internal numbering moves) */
#ifndef __cplusplus
#define false ((unsigned char)0)
#define true  ((unsigned char)1)
#endif

/* AmbrosiaTools' AssertMacros.h: the code is copied once, then tested */
#define require_noerr(errorCode, exceptionLabel) \
    do { int32_t evalOnceErrorCode = (errorCode); \
         if (0 != evalOnceErrorCode) goto exceptionLabel; } while (0)

/* video globals */
extern Word VideoColorDepth;
extern uint32_t VideoWidth;
extern char *VideoPointer;
extern uint32_t ScreenWidth;
extern uint32_t ScreenHeight;
extern int32_t ScreenFlags;                         /* burgergraphics.c */
extern void *Win95MainWindow;
extern char **__argv;                            /* MSL setupargs.obj */
extern Word Win95ApplsActive;                    /* burgeronce.c */
extern short Win95DirectSoundVolumes[256];       /* burgeronce.c */
extern unsigned char Win95UseBackBuffer;         /* burgergraphics.c */
extern char *VideoOffscreen;
extern int32_t VideoPageLocked;
extern unsigned char CurrentPalette[0x300];
extern void *BurgerHPalette;
extern void *BurgerHBitMap;

/* the engine's names for BurgerLib's display globals, as the mac build spells them */
#define videoBase VideoPointer
#define gVideoWidth ScreenWidth
#define gVideoHeight ScreenHeight
#define gGamePalette CurrentPalette

/* input globals (burgerinput.c) */
extern int32_t Win95MouseX;
extern int32_t Win95MouseY;
extern Word JoystickPresent;                     /* globaldefs.c */
extern Word32 Win95MouseButton;                  /* burgerkbwin.c */
extern int32_t VideoFullScreen;                     /* burgervideomode.c */
extern int32_t Win95DestScreenX;                    /* burgervideomode.c */
extern int32_t Win95DestScreenY;                    /* burgervideomode.c */

/* sound globals (burgersndwin.c) */
struct DSound;
extern struct DSound *MyDirectSoundDevice;
extern Word MusicVolume;
extern Word SystemState;
extern ULong BurgerVoiceCount;                       /* BurgerVoiceCount */

struct _FILE;
struct adpcm_state;
struct AdpcmVoice;
struct TextBuilder;
struct WindowNode;
struct FTFile;

extern void AtomicEnterCritical(void *token);
extern void AtomicLeaveCritical(void *token);
extern ULongLong TimerGetMilliseconds(void);
extern void SystemIdle(void);
extern int32_t ST_Open(unsigned short channels, ULong device);
extern int32_t fgetstr(char *buf, int32_t size, evn_FILE *stream);
extern void fwritelong(int32_t value, evn_FILE *stream);
extern void fwritestr(const char *s, evn_FILE *stream);
extern int32_t fgetlong(evn_FILE *stream);
extern void *LoadAFileFP(evn_FILE *fp, uint32_t *length);
extern ULong fgetfilesize(evn_FILE *fp);     /* BurgerLib fgetfilesize */
extern int32_t CreateDirectoryPathNative(char *path);            /* BurgerLib CreateDirectoryPathNative */
extern char *ParseQuotedString(char *out, int32_t outSize, const char *src);
extern void MouseSetRange(int32_t width, int32_t height);   /* BurgerLib MouseSetRange */
extern void OpenGLTextureDraw2DSubRect(Word TexNum, int x, int y, Word Width, Word Height,
                                       const float *UVPtr);
extern unsigned char IndirectSetLock(void **handle, unsigned char lock);
extern ULong IndirectGetSize(void **handle);
extern int32_t IndirectSetSize(void **handle, ULong size);
extern char *StringCopySafe(char *dst, const char *src, uint32_t size);
extern char *StringFromNumber(char *out, ULong value);
extern char *String2UpperCopySafe(char *dst, const char *src, ULong n);
extern char *String2LowerCopySafe(char *dst, const char *src, ULong n);
extern char *StringDuplicate(const char *s);
extern char *String2Upper(char *s);
extern char *String2Lower(char *s);
extern char *StringFindString(const char *s, const char *sub);
#ifdef MAC1010
extern unsigned char *StringC2PCopy(unsigned char *dst, const char *src);
extern unsigned char *StringP2C(unsigned char *s);
extern char *StringC2P(char *s);
#endif
extern unsigned char IndirectGetLock(void **handle);
struct ITElement;
extern int32_t IT_ItemHandlerInstall(struct WindowNode *owner, const char *name,
                         int32_t (*proc)(void *hwnd, void *dc, struct ITElement *element,
                                      void **data, ULong msg, ULong a, void *b));

/* the interface tool's settings, window layer, drawing and input, called from its control
   procs (interfacetool.c, ittoolkit.c, itgfx.c, itinput.c) */
struct ITKeyEvent;
struct ITInputEvent;
struct ITClick;
extern unsigned char IT_ItemCallbackIsViewEnabled(void);
extern int32_t IT_ItemCallbackSoundEvent(int32_t event);
extern char *IT_ItemCallbackGetParam(const char *key, unsigned char inherit);
extern int32_t IT_ItemCallbackGetNumber(const char *s);
extern struct ITColor IT_ItemCallbackGetColor(const char *s);
extern unsigned char IT_ItemCallbackGetFlag(const char *s);
extern unsigned char IT_ItemCallbackGetAlign(int32_t *align);
#ifdef MAC1010
extern struct Point *CopyDT_2SysPoint(struct Point *dst, const struct ITPoint *src);
#else
extern struct ITPoint *CopyDT_2SysPoint(struct ITPoint *dst, const struct ITPoint *src);
#endif
extern int32_t _IT_ViewHandleIdle(struct WindowNode *window);
extern int32_t _IT_ViewHandleKey(struct WindowNode *window, const struct ITKeyEvent *key,
                         struct ITClick *click);
extern int32_t _IT_ViewHandleClick(struct WindowNode *window, struct ITInputEvent *event,
                         struct ITClick *click);
extern int32_t _IT_ViewHandleActivate(struct WindowNode *window, unsigned char active);
extern int32_t _IT_ViewHandleCursor(struct WindowNode *window, struct ITInputEvent *event,
                         unsigned short *modifiers);
extern struct ITImage *DT_ContextCreateRaw(void *dc, unsigned char save);
extern int32_t DT_ResetAttributes(struct ITImage *image, unsigned char pen, unsigned char brush,
                         unsigned char font, unsigned char clip, unsigned char origin);
extern int32_t DT_SetLineAttributes(struct ITImage *image, const struct ITColor *fore,
                         const struct ITColor *back, const unsigned char *pattern, int32_t width);
extern unsigned char InputReadMouseState(void *hwnd, struct ITInputEvent *event);
extern struct ITColor gDT_ColorBlack;              /* atpalinit.c: default text colour, black */
extern unsigned char gDT_PatternLtGray[];           /* atpalinit.c: the light grey pattern */
extern unsigned char gDT_PatternMdGray[];           /* atpalinit.c: the 50% grey pattern */
extern unsigned char gDT_PatternBlack[];           /* atpalinit.c: the solid pattern */
extern struct ITColor gDT_ColorWhite;              /* atpalinit.c: white, the default background */
extern struct ITControl *gIT_CurrentItem;        /* ittoolkit.c: the control a proc serves */
extern struct WindowNode *gIT_CurrentView;       /* ittoolkit.c: the window a proc serves */
extern void *gIT_DefaultHash;                    /* atcursor.c */
extern struct WindowNode *gIT_ViewList;          /* atcursor.c */

/* the interface tool's image loader, drawing, rectangles, window layer, markup tree and
   attributes, called across itimage.c, itgfx.c, itrect.c, ittoolkit.c, interfacetool.c,
   itmarkup.c, itattr.c and the control procs; charset and folder lookups from atcharset.c and
   atfilespec.c */
struct ITControl;
struct ITPage;
struct FolderSpec;
extern int32_t DT_ImageFileLoad(void **handle, struct ITImage **imageOut, struct ITImage **maskOut,
                         struct ITTransparent *trans, struct ITBackground *bg);
extern void *DT_PeekAtSysContext(struct ITImage *image);
extern int32_t DT_ContextDispose(struct ITImage *image);
extern int32_t DT_GetFillBackColor(struct ITImage *image, struct ITColor *back);
extern int32_t DT_SetFillBackColor(struct ITImage *image, const struct ITColor *back);
extern void *DT_GetClipRegion(struct ITImage *image);
extern int32_t DT_SetClipRegion(struct ITImage *image, void *region);
extern int32_t DT_SetClipRect(struct ITImage *image, const struct ITRect *r);
extern int32_t DT_AddInvalidRect(struct ITImage *image, const struct ITRect *r);
#ifdef MAC1010
extern int32_t DT_DrawImageTile(struct ITImage *image, const struct ITRect *clip,
                         const struct ITPoint *origin, struct ITImage *src,
                         struct ITImage *mask, const struct ITTransparent *trans,
                         const struct ITRect *srcRect);
#else
extern int32_t DT_DrawImageTile(struct ITImage *image, const struct ITRect *clip,
                         const struct ITPoint *origin, struct ITImage *src,
                         struct ITImage *mask, int32_t unused, const struct ITRect *srcRect);
#endif
extern int32_t DT_EraseRect(struct ITImage *image, const struct ITRect *r);
extern unsigned char DT_IsPointInRect(const struct ITPoint *pt, const struct ITRect *r);
extern unsigned char DT_IsRectEmpty(const struct ITRect *r);
extern struct ITRect *DT_RectInset(struct ITRect *r, int32_t dh, int32_t dv);
extern struct ITRect *DT_RectIntersect(struct ITRect *r, const struct ITRect *a,
                                   const struct ITRect *b);
extern void DT_RegionDispose(void *region);
#ifdef MAC1010
#define DTSysRect struct Rect      /* what CopySys2DT_Rect reads */
extern struct RGBColor DT_2SysColor(const struct ITColor *color);
extern struct ITPoint *CopySys2DT_Point(struct ITPoint *dst, const struct Point *src);
extern struct ITRect *CopySys2DT_Rect(struct ITRect *dst, const struct Rect *src);
extern struct ITColor *CopySys2DT_Color(struct ITColor *color, const struct RGBColor *ref);
#else
#define DTSysRect struct ITRect
extern ULong DT_2SysColor(const struct ITColor *color);
extern struct ITPoint *CopySys2DT_Point(struct ITPoint *dst, const struct ITPoint *src);
extern struct ITRect *CopySys2DT_Rect(struct ITRect *dst, const struct ITRect *src);
extern struct ITColor *CopySys2DT_Color(struct ITColor *color, const ULong *ref);
#endif
extern int32_t IT_ViewChange(struct WindowNode *window, struct FolderSpec **folder, const char *file,
                         const char *path, const char *anchor);
extern int32_t IT_ViewDispose(struct WindowNode *window);
extern int32_t _IT_ViewHandleTest(struct WindowNode *window, ULong item, unsigned char *hit);
extern int32_t _IT_ViewHandleDraw(struct WindowNode *window, const struct ITRect *area,
                         unsigned char noBlit);
extern int32_t _IT_ViewSetFocus(struct WindowNode *window, struct ITControl *control, uint32_t item,
                         unsigned char keep);
extern int32_t _IT_ViewMoveFocus(struct WindowNode *window, unsigned char forward,
                         struct WindowNode *skipWindow, struct ITControl *skipControl,
                         ULong only);
extern int32_t _IT_ViewQueryFocus(struct WindowNode *window, void **out);
extern int32_t _IT_ViewValidate(struct WindowNode *window);
extern int32_t _IT_ViewCallback(struct WindowNode *window, struct ITPage *page,
                         struct ITControl *control, int32_t event);
extern int32_t _IT_ViewDissectLink(struct WindowNode *window, const char *link, char **file,
                         char **anchor);
extern int32_t _IT_ItemCallback(struct WindowNode *window, struct ITControl *control, ULong msg,
                         ULong a, void *b);
extern int32_t IT_ViewSetItemEnabled(void *windowRef, int32_t index, unsigned char enable);
extern struct ITElement *PT_ParseTreeInsertNode(struct ITElement *root, struct ITElement *where,
                                      struct ITElement *node, unsigned char before);
extern struct ITElement *PT_ParseTreeGetParent(struct ITElement *root, struct ITElement *node);
extern struct ITElement *PT_ParseTreeGetNext(struct ITElement *root, struct ITElement *node);
extern void PT_ParseTreeDispose(struct ITElement *e);
extern struct ITElement *_IT_ParseTreeFindNextTag(struct ITElement *root, struct ITElement *start,
                                      const char *name, unsigned char skip);
extern struct ITElement *_IT_ParseTreeFindLastNode(void *doc, struct ITElement *element);
extern char *_IT_ParseTreeFindAttribute(struct ITElement *root, struct ITElement *element, const char *tag,
                          const char *key, unsigned char inherit);
extern char *_IT_SimpleFindAttribute(struct ITElement *element, const char *key);
extern int32_t _IT_ParseAttributeGetNumber(unsigned char wide, const char *s);
extern struct ITColor _IT_ParseAttributeGetColor(unsigned char wide, const char *s);
#ifdef MAC1010
extern void _IT_ParseAttributeGetRect(void *rect, unsigned char wide, char *s);
#else
extern struct ITRect *_IT_ParseAttributeGetRect(void *rect, unsigned char wide, const char *s);
#endif
#ifdef MAC1010
extern void _IT_PageListDispose(void *tableRef);
#else
extern int32_t _IT_PageListDispose(void *tableRef);
#endif
extern unsigned char IndirectIsUnicode(void **handle, char *nameOut);
extern int32_t FT_PathDispose(struct FolderSpec **spec);

/* called by the nag screen (rt3nag.c) */
extern void *FT_GetResFile(void);                 /* atfilespec.c */
extern int32_t FT_ResourceLoad(void *module, unsigned short id, const char *resType, ULong macType,
                         ULong maxSize, int32_t unused, void ***handle);
extern int32_t FT_VirtualPathMount(void **handle, unsigned char mode, struct FolderSpec ***result);
extern unsigned char DT_IsOpen(void);         /* atpalinit.c */
extern unsigned char IT_IsOpen(void);         /* atcursor.c */
extern ULong RT3_GetHoursUsed(void);             /* rt3.c */
extern int32_t IT_ViewCreateModal(void *folder, const char *file, const char *path,
                         int32_t (*proc)(struct WindowNode *window, int32_t refcon, int32_t event,
                                      void *where, int32_t index, const char *name),
                         int32_t refcon, char *result);   /* ittoolkit.c */
extern int32_t IT_ViewGetNamedItem(struct WindowNode *window, const char *name);   /* ittoolkit.c */
extern int32_t IT_ViewSetItemVisible(void *windowRef, int32_t index, unsigned char show);

/* the interface tool's control procs, registered by name (atwndclass.c) */
extern int32_t IT_ButtonHandler(void *hwnd, void *dc, struct ITElement *element, void **data,
                         ULong msg, ULong a, void *b);       /* BUTTON, IMAGE, IMG: ittoolkit.c */
extern int32_t IT_BorderHandler(void *hwnd, void *dc, struct ITElement *element, void **data,
                         ULong msg, ULong a, void *b);       /* BORDER: itborder.c */
extern int32_t IT_FrameHandler(void *hwnd, void *dc, struct ITElement *element, void **data,
                         ULong msg, ULong a, void *b);       /* FRAME: itframe.c */
extern int32_t IT_PopupHandler(void *hwnd, void *dc, struct ITElement *element, void **data,
                         ULong msg, ULong a, void *b);       /* POPUP: itpopup.c */
extern int32_t IT_TextHandler(void *hwnd, void *dc, struct ITElement *element, void **data,
                         ULong msg, ULong a, void *b);       /* TEXTBOX: ittextbox.c */
extern int32_t _FT_FileRead(struct FTFile *file, ULong count, void *buffer);
extern int32_t _FT_FileWrite(struct FTFile *file, ULong count, const void *buffer);
extern int32_t _FT_FileGetEOF(struct FTFile *file, ULong *size);
extern int32_t _FT_FileSetEOF(struct FTFile *file, ULong size);
extern int32_t _FT_FileGetMark(struct FTFile *file, ULong *position);
extern int32_t _FT_FileSetMark(struct FTFile *file, ULong position);
extern int32_t _FT_ChunkRead(struct FTFile *file, ULong count, void *buffer);
extern int32_t _FT_ChunkWrite(struct FTFile *file, ULong count, const void *buffer);
extern int32_t _FT_ChunkGetEOF(struct FTFile *file, ULong *size);
extern int32_t _FT_ChunkSetEOF(struct FTFile *file, ULong size);
extern int32_t _FT_ChunkGetMark(struct FTFile *file, ULong *position);
extern int32_t _FT_ChunkSetMark(struct FTFile *file, ULong position);
extern int32_t _FT_BufferRead(struct FTFile *file, ULong count, void *buffer);
extern int32_t _FT_BufferWrite(struct FTFile *file, ULong count, const void *buffer);
extern int32_t _FT_BufferGetEOF(struct FTFile *file, ULong *size);
extern int32_t _FT_BufferSetEOF(struct FTFile *file, ULong size);
extern int32_t _FT_BufferGetMark(struct FTFile *file, ULong *position);
extern int32_t _FT_BufferSetMark(struct FTFile *file, ULong position);
extern struct ControlType *gIT_HandlerList;
extern void StreamHandleInitPut(struct TextBuilder *b);
extern void StreamHandleDestroy(struct TextBuilder *b);
extern void StreamHandleEndSave(struct TextBuilder *b);
extern void StreamHandlePutByte(struct TextBuilder *b, int32_t ch);
extern void StreamHandlePutStringNoZero(struct TextBuilder *b, const char *s);
extern void FastMemSet16(void *dst, ULong value, ULong bytes);
extern uint32_t crc32(uint32_t crc, const unsigned char *buf,
                           unsigned len);
extern void adpcm_coder(short indata[], char outdata[], int len,
                        struct adpcm_state *state);
extern unsigned char ADPCM_Mixer(ULong *data, short **out, ULong frames, ULong step,
                                  struct AdpcmVoice *voice, unsigned short left,
                                  unsigned short right);

extern unsigned char gPlatformOpen;
extern unsigned char gClockReadyFlag;
extern int32_t gNumVoices;
extern ULong gSaveRightVolume;
extern ULong gSampleRate;
extern ULong BurgerSndExitIn;                    /* burgerfile.c */
extern int32_t BurgerSndKillProcs[3];               /* burgerfile.c */

extern unsigned char _ST_SupportsSystemVolume(void);
extern int32_t _ST_GetSystemVolume(ULong *left, ULong *right, ULong *max);
extern ULong UFix32_To_UInt32(ULong rate);
extern void CriticalEnter(void *lock);
extern void CriticalLeave(void *lock);
extern void _ReggieFixProductName(char *out, const char *src);
extern int32_t HashInsert(struct HashTable *t, const char *key, const char *value);
extern int32_t HashAppend(struct HashTable *t, const char *key, const char *value);
extern unsigned char HashRemove(struct HashTable *t, const char *key);
struct WindowNode;
extern int32_t IT_ViewParamText(struct WindowNode *window, const char *key, const char *value);
extern ULong RT3_GetNumLaunches(void);
extern int32_t IT_SetCursor(short cursorId);
extern void MemoryMove(void *dst, const void *src, ULong n);
extern int32_t IndirectAppendData(void **handle, const void *data, ULong length);
extern int32_t IndirectReplaceData(void **handle, const void *data, ULong length);
extern int32_t IndirectAppend(void **handle, void **from);
extern int32_t IndirectReplace(void **handle, void **from);
extern int32_t IndirectInsertData(void **handle, const void *data, ULong length);
extern int32_t IndirectSpliceData(void **handle, ULong offset, ULong oldLength, const void *data,
                         ULong newLength);
extern ULong UnicodeGetByteCount(const unsigned short *s);
extern const unsigned short *UnicodeAdvance(const unsigned short *s, ULong *code);
extern unsigned short *C2UStringCopySafe(unsigned short *dst, const unsigned char *src,
                                    ULong dstBytes, ULong maxChars);
extern char UnicodePoint2Char8(ULong code);
extern char *StringCopy(char *dst, const char *src);

/* StringCopy's body, inlined where the exe copies a name byte by byte (atnet.c, atresolve.c) */
static inline char *CopyString(char *dst, const char *src)
{
    char *start;

    start = dst;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}

extern void **IndirectDuplicate(void **handle);
extern ULong TimerGetUTCSeconds(void);
extern uint32_t TimerGetElapsedSeconds(void);
extern int32_t _RT3_LicenseRefresh(void);
extern struct RT3License gRT3_License;
extern struct RT3LogEntry gLogEntry;
extern char gPrivateLicenseName[256];
extern char gPrivateProductName[256];
extern char gDisplayProductName[256];
extern unsigned char gRT3_Open;
extern const void *gRT3_IsRegisteredPtr1;        /* atplatopen.c, license data */
struct FTFileInfo;
typedef struct PackInfo {       /* filled by the handle packers when not NULL */
    ULong magic;                /* 'sqsh' */
    ULong tag;                  /* 'zlib', 'pack', 'PACK' or 'none' */
    ULong size;                 /* unpacked size and checksum */
    ULong sum;
    ULong packedSize;
    ULong packedSum;
    ULong reserved[2];
} PackInfo;
extern int32_t IndirectDecompress(void **handle, ULong tag, PackInfo *info);
extern int32_t IndirectCompress(void **handle, ULong tag, PackInfo *info);
/* 1.0.10 passes the get-flags and save flag in a byte */
#ifdef MAC1010
#define FTFlagArg unsigned char
#else
#define FTFlagArg int32_t
#endif
extern int32_t FT_FileGetFlags(int32_t folder, const char *name, struct FTFileInfo *info, FTFlagArg flags);
extern int32_t FT_FileLoad(int32_t folder, const char *name, ULong maxSize, int32_t flags,
                         void ***handle);
extern int32_t FT_FileSave(int32_t folder, const char *name, FTFlagArg flags, ULong type,
                         ULong creator, void **handle);
extern int32_t FT_FileDelete(int32_t folder, const char *name);
struct FileTypeMap;
extern void _FT_FileTypesLookup(const char *name, ULong type, struct FileTypeMap *entry);
extern unsigned char AtomicMaskTest(ULong *word, ULong bit);
extern void **IndirectAllocateTempMem(ULong size);
extern int32_t PT_ParseEncode(void **h, unsigned char wide, unsigned char url,
                         unsigned char spaces, unsigned char quotes);
extern unsigned char IsPlatformOpen(void);
extern unsigned char _FT_IsIndirectBusy(void *p);
extern void **IndirectAllocate(ULong size);
#ifndef MAC1010
extern void GetDateTime(uint32_t *seed);
#endif
extern void ResourceDestroy(struct RezFile *rez);
extern float FUN_004eed20(float lenSquared);
extern int32_t MemoryChecksum(void *data, uint32_t size);
extern char *ConvertNativePathToPath(const char *path);
extern void *LoadAFileHandle(const char *path);
extern void **AllocAHandleClear(ULong size);
extern void _PlatformSetWindow(void *window);
#ifdef MAC1010
extern uint32_t RT3_IsRegistered(void);
#else
extern int32_t RT3_IsRegistered(void);
#endif
extern void RT3_Idle(void);
#ifdef MAC1010
extern unsigned char NetworkStackGetActive(unsigned short mode);
#else
extern char NetworkStackGetActive(unsigned short mode);
#endif
extern int32_t ReggieVersionBegin(struct SessionContext *ctx, const char *name);
extern void VideoGetCurrentMode(int32_t *out);
extern int32_t VideoSetMode(int32_t *rec);
extern void DrawARect16(int x, int y, Word Width, Word Height, Word Color);
extern void KeyboardFlush(void);
extern int32_t CreateDirectoryPath(const char *path);
extern int32_t Win95Allow1Instance(const char *name);
extern void KillSoundPlayer(void);
extern void JoystickDestroy(void);
extern void Fatal(const char *fmt, ...);
extern void NonFatal(const char *fmt, ...);
#if defined(__MWERKS__) && !defined(MACBUILD) && (!defined(__cplusplus) || !defined(_MSL_CSTRING))
extern void *memmove(void *dst, const void *src, unsigned int n);
extern void *memcpy(void *dst, const void *src, unsigned int n);
#endif

/* Burger.h maps strlen onto the library's word-at-a-time FastStrLen (burgerpow2.c) */
extern Word FastStrLen(const char *Input);
#ifndef MACBUILD
#undef strlen
#define strlen(x) FastStrLen(x)
#endif

#ifndef MACBUILD
/* the guarded copy the file and sound tools inline (atfilespec.c, atsndmem.c) */
static inline void BlockMoveData(const void *src, void *dst, ULong size)
{
    if (size != 0)
        memmove(dst, src, size);
}
#endif
#ifdef MACBUILD
#ifdef MAC1010
extern short OpenEVPlugins(short refNum, ConstStr255Param folderName, OSType type, evn_FILE *log);
#else
extern short OpenEVPlugins(FSRef folder, OSType type);
#endif
#else
extern short OpenEVPlugins(const char *path, const char *ext,
                           evn_FILE *log);
#endif
#if defined(__MWERKS__) && !defined(MACBUILD) && !defined(__cplusplus)
extern int fclose(evn_FILE *file);
extern unsigned int fread(void *ptr, unsigned int size, unsigned int count,
                          evn_FILE *stream);
extern unsigned int fwrite(const void *ptr, unsigned int size, unsigned int count,
                           evn_FILE *stream);
#endif
#if defined(__MWERKS__) && !defined(MACBUILD) && (!defined(__cplusplus) || !defined(_MSL_CSTDLIB))
extern int atexit(void (*fn)(void));
#endif
extern void ImageDestroy(struct Image_t *ImagePtr);
extern void ImageDelete(struct Image_t *ImagePtr);
extern unsigned int KeyboardKbhit(void);
extern unsigned int KeyboardGetch(void);
extern void KeyboardCallPollingProcs(void);
extern void ProcessSystemEvents(void);
extern void KeyboardAddRoutine(void (*proc)(void *), void *data);
extern void KeyboardRemoveRoutine(void (*proc)(void *), void *data);
extern void KillDigitalDriver(void);
extern Word32 PowerOf2(Word32 Input);
extern void DigitalMusicPlay(Word SongNum);
extern void DigitalMusicShutdown(void);

/* the song decoders' front end (burgerdigsong.c) */
struct DigitalMusicReadState_t;
extern Word DigitalMusicGetSilenceVal(Word32 Format);
extern Word32 DigitalMusicDecode(struct DigitalMusicReadState_t *State, void *Buffer,
                                 Word32 Length);
extern Word8 DigitalMusicReadStateInit(struct DigitalMusicReadState_t *State,
                                       struct AudioDesc *Desc, const Word8 *Image,
                                       Word32 BufferSize,
                                       Word32 (*ReadProc)(void *, Word8 *, Word32),
                                       void *ReadData);
extern void DigitalMusicReadStateDestroy(struct DigitalMusicReadState_t *State);
extern void DigitalMusicReset(struct DigitalMusicReadState_t *State);

/* the sample expanders the song decoders call (burgermace.c, burgermsadpcm.c) and the
   8-bit law tables (burgerulaw.c, burgeralaw.c) */
struct MACEState_t;
struct ADPCMUnpackState_t;
extern void MACEExp1to6(const Word8 *InBufPtr, Word8 *OutBufPtr, Word32 Count,
                        struct MACEState_t *InStatePtr, struct MACEState_t *OutStatePtr,
                        Word NumChannels, Word WhichChannel);
extern void MACEExp1to3(const Word8 *InBufPtr, Word8 *OutBufPtr, Word32 Count,
                        struct MACEState_t *InStatePtr, struct MACEState_t *OutStatePtr,
                        Word NumChannels, Word WhichChannel);
extern Word ADPCMDecodeBlock(struct ADPCMUnpackState_t *StatePtr);
extern const short UnpackULawTable[256];
extern const short UnpackALawTable[256];

/* the read-ahead file stream (burgerrswin.c) */
struct ReadFileStream_t;
extern struct ReadFileStream_t *ReadFileStreamNew(const char *FileName, Word32 ChunkCount,
                                                  Word32 ChunkSize);
extern void ReadFileStreamDelete(struct ReadFileStream_t *Input);
extern Word ReadFileStreamActive(struct ReadFileStream_t *Input);
extern void ReadFileStreamStop(struct ReadFileStream_t *Input);
extern void ReadFileStreamStart(struct ReadFileStream_t *Input, Word32 Offset);
extern Word8 *ReadFileStreamGetData(struct ReadFileStream_t *Input, Word32 *LengthPtr,
                                    Word32 Length);
extern void StopASound(ULong cookie);
extern unsigned int OSCursorIsVisible(void);
extern void CStr2PStr(char *dst, const char *src);
extern int32_t TimerGetSeconds(void);
extern int32_t ST_PlaySound(void *snd, unsigned short priority, unsigned short level);
extern ULong ST_PlaySoundParam(SoundRequest *req);

extern unsigned char KeyArray[128];
extern unsigned char *DAT_00576ce8;
extern int32_t gGDIDIBSectionFunc;

#ifdef __cplusplus
}

/* W9Win95Code's application instance, read by OCWin95Code; set nowhere in this exe. */
namespace Burger {
class Win32 {
public:
    static void *m_pInstance;
};
}
#endif

#endif
