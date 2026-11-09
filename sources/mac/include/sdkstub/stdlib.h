/* stdlib.h -- stand-in for the MacOSX10.4u SDK header (declarations only) until the SDK is configured. */
#ifndef _STDLIB_H_
#define _STDLIB_H_
#include <stddef.h>
#define RAND_MAX 0x7fffffff
int abs(int);
long labs(long);
double atof(const char *);
int atoi(const char *);
long atol(const char *);
long strtol(const char *, char **, int);
void *calloc(size_t, size_t);
void free(void *);
void *malloc(size_t);
void *realloc(void *, size_t);
int atexit(void (*)(void));
void exit(int) __attribute__((__noreturn__));
void abort(void) __attribute__((__noreturn__));
int rand(void);
void srand(unsigned);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
#endif
