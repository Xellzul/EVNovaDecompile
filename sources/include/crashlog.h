/*
 * crashlog.h — debug logging (win TU 0x00470000-0x004703d3).
 *
 * The log is only written when the user pre-creates the log file next to the
 * game: crashlog_open() first probes with fopen(name, "r") and stays silent
 * if the file does not already exist.
 */
#ifndef CRASHLOG_H
#define CRASHLOG_H

#include "macport.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void crashlog_open(char *name);                                    /* 0x00470000 */
extern void crashlog_close(void);                                         /* 0x00470100 */
extern void crashlog_print(const char *str);                              /* 0x00470120 */
extern void crashlog_print_pstr(StringPtr pstr);                          /* 0x00470150 */
extern void crashlog_print_mdy(DateTimeRec *dtr);                         /* 0x004701b0 */
extern void crashlog_print_num(int num);                                  /* 0x004701f0 */
extern void crashlog_mission_bit_set(char *str, char *pstr, short num, short num2, char *setString); /* 0x00470220 */
extern void crashlog(char *str);                                          /* 0x00470390 */
extern char crashlog_isactive(void);                                      /* 0x004703c0 */

#ifdef __cplusplus
}
#endif

#endif /* CRASHLOG_H */
