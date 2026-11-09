/* time.h -- stand-in for the MacOSX10.4u SDK header (declarations only) until the SDK is configured. */
#ifndef _TIME_H_
#define _TIME_H_
#include <stddef.h>
typedef long time_t;
typedef unsigned long clock_t;
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
    long tm_gmtoff;
    char *tm_zone;
};
extern time_t time(time_t *);
extern struct tm *localtime(const time_t *);
extern clock_t clock(void);
#endif
