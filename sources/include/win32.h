/*
 * win32.h -- the Win32 imports the port and the runtime call directly.
 * The original built against <windows.h>; we cannot, so these are the
 * hand-written prototypes, one home per name. Plain types only (no ULong,
 * no Mac types) so any TU can include this without pulling a subsystem in.
 */
#ifndef WIN32_H
#define WIN32_H

#ifdef __cplusplus
extern "C" {
#endif

/* user32 window messages */
#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_MOVE             0x0003
#define WM_SIZE             0x0005
#define WM_ACTIVATE         0x0006
#define WM_SETFOCUS         0x0007
#define WM_KILLFOCUS        0x0008
#define WM_GETTEXTLENGTH    0x000e
#define WM_PAINT            0x000f
#define WM_CLOSE            0x0010
#define WM_QUIT             0x0012
#define WM_ERASEBKGND       0x0014
#define WM_ENDSESSION       0x0016
#define WM_ACTIVATEAPP      0x001c
#define WM_SETCURSOR        0x0020
#define WM_GETMINMAXINFO    0x0024
#define WM_SETFONT          0x0030
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_DEADCHAR         0x0103
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_SYSCHAR          0x0106
#define WM_SYSDEADCHAR      0x0107
#define WM_INITDIALOG       0x0110
#define WM_COMMAND          0x0111
#define WM_SYSCOMMAND       0x0112
#define WM_TIMER            0x0113
#define WM_CTLCOLORMSGBOX   0x0132
#define WM_CTLCOLOREDIT     0x0133
#define WM_CTLCOLORLISTBOX  0x0134
#define WM_CTLCOLORBTN      0x0135
#define WM_CTLCOLORDLG      0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC   0x0138
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_MOUSEWHEEL       0x020a
#define WM_PRINT            0x0317
#define WM_QUERYNEWPALETTE  0x030f
#define BM_SETSTATE         0x00f3
#define PRF_CHECKVISIBLE    0x00000001
#define PRF_CLIENT          0x00000004
#define PRF_ERASEBKGND      0x00000008

/* user32 window styles, virtual keys and other constants */
#define WS_POPUP            0x80000000UL
#define WS_CHILD            0x40000000UL
#define WS_VISIBLE          0x10000000UL
#define WS_CAPTION          0x00c00000UL
#define WS_BORDER           0x00800000UL
#define WS_SYSMENU          0x00080000UL
#define WS_MINIMIZEBOX      0x00020000UL
#define WS_TABSTOP          0x00010000UL
#define SW_HIDE             0
#define SW_SHOW             5
#define GWL_HINSTANCE       (-6)
#define IDC_ARROW           32512
#define IDC_HAND            32649
#define VK_LBUTTON          0x01
#define VK_RBUTTON          0x02
#define VK_MBUTTON          0x04
#define VK_RETURN           0x0d
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_CAPITAL          0x14
#define VK_LWIN             0x5b
#define VK_RWIN             0x5c
#define VK_NUMLOCK          0x90
#define VK_SCROLL           0x91

/* gdi32 */
#define CLR_INVALID         0xffffffffUL
#define BLACK_BRUSH         4
#define WHITE_BRUSH         0
#define BITSPIXEL           12
#define LOGPIXELSX          88
#define LOGPIXELSY          90
#define FW_NORMAL           400
#define FW_BOLD             700
#define SRCCOPY             0x00cc0020UL
#define SRCAND              0x008800c6UL
#define SRCPAINT            0x00ee0086UL
#define ROP_DSNA            0x00220326UL    /* dest AND NOT source */

/* kernel32 files */
#define GENERIC_READ        0x80000000UL
#define OPEN_EXISTING       3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_BEGIN          0
#define INVALID_HANDLE_VALUE ((void *)-1)
#define FILE_CURRENT        1
#define GENERIC_WRITE       0x40000000UL
#define FILE_SHARE_READ     0x00000001
#define CREATE_NEW          1
#define FILE_ATTRIBUTE_READONLY 0x0001
#define FILE_ATTRIBUTE_HIDDEN 0x0002
#define FILE_ATTRIBUTE_OFFLINE 0x1000
#define SEM_NOOPENFILEERRORBOX 0x8000

/* kernel32 error codes (winerror.h) */
#define ERROR_SUCCESS                   0
#define ERROR_FILE_NOT_FOUND            2
#define ERROR_PATH_NOT_FOUND            3
#define ERROR_TOO_MANY_OPEN_FILES       4
#define ERROR_ACCESS_DENIED             5
#define ERROR_INVALID_HANDLE            6
#define ERROR_NOT_ENOUGH_MEMORY         8
#define ERROR_INVALID_ACCESS            12
#define ERROR_OUTOFMEMORY               14
#define ERROR_INVALID_DRIVE             15
#define ERROR_CURRENT_DIRECTORY         16
#define ERROR_NO_MORE_FILES             18
#define ERROR_WRITE_PROTECT             19
#define ERROR_BAD_UNIT                  20
#define ERROR_SEEK                      25
#define ERROR_SECTOR_NOT_FOUND          27
#define ERROR_SHARING_VIOLATION         32
#define ERROR_LOCK_VIOLATION            33
#define ERROR_HANDLE_EOF                38
#define ERROR_HANDLE_DISK_FULL          39
#define ERROR_REM_NOT_LIST              51
#define ERROR_DUP_NAME                  52
#define ERROR_BAD_NETPATH               53
#define ERROR_DEV_NOT_EXIST             55
#define ERROR_FILE_EXISTS               80
#define ERROR_CANNOT_MAKE               82
#define ERROR_OUT_OF_STRUCTURES         84
#define ERROR_DISK_CHANGE               107
#define ERROR_DRIVE_LOCKED              108
#define ERROR_OPEN_FAILED               110
#define ERROR_DISK_FULL                 112
#define ERROR_NO_MORE_SEARCH_HANDLES    113
#define ERROR_INVALID_NAME              123
#define ERROR_NO_VOLUME_LABEL           125
#define ERROR_NEGATIVE_SEEK             131
#define ERROR_SEEK_ON_DEVICE            132
#define ERROR_DIR_NOT_ROOT              144
#define ERROR_DIR_NOT_EMPTY             145
#define ERROR_PATH_BUSY                 148
#define ERROR_BAD_PATHNAME              161
#define ERROR_BUSY                      170
#define ERROR_ALREADY_EXISTS            183
#define ERROR_FILE_INVALID              1006
#define ERROR_TOO_MANY_DESCRIPTORS      1158
#define ERROR_NO_VOLUME_ID              1173
#define ERROR_CANCELLED                 1223

/* shell32 special folders, advapi32 tokens */
#define CSIDL_PERSONAL          0x0005
#define CSIDL_APPDATA           0x001a
#define CSIDL_INTERNET_CACHE    0x0020
#define CSIDL_COMMON_APPDATA    0x0023
#define CSIDL_FLAG_CREATE       0x8000
#define TOKEN_QUERY             0x0008

/* kernel32 threads and waits, wsock32 */
#define CREATE_SUSPENDED    0x00000004
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2
#define THREAD_PRIORITY_TIME_CRITICAL 15
#define INFINITE            0xffffffff
#define WAIT_FAILED         0xffffffffUL
#define WAIT_TIMEOUT        0x102
#define AF_INET             2
#define SOCK_DGRAM          2

/* Win32's RECT: four longs in left/top/right/bottom order, which is what
   DirectDraw's Blt and the window calls expect. Not the Mac Rect. */
typedef struct WinRect {
    long left;
    long top;
    long right;
    long bottom;
} WinRect;

/* Win32's WNDCLASSA, spelled the way the SDK spells it because that is what a
   caller of RegisterClassA would have written. */
typedef struct WNDCLASSA {
    unsigned long          style;                    /* +0x00 */
    void                   *lpfnWndProc;             /* +0x04 */
    long                   cbClsExtra;               /* +0x08 */
    long                   cbWndExtra;               /* +0x0c */
    void                   *hInstance;               /* +0x10 */
    void                   *hIcon;                   /* +0x14 */
    void                   *hCursor;                 /* +0x18 */
    void                   *hbrBackground;           /* +0x1c */
    const char             *lpszMenuName;            /* +0x20 */
    const char             *lpszClassName;           /* +0x24 */
} WNDCLASSA;

typedef struct OPENFILENAMEA {
    unsigned long lStructSize;      /* +0x00 */
    void *hwndOwner;                /* +0x04 */
    void *hInstance;                /* +0x08 */
    const char *lpstrFilter;        /* +0x0c */
    char *lpstrCustomFilter;        /* +0x10 */
    unsigned long nMaxCustFilter;   /* +0x14 */
    unsigned long nFilterIndex;     /* +0x18 */
    char *lpstrFile;                /* +0x1c */
    unsigned long nMaxFile;         /* +0x20 */
    char *lpstrFileTitle;           /* +0x24 */
    unsigned long nMaxFileTitle;    /* +0x28 */
    const char *lpstrInitialDir;    /* +0x2c */
    const char *lpstrTitle;         /* +0x30 */
    unsigned long Flags;            /* +0x34 */
    unsigned short nFileOffset;     /* +0x38 */
    unsigned short nFileExtension;  /* +0x3a */
    const char *lpstrDefExt;        /* +0x3c */
    long lCustData;                 /* +0x40 */
    void *lpfnHook;                 /* +0x44 */
    const char *lpTemplateName;     /* +0x48 */
} OPENFILENAMEA;                    /* 0x4c = 76 bytes */

typedef struct OSVERSIONINFOA {
    unsigned long dwOSVersionInfoSize;   /* +0x00 */
    unsigned long dwMajorVersion;        /* +0x04 */
    unsigned long dwMinorVersion;        /* +0x08 */
    unsigned long dwBuildNumber;         /* +0x0c */
    unsigned long dwPlatformId;          /* +0x10 */
    char szCSDVersion[128];              /* +0x14 */
} OSVERSIONINFOA;

typedef struct PAINTSTRUCT {
    void *hdc;                  /* +0x00 */
    int fErase;                 /* +0x04 */
    WinRect rcPaint;            /* +0x08 */
    int fRestore;               /* +0x18 */
    int fIncUpdate;             /* +0x1c */
    char rgbReserved[32];       /* +0x20 */
} PAINTSTRUCT;

typedef struct MemoryStatus {
    unsigned long dwLength;
    unsigned long dwMemoryLoad;
    unsigned long dwTotalPhys;
    unsigned long dwAvailPhys;
    unsigned long dwTotalPageFile;
    unsigned long dwAvailPageFile;
    unsigned long dwTotalVirtual;
    unsigned long dwAvailVirtual;
} MemoryStatus;

/* gdi32 / user32 -- the port's drawing and window calls */
__declspec(dllimport) void * __stdcall GetDC(void *hwnd);
__declspec(dllimport) int __stdcall ReleaseDC(void *hwnd, void *hdc);
__declspec(dllimport) int __stdcall DeleteObject(void *obj);
__declspec(dllimport) int __stdcall UpdateWindow(void *hwnd);
__declspec(dllimport) int __stdcall MoveWindow(void *hwnd, int x, int y,
                                               int w, int h, int repaint);
__declspec(dllimport) void * __stdcall SetFocus(void *hwnd);
__declspec(dllimport) int __stdcall SetForegroundWindow(void *hWnd);
__declspec(dllimport) unsigned short __stdcall RegisterClassA(WNDCLASSA *wc);
__declspec(dllimport) int __stdcall GetWindowRect(void *hwnd, WinRect *r);
                                                         /* IAT 0x0058f8e0 */
__declspec(dllimport) void * __stdcall GetMenu(void *hwnd);
                                                         /* IAT 0x0058f8d4 */
__declspec(dllimport) void * __stdcall GetStockObject(int which);
                                                         /* IAT 0x0058f61c GDI32 */
__declspec(dllimport) void * __stdcall SelectObject(void *hdc, void *hobj);
__declspec(dllimport) void * __stdcall CreateCompatibleDC(void *hdc);
__declspec(dllimport) int __stdcall DeleteDC(void *hdc);
__declspec(dllimport) void * __stdcall SelectPalette(void *hdc, void *hpal, int background);
__declspec(dllimport) unsigned int __stdcall RealizePalette(void *hdc);
__declspec(dllimport) int __stdcall GetDeviceGammaRamp(void *hdc, void *ramp);
__declspec(dllimport) int __stdcall SetDeviceGammaRamp(void *hdc, void *ramp);
__declspec(dllimport) unsigned int __stdcall SetPaletteEntries(void *hpal, unsigned int start,
                                                               unsigned int count,
                                                               const void *entries);
__declspec(dllimport) unsigned int __stdcall SetDIBColorTable(void *hdc, unsigned int start,
                                                              unsigned int count,
                                                              const void *colors);
__declspec(dllimport) int __stdcall RestoreDC(void *hdc, int saved);
__declspec(dllimport) int __stdcall AbortDoc(void *hdc);
__declspec(dllimport) int __stdcall EndDoc(void *hdc);
__declspec(dllimport) int __stdcall GdiFlush(void);
__declspec(dllimport) int __stdcall GetDIBits(void *hdc, void *bitmap, unsigned int start,
    unsigned int lines, void *bits, void *info, unsigned int usage);
__declspec(dllimport) int __stdcall SetDIBits(void *hdc, void *bitmap, unsigned int start,
    unsigned int lines, const void *bits, const void *info, unsigned int usage);
__declspec(dllimport) int __stdcall GlobalUnlock(void *mem);
__declspec(dllimport) void * __stdcall CreateCompatibleBitmap(void *hdc, int w, int h);
__declspec(dllimport) int __stdcall SaveDC(void *hdc);
__declspec(dllimport) int __stdcall GetClientRect(void *hwnd, WinRect *r);
__declspec(dllimport) unsigned int __stdcall GetDIBColorTable(void *hdc, unsigned int start,
    unsigned int count, void *colors);
__declspec(dllimport) int __stdcall GetClipRgn(void *hdc, void *rgn);
__declspec(dllimport) int __stdcall GetRgnBox(void *rgn, WinRect *r);
__declspec(dllimport) int __stdcall DPtoLP(void *hdc, void *points, int count);
__declspec(dllimport) int __stdcall SetRectRgn(void *rgn, int left, int top, int right,
    int bottom);
__declspec(dllimport) int __stdcall OffsetRgn(void *rgn, int dx, int dy);
__declspec(dllimport) unsigned long __stdcall GetRegionData(void *rgn, unsigned long size,
    void *data);
__declspec(dllimport) void * __stdcall ExtCreateRegion(const void *xform, unsigned long size,
    const void *data);
__declspec(dllimport) int __stdcall CombineRgn(void *dst, void *src1, void *src2, int mode);
__declspec(dllimport) void * __stdcall CreateWindowExA(unsigned long exStyle,
    const char *className, const char *windowName, unsigned long style,
    int x, int y, int w, int h, void *parent, void *menu, void *instance,
    void *param);
__declspec(dllimport) long __stdcall DefWindowProcA(void *hwnd,
    unsigned long msg, unsigned long wParam, long lParam);
                                                         /* IAT 0x0058f8f0 */
__declspec(dllimport) long __stdcall GetWindowLongA(void *hwnd, int index);
                                                         /* IAT 0x0058f8d0 */
__declspec(dllimport) int __stdcall AdjustWindowRectEx(WinRect *r,
    unsigned long style, int menu, unsigned long exStyle);
                                                         /* IAT 0x0058f8d8 */
__declspec(dllimport) int __stdcall SystemParametersInfoA(unsigned int action,
    unsigned int param, void *pv, unsigned int flags);
__declspec(dllimport) void * __stdcall LoadCursorA(void *inst,
                                                   const char *name);
__declspec(dllimport) int __stdcall ShowCursor(int show);
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *count);
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *freq);
__declspec(dllimport) void * __stdcall CreateDIBSection(void *hdc, void *pbmi,
                                                        unsigned int usage,
                                                        void **ppvBits,
                                                        void *hSection,
                                                        unsigned long offset);

/* kernel32 -- the runtime's process, module and synchronisation calls */
__declspec(dllimport) void __stdcall Sleep(unsigned long ms);
__declspec(dllimport) unsigned long __stdcall GetLastError(void);
                                                         /* IAT 0x0058f724 */
__declspec(dllimport) unsigned long __stdcall GetFileAttributesA(const char *path);
                                                         /* IAT 0x0058f7d0 */
__declspec(dllimport) unsigned long __stdcall GetLogicalDrives(void);
__declspec(dllimport) unsigned int __stdcall SetErrorMode(unsigned int mode);
__declspec(dllimport) unsigned int __stdcall GetDriveTypeA(const char *root);
__declspec(dllimport) int __stdcall GetVolumeInformationA(const char *root, char *name,
                                                          unsigned long nameSize,
                                                          unsigned long *serial,
                                                          unsigned long *maxComponent,
                                                          unsigned long *fsFlags,
                                                          char *fsName,
                                                          unsigned long fsNameSize);
__declspec(dllimport) int __stdcall CloseHandle(void *h);
__declspec(dllimport) int __stdcall FlushFileBuffers(void *h);  /* IAT 0x0058f874 */
__declspec(dllimport) int __stdcall ReadFile(void *h, void *buffer, unsigned long count,
                                             unsigned long *read, void *overlapped);
__declspec(dllimport) int __stdcall WriteFile(void *h, const void *buffer,
                                              unsigned long count, unsigned long *written,
                                              void *overlapped);
__declspec(dllimport) unsigned long __stdcall SetFilePointer(void *h, long distance,
                                                             long *distanceHigh,
                                                             unsigned long method);
__declspec(dllimport) int __stdcall SetEndOfFile(void *h);
__declspec(dllimport) void * __stdcall CreateFileA(const char *path, unsigned long access,
                                                   unsigned long share, void *sa,
                                                   unsigned long disposition,
                                                   unsigned long attributes, void *tmpl);
__declspec(dllimport) unsigned long __stdcall GetFileSize(void *h, unsigned long *high);
__declspec(dllimport) void * __stdcall FindResourceA(void *module, const char *name,
                                                     const char *type);
__declspec(dllimport) unsigned long __stdcall SizeofResource(void *module, void *res);
__declspec(dllimport) void * __stdcall LoadResource(void *module, void *res);
__declspec(dllimport) void * __stdcall LockResource(void *data);
__declspec(dllimport) int __stdcall DeleteFileA(const char *path);
__declspec(dllimport) int __stdcall RemoveDirectoryA(const char *path);
__declspec(dllimport) void * __stdcall GetModuleHandleA(const char *name);
__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void *h,
                                                                  unsigned long ms);
__declspec(dllimport) int __stdcall ReleaseMutex(void *h);
__declspec(dllimport) void * __stdcall LoadLibraryA(const char *name);
__declspec(dllimport) void * __stdcall GetProcAddress(void *module,
                                                      const char *name);
__declspec(dllimport) void * __stdcall CreateMutexA(void *sa, int owned,
                                                    const char *name);
__declspec(dllimport) unsigned long __stdcall GetSystemDirectoryA(char *buf,
                                                      unsigned long size);
__declspec(dllimport) unsigned long __stdcall GetModuleFileNameA(void *module,
                                                      char *out,
                                                      unsigned long size);
__declspec(dllimport) unsigned long __stdcall GetTickCount(void);
__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
__declspec(dllimport) void __stdcall InitializeCriticalSection(void *section);
__declspec(dllimport) void __stdcall EnterCriticalSection(void *section);
__declspec(dllimport) void __stdcall LeaveCriticalSection(void *section);
__declspec(dllimport) void __stdcall DeleteCriticalSection(void *section);
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *text);

/* kernel32 threads and events */
typedef unsigned long (__stdcall *LPTHREAD_START_ROUTINE)(void *param);
__declspec(dllimport) void * __stdcall CreateThread(void *sa, unsigned long stack,
    LPTHREAD_START_ROUTINE start, void *param, unsigned long flags,
    unsigned long *threadId);
__declspec(dllimport) int __stdcall SetThreadPriority(void *thread, int priority);
__declspec(dllimport) void * __stdcall GetCurrentThread(void);
__declspec(dllimport) unsigned long __stdcall GetVersion(void);
__declspec(dllimport) unsigned long __stdcall SuspendThread(void *thread);
__declspec(dllimport) unsigned long __stdcall ResumeThread(void *thread);
__declspec(dllimport) int __stdcall TerminateThread(void *thread, unsigned long code);
__declspec(dllimport) void * __stdcall CreateEventA(void *sa, int manual, int initial,
                                                   const char *name);
__declspec(dllimport) int __stdcall SetEvent(void *event);
__declspec(dllimport) int __stdcall ResetEvent(void *event);

/* kernel32 clock, files and pointer probes */
typedef struct SYSTEMTIME {
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
} SYSTEMTIME;

typedef struct SECURITY_ATTRIBUTES {
    unsigned long nLength;
    void *lpSecurityDescriptor;
    int bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef struct FILETIME {
    unsigned long dwLowDateTime;
    unsigned long dwHighDateTime;
} FILETIME;

__declspec(dllimport) int __stdcall FileTimeToSystemTime(const FILETIME *ft, SYSTEMTIME *st);
                                                         /* IAT 0x0058f7cc */
__declspec(dllimport) int __stdcall FileTimeToLocalFileTime(const FILETIME *ft,
                                                            FILETIME *localTime);
                                                         /* IAT 0x0058f7c8 */

typedef struct WIN32_FIND_DATAA {
    unsigned long dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    unsigned long nFileSizeHigh;
    unsigned long nFileSizeLow;
    unsigned long dwReserved0;
    unsigned long dwReserved1;
    char cFileName[260];
    char cAlternateFileName[14];
} WIN32_FIND_DATAA;

__declspec(dllimport) void * __stdcall FindFirstFileA(const char *pattern,
                                                      WIN32_FIND_DATAA *data);
__declspec(dllimport) int __stdcall FindNextFileA(void *find, WIN32_FIND_DATAA *data);
__declspec(dllimport) int __stdcall FindClose(void *find);
__declspec(dllimport) void __stdcall GetSystemTime(SYSTEMTIME *st);
__declspec(dllimport) void __stdcall GetLocalTime(SYSTEMTIME *st);
__declspec(dllimport) unsigned long __stdcall GetWindowsDirectoryA(char *buf,
                                                                   unsigned long size);
__declspec(dllimport) int __stdcall CreateDirectoryA(const char *path,
                                                     SECURITY_ATTRIBUTES *sa);
__declspec(dllimport) int __stdcall IsBadReadPtr(const void *p, unsigned long size);
__declspec(dllimport) int __stdcall IsBadWritePtr(void *p, unsigned long size);

/* kernel32 thread-local storage */
__declspec(dllimport) unsigned long __stdcall TlsAlloc(void);
__declspec(dllimport) void * __stdcall TlsGetValue(unsigned long index);
__declspec(dllimport) int __stdcall TlsSetValue(unsigned long index, void *value);
__declspec(dllimport) int __stdcall TlsFree(unsigned long index);

/* kernel32 heap, file mapping, paths, processes and the console */
typedef int (__stdcall *PHANDLER_ROUTINE)(unsigned long ctrlType);
__declspec(dllimport) void * __stdcall GetProcessHeap(void);
__declspec(dllimport) void * __stdcall HeapAlloc(void *heap, unsigned long flags,
                                                 unsigned long bytes);
__declspec(dllimport) int __stdcall HeapFree(void *heap, unsigned long flags, void *mem);
__declspec(dllimport) unsigned long __stdcall GlobalSize(void *mem);           /* IAT 0x0058f7b8 */
__declspec(dllimport) int __stdcall VirtualUnlock(void *address, unsigned long size);
                                                         /* IAT 0x0058f7bc */
__declspec(dllimport) void * __stdcall GlobalFree(void *mem);                  /* IAT 0x0058f7c0 */
__declspec(dllimport) void * __stdcall GlobalAlloc(unsigned int flags, unsigned long bytes);
__declspec(dllimport) int __stdcall VirtualLock(void *address, unsigned long size);
__declspec(dllimport) void * __stdcall CreateFileMappingA(void *file,
    SECURITY_ATTRIBUTES *sa, unsigned long protect, unsigned long sizeHigh,
    unsigned long sizeLow, const char *name);
__declspec(dllimport) unsigned long __stdcall GetFullPathNameA(const char *name,
    unsigned long bufLen, char *buf, char **filePart);
__declspec(dllimport) unsigned long __stdcall GetCurrentDirectoryA(unsigned long bufLen,
                                                                   char *buf);
__declspec(dllimport) int __stdcall SetCurrentDirectoryA(const char *path);
__declspec(dllimport) unsigned long __stdcall GetTempPathA(unsigned long bufLen, char *buf);
__declspec(dllimport) unsigned long __stdcall SleepEx(unsigned long ms, int alertable);
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId(void);
__declspec(dllimport) int __stdcall FreeLibrary(void *module);
__declspec(dllimport) int __stdcall SetConsoleCtrlHandler(PHANDLER_ROUTINE handler, int add);
__declspec(dllimport) unsigned long __stdcall GetConsoleTitleA(char *title,
                                                               unsigned long size);
__declspec(dllimport) int __stdcall SetConsoleTitleA(const char *title);

/* user32 */
__declspec(dllimport) short __stdcall GetKeyState(int nVirtKey);   /* IAT 0x0058f8a8 */
__declspec(dllimport) int __stdcall IsIconic(void *hwnd);          /* IAT 0x0058f8dc */
__declspec(dllimport) int __stdcall UnhookWindowsHookEx(void *hook);
__declspec(dllimport) void __stdcall PostQuitMessage(int code);
__declspec(dllimport) int __stdcall GetSystemMetrics(int index);

typedef struct POINT {
    long x;
    long y;
} POINT;

__declspec(dllimport) int __stdcall ClientToScreen(void *hwnd, POINT *point);

__declspec(dllimport) void * __stdcall SetWindowsHookExA(int idHook, void *proc,
    void *hmod, unsigned long threadId);
__declspec(dllimport) long __stdcall CallNextHookEx(void *hook, int code,
    unsigned long wParam, long lParam);                    /* IAT 0x0058f8cc */
__declspec(dllimport) int __stdcall FillRect(void *hdc, const WinRect *r,
    void *brush);
__declspec(dllimport) int __stdcall DrawTextA(void *hdc, const char *text,
                                              int count, WinRect *r,
                                              unsigned int format);
__declspec(dllimport) unsigned int __stdcall RegisterWindowMessageA(const char *name);
__declspec(dllimport) void * __stdcall GetDesktopWindow(void);
__declspec(dllimport) void * __stdcall GetForegroundWindow(void);
__declspec(dllimport) int __stdcall PostMessageA(void *hwnd, unsigned int msg,
    unsigned int wParam, long lParam);
__declspec(dllimport) int __stdcall WaitMessage(void);
__declspec(dllimport) int __stdcall TranslateMessage(const void *msg);
__declspec(dllimport) long __stdcall DispatchMessageA(const void *msg);
__declspec(dllimport) void * __stdcall SetCapture(void *hwnd);
__declspec(dllimport) int __stdcall ReleaseCapture(void);

/* Win32's MSG, the record PeekMessageA fills. */
typedef struct WinMsg {
    void *hwnd;                                  /* +0x00 */
    unsigned int message;                        /* +0x04 */
    unsigned int wParam;                         /* +0x08 */
    long lParam;                                 /* +0x0c */
    unsigned long time;                          /* +0x10 */
    POINT pt;                                    /* +0x14 */
} WinMsg;
__declspec(dllimport) void * __stdcall FindWindowA(const char *className,
                                                   const char *windowName);
__declspec(dllimport) long __stdcall SetWindowLongA(void *hwnd, int index, long value);
__declspec(dllimport) int __stdcall SetWindowPos(void *hwnd, void *insertAfter,
    int x, int y, int cx, int cy, unsigned int flags);
__declspec(dllimport) int __stdcall MessageBoxA(void *hwnd, const char *text,
                                                const char *caption, unsigned int type);
__declspec(dllimport) int __stdcall MessageBeep(unsigned int type);
__declspec(dllimport) int __stdcall ClipCursor(const WinRect *r);
__declspec(dllimport) void * __stdcall LoadIconA(void *inst, const char *name);
__declspec(dllimport) void * __stdcall CopyIcon(void *icon);
__declspec(dllimport) int __stdcall DestroyCursor(void *cursor);
__declspec(dllimport) void * __stdcall CreateCursor(void *inst, int hotX, int hotY,
    int width, int height, const void *andPlane, const void *xorPlane);
__declspec(dllimport) void * __stdcall SetCursor(void *cursor);
__declspec(dllimport) int __cdecl wsprintfA(char *out, const char *fmt, ...);

/* gdi32 */
__declspec(dllimport) void * __stdcall CreateFontA(int height, int width,
        int escapement, int orientation, int weight, unsigned long italic,
        unsigned long underline, unsigned long strikeOut, unsigned long charSet,
        unsigned long outPrecision, unsigned long clipPrecision,
        unsigned long quality, unsigned long pitchAndFamily, const char *faceName);
__declspec(dllimport) int __stdcall GetDeviceCaps(void *hdc, int index);
__declspec(dllimport) int __stdcall SetBkMode(void *hdc, int mode);
__declspec(dllimport) int __stdcall SetMapMode(void *hdc, int mode);
__declspec(dllimport) int __stdcall SetROP2(void *hdc, int mode);
__declspec(dllimport) unsigned int __stdcall SetTextAlign(void *hdc, unsigned int align);
__declspec(dllimport) void * __stdcall GetCurrentObject(void *hdc, unsigned int type);
__declspec(dllimport) void * __stdcall CreatePen(int style, int width, unsigned long color);
__declspec(dllimport) int __stdcall MoveToEx(void *hdc, int x, int y, POINT *old);

typedef struct TEXTMETRICA {
    long tmHeight;
    long tmAscent;
    long tmDescent;
    long tmInternalLeading;
    long tmExternalLeading;
    long tmAveCharWidth;
    long tmMaxCharWidth;
    long tmWeight;
    long tmOverhang;
    long tmDigitizedAspectX;
    long tmDigitizedAspectY;
    unsigned char tmFirstChar;
    unsigned char tmLastChar;
    unsigned char tmDefaultChar;
    unsigned char tmBreakChar;
    unsigned char tmItalic;
    unsigned char tmUnderlined;
    unsigned char tmStruckOut;
    unsigned char tmPitchAndFamily;
    unsigned char tmCharSet;
} TEXTMETRICA;

__declspec(dllimport) int __stdcall GetTextMetricsA(void *hdc, TEXTMETRICA *tm);
__declspec(dllimport) unsigned long __stdcall SetTextColor(void *hdc, unsigned long color);
__declspec(dllimport) void * __stdcall CreateRectRgn(int left, int top, int right, int bottom);
__declspec(dllimport) int __stdcall SelectClipRgn(void *hdc, void *hrgn);
__declspec(dllimport) unsigned long __stdcall GetPixel(void *hdc, int x, int y);
__declspec(dllimport) unsigned long __stdcall SetPixel(void *hdc, int x, int y,
                                                       unsigned long color);
__declspec(dllimport) void * __stdcall CreatePalette(const void *logPalette);
#define RGB(r, g, b) \
    ((unsigned long)(((unsigned char)(r) | ((unsigned short)((unsigned char)(g)) << 8)) | \
                     (((unsigned long)(unsigned char)(b)) << 16)))
__declspec(dllimport) unsigned long __stdcall GetTextColor(void *hdc);
__declspec(dllimport) unsigned long __stdcall GetBkColor(void *hdc);
__declspec(dllimport) unsigned long __stdcall SetBkColor(void *hdc, unsigned long color);
__declspec(dllimport) void * __stdcall CreateSolidBrush(unsigned long color);
__declspec(dllimport) void * __stdcall CreateBitmap(int width, int height, unsigned int planes,
                                                    unsigned int bitCount, const void *bits);
__declspec(dllimport) void * __stdcall CreatePatternBrush(void *bitmap);
__declspec(dllimport) int __stdcall LPtoDP(void *hdc, POINT *points, int count);
__declspec(dllimport) int __stdcall ExtSelectClipRgn(void *hdc, void *hrgn, int mode);
__declspec(dllimport) int __stdcall SetViewportOrgEx(void *hdc, int x, int y, POINT *old);
__declspec(dllimport) int __stdcall GetMapMode(void *hdc);
__declspec(dllimport) unsigned long __stdcall GetObjectType(void *obj);
__declspec(dllimport) int __stdcall BitBlt(void *dst, int x, int y, int width, int height,
                                           void *src, int srcX, int srcY, unsigned long rop);
__declspec(dllimport) int __stdcall StretchBlt(void *dst, int x, int y, int width, int height,
                                               void *src, int srcX, int srcY, int srcWidth,
                                               int srcHeight, unsigned long rop);
__declspec(dllimport) int __stdcall StretchDIBits(void *hdc, int x, int y, int width, int height,
                                                  int srcX, int srcY, int srcWidth,
                                                  int srcHeight, const void *bits,
                                                  const void *info, unsigned int usage,
                                                  unsigned long rop);
__declspec(dllimport) int __stdcall SetDIBitsToDevice(void *hdc, int x, int y,
                                                      unsigned long width, unsigned long height,
                                                      int srcX, int srcY, unsigned int startScan,
                                                      unsigned int lines, const void *bits,
                                                      const void *info, unsigned int usage);

/* user32, the interface tool */
__declspec(dllimport) int __stdcall IsWindowVisible(void *hwnd);
__declspec(dllimport) int __stdcall InvalidateRect(void *hwnd, const WinRect *r, int erase);
__declspec(dllimport) int __stdcall ValidateRect(void *hwnd, const WinRect *r);
__declspec(dllimport) int __stdcall AdjustWindowRect(WinRect *r, unsigned long style, int menu);
__declspec(dllimport) void * __stdcall ChildWindowFromPointEx(void *hwnd, POINT pt,
                                                              unsigned int flags);
__declspec(dllimport) void * __stdcall WindowFromPoint(POINT pt);
__declspec(dllimport) int __stdcall ScreenToClient(void *hwnd, POINT *point);
__declspec(dllimport) int __stdcall GetCursorPos(POINT *point);
__declspec(dllimport) int __stdcall DestroyWindow(void *hwnd);
__declspec(dllimport) int __stdcall ShowWindow(void *hwnd, int show);
__declspec(dllimport) void * __stdcall GetParent(void *hwnd);
__declspec(dllimport) int __stdcall GetWindowTextA(void *hwnd, char *text, int size);
__declspec(dllimport) int __stdcall SetWindowTextA(void *hwnd, const char *text);
__declspec(dllimport) int __stdcall GetWindowTextLengthA(void *hwnd);
__declspec(dllimport) int __stdcall EnableWindow(void *hwnd, int enable);
__declspec(dllimport) void * __stdcall ChildWindowFromPoint(void *hwnd, POINT point);
__declspec(dllimport) long __stdcall SendMessageA(void *hwnd, unsigned int msg,
                                                  unsigned int wParam, long lParam);
__declspec(dllimport) int __stdcall PeekMessageA(void *msg, void *hwnd, unsigned int first,
                                                 unsigned int last, unsigned int remove);
__declspec(dllimport) unsigned int __stdcall SetTimer(void *hwnd, unsigned int id,
                                                      unsigned int elapse, void *proc);
__declspec(dllimport) short __stdcall GetAsyncKeyState(int key);
__declspec(dllimport) long __stdcall GetDialogBaseUnits(void);
__declspec(dllimport) int __stdcall EnumWindows(void *proc, long lParam);
__declspec(dllimport) int __stdcall EnumChildWindows(void *parent, void *proc, long lParam);
__declspec(dllimport) int __stdcall DialogBoxIndirectParamA(void *instance, const void *dlg,
                                                            void *parent, void *proc,
                                                            long lParam);
__declspec(dllimport) int __stdcall EndDialog(void *hwnd, int result);
__declspec(dllimport) int __stdcall GetDlgCtrlID(void *hwnd);
__declspec(dllimport) int __stdcall MapWindowPoints(void *from, void *to, POINT *points,
                                                    unsigned int count);
__declspec(dllimport) int __stdcall EqualRect(const WinRect *a, const WinRect *b);

/* gdi32's LOGFONTA */
typedef struct LOGFONTA {
    long lfHeight;                  /* +0x00 */
    long lfWidth;                   /* +0x04 */
    long lfEscapement;              /* +0x08 */
    long lfOrientation;             /* +0x0c */
    long lfWeight;                  /* +0x10 */
    unsigned char lfItalic;         /* +0x14 */
    unsigned char lfUnderline;      /* +0x15 */
    unsigned char lfStrikeOut;      /* +0x16 */
    unsigned char lfCharSet;        /* +0x17 */
    unsigned char lfOutPrecision;   /* +0x18 */
    unsigned char lfClipPrecision;  /* +0x19 */
    unsigned char lfQuality;        /* +0x1a */
    unsigned char lfPitchAndFamily; /* +0x1b */
    char lfFaceName[32];            /* +0x1c */
} LOGFONTA;                         /* 0x3c */

__declspec(dllimport) void * __stdcall CreateFontIndirectA(const LOGFONTA *font);
__declspec(dllimport) int __stdcall SetPixelV(void *hdc, int x, int y, unsigned long color);
__declspec(dllimport) int __stdcall BeginPath(void *hdc);
__declspec(dllimport) int __stdcall EndPath(void *hdc);
__declspec(dllimport) int __stdcall WidenPath(void *hdc);
__declspec(dllimport) int __stdcall FillPath(void *hdc);
__declspec(dllimport) int __stdcall GetTextExtentPoint32A(void *hdc, const char *text, int length,
                                                          POINT *size);
__declspec(dllimport) int __stdcall GetCurrentPositionEx(void *hdc, POINT *point);
__declspec(dllimport) int __stdcall TextOutA(void *hdc, int x, int y, const char *text,
                                             int length);
__declspec(dllimport) int __stdcall EnumFontFamiliesA(void *hdc, const char *family,
                                                      void *proc, long lParam);

/* comctl32 */
__declspec(dllimport) void __stdcall InitCommonControls(void);

__declspec(dllimport) void * __stdcall BeginPaint(void *hwnd, PAINTSTRUCT *ps);
__declspec(dllimport) int __stdcall EndPaint(void *hwnd, PAINTSTRUCT *ps);

/* comdlg32 */
__declspec(dllimport) unsigned long __stdcall CommDlgExtendedError(void); /* IAT 0x0058fa78 */
__declspec(dllimport) int __stdcall GetOpenFileNameA(OPENFILENAMEA *ofn); /* IAT 0x0058fa74 */

/* kernel32 */
__declspec(dllimport) int __stdcall MulDiv(int a, int b, int c);
__declspec(dllimport) void __stdcall SetLastError(unsigned long err); /* IAT 0x0058f720 */
__declspec(dllimport) void __stdcall GlobalMemoryStatus(MemoryStatus *ms); /* IAT 0x0058f71c */
__declspec(dllimport) int __stdcall GetVersionExA(OSVERSIONINFOA *info);
__declspec(dllimport) int __stdcall LoadStringA(void *hInstance, unsigned int id,
                                                char *buf, int bufMax); /* IAT 0x0058f90c */

/* shell32 */
__declspec(dllimport) void * __stdcall ShellExecuteA(void *hwnd, const char *op,
    const char *file, const char *params, const char *dir, int show);

/* advapi32 */
#define HKEY_CURRENT_USER   ((void *)0x80000001)
#define HKEY_LOCAL_MACHINE  ((void *)0x80000002)
#define KEY_READ            0x20019
#define KEY_ALL_ACCESS      0xf003f
#define REG_BINARY          3
__declspec(dllimport) long __stdcall RegOpenKeyExA(void *key, const char *subKey,
    unsigned long options, unsigned long access, void **result);
__declspec(dllimport) long __stdcall RegQueryValueExA(void *key, const char *name,
    unsigned long *reserved, unsigned long *type, unsigned char *data,
    unsigned long *size);
__declspec(dllimport) long __stdcall RegCloseKey(void *key);
__declspec(dllimport) long __stdcall RegCreateKeyExA(void *key, const char *subKey,
    unsigned long reserved, char *className, unsigned long options,
    unsigned long access, void *security, void **result, unsigned long *disposition);
__declspec(dllimport) long __stdcall RegSetValueExA(void *key, const char *name,
    unsigned long reserved, unsigned long type, const unsigned char *data,
    unsigned long size);
__declspec(dllimport) long __stdcall RegFlushKey(void *key);
__declspec(dllimport) int __stdcall GetUserNameA(char *buf, unsigned long *size);

/* iconv */
__declspec(dllimport) void *libiconv_open(const char *tocode, const char *fromcode);
__declspec(dllimport) unsigned long libiconv(void *cd, char **inbuf, unsigned long *inbytesleft,
                                             char **outbuf, unsigned long *outbytesleft);

/* winmm */
__declspec(dllimport) unsigned long __stdcall timeGetTime(void);

/* WAVEFORMATEX: 18 bytes, packed as the SDK packs it */
#pragma options align=packed
typedef struct WaveFormat {
    unsigned short formatTag;
    unsigned short channels;
    unsigned long samplesPerSec;
    unsigned long avgBytesPerSec;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    unsigned short cbSize;
} WaveFormat;
#pragma options align=reset

/* WAVEHDR */
typedef struct WaveHdr {
    char *data;
    unsigned long bufferLength;
    unsigned long bytesRecorded;
    unsigned long user;
    unsigned long flags;
    unsigned long loops;
    struct WaveHdr *next;
    unsigned long reserved;
} WaveHdr;

/* WAVEOUTCAPSA */
typedef struct WaveOutCaps {
    unsigned short mid;
    unsigned short pid;
    unsigned long driverVersion;
    char name[32];
    unsigned long formats;
    unsigned short channels;
    unsigned short reserved1;
    unsigned long support;
} WaveOutCaps;

__declspec(dllimport) unsigned long __stdcall waveOutGetNumDevs(void);
__declspec(dllimport) unsigned long __stdcall waveOutOpen(void **out, unsigned long device,
    const WaveFormat *format, unsigned long callback, unsigned long instance,
    unsigned long flags);
__declspec(dllimport) unsigned long __stdcall waveOutClose(void *wave);
__declspec(dllimport) unsigned long __stdcall waveOutReset(void *wave);
__declspec(dllimport) unsigned long __stdcall waveOutPrepareHeader(void *wave, WaveHdr *hdr,
                                                                   unsigned long size);
__declspec(dllimport) unsigned long __stdcall waveOutUnprepareHeader(void *wave, WaveHdr *hdr,
                                                                     unsigned long size);
__declspec(dllimport) unsigned long __stdcall waveOutWrite(void *wave, WaveHdr *hdr,
                                                           unsigned long size);
__declspec(dllimport) unsigned long __stdcall waveOutGetDevCapsA(unsigned long device,
    WaveOutCaps *caps, unsigned long size);
__declspec(dllimport) unsigned long __stdcall waveOutGetVolume(void *wave,
                                                               unsigned long *volume);
__declspec(dllimport) unsigned long __stdcall waveOutSetVolume(void *wave,
                                                               unsigned long volume);

/* wsock32: called through the linker's jump thunks, as the exe does, so not dllimport */
extern int __stdcall WSAStartup(unsigned short version, void *data);
extern int __stdcall WSACleanup(void);
extern int __stdcall WSAGetLastError(void);
extern unsigned long __stdcall socket(int af, int type, int protocol);
extern int __stdcall closesocket(unsigned long s);
extern int __stdcall bind(unsigned long s, const void *name, int nameLen);
extern int __stdcall getsockname(unsigned long s, void *name, int *nameLen);
extern int __stdcall getsockopt(unsigned long s, int level, int name, void *value,
                                int *length);
extern int __stdcall setsockopt(unsigned long s, int level, int name, const void *value,
                                int length);
extern int __stdcall recvfrom(unsigned long s, void *buf, int length, int flags,
                              void *from, int *fromLen);
extern int __stdcall sendto(unsigned long s, const void *buf, int length, int flags,
                            const void *to, int toLen);
extern void * __stdcall gethostbyname(const char *name);
extern int __stdcall gethostname(char *name, int length);
extern unsigned long __stdcall accept(unsigned long s, void *addr, int *addrLen);
extern int __stdcall connect(unsigned long s, const void *name, int nameLen);
extern int __stdcall recv(unsigned long s, void *buf, int length, int flags);
extern int __stdcall send(unsigned long s, const void *buf, int length, int flags);
extern int __stdcall shutdown(unsigned long s, int how);
extern void * __stdcall gethostbyaddr(const void *addr, int length, int type);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_H */
