/* blitpixie.h -- BlitPixie 1.0's own header as the mac 1.1.1 build saw it: the debug hooks (both empty in release), the version and the CPU enum. */
#define BLITPIXIE_DEBUGGER(x)
#define BLITPIXIE_ASSERT(x) if (!(x)) { BLITPIXIE_DEBUGGER("Assertion failed: " #x " (" __FILE__ ")"); }

#define BLITPIXIE_VERSION_NUM		0x01008000
#define BLITPIXIE_VERSION_STRING	"1.0"
#define BLITPIXIE_VERSION_STRINGLEN	32

enum BlitPixieProcessorTypes
{
	kUnknownProcessor = 0,
	k68000, k68010, k68020, k68030, k68040,
	k601, k603, k604, kG3, kG4
};
