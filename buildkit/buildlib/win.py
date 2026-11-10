"""EV Nova.exe 1.0.10 for Windows, and the EV Nova CE build (-DCEFIX), with the original
Metrowerks tools: mwcc 3.2.4 b418 for the game, mwccsym2 2.4.7 b335 for zlib/libpng/REGGIE,
mwwinrc for the resources, mwld for the archives and the link.

The steps are those of tools/build.ps1 and tools/link.ps1 (Research/the-link-step.md):
  1. compile every sources/**.c (not include/, mac/, v111/; cefix/ only for CE), each on the flag
     line of its `// COMPILE:` marker and the compiler of its `// COMPILER:` marker
  2. math_x87.obj: MSL's own source with cbrt's coefficient spelled as the exe's (tools/mslmath.py)
  3. the eleven QuickTime client-library members the exe interleaves, copied with the names the tree
     also defines renamed away (tools/qtorder.py)
  4. win: rt.lib and qt.lib by `mwld -library`, members and pull order from sources/link/*.txt;
     CE: the same order derived from the sources' FUNCTION markers (tools/pullorder.py --cefix)
  5. the resources: evnova.rc by mwwinrc, the icons and CHNK read out of the user's own exe
  6. the link, then the post-steps: the date pin, the signature copied from the user's exe (win),
     the large-address-aware bit (CE)
"""
import concurrent.futures
import re
import shutil
import struct

from . import coff, pe
from .host import KIT, REPO, BuildError, exe_env, run_exe, say, winlist, winpath

SOURCES = REPO / "sources"
BASELINE = ["-opt", "level=4,intrinsics", "-enum", "min", "-Cpp_exceptions", "off"]
MARKER = re.compile(r"^\s*//\s*COMPILE:\s*(.+)$")
COMPILER = re.compile(r"^\s*//\s*COMPILER:\s*(.+)$")
SYM335 = "mwccsym2 2.4.7 build 335"
SYM335_MSL2001 = "mwccsym2 2.4.7 build 335 + MSL 2001"
QT_SHADOW = "__qtmlshadow"


class Tools:
    """The Windows toolchain as build.ini names it."""

    def __init__(self, cfg):
        cw = self.cw = cfg.path("win", "codewarrior")
        cli = cw / "Other Metrowerks Tools" / "Command Line Tools"
        self.mwcc, self.mwld, self.mwwinrc = cli / "mwcc.exe", cli / "mwld.exe", cli / "mwwinrc.exe"
        self.mwccsym2 = cfg.path("win", "symbian") / "Other_Metrowerks_Tools" / "Command Line Tools" / "mwccsym2.exe"
        self.sym_msl = cfg.path("win", "symbian3") / "Symbian_Support" / "MSL"
        self.dlls = [cw / "Bin", cfg.path("win", "flexlm326")]       # LMGR8C.dll (418), LMGR326B.dll (335)
        self.license = cfg.path("win", "license")
        self.includes = [cw / p for p in (
            "MSL/MSL_C/MSL_Common/Include", "MSL/MSL_C/MSL_Win32/Include", "MSL/MSL_C/MSL_X86",
            "MSL/MSL_C++/MSL_Common/Include", "MSL/MSL_Extras/MSL_Common/Include", "MSL/MSL_Extras/MSL_Win32/Include",
            "Win32-x86 Support/Headers/Win32 SDK", "Win32-x86 Support/Headers/Precompiled Headers",
            "Win32-x86 Support/Headers/Prefix Files", "Win32-x86 Support/Headers/VCPP Headers",
            "Win32-x86 Support/Headers/Alloca", "Win32-x86 Support/Headers/Intrinsics", "MSL/MSL_C++/MSL_Precompiled_Headers")]
        # libpng 1.2.7 was built against the 2001 MSL that CodeWarrior for Symbian v3.0 ships
        self.includes_msl2001 = [self.sym_msl / p for p in (
            "MSL_C/MSL_Common/include", "MSL_C/MSL_Win32/include", "MSL_C/MSL_x86", "MSL_CPP/MSL_Common/include")] + \
            [cw / p for p in ("Win32-x86 Support/Headers/Win32 SDK", "Win32-x86 Support/Headers/Prefix Files",
                              "Win32-x86 Support/Headers/VCPP Headers")]
        self.libdirs = [cw / p for p in (
            "Win32-x86 Support/Libraries/Runtime/Libs", "Win32-x86 Support/Libraries/Win32 SDK",
            "MSL/MSL_C/MSL_Win32/Lib/x86", "MSL/MSL_C++/MSL_Win32/Lib/x86", "MSL/MSL_Extras/MSL_Win32/Lib/x86")]

    def env(self, **more):
        base = dict(LM_LICENSE_FILE=winpath(self.license), METROWKS_LICENSE_FILE=winpath(self.license),
                    MWCWinx86Includes=winlist(self.includes))
        base.update(more)
        return exe_env(self.dlls, **base)


# -- 1. compile ------------------------------------------------------------------------------------
def sources_for(cefix):
    out, stems = [], {}
    for p in sorted(SOURCES.rglob("*.c")):
        parts = p.relative_to(SOURCES).parts
        if parts[0] in ("include", "mac", "v111") or (parts[0] == "cefix" and not cefix):
            continue
        if p.stem.lower() in stems:
            raise BuildError("two sources with the stem %s: %s, %s" % (p.stem, stems[p.stem.lower()], p))
        stems[p.stem.lower()] = p
        out.append(p)
    return out


def compile_one(t, src, objdir, cefix):
    head = src.read_text(encoding="latin-1").splitlines()[:30]
    flags = next((MARKER.match(l).group(1).split() for l in head if MARKER.match(l)), list(BASELINE))
    flags = flags + ["-i", winpath(SOURCES / "include")] + (["-DCEFIX"] if cefix else [])
    which = next((COMPILER.match(l).group(1).strip() for l in head if COMPILER.match(l)), None)
    cc, includes = t.mwcc, t.includes
    if which in (SYM335, SYM335_MSL2001):
        cc = t.mwccsym2
        if which == SYM335_MSL2001:
            includes = t.includes_msl2001
            flags.append("-D_MSL_TRUE=1")        # that MSL's cerrno names a macro its ansi_parms.h never defines
    elif which:
        raise BuildError("%s: unknown // COMPILER: %r" % (src.name, which))
    obj = objdir / (src.stem + ".o")
    if obj.exists():
        obj.unlink()
    rc, msg = run_exe(cc, ["-c"] + flags + ["-o", winpath(obj), winpath(src)], env=t.env(MWCSym2Includes=winlist(includes)))
    if rc or not obj.exists():
        if obj.exists():
            obj.unlink()                         # never leave an object that would link as if current
        raise BuildError("compile failed: %s\n%s" % (src.relative_to(REPO), msg[-3000:]))
    return src


def compile_all(cfg, objdir, cefix):
    t = Tools(cfg)
    objdir.mkdir(parents=True, exist_ok=True)
    for old in objdir.glob("*.o"):
        old.unlink()
    srcs = sources_for(cefix)
    say("  compiling %d files (%d at a time) ..." % (len(srcs), cfg.jobs("win")))
    with concurrent.futures.ThreadPoolExecutor(cfg.jobs("win")) as ex:
        for f in [ex.submit(compile_one, t, s, objdir, cefix) for s in srcs]:
            f.result()
    return t


# -- 2. MSL's math_x87 with the exe's cbrt coefficient ---------------------------------------------
def math_x87(t, outdir):
    """mwcc converts 0.1950252994681516 through its 80-bit mantissa onto a rounding tie, so every stock
    MSL_All_x86.lib holds pool byte 0x58 where the exe has 0x57; the longer spelling converts to the exe's."""
    src = outdir / "src"
    shutil.rmtree(src, ignore_errors=True)
    src.mkdir(parents=True)
    msl = t.cw / "MSL" / "MSL_C"
    shutil.copyfile(msl / "MSL_X86" / "math_x87.c", src / "math_x87.c")
    text = (msl / "MSL_Common" / "Src" / "math.c").read_bytes()
    old, new = b"0.1950252994681516", b"0.19502529946815159"
    if text.count(old) != 1:
        raise BuildError("MSL math.c: cbrt's coefficient found %d times, expected once" % text.count(old))
    (src / "math.c").write_bytes(text.replace(old, new))
    (src / "pfx.h").write_text("#pragma avoid_ebp on\n")
    inc = [src] + [t.cw / p for p in (
        "MSL/MSL_C/MSL_Common/Include", "MSL/MSL_C/MSL_Win32/Include", "MSL/MSL_C/MSL_X86", "MSL/MSL_C/MSL_Common/Src",
        "MSL/MSL_C/MSL_Win32/Src", "MSL/MSL_C++/MSL_Common/Include", "MSL/MSL_Extras/MSL_Common/Include",
        "MSL/MSL_Extras/MSL_Win32/Include", "Win32-x86 Support/Headers/Win32 SDK", "Win32-x86 Support/Headers/Prefix Files")]
    obj = outdir / "math_x87.obj"
    rc, msg = run_exe(t.mwcc, ["-c", "-opt", "level=4,intrinsics", "-lang", "c99", "-enum", "min",
                               "-Cpp_exceptions", "off", "-str", "noreadonly", "-prefix", winpath(src / "pfx.h"),
                               winpath(src / "math_x87.c"), "-o", winpath(obj)], env=t.env(MWCWinx86Includes=winlist(inc)))
    if rc or not obj.exists():
        raise BuildError("math_x87.c failed to compile\n" + msg[-2000:])
    # the library member carries no .drectve text: empty the section (mwld ignores a zero-sized one)
    b = bytearray(obj.read_bytes())
    nsec, _ts, _symp, _nsym, opt = struct.unpack_from("<HIIIH", b, 2)
    for k in range(nsec):
        o = 20 + opt + 40 * k
        if bytes(b[o:o + 8]).rstrip(b"\0") == b".drectve":
            raw, ptr = struct.unpack_from("<II", b, o + 16)
            b[ptr:ptr + raw] = b"\0" * raw
            struct.pack_into("<I", b, o + 16, 0)
    obj.write_bytes(bytes(b))
    return obj


# -- 3. the QuickTime client-library members -------------------------------------------------------
def table(name):
    """A tab-separated table of buildkit/tables/ (the exe's orders, read off the exe once): rows of fields."""
    rows = []
    for line in (KIT / "tables" / name).read_text().splitlines():
        if line and not line.startswith("#"):
            rows.append(line.split("\t"))
    return rows


def qt_members(qtlib, objdir, outdir):
    """Write qt_<member> for every row of tables/qtorder.tsv; return [(exe VA, path)]."""
    lib = {}
    for name, body in coff.members(qtlib / "qtmlClient.lib"):
        lib.setdefault(name, []).append(body)
    tree = set()
    for o in objdir.glob("*.o"):
        tree |= coff.defined(o.read_bytes())
    outdir.mkdir(parents=True, exist_ok=True)
    out = []
    for row in table("qtorder.tsv"):
        name, va = row[0], int(row[1], 16)
        ren = dict(p.split("=") for p in row[2].split(",")) if len(row) > 2 and row[2] else {}
        if len(lib.get(name, [])) != 1:
            raise BuildError("qtmlClient.lib: %d members named %s" % (len(lib.get(name, [])), name))
        b = lib[name][0]
        defs = {n for _i, n, sec, sc, _v in coff.symbols(b) if sc == coff.EXTERNAL and sec > 0}
        allren = dict(ren)
        allren.update({n: QT_SHADOW + n for n in defs if n not in ren and n in tree})
        if allren:
            b = coff.rename(b, allren)
        p = outdir / ("qt_" + name)
        p.write_bytes(b)
        out.append((va, p))
    return out


# -- 4. the link order -----------------------------------------------------------------------------
def mwld_library(t, lib, members):
    rsp = lib.with_suffix(".rsp")
    rsp.write_text("".join('"%s"\n' % winpath(m) for m in members))
    if lib.exists():
        lib.unlink()
    rc, msg = run_exe(t.mwld, ["-library", "-nostdlib", "-o", winpath(lib), "@" + winpath(rsp)], env=t.env())
    if not lib.exists():
        raise BuildError("mwld -library did not write %s\n%s" % (lib.name, msg[-2000:]))
    return lib


def win_order(t, objdir, work, mslobj, qtdir):
    """The committed order: sources/link/objects-win.txt and pull-win.txt. Returns (inputs, -include names)."""
    plain, rt, qt, listed = [], [], [], set()
    for line in (SOURCES / "link" / "objects-win.txt").read_text().splitlines():
        if not line.strip():
            continue
        kind, name = line.split(" ", 1)
        p = {"obj": objdir / (name + ".o"), "rt": objdir / (name + ".o"),
             "msl": mslobj.parent / name, "qt": qtdir / ("qt_" + name)}.get(kind)
        if p is None:
            raise BuildError("sources/link/objects-win.txt: unknown line %r" % line)
        if not p.exists():
            raise BuildError("sources/link/objects-win.txt lists %s, which was not built" % p.name)
        if kind in ("obj", "rt"):
            listed.add(name.lower())
        (plain if kind == "obj" else qt if kind == "qt" else rt).append(p)
    extra = sorted(o.name for o in objdir.glob("*.o") if o.stem.lower() not in listed)
    if extra:
        raise BuildError("objects not in sources/link/objects-win.txt: %s" % " ".join(extra))
    pulls = [l for l in (SOURCES / "link" / "pull-win.txt").read_text().splitlines() if l]
    return plain + [mwld_library(t, work / "rt.lib", rt), mwld_library(t, work / "qt.lib", qt)], pulls


MARK = re.compile(r"//\s*FUNCTION:\s*EVNOVA\s+0x([0-9a-fA-F]+)")
ANCHOR = re.compile(r"^//\s*LINK:\s*(after|before)\s+(\w+)\s*;\s*data\s+EVNOVA\s+0x([0-9a-fA-F]+)", re.M)


def ce_order(t, objdir, work, mslobj, qtmembers):
    """The CE link has no committed order: the exe's order derived as tools/pullorder.py --cefix does.
    Plain objects sort by their source's lowest FUNCTION address (a `// LINK:` anchor sits by its
    neighbour); the runtime units, MSL and QuickTime members and import thunks are pulled in exe order
    by a first input that only names them (driver.obj); the imports first, in the exe's IAT order."""
    lo, anchors = {}, {}
    for p in SOURCES.rglob("*.c"):
        parts = p.relative_to(SOURCES).parts
        if "include" in parts or parts[0] == "mac":
            continue
        text = p.read_text(encoding="latin-1", errors="replace")
        a = [int(x, 16) for x in MARK.findall(text)]
        lo[p.stem.lower()] = min(a) if a else None
        m = ANCHOR.search("\n".join(text.splitlines()[:30]))
        if m and not a:
            anchors[p.stem.lower()] = (m.group(1), m.group(2).lower(), int(m.group(3), 16))

    def plain_key(o):
        s = o.stem.lower()
        if s in anchors:
            side, n, va = anchors[s]
            return (False, lo[n], 0 if side == "before" else 2, va, o.name.lower())
        return (lo[s] is None, lo[s] or 0, 1, 0, o.name.lower())
    runtime = {p.stem.lower() for p in (SOURCES / "runtime").glob("*.c")}
    units, plain, wanted = [], [], set()
    for o in sorted(objdir.glob("*.o"), key=plain_key):
        b = o.read_bytes()
        wanted |= coff.undefined(b)
        s = o.stem.lower()
        if s not in runtime:
            plain.append(o)
            continue
        sym = coff.pull_symbol(b)
        if sym is None:
            raise BuildError("%s has no global symbol to pull it by" % o.name)
        if s in anchors:
            side, n, va = anchors[s]
            key = (lo[n], 0 if side == "before" else 2, va)
        else:
            key = (lo[s], 1, 0) if lo.get(s) is not None else (1 << 40, 1, 0)
        units.append((key, sym, o))
    msl = dict(coff.members(t.libdirs[0] / "MSL_All_x86.lib")[::-1])     # first member of a name wins
    for name, va in table("mslorder.tsv"):
        mine = mslobj if name == mslobj.name else None
        b = mine.read_bytes() if mine else msl[name]
        wanted |= coff.undefined(b)
        units.append(((int(va, 16), 1, 0), coff.pull_symbol(b), mine))
    for va, p in qtmembers:
        b = p.read_bytes()
        wanted |= coff.undefined(b)
        units.append(((va, 1, 0), coff.pull_symbol(b, avoid=QT_SHADOW), p))
    thunked = set()
    for sym, va, _dll in table("thunkorder.tsv"):
        if sym in wanted or "__imp_" + sym in wanted:
            units.append(((int(va, 16), 1, 0), sym, None))
            thunked.add("__imp_" + sym)
    units.sort(key=lambda u: u[0])
    imports = [r[0] for r in table("iatorder.tsv") if r[0].startswith("__IMPORT_DESCRIPTOR_") or
               (r[0] not in thunked and (r[0] in wanted or r[0][len("__imp_"):] in wanted))]
    driver = work / "driver.obj"
    coff.write_undefined_object(driver, imports + [u[1] for u in units])
    rt = [o for _k, _s, o in units if o and not o.name.startswith("qt_")]
    qt = [o for _k, _s, o in units if o and o.name.startswith("qt_")]
    coff.write_archive(work / "rt.lib", [(o.name, o.read_bytes()) for o in rt])
    coff.write_archive(work / "qt.lib", [(o.name, o.read_bytes()) for o in qt])
    return [driver] + plain + [work / "rt.lib", work / "qt.lib"], []


# -- 5. resources ----------------------------------------------------------------------------------
TYPE_NAME = {3: "ICON", 14: "GROUP_ICON"}


def resources(t, exe, work):
    """novaport.res: VERSION and STRING from sources/rsrc/evnova.rc, ICON, GROUP_ICON and CHNK read out
    of the user's exe into work/rsrc/ (never stored anywhere else)."""
    rsrc = work / "rsrc"
    rsrc.mkdir(parents=True, exist_ok=True)
    assets = [(ty, rid, b) for ty, rid, b in pe.resources(exe) if ty not in (6, 16)]
    if sorted((str(ty), rid) for ty, rid, _b in assets) != [("14", 202), ("3", 1), ("CHNK", 950)]:
        raise BuildError("%s: not the 1.0.10 resource set (ICON 1, GROUP_ICON 202, CHNK 950)" % exe)
    for ty, rid, b in assets:
        (rsrc / ("%s_%s.bin" % (TYPE_NAME.get(ty, ty), rid))).write_bytes(b)
    res = work / "novaport.res"
    rc, msg = run_exe(t.mwwinrc, ["-cwd", "proj", "-d", "EVNOVA_ASSETS", "-o", winpath(res),
                                  winpath(SOURCES / "rsrc" / "evnova.rc")], cwd=work, env=t.env())
    if rc or not res.exists():
        raise BuildError("mwwinrc failed\n" + msg[-2000:])
    return res


# -- 6. the link -----------------------------------------------------------------------------------
LIBS = ["MSL_All_x86.lib", "qtmlClient.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "winmm.lib", "wsock32.lib",
        "advapi32.lib", "comctl32.lib", "shell32.lib", "version.lib", "comdlg32.lib", "iphlpapi.lib", "ole32.lib",
        "oleaut32.lib", "opengl32.lib"]


def link(cfg, objdir, work, out_exe, cefix):
    t = Tools(cfg)
    work.mkdir(parents=True, exist_ok=True)
    exe = cfg.path("game", "win1010") / "EV Nova.exe"
    mslobj = math_x87(t, work / "mslmath")
    qtm = qt_members(cfg.path("win", "quicktime"), objdir, work / "qt")
    if cefix:
        inputs, pulls = ce_order(t, objdir, work, mslobj, qtm)
    else:
        inputs, pulls = win_order(t, objdir, work, mslobj, work / "qt")
    inputs = inputs + [resources(t, exe, work)]
    # iconv.dll is loaded through an import library made from tables/iconv.def (the hints of the DLL the
    # exe was linked against); CE draws text without it, and then the DLL leaves the import table
    if any("libiconv" in n for o in objdir.glob("*.o") for n in coff.undefined(o.read_bytes())):
        lib = work / "iconv_imp.lib"
        if lib.exists():
            lib.unlink()
        run_exe(t.mwld, ["-importlib", "-o", winpath(lib), "-name", "iconv.dll", winpath(KIT / "tables" / "iconv.def")], env=t.env())
        if not lib.exists():
            raise BuildError("mwld -importlib did not write iconv_imp.lib")
        inputs.append(lib)
    rsp = work / "link.rsp"
    rsp.write_text("".join("-include %s\n" % n for n in pulls) + "".join('"%s"\n' % winpath(p) for p in inputs))
    if out_exe.exists():
        out_exe.unlink()
    libdirs = t.libdirs + [cfg.path("win", "quicktime")]
    env = t.env(MWWinx86Libraries=winlist(libdirs), MWWinx86LibraryFiles=";".join(LIBS + (["GdiPlus.lib"] if cefix else [])))
    say("  linking %d inputs ..." % len(inputs))
    # -filealign 512 and -checksum: the original's header (FileAlignment 512, CheckSum set)
    rc, log = run_exe(t.mwld, ["-subsystem", "windows", "-subsysver", "4,0", "-filealign", "512", "-checksum",
                               "-o", winpath(out_exe), "-map", winpath(work / "link.map"), "-maxerrors", "0",
                               "-maxwarnings", "0", "@" + winpath(rsp)], env=env)
    (work / "link.log").write_text(log)
    undef = sorted(set(re.findall(r"Undefined symbol:\s*'([^']+)'", log)))
    if undef:
        raise BuildError("link: %d undefined symbols, first %s (log: %s)" % (len(undef), " ".join(undef[:8]), work / "link.log"))
    if rc or not out_exe.exists():
        raise BuildError("link failed (log: %s)\n%s" % (work / "link.log", log[-2000:]))
    # mwld keeps one definition of a clash, warns and exits 0. The one expected kind: a qtmlClient.lib
    # Toolbox-shim COMDAT losing to the port's own function of that name
    blocks = re.findall(r"Multiply defined section:\s*(\S+) in\s*\r?\n#\s*files ([^\r\n]*)\r?\n#\s*keeping definition in ([^\r\n]*)", log)
    bad = [b[0] for b in blocks if not ("(qtmlClient.lib)" in b[1] and re.match(r"^[A-Za-z0-9_]+\.o$", b[2].strip()))]
    if bad or len(blocks) != log.count("Multiply defined section:"):
        raise BuildError("link: a definition was silently displaced: %s (log: %s)" % (" ".join(bad) or "?", work / "link.log"))
    return exe


def build(cfg, out, cefix=False, pin_date=True, signed=True):
    """Compile, link and finish one Windows target into OUT; returns the exe's path."""
    name = "EV Nova CE.exe" if cefix else "EV Nova.exe"
    objdir, work, exe = out / "obj", out / "link", out / name
    say("  [1/3] compile" + (" (-DCEFIX, sources/cefix/ included)" if cefix else ""))
    compile_all(cfg, objdir, cefix)
    say("  [2/3] link")
    original = link(cfg, objdir, work, exe, cefix)
    say("  [3/3] post-steps")
    if cefix:
        pe.set_large_address_aware(exe)      # CE links with --large-address-aware for its 2,040 MB pool
    if pin_date:
        pe.set_stamp(exe)                    # mwld writes the link time; the original's date instead
    if signed and not cefix:
        pe.copy_signature(exe, original)
    return exe
