// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
// COMPILE-WHY: -lang c++ -bool off, the game's C++ build (user decision 10, 2026-10-13): C++ linkage puts a TU's globals in place in .bss, bool off keeps a comparison's value int-typed (parser.c); Research/compiler-inventory.md
/* trigtables.c -- angle-indexed trig tables, TableSin/TableCos, and the arctan ramp TableArcTan's atan2-by-table helper walks (win 0x00461830..0x004619b0). Notes: Research/notes/trigtables.md */
#include <math.h>
#include "gameconst.h"
#include "game.h"
#include "space.h"

#define kDegToRad   0.01745329
#define kArctanStep 0.01
#define kRadToDeg   57.2957795

float tableSin[360];
float tableCos[360];
float tableTan[360];
float trigResult;
short tableArcTan[0x400];
#ifndef MACBUILD
unsigned char DAT_007cc48c[260];
#endif

/* BuildTrigTable (mac _BuildTrigTable). STATE: MATCH 100.0%, exe 79 / obj 79 instrs,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00461830
// MAC: _BuildTrigTable i386 0x00025828 ppc 0x00029e50
void BuildTrigTable(void)
{
    int32_t i;
    float ang;
    double v;

    for (i = 0; i <= kFullCircle - 1; i++) {
        v = ang = (float)i * kDegToRad;
        tableSin[i] = sin(v);
        tableCos[i] = cos(v);
        tableTan[i] = tan(v);
    }
    for (i = 0; i <= 0x3ff; i++) {
        v = atan(i * kArctanStep);
        v *= kRadToDeg;
        tableArcTan[i] = v;
    }
}


/* TableSin. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00461930
// MAC: _TableSin i386 0x0002590a ppc 0x00029fa0
void TableSin(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = tableSin[angle];
}

/* TableCos. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00461970
// MAC: _TableCos i386 0x0002593c ppc 0x00029fe8
void TableCos(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = tableCos[angle];
}

/* TableTan (mac _TableTan, dead on win: uncalled, the linker strips it). STATE: see the mac
   baseline. OPEN: none */
// MAC: _TableTan i386 0x0002596e ppc 0x0002a030
void TableTan(short angle)
{
    while (angle < 0)
        angle += kFullCircle;
    while (angle >= kFullCircle)
        angle -= kFullCircle;
    trigResult = tableTan[angle];
}

#define kArctanScale   100.0f
#define kArctanLast    0x3ff

/* TableArcTan. STATE: MATCH 100.0%, exe 164 / obj 164 instrs,
   byte-identical yes.
   OPEN: none. It RETURNS short: the exe's `mov eax,ecx' where an int return
   needs `movsx eax,cx' is the tell, and the whole tail's colouring was
   downstream of it. see notes */
// FUNCTION: EVNOVA 0x004619b0
// MAC: _TableArcTan i386 0x000259a0 ppc 0x0002a078
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

    ang = tableArcTan[idx];
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
