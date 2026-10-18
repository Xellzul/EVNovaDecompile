// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
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
void CenterWindow(void *front, void *window)
{
}
