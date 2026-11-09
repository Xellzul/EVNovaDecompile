// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* cescale.c -- EV Nova CE: the ui_scale factor and the dialog scaling core (CEFIX only).
   Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <stddef.h>
#include "macport.h"
#include "burger.h"
#include "cefix.h"

double ceScaleFactor;
unsigned char ceScaleEnabled;
int ceGridCellWidth = 83;           /* the grid pitch tradedlg.c lays out (0x00499150), not 84x55 */
int ceGridCellHeight = 54;
int ceGridThumbSize = 32;
int ceStatusBarWidth = 194;
int ceListItemBaseHeight = 8;       /* the list font's 9 in the original; 8 as in Mac Nova */

static short SwapShort(unsigned short v)
{
    return (short)(v >> 8 | v << 8);
}

int CE_Round(double value)
{
    return (int)(value + (value >= 0 ? 0.5 : -0.5));
}

/* ddraw.ini's ui_scale, read before the fonts are set up. */
void CE_InitScaleFactor(void)
{
    ceScaleFactor = CE_IniDouble("ui_scale", "1.0");
    if (ceScaleFactor == 0)
        ceScaleFactor = 1.0;
    ceScaleEnabled = ceScaleFactor != 1.0;
    ceGridCellWidth = CE_Round(ceGridCellWidth * ceScaleFactor);
    ceGridCellHeight = CE_Round(ceGridCellHeight * ceScaleFactor);
    ceGridThumbSize = CE_Round(ceGridThumbSize * ceScaleFactor);
    ceStatusBarWidth = CE_Round(ceStatusBarWidth * ceScaleFactor);
    ceListItemBaseHeight = CE_Round(ceListItemBaseHeight * ceScaleFactor);
}

int CE_Scale(int value)
{
    return CE_Round(value * ceScaleFactor);
}

/* CE bug: the grid hit-test is CE's DIV r8, the 16-bit offset by the cell size's low byte,
   so a cell of 256 px or more (ui_scale 3.08 and up) makes a click divide by 0 or need a
   quotient over 255, and both fault (#DE). */
int CE_GridDivide(int offset, int cell)
{
    unsigned int quotient;

    quotient = (unsigned short)offset / (unsigned char)cell;
    if (quotient > 0xff)
        RaiseException(kCEIntOverflow, 0, 0, NULL);
    return (unsigned char)quotient;
}

void CE_ScaleRect(Rect *r)
{
    if (ceScaleEnabled) {
        r->top = CE_Scale(r->top);
        r->left = CE_Scale(r->left);
        r->bottom = CE_Scale(r->bottom);
        r->right = CE_Scale(r->right);
    }
}

/* The pen position set directly, past MoveTo's own scaling inside dialog items: CE's
   SET_ORIGIN_SCALED sites pass (x - xo + CE_Scale(xo), y - yo + CE_Scale(yo)). */
void CE_SetPen(short h, short v)
{
    /* CE bug: no NULL-port test, as in CE (no caller in play has no port). */
    CurrentGrafPort->pnLoc.h = h;
    CurrentGrafPort->pnLoc.v = v;
}

/* A multipart dialog background or the status bar: scaled, then moved into place. */
void CE_ScaleOffsetRect(Rect *r, int dh, int dv)
{
    CE_ScaleRect(r);
    OffsetRect(r, dh, dv);
}

/* The same for a bottom part, which keeps its bottom edge where it was. */
void CE_ScaleOffsetRectBottom(Rect *r, int dh, int dv)
{
    short bottom;

    bottom = r->bottom;
    CE_ScaleRect(r);
    OffsetRect(r, dh, dv - (r->bottom - bottom));
}

/* The DLOG bounds, scaled and centred on the screen without the original's rounding of
   each half down to even; returns the DITL id. The screen size is taken signed, as CE
   does, so a dialog larger than the screen centres off its top-left edge. */
long CE_SetDialogTemplateBounds(DialogPtr dlg, const unsigned short *tmpl)
{
    short width;
    short height;

    width = CE_Scale(SwapShort(tmpl[3]) - SwapShort(tmpl[1]));
    height = CE_Scale(SwapShort(tmpl[2]) - SwapShort(tmpl[0]));
    dlg->bounds.top = ((long)gVideoHeight - height) / 2;
    dlg->bounds.left = ((long)gVideoWidth - width) / 2;
    dlg->bounds.bottom = dlg->bounds.top + height;
    dlg->bounds.right = dlg->bounds.left + width;
    return SwapShort(tmpl[9]);
}

#endif /* CEFIX */
