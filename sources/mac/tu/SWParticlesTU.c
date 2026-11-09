/* SWParticlesTU.c -- the mac 1.1.1 SWParticles.o. Notes: Research/notes/spriteworld-mac.md */
#pragma optimization_level 4
#define MAC_PARTICLES_ONLY
#define SW_FILE "SWParticles.c"
#define SW_LINE1010(n) ((n) >= 200 && (n) < 260 ? (n) + 205 : (n))
#include "macport.h"
#include "spriteworld.h"
#include "../include/swutils.h"
#include "../../lib/swparticles.c"
