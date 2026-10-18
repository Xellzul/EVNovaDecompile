// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swcreatespritelayer.c -- SWCreateSpriteLayer (win 0x004770d0, donor SpriteLayer.c line 39) and SWDisposeSpriteLayer (win 0x00477120, donor line 73). Notes: Research/notes/swcreatespritelayer.md */
/* The fork's SpriteLayerRec is 0x1c bytes, not the donor's 0x18: a 4-byte member at
   +0x10 (written only by SWSetLayerCallBack) pushes tileLayer to +0x18, isPaused to
   +0x1a, layerIsNonScrolling to +0x1b and userData to +0x14. */
#include "macport.h"
#include "shipobj.h"
#include "burger.h"
#include "spriteworld.h"

/* SWCreateSpriteLayer (mac _SWCreateSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004770d0
short SWCreateSpriteLayer(SpriteLayerRec **spriteLayerP)
{
    short err;
    SpriteLayerRec *tempSpriteLayerP;

    err = 0;
    *spriteLayerP = 0;

    tempSpriteLayerP = (SpriteLayerRec *)AllocAPointerClear(sizeof(SpriteLayerRec));

    if (tempSpriteLayerP != 0) {
        tempSpriteLayerP->tileLayer = 10;
        tempSpriteLayerP->isPaused = 0;
        tempSpriteLayerP->layerIsNonScrolling = 0;
        tempSpriteLayerP->headSpriteP = 0;
        tempSpriteLayerP->layerCallBack = 0;

        *spriteLayerP = tempSpriteLayerP;
    } else {
        err = memFullErr;
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWDisposeSpriteLayer (mac _SWDisposeSpriteLayer). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00477120
void SWDisposeSpriteLayer(SpriteLayerRec **spriteLayerPP)
{
    SpriteLayerRec *spriteLayerP = *spriteLayerPP;

    if (spriteLayerP != 0) {
        DeallocAPointer(spriteLayerP);
        *spriteLayerPP = 0;
    }
}
