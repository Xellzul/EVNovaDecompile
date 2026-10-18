// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* textedit.c -- the port's TextEdit, over Win32 DrawText (win TU region 0x004bcd30-0x004bcf80). Notes: Research/notes/textedit.md */
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "burger.h"



/* DrawText with DT_CALCRECT|DT_WORDBREAK: the wrapped height for a width */




/* the one-shot text drawer: erase, position, draw wrapped, done */
/* TETextBox. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcd30
void TETextBox(const void *text, long length, Rect *box, short just)
{
    if (CurrentGrafPort == NULL)
        return;
    EraseRect(box);
    MoveTo(box->left, (box->top + CurrentGrafPort->txSize));
    DrawTextRun(text, length, box, 1, 0);
}

/* TENew. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcd90
TEHandle TENew(const Rect *destRect, const Rect *viewRect)
{
    TEHandle te;
    Rect *r;

    te = (TEHandle)AllocAHandle2(sizeof(TERec), 0);
    LockAHandle((Handle)te);
    memset(*te, 0, sizeof(TERec));

    r = &(*te)->destRect;
    r->top = destRect->top;
    r->left = destRect->left;
    r->bottom = destRect->bottom;
    r->right = destRect->right;

    r = &(*te)->viewRect;
    r->top = viewRect->top;
    r->left = viewRect->left;
    r->bottom = viewRect->bottom;
    r->right = viewRect->right;
    return te;
}

/* TECalText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bce10
void TECalText(TEHandle hTE)
{
    (*hTE)->height = MeasureWrappedText((*hTE)->text, (*hTE)->textLength,
                                        (*hTE)->viewRect.right - (*hTE)->viewRect.left);
}

/* TESetText. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bce40
void TESetText(const void *text, long length, TEHandle hTE)
{
    if ((*hTE)->text != NULL)
        DeallocAPointer((*hTE)->text);
    (*hTE)->text = AllocAPointer(length);
    (*hTE)->textLength = length;
    memcpy((*hTE)->text, text, length);
    TECalText(hTE);
}

/*
 * Scrolls the text within its view, clamped so the last line cannot rise
 * above the bottom of the view nor the first fall below its top.
 */
/* TEPinScroll. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bce90
void TEPinScroll(int dh, int dv, TEHandle hTE)
{
    int top;
    int bottom;
    int viewBottom;
    int viewTop;
    CGrafPtr savePort;

    top = (*hTE)->destRect.top;
    bottom = top + (*hTE)->height;
    viewBottom = (*hTE)->viewRect.bottom;
    if (dv + bottom < viewBottom) {
        dv = viewBottom - bottom;
        if (dv > 0)
            dv = 0;
    } else {
        viewTop = (*hTE)->viewRect.top;
        if (top + dv > viewTop)
            dv = viewTop - top;
    }
    OffsetRect(&(*hTE)->destRect, dh, dv);

    if ((*hTE)->port) {
        GetPort(&savePort);
        SetPort((*hTE)->port);
        TEUpdate(&(*hTE)->viewRect, hTE);
        SetPort(savePort);
        nv_UpdateDialogWindow(0);
    }
}

/* TEUpdate. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcf30
void TEUpdate(Rect *rUpdate, TEHandle hTE)
{
    EraseRect(rUpdate);
    MoveTo(rUpdate->left, rUpdate->top);
    DrawTextRun((*hTE)->text, (*hTE)->textLength, rUpdate, 1,
                (*hTE)->viewRect.top - (*hTE)->destRect.top);
    ScreenEndScene();
}

/* TEDispose. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004bcf80
void TEDispose(TEHandle hTE)
{
    if ((*hTE)->text != NULL)
        DeallocAPointer((*hTE)->text);
    DeallocAHandle(hTE);
}
