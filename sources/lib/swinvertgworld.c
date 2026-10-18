// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swinvertgworld.c -- SWInvertGWorld (win 0x004791b0, donor SpriteWorldUtils.c line 506) and SWInvertRect (win 0x004791d0, donor line 518). Notes: Research/notes/swinvertgworld.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

/* SWInvertGWorld (mac _SWInvertGWorld). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004791b0
void SWInvertGWorld(CGrafPtr gWorldP)
{
    SWInvertRect(gWorldP, GetWindowPortRect(gWorldP));
}

/* SWInvertRect (mac _SWInvertRect). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004791d0
void SWInvertRect(CGrafPtr gWorldP, Rect *rectP)
{
    CGrafPtr saveGWorld;
    void *saveDevice;

    GetPort(&saveGWorld);
    saveDevice = GetGWorldDevice(saveGWorld);
    SetPort(gWorldP);

    InvertRect(rectP);

    SetPort(saveGWorld);
}
