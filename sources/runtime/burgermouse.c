// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgermouse.c -- BurgerLib InMouseWin95: mouse presence, buttons, position, and the cursor clip. Notes: Research/notes/burgermouse.md */

#include <stdlib.h>

#include "burgertypes.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

Word MousePresent;                               /* 0x008701b8 */
static unsigned char Once;                       /* 0x00865ab0, clip hook registered */

/* MouseInit (BurgerLib InMouseWin95). STATE: MATCH 100.0%, exe 3 / obj 3, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7af0
unsigned int MouseInit(void)
{
    MousePresent = 1;
    return 1;
}

/* MouseDestroy (BurgerLib MouseDestroy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d7b00
void MouseDestroy(void)
{
}

/* MouseReadButtons (BurgerLib InMouseWin95). STATE: MATCH 100.0%, exe 2 / obj 2,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7b10
int MouseReadButtons(void)
{
    return Win95MouseButton;
}

/* Both axes are clamped to [0, extent-1]; a negative reads as 0. */
/* MouseReadPosition (BurgerLib MouseReadAbs). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004d7b20
void MouseReadPosition(long *h, long *v)
{
    long p;

    if (h != 0) {
        p = Win95MouseX;
        if (p < 0)
            p = 0;
        if (p >= (long)ScreenWidth)
            p = ScreenWidth - 1;
        *h = p;
    }
    if (v != 0) {
        p = Win95MouseY;
        if (p < 0)
            p = 0;
        if (p >= (long)ScreenHeight)
            p = ScreenHeight - 1;
        *v = p;
    }
}

/* KillClipping (BurgerLib KillClipping). STATE: MATCH 100.0%, exe 4 / obj 4, byte-identical
   yes. OPEN: none */
// FUNCTION: EVNOVA 0x004d7b80
void KillClipping(void)
{
    ClipCursor(0);
}

/* MouseSetRange (BurgerLib MouseSetRange). STATE: MATCH, exe 43 / obj 43, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004d7ba0
void MouseSetRange(long width, long height)
{
    WinRect bounds;
    POINT origin;

    if (Once == 0) {
        Once = 1;
        atexit(KillClipping);
    }
    if (VideoFullScreen == 0) {
        origin.x = 0;
        origin.y = 0;
        if (Win95MainWindow != 0)
            ClientToScreen(Win95MainWindow, &origin);
        Win95DestScreenX = origin.x;
        Win95DestScreenY = origin.y;
        ClipCursor(0);
    } else {
        bounds.right = width;
        bounds.bottom = height;
        Win95DestScreenX = 0;
        bounds.left = 0;
        bounds.top = 0;
        Win95DestScreenY = 0;
        ClipCursor(&bounds);
    }
}

#ifdef __cplusplus
}
#endif
