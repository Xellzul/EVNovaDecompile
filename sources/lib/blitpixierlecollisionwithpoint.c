// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* blitpixierlecollisionwithpoint.c -- BlitPixieRLECollisionWithPoint (win 0x00472420). Notes: Research/notes/blitpixierlecollisionwithpoint.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieRLECollisionWithPoint (mac _BlitPixieRLECollisionWithPoint). STATE: MATCH, exe 103 / obj 103 instrs, byte-identical yes.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00472420
char BlitPixieRLECollisionWithPoint(unsigned char *rleData, Point *pt, Rect *r)
{
    long runStart;
    unsigned long opcode;
    unsigned long op;
    unsigned long count;
    long i;
    unsigned long token;
    long x;

    for (i = 0; i < pt->v - r->top; i++) {
        token = *(unsigned long *)rleData;
        rleData += 4;
        opcode = token >> 24;
        count = token & 0xffffff;

        if (opcode == 1)
            rleData += count;
        else if (opcode == 0)
            return 0;
    }

    token = *(unsigned long *)rleData;
    rleData += 4;
    op = token >> 24;
    if (op == 0)
        return 0;

    op = -1;
    runStart = x = r->left;

    while (op != 1 && op != 0) {
        token = *(unsigned long *)rleData;
        rleData += 4;
        op = token >> 24;
        count = token & 0xffffff;

        switch (op) {
        case 2:
            x += count;
            if (pt->h >= runStart && pt->h <= x)
                return 1;

            rleData += count;
            rleData += (-(long)rleData) & 3;
            break;

        case 3:
            runStart += count;
            x += count;
            break;

        case 1:
            runStart = x = r->left;
            break;

        case 0:
            return 0;
        }
    }

    return 0;
}
