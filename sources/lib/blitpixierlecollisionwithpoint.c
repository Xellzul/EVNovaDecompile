// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* blitpixierlecollisionwithpoint.c -- BlitPixieRLECollisionWithPoint (win 0x00472420). Notes: Research/notes/blitpixierlecollisionwithpoint.md */
#include "macport.h"
#include "shipobj.h"

/* BlitPixieRLECollisionWithPoint (mac _BlitPixieRLECollisionWithPoint). STATE: MATCH, exe 103 / obj 103 instrs, byte-identical yes.
   OPEN: none. see notes */
// FUNCTION: EVNOVA 0x00472420
// MAC: _BlitPixieRLECollisionWithPoint i386 0x000c0ebf ppc 0x000ccd04
Boolean BlitPixieRLECollisionWithPoint(unsigned char *rleData, Point *pt, Rect *r)
{
#ifdef MAC1010 /* 1.0.10 spelling */
    uint32_t token;
    uint32_t opcode;
    uint32_t op;
    uint32_t count;
    int32_t runStart;
    int32_t i;
    int32_t x;
#else
    int32_t runStart;
    uint32_t opcode;
    uint32_t op;
    uint32_t count;
    int32_t i;
    uint32_t token;
    int32_t x;
#endif

    for (i = 0; i < pt->v - r->top; i++) {
        token = *(uint32_t *)rleData;
        rleData += 4;
        opcode = token >> 24;
        count = token & 0xffffff;

        if (opcode == 1)
            rleData += count;
        else if (opcode == 0)
            return 0;
    }

    token = *(uint32_t *)rleData;
    rleData += 4;
    op = token >> 24;
    if (op == 0)
        return 0;

    op = -1;
    runStart = x = r->left;

    while (op != 1 && op != 0) {
        token = *(uint32_t *)rleData;
        rleData += 4;
        op = token >> 24;
        count = token & 0xffffff;

        switch (op) {
        case 2:
            x += count;
            if (pt->h >= runStart && pt->h <= x)
                return 1;

            rleData += count;
#if defined(MACBUILD) && V111
            (int32_t)rleData += (-(int32_t)rleData) & 3;
#else
            rleData += (-(int32_t)rleData) & 3;
#endif
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
