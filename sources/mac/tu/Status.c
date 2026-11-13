/* Status.c -- the mac 1.1.1 Status.o: syststate.c, statusradar.c, statusparts.c and statusbar.c. Notes: Research/notes/statusbar.md */
#include "../../engine/syststate.c"
#ifdef MAC1010 /* 1.0.10's object order: statusbar.c's two come first and call the rest with `bl; nop` */
#include "../../engine/statusbar.c"
#include "../../engine/statusradar.c"
#include "../../engine/statusparts.c"
#else
#include "../../engine/statusradar.c"
#include "../../engine/statusparts.c"
#include "../../engine/statusbar.c"
#endif
int32_t lastJumpFlash;
int32_t lastJumpHilite;
