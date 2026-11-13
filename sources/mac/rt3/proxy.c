/* proxy.c -- RT3, the web proxy settings the renewal goes through: Internet Config's HTTP and SOCKS proxy preferences and no-proxy domains, System Configuration's HTTPS proxy and, on Mac OS X, the login kept in the keychain; the mac 1.1.1 librt3_nonag.a(proxy.o) (mac only). Notes: Research/notes/rt3mac.md */
#include <Security/Security.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include "rt3_lib.h"

#define kKeychainDenied 0xffffe4a1
#define kNoMemory 0xffffe4a4
#define kHashFold 0x00000001
#define kHashBinary 0x02000000

extern unsigned char *C2PStringCopy(unsigned char *pstr, const char *cstr);
extern char *P2CStringCopy(char *cstr, const unsigned char *pstr);
extern struct HashTable *HashCreate(ULong flags);
extern void HashDispose(void *ref);
extern char *HashLookup(void *reply, const char *key, void *after);
extern int32_t HashAppend(struct HashTable *t, const char *key, const char *value);

static inline OSType GetProcessCreator(void)
{
    static OSType creatorCode = -1;
    ProcessInfoRec info;
    Str255 processName;
    FSSpec appSpec;
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    OSErr err;

    if (creatorCode != -1)
        goto done;
    info.processInfoLength = sizeof(info);
    info.processAppSpec = &appSpec;
    info.processName = processName;
    err = GetProcessInformation(&psn, &info);
    if (err == noErr)
        creatorCode = info.processSignature;
    else
        creatorCode = 0x3f3f3f3f;
done:
    return creatorCode;
}

// MAC: _GetHTTPSProxy i386 0x000d738b ppc 0x000e1e70
int32_t GetHTTPSProxy(char *host, char *user, char *password)
{
    int32_t err = 0;
    Boolean wantLogin;
    int enabled = 0;
    int port = 0;
    char hostName[256] = "";
    CFDictionaryRef proxies;
    CFTypeRef value;

    if (user != 0 && err == 0) /* V111 pad: err is still 0 here, folds away */
        wantLogin = 1;
    else
        wantLogin = 0;
    *host = 0;
    if (user != 0 && wantLogin)
        *user = 0;
    if (password != 0)
        *password = 0;
    proxies = SCDynamicStoreCopyProxies(0);
    if (proxies != 0) {
        value = CFDictionaryGetValue(proxies, CFSTR("HTTPSEnable"));
        if (value != 0 && CFGetTypeID(value) == CFNumberGetTypeID())
            CFNumberGetValue(value, kCFNumberIntType, &enabled);
        if (enabled != 0) {
            value = CFDictionaryGetValue(proxies, CFSTR("HTTPSProxy"));
            if (value != 0 && CFGetTypeID(value) == CFStringGetTypeID())
                CFStringGetCString(value, hostName, 256, kCFStringEncodingASCII);
            value = CFDictionaryGetValue(proxies, CFSTR("HTTPSPort"));
            if (value != 0 && CFGetTypeID(value) == CFNumberGetTypeID())
                CFNumberGetValue(value, kCFNumberIntType, &port);
            StringFormatSafe(host, 256, port != 0 ? "%s:%i" : "%s", hostName, port);
        }
        CFRelease(proxies);
    }
    if (SystemMacOSXOrLater() && *host != 0 && user != 0 && password != 0) {
        SecKeychainSearchRef search = 0;
        SecKeychainItemRef item = 0;
        SecProtocolType protocol;
        SecKeychainAttribute attrs[2];
        SecKeychainAttributeList list;
        void *passwordData = 0;
        UInt32 passwordLength = 0;

        protocol = kSecProtocolTypeHTTPSProxy;
        attrs[0].tag = kSecServerItemAttr;
        attrs[0].data = host;
        attrs[0].length = StringGetLength(host);
        attrs[1].tag = kSecProtocolItemAttr;
        attrs[1].data = &protocol;
        attrs[1].length = sizeof(protocol);
        list.count = 2;
        list.attr = attrs;
        if (SecKeychainSearchCreateFromAttributes(0, kSecInternetPasswordItemClass, &list, &search) == noErr) {
            while (SecKeychainSearchCopyNext(search, &item) == noErr) {
                attrs[0].tag = kSecAccountItemAttr;
                list.attr = attrs;
                list.count = 1;
                err = SecKeychainItemCopyContent(item, 0, &list, &passwordLength, &passwordData);
                if (err == errSecAuthFailed) {
                    err = kKeychainDenied;
                    goto done;
                }
                if (err == noErr && passwordData != 0) {
                    if (attrs[0].length > 0xff)
                        attrs[0].length = 0xff;
                    StringCopySafe(user, attrs[0].data, attrs[0].length + 1);
                    if (passwordLength > 0xff)
                        passwordLength = 0xff;
                    StringCopySafe(password, passwordData, passwordLength + 1);
                    SecKeychainItemFreeContent(&list, passwordData);
                }
                if (item != 0) {
                    CFRelease(item);
                    item = 0;
                }
            }
            if (search != 0)
                CFRelease(search);
        }
    }
done:
    return err;
}

// MAC: _GetSOCKSServer i386 0x000d79c6 ppc 0x000e2408
int32_t GetSOCKSServer(char *host, char *user, char *password)
{
    ICInstance ic = 0;
    int32_t err = 0;
    Boolean wantLogin;
    ICAttr attr;
    Str255 key;
    int32_t size;
    Str255 hostName;
    Boolean useSocks;

    if (user != 0 && err == 0) /* V111 pad: err is still 0 here, folds away */
        wantLogin = 1;
    else
        wantLogin = 0;
    *host = 0;
    if (user != 0 && wantLogin)
        *user = 0;
    if (password != 0)
        *password = 0;
    if (ICStart(&ic, GetProcessCreator()) == noErr && ICBegin(ic, icReadOnlyPerm) == noErr) {
        size = 1;
        if (ICGetPref(ic, C2PStringCopy(key, "UseSocks"), &attr, (Ptr)&useSocks, &size) == noErr && useSocks) {
            size = 256;
            if (ICGetPref(ic, C2PStringCopy(key, "SocksHost"), &attr, (Ptr)hostName, &size) == noErr && hostName[0] != 0)
                P2CStringCopy(host, hostName);
        }
    }
    if (SystemMacOSXOrLater() && *host != 0 && user != 0 && password != 0) {
        SecKeychainSearchRef search = 0;
        SecKeychainItemRef item = 0;
        SecProtocolType protocol;
        SecKeychainAttribute attrs[2];
        SecKeychainAttributeList list;
        void *passwordData = 0;
        UInt32 passwordLength = 0;

        protocol = kSecProtocolTypeSOCKS;
        attrs[0].tag = kSecServerItemAttr;
        attrs[0].data = host;
        attrs[0].length = StringGetLength(host);
        attrs[1].tag = kSecProtocolItemAttr;
        attrs[1].data = &protocol;
        attrs[1].length = sizeof(protocol);
        list.count = 2;
        list.attr = attrs;
        if (SecKeychainSearchCreateFromAttributes(0, kSecInternetPasswordItemClass, &list, &search) == noErr) {
            while (SecKeychainSearchCopyNext(search, &item) == noErr) {
                attrs[0].tag = kSecAccountItemAttr;
                list.attr = attrs;
                list.count = 1;
                err = SecKeychainItemCopyContent(item, 0, &list, &passwordLength, &passwordData);
                if (err == errSecAuthFailed) {
                    err = kKeychainDenied;
                    goto done;
                }
                if (err == noErr && passwordData != 0) {
                    if (attrs[0].length > 0xff)
                        attrs[0].length = 0xff;
                    StringCopySafe(user, attrs[0].data, attrs[0].length + 1);
                    if (passwordLength > 0xff)
                        passwordLength = 0xff;
                    StringCopySafe(password, passwordData, passwordLength + 1);
                    SecKeychainItemFreeContent(&list, passwordData);
                }
                if (item != 0) {
                    CFRelease(item);
                    item = 0;
                }
            }
            if (search != 0) {
                CFRelease(search);
                search = 0;
            }
        }
    }
done:
    if (ic != 0)
        ICStop(ic);
    return err;
}

// MAC: _GetHTTPProxy i386 0x000d7d64 ppc 0x000e277c
int32_t GetHTTPProxy(char *host, char *user, char *password)
{
    ICInstance ic = 0;
    int32_t err = 0;
    Boolean wantLogin;
    ICAttr attr;
    Str255 key;
    int32_t size;
    Str255 hostName;
    Boolean useProxy;

    if (user != 0 && err == 0) /* V111 pad: err is still 0 here, folds away */
        wantLogin = 1;
    else
        wantLogin = 0;
    *host = 0;
    if (user != 0 && wantLogin)
        *user = 0;
    if (password != 0)
        *password = 0;
    if (ICStart(&ic, GetProcessCreator()) == noErr && ICBegin(ic, icReadOnlyPerm) == noErr) {
        size = 1;
        if (ICGetPref(ic, C2PStringCopy(key, "UseHTTPProxy"), &attr, (Ptr)&useProxy, &size) == noErr && useProxy) {
            size = 256;
            if (ICGetPref(ic, C2PStringCopy(key, "HTTPProxyHost"), &attr, (Ptr)hostName, &size) == noErr && hostName[0] != 0)
                P2CStringCopy(host, hostName);
        }
    }
    if (SystemMacOSXOrLater() && *host != 0 && user != 0 && password != 0) {
        SecKeychainSearchRef search = 0;
        SecKeychainItemRef item = 0;
        SecProtocolType protocol;
        SecKeychainAttribute attrs[2];
        SecKeychainAttributeList list;
        void *passwordData = 0;
        UInt32 passwordLength = 0;

        protocol = kSecProtocolTypeHTTPProxy;
        attrs[0].tag = kSecServerItemAttr;
        attrs[0].data = host;
        attrs[0].length = StringGetLength(host);
        attrs[1].tag = kSecProtocolItemAttr;
        attrs[1].data = &protocol;
        attrs[1].length = sizeof(protocol);
        list.count = 2;
        list.attr = attrs;
        if (SecKeychainSearchCreateFromAttributes(0, kSecInternetPasswordItemClass, &list, &search) == noErr) {
            while (SecKeychainSearchCopyNext(search, &item) == noErr) {
                attrs[0].tag = kSecAccountItemAttr;
                list.attr = attrs;
                list.count = 1;
                err = SecKeychainItemCopyContent(item, 0, &list, &passwordLength, &passwordData);
                if (err == errSecAuthFailed) {
                    err = kKeychainDenied;
                    goto done;
                }
                if (err == noErr && passwordData != 0) {
                    if (attrs[0].length > 0xff)
                        attrs[0].length = 0xff;
                    StringCopySafe(user, attrs[0].data, attrs[0].length + 1);
                    if (passwordLength > 0xff)
                        passwordLength = 0xff;
                    StringCopySafe(password, passwordData, passwordLength + 1);
                    SecKeychainItemFreeContent(&list, passwordData);
                }
                if (item != 0) {
                    CFRelease(item);
                    item = 0;
                }
            }
            if (search != 0) {
                CFRelease(search);
                search = 0;
            }
        }
    }
done:
    if (ic != 0)
        ICStop(ic);
    return err;
}

// MAC: _GetNoProxyList i386 0x000d774b ppc 0x000e21c0
struct HashTable *GetNoProxyList(Boolean *flag)
{
    ICInstance ic = 0;
    Handle prefs;
    struct HashTable *list;
    int32_t err = 0;
    int32_t e;
    ICAttr attr;
    Str255 key;
    char name[256];
    int count;
    unsigned char *entry;
    int i;

    if (flag != 0)
        *flag = 0;
    prefs = 0;
    list = HashCreate(kHashBinary | kHashFold);
    err = kNoMemory;
    if (list != 0) {
        if (ICStart(&ic, GetProcessCreator()) == noErr && ICBegin(ic, icReadOnlyPerm) == noErr) {
            prefs = NewHandle(0);
            if (prefs == 0)
                goto done;
            if (ICFindPrefHandle(ic, C2PStringCopy(key, "NoProxyDomains"), &attr, prefs) == noErr) {
                HLockHi(prefs);
                count = *(short *)*prefs;   /* BUG: a big endian count read in native order on the Intel build */
                entry = (unsigned char *)*prefs + 2;
                i = 0;
                while (i++ != count) {
                    if (*entry != 0) {
                        P2CStringCopy(name, entry);
                        if (HashLookup(list, name, 0) == 0) {
                            e = HashAppend(list, name, "");
                            if (e != 0) {
                                err = e;
                                goto done;
                            }
                        }
                    }
                    entry += *entry + 1;
                }
            }
        }
        err = 0;
    }
done:
    if (flag != 0)
        *flag = 0;
    if (prefs != 0)
        DisposeHandle(prefs);
    if (ic != 0)
        ICStop(ic);
    if (err == kNoMemory && list != 0) {
        HashDispose(list);
        list = 0;
    }
    return list;
}
