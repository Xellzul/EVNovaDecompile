/* ASWAboutBoxCarbonAPI.h -- Ambrosia's about box, run from its Carbon bridge bundle (a private framework of the app bundle): a static function, out of line in EVEventLoop.o (two callers). The debug map names this header for the function, so it lives here. Notes: Research/notes/eveventloop.md */
#ifndef ASWABOUTBOXCARBONAPI_H
#define ASWABOUTBOXCARBONAPI_H

/* ASWAboutBoxLoadPrivateFrameworkBundle (mac _ASWAboutBoxLoadPrivateFrameworkBundle, mac-only). STATE: see the mac gate. */
// MAC: _ASWAboutBoxLoadPrivateFrameworkBundle i386 0x000cc3d7 ppc 0x000d7f3c
static void ASWAboutBoxLoadPrivateFrameworkBundle(CFStringRef name, CFBundleRef *outBundle)
{
    CFBundleRef mainBundle;
    CFURLRef frameworksURL;
    CFURLRef bundleURL;

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
                    CFRelease(bundleURL);
                }
                CFRelease(frameworksURL);
            }
        }
    }
}

#endif
