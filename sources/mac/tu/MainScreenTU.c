/* MainScreenTU.c -- the mac 1.1.1 Main Screen.o: gameview.c through mainscreenrollover.c in win link order (mac 1.0.10's source order of Main Screen.c). Notes: Research/notes/mainscreen.md */
#include "../include/carbonstatics.h"
#include "int32.h"
#ifdef MAC1010 /* 1.0.10 spelling: Main Screen.c's head, ahead of every literal and anonymous object */
char reggieVersion[256] = "";
Boolean gUpdateFlag = 0;
uint32_t reggieStarted = 0;
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
int32_t newSoundVol;
unsigned char origLicenseName[256];
PicHandle prefPic[5];
Rect screenMinusDockRect;
PicHandle sliderPic[2];
int32_t sliderPos;
DialogPtr speedDialog;
