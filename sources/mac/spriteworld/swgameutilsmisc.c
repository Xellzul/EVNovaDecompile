/* swgameutilsmisc.c -- SWGameUtils.o: AlignRect, ResourceExists and the event-mask and menu-bar helpers, stubbed under Carbon (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"

short		gOldEventMask;
Boolean		eventMaskIsGood = false;

/* AlignRect (mac _AlignRect, mac-only). STATE: see the mac gate. */
// MAC: _AlignRect i386 0x000c9478 ppc 0x000d5430
void	AlignRect(Rect* rectP)
{
	short	rectWidth, overflow;

	rectWidth = rectP->right - rectP->left;
	overflow = rectP->left & 3;
	rectP->left = rectP->left>>2<<2;
	if (overflow > 2)
		rectP->left += 4;
	rectP->right = rectP->left + rectWidth;
}

/* ResourceExists (mac _ResourceExists, mac-only). STATE: see the mac gate. */
// MAC: _ResourceExists i386 0x000c94c1 ppc 0x000d546c
Boolean		ResourceExists(ResType resourceType, short resID)
{
	Boolean	doesExist;
	Handle	resourceHandle;

#ifdef MAC1010
	SetResLoad(false);
#endif
	resourceHandle = GetResource(resourceType, resID);
	if (resourceHandle != NULL)
	{
		doesExist = true;
		ReleaseResource(resourceHandle);
	}
	else
		doesExist = false;
#ifdef MAC1010
	SetResLoad(true);
#endif

	return doesExist;
}

/* AllowKeyUpEvents (mac _AllowKeyUpEvents, mac-only). STATE: see the mac gate. */
// MAC: _AllowKeyUpEvents i386 0x000c94f2 ppc 0x000d54a4
void AllowKeyUpEvents( void )
{
	eventMaskIsGood = true;
}

/* RestoreEventMask (mac _RestoreEventMask, mac-only). STATE: see the mac gate. */
// MAC: _RestoreEventMask i386 0x000c94fe ppc 0x000d54b4
void RestoreEventMask( void )
{
}

/* SWHideMenuBar (mac _SWHideMenuBar, mac-only). STATE: see the mac gate. */
// MAC: _SWHideMenuBar i386 0x000c9503 ppc 0x000d54b8
RgnHandle	SWHideMenuBar(GrafPtr grafPort)
{
	HideMenuBar();
	return NULL;
}

/* KeepMenuBarHidden (mac _KeepMenuBarHidden, mac-only). STATE: see the mac gate. */
// MAC: _KeepMenuBarHidden i386 0x000c9512 ppc 0x000d54dc
void KeepMenuBarHidden(GrafPtr grafPort)
{
}

/* SWShowMenuBar (mac _SWShowMenuBar, mac-only). STATE: see the mac gate. */
// MAC: _SWShowMenuBar i386 0x000c9517 ppc 0x000d54e0
void SWShowMenuBar(GrafPtr grafPort)
{
	ShowMenuBar();
}
