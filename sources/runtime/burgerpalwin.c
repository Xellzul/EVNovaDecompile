// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
/* burgerpalwin.c -- BurgerLib PlWin95Code: loading a run of palette entries into DirectDraw and GDI. Notes: Research/notes/burgerpalwin.md */

#include <string.h>

#include "burgertypes.h"
#include "burgerddraw.h"
#include "burger.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

void (*PaletteChangedProc)(void);                /* 0x00870948, never set in this exe */

#define kLastEntry      255                      /* entries 0 and 255 stay black and white */
#define kRGBSize        3

/* PaletteSetPtr (BurgerLib PlWin95Code). STATE: MATCH 100.0%, exe 232 / obj 232,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00502080
void PaletteSetPtr(Word start, Word count, const Word8 *palette)
{
    Word8 entries[kPaletteEntries][4];
    void *memDC;
    void *hdc;
    Word temp;
    Word8 *entry;
    Word i;

    if (palette == 0 || start >= kPaletteEntries)
        return;
    if (start + count >= kPaletteEntries + 1)
        count = kPaletteEntries - start;
    if (count == 0)
        return;
    if (start < 1) {
        temp = 1 - start;
        if (count <= temp)
            return;
        count -= temp;
        start = 1;
        palette += temp * kRGBSize;
    }
    if (start >= kLastEntry)
        return;
    if (start + count > kLastEntry)
        count = kLastEntry - start;
    if (count != 0) {
        memcpy(&CurrentPalette[start * kRGBSize], palette, count * kRGBSize);
        if (PaletteChangedProc != 0)
            PaletteChangedProc();
        if (Win95WindowPalettePtr != 0 || BurgerHPalette != 0) {
            i = count;
            entry = entries[0];
            do {
                entry[0] = palette[0];
                entry[1] = palette[1];
                entry[2] = palette[2];
                palette += kRGBSize;
                entry[3] = 0;
                entry += 4;
            } while (--i);
            temp = VideoPageLocked;
            if (temp != 0)
                UnlockVideoPage();
            if (Win95WindowPalettePtr != 0 &&
                Win95WindowPalettePtr->vtbl->SetEntries(Win95WindowPalettePtr, 0, start,
                                                        count, entries) == kSurfaceLost) {
                Win95FrontBuffer->vtbl->Restore(Win95FrontBuffer);
                Win95WindowPalettePtr->vtbl->SetEntries(Win95WindowPalettePtr, 0, start,
                                                        count, entries);
            }
            if (BurgerHPalette != 0) {
                SetPaletteEntries(BurgerHPalette, start, count, entries);
                hdc = GetDC(Win95MainWindow);
                if (hdc != 0) {
                    if (BurgerHPalette != 0)
                        SelectPalette(hdc, BurgerHPalette, 0);
                    RealizePalette(hdc);
                    if (BurgerHBitMap != 0) {
                        memDC = CreateCompatibleDC(hdc);
                        if (memDC != 0) {
                            i = count;
                            entry = entries[0];
                            do {
                                Word8 red = entry[0];
                                entry[0] = entry[2];
                                entry[2] = red;
                                entry += 4;
                            } while (--i);
                            SelectObject(memDC, BurgerHBitMap);
                            SetDIBColorTable(memDC, start, count, entries);
                            DeleteDC(memDC);
                        }
                    }
                    ReleaseDC(Win95MainWindow, hdc);
                }
                if (Win95MainWindow != 0)
                    InvalidateRect(Win95MainWindow, 0, 0);
            }
            if (temp != 0)
                LockVideoPage();
        }
    }
}

#ifdef __cplusplus
}
#endif
