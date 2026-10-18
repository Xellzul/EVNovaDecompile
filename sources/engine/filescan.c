// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* filescan.c -- count the files of one extension in a directory, logging each (win 0x0046f690). Notes: Research/notes/filescan.md */
#include <stdio.h>

#include "macport.h"
#include "burger.h"
#include "burgerplat.h"
#include "crashlog.h"

/* OpenEVPlugins (mac _OpenEVPlugins). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f500
short OpenEVPlugins(const char *path, const char *ext, FILE *log)
{
    DirectorySearch_t dir;
    char buf[0x800];

    if (OpenADirectory(&dir, path) != 0)
        return -1;
    if (ext != NULL && *ext == '.')
        ext++;
    while (GetADirectoryEntryExtension(&dir, ext) == 0) {
        if (dir.Dir == 0) {
            crashlog_print("  ");
            crashlog_print(dir.Name);
            if (log != NULL) {
                fprintf(log, "  %s", dir.Name);
                fflush(log);
            }
            strcpy(buf, path);
            if (buf[strlen(buf) - 1] != ':')
                strcat(buf, ":");
            strcat(buf, dir.Name);
            if (EVOpenResourceFile(buf) == -1) {
                crashlog_print(" - failed or has no resource fork");
                if (log != NULL)
                    fprintf(log, " - failed or has no resource fork");
            }
            crashlog_print("\n");
            if (log != NULL) {
                fprintf(log, "\n");
                fflush(log);
            }
        }
    }
    CloseADirectory(&dir);
    return 0;
}

/* CountEVPlugins. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0046f690
short CountEVPlugins(const char *path, const char *ext, FILE *log)
{
    DirectorySearch_t dir;
    Word count;

    if (OpenADirectory(&dir, path) != 0)
        return 0;
    count = 0;
    if (ext != NULL && *ext == '.')
        ext++;
    while (GetADirectoryEntryExtension(&dir, ext) == 0) {
        if (dir.Dir == 0) {
            if (log != NULL) {
                fprintf(log, "  %s\n", dir.Name);
                fflush(log);
            }
            count++;
        }
    }
    CloseADirectory(&dir);
    return count;
}
