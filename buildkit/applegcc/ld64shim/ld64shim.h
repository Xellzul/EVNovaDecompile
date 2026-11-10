/* ld64shim.h -- force-included host plumbing for the Linux build of ld64-97.17 (tools/mac/build_ld64.sh).
   Nothing here decides linker output: BSD string helpers, Mach timing stubs. */
#ifndef LD64SHIM_H
#define LD64SHIM_H
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#define AR_EFMT1 "#1/"
#ifdef __cplusplus
extern "C" {
#endif
static inline size_t strlcpy(char *d, const char *s, size_t n)
{
	size_t l = strlen(s);
	if (n) { size_t c = l >= n ? n - 1 : l; memcpy(d, s, c); d[c] = 0; }
	return l;
}
static inline size_t strlcat(char *d, const char *s, size_t n)
{
	size_t dl = strnlen(d, n);
	if (dl == n) return n + strlen(s);
	return dl + strlcpy(d + dl, s, n - dl);
}
#ifdef __cplusplus
}
#endif
#endif
