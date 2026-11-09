// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* cetext.c -- EV Nova CE: MacRoman text drawn as UTF-16 in place of iconv's CP1252, and the
   version text on the main menu (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <stddef.h>
#include "macport.h"
#include "render.h"
#include "cefix.h"

#define kCPMacCP            2           /* CP_MACCP: the system's Macintosh code page */
#define kVersionText        "EV Nova Community Edition r4"
#define kVersionGrey        0x8000
#define kVersionLeft        5
#define kVersionRaise       6           /* baseline above the backdrop's bottom edge */
#define kVersionTextSize    9

__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int codePage,
    unsigned long flags, const char *src, int srcCount, unsigned short *dst, int dstCount);
__declspec(dllimport) int __stdcall DrawTextW(void *hdc, const unsigned short *text,
    int count, WinRect *r, unsigned int format);

/* One UTF-16 unit per MacRoman byte, so a byte count is still the character count. */
int CE_MacRomanToWide(const void *text, unsigned long count, char *out, unsigned long outCount)
{
    return MultiByteToWideChar(kCPMacCP, 0, (const char *)text, count,
                               (unsigned short *)out, outCount);
}

int CE_DrawTextW(void *hdc, const char *text, int count, struct WinRect *r, unsigned int format)
{
    return DrawTextW(hdc, (const unsigned short *)text, count, r, format);
}

/* Grey, in the lower-left corner of the main menu's backdrop. */
void CE_DrawVersion(void)
{
    static const char version[] = kVersionText;

    CurrentGrafPort->foreColor.red = kVersionGrey;
    CurrentGrafPort->foreColor.green = kVersionGrey;
    CurrentGrafPort->foreColor.blue = kVersionGrey;
    CurrentGrafPort->pnLoc.h = kVersionLeft;
    CurrentGrafPort->pnLoc.v = CurrentGrafPort->pix.bounds.bottom - kVersionRaise;
    CurrentGrafPort->txFont = smallTextFont;
    TextSize(kVersionTextSize);
    DrawTextRun(version, sizeof(version) - 1, NULL, 0, 0);
}

#endif /* CEFIX */
