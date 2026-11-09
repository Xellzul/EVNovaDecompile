/* SetupTU.c -- the mac 1.1.1 Setup.o: all of setup.c (window, arrays, sprites, sounds, objects, player reset, Bye). Notes: Research/notes/setup.md */
#define SW_LINE1010(n) ((n) < 400 ? (n) + 16 : (n) < 1500 ? (n) - 300 : (n) - 296)
#include "../../engine/setup.c"
