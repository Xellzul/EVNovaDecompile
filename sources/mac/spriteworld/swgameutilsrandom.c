/* swgameutilsrandom.c -- SWGameUtils.o: the random-number helpers (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"

/* Randomize (mac _Randomize, mac-only). STATE: see the mac gate. */
// MAC: _Randomize i386 0x000c9346 ppc 0x000d52e4
void Randomize( void )
{
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
	GetDateTime( (uint32_t *)&qd.randSeed );
#else
	uint32_t	secs;

	GetDateTime( &secs );
	SetQDGlobalsRandomSeed( secs );
#endif
}

/* GetRandom (mac _GetRandom, mac-only). STATE: see the mac gate. */
// MAC: _GetRandom i386 0x000c9364 ppc 0x000d5310
short GetRandom(short min, short max)
{
	unsigned short		random;
	int32_t				range, temp;

	random = Random();
	range = (max - min) + 1;
	temp = (random * range) / 65536;
	random = temp + min;

	return random;
}

/* GetUnsignedRandom (mac _GetUnsignedRandom, mac-only). STATE: see the mac gate. */
// MAC: _GetUnsignedRandom i386 0x000c93a5 ppc 0x000d5360
unsigned short	GetUnsignedRandom(unsigned short min, unsigned short max)
{
	unsigned short	random;
	uint32_t	range, temp;

	random = Random();
	range = (max - min) + 1;
	temp = (random * range) / 65536;
	random = temp + min;

	return random;
}
