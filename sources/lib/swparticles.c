// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* swparticles.c -- SpriteWorld's SWParticles.c, the whole object (win 0x0047b7d0-0x0047c800). Notes: Research/notes/swparticles.md */
#include <stddef.h>

#include "gametypes.h"
#include "game.h"
#include "burger.h"
#include "macport.h"
#include "spriteworld.h"

static ParticlePtr gParticleArray = NULL;
static int gLowestUnusedElement = 0;
static long gLastElementInUse = 0;
static int gNumberOfParticlesActive = 0;
static int gGravity = 0;
static unsigned long gNumArrayElements = 0;
static char gParticlesInitialized = false;
static long scrollOffsetHoriz = 0;
static long scrollOffsetVert = 0;
static unsigned char DAT_007d053c[260];

/* RestoreControlStrip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7d0
void RestoreControlStrip(void)
{
}

/* InitParticles (mac _InitParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7e0
short InitParticles(int numElements, int gravity)
{
    short err;

    gNumArrayElements = numElements;
    err = 0;
    gGravity = gravity;
    numElements = numElements * sizeof(Particle);
    gParticleArray = (ParticlePtr)AllocAPointer(numElements);
    if (gParticleArray == NULL)
        err = memFullErr;
    else
        ClearParticles(0);
    gLowestUnusedElement = 0;
    gLastElementInUse = 0;
    gNumberOfParticlesActive = 0;
    scrollOffsetHoriz = scrollOffsetVert = 0;
    if ((int)err == 0)
        gParticlesInitialized = 1;
    else
        gParticlesInitialized = 0;
    return err;
}

/* DisposeParticles (mac _DisposeParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b870
void DisposeParticles(void)
{
    if (gParticleArray != NULL) {
        DeallocAPointer(gParticleArray);
        gParticleArray = NULL;
        gLowestUnusedElement = 0;
        gLastElementInUse = 0;
        gNumberOfParticlesActive = 0;
    }
    gParticlesInitialized = 0;
}

/* ClearParticles. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b8c0
void ClearParticles(char active)
{
    unsigned long i;

    if (gParticleArray == NULL)
        SWAssertFail("SWParticles.c", 0x5d);
    for (i = 0; i < gNumArrayElements; i++) {
        if (active != 0 && gParticleArray[i].lifeRemaining > 1)
            gParticleArray[i].lifeRemaining = 1;
        else
            gParticleArray[i].lifeRemaining = 0;
    }
    gLowestUnusedElement = 0;
    gLastElementInUse = 0;
    gNumberOfParticlesActive = 0;
}

/* SetParticleScrollOffset (mac _SetParticleScrollOffset). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b940
void SetParticleScrollOffset(int h, int v)
{
    scrollOffsetHoriz = h;
    scrollOffsetVert = v;
}

/* NewParticle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b960
void NewParticle(long color, long horizLoc, long vertLoc, long horizSpeed,
                 long vertSpeed, short shade, short lifeCount)
{
    if (!gParticlesInitialized)
        return;

    if (gNumberOfParticlesActive < gNumArrayElements) {
        while (gParticleArray[gLowestUnusedElement].lifeRemaining > 0)
            gLowestUnusedElement++;

        if (gLowestUnusedElement >= gNumArrayElements || gLowestUnusedElement < 0)
            return;

        gParticleArray[gLowestUnusedElement].lifeRemaining = lifeCount;
        gParticleArray[gLowestUnusedElement].color = color;
        gParticleArray[gLowestUnusedElement].horizSpeed = horizSpeed;
        gParticleArray[gLowestUnusedElement].vertSpeed = vertSpeed;
        gParticleArray[gLowestUnusedElement].horizLoc = horizLoc;
        gParticleArray[gLowestUnusedElement].vertLoc = vertLoc;
        gParticleArray[gLowestUnusedElement].oldHorizLoc = horizLoc;
        gParticleArray[gLowestUnusedElement].oldVertLoc = vertLoc;
        gParticleArray[gLowestUnusedElement].drawnHorizLoc = -1;
        gParticleArray[gLowestUnusedElement].drawnVertLoc = -1;
        gParticleArray[gLowestUnusedElement].shade = shade;

        if (gLowestUnusedElement > gLastElementInUse)
            gLastElementInUse = gLowestUnusedElement;

        gNumberOfParticlesActive++;
        gLowestUnusedElement++;
    }
}

/* EraseParticlesOffscreen (mac _EraseParticlesOffscreen). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047bb30
void EraseParticlesOffscreen(SpriteWorldPtr spriteWorldP)
{
    Particle *particlePtr;
    FramePtr workFrameP = spriteWorldP->workFrameP,
             backFrameP = spriteWorldP->backFrameP;
    int visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
    int horizLoc, vertLoc;
    long index, count = gLastElementInUse;

    if (spriteWorldP == 0)
        SWAssertFail("SWParticles.c", 185);
    if (workFrameP == 0 || !workFrameP->isFrameLocked)
        SWAssertFail("SWParticles.c", 186);
    if (workFrameP->frameDepth < 8)
        SWAssertFail("SWParticles.c", 187);
    if (gParticlesInitialized != 1)
        SWAssertFail("SWParticles.c", 188);

    visBoundsLeft = workFrameP->frameRect.left;
    visBoundsRight = workFrameP->frameRect.right;
    visBoundsTop = workFrameP->frameRect.top;
    visBoundsBottom = workFrameP->frameRect.bottom;

    FOREACH_DEPTH_DO
    (
        spriteWorldP->pixelDepth,
        for (index = 0, particlePtr = gParticleArray; index <= count; index++, particlePtr++)
        {
            if (particlePtr->lifeRemaining>0)
            {
                horizLoc = particlePtr->drawnHorizLoc;
                vertLoc = particlePtr->drawnVertLoc;

                if ((horizLoc<visBoundsRight)&&(horizLoc>visBoundsLeft)&&
                    (vertLoc<visBoundsBottom)&&(vertLoc>visBoundsTop))
                {
                    SET_DEPTH_PIXEL( workFrameP, horizLoc, vertLoc ,
                        GET_DEPTH_PIXEL( backFrameP, horizLoc, vertLoc ) )
                }
            }
        }
    )
}

#define SET_TRANS_PIXEL(frame, x, y, c, level)                                  \
    if (DEPTH == 8)                                                             \
        *SW_PIXELPTR8(frame, x, y) = Trans8Bit(c, *SW_PIXELPTR8(frame, x, y), level); \
    else if (DEPTH == 16)                                                       \
        *SW_PIXELPTR16(frame, x, y) = Trans16Bit(c, *SW_PIXELPTR16(frame, x, y), level); \
    else                                                                        \
        *SW_PIXELPTR32(frame, x, y) = c;

/* DrawParticlesOffscreen (mac _DrawParticlesOffscreen). STATE: MATCH, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x0047bdd0
void DrawParticlesOffscreen(SpriteWorldPtr spriteWorldP)
{
    Particle *particlePtr;
    FramePtr workFrameP = spriteWorldP->workFrameP;
    int visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
    int horizLoc, vertLoc;
    long index, count = gLastElementInUse;

    if (spriteWorldP == 0)
        SWAssertFail("SWParticles.c", 299);
    if (workFrameP == 0 || !workFrameP->isFrameLocked)
        SWAssertFail("SWParticles.c", 300);
    if (workFrameP->frameDepth < 8)
        SWAssertFail("SWParticles.c", 301);
    if (gParticlesInitialized != 1)
        SWAssertFail("SWParticles.c", 302);

    visBoundsLeft = workFrameP->frameRect.left;
    visBoundsRight = workFrameP->frameRect.right;
    visBoundsTop = workFrameP->frameRect.top;
    visBoundsBottom = workFrameP->frameRect.bottom;

    FOREACH_DEPTH_DO
    (
        spriteWorldP->pixelDepth,
        for (index = 0, particlePtr = gParticleArray; index <= count; index++, particlePtr++)
        {
            if (particlePtr->lifeRemaining>1)
            {
                horizLoc = (particlePtr->horizLoc >> 8) - scrollOffsetHoriz;
                vertLoc = (particlePtr->vertLoc >> 8) - scrollOffsetVert;

                if ((horizLoc<visBoundsRight)&&(horizLoc>visBoundsLeft)&&
                (vertLoc<visBoundsBottom)&&(vertLoc>visBoundsTop))
                {
                    if (particlePtr->shade != 32)
                    {
                        if (particlePtr->lifeRemaining > 32)
                            SET_DEPTH_PIXEL( workFrameP, horizLoc, vertLoc,
                                particlePtr->color)
                        else
                            SET_TRANS_PIXEL( workFrameP, horizLoc, vertLoc,
                                particlePtr->color, particlePtr->lifeRemaining)
                    }
                    else
                    {
                        if (particlePtr->lifeRemaining > particlePtr->shade)
                            SET_TRANS_PIXEL( workFrameP, horizLoc, vertLoc,
                                particlePtr->color, particlePtr->shade)
                        else
                            SET_TRANS_PIXEL( workFrameP, horizLoc, vertLoc,
                                particlePtr->color, particlePtr->lifeRemaining)
                    }
                }
            }
        }
    )
}

/* UpdateParticlesInWindow (mac _UpdateParticlesInWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047c3a0
void UpdateParticlesInWindow(SpriteWorldRec *spriteWorldP)
{
    Particle *particlePtr;
    FramePtr workFrameP = spriteWorldP->workFrameP,
             windowFrameP = spriteWorldP->windowFrameP;
    long count = gLastElementInUse;
    int visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
    int horizLoc, vertLoc;
    long index;
    char noDirectBlit;

    if (spriteWorldP == 0)
        SWAssertFail("SWParticles.c", 439);
    if (windowFrameP == 0 || !windowFrameP->isFrameLocked)
        SWAssertFail("SWParticles.c", 440);
    if (windowFrameP->frameDepth < 8)
        SWAssertFail("SWParticles.c", 441);
    if (gParticlesInitialized != 1)
        SWAssertFail("SWParticles.c", 442);

    noDirectBlit = (spriteWorldP->fastBlit == 0);

    visBoundsLeft = windowFrameP->frameRect.left;
    visBoundsRight = windowFrameP->frameRect.right;
    visBoundsTop = windowFrameP->frameRect.top;
    visBoundsBottom = windowFrameP->frameRect.bottom;

    FOREACH_DEPTH_DO
    (
        spriteWorldP->pixelDepth,
        for (index = 0, particlePtr = gParticleArray; index <= count; index++, particlePtr++)
        {
            if (particlePtr->lifeRemaining > 0)
            {
                horizLoc = particlePtr->drawnHorizLoc;
                vertLoc = particlePtr->drawnVertLoc;

                if ((horizLoc<visBoundsRight)&&(horizLoc>visBoundsLeft)&&
                    (vertLoc<visBoundsBottom)&&(vertLoc>visBoundsTop)&&!noDirectBlit)
                {
                    SET_DEPTH_PIXEL( windowFrameP, horizLoc, vertLoc ,
                        GET_DEPTH_PIXEL( workFrameP, horizLoc, vertLoc ) )
                }

                horizLoc = (particlePtr->horizLoc >> 8) - scrollOffsetHoriz;
                vertLoc = (particlePtr->vertLoc >> 8) - scrollOffsetVert;

                particlePtr->drawnHorizLoc = horizLoc;
                particlePtr->drawnVertLoc = vertLoc;

                if ((horizLoc<visBoundsRight)&&(horizLoc>visBoundsLeft)&&
                    (vertLoc<visBoundsBottom)&&(vertLoc>visBoundsTop)&&!noDirectBlit)
                {
                    SET_DEPTH_PIXEL( windowFrameP, horizLoc, vertLoc ,
                        GET_DEPTH_PIXEL( workFrameP, horizLoc, vertLoc ) )
                }
            }
        }
    )

    MoveParticles();
}

/* MoveParticles (mac _MoveParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047c800
void MoveParticles(void)
{
    Particle *p;
    int i;
    int last;

    last = gLastElementInUse;

    if (!gParticlesInitialized)
        return;

    p = gParticleArray;
    for (i = 0; i <= last; i++, p++) {
        if (i >= gNumArrayElements)
            SWAssertFail("SWParticles.c", 0x221);
        if (p->lifeRemaining > 0) {
            if (--p->lifeRemaining == 0) {
                gNumberOfParticlesActive--;
                if (i < gLowestUnusedElement)
                    gLowestUnusedElement = i;
            } else {
                p->oldHorizLoc = p->horizLoc;
                p->oldVertLoc = p->vertLoc;
                p->horizLoc += p->horizSpeed;
                p->vertLoc += p->vertSpeed;
                p->vertSpeed += gGravity;
            }
        }
    }

    if (gNumberOfParticlesActive > 0) {
        while (gParticleArray[gLastElementInUse].lifeRemaining == 0 && gLastElementInUse > 0)
            gLastElementInUse--;
    } else {
        gLastElementInUse = 0;
    }
}
