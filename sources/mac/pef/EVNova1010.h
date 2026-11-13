/* EVNova1010.h -- what the prefixes of the two mac 1.0.10 fragments share: the third source state, MACBUILD + MAC1010 without V111, under MWCPPC. Notes: Research/mac-1.0.10-pin.md section 8 */
#define MACBUILD 1
#define MAC1010 1

/* the win C++ TUs' linkage pragma: this build is C throughout, gcc ignores the pragma and MWCPPC obeys it */
#define cplusplus win_only

#include <ConditionalMacros.h>
#include <MacTypes.h>
#include "../../include/int32.h"
#define NDEBUG 1
#include <assert.h>
#include <Quickdraw.h>
#include <QDOffscreen.h>
#include <Events.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>
#include <Fonts.h>
#include <Resources.h>
#include <Files.h>
#include <Folders.h>
#include <MacMemory.h>
#include <OSUtils.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Sound.h>
#include <Gestalt.h>
#include <Movies.h>
#include <Lists.h>
#include <Palettes.h>
#include <Processes.h>
#include <Timer.h>
#include <Appearance.h>
#include <Speech.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <Navigation.h>
#include <AppleEvents.h>
#include <CFString.h>
#include <CFURL.h>

/* names later than these Interfaces: three in game text on Carbon, the rest 1.1.1's Darwin text standing in for RT3 and MoreFiles */
#define kAEShowPreferences 'pref'
#define kHICommandPreferences 'pref'
#define kPlainWindowClass 13L
#define kUserDomain -32763
#define kLocalDomain -32765
#define kFSVolFlagHardwareLockedMask 0x0080
#define kFSVolFlagSoftwareLockedMask 0x8000
#define kFNDirectoryModifiedMessage 1
#define kFSIterateDelete 2
extern CFURLRef CFURLCreateFromFSRef(CFAllocatorRef allocator, const FSRef *fsRef);
extern CFURLRef CFURLCreateCopyAppendingPathComponent(CFAllocatorRef allocator, CFURLRef url, CFStringRef pathComponent, Boolean isDirectory);
extern CFStringRef CFURLCopyFileSystemPath(CFURLRef anURL, CFURLPathStyle pathStyle);
extern pascal OSErr FSFindFolder(short vRefNum, OSType folderType, Boolean createFolder, FSRef *foundRef);

/* the win port's calling-convention keywords */
#define __stdcall
#define __cdecl
#define __declspec(x)

/* win placeholder name of MSL's out-of-line sqrtf, as in EVNovaPrefix.h; MSL's <math.h> makes sqrtf a macro */
#define FUN_004eed20 sqrtf
#undef sqrtf

/* the win port reads the tick count through BurgerLib; the mac engine calls the Toolbox */
#define ReadTick TickCount

/* gcc's spellings in the shared mac text */
#define __builtin_abs __abs
#define __builtin_expect(a, b) (a)

/* the assert sites name the bare file, as mwcc's __FILE__ does */
#define kEngineSourceDir ""

/* 1.1.1 chooses its rendering pipeline at run time; 1.0.10 has the one */
#define useLegacyRenderingPipeline 1

/* the later SDK's name for no flag bits */
#define kNativeEndianPixMap 0

#include <classiccarbon.h>

/* the Picture record is open to both fragments; the accessor is the later SDK's */
#define QDGetPictureBounds(pic, r) (*(r) = (*(PicHandle)(pic))->picFrame)
