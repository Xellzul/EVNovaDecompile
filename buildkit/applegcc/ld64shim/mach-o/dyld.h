#ifndef SHIM_DYLD_H
#define SHIM_DYLD_H
#include <unistd.h>
#include <stdint.h>
static inline int _NSGetExecutablePath(char *b, uint32_t *n)
{
	ssize_t r = readlink("/proc/self/exe", b, *n - 1);
	if (r < 0) return -1;
	b[r] = 0;
	return 0;
}
#endif
