// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* licensescramble.c -- ScrambleRand, an RT3 license-scramble helper only InitEVNova calls; exe 0x004ab970, the object after music.c's .CRT hook 0x004ab960. Notes: Research/notes/music.md */

#include "macport.h"
#include "game.h"
#include "sound.h"
#include "player.h"
#include "render.h"
#include "burger.h"
#include "gameutil.h"

/*
 * Not music code: the only caller is InitEVNova, discarding the result -
 * a member of the RT3 registration-library scramble family (the others sit
 * in parser.c, commdlg.c and gameview.c). No mac symbol exists for it.
 */

/* ScrambleRand. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab970
short ScrambleRand(void)
{
    return Rand(0);
}
