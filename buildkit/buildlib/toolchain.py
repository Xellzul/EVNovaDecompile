"""What `build.py check` verifies: every tool, library and original the targets read, by size, CRC32 and
SHA-256. These are the copies the identical builds were made with (2026-11-09). `exact` rows must match. `built` rows are compilers built from source on the build host
(Apple gcc, cctools, ld64): another build of the same source may differ in its own bytes and still
produce identical output, so a difference there is reported, not fatal."""
from .host import WINE, BuildError, crc32, run, say, sha256

CLI = "Other Metrowerks Tools/Command Line Tools/"
SL = "MacOS Support/Universal/Libraries/StubLibraries/"
OT = "MacOS Support/OpenTransport/Open Tpt Client Developer/PPC Debug Libraries/"
MSLPPC = "MSL/MSL_C/MSL_MacOS/Lib/PPC/"

# (targets, build.ini section, key, path inside it, size, CRC32, SHA-256, kind)
FILES = [
    # CodeWarrior for Windows 9.2 with its 9.3 update: mwcc/mwld/mwwinrc 3.2.4 build 418
    ("win ce", "win", "codewarrior", CLI + "mwcc.exe", 2445312, "a797f81c", "0f9a7167a5d4a8175865d25efcdee13f903d74a85af7365136b55b8fbe52363d", "exact"),
    ("win ce", "win", "codewarrior", CLI + "mwld.exe", 921600, "39a1d419", "2f53d5be2b0f3081d43d5514893ed3182d20b25dccc6dfb61c82749b6fb2b460", "exact"),
    ("win ce", "win", "codewarrior", CLI + "mwwinrc.exe", 368640, "61aad37c", "42e5dc26c2fe8a8bdcaae80abbdc1aa7d0df1d96ea405afe1a70741a1b61ac59", "exact"),
    ("win ce", "win", "codewarrior", "Bin/lmgr8c.dll", 851968, "f997f290", "996041141858fd4724ec36add0913755a0067498b93271d7d26e4a1bfa04a561", "exact"),
    ("win ce", "win", "codewarrior", "Win32-x86 Support/Libraries/Runtime/Libs/MSL_All_x86.lib", 969696, "0635a099", "9ccc2025c52358ac7bbbe1ef9fd9a617817a41ff8d56c94f538275fe345bd707", "exact"),
    ("win ce", "win", "codewarrior", "MSL/MSL_C/MSL_X86/math_x87.c", 9425, "70fa6fe2", "35a179bb3f664eacc25aacc235d6c50b7317f2ec6d914cdf7e88cd5f22b85b09", "exact"),
    ("win ce", "win", "codewarrior", "MSL/MSL_C/MSL_Common/Src/math.c", 26674, "57c1c084", "d0440d426bf47910cd659fd81482f1d140eac1fdb9e3b6703737a85137428e9a", "exact"),
    # CodeWarrior for Symbian Personal v2.0: mwccsym2 2.4.7 build 335 (zlib, libpng, REGGIE)
    ("win ce", "win", "symbian", "Other_Metrowerks_Tools/Command Line Tools/mwccsym2.exe", 1474560, "49a720e5", "5215c7df4135cdcdb858dad131a67258aa95a8b50a02d266ee476769c89fa345", "exact"),
    # FlexLM 3.26: mwccsym2 loads Symbian 2.0's, MWLinkPPC Pro 6's (the same file)
    ("win ce", "win", "flexlm326", "lmgr326b.dll", 306688, "bcff74e0", "bd9415ab11f4aef42c372e8547a8303b02464b59549745be597cf09e460737e8", "exact"),
    ("mac1010", "mac1010", "flexlm326", "lmgr326b.dll", 306688, "bcff74e0", "bd9415ab11f4aef42c372e8547a8303b02464b59549745be597cf09e460737e8", "exact"),
    # Apple's QuickTime 6 SDK for Windows: the exe's QTML client library
    ("win ce", "win", "quicktime", "qtmlClient.lib", 774658, "bf1f090c", "9662e541e5e1b30b17888427940602dbe237d24c2199f5ca579a7bb294581faa", "exact"),
    # the Mac PPC compiler plugins (run under Unicorn) and the Pro 6 linker
    ("mac1010", "mac1010", "plugin_b0160", "MW C++ PPC", 1707321, "a7a302c2", "46451de5a31d3e7d8ae0908ea923bf39f69601aa6fd34c2f27685a905d47ff70", "exact"),
    ("mac1010", "mac1010", "plugin_b0160", "MW C++ PPC.rsrc", 18122, "b850cb1d", "cf9205e44f2d3e05cd2727e118d5ca4dfac8ef90249c0346ecdda440d88ad0d8", "exact"),
    ("mac1010", "mac1010", "plugin_b222", "MW C++ PPC", 1673630, "7aec72c3", "94022ff3bf421ce57761ac455202bfde37959698ee6ef364b5eb3f7b13ecfb5d", "exact"),
    ("mac1010", "mac1010", "plugin_b222", "MW C++ PPC.rsrc", 19876, "00dca8e6", "4b6210e8cd496d0bdcd96157d35c0361038f1044f7fe9911009bdd4e5849a87e", "exact"),
    ("mac1010", "mac1010", "mwlinkppc", "", 580608, "0e4c09e4", "1fe73016f9d8f4ce8ea7ff07d09fd900422c08b818c19e281212b131ca52118e", "exact"),
    # the Mac libraries the link reads: Pro 6.3's MSL and runtime, Pro 6's OT glue and stubs, the IC 2.0.2 and CarbonLib 1.4 SDK stubs
    ("mac1010", "mac1010", "pro63", MSLPPC + "MSL C.PPC.Lib", 228192, "d15cd517", "5776ada9d094a072e95781ba8b87033ec5a0f99b587aab5cbe3f39ce783e8871", "exact"),
    ("mac1010", "mac1010", "pro63", MSLPPC + "MSL C.Carbon.Lib", 229208, "6c85baa3", "3fd5f70e25107898a68c0ab03a5964fc63ff2dfcee911504124aafbc1ca3e03c", "exact"),
    ("mac1010", "mac1010", "pro63", MSLPPC + "MSL SIOUX.PPC.Lib", 49150, "6059348e", "325ac43d291976e12aded187f427632c0e6c1870b44a4a34082f5e46f325bdd3", "exact"),
    ("mac1010", "mac1010", "pro63", MSLPPC + "MSL SIOUX.Carbon.Lib", 51864, "8a2b5320", "6e5b38eb51f160505d2ab4210479b84e784848df18c0c717f6f2a1b90d2ede33", "exact"),
    ("mac1010", "mac1010", "pro63", "MacOS Support/Libraries/Runtime/Runtime PPC/Libs/MSL RuntimePPC.Lib", 72762, "c8a400df", "07897b9271e96a164bff0bad80d15b4a0a7b54bd51c1bdb82f461aed448143e6", "exact"),
    ("mac1010", "mac1010", "pro6", OT + "OpenTransportAppPPC.debug.o", 85479, "828b7d73", "d2a6ebd062bee28b9e250725086e8e1735bd7c48ae4e971a131df244fe910ab0", "exact"),
    ("mac1010", "mac1010", "pro6", OT + "OpenTptATalkPPC.debug.o", 4950, "8b59f70f", "30c39c04520efb50f436559123500f4231f74ef544eb1f70363de14ce6a0296f", "exact"),
    ("mac1010", "mac1010", "pro6", OT + "OpenTptInetPPC.debug.o", 4968, "d89f2a3d", "136308a54416ed15cd39fe17bf3f5bc13651607eef73b20884283aa3082630f2", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "InterfaceLib", 144624, "124b9f28", "20137af2a0a38aade8ec63ed5706c2772afb26cc2467b5bc90bb5b682f9fffed", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "SoundLib", 1728, "d1933dce", "740200a1e710fbc206621f7c44b814b119978eb541e91968418dd4c99d595bcc", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "MathLib", 3360, "0867f5ec", "d12090caf95dfc602f6108009c882b9b11667f5bfad00c90f2735611ffb732f5", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "DisplayLib", 1120, "209a3c17", "ec778515a07c1f4a87f196f963b4ae1d9130fffee9c1dc3072f42053b0a563c4", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "QuickTimeLib", 100288, "43f623b5", "8aac056b3bff11b8f1a09cf896428051cfb6250d34ffb273a3e9ec7c35680fe8", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "SpeechLib", 848, "22fc2bfc", "077b403fe6f504adf0c2f8f0695ebf35fa146f0e87699188d1e94a6cf8f404c5", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "OpenTransportLib", 18368, "5a35df31", "f40645a12905c301a8f05bcea203fcdeeb0030f109a1f7ef33db62a13d16b897", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "OpenTptAppleTalkLib", 2310, "8267bfe3", "b797bd62418792e28b2f28c000ac1d67ff503844a191310534c3a1dea8db4a77", "exact"),
    ("mac1010", "mac1010", "pro6", SL + "OpenTptInternetLib", 904, "b15359f1", "97610ab1b0811235f44420ba19ade99b7183bb1d07304d2d9b58f9500abb992c", "exact"),
    ("mac1010", "mac1010", "ic_sdk", SL + "InternetConfigLib", 6336, "52389953", "fb7589f7a5a960dfd052acf8e1d4d64f22c64234d172108c885ade275167cf89", "exact"),
    ("mac1010", "mac1010", "carbonlib_sdk", SL + "CarbonLib", 250744, "9d17d5a8", "884a15c5bcc31f90fa5c67c896e20b593b6f6d09983797c378504561f8720ba7", "exact"),
    # Apple gcc_40-5494, cctools-port 877.8, ld64-97.17: built from source on the build host
    ("mac111", "mac111", "applegcc", "i686/bin/xgcc", 176688, "290ae508", "15ef369819793f056a33bd9c52353af605d240b58ab9ec505f6e4c52826ea1aa", "built"),
    ("mac111", "mac111", "applegcc", "i686/bin/cc1", 6180776, "fe6f3927", "c899af2c09b66621ece79b6c7d91d48d25bc53b8240f4d2431ed3bef01f29575", "built"),
    ("mac111", "mac111", "applegcc", "i686/bin-objc/xgcc", 176688, "3b45365b", "0680d430b40e6d7dd862d7dc785d3316a7f027ebe73f146b31b1579fbd85bbe6", "built"),
    ("mac111", "mac111", "applegcc", "i686/bin-objc/cc1obj", 6364624, "8ede5c8c", "d13738f646be90900dfdc7609fab6122d9379a83ed45c424b12d78ee8e90f442", "built"),
    ("mac111", "mac111", "applegcc", "powerpc/bin/xgcc", 172456, "960dd07f", "edb35de6bfe2d492a31e8e4590ba6b1bcfeaddc446abf4b33b589f875bacd681", "built"),
    ("mac111", "mac111", "applegcc", "powerpc/bin/cc1", 6013320, "c03f3f7c", "f71838cf29b5a9fd58be7e1e27d41fe094817996744e2a16e18aed290a8423b8", "built"),
    ("mac111", "mac111", "applegcc", "powerpc/bin-objc/xgcc", 176552, "1a5b603e", "043f5446d127cb9f279f34f21ce88a82487945d2f6cab800db73c4739d6b13d9", "built"),
    ("mac111", "mac111", "applegcc", "powerpc/bin-objc/cc1obj", 6197104, "33b334b2", "325699c9f9a591993db545b77eddaa62c55f7ca15e823ff75a0de21e5c17eb84", "built"),
    ("mac111", "mac111", "applegcc", "cctools/libexec/as/i386/as", 299944, "2e8d6606", "a6a483ed258734c3ab0522c9675c9ed9c5914eb00a44561053c1e9fdf85b612f", "built"),
    ("mac111", "mac111", "applegcc", "cctools/libexec/as/ppc/as", 276496, "bfe29b34", "d767ea4c2630c15e843786039ad334ba577e5fa7d87589c4e267438441c77efa", "built"),
    ("mac111", "mac111", "applegcc", "cctools/bin/libtool", 180920, "9120ff45", "0cdf9bb049f7b181530c604f04e7d2c560f2ee0b99c4b7cf5d6409a43d23db53", "built"),
    ("mac111", "mac111", "applegcc", "cctools/bin/lipo", 84144, "38bd824b", "ee711208b4be2cdeae78031ab9f75f6e971b6f5f7cffc9e53c3ec24beff8af8e", "built"),
    ("mac111", "mac111", "applegcc", "cctools/bin/install_name_tool", 158856, "dc1745d2", "a21a31b2fa42358f4f52350cc8adcd948225658a5b496506203d75c80ba045a7", "built"),
    ("mac111", "mac111", "applegcc", "ld64/ld", 26404672, "6afb6cf6", "f518131338b0ad7d0f97fd9ddcbf05e032530974e6efa310b62c048cf9d1d3c0", "built"),
    ("mac111", "mac111", "applegcc", "lib/darwin_qsort.so", 15688, "e9595712", "ff00c796d11e4bbce6d34a217bd007d2380654e8b253237e1220789acf5a9631", "built"),
    # the user's own game copies
    ("win ce", "game", "win1010", "EV Nova.exe", 1703968, "eac2e999", "43e8db386caa0fd32d48e8b5a45b81c894063870a0b52e95ef89a1c6471a248c", "exact"),
    ("mac1010", "game", "mac1010", "EV Nova", 3172264, "4f95e9ae", "b81b382b28bb539991f2097371fd596ef2d8ae2e312aeefc2d3f104f01c8ed10", "exact"),
    ("mac1010", "game", "mac1010", ".rsrc/EV Nova", 2119655, "0042f3bb", "7445011314a0cd8c579fc85c46c53b2973a7294a5e6ce76f74334de5064fbb12", "exact"),
    ("mac111", "game", "mac111", "", 2419936, "c0346fc8", "3f880767606bb9219c0f541b747c334d09b70723cc07f11fe1bdf962ec2a1942", "exact"),
]

# Folders and files that must exist but have no single hash (headers, licences, SDK trees)
EXIST = [
    ("win ce", "win", "license", ""),
    ("win ce", "win", "symbian3", "Symbian_Support/MSL/MSL_C/MSL_Common/include"),
    ("mac1010", "mac1010", "plugin_b0160", "license.dat"),
    ("mac1010", "mac1010", "plugin_b222", "license.dat"),
    ("mac1010", "mac1010", "license", ""),
    ("mac1010", "mac1010", "pro63", "MacOS Support/Universal/Interfaces/CIncludes"),
    ("mac111", "mac111", "sdk", "usr/include"),
    ("mac111", "mac111", "xcode", "gcc4.0/usr/lib/gcc"),
    ("mac111", "mac111", "frameworks", "AmbrosiaTools.framework"),
    ("mac111", "mac111", "frameworks", "ASWDebug.framework"),
    ("mac111", "mac111", "frameworks", "ShortcutRecorder.framework"),
]


def check(cfg, targets):
    """Print one line per file; return the number of failures."""
    bad = 0
    for tg, sec, key, rel, size, crc, sha, kind in FILES:
        if not set(tg.split()) & set(targets):
            continue
        try:
            p = cfg.path(sec, key) / rel if rel else cfg.path(sec, key)
        except BuildError as e:
            say("  FAIL     %s" % e)
            bad += 1
            continue
        if not p.is_file():
            say("  MISSING  [%s] %s: %s" % (sec, key, p))
            bad += 1
            continue
        got = (p.stat().st_size, crc32(p), sha256(p))
        if got == (size, crc, sha):
            say("  ok       %s" % p)
        elif kind == "built":
            say("  NOTE     %s: %d B CRC32 %s, expected %d B %s (built on another host; the output decides)" % (p, got[0], got[1], size, crc))
        else:
            say("  WRONG    %s: %d B CRC32 %s SHA-256 %s, expected %d B %s %s" % (p, got[0], got[1], got[2], size, crc, sha))
            bad += 1
    for tg, sec, key, rel in EXIST:
        if not set(tg.split()) & set(targets):
            continue
        try:
            p = cfg.path(sec, key) / rel if rel else cfg.path(sec, key)
        except BuildError as e:
            say("  FAIL     %s" % e)
            bad += 1
            continue
        if p.exists():
            say("  ok       %s" % p)
        else:
            say("  MISSING  [%s] %s: %s" % (sec, key, p))
            bad += 1
    if "mac1010" in targets:
        try:
            import unicorn  # noqa: F401
            say("  ok       Python module unicorn %s" % unicorn.__version__)
        except ImportError:
            say("  MISSING  Python module unicorn (pip install unicorn==2.1.4)")
            bad += 1
    if set(targets) & {"win", "ce", "mac1010"}:
        try:
            rc, out = run(WINE + ["--version"], timeout=60)
            say("  %s %s" % ("ok      " if rc == 0 else "FAIL    ", out.strip() or "wine"))
            bad += rc != 0
        except BuildError as e:
            say("  MISSING  %s" % e)
            bad += 1
    return bad
