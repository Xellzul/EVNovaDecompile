// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* getgdevicedepth.c -- GetGDeviceDepth (win 0x0047b7c0, donor SWGameUtils.c line 372). Notes: Research/notes/getgdevicedepth.md */
#include "macport.h"

/* GetGDeviceDepth (mac _GetGDeviceDepth). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0047b7c0
short GetGDeviceDepth(void *theGDH)
{
    return 16;
}
