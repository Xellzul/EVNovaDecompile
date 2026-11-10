// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* gameutils.c -- the engine helpers: CenterWindow (empty, two windows). Notes: Research/notes/gameutils.md */
#include <math.h>
#include <stddef.h>

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "burger.h"
#include "crashlog.h"
#include "prototypes.h"

/* three more of the port's compiled-away entry points */

/* CenterWindow (mac _CenterWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00461bc0
// MAC: _CenterWindow i386 0x0000220e ppc 0x00002b08
#ifdef MACBUILD
void CenterWindow(void *front, void *window)
{
    Point center;
    GrafPtr savePort;
#ifdef MAC1010 /* 1.0.10 spelling */
    short halfWidth;
    short halfHeight;
#else
    short halfHeight;
    short halfWidth;
#endif

    if (front == NULL)
        return;
    GetPort(&savePort);
    halfWidth = (GetWindowPortRect(front)->right - GetWindowPortRect(front)->left) / 2;
    halfHeight = (GetWindowPortRect(front)->bottom - GetWindowPortRect(front)->top) / 2;
    if (window != NULL) {
        center.h = (GetWindowPortRect(window)->right - GetWindowPortRect(window)->left) / 2;
        center.v = (GetWindowPortRect(window)->bottom - GetWindowPortRect(window)->top) / 2;
        SetPort((GrafPtr)GetWindowGrafPort(window));
        LocalToGlobal(&center);
    } else {
        center.h = ((*mainScreenDevice)->gdRect.right - (*mainScreenDevice)->gdRect.left) / 2;
        center.v = ((*mainScreenDevice)->gdRect.bottom - (*mainScreenDevice)->gdRect.top) / 2;
    }
    MoveWindow(front, center.h - halfWidth, center.v - halfHeight, false);
    SetPort(savePort);
}
#else
void CenterWindow(void *front, void *window)
{
}
#endif

#ifdef MACBUILD
/* PointToXYShort (mac _PointToXYShort, mac-only). STATE: see the mac gate. */
// MAC: _PointToXYShort i386 0x00002361 ppc 0x00002c70
void PointToXYShort(Point pt, ShortPoint *xy)
{
    xy->h = pt.h;
    xy->v = pt.v;
}

/* XYShortToPoint (mac _XYShortToPoint, mac-only). STATE: see the mac gate. */
// MAC: _XYShortToPoint i386 0x00002378 ppc 0x00002c80
void XYShortToPoint(ShortPoint xy, Point *pt)
{
    pt->h = xy.h;
    pt->v = xy.v;
}
#endif
