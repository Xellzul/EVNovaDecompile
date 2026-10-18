// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* trackmainscreenhilite.c -- TrackMainScreenHilite; exe 0x0048c210. Notes: Research/notes/mainscreen.md */

#include <stddef.h>
#include "prototypes.h"
#include "resources.h"
#include "shipobj.h"
#include "gametypes.h"
#include "gameconst.h"
#include "macport.h"
#include "dialogs.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "burger.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "crashlog.h"
#include "game.h"
#define kHiliteStep      4



/* TrackMainScreenHilite. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0048c210
void TrackMainScreenHilite(void)
{
    unsigned long now;
    int i;
    void *sprite;
    short sel;
    Point pt;

    if (inBackground == 0) {
        if (gAnimStep[0] < gAnimEnd[0] || gAnimStep[1] < gAnimEnd[1] ||
            gAnimStep[2] < gAnimEnd[2]) {
            gHiliteLevel = 0;
            goto done;
        }
        now = ReadTick();
        if (now <= gHiliteLastTick)
            goto done;
        if (menuRolloverSprite != 0)
            sel = menuRolloverSprite->numFrames - 1;
        else
            sel = 0;
        gHiliteLastTick = ReadTick();
        GetGlobalMouse(&pt);
        i = 0;
        do {
            sprite = gMainButtonSprites[(short)i];
            if (sprite != 0 && SWRLECollisionWithPoint(sprite, &pt) != 0) {
                sel = i;
                break;
            }
            i = i + 1;
        } while ((short)i < kMainButtons);
        if (sel != gHilitedButton) {
            if (gHiliteLevel > 0 && gSpriteWorld->pixelDepth == kDepth16)
                gHiliteLevel -= kHiliteStep;
            else
                gHilitedButton = sel;
        } else if (gHiliteLevel < kHiliteFull) {
            gHiliteLevel += kHiliteStep;
        }
        if (gHiliteLevel < 0)
            gHiliteLevel = 0;
        if (gHiliteLevel <= kHiliteFull)
            goto done;
    } else if (menuRolloverSprite != 0) {
        gHilitedButton = menuRolloverSprite->numFrames - 1;
    } else {
        gHilitedButton = 0;
    }
    gHiliteLevel = kHiliteFull;
done:
    ;
}
