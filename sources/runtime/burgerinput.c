// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members); noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); -inline auto -align 4 are the library's (Vorbis), listing-identical here; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgerinput.c -- the alert box and the debug log. Notes: Research/notes/burgerinput.md */

#include "burgertypes.h"
#include "burger.h"
#include "macport.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif


long Win95MouseX;                                /* 0x0087001c */
long Win95MouseY;                                /* 0x00870020 */
long MouseClicked;                               /* 0x0087017c */

#define kAlertStyle   0

static Word Tested;                              /* 0x00865ad8 DbWin95Code's .bss, in order */
static int (__stdcall *DebugTest)(void);
static char Semi[24];                            /* 0x00865ae0 critsec, the log file's lock */
static long Inited;                              /* 0x00865af8 lock state */

#define kDebugLockNone  0
#define kDebugLockLive  1
#define kDebugLockGone  2

/* ShowAlert. STATE: DIFF 30.4%, exe 23 / obj 18 instrs. OPEN: foreign-compiler ceiling; see notes */
// FUNCTION: EVNOVA 0x004d9c90
void ShowAlert(const char *title, const char *message)
{
    unsigned int wasVisible;

    wasVisible = OSCursorIsVisible();
    OSCursorShow();
    SetForegroundWindow(GetDesktopWindow());
    MessageBoxA(Win95MainWindow, message, title, kAlertStyle);
    if (wasVisible == 0)
        OSCursorHide();
}

/* Cleanup. STATE: DIFF 0.0%, exe 5 / obj 4 instrs, byte-identical no.
   OPEN: foreign-compiler ceiling (cwsig FOREIGN); see notes */
// FUNCTION: EVNOVA 0x004d9cf0
void Cleanup(void)
{
    DeleteCriticalSection(Semi);
    Inited = kDebugLockGone;
}

/* Halt (BurgerLib dbwin32): break into an attached debugger. Not linked. */
__declspec(dllimport) void __stdcall DebugBreak(void);

void Halt(void)
{
    void *kernel;

    if (!Tested) {
        kernel = LoadLibraryA("kernel32.dll");
        if (kernel)
            DebugTest = (int (__stdcall *)(void))GetProcAddress(kernel, "IsDebuggerPresent");
        Tested = 1;
    }
    if (DebugTest && DebugTest())
        DebugBreak();
}

/* DebugXString. STATE: DIFF 8.5%, exe 59 / obj 51 instrs, byte-identical no.
   OPEN: -proc pentium4 MATCH (pending the pin); see notes */
// FUNCTION: EVNOVA 0x004d9d10
void DebugXString(const char *string)
{
    struct _FILE *log;
    unsigned int length;

    if (Inited == kDebugLockNone) {
        InitializeCriticalSection(Semi);
        Inited = kDebugLockLive;
        atexit(Cleanup);
    }
    if (Inited == kDebugLockLive)
        EnterCriticalSection(Semi);
    length = strlen(string);
    if (length != 0) {
        log = (struct _FILE *)FOpenExpandedPath("LogFile.Txt", "a");
        if (log != 0) {
            fwrite(string, 1, length, log);
            fclose(log);
        }
    }
    OutputDebugStringA(string);
    if (Inited == kDebugLockLive)
        LeaveCriticalSection(Semi);
}

#ifdef __cplusplus
}
#endif
