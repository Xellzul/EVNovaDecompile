/* stdio.h -- stand-in for the MacOSX10.4u SDK header (declarations only) until the SDK is configured. */
#ifndef _STDIO_H_
#define _STDIO_H_
#include <stddef.h>
#include <stdarg.h>
typedef struct __sFILE FILE;
extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;
#define stdin __stdinp
#define stdout __stdoutp
#define stderr __stderrp
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
FILE *fopen(const char *, const char *);
int fclose(FILE *);
int fflush(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
int fprintf(FILE *, const char *, ...);
int printf(const char *, ...);
int sprintf(char *, const char *, ...);
int vsprintf(char *, const char *, va_list);
int vfprintf(FILE *, const char *, va_list);
int fputs(const char *, FILE *);
int fputc(int, FILE *);
int fgetc(FILE *);
char *fgets(char *, int, FILE *);
#endif
