/* ld64shim.c -- what Apple's build generates or libSystem supplies for ld64-97.17 on Linux. */
#include <stdio.h>
#include <stdlib.h>

const char ldVersionString[] = "@(#)PROGRAM:ld  PROJECT:ld64-97.17 (linux host, tools/mac/build_ld64.sh)\n";

void __assert_rtn(const char *func, const char *file, int line, const char *expr)
{
	fprintf(stderr, "ld: assertion failed: %s, %s:%d: %s\n", func ? func : "?", file, line, expr);
	abort();
}
