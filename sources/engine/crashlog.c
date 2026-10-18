// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* crashlog.c -- debug logging. Notes: Research/notes/crashlog.md */
#include <stdio.h>

#include "burger.h"
#include "macport.h"
#include "crashlog.h"

// GLOBAL: EVNOVA 0x007cdab0
static FILE *logfile;

/* crashlog_open (mac _crashlog_open). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470000
void crashlog_open(char *name)
{
    DateTimeRec dtr;

    logfile = fopen(name, "r");
    if (logfile == NULL)
        return;
    fclose(logfile);
    logfile = fopen(name, "w");
    if (logfile) {
        fprintf(logfile, "EV Nova debug log\n\n");
        GetTime(&dtr);
        fprintf(logfile, "Output on %i/%i/%i at ", dtr.month, dtr.day, dtr.year);
        if (dtr.hour < 12)
            fprintf(logfile, "%i:%02i AM", dtr.hour, dtr.minute);
        else if (dtr.hour == 12)
            fprintf(logfile, "%i:%02i PM", dtr.hour, dtr.minute);
        else
            fprintf(logfile, "%i:%02i PM", dtr.hour - 12, dtr.minute);
        fprintf(logfile, "\n\n");
    }
}

/* crashlog_close (mac _crashlog_close). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470100
void crashlog_close(void)
{
    if (logfile) {
        fclose(logfile);
        logfile = NULL;
    }
}

/* crashlog_print (mac _crashlog_print). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470120
void crashlog_print(const char *str)
{
    if (logfile) {
        fprintf(logfile, "%s", str);
        fflush(logfile);
    }
}

/* crashlog_print_pstr (mac _crashlog_print_pstr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470150
void crashlog_print_pstr(StringPtr pstr)
{
    char buf[256];

    if (logfile) {
        strncpy(buf, (char *)pstr, 255);
        PtoCstr((unsigned char *)buf);
        fprintf(logfile, "%s", buf);
        fflush(logfile);
    }
}

/* crashlog_print_mdy (mac _crashlog_print_mdy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004701b0
void crashlog_print_mdy(DateTimeRec *dtr)
{
    if (logfile) {
        fprintf(logfile, "%02i/%02i/%i", dtr->month, dtr->day, dtr->year);
        fflush(logfile);
    }
}

/* crashlog_print_num (mac _crashlog_print_num). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004701f0
void crashlog_print_num(int num)
{
    if (logfile) {
        fprintf(logfile, "%i", num);
        fflush(logfile);
    }
}

/* crashlog_mission_bit_set (mac _crashlog_mission_bit_set). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470220
void crashlog_mission_bit_set(char *str, char *pstr, short num, short num2, char *setString)
{
    char buf[256];

    if (logfile && setString) {
        if (strlen(setString))
            fprintf(logfile, "evaluating ncb set string '%s'", setString);
        if (str) {
            if (strlen(str)) {
                if (*setString)
                    fprintf(logfile, " (%s", str);
                else
                    fprintf(logfile, "%s", str);
                if (pstr == NULL) {
                    if (num)
                        fprintf(logfile, " %i", num);
                }
                else {
                    if (strlen(pstr)) {
                        strcpy(buf, pstr);
                        if (num)
                            PtoCstr((unsigned char *)buf);
                        fprintf(logfile, " %s", buf);
                        if (num2 > 0)
                            fprintf(logfile, " (%i)", num2);
                    }
                }
                if (*setString)
                    fprintf(logfile, ")");
            }
        }
        fprintf(logfile, "\n");
        fflush(logfile);
    }
}

/* crashlog (mac _crashlog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470390
void crashlog(char *str)
{
    if (logfile) {
        fprintf(logfile, "%s\n", str);
        fflush(logfile);
    }
}

/* crashlog_isactive (mac _crashlog_isactive). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004703c0
char crashlog_isactive(void)
{
    return logfile ? 1 : 0;
}
