// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* swparticles.c -- SpriteWorld's SWParticles.c, the whole object (win 0x0047b7d0-0x0047c800). Notes: Research/notes/swparticles.md */
#include <stddef.h>

#include "gametypes.h"
#include "game.h"
#include "burger.h"
#include "macport.h"
#include "spriteworld.h"

EXTERN_C_BEGIN

#if defined(MACBUILD) && !defined(MAC_GAMEUTILS_ONLY)
#ifdef MAC1010 /* 1.0.10 spelling */
static int gLowestUnusedElement;
static int32_t gLastElementInUse;
static int gNumberOfParticlesActive;
static int gGravity;
static uint32_t gNumArrayElements;
static int32_t scrollOffsetHoriz;
static int32_t scrollOffsetVert;
static ParticlePtr gParticleArray = NULL;
static Boolean gParticlesInitialized = false;
#else
/* tentative, in the image's __bss order (gcc emits these in declaration order; initialised to zero they follow the reference walk) */
static ParticlePtr gParticleArray;
static Boolean gParticlesInitialized;
static int gLowestUnusedElement;
static int32_t gLastElementInUse;
static int gNumberOfParticlesActive;
static int gGravity;
static uint32_t gNumArrayElements;
static int32_t scrollOffsetHoriz;
static int32_t scrollOffsetVert;
#endif
#else
static ParticlePtr gParticleArray = NULL;
static int gLowestUnusedElement = 0;
static int32_t gLastElementInUse = 0;
static int gNumberOfParticlesActive = 0;
static int gGravity = 0;
static uint32_t gNumArrayElements = 0;
static Boolean gParticlesInitialized = 0;
static int32_t scrollOffsetHoriz = 0;
static int32_t scrollOffsetVert = 0;
#endif
#ifndef MACBUILD
static unsigned char DAT_007d053c[260];
#endif

#ifndef MAC_PARTICLES_ONLY
/* RestoreControlStrip. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7d0
// MAC: _RestoreControlStrip i386 0x000c95d6 ppc 0x000d55dc
void RestoreControlStrip(void)
{
#if TARGET_API_MAC_OS8
    if (gControlStripWasVisible) {
        gControlStripWasVisible = false;
        SBShowHideControlStrip(true);
    }
#endif
}

#if TARGET_API_MAC_OS8
Boolean SBIsControlStripVisible(void)
{
    return CallUniversalProc(GetToolTrapAddress(_ControlStripDispatch), 0xd8, 0);
}

void SBShowHideControlStrip(Boolean showIt)
{
    CallUniversalProc(GetToolTrapAddress(_ControlStripDispatch), 0x1c8, 1, showIt);
}
#endif
#endif

#ifndef MAC_GAMEUTILS_ONLY

/* InitParticles (mac _InitParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7e0
// MAC: _InitParticles i386 0x000c4f83 ppc 0x000d11c0
short InitParticles(int numElements, int gravity)
{
    short err;

    err = 0;
    gNumArrayElements = numElements;
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
// MAC: _DisposeParticles i386 0x000c4e80 ppc 0x000d1030
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
// MAC: _ClearParticles i386 0x000c4ec8 ppc 0x000d10a4
void ClearParticles(Boolean active)
{
    int32_t i;

    if (gParticleArray == NULL)
        SW_ASSERT_FAIL("SWParticles.c", 0x5d, 0x5d);
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
// MAC: _SetParticleScrollOffset i386 0x000c5008 ppc 0x000d129c
void SetParticleScrollOffset(int h, int v)
{
    scrollOffsetHoriz = h;
    scrollOffsetVert = v;
}

/* NewParticle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b960
// MAC: _NewParticle i386 0x000c501d ppc 0x000d12b0
void NewParticle(int32_t color, int32_t horizLoc, int32_t vertLoc, int32_t horizSpeed,
                 int32_t vertSpeed, short shade, short lifeCount)
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
// MAC: _EraseParticlesOffscreen i386 0x000c511b ppc 0x000d13c8
void EraseParticlesOffscreen(SpriteWorldPtr spriteWorldP)
{
    Particle *particlePtr;
    FramePtr workFrameP = spriteWorldP->workFrameP,
             backFrameP = spriteWorldP->backFrameP;
    int visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
    int horizLoc, vertLoc;
    int32_t index, count = gLastElementInUse;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SWParticles.c", 185, 185);
    if (workFrameP == 0 || !workFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWParticles.c", 186, 186);
    if (workFrameP->frameDepth < 8)
        SW_ASSERT_FAIL("SWParticles.c", 187, 187);
    if (gParticlesInitialized != 1)
        SW_ASSERT_FAIL("SWParticles.c", 188, 188);

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

#if V111
#ifdef MACBUILD
/* EraseParticlesScrollingOffscreen (mac _EraseParticlesScrollingOffscreen, mac-only). STATE: see the mac gate. */
// MAC: _EraseParticlesScrollingOffscreen i386 0x000c5403 ppc 0x000d16b4
void	EraseParticlesScrollingOffscreen( SpriteWorldPtr spriteWorldP )
{
	Particle	*particlePtr;
	FramePtr	workFrameP = spriteWorldP->workFrameP,
				backFrameP = spriteWorldP->backFrameP;
	int			visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
	int			horizLoc, vertLoc;
	int32_t		index, count = gLastElementInUse;

	SW_ASSERT(spriteWorldP != NULL, 234);
	SW_ASSERT(spriteWorldP->workFrameP != NULL && workFrameP->isFrameLocked, 235);
	SW_ASSERT(workFrameP->frameDepth >= 8, 236);
	SW_ASSERT(gParticlesInitialized == true, 237);

	visBoundsLeft = spriteWorldP->oldVisScrollRect.left;
	visBoundsRight = spriteWorldP->oldVisScrollRect.right;
	visBoundsTop = spriteWorldP->oldVisScrollRect.top;
	visBoundsBottom = spriteWorldP->oldVisScrollRect.bottom;
	FOREACH_DEPTH_DO
	(
		spriteWorldP->pixelDepth,
		for (index = 0, particlePtr = gParticleArray; index <= count; index++, particlePtr++)
		{
			if (particlePtr->lifeRemaining>0)
			{
				horizLoc = (particlePtr->oldHorizLoc >> 8);
				vertLoc = (particlePtr->oldVertLoc >> 8);

				if ((horizLoc<visBoundsRight)&&(horizLoc>visBoundsLeft)&&
					(vertLoc<visBoundsBottom)&&(vertLoc>visBoundsTop))
				{
					horizLoc -= spriteWorldP->backRect.right * (spriteWorldP->oldVisScrollRect.left / spriteWorldP->backRect.right);
					vertLoc -= spriteWorldP->backRect.bottom * (spriteWorldP->oldVisScrollRect.top / spriteWorldP->backRect.bottom);

					if (horizLoc >=  workFrameP->frameRect.right)
						horizLoc -= workFrameP->frameRect.right;
					else if (horizLoc < workFrameP->frameRect.left)
						horizLoc += workFrameP->frameRect.right;

					if (vertLoc >= workFrameP->frameRect.bottom)
						vertLoc -= workFrameP->frameRect.bottom;
					else if (vertLoc < workFrameP->frameRect.top)
						vertLoc += workFrameP->frameRect.bottom;

					SET_DEPTH_PIXEL( workFrameP, horizLoc, vertLoc ,
						GET_DEPTH_PIXEL( backFrameP, horizLoc, vertLoc ) )
				}
			}
		}
	)
}
#endif
#endif

/* DrawParticlesOffscreen (mac _DrawParticlesOffscreen). STATE: MATCH, byte-identical.
   OPEN: none */
// FUNCTION: EVNOVA 0x0047bdd0
// MAC: _DrawParticlesOffscreen i386 0x000c590a ppc 0x000d1b24
void DrawParticlesOffscreen(SpriteWorldPtr spriteWorldP)
{
    Particle *particlePtr;
    FramePtr workFrameP = spriteWorldP->workFrameP;
    int visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
    int horizLoc, vertLoc;
    int32_t index, count = gLastElementInUse;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SWParticles.c", 299, 299);
    if (workFrameP == 0 || !workFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWParticles.c", 300, 300);
    if (workFrameP->frameDepth < 8)
        SW_ASSERT_FAIL("SWParticles.c", 301, 301);
    if (gParticlesInitialized != 1)
        SW_ASSERT_FAIL("SWParticles.c", 302, 302);

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
                    if ((int32_t)particlePtr->shade != 32)
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

#ifdef MACBUILD
/* DrawParticlesScrollingOffscreen (mac _DrawParticlesScrollingOffscreen, mac-only). STATE: see the mac gate. */
// MAC: _DrawParticlesScrollingOffscreen i386 0x000c5cf9 ppc 0x000d1f34
void	DrawParticlesScrollingOffscreen( SpriteWorldPtr spriteWorldP )
{
	Particle	*particlePtr;
	FramePtr	workFrameP = spriteWorldP->workFrameP;
	int			visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
	int			horizLoc, vertLoc;
	int32_t		index, count = gLastElementInUse;

	SW_ASSERT(spriteWorldP != NULL, 372);
	SW_ASSERT(workFrameP != NULL && workFrameP->isFrameLocked, 373);
	SW_ASSERT(workFrameP->frameDepth >= 8, 374);
	SW_ASSERT(gParticlesInitialized == true, 375);

	visBoundsLeft = spriteWorldP->visScrollRect.left;
	visBoundsRight = spriteWorldP->visScrollRect.right;
	visBoundsTop = spriteWorldP->visScrollRect.top;
	visBoundsBottom = spriteWorldP->visScrollRect.bottom;
	FOREACH_DEPTH_DO
	(
		spriteWorldP->pixelDepth,
		for (index = 0, particlePtr = gParticleArray; index <= count; index++, particlePtr++)
		{
			if (particlePtr->lifeRemaining>1)
			{
				horizLoc = (particlePtr->horizLoc >> 8);
				vertLoc = (particlePtr->vertLoc >> 8);

				if ((horizLoc<visBoundsRight)&&(horizLoc>visBoundsLeft)&&
					(vertLoc<visBoundsBottom)&&(vertLoc>visBoundsTop))
				{
					horizLoc -= spriteWorldP->horizScrollRectOffset;
					vertLoc -= spriteWorldP->vertScrollRectOffset;

					if (horizLoc >=  workFrameP->frameRect.right)
						horizLoc -= workFrameP->frameRect.right;
					else if (horizLoc < workFrameP->frameRect.left)
						horizLoc += workFrameP->frameRect.right;

					if (vertLoc >= workFrameP->frameRect.bottom)
						vertLoc -= workFrameP->frameRect.bottom;
					else if (vertLoc < workFrameP->frameRect.top)
						vertLoc += workFrameP->frameRect.bottom;

					SET_DEPTH_PIXEL( workFrameP, horizLoc, vertLoc, particlePtr->color );
				}
			}
		}
	)
}
#endif

/* UpdateParticlesInWindow (mac _UpdateParticlesInWindow). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047c3a0
// MAC: _UpdateParticlesInWindow i386 0x000c61fb ppc 0x000d24b4
#if V111
void UpdateParticlesInWindow(SpriteWorldPtr spriteWorldP)
{
    Particle *particlePtr;
    FramePtr windowFrameP = spriteWorldP->windowFrameP;
    int32_t count = gLastElementInUse;
    int horizLoc, vertLoc;
    int32_t index;

    if (windowFrameP == 0 || !windowFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWParticles.c", 440, 440);
    if (windowFrameP->frameDepth < 8)
        SW_ASSERT_FAIL("SWParticles.c", 441, 441);
    if (gParticlesInitialized != 1)
        SW_ASSERT_FAIL("SWParticles.c", 442, 442);

    FOREACH_DEPTH_DO
    (
        spriteWorldP->pixelDepth,
        for (index = 0, particlePtr = gParticleArray; index <= count; index++, particlePtr++)
        {
            if (particlePtr->lifeRemaining > 0)
            {
                horizLoc = (particlePtr->horizLoc >> 8) - scrollOffsetHoriz;
                vertLoc = (particlePtr->vertLoc >> 8) - scrollOffsetVert;

                particlePtr->drawnHorizLoc = horizLoc;
                particlePtr->drawnVertLoc = vertLoc;
            }
        }
    )

    MoveParticles();
}

#ifdef MACBUILD
/* UpdateParticlesInScrollingWindow (mac _UpdateParticlesInScrollingWindow, mac-only). STATE: see the mac gate. */
// MAC: _UpdateParticlesInScrollingWindow i386 0x000c63ba ppc 0x000d26b4
void UpdateParticlesInScrollingWindow(SpriteWorldPtr spriteWorldP)
{
    SW_ASSERT(gParticlesInitialized == true, 522);
    MoveParticles();
}
#endif
#else
void UpdateParticlesInWindow(SpriteWorldRec *spriteWorldP)
{
    Particle *particlePtr;
    FramePtr workFrameP = spriteWorldP->workFrameP,
             windowFrameP = spriteWorldP->windowFrameP;
    int32_t count = gLastElementInUse;
    int visBoundsLeft, visBoundsRight, visBoundsTop, visBoundsBottom;
    int horizLoc, vertLoc;
    int32_t index;
    Boolean noDirectBlit;

    if (spriteWorldP == 0)
        SW_ASSERT_FAIL("SWParticles.c", 439, 439);
    if (windowFrameP == 0 || !windowFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWParticles.c", 440, 440);
    if (windowFrameP->frameDepth < 8)
        SW_ASSERT_FAIL("SWParticles.c", 441, 441);
    if (gParticlesInitialized != 1)
        SW_ASSERT_FAIL("SWParticles.c", 442, 442);

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
#endif

/* MoveParticles (mac _MoveParticles). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047c800
// MAC: _MoveParticles i386 0x000c60c5 ppc 0x000d2304
void MoveParticles(void)
{
    Particle *p;
    int i;
    int last;

    last = gLastElementInUse;

    if (!gParticlesInitialized)
        return;

    i = 0;
    p = gParticleArray;
    for (; i <= last; i++, p++) {
        if (i >= gNumArrayElements)
            SW_ASSERT_FAIL("SWParticles.c", 0x221, 0x221);
        if (p->lifeRemaining > 0) {
            p->lifeRemaining--;
            if (p->lifeRemaining == 0) {
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

    if (gNumberOfParticlesActive <= 0) {
        gLastElementInUse = 0;
    } else {
        while (gParticleArray[gLastElementInUse].lifeRemaining == 0 && gLastElementInUse > 0)
            gLastElementInUse--;
    }
}
#endif

EXTERN_C_END
