// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* trigtables.c -- angle-indexed trig tables, SinLookup/CosLookup, and the arctan ramp TableArcTan's atan2-by-table helper walks (win 0x00461830..0x004619b0). Notes: Research/notes/trigtables.md */
#include <math.h>
#include "gameconst.h"
#include "game.h"
#include "space.h"

#define kDegToRad   0.01745329
#define kArctanStep 0.01
#define kRadToDeg   57.2957795

float sinTable[360];
float cosTable[360];
float gTanTable[360];
float trigResult;
short gArctanTable[0x400];
unsigned char DAT_007cc48c[260];

/* BuildTrigTable (mac _BuildTrigTable). STATE: MATCH 100.0%, exe 79 / obj 79 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00461830
void BuildTrigTable(void)
{
    long i;
    float ang;
    double v;

    for (i = 0; i <= kFullCircle - 1; i++) {
        ang = (i * kDegToRad);
        sinTable[i] = sin((double)ang);
        cosTable[i] = cos((double)ang);
        gTanTable[i] = tan((double)ang);
    }
    for (i = 0; i <= 0x3ff; i++) {
        v = atan2(i * kArctanStep, 1.0);
        v = v * kRadToDeg;
        gArctanTable[i] = v;
    }
}


/* SinLookup. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00461930
void SinLookup(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = sinTable[angle];
}

/* CosLookup. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00461970
void CosLookup(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = cosTable[angle];
}

#define kArctanScale   100.0f
#define kArctanLast    0x3ff

/* TableArcTan. STATE: MATCH 100.0%, exe 164 / obj 164 instrs,
   byte-identical yes.
   OPEN: none. It RETURNS short: the exe's `mov eax,ecx' where an int return
   needs `movsx eax,cx' is the tell, and the whole tail's colouring was
   downstream of it. see notes */
// FUNCTION: EVNOVA 0x004619b0
short TableArcTan(float dx, float dy)
{
    float ratio;
    float ax;
    float ay;
    short idx;
    short ang;

    if (dx == 0.0 && dy == 0.0)
        return 0;

    if (dx < 0.0)
        ax = dx * -1.0;
    else
        ax = dx;
    if (dy < 0.0)
        ay = dy * -1.0;
    else
        ay = dy;
    if (ax < ay)
        ratio = ax / ay;
    else
        ratio = ay / ax;
    if (ratio < 0.0f)
        ratio = -ratio;

    idx = (ratio * kArctanScale);
    if (idx < 0)
        idx = 0;
    if (idx > kArctanLast)
        idx = kArctanLast;

    ang = gArctanTable[idx];
    if (ang < 0)
        ang = -ang;
    if (ax < ay)
        ang = kQuarterCircle - ang;
    if (dx < 0.0f && dy >= 0.0f)
        ang = kHalfCircle - ang;
    if (dx < 0.0f && dy < 0.0f)
        ang = ang + kHalfCircle;
    if (dx >= 0.0f && dy < 0.0f)
        ang = -ang;

    ang -= kQuarterCircle;
    if (ang < 0)
        ang += kFullCircle;
    if (ang >= kFullCircle)
        ang -= kFullCircle;
    return ang;
}
