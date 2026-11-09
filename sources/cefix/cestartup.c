// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* cestartup.c -- EV Nova CE: startup, ddraw.ini, video mode, cnc-ddraw windowing, x2 key,
   window procedure (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include "macport.h"
#include "burger.h"
#include "player.h"
#include "cefix.h"

#define kIniFileName        "ddraw.ini"
#define kDefaultX2Hotkey    "0x14"              /* VK_CAPITAL */
#define kVideoDepth         15
#define kVideoFullScreen    0x01                /* cnc-ddraw decides the real mode */
#define kVideoAlways        0x02
#define kSMCXScreen         0
#define kSMCYScreen         1
#define kWMSetCursor        0x0020
#define kWMToggleFullscreen (0x8000 + 117)      /* WM_APP + 117, cnc-ddraw */
#define kCncSetFullscreen   1
#define kCncSetWindowed     2
#define kHTClient           1
#define kIDCArrow           ((const char *)32512)
#define kRegOptionNonVolatile 0
#define kKeyWrite           0x20006
#define kRegSZ              1
#define kSHCNEAssocChanged  0x08000000
#define kSHCNFIDList        0
#define kWineOverride       "native, builtin"
#define kGameIconGroup      202
#define kNplayIconGroup     203                 /* CE's .nplay icon, linked by -CeRsrcExe */
#define kRTGroupIcon        ((const char *)14)  /* RT_GROUP_ICON */

__declspec(dllimport) unsigned int __stdcall GetPrivateProfileIntA(const char *app,
    const char *key, int def, const char *file);
__declspec(dllimport) unsigned long __stdcall GetPrivateProfileStringA(const char *app,
    const char *key, const char *def, char *out, unsigned long size, const char *file);
__declspec(dllimport) int __stdcall PostMessageA(void *hwnd, unsigned int msg,
    unsigned int wParam, long lParam);
__declspec(dllimport) void __stdcall SHChangeNotify(long eventId, unsigned int flags,
    const void *item1, const void *item2);

extern int __argc;                              /* MSL runtime */
extern void *gCustomCursorH;                      /* the game cursor, atcursor.c */

char ceIniPath[kCEMaxPath];
int ceX2Hotkey;
unsigned char ceRunInAWindow;

static void SetRegValue(const char *key, const char *value)
{
    void *hkey;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, key, 0, NULL, kRegOptionNonVolatile, kKeyWrite,
                        NULL, &hkey, NULL) != 0)
        return;
    RegSetValueExA(hkey, NULL, 0, kRegSZ, (const unsigned char *)value, strlen(value) + 1);
    RegCloseKey(hkey);
}

/* Under Wine, ddraw.dll next to the exe is used only once a dll override names it;
   a newly added override needs a restart. */
static int AddWineDllOverride(const char *exePath)
{
    char key[kCEMaxPath + 64];
    const char *exeName;
    void *hkey;
    int added;

    if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version") == NULL)
        return 0;
    exeName = strrchr(exePath, '\\');
    exeName = exeName != NULL ? exeName + 1 : exePath;
    sprintf(key, "SOFTWARE\\Wine\\AppDefaults\\%s\\DllOverrides", exeName);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, key, 0, NULL, kRegOptionNonVolatile,
                        kKeyWrite | KEY_READ, NULL, &hkey, NULL) != 0)
        return 0;
    added = 0;
    if (RegQueryValueExA(hkey, "ddraw", NULL, NULL, NULL, NULL) != 0
        && RegSetValueExA(hkey, "ddraw", 0, kRegSZ, (const unsigned char *)kWineOverride,
                          sizeof(kWineOverride)) == 0
        && RegQueryValueExA(hkey, "ddraw", NULL, NULL, NULL, NULL) == 0)
        added = 1;
    RegCloseKey(hkey);
    return added;
}

/* CE's entry wrapper and its MSL-entry hook, run as WinMain's first statement: the
   ddraw.ini path, the App Paths key and the .nplay association, then the first argument
   becomes the current directory. Returns nonzero when the game must quit because a
   restarted copy was launched instead. */
int CE_Startup(char *cmdLine, int cmdShow)
{
    char exePath[kCEMaxPath];
    char command[kCEMaxPath + 16];
    char *slash;
    int icon;

    /* CE imports ddraw.dll: cnc-ddraw's DllMain sets DPI awareness before the window exists,
       and this reference keeps it loaded through ReleaseVideo's FreeLibrary. */
    LoadLibraryA("ddraw.dll");
    /* CE bug: CE's mingw startup runs fninit, so the game runs at 64-bit x87 precision, not
       Windows' 53, and some results round differently (r4 shows a speed-300 ship as 199). */
    _controlfp(_PC_64, _MCW_PC);
    GetModuleFileNameA(NULL, exePath, sizeof(exePath));
    if (AddWineDllOverride(exePath)) {
        ShellExecuteA(NULL, "open", exePath, cmdLine, NULL, cmdShow);
        return 1;
    }

    strcpy(ceIniPath, exePath);
    slash = strrchr(ceIniPath, '\\');
    strcpy(slash != NULL ? slash + 1 : ceIniPath, kIniFileName);

    SetRegValue("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\EV Nova.exe",
                exePath);
    SetRegValue("Software\\Classes\\.nplay", "EVNova.Play.1");
    SetRegValue("Software\\Classes\\EVNova.Play.1", "Play EV Nova");
    sprintf(command, "\"%s\" \"%%1\"", exePath);
    SetRegValue("Software\\Classes\\EVNova.Play.1\\shell\\open\\command", command);
    icon = FindResourceA(NULL, (const char *)kNplayIconGroup, kRTGroupIcon) != NULL
               ? kNplayIconGroup : kGameIconGroup;
    sprintf(command, "\"%s\",-%d", exePath, icon);
    SetRegValue("Software\\Classes\\EVNova.Play.1\\DefaultIcon", command);
    SHChangeNotify(kSHCNEAssocChanged, kSHCNFIDList, NULL, NULL);

    /* like CE: a .nplay FILE as the argument fails here and changes nothing */
    if (__argc > 1)
        SetCurrentDirectoryA(__argv[1]);
    return 0;
}

void CE_ReadX2Hotkey(void)
{
    char buf[8];

    GetPrivateProfileStringA(kCEIniSection, "key_x2mode", kDefaultX2Hotkey, buf, sizeof(buf),
                             ceIniPath);
    ceX2Hotkey = strtol(buf, NULL, 0);
}

double CE_IniDouble(const char *key, const char *def)
{
    char buf[8];

    GetPrivateProfileStringA(kCEIniSection, key, def, buf, sizeof(buf), ceIniPath);
    return atof(buf);
}

/* Replaces the monitor setup: ddraw.ini's game_width and game_height, else the desktop. */
void CE_InitializeMonitor(void)
{
    long width;
    long height;

    width = GetPrivateProfileIntA(kCEIniSection, "game_width", 0, ceIniPath);
    height = GetPrivateProfileIntA(kCEIniSection, "game_height", 0, ceIniPath);
    if (width == 0)
        width = GetSystemMetrics(kSMCXScreen);
    if (height == 0)
        height = GetSystemMetrics(kSMCYScreen);
    SetDisplayToSize(width, height, kVideoDepth, kVideoFullScreen | kVideoAlways);
}

/* The windowed checkbox shows cnc-ddraw's state. Without cnc-ddraw the system ddraw.dll
   has no DDIsWindowed, and the checkbox falls back to the game's own pref. */
unsigned char CE_IsWindowed(void)
{
    void *ddraw;
    long (__stdcall *isWindowed)(void);

    isWindowed = NULL;
    ddraw = GetModuleHandleA("ddraw.dll");
    if (ddraw != NULL)
        isWindowed = (long (__stdcall *)(void))GetProcAddress(ddraw, "DDIsWindowed");
    if (isWindowed == NULL)
        return settings.windowedPending;
    return isWindowed() != 0;
}

void CE_ToggleFullscreen(void)
{
    PostMessageA(Win95MainWindow, kWMToggleFullscreen,
                 ceRunInAWindow ? kCncSetWindowed : kCncSetFullscreen, 0);
}

/* After a resize over the client area the cursor is the game's again, not an hourglass. */
long __stdcall CE_WndProc(void *hwnd, unsigned long msg, unsigned long wParam, long lParam)
{
    if (msg == kWMSetCursor && (lParam & 0xffff) == kHTClient)
        SetCursor(gCustomCursorH != NULL ? gCustomCursorH : LoadCursorA(NULL, kIDCArrow));
    return WinGameWndProc(hwnd, msg, wParam, lParam);
}

#endif /* CEFIX */
