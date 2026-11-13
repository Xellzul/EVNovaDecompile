/* eveventloop.c -- the mac application entry: main, the Apple Event and Carbon event handlers, the Ambrosia bridge bundle loaders, and MoreAppleEvents' four helpers; the mac 1.1.1 EVEventLoop.o (mac only). Notes: Research/notes/eveventloop.md */
#if V111
#include <assert.h>
#include <stddef.h>

#include "macport.h"
#include "sound.h"
#include "burger.h"
#include "gameutil.h"
#include "render.h"

static void (*_ASWSparkleApplicationDidFinishLaunching)(void);
static void (*_ASWSparkleScheduleCheck)(void);
static void (*_ASWSparkleCheckForUpdates)(void);
static void (*_ASWSparkleInit)(void);
static CFBundleRef _aswSparkle;
#include "../include/carbonstatics.h"
#include "game.h"

/* <assert.h>'s assert as EVEventLoop.c expands it: the build machine's __FILE__, the source's line */
#define MoreAssertQ(e, line) \
    ((__builtin_expect(!(e), 0) ? __assert(#e, kEngineSourceDir "EVEventLoop.c", line) : 0))

/* this TU's own declarations: EVYesNoDialog, meetsMinimumRequirements and checkForUpdates are
   called without a prototype (int), RT3_Open and EVPlatformInit are read as OSErr */
extern Boolean gQuit;
extern Boolean screenFaded;
extern void DoPrefsDialog(void);
extern void RunPrefsDialog(void);
extern void InitCocoa(void);
extern int checkForUpdates(void);
extern int EVPlatformInit(void);

#define kCmdAbout 'abou'
#define kCmdPrefs 'pref'
#define kCmdPreambles 'OMG '
#define kCmdRegister 'ASMW'
#define kCmdHide 'hide'
#define kCmdHideOthers 'hido'

#include "../include/ASWCarbonSparkleBridge.h"
#include "../include/ASWAboutBoxCarbonAPI.h"
#include "../include/ASWRegistrationCarbonAPI.h"

/* MoreAENullDesc (mac _MoreAENullDesc, Apple's MoreAppleEvents). STATE: see the mac gate. */
// MAC: _MoreAENullDesc i386 0x000cc5b5 ppc 0x000d81a8
void MoreAENullDesc(AEDesc *desc)
{
    MoreAssertQ(desc != NULL, 40);

    desc->descriptorType = typeNull;
    desc->dataHandle = NULL;
}

/* MoreAEDisposeDesc (mac _MoreAEDisposeDesc, Apple's MoreAppleEvents). STATE: see the mac gate. */
// MAC: _MoreAEDisposeDesc i386 0x000ccd2a ppc 0x000d8a1c
void MoreAEDisposeDesc(AEDesc *desc)
{
    OSStatus junk;

    MoreAssertQ(desc != NULL, 50);

    junk = AEDisposeDesc(desc);
    MoreAssertQ(junk == noErr, 53);

    MoreAENullDesc(desc);
}

/* MoreAECreateAppleEventProcessTarget (mac _MoreAECreateAppleEventProcessTarget, Apple's MoreAppleEvents). STATE: see the mac gate. */
// MAC: _MoreAECreateAppleEventProcessTarget i386 0x000ccd8e ppc 0x000d8aa4
OSStatus MoreAECreateAppleEventProcessTarget(const ProcessSerialNumberPtr psnPtr,
                                             AEEventClass pAEEventClass,
                                             AEEventID pAEEventID,
                                             AppleEvent *pAppleEvent)
{
    OSStatus anErr = noErr;
    AEDesc targetAppDesc = {typeNull, NULL};

    anErr = AECreateDesc(typeProcessSerialNumber, psnPtr, sizeof(ProcessSerialNumber), &targetAppDesc);

    if (noErr == anErr) {
        anErr = AECreateAppleEvent(pAEEventClass, pAEEventID, &targetAppDesc,
                                   kAutoGenerateReturnID, kAnyTransactionID, pAppleEvent);
    }

    MoreAEDisposeDesc(&targetAppDesc);

    return anErr;
}

/* MoreAECreateAppleEventSelfTarget (mac _MoreAECreateAppleEventSelfTarget, Apple's MoreAppleEvents). STATE: see the mac gate. */
// MAC: _MoreAECreateAppleEventSelfTarget i386 0x000cce0f ppc 0x000d8b3c
OSStatus MoreAECreateAppleEventSelfTarget(AEEventClass pAEEventClass,
                                          AEEventID pAEEventID,
                                          AppleEvent *pAppleEvent)
{
    OSStatus anErr = noErr;

    ProcessSerialNumber selfPSN = {0, kCurrentProcess};

    anErr = MoreAECreateAppleEventProcessTarget(&selfPSN, pAEEventClass, pAEEventID, pAppleEvent);

    return anErr;
}

/* RegisterHelpBook (mac _RegisterHelpBook, mac-only). STATE: see the mac gate. */
// MAC: _RegisterHelpBook i386 0x000cc5f5 ppc 0x000d8200
void RegisterHelpBook(void)
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef bundleURL;
    FSRef ref;

    if (mainBundle != NULL) {
        bundleURL = CFBundleCopyBundleURL(mainBundle);
        if (bundleURL != NULL) {
            if (CFURLGetFSRef(bundleURL, &ref))
                AHRegisterHelpBook(&ref);
            CFRelease(bundleURL);
        }
    }
}

static pascal OSErr openApplicationAEHandler(const AppleEvent *event, AppleEvent *reply, int32_t refcon);
static pascal OSErr quitApplicationAEHandler(const AppleEvent *event, AppleEvent *reply, int32_t refcon);
static pascal OSStatus AppEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData);

/* openApplicationAEHandler (mac _openApplicationAEHandler, mac-only). STATE: see the mac gate. */
// MAC: _openApplicationAEHandler i386 0x000cc643 ppc 0x000d825c
static pascal OSErr openApplicationAEHandler(const AppleEvent *event, AppleEvent *reply, int32_t refcon)
{
    DescType actualType;
    Size actualSize;
    OSErr err;

    err = AEGetAttributePtr(event, keyMissedKeywordAttr, typeWildCard, &actualType, NULL, 0, &actualSize);
    if (err == noErr)
        err = errAEParamMissed;
    else if (err == errAEDescNotFound) {
        EventTypeSpec specs[3] = {
            {kEventClassCommand, kEventCommandProcess},
            {kEventClassApplication, kEventAppActivated},
            {kEventClassApplication, kEventAppDeactivated}
        };
        OSErr result;
        MenuRef helpMenu;
        MenuItemIndex firstItem;
        CFBundleRef bundle;
        void (*fn)(void);

        gQuitAppAEHandler = NewAEEventHandlerUPP(quitApplicationAEHandler);
        result = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, gQuitAppAEHandler, 0, false);
        if (result == noErr) {
            gAppEventHandler = NewEventHandlerUPP(AppEventHandler);
            InstallEventHandler(GetApplicationEventTarget(), gAppEventHandler, 3, specs, NULL, NULL);
            result = EVPlatformInit();
        }
        if (result == noErr) {
            result = RT3_Open(1, 3, "EV Nova", "EV Nova");
            if (result != noErr)
                FatalStartupError(26);
        }
        if (result != noErr) {
            DisposeAEEventHandlerUPP(gOpenAppAEHandler);
            DisposeAEEventHandlerUPP(gQuitAppAEHandler);
            ExitToShell();
            return result;
        }

        bundle = NULL;
        ASWRegistrationLoadPrivateFrameworkBundle(CFSTR("ASWRegistrationCarbonBridge.bundle"), &bundle);
        if (bundle != NULL) {
            fn = CFBundleGetFunctionPointerForName(bundle, CFSTR("InitializeRegistration"));
            if (fn != NULL)
                fn();
        }
        if (!RT3_IsRegistered())
            ASWReg_ShowDialogWithOptions(true, true);
        regNoticeShown = 1;

        if (_aswSparkle == NULL)
            ASWSparkleLoadPrivateFrameworkBundle(CFSTR("ASWCarbonSparkleBridge.bundle"), NULL, &_aswSparkle);
        if (_aswSparkle != NULL) {
            _ASWSparkleInit = CFBundleGetFunctionPointerForName(_aswSparkle, CFSTR("_frameworkInit"));
            _ASWSparkleCheckForUpdates = CFBundleGetFunctionPointerForName(_aswSparkle, CFSTR("checkForUpdatesAndNotify"));
            _ASWSparkleScheduleCheck = CFBundleGetFunctionPointerForName(_aswSparkle, CFSTR("scheduleCheckWithInterval"));
            _ASWSparkleApplicationDidFinishLaunching = CFBundleGetFunctionPointerForName(_aswSparkle, CFSTR("applicationDidFinishLaunching"));
            if (_ASWSparkleInit != NULL)
                _ASWSparkleInit();
        }
        if (NetworkStackGetActive(0) && checkForUpdates()) {
            if (_aswSparkle == NULL)
                ASWSparkleLoadPrivateFrameworkBundle(CFSTR("ASWCarbonSparkleBridge.bundle"), NULL, &_aswSparkle);
            if (_aswSparkle != NULL) {
                if (_ASWSparkleApplicationDidFinishLaunching == NULL)
                    _ASWSparkleApplicationDidFinishLaunching = CFBundleGetFunctionPointerForName(_aswSparkle, CFSTR("applicationDidFinishLaunching"));
                if (_ASWSparkleApplicationDidFinishLaunching != NULL)
                    _ASWSparkleApplicationDidFinishLaunching();
            }
        }

        RegisterHelpBook();
        helpMenu = NULL;
        firstItem = -1;
        result = HMGetHelpMenu(&helpMenu, &firstItem);
        if (result == noErr)
            InsertMenuItemTextWithCFString(helpMenu, CFSTR("Read the Preambles"), firstItem, 0, kCmdPreambles);

        InitEVNova();
        err = noErr;
    }
    return err;
}

/* HandleQuitEvent (mac _HandleQuitEvent, mac-only). STATE: see the mac gate. */
// MAC: _HandleQuitEvent i386 0x000cc958 ppc 0x000d85ac
OSErr HandleQuitEvent(void)
{
    OSErr result = noErr;
    Str255 prompt;

    if (gameInProgress && gameLoaded && !gPause) {
        result = eventNotHandledErr;
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
        ShowTheCursor();
        HideCursorSprite();
        RefreshSAT();
        if (!screenFaded) {
            GetIndString(prompt, 0x8c, 10);
            result = noErr;
            if (!EVYesNoDialog(prompt)) {
                result = eventNotHandledErr;
                RedrawStatus(1);
            }
        }
        RefreshSAT();
    }
    return result;
}

/* quitApplicationAEHandler (mac _quitApplicationAEHandler, mac-only). STATE: see the mac gate. */
// MAC: _quitApplicationAEHandler i386 0x000cca24 ppc 0x000d8698
static pascal OSErr quitApplicationAEHandler(const AppleEvent *event, AppleEvent *reply, int32_t refcon)
{
    DescType actualType;
    Size actualSize;
    OSErr err;

    err = AEGetAttributePtr(event, keyMissedKeywordAttr, typeWildCard, &actualType, NULL, 0, &actualSize);
    if (err == noErr)
        err = errAEParamMissed;
    else if (err == errAEDescNotFound) {
        if (!gQuit) {
            err = HandleQuitEvent();
            if (err != noErr)
                goto done;
        }
        if (gQuit || err == noErr) {
            gQuit = 1;
            RT3_Close();
            ExitToShell();
        }
    done:
        err = noErr;
    }
    return err;
}

/* main (mac _main, mac-only). STATE: see the mac gate. */
// MAC: _main i386 0x000ccab1 ppc 0x000d8738
int main(void)
{
    OSStatus err;
    OSErr aeErr;

#if V111
    useLegacyRenderingPipeline = 0;
    if (!meetsMinimumRequirements())
        useLegacyRenderingPipeline = 1;
    InitCocoa();
    err = CreateNibReference(CFSTR("main"), &sNibRef);
    if (err == noErr)
        err = SetMenuBarFromNib(sNibRef, CFSTR("MenuBar"));
#else
    err = noErr;
#endif
    if (err == noErr) {
        gOpenAppAEHandler = NewAEEventHandlerUPP(openApplicationAEHandler);
        aeErr = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, gOpenAppAEHandler, 0, false);
        if (aeErr != noErr)
            err = aeErr;
    }
    if (err != noErr)
        ExitToShell();
    RunApplicationEventLoop();
    DisposeAEEventHandlerUPP(gOpenAppAEHandler);
    DisposeAEEventHandlerUPP(gQuitAppAEHandler);
    DisposeEventHandlerUPP(gAppEventHandler);
    return 0;
}

/* CERunPrefsDialog (mac _CERunPrefsDialog, mac-only). STATE: see the mac gate. */
// MAC: _CERunPrefsDialog i386 0x000ccb7a ppc 0x000d8830
void CERunPrefsDialog(void)
{
    RunPrefsDialog();
}

/* AppEventHandler (mac _AppEventHandler, mac-only). STATE: see the mac gate. */
// MAC: _AppEventHandler i386 0x000ccb83 ppc 0x000d8834
static pascal OSStatus AppEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    HICommand command;
    CFBundleRef bundle;
    CFURLRef url;
    void (*fn)(void);

    switch (GetEventClass(event)) {
    case kEventClassApplication:
        switch (GetEventKind(event)) {
        case kEventAppActivated:
            MainScreenActivate(1);
            break;
        case kEventAppDeactivated:
            MainScreenActivate(0);
            break;
        }
        break;
    case kEventClassCommand:
        GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
        if (GetEventKind(event) == kEventCommandProcess) {
            switch (command.commandID) {
            case kCmdPreambles:
                url = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("EV Nova Preambles.pdf"), NULL, NULL);
                if (url != NULL) {
                    LSOpenCFURLRef(url, NULL);
                    CFRelease(url);
                }
                break;
            case kCmdAbout:
                bundle = NULL;
                ASWAboutBoxLoadPrivateFrameworkBundle(CFSTR("ASWAppKit.framework"), &bundle);
                ASWAboutBoxLoadPrivateFrameworkBundle(CFSTR("ASWAboutBoxCarbonBridge.bundle"), &bundle);
                if (bundle != NULL) {
                    fn = CFBundleGetFunctionPointerForName(bundle, CFSTR("show"));
                    if (fn != NULL)
                        fn();
                }
                return noErr;
            case kCmdPrefs:
                DoPrefsDialog();
                return noErr;
            case kCmdHideOthers:
            case kCmdHide:
                if (CGDisplayIsCaptured(CGMainDisplayID()))
                    return noErr;
                break;
            case kCmdRegister:
                ASWReg_ShowDialogWithOptions(true, true);
                break;
            }
        }
        break;
    }
    return eventNotHandledErr;
}
#endif
