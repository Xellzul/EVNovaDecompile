/* blitpixieutils.c -- BlitPixieUtils.o up to the memory routines: the version and the CPU probe (mac only). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "spriteworld.h"
#include "../include/blitpixie.h"

short gBlitPixieProcessorType = kUnknownProcessor;

/* BlitPixieVersion (mac _BlitPixieVersion, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieVersion i386 0x000c02d0 ppc 0x000cbf20
uint32_t BlitPixieVersion( void )
{
	return BLITPIXIE_VERSION_NUM;
}

/* BlitPixieVersionString (mac _BlitPixieVersionString, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieVersionString i386 0x000c02da ppc 0x000cbf2c
void BlitPixieVersionString( char string[BLITPIXIE_VERSION_STRINGLEN] )
{
	char	*p,*version = BLITPIXIE_VERSION_STRING;

	for ( p = version; *p; *string++ = *p++ )
		{}
}

/* BlitPixieGetProcessorType (mac _BlitPixieGetProcessorType, mac-only). STATE: see the mac gate. */
// MAC: _BlitPixieGetProcessorType i386 0x000c02fe ppc 0x000cbf64
void BlitPixieGetProcessorType(void)
{
	OSErr		err;
	int32_t		response;
	short		processor;

	err = Gestalt(gestaltSysArchitecture,&response);
	if ( err != noErr )
		return;

	if ( response == gestalt68k )
	{
		err = Gestalt(gestaltProcessorType,&response);
		if ( err != noErr )
			return;

		switch (response)
		{
			case gestalt68000:		processor = k68000;		break;
			case gestalt68010:		processor = k68010;		break;
			case gestalt68020:		processor = k68020;		break;
			case gestalt68030:		processor = k68030;		break;
			case gestalt68040:		processor = k68040;		break;
			default:				return;
		}
	}
	else if ( response == gestaltPowerPC )
	{
		err = Gestalt(gestaltNativeCPUtype,&response);
		if ( err != noErr )
			return;

		switch (response)
		{
			case gestaltCPU601:		processor = k601;		break;
			case gestaltCPU603:		processor = k603;		break;
			case gestaltCPU603e:
			case gestaltCPU603ev:
									processor = k603;		break;

			case gestaltCPU604:
			case gestaltCPU604e:
			case gestaltCPU604ev:
									processor = k604;		break;
			case gestaltCPU750:
									processor = kG3;		break;

			default:				return;
		}
	}

	gBlitPixieProcessorType = processor;
}
