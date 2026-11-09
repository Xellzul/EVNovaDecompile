/* MainScreenTU.c -- the mac 1.1.1 Main Screen.o: gameview.c through mainscreenrollover.c in win link order (mac 1.0.10's source order of Main Screen.c). Notes: Research/notes/mainscreen.md */
#include "../include/carbonstatics.h"
#ifdef MAC1010 /* 1.0.10 spelling: Main Screen.c's head, ahead of every literal and anonymous object */
char reggieVersion[256] = "";
Boolean gUpdateFlag = 0;
unsigned long reggieStarted = 0;
Boolean reggieSuccess = 0;
#endif
#include "../../engine/gameview.c"
#include "../../engine/mainscreen.c"
#include "../../engine/maininfo.c"
#include "../../engine/newpilotdlg.c"
#include "../../engine/showintroscreen.c"
#include "../../engine/keydlg.c"
#include "../../engine/dlgsound.c"
#include "../../engine/trackmainscreenhilite.c"
#include "../../engine/mainscreenpict.c"
#include "../../engine/mainscreenrollover.c"
PicHandle aboutPic;
Rect borderRect;
MenuHandle editMenu;
MenuHandle fileMenu;
long newSoundVol;
unsigned char origLicenseName[256];
PicHandle prefPic[5];
Rect screenMinusDockRect;
PicHandle sliderPic[2];
long sliderPos;
DialogPtr speedDialog;
