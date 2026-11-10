// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* getgdevicedepth.c -- GetGDeviceDepth (win 0x0047b7c0, donor SWGameUtils.c line 372). Notes: Research/notes/getgdevicedepth.md */
#include "macport.h"

/* GetGDeviceDepth (mac _GetGDeviceDepth). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7c0
// MAC: _GetGDeviceDepth i386 0x000c9520 ppc 0x000d54e4
#ifdef MACBUILD
short GetGDeviceDepth( GDHandle theGDH )
{
	long			response;
	PixMapHandle	thePixMap;

	Gestalt(gestaltQuickdrawFeatures, &response);
	if ( !(response & (1 << gestaltHasColor)) )
		return 1;
	else
	{
		thePixMap = (**theGDH).gdPMap;
		return (**thePixMap).pixelSize;
	}
}
#else
short GetGDeviceDepth(void *theGDH)
{
    return 16;
}
#endif
