# EV Nova build kit: input files

Files needed to build this game. You only need the files of the targets you want.
Check a copy with `sha256sum -c --ignore-missing SHA256SUMS`, or `7z h FILE` for the CRC32.

## win and ce: EV Nova 1.0.10 for Windows, and the CE build of the same sources

| file | source | bytes | CRC32 |
|---|---|---|---|
| `CW_Windows_9.2.exe` | https://archive.org/download/cwpro9/Metrowerks%20CodeWarrior%20for%20Windows%209.4.7z/Metrowerks%20CodeWarrior%20for%20Windows%209.4%2FCW_Windows_9.2.exe (archive.org item `cwpro9`) | 221461396 | B21A5C36 |
| `CW_Windows_9.3_Update.exe` | https://archive.org/download/ftp-metrowerks-updates-archive/ftp_metrowerks_updates.7z/Metrowerks%2FCWWindows9%2FCW_Windows_9.3_Update.exe (archive.org item `ftp-metrowerks-updates-archive`, Metrowerks' `ftp.metrowerks.com/pub/updates/` of late 2005) | 54669609 | FBD8BBBD |
| `CW_Windows_9_license.dat` (your CodeWarrior 9 `license.dat`, renamed; any one with the `Win32_Plugins_Win32` 9.0 features) | https://archive.org/download/cwpro9/Metrowerks%20CodeWarrior%20for%20Windows%209.4.7z/Metrowerks%20CodeWarrior%20for%20Windows%209.4%2Flicense.dat (archive.org item `cwpro9`; 452 B, CRC32 F97B7AF0) | any | any |
| `CodeWarrior_for_Symbian_OS_2.0_Personal_Edition.zip` | https://archive.org/download/nokia_sdks_n_dev_tools/CodeWarrior_for_Symbian_OS_2.0_Personal_Edition.zip | 46083242 | 6443CAA0 |
| `CodeWarrior_Development_Studio_for_SymbianOS_3.0.zip` | https://archive.org/details/cw_mobile (also in `nokia_sdks_n_dev_tools`) | 96671512 | 5951EC23 |
| `QT6SDK_Win.hqx` | https://discmaster.textfiles.com/browse/17099/staticky.com.zip/staticky.com/staticky.com/mirrors/ftp.apple.com/developer/Development_Kits/QT6SDK_Win.hqx | 4425850 | E83855BE |
| `EV_Nova_1.0.10_for_Windows.zip` | https://macintoshgarden.org/games/escape-velocity-nova | 77558533 | 295A9F47 |

## mac1010: EV Nova 1.0.10 for Mac OS (Classic and Carbon)

| file | source | bytes | CRC32 |
|---|---|---|---|
| `cw_pro_6.2_update_installer.sit` | https://macintoshgarden.org/apps/codewarrior-6 | 70199892 | 4ED7DBEB |
| `CW_Tools_7.1.toast_.zip` | https://macintoshgarden.org/apps/codewarrior-pro-71 | 237306829 | E01B2E12 |
| `CodeWarrior6.image_.zip` | https://macintoshgarden.org/apps/codewarrior-6 | 815731118 | 4662DFFD |
| `CodeWarrior-Pro-6-win.zip` | https://macintoshgarden.org/apps/codewarrior-6 | 588358423 | 1F3F368E |
| `ICProgKit2.0.2.dsk.zip` | https://www.gryphel.com/c/sw/sysutils/netcnfg/ | 552957 | 1A803C34 |
| `CarbonLib_1.4GM_SDK.img.bin` | https://discmaster.textfiles.com/browse/17099/staticky.com.zip/staticky.com/staticky.com/mirrors/ftp.apple.com/developer/Development_Kits/CarbonLib_1.4GM_SDK.img.bin | 11819392 | A49C404A |
| `EV_Nova_1.0.10.sit` | https://macintoshgarden.org/games/escape-velocity-nova | 96395399 | 857DA2C1 |

## mac111: EV Nova 1.1.1 for Mac OS X (i386 and ppc)

| file | source | bytes | CRC32 |
|---|---|---|---|
| `xcode_3.2.6_and_ios_sdk_4.3.dmg` | https://archive.org/download/0-GFortran-Xcode-GCC/xcode_3.2.6_and_ios_sdk_4.3.dmg | 4443150993 | A90B7BB9 |
| `EV_Nova_1.1.1_for_Mac_OS_X.dmg` | https://macintoshgarden.org/games/escape-velocity-nova | 94465034 | C72B4DEC |
| `mac111-source/gcc_40-5494.tar.gz` | https://distfiles.macports.org/apple-gcc40/gcc_40-5494.tar.gz | 40546880 | 81A5BDF4 |
| `mac111-source/cctools-port-f01442232f185291c671578a81d74fe60eb189a2.tar.gz` | https://github.com/tpoechtrager/cctools-port/archive/f01442232f185291c671578a81d74fe60eb189a2.tar.gz | 2613510 | 9E66FB14 |
| `mac111-source/ld64-97.17.tar.gz` (a browser saves it as `ld64-ld64-97.17.tar.gz`: rename it) | https://github.com/apple-oss-distributions/ld64/archive/refs/tags/ld64-97.17.tar.gz | 418239 | B0676544 |
| `mac111-source/ld64-97-standalone-libunwind-headers.patch` | https://distfiles.macports.org/ld64/ld64-97-standalone-libunwind-headers.patch | 141802 | 4A84F00E |
