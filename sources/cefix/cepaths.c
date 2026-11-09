// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* cepaths.c -- EV Nova CE: data, plug-in, movie and music paths relative to the current
   directory, and unencrypted pilot files (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include <stddef.h>
#include "macport.h"
#include "gameconst.h"
#include "burger.h"
#include "player.h"
#include "cefix.h"

#define kExeFolder          "9:"        /* BurgerLib prefix 9: the exe's folder */
#define kNovaPrefix         "Nova "     /* "Nova Plug-Ins" is "Plug-Ins" on the Mac */
#define kNovaFilesFolder    "Nova Files"

static unsigned char ceNovaFilesFallback;   /* Nova Files was found beside the exe */
static unsigned char cePlugInsFallback;     /* the plug-ins folder is "Plug-Ins" */

/* CE saves pilots plain. The first word of a plain record is a stellar index, which is
   below kMaxStellars; anything else is still encrypted. */
long CE_DecryptIfNecessary(void *buf, unsigned long len, unsigned long key)
{
    if (*(unsigned short *)buf >= kMaxStellars)
        return DoEncryption(buf, len, key);
    return 0;
}

/* No Nova Files in the current directory: the one beside the exe, instead of creating
   an empty folder here. */
short CE_OpenNovaFilesFallback(const char *path, const char *ext, struct _FILE *log)
{
    ceNovaFilesFallback = 1;
    return OpenEVPlugins(kExeFolder kNovaFilesFolder, ext, log);
}

/* No Nova Plug-Ins in the current directory: try the Mac name before the game creates
   an empty folder. */
short CE_OpenPlugInsFallback(const char *path, const char *ext, struct _FILE *log)
{
    short err;

    err = OpenEVPlugins(path, ext, log);
    if (err != 0) {
        err = OpenEVPlugins(path + sizeof(kNovaPrefix) - 1, ext, log);
        cePlugInsFallback = err == 0;
    }
    return err;
}

/* The movie and music code asks for "9:Nova Files:<name>"; look where OpenDataFiles
   found Nova Files. */
void CE_ExpandFilesPath(char *dst, const char *path)
{
    if (!ceNovaFilesFallback)
        path += sizeof(kExeFolder) - 1;
    ExpandAPathToBufferNative(dst, path);
}

/* ... and "9:Nova Plug-ins:<name>", where OpenDataFiles found the plug-ins. */
void CE_ExpandPlugInsPath(char *dst, const char *path)
{
    if (cePlugInsFallback)
        path += sizeof(kExeFolder kNovaPrefix) - 1;
    else
        path += sizeof(kExeFolder) - 1;
    ExpandAPathToBufferNative(dst, path);
}

#endif /* CEFIX */
