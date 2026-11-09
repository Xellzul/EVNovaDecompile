// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* winmain.c -- the port's WinMain (win 0x004d2a80). Notes: Research/notes/winmain.md */
#include <stddef.h>
#include "macport.h"
#include "burger.h"
#include "game.h"
#include "player.h"
#include "cefix.h"
#ifdef V111
#include "render.h"
#endif


/* WinMain. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d2a80
int __stdcall WinMain(void *hInstance, void *hPrevInstance, char *lpCmdLine,
                      int nCmdShow)
{
    MemoryStatus ms;
    char buf[256];

#ifdef V111 /* V111: the mac main picks the pipeline first; win draws through the window frame */
    useLegacyRenderingPipeline = 1;
#endif
#ifdef CEFIX /* F07.13 */
    if (CE_Startup(lpCmdLine, nCmdShow) != 0)
        return 0;
#endif
    if (Win95Allow1Instance("Lock for EV Nova") == 0) {
        CreateGameWindow(hInstance, nCmdShow);
        ms.dwLength = 0x20;
        GlobalMemoryStatus(&ms);
#ifdef CEFIX /* F30.01-02 */
        MaxMemSize = kCEMemoryPoolSize;
#else
        MaxMemSize = ms.dwTotalPhys * 2 / 4;
#endif
        if (MaxMemSize < 0x8000000)
            MaxMemSize = 0x8000000;
        InitMemory();
        SetDefaultPrefixs();
        atexit(ShutdownWinGame);
        OSCursorReset();
#ifndef CEFIX /* F07.12 */
        if (DoesFileExist("9:Nova Files:Nova Music.mp3") == 0)
            ShowAlert("Nova Music.mp3 is missing",
                      "The file \"Nova Music.mp3\" is missing. Please make sure "
                      "that you've properly updated your copy of EV Nova for "
                      "Windows by copying this file into the \"Nova Files\" "
                      "folder.");
#endif
        InitializeQTML(2);
        gRezOpenPending = 1;
        EVOpenResourceFile("9:Nova.rez");
        SetLastError(0);
        LoadStringA(hInstance, 0x3b6, buf, 0x100);
        GetLastError();
        LoadStringA(hInstance, 0x3b7, buf, 0x100);
        GetLastError();
        LoadStringA(hInstance, 0x3b8, buf, 0x100);
        GetLastError();
#ifdef CEFIX /* F12.88 */
        CE_InitScaleFactor();
#endif
        InitFonts();
        InitEVNova();
        TerminateQTML();
    }
    return 0;
}
