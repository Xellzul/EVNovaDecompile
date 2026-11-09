/* ASWCarbonSparkleBridge.h -- Ambrosia's Sparkle update bridge, loaded from a private framework of the app bundle: a static function, out of line in EVEventLoop.o (two callers). The debug map names this header for the function, so it lives here. Notes: Research/notes/eveventloop.md */
#ifndef ASWCARBONSPARKLEBRIDGE_H
#define ASWCARBONSPARKLEBRIDGE_H

/* ASWSparkleLoadPrivateFrameworkBundle (mac _ASWSparkleLoadPrivateFrameworkBundle, mac-only). STATE: see the mac gate. */
// MAC: _ASWSparkleLoadPrivateFrameworkBundle i386 0x000cc321 ppc 0x000d7e68
static void ASWSparkleLoadPrivateFrameworkBundle(CFStringRef name, CFBundleRef mainBundle, CFBundleRef *outBundle)
{
    CFURLRef frameworksURL;
    CFURLRef bundleURL;

    if (outBundle != NULL) {
        *outBundle = NULL;
        if (mainBundle == NULL)
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
