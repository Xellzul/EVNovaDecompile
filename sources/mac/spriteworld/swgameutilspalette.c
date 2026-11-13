/* swgameutilspalette.c -- SWGameUtils.o: RestoreSystemPalette, HasSystem7 and HideControlStrip (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"

/* RestoreSystemPalette (mac _RestoreSystemPalette, mac-only). STATE: see the mac gate. */
// MAC: _RestoreSystemPalette i386 0x000c9554 ppc 0x000d553c
void	RestoreSystemPalette( void )
{
	RestoreDeviceClut(nil);
	PaintBehind((FrontWindow)(), GetGrayRgn());
	PaintOne(nil, GetGrayRgn());
	DrawMenuBar();
}

/* HasSystem7 (mac _HasSystem7, mac-only). STATE: see the mac gate. */
// MAC: _HasSystem7 i386 0x000c959e ppc 0x000d5590
Boolean		HasSystem7( void )
{
	int32_t	gestaltResponse;
	OSErr	err;

	err = Gestalt( gestaltSystemVersion, &gestaltResponse );
	return ( err == noErr && ((uint32_t)gestaltResponse >= 0x0700));
}

/* HideControlStrip (mac _HideControlStrip, mac-only). STATE: see the mac gate. */
// MAC: _HideControlStrip i386 0x000c95d1 ppc 0x000d55d8
#if TARGET_API_MAC_OS8
#define _Unimplemented 0xA89F
#define _ControlStripDispatch 0xAAF2
extern Boolean gControlStripWasVisible;
Boolean SBIsControlStripVisible(void);
void SBShowHideControlStrip(Boolean showIt);
#endif

void HideControlStrip( void )
{
#if TARGET_API_MAC_OS8
	int32_t	response;

	if (Gestalt('sdev', &response) != noErr || (response & 1) == 0)
		return;
	if (GetToolTrapAddress(_ControlStripDispatch) != GetToolTrapAddress(_Unimplemented) && SBIsControlStripVisible())
	{
		gControlStripWasVisible = true;
		SBShowHideControlStrip(false);
	}
#endif
}
