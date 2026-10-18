// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* construct8bitalphatable.c -- Construct8BitAlphaTable (win 0x004740d0). Notes: Research/notes/construct8bitalphatable.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"

#define kAlphaTableType     0x54748a62      /* 'Tt\xe4b' */
#define kAlphaTableID       128


/* The resource is five 256x256 byte blocks; one block is one blend level. */
typedef unsigned char AlphaBlock[256][256];


/* Construct8BitAlphaTable (mac _Construct8BitAlphaTable). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004740d0
short Construct8BitAlphaTable(void)
{
    Handle tableH;
    short level, i, j;

    tableH = GetResource(kAlphaTableType, kAlphaTableID);

    if (tableH == 0)
        return resNotFound;

    LockAHandle(tableH);

    for (level = 0; level < 5; level++) {
        for (i = 0; i < 256; i++) {
            alphaTable[level][i] = (unsigned char *)AllocAPointer(256);

            for (j = 0; j < 256; j++) {
                alphaTable[level][i][j] =
                        ((AlphaBlock *)*tableH)[4 - level][i][j];
            }
        }
    }

    ReleaseResource(tableH);

    return noErr;
}
