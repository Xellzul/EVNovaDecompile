/* swgameutilsdepth.c -- SWGameUtils.o: the screen-depth queries (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"

/* GetDepthFromGlobalRect (mac _GetDepthFromGlobalRect, mac-only). STATE: see the mac gate. */
// MAC: _GetDepthFromGlobalRect i386 0x000c95db ppc 0x000d55e0
short GetDepthFromGlobalRect( Rect* globalRect )
{
	long			response;
	GDHandle		rectGDH;
	short			depth;

	Gestalt(gestaltQuickdrawFeatures, &response);
	if ( !(response & (1 << gestaltHasColor)) )
	{
		depth = 1;
	}
	else
	{
		rectGDH = GetMaxDevice( globalRect );
		depth = GetGDeviceDepth( rectGDH );
	}

	return depth;
}

/* GetDepthFromWindow (mac _GetDepthFromWindow, mac-only). STATE: see the mac gate. */
// MAC: _GetDepthFromWindow i386 0x000c963d ppc 0x000d5668
short GetDepthFromWindow( WindowPtr theWindow )
{
	GWorldPtr		oldGWorld, windowGWld;
	GDHandle		oldGDH, windowGDH;
	short			depth;
	long			response;

	Gestalt(gestaltQuickdrawFeatures, &response);
	if ( !(response & (1 << gestaltHasColor)) )
		depth = 1;
	else
	{
		GetGWorld( &oldGWorld, &oldGDH );
		SetPort( (GrafPtr)GetWindowPort( theWindow ) );
		GetGWorld( &windowGWld, &windowGDH );
		depth = GetGDeviceDepth( windowGDH );
		SetGWorld( oldGWorld, NULL );
	}

	return depth;
}
