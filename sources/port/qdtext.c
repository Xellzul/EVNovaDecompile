// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* qdtext.c -- the port's QuickDraw text calls, DrawTextRun .. MeasureWrappedText (8 functions); exe 0x004bc760..0x004bcc70. Notes: Research/notes/toolbox.md */

#include <stddef.h>
#include <string.h>
#include "macport.h"
#include "gametypes.h"
#include "burger.h"
#include "player.h"
#define DT_SINGLELINE 0x0020
#define DT_WORDBREAK  0x0010
#define DT_NOCLIP     0x0100
#define DT_CALCRECT   0x0400
#define DT_NOPREFIX   0x0800
#define iconv libiconv                                    /* as GNU iconv.h does */

/* DrawTextRun. STATE: MATCH, byte-identical. OPEN: the walk's compare constant and counter
   name are guesses (both dead in the exe); see notes */
// FUNCTION: EVNOVA 0x004bc760
void DrawTextRun(const void *text, unsigned long byteCount, Rect *box, char clipped, int height)
{
    void *oldBitmap;
    int oldBkMode;
    void *hdc;
    unsigned long outLeft;
    unsigned long color;
    WinRect r;
    unsigned long inLeft;
    char *outPtr;
    unsigned long flags;
    char *buf;
    long width;
    long vpos;
    void *hrgn;
    long n;
    long lines;

    if (text == NULL || byteCount == 0)
        return;

    buf = AllocAPointer(byteCount);
    n = byteCount;
    outPtr = (char *)text;
    lines = 0;
    do {
        if (*outPtr++ == '\r')
            lines++;
        n--;
    } while (n != 0);
    inLeft = outLeft = byteCount;
    outPtr = buf;
    iconv(gMacRomanToCP1252, (char **)&text, &inLeft, &outPtr, &outLeft);

    width = CurrentGrafPort->pnLoc.h - CurrentGrafPort->pix.bounds.left;
    vpos = CurrentGrafPort->pnLoc.v - CurrentGrafPort->pix.bounds.top;

    hdc = CurrentGrafPort->hdc;
    if (hdc == NULL)
        return;

    oldBitmap = SelectObject(hdc, CurrentGrafPort->bitmap);
    SelectObject(hdc,
                 GetFont(CurrentGrafPort->txFont, CurrentGrafPort->txSize,
                         CurrentGrafPort->txFace));
    oldBkMode = SetBkMode(hdc, 1);

    color = (unsigned char)(gGammaRamp[CurrentGrafPort->foreColor.blue >> 11] * 8) << 16;
    color |= ((unsigned char)(gGammaRamp[CurrentGrafPort->foreColor.green >> 11] * 8) << 8)
           | (unsigned char)(gGammaRamp[CurrentGrafPort->foreColor.red >> 11] * 8);
    SetTextColor(hdc, color);

    if (clipped == 0) {
        flags = DT_SINGLELINE;
        if (box != NULL) {
            r.top = vpos - CurrentGrafPort->txSize;
            r.left = width;
            r.right = box->right;
            r.bottom = box->bottom;
        } else {
            flags |= DT_NOCLIP;
            r.top = vpos - CurrentGrafPort->txSize;
            r.left = width;
            r.right = CurrentGrafPort->pix.bounds.right;
            r.bottom = CurrentGrafPort->pix.bounds.bottom;
        }
    } else {
        flags = DT_WORDBREAK;
        r.top = box->top;
        r.left = box->left;
        r.right = box->right;
        r.bottom = box->bottom;
        hrgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
        SelectClipRgn(hdc, hrgn);
        DeleteObject(hrgn);
        r.top -= height;
    }
    flags |= DT_NOPREFIX;
    DrawTextA(hdc, buf, byteCount, &r, flags);
    if (clipped != 0) {
        SelectClipRgn(hdc, NULL);
    } else {
        if (DrawTextA(hdc, buf, byteCount, &r, DT_CALCRECT) != 0)
            CurrentGrafPort->pnLoc.h += (r.right - r.left);
    }

    DeallocAPointer(buf);
    SelectObject(hdc, oldBitmap);
    SetBkMode(hdc, oldBkMode);
}

/* DrawText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bca70
void DrawText(const void *textBuf, int firstByte, int byteCount)
{
    DrawTextRun((char *)textBuf + firstByte, byteCount, NULL, 0, 0);
}

/* DrawString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bca90
void DrawString(const unsigned char *str)
{
    DrawTextRun(str + 1, str[0], NULL, 0, 0);
}

/* DrawCString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcab0
void DrawCString(const char *text)
{
    DrawTextRun(text, strlen(text), NULL, 0, 0);
}

/* the same measurer over a Pascal string */
/* StringWidth. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcad0
short StringWidth(const unsigned char *str)
{
    char text[256];

    PStr2CStr(text, str);
    return MeasureCString(text);
}

/* MeasureCString. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcb00
short MeasureCString(const char *text)
{
    WinRect r;

    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || text[0] == 0)
        return 0;
    SelectObject(CurrentGrafPort->hdc,
                 GetFont(CurrentGrafPort->txFont, CurrentGrafPort->txSize,
                         CurrentGrafPort->txFace));
    r.top = r.left = r.bottom = r.right = 0;
    DrawTextA(CurrentGrafPort->hdc, text, strlen(text), &r,
              DT_CALCRECT | DT_NOCLIP | DT_SINGLELINE);
    return r.right;
}

/* TextWidth. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcbc0
short TextWidth(const char *text, int byteCount)
{
    WinRect r;

    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || text[0] == 0)
        return 0;
    SelectObject(CurrentGrafPort->hdc,
                 GetFont(CurrentGrafPort->txFont, CurrentGrafPort->txSize,
                         CurrentGrafPort->txFace));
    r.top = r.left = r.bottom = r.right = 0;
    DrawTextA(CurrentGrafPort->hdc, text, byteCount, &r,
              DT_CALCRECT | DT_NOCLIP | DT_SINGLELINE);
    return r.right;
}

/* MeasureWrappedText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcc70
int MeasureWrappedText(const char *text, int byteCount, int width)
{
    WinRect r;

    if (CurrentGrafPort == NULL)
        return 0;
    if (text == NULL || byteCount == 0)
        return 0;
    r.top = r.left = r.bottom = 0;
    r.right = width;
    SelectObject(CurrentGrafPort->hdc,
                 GetFont(CurrentGrafPort->txFont, CurrentGrafPort->txSize,
                         CurrentGrafPort->txFace));
    DrawTextA(CurrentGrafPort->hdc, text, byteCount, &r,
              DT_CALCRECT | DT_WORDBREAK);
    return r.bottom;
}
