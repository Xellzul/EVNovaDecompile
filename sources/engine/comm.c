// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* comm.c -- the communication message strip (win TU region 0x0047e2d0-0x0047e463). Notes: Research/notes/comm.md */
/*
 * The two SpriteWorld frame offsets are the real ones from SpriteWorld's own
 * SpriteWorld.h - backFrameP at +0x14, workFrameP at +0x18, each a FramePtr
 * whose first field is the CGrafPtr the port hands to CopyBits.
 */
#include <stddef.h>

#include "burger.h"
#include "macport.h"
#include "game.h"
#include "missions.h"
#include "render.h"
#include "stellar.h"

#define kCommMsgMax     0xff
#define kCommTextMax    0xfe
#define kTextFaceBold   2
#define kCommTextSize   12

/* DisplayComm (mac _DisplayComm). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e2d0
// MAC: _DisplayComm i386 0x00090201 ppc 0x0009be7c
void DisplayComm(PStrChar *msg, short count, short font, short size,
                 RGBColor *color, Boolean bold, unsigned char noBlit)
{
    unsigned char text[256];

    if (playerKilledByHector != 0)
        return;
    commCount = count;
    strncpy(commMsg, (char *)msg, kCommMsgMax);
    strncpy((char *)text, (char *)msg, kCommTextMax);
    PtoCstr(text);
    SWSetPortToBackground(gSpriteWorld);
    RGBForeColor(color);
    TextFont(font);
    TextSize(size);
    if (bold != 0)
        TextFace(kTextFaceBold);
    SetSystemBackgroundBackColor();
    TETextBox(text, strlen((char *)text), &commRect, 0);
    ForeColor(blackColor);
    BackColor(whiteColor);
    SWSetPortToWindow(gSpriteWorld);
    if (noBlit == 0) {
        ForeColor(blackColor);
        BackColor(whiteColor);
        CopyBits(GetPortPixMapBits(
                     gSpriteWorld->backFrameP->framePort),
                 GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
                 &commRect, &commRect, 0, NULL);
    }
}

/* the stored count doubles as a "there is a message" flag */
/* RedrawComm (mac _RedrawComm). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047e430
// MAC: _RedrawComm i386 0x000903a0 ppc 0x0009bfdc
void RedrawComm(unsigned char noBlit)
{
    if (commCount > 0)
        DisplayComm((PStrChar *)commMsg, commCount, 0, kCommTextSize, &rgbWhite, 0,
                    noBlit);
}
