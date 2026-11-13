/* Intro.c -- the mac 1.1.1 Intro.o: fadeblack.c, loadbar.c and music.c in win link order. Notes: Research/notes/loadbar.md */
#include "../../engine/fadeblack.c"
#include "../../engine/loadbar.c"
#include "../../engine/music.c"
short fireEndIndex;
GWorldContainer firePartsPort;
int32_t introMovieFinished;
PixMapHandle loadPixMap;
int32_t musicChannel;
PixMapHandle partsPixMap;
CTabHandle splashClut;
PixMapHandle winPixMap;
PixMapHandle workPixMap;
