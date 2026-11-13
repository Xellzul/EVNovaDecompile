// MAC-CFLAGS: -mno-dynamic-no-pic -fvisibility=default -DRT3_LIBRARY -std=gnu89
/* rt3_mac.h -- the types, constants and library calls of the mac RT3 sources (librt3_nonag.a is position independent and keeps its symbols extern: the flags line above is the gate's compile line for every file that reaches this header). Notes: Research/notes/rt3mac.md */
#ifndef RT3_MAC_H
#define RT3_MAC_H
#include "int32.h"

typedef uint32_t ULong;
typedef unsigned long long ULongLong;

#define kTextSize 0x100
#define kCodeDigits 12
#define kCodeBits 5
#define kNoDigit 0xff

/* the registration record and the usage record (per product) */
typedef struct RT3License {
    char name[kTextSize];
    char code[kTextSize];
    int32_t copies;
    ULong machine;
    ULong level;
    ULong user;
    ULong reserved[2];
} RT3License;

typedef struct RT3LogEntry {
    char product[0xf0];
    ULongLong tamper[2];
    ULong firstLaunch;
    ULong secondsUsed;
    ULong launches;
    ULong closeStamp;
    ULong saveStamp;
    ULong reserved[2];
} RT3LogEntry;

/* BurgerLib */
struct PackInfo;
struct FTFileInfo;
extern ULong TimerGetUTCSeconds(void);
extern ULong TimerGetElapsedSeconds(void);
extern ULong TimerGetLocalSeconds(void);
extern int32_t TimerGetUTCOffset(int32_t zone);
extern ULongLong TimerGetMicroseconds(void);
extern ULong TimerGetTicks(void);
extern ULong TimerGetTickRate(void);
extern char *StringCopy(char *dst, const char *src);
extern char *StringCopySafe(char *dst, const char *src, ULong size);
extern unsigned char *StringAppendSafe(unsigned char *dst, const unsigned char *src, ULong size);
extern char *StringFromNumber(char *out, ULong value);
extern ULong StringGetLength(const unsigned char *s);
extern int32_t StringCompare(const char *a, const char *b);
extern void MemoryClear(void *p, ULong size);
extern void MemoryCopy(void *dst, const void *src, ULong size);
extern int32_t MemoryChecksum(void *p, ULong size);
extern void *MemoryAllocate(ULong size);
extern void MemoryDeallocate(void *p);
extern void **IndirectAllocate(ULong size);
extern int32_t IndirectDeallocate(void **h);
extern void **IndirectDuplicate(void **h);
extern void *IndirectInitialize(const void *data, ULong size);
extern int32_t IndirectCompress(void **h, ULong tag, struct PackInfo *info);
extern int32_t IndirectDecompress(void **h, ULong tag, struct PackInfo *info);
extern int32_t IndirectAppendData(void **h, const void *data, ULong size);
extern ULong IndirectGetSize(void **h);
extern unsigned char IndirectSetLock(void **h, unsigned char lock);
extern int32_t PlatformOpen(void);
extern int32_t FT_Open(void);
extern int32_t FT_FileGetFlags(int32_t folder, const char *name, struct FTFileInfo *info, int32_t flags);
extern int32_t FT_FileSave(int32_t folder, const char *name, int32_t flags, ULong type, ULong creator, void **data);
extern int32_t FT_FileLoad(int32_t folder, const char *name, ULong maxSize, int32_t flags, void ***data);
extern int32_t FT_FileSetFlags(int32_t folder, const char *name, struct FTFileInfo *info);
extern unsigned char SystemMacOSXOrLater(void);
extern unsigned char SystemRunningAsAdmin(void);
extern unsigned char NetworkStackGetActive(unsigned short which);
extern void SystemSetIdleProc(void (*proc)(void));
extern void *SystemLoadMachOSymbol(const void *framework, const void *name);
extern void *SystemCreateMachOWrapper(void *function);

/* the library's own entry points */
extern char *GetPathToSystemPreferences(const char *name);
extern int32_t WriteToFileAsSuperUser(char *path, const void *data, int32_t size);

extern unsigned char gRT3_EightBitToFiveBitChars[256];
extern void RT3_InitTables(void);
extern unsigned char RT3_CalcTimePeriod(ULong seconds);
extern void RT3_FixProductName(char *out, const char *src);
extern void RT3_FixLicenseName(char *out, const char *src);
extern ULongLong RT3_CalcMainHash(const char *product, const char *name, int32_t copies);
extern void RT3_LicenseCodeToText(ULongLong code, char *text);
extern void RT3_LicenseTextToCode(const char *text, ULongLong *code);

#endif
