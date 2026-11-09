/* ASWRegistrationCarbonAPI.h -- Ambrosia's registration window, run from its Carbon bridge bundle
   (a private framework of the app bundle): static functions, out of line in EVEventLoop.o (two
   callers), inlined in Main Screen.o and Escape Velocity.o (one). Notes: Research/notes/mainscreen.md */
#ifndef ASWREGISTRATIONCARBONAPI_H
#define ASWREGISTRATIONCARBONAPI_H

#if V111
/* ASWRegistrationLoadPrivateFrameworkBundle (mac _ASWRegistrationLoadPrivateFrameworkBundle, mac-only). STATE: see the mac gate. */
// MAC: _ASWRegistrationLoadPrivateFrameworkBundle i386 0x000cc481 ppc 0x000d8004
static void ASWRegistrationLoadPrivateFrameworkBundle(CFStringRef name, CFBundleRef *outBundle)
{
    CFBundleRef mainBundle;
    CFURLRef frameworksURL = NULL;
    CFURLRef bundleURL = NULL;

    if (outBundle != NULL) {
        *outBundle = NULL;
        mainBundle = CFBundleGetMainBundle();
        if (mainBundle != NULL) {
            frameworksURL = CFBundleCopyPrivateFrameworksURL(mainBundle);
            if (frameworksURL != NULL) {
                bundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorSystemDefault, frameworksURL, name, false);
                if (bundleURL != NULL) {
                    *outBundle = CFBundleCreate(kCFAllocatorSystemDefault, bundleURL);
                    if (*outBundle != NULL && !CFBundleLoadExecutable(*outBundle)) {
                        CFRelease(*outBundle);
                        *outBundle = NULL;
                    }
                }
            }
        }
        if (bundleURL != NULL)
            CFRelease(bundleURL);
        if (frameworksURL != NULL)
            CFRelease(frameworksURL);
    }
}

/* ASWReg_ShowDialogWithOptions (mac _ASWReg_ShowDialogWithOptions, mac-only). STATE: see the mac gate. */
// MAC: _ASWReg_ShowDialogWithOptions i386 0x000cc52b ppc 0x000d80cc
static long ASWReg_ShowDialogWithOptions(Boolean modal, Boolean countdown)
{
    CFBundleRef bundle = NULL;
    long result = fnfErr;
    Boolean (*runModal)(void);
    void (*run)(void);

    ASWRegistrationLoadPrivateFrameworkBundle(CFSTR("ASWRegistrationCarbonBridge.bundle"), &bundle);
    if (bundle != NULL) {
        if (modal) {
            if (countdown)
                runModal = CFBundleGetFunctionPointerForName(bundle, CFSTR("runModal"));
            else
                runModal = CFBundleGetFunctionPointerForName(bundle, CFSTR("runModalWithoutCountDown"));
            result = 0;
            if (runModal != NULL)
                result = runModal();
        } else {
            if (countdown)
                run = CFBundleGetFunctionPointerForName(bundle, CFSTR("run"));
            else
                run = CFBundleGetFunctionPointerForName(bundle, CFSTR("runWithoutCountDown"));
            result = 0;
            if (run != NULL)
                run();
        }
    }
    return result;
}
#endif

#endif
