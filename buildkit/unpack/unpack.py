#!/usr/bin/env python3
"""unpack.py INPUTS DEST -- lay out every tool, library and game copy the kit reads, from the original
downloads in one folder (README.md lists them), the way their installers would, without running any.
Run by the Dockerfile; DEST is /opt/evnova, and build.ini names the folders below it. Every input is
optional: a target is laid out when all of its inputs are there, and DEST/targets.txt lists those."""
import hashlib
import shutil
import subprocess
import sys
from pathlib import Path

import installshield
import macfs

# the originals, as supplied: name in the inputs folder, SHA-256 (None: your own file, any content).
# The Xcode dmg is checked by the Dockerfile, which reads it first.
INPUTS = {
    "cw92": ("CW_Windows_9.2.exe", "86608e30a15f501065fc0e0aab55b0ac3372aa02818f7f37f2aa20e47dccd6dc"),
    "cw93": ("CW_Windows_9.3_Update.exe", "3b23182e13646d076829861f46fa963066f04ec639bc06fba6c02edd2df96467"),
    "cw9lic": ("CW_Windows_9_license.dat", None),
    "sym20": ("CodeWarrior_for_Symbian_OS_2.0_Personal_Edition.zip", "3739222b9da9da0209b8b536793cc777ed0d1856b5f132134374a1f40811cf88"),
    "sym30": ("CodeWarrior_Development_Studio_for_SymbianOS_3.0.zip", "764d379dd88707a0da045c1fb8425e3234c3e000c5b263c9129bff91d57c940c"),
    "pro6": ("CodeWarrior-Pro-6-win.zip", "739dc39e04a8b7ecc03dfc4213ec728d869f0787669ca6397e4b9bb96900775d"),
    "pro62": ("cw_pro_6.2_update_installer.sit", "3c2a8f1e0e8c04e1a7686b5dd566603861782e8b8d7f9a29d02622a6649e75b7"),
    "pro63": ("CodeWarrior6.image_.zip", "726adf30ec90b58d88d94840a7329eae33840ccd53b66ae82e5eda31abd4bdec"),
    "pro71": ("CW_Tools_7.1.toast_.zip", "4f045a820cff806a7727b503c4b1fdd850453378a5e472044aa2667aa99c3e8b"),
    "ic": ("ICProgKit2.0.2.dsk.zip", "88f811f705707bae38d22537254a72694706baeee7de837f2b4ecae634f9c543"),
    "carbonlib": ("CarbonLib_1.4GM_SDK.img.bin", "c7f540d64fd5dbd2540abcab0af1a9c60ec067cb591ea619329da83e34ad1d28"),
    "qt6": ("QT6SDK_Win.hqx", "fbcb3822ac4a16a5d21fa6575677b17cfa888bf937f4d59e8aa7ad75d72efb11"),
    "win1010": ("EV_Nova_1.0.10_for_Windows.zip", "ce3650391697be80968cea2527a612e6e8cd836b898c355724d2671af4a5cb05"),
    "mac1010": ("EV_Nova_1.0.10.sit", "33f363a32e50f704fb3865f4f0ab504b579ac14d1116e3b29fac293ac827d0aa"),
    "mac111": ("EV_Nova_1.1.1_for_Mac_OS_X.dmg", "39c7628d4b0ca99bb45f0b21f46e253da28847554747ba644ae3c9bb603528d5"),
}
# the inputs each target needs; mac111 also needs the Xcode dmg and mac111-source/, built by the Dockerfile
TARGETS = {
    "win": ["cw92", "cw93", "cw9lic", "sym20", "sym30", "qt6", "win1010"],
    "ce": ["cw92", "cw93", "cw9lic", "sym20", "sym30", "qt6", "win1010"],
    "mac1010": ["pro6", "pro62", "pro63", "pro71", "ic", "carbonlib", "mac1010"],
    "mac111": ["mac111"],
}
SDK = Path("/opt/applegcc/sdk/MacOSX10.4u.sdk")
XCODE = "xcode_3.2.6_and_ios_sdk_4.3.dmg and mac111-source/"
STUBS = Path("MacOS Support/Universal/Libraries/StubLibraries")
WORK = Path("/var/tmp/unpack")


def sh(*cmd):
    subprocess.run([str(c) for c in cmd], check=True, stdout=subprocess.DEVNULL)


def fresh(name):
    d = WORK / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    return d


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def stub(dest, name, data, rsrc):
    """a stub library as CodeWarrior on Windows keeps it: the resource fork in RESOURCE.FRK beside it"""
    write(dest / STUBS / name, data)
    write(dest / STUBS / "RESOURCE.FRK" / name, rsrc)


def check_inputs(src):
    """the inputs present, each checked; exits on a wrong copy"""
    have, bad = set(), 0
    for key, (name, sha) in INPUTS.items():
        p = src / name
        if not p.is_file():
            continue
        h = hashlib.sha256()
        with open(p, "rb") as f:
            for block in iter(lambda: f.read(1 << 20), b""):
                h.update(block)
        ok = sha is None or h.hexdigest() == sha
        print("  %s %s" % ("ok      " if ok else "WRONG   ", name))
        bad += not ok
        have.add(key)
    if bad:
        sys.exit("unpack: an input is not the expected copy (README.md lists them)")
    return have


def cw9(src, dest):
    # CodeWarrior for Windows 9.2 and its 9.3 update: mwcc, mwld, mwwinrc 3.2.4 build 418
    cw = dest / "cw-win-9.2-with-9.3-update"
    w = fresh("cw92")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["cw92"][0])
    installshield.groups(str(w / "data1.cab"), str(cw), ["Tools"])
    w = fresh("cw93")
    installshield.sfx7(src / INPUTS["cw93"][0], str(w))
    installshield.groups(str(w / "Disk1" / "data1.cab"), str(cw), ["Patch Files"])
    shutil.copyfile(src / INPUTS["cw9lic"][0], cw / "license.dat")


def sym20(src, dest):
    # CodeWarrior for Symbian Personal 2.0: mwccsym2 2.4.7 build 335 and FlexLM 3.26
    w = fresh("sym20")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["sym20"][0])
    installshield.groups(str(w / "data1.cab"), str(dest / "cw-symbian-2.0-personal"), ["!!ALL FILES!!"])


def sym30(src, dest):
    # CodeWarrior for Symbian OS 3.0: its Symbian Support MSL headers only
    w = fresh("sym30")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["sym30"][0], "Setup/data*")
    installshield.groups(str(w / "Setup" / "data1.cab"), str(dest / "cw-symbian-3.0" / "Symbian_Support"), ["Symbian Support"])


def pro6(src, dest):
    # CodeWarrior Pro 6 for Windows, its tools disc: MWLinkPPC, the stub libraries, Open Transport's PPC debug
    # libraries (CodeWarrior.zip), and from its installer the licence and FlexLM 3.26
    w = fresh("pro6")
    sh("7z", "e", "-y", "-o%s" % w, src / INPUTS["pro6"][0], "CodeWarrior Pro 6/Tools.iso")
    sh("7z", "e", "-y", "-o%s" % w, w / "Tools.iso", "CodeWarrior.zip", "data1.cab", "data1.hdr", "data2.cab")
    (w / "Tools.iso").unlink()
    pro6 = dest / "cw-mac-pro6"
    sh("7z", "x", "-y", "-o%s" % pro6, w / "CodeWarrior.zip",
       "Other Metrowerks Tools/Command Line Tools/MWLinkPPC.exe", "%s/*" % STUBS,
       "MacOS Support/OpenTransport/Open Tpt Client Developer/PPC Debug Libraries/*")
    installshield.groups(str(w / "data1.cab"), str(pro6), ["CodeWarrior IDE"], only="license.dat")
    installshield.groups(str(w / "data1.cab"), str(pro6), ["CodeWarrior IDE - Shared"], only="lmgr326b.dll")


def pro63_pro62(src, dest):
    # CodeWarrior Pro 6.3 (Mac): Universal Interfaces, MSL and the runtime, data forks; its licence
    w = fresh("pro63")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["pro63"][0])
    hfs = macfs.Hfs(next(w.rglob("CodeWarrior6.image")))
    pro63 = dest / "cw-mac-pro6.3"
    lic63 = None
    for parts, rec in hfs.files():
        if parts[:2] != ["Metrowerks CodeWarrior 6.0", "Metrowerks CodeWarrior"]:
            continue
        if parts[2:4] in (["MSL", "MSL_C"], ["MacOS Support", "Libraries"], ["MacOS Support", "Universal"]):
            write(pro63.joinpath(*[p.replace("/", "_") for p in parts[2:]]), hfs.forks(rec)[0])
        elif parts[2:] == ["license.dat"]:
            lic63 = hfs.forks(rec)[0]
    del hfs
    shutil.rmtree(w)
    # the Mac PPC compiler plugin of the CodeWarrior Pro 6.2 update: MW C/C++ PPC 2.4.1 build 0160, Pro 6.3's licence
    w = fresh("pro62")
    sh("unar", "-q", "-k", "visible", "-o", w, src / INPUTS["pro62"][0])
    data, rsrc = macfs.vise((w / "CW Pro 6.2 Update Installer").read_bytes(), "MW C/C++ PPC")
    plug = dest / "cw-mac-pro6.2-plugin"
    write(plug / "MW C++ PPC", data)
    write(plug / "MW C++ PPC.rsrc", rsrc)
    write(plug / "license.dat", lic63)


def pro71(src, dest):
    # the plugin of the CodeWarrior Pro 7.1 update: MW C/C++ PPC 2.4.6 build 222, and that disc's licence
    w = fresh("pro71")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["pro71"][0])
    hfs = macfs.Hfs(next(w.rglob("*.toast")))
    plug = dest / "cw-mac-pro7.1-plugin"
    for parts, rec in hfs.files():
        if parts == ["CW Pro 7.1 Update Installer"]:
            data, rsrc = macfs.vise(hfs.forks(rec)[0], "MW C-C++ PPC")
            write(plug / "MW C++ PPC", data)
            write(plug / "MW C++ PPC.rsrc", rsrc)
        elif parts == ["Metrowerks CodeWarrior", "license.dat"]:
            write(plug / "license.dat", hfs.forks(rec)[0])
    del hfs
    shutil.rmtree(w)


def apple_stubs(src, dest):
    # Apple's Internet Config 2.0.2 Programmer's Kit and CarbonLib 1.4 SDK: their stub libraries
    w = fresh("ic")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["ic"][0])
    hfs = macfs.Hfs(next(w.rglob("*.dsk")))
    for parts, rec in hfs.files():
        if parts == ["APIs", "InternetConfigLib"]:
            stub(dest / "apple-ic-sdk-2.0.2", "InternetConfigLib", *hfs.forks(rec))
    w = fresh("carbonlib")
    macfs.ndif(src / INPUTS["carbonlib"][0], w / "vol.raw")
    stub(dest / "apple-carbonlib-sdk-1.4", "CarbonLib",
         *macfs.HfsPlus(w / "vol.raw").file(["Carbon Support", "CarbonLib", "Stub", "CarbonLib"]))


def qt6(src, dest):
    # Apple's QuickTime 6 SDK for Windows (BinHex around a zip): qtmlClient.lib
    w = fresh("qt6")
    sh("unar", "-q", "-o", w, src / INPUTS["qt6"][0])
    sh("7z", "e", "-y", "-o%s" % (dest / "apple-quicktime-6-sdk"), w / "QT6SDK.zip",
       "QT6 SDK (Win)/Interfaces & Libraries/QTDevWin/Libraries/qtmlClient.lib")


def win1010(src, dest):
    w = fresh("win1010")
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["win1010"][0], "EV Nova 1.0.10 for Windows/EV Nova 1.0.10.exe")
    sh("7z", "e", "-y", "-o%s" % (dest / "game" / "win-1.0.10"), w / "EV Nova 1.0.10 for Windows" / "EV Nova 1.0.10.exe",
       "EV Nova.exe")


def mac1010(src, dest):
    w = fresh("mac1010")
    sh("unar", "-q", "-k", "visible", "-o", w, src / INPUTS["mac1010"][0])
    g = dest / "game" / "mac-1.0.10"
    write(g / "EV Nova", (w / "EV Nova 1.0.10" / "EV Nova").read_bytes())
    write(g / ".rsrc" / "EV Nova", macfs.appledouble_rsrc(w / "EV Nova 1.0.10" / "EV Nova.rsrc"))


def mac111(src, dest):
    w = fresh("mac111")
    app = "EV Nova/EV Nova.app/Contents/"
    names = ["AmbrosiaTools", "ASWDebug", "ShortcutRecorder"]
    sh("7z", "x", "-y", "-o%s" % w, src / INPUTS["mac111"][0], app + "MacOS/EV Nova",
       *[app + "Frameworks/%s.framework/Versions/A/%s" % (n, n) for n in names])
    (dest / "game").mkdir(parents=True, exist_ok=True)
    shutil.move(str(w / app), str(dest / "game" / "mac-1.1.1"))


# each unpacking step and the inputs it reads
STEPS = [(cw9, ["cw92", "cw93", "cw9lic"]), (sym20, ["sym20"]), (sym30, ["sym30"]),
         (pro6, ["pro6"]), (pro63_pro62, ["pro63", "pro62"]), (pro71, ["pro71"]), (apple_stubs, ["ic", "carbonlib"]), (qt6, ["qt6"]),
         (win1010, ["win1010"]), (mac1010, ["mac1010"]), (mac111, ["mac111"])]


def main(src, dest):
    src, dest = Path(src), Path(dest)
    print("checking the inputs in %s ..." % src)
    have = check_inputs(src)
    targets, used = [], set()
    for t, need in TARGETS.items():
        gone = [INPUTS[k][0] for k in need if k not in have] + ([XCODE] if t == "mac111" and not SDK.is_dir() else [])
        if gone:
            print("  %-8s left out: no %s" % (t, ", ".join(gone)))
        else:
            targets.append(t)
            used |= set(need)
    for step, keys in STEPS:
        if set(keys) <= used:
            print("unpacking: %s" % step.__name__)
            step(src, dest)
    shutil.rmtree(WORK, ignore_errors=True)
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "targets.txt").write_text("".join(t + "\n" for t in targets))
    print("unpack: done; targets: %s" % (" ".join(targets) or "none"))


if __name__ == "__main__":
    main(*sys.argv[1:3])
